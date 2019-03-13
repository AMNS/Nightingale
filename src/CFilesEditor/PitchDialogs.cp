/***************************************************************************
*	FILE:	PitchDialogs.c
*	PROJ:	Nightingale
*	DESC:	Dialog-handling routines for pitch-changing dialogs
/***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
	TranspFilter				TransposeDialog			CalcSemiTones
	DTransposeDialog			DoTranspArrowKey		TransKeyDialog	
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* --------------------------------------------- Transpose- and DTransposeDialog -- */

static pascal Boolean TranspFilter(DialogPtr, EventRecord *, short *);
static void	CalcSemiTones(DialogPtr dlog);
static void	DoTranspArrowKey(DialogPtr dlog, short whichKey);
static void	ShowPlusSign(DialogPtr dlog);

static enum
{
	OCTAVES_DI=4,				/* Item numbers (4-6 common to all transpose dlogs) */
	PLUS_DI=5,
	INTERVAL_DI=6,
	DCHORDSLASHES_DI=7,
	SEMITONES_DI=8,
	CHORDSLASHES_DI=10
} E_TranspCommonItems;

static UserPopUp popupIntval;
static UserPopUp popupOctave;

static char	hilitedItem = -1;  	/* ItemNum of item currently hilited by using arrow keys. If -1, none. */


static pascal Boolean TranspFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Point where;
	Boolean ans=False; WindowPtr w;
	short ch;

	w = (WindowPtr)(evt->message);
	switch(evt->what) {
		case updateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetWindowPort(w));
				BeginUpdate(GetDialogWindow(dlog));
				UpdateDialogVisRgn(dlog);
				DrawPopUp(&popupIntval);
				DrawPopUp(&popupOctave);
				FrameDefault(dlog,OK,True);
				EndUpdate(GetDialogWindow(dlog));
				ans = True; *itemHit = 0;
			}
			break;
		case activateEvt:
			if (w == GetDialogWindow(dlog)) {
				short activ = (evt->modifiers & activeFlag)!=0;
				SetPort(GetWindowPort(w));
			}
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			if (PtInRect(where,&popupIntval.shadow)) {
				*itemHit = DoUserPopUp(&popupIntval) ? INTERVAL_DI : 0;
				hilitedItem = -1;
				HilitePopUp(&popupIntval, False);
				HilitePopUp(&popupOctave, False);
				ans = True;
			}
			else if (PtInRect(where,&popupOctave.shadow)) {
				*itemHit = DoUserPopUp(&popupOctave) ? OCTAVES_DI : 0;
				hilitedItem = -1;
				HilitePopUp(&popupOctave, False);
				HilitePopUp(&popupIntval, False);
				ans = True;
			}
			break;
		case autoKey:
		case keyDown:
			if (DlgCmdKey(dlog, evt, itemHit, False))
				return True;
			else {
				ch = (unsigned char)evt->message;
				switch (ch) {
					case UPARROWKEY:
					case DOWNARROWKEY:
					case LEFTARROWKEY:
					case RIGHTARROWKEY:
						DoTranspArrowKey(dlog, ch);
						*itemHit = OCTAVES_DI;			/* forces Transpose dlog to update semitones text field */	
						ans = True;
						break;						
				}
			}
			break;		
		}
		
	return(ans);
}

/* For each entry in Octave pop-up menu, give {octaves, direction} (up:1 or down:0) */
short octaves[][2] = {
	{99,99},	/* (unused) */
	{4,1},	/* up 4 octaves */
	{3,1},
	{2,1},
	{1,1},
	{0,1},	/* up no octaves */
	{0,99},	/* (dividing line) */
	{0,0},	/* down no octaves */
	{1,0},
	{2,0},
	{3,0},
	{4,0},	/* down 4 octaves */
	{-99,-99} /* (mark end of list) */
};

/* For each entry in Transpose pop-up menu, give {diatonic steps,half-steps} */
short transp[][2] = {
	{99,99},	/* (unused) */
	{0,0},	/* Perfect unison = no change */
	{0,99},	/* (dividing line) */
	{0,1},
	{1,0},	/* diminished 2nd */
	{1,1},
	{1,2},	/* Major 2nd */
	{1,3},
	{2,3},	/* minor 3rd */
	{2,4},	/* Major 3rd */
	{3,4},
	{3,5},	/* Perfect 4th */
	{3,6},
	{4,6},
	{4,7},	/* Perfect 5th */
	{4,8},
	{5,8},	/* minor 6th */	
	{5,9},
	{6,10},	/* minor 7th */
	{6,11},
	{-99,-99} /* (mark end of list) */
};

