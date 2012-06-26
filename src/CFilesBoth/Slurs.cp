/***************************************************************************
*	FILE:	Slurs.c																				*
*	PROJ:	Nightingale, slight rev. for v.2.1											*
*	DESC:	Slur screen drawing, editing, and antikinking routines.				*
/***************************************************************************/

/*								NOTICE
 *
 *	THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 *	PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 *	TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT
 *	RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 *	Copyright © 1988-99 by Advanced Music Notation Systems, Inc.
 *	All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include <math.h>						/* For sqrt() prototype */

#define BOXSIZE 4						/* For editing handles (in pixels) */

static Boolean	searchSpline,			/* Suppress drawing and search for point instead */
					getBoundsSpline;		/* Suppress drawing and get bounding box instead */
static short	xSearch,ySearch;		/* Point to search for (screen coordinates) */
static short	slurLeft,slurRight,	/* Bounds to be found */
					slurBottom,slurTop;

/* Prototypes for local routines */

static void		DrawSlurBox(Rect *paper,DPoint knot,DPoint c0,DPoint c1,DPoint endPt);
static Boolean	SameDPoint(DPoint p1, DPoint p2);
static Boolean	DoSlurMouseDown(Document *doc, DPoint pt, LINK pL, LINK aSlurL, short index);
static Boolean	EditSlur(Rect *paper, SplineSeg *seg, DPoint *endpt, DPoint pt, short how);
static Boolean	Slursor(Rect *paper, DPoint *start,DPoint *end,DPoint *c0,DPoint *c1,short type,
						DPoint *q,short curve);
static void		StartSearch(Rect *paper, DPoint pt);
static void		EndSearch(void);
static Boolean	BezierTo(Point startPt, Point endPt, Point c0, Point c1, Boolean dashed);
static Boolean	DrawSegment(Rect *paper, SplineSeg *seg, DPoint endpoint);
static void		DeselectKnots(Rect *paper, LINK aSlurL);
static void		HiliteSlur(Rect *paper, LINK aSlurL);
static void		DrawDBox(Rect *paper, DPoint pt, short size);
static void		DRound(DPoint *dst, DPoint src);

static void		CreateTies(Document *doc, LINK pL, LINK aNoteL);

/* A slur object consists of a table of subobjects, each of which contains a record
consisting of one SplineSeg, where a SplineSeg is the coordinates of the start of
a spline, plus two control points.  All points are kept as pairs of DDISTs.  The
end of each segment would be the start of the next segment, and the end of the last
segment is kept separately in the record.  This is in case compound slurs are later
implemented. But the vast majority of slurs will only need the preallocated list of
one segment, and editing only supports single segment slurs right now, anyway. */

/*
 *	Take the offsets stored in aSlur->seg.knot, etc; take the endpoints stored 
 * in aSlur->startPt, endPt; add them to compute results to be returned in 
 * knot, c0, c1, endPt; these points can then be passed directly to BezierTo, etc.
 *	Points returned are in paper-relative DDISTs (not offsets from startPt, etc.)
 */
 
void GetSlurPoints(LINK	aSlurL, DPoint *knot, DPoint *c0, DPoint *c1, DPoint *endpoint)
 {
	SplineSeg *thisSeg; PASLUR aSlur;
			
	aSlur = GetPASLUR(aSlurL);
	thisSeg = &aSlur->seg;

	SetDPt(knot,p2d(aSlur->startPt.h),p2d(aSlur->startPt.v));
	SetDPt(endpoint,p2d(aSlur->endPt.h),p2d(aSlur->endPt.v));
	aSlur = GetPASLUR(aSlurL);
	
	knot->v += thisSeg->knot.v; knot->h += thisSeg->knot.h;
	endpoint->v += aSlur->endpoint.v; endpoint->h += aSlur->endpoint.h;
					
	c0->v = knot->v + thisSeg->c0.v; c0->h = knot->h + thisSeg->c0.h; 
	c1->v = endpoint->v + thisSeg->c1.v; c1->h = endpoint->h + thisSeg->c1.h;
 }

/*
 *	Check to see if two DPoint's are within slop of each other.
 */

static Boolean SameDPoint(DPoint p1, DPoint p2)
	{
		short dx,dy; static DDIST slop = pt2d(2);
		
		dx = p1.h - p2.h; if (dx < 0) dx = -dx;
		dy = p1.v - p2.v; if (dy < 0) dy = -dy;
		
		return( dx<=slop && dy<=slop );
	}
	
/*
 *	Use a wide line to draw a filled box at a point.
 */

static void DrawDBox(Rect *paper, DPoint p, short size)
	{
		p.h = d2p(p.h); p.v = d2p(p.v);
		
		PenSize(size,size); size >>= 1;
		MoveTo(paper->left + (p.h -= size), paper->top + (p.v -= size));
		LineTo(paper->left+p.h, paper->top+p.v);
		PenSize(1,1);
	}

/*
 *	Entertain events in a separate event loop for manipulating slur objects.
 *	While mouse clicks stay within the slur's objRect (bounding Box),
 *	we stay here.  If any other event, we leave here so that main event loop
 *	can deal with it.
 *
 *	This event loop is not considerate of other processes.
 */

void DoSlurEdit(Document *doc, LINK pL, LINK aSlurL, short index)
	{
		EventRecord		evt;
		Boolean			stillEditing = TRUE;
		DPoint			pt;
		Point				ptStupid;
		Rect				bboxBefore,bboxAfter;

		FlushEvents(everyEvent, 0);
		
		while (stillEditing) {
			ArrowCursor();
			if (EventAvail(everyEvent, &evt))
				switch(evt.what) {
					case mouseDown:
						ptStupid = evt.where; GlobalToLocal(&ptStupid);
						pt.v = p2d(ptStupid.v - doc->currentPaper.top);
						pt.h = p2d(ptStupid.h - doc->currentPaper.left);
						GetSlurBBox(doc,pL,aSlurL,&bboxBefore,BOXSIZE);
						stillEditing = DoSlurMouseDown(doc,pt,pL,aSlurL,index);
						FlushEvents(mUpMask,0);
                  if (stillEditing) {
                     EraseAndInval(&bboxBefore);
                     GetSlurBBox(doc,pL,aSlurL,&bboxAfter,BOXSIZE);
                     EraseAndInval(&bboxAfter);
                     }
						break;
					case updateEvt:
        	    		DoUpdate((WindowPtr)(evt.message));
        	    		if (stillEditing)
        	    			HiliteSlur(&doc->currentPaper,aSlurL);
						break;
					default:
						stillEditing = FALSE;
						break;
					}
			}
					
	DeselectKnots(&doc->currentPaper,aSlurL);
	}

/*
 *	Deliver the bounding box of the given slur sub-object in window-relative
 *	coordinates (not DDISTs).  Enlarge bbox by margin (for including handles
 *	to control pts).
 */

