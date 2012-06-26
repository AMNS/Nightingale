/***************************************************************************
	FILE:	Reformat.c
	PROJ:	Nightingale, rev. for v.3.5
	DESC:	Routines for standard reformatting (does no respacing, works on any
	any range).

	BuildMeasTable				NewSysNums
	FixMeasVis					AddFinalMeasure
	RfmtSystems					BuildSysTable				NewSheetNums
	GetTitleMargin				RfmtPages					Reformat
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-98 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#if 0
#define RFMTBUG						/* Compile debugging code */
#endif

#define MAX_MEAS_RFMT 1000			/* For debugging, max. Measures reformatted we can print info on */
#define RIGHTEND_SLOP pt2d(1)		/* Last usable position relative to end of System */

typedef struct {
	LINK		measL;
	LINK		systemL;
	short		firstOfSys:1;			/* TRUE=first Measure of System */
	short		lastOfSys:1;			/* TRUE=last Measure of System */
	short		secondPiece:1;			/* TRUE=2nd piece of split Measure: combine if possible */
	short		newSysNum:13;
	DDIST		width;					/* Normal width of Measure */
	DDIST		lastWidth;				/* Width of Measure if it's last in its System */
	long		lTimeStamp;
	SignedByte	barType;
	SignedByte	filler;
} MEASDATA;

typedef struct {
	LINK		sysL;
	LINK		pageL;
	short		newSheetNum;
	DDIST		height;
} SYSDATA;

static short CountMeasures(Document *doc, LINK startSysL, LINK endSysL);
static short CountRealMeasures(Document *doc, LINK startSysL, LINK endSysL);
static short CountSystems(Document *doc, LINK startSysL);
static MEASDATA *AllocMeasTable(Document *doc, LINK startSysL, LINK endSysL);
static SYSDATA *AllocSysTable(Document *doc, LINK startSysL);

static DDIST GetMWidth(Document *doc, LINK measL);
static short BuildMeasTable(Document *, LINK, LINK, MEASDATA []);
static short BuildSysTable(Document *, LINK, LINK, SYSDATA []);
static short LinkToSIndex(LINK sysL, SYSDATA *sysTable);

static void DebugRfmtTable(MEASDATA measTable[], short mCount, DDIST sysWChecked[], 
							DDIST staffLengthUse);
static short NewSysNums(Document *, short, Boolean, MEASDATA [], short, LINK);
static void FixMeasVis(Document *doc, MEASDATA [], short);
static Boolean AddFinalMeasure(Document *, LINK, LINK, DDIST);
static short GetSysWhere(LINK startSysL, LINK newSysL);
static short RfmtSystems(Document *, LINK, LINK, short, Boolean, LINK *);
static short NPrevSysInPage(LINK);
static short GetRfmtRange(Document *doc, SYSDATA sysTable[], short sCount);
static short CheckSysHeight(Document *doc, SYSDATA sysTable[], short sCount, short titleMargin);
static void DebugSysTable(SYSDATA sysTable[],short s,
							DDIST pgHtUsed,DDIST pgHtAvail,short pgSysNum);
static Boolean NewSheetNums(Document *, LINK, short, SYSDATA [], short, short);
static LINK RfmtResetSlurs(Document *doc, LINK startMoveL);
static Boolean SFirstMeasInSys(LINK measL);
static void MoveSysJDObjs(Document *doc, LINK sysL, LINK measL, LINK endL, LINK newMeasL);
static Boolean PrepareMoveMeasJDObjs(Document *, LINK, LINK);
static Boolean MoveMeasJDObjs(Document *, LINK, LINK);
static short GetTitleMargin(Document *doc);
static short RfmtPages(Document *, LINK, LINK, short, short);

static short nSysInScore,startSIndex;

/* ------------------------------------------- CountMeasures/RealMeasures/Systems -- */

/* Return the number of Measure objects in the given range of LINKs. NB: does not
distinguish real and "fake" Measures. */

static short CountMeasures(Document */*doc*/, LINK startL, LINK endL)
{
	return CountObjects(startL, endL, MEASUREtype);
}

/* Return the number of non-fake Measure objects in the given range of LINKs. */

static short CountRealMeasures(Document */*doc*/, LINK startL, LINK endL)
{
	short count; LINK pL;
	
	for (count = 0, pL = startL; pL && pL!=endL; pL = RightLINK(pL))
		if (ObjLType(pL)==MEASUREtype && !MeasISFAKE(pL))
			count++;
	
	return count;
}

/* Return the number of System objects from the given LINK to the end. */

static short CountSystems(Document *doc, LINK startL)
{
	return CountObjects(startL, doc->tailL, SYSTEMtype);
}

/* ------------------------------------------------------ AllocMeasTable/SysTable -- */

#define EXTRA 4

static MEASDATA *AllocMeasTable(Document *doc, LINK startSysL, LINK endSysL)
{
	short nMeasures; MEASDATA *measTable;

	nMeasures = CountMeasures(doc,startSysL,endSysL);

	measTable = (MEASDATA *)NewPtr((nMeasures+EXTRA)*sizeof(MEASDATA));
	if (!GoodNewPtr((Ptr)measTable)) {
		OutOfMemory((nMeasures+EXTRA)*sizeof(MEASDATA));
		return NULL;
	}

	return measTable;
}

static SYSDATA *AllocSysTable(Document *doc, LINK startSysL)
{
	SYSDATA *sysTable;
	short nSystems;

	nSystems = CountSystems(doc,startSysL);

	sysTable = (SYSDATA *)NewPtr((nSystems+EXTRA)*sizeof(SYSDATA));
	if (!GoodNewPtr((Ptr)sysTable)) {
		OutOfMemory((nSystems+EXTRA)*sizeof(SYSDATA));
		return NULL;
	}

	return sysTable;
}

/* ------------------------------------------------------------------- GetMWidth -- */

static DDIST GetMWidth(Document *doc, LINK measL)
{
	LINK lastL;

	lastL = EndMeasSearch(doc, measL);

	if (LastMeasInSys(measL))
		return MeasOccupiedWidth(doc, measL, MeasSpaceProp(measL));
	
	return SysRelxd(lastL)-SysRelxd(measL);
}

/* -------------------------------------------------------------- BuildMeasTable -- */
/* Build a table of widths and current owning Systems of the Measures to be
reformatted, with a blank slot for the new System number of each Measure. Delivers
the number of Measures in the table. */

