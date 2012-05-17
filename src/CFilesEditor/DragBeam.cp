/***************************************************************************
*	FILE:	DragBeam.c																			*
*	PROJ:	Nightingale, by John Gibson; rev. for v.3.1								*
*	DESC:	Beam dragging routines															*
***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 *
 * Copyright © 1986-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


static enum {
	LGRIP=1,										/* Dragging the left "grip" (handle) */
	RGRIP,										/* Dragging the right "grip" (handle) */
	DRAGBEAM										/* Dragging the entire beam */
} E_DragBeamItems;

#define BOXSIZE	4
#define STEM_UP	1
#define STEM_DOWN	-1

typedef struct {
	Point			leftPt, rightPt;			/* paper coords */
	SignedByte	leftUOD, rightUOD;		/* 1: upstem, -1: downstem */
	INT16			thickness;					/* pixels */
	Rect			objRect;						/* paper coords. For updating purposes only */
} BEAMFEEDBACK;

/* Local Prototypes */
static Boolean QuantizeBeamEndYStems(Document *doc, LINK beamL, DDIST yDiffFirst,
										DDIST yDiffLast, DDIST *pDeltaFirst, DDIST *pDeltaLast);
static Boolean PtInBeam(Document *, Point, BEAMFEEDBACK *);
static void EditBeam(Document *, LINK, INT16, BEAMFEEDBACK *);
static void InitBeamGrips(BEAMFEEDBACK *);
static void GetMaxSecs(LINK, INT16 [], INT16 [], INT16 [], INT16 *, INT16 *);
static void SetBeamFeedback(Document *, LINK, BEAMFEEDBACK *);
static void InitBeamBounds(Document *, LINK, Rect *);
static void DoBeamFeedback(Document *, BEAMFEEDBACK *);
static void DrawBeamGrip(Document *, INT16);
static void DrawBothGrips(Document *);

static Point	lGrip, rGrip;				/* in paper coords */

/* ------------------------------------------------------------------ DoBeamEdit -- */

void DoBeamEdit(Document *doc, LINK beamL)
{
	Point 			mousePt;							/* paper coords */
	EventRecord		beamEventRec;
	Rect				newObjRect, oldObjRect, updateRect;
	BEAMFEEDBACK	origBeam, oldBeam, thisBeam;
	DDIST				yDiffLeft, yDiffRight, deltaFirst, deltaLast;
	INT16				oldGripV, newGripV, grip;

	FlushEvents(everyEvent, 0);
	
	thisBeam.objRect = LinkOBJRECT(beamL);
	SetBeamFeedback(doc, beamL, &thisBeam);
	origBeam = oldBeam = thisBeam;
	InitBeamGrips(&thisBeam);
	DrawBothGrips(doc);
	
	while (TRUE) {
		if (GetNextEvent(updateMask, &beamEventRec)) {
			if (grip != DRAGBEAM)
				DrawBothGrips(doc);											/* erase grips */
			DoUpdate((WindowPtr)(beamEventRec.message));
			ArrowCursor();
			InitBeamGrips(&thisBeam);										/* grip h-pos may change */
			DrawBothGrips(doc);												/* redraw them */
		}
		if (EventAvail(everyEvent, &beamEventRec)) {		
			if (beamEventRec.what == mouseDown) {
				mousePt = beamEventRec.where;
				GlobalToPaper(doc, &mousePt);
				if (!(SamePoint(mousePt, lGrip) ||
						SamePoint(mousePt, rGrip) ||
						PtInBeam(doc, mousePt, &thisBeam)))
					break;
			}
			else if (beamEventRec.what != mouseUp)
				break;
			
			/* If mouse down event, edit hairpin. Swallow mouse up. */		
			GetNextEvent(mDownMask|mUpMask, &beamEventRec);
			if (beamEventRec.what == mouseUp) continue;

			/* Leave initial beam in gray. NB: If you want the beam
			 * to be fainter than staff lines, use dkGray (sic) instead.
			 */
			PenMode(patXor);
			DoBeamFeedback(doc, &thisBeam);								/* erase black beam */
			PenPat(NGetQDGlobalsGray());				/* ???dkGray might be better here; easier to see black beam */
			DoBeamFeedback(doc, &thisBeam);								/* draw it in gray */
			PenNormal();

			if (SamePoint(mousePt, lGrip)) {
				oldGripV = lGrip.v;
				EditBeam(doc, beamL, grip=LGRIP, &thisBeam);
				newGripV = lGrip.v;
			}
			else if (SamePoint(mousePt, rGrip)) {
				oldGripV = rGrip.v;
				EditBeam(doc, beamL, grip=RGRIP, &thisBeam);
				newGripV = rGrip.v;
			}
			else if (PtInRect(mousePt, &thisBeam.objRect)) {
				oldGripV = lGrip.v;
				EditBeam(doc, beamL, grip=DRAGBEAM, &thisBeam);
				newGripV = lGrip.v;
			}

			yDiffLeft = yDiffRight = 0;
			if (grip==LGRIP || grip==DRAGBEAM)
				yDiffLeft = p2d(newGripV - oldGripV);
			if (grip==RGRIP || grip==DRAGBEAM)
				yDiffRight = p2d(newGripV - oldGripV);
		
			/* ??It'd be nice to give user a way to say, don't quantize thin beams.*/
			if ( LXOR(config.quantizeBeamYPos!=0, CmdKeyDown()) )
				if (QuantizeBeamEndYStems(doc, beamL, yDiffLeft, yDiffRight,
													&deltaFirst, &deltaLast)) {
					yDiffLeft += deltaFirst;
					yDiffRight += deltaLast;
				}
				
			/*
			 * Fix stem ends after drag and quantization. Because of the quantization,
			 * both beam endpoints might have moved even if only one was dragged.
			 */
				if (GraceBEAM(beamL)) {
					if (yDiffLeft!=0)
						FixGRStemLengths(beamL, yDiffLeft, LGRIP);
					if (yDiffRight!=0)
						FixGRStemLengths(beamL, yDiffRight, RGRIP);
				}
				else {
					if (yDiffLeft!=0)
						FixStemLengths(beamL, yDiffLeft, LGRIP);
					if (yDiffRight!=0)
						FixStemLengths(beamL, yDiffRight, RGRIP);
				}

			SetBeamFeedback(doc, beamL, &thisBeam);
			
			/* force an update event */
			newObjRect = thisBeam.objRect;
			oldObjRect = oldBeam.objRect;
			Rect2Window(doc, &newObjRect);
			Rect2Window(doc, &oldObjRect);
			UnionRect(&oldObjRect, &newObjRect, &updateRect);
#ifndef PUBLIC_VERSION
			if (ShiftKeyDown()) {
				FrameRect(&oldObjRect);				/* to debug update rect */
				SleepTicks(90L);
				FrameRect(&newObjRect);
				SleepTicks(90L);
			}
#endif
			EraseAndInval(&updateRect);

			oldBeam = thisBeam;
		}
	}
	
	if (BlockCompare(&thisBeam, &origBeam, sizeof(BEAMFEEDBACK)))
		doc->changed = TRUE;
	DrawBothGrips(doc);	
	MEHideCaret(doc);	
}