void GetSlurBBox(Document *doc, LINK pL, LINK aSlurL, Rect *bbox, short margin)
	{
		DPoint knot,c0,c1,endpoint; short j; LINK sL;
		Point startPt[MAXCHORD],endPt[MAXCHORD];
		PASLUR aSlur;
		
		GetSlurContext(doc, pL, startPt, endPt);						/* Get absolute positions, in DPoints */
		
		for (j=0,sL=FirstSubLINK(pL); sL!=aSlurL; j++, sL=NextSLURL(sL)) ;
		aSlur = GetPASLUR(aSlurL);
		aSlur->startPt = startPt[j];
		aSlur->endPt = endPt[j];

		GetSlurPoints(aSlurL,&knot,&c0,&c1,&endpoint);
		
		/* Get convex hull of 4 points */
		
		bbox->left = bbox->right = knot.h;
		bbox->top = bbox->bottom = knot.v;
		if (c0.h < bbox->left) bbox->left = c0.h;
		 else if (c0.h > bbox->right) bbox->right = c0.h;
		if (c1.h < bbox->left) bbox->left = c1.h;
		 else if (c1.h > bbox->right) bbox->right = c1.h;
		if (endpoint.h < bbox->left) bbox->left = endpoint.h;
		 else if (endpoint.h > bbox->right) bbox->right = endpoint.h;
		 
		if (c0.v < bbox->top) bbox->top = c0.v;
		 else if (c0.v > bbox->bottom) bbox->bottom = c0.v;
		if (c1.v < bbox->top) bbox->top = c1.v;
		 else if (c1.v > bbox->bottom) bbox->bottom = c1.v;
		if (endpoint.v < bbox->top) bbox->top = endpoint.v;
		 else if (endpoint.v > bbox->bottom) bbox->bottom = endpoint.v;
		
		D2Rect((DRect *)bbox,bbox);
		
		InsetRect(bbox,-margin,-margin);
		OffsetRect(bbox,doc->currentPaper.left,doc->currentPaper.top);
	}

/*
 *	A slur is hilited by drawing a small box at each of its knots and at each of
 *	the spline segments' control points.  We use Xor mode so that pairs of calls
 *	hilite and unhilite without graphic interference.
 */

void HiliteSlur(Rect *paper, LINK aSlurL)
	{
		SplineSeg *thisSeg; short nSegs=1,i;
		DPoint knot, c0, c1, endpoint;
		short size = BOXSIZE; PASLUR aSlur;
		
		if (aSlurL) {
			PenMode(patXor);
			aSlur = GetPASLUR(aSlurL);
			thisSeg = &aSlur->seg;
			for (i=1; i<=nSegs; i++,thisSeg++) {
				GetSlurPoints(aSlurL,&knot,&c0,&c1,&endpoint);
				
				DrawDBox(paper,knot,size); DrawDBox(paper,endpoint,size);
				DrawDBox(paper,c0,size); DrawDBox(paper,c1,size);
				}
			PenNormal();
			}
	}


/*
 *	Designate the currently selected slur by graphically hiliting it.  If a
 *	selected slur already exists, unhilite it (since we're using Xor mode),
 *	and then hilite the new one.  The NULL slur is legal and just deselects.
 *	The routine delivers the previous value of the selected slur.
 */

void SelectSlur(Rect *paper, LINK aSlurL)
	{
		HiliteSlur(paper,aSlurL);
	}

static void DeselectKnots(Rect *paper, LINK aSlurL)
	{
		HiliteSlur(paper,aSlurL);
	}
	
/*
 *	Deal with a mouse down event at DPoint pt.  Deliver TRUE if mouse down
 *	is eaten and dealt with here, FALSE if not dealt with.
 */

static Boolean DoSlurMouseDown(Document *doc, DPoint pt, LINK /*pL*/, LINK aSlurL,
											short /*index*/)
	{
		SplineSeg *thisSeg,seg; short how = -1, nSegs=1,i;
		DPoint endpoint;
		static Boolean found,changed;
		PASLUR aSlur;
		Rect paper;
		
		paper = doc->currentPaper;
		
		/*
		 *	For each spline segment in slur (these days there's only the first seg)
		 *	During all this, we turn search mode on so that we don't draw while
		 *	searching for spline segment "near" the given point.
		 */
		
		aSlur = GetPASLUR(aSlurL);
		for (thisSeg = &aSlur->seg,i = 1; i <= nSegs; i++,thisSeg++) {
		
			if (i == nSegs) endpoint = aSlur->endpoint;
			 else				 endpoint = (thisSeg+1)->knot;
			 
			GetSlurPoints(aSlurL, &seg.knot, &seg.c0, &seg.c1, &endpoint);
			
			/* Check if mouse falls in end knot points or control points */
			/* For compound spline curves, should check bBox first */
			
			if (SameDPoint(pt,seg.knot))	{ how = S_Start; goto gotit; }
			if (SameDPoint(pt,seg.c0))		{ how = S_C0; goto gotit; }
			if (SameDPoint(pt,seg.c1))		{ how = S_C1; goto gotit; }
			if (SameDPoint(pt,endpoint))	{ how = S_End; goto gotit; }
			
			/* Or along Bezier curve */
			
			StartSearch(&paper,pt);
			found = DrawSegment(&paper,&seg,endpoint);
			EndSearch();
			if (found) { how = S_Whole; goto gotit; }
		}

		return(FALSE);

gotit:
		/* Our mouse down event should be eaten and dealt with here */

		FlushEvents(mDownMask,0);
		/*
		 *	Temporarily deselect for a whole knot drag, so as not to leave
		 *	hanging boxes on the screen.  Prepare for a dragging slursor
		 */
		
		HiliteSlur(&paper,aSlurL);						/* Erase old knotboxes */
				
		if (changed = EditSlur(&paper, &seg, &endpoint, pt, how)) doc->changed = TRUE;

		if (changed) {

			/* Convert paper-relative coords back to offsets */
			aSlur = GetPASLUR(aSlurL);
			aSlur->seg.c0.h = seg.c0.h - seg.knot.h;
			aSlur->seg.c0.v = seg.c0.v - seg.knot.v;
			aSlur->seg.c1.h = seg.c1.h - endpoint.h;
			aSlur->seg.c1.v = seg.c1.v - endpoint.v;
			aSlur->seg.knot.h = seg.knot.h - p2d(aSlur->startPt.h); 
			aSlur->seg.knot.v = seg.knot.v - p2d(aSlur->startPt.v); 
			aSlur->endpoint.h = endpoint.h - p2d(aSlur->endPt.h);
			aSlur->endpoint.v = endpoint.v - p2d(aSlur->endPt.v);
			}
		
		DrawSegment(&paper,&seg,endpoint);
		return(TRUE);
	}

/*
 *	Given a slur segment of the selected slur, in which a mouse click has specified the
 *	indicated part, interactively change that part.  Deliver TRUE if an actual change
 *	occurred, FALSE if not.
 */

