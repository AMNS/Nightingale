/* File Tuplet.c - tuplet-related functions for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
DisposeArrays			VoiceInSelRange			CheckContinVoice
CheckMaxTupleNum		PrepareSelRange			ShellSort
SortPTimes				GetTupleDenom			ComputePlayDurs
RearrangeNotes			DoTuple
GetBracketVis			InitTuplet				SetTupletYPos
CreateTUPLET			SelRangeChkTuplet
PullFromSync			CombineSyncs
Untuple					DoUntuple				UntupleObj
InterpY					DrawTupletBracket		DrawPSTupletBracket
GetTupletInfo			DrawTUPLET et al
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define STD_TUPLET_MARGIN_ABOVE (-5*STD_LINEHT/4)		/* Default position */
#define STD_TUPLET_MARGIN_BELOW (11*STD_LINEHT/4)

static PTIME			*pDurArray, *qDurArray;
static LINK				startMeas, endMeas;
static SPACETIMEINFO	*spTimeInfo;
static long 			*stfTimeDiff;

/* Prototypes for internal functions */

static void DisposeArrays(void);

static PTIME *PrepareSelRange(Document *doc, short *nInRange, LINK *baseMeasL);

static void ShellSort(short n, short nvoices);
static void SortPTimes(short nInSelRange, short nvoices);

static short GetTupleDenom(short tupleNum, TupleParam *tParam);
static Boolean ComputePlayDurs(Document *doc, SELRANGE [], short [], short [], short, TupleParam *);
static Boolean RearrangeNotes(Document *doc, SELRANGE [], short, LINK);
static Boolean CheckContinSel(Document *);

static void SelectSync(LINK pL, short v, Boolean sel);
static void SelectTuplet(Document *doc, short v, Boolean sel);
static void DeleteTuplet(Document *doc,LINK tupletL);
static Boolean RecomputePlayDurs(Document *, SELRANGE [], short [], short [], short, short);
static void MarkTuplets(Document *doc);

static Boolean SelRangeChkTuplet(short, LINK, LINK);


/* --------------------------------------------------------------------------------- */
/* Utilities for DoTuple. */

/* Dispose Tuplet arrays. pDurArray is allocated by PrepareSelRange;
qDurArray is allocated by RearrangeNotes; the others by ComputePlayDurs. */

static void DisposeArrays()
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

/* Return TRUE if both numerator and denominator for tuplet to be created in
voice v are below the allowed maximum of MAX_TUPLENUM = 255. */

Boolean CheckMaxTupleNum(short v, LINK vStartL, LINK vEndL, TupleParam *tParam)
{
	short tupleNum,tupleDenom;

	tupleNum = GetTupleNum(vStartL, vEndL, v, TRUE);
	tupleDenom = GetTupleDenom(tupleNum,tParam);

	return (tupleNum < MAX_TUPLENUM && tupleDenom < MAX_TUPLENUM);
}


/* ----------------------------------------------------------------- GetTupleNum -- */
/* Get the accessory numerator of the tuplet for the range in the voice. 
1. If a tuplet already exists in the range, return 0. 
2. If there are not enough notes/rests in the range, return 0.
3. If any of the notes has logical duration or is a multibar rest,
	return 0. */

short GetTupleNum(LINK startL, LINK endL, short voice, Boolean /*needSelected*/)
{
	short 	durUnit, tupleNum;
	PANOTE	aNote;
	LINK	pL, aNoteL;
	Boolean  firstNote;
	
	tupleNum = 0;
	durUnit = PDURUNIT;											/* Start with shortest possible duration */
	firstNote = TRUE;
	if (SelRangeChkTuplet(voice, startL, endL))  return 0;
	if (VCountNotes(voice, startL, endL, TRUE) < 2)  return 0;	/* Counts Syncs with notes in voice */
	
	/* Get GCD of simple logical duration of candidate notes */
	for (pL = startL; pL!= endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->voice==voice) {
					if (aNote->subType<=UNKNOWN_L_DUR) return 0;
					if (firstNote) {
						durUnit = SimpleLDur(aNoteL);
						firstNote = FALSE;
					}
					else
						durUnit = GCD(durUnit, (short)SimpleLDur(aNoteL));
					break;						/* Assumes all notes in chord have same duration. */
				}
			}
		}
					
	/* Get tupleNum */
	for (pL = startL; pL!= endL; pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->voice==voice) {
					tupleNum += SimpleLDur(aNoteL)/durUnit;
					break;						/* Each chord only contributes once to calc of tupletNum. */
				}
			}
		}

	return tupleNum;
}


/* ------------------------------------------------------------- PrepareSelRange -- */
/* Prepare array for use by CreateTUPLET. If not enough memory, return NULL. */

static PTIME *PrepareSelRange(Document *doc, short *nInRange, LINK *baseMeasL)
{
	short nInMeas = 0, numNotes;
	LINK pL;
	
	*baseMeasL = startMeas = LSSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	endMeas = EndMeasSearch(doc, startMeas);

	for (pL = startMeas; pL!=endMeas; pL = RightLINK(pL))
		if (SyncTYPE(pL)) nInMeas++;

	/* numNotes provides one note per voice for all syncs in selection range. */ 
	numNotes = nInMeas*(MAXVOICES+1);

	/* Allocate pDurArray[numNotes]. First initialize the unused 0'th row of
		the array, then the rest of the rows. Set pTimes of 0'th row to BIGNUM to
		sort them above the world. */

	pDurArray = (PTIME *)NewPtr((Size)numNotes*sizeof(PTIME));
	if (!GoodNewPtr((Ptr)pDurArray)) { NoMoreMemory(); return NULL; }

	InitDurArray(pDurArray,nInMeas);

	*nInRange = nInMeas;
	return pDurArray;
}


/* ------------------------------------------------------------------- ShellSort -- */
/* Sort the pDurArray via Shell's algorithm. See comments on the algorithm in
MIDIRecord.c and PitchUtils.c. */

static void ShellSort(short n, short nvoices)
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


/* ------------------------------------------------------------------ SortPTimes -- */
/* Sort the pTime array pDurArray based on the pTime field. */

static void SortPTimes(short nInMeas, short nvoices)
{
	ShellSort(nInMeas, nvoices);
}


/* --------------------------------------------------------------- GetTupleDenom -- */
/* Return the usual tuplet denominator for the given numerator: if tupleNum is 4,
return 3, else return the next greatest power of 2 less than tupleNum. This is 
standard but not universal practice: e.g., for some composers/editors, "7" means
"7 in the time of 8" instead of "7 in the time of 4". */

static short GetTupleDenom(short tupleNum, TupleParam *tParam)
{
	short i, tupleDenom;

	if (tParam->isFancy) return tParam->accDenom;

	if (tupleNum==4) return 3;

	for (i = 1; i<tupleNum; i *= 2) tupleDenom = i;
	return tupleDenom;
}


/* ------------------------------------------------------------- ComputePlayDurs -- */