/* ----------------------------------------------- QuantizeBeamEndYStems and ally -- */

/* If the given stem endpoint positions, presumably of the notes beginning and
ending a beam, are within the staff, quantize them to multiples of a quarter space.
This is intended to help get each end of a beam into the "hang", "straddle", or "sit"
position: see Ross' book, p. 98ff. But NB: there are four quarter-space positions:
besides hang, straddle, sit, there's a "center" position that doesn't touch the staff
lines; so if <noCenter>, we also avoid the center position.

Returns TRUE if it changes anything, FALSE if not.

Not recommended for cross-staff beams: their stems will normally be outside the staff,
but even if not, this probably doesn't make sense for them. Also, this assumes
the thickness of the beam is a quarter-space! Most important, this should really
only be used interactively, e.g., in response to the user dragging the beam: there's
a good chance it'll actually make the beam look worse in terms of slope or by
putting the endpoints on different staff lines. */

static Boolean QuantizeYStemsForBeam(CONTEXT context, Boolean stemUp, Boolean noCenter,
										INT16 ledgerSpaces, DDIST *pFirstystem, DDIST *pLastystem);
static Boolean QuantizeYStemsForBeam(
						CONTEXT context,
						Boolean stemUp,
						Boolean noCenter,						/* TRUE=don't allow the "center" position */
						INT16 ledgerSpaces,					/* How far above/below staff to go (spaces) */
						DDIST *pFirstystem, DDIST *pLastystem	/* Both inputs and outputs! */
						)
{
	DDIST firstystem, lastystem, origFirstystem, origLastystem, dSpace, dQuarterSpace,
			dRound, dTolerance;
	INT16 firstMult, lastMult, centerPos;
		
	dSpace = LNSPACE(&context);
	dTolerance = ((ledgerSpaces-1)*dSpace) + (dSpace/2);
	dQuarterSpace = dSpace/4;
	dRound = dQuarterSpace/2;

	/* If either endpoint is well outside the staff, do nothing. */
	
	if (*pFirstystem<-dTolerance || *pFirstystem>context.staffHeight+dTolerance)
		return FALSE;
	if (*pLastystem<-dTolerance || *pLastystem>context.staffHeight+dTolerance)
		return FALSE;
	
	origFirstystem = firstystem = *pFirstystem;
	origLastystem = lastystem = *pLastystem;

	/*
	 * Round firstystem and lastystem to multiples of dQuarterSpace. We must take
	 * the sign into account to avoid annoying inconsistencies.
	 */	
	firstystem = (DDIST)RoundSignedInt((INT16)firstystem, (INT16)dQuarterSpace);
	lastystem = (DDIST)RoundSignedInt((INT16)lastystem, (INT16)dQuarterSpace);
	
	/* NB: as of v. 3.1b39, <noCenter> is never TRUE, so this code isn't well tested. */
	
	if (noCenter) {
		centerPos = (stemUp? 1 : 3);
		/*
		 * We want to avoid the "center" position. If the endpoint is now in that
		 * position, move it to the adjacent position that's closer to the original
		 * value (to do this, we have to take the sign into account). Don't worry
		 * about the slope or keeping the endpoints on the same staff line line--we
		 * assume this is being used interactively, so it's up to the user.
		 */
		firstMult = firstystem/dQuarterSpace;
		lastMult = lastystem/dQuarterSpace;
		if (firstMult%4 == centerPos)
			firstystem += (firstystem>0? dQuarterSpace : -dQuarterSpace);
		if (lastMult%4 == centerPos)
		lastystem += (lastystem>0? dQuarterSpace : -dQuarterSpace);
	}

	if (firstystem==origFirstystem && lastystem==origLastystem) return FALSE;

	*pFirstystem = firstystem;
	*pLastystem = lastystem;	
	return TRUE;
}


