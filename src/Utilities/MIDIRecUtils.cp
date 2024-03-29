/* MIDIRecUtils.c for Nightingale

Utilities for handling the object list for MIDI recording and importing MIDI files.
These functions have no user interface implications.
	SetupMIDINote		NUICreateSync			NUIAddNoteToSync
	MIDI2HalfLn			FillMergeBuffer			ObjTimeInTable
	MRFixTieLinks		FixTupletLinks			MRMerge
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


static void SetupMIDINote(Document *, LINK, LINK, MNOTE, CONTEXT, SignedByte, short,
							short, SignedByte, Boolean, long);
static short MIDI2HalfLn(Document *, Byte, short, Boolean, SignedByte *);
static short ObjTimeInTable(long, LINKTIMEINFO *, short);
static void MRFixTieLinks(Document *, LINK, LINK, short);
static void MRFixTupletLinks(Document *, LINK, LINK, short);


/* --------------------------------------------------------------------- SetupMIDINote -- */
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
							long /*timeShift*/					/* obsolete and ignored */
							)
{
	PANOTE		aNote;
	SHORTQD		yqpit;
	short      	halfLn, midCHalfLn, qStemLen;
	SignedByte	acc;
	Boolean		stemDown;

PushLock(NOTEheap);
	aNote = GetPANOTE(aNoteL);
	
	aNote->staffn = staffn;
	aNote->voice = voice;									/* Must set before calling GetStemInfo */
	aNote->selected = False;
	aNote->visible = True;
	aNote->soft = False;
	aNote->xd = 0;
	aNote->rest = isRest;

	if (isRest) {
		halfLn = 0;
		yqpit = halfLn2qd(context.staffLines-1);
		aNote->yd = qd2d(yqpit, context.staffHeight,
						context.staffLines);
		aNote->yqpit = -1;										/* No QDIST pitch */
		aNote->ystem = 0;										/* No stem end pos. */
		aNote->noteNum = 0;										/* No MIDI note number */
		aNote->accident = 0;									/* No accidental */
	}
	else {
		midCHalfLn = ClefMiddleCHalfLn(context.clefType);
		halfLn = MIDI2HalfLn(doc, MIDINote.noteNumber, midCHalfLn, recordFlats, &acc);
		yqpit = halfLn2qd(halfLn);
		aNote->yd = qd2d(yqpit, context.staffHeight, context.staffLines);
		aNote->yqpit = yqpit-halfLn2qd(midCHalfLn);
		stemDown = GetStemInfo(doc, syncL, aNoteL, &qStemLen);
		aNote->ystem = CalcYStem(doc, aNote->yd, NFLAGS(lDur), stemDown,
											context.staffHeight, context.staffLines,
											qStemLen, False);
		aNote->accident = acc;
		
		aNote->noteNum = MIDINote.noteNumber;
	}
		
	aNote->onVelocity = MIDINote.onVelocity;
	aNote->offVelocity = MIDINote.offVelocity;
	aNote->subType = lDur;

	aNote->ndots = ndots;
	aNote->xMoveDots = 3;
	aNote->yMoveDots = (halfLn%2==0 ? 1 : 2);

	aNote->playTimeDelta = 0;								/* For the moment */
	if (lDur>UNKNOWN_L_DUR)									/* Known dur. & not multibar rest? */
		aNote->playDur = SimplePlayDur(aNoteL);
	else
		aNote->playDur = MIDINote.duration;
	aNote->pTime = 0;										/* Used by Tuplet routines */

	aNote->inChord = False;
	aNote->unpitched = False;
	aNote->beamed = False;
	aNote->otherStemSide = False;
	aNote->rspIgnore = False;	
	aNote->accSoft = True;
	aNote->playAsCue = False;
	aNote->micropitch = 0;
	aNote->xmoveAcc = DFLT_XMOVEACC;
	aNote->courtesyAcc = 0;
	aNote->doubleDur = False;
	aNote->headShape = NORMAL_VIS;
	aNote->firstMod = NILINK;
	aNote->tiedR = aNote->tiedL = False;
	aNote->slurredR = aNote->slurredL = False;
	aNote->inTuplet = False;
	aNote->inOttava = False;
	aNote->small = False;
	aNote->tempFlag = False;
	aNote->artHarmonic = 0;
	aNote->userID = 0;
//	for (short k = 0; k<4; k++) aNote->segments[k] = 0;
	aNote->reservedN = 0L;
		
PopLock(NOTEheap);
}


/* --------------------------------------------------------------------- NUICreateSync -- */
/* Create a new Sync at the insertion point containing one note. */