static Boolean ComputePlayDurs(Document *doc, SELRANGE selRange[], short tupleNum[],
									short /*nInTuple*/[], short nInMeas, TupleParam *tParam)
{
	short v, notes, vNotes, *noteStart, tupleDenom;
	long tempDur, stfStartTime, tempTime, tempTime2, playDur;
	LINK pL, qL, aNoteL;  PANOTE aNote;
	Boolean nextSyncExists, isFancy;

	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) goto broken;
	
	stfTimeDiff = (long *)NewPtr((Size)(MAXVOICES+1) * sizeof(long));
	if (!GoodNewPtr((Ptr)stfTimeDiff)) goto broken;
	
	/* Initialize noteStart[v], an array indicating the number of Syncs
		selRange[v].startL is after startMeas. */

	noteStart = (short *)NewPtr((Size)(MAXVOICES+1)*sizeof(short));
	if (!GoodNewPtr((Ptr)noteStart)) goto broken;

	isFancy = tParam->isFancy;
	for (v = 0; v<=MAXVOICES; v++)
		noteStart[v] = 0;
	for (v = 1; v<=MAXVOICES; v++)
		if (selRange[v].startL)
			for (pL = startMeas; pL!=selRange[v].startL; pL = RightLINK(pL))
				if (SyncTYPE(pL)) noteStart[v]++;
			
	/* Set playDur values for the pDurArray. */

	SetPlayDurs(doc, pDurArray, nInMeas, startMeas, endMeas);

	/* Set link values for owning beams, ottavas & tuplets. */

	SetLinkOwners(pDurArray, nInMeas, startMeas, endMeas);

	/* Re-compute tupled playDur values for the array. */

	for (v = 1; v<=MAXVOICES; v++)
		if (tupleNum[v]>=2) {
			tupleDenom = GetTupleDenom(tupleNum[v],tParam);
			notes=noteStart[v];
			for (pL=selRange[v].startL; pL!=selRange[v].endL; pL=RightLINK(pL))
				if (SyncTYPE(pL)) {
					for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
						if (MainNoteInVOICE(aNoteL, v)) {
							tempDur = SimpleLDur(aNoteL)*tupleDenom;
							(pDurArray + v*nInMeas+notes)->playDur = tempDur/tupleNum[v];

							playDur = (pDurArray + v*nInMeas+notes)->playDur;
							playDur = (config.legatoPct*playDur)/100L;
							(pDurArray + v*nInMeas+notes)->playDur = playDur;
						}
					notes++;
				}
		}

	/* Set pTime values for the pDurArray. */

	SetPTimes(doc, pDurArray, nInMeas, spTimeInfo, startMeas, endMeas);

	/* Compute tupled pTime values for the pDurArray. Get the start pTime of
		the first note in the tuplet on the voice, get the difference between the
		pTime of the given note and that time, shrink that difference by tupleFactor,
		and add the newly computed difference to the original start time, to get
		the tupled pTime for that note. */

	for (v = 1; v<=MAXVOICES; v++)
		stfTimeDiff[v] = 0L;

	for (v = 1; v<=MAXVOICES; v++) {
		stfStartTime = (pDurArray + v*nInMeas+noteStart[v])->pTime;
		notes=noteStart[v];

		nextSyncExists = FALSE;
		for (qL=selRange[v].endL; qL && qL!=endMeas; qL=RightLINK(qL))
			if (SyncTYPE(qL))
				{ nextSyncExists = TRUE; break; }

		for (pL=selRange[v].startL; pL!=selRange[v].endL; pL=RightLINK(pL))
			if (SyncTYPE(pL)) {
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (MainNoteInVOICE(aNoteL, v) && tupleNum[v]) {
						aNote = GetPANOTE(aNoteL);
						tempTime = (pDurArray + v*nInMeas+notes)->pTime;
						tupleDenom = GetTupleDenom(tupleNum[v],tParam);
						tempTime2 = (tempTime - stfStartTime)*tupleDenom;
						tempTime2 /= tupleNum[v];
						(pDurArray + v*nInMeas+notes)->pTime = aNote->pTime = 
							stfStartTime + tempTime2;

						/* If a next Sync exists, get the next sync with a note in v.
							Use its pTime to compute the stfTimeDiff. */

						if (nextSyncExists) {

							vNotes = notes;
							for (qL = RightLINK(pL); qL!=endMeas; qL=RightLINK(qL)) {
								if (SyncTYPE(qL)) {
									vNotes++;
									if (SyncInVoice(qL, v))
										break;
								}
							}

							tempTime = (pDurArray + v*nInMeas+vNotes)->pTime;
							tempTime2 = (tempTime - stfStartTime)*tupleDenom;
							tempTime2 /= tupleNum[v];
							stfTimeDiff[v] = (tempTime - (stfStartTime + tempTime2));
						}
					}
				notes++;
			}
	}

	/* Sort the pDurArray in order of increasing pTimes. */
	SortPTimes(nInMeas, MAXVOICES+1);

	if (noteStart) DisposePtr((Ptr)noteStart);
	return TRUE;
	
broken:
	if (noteStart) DisposePtr((Ptr)noteStart);
	DisposeArrays();
	NoMoreMemory();
	return FALSE;
}


/* --------------------------------------------------------------- RearrangeNotes -- */
/* This function sets the <playDur> of every note in its range - presumably an entire
measure - to the default for its logical duration. It would be much better if it
left <playDurs> of notes not being tupled or untupled alone (see code before call
to CopyNotes), but it's not obvious to me how it can tell. Probably a flag for this
would have to be added to qDurArray. */

static Boolean RearrangeNotes(Document *doc, SELRANGE /*selRange*/[], short nNotes,
								LINK /*baseMeasL*/)
{
	short i, v, notes, numNotes, subCount=0, arrBound;
	PTIME *pTime, *qTime;
	LINK pL, firstSubObj, newObjL, subL, newSubL, tempNewSubL, newSelStart;
	PMEVENT pObj, pNewObj;
	Boolean objSel;
	LINK headL=NILINK, tailL=NILINK, initL, prevL, insertL, firstL, lastL;

	pTime=pDurArray;
	newSelStart = NILINK;
	
	/* Compact the pDurArray: allocate qDurArray, copy into it only those
		pTimes s.t. pTime->subL!= NILINK, and then fill in the rest of
		the entries with default values. */

	numNotes = nNotes*(MAXVOICES+1);

	qDurArray = (PTIME *)NewPtr((Size)numNotes*sizeof(PTIME));
	if (!GoodNewPtr((Ptr)qDurArray)) { NoMoreMemory(); return FALSE; }

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

	pTime = qDurArray;

#ifdef DEBUG_DURARRAY
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

	headL = NewNode(doc, HEADERtype, 2);			/* Need nparts+1 subobjs */
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
			subCount += qTime->mult;							/* Handle chords */
			qTime++; i++;
		}
		
		/* Create the sync object, and copy the first note's object
			information into it. */
		if (subCount) {
			newObjL = NewNode(doc, SYNCtype, subCount);
			if (newObjL == NILINK)
				goto broken;

			pObj    = (PMEVENT)LinkToPtr(OBJheap, pTime->objL);
			pNewObj = (PMEVENT)LinkToPtr(OBJheap, newObjL);
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
						if (NoteVOICE(subL)==v) {
							newSubL = CopyNotes(doc, subL, newSubL, qTime->playDur, qTime->pTime);
						}

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
					newSubL = CopyNotes(doc, subL, newSubL, qTime->playDur, qTime->pTime);
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
	
	/* Relocate all J_IP objects and all J_D objects which can only be relative to
		SYNCs. */

	RelocateObjs(doc,headL,tailL,startMeas,endMeas,qDurArray);
	
	/* Relocate all J_D objects which can be relative to J_IT objects other than
		SYNCs or J_IP objects. */
	
	RelocateGenlJDObjs(doc, headL, tailL, qDurArray);
	
	for (pL = startMeas; pL!=endMeas; pL=RightLINK(pL))
		if (LinkSEL(pL))
			{ doc->selStartL = pL; break; }

	for (pL = endMeas; pL!=startMeas; pL=LeftLINK(pL))
		if (LinkSEL(pL))
			{ doc->selEndL = RightLINK(pL); break; }

	FixCrossPtrs(doc, startMeas, endMeas, qDurArray, NULL);

	DeleteRange(doc, RightLINK(headL), tailL);
	DeleteNode(doc, headL);
	DeleteNode(doc, tailL);
	return TRUE;
	
broken:
	if (headL && tailL) DeleteRange(doc, RightLINK(headL), tailL);
	
broken1:
	if (headL) DeleteNode(doc, headL);
	if (tailL) DeleteNode(doc, tailL);
	DisposeArrays();
	NoMoreMemory();
	return FALSE;
}


/* Check if there are any gaps in the logical times of notes in the selection
in any voice; if there are, return FALSE, otherwise return TRUE. */