static Boolean EditSlur(Rect *paper, SplineSeg *seg, DPoint *endpoint, DPoint pt,
								short how)
	{
		DPoint oldStart={0,0},
					oldc0={0,0},
					oldc1={0,0},
					oldEnd={0,0};
	 	Boolean changed=FALSE;

		/* Get new position */
		Slursor(paper,&seg->knot,endpoint,&seg->c0,&seg->c1,how,&pt,0);
		
		changed = (oldStart.h!=seg->knot.h || oldStart.v!=seg->knot.v ||
						oldc0.h!=seg->c0.h || oldc0.v!=seg->c0.v ||
						oldc1.h!=seg->c1.h || oldc1.v!=seg->c1.v ||
						oldEnd.h!=endpoint->h || oldEnd.v!=endpoint->v);
	
		return changed;
	}

/*
 *	Draw a given spline segment with anchor point at seg->knot to the
 *	given end point. Return whether or not the current search point was
 *	encountered while "drawing" (see BezierTo).
 */

Boolean DrawSegment(Rect *paper, SplineSeg *seg, DPoint endpoint)
	{
		Point start,c0,c1,end;
		
		start.h = paper->left + d2p(seg->knot.h);
		start.v = paper->top + d2p(seg->knot.v);
		end.h = paper->left + d2p(endpoint.h);
		end.v = paper->top + d2p(endpoint.v);
		c0.h = paper->left + d2p(seg->c0.h);
		c0.v = paper->top + d2p(seg->c0.v);
		c1.h = paper->left + d2p(seg->c1.h);
		c1.v = paper->top + d2p(seg->c1.v);
		
		MoveTo(start.h,start.v);
		return(BezierTo(start,end,c0,c1,FALSE));
	}

	
/*
 *	Here we create a movable slur spline section, between start and end with
 *	control points c0 and c1.  If type is S_New or S_Extend, we initialise end,
 *	c0, and c1 here; otherwise we use the provided arguments.  When we're done,
 *	the current spline (in DDIST coordinates) is returned in end, c0, and c1
 *	(usually anchored at start).  q points to the original click point (which is
 *	not a control point if type == S_Whole). When we drag an endpoint, the assoc-
 *	iated control point follows in parallel.
 *
 *	If the Shift key is held down: if we're dragging a control point, we force
 *	the opposite control point to work symmetrically; if we're dragging an end
 *	point, we constrain motion to whichever axis was used first (this is the
 * conventional meaning of the Shift key when dragging in Mac graphics programs).
 *	Finally, we deliver TRUE if resulting spline is at least a certain size;
 *	otherwise FALSE.
 */

static DPoint keepStart,keepEnd,keepc0,keepc1;
static Rect keepPaper;
static short keepType;

Boolean Slursor(Rect *paper, DPoint *start, DPoint *end,
						DPoint *c0, DPoint *c1, short type, DPoint *q, short curve)
	{
		DPoint p, tmp; DPoint *test; DDIST dist, dist4, dist8, dx, dy;
		Boolean first=TRUE, shift=FALSE,up,constrain,knotConstrain,horiz,
					stillWithinSlop;
		Point pt, origPt; short xdiff,ydiff;
		
		if (type == S_Extend) type = S_New;		/* Not implemented: use New */
		
		if (type == S_New) *end = *c0 = *c1 = *start;
		 else			   up = curve>0 ? TRUE : curve<0 ? FALSE : (c0->v >= start->v);
				
		PenMode(patXor);									/* Everything will be drawn twice */
		DrawSlursor(paper,start,end,c0,c1,type,FALSE);	/* Draw first one in before loop */

		 /* Choose which point we want to follow the mouse */
		 
		switch(type) {
			case S_New:		test = end; break;
			case S_Start:	test = start; tmp = *start; break;
			case S_C0:		test = c0; break;
			case S_C1:		test = c1; break;
			case S_End:		test = end; tmp = *end; break;
			case S_Whole:	test = q; tmp = *q; break;
			}
		
		theSelectionType = SLURSOR;
		constrain = ShiftKeyDown();
		knotConstrain = (constrain && (type==S_Start || type==S_End));
		stillWithinSlop = TRUE;
		GetPaperMouse(&origPt,paper);

		if (StillDown())
			while (WaitMouseUp()) {
				GetPaperMouse(&pt,paper);
				if (knotConstrain && stillWithinSlop) {
					xdiff = pt.h - origPt.h; ydiff = pt.v - origPt.v;
					
					/* If we're still within slop bounds, don't do anything */
					if (ABS(xdiff)<2 && ABS(ydiff)<2) continue;
					horiz = ABS(xdiff) > ABS(ydiff);      /* 45 degree movement => vertical */
					stillWithinSlop = FALSE;
					}

				p.h = p2d(pt.h); p.v = p2d(pt.v);
				if (test->h!=p.h || test->v!=p.v) {
					/* Don't flicker if the axis we care about hasn't changed. */
					if (knotConstrain) {
						if (horiz && test->h==p.h || !horiz && test->v==p.v)
							continue;
					}
					if (first) if (type==S_New || type==S_Extend)
						{ up = curve>0 ? TRUE : curve<0 ? FALSE : p.v<=end->v; first = FALSE; }
					
					DrawSlursor(paper,start,end,c0,c1,type,FALSE);	/* Erase old slursor */
					GetPaperMouse(&pt,paper);						/* Get the most up-to-date position */

					/* If we're constraining to horiz. or vertical, discard other axis */
					if (knotConstrain) {
						if (horiz) pt.v = d2p(test->v);
						else		  pt.h = d2p(test->h);
						}
					
					test->v = p2d(pt.v); test->h = p2d(pt.h);
					
					switch(type) {
						case S_New:
							dist = end->h - start->h;
							dist4 = dist>>2; dist8 = dist4>>1;
							if (dist8 > 0) dist8 = -dist8;
							if (up) dist8 = -dist8;
							c0->h = start->h + dist4;
							c1->h =   end->h - dist4;
							c0->v = start->v - dist8;
							c1->v =   end->v - dist8;
							break;
						case S_Start:
							dx = test->h - tmp.h; dy = test->v - tmp.v;
							c0->h += dx; c0->v += dy;
							tmp = *test;
							break;
						case S_C0:
							if (constrain) {
								dx = test->h - start->h;
								dy = test->v - start->v;
								c1->h = end->h - dx;
								c1->v = end->v + dy;
								}
							break;
						case S_C1:
							if (constrain) {
								dx = test->h - end->h;
								dy = test->v - end->v;
								c0->h = start->h - dx;
								c0->v = start->v + dy;
								}
							break;
						case S_End:
							dx = test->h - tmp.h; dy = test->v - tmp.v;
							c1->h += dx; c1->v += dy;
							tmp = *test;
							break;
						case S_Whole:
					 		dx = test->h - tmp.h; dy = test->v - tmp.v;
					 		start->h += dx; end->h += dx;
					 		c0->h += dx; c1->h += dx;
					 		start->v += dy; end->v += dy;
					 		c0->v += dy; c1->v += dy;
					 		tmp = *test;
					 		break;
						}
					DrawSlursor(paper,start,end,c0,c1,type,FALSE);	/* Draw new section */
					/* Save what we just drew for benefit of AutoScrolling */
					keepStart = *start;
					keepEnd = *end;
					keepc0 = *c0;
					keepc1 = *c1;
					keepPaper = *paper;
					keepType = type;
		
					AutoScroll();
					}
				}
		/* Erase final image of cursor */
		DrawSlursor(paper,start,end,c0,c1,type,FALSE);
		PenNormal();
		theSelectionType = 0;
		
		 /* Now determine if spline is large enough to be good (4 pixels on each axis) */
		dist = p2d(4);
		dx = ABS(start->h - end->h); dy = ABS(start->v-end->v);
		return(dx>dist || dy>dist);
	}

