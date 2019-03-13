/***************************************************************************
*	FILE:	Clear.c
*	PROJ:	Nightingale
*	DESC:	Routines for Clear function (Clear command and Cut command)
/***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void RespaceAfterDelete(Document *doc, LINK prevSelL);
static Boolean IsBetween(LINK node,LINK firstL,LINK lastL);
static void InvalObjRanges(Document *doc,LINK startL,LINK endL,Boolean after);
static void ExtendInvalRange(Document *doc);
static void InvalBefFirst(Document *doc,LINK startL);
static void InvalForClear(Document *doc,LINK prevSelL,Boolean hasClef,Boolean hasKS,
							Boolean hasTS,Boolean befFirst,Boolean hasPgRelGraphic);


void SelRangeCases(Document *doc, Boolean *hasClef, Boolean *hasKS, Boolean *hasTS,
							Boolean *befFirst, Boolean *hasPRelGraphic)
{
	LINK pL;

	*hasClef = *hasKS = *hasTS = False;
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (ClefTYPE(pL)) *hasClef = True;
		else if (KeySigTYPE(pL)) *hasKS = True;
		else if (TimeSigTYPE(pL)) *hasTS = True;
	}
	*befFirst = BeforeFirstMeas(doc->selStartL);
	if (*befFirst) {
		*hasPRelGraphic = False;
		for (pL = doc->headL; !MeasureTYPE(pL); pL = RightLINK(pL))
			if (LinkSEL(pL) && GraphicTYPE(pL))
				{ *hasPRelGraphic = True; break; }
	}
}


static void RespaceAfterDelete(Document *doc, LINK prevSelL)
{
	long resFact;

	if (doc->autoRespace) {
		resFact = RESFACTOR*(long)doc->spacePercent;
		RespaceBars(doc, prevSelL, doc->selEndL, resFact, False, False);
	}
}


/* Return True if pL is between firstL and lastL (including firstL, excluding lastL). */

static Boolean IsBetween(LINK node, LINK firstL, LINK lastL)
{
	LINK pL;

	if (firstL==lastL)										/* Empty range. */
		return False;
	if (IsAfter(lastL, firstL))								/* Backwards range. */
		return False;
	for (pL=firstL; pL!=lastL; pL=RightLINK(pL))
		if (node==pL) return True;							/* Node in range. */
		
	return False;
}


/* This can add a call to InvalObject (and if needed, a slot for the objType inside
InvalObject) for any object which isn't erased completely when extending outside
the systemRect. after should be True if the ranges are to be invalled for objects
extending beyond the selRange. */

static void InvalObjRanges(Document *doc, LINK startL, LINK endL, Boolean after)
{
	LINK pL, firstL, lastL;

	for (pL=startL; pL!=endL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SLURtype:
				if (after || IsBetween(SlurLASTSYNC(pL),doc->selStartL,doc->selEndL))
					InvalObject(doc,pL,False);
				break;
			case BEAMSETtype:
				if (after || IsBetween(LastInBeam(pL),doc->selStartL,doc->selEndL))
					InvalMeasures(FirstInBeam(pL),LastInBeam(pL),ANYONE);
				break;
			case TUPLETtype:
				if (after || IsBetween(LastInTuplet(pL),doc->selStartL,doc->selEndL))
					InvalMeasures(FirstInTuplet(pL),LastInTuplet(pL),ANYONE);
				break;
			case OTTAVAtype:
				if (after || IsBetween(LastInOttava(pL),doc->selStartL,doc->selEndL))
					InvalMeasures(FirstInOttava(pL),LastInOttava(pL),ANYONE);
				break;
			case DYNAMtype:
				if (IsHairpin(pL)) {
					if (IsBetween(DynamLASTSYNC(pL),doc->selStartL,doc->selEndL))
						InvalMeasures(DynamFIRSTSYNC(pL),DynamLASTSYNC(pL),ANYONE);
				}
				else if (after)
					InvalMeasure(DynamFIRSTSYNC(pL),ANYONE);
				break;
			case GRAPHICtype:
				if (after)
					InvalObject(doc,pL,False);
				break;
			case TEMPOtype:	
				if (after)
					InvalObject(doc,pL,False);
				break;
			case ENDINGtype:
				firstL = EndingFIRSTOBJ(pL);
				lastL = EndingLASTOBJ(pL);

				if (after || IsBetween(lastL,doc->selStartL,doc->selEndL))
					InvalMeasures(firstL,lastL,ANYONE);
				break;
			default:
				break;
		}
	}
}


/* Extend the range to be invalled to include the measures to be deleted and the
extent before and after the range covering all extended objects extending into and
out of that range. */

