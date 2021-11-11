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


static short durationIdx;

enum {
	DURATION_CHOICES_DI=4
};

static Boolean DrawDurationPalette(Rect *pBox);
static Boolean DrawDurationPalette(Rect *pBox)
{	
DHexDump(LOG_DEBUG, "DurPal", bmpDurationPal.bitmap, 8*16, 4, 16, True);

	DrawBMP(bmpDurationPal.bitmap, bmpDurationPal.bWidth, bmpDurationPal.bWidthPadded,
			bmpDurationPal.height, *pBox);
	return True;
}


static short durationCode[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1 };
								
#define NDURATIONS (sizeof durationCode/sizeof(short))

#define NROWS 3		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?
#define NCOLS 9		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?

static Rect durationCell[NROWS*NCOLS];

static void InitDurationCells(Rect *pBox);
static void InitDurationCells(Rect *pBox)
{
	short cellWidth, cellHeight, cellNum;
	
	cellWidth = (pBox->right-pBox->left)/NCOLS;
	cellHeight = (pBox->bottom-pBox->top)/NROWS;
//LogPrintf(LOG_DEBUG, "InitDurationCells: cellWidth=%d cellHeight==%d\n", cellWidth, cellHeight);

	for (short rn = 0; rn<NROWS; rn++)
		for (short cn = 0; cn<NCOLS; cn++) {
			cellNum = (NCOLS*rn) + cn;
			durationCell[cellNum].top = cellHeight*rn;
			durationCell[cellNum].left = cellWidth*cn;
			durationCell[cellNum].bottom = durationCell[cellNum].top+cellHeight;
			durationCell[cellNum].right = durationCell[cellNum].left+cellWidth;
//LogPrintf(LOG_DEBUG, "InitDurationCells: rn=%d cn=%d cellNum=%d durationCell tlbr=%d,%d,%d,%d\n",
//rn, cn, cellNum, durationCell[cellNum].top, durationCell[cellNum].left,
//durationCell[cellNum].bottom, durationCell[cellNum].right);
		}
}


/* Return the number of the cell in the duration palette containing the given point. We
don't check whether cell is a valid selection; it's up to the calling routine to do that. */
 
static short FindDurationCell(Point where, Rect *pBox);
static short FindDurationCell(Point where, Rect *pBox)
{
	short xInBox, yInBox, cellNum, tryCell;
	
	xInBox = where.h - pBox->left;
	yInBox = where.v - pBox->top;
	cellNum = -1;
	for (short rn = 0; rn<NROWS; rn++)
		for (short cn = 0; cn<NCOLS; cn++) {
			tryCell = (NCOLS*rn) + cn;
			if (yInBox<durationCell[tryCell].top) continue;
			if (xInBox<durationCell[tryCell].left) continue;
			if (yInBox>=durationCell[tryCell].bottom) continue;
			if (xInBox>=durationCell[tryCell].right) continue;
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


static void HiliteCell(short durIdx, Rect *box);
static void HiliteCell(short durIdx, Rect *box)
{
	Rect theCell = durationCell[durIdx];
	OffsetRect(&theCell, box->left, box->top);
	InsetRect(&theCell, 1, 1);
	InvertRect(&theCell);
}


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


#define SWITCH_HILITE(curIdx, newIdx, pBox)		HiliteCell((curIdx), pBox); curIdx = (newIdx); HiliteCell((curIdx), pBox)

static pascal Boolean DurationFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static pascal Boolean DurationFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
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
				GetDialogItem(dlog, DURATION_CHOICES_DI, &type, &hndl, &box);
//LogPrintf(LOG_DEBUG, "DurationFilter: box tlbr=%d,%d,%d,%d\n", box.top, box.left, box.bottom,
//box.right);
				FrameRect(&box);
				DrawDurationPalette(&box);
				HiliteCell(durationIdx, &box);
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
			GetDialogItem(dlog, DURATION_CHOICES_DI, &type, &hndl, &box);
			if (PtInRect(where, &box)) {
				if (evt->what==mouseUp) {
					/* If the mouseUp was in an invalid (presumably because empty) cell,
					   ignore it. Otherwise, unhilite the previously-selected cell and
					   hilite the new one. */

					short newDurIdx = FindDurationCell(where, &box);
					if (newDurIdx<0 || newDurIdx>(short)NDURATIONS-1) return False;
					SWITCH_HILITE(durationIdx, newDurIdx, &box);
				}
				*itemHit = DURATION_CHOICES_DI;
				return True;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False)) return True;
			ch = (unsigned char)evt->message;
			GetDialogItem(dlog, DURATION_CHOICES_DI, &type, &hndl, &box);
			*itemHit = 0;
			ans = DurationKey(ch);
			if (ans>=0) {
LogPrintf(LOG_DEBUG, "ch='%c' durationIdx=%d ans=%d\n", ch, durationIdx, ans);  
				SWITCH_HILITE(durationIdx, ans, &box);
				*itemHit = DURATION_CHOICES_DI;
				return True;
			}
	}
	
	return False;
}

Boolean SetDurationDialog(SignedByte *durationType)
{	
	DialogPtr dlog;
	short ditem=Cancel, type, oldResFile;
	Boolean dialogOver;
	Handle hndl;
	Rect box, theCell;
	GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(DurationFilter);
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

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);							/* popup code uses Get1Resource */

	GetDialogItem(dlog, DURATION_CHOICES_DI, &type, &hndl, &box);
	InitDurationCells(&box);
	
	/* Find and hilite the initially-selected cell. We should always find it, but if
	   we can't, make an arbitrary choice. */
	
	durationIdx = 3;									/* In case we can't find it */
	for (unsigned short k = 0; k<NDURATIONS; k++) {
		if (*durationType==durationCode[k]) { durationIdx = k;  break; }
	}
	
	theCell = durationCell[durationIdx];
	OffsetRect(&theCell, box.left, box.top);
	InvertRect(&theCell);

	CenterWindow(GetDialogWindow(dlog), 100);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	dialogOver = False;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
			case Cancel:
				dialogOver = True;
				break;
			default:
				;
		}
	}
	if (ditem==OK)
		*durationType = durationCode[durationIdx];
		
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}
