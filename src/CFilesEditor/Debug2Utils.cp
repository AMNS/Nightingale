/* File Debug2Utils.c - debugging functions. This is the other part of the 
"DebugUtils" source file, made into a separate file long ago because DebugUtils.c
was just too big for certain Mac development systems of the 1990's, which had a
limit of 32K per module object code.

	DCheckVoiceTable		DCheckTempi				DCheckRedundKS
	DCheckRedundTS			DCheckMeasDur			DCheckUnisons
	DCheck1NEntries			DCheckNEntries			DCheck1SubobjLinks	
	DBadNoteNum				DCheckNoteNums
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

#define ulong unsigned long

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

Boolean DCheckVoiceTable(Document *doc,
			Boolean fullCheck,				/* FALSE=skip less important checks */
			short *pnVoicesUsed)
{
	Boolean bad, foundEmptySlot;
	LINK pL, aNoteL, aGRNoteL;
	LINK voiceUseTab[MAXVOICES+1], partL;
	short v, stf, partn;
	PPARTINFO pPart;
	Boolean voiceInWrongPart;

	bad = FALSE;

	for (foundEmptySlot = FALSE, v = 1; v<=MAXVOICES; v++) {
		if (doc->voiceTab[v].partn==0) foundEmptySlot = TRUE;
		if (doc->voiceTab[v].partn!=0 && foundEmptySlot) {
				COMPLAIN("*DCheckVoiceTable: VOICE %ld IN TABLE FOLLOWS AN EMPTY SLOT.\n", (long)v);
				break;
		}
	}

	partL = NextPARTINFOL(FirstSubLINK(doc->headL));
	for (partn = 1; partL; partn++, partL=NextPARTINFOL(partL)) {
		pPart = GetPPARTINFO(partL);
		
		if (pPart->firstStaff<1 || pPart->firstStaff>doc->nstaves
		||  pPart->lastStaff<1 || pPart->lastStaff>doc->nstaves) {
			COMPLAIN("•DCheckVoiceTable: PART %ld firstStaff OR lastStaff IS ILLEGAL.\n",
						(long)partn);
			return bad;
		}
		
		voiceInWrongPart = FALSE;
		for (stf = pPart->firstStaff; stf<=pPart->lastStaff; stf++) {
			if (doc->voiceTab[stf].partn!=partn) {
				COMPLAIN2("*DCheckVoiceTable: VOICE %ld SHOULD BELONG TO PART %ld BUT DOESN'T.\n",
								(long)stf, (long)partn);
				voiceInWrongPart = TRUE;
			}
		}
	}

	for (v = 1; v<=MAXVOICES; v++) {
		voiceUseTab[v] = NILINK;

	/*
	 *	The objects with voice numbers other than notes and grace notes always get
	 *	their voice numbers from the notes and grace notes they're attached to, so
	 *	we'll assume they're OK, though it would be better to look.
	 */
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (voiceUseTab[NoteVOICE(aNoteL)]==NILINK)
						voiceUseTab[NoteVOICE(aNoteL)] = pL;
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
					if (voiceUseTab[GRNoteVOICE(aGRNoteL)]==NILINK)
						voiceUseTab[GRNoteVOICE(aGRNoteL)] = pL;
				break;
			default:
				;
		}
	}

	for (v = 1; v<=MAXVOICES; v++) {
		if (voiceUseTab[v]!=NILINK)
			if (doc->voiceTab[v].partn==0)
				COMPLAIN2("*DCheckVoiceTable: VOICE %ld (FIRST USED AT %lu) NOT IN TABLE.\n",
					(long)v, (ulong)voiceUseTab[v]);
	}
	
	for (v = 1; v<=MAXVOICES; v++) {
		if (fullCheck && doc->voiceTab[v].partn!=0 && voiceUseTab[v]==NILINK)
			COMPLAIN("DCheckVoiceTable: VOICE %ld HAS NO NOTES, RESTS, OR GRACE NOTES.\n",
				(long)v);
	}

	for (*pnVoicesUsed = 0, v = 1; v<=MAXVOICES; v++)
		if (voiceUseTab[v]!=NILINK) (*pnVoicesUsed)++;

	if (voiceInWrongPart) return bad;				/* Following checks may give 100's of errors! */
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					v = NoteVOICE(aNoteL);
					stf = NoteSTAFF(aNoteL);
					if (doc->voiceTab[v].partn!=Staff2Part(doc, stf))
						COMPLAIN3("*DCheckVoiceTable: NOTE IN SYNC AT %lu STAFF %d IN ANOTHER PART'S VOICE, %ld.\n",
										(ulong)pL, stf, (long)v);
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					v = GRNoteVOICE(aGRNoteL);
					stf = GRNoteSTAFF(aGRNoteL);
					if (doc->voiceTab[v].partn!=Staff2Part(doc, stf))
						COMPLAIN3("*DCheckVoiceTable: NOTE IN GRSYNC AT %lu STAFF %d IN ANOTHER PART'S VOICE, %ld.\n",
										(ulong)pL, stf, (long)v);
				}
				break;
			default:
				;
		}

	for (v = 1; v<=MAXVOICES; v++) {
		if (doc->voiceTab[v].partn!=0) {
			if (doc->voiceTab[v].voiceRole<UPPER_DI || doc->voiceTab[v].voiceRole>SINGLE_DI)
				COMPLAIN("*DCheckVoiceTable: voiceTab[%ld].voiceRole IS ILLEGAL.\n", (long)v);
			if (doc->voiceTab[v].relVoice<1 || doc->voiceTab[v].relVoice>MAXVOICES)
				COMPLAIN("*DCheckVoiceTable: voiceTab[%ld].relVoice IS ILLEGAL.\n", (long)v);
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




/* -------------------------------------------------------------- DCheckRedundKS -- */
/* Check for redundant key signatures, "redundant" in the sense that the key sig. is
a cancellation and the last key signature on the same staff was a cancel. */

Boolean DCheckRedundKS(Document *doc)
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
					COMPLAIN2("*DCheckRedundKS: KEYSIG AT %u REPEATS CANCEL ON %d STAVES.\n",
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

/* -------------------------------------------------------------- DCheckRedundTS -- */
/* Check for redundant time signatures, "redundant" in the sense that there's already
been a time signature on the same staff in the same measure. */

Boolean DCheckRedundTS(Document *doc)
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
					COMPLAIN2("DCheckRedundTS: TIMESIG AT %u BUT MEAS ALREADY HAS TIMESIG ON %d STAVES.\n",
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
	-The total duration of each voice that has any notes agrees with the time signature
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
				measDurActual = GetMeasDur(doc, barTermL);
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
/* Check chords for unisons. */

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


/* -------------------------------------------------------------- DCheck1NEntries -- */
/* Check the given object's nEntries field for agreement with the length of its list
of subobjects. */

Boolean DCheck1NEntries(
				Document */*doc*/,		/* unused */
				LINK pL)
{
	LINK subL, tempL; Boolean bad; short subCount;
	HEAP *myHeap;
	
	bad = FALSE;
		
	if (TYPE_BAD(pL)) {
		COMPLAIN2("•DCheck1NEntries: OBJ AT %u HAS BAD type %d.\n", pL, ObjLType(pL));
		return bad;
	}

	myHeap = Heap + ObjLType(pL);
	for (subCount = 0, subL = FirstSubLINK(pL); subL!=NILINK;
			subL = tempL, subCount++) {
		if (subCount>255) {
			COMPLAIN2("•DCheck1NEntries: OBJ AT %u HAS nEntries=%d  BUT SEEMS TO HAVE OVER 255 SUBOBJECTS.\n",
							pL, LinkNENTRIES(pL));
			break;
		}
		tempL = NextLink(myHeap, subL);
	}

	if (LinkNENTRIES(pL)!=subCount)
		COMPLAIN3("•DCheck1NEntries: OBJ AT %u HAS nEntries=%d BUT %d SUBOBJECTS.\n",
						pL, LinkNENTRIES(pL), subCount);
	
	return bad;
}


/* -------------------------------------------------------------- DCheckNEntries -- */
/* For the entire main object list, Check objects' nEntries fields for agreement with
the lengths of their lists of subobjects. This function is designed to be called
independently of Debug when things are bad, e.g., to check for Mackey's Disease, so it
protects itself against other simple data structure problems. */

Boolean DCheckNEntries(Document *doc)
{
	LINK pL; Boolean bad; long soon=TickCount()+60L;
	
	bad = FALSE;
		
	for (pL = doc->headL; pL && pL!=doc->tailL; pL = RightLINK(pL)) {
		if (TickCount()>soon) WaitCursor();
		if (DCheck1NEntries(doc, pL)) { bad = TRUE; break; }
	}
	
	return bad;
}


/* ---------------------------------------------------------- DCheck1SubobjLinks -- */
/* Check subobject links for the given object. */

Boolean DCheck1SubobjLinks(Document *doc, LINK pL)
{
	PMEVENT p; HEAP *tmpHeap;
	LINK subObjL, badLink; Boolean bad;
	
	bad = FALSE;
	
	switch (ObjLType(pL)) {
		case HEADERtype: {
			LINK aPartL;
			
			for (aPartL=FirstSubLINK(pL); aPartL; aPartL=NextPARTINFOL(aPartL))
				if (DBadLink(doc, ObjLType(pL), aPartL, FALSE))
					{ badLink = aPartL; bad = TRUE; break; }
			break;
		}
		case STAFFtype: {
			LINK aStaffL;
			
				for (aStaffL=FirstSubLINK(pL); aStaffL; aStaffL=NextSTAFFL(aStaffL))
				if (DBadLink(doc, ObjLType(pL), aStaffL, FALSE))
					{ badLink = aStaffL; bad = TRUE; break; }
			}
			break;
		case CONNECTtype: {
			LINK aConnectL;
			
				for (aConnectL=FirstSubLINK(pL); aConnectL; aConnectL=NextCONNECTL(aConnectL))
				if (DBadLink(doc, ObjLType(pL), aConnectL, FALSE))
					{ badLink = aConnectL; bad = TRUE; break; }
			}
			break;
		case SYNCtype:
		case GRSYNCtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case MEASUREtype:
		case PSMEAStype:
		case DYNAMtype:
		case RPTENDtype:
			tmpHeap = Heap + ObjLType(pL);		/* p may not stay valid during loop */
			
			for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL=NextLink(tmpHeap,subObjL))
				if (DBadLink(doc, ObjLType(pL), subObjL, FALSE))
					{ badLink = subObjL; bad = TRUE; break; }
			break;
		case SLURtype: {
			LINK aSlurL;
			
				for (aSlurL=FirstSubLINK(pL); aSlurL; aSlurL=NextSLURL(aSlurL))
				if (DBadLink(doc, ObjLType(pL), aSlurL, FALSE))
					{ badLink = aSlurL; bad = TRUE; break; }
			}
			break;
			
		case BEAMSETtype: {
			LINK aNoteBeamL;
			
			aNoteBeamL = FirstSubLINK(pL);
			for ( ; aNoteBeamL; aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
				if (DBadLink(doc, ObjLType(pL), aNoteBeamL, FALSE))
					{ badLink = aNoteBeamL; bad = TRUE; break; }
			}
			break;
		}
		case OTTAVAtype: {
			LINK aNoteOctL;
			
			aNoteOctL = FirstSubLINK(pL);
			for ( ; aNoteOctL; aNoteOctL=NextNOTEOTTAVAL(aNoteOctL)) {
				if (DBadLink(doc, ObjLType(pL), aNoteOctL, FALSE))
					{ badLink = aNoteOctL; bad = TRUE; break; }
			}
			break;
		}
		case TUPLETtype: {
			LINK aNoteTupleL;
			
			aNoteTupleL = FirstSubLINK(pL);
			for ( ; aNoteTupleL; aNoteTupleL=NextNOTETUPLEL(aNoteTupleL)) {
				if (DBadLink(doc, ObjLType(pL), aNoteTupleL, FALSE))
					{ badLink = aNoteTupleL; bad = TRUE; break; }
			}
			break;
		}

		case GRAPHICtype:
		case TEMPOtype:
		case SPACERtype:
		case ENDINGtype:
			break;
			
		default:
			;
			break;
	}
	
	if (bad) COMPLAIN2("•DCheck1SubobjLinks: A SUBOBJ OF OBJ AT %u HAS A BAD LINK OF %d.\n",
								pL, badLink);
	
	return bad;
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

	midCHalfLn = ClefMiddleCHalfLn(clefType);				/* Get middle C staff pos. */		
	yqpit = NoteYQPIT(theNoteL)+halfLn2qd(midCHalfLn);
	halfLn = qd2halfLn(yqpit);									/* Number of half lines from stftop */

	if (!FirstTiedNote(syncL, theNoteL, &firstSyncL, &firstNoteL))
		return FALSE;												/* Object list is messed up */
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
