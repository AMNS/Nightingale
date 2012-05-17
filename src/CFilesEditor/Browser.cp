/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/* File Browser.c - object list browser functions - rev. for v.2000 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

//#ifndef PUBLIC_VERSION		/* If public, skip this file completely! */

#define BROWSER_DLOG 1900
#define CONTEXT_DLOG 1901
#define LEADING 11			/* Vertical space between lines displayed (pixels) */

static Rect bRect,objRect,oldObjRect,paperRect;
static DRect systemRect;
static INT16 linenum, showBPage;
static char s[300];			/* A bit more than 256 to protect against 255-char. Graphics, etc. */
static char objList[16];
static LINK subL;

static void ShowObjRect(Document *);
static void ChangeSelectObj(Document *, LINK, INT16, INT16, Boolean);
static void ShowObject(Document *, LINK, INT16);
static void DrawLine(char *);
static void BrowseHeader(Document *, LINK, INT16);
static void BrowsePage(LINK);
static void BrowseSystem(LINK);
static void BrowseStaff(LINK, INT16);
static void BrowseConnect(LINK, INT16);
static void BrowseClef(LINK, INT16);
static void BrowseKeySig(LINK, INT16);
static void BrowseTimeSig(LINK, INT16);
static void BrowseMeasure(LINK, INT16);
static void BrowsePseudoMeas(LINK, INT16);
static void BrowseSync(LINK, INT16);
static void BrowseGRSync(LINK, INT16);
static void BrowseBeamset(LINK, INT16);
static void BrowseTuplet(LINK, INT16);
static void BrowseOctava(LINK, INT16);
static void BrowseDynamic(LINK, INT16);
static void BrowseRptEnd(LINK, INT16);
static void BrowseEnding(LINK, INT16);
static void BrowseGraphic(LINK);
static void BrowseTempo(LINK);
static void BrowseSpace(LINK);
static void BrowseSlur(LINK, INT16);
static pascal Boolean BrowserFilter(DialogPtr, EventRecord *, INT16 *);


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

pascal Boolean BrowserFilter(DialogPtr theDialog, EventRecord *theEvent, INT16 *item)
{
		Boolean ans = FALSE;
		short itype; Handle tHdl; Rect textRect; Point where;
		WindowPtr w = (WindowPtr)theEvent->message;
		int part;
		
		switch (theEvent->what) {
			case updateEvt:
				if (w == GetDialogWindow(theDialog)) {
					BeginUpdate(GetDialogWindow(theDialog));
					DrawDialog(theDialog);
//					OutlineOKButton(theDialog,TRUE);
					EndUpdate(GetDialogWindow(theDialog));
					}
				*item = 0;
				ans = TRUE;
				break;
			case mouseDown:
				where = theEvent->where;
				GlobalToLocal(&where);
				GetDialogItem(theDialog, iText, &itype, &tHdl, &textRect);	
				if (PtInRect(where, &textRect)) {
					*item = iShowMore;
					ans = TRUE;
					break;
				}
				else {
					Rect bounds = GetQDScreenBitsBounds();
					part = FindWindow(theEvent->where,&w);
					switch(part) {
						case inDrag:
							if (w == GetDialogWindow(theDialog)) {
								DragWindow(w,theEvent->where,&bounds);
							}
							break;
					}
				}

			case keyDown:
			case autoKey:
				if (DlgCmdKey(theDialog, theEvent, item, FALSE)) return TRUE;
				break;
			}
		return ans;
	}


/* ------------------------------------------------------------------ Browser -- */
/* Browser displays and handles interaction with a small window that lets the
user (a Nightingale programmer, presumably) prowl around in an object list.
tailL may be NILINK; in this case, the "go to tail" button will be inoperative,
but everything else will work as usual. */

