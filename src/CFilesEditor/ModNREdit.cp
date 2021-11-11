/* ModNREdit.c for Nightingale - formerly ModNRPopUp, for use with accompanying graphic
MDEF; it now uses a BMP displayed in its entirety.  */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016-21 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


enum {
	MODNR_CHOICES_DI=4
};

static short modNRIdx;

static Boolean DrawModNRPalette(Rect *pBox);
static Boolean DrawModNRPalette(Rect *pBox)
{	
DHexDump(LOG_DEBUG, "ModNRPal", bmpModifierPal.bitmap, 8*16, 4, 16, True);

	DrawBMP(bmpModifierPal.bitmap, bmpModifierPal.bWidth, bmpModifierPal.bWidthPadded,
			bmpModifierPal.height, *pBox);
	return True;
}


static short modNRCode[] = {
	MOD_STACCATO, MOD_WEDGE, MOD_ACCENT, MOD_TENUTO, MOD_HEAVYACCENT, MOD_HEAVYACC_STACC,
	MOD_FERMATA, MOD_TRILL, MOD_INV_MORDENT, MOD_MORDENT, MOD_LONG_INVMORDENT, MOD_TURN,
	0, 1, 2, 3, 4, 5,
	MOD_UPBOW, MOD_DOWNBOW, MOD_CIRCLE, MOD_PLUS
};
								
#define NMODNRS (sizeof modNRCode/sizeof(short))

#define NROWS 4		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?
#define NCOLS 6		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?

static Rect modNRCell[NROWS*NCOLS];

static void InitModNRCells(Rect *pBox);
static void InitModNRCells(Rect *pBox)
{
	short cellWidth, cellHeight, cellNum;
	
	cellWidth = (pBox->right-pBox->left)/NCOLS;
	cellHeight = (pBox->bottom-pBox->top)/NROWS;
//LogPrintf(LOG_DEBUG, "InitModNRCells: cellWidth=%d cellHeight==%d\n", cellWidth, cellHeight);

	for (short rn = 0; rn<NROWS; rn++)
		for (short cn = 0; cn<NCOLS; cn++) {
			cellNum = (NCOLS*rn) + cn;
			modNRCell[cellNum].top = cellHeight*rn;
			modNRCell[cellNum].left = cellWidth*cn;
			modNRCell[cellNum].bottom = modNRCell[cellNum].top+cellHeight;
			modNRCell[cellNum].right = modNRCell[cellNum].left+cellWidth;
//LogPrintf(LOG_DEBUG, "InitModNRCells: rn=%d cn=%d cellNum=%d modNRCell tlbr=%d,%d,%d,%d\n",
//rn, cn, cellNum, modNRCell[cellNum].top, modNRCell[cellNum].left,
//modNRCell[cellNum].bottom, modNRCell[cellNum].right);
		}
}


/* Return the number of the cell in the modifier palette containing the given point. We
don't check whether cell is a valid selection; it's up to the calling routine to do that. */
 
static short FindModNRCell(Point where, Rect *pBox);
static short FindModNRCell(Point where, Rect *pBox)
{
	short xInBox, yInBox, cellNum, tryCell;
	
	xInBox = where.h - pBox->left;
	yInBox = where.v - pBox->top;
	cellNum = -1;
	for (short rn = 0; rn<NROWS; rn++)
		for (short cn = 0; cn<NCOLS; cn++) {
			tryCell = (NCOLS*rn) + cn;
			if (yInBox<modNRCell[tryCell].top) continue;
			if (xInBox<modNRCell[tryCell].left) continue;
			if (yInBox>=modNRCell[tryCell].bottom) continue;
			if (xInBox>=modNRCell[tryCell].right) continue;
			cellNum = tryCell;  break;
		}

//LogPrintf(LOG_DEBUG, "FindModNRCell: xInBox=%d yInBox=%d cellNum=%d\n", xInBox, yInBox, cellNum);
	if (cellNum<0) {
		LogPrintf(LOG_WARNING, "Can't find cell for where=(%d,%d) xInBox=%d yInBox=%d.  (FindModNRCell)\n",
					where.h, where.v, xInBox, yInBox);
		cellNum = 1;
	}
	
	return cellNum;
}


