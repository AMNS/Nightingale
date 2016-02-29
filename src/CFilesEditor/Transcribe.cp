/*	Transcribe.c (formerly GuessDurs.c) for Nightingale - routines for quantizing and
clarifying rhythm of unknown-duration notes in existing scores - rev. for v.3.1. */

/*										NOTICE
 *
 *	THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROPERTY OF
 *	ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE SECRET AND IS
 *	NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED WRITTEN
 *	AUTHORIZATION FROM THE OWNER.
 *
 *	Copyright ©1993-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define MERGE_TAB_SIZE 2500		/* Max. syncs in table for merging into */
#define MAX_TSCHANGE 1000		/* Max. no. of time sig. changes in area */

static short measTabLen=0;
static MEASINFO *measInfoTab;

static Boolean InitQuantize(Document *, Boolean);
static short Voice2RTStructs(Document *,short,long,LINKTIMEINFO [],short,short *,NOTEAUX [],short *);
static short Voice2KnownDurs(Document *,short,short,short,LINKTIMEINFO [],short,short,NOTEAUX [],short,
							long,LINK,LINK,LINK *);
static LINK QuantAllVoices(Document *, short, short, Boolean);

static LINK GDNewMeasure(Document *, LINK);
static void GDFixTStampsForTimeSigs(Document *, LINK, LINK);
static LINK GDAddMeasures(Document *, LINK);
static Boolean SyncOKToQuantize(LINK);
static Boolean GDRespAndRfmt(Document *, short, short, Boolean);
static Boolean QuantizeSelDurs(Document *, short, Boolean);
static Boolean QuantizeDialog(Document *, short *, Boolean *, Boolean *);

/* ---------------------------------------------------------------- InitQuantize -- */
/* Initialize for Voice2KnownDurs: build measInfoTab[]. */

static Boolean InitQuantize(Document *doc, Boolean merge)
{
	CONTEXT context; long measDur, selStartTime, selEndTime; short count;
	LINK measL, syncL;
	
	/* For now, no time sigs. are allowed in the range to be quantized, so this is
		easy: the table always has just one entry. */
	
	GetContext(doc, doc->selStartL, 1, &context);

	if (merge)
		measInfoTab[0].count = 1;
	else {
		measDur = TimeSigDur(0, context.numerator, context.denominator);
		measL = LSSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
		selStartTime = MeasureTIME(measL);
		syncL = LSSearch(LeftLINK(doc->selEndL), SYNCtype, ANYONE, GO_LEFT, FALSE);
		selEndTime = SyncAbsTime(syncL)+SyncMaxDur(doc, syncL);
		count = (selEndTime-selStartTime)/measDur;
		if (count*measDur<selEndTime-selStartTime) count++;			/* Always round up */
		measInfoTab[0].count = count;
	}

	measInfoTab[0].numerator = context.numerator;
	measInfoTab[0].denominator = context.denominator;
	measTabLen = 1;
	
	return TRUE;
}

/* -------------------------------------------------------------- Voice2RTStructs -- */

#ifdef PUBLIC_VERSION

#define DEBUG_NOTE

#else

#define DEBUG_NOTE if (ShiftKeyDown() && OptionKeyDown())										\
						LogPrintf(LOG_NOTICE, "iSync=%d %cvoice=%d [%d] noteNum=%d dur=%ld\n",	\
						iSync, (nInChord>1? '+' : ' '), voice, nAux,							\
						rawNoteAux[nAux].noteNumber, rawNoteAux[nAux].duration)

#endif

static short Voice2RTStructs(
					Document *doc,
					short voice,
					long startTime,
					LINKTIMEINFO rawSyncTab[],		/* Output: raw (unquantized and unclarified) NCs */
					short maxRawSyncs,				/* Maximum size of rawSyncTab */
					short *pnSyncs,
					NOTEAUX rawNoteAux[],			/* Output: auxiliary info on raw notes */
					short *pnAux
					)
{
	long prevStartTime, chordEndTime;
	short iSync, nAux, nInChord;
	LINK pL, aNoteL; PANOTE aNote;
	
	prevStartTime = -999999L;
	iSync = -1;
	nAux = -1;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
				if (LinkSEL(pL) && SyncInVoice(pL, voice)) {
					nInChord = 0;
					for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteSEL(aNoteL) && NoteVOICE(aNoteL)==voice) {
							nInChord++;
							nAux++;
							if (nInChord==1) {
								iSync++;
								if (iSync>=maxRawSyncs) {
									MayErrMsg("Voice2RTStructs: maxRawSyncs exceeded.");
									return FAILURE;
								}
								rawSyncTab[iSync].link = nAux;
							}
		
							aNote = GetPANOTE(aNoteL);
							rawNoteAux[nAux].noteNumber = aNote->noteNum;
							rawNoteAux[nAux].onVelocity =  aNote->onVelocity;
							rawNoteAux[nAux].offVelocity =  aNote->offVelocity;
							rawNoteAux[nAux].duration =  aNote->playDur;
							rawNoteAux[nAux].first = (nInChord==1);
							DEBUG_NOTE;
	
							prevStartTime = SyncAbsTime(pL)-startTime;
			
							/* If this note is part of a chord, use the latest end time. */
							
							if (nInChord>1) {
								chordEndTime = prevStartTime+rawSyncTab[iSync].mult;
								if (prevStartTime+rawNoteAux[nAux].duration>chordEndTime) {
									rawSyncTab[iSync].mult = rawNoteAux[nAux].duration;
								}
		
							}
							else {
								rawSyncTab[iSync].mult = rawNoteAux[nAux].duration;
								rawSyncTab[iSync].time = SyncAbsTime(pL)-startTime;
							}			
						}
				}
				break;

			default:
				;
		}
	}

	rawNoteAux[++nAux].first = TRUE;											/* Sentinel at end */

	*pnSyncs = iSync+1;
	*pnAux = nAux;
	return (iSync>=0? OP_COMPLETE : NOTHING_TO_DO);
}

/* -------------------------------------------------------------- Voice2KnownDurs -- */

