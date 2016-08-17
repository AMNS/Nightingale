/***************************************************************************
*	FILE:	DialogsEditor.c
*	PROJ:	Nightingale
*	DESC:	Dialog-handling routines for miscellaneous score-editing dialogs
***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
	LeftEndDialog & friends
	SpaceDialog					TremSlashesDialog			EndingDialog
	MeasNumDialog				PageNumDialog				OttavaDialog
	TupletDialog & friends		SetDurDialog & friends		TempoDialog & friends
	SetMBRestDialog
	DrawSampleStaff				RLarger/RSmallerStaff		RHandleKeyDown
	RHandleMouseDown			RFilter						RastralDialog
	MarginsDialog				KeySigDialog & friends		SetKSDialogGuts
	TimeSigDialog & friends		RehearsalMarkDialog
	ChordFrameDialog			SymbolIsBarline				InsMeasUnkDurDialog		
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include <ctype.h>					/* for isdigit() */

static void DrawSampleStaff(void);

static pascal Boolean LeftEndFilter(DialogPtr, EventRecord *, short *);
static Boolean LeftEndBadValues(Document *, short, short);

static Boolean GetOttavaAlta(Document *, short);
static void RLargerStaff(void);
static void RSmallerStaff(void);
static Boolean RHandleKeyDown(EventRecord *);
static Boolean RHandleMouseDown(EventRecord *);
static pascal Boolean RFilter(DialogPtr, EventRecord *, short *);
static pascal Boolean TSFilter(DialogPtr, EventRecord *, short *);
static Boolean TSHandleMouseDown(EventRecord *);
static void TSDrawStaff(void);
static void TSDenominatorDown(void);
static void TSDenominatorUp(void);
static void TSNumeratorDown(void);
static void TSNumeratorUp(void);
typedef void (*TrackArrowFunc)(void);
static void TrackArrow(Rect *, TrackArrowFunc func);


/* --------------------------------- Declarations for simple "set number" Dialogs -- */

#define NUMBER_DI 	3				/* DITL index of number to be adjusted */
#define UpRect_DI	5				/* DITL index of up button rect */
#define DownRect_DI	6				/* DITL index of down button rect */

extern short minVal, maxVal;


/* ---------------------------------------------------------- LeftEndDialog et al -- */
/*	Handler for the "Left End of Systems" dialog and its help functions. */

Rect firstRect, otherRect;

static pascal Boolean LeftEndFilter(DialogPtr theDialog, EventRecord *theEvent,
										short *item)
{
	switch (theEvent->what) {
		case updateEvt:
			BeginUpdate(GetDialogWindow(theDialog));
			/* Outline the panels in background */
			FrameRect(&firstRect);
			FrameRect(&otherRect);
			UpdateDialogVisRgn(theDialog);
			FrameDefault(theDialog, OK, TRUE);
			EndUpdate(GetDialogWindow(theDialog));
			*item = 0;
			return TRUE;
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, item, FALSE)) return TRUE;
			break;
	}
	return FALSE;
}

static Boolean LeftEndBadValues(Document *doc,
					short newFirstDist, short newOtherDist)	/* in points */
{
	DDIST sysWidth;

	if (newFirstDist<0 || newOtherDist<0) {
		GetIndCString(strBuf, DIALOGERRS_STRS, 1);			/* "can't have a negative indent" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return TRUE;
	}
	
	sysWidth = MARGWIDTH(doc)-pt2d(newFirstDist);
	if (sysWidth<pt2d(in2pt(2))) {
		GetIndCString(strBuf, DIALOGERRS_STRS, 2);			/* "would make the 1st system too short" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return TRUE;
	}
	
	sysWidth = MARGWIDTH(doc)-pt2d(newOtherDist);
	if (sysWidth<pt2d(in2pt(2))) {
		GetIndCString(strBuf, DIALOGERRS_STRS, 3);			/* "would make the other systems too short" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return TRUE;
	}

	return FALSE;
}


static enum
{
	USER5=5,
	FULLNAMES_FIRST_DI=6,							/* Item numbers */
	ABBREVNAMES_FIRST_DI,
	NONE_FIRST_DI,
	NEED_FIRST_DI=10,
	DIST_FIRST_DI=13,
	USER16=16,
	FULLNAMES_OTHER_DI=17,
	ABBREVNAMES_OTHER_DI,
	NONE_OTHER_DI,
	NEED_OTHER_DI=21,
	DIST_OTHER_DI=24
} E_LeftEndItems;

Boolean LeftEndDialog(short *pFirstNames,
						short *pFirstDist,	/* First system indent, in points */
						short *pOtherNames,
						short *pOtherDist)	/* Other systems indent, in points */
{
	DialogPtr dlog;
	GrafPtr oldPort;
	short dialogOver, ditem, type;
	short group1, group2, nameCode, newFirstDist, newOtherDist;
	Handle hndl;
	double inchNFDist, inchDFDist, inchNODist, inchDODist, inchTemp;
	Document *doc=GetDocumentFromWindow(TopDocument);
	ModalFilterUPP	filterUPP;

	if (doc==NULL) {
		//MissingDocument(LEFTEND_DLOG);
		return FALSE;
	}
	filterUPP = NewModalFilterUPP(LeftEndFilter);
	if (filterUPP == NULL) {
		MissingDialog(LEFTEND_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(LEFTEND_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(LEFTEND_DLOG);
		return FALSE;
	}
	CenterWindow(GetDialogWindow(dlog), 60);
	SetPort(GetDialogWindowPort(dlog));
	
	/* Get the background panel rectangles, as defined by some user items, and
		get them out of the way so they don't hide any items underneath */
	
	GetDialogItem(dlog, USER5, &type, &hndl, &firstRect);
	GetDialogItem(dlog, USER16, &type, &hndl, &otherRect);		
	HideDialogItem(dlog,USER5);
	HideDialogItem(dlog,USER16);

	/* Set up radio button group, needed and actual indents for "First system". */
	
	if (*pFirstNames==NONAMES)			group1 = NONE_FIRST_DI;
	else if (*pFirstNames==ABBREVNAMES) group1 = ABBREVNAMES_FIRST_DI;
	else								group1 = FULLNAMES_FIRST_DI;
	PutDlgChkRadio(dlog, group1, TRUE);

	inchNFDist = PartNameMargin(doc, *pFirstNames);
	PutDlgDouble(dlog, NEED_FIRST_DI, inchNFDist, FALSE);
	
	inchDFDist = pt2in(*pFirstDist);
	inchDFDist = RoundDouble(inchDFDist, .01);
	PutDlgDouble(dlog, DIST_FIRST_DI, inchDFDist, FALSE);

	/* Set up radio button group, needed and actual indents for "Other systems". */
	
	if (*pOtherNames==NONAMES)			group2 = NONE_OTHER_DI;
	else if (*pOtherNames==ABBREVNAMES) group2 = ABBREVNAMES_OTHER_DI;
	else								group2 = FULLNAMES_OTHER_DI;
	PutDlgChkRadio(dlog, group2, TRUE);

	inchNODist = PartNameMargin(doc, *pOtherNames);
	PutDlgDouble(dlog, NEED_OTHER_DI, inchNODist, FALSE);
	
	inchDODist = pt2in(*pOtherDist);
	inchDODist = RoundDouble(inchDODist, .01);
	PutDlgDouble(dlog, DIST_OTHER_DI, inchDODist, FALSE);

	SelectDialogItemText(dlog, DIST_FIRST_DI, 0, ENDTEXT);

	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	dialogOver = 0;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);	
		switch (ditem) {
			case OK:
				newFirstDist = *pFirstDist;
				newOtherDist = *pOtherDist;
				/* Get new indent values. Be careful to avoid roundoff error! */
			
				if (GetDlgDouble(dlog, DIST_FIRST_DI, &inchTemp))
					if (ABS(inchTemp-inchDFDist)>.001) {
						newFirstDist = in2pt(inchTemp)+.5;
					}
				if (GetDlgDouble(dlog, DIST_OTHER_DI, &inchTemp))
					if (ABS(inchTemp-inchDODist)>.001) {
						newOtherDist = in2pt(inchTemp)+.5;
					}
				dialogOver = (LeftEndBadValues(doc, newFirstDist, newOtherDist)?
									0 : OK);
				break;
			case Cancel:
				dialogOver = Cancel;
				break;
			case FULLNAMES_FIRST_DI:
			case ABBREVNAMES_FIRST_DI:
			case NONE_FIRST_DI:
				if (ditem!=group1) {
					SwitchRadio(dlog, &group1, ditem);
					if (group1==NONE_FIRST_DI)				nameCode = NONAMES;
					else if (group1==ABBREVNAMES_FIRST_DI)	nameCode = ABBREVNAMES;
					else									nameCode = FULLNAMES;
					inchTemp = PartNameMargin(doc, nameCode);
					PutDlgDouble(dlog, NEED_FIRST_DI, inchTemp, FALSE);
				}
				break;
			case FULLNAMES_OTHER_DI:
			case ABBREVNAMES_OTHER_DI:
			case NONE_OTHER_DI:
				if (ditem!=group2) {
					SwitchRadio(dlog, &group2, ditem);
					if (group2==NONE_OTHER_DI)				nameCode = NONAMES;
					else if (group2==ABBREVNAMES_OTHER_DI)	nameCode = ABBREVNAMES;
					else									nameCode = FULLNAMES;
					inchTemp = PartNameMargin(doc, nameCode);
					PutDlgDouble(dlog, NEED_OTHER_DI, inchTemp, FALSE);
				}
				break;
			default:
				;
		}
	}
	
	if (dialogOver==OK) {
		if (group1==NONE_FIRST_DI)				*pFirstNames = NONAMES;
		else if (group1==ABBREVNAMES_FIRST_DI)	*pFirstNames = ABBREVNAMES;
		else									*pFirstNames = FULLNAMES;
		if (GetDlgDouble(dlog, DIST_FIRST_DI, &inchTemp))
			*pFirstDist = in2pt(inchTemp)+.5;

		if (group2==NONE_OTHER_DI)				*pOtherNames = NONAMES;
		else if (group2==ABBREVNAMES_OTHER_DI)	*pOtherNames = ABBREVNAMES;
		else									*pOtherNames = FULLNAMES;
		if (GetDlgDouble(dlog, DIST_OTHER_DI, &inchTemp))
			*pOtherDist = in2pt(inchTemp)+.5;
	}
	
broken:
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	return (dialogOver==OK);		
}


/* ----------------------------------------------------------------- SpaceDialog --- */
/* Handle Respace and (obsolete) Tightness dialogs.  Returns new percent if dialog
OKed, CANCEL_INT if Cancelled. N.B. Tightness should really be handled by a separate
routine, since this contains some code that could misbehave for it, e.g., if
minPercent!=maxPercent or either is below MINSPACE or above MAXSPACE.*/

#define Range_DI 10

short SpaceDialog(
		short	dlogID,						/* ID of DLOG resource to use */
		short	minPercent,					/* Minimum percentage in score range now */
		short	maxPercent 					/* Maximum percentage in score range now */
		)
{	
	short		ditem, newspace, itype, showPercent;
	Rect		tRect;
	Handle		tHdl;
	DialogPtr	dlog;
	GrafPtr		oldPort;
	char		fmtStr[256];

	newspace = CANCEL_INT;
	
	GetPort(&oldPort);
	dlog = GetNewDialog(dlogID, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		CenterWindow(GetDialogWindow(dlog), 70);
		
		/*
		 * Fill in dialog's values. Initialize spacing percentage to a value between the
		 * minimum and maximum current values, truncated to a legal range. We can only
		 * make a very rough guess at what the user is likely to want, so it's better
		 * to use a value closer to the minimum to reduce the chances of having to
		 * reformat. If the resulting value, min., and max. are not all the same, we
		 * also display min. and max.
		 */
		showPercent = minPercent+2*(maxPercent-minPercent)/5;
		if (showPercent<MINSPACE) showPercent = MINSPACE;
		if (showPercent>MAXSPACE) showPercent = MAXSPACE;
		
		GetDialogItem(dlog, Range_DI, &itype, &tHdl, &tRect);
		if (showPercent==minPercent && minPercent==maxPercent)
			SetDialogItemCText(tHdl, "");
		else {
			GetIndCString(fmtStr, DIALOG_STRS, 1);   		 /* "Selected measures' spacing is %d to %d%%." */
			sprintf(strBuf, fmtStr, minPercent, maxPercent); 
			SetDialogItemCText(tHdl, strBuf);
		}
		PutDlgWord(dlog,NUMBER_DI,showPercent,TRUE);
		
		UseNumberFilter(dlog,NUMBER_DI,UpRect_DI,DownRect_DI);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
		
		minVal = MINSPACE;
		maxVal = MAXSPACE;
		
		do {
			while (TRUE) {
				ModalDialog(&NumberFilter, &ditem);
				if (ditem == OK || ditem==Cancel) break;
				}
			if (ditem == OK) {
				GetDlgWord(dlog,NUMBER_DI,&newspace);
				if (newspace<MINSPACE || newspace>MAXSPACE) {
					Inform(SPACE_ALRT);									/* No room for StopInform icon! */
					newspace = CANCEL_INT;
					}
				}
			 else
				break;
		} while (newspace<MINSPACE || newspace>MAXSPACE);
	
		DisposeDialog(dlog);
		}
	else
		MissingDialog(dlogID);

	SetPort(oldPort);
	return newspace;
}


/* ------------------------------------------------------------ TremSlashesDialog -- */
/* Conduct dialog to get number of slashes in "bowed" tremolo from user.  Returns
result, or CANCEL_INT for Cancel. */


short TremSlashesDialog(short initSlashes)		/* Initial (default) value */
{
	DialogPtr	dlog;
	short		ditem, nslashes;
	GrafPtr		oldPort;
	char		fmtStr[256];
	ModalFilterUPP filterUPP;
	
	filterUPP = NewModalFilterUPP(NumberFilter);
	if (filterUPP == NULL) {
		MissingDialog(TREMSLASHES_DLOG);
		return CANCEL_INT;
	}
	
	nslashes = CANCEL_INT;
	
	GetPort(&oldPort);
	dlog = GetNewDialog(TREMSLASHES_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		PutDlgWord(dlog, NUMBER_DI, initSlashes, TRUE);
		UseNumberFilter(dlog, NUMBER_DI, UpRect_DI, DownRect_DI);

		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();

		minVal = 1;
		maxVal = 6;
	
		do {
			while (TRUE) {
				ModalDialog(&NumberFilter, &ditem);
				if (ditem==OK || ditem==Cancel) break;
				}
			if (ditem==OK) {
				GetDlgWord(dlog, NUMBER_DI, &nslashes);
				if (nslashes>=minVal && nslashes<=maxVal) break;
				GetIndCString(fmtStr, DIALOGERRS_STRS, 4);				/* "Number of slashes..." */
				sprintf(strBuf, fmtStr, minVal, maxVal);
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				nslashes = CANCEL_INT;
				}
			 else
				break;
			} while (nslashes<minVal || nslashes>maxVal);
			
		DisposeDialog(dlog);
		}
	else
		MissingDialog(TREMSLASHES_DLOG);
	
	SetPort(oldPort);
	return nslashes;
}


/* ----------------------------------------------------------------- EndingDialog -- */
/* Conduct dialog to get Ending symbol attributes from user. Returns TRUE if OK'ed,
FALSE if cancelled. */

static enum {
	ENDINGNUM_POP_DI=3,
	ENDINGNUM_LABEL_DI,
	CLOSED_OPEN_DI=5,
	CLOSED_CLOSED_DI,
	OPEN_CLOSED_DI
} E_Endingitems;

static UserPopUp endingPopup;

static Boolean hilitedItem = FALSE;  	/* Is an item currently hilited by using arrow keys? */

static pascal Boolean EndingFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Point where;
	Boolean ans=FALSE; WindowPtr w;
	short choice;

	w = (WindowPtr)(evt->message);
	switch(evt->what) {
		case updateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetWindowPort(w));
				BeginUpdate(GetDialogWindow(dlog));
				UpdateDialogVisRgn(dlog);
				DrawPopUp(&endingPopup);
				OutlineOKButton(dlog, TRUE);
				EndUpdate(GetDialogWindow(dlog));
				ans = TRUE; *itemHit = 0;
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
			choice = endingPopup.currentChoice;
			GlobalToLocal(&where);
			if (PtInRect(where, &endingPopup.shadow)) {
				*itemHit = (DoUserPopUp(&endingPopup) ? ENDINGNUM_POP_DI : 0);
				ans = TRUE;
				break;
			}
			break;
		case keyDown:
			choice = endingPopup.currentChoice;
			if (DlgCmdKey(dlog, evt, itemHit, FALSE))
				return TRUE;
			else {
				short ch = (unsigned char)evt->message;
				switch (ch) {
					case UPARROWKEY:
					case DOWNARROWKEY:
						HiliteArrowKey(dlog, ch, &endingPopup, &hilitedItem);
						ans = TRUE;
						break;						
				}
			}
			break;
	}
	
	return ans;
}


