/******************************************************************************************
*	FILE:	Delete.c
*	PROJ:	Nightingale
*	DESC:	Deletion-related routines.
		DSRemoveGraphic			DSRemoveTempo			DSRemoveEnding
		DeleteOtherDynamic		FixDynamicContext
		DSRemoveDynamic			DeleteMeasure			DeleteWhole
		DeleteTie				DeleteSlur				FixDelSlurs		
		DeleteOtherSlur			FixSyncForSlur
		DeleteXSysSlur			FixDelCrossSysObjs		FixDelBeams
		PostFixDelBeams			FixDelMeasures
		PrevTiedNote			FirstTiedNote			FixDelAccidentals	
		DelTupletForSync		DelOttavaForSync		DelModsForSync
		FixDelContext			ExtendJDRange
		DelClefBefFirst			DelKeySigBefFirst		DelTimeSigBefFirst
		FixDelChords			FixDelGRChords			FixAccsForNoTie
		DelSelPrepare			DelSelCleanup
		DeleteSelection			DelNoteRedAcc			DelGRNoteRedAcc
		ArrangeSyncAccs			DelRedundantAccs
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Local variables and functions */

static void DSRemoveGraphic(Document *, LINK);
static void DSRemoveTempo(Document *, LINK);
static void DSRemoveEnding(Document *, LINK);
static void DSObjDelGraphic(Document *, LINK, LINK);
static void DSObjRemGraphic(Document *, LINK);
static void DSObjDelTempo(Document *, LINK, LINK);
static void DSObjRemTempo(Document *, LINK);
static void DSObjDelEnding(Document *, LINK, LINK);
static void DSObjRemEnding(Document *, LINK);
static void FixDynamicContext(Document *, LINK);
static void DeleteOtherDynamic(Document *, LINK);
static void DSRemoveDynamic(Document *, LINK, short);

static Boolean DeleteWhole(Document *, LINK);

static void DeleteSlur(Document *, LINK, LINK, Boolean, Boolean);
static void FixDelSlurs(Document *);
static void DeleteOtherSlur(Document *, Boolean, Boolean, LINK);

static LINK FindTheBeam(LINK, short);
static LINK FindTheGRBeam(LINK, short);
static void FixDelBeams(Document *);
static void PostFixDelBeams(Document *);
static void FixDelGRBeams(Document *);

static void ClearInTupletFlags(LINK tupletL);
static void DelTupletForSync(Document *, LINK, LINK);
static void DelOttavaForSync(Document *, LINK, LINK);

static void FixDelMeasures(Document *, LINK);
static void FixDelAccidentals(Document *, short, LINK, LINK);
static void FixDelContext(Document *);
static void ExtendJDRange(Document *doc);

static DDIST GetNewKSWidth(Document *,LINK,short);
static void FixStfBeforeFirst(Document *,LINK,LINK);
 
static Boolean InvisifyBFClef(Document *,LINK);
static void RemoveBFClef(Document *,LINK);
static Boolean DelClefBefFirst(Document *, LINK, LINK *);

static Boolean InvisifyBFKeySig(Document *,LINK);
static void RemoveBFKeySig(Document *,LINK);
static Boolean DelKeySigBefFirst(Document *, LINK);

static Boolean InvisifyBFTimeSig(Document *,LINK);
static void RemoveBFTimeSig(Document *,LINK, DDIST);
static Boolean DelTimeSigBefFirst(Document *, LINK);

static void FixDelChords(Document *, LINK, Boolean []);
static void FixDelGRChords(Document *, LINK, Boolean []);

static Boolean DelSelPrepare(Document *, LINK *, LINK *, LINK *, Boolean *, short *, Boolean *);
static void DelSelCleanup(Document *, LINK, LINK, LINK, LINK, Boolean, Boolean, short, Boolean);


static LINK	beamStartL[MAXVOICES+1], beamEndL[MAXVOICES+1];
static LINK cntxtDoneL[MAXSTAVES+1];


/* ======================================= Routines for deleting Graphics and Dynamics == */

/* ------------------------------------------------------------------- DSRemoveGraphic -- */
/* DSRemoveGraphic deletes any Graphic which is attached to <pL>. */

static void DSRemoveGraphic(Document *doc, LINK pL)
{
	LINK graphicL, objL;

	objL = pL;
	do {
		graphicL = LSSearch(objL, GRAPHICtype, ANYONE, GO_LEFT, FALSE);
		if (graphicL) {
			objL = LeftLINK(graphicL);
			if (GraphicFIRSTOBJ(graphicL)==pL) DeleteNode(doc, graphicL);
			else if (GraphicSubType(graphicL)==GRDraw)			
				if (GraphicLASTOBJ(graphicL)==pL) DeleteNode(doc, graphicL);
		}
	} while (graphicL && objL);
}

/* ---------------------------------------------------------------------- DSRemoveTempo -- *
/* DSRemoveTempo deletes any Tempo object which is attached to <pL>. */

static void DSRemoveTempo(Document *doc, LINK pL)
{
	LINK tempoL, objL;
	PTEMPO pTempo;

	objL = pL;
	do {
		tempoL = LSSearch(objL, TEMPOtype, ANYONE, GO_LEFT, FALSE);
		if (tempoL) {
			objL = LeftLINK(tempoL);
			pTempo = GetPTEMPO(tempoL);
			if (pTempo->firstObjL==pL) DeleteNode(doc, tempoL);
		}
	} while (tempoL && objL);
}

/* -------------------------------------------------------------------- DSRemoveEnding -- */
/* DSRemoveEnding deletes any Ending object which is attached to <pL>. */

static void DSRemoveEnding(Document *doc, LINK pL)
{
	LINK endingL, objL;
	PENDING pEnding;

	objL = pL;
	do {
		endingL = LSSearch(objL, ENDINGtype, ANYONE, GO_LEFT, FALSE);
		if (endingL) {
			objL = LeftLINK(endingL);
			pEnding = GetPENDING(endingL);
			if (pEnding->firstObjL==pL || pEnding->lastObjL==pL)
				DeleteNode(doc, endingL);
		}
	} while (endingL && objL);
}

/* ------------------------------------------------------------------- DSObjDelGraphic -- */
/* Delete graphicL when pL (its firstObj) no longer has any subObjs on graphicL's
staff or in its voice. */

static void DSObjDelGraphic(Document *doc, LINK pL, LINK graphicL)
{
	switch (ObjLType(pL)) {
		case MEASUREtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
			if (!ObjOnStaff(pL, GraphicSTAFF(graphicL), FALSE))
				DeleteNode(doc,graphicL);
			break;
		case SYNCtype:
			if (!SyncInVoice(pL, GraphicVOICE(graphicL)))
				DeleteNode(doc,graphicL);
			break;
		case GRSYNCtype:
			if (!GRSyncInVoice(pL, GraphicVOICE(graphicL)))
				DeleteNode(doc,graphicL);
			break;
	}
}

/* ------------------------------------------------------------------- DSObjRemGraphic -- */
/* DSObjRemGraphic deletes any Graphic which is attached to <pL>, when all of
pL's subObjs on the Graphic's staff or in its voice have been deleted. */

static void DSObjRemGraphic(Document *doc, LINK pL)
{
	LINK graphicL, objL;

	objL = pL;
	do {
		graphicL = LSSearch(objL, GRAPHICtype, ANYONE, GO_LEFT, FALSE);
		if (graphicL) {
			objL = LeftLINK(graphicL);
			if (GraphicFIRSTOBJ(graphicL)==pL) DSObjDelGraphic(doc,pL,graphicL);
			else if (GraphicSubType(graphicL)==GRDraw)			
				if (GraphicLASTOBJ(graphicL)==pL) DSObjDelGraphic(doc,pL,graphicL);
		}
	} while (graphicL && objL);
}


/* --------------------------------------------------------------------- DSObjDelTempo -- */
/* Delete tempoL when pL (its firstObj) no longer has any subObjs on tempoL's
staff. */

static void DSObjDelTempo(Document *doc, LINK pL, LINK tempoL)
{
	switch (ObjLType(pL)) {
		case MEASUREtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case SYNCtype:
			if (!ObjOnStaff(pL, TempoSTAFF(tempoL), FALSE))
				DeleteNode(doc,tempoL);
			break;
	}
}

/* --------------------------------------------------------------------- DSObjRemTempo -- */
/* DSObjDelTempo deletes any Tempo which is attached to <pL>, when all of
pL's subObjs on the Tempo's staff have been deleted. */

static void DSObjRemTempo(Document *doc, LINK pL)
{
	LINK tempoL, objL;
	PTEMPO pTempo;

	objL = pL;
	do {
		tempoL = LSSearch(objL, TEMPOtype, ANYONE, GO_LEFT, FALSE);
		if (tempoL) {
			objL = LeftLINK(tempoL);
			pTempo = GetPTEMPO(tempoL);
			if (pTempo->firstObjL==pL) {
				DSObjDelTempo(doc,pL,tempoL);
			}
		}
	} while (tempoL && objL);
}

/* -------------------------------------------------------------------- DSObjDelEnding -- */
/* Delete endingL when its firstObj <pL> no longer has any subObjs on endingL's staff. */

static void DSObjDelEnding(Document *doc, LINK pL, LINK endingL)
{
	switch (ObjLType(pL)) {
		case MEASUREtype:
		case SYNCtype:
		case PSMEAStype:
			if (!ObjOnStaff(pL, EndingSTAFF(endingL), FALSE))
				DeleteNode(doc,endingL);
			break;
	}
}

/* -------------------------------------------------------------------- DSObjRemEnding -- */
/* DSObjDelEnding deletes any Ending which is attached to <pL>, when all of pL's subObjs
on the Ending's staff have been deleted. */

static void DSObjRemEnding(Document *doc, LINK pL)
{
	LINK endingL, objL;
	PENDING pEnding;

	objL = pL;
	do {
		endingL = LSSearch(objL, ENDINGtype, ANYONE, GO_LEFT, FALSE);
		if (endingL) {
			objL = LeftLINK(endingL);
			pEnding = GetPENDING(endingL);
			if (pEnding->firstObjL==pL || pEnding->lastObjL==pL) {
				DSObjDelEnding(doc,pL,endingL);
			}
		}
	} while (endingL && objL);
}


/* ----------------------------------------------------------------- FixDynamicContext -- */
/* Fix up the context for the dynamic <pL> */

static void FixDynamicContext(Document *doc, LINK pL)
{
	LINK aDynamicL;  short staff;
	CONTEXT oldContext, newContext;

	aDynamicL = FirstSubLINK(pL);
	staff = DynamicSTAFF(aDynamicL);
	
	GetContext(doc, pL, staff, &oldContext);
	GetContext(doc, LeftLINK(pL), staff, &newContext);
	FixContextForDynamic(doc, RightLINK(pL), staff, oldContext.dynamicType, 
		newContext.dynamicType);
}


/* ---------------------------------------------------------------- DeleteOtherDynamic -- */
/* DeleteOtherDynamic deletes any cross system dynamic which is paired to <pL>. It
traverses in the direction of the other piece until it finds it; if it finds a
mismatched piece first, it sets dynamError TRUE and stops the traversal. */

static void DeleteOtherDynamic(Document *doc, LINK pL)
{
	LINK aDynamicL,qL,firstSync,lastSync;
	PDYNAMIC p,q;
	short dynamStaff;
	Boolean isHairpin,crossSys,foundDynam=FALSE,haveDynam=TRUE,dynamError=FALSE;

	isHairpin = IsHairpin(pL);
	aDynamicL = FirstSubLINK(pL);
	dynamStaff = DynamicSTAFF(aDynamicL);
	firstSync = DynamFIRSTSYNC(pL);
	lastSync = DynamLASTSYNC(pL);
	p = GetPDYNAMIC(pL);
	crossSys = p->crossSys;

	/* Delete the other piece of the cross system dynamic. Assumes each piece
		of cross system dynamic is paired to one and only one corresponding
		piece. */
	if (isHairpin && crossSys) {
		if (SystemTYPE(lastSync))
			while (!foundDynam && haveDynam) {			/* Search while there are still dynamics & haven't found the right one */
				qL = LSSearch(RightLINK(pL), DYNAMtype, dynamStaff, FALSE, FALSE);
				if (qL) {
					haveDynam = TRUE;
					if (IsHairpin(qL)) {
						q = GetPDYNAMIC(qL);
						if (q->crossSys && MeasureTYPE(DynamFIRSTSYNC(qL))) {
							FixDynamicContext(doc, qL);	/* Found the right piece */
							DeleteNode(doc, qL);
							foundDynam = TRUE;
						}
						else if (q->crossSys && SystemTYPE(DynamLASTSYNC(qL))) {
							dynamError = TRUE;				/* Got to another xSystem dynam first */
							foundDynam = TRUE;
						}
					}
				}
				else {
					haveDynam = FALSE;						/* No corresponding piece */
					dynamError = TRUE;
				}
			}
		else if (MeasureTYPE(firstSync))
			while (!foundDynam && haveDynam) {				/* cf supra */
				qL = LSSearch(LeftLINK(pL), DYNAMtype, dynamStaff, TRUE, FALSE);
				if (qL) {
					haveDynam = TRUE;
					if (IsHairpin(qL)) {
						q = GetPDYNAMIC(qL);
						if (q->crossSys && SystemTYPE(DynamLASTSYNC(qL))) {
							FixDynamicContext(doc, qL);
							DeleteNode(doc, qL);
							foundDynam = TRUE;
						}
						else if (q->crossSys && MeasureTYPE(DynamFIRSTSYNC(qL))) {
							dynamError = TRUE;
							foundDynam = TRUE;
						}
					}
				}
				else {
					haveDynam = FALSE;
					dynamError = TRUE;
				}
			}
	}
	if (isHairpin && crossSys)
		InvalMeasures(firstSync,lastSync,dynamStaff);
}


/* ------------------------------------------------------------------- DSRemoveDynamic -- */
/* Called when a note or sync is deleted; deletes the dynamic attached to the sync
if the last note (of a possible chord) in the sync on the staff of the dynamic is
deleted.

We need a determinate test for pairing of cross system dynamics; until then, it won't
be easy to decide how to remove cross system pairs in this function.
*/

