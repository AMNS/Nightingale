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
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "DebugUtils.h"

#define DDB


/* ------------------------------------------------------------------ DCheckVoiceTable -- */
/* Check the voice-mapping table and its relationship to the object list:
		that the voice table contains no empty slots;
		that every default voice belongs to the correct part;
		that the voice no. of every note, rest, and grace note appears in the voice table;
		agreement between voice table part numbers and note, rest, and grace note part
			numbers;
		the legality of voiceRole fields.
We could also check other symbols with voice nos. */

/* -------------------------------------------------------------------- DCheckPlayDurs -- */
/* Check that playDurs of notes appear reasonable for their logical durations. */

Boolean DCheckPlayDurs(
				Document *doc,
				Boolean maxCheck		/* False=skip less important checks */
				)
{
	register LINK pL, aNoteL;
	PANOTE aNote;  PTUPLET pTuplet;
	register Boolean bad;
	short v, shortDurThresh, tupletNum[MAXVOICES+1], tupletDenom[MAXVOICES+1];
	long lDur;

	bad = False;

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
							COMPLAIN3("DCheckPlayDurs: Note in Sync L%u voice %d in measure %d playDur is extremely short.\n",
											pL, aNote->voice, GetMeasNum(doc, pL));
						}
						else {
							lDur = SimpleLDur(aNoteL);
							if (aNote->inTuplet) {
								v = aNote->voice;
								lDur = (lDur*tupletDenom[v])/tupletNum[v];
							}
							if (aNote->playDur>lDur)
								COMPLAIN4("DCheckPlayDurs: Note in Sync L%u voice %d in measure %d playDur is longer than full dur. of %d\n",
											pL, aNote->voice, GetMeasNum(doc, pL), lDur);
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


/* ----------------------------------------------------------------------- DCheckTempi -- */
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
		
	bad = False;
	havePrevTempo = havePrevMetro = False;
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;
		
		/* If we found a Sync, we're no longer "at the same point in the score". */
		if (SyncTYPE(pL)) havePrevTempo = havePrevMetro = False;
		
		if (TempoTYPE(pL)) {
			if (havePrevTempo) {
				theStrOffset = TempoSTRING(pL);
				Pstrcpy((StringPtr)tempoStr, (StringPtr)PCopy(theStrOffset));
				PtoCstr((StringPtr)tempoStr);
//LogPrintf(LOG_DEBUG, "DCheckTempi: prevTempoL=%u '%s' pL=%u '%s'\n", prevTempoL, prevTempoStr,
//					pL, tempoStr);
				/* If this Tempo object has a tempo string, is it the expected string? */
				if (strlen(tempoStr)>0 && strcmp(prevTempoStr, tempoStr)!=0)
					COMPLAIN4("DCheckTempi: Tempo marks L%u ('%s') and L%u in measure %d are inconsistent.\n",
								prevTempoL, prevTempoStr, pL, GetMeasNum(doc, pL));
			}
			
			if (havePrevMetro) {
				theStrOffset = TempoMETROSTR(pL);
				Pstrcpy((StringPtr)metroStr, (StringPtr)PCopy(theStrOffset));
				PtoCstr((StringPtr)metroStr);
//LogPrintf(LOG_DEBUG, "DCheckTempi: prevTempoL=%u '%s' pL=%u '%s'\n", prevTempoL, prevMetroStr,
//					pL, metroStr);
				/* If this Tempo object has an M.M., is it the expected M.M. string? */
				if (!TempoNOMM(pL) && strcmp(prevMetroStr, metroStr)!=0)
					COMPLAIN4("DCheckTempi: M.M.s of Tempo objects L%u ('%s') and L%u in measure %d are inconsistent.\n",
								prevTempoL, prevMetroStr, pL, GetMeasNum(doc, pL));
			}
			else {
				theStrOffset = TempoSTRING(pL);
				Pstrcpy((StringPtr)prevTempoStr, (StringPtr)PCopy(theStrOffset));
				PtoCstr((StringPtr)prevTempoStr);
				theStrOffset = TempoMETROSTR(pL);
				Pstrcpy((StringPtr)prevMetroStr, (StringPtr)PCopy(theStrOffset));
				PtoCstr((StringPtr)prevMetroStr);
				if (strlen(tempoStr)>0) havePrevTempo = True;
				if (!TempoNOMM(pL)) havePrevMetro = True;
				prevTempoL = pL;
			}
		}
	}
	
	return bad;
}




/* ----------------------------------------------------------------- DCheckRedundantKS -- */
/* Check for redundant key signatures, "redundant" in the sense that the key sig. is
a cancellation and the last key signature on the same staff was a cancel. */

