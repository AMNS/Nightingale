/*	RTRhythmDur.c for Nightingale - routines for identifying tuplets, quantizing,
and clarifying rhythm in real-time situations. No user-interface implications. */

/*										NOTICE
 *
 *	THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROPERTY OF
 *	ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE SECRET AND IS
 *	NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED WRITTEN
 *	AUTHORIZATION FROM THE OWNER.
 *
 *	Copyright ©1992-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include <math.h>						/* For sqrt() prototype */

static Boolean ClarToMeasFromList(Document *, LINK, short, short, short, LINK []);
static short FindTimeSig(long, short, MEASINFO [], short, long *, long *, long *);
static Boolean TooManySyncs(short, short);
static short Clar1ToMeas(Document *, LINKTIMEINFO [], short, short *, short *, MEASINFO [],
							short, short *);

static void FindTimeRange(long, long, LINKTIMEINFO [], short, short *, short *);
static short FitTime(LINKTIMEINFO [], short, short, short, short, short, short);
static void LevelChooseTuplets(long, long, short, LINKTIMEINFO [], short, short, short, short);
static void ChooseTuplets(long, long, short, short, LINKTIMEINFO [], short, short, short);
static short GetNInTuple(LINKTIMEINFO [], short, short, short);
static Boolean Clar1ToTuplet(Document *, short, LINKTIMEINFO [], short *, short, unsigned short);

static Boolean MFClarifyFromList(Document *, LINK, short, short, NOTEPIECE [], short, PCONTEXT,
									LINKTIMEINFO [], short *);
static Boolean MFSetNRCDur(LINK, short, unsigned short, short, short, Boolean);
static Boolean Clar1BelowMeas(Document *, short, LINKTIMEINFO, short, MEASINFO [], short, short *,
								LINKTIMEINFO [], unsigned short, short *);

static Boolean Add1Tuplet(Document *, short, LINKTIMEINFO [], short, short);


/* Simple-minded tuplet denominator for given numerator: cf. GetTupleDenom */
#define MF_TupleDenom(num) ((num)==5? 4 : 2)


/* ---------------------------------------------------------- Clarify to Measures -- */

#define CHECK_TS(m)																								\
if (TSNUM_BAD(measInfoTab[(m)].numerator) || TSDENOM_BAD(measInfoTab[(m)].denominator)) \
	MayErrMsg("Time signature %ld/%ld is illegal.",														\
				(long)measInfoTab[(m)].numerator, (long)measInfoTab[(m)].denominator)


short FindTimeSig(long timeUsed, short m, MEASINFO measInfoTab[], short measTabLen,
						long *tsStartTime, long *tsEndTime, long *measDur)
{
	while (timeUsed>=*tsEndTime || m<0) {
		m++;
		CHECK_TS(m);
		*tsStartTime = *tsEndTime;
		*measDur = TimeSigDur(0, measInfoTab[m].numerator, measInfoTab[m].denominator);
		
		/* If we've reached the end of the time sig. table, let the last entry go to
			"infinity". Various things including quantization might make the last notes
			go beyond the supposed end of the file, and it doesn't much matter, anyway. */
			 
		if (m==measTabLen-1) *tsEndTime = BIGNUM;
		else					   *tsEndTime += (*measDur)*measInfoTab[m].count;
	}
	
	return m;
}


Boolean TooManySyncs(short numNeed, short maxSyncs)
{
	char fmtStr[256];
	
	if (numNeed>=maxSyncs) {
		GetIndCString(fmtStr, RHYTHMERRS_STRS, 5);    /* "While clarifying rhythm, there are over %d Syncs, which is more than Nightingale expected." */
		sprintf(strBuf, fmtStr, maxSyncs); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return TRUE;
	}
	
	return FALSE;
}


/* Rhythmically "clarify" the given note/chord (NC) down to the Measure level, i.e.,
if it extends across two or more Measures, divide it into a series of unquantized NCs
that don't cross Measure boundaries. We simply add descriptions of the new NCs to
<rawSyncTab>. Return OP_COMPLETE if we clarify anything, FAILURE if we try to but
can't, or NOTHING_TO_DO. */

short Clar1ToMeas(
			Document *doc,
			LINKTIMEINFO rawSyncTab[],		/* Input and output: unclarified NCs */
			short maxRawSyncs,				/* Maximum size of rawSyncTab */
			short *pr,							/* Index into rawSyncTab */
			short *pnRawSyncs,				/* Current size of rawSyncTab */
			MEASINFO	measInfoTab[],
			short measTabLen,
			short *pm 							/* Index into measInfoTab; -1 = initialize */
			)
{
	short nPieces, nNewSyncs;
	short pieceDur[MF_MAXPIECES], i, n, k, nInTimeSig;
	long startTime, timeUsed, timeUsedInMeas, timeLeftInMeas, lDur, segDur;
	static long measDur, tsStartTime, tsEndTime;
	
	startTime = timeUsed = rawSyncTab[*pr].time;
	
	if (*pm<0) tsEndTime = 0L;

	/*
	 *	Find the lengths into which to divide the NC. Note that the time signature
	 * might	change during the NC, possibly more than once. Each piece will exactly
	 * fill a measure except for the first and last pieces, which may or may not.
	 */
	nPieces = 0;
	lDur = rawSyncTab[*pr].mult;
	
	/* The next note might start long before this one ends, for example if we're working
		on a track of a MIDI file, since MIDI file tracks can contain arbitrary polyphony.
		If there's any such overlap, we'd better fix it now, since we're only going to
		generate one (homophonic) voice for the track. For now, do the simplest thing:
		just truncate this note. (Ray Spears' fancy solution is to have (e.g.) a single
		note tied to a dyad tied to a single note.) */
		
	if (*pr<*pnRawSyncs-1 && rawSyncTab[*pr+1].time<startTime+lDur)
		lDur = rawSyncTab[*pr+1].time-startTime;
	
	while (lDur>0L) {
		*pm = FindTimeSig(timeUsed, *pm, measInfoTab, measTabLen, &tsStartTime,
									&tsEndTime, &measDur);

		/* The time sig. in effect at this pt in the NC goes from tsStartTime to tsEndTime. */
		
		if (nPieces==0) {
			timeUsedInMeas = (timeUsed-tsStartTime) % measDur;
			timeLeftInMeas = measDur-timeUsedInMeas;
			if (lDur<timeLeftInMeas) return NOTHING_TO_DO;	/* The NC already fits in a measure */
		}
		
		segDur = n_min(lDur, tsEndTime-timeUsed);				/* Duration in this time sig. */
		nInTimeSig = segDur/measDur;
		if (nInTimeSig*measDur<segDur) nInTimeSig++;			/* Round up */
		for (i = nPieces; i<nPieces+nInTimeSig; i++) {
			if (i==0)				  pieceDur[i] = timeLeftInMeas;
			else if (measDur>lDur) pieceDur[i] = lDur;
			else						  pieceDur[i] = measDur;
			lDur -= pieceDur[i];
		}
		nPieces += nInTimeSig;

		timeUsed += segDur;
	}

	nNewSyncs = nPieces-1;
	if (TooManySyncs(*pnRawSyncs+nNewSyncs, maxRawSyncs))
		return FAILURE;

	/*
	 *	<pieceDur> now contains a list of durations for the segments we need to
	 * divide our NC into. Open up space for the new entries in the table... */
	
	for (n = *pnRawSyncs-1; n>*pr; n--)
		rawSyncTab[n+nNewSyncs] = rawSyncTab[n];
		
	/* And fill them in. */
	
	timeUsed = startTime;
	for (k = 0; k<nPieces; k++) {
		rawSyncTab[*pr+k].link = rawSyncTab[*pr].link;
		rawSyncTab[*pr+k].time = timeUsed;
		rawSyncTab[*pr+k].mult = pieceDur[k];
		rawSyncTab[*pr+k].added = (k>0);
		rawSyncTab[*pr+k].tupleNum = 0;							/* Assume not a tuplet */

		timeUsed += pieceDur[k];
	}

	*pr += nNewSyncs;
	*pnRawSyncs += nNewSyncs;
	doc->changed = TRUE;
	
	return OP_COMPLETE;
}