/* Quantize the stem endpoints of the notes beginning and ending the given beam to
multiples of a quarter space. This is intended to help get each end of a beam into the
"hang", "straddle", or "sit" position: see Ross' book, p. 98ff. But NB: there are four
quarter-space positions: besides hang, straddle, sit, there's a "center" position that
doesn't touch the staff lines; it's up to the user to avoid the center position.

Returns TRUE if it changes anything, FALSE if not.

Not recommended for cross-staff beams: their stems will normally be outside the staff,
but even if not, this probably doesn't make sense for them. Also, this assumes
the thickness of the beam is a quarter-space! Most important, this should really
only be used interactively, e.g., in response to the user dragging the beam: there's
a good chance it'll actually make the beam look worse in terms of slope or by
putting the endpoints on different staff lines. */

static Boolean QuantizeBeamEndYStems(
						Document *doc, LINK beamL,
						DDIST yDiffFirst, DDIST yDiffLast,		/* Offsets on current positions */
						DDIST *pDeltaFirst, DDIST *pDeltaLast	/* Outputs */
						)
{
	CONTEXT context;
	LINK firstSyncL, lastSyncL, firstNoteL, lastNoteL;
	DDIST firstystem, lastystem, origFirstystem, origLastystem;
	Boolean stemDown;
	Boolean ledgerSpaces;
	
	GetContext(doc, beamL, BeamSTAFF(beamL), &context);
	firstSyncL = FirstInBeam(beamL);
	lastSyncL = LastInBeam(beamL);
	firstNoteL = FindMainNote(firstSyncL, BeamVOICE(beamL));
	lastNoteL = FindMainNote(lastSyncL, BeamVOICE(beamL));
	firstystem = NoteYSTEM(firstNoteL)+yDiffFirst;
	lastystem = NoteYSTEM(lastNoteL)+yDiffLast;

	stemDown = (NoteYD(firstNoteL) < NoteYSTEM(firstNoteL));

	/*
	 * Set the range to quantize from the config field, unless it's zero. In the latter
	 * case, we have no information to set it from, so make it "infinity".
	 */
	if (config.quantizeBeamYPos==0)
		ledgerSpaces = MAX_LEDGERS;
	else {
		if (config.quantizeBeamYPos<=2)
			ledgerSpaces = config.quantizeBeamYPos;
		else
			ledgerSpaces = MAX_LEDGERS;
	}

	origFirstystem = firstystem;
	origLastystem = lastystem;
	
	if (QuantizeYStemsForBeam(context, !stemDown, FALSE, ledgerSpaces, &firstystem,
			&lastystem)) {
		*pDeltaFirst = firstystem-origFirstystem;
		*pDeltaLast = lastystem-origLastystem;
		return TRUE;
	}
	
	return FALSE;
}


/* ------------------------------------------------------------------- PtInBeam -- */

static Boolean PtInBeam(
						Document			*doc,
						Point				pt,			/* paper-rel coords */
						BEAMFEEDBACK	*bm
						)
{
	Point lPtUp, lPtDn, rPtUp, rPtDn;
	register RgnHandle beamRgn;
	short		thick,
				slop = 3;				/* pixels, regardless of magnification */
	Boolean	isInBeam;

	thick = bm->thickness;
	lPtUp = bm->leftPt;				rPtUp = bm->rightPt;
	Pt2Window(doc, &lPtUp);			Pt2Window(doc, &rPtUp);
	lPtDn = lPtUp;						rPtDn = rPtUp;
	
	if (bm->leftUOD==STEM_DOWN)	{	lPtUp.v -= thick;		lPtDn.v++;			}
	else 									{	lPtUp.v--;				lPtDn.v += thick;	}
	if (bm->rightUOD==STEM_DOWN)	{	rPtUp.v -= thick;		rPtDn.v++;			}
	else 									{	rPtUp.v--;				rPtDn.v += thick;	}
	
	lPtUp.v -= slop;					rPtUp.v -= slop;
	lPtDn.v += slop;					rPtDn.v += slop;
	
	/* Define a region that encloses just the 1st primary beam + vertical slop... */
	beamRgn = NewRgn();
	OpenRgn();
#ifdef DBDEBUG						/* for debugging rgn bounds */
	ShowPen();
#endif
	MoveTo(lPtDn.h, lPtDn.v);
	LineTo(lPtUp.h, lPtUp.v);
	LineTo(rPtUp.h, rPtUp.v);
	LineTo(rPtDn.h, rPtDn.v);
	LineTo(lPtDn.h, lPtDn.v);
	CloseRgn(beamRgn);

	/* ...and see if the point is in this region */
	Pt2Window(doc, &pt);
	isInBeam = PtInRgn(pt, beamRgn);
	
	DisposeRgn(beamRgn);
	return isInBeam;
}


/* ------------------------------------------------------------------- EditBeam -- */
/*	Handle the actual dragging of the beam.
<grip> tells which mode of dragging the beam is in effect.
<mousePt> and grip points are in paper-relative coordinates. */