Boolean DCheckRedundantKS(Document *doc)
{
	LINK pL, aKSL;  PAKEYSIG aKeySig;
	Boolean bad, firstKS;
	short lastKSNAcc[MAXSTAVES+1], s, nRedundant;
	
	bad = False;
	firstKS = True;
		
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
				firstKS = False;
				nRedundant = 0;
				for (aKSL = FirstSubLINK(pL); aKSL; aKSL = NextKEYSIGL(aKSL)) {
					aKeySig = GetPAKEYSIG(aKSL);
					s = aKeySig->staffn;
					if (lastKSNAcc[s]==0 && aKeySig->nKSItems==0)
						nRedundant++;
				}
				if (nRedundant>0)
					COMPLAIN2("*DCheckRedundantKS: Keysig L%u repeats cancel on %d staves.\n",
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

/* --------------------------------------------------------------------- DCheckExtraTS -- */
/* Check for extra time signatures, "extra" in the sense that there's already been
a time signature on the same staff in the same measure. */

Boolean DCheckExtraTS(Document *doc)
{
	LINK pL, aTSL;
	Boolean haveTS[MAXSTAVES+1], bad;
	short s, nRedundant;
	
	bad = False;
		
	for (s = 1; s<=doc->nstaves; s++)
		haveTS[s] = False;

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		switch (ObjLType(pL)) {
			case TIMESIGtype:
				nRedundant = 0;
				for (aTSL = FirstSubLINK(pL); aTSL; aTSL = NextTIMESIGL(aTSL))
					if (haveTS[TimeSigSTAFF(aTSL)])
						nRedundant++;
				if (nRedundant>0)
					COMPLAIN3("DCheckExtraTS: L%u is a Timesig but Measure %d already has Timesig on %d staves.\n",
									pL, GetMeasNum(doc, pL), nRedundant);

				for (aTSL = FirstSubLINK(pL); aTSL; aTSL = NextTIMESIGL(aTSL))
					haveTS[TimeSigSTAFF(aTSL)] = True;
				break;
			case MEASUREtype:
				for (s = 1; s<=doc->nstaves; s++)
					haveTS[s] = False;
				break;
			default:
				;
		}
	}
		
	return bad;
}


/* ---------------------------------------------------------------- DCheckCautionaryTS -- */
/* Check that when a staff begins with a time signature, it's anticipated by the same
time signature appearing on that staff at the end of the previous system. */

Boolean DCheckCautionaryTS(Document *doc)
{
	LINK pL, aTSL, prevMeasL;
	Boolean haveEndSysTS[MAXSTAVES+1], inNewSys[MAXSTAVES+1], bad;
	short s, stf, numerator[MAXSTAVES+1], denominator[MAXSTAVES+1];
	
	bad = False;
		
	for (s = 1; s<=doc->nstaves; s++) {
		haveEndSysTS[s] = False;
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
							COMPLAIN3("DCheckCautionaryTS: Timesig at start of system in measure %d, staff %d (L%u) not anticipated.\n",
										GetMeasNum(doc, pL), stf, pL);
						}
						else if (TimeSigNUMER(aTSL)!=numerator[stf] || TimeSigDENOM(aTSL)!=denominator[stf])
							COMPLAIN3("DCheckCautionaryTS: Timesig at start of system in measure %d, staff %d (L%u) disagrees with anticipating timesig.\n",
										GetMeasNum(doc, pL), stf, pL);
					}
					haveEndSysTS[stf] = True;
					numerator[stf] = TimeSigNUMER(aTSL);
					denominator[stf] = TimeSigDENOM(aTSL);
				}
				break;
			case MEASUREtype:
				/* If this is the 1st (invisible barline) Measure of the system, ignore it */
				prevMeasL = LSSearch(LeftLINK(pL), MEASUREtype, 1, GO_LEFT, False);
				if (prevMeasL==NILINK || !SameSystem(prevMeasL, pL)) break;
				for (s = 1; s<=doc->nstaves; s++)
					haveEndSysTS[s] = False;
				break;
			case SYSTEMtype:
				for (s = 1; s<=doc->nstaves; s++)
					inNewSys[s] = True;
				break;
			case SYNCtype:
				for (s = 1; s<=doc->nstaves; s++)
					inNewSys[s] = False;
				break;
			default:
				;
		}
	}
		
	return bad;
}


/* --------------------------------------------------------------------- DCheckMeasDur -- */
/* Check the actual duration of notes in every measure, considered as a whole,
against its time signature on every staff. Also check the actual duration against
the time signature on each individual staff. NB: I think this function makes more
assumptions about the consistency of the data structure than the other "DCheck"
functions, so it may be more dangerous to use when things are shaky. NB2: It's
quite stupid. It probably would be better to check only "rhythmically understood
measures" in which: 
	-There are no unknown duration notes
	-All time signatures denote the same total time
	-No time signature appears after the first Sync
	-Every Sync after the 1st has a common voice with the preceding Sync
*/

