/******************************************************************************************
*	FILE:	TempoEdit.c
*	PROJ:	Nightingale
*	DESC:	Dialog-handling routines for creating and editing tuplets
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017-2022 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* ------------------------------------ Declarations & Help Functions for TempoDialog -- */

static pascal Boolean TempoFilter(DialogPtr, EventRecord *, short *);
static void DimOrUndimMMNumberEntry(DialogPtr dlog, Boolean undim, unsigned char * /* metroStr */);
static Boolean AllIsWell(DialogPtr dlog);

static short choiceIdx;

enum {
	VerbalDI=3,
	SET_DUR_DI,
	MetroDI,
	ShowMMDI,
	UseMMDI=9,
	EqualsDI,
	TDummyFldDI=11,
	ExpandDI
};

static pascal Boolean TempoFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr	w;
	short		ch, field, type, byChLeftPos, chTopPos, ans;
	Handle		hndl;
	Rect		box;
	Point		where;
	GrafPtr		oldPort;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort);  SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));			
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, OK, True);
				GetDialogItem(dlog, SET_DUR_DI, &type, &hndl, &box);
				byChLeftPos = choiceIdx*(DP_COL_WIDTH/8);
				chTopPos = 3*DP_ROW_HEIGHT;
				if (choiceIdx>DP_NCOLS) {
					byChLeftPos -= DP_NCOLS*(DP_COL_WIDTH/8);
					chTopPos -= DP_ROW_HEIGHT;	/* Correct for padding ??CHECK! */
				}
				DrawBMPChar(bmpDurationPal.bitmap, bmpDurationPal.byWidth, bmpDurationPal.byWidthPadded,
							DP_ROW_HEIGHT, byChLeftPos, chTopPos, box);
//LogPrintf(LOG_DEBUG, "TempoFilter: choiceIdx=%d DP_NCOLS=%d byChLeftPos=%d\n", choiceIdx,
//DP_NCOLS, byChLeftPos);
				FrameShadowRect(&box);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return True;
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
#if 11
			where = evt->where;
			GlobalToLocal(&where);
			GetDialogItem(dlog, SET_DUR_DI, &type, &hndl, &box);
			if (PtInRect(where, &box)) {
// ??WHAT IS THIS FOR???????????????????????????????
				NoteInform(DURCHOICE_1DOT_DLOG);
				*itemHit = SET_DUR_DI;
				return True;
			}
			break;
#endif
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False)) return True;
			ch = (unsigned char)evt->message;
			
			/* The Dialog Manager considers only EditText fields as candidates for being
			   activated by the tab key, so handle tabbing from field to field ourselves
			   so user can direct keystrokes to the pop-up ??THERE IS NO POP-UP. WHAT TO
			   DO HERE? CF. TransMFFilter()!!! as well as the EditText
			   fields. */
			   
			field = GetDialogKeyboardFocusItem(dlog);
			if (ch=='\t') {
				if (field==VerbalDI) field = TDummyFldDI;
				else if (field==TDummyFldDI) field = MetroDI;
				else field = VerbalDI;
#if 11
#else
				popUpHilited = (field==TDummyFldDI);
				SelectDialogItemText(dlog, field, 0, ENDTEXT);
				HiliteGPopUp(curPop, popUpHilited);
#endif
				*itemHit = 0;
				return True;
			}
			else {
				if (field==TDummyFldDI) {
#if 11
#else
					ans = DurPopupKey(curPop, popKeys1dot, ch);
					*itemHit = ans? SET_DUR_DI : 0;
					HiliteGPopUp(curPop, True);
#endif
					return True;					/* so no chars get through to TDummyFldDI edit field */
				}									/* NB: however, DlgCmdKey will let you paste into TDummyFldDI! */
			}
			break;
	}
	
	return False;
}


/* Disable and dim the number-entry field and fields subordinate to it, or enable and
undim them. */

static void DimOrUndimMMNumberEntry(DialogPtr dlog, Boolean undim, unsigned char * /* metroStr */)
{
	short	type, newType;
	Handle	hndl;
	Rect	box;

	newType = (undim? editText : statText+itemDisable);
	GetDialogItem(dlog, MetroDI, &type, &hndl, &box);
	SetDialogItem(dlog, MetroDI, newType, hndl, &box);

	if (undim) {
		ShowDialogItem(dlog, SET_DUR_DI);
		ShowDialogItem(dlog, EqualsDI);
		ShowDialogItem(dlog, MetroDI);
		XableControl(dlog, ShowMMDI, True);
	}
	else {
		HideDialogItem(dlog, SET_DUR_DI);
		HideDialogItem(dlog, EqualsDI);
		HideDialogItem(dlog, MetroDI);
		XableControl(dlog, ShowMMDI, False);
	}
}


static Boolean AllIsWell(DialogPtr dlog)
{
	short expandedSetting, maxLenExpanded, type, i;
	Handle hndl;  Rect box;
	long beatsPM; 
	Str255 str, metStr;
	char fmtStr[256];
	Boolean strOkay = True;

	GetDialogItem(dlog,VerbalDI, &type, &hndl, &box);
	GetDialogItemText(hndl, str);
	GetDialogItem(dlog, ExpandDI, &type, &hndl, &box);
	expandedSetting = GetControlValue((ControlHandle)hndl);
	if (expandedSetting!=0) {
		maxLenExpanded = (EXPAND_WIDER? 255/3 : 255/2);
		if (Pstrlen(str)>maxLenExpanded) strOkay = False;
		else {
			for (i = 1; i <= Pstrlen(str); i++)
			if (str[i]==CH_NLDELIM) {
				strOkay = False;
				break;
			}
		}
	}
	if (!strOkay) {
		GetIndCString(fmtStr, TEXTERRS_STRS, 2);		/* "Multiline strings and strings of..." */
		sprintf(strBuf, fmtStr, maxLenExpanded);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
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
		return False;
	}
	
	GetDlgString(dlog, VerbalDI, str);
	if (Pstrlen(str)==0 && !GetDlgChkRadio(dlog, UseMMDI)) {
		GetIndCString(strBuf, DIALOGERRS_STRS, 23);			/* "No tempo string and no M.M. is illegal" */ 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		SelectDialogItemText(dlog, MetroDI, 0, ENDTEXT);	/* Select field so user knows which one is bad. */
		return False;
	}
	
	return True;
}


