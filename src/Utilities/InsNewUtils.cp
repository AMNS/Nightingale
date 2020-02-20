/******************************************************************************************
	FILE:	InsNewUtils.c
	PROJ:	Nightingale
	DESC:	Utility routines for low-level symbol insertion routines that
			actually modify the data structures - MemMacroized version.
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
	UpdateBFClefStaff		UpdateBFKSStaff			UpdateBFTSStaff
	ReplaceClef				ReplaceKeySig			ReplaceTimeSig
	EnlargeResAreas			FixInitialxds			FixInitialKSxds
	CreateMeasure
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ----------------------------------------------------------- UpdateBFClef/KS/TSStaff -- */
/* Keep the context fields in the staff object consistent with the context established
by the BeforeFirstMeas objects. */

void UpdateBFClefStaff(LINK firstClefL, short staffn, short subtype)
{
	LINK staffL, aStaffL;

	staffL = LSSearch(firstClefL, STAFFtype, ANYONE, GO_LEFT, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==staffn)
			{ StaffCLEFTYPE(aStaffL) = subtype; break; }
}

void UpdateBFKSStaff(LINK firstKSL, short staffn, KSINFO newKSInfo)
{
	LINK staffL, aStaffL;

	staffL = LSSearch(firstKSL, STAFFtype, ANYONE, GO_LEFT, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==staffn)
			/* Copy new keysig. info: use a macro so memory can't get moved on us! */
			KEYSIG_COPY(&newKSInfo, (PKSINFO)StaffKSITEM(aStaffL));
}

void UpdateBFTSStaff(LINK firstTSL, short staffn, short /*subType*/, short /*numerator*/,
							short /*denominator*/)
{
	LINK staffL, aStaffL;

	staffL = LSSearch(firstTSL, STAFFtype, ANYONE, GO_LEFT, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==staffn) {
			StaffTIMESIGTYPE(aStaffL) = TimeSigType(firstTSL);
			StaffNUMER(aStaffL) = TimeSigNUMER(firstTSL);
			StaffDENOM(aStaffL) = TimeSigDENOM(firstTSL);
		}
}


/* ----------------------------------------------------------------------- ReplaceClef -- */
/* Replace clef before first (with invisible barline) Measure. Return the first
LINK after the range affected, i.e., the next clef change on the staff, if any. */

LINK ReplaceClef(Document *doc, LINK firstClefL, short staffn, char subtype)
{
	LINK	aClefL, doneL;
	char	oldClefType;
	
	LinkSEL(firstClefL) = True;
	ClefINMEAS(firstClefL) = False;

	aClefL = FirstSubLINK(firstClefL);
	for ( ; aClefL; aClefL = NextCLEFL(aClefL))
		if (ClefSTAFF(aClefL)==staffn) {
			oldClefType = ClefType(aClefL);
			InitClef(aClefL, staffn, 0, subtype);

			ClefSEL(aClefL) = True;
			ClefVIS(aClefL) = True;
			
			UpdateBFClefStaff(firstClefL, staffn, subtype);

			doc->changed = True;
			break;
		}

	doc->selStartL = firstClefL;
	doc->selEndL = RightLINK(doc->selStartL);
	doneL = FixContextForClef(doc, RightLINK(firstClefL), staffn, oldClefType, subtype);
	return doneL;
}


/* -------------------------------------------------------------------- ReplaceKeySig --- */
/* Replace key signature before first (with invisible barline) Measure. Return the
end of the range affected. */

