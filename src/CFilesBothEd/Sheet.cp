/* Sheet.c for Nightingale - slight rev. for v.3.1 */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean GetVisibleSheet(Document *doc, INT16 *i);

/* Scroll the sheet world so that a given sheet is visible in upper left.
??DOES NOTHING if the sheet is not in the visible array! */

void GotoSheet(Document *doc, INT16 nSheet, INT16 drawIt)
	{
		Rect sheet; INT16 dx,dy; short status;
		
		status = GetSheetRect(doc,nSheet,&sheet);
		if (status==INARRAY_INRANGE || status==INARRAY_OVERFLOW) {
			dx = sheet.left - doc->origin.h - config.hPageSep;
			dy = sheet.top  - doc->origin.v - config.vPageSep;
			QuickScroll(doc,dx,dy,TRUE,drawIt);
			}
	}


#ifdef OVERVIEW_MAYBE_SOME_DAY

/* Temporarily change the size of a paper to 1/10 the usual size, so
that user can see lots of sheets and click on one to go to it.  If
we're already in overview mode, go back to where we were.  If fromMenu
is TRUE, then this call is due to the user choosing Goto from menu,
otherwise, it's an internal call.  If a sheet was picked, then i has
its number in it, and fromMenu will be FALSE.  If fromMenu is TRUE,
i is undefined. */

void PickSheetMode(Document *doc, Boolean fromMenu, INT16 i)
	{
		WindowPtr w = doc->theWindow; Rect sheet;
		double scale = 0.1; INT16 x,y;

		/* Fill view with background pattern before redrawing sheets */
		
		ClipRect(&w->portRect);
		ForeColor(blueColor);
		FillRect(&doc->viewRect,paperBack);
		ForeColor(blackColor);
		
		/* Change modes and setup accordingly */
		
		doc->overview = !doc->overview;
		if (doc->overview) {
			/* Change to a scaled paper size, and scroll to upper left of array */
			doc->holdPaperRect = doc->paperRect;
			doc->holdOrigin = doc->origin;
			SetRect(&doc->paperRect,0,0,(short)(doc->paperRect.right*scale),
										(short)(doc->paperRect.bottom*scale));
			/* Don't let it get less than an inch, but maintain aspect ratio */
			if (doc->paperRect.right < doc->paperRect.bottom) {
				if (doc->paperRect.right < POINTSPERIN) {
					scale = (double)doc->paperRect.bottom / (double)doc->paperRect.right;
					doc->paperRect.right = POINTSPERIN;
					doc->paperRect.bottom = POINTSPERIN*scale;
					}
				}
			 else {
				if (doc->paperRect.bottom < POINTSPERIN) {
					scale = (double)doc->paperRect.right / (double)doc->paperRect.bottom;
					doc->paperRect.bottom = POINTSPERIN;
					doc->paperRect.right = POINTSPERIN * scale;
					}
				}
			x = doc->sheetOrigin.h - config.hPageSep;
			y = doc->sheetOrigin.v - config.vPageSep;
			GetAllSheets(doc);
			RecomputeView(doc);
			}
		 else {
			/* Restore old paper size and either scroll to new or old position */
			doc->paperRect = doc->holdPaperRect;
			GetAllSheets(doc);
			RecomputeView(doc);
			
			if (fromMenu) {
				/* Getting out of pick mode without picking: go back to where we were */
				x = doc->holdOrigin.h;
				y = doc->holdOrigin.v;
				}
			 else {
			 	/* Scroll to upper left corner of sheet user picked */
				GetSheetRect(doc,i,&sheet);			/* Always within the array */
				x = sheet.left - config.hPageSep;
				y = sheet.top  - config.vPageSep;
				}
			}
		
		QuickScroll(doc,x,y,FALSE,FALSE);			/* Internal scroll without update */
		InvalWindowRect(doc->theWindow,&doc->viewRect);						/* Redraw everything */
	}
	
#endif


