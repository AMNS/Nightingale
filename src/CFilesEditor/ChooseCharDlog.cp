/* ChooseCharDlog.c for Nightingale, by John Gibson */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "ChooseCharLDEF.h"

/* No one point size is good for both music fonts and normal text fonts. 24 is a nice
size for Sonata and similar fonts, whether compatible or not. */
#define MIN_FONTSIZE 	14		/* still too small for music fonts! */
#define MAX_FONTSIZE 	48		/* too big for non-music fonts? too small for 72pt Seville, etc.*/
#define MIN_CELLWIDTH	32		/* pixels */
#define MIN_CELLHEIGHT	32

#define USE_CUSTOMLIST

/* dialog item numbers */
#define LIST3		3
#define LASTITEM	LIST3

static ListHandle charListHndl;
static Rect	charListBounds;
static short numColumns, numRows;
static short lastChoice;

/* local prototypes */
static pascal Boolean ChooseCharFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static Boolean BuildList(DialogPtr dlog);


/* Display the Choose Char modal dialog.  Return True if OK, False if Cancel or error. */

Boolean ChooseCharDlog(short fontNum, short fontSize, unsigned char *theChar)
{
	short			itemHit, type;
	Boolean			okay, keepGoing=True;
	Cell			aCell;
	register DialogPtr dlog;
	Handle			hndl;
	Rect			box;
	GrafPtr			oldPort;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(ChooseCharFilter);
	if (filterUPP == NULL) {
		MissingDialog(CHOOSECHAR_DLOG);
		return False;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(CHOOSECHAR_DLOG, NULL, BRING_TO_FRONT);
	if (dlog == NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(CHOOSECHAR_DLOG);
		return False;
	}
	SetPort(GetDialogWindowPort(dlog));

	lastChoice = *theChar;

	if (fontSize<MIN_FONTSIZE) fontSize = MIN_FONTSIZE;
	if (fontSize>MAX_FONTSIZE) fontSize = MAX_FONTSIZE;

	/* Only use sizes that are installed, so we don't get ugly scaling. (All sizes of
		TrueType and, I assume, ATM fonts are "real" fonts.) */
	if (!RealFont(fontNum, fontSize)) {
		register short i;
		Boolean gotIt=False;
		for (i=fontSize; i<=MAX_FONTSIZE; i++) {			/* look for a larger installed size */
			if (RealFont(fontNum, i)) {
				gotIt = True;
				fontSize = i;
				break;
			}
		}
		if (!gotIt) {
			for (i=fontSize; i>=MIN_FONTSIZE; i--) {		/* look for a smaller installed size */
				if (RealFont(fontNum, i)) {
					fontSize = i;
					break;
				}
			}
		}
		/* else use scaled font */
	}

	/* Set font for list */
	TextFont(fontNum);
	TextSize(fontSize);
	
	if (!BuildList(dlog)) { okay = False; goto broken; }
	
	CenterWindow(GetDialogWindow(dlog), 0);
	ShowWindow(GetDialogWindow(dlog));

	/* Entertain filtered user events until dialog is dismissed */	
	while (keepGoing) {
		ModalDialog(filterUPP, &itemHit);
		if (itemHit<1 || itemHit>=LASTITEM) continue;
		GetDialogItem(dlog, itemHit, &type, &hndl, &box);
		switch (itemHit) {
			case OK:
				keepGoing = False; okay = True;
				break;
			case Cancel:
				keepGoing = False;
				break;
			case LIST3:					/* handled in filter */
				break;
		}
	}
	
	okay = (itemHit==OK);
	if (okay) {
		aCell.h = aCell.v = 0;
		LGetSelect(True, &aCell, charListHndl);
		*theChar = (aCell.v * numColumns) + aCell.h;
		lastChoice = *theChar;
	}

	LDispose(charListHndl);
broken:							/* Error return */
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);	
	return (okay);
}


