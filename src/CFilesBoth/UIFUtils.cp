/******************************************************************************
*	FILE:	UIFUtils.c
*	PROJ:	Nightingale
*	DESC:	General-purpose utility routines for implementing the user interface.
		GetStaffLim				HiliteInsertNode		HiliteTwoNodesOn
		HiliteAttPoints
		FixCursor				FlashRect				SamePoint
		Advise					NoteAdvise				CautionAdvise
		StopAdvise				Inform					NoteInform
		CautionInform			StopInform				ProgressMsg
		UserInterrupt			UserInterruptAndSel
		NameHeapType			NameNodeType			NameGraphicType	
		ConvertQuote			DrawBox					HiliteRect
		Voice2UserStr			Staff2UserStr
		DrawPUpTriangle			DrawPopUp				TruncPopUpString
		InitPopUp				DoUserPopUp				ChangePopUpChoice
		DisposePopUp			HilitePopUp				ResizePopUp
		ShowPopUp				VLogPrintf				LogPrintf
		InitLogPrintf	
/******************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE� PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright � 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static DDIST GetStaffLim(Document *doc, LINK pL, short s, Boolean top, PCONTEXT pContext);
	

/* ----------------------------------------------------------------- GetStaffLim -- */
/* Return a rough top or bottom staff limit: staffTop - 6 half-lines, or staff
bottom + 6 half-lines. */

static DDIST GetStaffLim(
					Document *doc, LINK pL,
					short staffn,
					Boolean top,					/* TRUE if getting top staff limit. */
					PCONTEXT pContext)
{
	DDIST blackTop, blackBottom,
			dhalfLn;								/* Distance between staff half-lines */

	GetContext(doc, pL, staffn, pContext);
	dhalfLn = pContext->staffHeight/(2*(pContext->staffLines-1));
	if (top) {
		blackTop = pContext->staffTop;
		blackTop -= 6*dhalfLn;
		return blackTop;
	}

	blackBottom = pContext->staffTop+pContext->staffHeight;
	blackBottom += 6*dhalfLn;
	return blackBottom;
}

/* ------------------------------------------------------------ HiliteInsertNode -- */
/* Hilite an object in a distinctive way to show it's the one into which a
subobject is about to be inserted, or to which an object is or will be attached.
The hiliting we do is a vertical dotted line thru the object, thickened slightly
at the "special" staff if there is one. If pL is a structural element--SYSTEM, PAGE,
STAFF, etc.--do nothing. */

void HiliteInsertNode(
			Document *doc,
			LINK	pL,
			short	staffn,			/* No. of "special" staff to emphasize or NOONE */
			Boolean	flash			/* TRUE=flash to emphasize special staff */
			)
{
	short		xd, xp, ypTop, ypBot;
	DDIST		blackTop, blackBottom;
	CONTEXT		context;

	if (!pL) return;											/* Just in case */
	
	if (objTable[ObjLType(pL)].justType!=J_STRUC) {
		if (staffn==NOONE)
			GetContext(doc, pL, 1, &context);
		else {
			blackTop = GetStaffLim(doc, pL, staffn, TRUE, &context);
			blackBottom = GetStaffLim(doc, pL, staffn, FALSE, &context);
		}
		xd = SysRelxd(pL)+context.systemLeft;					/* abs. origin of object */
		
		/* Draw with gray pattern (to make dotted lines), in XOR mode (to invert the
			pixels' existing colors). */
		PenMode(patXor);
		PenPat(NGetQDGlobalsGray());
		xp = context.paper.left+d2p(xd);
		ypTop = context.paper.top+d2p(context.systemTop)+1;
		ypBot = context.paper.top+d2p(context.systemBottom)-1;
		MoveTo(xp+1, ypTop);									/* Draw thru whole system */
		LineTo(xp+1, ypBot);
		
		if (staffn!=NOONE) {
			PenSize(3, 1);										/* Draw thicker on special staff */
			MoveTo(xp, context.paper.top+d2p(blackTop));
			LineTo(xp, context.paper.top+d2p(blackBottom));
			
			if (flash) {
				SleepTicks(4);
				MoveTo(xp, context.paper.top+d2p(blackTop));
				LineTo(xp, context.paper.top+d2p(blackBottom));
				
				SleepTicks(4);
				MoveTo(xp, context.paper.top+d2p(blackTop));
				LineTo(xp, context.paper.top+d2p(blackBottom));
			}
		}
		PenNormal();											/* Restore normal drawing */
	}
}


/* ------------------------------------------------------------ HiliteTwoNodesOn -- */
/* Show in a sensible way insert-type hiliting on two nodes that may be identical.
Should NOT be used to erase the hiliting: for that, call HiliteInsertNode once
or twice. */

void HiliteTwoNodesOn(Document *doc, LINK node1, LINK node2, short staffn)
{	
	HiliteInsertNode(doc, node1, staffn, TRUE);							/* Hiliting on */
	/* If both ends are the same, wait, erase, wait, and draw again */ 
	if (node1==node2) {
		SleepTicks(HILITE_TICKS);
		HiliteInsertNode(doc, node1, staffn, FALSE);
		SleepTicks(HILITE_TICKS);
		HiliteInsertNode(doc, node1, staffn, FALSE);
	}
	else
		HiliteInsertNode(doc, node2, staffn, TRUE);						/* Hiliting on */
}


/* ------------------------------------------------------------- HiliteAttPoints -- */
/* Hilite attachment point(s) and wait as long as the mouse button is down, or in
any case for a short minimum time; then erase the hiliting. If <firstL> is NILINK,
we do nothing at all. <lastL> may be NILINK (to indicate there's really only one
attachment point) or may be identical to <firstL>. In the latter case, we simply
hilite that link twice. */

void HiliteAttPoints(
			Document *doc,
			LINK firstL, LINK lastL,		/* lastL may be NILINK */
			short staffn)
{
	if (!firstL) return;
	
	if (lastL) HiliteTwoNodesOn(doc, firstL, lastL, staffn);					/* On */
	else	   HiliteInsertNode(doc, firstL, staffn, TRUE);						/* On */

	SleepTicks(HILITE_TICKS);
	while (Button()) ;
	
	HiliteInsertNode(doc, firstL, staffn, FALSE);								/* Off */
	if (lastL && firstL!=lastL) HiliteInsertNode(doc, lastL, staffn, FALSE);	/* Off */
}

