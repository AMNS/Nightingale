/* DelRedTimeSigs.c for Nightingale - delete redundant time signatures. */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1996-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

DDIST GetNormalStaffLength(Document *doc, LINK pL);
void FixMeasureRect(Document *doc, LINK measL);
void FixObjMeasureRect(Document *doc, LINK pL);
Boolean SameTimeSigOnAllStaves(Document *doc, LINK timeSigL);


/* Get the "normal" length of the Staff <pL> is in. ??Cf. DSUtils.c:StaffLength,
which gets the actual length--shouldn't we use that and forget this? */

DDIST GetNormalStaffLength(Document *doc, LINK pL)
{
	LINK systemL; DDIST staffLength;
	
	systemL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	if (!LinkLSYS(systemL))
		staffLength = MARGWIDTH(doc)-doc->firstIndent;
	else
		staffLength = MARGWIDTH(doc)-doc->otherIndent;
	
	return staffLength;
}

/* Set the measureRect of the given object's Measure so it extends the actual width
of the measure. ??CF. FixDelMeasures: THIS HAS IMPROVED CALC. OF SYSTEM WIDTH! */

void FixMeasureRect(Document *doc, LINK measL)
{
	PAMEASURE	aMeasure;
	LINK			aMeasureL, systemL;
	DDIST			measWidth;
	Boolean 		endOfSystem;
	
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
			aMeasure->measureRect.right = measWidth;
		}

		return;
	}

	/* measL is not the last Measure of the system or the score: set its
		measureRect.right to the xd of its rMeasure minus its own xd. */

	aMeasureL = FirstSubLINK(measL);
	for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
		aMeasure = GetPAMEASURE(aMeasureL);
		aMeasure->measureRect.right = LinkXD(LinkRMEAS(measL))-LinkXD(measL);
	}

	/* Then reset the measureRect of the last Measure in the system the same
		as above. */

	if (LinkRSYS(systemL))
		measL = LSSearch(LinkRSYS(systemL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
	else
		measL = LSSearch(doc->tailL, MEASUREtype, ANYONE, GO_LEFT, FALSE);

	LinkVALID(measL) = FALSE;														/* So Draw updates measureBBox */

	/* As in Score.c:MakeMeasure. This is almost certainly the right idea. */
	measWidth = GetNormalStaffLength(doc, measL) - LinkXD(measL);
	aMeasureL = FirstSubLINK(measL);
	for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
		aMeasure = GetPAMEASURE(aMeasureL);
		aMeasure->measureRect.right = measWidth;
	}
}

void FixObjMeasureRect(Document *doc, LINK pL)
{
	LINK measL;
	
	measL = SSearch(pL, MEASUREtype, GO_LEFT);
	if (!measL) return;									/* <pL> must be before the 1st Measure */
	FixMeasureRect(doc, measL);
}


Boolean SameTimeSigOnAllStaves(Document *doc, LINK timeSigL)
{
	LINK aTimeSigL; PATIMESIG aTimeSig;
	INT16 subType, numerator, denominator;
	
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

/* Delete redundant time signatures. "Redundant" is defined as "another instance of
the same numerator/denominator with no Measures or Syncs in between"  (recall that
in Nightingale every new System starts a new Measure) . Since it's affected by
system breaks, this should be called after any Reformatting that might change system
breaks. Returns FALSE if it finds a problem, else TRUE.

NB: DCheckRedundTS checks for "redundancy" in a very different sense. */

Boolean DelRedTimeSigs(
			Document *doc,
			Boolean respace,					/* TRUE=remove the space deleted timesigs occupied */
			LINK *pFirstDelL, LINK *pLastDelL	/* Output: first and last timesigs deleted */
		)
{
	LINK pL, aTimeSigL, prevTSL, firstDelL, lastDelL;
	PATIMESIG aTimeSig;
	char prevNumerator, prevDenominator;
	INT16 measNum;
	char fmtStr[256];
	Boolean okay=TRUE;

	prevTSL = firstDelL = lastDelL = NILINK;
	
	pL = SSearch(doc->headL, MEASUREtype, GO_RIGHT);
	for ( ; pL!=doc->tailL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case TIMESIGtype:
					/*
					 * If the time signature isn't identical on all staves, things are
					 * more complex than we want to bother handling: just give up.
					 */
					if (!SameTimeSigOnAllStaves(doc, pL)) {
						measNum = GetMeasNum(doc, pL);
						GetIndCString(fmtStr, MISCERRS_STRS, 26);		/* "In measure %d, the time signature isn't identical on all..." */
						sprintf(strBuf, fmtStr, measNum);
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						okay = FALSE;
						goto Finish;
					}
					aTimeSigL = FirstSubLINK(pL);
					if (prevTSL) {
						/*
						 * Found another time sig. in the same Measure, with no intervening
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