static void EditBeam(
					Document			*doc,
					LINK				beamL,
					INT16	 			grip,			/* Edit mode: LGRIP, RGRIP, DRAGBEAM */
					BEAMFEEDBACK	*bm
					)
{
	Point		oldPt, newPt;
	long		aLong;
	Rect		boundsRect;				/* in paper coords */
	INT16		dv;
		
	if (grip==DRAGBEAM)								/* Erase the handles prior to dragging the beam. */
		DrawBothGrips(doc);

	InitBeamBounds(doc, beamL, &boundsRect);

	GetPaperMouse(&oldPt, &doc->currentPaper);
	newPt = oldPt;

	if (StillDown()) {
		while (WaitMouseUp()) {
			GetPaperMouse(&newPt, &doc->currentPaper);

			/* Force the code below to see the mouse as always within boundsRect. */
			aLong = PinRect(&boundsRect, newPt);
			newPt.v = HiWord(aLong);	newPt.h = LoWord(aLong);

			if (EqualPt(newPt, oldPt)) continue;
			
			dv = newPt.v - oldPt.v;
			
			PenMode(patXor);
			DoBeamFeedback(doc, bm);								/* erase old beam */
			
			switch (grip) {
				case DRAGBEAM:
					lGrip.v += dv;
					rGrip.v += dv;
					bm->leftPt.v += dv;
					bm->rightPt.v += dv;
					break;
				case LGRIP:
					DrawBeamGrip(doc, LGRIP);							/* erase grip */
					lGrip.v += dv;
					bm->leftPt.v += dv;
					break;
				case RGRIP:
					DrawBeamGrip(doc, RGRIP);
					rGrip.v += dv;
					bm->rightPt.v += dv;
					break;
			}
			AutoScroll();								/* ??Only problem: old beam drawn in black when re-emerging; not too serious. */

			if (grip != DRAGBEAM)
				DrawBeamGrip(doc, grip);
			DoBeamFeedback(doc, bm);									/* draw new beam */
			
			oldPt = newPt;
		}
	}
	DoBeamFeedback(doc, bm);											/* erase old beam */
	PenNormal();
}


/* -------------------------------------------------------------- InitBeamGrips -- */
/* Set up handles for manipulating beam slant. */

static void InitBeamGrips(BEAMFEEDBACK *bm)
{
	SetPt(&lGrip, bm->leftPt.h-5, bm->leftPt.v);
	SetPt(&rGrip, bm->rightPt.h+6, bm->rightPt.v);
	if (bm->leftUOD != bm->rightUOD) {						/* center or cross-staff beam */
		if (bm->leftUOD==STEM_DOWN)
			rGrip.v += bm->thickness;
		else
			rGrip.v -= bm->thickness;
	}
}


/* ------------------------------------------------------------------ GetMaxSecs -- */
/* Get the max. no. of secondary beams above and below primaries in the given beam. */

static void GetMaxSecs(
					LINK		beamL,
					INT16		nSecs[],
					INT16		nSecsA[],
					INT16		nSecsB[],
					INT16		*pMaxAbove,
					INT16		*pMaxBelow
					)
{
	INT16 numEntries, maxAbove, maxBelow; register INT16 n;
	
	numEntries = LinkNENTRIES(beamL);
	maxAbove = maxBelow = 0;

	if (GraceBEAM(beamL)) {
		for (n=0; n<numEntries; n++) {
			if (nSecs[n] < 0 && nSecs[n] < maxAbove)
				maxAbove = nSecs[n];
			else if (nSecs[n] > 0 && nSecs[n] > maxBelow)
				maxBelow = nSecs[n];
		}
	}
	else {
		for (n=0; n<numEntries; n++) {
			if (nSecsA[n] > 0 && nSecsA[n] > maxAbove)
				maxAbove = nSecsA[n];
			if (nSecsB[n] > 0 && nSecsB[n] > maxBelow)
				maxBelow = nSecsB[n];
		}
		maxAbove = -maxAbove;
	}
	
	*pMaxAbove = maxAbove;
	*pMaxBelow = maxBelow;
}

/* ------------------------------------------------------------- SetBeamFeedback -- */
/* Set up the BEAMFEEDBACK struct used in drawing the beam feedback animation,
which consists only of the first primary beam. Set its tmpObjRect field, used for
calculating the area of the screen to update after each call to EditBeam. NB: Beams
for upstemmed notes hang from bm->leftPt (or bm->rightPt), whereas those for
downstemmed notes sit on the points. */

/* ???changes needed:
	+- If more than 1 primary, beam feedback is bad. Which primary do we use
		as beam feedback? The one that's closer to stem end of left note.
		
	- NB: ystems not set correctly after dragging grip of a cross-sys beam
	- ystems not set correctly for cross-STAFF beams (too high by 16 DDIST?).	
		I think both of these are the fault of FixStemLengths though. (Happens
		with old beam drag code also.) It's not AdjustYStem's fault, because
		that just dumbly offsets current ystem.
*/

#define BEAMTABLEN 300	/* Should be enough: even 127 alternate 128ths & 8ths need only 257 */

