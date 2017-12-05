/***************************************************************************
	FILE:	RfmtRaw.c
	PROJ:	Nightingale
	DESC:	Routines for "raw" reformatting: respaces and assumes a very simple
			range. Intended for use by recording routines. Cf. Reformat.c.

	UMoveInMeasure			UMoveMeasures			UMoveRestOfSystem
	USysRelxd				AddSysMaybePage
	RespaceRaw				BreakSystem				RespAndRfmtRaw
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

LINK UMoveInMeasure(LINK, LINK, DDIST);
void UMoveMeasures(LINK, LINK, DDIST);
void UMoveRestOfSystem(Document *, LINK, DDIST);
unsigned DDIST USysRelxd(LINK);
LINK AddSysMaybePage(Document *, LINK);
DDIST RespaceRaw(Document *, LINK, LINK, long);

/* ----------------------------------------------- UMoveInMeasure, UMoveMeasures -- */

#define LinkUXD(link)		( *(unsigned DDIST *)((3*sizeof(LINK)) + LinkToPtr(OBJheap,link)) )

/* UMoveInMeasure moves objects [startL,endL) by diffxd. If endL is not in the
same Measure as startL, it'll stop moving at the end of the Measure. Identical to
MoveInMeasure (q.v.) except that it treats xd's as unsigned, instead of signed,
numbers. */

LINK UMoveInMeasure(LINK startL, LINK endL, DDIST diffxd)
{
	register LINK pL,nextL;

	pL = nextL = FirstValidxd(startL, GO_RIGHT);
	if (!IsAfter(pL,endL)) return endL;

	for ( ; pL!=endL && !MeasureTYPE(pL) && !J_STRUCTYPE(pL); pL=RightLINK(pL))
		if (pL==nextL) {
			nextL = FirstValidxd(RightLINK(pL), GO_RIGHT);
			LinkUXD(pL) += diffxd;
		}

	return pL;
}


/* UMoveMeasures moves Measure objects in [startL,endL) by diffxd. Identical to
MoveMeasures (q.v.) except that it treats xd's as unsigned, instead of signed,
numbers. */

void UMoveMeasures(LINK startL, LINK endL, DDIST diffxd)
{
	register LINK pL;

	for (pL = startL; pL!=endL && !J_STRUCTYPE(pL); pL=RightLINK(pL))
		if (MeasureTYPE(pL))
			LinkUXD(pL) += diffxd;
}


/* -------------------------------------------------------------- UMoveRestOfSystem -- */
/* Given a Measure and the width of that Measure, if there's anything following
in that Measure's System, move that following stuff left or right as appropriate
so it starts where the Measure ends. Identical to CanMoveRestOfSystem (q.v.) except
that it treats xd's as unsigned, instead of signed, numbers. ??SHOULD IT JUST
SKIP SetMeasFillSystem, SINCE IT'S NOT NEEDED AND CAN'T BE RELIABLE? */

void UMoveRestOfSystem(Document *doc, LINK measL, DDIST measWidth)
{
	LINK nextMeasL, endSysL;
	DDIST diffxd;
	
	nextMeasL = LinkRMEAS(measL);
	if (nextMeasL) {
		/*
		 *	Distance to shift is the right end of the Measure minus the left
		 *	end of the Measure following.
		 */
		diffxd = LinkUXD(measL)+measWidth-LinkUXD(nextMeasL);
		if (diffxd!=0) {
			endSysL = EndSystemSearch(doc, measL);
			if (!IsAfter(endSysL, nextMeasL)) {
				UMoveMeasures(nextMeasL, endSysL, diffxd);
			}
		}
	}
}


/* ------------------------------------------------------------------- USysRelxd -- */
/* Return object's xd relative to the System, regardless of the object's type.
Identical to SysRelxd (q.v.) except that it treats xd's as unsigned, instead of
signed, numbers. */

unsigned DDIST USysRelxd(register LINK pL)
{
	LINK measL;
	
	if (MeasureTYPE(pL)) return (LinkUXD(pL));
	else {
		measL = LSSearch(LeftLINK(pL), MEASUREtype, ANYONE, True, False);
		if (measL) {
			if (SameSystem(measL, pL))
				return (LinkUXD(measL)+LinkUXD(pL));
			else return (LinkUXD(pL));							/* before 1st measure of its system */
		}
		else
			return (LinkUXD(pL));								/* before 1st measure of first system */
	}
}


