/* File Browser.c - object list browser functions */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2020 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

//#ifndef PUBLIC_VERSION		/* If public, skip this file completely! */

#define BROWSER_DLOG 1900
#define CONTEXT_DLOG 1901
#define LEADING 11				/* Vertical space between lines displayed (pixels) */

static Rect bRect, paperRect;
static DRect systemRect;
static short linenum, showBPage;
static char s[300];				/* A bit more than 256 to protect against 255-char. Graphics, etc. */
static char dynStr[40];			/* for dynamics only */
static char clefStr[40];		/* for clefs only */
static char objList[16];
static LINK subL;

static void DrawTextLine(char *);
static void InvertObjRect(Document *, Rect *);
static void ChangeSelectObj(Document *, LINK, short, short, Boolean, Rect *);
static void ShowObject(Document *, LINK, short, Rect *);
static void ShowVoicePage(Document *, short);
static void BrowseHeader(Document *, LINK, short, Rect *);
static void BrowsePage(LINK, Rect *);
static void BrowseSystem(LINK, Rect *);
static void BrowseStaff(LINK, short, Rect *);
static void BrowseConnect(LINK, short, Rect *);
static void BrowseClef(LINK, short, Rect *);
static void BrowseKeySig(LINK, short, Rect *);
static void BrowseTimeSig(LINK, short, Rect *);
static void BrowseMeasure(LINK, short, Rect *);
static void BrowsePseudoMeas(LINK, short, Rect *);
static void BrowseSync(LINK, short, Rect *);
static void BrowseGRSync(LINK, short, Rect *);
static void BrowseBeamset(LINK, short, Rect *);
static void BrowseTuplet(LINK, short, Rect *);
static void BrowseOttava(LINK, short, Rect *);
static void BrowseDynamic(LINK, short, Rect *);
static void BrowseRptEnd(LINK, short, Rect *);
static void BrowseEnding(LINK, short, Rect *);
static void BrowseGraphic(LINK, Rect *);
static void BrowseTempo(LINK, Rect *);
static void BrowseSpace(LINK, Rect *);
static void BrowseSlur(LINK, short, Rect *);
static pascal Boolean BrowserFilter(DialogPtr, EventRecord *, short *);


enum {
	iOK = 1,
	iText,
	iHead,
	iLeftJump,
	iRightJump,
	iTail,
	iUp,
	iDown,
	iDeselect,
	iSelect,
	iScrollLeft,
	iScrollRight,
	iScrollDown,
	iScrollUp,
	iLeft,
	iRight,
	iPicture,
	iGoNum,
	iGo,
	iShowMore		/* Fake item (not in the DITL) */
};

pascal Boolean BrowserFilter(DialogPtr theDialog, EventRecord *theEvt, short *item)
{
		Boolean ans = False;
		short itype;  Handle tHdl;
		Rect textRect;  Point where;
		WindowPtr w = (WindowPtr)theEvt->message;
		int part;
		
		switch (theEvt->what) {
			case updateEvt:
				if (w == GetDialogWindow(theDialog)) {
					BeginUpdate(GetDialogWindow(theDialog));
					DrawDialog(theDialog);
//					OutlineOKButton(theDialog, True);
					EndUpdate(GetDialogWindow(theDialog));
				}
				*item = 0;
				ans = True;
				break;
			case mouseDown:
				where = theEvt->where;
				GlobalToLocal(&where);
				GetDialogItem(theDialog, iText, &itype, &tHdl, &textRect);
				if (PtInRect(where, &textRect)) {
					*item = iShowMore;
					ans = True;
					break;
				}
				else {
					Rect bounds = GetQDScreenBitsBounds();
					part = FindWindow(theEvt->where, &w);
					switch (part) {
						case inDrag:
							if (w == GetDialogWindow(theDialog)) {
								DragWindow(w, theEvt->where, &bounds);
							}
							break;
					}
				}

			case keyDown:
			case autoKey:
				if (DlgCmdKey(theDialog, theEvt, item, False)) return True;
				break;
			}
		return ans;
	}


/* --------------------------------------------------------------------------- Browser -- */
/* Browser displays and handles interaction with a small window that lets the user
(a Nightingale programmer, presumably) prowl around in an object list (not necessarily
the main object list of a score; it could also be the Master Page or Undo obj list).

If headL is anything other than doc->headL, the "Go to" button will be inoperative; if
tailL is NILINK, the tiny "go to tail" button will be inoperative. In either case,
everything else will work as usual. */

void Browser(Document *doc, LINK headL, LINK tailL)
{
	DialogPtr dlog;
	short itype, ditem, type;
	ControlHandle selHdl, deselHdl, tHdl;
	Rect tRect;
	short index, oldIndex, oldShowBPage, goLoc;
	Boolean done;
	Rect objRect;
	GrafPtr oldPort;
	LINK pL, oldL, oldpL;
	ControlHandle scrollHdl;
	short part=0;
	Document *saveDoc;
	ModalFilterUPP	filterUPP;

	if (!headL) {
		LogPrintf(LOG_WARNING, "Object list is nonexistent.  (Browser)\n");
		return;
	}

/* --- 1. Create the dialog and initialize its contents. --- */
	
	filterUPP = NewModalFilterUPP(BrowserFilter);
	if (filterUPP == NULL) {
		MissingDialog(BROWSER_DLOG);
		return;
	}	
	saveDoc = currentDoc;
	InstallDoc(doc);
	GetPort(&oldPort);
	dlog = GetNewDialog(BROWSER_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(BROWSER_DLOG);
		return;
	}
	SetPort(GetDialogWindowPort(dlog));
	
	SetDialogDefaultItem(dlog, OK);
	
	GetDialogItem(dlog, iText, &itype, (Handle *)&tHdl, &bRect);
	GetDialogItem(dlog, iSelect, &itype, (Handle *)&selHdl, &tRect);
	GetDialogItem(dlog, iDeselect, &itype, (Handle *)&deselHdl, &tRect);
	
	/* Initialize for the object list being browsed */
	
	if (headL==doc->headL) pL = doc->selStartL;
	else pL = headL;
	
	if (headL==doc->headL) strcpy(objList, "");
	else if (headL==doc->masterHeadL) strcpy(objList, "Master Page ");
	else if (headL==doc->undo.headL) strcpy(objList, "Undo ");
	
	/* We support the "Go" button only if headL is the head of the main object list. */
	
	if (headL!=doc->headL) {
		GetDialogItem(dlog, iGo, &itype, (Handle *)&tHdl, &tRect);
		HiliteControl(tHdl, CTL_INACTIVE);
	}
	else PutDlgWord(dlog, iGoNum, pL, True);

	ArrowCursor();

/*--- 2. Interact with user til they push OK or Cancel. --- */

	SetRect(&objRect, 0, 0, 0, 0);
	
	index = 0;
	showBPage = 0;

	oldpL = NILINK;
	oldIndex = -1;
	oldShowBPage = -1;

	done = False;
	ShowWindow(GetDialogWindow(dlog));
	
	do {
		/* If the desired thing has changed or, for Header, may have changed... */
		
		if (pL!=oldpL || index!=oldIndex
		|| (ObjLType(pL)==HEADERtype && index<0 && showBPage!=oldShowBPage)) {
			EraseRect(&bRect);
			
			InvertObjRect(doc, &objRect);				/* Restore previous objRect */
			ShowObject(doc, pL, index, &objRect); 		/* Write out info and get new objRect */
			InvertObjRect(doc, &objRect);				/* Hilite this objRect */
			
			if (ObjLType(pL)==HEADERtype
			||  ObjLType(pL)==TAILtype) {				/* Case iSelect & iDeselect don't handle these */
				HiliteControl(selHdl, CTL_INACTIVE);
				HiliteControl(deselHdl, CTL_INACTIVE);
			}
			else {
				HiliteControl(selHdl, CTL_ACTIVE);
				HiliteControl(deselHdl, CTL_ACTIVE);
			}
		}
		oldpL = pL; oldIndex = index; oldShowBPage = showBPage;
		ModalDialog(filterUPP, &ditem);					/* Handle dialog events */
		scrollHdl = NULL;
		switch (ditem) {
			case iOK:
			  	done = True;
				index = 0;
			  	break;
			case iHead:
				/* Go back to start of object list */
				
			  	pL = headL;
				index = 0;
			  	break;
			case iLeftJump:
				if (pL == tailL) {
					if (LeftLINK(tailL)) pL = LeftLINK(tailL);
				}
				 else {
					/* Search back for previous or (with mod key) 1st obj of same type as pL */
					
					type = ObjLType(pL);
					oldL = pL;
					while (LeftLINK(pL)) {
						pL = LeftLINK(pL);
						if (ObjLType(pL) == type) {
							oldL = pL;
							if (!OptionKeyDown()) break;
						}
					}
					pL = oldL;
				}
				index = 0;
				break;
			case iLeft:
				/* Go to previous object regardless of type */
				
				if (LeftLINK(pL)) pL = LeftLINK(pL);
				index = 0;
				break;
			case iTail:
				/* If tailL is NILINK, it's not a disaster, but ignore user's action */
				
				if (tailL==NILINK) {
					SysBeep(1);
					LogPrintf(LOG_WARNING, "tailL is NILINK. (Browser)\n");
				}
				else pL = tailL;
				index = 0;
				break;
			case iRightJump:
				if (pL == headL) {
					if (RightLINK(headL)) pL = RightLINK(headL);
				}
				 else {
					/* Search ahead for next or (with mod key) last object of same type as pL */
					
					type = ObjLType(pL);
					oldL = pL;
					while (RightLINK(pL)) {
						pL = RightLINK(pL);
						if (ObjLType(pL) == type) {
							oldL = pL;
							if (!OptionKeyDown()) break;
						}
					}
					pL = oldL;
				}
				index = 0;
				break;
			case iRight:
				if (RightLINK(pL)) pL = RightLINK(pL);
				index = 0;
				break;
			case iUp:
				switch (ObjLType(pL)) {
					case HEADERtype:
						if (OptionKeyDown())
							index = 0;
						else
							if (index > -1) index--;	/* BrowseHeader uses -1 for voice table */
						break;
					case STAFFtype:
					case CLEFtype:
					case TIMESIGtype:
					case KEYSIGtype:
					case MEASUREtype:
					case PSMEAStype:
					case SYNCtype:
					case GRSYNCtype:
					case CONNECTtype:
					case BEAMSETtype:
					case TUPLETtype:
					case OTTAVAtype:
					case RPTENDtype:
					case SLURtype:
						if (OptionKeyDown())
							index = 0;
						else
							if (index > 0) index--;
						break;
					default:
						;
				}
				break;
			case iDown:
				switch (ObjLType(pL)) {
					case HEADERtype:
					case STAFFtype:
					case CLEFtype:
					case TIMESIGtype:
					case KEYSIGtype:
					case MEASUREtype:
					case PSMEAStype:
					case SYNCtype:
					case GRSYNCtype:
					case CONNECTtype:
					case BEAMSETtype:
					case TUPLETtype:
					case OTTAVAtype:
					case RPTENDtype:
					case SLURtype:
						if (OptionKeyDown())
								index = LinkNENTRIES(pL)-1;
						else
							if (index+1 < LinkNENTRIES(pL)) index++;
						break;
					default:
						;
				}
				break;
			case iSelect:
				/* If shift key down, select only current subobject, if there is one */
				
				if (headL==doc->headL)
			 		ChangeSelectObj(doc, pL, index, SMSelect, ShiftKeyDown(), &objRect);
			   break;
			case iDeselect:
			 	if (headL==doc->headL)
					ChangeSelectObj(doc, pL, index, SMDeselect, False, &objRect);
			 	break;
			case iScrollUp:
			 	scrollHdl = doc->vScroll; part = kControlPageUpPart;
			 	break;
			case iScrollDown:
			 	scrollHdl = doc->vScroll; part = kControlPageDownPart;
			 	break;
			case iScrollRight:
			 	scrollHdl = doc->hScroll; part = kControlPageUpPart;
			   break;
			case iScrollLeft:
			 	scrollHdl = doc->hScroll; part = kControlPageDownPart;
			 	break;
			case iGo:
				SelectDialogItemText(dlog, iGoNum, 0, ENDTEXT); 		/* hilite number */
			 	if (GetDlgWord(dlog, iGoNum, &goLoc))
					if (InObjectList(doc, (LINK)goLoc, MAIN_DSTR)) {
						pL = (LINK)goLoc;
						index = 0;
						break;
					}
				SysBeep(1);
				LogPrintf(LOG_WARNING, "Tried to go to a nonexistent link. (Browser)\n");
				break;
			case iShowMore:
				showBPage++;
				break;			   
			 default:
			 	;
		}
		if (scrollHdl) {
			Rect box, portRect;
			WindowPtr w; 
			Point oldOrigin;
			short dx, dy;
			
			InvertObjRect(doc, &objRect);
			
			w = doc->theWindow;
			GetWindowRgnBounds(doc->theWindow, kWindowStructureRgn, &box);
			GetPort(&oldPort); 
			SetPort(GetDocWindowPort(doc));
			GlobalToLocal(&TOP_LEFT(box));
			GlobalToLocal(&BOT_RIGHT(box));
			oldOrigin = doc->origin;
			GetWindowPortBounds(w, &portRect);
			ClipRect(&portRect);
			if (part>0) ScrollDocument(scrollHdl, part);
			GetWindowPortBounds(w, &portRect);
			ClipRect(&portRect);
			DrawControls(w);
			ClipRect(&doc->viewRect);
			dx = oldOrigin.h - doc->origin.h;
			dy = oldOrigin.v - doc->origin.v;
			if (dx!=0 || dy!=0) {
				OffsetRect(&box, dx, dy);
				InvalWindowRect(w, &box);
				DoUpdate(doc->theWindow);
			}
			SetPort(oldPort);
			
			InvertObjRect(doc, &objRect);
			ArrowCursor();
		}
		
	} while (!done);
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	InstallDoc(saveDoc);
	InvertRect(&objRect);					/* Restore appearance of last-displayed objRect */
	SetRect(&objRect, 0, 0, 0, 0);
	return; 
}


