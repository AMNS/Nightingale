/* File Debug2Utils.c - "debugging" functions for the musical content: Does the score
obey musical and music-notation constraints?

	DCheckPlayDurs			DCheckTempi				DCheckRedundantKS
	DCheckExtraTS			DCheckCautionaryTS		DCheckMeasDur
	DCheckUnisons			DBadNoteNum				DCheckNoteNums
*/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "DebugUtils.h"

#define DDB


/* ------------------------------------------------------------- DCheckVoiceTable -- */
/* Check the voice-mapping table and its relationship to the object list:
		that the voice table contains no empty slots;
		that every default voice belongs to the correct part;
		that the voice no. of every note, rest, and grace note appears in the voice table;
		agreement between voice table part numbers and note, rest, and grace note part
			numbers;
		the legality of voiceRole fields.
We could also check other symbols with voice nos. */

/* ----------------------------------------------------------------- DCheckPlayDurs -- */
/* Check that playDurs of notes appear reasonable for their logical durations. */

Boolean DCheckPlayDurs(
				Document *doc,
				Boolean maxCheck		/* FALSE=skip less important checks */
				)
{
	register LINK pL, aNoteL;
	PANOTE aNote; PTUPLET pTuplet;
	register Boolean bad;
	short v, shortDurThresh, tupletNum[MAXVOICES+1], tupletDenom[MAXVOICES+1];
	long lDur;

	bad = FALSE;

	shortDurThresh = PDURUNIT/2;
	for (v = 0; v<=MAXVOICES; v++)
		tupletNum[v] = 0;

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		switch (ObjLType(pL)) {
			case TUPLETtype:			 	
				v = TupletVOICE(pL);
				pTuplet = GetPTUPLET(pL);
				tupletNum[v] = pTuplet->accNum;
				tupletDenom[v] = pTuplet->accDenom;
				break;
				
			case SYNCtype:
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->subType!=UNKNOWN_L_DUR && aNote->subType>WHOLEMR_L_DUR) {
						if (aNote->playDur<shortDurThresh) {
							COMPLAIN2("DCheckPlayDurs: NOTE IN VOICE %d IN SYNC AT %u HAS EXTREMELY SHORT playDur.\n",
											aNote->voice, pL);
						}
						else {
							lDur = SimpleLDur(aNoteL);
							if (aNote->inTuplet) {
								v = aNote->voice;
								lDur = (lDur*tupletDenom[v])/tupletNum[v];
							}
							if (aNote->playDur>lDur)
								COMPLAIN3("DCheckPlayDurs: NOTE IN VOICE %d IN SYNC AT %u playDur IS LONGER THAN FULL DUR. OF %d\n",
											aNote->voice, pL, lDur);
						}
					}
				}
				break;
				
			default:
				;
		}
	}
	
	return bad;
}


/* ------------------------------------------------------------------- DCheckTempi -- */
/* If there are multiple Tempo objects at the same point in the score, check that their
tempo and/or M.M. strings are identical. This is on the assumption that the reason for
multiple Tempos at the same point is just to make the tempo and/or M.M. appear at more
than one vertical point, presumably in a score with many staves. In any case, as of v.
5.7, Nightingale doesn't support independent tempi on different staves, so different
M.M.s really are a problem. NB: A Tempo object may have no tempo component (indicated
by an empty tempo string), or no M.M. component (indicated by its _noMM_ flags). Check
the tempo string only if both objects being compared at a given time have a tempo
component, and likewise for M.M. strings. */

