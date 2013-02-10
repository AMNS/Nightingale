/* MasterMouse.c for Nightingale - handle mouse in Master Page */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/* The routines in this module are called upon when user clicks mouse in the Master
Sheet. We allow her to drag margin lines, etc. to directly change various attributes
that should be applied to every sheet in the document, and also to select things
to be modified, mostly by menu commands. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define SLOP	1		/* pixels */
#define GAP		4		/* pixels */

enum {
	LMARG,
	TMARG,
	RMARG,
	BMARG
};

static void sprintfInches(char *, short);
static void DrawMarginParams(Document *doc, short whichMarg, short marg, short d);
static Boolean EditDocMargin(Document *doc, Point pt, short modifiers, short doubleClick);
static void DoMasterDblClick(Document *doc, Point pt, short modifiers);
static void SelectMasterRectangle(Document *doc, Point pt);
static void DoMasterObject(Document *, Point, short modifiers);


static void sprintfInches(char *label, short points)
{
	long value;
	char fmtStr[256];
	
	value = 100L*pt2in(points);
	GetIndCString(fmtStr, MPGENERAL_STRS, 1);						/* "%s = %ld.%02ld in." */
	sprintf(strBuf, fmtStr, label, value/100L, value%100L);
}


/* ------------------------------------------------------------ NSysOnMasterPage -- */
/* Returns number of copies of the Master Page's system that fit on a page. */

short NSysOnMasterPage(Document *);
short NSysOnMasterPage(Document *doc)
{
	LINK sysL; DRect sysRect;
	DDIST sysHeight, sysOffset;
	short count;

	/* Count the single "real" master system. */
	
	count = 1;

	sysL = LSSearch(doc->masterHeadL, SYSTEMtype, ANYONE, GO_RIGHT, FALSE);
	sysRect = SystemRECT(sysL);

	sysHeight = SystemRECT(sysL).bottom - SystemRECT(sysL).top;
	sysOffset = sysHeight;
	
	/*
	 * Count as many additional copies of the master system as fit on a page. Be
	 *	careful of overflow.
	 */
	if ((long)SystemRECT(sysL).bottom+(long)sysOffset<=32767L) {
		OffsetDRect(&SystemRECT(sysL), 0, sysOffset);
		while (MasterRoomForSystem(doc,sysL)) {
			count++;
			if ((long)SystemRECT(sysL).bottom+(long)sysOffset>32767L) break;
			OffsetDRect(&SystemRECT(sysL), 0, sysOffset);
		}
	}

	/* Restore the real SystemRECT. */
	
	SystemRECT(sysL) = sysRect;	
	return count;
}


void MPDrawParams(Document *doc,
						LINK obj, LINK /*subObj*/,
						short /*param*/, short /*d*/)			/* Both unused */
	{
		char fmtStr[256];
				
		switch (ObjLType(obj)) {
			case SYSTEMtype:
				GetIndCString(fmtStr, MPGENERAL_STRS, 2);		/* "%d systems per default page" */
				sprintf(strBuf, fmtStr, NSysOnMasterPage(doc));
				break;
			default:
				*strBuf = '\0';
				break;
			}

		DrawMessageString(doc, strBuf);
	}


static void DrawMarginParams(Document *doc,
										short whichMarg, short marg,
										short /*d*/)						/* Unused */
	{
		char fmtStr[256];
		
		switch (whichMarg) {
			case LMARG:
				GetIndCString(fmtStr, MPGENERAL_STRS, 3);				/*	"Left margin" */
				break;
			case TMARG:
				GetIndCString(fmtStr, MPGENERAL_STRS, 4);				/*	"Top margin" */
				break;
			case RMARG:
				GetIndCString(fmtStr, MPGENERAL_STRS, 5);				/*	"Right margin" */
				break;
			case BMARG:
				GetIndCString(fmtStr, MPGENERAL_STRS, 6);				/*	"Bottom margin" */
				break;
		}
		sprintfInches((char *)fmtStr, p2pt(marg));

		DrawMessageString(doc, strBuf);
	}


/* If the user is editing the Master Page, find out here if she is editing the
document margin; if so, edit it here, and return TRUE; otherwise return FALSE. */