void Browser(Document *doc, LINK headL, LINK tailL)
{
	DialogPtr dlog;
	short itype, ditem; INT16 type;
	ControlHandle selHdl, deselHdl, tHdl;
	Rect tRect;
	INT16 index, oldIndex, oldShowBPage, goLoc;
	Boolean done;
	GrafPtr oldPort;
	LINK	pL, oldL;
	LINK oldpL;
	ControlHandle scroll; short part;
	Document *saveDoc;
	ModalFilterUPP	filterUPP;

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
	
	SetDialogDefaultItem(dlog,OK);
	
	GetDialogItem(dlog, iText, &itype, (Handle *)&tHdl, &bRect);
	GetDialogItem(dlog, iSelect, &itype, (Handle *)&selHdl, &tRect);
	GetDialogItem(dlog, iDeselect, &itype, (Handle *)&deselHdl, &tRect);
	
	/* Initialize for the object list being browsed */
	
	if (headL==doc->headL) pL = doc->selStartL;
	else pL = headL;
	
	if (headL==doc->headL) strcpy(objList, "");
	else if (headL==doc->masterHeadL) strcpy(objList, "Master Page ");
	else if (headL==doc->undo.headL) strcpy(objList, "Undo ");
	
	/* We support the "Go" button only for the main object list. */
	if (headL!=doc->headL) {
		GetDialogItem(dlog, iGo, &itype, (Handle *)&tHdl, &tRect);
		HiliteControl(tHdl, CTL_INACTIVE);
	}
	else PutDlgWord(dlog, iGoNum, pL, TRUE);

	ArrowCursor();

/*--- 2. Interact with user til they push OK or Cancel. --- */

	SetRect(&oldObjRect,0,0,0,0);
	
	index = 0;
	showBPage = 0;

	oldpL = NILINK;
	oldIndex = -1;
	oldShowBPage = -1;

	done = FALSE;
	ShowWindow(GetDialogWindow(dlog));
//	OutlineOKButton(dlog,TRUE);
	
	do {
		/* If the desired thing has changed or, for Header, may have changed... */
		if (pL!=oldpL || index!=oldIndex
		|| (ObjLType(pL)==HEADERtype && index<0 && showBPage!=oldShowBPage)) {
		
			EraseRect(&bRect);
			
			/* objRect = oldObjRect; */
			ShowObjRect(doc);									/* Restore previous objRect */
			ShowObject(doc, pL, index); 					/*	Write out info and get new objRect */
			ShowObjRect(doc);									/* Invert this objrect */
			/* oldObjRect = objRect; 							Save for later restoration */
			
			if (ObjLType(pL)==HEADERtype
			||  ObjLType(pL)==TAILtype) {					/* Case iSelect & iDeselect don't handle these */
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
		scroll = NULL;
		switch (ditem) {
			case iOK:
			  	done = TRUE;
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
							if (!(OptionKeyDown() || CmdKeyDown())) break;
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
				if (tailL) pL = tailL;
				else       SysBeep(1);
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
							if (!(OptionKeyDown() || CmdKeyDown())) break;
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
					case OCTAVAtype:
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
					case OCTAVAtype:
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
			 		ChangeSelectObj(doc, pL, index, SMSelect, ShiftKeyDown());
			   break;
			case iDeselect:
			 	if (headL==doc->headL)
					ChangeSelectObj(doc, pL, index, SMDeselect, FALSE);
			 	break;
			case iScrollUp:
			 	scroll = doc->vScroll; part = kControlPageUpPart;
			 	break;
			case iScrollDown:
			 	scroll = doc->vScroll; part = kControlPageDownPart;
			 	break;
			case iScrollRight:
			 	scroll = doc->hScroll; part = kControlPageUpPart;
			   break;
			case iScrollLeft:
			 	scroll = doc->hScroll; part = kControlPageDownPart;
			 	break;
			case iGo:
				SelectDialogItemText(dlog, iGoNum, 0, ENDTEXT); 					/* hilite number */
			 	if (GetDlgWord(dlog, iGoNum, &goLoc))
					if (InDataStruct(doc, (LINK)goLoc, MAIN_DSTR)) {
						pL = (LINK)goLoc;
						index = 0;
						break;
					}
				SysBeep(1);
				break;
			case iShowMore:
				showBPage++;
				break;			   
			 default:
			 	;
		}
		if (scroll) {
			GrafPtr oldPort; Rect box,portRect;
			WindowPtr w; 
			Point oldOrigin; INT16 dx,dy;
			ShowObjRect(doc);
			
			w = doc->theWindow;
			//box = (*(((WindowPeek)doc))->strucRgn)->rgnBBox;
			GetWindowRgnBounds(doc->theWindow, kWindowStructureRgn, &box);
			GetPort(&oldPort); 
			SetPort(GetDocWindowPort(doc));
			GlobalToLocal(&TOP_LEFT(box));
			GlobalToLocal(&BOT_RIGHT(box));
			oldOrigin = doc->origin;
			GetWindowPortBounds(w,&portRect);
			ClipRect(&portRect);
			ScrollDocument(scroll,part);
			GetWindowPortBounds(w,&portRect);
			ClipRect(&portRect);
			DrawControls(w);
			ClipRect(&doc->viewRect);
			dx = oldOrigin.h - doc->origin.h;
			dy = oldOrigin.v - doc->origin.v;
			if (dx!=0 || dy!=0) {
				OffsetRect(&box,dx,dy);
				InvalWindowRect(w,&box);
				DoUpdate(doc->theWindow);
				}
			SetPort(oldPort);
			
			ShowObjRect(doc);
			ArrowCursor();
			}
		
	} while (!done);
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	InstallDoc(saveDoc);
	InvertRect(&objRect);
	SetRect(&objRect,0,0,0,0);
	return; 
}


/* ------------------------------------------------------------------ DrawLine -- */
/* Draw the specified C string on the next line in Browser dialog. */

void DrawLine(char *s)
{
	MoveTo(bRect.left, bRect.top+linenum*LEADING);
	DrawCString(s);
	++linenum;
}


/* --------------------------------------------------------------- ShowObjRect -- */

static void ShowObjRect(Document *doc)
{
	GrafPtr oldPort;
	
	GetPort(&oldPort); SetPort(GetDocWindowPort(doc));
	InvertRect(&objRect);
	SetPort(oldPort);
}


/* ----------------------------------------------------------------- SelSubObj -- */

static void SelSubObj(LINK, LINK);
static void SelSubObj(LINK pL, LINK subL)
{
	if (pL && subL)
		switch (ObjLType(pL)) {
			case CONNECTtype:
				ConnectSEL(subL) = TRUE; break;
			case STAFFtype:
				StaffSEL(subL) = TRUE; break;
			case SYNCtype:
				NoteSEL(subL) = TRUE; break;
			case CLEFtype:
				ClefSEL(subL) = TRUE; break;
			case KEYSIGtype:
				KeySigSEL(subL) = TRUE; break;
			case TIMESIGtype:
				TimeSigSEL(subL) = TRUE; break;
			case DYNAMtype:
				DynamicSEL(subL) = TRUE; break;
			case MEASUREtype:
				MeasureSEL(subL) = TRUE; break;
			case SLURtype:
				SlurSEL(subL) = TRUE; break;
			default:
				SysBeep(1);
		}	
}

/* ----------------------------------------------------------- ChangeSelectObj -- */
/* If mode is SMSelect, select pL and (if it has any subobjects) either the
current one or all of them; if mode is SMDeselect, deselect pL and all of its
subobjects, if it has any. This function is designed to handle anomalous
situations resulting (most likely) from bugs in Nightingale, so it tries to
work in the safest possible way. */

static void ChangeSelectObj(Document *doc, LINK pL,
							INT16	index,		/* Index of subobject currently displayed */
							INT16	mode,
							Boolean selSub	/* If SMSelect, TRUE=select only current subobject */
							)
{
	CONTEXT	context[MAXSTAVES+1];
	Boolean	found;
	INT16		pIndex;
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
				else			SelAllSubObjs(pL);
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
		case SPACEtype:
		case ENDINGtype:
		case OCTAVAtype:
			CheckObject(doc, pL, &found, NULL, context, mode, &pIndex, stfRange);
			break;
		default:
			MayErrMsg("ChangeSelectObj: can't handle type %ld", (long)ObjLType(pL));
			return;
	}
	LinkSEL(pL) = (mode==SMSelect);
	UpdateSelection(doc);
	if (doc->selStartL==doc->selEndL) MEAdjustCaret(doc,TRUE);
	EraseRect(&bRect);
	ShowObject(doc, pL, index);							/*	Show the thing again */
}


/* ----------------------------------------------------------------- ShowObject -- */

void ShowObject(Document *doc, LINK pL, INT16 index)
{
	PMEVENT p;
	PSYSTEM pSystem;
	const char *ps;
	Rect r;
	LINK pageL,sysL,measL;
	
	linenum = 1;
	TextFont(SYSFONTID_MONOSPACED);
	TextSize(9);

	ps = NameNodeType(pL);
	sprintf(s, "%s%s [%d]", objList, ps, ObjLType(pL));
	if (doc->selStartL==pL)
		strcat(s, "  (selStartL)");
	else if (doc->selEndL==pL)
		strcat(s, "  (selEndL)");
	DrawLine(s);

	sprintf(s, "here (left,right)=%u (%u,%u)", pL, LeftLINK(pL), RightLINK(pL));
	DrawLine(s);
	p = GetPMEVENT(pL);
	sprintf(s, "@%lx xd,yd=%d,%d", p, p->xd, p->yd);
	DrawLine(s);
	strcpy(s, "flags=");
	if (LinkSEL(pL))
		strcat(s, "SELECTED ");
	if (LinkVIS(pL))
		strcat(s, "VISIBLE ");
	if (LinkSOFT(pL))
		strcat(s, "SOFT ");
	if (LinkVALID(pL))
		strcat(s, "VALID ");
	if (LinkTWEAKED(pL))
		strcat(s, "TWEAKED ");
	DrawLine(s);
	r = LinkOBJRECT(pL);
	sprintf(s, "objRect=%d %d %d %d",
				r.top, r.left, r.bottom, r.right);

	/* Search back for this object's measure, system, and page (poor man's GetContext) */
	
	measL = pL;
	while (measL!=NILINK && ObjLType(measL)!=MEASUREtype) measL = LeftLINK(measL);
#ifdef NOMORE
	if (measL) {
		pMeasure = GetPMEASURE(sysL);
		measureBBox = pMeasure->measureBBox;
		}
	 else
		SetRect(&measureBBox,0,0,0,0);
#endif
	sysL = pL;
	while (sysL!=NILINK && ObjLType(sysL)!=SYSTEMtype) sysL = LeftLINK(sysL);
	if (sysL) {
		pSystem = GetPSYSTEM(sysL);
		systemRect = pSystem->systemRect;
		}
	 else
		SetDRect(&systemRect,0,0,0,0);
	
	/* Get sheet this object is on so we can mark object's bounding box while browsing */
	pageL = pL;
	while (pageL!=NILINK && ObjLType(pageL)!=PAGEtype) pageL = LeftLINK(pageL);
	if (pageL)
		if (GetSheetRect(doc,SheetNUM(pageL),&paperRect)!=INARRAY_INRANGE) pageL = NILINK;
		
	DrawLine(s);
	sprintf(s, "nEntries=%d 1stSubObj=%u", LinkNENTRIES(pL), FirstSubLINK(pL));
	DrawLine(s);
	DrawLine("----------------------------");
	
	subL = NILINK;
	switch (ObjLType(pL)) {
	case HEADERtype:
		BrowseHeader(doc, pL, index);
		break;
	case PAGEtype:
		BrowsePage(pL);
		break;
	case SYSTEMtype:
		BrowseSystem(pL);
		break;
	case STAFFtype:
		BrowseStaff(pL, index);
		break;
	case CONNECTtype:
		BrowseConnect(pL, index);
		break;
	case CLEFtype:
		BrowseClef(pL, index);
		break;
	case KEYSIGtype:
		BrowseKeySig(pL, index);
		break;
	case TIMESIGtype:
		BrowseTimeSig(pL, index);
		break;
	case MEASUREtype:
		BrowseMeasure(pL, index);
		break;
	case PSMEAStype:
		BrowsePseudoMeas(pL, index);
		break;
	case SYNCtype:
		BrowseSync(pL, index);
		break;
	case GRSYNCtype:
		BrowseGRSync(pL, index);
		break;
	case BEAMSETtype:
		BrowseBeamset(pL, index);
		break;
	case TUPLETtype:
		BrowseTuplet(pL, index);
		break;
	case OCTAVAtype:
		BrowseOctava(pL, index);
		break;
	case DYNAMtype:
		BrowseDynamic(pL, index);
		break;
	case RPTENDtype:
		BrowseRptEnd(pL, index);
		break;
	case ENDINGtype:
		BrowseEnding(pL, index);
		break;
	case GRAPHICtype:
		BrowseGraphic(pL);
		break;
	case TEMPOtype:
		BrowseTempo(pL);
		break;
	case SPACEtype:
		BrowseSpace(pL);
		break;
	case SLURtype:
		BrowseSlur(pL, index);
		break;
	default:
		;
	}
}

/* -------------------------------------------------------------- ShowVoicePage -- */

#define VOICEPAGESIZE 15

void ShowVoicePage(Document *, INT16);
void ShowVoicePage(Document *doc, INT16 startV)
{
	INT16 v; unsigned char ch;
	
	if (startV<1) startV = 1;				/* Avoid negative index in loop below */
	
	for (v = startV; v<startV+VOICEPAGESIZE && v<=MAXVOICES; v++)
		if (doc->voiceTab[v].partn!=0) {
			switch (doc->voiceTab[v].voiceRole) {
				case UPPER_DI: ch = 'U'; break;
				case LOWER_DI: ch = 'L'; break;
				case CROSS_DI: ch = 'C'; break;
				case SINGLE_DI: ch = '.'; break;
				default: ch = '?';
			}
			sprintf(s, "%c%ciVoice %d in part %d vcPos=%c relVc=%d",
						(v==doc->lookVoice? 'L' : ' '),
						(v>1 && doc->voiceTab[v-1].partn==0? 'É' : ' '),
						v, doc->voiceTab[v].partn, ch,
						doc->voiceTab[v].relVoice);
			DrawLine(s);
		}
}


/* --------------------------------------------------------------- BrowseHeader -- */

void BrowseHeader(Document *doc, LINK pL, INT16 index)
{
	PPARTINFO q;
	INT16 i, v;
	LINK qL;
	char string[256];
	
	if (index<0) {
		INT16 startV; Boolean pageEmpty;
		/*
		 *	Show a "page" of the voice-mapping table. If the current page is empty
		 * or it's past the end of the table, go back to the first page.
		 */
		startV = showBPage*VOICEPAGESIZE+1;
		if (startV>MAXVOICES)
			pageEmpty = TRUE;
		else
			for (pageEmpty = TRUE, v = startV;
				  v<=startV+VOICEPAGESIZE && v<=MAXVOICES; v++)
				if (doc->voiceTab[v].partn!=0) pageEmpty = FALSE;
		if (pageEmpty) {
			showBPage = 0;
			startV = showBPage*VOICEPAGESIZE+1;
		}
		ShowVoicePage(doc, startV);
	}
		
	else {
		sprintf(s, "numSheets=%d currSheet=%d 1stSheet=%d", doc->numSheets,
					doc->currentSheet, doc->firstSheet);
		DrawLine(s);
		sprintf(s, "1stPgNum=%d; nsys=%d currSys=%d nstf=%d", doc->firstPageNumber,
					doc->nsystems, doc->currentSystem, doc->nstaves);
		DrawLine(s);
		sprintf(s, "s/altsrastral=%d,%d transp=%d lastGlFont=%d", doc->srastral, 
					doc->altsrastral, doc->transposed, doc->lastGlobalFont);
		DrawLine(s);
	
		sprintf(s, "pprRect=p%d,%d,%d,%d marg=p%d,%d,%d,%d",
			doc->paperRect.top, doc->paperRect.left,
			doc->paperRect.bottom, doc->paperRect.right,
			doc->marginRect.top, doc->marginRect.left,
			doc->marginRect.bottom, doc->marginRect.right);
		DrawLine(s);
		sprintf(s, "origPpr=%d,%d,%d,%d",
			doc->origPaperRect.top, doc->origPaperRect.left,
			doc->origPaperRect.bottom, doc->origPaperRect.right);
		DrawLine(s);
		sprintf(s, "origin=%d,%d shtOrgn=%d,%d",
			doc->origin.v, doc->origin.h,
			doc->sheetOrigin.v, doc->sheetOrigin.h);
		DrawLine(s);
		sprintf(s, "currPpr=%d,%d,%d,%d",
			doc->currentPaper.top, doc->currentPaper.left,
			doc->currentPaper.bottom, doc->currentPaper.right);
		DrawLine(s);
		sprintf(s, "viewR=%d,%d,%d,%d",
			doc->viewRect.top, doc->viewRect.left,
			doc->viewRect.bottom, doc->viewRect.right);
		DrawLine(s);
		sprintf(s, "ledgerYSp=%d nFontRecs=%d spPercent=%d",
					doc->ledgerYSp, doc->nFontRecords, doc->spacePercent);
		DrawLine(s);
		sprintf(s, "1stIndent=%d yBtwnSys=%d spTab=%d mag=%d",
						doc->firstIndent, doc->yBetweenSys, doc->spaceTable,doc->magnify);
		DrawLine(s);
		sprintf(s, "comment='%s'", doc->comment);
		DrawLine(s);
		sprintf(s, "strPool=%lx size=%ld nfontsUsed=%d:", doc->stringPool,
						GetHandleSize((Handle)doc->stringPool), doc->nfontsUsed);
		DrawLine(s);
	/* Show a max. of the first 10 fonts used because of dialog space limitations, but
	 * actually, as of v. 99b6, that's all there can be anyway.
	 */
		for (i = 0; i<doc->nfontsUsed && i<=10; i++) {
			PStrCopy((StringPtr)(doc->fontTable[i].fontName),
						(StringPtr)string);
			PToCString((StringPtr)string);
			sprintf(s, "  (%d) name='%s' ID=%d", i, string, doc->fontTable[i].fontID);
			DrawLine(s);
		}
		sprintf(s, "fmsInputDevice=%d",
			doc->fmsInputDevice);
		DrawLine(s);
		sprintf(s, "(matchType=%d name=%#s)",
			doc->fmsInputDestination.basic.destinationType,
			doc->fmsInputDestination.basic.name);
		DrawLine(s);
		
		sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
		DrawLine(s);
	
		if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

		for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextPARTINFOL(qL)) ;
	
		q = GetPPARTINFO(qL);
		sprintf(s, "link=%u @%lx next=%d", qL, q, q->next);
		DrawLine(s);	q = GetPPARTINFO(qL);
		sprintf(s, "firststf=%d laststf=%d velo=%d transp=%d",
			q->firstStaff, q->lastStaff,
			q->partVelocity, q->transpose);
		DrawLine(s);	q = GetPPARTINFO(qL);
		sprintf(s, "patch=%d chan=%d name=%s",
			q->patchNum, q->channel, q->name);
		DrawLine(s);
#ifdef CARBON_NOTYET_BANKNUM
		sprintf(s, "bankNumber0=%d bankNumber32=%d",
			q->bankNumber0, q->bankNumber32);
#endif
		DrawLine(s);
		sprintf(s, "fmsOutputDevice=%d",
			q->fmsOutputDevice);
		DrawLine(s);
		sprintf(s, "(matchType=%d name=%#s)",
			q->fmsOutputDestination.basic.destinationType,
			q->fmsOutputDestination.basic.name);
		DrawLine(s);
	}
	
	SetRect(&objRect,0,0,0,0);
}


