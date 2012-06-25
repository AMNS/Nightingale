/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

/* File CompactVoices.c - functions to remove gaps in voices - MemMacroized version.

CVDisposeArrays		CVPrepareSelRange			CVComputePlayTimes
CVRearrangeNotes		DoCompactVoices			SDCVComputePlayTimes
SetDurCptV				CompactVoices

*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static PTIME			*pDurArray, *qDurArray;
static LINK				startMeas, endMeas;
static SPACETIMEINFO	*spTimeInfo;
static long 			*stfTimeDiff;

/* Prototypes for internal functions */

static void CVDisposeArrays(void);
static PTIME *CVPrepareSelRange(Document *doc,LINK measL,short *nInRange);
static void CVShellSort(short n, short nvoices);
static void CVSortPTimes(short nInSelRange, short nvoices);
static void TuplePlayDurs(PTIME *, short, LINK, LINK);
static Boolean CVComputePlayTimes(Document *doc, SELRANGE [], char[], short);

static Boolean CVRearrangeNotes(Document *doc, SELRANGE [], short, LINK);

static Boolean CheckCompactVoice(Document *);

static void InitSelRange(SELRANGE selRange[]);
static void SetupSelRange(Document *doc,SELRANGE selRange[]);
static void GetMeasVRange(Document *doc,LINK measL,SELRANGE selRange[],char measRange[]);

static Boolean SDCVComputePlayTimes(Document *doc, SELRANGE [], char[], short);

/* -------------------------------------------------------------------------------- */
/* Utilities for CompactVoices. */

/*
 * Dispose compactVoice arrays. pDurArray is allocated by CVPrepareSelRange;
 * qDurArray is allocated by CVRearrangeNotes; the others by CVComputePlayTimes.
 */

static void CVDisposeArrays()
{
	if (pDurArray)
		{ DisposePtr((Ptr)pDurArray); pDurArray = NULL; }
	if (qDurArray)
		{ DisposePtr((Ptr)qDurArray); qDurArray = NULL; }
	if (spTimeInfo)
		{ DisposePtr((Ptr)spTimeInfo); spTimeInfo = NULL; }
	if (stfTimeDiff)
		{ DisposePtr((Ptr)stfTimeDiff); stfTimeDiff = NULL; }
}

/* ------------------------------------------------------------ CVPrepareSelRange -- */
/* Prepare array for use by CompactVoices. */

static PTIME *CVPrepareSelRange(Document *doc, LINK measL, short *nInRange)
{
	short nInMeas = 0, numNotes;
	LINK pL;
	
	startMeas = measL;
	endMeas = EndMeasSearch(doc,startMeas);

	for (pL = startMeas; pL!=endMeas; pL = RightLINK(pL))
		if (SyncTYPE(pL)) nInMeas++;

	/* numNotes provides one note per voice for all syncs in selection range. */ 
	numNotes = nInMeas*(MAXVOICES+1);

	/* Allocate pDurArray[numNotes]. First initialize the unused 0'th row of
		the array, then the rest of the rows. Set pTimes of 0'th row to BIGNUM to
		sort them above the world. */

	pDurArray = (PTIME *)NewPtr((Size)numNotes*sizeof(PTIME));
	if (!GoodNewPtr((Ptr)pDurArray))
		{ NoMoreMemory(); return NULL; }

	InitDurArray(pDurArray,nInMeas);

	*nInRange = nInMeas;
	return pDurArray;
}

/* ------------------------------------------------------------------ CVShellSort -- */
/* Sort the pDurArray (via Shell sort). */

static void CVShellSort(short n, short nvoices)
{
	short gap, i, j, arrLen;
	PTIME t;

	arrLen = n*nvoices;
	for (gap=arrLen/2; gap>0; gap /= 2)
		for (i=gap; i<arrLen; i++)
			for (j=i-gap; 
				  j>=0 && pDurArray[j].pTime>pDurArray[j+gap].pTime; 
				  j-=gap) {
				t = pDurArray[j];
				pDurArray[j] = pDurArray[j+gap];
				pDurArray[j+gap]=t;
			}
}


/* ------------------------------------------------------------------- SortPTimes -- */
/* Sort the pTime array pDurArray based on the pTime field. */

static void CVSortPTimes(short nInMeas, short nvoices)
{
	CVShellSort(nInMeas, nvoices);
}


