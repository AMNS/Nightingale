/* DrawingEdit.c - routines for graphically editing GRDraw objects for Nightingale */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static enum {
	LGRIP=1,
	RGRIP,
	DRAGOBJ
} E_DrawEditItems;

#define  BOXSIZE		4

typedef struct {
	Point	leftPt, rightPt;					/* paper coords */
	Rect	objRect;								/* paper coords */
	short	penThick;							/* pen thickness (pixels) */
	short centerOffset;						/* offset for vertical centering (pixels) */
} DRAWOBJ;

/* Local prototypes */

static void EditDrawObj(Document *, LINK, short, DRAWOBJ *);
static Boolean InitFeedback(Document *, PCONTEXT, LINK, DRAWOBJ *);
static void InitGrips(Document *, PCONTEXT, LINK);
static void InitDrawObjBounds(Document *, LINK, short, DRAWOBJ *, Point, Rect *);
static void DoDrawFeedback(Document *, DRAWOBJ *);
static void UpdateTmpObjRect(Document *, short, DRAWOBJ *, short, short);
static void DrawGrip(Document *, short);
static void DrawBothGrips(Document *);
static void DrawInvalRects(Document *, LINK, DRAWOBJ *, Boolean);
static void UpdateDrawObjData(Document *, PCONTEXT, LINK, DRAWOBJ *);

static Point	lGrip, rGrip;				/* in paper coords */


/* -------------------------------------------------------------- DoDrawingEdit -- */

void DoDrawingEdit(Document *doc, LINK pL)
{
	Point 		mousePt;							/* paper coord */
	EventRecord	eventRec;
	CONTEXT		context;
	DRAWOBJ		oldDrawObj, origDrawObj, thisDrawObj;

	FlushEvents(everyEvent, 0);
	
	GetContext(doc, pL, GraphicSTAFF(pL), &context);

	/* Deselect Graphic to prevent flashing when user clicks outside of
	 * Graphic, and to avoid hiliting original Graphic when autoscrolling.
	 */	
	DeselectNode(pL);

	if (!InitFeedback(doc, &context, pL, &thisDrawObj))
		return;
	oldDrawObj = origDrawObj = thisDrawObj;
	InitGrips(doc, &context, pL);
	DrawBothGrips(doc);
	
	while (TRUE) 
	{
		GetNextEvent(mDownMask, &eventRec);
		if (eventRec.what == mouseDown) {
			mousePt = eventRec.where;
			GlobalToPaper(doc, &mousePt);
			if (!(SamePoint(mousePt, lGrip) ||
				   SamePoint(mousePt, rGrip) ||
				   PtInRect(mousePt, &thisDrawObj.objRect)))
				break;
		}

		/* Leave initial object in gray. NB: If you want the object
		 * to be fainter than staff lines, use dkGray (sic) instead.
		 */
		PenMode(patXor);
		DoDrawFeedback(doc, &thisDrawObj);					/* erase black object */
		PenPat(NGetQDGlobalsGray());
		DoDrawFeedback(doc, &thisDrawObj);					/* draw it in gray */
		PenNormal();

		if (SamePoint(mousePt, lGrip))
			EditDrawObj(doc, pL, LGRIP, &thisDrawObj);
		else if (SamePoint(mousePt, rGrip))
			EditDrawObj(doc, pL, RGRIP, &thisDrawObj);
		else if (PtInRect(mousePt, &thisDrawObj.objRect)) {
			EditDrawObj(doc, pL, DRAGOBJ, &thisDrawObj);
			DrawBothGrips(doc);
		}
		
		PenMode(patXor);
		DoDrawFeedback(doc, &oldDrawObj);					/* erase old gray object */
		PenPat(NGetQDGlobalsGray());
		DoDrawFeedback(doc, &oldDrawObj);					/* (must do this twice, because gray pat is misaligned ) */
		PenNormal();

		UpdateDrawObjData(doc, &context, pL, &thisDrawObj);	/* see comment on AutoScroll below */

		oldDrawObj = thisDrawObj;
	}
	DoDrawFeedback(doc, &thisDrawObj);							/* draw black object */
	DrawBothGrips(doc);												/* erase grips */

	/* Update object in data structure and inval rect if it's changed. */
	if (BlockCompare(&thisDrawObj, &origDrawObj, sizeof(DRAWOBJ))) {
		UpdateDrawObjData(doc, &context, pL, &thisDrawObj);
		doc->changed = TRUE;
		DrawInvalRects(doc, pL, &thisDrawObj, TRUE);
	}
	MEHideCaret(doc);	
}


