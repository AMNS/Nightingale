/***************************************************************************
*	FILE:	Slurs.c
*	PROJ:	Nightingale
*	DESC:	Slur screen drawing, editing, and antikinking routines.
/***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include <math.h>						/* For sqrt() prototype */

#define BOXSIZE 4						/* For editing handles (in pixels) */

static Boolean	searchSpline,			/* Suppress drawing and search for point instead */
				getBoundsSpline;		/* Suppress drawing and get bounding box instead */
static short	xSearch, ySearch;		/* Point to search for (screen coordinates) */
static short	slurLeft, slurRight,	/* Bounds to be found */
				slurBottom, slurTop;

/* Prototypes for local routines */

static Boolean	SameDPoint(DPoint p1, DPoint p2);
static Boolean	DoSlurMouseDown(Document *doc, DPoint pt, LINK pL, LINK aSlurL, short index);
static Boolean	EditSlur(Rect *paper, SplineSeg *seg, DPoint *endpt, DPoint pt, short how);
static Boolean	Slursor(Rect *paper, DPoint *start, DPoint *end, DPoint *c0, DPoint *c1,
						short type, DPoint *q, short curve);
static void		StartSearch(Rect *paper, DPoint pt);
static void		EndSearch(void);
static void		LineOrMoveTo(short, short, short, Boolean *, short *);
static Boolean	BezierTo(Point startPt, Point endPt, Point c0, Point c1, Boolean dashed);
static Boolean	DrawSegment(Rect *paper, SplineSeg *seg, DPoint endKnot);
static void		DeselectKnots(Rect *paper, LINK aSlurL);
static void		HiliteSlur(Rect *paper, LINK aSlurL);
static void		DrawDBox(Rect *paper, DPoint pt, short size);

static void		CreateTies(Document *doc, LINK pL, LINK aNoteL);

/* Nightingale models slurs with cubic Bezier curves. This is nice for compatibility
with PostScript, which also uses cubic Beziers (as does HTML5); but it also suits the
way slurs are used in CWMN in that a cubic Beizer curve can have at most one inflection
point. The vast majority of slurs -- say, 95% or more -- have no inflection points, and
the vast majority of the others -- again probably 95% or more -- have just one. As for
the small fraction of a percent that have more than one, as of version 5.7, Nightingale
can't handle them.

A Nightingale Slur object consists of a table of subobjects, each of which contains a
record consisting of one SplineSeg, where a SplineSeg is the coordinates of the start of
a spline, plus two control points.  All points are kept as pairs of DDISTs.  The end
of each segment would be the start of the next segment, and the end of the last segment
is kept separately in the record.  This is in case we someday implement "compound slurs",
i.e., slurs with more than one inflection point.

NB: There's much confusion about the terminology for the points that define Bezier
curves. Every Bezier has two endpoints, also called _anchor_ points or _knots_, where
the curve starts and ends. A cubic Bezier also has two _control points_, points that
affect the shape of the curve but which the curve rarely passes through. But the
endpoints are sometines also called control points, sometimes not! Herein we do not
consider the endpoints to be control points.

NB2: Perhaps of interest to the mathematically inclined: every Bezier curve is contained
within the convex hull of its (four, for cubic Beziers) defining points. */


/* Take the offsets stored in aSlur->seg.knot, etc; take the endpoints stored in
aSlur->startPt, endPt; add them to compute results to be returned in knot, c0, c1,
endPt; these points can then be passed directly to BezierTo, etc. Points returned are
in paper-relative DDISTs (not offsets from startPt, etc.)  */
 
void GetSlurPoints(LINK	aSlurL, DPoint *knot, DPoint *c0, DPoint *c1, DPoint *endKnot)
{
	SplineSeg *thisSeg;  PASLUR aSlur;
			
	aSlur = GetPASLUR(aSlurL);
	thisSeg = &aSlur->seg;

	SetDPt(knot, p2d(aSlur->startPt.h), p2d(aSlur->startPt.v));
	SetDPt(endKnot, p2d(aSlur->endPt.h), p2d(aSlur->endPt.v));
	aSlur = GetPASLUR(aSlurL);
	
	knot->v += thisSeg->knot.v;  knot->h += thisSeg->knot.h;
	endKnot->v += aSlur->endKnot.v;  endKnot->h += aSlur->endKnot.h;
					
	c0->v = knot->v + thisSeg->c0.v; c0->h = knot->h + thisSeg->c0.h; 
	c1->v = endKnot->v + thisSeg->c1.v; c1->h = endKnot->h + thisSeg->c1.h;
}

/* Check to see if two DPoints are within slop of each other. */

#define SLURDPOINT_SLOP pt2d(2)

static Boolean SameDPoint(DPoint p1, DPoint p2)
{
	short dx, dy;
	
	dx = p1.h - p2.h; if (dx < 0) dx = -dx;
	dy = p1.v - p2.v; if (dy < 0) dy = -dy;
	
	return (dx<=SLURDPOINT_SLOP && dy<=SLURDPOINT_SLOP);
}
	
/* Use a wide line to draw a filled box at a point. */

