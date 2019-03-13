/* MIDIPlayWhileRec.c for Nightingale - routines to play the existing score while
recording - formerly in MIDIRecord.c.
		ShellSortNPBuf			BIMIDIAllocNoteBuffer	RecPlayAddAllClicks
		BIMIDIAddNote			RecPlayAddAllNotes		RecPreparePlayback
		RecPlayNotes
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

#include <CoreMIDI/MIDIServices.h>		/* for MIDIPacket */

Boolean BIMIDINoteAtTime(short noteNum, short channel, short velocity, long time);

/* ======================================= Routines to play along while recording == */
/* The following routines to play along while recording are for built-in MIDI only.
But many of them are independent of the MIDI technology, and it would be extremely
nice to have the whole package working for OMS and maybe MIDI Manager. */

typedef struct {
	short noteNum;
	short channel;
	long time;
	short velocity;	
} NOTEPLAYINFO;

void ShellSortNPBuf(NOTEPLAYINFO notePlayBuf[], short npBufInd);
static void GetClickTimeInfo(Document *doc, LINK recMeasL, LINK playToL, short nLeadInMeas,
								TCONVERT tConvertTab[], short tempoCount, long *pToffset,
								long *pClickLeadInDur, long *pLastStartTime);
static Boolean RecPlayAddAllClicks(long msPerBeat, long lastStartTime);
static Boolean BIMIDIAddNote(short noteNum, short channel, long startTime, long endTime,
								short velocity);
static void RecPlayAddAllNotes(Document *doc, LINK fromL, LINK toL, TCONVERT tConvertTab[],
								short tempoCount, long toffset);
short RecPreparePlayback(Document *doc, long msPerBeat, long *ptLeadInOffset);
Boolean RecPlayNotes(unsigned short	outBufSize);

/* --------------------------------------------------------------- ShellSortNPBuf -- */
/* ShellSortNPBuf does a Shell (diminishing increment) sort on <.times> of the
given NOTEPLAYINFO array, putting them into ascending order. The increments used
are powers of 2, which does not give the fastest possible execution, though the
difference should be negligible for a few hundred elements or less. See Knuth,
The Art of Computer Programming, vol. 2, pp. 84-95. */

void ShellSortNPBuf(
			NOTEPLAYINFO notePlayBuf[],
			short npBufInd)
{
	short nstep, ncheck, i, n; NOTEPLAYINFO temp;
	
	for (nstep = npBufInd/2; nstep>=1; nstep = nstep/2)
	{
/* Sort <nstep> subarrays by simple insertion */
		for (i = 0; i<nstep; i++)
		{
			for (n = i+nstep; n<npBufInd; n += nstep) {			/* Look for item <n>'s place */
				temp = notePlayBuf[n];									/* Save it */
				for (ncheck = n-nstep; ncheck>=0;
					  ncheck -= nstep)
					if (temp.time<notePlayBuf[ncheck].time)
						notePlayBuf[ncheck+nstep] = notePlayBuf[ncheck];
					else {
						notePlayBuf[ncheck+nstep] = temp;
						break;
					}
					notePlayBuf[ncheck+nstep] = temp;
			}
		}
	}
}

static short npBufInd;				/* Index of first free slot in notePlayBuf */
NOTEPLAYINFO *notePlayBuf;
static short npBufSize;				/* Size of notePlayBuf */

/* Return in a parameter the offset time, i.e., the absolute time since the beginning 
of the score when playing clicks starts, in milliseconds: note that this time might be
negative! Also return in parameters the lead-in time and start time of the last click. */

static void GetClickTimeInfo(
					Document *doc,
					LINK recMeasL,
					LINK playToL,
					short nLeadInMeas,
					TCONVERT tConvertTab[],
					short tempoCount,
					long *pToffset,						/* output, in milliseconds: may be negative! */
					long *pClickLeadInDur,				/* output, in milliseconds */
					long *pLastStartTime)				/* output, in milliseconds */
{
	CONTEXT context;
	long measDur, clickLeadInDur;
	long plFirstStartTime, plLastStartTime,		/* in PDUR ticks */
			toffset;											/* in milliseconds: may be negative! */
	LINK lastSyncL;

	GetContext(doc, recMeasL, 1, &context);
	measDur = TimeSigDur(N_OVER_D, context.numerator, context.denominator);
	clickLeadInDur = nLeadInMeas*measDur;
	*pClickLeadInDur = clickLeadInDur;

  	plFirstStartTime = MeasureTIME(recMeasL)-clickLeadInDur;
	toffset = PDur2RealTime(plFirstStartTime, tConvertTab, tempoCount);	/* Convert time to millisecs. */
	*pToffset = toffset;

	lastSyncL = SSearch(LeftLINK(playToL), SYNCtype, GO_LEFT);
  	plLastStartTime = SyncAbsTime(lastSyncL);
	*pLastStartTime = PDur2RealTime(plLastStartTime, tConvertTab, tempoCount);	/* Convert time to millisecs. */
	*pLastStartTime -= toffset;
}