/* ---------------------------------------------------------------- EditDrawObj -- */
/*	Handle the actual dragging of the object or one of its handles.
 * <grip> tells which mode of dragging the object is in effect.
 * <mousePt> and grip points are in paper-relative coordinates.
 */

static void EditDrawObj(Document *doc, LINK pL,
						short grip,		/* Edit mode: LGRIP, RGRIP, DRAGOBJ */
						DRAWOBJ *pd)	
{
	Point		oldPt, newPt;
	long		aLong;
	Rect		boundsRect;				/* in paper coords */
	short		dh, dv;
			
	if (grip==DRAGOBJ)										/* Erase the boxes before dragging */
		DrawBothGrips(doc);

	GetPaperMouse(&oldPt, &doc->currentPaper);
	InitDrawObjBounds(doc, pL, grip, pd, oldPt, &boundsRect);

	newPt = oldPt;

	if (StillDown()) {
		while (WaitMouseUp()) {
			GetPaperMouse(&newPt, &doc->currentPaper);

			/* Force the code below to see the mouse as always within boundsRect. */
			aLong = PinRect(&boundsRect, newPt);
			newPt.v = HiWord(aLong);	newPt.h = LoWord(aLong);

			if (EqualPt(newPt, oldPt)) continue;
			
			dh = newPt.h - oldPt.h;
			dv = newPt.v - oldPt.v;
		
			PenMode(patXor);
			DoDrawFeedback(doc, pd);							/* erase old object */
			
			switch (grip) {
				case DRAGOBJ:
					lGrip.h += dh;		lGrip.v += dv;
					rGrip.h += dh;		rGrip.v += dv;
					pd->leftPt.h += dh;	pd->leftPt.v += dv;
					pd->rightPt.h += dh;	pd->rightPt.v += dv;
					break;
				case LGRIP:
					DrawGrip(doc, LGRIP);						/* erase grip */
					lGrip.h += dh;
					pd->leftPt.h += dh;
					lGrip.v += dv;
					pd->leftPt.v += dv;
					break;
				case RGRIP:
					DrawGrip(doc, RGRIP);
					rGrip.h += dh;
					pd->rightPt.h += dh;
					rGrip.v += dv;
					pd->rightPt.v += dv;
					break;
			}
			UpdateTmpObjRect(doc, grip, pd, dh, dv);
			
#if 1
			AutoScroll();						
#endif
			/* ??NB: There are some problems with autoscroll:
			 *	1. It draws the original hairpin in black when it comes back into view after
			 *		having been scrolled scrolled out of view. Can solve this by clearing hairpin's
			 *		vis flag (in InitFeedback) and restoring it later (if it was vis to begin
			 *		with). However, this won't work if Show Invisibles is on.
			 *		Calling UpdateHairData after each call to EditHairpin at least insures that
			 *		the most recent result of editing will be drawn in black.
			 * 2. Similar problem with gray hairpins. AutoScroll doesn't know about them,
			 *		so it can't redraw them when they come back into view. The subsequent xor
			 *		drawing that's supposed to erase the gray hairpin actualling redraws it.
			 *	I'm not sure these problems outweigh the advantage of autoscrolling, which after
			 * all won't be used that often, and won't always cause these problems.
			 */
			 
			if (grip != DRAGOBJ)
				DrawGrip(doc, grip);
			DoDrawFeedback(doc, pd);							/* draw new object */
			PenNormal();
			
			oldPt = newPt;
		}
	}
}


#ifdef PROBABLY_NOT

/* ----------------------------------------------------------------- DragDrawObj -- */
/* Called from DoSymbolDrag. Similar to DoDrawingEdit using only DRAGOBJ grip. */