Boolean EndingDialog(short initNumber, short *pNewNumber, short initCutoffs,
							short *pNewCutoffs)
{
	static Boolean firstCall = TRUE;
	DialogPtr	dlog;
	short		ditem, number, group1, n, strOffset;
	GrafPtr		oldPort;
	char		fmtStr[256], numStr[MAX_ENDING_STRLEN];

	number = CANCEL_INT;
	
	GetPort(&oldPort);
	dlog = GetNewDialog(ENDING_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		if (!endingPopup.currentChoice) endingPopup.currentChoice = 2;
		
		/* Init the popup menu of labels on the first call only.
		 * CER 5.18.2004: InitPopUp gets the menu from the resource, abandoning the previous
		 * menu. If we init the popup but only build the menu for the first call, successive
		 * calls have an empty menu.
		 * If we init the popup and rebuild the menu for every call, do we have a memory leak
		 * when we abandon the old menu?
		 */
		if (firstCall) {
			if (!InitPopUp(dlog, &endingPopup, ENDINGNUM_POP_DI, ENDINGNUM_LABEL_DI,
								ENDING_MENU, endingPopup.currentChoice))
				goto Cleanup;
		}
		
		/* Build the popup menu of labels on the first call only. 1st label is string 0. 
		 * CER 5.18.2004: InitPopUp gets the menu from the resource, abandoning the previous
		 * menu.
		 */
		
		if (firstCall) {
			firstCall = FALSE;
			for (n = 0; n<maxEndingNum; n++) {
				strOffset = n*MAX_ENDING_STRLEN;
				GoodStrncpy(numStr, &endingString[strOffset], MAX_ENDING_STRLEN-1);
				if (strlen(numStr)>0)
					AppendCMenu(endingPopup.menu, numStr);
			}
		}
		
		ChangePopUpChoice(&endingPopup, initNumber+1);
		hilitedItem = FALSE;

		if (initCutoffs==1)			group1 = CLOSED_OPEN_DI;
		else if (initCutoffs==2)	group1 = OPEN_CLOSED_DI;
		else						group1 = CLOSED_CLOSED_DI;
		PutDlgChkRadio(dlog, group1, TRUE);

		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();

		minVal = 0;
		maxVal = maxEndingNum;
	
		do {
			while (TRUE) {
				ModalDialog(&EndingFilter, &ditem);
				if (ditem==OK || ditem==Cancel)
					break;
				else if (ditem>=CLOSED_OPEN_DI && ditem<=OPEN_CLOSED_DI)
					if (ditem!=group1)
						SwitchRadio(dlog, &group1, ditem);
			}
			if (ditem==OK) {
				number = endingPopup.currentChoice-1;
				if (number>=minVal && number<=maxVal) {
					*pNewNumber = number;
					if (group1==CLOSED_OPEN_DI)		*pNewCutoffs = 1;
					else if (group1==OPEN_CLOSED_DI) *pNewCutoffs = 2;
					else										*pNewCutoffs = 0;
				}
				else {
					GetIndCString(fmtStr, DIALOGERRS_STRS, 5);			/* "Ending/bracket number..." */
					sprintf(strBuf, fmtStr, minVal, maxVal);
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					number = CANCEL_INT;
				}
			}
			 else
				break;
		} while (number<minVal || number>maxVal);
			
		DisposeDialog(dlog);
	}
	else
		MissingDialog(ENDING_DLOG);
	
Cleanup:
	SetPort(oldPort);
	return (ditem==OK);
}


/* ---------------------------------------------------------------- MeasNumDialog -- */
/* Conduct dialog to get information on how to number measures.  Delivers FALSE on
Cancel or error, TRUE on OK. */

static enum
{
	NONE_DI=4,												/* Item numbers */
	EVERYN_DI,
	NMEAS_DI,
	EVERYSYS_DI=8,
	FIRSTMNUM_DI=10,
	ABOVE_DI=12,
	BELOW_DI,
	YOFFSET_DI=14,
	XOFFSET_DI=17,
	SYSFIRST_DI=19,
	XSYSOFFSET_DI,
	STARTPRINT1_DI=22,
	STARTPRINT2_DI=23
} E_MeasNumitems;

Boolean MeasNumDialog(Document *doc)
{	
	DialogPtr	dlog;
	short		ditem, group1, group2, group3, dialogOver;
	GrafPtr		oldPort;
	short		numberMeas, xOffset, xSysOffset, yOffset, firstNumber;
	Boolean		above, sysFirst, startPrint1;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(MEASNUM_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(MEASNUM_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(MEASNUM_DLOG);
		return FALSE;
	}
	
	numberMeas = doc->numberMeas;
	xOffset = doc->xMNOffset;
	xSysOffset = doc->xSysMNOffset;
	yOffset = doc->yMNOffset;
	above = doc->aboveMN;
	sysFirst = doc->sysFirstMN;
	firstNumber = doc->firstMNNumber;
	startPrint1 = doc->startMNPrint1;
	
	SetPort(GetDialogWindowPort(dlog));
	
	/* Set up radio button groups */
	
	if (numberMeas>0)		group1 = EVERYN_DI;							/* First group */
	else if (numberMeas<0)	group1 = EVERYSYS_DI;
	else					group1 = NONE_DI;
	PutDlgChkRadio(dlog, group1, TRUE);
		
	group2 = (above? ABOVE_DI : BELOW_DI);								/* Second group */	
	PutDlgChkRadio(dlog, group2, TRUE);
	
	group3 = (startPrint1? STARTPRINT1_DI : STARTPRINT2_DI);			/* Third group */	
	PutDlgChkRadio(dlog, group3, TRUE);
	
	PutDlgWord(dlog,NMEAS_DI, numberMeas>0 ? numberMeas : 1, FALSE);
	PutDlgWord(dlog,XOFFSET_DI, xOffset, FALSE);
	PutDlgChkRadio(dlog, SYSFIRST_DI, sysFirst);
	PutDlgWord(dlog,XSYSOFFSET_DI, xSysOffset, FALSE);
	PutDlgWord(dlog,FIRSTMNUM_DI, firstNumber, FALSE);
	PutDlgWord(dlog,YOFFSET_DI, yOffset, TRUE);

	CenterWindow(GetDialogWindow(dlog), 55);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	minVal = 0;
	maxVal = 100;
	dialogOver = 0;
	while (dialogOver==0) {
		do {
			ModalDialog(filterUPP, &ditem);
			switch (ditem) {
				case OK:
					if (GetDlgChkRadio(dlog, NONE_DI)) 
						numberMeas = 0;
					else if (GetDlgChkRadio(dlog, EVERYSYS_DI))
						numberMeas = -1;
					else if (!GetDlgWord(dlog, NMEAS_DI, &numberMeas)
								|| (numberMeas<1 || numberMeas>100)) {
							GetIndCString(strBuf, DIALOGERRS_STRS, 6);		/* "Measure numbering interval..." */
							CParamText(strBuf, "", "", "");
							StopInform(GENERIC_ALRT);
							dialogOver = 0;
							break;
						}
						
					if (!GetDlgWord(dlog, FIRSTMNUM_DI, &firstNumber)
										|| firstNumber<0 || firstNumber>8000) {
						GetIndCString(strBuf, DIALOGERRS_STRS, 7);			/* "First measure number..." */
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						dialogOver = 0;
						break;
					}
					
					GetDlgWord(dlog, XOFFSET_DI, &xOffset);
					GetDlgWord(dlog, XSYSOFFSET_DI, &xSysOffset);
					GetDlgWord(dlog, YOFFSET_DI, &yOffset);
					above = GetDlgChkRadio(dlog, ABOVE_DI);
					sysFirst = GetDlgChkRadio(dlog, SYSFIRST_DI);
					GetDlgWord(dlog, FIRSTMNUM_DI, &firstNumber);
					startPrint1 = GetDlgChkRadio(dlog, STARTPRINT1_DI);
					if (doc->numberMeas!=numberMeas
							||  doc->xMNOffset!=xOffset
							||  doc->sysFirstMN!=sysFirst
							||  doc->xSysMNOffset!=xSysOffset
							||  doc->yMNOffset!=yOffset
							||  doc->aboveMN!=above
							||  doc->firstMNNumber!=firstNumber
							||  doc->startMNPrint1!=startPrint1) {
						doc->changed = TRUE;
						doc->numberMeas = numberMeas;
						doc->xMNOffset = xOffset;
						doc->sysFirstMN = sysFirst;
						doc->xSysMNOffset = xSysOffset;
						doc->yMNOffset = yOffset;
						doc->aboveMN = above;
						doc->firstMNNumber = firstNumber;
						doc->startMNPrint1 = startPrint1;
					}				/* drop thru... */
				case Cancel:
					dialogOver = ditem;
					break;
				case NONE_DI:
				case EVERYN_DI:
				case EVERYSYS_DI:
					if (ditem != group1)
						SwitchRadio(dlog, &group1, ditem);
					break;
				case ABOVE_DI:
				case BELOW_DI:
					if (ditem != group2)
						SwitchRadio(dlog, &group2, ditem);
					break;
				case STARTPRINT1_DI:
				case STARTPRINT2_DI:
					if (ditem != group3)
						SwitchRadio(dlog, &group3, ditem);
					break;
				case SYSFIRST_DI:
				 	PutDlgChkRadio(dlog, SYSFIRST_DI, sysFirst=!sysFirst);
					break;
				default:
					;
				 }
			} while (dialogOver==0);
		}

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	SetPort(oldPort);
	return (dialogOver==OK);
}


/* ---------------------------------------------------------------- PageNumDialog -- */
/* Conduct dialog to get info on how to number pages.  Delivers FALSE on
Cancel or error, TRUE on OK. */

static enum
{
	PNNONE_DI=4,												/* Item numbers */
	PNEVERYBUT_DI,
	PNEVERY_DI,
	FIRSTNUM_DI=8,
	TOP_DI=10,
	BOTTOM_DI,
	LEFT_DI,
	CENTER_DI,
	RIGHT_DI,
	ALTERNATE_DI
} E_PageNumitems;

Boolean PageNumDialog(Document *doc)
{
	DialogPtr		dlog;
	short			showGroup, oldShowGroup,
					vPosGroup, oldVPosGroup,
					hPosGroup, oldHPosGroup,
					ditem, dialogOver;
	GrafPtr			oldPort;
	short			firstNumber, alternate;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(PAGENUM_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(PAGENUM_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(PAGENUM_DLOG);
		return FALSE;
		}
		
	SetPort(GetDialogWindowPort(dlog));
		
	/* Set up radio button groups. For now, we don't pay attention to the actual
	 *	value of doc->startPageNumber, we just distinguish 3 relationships between
	 *	it and doc->firstPageNumber.
	 */
	if		(doc->firstPageNumber>=doc->startPageNumber)	showGroup = PNEVERY_DI;
	else if (doc->firstPageNumber+1==doc->startPageNumber)	showGroup = PNEVERYBUT_DI;
	else													showGroup = PNNONE_DI;
	PutDlgChkRadio(dlog, showGroup, TRUE);
	oldShowGroup = showGroup;
	
	vPosGroup = (doc->topPGN? TOP_DI : BOTTOM_DI);
	PutDlgChkRadio(dlog, vPosGroup, TRUE);
	oldVPosGroup = vPosGroup;
	
	if (doc->hPosPGN==LEFT_SIDE)	hPosGroup = LEFT_DI;
	else if (doc->hPosPGN==CENTER)	hPosGroup = CENTER_DI;
	else							hPosGroup = RIGHT_DI;
	PutDlgChkRadio(dlog, hPosGroup, TRUE);
	oldHPosGroup = hPosGroup;	

	/* Initialize other dialog controls. */
	
	firstNumber = doc->firstPageNumber;
	PutDlgWord(dlog, FIRSTNUM_DI, firstNumber, TRUE);

	PutDlgChkRadio(dlog, ALTERNATE_DI, doc->alternatePGN);

	CenterWindow(GetDialogWindow(dlog), 55);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	dialogOver = 0;
	while (dialogOver==0) {
		do {
			ModalDialog(filterUPP, &ditem);
			switch (ditem) {
				case OK:
					GetDlgWord(dlog, FIRSTNUM_DI, &firstNumber);
					if (showGroup!=oldShowGroup
					|| firstNumber!=doc->firstPageNumber
					|| vPosGroup!=oldVPosGroup
					|| hPosGroup!=oldHPosGroup
					|| GetDlgChkRadio(dlog, ALTERNATE_DI)!=doc->alternatePGN) {
						doc->firstPageNumber = firstNumber;
						if (doc->firstPageNumber>999) doc->firstPageNumber = 999;
						if (showGroup==PNEVERY_DI)
							doc->startPageNumber = doc->firstPageNumber;
						else if (showGroup==PNEVERYBUT_DI)
							doc->startPageNumber = doc->firstPageNumber+1;
						else
							doc->startPageNumber = SHRT_MAX;

						doc->topPGN = (vPosGroup==TOP_DI);

						if (hPosGroup==LEFT_DI)			doc->hPosPGN = LEFT_SIDE;
						else if (hPosGroup==CENTER_DI)	doc->hPosPGN = CENTER;
						else							doc->hPosPGN = RIGHT_SIDE;
						
						doc->alternatePGN = GetDlgChkRadio(dlog, ALTERNATE_DI);

						doc->changed = TRUE;
					}													/* drop thru... */
				case Cancel:
					dialogOver = ditem;
					break;
				case PNNONE_DI:
				case PNEVERYBUT_DI:
				case PNEVERY_DI:
					if (ditem!=showGroup) SwitchRadio(dlog, &showGroup, ditem);
					break;
				case TOP_DI:
				case BOTTOM_DI:
					if (ditem!=vPosGroup) SwitchRadio(dlog, &vPosGroup, ditem);
					break;
				case LEFT_DI:
				case CENTER_DI:
				case RIGHT_DI:
					if (ditem!=hPosGroup) SwitchRadio(dlog, &hPosGroup, ditem);
					if (GetDlgChkRadio(dlog, ALTERNATE_DI) && hPosGroup==CENTER_DI)
						PutDlgChkRadio(dlog, ALTERNATE_DI, FALSE);	
					break;
				case ALTERNATE_DI:
					alternate = !GetDlgChkRadio(dlog, ALTERNATE_DI);
					PutDlgChkRadio(dlog, ALTERNATE_DI, alternate);	
					if (alternate && hPosGroup==CENTER_DI)
						SwitchRadio(dlog, &hPosGroup, LEFT_DI);
					break;
				default:
					;
			}
		} while (dialogOver==0);
	}
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);

	SetPort(oldPort);
	return (dialogOver==OK);
}


/* ----------------------------------------------------------------- OttavaDialog -- */

static enum 
{
	OCT8va_DI=3,			/* N.B. value returned depends on these...Dept. of Redundancy Dept. */
	OCT15ma_DI,
	OCT22ma_DI,
	OCT8vaBassa_DI,
	OCT15maBassa_DI,
	OCT22maBassa_DI
} E_OttavaItems;

/* Determine whether to initialize the ottava dialog with OCT8va or
OCT8vaBassa. Returns true if the average position of all notes on <selStf>
in the selection range is above the staffTop. */

Boolean GetOttavaAlta(Document *doc, short selStf)
{
	LINK pL, aNoteL;
	PANOTE aNote;
	DDIST yd=0;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSTAFF(aNoteL)==selStf && NoteSEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					yd += aNote->yd;
/* If huge selection range, avoid overflow. FIXME: A better way: use long DDIST for yd. */
					if (yd > 32000 || yd < -32000) goto done;
				}
		}

	done: 	
	return (yd <= 0);
}

/* Dialog to get Ottava type.  Returns FALSE for Cancel, TRUE for OK; also return in
a parameter the type of octave sign. */