static void DrawDBox(Rect *paper, DPoint ptd, short size)
{
	ptd.h = d2p(ptd.h);  ptd.v = d2p(ptd.v);
	
	PenSize(size,size);  size >>= 1;
	MoveTo(paper->left + (ptd.h -= size), paper->top + (ptd.v -= size));
	LineTo(paper->left+ptd.h, paper->top+ptd.v);
	PenSize(1, 1);
}

/*
 *	Entertain events in a separate event loop for manipulating slur objects.
 *	While mouse clicks stay within the slur's objRect (bounding Box), we stay
 *	here.  If any other event, we leave here so that main event loop can deal
 *	with it.
 *
 *	Caution: This event loop is not considerate of other processes.
 */

void DoSlurEdit(Document *doc, LINK pL, LINK aSlurL, short index)
{
	EventRecord	evt;
	Boolean		stillEditing = True;
	DPoint		pt;
	Point		ptStupid;
	Rect		bboxBefore, bboxAfter;

	FlushEvents(everyEvent, 0);
	
	while (stillEditing) {
		ArrowCursor();
		if (EventAvail(everyEvent, &evt))
			switch (evt.what) {
				case mouseDown:
					ptStupid = evt.where;  GlobalToLocal(&ptStupid);
					pt.v = p2d(ptStupid.v - doc->currentPaper.top);
					pt.h = p2d(ptStupid.h - doc->currentPaper.left);
					GetSlurBBox(doc, pL, aSlurL, &bboxBefore, BOXSIZE);
					stillEditing = DoSlurMouseDown(doc, pt, pL, aSlurL, index);
					FlushEvents(mUpMask, 0);
			  if (stillEditing) {
				 EraseAndInval(&bboxBefore);
				 GetSlurBBox(doc, pL, aSlurL, &bboxAfter, BOXSIZE);
				 EraseAndInval(&bboxAfter);
			  }
					break;
				case updateEvt:
					DoUpdate((WindowPtr)(evt.message));
					if (stillEditing)
						HiliteSlur(&doc->currentPaper, aSlurL);
					break;
				default:
					stillEditing = False;
					break;
			}
	}
				
	DeselectKnots(&doc->currentPaper, aSlurL);
}

/*
 *	Deliver the bounding box of the given slur sub-object in window-relative coordinates
 *	(not DDISTs).  Enlarge bbox by margin (for including handles to control pts).
 */

void GetSlurBBox(Document *doc, LINK pL, LINK aSlurL, Rect *bbox, short margin)
{
	DPoint knot, c0, c1, endKnot;
	short j;
	LINK sL;
	Point startPt[MAXCHORD], endPt[MAXCHORD];
	PASLUR aSlur;
	
	GetSlurContext(doc, pL, startPt, endPt);			/* Get absolute positions, in DPoints */
	
	for (j=0, sL=FirstSubLINK(pL); sL!=aSlurL; j++, sL=NextSLURL(sL)) ;
	aSlur = GetPASLUR(aSlurL);
	aSlur->startPt = startPt[j];
	aSlur->endPt = endPt[j];

	GetSlurPoints(aSlurL, &knot, &c0, &c1, &endKnot);
	
	/* Get convex hull of 4 points */
	
	bbox->left = bbox->right = knot.h;
	bbox->top = bbox->bottom = knot.v;
	if (c0.h < bbox->left)				bbox->left = c0.h;
	else if (c0.h > bbox->right)		bbox->right = c0.h;
	if (c1.h < bbox->left)				bbox->left = c1.h;
	else if (c1.h > bbox->right)		bbox->right = c1.h;
	if (endKnot.h < bbox->left)			bbox->left = endKnot.h;
	else if (endKnot.h > bbox->right)	bbox->right = endKnot.h;
	 
	if (c0.v < bbox->top)				bbox->top = c0.v;
	else if (c0.v > bbox->bottom)		bbox->bottom = c0.v;
	if (c1.v < bbox->top)				bbox->top = c1.v;
	else if (c1.v > bbox->bottom)		bbox->bottom = c1.v;
	if (endKnot.v < bbox->top)			bbox->top = endKnot.v;
	else if (endKnot.v > bbox->bottom)	bbox->bottom = endKnot.v;
	
	D2Rect((DRect *)bbox,bbox);
	
	InsetRect(bbox,-margin,-margin);
	OffsetRect(bbox,doc->currentPaper.left,doc->currentPaper.top);
}

/*
 *	A slur is hilited by drawing a small box at each of its knots and at each of the
 *	spline segments' control points.  We use Xor mode so that pairs of calls hilite
 *	and unhilite without graphic interference.
 */

void HiliteSlur(Rect *paper, LINK aSlurL)
{
	SplineSeg *thisSeg;  short nSegs=1, i;
	DPoint knot, c0, c1, endKnot;
	short size = BOXSIZE;  PASLUR aSlur;
	
	if (aSlurL) {
		PenMode(patXor);
		aSlur = GetPASLUR(aSlurL);
		thisSeg = &aSlur->seg;
		for (i=1; i<=nSegs; i++,thisSeg++) {
			GetSlurPoints(aSlurL, &knot, &c0, &c1, &endKnot);
			
			DrawDBox(paper, knot, size);  DrawDBox(paper, endKnot, size);
			DrawDBox(paper, c0, size);  DrawDBox(paper, c1, size);
		}
		PenNormal();
	}
}


