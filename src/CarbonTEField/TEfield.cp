/* Routines for handling a text edit field with vertical scroll bar, by
 * John Gibson. Based on Macintosh Revealed, vol. 2, by Stephen Chernicoff.
 *	"Smart quotes" option (not used in this comment) added by Don Byrd, 10/92.
 * ScrollToSelection made public, 2/93.
 *
 * Revised for CW Pro 5 by JGG, 5/13/00, including:
 *		- Remove old code not enabled by Nightmare95 define (also removed)
 *		- Replace OLDROUTINENAMES with new ones.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "TEfieldPub.h"
#include "TEfieldPriv.h"

#include <string.h>	/* for strlen only */

#define SMART_QUOTES

/* local prototypes */
static void AdjustScrollBar(PEDITFIELD);
static void AdjustText(TEHandle, ControlHandle);
static void DoScroll(PEDITFIELD, short, Point);
static pascal Boolean EditFieldAutoScroll(TERec*);
static pascal void ScrollText(ControlHandle, short);
static void ScrollCharacter(PEDITFIELD, short, Boolean);
//int ConvertQuote(TEHandle, int);

static Boolean			scrapDirty;
static short			scrapCompare;

/* For current TE field, so EditFieldAutoScroll, ScrollText and dependents can get them */ 
static ControlHandle	tmpScrollH;
static TEHandle		tmpTEH;

static TEClickLoopUPP clickUPP = NULL;


/* NB: Call this only after you've SetPort to your window's port! */
Boolean CreateEditField(
		WindowPtr		wind,
		Rect				box,				/* enclosing rect, including scroll bar, in local ccords */
		short				fontNum,
		short				fontSize,
		short				fontStyle,
		Handle			textH,			/* put this text in the field */
		Boolean			sel,				/* TRUE: selAll, FALSE: insertion pt at end */
		PEDITFIELD		theField
		)
{
	Rect				viewRect, destRect, scrollRect, boundsRect;
	TEHandle			teH;
	ControlHandle	scrollH;
	Size				len;
	FontInfo			finfo;
	short				oldFont, oldSize, oldStyle, lineHeight, numLines, extraPixels;
	
	/* get font characteristics */
	oldFont = GetPortTxFont(); oldSize = GetPortTxSize(); oldStyle = GetPortTxFace();
	TextFont(fontNum); TextSize(fontSize); TextFace(fontStyle);
	GetFontInfo(&finfo);
	TextFont(oldFont); TextSize(oldSize); TextFace(oldStyle);
	lineHeight = finfo.ascent + finfo.descent + finfo.leading;

	/* set box size to fit an integral number of lines */
	numLines = (box.bottom-box.top) / lineHeight;				/* lines of text that will fit in box */
	extraPixels = (box.bottom-box.top) % lineHeight;			/* remaining pixels */
	box.bottom -= extraPixels;
	
/* *** Should allow a border around all sides. (When you select text, you'll
still see a white border around edges, like in THINK C.) */
	/* set up rectangles */
	viewRect = box;
	viewRect.right -= S_BAR_WIDTH-1;
	boundsRect = box;
	InsetRect(&boundsRect, -1, -1);
	scrollRect = boundsRect;
	scrollRect.left = scrollRect.right - S_BAR_WIDTH;
	destRect = viewRect;
	InsetRect(&destRect, TEXTMARGIN, 0);
	
	/* create TE field and scrollbar */
	teH = TENew(&destRect, &viewRect);
	scrollH = NewControl(wind, &scrollRect, "\p", TRUE, 0, 0, 100, scrollBarProc, 0L);
	if (teH == NIL || scrollH == NIL) {
		SysBeep(10);		/* ***FIXME: need more than this! */
		return FALSE;
	}
	
	/* set font for TE record */
	(**teH).txFont = fontNum;
	(**teH).txSize = fontSize;
	(**teH).txFace = (char)fontStyle;
	(**teH).lineHeight = lineHeight;
	(**teH).fontAscent = finfo.ascent;

	/* stuff text into field */
	if (textH != NIL) {
		len = GetHandleSize(textH);
		HLock(textH);
		TESetText(*textH, (long) len, teH);
		TESetSelect(sel? 0L : (long)32767, (long)32767, teH);
		HUnlock(textH);
	}

	/* Allocate the routine descriptor just once, since routine could be shared. */
	if (clickUPP == NIL)
		clickUPP = NewTEClickLoopUPP(EditFieldAutoScroll);
	if (clickUPP)
		TESetClickLoop(clickUPP, teH);
	
	/* store into struct for caller */
	theField->teH = teH;
	theField->scrollH = scrollH;
	theField->bounds = boundsRect;
	theField->active = TRUE;
	
	AdjustScrollBar(theField);
	
	return TRUE;
}