static Boolean CheckContinSel(Document *doc)
{
	LINK link, measL, endMeasL;  short i, lastNode,v;  Boolean first;
	SPACETIMEINFO *spTimeInfo;
	long startTime,nextlTime;

	
	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) return -1L;

	measL = LSSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	endMeasL = LinkRMEAS(measL) ? LinkRMEAS(measL) : LeftLINK(doc->tailL);

	lastNode = GetSpTimeInfo(doc, RightLINK(measL), endMeasL, spTimeInfo, FALSE);

	/* For each voice in use <v>, traverse spTimeInfo's list of syncs. After
		the first sync selected in v, each sync following <link> in v should be
		at startTime equal to link's startTime plus its duration in the voice.
		If it's at a greater time, there is a hole in the voice, so return FALSE.
		Note the confusing way we have to set the startL and endL params for
		GetSpTimeInfo. */

	for (v = 1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {

			if (!VoiceInSelRange(doc, v)) continue;

			first = TRUE;
			startTime = nextlTime = 0L;
			for (i=0; i<=lastNode; i++) {
				link = spTimeInfo[i].link;
				if (SyncTYPE(link))
					if (NoteInVoice(link,v,TRUE)) {
						startTime = spTimeInfo[i].startTime;
						if (first) first = FALSE;
						else
							if (startTime > nextlTime) {
								DisposePtr((Ptr)spTimeInfo);
								return FALSE;
							}

						nextlTime = startTime+GetVLDur(doc, link, v);
					}
			}
		}

	DisposePtr((Ptr)spTimeInfo);
	return TRUE;
}


/* --------------------------------------------------------------------- DoTuple -- */
/* Tuple the selection. */

void DoTuple(Document *doc, TupleParam *tParam)
{
	short		v, nInMeas,
				nInTuple[MAXVOICES+1],	/* number of tuplable notes in selection */
				tupleNum[MAXVOICES+1],	/* accessory numeral for tuplet */
				maxTupleNum;
	SELRANGE	selRange[MAXVOICES+1];
	LINK		tupletL, vStartL, vEndL, baseMeasL;
	char		fmtStr[256];
	
	PrepareUndo(doc, doc->selStartL, U_Tuple, 38);    			/* "Undo Tuple" */

	if (!CheckContinSel(doc)) {
		if (!CompactVoices(doc))
			return;
	}

	for (v = 1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {
			GetNoteSelRange(doc, v, &vStartL, &vEndL, NOTES_ONLY);
			if (!vStartL || !vEndL) continue;
			
			if (!CheckMaxTupleNum(v, vStartL, vEndL, tParam)) {
				GetIndCString(fmtStr, TUPLETERRS_STRS, 1);    	/* "Tuple numerator or denominator for voice %d exceeds maximum of 255." */
				sprintf(strBuf, fmtStr, v); 
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				DisableUndo(doc,TRUE);
				return;
			}
		}
	maxTupleNum = -9999;
	for (v = 1; v<=MAXVOICES; v++) {
		GetNoteSelRange(doc, v, &vStartL, &vEndL, NOTES_ONLY);
		selRange[v].startL = vStartL;
		selRange[v].endL = vEndL;

		if (!vStartL || !vEndL)
			tupleNum[v] = nInTuple[v] = 0;
		else {
			tupleNum[v] = GetTupleNum(vStartL, vEndL, v, TRUE);
			nInTuple[v] = VCountNotes(v, vStartL, vEndL, TRUE);
		}
		maxTupleNum = n_max(maxTupleNum, tupleNum[v]);
	}
	if (maxTupleNum < 2) {
		DisableUndo(doc, FALSE);
		return;
	}

	SetSpareFlags(doc->headL, doc->tailL, FALSE);
	if (!(pDurArray = PrepareSelRange(doc, &nInMeas, &baseMeasL))) {
		DisableUndo(doc, TRUE);
		return;
	}
	if (!ComputePlayDurs(doc, selRange, tupleNum, nInTuple, nInMeas, tParam))
		goto broken;
	if (!RearrangeNotes(doc, selRange, nInMeas, baseMeasL)) goto broken;

	for (v = 1; v<=MAXVOICES; v++) {
		if (tupleNum[v]>=2) {
			GetNoteSelRange(doc, v, &vStartL, &vEndL, NOTES_ONLY);
			tupletL = CreateTUPLET(doc, vStartL, vEndL, v, nInTuple[v], tupleNum[v],
									TRUE, TRUE, tParam);
			if (doc->selStartL==FirstInTuplet(tupletL))
				doc->selStartL = tupletL;
		}
	}
	
	DisposeArrays();

	if (doc->autoRespace)
		RespaceBars(doc, doc->selStartL, doc->selEndL, 0L, FALSE, FALSE);
	else
		InvalMeasures(doc->selStartL, doc->selEndL, ANYONE);						/* Force redrawing all affected measures */
		
	/* Need these calls to fix up links for Beams and Ottavas which extend
		outside the tupleted measure. */

	FixAllBeamLinks(doc, doc, doc->headL, doc->tailL);
	FixOttavaLinks(doc, doc, doc->headL, doc->tailL);

broken:
	DisableUndo(doc,TRUE);
	DisposeArrays();
}


/* ---------------------------------------------------- Utilities for CreateTUPLET -- */

/* --------------------------------------------------------------- GetBracketVis -- */
/* Should a tuplet bracket be visible? Return FALSE (bracket not visible) if all the
notes in <voice> from startL to endL are in one and the same beamset, else return
TRUE. This is the standard conventional-notation convention. Assumes a valid range
for tupling, e.g., more than one note in the voice in the range. */

Boolean GetBracketVis(Document *doc, LINK startL, LINK endL, short voice)
{
	LINK startSync, endSync, startBeamL, endBeamL, aNoteL;
	PANOTE aNote;
	Boolean beamed = FALSE;		/* To break out of the inner loop. */
	
	if (!startL || !endL) return TRUE;
	
	for (startSync = startL; startSync!=endL && !beamed; 
		startSync = RightLINK(startSync))
		if (SyncTYPE(startSync)) {
			aNoteL = FirstSubLINK(startSync);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteVOICE(aNoteL)==voice) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->beamed) 
						{ beamed = TRUE; break; }
					else return TRUE;
				}
		}

	for (beamed = FALSE, endSync = LeftLINK(endL); endSync!=startL && !beamed; 
		endSync = LeftLINK(endSync))
		if (SyncTYPE(endSync)) {
			aNoteL = FirstSubLINK(endSync);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteVOICE(aNoteL)==voice) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->beamed) 
						{ beamed = TRUE; break; }
					else return TRUE;
				}
		}

	startBeamL = LVSearch(startSync, BEAMSETtype, voice, TRUE, FALSE);

	/* If there are other notes in the beamset which are not to be
		tupled, make bracket visible. */

	if (IsAfter(doc->selEndL, LastInBeam(startBeamL)) || 
		 doc->selEndL==LastInBeam(startBeamL) ||
		 IsAfter(FirstInBeam(startBeamL), doc->selStartL)) return TRUE;

	/* Make sure the range to be tupled contains only one and the same
		beamset. */

	endBeamL = LVSearch(endSync, BEAMSETtype, voice, TRUE, FALSE);

	return (startBeamL!=endBeamL);
}


/* ------------------------------------------------------------------- InitTuplet -- */

void InitTuplet(
			LINK tupletL,
			short staff, short voice,
			short tupleNum, short tupleDenom,
			Boolean numVis, Boolean	denomVis,
			Boolean brackVis
			)
{
	PTUPLET pTuplet;

	pTuplet = GetPTUPLET(tupletL);
	pTuplet->tweaked = FALSE;
	pTuplet->voice = voice;
	pTuplet->staffn = staff;
	pTuplet->accNum = tupleNum;
	pTuplet->accDenom = tupleDenom;
	pTuplet->numVis = numVis;
	pTuplet->denomVis = denomVis;
	pTuplet->brackVis = brackVis;

	pTuplet->small = 0;
	pTuplet->filler = 0;
	
	pTuplet->acnxd = pTuplet->acnyd = 0;
	pTuplet->xdFirst = pTuplet->xdLast = 0;
	pTuplet->ydFirst = pTuplet->ydLast = 0;	/* Caller should almost always reset these */
}


/* ---------------------------------------------------------------- SetTupletYPos -- */
/* Set the given tuplet to (a guess at) a reasonable vertical position, based on the
position of the MainNote of its first note/chord. (It'd surely be better to look at
all of its notes/chords, though it's not entirely obvious what to do with that
information.) */ 