/* Clarify down to the Measure level: break every note/rest/chord that crosses measure
boundaries at each measure boundary. */

Boolean ClarAllToMeas(
				Document *doc,
				LINKTIMEINFO rawSyncTab[],		/* Input and output: unclarified NRCs */
				short maxRawSyncs,				/* Maximum size of rawSyncTab */
				short *pnRawSyncs,				/* Current size of rawSyncTab */
				short /*quantum*/,						/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
				MEASINFO	measInfoTab[],
				short measTabLen
				)
{
	short m, q;
	
	for (q = 0; q<*pnRawSyncs; q++) {
		rawSyncTab[q].added = FALSE;
		rawSyncTab[q].tupleNum = 0;				/* Assume not a tuplet */
	}
	
	m = -1;
	for (q = 0; q<*pnRawSyncs; q++) {
		/* Clar1ToMeas clarifies the rhythm by dividing the note/chord at measure
			boundaries, storing info about the pieces back into rawSyncTab. */
		
		if (Clar1ToMeas(doc, rawSyncTab, maxRawSyncs, &q, pnRawSyncs, measInfoTab,
								measTabLen, &m)==FAILURE)
			return FALSE;
	}
	
	return TRUE;
}


/* --------------------------------------------------------------- Handle Tuplets -- */

/* Return the indices of the range of NRCs in rawSyncTab[] that fall partly or
entirely in the range of times [startTime,endTime); if there are none, return both
indices <0. NB: nRawSyncs might be pretty big: it would be better to use binary search
for speed. */

void FindTimeRange(
			long startTime, long endTime,
			LINKTIMEINFO rawSyncTab[],
			short nRawSyncs,
			short *pStart, short *pEnd 	/* 1st entry in range, 1st AFTER range: -1=none in range */
			)
{
	short q, qEndTime;
	
	*pStart = *pEnd = -1;
	
	for (q = 0; q<nRawSyncs; q++) {
		/* Set *pStart to the first item that ENDS in the range. */
		
		if (q<nRawSyncs-1) {							/* Range can't start with the very last NRC */
			qEndTime = rawSyncTab[q+1].time-1;
			if (qEndTime>=startTime && qEndTime<endTime && *pStart<0) *pStart = q;
		}
			
		/* Set *pEnd to the last item that BEGINS in the range. */
		
		if (rawSyncTab[q].time>=startTime && rawSyncTab[q].time<endTime) *pEnd = q;
		
		if (rawSyncTab[q].time>=endTime) break;
	}
	
	if (*pEnd>=0) (*pEnd)++;	/* We want the first rawSyncTab entry AFTER the range */
}


/* Decide whether attack times in the range [start,end) in rawSyncTab[] can be
approximated more closely with a tuplet than with normal notes, considering the given
bias. Return the tuplet numerator if so (3 = 3:2 is the only possibility right now),
0 if not. */

short FitTime(
			LINKTIMEINFO rawSyncTab[],
			short /*nRawSyncs*/,
			short start, short end,
			short rangeDur,			/* Duration of the range being considered */
			short quantum,				/* for the non-tuplet version, in PDUR ticks */
			short tripletBias 		/* Bias towards triplets (roughly, -100=no triplets, +100=max.) */
			)
{
	short quantumN, round=quantum/2, roundN, q, scaledTripBias;
	long temp, qTime, error, totalError, totalErrorN;
	
	/* Scale <tripletBias> to cover range of -2*quantum to 2*quantum . */
	
	scaledTripBias = tripletBias*2L*quantum/100L;

	/* Do either least-linear or least-squares fit of attack times in rawSyncTab[q].
		Note that these times are from the beginning of the score, not the measure
		or the beat; however, we care only about the error in the closest multiple
		of <quantum>, and that will be correct as long as every previous measure has
		a duration that's a multiple of <quantum>. */
	
	totalError = 0;
	for (q = start; q<end; q++) {
		temp = (rawSyncTab[q].time+round)/quantum;
		qTime = temp*quantum;
		error = ABS(rawSyncTab[q].time-qTime);
		totalError += (config.leastSquares? error*error : error);	/* non-tuplet error */
	}
	
	quantumN = rangeDur/3;
	roundN = quantumN/2;
	totalErrorN = 0;
	for (q = start; q<end; q++) {
		temp = (rawSyncTab[q].time+roundN)/quantumN;
		qTime = temp*quantumN;
		error = ABS(rawSyncTab[q].time-qTime);
		totalErrorN += (config.leastSquares? error*error : error);	/* triplet error */
	}

	if (config.leastSquares)
		return (sqrt(totalErrorN-scaledTripBias)<totalError? 3 : 0);
	else
		return (totalErrorN-scaledTripBias<totalError? 3 : 0);
}


/* NB: In the following functions, "duration level of the beat" means the tuplet's
TOTAL duration is 1 beat. For example, in 4/4, beat level is total duration a quarter,
the level above the beat is total duration a half, the level below is total duration
an eighth. */

#define MAX_TUP_SLOTS (3*64)			/* Max. subbeats in any reasonable time sig. */

Boolean slotTupled[MAX_TUP_SLOTS];

short tupLevel[3];	 						/* 1=1 level above beat, 2=beat, 3=1 below beat */

/* Decide which NRCs in the given measure in rawSyncTab[] should be tupleted at the
given duration level, and store the results in the tuplet fields of rawSyncTab[].
Also set <slotTupled> flags so we don't try to tuplet these NRCs again. */

void LevelChooseTuplets(
			long measStartTime, long measEndTime,
			short levBtDur,				/* Total dur. of tuplet on the desired level */
			LINKTIMEINFO rawSyncTab[],
			short nRawSyncs,
			short quantum,					/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
			short tripletBias,			/* Percent bias towards triplets >= 2/3 durQuantum */
			short subBtDur
			)
{
	long btStartTime, btEndTime; Boolean skip;
	short i, startSlot, endSlot, start, end, bestNum;

	if (levBtDur<2*quantum) return;	
	
	btStartTime = measStartTime;
	for ( ; btStartTime<measEndTime; btStartTime += levBtDur) {
		btEndTime = btStartTime+levBtDur;
		startSlot = (btStartTime-measStartTime)/subBtDur;
		endSlot = (btEndTime-measStartTime)/subBtDur;
		for (skip = FALSE, i = startSlot; i<endSlot; i++)
			if (slotTupled[i]) skip = TRUE;					/* Can't tuple if already tupled */
		if (skip) continue;
		
		FindTimeRange(btStartTime, btEndTime, rawSyncTab, nRawSyncs, &start, &end);
		if (start<0 || end<0) continue; 						/* No notes: can't tuple */
		if (end-start<2) continue;								/* Fewer than 2 notes: can't tuple */

		bestNum = FitTime(rawSyncTab,nRawSyncs,start,end,levBtDur,quantum,tripletBias);
		if (bestNum!=0) {
			rawSyncTab[start].tupleNum = bestNum;
			rawSyncTab[start].tupleDur = levBtDur;
			rawSyncTab[start].tupleTime = btStartTime-rawSyncTab[start].time;
			for (i = startSlot; i<endSlot; i++)
				slotTupled[i] = TRUE;
		}
	}
}


/* Decide which NRCs in the given measure in rawSyncTab[] should be tupleted. For now,
don't consider tuplets if meter is compound. */