/*
 *	Draw on the screen a spline from start to end, using c0 and c1 as control points.
 *	According to type, also draw a line from the appropriate endpoint to the
 *	associated control point.
 */

void DrawSlursor(Rect *paper, DPoint *start, DPoint *end, DPoint *c0, DPoint *c1,
						short type, Boolean dashed)
	{
		Point pStart,pEnd,pC0,pC1;
		
		/* Convert all arguments from DDIST to paper-relative points */
		
		pStart.h = paper->left + d2p(start->h);
		pStart.v = paper->top + d2p(start->v);
		pEnd.h = paper->left + d2p(end->h);
		pEnd.v = paper->top + d2p(end->v);
		pC0.h = paper->left + d2p(c0->h);
		pC0.v = paper->top + d2p(c0->v);
		pC1.h = paper->left + d2p(c1->h);
		pC1.v = paper->top + d2p(c1->v);
		
		/* Draw Bezier curve plus a line to any pertinent control point */
		
		MoveTo(pStart.h,pStart.v);
		BezierTo(pStart,pEnd,pC0,pC1,dashed);

		if (type == S_C1) {
			MoveTo(pEnd.h,pEnd.v);
			LineTo(pC1.h,pC1.v);
		 	}
		 else if (type == S_C0) {
			MoveTo(pStart.h,pStart.v);
			LineTo(pC0.h,pC0.v);
			}

		/* Draw a box for the handle according to type */
				
		switch (type) {
			case S_Start:	
				DrawDBox(paper,*start, BOXSIZE); break;
			case S_C0:
				DrawDBox(paper,*c0, BOXSIZE); break;
			case S_C1:
				DrawDBox(paper,*c1, BOXSIZE); break;
			case S_End:
				DrawDBox(paper,*end, BOXSIZE); break;
		}
	}

void DrawTheSlursor()
	{
		DrawSlursor(&keepPaper,&keepStart,&keepEnd,&keepc0,&keepc1,keepType,FALSE);
	}

/*
 *	Draw a Bezier spline from startPt to endPt, using c0, c1 as control Points.
 *	This version is a fast version for integer coordinates, done in homebrew
 *	Fixed arithmetic, suitable for changing cursors, etc.
 *	If searchSpline is TRUE, don't draw; rather check for proximity to the point
 *	(xSearch,ySearch), and return TRUE if point is "close" to the spline.
 *	If getBoundsSpline is TRUE, maximize/minimize to find bounding box.
 */

void StartSearch(Rect *paper, DPoint pt)
	{
		searchSpline = TRUE;
		xSearch = paper->left + d2p(pt.h); ySearch = paper->top + d2p(pt.v);
	}

void EndSearch()
	{
		searchSpline = FALSE;
	}
	
Boolean FindSLUR(Rect *paper, Point pt, LINK aSlurL)
	{
		DPoint p,endpoint; short i,nSegs=1; PASLUR aSlur;
		SplineSeg *thisSeg,seg; Boolean found = FALSE;
		
		p.h = p2d(pt.h); p.v = p2d(pt.v);
		StartSearch(paper,p);
		
		aSlur = GetPASLUR(aSlurL);
		for (i=1,thisSeg = &aSlur->seg; i<=nSegs; i++,thisSeg++) {
			if (i == nSegs) endpoint = aSlur->endpoint;
			 else				 endpoint = (thisSeg+1)->knot;
			GetSlurPoints(aSlurL,&seg.knot,&seg.c0,&seg.c1,&endpoint);
			if (DrawSegment(paper,&seg,endpoint)) { found = TRUE; break; }		/* In search mode */
			}
		EndSearch();
		return found;
	}

void StartSlurBounds()
	{
		searchSpline = FALSE;
		getBoundsSpline = TRUE;
		slurLeft = slurTop = 32767;
		slurRight = slurBottom = -32767;
	}

void EndSlurBounds(Rect *paper, Rect *box)
	{
		SetRect(box,slurLeft,slurTop,slurRight,slurBottom);
		OffsetRect(box,-paper->left,-paper->top);				/* Convert box to paper coords */
		getBoundsSpline = FALSE;
	}

/*
 * Draw next segment of dashed line: alternate between a series of dash segments
 *	(LineTo's) and a series of space segments (MoveTo's).
 */
 
static void LineOrMoveTo(short, short, short, Boolean *, short *);
static void LineOrMoveTo(short x, short y, short segsPerDash,
									Boolean *pDoingDash, short *pSegsThisDash)
	{
		if (*pDoingDash) LineTo(x,y);
		else				  MoveTo(x,y);
		
		(*pSegsThisDash)--;
		if (*pSegsThisDash<=0) {
			*pDoingDash = !(*pDoingDash);
			*pSegsThisDash = segsPerDash;
		}
	}


#define DASHLEN 3	/* in pixels */

/*
 *	Draw, or search for a point near, a Bezier spline in integer window coordinates.
 */