Boolean OttavaDialog(Document *doc, Byte *octSignType)
{	
	DialogPtr	dlog;
	short		ditem, radio, selStf;
	GrafPtr		oldPort;
	Boolean		alta;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(OTTAVA_DLOG);
		return FALSE;
	}
	
	GetPort(&oldPort);
	dlog = GetNewDialog(OTTAVA_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		selStf = GetSelectionStaff(doc);
		alta = (selStf == NOONE) ? TRUE : GetOttavaAlta(doc, selStf);
		radio = alta ? OCT8va_DI : OCT8vaBassa_DI;
		PutDlgChkRadio(dlog, radio, TRUE);
	
		CenterWindow(GetDialogWindow(dlog), 70);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		while (TRUE) {
			ModalDialog(filterUPP, &ditem);
			if (ditem==OK || ditem==Cancel) break;
			if (ditem>=OCT8va_DI && ditem<=OCT22maBassa_DI && ditem!=radio)
				SwitchRadio(dlog, &radio, ditem);
			}
		
		if (ditem == OK) *octSignType = radio-OCT8va_DI+1;
		
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
		}
	else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(OTTAVA_DLOG);
		ditem = Cancel;
		}
	
	SetPort(oldPort);
	return (ditem==OK);
}


/* ===================================================== TupletDialog and friends == */

static short VoiceTotDur(short, LINK, LINK);
static short VoiceTotDur(short voice, LINK voiceStartL, LINK voiceEndL)
{
	LINK pL, aNoteL;
	short totalDur;
	
	totalDur = 0;
	for (pL = voiceStartL; pL!= voiceEndL; pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteVOICE(aNoteL)==voice) {
					totalDur += SimpleLDur(aNoteL);
					break;						/* Assumes all notes in chord have same duration. */
				}
			}
		}
	return totalDur;
}

/* Get values with which to initialize Fancy Tuplet Dialog from the current
selection. First check all voices to see that tupleNum (the prospective accessory
numeral) can be the same for all, and if not, return FALSE. Otherwise, fill in all
fields of the TupleParam and return TRUE. */

#define DIFF_TUPLETS	\
	GetIndCString(strBuf, DIALOGERRS_STRS, 20);		/* "Fancy Tuplet can't handle different tuplets..." */	\
	CParamText(strBuf, "", "", "");					\
	StopInform(GENERIC_ALRT)

Boolean FTupletCheck(Document *doc, TupleParam *ptParam)
{
	short i, voice, firstVoice, tupleNum, tupleDenom, firstTupleNum, totalDur, selStf;
	LINK  voiceStartL, voiceEndL;
	TupleParam tempParam;
	char fmtStr[256];
	
	firstVoice = -1;
	for (voice = 1; voice<=MAXVOICES; voice++)
		if (VOICE_MAYBE_USED(doc, voice)) {
			GetNoteSelRange(doc, voice, &voiceStartL, &voiceEndL, NOTES_ONLY);
			if (voiceStartL!=NILINK && voiceEndL!=NILINK) {
				/*
				 * This voice has something selected. If it's not tuplable, or it is
				 * but its numerator is different from that of the first selected voice,
				 * we know the selection can't be Fancy Tupled.
				 */
				if (!CheckMaxTupleNum(voice, voiceStartL, voiceEndL, &tempParam)) {
					GetIndCString(fmtStr, DIALOGERRS_STRS, 16);			/* "numerator or denominator exceeds max..." */
					sprintf(strBuf, fmtStr, voice);
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					return FALSE;
				}
				tupleNum = GetTupleNum(voiceStartL, voiceEndL, voice, TRUE);
				if (tupleNum<=0) { DIFF_TUPLETS; return FALSE; }
				if (firstVoice<0) {
					firstTupleNum = tupleNum;
					firstVoice = voice;
				}
				else {
					if (tupleNum!=firstTupleNum) { DIFF_TUPLETS; return FALSE; }
				}
			}
		}
		
	GetNoteSelRange(doc, firstVoice, &voiceStartL, &voiceEndL, NOTES_ONLY);
	ptParam->accNum = firstTupleNum;
	
	if (firstTupleNum==4)
		tupleDenom = 3;
	else
		for (i = 1; i<firstTupleNum; i *= 2)
			tupleDenom = i;

	ptParam->accDenom = tupleDenom;

	totalDur = VoiceTotDur(firstVoice, voiceStartL, voiceEndL);
	ptParam->durUnit = GetDurUnit(totalDur, ptParam->accNum, ptParam->accDenom);

	ptParam->numVis = TRUE;
	ptParam->denomVis = FALSE;
	if ((selStf = GetSelectionStaff(doc)) == NOONE)
		ptParam->brackVis = TRUE;
	else
		ptParam->brackVis = GetBracketVis(doc, doc->selStartL, doc->selEndL, selStf);

	return TRUE;
}


static GRAPHIC_POPUP	durPop0dot, durPop1dot, durPop2dot, *curPop;
static POPKEY			*popKeys0dot, *popKeys1dot, *popKeys2dot;
static short			popUpHilited=TRUE, show2dots=FALSE;

/* ----------------------------------- Declarations & Help Funcs. for TupletDialog -- */

static enum	{
	TUPLE_NUM=4,										/* Item numbers */
	TUPLE_PTEXT,
	ED_TUPLE_DENOM,
	BOTH_VIS,
	NUM_VIS,
	NEITHER_VIS,
	SAMPLE_ITEM,
	BRACK_VIS,
	TPOPUP_ITEM,
	STAT_TUPLE_DENOM,
	TDUMMY_ITEM
} E_TupletDlgItems;

#define BREVE_DUR		3840
#define WHOLE_DUR		1920
#define HALF_DUR		960
#define QUARTER_DUR		480
#define EIGHTH_DUR		240
#define SIXTEENTH_DUR	120
#define THIRTY2ND_DUR	60
#define SIXTY4TH_DUR	30
#define ONE28TH_DUR		15
#define NO_DUR			0

short tupleDur;						/* Used to index duration strings. */
short tupleDenomItem;

/* Following declarations are basically a TupleParam but with ints for num/denom. */
short accNum, accDenom;
short durUnit;
Boolean numVis, denomVis, brackVis;

/* Local prototypes for TupletDialog. */
static pascal Boolean TupleFilter(DialogPtr, EventRecord *, short *);
static void DrawTupletItems(DialogPtr, short);

static pascal Boolean TupleFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr		w;
	short			ch, field, ans;
	Point			where;
	GrafPtr			oldPort;
	short			anInt;
	Handle			aHdl;
	Rect			tdRect;
	Boolean			denomItemVisible;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));			
				UpdateDialogVisRgn(dlog);
				DrawTupletItems(dlog,SAMPLE_ITEM);
				FrameDefault(dlog, OK, TRUE);
				DrawGPopUp(curPop);		
				HiliteGPopUp(curPop, popUpHilited);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return TRUE;
			}
			break;
		case activateEvt:
			if (w==GetDialogWindow(dlog))
				SetPort(GetDialogWindowPort(dlog));
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			if (PtInRect(where, &curPop->box)) {
				DoGPopUp(curPop);
				*itemHit = TPOPUP_ITEM;
				return TRUE;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, FALSE))
				return TRUE;
			ch = (unsigned char)evt->message;
			field = GetDialogKeyboardFocusItem(dlog);
			/*
			 * The Dialog Manager considers only EditText fields as candidates for being
			 *	activated by the tab key, so handle tabbing from field to field ourselves
			 *	so that user can direct keystrokes to the pop-up as well as the EditText
			 *	fields. But skip this if ED_TUPLE_DENOM isn't visible.
			 */
			GetDialogItem(dlog, ED_TUPLE_DENOM, &anInt, &aHdl, &tdRect);
			denomItemVisible = (tdRect.left<8192);
			if (ch=='\t') {
				if (denomItemVisible) {
					field = field==ED_TUPLE_DENOM? TDUMMY_ITEM : ED_TUPLE_DENOM;
					popUpHilited = field==ED_TUPLE_DENOM? FALSE : TRUE;
					SelectDialogItemText(dlog, field, 0, ENDTEXT);
					HiliteGPopUp(curPop, popUpHilited);
					*itemHit = 0;
					return TRUE;
				}
			}
			else {
				if (field==TDUMMY_ITEM || !denomItemVisible) {
					ans = DurPopupKey(curPop, popKeys0dot, ch);
					*itemHit = ans? TPOPUP_ITEM : 0;
					HiliteGPopUp(curPop, TRUE);
					return TRUE;						/* so no chars get through to TDUMMY_ITEM edit field */
				}										/* NB: however, DlgCmdKey will let you paste into TDUMMY_ITEM! */
			}
			break;
	}
	return FALSE;
}


static void DrawTupletItems(DialogPtr dlog, short /*ditem*/)
{
	short		xp, yp, itype, nchars, tupleLen, xColon;
	short		oldTxFont, oldTxSize, tupleWidth;
	Handle		tHdl;
	Rect		userRect;
	unsigned	char tupleStr[30],denomStr[20];
	DPoint		firstPt, lastPt;
	Document 	*doc=GetDocumentFromWindow(TopDocument);
	
	if (doc==NULL) return;

	GetDialogItem(dlog, SAMPLE_ITEM, &itype, &tHdl, &userRect);
	EraseRect(&userRect);
	xp = userRect.left + (userRect.right-userRect.left)/2;
	yp = userRect.top + (userRect.bottom-userRect.top)/2;
	
	if (numVis) {
		oldTxFont = GetPortTxFont();
		oldTxSize = GetPortTxSize();
		TextFont(sonataFontNum);					/* Set to Sonata font */
		TextSize(Staff2MFontSize(drSize[1]));		/* Nice and big for readability */	

		NumToSonataStr((long)accNum, tupleStr);
		tupleWidth = StringWidth(tupleStr);
		if (denomVis) {
			/* Since Sonata has no ':', leave space and we'll fake it */
			tupleLen = tupleStr[0]+1;
			tupleStr[tupleLen] = ' ';
			xColon = tupleWidth;

			/* Append the denominator string. */
			NumToSonataStr((long)accDenom, denomStr);
			nchars = denomStr[0];
			tupleStr[0] = nchars+tupleLen;
			while (nchars>0) {
				tupleStr[nchars+tupleLen] = denomStr[nchars];
				nchars--;
			}
		}
		tupleWidth = StringWidth(tupleStr);
		MoveTo(xp-tupleWidth/2, yp+3);
		DrawString(tupleStr);
		if (denomVis) {
			MoveTo(xp-tupleWidth/2+xColon, yp+2);
			DrawMColon(doc, TRUE, FALSE, 0);		/* 0 for last arg immaterial for Sonata */
		}

		SetDPt(&firstPt, p2d(userRect.left+10), p2d(yp));
		SetDPt(&lastPt, p2d(userRect.right-10), p2d(yp));
		if (brackVis)
			DrawTupletBracket(firstPt, lastPt, 0, p2d(4), p2d(xp), tupleWidth, FALSE,
										FALSE, NULL, FALSE);
		TextFont(oldTxFont);
		TextSize(oldTxSize);
	}
	
	else {
		SetDPt(&firstPt, p2d(userRect.left+10), p2d(yp));
		SetDPt(&lastPt, p2d(userRect.right-10), p2d(yp));
		if (brackVis)
			DrawTupletBracket(firstPt, lastPt, 0, p2d(4), p2d(xp), -1, FALSE,
										FALSE, NULL, FALSE);
	}
}

/* ----------------------------------------------------------------- TupletDialog -- */
/* Conduct the "Fancy Tuplet" dialog for a new or used tuplet whose initial
state is described by <ptParam>. */

Boolean TupletDialog(
					Document */*doc*/,
					TupleParam *ptParam,
					Boolean tupletNew)	/* TRUE=tuplet about to be created, FALSE=already exists */
{
	DialogPtr	dlog;
	GrafPtr		oldPort;
	short		ditem, type, i, logDenom, tempNum, maxChange, evenNum, radio;
	short		oldResFile;
	short		choice, newLDur, oldLDur, deltaLDur;
	Rect		staffRect;
	Handle		tHdl, hndl;
	Rect		box;
	ModalFilterUPP filterUPP;
		
	filterUPP = NewModalFilterUPP(TupleFilter);
	if (filterUPP==NULL) {
		MissingDialog(FANCYTUPLET_DLOG);
		return FALSE;
	}
	
	dlog = GetNewDialog(FANCYTUPLET_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(FANCYTUPLET_DLOG);
		return FALSE;
	}
	
	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));
	CenterWindow(GetDialogWindow(dlog), 50);

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);											/* popup code uses Get1Resource */

	accNum = ptParam->accNum;
	accDenom = ptParam->accDenom;
	durUnit = ptParam->durUnit;
	numVis = ptParam->numVis;
	denomVis = ptParam->denomVis;
	brackVis = ptParam->brackVis;

	switch (durUnit) {
		case BREVE_DUR:
			tupleDur = BREVE_L_DUR;
			break;
		case WHOLE_DUR:
			tupleDur = WHOLE_L_DUR;
			break;
		case HALF_DUR:
			tupleDur = HALF_L_DUR;
			break;
		case QUARTER_DUR:
			tupleDur = QTR_L_DUR;
			break;
		case EIGHTH_DUR:
			tupleDur = EIGHTH_L_DUR;
			break;
		case SIXTEENTH_DUR:
			tupleDur = SIXTEENTH_L_DUR;
			break;
		case THIRTY2ND_DUR:
			tupleDur = THIRTY2ND_L_DUR;
			break;
		case SIXTY4TH_DUR:
			tupleDur = SIXTY4TH_L_DUR;
			break;
		case ONE28TH_DUR:
			tupleDur = ONE28TH_L_DUR;
			break;
		default:
			tupleDur = NO_L_DUR;
			break;
		}

	durPop0dot.menu = NULL;					/* NULL makes any goto broken safe */
	durPop0dot.itemChars = NULL;
	popKeys0dot = NULL;

	GetDialogItem(dlog, TPOPUP_ITEM, &type, &hndl, &box);
	if (!InitGPopUp(&durPop0dot, TOP_LEFT(box), NODOTDUR_MENU, 1)) goto broken;
	popKeys0dot = InitDurPopupKey(&durPop0dot);
	if (popKeys0dot==NULL) goto broken;
	curPop = &durPop0dot;
	
	choice = GetDurPopItem(curPop, popKeys0dot, tupleDur, 0);
	if (choice==NOMATCH) choice = 1;
	SetGPopUpChoice(curPop, choice);
	oldLDur = tupleDur;

	tupleDenomItem = (tupletNew? ED_TUPLE_DENOM : STAT_TUPLE_DENOM);
	ShowDialogItem(dlog, (tupletNew? ED_TUPLE_DENOM : STAT_TUPLE_DENOM));
	HideDialogItem(dlog, (tupletNew? STAT_TUPLE_DENOM : ED_TUPLE_DENOM));
	PutDlgWord(dlog,TUPLE_NUM,accNum,FALSE);
	PutDlgWord(dlog,tupleDenomItem,accDenom,tupletNew);

	/* logDenom is the log2 of the accessory denominator; (tupleDur-logDenom+1) is the max.
	 *	duration at which the denominator is at least 2. evenNum is the number of times the
	 * numerator can be divided by 2 exactly. The minimum of these two can be subtracted
	 *	from tupleDur to give the maximum duration note allowable.
	 */
	for (logDenom=0, i=1; i<accDenom; i*=2) logDenom++;
	tempNum = accNum;
	for (evenNum=0; !odd(tempNum); tempNum /= 2) evenNum++;
	maxChange = n_min(logDenom-1, evenNum);
	minVal = tupleDur-maxChange;
	
	GetDialogItem(dlog, SAMPLE_ITEM, &type, &tHdl, &staffRect);
	/* Sample is a user Item */

	if (denomVis) radio = BOTH_VIS;
	else if (numVis) radio = NUM_VIS;
	else radio = NEITHER_VIS;
	PutDlgChkRadio(dlog,BOTH_VIS,radio==BOTH_VIS);
	PutDlgChkRadio(dlog,NUM_VIS,radio==NUM_VIS);
	PutDlgChkRadio(dlog,NEITHER_VIS,radio==NEITHER_VIS);
	PutDlgChkRadio(dlog,BRACK_VIS,brackVis);

	if (popUpHilited)
		SelectDialogItemText(dlog, TDUMMY_ITEM, 0, ENDTEXT);
	else
		SelectDialogItemText(dlog, TPOPUP_ITEM, 0, ENDTEXT);

	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	while (TRUE) {
		ModalDialog(filterUPP, &ditem);
		if (ditem==OK) {
			if (accNum<1 || accNum>255 || accDenom<1 || accDenom>255) {
				GetIndCString(strBuf, DIALOGERRS_STRS, 12);			/* "Numbers must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
			}
			else
				break;
		}
		if (ditem==Cancel) break;
		
		switch (ditem) {
			case TUPLE_NUM:
			case ED_TUPLE_DENOM:
			case STAT_TUPLE_DENOM:
				GetDlgWord(dlog,TUPLE_NUM,&accNum);
				GetDlgWord(dlog,tupleDenomItem,&accDenom);
				break;
			case TPOPUP_ITEM:
				newLDur = popKeys0dot[curPop->currentChoice].durCode;
	 			/*
	 			 *	If user just set the popup for a duration longer than the maximum
	 			 * allowable, change it to the maximum now.
	 			 */
				if (newLDur<minVal) {
					newLDur = minVal;
					choice = GetDurPopItem(curPop, popKeys0dot, newLDur, 0);
					if (choice==NOMATCH) choice = 1;
					SetGPopUpChoice(curPop, choice);
				}
				deltaLDur = newLDur-oldLDur;
				if (deltaLDur>0)
					for (i= 1; i<=ABS(deltaLDur); i++) {
						accNum *= 2; accDenom *= 2;
					}
				else
					for (i= 1; i<=ABS(deltaLDur); i++) {
						accNum /= 2; accDenom /= 2;
					}
				PutDlgWord(dlog,TUPLE_NUM,accNum,FALSE);
				PutDlgWord(dlog,tupleDenomItem,accDenom,tupletNew);
				oldLDur = newLDur;
				SelectDialogItemText(dlog, TDUMMY_ITEM, 0, ENDTEXT);
				HiliteGPopUp(curPop, popUpHilited = TRUE);
				break;
			case BOTH_VIS:
			case NUM_VIS:
			case NEITHER_VIS:
				if (ditem!=radio) SwitchRadio(dlog, &radio, ditem);
				numVis = (ditem==BOTH_VIS || ditem==NUM_VIS);
				denomVis = ditem == BOTH_VIS;
		   	break;
		   case BRACK_VIS:
		   	PutDlgChkRadio(dlog,ditem,brackVis = !brackVis);
		   	break;
			}
		DrawTupletItems(dlog, SAMPLE_ITEM);
	}
	
	if (ditem==OK) {
		ptParam->accNum = accNum;
		ptParam->accDenom = accDenom;
		ptParam->durUnit = durUnit;
		ptParam->numVis = numVis;
		ptParam->denomVis = denomVis;
		ptParam->brackVis = brackVis;
	}

broken:			
	DisposeGPopUp(&durPop0dot);
	if (popKeys0dot) DisposePtr((Ptr)popKeys0dot);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}