LINK ReplaceKeySig(Document *doc, LINK firstKeySigL,
							short staffn,							/* Desired staff no., or ANYONE */
							short sharpsOrFlats
							)
{
	KSINFO	oldKSInfo,newKSInfo;
	LINK	pL,endL,aKeySigL;
	
	LinkSEL(firstKeySigL) = True;
	KeySigINMEAS(firstKeySigL) = False;

	endL = firstKeySigL;
	aKeySigL = FirstSubLINK(firstKeySigL);
	for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) 
		if (KeySigSTAFF(aKeySigL)==staffn || staffn==ANYONE) {
			if (sharpsOrFlats)
				KeySigSEL(aKeySigL) = KeySigVIS(aKeySigL) = True;

			KEYSIG_COPY((PKSINFO)(KeySigKSITEM(aKeySigL)), &oldKSInfo);			/* Copy old keysig. info */
			InitKeySig(aKeySigL, KeySigSTAFF(aKeySigL), 0, sharpsOrFlats);
			SetupKeySig(aKeySigL, sharpsOrFlats);
			
			KEYSIG_COPY((PKSINFO)(KeySigKSITEM(aKeySigL)), &newKSInfo);			/* Copy new keysig. info */
			
			UpdateBFKSStaff(firstKeySigL,staffn,newKSInfo);

			FixContextForKeySig(doc, RightLINK(firstKeySigL), 
										KeySigSTAFF(aKeySigL), oldKSInfo, newKSInfo);
			pL = FixAccsForKeySig(doc, firstKeySigL, KeySigSTAFF(aKeySigL),
										oldKSInfo, newKSInfo);
			if (pL)
				if (IsAfter(endL, pL)) endL = pL;
		}

	doc->changed = True;
	if (sharpsOrFlats) {
		doc->selStartL = firstKeySigL;
		doc->selEndL = RightLINK(doc->selStartL);
	}

	return endL;
}


/* -------------------------------------------------------------------- ReplaceTimeSig -- */
/* Replace time signature before first (with invisible barline) Measure */

void ReplaceTimeSig(Document *doc,
							LINK firstTimeSigL,
							short staffn,			/* Desired staff no., or ANYONE */
							short type,
							short numerator,
							short denominator
							)
{
	PTIMESIG	pTimeSig;
	LINK		aTimeSigL;
	TSINFO		timeSigInfo;

	pTimeSig = GetPTIMESIG(firstTimeSigL);
	pTimeSig->selected = True;
	pTimeSig->inMeasure = False;

	aTimeSigL = FirstSubLINK(firstTimeSigL);
	for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
		if (TimeSigSTAFFN(aTimeSigL)==staffn || staffn==ANYONE) {
			InitTimeSig(aTimeSigL, TimeSigSTAFFN(aTimeSigL), 0, type, numerator, denominator);
			TimeSigSEL(aTimeSigL) = True;
			TimeSigVIS(aTimeSigL) = True;

			UpdateBFTSStaff(firstTimeSigL,staffn,type,numerator,denominator);

			timeSigInfo.TSType = TimeSigType(aTimeSigL);
			timeSigInfo.numerator = TimeSigNUMER(aTimeSigL);
			timeSigInfo.denominator = TimeSigDENOM(aTimeSigL);
			FixContextForTimeSig(doc,RightLINK(firstTimeSigL),TimeSigSTAFFN(aTimeSigL),timeSigInfo);
			doc->changed = True;
		}
	}
	FixTimeStamps(doc, firstTimeSigL, NILINK);			/* In case of whole-meas. rests */
	doc->selStartL = firstTimeSigL;
	doc->selEndL = RightLINK(firstTimeSigL);
}



/* ------------------------------------------ EnlargeResAreas and FixInitial functions -- */
/* Enlarge all System reserved areas from <startL> through the System containing <endL>,
inclusive, by <shift>. */

static Boolean EnlargeResAreas(Document *, LINK, LINK, DDIST);
static Boolean EnlargeResAreas(Document *doc, LINK startL, LINK endL, DDIST shift)
{
	LINK	pL, endSysL;
	
	/* Get the measure which terminates the system containing endL. */
	
	endSysL = EndSystemSearch(doc, endL);

	/* Move objects following startL in its "Measure" (normally a System's reserved 
	   area), and all following Measures till endSysL regardless of intervening
	   Systems or Pages, to right by <shift>. */
		
	pL = MoveInMeasure(startL, endSysL, shift);
	MoveAllMeasures(pL, endSysL, shift);
			
	InvalContent(startL, endSysL);						/* Force updating all objRects */
	return True;
}
	
/* #1. These numbers must be the same as those computed by DelClefBefFirst and
DelTimeSigBefFirst. */

void FixInitialxds(Document *doc, LINK firstObjL, LINK endL, short type)
{
	DDIST change=0;

	switch (type) {
		case CLEFtype:
			change = GetClefSpace(firstObjL);							/* #1 */
			break;
		case TIMESIGtype:
			change = GetTSWidth(firstObjL);								/* #1 */
			break;
		default:
			MayErrMsg("FixInitialxds: can't handle object type %ld", (long)type);
			break;
	}
	if (change>0) EnlargeResAreas(doc, RightLINK(firstObjL), endL, change);
}


