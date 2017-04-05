/* Reconstruct.c for Nightingale. Used for functions that re-arrange the object
list (based on time?), e.g., Remove Gaps in Voices, Merge, Create/Remove Tuplet. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean Check1ContinVoice(Document *, LINK, Boolean [], SPACETIMEINFO *);
static void FixObjStfSize(Document *, LINK);

static LINK InsertClJITBefore(Document *doc, LINK pL, LINK newObjL, COPYMAP *mergeMap);
static LINK InsertClJIPBefore(Document *doc, LINK pL, LINK newObjL, COPYMAP *mergeMap);
static LINK InsertClJDBefore(Document *doc, LINK pL, LINK newObjL, COPYMAP *mergeMap);

static void LocateClJITObj(Document *, LINK, LINK, LINK, PTIME *, short, COPYMAP *, short *);
static void LocateClJIPObj(Document *, LINK, LINK, LINK, PTIME *, short, COPYMAP *, short *);
static void LocateClGenlJDObj(Document *, LINK, LINK, LINK, LINK, LINK, PTIME *, short,COPYMAP *,
								short *);
static void LocateClJDObj(Document *, LINK, LINK, PTIME *, short, COPYMAP *, short *);

static void FixSlurLinks(LINK slurL, PTIME *durArray);
static void FixDynamLinks(LINK dynamL, PTIME *durArray);
static void FixEndingLinks(LINK endingL, PTIME *durArray);
static void FixGRDrawLinks(LINK graphicL, PTIME *durArray);

/* --------------------------------------------------------------------------------- */
/* Reconstruction utilities */

/* Return TRUE if voice v contains notes in the selection range. */

Boolean VoiceInSelRange(Document *doc, short v)
{
	LINK vStartL,vEndL;
	
	GetNoteSelRange(doc,v,&vStartL,&vEndL, NOTES_ONLY);
	return (vStartL && vEndL);
}


/* Check all voices in the selection to see if there are any gaps in the logical
time of notes in those voices in the given measure; if so, return FALSE. */

Boolean Check1ContinVoice(Document *doc, LINK measL, Boolean vInSel[],
									SPACETIMEINFO *spTimeInfo)
{
	LINK link, endMeasL; short i, lastNode, v;
	Boolean first;
	long startTime, nextlTime;

	endMeasL = LinkRMEAS(measL) ? LinkRMEAS(measL) : LeftLINK(doc->tailL);
	lastNode = GetSpTimeInfo(doc, RightLINK(measL), endMeasL, spTimeInfo, FALSE);

	/* For each voice in use <v>, traverse spTimeInfo's list of syncs for the
		measure the selection is in. The first sync should be at startTime zero.
		After the first sync, each sync following <link> in v should be at startTime
		equal to link's startTime plus its duration in the voice. If it is at a
		greater time, there is a hole in the voice, so return FALSE.
		Note the confusing way we have to set the startL and endL params
		for GetSpTimeInfo. */

	for (v = 1; v<=MAXVOICES; v++)
		if (vInSel[v]) {
			first = TRUE;
			startTime = nextlTime = 0L;
			for (i=0; i<=lastNode; i++) {
				link = spTimeInfo[i].link;
				if (SyncTYPE(link))
					if (SyncInVoice(link,v)) {
						startTime = spTimeInfo[i].startTime;
						if (first) {
							if (startTime!=0) return FALSE;
							first = FALSE;
						}
						else
							if (startTime > nextlTime) return FALSE;

						nextlTime = startTime+GetVLDur(doc,link,v);
					}
			}
		}

	return TRUE;
}


/* Check all voices in the selection to see if there are any gaps in their logical
times in any measure in the selection range. If there are any such gaps, return
FALSE. */

Boolean CheckContinVoice(Document *doc)
{
	LINK measL, lastL; short v;
	Boolean okay=FALSE, vInSel[MAXVOICES+1];
	SPACETIMEINFO *spTimeInfo;
	
	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) return TRUE;

	for (v = 1; v<=MAXVOICES; v++)
		vInSel[v] = VoiceInSelRange(doc,v);
	
	measL = LSSearch(doc->selStartL,MEASUREtype,ANYONE,GO_LEFT,FALSE);
	lastL = EndMeasSearch(doc, LeftLINK(doc->selEndL));

	for ( ; measL && measL!=lastL; measL=LinkRMEAS(measL))
		if (!Check1ContinVoice(doc,measL,vInSel,spTimeInfo)) goto Cleanup;
	
	okay = TRUE;
	
Cleanup:
	DisposePtr((Ptr)spTimeInfo);
	return okay;
}


/* First initialize the unused 0'th row of the array, then the rest of the rows.
Set pTimes of 0'th row to BIGNUM to sort them above the world. */

void InitDurArray(PTIME *durArray, short nInMeas)
{
	short notes,v;

	for (notes = 0; notes<nInMeas; notes++) {				/* unused 0'th row */
			(durArray + notes)->objL = NILINK;
			(durArray + notes)->subL = NILINK;
			(durArray + notes)->newObjL = NILINK;
			(durArray + notes)->newSubL = NILINK;
			(durArray + notes)->beamL = NILINK;
			(durArray + notes)->octL = NILINK;
			(durArray + notes)->tupletL = NILINK;
			(durArray + notes)->slurFirstL = NILINK;
			(durArray + notes)->slurLastL = NILINK;
			(durArray + notes)->tieFirstL = NILINK;
			(durArray + notes)->tieLastL = NILINK;
			(durArray + notes)->mult = 0;
			(durArray + notes)->playDur = 0L;
			(durArray + notes)->pTime = BIGNUM;
	}
	for (v = 1; v<=MAXVOICES; v++)
		for (notes = 0; notes<nInMeas; notes++) {
			(durArray + v*nInMeas+notes)->objL = NILINK;
			(durArray + v*nInMeas+notes)->subL = NILINK;
			(durArray + v*nInMeas+notes)->newObjL = NILINK;
			(durArray + v*nInMeas+notes)->newSubL = NILINK;
			(durArray + v*nInMeas+notes)->beamL = NILINK;
			(durArray + v*nInMeas+notes)->octL = NILINK;
			(durArray + v*nInMeas+notes)->tupletL = NILINK;
			(durArray + v*nInMeas+notes)->slurFirstL = NILINK;
			(durArray + v*nInMeas+notes)->slurLastL = NILINK;
			(durArray + v*nInMeas+notes)->tieFirstL = NILINK;
			(durArray + v*nInMeas+notes)->tieLastL = NILINK;
			(durArray + v*nInMeas+notes)->mult = 0;
			(durArray + v*nInMeas+notes)->playDur = 0L;
			(durArray + v*nInMeas+notes)->pTime = 0L;
		}
}

/* ----------------------------------------------- Utilities for ComputePlayTimes -- */

/* Set playDur values for the pDurArray. */

void SetPlayDurs(Document */*doc*/, PTIME *durArray, short nInMeas, LINK startMeas,
						LINK endMeas)
{
	short v,notes;
	LINK pL,aNoteL;
	PANOTE aNote;
	PTIME *pTime;

	for (v = 1; v<=MAXVOICES; v++)
		for (notes=0, pL=startMeas; pL!=endMeas; pL=RightLINK(pL))
			if (SyncTYPE(pL)) {
				(durArray + v*nInMeas+notes)->objL = pL;
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteVOICE(aNoteL)==v) {
						pTime = durArray + v*nInMeas+notes;
						if (MainNote(aNoteL)) {
							aNote = GetPANOTE(aNoteL);
							pTime->subL = aNoteL;
							pTime->playDur = aNote->playDur;
						}
						pTime->mult++;
					}
				notes++;
			}
}


/* Set playDur values for the pDurArray from the note's logical duration. */

void SetLDurs(Document *doc, PTIME *durArray, short nInMeas, LINK startMeas, LINK endMeas);
void SetLDurs(Document *doc, PTIME *durArray, short nInMeas, LINK startMeas, LINK endMeas)
{
	short v, notes;
	LINK pL, aNoteL;
	PANOTE aNote;
	PTIME *pTime;

	for (v = 1; v<=MAXVOICES; v++)
		for (notes=0, pL=startMeas; pL!=endMeas; pL=RightLINK(pL))
			if (SyncTYPE(pL)) {
				(durArray + v*nInMeas+notes)->objL = pL;
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteVOICE(aNoteL)==v) {
						pTime = durArray + v*nInMeas+notes;
						if (MainNote(aNoteL)) {
							aNote = GetPANOTE(aNoteL);
							pTime->subL = aNoteL;
							pTime->playDur = CalcNoteLDur(doc,aNoteL,pL);
						}
						pTime->mult++;
					}
				notes++;
			}
}


/* Set pTime values for the pDurArray. */

