/* DelAddRedTimeSigs.c for Nightingale - delete redundant time signatures; add "redundant"
(really _cautionary_) time signatures. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static DDIST GetNormalStaffLength(Document *doc, LINK pL);
static void FixMeasureRect(Document *doc, LINK measL);
static void FixObjMeasureRect(Document *doc, LINK pL);
Boolean SameTimeSigOnAllStaves(Document *doc, LINK timeSigL);


/* Get the "normal" length of the Staff <pL> is in. FIXME: Cf. DSUtils.c:StaffLength(),
which gets the actual length--shouldn't we use that and forget this? */

static DDIST GetNormalStaffLength(Document *doc, LINK pL)
{
	LINK systemL;  DDIST staffLength;
	
	systemL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	if (!LinkLSYS(systemL))
		staffLength = MARGWIDTH(doc)-doc->firstIndent;
	else
		staffLength = MARGWIDTH(doc)-doc->otherIndent;
	
	return staffLength;
}

/* Set the measureRect of the given object's Measure so it extends the actual width
of the measure. FIXME: Cf. FixDelMeasures: THIS HAS IMPROVED CALC. OF SYSTEM WIDTH! */

static void FixMeasureRect(Document *doc, LINK measL)
{
	PAMEASURE	aMeasure;
	LINK		aMeasureL, systemL;
	DDIST		measWidth;
	Boolean 	endOfSystem;
	
	if (!MeasureTYPE(measL)) 
		MayErrMsg("FixMeasureRect: argument %ld not a Measure", (long)measL);

	endOfSystem = LastMeasInSys(measL);
	systemL = LSSearch(measL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);

	/* If measL is the last Measure of the system or the score, get the width
		of its system, and set the measureRect.right of measL to that minus measL's
		xd, i.e., reset the measureRect of measL based on the system width and
		measL's xd. */

	if (!LinkRMEAS(measL) || endOfSystem) {
		/* As in Score.c:MakeMeasure. This is almost certainly the right idea. */
		measWidth = GetNormalStaffLength(doc, measL) - LinkXD(measL);
		aMeasureL = FirstSubLINK(measL);
		for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
			aMeasure = GetPAMEASURE(aMeasureL);
			aMeasure->measSizeRect.right = measWidth;
		}

		return;
	}

	/* measL is not the last Measure of the system or the score: set its 
		measSizeRect.right to the xd of its rMeasure minus its own xd. */

	aMeasureL = FirstSubLINK(measL);
	for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
		aMeasure = GetPAMEASURE(aMeasureL);
		aMeasure->measSizeRect.right = LinkXD(LinkRMEAS(measL))-LinkXD(measL);
	}

	/* Then reset the measSizeRect of the last Measure in the system the same as above. */

	if (LinkRSYS(systemL))
		measL = LSSearch(LinkRSYS(systemL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
	else
		measL = LSSearch(doc->tailL, MEASUREtype, ANYONE, GO_LEFT, FALSE);

	LinkVALID(measL) = FALSE;								/* So Draw updates measureBBox */

	/* As in Score.c:MakeMeasure(). This is almost certainly the right idea. */
	measWidth = GetNormalStaffLength(doc, measL) - LinkXD(measL);
	aMeasureL = FirstSubLINK(measL);
	for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
		aMeasure = GetPAMEASURE(aMeasureL);
		aMeasure->measSizeRect.right = measWidth;
	}
}

static void FixObjMeasureRect(Document *doc, LINK pL)
{
	LINK measL;
	
	measL = SSearch(pL, MEASUREtype, GO_LEFT);
	if (!measL) return;									/* <pL> must be before the 1st Measure */
	FixMeasureRect(doc, measL);
}