/* ----------------------------------------------------------------- SetDurDialog -- */

static Boolean IsSelInTuplet(Document *doc);
static Boolean IsSelInTupletNotTotallySel(Document *doc);
static Boolean SDAnyBadValues(Document *, DialogPtr, Boolean, short, short, short);
static pascal Boolean SetDurFilter(DialogPtr, EventRecord *, short *);

static enum
{
	SETLDUR_DI=3,
	SDDURPOP_DI,
	SHOW2DOTS_DI,
	CV_DI,
	SETPDUR_DI,
	PDURPCT_DI,
	DUMMYFLD_DI=11,
	HALVEDURS_DI=12,
	DOUBLEDURS_DI,
	SETDURSTO_DI
} E_SetDurItems;

static short setDurGroup;

/* Return TRUE if any selected notes (or rests) are in tuplets. */

static Boolean IsSelInTuplet(Document *doc)
{
	LINK pL,aNoteL;

	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteINTUPLET(aNoteL) && NoteSEL(aNoteL))
					return TRUE;
	
	return FALSE;			
}


/* Return TRUE if any selected notes (or rests) are in a tuplet, but not all the
	notes of the tuplet are selected. */

static Boolean IsSelInTupletNotTotallySel(Document *doc)
{
	LINK pL, aNoteL, voice, aTupletL, tpSyncL;
	short numSelNotes, numNotSelNotes;

	pL = LSSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	if (pL==NILINK)
		pL = doc->selStartL;

	for ( ; pL!=doc->selEndL; pL=RightLINK(pL))
		if (TupletTYPE(pL)) {
			voice = TupletVOICE(pL);
			numSelNotes = numNotSelNotes = 0;
			aTupletL = FirstSubLINK(pL);
			for ( ; aTupletL; aTupletL=NextNOTETUPLEL(aTupletL)) {
				tpSyncL = NoteTupleTPSYNC(aTupletL);
				aNoteL = FirstSubLINK(tpSyncL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteVOICE(aNoteL)==voice) {
						if (NoteSEL(aNoteL))
							numSelNotes++;
						else
							numNotSelNotes++;
					}
			}
			if (numSelNotes>0 && numNotSelNotes>0)
				return TRUE;
		}

	return FALSE;			
}


static Boolean SDAnyBadValues(Document *doc, DialogPtr dlog, Boolean newSetLDur,
								short newLDurAction, short /*newnDots*/, short newpDurPct)
{	
	if (newpDurPct<1 || newpDurPct>500) {
		GetIndCString(strBuf, DIALOGERRS_STRS, 13);				/* "Play duration percent must be..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		SelectDialogItemText(dlog, PDURPCT_DI, 0, ENDTEXT);
		return TRUE;
	}

	if (newSetLDur) {
		if (newLDurAction==SET_DURS_TO) {
			if (IsSelInTuplet(doc)) {
				GetIndCString(strBuf, DIALOGERRS_STRS, 14);		/* "can't Set notated Duration in tuplets" */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return TRUE;
			}
		}
		else {
			if (IsSelInTupletNotTotallySel(doc)) {
				GetIndCString(strBuf, DIALOGERRS_STRS, 21);		/* "...select all the notes of the tuplet" */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return TRUE;
			}
		}
	}

	return FALSE;
}


static pascal Boolean SetDurFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr	w;
	short		ch, field, ans;
	Point		where;
	GrafPtr		oldPort;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));			
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, OK, TRUE);
				DrawGPopUp(curPop);		
				HiliteGPopUp(curPop, popUpHilited);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return TRUE;
			}
			break;
		case activateEvt:
			if (w==GetDialogWindow(dlog))
				SetPort(GetDialogWindowPort(dlog));
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			if (PtInRect(where, &curPop->box)) {
				DoGPopUp(curPop);
				*itemHit = SDDURPOP_DI;
				return TRUE;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, FALSE))
				return TRUE;
			ch = (unsigned char)evt->message;
			field = GetDialogKeyboardFocusItem(dlog);
			/*
			 * The Dialog Manager considers only EditText fields as candidates for being
			 *	activated by the tab key, so handle tabbing from field to field ourselves
			 *	so user can direct keystrokes to the pop-up as well as the EditText fields.
			 */
			if (ch=='\t') {
				field = field==PDURPCT_DI? DUMMYFLD_DI : PDURPCT_DI;
				popUpHilited = field==PDURPCT_DI? FALSE : TRUE;
				SelectDialogItemText(dlog, field, 0, ENDTEXT);
				HiliteGPopUp(curPop, popUpHilited);
				*itemHit = 0;
				return TRUE;
			}
			else {
				if (field==DUMMYFLD_DI) {
					ans = DurPopupKey(curPop, show2dots? popKeys2dot : popKeys1dot, ch);
					*itemHit = ans? SDDURPOP_DI : 0;
					HiliteGPopUp(curPop, TRUE);
					if (setDurGroup!=SETDURSTO_DI)
						SwitchRadio(dlog, &setDurGroup, SETDURSTO_DI);
					return TRUE;					/* so no chars get through to DUMMYFLD_DI edit field */
				}									/* NB: however, DlgCmdKey will let you paste into DUMMYFLD_DI! */
			}
			break;
	}
	return FALSE;
}


static void XableLDurPanel(DialogPtr dlog, Boolean enable)
{
	if (enable) {
		XableControl(dlog, HALVEDURS_DI, TRUE);
		XableControl(dlog, DOUBLEDURS_DI, TRUE);
		XableControl(dlog, SETDURSTO_DI, TRUE);
		XableControl(dlog, SHOW2DOTS_DI, TRUE);
		XableControl(dlog, CV_DI, TRUE);
	}
	else {
		XableControl(dlog, HALVEDURS_DI, FALSE);
		XableControl(dlog, DOUBLEDURS_DI, FALSE);
		XableControl(dlog, SETDURSTO_DI, FALSE);
		XableControl(dlog, SHOW2DOTS_DI, FALSE);
		XableControl(dlog, CV_DI, FALSE);
	}
}


Boolean SetDurDialog(
				Document *doc,
				Boolean *setLDur,			/* Set "notated" (logical) durations? */
				short *lDurAction,			/* HALVE_DURS, DOUBLE_DURS, or SET_DURS_TO */
				short *lDur, short *nDots,
				Boolean *setPDur,			/* Set play durations? */
				short *pDurPct,
				Boolean *cptV,				/* Compact Voices afterwards? */
				Boolean *doUnbeam			/* Unbeam selection first? */
				)	
{
	DialogPtr		dlog;
	GrafPtr			oldPort;
	short			newnDots, newSetLDur,
					oldResFile, newLDur, choice;
	short			ditem=0;
	short			type, newpDurPct;
	Handle			hndl;
	POPKEY			*pk;
	Rect			box;
	Boolean			beamed, done;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(SetDurFilter);
	if (filterUPP == NULL) {
		MissingDialog(SETDUR_DLOG);
		return FALSE;
	}
	
	dlog = GetNewDialog(SETDUR_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SETDUR_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);							/* popup code uses Get1Resource */

	if (*nDots>2) {
		SysBeep(1);										/* popup has 2 dots maximum */
		LogPrintf(LOG_WARNING, "SetDurDialog: can't handle %d dots.\n", *nDots);
		 *nDots = 2;
	}

	*doUnbeam = FALSE;
	beamed = (SelBeamedNote(doc)!=NILINK);

	newLDur = *lDur;
	newnDots = *nDots;
	newpDurPct = *pDurPct;
	if (newnDots>1) show2dots = TRUE;
	
	if (*lDurAction==HALVE_DURS)		setDurGroup = HALVEDURS_DI;
	else if (*lDurAction==DOUBLE_DURS)	setDurGroup = DOUBLEDURS_DI;
	else								setDurGroup = SETDURSTO_DI;
	PutDlgChkRadio(dlog, setDurGroup, TRUE);

	durPop1dot.menu = durPop2dot.menu = NULL;					/* NULL makes any goto broken safe */
	durPop1dot.itemChars = durPop2dot.itemChars = NULL;
	popKeys1dot = popKeys2dot = NULL;

	GetDialogItem(dlog, SDDURPOP_DI, &type, &hndl, &box);
	if (!InitGPopUp(&durPop1dot, TOP_LEFT(box), ONEDOTDUR_MENU, 1)) goto broken;
	if (!InitGPopUp(&durPop2dot, TOP_LEFT(box), TWODOTDUR_MENU, 1)) goto broken;
	popKeys1dot = InitDurPopupKey(&durPop1dot);
	if (popKeys1dot==NULL) goto broken;
	popKeys2dot = InitDurPopupKey(&durPop2dot);
	if (popKeys2dot==NULL) goto broken;
	curPop = show2dots? &durPop2dot : &durPop1dot;
	
	choice = GetDurPopItem(curPop, show2dots?popKeys2dot:popKeys1dot, newLDur, newnDots);
	if (choice==NOMATCH) choice = 1;
	SetGPopUpChoice(curPop, choice);

	PutDlgChkRadio(dlog, SHOW2DOTS_DI, show2dots);
	PutDlgChkRadio(dlog, SETLDUR_DI, *setLDur);
	PutDlgChkRadio(dlog, SETPDUR_DI, *setPDur);
	hndl = PutDlgChkRadio(dlog, CV_DI, *cptV);
	HiliteControl((ControlHandle)hndl, (*setLDur? CTL_ACTIVE : CTL_INACTIVE));
	
	PutDlgWord(dlog, PDURPCT_DI, *pDurPct, FALSE);

	if (*setLDur) {
		XableLDurPanel(dlog, TRUE);
		SelectDialogItemText(dlog, DUMMYFLD_DI, 0, ENDTEXT);
	}
	else {
		XableLDurPanel(dlog, FALSE);
		SelectDialogItemText(dlog, PDURPCT_DI, 0, ENDTEXT);
	}

	CenterWindow(GetDialogWindow(dlog), 75);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	done = FALSE;
	while (!done) {
		ModalDialog(filterUPP, &ditem);

		switch (ditem) {
			case OK:
				show2dots = GetDlgChkRadio(dlog, SHOW2DOTS_DI);
				pk = show2dots? popKeys2dot : popKeys1dot;
				newLDur = pk[curPop->currentChoice].durCode;
				newnDots = pk[curPop->currentChoice].numDots;
				GetDlgWord(dlog, PDURPCT_DI, &newpDurPct);
				newSetLDur = GetDlgChkRadio(dlog, SETLDUR_DI);
				
				if (setDurGroup==HALVEDURS_DI)
					*lDurAction = HALVE_DURS;
				else if (setDurGroup==DOUBLEDURS_DI)
					*lDurAction = DOUBLE_DURS;
				else
					*lDurAction = SET_DURS_TO;

				if (!SDAnyBadValues(doc, dlog, newSetLDur, *lDurAction, newnDots, newpDurPct)) {
					/* If any selected note is beamed, tell user it must be unbeamed if the
						logical durations are to be set. */
			
					if (newSetLDur && beamed) {
						if (CautionAdvise(SDBEAM_ALRT)==Cancel) break;
						*doUnbeam = TRUE;						
					}
					*setLDur = newSetLDur;
					*lDur = newLDur;
					*nDots = newnDots;
					*setPDur = GetDlgChkRadio(dlog, SETPDUR_DI);
					*pDurPct = newpDurPct;
					*cptV = GetDlgChkRadio(dlog, CV_DI);
					done = TRUE;
				}
				break;
			case Cancel:
				done = TRUE;
				break;
			case HALVEDURS_DI:
			case DOUBLEDURS_DI:
			case SETDURSTO_DI:
				if (ditem!=setDurGroup)
					SwitchRadio(dlog, &setDurGroup, ditem);
				break;
			case SETLDUR_DI:
				PutDlgChkRadio(dlog, SETLDUR_DI, !GetDlgChkRadio(dlog, SETLDUR_DI));
				newSetLDur = GetDlgChkRadio(dlog, SETLDUR_DI);
				if (newSetLDur) {
					XableLDurPanel(dlog, TRUE);
				}
				else {
					XableLDurPanel(dlog, FALSE);
					SelectDialogItemText(dlog, PDURPCT_DI, 0, ENDTEXT);
					HiliteGPopUp(curPop, popUpHilited = FALSE);
				}
				break;
			case SDDURPOP_DI:
				SelectDialogItemText(dlog, DUMMYFLD_DI, 0, ENDTEXT);
				HiliteGPopUp(curPop, popUpHilited = TRUE);
				PutDlgChkRadio(dlog, SETLDUR_DI, newSetLDur = TRUE);
				XableLDurPanel(dlog, TRUE);
				if (setDurGroup!=SETDURSTO_DI)
					SwitchRadio(dlog, &setDurGroup, SETDURSTO_DI);
				break;
			case SHOW2DOTS_DI:
				PutDlgChkRadio(dlog, SHOW2DOTS_DI, !GetDlgChkRadio(dlog, SHOW2DOTS_DI));
				show2dots = GetDlgChkRadio(dlog, SHOW2DOTS_DI);
				choice = curPop->currentChoice;
				if (show2dots) {
					choice = GetDurPopItem(&durPop2dot, popKeys2dot,
										popKeys1dot[choice].durCode, popKeys1dot[choice].numDots);
					curPop = &durPop2dot;
				}
				else {
					newnDots = popKeys2dot[choice].numDots;
					if (newnDots==2) newnDots--;
					choice = GetDurPopItem(&durPop1dot, popKeys1dot,
										popKeys2dot[choice].durCode, newnDots);
					curPop = &durPop1dot;
				}
				SetGPopUpChoice(curPop, choice);
				SelectDialogItemText(dlog, DUMMYFLD_DI, 0, ENDTEXT);
				HiliteGPopUp(curPop, popUpHilited = TRUE);
				break;
			case CV_DI:
			case SETPDUR_DI:
				PutDlgChkRadio(dlog, ditem, !GetDlgChkRadio(dlog, ditem));
				break;
			case PDURPCT_DI:
				if (popUpHilited)
					HiliteGPopUp(curPop, popUpHilited = FALSE);
				PutDlgChkRadio(dlog, SETPDUR_DI, TRUE);
				break;
		}
	}
broken:			
	DisposeGPopUp(&durPop1dot);
	DisposeGPopUp(&durPop2dot);
	if (popKeys1dot) DisposePtr((Ptr)popKeys1dot);
	if (popKeys2dot) DisposePtr((Ptr)popKeys2dot);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}