/* --------------------------------------------------------------- BrowsePage -- */

void BrowsePage(LINK pL)
{
	PPAGE p;
	p = GetPPAGE(pL);
	sprintf(s, "l/r Page=%d/%d", p->lPage, p->rPage);
	DrawLine(s);
	p = GetPPAGE(pL);
	sprintf(s, "sheetNum=%d", p->sheetNum);
	DrawLine(s);
	
	objRect = paperRect;
}


/* ------------------------------------------------------------- BrowseSystem -- */

void BrowseSystem(LINK pL)
{
	PSYSTEM p;
	
	p = GetPSYSTEM(pL);
	sprintf(s, "l/r System=%d/%d", p->lSystem, p->rSystem);
	DrawLine(s);
	p = GetPSYSTEM(pL);
	sprintf(s, "systemNum=%d", p->systemNum);
	DrawLine(s);
	p = GetPSYSTEM(pL);
	sprintf(s, "pageL=%d", p->pageL);
	DrawLine(s);
	p = GetPSYSTEM(pL);
	D2Rect(&p->systemRect, &objRect);
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	sprintf(s, "systemRect=%d %d %d %d",
			p->systemRect.top,
			p->systemRect.left,
			p->systemRect.bottom,
			p->systemRect.right);
	DrawLine(s);
}