void ChooseTuplets(
			long measStartTime,
			long measEndTime,
			short numer, short denom,	/* Time signature */
			LINKTIMEINFO rawSyncTab[],
			short nRawSyncs,
			short quantum,					/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
			short tripletBias 			/* Percent bias towards triplets >= 2/3 durQuantum */
			)
{
	short start, end, beatDur, subBeatDur, i, k;
	short levBtDur;								/* Total dur. of tuplet on the desired level */
	Boolean compound, tryOnlyOne;
	
	compound = COMPOUND(numer);
	if (compound) return;										/* for the near future, at least */
	if (tripletBias<=-100) return;
	
	FindTimeRange(measStartTime, measEndTime, rawSyncTab, nRawSyncs, &start, &end);
	if (start<0 || end<0) return; 							/* No notes: can't tuple */
	if (end-start<2) return;									/* Fewer than 2 notes: can't tuple */
	
	beatDur = (long)l2p_durs[WHOLE_L_DUR]/(long)denom;
	if (compound) beatDur *= 3;
	subBeatDur = (compound? beatDur/3 : beatDur/2);

	/* If the meter has too many subbeats to keep track of, try only one level. */
	
	tryOnlyOne = ((measEndTime-measStartTime)/subBeatDur>MAX_TUP_SLOTS);

	/* Check tuplet possibilities on several levels:
		-the beat level;
		-the level above the beat, if it's simple meter and there's more than one beat per
			measure; and
		-the level below the beat. */

	for (i = 0; i<MAX_TUP_SLOTS; i++)
		slotTupled[i] = FALSE;
	
	for (k = 0; k<3; k++)
		if (tupLevel[k]>=1 && tupLevel[k]<=3) {
			if (tupLevel[k]==1) {
				if (!compound && 2*beatDur<=measEndTime-measStartTime) levBtDur = 2*beatDur;
				else continue;
			}
			else if (tupLevel[k]==2) levBtDur = beatDur;
			else if (tupLevel[k]==3) levBtDur = subBeatDur;
			LevelChooseTuplets(measStartTime, measEndTime, levBtDur, rawSyncTab, nRawSyncs,
									quantum, tripletBias, subBeatDur);
			if (tryOnlyOne) break;
		}
}


Boolean ChooseAllTuplets(
				Document */*doc*/,
				LINKTIMEINFO rawSyncTab[],	/* Input: raw (unquantized and unclarified) NCs */
				short /*maxRawSyncs*/,		/* Maximum size of rawSyncTab */
				short nRawSyncs,				/* Current size of rawSyncTab */
				short quantum,					/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
				short tripletBias,			/* Percent bias towards quant. to triplets >= 2/3 durQuantum */
				MEASINFO	measInfoTab[],
				short measTabLen
				)
{
	short i, m, shiftDenom, k; long measDur, measStartTime=0L, measEndTime;
	
	shiftDenom = 100;
	for (k = 0; k<3; k++, shiftDenom /= 10)	
		tupLevel[k] = (config.tryTupLevels/shiftDenom) % 10;
	
	/* Decide tuplets for NRCs in one Measure at a time. */
	
	for (i = 0; i<measTabLen; i++) {
		if (measInfoTab[i].count==0) continue;		/* in case of consecutive time sigs. */
		
		measDur = TimeSigDur(N_OVER_D, measInfoTab[i].numerator, measInfoTab[i].denominator);

		for (m = 0; m<measInfoTab[i].count; m++) {
			measEndTime = measStartTime+measDur;
			ChooseTuplets(measStartTime, measEndTime, measInfoTab[i].numerator,
						measInfoTab[i].denominator, rawSyncTab, nRawSyncs, quantum, tripletBias);
			measStartTime += measDur;
		}
	}
	
	return TRUE;
}


/* Find the number of NRCs in the tuplet whose first element is syncTab[start]. If
we can't find the end of the tuplet, assume it ends with the last element of syncTab[].
Assumes syncTab[] has already been clarifed to the tuplet level. */

short GetNInTuple(
			LINKTIMEINFO syncTab[],		/* Partly- or fully-clarified NRCs */
			short nSyncs,					/* Current size of syncTab */
			short start,					/* Index in syncTab of first element */
			short quantum 					/* In PDUR ticks, or <=1=already quantized */
			)
{
	short n;
	short tupQuantum, tupRound;		/* Quantum considering the current tuplet */
	long temp, tupEndTime, qStartTime;

	if (quantum>1) {
		tupQuantum = syncTab[start].tupleDur/syncTab[start].tupleNum;
		tupRound = tupQuantum/2;
	}

	tupEndTime = syncTab[start].time+syncTab[start].tupleDur;
	for (n = 0; start+n<nSyncs; n++) {
		if (quantum>1) {
			temp = (syncTab[start+n].time+tupRound)/tupQuantum;
			qStartTime = temp*tupQuantum;
		}
		else
			qStartTime = syncTab[start+n].time;
		if (qStartTime>=tupEndTime) return n;
	}
	return (nSyncs-start);
}


/* We use rawSyncTab[].link==REST_LINKVAL to mean "put a rest here", so "real" link
values in rawSyncTab[] must never have this value! */

#define REST_LINKVAL 65535

/* Clarify the given NC by dividing it at the given point. Note that keeping a raw
NC from crossing tuplet boundaries may require more than one division, hence more
than one call. */

static Boolean Clar1ToTuplet(
						Document */*doc*/,
						short /*voice*/,
						LINKTIMEINFO rawSyncTab[],	/* Input and output: unclarified NCs */
						short *pnRawSyncs,
						short nDiv,						/* Index of the NC that crosses a tuplet boundary */
						unsigned short divTime 		/* Division point, relative to the start of the NC */
						)
{
	short n; unsigned short newMult; Boolean restFill;
	
	/*
	 * Add an identical NC that will start the tuplet (or region FOLLOWING a tuplet--
	 * the logic is the same). If the NC doesn't really extend even to the beginning of
	 *	the tuplet, the added one should be replaced later with a rest. Open up space for
	 *	the new NC in the table by copying everything after it down a slot; go far enough
	 *	to copy the new one from its "parent", rawSyncTab[nDiv], then correct it for the
	 *	new situation.
	 */
	
	for (n = *pnRawSyncs-1; n>=nDiv; n--)
		rawSyncTab[n+1] = rawSyncTab[n];
		
	restFill = rawSyncTab[nDiv].mult<divTime;
	if (restFill) rawSyncTab[nDiv+1].link = REST_LINKVAL;

	rawSyncTab[nDiv+1].time = rawSyncTab[nDiv].time+divTime;
	newMult = rawSyncTab[nDiv].mult-divTime;
	rawSyncTab[nDiv+1].mult = (newMult<0? 0 : newMult);
	rawSyncTab[nDiv+1].added = !restFill;
	/* Keep the tupleNum and tupleDur fields already copied from "parent" */ 
	rawSyncTab[nDiv+1].tupleTime = 0;

	/* Finally, update the "parent" and the table length. */
	
	rawSyncTab[nDiv].mult = divTime;
	rawSyncTab[nDiv].tupleNum = 0;				/* Parent doesn't start a tuplet after all */

	(*pnRawSyncs)++;
	return TRUE;
}


/* Clarify down to the Tuplet level: break every note/rest/chord that crosses tuplet
boundaries at each tuplet boundary. Simply adds information on the clarified NCs to
rawSyncTab[]. */

Boolean ClarAllToTuplets(
				Document *doc,
				short voice,
				LINKTIMEINFO rawSyncTab[],	/* Input and output: unclarified NCs */
				short maxRawSyncs,			/* Maximum size of rawSyncTab */
				short *pnRawSyncs,
				short /*quantum*/ 			/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
				)
{
	long endTupTime, endThisTime;
	short q, tupleNum, endTuplet, qr; unsigned short divTime;
	
	/* Break NRCs at tuplet boundaries. */

	endTupTime = 0L;
	for (q = 0; q<*pnRawSyncs; q++) {
		/* This may be the end of one tuplet and/or the beginning of another. */
		
		if (rawSyncTab[q].time>=endTupTime)
			tupleNum = 0;
		if (rawSyncTab[q].tupleNum>0) {
			tupleNum = rawSyncTab[q].tupleNum;
			endTupTime = rawSyncTab[q].time+rawSyncTab[q].tupleDur;
		}

		if (rawSyncTab[q].tupleNum>0) {
			if (rawSyncTab[q].tupleTime>0) {
				/* Entry q crosses start of tuplet: divide it AND SKIP TO NEXT ENTRY, i.e.,
					the brand-new one, which is now the start of the tuplet. */
					
				if (TooManySyncs(*pnRawSyncs+1, maxRawSyncs))
					return FALSE;
				if (!Clar1ToTuplet(doc, voice, rawSyncTab, pnRawSyncs, q, rawSyncTab[q].tupleTime))
					return FALSE;
				q++;
				endTupTime = rawSyncTab[q].time+rawSyncTab[q].tupleDur;
			}

			for (qr = q+1; qr<*pnRawSyncs; qr++)
				if (rawSyncTab[qr].time>=endTupTime) break;
			endTuplet = (rawSyncTab[qr].time>=endTupTime? qr : *pnRawSyncs);
			if (endTuplet>*pnRawSyncs) {
				MayErrMsg("ClarAllToTuplets: tuplet goes past end of rawSyncTab");
				return FALSE;
			}

			endThisTime = (endTuplet<*pnRawSyncs? rawSyncTab[endTuplet].time
								: rawSyncTab[endTuplet-1].time+rawSyncTab[endTuplet-1].mult);
			if (endThisTime>endTupTime) {
				/* Entry endTuplet-1 crosses end of tuplet: divide it ??AND SKIP?? */
				
				if (TooManySyncs(*pnRawSyncs+1, maxRawSyncs))
					return FALSE;
				divTime = endTupTime-rawSyncTab[endTuplet-1].time;
				if (!Clar1ToTuplet(doc, voice, rawSyncTab, pnRawSyncs, endTuplet-1, divTime))
					return FALSE;
			}
		}
	}
	
	return TRUE;
}