static Boolean EditDocMargin(Document *doc, Point pt, short /*modifiers*/, short /*doubleClick*/)
	{
		Boolean didSomething = FALSE; Point oldPt;
		Rect topMargin,leftMargin,rightMargin,bottomMargin,margin;		/* in pixels */
		Rect origMarginRect;															/* in points */
		CursHandle upDownCursor,rightLeftCursor;
		short *dragVal,oldVal,dx,dy,minVal,maxVal;
		Boolean horiz=FALSE,vert=FALSE,moved;
		Boolean inTop=FALSE,inBottom=FALSE,inRight=FALSE,inLeft=FALSE;
		short whichMarg, margv, margh, hdiff, vdiff;
		
		/*
		 * Compute <margin> in paper-relative pixels. Use that to define the
		 *	bounding boxes of the margin lines, whose positions on the screen
		 *	should have been shown by a call like FrameRect(&doc->marginRect).
		 *	We use these bounding boxes to determine if the user has clicked
		 *	on any of the four sides of the margin box.
		 */
		
		oldPt = pt;
		origMarginRect = doc->marginRect;
		margin = origMarginRect;
		MagnifyPaper(&margin,&margin,doc->magnify);
		
		topMargin = margin;
		topMargin.bottom = topMargin.top + 1;
		
		leftMargin = margin;
		leftMargin.right = leftMargin.left + 1;
		
		bottomMargin = margin;
		bottomMargin.top = bottomMargin.bottom - 1;
		
		rightMargin = margin;
		rightMargin.left = rightMargin.right - 1;
		
		/* Add a slop factor for each rectangle */
		
		InsetRect(&topMargin,0,-SLOP); InsetRect(&bottomMargin,0,-SLOP);
		InsetRect(&leftMargin,-SLOP,0); InsetRect(&rightMargin,-SLOP,0);
		
		/* During dragging we work in window coordinates instead of paper */
		
		OffsetRect(&margin,doc->currentPaper.left,doc->currentPaper.top);
		
		/*
		 *	Check to see if point is within our sloppy margin rectangles.  If so,
		 *	then pull in the applicable cursor, but muck with its hotspot first
		 *	so that user sees it drawn directly on margin line.  We therefore have
		 *	to muck with a copy of the cursor.
		 */
		
		if ((inTop=PtInRect(pt,&topMargin)) || (inBottom=PtInRect(pt,&bottomMargin))) {
			whichMarg = inTop ? TMARG : BMARG;
			upDownCursor = GetCursor(DragUpDownID);
			if (upDownCursor) {
				HandToHand((Handle *)&upDownCursor);
				if (MemError() == noErr) {
					/* Shift the cursor's hotspot over by distance from line */
					dy = pt.v - (SLOP + (inTop ? topMargin.top : (bottomMargin.top+1)));
					(*upDownCursor)->hotSpot.v += dy;
					SetCursor(*upDownCursor);
					/*
					 *	Precompute values of interest; don't let margin closer
					 *	than GAP pixels to edge
					 */
					if (inTop) {
						dragVal = &margin.top;
						minVal = doc->paperRect.top + GAP;
						maxVal = doc->marginRect.bottom - GAP;
						}
					 else {
						dragVal = &margin.bottom;
						maxVal = doc->paperRect.bottom - GAP;
						minVal = doc->marginRect.top + GAP;
						}
					minVal += doc->currentPaper.top;
					maxVal += doc->currentPaper.top;
					didSomething = vert = TRUE;
					}
				}
			}
		 else if ((inRight=PtInRect(pt,&rightMargin)) || (inLeft=PtInRect(pt,&leftMargin))) {
		 	whichMarg = inLeft ? LMARG : RMARG;
			rightLeftCursor = GetCursor(DragRightLeftID);
			if (rightLeftCursor) {
				HandToHand((Handle *)&rightLeftCursor);
				if (MemError() == noErr) {
					dx = pt.h - (SLOP + (inLeft ? leftMargin.left : (rightMargin.left+1)));
					(*rightLeftCursor)->hotSpot.h += dx;
					SetCursor(*rightLeftCursor);
					if (inLeft) {
						dragVal = &margin.left;
						minVal = doc->paperRect.left + GAP;
						maxVal = doc->marginRect.right- GAP;
						}
					 else {
					 	dragVal = &margin.right;
					 	minVal = doc->marginRect.left + GAP;
					 	maxVal = doc->paperRect.right - GAP;
						}
					minVal += doc->currentPaper.left;
					maxVal += doc->currentPaper.left;
					didSomething = horiz = TRUE;
					}
				}
			}

		/*
		 *	Head into loop if we've changed cursor: we have to track doc->marginRect
		 *	as well (via *origVal), so that we can have proper AutoScroll updating.
		 *	Coordinates within this loop are all window-relative pixels.
		 */
		
		if (didSomething) {
			PenMode(patXor);
			PenPat(NGetQDGlobalsGray());
			oldVal = horiz ? pt.h : pt.v;
			if (StillDown()) while (WaitMouseUp()) {
				GetMouse(&pt);
				moved = FALSE;
				if (horiz) {
					if (pt.h < minVal) pt.h = minVal; else if (pt.h > maxVal) pt.h = maxVal;
					if (pt.h != oldVal) {
						if (whichMarg==LMARG) {
							margh = pt.h-doc->currentPaper.left;
							hdiff = pt.h-oldPt.h - doc->currentPaper.left;
							}
						 else {
							margh = doc->currentPaper.right-pt.h;
							hdiff = doc->currentPaper.left-(pt.h-oldPt.h);
							}
						DrawMarginParams(doc, whichMarg, margh, hdiff);
						FrameRect(&margin);		/* Erase old margin */
						*dragVal = pt.h - dx;	/* Set new margin */
						oldVal = pt.h;
						FrameRect(&margin);		/* Draw new margin */
						moved = TRUE;
						doc->margHChangedMP = TRUE;
						}
					}
				 else if (vert) {
					if (pt.v < minVal) pt.v = minVal; else if (pt.v > maxVal) pt.v = maxVal;
				 	if (pt.v != oldVal) {
				 		if (whichMarg==TMARG) {
				 			margv = pt.v-doc->currentPaper.top;
							vdiff = pt.v-oldPt.v - doc->currentPaper.top;
							}
						 else {
				 			margv = doc->currentPaper.bottom-pt.v;
							vdiff = doc->currentPaper.top-(pt.v-oldPt.v);
						 	}
						DrawMarginParams(doc, whichMarg, margv, vdiff);
						FrameRect(&margin);		/* Erase old margin */
						oldVal = pt.v;
						*dragVal = pt.v - dy;	/* Set new margin */
						FrameRect(&margin);		/* Draw new margin */
						moved = TRUE;
						doc->margVChangedMP = TRUE;
						}
					}
				if (moved) {
					/*
					 *	Before calling AutoScroll(), update doc->marginRect. It is
					 *	always in paper-relative points.
					 */
					doc->marginRect = margin;
					OffsetRect(&doc->marginRect,-doc->currentPaper.left,-doc->currentPaper.top);
					UnmagnifyPaper(&doc->marginRect,&doc->marginRect,doc->magnify);
					}
				AutoScroll();
				SleepTicks(2L);			/* Avoid too much flicker */
				}

			/* Mouse button off: doc->marginRect is already set for the new margins. */
			if (!EqualRect(&origMarginRect,&doc->marginRect)) didSomething = TRUE;
			
			if (horiz) DisposeHandle((Handle)rightLeftCursor);
			 else if (vert) DisposeHandle((Handle)upDownCursor);
			}
			
		return (didSomething);
	}


