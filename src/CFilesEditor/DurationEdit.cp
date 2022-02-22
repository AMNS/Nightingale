/* DurationEdit.c for Nightingale - formerly DurationPopUp, for use with accompanying
graphic MDEF; it now uses a BMP displayed in its entirety. Nightingale has several
features that let the user choose a duration, and this file includes both general routines
for handling the duration palette and the code for the Set Duration command. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016-21 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* =================================== General routines to handle the duration palette == */
/* Note durations appear in features of Nightingale other than Set Duration: metronome
marks, "fancy" tuplets, and quantizing durations. The different features allow
different numbers of augmentation dots, and therefore different subsets of the palette!
The following routines are generic. */

void InitDurationCells(Rect *pBox, short nCols, short nRows, Rect durCell[])
{
	short cellWidth, cellHeight, cellNum;
	
	cellWidth = (pBox->right-pBox->left)/nCols;
	cellHeight = (pBox->bottom-pBox->top)/nRows;

	for (short rn = 0; rn<nRows; rn++)
		for (short cn = 0; cn<nCols; cn++) {
			cellNum = (nCols*rn) + cn;
			durCell[cellNum].top = cellHeight*rn;
			durCell[cellNum].left = cellWidth*cn;
			durCell[cellNum].bottom = durCell[cellNum].top+cellHeight;
			durCell[cellNum].right = durCell[cellNum].left+cellWidth;
//LogPrintf(LOG_DEBUG, "InitDurationCells: rn=%d cn=%d cellNum=%d durCell tlbr=%d,%d,%d,%d\n",
//rn, cn, cellNum, durCell[cellNum].top, durCell[cellNum].left,
//durCell[cellNum].bottom, durCell[cellNum].right);
		}
}


/* Return the number of the cell in the duration palette containing the given point. We
don't check whether cell is a valid selection; it's up to the calling routine to do that. */
 
short FindDurationCell(Point where, Rect *pBox, short nCols, short nRows, Rect durCell[])
{
	short xInBox, yInBox, cellNum, tryCell;
	
	xInBox = where.h - pBox->left;
	yInBox = where.v - pBox->top;
	cellNum = -1;
	for (short rn = 0; rn<nRows; rn++)
		for (short cn = 0; cn<nCols; cn++) {
			tryCell = (nCols*rn) + cn;
			if (yInBox<durCell[tryCell].top) continue;
			if (xInBox<durCell[tryCell].left) continue;
			if (yInBox>=durCell[tryCell].bottom) continue;
			if (xInBox>=durCell[tryCell].right) continue;
			cellNum = tryCell;  break;
		}

	if (cellNum<0) {
		SysBeep(1);
		LogPrintf(LOG_WARNING, "Can't find cell for where=(%d,%d) xInBox=%d yInBox=%d.  (FindDurationCell)\n",
					where.h, where.v, xInBox, yInBox);
		cellNum = 1;
	}
	
	return cellNum;
}


void HiliteDurCell(short durIdx, Rect *pBox, Rect durCell[])
{
	Rect theCell = durCell[durIdx];
	OffsetRect(&theCell, pBox->left, pBox->top);
	InsetRect(&theCell, 1, 1);
	InvertRect(&theCell);
}


/* Return the index into our palette for the given character. If the character occurs
more than once in the palette, return the index of the first occurrence; if it isn't in
the palette, return -1. */

short DurationKey(unsigned char theChar, unsigned short numDurations)
{
	short newDurIdx=-1, intChar, sym;
		
	/* First remap theChar according to the 'PLMP' resource. */
	
	intChar = (short)theChar;
	TranslatePalChar(&intChar, 0, False);
	theChar = (unsigned char)intChar;

	sym = GetSymTableIndex(theChar);
	if (symtable[sym].objtype!=SYNCtype) return -1;
	for (unsigned short k = 0; k<numDurations; k++)
		if (symtable[sym].durcode==durPalCode[k]) { newDurIdx = k;  break; }
	
	return newDurIdx;
}


