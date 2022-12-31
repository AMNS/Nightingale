/* MIDIUtils.c for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include <CoreMIDI/MIDIServices.h>		/* for MIDIPacket */

#include "MidiMap.h"
#include "CarbonStubs.h"

/* Thus far, the event list maintained herein contains only one type of event -- Note Off
-- so there's no need for a code for the event type. */

static MIDIEvent	eventList[MAXEVENTLIST];
static CMMIDIEvent	cmEventList[MAXEVENTLIST];

static short	lastEvent;

static Boolean	CMHaveLaterEnding(short note, SignedByte channel, long endTime);
static Boolean	InsertEvent(short note, SignedByte channel, long endTime, Boolean playMaxDur,
								short ioRefNum);
static Boolean	CMInsertEvent(short note, SignedByte channel, long endTime, Boolean playMaxDur,
								long ioRefNum);
static void		KillEventList(void);
static void		CMKillEventList(void);


/* -------------------------------------------------------------------- UseMIDIChannel -- */
/* Return the MIDI channel number to use for the given part.

UseMIDIChannel will get the channel from the partInfo record, entered in the range [1,16],
and pin it inside the range [1,MAXCHANNEL] where MAXCHANNEL = 16. This is always correct,
regardless of the channel base; so we do not need to define CMUseMIDIChannel here to take
into account CM_CHANNEL_BASE. The channel will be translated to its proper base when
constructing the packet either from CMMIDIProgram or using CMGetNotePlayInfo. */
 
short UseMIDIChannel(Document *doc, short partn)
{
	short		useChan;
	LINK		partL;
	PPARTINFO	pPart;
	
	if (doc->polyTimbral) {
		partL = FindPartInfo(doc, partn);
		pPart = GetPPARTINFO(partL);
		useChan = pPart->channel;
		if (useChan<1) useChan = 1;
		if (useChan>MAXCHANNEL) useChan = MAXCHANNEL;
		return useChan;
	}
	else
		return doc->channel;
}


/* ---------------------------------------------------------------------- NoteNum2Note -- */
/* Return the note (not rest) in the given sync and voice with the given MIDI note
number, if there is one; else return NILINK. If the sync and voice have more than one
note with the same note number, finds the first one. Intended for finding ties' notes. */

LINK NoteNum2Note(LINK syncL, short voice, short noteNum)
{
	LINK aNoteL;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice && !NoteREST(aNoteL) && NoteNUM(aNoteL)==noteNum) {
			return aNoteL;
		}
	return NILINK;
}


/* --------------------------------------------------------------------------- TiedDur -- */
/* Return total performance duration of <aNoteL> and all following notes tied to
<aNoteL>. If <selectedOnly>, only includes tied notes until the first unselected one.
We use the note's <playDur> only for the last note of the series, its NOTATED duration
for all the others. */

long TiedDur(Document */*doc*/, LINK syncL, LINK aNoteL, Boolean selectedOnly)
{
	LINK	syncPrevL, aNotePrevL, continL, continNoteL;
	short	voice;
	long	dur, prevDur;
	
	voice = NoteVOICE(aNoteL);
	
	dur = 0L;
	syncPrevL = syncL;
	aNotePrevL = aNoteL;
	while (NoteTIEDR(aNotePrevL)) {
		continL = LVSearch(RightLINK(syncPrevL),			/* Tied note should always exist */
						SYNCtype, voice, GO_RIGHT, False);
		continNoteL = NoteNum2Note(continL, voice, NoteNUM(aNotePrevL));
		if (continNoteL==NILINK) break;						/* Should never happen */
		if (selectedOnly && !NoteSEL(continNoteL)) break;

		/* We know this note will be played, so add in the previous note's notated dur. */
		
		prevDur = SyncAbsTime(continL)-SyncAbsTime(syncPrevL);
		dur += prevDur;

		syncPrevL = continL;
		aNotePrevL = continNoteL;
	}
	dur += NotePLAYDUR(aNotePrevL);
	return dur;
}


/* -------------------------------------------------------------------- UseMIDINoteNum -- */
/* Return the MIDI note number that should be used to play the given note, considering
transposition. If the "note" is a rest or the continuation of a tied note, or has
velocity 0, it should not be played: indicate by returning -1. */