/* ----------------------------------------------- Tempo Dialog & helper functions -- */

static pascal Boolean TempoFilter(DialogPtr, EventRecord *, short *);
static void DimOrUndimMMNumberEntry(DialogPtr dlog, Boolean undim, unsigned char *metroStr);
static Boolean AllIsWell(void);

static enum
{
	VerbalDI=3,
	TDurPopDI,
	MetroDI,
	ShowMMDI,
	UseMMDI=9,
	EqualsDI,
	TDummyFldDI=11,
	ExpandDI
} E_TempoItems;

static pascal Boolean TempoFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr	w;
	short		ch, field, ans;
	Point		where;
	GrafPtr		oldPort;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));			
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, OK, TRUE);
				DrawGPopUp(curPop);		
				HiliteGPopUp(curPop, popUpHilited);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return TRUE;
			}
			break;
		case activateEvt:
			if (w==GetDialogWindow(dlog))
				SetPort(GetDialogWindowPort(dlog));
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			if (PtInRect(where, &curPop->box)) {
				DoGPopUp(curPop);
				*itemHit = TDurPopDI;
				return TRUE;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, FALSE))
				return TRUE;
			ch = (unsigned char)evt->message;
			/*
			 * The Dialog Manager considers only EditText fields as candidates for being
			 *	activated by the tab key, so handle tabbing from field to field ourselves
			 *	so user can direct keystrokes to the pop-up as well as the EditText fields.
			 */
			field = GetDialogKeyboardFocusItem(dlog);
			if (ch=='\t') {
				if (field==VerbalDI) field = TDummyFldDI;
				else if (field==TDummyFldDI) field = MetroDI;
				else field = VerbalDI;
				popUpHilited = (field==TDummyFldDI);
				SelectDialogItemText(dlog, field, 0, ENDTEXT);
				HiliteGPopUp(curPop, popUpHilited);
				*itemHit = 0;
				return TRUE;
			}
			else {
				if (field==TDummyFldDI) {
					ans = DurPopupKey(curPop, popKeys1dot, ch);
					*itemHit = ans? TDurPopDI : 0;
					HiliteGPopUp(curPop, TRUE);
					return TRUE;					/* so no chars get through to TDummyFldDI edit field */
				}									/* NB: however, DlgCmdKey will let you paste into TDummyFldDI! */
			}
			break;
	}
	
	return FALSE;
}


/* Disable and dim the number-entry field and fields subordinate to it, or enable and
undim them. */

static void DimOrUndimMMNumberEntry(DialogPtr dlog, Boolean undim, unsigned char *metroStr)
{
	short	type, newType;
	Handle	hndl;
	Rect	box;

	newType = (undim? editText : statText+itemDisable);
	//LogPrintf(LOG_DEBUG, "DimOrUndimMMNumberEntry: newType=%d\n", newType);
	GetDialogItem(dlog, MetroDI, &type, &hndl, &box);
	SetDialogItem(dlog, MetroDI, newType, hndl, &box);

	if (undim) {
		ShowDialogItem(dlog, TDurPopDI);
		ShowDialogItem(dlog, EqualsDI);
		ShowDialogItem(dlog, MetroDI);
		XableControl(dlog, ShowMMDI, TRUE);
	}
	else {
		HideDialogItem(dlog, TDurPopDI);
		HideDialogItem(dlog, EqualsDI);
		HideDialogItem(dlog, MetroDI);
		XableControl(dlog, ShowMMDI, FALSE);
	}
}


static Boolean AllIsWell(void)
{
	short expandedSetting, maxLenExpanded, type, i;
	Handle hndl; Rect box;
	DialogPtr dlog;
	long beatsPM; 
	Str255 str, metStr;
	char fmtStr[256];
	Boolean strOkay = TRUE;

	GetDialogItem(dlog,VerbalDI, &type, &hndl, &box);
	GetDialogItemText(hndl, str);
	GetDialogItem(dlog, ExpandDI, &type, &hndl, &box);
	expandedSetting = GetControlValue((ControlHandle)hndl);
	if (expandedSetting!=0) {
		maxLenExpanded = (EXPAND_WIDER? 255/3 : 255/2);
		if (Pstrlen(str)>maxLenExpanded) strOkay = FALSE;
		else {
			for (i = 1; i <= Pstrlen(str); i++)
			if (str[i]==CH_NLDELIM) {
				strOkay = FALSE;
				break;
			}
		}
	}
	if (!strOkay) {
		GetIndCString(fmtStr, TEXTERRS_STRS, 2);		/* "Multiline strings and strings of..." */
		sprintf(strBuf, fmtStr, maxLenExpanded);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	GetDlgString(dlog, MetroDI, metStr);
	beatsPM = FindIntInString(metStr);
	if (beatsPM<MIN_BPM || beatsPM>MAX_BPM) {
		if (beatsPM<0L)
			GetIndCString(strBuf, DIALOGERRS_STRS, 17); 	/* "M.M. field must contain a number" */ 
		else {
			GetIndCString(fmtStr, DIALOGERRS_STRS, 18);		/* "%ld beats per minute is illegal" */
			sprintf(strBuf, fmtStr, beatsPM, MIN_BPM, MAX_BPM);
		}
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		SelectDialogItemText(dlog, MetroDI, 0, ENDTEXT);	/* Select field so user knows which one is bad. */
		return FALSE;
	}
	
	GetDlgString(dlog, VerbalDI, str);
	if (Pstrlen(str)==0 && !GetDlgChkRadio(dlog, UseMMDI)) {
		GetIndCString(strBuf, DIALOGERRS_STRS, 23);			/* "No tempo string and no M.M. is illegal" */ 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		SelectDialogItemText(dlog, MetroDI, 0, ENDTEXT);	/* Select field so user knows which one is bad. */
		return FALSE;
	}
	
	return TRUE;
}


/* Dialog to get Tempo and metronome mark parameters from user. */

Boolean TempoDialog(Boolean *useMM, Boolean *showMM, short *dur, Boolean *dotted, Boolean *expanded,
						unsigned char *tempoStr, unsigned char *metroStr)
{	
	DialogPtr dlog;
	short ditem=Cancel, type, oldResFile;
	short choice, newLDur, oldLDur, k;
	Boolean dialogOver, oldDotted;
	Handle hndl; Rect box;
	GrafPtr oldPort; POPKEY *pk;
	ModalFilterUPP filterUPP;

	filterUPP = NewModalFilterUPP(TempoFilter);
	if (filterUPP == NULL) {
		MissingDialog(TEMPO_DLOG);
		return FALSE;
	}
	
	dlog = GetNewDialog(TEMPO_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(TEMPO_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);								/* popup code uses Get1Resource */

	durPop1dot.menu = NULL;									/* NULL makes any goto broken safe */
	durPop1dot.itemChars = NULL;
	popKeys1dot = NULL;

	GetDialogItem(dlog, TDurPopDI, &type, &hndl, &box);
	if (!InitGPopUp(&durPop1dot, TOP_LEFT(box), ONEDOTDUR_MENU, 1)) goto broken;
	popKeys1dot = InitDurPopupKey(&durPop1dot);
	if (popKeys1dot==NULL) goto broken;
	curPop = &durPop1dot;
	
	choice = GetDurPopItem(curPop, popKeys1dot, *dur, (*dotted? 1 : 0));
	if (choice==NOMATCH) choice = 1;
	SetGPopUpChoice(curPop, choice);
	oldLDur = *dur;
	oldDotted = *dotted;

	/* Replace CH_CR with the UI "start new line" code. */
	for (k=1; k<=Pstrlen(tempoStr); k++)
		if (tempoStr[k]==CH_CR) tempoStr[k] = CH_NLDELIM;

	PutDlgChkRadio(dlog, UseMMDI, *useMM);
	PutDlgChkRadio(dlog, ShowMMDI, *showMM);
	PutDlgChkRadio(dlog, ExpandDI, *expanded);
	PutDlgString(dlog, VerbalDI, tempoStr, FALSE);
	PutDlgString(dlog, MetroDI, metroStr, TRUE);
	
	/* If the M.M. option isn't chosen, disable and dim the number-entry field */
	DimOrUndimMMNumberEntry(dlog, *useMM, metroStr);

	SelectDialogItemText(dlog, (popUpHilited? TDummyFldDI : VerbalDI), 0, ENDTEXT);

	CenterWindow(GetDialogWindow(dlog), 70);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	dialogOver = FALSE;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
				if (AllIsWell()) dialogOver = TRUE;
				break;
			case Cancel:
				dialogOver = TRUE;
				break;
			case TDurPopDI:
	 			/*
	 			 *	If user just set the popup to unknown duration, which makes no sense in a
	 			 * metronome mark, change it back to what it was.
	 			 */
				newLDur = popKeys1dot[curPop->currentChoice].durCode;
				if (newLDur==UNKNOWN_L_DUR) {
					newLDur = oldLDur;
					choice = GetDurPopItem(curPop, popKeys1dot, newLDur, (oldDotted? 1 : 0));
					if (choice==NOMATCH) choice = 1;
					SetGPopUpChoice(curPop, choice);
				}
				oldLDur = newLDur;
				oldDotted = (popKeys1dot[curPop->currentChoice].numDots>0);
				break;
			case UseMMDI:
				PutDlgChkRadio(dlog, UseMMDI, !GetDlgChkRadio(dlog, UseMMDI));
				DimOrUndimMMNumberEntry(dlog, GetDlgChkRadio(dlog, UseMMDI), metroStr);
				break;
			case ShowMMDI:
				PutDlgChkRadio(dlog, ShowMMDI, !GetDlgChkRadio(dlog, ShowMMDI));
				break;
			case ExpandDI:
				PutDlgChkRadio(dlog, ExpandDI, !GetDlgChkRadio(dlog, ExpandDI));
				break;
			default:
				;
			}
		}
	if (ditem==OK) {
		pk = popKeys1dot;
		*dur = pk[curPop->currentChoice].durCode;
		*dotted = (pk[curPop->currentChoice].numDots>0);
		*useMM = GetDlgChkRadio(dlog, UseMMDI);
		*showMM = GetDlgChkRadio(dlog, ShowMMDI);
		*expanded = GetDlgChkRadio(dlog, ExpandDI);
		
		/* In the tempo string, replace the UI "start new line" code with CH_CR. */
		GetDlgString(dlog, VerbalDI, tempoStr);
char strC[256];	Pstrcpy((unsigned char *)strC, tempoStr); PToCString((unsigned char *)strC);
LogPrintf(LOG_DEBUG, "TempoDialog OK1: len=%d tempoStr='%s'\n", Pstrlen(tempoStr), strC);
		for (k=1; k<=Pstrlen(tempoStr); k++)
			if (tempoStr[k]==CH_NLDELIM) tempoStr[k] = CH_CR;
Pstrcpy((unsigned char *)strC, tempoStr); PToCString((unsigned char *)strC);
LogPrintf(LOG_DEBUG, "TempoDialog OK2: len=%d tempoStr='%s'\n", Pstrlen(tempoStr), strC);

		GetDlgString(dlog, MetroDI, metroStr);
	}
		
broken:
	DisposeGPopUp(&durPop1dot);
	if (popKeys1dot) DisposePtr((Ptr)popKeys1dot);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}


/* -------------------------------------------------------------- SetMBRestDialog -- */

static enum {
	 MBR_NMEAS_DI=3,
	 UP_MBR_DI=5,
	 DOWN_MBR_DI
} E_MBRestItems; 

Boolean SetMBRestDialog(Document */*doc*/, short *nMeas)
{
	DialogPtr dlog;
	GrafPtr oldPort;
	short newNMeas, ditem;
	Boolean done;
	ModalFilterUPP filterUPP;

	filterUPP = NewModalFilterUPP(NumberFilter);
	if (filterUPP == NULL) {
		MissingDialog(MBREST_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(MBREST_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		
		newNMeas = *nMeas;
		PutDlgWord(dlog, MBR_NMEAS_DI, newNMeas,TRUE);
		
		UseNumberFilter(dlog, MBR_NMEAS_DI, UP_MBR_DI, DOWN_MBR_DI);
		minVal = 1;
		maxVal = 127;
		
		CenterWindow(GetDialogWindow(dlog), 50);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
		
		done = FALSE;
		while (!done) {
			ModalDialog(filterUPP, &ditem);
			switch (ditem) {
				case OK:
					GetDlgWord(dlog, MBR_NMEAS_DI, &newNMeas);
					if (newNMeas<1 || newNMeas>127) {
						GetIndCString(strBuf, DIALOGERRS_STRS, 15);			/* "measures rest must be..." */
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
					}
					else {
						*nMeas = newNMeas;
						done = TRUE;
					}
					break;
				case Cancel:
					done = TRUE;
					break;
			}
		}
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
	}
	else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(MBREST_DLOG);
	}
	SetPort(oldPort);
	return (ditem==OK);
}


/* ============================ RastralDialog and friends =========================== */

static enum {
	SizeITM=3,										/* Staff Size dialog items */
	RspITM,
	StSampITM=6,
	rDownITM=8,
	rUpITM,
	PointsITM=11,
	SelPartsITM=13,
	AllPartsITM	
} E_RastralItems;

#define STAFF_DOWN 35							/* Vertical position of staff in user item */

DialogPtr rDialogp;								/* Dialog pointer for Rastral dialog */
Rect staffArea, rUpRect, rDownRect;
Handle ptHdl;

/* --------------------------------------------------------------- DrawSampleStaff -- */

#define GOOD_SCREENSIZE(r) ((r)==1 || (r)==2 || (r)==5 || (r)==7)

static void DrawSampleStaff()
{
	short		line, rastral, yp, mPoints,
				oldtxFont, oldtxSize;
	Point		pt;
	long		value;
	char		fmtStr[256];
	Rect		portRect;
	
	GetDlgWord(rDialogp, SizeITM, &rastral);			/* Get new staff size value */
	if (rastral<0 || rastral>MAXRASTRAL) return;		/* Ignore temporary illegal values */

	EraseRect(&staffArea);
	ClipRect(&staffArea);

	PenPat(NGetQDGlobalsGray());
	for (line=0; line<STFLINES; line++) {
		yp = staffArea.top+STAFF_DOWN+ 
				d2p(halfLn2d(2*line, p2d(pdrSize[rastral]), STFLINES));
		MoveTo(staffArea.left, yp);
		LineTo(staffArea.right-1, yp);
	}
	PenNormal();

	oldtxFont = GetPortTxFont();
	oldtxSize = GetPortTxSize();
	TextFont(sonataFontNum);								/* Use Sonata font */
	TextSize(Staff2MFontSize(drSize[rastral]));

	MoveTo(staffArea.left+5, staffArea.top+STAFF_DOWN+pdrSize[rastral]);
	DrawChar(MCH_trebleclef);
	GetPen(&pt);
	MoveTo(pt.h+5, staffArea.top+STAFF_DOWN+(pdrSize[rastral]/4));
	DrawChar(MCH_16thNoteStemDown);
	portRect = GetQDPortBounds();
	ClipRect(&portRect);

	if (GOOD_SCREENSIZE(rastral)) {
		MoveTo(staffArea.left+14, staffArea.top+20);
		SetFont(1);
		DrawCString("Sonata ");
		mPoints = Staff2MFontSize(drSize[rastral]);
		sprintf(strBuf, "%d", mPoints);
		DrawCString(strBuf);
	}
	TextFont(oldtxFont);
	TextSize(oldtxSize);

	value = 100L*pt2in(pdrSize[rastral]);
	GetIndCString(fmtStr, DIALOG_STRS, (rastral==0? 3 : 4));		/* "currently/= %d points = "... */
	sprintf(strBuf, fmtStr, pdrSize[rastral]);

	if (value/100L>=1) {
		GetIndCString(fmtStr, DIALOG_STRS, 8);							/* "%ld.%02ld in." */
		sprintf(&strBuf[strlen(strBuf)], fmtStr, value/100L, value%100L);
	}
	else {
		GetIndCString(fmtStr, DIALOG_STRS, 9);							/* ".%02ld in." */
		sprintf(&strBuf[strlen(strBuf)], fmtStr, value%100L);
	}

	SetDialogItemCText(ptHdl, strBuf);
}


/* --------------------------------------------------------- RLarger/SmallerStaff -- */

static void RLargerStaff()
{
	short rNum;
	
	GetDlgWord(rDialogp, SizeITM, &rNum);
	if (rNum>0) {
		rNum--;
		PutDlgWord(rDialogp, SizeITM, rNum, TRUE);
		DrawSampleStaff();
	}
}

static void RSmallerStaff()
{
	short rNum;
	
	GetDlgWord(rDialogp, SizeITM, &rNum);
	if (rNum<MAXRASTRAL) {
		rNum++;
		PutDlgWord(rDialogp, SizeITM, rNum, TRUE);
		DrawSampleStaff();
	}
}

/* ---------------------------------------------------- RHandleKeyDown/MouseDown -- */

static Boolean RHandleKeyDown(EventRecord *theEvent)
{
	char	theChar;

	theChar = theEvent->message & charCodeMask;
	if (theChar==UPARROWKEY) {
		RSmallerStaff();
		return TRUE;
	}
	else if (theChar==DOWNARROWKEY) {
		RLargerStaff();
		return TRUE;
	}
	else {
		DrawSampleStaff();
		return FALSE;
	}
}

static Boolean RHandleMouseDown(EventRecord *theEvent)
{
	Point	where;

	where = theEvent->where;
	GlobalToLocal(&where);
	if (PtInRect(where, &rUpRect)) {
		SelectDialogItemText(rDialogp, SizeITM, 0, ENDTEXT);		/* Select & unhilite number */
		TrackArrow(&rUpRect, &RLargerStaff);
		return TRUE;
	}
	else if (PtInRect(where, &rDownRect)) {
		SelectDialogItemText(rDialogp, SizeITM, 0, ENDTEXT);		/* Select & unhilite number */
		TrackArrow(&rDownRect, &RSmallerStaff);
		return TRUE;
	}
	else
		return FALSE;
}


/* --------------------------------------------------------------------- RFilter -- */

static pascal Boolean RFilter(DialogPtr theDialog, EventRecord *theEvent, short *itemHit)
{
	GrafPtr oldPort;

	switch (theEvent->what) {
		case updateEvt:
			WindowPtr w = GetDialogWindow(theDialog);
			if ((WindowPtr)theEvent->message == w) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
				BeginUpdate(GetDialogWindow(theDialog));
				UpdateDialogVisRgn(theDialog);
				DrawSampleStaff();
				OutlineOKButton(theDialog,TRUE);
				EndUpdate(GetDialogWindow(theDialog));
				SetPort(oldPort);
				}
#ifdef TRULY_ANNOYING_FOR_LARGE_SCORES
			else
				DoUpdate((WindowPtr)theEvent->message);
#endif
			*itemHit = 0;
			return TRUE;
			break;
		case mouseDown: 
			if (RHandleMouseDown(theEvent)) {									/* Click in elevator btn? Handle &... */
				SelectDialogItemText(rDialogp, SizeITM, 0, ENDTEXT);		/*   select & unhilite number */
				return FALSE;
			}
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, (short *)itemHit, FALSE))
				return TRUE;
			else {
				if (RHandleKeyDown(theEvent))										/* Arrow key? Handle it &... */
					SelectDialogItemText(rDialogp, SizeITM, 0, ENDTEXT);	/*   select & unhilite number */
				return FALSE;
			}
			break;
		default:
			;
	}
	return FALSE;
}