static void SetBeamFeedback(Document *doc, LINK beamL, BEAMFEEDBACK *bm)
{
	PBEAMSET		beamP;
	BEAMINFO		beamTab[BEAMTABLEN];
	INT16			firstStaff, lastStaff, voice, stemUOD, n, whichWay, beamCount,
					maxAbove, maxBelow, nPrimary, fracLevUp, fracLevDown;
	INT16			nSecs[MAXINBEAM], nSecsA[MAXINBEAM], nSecsB[MAXINBEAM];
	DDIST			sysLeft, firstStaffTop, lastStaffTop, firstSyncXD, lastSyncXD, beamThick,
					firstXStem, lastXStem, leftYstem, rightYstem, flagYDelta, xtraWidD, dQuarterSp;
	LINK			firstSyncL, lastSyncL, firstNoteL, lastNoteL, staffL, aStaffL, measL;
	CONTEXT		firstContext, lastContext;
	SignedByte	stemUpDown[MAXINBEAM];

	/* Initialize variables */
	
	voice = BeamVOICE(beamL);
	beamP = GetPBEAMSET(beamL);
	
	firstSyncL = FirstInBeam(beamL);
	lastSyncL = LastInBeam(beamL);

	if (GraceBEAM(beamL)) {
		firstNoteL = FindGRMainNote(firstSyncL, voice);				
		lastNoteL = FindGRMainNote(lastSyncL, voice);				
		firstStaff = GRNoteSTAFF(firstNoteL);
		lastStaff = GRNoteSTAFF(lastNoteL);
		leftYstem = GRNoteYSTEM(firstNoteL);
		rightYstem = GRNoteYSTEM(lastNoteL);
	}
	else {
		firstNoteL = FindMainNote(firstSyncL, voice);				
		lastNoteL = FindMainNote(lastSyncL, voice);				
		firstStaff = NoteSTAFF(firstNoteL);
		lastStaff = NoteSTAFF(lastNoteL);
		leftYstem = NoteYSTEM(firstNoteL);
		rightYstem = NoteYSTEM(lastNoteL);
	}	
	firstSyncXD = SysRelxd(firstSyncL);
	lastSyncXD = SysRelxd(lastSyncL);

	GetContext(doc, beamL, firstStaff, &firstContext);			/* separate in case beam spans staves of different rastrals */
	GetContext(doc, beamL, lastStaff, &lastContext);
	sysLeft = firstContext.systemLeft;
	
	staffL = LSSearch(lastSyncL, STAFFtype, lastStaff, GO_LEFT, FALSE);
	aStaffL = StaffOnStaff(staffL, firstStaff);
	firstStaffTop = StaffTOP(aStaffL) + firstContext.systemTop;
	aStaffL = StaffOnStaff(staffL, lastStaff);
	lastStaffTop = StaffTOP(aStaffL) + firstContext.systemTop;

	if (GraceBEAM(beamL)) {
		whichWay = AnalyzeGRBeamset(beamL, &nPrimary, nSecs);
		beamCount = BuildGRBeamDrawTable(beamL, whichWay, nPrimary, nSecs, beamTab, BEAMTABLEN);
	}
	else {
		whichWay = AnalyzeBeamset(beamL, &nPrimary, nSecsA, nSecsB, stemUpDown);
		beamCount = BuildBeamDrawTable(beamL, whichWay, nPrimary, nSecsA, nSecsB,
													stemUpDown, beamTab, BEAMTABLEN);
	}
	flagYDelta = FlagLeading(LNSPACE(&firstContext));		/* use first staff's lnSp for both staves? */


	/* Fill in BEAMFEEDBACK struct. First, set up leftPt and rightPt. */
	
	stemUOD = ( (leftYstem < NoteYD(firstNoteL)) ? STEM_UP : STEM_DOWN );
	measL = LSSearch(firstSyncL, MEASUREtype, firstStaff, GO_LEFT, FALSE);
	if (GraceBEAM(beamL))
		firstXStem = CalcGRXStem(doc, firstSyncL, voice, stemUOD, LinkXD(measL), &firstContext, TRUE);
	else
		firstXStem = CalcXStem(doc, firstSyncL, voice, stemUOD, LinkXD(measL), &firstContext, TRUE);
	bm->leftUOD = stemUOD;
	
	stemUOD = ( (rightYstem < NoteYD(lastNoteL)) ? STEM_UP : STEM_DOWN );
	measL = LSSearch(lastSyncL, MEASUREtype, lastStaff, GO_LEFT, FALSE);
	if (GraceBEAM(beamL))
		lastXStem = CalcGRXStem(doc, lastSyncL, voice, stemUOD, LinkXD(measL), &lastContext, FALSE);
	else
		lastXStem = CalcXStem(doc, lastSyncL, voice, stemUOD, LinkXD(measL), &lastContext, FALSE);
	bm->rightUOD = stemUOD;
	
	SetPt(&bm->leftPt, d2p(firstXStem+sysLeft), d2p(firstStaffTop+leftYstem));
	SetPt(&bm->rightPt, d2p(lastXStem+sysLeft), d2p(lastStaffTop+rightYstem));
		
	/* Now extend beam past stem if it's cross-system (cf. DrawBEAMSET). */	
	if (beamP->crossSystem) {
		dQuarterSp = LNSPACE(&firstContext)/4;
		if (beamP->firstSystem)	bm->rightPt.h += d2p(config.crossStaffBeamExt*dQuarterSp);
		else							bm->leftPt.h -= d2p(config.crossStaffBeamExt*dQuarterSp);
	}

	/* If there's more than 1 primary beam and the beamset is a center (or cross-staff) beam,
	 * use as the feedback beam the primary that is closer to the stem end of the left note.
	 */
	if (nPrimary>1 && (bm->leftUOD != bm->rightUOD)) {
		if (bm->leftUOD==STEM_DOWN)
			bm->rightPt.v += d2p((nPrimary-1) * flagYDelta);
		else
			bm->rightPt.v -= d2p((nPrimary-1) * flagYDelta);
	}

	/* Derive beam thickness */
	
	beamThick = LNSPACE(&firstContext)/2;							/* get thickness from first staff rastral */
	if (beamP->thin) beamThick /= 2;
	if (GraceBEAM(beamL)) {
		beamThick = GRACESIZE(beamThick);
		beamThick = beamThick<p2d(1)? p2d(1) : beamThick;
	}
	else { 		/* Be sure normal (thin) beams are at least 2 (1) pixels thick. */	
		if (beamP->thin)	beamThick = beamThick<p2d(1)? p2d(1) : beamThick;
		else					beamThick = beamThick<p2d(2)? p2d(2) : beamThick;	
	}
	bm->thickness = d2p(beamThick);

	/* Determine TMPOBJRECT */
	
	/* Determine right and left of objRect, taking into account any
	 * otherstemside notes, accidentals and dots.
	 */
	 
	xtraWidD = SymDWidthLeft(doc, firstSyncL, firstStaff, firstContext);
	bm->objRect.left = d2p(sysLeft + SysRelxd(firstSyncL) + NoteXD(firstNoteL) - xtraWidD);
	xtraWidD = SymDWidthRight(doc, lastSyncL, lastStaff, FALSE, lastContext);
	bm->objRect.right = d2p(sysLeft + SysRelxd(lastSyncL) + NoteXD(lastNoteL) + xtraWidD);

	/* in case beam has a cross-system extension */
	bm->objRect.left = n_min(bm->leftPt.h, bm->objRect.left);
	bm->objRect.right = n_max(bm->rightPt.h, bm->objRect.right);
	
	/* Determine top and bottom of objRect.
	 * NB: This yields a larger rect than necessary in some cases, but that's better
	 * than one that's too small. All it means is that a larger rect will be passed
	 * to EraseAndInval. It doesn't seem worth the effort to be more precise, unless
	 * we're often forcing adjacent measures to be redrawn unnecessarily.
	 */
	
	/* Initialize top and bottom from note stem ends */
	bm->objRect.top = n_min(bm->leftPt.v, bm->rightPt.v);
	bm->objRect.bottom = n_max(bm->leftPt.v, bm->rightPt.v);

	/* adjust top and bottom for thickness of 1st primary beam */
	if (bm->leftUOD==STEM_DOWN || bm->rightUOD==STEM_DOWN)
		bm->objRect.top -= bm->thickness;
	if (bm->leftUOD==STEM_UP || bm->rightUOD==STEM_UP)
		bm->objRect.bottom += bm->thickness;

	/* ...and for any other primary, secondary and fractional beams */
	GetMaxSecs(beamL, nSecs, nSecsA, nSecsB, &maxAbove, &maxBelow);

	if (nPrimary > 1) {
		bm->objRect.top -= d2p((nPrimary-1) * flagYDelta);
		bm->objRect.bottom += d2p((nPrimary-1) * flagYDelta);
	}
	if (maxAbove)
		bm->objRect.top += d2p(maxAbove * flagYDelta);
	if (maxBelow)
		bm->objRect.bottom += d2p(maxBelow * flagYDelta);
	
	/* Make sure rect encloses any fractional beams */
	fracLevUp = fracLevDown = 0;
	for (n=0; n<beamCount; n++) {
		if (beamTab[n].start==beamTab[n].stop) {						/* it's a fractional beam */
			if (beamTab[n].startLev < 0 && beamTab[n].startLev < fracLevUp)
				fracLevUp = beamTab[n].startLev;
			else if (beamTab[n].startLev > 0 && beamTab[n].startLev > fracLevDown)
				fracLevDown = beamTab[n].startLev;
		}
	}
	if (fracLevUp < maxAbove)									/* fractional exceeds all secondary beams */
		bm->objRect.top += d2p(flagYDelta*(fracLevUp-maxAbove));
	if (fracLevDown > maxBelow)
		bm->objRect.bottom += d2p(flagYDelta*(fracLevDown-maxBelow));

	/* Finally, add some slop */
	bm->objRect.left -= pt2p(1);
	bm->objRect.top -= pt2p(1);
	bm->objRect.right += pt2p(2);
	bm->objRect.bottom += pt2p(2);
}