static Boolean BezierTo(Point startPt, Point endPt, Point c0, Point c1, Boolean dashed)
	{
		long i,ax,ay,bx,by,cx,cy,curx,cury;
		short x,y,s,s1,s2,s3,lastx,lasty,dx,dy;
		Boolean found = FALSE;
		static short epsilon = 5;
		FASTFLOAT segLen; short segsPerDash,segsThisDash;
		Boolean doingDash;

		curx = startPt.h; cury = startPt.v;
		
		lastx = curx-1;									/* Force first draw */

		/*	Compute the integer Bezier spline coefficients, a, b, and c. */

		cx = (c0.h - curx); cx += cx << 1;			/* times 3 */
		cy = (c0.v - cury); cy += cy << 1;
		
		bx = (c1.h - c0.h); bx += (bx << 1) - cx;	/* times 3 - c */
		by = (c1.v - c0.v); by += (by << 1) - cy;
		
		ax = (endPt.h - curx) - cx - bx;
		ay = (endPt.v - cury) - cy - by;


		s = 32; s1 = 5;						/* Number of lines in spline, and bits */
		if (outputTo==toScreen) {
			if (config.fastScreenSlurs==1)
				{ s = 16; s1 = 4; }			/* Number of lines in spline, and bits */
			if (config.fastScreenSlurs==2)
				{ s = 8; s1 = 3; }			/* Number of lines in spline, and bits */
		}

		s2 = s1+s1; s3 = s2+s1;		/* s3 is 15 bits worth of scaling */
		
		bx   <<= s1;   by <<= s1;	/* Scale operands up for later according */
		cx   <<= s2;   cy <<= s2;	/* to the degree in i in loop below */
		curx <<= s3; cury <<= s3;

		if (dashed) {
			/*
			 * Try to find the no. of segments that will make dashes (and spaces) as
			 * close as possible to DASHLEN pixels. The following doesn't do a very
			 *	good job. Also, it would be better to make dashes and spaces on the screen
			 *	agree with the PostScript versions, so their lengths should be independent
			 *	of magnification.
			 */
			segLen = ABS(endPt.h-startPt.h)/(FASTFLOAT)s;
			segsPerDash = DASHLEN/segLen;
			if (segsPerDash<1) segsPerDash = 1;
			doingDash = TRUE;
			segsThisDash = segsPerDash;
		}
		
		if (searchSpline)

			for (i=0; i<=s; i++) {
				x = (i * (i * (i * ax + bx) + cx) + curx) >> s3;
				y = (i * (i * (i * ay + by) + cy) + cury) >> s3;
				if (lastx!=x || lasty!=y) {							/* ?? lasty NEVER INIT.! */
					dx = x - xSearch; if (dx < 0) dx = -dx;
					dy = y - ySearch; if (dy < 0) dy = -dy;
					if (dx<=epsilon && dy<=epsilon) { found = TRUE; break; }
					lastx = x; lasty = y;
					}
				}

		 else if (getBoundsSpline)
		 
			for (i=0; i<=s; i++) {
				x = (i * (i * (i * ax + bx) + cx) + curx) >> s3;
				y = (i * (i * (i * ay + by) + cy) + cury) >> s3;
				if (lastx!=x || lasty!=y) {
					if (dashed) LineOrMoveTo(x,y,segsPerDash,&doingDash,&segsThisDash);
					else			LineTo(x,y);
					 /* Keep track of bounding box */
					if (x < slurLeft) slurLeft = x;
					if (x > slurRight) slurRight = x;
					if (y < slurTop) slurTop = y;
					if (y > slurBottom) slurBottom = y;
					lastx = x; lasty = y;
					}
				}
	
		 else								/* Otherwise, just draw spline */
		 
			for (i=0; i<=s; i++) {
				x = (i * (i * (i * ax + bx) + cx) + curx) >> s3;
				y = (i * (i * (i * ay + by) + cy) + cury) >> s3;
				if (lastx!=x || lasty!=y) {
					dx = x - lastx; if (dx < 0) dx = -dx;
					dy = y - lasty; if (dy < 0) dy = -dy;
					if (dx<=1 && dy<=1) MoveTo(x,y);

					if (dashed) LineOrMoveTo(lastx=x,lasty=y,segsPerDash,
														&doingDash,&segsThisDash);
					else			LineTo(lastx=x,lasty=y);
					}
				}
	
		return(found);
  }

/*
 *	TrackSlur lets the user pull around a slur, anchored at DPoint p, and then,
 *	when the user lets go of the mouse, delivers the paper-relative coordinates
 *	(in DPoints) of the slur.  It returns FALSE if the slur is below a certain
 * minimum length.
 */

Boolean TrackSlur(Rect *paper, DPoint p, DPoint *knot, DPoint *endpoint,
							 DPoint *c0, DPoint *c1, Boolean curveUp)
	{
		*knot = p; *endpoint = p; *c0 = p; *c1 = p;					/* Default: degenerate */
		
		return Slursor(paper,knot,endpoint,c0,c1,S_New,&p,curveUp?1:-1);
	}

/* ------------------------------------------------------------- GetSlurNoteLinks -- */
/* Given a Slur object and one of its subobjects, and firstSyncL and lastSyncL (from
the Slur object), return LINKs to the note subobjects that our Slur subobject connects.
Notice that if the Slur is cross-system, either firstSyncL or lastSyncL will not
actually be a Sync; in that case, we return NILINK for the corresponding note.
NB: Perhaps this should call Tie2NoteLINK. */

static void GetSlurNoteLinks(LINK, LINK, LINK, LINK, LINK *, LINK *);
static void GetSlurNoteLinks(
					LINK slurL, LINK aSlurL,					/* Slur object, subobject */
					LINK firstSyncL, LINK lastSyncL,
					LINK *pFirstNoteL, LINK *pLastNoteL		/* Notes/rests or NILINK */
					)
{
	LINK firstNoteL=NILINK, lastNoteL=NILINK, aNoteL; PANOTE aNote;
	PASLUR aSlur; Boolean isASlur;
	short voice, slFirstInd, slLastInd, firstInd=-1, lastInd=-1;
	
	voice = SlurVOICE(slurL);
	isASlur = !SlurTIE(slurL);
	aSlur = GetPASLUR(aSlurL);
	slFirstInd = aSlur->firstInd;
	slLastInd = aSlur->lastInd;

	if (SyncTYPE(firstSyncL)) {
		aNoteL = FirstSubLINK(firstSyncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->voice==voice) {
				if (isASlur) {
					if (aNote->slurredR) { firstNoteL = aNoteL; break; }
				}
				else
					if (++firstInd==slFirstInd) { firstNoteL = aNoteL; break; }
			}
		}
	}

	if (SyncTYPE(lastSyncL)) {
		aNoteL = FirstSubLINK(lastSyncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->voice==voice) {
				if (isASlur) {
					if (aNote->slurredL) { lastNoteL = aNoteL; break; }
				}
				else
					if (++lastInd==slLastInd) { lastNoteL = aNoteL; break; }
			}
		}
	}

	*pFirstNoteL = firstNoteL;
	*pLastNoteL = lastNoteL;
}

/* -------------------------------------------------------------- GetSlurContext -- */
/* Given a Slur object, return arrays of the paper-relative starting and ending
positions (expressed in points) of the notes delimiting its subobjects. */