/* ---------------------------------------------------------------------- DrawTextLine -- */
/* Draw the specified C string on the next line in Browser dialog. */

void DrawTextLine(char *str)
{
	MoveTo(bRect.left, bRect.top+linenum*LEADING);
	DrawCString(str);
	++linenum;
}


/* --------------------------------------------------------------------- InvertObjRect -- */

static void InvertObjRect(Document *doc, Rect *pObjRect)
{
	GrafPtr oldPort;
	
	GetPort(&oldPort);
	SetPort(GetDocWindowPort(doc));
	InvertRect(pObjRect);
	SetPort(oldPort);
}


/* ------------------------------------------------------------------------- SelSubObj -- */

static void SelSubObj(LINK, LINK);
static void SelSubObj(LINK pL, LINK aSubL)
{
	if (pL && aSubL)
		switch (ObjLType(pL)) {
			case CONNECTtype:
				ConnectSEL(aSubL) = True;  break;
			case STAFFtype:
				StaffSEL(aSubL) = True;  break;
			case SYNCtype:
				NoteSEL(aSubL) = True;  break;
			case CLEFtype:
				ClefSEL(aSubL) = True;  break;
			case KEYSIGtype:
				KeySigSEL(aSubL) = True;  break;
			case TIMESIGtype:
				TimeSigSEL(aSubL) = True;  break;
			case DYNAMtype:
				DynamicSEL(aSubL) = True;  break;
			case MEASUREtype:
				MeasureSEL(aSubL) = True;  break;
			case SLURtype:
				SlurSEL(aSubL) = True;  break;
			default:
				SysBeep(1);
				LogPrintf(LOG_WARNING, "Can't select subobject of this type. (SelSubObj)\n");
		}	
}

/* ------------------------------------------------------------------- ChangeSelectObj -- */
/* If mode is SMSelect, select pL and (if it has any subobjects) either the current one
or all of them; if mode is SMDeselect, deselect pL and all of its subobjects, if it has
any. This function is designed to handle anomalous situations resulting (most likely)
from bugs in Nightingale, so it tries to work in the safest possible way. */

static void ChangeSelectObj(Document *doc, LINK pL,
				short	index,		/* Index of subobject currently displayed */
				short	mode,
				Boolean selSub,		/* If SMSelect, True=select only current subobject */
				Rect *pObjRect
				)
{
	CONTEXT		context[MAXSTAVES+1];
	Boolean		found;
	short		pIndex;
	STFRANGE	stfRange={0,0};
	
	GetAllContexts(doc, context, pL);
	switch (ObjLType(pL)) {
		case PAGEtype:
		case SYSTEMtype:
			break;
		case CONNECTtype:
		case STAFFtype:
		case MEASUREtype:
			if (mode==SMSelect) {
				if (selSub) SelSubObj(pL, subL);
				else		SelAllSubObjs(pL);
			}
			else
				DeselectNode(pL);
			break;
		case SYNCtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case DYNAMtype:
		case SLURtype:
			if (mode==SMSelect && selSub)
				SelSubObj(pL, subL);
			else
				CheckObject(doc, pL, &found, NULL, context, mode, &pIndex, stfRange);
			break;
		case RPTENDtype:
		case PSMEAStype:
		case BEAMSETtype:
		case TUPLETtype:
		case GRAPHICtype:
		case TEMPOtype:
		case SPACERtype:
		case ENDINGtype:
		case OTTAVAtype:
			CheckObject(doc, pL, &found, NULL, context, mode, &pIndex, stfRange);
			break;
		default:
			MayErrMsg("ChangeSelectObj: can't handle type %ld", (long)ObjLType(pL));
			return;
	}
	LinkSEL(pL) = (mode==SMSelect);
	UpdateSelection(doc);
	if (doc->selStartL==doc->selEndL) MEAdjustCaret(doc,True);
	EraseRect(&bRect);
	ShowObject(doc, pL, index, pObjRect);						/*	Show the thing again */
}


/* ------------------------------------------------------------------------ ShowObject -- */