short DurCodeToDurPalIdx(short durCode, short nDots, short nDurations)
{
	short durPalIdx = -1;									/* In case we can't find it */
	for (unsigned short k = 0; k<nDurations; k++) {
		if (durCode==durPalCode[k] && nDots==durPalNDots[k]) {
			durPalIdx = k;
			break;
		}
	}
	return durPalIdx;
}

short DurPalIdxToDurCode(short durPalIdx, short *pNDots)
{
	*pNDots = durPalNDots[durPalIdx];
	return durPalCode[durPalIdx];
}


/* ------------------------------------------------------------------ DurPalChoiceDlog -- */
/* Display a simple modal dialog for choosing an undotted or single-dotted duration. */

static short choiceIdx, nPalRows;

static pascal Boolean DurPalChoiceFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static pascal Boolean DurPalChoiceFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr		w;
	short			ch, ans, type;
	Handle			hndl;
	Rect			box;
	Point			where;
	GrafPtr			oldPort;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort);  SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, OK, True);
				GetDialogItem(dlog, DP_DURCHOICE_DI, &type, &hndl, &box);
//LogPrintf(LOG_DEBUG, "DurPalChoiceFilter: choiceIdx=%d box tlbr=%d,%d,%d,%d\n",
//choiceIdx, box.top, box.left, box.bottom, box.right);
				FrameRect(&box);
				DrawBMP(bmpDurationPal.bitmap, bmpDurationPal.byWidth, bmpDurationPal.byWidthPadded,
						bmpDurationPal.height, nPalRows*DP_ROW_HEIGHT, box);
				HiliteDurCell(choiceIdx, &box, durPalCell);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return True;
			}
			break;
		case activateEvt:
			if (w==GetDialogWindow(dlog)) SetPort(GetDialogWindowPort(dlog));
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			GetDialogItem(dlog, DP_DURCHOICE_DI, &type, &hndl, &box);
			if (PtInRect(where, &box)) {
				if (evt->what==mouseUp) {
					/* If the mouseUp was in an invalid (presumably because empty) cell,
					   ignore it. Otherwise, unhilite the previously-selected cell and
					   hilite the new one. */

					short newDurIdx = FindDurationCell(where, &box, DP_NCOLS, nPalRows,
										durPalCell);
//LogPrintf(LOG_DEBUG, "DurPalChoiceFilter: choiceIdx=%d newDurIdx=%d\n", choiceIdx, newDurIdx);  
					if (newDurIdx<0 || newDurIdx>(short)DP_NDURATIONS-1) return False;
					if (durPalCode[newDurIdx]==NO_L_DUR) return False;
					SWITCH_DPCELL(choiceIdx, newDurIdx, &box);
				}
				*itemHit = DP_DURCHOICE_DI;
				return True;
			}
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False)) return True;
			ch = (unsigned char)evt->message;
			GetDialogItem(dlog, DP_DURCHOICE_DI, &type, &hndl, &box);
			*itemHit = 0;
			ans = DurationKey(ch, DP_NDURATIONS);
			if (ans>=0) {
//LogPrintf(LOG_DEBUG, "DurPalChoiceFilter: ch='%c' choiceIdx=%d ans=%d\n", ch, choiceIdx, ans);  
				SWITCH_DPCELL(choiceIdx, ans, &box);
				*itemHit = DP_DURCHOICE_DI;
				return True;
			}
			else {
				ans = choiceIdx;
				switch (ch) {
					case LEFTARROWKEY:
						if (ans>0) ans--;
						break;
					case RIGHTARROWKEY:
						if (ans<(DP_NCOLS*nPalRows)-1) ans++;
						break;
					case UPARROWKEY:
						if (ans>DP_NCOLS-1) ans -= DP_NCOLS;
						break;
					case DOWNARROWKEY:
						if (ans<DP_NCOLS*(nPalRows-1)) ans += DP_NCOLS;
						break;
					default:
						;
					}
				if (ans!=choiceIdx) {
					if (durPalCode[ans]==NO_L_DUR) return False;
					SWITCH_DPCELL(choiceIdx, ans, &box);
				}
			}
	}
	
	return False;
}