/* -------------------------------------------------------------- InitBeamBounds -- */
/* Set up the rectangle beyond which the user won't be allowed to drag the beam. */

#define SYSHT_SLOP pt2d(8)		/* DDIST */

static void InitBeamBounds(
					Document *doc,
					LINK		beamL,
					Rect		*bounds 				/* paper coords */
					)
{
	INT16		staffn;
	CONTEXT	context;

	staffn = BeamSTAFF(beamL);												/* doesn't matter which one of beam's staves */
	GetContext(doc, beamL, staffn, &context);
	
	bounds->top = d2p(context.systemTop - SYSHT_SLOP);
	bounds->bottom = d2p(context.systemBottom + SYSHT_SLOP);
	bounds->left = d2p(context.systemLeft);							/* really irrelevant for beams, because no horiz. motion allowed */
	bounds->right = d2p(context.staffRight);
}


/* --------------------------------------------------------------- DoBeamFeedback -- */
/* Draw the feedback beam using the current pen characteristics (penPat and penMode). */

static void DoBeamFeedback(Document *doc, BEAMFEEDBACK *bm)
{
	Point	lPt, rPt;
	short	thick;

	lPt = bm->leftPt;
	rPt = bm->rightPt;
	Pt2Window(doc, &lPt);
	Pt2Window(doc, &rPt);
	thick = bm->thickness;
	
	if (bm->leftUOD == STEM_DOWN)
		lPt.v -= thick-1;
	if (bm->rightUOD == STEM_DOWN)
		rPt.v -= thick-1;
	
	PenSize(1, thick);
	MoveTo(lPt.h, lPt.v);
	LineTo(rPt.h, rPt.v);
	PenSize(1, 1);
}


/* --------------------------------------------------------------- DrawBeamGrip -- */
/* Draw a small box for the left or right grip of the feedback beam. */

static void DrawBeamGrip(
					Document *doc,
					INT16		whichOne 		/* LGRIP or RGRIP */
					)
{
	Point gripPt;

	if (whichOne==LGRIP)	gripPt = lGrip;
	else						gripPt = rGrip;
	Pt2Window(doc, &gripPt);
	DrawBox(gripPt, BOXSIZE);
}


/* ------------------------------------------------------------- DrawBothGrips -- */

static void DrawBothGrips(Document *doc)
{
	PenMode(patXor);
	DrawBeamGrip(doc, LGRIP);			
	DrawBeamGrip(doc, RGRIP);			
	PenNormal();
}