/* Handle a double-click in Master Page: call FindMasterObject with selection
mode SMDblClick to bring up InstrDialog to edit a part.

Must save and restore doc->nstaves, since the number of staves in Master
Page may be temporarily inconsistent with the number in the score, and
intervening routines use doc->nstaves and need to know how many staves they
are dealing with. */

static void DoMasterDblClick(Document *doc, Point pt, short /*modifiers*/)
{
	short oldnstaves,index;

	oldnstaves = doc->nstaves;
	doc->nstaves = doc->nstavesMP;
	FindMasterObject(doc, pt, &index, SMDblClick);
	doc->nstaves = oldnstaves;
}


/* ------------------------------------------------ Master Page Selection Routines -- */

/* Track mouse dragging and select all Master Page objects in rectangular area.
The origin and clip region should already be set correctly.
Usage of doc->selStartL && doc->selEndL is handled by Enter/ExitMasterView. */

void SelectMasterRectangle(Document *doc, Point pt)
{
	Boolean found;	short pIndex; Rect selRect;
	CONTEXT context[MAXSTAVES+1]; STFRANGE stfRange={0,0};
	LINK pL,oldSelStartL,oldSelEndL;
	

	SetCursor(*handCursor);
	GetUserRect(doc, pt, pt, 0, 0, &selRect);
	OffsetRect(&selRect,-doc->currentPaper.left,-doc->currentPaper.top);

	oldSelStartL = doc->selStartL;
	oldSelEndL = doc->selEndL;
	doc->selStartL = doc->selEndL = NILINK;					/* This routine MUST change these before exiting */ 
	 
	pL = SSearch(doc->masterHeadL,PAGEtype,GO_RIGHT);
	while (pL!=doc->masterTailL) {
		ContextObject(doc, pL, context);
		if (VISIBLE(pL))
			/* ObjRect of staffobject is NULL here. */
			if (/* SectRect(&selRect, &LinkOBJRECT(pL), &aRect) */ 1 ) {
				found = FALSE;
				CheckMasterObject(doc, pL, &found, (Ptr)&selRect, context, SMDrag, &pIndex, stfRange);
				if (found) {
					LinkSEL(pL) = TRUE;								/* update selection */
					if (!doc->selStartL)
						doc->selStartL = pL;
					doc->selEndL = RightLINK(pL);
				}
	  		}
			pL = RightLINK(pL);
	}		

	/* doc->selStartL and doc->selEndL can only be set as a pair, and Master Page
		objects can be selected if and only if this pair is set; therefore if either
		is NILINK, we can safely reset both to their old values. */

	if (doc->selStartL==NILINK || doc->selEndL==NILINK) {
		doc->selStartL = oldSelStartL;
		doc->selEndL = oldSelEndL;
	}
	ArrowCursor();														/* Can only get here if cursor should become arrow */
}


