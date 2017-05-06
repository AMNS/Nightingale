/***************************************************************************
	FILE:	CrossSystem.c
	PROJ:	Nightingale
	DESC:	Routines for handling the cross-systemness of objects, either
			making cross-system objects non-cross-system or vice-versa.
			
	FixCrossSysSlurs		FixCrossSysBeams		FixCrossSysHairpins
	FixCrossSysOttavas		FixCrossSysEndings		FixCrossSysObjects
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

static Boolean MakeSlurCrossSys(Document *, LINK, LINK, LINK, LINK);
static void MakeSlurNoncrossSys(Document *, LINK, LINK);
static short FixCrossSysSlurs(Document *, LINK, LINK, short *, short *);
static Boolean MakeBeamCrossSys(Document *,LINK);
static LINK MakeBeamNoncrossSys(Document *, LINK, LINK);
static short FixCrossSysBeams(Document *, LINK, LINK, short *, short *);

static short FixCrossSysHairpins(Document *, LINK, LINK, short *, short *);
static short FixCrossSysDrawObjs(Document *, LINK, LINK, short *, short *);
static void MakeOctCrossSys(Document *, LINK, LINK, LINK);
static short FixCrossSysOttavas(Document *, LINK, LINK, short *, short *);
static short FixCrossSysEndings(Document *, LINK, LINK, short *, short *);

/* ---------------------------------------------------- MakeSlurCrossSys/NoncrossSys -- */
/* Given a "normal" slur that should now be cross-system (presumably because its
endpoints are now on different Systems), create the other (second) piece and insert
it into the data structure, and fill in the intermediate endpoint of each piece.
Also reset both pieces to default shape. Intended for use by routines that can move
Measures from one System to another, e.g., Reformat, MoveBarsUp/Down. */

static Boolean MakeSlurCrossSys(Document *doc, LINK pL, LINK thisLastObj,
											LINK otherFirstObj, LINK otherLastObj)
{
	LINK otherSlurL, sysL, firstMeasL;
	
	/* Duplicate the slur and insert it on the second system immediately after
		that system's first invis measure */

	otherSlurL = DuplicateObject(SLURtype, pL, false, doc, doc, false);
	if (!otherSlurL) { NoMoreMemory(); return false; }
	sysL = LSSearch(otherLastObj,SYSTEMtype,ANYONE,GO_LEFT,false);
	firstMeasL = LSSearch(sysL,MEASUREtype,ANYONE,GO_RIGHT,false);
	InsNodeInto(otherSlurL, RightLINK(firstMeasL));

	SlurCrossSYS(pL) = true;
	SlurLASTSYNC(pL) = thisLastObj;
	SetAllSlursShape(doc, pL, true);
	
	LinkSOFT(otherSlurL) = true;
	SlurCrossSYS(otherSlurL) = true;
	SlurFIRSTSYNC(otherSlurL) = otherFirstObj;
	SlurLASTSYNC(otherSlurL) = otherLastObj;
	SetAllSlursShape(doc, otherSlurL, true);

	return true;
}

/* Given the two pieces of a cross-system slur that should no longer be cross-system
(presumably because its endpoints are now on the same System), make the first piece
point to the correct right end Sync, reset it to default shape, and delete the
second piece. Intended for use by routines that can move Measures from one System
to another, e.g., Reformat, MoveBarsUp/Down. */

static void MakeSlurNoncrossSys(Document *doc, LINK pL, LINK otherSlurL)
{
	SlurLASTSYNC(pL) = SlurLASTSYNC(otherSlurL);
	SlurCrossSYS(pL) = false;
	SetAllSlursShape(doc, pL, true);
	DeleteNode(doc, otherSlurL);
}


/* --------------------------------------------------------------------- MoveSlurEnd -- */
/* Move the right endpoint of the Slur or Tie in the given voice from one Sync to
another. */

static void MoveSlurEnd(short voice, LINK oldSyncL, LINK newSyncL, Boolean isTie);
static void MoveSlurEnd(short voice, LINK oldSyncL, LINK newSyncL, Boolean isTie)
{
	LINK aNoteL;
	        
	/* For the old right-end Sync/voice, clear all slurred- or tied-left flags. */
	aNoteL = FirstSubLINK(oldSyncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			if (isTie) NoteTIEDL(aNoteL) = false;
			else       NoteSLURREDL(aNoteL) = false;
		}
	}
	
	/* For the new right-end Sync/voice MainNote, set slurred- or tied-left flag. */
	aNoteL = FindMainNote(newSyncL, voice);
	if (isTie) NoteTIEDL(aNoteL) = true;
	else       NoteSLURREDL(aNoteL) = true;
}

