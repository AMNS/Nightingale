/* HairpinEdit.c for Nightingale - Routines for graphically editing hairpins,
by John Gibson. */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-2001 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static enum {
	LGRIP=1,
	RGRIP,
	DRAGOBJ
} E_HairpinGrips;

#define  BOXSIZE		4

typedef struct {
	Point	leftPt, rightPt;					/* paper coords */
	short	leftRise, rightRise;				/* pixels */
	Rect	objRect;								/* paper coords */
	short	penThick;							/* pen thickness */
} HAIRPIN;

/* local prototypes for hairpin editing routines */
static void EditHairpin(Document *, LINK, short, HAIRPIN *);
static Boolean InitHairFeedback(Document *, PCONTEXT, LINK, HAIRPIN *);
static void InitHairGrips(Document *, PCONTEXT, LINK);
static void InitHairBounds(Document *, LINK, short, HAIRPIN *, Point, Rect *);
static void DoHairFeedback(Document *, HAIRPIN *);
static void UpdateTmpObjRect(Document *, short, HAIRPIN *, short, short);
static void DrawHairGrip(Document *, short);
static void DrawBothGrips(Document *);
static void HairInvalRects(Document *, LINK, HAIRPIN *, Boolean);
static void UpdateHairData(Document *, PCONTEXT, LINK, HAIRPIN *);

static Point	lGrip, rGrip;				/* in paper coords */


/* ----------------------------------------------------------------- DoHairpinEdit -- */

void DoHairpinEdit(Document *doc, LINK pL)
{
	Point 		mousePt;							/* paper coord */
	EventRecord	eventRec;
	CONTEXT		context;
	HAIRPIN		oldHair, origHair, thisHair;

	FlushEvents(everyEvent, 0);
	
	GetContext(doc, pL, DynamicSTAFF(FirstSubLINK(pL)), &context);

	/* Deselect hairpin to prevent flashing when user clicks outside of
	 * hairpin, and to avoid hiliting original hairpin when autoscrolling.
	 */	
	DeselectNode(pL);

	if (!InitHairFeedback(doc, &context, pL, &thisHair))
		return;
	oldHair = origHair = thisHair;
	InitHairGrips(doc, &context, pL);
	DrawBothGrips(doc);
	
	while (TRUE) {
		if (EventAvail(everyEvent, &eventRec)) {
		
			if (eventRec.what == mouseDown) {
				mousePt = eventRec.where;
				GlobalToPaper(doc, &mousePt);
				if (!(SamePoint(mousePt, lGrip) ||
					   SamePoint(mousePt, rGrip) ||
					   PtInRect(mousePt, &thisHair.objRect)))
					break;
			}
			else
				break;
			
			/* If mouse down event, edit hairpin. */			
			GetNextEvent(mDownMask, &eventRec);

			/* Leave initial hairpin in gray. NB: If you want the hairpin
			 * to be fainter than staff lines, use dkGray (sic) instead.
			 */
			PenMode(patXor);
			DoHairFeedback(doc, &thisHair);				/* erase black hairpin */
			PenPat(NGetQDGlobalsGray());
			DoHairFeedback(doc, &thisHair);				/* draw it in gray */
			PenNormal();

			if (SamePoint(mousePt, lGrip))
				EditHairpin(doc, pL, LGRIP, &thisHair);
			else if (SamePoint(mousePt, rGrip))
				EditHairpin(doc, pL, RGRIP, &thisHair);
			else if (PtInRect(mousePt, &thisHair.objRect)) {
				EditHairpin(doc, pL, DRAGOBJ, &thisHair);
				DrawBothGrips(doc);
			}
			
			PenMode(patXor);
			DoHairFeedback(doc, &oldHair);				/* erase old gray hairpin */
			PenPat(NGetQDGlobalsGray());
			DoHairFeedback(doc, &oldHair);				/* (must do this twice, because gray pat is misaligned ) */
			PenNormal();

			UpdateHairData(doc, &context, pL, &thisHair);		/* see comment on AutoScroll below */

			oldHair = thisHair;
		}
	}
	DoHairFeedback(doc, &thisHair);						/* draw black hairpin */
	DrawBothGrips(doc);										/* erase grips */

	/* Update hairpin in data structure and inval rect if it's changed. */
	if (BlockCompare(&thisHair, &origHair, sizeof(HAIRPIN))) {
		UpdateHairData(doc, &context, pL, &thisHair);
		doc->changed = TRUE;
		HairInvalRects(doc, pL, &thisHair, TRUE);
	}
	MEHideCaret(doc);	
}


