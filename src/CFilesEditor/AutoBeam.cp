/* File AutoBeam.c - automatic beaming functions, original versions by Ray Spears,
rewritten by Charlie Rose and Don Byrd. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Copyright © 2016 by Avian Music Notation Foundation.
 * All Rights Reserved.
 *  
 *  Nightingale is an open-source project, hosted at github.com/AMNS/Nightingale .
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define StartAtRest 1
#define CROSSRESTS 2
#define StemsToRests 4
#define CrossBars 8
#define CrossSystems 0x10
#define AUTOBEAM 0x20

/* BeamNoteRelationship returns CONTINUEBEAM or DISCONTINUEBEAM */

#define DISCONTINUEBEAM 0
#define CONTINUEBEAM 4
#define CROSSEDBEAMBREAK 8

enum {
	NOTBEAMING,
	POSTULATED,
	BEAMING
};

struct BeamCounter {
	Byte BeamState;
	Byte NotesInBeam;
	Word nextLINK;
	long LastSyncLCDsIntoThisMeasure;
	long LCDsIntoThisMeasure;
	LINK Syncs[MAXINBEAM];
};

struct BeamCounter *beamCounterPtr;
Byte UserBeamBreaksAt[32];						/* initialized but unused so far */
Byte CurrentBeamCondition = CROSSRESTS;

static void CreateNBeamBeatList(Byte, Byte);
static Byte BeamNoteRelationship(LINK, Byte);
static void CreateBEAMandResetBeamCounter(Document *, Byte, LINK);
static void PostulateBeam (LINK);
static long GetAnyLTime(Document *, LINK);
static void ExpandSelRange(Document *doc);
static void CreateBeatList(Document *doc, LINK pL, LINK aNoteL);
static Byte SetBeamCounter(Document *doc, LINK pL, LINK aNoteL);
static void DoNextBeamState(Document *doc, LINK pL, short voice, Byte nextBeamState);


/* begin: moved here from old OpcodeUtils.c by chirgwin Mon May 28 07:17:18 PDT 2012 
 TODO: remove this function or at least find it a better home
 */
  
 /* Given a one-byte value, starting address, and length, fill memory with the value. */

void FillMem(Byte value, void *loc, DoubleWord len)
{
	Byte *ptr = (Byte *)loc;
	long slen = len;

	while (--slen >= 0L) *ptr++ = value;
}


/* end: moved here from old OpcodeUtils.c by chirgwin Mon May 28 07:17:18 PDT 2012 */


/* Some comments by Ray:
The Beam Beat is the beat across which a beam cannot extend. The beatEX is the
beat of a compound measure where the quantizing portion of [Vision] starts looking
for triplets and the like. In this model we have one meter per sequence.

Variable Naming conventions:  Variables whose names end in EX are ones in which a
beat is FineFactor * LCDs (i.e. 480*14=6720 Decimal to the quarter note, 1A40 Hex).
The fine factor is 3*5*7*64/480=14.
Variables whose names end in LCD are 480 to the quarter note.
*/

long BeamBeatListEX[5];
long beatEX;