short SetPTimes(Document *doc, PTIME *durArray, short nInMeas, SPACETIMEINFO *spTimeInfo,
													LINK startMeas, LINK endMeas)
{
	short j, v, notes, nInMeasure;
	LINK pL, aNoteL, barLastL;
	PANOTE aNote;

	barLastL = EndMeasSearch(doc, startMeas);
	nInMeasure = GetSpTimeInfo(doc, startMeas, barLastL, spTimeInfo, FALSE);
	for (v = 1; v<=MAXVOICES; v++)
		for (notes=0, pL=startMeas; pL!=endMeas; pL=RightLINK(pL))
			if (SyncTYPE(pL)) {
				for (j=0; j<MAX_MEASNODES; j++)
					if (spTimeInfo[j].link == pL) break;
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (MainNoteInVOICE(aNoteL, v)) {
						aNote = GetPANOTE(aNoteL);
						(durArray + v*nInMeas+notes)->pTime = aNote->pTime = 
												spTimeInfo[j].startTime + aNote->playTimeDelta;
					}
				notes++;
			}

	return nInMeasure;
}


/* Set link values for owning beams, octave signs, tuplets, and slurs. */

void SetLinkOwners(PTIME *durArray, short nInMeas, LINK startMeas, LINK endMeas)
{
	short v,notes;
	LINK pL,aNoteL,beamL,octL,tupletL,slurL,tieL;
	PANOTE aNote;
	SearchParam pbSearch;

	for (v = 1; v<=MAXVOICES; v++)
		/* Skipping voices that aren't in use could save a lot of time here, but it's
			probably not worth the trouble. */
		for (notes=0, pL=startMeas; pL!=endMeas; pL=RightLINK(pL))
			if (SyncTYPE(pL)) {
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (MainNoteInVOICE(aNoteL, v)) {
						/*
						 * For each MainNote (chord or individual note/rest), if it's
						 * beamed, find its Beamset object. Likewise for octave signs,
						 * tuplets, and slurs (including ties).
						 */
						pL = (durArray + v*nInMeas+notes)->objL;
						if (NoteBEAMED(aNoteL)) {
							beamL = LVSearch(pL, BEAMSETtype, v, TRUE, FALSE);
							if (beamL)
								(durArray + v*nInMeas+notes)->beamL = beamL;
						}
						if (NoteINOTTAVA(aNoteL)) {
							octL = LSSearch(pL,OTTAVAtype,NoteSTAFF(aNoteL),TRUE,FALSE);
							if (octL)
								(durArray + v*nInMeas+notes)->octL = octL;
						}
						if (NoteINTUPLET(aNoteL)) {
							tupletL = LVSearch(pL, TUPLETtype, v, TRUE, FALSE);
							if (tupletL)
								(durArray + v*nInMeas+notes)->tupletL = tupletL;
						}

						aNote = GetPANOTE(aNoteL);
						InitSearchParam(&pbSearch);
						pbSearch.id = ANYONE;
						pbSearch.voice = v;
						pbSearch.needSelected = FALSE;
						pbSearch.inSystem = FALSE;
						pbSearch.optimize = TRUE;

						if (aNote->slurredR) {
							pbSearch.subtype = FALSE;			/* FALSE for Slur, TRUE for Tie */
							slurL = L_Search(pL, SLURtype, TRUE, &pbSearch);
							if (slurL)
								(durArray + v*nInMeas+notes)->slurFirstL = slurL;
						}
						aNote = GetPANOTE(aNoteL);
						if (aNote->slurredL) {
							pbSearch.subtype = FALSE;
							slurL = L_Search(pL, SLURtype, TRUE, &pbSearch);
							while (slurL) {
								if (SlurLASTSYNC(slurL)==pL) {
									(durArray + v*nInMeas+notes)->slurLastL = slurL;
									break;
								}
								slurL = L_Search(LeftLINK(slurL), SLURtype, TRUE, &pbSearch);
							}
						}
						if (aNote->tiedR) {
							pbSearch.subtype = TRUE;
							tieL = L_Search(pL, SLURtype, TRUE, &pbSearch);
							if (tieL)
								(durArray + v*nInMeas+notes)->tieFirstL = tieL;
						}
						if (aNote->tiedL) {
							pbSearch.subtype = TRUE;
							tieL = L_Search(pL, SLURtype, TRUE, &pbSearch);
							while (tieL) {
								if (SlurLASTSYNC(tieL)==pL) {
									(durArray + v*nInMeas+notes)->tieLastL = tieL;
									break;
								}
								tieL = L_Search(LeftLINK(tieL), SLURtype, TRUE, &pbSearch);
							}
						}
					}
				notes++;
			}
}


/* ------------------------------------------------- Utilities for RearrangeNotes -- */

/* Compact the pDurArray into qDurArray: copy into qDurArray only those pTimes s.t.
pTime->subL!= NILINK, and then fill in the rest of the entries with default values. */

short CompactDurArray(PTIME *pDurArray, PTIME *qDurArray, short numNotes)
{
	PTIME *pTime,*qTime; short i,arrBound,notes;

	pTime = pDurArray;
	qTime = qDurArray;
	for (notes=0, i=0; notes<numNotes; notes++,pTime++)
		if (pTime->subL) 
			{ *qTime = *pTime; i++; qTime++; }

	arrBound = i;
	for ( ; i<numNotes; i++) {
		(qDurArray + i)->objL = NILINK;
		(qDurArray + i)->subL = NILINK;
		(qDurArray + i)->newObjL = NILINK;
		(qDurArray + i)->newSubL = NILINK;
		(qDurArray + i)->beamL = NILINK;
		(qDurArray + i)->octL = NILINK;
		(qDurArray + i)->tupletL = NILINK;
		(qDurArray + i)->slurFirstL = NILINK;
		(qDurArray + i)->slurLastL = NILINK;
		(qDurArray + i)->mult = 0;
		(qDurArray + i)->playDur = 0L;
		(qDurArray + i)->pTime = BIGNUM;
	}
	
	return arrBound;
}

/* ------------------------------------------------------------------- CopyNotes -- */
/* Copy note subL into note newSubL, and copy the pTime field and perhaps the playDur
field into it.

#1. pNewSub->firstMod is a still valid LINK to subL's linked list of modNR's in
subL's doc heaps. CopyMODNRList copies this list into a duplicate list in newSubL's
doc heaps. */

LINK CopyNotes(Document *doc, LINK subL, LINK newSubL,
					long playDur, long pTime)						/* If playDur<0, ignore it */
{
	PANOTE pSub, pNewSub;
	LINK nextL;

	pSub    = GetPANOTE(subL);
	pNewSub = GetPANOTE(newSubL);
	nextL = NextNOTEL(newSubL);
	BlockMove(pSub, pNewSub, (long)sizeof(ANOTE));
	NextNOTEL(newSubL) = nextL;
	if (playDur>=0L) pNewSub->playDur = playDur;
	pNewSub->pTime = pTime;
	if (pNewSub->firstMod)
		pNewSub->firstMod = CopyModNRList(doc,doc,pNewSub->firstMod); 	/* #1. */
	return nextL;
}


/* --------------------------------------------------------------- CopyClipNotes -- */
/* Copy note subL from the clipboard into note newSubL from doc, and copy the
pTime field into it. */

LINK CopyClipNotes(Document *doc, LINK subL, LINK newSubL, long pTime)
{
	PANOTE pSub, pNewSub;
	LINK nextL;

	pSub    = DGetPANOTE(clipboard,subL);
	pNewSub = DGetPANOTE(doc,newSubL);
	nextL = DNextNOTEL(doc,newSubL);
	BlockMove(pSub, pNewSub, (long)sizeof(ANOTE));
	DNextNOTEL(doc,newSubL) = nextL;
	pNewSub->pTime = pTime;
	if (pNewSub->firstMod)
		pNewSub->firstMod = CopyModNRList(clipboard,doc,pNewSub->firstMod);
	return nextL;
}


/* ----------------------------------------------------------------- CopySubObjs -- */
/* Copy the subObject information from each note at this time into the subObjects
of the newly created object. Decrement the nEntries field of the object from which
the subObj is copied, and remove that subObject, or delete the object, if it has
only one subObj left. This refers to the sync still in the object list. Returns
the subObj LINK in the list following the last one initialized, or NILINK if every
LINK is initialized. */

LINK CopySubObjs(Document *doc, LINK newObjL, PTIME *pTime)
{
	LINK subL,newSubL,tempSubL; PTIME *qTime;
	Boolean objSel=FALSE; short v;

	qTime = pTime;
	subL = qTime->subL;
	newSubL = FirstSubLINK(newObjL);

	while (qTime->pTime==pTime->pTime) {
		if (NoteSEL(subL)) objSel = TRUE;

		if (qTime->mult > 1) {
			v = NoteVOICE(subL);
			subL = FirstSubLINK(qTime->objL);
			for ( ; subL; subL = NextNOTEL(subL))
				if (NoteVOICE(subL)==v)
					newSubL = CopyNotes(doc, subL, newSubL, -1L, qTime->pTime);

			/* Set the newObjL & newSubL fields for use by FixObjLinks &
				GetInsertPt. Use tempSubL here, since the else block of this
				if requires newSubL to be set correctly. */

			qTime->newObjL = newObjL;
			tempSubL = FirstSubLINK(newObjL);
			for ( ; tempSubL; tempSubL = NextNOTEL(tempSubL))
				if (MainNote(tempSubL) && NoteVOICE(tempSubL)==v) 
					{ qTime->newSubL = tempSubL; break; }

		}
		else {
			qTime->newObjL = newObjL;
			qTime->newSubL = newSubL;
			newSubL = CopyNotes(doc, subL, newSubL, -1L, qTime->pTime);
		}
		qTime++;
		subL = qTime->subL;
	}

	LinkSEL(newObjL) = objSel;

	return newSubL;
}