void ShowObject(Document *doc, LINK pL, short index, Rect *pObjRect)
{
	PMEVENT p;
	PSYSTEM pSystem;
	const char *ps;
	Rect r;
	LINK pageL, sysL, measL;
	
	linenum = 1;
	TextFont(SYSFONTID_MONOSPACED);
	TextSize(9);

	ps = NameObjType(pL);
	sprintf(s, "%s%s [type %d]", objList, ps, ObjLType(pL));
	if (doc->selStartL==pL)
		strcat(s, "  (selStartL)");
	else if (doc->selEndL==pL)
		strcat(s, "  (selEndL)");
	DrawTextLine(s);

	sprintf(s, "here (left,right)=%u (%u,%u)", pL, LeftLINK(pL), RightLINK(pL));
	DrawTextLine(s);
	p = GetPMEVENT(pL);
	sprintf(s, "@%lx xd,yd=%d,%d", (long unsigned int)p, p->xd, p->yd);
	DrawTextLine(s);
	strcpy(s, "flags=");
	if (LinkSEL(pL))		strcat(s, "SELECTED ");
	if (LinkVIS(pL))		strcat(s, "VISIBLE ");
	if (LinkSOFT(pL))		strcat(s, "SOFT ");
	if (LinkVALID(pL))		strcat(s, "VALID ");
	if (LinkTWEAKED(pL))	strcat(s, "TWEAKED ");
	if (LinkSPAREFLAG(pL))	strcat(s, "SPARE ");
	DrawTextLine(s);
	r = LinkOBJRECT(pL);
	sprintf(s, "objRect/t l b r=%d %d %d %d", r.top, r.left, r.bottom, r.right);

	/* Search back for this object's measure, system, and page (poor man's GetContext) */
	
	measL = pL;
	while (measL!=NILINK && ObjLType(measL)!=MEASUREtype) measL = LeftLINK(measL);
	sysL = pL;
	while (sysL!=NILINK && ObjLType(sysL)!=SYSTEMtype) sysL = LeftLINK(sysL);
	if (sysL) {
		pSystem = GetPSYSTEM(sysL);
		systemRect = pSystem->systemRect;
		}
	 else
		SetDRect(&systemRect, 0, 0, 0, 0);
	
	/* Get sheet this object is on so we can mark object's bounding box while browsing */
	
	pageL = pL;
	while (pageL!=NILINK && ObjLType(pageL)!=PAGEtype) pageL = LeftLINK(pageL);
	if (pageL)
		if (GetSheetRect(doc,SheetNUM(pageL),&paperRect)!=INARRAY_INRANGE) pageL = NILINK;
		
	DrawTextLine(s);
	sprintf(s, "nEntries=%d 1stSubObj=%u", LinkNENTRIES(pL), FirstSubLINK(pL));
	DrawTextLine(s);
	DrawTextLine("----------------------------");
	
	subL = NILINK;
	switch (ObjLType(pL)) {
	case HEADERtype:
		BrowseHeader(doc, pL, index, pObjRect);
		break;
	case PAGEtype:
		BrowsePage(pL, pObjRect);
		break;
	case SYSTEMtype:
		BrowseSystem(pL, pObjRect);
		break;
	case STAFFtype:
		BrowseStaff(pL, index, pObjRect);
		break;
	case CONNECTtype:
		BrowseConnect(pL, index, pObjRect);
		break;
	case CLEFtype:
		BrowseClef(pL, index, pObjRect);
		break;
	case KEYSIGtype:
		BrowseKeySig(pL, index, pObjRect);
		break;
	case TIMESIGtype:
		BrowseTimeSig(pL, index, pObjRect);
		break;
	case MEASUREtype:
		BrowseMeasure(pL, index, pObjRect);
		break;
	case PSMEAStype:
		BrowsePseudoMeas(pL, index, pObjRect);
		break;
	case SYNCtype:
		BrowseSync(pL, index, pObjRect);
		break;
	case GRSYNCtype:
		BrowseGRSync(pL, index, pObjRect);
		break;
	case BEAMSETtype:
		BrowseBeamset(pL, index, pObjRect);
		break;
	case TUPLETtype:
		BrowseTuplet(pL, index, pObjRect);
		break;
	case OTTAVAtype:
		BrowseOttava(pL, index, pObjRect);
		break;
	case DYNAMtype:
		BrowseDynamic(pL, index, pObjRect);
		break;
	case RPTENDtype:
		BrowseRptEnd(pL, index, pObjRect);
		break;
	case ENDINGtype:
		BrowseEnding(pL, index, pObjRect);
		break;
	case GRAPHICtype:
		BrowseGraphic(pL, pObjRect);
		break;
	case TEMPOtype:
		BrowseTempo(pL, pObjRect);
		break;
	case SPACERtype:
		BrowseSpace(pL, pObjRect);
		break;
	case SLURtype:
		BrowseSlur(pL, index, pObjRect);
		break;
	default:
		;
	}
}

/* --------------------------------------------------------------------- ShowVoicePage -- */

#define VOICEPAGESIZE 25		/* Max. no. of lines (voices) that fit in Browser window */ 

static void ShowVoicePage(Document *doc, short startV)
{
	short v;
	
	sprintf(s, "The score uses %d voices.", NCountVoices(doc));
	DrawTextLine(s);
	
	if (startV<1) startV = 1;				/* Avoid negative index in loop below */
	
	for (v = startV; v<startV+VOICEPAGESIZE && v<=MAXVOICES; v++) {
		GetVoiceTableLine(doc, v, s);
		DrawTextLine(s);
	}
}


/* ---------------------------------------------------------------------- BrowseHeader -- */

void BrowseHeader(Document *doc, LINK pL, short index, Rect *pObjRect)
{
	PPARTINFO q;
	short i, v;
	LINK qL;
	char string[256];
	STRINGOFFSET hOffset, fOffset;
	StringPtr pStr;
	
	if (index<0) {
		short startV; Boolean pageEmpty;

		/* Show a "page" of the voice-mapping table. If the current page is empty or
		   it's past the end of the table, go back to the first page. */
		   
		startV = showBPage*VOICEPAGESIZE+1;
		if (startV>MAXVOICES)
			pageEmpty = True;
		else
			for (pageEmpty = True, v = startV; v<=startV+VOICEPAGESIZE && v<=MAXVOICES; v++)
				if (doc->voiceTab[v].partn!=0) pageEmpty = False;
		if (pageEmpty) {
			showBPage = 0;
			startV = showBPage*VOICEPAGESIZE+1;
		}
		ShowVoicePage(doc, startV);
	}
		
	else {
		sprintf(s, "numSheets=%d currSheet=%d 1stSheet=%d", doc->numSheets,
					doc->currentSheet, doc->firstSheet);
		DrawTextLine(s);
		sprintf(s, "1stPgNum=%d; nsys=%d currSys=%d nstf=%d", doc->firstPageNumber,
					doc->nsystems, doc->currentSystem, doc->nstaves);
		DrawTextLine(s);
		sprintf(s, "s/altsrastral=%d,%d transp=%d lastGlFont=%d", doc->srastral, 
					doc->altsrastral, doc->transposed, doc->lastGlobalFont);
		DrawTextLine(s);

		hOffset = doc->headerStrOffset;
		pStr = PCopy(hOffset);
		if (pStr[0]!=0) {
			Pstrcpy((StringPtr)string, (StringPtr)pStr);
			PToCString((StringPtr)string);
			sprintf(s, "page header='%s'", string);
			DrawTextLine(s);
		}

		fOffset = doc->footerStrOffset;
		pStr = PCopy(fOffset);
		if (pStr[0]!=0) {
			Pstrcpy((StringPtr)string, (StringPtr)pStr);
			PToCString((StringPtr)string);
			sprintf(s, "page footer='%s'", string);
			DrawTextLine(s);
		}

		sprintf(s, "paperRect/t l b r=p%d,%d,%d,%d",
			doc->paperRect.top, doc->paperRect.left,
			doc->paperRect.bottom, doc->paperRect.right);
		DrawTextLine(s);
		sprintf(s, "marginRect=pt%d,%d,%d,%d",
			doc->marginRect.top, doc->marginRect.left,
			doc->marginRect.bottom, doc->marginRect.right);
		DrawTextLine(s);
		sprintf(s, "origPaperRect=pt%d,%d,%d,%d",
			doc->origPaperRect.top, doc->origPaperRect.left,
			doc->origPaperRect.bottom, doc->origPaperRect.right);
		DrawTextLine(s);
		sprintf(s, "origin=%d,%d sheetOrigin=%d,%d",
			doc->origin.v, doc->origin.h,
			doc->sheetOrigin.v, doc->sheetOrigin.h);
		DrawTextLine(s);
		sprintf(s, "currPaper=%d,%d,%d,%d",
			doc->currentPaper.top, doc->currentPaper.left,
			doc->currentPaper.bottom, doc->currentPaper.right);
		DrawTextLine(s);
		sprintf(s, "viewRect=%d,%d,%d,%d",
			doc->viewRect.top, doc->viewRect.left,
			doc->viewRect.bottom, doc->viewRect.right);
		DrawTextLine(s);
		sprintf(s, "ledgerYSp=%d nFontRecs=%d spPercent=%d",
					doc->ledgerYSp, doc->nFontRecords, doc->spacePercent);
		DrawTextLine(s);
		sprintf(s, "indentFirst=%d yBtwnSys=%d spTab=%d mag=%d",
						doc->dIndentFirst, doc->yBetweenSys, doc->spaceTable,doc->magnify);
		DrawTextLine(s);
		sprintf(s, "comment='%s'", doc->comment);
		DrawTextLine(s);
		sprintf(s, "strPool=%lx size=%ld nfontsUsed=%d:", (long unsigned int)doc->stringPool,
						GetHandleSize((Handle)doc->stringPool), doc->nfontsUsed);
		DrawTextLine(s);
		
		/* Show at most the first 10 fonts used because of dialog space limitations. */
	
		for (i = 0; i<doc->nfontsUsed && i<=10; i++) {
			Pstrcpy((StringPtr)string, (StringPtr)(doc->fontTable[i].fontName));
			PToCString((StringPtr)string);
			sprintf(s, "  (%d) name='%s' ID=%d", i, string, doc->fontTable[i].fontID);
			DrawTextLine(s);
		}
		sprintf(s, "fmsInputDevice=%d", doc->fmsInputDevice);
		DrawTextLine(s);
		sprintf(s, "(matchType=%d name=%#s)", doc->fmsInputDestination.basic.destinationType,
					doc->fmsInputDestination.basic.name);
		DrawTextLine(s);
		
		sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
		DrawTextLine(s);
	
		if (index+1>LinkNENTRIES(pL)) return;				/* should never happen */

		for (i=0, qL=FirstSubLINK(pL); i<index; i++, qL=NextPARTINFOL(qL)) ;
	
		q = GetPPARTINFO(qL);
		sprintf(s, "link=%u @%lx next=%u", qL, (long unsigned int)q, q->next);
		DrawTextLine(s);	q = GetPPARTINFO(qL);
		sprintf(s, "firststf=%d laststf=%d velo=%d transp=%d",
			q->firstStaff, q->lastStaff,
			q->partVelocity, q->transpose);
		DrawTextLine(s);	q = GetPPARTINFO(qL);
		sprintf(s, "patch=%d chan=%d name='%s'",
			q->patchNum, q->channel, q->name);
		DrawTextLine(s);
		sprintf(s, "fmsOutputDevice=%d",
			q->fmsOutputDevice);
		DrawTextLine(s);
		sprintf(s, "(matchType=%d name=%#s)",
			q->fmsOutputDestination.basic.destinationType,
			q->fmsOutputDestination.basic.name);
		DrawTextLine(s);
	}
	
	SetRect(pObjRect, 0, 0, 0, 0);
}