/* ---------------------------------------------------------------- RastralDialog -- */
/* Conduct dialog to get staff rastral size from user.  Returns result, or
CANCEL_INT for Cancel. */

#define STAFFBOX_LEFT 208								/* Staff sample Rect position */
#define STAFFBOX_TOP 18
#define STAFFBOX_LEN 60
#define STAFFBOX_BOTTOM (STAFFBOX_TOP+76)

short RastralDialog(
			Boolean	/*canChoosePart*/,		/* TRUE: enable "Sel parts/All parts" radio buttons */
			short	initval,				/* Initial (default) value */
			Boolean	*rsp,					/* Respace staves proportionally? */
			Boolean	*selPartsOnly			/* set only if canChoosePart is TRUE */
			)
{
	char	msg[16];
	short 	dialogOver, ditem, anInt, newval, radio;
	Handle 	rspHdl, aHdl;
	Rect 	aRect;
	GrafPtr	oldPort;
	ModalFilterUPP filterUPP;
	
	filterUPP = NewModalFilterUPP(RFilter);
	if (filterUPP==NULL) {
		MissingDialog(STAFFSIZE_DLOG);
		return CANCEL_INT;
	}

	GetPort(&oldPort);
	rDialogp = GetNewDialog(STAFFSIZE_DLOG, NULL, BRING_TO_FRONT);
	if (!rDialogp) {
		MissingDialog(STAFFSIZE_DLOG);
		DisposeModalFilterUPP(filterUPP);
		return CANCEL_INT;
	}
	SetPort(GetDialogWindowPort(rDialogp));

	/*
	 * Get the sample staff rectangle, as defined by a user item, and get it out of
	 * the way so it doesn't hide any items underneath.
	 */
	GetDialogItem(rDialogp, StSampITM, &anInt, &aHdl, &staffArea);		
	HideDialogItem(rDialogp, StSampITM);

	// Just draw it directly in update part of filter
	// SetDialogItem(rDialogp, StSampITM, 0, (Handle)UserDrawStaff, &staffArea);

	PutDlgWord(rDialogp, SizeITM, initval, TRUE);					/* Fill in initial value */
	GetDialogItem(rDialogp, PointsITM, &anInt, &ptHdl, &aRect);
	
	GetDialogItem(rDialogp, RspITM, &anInt, &rspHdl, &aRect);
	SetControlValue((ControlHandle)rspHdl, 1);						/* Initially set prop.respace */

	GetDialogItem(rDialogp, rDownITM, &anInt, &aHdl, &rDownRect);
	GetDialogItem(rDialogp, rUpITM, &anInt, &aHdl, &rUpRect);

	HideDialogItem(rDialogp, SelPartsITM);
	HideDialogItem(rDialogp, AllPartsITM);
	
	CenterWindow(GetDialogWindow(rDialogp), 70);
	ShowWindow(GetDialogWindow(rDialogp));
	ArrowCursor();
	
	dialogOver = 0;
	while (dialogOver==0) {
		do {
			ModalDialog(filterUPP, &ditem);
			switch (ditem) {
				case OK:
				case Cancel:
					dialogOver = ditem;
					break;
				case RspITM:
					SetControlValue((ControlHandle)rspHdl,1-GetControlValue((ControlHandle)rspHdl));	/* Toggle prop. respace */
					break;
				case SizeITM:
				case StSampITM:
					GetDlgWord(rDialogp, SizeITM, &newval);
					if (newval<0 || newval>MAXRASTRAL) SysBeep(1);
					else							   DrawSampleStaff();
					break;
				case SelPartsITM:
				case AllPartsITM:
					if (ditem!=radio) {
						SwitchRadio(rDialogp, &radio, ditem);
						*selPartsOnly = (ditem==SelPartsITM);
					}
					break;
				default:
			 		;
			}
		} while (!dialogOver);
		
		if (dialogOver==Cancel) {
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(rDialogp);
			SetPort(oldPort);
			return CANCEL_INT;
		}
		else {
			GetDlgWord(rDialogp, SizeITM, &newval);
			if (newval<0 || newval>MAXRASTRAL) {
				sprintf(msg, "%d", MAXRASTRAL);
				CParamText(msg, "", "", "");
				StopInform(RASTRAL_ALRT);
				dialogOver = 0;									/* Keep dialog on screen */
			}
		}
	}

	GetDialogItem(rDialogp, RspITM, &anInt, &rspHdl, &aRect);
	*rsp = GetControlValue((ControlHandle)rspHdl);				/* Pick up "respace" flag */
	DisposeDialog(rDialogp);									/* Free heap space */
	DisposeModalFilterUPP(filterUPP);
	SetPort(oldPort);
	return newval; 
}


/* =========================== StaffLinesDialog and friends ========================= */

static enum {
	STAFFLINES_DI=5,
	SHOWLEDGERS_DI,
	SELPARTS_DI,
	ALLPARTS_DI
} E_SLPartsItems;

static enum {		/* popup menu item numbers */
	SIX_LINES=1,
	FIVE_LINES,
	FOUR_LINES,
	THREE_LINES,
	TWO_LINES,
	ONE_LINE,
	ZERO_LINES
} E_StaffLinesItems;

static UserPopUp staffLinesPopUp;
static pascal Boolean SLFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static short StaffLines2MenuItemNum(short numLines);
static short MenuItemNum2StaffLines(short itemNum);

static pascal Boolean SLFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Point where;
	Boolean ans=FALSE; WindowPtr w;
	short choice;

	w = (WindowPtr)(evt->message);
	switch(evt->what) {
		case updateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetWindowPort(w));
				BeginUpdate(GetDialogWindow(dlog));
				UpdateDialogVisRgn(dlog);
				DrawPopUp(&staffLinesPopUp);
				OutlineOKButton(dlog, TRUE);
				EndUpdate(GetDialogWindow(dlog));
				ans = TRUE; *itemHit = 0;
			}
			else
				DoUpdate(w);
			break;
		case activateEvt:
			if (w == GetDialogWindow(dlog)) {
				short activ = (evt->modifiers & activeFlag)!=0;
				SetPort(GetWindowPort(w));
			}
			break;
		case mouseDown:
			where = evt->where;
			choice = staffLinesPopUp.currentChoice;
			GlobalToLocal(&where);
			if (PtInRect(where, &staffLinesPopUp.shadow)) {
				*itemHit = DoUserPopUp(&staffLinesPopUp) ? STAFFLINES_DI : 0;
				ans = TRUE;
				break;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, itemHit, FALSE))
				return TRUE;
			break;
	}
	return ans;
}

static short StaffLines2MenuItemNum(short numLines)
{
	switch (numLines) {
		case 0:	return ZERO_LINES;
		case 1:	return ONE_LINE;
		case 2:	return TWO_LINES;
		case 3:	return THREE_LINES;
		case 4:	return FOUR_LINES;
		case 5:	return FIVE_LINES;
		case 6:	return SIX_LINES;
		default:
			AlwaysErrMsg("StaffLines2MenuItemNum: numLines invalid");
			return FIVE_LINES;
	}
}

static short MenuItemNum2StaffLines(short itemNum)
{
	switch (itemNum) {
		case ZERO_LINES:	return 0;
		case ONE_LINE:		return 1;
		case TWO_LINES:		return 2;
		case THREE_LINES:	return 3;
		case FOUR_LINES:	return 4;
		case FIVE_LINES:	return 5;
		case SIX_LINES:		return 6;
		default:
			AlwaysErrMsg("MenuItemNum2StaffLines: itemNum is invalid");
			return 5;
	}
}

/* ------------------------------------------------------------- StaffLinesDialog -- */
/* Conduct dialog to get from user the number of staff lines, whether to show
ledger lines, and whether to make these changes for all parts or only selected
parts. Returns TRUE if OK, FALSE if user cancelled or there was an error. */

short StaffLinesDialog(
			Boolean	canChoosePart,		/* TRUE: enable "Sel parts/All parts" radio buttons */
			short		*staffLines,
			Boolean	*showLedgers,
			Boolean	*selPartsOnly		/* set only if canChoosePart is TRUE */
			)	
{
	short 		dialogOver, ditem, radio, retval = FALSE;
	DialogPtr	dlog;
	GrafPtr		oldPort;
	ModalFilterUPP filterUPP;
	
	filterUPP = NewModalFilterUPP(SLFilter);
	if (filterUPP==NULL) {
		MissingDialog(STAFFLINES_DLOG);
		return CANCEL_INT;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(STAFFLINES_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		MissingDialog(STAFFLINES_DLOG);
		DisposeModalFilterUPP(filterUPP);
		return CANCEL_INT;
	}
	SetPort(GetDialogWindowPort(dlog));

	staffLinesPopUp.currentChoice = StaffLines2MenuItemNum(*staffLines);
	if (!InitPopUp(dlog, &staffLinesPopUp, STAFFLINES_DI, 0, STAFFLINES_MENU,
												staffLinesPopUp.currentChoice)) {
		AlwaysErrMsg("StaffLinesDialog: failure initializing popup menu");
		goto broken;
	}
	
	PutDlgChkRadio(dlog, SHOWLEDGERS_DI, *showLedgers);

	if (canChoosePart) {
		radio = (*selPartsOnly? SELPARTS_DI : ALLPARTS_DI);
		PutDlgChkRadio(dlog, radio, TRUE);
	}
	else {
		PutDlgChkRadio(dlog, ALLPARTS_DI, TRUE);
		XableControl(dlog, SELPARTS_DI, FALSE);
		XableControl(dlog, ALLPARTS_DI, FALSE);
	}
	
	CenterWindow(GetDialogWindow(dlog), 70);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	dialogOver = 0;
	while (dialogOver==0) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
			case Cancel:
				dialogOver = ditem;
				break;
			case STAFFLINES_DI:
				break;
			case SHOWLEDGERS_DI:
				PutDlgChkRadio(dlog, ditem, !GetDlgChkRadio(dlog, ditem));
				break;
			case SELPARTS_DI:
			case ALLPARTS_DI:
				if (ditem!=radio) {
					SwitchRadio(dlog, &radio, ditem);
					*selPartsOnly = (ditem==SELPARTS_DI);
				}
				break;
			default:
			 	;
		}
	}
	if (dialogOver==OK) {
		*staffLines = MenuItemNum2StaffLines(staffLinesPopUp.currentChoice);
		*showLedgers = GetDlgChkRadio(dlog, SHOWLEDGERS_DI);
		retval = TRUE;
	}

broken:
	DisposeDialog(dlog);
	DisposeModalFilterUPP(filterUPP);
	SetPort(oldPort);
	return retval; 
}


/* ---------------------------------------------------------------- MarginsDialog -- */

static enum {
	LMARG_DI=3,
	TMARG_DI,
	RMARG_DI,
	BMARG_DI,
	WIDTH_DI=13,
	HEIGHT_DI=15
} E_MarginsItems;

/* Conduct dialog to set margins. Passes margins in parameters; returns TRUE for
OK, FALSE for Cancel. */