/* Expand or compress the space given to the initial keySig in one or more Systems.
If the change of keySig is on all staves, simply move by the distance needed minus the
space already available; else, move by the maximum distance needed by all keySigs on
any individual staff minus the space available. */

void FixInitialKSxds(
					Document *doc,
					LINK firstKeySigL,		/* the new initial keySig */
					LINK endL,				/* end of the range affected */
					short staffn			/* staff no. or ANYONE */
					)
{
	DDIST 	needWidth, haveWidth, change;
	LINK	rightL;
	
	needWidth = GetKeySigWidth(doc,firstKeySigL,staffn);

	/* We cannot use xd of any Graphic or other J_D symbol inserted between keySig
	   and timeSig. */

	rightL = FirstValidxd(RightLINK(firstKeySigL), GO_RIGHT);
	haveWidth = SysRelxd(rightL)-SysRelxd(firstKeySigL);
	change = needWidth-haveWidth;
	if (change) EnlargeResAreas(doc, RightLINK(firstKeySigL), endL, change);
}


/* --------------------------------------------------------------------- CreateMeasure -- */
/* Add a Measure for all staves to the object list before the given link. Assumes
there's already at least one Measure before the given link on its System. Makes no
assumptions about user interface, e.g., whether or not it's being called in response
to user explicitly inserting a barline. Returns link to the new Measure if it succeeds,
NILINK if anything goes wrong.

NB: If the position of the new Measure is not yet known, using an arbitrary value for
<xd> here could (in extreme cases) cause overflow of coords. of objects within the
Measure, since they're relative to the Measure's position. It's safer to pass a
negative value for <xd> (see below). */