/* ------------------------------------------------------------------------ BrowsePage -- */

void BrowsePage(LINK pL, Rect *pObjRect)
{
	PPAGE p;
	p = GetPPAGE(pL);
	sprintf(s, "l/r Page=%d/%d", p->lPage, p->rPage);
	DrawTextLine(s);
	p = GetPPAGE(pL);
	sprintf(s, "sheetNum=%d", p->sheetNum);
	DrawTextLine(s);
	
	*pObjRect = paperRect;
}


/* ---------------------------------------------------------------------- BrowseSystem -- */

void BrowseSystem(LINK pL, Rect *pObjRect)
{
	PSYSTEM p;
	
	p = GetPSYSTEM(pL);
	sprintf(s, "l/r System=%d/%d", p->lSystem, p->rSystem);
	DrawTextLine(s);
	p = GetPSYSTEM(pL);
	sprintf(s, "systemNum=%d", p->systemNum);
	DrawTextLine(s);
	p = GetPSYSTEM(pL);
	sprintf(s, "pageL=%d", p->pageL);
	DrawTextLine(s);
	p = GetPSYSTEM(pL);
	D2Rect(&p->systemRect, pObjRect);
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	sprintf(s, "systemRect/t l b r=d%d %d %d %d",
			p->systemRect.top,
			p->systemRect.left,
			p->systemRect.bottom,
			p->systemRect.right);
	DrawTextLine(s);
}


/* ----------------------------------------------------------------------- BrowseStaff -- */