static void DSRemoveDynamic(Document *doc, LINK pL,
							short staff)	/* staff of dynamic, or ANYONE for DeleteWhole */
{
	LINK dynamicL, aDynamicL, objL, aNoteL, firstSyncL, lastSyncL;
	PDYNAMIC pDynamic;
	Boolean hasNoteOnStf=FALSE;
	
	/* If the entire sync is not being deleted, a note on a staff is being deleted.
	If this is the case, remove the dynamic only if this is the last note in the sync
	on the staff; if notes will remain after deleting this one, dynamics can still be
	attached to the remaining notes and should not be removed. */

	if (staff!=ANYONE) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==staff) {
				if (hasNoteOnStf) return;
				else hasNoteOnStf = TRUE;
			}
	}

	objL = pL;
	do {
		dynamicL = LSSearch(objL, DYNAMtype, ANYONE, TRUE, FALSE);
		if (dynamicL) {
			objL = LeftLINK(dynamicL);
			pDynamic = GetPDYNAMIC(dynamicL);
			firstSyncL = DynamFIRSTSYNC(dynamicL);
			lastSyncL = DynamLASTSYNC(dynamicL);

			/* Check lastSyncL only for hairpins; check firstSyncL for all. */
			if (IsHairpin(dynamicL))
				if (pDynamic->lastSyncL==pL) {
					aDynamicL = FirstSubLINK(dynamicL);
					if (staff==DynamicSTAFF(aDynamicL) || staff==ANYONE) {
						FixDynamicContext(doc, dynamicL);
						DeleteOtherDynamic(doc, dynamicL);
						DeleteNode(doc, dynamicL); 
						InvalMeasures(firstSyncL, lastSyncL, staff);
						continue;
					}
				}
			if (pDynamic->firstSyncL==pL) {
				aDynamicL = FirstSubLINK(dynamicL);
				if (staff==DynamicSTAFF(aDynamicL) || staff==ANYONE) {
					FixDynamicContext(doc, dynamicL);
					DeleteOtherDynamic(doc, dynamicL);
					DeleteNode(doc, dynamicL);
					InvalMeasures(firstSyncL, lastSyncL, staff);
				}
			}
		}
	} while (dynamicL && objL);
}


/* =============================================== Routine for deleting entire objects == */

/* DeleteMeasure deletes the given Measure object and fixes up cross-links. It does NOT
update coords. of objects formerly in the Measure for their new status. */

void DeleteMeasure(Document *doc, register LINK measL)
{
	PMEASURE pMeas, lMeasure, rMeasure;

	pMeas = GetPMEASURE(measL);
	if (pMeas->lMeasure) {
		lMeasure = GetPMEASURE(pMeas->lMeasure);
		lMeasure->rMeasure =
			(pMeas->rMeasure? pMeas->rMeasure : NILINK);
	}
	if (pMeas->rMeasure) {
		rMeasure = GetPMEASURE(pMeas->rMeasure);
		rMeasure->lMeasure =
			(pMeas->lMeasure? pMeas->lMeasure : NILINK);
	}

	DeleteNode(doc, measL);
}


/* ----------------------------------------------------------------------- DeleteWhole -- */
/* DeleteWhole deletes <pL> and makes the object list consistent for the deletion. It:
1. Deletes Graphics, Tempos, and Endings that refer to pL.
2. Updates pointers for the deleted object. 
	a. Updates pGraphic->firstObj pointers for objects of all types. 
	b. Updates tuplets, octave signs, and note modifiers that refer to <pL>.
3. If auto-respacing is on, moves (graphically) all following objects in
	the system to the left by the space occupied by pL.  It does NOT clear
	valid flags of objects it moves.
4. Deletes the entire object pL.
It does not need to update Beamsets that involve <pL>; that should be done by
FixDelBeams ahead of time. It also does not update measures' lTimeStamps nor
Syncs' timeStamps; this must be done later.
*/
 
static Boolean DeleteWhole(Document *doc, register LINK pL)
{
	DDIST 	xMove, xRelMeas;
	PANOTE	aNote;
	LINK	aNoteL, tempL, aKSL;
	
	tempL = LeftLINK(pL);
	DSRemoveGraphic(doc, pL);
	DSRemoveTempo(doc, pL);
	DSRemoveEnding(doc, pL);
			
	if (SyncTYPE(pL)) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->inTuplet) DelTupletForSync(doc, pL, aNoteL);
			if (aNote->inOttava) DelOttavaForSync(doc, pL, aNoteL);
			if (aNote->firstMod) DelModsForSync(pL, aNoteL);
		}
		DSRemoveDynamic(doc, pL, ANYONE);
	}

	switch(ObjLType(pL)) {
		case SYNCtype:
		case GRSYNCtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case PSMEAStype:
			if (doc->autoRespace) {
				xMove = PMDist(pL, ObjWithValidxd(RightLINK(pL), TRUE));
				tempL = MoveInMeasure(RightLINK(pL), doc->tailL, -xMove);
				MoveMeasures(tempL, doc->tailL, -xMove);
			}
			if (ObjLType(pL)==KEYSIGtype) {
				aKSL = FirstSubLINK(pL);
				for ( ; aKSL; aKSL=NextKEYSIGL(aKSL))
					FixStfBeforeFirst(doc,pL,aKSL);
			}
			DeleteNode(doc, pL);
			break;

		case MEASUREtype: {
			DDIST	dTemp;
			LINK	prevMeasureL;

			if (doc->autoRespace) {
				/* Adjust objects' x-coords. to make relative to their new measure and
					move them left by the space formerly used by the deleted measure. */

				if (MeasureTYPE(RightLINK(pL))) {
					xRelMeas = SysRelxd(RightLINK(pL))-SysRelxd(pL);
					MoveMeasures(RightLINK(pL), doc->tailL, -xRelMeas);
				}
				else {
					dTemp = LinkXD(ObjWithValidxd(RightLINK(pL), FALSE));
					prevMeasureL = LSSearch(LeftLINK(pL),MEASUREtype,ANYONE,
											TRUE,FALSE);
					xRelMeas = SysRelxd(pL)-SysRelxd(prevMeasureL)-dTemp;
					tempL = MoveInMeasure(RightLINK(pL), doc->tailL, xRelMeas);
					MoveMeasures(tempL, doc->tailL, dTemp);
				}
			}
			else {
				/* Adjust objects' x-coords. to make relative to their new measure but
					keep them in the same place on the page. */

			 	if (!MeasureTYPE(RightLINK(pL))) {
					prevMeasureL = LSSearch(LeftLINK(pL),MEASUREtype,ANYONE,TRUE,FALSE);
			 		xMove = SysRelxd(pL)-SysRelxd(prevMeasureL);
			 		MoveInMeasure(RightLINK(pL), doc->tailL, xMove);
			 	}
			}
			
			DeleteMeasure(doc, pL);
			break;
		}

		case CONNECTtype:
			DeleteNode(doc, pL);
			break;

		default:
			MayErrMsg("DeleteWhole: Illegal type %ld at %ld", (long)ObjLType(pL), pL);
			return FALSE;
	}
	
	return TRUE;
}

/* ======================================================= Routines for Deleting Slurs == */
/* Deletion of slurs is handled by: 1) FixDelSlurs, 2) FixTieIndices, and 3) case
SLURtype in DeleteSelection. They are all functionally independent.

1. FixDelSlurs deletes all slurs in the selection range, or slur subobjects if the
	slur is a tie with multiple subobjects. This happens regardless of whether or not
	the slur is selected; the actual deletion is performed by DeleteSlur, which either
	calls DeleteTie to delete tie subobjects, or DeleteNode to delete the entire slur.
2. FixTieIndices is called in case SYNCtype of DeleteSelection. It updates the values
	pSlur->firstInd & lastInd of a tie with multiple subobjects which ties a chord one
	(or more) of whose notes has been deleted.
3. case SLURtype performs the ordinary deletion of a slur which has been selected by
	clicking or dragging and is to be explicitly deleted. *

Note: DeleteTie removed by chirgwin Mon Jun 25 21:56:05 PDT 2012 */


/* ------------------------------------------------------------------------ DeleteSlur -- */
/* 1. DeleteSlur is called with parameters pL & slurNoteL, where slurNoteL
		is the first subobject of pL on the staff with tied or slurred flag set. 
	2. goLeft is set depending on whether FixDelSlurs is traversing left or
		right; isTie is set depending on whether slurNote's tiedL/R or slurredL/R
		flag is set.
	3. DeleteSlur searches in the direction 'goLeft', finds a slur on slurNoteL's
		staff, picks up that slur's firstSyncL or lastSyncL depending on goLeft, 
		clears the flags of the notes in that sync on the slur's staff, and either 
		deletes the entire slur, or passes slurNoteL and first/lastSyncL to 
		DeleteTie (??NOT YET--FOR NOW, it always deletes the entire slur). It only
		needs to clear the flags on one end of the slur, since the note(s) on the
		other end are about to be deleted.
	4. Note that in traversing left, FixDelSlurs deals with notes whose tiedR
		flags are set, and that lastSyncL is therefore the sync whose flags are
		to be cleared, and which is to be passed to DeleteTie, not firstSyncL. */

static void DeleteSlur(
					Document *doc,
					LINK pL,						/* Sync */
					LINK slurNoteL,
					Boolean goLeft, Boolean isTie)
{
	LINK syncL, slurL;
	Boolean firstIsMeas, lastIsSys;
	SearchParam pbSearch;

	firstIsMeas = lastIsSys = FALSE;
	InitSearchParam(&pbSearch);
	pbSearch.voice = NoteVOICE(slurNoteL);
	pbSearch.inSystem = TRUE;
	pbSearch.subtype = isTie;
	
	if (goLeft) {
		slurL = L_Search(pL, SLURtype, GO_LEFT, &pbSearch);
		if (SlurLASTSYNC(slurL)!=pL)
			slurL = L_Search(LeftLINK(slurL), SLURtype, GO_LEFT, &pbSearch);
	}
	else slurL = L_Search(pL, SLURtype, GO_LEFT, &pbSearch);
	
	if (!slurL) {
		MayErrMsg("DeleteSlur: can't find slur in voice %ld", (long)pbSearch.voice);
		return;
	}

	/* Find out if the slur is crossSystem:
			if its first sync is a MEASURE, or last sync is a SYSTEM, set
				firstIsMeas, lastIsSys TRUE.
			If it is a left traversal, the first of the crossSystem pair
				has a MEASURE object in firstSyncL; if right, a SYSTEM object
				in lastSyncL.
		Set one or the other and pass values to DeleteOtherSlur.
		Call FixSyncForSlur to set tiedL/R or slurredL/R flags FALSE in
		sync whose slur is to be deleted. */
		
	syncL = goLeft ? SlurFIRSTSYNC(slurL) : SlurLASTSYNC(slurL);
	
	if (MeasureTYPE(syncL)) 		firstIsMeas = TRUE;
	else if (SystemTYPE(syncL)) 	lastIsSys = TRUE;
	else FixSyncForSlur(syncL, SlurVOICE(slurL), isTie, goLeft);
	
	if (slurL==doc->selStartL) doc->selStartL = RightLINK(doc->selStartL);
		
	if (firstIsMeas || lastIsSys)
		DeleteOtherSlur(doc, firstIsMeas, lastIsSys, slurL);
	
	DeleteNode(doc, slurL);
}


/* ----------------------------------------------------------------------- FixDelSlurs -- */
/* FixDelSlurs pre-processes the selection range for slurs. After expanding the
selection range to include all slur objects immediately following selEndL, it
traverses the object list in each voice once right from selStartL and once left
from selEndL in order to pick up slurs that cross the ends of the selection range.
It calls DeleteSlur with aNoteL as the first subobject in the voice with tied or
slurred flag set. L_Search() guarantees  that the subobject is the first with the
stated characteristics; however, the loop:
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		}
may pass to subobjects on other staves. But the test following:
if (NoteVOICE(aNoteL)==voice...) should take care of this. */

void FixDelSlurs(Document *doc)
{
	LINK			pL, aNoteL;
	PANOTE			aNote;
	short			voice;
	SearchParam		pbSearch;
	Boolean			voiceTiesDone;

	/* If selEndL is a slur and it gets deleted, the selection range will be
	garbage. To avoid this, extend the selection range to include all contiguous
	following slurs. */

	while (SlurTYPE(doc->selEndL)) doc->selEndL = RightLINK(doc->selEndL);
	
	InitSearchParam(&pbSearch);
	pbSearch.needSelected = FALSE;
	pbSearch.id = ANYONE;
	pbSearch.inSystem = FALSE;

	/* Traverse right from selection start, picking up all notes with tiedL/slurredL
		flags set, and call DeleteSlur to delete their slurs. */

	for (voice = 1; voice<=MAXVOICES; voice++)
		if (VOICE_MAYBE_USED(doc, voice)) {
			pbSearch.voice = voice;
			pL = L_Search(doc->selStartL, SYNCtype, GO_RIGHT, &pbSearch);
			for ( ; pL!=doc->selEndL;
					  pL=L_Search(RightLINK(pL), SYNCtype, GO_RIGHT, &pbSearch)) {
	
				if (!pL || IsAfter(doc->selEndL, pL)) break;
				
				voiceTiesDone = FALSE;
				aNoteL = pbSearch.pEntry;
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteVOICE(aNoteL)==voice && NoteSEL(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->tiedL && !voiceTiesDone) {				/* DeleteSlur clears the appropriate flags */
							DeleteSlur(doc, pL, aNoteL, GO_LEFT, TRUE);
							voiceTiesDone = TRUE;
						}
						else if (aNote->tiedR) break;	
						
						if (aNote->slurredL)
							DeleteSlur(doc, pL, aNoteL, GO_LEFT, FALSE);
						else if (aNote->slurredR) break;
					}
			}
		}

	/* Traverse left from selection end, picking up all notes with tiedR/slurredR
		flags	set, and call DeleteSlur to delete their slurs. */

	for (voice = 1; voice<=MAXVOICES; voice++)
		if (VOICE_MAYBE_USED(doc, voice)) {
			pbSearch.voice = voice;
			pL = L_Search(LeftLINK(doc->selEndL), SYNCtype, GO_LEFT, &pbSearch);
			for ( ; pL!=LeftLINK(doc->selStartL); 
					  pL = L_Search(LeftLINK(pL), SYNCtype, GO_LEFT, &pbSearch)) {
	
				if (!pL || IsAfter(pL, doc->selStartL)) break;

				voiceTiesDone = FALSE;
				aNoteL = pbSearch.pEntry;
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteVOICE(aNoteL)==voice && NoteSEL(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->tiedR && !voiceTiesDone) {
							DeleteSlur(doc, pL, aNoteL, GO_RIGHT, TRUE);
							voiceTiesDone = TRUE;
						}
						else if (aNote->tiedL) break;
						
						if (aNote->slurredR)
							DeleteSlur(doc, pL, aNoteL, GO_RIGHT, FALSE);
						else if (aNote->slurredL) break;
					}
			}
		}
}