/*
 *	Designate the currently selected slur by graphically hiliting it.  If a selected
 *	slur already exists, unhilite it (since we're using Xor mode), and then hilite
 *	the new one.  The NULL slur is legal and just deselects.
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
 *	Deal with a mouse down event at DPoint pt.  Deliver True if mouse down
 *	is eaten and dealt with here, False if not dealt with.
 */

static Boolean DoSlurMouseDown(Document *doc, DPoint pt, LINK /*pL*/, LINK aSlurL,
									short /*index*/)
	{
		SplineSeg *thisSeg, seg; 
		short how = -1, nSegs=1, i;
		DPoint endKnot;
		static Boolean found, changed;
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
		
			if (i==nSegs)	endKnot = aSlur->endKnot;
			else			endKnot = (thisSeg+1)->knot;
			 
			GetSlurPoints(aSlurL, &seg.knot, &seg.c0, &seg.c1, &endKnot);
			
			/* Check if mouse falls in end knot points or control points */
			/* For compound spline curves, should check bBox first */
			
			if (SameDPoint(pt, seg.knot))	{ how = S_Start;  goto gotit; }
			if (SameDPoint(pt, seg.c0))		{ how = S_C0;  goto gotit; }
			if (SameDPoint(pt, seg.c1))		{ how = S_C1;  goto gotit; }
			if (SameDPoint(pt, endKnot))		{ how = S_End;  goto gotit; }
			
			/* Or along Bezier curve */
			
			StartSearch(&paper, pt);
			found = DrawSegment(&paper, &seg, endKnot);
			EndSearch();
			if (found) { how = S_Whole;  goto gotit; }
		}

		return(False);

gotit:
		/* Our mouse down event should be eaten and dealt with here */

		FlushEvents(mDownMask, 0);
		/*
		 *	Temporarily deselect for a whole knot drag, so as not to leave hanging
		 *	boxes on the screen.  Prepare for dragging a slursor
		 */
		HiliteSlur(&paper, aSlurL);						/* Erase old knot boxes */
		
		changed = EditSlur(&paper, &seg, &endKnot, pt, how);
		if (changed) {
			doc->changed = True;
			/* Convert paper-relative coords back to offsets */
			aSlur = GetPASLUR(aSlurL);
			aSlur->seg.c0.h = seg.c0.h - seg.knot.h;
			aSlur->seg.c0.v = seg.c0.v - seg.knot.v;
			aSlur->seg.c1.h = seg.c1.h - endKnot.h;
			aSlur->seg.c1.v = seg.c1.v - endKnot.v;
			aSlur->seg.knot.h = seg.knot.h - p2d(aSlur->startPt.h); 
			aSlur->seg.knot.v = seg.knot.v - p2d(aSlur->startPt.v); 
			aSlur->endKnot.h = endKnot.h - p2d(aSlur->endPt.h);
			aSlur->endKnot.v = endKnot.v - p2d(aSlur->endPt.v);
			}
		
		DrawSegment(&paper, &seg, endKnot);
		return(True);
	}

/*
 *	Given a slur segment of the selected slur, in which a mouse click has specified the
 *	indicated part, interactively change that part.  Deliver True if an actual change
 *	occurred, False if not.
 */

static Boolean EditSlur(Rect *paper, SplineSeg *seg, DPoint *endKnot, DPoint pt, short how)
{
	DPoint oldStart={0,0},
			oldc0={0,0},
			oldc1={0,0},
			oldEnd={0,0};
	Boolean changed=False;

	/* Track user action and get new position */
	Slursor(paper, &seg->knot, endKnot, &seg->c0, &seg->c1, how, &pt, 0);
	
	changed = (oldStart.h!=seg->knot.h || oldStart.v!=seg->knot.v ||
					oldc0.h!=seg->c0.h || oldc0.v!=seg->c0.v ||
					oldc1.h!=seg->c1.h || oldc1.v!=seg->c1.v ||
					oldEnd.h!=endKnot->h || oldEnd.v!=endKnot->v);

	return changed;
}

/*
 *	Draw a given spline segment with anchor point at seg->knot to the given endpoint.
 *	Return whether or not the current search point was encountered while "drawing"
 *	(see BezierTo).
 */

Boolean DrawSegment(Rect *paper, SplineSeg *seg, DPoint endKnot)
{
	Point start,c0,c1,end;
	
	start.h = paper->left + d2p(seg->knot.h);
	start.v = paper->top + d2p(seg->knot.v);
	end.h = paper->left + d2p(endKnot.h);
	end.v = paper->top + d2p(endKnot.v);
	c0.h = paper->left + d2p(seg->c0.h);
	c0.v = paper->top + d2p(seg->c0.v);
	c1.h = paper->left + d2p(seg->c1.h);
	c1.v = paper->top + d2p(seg->c1.v);
	
	MoveTo(start.h,start.v);
	return(BezierTo(start,end,c0,c1,False));
}

	
/*
 *	Here we create or update a movable slur spline section, between start and end with
 *	control points c0 and c1.  If type is S_New or S_Extend, we initialise end, c0, and
 *	c1 here; otherwise we use the provided arguments.  When we're done, the current
 *	spline (in DDIST coordinates) is returned in end, c0, and c1 (usually anchored at
 *	start).  q points to the original click point (which is not a control point if type
 *	== S_Whole). When we drag an endpoint, the associated control point follows in
 *	parallel.
 *	
 *	If the Shift key is held down, we constrain motion to whichever axis was used first
 *	(this is the conventional meaning of the Shift key when dragging in Mac graphics
 *	programs). If the Option key is down, force the opposite control point or endpoint to
 *	move symmetrically. Finally, we deliver True if the resulting spline is at least a
 *	certain size; otherwise False.
 */

