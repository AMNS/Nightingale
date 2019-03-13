/******************************************************************************************
*	FILE:	DialogsBoth.c
*	PROJ:	Nightingale
*	DESC:	Dialog-handling routines for misc. dialogs needed in "viewer version"
*			("viewer version" removed by chirgwin at Mon Jun 25 15:57:30 PDT 2012)
*			as well as the full version.
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
	LookAtDialog				GoToDialog
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* -------------------------------------- Declarations for simple "set number" Dialogs -- */

#define NUMBER_DI 	3			/* DITL index of number to be adjusted */
#define UpRect_DI	5			/* DITL index of up button rect */
#define DownRect_DI	6			/* DITL index of down button rect */

extern short minDlogVal, maxDlogVal;


/* ---------------------------------------------------------------------- LookAtDialog -- */
/* Conduct dialog to get (part-relative user, not internal) voice number to look
at.  Returns new voice number or CANCEL_INT for Cancel. */

static enum
{
	PARTNAME_DI=9,
	NEW_DI
} E_LookAtItems;


short LookAtDialog(Document *doc, short initVoice, LINK partL)
{	
	DialogPtr		dlog;
	short			ditem, itype;
	short			voice, oldVoice;
	Handle			newHdl;
	Rect			tRect;
	GrafPtr			oldPort;
	PPARTINFO		pPart;
	char			partName[256];
	char			fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(NumberFilter);
	if (filterUPP == NULL) {
		MissingDialog(LOOK_DLOG);
		return CANCEL_INT;
	}
	
	voice = CANCEL_INT;
	GetPort(&oldPort);
	
	dlog = GetNewDialog(LOOK_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		
		SetDialogDefaultItem(dlog,OK);
	
		UseNumberFilter(dlog,NUMBER_DI,UpRect_DI,DownRect_DI);

		voice = (initVoice<0? 1 : initVoice);
		PutDlgWord(dlog,NUMBER_DI,voice,True);
		pPart = GetPPARTINFO(partL);
		strcpy(partName, (strlen(pPart->name)>14? pPart->shortName : pPart->name));
		PutDlgString(dlog,PARTNAME_DI,CToPString(partName), False);
		GetDialogItem(dlog, NEW_DI, &itype, &newHdl, &tRect);
	
		CenterWindow(GetDialogWindow(dlog), 70);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		minDlogVal = 1;
		maxDlogVal = n_min(MAXVOICES, 31);		/* bcs VOICEINFO's relVoice is a 5-bit field */
		oldVoice = voice;

		do {
			while (True) {
				ModalDialog(filterUPP, &ditem);
				if (ditem==OK || ditem==Cancel) break;
				if (ditem==NEW_DI) {
					voice = NewVoiceNum(doc, partL);
					PutDlgWord(dlog,NUMBER_DI,voice,True);
				}
			}
				
			if (ditem==Cancel) { voice = CANCEL_INT; break; }
			GetDlgWord(dlog,NUMBER_DI,&voice);
			if (voice<1 || voice>maxDlogVal) {
				GetIndCString(fmtStr, DIALOGERRS_STRS, 8);			/* "Voice number must be..." */
				sprintf(strBuf, fmtStr, maxDlogVal);
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				voice = -99;
			}
		} while (voice<1 || voice>MAXVOICES);
	
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
	}
	 else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(LOOK_DLOG);
		voice = CANCEL_INT;
	}
		
	SetPort(oldPort);
	return voice;
}


/* ------------------------------------------------------------------ GoToDialog et al -- */

static enum
{
	GOTOSTXT_DI=7,
	GOTOPOP_DI,
	GOTORANGE_DI
} E_GoToItems;

#define ANYMARK "\p"
#define RMARKSTR(rMark)		( PCopy(GetPAGRAPHIC(FirstSubLINK(rMark))->strOffset) )

static UserPopUp gotoPopUp8;				/* popup8 is for TransposeDialog ??WHAT? */
static short currPage, currMeas;
static LINK firstMark, lastMark;
static Document *gotoDoc;

static void RMClickUp(short, DialogPtr);
static void RMClickDown(short, DialogPtr);
static Boolean RMHandleKeyDown(EventRecord *, LINK, LINK, DialogPtr);
static Boolean RMHandleMouseDown(EventRecord *, LINK, LINK, DialogPtr);
static pascal Boolean GoToFilter(DialogPtr, EventRecord *, short *);