/* ------------------------------------------------------------------- PPageFixDelSlurs - */
/* Version of FixDelSlurs for clearing (not pasting, as the name and former comments
said!) pages; doesn't require that Syncs be selected, since it assumes the selection
range is set before it's called, and all slurs on page to be cleared must be
processed. */

void PPageFixDelSlurs(Document *doc)
{
	LINK			pL, aNoteL;
	PANOTE			aNote;
	short			voice;
	SearchParam		pbSearch;
	Boolean			voiceTiesDone;

	/* If selEndL is a slur and it gets deleted, the selection range will be
	garbage. To avoid this, extend the selection range to include all contiguous
	following slurs. */

	while (SlurTYPE(doc->selEndL)) doc->selEndL = RightLINK(doc->selEndL);
	
	InitSearchParam(&pbSearch);
	pbSearch.needSelected = FALSE;
	pbSearch.id = ANYONE;
	pbSearch.inSystem = FALSE;

	/* Traverse right from selection start, picking up all notes with tiedL/slurredL
		flags set, and call DeleteSlur to delete their slurs. */

	for (voice = 1; voice<=MAXVOICES; voice++)
		if (VOICE_MAYBE_USED(doc, voice)) {
			pbSearch.voice = voice;
			pL = L_Search(doc->selStartL, SYNCtype, GO_RIGHT, &pbSearch);
			for ( ; pL!=doc->selEndL;
					  pL=L_Search(RightLINK(pL), SYNCtype, GO_RIGHT, &pbSearch)) {
	
				if (!pL || IsAfter(doc->selEndL, pL)) break;
				
				voiceTiesDone = FALSE;
				aNoteL = pbSearch.pEntry;
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteVOICE(aNoteL)==voice) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->tiedL && !voiceTiesDone) {				/* DeleteSlur clears the appropriate flags */
							DeleteSlur(doc, pL, aNoteL, GO_LEFT, TRUE);
							voiceTiesDone = TRUE;
						}
						else if (aNote->tiedR) break;	
						
						if (aNote->slurredL)
							DeleteSlur(doc, pL, aNoteL, GO_LEFT, FALSE);
						else if (aNote->slurredR) break;
					}
			}
		}

	/* Traverse left from selection end, picking up all notes with tiedR/slurredR
		flags	set, and call DeleteSlur to delete their slurs. */

	for (voice = 1; voice<=MAXVOICES; voice++)
		if (VOICE_MAYBE_USED(doc, voice)) {
			pbSearch.voice = voice;
			pL = L_Search(LeftLINK(doc->selEndL), SYNCtype, GO_LEFT, &pbSearch);
			for ( ; pL!=LeftLINK(doc->selStartL); 
					  pL = L_Search(LeftLINK(pL), SYNCtype, GO_LEFT, &pbSearch)) {
	
				if (!pL || IsAfter(pL, doc->selStartL)) break;

				voiceTiesDone = FALSE;
				aNoteL = pbSearch.pEntry;
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteVOICE(aNoteL)==voice) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->tiedR && !voiceTiesDone) {
							DeleteSlur(doc, pL, aNoteL, GO_RIGHT, TRUE);
							voiceTiesDone = TRUE;
						}
						else if (aNote->tiedL) break;
						
						if (aNote->slurredR)
							DeleteSlur(doc, pL, aNoteL, GO_RIGHT, FALSE);
						else if (aNote->slurredL) break;
					}
			}
		}
}

/* ------------------------------------------------------------------- DeleteOtherSlur -- */
/* Handle the deletion of the other slur for crossSystem slurs.
 *
 * firstIsMeas TRUE indicates that the slur whose companion is to be deleted
 * here has a MEASURE in firstSyncL, and that the other slur must be searched
 * for on the previous system. Search for this slur, test if its lastSyncL
 * is a SYSTEM, call FixSyncForSlur to set its tied/slurredL/R flags, and
 * delete it. 
 * Likewise for lastIsSys.
 */

static void DeleteOtherSlur(Document *doc,
							Boolean firstIsMeas, Boolean lastIsSys,
							LINK pL)		/* Cross-system slur */
{
	LINK otherSlur, firstSyncL, lastSyncL;
	Boolean right = TRUE, left = FALSE;
	
	if (firstIsMeas) {
		otherSlur = XSysSlurMatch(pL);
		lastSyncL = SlurLASTSYNC(otherSlur);
		if (SystemTYPE(lastSyncL)) {
			firstSyncL = SlurFIRSTSYNC(otherSlur);
			FixSyncForSlur(firstSyncL, SlurVOICE(pL), SlurTIE(pL), right);
		}
		InvalMeasure(otherSlur, SlurSTAFF(otherSlur));
		DeleteNode(doc, otherSlur);
	}
	if (lastIsSys) {
		otherSlur = XSysSlurMatch(pL);
		firstSyncL = SlurFIRSTSYNC(otherSlur);
		if (MeasureTYPE(firstSyncL)) {
			lastSyncL = SlurLASTSYNC(otherSlur);
			FixSyncForSlur(lastSyncL, SlurVOICE(pL), SlurTIE(pL), left);
		}
		InvalMeasure(otherSlur, SlurSTAFF(otherSlur));
		DeleteNode(doc, otherSlur);
	}
}


/* -------------------------------------------------------------------- FixSyncForSlur -- */
/* Clear the tiedL/R or slurredL/R flags of every note in the given voice in the
given sync, presumably because the slur object is being deleted. */

void FixSyncForSlur(LINK pL,					/* Sync */
					short voice,
					Boolean isTie, Boolean right)
{
	LINK aNoteL; PANOTE aNote;
	
	if (!SyncTYPE(pL)) {
		MayErrMsg("FixSyncForSlur: called with non-Sync LINK %ld of type %ld",
						(long)pL, (long)ObjLType(pL));
		return;
	}

	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->voice==voice) {
			if (right) {
				if (isTie) aNote->tiedR = FALSE;
				else		  aNote->slurredR = FALSE;
			}
			else {
				if (isTie) aNote->tiedL = FALSE;
				else		  aNote->slurredL = FALSE;
			}
		}
	}
}


/* ======================================== Routines for Updating Cross-System Objects == */

/* -------------------------------------------------------------------- DeleteXSysSlur -- */
/* Delete a non-matched crossSystem slur. Get the first or last syncL, whichever
 * is a sync, and call FixSyncForSlur to fix it; then delete the slur.
 */

void DeleteXSysSlur(Document *doc, LINK pL)		/* pL = cross-system slur */
{
	LINK lastSyncL, firstSyncL;
	Boolean right = TRUE, left = FALSE;

	firstSyncL = SlurFIRSTSYNC(pL);
	lastSyncL = SlurLASTSYNC(pL);
	if (SyncTYPE(firstSyncL))
		FixSyncForSlur(firstSyncL, SlurVOICE(pL), SlurTIE(pL), right);
	
	if (SyncTYPE(lastSyncL))
		FixSyncForSlur(lastSyncL, SlurVOICE(pL), SlurTIE(pL), left);

	DeleteNode(doc, pL);
}


/* ---------------------------------------------------------------- FixDelCrossSysObjs -- */
/* Traverse the object list from the first system before the selRange to the system
 * following the selRange, checking for and updating mismatched crossSystem
 * objects. N.B. The updating technique assumes that something has been deleted;
 * it is NOT appropriate for other situations, e.g., where a range has been moved
 *	from one system to another. See below for further details.
 */

void FixDelCrossSysObjs(Document *doc)
{
	FixDelCrossSysBeams(doc);
	FixDelCrossSysSlurs(doc);
	FixDelCrossSysHairpins(doc);
}


/* --------------------------------------------------------------- FixDelCrossSysBeams -- */
/* After post-processing beamsets in PostFixDelBeams, traverse the object list
 * from the first system before the selRange to the system following the selRange,
 * checking for and updating mismatched crossSystem beams. Specifically, when we
 * find a beamset that claims to be crossSystem, we look for what should be its
 *	other piece; if we fail to find one, or we find one but IT's not crossSystem,
 * we set the beamset we started with to be non-crossSystem.
 */

#define CrossSysAndFirstSys(beamL)	 	( BeamCrossSYS(beamL) && BeamFirstSYS(beamL) )
#define CrossSysAndSecondSys(beamL)		( BeamCrossSYS(beamL) && !BeamFirstSYS(beamL) )

void FixDelCrossSysBeams(register Document *doc)
{
	register LINK sysL, pL, firstL, lastL;
	LINK nextBeamL, prevBeamL;
	register short v; register PBEAMSET pBeam;
	Boolean invalSystems=FALSE;

	firstL = lastL = NILINK;
	sysL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, TRUE, TRUE);
	if (sysL) {
		sysL = LSSearch(LeftLINK(sysL), SYSTEMtype, ANYONE, TRUE, TRUE);
		if (sysL) firstL = sysL;
		else firstL = doc->headL;
	}
	else firstL = doc->headL;
	
	sysL = LSSearch(doc->selEndL, SYSTEMtype, ANYONE, FALSE, TRUE);
	if (sysL) {
		sysL = LSSearch(RightLINK(sysL), SYSTEMtype, ANYONE, FALSE, TRUE);
		if (sysL) lastL = sysL;
		else lastL = doc->tailL;
	}
	else lastL = doc->tailL;

	for (v=1; v<=MAXVOICES; v++) {
		if (VOICE_MAYBE_USED(doc, v)) {
			for (pL=firstL; pL!=lastL; pL=RightLINK(pL))
				if (BeamsetTYPE(pL) && BeamVOICE(pL)==v) {
					pBeam = GetPBEAMSET(pL);
					if (pBeam->crossSystem)
						if (pBeam->firstSystem) {
							nextBeamL = LVSearch(RightLINK(pL), BEAMSETtype, v, FALSE, FALSE);
							if (!nextBeamL || !CrossSysAndSecondSys(nextBeamL)) {
								pBeam->crossSystem = pBeam->firstSystem = FALSE;
								invalSystems = TRUE;
							}
						}
						else {
							prevBeamL = LVSearch(LeftLINK(pL), BEAMSETtype, v, TRUE, FALSE);
							if (!prevBeamL || !CrossSysAndFirstSys(prevBeamL)) {
								pBeam->crossSystem = pBeam->firstSystem = FALSE;
								invalSystems = TRUE;
							}
						}
				}
			}
		}

	if (invalSystems) InvalSystems(firstL, lastL);
}


/* --------------------------------------------------------------- FixDelCrossSysSlurs -- */
/* Traverse the object list from the first system before the selRange to the
 * system following the selRange, checking for and updating mismatched crossSystem
 * slurs.
 */
 
#define CrossSysAndFirstSysSl(pL)	 	( SlurCrossSYS(pL) && SlurFirstIsSYSTEM(pL) )
#define CrossSysAndSecondSysSl(pL)		( SlurCrossSYS(pL) && SlurLastIsSYSTEM(pL) )

void FixDelCrossSysSlurs(register Document *doc)
{
	register short v; PSLUR pSlur; Boolean invalSystems=FALSE;
	register LINK sysL, pL;
	LINK firstL, lastL, nextSlurL, prevSlurL, rightL;

	/* Get the system object from which to start the update operation. */
	firstL = lastL = NILINK;
	sysL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, GO_LEFT, TRUE);
	if (sysL)
		sysL = LinkLSYS(sysL);
	firstL = sysL ? sysL : doc->headL;
	
	/* Get the system object at which to end the update operation. */
	sysL = LSSearch(doc->selEndL, SYSTEMtype, ANYONE, GO_RIGHT, TRUE);
	if (sysL)
		sysL = LinkRSYS(sysL);
	lastL = sysL ? sysL : doc->tailL;

	for (v=1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {
			for (pL=firstL; pL!=lastL; pL=rightL) {
				rightL = RightLINK(pL);
				if (SlurTYPE(pL) && SlurVOICE(pL)==v) {
					pSlur = GetPSLUR(pL);
					if (pSlur->crossSystem)
						if (SlurFirstIsSYSTEM(pL)) {
							nextSlurL = LVSearch(RightLINK(pL), SLURtype, v, GO_RIGHT, FALSE);
							if (!nextSlurL || !CrossSysAndSecondSysSl(nextSlurL)) {
								DeleteXSysSlur(doc, pL);
								invalSystems = TRUE;
							}
						}
						else {
							prevSlurL = LVSearch(LeftLINK(pL), SLURtype, v, GO_LEFT, FALSE);
							if (!prevSlurL || !CrossSysAndFirstSysSl(prevSlurL)) {
								DeleteXSysSlur(doc, pL);
								invalSystems = TRUE;
							}
						}
				}
			}
		}

	if (invalSystems) InvalSystems(firstL, lastL);
}


/* ------------------------------------------------------------ FixDelCrossSysHairpins -- */
/* Traverse the object list from the first system before the selRange to the
 * system following the selRange, checking for and updating mismatched crossSystem
 * hairpins. Assumes that there are no intervening dynamics between the first
 * and last syncs of a hairpin on its staff, [-> which would be notationally
 * incorrect <-] . 
 */

#define CrossSysAndFirstSysDyn(pL)	 	( DynamCrossSYS(pL) && DynamFirstIsSYSTEM(pL) )
#define CrossSysAndSecondSysDyn(pL)		( DynamCrossSYS(pL) && DynamLastIsSYSTEM(pL) )