Boolean DCheckTempi(Document *doc)
{
	LINK pL, prevTempoL;
	Boolean bad, havePrevTempo, havePrevMetro;
	StringOffset theStrOffset;
	char tempoStr[256], metroStr[256], prevTempoStr[256], prevMetroStr[256];
		
	bad = FALSE;
	havePrevTempo = havePrevMetro = FALSE;
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;
		
		/* If we find a Sync, we're no longer "at the same point in the score". */
		if (SyncTYPE(pL)) havePrevTempo = havePrevMetro = FALSE;
		
		if (TempoTYPE(pL)) {
			if (havePrevTempo) {
				theStrOffset = TempoSTRING(pL);
				Pstrcpy((StringPtr)tempoStr, (StringPtr)PCopy(theStrOffset));
				PtoCstr((StringPtr)tempoStr);
//LogPrintf(LOG_DEBUG, "DCheckTempi: prevTempoL=%u '%s' pL=%u '%s'\n", prevTempoL, prevTempoStr,
//					pL, tempoStr);
				/* If this Tempo object has a tempo string, is it the expected string? */
				if (strlen(tempoStr)>0 && strcmp(prevTempoStr, tempoStr)!=0)
					COMPLAIN3("DCheckTempi: Tempo marks at %u ('%s') AND %u ARE INCONSISTENT.\n",
								prevTempoL, prevTempoStr, pL);
			}
			
			if (havePrevMetro) {
				theStrOffset = TempoMETROSTR(pL);
				Pstrcpy((StringPtr)metroStr, (StringPtr)PCopy(theStrOffset));
				PtoCstr((StringPtr)metroStr);
//LogPrintf(LOG_DEBUG, "DCheckTempi: prevTempoL=%u '%s' pL=%u '%s'\n", prevTempoL, prevMetroStr,
//					pL, metroStr);
				/* If this Tempo object has an M.M., is it the expected M.M. string? */
				if (!TempoNOMM(pL) && strcmp(prevMetroStr, metroStr)!=0)
					COMPLAIN3("DCheckTempi: M.M.s of Tempo objects at %u ('%s') AND %u ARE INCONSISTENT.\n",
								prevTempoL, prevMetroStr, pL);
			}
			else {
				theStrOffset = TempoSTRING(pL);
				Pstrcpy((StringPtr)prevTempoStr, (StringPtr)PCopy(theStrOffset));
				PtoCstr((StringPtr)prevTempoStr);
				theStrOffset = TempoMETROSTR(pL);
				Pstrcpy((StringPtr)prevMetroStr, (StringPtr)PCopy(theStrOffset));
				PtoCstr((StringPtr)prevMetroStr);
				if (strlen(tempoStr)>0) havePrevTempo = TRUE;
				if (!TempoNOMM(pL)) havePrevMetro = TRUE;
				prevTempoL = pL;
			}
		}
	}
	
	return bad;
}




/* ------------------------------------------------------------- DCheckRedundantKS -- */
/* Check for redundant key signatures, "redundant" in the sense that the key sig. is
a cancellation and the last key signature on the same staff was a cancel. */

Boolean DCheckRedundantKS(Document *doc)
{
	LINK pL, aKSL; PAKEYSIG aKeySig;
	Boolean bad, firstKS;
	short lastKSNAcc[MAXSTAVES+1], s, nRedundant;
	
	bad = FALSE;
	firstKS = TRUE;
		
	for (s = 1; s<=doc->nstaves; s++)
		lastKSNAcc[s] = 99;

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		switch (ObjLType(pL)) {
			case KEYSIGtype:
				/*
				 * The score's initial keysig is meaningful even though it's not inMeasure.
				 * All other non-inMeasure key sigs. are just repeating the context.
				 */
				if (!firstKS && !KeySigINMEAS(pL)) break;
				firstKS = FALSE;
				nRedundant = 0;
				for (aKSL = FirstSubLINK(pL); aKSL; aKSL = NextKEYSIGL(aKSL)) {
					aKeySig = GetPAKEYSIG(aKSL);
					s = aKeySig->staffn;
					if (lastKSNAcc[s]==0 && aKeySig->nKSItems==0)
						nRedundant++;
				}
				if (nRedundant>0)
					COMPLAIN2("*DCheckRedundantKS: KEYSIG AT %u REPEATS CANCEL ON %d STAVES.\n",
									pL, nRedundant);

				for (aKSL = FirstSubLINK(pL); aKSL; aKSL = NextKEYSIGL(aKSL)) {
					aKeySig = GetPAKEYSIG(aKSL);
					s = aKeySig->staffn;
					lastKSNAcc[s] = aKeySig->nKSItems;
				}
				break;
			default:
				;
		}
	}
		
	return bad;
}

/* ---------------------------------------------------------------- DCheckExtraTS -- */
/* Check for extra time signatures, "extra" in the sense that there's already been
a time signature on the same staff in the same measure. */

Boolean DCheckExtraTS(Document *doc)
{
	LINK pL, aTSL;
	Boolean haveTS[MAXSTAVES+1], bad;
	short s, nRedundant;
	
	bad = FALSE;
		
	for (s = 1; s<=doc->nstaves; s++)
		haveTS[s] = FALSE;

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		switch (ObjLType(pL)) {
			case TIMESIGtype:
				nRedundant = 0;
				for (aTSL = FirstSubLINK(pL); aTSL; aTSL = NextTIMESIGL(aTSL))
					if (haveTS[TimeSigSTAFF(aTSL)])
						nRedundant++;
				if (nRedundant>0)
					COMPLAIN2("DCheckExtraTS: TIMESIG AT %u BUT MEASURE ALREADY HAS TIMESIG ON %d STAVES.\n",
									pL, nRedundant);

				for (aTSL = FirstSubLINK(pL); aTSL; aTSL = NextTIMESIGL(aTSL))
					haveTS[TimeSigSTAFF(aTSL)] = TRUE;
				break;
			case MEASUREtype:
				for (s = 1; s<=doc->nstaves; s++)
					haveTS[s] = FALSE;
				break;
			default:
				;
		}
	}
		
	return bad;
}