/* ------------------------------------------------------------------ EditHairpin -- */
/*	Handle the actual dragging of the hairpin or one of its handles.
<grip> tells which mode of dragging the hairpin is in effect.
<mousePt> and grip points are in paper-relative coordinates. */

static void EditHairpin(
					Document	*doc,
					LINK		pL,
					short	 	grip,					/* Edit mode: LGRIP, RGRIP, DRAGOBJ */
					HAIRPIN	*hp
					)
{
	Point		oldPt, newPt;
	long		aLong;
	Rect		boundsRect;				/* in paper coords */
	short		dh, dv;
	Boolean	allowTilt = TRUE;		/* FALSE only if shiftKeyDown */
	
	if (ShiftKeyDown()) allowTilt = FALSE;		/* ??but should also allow vert constraint */
		
	if (grip==DRAGOBJ)				/* Erase the boxes before dragging the hairpin. */
		DrawBothGrips(doc);

	GetPaperMouse(&oldPt, &doc->currentPaper);
	InitHairBounds(doc, pL, grip, hp, oldPt, &boundsRect);

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
			DoHairFeedback(doc, hp);							/* erase old hairpin */
			
			switch (grip) {
				case DRAGOBJ:
					lGrip.h += dh;		lGrip.v += dv;
					rGrip.h += dh;		rGrip.v += dv;
					hp->leftPt.h += dh;	hp->leftPt.v += dv;
					hp->rightPt.h += dh;	hp->rightPt.v += dv;
					break;
				case LGRIP:
					DrawHairGrip(doc, LGRIP);					/* erase grip */
					lGrip.h += dh;
					hp->leftPt.h += dh;
					if (allowTilt) {
						lGrip.v += dv;
						hp->leftPt.v += dv;
					}
					break;
				case RGRIP:
					DrawHairGrip(doc, RGRIP);
					rGrip.h += dh;
					hp->rightPt.h += dh;
					if (allowTilt) {
						rGrip.v += dv;
						hp->rightPt.v += dv;
					}
					break;
			}
			UpdateTmpObjRect(doc, grip, hp, dh, dv);
			
#if 1
			AutoScroll();						
#endif
			/* ??NB: There are some problems with autoscroll:
			 *	1) It draws the original hairpin in black when it comes back into view after
			 *		having been scrolled out of view. Can solve this by clearing hairpin's
			 *		vis flag (in InitHairFeedback) and restoring it later (if it was vis to begin
			 *		with). However, this won't work if Show Invisibles is on.
			 *		Calling UpdateHairData after each call to EditHairpin at least insures that
			 *		the most recent result of editing will be drawn in black.
			 * 2) Similar problem with gray hairpins. AutoScroll doesn't know about them,
			 *		so it can't redraw them when they come back into view. The subsequent xor
			 *		drawing that's supposed to erase the gray hairpin actually redraws it.
			 *	I'm not sure these problems outweigh the advantage of autoscrolling, which after
			 * all won't be used that often, and won't always cause these problems.
			 */
			 
			if (grip != DRAGOBJ)
				DrawHairGrip(doc, grip);
			DoHairFeedback(doc, hp);							/* draw new hairpin */
			PenNormal();
			
			oldPt = newPt;
		}
	}
}


/* ------------------------------------------------------------------ DragHairpin -- */
/* Called from DoSymbolDrag. Similar to DoHairpinEdit using only DRAGOBJ grip. */

