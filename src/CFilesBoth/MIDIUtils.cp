/* MIDIUtils.c for Nightingale - rev. for v.2000 */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1997-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include <CoreMIDI/MIDIServices.h>		/* for MIDIPacket */

#include "MidiMap.h"
#include "CarbonStubs.h"

/* With THINK C 7 on my PowerBook 540 and my PowerBook G3/400, Nightingale links okay,
but trying to run results in a mysterious data segment overflow apparently resulting
from the fact that Symantec's ANSI—small library takes 600+ bytes more on those
machines, even though the file containing the library appears to be identical! Comment
the following kludge in to avoid it. */
static MIDIEvent	eventList[MAXEVENTLIST];
static CMMIDIEvent	cmEventList[MAXEVENTLIST];

static short		lastEvent;

static Boolean	InsertEvent(short note, SignedByte channel, long endTime, short ioRefNum);

void		KillEventList(void);
void		SendAllNotesOff(void);

/* -------------------------------------------------------------- UseMIDIChannel -- */
/* Return the MIDI channel number to use for the given part. */

short UseMIDIChannel(Document *doc, short	partn)
{
	short				useChan;
	LINK				partL;
	PPARTINFO		pPart;
	
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


/* ---------------------------------------------------------------- NoteNum2Note -- */
/* Return the note (not rest) in the given sync and voice with the given MIDI note
number, if there is one; else return NILINK. If the sync and voice have more than
one note with the same note number, finds the first one. Intended for finding ties'
notes. */

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


/* --------------------------------------------------------------------- TiedDur -- */
/* Return total performance duration of <aNoteL> and all following notes tied to
<aNoteL>. If <selectedOnly>, only includes tied notes until the first unselected
one. We use the note's <playDur> only for the last note of the series, its NOTATED
duration for all the others. */

long TiedDur(Document */*doc*/, LINK syncL, LINK aNoteL, Boolean selectedOnly)
{
	LINK			syncPrevL, aNotePrevL, continL, continNoteL;
	short			voice;
	long			dur, prevDur;
	
	voice = NoteVOICE(aNoteL);
	
	dur = 0L;
	syncPrevL = syncL;
	aNotePrevL = aNoteL;
	while (NoteTIEDR(aNotePrevL)) {
		continL = LVSearch(RightLINK(syncPrevL),			/* Tied note should always exist */
						SYNCtype, voice, GO_RIGHT, FALSE);
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


/* -------------------------------------------------------------- UseMIDINoteNum -- */
/* Return the MIDI note number that should be used to play the given note,
considering transposition. If the "note" is a rest or the continuation of a
tied note, or has velocity 0, it should not be played: indicate by returning
-1. */

short UseMIDINoteNum(Document *doc, LINK aNoteL, short transpose)
{
	short midiNote;
	PANOTE aNote;
	LINK partL;
	PPARTINFO pPart;
	Byte patchNum;

	aNote = GetPANOTE(aNoteL);
	if (aNote->rest || aNote->tiedL					/* Don't play rests and continuations */
	||  aNote->onVelocity==0)							/* ...and notes w/velocity 0 */
		midiNote = -1;
	else {
		midiNote = aNote->noteNum;
		if (doc->transposed)								/* Handle "transposed score" */
			midiNote += transpose;
		if (midiNote<1) midiNote = 1;					/* Clip to legal range */
		if (midiNote>MAX_NOTENUM) midiNote = MAX_NOTENUM;
	}
#ifdef TARGET_API_MAC_CARBON_MIDI
	partL = Staff2PartLINK(doc, NoteSTAFF(aNoteL));
	pPart = GetPPARTINFO(partL);
	patchNum = pPart->patchNum;
	if (IsPatchMapped(doc, patchNum)) {
		midiNote = GetMappedNoteNum(doc, midiNote);
	}	
#endif
	return midiNote;
}


/* ------------------------------------------------------------- GetModNREffects -- */
/* Given a note, if it has any modifiers, get information about how its
modifiers should affect playback, and return TRUE. If it has no modifiers,
just return FALSE. NB: The time factor is unimplemented. */

Boolean GetModNREffects(LINK aNoteL, short *pVelOffset, short *pDurFactor,
			short *pTimeFactor)
{
	LINK		aModNRL;
	short		velOffset, durFactor, timeFactor;

	if (!config.useModNREffects)
		return FALSE;

	aModNRL = NoteFIRSTMOD(aNoteL);
	if (!aModNRL) return FALSE;

	velOffset = 0;
	durFactor = timeFactor = 100;

	for ( ; aModNRL; aModNRL = NextMODNRL(aModNRL)) {
		Byte code = ModNRMODCODE(aModNRL);
		if (code>31) continue;					/* Silent failure: arrays sized for 32 items. */
		velOffset += modNRVelOffsets[code];
// FIXME: What if more than one modifier?
		durFactor = modNRDurFactors[code];
//		timeFactor = modNRTimeFactors[code];
	}

	*pVelOffset = velOffset;
	*pDurFactor = durFactor;
	*pTimeFactor = timeFactor;

	return TRUE;
}


/* ------------------------------------------------------- Tempo/time conversion -- */

/* Given a TEMPO object, return its "timeScale", i.e., tempo in PDUR ticks per minute. */

long Tempo2TimeScale(LINK tempoL)
{
	PTEMPO pTempo; long tempo, beatDur, timeScale;
	
	pTempo = GetPTEMPO(tempoL);
	tempo = pTempo->tempo;
	beatDur = Code2LDur(pTempo->subType, (pTempo->dotted? 1 : 0));
	timeScale = tempo*beatDur;
	return timeScale;
}


/* Return the tempo in effect at the given LINK, if any. The tempo is computed from the
last preceding tempo mark; if there is no preceding tempo mark, it's computed from the
default tempo. In any case, the tempo is expressed as a "timeScale", i.e., in PDUR ticks
per minute. */

long GetTempo(
			Document */*doc*/,		/* unused */
			LINK startL)
{
	LINK tempoL; long timeScale;

	timeScale = (long)config.defaultTempo*DFLT_BEATDUR;	/* Default in case no tempo marks found */

	if (startL) {
		tempoL = LSSearch(startL, TEMPOtype, ANYONE, GO_LEFT, FALSE);
		if (tempoL) timeScale = Tempo2TimeScale(tempoL);
	}

	return timeScale;
}


/* Convert PDUR ticks to millisec., with tempo varying as described by tConvertTab[].
If we can't convert it, return -1L. */

long PDur2RealTime(
				long t,						/* time in PDUR ticks */
				TCONVERT	tConvertTab[],
				short tabSize)
{
	short i; long msAtPrevTempo, msSincePrevTempo;
	
	/*
	 * If the table is empty, just return zero. Otherwise, find the 1st entry in the
	 * table for a PDUR time after <t>; if there's no such entry, assume the last entry 
	 * in the table applies.
	 */	
	if (tabSize<=0) return 0L;
	
	for (i = 1; i<tabSize; i++)
		if (tConvertTab[i].pDurTime>t) break;
	if (tConvertTab[i].pDurTime<=t) i = tabSize;	/* ??IS i GUARANTEED TO BE MEANINGFUL? */
	msAtPrevTempo = tConvertTab[i-1].realTime;
	
	msSincePrevTempo = PDUR2MS(t-tConvertTab[i-1].pDurTime, tConvertTab[i-1].microbeats);
	if (msSincePrevTempo<0) return -1L;
	
	return msAtPrevTempo+msSincePrevTempo;
}


/* Build a table of tempi in effect in the given range, sorted by increasing time.
Return value is the size of the table, or -1 if the table is too large. Intended to
get information for changing tempo during playback. */

short MakeTConvertTable(
				Document *doc,
				LINK fromL, LINK toL,			/* range to be played */
				TCONVERT	tConvertTab[],
				short maxTabSize
				)
{
	short		tempoCount;
	LINK		pL, measL, syncL, syncMeasL;
	long		microbeats,							/* microsec. units per PDUR tick */
				timeScale,							/* PDUR ticks per minute */
				pDurTime;							/* in PDUR ticks */

	tempoCount = 0;

	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:
				measL = pL;
				break;
			case SYNCtype:
			  	/* If no tempo found yet, initial tempo is the last previous one. */
			  	
			  	if (tempoCount==0) {
					syncL = LSSearch(fromL, SYNCtype, ANYONE, GO_RIGHT, FALSE);
					timeScale = GetTempo(doc, syncL);				/* OK even if syncL is NILINK */
					microbeats = TSCALE2MICROBEATS(timeScale);
					tConvertTab[0].microbeats = microbeats;
					tConvertTab[0].pDurTime = 0L;
					tConvertTab[0].realTime = 0L;
					tempoCount = 1;
				}
				break;
			case TEMPOtype:
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
					 
				tConvertTab[tempoCount].realTime = PDur2RealTime(pDurTime,
																				 tConvertTab, tempoCount);
				tempoCount++;
				break;
			default:
				;
		}
	}

#ifdef TDEBUG
{	short i;
	for (i = 0; i<tempoCount; i++)
		DebugPrintf("tConvertTab[%d].microbeats=%ld pDurTime=%ld realTime=%ld\n",
			i, tConvertTab[i].microbeats, tConvertTab[i].pDurTime, tConvertTab[i].realTime);
}
#endif
	return tempoCount;
}


/* ------------------------------------------- MIDI Manager/Driver glue functions -- */
/* The following functions support Apple Core MIDI */

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

#define TURN_PAGES_WAIT TRUE	/* After "turning" page, TRUE=resume in tempo, FALSE="catch up" */

long GetMIDITime(long pageTurnTOffset)
{
	long time;

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
		KillEventList();												/* stop all notes being played */
	
	StopMIDITime();
}


/* --------------------------------------------------------- Event list functions -- */

/* Initialize the Event list to empty. */

void InitEventList()
{
	lastEvent = 0;
}

/*	Insert the specified note into the event list. If we succeed, return TRUE; if
we fail (because the list is full), give an error message and return FALSE. */

static Boolean InsertEvent(short note, SignedByte channel, long endTime, short ioRefNum)
{
	short			i;
	MIDIEvent	*pEvent;
	char			fmtStr[256];

	/* Find first free slot in list, which may be at lastEvent (end of list) */
	
	for (i=0, pEvent=eventList; i<lastEvent; i++,pEvent++)
		if (pEvent->note == 0) break;
	
	/* Insert note into free slot, or append to end of list if there's room */
	
	if (i<lastEvent || lastEvent++<MAXEVENTLIST) {
		pEvent->note = note;
		pEvent->channel = channel;
		pEvent->endTime = endTime;
		pEvent->omsIORefNum = ioRefNum;
		return TRUE;
	}
	else {
		lastEvent--;
		GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 13);		/* "can play only %d notes at once" */
		sprintf(strBuf, fmtStr, MAXEVENTLIST);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
}

/*	Insert the specified note into the event list. If we succeed, return TRUE; if
we fail (because the list is full), give an error message and return FALSE. */

static Boolean CMInsertEvent(short note, SignedByte channel, long endTime, long ioRefNum)
{
	short			i;
	CMMIDIEvent	*pEvent;
	char			fmtStr[256];

	/* Find first free slot in list, which may be at lastEvent (end of list) */
	
	for (i=0, pEvent=cmEventList; i<lastEvent; i++,pEvent++)
		if (pEvent->note == 0) break;
	
	/* Insert note into free slot, or append to end of list if there's room */
	
	if (i<lastEvent || lastEvent++<MAXEVENTLIST) {
		pEvent->note = note;
		pEvent->channel = channel;
		pEvent->endTime = endTime;
		pEvent->cmIORefNum = ioRefNum;
		return TRUE;
	}
	else {
		lastEvent--;
		GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 13);		/* "can play only %d notes at once" */
		sprintf(strBuf, fmtStr, MAXEVENTLIST);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
}

/*	Checks eventList[] to see if any notes are ready to be turned off; if so,
frees their slots in the eventList and (if we're not using MIDI Manager) turns
them off. Returns TRUE if the list is empty. */

Boolean CheckEventList(long pageTurnTOffset)
{
	Boolean		empty;
	MIDIEvent	*pEvent;
	short			i;
	long			t;
	
	t = GetMIDITime(pageTurnTOffset);
	empty = TRUE;
	for (i=0, pEvent = eventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
			empty = FALSE;
			if (pEvent->endTime<=t) {							/* note is done, t = now */
				EndNoteNow(pEvent->note, pEvent->channel, pEvent->omsIORefNum);
				pEvent->note = 0;													/* slot available now */
			}
		}

	return empty;
}

/*	Checks eventList[] to see if any notes are ready to be turned off; if so,
frees their slots in the eventList and (if we're not using MIDI Manager) turns
them off. Returns TRUE if the list is empty. */

Boolean CMCheckEventList(long pageTurnTOffset)
{
	Boolean		empty;
	CMMIDIEvent	*pEvent;
	short			i;
	long			t;
	
	t = GetMIDITime(pageTurnTOffset);
	empty = TRUE;
	for (i=0, pEvent = cmEventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
			empty = FALSE;
			if (pEvent->endTime<=t) {							/* note is done, t = now */
				CMEndNoteNow(pEvent->cmIORefNum, pEvent->note, pEvent->channel);
				pEvent->note = 0;													/* slot available now */
			}
		}

	return empty;
}

/*	Turn off all notes in eventList[] and re-initialize it. */

void CMKillEventList()
{
	CMMIDIEvent	*pEvent;
	short			i;
	
	for (i = 0, pEvent = cmEventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
			CMEndNoteNow(pEvent->cmIORefNum, pEvent->note, pEvent->channel);
		}

	InitEventList();
}



/*	Turn off all notes in eventList[] and re-initialize it. */

void KillEventList()
{
	MIDIEvent	*pEvent;
	short			i;
	
	for (i = 0, pEvent = eventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
			EndNoteNow(pEvent->note, pEvent->channel, pEvent->omsIORefNum);
		}

	InitEventList();
}

/* ------------------------------------------------------------- GetPartPlayInfo -- */

void GetPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
							Byte channelPatch[], SignedByte partVelo[])
{
	short i; LINK partL; PARTINFO aPart;

	for (i = 1; i<=MAXCHANNEL; i++)
		channelPatch[i] = MAXPATCHNUM+1;						/* Initialize to illegal value */
	
	partL = FirstSubLINK(doc->headL);
	for (i = 0; i<=LinkNENTRIES(doc->headL)-1; i++, partL = NextPARTINFOL(partL)) {
		aPart = GetPARTINFO(partL);
		partVelo[i] = aPart.partVelocity;
		partChannel[i] = UseMIDIChannel(doc, i);
		channelPatch[partChannel[i]] = aPart.patchNum;
		partTransp[i] = aPart.transpose;
	}
}