void DragDrawObj(Document *doc, LINK pL)
{
	Point		oldPt, newPt, origPt;
	long		aLong;
	Rect		boundsRect;				/* in paper coords */
	short		dh, dv, dhTotal, dvTotal;
	CONTEXT	allContexts[MAXSTAVES+1];
	CONTEXT	context;
	STFRANGE	stfRange;
	Point		enlarge = {0,0};
	DRAWOBJ	oldHair, origHair, thisHair;
	Boolean	stillWithinSlop, horiz, vert;
	
	FlushEvents(everyEvent, 0);
	
	GetContext(doc, pL, GraphicSTAFF(pL), &context);

	/* Deselect hairpin to prevent flashing when user clicks outside of
	 * hairpin, and to avoid hiliting original hairpin when autoscrolling.
	 */	
	DeselectNode(pL);

	if (!InitFeedback(doc, &context, pL, &thisHair))
		return;
	oldHair = origHair = thisHair;
	GetPaperMouse(&oldPt, &doc->currentPaper);
	InitDrawObjBounds(doc, pL, DRAGOBJ, &thisHair, oldPt, &boundsRect);

	PenMode(patXor);
	DoDrawFeedback(doc, &thisHair);				/* erase black hairpin */
	PenPat(NGetQDGlobalsGray());
	DoDrawFeedback(doc, &thisHair);				/* draw it in gray */
	PenPat(NGetQDGlobalsBlack());

	origPt = newPt = oldPt;
	stillWithinSlop = TRUE;
	horiz = vert = TRUE;

	if (StillDown()) {
		while (WaitMouseUp()) {
			GetPaperMouse(&newPt, &doc->currentPaper);

			/* Force the code below to see the mouse as always within boundsRect. */
			aLong = PinRect(&boundsRect, newPt);
			newPt.v = HiWord(aLong);	newPt.h = LoWord(aLong);

			if (EqualPt(newPt, oldPt)) continue;
			if (stillWithinSlop) {
				/* If mouse is still within slop bounds, don't do anything. Otherwise,
					decide whether horizontally/vertically constrained. */
				dhTotal = newPt.h - origPt.h;
				dvTotal = newPt.v - origPt.v;
				if (ABS(dhTotal)<2 && ABS(dvTotal)<2) continue;
				if (ShiftKeyDown()) {
					horiz = ABS(dhTotal) > ABS(dvTotal);		/* 45 degree movement => vertical */
					vert = !horiz;
				}
				/* And don't ever come back, you hear! */
				stillWithinSlop = FALSE;
			}
			dh = (horiz? newPt.h - oldPt.h : 0);
			dv = (vert? newPt.v - oldPt.v : 0);
		
			DoDrawFeedback(doc, &thisHair);							/* erase old hairpin */
			
			thisHair.leftPt.h += dh;	thisHair.leftPt.v += dv;
			thisHair.rightPt.h += dh;	thisHair.rightPt.v += dv;

			UpdateTmpObjRect(doc, DRAGOBJ, &thisHair, dh, dv);
#if 1
			AutoScroll();					/* ??NB: See note in EditDrawObj */					
#endif
			DoDrawFeedback(doc, &thisHair);							/* draw new hairpin */
			
			oldPt = newPt;
		}
	}
	/* Erase gray hairpin, in case it's outside system rect (thus not affected by inval below) */
	PenMode(patXor);
	DoDrawFeedback(doc, &oldHair);
	PenPat(NGetQDGlobalsGray());
	DoDrawFeedback(doc, &oldHair);				/* (must do this twice, because gray pat is misaligned ) */
	PenNormal();

	DoDrawFeedback(doc, &thisHair);						/* draw black hairpin */

	/* Update hairpin in data structure and inval rect if it's changed. */
	if (BlockCompare(&thisHair, &origHair, sizeof(DRAWOBJ))) {
		UpdateHairData(doc, &context, pL, &thisHair);
		doc->changed = TRUE;
	}
	DrawInvalRects(doc, pL, &thisHair, FALSE);			/* must do this even if hairpin not changed (affects hiliting) */
	MEHideCaret(doc);	

	LinkSEL(pL) = TRUE;								/* so that hairpin will hilite after we're done dragging */
	doc->selStartL = pL;	doc->selEndL = RightLINK(pL);
}