/* ------------------------------------------------------------------- RespaceRaw -- */
/* Respace the range [startL,endL). The range may not contain any type J_IP or J_IT
symbols other than Syncs; in particular, it may not contain any Measures. In that
respect, this routine is much more specialized than RespaceBars; on the other hand,
there is no limit on the number of nodes in the range, although the respaced result
still must not exceed the maximum DDIST value of 2**15-1. Returns the space needed
by the respaced material. */

#define STFHEIGHT drSize[doc->srastral]	/* For now, assume all staves are same height */

DDIST RespaceRaw(
				Document *doc,
				LINK startL, LINK endL,		/* Range to respace */
				long spaceProp 				/* Use spaceProp/(RESFACTOR*100) of normal spacing */
				)
{
	LONGDDIST position, measPos;
	LINK prevL, pL, measL;
	STDIST idealSp, prevSp, change;
	
	if (startL==endL) return 0;
	
	measL = SSearch(LeftLINK(startL), MEASUREtype, GO_LEFT);
	measPos = LinkXD(measL);
	prevL = FirstValidxd(LeftLINK(startL), GO_LEFT);
	prevSp = SymLikelyWidthRight(doc, prevL, spaceProp);
	position = USysRelxd(prevL)+std2d(prevSp, STFHEIGHT, STFLINES);
	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		LinkXD(pL) = position-measPos;
		idealSp = IdealSpace(doc, SyncMaxDur(doc, pL), spaceProp);
		position += std2d(idealSp, STFHEIGHT, STFLINES);
	}
		
	change = position-USysRelxd(endL);
	return change;
}


/* ----------------------------------------------------------------- BreakSystem -- */
/* Break the System before <breakL>: create a new System below <breakL>'s System,
and move everything from <breakL> to the end of its System down onto the new one. */

LINK BreakSystem(register Document *doc, LINK breakL)
{
	LINK sysTermL, prevSysL, newSysL, prevMeasL, newMeasL, firstMoveMeasL, startL, syncL;
	DRect prevSysRect;
	DDIST xdPrev, xdMove, sysTop;
	long prevMeasDur;
	
	sysTermL = EndSystemSearch(doc, breakL);
	prevSysL = LSSearch(breakL, SYSTEMtype, ANYONE, GO_LEFT, False);

	prevSysRect = SystemRECT(prevSysL);
	sysTop = prevSysRect.bottom;
	newSysL = CreateSystem(doc, LeftLINK(sysTermL), sysTop, SuccSystem);
	if (!newSysL)  { NoMoreMemory(); return NILINK; }
	
	prevMeasL = LSSearch(breakL, MEASUREtype, ANYONE, GO_LEFT, False);
	newMeasL = LSSearch(newSysL, MEASUREtype, ANYONE, GO_RIGHT, False);
		
	/*
	 *	Move the stuff that belongs in the new System to its new location, both
	 *	logically and graphically (i.e., in horizontal position). Add accidentals
	 *	in the new Measure that it formerly inherited from notes that are now in
	 *	the previous Measure.
	 */
	xdPrev = USysRelxd(breakL);
	MoveRange(breakL, newSysL, RightLINK(newMeasL));
	FixStructureLinks(doc, doc, doc->headL, doc->tailL);

	FixNewMeasAccs(doc, newMeasL);

	xdMove = LinkXD(breakL)-std2d(config.spAfterBar, STFHEIGHT, STFLINES);
	UMoveInMeasure(breakL, doc->tailL, -xdMove);		/* Really stops moving at next Measure */
	xdMove = xdPrev-USysRelxd(newMeasL);
	firstMoveMeasL = LinkRMEAS(newMeasL);
	if (firstMoveMeasL && SameSystem(newMeasL, firstMoveMeasL))
		UMoveMeasures(firstMoveMeasL, sysTermL, -xdMove);
	
	FixMeasRectXs(prevMeasL, NILINK);

 	/*
 	 *	Correct the timestamp of the new Measure, and adjust times of everything within
 	 *	it to be relative to it, so the first Sync in it will be at relative time 0.
 	 */
	syncL = LSSearch(newMeasL, SYNCtype, ANYONE, GO_RIGHT, False);
	if (syncL)
		prevMeasDur = SyncTIME(syncL);
	else
		prevMeasDur = GetMeasDur(doc, newMeasL, ANYONE);
	MeasureTIME(newMeasL) = MeasureTIME(prevMeasL)+prevMeasDur;
	MoveTimeInMeasure(breakL, doc->tailL, -prevMeasDur);

	/* Handle slurs and beams becoming cross-system. */
	
	startL = LSSearch(prevMeasL, SYSTEMtype, ANYONE, GO_LEFT, False);
	sysTermL = EndSystemSearch(doc, newMeasL);
	FixCrossSysObjects(doc, startL, sysTermL);
	
	return newSysL;
}