static void HiliteCell(short modIdx, Rect *box);
static void HiliteCell(short modIdx, Rect *box)
{
	Rect theCell = modNRCell[modIdx];
	OffsetRect(&theCell, box->left, box->top);
	InsetRect(&theCell, 1, 1);
	InvertRect(&theCell);
}

/* Return the index into our palette for the given character. If the character isn't in
the palette, return -1. */

static short ModNRKey(unsigned char theChar);
static short ModNRKey(unsigned char theChar)
{
	short newModNRIdx=-1, intChar, sym;
		
	/* First remap theChar according to the 'PLMP' resource. */
	
	intChar = (short)theChar;
	TranslatePalChar(&intChar, 0, False);
	theChar = (unsigned char)intChar;

	sym = GetSymTableIndex(theChar);
	if (symtable[sym].objtype!=MODNRtype) return -1;
	for (unsigned short k = 0; k<NMODNRS; k++)
		if (symtable[sym].subtype==modNRCode[k]) { newModNRIdx = k;  break; }
	
	return newModNRIdx;
}


#define SWITCH_HILITE(curIdx, newIdx, pBox)		HiliteCell((curIdx), pBox); curIdx = (newIdx); HiliteCell((curIdx), pBox)

static pascal Boolean ModNRFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static pascal Boolean ModNRFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
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
				GetDialogItem(dlog, MODNR_CHOICES_DI, &type, &hndl, &box);
//LogPrintf(LOG_DEBUG, "ModNRFilter: box tlbr=%d,%d,%d,%d\n", box.top, box.left, box.bottom,
//box.right);
				FrameRect(&box);
				DrawModNRPalette(&box);
				HiliteCell(modNRIdx, &box);
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
			GetDialogItem(dlog, MODNR_CHOICES_DI, &type, &hndl, &box);
			if (PtInRect(where, &box)) {
LogPrintf(LOG_DEBUG, "ModNRFilter: where=%d,%d\n", where.h, where.v);
				if (evt->what==mouseUp) {
					/* If the mouseUp was in an invalid (presumably because empty) cell,
					   ignore it. Otherwise, unhilite the previously-selected cell and
					   hilite the new one. */

					short newModNRIdx = FindModNRCell(where, &box);
					if (newModNRIdx<0 || newModNRIdx>(short)NMODNRS-1) return False;
					SWITCH_HILITE(modNRIdx, newModNRIdx, &box);
				}
				*itemHit = MODNR_CHOICES_DI;
				return True;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False)) return True;
			ch = (unsigned char)evt->message;
			GetDialogItem(dlog, MODNR_CHOICES_DI, &type, &hndl, &box);
			*itemHit = 0;
			ans = ModNRKey(ch);
			if (ans>=0) {
LogPrintf(LOG_DEBUG, "ModNRFilter: ch='%c' modNRIdx=%d ans=%d\n", ch, modNRIdx, ans);
				SWITCH_HILITE(modNRIdx, ans, &box);
				*itemHit = MODNR_CHOICES_DI;
				return True;
			}
	}
	
	return False;
}

/* ------------------------------------------------------------------- xxxModNRDialogs -- */

Boolean ModNRDialog(unsigned short dlogID, Byte *modCode)
{	
	DialogPtr dlog;
	short ditem=Cancel, type, oldResFile;
	Boolean dialogOver;
	Handle hndl;
	Rect box, theCell;
	GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(ModNRFilter);
	if (filterUPP == NULL) {
		MissingDialog(dlogID);
		return False;
	}
	
	dlog = GetNewDialog(dlogID, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(dlogID);
		return False;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);							/* popup code uses Get1Resource */

	GetDialogItem(dlog, MODNR_CHOICES_DI, &type, &hndl, &box);
	InitModNRCells(&box);
	
	/* Find and hilite the initially-selected cell. We should always find it, but if
	   we can't, make an arbitrary choice. */
	
	modNRIdx = 3;									/* In case we can't find it */
	for (unsigned short k = 0; k<NMODNRS; k++) {
		if (*modCode==modNRCode[k]) { modNRIdx = k;  break; }
	}
	
	theCell = modNRCell[modNRIdx];
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
		*modCode = modNRCode[modNRIdx];
		
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}