/* Display a simple modal dialog for choosing an undotted or single-dotted duration.
<durPalIdx> is an index into the full duration palette; <maxDots> must be 0 or 1.
Attempts to choose cells with code NO_L_DUR (presumably blank) are ignored. Return
the new <durPalIdx> if OK, -1 if Cancel or error. */

short DurPalChoiceDlog(short durPalIdx, short maxDots)
{
	DialogPtr		dlog;
	unsigned short	durChoiceDlogID;
	short			itemHit, type;
	Boolean			okay, keepGoing=True;
	Handle			hndl;
	Rect			box;
	GrafPtr			oldPort;
	ModalFilterUPP	filterUPP;

	if (maxDots!=0 && maxDots!=1) {
		AlwaysErrMsg("DurPalChoiceDlog: maxDots must be 0 or 1!");
		return -1;
	}

	if (maxDots==1) {
		filterUPP = NewModalFilterUPP(DurPalChoiceFilter);
		if (filterUPP==NULL) { MissingDialog(DURCHOICE_1DOT_DLOG); return -1; }
		durChoiceDlogID = DURCHOICE_1DOT_DLOG;
	}
	else {
		filterUPP = NewModalFilterUPP(DurPalChoiceFilter);
		if (filterUPP==NULL) { MissingDialog(DURCHOICE_NODOTS_DLOG); return -1; }
		durChoiceDlogID = DURCHOICE_NODOTS_DLOG;
	}

	dlog = GetNewDialog(durChoiceDlogID, NULL, BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(durChoiceDlogID);
		return -1;
	}
	
	nPalRows = maxDots+1;

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	GetDialogItem(dlog, DP_DURCHOICE_DI, &type, &hndl, &box);
	InitDurationCells(&box, DP_NCOLS, nPalRows, durPalCell);

	/* Hilite the initially-selected cell. */
	
	choiceIdx = durPalIdx;
	HiliteDurCell(choiceIdx, &box, durPalCell);
	
	CenterWindow(GetDialogWindow(dlog), 120);
	ShowWindow(GetDialogWindow(dlog));

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP, &itemHit);
		if (itemHit<1 || itemHit>=DP_LAST_DI_SD) continue;
		GetDialogItem(dlog, itemHit, &type, &hndl, &box);
		switch (itemHit) {
			case OK:
				keepGoing = False; okay = True;
				break;
			case Cancel:
				keepGoing = False;
				break;
			case DP_DURCHOICE_DI:					/* handled in filter */
				break;
		}
	}
	
	DisposeModalFilterUPP(DurPalChoiceFilter);
	DisposeDialog(dlog);
	SetPort(oldPort);
	okay = (itemHit==OK);
	return (okay? choiceIdx : -1);
}


/* ================================================= Code for the Set Duration command == */

//static short choiceIdx;	//USE A DIFF. VARIABLE FROM ABOVE? I DON'T SEE WHY

static Boolean DrawDurationPalette(Rect *pBox);
static Boolean DrawDurationPalette(Rect *pBox)
{	
//DHexDump(LOG_DEBUG, "DurPal", bmpDurationPal.bitmap, 4*16, 4, 16, True);

	DrawBMP(bmpDurationPal.bitmap, bmpDurationPal.byWidth, bmpDurationPal.byWidthPadded,
			bmpDurationPal.height, bmpDurationPal.height, *pBox);
	return True;
}
								 

#define DE_NROWS 3		/* Show all three rows (no dots, one dot, two dots) of duration palette */
#define NDURATIONS DE_NROWS*DP_NCOLS

/* ---------------------------------------------------------------------- SetDurDialog -- */

static Boolean SDAnyBadValues(Document *, DialogPtr, Boolean, short, short, short);
static pascal Boolean SetDurFilter(DialogPtr, EventRecord *, short *);