/* --------------------------------------------------------------- RespAndRfmtRaw -- */
/* RespAndRfmtRaw respaces, i.e., it performs global punctuation in the range
[startL,endL) by fixing up object x-coordinates so that each is allowed the "correct"
space. startL and endL must be within a single Measure! RespAndRfmtRaw also moves any
following Measures in the System right or left to match. If, as a result of all this,
there's not enough room in the System, it will create new Systems and possibly Pages
to put the material into; since Nightingale always starts a new Measure on every System,
this may well have the effect of breaking the Measure containing the range up into two
or more Measures. The range should be "raw" in that it cannot contain anything but
Syncs. Furthermore, no "extended objects"--slurs, beams, ottavas, tuplets, hairpins--
can start before the range and end after it.

Intended for use on notes that were either played-in or read from a MIDI file or
similar source, of either unknown or known duration. Assumes it's operating on the
active document, so it updates objRects and redraws. Returns True if it creates any
new Systems or Pages, False if not. */

#define EXTRA_MARGIN	0			/* End of each System relative to last usable position (DDIST) */

Boolean RespAndRfmtRaw(
					register Document *doc,
					LINK startL, LINK endL,		/* Range to respace */
					long spaceProp 				/* Use spaceProp/(RESFACTOR*100) of normal spacing */
					)
{
	LINK				barTermL,
						barL,								/* Measure containing range of "raw" stuff */
						lastxdObjL,						/* Last obj with valid xd in Measure */
						sysTermL, newSysL;
	register LINK	breakL, pL;
	DDIST				oldMWidth, newMWidth,
						widthChange;
	LONGDDIST		staffLengthUse;
	Boolean			newSysOrPage=False;
	short				status;

	barTermL = EndMeasSearch(doc, startL);
	barL = LSSearch(LeftLINK(barTermL), MEASUREtype, ANYONE, GO_LEFT, False);
		
	/* Respace the range and get the change in its Measure's width */
	
	oldMWidth = MeasWidth(startL);	
	widthChange = RespaceRaw(doc, startL, endL, spaceProp);
	
	UMoveInMeasure(endL, barTermL, widthChange);
	newMWidth = oldMWidth+widthChange;
	SetMeasWidth(barL, newMWidth);
	UMoveRestOfSystem(doc, barL, newMWidth);
	
	staffLengthUse = StaffLength(barL)-EXTRA_MARGIN;

	sysTermL = EndSystemSearch(doc, startL);
	lastxdObjL = FirstValidxd(LeftLINK(sysTermL), GO_LEFT);

	/*
	 *	Respacing the range may result in its System overflowing. If so, we want to break
	 * the System into as many pieces as necessary, filling each as fully as possible.
	 * Look for the first object with valid xd that's past the end of the System.
	 */
	breakL = startL;
	do {
		pL = RightLINK(breakL);
		if (!pL) break;
		breakL = NILINK;
		for ( ; pL!=RightLINK(lastxdObjL); pL = RightLINK(pL))
			if (HasValidxd(pL))
				if (USysRelxd(pL)>staffLengthUse) {
					if (IsAfter(barTermL, pL))
						breakL = LSSearch(pL, MEASUREtype, ANYONE, GO_LEFT, False);
					else
						breakL = pL;
					break;
				}
		if (!breakL) break;						/* If nothing past end of System, we're done */
		
		if (MeasureTYPE(breakL)) breakL = RightLINK(breakL);
		
		/*
		 * Found a likely breaking point. Break the System at <breakL> unless (1) it's
		 * the object ending the range's System, or (2) it's a Measure and the next
		 * object ends the range's System.
		 */
		if (!(breakL==sysTermL)
		&&  !(MeasureTYPE(breakL) && RightLINK(breakL)==sysTermL)) {
			newSysL = BreakSystem(doc, breakL);
		 	if (newSysL)
				newSysOrPage = True;
			else
				goto Cleanup;
		}
	} while (breakL);

Cleanup:
	/* ??Should probably not Reformat if we jump to Cleanup--move this call above? */
	status = Reformat(doc, startL, endL, False, 9999, False, 999, config.titleMargin);
	if (status==FAILURE)
		newSysOrPage = False;

	InvalRange(barL, sysTermL);
	InvalWindow(doc);
	return newSysOrPage;
}