static void CantHandleVoice(Document *, short, short);
static void CantHandleVoice(Document *doc, short voice, short tVErr)
{
	short userVoice; LINK partL; PPARTINFO pPart; char partName[256];
	char fmtStr[256];
	
	if (Int2UserVoice(doc, voice, &userVoice, &partL)) {
		pPart = GetPPARTINFO(partL);
		strcpy(partName, (strlen(pPart->name)>14? pPart->shortName : pPart->name));
		GetIndCString(fmtStr, MIDIERRS_STRS, 11);				/* "Couldn't handle voice..." */
		sprintf(strBuf, fmtStr, userVoice, partName, tVErr);
		CParamText(strBuf, "", "", "");
		StopInform(GUESSDUR_ALRT);
	}
	else
		SysBeep(1);
}


/* Quantize, i.e., "Transcribe Recording" of, notes in the given voice described by
rawSyncTab[] and rawNoteAux[], merge the results into <doc>, and add rests between
notes. InitQuantize MUST be called before this! Returns FAILURE if there's a problem,
else NOTHING_TO_DO or OP_COMPLETE.*/

static short Voice2KnownDurs(
					Document *doc,
					short voice,
					short quantum,						/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
					short tripletBias,				/* Percent bias towards quant. to triplets >= 2/3 quantum */
					LINKTIMEINFO rawSyncTab[],		/* Input: raw (unquantized and unclarified) NCs */
					short maxRawSyncs,				/* Maximum size of rawSyncTab */
					short nRawSyncs,					/* Current size of rawSyncTab */
					NOTEAUX rawNoteAux[],			/* Input: raw notes */
					short nAux,							/* Size of rawNoteAux */
					long tOffset,						/* Offset time for quantized notes/chords, in PDUR ticks */
					LINK qExtraMeasL, LINK qEndL,	/* The extra Measure, the end of the range to fill with rests */
					LINK *pLastL 						/* The last LINK affected by the merge */
					)
{
	LINKTIMEINFO *newSyncTab=NULL, *mergeObjs=NULL;
	LINK newFirstSync, qStartMeasL;
	LINK endMeasL, lastL, endFillL;
	short nNewSyncs, nMergeObjs, tVErr, i;
	unsigned short maxNewSyncs;
	Boolean okay=FALSE;
	char fmtStr[256];
	
	if (nRawSyncs<=0 || nAux<=0) return NOTHING_TO_DO;

	/*
	 *	We have one or more notes. Convert and merge them. <maxNewSyncs> has to be
	 * large enough for all existing Syncs plus any generated by clarifying. I can't
	 *	think of a good way to guess how much clarifying to allow for. But <maxNewSyncs>
	 * certainly can't be more than 65535, since that's the most notes/rests we can
	 * have in an entire score! For now, make a wild but generous guess.
	 */
	maxNewSyncs = n_min(5L*nRawSyncs+MF_MAXPIECES, 65535L);
	newSyncTab = (LINKTIMEINFO *)NewPtr(maxNewSyncs*sizeof(LINKTIMEINFO));
	if (!GoodNewPtr((Ptr)newSyncTab))
		{ NoMoreMemory(); newSyncTab = NULL; goto Done; }
		
	/*
	 *	Generate quantized and clarified notes and chords from the tables.
	 */
	newFirstSync = TranscribeVoice(doc, voice, rawSyncTab, maxRawSyncs, nRawSyncs,
									rawNoteAux, nAux, quantum, tripletBias, measInfoTab,
									measTabLen, qExtraMeasL, newSyncTab, maxNewSyncs,
									&nNewSyncs, &tVErr);
	if (!newFirstSync) {
		CantHandleVoice(doc, voice, tVErr);
		goto Done;
	}
	
	/* At this point, Measure <qExtraMeasL> of the score contains ALL of the quantized
		notes/chords, [newSyncTab[0], newSyncTab[nRawSyncs-1]], plus possible ties
		and tuplets; we want to merge all of it starting at the next Measure. Note that
		the number of objects in that Measure can greatly exceed MAX_MEASNODES, so a lot
		of Nightingale's machinery can't be used now. */

	qStartMeasL = LinkRMEAS(qExtraMeasL);

	for (i = 0; i<nNewSyncs; i++)
		newSyncTab[i].time += tOffset;

	mergeObjs = (LINKTIMEINFO *)NewPtr(MERGE_TAB_SIZE*sizeof(LINKTIMEINFO));
	if (!GoodNewPtr((Ptr)mergeObjs))
		{ NoMoreMemory(); mergeObjs = NULL; goto Done; }

	nMergeObjs = FillMergeBuffer(qStartMeasL, mergeObjs, MERGE_TAB_SIZE, FALSE);
	if (nMergeObjs>MERGE_TAB_SIZE) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 12);			/* "Can't Transcribe more than..." */
		sprintf(strBuf, fmtStr, MERGE_TAB_SIZE);
		CParamText(strBuf, "", "", "");
		StopInform(GUESSDUR_ALRT);
		goto Done;
	}

	/*
	 * Merge everything into its proper place. NB: MRMerge gets timing information
	 *	from the LINKTIMEINFO arrays, not from the object list!
	 */
	if (!MRMerge(doc, voice, newSyncTab, nNewSyncs, mergeObjs, nMergeObjs, &lastL))
		{ NoMoreMemory(); goto Done; }

	if (!PreflightMem(40))
		{ NoMoreMemory(); goto Done; }
	
	endFillL = (qEndL? LeftLINK(qEndL) : lastL);
	endMeasL = LSSearch(endFillL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	if (!FillNonemptyMeas(doc, qStartMeasL, endMeasL, voice, quantum)) goto Done;
	
	okay = TRUE;

Done:
	//if (rawSyncTab) DisposePtr((Ptr)rawSyncTab);		// BAD BAD BAD - Dont do this here!!
	//if (rawNoteAux) DisposePtr((Ptr)rawNoteAux);		// BAD BAD BAD - Dont do this here!!
	if (mergeObjs) DisposePtr((Ptr)mergeObjs);
	if (newSyncTab) DisposePtr((Ptr)newSyncTab);
	
	*pLastL = lastL;
	if (!okay) return FAILURE;
	return (newFirstSync? OP_COMPLETE : NOTHING_TO_DO);
}


/* ---------------------------------------------------------------- GDNewMeasure -- */
/* Add a new Measure object just before <pL> and leave an insertion point set just
before <pL>. At the moment, virtually identical to MFNewMeasure. */