/* ------------------------------------------------------------- BrowseStaff -- */

void BrowseStaff(LINK pL, INT16 index)
{
	PSTAFF p;
	PASTAFF q;
	LINK qL;
	INT16 i;

	p = GetPSTAFF(pL);
	sprintf(s, "l/r Staff=%d/%d", p->lStaff, p->rStaff);
	DrawLine(s);
	p = GetPSTAFF(pL);
	sprintf(s, "systemL=%d", p->systemL);
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;		/* since Master Page can have 0 staves */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextSTAFFL(qL))
		;
	q = GetPASTAFF(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%d", qL, q, q->staffn, q->next);
	DrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "vis=%s sel=%s", q->visible ? "TRUE" : "false",
										 q->selected ? "TRUE" : "false");
	DrawLine(s); q = GetPASTAFF(qL);
	
	SetRect(&objRect,q->staffLeft,q->staffTop,
						  systemRect.right-systemRect.left,q->staffTop+q->staffHeight);
	OffsetRect(&objRect,systemRect.left,systemRect.top);
	D2Rect((DRect *)(&objRect),&objRect);
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "staff top/left/right=%d %d %d",
				q->staffTop, q->staffLeft, q->staffRight);
	DrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "staffHeight/Lines=%d %d", q->staffHeight, q->staffLines);
	DrawLine(s); q = GetPASTAFF(qL);
	if (q->showLines==SHOW_ALL_LINES)
		sprintf(s, "showLines=all showLedgers=%s", q->showLedgers? "yes" : "no");
	else
		sprintf(s, "showLines=%d showLedgers=%s",
				q->showLines, q->showLedgers? "yes" : "no");
	DrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "fontSize=%d spaceBelow=%d", q->fontSize, q->spaceBelow);
	DrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "ledger/noteHead/fracBeam=%d %d %d",
				q->ledgerWidth,
				q->noteHeadWidth,
				q->fracBeamWidth);
	DrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "clefType=%d", q->clefType);
	DrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "nKSItems=%hd", q->nKSItems);
	DrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "timeSigType,n/d=%hd,%hd/%hd",
				q->timeSigType,
				q->numerator,
				q->denominator);
	DrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "dynamicType=%hd", q->dynamicType);
	DrawLine(s);
}


/* ------------------------------------------------------------ BrowseConnect -- */

void BrowseConnect(LINK pL, INT16 index)
{
	PCONNECT p;
	PACONNECT q;
	LINK qL;
	INT16 i;

	p = GetPCONNECT(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);
	
	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextCONNECTL(qL)) 
		;
	q = GetPACONNECT(qL);
	sprintf(s, "link=%u @%lx next=%d", qL, q, q->next);
	DrawLine(s);	q = GetPACONNECT(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	DrawLine(s); 	q = GetPACONNECT(qL);
	switch (q->connLevel) {
		case SystemLevel:
			DrawLine("connLevel=SystemLevel");
			break;
		case GroupLevel:
			DrawLine("connLevel=GroupLevel");
			break;
		case PartLevel:
			DrawLine("connLevel=PartLevel");
			break;
		default:
			DrawLine("connLevel=Unknown");
			break;
	}
	DrawLine(s); 	q = GetPACONNECT(qL);
	switch (q->connectType) {
		case CONNECTLINE:
			DrawLine("connectType=CONNECTLINE");
			break;
		case CONNECTCURLY:
			DrawLine("connectType=CONNECTCURLY");
			break;
		case CONNECTBRACKET:
			DrawLine("connectType=CONNECTBRACKET");
			break;
		default:
			DrawLine("connectType=Unknown");
			break;
	}
	
	sprintf(s, "staffAbove=%hd staffBelow=%hd", q->staffAbove, q->staffBelow);
	DrawLine(s); 	q = GetPACONNECT(qL);
	sprintf(s, "firstPart=%d lastPart=%d", q->firstPart,q->lastPart);
	DrawLine(s); 	q = GetPACONNECT(qL);
	sprintf(s, "xd=%d", q->xd);
	DrawLine(s);
	
	subL = qL;
}


/* -------------------------------------------------------------- BrowseClef -- */