void FixDelCrossSysHairpins(Document *doc)
{
	short s; PDYNAMIC pDynamic; Boolean invalSystems=FALSE, isHairpin;
	LINK sysL, firstL, lastL, pL, nextDynamL, prevDynamL, firstSync, lastSync;

	/* Get the system object from which to start the update operation. */
	firstL = lastL = NILINK;
	sysL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, TRUE, TRUE);
	if (sysL)
		sysL = LinkLSYS(sysL);
	firstL = sysL ? sysL : doc->headL;
	
	/* Get the system object at which to end the update operation. */
	sysL = LSSearch(doc->selEndL, SYSTEMtype, ANYONE, FALSE, TRUE);
	if (sysL)
		sysL = LinkRSYS(sysL);
	lastL = sysL ? sysL : doc->tailL;

	for (s=1; s<=doc->nstaves; s++)
		for (pL=firstL; pL!=lastL; pL=RightLINK(pL))
			if (DynamTYPE(pL) && DynamicSTAFF(pL)==s) {
				isHairpin = IsHairpin(pL);
				if (isHairpin) {
					firstSync = DynamFIRSTSYNC(pL);
					lastSync = DynamLASTSYNC(pL);
					pDynamic = GetPDYNAMIC(pL);
					if (pDynamic->crossSys)
						if (SystemTYPE(lastSync)) {
							nextDynamL = LSSearch(RightLINK(pL), DYNAMtype, s, FALSE, FALSE);
							if (!nextDynamL || !CrossSysAndSecondSysDyn(nextDynamL)) {
								DeleteNode(doc, pL);
								invalSystems = TRUE;
							}
						}
						else {
							prevDynamL = LSSearch(LeftLINK(pL), DYNAMtype, s, TRUE, FALSE);
							if (!prevDynamL || !CrossSysAndFirstSysDyn(prevDynamL)) {
								DeleteNode(doc, pL);
								invalSystems = TRUE;
							}
						}
				}
			}

	if (invalSystems) InvalSystems(firstL, lastL);
}


/* ================================================ FixDelBeams and associated routines = */


/* ----------------------------------------------------------------------- FindTheBeam -- */

static LINK FindTheBeam(LINK startL, short voice)
{
	LINK pL, lBeamL, aNBeamL; PANOTEBEAM aNBeam;
	SearchParam pbSearch;
	
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = FALSE;
	pbSearch.inSystem = TRUE;					/* TRUE until we start handling crossSystem beams. */
	pbSearch.subtype = NoteBeam;
	lBeamL = L_Search(startL, BEAMSETtype, TRUE, &pbSearch);
	
	if (lBeamL) {
		aNBeamL = FirstSubLINK(lBeamL);
		for (; aNBeamL; aNBeamL = NextNOTEBEAML(aNBeamL)) {
			aNBeam = GetPANOTEBEAM(aNBeamL);
			pL = aNBeam->bpSync;
			if (pL==startL) return lBeamL;
		}
	}
	return NILINK;
}

/* --------------------------------------------------------------------- FindTheGRBeam -- */

static LINK FindTheGRBeam(LINK startL, short voice)
{
	LINK pL, lBeamL, aNBeamL; PANOTEBEAM aNBeam;
	SearchParam pbSearch;
	
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = FALSE;
	pbSearch.inSystem = TRUE;					/* TRUE until we start handling crossSystem beams. */
	pbSearch.subtype = GRNoteBeam;
	lBeamL = L_Search(startL, BEAMSETtype, TRUE, &pbSearch);
	
	if (lBeamL) {
		aNBeamL = FirstSubLINK(lBeamL);
		for (; aNBeamL; aNBeamL = NextNOTEBEAML(aNBeamL)) {
			aNBeam = GetPANOTEBEAM(aNBeamL);
			pL = aNBeam->bpSync;
			if (pL==startL) return lBeamL;
		}
	}
	return NILINK;
}


/* ----------------------------------------------------------------------- FixDelBeams -- */
/* FixDelBeams pre-processes the selection range for beamsets. It traverses the 
 * range,
 * 1. deleting beamsets which extend into or occur within the range, and
 * re-beaming, if possible, notes before the range, and
 * 2. deleting beamsets which extend past the end of the range, re-beaming,
 * if possible, notes which are past the end of the range.
 */

static void FixDelBeams(Document *doc)
{
	short		i, h, voice, v, nInBeam;
	LINK		startL, endL, pL, qL, pFirstL, pLastL,
				rStartL, rEndL, lBeamL, rBeamL, beamFirstL, newBeamL;
	PBEAMSET	lBeam, rBeam, newBeam;
	Boolean		spansRange, crossSys, firstSys;
	
	spansRange = FALSE;
	for (i = 0; i<=MAXVOICES; i++)
		beamStartL[i] = beamEndL[i] = NILINK;

	for (voice=1; voice<=MAXVOICES; voice++) {
		if (VOICE_MAYBE_USED(doc, voice)) {
			GetNoteSelRange(doc, voice, &startL, &endL, NOTES_ONLY);
			if (!startL || !endL) continue;
			if (BetweenIncl(doc->selStartL, startL, doc->selEndL)
			&&  BetweenIncl(doc->selStartL, endL, doc->selEndL)) {
	
				lBeamL = FindTheBeam(startL, voice);
				if (lBeamL==doc->selStartL)
					doc->selStartL = RightLINK(doc->selStartL);
	
				/* If there is a beam across the left end of the voice's selRange,
					traverse right until the first node that has all notes selected
					in our voice. Count up all the notes in voice between the beamset
					and the selRange, and re-beam those notes. */
	
				if (lBeamL) {
					nInBeam = 0;
					qL=RightLINK(lBeamL);
					beamFirstL = NILINK;
					for (h=0; h<LinkNENTRIES(lBeamL); qL=RightLINK(qL)) {
						if (LinkSEL(qL)) 
							break;
						if (SyncTYPE(qL)) {
							if (SyncInVoice(qL, voice))
								{ nInBeam++; h++; }
						}
						if (!beamFirstL) beamFirstL = qL;
					}
	
					/* If lBeamL extends past the right end of the range (all the way
						across the range), set spansRange TRUE and save beam end syncs
						for lBeamL in beamStartL[voice],beamEndL[voice]. 
						If beamFirstL exists, it is the last unselected sync in the beginning
						of the range of the beamset. Otherwise, take the first unselected
						sync in the end of the range of the beamset, and use this for
						beamStartL.
						beamFirstL is the first beamable sync in lBeamL which is not to be
						deleted; it is saved in beamStartL[voice]; LastInBeam is saved in
						lBeamL[voice].
						endL as returned by GetNoteSelRange is the first object after the note
						selRange; therefore the call to IsAfter also needs to check equality. */
	
					pFirstL = FirstInBeam(lBeamL);
					pLastL = LastInBeam(lBeamL);
					if ((endL==pLastL) || IsAfter(LeftLINK(endL), pLastL)) {
						if (beamFirstL)
							beamStartL[voice] = beamFirstL;
						else
							beamStartL[voice] = endL;
						beamEndL[voice] = pLastL;
						spansRange = TRUE;
					}
	
					/* Delete the beam, and if it didn't span the range, re-beam
						those notes in the voice between the beamset and the range. */
					for (v=1; v<=MAXVOICES; v++)
						if (v!=voice) {
							if (lBeamL==beamStartL[v]) beamStartL[v] = RightLINK(lBeamL);
							if (lBeamL==beamEndL[v]) beamEndL[v] = LeftLINK(lBeamL);
						}
					
					lBeam = GetPBEAMSET(lBeamL);
					crossSys = lBeam->crossSystem;
					firstSys = lBeam->firstSystem;
					RemoveBeam(doc, lBeamL, voice, TRUE);
					if (spansRange) { spansRange = FALSE; continue; }
					if (nInBeam >= 2) {
						newBeamL = CreateBEAMSET(doc, pFirstL, qL, voice, nInBeam, FALSE,
															doc->voiceTab[voice].voiceRole);
						newBeam = GetPBEAMSET(newBeamL);
						if (crossSys && !firstSys) {
							newBeam->crossSystem = TRUE;
							newBeam->firstSystem = FALSE;
						}
					}
				}
	
				/* If there was a beam across the left end, it was removed by RemoveBeam,
					so rBeamL must be a different beam (starting within the range and
					spanning the right end).
					If rBeamL exists, save its start/end links, and remove rBeamL.
					Then, if possible, beam all notes on the voice between the end of the
					range and the last note of rBeamL. */
	
				rBeamL = VHasBeamAcross(endL, voice);
				if (rBeamL) {											/* Diff. beam across r.end ? */
					rStartL = FirstInBeam(rBeamL);						/* Yes, remove beam but save bounds */
					rEndL = LastInBeam(rBeamL);
					for (v=1; v<=MAXVOICES; v++)
						if (v!=voice) {
							if (rBeamL==beamStartL[v]) beamStartL[v] = RightLINK(rBeamL);
							if (rBeamL==beamEndL[v]) beamEndL[v] = LeftLINK(rBeamL);
						}
	
					rBeam = GetPBEAMSET(rBeamL);
					crossSys = rBeam->crossSystem;
					firstSys = rBeam->firstSystem;
					RemoveBeam(doc, rBeamL, voice, TRUE);
					rStartL = LSSearch(endL, SYNCtype, voice, FALSE, FALSE);	/* Get new beginning */
					
					if (!IsAfterIncl(rStartL, rEndL)) {							/* Excludes undefined relationship (IsAfter doesn't) */
						MayErrMsg("FixDelBeams: rStartL=%ld is after rEndL=%ld.\n",
									(long)rStartL, (long)rEndL);
						continue;
					}
	
					nInBeam = 0;
					for (pL = rStartL; pL!=RightLINK(rEndL) && pL; pL=RightLINK(pL))
						if (SyncTYPE(pL))
							if (SyncInVoice(pL, voice)) nInBeam++;
					if (nInBeam >=2) {
						newBeamL = CreateBEAMSET(doc, rStartL, RightLINK(rEndL), voice, nInBeam,
															FALSE, doc->voiceTab[voice].voiceRole);
						newBeam = GetPBEAMSET(newBeamL);
						if (crossSys && firstSys)
							newBeam->crossSystem = newBeam->firstSystem = TRUE;
					}
				}
			}
		}
	}
}
	

/* ------------------------------------------------------------------- PostFixDelBeams -- */
/* Post-process the beams. If there were any beams which spanned the range, their
First/LastInBeams were saved in beamStartL[]/EndL[]. For each voice, count all notes
on voice between beamStartL[voice] & beamEndL[voice], and beam them. */