/* ----------------------------------- Screen Page Overflow/Exceed View functions -- */

/* If it had the given number of pages, would it be possible for pages of the given
document, in its current screen page layout, to overflow screen coordinate space at
the given magnification? If so, return TRUE, else FALSE. */

Boolean ScreenPagesCanOverflow(Document *doc, INT16 magnify, INT16 numSheets)
{
	INT16 maxRows, maxCols;
	double scale;
	Boolean overflow;
	
	/*
	 * If we're not really magnifying, we assume there can't be a problem: this
	 * is because the user shouldn't have been allowed to set up a layout that
	 * overflows without magnifying in the first place.
	 */
	if (magnify<=0) return FALSE;
	
	/*
	 * There's a problem if the no. of rows in the layout and the actual no. of
	 * pages in the score each exceed the vertical coordinate space. Likewise
	 * for columns and the horizontal coordinate space.
	 */
	GetMaxRowCol(doc, doc->origPaperRect.right, doc->origPaperRect.bottom,
					  &maxRows, &maxCols);
	scale = UseMagnifiedSize(100, magnify)/100.0;
	maxRows /= scale;
	maxCols /= scale;

	overflow =  (doc->numRows>maxRows && numSheets>maxRows);
	overflow |=  (doc->numCols>maxCols && numSheets>maxCols);
	return overflow;
}


/* Give user an "alert" warning that the current score, Screen Page Layout, and
magnification overflow the screen coordinate space. We actually use a dialog
instead of an alert to facilitate handling a "don't warn again" button. */

static enum {
	BUT1_OK=1,
	DONT_WARN_AGAIN_DI=5
} E_SCREENOVERFLOW;

void WarnScreenPagesOverflow(Document */*doc*/)		/* doc is unused */
{
	static Boolean warnAgain=TRUE;
	DialogPtr dlog;
	Boolean keepGoing=TRUE;
	short itemHit;
	GrafPtr savePort;
	ModalFilterUPP	filterUPP;

	if (!warnAgain) return;

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP==NULL) {
		MissingDialog(SCREENOVERFLOW_DLOG);
		return;
	}
	GetPort(&savePort);
	dlog = GetNewDialog(SCREENOVERFLOW_DLOG,NULL,BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SCREENOVERFLOW_DLOG);
		return;
	}
	SetPort(GetDialogWindowPort(dlog));

	PlaceWindow(GetDialogWindow(dlog),(WindowPtr)NULL,0,70);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		switch (itemHit) {
			case BUT1_OK:
				keepGoing = FALSE;
				warnAgain = !GetDlgChkRadio(dlog, DONT_WARN_AGAIN_DI);
				break;
			case DONT_WARN_AGAIN_DI:
				PutDlgChkRadio(dlog, DONT_WARN_AGAIN_DI, 
									!GetDlgChkRadio(dlog, DONT_WARN_AGAIN_DI));
				break;
			default:
				;
		}
	}
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(savePort);
}


/* Are there more pages of the given document, in its current screen page
layout, than can be reached by scrolling, i.e., more than fit in the current
visible array? If so, return TRUE, else FALSE. This is not very serious,
but users might like to be informed anyway. */

Boolean ScreenPagesExceedView(Document *doc)
{
	return (doc->numSheets>doc->numRows*doc->numCols);
}


/* ----------------------------------------------------------------- SheetMagnify -- */
/* Change the viewing magnification for a document.  The magnification is done so
that the point scaleCenter (which should be the focus of the user's attention:
normally where they last clicked, but sometimes elsewhere--e.g., where the last-
inserted symbol would up after auto-respacing) ends up in a reasonable place.
Specifically, when reducing, we put that point in the same position in the window
as it was prior to the magnification; when enlarging, we put that point halfway
between its pre-magnification position and the center of the window. In both cases,
we do this modulo scroll bar minima/maxima. */

Boolean magnifyOnly;				/* Secret message to RecomputeView */