Boolean TransposeDialog(Boolean *pUp, short *pOctaves, short *pSteps, short *pSemiChange,
								Boolean *pSlashes)
{
	DialogPtr		dlog;
	GrafPtr			oldPort;
	short				dialogOver, ditem;
	short				semiChoice, octChoice, k;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(TranspFilter);
	if (filterUPP == NULL) {
		MissingDialog(TRANSPOSE_DLOG);
		return False;
	}
	GetPort(&oldPort);
	dlog = GetNewDialog(TRANSPOSE_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(TRANSPOSE_DLOG);
		return False;
	}
	CenterWindow(GetDialogWindow(dlog), 70);
	SetPort(GetDialogWindowPort(dlog));
	
	for (semiChoice = 1, k = 1; transp[k][0]>=0; k++)
		if (transp[k][0]==*pSteps && transp[k][1]==*pSemiChange)
			{ semiChoice = k; break; }
			
	for (octChoice = 5, k = 1; octaves[k][0]>=0; k++)
		if (octaves[k][0]==*pOctaves && octaves[k][1]==*pUp)
			{ octChoice = k; break; }

	if (!InitPopUp(dlog, &popupIntval, INTERVAL_DI, 0, transposePopID, semiChoice))
			goto broken;
	if (!InitPopUp(dlog, &popupOctave, OCTAVES_DI, 0, octavePopID, octChoice))
			goto broken;
	hilitedItem = -1;
	
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	PutDlgChkRadio(dlog, CHORDSLASHES_DI, *pSlashes);

	CalcSemiTones(dlog);
	ShowPlusSign(dlog);
	
	dialogOver = 0;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);	
		switch (ditem) {
			case OK:
			case Cancel:
				dialogOver = ditem;
				break;
			case OCTAVES_DI:
			case INTERVAL_DI:
				ShowPlusSign(dlog);
				CalcSemiTones(dlog);
				break;
			case CHORDSLASHES_DI:
				PutDlgChkRadio(dlog, CHORDSLASHES_DI, !GetDlgChkRadio(dlog, CHORDSLASHES_DI));
				break;
			default:
				;
		}
	}
	
	if (dialogOver==OK) {
		*pOctaves = octaves[popupOctave.currentChoice][0];
		*pUp = octaves[popupOctave.currentChoice][1];
		*pSteps = transp[popupIntval.currentChoice][0];
		*pSemiChange = transp[popupIntval.currentChoice][1];
		*pSlashes = GetDlgChkRadio(dlog, CHORDSLASHES_DI);
	}
	
broken:
	DisposePopUp(&popupIntval);
	DisposePopUp(&popupOctave);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);											/* Free heap space */
	SetPort(oldPort);
	return (dialogOver==OK);
}

/* Update number of semitones in static text item (transpose chromatic dlog only) */ 

void CalcSemiTones(DialogPtr dlog)
{
	char	semitones;
	
	semitones = (octaves[popupOctave.currentChoice][0]*12) + transp[popupIntval.currentChoice][1];
	if (octaves[popupOctave.currentChoice][1]==0)					/* If direction is down... */
		semitones = -semitones;												/* add neg sign. */
	PutDlgWord(dlog, SEMITONES_DI, semitones, False);
}

/* For each entry in Diatonic Transpose pop-up menu, give diatonic steps */
short dTransp[] = {
	99,		/* (unused) */
	0,			/* Perfect unison = no change */
	0,			/* (dividing line) */
	1,			/* 2nd */
	2,			/* 3rd */
	3,			/* 4th */
	4,			/* 5th */
	5,			/* 6th */
	6,			/* 7th */
	-99		/* (mark end of list) */
};