Boolean SameTimeSigOnAllStaves(Document *doc, LINK timeSigL)
{
	LINK aTimeSigL;  PATIMESIG aTimeSig;
	short subType, numerator, denominator;
	
	if (LinkNENTRIES(timeSigL)!=doc->nstaves) return FALSE;
	
	aTimeSigL = FirstSubLINK(timeSigL);
	aTimeSig = GetPATIMESIG(aTimeSigL);		
	subType = aTimeSig->subType;
	numerator = aTimeSig->numerator;
	denominator = aTimeSig->denominator;
	for (aTimeSigL = NextTIMESIGL(aTimeSigL); aTimeSigL; 
			aTimeSigL = NextTIMESIGL(aTimeSigL)) {
		aTimeSig = GetPATIMESIG(aTimeSigL);		
		if (aTimeSig->subType!=subType) return FALSE;
		if (aTimeSig->numerator!=numerator) return FALSE;
		if (aTimeSig->denominator!=denominator) return FALSE;
	}
	
	return TRUE;
}


/* Check if in time-signature object <timeSigL> the time signature isn't the same on all
staves. NB: That's legal in Nightingale as long as barlines always align; this function
is for use in situations where we simply don't want to handle the complexity. */

static Boolean WarnTimeSigsInconsistency(Document *doc, LINK timeSigL);
static Boolean WarnTimeSigsInconsistency(Document *doc, LINK timeSigL)
{
	short measNum;
	char fmtStr[256];
	
	if (!SameTimeSigOnAllStaves(doc, timeSigL)) {
		measNum = GetMeasNum(doc, timeSigL);
		GetIndCString(fmtStr, MISCERRS_STRS, 26);		/* "In measure %d, the time signature isn't identical on all..." */
		sprintf(strBuf, fmtStr, measNum);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return TRUE;
	}
	
	return FALSE;
}


/* ----------------------------------------------------------------------------------- */

/* Delete redundant time signatures. "Redundant" is defined as "another instance of
the same numerator/denominator with no Measures or Syncs in between". (Recall that in
Nightingale every new System starts a new Measure.) Since it's affected by system breaks,
this should be called after any Reformatting that might change system breaks. Returns
FALSE if it finds a problem, else TRUE. */

Boolean DelRedTimeSigs(
			Document *doc,
			Boolean respace,					/* TRUE=remove the space deleted timesigs occupied */
			LINK *pFirstDelL, LINK *pLastDelL	/* Output: first and last timesigs deleted */
		)
{
	LINK pL, aTimeSigL, prevTSL, firstDelL, lastDelL;
	PATIMESIG aTimeSig;
	char prevNumerator, prevDenominator;
	Boolean okay=TRUE;

	prevTSL = firstDelL = lastDelL = NILINK;
	
	pL = SSearch(doc->headL, MEASUREtype, GO_RIGHT);
	for ( ; pL!=doc->tailL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case TIMESIGtype:
					/* If the time signature isn't identical on all staves, things are
					   more complex than we want to bother with: warn user and give up. */
					if (WarnTimeSigsInconsistency(doc, pL)) {
						okay = FALSE;
						goto Finish;
					}
					aTimeSigL = FirstSubLINK(pL);
					if (prevTSL) {
						/*
						 * Found another time sig. in the same Measure with no intervening
						 * Syncs. If this time sig. is the same as the previous one, we need
						 * to delete either and remove the space it occupied. (Of course, we
						 * don't have to update context because it's redundant.) We remove the
						 * previous one instead of this one because, when there are two time
						 * signatures in a row, the second will always have the correct amount
						 * of space for a time signature there; the first may not.
						 */
						aTimeSig = GetPATIMESIG(aTimeSigL);
						if (aTimeSig->numerator==prevNumerator
						&& aTimeSig->denominator==prevDenominator) {
							if (respace) {
								RemoveObjSpace(doc, prevTSL);
								FixObjMeasureRect(doc, prevTSL);
							}
							if (!firstDelL) firstDelL = LeftLINK(prevTSL);
							lastDelL = RightLINK(prevTSL);
							DeleteNode(doc, prevTSL);
						}
					}
					aTimeSig = GetPATIMESIG(aTimeSigL);
					prevNumerator = aTimeSig->numerator;
					prevDenominator = aTimeSig->denominator;
					prevTSL = pL;
				break;
			case SYNCtype:
			case MEASUREtype:
				prevTSL = NILINK;
				break;
			default:
				;
		}
	}