/* ------------------------------------------------------------- GetNotePlayInfo -- */
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
	*pUseVelo = doc->velocity+aNote->onVelocity;
	if (doc->polyTimbral) *pUseVelo += partVelo[partn];
	
	if (*pUseVelo<1) *pUseVelo = 1;
	if (*pUseVelo>MAX_VELOCITY) *pUseVelo = MAX_VELOCITY;
}

/* --------------------------------------------------------------- SetMIDIProgram -- */

#define PATCHNUM_BASE 1			/* Some synths start numbering at 1, some at 0 */

/* ---------------------------------------- StartNoteNow,EndNoteNow,EndNoteLater -- */
/* Functions to start and end notes; handle OMS, FreeMIDI, MIDI Manager, and built-in
MIDI (MIDI Pascal or MacTutor driver). Caveat: EndNoteLater does not communicate with
the MIDI system, it simply adds the note to our event-list routines' queue. */

OSStatus StartNoteNow(short noteNum, SignedByte channel, SignedByte velocity, short ioRefNum)
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

OSStatus EndNoteNow(short noteNum, SignedByte channel, short ioRefNum)
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

Boolean EndNoteLater(
				short noteNum,
				SignedByte channel,			/* 1 to MAXCHANNEL */
				long endTime,
				short ioRefNum)
{
	return InsertEvent(noteNum, channel, endTime, ioRefNum);
}