void DisposeEditField(PEDITFIELD theField)
{
	TEDispose(theField->teH);
	DisposeControl(theField->scrollH);
}


/* Stuffs into the edit field the text contained in either <textH>, a handle
 * to raw text (not a Pascal or C string), or <textP>, a (null terminated) 
 * C string. The choice is yours; pass NIL for the one not used.  This 
 * replaces any text already in the field. Returns TRUE if ok, FALSE if error.
 */

/* ¥¥¥¥¥¥InsideMac says:
TESetText doesn't dispose of any text currently in the edit record.
Any repercussions for us?
*/

Boolean SetEditFieldText(PEDITFIELD theField, Handle textH, unsigned char *textP, Boolean sel)
{
	long	len;

	if (textH != NIL) {
		len = (long) GetHandleSize(textH);
		HLock(textH);
		TESetText(*textH, len, theField->teH);
		HUnlock(textH);
	}
	else if (textP != NIL) {
		len = strlen((char *)textP);
		TESetText(textH, len, theField->teH);
	}
	else
		return FALSE;

	TESetSelect(sel? 0L : (long)32767, (long)32767, theField->teH);
	AdjustScrollBar(theField);
	return TRUE;
}


/* Returns a new handle containing the entire text of the edit field.
 * Caller should dispose of this handle when finished with it.
 * Handle will be NIL if there's a problem.
 */
/*¥¥¥¥¥¥¥¥should make caller allocate handle and pass it in. Then resize it.
Otherwise, if caller wants to keep using the same handle (like flow-in),
they'll have to Dispose the old one...*/
CharsHandle GetEditFieldText(PEDITFIELD theField)
{
	CharsHandle	textH;
	OSErr			err;
	
	textH = TEGetText(theField->teH);
	err = HandToHand(&textH);
	if (err == noErr)
		return textH;
	else
		return (char**)NIL;
}


static void AdjustScrollBar(PEDITFIELD theField)
{
	TEHandle			teH;
	ControlHandle	scrollH;
	short				windowHt, maxTop;
	
	teH = theField->teH;
	scrollH = theField->scrollH;
	
	windowHt = ((**teH).viewRect.bottom - (**teH).viewRect.top) / (**teH).lineHeight;
	maxTop = (**teH).nLines - windowHt;			/* avoid white space at bottom */
	
	if (maxTop<=0) {									/* number of lines less than what will fit in view rect? */
		maxTop = 0;
		HiliteControl(scrollH, CTL_INACTIVE);
	}
	else
		HiliteControl(scrollH, CTL_ACTIVE);
	
	SetControlMaximum(scrollH, maxTop);			/* adjust range of scroll bar */
}


static void AdjustText(TEHandle teH, ControlHandle scrollH)
{
	short oldScroll, newScroll;
	
	HLock((Handle)teH);
	oldScroll = (**teH).viewRect.top - (**teH).destRect.top;
	newScroll = GetControlValue(scrollH) * (**teH).lineHeight;
	TEScroll(0, (oldScroll-newScroll), teH);
	HUnlock((Handle)teH);
}


static void DoScroll(PEDITFIELD theField, short part, Point pt)
{
	TEHandle				teH;
	ControlHandle		scrollH;
	ControlActionUPP	scrollUPP;

	teH = theField->teH;
	scrollH = theField->scrollH;
	if (part==kControlIndicatorPart) {
		part = TrackControl(scrollH, pt, (ControlActionUPP)NIL);
		AdjustText(teH, scrollH);
	}
	else {
		RgnHandle	saveClip;
		
		/* Must set clip rgn equal to whole window; otherwise
		 * scrollbar won't update.
		 */
		saveClip = NewRgn();
		GetClip(saveClip);
//		ClipRect(&qd.thePort->portRect);
		Rect bounds = GetQDPortBounds();
		ClipRect(&bounds);
		
		scrollUPP = NewControlActionUPP(ScrollText);
		part = TrackControl(scrollH, pt, scrollUPP);	/* use action proc for continuous scroll */
		if (scrollUPP)
			//DisposeRoutineDescriptor(scrollUPP);
			DisposeControlActionUPP(scrollUPP);
		
		SetClip(saveClip);
		DisposeRgn(saveClip);
	}
}