short UseMIDINoteNum(Document *doc, LINK aNoteL, short transpose)
{
	short midiNote;
	PANOTE aNote;
	LINK partL;
	PPARTINFO pPart;
	Byte patchNum;

	aNote = GetPANOTE(aNoteL);
	if (aNote->rest || aNote->tiedL						/* Don't play rests and continuations */
	||  aNote->onVelocity==0)							/* ...and notes w/ on velocity 0 */
		midiNote = -1;
	else {
		midiNote = aNote->noteNum;
		if (doc->transposed)							/* Handle "transposed score" */
			midiNote += transpose;
		if (midiNote<1) midiNote = 1;					/* Clip to legal range */
		if (midiNote>MAX_NOTENUM) midiNote = MAX_NOTENUM;
	}

	partL = Staff2PartLINK(doc, NoteSTAFF(aNoteL));
	pPart = GetPPARTINFO(partL);
	patchNum = pPart->patchNum;
	if (IsPatchMapped(doc, patchNum)) {
		midiNote = GetMappedNoteNum(doc, midiNote);
	}	

	return midiNote;
}


/* ------------------------------------------------------------------- GetModNREffects -- */
/* Given a note, if it has any modifiers, get information about how its modifiers should
affect playback, and return True. If it has no modifiers, just return False. NB: The
time factor is unimplemented. */

Boolean GetModNREffects(LINK aNoteL, short *pVelOffset, short *pDurFactor,
			short *pTimeFactor)
{
	LINK	aModNRL;
	short	velOffset, durFactor, timeFactor;

	if (!config.useModNREffects) return False;

	aModNRL = NoteFIRSTMOD(aNoteL);
	if (!aModNRL) return False;

	velOffset = 0;
	durFactor = timeFactor = 100;

	for ( ; aModNRL; aModNRL = NextMODNRL(aModNRL)) {
		Byte code = ModNRMODCODE(aModNRL);
		if (code>31) {										/* Arrays sized for 32 items. */
			LogPrintf(LOG_WARNING, "Illegal modifier code %d.  (GetModNREffects)\n", code);
			continue;
		}
		velOffset += modNRVelOffsets[code];
// FIXME: What if more than one modifier?
		durFactor = modNRDurFactors[code];
//		timeFactor = modNRTimeFactors[code];
	}

	*pVelOffset = velOffset;
	*pDurFactor = durFactor;
	*pTimeFactor = timeFactor;

	return True;
}


/* ------------------------------------------------------------- Tempo/time conversion -- */

/* Given a TEMPO object, return its "timeScale", i.e., tempo in PDUR ticks per minute. */

long Tempo2TimeScale(LINK tempoL)
{
	PTEMPO pTempo;
	long tempoMM, beatDur, timeScale;
	
	pTempo = GetPTEMPO(tempoL);
	tempoMM = pTempo->tempoMM;
	beatDur = Code2LDur(pTempo->subType, (pTempo->dotted? 1 : 0));
	timeScale = tempoMM*beatDur;
	return timeScale;
}


/* Return the tempo in effect at the given LINK, if any. The tempo is computed from the
last preceding metronome mark; if there is no preceding metronome mark, it's computed
from the default tempo. In any case, the tempo is expressed as a "timeScale", i.e., in
PDUR ticks per minute. */

long GetTempoMM(
		Document */*doc*/,		/* unused */
		LINK startL)
{
	LINK tmpL, tempoL;
	long timeScale;

	timeScale = (long)config.defaultTempoMM*DFLT_BEATDUR;	/* Default in case no tempo marks found */

	if (startL) {
		tempoL = NILINK;
		tmpL = startL;
		while (tempoL==NILINK) {
			tmpL = LSSearch(tmpL, TEMPOtype, ANYONE, GO_LEFT, False);
			if (!tmpL) break;
			if (TempoNOMM(tmpL)==False) tempoL = tmpL;
			tmpL = LeftLINK(tmpL);
		}
		if (tempoL) timeScale = Tempo2TimeScale(tempoL);
	}

	return timeScale;
}


/* Convert PDUR ticks to millisec., with tempo varying as described by tConvertTab[].
If we can't convert it, return -1L. */