Finish:
	*pFirstDelL = firstDelL;
	*pLastDelL = lastDelL;
	return okay;
}


/* ----------------------------------------------------------------------------------- */

/* Add "cautionary" time signatures: if any system begins with a time-signature change,
if the previous system ends with a barline, add the same time signature after the
barline on all staves. NB: We assume that every time-signature change is the same on
all staves! Nightingale requires all barlines to extend across all staves -- i.e.,
Measure objects must have subobjects on every Staff); that makes our assumption safer,
but not really safe; it'd be better to check here.  */

short AddCautionaryTimeSigs(Document *doc)
{
	LINK pL, systemL, endPrevSysL, timeSigL, aTimeSigL;
	short systemNum, type, numer, denom, numAdded;
	Boolean beforeFirstNRGR, firstChange;
	TSINFO timeSigInfo;

	/* Go thru the entire object list. When we encounter a time signature, if it's
	   before any Syncs or GRSyncs (i.e., notes, rests, or grace notes) in its
	   System and the last object of the previous System is a Measure (graphically, a
	   barline), add the same time signature at the end of the previous system. Note
	   that we can't just look for a time signature that's not <inMeas>, i.e., before
	   the first Measure of a System, since time sig. changes are _within_ the first
	   Measure. */
	numAdded = 0;
	systemNum = 0;
	beforeFirstNRGR = TRUE;
	firstChange = TRUE;
	for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYSTEMtype:
				systemNum++;
				systemL = pL;
				beforeFirstNRGR = TRUE;
				break;
			case SYNCtype:
			case GRSYNCtype:
				beforeFirstNRGR = FALSE;
				break;
			case TIMESIGtype:
				if (systemNum>1 && beforeFirstNRGR) {				
					/* If the time signature isn't identical on all staves, things are
					   more complex than we want to bother with: warn user and give up. */
					if (WarnTimeSigsInconsistency(doc, pL)) return numAdded;
					
					/* Consider adding the same timesig at the end of the previous System.
						If the previous System ended with a Measure, it must be the object
						just before the current System or -- if that object is a Page --
						the object just before that. */
					endPrevSysL = LeftLINK(systemL);
					if (PageTYPE(endPrevSysL)) endPrevSysL = LeftLINK(endPrevSysL);
					if (MeasureTYPE(endPrevSysL)) {
						if (firstChange) PrepareUndo(doc, endPrevSysL,
											U_AddCautionaryTS, 50);    /* "Add Cautionary TimeSigs" */
						firstChange = FALSE;
						aTimeSigL = FirstSubLINK(pL);
						type = TimeSigType(aTimeSigL);
						numer = TimeSigNUMER(aTimeSigL);
						denom = TimeSigDENOM(aTimeSigL);
						//LogPrintf(LOG_DEBUG, "AddCautionaryTimeSigs: adding TS type %d, %d/%d before measure %d\n",
						//				type, numer, denom, GetMeasNum(doc, pL));						
						/* Insert the cautionary time signature on all staves. */
						timeSigL = FIInsertTimeSig(doc, ANYONE, RightLINK(endPrevSysL),
													type, numer, denom);
						if (timeSigL==NILINK) {
							AlwaysErrMsg("AddCautionaryTimeSigs: can't add time signature.");
							return numAdded;
						}
						timeSigInfo.TSType = type;
						timeSigInfo.numerator = numer;
						timeSigInfo.denominator = denom;
						for (int s=1; s<=doc->nstaves; s++)
							FixContextForTimeSig(doc, RightLINK(timeSigL), s, timeSigInfo);
						RespaceBars(doc, timeSigL, timeSigL, 0, FALSE, FALSE);
						numAdded++;
					}
				}
			default:
				;
			}
		}
		
	return numAdded;
}