#define SLURDRAG_SLOP 2		/* in pixels */
#define MIN_GOOD_SIZE 4		/* in pixels */

static DPoint keepStart, keepEnd, keepc0, keepc1;
static Rect keepPaper;
static short keepType;

Boolean Slursor(Rect *paper, DPoint *start, DPoint *end, DPoint *c0, DPoint *c1,
					short type, DPoint *q, short curve)
{
	DPoint ptd, tmp, *test; 
	DDIST dist, dist4, dist8, dx, dy;
	Boolean first=True, up=True, constrainToAxis, constrainSymmetric, horiz=False,
				stillWithinSlop;
	Point pt, origPt;
	short xdiff, ydiff;
	
	if (type == S_Extend) type = S_New;						/* Not implemented: use New */
	
	if (type == S_New) *end = *c0 = *c1 = *start;
	 else			   up = curve>0 ? True : (curve<0 ? False : (c0->v >= start->v));
			
	PenMode(patXor);										/* Everything will be drawn twice */
	DrawSlursor(paper, start, end, c0, c1, type, False);	/* Draw first one in before loop */

	 /* Choose which point we want to follow the mouse */
	 
	switch (type) {
		case S_New:		test = end; break;
		case S_Start:	test = start; tmp = *start; break;
		case S_C0:		test = c0; break;
		case S_C1:		test = c1; break;
		case S_End:		test = end; tmp = *end; break;
		case S_Whole:	test = q; tmp = *q; break;
		default:		return False;						/* Should never happen */
	}
	
	theSelectionType = SLURSOR;
	constrainToAxis = ShiftKeyDown();
	constrainSymmetric = OptionKeyDown();
	stillWithinSlop = True;
	GetPaperMouse(&origPt, paper);

	if (StillDown())
		while (WaitMouseUp()) {
			GetPaperMouse(&pt, paper);
			if (constrainToAxis && stillWithinSlop) {
				xdiff = pt.h - origPt.h; ydiff = pt.v - origPt.v;
				
				/* If we're still within slop bounds, don't do anything */
				if (ABS(xdiff)<SLURDRAG_SLOP && ABS(ydiff)<SLURDRAG_SLOP) continue;
				horiz = ABS(xdiff) > ABS(ydiff);      /* 45 degree movement => vertical */
				stillWithinSlop = False;
			}

			ptd.h = p2d(pt.h); ptd.v = p2d(pt.v);
			if (test->h!=ptd.h || test->v!=ptd.v) {
				/* Don't flicker if the axis we care about hasn't changed. */
				if (constrainToAxis) {
					if (horiz && test->h==ptd.h || !horiz && test->v==ptd.v)
						continue;
				}
				if (first) if (type==S_New || type==S_Extend) {
					up = curve>0 ? True : (curve<0 ? False : ptd.v<=end->v);
					first = False;
				}
				
				DrawSlursor(paper,start,end,c0,c1,type,False);	/* Erase old slursor */
				GetPaperMouse(&pt,paper);						/* Get the most up-to-date position */

				/* If we're constraining to horiz. or vertical, discard change on other axis */
				if (constrainToAxis) {
					if (horiz)	pt.v = d2p(test->v);
					else		pt.h = d2p(test->h);
				}
				
				test->h = p2d(pt.h);  test->v = p2d(pt.v);
				
				switch (type) {
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
						if (constrainSymmetric) {
							c1->h -= dx; c1->v += dy;
							end->h -= dx; end->v += dy;
						}
						tmp = *test;
						break;
					case S_C0:
						if (constrainSymmetric) {
							dx = test->h - start->h;
							dy = test->v - start->v;
							c1->h = end->h - dx;
							c1->v = end->v + dy;
						}
						break;
					case S_C1:
						if (constrainSymmetric) {
							dx = test->h - end->h;
							dy = test->v - end->v;
							c0->h = start->h - dx;
							c0->v = start->v + dy;
						}
						break;
					case S_End:
						dx = test->h - tmp.h; dy = test->v - tmp.v;
						c1->h += dx; c1->v += dy;
						if (constrainSymmetric) {
							c0->h -= dx; c0->v += dy;
							start->h -= dx; start->v += dy;
						}
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
					
				DrawSlursor(paper,start,end,c0,c1,type,False);		/* Draw new section */
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
	DrawSlursor(paper,start,end,c0,c1,type,False);
	PenNormal();
	theSelectionType = 0;
	
	 /* Now determine if spline is large enough to be good (4 pixels on each axis) */
	dist = p2d(MIN_GOOD_SIZE);
	dx = ABS(start->h - end->h); dy = ABS(start->v-end->v);
	return (dx>dist || dy>dist);
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
	DrawSlursor(&keepPaper,&keepStart,&keepEnd,&keepc0,&keepc1,keepType,False);
}

/*
 *	Draw a Bezier spline from startPt to endPt, using c0, c1 as control Points.
 *	This version is a fast version for integer coordinates, done in homebrew Fixed
 *	arithmetic, suitable for changing cursors, etc.
 *	If searchSpline is True, don't draw; rather check for proximity to the point
 *	(xSearch,ySearch), and return True if point is "close" to the spline.
 *	If getBoundsSpline is True, maximize/minimize to find bounding box.
 */

void StartSearch(Rect *paper, DPoint pt)
{
	searchSpline = True;
	xSearch = paper->left + d2p(pt.h); ySearch = paper->top + d2p(pt.v);
}

void EndSearch()
{
	searchSpline = False;
}
	
Boolean FindSLUR(Rect *paper, Point pt, LINK aSlurL)
{
	DPoint ptd,endKnot;  short i,nSegs=1;  PASLUR aSlur;
	SplineSeg *thisSeg,seg;  Boolean found = False;
	
	ptd.h = p2d(pt.h); ptd.v = p2d(pt.v);
	StartSearch(paper,ptd);
	
	aSlur = GetPASLUR(aSlurL);
	for (i=1,thisSeg = &aSlur->seg; i<=nSegs; i++,thisSeg++) {
		if (i == nSegs)	endKnot = aSlur->endKnot;
		 else			endKnot = (thisSeg+1)->knot;
		GetSlurPoints(aSlurL,&seg.knot,&seg.c0,&seg.c1,&endKnot);
		if (DrawSegment(paper,&seg,endKnot)) { found = True; break; }		/* In search mode */
		}
	EndSearch();
	return found;
}

void StartSlurBounds()
{
	searchSpline = False;
	getBoundsSpline = True;
	slurLeft = slurTop = 32767;
	slurRight = slurBottom = -32767;
}

void EndSlurBounds(Rect *paper, Rect *box)
{
	SetRect(box,slurLeft,slurTop,slurRight,slurBottom);
	OffsetRect(box,-paper->left,-paper->top);				/* Convert box to paper coords */
	getBoundsSpline = False;
}

/*
 * Draw next segment of dashed line: alternate between a series of dash segments
 *	(LineTo's) and a series of space segments (MoveTo's).
 */
 
static void LineOrMoveTo(short x, short y, short segsPerDash, Boolean *pDoingDash,
								short *pSegsThisDash)
{
	if (*pDoingDash)	LineTo(x,y);
	else				MoveTo(x,y);
	
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
		Boolean found = False;
		static short epsilon = 5;
		FASTFLOAT segLen;  short segsPerDash = 1,segsThisDash;
		Boolean doingDash;

		curx = startPt.h; cury = startPt.v;
		
		lastx = curx-1;										/* Force first draw */

		/*	Compute the integer Bezier spline coefficients, a, b, and c. */

		cx = (c0.h - curx); cx += cx << 1;					/* times 3 */
		cy = (c0.v - cury); cy += cy << 1;
		
		bx = (c1.h - c0.h); bx += (bx << 1) - cx;			/* times 3 - c */
		by = (c1.v - c0.v); by += (by << 1) - cy;
		
		ax = (endPt.h - curx) - cx - bx;
		ay = (endPt.v - cury) - cy - by;


		s = 32; s1 = 5;								/* Number of lines in spline, and bits */
		if (outputTo==toScreen) {
			if (config.fastScreenSlurs==1)
				{ s = 16; s1 = 4; }					/* Number of lines in spline, and bits */
			if (config.fastScreenSlurs==2)
				{ s = 8; s1 = 3; }					/* Number of lines in spline, and bits */
		}

		s2 = s1+s1; s3 = s2+s1;						/* s3 is 15 bits worth of scaling */
		
		bx   <<= s1;   by <<= s1;					/* Scale operands up for later according */
		cx   <<= s2;   cy <<= s2;					/*   to the degree in i in loop below */
		curx <<= s3; cury <<= s3;

		if (dashed) {
			/*
			 * Try to find the number of segments that will make dashes (and spaces) as
			 * close as possible to DASHLEN pixels. The following doesn't do a very
			 * good job. Also, it would be better to make dashes and spaces on the screen
			 * agree with the PostScript versions, so their lengths should be independent
			 * of magnification.
			 */
			segLen = ABS(endPt.h-startPt.h)/(FASTFLOAT)s;
			segsPerDash = DASHLEN/segLen;
			if (segsPerDash<1) segsPerDash = 1;
			doingDash = True;
			segsThisDash = segsPerDash;
		}
		
		if (searchSpline)
			for (i=0; i<=s; i++) {
				x = (i * (i * (i * ax + bx) + cx) + curx) >> s3;
				y = (i * (i * (i * ay + by) + cy) + cury) >> s3;
				if (lastx!=x || lasty!=y) {							/* ?? lasty NEVER INIT.! */
					dx = x - xSearch; if (dx < 0) dx = -dx;
					dy = y - ySearch; if (dy < 0) dy = -dy;
					if (dx<=epsilon && dy<=epsilon) { found = True; break; }
					lastx = x; lasty = y;
					}
				}

		 else if (getBoundsSpline)
			for (i=0; i<=s; i++) {
				x = (i * (i * (i * ax + bx) + cx) + curx) >> s3;
				y = (i * (i * (i * ay + by) + cy) + cury) >> s3;
				if (lastx!=x || lasty!=y) {
					if (dashed)	LineOrMoveTo(x,y,segsPerDash,&doingDash,&segsThisDash);
					else		LineTo(x,y);
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

					if (dashed)	LineOrMoveTo(lastx=x,lasty=y,segsPerDash,
												&doingDash,&segsThisDash);
					else		LineTo(lastx=x,lasty=y);
					}
				}
	
		return(found);
  }

/*
 *	TrackSlur lets the user pull around a slur, anchored at DPoint ptd, and then,
 *	when the user lets go of the mouse, delivers the paper-relative coordinates
 *	(in DPoints) of the slur.  It returns False if the slur is below a certain
 *	minimum length.
 */

Boolean TrackSlur(Rect *paper, DPoint ptd, DPoint *knot, DPoint *endKnot, DPoint *c0,
					DPoint *c1, Boolean curveUp)
	{
		*knot = ptd; *endKnot = ptd; *c0 = ptd; *c1 = ptd;			/* Default: degenerate */
		
		return Slursor(paper, knot, endKnot, c0, c1, S_New, &ptd, (curveUp? 1:-1));
	}


/* ------------------------------------------------------------------ GetSlurNoteLinks -- */
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

/* -------------------------------------------------------------------- GetSlurContext -- */
/* Given a Slur object, return arrays of the paper-relative starting and ending
positions (expressed in points) of the notes delimiting its subobjects. */

void GetSlurContext(Document *doc, LINK pL, Point startPt[], Point endPt[])
	{
		CONTEXT localContext;
		DDIST	dFirstLeft, dFirstTop, dLastLeft, dLastTop,
				xdFirst, xdLast, ydFirst, ydLast;
		PANOTE 	firstNote, lastNote;
		PSLUR	p;
		LINK	firstNoteL, lastNoteL, aSlurL, firstSyncL, lastSyncL, systemL;
		PSYSTEM	pSystem;
		short 	k, xpFirst, xpLast, ypFirst, ypLast, firstStaff, lastStaff;
		Boolean	firstMEAS, lastSYS;

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

		firstSyncL = (firstMEAS ? LSSearch(p->firstSyncL, SYNCtype, firstStaff, GO_RIGHT, False)
										: p->firstSyncL);

		p = GetPSLUR(pL);
		if (lastSYS) {
			systemL = p->lastSyncL;
			lastSyncL = LSSearch(LinkRSYS(systemL),SYNCtype,lastStaff,GO_LEFT,False);
		}
		else
			lastSyncL = p->lastSyncL;

		GetContext(doc, firstSyncL, firstStaff, &localContext);
		dFirstLeft = localContext.measureLeft;							/* abs. origin of left end coords. */
		dFirstTop = localContext.measureTop;
		
		GetContext(doc, lastSyncL, lastStaff, &localContext);
		if (!lastSYS)
			dLastLeft = localContext.measureLeft;						/* abs. origin of right end coords. */
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

/* ------------------------------------------------------------------- HiliteSlurNodes -- */

void HiliteSlurNodes(Document *doc, LINK pL)
{
	PSLUR p;
	PenState pn;
	
	GetPenState(&pn);
	PenMode(patXor);

	/* We don't flash here because it's unnecessary (the slur's staff is obvious
		in any reasonable situation) and it slows down a common operation. */
	
	p = GetPSLUR(pL); InvertSymbolHilite(doc, p->firstSyncL, p->staffn, False);	/* Hiliting on... */
	p = GetPSLUR(pL); InvertSymbolHilite(doc, p->lastSyncL, p->staffn, False);
	SleepTicks(HILITE_TICKS);

	while (Button()) ;

	p = GetPSLUR(pL); InvertSymbolHilite(doc, p->firstSyncL, p->staffn, False);	/* Off. */
	p = GetPSLUR(pL); InvertSymbolHilite(doc, p->lastSyncL, p->staffn, False);

	SetPenState(&pn);
}

void PrintSlurPoints(LINK aSlurL, char */*str*/)
{
	PASLUR aSlur;
	
	aSlur = GetPASLUR(aSlurL);
}

/* ------------------------------------------------------------------ SetSlurCtlPoints -- */
/*	Given a slur or tie, set its Bezier control points. NB: This routine assumes the
slur/ties' startPt and endPt describe the positions of the notes they're attached to,
which doesn't sound like a very good idea; why not use the notes' actual positions? */

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
	DDIST		noteWidth=0, span, dLineSp;
	DDIST		x,y,x0,y0,x1,y1,tmp;
	FASTFLOAT	t;
	PASLUR		aSlur;
	//LINK		pL,aNoteL;
	//PANOTE	aNote;
	//short 	type,stemUp,accent,beam,degree;
	//DDIST		dTop,dBot,dLeft,xd,yd,stemLength;
	//DDIST		xoffFirst,xoffLast,yoffFirst,yoffLast;
	long		ltemp;
	
	/* Get horizontal offsets from note positions to slur ends. If it is a
		crossSystem slur, either firstSync or lastSync will not be a sync; in
		this case, make the respective noteWidth equal zero. */
	
	if (SyncTYPE(firstSyncL))
		noteWidth = std2d(SymWidthRight(doc, firstSyncL, staff, True),
									context.staffHeight, context.staffLines);

	aSlur = GetPASLUR(aSlurL);
	aSlur->seg.knot.h = (2*noteWidth)/3;

	noteWidth = 0;
	if (SyncTYPE(lastSyncL))
		noteWidth = std2d(SymWidthRight(doc, lastSyncL, staff, True),
									context.staffHeight, context.staffLines);

	aSlur = GetPASLUR(aSlurL);
	aSlur->endKnot.h = noteWidth/6;
	
	span = (p2d(aSlur->endPt.h)+aSlur->endKnot.h) -
			 (p2d(aSlur->startPt.h)+aSlur->seg.knot.h);
	dLineSp = std2d(STD_LINEHT, context.staffHeight, context.staffLines);
	aSlur->seg.knot.v = aSlur->endKnot.v = curveUp ? -dLineSp : dLineSp;

PrintSlurPoints(aSlurL, "SetSlurCtlPts: 1");

	/*
	 *	We now set the control points, which affect curvature.  This eventually wants
	 *	to be done using something like the Ohio slurs algorithms, but in the meantime,
	 *	we use the following methods, one for short spans and one for long spans, with
	 *	a linear blending between the two for medium spans.
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

	/* c0 is the offset from the left slur end to the left control point, c1 the offset
	from the right slur end to the right control point; but they are only symmetric
	about the vertical in the case where the two endpoints of the slur/tie are at the
	same height.  Therefore, we need to rotate the control point offset vectors to keep
	the slur symmetric. */

	aSlur = GetPASLUR(aSlurL);
	
	/* Get 0-based vector from slur knot to endKnot */
	x = (p2d(aSlur->endPt.h)+aSlur->endKnot.h) -
		 (p2d(aSlur->startPt.h)+aSlur->seg.knot.h);
	y = (p2d(aSlur->endPt.v)+aSlur->endKnot.v) -
		 (p2d(aSlur->startPt.v)+aSlur->seg.knot.v);

	RotateSlurCtrlPts(aSlurL,x,y,1);

	return True;
}


/* ----------------------------------------------------------------- RotateSlurCtrlPts -- */
/* Given a slur, rotate its control points by the angle of the vector (vx,vy).  Do it
counterclockwise if ccw is 1, clockwise if ccw is -1.  */

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

/* ------------------------------------------------------------------------ CreateTies -- */
/* Create a set of ties for aNoteL and all other notes in its voice in pL that have
corresponding notes in the next Sync in its voice that are tiedL. */

static void CreateTies(Document *doc, LINK pL, LINK aNoteL) 
{
	LINK qL, bNoteL; PANOTE aNote, bNote;
	
	aNote = GetPANOTE(aNoteL);
	if (aNote->tiedR) {
		qL = LVSearch(RightLINK(pL), SYNCtype, NoteVOICE(aNoteL), GO_RIGHT, False);
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


/* --------------------------------------------------------------------- CreateAllTies -- */
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


/* --------------------------------------------------------------------- DeleteSlurTie -- */
/* Delete the given slur object, which must be of subtype <tie>, and clear the tiedL
and tiedR flags of notes it connects. If the slur is cross-system, also delete its
other piece. */

Boolean DeleteSlurTie(Document *doc, LINK slurL)
{
	LINK	firstSyncL, lastSyncL, otherSlurL;
	Boolean	lastIsSys, firstIsMeas, left, right;
	
	if (!SlurTIE(slurL)) return False;
	
	right = True; left = False;
	lastIsSys = firstIsMeas = False;
	
	FixAccsForNoTie(doc, slurL);
	
	/* Clear tiedL/tiedR flags of notes for this slur. */
	
	firstSyncL = SlurFIRSTSYNC(slurL);
	if (MeasureTYPE(firstSyncL))
		firstIsMeas = True;
	else
		FixSyncForSlur(firstSyncL, SlurVOICE(slurL), True, right);
		
	lastSyncL = SlurLASTSYNC(slurL);
	if (SystemTYPE(lastSyncL))
		lastIsSys = True;
	else
		FixSyncForSlur(lastSyncL, SlurVOICE(slurL), True, left);
		
	/* If cross-system, clear tiedL/tiedR flags of notes for slur's other piece. */

	if (firstIsMeas) {
		otherSlurL = XSysSlurMatch(slurL);
		lastSyncL = SlurLASTSYNC(otherSlurL);
		if (SystemTYPE(lastSyncL)) {
			firstSyncL = SlurFIRSTSYNC(otherSlurL);
			FixSyncForSlur(firstSyncL, SlurVOICE(slurL), True, right);
		}
	}
	
	if (lastIsSys) {
		otherSlurL = XSysSlurMatch(slurL);
		firstSyncL = SlurFIRSTSYNC(otherSlurL);
		if (MeasureTYPE(firstSyncL)) {
			lastSyncL = SlurLASTSYNC(otherSlurL);
			FixSyncForSlur(lastSyncL, SlurVOICE(slurL), True, left);
		}
	}
	
	if (firstIsMeas || lastIsSys) 
		InvalMeasure(otherSlurL, SlurSTAFF(otherSlurL));

	/* Finally, delete the slur object(s). */
	
	DeleteNode(doc, slurL);
	if ((firstIsMeas || lastIsSys) && otherSlurL) DeleteNode(doc, otherSlurL);
	
	return True;
}


/* ----------------------------------------------------------------------- FlipSelSlur -- */
/* Flip the direction of every selected subobject of the given Slur object; return
True if there are any, else False. */

Boolean FlipSelSlur(LINK slurL)		/* Slur object */
{
	LINK	aSlurL;
	PASLUR	aSlur;
	DDIST	x, y;
	Boolean	flipped;

	flipped = False;
	for (aSlurL = FirstSubLINK(slurL); aSlurL; aSlurL = NextSLURL(aSlurL))
		if (SlurSEL(aSlurL)) {
			flipped = True;
			aSlur = GetPASLUR(aSlurL);
			
			/* Get 0-based vector from slur knot to endKnot */
			x = (p2d(aSlur->endPt.h)+aSlur->endKnot.h) -
				 (p2d(aSlur->startPt.h)+aSlur->seg.knot.h);
			y = (p2d(aSlur->endPt.v)+aSlur->endKnot.v) -
				 (p2d(aSlur->startPt.v)+aSlur->seg.knot.v);

			RotateSlurCtrlPts(aSlurL,x,y,-1);

			aSlur = GetPASLUR(aSlurL);
			aSlur->seg.c0.v *= -1;
			aSlur->seg.c1.v *= -1;

			RotateSlurCtrlPts(aSlurL,x,y,1);

			aSlur = GetPASLUR(aSlurL);
			aSlur->seg.knot.v *= -1;
			aSlur->endKnot.v *= -1;
		}

	return flipped;
}


/* =============================================================== Antikink and allies == */
/* If you move the endpoints of a Bezier curve much closer together, leaving the
control points in their same relative positions, the curve will tend to develop
kinks or even loops because the control points will cross. To avoid this, we "antikink"
by moving the control points closer to the endpoints. */

typedef struct {
	LINK	link;
	DDIST	len;
} SLURINFO;

static SLURINFO *slurInfo,**slurInfoHdl;
static short lastSlur,maxSlurs;

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
		return True;
	}
	else {
		lastSlur--;			
		GetIndCString(fmtStr, SLURERRS_STRS, 6);    /* "Nightingale can antikink only %d slurs at once." */
		sprintf(strBuf, fmtStr, maxSlurs); 
		CParamText(strBuf,	"", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
}


/* Return the SysRelxd of the given LINK, which is assumed to be a slur endpoint: if it's
a System, it represents the right end of that System, otherwise it represents itself. */

static DDIST SlurEndxd(LINK pL)
{
	PSYSTEM pSystem;  DDIST xd;
	
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
InitAntiKink. Return True if all OK, False if either there's an error or no slurs/ties
that might be affected. */

Boolean InitAntikink(Document *doc, LINK startL, LINK endL)
{
	LINK startChkL, endChkL, pL;  short i,slurCount;
	DDIST leftEnd, rightEnd;
	
	lastSlur = 0;

	/* No slur before the beginning of startL's System could continue to startL. */
	
	startChkL = LSSearch(startL, SYSTEMtype, ANYONE, GO_LEFT, False);
	if (!startChkL) startChkL = startL;
	endChkL = EndSystemSearch(doc, endL);

	/*
	 * Get an upper bound on the number of slurs we might be handling and allocate
	 * a block to hold SLURINFOs for them.
	 */
	slurCount = 0;
	for (pL = startChkL; pL!=endChkL; pL = RightLINK(pL))
		if (SlurTYPE(pL)) slurCount++;

	if (slurCount==0) return False;
	
	maxSlurs = slurCount;
	slurInfoHdl = (SLURINFO **)NewHandle((Size)sizeof(SLURINFO) * maxSlurs);
	if (!GoodNewHandle((Handle)slurInfoHdl)) {
		NoMoreMemory();
		return False;
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

	return True;
	
GiveUp:
	if (slurInfoHdl) {
		DisposeHandle((Handle)slurInfoHdl);
		slurInfoHdl = NULL;
	}
	return False;
}
	

/* Apply antikinking to any slurs in the list that have been shortened. */

void Antikink()
{
	short i;
	DDIST leftEnd, rightEnd, newLen, dist;
	LINK aSlurL; PASLUR aSlur;
	SplineSeg seg;
	DPoint endKnot;
	FASTFLOAT scale;

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
					endKnot = aSlur->endKnot;

					dist = seg.c0.h-seg.knot.h;
					seg.c0.h = seg.knot.h+scale*dist;
					dist = seg.c1.h-endKnot.h;
					seg.c1.h = endKnot.h+scale*dist;

					dist = seg.c0.v-seg.knot.v;
					seg.c0.v = seg.knot.v+scale*dist;
					dist = seg.c1.v-endKnot.v;
					seg.c1.v = endKnot.v+scale*dist;
					
					aSlur->seg = seg;
				}
			}
		}

	if (slurInfoHdl) {
		DisposeHandle((Handle)slurInfoHdl);
		slurInfoHdl = NULL;
	}
}