long PDur2RealTime(
			long t,						/* time in PDUR ticks */
			TCONVERT tConvertTab[],
			short tabSize)
{
	short i, iPos=1;
	long msAtPrevTempo, msSincePrevTempo;
	
	/* If the table is empty, just return zero. Otherwise, find the 1st entry in the
		table for a PDUR time after <t>, and use the previous entry; if there's no such
		entry, assume the last entry in the table applies. */
		
	if (tabSize<=0) return 0L;
	
	for (i = 1; i<tabSize; i++)
		if (tConvertTab[i].pDurTime>t) {
			iPos = i;
			break;
		}
	if (tConvertTab[iPos].pDurTime<=t) iPos = tabSize;
	msAtPrevTempo = tConvertTab[iPos-1].realTime;
	
	msSincePrevTempo = PDUR2MS(t-tConvertTab[iPos-1].pDurTime, tConvertTab[iPos-1].microbeats);
	if (msSincePrevTempo<0) return -1L;
	
	return msAtPrevTempo+msSincePrevTempo;
}


/* Build a table of metronome marks in effect in the given range, sorted by increasing
time. Return value is the size of the table, or -1 if the table is too large. Intended
to get information for changing tempo during playback. */

short MakeTConvertTable(
				Document *doc,
				LINK fromL, LINK toL,				/* range to be played */
				TCONVERT tConvertTab[],
				short maxTabSize
				)
{
	short	tempoCount;
	LINK	pL, measL, syncL, syncMeasL;
	long	microbeats,							/* microsec. units per PDUR tick */
			timeScale,							/* PDUR ticks per minute */
			pDurTime;							/* in PDUR ticks */

	tempoCount = 0;

	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:
				measL = pL;
				break;
			case SYNCtype:
			  	/* If no tempo found yet, our initial tempo is the last previous one. */
			  	
			  	if (tempoCount==0) {
					syncL = LSSearch(fromL, SYNCtype, ANYONE, GO_RIGHT, False);
					timeScale = GetTempoMM(doc, syncL);			/* OK even if syncL is NILINK */
					microbeats = TSCALE2MICROBEATS(timeScale);
					tConvertTab[0].microbeats = microbeats;
					tConvertTab[0].pDurTime = 0L;
					tConvertTab[0].realTime = 0L;
					tempoCount = 1;
				}
				break;
			case TEMPOtype:
				if (TempoNOMM(pL)) break;			/* Skip Tempo objects with no M.M. */
				if (tempoCount>=maxTabSize) return -1;

				timeScale = Tempo2TimeScale(pL);
				microbeats = TSCALE2MICROBEATS(timeScale);
				tConvertTab[tempoCount].microbeats = microbeats;
				syncL = SSearch(pL, SYNCtype, GO_RIGHT);
				syncMeasL = SSearch(syncL, MEASUREtype, GO_LEFT);
				pDurTime = MeasureTIME(syncMeasL)+SyncTIME(syncL);
				tConvertTab[tempoCount].pDurTime = pDurTime;
				
				/* It's OK to call PDur2RealTime here: the part of the table it needs
				   already exists. */
					 
				tConvertTab[tempoCount].realTime = PDur2RealTime(pDurTime, tConvertTab,
													tempoCount);
				tempoCount++;
				break;
			default:
				;
		}
	}

	if (DETAIL_SHOW) {
		for (short i = 0; i<tempoCount; i++)
			LogPrintf(LOG_DEBUG, "MakeTConvertTable: tConvertTab[%d].microbeats=%ld pDurTime=%ld realTime=%ld\n",
				i, tConvertTab[i].microbeats, tConvertTab[i].pDurTime, tConvertTab[i].realTime);
	}

	return tempoCount;
}


/* ------------------------------------------------ MIDI Manager/Driver glue functions -- */
/* The following functions support Apple Core MIDI. */

void StartMIDITime()
{
	switch (useWhichMIDI) {
		case MIDIDR_CM:
			CMStartTime();
			break;
		default:
			break;
	};
}

#define TURN_PAGES_WAIT True	/* After "turning" page, True=resume in tempo, False="catch up" */

long GetMIDITime(long pageTurnTOffset)
{
	long time=0L;

	switch (useWhichMIDI) {
		case MIDIDR_CM:
			time = CMGetCurTime();
			break;
		default:
			break;
	};
	return time-(TURN_PAGES_WAIT? pageTurnTOffset : 0);
}


void StopMIDITime()
{
	switch (useWhichMIDI) {
		case MIDIDR_CM:
			CMStopTime();
			break;
		default:
			break;
	};
}