/* ----------------------------------------------------------- CVComputePlayTimes -- */
/* Version of ComputePlayTimes for CompactVoices */

static Boolean CVComputePlayTimes(Document *doc, SELRANGE /*selRange*/[], char measRange[],
												short nInMeas)
{
	short v, notes, prevNotes; LINK pL,aNoteL,prevL,prevNoteL;
	long currTime,totalGap,gapTime,prevTime,prevDur,prevNoteEnd;
	Boolean firstNote;

	/* Allocate objects. */

	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) goto broken;
	stfTimeDiff = (long *)NewPtr((Size)(MAXVOICES+1) * sizeof(long));
	if (!GoodNewPtr((Ptr)stfTimeDiff)) goto broken;
	
	/* Set playDur values and pTime values for the pDurArray, and set link values
		for owning beams, octavas and tuplets. */

	SetPlayDurs(doc,pDurArray,nInMeas,startMeas,endMeas);
	SetPTimes(doc,pDurArray,nInMeas,spTimeInfo,startMeas,endMeas);
	SetLinkOwners(pDurArray,nInMeas,startMeas,endMeas);

	/* Compute time to be removed from each voice given holes in
		voices. */

	for (v = 1; v<=MAXVOICES; v++)
		stfTimeDiff[v] = 0L;

	for (v = 1; v<=MAXVOICES; v++)
		if (measRange[v]) {
	
			notes=0;
			totalGap = gapTime = 0L;
			firstNote = TRUE;
			
			/* Traverse all syncs in the measure. If firstNote is TRUE, and notes>0, then
				the first note in the voice is not in the first sync in the measure; it
				must be left justified by subtracting from its time the currTime.
				Otherwise, determine if there is a gap; if there is, reduce the time
				of the sync after the gap by the amount of the gap. */

			for (pL=startMeas; pL!=endMeas; pL=RightLINK(pL))
				if (SyncTYPE(pL)) {
					for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
						if (MainNoteInVOICE(aNoteL, v)) {
							/* aNote = GetPANOTE(aNoteL); */
							currTime = (pDurArray + v*nInMeas+notes)->pTime;

							if (notes>0) {
								if (firstNote) {
									firstNote = FALSE;
									gapTime = totalGap = currTime;
								}
								else {
									prevTime = (pDurArray + v*nInMeas+prevNotes)->pTime;
									prevL = (pDurArray + v*nInMeas+prevNotes)->objL;
									prevNoteL = (pDurArray + v*nInMeas+prevNotes)->subL;

									prevDur = CalcNoteLDur(doc, prevNoteL, prevL);
									prevNoteEnd = prevTime+prevDur;
									if (currTime>prevNoteEnd) {
										gapTime = currTime-prevNoteEnd;
										totalGap += gapTime;
									}
								}
							}
							else firstNote = FALSE;

							prevNotes = notes;
	
							(pDurArray + v*nInMeas+notes)->pTime -= gapTime;
							NotePTIME(aNoteL) -= gapTime;
	
						}
					notes++;
				}
		}

	/* Sort the pDurArray in order of increasing pTimes. */

	CVSortPTimes(nInMeas, MAXVOICES+1);
	return TRUE;
	
broken:
	NoMoreMemory();
	return FALSE;
}


/* ------------------------------------------------------------- CVRearrangeNotes -- */

