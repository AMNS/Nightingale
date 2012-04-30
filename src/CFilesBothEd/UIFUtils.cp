/******************************************************************************
*	FILE:	UIFUtils.c
*	PROJ:	Nightingale, rev. for v. 3.5
*	DESC:	General-purpose utility routines for implementing the user interface.
		GetStaffLim				HiliteInsertNode		HiliteTwoNodesOn
		FixCursor				FlashRect				SamePoint
		Advise					NoteAdvise				CautionAdvise
		StopAdvise				Inform					NoteInform
		CautionInform			StopInform				ProgressMsg
		UserInterrupt			UserInterruptAndSel
		NameHeapType			NameNodeType			NameGraphicType	
		ConvertQuote			DrawBox					HiliteRect
		Voice2UserStr			Staff2UserStr
		DrawPUpTriangle		DrawPopUp				TruncPopUpString
		InitPopUp				DoUserPopUp				ChangePopUpChoice
		DisposePopUp			HilitePopUp				ResizePopUp
		ShowPopUp
/******************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static DDIST GetStaffLim(Document *doc,LINK pL,INT16 s,Boolean top,PCONTEXT pContext);
	

/* ----------------------------------------------------------------- GetStaffLim -- */
/* Return a rough top or bottom staff limit: staffTop - 6 half-lines, or staff
bottom + 6 half-lines. */