void BrowseStaff(LINK pL, short index, Rect *pObjRect)
{
	PSTAFF p;  PASTAFF q;
	LINK qL;
	short i;
	DRect stfRect;
	char ksStr[256];

	p = GetPSTAFF(pL);
	sprintf(s, "l/r Staff=%d/%d", p->lStaff, p->rStaff);
	DrawTextLine(s);
	p = GetPSTAFF(pL);
	sprintf(s, "systemL=%d", p->systemL);
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;		/* since Master Page can have 0 staves */

	for (i=0, qL=FirstSubLINK(pL); i<index; i++, qL=NextSTAFFL(qL))
		;
	q = GetPASTAFF(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%u", qL, (long unsigned int)q, q->staffn, q->next);
	DrawTextLine(s); q = GetPASTAFF(qL);
	sprintf(s, "vis=%s sel=%s", q->visible ? "True" : "False",
									q->selected ? "True" : "False");
	DrawTextLine(s); q = GetPASTAFF(qL);
	
	stfRect.left = q->staffLeft;  stfRect.right = systemRect.right-systemRect.left;
	stfRect.top = q->staffTop;  stfRect.bottom = q->staffTop+q->staffHeight;
	DRect2ScreenRect(stfRect, systemRect, paperRect, pObjRect);

	sprintf(s, "staff top/left/right=%d %d %d",
				q->staffTop, q->staffLeft, q->staffRight);
	DrawTextLine(s); q = GetPASTAFF(qL);
	sprintf(s, "staffHeight/Lines=%d %d", q->staffHeight, q->staffLines);
	DrawTextLine(s); q = GetPASTAFF(qL);
	if (q->showLines==SHOW_ALL_LINES)
		sprintf(s, "showLines=all showLedgers=%s", q->showLedgers? "yes" : "no");
	else
		sprintf(s, "showLines=%d showLedgers=%s",
				q->showLines, q->showLedgers? "yes" : "no");
	DrawTextLine(s); q = GetPASTAFF(qL);
	sprintf(s, "fontSize=%d spaceBelow=%d", q->fontSize, q->spaceBelow);
	DrawTextLine(s); q = GetPASTAFF(qL);
	sprintf(s, "ledger/noteHead/fracBeam=%d %d %d",
				q->ledgerWidth,
				q->noteHeadWidth,
				q->fracBeamWidth);
	DrawTextLine(s); q = GetPASTAFF(qL);
	ClefToString(q->clefType, clefStr);	
	sprintf(s, "clefType=%d (%s)", q->clefType, clefStr);
	DrawTextLine(s); q = GetPASTAFF(qL);
	sprintf(s, "nKSItems=%hd", q->nKSItems);
	DrawTextLine(s); q = GetPASTAFF(qL);
	KeySigSprintf((PKSINFO)(KeySigKSITEM(qL)), ksStr);
	DrawTextLine(ksStr);
	sprintf(s, "timeSigType,n/d=%hd,%hd/%hd",
				q->timeSigType,
				q->numerator,
				q->denominator);
	DrawTextLine(s); q = GetPASTAFF(qL);
	DynamicToString(q->dynamicType, dynStr);	
	sprintf(s, "dynamicType=%hd (%s)", q->dynamicType, dynStr);
	DrawTextLine(s);
}


/* --------------------------------------------------------------------- BrowseConnect -- */

void BrowseConnect(LINK pL, short index, Rect *pObjRect)
{
	PCONNECT p;
	PACONNECT q;
	LINK qL;
	short i;

	p = GetPCONNECT(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);
	
	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextCONNECTL(qL)) 
		;
	q = GetPACONNECT(qL);
	sprintf(s, "link=%u @%lx next=%u", qL, (long unsigned int)q, q->next);
	DrawTextLine(s);	q = GetPACONNECT(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	DrawTextLine(s); 	q = GetPACONNECT(qL);
	switch (q->connLevel) {
		case SystemLevel:
			DrawTextLine("connLevel=SystemLevel");
			break;
		case GroupLevel:
			DrawTextLine("connLevel=GroupLevel");
			break;
		case PartLevel:
			DrawTextLine("connLevel=PartLevel");
			break;
		default:
			DrawTextLine("connLevel=Unknown");
			break;
	}
	DrawTextLine(s); 	q = GetPACONNECT(qL);
	switch (q->connectType) {
		case CONNECTLINE:
			DrawTextLine("connectType=CONNECTLINE");
			break;
		case CONNECTCURLY:
			DrawTextLine("connectType=CONNECTCURLY");
			break;
		case CONNECTBRACKET:
			DrawTextLine("connectType=CONNECTBRACKET");
			break;
		default:
			DrawTextLine("connectType=Unknown");
			break;
	}
	
	sprintf(s, "staffAbove=%hd staffBelow=%hd", q->staffAbove, q->staffBelow);
	DrawTextLine(s); 	q = GetPACONNECT(qL);
	sprintf(s, "firstPart=%d lastPart=%d", q->firstPart,q->lastPart);
	DrawTextLine(s); 	q = GetPACONNECT(qL);
	sprintf(s, "xd=%d", q->xd);
	DrawTextLine(s);
	
	subL = qL;
}


/* ------------------------------------------------------------------------ BrowseClef -- */

void BrowseClef(LINK pL, short index, Rect *pObjRect)
{
	PCLEF p;
	PACLEF q;
	LINK qL;
	short i;

	p = GetPCLEF(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	if (p->inMeasure) sprintf(s, "In Measure ");
	else					sprintf(s, "Not in Measure ");
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);
	
	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextCLEFL(qL)) 
		;
	q = GetPACLEF(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%u", qL, (long unsigned int)q, q->staffn, q->next);
	DrawTextLine(s); 	q = GetPACLEF(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	DrawTextLine(s);  	q = GetPACLEF(qL);
	sprintf(s, "xd=%d yd=%d", q->xd, q->yd);
	DrawTextLine(s); 	q = GetPACLEF(qL);
	ClefToString(q->subType, clefStr);	
	sprintf(s, "clefType=%d (%s) small=%d", q->subType, clefStr, q->small);

	DrawTextLine(s);
	
	subL = qL;
}


/* ---------------------------------------------------------------------- BrowseKeySig -- */

void BrowseKeySig(LINK pL, short index, Rect *pObjRect)
{
	PKEYSIG p;
	PAKEYSIG q;
	LINK qL;
	short i;
	char ksStr[256];

	p = GetPKEYSIG(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	if (p->inMeasure) sprintf(s, "In Measure ");
	else					sprintf(s, "Not in Measure ");
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextKEYSIGL(qL)) 
		;
	q = GetPAKEYSIG(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%u", qL, (long unsigned int)q, q->staffn, q->next);
	DrawTextLine(s);	q = GetPAKEYSIG(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	DrawTextLine(s); 	q = GetPAKEYSIG(qL);
	sprintf(s, "xd=%d", q->xd);
	DrawTextLine(s);	q = GetPAKEYSIG(qL);
	sprintf(s, "nKSItems=%hd subType(nNats)=%d", q->nKSItems, q->subType);
	DrawTextLine(s);
	KeySigSprintf((PKSINFO)(KeySigKSITEM(qL)), ksStr);
	DrawTextLine(ksStr);
	
	subL = qL;
}


/* --------------------------------------------------------------------- BrowseTimeSig -- */

void BrowseTimeSig(LINK pL, short index, Rect *pObjRect)
{
	PTIMESIG p;
	PATIMESIG q;
	LINK qL;
	short i;

	p = GetPTIMESIG(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	if (p->inMeasure) sprintf(s, "In Measure ");
	else					sprintf(s, "Not in Measure ");
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextTIMESIGL(qL)) 
		;
	q = GetPATIMESIG(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%u", qL, (long unsigned int)q, q->staffn, q->next);
	DrawTextLine(s);	q = GetPATIMESIG(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	DrawTextLine(s);	q = GetPATIMESIG(qL);
	sprintf(s, "xd=%d yd=%d", q->xd, q->yd);
	DrawTextLine(s);	q = GetPATIMESIG(qL);
	sprintf(s, "timeSigType,n/d=%hd,%hd/%hd",
				q->subType,
				q->numerator,
				q->denominator);
	DrawTextLine(s);
	
	subL = qL;
}


/* --------------------------------------------------------------------- BrowseMeasure -- */

void BrowseMeasure(LINK pL, short index, Rect *pObjRect)
{
	PMEASURE p;
	PAMEASURE q;
	LINK qL;
	short i, xd;
	Rect bbox;  DRect measSizeRect;

	p = GetPMEASURE(pL);
	sprintf(s, "l/r Measure=%d,%d", p->lMeasure, p->rMeasure);
	DrawTextLine(s);	p = GetPMEASURE(pL);
	sprintf(s, "systemL=%d staffL=%d", p->systemL, p->staffL);
	DrawTextLine(s);	p = GetPMEASURE(pL);
	sprintf(s, "fake=%s spPercent=%d", (p->fakeMeas ? "True" : "False"), p->spacePercent);
	DrawTextLine(s);	p = GetPMEASURE(pL);
	bbox = p->measureBBox;
	sprintf(s, "measureBBox=%d %d %d %d",
			p->measureBBox.top, p->measureBBox.left,
			p->measureBBox.bottom, p->measureBBox.right);
	DrawTextLine(s);
	sprintf(s, "lTimeStamp=%ld", p->lTimeStamp);
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);
	xd = p->xd;

	if (index+1>LinkNENTRIES(pL)) return;				/* should never happen */

	for (i=0, qL=FirstSubLINK(pL); i<index; i++, qL=NextMEASUREL(qL)) 
		;
	q = GetPAMEASURE(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%u", qL, (long unsigned int)q, q->staffn, q->next);
	DrawTextLine(s);	q = GetPAMEASURE(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	if (q->measureVisible) strcat(s, "MEASUREVIS ");
	DrawTextLine(s);	q = GetPAMEASURE(qL);
	measSizeRect = q->measSizeRect;
	sprintf(s, "measSizeRect=%d %d %d %d ",
			q->measSizeRect.top, q->measSizeRect.left,
			q->measSizeRect.bottom, q->measSizeRect.right);
	DrawTextLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "measureNum=%hd xMNSOff=%hd yMNSOff=%hd", q->measureNum,
				q->xMNStdOffset, q->yMNStdOffset);
	DrawTextLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "barlineType=%hd connStaff=%hd", q->subType, q->connStaff);
	if (q->connAbove) strcat(s, " CONNABOVE");
	DrawTextLine(s);	q = GetPAMEASURE(qL);
	ClefToString(q->clefType, clefStr);	
	sprintf(s, "clefType=%d (%s)", q->clefType, clefStr);
	DrawTextLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "nKSItems=%hd", q->nKSItems);
//LogPrintf(LOG_DEBUG, "BrowseMeasure1: link=%u nKSItems=%hd &q=%d &q->KSItem=%d\n", qL, q->nKSItems,
//&q, &q->KSItem);
	DrawTextLine(s);	q = GetPAMEASURE(qL); 
	KeySigSprintf((PKSINFO)(MeasKSITEM(qL)), s);
	//KeySigSprintf((PKSINFO)(&q->KSItem), s);
	DrawTextLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "timeSigType,n/d=%hd,%hd/%hd",
			q->timeSigType,
			q->numerator,
			q->denominator);
	DrawTextLine(s);	q = GetPAMEASURE(qL);
	DynamicToString(q->dynamicType, dynStr);	
	sprintf(s, "dynamicType=%hd (%s)", q->dynamicType, dynStr);
	DrawTextLine(s);
	
	/* measSizeRect really just gives the width and height of the Measure; its top and
	   left should always be 0. So we convert is horizontal coords. to System-relative,
	   then to screen coordinates. */
	
	OffsetDRect(&measSizeRect, xd, 0);
	DRect2ScreenRect(measSizeRect, systemRect, paperRect, pObjRect);
	
	subL = qL;
}


/* ------------------------------------------------------------------ BrowsePseudoMeas -- */

void BrowsePseudoMeas(LINK pL, short index, Rect * /* pObjRect */)
{
	PPSMEAS p;
	PAPSMEAS q;
	LINK qL;
	short i, xd;

	/* For now, ignore the pObjRect parameter and don't change hiliting. */
	
	p = GetPPSMEAS(pL);
	xd = p->xd;
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;				/* should never happen */

	for (i=0, qL=FirstSubLINK(pL); i<index; i++, qL=NextPSMEASL(qL)) 
		;
	q = GetPAPSMEAS(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%u", qL, (long unsigned int)q, q->staffn, q->next);
	DrawTextLine(s);	q = GetPAPSMEAS(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	DrawTextLine(s);	q = GetPAPSMEAS(qL);
	sprintf(s, "barlineType=%hd connStaff=%hd", q->subType, q->connStaff);
	if (q->connAbove) strcat(s, " CONNABOVE");
	DrawTextLine(s);
}


/* ------------------------------------------------------------------------ BrowseSync -- */

void BrowseSync(LINK pL, short index, Rect *pObjRect)
{
	PSYNC p;
	PANOTE q;
	LINK qL;
	short i;

	p = GetPSYNC(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "timeStamp=%u (onset time=%ld)", SyncTIME(pL), SyncAbsTime(pL));
	
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;				/* should never happen */

	for (i=0, qL=FirstSubLINK(pL); i<index; i++, qL=NextNOTEL(qL)) 
		;
	q = GetPANOTE(qL);
	sprintf(s, "link=%u @%lx stf=%d iv=%hd next=%u", qL, (long unsigned int)q, q->staffn,
				  q->voice, q->next);
	DrawTextLine(s);	q = GetPANOTE(qL);
	
	strcpy(s, "flags=");
	if (q->selected)		strcat(s, "SELECTED ");
	if (q->visible)			strcat(s, "VISIBLE ");
	if (q->soft)			strcat(s, "SOFT ");
	if (q->inChord)			strcat(s, "INCHORD");
	DrawTextLine(s);	q = GetPANOTE(qL);
	
	strcpy(s, "flags=");
	if (q->rest)			strcat(s, "REST ");
	if (q->beamed)			strcat(s, "BEAMED ");
	if (q->otherStemSide)	strcat(s, "OTHERSTEMSIDE");
	DrawTextLine(s);	q = GetPANOTE(qL);
	
	strcpy(s, "flags=");
	if (q->tiedL)			strcat(s, "TIEDL ");
	if (q->tiedR)			strcat(s, "TIEDR ");
	if (q->slurredL)		strcat(s, "SLURREDL ");
	if (q->slurredR)		strcat(s, "SLURREDR");
	DrawTextLine(s);	q = GetPANOTE(qL);

	strcpy(s, "flags=");
	if (q->inTuplet)		strcat(s, "INTUPLET ");
	if (q->inOttava)		strcat(s, "INOTTAVA ");
	if (q->playAsCue)		strcat(s, "PLAY-AS-CUE ");
	if (q->small)			strcat(s, "SMALL ");
	if (q->tempFlag)		strcat(s, "TEMPFLAG");
	DrawTextLine(s);	q = GetPANOTE(qL);
	
	sprintf(s, "xd=%d yd=%d yqpit=%hd ystem=%d", q->xd, q->yd, q->yqpit, q->ystem);
	DrawTextLine(s);	q = GetPANOTE(qL);
	sprintf(s, "l_dur=%hd ndots=%hd xMoveDots=%hd yMoveDots=%hd", q->subType, q->ndots,
					q->xMoveDots, q->yMoveDots);
	DrawTextLine(s);	q = GetPANOTE(qL);
	sprintf(s, "headShape=%d firstMod=%d", q->headShape, q->firstMod);
	DrawTextLine(s);	q = GetPANOTE(qL);
	sprintf(s, "accident=%hd accSoft=%s xmoveAcc=%hd", q->accident,
														q->accSoft ? "True" : "False",
														q->xmoveAcc);
	DrawTextLine(s);	q = GetPANOTE(qL);
	sprintf(s, "playTDelta=%d playDur=%d noteNum=%hd", q->playTimeDelta, q->playDur,
														q->noteNum);
	DrawTextLine(s);	q = GetPANOTE(qL);
	sprintf(s, "on,offVelocity=%hd,%hd", q->onVelocity, q->offVelocity);

	DrawTextLine(s);	q = GetPANOTE(qL);
	sprintf(s, "nhSegment=%hd,%hd,%hd,%hd", q->nhSegment[0], q->nhSegment[1],
			q->nhSegment[2], q->nhSegment[3]);

	DrawTextLine(s);
	
	subL = qL;
}


/* ---------------------------------------------------------------------- BrowseGRSync -- */

void BrowseGRSync(LINK pL, short index, Rect *pObjRect)
{
	PGRSYNC p;
	PAGRNOTE q;
	LINK qL;
	short i;

	p = GetPGRSYNC(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;				/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextGRNOTEL(qL)) 
		;
	q = GetPAGRNOTE(qL);
	sprintf(s, "link=%u @%lx stf=%d iv=%hd next=%u", qL, (long unsigned int)q, q->staffn,
				q->voice, q->next);
	DrawTextLine(s);	q = GetPAGRNOTE(qL);

	
	strcpy(s, "flags=");
	if (q->selected)		strcat(s, "SELECTED ");
	if (q->visible)			strcat(s, "VISIBLE ");
	if (q->soft)			strcat(s, "SOFT ");
	if (q->inChord)			strcat(s, "INCHORD");
	DrawTextLine(s);	q = GetPAGRNOTE(qL);
	
	strcpy(s, "flags=");
	if (q->rest)			strcat(s, "REST ");
	if (q->beamed)			strcat(s, "BEAMED ");
	if (q->otherStemSide)	strcat(s, "OTHERSTEMSIDE");
	DrawTextLine(s);	q = GetPAGRNOTE(qL);
	
	strcpy(s, "flags=");
	if (q->tiedL)			strcat(s, "TIEDL ");
	if (q->tiedR)			strcat(s, "TIEDR ");
	if (q->slurredL)		strcat(s, "SLURREDL ");
	if (q->slurredR)		strcat(s, "SLURREDR");
	DrawTextLine(s);	q = GetPAGRNOTE(qL);

	strcpy(s, "flags=");
	if (q->inTuplet)		strcat(s, "INTUPLET ");
	if (q->inOttava)		strcat(s, "INOTTAVA ");
	if (q->small)			strcat(s, "SMALL");
	DrawTextLine(s);	q = GetPAGRNOTE(qL);
	
	sprintf(s, "xd=%d yd=%d yqpit=%hd ystem=%d", q->xd, q->yd, q->yqpit, q->ystem);
	DrawTextLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "l_dur=%hd ndots=%hd xMoveDots=%hd yMoveDots=%hd", q->subType, q->ndots,
					q->xMoveDots, q->yMoveDots);
	DrawTextLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "headShape=%d firstMod=%d", q->headShape, q->firstMod);
	DrawTextLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "accident=%hd accSoft=%s xmoveAcc=%hd", q->accident,
														q->accSoft ? "True" : "False",
														q->xmoveAcc);
	DrawTextLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "playTDelta=%d playDur=%d noteNum=%hd", q->playTimeDelta, q->playDur,
														q->noteNum);
	DrawTextLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "on,offVelocity=%hd,%hd", q->onVelocity, q->offVelocity);
	DrawTextLine(s);
}


