/* DynamicEdit.c for Nightingale - formerly DynamicPopUp, for use with accompanying graphic
MDEF; it now uses a BMP displayed in its entirety.  */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016-21 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
/* Includes code for managing key presses that select from a popup menu of dynamics.
This code parallels that for duration popups in DurationPopUp.c, and (especially)
for modifier popups in ModNRPopUp.c. Also in this file are dialog routines invoked
when user double-clicks a dynamic.                      -- John Gibson, 8/5/00 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* -------------------------------------------------------------------------------------- */
/* SetDynamicDialog */

enum {
	DYNAM_CHOICES_DI=4
};

static short dynamicIdx = 3;		// ??INITIALIZE IT TO WHAT REALLY?

#define DYNAMIC_PALETTE_FN		"ChangeDynamicNB1b.bmp"

#define BITMAP_SPACE 30000

Byte bitmapPalette[BITMAP_SPACE];

static Boolean DrawDynamicPalette(Rect *pBox);
static Boolean DrawDynamicPalette(Rect *pBox)
{
	short nRead, width, bWidth, bWidthPadded, height;
	FILE *bmpf;
	long pixOffset, nBytesToRead;

	/* Open the BMP file and read the actual bitmap image. */

	bmpf = NOpenBMPFile(DYNAMIC_PALETTE_FN, &pixOffset, &width, &bWidth, &bWidthPadded,
						&height);
	if (!bmpf) {
		LogPrintf(LOG_ERR, "Can't open bitmap image file '%s'.  (DrawDynamicPalette)\n",
					DYNAMIC_PALETTE_FN);
		return False;
	}
	if (fseek(bmpf, pixOffset, SEEK_SET)!=0) {
		LogPrintf(LOG_ERR, "fseek to offset %ld in bitmap image file '%s' failed.  (DrawDynamicPalette)\n",
					pixOffset, DYNAMIC_PALETTE_FN);
		return False;
	}

	nBytesToRead = bWidthPadded*height;
LogPrintf(LOG_DEBUG, "DrawDynamicPalette: bWidth=%d bWidthPadded=%d height=%d nBytesToRead=%d\n",
bWidth, bWidthPadded, height, nBytesToRead);
	if (nBytesToRead>BITMAP_SPACE) {
		LogPrintf(LOG_ERR, "Bitmap needs %ld bytes but Nightingale allocated only %ld bytes.  (DrawDynamicPalette)\n",
					nBytesToRead, BITMAP_SPACE);
		return False;
	}
	nRead = fread(bitmapPalette, nBytesToRead, 1, bmpf);
	if (nRead!=1) {
		LogPrintf(LOG_ERR, "Couldn't read the bitmap from image file.  (DrawDynamicPalette)\n");
		return False;
	}
	
DHexDump(LOG_DEBUG, "DynPal", bitmapPalette, 8*16, 4, 16, True);

	DrawBMP(bitmapPalette, bWidth, bWidthPadded, height, *pBox);
	return True;
}


#define NROWS 2		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?
#define NCOLS 5		// ??NOT THE BEST WAY TO DO THIS. Is it worth doing better?

static Rect dynamicCell[NROWS*NCOLS];

static void InitDynamicCells(Rect *pBox);
static void InitDynamicCells(Rect *pBox)
{
	short cellWidth, cellHeight, cellNum;
	
	cellWidth = (pBox->right-pBox->left)/NCOLS;
	cellHeight = (pBox->bottom-pBox->top)/NROWS;
LogPrintf(LOG_DEBUG, "InitDynamicCells: cellWidth=%d cellHeight==%d\n", cellWidth, cellHeight);

	for (short rn = 0; rn<NROWS; rn++)
		for (short cn = 0; cn<NCOLS; cn++) {
			cellNum = (NCOLS*rn) + cn;
			dynamicCell[cellNum].top = cellHeight*rn;
			dynamicCell[cellNum].left = cellWidth*cn;
			dynamicCell[cellNum].bottom = dynamicCell[cellNum].top+cellHeight;
			dynamicCell[cellNum].right = dynamicCell[cellNum].left+cellWidth;
LogPrintf(LOG_DEBUG, "InitDynamicCells: rn=%d cn=%d cellNum=%d dynamicCell tlbr=%d,%d,%d,%d\n",
rn, cn, cellNum, dynamicCell[cellNum].top, dynamicCell[cellNum].left,
dynamicCell[cellNum].bottom, dynamicCell[cellNum].right);
		}
}

