/* DynamicEdit.c for Nightingale - formerly DynamicPopUp, for use with accompanying graphic
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
	DYNAM_CHOICES_DI=4
};

static short dynamicIdx;

static Boolean DrawDynamicPalette(Rect *pBox);
static Boolean DrawDynamicPalette(Rect *pBox)
{	
//DHexDump(LOG_DEBUG, "DynPal", bmpDynamicPal.bitmap, 5*16, 4, 16, True);

	DrawBMP(bmpDynamicPal.bitmap, bmpDynamicPal.bWidth, bmpDynamicPal.bWidthPadded,
			bmpDynamicPal.height, bmpDynamicPal.height, *pBox);
	return True;
}


static short dynamicCode[] = { PPP_DYNAM,PP_DYNAM, P_DYNAM, MP_DYNAM, MF_DYNAM,
								F_DYNAM, FF_DYNAM, FFF_DYNAM, SF_DYNAM };
								
#define NDYNAMS (sizeof dynamicCode/sizeof(short))

#define NROWS 2		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?
#define NCOLS 5		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?

static Rect dynamicCell[NROWS*NCOLS];

static void InitDynamicCells(Rect *pBox);
static void InitDynamicCells(Rect *pBox)
{
	short cellWidth, cellHeight, cellNum;
	
	cellWidth = (pBox->right-pBox->left)/NCOLS;
	cellHeight = (pBox->bottom-pBox->top)/NROWS;
//LogPrintf(LOG_DEBUG, "InitDynamicCells: cellWidth=%d cellHeight==%d\n", cellWidth, cellHeight);

	for (short rn = 0; rn<NROWS; rn++)
		for (short cn = 0; cn<NCOLS; cn++) {
			cellNum = (NCOLS*rn) + cn;
			dynamicCell[cellNum].top = cellHeight*rn;
			dynamicCell[cellNum].left = cellWidth*cn;
			dynamicCell[cellNum].bottom = dynamicCell[cellNum].top+cellHeight;
			dynamicCell[cellNum].right = dynamicCell[cellNum].left+cellWidth;
//LogPrintf(LOG_DEBUG, "InitDynamicCells: rn=%d cn=%d cellNum=%d dynamicCell tlbr=%d,%d,%d,%d\n",
//rn, cn, cellNum, dynamicCell[cellNum].top, dynamicCell[cellNum].left,
//dynamicCell[cellNum].bottom, dynamicCell[cellNum].right);
		}
}


/* Return the number of the cell in the dynamic palette containing the given point. We
don't check whether cell is a valid selection; it's up to the calling routine to do that. */
 
static short FindDynamicCell(Point where, Rect *pBox);
static short FindDynamicCell(Point where, Rect *pBox)
{
	short xInBox, yInBox, cellNum, tryCell;
	
	xInBox = where.h - pBox->left;
	yInBox = where.v - pBox->top;
	cellNum = -1;
	for (short rn = 0; rn<NROWS; rn++)
		for (short cn = 0; cn<NCOLS; cn++) {
			tryCell = (NCOLS*rn) + cn;
			if (yInBox<dynamicCell[tryCell].top) continue;
			if (xInBox<dynamicCell[tryCell].left) continue;
			if (yInBox>=dynamicCell[tryCell].bottom) continue;
			if (xInBox>=dynamicCell[tryCell].right) continue;
			cellNum = tryCell;  break;
		}

//LogPrintf(LOG_DEBUG, "FindDynamicCell: xInBox=%d yInBox=%d cellNum=%d\n", xInBox, yInBox, cellNum);
	if (cellNum<0) {
		LogPrintf(LOG_WARNING, "Can't find cell for where=(%d,%d) xInBox=%d yInBox=%d.  (FindDynamicCell)\n",
					where.h, where.v, xInBox, yInBox);
		cellNum = 1;
	}
	
	return cellNum;
}