static void CreateNBeamBeatList(Byte num, Byte denom)
{
	Byte Index, SIndex, meterType;
	long breakDurEX, lTemp;
	Boolean break4, breakMin;
	
	break4 = (config.autoBeamOptions & 1)==0;
	breakMin = ((config.autoBeamOptions & 2)==0? 2 : 3);
	
	if (((num % 3) == 0) && (num >= 6))					/* dotted: 2 dotted-quarter = 6/8 */
		meterType = Compound;
	else
		meterType = (num % 2) ? Imperfect : Simple;

	lTemp = (long)num*FineFactor*LCD4;
	MeasureLengthEX  = lTemp /denom;

	beatEX = (LCD4 * FineFactor) / denom;
	RCP = RCPtr;

	FillMem (0, RCPtr, 32 * sizeof (struct RhythmClarification));
	FillMem (0, UserBeamBreaksAt, 32);

	RCP->BeatsThisRunoff = num;
	RCP->Watershed128 = 0;
	RCP->RunoffdenompH = 0xff;
	RCP->WatershedEX = MeasureLengthEX;
	RCP->RelativeStrength = 9;
	SIndex = Index = RCP - RCPtr;
	while (RCP > RCPtr) {
		--RCP;
		--Index;
		RCP->NextStrongBeat = SIndex;
		if (RCP->RelativeStrength != 0) SIndex = Index;
	}

	/* Now mark each beat where beams ought to break.  Eventually these "tab stops"
		should be settable by the user.  All entries are terminated by 0; the first
		entry of num is taken for granted.  The breakpoints are arranged in reverse
		because they will be compared to the beats remaining in the measure. */

	if (num < breakMin) {								/* Numerator<minimum: no breakpoints */
		BeamBeatListEX[0] = 0;
		return;
	}

	if ((num & 3) == 0 && break4) {						/* Numerator div. by 4: maybe break in 4 */
		breakDurEX = (beatEX*num) / 4L;
		BeamBeatListEX[3] = 0;
		BeamBeatListEX[2] = breakDurEX;
		BeamBeatListEX[1] = breakDurEX * 2L;
		BeamBeatListEX[0] = breakDurEX * 3L;
		return;
	}

	if ((num & 1) == 0) {								/* Numerator even: break into 2 */
		BeamBeatListEX[1] = 0;
		BeamBeatListEX[0] = (beatEX*num) / 2L;
		return;
	}

	if ((num % 3) == 0) {								/* Numerator odd & div. by 3: break into 3 */
		breakDurEX = (beatEX*num) / 3L;
		BeamBeatListEX[2] = 0;
		BeamBeatListEX[1] = breakDurEX;
		BeamBeatListEX[0] = breakDurEX * 2L;
		return;
	}

	BeamBeatListEX[1] = 0;								/* Any other: break roughly in half */
	BeamBeatListEX[0] = beatEX * (1 + (num / 2L));
}

/* ----------------------------------------------------------------------------------- */
/* Beam the selection. Traverse the selection once, making potential beam events every
time a note in a voice is short enough to beam across; when a beam isn't in progress;
and terminating it and linking it in or discarding it when it terminates. */

static Byte BeamNoteRelationship(LINK aNoteL, Byte beamConditions)
{
	long *T;
	
	if (NoteType(aNoteL) <= QTR_L_DUR)
		return DISCONTINUEBEAM; /* quarter note or longer */

	/* only the above condition can always halt a beam.  The user can override
		all the other algorithms by selecting notes which violate it */

	if ((beamConditions & AUTOBEAM) == 0)
		return CONTINUEBEAM;

	/* we get here if we're beaming by general parameters rather than specific
		request: as with a MIDI record or a MIDI file (??Looks like a Ray Spears
		comment: does this make sense in Nightingale? -DAB) */

	if (NoteREST(aNoteL)) {
		if (beamCounterPtr->BeamState == NOTBEAMING) {
			if ((beamConditions & StartAtRest) == 0)
				return DISCONTINUEBEAM;
		} else {
			if ((beamConditions & CROSSRESTS) == 0)
				return DISCONTINUEBEAM;
		}
	}

	/* and the timing constraints.  If the time of the sync
		lies on the opposite side of a beam cross point, then discontinue. ##### */

	for (T = BeamBeatListEX; *T > 0; T++) {
		if (beamCounterPtr->LCDsIntoThisMeasure >= *T) {
			if (beamCounterPtr->LastSyncLCDsIntoThisMeasure < *T) {
				return CROSSEDBEAMBREAK;
				}
			}
		}
	return CONTINUEBEAM;
}

static void CreateBEAMandResetBeamCounter(Document *doc, Byte voice, LINK endL)
{
	if (beamCounterPtr->BeamState == BEAMING) {
		CreateBEAMSET(doc, beamCounterPtr->Syncs[0], endL, voice,
							beamCounterPtr->nextLINK, True,
							doc->voiceTab[voice].voiceRole);
		}
	beamCounterPtr->BeamState = NOTBEAMING;
	beamCounterPtr->nextLINK = 0;
}

static void PostulateBeam(LINK pL)
{
	beamCounterPtr->BeamState = POSTULATED;
	beamCounterPtr->Syncs[0] = pL;
	beamCounterPtr->nextLINK = 1;
}