void SetTupletYPos(Document *doc, LINK tupletL)
{
	LINK aNoteTupleL, mainNoteL;  PANOTETUPLE aNoteTuple;
	Boolean bracketBelow;  DDIST brackyd, firstyd;
	CONTEXT context;
	PTUPLET pTuplet;

	aNoteTupleL = FirstSubLINK(tupletL);
	aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
	mainNoteL = FindMainNote(aNoteTuple->tpSync, TupletVOICE(tupletL));
	firstyd = NoteYD(mainNoteL);

	GetContext(doc, tupletL, TupletSTAFF(tupletL), &context);
	bracketBelow = (firstyd<=context.staffHeight/2);		/* Below if head above staff ctr line */
	if (bracketBelow)	brackyd = context.staffHeight +
									std2d(STD_TUPLET_MARGIN_BELOW, context.staffHeight,
									context.staffLines);
	else				brackyd = std2d(STD_TUPLET_MARGIN_ABOVE, context.staffHeight,
									context.staffLines);
	pTuplet = GetPTUPLET(tupletL);
	pTuplet->ydFirst = pTuplet->ydLast = brackyd;
}


/* ----------------------------------------------------------------- CreateTUPLET -- */
/* Create tuplet from startL to endL, excluding endL, in voice <v>. */

LINK CreateTUPLET(
			Document *doc,
			LINK startL, LINK endL,
			short v,
			short nInTuple,
			short tupletNumeral,
			Boolean needSelected,	/* TRUE if we only want selected items */
			Boolean doTuple,		/* TRUE if we're explicitly tupling notes, so tuplet will be selected */
			TupleParam *tParam
			)
{
	LINK		pL, tupletL, aNoteTupleL;
	PANOTETUPLE aNoteTuple;
	short 		tuplElem, iTuplElem, staff,
				tupleNum, tupleDenom;					/* Printable "accessory" denominator */
	Boolean		numVis, denomVis, brackVis;
	LINK		aNoteL;
	LINK		tpSync[MAXINTUPLET];
	SearchParam pbSearch;
	
	if (nInTuple<2) {
		MayErrMsg("CreateTUPLET: can't make tuplet with only %ld notes.", (long)nInTuple);
		return NILINK;
	}
	
	if (tParam->isFancy) {
		tupleNum = tParam->accNum;
		tupleDenom = tParam->accDenom;
		numVis = tParam->numVis;
		denomVis = tParam->denomVis;
		brackVis = tParam->brackVis;
	}
	else {
		tupleNum = tupletNumeral;
		tupleDenom = GetTupleDenom(tupleNum,tParam);
		numVis = TRUE;
		denomVis = FALSE;
		brackVis = GetBracketVis(doc, startL, endL, v);
	}

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = v;
	pbSearch.needSelected = needSelected;
	pbSearch.inSystem = FALSE;
	pbSearch.subtype = ANYSUBTYPE;
	startL = L_Search(startL, SYNCtype, FALSE, &pbSearch);	/* Start at 1st Sync in range */

	/* ??If the tuplet is cross-staff, the following sets its staff no. to the staff of
		its first note. Unfortunately, code in CrossStaff2CrossPart assumes the staff no. is of the
		upper staff, which is the way cross-staff Beams and Slurs are handled. Change this?
	 */
	staff = NoteSTAFF(pbSearch.pEntry);								/* Pick up tuplet's staff no. */

	tupletL = InsertNode(doc, startL, TUPLETtype, nInTuple);
	if (!tupletL) { NoMoreMemory(); return NILINK; }

	SetObject(tupletL, 0, 0, FALSE, TRUE, FALSE);
	InitTuplet(tupletL, staff, v, tupleNum, tupleDenom, numVis, denomVis, brackVis);
	LinkSEL(tupletL) = doTuple;

	doc->changed = TRUE;											/* File has unsaved change(s) */

	for (tuplElem = 0, pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL) && (!needSelected || LinkSEL(pL)))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteVOICE(aNoteL)==v) {							/* Rests are acceptable */
					NoteINTUPLET(aNoteL) = TRUE;
					if (MainNote(aNoteL)) {
						tpSync[tuplElem] = pL;
						tuplElem++;
						InvalMeasure(pL, staff);
					}
				}

	if (tuplElem!=nInTuple) {
		MayErrMsg("CreateTUPLET: expected %ld tuplable notes but found %ld.",
					(long)nInTuple, (long)tuplElem);
		return NILINK;
	}
	
	FixTimeStamps(doc, startL, endL);

	aNoteTupleL = FirstSubLINK(tupletL);
	for (iTuplElem=0; iTuplElem<tuplElem; 
			iTuplElem++, aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
		aNoteTuple = GetPANOTETUPLE(aNoteTupleL);			/* Ptr to note info in TUPLET */
		aNoteTuple->tpSync = tpSync[iTuplElem];
	}	

	SetTupletYPos(doc, tupletL);
			
	return tupletL;
}


/* --------------------------------------------------------------------------------- */
/* Utilities for DoUntuple. */

static void SelectSync(LINK pL, short v, Boolean sel)
{
	LINK aNoteL;
	
	LinkSEL(pL) = sel;
	for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		NoteSEL(aNoteL) = FALSE;
		if (MainNoteInVOICE(aNoteL, v))
			NoteSEL(aNoteL) = sel;
	}
}


static void SelectTuplet(Document *doc, short v, Boolean sel)
{
	LINK pL;
	
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (SyncTYPE(pL))
			SelectSync(pL,v,sel);
}


static void DeleteTuplet(Document *doc, LINK tupletL)
{
	short v;  LINK pL, firstL, lastL, aNoteL;
	
	v = TupletVOICE(tupletL);
	firstL = FirstInTuplet(tupletL);
	lastL = LastInTuplet(tupletL);

	for (pL=firstL; pL!=RightLINK(lastL); pL=RightLINK(pL))
		if (SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteVOICE(aNoteL)==v)
					NoteINTUPLET(aNoteL) = FALSE;
					
	DeleteNode(doc, tupletL);
}


/* ------------------------------------------------------------ RecomputePlayDurs -- */