Boolean DTransposeDialog(Boolean *pUp, short *pOctaves, short *pSteps, Boolean *pSlashes)
{
	DialogPtr		dlog;
	GrafPtr			oldPort;
	short				k;
	short				dialogOver, ditem, stepChoice, octChoice;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(TranspFilter);
	if (filterUPP == NULL) {
		MissingDialog(DTRANSPOSE_DLOG);
		return False;
	}
	GetPort(&oldPort);
	dlog = GetNewDialog(DTRANSPOSE_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(DTRANSPOSE_DLOG);
		return False;
	}
	CenterWindow(GetDialogWindow(dlog), 70);
	SetPort(GetDialogWindowPort(dlog));
		
	for (stepChoice = 1, k = 1; dTransp[k] >= 0; k++) {
		if (dTransp[k] == *pSteps) {
			stepChoice = k; break;
		}
	}

	for (octChoice = 5, k = 1; octaves[k][0] >= 0; k++) {
		if (octaves[k][0] == *pOctaves && octaves[k][1] == *pUp) {
			octChoice = k; break;
		}
	}
	
	if (!InitPopUp(dlog, &popupIntval, INTERVAL_DI, 0, dTransposePopID, stepChoice))
			goto broken;
	if (!InitPopUp(dlog, &popupOctave, OCTAVES_DI, 0, octavePopID, octChoice))
			goto broken;
	hilitedItem = -1;
	
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	PutDlgChkRadio(dlog, DCHORDSLASHES_DI, *pSlashes);

	ShowPlusSign(dlog);
	
	dialogOver = 0;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);	
		switch (ditem) {
			case OK:
			case Cancel:
				dialogOver = ditem;
				break;
			case OCTAVES_DI:
			case INTERVAL_DI:
				ShowPlusSign(dlog);
				break;
			case DCHORDSLASHES_DI:
				PutDlgChkRadio(dlog, DCHORDSLASHES_DI, !GetDlgChkRadio(dlog, DCHORDSLASHES_DI));
				break;
			default:
				;
		}
	}
	
	if (dialogOver==OK) {
		*pOctaves = octaves[popupOctave.currentChoice][0];
		*pUp = octaves[popupOctave.currentChoice][1];
		*pSteps = dTransp[popupIntval.currentChoice];
		*pSlashes = GetDlgChkRadio(dlog, DCHORDSLASHES_DI);
	}
	
broken:
	DisposePopUp(&popupIntval);
	DisposePopUp(&popupOctave);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);										/* Free heap space */
	SetPort(oldPort);
	return (dialogOver==OK);		
}


/* This lets user select popup items using arrow keys. It doesn't work if menu has
any disabled items, unless they are the '--------' kind and are not the first or
last items. */

void DoTranspArrowKey(DialogPtr /*dlog*/, short whichKey)
{
	short lastItemNum, newChoice;
	UserPopUp *p;
					
	switch (whichKey) {
		case UPARROWKEY:
		case DOWNARROWKEY:
			if (hilitedItem==-1) {
				hilitedItem = INTERVAL_DI;
				HilitePopUp(&popupIntval, True); }
			else {
				p = (hilitedItem==OCTAVES_DI) ? &popupOctave : &popupIntval;
				newChoice = p->currentChoice;
				lastItemNum = CountMenuItems(p->menu);
				if (whichKey == UPARROWKEY) {				
					newChoice -= 1;
					if (newChoice == 0) return;					/* Top of menu reached; don't wrap around. */
					GetMenuItemText (p->menu, newChoice, p->str);
					if (p->str[1] == '-') newChoice -= 1;		/* Skip over the dash item */ 						
				}
				else {
					newChoice += 1;
					if (newChoice == lastItemNum + 1) return;	/* Bottom of menu reached; don't wrap around. */
					GetMenuItemText (p->menu, newChoice, p->str);
					if (p->str[1] == '-') newChoice += 1;		/* Skip over the dash item */ 						
				}
				ChangePopUpChoice (p, newChoice);
				HilitePopUp(p,True);
			}
			break;
		case LEFTARROWKEY:
			if (hilitedItem != OCTAVES_DI) {
				HilitePopUp(&popupIntval, False);
				HilitePopUp(&popupOctave, True);
				hilitedItem = OCTAVES_DI; }							
			break;
		case RIGHTARROWKEY:
			if (hilitedItem != INTERVAL_DI) {
				HilitePopUp(&popupOctave, False);
				HilitePopUp(&popupIntval, True);
				hilitedItem = INTERVAL_DI; }
			break;						
	}
}