/* ------------------------------------------------------- RemoveShortAddedItems -- */
/* Remove "added" NRC's that have very short duration. */

static void RemoveShortAddedItems(LINKTIMEINFO [], short *, short);
static void RemoveShortAddedItems(
					LINKTIMEINFO rawSyncTab[],	/* Input: semi-raw (partly-clarified) NCs */
					short *pnRawSyncs,			/* Input and output: size of rawSyncTab */
					short quantum 					/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
					)
{
	short q, nSkipped;
	
	nSkipped = 0;
	for (q = 0; q<*pnRawSyncs; q++) {
		
		if (rawSyncTab[q].added && rawSyncTab[q].mult<quantum/2 && q<*pnRawSyncs-1) {
			/* This NRC should be removed. Before doing so, if this one starts a
				tuplet, move the tuplet information to the following NRC. */
			
			nSkipped++;
			
			if (rawSyncTab[q].tupleNum && q+1<*pnRawSyncs-1) {
				rawSyncTab[q+1].tupleNum = rawSyncTab[q].tupleNum;
				rawSyncTab[q+1].tupleDur = rawSyncTab[q].tupleDur;
				rawSyncTab[q+1].tupleTime = rawSyncTab[q].tupleTime;
			}
		}

		/* Move this NRC into the correct slot. */
			
		else if (nSkipped>0)
			rawSyncTab[q-nSkipped] = rawSyncTab[q];
	}

	*pnRawSyncs -= nSkipped;
}


/* ----------------------------------------------------------------- ChooseChords -- */
/* For each item in <rawSyncTab>, get the information about its notes from <rawNoteAux>
and decide how they should be synced--i.e., since we're only dealing with one voice,
which ones should be combined into a chord. Output is by modifying <rawSyncTab>.

To simplify things for itself, especially in case of changing time sigs., ChooseChords
assumes the notes/chords ("NC"s) never cross measure boundaries; they may cross tuplet
boundaries.

NB: ChooseChords has to know how things are going to be quantized, including whether
they'll be in a tuplet and, if so, what kind; but it doesn't actually do the quantizing:
it leaves that to somebody else. */ 

Boolean ChooseChords(
				Document */*doc*/,
				short /*voice*/,				/* Same as the staff, since we use only default voices */
				LINKTIMEINFO rawSyncTab[],	/* Input and output: raw (unquantized and unclarified) NCs */
				short /*maxRawSyncs*/,		/* Maximum size of rawSyncTab */
				short *pnRawSyncs,			/* Current size of rawSyncTab */
				NOTEAUX rawNoteAux[],		/* Input: raw notes */
				short quantum 					/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
				)
{
	short endTuplet, q, qr, n;
	short tupQuantum, nextTupQuantum, tupRound;		/* Quantum considering the current tuplet */
	long endTupTime, qStartTime, prevStartTime, qPrevStartTime, temp;
	Boolean chordWithLast, okay=FALSE;
	
	if (quantum<=1) return TRUE;
	/*
	 * Discard any "added" items that are very short immediately, before they cause trouble.
	 */
	RemoveShortAddedItems(rawSyncTab, pnRawSyncs, quantum);

	qPrevStartTime = prevStartTime = -999999L;

	endTuplet = 0;								/* So we'll initialize the first time thru */
	for (q = 0; q<*pnRawSyncs; q++) {
		/* One tuplet may end here or in the middle of the previous NC; another may
			start at the beginning of or in the middle of this NC. Set quantization
			according to the tuplet or non-tuplet in effect at the beginning of this
			NC, and set "next" quantization for what's in effect at its end. */

		if (q==endTuplet)
			nextTupQuantum = tupQuantum = quantum;
		
		if (rawSyncTab[q].tupleNum>0) {
			nextTupQuantum = rawSyncTab[q].tupleDur/rawSyncTab[q].tupleNum;
			if (rawSyncTab[q].tupleTime==0) tupQuantum = nextTupQuantum;
			endTupTime = rawSyncTab[q].time+rawSyncTab[q].tupleTime
								+rawSyncTab[q].tupleDur;
			for (qr = q+1; qr<*pnRawSyncs; qr++)
				if (rawSyncTab[qr].time>=endTupTime) break;
			endTuplet = (rawSyncTab[qr].time>=endTupTime? qr : *pnRawSyncs);
		}

		/*
		 *	Musically, it seems fine to round times in the obvious way, except it's much
		 *	better to round the midpoint to the beginning of the interval than to the
		 *	end. If it's ever desired to round ALL notes to the beginning of their
		 *	"beats", just set tupRound to 0.
		 */
		tupRound = (tupQuantum/2)-1;

		temp = (rawSyncTab[q].time+tupRound)/tupQuantum;
		qStartTime = temp*tupQuantum;
		chordWithLast = (qStartTime-qPrevStartTime<tupRound);

		qPrevStartTime = qStartTime;
		
		tupQuantum = nextTupQuantum;
				
		if (chordWithLast) {
			if (rawSyncTab[q].added) {
				/*
				 * if this NC is <added>, it's a continuation of a previous NC, and the
				 * only way they could have ended up in the same chord is by the previous
				 *	one's attack time being rounded up; but THIS one has the duration we
				 *	want, so replace the chord's duration with this one's, then skip it.
				 */
				rawSyncTab[q-1].mult = rawSyncTab[q].mult;
				rawSyncTab[q-1].tupleNum = rawSyncTab[q].tupleNum;
				rawSyncTab[q-1].tupleDur = rawSyncTab[q].tupleDur;
				rawSyncTab[q-1].tupleTime = rawSyncTab[q].tupleTime;
			}
			else {
				rawNoteAux[rawSyncTab[q].link].first = FALSE;
				prevStartTime = rawSyncTab[q].time;
					/*
					 *	If the next NC is <added>, it's a continuation of this one; since this
					 * NC is being merged into the previous one, extend the previous one's dur.
					 * to last until the end of this one so there's no gap before the next one.
					 * (THIS NC can't be <added> and being merged into the previous one, or we
					 *	wouldn't have gotten here.)
					 */
					if (q<*pnRawSyncs-1 && rawSyncTab[q+1].added)
						rawSyncTab[q-1].mult += rawSyncTab[q].mult;
			}

			/*
			 *	Close up space occupied by the NRC in the table; then decrement <q>,
			 *	since we haven't yet handled the item that's NOW rawSyncTab[q].
			 */
			for (n = q; n<*pnRawSyncs; n++)
				rawSyncTab[n] = rawSyncTab[n+1];
			(*pnRawSyncs)--;
			
			q--;

		}
		else
			prevStartTime = rawSyncTab[q].time;
	}
	
	okay = TRUE;

Done:
	return okay;
}


/* -------------------------------------------------------------- QuantizeAndClip -- */
/* QuantizeAndClip converts raw timing information in <rawSyncTab> to quantized form,
considering tuplets, if desired. For each note/rest/chord in <rawSyncTab>, it rounds
the <time>, given in ticks, to a multiple of <quantum>; and it converts the <mult> and
<tupleDur>, given in ticks, to a multiple of <quantum> adjusted for the chosen tuplet.
For example, if <quantum> is 120 and an NRC's time=223 and mult=152, the results will be
time=240 and, normally, mult=120. If the NRC is inside a triplet, it'll be mult=160.

We also "clip" durations after quantizing to keep the NRC's from overlapping. Note that,
if consecutive NRC's get quantized to the same time, the first one will get a duration
of zero. In this case, it's simplest for subsequent processing to just ignore it, but
caveat: its <rawSyncTab> entry may carry tuplet information, which should NOT be
ignored! In the opposite case, where there's a gap between consecutive NRC's, we do
nothing unless the gap is within a tuplet; if it is, we add a rest to fill the gap.

Handling tuplets makes matters much more difficult. To simplify things, especially in
case of changing time sigs., QuantizeAndClip assumes the NRCs never cross measure
boundaries or tuplet boundaries. */