static void HiliteDynCell(short dynIdx, Rect *box);
static void HiliteDynCell(short dynIdx, Rect *box)
{
	Rect theCell = dynamicCell[dynIdx];
	OffsetRect(&theCell, box->left, box->top);
	InsetRect(&theCell, 1, 1);
	InvertRect(&theCell);
}


/* Return the index into our palette for the given character. If the character isn't in
the palette, return -1. */

static short DynamicKey(unsigned char theChar);
static short DynamicKey(unsigned char theChar)
{
	short newDynIdx=-1, intChar, sym;
		
	/* First remap theChar according to the 'PLMP' resource. */
	
	intChar = (short)theChar;
	TranslatePalChar(&intChar, 0, False);
	theChar = (unsigned char)intChar;

	sym = GetSymTableIndex(theChar);
	if (symtable[sym].objtype!=DYNAMtype) return -1;
	for (unsigned short k = 0; k<NDYNAMS; k++)
		if (symtable[sym].subtype==dynamicCode[k]) { newDynIdx = k;  break; }
	
	return newDynIdx;
}


#define SWITCH_HILITE(curIdx, newIdx, pBox)		HiliteDynCell((curIdx), (pBox)); curIdx = (newIdx); HiliteDynCell((curIdx), (pBox))

static pascal Boolean DynamicFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static pascal Boolean DynamicFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
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
				GetDialogItem(dlog, DYNAM_CHOICES_DI, &type, &hndl, &box);
//LogPrintf(LOG_DEBUG, "DynamicFilter: box tlbr=%d,%d,%d,%d\n", box.top, box.left, box.bottom,
//box.right);
				FrameRect(&box);
				DrawDynamicPalette(&box);
				HiliteDynCell(dynamicIdx, &box);
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
			GetDialogItem(dlog, DYNAM_CHOICES_DI, &type, &hndl, &box);
			if (PtInRect(where, &box)) {
				if (evt->what==mouseUp) {
					/* If the mouseUp was in an invalid (presumably because empty) cell,
					   ignore it. Otherwise, unhilite the previously-selected cell and
					   hilite the new one. */

					short newDynIdx = FindDynamicCell(where, &box);
					if (newDynIdx<0 || newDynIdx>(short)NDYNAMS-1) return False;
					SWITCH_HILITE(dynamicIdx, newDynIdx, &box);
				}
				*itemHit = DYNAM_CHOICES_DI;
				return True;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False)) return True;
			ch = (unsigned char)evt->message;
			GetDialogItem(dlog, DYNAM_CHOICES_DI, &type, &hndl, &box);
			*itemHit = 0;
			ans = DynamicKey(ch);
			if (ans>=0) {
LogPrintf(LOG_DEBUG, "ch='%c' dynamicIdx=%d ans=%d\n", ch, dynamicIdx, ans);  
				SWITCH_HILITE(dynamicIdx, ans, &box);
				*itemHit = DYNAM_CHOICES_DI;
				return True;
			}
	}
	
	return False;
}

Boolean SetDynamicDialog(SignedByte *dynamicType)
{	
	DialogPtr dlog;
	short ditem=Cancel, type, oldResFile;
	Boolean dialogOver;
	Handle hndl;
	Rect box, theCell;
	GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(DynamicFilter);
	if (filterUPP == NULL) {
		MissingDialog(SETDYN_DLOG);
		return False;
	}
	
	dlog = GetNewDialog(SETDYN_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SETDYN_DLOG);
		return False;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);							/* popup code uses Get1Resource */

	GetDialogItem(dlog, DYNAM_CHOICES_DI, &type, &hndl, &box);
	InitDynamicCells(&box);
	
	/* Find and hilite the initially-selected cell. We should always find it, but if
	   we can't, make an arbitrary choice. */
	
	dynamicIdx = 3;									/* In case we can't find it */
	for (unsigned short k = 0; k<NDYNAMS; k++) {
		if (*dynamicType==dynamicCode[k]) { dynamicIdx = k;  break; }
	}
	
	theCell = dynamicCell[dynamicIdx];
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
		*dynamicType = dynamicCode[dynamicIdx];
		
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}