/* --------------------------------------------------------------- FixCrossSysSlurs -- */
/* In the range [startL,endL), fix "cross-system-ness" and higher-level links of all
Slurs, as described in FixCrossSysObjects. Assumes cross links in the data structure
are valid! Since Nightingale allows Slurs to cross only one system break, we truncate
any that cross two or more.

Return value is the number of Slurs truncated. If it's non-zero, we also return in
parameters the first and last measures in which any are truncated. */

short FixCrossSysSlurs(Document *doc, LINK startL, LINK endL, short *pTruncFirstMeas,
								short *pTruncLastMeas)
{
	short v, nTrunc, thisMeas, truncFirstMeas=SHRT_MAX, truncLastMeas=0;
	LINK pL;
	LINK otherSlurL, firstSys, lastSys, secondSys, thirdSys, thisFirstObj,
			otherFirstObj, thisLastObj, otherLastObj, lastObjSys, oldLastObj;
	Boolean slurRemoved, slurTruncated;
	
	for (nTrunc = 0, v = 1; v<=MAXVOICES; v++) {
		if (!VOICE_MAYBE_USED(doc, v)) continue;
		for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
			if (SlurTYPE(pL) && SlurVOICE(pL)==v) {
				/*
				 *	We have a slur in the correct voice. There are three possible types:
				 *	not cross-system, first piece of cross-system, second piece of
				 *	cross-system.
				 */
				if (!SlurCrossSYS(pL)) {
					/*
					 * Non-cross-system slur. If it should be cross-system, be sure it
					 * ends in the system after the one it begins in, create its other
					 *	piece, and make this piece cross-system.
					 */
			 		firstSys = LSSearch(SlurFIRSTSYNC(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
			 		lastSys = LSSearch(SlurLASTSYNC(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
					if (firstSys!=lastSys) {
						slurRemoved = slurTruncated = false;
						secondSys = LinkRSYS(firstSys);
						thisLastObj = firstSys;
						otherFirstObj = LSSearch(secondSys,MEASUREtype,ANYONE,GO_RIGHT,false);
				 		
				 		/*
				 		 * If slur ends on the system after <firstSys>, no problem. Otherwise
				 		 * make it end with the last Sync in the voice on that System, if
				 		 * there is one; if not, this is a pathological case, and the most
				 		 * important thing is avoiding crashes.
				 		 *
				 		 * Should be handled by existing utils: DeleteXSysSlur or DeleteNonXSysSLur,
				 		 * depending on whether it is xsys or not.
				 		 * Delete.c: code to delete slurs.
				 		 * Ex: score 3 barL, notes in all m in top stf, stf 2: note in bL 1 & 3,
				 		 * none in bL 2; stf 2, slur 1 -> 3, reformat to 1 meas/sys.
				 		 */

						if (lastSys==secondSys)
							otherLastObj = SlurLASTSYNC(pL);
						else {
							thirdSys = LinkRSYS(secondSys);					/* must exist */
							otherLastObj = LVSearch(thirdSys, SYNCtype, v, GO_LEFT, false);
							lastObjSys = LSSearch(otherLastObj,SYSTEMtype,ANYONE,GO_LEFT,false);

							/* Pathological case: delete the slur */
							if (lastObjSys!=secondSys) {
								DeleteXSysSlur(doc, pL);
								slurRemoved = true;
							}
							else {
								slurTruncated = true;
								thisMeas = GetMeasNum(doc, pL);
								truncFirstMeas = n_min(thisMeas, truncFirstMeas);
								truncLastMeas = n_max(thisMeas, truncLastMeas);
								nTrunc++;
							}
						}
						if (!slurRemoved) {
							oldLastObj = SlurLASTSYNC(pL);
							MakeSlurCrossSys(doc,pL,thisLastObj,otherFirstObj,otherLastObj);
//Browser(doc,doc->headL, doc->tailL);
							MoveSlurEnd(v, oldLastObj, otherLastObj, SlurTIE(pL));
//Browser(doc,doc->headL, doc->tailL);
						}
					}
				}	

				else if (SlurFirstIsSYSTEM(pL)) {
					/*
					 *	First piece of cross-system slur. If it should be non-cross-system,
					 *	delete its other piece and make this one non-cross-system.
					 */
					otherSlurL = XSysSlurMatch(pL);
					if (!otherSlurL)
						MayErrMsg("FixCrossSysSlurs: can't find 2nd half of slur at %ld",
									(long)pL);
					if (SameSystem(SlurFIRSTSYNC(pL), SlurLASTSYNC(otherSlurL)))
						MakeSlurNoncrossSys(doc, pL, otherSlurL);
					else {
					
						/* It should stay cross-system. Update its System LINK. */
						
						thisLastObj = LSSearch(SlurFIRSTSYNC(pL), SYSTEMtype, ANYONE,
				 										GO_LEFT, false);
						SlurLASTSYNC(pL) = thisLastObj;						
					}
				}
				else {
					/*
					 * Second piece of cross-system slur. We don't have to consider the
					 * possibility that it needs to be made non-cross-system, since we're
					 * traversing the range that needs work from left to right, and any second
					 *	piece of a cross-system slur that needed to be made non-cross-system
					 *	would already have been handled before we got to it. But we do have to
					 * update its Measure LINK.??AND be sure it's in the system following
					 * the one the first piece is in.
					 */
					 thisFirstObj = LSSearch(SlurLASTSYNC(pL), SYSTEMtype, ANYONE, GO_LEFT,
					 									false);
					 thisFirstObj = LSSearch(thisFirstObj,MEASUREtype, ANYONE, GO_RIGHT,
					 									false);
					 SlurFIRSTSYNC(pL) = thisFirstObj;
				}
			}
		}
	}
	
	*pTruncFirstMeas = truncFirstMeas;
	*pTruncLastMeas = truncLastMeas;
	return nTrunc;
}


/* -------------------------------------------------- MakeBeamCrossSys/NoncrossSys -- */
/* Similar to MakeSlurCrossSys/NoncrossSys, except that - since Beamsets are more
complex than slurs in terms of data structure - it's easier to remove the existing
Beamset(s) and create new one(s) than to fix up the existing ones. This may result
in the user losing tweaking, but too bad. */

static Boolean MakeBeamCrossSys(Document *doc, LINK pL)
{
	LINK firstSyncL,lastSyncL,firstSys,lastSys,secondSys,
	qL,prevL,qL2,prevL2,noteBeamL,beamL1,beamL2;
	short i,v,nInBeamOrig,nInBeam1,nInBeam2;
	PANOTEBEAM aNoteBeam;
	Boolean truncate=false;
	
	firstSyncL = FirstInBeam(pL);
	lastSyncL = LastInBeam(pL);
	v = BeamVOICE(pL);
	nInBeamOrig = LinkNENTRIES(pL);

 	firstSys = LSSearch(FirstInBeam(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
 	lastSys = LSSearch(LastInBeam(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
	if (firstSys==lastSys) return false;
	secondSys = LinkRSYS(firstSys);

	/*
	 * Find the last element of the existing Beamset that's in the first System
	 * and the last element that's in the second System; then remove the existing
	 * Beamset and create new one(s).
	 */
	
	noteBeamL = FirstSubLINK(pL);
	prevL = qL = firstSyncL;
	for (i=0; i<LinkNENTRIES(pL); i++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		aNoteBeam = GetPANOTEBEAM(noteBeamL);
		prevL = qL;
		qL = aNoteBeam->bpSync;
		if (!SameSystem(qL, firstSys)) break;
	}
	nInBeam1 = i;

	prevL2 = qL2 = qL;
	for (i=0; i<LinkNENTRIES(pL); i++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		aNoteBeam = GetPANOTEBEAM(noteBeamL);
		prevL2 = qL2;
		qL2 = aNoteBeam->bpSync;
		if (!SameSystem(qL2, secondSys))
			break;
	}
	nInBeam2 = i;

	RemoveBeam(doc, pL, v, false);

	/*
	 * For now, don't rebeam either System unless it has at least 2 beamable notes.
	 * But eventually we should handle cross-system beams with only 1 note in
	 * either piece.
	 */
	if (nInBeam1>1) {
		beamL1 = CreateNonXSysBEAMSET(doc, firstSyncL, RightLINK(prevL), v, nInBeam1,
										false, false, doc->voiceTab[v].voiceRole);
		if (nInBeam2>1) {
			beamL2 = CreateNonXSysBEAMSET(doc, qL, RightLINK(prevL2), v, nInBeam2,
											false, false, doc->voiceTab[v].voiceRole);
			BeamCrossSYS(beamL1) = true;
			BeamFirstSYS(beamL1) = true;
			BeamCrossSYS(beamL2) = true;
			BeamFirstSYS(beamL2) = false;
		}
		else
			;
	}
	
	truncate = (nInBeam1+nInBeam2!=nInBeamOrig);
/*	if (truncate) fix stems for [RightLINK(prevL2), lastSyncL) here. */

	return truncate;
}

static LINK MakeBeamNoncrossSys(Document *doc, LINK pL, LINK otherBeamL)
{
	LINK newFirstSyncL, newLastSyncL, beamL=NILINK;
	short v, nInBeam;
	
	v = BeamVOICE(pL);

	newFirstSyncL = FirstInBeam(pL);
	newLastSyncL = LastInBeam(otherBeamL);

	RemoveBeam(doc, pL, v, false);	
	RemoveBeam(doc, otherBeamL, v, false);
		
	nInBeam = CountBeamable(doc, newFirstSyncL, RightLINK(newLastSyncL), v, false);
	if (nInBeam>=2)														/* Should always succeed */
		beamL = CreateBEAMSET(doc, newFirstSyncL, RightLINK(newLastSyncL), v,
										nInBeam, false, doc->voiceTab[v].voiceRole);
	
	return beamL;
}

/* --------------------------------------------------------------------- RebeamXStf -- */
/* Rebeam the notes/rests beamed by the two pieces of a cross-staff beam <beamL1>
and <beamL2>. Handles grace AND regular beams. */

static LINK RebeamXStf(Document *, LINK, LINK);
static LINK RebeamXStf(Document *doc, LINK beamL1, LINK beamL2)
{
	short voice, nInBeam1, nInBeam2;
	LINK firstSyncL1, lastSyncL1, firstSyncL2, lastSyncL2;

	voice = BeamVOICE(beamL1);

	nInBeam1 = LinkNENTRIES(beamL1);
	firstSyncL1 = FirstInBeam(beamL1);
	lastSyncL1 = LastInBeam(beamL1);
	nInBeam2 = LinkNENTRIES(beamL2);
	firstSyncL2 = FirstInBeam(beamL2);
	lastSyncL2 = LastInBeam(beamL2);

	if (GraceBEAM(beamL1)) {
		RemoveGRBeam(doc, beamL1, voice, false);
		RemoveGRBeam(doc, beamL2, voice, false);
		return CreateGRBEAMSET(doc, firstSyncL1, RightLINK(lastSyncL2), voice,
										nInBeam1+nInBeam2, false, false);
	}

	RemoveBeam(doc, beamL1, voice, false);
	RemoveBeam(doc, beamL2, voice, false);
	return CreateBEAMSET(doc, firstSyncL1, RightLINK(lastSyncL2), voice,
									nInBeam1+nInBeam2, false,
									doc->voiceTab[voice].voiceRole);
}

/* Given the two pieces of a cross-system Beamset that should be cross-system, check
that  the boundary between the two pieces is correct, i.e., that Syncs in the 1st piece
don't belong in the 2nd and vice-versa. If there's a problem, correct it by
rebeaming. */

static Boolean UpdateCrossSysBeam(Document *, LINK, LINK);
static Boolean UpdateCrossSysBeam(Document *doc, LINK pL, LINK otherBeamL)
{
	LINK aNoteBeamL; PANOTEBEAM aNoteBeam; short nInBeam, i;
	
	nInBeam = LinkNENTRIES(pL);
	aNoteBeamL = FirstSubLINK(pL);
	for (i = 0; i<nInBeam; i++, aNoteBeamL = NextNOTEBEAML(aNoteBeamL)) {
		aNoteBeam = GetPANOTEBEAM(aNoteBeamL);
		if (!SameSystem(pL, aNoteBeam->bpSync)) {
			RebeamXStf(doc, pL, otherBeamL);
			return false;
		}
	}
	
	nInBeam = LinkNENTRIES(otherBeamL);
	aNoteBeamL = FirstSubLINK(otherBeamL);
	for (i = nInBeam-1; i>=0; i--, aNoteBeamL = NextNOTEBEAML(aNoteBeamL)) {
		aNoteBeam = GetPANOTEBEAM(aNoteBeamL);
		if (!SameSystem(otherBeamL, aNoteBeam->bpSync)) {
			RebeamXStf(doc, pL, otherBeamL);
			return false;
		}
	}
	
	return true;
}


/* ---------------------------------------------------------------- FixCrossSysBeams -- */
/* In the range [startL,endL), fix "cross-system-ness" of all beams, as described in
FixCrossSysObjects. Assumes cross links in the data structure are valid! Since
Nightingale allows Beamsets to cross only one system break, we truncate any that
cross two or more.

Return value is the number of Beamsets truncated. If it's non-zero, we also return in
parameters the first and last measures in which any are truncated. */

#define CrossSysAndFirstSys(beamL)	 	( BeamCrossSYS(beamL) && BeamFirstSYS(beamL) )
#define CrossSysAndSecondSys(beamL)		( BeamCrossSYS(beamL) && !BeamFirstSYS(beamL) )

short FixCrossSysBeams(Document *doc, LINK startL, LINK endL, short *pTruncFirstMeas,
								short *pTruncLastMeas)
{
	short v, nTrunc, thisMeas, truncFirstMeas=SHRT_MAX, truncLastMeas=0;
	LINK pL, otherBeamL, nextL;
	
	for (nTrunc = 0, v = 1; v<=MAXVOICES; v++) {
		if (!VOICE_MAYBE_USED(doc, v)) continue;
		for (pL = startL; pL!=endL; pL = nextL) {
			nextL = RightLINK(pL);								/* Save link in case pL is deleted */
			if (BeamsetTYPE(pL) && BeamVOICE(pL)==v) {
				/*
				 *	We have a Beamset in the correct voice. Just as with slurs, there are
				 * three possible types: not cross-system, first piece of cross-system,
				 * second piece of cross-system.
				 */
				if (!BeamCrossSYS(pL)) {
					/*
					 * Non-cross-system Beamset. If it should be cross-system, be sure it
					 * ends in the system after the one it begins in, create its other
					 * piece, and make this piece cross-system.
					 */
				 	if (!SameSystem(FirstInBeam(pL), LastInBeam(pL)))
						if (MakeBeamCrossSys(doc, pL)) {
							thisMeas = GetMeasNum(doc, pL);
							truncFirstMeas = n_min(thisMeas, truncFirstMeas);
							truncLastMeas = n_max(thisMeas, truncLastMeas);
							nTrunc++;
						}
				}	
				else if (CrossSysAndFirstSys(pL)) {
				
				/* First piece of cross-system Beamset. If it should be non-cross-system,
				   delete its other piece and make this one non-cross-system. If it
				   should stay cross-system, check that the boundary between the two
				   pieces hasn't changed, i.e., that Syncs in the 1st piece don't belong
				   in the 2nd and vice-versa. */
				   
					otherBeamL = LVSearch(RightLINK(pL), BEAMSETtype, v, GO_RIGHT, false);
					if (!otherBeamL)
						MayErrMsg("FixCrossSysBeams: can't find 2nd piece for Beamset at %ld",
									(long)pL);
					if (SameSystem(FirstInBeam(pL), LastInBeam(otherBeamL)))
						MakeBeamNoncrossSys(doc, pL, otherBeamL);
					else
						UpdateCrossSysBeam(doc, pL, otherBeamL);
				}
				else
					/*
					 Second piece of cross-system Beamset. We don't have to consider the
					 possibility that it needs to be made non-cross-system, since we're
					 traversing the range that needs work from left to right, and any
					 second piece of a cross-system Beamset that needed to be made non-
					 cross-system would already have been handled before we got to it.
					 ??But we do have to be sure it ends in the system following the one
					 the first piece is in and, if not, truncate it.
					 */
					;
			}
		}
	}

	*pTruncFirstMeas = truncFirstMeas;
	*pTruncLastMeas = truncLastMeas;
	return nTrunc;
}


/* ------------------------------------------------------------ FixCrossSysHairpins -- */
/* In the range [startL,endL), fix "cross-system-ness" of all hairpins. We'd like to
do it as described in FixCrossSysObjects, but Nightingale does not allow cross-system
hairpins yet, so we just clip them to keep them on one system. It'd be better to
also add a second piece of the hairpin on the following system, even though they
can't be linked the way pieces of cross-system slurs and beams are.

Return value is the number of hairpins truncated. If it's non-zero, we also return in
parameters the first and last measures in which any are truncated. */

short FixCrossSysHairpins(Document *doc, LINK startL, LINK endL, short *pTruncFirstMeas,
									short *pTruncLastMeas)
{
	LINK pL, firstSys, lastSys, lastSync, dynamL;
	short staff, nTrunc, thisMeas, truncFirstMeas=SHRT_MAX, truncLastMeas=0;
	PADYNAMIC aDynamic; DDIST endxd;
	
	for (nTrunc = 0, pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==DYNAMtype && IsHairpin(pL)) {
			/*
			 *	If this hairpin crosses systems, clip it at the end of its first system:
			 * set its lastSync to the last Sync on its staff of the first system, and
			 * set its endxd so it extends to just short of the right edge of the system.
			 */
			 firstSys = LSSearch(DynamFIRSTSYNC(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
			 lastSys = LSSearch(DynamLASTSYNC(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
			if (firstSys!=lastSys) {
				dynamL = FirstSubLINK(pL);
				staff = DynamicSTAFF(dynamL);
				lastSync = LSSearch(LinkRSYS(firstSys), SYNCtype, staff, GO_LEFT, false);
				DynamLASTSYNC(pL) = lastSync;
				endxd = StaffLength(lastSync)-pt2d(4)-SysRelxd(lastSync);
 				aDynamic = GetPADYNAMIC(dynamL);
				aDynamic->endxd = endxd;
				nTrunc++;
				thisMeas = GetMeasNum(doc, pL);
				truncFirstMeas = n_min(thisMeas, truncFirstMeas);
				truncLastMeas = n_max(thisMeas, truncLastMeas);
			}
		}
	}
	
	*pTruncFirstMeas = truncFirstMeas;
	*pTruncLastMeas = truncLastMeas;
	return nTrunc;
}


/* ------------------------------------------------------------- FixCrossSysDrawObjs -- */
/* In the range [startL,endL), fix "cross-system-ness" of all GRDraw Graphics. We'd
like to do it as described in FixCrossSysObjects, but Nightingale does not allow
cross-system Graphics yet, so we just clip them to keep them on one system. It'd be
better to also add a second piece of the Graphics on the following system, even
though they can't be linked the way pieces of cross-system slurs and beams are.

Return value is the number of Graphics truncated. If it's non-zero, we also return in
parameters the first and last measures in which any are truncated. */

short FixCrossSysDrawObjs(Document *doc, LINK startL, LINK endL, short *pTruncFirstMeas,
									short *pTruncLastMeas)
{
	LINK pL, firstSys, lastSys, lastObj;
	short staff, nTrunc, thisMeas, truncFirstMeas=SHRT_MAX, truncLastMeas=0;
	PGRAPHIC pGraphic; DDIST endxd;
	
	for (nTrunc = 0, pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==GRAPHICtype && GraphicSubType(pL)==GRDraw) {
			/*
			 *	If this GRDraw Graphic crosses systems, clip it at the end of its first
			 * system: set its lastObj to the last object ??IS THIS SAFE? on its staff of the
			 * first system, and set its endxd so it extends to just short of the right
			 * edge of the system.
			 */
			 firstSys = LSSearch(GraphicFIRSTOBJ(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
			 lastSys = LSSearch(GraphicLASTOBJ(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
			if (firstSys!=lastSys) {
				staff = GraphicSTAFF(pL);
				lastObj = LeftLINK(LinkRSYS(firstSys));
				GraphicLASTOBJ(pL) = lastObj;
				endxd = StaffLength(lastObj)-pt2d(4)-SysRelxd(lastObj);
				pGraphic = GetPGRAPHIC(pL);
				pGraphic->info = endxd;
				thisMeas = GetMeasNum(doc, pL);
				truncFirstMeas = n_min(thisMeas, truncFirstMeas);
				truncLastMeas = n_max(thisMeas, truncLastMeas);
				nTrunc++;
			}
		}
	}
	
	*pTruncFirstMeas = truncFirstMeas;
	*pTruncLastMeas = truncLastMeas;
	return nTrunc;
}


/* ----------------------------------------------------------------- MakeOctCrossSys -- */
/* Nightingale doesn't handle cross-system octave signs yet, so just truncate the
octave sign at the end of the first system. */

static void MakeOctCrossSys(Document *doc, LINK octL, LINK firstSys,
										LINK /*lastSys*/									/* unused */
										)
{
	LINK qL, prevL, firstSyncL, lastSyncL, noteOctL;
	short i, s, octType, nInOttava1;
	PANOTEOTTAVA anoteOct;
	
	firstSyncL = FirstInOttava(octL);
	lastSyncL = LastInOttava(octL);
	s = OttavaSTAFF(octL);
	octType = OctType(octL);

	noteOctL = FirstSubLINK(octL);
	prevL = qL = firstSyncL;

	for (i=0; i<LinkNENTRIES(octL); i++, noteOctL=NextNOTEOTTAVAL(noteOctL)) {
		anoteOct = GetPANOTEOTTAVA(noteOctL);
		prevL = qL;
		qL = anoteOct->opSync;
		if (!SameSystem(qL, firstSys)) break;
	}
	nInOttava1 = i;

	RemoveOctOnStf(doc, octL, firstSyncL, RightLINK(lastSyncL), s);

	if (nInOttava1>0) CreateOTTAVA(doc, firstSyncL, RightLINK(prevL), s,
											nInOttava1, octType, false, false);
}


/* ------------------------------------------------------------- FixCrossSysOttavas -- */
/* In the range [startL,endL), fix "cross-system-ness" of all Ottavas. We'd like to
do it as described in FixCrossSysObjects, but Nightingale does not allow cross-system
Ottavas yet, so we just clip them to keep them on one system. It might be better to
also add a second piece of the Ottava on the following system, even though they can't
be linked the way pieces of cross-system slurs and beams are.

Return value is the number of Ottavas truncated. */

short FixCrossSysOttavas(Document *doc, LINK startL, LINK endL, short *pTruncFirstMeas,
									short *pTruncLastMeas)
{
	LINK pL, firstSys, lastSys;
	short nTrunc, thisMeas, truncFirstMeas=SHRT_MAX, truncLastMeas=0;
	POTTAVA pOttava;
	
	for (nTrunc = 0, pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==OTTAVAtype) {
			/*
			 *	If this Ottava crosses systems, clip it at the end of its first system.
			 */
			firstSys = LSSearch(FirstInOttava(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
			lastSys = LSSearch(LastInOttava(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
			if (firstSys!=lastSys) {
				MakeOctCrossSys(doc, pL, firstSys, lastSys);
				thisMeas = GetMeasNum(doc, pL);
				truncFirstMeas = n_min(thisMeas, truncFirstMeas);
				truncLastMeas = n_max(thisMeas, truncLastMeas);
				nTrunc++;
				pOttava = GetPOTTAVA(pL);
				pOttava->noCutoff = true;
			}
		}
	}
	
	*pTruncFirstMeas = truncFirstMeas;
	*pTruncLastMeas = truncLastMeas;
	return nTrunc;
}


/* -------------------------------------------------------------- FixCrossSysEndings -- */
/* In the range [startL,endL), fix "cross-system-ness" of all Endings. We'd like to
do it as described in FixCrossSysObjects, but Nightingale does not allow cross-system
Endings yet, so we just clip them to keep them on one system. It might be better to
also add a second piece of the Ending on the following system, even though they
can't be linked the way pieces of cross-system slurs and beams are.

Return value is the number of Endings truncated. */

short FixCrossSysEndings(Document *doc, LINK startL, LINK endL, short *pTruncFirstMeas,
									short *pTruncLastMeas)
{
	LINK pL;
	PENDING p;
	LINK firstSys, lastSys, lastSync;
	short staff, nTrunc, thisMeas, truncFirstMeas=SHRT_MAX, truncLastMeas=0;
	DDIST endxd;
	
	for (nTrunc = 0, pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==ENDINGtype) {
			/*
			 *	If this Ending crosses systems, clip it at the end of its first system:
			 * set its lastSync to the last Sync on its staff of the first system, and
			 * set its endxd so it extends to just short of the right edge of the system.
			 */
			 firstSys = LSSearch(EndingFIRSTOBJ(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
			 lastSys = LSSearch(EndingLASTOBJ(pL), SYSTEMtype, ANYONE, GO_LEFT, false);
			if (firstSys!=lastSys) {
				staff = EndingSTAFF(pL);
				lastSync = LSSearch(LinkRSYS(firstSys), SYNCtype, staff, GO_LEFT, false);
				EndingLASTOBJ(pL) = lastSync;
				endxd = StaffLength(lastSync)-pt2d(4)-SysRelxd(lastSync);
				p = GetPENDING(pL);
				p->endxd = endxd;
				p->noRCutoff = true;
				thisMeas = GetMeasNum(doc, pL);
				truncFirstMeas = n_min(thisMeas, truncFirstMeas);
				truncLastMeas = n_max(thisMeas, truncLastMeas);
				nTrunc++;
			}
		}
	}
	
	*pTruncFirstMeas = truncFirstMeas;
	*pTruncLastMeas = truncLastMeas;
	return nTrunc;
}


/* -------------------------------------------------------------- FixCrossSysObjects -- */
/* Fix up cross-system-ness of objects in the given range. Intended to be called after
reformatting operations, including "move measures up/down".

We make existing cross-system objects spanning Measures that are now in the same
System non-cross-system. Similarly, we WANT to make non-cross-system objects crossing
from a Measure that's now in one System into a Measure that's now in another System
cross-system; unfortunately, we can't always do this because Nightingale restricts
cross-system Slurs and Beamsets to just two Systems, and it doesn't understand cross-
system hairpins, octave signs, endings, and GRDraw (line) Graphics at all. (The other
type of "extended object",the tuplet, can't cross measure boundaries, so it's not a
concern.) In any case, we also fix up higher-level links of cross-system objects that
stay cross-system. */

void FixCrossSysObjects(Document *doc, LINK startL, LINK endL)
{
	short nSlurTrunc, nBeamTrunc, nHairTrunc, nOctTrunc, nEndingTrunc, nDrawTrunc;
	short truncFirstMeas, truncLastMeas, truncVeryFirstMeas, truncVeryLastMeas;
	char numStr1[32], numStr2[32];
	char strSlur[32], strSlurs[32], strBeam[32], strBeams[32];
	char strHairpin[32], strHairpins[32], strOctave[32], strOctaves[32], strEnding[32],
			strEndings[32], strLine[32], strLines[32];
	
	nSlurTrunc = FixCrossSysSlurs(doc, startL, endL, &truncVeryFirstMeas, &truncVeryLastMeas);
	nBeamTrunc = FixCrossSysBeams(doc, startL, endL, &truncFirstMeas, &truncLastMeas);
	truncVeryFirstMeas = n_min(truncFirstMeas, truncVeryFirstMeas);
	truncVeryLastMeas = n_max(truncLastMeas, truncVeryLastMeas);

	if (nSlurTrunc>0 || nBeamTrunc>0) {
		sprintf(strBuf, "");

		if (nSlurTrunc>0) {
			GetIndCString(strSlur, EXTOBJ_STRS, 1);
			GetIndCString(strSlurs, EXTOBJ_STRS, 2);
			sprintf(&strBuf[strlen(strBuf)], " %d %s,",
					nSlurTrunc, (nSlurTrunc>1? strSlurs : strSlur));
		} 
		if (nBeamTrunc>0) {
			GetIndCString(strBeam, EXTOBJ_STRS, 3);
			GetIndCString(strBeams, EXTOBJ_STRS, 4);
			sprintf(&strBuf[strlen(strBuf)], " %d %s,",
					nBeamTrunc, (nBeamTrunc>1? strBeams : strBeam));
		} 

		strBuf[strlen(strBuf)-1] = '\0';					/* Remove final "," */
		sprintf(numStr1, "%d", truncVeryFirstMeas);
		sprintf(numStr2, "%d", truncVeryLastMeas);
		CParamText(strBuf, numStr1, numStr2, "");
		StopInform(RFMT_XSYS1_ALRT);
	}
	
	nHairTrunc = FixCrossSysHairpins(doc, startL, endL, &truncVeryFirstMeas, &truncVeryLastMeas);

	nDrawTrunc = FixCrossSysDrawObjs(doc, startL, endL, &truncFirstMeas, &truncLastMeas);
	truncVeryFirstMeas = n_min(truncFirstMeas, truncVeryFirstMeas);
	truncVeryLastMeas = n_max(truncLastMeas,truncVeryLastMeas);

	nOctTrunc = FixCrossSysOttavas(doc, startL, endL, &truncFirstMeas, &truncLastMeas);
	truncVeryFirstMeas = n_min(truncFirstMeas, truncVeryFirstMeas);
	truncVeryLastMeas = n_max(truncLastMeas,truncVeryLastMeas);

	nEndingTrunc = FixCrossSysEndings(doc, startL, endL, &truncFirstMeas, &truncLastMeas);
	truncVeryFirstMeas = n_min(truncFirstMeas, truncVeryFirstMeas);
	truncVeryLastMeas = n_max(truncLastMeas,truncVeryLastMeas);

	if (nHairTrunc>0 || nOctTrunc>0 || nEndingTrunc>0 || nDrawTrunc) {
		sprintf(strBuf, "");

		if (nHairTrunc>0) {
			GetIndCString(strHairpin, EXTOBJ_STRS, 5);
			GetIndCString(strHairpins, EXTOBJ_STRS, 6);
			sprintf(&strBuf[strlen(strBuf)], " %d %s,",
					nHairTrunc, (nHairTrunc>1? strHairpins : strHairpin));
		}
		if (nOctTrunc>0) {
			GetIndCString(strOctave, EXTOBJ_STRS, 7);
			GetIndCString(strOctaves, EXTOBJ_STRS, 8);
			sprintf(&strBuf[strlen(strBuf)], " %d %s,",
					nOctTrunc, (nOctTrunc>1? strOctaves : strOctave)); 
		}
		if (nEndingTrunc>0) {
			GetIndCString(strEnding, EXTOBJ_STRS, 9);
			GetIndCString(strEndings, EXTOBJ_STRS, 10);
			sprintf(&strBuf[strlen(strBuf)], " %d %s,",
					nEndingTrunc, (nEndingTrunc>1? strEndings : strEnding));
		}
		if (nDrawTrunc>0) {
			GetIndCString(strLine, EXTOBJ_STRS, 11);
			GetIndCString(strLines, EXTOBJ_STRS, 12);
			sprintf(&strBuf[strlen(strBuf)], " %d %s,",
					nDrawTrunc, (nDrawTrunc>1? strLines : strLine));
		}

		strBuf[strlen(strBuf)-1] = '\0';					/* Remove final "," */
		sprintf(numStr1, "%d", truncVeryFirstMeas);
		sprintf(numStr2, "%d", truncVeryLastMeas);
		CParamText(strBuf, numStr1, numStr2, "");
		LogPrintf(LOG_INFO, "Can’t handle hairpins, 8vas, endings, or lines crossing a system break, so these were truncated: %s in mm. %d thru %d\n",
					strBuf, truncVeryFirstMeas, truncVeryLastMeas);
		StopInform(RFMT_XSYS2_ALRT);						/* "Nightingale can't handle hairpins, etc..." */
	}
}