/* ------------------------------------------------------------- FixStemLengths -- */
/* After the beam has been dragged, fix the stem lengths of the notes in 
the beamset. */

Boolean FixStemLengths(LINK beamL, DDIST yDiff, INT16 grip)
{
	DDIST		hDiff,						/* horiz. distance btw. 1st and last syncs in beam */
				noteDiff, stemChange;
	LINK		qL, firstSyncL, lastSyncL, qSubL;
	PANOTE	qSub;
	short		h, stemUp, oldStemUp;
	FASTFLOAT 	fhDiff, fnoteDiff, fyDiff;
	Boolean	stemDirChange, didFixChordForYStem = FALSE;

	firstSyncL = FirstInBeam(beamL);
	lastSyncL = LastInBeam(beamL);
	hDiff = SysRelxd(lastSyncL)-SysRelxd(firstSyncL);

	switch (grip) {
		case RGRIP:
		case LGRIP:
			for (h=0, qL=RightLINK(beamL); qL && h<LinkNENTRIES(beamL); qL=RightLINK(qL)) {
				stemDirChange = FALSE;
				if (ObjLType(qL)==SYNCtype) {
					for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextNOTEL(qSubL)) {
						if ((NoteVOICE(qSubL)==BeamVOICE(beamL)) && MainNote(qSubL)) {
							qSub = GetPANOTE(qSubL);
							if (qSub->beamed) {
								if ((qL==firstSyncL && grip==LGRIP) || (qL==lastSyncL && grip==RGRIP))
									stemChange = yDiff;
								else {
							 		noteDiff = SysRelxd(qL)-SysRelxd(firstSyncL);
									if (hDiff) {	/* ??why? */
										if (grip==LGRIP)
											fnoteDiff = hDiff-noteDiff;
										else
											fnoteDiff = noteDiff;
										fhDiff = hDiff;	fyDiff = yDiff;
										stemChange = (DDIST)fyDiff*(fnoteDiff/fhDiff);
									}
									else stemChange = 0;
								}
								if (qSub->ystem>qSub->yd) oldStemUp = -1;		/* get current stem direction */
								else oldStemUp = 1;
						 		qSub->ystem += stemChange;							/* change stem length */
								if (qSub->ystem>qSub->yd) stemUp = -1;			/* get new stem direction */
								else stemUp = 1;
								if (oldStemUp != stemUp) stemDirChange = TRUE;
							}
							else if (!NoteREST(qSubL))
								MayErrMsg("FixStemLengths: Unbeamed note in sync %ld where beamed note expected", (long)qL);
							if (!NoteREST(qSubL) || qSub->beamed)
								h++;
						}
					}
					/* Call FixChordForYStem only if stem direction has changed. Otherwise,
						we'll be throwing away accidental hpos tweaks unnecessarily.
						Must use a new loop, since FixChordForYStem can change the main note
						and cause the block which handles MainNote(qSubL) to be called
						too many times. 
					*/
					if (!stemDirChange) continue;
					for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextNOTEL(qSubL)) {
						if ((NoteVOICE(qSubL)==BeamVOICE(beamL)) && MainNote(qSubL)) {
							qSub = GetPANOTE(qSubL);
							if (qSub->inChord) {
								if (qSub->ystem>qSub->yd) stemUp = -1;
								else stemUp = 1;
								FixChordForYStem(qL, NoteVOICE(qSubL), stemUp, qSub->ystem);
								didFixChordForYStem = TRUE;
								break;
							}
						}
					}
				}
			}
			break;

		case DRAGBEAM:
			for (h=0, qL=RightLINK(beamL); qL && h<LinkNENTRIES(beamL); qL=RightLINK(qL)) {
				stemDirChange = FALSE;
				if (ObjLType(qL)==SYNCtype) {
					for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextNOTEL(qSubL)) {
						if ((NoteVOICE(qSubL)==BeamVOICE(beamL)) && MainNote(qSubL)) {
							qSub = GetPANOTE(qSubL);
							if (qSub->beamed) {
								if (qSub->ystem>qSub->yd) oldStemUp = -1;		/* get current stem direction */
								else oldStemUp = 1;
						 		qSub->ystem += yDiff;								/* change stem length */
								if (qSub->ystem>qSub->yd) stemUp = -1;			/* get new stem direction */
								else stemUp = 1;
								if (oldStemUp != stemUp) stemDirChange = TRUE;
							}
							else
								if (!NoteREST(qSubL))
									MayErrMsg("FixStemLengths: Unbeamed note in sync %ld where beamed note expected", (long)qL);

							if (!NoteREST(qSubL) || qSub->beamed)
								h++;
						}
					}
					/* Call FixChordForYStem only if stem direction has changed. Otherwise,
						we'll be throwing away accidental hpos tweaks unnecessarily.
						Must use a new loop, since FixChordForYStem can change the main note
						and cause the block which handles MainNote(qSubL) to be called
						too many times. 
					*/
					if (!stemDirChange) continue;
					for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextNOTEL(qSubL)) {
						if ((NoteVOICE(qSubL)==BeamVOICE(beamL)) && MainNote(qSubL)) {
							qSub = GetPANOTE(qSubL);
							if (qSub->inChord) {
								if (qSub->ystem>qSub->yd) stemUp = -1;
								else stemUp = 1;
								FixChordForYStem(qL, NoteVOICE(qSubL), stemUp, qSub->ystem);
								didFixChordForYStem = TRUE;
								break;
							}
						}
					}
				}
			}
			break;
		default:
			break;
	}
	
	return didFixChordForYStem;
}


