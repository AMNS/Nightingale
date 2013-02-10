/* MIDIRecUtils.c for Nightingale, small rev. for v.3.1

Utilities for handling the object list for MIDI recording and importing MIDI files.
These functions have no user interface implications.
	SetupMIDINote		CreateSync				AddNoteToSync
	MIDI2HalfLn			FillMergeBuffer		ObjTimeInTable
	MRFixTieLinks		FixTupletLinks			MRMerge
	AvoidUnisons		AvoidUnisonsInRange
*/				

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


static void SetupMIDINote(Document *, LINK, LINK, MNOTE, CONTEXT, SignedByte, short, short,
							SignedByte, Boolean, long);
static short MIDI2HalfLn(Document *, Byte, short, Boolean, SignedByte *);
static short ObjTimeInTable(long, LINKTIMEINFO *, short);
static void MRFixTieLinks(Document *, LINK, LINK, short);
static void MRFixTupletLinks(Document *, LINK, LINK, short);
Boolean AvoidUnisons(Document *, LINK, short, PCONTEXT);


/* ---------------------------------------------------------------- SetupMIDINote -- */
/* Setup a vanilla (what some people call "B-flat") note or rest, presumably
entered via MIDI or MIDI File, so its logical duration may not be known and its
accidental must be inferred as best we can. lDur may be UNKNOWN_L_DUR or any
larger value, i.e., it cannot be WHOLEMR_L_DUR or below. */

static void SetupMIDINote(Document *doc,
									LINK syncL, LINK aNoteL,
									MNOTE MIDINote,
									CONTEXT context,
									SignedByte staffn,
									short lDur, short ndots,
									SignedByte voice,
									Boolean isRest,
									long /*timeShift*/						/* obsolete and ignored */
									)
{
	register PANOTE aNote;
	SHORTQD		yqpit;
	short      	halfLn, midCHalfLn, qStemLen;
	SignedByte	acc;
	Boolean		stemDown;

PushLock(NOTEheap);
	aNote = GetPANOTE(aNoteL);
	
	aNote->staffn = staffn;
	aNote->voice = voice;										/* Must set before calling GetStemInfo */
	aNote->selected = FALSE;
	aNote->visible = TRUE;
	aNote->soft = FALSE;
	aNote->xd = 0;
	aNote->rest = isRest;
	if (isRest) {
		halfLn = 0;
		yqpit = halfLn2qd(context.staffLines-1);
		aNote->yd = qd2d(yqpit, context.staffHeight,
						context.staffLines);
		aNote->yqpit = -1;												/* No QDIST pitch */
		aNote->accident = 0;												/* No accidental */
		aNote->noteNum = 0;												/* No MIDI note number */
		aNote->ystem = 0;													/* No stem end pos. */
	}
	else {
		midCHalfLn = ClefMiddleCHalfLn(context.clefType);
		halfLn = MIDI2HalfLn(doc, MIDINote.noteNumber, midCHalfLn, recordFlats, &acc);
		yqpit = halfLn2qd(halfLn);
		aNote->yd = qd2d(yqpit, context.staffHeight, context.staffLines);
		aNote->yqpit = yqpit-halfLn2qd(midCHalfLn);
		stemDown = GetStemInfo(doc, syncL, aNoteL, &qStemLen);
		aNote->ystem = CalcYStem(doc, aNote->yd, NFLAGS(lDur),
											stemDown,
											context.staffHeight, context.staffLines,
											qStemLen, FALSE);
		aNote->accident = acc;
		
		aNote->noteNum = MIDINote.noteNumber;
	}
		
	aNote->onVelocity = MIDINote.onVelocity;
	aNote->offVelocity = MIDINote.offVelocity;
	aNote->subType = lDur;
	aNote->ndots = ndots;
	aNote->xmovedots = 3;
	aNote->ymovedots = (halfLn%2==0 ? 1 : 2);

	aNote->inChord = FALSE;
	aNote->unpitched = FALSE;
	aNote->beamed = FALSE;
	aNote->otherStemSide = FALSE;
	aNote->accSoft = TRUE;
	aNote->micropitch = 0;
	aNote->xmoveAcc = DFLT_XMOVEACC;
	aNote->headShape = NORMAL_VIS;
	aNote->courtesyAcc = 0;
	aNote->doubleDur = FALSE;
	aNote->firstMod = NILINK;
	aNote->tiedR = aNote->tiedL = FALSE;
	aNote->slurredR = aNote->slurredL = FALSE;
	aNote->inTuplet = FALSE;
	aNote->inOctava = FALSE;
	aNote->small = FALSE;
	aNote->tempFlag = FALSE;
	aNote->fillerN = 0;

	aNote->playTimeDelta = 0;											/* For the moment */

	if (lDur>UNKNOWN_L_DUR)												/* Known dur. & not MB rest? */
		aNote->playDur = SimplePlayDur(aNoteL);
	else
		aNote->playDur = MIDINote.duration;

	aNote->pTime = 0;														/* Used by Tuplet routines */
		
PopLock(NOTEheap);
}