void QuantizeAndClip(
			Document */*doc*/,
			short /*voice*/,
			LINKTIMEINFO rawSyncTab[],	/* Input and output: unclarified NCs */
			short *pnRawSyncs,
			short quantum 					/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
			)
{
	short round=quantum/2, q, prev, endTuplet, n;
	long prevEndTime, temp, overlapTime, newMult;
	long tupTime, timeSinceTup;				/* Time at last tuplet boundary, time since then */
	long endTupTime;
	short tupQuantum, tupRound;				/* Quantum considering the current tuplet */
		
	if (quantum==1) return;
	
	tupQuantum = quantum;
	tupRound = round;
	tupTime = 0L;

	prevEndTime = 0L;
	endTuplet = SHRT_MAX;
	for (q = 0; q<*pnRawSyncs; q++) {
		
		/* This may be the end of one tuplet and/or the beginning of another. (Since
			the NRCs don't cross tuplet boundaries, we can ignore their tupleTimes.) */
		
		if (q==endTuplet) {
			tupQuantum = quantum;
			tupRound = round;

			/* Time at tuplet boundary should always be quantized as a non-tuplet. */
			
			temp = (rawSyncTab[q].time+round)/quantum;
			tupTime = temp*quantum;
		}
		if (rawSyncTab[q].tupleNum>0) {
			tupQuantum = rawSyncTab[q].tupleDur/rawSyncTab[q].tupleNum;
			tupRound = tupQuantum/2;

			/* Time at tuplet boundary should always be quantized as a non-tuplet. */

			temp = (rawSyncTab[q].time+round)/quantum;
			tupTime = temp*quantum;

			endTupTime = tupTime+rawSyncTab[q].tupleDur;
			endTuplet = q+GetNInTuple(rawSyncTab, *pnRawSyncs, q, quantum);
		}

		/* Quantize attack time, duration, and <tupleDur>. Just round the latter two to
		 	ticks multiple of <tupQuantum>. Attack time is trickier: we need to make the
		 	time since the end of the last tuplet, if any, a multiple of <tupQuantum>.*/
		
		timeSinceTup = rawSyncTab[q].time-tupTime;
		temp = (timeSinceTup+tupRound)/tupQuantum;
		timeSinceTup = temp*tupQuantum;
		rawSyncTab[q].time = tupTime+timeSinceTup;

		temp = (rawSyncTab[q].mult+tupRound)/tupQuantum;
		rawSyncTab[q].mult = temp*tupQuantum;
		if (rawSyncTab[q].mult==0) rawSyncTab[q].mult = tupQuantum;			/* Avoid zero duration */

		if (rawSyncTab[q].tupleNum>0) {
			temp = (rawSyncTab[q].tupleDur+tupRound)/tupQuantum;
			rawSyncTab[q].tupleDur = temp*tupQuantum;
		}

		/*	If it's past time for this Sync, truncate as many previous Sync(s) as necessary
			so they don't overlap it. If one or more consecutive previous Syncs were
			generated by ClarAllToMeas, it's possible this one will start at the same time
			or even earlier than them, so the previous ones will end up with zero or
			negative duration; in that case, they should simply be ignored by subsequent 
			processing. On the other hand, if it's not yet time for this Sync and we're
			in a tuplet, to avoid problems with AddTuplets, add a rest to fill the gap. */
		
		for (prev = q-1; prev>=0; prev--) {
			prevEndTime = rawSyncTab[prev].time+rawSyncTab[prev].mult;
			overlapTime = prevEndTime-rawSyncTab[q].time;
			if (prev==q-1 && overlapTime<=-tupQuantum && tupQuantum!=quantum) {

				/* Add an NC before this one to fill the gap. */
	
				for (n = *pnRawSyncs-1; n>=q; n--)
					rawSyncTab[n+1] = rawSyncTab[n];
				(*pnRawSyncs)++;
				if (endTuplet>q) endTuplet++;
				rawSyncTab[q].link = REST_LINKVAL;
				rawSyncTab[q].time = prevEndTime;
				rawSyncTab[q].mult = -overlapTime;
			}
			else if (overlapTime>PDURUNIT) {
				newMult = rawSyncTab[prev].mult-overlapTime;
				rawSyncTab[prev].mult = (newMult<=0? 0 : newMult);
			}
		}
	}

	/* If the last Sync is within a tuplet and doesn't extend to the end of the tuplet,
	add a rest to extend it. */
	
	if (tupQuantum!=quantum) {
		prevEndTime = rawSyncTab[*pnRawSyncs-1].time+rawSyncTab[*pnRawSyncs-1].mult;
		if (prevEndTime<endTupTime) {
			rawSyncTab[*pnRawSyncs] = rawSyncTab[*pnRawSyncs-1];
			rawSyncTab[*pnRawSyncs].link = REST_LINKVAL;
			rawSyncTab[*pnRawSyncs].time = prevEndTime;
			rawSyncTab[*pnRawSyncs].mult = n_max(endTupTime-prevEndTime, tupQuantum);
			(*pnRawSyncs)++;
		}
	}
}


/* ----------------------------------------------------------- RemoveZeroDurItems -- */
/* Remove NRC's that have been truncated to zero duration. */

static void RemoveZeroDurItems(LINKTIMEINFO [], short *);
static void RemoveZeroDurItems(
					LINKTIMEINFO rawSyncTab[],		/* Input: semi-raw (partly-clarified) NCs */
					short *pnRawSyncs 				/* Input and output: size of rawSyncTab */
					)
{
	short q, nSkipped;
	
	nSkipped = 0;
	for (q = 0; q<*pnRawSyncs; q++) {
		
		if (rawSyncTab[q].mult<=0) {
			/* This NRC should be removed. Before doing so, make sure the following NRC
				isn't considered a piece of the preceding one. Also, if this one starts a
				tuplet, move the tuplet information to the following NRC. (The very last
				NRC can never be truncated, so there will always be a following one.) */
			
			nSkipped++;
			rawSyncTab[q+1].added = FALSE;
			
			if (rawSyncTab[q].tupleNum) {
				rawSyncTab[q+1].tupleNum = rawSyncTab[q].tupleNum;
				rawSyncTab[q+1].tupleDur = rawSyncTab[q].tupleDur;
				rawSyncTab[q+1].tupleTime = rawSyncTab[q].tupleTime;
			}
		}

		/* Move this NRC into the correct slot. */
			
		else
			rawSyncTab[q-nSkipped] = rawSyncTab[q];
	}

	*pnRawSyncs -= nSkipped;
 }


/* -------------------------------------------------------- RemoveOneItemTuplets -- */
/* If any of our proposed tuplets would end up containing only one NRC, cancel
making the tuplet. */

static void RemoveOneItemTuplets(LINKTIMEINFO [], short);
static void RemoveOneItemTuplets(
					LINKTIMEINFO rawSyncTab[],		/* Input: semi-raw (partly-clarified) NCs */
					short nRawSyncs 					/* Current size of rawSyncTab */
					)
{
	short q; long endTupTime;
	
	for (q = 0; q<nRawSyncs; q++) {
		if (rawSyncTab[q].tupleNum>0) {
			endTupTime = rawSyncTab[q].time+rawSyncTab[q].tupleDur;
			
			/* If this is the last thing in the table, or if the next one starts at
				or after the tuplet's end time, there's only one NRC in the tuplet. */
			
			if (q==nRawSyncs-1 || rawSyncTab[q+1].time>=endTupTime)
				rawSyncTab[q].tupleNum = 0;
		}
	}
}


/* ------------------------------------------------------------------ BuildSyncs -- */
/* Actually add valid Syncs, with note (and rest) subobjects, as described by
rawSyncTab[] and rawNoteAux[]. */