static void ExtendInvalRange(Document *doc)
{
	LINK sysL;
	
	/* Inval the range to be deleted. */

	InvalMeasures(doc->selStartL,doc->selEndL,ANYONE);

	sysL = EitherSearch(doc->selStartL,SYSTEMtype,ANYONE,GO_LEFT,False);

	/* Inval ranges for all objects extending into the range to be deleted. */
	InvalObjRanges(doc,sysL,doc->selStartL,False);

	/* Inval ranges for all objects extending beyond the range to be deleted. */
	InvalObjRanges(doc,doc->selStartL,doc->selEndL,True);
}


/* Erase and Inval a slice at the vertical level of the first system and
extending from the left margin to the end of the first measure. */

static void InvalBefFirst(Document *doc, LINK startL)
{
	LINK sysL,measL;
	DRect sysRect;
	Rect r; 
	
	sysL = EitherSearch(startL,SYSTEMtype,ANYONE,GO_LEFT,False);
	measL = LSSearch(startL,MEASUREtype,ANYONE,GO_RIGHT,False);

	sysRect = SystemRECT(sysL);
	sysRect.right = sysRect.left + LinkXD(measL);
	sysRect.left = 0;

	D2Rect(&sysRect,&r);
	OffsetRect(&r,doc->currentPaper.left,doc->currentPaper.top);
	EraseAndInval(&r);
}


/* Handle respacing and invalidating after cut and clear operations. The Boolean
parameters refer to various special cases: a clef, key sig., or time sig.
in the deleted range, the range before the first measure, or nothing but
J_D type symbols in the range. */

static void InvalForClear(Document *doc, LINK prevSelL, Boolean hasClef, Boolean hasKS,
								Boolean hasTS, Boolean befFirst, Boolean hasPgRelGraphic)
{
	LINK selStartL;

	/* Handle general case: all measures from the start of the range deleted to
		doc->selEndL. This has to be done before the clear, and so probably
		doesn't need to be done again here. */

	selStartL = RightLINK(prevSelL);
	
	/* Handle redrawing for special cases. Changes in clefs and keySigs can propagate
		to the end of the score, so need to redraw and inval from here to the end of
		the window; for now, redraw the entire window. */
	 
	if (hasClef || hasKS || (hasTS && doc->showDurProb)) {
		InvalWindow(doc);
		if (hasClef || hasKS) InvalRange(selStartL, doc->tailL);
	}
	else if (befFirst) {
		if (hasPgRelGraphic)
			InvalWindow(doc);
		else
			InvalBefFirst(doc,selStartL);
	}
}


/* Return True if we actually Clear anything, False if not. */

Boolean Clear(Document *doc)
{
	LINK prevSelL, pL;
	Boolean hasClef,hasKS,hasTS,befFirst,hasPgRelGraphic, dontResp;		/* for special cases */
	Boolean delMeas, anyStillSel, deselAnyMeas, didClear=False;

   MEHideCaret(doc);

	/* If any Measure objects are only partly selected, we have to do something so
		we don't end up with a data structure with Measure objs with subobjs on only
		some staves, since that is illegal. Anyway, the user probably doesn't really
		want to delete such Measures, so if there are any, ask user whether to delete
		such Measures on all staves or leave them completely untouched. */
		
	if (!AllMeasHomoSel(doc)) {
		delMeas = (CautionAdvise(CLEAR_BARS_ALRT)==OK);
		if (delMeas) SelPartlySelMeasSubobjs(doc);
		else {
			anyStillSel = False;
			deselAnyMeas = DeselPartlySelMeasSubobjs(doc);
			if (deselAnyMeas) {
				for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
					if (LinkSEL(pL)) { anyStillSel = True; break; }
				if (!anyStillSel) {
					doc->selEndL = doc->selStartL;
					goto Cleanup;
				}
			}
		}
	}
	
	WaitCursor();
	SelRangeCases(doc,&hasClef,&hasKS,&hasTS,&befFirst,&hasPgRelGraphic);
	if (!befFirst) ExtendInvalRange(doc);

	InitAntikink(doc, doc->selStartL, doc->selEndL);

	/* Find the rightmost LINK before the selection that certainly won't be deleted. */

	prevSelL = FirstValidxd(LeftLINK(doc->selStartL), True);
	DeleteSelection(doc, True, &dontResp);							/* True to only hide init clef,keysig,timesig */
	didClear = True;

	if (MeasureTYPE(prevSelL))
		prevSelL = RightLINK(prevSelL);

 	if (!dontResp)
 		RespaceAfterDelete(doc, prevSelL);
	Antikink();

 	InvalForClear(doc, prevSelL, hasClef, hasKS, hasTS, befFirst, hasPgRelGraphic);
Cleanup:
	MEAdjustCaret(doc, True);
	return didClear;
}


/* Remove the selection from the score without affecting the clipboard. */

void DoClear(Document *doc)
{
	PrepareUndo(doc, doc->selStartL, U_Clear, 5);				/* "Undo Clear" */

	Clear(doc);
}