Boolean CMEndNoteLater(
				short noteNum,
				SignedByte channel,			/* 1 to MAXCHANNEL */
				long endTime,
				long ioRefNum)
{
	return CMInsertEvent(noteNum, channel, endTime, ioRefNum);
}




/* -------------------------------------------- MMStartNoteNow, MMEndNoteAtTime -- */
/* For MIDI Manager: start the note now, end the note at the given time. */
 
void MMStartNoteNow(
				short			noteNum,
				SignedByte	channel,			/* 1 to MAXCHANNEL */
				SignedByte	velocity
				)
{
	MMMIDIPacket mPacket;

	mPacket.flags = MM_STD_FLAGS;	
	mPacket.len = MM_NOTE_SIZE;
			
	mPacket.tStamp = MM_NOW;
	mPacket.data[0] = MNOTEON+channel-1;
	mPacket.data[1] = noteNum;
	mPacket.data[2] = velocity;
	MIDIWritePacket(outputMMRefNum, &mPacket);
}

void MMEndNoteAtTime(
				short			noteNum,
				SignedByte	channel,			/* 1 to MAXCHANNEL */
				long			endTime
				)
{
	MMMIDIPacket mPacket;
	
	mPacket.flags = MM_STD_FLAGS;	
	mPacket.len = MM_NOTE_SIZE;
			
	mPacket.tStamp = endTime;
	mPacket.data[0] = MNOTEON+channel-1;
	mPacket.data[1] = noteNum;
	mPacket.data[2] = 0;										/* 0 velocity = Note Off */
	
	MIDIWritePacket(outputMMRefNum, &mPacket);
}


