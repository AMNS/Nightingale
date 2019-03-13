/* DrawDocument.c - draw the contents of the given document in its window */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Draw the scroll bars, message box/horizontal scroll bar boundary, and grow box for the
given document, gated by the global flag <alreadyDrawn>, which is used to skip the
update event after an activate event that redraws the controls. */

void DrawDocumentControls(Document *doc)
{
	Point pt;
	Rect portRect;
	
	if (!alreadyDrawn) {
		GetWindowPortBounds(doc->theWindow, &portRect);
		ClipRect(&portRect);
		pt = TOP_LEFT(doc->growRect);

		/* Draw in new grow box. NB: DrawGrowIcon draws the boundary lines as well! */

		DrawGrowIcon(doc->theWindow);
		DrawControls(doc->theWindow);
		ClipRect(&doc->viewRect);
		alreadyDrawn = False;
	}
}

/* Draw everything that should show in a Document window, clipped to the current value
of doc->viewRect if updateRect is NULL, or to updateRect if not. "Everything" means the
window background and the visible sheets of paper. Within each sheet, draw its contents. */

void DrawDocumentView(Document *doc, Rect *updateRect)
	{
		Rect updater, margin, paper, result;
		short i;
		
		if (doc->showWaitCurs) WaitCursor();
		doc->showWaitCurs = True;
		
		if (updateRect) {
			SectRect(&doc->viewRect,updateRect,&updater);
			ClipRect(&updater);
			}
		 else
			updater = doc->viewRect;
		
		/* Paint in around paper, but not where paper is, to avoid flashing */
		
		ForeColor(blueColor);
		FillRgn(doc->background,&paperBack);
		ForeColor(blackColor);
		
		/* Now redraw each sheet in the document, or master sheet */
		
		PenNormal();
		if (doc->masterView) {
			Rect r;
			/* Draw master sheet in current sheet's place */
			GetSheetRect(doc,doc->currentSheet,&r);
			EraseRect(&r);
			FrameRect(&r);

			/* Add lines on right and bottom to look like a tablet */
			for (i=0; i<7; i++)  {
				PenPat(((i&1)||i==6) ? NGetQDGlobalsBlack() : NGetQDGlobalsWhite());
				MoveTo(r.left+1+i,r.bottom+i);
				LineTo(r.right+i,r.bottom+i);
				LineTo(r.right+i,r.top+1+i);
				}

			/* Binding along top */
			MoveTo(r.left-1,r.top-2);
			LineTo(r.right-2,r.top-2);
			MoveTo(r.left-1,r.top-1);
			LineTo(r.right-1,r.top-1);
			/* And down back side on right */
			PenSize(1,3);
			LineTo(r.right+5,r.top+5);
			/* And same on opposite corner */
			PenSize(1,1);
			MoveTo(r.left,r.bottom-1);
			LineTo(r.left+6,r.bottom+5);
			/* And on lower right corner */
			MoveTo(r.right-1,r.bottom-1);
			LineTo(r.right+6,r.bottom+6);

			/* Draw master page. */
			DrawRange(doc, RightLINK(doc->masterHeadL), doc->masterTailL, &r, &doc->viewRect);
			
			/* Show margins */
			PenPat(NGetQDGlobalsGray());
			PenMode(patXor);
			margin = doc->marginRect;
			MagnifyPaper(&margin,&margin,doc->magnify);
			OffsetRect(&margin,r.left,r.top);
			FrameRect(&margin);
			
			/* Page number */

#ifdef DRAW_PAGENUM
			pageNumR = doc->headerFooterMargins;
			OffsetRect(&pageNumR,r.left,r.top);
			FrameRect(&pageNumR);
#endif

			PenPat(NGetQDGlobalsBlack());
			PenMode(patCopy);
			}
		 else {
			/* Prepare each blank sheet in sheet array */
			for (i=doc->firstSheet; i<doc->numSheets; i++)
				if (GetSheetRect(doc,i,&paper)==INARRAY_INRANGE) {
					if (SectRect(&paper,&doc->viewRect,&result)) {
						if (!doc->enterFormat) EraseRect(&paper);
						FrameRect(&paper);
						/* And a drop shadow */
						MoveTo(paper.left+2,paper.bottom);
						LineTo(paper.right,paper.bottom);
						LineTo(paper.right,paper.top+2);
						}
					}
				 else
					break;
			/* And draw in each sheet's contents */
			for (i=doc->firstSheet; i<doc->numSheets; i++)
				if (GetSheetRect(doc,i,&paper)==INARRAY_INRANGE) {
					if (SectRect(&paper,&updater,&result)) {
						/*
						 *	Time to draw any of the i'th sheet's contents that
						 *	are found to intersect "result".
						 *	We temporarily change the port's origin
						 *	so that the upper left corner of the
						 *	sheet is at (0,0), draw the page in, and
						 *	then restore the port back to what it was.
						 *	Since we're moving the coordinate system
						 *	out from under the current clipping region,
						 *	we temporarily ask it to follow as well.
						 */
						DrawSheet(doc,i,&paper,&result);
						if (i == doc->currentSheet) MEUpdateCaret(doc);
						}
					}
				 else
					break;
			}
		
		/* Clear "entering Show Format" flag and update any carets or selections. */
		
		doc->enterFormat = False;
		DrawTheSelection();

		ClipRect(&doc->viewRect);
	}