enum {
	SETLDUR_DI=3,
	SDDURPAL_DI,
	CV_DI,
	SETPDUR_DI,
	PDURPCT_DI,
	DUMMYFLD_DI=9,
	HALVEDURS_DI=10,
	DOUBLEDURS_DI,
	SETDURSTO_DI
};

static short setDurGroup;

static Boolean SDAnyBadValues(Document *doc, DialogPtr dlog, Boolean newSetLDur,
								short newLDurAction, short /*newnDots*/, short newpDurPct)
{	
	if (newpDurPct<1 || newpDurPct>500) {
		GetIndCString(strBuf, DIALOGERRS_STRS, 13);				/* "Play duration percent must be..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		SelectDialogItemText(dlog, PDURPCT_DI, 0, ENDTEXT);
		return True;
	}

	if (newSetLDur) {
		if (newLDurAction==SET_DURS_TO) {
			if (IsSelInTuplet(doc)) {
				GetIndCString(strBuf, DIALOGERRS_STRS, 14);		/* "can't Set notated Duration in tuplets" */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return True;
			}
		}
		else {
			if (IsSelInTupletNotAllSel(doc)) {
				GetIndCString(strBuf, DIALOGERRS_STRS, 21);		/* "...select all the notes of the tuplet" */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return True;
			}
		}
	}

	return False;
}

static pascal Boolean SetDurFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr		w;
	short			ch, ans, type;
	Handle			hndl;
	Rect			box;
	Point			where;
	GrafPtr			oldPort;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort);  SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, OK, True);
				GetDialogItem(dlog, SDDURPAL_DI, &type, &hndl, &box);
//LogPrintf(LOG_DEBUG, "DurationFilter: box tlbr=%d,%d,%d,%d\n", box.top, box.left, box.bottom,
//box.right);
				FrameRect(&box);
				DrawDurationPalette(&box);
				HiliteDurCell(choiceIdx, &box, durPalCell);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return True;
			}
			break;
		case activateEvt:
			if (w==GetDialogWindow(dlog)) SetPort(GetDialogWindowPort(dlog));
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			GetDialogItem(dlog, SDDURPAL_DI, &type, &hndl, &box);
			if (PtInRect(where, &box)) {
				if (evt->what==mouseUp) {
					/* If the mouseUp was in an invalid (presumably because empty) cell,
					   ignore it. Otherwise, unhilite the previously-selected cell and
					   hilite the new one. */

					short newDurIdx = FindDurationCell(where, &box, DP_NCOLS, DP_NROWS, durPalCell);
//LogPrintf(LOG_DEBUG, "SetDurFilter: choiceIdx=%d newDurIdx=%d\n", choiceIdx, newDurIdx);
					if (newDurIdx<0 || newDurIdx>(short)NDURATIONS-1) return False;
					if (durPalCode[newDurIdx]==NO_L_DUR) return False;
					SWITCH_DPCELL(choiceIdx, newDurIdx, &box);
				}
				*itemHit = SDDURPAL_DI;
				return True;
			}
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False)) return True;
			ch = (unsigned char)evt->message;
			GetDialogItem(dlog, SDDURPAL_DI, &type, &hndl, &box);
			*itemHit = 0;
			
			/* If the character is something in the palette, use that something; if not
			   and it's an arrow key, move in the appropriate direction, but ignore it
			   if the new cell is empty (it is iff its code is NO_L_DUR); otherwise
			   ignore it. */
			   
			ans = DurationKey(ch, NDURATIONS);
			if (ans>=0) {
//LogPrintf(LOG_DEBUG, "SetDurFilter: ch='%c' choiceIdx=%d ans=%d\n", ch, choiceIdx, ans);
				SWITCH_DPCELL(choiceIdx, ans, &box);
				*itemHit = SDDURPAL_DI;
				return True;
			}
			else {
				ans = choiceIdx;
				switch (ch) {
					case LEFTARROWKEY:
						if (ans>0) ans--;
						break;
					case RIGHTARROWKEY:
						if (ans<(DP_NCOLS*DP_NROWS)-1) ans++;
						break;
					case UPARROWKEY:
						if (ans>DP_NCOLS-1) ans -= DP_NCOLS;
						break;
					case DOWNARROWKEY:
						if (ans<DP_NCOLS*(DP_NROWS-1)) ans += DP_NCOLS;
						break;
					default:
						;
					}
				if (ans!=choiceIdx) {
					if (durPalCode[ans]==NO_L_DUR) return False;
					SWITCH_DPCELL(choiceIdx, ans, &box);
				}
			}
	}
	
	return False;
}