LINK CreateMeasure(register Document *doc,
						LINK insertL,
						DDIST xd,				/* <0=use prev. measure's+(DDIST)1 */
						short sym,
						CONTEXT context
						)
{
	short			newRight;
	register PMEASURE pMeasure;
	PMEASURE		prevMeas, nextMeasure;
	LINK			tmpL, endMeasL,
					measureL, prevMeasL, nextMeasureL;
	LINK			aMeasureL, aprevMeasL, nextSync, prevSync;
	DDIST			prevMeasWidth;
	long			offsetDur;
	LINK			result=NILINK;

PushLock(OBJheap);
PushLock(MEASUREheap);
	prevMeasL = LSSearch(LeftLINK(insertL), MEASUREtype, ANYONE,
													GO_LEFT, False);
	if (!prevMeasL) goto Cleanup;										/* Should never happen */
	
	nextMeasureL = LSSearch(insertL, MEASUREtype, ANYONE, GO_RIGHT, False);

	measureL = InsertNode(doc, insertL, symtable[sym].objtype, doc->nstaves);
	if (!measureL) { NoMoreMemory(); goto Cleanup; }
	
	if (xd<0) xd = LinkXD(prevMeasL)+1;
	NewObjSetup(doc, measureL, ANYONE, xd);

	pMeasure = GetPMEASURE(measureL);
	pMeasure->lMeasure = prevMeasL;										/* Update cross links */
	prevMeas = GetPMEASURE(prevMeasL);
	prevMeas->rMeasure = measureL;
	pMeasure->rMeasure = nextMeasureL;
	if (nextMeasureL) {
		nextMeasure = GetPMEASURE(nextMeasureL);
		nextMeasure->lMeasure = measureL;
	}
	tmpL = LSSearch(LeftLINK(measureL), SYSTEMtype, ANYONE,
													GO_LEFT, False);	/* Must start search at left! */
	pMeasure = GetPMEASURE(measureL);
	pMeasure->systemL = tmpL;
	tmpL = LSSearch(LeftLINK(measureL), STAFFtype, ANYONE,
													GO_LEFT, False);	/* Must start search at left! */
	pMeasure = GetPMEASURE(measureL);
	pMeasure->staffL = tmpL;
	SetRect(&pMeasure->measureBBox, 0, 0, 0, 0);						/* Must init. but will be computed when drawn */ 
	pMeasure->spacePercent = prevMeas->spacePercent;
	pMeasure->fillerM = 0;

	aMeasureL = FirstSubLINK(measureL);
	aprevMeasL = FirstSubLINK(prevMeasL);
	for ( ; aMeasureL; 
			aprevMeasL = NextMEASUREL(aprevMeasL),
			aMeasureL = NextMEASUREL(aMeasureL)) {
		
	/*	Make the new Measure's right end relative to its position. */
		newRight = (MeasMRECT(aprevMeasL)).right+LinkXD(prevMeasL)-LinkXD(measureL);
		InitMeasure(aMeasureL, MeasureSTAFFN(aprevMeasL),
							0, (MeasMRECT(aprevMeasL)).top,
							newRight, (MeasMRECT(aprevMeasL)).bottom,
							True, MeasCONNABOVE(aprevMeasL), MeasCONNSTAFF(aprevMeasL),
							MeasMEASURENUM(aprevMeasL)+1);

		MeasSUBTYPE(aMeasureL) = symtable[sym].subtype;
		MeasureSEL(aMeasureL) = True;									/* Select the subobj */
		MeasSOFT(aMeasureL) = False;
		
		/* Make xd relative to the previous Measure. */
	
		prevMeasWidth = xd-LinkXD(prevMeasL);
		(MeasMRECT(aprevMeasL)).right = prevMeasWidth;
		
		/* Fill in clef, key sig., meter, and dynamic context fields of new Measure. */
	
		GetContext(doc, LeftLINK(measureL), MeasureSTAFFN(aMeasureL), &context);
		FixMeasureContext(aMeasureL, &context);
	}
	
	LinkVALID(prevMeasL) = LinkVALID(measureL) = False;					/* recalc measureBBox */
	if (nextMeasureL)													/* these have new objRects */
		InvalRange(measureL, nextMeasureL);
	else
		InvalRange(measureL, doc->tailL);
	InvalMeasure(prevMeasL, ANYONE);

	endMeasL = EndMeasSearch(doc, measureL);

	/* Notes that formerly inherited accidentals from preceding notes on their line/
	   space in what is now the previous measure must have explicit accidentals added. */
	   
	FixNewMeasAccs(doc, measureL);
	
	 /* Adjust coordinates of everything in the new Measure to be relative to it. */

	MoveInMeasure(RightLINK(measureL), endMeasL, -prevMeasWidth);
	
	/* Adjust timestamps of everything in the new Measure to be relative to it.
	   Subtracting the duration of the previous Measure from times in this Measure
	   may not work because adding the barline may actually change relative times;
	   instead, look for the first Sync in this Measure or following and make its rel.
	   time zero. Also set the timestamp of this and update timestamps of following
	   Measures. */

	nextSync = LSSearch(measureL, SYNCtype, ANYONE, GO_RIGHT, False);
	if (nextSync) {
		/* nextSync may not even be in our new Measure, but the following code will still
		   work because MoveTimeInMeasure won't touch anything outside of our Measure.
		   But if nextSync IS in our Measure and contains unknown durations, this may not
		   be a good idea because there may be a pause before it that should be preserved!
		   Comments, anyone? */
		
		offsetDur = SyncTIME(nextSync);
		MoveTimeInMeasure(RightLINK(measureL), endMeasL, -offsetDur);
	}

	if (RhythmUnderstood(doc, LinkLMEAS(measureL), True)) {
		pMeasure->lTimeStamp = prevMeas->lTimeStamp+GetMeasDur(doc, measureL, ANYONE);
		FixTimeStamps(doc, measureL, measureL);
	}
	else if (nextSync) {
		pMeasure->lTimeStamp = prevMeas->lTimeStamp+offsetDur; 
		FixTimeStamps(doc, measureL, measureL);
	}
	else {
		/* Rhythm in the preceding Measure isn't understood, and there are no following
		   Syncs. The only thing we need to do is set the timestamp of the new Measure;
		   the only obvious way to do that is from the end time of the last note of the
		   previous Measure, if it contained any notes, else from the previous Measure
		   itself. */
		   
		prevSync = LSSearch(measureL, SYNCtype, ANYONE, GO_LEFT, False);
		if (IsAfter(prevMeasL, prevSync))
			offsetDur = SyncTIME(prevSync)+SyncMaxDur(doc, prevSync);
		else
			offsetDur = 0L;
		pMeasure->lTimeStamp = prevMeas->lTimeStamp+offsetDur;
	}
	
	InsFixMeasNums(doc, measureL);

	result = measureL;
	
Cleanup:
PopLock(OBJheap);
PopLock(MEASUREheap);

	return result;
}