static void DebugLogPrint(char *str);
static void DebugLogPrint(char *str) 
{
	//LogPrintf(LOG_NOTICE, str);
}

/* -------------------------------------------------------------------- FixCursor -- */
/*	Set cursor shape based on window and mouse position. This version allows "shaking
off the mode".	*/

#define MAXSHAKES	4		/* Changes in X direction before a good shake */
#define CHECKMIN	2		/* Number ticks before next dx check */
#define SWAPMIN		30		/* Number ticks between consecutive shakeoffs */

void FixCursor()
{
	Point			mousept, globalpt;
	WindowPtr		wp;
	CursHandle		newCursor;
	Document		*doc;
	static short	x, xOld, dx, dxOld, shaker, shookey;
	static long 	now, soon, nextcheck;
	PaletteGlobals *toolPalette;
	char			message[256];
	Boolean			foundPalette = FALSE;
	
	toolPalette = *paletteGlobals[TOOL_PALETTE];
	
	/*
	 * <currentCursor> is a global that Nightingale uses elsewhere to keep
	 * track of the cursor. <newCursor> is a temporary variable used solely to
	 * set the cursor here. They'll always be the same except when a modifier
	 * key is down that forces a specific cursor temporarily--at the moment,
	 * only the <genlDragCursor>.
	 */
	GetMouse(&mousept);
	globalpt = mousept;
	LocalToGlobal(&globalpt);
	
	/* If no windows at all, use arrow */
	
	if (TopWindow == NULL) {
		holdCursor = FALSE; DebugLogPrint("1. TopWindow is null\n"); ArrowCursor(); return;
		}
	
	/* If mouse over any palette, use arrow unless a tool was just chosen */

	if (GetWindowKind(TopWindow) == PALETTEKIND) {
		holdCursor = TRUE;
		for (wp=TopPalette; wp!=NULL && GetWindowKind(wp)==PALETTEKIND; wp=GetNextWindow(wp)) {
			RgnHandle strucRgn = NewRgn();
			GetWindowRegion(wp, kWindowStructureRgn, strucRgn);
			if (PtInRgn(globalpt, strucRgn)) {
				//if (!holdCursor) ArrowCursor();
				DisposeRgn(strucRgn);
				foundPalette = TRUE;
				}
				else {
				DisposeRgn(strucRgn);
				}
			}
		}

	holdCursor = FALSE;			/* OK to change cursor back to arrow when it's over a palette */
	
	/* If no Documents, use arrow */
	
	if (TopDocument==NULL) { DebugLogPrint("3. TopDocument is null\n"); ArrowCursor(); return; }

	/* If mouse over any kind of window other than a Document, use arrow */
	
	FindWindow(globalpt, (WindowPtr *)&wp);
	
	if (wp==NULL) { DebugLogPrint("3-1. FoundWindow is null\n"); ArrowCursor(); return; }
	if (GetWindowKind(wp)==PALETTEKIND) { DebugLogPrint("3-2. FoundWindow is a palette\n"); return; }	
	if (GetWindowKind(wp)!=DOCUMENTKIND) {
		DebugLogPrint("3-3. FoundWindow not our Document\n"); ArrowCursor(); return;
		}
	
	doc = GetDocumentFromWindow(wp);
	
	if (doc==NULL) { ArrowCursor(); DebugLogPrint("4. FixCursor: doc is null\n"); return; }
	sprintf(message, "4.1 found document %s\n", doc->name);
	DebugLogPrint(message);
	
	/* If mouse not over the Document's viewRect, use arrow */
	
	if (!PtInRect(mousept,&doc->viewRect)) { DebugLogPrint("5. Not in viewRect\n");  ArrowCursor(); return; }
	
	/* If Document showing Master Page or Work on Format, use arrow */
	
	if (doc->masterView || doc->showFormat) { DebugLogPrint("6. MasterPage or ShowFormat\n"); ArrowCursor(); return; }
	
	/*
	 * Mouse is over the viewRect of a Document that is showing the "normal" (not
	 * Master Page or Work on Format) view. Now we have to think harder.
	 */
	 
	newCursor = currentCursor;
	
	/*
	 *	At this point, we check for shakeoffs.  We do this by keeping track of the 
	 * times at which the mouse changes horizontal direction, but no oftener than
	 * CHECKMIN ticks, since otherwise we won't pick up the direction changes
	 * (dx == 0 most of the time). Each time the direction does change we check to
	 * see if it has happened within mShakeThresh ticks of the last change.  If it
	 * hasn't, we start over.  If it has, we keep track of the number of consecutive
	 * shakes, each occurring within mShakeThresh ticks of each other, until there are
	 * MAXSHAKES of them, in which case we change the cursor to the arrow cursor.
	 *	We save the previous cursor (in shook), so that on the next shakeoff, we can
	 *	go back to whatever used to be there, if it's still the arrow. Finally, we
	 *	don't allow consecutive shakeoffs to occur more often than SWAPMIN ticks.
	 *	Yours truly, Doug McKenna.
	 */
	now = TickCount();
	if (now > nextcheck) {
		x = mousept.h;
		dx = x - xOld;
		nextcheck = now + CHECKMIN;
		
		if ((dx<0 && dxOld>0) || (dx>0 && dxOld<0))
		
			if (shaker++ == 0)
				soon = now + config.mShakeThresh;					/* Start timing */
			 else
				if (now < soon)										/* A quick-enough shake ? */
					if (shaker >= MAXSHAKES) {
						if (shookey==0 || currentCursor!=arrowCursor) {
							shookey = GetPalChar((*paletteGlobals[TOOL_PALETTE])->currentItem);
							shook = currentCursor;
							PalKey(CH_ENTER);
							newCursor = arrowCursor;
							}
						 else {
							PalKey(shookey);
							newCursor = shook;
							shookey = 0;
							}
						shaker = 0;
						currentCursor = newCursor;
						nextcheck += SWAPMIN;
						}
					 else
						soon = now + config.mShakeThresh;
				 else
					shaker = 0;												/* Too late: reset */

		dxOld = dx; xOld = x;
		}
	
	/*
	 * If option key is down and shift key is NOT down, use "general drag" cursor, no
	 *	matter what. The shift key test is unfortunately necessary so we can use
	 * shift-option-E (for example) as the keyboard equivalent for eighth grace
	 *	note without confusing users by having the wrong cursor as long they actually
	 * have the option key down. */

	if (OptionKeyDown() && !ShiftKeyDown()) newCursor = genlDragCursor;
	
	DebugLogPrint("7. Installing current cursor\n");
	
	/* Install the new cursor, whatever it may be. */
	SetCursor(*newCursor);
}