void StopMIDI()
{
	if (useWhichMIDI==MIDIDR_CM)
		CMKillEventList();
	else
		KillEventList();								/* stop all notes being played */
	
	StopMIDITime();
}


/* -------------------------------------------------------------- Event list functions -- */

/* Initialize the Event list to empty. */

void InitEventList()
{
	lastEvent = 0;
}


static Boolean CMHaveLaterEnding(short note, SignedByte channel, long endTime)
{
	short		i;
	CMMIDIEvent	*pEvent;

	for (i = 0, pEvent = cmEventList; i<lastEvent; i++, pEvent++) {
#if PMDDEBUG
LogPrintf(LOG_DEBUG,
"CMHaveLaterEnding: pEvent->note,note=%hd,%d   pEvent->channel,channel=%hd,%d   pEvent->endTime,endTime=%hd,%d\n",
(short)(pEvent->note), note, (short)(pEvent->channel), channel, (short)(pEvent->endTime), endTime);
#endif
		if (pEvent->note == note && pEvent->channel == channel && pEvent->endTime>endTime)
			return True;
	}
	
	return False;
}


/*	Insert the specified note into the event list. Exception FIXME: not implemented! :
if _playMaxDur_ and there's already an event for that note no. on the same channel
with a later end time, do nothing. If we succeed, return True; if we fail (because the
list is full), give an error message and return False. */

static Boolean InsertEvent(short note, SignedByte channel, long endTime, Boolean /* playMaxDur */,
					short ioRefNum)
{
	short		i;
	MIDIEvent	*pEvent;
	char		fmtStr[256];

	/* If _playMaxDur_ and there's already an event for that note no. on the same channel
	   with a later end time, we have nothing to do. */
	// if (playMaxDur && HaveLaterEnding(note, channel, endTime)) return True;
		
	/* Find first free slot in list, which may be at lastEvent (end of list) */
	
	for (i = 0, pEvent = eventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note == 0) break;
	
	/* Insert note into free slot, or append to end of list if there's room */
	
	if (i<lastEvent || lastEvent++<MAXEVENTLIST) {
		pEvent->note = note;
		pEvent->channel = channel;
		pEvent->endTime = endTime;
		pEvent->omsIORefNum = ioRefNum;
		return True;
	}
	else {
		lastEvent--;
		GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 13);		/* "can play only %d notes at once" */
		sprintf(strBuf, fmtStr, MAXEVENTLIST);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
}


static void AddToEventList(short note, SignedByte channel, long endTime, long ioRefNum,
					CMMIDIEvent *pEvent)
{
	pEvent->note = note;
	pEvent->channel = channel;
	pEvent->endTime = endTime;
	pEvent->cmIORefNum = ioRefNum;
}


/*	Insert the specified note into the event list.  Exception: if _playMaxDur_ and
there's already an event for that note no. on the same channel with a later end time,
do nothing. If we succeed, return True; if we fail (because the list is full),
give an error message and return False. */

static Boolean CMInsertEvent(short note, SignedByte channel, long endTime, Boolean playMaxDur,
					long ioRefNum)
{
	short		i;
	CMMIDIEvent	*pEvent;
	char		fmtStr[256];

	/* If _playMaxDur_ and there's already an event for that note no. on the same channel
	   with a later end time, we have nothing to do. */
	   
		if (playMaxDur && CMHaveLaterEnding(note, channel, endTime)) {
//LogPrintf(LOG_DEBUG, "CMInsertEvent: HaveLaterEnding for note=%d\n", note);
			return True;
		}

//LogPrintf(LOG_DEBUG, "CMInsertEvent: note=%d\n", note);

	/* If _playMaxDur_ and there's already an event for that note no. on the same channel
	   with an earlier end time, replace it with this event. Otherwise, just find the
	   first free slot in list, which may be at lastEvent (end of list). */
	
	for (i = 0, pEvent = cmEventList; i<lastEvent; i++, pEvent++) {
#if PMDDEBUG
LogPrintf(LOG_DEBUG,
"CMInsertEvent: pEvent->note=%hd, pEvent->channel=%hd, pEvent->endTime=%hd\n",
(short)(pEvent->note), note, (short)(pEvent->channel), channel, (short)(pEvent->endTime), endTime);
#endif
		if (pEvent->note == note && pEvent->channel == channel && pEvent->endTime<endTime) {
			AddToEventList(note, channel, endTime, ioRefNum, pEvent);
			return True;
		}
		if (pEvent->note == 0) break;
	}
	
	/* Insert note into free slot, or append to end of list if there's room */
	
	if (i<lastEvent || lastEvent++<MAXEVENTLIST) {
		AddToEventList(note, channel, endTime, ioRefNum, pEvent);
		return True;
	}
	else {
		lastEvent--;
		GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 13);		/* "can play only %d notes at once" */
		sprintf(strBuf, fmtStr, MAXEVENTLIST);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
}