static short BuildMeasTable(Document *doc, LINK startSysL, LINK endSysL,
									MEASDATA measTable[])
{
	short mIndex;
	LINK measL, startMeasL, endMeasL;
	DDIST mWidth, mLastWidth;
	CONTEXT context;
	
	startMeasL = LSSearch(startSysL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	endMeasL = LSSearch(endSysL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);

	mIndex = 0;
	for (measL=startMeasL; measL!=endMeasL; mIndex++,measL=LinkRMEAS(measL)) {
		
		mWidth = GetMWidth(doc,measL);
		GetContext(doc, measL, 1, &context);
		mLastWidth = MeasJustWidth(doc, measL, context);

		measTable[mIndex].firstOfSys = FirstMeasInSys(measL);
		measTable[mIndex].lastOfSys = LastMeasInSys(measL);
		measTable[mIndex].secondPiece = (FirstMeasInSys(measL) && mIndex>0);
		measTable[mIndex].measL = measL;
		measTable[mIndex].systemL = MeasSYSL(measL);
		measTable[mIndex].width = mWidth;
		measTable[mIndex].lastWidth = mLastWidth;
		measTable[mIndex].lTimeStamp = MeasureTIME(measL);
		measTable[mIndex].barType = MeasSUBTYPE(FirstSubLINK(measL));
	}

	return mIndex;
}


/* --------------------------------------------------------------- BuildSysTable -- */
/* Build a table of heights and current owning Pages of the Systems to be
reformatted, with a blank slot for the new sheet number of each System. Builds
a table for all systems in the score starting with startSysL. Returns the number
of Systems in the range to be reformatted, which is the number of systems in
the range from startSysL up to and not including endL. */

static short BuildSysTable(Document */*doc*/, LINK startSysL, LINK endL, SYSDATA sysTable[])
{
	short sIndex=0;
	LINK sysL;
	DDIST sHeight;
	DRect sysRect;
	
	sysL = startSysL;	
	for ( ; sysL; sIndex++,sysL = LinkRSYS(sysL)) {
		sysRect = SystemRECT(sysL);
		sHeight = sysRect.bottom-sysRect.top;

		sysTable[sIndex].sysL = sysL;
		sysTable[sIndex].pageL = SysPAGE(sysL);
		sysTable[sIndex].height = sHeight;
		sysTable[sIndex].newSheetNum = 0;
	}
	
	return CountObjects(startSysL, endL, SYSTEMtype);
}

/* -------------------------------------------------------------- DebugRfmtTable -- */

#ifdef RFMTBUG
static void DebugRfmtTable(MEASDATA measTable[], short mCount, DDIST sysWChecked[],
									DDIST staffLengthUse)
{
	short m;

	for (m = 0; m<mCount && m<MAX_MEAS_RFMT; m++) {
		DebugPrintf("m[%d] measL=%d sysL=%d wid=%d (widChkd=%d lenUse=%d) TS=%ld",
						m, measTable[m].measL, measTable[m].systemL,
						measTable[m].width, sysWChecked[m], staffLengthUse,
						measTable[m].lTimeStamp);
		if (measTable[m].lastOfSys) DebugPrintf(" LAST");
		if (measTable[m].secondPiece) DebugPrintf(" 2ND");
		if (m==0 || (m>0 && measTable[m].newSysNum!=measTable[m-1].newSysNum))
			DebugPrintf(" newSysN=%d", measTable[m].newSysNum);
		DebugPrintf("\n");
	}
}
#else
static void DebugRfmtTable(MEASDATA [], short, DDIST [], DDIST)
{}
#endif

/* ------------------------------------------------------------------ NewSysNums -- */
/*	Fill in the new System number each Measure will have. Start a new System when
the desired number of Measures per System is exceeded or, optionally, when the
current System overflows. Exception: if the very last Measure being reformatted
contains nothing at all, or if it exceeds the Measures per System limit and is fake,
don't start a new System for it. Also, we never really ignore overflow: we always
start a new System when the System length reaches the max. that fits in a DDIST,
(DDIST)SHRT_MAX = about 28 in.

NewSysNums returns FAILURE if there's a problem, else NOTHING_TO_DO or OP_COMPLETE.

Since the System where we start may be the first System of the score and therefore
have a different length (because of its indent), we use one "usable staff length"
for the System we start with and another for all other Systems. */

static short NewSysNums(
					Document *doc,
					short measPerSys,
					Boolean ignoreOver,			/* Ignore overflow (up to max.DDIST length) */
					MEASDATA measTable[],
					short mCount,
					LINK startSysL
					)
{
	short m,prevSysNum,measThisSys,startSysNum,oldSysNum;
	LONGDDIST currWidth;
	DDIST sysWidthUsed,
			sysWChecked[MAX_MEAS_RFMT],			/* Just so we can DebugPrintf later */
			gutter1, gutter2, staffLengthUse,
			staffLenUse1,staffLenUse2;
	Boolean tooManyMeas, lastAndEmpty, doSomething=FALSE;
	LINK nextSysL, nextSysMeasL, endMeasL, sysL;
	
	startSysNum = SystemNUM(startSysL);
	
	if (ignoreOver)
		staffLenUse1 = staffLenUse2 = (DDIST)SHRT_MAX;
	else {
		staffLenUse1 = staffLenUse2 =
			MARGWIDTH(doc)-doc->otherIndent+RIGHTEND_SLOP;
		if (startSysNum==1)
			staffLenUse1 = MARGWIDTH(doc)-doc->firstIndent+RIGHTEND_SLOP;
	}
	
	/*
	 *	Set the gutter width for the first System we'll create to the gutter width of
	 * the first System we're starting with. For all following Systems we'll create,
	 * set it to the gutter width of the System after the one we're starting with,
	 * if there is another System; if not, just use the starting System's gutter again.
	 * If we're starting with the first System of the score, this will probably be too
	 *	great because it'll include the time signature, but oh well. Also, this doesn't
	 *	consider changes to gutter width resulting from key signature changes.
	 */
	gutter2 = gutter1 = SysRelxd(measTable[0].measL);
	nextSysL = LinkRSYS(MeasSYSL(measTable[0].measL));
	if (nextSysL) {
		nextSysMeasL = SSearch(nextSysL, MEASUREtype, GO_RIGHT); 
		gutter2 = SysRelxd(nextSysMeasL);
	}

	prevSysNum = startSysNum;
	measThisSys = 0;
	sysWidthUsed = gutter1;
	staffLengthUse = staffLenUse1;
	lastAndEmpty = FALSE;

	for (m = 0; m<mCount; m++) {
		if (!MeasISFAKE(measTable[m].measL))
			measThisSys++;
		tooManyMeas = (measThisSys>measPerSys);
		
		/* Try to avoid starting a new System just for an empty last Measure, i.e.,
			a final barline with nothing after it. */
		if (m==mCount-1) {
			endMeasL = EndMeasSearch(doc, measTable[m].measL);
			lastAndEmpty = (endMeasL==RightLINK(measTable[m].measL));
		}

		if (m<MAX_MEAS_RFMT)
			sysWChecked[m] = sysWidthUsed+measTable[m].lastWidth;

		currWidth = (LONGDDIST)sysWidthUsed+(LONGDDIST)measTable[m].lastWidth;
		if (!lastAndEmpty)
			if (currWidth>staffLengthUse || tooManyMeas) {
				if (ignoreOver && !tooManyMeas) {
					StopInform(RFMT_EXACT_ALRT);
					return FAILURE;
				}
				prevSysNum++;
				measThisSys = 1;
				sysWidthUsed = gutter2;
				staffLengthUse = staffLenUse2;
			}

		sysWidthUsed += measTable[m].width;
		measTable[m].newSysNum = prevSysNum;
		/*
		 *	If this is the 1st Measure of the System, we're not going to combine it
		 *	with the previous Measure, no matter what.
		 */
		if (measThisSys==1 && measTable[m].secondPiece)
				measTable[m].secondPiece = FALSE;
	}

	DebugRfmtTable(measTable,mCount,sysWChecked,staffLengthUse);

	/* If every Measure will stay in the same System, nothing really needs to be done. */
	
	oldSysNum = startSysNum;
	sysL = startSysL;
	for (m = 0; m<mCount && !doSomething; m++) {
		if (measTable[m].systemL!=sysL) {
			oldSysNum++;
			sysL = measTable[m].systemL;
		}
		if (measTable[m].newSysNum!=oldSysNum)
			doSomething = TRUE;
	}
		
	return (doSomething? OP_COMPLETE : NOTHING_TO_DO);
}


/* ------------------------------------------------------------------ FixMeasVis -- */
/* Given a "measure table" ala Reformat, make visible every Measure that used to be
the first one in a System but isn't any more. */

static void FixMeasVis(Document */*doc*/, MEASDATA measTable[], short mCount)
{
	LINK sysL;
	short m;
	
	sysL = measTable[0].systemL;		/* Reformat never touches the 1st Meas in table */
	for (m = 0; m<mCount; m++)
		if (measTable[m].measL) {
			if (measTable[m].systemL!=sysL && !FirstMeasInSys(measTable[m].measL))
				SetMeasVisible(measTable[m].measL, TRUE);
			sysL = measTable[m].systemL;
		}
}
	

/* ------------------------------------------------------------- AddFinalMeasure -- */

static Boolean AddFinalMeasure(Document *doc, LINK startSysL, LINK newSysL,
											DDIST measWidth)
{
	LINK		prevMeasL, staffL, newMeasL, aMeasureL;
	DDIST 	staffTop[2];										/* Dummy, for MakeMeasure */		
	CONTEXT	theContext;

	prevMeasL = SSearch(startSysL, MEASUREtype, GO_LEFT);
	staffL = SSearch(startSysL, STAFFtype, GO_LEFT);
	if (!prevMeasL || !staffL) {
		MayErrMsg("AddFinalMeasure: SSearch from %ld couldn't find MEASURE or STAFF.", (long)startSysL);
		return FALSE;
	}
	newMeasL = MakeMeasure(doc, LeftLINK(startSysL), prevMeasL, staffL, newSysL,
																	measWidth, staffTop, succMeas);
	if (!newMeasL) return FALSE;
	
	SetMeasVisible(newMeasL, TRUE);
	aMeasureL = FirstSubLINK(newMeasL);
	for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
		GetContext(doc, LeftLINK(newMeasL), MeasureSTAFF(aMeasureL), &theContext);
		FixMeasureContext(aMeasureL, &theContext);
	}
	return TRUE;
}