static LINK GDNewMeasure(Document *doc, LINK pL)
{
	LINK measL; short sym; CONTEXT context;

	doc->selStartL = doc->selEndL = pL;
	NewObjInit(doc, MEASUREtype, &sym, singleBarInChar, ANYONE, &context);
	measL = CreateMeasure(doc, pL, -1, sym, context);
	doc->selStartL = doc->selEndL = pL;
	return measL;
}


/* ----------------------------------------------------- GDFixTStampsForTimeSigs -- */
/* Starting at a given point in the score, set the timestamp of the next Measure to 0;
then set the timestamp of every following Measure until a given point to reflect the
notated (i.e., time signature's) duration of the previous Measure, paying no attention
to the contents of the Measures. Finally, offset the timestamps of all Measures
after that accordingly (preserving their durations). NB: MIDIFileOpen.c contains a
similar function: the two should probably be combined.*/

static void GDFixTStampsForTimeSigs(
					Document *doc,
					LINK startL,	/* Start at 1st Meas at or after this, or NILINK=start at very 1st Meas */
					LINK endMeasL 	/* Don't change dur of this Meas and following ones, or NILINK */
					)
{
	long measDur, refTime, timeChange;
	LINK measL, nextMeasL, endL;
	CONTEXT context;
	
	startL = SSearch((startL? startL : doc->headL), MEASUREtype, GO_RIGHT);
	
	measL = startL;
	MeasureTIME(measL) = 0L;
	endL = (endMeasL? LinkLMEAS(endMeasL) : NILINK);
	
	refTime = (endL? MeasureTIME(endL) : 0L);

	for ( ; measL && measL!=endL; measL = LinkRMEAS(measL)) {
		nextMeasL = LinkRMEAS(measL);
		if (nextMeasL) {
			if (nextMeasL==startL)
				MeasureTIME(nextMeasL) = 0L;
			else if (!MeasISFAKE(measL)) {
				/* Time sig. for this Meas. may appear AFTER it, so GetContext at next Meas. */ 
				GetContext(doc, nextMeasL, 1, &context);
				measDur = TimeSigDur(N_OVER_D, context.numerator, context.denominator);
				MeasureTIME(nextMeasL) = MeasureTIME(measL)+measDur;
			}
			else
				MeasureTIME(nextMeasL) = MeasureTIME(measL);
		}
	}
	
	if (!endL) return;

	timeChange = MeasureTIME(endL)-refTime;
	MoveTimeMeasures(RightLINK(measL), doc->tailL, timeChange);
}


/* ---------------------------------------------------------------- GDAddMeasures -- */
/* Just before <qEndL>, add an "extra" Measure, which should later be removed, to
create a temporary storage area, and add Measures as described by <measInfoTab[0]>
(at some future time we may support time sig. changes, as Import MIDI File's
MFAddMeasures does). Set the new Measures' timestamps to give them the durations
indicated by the time signature, with the extra Measure's timestamp set to 0. Return
the first new Measure, the "extra" Measure. If we fail (due to lack of memory), return
NILINK. NB: Destroys the selection range and leaves timestamps in an inconsistent
state! */

static LINK GDAddMeasures(Document *doc, LINK qEndL)
{
	short count, m;
	LINK measL, newMeasL, startFixL, endFixL;
	Boolean okay=FALSE;
	
	/* Add the "extra" Measure and all the other Measures (if any!) we need. */
	
	count = measInfoTab[0].count;			/* Measure CONTENTS needed, so already includes the extra */
	for (newMeasL = NILINK, m = 0; m<count; m++) {
		measL = GDNewMeasure(doc, qEndL);
		if (!measL) goto Done;
		if (m==0) newMeasL = measL;
	}
	
	okay = TRUE;

Done:
	if (!okay) {
		GetIndCString(strBuf, MIDIERRS_STRS, 13);					/* "Couldn't add measures" */
		CParamText(strBuf, "", "", "");
		StopInform(GUESSDUR_ALRT);
		return NILINK;
	}
	
	/* Set timestamps, starting with the "extra" Measure. */
	
	startFixL = newMeasL;
	endFixL = SSearch(qEndL, MEASUREtype, GO_RIGHT);
	endFixL = (endFixL? LinkRMEAS(endFixL) : NILINK);
	GDFixTStampsForTimeSigs(doc, startFixL, endFixL);

	return startFixL;
}


/* ----------------------------------------------------------- DelAndMoveMeasure -- */

static void DelAndMoveMeasure(Document *, LINK);
static void DelAndMoveMeasure(Document *doc, LINK measL)
{
	LINK prevMeasL; DDIST xMove, prevWidth, width;
	
	/* Adjust objects' x-coords. to make relative to their new measure but
		keep them in the same place on the page. */

 	if (!MeasureTYPE(RightLINK(measL))) {
		prevMeasL = LSSearch(LeftLINK(measL),MEASUREtype,ANYONE,GO_LEFT,FALSE);
 		xMove = SysRelxd(measL)-SysRelxd(prevMeasL);
 		MoveInMeasure(RightLINK(measL), doc->tailL, xMove);
 	}
 	
	prevMeasL = LSSearch(LeftLINK(measL),MEASUREtype,ANYONE,GO_LEFT,FALSE);
	prevWidth = MeasWidth(prevMeasL);
	width = MeasWidth(measL);
	SetMeasWidth(prevMeasL, prevWidth+width);

	if (measL) DeleteMeasure(doc, measL);
}

/* -------------------------------------------------------------- QuantAllVoices -- */
/* Quantize the selection. Restrictions:
	- Between doc->selStartL and selEndL, there can only be Syncs and Measures.
	- Every Sync in the range must be selected.
	- The Syncs can contain only unknown-duration notes, all of them selected.
	- The notes may not have any modifers or be beamed, slurred, tied, or in Ottavas.
	- If !merge, doc->selStartL must be the first obj in a Measure, and selEndL must
		end a Measure.
The quantized notes will have redundant accidentals; call DelRedundantAccs to remove
them.

Return NILINK either if we actually have no notes to quantize or if there are any
problems.

The calling routine is responsible for respacing and reformatting, if appropriate.
*/

#define EXTRA_OBJS 10