void GetSlurContext(Document *doc, LINK pL, Point startPt[], Point endPt[])
	{
		CONTEXT 	localContext;
		DDIST		dFirstLeft, dFirstTop, dLastLeft, dLastTop,
					xdFirst, xdLast, ydFirst, ydLast;
		PANOTE 	firstNote, lastNote;
		PSLUR		p;
		LINK		firstNoteL, lastNoteL, aSlurL, firstSyncL, 
					lastSyncL, systemL;
		PSYSTEM  pSystem;
		short 	k, xpFirst, xpLast, ypFirst, ypLast, firstStaff, lastStaff;
		Boolean  firstMEAS, lastSYS;

		/* If the Slur is cross-system, its firstSyncL may actually be a Measure (if
			it's the second piece of the cross-system pair), or its lastSyncL may actually
			be a System (if it's the first piece of the cross-system pair, in which case
			there must be another System following). These are the only legal poss-
			ibilities. */
		
		p = GetPSLUR(pL);
		firstMEAS = MeasureTYPE(p->firstSyncL);
		lastSYS = SystemTYPE(p->lastSyncL);
			
		/* Handle special cases for crossSystem & crossStaff slurs. If p->crossStaff,
			and slur drawn bottom to top, firstStaff is one greater, else lastStaff is.
			(??Surely we should really do this by voice and ignore the staff, though
			it looks like there'll only be a difference in patholgical cases.)  */
		
		firstStaff = lastStaff = p->staffn;
		if (p->crossStaff) {
			if (p->crossStfBack)
					firstStaff += 1;
			else	lastStaff += 1;
		}

		/* Get left and right end context, considering special cases for crossSystem
			slurs. If p->firstSyncL is not a sync, find the first sync to get the context
			from. If lastSYS, p->lastSyncL must have an RSYS; find the first sync to the
			left of RSYS to get the context from. */

		firstSyncL = (firstMEAS ? LSSearch(p->firstSyncL, SYNCtype, firstStaff, GO_RIGHT, FALSE)
										: p->firstSyncL);

		p = GetPSLUR(pL);
		if (lastSYS) {
			systemL = p->lastSyncL;
			lastSyncL = LSSearch(LinkRSYS(systemL),SYNCtype,lastStaff,GO_LEFT,FALSE);
		}
		else
			lastSyncL = p->lastSyncL;

		GetContext(doc, firstSyncL, firstStaff, &localContext);
		dFirstLeft = localContext.measureLeft;								/* abs. origin of left end coords. */
		dFirstTop = localContext.measureTop;
		
		GetContext(doc, lastSyncL, lastStaff, &localContext);
		if (!lastSYS)
			dLastLeft = localContext.measureLeft;							/* abs. origin of right end coords. */
		else {
			pSystem = GetPSYSTEM(systemL);
			dLastLeft = pSystem->systemRect.right;
		}
		dLastTop = localContext.measureTop;

		/* Find the links to the first and last notes to which each slur/tie is attached,
			and get the notes' positions. If the slur is crossSystem, ??? */
		
		aSlurL = FirstSubLINK(pL);
		for (k = 0; aSlurL; k++, aSlurL = NextSLURL(aSlurL)) {
			GetSlurNoteLinks(pL, aSlurL, firstSyncL, lastSyncL, &firstNoteL, &lastNoteL);

			p = GetPSLUR(pL);
			if (firstMEAS) {
				lastNote = GetPANOTE(lastNoteL);
				xdFirst = dFirstLeft;
				ydFirst = dFirstTop + lastNote->yd;
			}
			else {
				firstNote = GetPANOTE(firstNoteL);
				xdFirst = dFirstLeft + LinkXD(p->firstSyncL) + firstNote->xd;	/* abs. position of 1st note */
				ydFirst = dFirstTop + firstNote->yd;
			}
			
			if (lastSYS) {
				/*
				 *	The slur should extend only to the end of the staff, but xdLast is now
				 *	the end of the systemRect, which is ExtraSysWidth(doc) further to the
				 *	right. Correct this. ??NB: this makes no sense, but subtracting 64 gives
				 * better results than subtracting ExtraSysWidth(doc). Somebody explain.
				 */
				
				firstNote = GetPANOTE(firstNoteL);
				xdLast = dLastLeft-64;
				ydLast = dLastTop + firstNote->yd;
			}
			else {
				lastNote = GetPANOTE(lastNoteL);
				xdLast = dLastLeft + LinkXD(p->lastSyncL) + lastNote->xd;			/* abs. position of last note */
				ydLast = dLastTop + lastNote->yd;
			}

			/* If either end is a rest, make the slur horizontal, as if a tie. */
			if (NoteREST(firstNoteL) && !lastSYS) ydFirst = ydLast;
			if (NoteREST(lastNoteL) && !firstMEAS) ydLast = ydFirst;

			xpFirst = d2p(xdFirst);
			ypFirst = d2p(ydFirst);
			xpLast = d2p(xdLast);
			ypLast = d2p(ydLast);
			SetPt(&startPt[k], xpFirst, ypFirst);
			SetPt(&endPt[k], xpLast, ypLast);
		}
	}

/* --------------------------------------------------------------- HiliteSlurNodes -- */

void HiliteSlurNodes(Document *doc, LINK pL)
{
	PSLUR p;
	PenState pn;
	
	GetPenState(&pn);
	PenMode(patXor);

	/* We don't flash here because it's unnecessary (the slur's staff is obvious
		in any reasonable situation) and it slows down a common operation. */
	
	p = GetPSLUR(pL); HiliteInsertNode(doc, p->firstSyncL, p->staffn, FALSE);	/* Hiliting on... */
	p = GetPSLUR(pL); HiliteInsertNode(doc, p->lastSyncL, p->staffn, FALSE);
	SleepTicks(HILITE_TICKS);

	while (Button()) ;

	p = GetPSLUR(pL); HiliteInsertNode(doc, p->firstSyncL, p->staffn, FALSE);	/* Off. */
	p = GetPSLUR(pL); HiliteInsertNode(doc, p->lastSyncL, p->staffn, FALSE);

	SetPenState(&pn);
}

void PrintSlurPoints(LINK aSlurL, char */*str*/)
{
	PASLUR aSlur;
	
	aSlur = GetPASLUR(aSlurL);
}

/* ------------------------------------------------------------ SetSlurCtlPoints -- */
/*	Given a slur or tie, set its Bezier control points.
N.B. This routine assumes the slur/ties' startPt and endPt describe the
positions of the notes they're attached to, which doesn't sound like a
very good idea--why can't it get the notes' actual positions? */

#define SCALECURVE(z)	(4*(z)/2)		/* Scale default curvature */