/* Add to the play-while-recording buffer Note On and Note Off (or velocity zero Note
On) events for all the metronome clicks we might have to play. Should be called only if
we're using built-in MIDI and the metronome is set to use MIDI! */

static Boolean RecPlayAddAllClicks(
	long msPerBeat,
	long lastStartTime)						/* in milliseconds */
{
	long startTime, endTime;				/* in milliseconds */
	
	startTime = 0L;

	for ( ; startTime<=lastStartTime+msPerBeat; startTime += msPerBeat) {
		endTime = startTime+config.metroDur;
		if (!BIMIDIAddNote(config.metroNote, config.metroChannel, startTime, endTime,
									config.metroVelo)) {
			MayErrMsg("RecPlayAddAllClicks: BIMIDIAddNote failed: too many notes for buffer.");
			return False;
		}
	}
	
	return True;
}

static Boolean BIMIDIAddNote(short noteNum, short channel, long startTime, long endTime,
										short velocity)
{
	if (npBufSize<=npBufInd+2) return False;

	notePlayBuf[npBufInd].noteNum = noteNum;
	notePlayBuf[npBufInd].channel = channel;
	notePlayBuf[npBufInd].time = startTime;
	notePlayBuf[npBufInd].velocity = velocity;
	npBufInd++;	
	
	notePlayBuf[npBufInd].noteNum = noteNum;
	notePlayBuf[npBufInd].channel = channel;
	notePlayBuf[npBufInd].time = endTime;
	notePlayBuf[npBufInd].velocity = 0;
	npBufInd++;	

	return True;
}

#ifdef DEBUG_MIDIOUT
pascal void DBGMidiOut(short Midibyte, long TimeStamp, short * Result);
pascal void DBGMidiOut(short Midibyte, long TimeStamp, short * Result)
{
LogPrintf(LOG_NOTICE, "voidDBGMidiOut(0x%x, %ld)\n", Midibyte, TimeStamp);
	MidiOut(Midibyte, TimeStamp, Result);
}
#define MidiOut DBGMidiOut
#endif

#define MAX_TCONVERT 100						/* Max. no. of tempo changes we handle */
#define PLAY_CRITERION True						/* Someday may change for, e.g., punch in/out */

/* Add to the play-while-recording buffer Note On and Note Off (or velocity zero Note
On) events for all the notes we might have to play. */

static void RecPlayAddAllNotes(Document *doc, LINK fromL, LINK toL, TCONVERT tConvertTab[],
									short tempoCount, long toffset)
{
	SignedByte	partVelo[MAXSTAVES];
	Byte			partChannel[MAXSTAVES];
	short			partTransp[MAXSTAVES];
	Byte			channelPatch[MAXCHANNEL];
	short			useNoteNum,
					useChan, useVelo;
	LINK			pL, measL, aNoteL;
	long			playDur,								/* in PDUR ticks */
					plStartTime, plEndTime,					/* in PDUR ticks */
					startTime, endTime;						/* in milliseconds */
	
	GetPartPlayInfo(doc, partTransp, partChannel, channelPatch, partVelo);

	measL = SSearch(fromL, MEASUREtype, GO_LEFT);					/* starting measure */

	for (pL = fromL; pL!=toL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case MEASUREtype:
				measL = pL;
				break;
			case SYNCtype:
		  		plStartTime = MeasureTIME(measL)+SyncTIME(pL);
				startTime = PDur2RealTime(plStartTime, tConvertTab, tempoCount);	/* Convert play time to millisecs. */
				startTime -= toffset;
				aNoteL = FirstSubLINK(pL);

				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					if (PLAY_CRITERION) {
						GetNotePlayInfo(doc, aNoteL, partTransp, partChannel, partVelo,
												&useNoteNum, &useChan, &useVelo);
						playDur = TiedDur(doc, pL, aNoteL, False);
						plEndTime = plStartTime+playDur;						

						/* If it's a real note (not rest or continuation), add it */
						
						if (useNoteNum>=0) {
							endTime = PDur2RealTime(plEndTime, tConvertTab, tempoCount);	/* Convert time to millisecs. */
							endTime -= toffset;
							if (!BIMIDIAddNote(useNoteNum, useChan, startTime, endTime,
														useVelo)) {
								MayErrMsg("RecPlayAddAllNotes: BIMIDIAddNote failed: too many notes for buffer.");
								return;
							}
						}
					}
				}
				break;
			default:
				;
		}
}