void BrowseClef(LINK pL, INT16 index)
{
	PCLEF p;
	PACLEF q;
	LINK qL;
	INT16 i;

	p = GetPCLEF(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	if (p->inMeasure) sprintf(s, "In measure ");
	else					sprintf(s, "Not in measure ");
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);
	
	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextCLEFL(qL)) 
		;
	q = GetPACLEF(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%d", qL, q, q->staffn, q->next);
	DrawLine(s); 	q = GetPACLEF(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	DrawLine(s);  	q = GetPACLEF(qL);
	sprintf(s, "xd=%d yd=%d", q->xd, q->yd);
	DrawLine(s); 	q = GetPACLEF(qL);
	sprintf(s, "clefType=%d small=%d", q->subType, q->small);

	DrawLine(s);
	
	subL = qL;
}


/* -------------------------------------------------------------- BrowseKeySig -- */

void BrowseKeySig(LINK pL, INT16 index)
{
	PKEYSIG p;
	PAKEYSIG q;
	LINK qL;
	INT16 i;

	p = GetPKEYSIG(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	if (p->inMeasure) sprintf(s, "In measure ");
	else					sprintf(s, "Not in measure ");
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextKEYSIGL(qL)) 
		;
	q = GetPAKEYSIG(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%d", qL, q, q->staffn, q->next);
	DrawLine(s);	q = GetPAKEYSIG(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	DrawLine(s); 	q = GetPAKEYSIG(qL);
	sprintf(s, "xd=%d", q->xd);
	DrawLine(s);	q = GetPAKEYSIG(qL);
	sprintf(s, "nKSItems=%hd subType(nNats)=%d", q->nKSItems, q->subType);
	DrawLine(s);
	
	subL = qL;
}


/* ------------------------------------------------------------ BrowseTimeSig -- */

void BrowseTimeSig(LINK pL, INT16 index)
{
	PTIMESIG p;
	PATIMESIG q;
	LINK qL;
	INT16 i;

	p = GetPTIMESIG(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	if (p->inMeasure) sprintf(s, "In measure ");
	else					sprintf(s, "Not in measure ");
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextTIMESIGL(qL)) 
		;
	q = GetPATIMESIG(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%d", qL, q, q->staffn, q->next);
	DrawLine(s);	q = GetPATIMESIG(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	DrawLine(s);	q = GetPATIMESIG(qL);
	sprintf(s, "xd=%d yd=%d", q->xd, q->yd);
	DrawLine(s);	q = GetPATIMESIG(qL);
	sprintf(s, "timeSigType,n/d=%hd,%hd/%hd",
				q->subType,
				q->numerator,
				q->denominator);
	DrawLine(s);
	
	subL = qL;
}


/* ------------------------------------------------------------- BrowseMeasure -- */

void BrowseMeasure(LINK pL, INT16 index)
{
	PMEASURE p;
	PAMEASURE q;
	LINK qL;
	INT16 i, xd;
	Rect bbox; DRect mrect;

	p = GetPMEASURE(pL);
	sprintf(s, "l/r Measure=%d,%d", p->lMeasure, p->rMeasure);
	DrawLine(s);	p = GetPMEASURE(pL);
	sprintf(s, "systemL=%d staffL=%d", p->systemL, p->staffL);
	DrawLine(s);	p = GetPMEASURE(pL);
	sprintf(s, "fake=%s spPercent=%d", (p->fakeMeas ? "TRUE" : "false"), p->spacePercent);
	DrawLine(s);	p = GetPMEASURE(pL);
	bbox = p->measureBBox;
	sprintf(s, "measureBBox=%d %d %d %d",
			p->measureBBox.top,
			p->measureBBox.left,
			p->measureBBox.bottom,
			p->measureBBox.right);
	DrawLine(s);
	sprintf(s, "lTimeStamp=%ld", p->lTimeStamp);
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);
	xd = p->xd;

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextMEASUREL(qL)) 
		;
	q = GetPAMEASURE(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%d", qL, q, q->staffn, q->next);
	DrawLine(s);	q = GetPAMEASURE(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	if (q->measureVisible) strcat(s, "MEASUREVIS ");
	DrawLine(s);	q = GetPAMEASURE(qL);
	mrect = q->measureRect;
	sprintf(s, "measureRect=%d %d %d %d ",
			q->measureRect.top,
			q->measureRect.left,
			q->measureRect.bottom,
			q->measureRect.right);
	DrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "measureNum=%hd xMNSOff=%hd yMNSOff=%hd", q->measureNum,
				q->xMNStdOffset, q->yMNStdOffset);
	DrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "barlineType=%hd connStaff=%hd", q->subType, q->connStaff);
	if (q->connAbove) strcat(s, " CONNABOVE");
	DrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "clefType=%d", q->clefType);
	DrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "nKSItems=%hd", q->nKSItems);
	DrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "timeSigType,n/d=%hd,%hd/%hd",
			q->timeSigType,
			q->numerator,
			q->denominator);
	DrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "dynamicType=%hd", q->dynamicType);
	DrawLine(s);
	
	OffsetDRect(&mrect,xd,systemRect.top);
	SetRect(&objRect,mrect.left,mrect.top,mrect.right,mrect.bottom);
	OffsetRect(&objRect,systemRect.left,0);
	D2Rect((DRect *)&objRect,&objRect);
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	subL = qL;
}


/* ---------------------------------------------------------- BrowsePseudoMeas -- */

void BrowsePseudoMeas(LINK pL, INT16 index)
{
	PPSMEAS p;
	PAPSMEAS q;
	LINK qL;
	INT16 i, xd;

	p = GetPPSMEAS(pL);
	xd = p->xd;
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextPSMEASL(qL)) 
		;
	q = GetPAPSMEAS(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%d", qL, q, q->staffn, q->next);
	DrawLine(s);	q = GetPAPSMEAS(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	DrawLine(s);	q = GetPAPSMEAS(qL);
	sprintf(s, "barlineType=%hd connStaff=%hd", q->subType, q->connStaff);
	if (q->connAbove) strcat(s, " CONNABOVE");
	DrawLine(s);
}


/* ---------------------------------------------------------------- BrowseSync -- */

void BrowseSync(LINK pL, INT16 index)
{
	PSYNC p;
	PANOTE q;
	LINK qL;
	INT16 i;

	p = GetPSYNC(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "timeStamp=%u (onset time=%ld)", SyncTIME(pL), SyncAbsTime(pL));
	
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextNOTEL(qL)) 
		;
	q = GetPANOTE(qL);
	sprintf(s, "link=%u @%lx stf=%d v=%hd next=%d", qL, q,
				 q->staffn, q->voice, q->next);
	DrawLine(s);	q = GetPANOTE(qL);
	
	strcpy(s, "flags=");
	if (q->selected)			strcat(s, "SELECTED ");
	if (q->visible)			strcat(s, "VISIBLE ");
	if (q->soft)				strcat(s, "SOFT ");
	if (q->inChord)			strcat(s, "INCHORD");
	DrawLine(s);	q = GetPANOTE(qL);
	
	strcpy(s, "flags=");
	if (q->rest)				strcat(s, "REST ");
	if (q->beamed)				strcat(s, "BEAMED ");
	if (q->otherStemSide)	strcat(s, "OTHERSTEMSIDE");
	DrawLine(s);	q = GetPANOTE(qL);
	
	strcpy(s, "flags=");
	if (q->tiedL)			strcat(s, "TIEDL ");
	if (q->tiedR)			strcat(s, "TIEDR ");
	if (q->slurredL)			strcat(s, "SLURREDL ");
	if (q->slurredR)			strcat(s, "SLURREDR");
	DrawLine(s);	q = GetPANOTE(qL);

	strcpy(s, "flags=");
	if (q->inTuplet)			strcat(s, "INTUPLET ");
	if (q->inOctava)			strcat(s, "INOCTAVA ");
	if (q->small)				strcat(s, "SMALL ");
	if (q->tempFlag)			strcat(s, "TEMPFLAG");
	DrawLine(s);	q = GetPANOTE(qL);
	
	sprintf(s, "xd=%d yd=%d yqpit=%hd ystem=%d", q->xd, q->yd, q->yqpit, q->ystem);
	DrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "l_dur=%hd ndots=%hd xmovedots=%hd ymovedots=%hd", q->subType, q->ndots,
					q->xmovedots, q->ymovedots);
	DrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "headShape=%d firstMod=%d", q->headShape, q->firstMod);
	DrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "accident=%hd accSoft=%s xmoveAcc=%hd", q->accident,
														q->accSoft ? "TRUE" : "false",
														q->xmoveAcc);
	DrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "playTDelta=%d playDur=%d noteNum=%hd", q->playTimeDelta, q->playDur,
														q->noteNum);
	DrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "on,offVelocity=%hd,%hd", q->onVelocity, q->offVelocity);
	DrawLine(s);
	
	subL = qL;
}


/* ----------------------------------------------------------- BrowseGRSync -- */