void SheetMagnify(Document *doc,
						INT16 inc)			/* Increment for <doc->magnify> */
	{
		INT16 x,y,sheet,xMid,yMid; Point saveCenter; Rect paper,r,portRect;
		double xAcross,yDown,xSave,ySave; INT16 wAcross,wDown;
		WindowPtr w = doc->theWindow;
		
		if (ScreenPagesCanOverflow(doc, doc->magnify+inc, doc->numSheets)) {
			GetIndCString(strBuf, MISCERRS_STRS, 24);				/* "The screen will stay at the old magnification" */
			CParamText(strBuf, "", "", "");
			WarnScreenPagesOverflow(doc);
			return;
		}

		if (FindSheet(doc,doc->scaleCenter,&sheet)) {
			/*
			 *	When zooming in, keeping scaleCenter fixed could result in nearby
			 *	material going out of view. To avoid this during zooming in, we
			 *	first move the scale center half of the way from its current position
			 *	away from the center of the window so that dilating about it forces
			 *	the point it used to be towards the center. When zooming out, we
			 *	simply move the scale center to the middle of the window's view.
			 */
			r = doc->viewRect;
			xMid = r.left + (r.right-r.left)/2;
			yMid = r.top  + (r.bottom-r.top)/2;
			/* Keep position of old scaleCenter before mucking with it */
			saveCenter = doc->scaleCenter;
			
			if (inc > 0) {
				doc->scaleCenter.h -= (xMid - doc->scaleCenter.h) / 2;
				doc->scaleCenter.v -= (yMid - doc->scaleCenter.v) / 2;
				}
			 else {
				doc->scaleCenter.h = xMid;
				doc->scaleCenter.v = yMid;
				}
			
			/* Get percentages of the way across and down scaleCenter is now */
			GetSheetRect(doc,sheet,&paper);
			xAcross = (double)(doc->scaleCenter.h - paper.left) / (double)(paper.right - paper.left);
			yDown   = (double)(doc->scaleCenter.v - paper.top)  / (double)(paper.bottom - paper.top);
			xSave   = (double)(saveCenter.h  - paper.left) / (double)(paper.right - paper.left);
			ySave   = (double)(saveCenter.v  - paper.top)  / (double)(paper.bottom - paper.top);
			/* Get window coord offset from upper left corner */
			wAcross = doc->scaleCenter.h - doc->origin.h;
			wDown   = doc->scaleCenter.v - doc->origin.v;
			}
		 else {
			/* Click wasn't on a page: just use the page that's showing now */
			GetVisibleSheet(doc,&sheet);
			xAcross = 0.0;
			yDown = 0.0;
			wAcross = 0.0;
			wDown = 0.0;
			}
		
		MEHideCaret(doc);
		InvalRange(doc->headL, doc->tailL);

		/* Fill view with background pattern before redrawing sheets */
		
		GetWindowPortBounds(w,&portRect);
		ClipRect(&portRect);
		ForeColor(blueColor);
		FillRect(&doc->viewRect,&paperBack);
		ForeColor(blackColor);
		
		/* GetVisibleSheet(doc,&sheet); */
		doc->magnify += inc;
		
		/*
		 *	doc->origPaperRect always has the current paper size in paper-relative
		 *	pixels, non-magnified.  doc->paperRect has the current paper size in
		 *	paper-relative pixels, after magnification.  The same for marginRect.
		 */
		
		MagnifyPaper(&doc->origPaperRect,&doc->paperRect,doc->magnify);

		if (GetSheetRect(doc,sheet,&paper)==INARRAY_INRANGE) {	/* Have to call it again */
			/* Get point that wants to be placed at same position as old scaleCenter */
			x = paper.left + xSave * (paper.right-paper.left);
			y = paper.top  + ySave   * (paper.bottom-paper.top);
			doc->scaleCenter.h = x; doc->scaleCenter.v = y;
			/* Convert to point that wants to be new window origin */
			x = paper.left + xAcross * (paper.right-paper.left);
			y = paper.top  + yDown   * (paper.bottom-paper.top);
			x -= wAcross;
			y -= wDown;
			}
		 else {
			/* Go back to origin of sheet array (shouldn't happen) */
			x = doc->origin.h - 2 * config.hPageSep;
			y = doc->origin.v - 2 * config.vPageSep;
			}
		GetAllSheets(doc);
		magnifyOnly = TRUE;
		RecomputeView(doc);
		magnifyOnly = FALSE;
		QuickScroll(doc,x,y,FALSE,FALSE);		/* Internal scroll without update */
		DrawDocumentView(doc, &doc->viewRect);
		if (doc->selStartL==doc->selEndL) MEAdjustCaret(doc, TRUE);
	}