LINK BuildSyncs(
			Document *doc,
			short voice,					/* Same as the staff, since we use only default voices */
			LINKTIMEINFO rawSyncTab[],	/* Input: raw (unquantized and unclarified) NCs */
			short nRawSyncs,				/* Size of rawSyncTab */
			NOTEAUX rawNoteAux[],		/* Input: raw notes */
			short /*nAux*/,						/* (unused) Size of rawNoteAux */
			LINK measL 						/* Measure into which to put the new Syncs */
			)
{
	LINK firstL, lSync, aNoteL, firstSync; MNOTE theNote;
	short q; unsigned short iAux; Boolean okay=FALSE;
	
	/*
	 *	Put objects generated by this track after the given Measure so they can be
	 *	merged later.
	 */
	firstL = RightLINK(measL);
	doc->selStartL = doc->selEndL = firstL;
	
	firstSync = NILINK;

	for (q = 0; q<nRawSyncs; q++) {		
		lSync = NILINK;

		if (rawSyncTab[q].link==REST_LINKVAL) {
			/* Create a new Sync for the rest. Most of the <theNote> fields will
				be overridden, so set them to some innocuous values. */
			
			theNote.noteNumber = 0;
			theNote.onVelocity = config.feedbackNoteOnVel;
			theNote.offVelocity = config.noteOffVel;
			theNote.duration = 15;												/* ??Does this matter? */
			
			aNoteL = CreateSync(doc, theNote, &lSync, voice, 			/* staffn = voice */
										UNKNOWN_L_DUR, 0,							/* placeholders */
										voice, TRUE, 0);
			if (!firstSync) firstSync = lSync;
			rawSyncTab[q].link = lSync;
			if (!aNoteL) goto Done;
			continue;
		}

		/*
		 *	We can't rely on the next rawSyncTab[].link to say where this one's notes
		 *	end because, if the next one was <added>, its .link will be identical to
		 *	this one! Hence we use a wierd scheme where the NOTEAUX has a .first field
		 *	which is TRUE iff it starts a new "chord", and leading to the following
		 *	wierd <for> statement.
		 */ 
		iAux = rawSyncTab[q].link;												/* really aux index! */
		for ( ; iAux==rawSyncTab[q].link || !rawNoteAux[iAux].first; iAux++) {
			/*
			 * If the chord already contains a note with this one's note number,
			 * skip this one. Important because Nightingale doesn't handle unisons
			 *	well, but probably a good idea regardless. (This only avoids
			 * perfect unisons: augmented unisons are still a problem.)
			 */
			if (lSync)
				if (FindNoteNum(lSync, voice, rawNoteAux[iAux].noteNumber)!=NILINK)
					break;
			
			/* Now add the note to the existing Sync or to a new one. */
			
			theNote.noteNumber = rawNoteAux[iAux].noteNumber;
			theNote.onVelocity = rawNoteAux[iAux].onVelocity;
			theNote.offVelocity = rawNoteAux[iAux].offVelocity;
			theNote.duration = rawNoteAux[iAux].duration;
			
			if (lSync)
				aNoteL = AddNoteToSync(doc, theNote, lSync, voice,		/* staffn = voice */
											UNKNOWN_L_DUR, 0,						/* placeholders */
											voice, FALSE, 0);
			else {
				aNoteL = CreateSync(doc, theNote, &lSync, voice, 		/* staffn = voice */
											UNKNOWN_L_DUR, 0,						/* placeholders */
											voice, FALSE, 0);
				if (!firstSync) firstSync = lSync;
				rawSyncTab[q].link = lSync;
			}
			if (!aNoteL) goto Done;
		}		
	}
	
	okay = TRUE;

Done:
	if (okay) return firstSync;
	return NILINK;
}


/* -------------------------------------------------------- Clarify below measure -- */

#define SET_PLAYDUR FALSE	/* If setting playDurs is wanted, should be done later */

/* Given a note/rest/chord and a list <piece> of start times and durations for
segments we want to divide it into, divide the note/rest/chord (henceforth, "NRC")
up, putting each segment after the first into a brand-new Sync. If the NRC is
tied to the right, also updates the Slur's firstSyncL to point to the last segment;
in this case, assumes there are no slur-subtype Slurs in the voice, only tie-subtype.
Similar to ClarifyFromList, except that function considers merging the new segments
with existing Syncs and does not return <newSyncs> and <nNewSyncs>. */

Boolean MFClarifyFromList(
			Document *doc,
			LINK		syncL,
			short		voice,
			short		tieStaff,		/* Staff no. for ties, or <=0 for no ties (use with rests) */
			NOTEPIECE piece[],
			short		kount,
			PCONTEXT pContext,
			LINKTIMEINFO newSyncs[],
			short 	*nNewSyncs
			)
{
	register LINK newL; LINK qL, prevL, aNoteL, tieL;
	short k; long curTime;
	
	/*
	 *	Strategy: set the existing NRC's duration to the first segment's, and add new
	 *	NRCs for the other segments if there are more. Put each of the new NRCs into a
	 *	new Sync.
	 */
	curTime = newSyncs[*nNewSyncs].time;
	
	SetNRCDur(doc, syncL, voice, piece[1].lDur, piece[1].nDots, pContext, SET_PLAYDUR);
	doc->changed = TRUE;

	if (kount<2) return TRUE;
	
	aNoteL = FindMainNote(syncL, voice);
	if (NoteTIEDR(aNoteL)) tieL = LVSearch(syncL, SLURtype, voice, GO_LEFT, FALSE);
	else						  tieL = NILINK; 
	prevL = syncL;

	for (qL = syncL, k = 2; k<=kount; k++) {
		newL = DuplicNC(doc, qL, voice);
		if (!newL) return FALSE;
		
		InsNodeIntoSlot(newL, RightLINK(prevL));

		SetNRCDur(doc, newL, voice, piece[k].lDur, piece[k].nDots, pContext, SET_PLAYDUR);
		SyncTIME(newL) = SyncTIME(prevL)+Code2LDur(piece[k-1].lDur, piece[k-1].nDots);
		LinkSEL(newL) = TRUE;
		if (tieStaff>0)
			if (!AddNoteTies(doc, qL, newL, tieStaff, voice)) return FALSE;
		ClearAccAndMods(doc, newL, voice);
		qL = newL;
		(*nNewSyncs)++;
		newSyncs[*nNewSyncs].link = newL;
		curTime += Code2LDur(piece[k-1].lDur, piece[k-1].nDots);
		newSyncs[*nNewSyncs].time = curTime;
		newSyncs[*nNewSyncs].tupleNum = 0;
		prevL = newL;
	}
	
	if (tieL) {
		MoveNode(tieL, prevL);
		SlurFIRSTSYNC(tieL) = prevL;
	}

	return TRUE;
}


#define MAX_ERR (PDURUNIT-1)
#define MAX_DOTS 2	/* Max. aug. dots allowed: be generous, since we have no alternative */

Boolean MFSetNRCDur(LINK syncL, short voice, unsigned short lDur, short tupleNum,
							short tupleDenom, Boolean setPDur)
{
	char durCode, nDots; LINK aNoteL; PANOTE aNote;
	
	if (LDur2Code((short)lDur, MAX_ERR, MAX_DOTS, &durCode, &nDots)) {
		for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==voice) {
				aNote = GetPANOTE(aNoteL);
				aNote->subType = durCode;
				aNote->ndots = nDots;
				if (setPDur) aNote->playDur = (tupleDenom*lDur)/tupleNum;
			}
		return TRUE;
	}
	return FALSE;
}


/* Set the given note/rest/chord to duration <lDur> and "clarify" the result, i.e.,
if necessary, divide it into a series of tied notes/rests/chords for readability.
If we succeed, return TRUE, else FALSE. Like SetAndClarifyDur, except that function
gets startTime, numerator, and denominator from context and does not have <newSyncTab>
and <nNewSyncs> parameters. */