#endif

void LocateDrawObjEndPts(LINK pL, DDIST *pFirstXD, DDIST *pLastXD);
void LocateDrawObjEndPts(LINK pL, DDIST *pFirstXD, DDIST *pLastXD)
{
	LINK firstL, lastL, aNoteL;
	short voice;
	
	firstL = GraphicFIRSTOBJ(pL);
	lastL = GraphicLASTOBJ(pL);
	*pFirstXD = SysRelxd(firstL);
	*pLastXD = SysRelxd(lastL);

	voice = GraphicVOICE(pL);
	aNoteL = FindMainNote(firstL, voice);
	*pFirstXD += NoteXD(aNoteL);
	aNoteL = FindMainNote(lastL, voice);
	*pLastXD += NoteXD(aNoteL);
}


/* ---------------------------------------------------------------- InitFeedback -- */
/* Fill in the DRAWOBJ struct we use for feedback. */

static Boolean InitFeedback(Document */*doc*/,
								PCONTEXT pContext,
								LINK pL,							/* GRDraw Graphic */
								DRAWOBJ *pd)
{
	DDIST		firstXD, lastXD, sysLeft, staffTop, lnSpace, dThick;
	PGRAPHIC	pGraphic;
	
	lnSpace = LNSPACE(pContext);
	/*
	 * We interpret thickening as horizontal only--not good unless the line is
	 * nearly horizontal, or thickness is small enough that it doesn't matter.
	 */
	pGraphic = GetPGRAPHIC(pL);
	dThick = (long)(pGraphic->gu.thickness*lnSpace) / 100L;
	pd->penThick = (d2p(dThick)>1? d2p(dThick) : 1);
	pd->centerOffset = d2p(dThick/2);

	staffTop = pContext->staffTop;
	sysLeft = pContext->systemLeft;

	LocateDrawObjEndPts(pL, &firstXD, &lastXD);
	
	/*
	 * Position the object with vertical centering. This should be done exactly as the
	 * drawing routine, DrawGRDraw, does it, with the same rounding, or feedback won't
	 * always be correct. Unfortunately, the following does NOT do so: cf. DrawGRAPHIC
	 * and the routines it calls, GetGraphicDrawInfo, GetGRDrawLastDrawInfo, and
	 * DrawGRDraw.
	 */
	pGraphic = GetPGRAPHIC(pL);
	SetPt(&pd->leftPt, d2p(firstXD+LinkXD(pL)+sysLeft), d2p(staffTop+LinkYD(pL)));
	SetPt(&pd->rightPt, d2p(lastXD+pGraphic->info+sysLeft), d2p(staffTop+pGraphic->info2));
	pd->leftPt.v -= pd->centerOffset;
	pd->rightPt.v -= pd->centerOffset;
	
	pd->objRect = LinkOBJRECT(pL);
	pd->objRect.bottom += 1;				/* make it easier to grab */
	
	return TRUE;
}


/* -------------------------------------------------------------------- InitGrips -- */
/* Set up the draw object grips temporarily here. */

static void InitGrips(Document */*doc*/, PCONTEXT pContext, LINK pL)
{
	DDIST		firstXD, lastXD, sysLeft, staffTop;
	PGRAPHIC	pGraphic;

	staffTop = pContext->staffTop;
	sysLeft = pContext->systemLeft;

	LocateDrawObjEndPts(pL, &firstXD, &lastXD);
	
	pGraphic = GetPGRAPHIC(pL);
	SetPt(&lGrip, d2p(firstXD+LinkXD(pL)+sysLeft), d2p(staffTop+LinkYD(pL)));
	lGrip.h -= 5;
	SetPt(&rGrip, d2p(lastXD+pGraphic->info+sysLeft), d2p(staffTop+pGraphic->info2));
	rGrip.h += 6;
}


/* ----------------------------------------------------------- InitDrawObjBounds -- */
/* Set up the rectangle beyond which the user won't be allowed to drag the object. */