/* -------------------------------------------------------------------- CopySync -- */
/* Create a new Sync with subCount nEntries and copy object level information
from pTime into it. */

LINK CopySync(Document *doc, short subCount, PTIME *pTime)
{
	PMEVENT pObj,pNewObj;
	LINK firstSubObjL,newObjL;

	newObjL = NewNode(doc, SYNCtype, subCount);
	if (newObjL == NILINK) return NILINK;

	pObj    = (PMEVENT)LinkToPtr(OBJheap,pTime->objL);
	pNewObj = (PMEVENT)LinkToPtr(OBJheap,newObjL);
	firstSubObjL = FirstSubLINK(newObjL);				/* Save it before it gets wiped out */
	BlockMove(pObj, pNewObj, sizeof(SUPEROBJECT));
	FirstSubLINK(newObjL) = firstSubObjL;				/* Restore it. */
	LinkNENTRIES(newObjL) = subCount;					/* Set new nEntries. */
	
	return newObjL;
}

/* -------------------------------------------------------------- InsertJDBefore -- */
/* Pull pL out of its linked list and insert it before newObjL
in newObjL's list. */

void InsertJDBefore(LINK pL, LINK newObjL)
{
	RightLINK(LeftLINK(pL)) = RightLINK(pL);
	LeftLINK(RightLINK(pL)) = LeftLINK(pL);

	LeftLINK(pL) = LeftLINK(newObjL);
	RightLINK(pL) = newObjL;
	
	RightLINK(LeftLINK(newObjL)) = pL;
	LeftLINK(newObjL) = pL;
}

/* ------------------------------------------------------------- InsertJITBefore -- */
/* Pull pL out of its linked list and insert it before newObjL in newObjL's
list. Identical to InsertJIPBefore; wait to combine until we get correct
name, and determine that it will stay identical. */

void InsertJITBefore(LINK pL, LINK newObjL)
{
	RightLINK(LeftLINK(pL)) = RightLINK(pL);
	LeftLINK(RightLINK(pL)) = LeftLINK(pL);

	LeftLINK(pL) = LeftLINK(newObjL);
	RightLINK(pL) = newObjL;
	
	RightLINK(LeftLINK(newObjL)) = pL;
	LeftLINK(newObjL) = pL;
}


/* ------------------------------------------------------------- InsertJIPBefore -- */
/* Pull pL out of its linked list and insert it before newObjL in newObjL's
list. Identical to InsertJIPBefore; wait to combine until we get correct
name, and determine that it will stay identical. */

void InsertJIPBefore(LINK pL, LINK newObjL)
{
	RightLINK(LeftLINK(pL)) = RightLINK(pL);
	LeftLINK(RightLINK(pL)) = LeftLINK(pL);

	LeftLINK(pL) = LeftLINK(newObjL);
	RightLINK(pL) = newObjL;
	
	RightLINK(LeftLINK(newObjL)) = pL;
	LeftLINK(newObjL) = pL;
}


/* ---------------------------------------------------------------- LocateJITObj -- */
/* Find the pt into which to insert the J_IT object pL in the rearranged score
object list, and call InsertJITBefore to insert it there. */

void LocateJITObj(Document */*doc*/, LINK pL, LINK nextL, LINK endMeasL, PTIME *durArray)
{
	PTIME *pTime; LINK newObjL=NILINK;
	
	if (nextL)
		for (pTime=durArray; newObjL==NILINK && pTime->pTime<BIGNUM; pTime++)
			if (pTime->objL==nextL) {
	
				switch (ObjLType(pL)) {
					case SPACERtype:
						if (SpacerSTAFF(pL)==NoteSTAFF(pTime->newSubL))
							newObjL = pTime->newObjL;
						break;
					case RPTENDtype:
					case PSMEAStype:
						newObjL = pTime->newObjL;
						break;
				}
			}
	
	if (!newObjL)
		newObjL = endMeasL;

	InsertJITBefore(pL, newObjL);
}

/* ---------------------------------------------------------------- LocateJIPObj -- */
/* Find the pt into which to insert the J_IP object pL in the rearranged score
object list, and call InsertJIPBefore to insert it there. */

void LocateJIPObj(Document */*doc*/, LINK pL, LINK nextL, LINK endMeasL, PTIME *durArray)
{
	PTIME *pTime; LINK newObjL=NILINK,aClefL,aTimeSigL,aKeySigL,aGRNoteL;

	if (nextL)
		for (pTime=durArray; newObjL==NILINK && pTime->pTime<BIGNUM; pTime++)
			if (pTime->objL==nextL) {

				/* Need code here to handle the possibility that J_IP object
					must be split up: e.g. if range containing timeSig on all
					staves is tupled on one of the staves, the timeSig may have to
					be inserted into the object list in different locations
					on different staves. Even with this situation, could run
					into problems if try to tuple heterogeneous overlapping
					ranges in 2 separate voices on the same staff. */

				switch (ObjLType(pL)) {
					case CLEFtype:
						aClefL = FirstSubLINK(pL);
						for ( ; aClefL; aClefL = NextCLEFL(aClefL))
							if (ClefSTAFF(aClefL)==NoteSTAFF(pTime->newSubL))
								newObjL = pTime->newObjL;
						break;
					case TIMESIGtype:
						aTimeSigL = FirstSubLINK(pL);
						for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
							if (TimeSigSTAFF(aTimeSigL)==NoteSTAFF(pTime->newSubL))
								newObjL = pTime->newObjL;
						break;
					case KEYSIGtype:
						aKeySigL = FirstSubLINK(pL);
						for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
							if (KeySigSTAFF(aKeySigL)==NoteSTAFF(pTime->newSubL))
								newObjL = pTime->newObjL;
						break;
					case GRSYNCtype:
						aGRNoteL = FirstSubLINK(pL);
						for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
							if (GRNoteSTAFF(aGRNoteL)==NoteSTAFF(pTime->newSubL))
								newObjL = pTime->newObjL;
						break;
				}
			}

	if (!newObjL)
		newObjL = endMeasL;

	InsertJIPBefore(pL, newObjL);
}


/* ------------------------------------------------------------- LocateGenlJDObj -- */
/* Find the pt into which to insert the J_D object pL in the rearranged score
object list, call InsertJDBefore to insert it there, and, if appropriate, update its
relative-object links. For J_D objects which can be relative to any J_IT or J_IP
object. Must be called after all J_IP objects have been re-inserted in the object
list, so that we can be sure to have the link relative to which to re-insert the
J_D object. */

void LocateGenlJDObj(Document */*doc*/, LINK pL, PTIME *durArray)
{
	LINK firstL, newObjL, lastL, newLastL; PTIME *pTime;

	switch (ObjLType(pL)) {
		case GRAPHICtype:
			if (GraphicSubType(pL)==GRDraw) {
				newObjL = NILINK;
				firstL = GraphicFIRSTOBJ(pL);
	
				/* We don't need to consider GRSyncs, since they can't be rearranged
				   by the reconstruction process. */
				   
				if (SyncTYPE(firstL)) {
					for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
						if (pTime->objL==firstL) {
							if (NoteVOICE(pTime->newSubL)==GraphicVOICE(pL)) {
								newObjL = pTime->newObjL;
								GraphicFIRSTOBJ(pL) = newObjL;
								break;
							}
						}
				}
				else
					newObjL = firstL;
					
				if (newObjL) {
					lastL = GraphicLASTOBJ(pL);
					newLastL = NILINK;
					if (SyncTYPE(lastL)) {
						for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
							if (pTime->objL==lastL) {
								if (NoteVOICE(pTime->newSubL)==GraphicVOICE(pL)) {
									newLastL = pTime->newObjL;
									GraphicLASTOBJ(pL) = newLastL;
									break;
								}
							}
					}
					InsertJDBefore(pL, newObjL);
				}
			}
			else {
				newObjL = NILINK;
				firstL = GraphicFIRSTOBJ(pL);
	
				if (SyncTYPE(firstL)) {
					for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
						if (pTime->objL==firstL) {
							if (NoteVOICE(pTime->newSubL)==GraphicVOICE(pL)) {
								newObjL = pTime->newObjL;
								GraphicFIRSTOBJ(pL) = newObjL;
								break;
							}
						}
				}
				else
					newObjL = firstL;
					
				if (newObjL)
					InsertJDBefore(pL, newObjL);
			}
			break;

		case TEMPOtype:
			newObjL = NILINK;
			firstL = TempoFIRSTOBJ(pL);

			if (SyncTYPE(firstL)) {
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->objL==firstL) {
						if (NoteSTAFF(pTime->newSubL)==TempoSTAFF(pL)) {
							newObjL = pTime->newObjL;
							TempoFIRSTOBJ(pL) = newObjL;
							break;
						}
					}
			}
			else
				newObjL = firstL;
				
			if (newObjL)
				InsertJDBefore(pL, newObjL);
			break;

		case ENDINGtype:
			newObjL = NILINK;
			firstL = EndingFIRSTOBJ(pL);

			if (SyncTYPE(firstL)) {
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->objL==firstL) {
						if (NoteSTAFF(pTime->newSubL)==EndingSTAFF(pL)) {
							newObjL = pTime->newObjL;
							EndingFIRSTOBJ(pL) = newObjL;
							break;
						}
					}
			}
			else
				newObjL = firstL;

			if (newObjL) {
				lastL = EndingLASTOBJ(pL);
				newLastL = NILINK;

				if (SyncTYPE(lastL))
					for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
						if (pTime->objL==lastL) {
							if (NoteSTAFF(pTime->newSubL)==EndingSTAFF(pL)) {
								newLastL = pTime->newObjL;
								EndingLASTOBJ(pL) = newLastL;
								break;
							}
						}
				InsertJDBefore(pL, newObjL);
			}
			break;

		case BEAMSETtype:
			if (GraceBEAM(pL)) {
				newObjL = FirstInBeam(pL);
				InsertJDBefore(pL, newObjL);
			}
			break;

		default:
			;
	}
}