Boolean SetSlurCtlPoints(
		Document *doc,
		LINK slurL, LINK aSlurL,
		LINK firstSyncL, LINK lastSyncL,
		short staff, short /*voice*/,
		CONTEXT context,
		Boolean curveUp
		)
{
	DDIST			noteWidth=0, span, dLineSp;
	DDIST			x,y,x0,y0,x1,y1,tmp;
	FASTFLOAT	t;
	PASLUR		aSlur;
	//LINK		pL,aNoteL;
	//PANOTE		aNote;
	//short 		type,stemUp,accent,beam,degree;
	//DDIST		dTop,dBot,dLeft,xd,yd,stemLength;
	//DDIST		xoffFirst,xoffLast,yoffFirst,yoffLast;
	long			ltemp;
	
	/* Get horizontal offsets from note positions to slur ends. If it is a
		crossSystem slur, either firstSync or lastSync will not be a sync; in
		this case, make the respective noteWidth equal zero. */
	
	if (SyncTYPE(firstSyncL))
		noteWidth = std2d(SymWidthRight(doc, firstSyncL, staff, TRUE),
									context.staffHeight, context.staffLines);

	aSlur = GetPASLUR(aSlurL);
	aSlur->seg.knot.h = (2*noteWidth)/3;

	noteWidth = 0;
	if (SyncTYPE(lastSyncL))
		noteWidth = std2d(SymWidthRight(doc, lastSyncL, staff, TRUE),
									context.staffHeight, context.staffLines);

	aSlur = GetPASLUR(aSlurL);
	aSlur->endpoint.h = noteWidth/6;
	
	span = (p2d(aSlur->endPt.h)+aSlur->endpoint.h) -
			 (p2d(aSlur->startPt.h)+aSlur->seg.knot.h);
	dLineSp = std2d(STD_LINEHT, context.staffHeight, context.staffLines);
	aSlur->seg.knot.v = aSlur->endpoint.v = curveUp ? -dLineSp : dLineSp;

PrintSlurPoints(aSlurL, "SetSlurCtlPts: 1");

	/*
	 *	We now set the control points, which affect curvature.  This eventually
	 *	wants to be done using the Ohio slurs algorithms, but in the meantime,
	 *	we use the following methods, one for short spans and one for long spans,
	 *	with a linear blending between the two for medium spans.
	 */

	/* Compute left control point offset from slur start for short slurs */
	
	x0 = SCALECURVE(dLineSp);
	if (x0 > span/3) x0 = span/3;
	ltemp = (SlurTIE(slurL)? config.tieCurvature*dLineSp : config.slurCurvature*dLineSp);
	y0 = (short)(ltemp/100L);
	
	/* Compute left control point offset from slur start for long slurs */
	
	x1 = span/4;
	y1 = span/16;
	tmp = 6*dLineSp;
	
	if (span > 2*tmp) {				/* Long slur */
		x0 = x1;
		y0 = y1;
		}
	 else if (span > tmp) {			/* Blend the two */
	 	t = (span - tmp) / tmp;
	 	x0 = x0 + t*(x1-x0);
	 	y0 = y0 + t*(y1-y0);
	 	}
	 else ;								/* Otherwise, (x0,y0) has the short slur offset already */

	/* Now make left control point symmetric version of right */
	aSlur = GetPASLUR(aSlurL);
	aSlur->seg.c0.h = x0;
	aSlur->seg.c0.v = curveUp ? -y0 : y0;
	aSlur->seg.c1.h = -x0;
	aSlur->seg.c1.v = aSlur->seg.c0.v;

/*
	c0 is the offset from the left slur end to the left control point, c1 the offset
	from the right slur end to the right control point; but they are only symmetric
	about the vertical in the case where the two endpoints of the slur/tie are at the
	same height.  Therefore, we need to rotate the control point offset vectors to keep
	the slur symmetric.
 */

	aSlur = GetPASLUR(aSlurL);
	
	/* Get 0-based vector from slur knot to endpoint */
	x = (p2d(aSlur->endPt.h)+aSlur->endpoint.h) -
		 (p2d(aSlur->startPt.h)+aSlur->seg.knot.h);
	y = (p2d(aSlur->endPt.v)+aSlur->endpoint.v) -
		 (p2d(aSlur->startPt.v)+aSlur->seg.knot.v);

	RotateSlurCtrlPts(aSlurL,x,y,1);

	return TRUE;
}


/* ------------------------------------------------------------ RotateSlurCtrlPts -- */
/* Given a slur, rotate its control points by the angle of the vector (vx,vy).  Do it
counterclockwise if ccw is 1, clockwise if ccw is -1  */

void RotateSlurCtrlPts(LINK aSlurL, DDIST vx, DDIST vy, short ccw)
{
	PASLUR aSlur;
	FASTFLOAT x,y,cx,cy,r;
	FASTFLOAT cs,sn;
	
	/* Get the length, cosine, and sine of the angle of the vector */
	x = vx; y = vy;
	r = sqrt(x*x + y*y);
	if (r > 0.0) {
		cs = x/r;							/* Cosine */
		sn = y/r;							/* Sine */
		sn *= ccw;
		
		/* Rotate the c0 offset from the start of the slur by same angle as (x,y) */
		
		aSlur = GetPASLUR(aSlurL);	
		cx = aSlur->seg.c0.h * cs - aSlur->seg.c0.v * sn;
		cy = aSlur->seg.c0.h * sn + aSlur->seg.c0.v * cs;
		aSlur->seg.c0.h = cx;
		aSlur->seg.c0.v = cy;
		
		/* Rotate the c1 offset from the end of the slur by the same angle also */
		
		cx = aSlur->seg.c1.h * cs - aSlur->seg.c1.v * sn;
		cy = aSlur->seg.c1.h * sn + aSlur->seg.c1.v * cs;
		aSlur->seg.c1.h = cx;
		aSlur->seg.c1.v = cy;
	}
}

/* ------------------------------------------------------------------ CreateTies -- */
/* Create a set of ties for aNoteL and all other notes in its voice in pL that have
corresponding notes in the next Sync in its voice that are tiedL. */

static void CreateTies(Document *doc, LINK pL, LINK aNoteL) 
{
	LINK qL, bNoteL; PANOTE aNote, bNote;
	
	aNote = GetPANOTE(aNoteL);
	if (aNote->tiedR) {
		qL = LVSearch(RightLINK(pL), SYNCtype, NoteVOICE(aNoteL), GO_RIGHT, FALSE);
		if (qL) {
			bNoteL = FirstSubLINK(qL);
			for ( ; bNoteL; bNoteL=NextNOTEL(bNoteL))
				if (NoteVOICE(bNoteL)==NoteVOICE(aNoteL) && MainNote(bNoteL)) {
					bNote = GetPANOTE(bNoteL);
					if (bNote->tiedL) {
						AddNoteTies(doc, pL, qL, NoteSTAFF(aNoteL), NoteVOICE(aNoteL));
					}
					else
						MayErrMsg("CreateTies: aNoteL %ld tiedR with following note bNoteL %ld not tiedL",
										(long)aNoteL, (long)bNoteL);
				}
		}
		else
			MayErrMsg("CreateTies: aNoteL %ld tiedR with no following note in voice %ld",
							(long)aNoteL, (long)NoteVOICE(aNoteL));
	}
}


/* ---------------------------------------------------------------- CreateAllTies -- */
/* Assuming the given score has notes with their tiedR/tiedL flags set appropriately
but no corresponding Slur objects describing the ties, generate Slur objects for all
those notes. Does not check whether they already have Slurs! */
 
void CreateAllTies(Document *doc)
{
	LINK pL,aNoteL;
	
	for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (MainNote(aNoteL))
					CreateTies(doc, pL, aNoteL);
		}
}


/* ----------------------------------------------------------------- FlipSelSlur -- */
/* Flip the direction of every selected subobject of the given Slur object; return
TRUE if there are any, else FALSE. */

Boolean FlipSelSlur(LINK slurL)		/* Slur object */
{
	LINK		aSlurL;
	PASLUR	aSlur;
	DDIST		x, y;
	Boolean	flipped;

	flipped = FALSE;
	for (aSlurL = FirstSubLINK(slurL); aSlurL; aSlurL = NextSLURL(aSlurL))
		if (SlurSEL(aSlurL)) {
			flipped = TRUE;
			aSlur = GetPASLUR(aSlurL);
			
			/* Get 0-based vector from slur knot to endpoint */
			x = (p2d(aSlur->endPt.h)+aSlur->endpoint.h) -
				 (p2d(aSlur->startPt.h)+aSlur->seg.knot.h);
			y = (p2d(aSlur->endPt.v)+aSlur->endpoint.v) -
				 (p2d(aSlur->startPt.v)+aSlur->seg.knot.v);

			RotateSlurCtrlPts(aSlurL,x,y,-1);

			aSlur = GetPASLUR(aSlurL);
			aSlur->seg.c0.v *= -1;
			aSlur->seg.c1.v *= -1;

			RotateSlurCtrlPts(aSlurL,x,y,1);

			aSlur = GetPASLUR(aSlurL);
			aSlur->seg.knot.v *= -1;
			aSlur->endpoint.v *= -1;
		}

	return flipped;
}