/* --------------------------------------------------------------------- BrowseBeamset -- */

void BrowseBeamset(LINK pL, short index, Rect *pObjRect)
{
	PBEAMSET	p;
	PANOTEBEAM	q;
	LINK qL;
	short i;

	p = GetPBEAMSET(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "link=%u stf=%d voice=%d", pL, p->staffn, p->voice);
	DrawTextLine(s);
	sprintf(s, "thin=%d beamRests=%d feather=%d grace=%d", p->thin, p->beamRests,
					p->feather, p->grace);
	DrawTextLine(s);
	sprintf(s, "firstSystem=%d crossStf=%d crossSys=%d",
					p->firstSystem, p->crossStaff, p->crossSystem);
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;				/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextNOTEBEAML(qL)) 
		;
	q = GetPANOTEBEAM(qL);
	sprintf(s, "link=%u @%lx bpSync=%d next=%u", qL, (long unsigned int)q, q->bpSync, q->next);
	DrawTextLine(s);	q = GetPANOTEBEAM(qL);
	sprintf(s, "startend=%d", q->startend);
	DrawTextLine(s);	q = GetPANOTEBEAM(qL);
	sprintf(s, "fracs=%d", q->fracs);
	DrawTextLine(s);	q = GetPANOTEBEAM(qL);
	sprintf(s, "fracGoLeft=%s", q->fracGoLeft ? "True" : "False");
	DrawTextLine(s);
}


/* ---------------------------------------------------------------------- BrowseTuplet -- */

void BrowseTuplet(LINK pL, short index, Rect *pObjRect)
{
	PTUPLET p;
	PANOTETUPLE	q;
	LINK qL;
	short i;

	p = GetPTUPLET(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "staff=%d voice=%d num=%d denom=%d", p->staffn, p->voice,
		p->accNum, p->accDenom);
	DrawTextLine(s);	p = GetPTUPLET(pL);
	sprintf(s, "numVis=%d denomVis=%d brackVis=%d", p->numVis, p->denomVis,
		p->brackVis);
	DrawTextLine(s);	p = GetPTUPLET(pL);
	sprintf(s, "acnxd=%d acnyd=%d", p->acnxd, p->acnyd);
	DrawTextLine(s);	p = GetPTUPLET(pL);
	sprintf(s, "xd1st=%d yd1st=%d xdLast=%d ydLast=%d", p->xdFirst, p->ydFirst,
				p->xdLast, p->ydLast);
	DrawTextLine(s);	p = GetPTUPLET(pL);
	sprintf(s, "link=%u stf=%d", pL, p->staffn);
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;				/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextNOTETUPLEL(qL)) 
		;
	q = GetPANOTETUPLE(qL);
	sprintf(s, "link=%u @%lx tpSync=%d next=%u", qL, (long unsigned int)q, q->tpSync, q->next);
	DrawTextLine(s);	q = GetPANOTETUPLE(qL);
}


/* ---------------------------------------------------------------------- BrowseOttava -- */

void BrowseOttava(LINK pL, short index, Rect *pObjRect)
{
	POTTAVA p;
	PANOTEOTTAVA q;
	LINK qL;
	short i;

	p = GetPOTTAVA(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "staff=%d", p->staffn);
	DrawTextLine(s);	p = GetPOTTAVA(pL);
	sprintf(s, "nxd=%d nyd=%d", p->nxd, p->nyd);
	DrawTextLine(s);	p = GetPOTTAVA(pL);
	sprintf(s, "xd1st=%d yd1st=%d xdLast=%d ydLast=%d", p->xdFirst, p->ydFirst,
				p->xdLast, p->ydLast);
	DrawTextLine(s);	p = GetPOTTAVA(pL);
	sprintf(s, "octSignType=%d noCutoff=%d", p->octSignType, p->noCutoff);
	DrawTextLine(s);
	sprintf(s, "numberVis=%d brackVis=%d", p->numberVis, p->brackVis);
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;				/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextNOTEOTTAVAL(qL)) 
		;
	q = GetPANOTEOTTAVA(qL);
	sprintf(s, "link=%u @%lx opSync=%d next=%u", qL, (long unsigned int)q, q->opSync, q->next);
	DrawTextLine(s);
}

/* --------------------------------------------------------------------- BrowseDynamic -- */