void BrowseGRSync(LINK pL, INT16 index)
{
	PGRSYNC p;
	PAGRNOTE	q;
	LINK qL;
	INT16 i;

	p = GetPGRSYNC(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextGRNOTEL(qL)) 
		;
	q = GetPAGRNOTE(qL);
	sprintf(s, "link=%u @%lx stf=%d v=%hd next=%d", qL, q,
				 q->staffn, q->voice, q->next);
	DrawLine(s);	q = GetPAGRNOTE(qL);

	
	strcpy(s, "flags=");
	if (q->selected)			strcat(s, "SELECTED ");
	if (q->visible)			strcat(s, "VISIBLE ");
	if (q->soft)				strcat(s, "SOFT ");
	if (q->inChord)			strcat(s, "INCHORD");
	DrawLine(s);	q = GetPAGRNOTE(qL);
	
	strcpy(s, "flags=");
	if (q->rest)				strcat(s, "REST ");
	if (q->beamed)				strcat(s, "BEAMED ");
	if (q->otherStemSide)	strcat(s, "OTHERSTEMSIDE");
	DrawLine(s);	q = GetPAGRNOTE(qL);
	
	strcpy(s, "flags=");
	if (q->tiedL)			strcat(s, "TIEDL ");
	if (q->tiedR)			strcat(s, "TIEDR ");
	if (q->slurredL)			strcat(s, "SLURREDL ");
	if (q->slurredR)			strcat(s, "SLURREDR");
	DrawLine(s);	q = GetPAGRNOTE(qL);

	strcpy(s, "flags=");
	if (q->inTuplet)			strcat(s, "INTUPLET ");
	if (q->inOctava)			strcat(s, "INOCTAVA ");
	if (q->small)				strcat(s, "SMALL");
	DrawLine(s);	q = GetPAGRNOTE(qL);
	
	sprintf(s, "xd=%d yd=%d yqpit=%hd ystem=%d", q->xd, q->yd, q->yqpit, q->ystem);
	DrawLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "l_dur=%hd ndots=%hd xmovedots=%hd ymovedots=%hd", q->subType, q->ndots,
					q->xmovedots, q->ymovedots);
	DrawLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "headShape=%d firstMod=%d", q->headShape, q->firstMod);
	DrawLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "accident=%hd accSoft=%s xmoveAcc=%hd", q->accident,
														q->accSoft ? "TRUE" : "false",
														q->xmoveAcc);
	DrawLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "playTDelta=%d playDur=%d noteNum=%hd", q->playTimeDelta, q->playDur,
														q->noteNum);
	DrawLine(s);	q = GetPAGRNOTE(qL);
	sprintf(s, "on,offVelocity=%hd,%hd", q->onVelocity, q->offVelocity);
	DrawLine(s);
}


/* ----------------------------------------------------------- BrowseBeamset -- */

void BrowseBeamset(LINK pL, INT16 index)
{
	PBEAMSET	p;
	PANOTEBEAM	q;
	LINK qL;
	INT16 i;

	p = GetPBEAMSET(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "link=%u stf=%d voice=%d", pL, p->staffn, p->voice);
	DrawLine(s);
	sprintf(s, "thin=%d beamRests=%d feather=%d grace=%d", p->thin, p->beamRests,
					p->feather, p->grace);
	DrawLine(s);
	sprintf(s, "firstSystem=%d crossStf=%d crossSys=%d",
					p->firstSystem, p->crossStaff, p->crossSystem);
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextNOTEBEAML(qL)) 
		;
	q = GetPANOTEBEAM(qL);
	sprintf(s, "link=%u @%lx bpSync=%d next=%d", qL, q, q->bpSync, q->next);
	DrawLine(s);	q = GetPANOTEBEAM(qL);
	sprintf(s, "startend=%d", q->startend);
	DrawLine(s);	q = GetPANOTEBEAM(qL);
	sprintf(s, "fracs=%d", q->fracs);
	DrawLine(s);	q = GetPANOTEBEAM(qL);
	sprintf(s, "fracGoLeft=%s", q->fracGoLeft ? "TRUE" : "false");
	DrawLine(s);
}


/* --------------------------------------------------------------- BrowseTuplet -- */

void BrowseTuplet(LINK pL, INT16 index)
{
	PTUPLET p;
	PANOTETUPLE	q;
	LINK qL;
	INT16 i;

	p = GetPTUPLET(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "staff=%d voice=%d num=%d denom=%d", p->staffn, p->voice,
		p->accNum, p->accDenom);
	DrawLine(s);	p = GetPTUPLET(pL);
	sprintf(s, "numVis=%d denomVis=%d brackVis=%d", p->numVis, p->denomVis,
		p->brackVis);
	DrawLine(s);	p = GetPTUPLET(pL);
	sprintf(s, "acnxd=%d acnyd=%d", p->acnxd, p->acnyd);
	DrawLine(s);	p = GetPTUPLET(pL);
	sprintf(s, "xd1st=%d yd1st=%d xdLast=%d ydLast=%d", p->xdFirst, p->ydFirst,
				p->xdLast, p->ydLast);
	DrawLine(s);	p = GetPTUPLET(pL);
	sprintf(s, "link=%u stf=%d", pL, p->staffn);
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextNOTETUPLEL(qL)) 
		;
	q = GetPANOTETUPLE(qL);
	sprintf(s, "link=%u @%lx tpSync=%d next=%d", qL, q, q->tpSync, q->next);
	DrawLine(s);	q = GetPANOTETUPLE(qL);
}


/* -------------------------------------------------------------- BrowseOctava -- */

void BrowseOctava(LINK pL, INT16 index)
{
	POCTAVA p;
	PANOTEOCTAVA q;
	LINK qL;
	INT16 i;

	p = GetPOCTAVA(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "staff=%d", p->staffn);
	DrawLine(s);	p = GetPOCTAVA(pL);
	sprintf(s, "nxd=%d nyd=%d", p->nxd, p->nyd);
	DrawLine(s);	p = GetPOCTAVA(pL);
	sprintf(s, "xd1st=%d yd1st=%d xdLast=%d ydLast=%d", p->xdFirst, p->ydFirst,
				p->xdLast, p->ydLast);
	DrawLine(s);	p = GetPOCTAVA(pL);
	sprintf(s, "octSignType=%d noCutoff=%d", p->octSignType, p->noCutoff);
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextNOTEOCTAVAL(qL)) 
		;
	q = GetPANOTEOCTAVA(qL);
	sprintf(s, "link=%u @%lx opSync=%d next=%d", qL, q, q->opSync, q->next);
	DrawLine(s);
}

/* ----------------------------------------------------------- BrowseDynamic -- */

void BrowseDynamic(LINK pL, INT16 index)
{
	PDYNAMIC p;
	PADYNAMIC q;
	LINK qL;
	INT16 i;

	p = GetPDYNAMIC(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	DrawLine("");
	sprintf(s, "firstSyncL=%d lastSyncL=%d",
					DynamFIRSTSYNC(pL), DynamLASTSYNC(pL));
	DrawLine(s);
	sprintf(s, "dynamicType=%d IsHairpin=%d", DynamType(pL),IsHairpin(pL));
	DrawLine(s);
	p = GetPDYNAMIC(pL);
	sprintf(s, "crossSys=%d ", p->crossSys);
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL); i<index; i++,qL=NextDYNAMICL(qL)) 
		;
	q = GetPADYNAMIC(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%d", qL, q, q->staffn, q->next);
	DrawLine(s);	q = GetPADYNAMIC(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	DrawLine(s);	q = GetPADYNAMIC(qL);
	sprintf(s, "mouthWidth=%d otherWidth=%d", q->mouthWidth, q->otherWidth);
	DrawLine(s);	q = GetPADYNAMIC(qL);
	sprintf(s, "small=%d", q->small);
	DrawLine(s);	q = GetPADYNAMIC(qL);
	sprintf(s, "xd,yd=%d,%d endxd,endyd=%d,%d", q->xd, q->yd, q->endxd, q->endyd);
	DrawLine(s);
	
	subL = qL;
}


/* ----------------------------------------------------------- BrowseRptEnd -- */

void BrowseRptEnd(LINK pL, INT16 index)
{
	PRPTEND p;
	PARPTEND q;
	LINK qL;
	INT16 i;

	p = GetPRPTEND(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "rptEndType=%hd", p->subType);
	DrawLine(s);	p = GetPRPTEND(pL);
	sprintf(s, "firstObj=%d startRpt=%d endRpt=%d", p->firstObj,
					p->startRpt, p->endRpt);
	DrawLine(s);
	sprintf(s, "count=%d", p->count);
	DrawLine(s);
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);

	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,qL=FirstSubLINK(pL);i<index;i++,qL=NextRPTENDL(qL)) 
		;
	q = GetPARPTEND(qL);
	sprintf(s, "link=%u @%lx stf=%d next=%d", qL, q, q->staffn, q->next);
	DrawLine(s);	q = GetPARPTEND(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	DrawLine(s);	q = GetPARPTEND(qL);
	sprintf(s, "rptEndType=%hd", q->subType);
	DrawLine(s);
}