void DragHairpin(Document	*doc, LINK pL)
{
	Point		oldPt, newPt, origPt;
	long		aLong;
	Rect		boundsRect;				/* in paper coords */
	short		dh, dv, dhTotal, dvTotal;
	CONTEXT	context;
	Point		enlarge = {0,0};
	HAIRPIN	oldHair, origHair, thisHair;
	Boolean	stillWithinSlop, horiz, vert;
	
	FlushEvents(everyEvent, 0);
	
	GetContext(doc, pL, DynamicSTAFF(FirstSubLINK(pL)), &context);

	/* Deselect hairpin to prevent flashing when user clicks outside of
	 * hairpin, and to avoid hiliting original hairpin when autoscrolling.
	 */	
	DeselectNode(pL);

	if (!InitHairFeedback(doc, &context, pL, &thisHair))
		return;
	oldHair = origHair = thisHair;
	GetPaperMouse(&oldPt, &doc->currentPaper);
	InitHairBounds(doc, pL, DRAGOBJ, &thisHair, oldPt, &boundsRect);

	PenMode(patXor);
	DoHairFeedback(doc, &thisHair);				/* erase black hairpin */
	PenPat(NGetQDGlobalsGray());
	DoHairFeedback(doc, &thisHair);				/* draw it in gray */
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
		
			DoHairFeedback(doc, &thisHair);							/* erase old hairpin */
			
			thisHair.leftPt.h += dh;	thisHair.leftPt.v += dv;
			thisHair.rightPt.h += dh;	thisHair.rightPt.v += dv;

			UpdateTmpObjRect(doc, DRAGOBJ, &thisHair, dh, dv);
#if 1
			AutoScroll();					/* ??NB: See note in EditHairpin */					
#endif
			DoHairFeedback(doc, &thisHair);							/* draw new hairpin */
			
			oldPt = newPt;
		}
	}
	/* Erase gray hairpin, in case it's outside system rect (thus not affected by inval below) */
	PenMode(patXor);
	DoHairFeedback(doc, &oldHair);
	PenPat(NGetQDGlobalsGray());
	DoHairFeedback(doc, &oldHair);				/* (must do this twice, because gray pat is misaligned ) */
	PenNormal();

	DoHairFeedback(doc, &thisHair);						/* draw black hairpin */

	/* Update hairpin in data structure and inval rect if it's changed. */
	if (BlockCompare(&thisHair, &origHair, sizeof(HAIRPIN))) {
		UpdateHairData(doc, &context, pL, &thisHair);
		doc->changed = TRUE;
	}
	HairInvalRects(doc, pL, &thisHair, FALSE);			/* must do this even if hairpin not changed (affects hiliting) */
	MEHideCaret(doc);	

	LinkSEL(pL) = TRUE;								/* so that hairpin will hilite after we're done dragging */
	DynamicSEL(FirstSubLINK(pL)) = TRUE;
	doc->selStartL = pL;	doc->selEndL = RightLINK(pL);
}


/* -------------------------------------------------------------- InitHairFeedback -- */
/* Fill in the HAIRPIN struct we use for feedback; return FALSE if error. */

static Boolean InitHairFeedback(Document */*doc*/, PCONTEXT pContext, LINK pL, HAIRPIN *hp)
{
	PADYNAMIC	aDynamic;
	DDIST			mouthWidD, otherWidD,
					firstSyncXD, lastSyncXD, sysLeft, staffTop;
	LINK			firstSyncL, lastSyncL;
	char			dynamicType;
	
	aDynamic = GetPADYNAMIC(FirstSubLINK(pL));
	dynamicType = DynamType(pL);	

	staffTop = pContext->staffTop;
	sysLeft = pContext->systemLeft;
	firstSyncL = DynamFIRSTSYNC(pL);
	lastSyncL = DynamLASTSYNC(pL);
	if (!SyncTYPE(firstSyncL) || !SyncTYPE(lastSyncL)) {
		MayErrMsg("InitHairFeedback: firstSyncL or lastSyncL is not a sync.");
		return FALSE;
	}
	firstSyncXD = SysRelxd(firstSyncL);
	lastSyncXD = SysRelxd(lastSyncL);

	SetPt(&hp->leftPt, d2p(firstSyncXD+LinkXD(pL)+sysLeft), d2p(staffTop+aDynamic->yd));
	SetPt(&hp->rightPt, d2p(lastSyncXD+aDynamic->endxd+sysLeft), d2p(staffTop+aDynamic->endyd));

	mouthWidD = qd2d(aDynamic->mouthWidth, pContext->staffHeight, pContext->staffLines);
	otherWidD = qd2d(aDynamic->otherWidth, pContext->staffHeight, pContext->staffLines);
	
	switch (dynamicType) {
		case DIM_DYNAM:
			hp->leftRise = d2p(mouthWidD>>1);
			hp->rightRise = d2p(otherWidD>>1);
			break;
		case CRESC_DYNAM:
			hp->leftRise = d2p(otherWidD>>1);
			hp->rightRise = d2p(mouthWidD>>1);
			break;
	}
	
	hp->objRect = LinkOBJRECT(pL);
	hp->objRect.bottom += 1;				/* make it easier to grab */
	
	/* Set pen thickness for drawing hairpin (cf DrawHairpin) */
	hp->penThick = d2p(LNSPACE(pContext)/6);
	if (hp->penThick<1) hp->penThick = 1;
	if (hp->penThick>2) hp->penThick = 2;
	
	return TRUE;
}