/* ----------------------------------------------------------------- LocateJDObj -- */
/* Find the pt into which to insert the J_D object pL in the rearranged score
object list, call InsertJDBefore to insert it there, and, if appropriate, update
its relative-object links.. For J_D objects which can only be relative to SYNCs. */

void LocateJDObj(Document */*doc*/, LINK pL, LINK baseMeasL, PTIME *durArray)
{
	LINK firstL, newObjL, lastL, newLastL; PTIME *pTime;

	switch (ObjLType(pL)) {
		case BEAMSETtype:
			if (!GraceBEAM(pL)) {
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->beamL==pL) {
						if (NoteVOICE(pTime->newSubL)==BeamVOICE(pL)) {
							newObjL = pTime->newObjL;
							break;
						}
					}
				InsertJDBefore(pL, newObjL);
			}
			break;
		case OTTAVAtype:
			for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
				if (pTime->octL==pL) {
					if (NoteSTAFF(pTime->newSubL)==OttavaSTAFF(pL)) {
						newObjL = pTime->newObjL;
						break;
					}
				}
			InsertJDBefore(pL, newObjL);
			break;
		case TUPLETtype:
			for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
				if (pTime->tupletL==pL) {
					if (NoteVOICE(pTime->newSubL)==TupletVOICE(pL)) {
						newObjL = pTime->newObjL;
						break;
					}
				}
			InsertJDBefore(pL, newObjL);
			break;
		case DYNAMtype:
			if (IsHairpin(pL)) {
				newObjL = NILINK;
				firstL = DynamFIRSTSYNC(pL);
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->objL==firstL) {
						if (NoteSTAFF(pTime->newSubL)==DynamicSTAFF(FirstSubLINK(pL))) {
							newObjL = pTime->newObjL;
							DynamFIRSTSYNC(pL) = newObjL;
							break;
						}
					}
	
				if (newObjL) {
					/* Update the lastSyncL of the hairpin. */

					lastL = DynamLASTSYNC(pL);
					newLastL = NILINK;
					for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
						if (pTime->objL==lastL) {
							if (NoteSTAFF(pTime->newSubL)==DynamicSTAFF(FirstSubLINK(pL))) {
								newLastL = pTime->newObjL;
								DynamLASTSYNC(pL) = newLastL;
								break;
							}
						}

					InsertJDBefore(pL, newObjL);
				}
			}
			else {
				newObjL = NILINK;
				firstL = DynamFIRSTSYNC(pL);
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->objL==firstL)
						if (NoteSTAFF(pTime->newSubL)==DynamicSTAFF(FirstSubLINK(pL))) {
							newObjL = pTime->newObjL;
							DynamFIRSTSYNC(pL) = newObjL;
							break;
						}
					
				if (newObjL)
					InsertJDBefore(pL, newObjL);
			}
			break;
		case SLURtype:
			newObjL = NILINK;
			if (SlurLastIsSYSTEM(pL))
				newObjL = RightLINK(baseMeasL);
			else
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++) {
					if (SlurTIE(pL)) {
						if (pTime->tieFirstL==pL)
							if (NoteVOICE(pTime->newSubL)==SlurVOICE(pL))
								{ newObjL = pTime->newObjL; break; }
					}
					else {
						if (pTime->slurFirstL==pL)
							if (NoteVOICE(pTime->newSubL)==SlurVOICE(pL))
								{ newObjL = pTime->newObjL; break; }
					}
				}
			if (newObjL)
				InsertJDBefore(pL, newObjL);
			else {
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (SlurFIRSTSYNC(pL)==pTime->objL)
						{ newObjL = pTime->newObjL; break; }
				if (newObjL)
					InsertJDBefore(pL, newObjL);
			}
			break;
		case TEMPOtype:
		case GRAPHICtype:
		case ENDINGtype:
		case CONNECTtype:
		default:
			;
	}
}

#ifndef PUBLIC_VERSION

/* --------------------------------------------------------------- DebugDurArray -- */
/* Print out values of the durArray. */

void DebugDurArray(short arrBound, PTIME *durArray)
{
	short notes;

	for (notes=0; notes<arrBound; notes++) {
		LogPrintf(LOG_NOTICE, "notes=%d objL=%d newObjL=%d subL=%d newSubL=%d slurFirstL=%d tieFirstL=%d\n",
			notes, (durArray + notes)->objL,
			(durArray + notes)->newObjL,
			(durArray + notes)->subL,
			(durArray + notes)->newSubL,
			(durArray + notes)->slurFirstL,
			(durArray + notes)->tieFirstL);
		
		LogPrintf(LOG_NOTICE, "beamL=%d mult=%d playDur=%ld pTime=%ld\n",
			(durArray + notes)->beamL,
			(durArray + notes)->mult,
			(durArray + notes)->playDur,
			(durArray + notes)->pTime);
	}
	LogPrintf(LOG_NOTICE, "\n");
}

#endif

/* ---------------------------------------------------------------- RelocateObjs -- */
/* Relocate all J_IT & J_IP objects, and all J_D objects which can only be
relative to SYNCs. */

void RelocateObjs(Document *doc, LINK headL, LINK tailL, LINK startMeas, LINK endMeas,
						PTIME *durArray)
{
	LINK pL,nextL,nextSync,qL; short objType;

	for (pL = headL; pL!=tailL; pL=nextL) {
		nextL = RightLINK(pL);
		switch (JustTYPE(pL)) {
			case J_IT:
				objType = ObjLType(pL);
				if (objType==SPACERtype || objType==RPTENDtype || objType==PSMEAStype) {
					for (nextSync=NILINK, qL=pL; qL; qL=RightLINK(qL))
						if (SyncTYPE(qL)) 
							{ nextSync = qL; break; }
					LocateJITObj(doc, pL, nextSync, endMeas, durArray);
				}
				break;
			case J_IP:
				for (nextSync=NILINK, qL=pL; qL; qL=RightLINK(qL))
					if (SyncTYPE(qL)) 
						{ nextSync = qL; break; }
				LocateJIPObj(doc, pL, nextSync, endMeas, durArray);
				break;
			case J_D:
				if (!GenlJ_DTYPE(pL))
					LocateJDObj(doc, pL, startMeas, durArray);
				break;
			case J_STRUC:
				break;
		}
	}
}


/* ---------------------------------------------------------- RelocateGenlJDObjs -- */
/* Relocate all J_D objects which can be relative to J_IT objects other than
SYNCs or J_IP objects. */
	
void RelocateGenlJDObjs(Document *doc, LINK headL, LINK tailL, PTIME *durArray)
{
	LINK pL,nextL;

	if (RightLINK(headL) != tailL)
		for (pL = headL; pL!=tailL; pL=nextL) {
			nextL = RightLINK(pL);
			switch(JustTYPE(pL)) {
				case J_IT:
				case J_IP:
					break;
				case J_D:
					if (GenlJ_DTYPE(pL) || BeamsetTYPE(pL))
						LocateGenlJDObj(doc, pL, durArray);
					break;
				case J_STRUC:
					break;
			}
		}
}


/* ------------------------------------------- Init/Set/GetCopyMap, GetNumClObjs -- */

/* Caller uses InitCopyMap to init the copyMap with LINKs in the clipboard, and
expects clipboard to be install upon entry to and return from InitCopyMap.

#1. To be filled in when nodes are merged in (MMergeSync), or copied in (MCreateClSync,
RelocateClObjs), etc. */