static pascal void ScrollText(ControlHandle scrollH, short part)
{
	short		delta, oldValue;
	TEHandle	teH;
	
	teH = tmpTEH;

	switch (part) {
		case kControlUpButtonPart:
			delta = -1;
			break;
		case kControlDownButtonPart:
			delta = 1;
			break;
		case kControlPageUpPart:
			delta = ((**teH).viewRect.top - (**teH).viewRect.bottom) / (**teH).lineHeight + 1;
			break;
		case kControlPageDownPart:
			delta = ((**teH).viewRect.bottom - (**teH).viewRect.top) / (**teH).lineHeight - 1;
			break;
		default:
			break;
	}
	
	if (part!=0) {
		oldValue = GetControlValue(scrollH);
		SetControlValue(scrollH, oldValue + delta);	/* adjust by scroll amount */
		AdjustText(teH, scrollH);
	}
}


/* Click loop routine--handles automatic scrolling during text selection. */
static pascal Boolean EditFieldAutoScroll(TERec*)
{
	Point			mousePt;
	Rect			textRect;
	RgnHandle	saveClip;
	
	/* Must set clip rgn equal to whole window; otherwise
	 * scrollbar won't update.
	 */
	saveClip = NewRgn();
	GetClip(saveClip);
//	ClipRect(&qd.thePort->portRect);
	Rect bounds = GetQDPortBounds();
	ClipRect(&bounds);
	
	GetMouse(&mousePt);
	textRect = (**tmpTEH).viewRect;
	if (mousePt.v < textRect.top)
		ScrollText(tmpScrollH, kControlUpButtonPart);
	else if (mousePt.v > textRect.bottom)
		ScrollText(tmpScrollH, kControlDownButtonPart);
	
	SetClip(saveClip);
	DisposeRgn(saveClip);
	return TRUE;
}


void ScrollToSelection(PEDITFIELD theField)
{
	TEHandle			teH;
	ControlHandle	scrollH;
	short				topLine, botLine, windHt;
	
	teH = theField->teH;
	scrollH = theField->scrollH;

	HLock((Handle)teH);
	
	topLine = GetControlValue(scrollH);
	windHt = ((**teH).viewRect.bottom - (**teH).viewRect.top) / (**teH).lineHeight;
	botLine = topLine + windHt;
	
	if (GetControlMaximum(scrollH) == 0)
		AdjustText(teH, scrollH);
	else if ((**teH).selEnd < (**teH).lineStarts[topLine])
		ScrollCharacter(theField, (**teH).selStart, FALSE);
	else if ((**teH).selStart >= (**teH).lineStarts[botLine])
		ScrollCharacter(theField, (**teH).selEnd, TRUE);

	HUnlock((Handle)teH);
}


static void ScrollCharacter(PEDITFIELD theField, short charPos, Boolean toBottom)
{
	short				theLine, windHt;
	TEHandle			teH;
	ControlHandle	scrollH;

	teH = theField->teH;
	scrollH = theField->scrollH;
	
	/* find line containing character */
	for (theLine=0; (**teH).lineStarts[theLine+1]<=charPos; theLine++) ;
	
	if (toBottom) {
		windHt = ((**teH).viewRect.bottom - (**teH).viewRect.top) / (**teH).lineHeight;
		theLine -= windHt-1;
	}
	SetControlValue(scrollH, theLine);
	AdjustText(teH, scrollH);
}


/* ¥¥¥What about handling menu enabling? Won't that conflict w/dlog mgr? */
void DoTEEdit(PEDITFIELD theField, short cmd)
{
	long		scrapLen, teLen, numSelChars;
	
	ScrollToSelection(theField);
	
	switch (cmd) {
		case TE_CUT:
			TECut(theField->teH);
			scrapDirty = TRUE;
			break;
		case TE_COPY:
			TECopy(theField->teH);
			scrapDirty = TRUE;
			return;								/* skip stuff at bottom that the others need */
			break;
		case TE_PASTE:
			/* If pasting would exceed text edit limits, paste what will fit. */
			teLen = (**theField->teH).teLength;
			scrapLen = TEGetScrapLength();
			numSelChars = (long) ((**theField->teH).selEnd - (**theField->teH).selStart);
			if ((scrapLen + teLen - numSelChars) > MAX_CHARS_IN_FIELD) {
#if 1
				SysBeep(10);
#else
				charsToPaste = MAX_CHARS_IN_FIELD - teLen;
				scrapH = TEScrapHandle();
				HLock(scrapH);
/* ¥¥¥¥
copy chars 0 through charsToPaste from scrapH
"paste" them into teH. How?
¥¥¥¥Use TEInsert.
*/				
				HUnlock(scrapH);
#endif
			}
			else
				TEPaste(theField->teH);
			break;
		case TE_CLEAR:
			TEDelete(theField->teH);
			break;
	}
	AdjustScrollBar(theField);
	AdjustText(theField->teH, theField->scrollH);
	ScrollToSelection(theField);						/* keep insertion point visible */
}