/* ---------------------------------------------------------------- InitHairGrips -- */
/* Set up the hairpin grips temporarily here. */

static void InitHairGrips(Document	*/*doc*/, PCONTEXT pContext, LINK pL)
{
	PADYNAMIC	hairP;
	LINK			firstSyncL, lastSyncL;
	DDIST			firstSyncXD, lastSyncXD, sysLeft, staffTop;

	staffTop = pContext->staffTop;
	sysLeft = pContext->systemLeft;

	hairP = GetPADYNAMIC(FirstSubLINK(pL));
	firstSyncL = DynamFIRSTSYNC(pL);
	lastSyncL = DynamLASTSYNC(pL);
	firstSyncXD = SysRelxd(firstSyncL);
	lastSyncXD = SysRelxd(lastSyncL);

	SetPt(&lGrip, d2p(firstSyncXD+LinkXD(pL)+sysLeft) - 5, d2p(staffTop+hairP->yd));
	SetPt(&rGrip, d2p(lastSyncXD+hairP->endxd+sysLeft) + 6, d2p(staffTop+hairP->endyd));
}


/* -------------------------------------------------------------- InitHairBounds -- */
/* Set up the rectangle beyond which the user won't be allowed to drag the hairpin. */

#define SYSHT_SLOP pt2d(8)

static void InitHairBounds(
		Document *doc,
		LINK		pL,
		short		grip,
		HAIRPIN	*hp,
		Point		mousePt,				/* paper coords */
		Rect		*bounds 				/* paper coords */
		)
{
	short			staffn, mouseFromLeft, mouseFromRight;
	CONTEXT		context;
	PAMEASURE	aMeasP;
	DDIST			sysLeft, sysTop, sysRight, sysBot, measWid;
	LINK			firstSyncL, lastSyncL, measL, prevMeasL, nextMeasL, targetMeasL;

	mouseFromLeft = hp->leftPt.h - mousePt.h;
	mouseFromRight = hp->rightPt.h - mousePt.h;
	
	staffn = DynamicSTAFF(FirstSubLINK(pL));
	GetContext(doc, pL, staffn, &context);
	sysLeft = context.systemLeft;
	sysTop = context.systemTop;
	sysRight = context.staffRight;
	sysBot = context.systemBottom;
	
	firstSyncL = DynamFIRSTSYNC(pL);
	lastSyncL = DynamLASTSYNC(pL);

	/* largest acceptable rect */
	bounds->left = d2p(sysLeft);
	bounds->top = d2p(sysTop - SYSHT_SLOP);
	bounds->right = d2p(sysRight);
	bounds->bottom = d2p(sysBot + SYSHT_SLOP);
	
	/* Constrain further in these ways:
	 * 	- Hairpin should never be less than 2 pixels long.
	 * 	- Don't let a cresc become a dim, or vice versa.
	 *		- Neither endpoint should be further than 1 measure
	 *			away from one of the hairpin's syncs.
	 */

	/* Find the measure that will be the left boundary */
	measL = LSSearch(firstSyncL, MEASUREtype, staffn, GO_LEFT, FALSE);		/* meas containing 1st sync */
	prevMeasL = LinkLMEAS(measL);															/* meas before that */
	if (SameSystem(prevMeasL, measL))
		targetMeasL = prevMeasL;
	else
		targetMeasL = measL;
		
	/* Allow dragging into reserved area */
	measL = LSSearch(MeasSYSL(targetMeasL), MEASUREtype, staffn, GO_RIGHT, FALSE);	/* 1st meas in system */	
	if (targetMeasL == measL)																/* targetMeas is 1st in system */
		bounds->left = d2p(sysLeft);
	else
		bounds->left = d2p(LinkXD(targetMeasL) + sysLeft);

	/* Find the measure that will be the right boundary */
	measL = LSSearch(lastSyncL, MEASUREtype, staffn, GO_LEFT, FALSE);			/* meas containing last sync */
	nextMeasL = LinkRMEAS(measL);															/* meas after that */
	if (SameSystem(nextMeasL, measL))
		targetMeasL = nextMeasL;
	else
		targetMeasL = measL;
	aMeasP = GetPAMEASURE(FirstSubLINK(targetMeasL));								/* get xd of END of meas */
	measWid = aMeasP->measureRect.right;
	bounds->right = d2p(LinkXD(targetMeasL) + measWid + sysLeft);

	/* Prevent hairpins < 2 pixels long and crossing of endpoints. Offset
	 *	boundaries depending on mouse position relative to hairpin endpoints.
	 *	(PinRect in EditHairpin decides whether mousePt, not hairpin, is
	 *	 within bounds.)
	 */
	switch (grip) {
		case DRAGOBJ:
			bounds->left -= mouseFromLeft;
			bounds->right -= mouseFromRight-1;
			break;
		case LGRIP:
			bounds->right = hp->rightPt.h - (BOXSIZE+3);
			bounds->left -= mouseFromLeft;
			break;
		case RGRIP:
			bounds->left = hp->leftPt.h + (BOXSIZE+5);
			bounds->right -= mouseFromRight-1;
			break;
	}			
}