/* ------------------------------------------------------- Magnify/UnmagnifyPaper -- */
/* These functions assumes that magnify>=0 doesn't reduce, and that magnify<0
doesn't enlarge. If these assumptions are violated, underflow or overflow is
likely, respectively. They also assume that magnify>=0 scales by a value that's
a multiple of 1/2! NB: Using a power of 2 (say, 16384) as the value to be scaled
by UseMagnifiedSize should greatly reduce roundoff error for magnifications that
are powers of 2. */

/* Given a Rect, paper, in standard non-magnified coordinates (points), convert it to
a magnified (pixels) version in magPaper, using the magnification counter, magnify.
magPaper can be the same as paper. */

void MagnifyPaper(Rect *paper, Rect *magPaper, INT16 magnify)
	{
		double scale;
				
		if (magnify >= 0) scale = UseMagnifiedSize(2, magnify)/2.0;			/* Handle 150% */
		else					scale = UseMagnifiedSize(16384, magnify)/16384.0;

		SetRect(magPaper,
					(INT16)(paper->left * scale),
					(INT16)(paper->top * scale),
					(INT16)(paper->right * scale),
					(INT16)(paper->bottom * scale));
	}


/* Given a Rect, magPaper, in magnified coordinates (pixels), convert it to a
standard non-magnified (points) version in paper, using the magnification counter,
magnify. paper can be the same as magPaper. */

void UnmagnifyPaper(Rect *magPaper, Rect *paper, INT16 magnify)
	{
		double scale;
		
		if (magnify >= 0) scale = 2.0/UseMagnifiedSize(2, magnify);		/* Handle 150% */
		else					scale = 16384.0/UseMagnifiedSize(16384, magnify);

		SetRect(paper,
					(short)(magPaper->left*scale),
					(short)(magPaper->top*scale),
					(short)(magPaper->right*scale),
					(short)(magPaper->bottom*scale));
	}


#ifndef VIEWER_VERSION

/* Go directly to Master Page view, or back to current page */

void DoMasterSheet(Document *doc)
	{
		WindowPtr w = doc->theWindow;
		static INT16 oldCurrentSheet;
		Rect portRect;
		
		/* If all parts have been deleted, can't leave masterView. Call ExitMasterView
			to get user choice for discarding changes or staying inside masterView */

		if (doc->masterView && doc->nstavesMP<=0) {

			if (ExitMasterView(doc)) {

				doc->masterView = FALSE;
				doc->currentSheet = oldCurrentSheet;
				GotoSheet(doc,doc->currentSheet,FALSE);
	
				GetAllSheets(doc);
				RecomputeView(doc);
					
				GetWindowPortBounds(w,&portRect);
				ClipRect(&portRect);
				InvalWindowRect(w,&portRect);
	
				MEAdjustCaret(doc, FALSE);
				}
			return;
			}
		
		if (doc->masterView = !doc->masterView) {
			oldCurrentSheet = doc->currentSheet;
			doc->currentSheet = 0;
			GotoSheet(doc,0,FALSE);
			}
		 else {
			doc->currentSheet = oldCurrentSheet;
			GotoSheet(doc,doc->currentSheet,FALSE);
			}
		
		GetAllSheets(doc);
		RecomputeView(doc);
			
		GetWindowPortBounds(w,&portRect);
		ClipRect(&portRect);
		InvalWindowRect(w,&portRect);
		
		if (doc->masterView) {
			MEHideCaret(doc);
			EnterMasterView(doc);				/* Allocate memory, import values, etc. */
			}
		 else
			if (ExitMasterView(doc))			/* Dispose memory, export values, etc. */
				MEAdjustCaret(doc, FALSE);
				
		// CER 05.11.2003, 5_02a4 - Carbon version, window not being re-drawn
		
		EraseRect(&portRect);
		InvalWindowRect(w,&portRect);
		
		// 5_02a4
	}