/* ----------------------------------------------------------- DCheckCautionaryTS -- */
/* Check that when a staff begins with a time signature, it's anticipated by the same
time signature appearing on that staff at the end of the previous system. */

Boolean DCheckCautionaryTS(Document *doc)
{
	LINK pL, aTSL, prevMeasL;
	Boolean haveEndSysTS[MAXSTAVES+1], inNewSys[MAXSTAVES+1], bad;
	short s, stf, numerator[MAXSTAVES+1], denominator[MAXSTAVES+1];
	
	bad = FALSE;
		
	for (s = 1; s<=doc->nstaves; s++) {
		haveEndSysTS[s] = FALSE;
	}

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		switch (ObjLType(pL)) {
			case TIMESIGtype:
				if (!TimeSigINMEAS(pL)) continue;					/* Skip context-setting timesigs */
				for (aTSL = FirstSubLINK(pL); aTSL; aTSL = NextTIMESIGL(aTSL)) {
					stf = TimeSigSTAFF(aTSL);
					if (inNewSys[stf]) {
					/* We have a time signature at the beginning of a system. See if
						there's an equivalent cautionary timesig on this staff at the
						end of the previous system. */
						if (!haveEndSysTS[stf]) {
							COMPLAIN2("DCheckCautionaryTS: TIMESIG AT START OF SYSTEM ON STAFF %d AT %u NOT ANTICIPATED.\n",
										stf, pL);
						}
						else if (TimeSigNUM(aTSL)!=numerator[stf] || TimeSigDENOM(aTSL)!=denominator[stf])
							COMPLAIN2("DCheckCautionaryTS: TIMESIG AT START OF SYSTEM ON STAFF %d AT %u DISAGREES WITH ANTICIPATING TIMESIG.\n",
										stf, pL);
					}
					haveEndSysTS[stf] = TRUE;
					numerator[stf] = TimeSigNUM(aTSL);
					denominator[stf] = TimeSigDENOM(aTSL);
				}
				break;
			case MEASUREtype:
				/* If this is the 1st (invisible barline) Measure of the system, ignore it */
				prevMeasL = LSSearch(LeftLINK(pL), MEASUREtype, 1, GO_LEFT, FALSE);
//				LogPrintf(LOG_DEBUG, "DCheckCautionaryTS: prevMeasL=%u SameSystem=%d haveEndSysTS[1]=%d\n",
//							prevMeasL, SameSystem(prevMeasL, pL), haveEndSysTS[1]);
				if (prevMeasL==NILINK || !SameSystem(prevMeasL, pL)) break;
				for (s = 1; s<=doc->nstaves; s++)
					haveEndSysTS[s] = FALSE;
				break;
			case SYSTEMtype:
				for (s = 1; s<=doc->nstaves; s++)
					inNewSys[s] = TRUE;
				break;
			case SYNCtype:
				for (s = 1; s<=doc->nstaves; s++)
					inNewSys[s] = FALSE;
				break;
			default:
				;
		}
	}
		
	return bad;
}


/* ---------------------------------------------------------------- DCheckMeasDur -- */
/* Check the actual duration of notes in every measure, considered as a whole,
against its time signature on every staff. N.B. I think this function makes more
assumptions about the consistency of the data structure than the other "DCheck"
functions, so it may be more dangerous to use when things are shaky. N.B.2. It's
quite stupid. It probably would be better to check only "rhythmically understood
measures" in which: 
	-There are no unknown duration notes
	-All time signatures denote the same total time
	-No time signature appears after the first Sync
	-Every Sync after the 1st has a common voice with the preceding Sync
*/

Boolean DCheckMeasDur(Document *doc)
{
	LINK pL, barTermL;
	long measDurFromTS, measDurActual;
	const char *adverb;
	Boolean bad;
	
	bad = FALSE;
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		if (MeasureTYPE(pL) && !FakeMeasure(doc, pL)) {
			barTermL = EndMeasSearch(doc, pL);
			if (barTermL) {
				measDurFromTS = GetTimeSigMeasDur(doc, barTermL);
				if (measDurFromTS<0) {
					COMPLAIN("DCheckMeasDur: MEAS AT %u TIME SIG. DURATION DIFFERENT ON DIFFERENT STAVES.\n",
									pL);
					continue;
				}
				measDurActual = GetMeasDur(doc, barTermL, ANYONE);
				if (measDurFromTS!=measDurActual) {
					adverb = (ABS(measDurFromTS-measDurActual)<PDURUNIT? " MINUTELY " : " ");
					COMPLAIN3("DCheckMeasDur: MEAS AT %u DURATION OF %ld%sDIFFERENT FROM TIME SIG.\n",
									pL, measDurActual, adverb);
				}
			}
		}
	}
		
	return bad;
}