/* ========================================================== Antikink and allies == */

typedef struct {
	LINK	link;
	DDIST	len;
} SLURINFO;

static SLURINFO *slurInfo,**slurInfoHdl;
static short lastSlur,maxSlurs;

/* If you move the endpoints of a Bezier curve much closer together, leaving the
control points in their same relative positions, the curve will tend to develop
kinks or even loops because the control points will cross. "Antikinking" is what
we call moving the control points closer to the endpoints to avoid this. */

Boolean AddSlurToList(LINK);
static DDIST SlurEndxd(LINK);

/*	Add the given slur to the antikinking list. */

Boolean AddSlurToList(LINK slurL)
{
	short i;
	char fmtStr[256];

	/* Find first free slot in list, which may be at lastSlur (end of list) */
	
	for (i = 0; i<lastSlur; i++)
		if (!slurInfo[i].link) break;
	
	/* Insert slur into free slot, or append to end of list if there's room */
	
	slurInfo[i].link = slurL;
	
	if (i<lastSlur || lastSlur++<maxSlurs) {
		slurInfo[i].link = slurL;
		return TRUE;
	}
	else {
		lastSlur--;			
		GetIndCString(fmtStr, SLURERRS_STRS, 6);    /* "Nightingale can antikink only %d slurs at once." */
		sprintf(strBuf, fmtStr, maxSlurs); 
		CParamText(strBuf,	"", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
}


/* Return the SysRelxd of the given LINK, which is assumed to be a slur endpoint:
if it's a System, it represents the right end of that System, otherwise it
represents itself. */

static DDIST SlurEndxd(LINK pL)
{
	PSYSTEM pSystem; DDIST xd;
	
	if (SystemTYPE(pL)) {
		pSystem = GetPSYSTEM(pL);
		xd = pSystem->systemRect.right;
	}
	else
		xd = SysRelxd(pL);
		
	return xd;
}

/* Do "antikinking" on slurs/ties that might be affected by changes to the systems
from the one containing startL to the one containing endL. Every call to InitAntiKink
must be paired with a call to Antikink, which will dispose of the handle allocated by
InitAntiKink. Return TRUE if all OK, FALSE if either there's an error or no slurs/ties
that might be affected. */

Boolean InitAntikink(Document *doc, LINK startL, LINK endL)
{
	LINK startChkL, endChkL, pL; short i,slurCount;
	DDIST leftEnd, rightEnd;
	
	lastSlur = 0;

	/* No slur before the beginning of startL's System could continue to startL. */
	
	startChkL = LSSearch(startL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	if (!startChkL) startChkL = startL;
	endChkL = EndSystemSearch(doc, endL);

	/*
	 * Get an upper bound on the number of slurs we might be handling and
	 * allocate a block to hold SLURINFOs for them.
	 */
	slurCount = 0;
	for (pL = startChkL; pL!=endChkL; pL = RightLINK(pL))
		if (SlurTYPE(pL)) slurCount++;

	if (slurCount==0) return FALSE;
	
	maxSlurs = slurCount;
	slurInfoHdl = (SLURINFO **)NewHandle((Size)sizeof(SLURINFO) * maxSlurs);
	if (!GoodNewHandle((Handle)slurInfoHdl)) {
		NoMoreMemory();
		return FALSE;
	}

	MoveHHi((Handle)slurInfoHdl);
	HLock((Handle)slurInfoHdl);
	slurInfo = *slurInfoHdl;

	for (pL = startChkL; pL!=startL; pL = RightLINK(pL)) {
		if (SlurTYPE(pL)) {
			if (!AddSlurToList(pL)) goto GiveUp;
		}
		else
			/* Remove from the list all slurs that end at pL. */
			for (i = 0; i<lastSlur; i++) {
				if (slurInfo[i].link)
					if (SlurLASTSYNC(slurInfo[i].link)==pL) slurInfo[i].link = NILINK;
				}
	}
	
	/*
	 * The list now contains all slurs that start before startL and continue to it
	 * or further. Add any that start between startL and endChkL, regardless of where
	 * they end.
	 */
	 for (pL = startL; pL!=endChkL; pL = RightLINK(pL))
		if (SlurTYPE(pL)) {
			if (!AddSlurToList(pL)) goto GiveUp;
		}
	
	/* Fill in all the slurs' current lengths. */
	
	for (i = 0; i<lastSlur; i++)
		if (slurInfo[i].link) {
			leftEnd = SlurEndxd(SlurFIRSTSYNC(slurInfo[i].link));
			rightEnd = SlurEndxd(SlurLASTSYNC(slurInfo[i].link));
			slurInfo[i].len = rightEnd-leftEnd;
		}

	return TRUE;
	
GiveUp:
	if (slurInfoHdl) {
		DisposeHandle((Handle)slurInfoHdl);
		slurInfoHdl = NULL;
	}
	return FALSE;
}
	

/* Apply antikinking to any slurs in the list that have been shortened. */

void Antikink()
{
	short i; DDIST leftEnd, rightEnd, newLen, dist;
	LINK aSlurL; PASLUR aSlur;
	SplineSeg seg; DPoint endpoint; FASTFLOAT scale;

	for (i = 0; i<lastSlur; i++)
		if (slurInfo[i].link) {
			leftEnd = SlurEndxd(SlurFIRSTSYNC(slurInfo[i].link));
			rightEnd = SlurEndxd(SlurLASTSYNC(slurInfo[i].link));
			newLen = rightEnd-leftEnd;
			if (newLen<slurInfo[i].len) {
				scale = (FASTFLOAT)newLen/(FASTFLOAT)slurInfo[i].len;

				aSlurL = FirstSubLINK(slurInfo[i].link);
				for ( ; aSlurL; aSlurL = NextSLURL(aSlurL)) {
					aSlur = GetPASLUR(aSlurL);
					seg = aSlur->seg;
					endpoint = aSlur->endpoint;

					dist = seg.c0.h-seg.knot.h;
					seg.c0.h = seg.knot.h+scale*dist;
					dist = seg.c1.h-endpoint.h;
					seg.c1.h = endpoint.h+scale*dist;

					dist = seg.c0.v-seg.knot.v;
					seg.c0.v = seg.knot.v+scale*dist;
					dist = seg.c1.v-endpoint.v;
					seg.c1.v = endpoint.v+scale*dist;
					
					aSlur->seg = seg;
				}
			}
		}

	if (slurInfoHdl) {
		DisposeHandle((Handle)slurInfoHdl);
		slurInfoHdl = NULL;
	}
}