#define NUM_LEADIN_MEAS_CLICK 2		/* No. of lead-in meas of metronome "click" */
#define NUM_LEADIN_MEAS_PLAY 0		/* No. of lead-in meas to play (not just click) */
#if NUM_LEADIN_MEAS_CLICK<NUM_LEADIN_MEAS_PLAY
#error  We cant handle NUM_LEADIN_MEAS_CLICK<NUM_LEADIN_MEAS_PLAY.
#endif

/* Prepare for playback during recording. For built-in MIDI, allocate the play-while-
recording buffer. For any driver, fill the appropriate buffer with all the notes and
metronome "click"s we might have to play. Assumes that recording starts at the
beginning of a Measure, and that the first note to play is no earlier than the first
click. Return value is the size of the buffer, or -1 if there's a problem. */

short RecPreparePlayback(Document *doc, long msPerBeat, long *ptLeadInOffset)
{
	short i, measNum;
	LINK recMeasL, playFromL, playToL;
	long maxRecTime, toffset, lastStartTime;
	TCONVERT tConvertTab[MAX_TCONVERT];
	
	recMeasL = EitherSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, False);

	measNum = GetMeasNum(doc, doc->selStartL);
	playFromL = MNSearch(doc, doc->headL, measNum-NUM_LEADIN_MEAS_PLAY-doc->firstMNNumber,
							GO_RIGHT, True);
	if (!playFromL) playFromL = LSSearch(doc->selStartL, MEASUREtype, ANYONE,
							GO_RIGHT, False);
	maxRecTime = MeasureTIME(recMeasL) + MAX_SAFE_MEASDUR;
	playToL = AbsTimeSearchRight(doc, doc->selStartL, maxRecTime);

	switch (useWhichMIDI) {
		case MIDIDR_CM:
			AlwaysErrMsg("RecPreparePlayback: only built-in MIDI is implemented.");
			return -1;
		default:
			;
	}

	/*
	 * Make a time conv. table so we can call PDur2RealTime, but ignore tempo changes
	 * by pretending there's just one entry in the table.
	 */
	(void)MakeTConvertTable(doc, doc->selStartL, playToL, tConvertTab, MAX_TCONVERT);

	if (playToL==NILINK) playToL = doc->tailL;
	GetClickTimeInfo(doc, recMeasL, playToL, NUM_LEADIN_MEAS_CLICK, tConvertTab,
							1, &toffset, ptLeadInOffset, &lastStartTime);
	if (config.metroViaMIDI)
		RecPlayAddAllClicks(msPerBeat, lastStartTime);
	RecPlayAddAllNotes(doc, playFromL, playToL, tConvertTab, 1, toffset);
	
	/*
	 * Sort the note starts and note ends by time. This is convenient for anti-synth-choking
	 * (just below) and required by MIDI Pascal's MidiOut.
	 */
	ShellSortNPBuf(notePlayBuf, npBufInd);

	/*
	 * To avoid choking slow synths or other MIDI gear, disperse times. If any play
	 * time is equal to the previous one, increase it slightly. If any time is less than
	 * the previous one, since we just sorted them, it can only be that they started equal
	 * and we already increased the previous one, so do the same thing.
	 */
	for (i = 1; i<npBufInd; i++) {
		if (notePlayBuf[i].time<=notePlayBuf[i-1].time)
			notePlayBuf[i].time = notePlayBuf[i-1].time+1;
	}
	
	return npBufSize;
}

/* Send Note On and Note Off (or velocity zero Note On) events for all notes, including
metronome "clicks". Return immediately, since playing is asynchronous. Assumes the event
times are sorted.

If we succeed, return True; if not (probably not enough memory in the buffer), False. */

Boolean RecPlayNotes(unsigned short outBufSize)
{
	unsigned short outBufCount;
	short i; long time;

	for (i = 0; i<npBufInd; i++) {
		switch (useWhichMIDI) {
			case MIDIDR_CM:
				AlwaysErrMsg("RecPlayNotes: only built-in MIDI is implemented.");
				return False;
			default:
				;
		}
	}
	
	return True;
	
Overflow:
	MayErrMsg("RecPlayNotes: BIMIDINoteAtTime failed (probably buffer overflow).");
	return False;
}