static Boolean RecomputePlayDurs(
					Document *doc,
					SELRANGE selRange[],
					short tupleNum[],
					short /*nInTuple*/[],
					short nInMeas,
					short tupleDenom
					)
{
	short v, notes, vNotes, *noteStart, k;
	long tempDur, stfStartTime, tempTime, tempTime2, vDur;
	LINK pL, qL, aNoteL;  PANOTE aNote;
	Boolean nextSyncExists;

	spTimeInfo = AllocSpTimeInfo();
	if (!spTimeInfo) goto broken;
	
	stfTimeDiff = (long *)NewPtr((Size)(MAXVOICES+1) * sizeof(long));
	if (!GoodNewPtr((Ptr)stfTimeDiff)) goto broken;
	
	/* Initialize noteStart[v], an array indicating the number of syncs
		selRange[v].startL is after startMeas. */

	noteStart = (short *)NewPtr((Size)(MAXVOICES+1)*sizeof(short));
	if (!GoodNewPtr((Ptr)noteStart)) goto broken;

	for (v = 0; v<=MAXVOICES; v++)
		noteStart[v] = 0;
	for (v = 1; v<=MAXVOICES; v++)
		if (selRange[v].startL)
			for (pL = startMeas; pL!=selRange[v].startL; pL = RightLINK(pL))
				if (SyncTYPE(pL)) noteStart[v]++;
			
	/* Set playDur values for the pDurArray. */

	SetPlayDurs(doc,pDurArray,nInMeas,startMeas,endMeas);

	/* Set link values for owning beams, ottavas, tuplets and slurs. */
	
	SetLinkOwners(pDurArray,nInMeas,startMeas,endMeas);

	/* Compute untupled playDur values for the array. */

	for (v = 1; v<=MAXVOICES; v++)
		if (tupleNum[v]>=2) {
			notes=noteStart[v];
			for (pL=selRange[v].startL; pL!=selRange[v].endL; pL=RightLINK(pL))
				if (SyncTYPE(pL)) {
					for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
						if (MainNoteInVOICE(aNoteL, v)) {
							tempDur = SimpleLDur(aNoteL);
							(pDurArray + v*nInMeas+notes)->playDur = tempDur;
							aNote = GetPANOTE(aNoteL);
							aNote->playDur = tempDur;
						}
					notes++;
				}
		}

	/* Set pTime values for the pDurArray. */

	SetPTimes(doc, pDurArray, nInMeas, spTimeInfo, startMeas, endMeas);

	/* Compute tupled pTime values for the pDurArray. Get the start pTime of
		the first note in the tuplet on the voice, get the difference between the
		pTime of the given note and that time, shrink that difference by tupleFactor,
		and add the newly computed difference to the original start time, to get
		the tupled pTime for that note. */

	for (v = 1; v<=MAXVOICES; v++)
		stfTimeDiff[v] = 0L;

	for (v = 1; v<=MAXVOICES; v++) {
		stfStartTime = (pDurArray + v*nInMeas+noteStart[v])->pTime;
		notes=noteStart[v];

		nextSyncExists = FALSE;
		for (qL=selRange[v].endL; qL && qL!=endMeas; qL=RightLINK(qL))
			if (SyncTYPE(qL))
				{ nextSyncExists = TRUE; break; }

		for (pL=selRange[v].startL; pL!=selRange[v].endL; pL=RightLINK(pL))
			if (SyncTYPE(pL)) {
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (MainNoteInVOICE(aNoteL, v) && tupleNum[v]) {
						aNote = GetPANOTE(aNoteL);
						tempTime = (pDurArray + v*nInMeas+notes)->pTime;

						for (vDur=0,k=noteStart[v]; k<notes; k++)
							vDur += (pDurArray + v*nInMeas+k)->playDur;

						(pDurArray + v*nInMeas+notes)->pTime = aNote->pTime = 
							stfStartTime + vDur;

						/* If a next sync exists, get the next sync with a note in v.
							Use its pTime to compute the stfTimeDiff. */

						if (nextSyncExists) {

							vNotes = notes;
							for (qL = RightLINK(pL); qL!=endMeas; qL=RightLINK(qL)) {
								if (SyncTYPE(qL)) {
									vNotes++;
									if (SyncInVoice(qL, v))
										break;
								}
							}

							tempTime = (pDurArray + v*nInMeas+vNotes)->pTime;
							tempTime2 = (tempTime - stfStartTime)*tupleNum[v];
							tempTime2 /= tupleDenom;
							stfTimeDiff[v] = (tempTime - (stfStartTime + tempTime2));
						}
					}
				notes++;
			}

		for (pL=selRange[v].endL; pL && pL!=endMeas; pL=RightLINK(pL))
			if (SyncTYPE(pL)) {
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (MainNoteInVOICE(aNoteL, v)) {
						aNote = GetPANOTE(aNoteL);
						aNote->pTime -= stfTimeDiff[v];
						(pDurArray + v*nInMeas+notes)->pTime -= stfTimeDiff[v];
					}
				notes++;
			}
	}

	/* Sort the pDurArray in order of increasing pTimes. */
	SortPTimes(nInMeas, MAXVOICES+1);

	if (noteStart) DisposePtr((Ptr)noteStart);
	return TRUE;
	
broken:
	if (noteStart) DisposePtr((Ptr)noteStart);
	DisposeArrays();
	NoMoreMemory();
	return FALSE;
}


/* Remove the given Tuplet. If out of memory, return NILINK. */

LINK RemoveTuplet(Document *doc, LINK tupletL)
{
	LINK nextL,baseMeasL; PTUPLET pTuplet;
	SELRANGE selRange[MAXVOICES+1];
	short v,tupletV,nInMeas,nInTuple[MAXVOICES+1],tupleNum[MAXVOICES+1],tupleDenom;

	/* Initialize selRange[],nInTuple[], and tupleNum[] for deletion of
		tupletL; all slots for voices other than tupletL's are NILINK or 0;
		and select tupletL's owned syncs. */

	nextL = RightLINK(tupletL);
	for (v = 1; v<=MAXVOICES; v++) {
		selRange[v].startL = selRange[v].endL = NILINK;
		nInTuple[v] = tupleNum[v] = 0;
	}

	tupletV = TupletVOICE(tupletL);
	selRange[tupletV].startL = FirstInTuplet(tupletL);
	selRange[tupletV].endL = RightLINK(LastInTuplet(tupletL));

	pTuplet = GetPTUPLET(tupletL);
	tupleNum[tupletV] = pTuplet->accNum;
	tupleDenom = pTuplet->accDenom;
	nInTuple[tupletV] = LinkNENTRIES(tupletL);

	doc->selStartL = selRange[tupletV].startL;
	doc->selEndL = selRange[tupletV].endL;
	SelectTuplet(doc,tupletV,TRUE);

	if (!(pDurArray = PrepareSelRange(doc, &nInMeas, &baseMeasL)))
		{ DisableUndo(doc,TRUE); return NILINK; }

	if (!RecomputePlayDurs(doc, selRange, tupleNum, nInTuple, nInMeas, tupleDenom))
		goto broken;

	if (!RearrangeNotes(doc, selRange, nInMeas, baseMeasL)) goto broken;
	DisposeArrays();
	
	SelectTuplet(doc,tupletV,FALSE);
	DeleteTuplet(doc,tupletL);
	return nextL;
	
broken:
	SelectTuplet(doc,tupletV,FALSE);
	DisposeArrays();
	return nextL;
}


/* -------------------------------------------------------- UnTuple, MarkTuplets -- */
/* Remove all marked tuples in the document. */
	
void UnTuple(Document *doc)
{
	LINK nextL,pL;

	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL)) DeselectNode(pL);

	for (pL = doc->headL; pL!=doc->tailL; pL=nextL) {
		nextL = RightLINK(pL);
		if (TupletTYPE(pL) && LinkSPAREFLAG(pL)) {
			nextL = RemoveTuplet(doc,pL);
			if (!nextL) return;
		}
	}
}


static void MarkTuplets(Document *doc)
{
	LINK startL, endL, pL, qL, vStartL, vEndL, tupletL, aNoteL;
	short v;

	/* Clear spareFlags for all tuplets. */

	SetSpareFlags(doc->headL, doc->tailL, FALSE);

	GetOptSelEnds(doc, &startL, &endL);

	/* For each voice, traverse note selRange; get tuplet for each intuplet
		note in the range in the voice; set that tuplet's spareFlag, and move
		pL along to node after tuplet's lastInTuplet or along to end of range,
		whichever is earlier.
		#1. pL is incremented to RightLINK(pL) after this statement and before
		testing pL!=vEndL. If we assigned pL to <RightLINK(qL) : vEndL> here, 
		would increment and be beyond vEndL before testing. */

	for (v = 1; v<=MAXVOICES; v++) {
		if (VOICE_MAYBE_USED(doc,v)) {
			GetNoteSelRange(doc, v, &vStartL, &vEndL, NOTES_ONLY);
			if (!vStartL || !vEndL) continue;
	
			for (pL = vStartL; pL!=vEndL; pL=RightLINK(pL))
				if (LinkSEL(pL) && SyncTYPE(pL))
					for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
						if (NoteVOICE(aNoteL)==v)
							if (NoteINTUPLET(aNoteL)) {
								tupletL = LVSearch(pL, TUPLETtype, v, GO_LEFT, FALSE);
								if (tupletL) {
									LinkSPAREFLAG(tupletL) = TRUE;
									qL = LastInTuplet(tupletL);				/* #1 */
									pL = ((IsAfter(RightLINK(qL),vEndL)) ? qL : LeftLINK(vEndL));
								}
								break;
							}
		}
	}
}


/* ------------------------------------------------------------------- DoUntuple -- */
/* Untuple the selection.

#1. Data structure is totally re-constructed, with the result that no LINK
values are guaranteed to be preserved; in particular, the previous selStart/
selEnd LINKs may or may not be in the data structure, and the selection
status of all LINKs is not preserved by untupling algorithm. Therefore
the two tests used to get selStart/selEnd from previous selRange fail:
testing if selStart/selEnd are in dataStructure, and using selection status
of new data structure to reset them.
Cannot use spareFlag (or other object-level fields) either, since some syncs
disappear when their notes are combined with notes of other syncs by
RearrangeNotes.
Therefore, get measure objects bounding previous selRange, and use these to
do re-spacing and deselection of range, and don't try to preserve previous
selection range. */