/* Call this in response to a mouseDown in the window's content area.
 * If click was in the TE viewrect or scrollbar rect, handles click
 * and returns TRUE; FALSE if not in either rect.
 */
Boolean DoEditFieldClick(PEDITFIELD theField, EventRecord *event)
{
	WindowPtr		wind;
	ControlHandle	whichCntlH;
	Boolean			extend;
	Point				localPt;
	short				part;
	Rect				tmpCtlRect;
	
//	wind = (**theField->scrollH).contrlOwner;
	wind = GetControlOwner(theField->scrollH);
	extend = (event->modifiers & shiftKey) != 0;
	localPt = event->where;
	GlobalToLocal(&localPt);

	if (!PtInRect(localPt, &theField->bounds)) return FALSE;		/* not in our field */
	
	tmpTEH = theField->teH;													/* so clikloop routine can get them */
	tmpScrollH = theField->scrollH;
 	GetControlBounds(tmpScrollH, &tmpCtlRect);
 	
	if (PtInRect(localPt, &(**tmpTEH).viewRect)) {
		TEClick(localPt, extend, tmpTEH);
		return TRUE;
	}
//	else if (PtInRect(localPt, &(**tmpScrollH).contrlRect)) {
	else if (PtInRect(localPt, &tmpCtlRect)) {
		part = FindControl(localPt, wind, &whichCntlH);
		if (part!=0 && part!=254 && whichCntlH==tmpScrollH)
			DoScroll(theField, part, localPt);
		return TRUE;															/* even though we may have hit an inactive cntl */
	}
	
	return FALSE;		
}


/* Call this in response to a keyDown or autoKey event, after having checked
 * for menu key equivalents or other special key behavior (e.g., arrow keys).
 */
void DoTEFieldKeyEvent(PEDITFIELD theField, EventRecord *event)
{
	unsigned char	ch;
	long				selStart, selEnd;
	
	ch = (unsigned char) event->message;
#ifdef SMART_QUOTES
	/*
	 * Pass command-quote/-double-quote thru so user can bypass "smart" quotes;
	 * filter out all other cmd-chars.
	 */
	if (event->modifiers & cmdKey)
		if (ch!='"' && ch!='\'') return;
#else
	if (event->modifiers & cmdKey) return;						/* filter out cmd key equivs */
#endif
	
	selStart = (**theField->teH).selStart;
	selEnd = (**theField->teH).selEnd;
	
	/* Handle pageUp -Dn, etc keys on extended keyboard. Change thumb position
	 * and text displayed, but don't change selection (in accordance w/UIF guidelines).
	 */
	tmpTEH = theField->teH;											/* so ScrollText can get it */
	switch (ch) {
		case PGUPKEY:
			ScrollText(theField->scrollH, kControlPageUpPart);
			return;
			break;
		case PGDNKEY:
			ScrollText(theField->scrollH, kControlPageDownPart);
			return;
			break;
		case HOMEKEY:
			SetControlValue(theField->scrollH, 0);
			AdjustText(theField->teH, theField->scrollH);
			return;
			break;
		case ENDKEY:
			SetControlValue(theField->scrollH, GetControlMaximum(theField->scrollH));
			AdjustText(theField->teH, theField->scrollH);
			return;
			break;
		default:
			break;
	}

#ifdef SMART_QUOTES
	/*
	 * Leave command-quote/-double-quote as std ASCII quotes so user can
	 * bypass "smart" quotes; covert all other chars.
	 */
	if (!(event->modifiers & cmdKey))
		ch = ConvertQuote(theField->teH,ch);
#endif

	/* Map forward delete char to delete and fiddle w/ sel range to make if work right. */
	if (ch == FORWARD_DEL_CHAR) {
		if (selStart == selEnd)
			TESetSelect(selStart+1, selEnd+1, theField->teH);
		ch = CH_BS;
	}
	
	/* If field is full, let TEKey get only certain chars. */
	if ((**theField->teH).teLength >= MAX_CHARS_IN_FIELD)
		if ( !(ch==CH_BS || ch==CH_ENTER || ch==LEFTARROWKEY || ch==RIGHTARROWKEY ||
														ch==UPARROWKEY || ch==DOWNARROWKEY) ) {
			SysBeep(10);			/* ¥¥¥need more than this? */
			return;
		}
		
	TEKey((short)ch, theField->teH);
	
	AdjustScrollBar(theField);
	AdjustText(theField->teH, theField->scrollH);
	ScrollToSelection(theField);
}