/* ----------------------------------------------------------------- GetSysWhere -- */
/* The location of the new System needs to be determined in order to get the
parameters correct for CreateSystem, even though after the system is created,
the order of things will be re-arranged. If there is no preceding System, we are
creating the new System before the first System of the score. Otherwise, if the
preceding System is before startSysL's page, the newly created System will be
the first on the page; otherwise, it's a "succeeding System". This code assumes
that LinkLSYS(startSysL) is valid! */

short GetSysWhere(LINK startSysL, LINK newSysL)
{
	LINK pageL;

	/*
	 * newSysL is NILINK for the first newly created system; after this,
	 * it's no longer NILINK and all moved systems are succSystems.
	 */
	if (newSysL==NILINK) {
		if (LinkLSYS(startSysL)) {
			pageL = SSearch(startSysL, PAGEtype, GO_LEFT);
			return (IsAfter(pageL, LinkLSYS(startSysL)) ? succSystem : firstOnPage);
		}
		return beforeFirstSys;
	}

	return succSystem;
}

/* ----------------------------------------------------------------- RfmtSystems -- */
/* Reformat systems from <startSysL>, which must be a System, thru <endSysL>, in
effect by moving Measures from System to System to fill them as well as possible,
given that we never put more than <measPerSys> Measures on a System. Actually, we
destroy all the old Systems in the range and create new ones; this means that, when
we're done, <startSysL> is no longer in the object list! Returns FAILURE,
NOTHING_TO_DO, or OP_COMPLETE. */