/* ------------------------------------------------------------------- CreateSync -- */
/* Create a new Sync at the insertion point containing one note. */

LINK CreateSync(register Document *doc,
						MNOTE MIDINote,
						LINK *syncL,
						SignedByte staffn,
						short lDur, short ndots,
						SignedByte voice,
						Boolean isRest,
						long timeShift							/* startTime offset for timeStamp */
						)
{
	register LINK	newpL, aNoteL;
	CONTEXT 			context;
			 
	newpL = InsertNode(doc, doc->selStartL,SYNCtype,1);
	if (newpL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}
	else {
		GetContext(doc, newpL, staffn, &context);
		SetObject(newpL, 0, 0, FALSE, TRUE, FALSE);
		doc->changed = doc->used = LinkSEL(newpL) = TRUE;
		LinkTWEAKED(newpL) = FALSE;
		aNoteL = FirstSubLINK(newpL);
		SetupMIDINote(doc, newpL, aNoteL, MIDINote, context, staffn, lDur, ndots,
							voice, isRest, timeShift);
		NoteSEL(aNoteL) = TRUE;
		SyncTIME(newpL) = MIDINote.startTime-timeShift;
		*syncL = newpL;
	}

	return aNoteL;
}


/* ---------------------------------------------------------------- AddNoteToSync -- */
/* Add a note to the given Sync. */

LINK AddNoteToSync(Document *doc, MNOTE MIDINote, LINK syncL, SignedByte staffn, short lDur,
							short ndots, SignedByte voice, Boolean isRest, long timeShift)
{
	CONTEXT  context;
	LINK		aNoteL;
	Boolean	inVoice;
				 
	inVoice = SyncInVoice(syncL, voice);
	GetContext(doc, syncL, staffn, &context);			/* ??COULD REUSE CreateSync's */
	if (!ExpandNode(syncL, &aNoteL, 1)) {
		NoMoreMemory();
		return NILINK;
	}

	SetupMIDINote(doc, syncL, aNoteL, MIDINote, context, staffn, lDur, ndots,
						voice, isRest, timeShift);	 	
	NoteSEL(aNoteL) = TRUE;
	
	/* If there was already a note in this note's voice, it's now a chord. */
	if (inVoice) FixSyncForChord(doc, syncL, voice, FALSE, 0, 0, &context);
	return aNoteL;
}


/* ------------------------------------------------------------------ MIDI2HalfLn -- */
/* Given MIDI note number <noteNum> and half-line position of middle C
<midCHalfLn>, return the note's half-line number as function value (where
top line of staff = 0??) and the accidental in <*pAcc>. */