static short paletteHilited=True;

/* Conduct dialog to get Tempo and metronome mark parameters from user. */

Boolean TempoDialog(Boolean *useMM, Boolean *showMM, short *durCode, Boolean *dotted,
			Boolean *expanded, unsigned char *tempoStr, unsigned char *metroStr)
{	
	DialogPtr dlog;
	short ditem=Cancel, type, oldResFile;
	short oldChoiceIdx, lDurCode, oldLDurCode, k, numDots;
	Boolean dialogOver, oldDotted;
	Handle hndl;  Rect box;
	GrafPtr oldPort;
	ModalFilterUPP filterUPP;

	filterUPP = NewModalFilterUPP(TempoFilter);
	if (filterUPP == NULL) {
		MissingDialog(TEMPO_DLOG);
		return False;
	}
	
	dlog = GetNewDialog(TEMPO_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(TEMPO_DLOG);
		return False;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);								/* popup code uses Get1Resource */

	GetDialogItem(dlog, SET_DUR_DI, &type, &hndl, &box);

	oldLDurCode = *durCode;
	oldDotted = *dotted;
	// ??2 IN NEXT SHOULD BE ??
	choiceIdx = DurCodeToDurPalIdx(oldLDurCode, oldDotted, 2*DP_NCOLS);
LogPrintf(LOG_DEBUG, "TempoDialog: oldLDurCode=%d oldDotted=%d choiceIdx=%d\n", oldLDurCode,
oldDotted, choiceIdx);

	/* Replace CH_CR with the UI "start new line" code. */
	
	for (k=1; k<=Pstrlen(tempoStr); k++)
		if (tempoStr[k]==CH_CR) tempoStr[k] = CH_NLDELIM;

	PutDlgChkRadio(dlog, UseMMDI, *useMM);
	PutDlgChkRadio(dlog, ShowMMDI, *showMM);
	PutDlgChkRadio(dlog, ExpandDI, *expanded);
	PutDlgString(dlog, VerbalDI, tempoStr, False);
	PutDlgString(dlog, MetroDI, metroStr, True);
	
	/* If the M.M. option isn't chosen, disable and dim the number-entry field */
	
	DimOrUndimMMNumberEntry(dlog, *useMM, metroStr);

	SelectDialogItemText(dlog, (paletteHilited? TDummyFldDI : VerbalDI), 0, ENDTEXT);

	CenterWindow(GetDialogWindow(dlog), 70);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	dialogOver = False;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
				if (AllIsWell(dlog)) dialogOver = True;
				break;
			case Cancel:
				dialogOver = True;
				break;
			case SET_DUR_DI:
				oldChoiceIdx = choiceIdx;
				choiceIdx = DurPalChoiceDlog(choiceIdx, 1);
				if (choiceIdx!=oldChoiceIdx) {
					lDurCode = durPalCode[choiceIdx];
					short byChLeftPos = choiceIdx*(DP_COL_WIDTH/8);
					short chTopPos = 3*DP_ROW_HEIGHT;
LogPrintf(LOG_DEBUG, "TempoDialog1: oldChoiceIdx=%d choiceIdx=%d DP_NCOLS=%d byChLeftPos=%d chTopPos=%d\n",
oldChoiceIdx, choiceIdx, DP_NCOLS, byChLeftPos, chTopPos);
					if (choiceIdx>DP_NCOLS) {
						byChLeftPos -= DP_NCOLS*(DP_COL_WIDTH/8);
						chTopPos -= DP_ROW_HEIGHT;	/* Correct for padding ??CHECK! */
					}
					EraseRect(&box);
					FrameRect(&box);
					DrawBMPChar(bmpDurationPal.bitmap, bmpDurationPal.byWidth, bmpDurationPal.byWidthPadded,
								DP_ROW_HEIGHT, byChLeftPos, chTopPos, box);
LogPrintf(LOG_DEBUG, "TempoDialog2: oldChoiceIdx=%d choiceIdx=%d DP_NCOLS=%d byChLeftPos=%d chTopPos=%d\n",
oldChoiceIdx, choiceIdx, DP_NCOLS, byChLeftPos, chTopPos);
				}
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
		*durCode = DurPalIdxToDurCode(choiceIdx, &numDots);
		*dotted = (numDots>0);
		*useMM = GetDlgChkRadio(dlog, UseMMDI);
		*showMM = GetDlgChkRadio(dlog, ShowMMDI);
		*expanded = GetDlgChkRadio(dlog, ExpandDI);
		
		/* In the tempo string, replace the UI "start new line" code with CH_CR. */
		
		GetDlgString(dlog, VerbalDI, tempoStr);
		for (k=1; k<=Pstrlen(tempoStr); k++)
			if (tempoStr[k]==CH_NLDELIM) tempoStr[k] = CH_CR;

		GetDlgString(dlog, MetroDI, metroStr);
	}
		
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}
