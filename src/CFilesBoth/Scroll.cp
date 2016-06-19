/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
 *	Scroll.c - routines for scrolling and related actions
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ScrollDocument should be called repeatedly by the Toolbox's TrackControl() when
doing continuous scrolling.  We always force the distance scrolled to be a
multiple of 8 since this is the period of the background pattern being painted
in behind sheets of paper in the document window, and if the coordinate system
stays in sync with the pattern, it will appear stationary. */

pascal void ScrollDocument(ControlHandle control, short part)
	{
		short change,size,dx=0,dy=0;
		register Document *theDoc = GetDocumentFromWindow(TopDocument);
		
		if (theDoc==NULL) return;
		
		size = (control == theDoc->vScroll) ?
				theDoc->viewRect.bottom - theDoc->viewRect.top :
				theDoc->viewRect.right - theDoc->viewRect.left;
		
		switch(part) {
			case kControlUpButtonPart:
				change = -32; break;		/* Should be multiple of background pattern period */
			case kControlDownButtonPart:
				change = 32; break;
			case kControlPageUpPart:
				change = -size/3;			/* Negative change */
				change -= (change & 7);		/* Get change mod 8 (0-7), round down */
				break;
			case kControlPageDownPart:
				change = size/3;
				change += 8 - (change & 7);		/* Multiple of 8 (round up) */
				break;
			default:
				break;
			}
		if (part) {
			if (control == theDoc->vScroll) dy = change; else dx = change;
			QuickScroll(theDoc,dx,dy,TRUE,TRUE);
			}
	}


/* QuickScroll(doc,dx,dy,rel,doCopy) scrolls the view rectangle (the content of the
window associated with document doc, minus the scroll and grow icon areas)
by dx pixels horizontally, and dy pixels vertically.  It does this without 
creating update events, since it can be called from within ScrollDocument(),
which is called repeatedly from TrackControl(), and from AutoScroll.
If either of dx or dy are so large that the entire view would be
scrolled offscreen, it just erases and redraws; otherwise, it BitBlt's
the view rectangle the appropriate distance, and redraws only the newly
scrolled-in rectangle(s).  Essentially, it implements an update region
using the document's view rectangle.  If doCopy is FALSE, then we don't
muck with anything in the view rectangle, but do everything else.
This gets complicated by any floating palettes, since each one of them
brings in 1 or 2 more rectangular areas to be updated.  Whether we should
just go back to standard update regions or not depends on how the page-
redrawing routines are when given this increasing number of of rectangles. */