static short MIDI2HalfLn(Document */*doc*/, Byte noteNum, short midCHalfLn,
								Boolean useFlats, SignedByte *pAcc)
{
	short		pitchClass,
				halfLines,
				octave,
				halfSteps;
				
	static short	hLinesTable[] =
		/* pitchclass	  C  C# D  D# E  F  F# G  G# A  A# B	*/				
							{ 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };	
	
	pitchClass = noteNum % 12;					
	halfSteps = hLinesTable[pitchClass];
	octave = (noteNum/12) - 5;
	halfLines = octave * 7 + halfSteps;
	
	if (pitchClass!=0 && halfSteps==hLinesTable[pitchClass-1]) {		/* Black key note? */
		if (useFlats) {
			halfLines += 1;
			*pAcc = AC_FLAT;
		}
		else
			*pAcc = AC_SHARP;
	}
	else 	
		*pAcc = AC_NATURAL;
	
	return (-halfLines+midCHalfLn);
}
		

static Boolean IsMeasAndMergable(LINK);
static Boolean IsMeasAndMergable(LINK pL)
{
	if (!MeasureTYPE(pL)) return FALSE;
	
	return !FirstMeasInSys(pL);
}

/* -------------------------------------------------------------- FillMergeBuffer -- */
/* Fill one of MRMerge's buffers with objects from <startL> to the end of the score,
but at most <maxObjs> objects. If there are more objects than that, return <maxObjs+1>
(in which case only the first <maxObjs> go into the buffer), else return the number
of objects.

The objects always include Syncs, plus optionally Measures, but only Measures that
don't begin Systems, since MRMerge may put Syncs before Measures. */

short FillMergeBuffer(
				LINK startL,
				LINKTIMEINFO mergeObjs[],
				short maxObjs,
				Boolean syncsOnly		/* TRUE=include Syncs only, else Syncs and Measures */
				)
{
	short i; LINK pL;
	
	for (i = 0, pL = startL; i<maxObjs && pL; pL = RightLINK(pL))
		if (SyncTYPE(pL) || (!syncsOnly && IsMeasAndMergable(pL))) {
			mergeObjs[i].link = pL;
			mergeObjs[i].time = ( SyncTYPE(pL)? SyncAbsTime(pL) : MeasureTIME(pL) );
			i++;
		}
	return i;
}


/* --------------------------------------------------------------- ObjTimeInTable -- */
/* Return the index of the last object (Sync or Measure) in the given LINKTIMEINFO
table whose absolute time is no greater than <lTime>. (If, for example, we have
several empty Measures, then a Measure containing several Syncs, all the Measures
and the first Sync will have the same time but we want to merge with the Sync.) If
everything in the table has time greater than <lTime>, return -1. */

static short ObjTimeInTable(long lTime, LINKTIMEINFO *mergeObjs, short nObjs)
{
	short i;
	
	for (i = 0; i<nObjs; i++)
		if (mergeObjs[i].time>lTime)
			return i-1;
	
	return nObjs-1;
}

/* ---------------------------------------------------------------- MRFixTieLinks -- */
/* In the range [startL,endL), fix the firstSyncL and lastSyncL of every tie-
subtype Slur in the given voice to agree with the order of objects in the data
structure, i.e., set the firstSyncL to the next Sync in voice and the lastSyncL
to the next Sync after that. */

static void MRFixTieLinks(
						Document */*doc*/,
						LINK startL, LINK endL,			/* Range to fix */
						short voice
						)
{
	LINK pL, firstSyncL, lastSyncL;
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SlurTYPE(pL) && SlurTIE(pL) && SlurVOICE(pL)==voice) {
			if (SlurCrossSYS(pL)) {
				MayErrMsg("MRFixTieLinks: cross-system Slur %ld", (long)pL);
				continue;
			}
			firstSyncL = LVSearch(pL, SYNCtype, voice, GO_RIGHT, FALSE);
			SlurFIRSTSYNC(pL) = firstSyncL;
			lastSyncL = LVSearch(RightLINK(firstSyncL), SYNCtype, voice,
										GO_RIGHT, FALSE);
			SlurLASTSYNC(pL) = lastSyncL;
		}
	
}


