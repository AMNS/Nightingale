/***************************************************************************
	FILE:	MoveUpDown.c
	PROJ:	Nightingale
	DESC:	Routines for moving Measures and Systems logically "up" and
			"down", i.e, from System to System or Page to Page.

	InitMoveBars			FixMeasOwners			FixStartContext
	CleanupMoveBars			MoveBarsUp				MoveBarsDown
	MoveSystemUp			MoveSystemDown
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

/* Prototypes for functions local to this file */

static Boolean InitMoveBars(Document *doc, LINK startL, LINK endL, LINK *startBarL,
							LINK *endBarL);
static void FixStartContext(Document *doc, LINK pL, LINK startL, LINK endL);
static void CleanupMoveBars(Document *doc, LINK startL, LINK endL);
static void MoveUpDownErr(LINK pL, short errNum);

/* ------------------------------------------------------------ Various Utilities -- */

/* Return range of bars to move in startBarL and endBarL. Returns FALSE if
startMeasL is not before endL. startBarL is first measure before startL, or
first measure of score if startL is before all measures. endBarL is object
terminating measure containing endL considered as selRange ptr.   */

static Boolean InitMoveBars(Document *doc, LINK startL, LINK endL, LINK *startBarL,
										LINK *endBarL)
{
	LINK startMeasL;

	startMeasL = LSISearch(LeftLINK(startL), MEASUREtype, ANYONE, TRUE, FALSE); /* Start at previous Measure */
	if (!startMeasL) {
		startMeasL = FirstSysMeas(LeftLINK(startL));
		if (!startMeasL) startMeasL = FirstDocMeas(doc);
		if (endL==startMeasL || IsAfter(endL, startMeasL)) return FALSE;
	}

	*startBarL = startMeasL;
	*endBarL = EndMeasSearch(doc, LeftLINK(endL));										/* Stop at end of Measure */
	return TRUE;
}


/* Fix up the context at the start of the page or system. */

static void FixStartContext(Document *doc, LINK pL, LINK startL, LINK endL)
{
	short s; KSINFO KSInfo; TSINFO TSInfo; CONTEXT context;

/* Fix start-of-system (or page) contexts. */
	for (s = 1; s<=doc->nstaves; s++) {
		GetContext(doc, pL, s, &context);

		EFixContForClef(doc, startL, endL, s, context.clefType, context.clefType, context);
		KeySigCopy((PKSINFO)context.KSItem, &KSInfo);
		EFixContForKeySig(startL, endL, s, KSInfo, KSInfo);

		TSInfo.TSType = context.timeSigType;
		TSInfo.numerator = context.numerator;
		TSInfo.denominator = context.denominator;					
		EFixContForTimeSig(startL, endL, s, TSInfo);

		EFixContForDynamic(startL, endL, s, context.dynamicType);
	}
}


/* Update screen and do miscellaneous cleanup. */