void InitCopyMap(LINK startL, LINK endL, COPYMAP *mergeMap)
{
	LINK pL; short i;
	
	for (i=0,pL=startL; pL!=endL; i++,pL=RightLINK(pL)) {
		mergeMap[i].srcL = pL;
		mergeMap[i].dstL = NILINK;					/* #1. */					
	}
}

void SetCopyMap(LINK srcL, LINK dstL, short numObjs, COPYMAP *mergeMap)
{
	short i;
	
	for (i=0; i<numObjs; i++)
		if (mergeMap[i].srcL==srcL)
			mergeMap[i].dstL = dstL;
}

LINK GetCopyMap(LINK srcL, short numObjs, COPYMAP *mergeMap)
{
	short i;

	for (i=0; i<numObjs; i++)
		if (mergeMap[i].srcL==srcL)
			return mergeMap[i].dstL;
			
	return NILINK;
}

short GetNumClObjs(Document *doc)
{
	short i=0; LINK pL;

	InstallDoc(clipboard);
	
	for (pL=RightLINK(clipFirstMeas); pL!=clipboard->tailL; i++,pL=RightLINK(pL)) ;

	InstallDoc(doc);

	return i;
}

/* ----------------------------------- RelocateClObjs, RelocateClGenlJDObjs, etc. -- */
/*  Functions for re-locating non-sync objects from the clipboard */

/* --------------------------------------------------------------- FixObjStfSize -- */
/* Fix staff size for pL; if staff rastral for clipboard is different from score,
update staff rastral of pL for score. */

static void FixObjStfSize(Document *doc, LINK pL)
{
	if (clipboard->srastral!=doc->srastral)
		SetStaffSize(doc, pL, RightLINK(pL), clipboard->srastral, doc->srastral);
}

/* ----------------------------------------------------------- InsertClJITBefore -- */
/* Pull pL out of its linked list and insert it before newObjL in newObjL's
list. Identical to InsertJIPBefore; wait to combine until we get correct
name, and determine that it will stay identical. */

static LINK InsertClJITBefore(Document *doc, LINK pL, LINK newObjL, COPYMAP *mergeMap)
{
	LINK copyL; short numObjs;

	copyL = DuplicateObject(ObjLType(pL),pL,FALSE,clipboard,doc,FALSE);
	if (!copyL) return NILINK;

	/* GetNumClObjs installs clipboard and re-installs doc; SetCopyMap
		doesn't depend upon any particular doc heaps being installed. */

	numObjs = GetNumClObjs(doc);
	SetCopyMap(pL,copyL,numObjs,mergeMap);

	InstallDoc(doc);

	LeftLINK(copyL) = LeftLINK(newObjL);
	RightLINK(copyL) = newObjL;
	
	RightLINK(LeftLINK(newObjL)) = copyL;
	LeftLINK(newObjL) = copyL;
	
	FixObjStfSize(doc, copyL);
	InstallDoc(clipboard);
	
	return copyL;
}


/* ----------------------------------------------------------- InsertClJIPBefore -- */
/* Pull pL out of its linked list and insert it before newObjL in newObjL's
list. Identical to InsertJIPBefore; wait to combine until we get correct
name, and determine that it will stay identical. */

static LINK InsertClJIPBefore(Document *doc, LINK pL, LINK newObjL, COPYMAP *mergeMap)
{
	LINK copyL; short numObjs;

	copyL = DuplicateObject(ObjLType(pL),pL,FALSE,clipboard,doc,FALSE);
	if (!copyL) return NILINK;

	/* GetNumClObjs installs clipboard and re-installs doc; SetCopyMap
		doesn't depend upon any particular doc heaps being installed. */

	numObjs = GetNumClObjs(doc);
	SetCopyMap(pL,copyL,numObjs,mergeMap);

	InstallDoc(doc);

	LeftLINK(copyL) = LeftLINK(newObjL);
	RightLINK(copyL) = newObjL;
	
	RightLINK(LeftLINK(newObjL)) = copyL;
	LeftLINK(newObjL) = copyL;
	
	FixObjStfSize(doc, copyL);
	InstallDoc(clipboard);
	
	return copyL;
}


/* ------------------------------------------------------------ InsertClJDBefore -- */
/* Pull pL out of its linked list and insert it before newObjL in newObjL's list. */

static LINK InsertClJDBefore(Document *doc, LINK pL, LINK newObjL, COPYMAP *mergeMap)
{
	LINK copyL; short numObjs;

	copyL = DuplicateObject(ObjLType(pL),pL,FALSE,clipboard,doc,FALSE);
	if (!copyL) return NILINK;

	/* GetNumClObjs installs clipboard and re-installs doc; SetCopyMap
		doesn't depend upon any particular doc heaps being installed. */

	numObjs = GetNumClObjs(doc);
	SetCopyMap(pL,copyL,numObjs,mergeMap);

	InstallDoc(doc);

	LeftLINK(copyL) = LeftLINK(newObjL);
	RightLINK(copyL) = newObjL;
	
	RightLINK(LeftLINK(newObjL)) = copyL;
	LeftLINK(newObjL) = copyL;
	
	FixObjStfSize(doc, copyL);
	InstallDoc(clipboard);
	
	return copyL;
}


/* -------------------------------------------------------------- LocateClJITObj -- */
/* Find the pt into which to insert the J_IP object pL in the rearranged score
object list, and call InsertClJITBefore to insert it there. clipboard document
should be installed for all LocateClJ_TYPEObj functions. pL and nextL are LINKs
in the clipboard; endMeasL is in the doc. nextL is the first Sync following pL
in the clipboard. pTime->newObjL is a LINK in the doc's object list which is
set by CopyClSubObjs (Merge.c).
vMap for the LocateCl functions allows determination of new voice for objects
translated by stfDiff. */

static void LocateClJITObj(Document *doc, LINK pL, LINK nextL, LINK endMeasL,
							PTIME *durArray, short stfDiff, COPYMAP *mergeMap, short *vMap)
{
	PTIME *pTime; LINK newObjL=NILINK,copyL;
	
	if (nextL)
		for (pTime=durArray; newObjL==NILINK && pTime->pTime<BIGNUM; pTime++)
			if (pTime->objL==nextL) {
	
				switch (ObjLType(pL)) {
					case SPACERtype:
						if (SpacerSTAFF(pL)+stfDiff==DNoteSTAFF(doc,pTime->newSubL))
							newObjL = pTime->newObjL;
						break;
					case RPTENDtype:
					case PSMEAStype:
						newObjL = pTime->newObjL;
						break;
				}
			}
	
	if (!newObjL)
		newObjL = endMeasL;

	copyL = InsertClJITBefore(doc, pL, newObjL, mergeMap);
	InstallDoc(doc);
	FixStfAndVoice(copyL,stfDiff,vMap);
	InstallDoc(clipboard);
}


/* -------------------------------------------------------------- LocateClJIPObj -- */
/* Find the pt into which to insert the J_IP object pL in the rearranged score
object list, and call InsertJIPBefore to insert it there.
#1. Need code here to handle the possibility that J_IP object must be split up:
e.g. if range containing timeSig on all staves is tupled on one of the staves,
the timeSig may have to be inserted into the object list in different locations
on different staves. Even with this situation, could run into problems if try to
tuple heterogeneous overlapping ranges in 2 separate voices on the same staff. */

static void LocateClJIPObj(Document *doc, LINK pL, LINK nextL, LINK endMeasL,
							PTIME *durArray, short stfDiff, COPYMAP *mergeMap, short *vMap)
{
	PTIME *pTime; LINK newObjL=NILINK,aClefL,aTimeSigL,aKeySigL,aGRNoteL,copyL;

	if (nextL)
		for (pTime=durArray; newObjL==NILINK && pTime->pTime<BIGNUM; pTime++)
			if (pTime->objL==nextL) {

				switch (ObjLType(pL)) {				/* #1. */
					case CLEFtype:
						aClefL = FirstSubLINK(pL);
						for ( ; aClefL; aClefL = NextCLEFL(aClefL))
							if (ClefSTAFF(aClefL)+stfDiff==DNoteSTAFF(doc,pTime->newSubL))
								newObjL = pTime->newObjL;
						break;
					case TIMESIGtype:
						aTimeSigL = FirstSubLINK(pL);
						for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
							if (TimeSigSTAFF(aTimeSigL)+stfDiff==DNoteSTAFF(doc,pTime->newSubL))
								newObjL = pTime->newObjL;
						break;
					case KEYSIGtype:
						aKeySigL = FirstSubLINK(pL);
						for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
							if (KeySigSTAFF(aKeySigL)+stfDiff==DNoteSTAFF(doc,pTime->newSubL))
								newObjL = pTime->newObjL;
						break;
					case GRSYNCtype:
						aGRNoteL = FirstSubLINK(pL);
						for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
							if (GRNoteSTAFF(aGRNoteL)+stfDiff==DNoteSTAFF(doc,pTime->newSubL))
								newObjL = pTime->newObjL;
						break;
				}
			}

	if (!newObjL)
		newObjL = endMeasL;

	copyL = InsertClJIPBefore(doc, pL, newObjL, mergeMap);
	InstallDoc(doc);
	FixStfAndVoice(copyL,stfDiff,vMap);
	InstallDoc(clipboard);
}