/*	Checks eventList[] to see if any notes are ready to be turned off; if so, frees
their slots in the eventList and (if we're not using MIDI Manager) turns them off.
Returns True if the list is empty. */

Boolean CheckEventList(long pageTurnTOffset)
{
	Boolean		empty;
	MIDIEvent	*pEvent;
	short		i;
	long		t;
	
	t = GetMIDITime(pageTurnTOffset);
	empty = True;
	for (i=0, pEvent = eventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
			empty = False;
			if (pEvent->endTime<=t) {							/* note is done, t = now */
				EndNoteNow(pEvent->note, pEvent->channel);
				pEvent->note = 0;								/* slot available now */
			}
		}

	return empty;
}

/*	Checks eventList[] to see if any notes are ready to be turned off; if so, frees
their slots in the eventList and (if we're not using MIDI Manager) turns them off.
Returns True if the list is empty. */

Boolean CMCheckEventList(long pageTurnTOffset)
{
	Boolean		empty;
	CMMIDIEvent	*pEvent;
	short		i;
	long		t;
	
	t = GetMIDITime(pageTurnTOffset);
	empty = True;
	for (i=0, pEvent = cmEventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
//LogPrintf(LOG_DEBUG, "CMCheckEventList: pEvent-note=%d\n", pEvent->note);
			empty = False;
			if (pEvent->endTime<=t) {							/* note is done, t = now */
				CMEndNoteNow(pEvent->cmIORefNum, pEvent->note, pEvent->channel);
				pEvent->note = 0;								/* slot available now */
			}
		}

	return empty;
}

/*	Turn off all notes in eventList[] and re-initialize it. */

static void CMKillEventList()
{
	CMMIDIEvent	*pEvent;
	short		i;
	
	for (i = 0, pEvent = cmEventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
			CMEndNoteNow(pEvent->cmIORefNum, pEvent->note, pEvent->channel);
		}

	InitEventList();
}


/*	Turn off all notes in eventList[] and re-initialize it. */

static void KillEventList()
{
	MIDIEvent	*pEvent;
	short		i;
	
	for (i = 0, pEvent = eventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
			EndNoteNow(pEvent->note, pEvent->channel);
		}

	InitEventList();
}

/* ------------------------------------------------------------------- GetPartPlayInfo -- */

void GetPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
						Byte channelPatch[], SignedByte partVelo[])
{
	short i; LINK partL; PARTINFO aPart;

	for (i = 1; i<=MAXCHANNEL; i++)
		channelPatch[i] = MAXPATCHNUM+1;					/* Initialize to illegal value */
	
	partL = FirstSubLINK(doc->headL);
	for (i = 0; i<=LinkNENTRIES(doc->headL)-1; i++, partL = NextPARTINFOL(partL)) {
		aPart = GetPARTINFO(partL);
		partVelo[i] = aPart.partVelocity;
		partChannel[i] = UseMIDIChannel(doc, i);
		channelPatch[partChannel[i]] = aPart.patchNum;
		partTransp[i] = aPart.transpose;
	}
}

/* ------------------------------------------------------------------- GetNotePlayInfo -- */
/* Given a note and tables of part transposition, channel, and offset velocity, return
the note's MIDI note number, including transposition; channel number; and velocity,
limited to legal range. */

void GetNotePlayInfo(Document *doc, LINK aNoteL, short partTransp[],
						Byte partChannel[], SignedByte partVelo[],
						short *pUseNoteNum, short *pUseChan, short *pUseVelo)
{
	short partn;
	PANOTE aNote;

	partn = Staff2Part(doc,NoteSTAFF(aNoteL));
	*pUseNoteNum = UseMIDINoteNum(doc, aNoteL, partTransp[partn]);
	*pUseChan = partChannel[partn];
	aNote = GetPANOTE(aNoteL);
	*pUseVelo = doc->velOffset+aNote->onVelocity;
	if (doc->polyTimbral) *pUseVelo += partVelo[partn];
	
	if (*pUseVelo<1) *pUseVelo = 1;
	if (*pUseVelo>MAX_VELOCITY) *pUseVelo = MAX_VELOCITY;
}