/* ---------------------------------------------------------------- DCheckUnisons -- */
/* Check chords for unisons. They're not necessarily a problem, though Nightingale's
user interface doesn't handle them well, e.g., showing their selection status. */

Boolean DCheckUnisons(Document *doc)
{
	LINK pL; Boolean bad; short voice;
	
	if (unisonsOK) return FALSE;
	
	bad = FALSE;
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;
		
		if (SyncTYPE(pL))
			for (voice = 1; voice<=MAXVOICES; voice++)
				if (VOICE_MAYBE_USED(doc, voice))
					if (ChordHasUnison(pL, voice))
						COMPLAIN2("DCheckUnisons: SYNC AT %u CHORD IN VOICE %d CONTAINS UNISON.\n",
										pL, voice);
	}
	
	return FALSE;
}


/* ------------------------------------------------------------------ DBadNoteNum -- */
/* Given a Sync and a note in it, plus the current clef, octave sign, and accidental
table: return the discrepancy in the note's MIDI note number (0 if none). */

short DBadNoteNum(
			Document *doc,
			short clefType, short octType,
			SignedByte /*accTable*/[],
			LINK syncL, LINK theNoteL)
{
	short halfLn;							/* Relative to the top of the staff */
	SHORTQD yqpit;
	short midCHalfLn, effectiveAcc, noteNum;
	LINK firstSyncL, firstNoteL;

	midCHalfLn = ClefMiddleCHalfLn(clefType);					/* Get middle C staff pos. */		
	yqpit = NoteYQPIT(theNoteL)+halfLn2qd(midCHalfLn);
	halfLn = qd2halfLn(yqpit);									/* Number of half lines from stftop */

	if (!FirstTiedNote(syncL, theNoteL, &firstSyncL, &firstNoteL))
		return FALSE;											/* Object list is messed up */
	effectiveAcc = EffectiveAcc(doc, firstSyncL, firstNoteL);
	if (octType>0)
		noteNum = Pitch2MIDI(midCHalfLn-halfLn+noteOffset[octType-1],
												effectiveAcc);
	else
		noteNum = Pitch2MIDI(midCHalfLn-halfLn, effectiveAcc);
	
	return (noteNum-NoteNUM(theNoteL));
}


/* --------------------------------------------------------------- DCheckNoteNums -- */
/* Check whether the MIDI note numbers of Notes agree with their notation. Ignores
both grace notes and rests. */

Boolean DCheckNoteNums(Document *doc)
{
	Boolean bad=FALSE, haveAccs;
	short staff, clefType, octType[MAXSTAVES+1], useOctType, nnDiff;
	LINK pL, aNoteL, aClefL;
	SignedByte accTable[MAX_STAFFPOS];

	/*
	 * We keep track of where octave signs begin. But keeping track of where they end
	 * is a bit messy: to avoid doing that, we'll just rely on each Note subobj's
	 * inOttava flag to say whether it's in one.
	 */
	for (staff = 1; staff<=doc->nstaves; staff++)
		octType[staff] = 0;											/* Initially, no octave sign */

	for (staff = 1; staff<=doc->nstaves; staff++) {

		for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
			switch (ObjLType(pL)) {
	
				case SYNCtype:
					aNoteL = FirstSubLINK(pL);
					haveAccs = FALSE;
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteSTAFF(aNoteL)==staff && !NoteREST(aNoteL)) {
							if (!haveAccs) {
								GetAccTable(doc, accTable, pL, staff);
								haveAccs = TRUE;
							}

							useOctType = (NoteINOTTAVA(aNoteL)? octType[staff] : 0);
							nnDiff = DBadNoteNum(doc, clefType, useOctType, accTable, pL, aNoteL);
							if (nnDiff!=0) {
								if (abs(nnDiff)<=2) {
									COMPLAIN3("DCheckNode: NOTE IN SYNC AT %u STAFF %d noteNum %d AND NOTATION DISAGREE: ACCIDENTAL?\n",
													pL, staff, NoteNUM(aNoteL));
								}
								else {
									COMPLAIN3("DCheckNode: NOTE IN SYNC AT %u STAFF %d noteNum %d AND NOTATION DISAGREE.\n",
													pL, staff, NoteNUM(aNoteL));
								}
							}
						}
					break;
				case CLEFtype:
					aClefL = ClefOnStaff(pL, staff);
					if (aClefL) clefType = ClefType(aClefL);
					break;
				case OTTAVAtype:
					if (OttavaSTAFF(pL)==staff)
						octType[staff] = OctType(pL);
					break;
				default:
					;
			}
		
	}

	return bad;
}