/* ------------------------------------------------------------------- FlashRect -- */

#define FLASH_RECT_COUNT 1

void FlashRect(Rect *pRect)
{
	short	j;
	
	for (j=0; j<FLASH_RECT_COUNT; j++) {
		InvertRect(pRect);
		SleepTicks(20L);
		InvertRect(pRect);
		SleepTicks(20L);		
	}
}


/* ------------------------------------------------------------------- SamePoint -- */
/* Check to see if two Points (presumably on the screen) are within slop of each
other. Needed for editing beams, lines, hairpins, etc. */

#define SAME_POINT_SLOP 2

Boolean SamePoint(Point p1, Point p2)
{
	short dx, dy;
	
	dx = p1.h - p2.h; if (dx < 0) dx = -dx;
	dy = p1.v - p2.v; if (dy < 0) dy = -dy;
	
	return (dx<=SAME_POINT_SLOP && dy<=SAME_POINT_SLOP);
}
	

/* ------------------------------------------------------------- Advise functions -- */
/* Give alerts of varlous types and return value. */

short Advise(short alertID)
{
	LogPrintf(LOG_NOTICE, "* Advise alertID=%d.\n\n", alertID);
	ArrowCursor();
	return Alert(alertID, NULL);
}

short NoteAdvise(short alertID)
{
	LogPrintf(LOG_NOTICE, "* NoteAdvise alertID=%d.\n", alertID);
	ArrowCursor();
	return NoteAlert(alertID, NULL);
}

short CautionAdvise(short alertID)
{
	LogPrintf(LOG_NOTICE, "* CautionAdvise alertID=%d!\n", alertID);
	ArrowCursor();
	return CautionAlert(alertID, NULL);
}

short StopAdvise(short alertID)
{
	LogPrintf(LOG_WARNING, "* StopAdvise alertID=%d!\n", alertID);
	ArrowCursor();
	return StopAlert(alertID, NULL);
}


/* ------------------------------------------------------------- Inform functions -- */
/* Give valueless alerts of various types. */
							
void Inform(short alertID)
{
	short dummy;
	
	LogPrintf(LOG_NOTICE, "* Inform alertID=%d\n", alertID);
	ArrowCursor();
	dummy = Alert(alertID, NULL);
}
						
void NoteInform(short alertID)
{
	short dummy;
	
	LogPrintf(LOG_NOTICE, "* NoteInform alertID=%d\n", alertID);
	ArrowCursor();
	dummy = NoteAlert(alertID, NULL);
}
						
void CautionInform(short alertID)
{
	short dummy;
	
	LogPrintf(LOG_NOTICE, "* CautionInform alertID=%d\n", alertID);
	ArrowCursor();
	dummy = CautionAlert(alertID, NULL);
}

void StopInform(short alertID)
{
	short dummy;
	
	LogPrintf(LOG_WARNING, "* StopInform alertID=%d\n", alertID);
	ArrowCursor();
	dummy = StopAlert(alertID, NULL);
}


/* ----------------------------------------------------------------- ProgressMsg -- */

#define MAX_MESSAGE_STR 19		/* Index of last message string */

#define MESSAGE_DI 1

/* If which!=0, put up a small window (actually a dialog) containing the specified
message, and pause for a brief time (HILITE_TICKS ticks). If which=0, remove the
window. If which!=0 and there's already a ProgressMsg window up, remove it before
putting up the new one. */

Boolean ProgressMsg(short which,
						char *moreInfo)				/* C string */
{
	static short lastWhich=-999;
	static DialogPtr dialogp=NULL; GrafPtr oldPort; char str[256];
	short aShort; Handle tHdl; Rect aRect;
	 
	GetPort(&oldPort);

	if (which>0 && which<=MAX_MESSAGE_STR) {
	
		if (dialogp && which!=lastWhich) ProgressMsg(0, "");		/* Remove existing msg window */
		lastWhich = which;
		
		if (!dialogp) {
			dialogp = GetNewDialog(MESSAGE_DLOG, NULL, BRING_TO_FRONT);
			if (!dialogp) return FALSE;
		}

		SetPort(GetDialogWindowPort(dialogp));

		GetIndCString(str, MESSAGE_STRS, which);
		strcat(str, moreInfo);
		GetDialogItem(dialogp, MESSAGE_DI, &aShort, &tHdl, &aRect);
		SetDialogItemCText(tHdl, str);

		CenterWindow(GetDialogWindow(dialogp),100);
		ShowWindow(GetDialogWindow(dialogp));
		UpdateDialogVisRgn(dialogp);
		SleepTicks(HILITE_TICKS);								/* So it's not erased TOO quickly */
	}

	else if (which==0) {
		if (dialogp) {
			HideWindow(GetDialogWindow(dialogp));
			DisposeDialog(dialogp);										/* Free heap space */
			dialogp = NULL;
		}
	}

	SetPort(oldPort);
	return TRUE;
}


/* ---------------------------------------------------------------- UserInterrupt -- */
/*	Returns TRUE if COMMAND and PERIOD (.) keys are both currently down; all other
keys are ignored. FIXME: Should be internationalized! */

Boolean UserInterrupt()
{
	if (!CmdKeyDown()) return FALSE;
	if (!KeyIsDown(47)) return FALSE;
	
	return TRUE;
}


/* ---------------------------------------------------------- UserInterruptAndSel -- */
/* Returns TRUE if COMMAND and SLASH (/) keys are both currently down; all other
keys are ignored. FIXME: Should be internationalized! */

Boolean UserInterruptAndSel()
{
	if (!CmdKeyDown()) return FALSE;
	if (!KeyIsDown(44)) return FALSE;
	
	return TRUE;
}


/* --------------------------------------------------- NameHeapType, NameNodeType -- */
/* Given a "heap index" or object type, return the name of the corresponding object. */