static void CleanupMoveBars(Document *doc, LINK startL, LINK endL)
{
	UpdateMeasNums(doc, LeftLINK(startL));

	/* Fix up entire measures containing moved range. In some calls to
		CleanupMoveBars we may expect endL to be NILINK; check this and
		find a measure for endL if at all possible. */

	if (!MeasureTYPE(startL))
		startL = LSSearch(startL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	if (endL && !MeasureTYPE(endL))
		endL = LSSearch(endL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	if (!endL)
		endL = LSSearch(doc->tailL, MEASUREtype, ANYONE, GO_LEFT, FALSE);

	if (startL) {
		FixMeasRectXs(startL, endL);
		InvalRange(startL, endL ? RightLINK(endL) : NILINK);
	}
	InvalWindow(doc);
	
	DeselRangeNoHilite(doc,doc->headL,doc->tailL);
	doc->selStartL = doc->selEndL = doc->tailL;

	MEAdjustCaret(doc,FALSE);
	doc->changed = TRUE;
}


static void MoveUpDownErr(LINK pL, short errNum)
{
	switch (errNum) {
		case 1:
			MayErrMsg("MoveBarsUp: no containing system for %ld", (long)pL);
			return;
		case 2:
			MayErrMsg("MoveBarsUp: no previous system for %ld", (long)pL);
			return;
		case 4:
			MayErrMsg("MoveBarsDown: no containing system for %ld", (long)pL);
			return;
		case 5:
			MayErrMsg("MoveBarsDown: no following system for %ld", (long)pL);
			return;
	}
}

/* ------------------------------------------------------------------ MoveJDObjs -- */
/*	Find all Graphics, Tempos and Endings that are attached to <srcAnchorL>, and
attach them to <dstAnchorL>. Return TRUE if we did anything, FALSE if there were
no J_D objects to move.

A question in case this function is ever generalized: Does this assume that these
params are measures, or merely that both must be valid anchor symbols for Graphic,
Tempo and Ending objects? */

Boolean MoveJDObjs(Document *doc, LINK srcAnchorL, LINK dstAnchorL);
Boolean MoveJDObjs(Document *doc, LINK srcAnchorL, LINK dstAnchorL)
{
	LINK		pL, stopL, prevL;
	Boolean	didSomething = FALSE;
	
	if (srcAnchorL==dstAnchorL) return FALSE;
	
	/* No need to search before previous measure for objects attached to srcAnchorL. */
	stopL = LSSearch(LeftLINK(srcAnchorL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
	if (stopL==NILINK) stopL = doc->headL;

	for (pL = LeftLINK(srcAnchorL); pL!=stopL; pL = prevL) {
		prevL = LeftLINK(pL);						/* get this now, in case we move pL below! */
		if (J_DTYPE(pL))
			switch (ObjLType(pL)) {
				case GRAPHICtype:
					if (GraphicFIRSTOBJ(pL)==srcAnchorL) {
						GraphicFIRSTOBJ(pL) = dstAnchorL;
						MoveNode(pL, dstAnchorL);
						didSomething = TRUE;
					}
					break;
				case TEMPOtype:
					if (TempoFIRSTOBJ(pL)==srcAnchorL) {
						TempoFIRSTOBJ(pL) = dstAnchorL;
						MoveNode(pL, dstAnchorL);
						didSomething = TRUE;
					}
					break;
				case ENDINGtype:
					if (EndingFIRSTOBJ(pL)==srcAnchorL) {
						EndingFIRSTOBJ(pL) = dstAnchorL;
						MoveNode(pL, dstAnchorL);
						didSomething = TRUE;
					}
			}
	}
	return didSomething;
}


/* ------------------------------------------------------------------ MoveBarsUp -- */
/* Move a range of Measures up from the beginning of a System to the end of the
preceding System. The range extends from the barLine prior to startL (which must
be the first Measure of its System) to the end of the measure including endL.
Areas before the initial barlines of systems, which are not part of any Measure,
are always left alone, even if they're within the specified area.
MoveBarsUp returns TRUE if it successfully moves anything, FALSE if not (either
because of an error or because the specified area is entirely before a system's
initial barline).

#1. A diagram of the Systems involved, before and after the operation.
[P] indicates [optional] page. node/ indicates that node is a barLine.

BEFORE: sys1L...leftBarL/
			[P] sys2L...startBarL/...startL...firstBarL/...endL...endBarL/...rightBarL/...
			sys3L
AFTER:  sys1L...leftBarL/ RightLINK(startBarL)...startL...firstBarL/...endL...endBarL/
			[P] sys2L...startBarL/...rightBarL/...
			sys3L

Enable "Move Measures Up" iff selStartL and selEndL are in the same System;
that System is not the first; selStartL is in the first Measure of that System;
and there's at least one Measure in the System after selStartL. 
*/

Boolean MoveBarsUp(Document *doc, LINK startL, LINK endL)
{
	LINK			startBarL, endBarL,						/* Objs delimiting Move area */
					sys1L, sys2L, sys3L,
					rightBarL, endRangeL,
					lastL, nextBarL;
	DDIST			xMove1, xMove2, linkXD, nextBarXD;	/* Dists to move ranges on upper and lower Systems */
	
	/* InitMoveBars returns range consisting of bars properly containing [startL,endL).
		If startL is before all measures, startBarL is the first barLine of score. */

	if (!InitMoveBars(doc, startL, endL, &startBarL, &endBarL))
		return FALSE;

	PrepareUndo(doc, startL, U_MoveMeasUp, 44);    				/* "Move Measures Up" */
	
	/* Don't move anything after the last barline of <endL>'s System.
		endBarL is returned by EndMeasSearch, and if it is not a barLine, it will be
		the structural obj terminating endL's measure, e.g. the following Page or
		System. If so, search left from endBarL to get the last barLine in endL's
		System. */

	if (!MeasureTYPE(endBarL))
		endBarL = LSSearch(endL, MEASUREtype, ANYONE, GO_LEFT, FALSE);

	sys2L = LSSearch(startL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	if (!sys2L)
		{ MoveUpDownErr(startL,1); return FALSE; }

	sys1L = LinkLSYS(sys2L);
	if (!sys1L)	
		{ MoveUpDownErr(startL,2); return FALSE; }

	sys3L = LinkRSYS(sys2L);
	endRangeL = sys3L ? sys3L : doc->tailL;

	lastL = LastOnPrevSys(sys2L);
	if (!MeasureTYPE(lastL)) {
		nextBarL = LSSearch(RightLINK(lastL),MEASUREtype,ANYONE,GO_RIGHT,FALSE);
		nextBarXD = SysRelxd(nextBarL);
	}

	rightBarL = LinkRMEAS(endBarL);
	
	/*
	 * Following code compares sysRelxd's for objects on different systems.
	 * Validity of comparison unclear.
	 * xMove1 is the amount by which to translate moved range on 2nd system
	 * so as to locate it at previous graphical starting point of destination
	 * slot on 1st system.
	 * xMove2 is the total horizontal width of moved range, amount by which the
	 * material following inserted range on 2nd system must be translated to
	 * left to fill in removed range.
	 */

	xMove1 = SysRelxd(lastL)-SysRelxd(startBarL);
	xMove2 = SysRelxd(startBarL)-SysRelxd(endBarL);

	/* cf. Diagram #1 */

	/* Move the range (startBarL,endBarL] to its new location, then fix crosslinks, 
		cross-system objects, and context. */
	
	MoveRange(RightLINK(startBarL), RightLINK(endBarL), RightLINK(lastL));

	{
		LINK dstAnchorL;
		
		if (MeasureTYPE(lastL)) dstAnchorL = lastL;
		else dstAnchorL = LSSearch(LeftLINK(lastL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
		
		MoveJDObjs(doc, startBarL, dstAnchorL);
		
		MoveJDObjs(doc, endBarL, startBarL);
	}

	FixStructureLinks(doc, doc, sys1L, endRangeL);
	FixCrossSysObjects(doc, startBarL, endBarL);
	FixStartContext(doc, endBarL, sys2L, RightLINK(startBarL));
	
	/* Translate all affected measures horizontally to new graphic locations.
		If lastL is a Measure, the previous system ended with a barline.
		Since nodes in the measure are relative to their measure, only the
		measures in the range moved need updating; MoveMeasures skips over
		all nodes in the range moved until its first measure, and moves the
		measures. The initial nodes in the range will already be correctly
		located relative to their measure, and need no updating.

		Otherwise, we need to position the nodes in the first measure of the
		range moved relative to their new measure. linkXD translates to xd of
		last obj in target meas plus space it occupies; this includes correction
		for previous xd of first node in first meas of moved range.

		Then, if nextBarL is in the target system, we are moving a multi-measure
		range, and must translate following measures. xd of nextBarL is sysRel,
		so need SysRelxd rather than LinkXD of lastL. If xMove1 was computed by:
			linkXD = SysRelxd(lastL); xMove1 = linkXD-nextBarXD;
		nextBarL would be located at exactly the same place as the last object
		in the previous measure [I do not understand why]. Therefore the
		ObjSpaceUsed by the FirstValidxd before nextBarL is added to the sysRelxd
		of lastL in computing linkXD.
			
		Finally, call MoveMeasures to take up the space occupied on the following
		system by the moved range. */

	if (MeasureTYPE(lastL))
		MoveMeasures(RightLINK(lastL), RightLINK(endBarL), xMove1);
	else {
		linkXD = LinkXD(lastL) + ObjSpaceUsed(doc,lastL)-LinkXD(RightLINK(lastL));
		nextBarL = LSSearch(RightLINK(lastL),MEASUREtype,ANYONE,GO_RIGHT,FALSE);
		MoveInMeasure(RightLINK(lastL),nextBarL,linkXD);

		if (SameSystem(lastL,nextBarL)) {
			linkXD = SysRelxd(lastL) +
				ObjSpaceUsed(doc,FirstValidxd(LeftLINK(nextBarL),GO_LEFT));
			xMove1 = linkXD-nextBarXD;
			MoveMeasures(nextBarL, RightLINK(endBarL), xMove1);
		}
	}

	MoveMeasures(RightLINK(startBarL), endRangeL, xMove2);

	/* Fix timestamps and clean up. */

	MeasureTIME(startBarL) = MeasureTIME(endBarL);

	CleanupMoveBars(doc, lastL, sys3L);
	return TRUE;
}

/* ---------------------------------------------------------------- MoveBarsDown -- */
/* Move a range of Measures down from the end of a System to the beginning of
the following System. The range extends from the barLine prior to startL to
the end of the measure including endL.
Areas before the initial barlines of systems, which are not part of any Measure,
are always left alone, even if they're within the specified area.
MoveBarsDown returns TRUE if it successfully moves anything, FALSE if not (either
because of an error or because the specified area is entirely before a system's
initial barline).

A diagram of the Systems involved.

BEFORE: sys1L...startBarL/...RightLINK(startBarL)...startL...endL...endBarL...
			[P] sys2L...rightBarL/...[rNextBarL]...
			[sys3L]
AFTER: sys1L...startBarL/
			[P] sys2L...rightBarL/...RightLINK(startBarL)...startL...endL...endBarL...[rNextBarL]...
			[sys3L]

MoveBarsDown is enabled iff selStartL and selEndL are in the same System;
and selEndL is in the last Measure of that System. 
*/

Boolean MoveBarsDown(Document *doc, LINK startL, LINK endL)
{
	LINK			startBarL, endBarL,				/* Objs delimiting Move area */
					sys1L, sys2L, sys3L,
					rightBarL, rNextBarL,
					endRangeL, prevBarL, prevL, sys1TermL;
	DDIST			xMoveA,xMoveB,sysTop;			/* Dists. to move stuff moved & to right of it */
	DRect			sysRect;

	/* InitMoveBars returns range consisting of bars properly containing [startL,endL).
		If startL is before all measures, startBarL is the first barLine of score. */

	if (!InitMoveBars(doc, startL, endL, &startBarL, &endBarL))
		return FALSE;

	PrepareUndo(doc, startL, U_MoveMeasDown, 45);    			/* "Move Measures Down" */
	
	sys1L = LSSearch(startL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	if (!sys1L)
		{ MoveUpDownErr(startL,4); return FALSE; }

	/* If sys2L is NILINK, there is no system following the measures to be moved
		down. Insert a system after on this page, if possible; if not, insert a
		page following this one, onto which to move the measures. */

	sys2L = LinkRSYS(sys1L);
	if (!sys2L) {
		prevL = LeftLINK(doc->tailL);
		if (RoomForSystem(doc, startL)) {
			sysRect = SystemRECT(sys1L);
			sysTop = sysRect.bottom;
			sys2L = CreateSystem(doc,prevL,sysTop,succSystem);
		}
		else {
			CreatePage(doc, prevL);
			sys2L = LinkRSYS(sys1L);
		}
		endBarL = RightLINK(prevL);
	}

	rightBarL = LSSearch(sys2L, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	rNextBarL = LinkRMEAS(rightBarL);

	sys3L = LinkRSYS(sys2L);
	endRangeL = sys3L ? sys3L : doc->tailL;
	
	/*
	 * Following code compares sysRelxd's for objects on different systems.
	 * Validity of comparison unclear.
	 * xMoveA is the amount by which to translate moved range on first system
	 * so as to locate it at previous graphical starting point of destination
	 * slot on 2nd system.
	 * xMoveB is the total horizontal width of moved range, amount by which the
	 * material following inserted range on 2nd system must be translated to
	 * right to accomodate newly inserted range.
	 *
	 * Tests for MeasureTYPE(endBarL): endBarL is returned to InitMoveBars
	 * from EndMeasSearch, and is the measure-terminating obj: either the following
	 * measure obj, or following structural obj.
	 * If it is a measure, it is the last obj in the range moved; it it is
	 * a structural obj, it terminates the range moved, and is not moved with the
	 * range. The 2 sets of cases depend on this distinction.
	 */

	xMoveA = SysRelxd(rightBarL)-SysRelxd(startBarL);

 	if (MeasureTYPE(endBarL)) {
		xMoveB = SysRelxd(endBarL)-SysRelxd(startBarL);
 	}
 	else{
 		prevBarL = LSSearch(endBarL,MEASUREtype,ANYONE,GO_LEFT,FALSE);
		xMoveB = MeasOccupiedWidth(doc, prevBarL, MeasSpaceProp(prevBarL));
 		if (prevBarL!=startBarL)
 			xMoveB += SysRelxd(prevBarL)-SysRelxd(startBarL);

		/* Save for use after MoveRange. sys1TermL is the last object on the 1st system,
			which might be the same as prevBarL. */
		sys1TermL = LeftLINK(endBarL);
 	}

	/* Relocate the range (startBarL,endBarL] to the slot immediately following
		the first measure of the system following its original system. cf. #1.
		Then call FixCrossSysObjects to fix up sync ptrs in Beamset-type J_D Objects;
		must call from sys1L since Beamsets, etc. on first system require updating. */

 	if (MeasureTYPE(endBarL)) {
		MoveRange(RightLINK(startBarL), RightLINK(endBarL), RightLINK(rightBarL));

		MoveJDObjs(doc, rightBarL, endBarL);
		MoveJDObjs(doc, startBarL, rightBarL);
	}
	else {
		LINK dstAnchorL;

		dstAnchorL = LSSearch(LeftLINK(endBarL), MEASUREtype, ANYONE, GO_LEFT, FALSE);

		MoveRange(RightLINK(startBarL), endBarL, RightLINK(rightBarL));

		if (dstAnchorL!=startBarL)
			MoveJDObjs(doc, rightBarL, dstAnchorL);
		MoveJDObjs(doc, startBarL, rightBarL);
	}

	FixStructureLinks(doc, doc, sys1L, endRangeL);
	FixCrossSysObjects(doc, sys1L, endRangeL);
	FixStartContext(doc, startBarL, sys2L, RightLINK(rightBarL));
	
	/* Translate affected measures horizontally to new graphic locations. Can call
		MoveMeasures up to RightLINK(endBarL) even if endBarL is not a terminating
		measure as MoveMeasures stops at J_STRUC objects. */

 	if (MeasureTYPE(endBarL))
		MoveMeasures(RightLINK(rightBarL), RightLINK(endBarL), xMoveA);
	else
		MoveMeasures(RightLINK(rightBarL), RightLINK(sys1TermL), xMoveA);

	if (rNextBarL && (!sys3L || IsAfter(rNextBarL, sys3L)))
		MoveMeasures(rNextBarL, endRangeL, xMoveB);

	/* Fix timestamps and clean up. */

	MeasureTIME(rightBarL) = MeasureTIME(startBarL);
	
	CleanupMoveBars(doc, startBarL, endRangeL);
	return TRUE;
}


/* ----------------------------------------------------------------- MoveSystemUp -- */
/* Move a System "up" from the top of one page to the bottom of the preceding one. The
System is the one containing <startL>. MoveSystemUp returns TRUE if it successfully
moves a System, FALSE if not (because of an error). */

Boolean MoveSystemUp(Document *doc, LINK startL)
{
	LINK			sys1L, sys2L, sys3L,
					srcPageL, destPageL, endSysL;
	PSYSTEM		pSys;
	DDIST			sysHeight, sysTop;
	
	PrepareUndo(doc, startL, U_MoveSysUp, 46);    						/* "Move System Up" */
	
	/* First, figure out what to move and collect relevant links. */

	sys2L = LSSearch(startL, SYSTEMtype, ANYONE, TRUE, FALSE);
	if (!sys2L) MayErrMsg("MoveSystemUp: no containing System for %ld", (long)startL);
	if (LastSysInPage(sys2L)) {
		GetIndCString(strBuf, MISCERRS_STRS, 15);    /* "Nightingale can't remove the only system on a page. Use Reformat to combine pages." */
		CParamText(strBuf,	"", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	endSysL = EndSystemSearch(doc, sys2L);
	sys1L = LinkLSYS(sys2L);
	if (!sys1L) MayErrMsg("MoveSystemUp: no preceding System for %ld", (long)startL);
	srcPageL = SysPAGE(sys2L);
	destPageL = SysPAGE(sys1L);
	sys3L = LinkRSYS(sys2L);													/* Must exist & be on same Page */

	/* Move the range [sys2L, endSysL) to its new location. */
	MoveRange(sys2L, endSysL, srcPageL);

	/* Fix the System's owning-Page link. */
	SysPAGE(sys2L) = SysPAGE(sys1L);
	
	FixStartContext(doc, srcPageL, srcPageL, sys3L);
	
	/* Move all affected Systems to new graphic locations. */
	pSys = GetPSYSTEM(sys2L);
	sysTop = pSys->systemRect.top;											/* Get first System top */
	pSys = GetPSYSTEM(sys3L);
	sysHeight = pSys->systemRect.bottom-pSys->systemRect.top;
	pSys->systemRect.top = sysTop;											/* Fix first System top & bottom */
	pSys->systemRect.bottom = pSys->systemRect.top+sysHeight;
	FixSystemRectYs(doc, FALSE);												/* Fix all Systems' top & bottom */

	/* Insure selection range is legal, update screen, and do miscellaneous cleanup. */
	InvalRange(destPageL, LinkRPAGE(srcPageL));
	InvalWindow(doc);
	doc->changed = TRUE;
	return TRUE;
}


/* -------------------------------------------------------------- MoveSystemDown -- */
/* Move a System "down" from the bottom of one page to the top of the next one. The
System is the one containing <startL>. MoveSystemDown returns TRUE if it successfully
moves a System, FALSE if not (because of an error). */

Boolean MoveSystemDown(Document *doc, LINK startL)
{
	LINK			sys1L, sys2L,
					destPageL, nextPageL;
	PSYSTEM		pSys;
	DDIST			sysHeight, sysTop;
	
	PrepareUndo(doc, startL, U_MoveSysDown, 47);    					/* "Move System Down" */
	
	/* First, figure out what to move and collect relevant links. */

	sys1L = LSSearch(startL, SYSTEMtype, ANYONE, TRUE, FALSE);
	if (!sys1L) MayErrMsg("MoveSystemDown: no containing System for %ld", (long)startL);
	if (FirstSysInPage(sys1L)) {
		GetIndCString(strBuf, MISCERRS_STRS, 15);    /* "Nightingale can't remove the only system on a page. Use Reformat to combine pages." */
		CParamText(strBuf,	"", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	destPageL = EndSystemSearch(doc, sys1L);
	if (!PageTYPE(destPageL))
		MayErrMsg("MoveSystemDown: obj at %ld ending System for %ld isn't a Page",
					(long)startL, (long)destPageL);
	nextPageL = LinkRPAGE(destPageL);
	sys2L = LinkRSYS(sys1L);
	if (!sys2L) MayErrMsg("MoveSystemDown: no following System for %ld", (long)startL);

	/* Move the range [sys1L, destPageL) to its new location. */
	MoveRange(sys1L, destPageL, sys2L);

	/* Fix the System's owning-Page link. */
	SysPAGE(sys1L) = SysPAGE(sys2L);
	
	FixStartContext(doc, sys1L, destPageL, sys1L);
	
	/* Move all affected Systems to new graphic locations. */
	pSys = GetPSYSTEM(sys2L);
	sysTop = pSys->systemRect.top;											/* Get first System top */
	pSys = GetPSYSTEM(sys1L);
	sysHeight = pSys->systemRect.bottom-pSys->systemRect.top;
	pSys->systemRect.top = sysTop;											/* Fix first System top & bottom */
	pSys->systemRect.bottom = pSys->systemRect.top+sysHeight;
	FixSystemRectYs(doc, FALSE);												/* Fix other Systems top & bottom */

	/* Insure selection range is legal, update screen, and do miscellaneous cleanup. */
	if (doc->selEndL==destPageL) doc->selEndL = sys2L;
	InvalRange(destPageL, nextPageL);
	InvalWindow(doc);
	MEAdjustCaret(doc, FALSE);
	doc->changed = TRUE;
	return TRUE;
}