static LINK QuantAllVoices(
		Document *doc,
		short quantum,
		short tripletBias,					/* Percent bias towards quant. to triplets >= 2/3 quantum */
		Boolean merge
		)
{
	short v, status, nSyncs, nNotes;
	short timeOffset=0;									/* BEFORE quantizing, in PDUR ticks: normally 0 */
	long startTime, len;
	long postQTimeOffset;								/* AFTER quantizing, in PDUR ticks: normally 0 */
	Boolean gotNotes=FALSE, okay=FALSE;
	LINKTIMEINFO *rawSyncTab[MAXVOICES+1];			/* Arrays of pointers to tables */
	NOTEAUX	*rawNoteAux[MAXVOICES+1];
	short maxSyncs[MAXVOICES+1], nRawSyncs[MAXVOICES+1], nAux[MAXVOICES+1];
	LINK prevMeasL, endExtraL, qStartL, qEndL, lastL;
	
	measInfoTab = NULL;
	for (v = 1; v<=MAXVOICES; v++) {
		rawSyncTab[v] = NULL;
		rawNoteAux[v] = NULL;
	}

	len = MAX_TSCHANGE*sizeof(MEASINFO);
	measInfoTab = (MEASINFO *)NewPtr(len);
	if (!GoodNewPtr((Ptr)measInfoTab)) {
		OutOfMemory(len);
		goto Done;
	}

	prevMeasL = SSearch(doc->selStartL, MEASUREtype, GO_LEFT);
	startTime = MeasureTIME(prevMeasL)-timeOffset;
	
	/* Make a rawSyncTab[] and a rawNoteAux[] for each voice in the range. */
	
	for (v = 1; v<=MAXVOICES; v++) {
		if (VOICE_MAYBE_USED(doc, v)) {
			nSyncs = VCountNotes(v, doc->selStartL, doc->selEndL, TRUE);
			/* If nothing in voice in range, set variables to safe values. */
			if (nSyncs<=0) {
				nRawSyncs[v] = nAux[v] = 0;
				rawSyncTab[v] = (LINKTIMEINFO *)NULL;
				rawNoteAux[v] = (NOTEAUX *)NULL;
				continue;
			}
			maxSyncs[v] = 2*nSyncs+EXTRA_OBJS;
			nNotes = VCountNotes(v, doc->selStartL, doc->selEndL, FALSE);
	
			len = maxSyncs[v]*sizeof(LINKTIMEINFO);
			rawSyncTab[v] = (LINKTIMEINFO *)NewPtr(len);
			if (!GoodNewPtr((Ptr)rawSyncTab[v])) {
				rawSyncTab[v] = NULL;
				OutOfMemory(len);
				goto Done;
			}
			len = (nNotes+EXTRA_OBJS)*sizeof(NOTEAUX);
			rawNoteAux[v] = (NOTEAUX *)NewPtr(len);
			if (!GoodNewPtr((Ptr)rawNoteAux[v])) {
				rawNoteAux[v] = NULL;
				OutOfMemory(len);
				goto Done;
			}
	
			status = Voice2RTStructs(doc, v, startTime, rawSyncTab[v], maxSyncs[v],
												&nRawSyncs[v], rawNoteAux[v], &nAux[v]);
			if (status==FAILURE) goto Done;
		}
		else
			nRawSyncs[v] = nAux[v] = 0;
	}
	
	InitQuantize(doc, merge);
	
	/* Since the notes can't have any modifiers nor be beamed, tied, etc., the
		rawSyncTab[]'s and the rawNoteAux[]'s have all the information there is about the
		notes, except for spelling, which we won't worry about. So we now delete the
		selection and replace it with zero or more Measures: this gives us an empty area
		with the correct structure for merging back into. */

	qEndL = doc->selEndL;
	DeleteRange(doc, RightLINK(prevMeasL), qEndL);
	FixStructureLinks(doc, doc, doc->headL, doc->tailL);	/* Overkill; optimize some day */

	UpdatePageNums(doc);
	UpdateSysNums(doc, doc->headL);
	UpdateMeasNums(doc, NILINK);
	
	endExtraL = GDAddMeasures(doc, qEndL);						/* Destroys sel range & timestamps */
	if (!endExtraL) goto Done;
	qStartL = LinkLMEAS(endExtraL);

	postQTimeOffset = 0L;

#ifndef PUBLIC_VERSION
	LogPrintf(LOG_NOTICE, "quantum=%d tripBias=%d tryLev=%d leastSq=%d\n",
		quantum, tripletBias, config.tryTupLevels, config.leastSquares);
#endif

	/* The range [qStartL,qEndL) now starts one before and extends to the last newly-
		added Measures. We're ready to go. Quantize and merge back in one voice at a
		time. */
	
	for (v = 1; v<=MAXVOICES; v++) {
		if (nRawSyncs[v]<=0) continue;
		
		sprintf(strBuf, " %d...", v);
		ProgressMsg(VOICE_PMSTR, strBuf);
		
		status = Voice2KnownDurs(doc, v, quantum, tripletBias, rawSyncTab[v], maxSyncs[v],
											nRawSyncs[v], rawNoteAux[v], nAux[v], postQTimeOffset,
											qStartL, (merge? NILINK : qEndL), &lastL);
		if (status==FAILURE) break;
		if (status==OP_COMPLETE) gotNotes = TRUE;

		if (v<MAXVOICES && CheckAbort()) {						/* Check for user cancelling */
			ProgressMsg(SKIPVOICE_PMSTR, "");
			SleepTicks(75L);											/* So user can read the msg */
			break;
		}
	}
	ProgressMsg(0, "");
	
	if (status==FAILURE) {
		/* Bail out gracefully. ??Not easy to do: in this situation in Import MIDI File,
			we call DoCloseDocument, but that's too drastic here, since the user might
			have done a lot of work on the score they want to keep. We really need to do
			the equivalent of Undo. For now, just leave things as they are. */
		goto Done;
	}

	/*
	 * Clean up. Kill the extra Measure (see comments in Voice2KnownDurs); try to
	 *	get rid of unisons in chords; etc. We need to handle unisons because the
	 *	process we've gone thru ignores the notes' current spellings, so it can turn
	 *	minor seconds into augmented unisons.
	 */
	if (gotNotes) {
		DelAndMoveMeasure(doc, endExtraL);
		UpdateMeasNums(doc, NILINK);
		
	for (v = 1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {
			if (nRawSyncs[v]<=0) continue;
			AvoidUnisonsInRange(doc, doc->headL, doc->tailL, v);
		}

		doc->selStartL = qStartL;
		doc->selEndL = qEndL;

		okay = TRUE;
	}

Done:
	if (measInfoTab) DisposePtr((Ptr)measInfoTab);
	for (v = 1; v<=MAXVOICES; v++) {
		if (!VOICE_MAYBE_USED(doc, v)) continue;
		if (rawSyncTab[v]) DisposePtr((Ptr)rawSyncTab[v]);
		if (rawNoteAux[v]) DisposePtr((Ptr)rawNoteAux[v]);
	}

	return (okay? lastL : NILINK);
}


/* ------------------------------------------------------------ SyncOKToQuantize -- */

static Boolean SyncOKToQuantize(LINK syncL)
{
	LINK aNoteL; PANOTE aNote;
	
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->rest) return FALSE;
		if (aNote->subType!=UNKNOWN_L_DUR) return FALSE;
		if (aNote->beamed || aNote->tiedL || aNote->tiedR
		||  aNote->slurredL || aNote->slurredR || aNote->inTuplet)
			return FALSE;
		if (aNote->firstMod) return FALSE;
	}
	
	return TRUE;
}

