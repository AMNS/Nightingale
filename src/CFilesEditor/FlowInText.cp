/***************************************************************************
*	FILE:	FlowInText.c																		*
*	PROJ:	Nightingale, slightly revised for v.3.6									*
*	DESC:	Routines to handle flow-in text dialog and insertion					*
/***************************************************************************/

/*								NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include <ctype.h>
#include "Nightingale.appl.h"
#include "TEfieldPub.h"

/* In debug versions of Nightingale, we #define in our "Handle Manager", an
intermediate layer of memory management routines that adds error checking: this
could cause meaningless complaints since we don't use it in TEfield.c, the text-
box routines the Flow In Text dialog uses. Avoid this annoyance (though at the
cost of losing some error checking) by #undef'ing Handle Manager routines. */

#undef DisposeHandle
#undef NewHandle

/* Symbolic Dialog Item Numbers */
static enum {
	BUT1_Flow = 1,
	BUT2_Cancel,
	STXT3_Flow,
	EDIT4,
	POP5,
	STXT6_Lyric,
	LASTITEM
} E_FlowinTextItems;
	
#define CANCEL_ITEM 	BUT2_Cancel

#define FLOWIN_CURS 280 	/* Use the Graphic-insertion cursor ??belongs in a header */

#define DEFAULT_VPOS 14		/* Default vertical position (pitch level) for new lyrics */


/* local prototypes */
static Boolean FlowInDialog(Document *, short *);
static DialogPtr OpenThisDialog(Document *);
static void CloseThisDialog(DialogPtr);
static void DoDialogUpdate(DialogPtr);
static Boolean DoDialogItem(Document *, DialogPtr, short);

static pascal Boolean MyFilter(DialogPtr, EventRecord *, short *);
static Boolean CheckUserItems(Point, short *);
static Boolean AnyBadValues(DialogPtr);

static short IsFlowInDelim(char);
static long GetWord(char *, char *, long);
static void FlowInFixCursor(Document *, Point, CursHandle);
static void FlowDrawMsgBox(Document *);
static void ShowLyricStyle(Document *, DialogPtr, short);
static void CopyStyle(Document *, short, TEXTSTYLE *);
static LINK InsertNewGraphic(Document *, LINK, short, short, char *, short, TEXTSTYLE, short);
static void InsertSyllable(Document *, LINK, LINK *, short, short, short, short, short, TEXTSTYLE);
static void FlowInTextObjects(Document *, short, TEXTSTYLE);

static void RegisterHyphen(LINK, short, Boolean);
static void StripHyphen(char *);
static void AddHyphens(Document *, short, TEXTSTYLE);
static void DisposeHyphenList(void);


static short lastTextStyle=4;						/* Default text style number */
static Size currWord=0, prevWord;
static LINK firstFlowL, lastFlowL;

static UserPopUp	popup5;
static EDITFIELD	myField;
static Handle		lyricBlk = NULL;

static ModalFilterUPP	dlogFilterUPP;

typedef struct {
	LINK			startL;	/* if NILINK, was a node that was deleted but never freed */
	LINK			termL;	/* if NILINK, we never got hyphen completor syllable in same voice */
	SignedByte	voice;
	char			filler;	/* ??numHyphens between startL and termL? */
} HYPHENSPAN;

static HYPHENSPAN	**hyphens = NULL;
static short		numHyphens = 0;


/* Display Flow In Text dialog. Return TRUE if OK, FALSE if CANCEL or error. */

Boolean FlowInDialog(Document *doc, short *font)	/* ??should be static */
{
	short itemHit;
	Boolean okay,keepGoing=TRUE;
	DialogPtr dlog; GrafPtr oldPort;

	/* Build dialog window and install its item values */
	
	GetPort(&oldPort);
	dlog = OpenThisDialog(doc);
	if (dlog == NULL) return(FALSE);
	
	ReadDeskScrap();

	/* Entertain filtered user events until dialog is dismissed */

	while (keepGoing) {
		ModalDialog(dlogFilterUPP,&itemHit);
		keepGoing = DoDialogItem(doc,dlog,itemHit);
	}
	
	/*
	 *	Do final processing of item values, such as exporting them to caller.
	 *	DoDialogItem() has already called AnyBadValues().
	 */
	
	if (okay = (itemHit==BUT1_Flow)) {
		Size len; char *p;
		
		if (lyricBlk) DisposeHandle(lyricBlk);
		lyricBlk = (Handle)GetEditFieldText(&myField);
		len = GetHandleSize(lyricBlk);
		
		currWord = (**myField.teH).selStart;
		if (currWord==len) currWord = 0;						/* if selStart at end, move to beginning */
		else if (currWord) {
			p = *lyricBlk + currWord;
			currWord++; p++;										/* wind up for loop */
			while (p>*lyricBlk) {								/* if selStart in middle of word, set currWord to start of word */
				p--; currWord--;
				if (IsFlowInDelim(*p)) break;
			}
		}
		p = *lyricBlk + currWord;
		while (IsFlowInDelim(*p++) && currWord<len)		/* in case multiple whitespace precedes first syllable (not handled by GetWord, etc.) */
			currWord++;
		if (currWord==len) {
			currWord = 0;											/* if currWord now at end, move to beginning */
			p = *lyricBlk + currWord;
			while (IsFlowInDelim(*p++) && currWord<len)	/* do this yet again */
				currWord++;
		}
		
		lastTextStyle = *font = popup5.currentChoice;
	}

	CloseThisDialog(dlog);
	SetPort(oldPort);
	
	WriteDeskScrap();

	return okay;
}