static short FindDynamicCell(Point where, Rect *pBox);
static short FindDynamicCell(Point where, Rect *pBox)
{
	short xInBox, yInBox, cellNum, tryCell;
	
	xInBox = where.h - pBox->left;
	yInBox = where.v - pBox->top;
#if 1
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
#else
	rowNum = (yInBox>20? 1 : 0);
	colNum = (xInBox>40? 3 : 2);
	cellNum = (5*rowNum) + colNum;
#endif

LogPrintf(LOG_DEBUG, "FindDynamicCell: xInBox=%d yInBox=%d cellNum=%d\n", xInBox, yInBox, cellNum);
	if (cellNum<0) {
		LogPrintf(LOG_WARNING, "Can't find cell for where=(%d,%d) xInBox=%d yInBox=%d.  (FindDynamicCell)\n",
					where.h, where.v, xInBox, yInBox);
		cellNum = 1;
	}
	return cellNum;
}


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
#if 13
				GetDialogItem(dlog, DYNAM_CHOICES_DI, &type, &hndl, &box);
LogPrintf(LOG_DEBUG, "DynamicFilter: box tlbr=%d,%d,%d,%d\n", box.top, box.left, box.bottom,
box.right);
				FrameRect(&box);
				DrawDynamicPalette(&box);
#else
				DrawGPopUp(curPop);		
				HiliteGPopUp(curPop, popUpHilited);
#endif
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
			Rect newCell;
			where = evt->where;
			GlobalToLocal(&where);
#if 13
			GetDialogItem(dlog, DYNAM_CHOICES_DI, &type, &hndl, &box);
			if (PtInRect(where, &box)) {
				if (evt->what==mouseUp) {
					/* Unhilite the previously-selected cell and hilite the new one. */
					dynamicIdx = FindDynamicCell(where, &box);
					newCell = dynamicCell[dynamicIdx];
					OffsetRect(&newCell, box.left, box.top);
					InvertRect(&newCell);
				}
				SysBeep(1);
#else
			if (PtInRect(where, &curPop->box)) {
				DoGPopUp(curPop);
#endif
				*itemHit = DYNAM_CHOICES_DI;
				return True;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, False)) return True;
			ch = (unsigned char)evt->message;
#if 13
			dynamicIdx = 7;			// ?????????????REALLY GET IT FROM THE CHAR. CODE
			ans = 1;				// ??????????????WHAT REALLY???
			*itemHit = ans? DYNAM_CHOICES_DI : 0;
			;						// ???????????????HIGHLIGHT WHAT?
#else
			ans = DynamicPopupKey(curPop, popKeysDynamic, ch);
			*itemHit = ans? DYNAM_CHOICES_DI : 0;
			HiliteGPopUp(curPop, True);
#endif
			return True;
			break;
	}
	
	return False;
}


Boolean SetDynamicDialog(SignedByte *dynamicType)
{	
	DialogPtr dlog;
	short ditem=Cancel, type, oldResFile;
	short choice;
	Boolean dialogOver;
	Handle hndl;  Rect box;
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
#if 13
	InitDynamicCells(&box);
#else
	if (!InitGPopUp(&dynamicPop, TOP_LEFT(box), DYNAMIC_MENU, 1)) goto broken;
	popKeysDynamic = InitDynamicPopupKey(&dynamicPop);
	if (popKeysDynamic==NULL) goto broken;
	curPop = &dynamicPop;
	
	choice = GetDynamicPopItem(curPop, popKeysDynamic, *dynamicType);
	if (choice==NOMATCH) choice = 1;
	SetGPopUpChoice(curPop, choice);
#endif

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
#if 13
		*dynamicType = dynamicIdx;					// ???????????????????????????WHAT??
#else
		*dynamicType = popKeysDynamic[curPop->currentChoice].dynamicType;
#endif
		
broken:
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}