/* ----------------------------------------------------------- LocateClGenlJDObj -- */
/* Find the pt into which to insert the J_D object pL in the rearranged score
object list, and call InsertJDBefore to insert it there. For J_D objects which
can be relative to any J_IT or J_IP object. Must be called after all J_IP objects
have been re-inserted in the object list, so that we can be sure to have the
link relative to which to re-insert the J_D object. */

static void LocateClGenlJDObj(Document *doc, LINK pL, LINK startClMeas, LINK endClMeas,
								LINK startMeas, LINK endMeas, PTIME *durArray,
								short stfDiff, COPYMAP *mergeMap, short *vMap)
{
	LINK firstL,newObjL,lastL,newLastL,copyL,qL;
	PTIME *pTime; short numObjs=0;

	for (qL=RightLINK(clipFirstMeas); qL!=clipboard->tailL; numObjs++,qL=RightLINK(qL)) ;

	switch (ObjLType(pL)) {
		case GRAPHICtype:
			newObjL = NILINK;
			firstL = GraphicFIRSTOBJ(pL);

			if (MeasureTYPE(firstL)) {
				if (firstL==startClMeas)
					newObjL = startMeas;
				else if (firstL==endClMeas)
					newObjL = endMeas;
			}
			else if (SyncTYPE(firstL)) {
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->objL==firstL) {
						if (DNoteVOICE(doc,pTime->newSubL)==vMap[GraphicVOICE(pL)]) {
							newObjL = pTime->newObjL;
							break;
						}
					}
			}
			else
				newObjL = GetCopyMap(firstL,numObjs,mergeMap);

			if (newObjL) {
				copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
				InstallDoc(doc);
				FixStfAndVoice(copyL,stfDiff,vMap);
				FixGraphicFont(doc, copyL);
				InstallDoc(clipboard);
			}
			break;

		case TEMPOtype:
			newObjL = NILINK;
			firstL = TempoFIRSTOBJ(pL);

			if (MeasureTYPE(firstL)) {
				if (firstL==startClMeas)
					newObjL = startMeas;
				else if (firstL==endClMeas)
					newObjL = endMeas;
			}
			else if (SyncTYPE(firstL)) {
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->objL==firstL) {
						if (DNoteSTAFF(doc,pTime->newSubL)==TempoSTAFF(pL)+stfDiff) {
							newObjL = pTime->newObjL;
							break;
						}
					}
			}
			else
				newObjL = GetCopyMap(firstL,numObjs,mergeMap);

			if (newObjL) {
				copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
				InstallDoc(doc);
				FixStfAndVoice(copyL,stfDiff,vMap);
				InstallDoc(clipboard);
			}
			break;

		case ENDINGtype:
			newObjL = NILINK;
			firstL = EndingFIRSTOBJ(pL);

			if (SyncTYPE(firstL)) {
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->objL==firstL) {
						if (DNoteSTAFF(doc,pTime->newSubL)==EndingSTAFF(pL)+stfDiff) {
							newObjL = pTime->newObjL;
							break;
						}
					}
			}

			if (newObjL) {
				lastL = EndingLASTOBJ(pL);
				newLastL = NILINK;

				if (SyncTYPE(lastL))
					for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
						if (pTime->objL==lastL) {
							if (DNoteSTAFF(doc,pTime->newSubL)==EndingSTAFF(pL)+stfDiff) {
								newLastL = pTime->newObjL;
								break;
							}
						}
				copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
				InstallDoc(doc);
				FixStfAndVoice(copyL,stfDiff,vMap);
				InstallDoc(clipboard);
			}
			break;

		case BEAMSETtype:
			if (GraceBEAM(pL)) {
				firstL = FirstInBeam(pL);
				newObjL = GetCopyMap(firstL,numObjs,mergeMap);

				copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
				InstallDoc(doc);
				FixStfAndVoice(copyL,stfDiff,vMap);
				InstallDoc(clipboard);
			}
			break;

		default:
			;
	}
}


/* --------------------------------------------------------------- LocateClJDObj -- */
/* Find the pt into which to insert the J_D object pL in the rearranged score
object list, and call InsertJDBefore to insert it there. For J_D objects which
can only be relative to SYNCs.

#1. Want the voice in the document of the new subObject, not that of an object in
the clipboard.*/

static void LocateClJDObj(Document *doc, LINK pL, LINK baseMeasL, PTIME *durArray,
								short stfDiff, COPYMAP *mergeMap, short *vMap)
{
	LINK firstL,newObjL,lastL,newLastL,copyL;
	PTIME *pTime;

	switch (ObjLType(pL)) {
		case BEAMSETtype:
			if (!GraceBEAM(pL)) {
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->beamL==pL) {
						if (DNoteVOICE(doc,pTime->newSubL)==vMap[BeamVOICE(pL)]) {		/* #1. */
							newObjL = pTime->newObjL;
							break;
						}
					}
				copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
				InstallDoc(doc);
				FixStfAndVoice(copyL,stfDiff,vMap);
				InstallDoc(clipboard);
			}
			break;
		case OTTAVAtype:
			for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
				if (pTime->octL==pL) {
					if (DNoteSTAFF(doc,pTime->newSubL)==OttavaSTAFF(pL)+stfDiff) {
						newObjL = pTime->newObjL;
						break;
					}
				}
			copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
			InstallDoc(doc);
			FixStfAndVoice(copyL,stfDiff,vMap);
			InstallDoc(clipboard);
			break;
		case TUPLETtype:
			for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
				if (pTime->tupletL==pL) {
					if (DNoteVOICE(doc,pTime->newSubL)==vMap[TupletVOICE(pL)]) {
						newObjL = pTime->newObjL;
						break;
					}
				}
			copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
			InstallDoc(doc);
			FixStfAndVoice(copyL,stfDiff,vMap);
			InstallDoc(clipboard);
			break;
		case DYNAMtype:
			if (IsHairpin(pL)) {
				newObjL = NILINK;
				firstL = DynamFIRSTSYNC(pL);
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->objL==firstL) {
						if (DNoteSTAFF(doc,pTime->newSubL)==DynamicSTAFF(FirstSubLINK(pL))+stfDiff) {
							newObjL = pTime->newObjL;
							break;
						}
					}
	
				if (newObjL) {
					lastL = DynamLASTSYNC(pL);
					newLastL = NILINK;
					for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
						if (pTime->objL==lastL) {
							if (DNoteSTAFF(doc,pTime->newSubL)==DynamicSTAFF(FirstSubLINK(pL))+stfDiff) {
								newLastL = pTime->newObjL;
								break;
							}
						}

					copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
					InstallDoc(doc);
					FixStfAndVoice(copyL,stfDiff,vMap);
					InstallDoc(clipboard);
				}
			}
			else {
				newObjL = NILINK;
				firstL = DynamFIRSTSYNC(pL);
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (pTime->objL==firstL)
						if (DNoteSTAFF(doc,pTime->newSubL)==DynamicSTAFF(FirstSubLINK(pL))+stfDiff) {
							newObjL = pTime->newObjL;
							break;
						}
					
				if (newObjL) {
					copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
					InstallDoc(doc);
					FixStfAndVoice(copyL,stfDiff,vMap);
					InstallDoc(clipboard);
				}
			}
			break;
		case SLURtype:
			newObjL = copyL = NILINK;
			
			if (SlurLastIsSYSTEM(pL))
				newObjL = RightLINK(baseMeasL);
			else
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++) {
					if (SlurTIE(pL)) {
						if (pTime->tieFirstL==pL)
							if (DNoteVOICE(doc,pTime->newSubL)==vMap[SlurVOICE(pL)])
								{ newObjL = pTime->newObjL; break; }
					}
					else {
						if (pTime->slurFirstL==pL)
							if (DNoteVOICE(doc,pTime->newSubL)==vMap[SlurVOICE(pL)])
								{ newObjL = pTime->newObjL; break; }
					}
				}
			if (newObjL)
				copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
			else {
				for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
					if (SlurFIRSTSYNC(pL)==pTime->objL)
						{ newObjL = pTime->newObjL; break; }
				if (newObjL)
					copyL = InsertClJDBefore(doc, pL, newObjL, mergeMap);
			}
			if (copyL) {
				InstallDoc(doc);
				FixStfAndVoice(copyL,stfDiff,vMap);
				InstallDoc(clipboard);
			}
			break;
		default:
			;
	}
}


/* -------------------------------------------------------------- RelocateClObjs -- */
/* Relocate all J_IT & J_IP objects from the clipboard, and all J_D objects
from the clipboard which can only be relative to SYNCs. */