/* --------------------------------------------------------------- MIDIConnected -- */
/*	Return TRUE if a MIDI device is connected. */

Boolean MIDIConnected()
{
	Boolean	result;
	
	result = FALSE;									/* ??ABOVE CODE ALWAYS SAYS "TRUE" */
	return result;
}


/* ====================================================== MIDI Feedback Functions == */

/* -------------------------------------------------------------------- MIDIFBOn -- */
/*	If feedback is enabled, turn on MIDI stuff. Exception: if the port is busy,
do nothing. */

void MIDIFBOn(Document *doc)
{
	if (doc->feedback) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				CMFBOn(doc);
				break;
			default:
				break;
		}
	}
}


/* ------------------------------------------------------------------- MIDIFBOff -- */
/*	If feedback is enabled, turn off MIDI stuff. Exception: if the port is busy,
do nothing. */

void MIDIFBOff(Document *doc)
{
	if (doc->feedback) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				CMFBOff(doc);
				break;
			default:
				break;
		}
	}
}


/* ---------------------------------------------------------------- MIDIFBNoteOn -- */
/*	Start MIDI "feedback" note by sending a MIDI NoteOn command for the specified
note and channel. Exception: if the port is busy, do nothing. */

void MIDIFBNoteOn(
				Document *doc,
				short	noteNum, short	channel,
				short	useIORefNum)			/* Ignored unless we're using OMS or FreeMIDI */
{
	if (doc->feedback) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				CMFBNoteOn(doc, noteNum, channel, useIORefNum);
				break;
			default:
				break;
		}
		
		/*
		 * Delay a bit before returning. NB: this causes problems with our little-used
		 * "chromatic" note input mode by slowing down AltInsTrackPitch. See comments
		 * in TrackVMove.c.
		 */
		SleepTicks(2L);
	}
}


/* --------------------------------------------------------------- MIDIFBNoteOff -- */
/*	End MIDI "feedback" note by sending a MIDI NoteOn command for the specified
note and channel with velocity 0 (which acts as NoteOff).  Exception: if the port
is busy, do nothing. */

void MIDIFBNoteOff(
				Document *doc,
				short	noteNum, short	channel,
				short useIORefNum)			/* Ignored unless we're using OMS or FreeMIDI */
{
	if (doc->feedback) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				CMFBNoteOff(doc, noteNum, channel, useIORefNum);
				break;
			default:
				break;
		}
		
		/*
		 * Delay a bit before returning. NB: this causes problems with our little-used
		 * "chromatic" note input mode by slowing down AltInsTrackPitch. See comments
		 * in TrackVMove.c.
		 */
		SleepTicks(2L);
	}
}