/* --------------------------------------------------------------- DoHairFeedback -- */

static void DoHairFeedback(Document *doc, HAIRPIN *hp)
{
	Point	kludgePt, lPt, rPt;
	register short	length, tilt, lRise, rRise;
	
	lPt = hp->leftPt;
	rPt = hp->rightPt;
	Pt2Window(doc, &lPt);
	Pt2Window(doc, &rPt);
	lRise = hp->leftRise;
	rRise = hp->rightRise;
	
#if 0	/* attempt to sync w/ vert. retrace; this is worse!
			NB: slotted Macs need a VBL task to do this. */
{
	long	ticks;
	ticks = TickCount();
	for ( ; ticks==TickCount(); /* say("ticks=%ld\n",TickCount()) */ ) ;
}
#endif

	PenSize(1, hp->penThick);

	MoveTo(lPt.h, lPt.v + lRise);
	LineTo(rPt.h, rPt.v + rRise);
	MoveTo(lPt.h, lPt.v - lRise);
	LineTo(rPt.h, rPt.v - rRise);

	/* Kludge to fix disappearing lines emerging from 0-width endpoints.
	 * (They disappear because the two hairpin lines overlap there and are
	 * drawn in patXor mode.)
	 * The kludgePt.h values below still don't completely prevent gaps in
	 * tilted hairpin lines, but at least the endpoint is always visible.
	 */
	length = rPt.h - lPt.h;
	tilt = rPt.v - lPt.v;
	if (lRise == 0) {
		kludgePt = lPt;
		if (tilt) {
			kludgePt.h += (length/(ABS(tilt) + rRise)) - 1;					/* the -1 here and +1 below help a little, but aren't quite right */
			kludgePt.v += tilt>0? 1 : -1;
		}
		else
			kludgePt.h += (length/(rRise==0? rRise+1 : rRise))>>1;		/* NB: rRise+1 avoids divide by 0 when rightRise==0 */
		MoveTo(lPt.h, lPt.v);
		LineTo(kludgePt.h, kludgePt.v);
	}
	if (rRise == 0) {
		kludgePt = rPt;
		if (tilt) {
			kludgePt.h -= (length/(ABS(tilt) + lRise)) + 1;
			kludgePt.v -= tilt>0? 1 : -1;
		}
		else
			kludgePt.h -= (length/(lRise==0? lRise+1 : lRise))>>1;
		MoveTo(rPt.h, rPt.v);
		LineTo(kludgePt.h, kludgePt.v);
	}
	
	PenSize(1,1);
}