#endif /* VIEWER_VERSION */


/* Given a point in window coordinates, deliver the sheet number of the sheet
that the point falls in and return TRUE; return FALSE if it's outside any
visible sheet.  Checks the current sheet first, since it's the most likely
to be tested. */

Boolean FindSheet(Document *doc, Point pt, INT16 *ans)
	{
		Rect paper; INT16 i;
		
		GetSheetRect(doc,doc->currentSheet,&paper);
		if (PtInRect(pt,&paper)) {
			*ans = doc->currentSheet;
			return(TRUE);
			}
		
		for (i=doc->firstSheet; i<doc->numSheets; i++)
			if (GetSheetRect(doc,i,&paper)==INARRAY_INRANGE) {
				if (PtInRect(pt,&paper)) {
					*ans = i;
					return(TRUE);
					}
				}
			 else
				break;
		
		return(FALSE);
	}

/* Set the current sheet to a given sheet number, which must be between 0
and numSheets-1.  The sheet's position in the window coordinate system is
provided as well so that various conversion routines can convert mouse
points to paper points quickly. */

static INT16 nestLevel = 0;

void SetCurrentSheet(Document */*doc*/, INT16 /*i*/, Rect */*paper*/)
	{

#if 1
		MayErrMsg("Shouldn't be calling SetCurrentSheet");		/* ??WHY? Is this for overview? */
#else		
		if (nestLevel++ == 0) {
			doc->currentSheet = i;
			doc->currentPaper = *paper;
			dx = doc->viewRect.left - paper->left;
			dy = doc->viewRect.top - paper->top;
			SetOrigin(dx,dy);
			dx -= doc->origin.h;
			dy -= doc->origin.v;
			
			OffsetRect(&(*(doc->theWindow)->clipRgn)->rgnBBox, dx, dy);
			}
#endif
	}


void SetBackground(Document *doc)
	{
		if (--nestLevel == 0) {
			SetOrigin(doc->origin.h,doc->origin.v);
			ClipRect(&doc->viewRect);
			}
	}


/* Given a sheet number and its document, return NOT_INARRAY if the sheet is
not in the current sheet array (and therefore not visible); INARRAY_OVERFLOW
if it's in the array but its coords. overflow the 16-bit limits of a Rect
(in which case it also can't be visible); and INARRAY_INRANGE if it's in the
array and its coords. don't overflow (in which case it MAY be visible). If
INARRAY_INRANGE, we also deliver the paperRect in absolute window coordinates
as it appears in the current sheet array, not including the page separation
space surrounding paper. NOT_INARRAY can happen only when there are too many
sheets to fit in the array. */