static pascal Boolean MyFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Boolean		ans=FALSE, doHilite=FALSE;
	WindowPtr	w;
	short			ch, part;
	Point			where;
	Rect			bounds;
	
	if (!DoTEFieldIdle(&myField, evt)) ArrowCursor();

	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				/* Update our dialog contents */
				DoDialogUpdate(dlog);
				ans = TRUE; *itemHit = 0;
			}
			else {
				/*
				 *	Call your main event loop DoUpdate(w) routine here if you
				 *	don't want unsightly holes in background windows.
				 */
			}
			break;
		case activateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetDialogWindowPort(dlog));
				DoTEFieldActivateEvent(&myField, evt);
				*itemHit = 0;
			}
			else {
				/*
				 *	Call your main event loop DoActivate(w) routine here if
				 *	you want to deactivate the former frontmost window, in order
				 *	to unhighlight any selection, scroll bars, etc.
				 */
			}
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);

			part = FindWindow(evt->where, &w);
			if (part==inDrag && w==GetDialogWindow(dlog)) {
				bounds = GetQDScreenBitsBounds();
				DragWindow(w, evt->where, &bounds);
				*itemHit = 0; ans = TRUE;
			}
			else if (part==inContent && w==GetDialogWindow(dlog))
				if (DoEditFieldClick(&myField, evt)) {
					*itemHit = 0; ans = TRUE;
				}
			ans = CheckUserItems(where,itemHit);
			break;
		case autoKey:
		case keyDown:
			ch = (unsigned char)evt->message;
			if (ch == CH_ENTER) {
				*itemHit = BUT1_Flow;
				doHilite = ans = TRUE;
			}
			/* Allow using cmd-' and cmd-" to override "smart quotes". */
			else if ((evt->modifiers & cmdKey) && (ch!='"' && ch!='\'')) {
				switch (ch) {
					case '.':
						*itemHit = CANCEL_ITEM;
						doHilite = TRUE;
						break;
					case 'c':
					case 'C':
						DoTEEdit(&myField, TE_COPY);
						break;
					case 'x':
					case 'X':
						DoTEEdit(&myField, TE_CUT);
						break;
					case 'v':
					case 'V':
						DoTEEdit(&myField, TE_PASTE);
						break;
				}
				ans = TRUE;		/* Other cmd-chars ignored */
			}
			/* We expect DoTEFieldKeyEvent to call ConvertQuote.  */
			else DoTEFieldKeyEvent(&myField, evt);
			break;
		}
	if (doHilite)
		FlashButton(dlog, *itemHit);
	return ans;
}


/* Mouse down event: Check if it's in some user item, and convert to itemHit if
appropriate. */

static Boolean CheckUserItems(Point where, short *itemHit)
{
	if (PtInRect(where, &popup5.shadow))
		{ *itemHit = DoUserPopUp(&popup5) ? POP5 : 0; return TRUE; }

	return FALSE;
}


/* Redraw the contents of this dialog in response to an update event. */

static void DoDialogUpdate(DialogPtr dlog)
{
	GrafPtr oldPort;

	GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
	BeginUpdate(GetDialogWindow(dlog));

	/* No need to restore the previous font because the only item not in
		the system font is the TE record, which has its own font. */
	TextFont(systemFont);
	UpdateDialogVisRgn(dlog);
	DrawPopUp(&popup5);

	EndUpdate(GetDialogWindow(dlog));

	DoTEFieldUpdate(&myField);

	SetPort(oldPort);
}


/*
 * Build this dialog's window on desktop, and install initial item values.
 * Return the dlog opened, or NULL if error (no resource, no memory).
 */