static Boolean Clar1BelowMeas(
						Document *doc,
						short voice,
						LINKTIMEINFO rawSyncInfo,
						short tupleNum,
						MEASINFO	measInfoTab[],
						short measTabLen,
						short *m,							/* Index into measInfoTab; -1 = initialize */
						LINKTIMEINFO newSyncTab[],
						unsigned short maxNewSyncs,
						short *nNewSyncs
						)
{
	LINK syncL, aNoteL;
	CONTEXT context;
	short tupleDenom, staff, nPieces;
	Boolean compound;
	NOTEPIECE piece[MF_MAXPIECES];
	unsigned short lDur;
	long timeUsed, timeUsedInMeas;
	static long measDur, tsStartTime, tsEndTime;
	char fmtStr[256];
	
	syncL = rawSyncInfo.link;
	if (!syncL) {
		MayErrMsg("Clar1BelowMeas: syncL=0 at time %ld", rawSyncInfo.time);
		return FALSE;
	}
	aNoteL = FindMainNote(syncL, voice);
	/* Convert play duration ticks to logical, not the other way around! */
	if (tupleNum>0) {
		tupleDenom = MF_TupleDenom(tupleNum);
		lDur = (tupleNum*rawSyncInfo.mult)/tupleDenom;
	}
	else
		lDur = rawSyncInfo.mult;

	(*nNewSyncs)++;
	newSyncTab[*nNewSyncs] = rawSyncInfo;
#ifdef PROBABLY_UNNEEDED
	round = quantum/2;
	newSyncTab[*nNewSyncs].mult = (newSyncTab[*nNewSyncs].mult+round)/quantum;
#endif

	if (*m<0) tsEndTime = 0L;
	timeUsed = newSyncTab[*nNewSyncs].time;

	*m = FindTimeSig(timeUsed, *m, measInfoTab, measTabLen, &tsStartTime,
								&tsEndTime, &measDur);

	timeUsedInMeas = (timeUsed-tsStartTime) % measDur;

	compound = COMPOUND(measInfoTab[*m].numerator);

	/* If this note is in a tuplet, MakeClarifyList can easily fail. To fake it out, we'd
	need to change time sig. as well as note times, e.g., in 4/4, with the 2nd triplet
	8th of the 2nd quarter of measure, set time sig.=12/8, timeUsedInMeas=960 (not 640),
	lDur=240 (not 160). But I'm still not sure it'll work--there may be problems because
	it's now a compound meter, and what if the meter is REALLY compound? For now, do
	something very simple for tupleted notes, which should be sufficient as long as the
	only tuplets we allow are triplets. */

	if (tupleNum>0) {
		if (!MFSetNRCDur(syncL, voice, lDur, tupleNum, tupleDenom, SET_PLAYDUR))		
			MayErrMsg("Clar1BelowMeas: can't find duration for notes in tuplet at %ld. dur=%ld",
						(long)syncL, (long)lDur);

		return TRUE;
	}

	if (!MakeClarifyList(timeUsedInMeas, lDur, measInfoTab[*m].numerator,
								measInfoTab[*m].denominator, compound, piece,
								MF_MAXPIECES, &nPieces)) return FALSE;
	if (*nNewSyncs+nPieces>=maxNewSyncs) {
		GetIndCString(fmtStr, RHYTHMERRS_STRS, 6);    /* "After clarifying rhythm, there are over %d Syncs, which is more than Nightingale expected." */
		sprintf(strBuf, fmtStr, maxNewSyncs); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	/*
	 *	<piece> now contains a list of start times and durations for the segments we want
	 *	to divide our note/rest/chord into. Divide it up.
	 */
	staff = NoteSTAFF(aNoteL);
	GetContext(doc, syncL, staff, &context);		/* NB: time sig. is invalid */
	
	MFClarifyFromList(doc, syncL, voice, (NoteREST(aNoteL)? 0 : staff), piece, nPieces,
							&context, newSyncTab, nNewSyncs);
	doc->changed = TRUE;

	return TRUE;
}


/* Rhythmically clarify all the notes/rests/chords described by rawSyncTab[0->nRawSyncs],
given the information on time signatures in measInfoTab[0->measTabLen-1]. We both put
information on the clarified NRCs into newSyncTab[], and directly affect the score
object list. */

Boolean ClarAllBelowMeas(
				Document *doc,
				short voice,
				LINKTIMEINFO rawSyncTab[],		/* Input: partly-clarified NRCs */
				short nRawSyncs,
				MEASINFO	measInfoTab[],
				short measTabLen,
				LINKTIMEINFO newSyncTab[],		/* Output: fully-clarified NRCs */
				unsigned short maxNewSyncs,
				short *nNewSyncs
				)
{
	short m, q, endTuplet, tupleNum;
	
	m = -1;
	*nNewSyncs = -1;

	endTuplet = 0;
	for (q = 0; q<nRawSyncs; q++) {
		/* This may be the end of one tuplet and/or the beginning of another. */
		
		if (q==endTuplet)
			tupleNum = 0;
		if (rawSyncTab[q].tupleNum>0) {
			tupleNum = rawSyncTab[q].tupleNum;
			endTuplet = q+GetNInTuple(rawSyncTab, nRawSyncs, q, 0);
		}

		/* Ignore note segments generated by ClarAllToMeas that are overlaid by
			following notes. See comment in QuantizeAndClip. */
			
		if (rawSyncTab[q].mult<=0) continue;
		if (rawSyncTab[q].link<=0) continue;
		
		/* Clar1BelowMeas clarifies the rhythm by dividing the note/chord into two
			or more pieces, storing info about the pieces directly into <newSyncTab>
			as well as the object list, and increasing <*nNewSyncs> accordingly. */
		
		if (!Clar1BelowMeas(doc, voice, rawSyncTab[q], tupleNum, measInfoTab, measTabLen,
									&m, newSyncTab, maxNewSyncs, nNewSyncs))
			return FALSE;
	}

	(*nNewSyncs)++;												/* Convert from index to count */
	return TRUE;
}


/* ---------------------------------------------------------- AddTies, AddTuplets -- */

Boolean AddTies(
				Document *doc,
				short voice,
				LINKTIMEINFO rawSyncTab[],		/* Input: semi-raw (partly-clarified) NCs */
				short nRawSyncs 					/* Current size of rawSyncTab */
				)
{
	short q;
	
	for (q = 1; q<nRawSyncs; q++) {
		/* Either note/chord may have been truncated to zero duration. */
		
		if (rawSyncTab[q-1].mult>0 && rawSyncTab[q].mult>0)
			if (rawSyncTab[q].added) {
				if (!AddNoteTies(doc, rawSyncTab[q-1].link, rawSyncTab[q].link, voice, voice))
					return FALSE;
			}
	}
	
	return TRUE;
 }


/* Add a tuplet to the object list. Cf. CreateTUPLET: this is simpler mostly because
we assume that everything in the object list with a voice is in <voice>.

This sets all brackets to visible; to get "conventional" visibility, after beaming,
you should call GetBracketVis for each tuplet. */

static Boolean Add1Tuplet(
						Document *doc,
						short voice,
						LINKTIMEINFO newSyncTab[],		/* Fully-clarified NCs */
						short nNewSyncs,					/* Current size of newSyncTab */
						short start 						/* Index in newSyncTab of first element */
						)
{
	LINK tupletL, pL, aNoteL, aNoteTupleL;
	short nInTuple, tupleNum, tupleDenom, n;
	Boolean numVis, denomVis, brackVis;
	PANOTETUPLE aNoteTuple;

	nInTuple = GetNInTuple(newSyncTab, nNewSyncs, start, 0);

	tupletL = InsertNode(doc, newSyncTab[start].link, TUPLETtype, nInTuple);
	if (!tupletL) { NoMoreMemory(); return NILINK; }

	tupleNum = newSyncTab[start].tupleNum;
	tupleDenom = MF_TupleDenom(tupleNum);
	numVis = TRUE;
	denomVis = FALSE;
	brackVis = TRUE;					/* Nothing may be beamed yet, so don't try to be fancy */

	SetObject(tupletL, 0, 0, FALSE, TRUE, FALSE);
	InitTuplet(tupletL, voice, voice, tupleNum, tupleDenom, numVis, denomVis, brackVis);
	LinkSEL(tupletL) = FALSE;

	for (n = 0; n<nInTuple; n++) {
		pL = newSyncTab[start+n].link;
		for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
			NoteINTUPLET(aNoteL) = TRUE;
	}

	aNoteTupleL = FirstSubLINK(tupletL);
	for (n = 0; n<nInTuple; 
			n++, aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
		aNoteTuple = GetPANOTETUPLE(aNoteTupleL);			/* Ptr to note info in TUPLET */
		aNoteTuple->tpSync = newSyncTab[start+n].link;
	}	

	SetTupletYPos(doc, tupletL);
			
	return tupletL;
}


Boolean AddTuplets(
				Document *doc,
				short voice,
				LINKTIMEINFO newSyncTab[],		/* Fully-clarified NCs */
				short nNewSyncs 					/* Current size of newSyncTab */
				)
{
	short q;
	
	for (q = 0; q<nNewSyncs; q++) {		
		if (newSyncTab[q].tupleNum>0)
			if (!Add1Tuplet(doc, voice, newSyncTab, nNewSyncs, q))
				return FALSE;
	}
	
	return TRUE;
}


/* -------------------------------------------------------------------- Debugging -- */


void DPrintMeasTab(char *label, MEASINFO	measInfoTab[], short measTabLen)
{
#ifndef PUBLIC_VERSION
	short m, nInTimeSig; long tsStartTime, tsEndTime, measDur;

	DebugPrintf("DPrintMeasTab '%s' measTabLen=%d:\n", label, measTabLen);

	tsEndTime = 0L;
	for (m = 0; m<measTabLen; m++) {
		CHECK_TS(m);
		tsStartTime = tsEndTime;
		measDur = TimeSigDur(0, measInfoTab[m].numerator, measInfoTab[m].denominator);
		tsEndTime += measDur*measInfoTab[m].count;
		nInTimeSig = (tsEndTime-tsStartTime)/measDur;
		if (nInTimeSig*measDur<(tsEndTime-tsStartTime)) nInTimeSig++;			/* Round up */
		DebugPrintf("  [%d] start=%ld end=%ld: %d bars of %d/%d\n", m, tsStartTime,
						tsEndTime, nInTimeSig,
						measInfoTab[m].numerator, measInfoTab[m].denominator);
	}
#endif
}

#define PR_LIMIT 16

void DPrintLTIs(char *label, LINKTIMEINFO itemTab[], short nItems)
{
#ifndef PUBLIC_VERSION
	short i;
	
	DebugPrintf("DPrintLTIs '%s' nItems=%d:\n", label, nItems);
	
	for (i = 0; i<nItems && i<(CapsLockKeyDown()? SHRT_MAX : PR_LIMIT); i++) {
		DebugPrintf("  [%d] link=%u time=%ld mult=%u %s",
							i, itemTab[i].link, itemTab[i].time, itemTab[i].mult,
							(itemTab[i].added? "added" : "")); 
		if (itemTab[i].tupleNum)
			DebugPrintf(" tupleNum=%d Time=%u Dur=%d", itemTab[i].tupleNum,
								itemTab[i].tupleTime, itemTab[i].tupleDur);
		DebugPrintf("\n");
	}
#endif
}

#define DBG (ShiftKeyDown() && OptionKeyDown())

/* -------------------------------------------------------------- TranscribeVoice -- */
/* The top-level rhythm clarification and quantization function: starting with tables
describing raw note/chords (no rests, please!) with durations in ticks, it goes all the
way to an object list of either unquantized "unknown duration" notes/chords ("NC"s) or
of rhythmically-readable, deflammed notes/chords with legal CMN durations, ties, and
tuplets. Generates no rests; after this, call FillNonemptyMeas to fill the gaps within
measures that have notes in them and/or FillEmptyMeas to fill measures that don't have
any notes. Assumes that all notes are in their staves' default voices.

IMPORTANT: the above description, and comments throughout this file, are misleading in
one way: we actually put EVERYTHING, no matter how much there is, into ONE SINGLE
MEASURE of the object list, namely Measure <measL>. However, the notes/chords are--where
necessary--broken into tied pairs according to the time signatures, so the contents of
<measL> can easily be merged into any number of other Measures. Note that the number of
objects in <measL> can greatly exceed MAX_MEASNODES, so a lot of Nightingale's machinery
can't be used after calling TranscribeVoice until Measures are added.

Even if we are quantizing (the usual case), the play durations of the notes are copied
from <rawNoteAux> unaltered. In fact, even the new attack times are only implied by
their Sync membership: the calling function should call FixTimeStamps afterwards to
set them.

If TranscribeVoice succeeds, it returns the LINK of the first Sync generated, else it
returns NILINK. */

LINK TranscribeVoice(
			Document *doc,
			short voice,						/* Same as the staff, since we use only default voices */
			LINKTIMEINFO rawSyncTab[],		/* Input: raw (unquantized and unclarified) NCs */
			short maxRawSyncs,				/* Maximum size of rawSyncTab */
			short nRawSyncs,					/* Current size of rawSyncTab */
			NOTEAUX rawNoteAux[],			/* Input: auxiliary info on raw notes */
			short nAux,							/* Size of rawNoteAux */
			short quantum,						/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
			short tripletBias,				/* Percent bias towards quant. to triplets >= 2/3 durQuantum */
			MEASINFO	measInfoTab[],
			short nMeasTab,
			LINK measL,							/* Measure into which to put the new Syncs */
			LINKTIMEINFO newSyncTab[],		/* Output: fully-clarified NCs */
			unsigned short maxNewSyncs,
			short *pnNewSyncs,
			short *errCode
			)
{
	LINK firstSync; short q;
	
	if (quantum>1) {
if (DBG) DPrintLTIs("<ClarAllToMeas",rawSyncTab,nRawSyncs);
		if (!ClarAllToMeas(doc,rawSyncTab,maxRawSyncs,&nRawSyncs,quantum,measInfoTab,nMeasTab))
			{ *errCode = 1; return NILINK; }
			
		if (!ChooseAllTuplets(doc,rawSyncTab,maxRawSyncs,nRawSyncs,quantum,tripletBias,
										measInfoTab,nMeasTab))
			{ *errCode = 2; return NILINK; }
								
if (DBG) DPrintLTIs("<ChooseChords",rawSyncTab,nRawSyncs);
		if (!ChooseChords(doc,voice,rawSyncTab,maxRawSyncs,&nRawSyncs,rawNoteAux,quantum))
			{ *errCode = 3; return NILINK; }
	
	
if (DBG) DPrintLTIs("<ClarAllToTuplets",rawSyncTab,nRawSyncs);
		if (!ClarAllToTuplets(doc,voice,rawSyncTab,maxRawSyncs,&nRawSyncs,quantum))
			{ *errCode = 4; return NILINK; }
	
if (DBG) DPrintLTIs("<QuantizeAndClip",rawSyncTab,nRawSyncs);
		QuantizeAndClip(doc,voice,rawSyncTab,&nRawSyncs,quantum);
		
		RemoveZeroDurItems(rawSyncTab,&nRawSyncs);
		RemoveOneItemTuplets(rawSyncTab,nRawSyncs);
	}
	
if (DBG) DPrintLTIs("<BuildSyncs",rawSyncTab,nRawSyncs);
	firstSync =	BuildSyncs(doc,voice,rawSyncTab,nRawSyncs,rawNoteAux,nAux,measL);
	if (!firstSync)
			{ *errCode = 5; return NILINK; }
	
	if (quantum>1) {
		if (!AddTies(doc,voice,rawSyncTab,nRawSyncs))
			{ *errCode = 6; return NILINK; }

		if (!ClarAllBelowMeas(doc,voice,rawSyncTab,nRawSyncs,measInfoTab,nMeasTab,
										newSyncTab,maxNewSyncs,pnNewSyncs))
			{ *errCode = 7; return NILINK; }

if (DBG) DPrintLTIs("<AddTuplets",newSyncTab,*pnNewSyncs);
		if (!AddTuplets(doc,voice,newSyncTab,*pnNewSyncs))
			{ *errCode = 8; return NILINK; }
	}
	else {
		/* No quantization desired: just make <newSyncTab> the same as <rawSyncTab>. */
		
		for (q = 0; q<nRawSyncs; q++)
			newSyncTab[q] = rawSyncTab[q];
		*pnNewSyncs = nRawSyncs;
	}

	return firstSync;
}