void DoUntuple(Document *doc)
{
	LINK selStart,selEnd;
	
	PrepareUndo(doc, doc->selStartL, U_Untuple, 39);    			/* "Undo Untuple" */

	/* #1. */
	selStart = LSSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	selEnd = LSSearch(doc->selEndL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	selEnd = EndMeasSearch(doc, selEnd);

	MarkTuplets(doc);
	UnTuple(doc);

	SetSpareFlags(doc->headL, doc->tailL, FALSE);
	doc->selStartL = selStart;
	doc->selEndL = selEnd;

	FixTimeStamps(doc, doc->headL, doc->tailL);

	if (doc->autoRespace)
		RespaceBars(doc, doc->selStartL, doc->selEndL, 0L, FALSE, FALSE);
	else
		InvalMeasures(doc->selStartL, doc->selEndL, ANYONE);
		
	DeselRange(doc, doc->selStartL, doc->selEndL);
	doc->selStartL = doc->selEndL;
	MEAdjustCaret(doc,TRUE);
}


/* ------------------------------------------------------------------ UntupleObj -- */
/* Set tuplet flag FALSE for all notes/rests on <staff> between <firstSync> and 
<lastSync> inclusive, restore their pDur's to non-tupled values, and delete
the tuplet <tupleL>. */
 
void UntupleObj(Document *doc, LINK tupleL, LINK firstSync, LINK lastSync, short voice)
{
	LINK	pL, aNoteL;
	PANOTE	aNote;
	long	tempDur;
	short 	tupleNum, tupleDenom;
	PTUPLET tupletp;
	
	tupletp = GetPTUPLET(tupleL);
	tupleNum = tupletp->accNum;
	tupleDenom = tupletp->accDenom;
	for (pL = firstSync; pL!=RightLINK(lastSync); pL=RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->voice==voice) {
					aNote->inTuplet = FALSE;
					tempDur = (long)aNote->playDur*tupleNum;
					aNote->playDur =tempDur/tupleDenom;
				}
			}
		}
	}
	DeleteNode(doc, tupleL);
}


/* ---------------------------------------------------------------------------------- */
/* Utilities for DrawTUPLET. */

/* ------------------------------------------------------------ DrawTupletBracket -- */
/* Draw a tuplet bracket with QuickDraw calls. If <pContext> is NULL, the page origin
is assumed to be at (0,0); this is to facilitate calling this routine to draw a sample
tuplet bracket in a dialog. */

#define NUMMARGIN	3						/* Horiz. margin in pixels. 2 isn't enough */

void DrawTupletBracket(
			DPoint firstPt, DPoint lastPt,/* Vertical origin of bracket */
			DDIST brackDelta, 				/* Distance to move bracket up from origin  */
			DDIST yCutoffLen,
			DDIST xMid,
			short tuplWidth,				/* Width of tuplet string (pixels), or <0 if none */
			Boolean firstUp,				/* Point left end of bracket upwards? */
			Boolean lastUp,					/* Point right end of bracket upwards? */
			PCONTEXT pContext,				/* Or NULL, e.g., to use in a dialog; see below */
			Boolean dim
			)
{
	short	firstx, firsty, lastx, lasty, y, ydelta, numxFirst, numxLast,
			yCutoff, paperLeft, paperTop;
	
	paperLeft = (pContext? pContext->paper.left : 0);
	paperTop = (pContext? pContext->paper.top : 0);
	
	ydelta = d2p(brackDelta);
	firstx = paperLeft+d2p(firstPt.h);
	firsty = paperTop+d2p(firstPt.v)-ydelta;
	lastx = paperLeft+d2p(lastPt.h);
	lasty = paperTop+d2p(lastPt.v)-ydelta;
	numxFirst = paperLeft+d2p(xMid)-tuplWidth/2-NUMMARGIN;
	numxLast = paperLeft+d2p(xMid)+tuplWidth/2+NUMMARGIN;
	
	/* If bracket so short there's no room for the numbers and a margin, don't draw */
	
	if (numxFirst-firstx<=0) return;

	if (dim) PenPat(NGetQDGlobalsGray());
	yCutoff = d2p(yCutoffLen);
	MoveTo(firstx,firsty+(firstUp? -yCutoff : yCutoff));
	LineTo(firstx,firsty);
	if (tuplWidth>=0) {
		y = InterpY(firstx, firsty, lastx, lasty, numxFirst);
		LineTo(numxFirst, y);
		y = InterpY(firstx, firsty, lastx, lasty, numxLast);
		MoveTo(numxLast, y);
	}
	LineTo(lastx, lasty);
	LineTo(lastx, lasty+(lastUp? -yCutoff : yCutoff));
	PenNormal();
}


/* ---------------------------------------------------------- DrawPSTupletBracket -- */
/* Draw a tuplet bracket via PostScript. */

void DrawPSTupletBracket(
			DPoint firstPt, DPoint lastPt,
			DDIST brackDelta, 				/* Distance from notehead to top (or bottom) of bracket */
			DDIST yCutoffLen, 				/* Distance from notehead to start of bracket */
			DDIST xMid,
			short tuplWidth,				/* Width of tuplet string (points), or <0 if none */
			Boolean firstUp,				/* Point left end of bracket upwards? */
			Boolean lastUp,					/* Point right end of bracket upwards? */
			PCONTEXT pContext
			)
{
	DDIST	numxdFirst, numxdLast, lnSpace, firstyd, lastyd, yd;
	
	lnSpace = LNSPACE(pContext);
	numxdFirst = xMid-pt2d(tuplWidth/2+NUMMARGIN);
	numxdLast = xMid+pt2d(tuplWidth/2+NUMMARGIN);
	
	/* If bracket so short there's no room for the numbers and a margin, don't draw */
	if (numxdFirst-firstPt.h<=0) return;

	firstyd = firstPt.v-brackDelta;
	lastyd = lastPt.v-brackDelta;

	PS_Line(firstPt.h, firstyd+(firstUp? -yCutoffLen : yCutoffLen),
			  firstPt.h, firstyd, TUPLE_BRACKTHICK(lnSpace));
			  
	if (tuplWidth>=0) {
		yd = (DDIST)InterpY((short)firstPt.h, (short)firstyd,
									(short)lastPt.h, (short)lastyd, numxdFirst);
		PS_Line(firstPt.h, firstyd, numxdFirst, yd, TUPLE_BRACKTHICK(lnSpace));
		yd = (DDIST)InterpY((short)firstPt.h, (short)firstyd,
									(short)lastPt.h, (short)lastyd, numxdLast);
		PS_Line(numxdLast, yd, lastPt.h, lastyd, TUPLE_BRACKTHICK(lnSpace));
	}
	else
		PS_Line(firstPt.h, firstyd, lastPt.h, lastyd, TUPLE_BRACKTHICK(lnSpace));
				  
	PS_Line(lastPt.h, lastyd, lastPt.h, lastyd+(lastUp? -yCutoffLen : yCutoffLen),
			  TUPLE_BRACKTHICK(lnSpace));
}


/* --------------------------------------------------------------- GetTupleNotes -- */
/* Fill in an array of MainNotes for the elements of a Tuplet. */

static void GetTupleNotes(LINK, LINK[]);
static void GetTupleNotes(LINK tupL, LINK notes[])
{
	LINK aNoteTupleL, syncL;  PTUPLET tup;  PANOTETUPLE aNoteTuple;
	short i, voice;

	tup = GetPTUPLET(tupL);
	voice = TupletVOICE(tupL);
	aNoteTupleL = FirstSubLINK(tupL);
	for (i = 0; aNoteTupleL; i++, aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
		aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
		syncL = aNoteTuple->tpSync;
		notes[i] = FindMainNote(syncL, voice);
	}
}


/* ------------------------------------------------------------- IsTupletCrossStf -- */

Boolean IsTupletCrossStf(LINK tupL)
{
	LINK tNotes[MAXINTUPLET];
	STFRANGE stfRange;

	GetTupleNotes(tupL, tNotes);
		
	return GetCrossStaff(LinkNENTRIES(tupL), tNotes, &stfRange);
}	


/* ------------------------------------------------------------------ NoteXStfYD -- */
/* Return note's yd relative to firstStfTop. */

static DDIST NoteXStfYD(LINK, STFRANGE, DDIST, DDIST, Boolean);
static DDIST NoteXStfYD(LINK aNoteL, STFRANGE stfRange, DDIST firstStfTop, 
								DDIST lastStfTop, Boolean crossStaff)
{
	DDIST yd; PANOTE aNote;

	aNote = GetPANOTE(aNoteL);
	if (crossStaff) {
		if (NoteSTAFF(aNoteL)==stfRange.topStaff)
				yd = aNote->yd;
		else	yd = aNote->yd+(lastStfTop-firstStfTop);
	}
	else yd = aNote->yd;
	
	return yd;
}