/* Show or hide the plus sign between the transpose popups */

static void ShowPlusSign(DialogPtr dlog)
{
	if (octaves[popupOctave.currentChoice][0]==0)
		HideDialogItem(dlog, PLUS_DI);
	else ShowDialogItem(dlog, PLUS_DI);
}


/* -------------------------------------------------------- TransKeyDialog et al -- */

static enum
{
	CHORDSYMS_DI=12,			/* Item numbers */
	NOTES_DI
} E_TransKeyItems;


Boolean TransKeyDialog(Boolean trStaff[], Boolean *pUp, short *pOctaves, short *pSteps,
								short *pSemiChange, Boolean *pSlashes, Boolean *pNotes,
								Boolean *pChordSyms)
{
	DialogPtr		dlog;
	GrafPtr			oldPort;
	short				dialogOver, ditem;
	short				semiChoice, octChoice, k, staffCount, s;
	char				str[32];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(TranspFilter);
	if (filterUPP == NULL) {
		MissingDialog(TRANSKEY_DLOG);
		return False;
	}
	GetPort(&oldPort);
	dlog = GetNewDialog(TRANSKEY_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(TRANSKEY_DLOG);
		return False;
	}
	CenterWindow(GetDialogWindow(dlog), 70);
	SetPort(GetDialogWindowPort(dlog));
	
	for (staffCount = 0, s = 1; s<=MAXSTAVES; s++)
		if (trStaff[s]) staffCount++;
	sprintf(str, "%d", staffCount);
	CParamText(str, "", "", "");

	for (semiChoice = 1, k = 1; transp[k][0]>=0; k++)
		if (transp[k][0]==*pSteps && transp[k][1]==*pSemiChange)
			{ semiChoice = k; break; }
			
	for (octChoice = 5, k = 1; octaves[k][0]>=0; k++)
		if (octaves[k][0]==*pOctaves && octaves[k][1]==*pUp)
			{ octChoice = k; break; }

	if (!InitPopUp(dlog, &popupIntval, INTERVAL_DI, 0, transposePopID, semiChoice))
			goto broken;
	if (!InitPopUp(dlog, &popupOctave, OCTAVES_DI, 0, octavePopID, octChoice))
			goto broken;
	hilitedItem = -1;
	
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	PutDlgChkRadio(dlog, CHORDSLASHES_DI, *pSlashes);
	PutDlgChkRadio(dlog, NOTES_DI, *pNotes);
	PutDlgChkRadio(dlog, CHORDSYMS_DI, *pChordSyms);

	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	CalcSemiTones(dlog);
	ShowPlusSign(dlog);
	
	dialogOver = 0;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);	
		switch (ditem) {
			case OK:
			case Cancel:
				dialogOver = ditem;
				break;
			case OCTAVES_DI:
			case INTERVAL_DI:
				ShowPlusSign(dlog);
				CalcSemiTones(dlog);
				break;
			case CHORDSLASHES_DI:
				PutDlgChkRadio(dlog, CHORDSLASHES_DI, !GetDlgChkRadio(dlog, CHORDSLASHES_DI));
				break;
			case NOTES_DI:
				PutDlgChkRadio(dlog, NOTES_DI, !GetDlgChkRadio(dlog, NOTES_DI));
				break;
			case CHORDSYMS_DI:
				PutDlgChkRadio(dlog, CHORDSYMS_DI, !GetDlgChkRadio(dlog, CHORDSYMS_DI));
				break;
			default:
				;
		}
	}
	
	if (dialogOver==OK) {
		*pOctaves = octaves[popupOctave.currentChoice][0];
		*pUp = octaves[popupOctave.currentChoice][1];
		*pSteps = transp[popupIntval.currentChoice][0];
		*pSemiChange = transp[popupIntval.currentChoice][1];
		*pSlashes = GetDlgChkRadio(dlog, CHORDSLASHES_DI);
		*pNotes = GetDlgChkRadio(dlog, NOTES_DI);
		*pChordSyms = GetDlgChkRadio(dlog, CHORDSYMS_DI);
	}
	
broken:
	DisposePopUp(&popupIntval);
	DisposePopUp(&popupOctave);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);											/* Free heap space */
	SetPort(oldPort);
	return (dialogOver==OK);
}