#define SYSHT_SLOP pt2d(8)

static void InitDrawObjBounds(Document *doc, LINK pL, short grip,
										DRAWOBJ *pd,
										Point mousePt, Rect *bounds)	/* paper coords */
{
	short			staffn, mouseFromLeft, mouseFromRight;
	CONTEXT		context;
	PAMEASURE	aMeasP;
	DDIST			sysLeft, sysTop, sysRight, sysBot, measWid;
	LINK			firstL, lastL, measL, prevMeasL, nextMeasL, targetMeasL;

	mouseFromLeft = pd->leftPt.h - mousePt.h;
	mouseFromRight = pd->rightPt.h - mousePt.h;
	
	staffn = GraphicSTAFF(pL);
	GetContext(doc, pL, staffn, &context);
	sysLeft = context.systemLeft;
	sysTop = context.systemTop;
	sysRight = context.staffRight;
	sysBot = context.systemBottom;
	
	firstL = GraphicFIRSTOBJ(pL);
	lastL = GraphicLASTOBJ(pL);

	/* largest acceptable rect */
	bounds->left = d2p(sysLeft);
	bounds->top = d2p(sysTop - SYSHT_SLOP);
	bounds->right = d2p(sysRight);
	bounds->bottom = d2p(sysBot + SYSHT_SLOP);
	
	/* Constrain further in these ways:
	 * 	- Object should never be less than 2 pixels long.
	 * 	- Don't let endpoints reverse left-to-right order.
	 *		- Neither endpoint should be further than 1 measure away
	 *			from one of the object's attachment points.
	 */

	/* Find the measure that will be the left boundary */
	measL = LSSearch(firstL, MEASUREtype, staffn, GO_LEFT, FALSE);		/* meas containing 1st att.point */
	prevMeasL = LinkLMEAS(measL);															/* meas before that */
	if (SameSystem(prevMeasL, measL))
		targetMeasL = prevMeasL;
	else
		targetMeasL = measL;
		
	/* Allow dragging into reserved area */
	measL = LSSearch(MeasSYSL(targetMeasL), MEASUREtype, staffn, GO_RIGHT, FALSE); /* 1st meas in system */	
	if (targetMeasL == measL)																/* targetMeas is 1st in system */
		bounds->left = d2p(sysLeft);
	else
		bounds->left = d2p(LinkXD(targetMeasL) + sysLeft);

	/* Find the measure that will be the right boundary */
	measL = LSSearch(lastL, MEASUREtype, staffn, GO_LEFT, FALSE);			/* meas containing last sync */
	nextMeasL = LinkRMEAS(measL);														/* meas after that */
	if (SameSystem(nextMeasL, measL))
		targetMeasL = nextMeasL;
	else
		targetMeasL = measL;
	aMeasP = GetPAMEASURE(FirstSubLINK(targetMeasL));							/* get xd of END of meas */
	measWid = aMeasP->measureRect.right;
	bounds->right = d2p(LinkXD(targetMeasL) + measWid + sysLeft);

	/* Prevent objects < 2 pixels long and crossing of endpoints. Offset
	 *	boundaries depending on mouse position relative to object endpoints.
	 *	(PinRect in EditDrawObj decides whether mousePt, not object, is
	 *	 within bounds.)
	 */
	switch (grip) {
		case DRAGOBJ:
			bounds->left -= mouseFromLeft;
			bounds->right -= mouseFromRight-1;
			break;
		case LGRIP:
			bounds->right = pd->rightPt.h - (BOXSIZE+3);
			bounds->left -= mouseFromLeft;
			break;
		case RGRIP:
			bounds->left = pd->leftPt.h + (BOXSIZE+5);
			bounds->right -= mouseFromRight-1;
			break;
	}			
}


/* -------------------------------------------------------------- DoDrawFeedback -- */

static void DoDrawFeedback(Document *doc, DRAWOBJ *pd)
{
	Point	lPt, rPt;
	
	lPt = pd->leftPt;
	rPt = pd->rightPt;
	Pt2Window(doc, &lPt);
	Pt2Window(doc, &rPt);
	
	PenSize(1, pd->penThick);

	MoveTo(lPt.h, lPt.v);
	LineTo(rPt.h, rPt.v);

	PenSize(1,1);
}