Boolean DCheckMeasDur(Document *doc)
{
	LINK pL, barTermL, aMeasL;
	long measDurFromTS, measDurActual, measDurOnStaff;
	const char *adverb;
	short staffn;
	Boolean bad;
	
	bad = False;
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		if (MeasureTYPE(pL) && !IsFakeMeasure(doc, pL)) {
			barTermL = EndMeasSearch(doc, pL);
			if (barTermL==NILINK) continue;

			measDurFromTS = GetTimeSigMeasDur(doc, barTermL);
			if (measDurFromTS<0) {
				COMPLAIN2("DCheckMeasDur: Measure %d (L%u) time sig. duration is different on different staves.\n",
								GetMeasNum(doc, pL), pL);
				continue;
			}
			measDurActual = GetMeasDur(doc, barTermL, ANYONE);
			if (measDurFromTS!=measDurActual) {
				adverb = (ABS(measDurFromTS-measDurActual)<PDURUNIT? "minutely" : "");
				COMPLAIN4("DCheckMeasDur: Measure %d (L%u) duration of %ld is %s different from time sig.\n",
								GetMeasNum(doc, pL), pL, measDurActual, adverb);
			}

			/* Check measure duration on individual staves. */
			aMeasL = FirstSubLINK(pL);
			for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
				staffn = MeasureSTAFF(aMeasL);			
				measDurOnStaff = GetMeasDur(doc, barTermL, staffn);
				if (measDurOnStaff!=0 && ABS(measDurFromTS-measDurOnStaff)>=PDURUNIT) {
					COMPLAIN4("DCheckMeasDur: Measure %d (L%u), staff %d duration %d is different from time sig.\n",
								GetMeasNum(doc, pL), pL, staffn, measDurOnStaff);
				}
			}
		}
	}
		
	return bad;
}


/* --------------------------------------------------------------------- DCheckUnisons -- */
/* Check chords for unisons. They're not necessarily a problem, though Nightingale's
user interface doesn't handle them well, e.g., showing their selection status. */

Boolean DCheckUnisons(Document *doc)
{
	LINK pL;  Boolean bad;  short voice;
	
	if (unisonsOK) return False;		/* If they're not a problem now, skip checking! */
	
	bad = False;
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;
		
		if (SyncTYPE(pL))
			for (voice = 1; voice<=MAXVOICES; voice++)
				if (VOICE_MAYBE_USED(doc, voice))
					if (ChordHasUnison(pL, voice))
						COMPLAIN3("DCheckUnisons: In measure %d, chord (L%u) in voice %d contains a unison.\n",
										GetMeasNum(doc, pL), pL, voice);
	}
	
	return False;
}


/* ----------------------------------------------------------------------- DBadNoteNum -- */
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
		return False;											/* Object list is messed up */
	effectiveAcc = EffectiveAcc(doc, firstSyncL, firstNoteL);
	if (octType>0)
		noteNum = Pitch2MIDI(midCHalfLn-halfLn+noteOffset[octType-1],
												effectiveAcc);
	else
		noteNum = Pitch2MIDI(midCHalfLn-halfLn, effectiveAcc);
	
	return (noteNum-NoteNUM(theNoteL));
}


/* -------------------------------------------------------------------- DCheckNoteNums -- */
/* Check whether the MIDI note numbers of Notes agree with their notation. Ignores
both grace notes and rests. */

Boolean DCheckNoteNums(Document *doc)
{
	Boolean bad=False, haveAccs;
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
					haveAccs = False;
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteSTAFF(aNoteL)==staff && !NoteREST(aNoteL)) {
							if (!haveAccs) {
								GetAccTable(doc, accTable, pL, staff);
								haveAccs = True;
							}

							useOctType = (NoteINOTTAVA(aNoteL)? octType[staff] : 0);
							nnDiff = DBadNoteNum(doc, clefType, useOctType, accTable, pL, aNoteL);
							if (nnDiff!=0) {
								if (abs(nnDiff)<=2) {
									COMPLAIN4("DCheckNoteNums: Note in Sync L%u in measure %d, staff %d notenum %d and notation disagree: wrong accidental?\n",
													pL, GetMeasNum(doc, pL), staff, NoteNUM(aNoteL));
								}
								else {
									COMPLAIN4("DCheckNoteNums: Note in Sync L%u in measure %d, staff %d notenum %d and notation disagree.\n",
													pL, GetMeasNum(doc, pL), staff, NoteNUM(aNoteL));
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