static void RMClickUp(
					short /*lastMark*/,		/* unused */
					DialogPtr theDialog
					)
{
	Str63 markStr;
	LINK theMark, nextMark;

	GetDlgString(theDialog,NUMBER_DI,(unsigned char *)markStr);
	theMark = RMSearch(gotoDoc, gotoDoc->headL, markStr, False);
	if (theMark) {
		nextMark = RMSearch(gotoDoc, RightLINK(theMark), ANYMARK, False);
		if (nextMark)
			PutDlgString(theDialog,NUMBER_DI,RMARKSTR(nextMark),True);
	}
}


static void RMClickDown(
					short /*lastMark*/,		/* unused */
					DialogPtr theDialog
					)
{
	Str63 markStr;
	LINK theMark, prevMark;
	
	GetDlgString(theDialog,NUMBER_DI,markStr);
	theMark = RMSearch(gotoDoc, gotoDoc->headL, markStr, False);
	if (theMark) {
		prevMark = RMSearch(gotoDoc, LeftLINK(theMark), ANYMARK, True);
		if (prevMark)
			PutDlgString(theDialog,NUMBER_DI,RMARKSTR(prevMark),True);
	}
}


static Boolean RMHandleKeyDown(EventRecord *theEvent, LINK firstMark, LINK lastMark,
											DialogPtr theDialog)
{
	char ch;

	ch = theEvent->message & charCodeMask;
	if (ch==UPARROWKEY) {
		RMClickUp(lastMark, theDialog);
		return True;
	}
	else if (ch==DOWNARROWKEY) {
		RMClickDown(firstMark, theDialog);
		return True;
	}
	else
		return False;
}


static Rect upRect, downRect;

static Boolean RMHandleMouseDown(EventRecord *theEvent, LINK firstMark, LINK lastMark,
									DialogPtr theDialog)
{
	Point	where;

	where = theEvent->where;
	GlobalToLocal(&where);
	if (PtInRect(where, &upRect)) {
		SelectDialogItemText(theDialog, NUMBER_DI, 0, ENDTEXT);			/* Select & unhilite number */
		TrackNumberArrow(&upRect, &RMClickUp, lastMark, theDialog);
		return True;
	}
	else if (PtInRect(where, &downRect)) {
		SelectDialogItemText(theDialog, NUMBER_DI, 0, ENDTEXT);			/* Select & unhilite number */
		TrackNumberArrow(&downRect, &RMClickDown, firstMark, theDialog);
		return True;
	}
	else
		return False;
}


static pascal Boolean GoToFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Point where;
	Boolean ans=False;
	WindowPtr w;
	short choice;

	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetWindowPort(w));
				BeginUpdate(GetDialogWindow(dlog));
				UpdateDialogVisRgn(dlog);
				DrawPopUp(&gotoPopUp8);
				OutlineOKButton(dlog,True);
				EndUpdate(GetDialogWindow(dlog));
				ans = True; *itemHit = 0;
			}
			else
				DoUpdate(w);
			break;
		case activateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetWindowPort(w));
			}
			break;
		case mouseDown:
			where = evt->where;
			choice = gotoPopUp8.currentChoice;
			GlobalToLocal(&where);
			if (PtInRect(where,&gotoPopUp8.shadow)) {
				*itemHit = DoUserPopUp(&gotoPopUp8) ? GOTOPOP_DI : 0;
				ans = True;
				break;
			}
			if (choice==gotoPAGE || choice==gotoBAR)
				if (HandleMouseDown(evt, minDlogVal, maxDlogVal, dlog)) {
					SelectDialogItemText(dlog, NUMBER_DI, 0, ENDTEXT);
					*itemHit = NUMBER_DI;
					ans = True;
					break;
				}
			if (choice==gotoREHEARSALMARK)
				if (RMHandleMouseDown(evt, firstMark, lastMark, dlog)) {
					SelectDialogItemText(dlog, NUMBER_DI, 0, ENDTEXT);
					*itemHit = NUMBER_DI;
					ans = True;
					break;
				}
			break;
		case keyDown:
			choice = gotoPopUp8.currentChoice;
			if (DlgCmdKey(dlog, evt, itemHit, False))
				return True;
			else if (choice==gotoPAGE || choice==gotoBAR)
				if (HandleKeyDown(evt, minDlogVal, maxDlogVal, dlog)) {
				*itemHit = NUMBER_DI;
				ans = True;
				}
			else if (choice==gotoREHEARSALMARK)
				if (RMHandleKeyDown(evt, firstMark, lastMark, dlog)) {
				*itemHit = NUMBER_DI;
				ans = True;
				}
			break;
	}
	return(ans);
}