void BrowseDynamic(LINK pL, short index, Rect *pObjRect)
{
	PDYNAMIC p;
	PADYNAMIC q;
	LINK qL;
	short i;

	p = GetPDYNAMIC(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	DrawTextLine("");
	sprintf(s, "firstSyncL=%d lastSyncL=%d",
					DynamFIRSTSYNC(pL), DynamLASTSYNC(pL));
	DrawTextLine(s);

	DynamicToString(DynamType(pL), dynStr);	
	sprintf(s, "dynamicType=%d (%s) IsHairpin=%d", DynamType(pL), dynStr, IsHairpin(pL));
	DrawTextLine(s);
	p = GetPDYNAMIC(pL);
	sprintf(s, "crossSys=%d ", p->crossSys);
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;				/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextDYNAMICL(qL)) 
		;
	q = GetPADYNAMIC(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%u", qL, (long unsigned int)q, q->staffn, q->next);
	DrawTextLine(s);	q = GetPADYNAMIC(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	DrawTextLine(s);	q = GetPADYNAMIC(qL);
	sprintf(s, "mouthWidth=%d otherWidth=%d", q->mouthWidth, q->otherWidth);
	DrawTextLine(s);	q = GetPADYNAMIC(qL);
	sprintf(s, "small=%d", q->small);
	DrawTextLine(s);	q = GetPADYNAMIC(qL);
	sprintf(s, "xd,yd=%d,%d endxd,endyd=%d,%d", q->xd, q->yd, q->endxd, q->endyd);
	DrawTextLine(s);
	
	subL = qL;
}


/* ---------------------------------------------------------------------- BrowseRptEnd -- */

void BrowseRptEnd(LINK pL, short index, Rect *pObjRect)
{
	PRPTEND p;
	PARPTEND q;
	LINK qL;
	short i;

	p = GetPRPTEND(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "rptEndType=%hd", p->subType);
	DrawTextLine(s);	p = GetPRPTEND(pL);
	sprintf(s, "firstObj=%u startRpt=%u endRpt=%u", p->firstObj, p->startRpt, p->endRpt);
	DrawTextLine(s);
	sprintf(s, "count=%d", p->count);
	DrawTextLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++, qL=NextRPTENDL(qL)) 
		;
	q = GetPARPTEND(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%u", qL, (long unsigned int)q, q->staffn, q->next);
	DrawTextLine(s);	q = GetPARPTEND(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	DrawTextLine(s);	q = GetPARPTEND(qL);
	sprintf(s, "rptEndType=%hd", q->subType);
	DrawTextLine(s);
}


/* ---------------------------------------------------------------------- BrowseEnding -- */

void BrowseEnding(LINK pL, short /*index*/, Rect *pObjRect)
{
	PENDING p;

	p = GetPENDING(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "stf=%d", p->staffn);
	DrawTextLine(s);

	sprintf(s, "firstObjL=%u lastObjL=%u", p->firstObjL, p->lastObjL);
	DrawTextLine(s);

	sprintf(s, "endNum=%d noL=%d noR=%d", p->endNum, p->noLCutoff, p->noRCutoff);
	DrawTextLine(s);

	sprintf(s, "xd=%d yd=%d endxd=%d", p->xd, p->yd, p->endxd);
	DrawTextLine(s);
}


/* -------------------------------------------------------------------- ChordSym2Print -- */
/* Convert in place a Pascal string representing a chord symbol to (semi-)readable
form by replacing all delimiters with another character. */

#define DELIMITER FWDDEL_KEY
#define CH_SUBST '|'

void ChordSym2Print(StringPtr);
void ChordSym2Print(StringPtr str)
{
	short i, count=*str;
	
	for (i = 1; i<=count; i++)
		if (str[i]==DELIMITER) str[i] = CH_SUBST;
}

/* --------------------------------------------------------------------- BrowseGraphic -- */

void BrowseGraphic(LINK pL, Rect *pObjRect)
{
	PGRAPHIC p;
	char s2[256];
	LINK aGraphicL;
	PAGRAPHIC aGraphic;

	p = GetPGRAPHIC(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "stf=%d voice=%d", p->staffn, p->voice);
	DrawTextLine (s);
	strcpy(strBuf, NameGraphicType(pL, False));
	sprintf(s, "graphicType=%s", strBuf);
	DrawTextLine (s);

	p = GetPGRAPHIC(pL);
	aGraphicL = FirstSubLINK(pL);
	aGraphic = GetPAGRAPHIC(aGraphicL);
	sprintf(s, "aGraphic->next=%u", aGraphic->next);
	DrawTextLine (s);	p = GetPGRAPHIC(pL);

	sprintf(s, "firstObj=%u lastObj=%u", p->firstObj, p->lastObj);
	DrawTextLine(s);	p = GetPGRAPHIC(pL);
	sprintf(s, "info=%d info2=%d", p->info, p->info2);
	DrawTextLine(s);	p = GetPGRAPHIC(pL);
	switch (p->graphicType) {
		case GRString:
		case GRLyric:
		case GRRehearsal:
		case GRChordSym:
		case GRChordFrame:
		case GRMIDIPatch:
		case GRSusPedalDown:
		case GRSusPedalUp:		
			sprintf(s, "fontInd=%d fontStyle=%d encl=%d", p->fontInd, p->fontStyle,
													p->enclosure);
			DrawTextLine(s);	p = GetPGRAPHIC(pL);
			sprintf(s, "fontSize=%d %s", p->fontSize,
												  p->relFSize? "staff-rel." : "points");
			DrawTextLine (s);	aGraphic = GetPAGRAPHIC(aGraphicL);

			sprintf(s, "aGraphic->strOffset=%ld ", aGraphic->strOffset);
			if (PCopy(aGraphic->strOffset)==NULL) {
				DrawTextLine(s);
				sprintf(s, "** PCopy(aGraphic->strOffset) is NULL **");
				DrawTextLine(s);			
			}
			else {
				Pstrcpy((unsigned char *)s2, PCopy(aGraphic->strOffset));
				sprintf(&s[strlen(s)], "char count=%d", (unsigned char)s2[0]);
				DrawTextLine(s);			
				if (p->graphicType==GRChordSym)
					ChordSym2Print((unsigned char *)s2);
				sprintf(s, "str='%s'", PToCString((unsigned char *)s2));
				DrawTextLine(s);
			}

			break;
		case GRArpeggio:
			sprintf(s, "arpInfo=%d", ARPINFO(p->info2));
			DrawTextLine(s);
			break;
		case GRDraw:
			sprintf(s, "thick=%d%%", p->gu.thickness);
			DrawTextLine(s);
			break;
		default:
			;
	}
}


/* ----------------------------------------------------------------------- BrowseTempo -- */

void BrowseTempo(LINK pL, Rect *pObjRect)
{
	PTEMPO p;
	char t[256];
	
	p = GetPTEMPO(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);

	sprintf(s, "stf=%d", p->staffn);
	DrawTextLine(s); p = GetPTEMPO(pL);

	switch (p->subType) {
		case BREVE_L_DUR:
			strcpy(t, "breve"); break;
		case WHOLE_L_DUR:
			strcpy(t, "whole"); break;
		case HALF_L_DUR:
			strcpy(t, "half"); break;
		case QTR_L_DUR:
			strcpy(t, "quarter"); break;
		case EIGHTH_L_DUR:
			strcpy(t, "eighth"); break;
		case SIXTEENTH_L_DUR:
			strcpy(t, "16th"); break;
		case THIRTY2ND_L_DUR:
			strcpy(t, "32nd"); break;
		case SIXTY4TH_L_DUR:
			strcpy(t, "64th"); break;
		case ONE28TH_L_DUR:
			strcpy(t, "128th"); break;
		case UNKNOWN_L_DUR:
			strcpy(t, "unknown duration"); break;
		default:
			strcpy(t, "ILLEGAL DURATION"); break;		
	}
	sprintf(s, "subType=%d (%s) dotted=%d", p->subType, t, p->dotted);
	DrawTextLine(s); p = GetPTEMPO(pL);

	sprintf(s, "noMM=%d hideMM=%d expanded=%d", p->noMM, p->hideMM, p->expanded);
	DrawTextLine(s); p = GetPTEMPO(pL);

	sprintf(s, "firstObjL=%u tempoMM=%d", p->firstObjL, p->tempoMM);
	DrawTextLine(s); p = GetPTEMPO(pL);

	sprintf(s, "p->strOffset=%ld ", p->strOffset);
	if (PCopy(p->strOffset)==NULL) {
		DrawTextLine(s);			
		sprintf(s, "** PCopy(p->strOffset) is NULL **");
		DrawTextLine(s);
	}
	else {
		Pstrcpy((unsigned char *)t, PCopy(p->strOffset));
		sprintf(&s[strlen(s)], "char count=%d", (unsigned char)t[0]);
		DrawTextLine(s);			
		sprintf(s, "str='%s'", PToCString((unsigned char *)t));
		DrawTextLine(s);
	}

	sprintf(s, "p->metroStrOffset=%ld ", p->metroStrOffset);
	if (PCopy(p->metroStrOffset)==NULL) {
		DrawTextLine(s);			
		sprintf(s, "** PCopy(p->metroStrOffset) is NULL **");
		DrawTextLine(s);			
	}
	else {
		Pstrcpy((unsigned char *)t, PCopy(p->metroStrOffset));
		sprintf(&s[strlen(s)], "char count=%d", (unsigned char)t[0]);
		DrawTextLine(s);
		sprintf(s, "str='%s'", PToCString((unsigned char *)t));
		DrawTextLine(s);
	}
}

/* ----------------------------------------------------------------------- BrowseSpace -- */

void BrowseSpace(LINK pL, Rect *pObjRect)
{
	PSPACER p;

	p = GetPSPACER(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	p = GetPSPACER(pL);
	sprintf(s, "stf=%d spWidth=%d", p->staffn, p->spWidth);
	DrawTextLine(s); p = GetPSPACER(pL);
	sprintf(s, "bottomStf=%d", p->bottomStaff);
	DrawTextLine(s);
}

/* ------------------------------------------------------------------------ BrowseSlur -- */

void BrowseSlur(LINK pL, short index, Rect *pObjRect)
{
	PSLUR p;
	PASLUR aSlur;
	LINK	aSlurL;
	short i;
	
	p = GetPSLUR(pL);
	*pObjRect = p->objRect;
	OffsetRect(pObjRect, paperRect.left, paperRect.top);
	
	sprintf(s, "stf=%d voice=%d", p->staffn, p->voice);
	DrawTextLine(s);	p = GetPSLUR(pL);
	sprintf(s, "tie=%s", p->tie ? "True" : "False");
	DrawTextLine(s);	p = GetPSLUR(pL);
	sprintf(s, "crossStf=%d crossStfBack=%d crossSys=%d",
		p->crossStaff,p->crossStfBack,p->crossSystem);
	DrawTextLine(s);	p = GetPSLUR(pL);

	sprintf(s, "firstSync=%d", SlurFIRSTSYNC(pL));
	if (!SyncTYPE(SlurFIRSTSYNC(pL)))
		sprintf(&s[strlen(s)], "(%s)", (MeasureTYPE(SlurFIRSTSYNC(pL))? "MEAS" : "TYPE?"));
	sprintf(&s[strlen(s)], " lastSync=%d", SlurLASTSYNC(pL));
	if (!SyncTYPE(SlurLASTSYNC(pL)))
		sprintf(&s[strlen(s)],  "(%s)", (SystemTYPE(SlurLASTSYNC(pL))? "SYS" : "TYPE?"));

	DrawTextLine(s);
	
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawTextLine(s);
	
	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,aSlurL=FirstSubLINK(pL); i<index; i++, aSlurL=NextSLURL(aSlurL)) 
		;
	aSlur = GetPASLUR(aSlurL);

	strcpy(s, "flags=");
	if (aSlur->selected) strcat(s, "SELECTED ");
	if (aSlur->visible)	strcat(s, "VISIBLE ");
	if (aSlur->soft)		strcat(s, "SOFT ");
	if (aSlur->dashed)	strcat(s, "DASHED ");
	DrawTextLine(s); 	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "firstInd=%d lastInd=%d", aSlur->firstInd, aSlur->lastInd);
	DrawTextLine(s); 	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "knot=%d,%d", aSlur->seg.knot.h, aSlur->seg.knot.v);
	DrawTextLine(s);	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "c0=%d,%d", aSlur->seg.c0.h, aSlur->seg.c0.v);
	DrawTextLine(s);	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "c1=%d,%d", aSlur->seg.c1.h, aSlur->seg.c1.v);
	DrawTextLine(s);	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "endKnot=%d,%d", aSlur->endKnot.h, aSlur->endKnot.v);
	DrawTextLine(s);
	sprintf(s, "startPt=%d,%d endPt=%d,%d", aSlur->startPt.h, aSlur->startPt.v,
				aSlur->endPt.h, aSlur->endPt.v);
	DrawTextLine(s);
	
	subL = aSlurL;
}