/* ------------------------------------------------------ SyncHasNondefaultVoice -- */

static Boolean SyncHasNondefaultVoice(LINK);
static Boolean SyncHasNondefaultVoice(LINK syncL)
{
	LINK aNoteL;
	
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)!=NoteSTAFF(aNoteL)) return TRUE;
	
	return FALSE;
}


/* --------------------------------------------------- GDRespAndRfmt and Helpers -- */

/* For every content object in the range of LINKs [startL, endL), SelectRange2
selects the object in the INCLUSIVE range of staves [firstStf, lastStf]: if it or
any of its subobjects are in the range of staves, it selects the object and those
subobjects. It does NOT deselect objects that aren't in the range of staves! Also
doesn't change hiliting or make any other user-interface assumptions.

You'd think a loop calling CheckObject could do the same thing, but probably not
--I don't think any of the CheckXXX routine modes quite do this. Of course, a mode
could be added to do this, but this is enough of a special case that I think it
makes sense having a separate routine for it.

??This is the same as SelectRange EXCEPT it skips non-inMeasure Clefs, KeySigs,
and TimeSigs, which really aren't content objects; this should probably replace
SelectRange. */

static void SelectRange2(Document *, LINK, LINK, short, short);
static void SelectRange2(Document *doc, LINK startL, LINK endL, short firstStf,
									short lastStf)
{
	LINK 			pL,subObjL,aSlurL;
	PMEVENT		p;
	GenSubObj 	*subObj;
	HEAP 			*tmpHeap;

	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (ClefTYPE(pL) && !ClefINMEAS(pL)) continue;
		if (KeySigTYPE(pL) && !KeySigINMEAS(pL)) continue;
		if (TimeSigTYPE(pL) && !TimeSigINMEAS(pL)) continue;
		
		p = GetPMEVENT(pL);
		switch (ObjLType(pL)) {
			case SYNCtype:
			case GRSYNCtype:
			case MEASUREtype:
			case PSMEAStype:
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case DYNAMtype:
			case RPTENDtype:
				tmpHeap = doc->Heap + ObjLType(pL);
				
				for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
					if (subObj->staffn>=firstStf && subObj->staffn<=lastStf) {
						LinkSEL(pL) = TRUE;
						subObj->selected = TRUE;
					}
				}	
				break;
			case BEAMSETtype:
			case TUPLETtype:
			case OTTAVAtype:
			case ENDINGtype:
			case SPACERtype:
			case GRAPHICtype:
			case TEMPOtype:
				if (((PEXTEND)p)->staffn>=firstStf && ((PEXTEND)p)->staffn<=lastStf)
					LinkSEL(pL) = TRUE;
				break;

			case SLURtype:
				if (SlurSTAFF(pL)>=firstStf && SlurSTAFF(pL)<=lastStf) {
					LinkSEL(pL) = TRUE;
					aSlurL = FirstSubLINK(pL);
					for ( ; aSlurL; aSlurL=NextSLURL(aSlurL))
						SlurSEL(aSlurL) = TRUE;
				}
				break;
		}
	}
}


/* Get the nos. of the Measures enclosing the selection range. If <selEndL> is
after the last Measure, return -1 for <*pEndNum>. */

