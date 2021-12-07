/* DurationEdit.c for Nightingale - formerly DurationPopUp, for use with accompanying
graphic MDEF; it now uses a BMP displayed in its entirety.  */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016-21 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ----------------------------------- General routines to handle the duration palette -- */
/* Note durations appear in features of Nightingale other than Set Duration: metronome
marks, "fancy" tuplets, and quantizing durations. Note that different features allow
different numbers of augmentation dots and therefore different subsets of the palette!
The following routines are generic. */

void InitDurationCells(Rect *pBox, short nCols, short nRows, Rect durCell[])
{
	short cellWidth, cellHeight, cellNum;
	
	cellWidth = (pBox->right-pBox->left)/nCols;
	cellHeight = (pBox->bottom-pBox->top)/nRows;
//LogPrintf(LOG_DEBUG, "InitDurationCells: cellWidth=%d cellHeight==%d\n", cellWidth, cellHeight);

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

//LogPrintf(LOG_DEBUG, "FindDurationCell: xInBox=%d yInBox=%d cellNum=%d\n", xInBox, yInBox, cellNum);
	if (cellNum<0) {
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


/* ------------------------------------------------- Code for the Set Duration command -- */

static short durationIdx;

static Boolean DrawDurationPalette(Rect *pBox);
static Boolean DrawDurationPalette(Rect *pBox)
{	
//DHexDump(LOG_DEBUG, "DurPal", bmpDurationPal.bitmap, 4*16, 4, 16, True);

	DrawBMP(bmpDurationPal.bitmap, bmpDurationPal.bWidth, bmpDurationPal.bWidthPadded,
			bmpDurationPal.height, bmpDurationPal.height, *pBox);
	return True;
}


static short durationCode[] =	{ 9, 8, 7, 6, 5, 4, 3, 2, 1,
								  9, 8, 7, 6, 5, 4, 3, 2, 1,
								  9, 8, 7, 6, 5, 4, 3, 2, 1 };
static short durNDots[] =		{ 0, 0, 0, 0, 0, 0, 0, 0, 0,
								  1, 1, 1, 1, 1, 1, 1, 1, 1,
								  2, 2, 2, 2, 2, 2, 2, 2, 2 };
#define NDURATIONS (sizeof durationCode/sizeof(short))

#define NROWS 3		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?
#define NCOLS 9		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?

static Rect durationCell[NROWS*NCOLS];


/* Return the index into our palette for the given character. If the character isn't in
the palette, return -1. */

static short DurationKey(unsigned char theChar);
static short DurationKey(unsigned char theChar)
{
	short newDurIdx=-1, intChar, sym;
		
	/* First remap theChar according to the 'PLMP' resource. */
	
	intChar = (short)theChar;
	TranslatePalChar(&intChar, 0, False);
	theChar = (unsigned char)intChar;

	sym = GetSymTableIndex(theChar);
	if (symtable[sym].objtype!=SYNCtype) return -1;
	for (unsigned short k = 0; k<NDURATIONS; k++)
		if (symtable[sym].durcode==durationCode[k]) { newDurIdx = k;  break; }
	
	return newDurIdx;
}


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
			if (IsSelInTupletNotTotallySel(doc)) {
				GetIndCString(strBuf, DIALOGERRS_STRS, 21);		/* "...select all the notes of the tuplet" */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return True;
			}
		}
	}

	return False;
}

#define SWITCH_HILITE(curIdx, newIdx, pBox)		HiliteDurCell((curIdx), (pBox), durationCell); curIdx = (newIdx); HiliteDurCell((curIdx), (pBox), durationCell)

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
				HiliteDurCell(durationIdx, &box, durationCell);
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

					short newDurIdx = FindDurationCell(where, &box, NCOLS, NROWS, durationCell);
					if (newDurIdx<0 || newDurIdx>(short)NDURATIONS-1) return False;
					SWITCH_HILITE(durationIdx, newDurIdx, &box);
				}
				*itemHit = SDDURPAL_DI;
				return True;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False)) return True;
			ch = (unsigned char)evt->message;
			GetDialogItem(dlog, SDDURPAL_DI, &type, &hndl, &box);
			*itemHit = 0;
			ans = DurationKey(ch);
			if (ans>=0) {
//LogPrintf(LOG_DEBUG, "ch='%c' durationIdx=%d ans=%d\n", ch, durationIdx, ans);  
				SWITCH_HILITE(durationIdx, ans, &box);
				*itemHit = SDDURPAL_DI;
				return True;
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
	Rect box, theCell;
	GrafPtr oldPort;
	ModalFilterUPP	filterUPP;
	short newpDurPct;
	short newnDots, newSetLDur, newLDur;

LogPrintf(LOG_DEBUG, "SetDurDialog: bWidth=%d bWidthPadded=%d height=%d\n",
bmpDurationPal.bWidth, bmpDurationPal.bWidthPadded, bmpDurationPal.height);
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
		SysBeep(1);										/* popup has 2 dots maximum */
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
	InitDurationCells(&box, NCOLS, NROWS, durationCell);
	
	/* Find and hilite the initially-selected cell. We should always find it, but if
	   we can't, make an arbitrary choice. */
	
	durationIdx = 3;									/* In case we can't find it */
	for (unsigned short k = 0; k<NDURATIONS; k++) {
		if (*lDurCode==durationCode[k] && *nDots==durNDots[k]) { durationIdx = k;  break; }
	}
	
	theCell = durationCell[durationIdx];
	OffsetRect(&theCell, box.left, box.top);
	InvertRect(&theCell);

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
					*lDurCode = durationCode[durationIdx];
					*nDots = durNDots[durationIdx];
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
				if (ditem!=setDurGroup)
					SwitchRadio(dlog, &setDurGroup, ditem);
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
					//HiliteGPopUp(curPop, popUpHilited = False);
				}
				break;
			case SDDURPAL_DI:
				PutDlgChkRadio(dlog, SETLDUR_DI, newSetLDur = True);
				XableLDurPanel(dlog, True);
				if (setDurGroup!=SETDURSTO_DI)
					SwitchRadio(dlog, &setDurGroup, SETDURSTO_DI);
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
				//if (popUpHilited)
					//HiliteGPopUp(curPop, popUpHilited = False);
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