Boolean MarginsDialog(Document *doc,
				short *lMarg, short *tMarg,	/* On entry, old margins; on exit, new ones */
				short *rMarg, short *bMarg
				)
{	
	DialogPtr	dlog;
	GrafPtr		oldPort;
	short		value, ditem, papHeight, papWidth;
	double		inchLMarg, inchTMarg, inchRMarg, inchBMarg, inchPapHeight, inchPapWidth,
				fTemp;
	Boolean		stillGoing=TRUE;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(MARGINS_DLOG);
		return FALSE;
	}
	
	GetPort(&oldPort);
	
	dlog = GetNewDialog(MARGINS_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		
		papHeight = doc->origPaperRect.bottom-doc->origPaperRect.top;
		inchPapHeight = (FASTFLOAT)papHeight/POINTSPERIN;
		inchPapHeight = RoundDouble(inchPapHeight, .01);
		PutDlgDouble(dlog, HEIGHT_DI, inchPapHeight, FALSE);
		papWidth = doc->origPaperRect.right-doc->origPaperRect.left;
		inchPapWidth = (FASTFLOAT)papWidth/POINTSPERIN;
		inchPapWidth = RoundDouble(inchPapWidth, .01);
		PutDlgDouble(dlog, WIDTH_DI, inchPapWidth, FALSE);

		inchTMarg = (FASTFLOAT)*tMarg/POINTSPERIN;
		inchTMarg = RoundDouble(inchTMarg, .01);
		PutDlgDouble(dlog, TMARG_DI, inchTMarg, FALSE);
		
		inchRMarg = (FASTFLOAT)*rMarg/POINTSPERIN;
		inchRMarg = RoundDouble(inchRMarg, .01);
		PutDlgDouble(dlog, RMARG_DI, inchRMarg, FALSE);
		
		inchBMarg = (FASTFLOAT)*bMarg/POINTSPERIN;
		inchBMarg = RoundDouble(inchBMarg, .01);
		PutDlgDouble(dlog, BMARG_DI, inchBMarg, FALSE);

		inchLMarg = (FASTFLOAT)*lMarg/POINTSPERIN;
		inchLMarg = RoundDouble(inchLMarg, .01);
		PutDlgDouble(dlog, LMARG_DI, inchLMarg, TRUE);
		
		CenterWindow(GetDialogWindow(dlog), 70);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
		
		while (stillGoing) {
		
			do {
				ModalDialog(filterUPP, &ditem);
			} while (ditem!=OK && ditem!=Cancel);
				
			value = ditem;
			if (ditem == Cancel) break;

			stillGoing = FALSE;

			/* Get new margin values. Be careful to avoid roundoff error! */
			
			if (GetDlgDouble(dlog, LMARG_DI, &fTemp)) {
				if (ABS(fTemp-inchLMarg)>.001) {
					*lMarg = (fTemp*POINTSPERIN)+.5;
				}
			}
			 else {
				StopInform(BADMARGIN_ALRT);
				stillGoing = TRUE;
			}

			if (GetDlgDouble(dlog, TMARG_DI, &fTemp)) {
				if (ABS(fTemp-inchTMarg)>.001) {
					*tMarg = (fTemp*POINTSPERIN)+.5;
				}
			}
			 else {
				StopInform(BADMARGIN_ALRT);
				stillGoing = TRUE;
			}

			if (GetDlgDouble(dlog, RMARG_DI, &fTemp)) {
				if (ABS(fTemp-inchRMarg)>.001) {
					*rMarg = (fTemp*POINTSPERIN)+.5;
				}
			}
			 else {
				StopInform(BADMARGIN_ALRT);
				stillGoing = TRUE;
			}

			if (GetDlgDouble(dlog, BMARG_DI, &fTemp)) {
				if (ABS(fTemp-inchBMarg)>.001) {
					*bMarg = (fTemp*POINTSPERIN)+.5;
				}
			}
			 else {
				StopInform(BADMARGIN_ALRT);
				stillGoing = TRUE;
			}
				
			/* If any of the values are negative or they result in negative paper sizes,
				do not allow the user to dismiss the dialog. */
			if (*lMarg<0 || *tMarg<0 || *rMarg<0 || *bMarg<0) {
					StopInform(BADMARGIN_ALRT);
					stillGoing = TRUE;
				}
			else if (doc->origPaperRect.right-*rMarg < *lMarg
			||  doc->origPaperRect.bottom-*bMarg < *tMarg) {
					StopInform(BADMARGIN_ALRT);
					stillGoing = TRUE;
				}

			}
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(dlog);											/* Free heap space */
		}
	else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(MARGINS_DLOG);
		value = Cancel;
	}
		
	SetPort(oldPort);
	return (value==OK);
}


/* =============== LOCAL TYPES, VARIABLES & FUNCTIONS for KeySigDialog ============= */

static enum {									/* Key Signature Dialog items */
	iKSUp=5,
	iKSDown,
	iKSStaff,
	iKSInsertIt,
	iKSThisStaff,
	iKSAllStaves,
	iKSReplaceIt
} E_KSItems;

short			sharps, flats;
Rect			ksStaffRect, ksUpRect, ksDownRect;
static short	sharpList[8] = {0, 9, 6, 10, 7, 4, 8, 5},
				flatList[8] = {0, 5, 8, 4, 7, 3, 6, 2};

static pascal Boolean KSFilter(DialogPtr, EventRecord *, short *);
static void KSDrawStaff(void);
static Boolean KSHandleKeyDown(EventRecord *);
static Boolean KSHandleMouseDown(EventRecord *);
static void KSMoreFlats(void);
static void KSMoreSharps(void);

/* ---------------------------------------------------------------- KeySigDialog -- */
/*	Handle dialog box to get key signature from user.  Returns TRUE if
user accepts dialog via OK, or FALSE if they CANCEL.  If <insert> is TRUE,
user is inserting a new keysig; else they're modifying an existing keysig.
Usage:	if KeySigDialog(&newSharps, &newFlats, &onAllStaves) {
				sharps = newSharps;
				flats = newFlats;
			}
Created: 4/28/87, dbw. */

Boolean KeySigDialog(short *sharpParam, short *flatParam, Boolean *onAllStaves,
								Boolean insert)
{
	DialogPtr		theDialog;
	short			aShort, itemHit;
	short			radio;
	Handle			tHdl;
	GrafPtr			oldPort;
	Boolean			done, result;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(KSFilter);
	if (filterUPP == NULL) {
		MissingDialog(KEYSIG_DLOG);
		return FALSE;
	}

	result = FALSE;
	GetPort(&oldPort);
	
	theDialog = GetNewDialog(KEYSIG_DLOG, 0L, BRING_TO_FRONT);
	if (theDialog) {
		SetPort(GetDialogWindowPort(theDialog));
		GetDialogItem(theDialog, iKSUp, &aShort, &tHdl, &ksUpRect);
		GetDialogItem(theDialog, iKSDown, &aShort, &tHdl, &ksDownRect);
		GetDialogItem(theDialog, iKSStaff, &aShort, &tHdl, &ksStaffRect);
		
		radio = (*onAllStaves? iKSAllStaves : iKSThisStaff);	/* Get initial all/this staves */
		PutDlgChkRadio(theDialog,radio,TRUE);
	
		if (insert) {
			ShowDialogItem(theDialog, iKSInsertIt);
			HideDialogItem(theDialog, iKSReplaceIt);
		}
		else {
			HideDialogItem(theDialog, iKSInsertIt);
			ShowDialogItem(theDialog, iKSReplaceIt);
		}
	
		ShowWindow(GetDialogWindow(theDialog));
		ArrowCursor();
	
		sharps = *sharpParam;											/* Get initial key signature */
		flats = *flatParam;
		KSDrawStaff();

		done = FALSE;
		do {
			ModalDialog(filterUPP, &itemHit);
			switch (itemHit) {
				case OK: 
					*sharpParam = sharps;
					*flatParam = flats;
					done = result = TRUE;
					break;
				case Cancel: 
					done = TRUE;
					result = FALSE;
					break;
				case iKSThisStaff:
				case iKSAllStaves:
					if (itemHit != radio) {
						SwitchRadio(theDialog, &radio, itemHit);
						*onAllStaves = (itemHit==iKSAllStaves);
					}
					break;
				default:
					;
				}
		} while (!done);
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(theDialog);
	}
	else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(KEYSIG_DLOG);
	}

	SetPort(oldPort);
	return result;
}


/* -------------------------------------------------------------- SetKSDialogGuts -- */

static enum {
	STAFFN_DI=4,
	ALLOW_CHANGES_DI=8
} E_KSGItems;

Boolean SetKSDialogGuts(short staffn, short *sharpParam, short *flatParam,
								Boolean *pCanChange)
{
	DialogPtr		dlog;
	short			aShort, itemHit;
	Handle			tHdl;
	GrafPtr			oldPort;
	Boolean			done, result;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(KSFilter);
	if (filterUPP == NULL) {
		MissingDialog(SETKEYSIG_DLOG);
		return FALSE;
	}

	result = FALSE;
	GetPort(&oldPort);
	
	dlog = GetNewDialog(SETKEYSIG_DLOG, 0L, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		GetDialogItem(dlog, iKSUp, &aShort, &tHdl, &ksUpRect);
		GetDialogItem(dlog, iKSDown, &aShort, &tHdl, &ksDownRect);
		GetDialogItem(dlog, iKSStaff, &aShort, &tHdl, &ksStaffRect);
		
		PutDlgWord(dlog, STAFFN_DI, staffn, FALSE);
		PutDlgChkRadio(dlog, ALLOW_CHANGES_DI, *pCanChange);
	
		PlaceWindow(GetDialogWindow(dlog), (WindowPtr)NULL, 0, 80);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		sharps = *sharpParam;											/* Get initial key signature */
		flats = *flatParam;
		KSDrawStaff();

		done = FALSE;
		do {
			ModalDialog(filterUPP, &itemHit);
			switch (itemHit) {
				case OK: 
					*sharpParam = sharps;
					*flatParam = flats;
					*pCanChange = GetDlgChkRadio(dlog, ALLOW_CHANGES_DI);
					done = result = TRUE;
					break;
				case Cancel: 
					done = TRUE;
					result = FALSE;
					break;
				case ALLOW_CHANGES_DI:
					PutDlgChkRadio(dlog, ALLOW_CHANGES_DI, !GetDlgChkRadio(dlog, ALLOW_CHANGES_DI));
					break;
				default:
					;
				}
		} while (!done);
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
	}
	else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(KEYSIG_DLOG);
	}

	SetPort(oldPort);
	return result;
}


/* ----------------------------------------------------------------- KSDrawStaff -- */
/*	Draw the KeySignature staff user item.  Expects <ksStaffRect>, <sharps> and
<flats> to be initialized. */

static void KSDrawStaff()
{
#define LineV(lineNum) (ksStaffRect.bottom-10-3*(lineNum))

	short	i;
	Point	pt;

	EraseRect(&ksStaffRect);
	PenPat(NGetQDGlobalsGray());
	for (i = 1; i<=5; i++) {
		MoveTo(ksStaffRect.left, LineV(2 * i - 1));
		Line(ksStaffRect.right - ksStaffRect.left - 10, 0);
	}
	PenNormal();
	TextFont(sonataFontNum);							/* Set to Sonata font */
	TextSize(18);
	MoveTo(ksStaffRect.left, LineV(1));
	DrawChar(MCH_trebleclef);							/* Treble clef */
	MoveTo(ksStaffRect.left + 18, 0);
	for (i=1; i<=sharps; i++) {
		GetPen(&pt);
		MoveTo(pt.h + 2, LineV(sharpList[i]) + 1);
		DrawChar(MCH_sharp);
	}
	for (i=1; i<=flats; i++) {
		GetPen(&pt);
		MoveTo(pt.h + 1, LineV(flatList[i]) + 1);
		DrawChar(MCH_flat);
	}
	TextFont(0);
	TextSize(0);
#undef LineV
}


/* -------------------------------------------------------------------- KSFilter -- */

static pascal Boolean KSFilter(DialogPtr theDialog, EventRecord *theEvent, short *itemHit)
{
	switch (theEvent->what) {
		case updateEvt:
				BeginUpdate(GetDialogWindow(theDialog));
				DrawDialog(theDialog);
				OutlineOKButton(theDialog, TRUE);
				EndUpdate(GetDialogWindow(theDialog));
				*itemHit = 0;
				return FALSE;
		case mouseDown: 
			if (KSHandleMouseDown(theEvent)) {
				*itemHit = 0;
				return FALSE;
			}
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, (short *)itemHit, FALSE))
				return TRUE;
			else {
				*itemHit = 0;
				return (KSHandleKeyDown(theEvent));
			}
			break;
		default:
			;
	}
	*itemHit = 0;
	return FALSE;
}


/* ------------------------------------------- KSHandleKeyDown,KSHandleMouseDown -- */

static Boolean KSHandleKeyDown(EventRecord *theEvent)
{
	char	theChar;

	theChar =theEvent->message & charCodeMask;
	if (theChar==UPARROWKEY) {
		KSMoreSharps();
		return TRUE;
	}
	else if (theChar==DOWNARROWKEY) {
		KSMoreFlats();
		return TRUE;
	}
	else
		return FALSE;
}

static Boolean KSHandleMouseDown(EventRecord *theEvent)
{
	long	t;
	Point	where;

	where = theEvent->where;
	t = theEvent->when;
	GlobalToLocal(&where);
	if (PtInRect(where, &ksUpRect)) {
		TrackArrow(&ksUpRect, &KSMoreSharps);
		return TRUE;
	}
	else if (PtInRect(where, &ksDownRect)) {
		TrackArrow(&ksDownRect, &KSMoreFlats);
		return TRUE;
	}
	else
		return FALSE;
}


/* -------------------------------------------------- KSMoreFlats, KSMoreSharps -- */
/*	Move <sharps> and <flats> one step in the direction of more flats. */

static void KSMoreFlats()
{
	if (flats<MAX_KSITEMS) {
		if (sharps) --sharps;
		else ++flats;
		KSDrawStaff();
	}
}

/*	Move <sharps> and <flats> one step in the direction of more sharps. */

static void KSMoreSharps()
{
	if (sharps<MAX_KSITEMS) {
		if (flats) --flats;
		else ++sharps;
		KSDrawStaff();
	}
}


/* =============== LOCAL TYPES, VARIABLES & FUNCTIONS for TimeSigDialog ============ */

static enum {								/* Time Signature Dialog items */
	iTSNumUp=6,
	iTSStaff,
	iTSNumDown,
	iTSDenomUp,
	iTSDenomDown,
	iTSInsertIt,
	iTSThisStaff=12,
	iTSAllStaves,
	iTSNormal=15,
	iTSC,
	iTSCut,
	iTSNOnly,
	iTSReplaceIt
} E_TSItems;

short	numerator, denominator, radio1, radio2;
Rect tsStaffRect, tsNumUpRect,tsNumDownRect, tsDenomUpRect, tsDenomDownRect;
ControlHandle cTimeHdl, cutTimeHdl;

/* ---------------------------------------------------------------- TimeSigDialog -- */
/*	Handle dialog box to get time signature from user.  Returns TRUE if user
accepts dialog via OK, or FALSE if they CANCEL. All parameters are both inputs
and outputs. If <insert> is TRUE, user is inserting a new timesig; else they're
modifying an existing timesig.
Created: 4/28/87, dbw. */