const char *NameHeapType(
			short heapIndex,
			Boolean friendly)		/* TRUE=give user-friendly names, FALSE=give "real" names */
{
	const char *ps;

	switch (heapIndex) {
		case HEADERtype:	ps = "HEAD"; break;
		case TAILtype:		ps = "TAIL"; break;
		case SYNCtype:		ps = (friendly? "note and rest" : "SYNC"); break;
		case RPTENDtype:	ps = (friendly? "repeat bar/segno" : "REPEATEND"); break;
		case ENDINGtype:	ps = (friendly? "ending" : "ENDING"); break;
		case PAGEtype:		ps = (friendly? "page" : "PAGE"); break;
		case SYSTEMtype:	ps = (friendly? "system" : "SYSTEM"); break;
		case STAFFtype:		ps = (friendly? "staff" : "STAFF"); break;
		case MEASUREtype:	ps = (friendly? "measure" : "MEASURE"); break;
		case PSMEAStype:	ps = (friendly? "pseudomeasure" : "PSEUDOMEAS"); break;
		case CLEFtype:		ps = (friendly? "clef" : "CLEF"); break;
		case KEYSIGtype:	ps = (friendly? "key signature" : "KEYSIG"); break;
		case TIMESIGtype:	ps = (friendly? "time signature" : "TIMESIG"); break;
		case BEAMSETtype:	ps = (friendly? "beamed group" : "BEAMSET"); break;
		case TUPLETtype:	ps = (friendly? "tuplet" : "TUPLET"); break;
		case CONNECTtype:	ps = (friendly? "staff bracket" : "CONNECT"); break;
		case DYNAMtype:		ps = (friendly? "dynamic" : "DYNAMIC"); break;
		case MODNRtype:		ps = (friendly? "note modifier" : "MODNR"); break;
		case GRAPHICtype:	ps = (friendly? "Graphic" : "GRAPHIC"); break;
		case OTTAVAtype:	ps = (friendly? "octave sign" : "OCTAVE"); break;
		case SLURtype:		ps = (friendly? "slur and tie" : "SLUR"); break;
		case GRSYNCtype:	ps = (friendly? "grace note" : "GRSYNC"); break;
		case TEMPOtype:		ps = (friendly? "tempo mark" : "TEMPO"); break;
		case SPACERtype:		ps = (friendly? "spacer" : "SPACER"); break;
		case OBJtype:		ps = "OBJECT"; break;
		default:			ps = "**UNKNOWN**";
	}

	return ps;
}


/* Given an object, return its "real" (and short) name. */

const char *NameNodeType(LINK pL)
{
	return NameHeapType(ObjLType(pL), FALSE);
}


/* -------------------------------------------------------------- NameGraphicType -- */
/* Given an object of type GRAPHIC, return its subtype. */

const char *NameGraphicType(
			LINK pL,
			Boolean friendly)		/* TRUE=give user-friendly names, FALSE=give "real" names */
{
	const char *ps;

	if (!GraphicTYPE(pL)) {											/* Just in case */
		ps = "*NOT A GRAPHIC*";
		return ps;
	}

	switch (GraphicSubType(pL)) {
		case GRPICT:			ps = (friendly? "PICT" : "GRPICT");  break;
		case GRString:			ps = (friendly? "Text" : "GRString");  break;
		case GRLyric:			ps = (friendly? "Lyric" : "GRLyric");  break;
		case GRDraw:			ps = (friendly? "Draw" : "GRDraw");  break;
		case GRRehearsal:		ps = (friendly? "Rehearsal" : "GRRehearsal");  break;
		case GRChordSym:		ps = (friendly? "Chord Sym" : "GRChordSym");  break;
		case GRArpeggio:		ps = (friendly? "Arpeggio" : "GRArpeggio");  break;
		case GRChordFrame:		ps = (friendly? "Chord Frame" : "GRChordFrame");  break;
		case GRMIDIPatch:		ps = (friendly? "MIDI Patch" : "GRMIDIPatch");  break;
		case GRMIDISustainOn:	ps = (friendly? "Sustain On" : "GRMIDISustainOn");  break;
		case GRMIDISustainOff:	ps = (friendly? "Sustain Off" : "GRMIDISustainOff");  break;
		case GRMIDIPan:			ps = (friendly? "Pan" : "GRMIDIPan");  break;
		default:				ps = (friendly? "*UNKNOWN*" : "*UNKNOWN*");
	}

	return ps;
}


/* ---------------------------------------------------------------- ConvertQuote -- */
/*	Given a TextEdit handle and a character, presumably one being inserted, if that
character is a (neutral) double quote or apostrophe, change it to its open/close
equivalent character according to the type of character that precedes it, if any;
return the converted character.  If the character not a neutral quote or apostrophe,
then just return it unchanged.
 *
The conversion to open quote is done if the previous char was a space, tab, option-
space or return; closed quote otherwise, unless it's the first character in the
string, in which case it's always open. If you want to allow the typing of standard
ASCII single or double quote, do it as a Command key char. */

short ConvertQuote(TEHandle textH, short ch)
	{
		short n; unsigned char prev;

		if (textH)
			if (ch=='"' || ch=='\'') {
				n = (*textH)->selStart;
				prev = (n > 0) ? ((unsigned char) *(*((*textH)->hText) + n-1)) : 0;
				if (prev=='\r' || prev==' ' || prev==' ' || prev=='\t' || n==0)
					ch = (ch=='"' ? '“' : '‘');
				 else
					ch = (ch=='"' ? '”' : '’');
				}
		return(ch);
	}


/* --------------------------------------------------------------------- DrawBox -- */
/* Use a wide line to draw a filled box centered at the given point.  Size should
normally be 4 so that the special drag cursor fits in with it. Intended to draw
handles for dragging.*/

void DrawBox(Point pt, short size)
	{
		PenSize(size,size); size >>= 1;
		MoveTo(pt.h -= size,pt.v -= size);
		LineTo(pt.h,pt.v);
		PenSize(1,1);
	}


/* ------------------------------------------------------------------ HiliteRect -- */
/*	Toggle the hiliting of the given rectangle, using the selection color. Intended
for object/subobject  hiliting. */