/* Conduct dialog to get from user the new page number, measure number, or rehearsal
mark to display. Returns the type of thing to go to if okay, <goDirectlyToJAIL> if
cancel. */

short GoToDialog(Document *doc, short *currPageNum, short *currBar,
						LINK *currRMark)
{	
	DialogPtr dlog;
	Str63 markStr;
	short ditem, aShort;
	short value, gotoType, minPageVal, maxPageVal, minMeasVal, maxMeasVal;
	LINK pageL, measL, currMark, rMarkL=NILINK;
	PPAGE pPage;
	PAMEASURE aMeasure;
	GrafPtr oldPort;
	Boolean okay=False, stillGoing;
	Handle rHdl;
	Rect aRect;
	char fmtStr[256];
	ModalFilterUPP filterUPP;

	filterUPP = NewModalFilterUPP(GoToFilter);
	if (filterUPP == NULL) {
		MissingDialog(GOTO_DLOG);
		return goDirectlyToJAIL;
	}
	GetPort(&oldPort);
	dlog = GetNewDialog(GOTO_DLOG, NULL, BRING_TO_FRONT);
	if (dlog == NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(GOTO_DLOG);
		return goDirectlyToJAIL;
	}

	SetPort(GetDialogWindowPort(dlog));
	PlaceWindow(GetDialogWindow(dlog), doc->theWindow, 0, 40);
	gotoDoc = doc;
	
	if (!gotoPopUp8.currentChoice) gotoPopUp8.currentChoice = gotoPAGE;
	
	if (!InitPopUp(dlog, &gotoPopUp8, GOTOPOP_DI, GOTOSTXT_DI, gotoPopID,
															gotoPopUp8.currentChoice))
		goto broken;
	
	UseNumberFilter(dlog,NUMBER_DI,UpRect_DI,DownRect_DI);

	/* Initialize values for Goto Page */
	currPage = *currPageNum;

	pageL = LSSearch(doc->headL, PAGEtype, ANYONE, GO_RIGHT, False);
	pPage = GetPPAGE(pageL);
	minPageVal = pPage->sheetNum+doc->firstPageNumber;

	pageL = LSSearch(doc->tailL, PAGEtype, ANYONE, GO_LEFT, False);
	pPage = GetPPAGE(pageL);
	maxPageVal = pPage->sheetNum+doc->firstPageNumber;
	
	/* Initialize values for Goto Measure */
	currMeas = *currBar;

	measL = MNSearch(doc, doc->headL, ANYONE, GO_RIGHT, True);
	aMeasure = GetPAMEASURE(FirstSubLINK(measL));
	minMeasVal = aMeasure->measureNum+doc->firstMNNumber;
	
	measL = MNSearch(doc, doc->tailL, ANYONE, GO_LEFT, True);
	aMeasure = GetPAMEASURE(FirstSubLINK(measL));
	maxMeasVal = aMeasure->measureNum+doc->firstMNNumber;
	
	/* Initialize values for Goto Rehearsal Mark */
	markStr[0] = 0;						/* Empty Pascal string */
	currMark = *currRMark;
	if (currMark) Pstrcpy(markStr,RMARKSTR(currMark));
	firstMark = RMSearch(doc, doc->headL, ANYMARK, GO_RIGHT);
	lastMark = RMSearch(doc, doc->tailL, ANYMARK, GO_LEFT);

	switch (gotoPopUp8.currentChoice) {
		case gotoPAGE:
			PutDlgWord(dlog,NUMBER_DI,*currPageNum,True);
			minDlogVal = minPageVal; maxDlogVal = maxPageVal;
			break;
		case gotoBAR:
			PutDlgWord(dlog,NUMBER_DI,*currBar,True);
			minDlogVal = minMeasVal; maxDlogVal = maxMeasVal;
			break;
		case gotoREHEARSALMARK:
			if (currMark)
				PutDlgString(dlog,NUMBER_DI,RMARKSTR(currMark),True);
			else
				PutDlgString(dlog,NUMBER_DI,"\p",False);
			break;
		default:
			break;
	}

	GetDialogItem(dlog, GOTORANGE_DI, &aShort, &rHdl, &aRect);

	if (gotoPopUp8.currentChoice==gotoREHEARSALMARK)
		SetDialogItemCText(rHdl, "");
	else {
		GetIndCString(fmtStr, DIALOG_STRS, 2);    								/* "%d to %d" */
		sprintf(strBuf, fmtStr, minDlogVal, maxDlogVal); 
		SetDialogItemCText(rHdl, strBuf);
	}
	
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	do {
		while (True) {
			ModalDialog(filterUPP, &ditem);
			switch (ditem) {
				case GOTOPOP_DI:
					switch (gotoPopUp8.currentChoice) {
						case gotoPAGE:
							minDlogVal = minPageVal; maxDlogVal = maxPageVal;
							PutDlgWord(dlog,NUMBER_DI,currPage,True);
							break;
						case gotoBAR:
							minDlogVal = minMeasVal; maxDlogVal = maxMeasVal;
							PutDlgWord(dlog,NUMBER_DI,currMeas,True);
							break;
						case gotoREHEARSALMARK:
							PutDlgString(dlog,NUMBER_DI,markStr,*markStr ? True : False);
							break;
					}

					if (gotoPopUp8.currentChoice==gotoREHEARSALMARK)
						SetDialogItemCText(rHdl, "");
					else {
						GetIndCString(fmtStr, DIALOG_STRS, 2);    				/* "%d to %d" */
						sprintf(strBuf, fmtStr, minDlogVal, maxDlogVal); 
						SetDialogItemCText(rHdl, strBuf);
					}
					break;
				case NUMBER_DI:
					switch (gotoPopUp8.currentChoice) {
						case gotoPAGE:
							GetDlgWord(dlog,NUMBER_DI,&currPage);
							break;
						case gotoBAR:
							GetDlgWord(dlog,NUMBER_DI,&currMeas);
							break;
						case gotoREHEARSALMARK:
							GetDlgString(dlog,NUMBER_DI,markStr);
							break;
					}
					break;
			}
			if (ditem==OK || ditem==Cancel) { okay = (ditem==OK); break; }
		}
		
		if (okay) {
			switch (gotoPopUp8.currentChoice) {
				case gotoPAGE:
					if (!GetDlgWord(dlog,NUMBER_DI,&value) ||
							value<minPageVal || value>maxPageVal) {
						GetIndCString(fmtStr, DIALOGERRS_STRS, 9);			/* "no such page" */
						sprintf(strBuf, fmtStr, minPageVal, maxPageVal);
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						SelectDialogItemText(dlog, NUMBER_DI, 0, ENDTEXT);
						stillGoing = True;
					}
					else
						stillGoing = False;
					currPage = value;
					break;
				case gotoBAR:
					if (!GetDlgWord(dlog,NUMBER_DI,&value) ||
							value<minMeasVal || value>maxMeasVal) {
						GetIndCString(fmtStr, DIALOGERRS_STRS, 10);			/* "no such measure" */
						sprintf(strBuf, fmtStr, minMeasVal, maxMeasVal);
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						SelectDialogItemText(dlog, NUMBER_DI, 0, ENDTEXT);
						stillGoing = True;
					}
					else
						stillGoing = False;
					currMeas = value;
					break;
				case gotoREHEARSALMARK:
					rMarkL = RMSearch(doc, doc->headL, markStr, False);
					if (!GetDlgString(dlog,NUMBER_DI,markStr) || !rMarkL) {
						GetIndCString(strBuf, DIALOGERRS_STRS, 11);			/* "no such rehearsal mark" */
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						SelectDialogItemText(dlog, NUMBER_DI, 0, ENDTEXT);
						stillGoing = True;
					}
					else
						stillGoing = False;
					break;
				default:													/* should never get here */
					LogPrintf(LOG_WARNING, "GoToDialog: unrecognized type.\n");
					stillGoing = False;
			}
		}
	} while (okay && stillGoing);

broken:
	if (okay) {
		*currPageNum = currPage;
		*currBar = currMeas;
		*currRMark = rMarkL;
	}
	gotoType = okay ? gotoPopUp8.currentChoice : goDirectlyToJAIL;
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	return gotoType;
}