static void PostFixDelBeams(Document *doc)
{
	short v, nInBeam; 
	LINK newBeamL, nextBeamL, prevBeamL;
	PBEAMSET newBeam, nextBeam, prevBeam;
	SearchParam pbSearch;

	for (v=1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {
			if (beamStartL[v])
				if (!IsAfterIncl(beamStartL[v], beamEndL[v])) continue;

				nInBeam = CountBeamable(doc, beamStartL[v], RightLINK(beamEndL[v]), v, FALSE);
	
				if (nInBeam >=2) {
					newBeamL = CreateBEAMSET(doc, beamStartL[v], RightLINK(beamEndL[v]), v, 
										nInBeam, FALSE, doc->voiceTab[v].voiceRole);
				
					/* Search for the previous and succeeding beamsets; if either (or both)
					exists, is not on the same system, and is crossSystem on the correct system,
					then the newly created beamset must be its (their) partner.
					Question: what if the beam must be both first and second system?
					(Need a flag for each). */
	
					InitSearchParam(&pbSearch);
					pbSearch.id = ANYONE;
					pbSearch.voice = v;
					pbSearch.needSelected = FALSE;
					pbSearch.inSystem = FALSE;
					pbSearch.subtype = NoteBeam;
					prevBeamL = L_Search(LeftLINK(newBeamL), BEAMSETtype, TRUE, &pbSearch);
					if (prevBeamL)
						if (!SameSystem(newBeamL, prevBeamL)) {
							prevBeam = GetPBEAMSET(prevBeamL);
							if (prevBeam->crossSystem && prevBeam->firstSystem) {
								newBeam = GetPBEAMSET(newBeamL);
								newBeam->crossSystem = TRUE;
								newBeam->firstSystem = FALSE;
							}
						}
					nextBeamL = L_Search(RightLINK(newBeamL), BEAMSETtype, FALSE, &pbSearch);
					if (nextBeamL)
						if (!SameSystem(newBeamL, nextBeamL)) {
							nextBeam = GetPBEAMSET(nextBeamL);
							if (nextBeam->crossSystem && !nextBeam->firstSystem) {
								newBeam = GetPBEAMSET(newBeamL);
								newBeam->crossSystem = newBeam->firstSystem = TRUE;
							}
						}
				}
			}

	FixDelCrossSysBeams(doc);
}


/* --------------------------------------------------------------------- FixDelGRBeams -- */

static void FixDelGRBeams(Document *doc)
{
	short v;
	LINK startL, endL, lStartL, lEndL, rStartL, rEndL, lBeamL, rBeamL;

	for (v = 1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {
			GetNoteSelRange(doc, v, &startL, &endL, GRNOTES_ONLY);
			if (!startL || !endL) continue;
	
			if (BetweenIncl(doc->selStartL, startL, doc->selEndL) &&
					BetweenIncl(doc->selStartL, endL, doc->selEndL)) {
	
				lBeamL = FindTheGRBeam(startL, v);
	
				if (lBeamL) {
					if (lBeamL==doc->selStartL)
						doc->selStartL = RightLINK(doc->selStartL);
	
					lStartL = FirstInBeam(lBeamL);
					lEndL = RightLINK(LastInBeam(lBeamL));
					RemoveGRBeam(doc, lBeamL, v, TRUE);
					RecomputeGRNoteStems(doc, lStartL, lEndL, v);
				}
	
				rBeamL = VHasGRBeamAcross(endL, v);
				if (rBeamL) {										/* Diff. beam across r.end ? */
					rStartL = FirstInBeam(rBeamL);					/* Yes, remove beam but save bounds */
					rEndL = LastInBeam(rBeamL);
					RemoveGRBeam(doc, rBeamL, v, TRUE);
					RecomputeGRNoteStems(doc, lStartL, lEndL, v);
				}
			}
		}
}


/* ============================================== Delete objects associated with syncs == */

/* Set inTuplet flags FALSE for all notes in tupletL. */

static void ClearInTupletFlags(LINK tupletL)
{
	PANOTETUPLE	aNoteTuple;	short v;
	LINK aNoteTupleL, qL, aNoteL;

	v = TupletVOICE(tupletL);
	aNoteTupleL = FirstSubLINK(tupletL);
	for ( ; aNoteTupleL; aNoteTupleL=NextNOTETUPLEL(aNoteTupleL)) {
		aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
		qL = aNoteTuple->tpSync;
		aNoteL = FirstSubLINK(qL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
			if (NoteVOICE(aNoteL)==v)
				NoteINTUPLET(aNoteL) = FALSE;
		}
	}	
}

/* ------------------------------------------------------------------ DelTupletForSync -- */
/* If a note to be deleted is inTuplet, delete the tuplet itself, and set all
inTuplet flags FALSE for all notes in that tuplet. */

static void DelTupletForSync(Document *doc, LINK pL, LINK aNoteL)
{
	PANOTETUPLE	aNoteTuple;
	LINK tupletL, aNoteTupleL;
	
	tupletL = LVSearch(pL, TUPLETtype, NoteVOICE(aNoteL), GO_LEFT, FALSE);
	if (tupletL) {
		aNoteTupleL = FirstSubLINK(tupletL);
		for ( ; aNoteTupleL; aNoteTupleL=NextNOTETUPLEL(aNoteTupleL)) {
			aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
			if (aNoteTuple->tpSync==pL) { 							/* tupletL is the one */
				ClearInTupletFlags(tupletL);
				DeleteNode(doc, tupletL);
				return;
			}
		}
	}
	else MayErrMsg("DelTupletForSync: can't find tuplet for note %ld in Sync %ld",
					(long)aNoteL, (long)pL);
}


/* ------------------------------------------------------------------- DelOttavaForSync - */
/* Assuming the given note is to be deleted and is inOttava, remove its subObject
from its ottava object, and decrement the ottava's nEntries. If that leaves
nothing in the ottava, remove it, too. */

static void DelOttavaForSync(Document *doc, LINK pL, LINK aNoteL)
{
	PANOTEOTTAVA	aNoteOttava;
	LINK				octL, aNoteOctL, nextOctL;
	
	octL = LSSearch(pL, OTTAVAtype, NoteSTAFF(aNoteL), GO_LEFT, FALSE);
	if (octL) {
		aNoteOctL = FirstSubLINK(octL);
		for ( ; aNoteOctL; aNoteOctL = nextOctL) {
			nextOctL = NextNOTEOTTAVAL(aNoteOctL);
			aNoteOttava = GetPANOTEOTTAVA(aNoteOctL);
			if (aNoteOttava->opSync==pL) { 							/* octL is the one */
				RemoveLink(octL, NOTEOTTAVAheap, FirstSubLINK(octL), aNoteOctL);
				HeapFree(NOTEOTTAVAheap, aNoteOctL);
				LinkNENTRIES(octL)--;
				if (LinkNENTRIES(octL)==0) {
					DeleteNode(doc, octL); return;
				}
			}
		}
	}
	else
		MayErrMsg("DelOttavaForSync: can't find Ottava for note %ld in Sync %ld",
					(long)aNoteL, (long)pL);
}


/* ------------------------------------------------------------------- DelModsForSync -- */
/* If a note to be deleted has any note modifiers, remove the modifiers
from the modifier subObject heap. */ 

void DelModsForSync(LINK /*pL*/, LINK aNoteL)
{
	LINK	aModNRL;
	PANOTE	aNote;
	
	aNote = GetPANOTE(aNoteL);
	if (aNote->firstMod) {
		aModNRL = aNote->firstMod;
		aNote->firstMod = NILINK;
		HeapFree(MODNRheap, aModNRL);
	}
}


/* ============================================= Update measures, context, accidentals == */


/* -------------------------------------------------------------------- FixDelMeasures -- */
/* Fix the measureRect.rights of the Measures whose width changes: the one
preceding the region deleted, and the one at the end of the System.  We
expect the Draw routines to update the measureBBoxes, since the valid flags
are FALSE. */

static void FixDelMeasures(Document *doc, LINK measL)	/* measL must be a Measure */
{
	PAMEASURE	aMeasure;
	PSYSTEM		pSystem;
	LINK		aMeasureL, qL, systemL;
	DDIST		measWidth;
	Boolean 	endOfSystem;
	
	if (!MeasureTYPE(measL)) 
		MayErrMsg("FixDelMeasures: argument %ld not a Measure", (long)measL);

	endOfSystem = LastMeasInSys(measL);
	systemL = LSSearch(measL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);

	/* If measL is the last measure of the system or the score, get the width
		of the systems in the score, and set the measureRect.right of measL to
		equal sysWidth minus measL's xd, i.e. reset the measureRect of measL based
		on the systemWidth and measL's new xd. */

	if (!LinkRMEAS(measL) || endOfSystem) {
		pSystem = GetPSYSTEM(systemL);

		measWidth = pSystem->systemRect.right - pSystem->systemRect.left - LinkXD(measL);
		aMeasureL = FirstSubLINK(measL);
		for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
			aMeasure = GetPAMEASURE(aMeasureL);
			aMeasure->measSizeRect.right = measWidth;
		}

		return;
	}

	/* If measL is not the last measure of the system or the score, set the
		measSizeRect.right of measL to equal the xd of measL's rMeasure minus measL's xd. */

	aMeasureL = FirstSubLINK(measL);
	for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
		aMeasure = GetPAMEASURE(aMeasureL);
		aMeasure->measSizeRect.right = LinkXD(LinkRMEAS(measL))-LinkXD(measL);
	}

	/* Then reset the measSizeRect of the last measure in the system the same
		as above. ??This sets it incorrectly (to much too large a value); nonetheless,
		after the Delete is done, it seems to be correct! ?? */

	if (LinkRSYS(systemL))
		measL = LSSearch(LinkRSYS(systemL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
	else
		measL = LSSearch(doc->tailL, MEASUREtype, ANYONE, GO_LEFT, FALSE);

	LinkVALID(measL) = FALSE;										/* So Draw updates measureBBox */
	qL = LSSearch(doc->tailL, MEASUREtype, ANYONE, GO_LEFT, FALSE);

	pSystem = GetPSYSTEM(systemL);
	measWidth = pSystem->systemRect.right - LinkXD(qL);
	aMeasureL = FirstSubLINK(measL);
	for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
		aMeasure = GetPAMEASURE(aMeasureL);
		aMeasure->measSizeRect.right = measWidth;
	}
}


/* ----------------------------------------------------------------- FixDelAccidentals -- */
/* FixDelAccidentals assumes the given range on the given staff is about to be
deleted and fixes up accidentals of following notes on the staff according to
standard CMN rules so they will retain their current pitches. If ACC_IN_CONTEXT
is FALSE, it does nothing.

This is tricky.
1. First, for every line/space that has a note with an explicit accidental in the
deleted stuff in the bar the delete ends in, that accidental should be propagated.
2. Then, for every note after the last barline preceding the deleted stuff (these
notes will end up in the bar the delete ends in, though they will have been before
the delete only if no barlines are being deleted) that has an explicit accidental
on it, a natural to cancel it should be propagated *unless* the note it would
propogate to is tied to it.
3. We must consider the key signature before the deleted stuff if and only if a
change of key signature is being deleted, since otherwise the effects of all key
signatures will already have been taken into account.

N.B. The loop under "Efficiency" could be made smarter, since any key sig. remaining
that affects the stuff after the delete may be the same as the last deleted one,
but at worst we'll just end up wasting some CPU time.
*/

static void FixDelAccidentals(Document *doc, short staff, LINK stfSelStart, LINK stfSelEnd)
{
	LINK		pL, fixFirstL, fixLastL, nextKSL,
				nextSyncL, aNoteL, prevSyncL, firstSyncL, bNoteL;
	PANOTE		aNote;
	SignedByte	accTab[MAX_STAFFPOS], pitTab1[MAX_STAFFPOS], pitTab2[MAX_STAFFPOS];
	short		halfLn, j;
	SearchParam	pbSearch;

	if (!ACC_IN_CONTEXT) return;
	
	/* Compute an accidental table that converts the pitch modifier table for the
		beginning of the delete region into the pitch modifier table for the end of
		the delete region. Each entry in the table says something like "the notation
		on this staff starting at <stfSelEnd> is correct for a context where, on the
		top staff line, double-sharp is in effect, but there won't be a double-sharp
		in effect after the delete.", or "don't worry about the 3rd space on this
		staff--the delete won't make any change in its accidental context". */

	GetPitchTable(doc, pitTab1, stfSelStart, staff);
	GetPitchTable(doc, pitTab2, stfSelEnd, staff);
	for (j = 0; j<MAX_STAFFPOS; j++)
		if (pitTab1[j]==pitTab2[j]) accTab[j] = 0;
		else								 accTab[j] = pitTab2[j];

	/* We want to fix accidentals from staff selEnd to the next Measure on the staff. */
	
	fixFirstL = stfSelEnd;
	fixLastL = LSSearch(stfSelEnd, MEASUREtype, staff, GO_RIGHT, FALSE);
	if (!fixLastL) fixLastL = doc->tailL;

	/* Handle the "unless" clause of rule 2: for each note in the first Sync after
		the delete region, if that note is now tied back across the barline, fix accTab
		to leave its accidental alone. */
	
	nextSyncL = SSearch(stfSelEnd, SYNCtype, GO_RIGHT);
	if (nextSyncL && IsAfter(nextSyncL, fixLastL))
		for (aNoteL = FirstSubLINK(nextSyncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==staff && GetPANOTE(aNoteL)->tiedL)
				if (PrevTiedNote(nextSyncL, aNoteL, &prevSyncL, &bNoteL))
					if (!SameMeasure(nextSyncL, prevSyncL))
						if (FirstTiedNote(nextSyncL, aNoteL, &firstSyncL, &bNoteL)) {
							aNote = GetPANOTE(aNoteL);
							halfLn = qd2halfLn(aNote->yqpit)+ACCTABLE_OFF;
							accTab[halfLn] = 0;
					}				
	
	/* Search left from the stfSelEnd for a keysig. If it is being deleted (it equals
		or follows stfSelStart), look for the first keysig after the stfSelRange,
		because that is the terminus of the area to be updated. (Cf supra:
		Efficiency.) */

	pL = LSSearch(LeftLINK(stfSelEnd), KEYSIGtype, staff, GO_LEFT, FALSE);
	if (!pL) pL = doc->headL;
	if (IsAfter(LeftLINK(stfSelStart), pL)) {						/* Deleting a key sig.? */
		InitSearchParam(&pbSearch);
		pbSearch.id = staff;
		pbSearch.voice = ANYONE;
		pbSearch.needSelected = FALSE;
		pbSearch.inSystem = FALSE;
		pbSearch.needInMeasure = TRUE;
		nextKSL = L_Search(stfSelEnd, KEYSIGtype, GO_RIGHT, &pbSearch);
		if (!nextKSL) nextKSL = doc->tailL;
	}
	else nextKSL = fixLastL; 									/* Nope */
	
	while (IsAfter(fixFirstL, fixLastL)) {						/* Till next key sig, fix accs. in 1 measure */
		for (j = 0; j<MAX_STAFFPOS; j++)						/* Initialize accTable for each bar, */
			accTable[j] = accTab[j];							/* since FixAllAccidentals destroys it */
		FixAllAccidentals(fixFirstL, fixLastL, staff, FALSE);
		fixFirstL = fixLastL;
		if (fixLastL!=doc->tailL) {								/* More to do? */
			fixLastL = LSSearch(RightLINK(fixLastL),MEASUREtype,staff,GO_RIGHT,FALSE);
			if (fixLastL==NILINK) fixLastL = doc->tailL;
			if (IsAfter(nextKSL, fixLastL)) fixLastL = nextKSL;
		}
	}
}


/* ---------------------------------------------------------------------- PreFDContext -- */
/* If the initial clef, keysig, or timesig on the given staff is about to be
deleted, first set the context fields of the initial Staff subobj (which is to
its left in the object list) to the current default. When these initial objects
are deleted, they aren't really: they're just made invisible and set to the
current defaults, so this is necessary to maintain consistency.

This function can be called safely only if the selection range is entirely before
the first Measure of the score. */

static void PreFDContext(Document *, short, LINK);
static void PreFDContext(Document *doc, short stf, LINK firstStaffL)
{
	LINK aStaffL, pL; PASTAFF aStaff;
	
	aStaffL = FirstSubLINK(firstStaffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==stf) break;
	aStaff = GetPASTAFF(aStaffL);
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case CLEFtype:
				break;
			case KEYSIGtype:
				break;
			case TIMESIGtype:
				if (ObjOnStaff(pL, stf, TRUE)) {
					aStaff->timeSigType = DFLT_TSTYPE;
					aStaff->numerator = config.defaultTSNum;
					aStaff->denominator = config.defaultTSDenom;
				}
				break;
			default:
				;
		}
}


/* --------------------------------------------------------------------- FixDelContext -- */
/* Fix context for clef, keysig, timesig and dynamic. */

static void FixDelContext(Document *doc)
{
	short	stf;
	LINK	firstMeasL, firstStaffL, stSelStartL, stSelEndL, doneL, endL;
	Boolean	beforeFirstMeas;
	CONTEXT	oldContext,newContext;
	
	for (stf = 1; stf<=MAXSTAVES; stf++)
		cntxtDoneL[stf] = NILINK;

	firstMeasL = SSearch(doc->headL, MEASUREtype, GO_RIGHT);
	firstStaffL = SSearch(doc->headL, STAFFtype, GO_RIGHT);
	beforeFirstMeas = IsAfter(doc->selStartL, firstMeasL);
	
	/* The following assumes not just that the selection is continuous
		from selStartL to selEndL overall, but also that it's continuous
		over some (possibly smaller) range on each staff. */
	
	for (stf = 1; stf<=doc->nstaves; stf++) {

		GetStfSelRange(doc, stf, &stSelStartL, &stSelEndL);		/* Get the selection range on the staff. */
		if (!stSelStartL || !stSelEndL) continue;
		if (stSelStartL==stSelEndL) continue;

		FixDelAccidentals(doc, stf, stSelStartL, stSelEndL);		/* Fix the accidentals for the new situation. */

		/* If the initial clef, keysig, or timesig on this staff is being
			deleted, first set the context fields of the initial Staff subobj
			(which is to its left in the object list) to the current
			default. This is so that ??? */
		if (beforeFirstMeas) PreFDContext(doc, stf, firstStaffL);
		
		/* Get the context at the end of the selRange, which is
			the context prior to deletion, and the context before
			the selRange, which will be the new context for the
			objects after the selRange, after deletion. */

		GetContext(doc, LeftLINK(stSelEndL), stf, &oldContext);
		GetContext(doc, LeftLINK(stSelStartL), stf, &newContext);

		/* Fix the context for clef, keySig, timesig & dynamic, based on the
			old and new contexts. */
		doneL = FixContextForClef(doc, stSelEndL, stf, oldContext.clefType,
											newContext.clefType);

		endL = UpdateKSContext(doc, stSelEndL, stf, oldContext, newContext);
		cntxtDoneL[stf] = endL;

		UpdateTSContext(doc, stSelEndL, stf, newContext);

		FixContextForDynamic(doc, stSelEndL, stf,
					oldContext.dynamicType, newContext.dynamicType);
	}
}


/*
 * Extend the selection range internally to include any J_D objects all of
 * whose owned notes are selected while the J_D obj itself is not selected.
 *
 * Currently, this only applies to beams.
 * This should only operate on beams entirely inside the range, as beams
 * extending across the ends of the range should have been processed by
 * FixDelBeams.
 */

static void ExtendJDRange(Document *doc)
{
	LINK pL,qL,firstL,lastL,aNoteL,aGRNoteL;
	short v,nSel;
	
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (BeamsetTYPE(pL) && !LinkSEL(pL)) {
			firstL = FirstInBeam(pL);
			lastL = LastInBeam(pL);
			v = BeamVOICE(pL);
			nSel = 0;
			
			if (GraceBEAM(pL)) {
				for (qL = firstL; qL!=RightLINK(lastL); qL = RightLINK(qL))
					if (GRSyncTYPE(qL)) {
						aGRNoteL = FirstSubLINK(qL);
						for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
							if (GRMainNoteInVOICE(aGRNoteL,v) && GRNoteSEL(aNoteL))
								nSel++;
						}
					}
			}
			else {
				for (qL = firstL; qL!=RightLINK(lastL); qL = RightLINK(qL))
					if (SyncTYPE(qL)) {
						aNoteL = FirstSubLINK(qL);
						for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
							if (MainNoteInVOICE(aNoteL,v) && NoteSEL(aNoteL))
								nSel++;
						}
					}
			}
			if (nSel==LinkNENTRIES(pL))
				LinkSEL(pL) = TRUE;
		}
}