void QuickScroll(register Document *doc, register short dx, register short dy,
					Boolean rel, Boolean doCopy)
	{
		Rect src,dst,r,vRect,hRect,*nextRect,*nr,screen,portRect;
		WindowPtr w; Point pt; GrafPtr oldPort;
		short width, height,cVal,cMax,cMin,nUpdates,i,nScreens;
		
		w = doc->theWindow;
		GetPort(&oldPort); SetPort(GetWindowPort(w));
		
		/*
		 *	The rest of the routine works in relative coordinates,
		 *	so if we're given absolute ones, convert them first to relative.
		 */
		
		if (!rel) {
			dx -= doc->origin.h;
			dy -= doc->origin.v;
			}
		
		/*
		 *	First, clip dx and dy so as not to scroll off page, and
		 *	adjust both the setting and the position of the scroll bars.
		 *	Assumes the current scroll bar maxima and minima have
		 *	pre-scrolled values in them.  We don't set the control value
		 *	until after we've checked clipping, so that the control
		 *	doesn't flash at its min or max positions.
		 */

		
		GetWindowPortBounds(w,&portRect);
		ClipRect(&portRect);
		if (dx) {
			cVal = GetControlValue(doc->hScroll);
			cMax = GetControlMaximum(doc->hScroll);
			cMin = GetControlMinimum(doc->hScroll);
			if ((dx + cVal) > cMax) dx = cMax - cVal;
			 else if ((dx + cVal) < cMin) dx = cMin - cVal;
			if (dx) SetControlValue(doc->hScroll,cVal+dx);
			}
		if (dy) {
			cVal = GetControlValue(doc->vScroll);
			cMax = GetControlMaximum(doc->vScroll);
			cMin = GetControlMinimum(doc->vScroll);
			if ((dy + cVal) > cMax) dy = cMax - cVal;
			 else if ((dy + cVal) < cMin) dy = cMin - cVal;
			if (dy) SetControlValue(doc->vScroll,cVal+dy);
			}
		ClipRect(&doc->viewRect);
		
		/* say("QuickScroll(%d,%d,%B,%B)\n",dx,dy,rel,doCopy); */
		
		 /* Skip the rest if no change is going to occur after clipping */
		 		
		if (dx==0 && dy==0) {
			SetPort(oldPort);
			return;
			}
		
		GetGlobalPort(w,&src);
		nScreens = GetMyScreen(&src,&screen);
		OffsetRect(&screen,doc->origin.h-src.left,doc->origin.v-src.top);
		
		src = doc->viewRect;
		SectRect(&src,&screen,&src);
		dst = src;
		width = src.right - src.left;
		height = src.bottom - src.top;
		
		/*
		 *	If scroll leaves any part of view visible, do a BitBlt to scroll
		 *	current image (without erasing first!), and then redraw the
		 *	content in the new update rectangle(s).  If the scroll is so far
		 *	that no part of the current image would be left on the screen,
		 *	just erase and update the whole view.
		 */
		 
		if (dx<width && dx>-width && dy<height && dy>-height) {
		
			/*
			 *	Scroll view's bits vertically and/or horizontally.  Use a
			 *	mask region that is the current visible region translated
			 *	it its eventual destination, so that any palette windows
			 *	are not copied in the full port copybits.  We'll update the
			 *	area opened up from under any palettes below.
			 */
	
			OffsetRect(&dst,-dx,-dy);
			if (doCopy) {
				RgnHandle maskRgn = NewRgn();				
				RgnHandle visRgn = NewRgn();
				GetPortVisibleRegion(GetWindowPort(w), visRgn);
				
				CopyRgn(visRgn,maskRgn);
				OffsetRgn(maskRgn,-dx,-dy);
				
				const BitMap *wPortBits = GetPortBitMapForCopyBits(GetWindowPort(w));
				CopyBits(wPortBits,wPortBits,&src,&dst,srcCopy,maskRgn);
				
				DisposeRgn(visRgn);
				DisposeRgn(maskRgn);
				}
			/*
			 *	Set a rectangle for each of the new areas just opened up.
			 *	If both dx and dy are non-zero, they will overlap, so
			 *	adjust one of them appropriately.
			 */
			
			OffsetRect(&src,dx,dy);
			if (dx) {
				vRect = src;
				if (dx > 0) vRect.left  += (width-dx);
				 else		vRect.right -= (width+dx);
				}
			if (dy) {
				hRect = src;
				if (dy > 0) hRect.top    += (height-dy);
				 else		hRect.bottom -= (height+dy);
				}
			nUpdates = 2;
			}
		 else {
			OffsetRect(&src,dx,dy);		/* New area just opened up */
			nUpdates = 1;
			}
		
		/* Change everything to new origin (dx,dy) away from current one */
				
		OffsetRect(&doc->viewRect,dx,dy);
		OffsetRect(&doc->growRect,dx,dy);
		SetOrigin(doc->origin.h=doc->viewRect.left, doc->origin.v=doc->viewRect.top);
		/* say("QuickScroll: Origin is now (%d,%d)\n",doc->origin.h,doc->origin.v);
		
		/*
		 *	We change a scroll bar's control rectangle directly without
		 *	calling MoveControl(), since that routine adds the old
		 *	position to the window's update region; we're doing our
		 *	own updating right here because this routine may be called
		 *	repeatedly during a single mouse click (see TrackControl()).
		 */
		OffsetContrlRect(doc->hScroll, dx, dy);
		OffsetContrlRect(doc->vScroll, dx, dy);
		
		/*
		 *	Now do the updates, by building list of rectangular areas to update.
		 *	Each rectangle in the list will be temporarily swapped with the
		 *	view rectangle while calling the document drawing routines, which
		 *	are expected to clip their behavior to the current viewRect.  When
		 *	we're done, we restore the view to the usual.  The size of the
		 *	rectangle array is always large enough to hold the two window
		 *	update rects plus two for each palette, so we don't need to check
		 *	for overflow.
		 */

		nextRect = updateRectTable;
		
		if (doCopy) {
			r = doc->viewRect;			/* Save for restoration below */
			if (nUpdates == 1) {
				/* Scrolled entire contents into oblivion: just redraw everything */
				*nextRect++ = src;
				}
			 else {
			 	if (dx) *nextRect++ = vRect;
			 	if (dy) *nextRect++ = hRect;
				/*
				 *	Now do the same for each visible floating palette above scroll area.
				 */
				for (i=0; i<TOTAL_PALETTES; i++)
					if (IsWindowVisible(palettes[i])) {
						if (dx) {
							/* Find position of palette in Document coord system */
							GetWindowPortBounds(palettes[i],nextRect);
							nextRect->top -= PALETTE_TITLE_BAR_HEIGHT+1;
							nextRect->bottom += 2;
							nextRect->right += 2;
							nextRect->left--;
							pt.h = pt.v = 0;
							SetPort(GetWindowPort(palettes[i]));
							LocalToGlobal(&pt);
							SetPort(GetDocWindowPort(doc));
							GlobalToLocal(&pt);			/* Origin already scrolled */
							
							/* Convert to window coords */
							OffsetRect(nextRect,pt.h,pt.v);
							if (SectRect(nextRect,&doc->viewRect,nextRect)) {
								OffsetRect(nextRect,-dx,0);
								
								/*
								 *	If scrolling diagonally, extend this rectangle
								 *	at top or bottom to get corner piece.  Don't
								 *	need to do this for both x and y, so we do it
								 *	here and not below in analogous y code.
								 */
								
								if (dy > 0) nextRect->top -= dy;
								 else if (dy < 0) nextRect->bottom -= dy;							
								
								/*
								 *	But clip rectangle on right and bottom to avoid
								 *	scroll bars
								 */
								if (nextRect->right > doc->viewRect.right)
									nextRect->right = doc->viewRect.right;
								if (nextRect->bottom > doc->viewRect.bottom)
									nextRect->bottom = doc->viewRect.bottom;
								nextRect++;
								}
							}
						if (dy) {
							GetWindowPortBounds(palettes[i],nextRect);
							nextRect->top -= PALETTE_TITLE_BAR_HEIGHT+1;
							nextRect->bottom += 2;
							nextRect->right += 2;
							nextRect->left--;
							pt.h = pt.v = 0;
							SetPort(GetWindowPort(palettes[i]));
							LocalToGlobal(&pt);
							SetPort(GetDocWindowPort(doc));
							GlobalToLocal(&pt);			/* Origin already scrolled */
							OffsetRect(nextRect,pt.h,pt.v);
							if (SectRect(nextRect,&doc->viewRect,nextRect)) {
								OffsetRect(nextRect,0,-dy);
								/*
								 *	But clip rectangle on right and bottom to avoid
								 *	scroll bars
								 */
								if (nextRect->right > doc->viewRect.right)
									nextRect->right = doc->viewRect.right;
								if (nextRect->bottom > doc->viewRect.bottom)
									nextRect->bottom = doc->viewRect.bottom;
								nextRect++;
								}
							}
						}
			 	}
			
			/* Got our list: now update them all in reverse order (palettes first) */
			
			for (nr=updateRectTable; nr!=nextRect; nr++) {
				doc->showWaitCurs = FALSE;	/* Avoid distracting cursor changes when auto-scrolling */
				DrawDocumentView(doc,nr);
				}
		
			/*
			 *	If window is on more than one screen, just invalidate everything
			 *	not on the screen that has already been scrolled, and redraw
			 *	using standard update mechanism.
			 */
			
			if (nScreens > 1) {
				InvalWindowRect(doc->theWindow,&doc->viewRect);
				ValidWindowRect(doc->theWindow,&src);
				ClipRect(&doc->viewRect);
				BeginUpdate(doc->theWindow);
				DrawDocumentView(doc,NULL);
				EndUpdate(doc->theWindow);
				}
			}

		/*
		 *	Port has moved but the old clipping rectangle hasn't; fix it.
		 */
		
		ClipRect(&doc->viewRect);
		SetPort(oldPort);
	}