/* ------------------------------------------------------------ UpdateTmpObjRect -- */

static void UpdateTmpObjRect(Document */*doc*/, short grip, DRAWOBJ *pd, short dh, short dv)
{	
	if (grip == DRAGOBJ)
		OffsetRect(&pd->objRect, dh, dv);
	else {
		pd->objRect.top = n_min(pd->leftPt.v, pd->rightPt.v);
		pd->objRect.bottom = n_max(pd->leftPt.v, pd->rightPt.v) + 1;
		if (grip == LGRIP)
			pd->objRect.left += dh;
		if (grip == RGRIP)
			pd->objRect.right += dh;
	}
}


/* -------------------------------------------------------------------- DrawGrip -- */
/* Draw a small box for the left and right grips of the feedback object. */

static void DrawGrip(Document *doc,
							short whichOne)		/* LGRIP or RGRIP */
{
	Point gripPt;

	if (whichOne==LGRIP)	gripPt = lGrip;
	else						gripPt = rGrip;
	Pt2Window(doc, &gripPt);
	DrawBox(gripPt, BOXSIZE);
}


/* --------------------------------------------------------------- DrawBothGrips -- */

static void DrawBothGrips(Document *doc)
{
	PenMode(patXor);
	DrawGrip(doc, LGRIP);			
	DrawGrip(doc, RGRIP);			
	PenNormal();
}


/* -------------------------------------------------------------- DrawInvalRects -- */
/* Inval both the original object's objRect and the new object's tmpObjRect. */

/* ???If updateRect falls entirely outside of the system rect, the update
 * triggered by invaling that rect doesn't take place. If it intersects
 * the system rect at all, the update is fine. This seems to be a problem
 * way up the calling chain.
 */

static void DrawInvalRects(Document *doc, LINK pL,
							DRAWOBJ *pd,
							Boolean	optimize	/* if TRUE erase & inval only the original objRect */
							)
{
	Rect	oldObjRect, newObjRect, updateRect;

/* ???NB: this should look at config.enlargeHilite value!! */
	newObjRect = pd->objRect;
	InsetRect(&newObjRect, -pt2p(2), -pt2p(3));			/* avoid hiliting problem on update */
	oldObjRect = LinkOBJRECT(pL);
	InsetRect(&oldObjRect, -pt2p(1), -pt2p(1));
	Rect2Window(doc, &newObjRect);
	Rect2Window(doc, &oldObjRect);
	UnionRect(&oldObjRect, &newObjRect, &updateRect);
#if 0
if (ShiftKeyDown()) {
	FrameRect(&updateRect);				/* to debug update rect */
	SleepTicks(60L);
}
#endif
	if (optimize)
		EraseAndInval(&oldObjRect);
	else
		EraseAndInval(&updateRect);

	LinkVALID(pL) = FALSE;										/* force objRect recomputation */
}


/* ----------------------------------------------------------- UpdateDrawObjData -- */

static void UpdateDrawObjData(Document */*doc*/, PCONTEXT pContext, LINK pL, DRAWOBJ *pd)
{
	DDIST		firstXD, lastXD, sysLeft, staffTop;
	PGRAPHIC	pGraphic;
	
	staffTop = pContext->staffTop;
	sysLeft = pContext->systemLeft;

	LocateDrawObjEndPts(pL, &firstXD, &lastXD);

	pGraphic = GetPGRAPHIC(pL);
	pGraphic->xd = p2d(pd->leftPt.h) - firstXD - sysLeft;
	pGraphic->yd = p2d(pd->leftPt.v) - staffTop;
	pGraphic->info = p2d(pd->rightPt.h) - lastXD - sysLeft;
	pGraphic->info2 = p2d(pd->rightPt.v) - staffTop;

	/* Compensate for vertical-centering offset (cf. InitFeedback) */
	
	pGraphic->yd += p2d(pd->centerOffset);
	pGraphic->info2 += p2d(pd->centerOffset);
	
	LinkTWEAKED(pL) = TRUE;
}