/* -------------------------------------------------------------------- SetMIDIProgram -- */

#define PATCHNUM_BASE 1			/* Some synths start numbering at 1, some at 0 */

/* -------------------------------------------- StartNoteNow, EndNoteNow, EndNoteLater -- */
/* Functions to start and end notes. NB: EndNoteLater does not communicate with the
MIDI system, it simply adds the note to our event-list routines' queue. */

OSStatus StartNoteNow(short noteNum, SignedByte channel, SignedByte velocity)
{
#if DEBUG_KEEPTIMES
	if (nkt<MAXKEEPTIMES) kStartTime[nkt++] = TickCount();
#endif
	OSStatus err = noErr;
	
	if (noteNum>=0) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				err = CMStartNoteNow(0, noteNum, channel, velocity);
				break;
			default:
				break;
		}
	}
	
	return err;
}

OSStatus EndNoteNow(short noteNum, SignedByte channel)
{
	OSStatus err = noErr;
	
	switch (useWhichMIDI) {
		case MIDIDR_CM:
			err = CMEndNoteNow(0, noteNum, channel);
			break;
		default:
			break;
	}
	
	return err;
}

/* If multiple notes are simultaneously playing the same note number on the same channel
but end at different times, do we hold the note till the last one ends or stop playing
the note when the first one ends? For many years, Nightingale always did the latter, but
the former makes more sense in most cases. */

#define MULTNOTES_PLAYMAXDUR True		/* Hold the note till the last instance ends? */ 

Boolean EndNoteLater(
			short noteNum,
			SignedByte channel,			/* 1 to MAXCHANNEL */
			long endTime,
			short ioRefNum)
{
	return InsertEvent(noteNum, channel, endTime, MULTNOTES_PLAYMAXDUR, ioRefNum);
}

Boolean CMEndNoteLater(
			short noteNum,
			SignedByte channel,			/* 1 to MAXCHANNEL */
			long endTime,
			long ioRefNum)
{
	return CMInsertEvent(noteNum, channel, endTime, MULTNOTES_PLAYMAXDUR, ioRefNum);
}


/* --------------------------------------------------- MMStartNoteNow, MMEndNoteAtTime -- */
/* For MIDI Manager: start the note now, end the note at the given time. */
 
void MMStartNoteNow(
			short noteNum,
			SignedByte channel,			/* 1 to MAXCHANNEL */
			SignedByte velocity
			)
{
	MMMIDIPacket mPacket;

	mPacket.flags = MM_STD_FLAGS;	
	mPacket.len = MM_NOTE_SIZE;
			
	mPacket.tStamp = MM_NOW;
	mPacket.data[0] = MNOTEON+channel-CM_CHANNEL_BASE;
	mPacket.data[1] = noteNum;
	mPacket.data[2] = velocity;
	MIDIWritePacket(outputMMRefNum, &mPacket);
}

void MMEndNoteAtTime(
			short noteNum,
			SignedByte channel,			/* 1 to MAXCHANNEL */
			long endTime
			)
{
	MMMIDIPacket mPacket;
	
	mPacket.flags = MM_STD_FLAGS;	
	mPacket.len = MM_NOTE_SIZE;
			
	mPacket.tStamp = endTime;
	mPacket.data[0] = MNOTEON+channel-CM_CHANNEL_BASE;
	mPacket.data[1] = noteNum;
	mPacket.data[2] = 0;								/* 0 velocity = Note Off */
	
	MIDIWritePacket(outputMMRefNum, &mPacket);
}


/* --------------------------------------------------------------------- MIDIConnected -- */
/*	Return True if a MIDI device is connected. */

Boolean MIDIConnected()
{
	Boolean	result;
	
	result = False;								/* FIXME: ABOVE CODE ALWAYS SAYS "True" ?HUH? */
	return result;
}


/* =========================================================== MIDI Feedback Functions == */

/* --------------------------------------------------------------- MIDIFBOn, MIDIFBOff -- */
/* If feedback is enabled, turn on MIDI apparatus. Exception: if the port is busy, do
nothing. */