static DialogPtr OpenThisDialog(Document *doc)
{
	short type; Handle hndl; Rect box; GrafPtr oldPort;
	DialogPtr dlog;

	dlogFilterUPP = NewModalFilterUPP(MyFilter);
	if (dlogFilterUPP == NULL) {
		MissingDialog(FLOWIN_DLOG);
		return NULL;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(FLOWIN_DLOG,NULL,BRING_TO_FRONT);
	if (dlog == NULL) {
		DisposeModalFilterUPP(dlogFilterUPP);
		MissingDialog(FLOWIN_DLOG);
		return NULL;
	}
	
	SetDialogDefaultItem(dlog,BUT1_Flow);

	CenterWindow(GetDialogWindow(dlog),0);
	SetPort(GetDialogWindowPort(dlog));

	/* Fill in dialog's values here */
	WaitCursor();
	GetDialogItem(dlog, EDIT4, &type, &hndl, &box);
	if (!CreateEditField(GetDialogWindow(dlog), box, textFontNum, textFontSize, 0, NULL, FALSE, &myField)) {
		DisposeDialog(dlog);
		return NULL;
	}
	if (lyricBlk)
		SetEditFieldText(&myField, lyricBlk, NULL, FALSE);

	TESetSelect(currWord, currWord, myField.teH);
	ScrollToSelection(&myField);

	if (!InitPopUp(dlog,&popup5,POP5,0, FLOWTEXTSTYLE_POPUP, lastTextStyle))
		goto broken;
	ShowLyricStyle(doc, dlog, popup5.currentChoice);
		
	ShowWindow(GetDialogWindow(dlog));
	return dlog;

broken:
	CloseThisDialog(dlog);
	SetPort(oldPort);
	return NULL;
}


/* Clean up any allocated stuff, and return dialog to primordial mists. */

static void CloseThisDialog(DialogPtr dlog)
{
	DisposeEditField(&myField);
	DisposePopUp(&popup5);
	DisposeModalFilterUPP(dlogFilterUPP);
	DisposeDialog(dlog);
}


/* Deal with user clicking on an item in this dialog, either modal or non-modal.
Returns whether or not to the dialog should be closed (keepGoing). */

static Boolean DoDialogItem(Document *doc, DialogPtr dlog, short itemHit)
{
	short type;
	Boolean okay=FALSE,keepGoing=TRUE;
	Handle hndl; Rect box;

	if (itemHit<1 || itemHit>=LASTITEM) return keepGoing;

	GetDialogItem(dlog,itemHit,&type,&hndl,&box);
	switch (itemHit) {
		case BUT1_Flow:
			keepGoing = FALSE;
			break;
		case BUT2_Cancel:
			keepGoing = FALSE;
			break;
		case POP5:
			ShowLyricStyle(doc, dlog, popup5.currentChoice);
			break;
	}

	if (okay) keepGoing = AnyBadValues(dlog);		/* ??okay IS NEVER TRUE! */
	return keepGoing;
}


/* Pull values out of dialog items and deliver TRUE if any of them are
illegal or inconsistent; otherwise deliver FALSE.  Doesn't need to do
anything for this dialog. */

static Boolean AnyBadValues(DialogPtr /*dlog*/)
{
	return FALSE;
}


/* Return TRUE if c is a delimiter for the units to be Flowed In: words (any white
space is a delimiter) or explicitly-marked syllables ('-' is the delimiter). */

static short IsFlowInDelim(char c)
{
	return (isspace(c) || c == '-');
}


static long GetWord(
					char *buf,				/* buffer to find word in */
					char *theWord,			/* receives first word of <buf> */
					long n					/* length of <buf> */
					)
{
	char *p,*w; Size len=0;

	p = buf; w = theWord;

	while (!IsFlowInDelim(*p) && len++<n)
		*w++ = *p++;

	if (*p=='-') *w++ = *p++;

	*w = '\0';

	return len;					/* length of theWord (excluding NULL terminator) */
	/* ??NB: len will not include any final '-',
	 * and len will be wrong for last word unless it's terminated by whitespace.
	 * Neither of these now create any problems for dependent code. Later, who knows.
	 */
}


static void FlowInFixCursor(Document *doc, Point pt, CursHandle flowInCursor)
{
	GlobalToLocal(&pt);
	if (PtInRect(pt,&doc->viewRect) && flowInCursor)
		SetCursor(*flowInCursor);
	else
		ArrowCursor();
}
	

#define MSG_BOTTOM_MARGIN 4

static void FlowDrawMsgBox(Document *doc)
{
	WindowPtr wPtr=doc->theWindow;
	Rect msgR,portRect; long wlen; char *p, w[256];
	Size currW, len;
	short	oldFont, oldSize;
	
	oldSize = GetWindowTxSize(wPtr);
	oldFont = GetWindowTxFont(wPtr);

	GetWindowPortBounds(wPtr,&portRect);

	SetRect(&msgR, portRect.left, 
					portRect.bottom-14,
					portRect.left+MESSAGEBOX_WIDTH-1,
					portRect.bottom);

	ClipRect(&msgR);
	EraseRect(&msgR);
	MoveTo(portRect.left+6, portRect.bottom-MSG_BOTTOM_MARGIN);
	TextFace(bold); TextFont(textFontNum); TextSize(textFontSmallSize);

	len = GetHandleSize(lyricBlk);
	HLock(lyricBlk);
	p = *lyricBlk;
	currW = currWord;
	wlen = GetWord(p+currWord,w,len-currW);	
	currW += wlen;													/* In case we want to draw more words */
	
	if (currWord>=len) {
		GetIndCString(strBuf, FLOWIN_STRS, 1);				/* "[end of text]" */
		DrawCString(strBuf);
	}
	else {
		p = w;
		while (*p) DrawChar(*p++);
	}
	
	HUnlock(lyricBlk);

	MoveTo(portRect.left+175, portRect.bottom-MSG_BOTTOM_MARGIN);
	TextFace(0);
	GetIndCString(strBuf, FLOWIN_STRS, 2);					/* "CMD-. TO EXIT" */
	DrawCString(strBuf);

	ClipRect(&doc->viewRect);
	
	TextFace(0); TextFont(oldFont); TextSize(oldSize);
}


static void ShowLyricStyle(Document *doc, DialogPtr dlog, short theFont)
{
	Str63 lyricStyleStr;
	
	GetIndString(lyricStyleStr, FLOWIN_STRS, 3);							/* "Lyric Style" */
	switch (theFont) {
		case TSRegular1STYLE:
			PutDlgString(dlog,STXT6_Lyric,(unsigned char *)(doc->lyric1 ? lyricStyleStr : "\p"), FALSE);
			return;
		case TSRegular2STYLE:
			PutDlgString(dlog,STXT6_Lyric,(unsigned char *)(doc->lyric2 ? lyricStyleStr : "\p"), FALSE);
			return;
		case TSRegular3STYLE:
			PutDlgString(dlog,STXT6_Lyric,(unsigned char *)(doc->lyric3 ? lyricStyleStr : "\p"), FALSE);
			return;
		case TSRegular4STYLE:
			PutDlgString(dlog,STXT6_Lyric,(unsigned char *)(doc->lyric4 ? lyricStyleStr : "\p"), FALSE);
			return;			
		case TSRegular5STYLE:
			PutDlgString(dlog,STXT6_Lyric,(unsigned char *)(doc->lyric5 ? lyricStyleStr : "\p"), FALSE);
			return;			
	}
}


static void CopyStyle(Document *doc, short theFont, TEXTSTYLE *pStyle)
{
	switch (theFont) {
		case TSRegular1STYLE:
			BlockMove(doc->fontName1,pStyle,sizeof(TEXTSTYLE));
			return;
		case TSRegular2STYLE:
			BlockMove(doc->fontName2,pStyle,sizeof(TEXTSTYLE));
			return;
		case TSRegular3STYLE:
			BlockMove(doc->fontName3,pStyle,sizeof(TEXTSTYLE));
			return;
		case TSRegular4STYLE:
			BlockMove(doc->fontName4,pStyle,sizeof(TEXTSTYLE));
			return;
		case TSRegular5STYLE:
			BlockMove(doc->fontName5,pStyle,sizeof(TEXTSTYLE));
			return;
	}
}


static LINK InsertNewGraphic(Document *doc, LINK pL,
						short stf, short v,
						char *str,				/* C string, allocated by caller */
						short font,
						TEXTSTYLE theStyle,
						short pitchLev
						)
{
	DDIST					xd, yd;
	LINK					newpL, aGraphicL;
	short					graphicType, fontInd;
	CONTEXT				context;
	PGRAPHIC				pGraphic;
	PAGRAPHIC			aGraphic;
	STRINGOFFSET		offset;

PushLock(OBJheap);
	GetContext(doc, LeftLINK(pL), stf, &context);

	newpL = InsertNode(doc, pL, GRAPHICtype, 1);
	if (newpL) {
		xd = 0;
		yd = halfLn2d(pitchLev, context.staffHeight, context.staffLines);
		NewObjSetup(doc, newpL, stf, xd);
		SetObject(newpL, xd, yd, TRUE, TRUE, FALSE);
	
		graphicType = (theStyle.lyric? GRLyric : GRString);
		fontInd = GetFontIndex(doc, theStyle.fontName);
		if (fontInd<0) {
			GetIndCString(strBuf, MISCERRS_STRS, 20);    /* "Will use most recently added font." */
			CParamText(strBuf, "", "", "");
			CautionInform(MANYFONTS_ALRT);
			fontInd = MAX_SCOREFONTS-1;
		}
		InitGraphic(newpL, graphicType, stf, v, fontInd, theStyle.relFSize, theStyle.fontSize,
						theStyle.fontStyle, theStyle.enclosure);
	
		pGraphic = GetPGRAPHIC(newpL);
		aGraphicL = FirstSubLINK(newpL);

		offset = PStore(CToPString(str));
		if (offset<0L) {
			NoMoreMemory();
			goto cleanup;
		}
		GraphicSTRING(aGraphicL) = offset;
		
		aGraphic = GetPAGRAPHIC(aGraphicL);
		aGraphic->next = NILINK;
		
		pGraphic->firstObj = pL;
		pGraphic->lastObj = NILINK;

		pGraphic->selected = FALSE;
		pGraphic->justify = GRJustLeft;
		pGraphic->info = User2HeaderFontNum(doc, font);
	}
	else
		NoMoreMemory();

cleanup:
	return newpL;
PopLock(OBJheap);
}


#define EXIT_NOW -1

short HandleOptionKeyMsgBox(Document *, Size);
short HandleOptionKeyMsgBox(Document *doc, Size /*lyricLen*/)
{
	static Boolean	optDownNow, optDownBefore=FALSE;
	Size tmpCurrWord;
	
	tmpCurrWord = currWord;
	optDownNow = OptionKeyDown();
		
	if (optDownNow != optDownBefore) {
		if (optDownNow) currWord = prevWord;
		FlowDrawMsgBox(doc);
	}
	
	currWord = tmpCurrWord;
	optDownBefore = optDownNow;
	return 1;
}


void InsertSyllable(Document *doc, LINK pL, LINK *lastGrL, short stf, short v,
							short lyricLen, short pitchLev, short theFont, TEXTSTYLE theStyle)
{
	LINK	newL;
	char	w[256], *p;
	long	wlen;
	
	if (firstFlowL) {
		if (IsAfter(pL, firstFlowL))
			firstFlowL = pL;
	}
	else firstFlowL = pL;
	if (lastFlowL) {
		if (IsAfter(lastFlowL, pL))
			lastFlowL = pL;
	}
	else lastFlowL = pL;
	
	HiliteInsertNode(doc, pL, stf, FALSE);

	/* If the user holds down the option key, don't increment the pointer
		to the next word; this allows the user to insert the same word over
		and over again; for example, into each of a number of voices. Rather,
		return to the word just inserted, insert it again, and then increment
		the point back to the place it was when we started the whole thing. */

	HLock(lyricBlk);
	if (OptionKeyDown()) {
		currWord = prevWord;

		p = *lyricBlk;
		wlen = GetWord(p+currWord, w, lyricLen-currWord);	
		currWord += wlen;
		p = *lyricBlk + currWord;

		while (IsFlowInDelim(*p++)) currWord++;
	}
	else {
		prevWord = currWord;
		p = *lyricBlk;
		wlen = GetWord(p+currWord, w, lyricLen-currWord);
	
		currWord += wlen;
		p = *lyricBlk + currWord;
	
		while (IsFlowInDelim(*p++)) currWord++;			/* advance to next word */
	}
	HUnlock(lyricBlk);
	if (theStyle.lyric) StripHyphen(w);

	newL = InsertNewGraphic(doc, pL, stf, v, w, theFont, theStyle, pitchLev);
	*lastGrL = newL;
	if (newL) {
		if (theStyle.lyric && !ShiftKeyDown())
			CenterTextGraphic(doc, newL);
	
		GetAllContexts(doc, contextA, pL);
		DrawGRAPHIC(doc, newL, contextA, TRUE);
		if (!OptionKeyDown())
			FlowDrawMsgBox(doc);
		
		RegisterHyphen(newL, prevWord, TRUE);
	}
}


static void FlowInTextObjects(Document	*doc, short theFont, TEXTSTYLE theStyle)
{
	short						ans, stf, v=0, sym, pitchLev;
	short						ch, val, oldVal, status, activ;
	short						change, oldPitchLev, lyricLen;
	WindowPtr				w = doc->theWindow;
	LINK						pL, prevGrL;
	LINK						lastGrL, aNoteL;
	EventRecord				evt;
	Point						pt;
	Rect						paper,portRect;
	Boolean					gotEvent=FALSE;
	ControlHandle			control;
	CursHandle				flowInCursor;
	ControlActionUPP		actionUPP;
	short						contrlHilite;
	
	flowInCursor = GetCursor(FLOWIN_CURS);
	if (flowInCursor) SetCursor(*flowInCursor);

	prevWord = currWord;
	prevGrL = lastGrL = NILINK;

	lyricLen = (short) GetHandleSize(lyricBlk);
	oldPitchLev = DEFAULT_VPOS;

	FlowDrawMsgBox(doc);

	while (1) {
		gotEvent = GetNextEvent(everyEvent, &evt);
		FlowInFixCursor(doc, evt.where, flowInCursor);
		HandleOptionKeyMsgBox(doc, (Size)lyricLen);
		if (!gotEvent) continue;
		switch (evt.what) {
			case mouseDown:
				pt = evt.where;
				GlobalToLocal(&pt);

				switch (FindControl(pt,w,&control)) {
					case kControlUpButtonPart:
					case kControlPageUpPart:
					case kControlDownButtonPart:
					case kControlPageDownPart:
						contrlHilite = GetControlHilite(control);
						if (contrlHilite != 255) {
							MEHideCaret(doc);
							GetWindowPortBounds(w,&portRect);
							ClipRect(&portRect);
							actionUPP = NewControlActionUPP(ScrollDocument);
							if (actionUPP) {
								TrackControl(control, pt, actionUPP);
								DisposeControlActionUPP(actionUPP);
							}
							GetWindowPortBounds(w,&portRect);
							ClipRect(&portRect);
							DrawControls(w);
							ClipRect(&doc->viewRect);
							}
						break;
					case kControlIndicatorPart:
						contrlHilite = GetControlHilite(control);
						if (contrlHilite != 255) {
							oldVal = GetControlValue(control);
							GetWindowPortBounds(w,&portRect);
							ClipRect(&portRect);
							TrackControl(control,pt,NULL);
							val = GetControlValue(control);
							ClipRect(&doc->viewRect);
							if (change = val-oldVal) {
								if (change & 7)
									/* Keep change a multiple of 8 */
									if (change > 0) change += (8-(change&7));
									 else			change -= (change & 7);
								/*
								 *	Reset it, cause QuickScroll expects old value,
								 *	but don't let user see you set it back.
								 */
								SetControlValue(control,oldVal);
								MEHideCaret(doc);
								/* OK, now go ahead */
								if (control == doc->vScroll) QuickScroll(doc,0,change,TRUE,TRUE);
								 else						 QuickScroll(doc,change,0,TRUE,TRUE);
								}
							}
						break;
					default:
						if (PtInRect(pt, &doc->viewRect)) {
							if (FindSheet(doc,pt, &ans)) {
								GetSheetRect(doc, ans, &paper);
								doc->currentSheet = ans;
								doc->currentPaper = paper;
							
								/* Transform to paper relative coords */
								pt.h -= paper.left;
								pt.v -= paper.top;
					
								pL = FindGraphicObject(doc, pt, &stf, &v);
								/* To allow attaching to objects other than notes, remove
									the following test for SyncTYPE. */
								if (pL && SyncTYPE(pL) && (currWord<lyricLen || OptionKeyDown())) {
									HiliteInsertNode(doc, pL, stf, TRUE);
									status = InsTrackUpDown(doc, pt, &sym, pL, stf, &pitchLev);
									
									if (status) {
										if (status<0) pitchLev = oldPitchLev;
										oldPitchLev = pitchLev;
										prevGrL = lastGrL;
										InsertSyllable(doc, pL, &lastGrL, stf, v,
																lyricLen, pitchLev, theFont, theStyle);
									}
								}
							}
						}
						else goto done;			/* clicked outside of score area, so exit */
						break;
				}

				break;
			case keyDown:
			case autoKey:
				ch = (unsigned int)(evt.message & charCodeMask);
				switch (ch) {
					case CH_BS:							/* Handle backspace to delete */
						if (lastGrL && lastGrL!=prevGrL) {
							/* Erase and inval the objrect of this Graphic. */
							Rect r = LinkOBJRECT(lastGrL);
							r.left -= pt2p(2);
							r.right += pt2p(4);		/* bbox often not wide enough on right */
							OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
							EraseAndInval(&r);
/* ??NB: DeleteNode doesn't get rid of string in StrMgr lib! */
							DeleteNode(doc, lastGrL);
							currWord = prevWord;
							RegisterHyphen(lastGrL, currWord, FALSE);
							lastGrL = prevGrL;
							FlowDrawMsgBox(doc);
						}
						break;
					case '.':
 						if ((evt.modifiers&cmdKey)!=0) goto done;
 						break;
					case '\t':
						/* We won't enter this block until user flows once via InsTrackUpDown */
						if (v && lastGrL && (currWord<lyricLen || OptionKeyDown())) {
							pL = LVSearch(RightLINK(GraphicFIRSTOBJ(lastGrL)), SYNCtype, v, GO_RIGHT, FALSE);
							if (pL) {
								aNoteL = NoteInVoice(pL, v, FALSE);
								stf = NoteSTAFF(aNoteL);
								HiliteInsertNode(doc, pL, stf, TRUE);
								prevGrL = lastGrL;
								InsertSyllable(doc, pL, &lastGrL, stf, v,
														lyricLen, pitchLev, theFont, theStyle);

								/* if new Graphic is offscreen, scroll it into view */
								pt = LinkToPt(doc, pL, TRUE);
								if (!PtInRect(pt, &doc->viewRect)) {
									LINK tmpSelStartL = doc->selStartL;
									doc->selStartL = pL;
									GoToSel(doc);
									doc->selStartL = tmpSelStartL;
								}
							}
						}
 						break;
					default:
						;
				}
				break;
			case keyUp:
				break;
			case activateEvt:
				activ = (evt.modifiers & activeFlag) != 0;
				DoActivate(&evt,activ,FALSE);
				if (flowInCursor) SetCursor(*flowInCursor);
				break;
			case updateEvt:
				DoUpdate((WindowPtr)(evt.message));
				FlowDrawMsgBox(doc);				/* DoUpdate may have overwritten msg box */
				break;
			case diskEvt:
				break;
			case app4Evt:
				DoSuspendResume(&evt);
				break;
			default:
				;
		}
	}
	
done:
	EraseAndInvalMessageBox(doc);
}


void DoTextFlowIn(Document *doc)
{
	short theFont;			/* NB: this is itemNumber of text style popup in FlowInDialog! 
										for use in 1 call to User2HeaderFontNum. */
	TEXTSTYLE style;

	if (FlowInDialog(doc, &theFont)) {
		CopyStyle(doc, theFont, &style);
		DisableUndo(doc, FALSE);
		firstFlowL = lastFlowL = NILINK;
		
		FlowInTextObjects(doc, theFont, style);
		
		/*
		 * If auto-respacing is on, the text we've just flowed in is in a lyric
		 * style, and we really did flow some in, respace the entire area from
		 * the first measure anything was added to thru the last. If any measures
		 * within the range didn't have anything added to them, maybe they should
		 * be left alone, but that would be pretty unusual, and this is easier.
		 */
		if (doc->autoRespace && style.lyric) {
			if (firstFlowL && lastFlowL) {
				LINK firstMeasL, prevMeasL;
				
				RespaceBars(doc, firstFlowL, lastFlowL, 0L, FALSE, FALSE);
				
				/* First Graphic might overlap previous measure. Inval that measure,
				 * if it's on the same system, just in case.
				 */
				firstMeasL = LSSearch(firstFlowL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
				prevMeasL = LinkLMEAS(firstMeasL);
				if (SameSystem(prevMeasL, firstMeasL))
					InvalMeasure(prevMeasL, ANYONE);
			}
		}
		if (style.lyric)
			AddHyphens(doc, theFont, style);
		DisposeHyphenList();
	}
}


/* -------------------------- Functions for creating hyphens ------------------------ */

/* Every time the user adds or deletes a syllable, the code above calls RegisterHyphen.
RegisterHyphen maintains an array of HYPHENSPAN structs, each of which contains links
to the Graphics that initiate and terminate a hyphen, as well as their voice number. */

static void RegisterHyphen(
					LINK		grL,				/* Graphic just inserted */
					short		charOffset,		/* offset of Graphic's text in lyricBlk */
					Boolean	add 				/* if TRUE, register; if FALSE, un-register */
				)
{
	short			i, numNodes;
	HYPHENSPAN	*h;
	Boolean		hasHyphen;
	SignedByte	voice;
	char			*p;
	char			syllable[256];		/* C string */
	Size			len;
	
	if (grL==NILINK) return;
	
	if (hyphens==NULL) {
		hyphens = (HYPHENSPAN **) NewHandle((Size)(sizeof(HYPHENSPAN) * 4));
		if (hyphens==NULL) {
			MayErrMsg("RegisterHyphen: can't allocate hyphen list");
			return;
		}
	}
	
	/* get syllable at charOffset from lyricBlk */ 
	len = GetHandleSize(lyricBlk);
	HLock(lyricBlk);
	p = *lyricBlk;
	GetWord(p+charOffset, syllable, len-charOffset);	
	HUnlock(lyricBlk);
	
	/* find out if it ends with hyphen */
	if (syllable[strlen(syllable)-1]=='-')		/* test (only) last letter of syllable */
		hasHyphen = TRUE;
	else
		hasHyphen = FALSE;
	
	voice = GraphicVOICE(grL);
	
	if (add) {
		if (numHyphens>0) {
			/* search for last incomplete HYPHENSPAN struct in this voice */
			HLock((Handle)hyphens);
			for (i=numHyphens-1, h=*hyphens+i; i>=0; i--, h--) {
				if (h->voice==voice) {
					if (h->termL==NILINK) {
						if (IsAfter(h->startL, grL)) {
							h->termL=grL;						/* our grL is a completor */
							break;
						}
						else {
							GetIndCString(strBuf, FLOWIN_STRS, 4);		/* "Lyrics must go from left to right within the voice" */
							CParamText(strBuf, "", "", "");
							StopInform(GENERIC_ALRT);
							/* ??offer to delete? */
							return; /*???*/
						}
					}
				}
			}
			HUnlock((Handle)hyphens);
		}

		/* If this syllable has a hyphen, start a new record. Set its termL to NILINK so
		 * we'll know it's incomplete later. NB: A syllable can both initiate and terminate
		 * hyphens.
		 */
		if (hasHyphen) {
			/* expand array if necessary */
			numNodes = GetHandleSize((Handle)hyphens) / sizeof(HYPHENSPAN);
			if (numHyphens==numNodes) {					/* array full; allocate more nodes */
				SetHandleSize((Handle)hyphens, (Size)(sizeof(HYPHENSPAN) * (numNodes + 4)));
				if (MemError()) {
					MayErrMsg("RegisterHyphen: can't expand hyphen list");
					return;
				}
			}
			
			/* start new record in 0-based array */
			h = *hyphens + numHyphens;
			h->startL = grL;
			h->termL = NILINK;				/* so we'll recognize it when completor follows */
			h->voice = voice;
			h->filler = 0;
			numHyphens++;
		}
	}
	
	/* This syllable has just been deleted. Clean up any hyphen record it was part of. */
	else {
		if (numHyphens>0) {
			/* search for last HYPHENSPAN struct in this voice */
			HLock((Handle)hyphens);
			for (i=numHyphens-1, h=*hyphens+i; i>=0; i--, h--) {
				if (h->voice==voice) {
					if (grL==h->termL) {
						h->termL = NILINK;						/* mark it for use later */
						break;
					}
					else if (grL==h->startL) {
						h->startL = h->termL = NILINK;
						h->voice = 0;
						if (i==numHyphens-1) numHyphens--;	/* if node is last, we can free it */
						/* don't break; grL may also be a termL in prev record in this voice */
					}
				}
			}
			HUnlock((Handle)hyphens);
		}
	}
}


/* Crudely strips one hyphen, if hyphen is last char of word */

static void StripHyphen(char *str)
{
	long	len;
	
	len = strlen(str);
	if (str[len-1]=='-')
		str[len-1] = 0;
}


/* ??NB: following comment doesn't know about multiple hyphens! (much
less about "cross-system" hyphens)
Determine desired xd for one hyphen roughly equidistant from the closest
edges of its h->startL and h->termL Graphics. Then find a sync whose xd
is closest to the desired xd, and attach the hyphen Graphic to that sync.
However, hyphens must appear in the same measure as their firstObj.
Otherwise, if we have this:
					wholenote          | note
					1st syllable    '-'  2nd syllable
(with hyphen attached to second note, to which it is closer), two
bad things can happen: 1) moving 2nd measure down might put hypen
in or to left of reserved area, or 2) respace might push 2nd note
to right in order to place hyphen after barline! (Try it.) */		 

/* Create a run of hyphens within one system. The syllables surrounding the hyphen
may not even be in this system. Returns number of errors encountered, or -1 in
case of a fatal error. */

short CreateHyphenRun(Document *, LINK, LINK, LINK, DDIST, DDIST, Boolean, short, DDIST,
								short, short, TEXTSTYLE, DDIST);

short CreateHyphenRun(
			Document		*doc,
			LINK			firstSylL,	/* first syllable's graphic, or NILINK if it's not on this system */
			LINK			startL,		/* If a sync, it's the first one we can attach hyphen to; */
											/* if a measure, hyphen's first syllable is on a previous system. */
			LINK			/*endL*/,	/* If a sync, it's the last one we can attach hyphen to; */
											/* if a measure, hyphen's last syllable is on a following system. */
											/* NB: if endL is a measure, it may come *before* startL! */
			DDIST			startXD,		/* left edge of space to fill with hyphen run */
			DDIST			endXD,		/* right edge [ditto] */
			Boolean		openSys,		/* TRUE if last meas in sys is not fake (i.e., syncs follow last bar line in sys) */
			short			v,				/* Attach hyphen graphics to syncs in this voice */
			DDIST			hyphenWid,	/* width of hyphen char in context of first syllable */
			short			pitchLev,	/* halfLine pos of first syllable */
			short			theFont,		/* for User2HeaderFontNum call in InsertNewGraphic */
			TEXTSTYLE	theStyle, 	/* style properties of first syllable */
			DDIST			dMaxHyphenDist	/* Maximum horiz. space one hyphen can span */
			)
{
	short				i, stf, hyphenCnt, errCnt=0;
	LINK				pL, lastL, prevL, measL, newL, aGraphicL, aNoteL;
	DDIST				emptySpace, subSpace, desiredXD, spcLeft, thisXD;
	STRINGOFFSET	offset;
	char				hyphStr[256];

	emptySpace = endXD - startXD;
	
	/* The space between hyphen-separated syllables divided by dMaxHyphenDist gives
	 * the number of hyphens we place between the syllables. We center each hyphen
	 * within its slice of this space.
	 */

	if (emptySpace>hyphenWid) {
		hyphenCnt = emptySpace / dMaxHyphenDist;
		if (emptySpace % dMaxHyphenDist) hyphenCnt++;
		subSpace = emptySpace / hyphenCnt;		/* equal-sized chunks of emptySpace */
	
		/* NB: attach hyphens only to Syncs; i.e., pL must be a Sync. */
		prevL = startL;
		for (i=0, spcLeft=startXD; i<hyphenCnt; i++, spcLeft+=subSpace) {	/* for each hyphen...*/
			desiredXD = spcLeft + ((subSpace-hyphenWid)>>1);

			for (lastL=prevL; ; lastL=pL) {								/* ...find best sync to attach it to */
				pL = LVSearch(RightLINK(lastL), SYNCtype, v, GO_RIGHT, FALSE);
				if (pL==NILINK) break;		/* "Can't happen", because there has to be a sync */
													/* for last syllable, even if it's not on this system. */
				if (!SameSystem(lastL, pL)) {
					if (SyncTYPE(lastL)) pL = lastL;
					else pL = NILINK;			/* it's not clear that this could ever happen */
					break;
				}
				thisXD = SysRelxd(pL);
				measL = LSISearch(pL, MEASUREtype, ANYONE, TRUE, FALSE);		/* get next meas on same sys */
				if (measL) {
					/* if hyphen appears before measL, attach to last sync */
					if (LinkXD(measL)>desiredXD) {
						pL = SyncTYPE(lastL)? lastL : pL;
						break;
					}
				}
				if (thisXD>desiredXD) {
					/* if lastL's xd is closer to desiredXD, use lastL if it's a sync */
					if (ABS(SysRelxd(lastL)-desiredXD) <= ABS(SysRelxd(pL)-desiredXD))
						pL = SyncTYPE(lastL)? lastL : pL;
					break;
				}
			}
			if (pL==NILINK) { continue; errCnt++; }
			
			/* Voice may be weaving between staves. Hyphens will have
			 * v-pos's relative to their parent note, which is undesirable
			 * because the attachment of hyphens to some of those notes
			 * is rather arbitrary. But the alternative is risking giving
			 * hyphen Graphic a staff number that none of its parent sync's
			 * notes are on. This would give a debug complaint (at the very
			 * least). Fortunately, it's unlikely the user will often want
			 * lyrics on a voice that crosses staves.
			 * ??To make this better, caller could pass the staff of first
			 * syllable's note, and we could fiddle with pitchLev to make
			 * all hyphens appear at same height, regardless of which staff
			 * their parent notes are on.
			 */
			aNoteL = NoteInVoice(pL, v, FALSE);
			stf = NoteSTAFF(aNoteL);

			/* create one hyphen Graphic */
			hyphStr[0] = '-'; hyphStr[1] = 0;				/* InsertNewGraphic changes this to Pascal str */
			newL = InsertNewGraphic(doc, pL, stf, v, hyphStr,
														theFont, theStyle, pitchLev);
			if (newL==NILINK) { continue; errCnt++; }
			
			/* center it within its slice of horizontal space */
			LinkXD(newL) = -(SysRelxd(pL) - desiredXD);
			
			/* This prevents hyphens from continuing to end of system when the
			 * last sync in system is substantially to the left of the end of 
			 * the system. (e.g., new, note, addSys, note, flow "hy-phen")
			 * If we don't do this, and user later justifies system, hyphens
			 * will end up off the right side of the page. 
			 */
			if (openSys) {
				LINK syncL = LVSearch(RightLINK(pL), SYNCtype, v, GO_RIGHT, FALSE);
				if (!SameSystem(syncL, pL) && desiredXD>(SysRelxd(pL)-(subSpace>>2))) break;
			}
			
			prevL = pL;
		}
	}
	else {
		/* No room for hyphen. For now (or maybe forever), append hyphen to
		 * first syllable's Graphic if it's on this system. NB: Space btw.
		 * lyrics (for respace) should be set so that a hypen will always fit.
		 */
		if (firstSylL) {
			aGraphicL = FirstSubLINK(firstSylL);
			Pstrcpy((unsigned char *)hyphStr, PCopy(GraphicSTRING(aGraphicL)));
			PStrCat((StringPtr)hyphStr, (StringPtr)"\p-");
			offset = PStore((unsigned char *)hyphStr);
			if (offset<0L) {
				NoMoreMemory();		/* ??but we've destroyed a syllable! */
				return -1;
			}
			else
				GraphicSTRING(aGraphicL) = offset;
		}
	}

	return errCnt;
}


static void AddHyphens(Document *doc, short theFont, TEXTSTYLE theStyle)
{
	short			i, j;
	HYPHENSPAN	*h;
	LINK			firstSyncL, lastSyncL;
	CONTEXT		context;
	PGRAPHIC		pGraphic;
	short			staff, voice, pitchLev, fontSize, fontNum, ans, errCnt=0;
	DDIST			startRtEdge, endLeftEdge, startGrWid, hyphenWid;
	DDIST			dMaxHyphenDist;	/* Maximum horiz. space one hyphen can span */
	
	WaitCursor();
	
	HLock((Handle)hyphens);
	for (i=0, h=*hyphens; i<numHyphens; i++, h++) {
		if (h->startL==NILINK || h->termL==NILINK) continue;		/* empty or incomplete node */
		firstSyncL = GraphicFIRSTOBJ(h->startL);
		lastSyncL = GraphicFIRSTOBJ(h->termL);
		if (firstSyncL==NILINK || lastSyncL==NILINK) { continue; errCnt++; }
				
		staff = GraphicSTAFF(h->startL);
		voice = GraphicVOICE(h->startL);
		GetContext(doc, h->startL, staff, &context);
		pitchLev = d2halfLn(LinkYD(h->startL), context.staffHeight, context.staffLines);
		if (config.maxHyphenDist==0) dMaxHyphenDist = pt2d(36);
		else                         dMaxHyphenDist = halfLn2d(config.maxHyphenDist, context.staffHeight, context.staffLines);
		
		/* get hyphen width (Must follow GetContext!) */
		pGraphic = GetPGRAPHIC(h->startL);
		fontNum = doc->fontTable[pGraphic->fontInd].fontID;
		fontSize = GetTextSize(theStyle.relFSize, theStyle.fontSize, LNSPACE(&context));
		hyphenWid = pt2d(NPtStringWidth(doc, "\p-", fontNum, fontSize, theStyle.fontStyle));

		startGrWid = pt2d(NPtGraphicWidth(doc, h->startL, &context));
		startRtEdge = SysRelxd(firstSyncL) + LinkXD(h->startL) + startGrWid;
		endLeftEdge = SysRelxd(lastSyncL) + LinkXD(h->termL);
	
		if (SameSystem(firstSyncL, lastSyncL))
			ans = CreateHyphenRun(doc, h->startL, firstSyncL, lastSyncL, startRtEdge,
									endLeftEdge, FALSE, voice, hyphenWid, pitchLev, theFont,
									/* NB: this  ^^^^^ doesn't matter in this case */
									theStyle, dMaxHyphenDist);

		else {			/* hyphen run spans multiple systems */
			LINK	startL, endL, startSysL, endSysL, midSysL, firstSylL=NILINK;
			DDIST	startXD, endXD;
			short	sysCnt; Boolean lastMeasFake;
			
			GetSysRange(doc, firstSyncL, RightLINK(lastSyncL), &startSysL, &endSysL);
			sysCnt = SystemNUM(endSysL) - SystemNUM(startSysL) + 1;
			for (j=1; j<=sysCnt; j++) {
				if (j==1) {							/* system containing first syllable */
					firstSylL = h->startL;
					startL = firstSyncL;
					startXD = startRtEdge;
					endL = GetLastMeasInSys(startSysL);
					lastMeasFake = FakeMeasure(doc, endL);
					if (FirstMeasInSys(endL))
						lastMeasFake = FALSE;				/* if only 1 barline on sys, disregard fakeness */
					if (!lastMeasFake)
						endXD = SysRectRIGHT(startSysL) - SysRectLEFT(startSysL);	/* sys-rel end of system */
					else
						endXD = SysRelxd(endL);
				}
				else if (j==sysCnt) {			/* system containing last syllable */
					startL = FirstSysMeas(lastSyncL);
					startXD = SysRelxd(startL);
					endL = lastSyncL;
					endXD = endLeftEdge;
					lastMeasFake = TRUE;			/* doesn't matter in this case */
				}
				else {								/* any intervening systems */
					midSysL = LSSearch(startSysL, SYSTEMtype, SystemNUM(startSysL)+j-1, GO_RIGHT, FALSE);
					startL = LSSearch(midSysL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);		/* 1st meas in sys */
					startXD = SysRelxd(startL);
					endL = GetLastMeasInSys(midSysL);												/* last meas in sys */
					lastMeasFake = FakeMeasure(doc, endL);
					if (FirstMeasInSys(endL))
						lastMeasFake = FALSE;				/* if only 1 barline on sys, disregard fakeness */
					if (!lastMeasFake)
						endXD = SysRectRIGHT(midSysL) - SysRectLEFT(midSysL);
					else
						endXD = SysRelxd(endL);
				}
		
				ans = CreateHyphenRun(doc, firstSylL, startL, endL, startXD, endXD,
									!lastMeasFake, voice, hyphenWid, pitchLev, theFont,
									theStyle, dMaxHyphenDist);
			}
		}
		if (ans==-1) { HUnlock((Handle)hyphens); return; }	/* fatal error in CreateHyphenRun */
		else errCnt += ans;
	}	
	HUnlock((Handle)hyphens);
	
	if (errCnt)
		MayErrMsg("%s couldn't create %d hyphen%s.", PROGRAM_NAME, errCnt, errCnt==1? "":"s");
}


static void DisposeHyphenList()
{
	if (hyphens)
		DisposeHandle((Handle)hyphens);
	hyphens = NULL;
	numHyphens = 0;
}


/* Given a pointer to a non-relocatable block <pText>, place a copy of this text into
the relocatable block used by the Flow In Text dialog (the static global lyricBlk).
Do this by allocating a handle and assigning it lyricBlk. If lyricBlk was not NULL,
first dispose it, without warning the user. This is not used by the Flow in Text
command: it was written for the Open ETF command.

If <pText> contains too much text, return FALSE. ??Probably we should truncate the
text in this case.

Returns TRUE if all okay. Gives an error msg and returns FALSE if error. */

Boolean SetFlowInText(Ptr pText)
{
	Size		pTextSize;
	OSErr		err;
	
	if (!pText) {
		MayErrMsg("SetFlowInText: <pText> is NULL!");
		return FALSE;
	}
	pTextSize = GetPtrSize(pText);
	if (pTextSize==0L) {
		MayErrMsg("SetFlowInText: <pText> is empty.");
		return FALSE;
	}
	if (pTextSize>(long)SHRT_MAX) {
		MayErrMsg("SetFlowInText: <pText> contains more than 32K bytes.");
		return FALSE;
	}
	
	if (lyricBlk) {
		DisposeHandle(lyricBlk);
		err = MemError();
		if (err) {
			MayErrMsg("SetFlowInText: DisposHandle on <lyricBlk> failed (%d)", err);
			return FALSE;
		}
	}
	
	err = PtrToHand(pText, &lyricBlk, pTextSize);
	if (err) {
		NoMoreMemory();
		lyricBlk = NULL;
		return FALSE;
	}
	
	return TRUE;
}