/* ------------------------------------------------------------- MRFixTupletLinks -- */
/* In the range [startL,endL), fix the tpSyncs of every Tuplet to agree with the order
of objects in the data structure, i.e., set the first tpSync to the next Sync in its
voice, the next tpSync to the next Sync after that, etc. */

static void MRFixTupletLinks(
						Document */*doc*/,
						LINK startL, LINK endL,
						short voice)
{
	LINK pL, syncL;
	LINK aNoteTupleL;
	PANOTETUPLE aNoteTuple;
	short i, nInTuplet;	
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (TupletTYPE(pL) && TupletVOICE(pL)==voice) {
			nInTuplet = LinkNENTRIES(pL);
			aNoteTupleL = FirstSubLINK(pL);
			syncL = pL;
			for (i=0; i<nInTuplet; i++, aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
				syncL = LVSearch(RightLINK(syncL), SYNCtype, voice, GO_RIGHT, FALSE);
				aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
				aNoteTuple->tpSync = syncL;
			}
		}
	
}

/* ---------------------------------------------------------------------- MRMerge -- */
/* Merge objects in the object list described by <newSyncs> into the object list by
time. The range [newSyncs[0], newSyncs[nNewSyncs-1]) should not contain any content
objects other than Slurs of subtype tie, Tuplets, and Syncs, and all of them should be
in <voice>; the range may also contain structure objects, which will be discarded.
Also, if <voice> is used in the area we're merging into, the results will be a bit
strange, though not disasterously so.

If we succeed, or if there's nothing to do, return TRUE; if not (probably because of
lack of memory), FALSE. In the latter case the object list will probably be a mess
and the caller had better do something drastic--probably inform the user and close the
document. (It'd be nice for this function to try to avoid that by checking there's
plenty of memory before doing anything, but it'd be difficult to estimate how much
it'll need--it'd have to guess.)

MRMerge depends on timing information on Syncs and Measures in newSyncs and in
mergeObjs (NOT timing information in the object list!). For example, newSyncs[0].time
and mergeObjs[0].time should normally be identical. */