static Boolean CVRearrangeNotes(Document *doc, SELRANGE /*selRange*/[], short nNotes,
											LINK /*baseMeasL*/)
{
	short i, v, notes, numNotes, subCount=0, arrBound;
	PTIME *pTime, *qTime;
	LINK pL, firstSubObj, newObjL, subL, newSubL, tempNewSubL, newSelStart;
	PMEVENT pObj, pNewObj;
	Boolean objSel;
	LINK headL,tailL,initL,prevL,insertL,firstL,lastL;

	pTime=pDurArray;
	newSelStart = NILINK;
	
	/* Compact the pDurArray: allocate qDurArray, copy into it
		only those pTimes s.t. pTime->subL!= NILINK, and then fill
		in the rest of the entries with default values. */

	numNotes = nNotes*(MAXVOICES+1);
	if (qDurArray = (PTIME *)NewPtr((Size)numNotes*sizeof(PTIME))) {
			pTime = pDurArray;
			qTime = qDurArray;
			for (notes=0, i=0; notes<nNotes*(MAXVOICES+1); notes++)
				if (pTime->subL) 
					{ *qTime = *pTime; i++; pTime++; qTime++; }
				else pTime++;
			arrBound = i;
			for ( ; i<nNotes*(MAXVOICES+1); i++) {
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
	}
	else {
		NoMoreMemory();
		return FALSE;
	}

	pTime = qDurArray;

#ifdef DEBUG_QDURARRAY
	DebugDurArray(arrBound,qDurArray);
#endif
	
	initL = prevL = startMeas;
	insertL = endMeas;
	firstL = RightLINK(startMeas);
	lastL = LeftLINK(endMeas);

	/* Link the selection range into a temporary data structure. This is
		simply to store the non-Sync nodes until they can be returned to the
		data structure, instead of calling:
		DeleteRange(doc, startMeas, endMeas). */

	headL = NewNode(doc, HEADERtype, 2);
	if (!headL) goto broken1;
	tailL = NewNode(doc, TAILtype, 0);
	if (!tailL) { DeleteNode(doc, headL); goto broken1; }

	LeftLINK(headL) = RightLINK(tailL) = NILINK;

	RightLINK(startMeas) = endMeas;
	LeftLINK(endMeas) = startMeas;

	RightLINK(headL) = firstL;
	LeftLINK(tailL) = lastL;
	
	LeftLINK(firstL) = headL;
	RightLINK(lastL) = tailL;
	
	/* Loop through the entire qDurArray, creating syncs out of notes
		which have the same pTimes. */
		
	qTime=pTime;
	for (i=0; i<arrBound; pTime=qTime) {

		subCount = 0;
		objSel = FALSE;

		/* Get the number of notes in this sync. */
		while (qTime->pTime==pTime->pTime) {
			subCount += qTime->mult;								/* Handle chords */
			qTime++; i++;
		}
		
		/* Create the sync object, and copy the first note's object
			information into it. */
		if (subCount) {
			newObjL = NewNode(doc, SYNCtype, subCount);
			if (newObjL == NILINK)
				goto broken;

			pObj    = (PMEVENT)LinkToPtr(OBJheap,pTime->objL);
			pNewObj = (PMEVENT)LinkToPtr(OBJheap,newObjL);
			firstSubObj = FirstSubLINK(newObjL);				/* Save it before it gets wiped out */
			BlockMove(pObj, pNewObj, sizeof(SUPEROBJECT));
			FirstSubLINK(newObjL) = firstSubObj;				/* Restore it. */
			LinkNENTRIES(newObjL) = subCount;					/* Set new nEntries. */
	
			qTime = pTime;
			subL 	  = qTime->subL;
			newSubL = FirstSubLINK(newObjL);
	
			/* Copy the subObject information from each note at this time into
				the subObjects of the newly created object. Decrement the nEntries
				field of the object from which the subObj is copied, and remove that
				subObject, or delete the object, if it has only one subObj left.
				This refers to the sync still in the data structure. */

			while (qTime->pTime==pTime->pTime) {
				if (NoteSEL(subL)) objSel = TRUE;

				if (qTime->mult > 1) {
					v = NoteVOICE(subL);
					subL = FirstSubLINK(qTime->objL);
					for ( ; subL; subL = NextNOTEL(subL))
						if (NoteVOICE(subL)==v)
							newSubL = CopyNotes(doc, subL, newSubL, -1L, qTime->pTime);

					/* Set the newObjL & newSubL fields for use by FixObjLinks &
						GetInsertPt. Use tempNewSubL here, since the else block of this
						if requires newSubL to be set correctly. */

					qTime->newObjL = newObjL;
					tempNewSubL = FirstSubLINK(newObjL);
					for ( ; tempNewSubL; tempNewSubL = NextNOTEL(tempNewSubL))
						if (MainNote(tempNewSubL) && NoteVOICE(tempNewSubL)==v) 
							{ qTime->newSubL = tempNewSubL; break; }

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

			RightLINK(newObjL) = insertL;
			LeftLINK(insertL) = newObjL;
			RightLINK(prevL) = newObjL;
			LeftLINK(newObjL) = prevL;
	
			prevL = newObjL;

		}
	}
	
#ifdef DEBUG_QDURARRAY
	DebugDurArray(arrBound,qDurArray);
#endif

	/* Relocate all J_IP objects and all J_D objects which can only be relative to
		SYNCs. */

	RelocateObjs(doc,headL,tailL,startMeas,endMeas,qDurArray);

	/* Relocate all J_D objects which can be relative to J_IT objects other than
		SYNCs or J_IP objects. */
	
	RelocateGenlJDObjs(doc,headL,tailL,qDurArray);
	
	for (pL = startMeas; pL!=endMeas; pL=RightLINK(pL))
		if (LinkSEL(pL))
			{ doc->selStartL = pL; break; }

	for (pL = endMeas; pL!=startMeas; pL=LeftLINK(pL))
		if (LinkSEL(pL))
			{ doc->selEndL = RightLINK(pL); break; }

	FixCrossPtrs(doc,startMeas,endMeas,qDurArray,NULL);

	DeleteRange(doc, RightLINK(headL), tailL);
	DeleteNode(doc, headL);
	DeleteNode(doc, tailL);
	return TRUE;
	
broken:
	DeleteRange(doc, RightLINK(headL), tailL);
	DeleteNode(doc, headL);
	DeleteNode(doc, tailL);
	
broken1:
	NoMoreMemory();
	return FALSE;
}


static Boolean CheckCompactVoice(Document *doc)
{
	/* If the voices are already continuous, there's nothing to do. */
	
	if (CheckContinVoice(doc)) return FALSE;

	return TRUE;
}


static void InitSelRange(SELRANGE selRange[])
{
	short v;

	for (v = 1; v<=MAXVOICES; v++)
		selRange[v].startL = selRange[v].endL = NILINK;
}


static void SetupSelRange(Document *doc, SELRANGE selRange[])
{
	short v; LINK vStartL,vEndL,startMeasL,endMeasL;

	InitSelRange(selRange);

	for (v = 1; v<=MAXVOICES; v++) {
		GetNoteSelRange(doc, v, &vStartL, &vEndL, NOTES_ONLY);
		if (vStartL && vEndL) {
			startMeasL = LSSearch(vStartL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
			endMeasL = LSSearch(vEndL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
			selRange[v].startL = startMeasL;
			selRange[v].endL = endMeasL;
		}
	}
}


static void GetMeasVRange(Document */*doc*/, LINK measL, SELRANGE selRange[],
									char measRange[])
{
	short v;
	
	for (v=1; v<=MAXVOICES; v++)
		measRange[v] = 0;
		
	for (v=1; v<=MAXVOICES; v++)
		if (BetweenIncl(selRange[v].startL, measL, selRange[v].endL))
			measRange[v] = 1;
}


/* ------------------------------------------------------------- DoCompactVoices -- */
/* Compact voices (remove temporal gaps between notes) in selected voices in the
selected range of Measures. Designed to be called in response to a menu command.
??Should really call CompactVoices instead of repeating its code. (Formerly called
simply CompactVoices.) */

void DoCompactVoices(Document *doc)
{
	short			nInMeas;
	char			measVRange[MAXVOICES+1];
	SELRANGE		selRange[MAXVOICES+1];
	LINK			measL,firstMeasL,lastMeasL,endL, selStartL;

	WaitCursor();
	PrepareUndo(doc, doc->selStartL, U_CompactVoice, 8);		/* "Undo Remove Gaps" */

	SetSpareFlags(doc->headL,doc->tailL,FALSE);
	SetupSelRange(doc,selRange);

	measL = firstMeasL = LSSearch(doc->selStartL,MEASUREtype,ANYONE,GO_LEFT,FALSE);
	if (measL)
		selStartL = measL;
	else {
		measL = firstMeasL = LSSearch(doc->selStartL,MEASUREtype,ANYONE,GO_RIGHT,FALSE);
		selStartL = doc->selStartL;
	}

	lastMeasL = LSSearch(doc->selEndL,MEASUREtype,ANYONE,GO_RIGHT,FALSE);
	endL = lastMeasL ? lastMeasL : doc->tailL;
	
	/* Traverse range to be compacted measure by measure. For each measure, fill
		in measVRange[] to indicate which voices contain selected syncs in the
		measure and will therefore need compacting. Then compact the voices in
		the measure for which measVRange[v] is TRUE. */

	for ( ; measL && measL!=lastMeasL; measL=LinkRMEAS(measL)) {
		GetMeasVRange(doc,measL,selRange,measVRange);

		pDurArray = CVPrepareSelRange(doc,measL,&nInMeas);
		if (!pDurArray) goto broken;
		if (nInMeas<=1) continue;

		if (!CVComputePlayTimes(doc, selRange, measVRange, nInMeas)) goto broken;
		if (!CVRearrangeNotes(doc, selRange, nInMeas, measL)) goto broken;

		CVDisposeArrays();
	}

	/* We need these calls to fix up links for Beams and Octavas which extend
		outside the compacted measure. */

	FixAllBeamLinks(doc,doc,doc->headL,doc->tailL);
	FixOctavaLinks(doc,doc,doc->headL,doc->tailL);

	if (doc->autoRespace)
		RespaceBars(doc, firstMeasL, endL, 0L, FALSE, FALSE);
	else
		InvalMeasures(firstMeasL, endL, ANYONE);

	/* ??If RespaceBars reformatted, it looks like <endL> may no longer exist now! */

	FixTimeStamps(doc,firstMeasL,endL);

	DeselRange(doc, selStartL, endL);
	doc->selStartL = doc->selEndL = endL;
	MEAdjustCaret(doc,TRUE);
	return;

broken:
	DisableUndo(doc,TRUE);
	CVDisposeArrays();
}

/* Version of ComputePlayTimes for SetDurCptV */

static Boolean SDCVComputePlayTimes(Document *doc, SELRANGE  /*selRange*/[],
												char measRange[], short nInMeas)
{
	short v, notes, prevNotes; LINK pL,aNoteL,prevL,prevNoteL;
	long currTime,totalGap,gapTime,prevTime,prevDur,prevNoteEnd;
	Boolean firstNote;

	/* Allocate objects. */

	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) goto broken;
	stfTimeDiff = (long *)NewPtr((Size)(MAXVOICES+1) * sizeof(long));
	if (!GoodNewPtr((Ptr)stfTimeDiff)) goto broken;
	
	/* Set playDur values and pTime values for the pDurArray, and set link values
		for owning beams, octavas and tuplets. */

	SetPlayDurs(doc,pDurArray,nInMeas,startMeas,endMeas);
	SetPTimes(doc,pDurArray,nInMeas,spTimeInfo,startMeas,endMeas);
	SetLinkOwners(pDurArray,nInMeas,startMeas,endMeas);

	/* Compute time to be removed from each voice given holes in voices. */

	for (v = 1; v<=MAXVOICES; v++)
		stfTimeDiff[v] = 0L;

	for (v = 1; v<=MAXVOICES; v++)
		if (measRange[v]) {
	
			notes=0;
			totalGap = gapTime = 0L;
			firstNote = TRUE;
			
			/* Traverse all syncs in the measure. Determine if there is a gap;
				if there is, reduce the time of the sync after the gap by the
				amount of the gap.
				If pL equals doc->selEndL, cease removing gaps:
				the gaps to be removed are all those after the first note in the
				selection range and before the last note; if pL is doc->selEndL, it
				is the first note after the selection range and the gap removed
				before it will be the gap after the last note of the selection
				range. */

			for (pL=startMeas; pL!=endMeas && pL!=doc->selEndL; pL=RightLINK(pL))
				if (SyncTYPE(pL)) {
					for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
						if (MainNoteInVOICE(aNoteL, v)) {
							/* aNote = GetPANOTE(aNoteL); */
							currTime = (pDurArray + v*nInMeas+notes)->pTime;

							if (notes>0) {
								prevTime = (pDurArray + v*nInMeas+prevNotes)->pTime;
								prevL = (pDurArray + v*nInMeas+prevNotes)->objL;
								prevNoteL = (pDurArray + v*nInMeas+prevNotes)->subL;

								prevDur = CalcNoteLDur(doc, prevNoteL, prevL);
								prevNoteEnd = prevTime+prevDur;
								if (currTime>prevNoteEnd) {
									gapTime = currTime-prevNoteEnd;
									totalGap += gapTime;
								}
							}

							prevNotes = notes;
	
							(pDurArray + v*nInMeas+notes)->pTime -= gapTime;
							NotePTIME(aNoteL) -= gapTime;
	
						}
					notes++;
				}
		}

	/* Sort the pDurArray in order of increasing pTimes. */

	CVSortPTimes(nInMeas, MAXVOICES+1);
	return TRUE;
	
broken:
	NoMoreMemory();
	return FALSE;
}


/* ------------------------------------------------------------------ SetDurCptV -- */
/* Compact voices in selection range; intended to be called after a Set Duration
or similar operation. NB: destroys the selection range. */

void SetDurCptV(Document *doc)
{
	short			nInMeas;
	char			measVRange[MAXVOICES+1];
	SELRANGE		selRange[MAXVOICES+1];
	LINK			measL,firstMeasL,lastMeasL,endL;

	/* If voices are already continuous, there are no gaps to compact. */
	
	if (!CheckCompactVoice(doc)) return;

	SetSpareFlags(doc->headL,doc->tailL,FALSE);
	SetupSelRange(doc,selRange);

	measL = firstMeasL = LSSearch(doc->selStartL,MEASUREtype,ANYONE,GO_LEFT,FALSE);
	lastMeasL = LSSearch(doc->selEndL,MEASUREtype,ANYONE,GO_RIGHT,FALSE);
	
	/* Return if before the first measure of the score. Should not be able to get
		here, for if selStartL is before the first measure of the score, the selRange
		should be discontinuous.???NO LONGER: NEEDS ATTENTION */

	if (!firstMeasL) return;
	endL = lastMeasL ? lastMeasL : doc->tailL;
	
	/* Traverse range to be compacted measure by measure. For each measure,
		fill in measVRange[] to indicate which voices contain selected syncs
		in the measure, and will need compacting. Then compact the voices
		in the measure for which measVRange[v] is TRUE. Compact introduced
		gaps between the first and last notes whose duration was modified. */

	for ( ; measL && measL!=lastMeasL; measL=LinkRMEAS(measL)) {
		GetMeasVRange(doc,measL,selRange,measVRange);

		pDurArray = CVPrepareSelRange(doc,measL,&nInMeas);
		if (!pDurArray) goto broken;
		if (nInMeas<=1) continue;

		if (!SDCVComputePlayTimes(doc, selRange, measVRange, nInMeas)) goto broken;
		if (!CVRearrangeNotes(doc, selRange, nInMeas, measL)) goto broken;

		CVDisposeArrays();
	}

	/* We need these calls to fix up links for Beams and Octavas which extend
		outside the compacted measure. */

	FixAllBeamLinks(doc,doc,doc->headL,doc->tailL);
	FixOctavaLinks(doc,doc,doc->headL,doc->tailL);

broken:
	CVDisposeArrays();
}

/* --------------------------------------------------------------- CompactVoices -- */
/* Compact voices (remove temporal gaps between notes) in selected voices in the
selected range of Measures. Makes no user-interface assumptions. */

Boolean CompactVoices(Document *doc)
{
	short			nInMeas;
	char			measVRange[MAXVOICES+1];
	SELRANGE		selRange[MAXVOICES+1];
	LINK			measL,firstMeasL,lastMeasL,endL;

	SetSpareFlags(doc->headL,doc->tailL,FALSE);
	SetupSelRange(doc,selRange);

	measL = firstMeasL = LSSearch(doc->selStartL,MEASUREtype,ANYONE,GO_LEFT,FALSE);
	lastMeasL = LSSearch(doc->selEndL,MEASUREtype,ANYONE,GO_RIGHT,FALSE);
	endL = lastMeasL ? lastMeasL : doc->tailL;
	
	/* Traverse range to be compacted measure by measure. For each measure,
		fill in measVRange[] to indicate which voices contain selected syncs
		in the measure, and will need compacting. Then compact the voices
		in the measure for which measVRange[v] is TRUE. */

	for ( ; measL && measL!=lastMeasL; measL=LinkRMEAS(measL)) {
		GetMeasVRange(doc,measL,selRange,measVRange);

		pDurArray = CVPrepareSelRange(doc,measL,&nInMeas);
		if (!pDurArray) goto broken;
		if (nInMeas<=1) continue;

		if (!CVComputePlayTimes(doc, selRange, measVRange, nInMeas)) goto broken;
		if (!CVRearrangeNotes(doc, selRange, nInMeas, measL)) goto broken;

		CVDisposeArrays();
	}

	/* We need these calls to fix up links for Beams and Octavas which extend
		outside the compacted measure. */

	FixAllBeamLinks(doc,doc,doc->headL,doc->tailL);
	FixOctavaLinks(doc,doc,doc->headL,doc->tailL);

	FixTimeStamps(doc,firstMeasL,endL);
	
	return TRUE;
	
broken:
	CVDisposeArrays();
	return FALSE;
}