/* ----------------------------------------------------------------------- ShowContext -- */

#define iText 2

void ShowContext(Document *doc)
{
	LINK		pL, tempoL, graphicL, aGraphicL;
	PTEMPO		pTempo;
	Boolean		done;
	CONTEXT		context;
	DialogPtr	dlog;
	Handle		tHdl;
	short		itype, ditem;
	short		theStaff;
	GrafPtr		oldPort;
	PAGRAPHIC	aGraphic;
	char		s2[256];

	/* Get LINK to and staff number of first selected object or of insertion point. */

	if (doc->selStartL==doc->selEndL) {
		pL = doc->selStartL;
		theStaff = doc->selStaff;
	}
	else {
		theStaff = GetStaffFromSel(doc, &pL);
		if (theStaff==NOONE) {
			SysBeep(4);
			LogPrintf(LOG_WARNING, "Can't get staff number.  (ShowContext)\n");
			return;
		}
	}
	
	GetContext(doc, pL, theStaff, &context);

	GetPort(&oldPort);
	dlog = GetNewDialog(CONTEXT_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) MissingDialog(CONTEXT_DLOG);
	SetPort(GetDialogWindowPort(dlog));
	ShowWindow(GetDialogWindow(dlog));

	GetDialogItem(dlog, iText, &itype, &tHdl, &bRect);

	done = False;
	TextFont(SYSFONTID_MONOSPACED);
	TextSize(9);
	EraseRect(&bRect);
	linenum = 1;

	sprintf(s, "doc->selStaff=%hd", doc->selStaff);
	DrawTextLine(s);
	sprintf(s, "doc->selStartL=%hd ->selEndL=%hd", doc->selStartL, doc->selEndL);
	DrawTextLine(s);
	sprintf(s, " ");
	DrawTextLine(s);
	sprintf(s, "Context at link %hd for staff %hd:", pL, theStaff);
	DrawTextLine(s);
	sprintf(s, " ");
	DrawTextLine(s);
	strcpy(s, "flags=");
	if (context.visible) strcat(s, "VISIBLE ");
	if (context.staffVisible) strcat(s, "STAFFVIS ");
	if (context.measureVisible) strcat(s, "MEASUREVIS  ");
	DrawTextLine(s);
	strcpy(s, "flags=");
	if (context.inMeasure) strcat(s, "IN MEASURE ");
	else strcat(s, "NOT IN MEASURE ");
	DrawTextLine(s);
	sprintf(s, "systemTop,Left,Bottom=%d,%d,%d", context.systemTop, context.systemLeft,
		context.systemBottom);
	DrawTextLine(s);
	sprintf(s, "staffTop,Left,Right=%d,%d,%d", context.staffTop, context.staffLeft,
		context.staffRight);
	DrawTextLine(s);
	sprintf(s, "staffHeight,HalfHeight=%d,%d", context.staffHeight, context.staffHalfHeight);
	DrawTextLine(s);
	if (context.showLines==SHOW_ALL_LINES)
		sprintf(s, "staffLines=%hd showLines=all", context.staffLines);
	else
		sprintf(s, "staffLines=%hd showLines=%hd", context.staffLines, context.showLines);
	DrawTextLine(s);
	sprintf(s, "showLedgers=%s fontSize=%d", context.showLedgers? "yes" : "no",
				context.fontSize);
	DrawTextLine(s);
	sprintf(s, "measureTop,Left=%d,%d", context.measureTop, context.measureLeft);
	DrawTextLine(s);
	ClefToString(context.clefType, clefStr);	
	sprintf(s, "clefType=%hd (%s)", context.clefType, clefStr);
	DrawTextLine(s);
	sprintf(s, "nKSItems=%hd", context.nKSItems);
	DrawTextLine(s);
	sprintf(s, "timeSigType,n/d=%hd,%hd/%hd", context.timeSigType, context.numerator,
		context.denominator);
	DrawTextLine(s);

	DynamicToString(context.dynamicType, dynStr);	
	sprintf(s, "dynamicType=%hd (%s)", context.dynamicType, dynStr);
	DrawTextLine(s);
	
	/* Though they're not part of the CONTEXT object, the current tempo is really part
	   of the context in the normal sense, and the last previous Graphic may well be
	   (e.g., if it's "solo", "pizz.", etc.); both are worth showing. */
	
	DrawTextLine("----------------------------");
	tempoL = LSSearch(pL, TEMPOtype, ANYONE, GO_LEFT, False);
	if (tempoL==NILINK) {
		DrawTextLine("No TEMPO object preceding this.");
	}
	else {
		pTempo  = GetPTEMPO(tempoL);
		sprintf(s, "Last prev. Tempo link=%u tempoMM=%d", tempoL, pTempo->tempoMM);
		DrawTextLine(s);
	}

	graphicL = LSSearch(pL, GRAPHICtype, doc->selStaff, GO_LEFT, False);
	if (graphicL==NILINK) {
		DrawTextLine("No GRAPHIC obj on staff before this.");
	}
	else {
		sprintf(s, "Last prev. Graphic link=%u", graphicL);
		DrawTextLine(s);
		switch (GraphicSubType(graphicL)) {
			case GRString:
			case GRLyric:
			case GRRehearsal:
			case GRChordSym:
			case GRChordFrame:
			case GRMIDIPatch:

				aGraphicL = FirstSubLINK(graphicL);
				aGraphic = GetPAGRAPHIC(aGraphicL);
			
				//sprintf(s, "aGraphic->strOffset=%ld ", aGraphic->strOffset);
				if (PCopy(aGraphic->strOffset)==NULL) {
					DrawTextLine(s);
					sprintf(s, "** PCopy(aGraphic->strOffset) is NULL **");
					DrawTextLine(s);			
				}
				else {
					Pstrcpy((unsigned char *)s2, PCopy(aGraphic->strOffset));
					//sprintf(&s[strlen(s)], "char count=%d", (unsigned char)s2[0]);
					//DrawTextLine(s);			
					if (GraphicSubType(graphicL)==GRChordSym)
						ChordSym2Print((unsigned char *)s2);
					sprintf(s, "str='%s'", PToCString((unsigned char *)s2));
					DrawTextLine(s);
				}

			default:
				;
		}
	}

	done = False;
	do {
		ModalDialog(NULL, &ditem);					/* Handle dialog events */
		if (ditem==OK) done = True;
	} while (!done);
	DisposeDialog(dlog);
	SetPort(oldPort);
}

//#endif /* PUBLIC_VERSION */