/* --------------------------------------------------------------- GetTupletInfo -- */
/* Return information about the given tuplet needed to draw it, plus related info. */

void GetTupletInfo(
			Document *doc,
			LINK tupL,
			PCONTEXT pContext,
			DDIST *pxd, DDIST *pyd,				/* Relative to the TOP_LEFT of tupL's Measure */
			short *pxColon,						/* Relative to the TOP_LEFT of tupL's Measure */
			DPoint *pFirstPt, DPoint *pLastPt,
			unsigned char tupleStr[],
			DDIST *pDTuplWidth,
			Rect *pTupleRect
			)
{
	PTUPLET		tup;
	short		staff;
	DDIST		firstxd, lastxd, firstyd, lastyd;
	DDIST		lastNoteWidth, dTuplWidth, measDiff;
	LINK		firstSyncL, lastSyncL, firstMeasL, lastMeasL;
	Boolean		sameMeas;
	Rect		tempR, tupleRect, denomRect;
	unsigned char denomStr[20];
	short		nchars, tupleLen, strWidth;
	GrafPtr		port;

	GetPort(&port);		
	short face = GetPortTextFace(port);
	
	tup = GetPTUPLET(tupL);
	staff = tup->staffn;
	firstSyncL = FirstInTuplet(tupL);
	lastSyncL = LastInTuplet(tupL);

	/* If firstSyncL and lastSyncL are in different measures (impossible as of v. 5.6,
		but be prepared!), add the difference of the measures' xds to lastxd. */
		
	firstMeasL = LSSearch(firstSyncL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	lastMeasL = LSSearch(lastSyncL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	sameMeas = (firstMeasL==lastMeasL);
	firstxd = LinkXD(firstSyncL)+tup->xdFirst;
	firstyd = tup->ydFirst;
	
	lastNoteWidth = std2d(SymWidthRight(doc, lastSyncL, staff, FALSE),
									pContext->staffHeight, pContext->staffLines);
	measDiff = sameMeas ? 0 : LinkXD(lastMeasL)-LinkXD(firstMeasL);			
	lastxd = measDiff+LinkXD(lastSyncL)+lastNoteWidth+tup->xdLast;
	lastyd = tup->ydLast;
	
	SetDPt(pFirstPt, firstxd, firstyd);
	SetDPt(pLastPt, lastxd, lastyd);

	/* The tuplet object has fields that allow positioning the bracket and accessory
		numeral independently. But for the time being, we ignore the accessory numeral
		position and just center it within the bracket. */

	*pxd = (firstxd+lastxd)/2;
	*pyd = (firstyd+lastyd)/2;
	
	/* Set up variables for the numerator only. Even if the tuplet is invisible,
		we'll need tupleRect and dTuplWidth, computed for the numerator only, to
		get the objRect. */
	
	NumToSonataStr((long)tup->accNum, tupleStr);
	tupleRect = StrToObjRect(tupleStr);
	strWidth = NPtStringWidth(doc, tupleStr, doc->musicFontNum,
									d2pt(pContext->staffHeight), face);
	dTuplWidth = pt2d(strWidth);

	if (tup->denomVis || doc->showInvis) {

		/* We're drawing the ':' delimiter and denominator, too. Since Sonata has
			no ':', leave space and we'll fake it. */
			
		tupleLen = tupleStr[0]+1;
		/* This won't work in all fonts.  E.g., in Petrucci, this is too wide.
			However, I think PostScript output is ok. */
		tupleStr[tupleLen] = ' ';
		tempR = CharRect(MapMusChar(doc->musFontInfoIndex, MCH_dot));	/* About the same size as ':' */
		*pxColon = tupleRect.right;

		/* First move tempR so its h origin is zero, then offset it to the right
			of tupleRect. */
			
		OffsetRect(&tempR, -tempR.left, 0);
		OffsetRect(&tempR, tupleRect.right, 0);
		UnionRect(&tupleRect, &tempR, &tupleRect);

		/* Append the denominator string. */
		
		NumToSonataStr((long)tup->accDenom, denomStr);
		denomRect = StrToObjRect(denomStr);
		nchars = denomStr[0];
		tupleStr[0] = nchars+tupleLen;
		while (nchars>0) {
			tupleStr[nchars+tupleLen] = denomStr[nchars];
			nchars--;
		}
		OffsetRect(&denomRect, -denomRect.left, 0);
		OffsetRect(&denomRect, tupleRect.right, 0);
		UnionRect(&tupleRect, &denomRect, &tupleRect);
		tupleRect.right += tupleStr[0]-1;					/* Empirical correction (approximate!) */
		strWidth = NPtStringWidth(doc, tupleStr, doc->musicFontNum,
									d2pt(pContext->staffHeight), face);
		dTuplWidth = pt2d(strWidth);
	}

	*pDTuplWidth = dTuplWidth;
	*pTupleRect = tupleRect;
}


/* ------------------------------------------------------------------- DrawTUPLET -- */
/* Draw a TUPLET object, i.e., its accessory numerals and/or bracket. */

#define BRACKETUP	(STD_LINEHT/2)				/* Dist. to move bracket up from bottom of numeral (STDIST) */

void DrawTUPLET(Document *doc, LINK tupL, CONTEXT context[])
{
	PTUPLET		tup;
	short		staff;
	PCONTEXT	pContext;
	DDIST		xd, yd, dTop, dLeft, acnxd, acnyd, temp,
				topStfTop, bottomStfTop, firstNoteyd, lastNoteyd;
	DDIST		brackDelta, yCutoffLen, dTuplWidth;
	DPoint		firstPt, lastPt;
	short		xp, yp, xColon;
	LINK		aNoteL, firstSyncL, lastSyncL,
				tNotes[MAXINTUPLET];
	Boolean		dim;							/* Should it be dimmed bcs in a voice not being looked at? */
	Rect		tupleRect;
	Boolean		firstBelow, lastBelow;			/* TRUE=bracket below the note at that end */
	unsigned char tupleStr[20];
	STFRANGE	stfRange;
	Point		enlarge = {0,0};
	short		tupletNumSize, tupletColonSize;

PushLock(OBJheap);

	tup = GetPTUPLET(tupL);
	staff = tup->staffn;
	pContext = &context[staff];
	TextSize(UseTextSize(pContext->fontSize, doc->magnify));

	GetTupletInfo(doc, tupL, pContext, &xd, &yd, &xColon, &firstPt, &lastPt, tupleStr,
						&dTuplWidth, &tupleRect);
	dLeft = pContext->measureLeft;
	dTop = pContext->measureTop;
	acnxd = dLeft+xd; acnyd = dTop+yd;
	firstPt.h += dLeft; firstPt.v += dTop;
	lastPt.h += dLeft; lastPt.v += dTop;

	GetTupleNotes(tupL, tNotes);

	/* Decide whether the bracket cutoffs should point up or down. */

	if (GetCrossStaff(LinkNENTRIES(tupL), tNotes, &stfRange)) {	
		if (staff!=stfRange.topStaff) {
			temp = stfRange.bottomStaff;
			stfRange.bottomStaff = stfRange.topStaff;
			stfRange.topStaff = temp;
		}
		topStfTop = context[stfRange.topStaff].staffTop;
		bottomStfTop = context[stfRange.bottomStaff].staffTop;

		firstSyncL = FirstInTuplet(tupL);
		lastSyncL = LastInTuplet(tupL);
		aNoteL = FindMainNote(firstSyncL, tup->voice);
		firstNoteyd = NoteXStfYD(aNoteL, stfRange, topStfTop, bottomStfTop, TRUE);
		aNoteL = FindMainNote(lastSyncL, tup->voice);
		lastNoteyd = NoteXStfYD(aNoteL, stfRange, topStfTop, bottomStfTop, TRUE);
	
		firstBelow = (firstNoteyd<=tup->ydFirst);
		lastBelow = (lastNoteyd<=tup->ydLast);
	}
	else
		firstBelow = lastBelow = (tup->ydFirst>0);
	
	brackDelta = std2d(BRACKETUP, pContext->staffHeight, pContext->staffLines);
	yCutoffLen = std2d(TUPLE_CUTOFFLEN, pContext->staffHeight, pContext->staffLines);

	dim = (outputTo==toScreen && !LOOKING_AT(doc, tup->voice));

	if (tup->numVis || doc->showInvis) {
		switch (outputTo) {
			case toScreen:
			case toBitmapPrint:
			case toPICT:
				ForeColor(Voice2Color(doc, tup->voice));
				xp = pContext->paper.left+d2p(acnxd-dTuplWidth/2);
				yp = pContext->paper.top+d2p(acnyd);

				MoveTo(xp, yp);
				if (dim) DrawMString(doc, (unsigned char *)tupleStr, NORMAL_VIS, TRUE);
				else		DrawString(tupleStr);
				if (tup->denomVis || doc->showInvis) {
					MoveTo(xp+xColon+1, yp-1);
					DrawMColon(doc, TRUE, dim, LNSPACE(pContext));
				}
				if (tup->brackVis || doc->showInvis)
					DrawTupletBracket(firstPt, lastPt, brackDelta, yCutoffLen, 
							acnxd, d2p(dTuplWidth), firstBelow, lastBelow, pContext, dim);
				ForeColor(blackColor);
				PenPat(NGetQDGlobalsBlack());
				break;
			case toPostScript:
				tupletNumSize = (config.tupletNumSize>0? config.tupletNumSize : 110);
				tupletColonSize = (config.tupletColonSize>0? config.tupletColonSize : 60);
				PS_MusString(doc,acnxd-dTuplWidth/2,acnyd,tupleStr,tupletNumSize);
				if (tup->denomVis || doc->showInvis)
					PS_MusColon(doc,acnxd,acnyd-(pContext->staffHeight/20),
									tupletColonSize,LNSPACE(pContext),TRUE);
				if (tup->brackVis || doc->showInvis) {
					DrawPSTupletBracket(firstPt, lastPt, brackDelta, yCutoffLen, 
							acnxd, d2pt(dTuplWidth), firstBelow, lastBelow, pContext);
					}
				break;
			default:
				break;
		}
	}
	else {																		/* Numbers not visible */
		if (tup->brackVis || doc->showInvis) {
			switch (outputTo) {
				case toScreen:
				case toBitmapPrint:
				case toPICT:
					DrawTupletBracket(firstPt, lastPt, brackDelta, yCutoffLen, 
							acnxd, -1, firstBelow, lastBelow, pContext, dim);
					break;
				case toPostScript:
					DrawPSTupletBracket(firstPt, lastPt, brackDelta, yCutoffLen, 
							acnxd, -1, firstBelow, lastBelow, pContext);
					break;
				default:
					break;
			}
		}
	}

	if (outputTo==toScreen && !LinkVALID(tupL)) {
		OffsetRect(&tupleRect, d2p(acnxd-dTuplWidth/2), d2p(acnyd));
		InsetRect(&tupleRect, pt2p(-2), pt2p(-2));
		LinkOBJRECT(tupL) = tupleRect;
	}

#ifdef USE_HILITESCORERANGE
	if (LinkSEL(pL))
		CheckTUPLET(doc, pL, context, (Ptr)NULL, SMHilite, stfRange, enlarge);
#endif

PopLock(OBJheap);
}


/* --------------------------------------------------------------------------------- */
/* General Tuplet Utilities. */

/* ----------------------------------------------------------- SelRangeChkTuplet -- */

static Boolean SelRangeChkTuplet(short v, LINK vStartL, LINK vEndL)
{
	LINK		pL, aNoteL;
	PANOTE	aNote;
	
	for (pL = vStartL; pL!=vEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteVOICE(aNoteL)==v) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->inTuplet) return TRUE;
					else break;
				}

	return FALSE;
}