void GetSelMeasNums(Document *, short *, short *);
void GetSelMeasNums(Document *doc, short *pStartNum, short *pEndNum)
{
	LINK startL, endL, aMeasureL; PAMEASURE aMeasure;
	
	startL = LSSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	aMeasureL = FirstSubLINK(startL);
	aMeasure = GetPAMEASURE(aMeasureL);
	*pStartNum = aMeasure->measureNum;

	endL = LSSearch(doc->selEndL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	if (endL) {
		aMeasureL = FirstSubLINK(endL);
		aMeasure = GetPAMEASURE(aMeasureL);
		*pEndNum = aMeasure->measureNum;
	}
	else
		*pEndNum = -1;
}


/* If any Measure in the score exceeds the maximum legal no. of objects, give an
error message and return TRUE. */

Boolean AnyOverfullMeasure(Document *doc)
{
	LINK measL, pL; short nInMeasure;
	char fmtStr[256];
	
	measL = LSSearch(doc->headL, MEASUREtype, 1, GO_RIGHT, FALSE);		/* Start at first measure */
	nInMeasure = 0;
	
	/* We should really skip reserved areas, but that shouldn't make much difference
		--we'll just be a little overly conservative. */
		
	for (pL = measL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (MeasureTYPE(pL)) nInMeasure = 0;
		if (J_ITTYPE(pL) || J_IPTYPE(pL)) nInMeasure++;
		
		if (nInMeasure>=MAX_MEASNODES) {
			GetIndCString(fmtStr, MIDIERRS_STRS, 14);			/* "Exceeded limit of objs per meas..." */
			sprintf(strBuf, fmtStr, MAX_MEASNODES);
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return TRUE;
		}
	}
	
	return FALSE;
}


/* Respace and reformat selected Measures. */

Boolean GDRespAndRfmt(
				Document *doc,
				short quantCode,		/* Respace appropriately for notes no shorter than this */
				short staffn,			/* Staff no. to select afterwards, or ANYONE for all */
				Boolean merge
				)
{
	LINK pL, startL, endL;
	short startMeasNum, endMeasNum; long spacePercent;
	
	/* Check if any Measure in the score has too much in it; if so, for now, give up.
		It would be much better to just add Measures as needed to keep under the limit,
		but this should very rarely happen anyway. */
		
	if (AnyOverfullMeasure(doc)) return FALSE;
	
	/* Take care of graphical matters: Respace and set slur shapes. If we KNOW there
		aren't going to be any short notes, save some space. Also, if RespaceBars
		reformats, it'll destroy the selection: save the sel. range beforehand and
		reselect afterwards. */
	
	spacePercent = 100L;
	if (quantCode==EIGHTH_L_DUR) spacePercent = 85L;
	else if (quantCode<EIGHTH_L_DUR && quantCode!=UNKNOWN_L_DUR) spacePercent = 70L;
	doc->spacePercent = spacePercent;

	/* If RespaceBars reformats, it loses selection status, so first save enough
		information to restore selection status afterwards. doc->selStartL and
		doc->selEndL may no longer be in the object list after reformatting, so we'll
		save the measure numbers that start and end the selection--that's all we need,
		since Guess Durations works only on entire measures. */
		 
	GetSelMeasNums(doc, &startMeasNum, &endMeasNum);
	
	RespaceBars(doc, doc->selStartL, doc->selEndL, RESFACTOR*spacePercent, FALSE, TRUE);

	/* If RespaceBars didn't reformat at all or didn't reformat everything, we might
		have normal, non-cross-system ties crossing system breaks. Turn any of these
		into real cross-system ties. (FixCrossSysObjects will also try to fix other
		types of objects, which (as of v.1.4) can't exist at this point, but may in the
		future, and the extra work FixCrossSysObjects does shouldn't cost much time.) */
		
	if (merge) FixCrossSysObjects(doc, doc->selStartL, doc->selEndL);

	startL = MNSearch(doc, doc->headL, startMeasNum, GO_RIGHT, TRUE);
	if (endMeasNum>=0)
		endL = MNSearch(doc, doc->tailL, endMeasNum, GO_LEFT, FALSE);
	else
		endL = doc->tailL;
	if (startL && endL) {
		DeselAllNoHilite(doc);
		doc->selStartL = RightLINK(startL);						/* Don't want start Measure selected */
		doc->selEndL = endL;
	
		if (staffn==ANYONE)
			SelectRange2(doc, doc->selStartL, doc->selEndL, 1, MAXSTAVES);
		else
			SelectRange2(doc, doc->selStartL, doc->selEndL, staffn, staffn);
		OptimizeSelection(doc);
	}

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (SlurTYPE(pL))
			SetAllSlursShape(doc, pL, TRUE);

	return TRUE;
}


/* ------------------------------------------------------------- QuantizeSelDurs -- */
/* Extend the selection to include everything in every measure in the selection
range; then quantize the attack time and duration of every selected note/rest/chord
to a grid whose points are separated by the duration described by <quantumDur>. If
there are pauses between the notes, intersperse rests as necessary. Return TRUE if
successful. NB: leaves timestamps in an inconsistent state: the calling function
should call FixTimeStamps afterwards. */

#define RESET_PLAYDURS TRUE

Boolean QuantizeSelDurs(Document *doc, short quantumDur, Boolean tuplets)
{
	short			quantum, tripletBias;
	Boolean		didAnything;
	LINK			startL, endL, pL, nextL, startMeasL, saveEndL;
	char			fmtStr[256];
	
	didAnything = FALSE;
	
	/* This routine is very sensitive to selection anomalies, so get rid of any. */
	
	OptimizeSelection(doc);

	startMeasL = SSearch(doc->selStartL, MEASUREtype, GO_LEFT);
	if (!startMeasL) {
		GetIndCString(strBuf, MIDIERRS_STRS, 15);		/* "selection can't include init. clef, key sig, or time sig" */
		goto Trouble;
	}
	startL = RightLINK(startMeasL);
	endL = EndMeasSearch(doc, LeftLINK(doc->selEndL));

	for (pL = startL; pL!=endL; pL = nextL) {
		nextL = RightLINK(pL);
		/* Skip over reserved areas of systems. */
		if (nextL!=endL && (PageTYPE(nextL) || SystemTYPE(nextL))) {
			nextL = SSearch(nextL, MEASUREtype, GO_RIGHT);
			nextL = RightLINK(nextL);										/* Might be tail */
		}

		switch(ObjLType(pL)) {
			case SYNCtype:
				if (!SyncOKToQuantize(pL)) {
					GetIndCString(fmtStr, MIDIERRS_STRS, 16);		/* "Measure contains rests; notes that aren't unknown..." */
					sprintf(strBuf, fmtStr, GetMeasNum(doc, pL));
					goto Trouble;
				}
				if (SyncHasNondefaultVoice(pL)) {
					GetIndCString(fmtStr, MIDIERRS_STRS, 17);		/* "Measure contains notes in non-dflt voices" */
					sprintf(strBuf, fmtStr, GetMeasNum(doc, pL));
					goto Trouble;
				}
				break;
			case MEASUREtype:
				if (!FirstMeasInSys(pL)) {
					GetIndCString(strBuf, MIDIERRS_STRS, 18);		/* "selection includes barlines" */
					goto Trouble;
				}
				break;
			default:
				GetIndCString(fmtStr, MIDIERRS_STRS, 19);			/* "Measure contains smthg other than notes/rests" */
				sprintf(strBuf, fmtStr, GetMeasNum(doc, pL));
				goto Trouble;
		}
	}

	for (pL = startL; pL!=endL; pL = nextL) {
		nextL = RightLINK(pL);
		/* Skip over reserved areas of systems. */
		if (nextL!=endL && (PageTYPE(nextL) || SystemTYPE(nextL))) {
			nextL = SSearch(nextL, MEASUREtype, GO_RIGHT);
			nextL = RightLINK(nextL);											/* Might be tail */
		}
		SelectObject(pL);
	}
	UpdateSelection(doc);

	InvalMeasures(doc->selStartL, LeftLINK(doc->selEndL), ANYONE);	/* Erase current measures */

	quantum = Code2LDur((char)quantumDur, 0);
	tripletBias = (tuplets? -config.noTripletBias : -100);

	/* We're finally ready. Save selection range info, do the actual quantizing, then
		set the selection range to include everything we quantized. This may leave the
		selection range slightly too large (e.g., including an extraneous Measure at the
		start and end) and with anomalies; we count on GDRespAndRfmt to fix these. */
	
	saveEndL = doc->selEndL;
	
	didAnything = (QuantAllVoices(doc, quantum, tripletBias, FALSE)!=NILINK);

	doc->selStartL = startMeasL;
	doc->selEndL = saveEndL;
	
	if (didAnything) {
		if (config.delRedundantAccs) DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);

		if (RESET_PLAYDURS) SetPDur(doc, config.legatoPct, TRUE);
		ProgressMsg(RESPACE_PMSTR, "");
		GDRespAndRfmt(doc, quantumDur, ANYONE, FALSE);				/* Fixes selection range */
		ProgressMsg(0, "");
	}		

	return didAnything;

Trouble:
	CParamText(strBuf, "", "", "");
	StopInform(GUESSDUR_ALRT);
	return didAnything;
}