void HiliteRect(Rect *r)
{
	LMSetHiliteMode(LMGetHiliteMode() & 0x7F);	/* Clear top bit = hilite in selection color */

	InsetRect(r,-config.enlargeHilite,-config.enlargeHilite);
	InvertRect(r);
	InsetRect(r,config.enlargeHilite,config.enlargeHilite);
}


/* ---------------------------------------------------------- Voice/Staff2UserStr -- */

/* Given an (internal) voice no., return its part and part-relative voice no. in
user-friendly format, e.g., "voice 2 of Piano".
FIXME: Based on code in HasSmthgAcross: that and similar code in 3-4 places should
be replaced with calls to this. */

void Voice2UserStr(Document *doc,
						short voice,
						char str[])			/* user-friendly string describing the voice */
{
	short userVoice;
	LINK partL; PPARTINFO pPart;
	char partName[256], fmtStr[256];

	Int2UserVoice(doc, voice, &userVoice, &partL);
	pPart = GetPPARTINFO(partL);
	strcpy(partName, (strlen(pPart->name)>6? pPart->shortName : pPart->name));
	GetIndCString(fmtStr, MISCERRS_STRS, 28);						/* "voice %d of %s" */
	sprintf(str, fmtStr, userVoice, partName);
}

/* Given an (internal) staff no., return its part and part-relative staff no. in
user-friendly format, e.g., "staff 3 (Clarinet)" or "staff 5 (staff 2 of Piano)".
FIXME: Based on code in StfHasSmthgAcross: that and similar code in 3-4 places should
be replaced with calls to this. */

void Staff2UserStr(Document *doc,
						short staffn,
						char str[])			/* user-friendly string describing the staff */
{
	short relStaff;
	LINK partL; PPARTINFO pPart;
	char fmtStr[256]; short len;

	partL = Staff2PartL(doc, doc->headL, staffn);
	pPart = GetPPARTINFO(partL);
	GetIndCString(fmtStr, MISCERRS_STRS, 29);						/* "staff %d" */
	sprintf(str, fmtStr, staffn);
	relStaff = staffn-pPart->firstStaff+1;
	len = strlen((char *)str);
	if (pPart->lastStaff>pPart->firstStaff) {
		GetIndCString(fmtStr, MISCERRS_STRS, 30);					/* "(staff %d of %s)" */
		sprintf(&str[len], fmtStr, relStaff, pPart->name);
	}
	else
		sprintf(&str[len], " (%s)", pPart->name);
}


/* ============================================================  Pop-up Utilities == */
/* The following utilities for handling pop-ups were written by Resorcerer and are
included by its kind permission and permission of its agent, Mr. Douglas M. McKenna. */

#define L_MARGIN 13
#define R_MARGIN 5
#define B_MARGIN 5

#define TRIANGLE_MARGIN 16

static void DrawPUpTriangle(Rect *bounds);
static void DrawPUpTriangle(Rect *bounds)
	{
		Rect box;
		
		box = *bounds;
		box.left = box.right - TRIANGLE_MARGIN;
		InsetRect(&box,1,1);
		EraseRect(&box);
		
		/* Draw triangle in right side of popup bounds, centered vertically */
		
		MoveTo(box.left,bounds->top+((bounds->bottom-bounds->top)-6)/2);
		Line(10,0); Move(-1,1);
		Line(-8,0); Move(1,1);
		Line(6,0); Move(-1,1);
		Line(-4,0); Move(1,1);
		Line(2,0); Move(-1,1);
		Line(0,0);
	}


/* Draw a given popup user item in its normal (un-popped-up) state */

void DrawPopUp(UserPopUp *p)
	{
		FrameRect(&p->bounds);
		/* And its drop shadow */
		MoveTo(p->bounds.right,p->shadow.top+2);
		LineTo(p->bounds.right,p->bounds.bottom);
		LineTo(p->shadow.left+2,p->bounds.bottom);
		/* And draw the current setting str in */
		MoveTo(p->bounds.left+L_MARGIN,p->bounds.bottom-B_MARGIN);
		EraseRect(&p->box);
		DrawString(p->str);
		DrawPUpTriangle(&p->bounds);
	}


/* TruncPopUpString() gets the string for the current setting and fits it into
the popup item's bounding box, if necessary truncating it and adding the
ellipsis character. */

void TruncPopUpString(UserPopUp *p)
	{
		short width,space,len;

		if (p->currentChoice)
			GetMenuItemText(p->menu,p->currentChoice,p->str);
		 else
			*p->str = 0;
		space = p->bounds.right - p->bounds.left - (L_MARGIN+R_MARGIN);

		/* We want the StringWidth ignoring trailing blanks */
		len = *(unsigned char *)p->str;
		while (len > 0) if (p->str[len]!=' ') break; else len--;
		*p->str = len;

		width = StringWidth(p->str);
		if (width > space) {
			len = *p->str;
			width -= CharWidth('…');
			while (len>0 && width>space)
				width -= CharWidth(p->str[len--]);
			p->str[++len] = '…';
			*p->str = len;
			}
	}


/* Initialise a UserPopUp data structure; return FALSE if error. If firstChoice=0,
no item is initially chosen. */

short InitPopUp(
			DialogPtr dlog,
			UserPopUp *p,
			short item,				/* Dialog item to set p->box from */
			short pItem,			/* Dialog item to set p->prompt from, or 0=set it empty */
			short menuID,
			short firstChoice		/* Initial choice, or 0=none */
			)
	{
		short type; Handle hndl;

		if (pItem) GetDialogItem(dlog,pItem,&type,&hndl,&p->prompt);
		 else		  SetRect(&p->prompt,0,0,0,0);
		
		GetDialogItem(dlog,item,&type,&hndl,&p->box);
		p->bounds = p->box; InsetRect(&p->bounds,-1,-1);
		p->shadow = p->bounds;
		p->shadow.right++; p->shadow.bottom++;
		p->menu = GetMenu(p->menuID = menuID);
		if (!p->menu) { SysBeep(1); return(FALSE); }
		p->currentChoice = firstChoice;
		TruncPopUpString(p);

		return(TRUE);
	}


/* Invoke a popup menu; return TRUE if new choice was made */