/* -------------------------------------------------------------- HasTupleAcross -- */
/* If <node> is between notes on <staff> in a tupleSet return the tupleL, else
return NILINK. Will return NILINK if <node> is in between the tupleL and its
notes in the data structure. */

LINK HasTupleAcross(LINK node, short staff)
{
	PANOTE	lNote, rNote;
	LINK	lSyncL, rSyncL, lNoteL, rNoteL, tupleL;

	if (node==NILINK) return NILINK;
	lSyncL = LSSearch(LeftLINK(node), SYNCtype, staff, TRUE, FALSE);
	rSyncL = LSSearch(node, SYNCtype, staff, FALSE, FALSE);
	if (lSyncL==NILINK || rSyncL==NILINK)
		return NILINK;

	lNoteL = FirstSubLINK(lSyncL);
	for ( ; lNoteL; lNoteL = NextNOTEL(lNoteL)) {
		lNote = GetPANOTE(lNoteL);
		if (lNote->staffn==staff)
			if (!lNote->inTuplet) return NILINK;
	}
	rNoteL = FirstSubLINK(rSyncL);
	for ( ; rNoteL; rNoteL = NextNOTEL(rNoteL)) {
		rNote = GetPANOTE(rNoteL);
		if (rNote->staffn==staff)	{
			if (!rNote->inTuplet) return NILINK;

			tupleL = LSSearch(rSyncL, TUPLETtype, staff, TRUE, FALSE);
			return (IsAfterIncl(tupleL, lSyncL) ? tupleL : NILINK);
		}
	}

	return NILINK;
}

LINK VHasTupleAcross(
			LINK node,
			short v,
			Boolean skipNode)	/* TRUE=look for tuplet status to right of <node> & skip <node> itself */
{
	PANOTE	lNote, rNote;
	LINK		lSyncL, rSyncL, lNoteL, rNoteL, tupleL;

	if (node==NILINK) return NILINK;
	lSyncL = LVSearch(LeftLINK(node), SYNCtype, v, TRUE, FALSE);
	rSyncL = LVSearch((skipNode? RightLINK(node) : node), SYNCtype, v, FALSE, FALSE);
	if (lSyncL==NILINK || rSyncL==NILINK)
		return NILINK;

	lNoteL = FirstSubLINK(lSyncL);
	for ( ; lNoteL; lNoteL = NextNOTEL(lNoteL)) {
		lNote = GetPANOTE(lNoteL);
		if (lNote->voice==v)
			if (!lNote->inTuplet) return NILINK;
	}
	rNoteL = FirstSubLINK(rSyncL);
	for ( ; rNoteL; rNoteL = NextNOTEL(rNoteL)) {
		rNote = GetPANOTE(rNoteL);
		if (rNote->voice==v)	{
			if (!rNote->inTuplet) return NILINK;

			tupleL = LVSearch(rSyncL, TUPLETtype, v, TRUE, FALSE);
			return (IsAfterIncl(tupleL, lSyncL) ? tupleL : NILINK);
		}
	}

	return NILINK;
}


/* -------------------------------------------------- FirstInTuplet, LastInTuplet -- */

LINK FirstInTuplet(LINK tupletL)
{
	PANOTETUPLE aNoteTuple;
	
	aNoteTuple = GetPANOTETUPLE(FirstSubLINK(tupletL));
	return (aNoteTuple->tpSync);
}

LINK LastInTuplet(LINK tupletL)
{
	LINK aNoteTupleL;
	PANOTETUPLE aNoteTuple;
	short i, nInTuplet;	
	
	nInTuplet = LinkNENTRIES(tupletL);
	aNoteTupleL = FirstSubLINK(tupletL);
	for (i = 0; i<nInTuplet; i++, aNoteTupleL = NextNOTETUPLEL(aNoteTupleL))
		aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
	
	return (aNoteTuple->tpSync);
}


/* --------------------------------------------------------------- SetBracketsVis -- */
/*  Set visibility of tuplet brackets to the conventional default. */

Boolean SetBracketsVis(Document *doc, LINK startL, LINK endL)
{
	LINK pL, firstL, lastL; Boolean brackVis, didSomething=FALSE;
	PTUPLET pTuplet;
	
	for (pL = startL; pL && pL!=endL; pL = RightLINK(pL))
		if (TupletTYPE(pL)) {
			firstL = FirstInTuplet(pL); lastL = LastInTuplet(pL);
			brackVis = GetBracketVis(doc, firstL, lastL, TupletVOICE(pL));
			pTuplet = GetPTUPLET(pL);
			if (pTuplet->brackVis!=brackVis) {
				pTuplet->brackVis = brackVis;
				didSomething = TRUE;
			}
	}
	
	return didSomething;
}