static pascal Boolean ChooseCharFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Boolean			ans=False, doHilite=False;
	WindowPtr		w;
	short			ch, listFont, listFontSize, sysFontSize;
	Cell			aCell;
	Point			clickPt;
	GrafPtr			oldPort;
	unsigned char	oldCellCh;
	
	w = (WindowPtr)(evt->message);
	switch(evt->what) {
		case updateEvt:
			if (w == GetDialogWindow(dlog)) {
				GetPort(&oldPort);  SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));
				
				/* Since the dlog port's font is the one used in the list, we temporarily
					switch to the system font for updating the dlog's buttons. */
				 
				listFont = GetPortTxFont();
				listFontSize = GetPortTxSize();
				sysFontSize = GetDefFontSize();
				if (sysFontSize < 1) sysFontSize = 12;
				TextFont(systemFont);
				TextSize(sysFontSize);
				UpdateDialogVisRgn(dlog);
				TextFont(listFont);
				TextSize(listFontSize);
				FrameDefault(dlog, OK, True);
				FrameRect(&charListBounds);
				LUpdateDVisRgn(dlog, charListHndl);			
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				ans = True;  *itemHit = 0;
			}
			break;
		case activateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetDialogWindowPort(dlog));
				LActivate((evt->modifiers & activeFlag)!=0, charListHndl);
				*itemHit = 0;
			}
			break;
		case mouseDown:
			clickPt = evt->where;
			GlobalToLocal(&clickPt);
			/* if double-click, simulate pressing OK button */
			if (LClick(clickPt, evt->modifiers, charListHndl)) {
				*itemHit = OK;
				doHilite = True;
			}
			ans = doHilite;									/* False if not a double-click in list */
			break;
		case autoKey:										/* arrow keys need autoKey events */
		case keyDown:
			ch = (unsigned char)evt->message;
			if (ch==CH_CR || ch==CH_ENTER) {
				*itemHit = OK;
				doHilite = ans = True;
			}
			else if (ch=='.' && evt->modifiers & cmdKey) {
				*itemHit = Cancel;
				doHilite = True;
				ans = True;									/* Ignore other cmd-chars */
			}
			else {				
				aCell.h = aCell.v = 0;
				LGetSelect(True, &aCell, charListHndl);
				LSetSelect(False, aCell, charListHndl);
				oldCellCh = (aCell.v * numColumns) + aCell.h;
				switch (ch) {
					case LEFTARROWKEY:
						if (oldCellCh > 0)
							oldCellCh--;
						break;
					case RIGHTARROWKEY:
						if (oldCellCh < (numColumns*numRows)-1)
							oldCellCh++;
						break;
					case UPARROWKEY:
						if (oldCellCh > numColumns-1)
							oldCellCh -= numColumns;
						break;
					case DOWNARROWKEY:
						if (oldCellCh < numColumns*(numRows-1))
							oldCellCh += numColumns;
						break;
					default:								/* select the character typed */
						oldCellCh = ch;
						break;
				}
				aCell.h = oldCellCh % numColumns;
				aCell.v = oldCellCh / numColumns;
				LSetSelect(True, aCell, charListHndl);
				LAutoScroll(charListHndl);
				ans = True;
			}
			break;
	}
	if (doHilite)
		FlashButton(dlog, *itemHit);
		
	return ans;
}


/* Build a new list in the LIST3 user item box of dlog. The list displays 256
characters in the current font and size, using a custom LDEF, which draws dotted
gray cell borders and centers chars within cells. The size of the list depends on the
size of the font and some defined constants. The window is sized accordingly, and the
OK and Cancel buttons moved. */