short DoUserPopUp(UserPopUp *p)
	{
		long choice; Point pt; short ans = FALSE;

		InvertRect(&p->prompt);
		InsertMenu(p->menu,-1);
		CalcMenuSize(p->menu);
		pt = *(Point *)(&p->box);
		LocalToGlobal(&pt);
		choice = PopUpMenuSelect(p->menu,pt.v,pt.h,p->currentChoice);
		InvertRect(&p->prompt);
		DeleteMenu(p->menuID);
		if (choice) {
			choice = LoWord(choice);
			if (choice != p->currentChoice) {
				ChangePopUpChoice(p,(short)choice);
				ans = TRUE;
				}
			}
		return(ans);
	}


/* Change a given popup to a given choice, regardless of the current popup choice.
This means unchecking the old choice, if any, and checking the new one, resetting
the display string, and redisplaying. */

void ChangePopUpChoice(UserPopUp *p, short choice)
	{
		if (p->currentChoice)
			SetItemMark(p->menu,p->currentChoice,0);
		if (choice>=1 && choice<=CountMenuItems(p->menu))
			SetItemMark(p->menu,choice,(char)checkMark);
		p->currentChoice = choice;
		
		TruncPopUpString(p);
		EraseRect(&p->box);
		DrawPopUp(p);
	}


/* Do end-of-dialog cleanup. */

void DisposePopUp(UserPopUp *p)
	{
		if (p->menu) DisposeMenu(p->menu);
		p->menu = NULL;
	}


/* Hilite or remove hiliting from the un-popped-up popup to show if it's active. */

void HilitePopUp(UserPopUp *p, short activ)
	{
		EraseRect(&p->box);

		/* Redraw the menu string */
		MoveTo(p->bounds.left+L_MARGIN,p->bounds.bottom-B_MARGIN);
		DrawString(p->str);
		DrawPUpTriangle(&p->bounds);

		if (activ) {
			PenPat(NGetQDGlobalsGray()); PenSize(2,1);
			FrameRect(&p->box);			
			PenNormal();
		}
	}


/* ResizePopUp changes the widths of an existing popup's rects to accommodate
its longest menu item string, and returns 1. The top left point isn't changed.
If no change is necessary, ResizePopUp returns 0. */
 
short ResizePopUp(UserPopUp *p)
	{
		short thisWidth,maxWidth = 0,space,lastItemNum,menuItem;
		Str255 str;
		
		lastItemNum = CountMenuItems(p->menu);
		
		for (menuItem=1;menuItem<=lastItemNum;menuItem++) {
			GetMenuItemText(p->menu,menuItem,str);
			thisWidth = StringWidth(str);
			maxWidth = n_max(maxWidth,thisWidth);
		}

		maxWidth += TRIANGLE_MARGIN-4;		/* <maxWidth> is generous so we can cheat a bit */
		
		space = p->bounds.right - p->bounds.left - (L_MARGIN+R_MARGIN);
		if (space==maxWidth) return(0);
		
		p->bounds.right = p->bounds.left + maxWidth + (L_MARGIN+R_MARGIN);
		p->box = p->bounds; InsetRect(&p->box,1,1);
		p->shadow = p->bounds;
		p->shadow.right++; p->shadow.bottom++;
		return(1);
	}


/*	ShowPopUp does the job for pop-ups that HideDialogItem and ShowDialogItem do for normal
dialog items. */

void ShowPopUp(UserPopUp *p, short vis)
	{
		switch (vis)	{
			case TRUE:
				if (p->box.left > 16000)	{
					p->box.left -= 16384;
					p->box.right -= 16384;}
				break;
			case FALSE:
				if (p->box.left < 16000)	{
					p->box.left += 16384;
					p->box.right += 16384;}	
				break;
		}			
		p->bounds = p->box; InsetRect(&p->bounds,-1,-1);
		p->shadow = p->bounds;
		p->shadow.right++; p->shadow.bottom++;
	}


/* Let the user select popup items using arrow keys. Doesn't work if the menu
has any disabled items, unless they are the '--------' kind and are not the first
or last items. Probably by John Gibson, not Resorcerer. */

void HiliteArrowKey(DialogPtr /*dlog*/, short whichKey, UserPopUp *pPopup,
										Boolean *pHilitedItem)
{
	short lastItemNum, newChoice;
	UserPopUp *p;
					
	if (!*pHilitedItem) {
		*pHilitedItem = TRUE;
		HilitePopUp(pPopup, TRUE);
	}
	else {
		p = pPopup;
		newChoice = p->currentChoice;
		lastItemNum = CountMenuItems(p->menu);
		if (whichKey == UPARROWKEY) {				
			newChoice -= 1;
			if (newChoice == 0) return;					/* Top of menu reached; don't wrap around. */
			GetMenuItemText(p->menu, newChoice, p->str);
			if (p->str[1] == '-') newChoice -= 1;		/* Skip over the dash item */ 						
		}
		else {
			newChoice += 1;
			if (newChoice == lastItemNum + 1) return;	/* Bottom of menu reached; don't wrap around. */
			GetMenuItemText(p->menu, newChoice, p->str);
			if (p->str[1] == '-') newChoice += 1;		/* Skip over the dash item */ 						
		}
		ChangePopUpChoice (p, newChoice);
		HilitePopUp(p,TRUE);
	}
}


// QuickDraw Globals

// QD Globals

Rect GetQDPortBounds()
{
	Rect bounds;
	
	GetPortBounds(GetQDGlobalsThePort(), &bounds);
	
	return bounds;
}

Rect GetQDScreenBitsBounds()
{
	BitMap screenBits;
	
	GetQDGlobalsScreenBits(&screenBits);
	
	return screenBits.bounds;
}

Rect *GetQDScreenBitsBounds(Rect *bounds)
{
	static Rect screenBitBounds = {0,0,0,0};
	BitMap screenBits;
	
	GetQDGlobalsScreenBits(&screenBits);
	
	screenBitBounds = screenBits.bounds;
	*bounds = screenBitBounds;
	
	return &screenBitBounds;
}

Pattern *NGetQDGlobalsDarkGray()
{
	static Pattern sDkGray;
	static Boolean once = TRUE;
	
	if (once)
	{	
		GetQDGlobalsDarkGray(&sDkGray);
		once = FALSE;
	}
	
	return &sDkGray;
}

Pattern *NGetQDGlobalsLightGray()
{
	static Pattern sLtGray;
	static Boolean once = TRUE;
	
	if (once)
	{	
		GetQDGlobalsLightGray(&sLtGray);
		once = FALSE;
	}
	
	return &sLtGray;
}