Boolean TimeSigDialog(short *type, short *numParam, short *denomParam,
								Boolean *onAllStaves, Boolean insert)
{
	DialogPtr		theDialog;
	short			aShort, itemHit;
	Handle			item;
	Rect			aRect;
	GrafPtr			oldPort;
	Boolean			done, result;
	char			fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(TSFilter);
	if (filterUPP == NULL) {
		MissingDialog(TIMESIG_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
 
  	theDialog = GetNewDialog(TIMESIG_DLOG, 0L, BRING_TO_FRONT);
	if (!theDialog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(TIMESIG_DLOG);
  		return FALSE;
  	}
  	
	SetPort(GetDialogWindowPort(theDialog));
	GetDialogItem(theDialog, iTSNumUp, &aShort, &item, &tsNumUpRect);
	GetDialogItem(theDialog, iTSNumDown, &aShort, &item, &tsNumDownRect);
	GetDialogItem(theDialog, iTSDenomUp, &aShort, &item, &tsDenomUpRect);
	GetDialogItem(theDialog, iTSDenomDown, &aShort, &item, &tsDenomDownRect);
	GetDialogItem(theDialog, iTSStaff, &aShort, &item, &tsStaffRect);
	GetDialogItem(theDialog, iTSC, &aShort, (Handle *)&cTimeHdl, &aRect);
	GetDialogItem(theDialog, iTSCut, &aShort, (Handle *)&cutTimeHdl, &aRect);

	if (insert) {
		ShowDialogItem(theDialog, iTSInsertIt);
		HideDialogItem(theDialog, iTSReplaceIt);
	}
	else {
		HideDialogItem(theDialog, iTSInsertIt);
		ShowDialogItem(theDialog, iTSReplaceIt);
	}
	
	ShowWindow(GetDialogWindow(theDialog));
	ArrowCursor();

	/* Set up radio button groups and initial time signature. */
	
	radio1 = (*onAllStaves? iTSAllStaves : iTSThisStaff);
	PutDlgChkRadio(theDialog,radio1,TRUE);

	if (*type==N_OVER_D)		radio2 = iTSNormal;
	else if (*type==C_TIME)		radio2 = iTSC;
	else if (*type==CUT_TIME)	radio2 = iTSCut;
	else						radio2 = iTSNOnly;
	PutDlgChkRadio(theDialog,radio2,TRUE);
	
	numerator = *numParam;
	denominator = *denomParam;
	HiliteControl(cTimeHdl, numerator==4 ? CTL_ACTIVE : CTL_INACTIVE);
	HiliteControl(cutTimeHdl, numerator==2 || numerator==4 ?
													CTL_ACTIVE : CTL_INACTIVE);
	TSDrawStaff();

	done = FALSE;
	do {
		ModalDialog(filterUPP, &itemHit);
		switch (itemHit) {
			case OK: 
				if (TSDUR_BAD(numerator, denominator)) {
					GetIndCString(fmtStr, DIALOGERRS_STRS, 19);			/* "too long a duration" */
					sprintf(strBuf, fmtStr, numerator, denominator);
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					break;
				}

				if (radio2==iTSNormal)	 *type = N_OVER_D;
				else if (radio2==iTSC)	 *type = C_TIME;
				else if (radio2==iTSCut) *type = CUT_TIME;
				else					 *type = N_ONLY;
				*numParam = numerator;
				*denomParam = denominator;
				*onAllStaves = (radio1==iTSAllStaves);
				done = result = TRUE;
				break;
			case Cancel: 
				done = TRUE;
				result = FALSE;
				break;
			case iTSThisStaff:
			case iTSAllStaves:
				if (itemHit!=radio1)
					SwitchRadio(theDialog, &radio1, itemHit);
				break;
			case iTSNormal:
			case iTSC:
			case iTSCut:
			case iTSNOnly:
				if (itemHit!=radio2)
					SwitchRadio(theDialog, &radio2, itemHit);
				break;
			default:
				;
		}
	} while (!done);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(theDialog);
	SetPort(oldPort);
	return result;
}


/* ----------------------------------------------------------------- TSDrawStaff -- */
/*	Draw the TimeSignature staff user item.  Expects <numerator> and
<denominator> to be initialized. */

#define LineV(lineNum) (tsStaffRect.bottom-10-3*(lineNum))

static void TSDrawStaff()
{
	char	s[20];
	short	i, left, right;

	left = tsStaffRect.left;
	right = tsStaffRect.right;
	EraseRect(&tsStaffRect);
	PenPat(NGetQDGlobalsGray());
	for (i = 1; i<=5; i++) {
		MoveTo(left, LineV(2 * i - 1));
		Line(right - left - 10, 0);
	}
	PenNormal();
	TextFont(sonataFontNum);						/* Set to Sonata font */
	TextSize(18);
	MoveTo(left, LineV(1));
	DrawChar(MCH_trebleclef);
	if (denominator) {
		sprintf(s, "%d", numerator);
		MoveTo(left+25-(CStringWidth(s)/2), LineV(7)+1);
		DrawCString(s);
		sprintf(s, "%d", denominator);
		MoveTo(left+25-(CStringWidth(s)/2), LineV(3)+1);
		DrawCString(s);
	}
	else {
		sprintf(s, "%d", numerator);
		MoveTo(left+25-(CStringWidth(s)/2), LineV(5)+1);
		DrawCString(s);
	}
	TextFont(0);
	TextSize(0);
#undef LineV
}


/* -------------------------------------------------------------------- TSFilter -- */

static pascal Boolean TSFilter(DialogPtr theDialog, EventRecord *theEvent,
											short *itemHit)
{
	Boolean cTimeActive, cutTimeActive;
	
	switch (theEvent->what) {
		case updateEvt:
			BeginUpdate(GetDialogWindow(theDialog));
			DrawDialog(theDialog);
			OutlineOKButton(theDialog, TRUE);
			EndUpdate(GetDialogWindow(theDialog));
			*itemHit = 0;
			return TRUE;
			break;
		case mouseDown: 
			if (TSHandleMouseDown(theEvent)) {
					cTimeActive = (numerator==4);
					cutTimeActive = (numerator==2 || numerator==4);
					HiliteControl(cTimeHdl, cTimeActive ? CTL_ACTIVE : CTL_INACTIVE);
					HiliteControl(cutTimeHdl, cutTimeActive ? CTL_ACTIVE : CTL_INACTIVE);
					if ( (radio2==iTSC && !cTimeActive)
					||	 (radio2==iTSCut && !cutTimeActive) )
							SwitchRadio(theDialog, &radio2, iTSNormal);
				*itemHit = 0;
				return TRUE;
			}
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, (short *)itemHit, FALSE))
				return TRUE;
			else {
				*itemHit = 0;
				return FALSE;
			}
			break;
		default:
			;
	}
	*itemHit = 0;
	return FALSE;
}


/* ----------------------------------------------------------- TSHandleMouseDown -- */

static Boolean TSHandleMouseDown(EventRecord *theEvent)
{
	Point	where;

	where = theEvent->where;
	GlobalToLocal(&where);
	if (PtInRect(where, &tsNumUpRect)) {
		TrackArrow(&tsNumUpRect, &TSNumeratorUp);
		return TRUE;
	}
	else if (PtInRect(where, &tsNumDownRect)) {
		TrackArrow(&tsNumDownRect, &TSNumeratorDown);
		return TRUE;
	}
	else if (PtInRect(where, &tsDenomUpRect)) {
		TrackArrow(&tsDenomUpRect, &TSDenominatorUp);
		return TRUE;
	}
	else if (PtInRect(where, &tsDenomDownRect)) {
		TrackArrow(&tsDenomDownRect, &TSDenominatorDown);
		return TRUE;
	}
	else return FALSE;
}


/* ------------------------------------------ TSDenominatorDown, TSDenominatorUp -- */
/*	Decrement <denominator> to the next legal value. */

static void TSDenominatorDown()
{
	if (denominator) {
		if (denominator>1) denominator /= 2;
		TSDrawStaff();
	}
}

/*	Increment <denominator> to the next legal value. */

static void TSDenominatorUp()
{
	if (denominator<MAX_TSDENOM) {
		denominator *= 2;
		TSDrawStaff();
	}
}


/* ----------------------------------------------- TSNumeratorDown, TSNumeratorUp -- */
/*	Decrement <numerator>. */

static void TSNumeratorDown()
{
	if (numerator>1) {
		--numerator;
		TSDrawStaff();
	}
}

/*	Increment <numerator>. */

static void TSNumeratorUp()
{
	if (numerator<MAX_TSNUM) {
		++numerator;
		TSDrawStaff();
	}
}


#define MOUSETHRESHTIME 24			/* ticks before arrows auto-repeat */
#define MOUSEREPEATTIME 3			/* ticks between auto-repeats */

/* ------------------------------------------------------------------- TrackArrow -- */
/*	Track arrow, calling <actionProc> after MOUSETHRESHTIME ticks, and
	repeating every MOUSEREPEATTIME ticks thereafter, as long as the mouse
	is still down inside of <arrowRect>. ??Should replace with TrackNumberArrow.*/

void TrackArrow(Rect	*arrowRect, TrackArrowFunc actionProc)
{
	long	t;
	Point	pt;

	(*actionProc)();											/* do it once */
	t = TickCount();											/* delay until auto-repeat */
	while (StillDown() && TickCount() < t+MOUSETHRESHTIME)
		;
	GetMouse(&pt);
	if (StillDown() && PtInRect(pt, arrowRect))
		(*actionProc)();										/* do it again */
	while (StillDown()) {
		t = TickCount();										/* auto-repeat rate */
		while (StillDown() && TickCount() < t+MOUSEREPEATTIME)
			;
		GetMouse(&pt);
		if (StillDown() && PtInRect(pt, arrowRect))
			(*actionProc)();									/* keep doing it */
	}
}


/* ------------------------------------------ RehearsalMarkDialog, ChordSymDialog -- */ 
 
static enum {
	STXT3_Add = 3,
	EDIT4_A
} E_RHMItems;

Boolean RehearsalMarkDialog(unsigned char *string)
{
	short			ditem;
	DialogPtr		dlog;
	GrafPtr			oldPort;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(REHEARSALMK_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(REHEARSALMK_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		CenterWindow(GetDialogWindow(dlog), 70);
		
		/* Fill in dialog's values */
		PutDlgString(dlog,EDIT4_A,string,TRUE);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();

		while (TRUE) {
			ModalDialog(filterUPP, &ditem);	
			if (ditem == OK || ditem==Cancel) break;
		}
		if (ditem == OK)
			GetDlgString(dlog,EDIT4_A,string);
	
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
		SetPort(oldPort);
		return (ditem == OK);
	}
	DisposeModalFilterUPP(filterUPP);
	MissingDialog(REHEARSALMK_DLOG);
	return FALSE;
}

static enum {
	EDIT4_MP = 4
} E_MPItems;

static Boolean CheckPatchVal(unsigned char *string) 
{
	short len = string[0];
	
	if (len <=0) return false;
	if (len >3) return false;
	
	for (int i = 1; i<=len; i++) {
		if (!isdigit(string[i])) return false;
	}
	
	return TRUE;
}

Boolean PatchChangeDialog(unsigned char *string)
{
	short			ditem;
	DialogPtr		dlog;
	GrafPtr			oldPort;
	ModalFilterUPP	filterUPP;
	Boolean			keepGoing = TRUE;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(PATCHCHANGE1_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(PATCHCHANGE1_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		CenterWindow(GetDialogWindow(dlog), 70);
		
		/* Fill in dialog's values */
		PutDlgString(dlog,EDIT4_MP,string,TRUE);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();

		while (keepGoing) {
			ModalDialog(filterUPP, &ditem);	
			switch (ditem) {
				case OK:
					GetDlgString(dlog,EDIT4_MP,string);
					if (CheckPatchVal(string)) keepGoing = FALSE;
					else keepGoing = TRUE;
					break;
				case Cancel:
					keepGoing = FALSE;
					break;				
			}
		}
	
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
		SetPort(oldPort);
		return (ditem == OK);
	}
	DisposeModalFilterUPP(filterUPP);
	MissingDialog(PATCHCHANGE1_DLOG);
	return FALSE;
}

static enum {
	EDIT4_MPan = 4
} E_MPanItems;

static Boolean CheckPanVal(unsigned char *string) 
{
	long panval = FindIntInString(string);
	
	if (panval < 0) return FALSE;
	if (panval > 127) return FALSE;
	
	return TRUE;
}

Boolean PanSettingDialog(unsigned char *string)
{
	short			ditem;
	DialogPtr		dlog;
	GrafPtr			oldPort;
	ModalFilterUPP	filterUPP;
	Boolean			keepGoing = TRUE;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(PATCHCHANGE1_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(PNSETTING_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		CenterWindow(GetDialogWindow(dlog), 70);
		
		/* Fill in dialog's values */
		PutDlgString(dlog,EDIT4_MPan,string,TRUE);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();

		while (keepGoing) {
			ModalDialog(filterUPP, &ditem);	
			switch (ditem) {
				case OK:
					GetDlgString(dlog,EDIT4_MP,string);
					if (CheckPanVal(string)) keepGoing = FALSE;
					else keepGoing = TRUE;
					break;
				case Cancel:
					keepGoing = FALSE;
					break;				
			}
		}
	
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
		SetPort(oldPort);
		return (ditem == OK);
	}
	DisposeModalFilterUPP(filterUPP);
	MissingDialog(PATCHCHANGE1_DLOG);
	return FALSE;
}


Boolean ChordFrameDialog(Document *doc,
				Boolean *relFSize,			/* TRUE means size=1...9 for Tiny...StaffHeight */
				short *size,				/* If *relFSize, Tiny...StaffHeight index, else in points */
				short *style,				/* Standard style bits */
				short *enclosure,			/* Enclosure code */
				unsigned char *fontname,	/* Fontname or empty */
				unsigned char *pTheChar
				)
{
	short fontNum, theSize;
	
	fontNum = config.chordFrameFontID;
	if (fontNum==0) fontNum = 123;	// FIXME: Seville is default, but shouldn't be set here!?
	/* Don't try to use the actual font size: just make it big, but cf. ChooseCharDlog's limit. */
	theSize = 48;
	
	if (!ChooseCharDlog(fontNum, theSize, pTheChar)) return FALSE;
	
	*relFSize = FALSE;
	*size = theSize;
	*style = 0;							/* plain */
	*enclosure = ENCL_NONE;
	PStrCopy((StringPtr)doc->fontNameCS, (StringPtr)fontname);

	return TRUE;
}


/* --------------------------------------------------------------- SymbolIsBarline -- */
/* Decide whether user wants a newly-inserted symbol (presumably a double bar or repeat
bar) to be a genuine barline or not. If the user hasn't said which to assume, ask them
which they want. Returns TRUE for barline, FALSE for not barline. */

static enum {
	BUT1_OK=1,
	BARLINE_DI=3,
	NOT_BARLINE_DI,
	ASSUME_DI=6
} E_SymIsBarlineItems;

static short group1;

Boolean SymbolIsBarline()
{
	short itemHit;
	Boolean value;
	static short assumeBarline=0, oldChoice=0;
	register DialogPtr dlog; GrafPtr oldPort;
	Boolean keepGoing=TRUE;
	static Boolean firstCall=TRUE;

	if (firstCall) {
		if (config.dblBarsBarlines==1)		assumeBarline = BARLINE_DI;
		else if (config.dblBarsBarlines==2) assumeBarline = NOT_BARLINE_DI;
		else								assumeBarline = 0;
		firstCall = FALSE;
	}
	
	if (assumeBarline)
		value = (assumeBarline==BARLINE_DI);
	else {
	
		/* Build dialog window and install its item values */
		
		ModalFilterUPP	filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP == NULL) {
			MissingDialog(DBLBAR_IS_BARLINE_DLOG);
			return FALSE;
		}

		GetPort(&oldPort);
		dlog = GetNewDialog(DBLBAR_IS_BARLINE_DLOG, NULL, BRING_TO_FRONT);
		if (dlog==NULL) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(DBLBAR_IS_BARLINE_DLOG);
			return FALSE;
		}
		SetPort(GetDialogWindowPort(dlog));
	
		/* Fill in dialog's values here */
	
		group1 = (oldChoice? oldChoice : BARLINE_DI);
		PutDlgChkRadio(dlog, BARLINE_DI, (group1==BARLINE_DI));
		PutDlgChkRadio(dlog, NOT_BARLINE_DI, (group1==NOT_BARLINE_DI));
	
		PlaceWindow(GetDialogWindow(dlog),(WindowPtr)NULL,0,80);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		/* Entertain filtered user events until dialog is dismissed */
		
		while (keepGoing) {
			ModalDialog(filterUPP, &itemHit);
			switch(itemHit) {
				case BUT1_OK:
					keepGoing = FALSE;
					value = (group1==BARLINE_DI);
					if (GetDlgChkRadio(dlog, ASSUME_DI)) assumeBarline = group1;
					oldChoice = group1;
					break;
				case BARLINE_DI:
				case NOT_BARLINE_DI:
					if (itemHit!=group1) SwitchRadio(dlog, &group1, itemHit);
					break;
				case ASSUME_DI:
					PutDlgChkRadio(dlog, ASSUME_DI, !GetDlgChkRadio(dlog, ASSUME_DI));
					break;
				default:
					;
			}
		}
		
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
		SetPort(oldPort);
	}
	
	return value;
}


/* Is it okay to insert a barline despite presence of unknown-duration notes? If user
has already said it's okay and not to ask again, just return TRUE; else warn user and
return if they say okay, FALSE otherwise. */

static enum {
	BUT1_InsAnyway=1,
	BUT2_Cancel=2,
	CHK3_Assume=5
} E_MeasUnkDurItems;

Boolean InsMeasUnkDurDialog()
{
	static Boolean assumeOK=FALSE;
	short itemHit, okay, keepGoing=TRUE;
	DialogPtr dlog; GrafPtr oldPort;
	ModalFilterUPP filterUPP;

	if (assumeOK) return TRUE;
	
	/* Build dialog window and install its item values */

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(INSMEASUNKDUR_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(INSMEASUNKDUR_DLOG, NULL, BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(INSMEASUNKDUR_DLOG);
		return FALSE;
	}
	SetPort(GetDialogWindowPort(dlog));

	PlaceWindow(GetDialogWindow(dlog), (WindowPtr)NULL, 0, 80);
	ShowWindow(GetDialogWindow(dlog));

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP, &itemHit);
		switch(itemHit) {
			case BUT1_InsAnyway:
				keepGoing = FALSE;
				if (GetDlgChkRadio(dlog, CHK3_Assume)) assumeOK = TRUE;
				okay = TRUE;
				break;
			case BUT2_Cancel:
				keepGoing = FALSE;
				okay = FALSE;
				break;
			case CHK3_Assume:
				PutDlgChkRadio(dlog, CHK3_Assume, !GetDlgChkRadio(dlog, CHK3_Assume));
				break;
		}
	}
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	
	return okay;
}


/* -------------------------------------------------------------- XLoadDialogsSeg -- */
/* Null function to allow loading or unloading Dialogs.c's segment. */

void XLoadDialogsSeg()
{
}