/* ----------------------------------------------------------- FixGRStemLengths -- */
/* After the beam has been dragged, fix the stem lengths of the grace notes
in the beamset. */

Boolean FixGRStemLengths(LINK beamL, DDIST yDiff, INT16 grip)
{
	DDIST		hDiff,						/* horiz. distance btw. 1st and last syncs in beam */
				noteDiff, stemChange;
	LINK		qL, firstGRSyncL, lastGRSyncL, qSubL;
	PANOTE	qSub;
	short		h, stemUp, oldStemUp;
	FASTFLOAT 	fhDiff, fnoteDiff, fyDiff;
	Boolean	stemDirChange, didFixChordForYStem = FALSE;

	firstGRSyncL = FirstInBeam(beamL);
	lastGRSyncL = LastInBeam(beamL);
	hDiff = SysRelxd(lastGRSyncL)-SysRelxd(firstGRSyncL);

	switch (grip) {
		case RGRIP:
		case LGRIP:
			for (h=0, qL=RightLINK(beamL); qL && h<LinkNENTRIES(beamL); qL=RightLINK(qL)) {
				stemDirChange = FALSE;
				if (GRSyncTYPE(qL)) {
					for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextGRNOTEL(qSubL)) {
						if ((GRNoteVOICE(qSubL)==BeamVOICE(beamL)) && GRMainNote(qSubL)) {
							qSub = GetPAGRNOTE(qSubL);
							if (qSub->beamed) {
								if ((qL==firstGRSyncL && grip==LGRIP) || (qL==lastGRSyncL && grip==RGRIP))
									stemChange = yDiff;
								else {
							 		noteDiff = SysRelxd(qL)-SysRelxd(firstGRSyncL);
									if (hDiff) {	/* ??why? */
										if (grip==LGRIP)
											fnoteDiff = hDiff-noteDiff;
										else
											fnoteDiff = noteDiff;
										fhDiff = hDiff;	fyDiff = yDiff;
										stemChange = (DDIST)fyDiff*(fnoteDiff/fhDiff);
									}
									else stemChange = 0;
								}
								if (qSub->ystem>qSub->yd) oldStemUp = -1;		/* get current stem direction */
								else oldStemUp = 1;
						 		qSub->ystem += stemChange;							/* change stem length */
								if (qSub->ystem>qSub->yd) stemUp = -1;			/* get new stem direction */
								else stemUp = 1;
								if (oldStemUp != stemUp) stemDirChange = TRUE;
							}
							else
								MayErrMsg("FixGRStemLengths: Unbeamed note in sync %ld where beamed note expected", (long)qL);
							h++;
						}
					}
					/* Call FixGRChordForYStem only if stem direction has changed. Otherwise,
						we'll be throwing away accidental hpos tweaks unnecessarily.
						Must use a new loop, since FixGRChordForYStem can change the main note
						and cause the block which handles GRMainNote(qSubL) to be called
						too many times. 
					*/
					if (!stemDirChange) continue;
					for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextGRNOTEL(qSubL)) {
						if ((GRNoteVOICE(qSubL)==BeamVOICE(beamL)) && GRMainNote(qSubL)) {
							qSub = GetPAGRNOTE(qSubL);
							if (qSub->inChord) {
								if (qSub->ystem>qSub->yd) stemUp = -1;
								else stemUp = 1;
								FixGRChordForYStem(qL, GRNoteVOICE(qSubL), stemUp, qSub->ystem);
								didFixChordForYStem = TRUE;
								break;
							}
						}
					}
				}
			}
			break;

		case DRAGBEAM:
			for (h=0, qL=RightLINK(beamL); qL && h<LinkNENTRIES(beamL); qL=RightLINK(qL)) {
				stemDirChange = FALSE;
				if (GRSyncTYPE(qL)) {
					for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextGRNOTEL(qSubL)) {
						if ((GRNoteVOICE(qSubL)==BeamVOICE(beamL)) && GRMainNote(qSubL)) {
							qSub = GetPAGRNOTE(qSubL);
							if (qSub->beamed) {
								if (qSub->ystem>qSub->yd) oldStemUp = -1;		/* get current stem direction */
								else oldStemUp = 1;
						 		qSub->ystem += yDiff;								/* change stem length */
								if (qSub->ystem>qSub->yd) stemUp = -1;			/* get new stem direction */
								else stemUp = 1;
								if (oldStemUp != stemUp) stemDirChange = TRUE;
							}
							else
								MayErrMsg("FixGRStemLengths: Unbeamed note in sync %ld where beamed note expected", (long)qL);
							h++;
						}
					}
					/* Call FixGRChordForYStem only if stem direction has changed. Otherwise,
						we'll be throwing away accidental hpos tweaks unnecessarily.
						Must use a new loop, since FixGRChordForYStem can change the main note
						and cause the block which handles GRMainNote(qSubL) to be called
						too many times. 
					*/
					if (!stemDirChange) continue;
					for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextGRNOTEL(qSubL)) {
						if ((GRNoteVOICE(qSubL)==BeamVOICE(beamL)) && GRMainNote(qSubL)) {
							qSub = GetPAGRNOTE(qSubL);
							if (qSub->inChord) {
								if (qSub->ystem>qSub->yd) stemUp = -1;
								else stemUp = 1;
								FixGRChordForYStem(qL, GRNoteVOICE(qSubL), stemUp, qSub->ystem);
								didFixChordForYStem = TRUE;
								break;
							}
						}
					}
				}
			}
			break;
		default:
			break;
	}
	
	return didFixChordForYStem;
}