Pattern *NGetQDGlobalsGray()
{
	static Pattern sGray;
	static Boolean once = TRUE;
	
	if (once)
	{	
		GetQDGlobalsGray(&sGray);
		once = FALSE;
	}
	
	return &sGray;
}

Pattern *NGetQDGlobalsBlack()
{
	static Pattern sBlack;
	static Boolean once = TRUE;
	
	if (once)
	{	
		GetQDGlobalsBlack(&sBlack);
		once = FALSE;
	}
	
	return &sBlack;
}

Pattern *NGetQDGlobalsWhite()
{
	static Pattern sWhite;
	static Boolean once = TRUE;
	
	if (once)
	{	
		GetQDGlobalsWhite(&sWhite);
		once = FALSE;
	}
	
	return &sWhite;
}


// Documents

// See also GetDocumentFromWindow(WindowPtr w) in Documents.c

GrafPtr GetDocWindowPort(Document *doc)
{
	return GetWindowPort(doc->theWindow);
}

// Ports


GrafPtr GetDialogWindowPort(DialogPtr dlog)
{
	return GetWindowPort(GetDialogWindow(dlog));
}

void GetDialogPortBounds(DialogPtr d, Rect *boundsR)
{
	GetWindowPortBounds(GetDialogWindow(d), boundsR);
}

//	bounds = (*(((WindowPeek)doc))->strucRgn)->rgnBBox;
//	
// GetWindowRegionBounds(w,kWindowStructureRgn,&bounds);

void GetWindowRgnBounds(WindowPtr w, WindowRegionCode rgnCode, Rect *bounds)
{
	RgnHandle wRgn = NewRgn();
	
	GetWindowRegion(w, rgnCode, wRgn);
	GetRegionBounds(wRgn,bounds);
	DisposeRgn(wRgn);
}
	
// Dialogs

void UpdateDialogVisRgn(DialogPtr d)
{
	RgnHandle visRgn;
	Rect portRect;

	visRgn = NewRgn();
	
	GetPortVisibleRegion(GetDialogWindowPort(d), visRgn);
	GetDialogPortBounds(d,&portRect);

	BeginUpdate(GetDialogWindow(d));
	
	UpdateDialog(d, visRgn);	
	FlushPortRect(&portRect);
	EndUpdate(GetDialogWindow(d));
	
	DisposeRgn(visRgn);

	DrawDialog(d);	
}

// Lists

void LUpdateDVisRgn(DialogPtr d, ListHandle lHdl)
{
	RgnHandle visRgn;

	visRgn = NewRgn();
	
	GetPortVisibleRegion(GetDialogWindowPort(d), visRgn);

	LUpdate(visRgn, lHdl);			
	
	DisposeRgn(visRgn);
}

void LUpdateWVisRgn(WindowPtr w, ListHandle lHdl)
{
	RgnHandle visRgn;

	visRgn = NewRgn();
	
	GetPortVisibleRegion(GetWindowPort(w), visRgn);

	LUpdate(visRgn, lHdl);			
	
	DisposeRgn(visRgn);
}

void FlushPortRect(Rect *r)
{
#if TARGET_API_MAC_CARBON
	GrafPtr port;
	RgnHandle rgn;
	GetPort(&port);
	if (QDIsPortBuffered(port)) {
		rgn = NewRgn();
		RectRgn(rgn,r);
		QDFlushPortBuffer(port,rgn);
		DisposeRgn(rgn);
	}
#endif
}

void FlushWindowPortRect(WindowPtr w)
{
#if TARGET_API_MAC_CARBON
	Rect portRect;
	GrafPtr port;
	RgnHandle rgn;
	port = GetWindowPort(w);
	if (QDIsPortBuffered(port)) {
		GetPortBounds(port,&portRect);
		rgn = NewRgn();
		RectRgn(rgn,&portRect);
		QDFlushPortBuffer(port,rgn);
		DisposeRgn(rgn);
	}
#endif
}

void FlushDialogPortRect(DialogPtr d)
{
	FlushWindowPortRect(GetDialogWindow(d));
}

// Port font, face, size


short GetPortTxFont()
{
	GrafPtr port = GetQDGlobalsThePort();
		
	return GetPortTextFont(port);
}

short GetPortTxFace()
{
	GrafPtr port = GetQDGlobalsThePort();
		
	return GetPortTextFace(port);
}

short GetPortTxSize()
{
	GrafPtr port = GetQDGlobalsThePort();
		
	return GetPortTextSize(port);
}

short GetPortTxMode()
{
	GrafPtr port = GetQDGlobalsThePort();
		
	return GetPortTextMode(port);
}

// Window font, face, size


short GetWindowTxFont(WindowPtr w)
{
	return GetPortTextFont(GetWindowPort(w));
}

short GetWindowTxFace(WindowPtr w)
{
	return GetPortTextFace(GetWindowPort(w));
}

short GetWindowTxSize(WindowPtr w)
{
	return GetPortTextSize(GetWindowPort(w));
}

void SetWindowTxFont(WindowPtr w, short font)
{
	SetPortTextFont(GetWindowPort(w), font);
}

void SetWindowTxFace(WindowPtr w, short face)
{
	SetPortTextFace(GetWindowPort(w), face);
}

void SetWindowTxSize(WindowPtr w, short size)
{
	SetPortTextSize(GetWindowPort(w), size);
}

// Dialog font, face, size

short GetDialogTxFont(DialogPtr d)
{
	return GetWindowTxFont(GetDialogWindow(d));
}

short GetDialogTxFace(DialogPtr d)
{
	return GetWindowTxFace(GetDialogWindow(d));
}

short GetDialogTxSize(DialogPtr d)
{
	return GetWindowTxSize(GetDialogWindow(d));
}

void SetDialogTxFont(DialogPtr d, short font)
{
	SetWindowTxFont(GetDialogWindow(d), font);
}

void SetDialogTxFace(DialogPtr d, short face)
{
	SetWindowTxFace(GetDialogWindow(d), face);
}

void SetDialogTxSize(DialogPtr d, short size)
{
	SetWindowTxSize(GetDialogWindow(d), size);
}


// Window kinds

short IsPaletteKind(WindowPtr w)
{
	return (GetWindowKind(w) == PALETTEKIND);
}