/* -------------------------------------------------------------- QuantizeDialog -- */

static GRAPHIC_POPUP	durPop0dot, *curPop;
static POPKEY			*popKeys0dot;
static short			popUpHilited=TRUE;

static enum
{
	QDURPOPUP_DI=4,				/* Item numbers */
	QTUPLET_DI,
	AUTOBEAM_DI=7
} E_QUANTITEMS;

static pascal Boolean QuantFilter(DialogPtr, EventRecord *, short *);
static pascal Boolean QuantFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr		w;
	register short	ch, ans;
	Point				where;
	GrafPtr			oldPort;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));			
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, OK, TRUE);
				DrawGPopUp(curPop);		
				HiliteGPopUp(curPop, popUpHilited);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return TRUE;
			}
			break;
		case activateEvt:
			if (w==GetDialogWindow(dlog))
				SetPort(GetDialogWindowPort(dlog));
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			if (PtInRect(where, &curPop->box)) {
				DoGPopUp(curPop);
				*itemHit = QDURPOPUP_DI;
				return TRUE;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, FALSE))
				return TRUE;
			ch = (unsigned char)evt->message;
			ans = DurPopupKey(curPop, popKeys0dot, ch);
			*itemHit = ans? QDURPOPUP_DI : 0;
			HiliteGPopUp(curPop, TRUE);
			return TRUE;
	}

	return FALSE;
}