/* ================================================== Deleting BeforeFirstMeas objects == */

void ChangeSpaceBefFirst(
		Document *doc,
		LINK pL,
		DDIST change,
		Boolean allSystems 	/* TRUE=change in pL's System and all following, else pL's only */
		)
{
	LINK qL, firstMeas;
	
	qL = MoveInMeasure(RightLINK(pL), doc->tailL, change);
	if (allSystems) MoveAllMeasures(qL, doc->tailL, change);
	else				 MoveMeasures(qL, doc->tailL, change);
	
	firstMeas = LSSearch(doc->headL, MEASUREtype, ANYONE, FALSE, FALSE);
	InvalRange(pL, firstMeas);
}

static DDIST GetNewKSWidth(Document *doc, LINK keySigL, short stf)
{
	LINK aKeySigL; CONTEXT context; DDIST KSWidth=0;

	aKeySigL = FirstSubLINK(keySigL);
	for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL))
		if (KeySigSTAFF(aKeySigL)!=stf) {
			GetContext(doc, keySigL, KeySigSTAFF(aKeySigL), &context);
			KSWidth = n_max(KSWidth,KSDWidth(aKeySigL,context));
		}

	return KSWidth;
}

/* keySigL is the keySig object from which subObj aKSL is being deleted.
Context is being updated */

static void FixStfBeforeFirst(Document *doc, LINK keySigL, LINK aKSL)
{
	short stf; LINK pL,sysKSL,rightL;
	DDIST haveWidth,needWidth,change;

	stf = KeySigSTAFF(aKSL);

	for (pL=keySigL; pL && pL!=cntxtDoneL[stf]; pL=RightLINK(pL))
		if (SystemTYPE(pL)) {
			sysKSL = SSearch(pL,KEYSIGtype,GO_RIGHT);
			rightL = RightLINK(sysKSL);
			haveWidth = LinkXD(rightL)-LinkXD(sysKSL);
			needWidth = GetNewKSWidth(doc,sysKSL,stf);
			change = needWidth-haveWidth;
			if (change) ChangeSpaceBefFirst(doc, sysKSL, change, FALSE);
		}
}


/* -------------------------------------------------------------------------------------- */
/* Utilities for DelClefBeforeFirst. */

/*
 * Traverse clefL's subObj list and process selected subObjects
 * for deletion: set them invisible, de-select them, and give
 * them default values.
 */

static Boolean InvisifyBFClef(Document */*doc*/, LINK clefL)
{
	LINK aClefL; PACLEF aClef;
	Boolean didAnything=FALSE;

	aClefL = FirstSubLINK(clefL);
	for ( ; aClefL; aClefL=NextCLEFL(aClefL))
		if (ClefSEL(aClefL)) {
			LinkVALID(clefL) = FALSE;
			aClef = GetPACLEF(aClefL);
			aClef->visible = FALSE;
			aClef->selected = FALSE;
			aClef->subType = DFLT_CLEF;
			
			UpdateBFClefStaff(clefL,ClefSTAFF(aClefL),DFLT_CLEF);

			didAnything = TRUE;
		}

	return didAnything;
}


/*
 * If deletion has remove all subObjs of clefL, translate following
 * objects to the left by the previous width of the clef, and remove
 * any graphics which may have been attached to it.
 */

static void RemoveBFClef(Document *doc, LINK clefL)
{
	Boolean isClef;
	DDIST change;

	isClef = InitialClefExists(clefL);

	if (!isClef) {
		change = GetClefSpace(clefL);
		ChangeSpaceBefFirst(doc, clefL, -change, FALSE);
		
		DSRemoveGraphic(doc, clefL);
		DSRemoveTempo(doc, clefL);
	}
}

/* ------------------------------------------------------------------- DelClefBefFirst -- */
/* Delete the initial clef <pL>. */

static Boolean DelClefBefFirst(Document *doc, LINK pL, LINK *pNewSelL)
{
	Boolean didAnything;
	
	didAnything = InvisifyBFClef(doc,pL);
	RemoveBFClef(doc,pL);

	if (didAnything) *pNewSelL = RightLINK(pL);
		
	return didAnything;
}


/* -------------------------------------------------------------------------------------- */
/* Utilities for DelKeySigBeforeFirst. */

/* Traverse keySigL's subObj list and process selected subObjects for deletion:
set them invisible, de-select them, and give them default values.

CR Note: I can find no facility for creating default KSINFO; this function
does nothing with the KSItem field of the deleted keySig subObj. */

static Boolean InvisifyBFKeySig(Document */*doc*/, LINK keySigL)
{
	LINK aKeySigL; PAKEYSIG aKeySig;
	Boolean didAnything=FALSE; KSINFO newKSInfo;

	aKeySigL = FirstSubLINK(keySigL);
	for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL))
		if (KeySigSEL(aKeySigL)) {
			LinkVALID(keySigL) = FALSE;
			aKeySig = GetPAKEYSIG(aKeySigL);
			aKeySig->visible = FALSE;
			aKeySig->selected = FALSE;
			aKeySig->nKSItems = DFLT_NKSITEMS;
			
			KEYSIG_COPY((PKSINFO)aKeySig->KSItem, &newKSInfo);		/* Copy old keysig. info */
			newKSInfo.nKSItems = DFLT_NKSITEMS;
			UpdateBFKSStaff(keySigL,KeySigSTAFF(aKeySigL),newKSInfo);
			didAnything = TRUE;
		}

	return didAnything;
}


static void RemoveBFKeySig(Document *doc, LINK keySigL)
{
	LINK rightL; DDIST needWidth,haveWidth,change;
	Boolean isKeySig;

	isKeySig = BFKeySigExists(keySigL);
	
	rightL = FirstValidxd(RightLINK(keySigL),GO_RIGHT);
	haveWidth = LinkXD(rightL)-LinkXD(keySigL);
	needWidth = GetKeySigWidth(doc,keySigL,1);
	
	change = needWidth-haveWidth;
	if (change) ChangeSpaceBefFirst(doc, keySigL, change, TRUE);
	if (!isKeySig) {
		DSRemoveGraphic(doc, keySigL);
		DSRemoveTempo(doc, keySigL);
	}
}


/* -------------------------------------------------------------- DelKeySigBeforeFirst -- */
/* Delete the initial key signature <pL>. */

static Boolean DelKeySigBefFirst(Document *doc, LINK pL)
{
	Boolean didAnything;

	didAnything = InvisifyBFKeySig(doc,pL);
	RemoveBFKeySig(doc,pL);

	return didAnything;
}

/* -------------------------------------------------------------------------------------- */
/* Utilities for DelTimeSigBeforeFirst. */

/*
 * Traverse timeSigL's subObj list and process selected subObjects
 * for deletion: set them invisible, de-select them, and give
 * them default values.
 */

static Boolean InvisifyBFTimeSig(Document */*doc*/, LINK timeSigL)
{
	LINK aTimeSigL; PATIMESIG aTimeSig;
	Boolean didAnything=FALSE;

	aTimeSigL = FirstSubLINK(timeSigL);
	for ( ; aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL))
		if (TimeSigSEL(aTimeSigL)) {
			LinkVALID(timeSigL) = FALSE;
			aTimeSig = GetPATIMESIG(aTimeSigL);
			aTimeSig->visible = FALSE;
			aTimeSig->selected = FALSE;
			aTimeSig->subType = DFLT_TSTYPE;
			aTimeSig->numerator = config.defaultTSNum;
			aTimeSig->denominator = config.defaultTSDenom;

			UpdateBFTSStaff(timeSigL,TimeSigSTAFF(aTimeSigL),
										DFLT_TSTYPE,config.defaultTSNum,config.defaultTSDenom);
			didAnything = TRUE;
		}

	return didAnything;
}


/*
 * If deletion has remove all subObjs of timeSigL, translate following
 * objects to the left by the previous width of the timeSig, and remove
 * any graphics which may have been attached to it.
 */

static void RemoveBFTimeSig(Document *doc, LINK timeSigL, DDIST width)
{
	DDIST change,timeSigWidth;
	Boolean isTimeSig;

	isTimeSig = BFTimeSigExists(timeSigL);

	if (!isTimeSig) {
		timeSigWidth = width;

		change = -timeSigWidth;
		ChangeSpaceBefFirst(doc, timeSigL, change, FALSE);
		DSRemoveGraphic(doc, timeSigL);
		DSRemoveTempo(doc, timeSigL);
	}
}

/* ------------------------------------------------------------- DelTimeSigBeforeFirst -- */
/* Delete the initial time signature <pL>. */

static Boolean DelTimeSigBefFirst(Document *doc, LINK pL)
{
	Boolean didAnything; DDIST tsWidth;
		
	tsWidth = GetTSWidth(pL);
	didAnything = InvisifyBFTimeSig(doc,pL);
	RemoveBFTimeSig(doc,pL,tsWidth);

	return didAnything;
}


/* ---------------------------------------------------------------------- FixDelChords -- */
/* Fix up all chords (including those that aren't really chords any more because
they have only one note left!) in the given Sync that have had notes deleted,
as indicated by voiceChanged[]. */

static void FixDelChords(Document *doc, LINK syncL, Boolean voiceChanged[])
{
	register short v;
	short	nInChord[MAXVOICES+1];
	PANOTE	aNote;
	LINK	aNoteL;
	
	for (v = 1; v<=MAXVOICES; v++)
		nInChord[v] = 0;
		
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		nInChord[aNote->voice]++;
	}
		
	for (v = 1; v<=MAXVOICES; v++)
		if (voiceChanged[v]) {
			if (nInChord[v]==1)
				FixSyncForNoChord(doc, syncL, v, NULL);
			else if (nInChord[v]>1)
				FixSyncForChord(doc, syncL, v, FALSE, 0, 0, NULL);	/* ??NOT CLEAR WHAT 4TH PARM. SHOULD BE */
		}
}


/* -------------------------------------------------------------------- FixDelGRChords -- */
/* Fix up all grace chords (including those that aren't really chords any more
because they have only one note left!) in the given GRSync that have had notes
deleted, as indicated by voiceChanged[]. */

static void FixDelGRChords(Document *doc, LINK grSyncL, Boolean voiceChanged[])
{
	register short v;
	short		nInChord[MAXVOICES+1];
	PAGRNOTE	aGRNote;
	LINK		aGRNoteL;
	
	for (v = 1; v<=MAXVOICES; v++)
		nInChord[v] = 0;
		
	aGRNoteL = FirstSubLINK(grSyncL);
	for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		nInChord[aGRNote->voice]++;
	}
		
	for (v = 1; v<=MAXVOICES; v++)
		if (voiceChanged[v]) {
			if (nInChord[v]==1)
				FixGRSyncForNoChord(doc, grSyncL, v, NULL);
			else if (nInChord[v]>1)
				FixGRSyncForChord(doc, grSyncL, v, FALSE, 0, 0, NULL);	/* ??NOT CLEAR WHAT 4TH PARM. SHOULD BE */
		}
}


/* ------------------------------------------------------------ FixAccsForNoTie -- */
/* Assuming the given tie-subtype Slur is about to be deleted, if it crosses a
barline, add accidentals to notes in its last Sync that are redundant with the
tie present (because of the rule that accidentals on tied notes carry across 
following barlines). */

void FixAccsForNoTie(Document *doc,
						LINK tiesetL)	/* Slur object of subtype tie */
{
	LINK lastSyncL, lastMeasL, aNoteL, bNoteL, firstSyncL;
	
	if (!SlurCrossSYS(tiesetL)) {
		lastSyncL = SlurLASTSYNC(tiesetL);
		lastMeasL = SSearch(lastSyncL, MEASUREtype, GO_LEFT);
		if (!IsAfter(lastMeasL, SlurFIRSTSYNC(tiesetL))) {
			/*
			 * The tie(s) crosses a barline. For each note in its last Sync that
			 * doesn't already have an explicit accidental, find its effective
			 * accidental and (if it's not a natural) make it explicit. It's
			 * important to leave existing explicit accidentals because the tie
			 * might involve a spelling change.
			 */
			aNoteL = FirstSubLINK(SlurLASTSYNC(tiesetL));
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteACC(aNoteL)==0 && NoteVOICE(aNoteL)==SlurVOICE(tiesetL)) {
					if (FirstTiedNote(lastSyncL, aNoteL, &firstSyncL, &bNoteL))
						NoteACC(aNoteL) = EffectiveAcc(doc, firstSyncL, bNoteL);
						if (NoteACC(aNoteL)==AC_NATURAL) NoteACC(aNoteL) = 0;
				}
			}
		}
	}
}