/* -------------------------------------------------------------------- GetAnyLTime -- */
/* Get "logical time" since previous Measure of any object:
		If the argument is a Sync, return its start time, in p_dur units.
		If the argument is anything else, return the logical time of the next
			Sync in the Measure; if there is none, return 0. */

static long GetAnyLTime(Document *doc, LINK target)
{
	LINK startL;
	
	for (startL = target; startL!=doc->tailL; startL = RightLINK(startL)) {
		if (SyncTYPE(startL)) return GetLTime(doc, startL);
		if (MeasureTYPE(startL)) return 0L;
	}
	return 0L;
}

/* ------------------------------------------------------- Utilities for DoAutoBeam -- */

/* We may have created one or more Beamsets just before doc->selStartL; if so,
	they're all selected, so be sure the selection range includes them. */

static void ExpandSelRange(Document *doc)
{
	LINK pL;
	
	pL=doc->selStartL;
	while (pL!=doc->headL && LinkSEL(LeftLINK(pL)))
		pL=LeftLINK(pL);
 	doc->selStartL = pL;
}

static void CreateBeatList(Document *doc, LINK pL, LINK aNoteL)
{
	CONTEXT context;

	GetContext(doc, pL, NoteSTAFF(aNoteL), &context);
	CreateNBeamBeatList(context.numerator, context.denominator);
}

static Byte SetBeamCounter(Document *doc, LINK pL, LINK aNoteL)
{
	Byte nextBeamState;

	beamCounterPtr->LCDsIntoThisMeasure =
					GetAnyLTime(doc,pL) * FineFactor;
	nextBeamState = beamCounterPtr->BeamState +
					BeamNoteRelationship(aNoteL,CurrentBeamCondition);
	beamCounterPtr->LastSyncLCDsIntoThisMeasure =
					beamCounterPtr->LCDsIntoThisMeasure;
					
	return nextBeamState;
}

static void DoNextBeamState(Document *doc, LINK pL, short voice, Byte nextBeamState)
{
	switch (nextBeamState) {
		case POSTULATED + DISCONTINUEBEAM:
			beamCounterPtr->BeamState = NOTBEAMING;
			break;

		case NOTBEAMING + CROSSEDBEAMBREAK:
		case NOTBEAMING + CONTINUEBEAM:
		case POSTULATED + CROSSEDBEAMBREAK:
			PostulateBeam(pL);
			break;

		case POSTULATED + CONTINUEBEAM:
			beamCounterPtr->BeamState = BEAMING;
		case BEAMING + CONTINUEBEAM:
			beamCounterPtr->Syncs[beamCounterPtr->nextLINK] = pL;
			beamCounterPtr->nextLINK++;
			break;

		case BEAMING + DISCONTINUEBEAM:
			CreateBEAMandResetBeamCounter(doc,voice,pL);
			break;

		/* if this beam has only been broken because it crossed a beam
			break, we should start it right back up again */

		case BEAMING + CROSSEDBEAMBREAK:
			CreateBEAMandResetBeamCounter(doc,voice,pL);
			PostulateBeam(pL);
			break;

		case NOTBEAMING + DISCONTINUEBEAM:
		default:
			break;

	}
}


/* ----------------------------------------------------------------------- AutoBeam -- */
/* Beam the selection "automatically", i.e., according to the meter. Return True
normally, False if unable to allocate temporary memory blocks. */