short GetSheetRect(Document *doc, INT16 nSheet, Rect *paper)
	{
		INT16 lastSheet,r,c,pWidth,pHeight;  long hPos,vPos;
		
		/* First return NOT_INARRAY if nSheet is not a currently showing sheet */
		
		if (nSheet<doc->firstSheet) return(NOT_INARRAY);
		lastSheet = doc->firstSheet + doc->numRows * doc->numCols;
		if (lastSheet > doc->numSheets) lastSheet = doc->numSheets;
		if (nSheet >= lastSheet) return(NOT_INARRAY);
		
		/* Compute the paper rect for it */
		
		nSheet -= doc->firstSheet;								/* Convert to array relative index */
		r = nSheet / doc->numCols;
		c = nSheet % doc->numCols;
		
		pWidth = config.hPageSep*2 + doc->paperRect.right - doc->paperRect.left;
		pHeight = config.vPageSep*2 + doc->paperRect.bottom - doc->paperRect.top;
		
		*paper = doc->paperRect;								/* Always (0,0) in upper left */
		if (outputTo!=toImageWriter && outputTo!=toPICT) {
			hPos = doc->sheetOrigin.h+(long)c*pWidth;
			vPos = doc->sheetOrigin.v+(long)r*pHeight;
			if (hPos<-32767L || hPos>32767L
			||  vPos<-32767L || vPos>32767L) {
				/* The sheet is in array but its coords. overflow the limits of a Rect. */
				return(INARRAY_OVERFLOW);
			}
			OffsetRect(paper,hPos,vPos);
		
			/* But don't include left and top page separation space */
			
			OffsetRect(paper,config.hPageSep,config.vPageSep);
		}
		
		return(INARRAY_INRANGE);
	}


/* Get the sheet number of the first sheet whose lower right corner is greater
than or equal (in both coordinates) to the current window origin. If one is
found, return TRUE. */

static Boolean GetVisibleSheet(Document *doc, INT16 *i)
	{
		INT16 pWidth,pHeight,r,c; Boolean ans = FALSE;
		
		pWidth = config.hPageSep*2 + doc->paperRect.right - doc->paperRect.left;
		pHeight = config.vPageSep*2 + doc->paperRect.bottom - doc->paperRect.top;
		
		c = ((long)doc->origin.h - (long)doc->sheetOrigin.h) / pWidth;
		r = ((long)doc->origin.v - (long)doc->sheetOrigin.v) / pHeight;
		
		*i = r*doc->numCols + c;
		*i += doc->firstSheet;
		
		ans = TRUE;
		return(ans);
	}


/* Compute the size of the bounding box of all sheets in given document. Sheets
extend from sheet 0 to sheet numSheets-1 in an array of numCols columns and numRows
rows.  Also computes the background region, which is the universe with a hole in it
for every sheet. */

void GetAllSheets(Document *doc)
	{
		INT16 r,c,w,h,i; Rect sheet;
		static RgnHandle sheetRgn; static Boolean once = TRUE;
		
		if (doc->masterView) {
			GetSheetRect(doc,doc->currentSheet,&doc->allSheets);
			doc->currentPaper = doc->allSheets;
			}
		 else {
		 
			w = config.hPageSep*2 + doc->paperRect.right  - doc->paperRect.left;
			h = config.vPageSep*2 + doc->paperRect.bottom - doc->paperRect.top;

			c = doc->numCols;
			r = 1 + (doc->numSheets-doc->firstSheet-1) / c;
			if (r <= 1) c = doc->numSheets-doc->firstSheet;
			 else if (r > doc->numRows) r = doc->numRows;
			
			SetRect(&doc->allSheets,0,0,c*w-2*config.hPageSep,r*h-2*config.vPageSep);
			OffsetRect(&doc->allSheets,
						doc->sheetOrigin.h+config.hPageSep,doc->sheetOrigin.v+config.vPageSep);
			}
		
		/* Now compute background region: all of space minus visible sheets */
		
		if (once) { sheetRgn = NewRgn(); once = FALSE; }
		
		w = 32000;
		SetRectRgn(doc->background,-w,-w,w,w);	/* Start with everything */
		if (doc->masterView) {
			GetSheetRect(doc,doc->currentSheet,&sheet);
			RectRgn(sheetRgn,&sheet);
			DiffRgn(doc->background,sheetRgn,doc->background);
			}
		 else
			for (i=doc->firstSheet; i<doc->numSheets; i++)
				if (GetSheetRect(doc,i,&sheet)==INARRAY_INRANGE) {
					RectRgn(sheetRgn,&sheet);
					DiffRgn(doc->background,sheetRgn,doc->background);
					}
				 else
					break;
	}