void RelocateClObjs(Document *doc, LINK startClMeas, LINK endClMeas, LINK startMeas,
						LINK endMeas, PTIME *durArray, short stfDiff, COPYMAP *mergeMap,
						short *vMap)
{
	LINK pL, nextL, nextSync,qL; short objType;
	
	InstallDoc(clipboard);

	pL = startClMeas;
	for ( ; pL!=endClMeas; pL=nextL) {
		nextL = RightLINK(pL);
		switch (JustTYPE(pL)) {
			case J_IT:
				objType = ObjLType(pL);
				if (objType==SPACERtype || objType==RPTENDtype || objType==PSMEAStype) {
					for (nextSync=NILINK, qL=pL; qL && qL!=endClMeas; qL=RightLINK(qL))
						if (SyncTYPE(qL))
							{ nextSync = qL; break; }
					LocateClJITObj(doc,pL,nextSync,endMeas,durArray,stfDiff,mergeMap,vMap);
				}
				break;
			case J_IP:
				for (nextSync=NILINK, qL=pL; qL && qL!=endClMeas; qL=RightLINK(qL))
					if (SyncTYPE(qL)) 
						{ nextSync = qL; break; }
				LocateClJIPObj(doc,pL,nextSync,endMeas,durArray,stfDiff,mergeMap,vMap);
				break;
			case J_D:
				if (!GenlJ_DTYPE(pL))		/* GenlJ_DTYPE: Graphic || Ending || Tempo */
					LocateClJDObj(doc,pL,startMeas,durArray,stfDiff,mergeMap,vMap);		
				break;
			case J_STRUC:
				break;
		}
	}
	
	InstallDoc(doc);
}


/* -------------------------------------------------------- RelocateClGenlJDObjs -- */
/* Relocate all J_D objects to be merged in from the clipboard which can be
relative to J_IT objects other than SYNCs or J_IP objects. */
	
void RelocateClGenlJDObjs(Document *doc, LINK startClMeas, LINK endClMeas, LINK startMeas,
									LINK endMeas, PTIME *durArray, short stfDiff, COPYMAP *mergeMap,
									short *vMap)
{
	LINK pL,nextL;

	InstallDoc(clipboard);

	if (RightLINK(startClMeas) != endClMeas)
		for (pL = RightLINK(startClMeas); pL!=endClMeas; pL=nextL) {
			nextL = RightLINK(pL);
			switch(JustTYPE(pL)) {
				case J_IT:
				case J_IP:
					break;
				case J_D:
					if (GenlJ_DTYPE(pL) || BeamsetTYPE(pL))
						LocateClGenlJDObj(doc,pL,startClMeas,endClMeas,startMeas,endMeas,durArray,stfDiff,mergeMap,vMap);
					break;
				case J_STRUC:
					break;
			}
		}
	
	InstallDoc(doc);
}

/* --------------------------------------------------- GetFirstBeam/Tuplet/Ottava -- */
/* The following functions return the first owning object in the object list for
any of the notes in the sync argument. */

LINK GetFirstBeam(LINK syncL)
{
	LINK beamL, firstBeam=NILINK, aNoteL;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
		if (NoteBEAMED(aNoteL)) {
			beamL = LVSearch(syncL, BEAMSETtype, NoteVOICE(aNoteL), TRUE, FALSE);
			if (firstBeam) {
				if (IsAfterIncl(beamL, firstBeam))
					firstBeam = beamL;
			}
			else firstBeam = beamL;
		}
	return firstBeam;
}

LINK GetFirstTuplet(LINK syncL)
{
	LINK tupletL, firstTuplet=NILINK, aNoteL;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
		if (NoteINTUPLET(aNoteL)) {
			tupletL = LVSearch(syncL, TUPLETtype, NoteVOICE(aNoteL), TRUE, FALSE);
			if (firstTuplet) {
				if (IsAfterIncl(tupletL, firstTuplet))
					firstTuplet = tupletL;
			}
			else firstTuplet = tupletL;
		}
	return firstTuplet;
}

LINK GetFirstOttava(LINK syncL)
{
	LINK octL, firstOct=NILINK, aNoteL;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
		if (NoteINOTTAVA(aNoteL)) {
			octL = LSSearch(syncL, OTTAVAtype, NoteSTAFF(aNoteL), TRUE, FALSE);
			if (firstOct) {
				if (IsAfterIncl(octL, firstOct))
					firstOct = octL;
			}
			else firstOct = octL;
		}
	return firstOct;
}


/* ---------------------------------------------------------------- GetFirstSlur -- */

LINK GetFirstSlur(PTIME *durArray)
{
	PTIME *pTime; LINK firstSlur=NILINK;
	
	for (pTime=durArray; pTime->pTime<BIGNUM; pTime++) {

		if (pTime->slurFirstL)
			if (firstSlur==NILINK)
				firstSlur = pTime->slurFirstL;
			else
				if (IsAfterIncl(pTime->slurFirstL, firstSlur))
					firstSlur = pTime->slurFirstL;

		if (pTime->slurLastL)
			if (firstSlur==NILINK)
				firstSlur = pTime->slurLastL;
			else
				if (IsAfterIncl(pTime->slurLastL, firstSlur))
					firstSlur = pTime->slurLastL;

		if (pTime->tieFirstL)
			if (firstSlur==NILINK)
				firstSlur = pTime->tieFirstL;
			else
				if (IsAfterIncl(pTime->tieFirstL, firstSlur))
					firstSlur = pTime->tieFirstL;

		if (pTime->tieLastL)
			if (firstSlur==NILINK)
				firstSlur = pTime->tieLastL;
			else
				if (IsAfterIncl(pTime->tieLastL, firstSlur))
					firstSlur = pTime->tieLastL;
	}
	
	return firstSlur;
}


/* ----------------------------------------------------------------- GetBaseLink -- */
/* Get the correct base link from which to fix up pointers at the end of 
RearrangeNotes. If the first sync in the selection range is beamed/in Ottava/
in Tuplet, the owning object could be anywhere prior to the selection range; 
find it and return it; else return the startMeas. As of v. 3.0, this is unused. */

LINK GetBaseLink(Document *doc, short type, LINK startMeasL)
{
	LINK syncL, aNoteL, beamL, octL, tupletL;
	
	syncL = LSSearch(doc->selStartL, SYNCtype, ANYONE, FALSE, FALSE);
	aNoteL = FirstSubLINK(syncL);

	switch (type) {
		case BEAMSETtype:
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteBEAMED(aNoteL)) {
					if (beamL = GetFirstBeam(syncL))
						return (IsAfterIncl(beamL, startMeasL) ? beamL : startMeasL);
					else
						return startMeasL;
				}
			return startMeasL;
		case TUPLETtype:
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteINTUPLET(aNoteL)) {
					if (tupletL = GetFirstTuplet(syncL))
						return (IsAfterIncl(tupletL, startMeasL) ? tupletL : startMeasL);
					else
						return startMeasL;
				}
			return startMeasL;
		case OTTAVAtype:
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteINOTTAVA(aNoteL)) {
					if (octL = GetFirstOttava(syncL))
						return (IsAfterIncl(octL, startMeasL) ? octL : startMeasL);
					else
						return startMeasL;
				}
			return startMeasL;

		default:
			;
	}
	return NILINK;
}

/* ----------------------------------------------------------------- FixSlurLinks -- */
/* Use durArray to update slurL's firstSyncL and lastSyncL fields.
#1. If a sync with notes in more than one voice is split into more than one object
by the operation we're reconstructing for, then, without this check, we could find
the old sync for a note in a voice which is mapped to a new  sync different than
the one to which the slurred note belongs. */

static void FixSlurLinks(LINK slurL, PTIME *durArray)
{
	PTIME *pTime;

	for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
		if (SlurFIRSTSYNC(slurL)==pTime->objL)
			if (NoteVOICE(pTime->newSubL)==SlurVOICE(slurL))		/* #1. */
				{ SlurFIRSTSYNC(slurL) = pTime->newObjL; break; }

	for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
		if (SlurLASTSYNC(slurL)==pTime->objL)
			if (NoteVOICE(pTime->newSubL)==SlurVOICE(slurL))		/* #1. */
				{ SlurLASTSYNC(slurL) = pTime->newObjL; break; }
}

/* ---------------------------------------------------------------- FixDynamLinks -- */
/* Use durArray to update dynamL's firstSyncL and lastSyncL fields.
#1. See above for slurs; however, dynamics do not have a voice; simply need to
check staffn. This could lead to ambiguous results in case described for slurs;
however, in this case the ambiguity cannot lead to incorrect object list, and is
inherent in the object list anyway - there is no way to resolve it, since hairpins
are attached to syncs, not to notes. */

static void FixDynamLinks(LINK dynamL, PTIME *durArray)
{
	PTIME *pTime;

	if (IsHairpin(dynamL)) {
		for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
			if (DynamFIRSTSYNC(dynamL)==pTime->objL)
				if (NoteSTAFF(pTime->newSubL)==DynamicSTAFF(FirstSubLINK(dynamL)))		/* #1. */
					{ DynamFIRSTSYNC(dynamL) = pTime->newObjL; break; }
	
		for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
			if (DynamLASTSYNC(dynamL)==pTime->objL)
				if (NoteSTAFF(pTime->newSubL)==DynamicSTAFF(FirstSubLINK(dynamL)))		/* #1. */
					{ DynamLASTSYNC(dynamL) = pTime->newObjL; break; }
	}
}