/* Call this in response to an update event for this window. */
void DoTEFieldUpdate(PEDITFIELD theField)
{
	PenState		savePenState;
	GrafPtr		oldPort;
	WindowPtr	wind;

	//wind = (**theField->scrollH).contrlOwner;
	wind = GetControlOwner(theField->scrollH);
	GetPort(&oldPort); SetPort(GetWindowPort(wind));
	
	GetPenState(&savePenState);

/* EraseRect(&theField->bounds); */
	Draw1Control(theField->scrollH);
	TEUpdate(&(**theField->teH).viewRect, theField->teH);
	
	PenSize(1, 1);
	FrameRect(&theField->bounds);
	SetPenState(&savePenState);
	
	SetPort(oldPort);
}


/* Call this in response to an activate event. */
void DoTEFieldActivateEvent(PEDITFIELD theField, EventRecord *event)
{
	GrafPtr		oldPort;
	WindowPtr	wind, myWind;

	if (event->what != activateEvt) return;
	
	wind = (WindowPtr) event->message;
	//myWind = (**theField->scrollH).contrlOwner;
	myWind = GetControlOwner(theField->scrollH);
	
	if (wind != myWind) return;
	
	GetPort(&oldPort); SetPort(GetWindowPort(wind));

	if (event->modifiers & activeFlag != 0) {
		theField->active = TRUE;
		HiliteControl(theField->scrollH, CTL_ACTIVE);
		TEActivate(theField->teH);
	}
	else {
		theField->active = FALSE;
		TEDeactivate(theField->teH);
		HiliteControl(theField->scrollH, CTL_INACTIVE);
	}
	
	SetPort(oldPort);
}


/* Call this in response to a nullEvent, or even every time through the event
 * loop (or dlog filter). It adjusts the cursor shape and causes the caret to blink.
 * Returns TRUE if mouse in field (we changed cursor); FALSE if not. It's the caller's
 * responsibility to set the cursor shape to something reasonable (e.g., arrow) when
 * this function returns FALSE.
 * NB: uses the QuickDraw global <arrow>.
 */
Boolean DoTEFieldIdle(PEDITFIELD theField, EventRecord *event)
{
	Point			localWhere;
	CursHandle	iBeam;
	Rect			tmpCtlRect;
	
	localWhere = event->where;
	GlobalToLocal(&localWhere);

	if (theField->active)
		TEIdle(theField->teH);

 	GetControlBounds(theField->scrollH, &tmpCtlRect);
 	
	if (PtInRect(localWhere, &(**theField->teH).viewRect)) {
		iBeam = GetCursor(iBeamCursor);
		if (iBeam != NIL)
			SetCursor(*iBeam);
	}
	else if (PtInRect(localWhere, &tmpCtlRect))
		ArrowCursor();
	else
		return FALSE;						/* not in our edit field */
	
	return TRUE;							/* we changed cursor */
}


/* The program should call this or its equivalent when starting up
 * and when responding to a resume event.
 */
void ReadDeskScrap()
{
#ifdef CARBON_NOTYET
	long			scrapLen, along;
	OSErr			result;
	PScrapStuff	Pscrap;

	Pscrap = InfoScrap();
	if (scrapCompare!=Pscrap->scrapCount) {
		scrapLen = GetScrap(NIL, 'TEXT', &along);
		if (scrapLen>=0) {
			result = TEFromScrap();
			if (result!=noErr)
				scrapLen = result;
		}
		if (scrapLen<=0)
			TESetScrapLength(0L);
		Pscrap = InfoScrap();
		scrapCompare = Pscrap->scrapCount;
	}
#endif
}


/* The program should call this or its equivalent when quitting
 * and when responding to a suspend event.
 */
void WriteDeskScrap()
{
#ifdef CARBON_NOTYET
	OSErr	result;
	
	if (scrapDirty) {
		scrapCompare = ClearCurrentScrap();
		result = TEToScrap();
		scrapDirty = FALSE;
	}
#endif
}