static void XableLDurPanel(DialogPtr dlog, Boolean enable)
{
	if (enable) {
		XableControl(dlog, HALVEDURS_DI, True);
		XableControl(dlog, DOUBLEDURS_DI, True);
		XableControl(dlog, SETDURSTO_DI, True);
		XableControl(dlog, CV_DI, True);
	}
	else {
		XableControl(dlog, HALVEDURS_DI, False);
		XableControl(dlog, DOUBLEDURS_DI, False);
		XableControl(dlog, SETDURSTO_DI, False);
		XableControl(dlog, CV_DI, False);
	}
}

Boolean SetDurDialog(
				Document *doc,
				Boolean *setLDur,			/* Set "notated" (logical) durations? */
				short *lDurAction,			/* HALVE_DURS, DOUBLE_DURS, or SET_DURS_TO */
				short *lDurCode,
				short *nDots,
				Boolean *setPDur,			/* Set play durations? */
				short *pDurPct,
				Boolean *cptV,				/* Compact Voices (remove gaps) afterwards? */
				Boolean *doUnbeam			/* Unbeam selection first? */
				)	
{	
	DialogPtr dlog;
	short ditem=Cancel, type;
	Boolean dialogOver, beamed;
	Handle hndl;
	Rect box;
	GrafPtr oldPort;
	ModalFilterUPP	filterUPP;
	short newpDurPct;
	short newnDots, newSetLDur, newLDur;

//LogPrintf(LOG_DEBUG, "SetDurDialog: byWidth=%d byWidthPadded=%d height=%d\n",
//bmpDurationPal.byWidth, bmpDurationPal.byWidthPadded, bmpDurationPal.height);
	filterUPP = NewModalFilterUPP(SetDurFilter);
	if (filterUPP == NULL) {
		MissingDialog(SETDUR_DLOG);
		return False;
	}
	
	dlog = GetNewDialog(SETDUR_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SETDUR_DLOG);
		return False;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	if (*nDots>2) {
		SysBeep(1);										/* palette has 2 dots maximum */
		LogPrintf(LOG_WARNING, "Can't handle %d dots: 2 is the maximum.  (SetDurDialog)\n", *nDots);
		 *nDots = 2;
	}

	*doUnbeam = False;
	beamed = (SelBeamedNote(doc)!=NILINK);

	newLDur = *lDurCode;
	newnDots = *nDots;
	newpDurPct = *pDurPct;
	
	if (*lDurAction==HALVE_DURS)		setDurGroup = HALVEDURS_DI;
	else if (*lDurAction==DOUBLE_DURS)	setDurGroup = DOUBLEDURS_DI;
	else								setDurGroup = SETDURSTO_DI;
	PutDlgChkRadio(dlog, setDurGroup, True);

	GetDialogItem(dlog, SDDURPAL_DI, &type, &hndl, &box);
	InitDurationCells(&box, DP_NCOLS, DP_NROWS, durPalCell);
	
	choiceIdx = DurCodeToDurPalIdx(*lDurCode, *nDots, NDURATIONS);
	if (choiceIdx<0) {
		SysBeep(1);
		LogPrintf(LOG_WARNING, "Illegal code %d for tuplet duration unit.  (SetDurDialog)\n",
					*lDurCode);
		choiceIdx = 3;									/* An arbitrary choice */
	}
	
	HiliteDurCell(choiceIdx, &box, durPalCell);

	PutDlgChkRadio(dlog, SETLDUR_DI, *setLDur);
	PutDlgChkRadio(dlog, SETPDUR_DI, *setPDur);
	hndl = PutDlgChkRadio(dlog, CV_DI, *cptV);
	HiliteControl((ControlHandle)hndl, (*setLDur? CTL_ACTIVE : CTL_INACTIVE));
	
	PutDlgWord(dlog, PDURPCT_DI, *pDurPct, False);

	if (*setLDur) {
		XableLDurPanel(dlog, True);
		SelectDialogItemText(dlog, DUMMYFLD_DI, 0, ENDTEXT);
	}
	else {
		XableLDurPanel(dlog, False);
		SelectDialogItemText(dlog, PDURPCT_DI, 0, ENDTEXT);
	}
	CenterWindow(GetDialogWindow(dlog), 100);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	/* Entertain filtered user events until dialog is dismissed */

	dialogOver = False;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
				GetDlgWord(dlog, PDURPCT_DI, &newpDurPct);
				newSetLDur = GetDlgChkRadio(dlog, SETLDUR_DI);
				
				if (setDurGroup==HALVEDURS_DI)			*lDurAction = HALVE_DURS;
				else if (setDurGroup==DOUBLEDURS_DI)	*lDurAction = DOUBLE_DURS;
				else									*lDurAction = SET_DURS_TO;

				if (!SDAnyBadValues(doc, dlog, newSetLDur, *lDurAction, newnDots, newpDurPct)) {
				
					/* If the logical durations are to be set and any selected note is beamed,
					   it must be unbeamed first; tell the user. */
			
					if (newSetLDur && beamed) {
						if (CautionAdvise(SDBEAM_ALRT)==Cancel) break;
						*doUnbeam = True;						
					}
					*setLDur = newSetLDur;
					*lDurCode = durPalCode[choiceIdx];
					*nDots = durPalNDots[choiceIdx];
					*setPDur = GetDlgChkRadio(dlog, SETPDUR_DI);
					*pDurPct = newpDurPct;
					*cptV = GetDlgChkRadio(dlog, CV_DI);
					dialogOver = True;
				}
			case Cancel:
				dialogOver = True;
				break;
			case HALVEDURS_DI:
			case DOUBLEDURS_DI:
			case SETDURSTO_DI:
				if (ditem!=setDurGroup) SwitchRadio(dlog, &setDurGroup, ditem);
				break;
			case SETLDUR_DI:
				PutDlgChkRadio(dlog, SETLDUR_DI, !GetDlgChkRadio(dlog, SETLDUR_DI));
				newSetLDur = GetDlgChkRadio(dlog, SETLDUR_DI);
				if (newSetLDur) {
					XableLDurPanel(dlog, True);
				}
				else {
					XableLDurPanel(dlog, False);
					SelectDialogItemText(dlog, PDURPCT_DI, 0, ENDTEXT);
				}
				break;
			case SDDURPAL_DI:
				PutDlgChkRadio(dlog, SETLDUR_DI, newSetLDur = True);
				XableLDurPanel(dlog, True);
				if (setDurGroup!=SETDURSTO_DI) SwitchRadio(dlog, &setDurGroup, SETDURSTO_DI);
#ifdef NOTYET
				SelectDialogItemText(dlog, DUMMYFLD_DI, 0, ENDTEXT);
				//HiliteGPopUp(curPop, popUpHilited = True);
#endif
				break;
			case CV_DI:
			case SETPDUR_DI:
				PutDlgChkRadio(dlog, ditem, !GetDlgChkRadio(dlog, ditem));
				break;
			case PDURPCT_DI:
				PutDlgChkRadio(dlog, SETPDUR_DI, True);
				break;
			default:
				;
		}
	}
		
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	SetPort(oldPort);
	return (ditem==OK);
}