short IsDocumentKind(WindowPtr w)
{
	return (GetWindowKind(w) == DOCUMENTKIND);
}

void SetPaletteKind(WindowPtr w)
{
	SetWindowKind(w, PALETTEKIND);
}

void SetDocumentKind(WindowPtr w)
{
	SetWindowKind(w, DOCUMENTKIND);
}


// Controls

void OffsetContrlRect(ControlRef ctrl, short dx, short dy)
{
	Rect contrlRect;
	
	GetControlBounds(ctrl, &contrlRect);
	OffsetRect(&contrlRect, dx, dy);
	SetControlBounds(ctrl, &contrlRect);
}

// Strings

StringPtr CtoPstr(StringPtr str)
{
	Byte len = strlen((char *)str);
	StringPtr p = &str[1];
	
	BlockMove(str,p,len);
	str[0] = len;
	
	return str;
}

StringPtr PtoCstr(StringPtr str)
{
	Byte len = str[0];
	StringPtr p = &str[1];
	
	BlockMove(p,str,len);
	str[len] = '\0';
	
	return str;
}


/* ------------------------------------------------------------ Log-file handling -- */
/* NB: LOG_DEBUG is the lowest level (=_highest_ internal code: caveat!), so --
according to the man page for syslog(3) -- the call to setlogmask in InitLogPrintf()
_should_ cause all messages to go to the log regardless of level; but on my G5, I
rarely if ever get anything below LOG_NOTICE. To avoid this problem, LogPrintf()
can change anything with a lower level(=higher code) to a given level, MIN_LOG_LEVEL.
To disable that feature, set MIN_LOG_LEVEL to a large number (not a small one!), say
999. */

#define MIN_LOG_LEVEL LOG_NOTICE			/* Force lower levels (higher codes) to this */

#define LOG_TO_STDERR FALSE					/* Print to stderr as well as system log? */

static Boolean HaveNewline(const char *str);

char inStr[1000], outStr[1000];

static Boolean HaveNewline(const char *str)
{
	while (*str!='\0') {
		if (*str=='\n') return TRUE;
		str++;
	}
	return FALSE;
}


Boolean VLogPrintf(const char *fmt, va_list argp)
{
	//if (strlen(inStr)+strlen(str)>=1000) return FALSE;	FIXME: How to check for buffer overflow?
	vsprintf(inStr, fmt, argp);
	return TRUE;
}


/* Display in the system log and maybe on stderr the message described by the second and
 following parameters. The message may contain at most one "\n". There are two cases:
 (1) It may be terminated by "\n", i.e., it may be a full line or a chunk ending a line.
 (2) If it's not terminated by "\n", it's a partial line, to be completed on a later call.
 In case #1, we send the complete line to syslog(); in case #2, just add it to a buffer.  */

Boolean LogPrintf(short priLevel, const char *fmt, ...)
{
	Boolean okay, fullLine;
	char *pch;
	
	if (priLevel>MIN_LOG_LEVEL) priLevel = LOG_NOTICE;
	
	va_list argp; 
	va_start(argp, fmt); 
	okay = VLogPrintf(fmt, argp); 
	va_end(argp);
	/* inStr now contains the results of this call. Handle both cases. */
	fullLine = HaveNewline(inStr);
	pch = strtok(inStr, "\n");
	//LogPrintf(LOG_NOTICE, "1------inStr='%s' fullLine=%d &inStr=%x pch=%x\n",
	//			inStr, fullLine, &inStr, pch);
	if (pch!='\0') strcat(outStr, pch);
	if (fullLine) {
		syslog(priLevel, outStr);
		outStr[0] = '\0';									/* Set <outStr> to empty */
	}
	
	return okay;
}


short InitLogPrintf()
{
	int logopt = 0;
	if (LOG_TO_STDERR) logopt = LOG_PERROR;
	openlog("Ngale", logopt, LOG_USER);
	
	/* LOG_DEBUG is the lowest level (=highest internal code), so the call to setlogmask
	 below _should_ cause all messages to go to the log regardless of level. But on
	 my G5 running OS 10.5.x, I rarely if ever get anything below LOG_NOTICE; I have
	 no idea why. --DAB, March 2016 */
	
	int oldLevelMask = setlogmask(LOG_UPTO(LOG_DEBUG));
	outStr[0] = '\0';										/* Set <outStr> to empty */
	
#ifdef TEST_LOGGING_FUNCS
	LogPrintf(LOG_NOTICE, "A simple one-line LogPrintf.\n");
	LogPrintf(LOG_NOTICE, "Another one-line LogPrintf.\n");
	LogPrintf(LOG_NOTICE, "This line is ");
	LogPrintf(LOG_NOTICE, "built from 2 pieces.\n");
	LogPrintf(LOG_NOTICE, "This line was ");
	LogPrintf(LOG_NOTICE, "built from 3 ");
	LogPrintf(LOG_NOTICE, "pieces.\n");
#endif
	
	LogPrintf(LOG_NOTICE, "InitLogPrintf: oldLevelMask = %d, MIN_LOG_LEVEL=%d\n", oldLevelMask, MIN_LOG_LEVEL);
	
#ifdef TEST_LOGGING_LEVELS
	SysBeep(1);
	LogPrintf(LOG_ALERT, "InitLogPrintf: LogPrintf LOG_ALERT message. Magic no. %d\n", 1729);
	LogPrintf(LOG_CRIT, "InitLogPrintf: LogPrintf LOG_CRIT message. Magic no. %d\n", 1729);
	LogPrintf(LOG_ERR, "InitLogPrintf: LogPrintf LOG_ERR message. Magic no. %d\n", 1729);
	LogPrintf(LOG_WARNING, "InitLogPrintf: LogPrintf LOG_WARNING message. Magic nos. %d, %f%d.\n", 1729, 3.14159265359);
	LogPrintf(LOG_NOTICE, "InitLogPrintf: LogPrintf LOG_NOTICE message. Magic no. %d\n", 1729);
	LogPrintf(LOG_INFO, "InitLogPrintf: LogPrintf LOG_INFO message. Magic no. %d\n", 1729);
	LogPrintf(LOG_DEBUG, "InitLogPrintf: LogPrintf LOG_DEBUG message. Magic no. %d\n\n", 1729);
#endif
	
	return oldLevelMask;
}