LINK NUICreateSync(Document *doc,
					MNOTE MIDINote,
					LINK *syncL,
					SignedByte staffn,
					short lDur, short ndots,
					SignedByte voice,
					Boolean isRest,
					long timeShift						/* startTime offset for timeStamp */
					)
{
	LINK	newpL, aNoteL;
	CONTEXT context;
			 
	newpL = InsertNode(doc, doc->selStartL, SYNCtype, 1);
	if (newpL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}
	else {
		GetContext(doc, newpL, staffn, &context);
		SetObject(newpL, 0, 0, False, True, False);
		doc->changed = doc->used = LinkSEL(newpL) = True;
		LinkTWEAKED(newpL) = False;
		aNoteL = FirstSubLINK(newpL);
		SetupMIDINote(doc, newpL, aNoteL, MIDINote, context, staffn, lDur, ndots,
							voice, isRest, timeShift);
		NoteSEL(aNoteL) = True;
		SyncTIME(newpL) = MIDINote.startTime-timeShift;
		*syncL = newpL;
	}

	return aNoteL;
}


/* ------------------------------------------------------------------ NUIAddNoteToSync -- */
/* Add a note to the given Sync. */

LINK NUIAddNoteToSync(Document *doc, MNOTE MIDINote, LINK syncL, SignedByte staffn, short
						lDur, short ndots, SignedByte voice, Boolean isRest, long timeShift)
{
	CONTEXT	context;
	LINK	aNoteL;
	Boolean	inVoice;
				 
	inVoice = SyncInVoice(syncL, voice);
	GetContext(doc, syncL, staffn, &context);			/* ??COULD REUSE CreateSync's */
	if (!ExpandNode(syncL, &aNoteL, 1)) {
		NoMoreMemory();
		return NILINK;
	}

	SetupMIDINote(doc, syncL, aNoteL, MIDINote, context, staffn, lDur, ndots,
						voice, isRest, timeShift);	 	
	NoteSEL(aNoteL) = True;
	
	/* If there was already a note in this note's voice, it's now a chord. */
	
	if (inVoice) FixSyncForChord(doc, syncL, voice, False, 0, 0, &context);

	return aNoteL;
}


/* ----------------------------------------------------------------------- MIDI2HalfLn -- */
/* Given MIDI note number <noteNum> and half-line position of middle C <midCHalfLn>,
return the note's half-line number as function value (where top line of staff = 0??)
and the accidental in <*pAcc>. */

static short MIDI2HalfLn(Document */*doc*/, Byte noteNum, short midCHalfLn,
								Boolean useFlats, SignedByte *pAcc)
{
	short pitchClass, halfLines, octave, halfSteps;
				
	static short hLinesTable[] =
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
	if (!MeasureTYPE(pL)) return False;
	
	return !FirstMeasInSys(pL);
}


/* ------------------------------------------------------------------- FillMergeBuffer -- */
/* Fill one of MRMerge's buffers with objects from <startL> to the end of the score,
but at most <maxObjs> objects. If there are more objects than that, return <maxObjs+1>
(in which case only the first <maxObjs> go into the buffer), else return the number
of objects.

The objects always include Syncs, plus optionally Measures, but only Measures that don't
begin Systems, since MRMerge may put Syncs before Measures. */

short FillMergeBuffer(
				LINK startL,
				LINKTIMEINFO mergeObjs[],
				short maxObjs,
				Boolean syncsOnly		/* True=include Syncs only, else Syncs and Measures */
				)
{
	short i;  LINK pL;
	
	for (i = 0, pL = startL; i<maxObjs && pL; pL = RightLINK(pL))
		if (SyncTYPE(pL) || (!syncsOnly && IsMeasAndMergable(pL))) {
			mergeObjs[i].link = pL;
			mergeObjs[i].time = ( SyncTYPE(pL)? SyncAbsTime(pL) : MeasureTIME(pL) );
			i++;
		}
	return i;
}


/* -------------------------------------------------------------------- ObjTimeInTable -- */
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


/* --------------------------------------------------------------------- MRFixTieLinks -- */
/* In the range [startL,endL), fix the firstSyncL and lastSyncL of every tie-subtype
Slur in the given voice to agree with the order of objects in the object list, i.e.,
set the firstSyncL to the next Sync in voice and the lastSyncL to the next Sync after
that. */

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
				MayErrMsg("Cross-system Slur %ld  (MRFixTieLinks)", (long)pL);
				continue;
			}
			firstSyncL = LVSearch(pL, SYNCtype, voice, GO_RIGHT, False);
			SlurFIRSTSYNC(pL) = firstSyncL;
			lastSyncL = LVSearch(RightLINK(firstSyncL), SYNCtype, voice,
										GO_RIGHT, False);
			SlurLASTSYNC(pL) = lastSyncL;
		}
	
}