Boolean MRMerge(
			Document *doc,
			short voice,
			LINKTIMEINFO *newSyncs,		/* Info about range to merge (e.g., from FillMergeBuffer) */
			short nNewSyncs,
			LINKTIMEINFO *mergeObjs,	/* Info about range to merge into (e.g, from FillMergeBuffer) */
			short nMergeObjs,
			LINK *pLastL					/* Last affected LINK, if any */
			)
{
	LINK startL, endL, beforeStartL, newL, matchL, copyL, insL, selEndL,
			slurL, measL, pL, tupL, lastL;
	short match, i;
	
	*pLastL = NILINK;
	if (nNewSyncs<=0) return TRUE;			/* If nothing to do */
	if (nMergeObjs<=0) return TRUE;			/* Should never happen, but just in case */

	startL = newSyncs[0].link;
	while (J_DTYPE(LeftLINK(startL)))		/* In case the first Sync is tied or inTuplet */
		startL = LeftLINK(startL);
	endL = RightLINK(newSyncs[nNewSyncs-1].link);
	beforeStartL = LeftLINK(startL);
	
#ifdef MRDEBUG
for (i = 0; i<nNewSyncs; i++) {
	DebugPrintf("newSyncs[%d].link=%d time=%ld\n",
	i, newSyncs[i].link, newSyncs[i].time); }
for (i = 0; i<n_min(nMergeObjs, 5); i++) {
	DebugPrintf("mergeObjs[%d].link=%d time=%ld\n",
	i, mergeObjs[i].link, mergeObjs[i].time); }
#endif

	for (i = 0; i<nNewSyncs; i++) {
		newL = newSyncs[i].link;
		
		/*
		 * Look for a slur and a tuplet in <voice> that begin with newL; if they exist,
		 *	they must be moved along with newL and must have their fields that refer to
		 * Syncs updated.
		 */
		slurL = NILINK;
		pL = LeftLINK(newL);
		for ( ; pL && J_DTYPE(pL); pL = LeftLINK(pL))
			if (SlurTYPE(pL) && SlurVOICE(pL)==voice) { slurL = pL; break; }
		if (slurL && !SlurTIE(slurL)) {
			MayErrMsg("MRMerge: non-tie slur found at %ld.", (long)slurL);
			slurL = NILINK;
		}

		tupL = NILINK;
		pL = LeftLINK(newL);
		for ( ; pL && J_DTYPE(pL); pL = LeftLINK(pL))
			if (TupletTYPE(pL) && TupletVOICE(pL)==voice) { tupL = pL; break; }

		match = ObjTimeInTable(newSyncs[i].time, mergeObjs, nMergeObjs);
		if (match>=0) {
			matchL = mergeObjs[match].link;
			/*
			 * If we have an exact match in time with a Sync, newL should be merged
			 * with it. If there's a voice conflict, we should presumably fix chord flags
			 * and so on, but that's not worth bothering with since calling functions
			 * should prevent it ever happening anyway, so just avoid disaster in such
			 * a case by not merging.
			 */
			if (mergeObjs[match].time==newSyncs[i].time && SyncTYPE(matchL)
			&&  !SyncInVoice(matchL, voice)) {
				copyL = DuplicateObject(SYNCtype, newL, FALSE, doc, doc, FALSE);
				if (!copyL) return FALSE;					/* A serious problem for caller */
				MergeObject(doc, copyL, matchL);
				LinkSEL(matchL) = TRUE;
				selEndL = RightLINK(matchL);
				if (slurL) MoveNode(slurL, matchL);		/* Must fix firstSyncL later */
				if (tupL) MoveNode(tupL, matchL);		/* Must fix subobj tpSyncs later */
				lastL = matchL;
			}
			else {
				/*
				 * For whatever reason, we can't merge newL into matchL. Instead,
				 * newL must go after matchL--more specifically, just before the
				 *	mergeable object after matchL.
				 */
				insL = (match<nMergeObjs-1? mergeObjs[match+1].link : doc->tailL);
				insL = LocateInsertPt(insL);
				MoveNode(newL, insL);
				selEndL = insL;
				if (slurL) MoveNode(slurL, newL);
				if (tupL) MoveNode(tupL, newL);
				lastL = newL;
			}
		}
		else {
			MoveNode(newL, doc->tailL);
			selEndL = doc->tailL;
			lastL = newL;
		}
		
		/* Update newL's time. This could probably be done much more efficiently. */
		measL = LSSearch(newL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
		SyncTIME(newL) = newSyncs[i].time-MeasureTIME(measL);
	}

	/*
	 * All the notes are now where they belong. Get rid of anything that's left in
	 * the range originally recorded and fix firstSyncL/lastSyncL of all ties that
	 *	might be affected.
	 */
	DeleteRange(doc, RightLINK(beforeStartL), endL);
	MRFixTieLinks(doc, endL, mergeObjs[nMergeObjs-1].link, voice);
	MRFixTupletLinks(doc, endL, mergeObjs[nMergeObjs-1].link, voice);
	
	*pLastL = lastL;
	doc->selStartL = endL;
	doc->selEndL = selEndL;
	return TRUE;
}


/* ----------------------------------------------------------------- AvoidUnisons -- */
/* In the given chord, try to avoid any kind of unisons by respelling. Returns FALSE
if there's at least one unison it can't get rid of. Assumes the chord doesn't con-
tain any fancy spellings (see below).  Handles only chords of normal notes, not
grace notes.*/

Boolean AvoidUnisons(Document *doc, LINK syncL, short voice, PCONTEXT pContext)
{
	short			noteCount, i, unisonCount;
	CHORDNOTE	chordNote[MAXCHORD];
	short			halfSpTab[MAXCHORD+1];
	PANOTE		aNote, bNote;
	Boolean		thisChanged, anyChanged;

	/*
	 *	Sort notes by note number and go thru them by y-position, making a table of
	 * y-position intervals. (We sort by note number instead of y-position to be
	 * sure notes in unisons are in the correct order; this could backfire if fancy
	 * enharmonic spellings were involved--say, E# and Fb--but that can't happen with
	 * newly-recorded notes.) For our purposes, it makes no difference whether we
	 * get the notes in descending or ascending order; arbitrarily choose ascending.
	 */
	noteCount = PSortChordNotes(syncL, voice, TRUE, chordNote);
	
	for (i = 1; i<noteCount; i++) {
		aNote = GetPANOTE(chordNote[i].noteL);
		bNote = GetPANOTE(chordNote[i-1].noteL);
		halfSpTab[i] = qd2halfLn(ABS(aNote->yqpit-bNote->yqpit));
	}

	halfSpTab[0] = halfSpTab[noteCount] = 999;
	
	for (anyChanged = FALSE, unisonCount = 0, i = 1; i<noteCount; i++) {
		if (halfSpTab[i]==0) {
		/*
		 *	Found a unison. Try to avoid it by changing the first note of the two (the
		 * lower-pitched) to an enharmonic equivalent on the line/space below its current
		 * position, if this would not CREATE a unison in that position; if it would, or
		 * if we have no equivalent on the line/space below, leave the first note alone
		 * and try to change the second note to an enharmonic equivalent on the line/space
		 * above its current position. If this would create a unison in that position,
		 * or if we have no equivalent on the line/space above, give up. I suspect there
		 * are cases where this will fail but just considering the notes in a different
		 * order would work. Oh well.
		 */
		 	thisChanged = FALSE;
			if (halfSpTab[i-1]>1)
				if (RespellNote(doc, syncL, chordNote[i-1].noteL, pContext))
					thisChanged = anyChanged = TRUE;
			if (halfSpTab[i+1]>1 && !thisChanged)
				if (RespellNote(doc, syncL, chordNote[i].noteL, pContext))
					thisChanged = anyChanged = TRUE;
			if (!thisChanged)
					unisonCount++;
		}
	}
	
	if (anyChanged) FixRespelledVoice(doc, syncL, voice);
	return (unisonCount==0);
}


/* ---------------------------------------------------------- AvoidUnisonsInRange -- */
/* In the range [startL,endL), try to avoid by respelling any kind of unisons
(perfect or augmented) in chords on the given staff. Handles only chords of normal
notes, not grace notes. */

void AvoidUnisonsInRange(Document *doc, LINK startL, LINK endL, short staff)
{
	CONTEXT context; short voice, relStaff; LINK pL, partL;
	PPARTINFO pPart; short nProblems=0;
	char fmtStr[256];
	
	/*
	 * Using the context before the first Measure is dangerous and there can't be
	 * any Syncs there, anyway, so if <startL> is before the first Measure,
	 * instead start at the first Measure.
	 */
	if (!SSearch(startL, MEASUREtype, GO_LEFT))
		startL = SSearch(startL, MEASUREtype, GO_RIGHT);

	GetContext(doc, startL, staff, &context);
	voice = USEVOICE(doc, staff);
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL))
			if (!AvoidUnisons(doc, pL, voice, &context))
				nProblems++;
	
	if (nProblems>0) {
		partL = FindPartInfo(doc, Staff2Part(doc, staff));
		pPart = GetPPARTINFO(partL);
		relStaff = staff-pPart->firstStaff+1;
		GetIndCString(fmtStr, MIDIERRS_STRS, 22);				/* "Nightingale couldn't avoid unisons in %d chord(s) on staff %d " */
		sprintf(strBuf, fmtStr, nProblems, staff); 
		if (pPart->lastStaff>pPart->firstStaff) {
			GetIndCString(fmtStr, MIDIERRS_STRS, 23);			/* "(staff %d of %s)." */
			sprintf(&strBuf[strlen(strBuf)], fmtStr, relStaff, pPart->name); 
		}
		else {
			GetIndCString(fmtStr, MIDIERRS_STRS, 24);			/* "(%s)." */
			sprintf(&strBuf[strlen(strBuf)], fmtStr, pPart->name); 
		}
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
}