Boolean QuantizeDialog(Document *doc,
								short *dur,
								Boolean *tuplet,
								Boolean *autoBeam)		/* Beam automatically? */
{
	register DialogPtr dlog;
	GrafPtr		oldPort;
	short			ditem, aShort, oldDur, startNum, endNum, newDur;
	Handle		aHdl;
	Rect			box;
	char			numStr1[15], numStr2[15];
	short			oldResFile, choice;
	Boolean		done, tups;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(QuantFilter);
	if (filterUPP == NULL) {
		MissingDialog(QUANTIZE_DLOG);
		return FALSE;
	}
	
	dlog = GetNewDialog(QUANTIZE_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(QUANTIZE_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);									/* popup code uses Get1Resource */
	
	startNum = GetMeasNum(doc, doc->selStartL);
	endNum = GetMeasNum(doc, LeftLINK(doc->selEndL));
	sprintf(numStr1, "%d", startNum);
	sprintf(numStr2, "%d", endNum);
	CParamText(numStr1, numStr2, "", "");

	newDur = *dur;

	durPop0dot.menu = NULL;										/* NULL makes any "goto Broken" safe */
	durPop0dot.itemChars = NULL;
	popKeys0dot = NULL;

	GetDialogItem(dlog, QDURPOPUP_DI, &aShort, &aHdl, &box);
	if (!InitGPopUp(&durPop0dot, TOP_LEFT(box), NODOTDUR_MENU, 1)) goto Broken;
	popKeys0dot = InitDurPopupKey(&durPop0dot);
	if (popKeys0dot==NULL) goto Broken;
	curPop = &durPop0dot;
		
	choice = GetDurPopItem(curPop, popKeys0dot, newDur, 0);
	if (choice==NOMATCH) choice = 1;
	SetGPopUpChoice(curPop, choice);

	PutDlgChkRadio(dlog,QTUPLET_DI,*tuplet);
	PutDlgChkRadio(dlog,AUTOBEAM_DI,*autoBeam);

	CenterWindow(GetDialogWindow(dlog), 60);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	oldDur = newDur;
	
	done = FALSE;
	while (!done) {
		ModalDialog(filterUPP, &ditem);
		oldDur = newDur;
		switch (ditem) {
		case OK:
			/* Unless secret key is down, don't allow triplets if the quantum
				is shorter than a sixteenth. This was to sidestep a bug that caused
				triplets containing unknown-duration notes, but I doubt whether this
				was enough to avoid the problem completely, and I think the bug has
				been fixed anyway (as of 1.0b14). */
				
			tups = GetDlgChkRadio(dlog,QTUPLET_DI);
			if (!(ControlKeyDown()) && tups && newDur>SIXTEENTH_L_DUR) {
				StopInform(TOO_FINE_QUANT_ALRT);
				done = FALSE;
				break;
			}
			
			*dur = newDur;
			*tuplet = GetDlgChkRadio(dlog,QTUPLET_DI);
			*autoBeam = GetDlgChkRadio(dlog,AUTOBEAM_DI);
			done = TRUE;
			break;
		case Cancel:
			done = TRUE;
			break;
		case QDURPOPUP_DI:
			newDur = popKeys0dot[curPop->currentChoice].durCode;
			HiliteGPopUp(curPop, popUpHilited = TRUE);
			break;
		case QTUPLET_DI:
			PutDlgChkRadio(dlog,QTUPLET_DI,!GetDlgChkRadio(dlog,QTUPLET_DI));
			break;
		case AUTOBEAM_DI:
			PutDlgChkRadio(dlog,AUTOBEAM_DI,!GetDlgChkRadio(dlog,AUTOBEAM_DI));
			break;
			}
		}
		
Broken:			
	DisposeGPopUp(&durPop0dot);
	if (popKeys0dot) DisposePtr((Ptr)popKeys0dot);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}


/* ------------------------------------------------------------------- DoQuantize -- */
/* Guess durations of notes/chords in the selection range as specified by user in
dialog. */
	
void DoQuantize(Document *doc)
{
	static short newDur = EIGHTH_L_DUR;
	static Boolean tuplet = FALSE, autoBeam = FALSE;

	if (QuantizeDialog(doc, &newDur, &tuplet, &autoBeam)) {
		PrepareUndo(doc, doc->selStartL, U_Quantize, 37);		/* "Undo Transcribe Recording" */
		if (QuantizeSelDurs(doc, newDur, tuplet)) {
			/* Start fixing timestamps before the range quantized so the Measure starting
				the range has the correct timestamp. */
			FixTimeStamps(doc, LeftLINK(doc->selStartL), doc->selEndL);

			if (autoBeam) {
				if (!AutoBeam(doc)) return;
				/* Set visibility of tuplet brackets to the conventional default: hide the
					bracket iff some beam has elements identical to the tuplet's. */
				SetBracketsVis(doc, doc->selStartL, doc->selEndL);
			}
		}
	}
}


/* ===================================================== Record/Transcribe Merge == */

/* --------------------------------------------------------- RTMQuantizeSelDurs -- */
/* Quantize the attack time and duration of every selected note/rest/chord
to a grid whose points are separated by the duration described by <quantumDur>
and merge by time with following Syncs. If there are pauses between the quantized
notes, intersperse rests as necessary. Return TRUE if successful. Intended for
the Record/Transcribe Merge command, this is analogous to QuantizeSelDurs, but
simpler mostly because this is intended to be called immediately after recording,
so the selection should contain only unknown-duration notes with no anomalies. On
the other hand, this is more complicated in one way: we can't insist here that
there be barlines on each side of the selection.

NB: leaves timestamps in an inconsistent state: the calling function should call
FixTimeStamps afterwards.*/

static Boolean RTMQuantizeSelDurs(Document *, short, Boolean, short);
static Boolean RTMQuantizeSelDurs(Document *doc, short quantumDur, Boolean tuplets,
												short staffn)
{
	short		quantum, tripletBias;
	LINK		saveStartL, lastL;

	quantum = Code2LDur((char)quantumDur, 0);
	tripletBias = (tuplets? -config.noTripletBias : -100);
	
	/* Save selection range info, do the actual quantizing and merging, then set
		the selection range to include everything we quantized. This may leave the
		selection range slightly too large (e.g., including an extraneous Measure at the
		start and end) and with anomalies; we count on GDRespAndRfmt to fix these. */
	
	saveStartL = LeftLINK(doc->selStartL);

	lastL = QuantAllVoices(doc, quantum, tripletBias, TRUE);
	
	doc->selStartL = RightLINK(saveStartL);
	
	/* Clean up, including respacing and reformatting, if we succeeded. */
	
	if (lastL) {
		doc->selEndL = RightLINK(lastL);
		if (config.delRedundantAccs) DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);

		if (RESET_PLAYDURS) SetPDur(doc, config.legatoPct, TRUE);
		ProgressMsg(RESPACE_PMSTR, "");
		GDRespAndRfmt(doc, quantumDur, staffn, TRUE);					/* Fixes selection range */
		ProgressMsg(0, "");
		return TRUE;
	}
	
	doc->selEndL = doc->selStartL;
	return FALSE;		
}

/* --------------------------------------------------------------- RecTransMerge -- */
/* Record, Transcribe, and Merge: Record data from MIDI and generate Nightingale
notes of unknown duration; transcribe them, i.e., identify their durations as
Transcribe Recording does; and merge them into the object list at the insertion
point. */

Boolean RecTransMerge(Document *doc)
{
	static short newDur = EIGHTH_L_DUR;
	static Boolean tuplet = FALSE, autoBeam = FALSE;
	short staffn;

	if (doc->selStartL!=doc->selEndL) {
		GetIndCString(strBuf, MIDIERRS_STRS, 6);		/* "Can't record without insertion pt" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	if (doc->lookVoice>=0 && doc->lookVoice!=doc->selStaff) {
		GetIndCString(strBuf, MIDIERRS_STRS, 20);		/* "Can't Record Merge in non-dflt voice" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	
	if (SyncNeighborTime(doc, doc->selStartL, TRUE)>=0L) {
		GetIndCString(strBuf, MIDIERRS_STRS, 21);		/* "Can Record Merge only at start of a measure" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

/* If the Measure where we're starting the merge begins with a time signature (other
than in the reserved area), RTMQuantizeSelDurs has big problems, and has for a long
time--this bug was already in Nightingale 2.0. Since as of this writing (2 Mar. 1996),
we're on the verge of shipping v. 3.0, just warn the poor user. Sigh. */

{	LINK pL;

	for (pL = doc->selStartL; pL && !MeasureTYPE(pL); pL = LeftLINK(pL))
		if (TimeSigTYPE(pL)) {
			if (CautionAdvise(RTM_TIMESIG_ALRT)==Cancel) return FALSE;
			break;
		}
}

	staffn = doc->selStaff;

	if (!RTMRecord(doc)) return FALSE;

	if (!QuantizeDialog(doc, &newDur, &tuplet, &autoBeam)) return FALSE;

	if (!RTMQuantizeSelDurs(doc, newDur, tuplet, staffn)) return FALSE;

	/* Start fixing timestamps before the range quantized so the Measure starting
		the range has the correct timestamp. */
	FixTimeStamps(doc, LeftLINK(doc->selStartL), doc->selEndL);

	if (autoBeam) {
		if (!AutoBeam(doc)) return FALSE;
		/* Set visibility of tuplet brackets to the conventional default: hide the
			bracket iff some beam has elements identical to the tuplet's. */
		SetBracketsVis(doc, doc->selStartL, doc->selEndL);
	}
	
	return TRUE;
}