static Boolean BuildList(DialogPtr dlog)
{
	short		i, j, type, scrnWid, scrnHt, cellWid, cellHt, 
				scrollBarWid, butWid, butHt, maxColumns, numRowsVis;
	Rect		userBox, limitRect, cancelBox, OKbox, lMarg, contentRect, dataBounds,
				portRect;
	Handle		hndl;
	unsigned char data[4];
	FontInfo	fontInfo;
	Cell		aCell;
	Point		cSize;

	/* Get screen size */
	limitRect = GetQDScreenBitsBounds();
	limitRect.top += GetMBarHeight();
	InsetRect(&limitRect, 30, 30);
	scrnWid = limitRect.right - limitRect.left;
	scrnHt = limitRect.bottom - limitRect.top;

	/* Get margins from user item rect. */
	GetDialogItem(dlog, LIST3, &type, &hndl, &userBox);
	lMarg = userBox;
	
	GetDialogPortBounds(dlog, &portRect);
	/* NB: right & bottom are dist. between list and closest edge of window */
	lMarg.right = portRect.right - userBox.right;
	lMarg.bottom = portRect.bottom - userBox.bottom;

	/* Calculate cell dimensions from current font characteristics */
	GetFontInfo(&fontInfo);
	cellWid = fontInfo.widMax;
	cellHt = fontInfo.ascent + fontInfo.descent + fontInfo.leading;
	cellWid = n_max(cellWid, MIN_CELLWIDTH);
	cellHt = n_max(cellHt, MIN_CELLHEIGHT);

	/* Determine how many cells can fit in a comfortably sized window */
	maxColumns = (scrnWid - (lMarg.left + lMarg.right)) / cellWid;
	numColumns = maxColumns>=16? 16 : (maxColumns>=8? 8 : 4);
	numRowsVis = (scrnHt - (lMarg.top + lMarg.bottom)) / cellHt;		/* number of rows visible at a time (without scrolling) */
	numRows = 256/numColumns;											/* total number of rows */
	if (numRowsVis>numRows) numRowsVis = numRows;
	scrollBarWid = numRowsVis==numRows? 0 : SCROLLBAR_WIDTH;			/* may not need scroll bar */
	
	/* Calculate size of list and dialog window */
	userBox.right = userBox.left + (cellWid*numColumns) + scrollBarWid;
	userBox.bottom = userBox.top + (cellHt*numRowsVis);

	/* set list bounding rect */
	charListBounds = userBox; InsetRect(&charListBounds, -1, -1);
	SetDialogItem(dlog, LIST3, userItem, NULL, &charListBounds);		/* so we get clicks in updated user item rect, I guess */

	/* Change window size */
	SizeWindow(GetDialogWindow(dlog), (userBox.right - userBox.left) + (lMarg.left + lMarg.right),
					(userBox.bottom - userBox.top) + (lMarg.top + lMarg.bottom), False);
	
	/* Move Cancel button */
	GetDialogItem(dlog, Cancel, &type, &hndl, &cancelBox);
	butWid = cancelBox.right - cancelBox.left;
	butHt = cancelBox.bottom - cancelBox.top;
	cancelBox.right = userBox.right;
	cancelBox.left = cancelBox.right - butWid;	
	GetDialogPortBounds(dlog, &portRect);
	cancelBox.bottom = portRect.bottom - 6;
	cancelBox.top = cancelBox.bottom - butHt;
	SetDialogItem(dlog, Cancel, type, hndl, &cancelBox);
	MoveControl((ControlHandle)hndl, cancelBox.left, cancelBox.top);	/* have to do this too */

	/* Move OK button */
	GetDialogItem(dlog, OK, &type, &hndl, &OKbox);
	butWid = OKbox.right - OKbox.left;
	butHt = OKbox.bottom - OKbox.top;
	OKbox.right = cancelBox.left - 10;
	OKbox.left = OKbox.right - butWid;
	OKbox.bottom = cancelBox.bottom;
	OKbox.top = OKbox.bottom - butHt;
	SetDialogItem(dlog, OK, type, hndl, &OKbox);
	MoveControl((ControlHandle)hndl, OKbox.left, OKbox.top);

	/* Content area (plus scroll bar) of list corresponds to modified user item box */
	contentRect = userBox; contentRect.right = userBox.right - scrollBarWid;

	SetRect(&dataBounds, 0, 0, numColumns, numRows);		/* for 256 chars */
	cSize.h = cellWid;	cSize.v = cellHt;
	
#ifdef USE_CUSTOMLIST
	ListDefSpec		defSpec;
	
	defSpec.defType = kListDefProcPtr;
	defSpec.u.userProc = NewListDefUPP( MyLDEFproc );

	// Create a custom list
	WindowPtr w = GetDialogWindow(dlog);
	CreateCustomList(&contentRect, &dataBounds, cSize, &defSpec, w, False,
						False, False, (scrollBarWid==0? False : True), &charListHndl);
#else	
	charListHndl = LNew(&contentRect, &dataBounds, cSize, CHOOSECHAR_LDEF,
							GetDialogWindow(dlog), False, False, False, scrollBarWid==0?
							 False : True);
#endif
	if (charListHndl) {
		(*charListHndl)->selFlags = lOnlyOne;
		for (i=0; i<numRows; i++) {
			for (j=0; j<numColumns; j++) {
				aCell.h = j;  aCell.v = i;
				*data = (unsigned char)((i*numColumns)+j);	/* an ASCII value */
				LSetCell(data, 1, aCell, charListHndl);
			}
		}
		aCell.h = lastChoice % numColumns;
		aCell.v = lastChoice / numColumns;
		LSetSelect(True, aCell, charListHndl);				/* select default (or sticky) cell */
		LAutoScroll(charListHndl);							/* and scroll to it */
		LSetDrawingMode(True, charListHndl);
	}
	
	return (charListHndl!=NULL);
}
