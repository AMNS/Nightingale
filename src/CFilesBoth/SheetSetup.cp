/* SheetSetup.c for Nightingale: support for a dialog that lets the user specify the
layout to be used for the "sheets" (pages) on the screen. */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Prototypes */

static Boolean AnyBadValues(DialogPtr dlog, Document *doc);
static void SetRowsCols(DialogPtr dlog, INT16 r, INT16 c);

/* Symbolic Dialog Item Numbers */

static enum {
	BUT1_OK = 1,
	BUT2_Cancel,
	RAD4_Single=4,
	RAD5_Horizontal,
	RAD6_Vertical,
	RAD7_Custom,
	EDIT8_Rows,
	EDIT10_Cols=10,
	LASTITEM
	} E_SheetSetupItems;

static INT16 group1;
static Boolean redraw;

static Point where;
static INT16 modifiers;


/* The public routine for invoking the Sheet Layout modal dialog. */

void DoSheetSetup(register Document *doc)
	{
		INT16 itemHit,type,okay=FALSE,keepGoing=TRUE,maxRows,maxCols;
		Handle hndl; Rect box;
		DialogPtr dlog; GrafPtr oldPort;
		ModalFilterUPP	filterUPP;

		filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP==NULL) {
			MissingDialog(sheetSetupID);
			return;
		}

		GetPort(&oldPort);

		dlog = GetNewDialog(sheetSetupID,NULL,BRING_TO_FRONT);
		if (dlog==NULL) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(sheetSetupID);
			return;
		}

		/* Center dialog with respect to main screen, near top */
		
		PlaceWindow(GetDialogWindow(dlog),(WindowPtr)NULL,0,40);
		SetPort(GetDialogWindowPort(dlog));

		redraw = FALSE;										/* Must be before any "goto broken;" */
		
		/* Fill in dialog's values here */
				
		if (doc->numRows==1 && doc->numCols==1) group1 = RAD4_Single;
		 else if (doc->numRows == 1) group1 = RAD5_Horizontal;
		 else if (doc->numCols == 1) group1 = RAD6_Vertical;
		 else						 group1 = RAD7_Custom;

		PutDlgChkRadio(dlog,RAD4_Single, group1==RAD4_Single);
		PutDlgChkRadio(dlog,RAD5_Horizontal, group1==RAD5_Horizontal);
		PutDlgChkRadio(dlog,RAD6_Vertical, group1==RAD6_Vertical);
		PutDlgChkRadio(dlog,RAD7_Custom, group1==RAD7_Custom);

		PutDlgWord(dlog,EDIT8_Rows,doc->numRows,FALSE);
		PutDlgWord(dlog,EDIT10_Cols,doc->numCols,group1==RAD7_Custom);
		
		ShowWindow(GetDialogWindow(dlog));

		while (keepGoing) {
			ModalDialog(filterUPP,&itemHit);
			if (itemHit<BUT1_OK || itemHit>LASTITEM) continue;
			GetDialogItem(dlog,itemHit,&type,&hndl,&box);
			switch(itemHit) {
				case BUT1_OK:
					if (keepGoing = AnyBadValues(dlog,doc))
						HiliteControl((ControlHandle)hndl,0);
					okay = TRUE;
					break;
				case BUT2_Cancel:
					keepGoing = okay = FALSE;
					break;
				case EDIT8_Rows:
				case EDIT10_Cols:
					/* Switch to "Custom" and drop thru to handle it. */
					itemHit = RAD7_Custom;									/* and drop thru... */
				case RAD4_Single:												/* Radio button group */
				case RAD5_Horizontal:
				case RAD6_Vertical:
				case RAD7_Custom:
					if (itemHit != group1) {
						SwitchRadio(dlog, &group1, itemHit);
						GetMaxRowCol(doc, doc->origPaperRect.right,
										  doc->origPaperRect.bottom,
										  &maxRows, &maxCols);
						if (group1 == RAD4_Single)
							SetRowsCols(dlog, 1, 1);
						 else if (group1 == RAD5_Horizontal)
							SetRowsCols(dlog, 1, maxCols);
						 else if (group1 == RAD6_Vertical)
							SetRowsCols(dlog, maxRows, 1);
						/* else leave rows and cols as is */
						}
					break;
				default:
					break;
				}
			}

broken:
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
		SetPort(oldPort);
		
		if (redraw) {
			InvalSheetView(doc);
			if (doc->selStartL==doc->selEndL)
				MEAdjustCaret(doc,FALSE);
		}
	}


/* Retrieve values from dialog items, check for any bad values, and deliver TRUE
if any are bad; FALSE if all are reasonable.  If all are reasonable, set the values
in the given document, etc. */

static Boolean AnyBadValues(DialogPtr dlog, register Document *doc)
	{
		INT16 numRows,numCols,maxCols,maxRows;
		long maxWidth = 32767L, maxHeight = 32767L;
		Rect paperRect;
		
		/* First check for empty items */
		
		paperRect = doc->origPaperRect;
		GetMaxRowCol(doc,paperRect.right-paperRect.left, paperRect.bottom-paperRect.top,
					 &maxRows, &maxCols);

		GetDlgWord(dlog,EDIT8_Rows,&numRows);
		if (numRows<1 || numRows>maxRows) {
			SelectDialogItemText(dlog,EDIT8_Rows,0,ENDTEXT);
			TooManyRows(maxRows);
			return(TRUE);
			}
	
		GetDlgWord(dlog,EDIT10_Cols,&numCols);
		if (numCols<1 || numCols>maxCols) {
			SelectDialogItemText(dlog,EDIT10_Cols,0,ENDTEXT);
			TooManyColumns(maxCols);
			return(TRUE);
			}
	
		if (doc->numRows!=numRows || doc->numCols!=numCols) {
			doc->numRows = numRows;
			doc->numCols = numCols;
			doc->changed = TRUE;
			redraw = TRUE;
			}
		
		return(FALSE);
	}


/* Figure maximum array size for given size page, using QuickDraw limits and
origin we're using. NB: Since we expect the paper size in pixels, the maximum
values returned are magnification-dependent! */

void GetMaxRowCol(
		Document *doc,
		INT16 width, INT16 height,				/* Paper size (pixels) */
		INT16 *maxRows, INT16 *maxCols		/* Output: maximum rows, maximum cols. */
		)
	{
		long w,h,maxWidth,maxHeight;
		
		w = config.hPageSep*2 + (long)width;
		h = config.vPageSep*2 + (long)height;

	 	maxWidth  = 30000L - (long)doc->sheetOrigin.h;			/* leaves about 4K slop */
	 	maxHeight = 30000L - (long)doc->sheetOrigin.v;
	 	
		*maxCols = maxWidth / w;
		if (*maxCols < 1) *maxCols = 1;
		 else if (*maxCols > config.maxCols) *maxCols = config.maxCols;
		*maxRows = maxHeight / h;
		if (*maxRows < 1) *maxRows = 1;
		 else if (*maxRows > config.maxRows) *maxRows = config.maxRows;
	}


/* Install a row and column pair into their respective edit items without disturbing
the others. */

static void SetRowsCols(DialogPtr dlog, INT16 r, INT16 c)
	{
		TextEditState(dlog,TRUE);
		PutDlgWord(dlog,EDIT8_Rows,r,FALSE);
		PutDlgWord(dlog,EDIT10_Cols,c,FALSE);
		TextEditState(dlog,FALSE);
	}

/* ------------------------------------------------------ XLoadSheetSetupSeg -- */
/* Null function to allow loading or unloading SheetSetup.c's segment. */

void XLoadSheetSetupSeg()
{
}
