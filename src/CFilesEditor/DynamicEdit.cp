/* DynamicEdit.c for Nightingale - formerly DynamicPopUp, for use with accompanying graphic
MDEF. ??NEED TO USE A BMP, PROBABLY NON-POPUP! */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
/* Includes code for managing key presses that select from a popup menu of dynamics.
This code parallels that for duration popups in DurationPopUp.c, and (especially)
for modifier popups in ModNRPopUp.c. Also in this file are dialog routines invoked
when user double-clicks a dynamic.                      -- John Gibson, 8/5/00 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "FileUtils.h"				// ??TEMPORARY! ADD THIS TO Nightingale_Prefix.pch!


/* -------------------------------------------------------------------------------------- */
/* SetDynamicDialog */

static enum {
	DYNAM_CHOICES_DI=4
} E_SetDynItems;

static short dynamicIdx = 3;		// ??INITIALIZE IT TO WHAT REALLY?

#define DYNAMIC_PALETTE_FN		"ChangeDynamicNB1b.bmp"

#define BITMAP_SPACE 30000

Byte bitmapPalette[BITMAP_SPACE];

static Boolean DrawDynamicPalette(Rect *pBox);
static Boolean DrawDynamicPalette(Rect *pBox)
{
	short defaultDynamic, nRead, width, bWidth, bWidthWithPad, height;
	FILE *bmpf;
	long pixOffset, nBytesToRead;

	/* Open the BMP file and read the actual bitmap image. */

	bmpf = NOpenBMPFile(DYNAMIC_PALETTE_FN, &pixOffset, &width, &bWidth, &bWidthWithPad,
						&height);
	if (!bmpf) {
		LogPrintf(LOG_ERR, "Can't open bitmap image file '%s'.  (DrawDynamicPalette)\n",
					DYNAMIC_PALETTE_FN);
		return False;
	}
	if (fseek(bmpf, pixOffset, SEEK_SET)!=0) {
		LogPrintf(LOG_ERR, "fseek to offset %ld in bitmap image file failed.  (DrawDynamicPalette)\n",
					pixOffset);
		return False;
	}

	nBytesToRead = bWidthWithPad*height;
	LogPrintf(LOG_DEBUG, "bWidth=%d bWidthWithPad=%d height=%d nBytesToRead=%d  (DrawDynamicPalette)\n",
		bWidth, bWidthWithPad, height, nBytesToRead);
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
	short startLoc;
	for (short nRow = 12; nRow>=0; nRow--) {
		startLoc = nRow*bWidthWithPad;
//printf("nRow=%d startLoc=%d\n", nRow, startLoc);
		DPrintRow(bitmapPalette, nRow, bWidth, startLoc, False, False);
		printf("\n");
	}
	DrawBMP(bitmapPalette, bWidth, bWidthWithPad, height, *pBox);
	return True;
}


static short FindDynamicCell(Point where, Rect *pBox);
static short FindDynamicCell(Point where, Rect *pBox)
{
	short xBox, yBox, rowNum, colNum, cellNum;
	
	xBox = where.h - pBox->left;
	yBox = where.v - pBox->top;
	rowNum = (yBox>20? 1 : 0);
	colNum = (xBox>40? 3 : 2);
	cellNum = (5*rowNum) + colNum;
LogPrintf(LOG_DEBUG, "xBox=%d yBox=%d cellNum=%d\n", xBox, yBox, cellNum);
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
			where = evt->where;
			GlobalToLocal(&where);
#if 13
			GetDialogItem(dlog, DYNAM_CHOICES_DI, &type, &hndl, &box);
			if (PtInRect(where, &box)) {
				dynamicIdx = FindDynamicCell(where, &box);
				//		??HILIGHT THE CELL!
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