/* =========================================== DeleteSelection and associated routines == */

static enum {
	RECALC_NONE,
	RECALC_ONEMEAS,
	RECALC_ALL
} E_RecalcItems;

/* -------------------------------------------------------------------- DelSelPrepare -- */
/* Prepare for DeleteSelection: search for firstMeasL, etc.; decide what needs to
be updated; prepare beamsets and slurs; and prepare context. */

static Boolean DelSelPrepare(
				register Document *doc,
				LINK *firstMeasL,	/* First Measure of the score */
				LINK *prevMeasL,	/* Measure just before doc->selStartL */
				LINK *startL,		/* Link just before doc->selStartL */
				Boolean *noResp,	/* Returns TRUE if nothing deleted that could need respacing */
				short *fixTimes,
				Boolean *noUpMeasNums
				)
{
	register LINK pL;
	Boolean		upMeasNums;
	
	if (doc->selStartL==doc->selEndL) return FALSE;

	/* Set up Measure links, start links, and the undo Record. */
	
	*firstMeasL = LSSearch(doc->headL, MEASUREtype, 1, GO_RIGHT, FALSE);
	*prevMeasL = LSSearch(LeftLINK(doc->selStartL), MEASUREtype, 1, GO_LEFT, FALSE);
	*startL = LeftLINK(doc->selStartL);
	
	/* Don't respace if the only symbols deleted are type J_D symbols. */
	
	*noResp = TRUE;
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (JustTYPE(pL)!=J_D) {
			*noResp = FALSE;
			break;
		}
		
	/* Don't recompute timeStamps of Syncs and Measures if no notes, Measures, or time
		signatures are being deleted. Time signatures can matter because of whole-measure
		rests: in fact, deleting one can affect durations far past what's deleted! */
	 	
	*fixTimes = RECALC_NONE;
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL)) {
			if (SyncTYPE(pL) || MeasureTYPE(pL)) *fixTimes = RECALC_ONEMEAS;			
			if (TimeSigTYPE(pL)) { *fixTimes = RECALC_ALL; break; }
		}
	
	/* Don't update Measure numbers if the range being deleted is entirely within
		one Measure and the Measure will still contain at least one Sync or GRSync. */
		
	upMeasNums = TRUE;
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && MeasureTYPE(pL)) goto upMNDone;
	for (pL=RightLINK(*prevMeasL); pL!=doc->selStartL; pL=RightLINK(pL))
		if (SyncTYPE(pL) || GRSyncTYPE(pL)) { upMeasNums = FALSE; goto upMNDone; }
	for (pL=doc->selEndL; pL && !(MeasureTYPE(pL) || J_STRUCTYPE(pL)); pL=RightLINK(pL))
		if (SyncTYPE(pL) || GRSyncTYPE(pL)) { upMeasNums = FALSE; break; }
upMNDone:
	*noUpMeasNums = !upMeasNums;

	/* Remove slurs which extend past selection range, remove all beams partly
		or entirely within selection, and fix up the context for accidentals,
		clefs, keysig, timesigs & dynamics prior to actually deleting anything. */

	FixDelBeams(doc);
	FixDelGRBeams(doc);
	FixDelSlurs(doc);
	FixDelContext(doc);
	ExtendJDRange(doc);

	return TRUE;
}


/* --------------------------------------------------------------------- DelSelCleanup -- */
/* DelSelCleanup does:
		Reset selection range,
		Fix selection for selected objects outside of range,
		Fix up Measure rects and beams,
		Fix whole rests/whole Measure rest statuses,
		Update Measure numbers and <fakeMeas> statuses,
		Update the table of voice numbers,
		Update p_times of Syncs. */

static void DelSelCleanup(
				register Document *doc,
				LINK /*startL*/,
				register LINK newSelL,
				register LINK firstMeasL,
				LINK prevMeasL,			/* Measure before start of delete or NILINK */
				Boolean content,
				Boolean didAnything,
				short fixTimes,
				Boolean noUpMeasNums
				)
{
	LINK pL, endL;
	
	/* If any subObjs or objs were actually deleted, determine the
		correct location and set selStartL and selEndL. */

	if (didAnything) {
		if (InDataStruct(doc, newSelL, MAIN_DSTR)) {
			if (content) {
				if (IsAfter(newSelL, RightLINK(firstMeasL)))
					doc->selStartL = doc->selEndL = RightLINK(firstMeasL);
				else
					doc->selStartL = doc->selEndL = newSelL;
			}
			else {
				firstMeasL = LSSearch(doc->headL, MEASUREtype, 1, FALSE, FALSE);
				if (firstMeasL) {
					if (IsAfter(newSelL, firstMeasL))
						doc->selStartL = doc->selEndL = RightLINK(firstMeasL);
					else
						doc->selStartL = doc->selEndL = newSelL;
				}
				else
					doc->selStartL = doc->selEndL = newSelL;
			}
		}
		else 
			MayErrMsg("DelSelCleanup: newSelL = %ld not in object list.",
						(long)newSelL);
	}

	/* Fix the measSizeRect.rights of the Measures whose width changes;
		post-process the beamsets on the boundary of the selection range,
		and constrain the selRange to be consistent with the actual range
		of selected objects. */

	if (content && prevMeasL) FixDelMeasures(doc, prevMeasL);
	PostFixDelBeams(doc);
	BoundSelRange(doc);
	
	/*
	 * Fix status of whole/whole-measure rests in every measure affected by the
	 * delete. If deletion was before the first measure of any system, since
	 * no selection that extends from such a point into a measure can be deleted,
	 * we don't need to do anything at all.
	 */

	if (prevMeasL) {
		endL = EndMeasSearch(doc, newSelL);
		for (pL = prevMeasL; pL!=endL; pL = EndMeasSearch(doc, pL))
			FixWholeRests(doc, pL);
	}
	
	if (!noUpMeasNums) UpdateMeasNums(doc, prevMeasL);
	
	if (content) UpdateVoiceTable(doc, TRUE);		/* If changing structure, can't do this yet */
	
	if (fixTimes==RECALC_ALL) FixTimeStamps(doc, prevMeasL, NILINK);
	else if (fixTimes==RECALC_ONEMEAS) FixTimeStamps(doc, prevMeasL, prevMeasL);
}


/* ------------------------------------------------------------------- DeleteSelection -- */
/* Delete the current selection.  This routine needs to be smart but not _too_
smart, since we refuse to call it unless the selection is continuous in any
given voice. It may not have System, Staff, and Measure objects  selected, but
GetContext should take care of them.

Note that, although the symbols to be deleted are all in the selection range
[doc->selStartL,doc->selEndL), this does NOT mean that all, or even any of,
the nodes in that range will be deleted, since they might not have all of
their subobjects selected. Less obvious, as a side effect of deleting the
selection, DeleteSelection may actually delete one or more nodes just before
or after the range--e.g., a beamset all of whose notes are being deleted.
However, it always leaves doc->selStartL=doc->selEndL set to a node just after
the deleted range and that's still in the object list.

To keep the object list consistent, many types of objects and subobjects that are
not selected need to be updated or even deleted: Graphics and note modifiers
attached to objects being deleted, slurs whose endpoints are being deleted,
beamsets, tuplets, and octave signs any of whose elements are being deleted, etc.
Also, context information in the object list must be updated, the selection range
must be made consistent (as already described), the voice table must be updated,
and so on. We handle some of this ahead of time in DelSelPrepare (beamsets, slurs,
context); some "on the fly" in DeleteSelection's main loop (tuplets, octave signs,
etc.); and some afterwards in DelSelCleanup (selection range, voice numbers, more
on beamsets, etc.).

DeleteSelection is called both for ordinary delete operations on the content of
the score, and to delete parts. The main difference is that in the former case
objects before the initial (invisible) measure are just set invisible and not
actually deleted. */