void MIDIFBOn(Document *doc)
{
	if (doc->noteInsFeedback) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				CMFBOn(doc);
				break;
			default:
				break;
		}
	}
}

/* If feedback is enabled, turn off MIDI apparatus. Exception: if the port is busy, do
nothing. */

void MIDIFBOff(Document *doc)
{
	if (doc->noteInsFeedback) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				CMFBOff(doc);
				break;
			default:
				break;
		}
	}
}


/* ------------------------------------------------------- MIDIFBNoteOn, MIDIFBNoteOff -- */
/*	Start MIDI "feedback" note by sending a MIDI NoteOn command for the specified note
and channel. Exception: if the port is busy, do nothing. */

void MIDIFBNoteOn(
			Document *doc,
			short noteNum, short channel,
			short useIORefNum)
{
	if (doc->noteInsFeedback) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				CMFBNoteOn(doc, noteNum, channel, useIORefNum);
				break;
			default:
				break;
		}
		
		/* Delay a bit before returning. NB: this causes problems with our little-used
		   "chromatic" note input mode by slowing down AltInsTrackPitch. See comments
		   in TrackVMove.c. */
		   
		SleepTicks(2L);
	}
}

/*	End MIDI "feedback" note by sending a MIDI NoteOn command for the specified note
and channel with velocity 0 (which acts as NoteOff).  Exception: if the port is busy,
do nothing. */

void MIDIFBNoteOff(
			Document *doc,
			short	noteNum, short	channel,
			short useIORefNum)
{
	if (doc->noteInsFeedback) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				CMFBNoteOff(doc, noteNum, channel, useIORefNum);
				break;
			default:
				break;
		}
		
		/* Delay a bit before returning. NB: this causes problems with our little-used
		   "chromatic" note input mode by slowing down AltInsTrackPitch. See comments
		   in TrackVMove.c. */

		SleepTicks(2L);
	}
}


/* ----------------------------------------------------- AnyNoteToPlay, NoteToBePlayed -- */
/* Given a Sync, should at least one "note" be played? A given "note" should be played
if it's really a note (not a rest); is in a part that's not muted; and, if we're
interested only in selected notes, is selected. */
	
Boolean AnyNoteToPlay(Document *doc, LINK syncL, Boolean selectedOnly)
{
	LINK aNoteL;
	INT16 notePartn;
	
	if (!LinkSEL(syncL) && selectedOnly) return False;
	
	/* Is _any_ real note in the Sync in an unmuted part? */
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteREST(aNoteL)) continue;
		notePartn = Staff2Part(doc, NoteSTAFF(aNoteL));
		if (notePartn!=doc->mutedPartNum && !NoteREST(aNoteL)) return True;
	}
	
	return False;
}


/* Given a "note", should it be played? The answer is no if it's actually a rest or if
it belongs to a part that's currently muted. */

Boolean NoteToBePlayed(Document *doc, LINK aNoteL, Boolean selectedOnly)
{
	INT16 notePartn;

	if (!NoteSEL(aNoteL) && selectedOnly) return False;
	
	if (NoteREST(aNoteL)) return False;
	notePartn = Staff2Part(doc, NoteSTAFF(aNoteL));
	if (notePartn==doc->mutedPartNum) return False;
	
	return True;
}


/* ------------------------------------------------------------------ DisplayMIDIEvent -- */