/* A window has changed size: recompute everything that depends on size, and directly
redraw the scroll bars, message box/horizontal scroll bar boundary, and grow box here,
rather than depending on update events. */

extern Boolean magnifyOnly;			/* Secret message from SheetMagnify */

void RecomputeView(Document *doc)
	{
		Rect r,rem,messageRect,portRect;
		WindowPtr w = doc->theWindow;
		Point pt;
		short width,height,x,y;
		Boolean changingView;
		Rect contrlRect;
		
		
		SetPort(GetWindowPort(w));
		GetWindowPortBounds(w,&portRect);
		ClipRect(&portRect);
		
		/* Get new view and move grow rectangle to new position */

		PrepareMessageDraw(doc,&messageRect,True);
		EraseAndInval(&doc->growRect);
		
		GetControlBounds(doc->hScroll, &contrlRect);
		EraseAndInval(&contrlRect);
		GetControlBounds(doc->vScroll, &contrlRect);
		EraseAndInval(&contrlRect);

		EraseAndInval(&messageRect);

		GetWindowPortBounds(w,&portRect);
		doc->viewRect = portRect;
		doc->viewRect.right -= SCROLLBAR_WIDTH;
		doc->viewRect.bottom -= SCROLLBAR_WIDTH;
		
		/*
		 *	When shrinking the window, it is possible for the paper to become
		 *	completely hidden.  In this case, we force the view to change so that
		 *	the top left corner of the paper always remains in view.
		 */
		r = doc->viewRect;
		r.right -= config.hScrollSlop;
		r.bottom -= config.vScrollSlop;
		
		changingView = !SectRect(&doc->allSheets,&r,&rem);
		if (changingView) {
			if (r.right < doc->allSheets.left)
				doc->origin.h = doc->allSheets.left - (r.right - r.left);
			 else if (r.left > doc->allSheets.right)
			 	doc->origin.h = doc->allSheets.right - config.hScrollSlop;
			if (r.bottom < doc->allSheets.top)
				doc->origin.v = -(r.bottom - r.top);
			 else if (r.top > doc->allSheets.bottom)
				doc->origin.v = doc->allSheets.bottom - config.vScrollSlop;
			SetOrigin(doc->origin.h, doc->origin.v);
			OffsetRect(&doc->viewRect,doc->origin.h-doc->viewRect.left,
									  doc->origin.v-doc->viewRect.top);
			GetWindowPortBounds(w,&portRect);
			ClipRect(&portRect);
			if (!magnifyOnly)
				InvalWindowRect(doc->theWindow,&doc->viewRect);
			}
		
		/* Draw in new grow box. NB: DrawGrowIcon draws the boundary lines as well! */
		
		SetRect(&doc->growRect,0,0,SCROLLBAR_WIDTH+1,SCROLLBAR_WIDTH+1);
		OffsetRect(&doc->growRect,doc->viewRect.right,doc->viewRect.bottom);
		pt = TOP_LEFT(doc->growRect);
		DrawGrowIcon(w);
		/* DrawGrowBox(w,pt,True); */
		ValidWindowRect(w,&doc->growRect);

		/*
		 *	Move scroll bars to new sizes and positions, and take them out
		 *	of the update region, since they've already been drawn right here.
		 *	(this cuts down on flash).
		 */
		HideControl(doc->vScroll);
		GetWindowPortBounds(w,&portRect);
		r = portRect;
		r.right++; r.top--;
		r.left = r.right - (SCROLLBAR_WIDTH+1);
		r.bottom -= SCROLLBAR_WIDTH-1;
		MoveControl(doc->vScroll,r.left,r.top);
		SizeControl(doc->vScroll,r.right-r.left,r.bottom-r.top);
		
		HideControl(doc->hScroll);
		GetWindowPortBounds(w,&r);
		r.bottom++; r.left--;
		r.top = r.bottom - (SCROLLBAR_WIDTH+1);
		r.right -= SCROLLBAR_WIDTH-1;
		MoveControl(doc->hScroll,r.left+MESSAGEBOX_WIDTH,r.top);
		SizeControl(doc->hScroll,r.right-(r.left+MESSAGEBOX_WIDTH),r.bottom-r.top);

		/*
		 *	Now reset the maximum and minimum values for the scroll bars. These
		 *	values are designed to allow the user to always scroll in any direction,
		 *	but never so far as to allow allSheets to disappear from the window. The
		 *	control values are the origin of the window the controls are in.  This is
		 *	all done while the controls are still hidden.
		 */
		r = doc->viewRect;
		width  = (r.right  - r.left) - config.hScrollSlop;
		height = (r.bottom - r.top)  - config.vScrollSlop;
		
		/*
		 *	Set the scroll min and max, ensuring that they're always a multiple of 8
		 *	so that scrolling to the max (or min) maintains the background pattern
		 *	synchronization necessary for the illusion that only the pages are being
		 *	scrolled while the background is remaining stationary in the window.
		 *	Since we're using a period of 8, any background pattern may be used.
		 */
		y = doc->allSheets.top-config.vScrollSlop;
		if (y & 7) y -= (y & 7);
		SetControlMinimum(doc->vScroll,y);
		x = doc->allSheets.left-config.hScrollSlop;
		if (x & 7) x -= (x & 7);
		SetControlMinimum(doc->hScroll,x);
		y = doc->allSheets.bottom - config.vScrollSlop;
		if (y & 7) y -= (y & 7);
		SetControlMaximum(doc->vScroll,y);
		x = doc->allSheets.right - config.hScrollSlop;
		if (x & 7) x -= (x & 7);
		SetControlMaximum(doc->hScroll,x);
		
		ShowControl(doc->vScroll);		
		if (!changingView) {
			GetControlBounds(doc->vScroll, &contrlRect);
			ValidWindowRect(doc->theWindow,&contrlRect);
		}
		ShowControl(doc->hScroll);
		if (!changingView) {
			GetControlBounds(doc->hScroll, &contrlRect);
			ValidWindowRect(doc->theWindow,&contrlRect);
		}
		
		ClipRect(&doc->viewRect);

		{
#ifdef DEBUG_RECOMPUTEVIEW
			Rect paper;
			GetSheetRect(doc,doc->currentSheet,&paper);
			LogPrintf(LOG_DEBUG, "RecomputeView: viewRect(%R), currentPaper(%R)\r",
						&doc->viewRect,&paper);
#endif
		}
	}