/* ------------------------------------------------------------------ MRFixTupletLinks -- */
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
				syncL = LVSearch(RightLINK(syncL), SYNCtype, voice, GO_RIGHT, False);
				aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
				aNoteTuple->tpSync = syncL;
			}
		}
}


/* --------------------------------------------------------------------------- MRMerge -- */
/* Merge objects in the object list described by <newSyncs> into the object list by
time. The range [newSyncs[0], newSyncs[nNewSyncs-1]) should not contain any content
objects other than Slurs of subtype tie, Tuplets, and Syncs, and all of them should be
in <voice>; the range may also contain structure objects, which will be discarded.
Also, if <voice> is used in the area we're merging into, the results will be a bit
strange, though not disasterously so.

If we succeed, or if there's nothing to do, return True; if not (probably because of
lack of memory), False. In the latter case the object list will probably be a mess
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
			LINK *pLastL				/* Last affected LINK, if any */
			)
{
	LINK startL, endL, beforeStartL, newL, matchL, copyL, insL, selEndL,
			slurL, measL, pL, tupL, lastL;
	short match, i;
	
	*pLastL = NILINK;
	if (nNewSyncs<=0) return True;			/* If nothing to do */
	if (nMergeObjs<=0) return True;			/* Should never happen, but just in case */

	startL = newSyncs[0].link;
	while (J_DTYPE(LeftLINK(startL)))		/* In case the first Sync is tied or inTuplet */
		startL = LeftLINK(startL);
	endL = RightLINK(newSyncs[nNewSyncs-1].link);
	beforeStartL = LeftLINK(startL);
	
#ifdef MRDEBUG
for (i = 0; i<nNewSyncs; i++) {
	LogPrintf(LOG_DEBUG, "newSyncs[%d].link=%d time=%ld\n",
	i, newSyncs[i].link, newSyncs[i].time); }
for (i = 0; i<n_min(nMergeObjs, 5); i++) {
	LogPrintf(LOG_DEBUG, "mergeObjs[%d].link=%d time=%ld\n",
	i, mergeObjs[i].link, mergeObjs[i].time); }
#endif

	for (i = 0; i<nNewSyncs; i++) {
		newL = newSyncs[i].link;
		
		/* Look for a slur and a tuplet in <voice> that begin with newL; if they exist,
		   they must be moved along with newL and must have their fields that refer to
		   Syncs updated. */
		   
		slurL = NILINK;
		pL = LeftLINK(newL);
		for ( ; pL && J_DTYPE(pL); pL = LeftLINK(pL))
			if (SlurTYPE(pL) && SlurVOICE(pL)==voice) { slurL = pL; break; }
		if (slurL && !SlurTIE(slurL)) {
			MayErrMsg("Non-tie slur found at %ld.  (MRMerge)", (long)slurL);
			slurL = NILINK;
		}

		tupL = NILINK;
		pL = LeftLINK(newL);
		for ( ; pL && J_DTYPE(pL); pL = LeftLINK(pL))
			if (TupletTYPE(pL) && TupletVOICE(pL)==voice) { tupL = pL; break; }

		match = ObjTimeInTable(newSyncs[i].time, mergeObjs, nMergeObjs);
		if (match>=0) {
			matchL = mergeObjs[match].link;
			
			/* If we have an exact match in time with a Sync, newL should be merged
			   with it. If there's a voice conflict, we should presumably fix chord flags
			   and so on, but that's not worth bothering with since calling functions
			   should prevent it ever happening anyway, so just avoid disaster in such
			   a case by not merging. */
			   
			if (mergeObjs[match].time==newSyncs[i].time && SyncTYPE(matchL)
			&&  !SyncInVoice(matchL, voice)) {
				copyL = DuplicateObject(SYNCtype, newL, False, doc, doc, False);
				if (!copyL) return False;					/* A serious problem for caller */
				MergeObject(doc, copyL, matchL);
				LinkSEL(matchL) = True;
				selEndL = RightLINK(matchL);
				if (slurL) MoveNode(slurL, matchL);			/* Must fix firstSyncL later */
				if (tupL) MoveNode(tupL, matchL);			/* Must fix subobj tpSyncs later */
				lastL = matchL;
			}
			else {

				/* For whatever reason, we can't merge newL into matchL. Instead,
				   newL must go after matchL--more specifically, just before the
				   mergeable object after matchL. */

				insL = (match<nMergeObjs-1? mergeObjs[match+1].link : doc->tailL);
				insL = FindInsertPt(insL);
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
		
		measL = LSSearch(newL, MEASUREtype, ANYONE, GO_LEFT, False);
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
	return True;
}