void DisplayMIDIEvent(DoubleWord deltaT, Byte statusByte, Byte eventData1)
{
	Byte command;
	char strDisp[256], strMeta[64];
	
	strcpy(strMeta, "");
	command = MCOMMAND(statusByte);
	switch (command) {
		case MNOTEON:
			if (!MORE_DETAIL_SHOW) return;
			KludgeOS10p5Delay4Log(False);			/* Avoid bug in OS 10.5/10.6 Console */
			strcpy(strDisp, "NoteOn"); break;
		case MNOTEOFF:
			if (!MORE_DETAIL_SHOW) return;
			strcpy(strDisp, "NoteOff"); break;
		case MPOLYPRES:
			strcpy(strDisp, "PolyPressure"); break;
		case MCTLCHANGE:
			strcpy(strDisp, "ControlChange"); break;
		case MPITCHBEND:
			strcpy(strDisp, "PitchBend"); break;
			break;
		case MPGMCHANGE:
			strcpy(strDisp, "PatchChange"); break;
		case MCHANPRES:
			strcpy(strDisp, "ChannelPressure"); break;
		case MSYSEX:
			/* This handles only the original one-chunk form of SysEx message. For some
			   elegant code for collecting arbitrary chunks of SysEx, see midifile.c in
			   Tim Thompson's ancient program mftext. */
			   
			strcpy(strDisp, "SysEx"); break;
		case METAEVENT:									/* No. of data bytes is in the command */
			strcpy(strDisp, "Metaevent");
			switch (eventData1) {
				case ME_SEQTRACKNAME: strcpy(strMeta, "SEQ/TRACK NAME"); break;
				case ME_EOT: strcpy(strMeta, "END-OF-TRACK"); break;
				case ME_TEMPO: strcpy(strMeta, "TEMPO"); break;
				case ME_TIMESIG: strcpy(strMeta, "TIMESIG"); break;
				case ME_KEYSIG: strcpy(strMeta, "KEYSIG"); break;
				default: strcpy(strMeta, ""); break;
			}
			break;
		default:
			strcpy(strDisp, "(unknown)"); break;
	}
	LogPrintf(LOG_DEBUG, "deltaT=%ld %s data=%u=0x%x", deltaT, strDisp,
				eventData1, eventData1);
	if (strlen(strMeta)>0) LogPrintf(LOG_INFO, ":%s", strMeta);
	//if (command==METAEVENT && eventData1==ME_TEMPO) LogPrintf(LOG_INFO, ", TEMPO");
	LogPrintf(LOG_DEBUG, "  (DisplayMIDIEvent)\n");
}


/* -------------------------------------------- MIDI2EffectiveAcc, MIDI2EffectiveGRAcc -- */

/* Given a Sync and a note in that Sync, return the code for the note's correct effective
accidental, based on comparing the notation to its MIDI note number. If the notation and
note number are so far apart they can't be reconciled with an accidental, return
NRV_ERROR. */

short MIDI2EffectiveAcc(
			Document */*doc*/,
			short clefType, short octType,
			LINK /*syncL*/, LINK theNoteL
			)
{
	short halfLn;										/* Relative to the top of the staff */
	SHORTQD yqpit;
	short midCHalfLn, noteNum, delta;

	midCHalfLn = ClefMiddleCHalfLn(clefType);					/* Get middle C staff pos. */		
	yqpit = NoteYQPIT(theNoteL)+halfLn2qd(midCHalfLn);
	halfLn = qd2halfLn(yqpit);									/* Number of half lines from stftop */

	if (octType>0)
		noteNum = Pitch2MIDI(midCHalfLn-halfLn+noteOffset[octType-1], AC_NATURAL);
	else
		noteNum = Pitch2MIDI(midCHalfLn-halfLn, AC_NATURAL);
	
	delta = NoteNUM(theNoteL)-noteNum;
	if (delta<-2 || delta>2) return NRV_ERROR;
	return delta+AC_NATURAL;
}


/* Given a GRSync and a grace note in that GRSync, return the code for the grace note's
correct effective accidental, based on comparing the notation to its MIDI note number.
If the notation and note number are so far apart they can't be reconciled with an
accidental, return NRV_ERROR. */

short MIDI2EffectiveGRAcc(
			Document */*doc*/,
			short clefType, short octType,
			LINK /*syncL*/, LINK theGRNoteL
			)
{
	short halfLn;									/* Relative to the top of the staff */
	SHORTQD yqpit;
	short midCHalfLn, noteNum, delta;

	midCHalfLn = ClefMiddleCHalfLn(clefType);					/* Get middle C staff pos. */		
	yqpit = GRNoteYQPIT(theGRNoteL)+halfLn2qd(midCHalfLn);
	halfLn = qd2halfLn(yqpit);									/* Number of half lines from stftop */

	if (octType>0)
		noteNum = Pitch2MIDI(midCHalfLn-halfLn+noteOffset[octType-1], AC_NATURAL);
	else
		noteNum = Pitch2MIDI(midCHalfLn-halfLn, AC_NATURAL);
	
	delta = GRNoteNUM(theGRNoteL)-noteNum;
	if (delta<-2 || delta>2) return NRV_ERROR;
	return delta+AC_NATURAL;
}