/* -------------------------------------------------------------- FixEndingLinks -- */
/* Use durArray to update endingL's firstObjL and lastObjL fields.
#1. See note for dynamics. */

static void FixEndingLinks(LINK endingL, PTIME *durArray)
{
	PTIME *pTime;

	for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
		if (EndingFIRSTOBJ(endingL)==pTime->objL)
			if (NoteSTAFF(pTime->newSubL)==EndingSTAFF(endingL))		/* #1. */
				{ EndingFIRSTOBJ(endingL) = pTime->newObjL; break; }

	for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
		if (EndingLASTOBJ(endingL)==pTime->objL)
			if (NoteSTAFF(pTime->newSubL)==EndingSTAFF(endingL))		/* #1. */
				{ EndingLASTOBJ(endingL) = pTime->newObjL; break; }
}

/* -------------------------------------------------------------- FixGRDrawLinks -- */
/* Use durArray to update a GRDraw Graphic's firstObj and lastObj fields.
#1. If a Sync with notes in more than one voice is split into more than one object
by the operation we're reconstructing for, then, without this check, we could find
the old Sync for a note in a voice which is mapped to a new Sync different than
the one to which the note with the attached Graphic belongs. */

static void FixGRDrawLinks(LINK graphicL, PTIME *durArray)
{
	PTIME *pTime; LINK firstL, lastL;

	firstL = GraphicFIRSTOBJ(graphicL);
	lastL = GraphicLASTOBJ(graphicL);
	if (!(SyncTYPE(firstL) || GRSyncTYPE(firstL))
	||  !(SyncTYPE(lastL) || GRSyncTYPE(lastL) || MeasureTYPE(lastL)) )
		MayErrMsg("FixGRDrawLinks: for GRDraw %d, firstObj=%d and/or lastObj=%d is an illegal type.",
					graphicL, firstL, lastL);
	
	for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
		if (firstL==pTime->objL)
			if (NoteVOICE(pTime->newSubL)==GraphicVOICE(graphicL))		/* #1. */
				{ firstL = pTime->newObjL; break; }

	for (pTime = durArray; pTime->pTime<BIGNUM; pTime++)
		if (lastL==pTime->objL)
			if (NoteVOICE(pTime->newSubL)==GraphicVOICE(graphicL))		/* #1. */
				{ lastL = pTime->newObjL; break; }
}


/* ----------------------------------------------------------------- FixNBJDLinks -- */
/* Use the durArray to update slur's firstSyncL and lastSyncL fields for all slurs
in the range [startL, endL). If the spareFlag of the object is set, then the object
was copied from the clipboard; mergeMap will be used to update its ptrs after all
measures are copied in. FIXME: I SERIOUSLY DOUBT THAT COMMENT IS ACCURATE.  --DAB */

void FixNBJDLinks(LINK startL, LINK endL, PTIME *durArray)
{
	LINK pL;
	
	for (pL = startL; pL!=endL; pL=RightLINK(pL))
		if (!LinkSPAREFLAG(pL))
			switch (ObjLType(pL)) {
				case SLURtype:
					FixSlurLinks(pL, durArray);
					break;
				case DYNAMtype:
					FixDynamLinks(pL, durArray);
					break;
				case ENDINGtype:
					FixEndingLinks(pL, durArray);
					break;
				case GRAPHICtype:
					if (GraphicSubType(pL)==GRDraw)
						FixGRDrawLinks(pL, durArray);
					break;
			}
}


/* ---------------------------------------------------------------- FixCrossPtrs -- */
/* Update all cross links (not "ptrs") after reconstructing the object list.
NB: clDurArray is unused and should be removed from calling sequence. */

void FixCrossPtrs(Document *doc, LINK startMeas, LINK endMeas, PTIME *durArray,
						PTIME */*clDurArray*/)
{
	LINK sysL, firstSysL, firstMeasL;

	sysL = SSearch(startMeas,SYSTEMtype,GO_LEFT);
	firstSysL = LinkLSYS(sysL) ? LinkLSYS(sysL) : sysL;

	/* Beamsets can begin in a previous system; crossSys beamsets
		can begin in the system before the previous system. */

	FixAllBeamLinks(doc, doc, firstSysL, endMeas);
	
	/* Rules are not clearly stated in FixGroupsMenu,NTypes.h, or DoOttava
		for Ottavas. Assuming we do not have crossSys Ottavas. Must update
		all Ottavas which can have octNotes in the measure; Ottavas located
		after endMeas are guaranteed to have none. */
	
	FixOttavaLinks(doc, doc, sysL, endMeas);

	/* Tuplets must begin and end in the same measure. */

	FixTupletLinks(doc, doc, startMeas, endMeas);

	/* NBJD objects which start before and end in the measure being processed
		will remain in place in the object list, and their firstSyncL/firstObjL
		will remain valid across the operation. This call to FixNBJDLinks will
		properly update their lastSyncL/lastObjL field. NB: As written, this is
		way overkill: it operates on the entire score! */

	firstMeasL = SSearch(doc->headL, MEASUREtype, GO_RIGHT);
	FixNBJDLinks(firstMeasL, doc->tailL, durArray);
}


/* ---------------------------------------------------------- FixStfAndVoice et al -- */
/* Update staff and voice numbers: for merging objects, both into staves other than
those they came from, and into any staff when Look at Voice is in effect. */

static void FixVoiceNum(LINK pL,short stfDiff,short *vMap);
static void FixStaffn(LINK pL,short stfDiff);

static void FixVoiceNum(LINK pL,
								short /*stfDiff*/,		/* obsolete; ignored */
								short *vMap)
{
	LINK aNoteL,aGRNoteL;

	switch (ObjLType(pL)) {
		case SYNCtype:
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				NoteVOICE(aNoteL) = vMap[NoteVOICE(aNoteL)];
			}
			break;
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				GRNoteVOICE(aGRNoteL) = vMap[GRNoteVOICE(aGRNoteL)];
			}
			break;
		case SLURtype:
			SlurVOICE(pL) = vMap[SlurVOICE(pL)];
			break;
		case BEAMSETtype:
			BeamVOICE(pL) = vMap[BeamVOICE(pL)];
			break;
		case TUPLETtype:
			TupletVOICE(pL) = vMap[TupletVOICE(pL)];
			break;
		case GRAPHICtype:
			GraphicVOICE(pL) = vMap[GraphicVOICE(pL)];
			break;
		default:
			;
	}
}

static void FixStaffn(LINK pL, short stfDiff)
{
	LINK aNoteL,aGRNoteL,aDynamL,aClefL,aKSL,aTSL;

	switch (ObjLType(pL)) {
		case SYNCtype:
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				NoteSTAFF(aNoteL) += stfDiff;
			break;
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
				GRNoteSTAFF(aGRNoteL) += stfDiff;
			break;
		case CLEFtype:
			aClefL = FirstSubLINK(pL);
			for ( ; aClefL; aClefL = NextCLEFL(aClefL))
				ClefSTAFF(aClefL) += stfDiff;
			break;
		case KEYSIGtype:
			aKSL = FirstSubLINK(pL);
			for ( ; aKSL; aKSL = NextKEYSIGL(aKSL))
				KeySigSTAFF(aKSL) += stfDiff;
			break;
		case TIMESIGtype:
			aTSL = FirstSubLINK(pL);
			for ( ; aTSL; aTSL = NextTIMESIGL(aTSL))
				TimeSigSTAFF(aTSL) += stfDiff;
			break;
		case DYNAMtype:
			aDynamL=FirstSubLINK(pL);
			for ( ; aDynamL; aDynamL = NextDYNAMICL(aDynamL))
				DynamicSTAFF(aDynamL) += stfDiff;
			break;
		case SLURtype:
			SlurSTAFF(pL) += stfDiff;
			break;
		case BEAMSETtype:
			BeamSTAFF(pL) += stfDiff;
			break;
		case TUPLETtype:
			TupletSTAFF(pL) += stfDiff;
			break;
		case OTTAVAtype:
			OttavaSTAFF(pL) += stfDiff;
			break;
		case SPACERtype:
			SpacerSTAFF(pL) += stfDiff;
			break;
		case TEMPOtype:
			TempoSTAFF(pL) += stfDiff;
			break;
		case ENDINGtype:
			EndingSTAFF(pL) += stfDiff;
			break;
		case GRAPHICtype:
			GraphicSTAFF(pL) += stfDiff;
			break;
		default:
			;
	}
}

void FixStfAndVoice(LINK pL, short stfDiff, short *vMap)
{
	FixVoiceNum(pL,stfDiff,vMap);
	FixStaffn(pL,stfDiff);
}