/* ------------------------------------------------------------- UpdateTmpObjRect -- */

static void UpdateTmpObjRect(Document */*doc*/, short grip, HAIRPIN *hp, short dh, short dv)
{	
	if (grip == DRAGOBJ)
		OffsetRect(&hp->objRect, dh, dv);
	else {
		hp->objRect.top = n_min(hp->leftPt.v - hp->leftRise, hp->rightPt.v - hp->rightRise);
		hp->objRect.bottom = n_max(hp->leftPt.v + hp->leftRise, hp->rightPt.v + hp->rightRise) + 1;
		if (grip == LGRIP)
			hp->objRect.left += dh;
		if (grip == RGRIP)
			hp->objRect.right += dh;
	}
/* ??NB: if both end widths are 0, this rect is only one pixel tall! */
}


/* ----------------------------------------------------------------- DrawHairGrip -- */
/* Draw a 2x2 box for the left and right grips of the feedback hairpin. */

static void DrawHairGrip(Document *doc,
									short whichOne)		/* LGRIP or RGRIP */
{
	Point gripPt;

	if (whichOne==LGRIP)	gripPt = lGrip;
	else						gripPt = rGrip;
	Pt2Window(doc, &gripPt);
	DrawBox(gripPt, BOXSIZE);
}


/* ---------------------------------------------------------------- DrawBothGrips -- */

static void DrawBothGrips(Document *doc)
{
	PenMode(patXor);
	DrawHairGrip(doc, LGRIP);			
	DrawHairGrip(doc, RGRIP);			
	PenNormal();
}


/* --------------------------------------------------------------- HairInvalRects -- */
/* Inval both the original hairpin's objRect and the new hairpin's tmpObjRect. */

/* ??If updateRect falls entirely outside of the system rect, the update
 * triggered by invaling that rect doesn't take place. If it intersects
 * the system rect at all, the update is fine. This seems to be a problem
 * way up the calling chain.
 */

static void HairInvalRects(
		Document	*doc,
		LINK pL,
		HAIRPIN *hp,
		Boolean optimize		/* if TRUE erase and inval only the original objRect */
		)
{
	Rect	oldObjRect, newObjRect, updateRect;

/* NB: other parts of Ngale don't depend on mouth & other wid when setting objRect
	(or is it just the rect used for hiliting?).
	If that's fixed, we may not need as much adjustment here. */
	
/* ??NB: this should look at config.enlargeHilite value!! */
	newObjRect = hp->objRect;
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

	LinkVALID(pL) = FALSE;							/* force objRect recomputation */
}


/* -------------------------------------------------------------- UpdateHairData -- */

static void UpdateHairData(Document */*doc*/, PCONTEXT pContext, LINK pL, HAIRPIN	*hp)
{
	PADYNAMIC	aDynamic;
	PDYNAMIC		pDyn;
	DDIST			firstSyncXD, lastSyncXD, sysLeft, staffTop;
	LINK			firstSyncL, lastSyncL;
	char			dynamicType;
	
	aDynamic = GetPADYNAMIC(FirstSubLINK(pL));
	pDyn = GetPDYNAMIC(pL);
	dynamicType = DynamType(pL);	
	staffTop = pContext->staffTop;
	sysLeft = pContext->systemLeft;
	firstSyncL = DynamFIRSTSYNC(pL);
	lastSyncL = DynamLASTSYNC(pL);
	firstSyncXD = SysRelxd(firstSyncL);
	lastSyncXD = SysRelxd(lastSyncL);

	pDyn->xd = p2d(hp->leftPt.h) - firstSyncXD - sysLeft;
	aDynamic->yd = p2d(hp->leftPt.v) - staffTop;
	aDynamic->endyd = p2d(hp->rightPt.v) - staffTop;
	aDynamic->endxd = p2d(hp->rightPt.h) - lastSyncXD - sysLeft;
	aDynamic->xd = 0;		/* probably unnecessary, since insertion now makes this zero */

/* ?? Someday when it's possible to graphically edit mouth and other
widths, update those fields in the data structure here. */
	
	LinkTWEAKED(pL) = TRUE;
}