/* Select, and hilite, every staff subobject in the part that staff <staffn>
belongs to. Similar to SelectStaff, except that function does no hiliting. */

static LINK SelPartStaves(Document *, LINK, CONTEXT [], short);
static LINK SelPartStaves(Document *doc, LINK staffL, CONTEXT context[], short staffn)
{
	short firstStaff,lastStaff;
	LINK partL; STFRANGE stfRange;
	Point enlarge = {0,0};

	firstStaff = lastStaff = NOONE;
	partL=FirstSubLINK(doc->masterHeadL);
	for (partL=NextPARTINFOL(partL); partL; partL=NextPARTINFOL(partL))		/* Skip unused first partL */
		if (PartFirstSTAFF(partL) <= staffn && PartLastSTAFF(partL) >= staffn) {
			firstStaff = PartFirstSTAFF(partL);
			lastStaff = PartLastSTAFF(partL);
			break;
		}
	
	stfRange.topStaff = firstStaff;
	stfRange.bottomStaff = lastStaff;
	CheckSTAFF(doc, staffL, context, NULL, SMSelectRange, stfRange, enlarge);

	return partL;
}


/* Handle clicking on objects in the Master Page: track dragging, when the object
is draggable, and selection. */
	
void DoMasterObject(Document *doc, Point pt, short modifiers)
{
	short index, oldnstaves, lowStaffn, hiStaffn, stf;
	LINK selStaffL, staffL, pL, aStaffL;

	oldnstaves = doc->nstaves;
	doc->nstaves = doc->nstavesMP;
	
	selStaffL = NILINK;
	staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSEL(aStaffL)) { selStaffL = aStaffL; break; }

	/* Look for an object clicked on: if it's draggable, track dragging with feedback. */
	
	pL = FindMasterObject(doc, pt, &index, SMClick);

	/*
	 * If there was already a staff selected and user shift-clicked on a staff, extend
	 * the range of selected staves on a whole-part-at-a-time basis.
	 */
	if (selStaffL && StaffTYPE(pL) && (modifiers & shiftKey)) {
		lowStaffn = 999; hiStaffn = -999;
		aStaffL = FirstSubLINK(pL);
		for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
			if (StaffSEL(aStaffL) || aStaffL==selStaffL) {
				if (StaffSTAFF(aStaffL)<lowStaffn) lowStaffn = StaffSTAFF(aStaffL);
				if (StaffSTAFF(aStaffL)>hiStaffn) hiStaffn = StaffSTAFF(aStaffL);
			}
		GetAllContexts(doc,contextA,pL);
		for (stf = lowStaffn; stf<=hiStaffn; stf++)
			SelPartStaves(doc, pL, contextA, stf);			/* Selects all staves in its part */
	}
	
	if (!pL)
		SelectMasterRectangle(doc, pt);

	doc->nstaves = oldnstaves;
}


/* ----------------------------------------------------------------- DoEditMaster -- */
/*
 *	Search for something (including both objects in the usual Nightingale sense
 * and page margins!) in the Master Page the user may have clicked on.  If something
 * found, handle dragging, selecting, and double-clicking, and return TRUE.  If nothing
 *	clickable, return FALSE.  The pt provided is expected in paper-relative pixels.
 */

Boolean DoEditMaster(Document *doc, Point pt, short modifiers, short doubleClick)
	{
		Boolean didSomething=FALSE;

		if (EditDocMargin(doc,pt,modifiers,doubleClick)) {
			didSomething = TRUE;
			UpdateMasterMargins(doc);
			}
		 else {	
			MEHideCaret(doc);
			if (doubleClick)
				DoMasterDblClick(doc, pt, modifiers);
			else
				DoMasterObject(doc, pt, modifiers);
		 	}
		
		PenNormal();
		
		return(didSomething);
	}