Boolean AutoBeam(Document *doc)
{
	Boolean firstNoteInVoice;
	Byte nextBeamState;
	LINK pL,measL,endMeasL,aTimeSigL;
	Byte voice;
	LINK aNoteL;
	Boolean okay=False;
	
	CurrentBeamCondition = (doc->beamRests? CROSSRESTS + AUTOBEAM : AUTOBEAM);

	beamCounterPtr = (struct BeamCounter *)0;
	RCPtr = (struct RhythmClarification *)0;
	
	beamCounterPtr = (struct BeamCounter *)NewPtr(sizeof(struct BeamCounter));
	if (!GoodNewPtr((Ptr)beamCounterPtr)) {
		OutOfMemory(sizeof(struct BeamCounter));
		goto Done;
	}
	RCPtr = (struct RhythmClarification *)NewPtr(32*sizeof(struct RhythmClarification));
	if (!GoodNewPtr((Ptr)RCPtr)) {
		DisposePtr((Ptr)beamCounterPtr);
		OutOfMemory(32*sizeof(struct RhythmClarification));
		goto Done;
	}

	PushLock(OBJheap);
	PushLock(NOTEheap);
	PushLock(TIMESIGheap);

	/* The time signature can be different for each staff, so each voice can have a
		different time signature and hence different automatic beaming. But since a
		voice can change staff at any time, handling this fully would be pretty messy.
		Besides, Nightingale insists all barlines coincide, so there's not that much
		that can be done with differing time sigs. So, for now, even though our outer
		loop is by voice, just use the first time sig. subobject of every time sig.
		object for everything. */

	for (voice = 1; voice<=MAXVOICES; voice++) {
		if (!VOICE_MAYBE_USED(doc, voice)) continue;
		
		firstNoteInVoice = True;
		FillMem (0, beamCounterPtr, sizeof(struct BeamCounter));
		
		beamCounterPtr->LCDsIntoThisMeasure =
				beamCounterPtr->LastSyncLCDsIntoThisMeasure =
				GetAnyLTime(doc,doc->selStartL) * FineFactor;
		
		/* Go thru the range and create beams for the voice. The new Beamsets will be
			in the middle of the range we're going thru: this won't cause any problems
			because they'll always be in a part of the range and apply to a part of the
			range we've already traversed. */
		
		for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
			switch (ObjLType(pL)) {
				case SYNCtype:
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
						if (NoteSEL(aNoteL) && NoteVOICE(aNoteL) == voice) {
							if (firstNoteInVoice) {
								CreateBeatList(doc,pL,aNoteL);
								firstNoteInVoice = False;
							}

							nextBeamState = SetBeamCounter(doc,pL,aNoteL);
							DoNextBeamState(doc,pL,voice,nextBeamState);

							break;					/* Only use 1 note per Sync */
						}
					break;

				case TIMESIGtype:
					CreateBEAMandResetBeamCounter(doc, voice, pL);
					aTimeSigL = FirstSubLINK(pL);
					CreateNBeamBeatList(TimeSigNUMER(aTimeSigL), TimeSigDENOM(aTimeSigL));
					break;
		
				case MEASUREtype:
					/* This might be the first Measure of a system; what we want is the
						object ending the previous Measure. */
						
					measL = SSearch(LeftLINK(pL), MEASUREtype, GO_LEFT);
					endMeasL = EndMeasSearch(doc, measL);
					CreateBEAMandResetBeamCounter(doc, voice, endMeasL);
					break;
				
				default:
					break;	
			}

		/* Try to beam the last few notes in the voice. */
		
		CreateBEAMandResetBeamCounter (doc, voice, doc->selEndL);
	}

	/* Expand selRange to include possibly-added beams. */
	
	ExpandSelRange(doc);
	okay = True;

	PopLock(OBJheap);
	PopLock(NOTEheap);
	PopLock(TIMESIGheap);
	
Done:
	if (RCPtr) DisposePtr((Ptr)RCPtr);
	if (beamCounterPtr) DisposePtr((Ptr)beamCounterPtr);
	return okay;
}


/* --------------------------------------------------------------------- DoAutoBeam -- */
/* User-interface level function to automatically beam the selection. */

void DoAutoBeam(Document *doc)
{
	if (doc->selEndL==doc->selStartL) return;

	/* If any selected note is beamed, it must be unbeamed before we can proceed. */

	if (SelBeamedNote(doc)) {
		if (CautionAdvise(BBBEAM_ALRT)==Cancel) return;
		PrepareUndo(doc, doc->selStartL, U_AutoBeam, 9);	/* "Undo Automatic Beam" */
		Unbeam(doc);
	}
	else
		PrepareUndo(doc, doc->selStartL, U_AutoBeam, 9);	/* "Undo Automatic Beam" */

	WaitCursor();
	if (!AutoBeam(doc))
		DisableUndo(doc, True);
}