static short RfmtSystems(
					Document	*doc,
					LINK		startSysL,
					LINK		endSysL,
					short		measPerSys,		/* Maximum no. of Measures allowed per System */
					Boolean	ignoreOver,		/* Ignore overflow and consider only <measPerSys> */
					LINK		*newStartSysL 	/* Output: the first newly-created System */
					)
{
	DDIST		prevEndPos, xMove,
				dspAfterBar, mWidth, topMargin;
	MEASDATA	*measTable;
	LINK		sysL, pL;
	LINK		newSysL, startMoveL, endMoveL, stL, measL, moveSysL,
				prevMeasL=NILINK, newMeasL;
	short		mCount, m, mPostRfmt;
	short		prevSysNum, sysWhere, status, returnCode;
	Boolean	moveRJD, moveMJD;
	CONTEXT	context;
	long		timeMove;

	measTable = AllocMeasTable(doc,startSysL,endSysL);
	if (!measTable) {
		returnCode = FAILURE; goto Cleanup;
	}
	
	/* Build a table of widths and current owning Systems of the Measures to be
	 	reformatted, with a blank slot for the new System number of each Measure. */

	mCount = BuildMeasTable(doc, startSysL, endSysL, measTable);

	/* Fill in the new System number each Measure will have. */
	 
	status = NewSysNums(doc, measPerSys, ignoreOver, measTable, mCount, startSysL);
	if (status!=OP_COMPLETE) { returnCode = status; goto Cleanup; }
	
	ProgressMsg(ARRANGEMEAS_PMSTR, "");

	/*
	 *	Traverse the Measures to be reformatted.  We create new Systems in the object
	 *	list, inserting them before the System that originally held the first
	 *	Measure to be reformatted.  Each newly created System is then filled with
	 *	consecutive Measures (as decided in NewSysNums) by moving them in the object
	 * list, one at a time, with MoveRange.
	 *
	 *	Any JD objects (Graphics, Tempos, and Endings) that are now attached either to
	 * objects in a reserved area or to its terminating Measure object, or that are
	 *	attached to a Measure that will end up terminating a reserved area, will need
	 *	updating, since their attached objects are going to disappear and be recreated:
	 *	this is what <moveRJD> and <moveMJD> detect.
	 */
	prevSysNum = -1;
	newSysL = NILINK;
	for (m = 0; m<mCount; m++) {
	
		/*
		 *	Get the Measure's barline and the first object after the end of the Measure.
		 * Also get the corresponding post-reformatting Measure: the same as the
		 * current Measure unless it's now the first in its System.
		 */
		startMoveL = measL = measTable[m].measL;
		endMoveL = EndMeasSearch(doc, measTable[m].measL);
		mPostRfmt = (measTable[m].firstOfSys && m>0? m-1 : m);
		newMeasL = measTable[mPostRfmt].measL;

		moveRJD = (measTable[m].firstOfSys!=0);
		if (moveRJD)
			moveSysL = SSearch(measL,SYSTEMtype,GO_LEFT);

		/* Check if it's necessary to create a new destination System. */

		if (measTable[m].newSysNum!=prevSysNum) {

			/*
			 *	It is necessary, which means that the Measure we're about to move will
			 * be the first Measure in the System (e.g., it follows the "invisible"
			 * barline). We don't know the vertical position of the System yet, but
			 * NewSheetNums uses this no. to decide page breaks, so it can't be any
			 * greater than the top margin; also, if it ends up as top system of its
			 * page, it'll stay at this position. So we set it to the top margin. The
			 * location of the new System relative to other Systems and Pages also
			 * needs to be determined, for CreateSystem.
			 */
			sysWhere = GetSysWhere(startSysL,newSysL);
			topMargin = pt2d(doc->marginRect.top);
			newSysL = CreateSystem(doc, LeftLINK(startSysL), topMargin, sysWhere);
			if (!newSysL) {
				returnCode = FAILURE;
				goto Cleanup;
			}

			/*
			 * Save LINK of the first new System; update LINK to the first Measure of
			 * every new System.
			 */
			if (m==0) *newStartSysL = newSysL;

			moveMJD = PrepareMoveMeasJDObjs(doc,measL,prevMeasL);
			/*
			 *	Since the new System already has a Measure, get rid of this Measure's
			 * barline before moving everything else in it back onto the new System.
			 */
			startMoveL = RightLINK(startMoveL);
			DeleteNode(doc, measTable[m].measL);
			measTable[m].measL = NILINK;

			prevEndPos = SysRelxd(LeftLINK(startSysL));

			/* Don't do this again until we get to a Measure with a new System number */

			prevSysNum = measTable[m].newSysNum;
		}

		if (moveRJD)
			MoveSysJDObjs(doc,moveSysL,startMoveL,startSysL,(m==0? NILINK : newMeasL));

		/*
		 * Decide how far to move the Measure graphically (right or left) to get it in
		 *	its proper horizontal position in the new System, and move the Measure, both
		 *	logically and graphically. If the Measure is a <secondPiece>, we want to
		 * combine it with its other piece, so skip the Measure object and just move
		 * its content to the new location, and also adjust its timestamp. Exception:
		 * if the Measure has no content, skip all of that. Regardless whether it has
		 * content, advance <prevEndPos> so we can position the next Measure.
		 */
		stL = (measTable[m].secondPiece? RightLINK(startMoveL) : startMoveL);
		if (stL!=endMoveL) {
			MoveRange(stL, endMoveL, startSysL);
			if (moveMJD)
				MoveMeasJDObjs(doc,newSysL,prevMeasL);
			prevMeasL = stL;
			if (measTable[m].secondPiece) {
			
				/* Moving the Measure content right by the width of the previous Measure
				 	(its other piece) is a bit too much because currently it's positioned
				 	to be the standard distance after the barline, so we have to compensate
				 	for that. Even though the object list is not in a consistent state, it
				 	should be safe to call GetContext at startMoveL, since it's a Measure. */

				GetContext(doc, startMoveL, 1, &context);
				dspAfterBar = std2d(config.spAfterBar, context.staffHeight, context.staffLines);
				xMove = measTable[m-1].width-dspAfterBar;
				MoveInMeasure(stL, endMoveL, xMove);
				
				timeMove = measTable[m].lTimeStamp-measTable[m-1].lTimeStamp;
				MoveTimeInMeasure(stL, endMoveL, timeMove);
			}
			else {
				xMove = prevEndPos-SysRelxd(startMoveL);
				MoveMeasures(stL, endMoveL, xMove);
			}
		}

		prevEndPos += measTable[m].width;
		
 		/*
 		 *	If this Measure will be the last in the new System - unless either it's the
 		 * last Measure being reformatted, or it was last in its old System - add a
 		 *	Measure after it to fill out the new System.
		 */
		if (m+1<mCount &&
				measTable[m+1].newSysNum!=prevSysNum &&
				!measTable[m].lastOfSys) {

			mWidth = measTable[m].width;
			if (measTable[m].secondPiece)
				mWidth += measTable[m-1].width;

			if (!AddFinalMeasure(doc, startSysL, newSysL, mWidth)) {
				returnCode = FAILURE;
				goto Cleanup;
			}

			{
				/* Set the type of this terminating barline to the same as the barline
					that formerly terminated this measure.
					
					NB: If that type is BAR_RPT_L or BAR_RPT_LR, we should really do something
					more sophisticated, including creating an additional barline at the
					beginning of the NEXT system. Unfortunately, we haven't created that
					system yet. What to do? (And shouldn't the additional barline be fake?)
				*/
				
				short	type = measTable[m+1].barType;
				LINK	termMeasL = SSearch(startSysL, MEASUREtype, GO_LEFT);
				/* ??Grrr...Let's make AddFinalMeasure return the LINK of the meas it makes */
	
				switch (type) {
					case BAR_RPT_LR:
					case BAR_RPT_L:
					case BAR_RPT_R:
					case BAR_DOUBLE:
					case BAR_FINALDBL:
					case BAR_HEAVYDBL:
						SetMeasureSubType(termMeasL, type);
						break;
					case BAR_SINGLE:
						/* nothing to do */
						break;
				}
			}
		}
	}

	/*
	 *	We've moved everything to its new home. Obliterate the empty, lifeless ghost
	 *	town of Systems (and their neighboring Staffs, Connects, etc., plus perhaps
	 *	some Pages) the Measures used to populate. We'll update page numbers as well as
	 *	system and measure numbers unconditionally--it's easier and should cause no
	 * problems. Before doing anything, however, we have to mark any cross-system
	 * Slurs attached to them as being first pieces of cross-system Slur pairs,
	 *	since otherwise the routine that updates them won't be able to tell which piece
	 *	they are. We don't NOT have to to mark cross-system Slurs attached to Measures
	 *	because, at the point where FixCrossSysObjects is called below, the Measures
	 *	will still be recognizable as Measure objects (even though they won't be in
	 * the object list any longer), and that's all FixCrossSysObjects needs. (We don't
	 * need to go thru the ENTIRE object list looking for slurs to mark; some day
	 *	this should be optimized.)
	 */
	sysL = startSysL;
	for ( ; sysL && sysL!=measTable[mCount-1].systemL; sysL = LinkRSYS(sysL)) {	 
		for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
			if (SlurTYPE(pL) && SlurLASTSYNC(pL)==sysL)
				SlurLASTSYNC(pL) = NILINK;
	}
	
	DeleteRange(doc, startSysL, endSysL);				/* OK bcs does not depend on cross-links */
	FixStructureLinks(doc, doc, doc->headL, doc->tailL);	/* Overkill; optimize some day */

	UpdatePageNums(doc);
	UpdateSysNums(doc, doc->headL);
	UpdateMeasNums(doc, NILINK);

	/*
	 * The selection range endpoints may have been deleted. We'll just set them to
	 * include everything now, then reset to something reasonable later.
	 */
	doc->selStartL = doc->selEndL = doc->tailL;
	
	/*
	 *	Fix up cross-System objects. Existing cross-System objects that are now in
	 *	the same System must be made non-cross-System; non-cross-System objects crossing
	 *	from a Measure that's now in one System into a Measure that's now in another
	 *	System must be made cross-System.
	 */
	FixCrossSysObjects(doc, *newStartSysL, endSysL);

	/* Make visible Measures that used to be first in their Systems but aren't now. */
	
	FixMeasVis(doc, measTable, mCount);

	ProgressMsg(0, "");
	if (measTable) DisposePtr((Ptr)measTable);
	return OP_COMPLETE;

Cleanup:
	ProgressMsg(0, "");
	if (measTable) DisposePtr((Ptr)measTable);
	return returnCode;
}


/* --------------------------------------------------------------- NPrevSysInPage -- */
/* Return the number of Systems in its Page preceding sysL, which must be a System:
0=none (i.e., it's the first in the Page), etc. */

short NPrevSysInPage(LINK sysL)
{
	LINK pageL, firstSysL;
	
	pageL = SysPAGE(sysL);
	firstSysL = SSearch(pageL, SYSTEMtype, GO_RIGHT); 
	return SystemNUM(sysL)-SystemNUM(firstSysL);
}


/* ---------------------------------------------------------------- GetRfmtRange -- */
/* Get the actual range of systems to be reformatted. This is the range of systems
in the object list segment [startSysL,endL), plus all systems following the last
system of the range which are on the same page as that last system. */

static short GetRfmtRange(Document *doc, SYSDATA sysTable[], short sCount)
{
	short s,nSystems,newSCount=sCount;
	LINK pageL;
	
	nSystems = CountSystems(doc,sysTable[0].sysL);
	pageL = sysTable[sCount-1].pageL;

	for (s=sCount; s<nSystems; s++) {
		if (sysTable[s].pageL==pageL)
			  newSCount++;
		else break;
	}
	
	return newSCount;
}

/* -------------------------------------------------------------- CheckSysHeight -- */
/*	Check all Systems in <sysTable> to see if any single System is too tall for the
height of the page. If so, if it's only because of the score's first System's extra
top margin, return CAN_FIT, otherwise return NO_WAY. If there's no problem, return
ALL_OK. */

enum {
	ALL_OK,
	CAN_FIT,		/* If the first System's extra top margin is reduced */
	NO_WAY
};
	