static DDIST GetStaffLim(
						Document *doc, LINK pL,
						INT16 staffn,
						Boolean top,			/* TRUE if getting top staff limit. */
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
STAFF, etc.--HiliteInsertNode does nothing. */

void HiliteInsertNode(
			Document *doc,
			LINK		pL,
			INT16		staffn,		/* No. of "special" staff to emphasize or NOONE */
			Boolean	flash 		/* TRUE=flash to emphasize special staff */
			)
{
	INT16			xd, xp, ypTop, ypBot;
	DDIST			blackTop, blackBottom;
	CONTEXT		context;

	if (!pL) return;															/* Just in case */
	
	if (objTable[ObjLType(pL)].justType!=J_STRUC) {
		if (staffn==NOONE)
			GetContext(doc, pL, 1, &context);
		else {
			blackTop = GetStaffLim(doc,pL,staffn,TRUE,&context);
			blackBottom = GetStaffLim(doc,pL,staffn,FALSE,&context);
		}
		xd = SysRelxd(pL)+context.systemLeft;						/* abs. origin of object */
		PenMode(patXor);
		PenPat(NGetQDGlobalsGray());
		xp = context.paper.left+d2p(xd);
		ypTop = context.paper.top+d2p(context.systemTop)+1;
		ypBot = context.paper.top+d2p(context.systemBottom)-1;
		MoveTo(xp+1, ypTop);												/* Draw thru whole system */
		LineTo(xp+1, ypBot);
		
		if (staffn!=NOONE) {
#ifdef NOMORE
			MoveTo(xp+1, context.paper.top+d2p(blackTop));		/* Erase on special staff */
			LineTo(xp+1, context.paper.top+d2p(blackBottom));
#endif
			PenSize(3, 1);													/* Draw thicker on special staff */
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
		PenNormal();														/* Restore normal drawing */
	}
}


/* ------------------------------------------------------------ HiliteTwoNodesOn -- */
/* Show in a sensible way insert-type hiliting on two nodes that may be identical.
Should NOT be used to erase the hiliting: for that, call HiliteInsertNode once
or twice. */

void HiliteTwoNodesOn(Document *doc, LINK node1, LINK node2, INT16 staffn)
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
			INT16 staffn)
{
	if (!firstL) return;
	
	if (lastL) HiliteTwoNodesOn(doc, firstL, lastL, staffn);							/* On */
	else		  HiliteInsertNode(doc, firstL, staffn, TRUE);							/* On */

	SleepTicks(HILITE_TICKS);
	while (Button()) ;
	
	HiliteInsertNode(doc, firstL, staffn, FALSE);										/* Off */
	if (lastL && firstL!=lastL) HiliteInsertNode(doc, lastL, staffn, FALSE);	/* Off */
}

static void logPrint(char *str) 
{
	//printf(str);
}

static void isWhichPalette(WindowPtr wp)
{
	WindowPtr palPtr = (*paletteGlobals[TOOL_PALETTE])->paletteWindow;
	if (wp==palPtr) {
		logPrint("Is Tool Palette\n"); return;
	}
	WindowPtr helpPtr = (*paletteGlobals[HELP_PALETTE])->paletteWindow;
	if (wp==palPtr) {
		logPrint("Is Help Palette\n"); return;
	}
	WindowPtr clavPtr = (*paletteGlobals[CLAVIER_PALETTE])->paletteWindow;
	if (wp==palPtr) {
		logPrint("Is Clavier Palette\n"); return;
	}
		
	logPrint("Is ???? Palette\n");
}

static void logPalette(WindowPtr wp)
{
	logPrint("2. In palette\n"); 
	isWhichPalette(wp); 
}

/* -------------------------------------------------------------------- FixCursor -- */
/*	Set cursor shape based on window and mouse position. This version allows "shaking
off the mode".	*/

#define MAXSHAKES		4		/* Changes in X direction before a good shake */
#define CHECKMIN		2		/* Number ticks before next dx check */
#define SWAPMIN		30		/* Number ticks between consecutive shakeoffs */

void FixCursor()
{
	Point				mousept, globalpt;
	WindowPtr		wp;
	CursHandle		newCursor;
	Document			*doc;
	static INT16	x,y,xOld,yOld,dx,dy,dxOld,dyOld,shaker,shookey;
	static long 	now,soon,nextcheck;
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
	
	if (TopWindow == NULL) { holdCursor = FALSE; logPrint("1. TopWindow is null\n"); ArrowCursor(); return; }
	
	/* If mouse over any palette, use arrow unless a tool was just chosen */

#if 0	
	if (GetWindowKind(TopWindow) == PALETTEKIND) {
		holdCursor = TRUE;
		wp = toolPalette->paletteWindow;
		RgnHandle strucRgn = NewRgn();
		GetWindowRegion(wp, kWindowStructureRgn, strucRgn);
		if (PtInRgn(globalpt,strucRgn)) {
			if (!holdCursor) {  logPalette(wp); ArrowCursor(); }
			DisposeRgn(strucRgn);
			return;
			}
		DisposeRgn(strucRgn);
		}		
#else
	if (GetWindowKind(TopWindow) == PALETTEKIND) {
		holdCursor = TRUE;
		for (wp=TopPalette; wp!=NULL && GetWindowKind(wp)==PALETTEKIND; wp=GetNextWindow(wp)) {
			RgnHandle strucRgn = NewRgn();
			GetWindowRegion(wp, kWindowStructureRgn, strucRgn);
			if (PtInRgn(globalpt,strucRgn)) {
				//if (!holdCursor) ArrowCursor();
				DisposeRgn(strucRgn);
				foundPalette = TRUE;
				}
				else {
				DisposeRgn(strucRgn);
				}
			}
		}
#endif

	holdCursor = FALSE;			/* OK to change cursor back to arrow when it's over a palette */
	
	/* If no Documents, use arrow */
	
	if (TopDocument == NULL) { logPrint("3. TopDocument is null\n"); ArrowCursor(); return; }

	/* If mouse over any kind of window other than a Document, use arrow */
	
	FindWindow(globalpt, (WindowPtr *)&wp);
	
	if (wp==NULL) { logPrint("3-1. FoundWindow is null\n"); ArrowCursor(); return; }
	if (GetWindowKind(wp)==PALETTEKIND) { logPrint("3-2. FoundWindow is a palette\n"); return; }	
	if (GetWindowKind(wp)!=DOCUMENTKIND) { logPrint("3-3. FoundWindow not our Document\n"); ArrowCursor(); return; }
	
	doc = GetDocumentFromWindow(wp);
	
	if (doc == NULL) { ArrowCursor(); logPrint("4. FixCursor: doc is null\n"); return; }
	sprintf(message, "4.1 found document %s\n", doc->name);
	logPrint(message);
	
	/* If mouse not over the Document's viewRect, use arrow */
	
	if (!PtInRect(mousept,&doc->viewRect)) { logPrint("5. Not in viewRect\n");  ArrowCursor(); return; }
	
	/* If Document showing Master Page or Work on Format, use arrow */
	
	if (doc->masterView || doc->showFormat) { logPrint("6. MasterPage or ShowFormat\n"); ArrowCursor(); return; }
	
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
				if (now < soon)											/* A quick-enough shake ? */
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
	
	logPrint("7. Installing current cursor\n");
	
	/* Install the new cursor, whatever it may be. */
	SetCursor(*newCursor);
}


/* ------------------------------------------------------------------- FlashRect -- */

#define FLASH_COUNT 1

void FlashRect(Rect *pRect)
{
	INT16	j;
	
	for (j=0; j<FLASH_COUNT; j++) {
		InvertRect(pRect);
		SleepTicks(20L);
		InvertRect(pRect);
		SleepTicks(20L);		
	}
}


/* ------------------------------------------------------------------- SamePoint -- */
/* Check to see if two Points (presumably on the screen) are within slop of each
other. Needed for editing beams, lines, hairpins, etc. */

Boolean SamePoint(Point p1, Point p2)
{
	INT16 dx,dy; static INT16 slop = 2;
	
	dx = p1.h - p2.h; if (dx < 0) dx = -dx;
	dy = p1.v - p2.v; if (dy < 0) dy = -dy;
	
	return ( dx<=slop && dy<=slop );
}
	

/* ------------------------------------------------------------- Advise functions -- */
/* Give alerts of varlous types and return value. */

INT16 Advise(INT16 alertID)
{
	ArrowCursor();
	return Alert(alertID, NULL);
}

INT16 NoteAdvise(INT16 alertID)
{
	ArrowCursor();
	return NoteAlert(alertID, NULL);
}

INT16 CautionAdvise(INT16 alertID)
{
	ArrowCursor();
	return CautionAlert(alertID, NULL);
}

INT16 StopAdvise(INT16 alertID)
{
	ArrowCursor();
	return StopAlert(alertID, NULL);
}


/* ------------------------------------------------------------- Inform functions -- */
/* Give valueless alerts of various types. */
							
void Inform(INT16 alertID)
{
	short dummy;
	
	ArrowCursor();
	dummy = Alert(alertID, NULL);
}
						
void NoteInform(INT16 alertID)
{
	short dummy;
	
	ArrowCursor();
	dummy = NoteAlert(alertID, NULL);
}
						
void CautionInform(INT16 alertID)
{
	short dummy;
	
	ArrowCursor();
	dummy = CautionAlert(alertID, NULL);
}

void StopInform(INT16 alertID)
{
	short dummy;
	
	ArrowCursor();
	dummy = StopAlert(alertID, NULL);
}


/* ----------------------------------------------------------------- ProgressMsg -- */

#ifdef JG_NOTELIST
#define MAX_MESSAGE_STR 19		/* including Open Notelist */
#else
#define MAX_MESSAGE_STR 15
#endif

#define MESSAGE_DI 1

/* If which!=0, put up a small window (actually a dialog) containing the specified
message, and pause for a brief time (HILITE_TICKS ticks). If which=0, remove the
window. If which!=0 and there's already a ProgressMsg window up, remove it before
putting up the new one. */

Boolean ProgressMsg(INT16 which,
							char *moreInfo)				/* C string */
{
	static INT16 lastWhich=-999;
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
keys are ignored. ??Should be internationalized! */

Boolean UserInterrupt()
{
#ifndef OS_MAC
#error MAC OS-ONLY CODE
#else
	if (!CmdKeyDown()) return FALSE;
	if (!KeyIsDown(47)) return FALSE;
	
	return TRUE;
#endif
}


/* ---------------------------------------------------------- UserInterruptAndSel -- */
/* Returns TRUE if COMMAND and SLASH (/) keys are both currently down; all other
keys are ignored. ??Should be internationalized! */

Boolean UserInterruptAndSel()
{
#ifndef OS_MAC
#error MAC OS-ONLY CODE
#else
	if (!CmdKeyDown()) return FALSE;
	if (!KeyIsDown(44)) return FALSE;
	
	return TRUE;
#endif
}


/* --------------------------------------------------- NameHeapType, NameNodeType -- */

/* Given a "heap index" or object type, return the name of the corresponding object. */

const char *NameHeapType(
			INT16 heapIndex,
			Boolean friendly)		/* TRUE=give user-friendly names, FALSE=give "real" names */
{
	const char *ps;

	switch (heapIndex) {
		case HEADERtype:	ps = "HEAD"; break;
		case TAILtype: 	ps = "TAIL"; break;
		case SYNCtype: 	ps = (friendly? "note and rest" : "SYNC"); break;
		case RPTENDtype:	ps = (friendly? "repeat bar/segno" : "REPEATEND"); break;
		case ENDINGtype:	ps = (friendly? "ending" : "ENDING"); break;
		case PAGEtype:		ps = (friendly? "page" : "PAGE"); break;
		case SYSTEMtype:	ps = (friendly? "system" : "SYSTEM"); break;
		case STAFFtype:	ps = (friendly? "staff" : "STAFF"); break;
		case MEASUREtype:	ps = (friendly? "measure" : "MEASURE"); break;
		case PSMEAStype:	ps = (friendly? "pseudomeasure" : "PSEUDOMEAS"); break;
		case CLEFtype:		ps = (friendly? "clef" : "CLEF"); break;
		case KEYSIGtype:	ps = (friendly? "key signature" : "KEYSIG"); break;
		case TIMESIGtype:	ps = (friendly? "time signature" : "TIMESIG"); break;
		case BEAMSETtype:	ps = (friendly? "beamed group" : "BEAMSET"); break;
		case TUPLETtype:	ps = (friendly? "tuplet" : "TUPLET"); break;
		case CONNECTtype:	ps = (friendly? "staff bracket" : "CONNECT"); break;
		case DYNAMtype:	ps = (friendly? "dynamic" : "DYNAMIC"); break;
		case MODNRtype:	ps = (friendly? "note modifier" : "MODNR"); break;
		case GRAPHICtype:	ps = (friendly? "Graphic" : "GRAPHIC"); break;
		case OCTAVAtype:	ps = (friendly? "octave sign" : "OCTAVE"); break;
		case SLURtype:		ps = (friendly? "slur and tie" : "SLUR"); break;
		case GRSYNCtype:	ps = (friendly? "grace note" : "GRSYNC"); break;
		case TEMPOtype:	ps = (friendly? "tempo mark" : "TEMPO"); break;
		case SPACEtype:	ps = (friendly? "spacer" : "SPACER"); break;
		case OBJtype:		ps = "OBJECT"; break;
		default:				ps = "**UNKNOWN**";
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

const char *NameGraphicType(LINK pL)
{
	char *ps;

	if (!GraphicTYPE(pL)) {														/* Just in case */
		ps = "*NOT A GRAPHIC*";
		return ps;
	}

	switch (GraphicSubType(pL)) {
		case GRPICT:			ps = "PICT"; break;
		case GRString:			ps = "TEXT"; break;
		case GRLyric:			ps = "LYRIC"; break;
		case GRDraw:			ps = "DRAW"; break;
		case GRRehearsal:		ps = "REHEARSAL"; break;
		case GRChordSym:		ps = "CHORDSYM"; break;
		case GRArpeggio:		ps = "ARPEGGIO"; break;
		case GRChordFrame:		ps = "CHORDFRAME"; break;
		case GRMIDIPatch:		ps = "MIDIPATCH"; break;
		case GRMIDISustainOn:	ps = "SUSTAINON"; break;
		case GRMIDISustainOff:	ps = "SUSTAINOFF"; break;
		case GRMIDIPan:			ps = "PAN"; break;
		default:				ps = "*UNKNOWN*";
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

INT16 ConvertQuote(TEHandle textH, INT16 ch)
	{
		short n; unsigned char prev;

		if (textH)
			if (ch=='"' || ch=='\'') {
				n = (*textH)->selStart;
				prev = (n > 0) ? ((unsigned char) *(*((*textH)->hText) + n-1)) : 0;
				if (prev=='\r' || prev==' ' || prev=='Ê' || prev=='\t' || n==0)
					ch = (ch=='"' ? 'Ò' : 'Ô');
				 else
					ch = (ch=='"' ? 'Ó' : 'Õ');
				}
		return(ch);
	}


/* --------------------------------------------------------------------- DrawBox -- */
/* Use a wide line to draw a filled box centered at the given point.  Size should
normally be 4 so that the special drag cursor fits in with it. Intended to draw
handles for dragging.*/

void DrawBox(Point pt, INT16 size)
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
??Based on code in HasSmthgAcross: that and similar code in 3-4 places should
be replaced with calls to this. */

void Voice2UserStr(Document *doc,
								INT16 voice,
								char str[])			/* user-friendly string describing the voice */
{
	INT16 userVoice;
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
??Based on code in StfHasSmthgAcross: that and similar code in 3-4 places should
be replaced with calls to this. */

void Staff2UserStr(Document *doc,
								INT16 staffn,
								char str[])			/* user-friendly string describing the staff */
{
	INT16 relStaff;
	LINK partL; PPARTINFO pPart;
	char fmtStr[256]; INT16 len;

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
			width -= CharWidth('É');
			while (len>0 && width>space)
				width -= CharWidth(p->str[len--]);
			p->str[++len] = 'É';
			*p->str = len;
			}
	}


/* Initialise a UserPopUp data structure; return FALSE if error. If firstChoice=0,
no item is initially chosen. */

INT16 InitPopUp(
			DialogPtr dlog,
			UserPopUp *p,
			INT16 item,				/* Dialog item to set p->box from */
			INT16 pItem,			/* Dialog item to set p->prompt from, or 0=set it empty */
			INT16 menuID,
			INT16 firstChoice		/* Initial choice, or 0=none */
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

INT16 DoUserPopUp(UserPopUp *p)
	{
		long choice; Point pt; INT16 ans = FALSE;

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
				ChangePopUpChoice(p,(INT16)choice);
				ans = TRUE;
				}
			}
		return(ans);
	}


/* Change a given popup to a given choice, regardless of the current popup choice.
This means unchecking the old choice, if any, and checking the new one, resetting
the display string, and redisplaying. */

void ChangePopUpChoice(UserPopUp *p, INT16 choice)
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

void HilitePopUp(UserPopUp *p, INT16 activ)
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
 
INT16 ResizePopUp(UserPopUp *p)
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

void ShowPopUp(UserPopUp *p, INT16 vis)
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

void HiliteArrowKey(DialogPtr /*dlog*/, INT16 whichKey, UserPopUp *pPopup,
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
	OffsetRect(&contrlRect,dx,dy);
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