void DeleteSelection(
		register Document *doc,
		Boolean content,		/* TRUE=deleting content (normal), else structure */
		Boolean *noResp		/* Returns TRUE if nothing deleted that could need respacing */
		)
{
	register LINK pL;
	LINK		newSelL, firstMeasL, rightL,
				startL, firstSyncL, lastSyncL, prevMeasL;
	register Boolean didAnything=FALSE;
	short		fixTimes;
	Boolean 	noUpMeasNums;


	if (!DelSelPrepare(doc, &firstMeasL, &prevMeasL, &startL, noResp,
								&fixTimes, &noUpMeasNums))
			return;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = rightL) {

		rightL = RightLINK(pL);			/* Save right link in case delete entire node */
		if (LinkSEL(pL))
			switch (ObjLType(pL)) {

			case STAFFtype: {
				PASTAFF	pSub; LINK subL, tempL;
					
			/* Get remaining nEntries of staff. If none, error; called by DeletePart
				which always expects at least one staff (=>connect) remaining. */
					subL = FirstSubLINK(pL);
					for ( ; subL; subL=NextSTAFFL(subL)) {
						pSub = GetPASTAFF(subL);
						if (pSub->selected) LinkNENTRIES(pL)--;
					}
					if (!LinkNENTRIES(pL)) {
						MayErrMsg("DeleteSelection: entire STAFF obj selected at %ld",
									(long)pL);
						break;
					}

			/* If any subObjs left, traverse the subList and delete selected
				subObjects. */
					subL = FirstSubLINK(pL);
					while (subL) {
						pSub = GetPASTAFF(subL);
						if (pSub->selected) {
							tempL = NextSTAFFL(subL);
							RemoveLink(pL, STAFFheap, FirstSubLINK(pL), subL);
							HeapFree(STAFFheap,subL);
							subL = tempL;
						}
						else subL = NextSTAFFL(subL);
					}
				}
				break;

			case CONNECTtype: {
				PACONNECT pSub; LINK subL, tempL;
					
			/* Get remaining nEntries of connect. If none, error; called by DeletePart
				which always expects at least one staff (=>connect) remaining. */
					subL = FirstSubLINK(pL);
					newSelL = RightLINK(pL);
					for ( ; subL; subL=NextCONNECTL(subL)) {
						pSub = GetPACONNECT(subL);
						if (pSub->selected) LinkNENTRIES(pL)--;
					}
					if (!LinkNENTRIES(pL)) {
						MayErrMsg("DeleteSelection: entire CONNECT obj selected at %ld", (long)pL);
						break;
					}

			/* If any subObjs left, traverse the subList and delete selected
				subObjects. */
					subL = FirstSubLINK(pL);
					while (subL) {
						pSub = GetPACONNECT(subL);
						if (pSub->selected) {
							tempL = NextCONNECTL(subL);
							RemoveLink(pL, CONNECTheap, FirstSubLINK(pL), subL);
							HeapFree(CONNECTheap,subL);
							didAnything = TRUE;
							subL = tempL;
						}
						else subL = NextCONNECTL(subL);					
					}
				}
				break;
			
			case MEASUREtype: {
					PMEASURE	pObj, lMeasure, rMeasure; PAMEASURE pSub; 
					LINK subL, tempL;
					
					/* We assume the only case of deleting measures we have to handle is
						where all subobjs are being deleted; the Clear function guarantees
						this (as of v.1.01), but ??I'm less sure of other calls to this. */

					/* Get remaining nEntries of measure. */
					pObj = GetPMEASURE(pL);
					subL = FirstSubLINK(pL);
					for ( ; subL; subL=NextMEASUREL(subL)) {
						pSub = GetPAMEASURE(subL);
						if (pSub->selected) LinkNENTRIES(pL)--;
					}

					/* If no remaining nEntries, fix the lMeasure and rMeasure fields of
						neighboring measures and delete the entire object. */
					if (!LinkNENTRIES(pL)) {										/* All of them being deleted? */
						newSelL = RightLINK(pL);									/* Yes, delete entire obj */
						pObj = GetPMEASURE(pL);
						if (pObj->lMeasure) {
							lMeasure = GetPMEASURE(pObj->lMeasure);
							lMeasure->rMeasure = pObj->rMeasure;
						}
						if (pObj->rMeasure) {
							rMeasure = GetPMEASURE(pObj->rMeasure);
							rMeasure->lMeasure = pObj->lMeasure;
						}
						InvalMeasure(pL,ANYONE);
						didAnything = DeleteWhole(doc, pL);
						break;
					}

					/* If any subObjs left, traverse the subList and delete selected 
						subObjects. */
					subL = FirstSubLINK(pL);
					while (subL) {
						pSub = GetPAMEASURE(subL);
						if (pSub->selected) {
							tempL = NextMEASUREL(subL);
							RemoveLink(pL, MEASUREheap, FirstSubLINK(pL), subL);
							HeapFree(MEASUREheap,subL);
							subL = tempL;
							didAnything = TRUE;
						}
						else subL = NextMEASUREL(subL);
					}
					DSObjRemGraphic(doc, pL);
					DSObjRemTempo(doc, pL);
					DSObjRemEnding(doc, pL);
				}
				break;

			case PSMEAStype: {
					PAPSMEAS pSub; 
					LINK subL, tempL;
					
					/* Deletion of a pseudomeasure by means of DoClear calls
						SelAllMeasSubobjs() which ensures that the only case
						to be handled will be that which calls DeleteWhole. */

			/* Get remaining nEntries of pseudomeasure. */
					subL = FirstSubLINK(pL);
					for ( ; subL; subL=NextPSMEASL(subL)) {
						pSub = GetPAPSMEAS(subL);
						if (pSub->selected) LinkNENTRIES(pL)--;
					}

			/* If no remaining nEntries, delete the entire object. */
					if (!LinkNENTRIES(pL)) {										/* All of them being deleted? */
						newSelL = RightLINK(pL);									/* Yes, delete entire obj */
						didAnything = DeleteWhole(doc, pL);
						break;
					}

			/* If any subObjs left, traverse the subList and delete selected 
				subObjects. */
					subL = FirstSubLINK(pL);
					while (subL) {
						pSub = GetPAPSMEAS(subL);
						if (pSub->selected) {
							tempL = NextPSMEASL(subL);
							RemoveLink(pL, PSMEASheap, FirstSubLINK(pL), subL);
							HeapFree(PSMEASheap,subL);
							subL = tempL;
							didAnything = TRUE;
						}
						else subL = NextPSMEASL(subL);
					}
				}
				DSObjRemEnding(doc, pL);
				break;

			case CLEFtype: {
					PACLEF pSub; LINK	subL, tempL;

					/* Handle deletion of initial clef. */
					newSelL = RightLINK(pL);			
					if (content && IsAfter(pL, firstMeasL)) {
						didAnything = DelClefBefFirst(doc, pL, &newSelL);
						break;
					}
					
					/* Get remaining nEntries of keySig. If none, delete entire object. */
					subL = FirstSubLINK(pL);
					for ( ; subL; subL=NextCLEFL(subL)) {
						pSub = GetPACLEF(subL);
						if (pSub->selected) LinkNENTRIES(pL)--;
					}
					if (!LinkNENTRIES(pL)) 
						{	didAnything = DeleteWhole(doc, pL); break; }
						
					/* If any subObjs left, traverse the subList and delete selected 
						subObjects. */
					subL = FirstSubLINK(pL);
					while (subL) {
						pSub = GetPACLEF(subL);
						if (pSub->selected) {
							tempL = NextCLEFL(subL);
							RemoveLink(pL, CLEFheap, FirstSubLINK(pL), subL);
							HeapFree(CLEFheap,subL);
							didAnything = TRUE;
							subL = tempL;
						}
						else subL = NextCLEFL(subL);
					}
					DSObjRemGraphic(doc, pL);
					DSObjRemTempo(doc, pL);
				}
				break;

			case KEYSIGtype: {
					PAKEYSIG	pSub; LINK subL, tempL;

					/* Handle deletion of initial keySig. */
					newSelL = RightLINK(pL);
					if (content && IsAfter(pL, firstMeasL)) 
						{ didAnything = DelKeySigBefFirst(doc, pL); break; }
					
					/* Get remaining nEntries of keySig. If none, delete entire object. */
					subL = FirstSubLINK(pL);
					for ( ; subL; subL=NextKEYSIGL(subL)) {
						pSub = GetPAKEYSIG(subL);
						if (pSub->selected) LinkNENTRIES(pL)--;
					}
					if (!LinkNENTRIES(pL)) 
						{	didAnything = DeleteWhole(doc, pL); break; }

					/* If any subObjs left, traverse the subList and delete selected 
						subObjects. */
					subL = FirstSubLINK(pL);
					while (subL) {
						pSub = GetPAKEYSIG(subL);
						if (pSub->selected) {
							tempL = NextKEYSIGL(subL);
							FixStfBeforeFirst(doc,pL,subL);
							RemoveLink(pL, KEYSIGheap,FirstSubLINK(pL),subL);
							HeapFree(KEYSIGheap,subL);
							didAnything = TRUE;
							subL = tempL;
						}
						else subL = NextKEYSIGL(subL);
					}
					
					/* If object is not deleted on all staves, context updating may require
						replacing that subobj with one requiring more or less space (i.e., with
						more or fewer sharps/flats): fix the width of the reserved area here. */
						
					subL = FirstSubLINK(pL);
					for ( ; subL; subL=NextKEYSIGL(subL))
						FixStfBeforeFirst(doc,pL,subL);
	
					DSObjRemGraphic(doc, pL);
					DSObjRemTempo(doc, pL);
				}
				break;

			case TIMESIGtype: {
				PATIMESIG pSub; LINK subL, tempL;	
					
					/* Handle deletion of initial timeSig. */
					newSelL = RightLINK(pL);
					if (content && IsAfter(pL, firstMeasL)) 
						{ didAnything = DelTimeSigBefFirst(doc, pL); break; }

					/* Get remaining nEntries of timeSig. If none, delete entire object. */
					subL = FirstSubLINK(pL);
					for ( ; subL; subL=NextTIMESIGL(subL)) {
						pSub = GetPATIMESIG(subL);
						if (pSub->selected) LinkNENTRIES(pL)--;
					}
					if (!LinkNENTRIES(pL)) 
						{	didAnything = DeleteWhole(doc, pL); break; }
						
					/* If any subObjs left, traverse the subList and delete selected
						subObjects. */
					subL = FirstSubLINK(pL);
					while (subL) {
						pSub = GetPATIMESIG(subL);
						if (pSub->selected) {
							tempL = NextTIMESIGL(subL);
							RemoveLink(pL, TIMESIGheap,FirstSubLINK(pL),subL);
							HeapFree(TIMESIGheap,subL);
							didAnything = TRUE;
							subL = tempL;
						}
						else subL = NextTIMESIGL(subL);
					}
					DSObjRemGraphic(doc, pL);
					DSObjRemTempo(doc, pL);
				}
				break;

			case SYNCtype: {
				PANOTE pSub; LINK pSubL,qSubL,tempL; short v;
				Boolean delOct=TRUE, voiceChanged[MAXVOICES+1];

					/* Get remaining nEntries of sync. If none, delete entire
						object. */
					pSubL = FirstSubLINK(pL);
					newSelL = RightLINK(pL);
					for ( ; pSubL; pSubL=NextNOTEL(pSubL)) {
						pSub = GetPANOTE(pSubL);
						if (pSub->selected)
							LinkNENTRIES(pL)--;
					}
					if (!LinkNENTRIES(pL)) 
						{ didAnything = DeleteWhole(doc, pL); break; }

					/* If any subObjs left, traverse the subList and delete
						selected subObjects. Check each note for tuplets, ottavas,
						note modifiers and dynamics, handling any associated with
						the note. */

					for (v = 0; v<=MAXVOICES; v++)
						voiceChanged[v] = FALSE;
					pSubL = FirstSubLINK(pL);
					while (pSubL) {
						tempL = NextNOTEL(pSubL);
						pSub = GetPANOTE(pSubL);
						if (pSub->selected) {
							voiceChanged[pSub->voice] = TRUE;						
							if (pSub->inTuplet)
								DelTupletForSync(doc, pL, pSubL);
							if (pSub->inOttava) {
								if (pSub->inChord) {
									qSubL = FirstSubLINK(pL);
									for ( ; qSubL ; qSubL = NextNOTEL(qSubL))
										if (NoteVOICE(qSubL)==NoteVOICE(pSubL) && !NoteSEL(qSubL))
											delOct = FALSE;
								}
								if (delOct)
									DelOttavaForSync(doc, pL, pSubL);
							}
							if (pSub->firstMod)
								DelModsForSync(pL, pSubL);
							DSRemoveDynamic(doc, pL, NoteSTAFF(pSubL));
							RemoveLink(pL, NOTEheap, FirstSubLINK(pL), pSubL);
							HeapFree(NOTEheap,pSubL);
							didAnything = TRUE;
						}
						pSubL = tempL;
					}
					DSObjRemGraphic(doc, pL);
					DSObjRemTempo(doc, pL);
					DSObjRemEnding(doc, pL);

					/* Update aspects of any chords contained in pL. Fix tie indices for
						chords tied with multiple ties, and fix stems, otherStemSides, etc.
						of notes in chords with notes deleted. */
					FixDelChords(doc, pL, voiceChanged);
				}
				break;

			case GRSYNCtype: {
				PAGRNOTE pSub; LINK subL, tempL; short v;
				Boolean voiceChanged[MAXVOICES+1];

					/* Get remaining nEntries of grSync. If none, delete entire object. */
					subL = FirstSubLINK(pL);
					newSelL = RightLINK(pL);
					for ( ; subL; subL=NextGRNOTEL(subL)) {
						pSub = GetPAGRNOTE(subL);
						if (pSub->selected) LinkNENTRIES(pL)--;
					}
					if (!LinkNENTRIES(pL)) 
						{ didAnything = DeleteWhole(doc, pL); break; }

					/* If any subObjs left, traverse the subList and delete selected
						subObjects. */
					for (v = 0; v<=MAXVOICES; v++)
						voiceChanged[v] = FALSE;
					subL = FirstSubLINK(pL);
					while (subL) {
						tempL = NextGRNOTEL(subL);
						pSub = GetPAGRNOTE(subL);
						if (pSub->selected) {
							voiceChanged[pSub->voice] = TRUE;						
							RemoveLink(pL, GRNOTEheap, FirstSubLINK(pL), subL);
							HeapFree(GRNOTEheap,subL);
							didAnything = TRUE;
						}
						subL = tempL;
					}

					DSObjRemGraphic(doc, pL);

					/* Update aspects of any chords contained in pL. Fix stems, otherStemSides,
						 etc., of grace notes in chords with grace notes deleted. */
					FixDelGRChords(doc, pL, voiceChanged);
				}
				break;

			case BEAMSETtype:
			/* When we have the actual beam object, UnbeamRange should be called
				rather than Unbeam, since we do not need to check the endpoints for
				beams across them. Same for ottavas and tuplets. */

				didAnything = TRUE;
				newSelL = RightLINK(pL);
				firstSyncL = FirstInBeam(pL);
				lastSyncL = LastInBeam(pL);
				if (GraceBEAM(pL))
					GRUnbeamRange(doc, firstSyncL, RightLINK(lastSyncL), BeamVOICE(pL));
				else
					UnbeamRange(doc, firstSyncL, RightLINK(lastSyncL), BeamVOICE(pL));
				break;

			case OTTAVAtype:
				didAnything = TRUE;
				newSelL = RightLINK(pL);
				firstSyncL = FirstInOttava(pL);
				lastSyncL = LastInOttava(pL);
				UnOttavaRange(doc, firstSyncL, RightLINK(lastSyncL), OttavaSTAFF(pL));
				break;

			case TUPLETtype:
				didAnything = TRUE;
				newSelL = RightLINK(pL);
				firstSyncL = FirstInTuplet(pL);
				lastSyncL = LastInTuplet(pL);
				UntupleObj(doc, pL, firstSyncL, lastSyncL, TupletVOICE(pL));
				break;

			case DYNAMtype: {
				Boolean isHairpin;

					isHairpin = IsHairpin(pL);
					didAnything = TRUE;
					newSelL = RightLINK(pL);
					InvalObject(doc, pL, FALSE);
					
					if (isHairpin)
						DeleteOtherDynamic(doc, pL);
					DeleteNode(doc, pL);
					if (isHairpin)
						InvalMeasures(DynamFIRSTSYNC(pL),DynamLASTSYNC(pL),
							DynamicSTAFF(FirstSubLINK(pL)));
				}
				break;

			case RPTENDtype: {
				PRPTEND pRptEnd, startRpt, endRpt;
				
					didAnything = TRUE;
					newSelL = RightLINK(pL);
					pRptEnd = GetPRPTEND(pL);
					if (pRptEnd->startRpt) {
						startRpt = GetPRPTEND(pRptEnd->startRpt);
						startRpt->endRpt = NILINK;
					}
					if (pRptEnd->endRpt) {
						endRpt = GetPRPTEND(pRptEnd->endRpt);
						endRpt->startRpt = NILINK;
					}
					DeleteNode(doc, pL);
				}
				break;

			case SLURtype: {
				PSLUR 	pSlur;
				LINK		firstSyncL, lastSyncL, otherSlur;
				Boolean  lastIsSys, firstIsMeas, left, right;
				
					didAnything = TRUE;
					right = TRUE; left = FALSE;
					lastIsSys = firstIsMeas = FALSE;
					newSelL = RightLINK(pL);
					pSlur = GetPSLUR(pL);
					
					if (SlurTIE(pL)) FixAccsForNoTie(doc, pL);
					
					firstSyncL = pSlur->firstSyncL;
					if (MeasureTYPE(firstSyncL))
						firstIsMeas = TRUE;
					else
						FixSyncForSlur(firstSyncL, SlurVOICE(pL), SlurTIE(pL), right);
	 				lastSyncL = pSlur->lastSyncL;
	 				if (SystemTYPE(lastSyncL))
	 					lastIsSys = TRUE;
	 				else
						FixSyncForSlur(lastSyncL, SlurVOICE(pL), SlurTIE(pL), left);
					if (firstIsMeas) {
						otherSlur = XSysSlurMatch(pL);
						lastSyncL = SlurLASTSYNC(otherSlur);
						if (SystemTYPE(lastSyncL)) {
							firstSyncL = SlurFIRSTSYNC(otherSlur);
							FixSyncForSlur(firstSyncL, SlurVOICE(pL), SlurTIE(pL), right);
						}
					}
					if (lastIsSys) {
						otherSlur = XSysSlurMatch(pL);
						firstSyncL = SlurFIRSTSYNC(otherSlur);
						if (MeasureTYPE(firstSyncL)) {
							lastSyncL = SlurLASTSYNC(otherSlur);
							FixSyncForSlur(lastSyncL, SlurVOICE(pL), SlurTIE(pL), left);
						}
					}
					if (firstIsMeas || lastIsSys) 
						InvalMeasure(otherSlur, SlurSTAFF(otherSlur));

					DeleteNode(doc, pL);
					if ((firstIsMeas || lastIsSys) && otherSlur)
						DeleteNode(doc, otherSlur);
				}
				break;
				
			case GRAPHICtype: {
					didAnything = TRUE;
					newSelL = RightLINK(pL);

					/* Make sure graphics which extend outside systemRects
						are completely erased. */
					InvalObject(doc,pL,FALSE);

					/* ??We should also delete the string itself from the string library. */

					DeleteNode(doc, pL);
				}
				break;
				
			case TEMPOtype:				
				didAnything = TRUE;
				InvalObject(doc, pL, FALSE);
				newSelL = RightLINK(pL);
				DeleteNode(doc, pL);
				break;

			case SPACERtype:
				DSRemoveGraphic(doc, pL);
				didAnything = TRUE;
				newSelL = RightLINK(pL);
				DeleteNode(doc, pL);
				break;

			case ENDINGtype:
				didAnything = TRUE;
				newSelL = RightLINK(pL);
				DeleteNode(doc, pL);
				break;

			default:
				MayErrMsg("DeleteSelection: can't handle type=%ld at %ld",
							(long)ObjLType(pL), (long)pL);
				break;
			}
	}
	
	DelSelCleanup(doc, startL, newSelL, firstMeasL, prevMeasL, content,
						didAnything, fixTimes, noUpMeasNums);
}