/* Draw the contents of the i'th sheet from the given document. It is this
routine's responsibility to draw the contents of the sheet in window-relative
coordinates. The background is already erased. The redrawing can be culled to
include only those objects that intersect the given rectangle, vis (a subset
of paper), which is always defined (in window coords). */

void DrawSheet(Document *doc, INT16 i, Rect *paper, Rect *vis)
	{
		char numStr[16]; INT16 pageNum; INT16 dx,dy,width,arraySize;
		Boolean pre=FALSE,post=FALSE; char *more = " ... ";

		pageNum = i + doc->firstPageNumber;
		sprintf(numStr, "%d", pageNum);
		
		width = CStringWidth(numStr);
		
		if (doc->overview) {
			/*
			 *	Prepare to place "..." before first page showing in array if there
			 *	are others before it not showing, and after last page showing
			 *	in array if there are others past it not showing.
			 */
			arraySize = doc->numRows * doc->numCols;
			pre = (i==doc->firstSheet && doc->firstSheet>0);
			post = (i==(doc->firstSheet+arraySize-1) && i!=(doc->numSheets-1));
			if (pre) width += CStringWidth(more);
			if (post) width += CStringWidth(more);
			}

		dx = doc->paperRect.left +
			(doc->paperRect.right-doc->paperRect.left - width)/2;
		if (numStr[1] == '1') dx -= 1;

		if (doc->overview) {
			/* Center page number in sheet */
			dy = doc->paperRect.top + (doc->paperRect.bottom-doc->paperRect.top +12)/2;
			MoveTo(paper->left,paper->top); Move(dx,dy);
			if (pre) DrawCString(more);
			/*	DrawCString(numStr); */
			if (post) DrawCString(more);
			/* Skip everything else */
			}
		 else {
			/* Print page number only if it should be shown */
			if (pageNum >= doc->startPageNumber) {
				MoveTo(paper->left,paper->top);
				Move(dx,doc->marginRect.bottom-3);
				/* DrawCString(numStr); */
				}

#ifdef DRAWMARGIN
			/* Show margin rectangle in this sheet */
			
			PenPat(gray);
			r = doc->marginRect; OffsetRect(&r,paper->left,paper->top);
			FrameRect(&doc-> );
			PenPat(black);
#endif
			DrawPageContent(doc,i,paper,vis);
			}
	}


/* Invalidate the content of the given range of sheets, forcing an update of
sheets firstSheet through lastSheet, inclusive. */

void InvalSheets(Document *doc, INT16 firstSheet, INT16 lastSheet)
	{
		INT16 i,once = TRUE;	/* ??<once> should be Boolean */
		Rect paper; GrafPtr oldPort;
		
		GetPort(&oldPort); SetPort(GetWindowPort(doc->theWindow));
		
		/* For all sheets in range and visible in array */
		
		for (i=firstSheet; i<=lastSheet && GetSheetRect(doc,i,&paper)==INARRAY_INRANGE;
				i++) {
			/* If at least some graphic activity, turn caret off first */
			if (once) { MEHideCaret(doc); once = FALSE; }
			EraseAndInval(&paper);
			}
		
		SetPort(oldPort);
	}


/* Whenever we want to erase the entire document in order to rebuild, we use this
routine, which paints the background pattern over everything rather than calling
EraseRect.  This reduces unsightly flash. */

void InvalSheetView(Document *doc)
	{
		GrafPtr oldPort;
		Rect portRect;
		
		GetPort(&oldPort); SetPort(GetWindowPort(doc->theWindow));
		
		ForeColor(blueColor);
		FillRect(&doc->viewRect,&paperBack);						/* Excludes scroll bars */
		ForeColor(blackColor);
		GetWindowPortBounds(doc->theWindow,&portRect);
		InvalWindowRect(doc->theWindow,&portRect);				/* Includes scroll bars */
		GetAllSheets(doc);
		RecomputeView(doc);
		
		SetPort(oldPort);
	}