/* ---------------------------------------------------------------- BrowseEnding -- */

void BrowseEnding(LINK pL, INT16 /*index*/)
{
	PENDING p;

	p = GetPENDING(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "stf=%d", p->staffn);
	DrawLine(s);

	sprintf(s, "firstObjL=%d lastObjL=%d", p->firstObjL, p->lastObjL);
	DrawLine(s);

	sprintf(s, "endNum=%d noL=%d noR=%d", p->endNum, p->noLCutoff, p->noRCutoff);
	DrawLine(s);

	sprintf(s, "xd=%d yd=%d endxd=%d", p->xd, p->yd, p->endxd);
	DrawLine(s);
}


/* -------------------------------------------------------------- ChordSym2Print -- */
/* Convert in place a string representing a chord symbol to (semi-)readable form
by replacing all delimiters with another character. */

#define DELIMITER FWDDEL_KEY
#define CH_SUBST '|'

void ChordSym2Print(unsigned char *);
void ChordSym2Print(unsigned char *str)					/* Pascal string */
{
	short i, count=*str;
	
	for (i = 1; i<=count; i++)
		if (str[i]==DELIMITER) str[i] = CH_SUBST;
}

/* --------------------------------------------------------------- BrowseGraphic -- */

void BrowseGraphic(LINK pL)
{
	PGRAPHIC p;
	char	*pStr, s2[256];
	LINK aGraphicL;
	PAGRAPHIC aGraphic;

	p = GetPGRAPHIC(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "stf=%d voice=%d", p->staffn, p->voice);
	DrawLine (s);
	p = GetPGRAPHIC(pL);
	switch (p->graphicType) {
		case GRPICT:
			pStr = "GRPICT";
			break;
		case GRChar:
			pStr = "GRChar";
			break;
		case GRString:
			pStr = "GRString";
			break;
		case GRLyric:
			pStr = "GRLyric";
			break;
		case GRDraw:
			pStr = "GRDraw";
			break;
		case GRRehearsal:
			pStr = "GRRehearsal";
			break;
		case GRChordSym:
			pStr = "GRChordSym";
			break;
		case GRArpeggio:
			pStr = "GRArpeggio";
			break;
		case GRChordFrame:
			pStr = "GRChordFrame";
			break;
		case GRMIDIPatch:
			pStr = "GRMIDIPatch";
			break;
		default:
			sprintf(s2, "UNKNOWN TYPE %d", p->graphicType);
			pStr = s2;
	}
	sprintf(s, "graphicType=%s", pStr);
	DrawLine (s);

	p = GetPGRAPHIC(pL);
	aGraphicL = FirstSubLINK(pL);
	aGraphic = GetPAGRAPHIC(aGraphicL);
	sprintf(s, "aGraphic->next=%d", aGraphic->next);
	DrawLine (s);	p = GetPGRAPHIC(pL);

#ifdef NOTYET
	strcpy(s, "");
	if (p->vConstrain) strcat(s, "VCONSTRAIN ");
	if (p->hConstrain) strcat(s, "HCONSTRAIN ");
	DrawLine(s);	p = GetPGRAPHIC(pL);
	switch (p->justify) {
		case GRJustLeft:
			pStr = "JUSTIFYLEFT";
			break;
		case GRJustRight:
			pStr = "JUSTIFYRIGHT";
			break;
		case GRJustBoth:
			pStr = "JUSTIFYBOTH";
			break;
		case GRJustCenter:
			pStr = "JUSTIFYCENTER";
			break;
		default:
			sprintf(s2, "UNKNOWN JUSTIFY TYPE %d", p->justify);
			pStr = s2;
	}
	sprintf(s, "justify=%s", pStr);
	DrawLine(s);	p = GetPGRAPHIC(pL);
	sprintf(s, "handle=0x%lx", p->handle);
	DrawLine(s);	p = GetPGRAPHIC(pL);
#endif

	sprintf(s, "firstObj=%d lastObj=%d", p->firstObj, p->lastObj);
	DrawLine(s);	p = GetPGRAPHIC(pL);
	sprintf(s, "info=%d", p->info);
	DrawLine(s);	p = GetPGRAPHIC(pL);
	switch (p->graphicType) {
		case GRString:
		case GRLyric:
		case GRRehearsal:
		case GRChordSym:
		case GRChordFrame:
		case GRMIDIPatch:
		case GRMIDISustainOn:
		case GRMIDISustainOff:		
			sprintf(s, "fontInd=%d fontStyle=%d encl=%d", p->fontInd, p->fontStyle,
													p->enclosure);
			DrawLine(s);	p = GetPGRAPHIC(pL);
			sprintf(s, "fontSize=%d %s", p->fontSize,
												  p->relFSize? "staff-rel." : "points");
			DrawLine (s);	aGraphic = GetPAGRAPHIC(aGraphicL);

			sprintf(s, "aGraphic->string=%ld ", aGraphic->string);
			if (PCopy(aGraphic->string)==NULL) {
				DrawLine(s);
				sprintf(s, "** PCopy(aGraphic->string) is NULL **");
				DrawLine(s);			
			}
			else {
				Pstrcpy((unsigned char *)s2, PCopy(aGraphic->string));
				sprintf(&s[strlen(s)], "char count=%d", (unsigned char)s2[0]);
				DrawLine(s);			
				if (p->graphicType==GRChordSym)
					ChordSym2Print((unsigned char *)s2);
				sprintf(s, "str='%s'", PToCString((unsigned char *)s2));
				DrawLine(s);
			}

			break;
		case GRArpeggio:
			sprintf(s, "arpInfo=%d", ARPINFO(p->info2));
			DrawLine(s);
			break;
		case GRDraw:
			sprintf(s, "info2=%d thick=%d", p->info2, p->gu.thickness);
			DrawLine(s);
			break;
		default:
			;
	}
}


/* ------------------------------------------------------------- BrowseTempo -- */

void BrowseTempo(LINK pL)
{
	PTEMPO p;
	char t[256];
	
	p = GetPTEMPO(pL);

	sprintf(s, "stf=%d", p->staffn);
	DrawLine(s); p = GetPTEMPO(pL);

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
		case NO_L_DUR:
			strcpy(t, "Unknown Duration"); break;
	}
	sprintf(s, "subType=%d (%s) dotted=%d", p->subType, t, p->dotted);
	DrawLine(s); p = GetPTEMPO(pL);

	sprintf(s, "hideMM=%d", p->hideMM);
	DrawLine(s); p = GetPTEMPO(pL);

	sprintf(s, "firstObjL=%d tempo=%d", p->firstObjL, p->tempo);
	DrawLine(s); p = GetPTEMPO(pL);

	sprintf(s, "p->string=%ld ", p->string);
	if (PCopy(p->string)==NULL) {
		DrawLine(s);			
		sprintf(s, "** PCopy(p->string) is NULL **");
		DrawLine(s);			
	}
	else {
		Pstrcpy((unsigned char *)t, PCopy(p->string));
		sprintf(&s[strlen(s)], "char count=%d", (unsigned char)t[0]);
		DrawLine(s);			
		sprintf(s, "str='%s'", PToCString((unsigned char *)t));
		DrawLine(s);
	}

	sprintf(s, "p->metroStr=%ld ", p->metroStr);
	if (PCopy(p->metroStr)==NULL) {
		DrawLine(s);			
		sprintf(s, "** PCopy(p->metroStr) is NULL **");
		DrawLine(s);			
	}
	else {
		Pstrcpy((unsigned char *)t, PCopy(p->metroStr));
		sprintf(&s[strlen(s)], "char count=%d", (unsigned char)t[0]);
		DrawLine(s);
		sprintf(s, "str='%s'", PToCString((unsigned char *)t));
		DrawLine(s);
	}
}