static short CheckSysHeight(Document *doc, SYSDATA sysTable[], short sCount, short titleMargin)
{
	DDIST topMargin,pageHeightUsed,pageHeightAvail,fullPgHeightUsed;
	short s=0;

	pageHeightAvail = pt2d(doc->marginRect.bottom);				/* Must include top margin! */
	
	/* If the first System of the score is to be reformatted, check it. Then check the
		remaining systems we're reformatting. We're always comparing the position the
		bottom of the System would have if it were the only System on the page, to the
		bottom margin position. */
	
	if (FirstSysInScore(sysTable[0].sysL)) {
		topMargin = pt2d(doc->marginRect.top);
		pageHeightUsed = topMargin+sysTable[0].height;
		fullPgHeightUsed = pageHeightUsed+pt2d(titleMargin);
		if (fullPgHeightUsed>pageHeightAvail) {
			if (pageHeightUsed>pageHeightAvail) return NO_WAY;
			else return CAN_FIT;
		}
		s++;
	}

	/* Check the remaining Systems in the table */
	
	for ( ; s<sCount; s++) {
		pageHeightUsed = pt2d(doc->marginRect.top)+sysTable[s].height;

		if (pageHeightUsed>pageHeightAvail) {
			return NO_WAY;
		}
	}

	return ALL_OK;
}

/* --------------------------------------------------------------- DebugSysTable -- */

#ifdef RFMTBUG
static void DebugSysTable(SYSDATA sysTable[], short s, DDIST pgHtUsed, DDIST pgHtAvail,
									short pgSysNum)
{
	DebugPrintf("sys[%d] sysL=%d pageL=%d height=%d (htUsed=%d Avail=%d)",
		s, sysTable[s].sysL,sysTable[s].pageL,sysTable[s].height,
		pgHtUsed, pgHtAvail);
	if (pgSysNum<=1) DebugPrintf(" newShtNum=%d", sysTable[s].newSheetNum);
	DebugPrintf("\n");
}
#else
static void DebugSysTable(SYSDATA [], short, DDIST, DDIST, short)
{}
#endif

/* ---------------------------------------------------------------- NewSheetNums -- */
/*	Fill in the new sheet number each System will have. Start a new Page when
either the current Page overflows, or the desired number of Systems per Page is
exceeded. Return TRUE if any System should be moved to another Page, else FALSE. */

static Boolean NewSheetNums(
						Document *doc,
						LINK startSysL,
						short sysPerPage,			/* Maximum no. of Systems allowed per Page */
						SYSDATA sysTable[],
						short sCount,				/* No. of Systems in <sysTable> */
						short titleMargin
						)
{
	short s, prevSheetNum,pgSysNum,oldSheetNum,startSheetNum;
	DDIST pgHtUsed,pgHtAvail,topMargin,firstMargin,prevPgHtUsed;
	Boolean tooManySys,doSomething=FALSE;
	LINK pageL;
	
	prevSheetNum = startSheetNum = SheetNUM(SysPAGE((startSysL)));
	
	topMargin = pt2d(doc->marginRect.top);
	pgHtAvail = pt2d(doc->marginRect.bottom);
	firstMargin = topMargin+pt2d(titleMargin);
	if (FirstSysInScore(sysTable[0].sysL))
		pgHtUsed = firstMargin;
	else
		/* Set the page height used from the system's CURRENT position, since it
			might be in the middle of the page. */
			
		pgHtUsed = SystemRECT(sysTable[0].sysL).top;
	
	pgSysNum = NPrevSysInPage(sysTable[0].sysL);
	
	/*
	 * For each System, compare the vertical space required by it and previous Systems
	 *	on the page to the available height, and compare the number of Systems on the
	 * page including it to the specified limit. If either is exceeded, put it on a
	 * new page.
	 */
	for (s = 0; s<sCount; s++) {
		pgHtUsed += sysTable[s].height;
		pgSysNum++;
		tooManySys = (pgSysNum>sysPerPage);
		prevPgHtUsed = pgHtUsed;
		if (pgHtUsed>pgHtAvail || tooManySys) {
			prevSheetNum++;
			pgHtUsed = topMargin+sysTable[s].height;
			pgSysNum = 1;
		}
		sysTable[s].newSheetNum = prevSheetNum;

		DebugSysTable(sysTable,s,prevPgHtUsed,pgHtAvail,pgSysNum);
	}

	/* If every System will stay in the same Page, nothing really needs to be done. */
	
	oldSheetNum = startSheetNum;
	pageL = SysPAGE(startSysL);
	for (s = 0; s<sCount && !doSomething; s++) {
		if (sysTable[s].pageL!=pageL) {
			oldSheetNum++;
			pageL = sysTable[s].pageL;
		}
		if (sysTable[s].newSheetNum!=oldSheetNum) doSomething = TRUE; 
	}
	
	return doSomething;
}

/* -------------------------------------------------------------- RfmtResetSlurs -- */
/* Mark any cross-system Slurs attached to its initial Measure as being second
pieces of cross-system Slur pair, and mark any cross-system Slurs attached
to the System as being first pieces of cross-system Slur pairs, since otherwise
the routine that updates them won't be able to tell which piece they are. ??WE
DON'T HAVE TO GO THRU ENTIRE OBJECT LIST LOOKING FOR SLURS TO MARK! */