/* --------------------------------------------------------------- BrowseSpace -- */

void BrowseSpace(LINK pL)
{
	PSPACE p;

	p = GetPSPACE(pL);
	sprintf(s, "stf=%d spWidth=%d", p->staffn,p->spWidth);
	DrawLine(s); p = GetPSPACE(pL);
	sprintf(s, "bottomStf=%d", p->bottomStaff);
	DrawLine(s);
}

/* ---------------------------------------------------------------- BrowseSlur -- */

void BrowseSlur(LINK pL, INT16 index)
{
	PSLUR p;
	PASLUR aSlur;
	LINK	aSlurL;
	INT16 i;
	
	p = GetPSLUR(pL);
	objRect = p->objRect;
	OffsetRect(&objRect,paperRect.left,paperRect.top);
	
	sprintf(s, "stf=%d voice=%d", p->staffn, p->voice);
	DrawLine(s);	p = GetPSLUR(pL);
	sprintf(s, "tie=%s", p->tie ? "TRUE" : "false");
	DrawLine(s);	p = GetPSLUR(pL);
	sprintf(s, "crossStf=%d crossStfBack=%d crossSys=%d",
		p->crossStaff,p->crossStfBack,p->crossSystem);
	DrawLine(s);	p = GetPSLUR(pL);

	sprintf(s, "firstSync=%d", SlurFIRSTSYNC(pL));
	if (!SyncTYPE(SlurFIRSTSYNC(pL)))
		sprintf(&s[strlen(s)], "(%s)", (MeasureTYPE(SlurFIRSTSYNC(pL))? "MEAS" : "TYPE?"));
	sprintf(&s[strlen(s)], " lastSync=%d", SlurLASTSYNC(pL));
	if (!SyncTYPE(SlurLASTSYNC(pL)))
		sprintf(&s[strlen(s)],  "(%s)", (SystemTYPE(SlurLASTSYNC(pL))? "SYS" : "TYPE?"));

	DrawLine(s);
	
	sprintf(s, "---------- %d of %d ----------", index+1, LinkNENTRIES(pL));
	DrawLine(s);
	
	if (index+1>LinkNENTRIES(pL)) return;			/* should never happen */

	for (i=0,aSlurL=FirstSubLINK(pL);i<index;i++,aSlurL=NextSLURL(aSlurL)) 
		;
	aSlur = GetPASLUR(aSlurL);

	strcpy(s, "flags=");
	if (aSlur->selected) strcat(s, "SELECTED ");
	if (aSlur->visible)	strcat(s, "VISIBLE ");
	if (aSlur->soft)		strcat(s, "SOFT ");
	if (aSlur->dashed)	strcat(s, "DASHED ");
	DrawLine(s); 	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "firstInd=%d lastInd=%d", aSlur->firstInd, aSlur->lastInd);
	DrawLine(s); 	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "knot=%d,%d", aSlur->seg.knot.h, aSlur->seg.knot.v);
	DrawLine(s);	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "c0=%d,%d", aSlur->seg.c0.h, aSlur->seg.c0.v);
	DrawLine(s);	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "c1=%d,%d", aSlur->seg.c1.h, aSlur->seg.c1.v);
	DrawLine(s);	aSlur = GetPASLUR(aSlurL);
	sprintf(s, "endpoint=%d,%d", aSlur->endpoint.h, aSlur->endpoint.v);
	DrawLine(s);
	sprintf(s, "startPt=%d,%d endPt=%d,%d", aSlur->startPt.h, aSlur->startPt.v,
				aSlur->endPt.h, aSlur->endPt.v);
	DrawLine(s);
	
	subL = aSlurL;
}


/* ------------------------------------------------------------- ShowContext -- */

#define iText 2

void ShowContext(Document *doc)
{
	LINK			pL;
	Boolean		done;
	CONTEXT		context;
	DialogPtr	dlog;
	Handle		tHdl;
	short			itype,ditem;
	INT16			theStaff;
	GrafPtr		oldPort;	

/* Get LINK to and staff number of first selected object or of insertion point. */

	if (doc->selStartL==doc->selEndL) {
		pL = doc->selStartL;
		theStaff = doc->selStaff;
	}
	else {
		theStaff = GetStaffFromSel(doc, &pL);
		if (theStaff==NOONE) {
			SysBeep(4);
			return;
		}
	}
	
	GetContext(doc, pL, theStaff, &context);

	GetPort(&oldPort);
	dlog = GetNewDialog(CONTEXT_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog)
		MissingDialog(CONTEXT_DLOG);
	SetPort(GetDialogWindowPort(dlog));
	ShowWindow(GetDialogWindow(dlog));

	GetDialogItem(dlog, iText, &itype, &tHdl, &bRect);

	done = FALSE;
	TextFont(SYSFONTID_MONOSPACED);
	TextSize(9);
	EraseRect(&bRect);
	linenum = 1;

	sprintf(s, "doc->selStaff=%hd", doc->selStaff);
	DrawLine(s);
	sprintf(s, "");
	DrawLine(s);
	sprintf(s, "Context at link %hd for staff %hd:", pL, theStaff);
	DrawLine(s);
	sprintf(s, "");
	DrawLine(s);
	strcpy(s, "flags=");
	if (context.visible)
		strcat(s, "VISIBLE ");
	if (context.staffVisible)
		strcat(s, "STAFFVIS ");
	if (context.measureVisible)
		strcat(s, "MEASUREVIS  ");
	DrawLine(s);
	strcpy(s, "flags=");
	if (context.inMeasure)
		strcat(s, "IN MEASURE ");
	else
		strcat(s, "NOT IN MEASURE ");
	DrawLine(s);
	sprintf(s, "systemTop,Left,Bottom=%d,%d,%d",
		context.systemTop,
		context.systemLeft,
		context.systemBottom);
	DrawLine(s);
	sprintf(s, "staffTop,Left,Right=%d,%d,%d",
		context.staffTop,
		context.staffLeft,
		context.staffRight);
	DrawLine(s);
	sprintf(s, "staffHeight,HalfHeight=%d,%d",
		context.staffHeight,
		context.staffHalfHeight);
	DrawLine(s);
	if (context.showLines==SHOW_ALL_LINES)
		sprintf(s, "staffLines=%hd showLines=all",
			context.staffLines);
	else
		sprintf(s, "staffLines=%hd showLines=%hd",
			context.staffLines, context.showLines);
	DrawLine(s);
	sprintf(s, "showLedgers=%s fontSize=%d",
			context.showLedgers? "yes" : "no", context.fontSize);
	DrawLine(s);
	sprintf(s, "measureTop,Left=%d,%d",
		context.measureTop,
		context.measureLeft);
	DrawLine(s);
	sprintf(s, "clefType=%hd",
		context.clefType);
	DrawLine(s);
	sprintf(s, "nKSItems=%hd",
		context.nKSItems);
	DrawLine(s);
	sprintf(s, "timeSigType,n/d=%hd,%hd/%hd",
		context.timeSigType,
		context.numerator,
		context.denominator);
	DrawLine(s);
	sprintf(s, "dynamicType=%hd", context.dynamicType);
	DrawLine(s);

	done = FALSE;
	do {
		ModalDialog(NULL, &ditem);					/* Handle dialog events */
		if (ditem==OK)
		  	done = TRUE;
	} while (!done);
	DisposeDialog(dlog);
	SetPort(oldPort);
}

//#endif /* PUBLIC_VERSION */