static LINK RfmtResetSlurs(Document *doc, LINK startMoveL)
{
	LINK pL,measL,sysL;

	measL = LSSearch(startMoveL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		if (SlurTYPE(pL) && SlurFIRSTSYNC(pL)==measL)
			SlurFIRSTSYNC(pL) = NILINK;

	/* Most of the time startMoveL is a System, but it might be a Page */
	sysL = LSSearch(startMoveL, SYSTEMtype, ANYONE, GO_RIGHT, FALSE);
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		if (SlurTYPE(pL) && SlurLASTSYNC(pL)==sysL)
			SlurLASTSYNC(pL) = NILINK;

	return measL;
}

/* ------------------------------------------------------------- SFirstMeasInSys -- */
/* Return TRUE if measL is the first Measure of its System. measL must be a
Measure. */

static Boolean SFirstMeasInSys(LINK measL)
{
	LINK sysL,lMeas;

	lMeas = SSearch(LeftLINK(measL), MEASUREtype, GO_LEFT);
	if (lMeas) {
		sysL = SSearch(measL, SYSTEMtype, GO_LEFT);
		if (sysL) 
			return(IsAfter(lMeas, sysL));
		return FALSE;										/* There's a Measure preceding but no System */
	}
	return TRUE;											/* This is first Measure of entire score */
}


/* ----------------------------------------------------- MoveXXXJDObjs functions -- */
/* These functions are to update links in JD objects attached to objects that may
disappear and be re-created by reformatting. The only objects that reformatting
re-creates are those in reserved areas of Systems plus Measures; the only objects
that can be attached to these objects are Graphics, Tempos, and Endings. */

/* In the range [sysL,measL), find every Graphic, Tempo, and Ending that's attached
to an object in the reserved area. For each, find the corresponding post-reformat
object to re-attach the Graphic/Tempo/Ending to, and move and re-attach it. The
question is, WHICH is the corresponding object? We currently use newMeasL (which
should be the Measure that Reformat is putting before the following content), unless
this is the first Measure of the first System being reformatted: in that case,
newMeasL should be NILINK, and we'll use the System before endL (which should be the
new System that Reformat is replacing this System with). This is fine unless the
content of the following Measure is going to stay at the beginning of a System; in
that case what it does is rather silly. */

static void MoveSysJDObjs(
		Document */*doc*/,
		LINK sysL, LINK measL,	/* Range to consider: normally a reserved area */
		LINK endL,					/* Corresponding System is first before this */
		LINK newMeasL 				/* Corresponding new Measure */
		)
{
	LINK newSysL,pL,firstObjL,newFirstObjL,nextL;
	short type;

	newSysL = SSearch(LeftLINK(endL),SYSTEMtype,GO_LEFT);
	if (!newSysL) {
		MayErrMsg("MoveSysJDObjs: unable to find dst sys before %ld", newSysL);
		return;
	}

	for (pL=sysL; pL!=measL; pL=nextL) {
		nextL = RightLINK(pL);
		if (J_DTYPE(pL)) {
			switch (ObjLType(pL)) {
				case GRAPHICtype:
					firstObjL = GraphicFIRSTOBJ(pL);
					break;
				case TEMPOtype:
					firstObjL = TempoFIRSTOBJ(pL);
					break;
				case ENDINGtype:
					firstObjL = EndingFIRSTOBJ(pL);
					break;
				default:
					firstObjL = NILINK;
					break;
			}

			if (!firstObjL) continue;

			/* Only move JD objects which are attached to objects that may be in the
				reserved area, including its terminating Measure. */

			type = ObjLType(firstObjL);

			if (type==CLEFtype && ClefINMEAS(firstObjL)) continue;
			if (type==KEYSIGtype && KeySigINMEAS(firstObjL)) continue;
			if (type==TIMESIGtype && TimeSigINMEAS(firstObjL)) continue;

			switch (type) {
				case CLEFtype:
				case KEYSIGtype:
				case TIMESIGtype:
				case MEASUREtype:
					if (newMeasL) newFirstObjL = newMeasL;
					else			  newFirstObjL = SSearch(newSysL,type,GO_RIGHT);
					if (newFirstObjL && newFirstObjL!=firstObjL) {
						MoveNode(pL, newFirstObjL);
						switch (ObjLType(pL)) {
							case GRAPHICtype:
								GraphicFIRSTOBJ(pL) = newFirstObjL;
								break;
							case TEMPOtype:
								TempoFIRSTOBJ(pL) = newFirstObjL;
								break;
							case ENDINGtype:
								EndingFIRSTOBJ(pL) = newFirstObjL;
								break;
						}
					}
					break;
				default:
					break;
			}
		}
	}
}

/* Find every Graphic, Tempo, and Ending that's attached to the Measure <measL>, and
set their object-attached-to links to NILINK to indicate they need updating. If we
find any, return TRUE, else FALSE. */

static Boolean PrepareMoveMeasJDObjs(
						Document *doc,
						LINK measL,			/* Measure to consider */
						LINK prevMeasL 	/* Measure to search for objects attached to <measL> */
						)
{
	LINK pL, endMeasL; Boolean didSomething;
	
	if (!measL || !prevMeasL) return FALSE;
	/*
	 *	Objects attached to <measL> were originally in the Measure preceding it; the
	 * contents of that Measure should already have been moved elsewhere, but they
	 *	begin with <prevMeasL>.
	 */
	if (MeasureTYPE(prevMeasL)) prevMeasL = RightLINK(prevMeasL);
	endMeasL = EndMeasSearch(doc, prevMeasL);
	
	didSomething = FALSE;
	for (pL = prevMeasL; pL!=endMeasL; pL = RightLINK(pL)) {
		if (J_DTYPE(pL))
			switch (ObjLType(pL)) {
				case GRAPHICtype:
					if (GraphicFIRSTOBJ(pL)==measL) {
						GraphicFIRSTOBJ(pL) = NILINK;
						didSomething = TRUE;
					}
					break;
				case TEMPOtype:
					if (TempoFIRSTOBJ(pL)==measL) {
						TempoFIRSTOBJ(pL) = NILINK;
						didSomething = TRUE;
					}
					break;
				case ENDINGtype:
					if (EndingFIRSTOBJ(pL)==measL) {
						EndingFIRSTOBJ(pL) = NILINK;
						didSomething = TRUE;
					}
			}
	}
	
	return didSomething;
}

/* Find every Graphic, Tempo, and Ending that has an object-attached-to link of
NILINK, and set its object-attached-to link to the first Measure of the given
System. */

static Boolean MoveMeasJDObjs(
							Document *doc,
							LINK newSysL,
							LINK prevMeasL		/* Measure to search for NILINKs */
							)
{
	LINK pL, endMeasL, newMeasL, nextL;
	
	if (!prevMeasL) return FALSE;

	if (MeasureTYPE(prevMeasL)) prevMeasL = RightLINK(prevMeasL);
	endMeasL = EndMeasSearch(doc, prevMeasL);
	newMeasL = SSearch(newSysL, MEASUREtype, GO_RIGHT);

	for (pL = prevMeasL; pL!=endMeasL; pL = nextL) {
		nextL = RightLINK(pL);
		if (J_DTYPE(pL))
			switch (ObjLType(pL)) {
				case GRAPHICtype:
					if (GraphicFIRSTOBJ(pL)==NILINK) {
						MoveNode(pL, newMeasL);
						GraphicFIRSTOBJ(pL) = newMeasL;
					}
					break;
				case TEMPOtype:
					if (TempoFIRSTOBJ(pL)==NILINK) {
						MoveNode(pL, newMeasL);
						TempoFIRSTOBJ(pL) = newMeasL;
					}
					break;
				case ENDINGtype:
					if (EndingFIRSTOBJ(pL)==NILINK) {
						MoveNode(pL, newMeasL);
						EndingFIRSTOBJ(pL) = newMeasL;
					}
			}
	}
	
	return TRUE;
}


/* -------------------------------------------------------------- GetTitleMargin -- */
/* Return the size, in points, of the given score's extra margin at the top of its
first page. */

short GetTitleMargin(Document *doc)
{
	LINK startSysL; DDIST topMargin; PSYSTEM pSystem; short titleMargin;
	
	startSysL = LSSearch(doc->headL, SYSTEMtype, ANYONE, GO_RIGHT, FALSE);
	pSystem = GetPSYSTEM(startSysL);
	topMargin = pSystem->systemRect.top;
	titleMargin = d2pt(topMargin)-doc->marginRect.top;
	return titleMargin;
}


/* ------------------------------------------------------------------- RfmtPages -- */
/* Reformat systems from <startSysL>, which must be a System, thru <endSysL>, in
effect by moving Systems from Page to Page to fill them as well as possible,
given that we never put more than <sysPerPage> Systems on a Page. Actually, we
destroy all the old Pages in the range and create new ones, replacing some Systems
along the way; this means that, when we're done, <startSysL> may no longer be in
the object list! Returns FAILURE, NOTHING_TO_DO, or OP_COMPLETE. */

static short RfmtPages(Document *doc,
							LINK startSysL, LINK endSysL,
							short sysPerPage,					/* Max. no. of Systems allowed per Page */
							short titleMargin)
{
	SYSDATA	*sysTable;
	LINK		newPageL, nextSysL, firstPageL=NILINK, lastObjL,
				measL, startMoveL, endMoveL, newMeasL;
	short		startSheetNum, sCount, prevSheetNum, returnCode;
	short		s;
	Boolean	sheetChange;

	sysTable = AllocSysTable(doc, startSysL);
	if (!sysTable) {
		returnCode = FAILURE; goto Cleanup;
	}

	/* Build a table of heights and current owning Pages of the Systems to be
		reformatted, with a blank slot for the new sheet number of each System. */

	sCount = BuildSysTable(doc, startSysL, endSysL, sysTable);
	
	/* Get the actual range of systems to be reformatted: from index 0 in the
		table to the index of the last system in the table needing reformatting. */

	sCount = GetRfmtRange(doc,sysTable,sCount);

	/* Check to see if a single system is too tall for the height of a page. ??Bug:
		it's too late to do this in RfmtPages: RfmtSystems has already moved Measures
		into new systems! */

	if (CheckSysHeight(doc,sysTable,sCount, titleMargin)!=ALL_OK) {
		if (CheckSysHeight(doc,sysTable,sCount, titleMargin)==CAN_FIT)
			GetIndCString(strBuf, MISCERRS_STRS, 7);	/* "Can't reformat bcs the 1st system is too tall..." */
		else
			GetIndCString(strBuf, MISCERRS_STRS, 2);	/* "Can't reformat bcs a single system is too tall..." */
		CParamText(strBuf,	"", "", "");
		StopInform(BIG_GENERIC_ALRT);
		returnCode = FAILURE;
		goto Cleanup;
	}

	/*
	 * Fill in the new sheet number each System will have. If every System will stay on
	 * the same page and there's no change to the margin at the top of the first page,
	 * there's nothing to do.
	 */
	sheetChange = NewSheetNums(doc, startSysL, sysPerPage, sysTable, sCount, titleMargin);
	if (!sheetChange && titleMargin==GetTitleMargin(doc)) {
		returnCode = NOTHING_TO_DO; goto Cleanup;
	}
	
	if (sheetChange) ProgressMsg(ARRANGESYS_PMSTR, "");
	else             ProgressMsg(CHANGE_TOPMARGIN_PMSTR, "");

	/*
	 * Yes, there's work to be done: reformat. Note that we may simply need to change
	 * the top margin of the first page, in which case we're going to do a lot more
	 * than we need to; I don't think that'll cause any real problems, though.
	 *	Traverse the Systems to be reformatted.  The first one or more Systems may
	 *	stay where they are. For following Systems, we create new Pages in the object
	 *	list, inserting them at the end of the range to be reformatted.  Each
	 *	newly created Page is then filled with consecutive Systems (as decided in
	 *	NewSheetNums) by moving them in the object list, one at a time, with MoveRange.
	 */

	startSheetNum = SheetNUM(SysPAGE((startSysL)));
	prevSheetNum = startSheetNum;

	for (s=0; s<sCount; s++) {

		/* Get System object and first object beyond System */
		startMoveL = sysTable[s].sysL;
		lastObjL = endMoveL = EndSystemSearch(doc, sysTable[s].sysL);
		newMeasL = SSearch(sysTable[s].sysL, MEASUREtype, GO_RIGHT);

		if (sysTable[s].newSheetNum!=prevSheetNum) {

			/* Create a new destination Page. */
			newPageL = CreatePage(doc, LeftLINK(endSysL));
			if (!newPageL) {
				returnCode = FAILURE; goto Cleanup;
			}

/* DebugPrintf("CP: newPageL=%d\n", newPageL); */

			/* Save the LINK of the first new Page created */
			if (!firstPageL) firstPageL = newPageL;
			newMeasL = SSearch(newPageL, MEASUREtype, GO_RIGHT);

			/* If the first object beyond the System was endSysL, the first object
				beyond it is may now the Page we just created, so adjust endMoveL
				accordingly. */ 
			if (endMoveL==endSysL) endMoveL = newPageL;

			/*
			 *	Since the new Page already has a System with all its associated struct-
			 *	ural objects, get rid of this System's System object and associated
			 *	objects, including its initial Measure, before moving everything else in
			 *	it forward onto the new Page. Before doing so, however, we have to mark
			 *	any cross-system Slurs.
			 */

			measL = RfmtResetSlurs(doc,startMoveL);
			startMoveL = RightLINK(measL);

			MoveSysJDObjs(doc, sysTable[s].sysL,startMoveL,endSysL, newMeasL);
			DeleteRange(doc, sysTable[s].sysL, startMoveL);
/* DebugPrintf("DR: sysL=%d startMoveL=%d\n", sysTable[s].sysL,startMoveL); */

			doc->nsystems--;
			sysTable[s].sysL = NILINK;
		
			/* Don't do this again until we get to a System with a new sheet number */

			prevSheetNum = sysTable[s].newSheetNum;
		}

		/*
		 * We're finally ready: move the entire content of the System to its new home.
		 */
		MoveRange(startMoveL, endMoveL, endSysL);
/* DebugPrintf("MR: startMoveL=%d endMoveL=%d endSysL=%d\n",startMoveL,endMoveL,endSysL); */

		/*
		 * If there's a Page before the next System and this System is not the end of
		 * the range being reformatted, then after doing the move, delete everything after
		 * where this System was until the next System. In addition to the Page, this may
		 * include Page-relative Graphics.
		 */
		if (PageTYPE(lastObjL) && s!=sCount-1) {
			nextSysL = SSearch(lastObjL, SYSTEMtype, GO_RIGHT);
			DeleteRange(doc, lastObjL, nextSysL);
/* DebugPrintf("DR: lastObjL=%d nextSysL=%d\n",lastObjL,nextSysL); */
		}

		FixStructureLinks(doc, doc, doc->headL, doc->tailL);
	}

	UpdatePageNums(doc);
	UpdateSysNums(doc, doc->headL);
	UpdateMeasNums(doc, NILINK);

	/*	Fix up cross-System objects that used to be attached to objects we've deleted. */
	
	FixCrossSysObjects(doc, firstPageL, endSysL);
	
	ProgressMsg(0, "");
	if (sysTable) DisposePtr((Ptr)sysTable);
	return OP_COMPLETE;
	
Cleanup:
	ProgressMsg(0, "");
	if (sysTable) DisposePtr((Ptr)sysTable);
	return returnCode;
}


/* ------------------------------------------------------------ FixFirstSysRectY -- */

static void FixFirstSysRectY(Document *doc, short titleMargin);
static void FixFirstSysRectY(Document *doc, short titleMargin)
{
	LINK startSysL; DDIST topMargin, sysHeight; PSYSTEM pSystem;

	startSysL = LSSearch(doc->headL, SYSTEMtype, ANYONE, GO_RIGHT, FALSE);
	topMargin = pt2d(doc->marginRect.top);
	topMargin += pt2d(titleMargin);

	pSystem = GetPSYSTEM(startSysL);
	sysHeight = pSystem->systemRect.bottom-pSystem->systemRect.top;
	pSystem->systemRect.top = topMargin;
	pSystem->systemRect.bottom = pSystem->systemRect.top+sysHeight;

}


/* ---------------------------------------------------- RJustifySystems and ally -- */
/* In the given range of systems, justify either all systems or only those that
are overflowing. */

Boolean DanglingContent(Document *, LINK);
Boolean RJustifySystems(Document *, LINK, LINK, Boolean);

Boolean DanglingContent(Document *doc, LINK sysL)
{
	LINK endL; DDIST endxd;
	
	endL = LastObjInSys(doc, RightLINK(sysL));
	endxd = (SysRectLEFT(sysL)+SysRelxd(endL));
	return (endxd > SysRectRIGHT(sysL)+RIGHTEND_SLOP);
}

Boolean RJustifySystems(
				Document *doc,
				LINK startL, LINK endL,
				Boolean doAll				/* TRUE=justify all, FALSE=justify only systems that overflow */
				)
{
	LINK startSysL, endSysL, currSysL, nextSysL, termSysL, firstMeasL, lastMeasL;
	Boolean didAnything=FALSE;

	GetSysRange(doc, startL, endL, &startSysL, &endSysL);
	endSysL = LinkRSYS(endSysL);							/* AFTER the last System to justify */
			
	/* For every System in the specified area... */
	
	for (currSysL = startSysL; currSysL!=endSysL; currSysL = nextSysL) {
		
		/* Get first and last Measure of this System and object ending it */
		
		firstMeasL = LSSearch(currSysL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
		termSysL = EndSystemSearch(doc, currSysL);
		lastMeasL = LSSearch(termSysL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
		nextSysL = LinkRSYS(currSysL);
		
		/*
		 *	If the System isn't empty--the invisible barline isn't the last barline
		 *	in the System--then we can justify the measures in it. (Actually, the
		 * System might contain a lot of material but no barlines besides the invisible
		 *	one; in that case, it COULD be justified, but we don't try.)
		 */
		if (lastMeasL!=firstMeasL) {
			if (doAll || DanglingContent(doc, currSysL))
				JustifySystem(doc, firstMeasL, lastMeasL);
			didAnything = TRUE;
		}
	}
	
	return didAnything;
}


/* -------------------------------------------------------------------- Reformat -- */
/* Reformat takes the range of Systems from the one containing <startL> to the one
one containing <endL> and rearranges things to fill them as well as possible by
moving Measures from System to System and, if necessary, creating new Systems and
Pages or deleting existing ones. It leaves spacing within the Measures alone.
Assumes it's operating on the active document, so it forces updating objRects and
redrawing the window. Caveat: it destroys selection status and sets an insertion pt.

Asking Reformat to change System breaks and not Page breaks would be likely to lead
to Systems dangling off the bottoms of their Pages, so it's not allowed: we always
consider changing Page breaks.

Reformat returns FAILURE if there's an error, or if we tell the user about a problem
(with hidden staves or pages overflowing the screen) and they cancel the operation;
else it returns NOTHING_TO_DO or OP_COMPLETE. Caveat: a return of FAILURE doesn't mean
"no reformatting done": we may have reformatted Systems and been unable to do Pages.*/

short Reformat(
			Document	*doc,
			LINK		startL,
			LINK		endL,
			Boolean	changeSBreaks,			/* Change System breaks? */
			short		measPerSys,				/* (Maximum) no. of Measures allowed per System */
			Boolean	justify,					/* Justify afterwards? */
			short 	sysPerPage, 			/* Maximum no. of Systems allowed per Page */
			short		titleMargin				/* In points */
			)
{
	LINK startSysL, endSysL, newStartSysL, firstMeasL, beforeL,
			startPageL, endPageL, pageL, pL;
	short returnCode, nMeasures;
	Boolean didAnything=FALSE, fixFirstSys, didChangeSBreaks=FALSE, nothingToDo=TRUE;
	Boolean exactMeasPerSys, foundHidden;
	static Boolean alreadyWarned=FALSE;
	
	/*
	 * We need to check in advance for possible screen-coordinate-overflow after
	 * reformatting, but it's not easy to compute how many pages the score will
	 * end up needing. For now, just take the worst case: assume every page of the
	 * entire score will end up contain only a single measure! This is ridiculously
	 * pessimistic, but note that (as of v.2.5) if the Screen Page Layout maxima
	 * can't overflow at the current magnification, ScreenPagesCanOverflow will
	 * know that there can't be a problem. Specifically, the maxima can't ever
	 * overflow if the magnification is 100 percent or less, and surely most people
	 * don't usually reformat when the magnification is above 100 percent. So this
	 * should not annoy users by "crying wolf" too much. (But if it does turn out to
	 * be obnoxious, note that all the Measures in Systems not being reformatted are
	 * going to end up occupying the same number of pages as now, maybe plus 1, so
	 * we could improve the estimate considerably without too much work.)
	 */
	nMeasures = CountRealMeasures(doc,doc->headL,doc->tailL);
	if (ScreenPagesCanOverflow(doc, doc->magnify, nMeasures)) {
		if (CautionAdvise(RFMT_QDOVERFLOW_ALRT)==Cancel) return FAILURE;
	}

	WaitCursor();
	MEHideCaret(doc);

	exactMeasPerSys = (measPerSys<0);
	measPerSys = ABS(measPerSys);

	GetSysRange(doc, startL, endL, &startSysL, &endSysL);
	endSysL = EndSystemSearch(doc, endSysL);			/* AFTER the last System to reformat */

	for (foundHidden = FALSE, pL = startSysL; pL!=endSysL; pL = RightLINK(pL))
		if (SystemTYPE(pL))
			if (NumVisStaves(pL)!=doc->nstaves) {
				foundHidden = TRUE;
				break;
			}
	if (foundHidden)
		if (CautionAdvise(RFMT_HIDDENSTAVES_ALRT)==Cancel) return FAILURE;
	
	fixFirstSys = FirstSysInScore(startSysL);

	if (changeSBreaks) {
		/*
		 *	Move Measures to new Systems and delete old ones as appropriate; then make
		 *	System and Measure numbers consistent again. N.B. RfmtSystems removes
		 *	<startSysL> from the object list!
		 */
		returnCode = RfmtSystems(doc, startSysL, endSysL, measPerSys, exactMeasPerSys,
											&newStartSysL);
		if (returnCode==FAILURE) return FAILURE;
		didAnything = (returnCode==OP_COMPLETE);
		didChangeSBreaks = (returnCode==OP_COMPLETE);
		if (returnCode!=NOTHING_TO_DO) nothingToDo = FALSE;
	}

	if (didAnything) {
		pageL = startPageL = SSearch(newStartSysL,PAGEtype,GO_LEFT);
		endPageL = SSearch(endSysL,PAGEtype,GO_LEFT);

		for ( ; pageL!=LinkRPAGE(endPageL); pageL=LinkRPAGE(pageL))
			PageFixSysRects(doc, pageL, TRUE);
	}
	else
		newStartSysL = startSysL;
	
	beforeL = LeftLINK(newStartSysL);
	/*
	 *	Move Systems to new Pages and delete old ones as appropriate; then make Page
	 * numbers consistent again. N.B. RfmtPages may remove <newStartSysL> from the
	 * object list!
	 */
	endPageL = EndPageSearch(doc, LeftLINK(endSysL));
	returnCode = RfmtPages(doc, newStartSysL, endPageL, sysPerPage, titleMargin);
	if (returnCode==FAILURE) return FAILURE;
	if (returnCode==OP_COMPLETE) {
		didAnything = TRUE;
		if (ScreenPagesExceedView(doc) && !alreadyWarned) {
			CautionInform(MANYPAGES_ALRT);
			alreadyWarned = TRUE;
		}
	}
	if (returnCode!=NOTHING_TO_DO) nothingToDo = FALSE;

	if (didAnything) {
		GetAllSheets(doc);
		RecomputeView(doc);

		/*
		 * Set measureRects' horizontal coordinates from their Measures' positions (this
		 *	must be done even if we only changed page breaks, since that creates new
		 *	systems at the top of each new page) and vertical coordinates from their Staffs'
		 *	positions.
		 */
		firstMeasL = LSSearch(RightLINK(beforeL), MEASUREtype, ANYONE, GO_RIGHT, FALSE);
		FixMeasRectXs(firstMeasL, NILINK);

		/*
		 * Assume that changing system breaks wipes out all local information about
		 *	systems; regenerate system and measureRects from the Master Page template.
		 * This doesn't sound like a great thing to do if the user has tweaked anything
		 *	with Work on Format: reconsider some day. If only page breaks have changed,
		 * just recompute systemRect .tops and .bottoms, preserving their heights.
		 */
		if (fixFirstSys) FixFirstSysRectY(doc, titleMargin);

		if (didChangeSBreaks)
			FixMeasRectYs(doc, NILINK, TRUE, FALSE, TRUE);
		else
			FixSystemRectYs(doc, FALSE);

		if (exactMeasPerSys)
			RJustifySystems(doc, RightLINK(beforeL), endSysL, justify);
		else if (justify)
			RJustifySystems(doc, RightLINK(beforeL), endSysL, TRUE);

		doc->selStartL = doc->headL;
		doc->selEndL = doc->tailL;
		DeselAll(doc);
		doc->selStartL = doc->selEndL = /*endSysL*/ doc->tailL;
		MEAdjustCaret(doc, FALSE);
		
		InvalRange(RightLINK(beforeL), endSysL);
		InvalWindow(doc);
		
		doc->changed = TRUE;
	}

	return (nothingToDo? NOTHING_TO_DO : OP_COMPLETE);
}
