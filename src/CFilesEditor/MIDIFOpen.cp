/*	MIDIFOpen.c for Nightingale: routines to read Standard MIDI Files */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* From OpcodeNightingaleData.h: variables shared with other Open MIDI File modules */

short infile;
long eofpos;


/* The format of Standard MIDI Files was originally described in a document of that
name available from the International MIDI Association. As of this writing (March 2017),
it appears that the "official" descripion is only in a 1996 document plus four
supplements of 1998 thru 2001 available from the MIDI Association, and only after
registering with them. Much easier to get the unoficial (and not-quite-up-to-date) 1999
"Standard MIDI-File Format Spec. 1.1, updated", found in various places on the Web.

We go thru two steps in converting a MIDI file to Nightingale score. In step 1, we
translate the MIDI file to our "MIDNight" format: see MF2MIDNight for comments on this.
In step 2, we generate Nightingale data structure from the MIDNight data, which is much
more difficult: see comments after the heading "MIDNight --> Nightingale".

This file also contains some utility functions for use by MIDI-file-reading user-
interface-level routines. */

static short errCode;

Word mfNTracks, mfTimeBase;
long qtrNTicks;					/* Ticks per quarter in Nightingale (not in the MIDI file!) */
			
/* Prototypes for local functions */

static Boolean eof(Word);
static Word getw(Word);
static DoubleWord getl(Word);
static void SkipChunksUntil(DoubleWord);

static Boolean IsTimeSigBad(short, short);

static DoubleWord GetVarLen(Byte [], DoubleWord *);

static void InitMFEventQue(DoubleWord, Byte);
static void SaveMFEventQue(DoubleWord *, Byte *);
static DoubleWord GetDeltaTime(Byte [], DoubleWord *);
static short GetNextMFEvent(DoubleWord *, Byte [], Boolean *);
static Boolean GetNoteOff(DoubleWord, Boolean, Byte [], long *pDuration);

/* -------------------------------------------------------------------------------------- */

/* Variables ending in <MF> are for our "MIDI Event queue"; they should never
be stored into except by the routines that manage the MIDI Event queue, viz.,
Init/SaveMFEventQue and GetNextMFEvent. */

Byte *pChunkMF;						/* MIDI file track pointer */
Word lenMF;							/* MIDI file track length */
DoubleWord locMF;					/* MIDI file track current position */
Byte statusByteMF=0;				/* MIDI file track status in case of running status */

/* -------------------------------------------------------------------------------------- */

#define DBG DETAIL_SHOW

#define TEMPO_WINDOW 65				/* in units given by <timeBase> in file header */

/* ----------------------------------------------------------------- Utility functions -- */

static Boolean eof(Word refnum)
{
	long fpos;
	GetFPos(refnum,&fpos);
	return (fpos==eofpos);
}


static Word getw(Word refnum)
{
	Word buffer;
	long length;

	length = 2;
	errCode = FSRead(refnum,&length,&buffer);
	return (buffer);
}


static DoubleWord getl(Word refnum)
{
	DoubleWord buffer;
	long length;

	length = 4;
	errCode = FSRead(refnum,&length,&buffer);
	return (buffer);
}


/* Find the next <type> chunk and leave file positioned at its first delta-time byte. */

static void SkipChunksUntil(DoubleWord type)
{
	DoubleWord len;
	
	while (getl(infile) != type) {
		if (errCode != noError) return;
		len = getl(infile);								/* After type is the 4-byte chunk length */
		if (errCode != noError) return;
		errCode = SetFPos(infile,0,len);
		if (eof(infile)) errCode = eofErr;
		if (errCode != noError) return;
	}
}


static Boolean IsTimeSigBad(short tsNum, short tsDenom)
{
	char fmtStr[256];

	if (TSNUMER_BAD(tsNum) || TSDENOM_BAD(tsDenom)) {
		GetIndCString(fmtStr, MIDIFILE_STRS, 20);    /* "Nightingale can't handle the time signature %d/%d." */
		sprintf(strBuf, fmtStr, tsNum, tsDenom); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return True;
	}
	if (TSDUR_BAD(tsNum, tsDenom)) {
		GetIndCString(fmtStr, MIDIFILE_STRS, 21);    /* "Time signature %d/%d has too long a duration for Nightingale." */
		sprintf(strBuf, fmtStr, tsNum, tsDenom); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return True;
	}
	
	return False;
}


/* ---------------------------------------------------------------------- ReadMFHeader -- */

Boolean ReadMFHeader(Byte *pFormat, Word *pnTracks, Word *pTimeBase)
{
	DoubleWord len;
	
	SkipChunksUntil('MThd');
	if (errCode!=noError) return False;
	len = getl(infile);
	if (errCode!=noError) return False;
	*pFormat = getw(infile);
	if (errCode!=noError) return False;
	*pnTracks = getw(infile);
	if (errCode!=noError) return False;
	*pTimeBase = getw(infile);
	if (errCode!=noError) return False;
	if (len!=6) {
		errCode = SetFPos(infile, 0, len-6);
		if (errCode!=noError) return False;
	}

	LogPrintf(LOG_NOTICE, "MThd len=%ld format=%d nTracks=%d timeBase=%d (qtrNTicks=%d)  (ReadMFHeader)\n",
					len, *pFormat, *pnTracks, *pTimeBase, qtrNTicks);
	return True;
}


/* ------------------------------------------------------------------------- GetVarLen -- */
/* Get the value of a MIDI file "variable-length number" whose first byte is at
chunk[*pLoc], and increment *pLoc to point to the first byte after the number. According
to the Standard MIDI File spec, a variable-length number must fit in four bytes: it's
unsigned, and since there are 7 significant bits per byte, it must be less than 2**28.
If the number occupies more than four bytes, we return an illegal value (0xFFFFFFFF). */

static DoubleWord GetVarLen(Byte chunk[], DoubleWord *pLoc)
{
	DoubleWord value;
	Word i;
	Byte addend;
	
	value = 0L;
	
	/* Accumulate significant bits and watch the top bit for the end-of-number flag. */
	
	for (i = 1; i<=4; i++, (*pLoc)++) {
		addend = chunk[*pLoc] & 0x7F;
		value = (value<<7)+addend;
		if (!(chunk[*pLoc] & 0x80)) {
			(*pLoc)++;
			return value;
		}
	}
	
	/* Something is wrong: the number seems to occupy more than four bytes. */
	
	return 0xFFFFFFFF;
}


#define MAX_DENOM_POW2 6		/* This and denomTab must agree with the TSDENOM_BAD macro */

/* In versions before 3.1b41, we limited track size to 65535. But as far as I know,
the only reason was to insure no more than 65535 notes/track so note indices fit in
the .link field of a LINKTIMEINFO; limiting track size to the same value is much too
conservative. */

#define MAX_TRACK_SIZE 2*65535L		/* in bytes */

/* ------------------------------------------------------------------------- ReadTrack -- */
/* Allocate a block and read the next track of the current MIDI file into it. If the
track is too large for us, doesn't exist, or can't be read, return 0; else return the
length of the track. */

Word ReadTrack(Byte **ppChunk)
{
	long len, actualLen;
	Byte *pChunk;
	char fmtStr[256];
	
	SkipChunksUntil('MTrk');
	if (errCode!=noError) {
		LogPrintf(LOG_ERR, "Can't read MIDI file track: can't find 'MTrk' chunk. errCode=%d  (ReadTrack)\n",
					errCode);
		return 0;
	}
	len = getl(infile);
	if (len>MAX_TRACK_SIZE) {
		GetIndCString(fmtStr, MIDIFILE_STRS, 22);    /* "The MIDI file has a track of %ld bytes; the largest..." */
		sprintf(strBuf, fmtStr, len, MAX_TRACK_SIZE); 
		CParamText(strBuf, "", "", "");
		StopInform(READMIDI_ALRT);
		return 0;
	}
	if (errCode!=noError) {
		LogPrintf(LOG_ERR, "Can't read MIDI file track: getl failed. errCode=%d  (ReadTrack)\n",
					errCode);
		return 0;
	}
	pChunk = (Byte *)NewPtr(len);
	if (!GoodNewPtr((Ptr)pChunk)) {
		OutOfMemory(len);
		return 0;
	}
	actualLen = len;
	if ((errCode = FSRead(infile, &actualLen, pChunk)) == 0)
		if (actualLen<len) errCode = eofErr;
	if (errCode || actualLen<=0) {
		LogPrintf(LOG_ERR, "Can't read MIDI file track: problem with length. errCode=%d len=%ld actualLen=%ld  (ReadTrack)\n",
					errCode, len, actualLen);
		return 0;
	}
	*ppChunk = pChunk;
	return (Word)actualLen;
}


/* --------------------------------------------------------------- Init/SaveMFEventQue -- */
/* Initialize or save the MIDI File event queue environment. */

static void InitMFEventQue(DoubleWord initLoc, Byte initStatus)
{
	locMF = initLoc;
	statusByteMF = initStatus;
}

static void SaveMFEventQue(DoubleWord *pLoc, Byte *pStatus)
{
	*pLoc = locMF;
	*pStatus = statusByteMF;
}


/* ---------------------------------------------------------------------- GetDeltaTime -- */
/* GetDeltaTime calls GetVarLen, then skips over any "no-ops" that have replaced
following commands and adds in their delta times, and returns the resulting value.
If an error is found, it returns 0xFFFFFFFF. */

static DoubleWord GetDeltaTime(Byte chunk[], DoubleWord *pLoc)
{
	DoubleWord varLenValue, deltaT=0L;
	char fmtStr[256];
	
Another:
	varLenValue = GetVarLen(chunk, pLoc);
	if (varLenValue==0xFFFFFFFF) {
		/* FIX ME: Should also give the track no. in this message. */
		GetIndCString(fmtStr, MIDIFILE_STRS, 40);    	/* "Illegal variable-length number at..." */
		sprintf(strBuf, fmtStr, (long)locMF); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return (DoubleWord)0xFFFFFFFF;
	}
	deltaT += varLenValue;

	if (locMF>lenMF) return -1L;						/* If no more in the track */
	if (chunk[locMF]!=MACTIVESENSE) return deltaT;

	while (chunk[locMF]==MACTIVESENSE) locMF++;			/* skip our "no-ops" */
	if (locMF>lenMF) return -1L;						/* If no more in the track */
	goto Another;
}


/* -------------------------------------------------------------------- GetNextMFEvent -- */
/* Try to find the next MIDI File event. If successful, deliver its delta time and
event data and return OP_COMPLETE, and leave <locMF> pointing at the first byte after
the event. If there are no more MIDI events, just return NOTHING_TO_DO. If there's a
problem, return FAILURE. */

static short GetNextMFEvent(DoubleWord *pDeltaTime,
										Byte eventData[],
										Boolean *pRunning)		/* True=used running status */
{
	Byte command, i;
	Word n;
	DoubleWord deltaT;
	short opStatus=FAILURE;

	if (locMF>=lenMF) return NOTHING_TO_DO;			/* If equal, can't have data AND delta time remaining */
	
	deltaT = GetDeltaTime(pChunkMF, &locMF);
	if (deltaT==(DoubleWord)0xFFFFFFFF) return FAILURE;

	*pRunning = !(pChunkMF[locMF] & MSTATUSMASK);
	if (!(*pRunning)) {
		statusByteMF = pChunkMF[locMF];				/* Pick up and move past explicit status */
		locMF++;
	}

	eventData[0] = statusByteMF;

	if (DBG) DisplayMIDIEvent(deltaT, statusByteMF, pChunkMF[locMF]);
	command = MCOMMAND(statusByteMF);
	switch (command) {
		case MNOTEON:
		case MNOTEOFF:
		case MPOLYPRES:
		case MCTLCHANGE:
		case MPITCHBEND:
			eventData[1] = pChunkMF[locMF++];		/* Commands with 2 data bytes */
			eventData[2] = pChunkMF[locMF++];
			opStatus = OP_COMPLETE;
			break;
		case MPGMCHANGE:
		case MCHANPRES:
			eventData[1] = pChunkMF[locMF++];		/* Commands with 1 data byte */
			opStatus = OP_COMPLETE;
			break;
		case MSYSEX:
			/* This handles only the original one-chunk form of SysEx message. See
			   midifile.c in Tim Thompson's mftext for some elegant code for collecting
			   arbitrary chunks of SysEx. */
			   
			n = 1;
			while (pChunkMF[locMF]!=MEOX)
				eventData[n++] = pChunkMF[locMF++];
			locMF++;									/* Skip the MEOX byte */
			opStatus = OP_COMPLETE;
			break;
		case METAEVENT:									/* No. of data bytes is in the command */
			eventData[1] = pChunkMF[locMF++];
			eventData[2] = pChunkMF[locMF];
			for (i = 1; i<=pChunkMF[locMF]; i++)
				eventData[i+2] = pChunkMF[locMF+i];
				
			/* JGG & DAB: Combining the next two operations with:
					locMF += pChunkMF[locMF++];
			   ...	doesn't work right on the CodeWarrior PowerPC compiler (ca. 2000?).
			   Not that we care about CodeWarrior anymore, but it might be a problem
			   with other compilers too. */
			   
			locMF += pChunkMF[locMF];
			locMF++;
			opStatus = OP_COMPLETE;
			break;
		default:
			locMF++;
			break;
	}

	if (locMF>lenMF) return NOTHING_TO_DO;

	*pDeltaTime = deltaT;
	return opStatus;
}


/* ----------------------------------------------------------- Note-Off List Functions -- */
/* The following functions are to maintain a list of locations of Note Offs that
have already been matched with Note Ons, so we can avoid matching them again. This
can only be a problem when two or more instances of the same note number are play-
ing simultaneously on the same track. That situation can occur either because of a
unison or when the track actually contains two or more rhythmically independent
voices, at least for a moment.

NB: After writing this, I discovered that most, if not all, well-known applications
write MIDI files in which one Note Off really does terminate multiple Note Ons! So
I don't know if this code will ever be useful; oh well. The "one Note Off for many
Note Ons" case can be handled either by not calling any of these functions, or
calling InitNoteOffList but never InsertNoteOffLoc: then the usedNoteOff list will
stay empty and MayBeNoteOff will always return True.  --DAB  */

static short usedNoteOff[MAXMFEVENTLIST];
static short noteOffLim;

Boolean InsertNoteOffLoc(short, short);
Boolean CheckNoteOffList(short);
void InitNoteOffList(void);
Boolean MayBeNoteOff(short);

/* Insert the specified Note Off command location into the usedNoteOff list. The
locations are those of the first data byte (the one containing the note number), not
the status byte, since--with running status--there might not be a status byte. */

Boolean InsertNoteOffLoc(short locOn, short locOff)
{
	short i;
	char fmtStr[256];

	CheckNoteOffList(locOn);
	
	/* Find first free slot in list, which may be at noteOffLim (end of list) */
	
	for (i = 0; i<noteOffLim; i++)
		if (usedNoteOff[i]==0) break;
	
	/* Insert Note Off location into free slot, or append to end of list if there's room */
	
	if (i<noteOffLim || noteOffLim++<MAXMFEVENTLIST) {
		usedNoteOff[i] = locOff;
		return True;
	}
	else {
		noteOffLim--;			
		GetIndCString(fmtStr, MIDIFILE_STRS, 23);    /* "Nightingale can keep track of only %d notes at once." */
		sprintf(strBuf, fmtStr, MAXMFEVENTLIST); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
}


/*	Check usedNoteOff[] to see if any its entries are no longer needed; if so, free
their slots in the list. Return True if the list ends up empty. */

Boolean CheckNoteOffList(short locOn)
{
	short i, maxUsed;
	
	for (i = 0, maxUsed = -1; i<noteOffLim; i++)
		if (usedNoteOff[i]!=0) {
			if (usedNoteOff[i]<locOn)
				usedNoteOff[i] = 0;							/* slot is available now */
			else
				maxUsed = i;								/* slot is still in use */
		}

	noteOffLim = maxUsed+1;
	return (maxUsed<0);
}


/*	Mark usedNoteOff[] as completely empty. */

void InitNoteOffList()
{
	noteOffLim = 0;
}

/* If the given location could not possibly be the first data byte of a valid Note Off
because it's in the usedNoteOff list, return False, else return True. */

Boolean MayBeNoteOff(short locOff)
{
	short i;
	
	for (i = 0; i<noteOffLim; i++)
		if (usedNoteOff[i]==locOff) return False;
	
	return True;
}


/* ------------------------------------------------------------------------ GetNoteOff -- */
/* Get the Note Off for the Note On whose first data byte, i.e., the byte holding the
note number, is pChunkMF[loc]; optionally mark it so it won't be found again so, if there
are multiple notes with the same note number simultaneously in the track, one Note Off
will be needed for each one. This option would be useful only for very strange MIDI
files: see comments on InsertNoteOffLoc. */

static Boolean GetNoteOff(
			DoubleWord loc,
			Boolean oneEndsOne,		/* True=one Note Off ends only one current note w/its note no. */
			Byte noteOffData[],		/* Byte 0=off velocity, 1+2=duration */
			long *pDuration			/* Output: is the note longer than the longest we can handle? */
			)
{
	Byte noteNumber, command;
	DoubleWord deltaTime, duration=0L;
	DoubleWord oldLoc;
	Byte oldStatus;
	Byte eventData[255];
	Boolean okay=False, runStatus;
	
	noteNumber = pChunkMF[loc];
	
	SaveMFEventQue(&oldLoc, &oldStatus);
	InitMFEventQue(loc+2, MNOTEON);									/* May be unnecessary */
	while (GetNextMFEvent(&deltaTime, eventData, &runStatus)==OP_COMPLETE) {
		duration += deltaTime;

		command = MCOMMAND(eventData[0]);
		if ( command==MNOTEOFF
		||  (command==MNOTEON && eventData[2]==0) ) {
			if (eventData[1]==noteNumber && MayBeNoteOff(locMF-2)) {
				noteOffData[0] = eventData[2];
				noteOffData[1] = ACHAR(duration, 1);
				noteOffData[2] = ACHAR(duration, 0);

				/*	Optionally mark this Note Off so it won't be found again. */

				if (oneEndsOne) InsertNoteOffLoc(loc, locMF-2);
				okay = True;
				break;
			}
		}
	}
	
	*pDuration = duration;
	InitMFEventQue(oldLoc, oldStatus);
	return okay;
}

/* ------------------------------------------------------------------ Time2LDurQuantum -- */
/* Given a duration in PDUR ticks, normally return the corresponding duration code.
However, if triplets are allowed and the duration can be represented as one, instead
return the negative duration code. If there is no corresponding duration code,
return UNKNOWN_L_DUR. */

static short Time2LDurQuantum(long, Boolean);
static short Time2LDurQuantum(long time, Boolean allowTrips)
{
	short qLDur, divisor, qLTry, tripPDurUnit;

	/* Try normal durations, starting with the shortest. */
	
	if (time % PDURUNIT==0) {
		divisor = PDURUNIT; qLTry = MAX_L_DUR;
		while ((time % divisor)==0 && qLTry>=WHOLE_L_DUR) {
			qLDur = qLTry;
			divisor *= 2;
			qLTry--;
		}
	
	return qLDur;
	}
		
	/* If triplets are allowed, try triplet durations, starting with the shortest. */
	
	tripPDurUnit = (2*PDURUNIT)/3;
	if (allowTrips && (time % tripPDurUnit==0)) {
		divisor = tripPDurUnit; qLTry = MAX_L_DUR;
		while ((time % divisor)==0 && qLTry>=WHOLE_L_DUR) {
			qLDur = qLTry;
			divisor *= 2;
			qLTry--;
		}
	
	return -qLDur;
	}

	/* We couldn't find a match, so... */
	
	return UNKNOWN_L_DUR;
}

/* ---------------------------------------------------------------------- GetTrackInfo -- */
/* Deliver certain information about the current MIDI file track:
	- the number of notes it contains;
	- the number of notes that exceed the maximum duration we can handle;
	- whether it contains any notes on each channel;
	- a value that says, e.g., "all attacks fit a 16th-note grid" or "some attacks
		don't fit any metric grid Nightingale can handle" (perhaps due to tuplets);
	- the time of the last event in the track (normally the End-of-Track event).
Return False if we have trouble parsing the track (in which case values returned are
as of the point where we had trouble), else True. */

Boolean GetTrackInfo(
				short *noteCount,
				short *nTooLong,
				Boolean chanUsed[MAXCHANNEL],
				short *quantumLDur,	/* code for coarsest grid that fits all attacks, or UNKNOWN_L_DUR=none */
				long *quantumLPt,	/* time where attack requiring finest grid first occurs */
				Boolean *trip,		/* need triplets to fit all attacks? */
				long *lastEvent		/* in ticks */
				)
{
	DoubleWord tickTime, deltaTime, qLPoint;
	Byte eventData[255], dummy[3];
	Byte command;
	long duration;
	short qLDur, qLDHere, i, opStatus;
	short totCount=0;
	Boolean runStatus, okay=False, trips=False;

	for (i = 0; i<MAXCHANNEL; i++)
		chanUsed[i] = False;
	*nTooLong = 0;
	tickTime = 0L;
	qLDur = WHOLE_L_DUR;
	InitMFEventQue(0, 0);
	InitNoteOffList();			/* Now (v.999) useless but if removed, remove calls to MaybeNoteOff! */
	
	while ((opStatus = GetNextMFEvent(&deltaTime, eventData, &runStatus))==OP_COMPLETE) {
		tickTime += deltaTime;

		command = MCOMMAND(eventData[0]);
		if (command==MNOTEOFF) continue;
		if (command==MNOTEON && eventData[2]==0) continue;		/* Really a Note Off */
		
		switch (command) {
			case MNOTEON:
				if (GetNoteOff(locMF-2, False, dummy, &duration)) {
					chanUsed[MCHANNEL(eventData[0])] = True;
					totCount++;
					if (duration>65535L) (*nTooLong)++;
					if  (qLDur!=UNKNOWN_L_DUR) {
						qLDHere = Time2LDurQuantum(tickTime, True);
						if (qLDHere<0) trips = True;
						if (qLDHere==UNKNOWN_L_DUR || qLDHere>qLDur) {
							qLDur = qLDHere;
							qLPoint = tickTime;
						}
					}
					break;
				}
				else
					goto Done;									/* Something is wrong */
			case MNOTEOFF:
				break;											/* Should never get here */
			case METAEVENT:
				break;
			default:
				;
		}
	}
	
	okay = (opStatus!=FAILURE);

Done:
	*quantumLDur = qLDur;
	*quantumLPt = qLPoint;
	*noteCount = totCount;
	*trip = trips;
	*lastEvent = tickTime;
	return okay;
}

/* ---------------------------------------------------------------- GetTimingTrackInfo -- */
/* Deliver certain information about the current MIDI file track, most appropriately
but not necessarily the timing track:
	- the number of time signatures it contains;
	- the number of illegal time signatures;
	- a value that gives the shortest time signature denominator;
	- the time of the last event in the track (normally the End-of-Track event).
Return False if we have trouble parsing the track (in which case values returned are
as of the point where we had trouble), else True. */

Boolean GetTimingTrackInfo(
				short *tsCount,
				short *nTSBad,
				short *quantumLDur,	/* code for coarsest grid that fits all timesig denoms, or UNKNOWN_L_DUR=none */
				long *lastEvent		/* in ticks */
				)
{
	DoubleWord tickTime, deltaTime;
	Byte eventData[255];
	Byte command;
	short tsDenom, maxDenom, totCount=0;
	Boolean runStatus;
	long tickDur;
	short denomTab[MAX_DENOM_POW2+1] = { 1, 2, 4, 8, 16, 32, 64 };

	*nTSBad = 0;
	tickTime = 0L;
	maxDenom = 0;
	InitMFEventQue(0, 0);
	
	while (GetNextMFEvent(&deltaTime, eventData, &runStatus)==OP_COMPLETE) {
		tickTime += deltaTime;

		command = MCOMMAND(eventData[0]);
		if (command==METAEVENT) {
			if (eventData[1]==ME_TIMESIG) {
				totCount++;
				tsDenom = (eventData[4]<=MAX_DENOM_POW2 ?
								denomTab[eventData[4]] : denomTab[MAX_DENOM_POW2]);
				if (IsTimeSigBad(eventData[3], tsDenom))
					(*nTSBad)++;
				else
					maxDenom = n_max(maxDenom, tsDenom);
			}
		}
	}
	
	if (totCount==0) maxDenom = 1;
	tickDur = l2p_durs[WHOLE_L_DUR]/(long)maxDenom;
	*quantumLDur = Time2LDurQuantum(tickDur, False);
	*tsCount = totCount;
	*lastEvent = tickTime;
	return True;
}


/* ======================================================================= MF2MIDNight == */
/* Convert an entire track of a MIDI file into our "MIDNight" form, an inter-
mediate form designed simply to make it easier to generate Nightingale data
structure. The only differences between MIDI file format and MIDNight are:
1. MIDNight has no Note Offs; instead, release velocity and duration are appended
	to Note Ons, giving notes the format (in bytes):
		Status NoteNum OnVel OffVel Dur1 Dur2
	Notice that this limits durations to a maximum of 2^16-1 = 65535 ticks; at our
	480 ticks/quarter resolution, this is over 34 whole notes, so it's very rarely
	a problem. (We limit measure durations in exactly the same way.)
2. Instead of variable-length delta times, MIDNight has 4-byte absolute times.
3. Every MIDNight event starts at an even (word) address: if necessary to achieve
	this, a byte of padding follows every event.
4. For now, at least, MIDNight ignores SysEx events and channel commands other than
	Note On and Note Off.

A note in a MIDI file takes from 4 to 6 bytes (2 or 3 each for Note On and Off), while
MIDNight always needs 6. Also, every event in a MIDI file needs from 1 to 4 bytes, but
rarely more than 2, for delta time, while MIDNight always needs 4 for its absolute time.
On the other hand, a MIDI file note has two delta times, while a MIDNight note has only
one. So, counting timing info, a note takes a total of 6 to 10 bytes in a MIDI file,
exactly 10 in MIDNight, for a total of 0 to 4 more in MIDNight. Considering running
status, a metaevent takes 0 to 4 bytes more in a MIDNight file. Furthermore, MIDNight
may need a byte of padding to make events start at even locations. Thus, a MIDNight
chunk is likely to be much larger than the MIDI file track it corresponds to, though
rarely if ever twice as large.

MF2MIDNight returns the length of the MIDNight chunk, or 0 if an error occurs. */

#define MN_NOTELEN 6L	/* Length of an MFNote, not counting tStamp */
#define MN_CTLLEN 6L	/* Length of a Control Change message, not counting tStamp */

Word MF2MIDNight(
			Byte **ppMNChunk)		/* Pointer to MIDNight track Pointer */
{
	DoubleWord lenMN, outLoc;
	DoubleWord tickTime, deltaTime;
	Byte *pChunk;
	Byte eventData[255];
	Byte command;
	long duration;
	Boolean runStatus;
	char fmtStr[256];

	/* Allow 75% plus a bit more extra space: that seems always to be enough, but it's
	   not guaranteed to be! */
	   
	lenMN = 175L*lenMF/100L+10L;
	if (lenMN>MAX_TRACK_SIZE) {
		GetIndCString(fmtStr, MIDIFILE_STRS, 24);    /* "Importing a track of the MIDI file requires %ld bytes; the largest Nightingale can handle is only %ld." */
		sprintf(strBuf, fmtStr, lenMN, MAX_TRACK_SIZE); 
		CParamText(strBuf, "", "", "");
		StopInform(READMIDI_ALRT);
		return 0;
	}
	
	pChunk = (Byte *)NewPtr(lenMN);
	if (!GoodNewPtr((Ptr)pChunk)) {
		OutOfMemory(lenMN);
		return 0;
	}

	outLoc = 0;
	tickTime = 0L;
	InitMFEventQue(0, 0);
	InitNoteOffList();			/* Now (v.999) useless but if removed, remove calls to MaybeNoteOff! */
	
	while (GetNextMFEvent(&deltaTime, eventData, &runStatus)==OP_COMPLETE) {
		if (outLoc>lenMN) {
			MayErrMsg("MF2MIDNight: output buffer overflow. Input buffer: locMF=%ld lenMF=%ld",
						(long)locMF, (long)lenMF);
			return 0;
		}
		tickTime += deltaTime;

		command = MCOMMAND(eventData[0]);
		if (command==MNOTEOFF) continue;
		if (command==MNOTEON && eventData[2]==0) continue;		/* Really a Note Off */
		
		switch (command) {
			case MNOTEON:
				BlockMove(&tickTime, pChunk+outLoc, sizeof(long));
				outLoc += sizeof(long);
	
				if (GetNoteOff(locMF-2, False, &eventData[3], &duration)) {	/* Get off vel. and dur. */
					BlockMove(eventData, pChunk+outLoc, MN_NOTELEN);
					
					/* Round up to an even (word) address for the benefit of 68000s.
					   We don't care about 68000s anymore, but it might possibly matter
					   with a machine we do care about, and it doesn't hurt.  --DAB, Dec.
					   2022 */
					   
					outLoc += ROUND_UP_EVEN(MN_NOTELEN);
				}
				else
					MayErrMsg("MF2MIDNight: can't find Note Off for note at %ld", locMF-2);
				break;
			case MSYSEX:
				break;
			case MCTLCHANGE:
				BlockMove(&tickTime, pChunk+outLoc, sizeof(long));
				outLoc += sizeof(long);
				
				BlockMove(eventData, pChunk+outLoc, (long)MN_CTLLEN);
				outLoc += ROUND_UP_EVEN(MN_CTLLEN);
				break;
			case MPGMCHANGE:
//LogPrintf(LOG_DEBUG, "MF2MIDNight: found MPGMCHANGE\n");
				break;
			case METAEVENT:
				BlockMove(&tickTime, pChunk+outLoc, sizeof(long));
				outLoc += sizeof(long);
	
				BlockMove(eventData, pChunk+outLoc, (long)(eventData[2]+3));
				outLoc += ROUND_UP_EVEN(eventData[2]+3);
				break;
			default:
				;													/* Skip misc. channel cmds, etc. */
		}
	}

	if (outLoc>lenMN) {
			MayErrMsg("MF2MIDNight: output buffer overflow. Input buffer: locMF=%ld lenMF=%ld",
					(long)locMF, (long)lenMF);
		return 0;
	}
	*ppMNChunk = pChunk;
	return outLoc;
}


/* ========================================================== MIDNight --> Nightingale == */
/* This is the fun part: building Nightingale object list from tracks in MIDNight
format. Each track consists of a sequence of note events and metaevents: no SysEx
events are allowed (MIDNight doesn't handle them yet, anyway). For now, these
additional restrictions hold:
	1. Track 1 may contain only metaevents, no Notes. Time signatures in it are used;
		if there's a key signature at time 0, it's used, but any others are ignored.
	2. The only metaevents tracks after 1 may contain are end-of-track and name of
		sequence/track.

MIDNight2Night is the top-level function here, but Track2Night is in charge of most of
the work. Track2Night goes thru an entire track on each call. If it's track 1 -- the
tempo map track -- it creates all Measures and time signatures for the entire score,
plus one extra Measure. Otherwise it generates two tables describing the contents of the
track: rawSyncTab for what will become Syncs, and rawNoteAux for the notes. When it's
gone thru a track (other than track 1), Track2Night calls TranscribeVoice, which:
	-Decides what notes should be tupleted, if tuplets were requested (ChooseTuplets);
	-Creates all the Syncs (BuildSyncs);
	-Quantizes the notes/chords, if quantization was requested (QuantizeAndClip);
	-Rhythmically clarifies the notes/chords (several Clarifyxxx functions, interleaved
		with the preceding steps).
All the notes are in one voice only, and BuildSyncs puts ALL of them in the first Measure
of the score (the "extra" Measure, which will later be removed); note that the number
of Syncs in that Measure can greatly exceed MAX_MEASNODES, so a lot of Nightingale's
machinery can't be used at this point. Also, the <subtype> (CMN duration) fields of the
note/rest subobjects are meaningless: we keep the start times and durations of the Syncs
in ticks in <rawSyncTab>. Then Track2Night:
	-Sets the timestamp of every Sync relative to its Measure in the usual way, but
		sets the timestamp of every Measure to reflect the notated duration of the
		previous Measure, paying no attention to the Syncs the Measures contain ??IS
		THIS AT ALL ACCURATE ANY MORE??;
	-Puts (32-bit) timing info for the new stuff, derived from the timestamps, into
		the <mergeObjs> data structure (FillMergeBuffer);
	-Merges the new stuff (consisting of Syncs and ties only) into the old by time
		(MRMerge), using <mergeObjs> for info about the new stuff, and getting info about
		the old stuff in the usual way from the timestamps in the object list; and
	-Fills the gaps before, between and after notes within measures with rests, leaving
		measures that don't contain any notes alone (FillNonemptyMeas).

Finally, MIDNight2Night deletes the extra Measure and adds clef changes. "Beam by Beat"
can be (and currently is) handled later and at an even higher level.
*/

typedef struct MFEvent {
	long		tStamp;		/* in units specified by <timeBase> in file header */
	Byte		data[258];	/* byte 0=opcode; for metaevents, 1=subop, 2=no. of bytes following */
} MFEvent;

typedef struct MFNote {
	long			tStamp;		/* in units specified by <timeBase> in file header */
	Byte			opcode;
	Byte			noteNum;
	Byte			onVelocity;
	Byte			offVelocity;
	unsigned short	dur;
} MFNote;


static void NamePart(LINK, char *);
static Boolean MakeMNote(MFNote *, Byte, MNOTE *);
static LINK MFNewMeasure(Document *, LINK);
static void FixTStampsForTimeSigs(Document *, LINK);
static Boolean MFAddMeasures(Document *, short, long *);
static Boolean DoMetaEvent(Document *, short, MFEvent *, short);

static void InitTrack2Night(Document *, long *, long *);
static short Track2RTStructs(Document *,TRACKINFO [],short,Boolean,long,long,short,LINKTIMEINFO [],
								unsigned short, short *,NOTEAUX [],short *,short);
static short Track2Night(Document *, TRACKINFO [], short, short, short, short, long);

/* Set basic sizes for merge table and one-track table. Cf. comments on InitTrack2Night. */

#define MERGE_TAB_SIZE 4000			/* Max. Syncs and Measures in table for merging into */
#define ONETRACK_TAB_SIZE 4000		/* Max. Syncs and max. notes for a single track */

#define MAX_TSCHANGE 1000			/* Max. no. of time sig. changes in score */
#define MAX_MF_TEMPOCHANGE 1000		/* Max. no. of tempo changes in score */
#define MAX_CTRLCHANGE 1000

static short tsNum, tsDenom, sharpsOrFlats;
static short measTabLen;
static long measDur, timeSigTime;
static MEASINFO *measInfoTab;
static short tempoTabLen;
static TEMPOINFO *tempoInfoTab;
static short ctrlTabLen;
static CTRLINFO *ctrlInfoTab;

/* --------------------------------------------------------------------- Miscellaneous -- */

/* Given a LINK to a part and a full name for the part's instrument, fill in the part's
full name and make a guess at its short name. */

#define SHORT_LEN 4		/* No. of chars. of full name to use in short name */

static void NamePart(LINK partL, char *fullName)
{
	PPARTINFO aPart;

	aPart = GetPPARTINFO(partL);
	strcpy(aPart->name, fullName);
	strncpy(aPart->shortName, fullName, SHORT_LEN);
	if (strlen(fullName)>SHORT_LEN) {
		aPart->shortName[SHORT_LEN] = '.';
		aPart->shortName[SHORT_LEN+1] = '\0';
	}
}


/* Convert time from MIDI file's ticks to Nightingale's and vice-versa. Be sure to
initialize qtrNTicks and mfTimeBase before using! */

#define MFTicks2NTicks(t)	(qtrNTicks==mfTimeBase? (t) : (qtrNTicks*(long)(t))/mfTimeBase )
#define NTicks2MFTicks(t)	(qtrNTicks==mfTimeBase? (t) : (mfTimeBase*(long)(t))/qtrNTicks )

/* Convert info in an MFNote (MIDNight note) into the form needed for a Nightingale note. */

static Boolean MakeMNote(MFNote *pn, Byte channel, MNOTE *pMNote)
{
	short	transpose=0;
	
	pMNote->noteNumber = pn->noteNum-transpose;
	pMNote->channel = channel;
	pMNote->startTime = MFTicks2NTicks(pn->tStamp);
	pMNote->onVelocity = pn->onVelocity;
	pMNote->duration = MFTicks2NTicks(pn->dur);
	
	pMNote->offVelocity = pn->offVelocity;
	
	/* If off velocity is 0, the "Note Off" in the MIDI file was probably a zero-velocity
	   Note On. In that case, the off velocity is unspecified, so use default. */
	   	
	if (pMNote->offVelocity==0) pMNote->offVelocity = config.noteOffVel;

	return True;
}


/* Add a new Measure object just before <pL> and leave an insertion point set just
before <pL>. */

static LINK MFNewMeasure(Document *doc, LINK pL)
{
	LINK measL;  short sym;  CONTEXT context;

	NewObjInit(doc, MEASUREtype, &sym, singleBarInChar, ANYONE, &context);
	measL = CreateMeasure(doc, pL, -1, sym, context);
	doc->selStartL = doc->selEndL = pL;
	return measL;
}


/* Set the timestamp of every Measure in the score to reflect the notated duration of
the previous Measure, paying no attention to the contents of the Measures. Optionally
restart timestamps from 0 at a specified point. */

static void FixTStampsForTimeSigs(
					Document *doc,
					LINK resetL)		/* Reset timestamps to 0 at 1st Meas. at or after this, or NILINK */
{
	long measDur;
	LINK measL, nextMeasL, resetML;
	CONTEXT context;
	
	resetML = (resetL? SSearch(resetL, MEASUREtype, GO_RIGHT) : NILINK);
	
	measL = SSearch(doc->headL, MEASUREtype, GO_RIGHT);
	MeasureTIME(measL) = 0L;
	for ( ; measL; measL = LinkRMEAS(measL)) {
		nextMeasL = LinkRMEAS(measL);
		if (nextMeasL) {
			if (nextMeasL==resetML)
				MeasureTIME(nextMeasL) = 0L;
			else if (!MeasISFAKE(measL)) {
				/* Time sig. for this Meas. may appear after it, so GetContext at next Meas. */
				
				GetContext(doc, nextMeasL, 1, &context);
				measDur = TimeSigDur(N_OVER_D, context.numerator, context.denominator);
				MeasureTIME(nextMeasL) = MeasureTIME(measL)+measDur;
			}
			else
				MeasureTIME(nextMeasL) = MeasureTIME(measL);
		}
	}
}


/* Add Measure objects and time signatures as described by <measInfoTab> to the given
score, which we assume is brand-new and devoid of content. Set the Measures' timeStamps
to give them the durations indicated by the time signatures. */

static Boolean MFAddMeasures(Document *doc, short maxMeasures, long *pStopTime)
{
	long measTime=0L, measDur;
	short i, count, m, measNum=0;
	LINK measL, lastMeasL;
	Boolean firstMeas=True;
	char fmtStr[256];
	
	for (i = 0; i<measTabLen; i++) {
		if (measInfoTab[i].count==0) continue;		/* in case of consecutive time sigs. */
		
		measDur = TimeSigDur(N_OVER_D, measInfoTab[i].numerator, measInfoTab[i].denominator);

		if (measTime==0L)
			TimeSigBeforeBar(doc, doc->headL, ANYONE, N_OVER_D,
							measInfoTab[i].numerator, measInfoTab[i].denominator);
		else
			NewTimeSig(doc, 0, ' ', ANYONE, N_OVER_D,
							measInfoTab[i].numerator, measInfoTab[i].denominator);

		firstMeas = False;
		
		/* If this is the first time signature, add one extra Measure object, since the
		   second Measure is removed at the end of Track2Night. */
			
		count = measInfoTab[i].count;
		if (i==0) count++;
		for (m = 0; m<count; m++) {
			measNum++;
			if (measNum>maxMeasures+1) goto Done;				/* Allow for extra Measure */
			if (measNum%10 == 0) {
				GetIndCString(fmtStr, MIDIFILE_STRS, 25);		/* ": adding measure %d..." */
				sprintf(strBuf, fmtStr, measNum); 
				ProgressMsg(TIMINGTRACK_PMSTR, strBuf);
			}
	
			measL = MFNewMeasure(doc, doc->tailL);
			if (!measL) return False;
			MeasureTIME(measL) = measTime;
			measTime += measDur;
		}
	}

Done:
	measL = SSearch(doc->headL, MEASUREtype, GO_RIGHT);
	FixTStampsForTimeSigs(doc, RightLINK(measL));				/* Start with 2nd Measure */

	lastMeasL = SSearch(doc->tailL, MEASUREtype, GO_LEFT);
	*pStopTime = MeasureTIME(lastMeasL);
	return True;
}

#ifdef NOMORE
static void DeleteAllMeasures(Document *doc, LINK startL, LINK endL)
{
	LINK pL, nextL;
	
	for (pL = startL; pL!=endL; pL = nextL) {
		nextL = RightLINK(pL);
		if (MeasureTYPE(pL)) DeleteMeasure(doc, pL);
	}
}
#endif

static unsigned long GetTempoMicrosecsPQ(MFEvent *p) 
{
	unsigned long microsecsPQ = 0L;
	unsigned long  byte3 = p->data[3];
	unsigned long  byte4 = p->data[4];
	unsigned long  byte5 = p->data[5];
	microsecsPQ = (byte3 << 16) | (byte4 << 8) | byte5;
	return microsecsPQ;
}


#define IGNORE_TIMESIGS		False			/* True = ignore any timesigs in the MIDI file */

static Boolean DoMetaEvent(
						Document *doc,
						short track,
						MFEvent *p,
						short tripletBias)	/* Percent bias towards quant. to triplets >= 2/3 quantum */
{
	long timeNow;
	LINK partL;
	short i, newDenom, measCount;
	char string[256], str1[20], str2[20];
	static short measNum;
	static short fileSharpsOrFlats;
	static long timePrev=-1;
	short denomTab[MAX_DENOM_POW2+1] = { 1, 2, 4, 8, 16, 32, 64 };
	char fmtStr[256];
	
	timeNow = MFTicks2NTicks(p->tStamp);
	if (timeNow==0) measNum = 1;							/* A funny way to init. but should work */
	if (timeNow<timePrev) fileSharpsOrFlats = 0;			/* New file: re-initialize */
	timePrev = timeNow;

	Pstrcpy((unsigned char *)string, (unsigned char *)&p->data[2]);
	PToCString((unsigned char *)string);

	switch (p->data[1]) {
		case ME_SEQTRACKNAME:
			partL = FirstSubLINK(doc->headL);				/* Skip 1st part: it's never used */
			for (i = 1; i<track; i++) {
				if (!partL) return False;
				partL = NextPARTINFOL(partL);
			}
			NamePart(partL, string);

			partL = FirstSubLINK(doc->masterHeadL);			/* Skip 1st part: it's never used */
			for (i = 1; i<track; i++) {
				if (!partL) return False;
				partL = NextPARTINFOL(partL);
			}
			NamePart(partL, string);

			return True;

		case ME_TEMPO:	
			if (tempoTabLen >= MAX_MF_TEMPOCHANGE) return False;
			
			unsigned long microsecsPQ = GetTempoMicrosecsPQ(p);
			tempoInfoTab[tempoTabLen].tStamp = timeNow;
			tempoInfoTab[tempoTabLen].microsecPQ = microsecsPQ;
			tempoTabLen++;

			return True;

		case ME_TIMESIG:
			if (IGNORE_TIMESIGS) return False;
			
			newDenom = (p->data[4]<=MAX_DENOM_POW2 ?
							denomTab[p->data[4]] : denomTab[MAX_DENOM_POW2]);
			if (p->data[3]!=tsNum || newDenom!=tsDenom) {
				if (IsTimeSigBad(p->data[3], newDenom)) return False;

				/* If <timeNow> is 0, we're just replacing the initial time sig.;
					otherwise, store count for previous time sig. and index to new one */
				
				if (timeNow!=0L) {
					/* The interval between time sigs. _might_ correspond to a fractional
					   number of measures. This is a pretty strange situation; I'm not sure
					   most MIDI-file-writing programs would ever write such a file, but it
					   can easily happen with MIDI files written by Nightingale itself, since
					   we don't insist measure contents agree with time sigs. In such cases,
					   Nightingale warns the user when they save the file that it may not be
					   easy to make sense of; we also warn here, when they open it, and
					   round the no. of measures. */
						
					measCount = (timeNow-timeSigTime)/measDur;
					if ((timeNow-timeSigTime)%measDur!=0) {
						sprintf(str1, "%d", measNum+measCount);
						sprintf(str2, "%ld", timeNow);
						CParamText(str1, str2, "", "");
						CautionInform(tripletBias>-100?
										OPENMF_TIMESIG_ALRT1 : OPENMF_TIMESIG_ALRT2);
						measCount = (timeNow-timeSigTime+measDur/2)/measDur;
					}
					measInfoTab[measTabLen-1].count = measCount;
					measTabLen++;
					measNum += measCount;

				}
				
				tsNum = p->data[3];
				tsDenom = newDenom;
				measDur = TimeSigDur(N_OVER_D, tsNum, tsDenom);
				
				/* Assume this time sig., rather than the previous one, applies to
					the current measure: this should certainly be valid normally. */
				
				measInfoTab[measTabLen-1].count = 1;
				measInfoTab[measTabLen-1].numerator = tsNum;
				measInfoTab[measTabLen-1].denominator = tsDenom;
				timeSigTime = timeNow;
			}
			return True;

		case ME_KEYSIG:
			if ((SignedByte)p->data[3]==fileSharpsOrFlats) return False;
			fileSharpsOrFlats = (SignedByte)p->data[3];
			if (timeNow!=0L) {
				GetIndCString(fmtStr, MIDIFILE_STRS, 26);    /* "Found key signature change at time %ld: Nightingale will ignore it and use enharmonic spelling." */
				sprintf(strBuf, fmtStr, timeNow); 
				CParamText(strBuf, "", "", "");
				CautionInform(GENERIC_ALRT);
				return False;
			}
			sharpsOrFlats = (SignedByte)p->data[3];
			KeySigBeforeBar(doc, doc->headL, (track==1? ANYONE : track-1), sharpsOrFlats);
			recordFlats = (sharpsOrFlats<0);
			return True;

		default:
#ifdef NOTYET
			if (track!=1
			&& (p->data[1]!=ME_EOT
				 && p->data[1]!=ME_SEQTRACKNAME && p->data[1]!=ME_INSTRNAME)) {
				GetIndCString(fmtStr, MIDIFILE_STRS, 27);    /* "Found metaevent %d in track %d, but Nightingale can only handle this metaevent in the timing track." */
				sprintf(strBuf, fmtStr, p->data[1], track); 
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return False;
			}
#endif
		
			return False;
	}
}


/* ----------------------------------------------------------- Clef-choosing functions -- */
/* AddClefChanges and its helpers crudely add clef changes on a measure-by-measure
basis to keep notes near the staff. They:
	- assume the only clefs in the score are the initial ones and all are treble or bass.
	- assume there's exactly one voice per staff, the default voice.(??THEY DO??)
	- ignore grace notes.
	- create only treble and bass clefs.

(It would probably be easy to make these functions leave alone any staves that start in
clefs other than treble or bass; beyond that, handling other clefs looks hard.) */

static LINK NewClefPrepare(Document *, LINK, short);
static LINK Replace1Clef(Document *, LINK, short, char);
static Boolean ReplaceClefs(Document *, char [], char [], short);
static Boolean AddClefs(Document *, LINK, char [], char [], short);
static Boolean ChangeClefs(Document *, LINK, short [], char []);
static Boolean AddClefs1Measure(Document *, LINK, char []);
static Boolean AddClefChanges(Document *);

static LINK NewClefPrepare(Document *doc, LINK insL, short nstaves)
{
	LINK newL;
	
	newL = InsertNode(doc, insL, CLEFtype, nstaves);
	if (newL) {
		SetObject(newL, 0, 0, False, True, False);
	 	LinkTWEAKED(newL) = False;
	}
	else
		NoMoreMemory();

	return newL;
}

/* Replace clef before first (with invisible barline) Measure. Return the first LINK
after the range affected, i.e., the next clef change on the staff, if any. Identical
to ReplaceClef except that this function doesn't affect selection. */

LINK Replace1Clef(Document *doc, LINK firstClefL, short staffn, char subtype)
{
	PACLEF	aClef;
	LINK	aClefL, doneL;
	char	oldClefType;
	
	ClefINMEAS(firstClefL) = False;

	aClefL = FirstSubLINK(firstClefL);
	for ( ; aClefL; aClefL = NextCLEFL(aClefL))
		if (ClefSTAFF(aClefL)==staffn) {
			aClef = GetPACLEF(aClefL);
			oldClefType = aClef->subType;
			InitClef(aClefL, staffn, 0, subtype);

			ClefVIS(aClefL) = True;
			
			UpdateBFClefStaff(firstClefL,staffn,subtype);

			doc->changed = True;
			break;
		}

	doneL = FixContextForClef(doc, RightLINK(firstClefL), staffn, oldClefType, subtype);
	return doneL;
}

static Boolean ReplaceClefs(Document *doc, char /*curClef*/[], char newClef[], short nToChange)
{
	LINK firstClefL;
	short i, s, staff;
	
	firstClefL = LSSearch(doc->headL, CLEFtype, 1, GO_RIGHT, False);
	
	for (i = 0; i<nToChange; i++) {
		for (staff = -1, s = 1; s<=doc->nstaves; s++)			/* Inefficient but not very */
			{ if (newClef[s]) staff++; if (staff==i) break; }
		Replace1Clef(doc, firstClefL, s, newClef[s]);
	}
	DeselectNode(firstClefL);									/* Since ReplaceClef selects */
	
	return True;
}

static Boolean AddClefs(Document *doc, LINK measL, char curClef[], char newClef[],
								short nToChange)
{
	LINK newL, aClefL;  PACLEF aClef;
	short i, s, staff;
	
	newL = NewClefPrepare(doc, measL, nToChange);
	if (!newL) return False;
	
	ClefINMEAS(newL) = True;
		
	aClefL = FirstSubLINK(newL);
	for (i = 0; aClefL && i<nToChange; aClefL = NextCLEFL(aClefL), i++) {
		for (staff = -1, s = 1; s<=doc->nstaves; s++)			/* Inefficient but not very */
			{ if (newClef[s]) staff++; if (staff==i) break; }
		
		InitClef(aClefL, s, 0, newClef[s]);
	
		aClef = GetPACLEF(aClefL);
		aClef->selected = False;
		aClef->small = ClefINMEAS(newL);
		aClef->soft = False;
	
		FixContextForClef(doc, RightLINK(newL), s, curClef[s], newClef[s]);
	}
	
	return True;
}

static Boolean ChangeClefs(
		Document *doc,
		LINK measL,
		short avgNoteNum[],	/* Average note numbers, by staff; -1 = no notes on the staff */
		char curClef[]
		)
{
	short s, nToChange;  char newClef[MAXSTAVES+1];
	LINK firstMeasL;  Boolean beforeFirst, okay;
	
	for (nToChange = 0, s = 1; s<=doc->nstaves; s++) {
		if (avgNoteNum[s]<=0)
			newClef[s] = 0;
		else if (avgNoteNum[s]>MIDI_BASS_TOP && curClef[s]==BASS_CLEF)
			{ newClef[s] = TREBLE_CLEF; nToChange++; }
		else if (avgNoteNum[s]<MIDI_TREBLE_BOTTOM && curClef[s]==TREBLE_CLEF)
			{ newClef[s] = BASS_CLEF; nToChange++; }
		else
			newClef[s] = 0;
	}
	
	if (nToChange==0) return True;
	
	firstMeasL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);
	beforeFirst = (measL==firstMeasL);
	if (beforeFirst) {
		ReplaceClefs(doc, curClef, newClef, nToChange);
		okay = True;
	}
	else
		okay = AddClefs(doc, measL, curClef, newClef, nToChange);
	
	for (s = 1; s<=doc->nstaves; s++)
		if (newClef[s]!=0) curClef[s] = newClef[s];
	
	return okay;
}

static Boolean AddClefs1Measure(Document *doc, LINK measL, char curClef[])
{
	LINK endMeasL, pL, aNoteL;
	short s, noteCount[MAXSTAVES+1], avgNoteNum[MAXSTAVES+1];
	long totNoteNum[MAXSTAVES+1];
	
	for (s = 1; s<=doc->nstaves; s++)
		totNoteNum[s] = noteCount[s] = 0;

	endMeasL = EndMeasSearch(doc, measL);
	for (pL = RightLINK(measL); pL!=endMeasL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (!NoteREST(aNoteL)) {
					totNoteNum[NoteSTAFF(aNoteL)] += NoteNUM(aNoteL);
					noteCount[NoteSTAFF(aNoteL)]++;
				}
			}
		}

	for (s = 1; s<=doc->nstaves; s++)
		avgNoteNum[s] = (noteCount[s]>0? totNoteNum[s]/noteCount[s] : -1);

	return ChangeClefs(doc, measL, avgNoteNum, curClef);
}

static Boolean AddClefChanges(Document *doc)
{
	LINK measL, clefL, aClefL;
	char curClef[MAXSTAVES+1];
			
	clefL = LSSearch(doc->headL, CLEFtype, ANYONE, GO_RIGHT, False);
	aClefL = FirstSubLINK(clefL);
	for ( ; aClefL; aClefL = NextCLEFL(aClefL))
		curClef[ClefSTAFF(aClefL)] = ClefType(aClefL);
	
	measL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);
	for ( ; measL; measL = LinkRMEAS(measL)) {
		if (!AddClefs1Measure(doc, measL, curClef)) return False;
	}

	return True;
}

/* ----------------------------------------------------- Functions for AddTempoChanges -- */

//long LastEndTime(Document *doc, LINK fromL, LINK toL);


/* ---------------------------------------------------------------------SyncSimpleLDur -- */
/*	Compute the maximum simple logical duration of all notes/rests in the given Sync. */

static long SyncSimpleLDur(LINK syncL)
{
	long dur, newDur;
	LINK aNoteL;
	
	dur = 0L; 
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		newDur = SimpleLDur(aNoteL);
		dur = n_max(dur, newDur);
	}
	return dur;
}

static short CountSyncs(Document *doc) 
{
	short nSyncs = 0;
	
	for (LINK pL = doc->headL; pL != doc->tailL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL)) nSyncs++;
	}
	return nSyncs;
}


static LINKTIMEINFO *DocSyncTab(Document *doc, short *tabSize, LINKTIMEINFO *rawSyncTab, short rawTabSize) 
{
	short nSyncs = CountSyncs(doc);
	long tLen = nSyncs * sizeof(LINKTIMEINFO);
	LINKTIMEINFO *docSyncTab = (LINKTIMEINFO *)NewPtr(tLen);
	if (!GoodNewPtr((Ptr)docSyncTab)) {
		*tabSize = 0; return NULL;
	}
	*tabSize = nSyncs;
	
	long currTime = 0;
	long dur = 0;
	
	int i = 0;
	LINKTIMEINFO info;
	for (LINK pL = doc->headL; pL != doc->tailL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			info = docSyncTab[i];
			info.link = pL;
			if (i<rawTabSize) {
				currTime = info.time = rawSyncTab[i].time;				
			}
			else {
				info.time = currTime;
				dur = SyncSimpleLDur(pL);
				currTime += dur;				
			}
			docSyncTab[i++] = info;
		}
	}
	
	if (*tabSize != rawTabSize) {
		LogPrintf(LOG_WARNING, "rawSyncTab size %ld disagrees with docSyncTab size of %ld\n",
					rawTabSize, *tabSize);
	}
//	if (*tabSize > rawTabSize) {
//		*tabSize = rawTabSize;
//	}
	
	return docSyncTab;
}


static LINK GetTempoRelObj(LINKTIMEINFO *docSyncTab, short tabSize, TEMPOINFO tempoInfo) 
{
	long timeStamp = tempoInfo.tStamp;
	
	for (int j = 0; j<tabSize; j++) {
		LINKTIMEINFO info = docSyncTab[j];
		if (info.time == timeStamp) {			// Should always be the case
			return info.link;
		}
		if (info.time > timeStamp) {			// Timestamps not equal, first sync after the tempo
			if (j>0) info = docSyncTab[j-1];
			return info.link;					
		}
	}
	return docSyncTab[tabSize-1].link;
}


static LINK GetCtrlRelObj(LINKTIMEINFO *docSyncTab, short tabSize, CTRLINFO ctrlInfo) 
{
	long timeStamp = ctrlInfo.tStamp;
	
	LogPrintf(LOG_NOTICE, "track=%ld num=%ld val=%ld time=%ld\n", ctrlInfo.track, ctrlInfo.ctrlNum, ctrlInfo.ctrlVal, timeStamp);
	
	for (int j = 0; j<tabSize; j++) {
		LINKTIMEINFO info = docSyncTab[j];
		
		if (info.time == timeStamp) {			// Should always be the case
			return info.link;
		}
		if (info.time > timeStamp) 				// Timestamps not equal, first sync after the tempo
		{				
			//if (j>0) {									
				info = docSyncTab[j];
			//}
			
			return info.link;					
		}
	}
	return docSyncTab[tabSize-1].link;
}


static void PrintDocSyncTab(char *tabname, LINKTIMEINFO *docSyncTab, short tabSize) 
{
	LogPrintf(LOG_DEBUG, "PrintDocSyncTab: for %s, tabSize=%d:\n", tabname, tabSize);
	
	for (int j = 0; j<tabSize; j++) {
		LINKTIMEINFO info = docSyncTab[j];
		LogPrintf(LOG_DEBUG, "  j=%ld link=%ld time=%ld mult=%ld\n", j, info.link,
					info.time, info.mult);
	}
}

static void PrintDocSyncDurs(Document *doc) 
{
	LINK pL;
	long dur;
	
	for (pL = doc->headL; pL != doc->tailL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			dur = SyncSimpleLDur(pL);
		
			//LogPrintf(LOG_INFO, "syncL=%ld dur=%ld\n", pL, dur);
		}
	}
}


/* Compact <tempoInfoTab> by discarding any tempo change that occurs within TEMPO_WINDOW
of a previous tempo change. */

static Boolean CompactTempoTab() 
{
	if (tempoTabLen <= 1) return True;
	
	TEMPOINFO *tempoInfoTabOrig = tempoInfoTab;	
	
	long tLen = MAX_MF_TEMPOCHANGE*sizeof(TEMPOINFO);
	tempoInfoTab = (TEMPOINFO *)NewPtr(tLen);
	if (!GoodNewPtr((Ptr)tempoInfoTab)) {
		OutOfMemory(tLen);
		return False;
	}
	
	int tempoTabLenOrig = tempoTabLen;
	long prevTStamp = tempoInfoTabOrig[0].tStamp;
	
	tempoTabLen = 1;
	tempoInfoTab[0].tStamp = tempoInfoTabOrig[0].tStamp;
	tempoInfoTab[0].microsecPQ = tempoInfoTabOrig[0].microsecPQ;

	for (int i = 1; i<tempoTabLenOrig; i++) {
		TEMPOINFO tempoInfo = tempoInfoTabOrig[i];
		if (tempoInfo.tStamp > prevTStamp + TEMPO_WINDOW) {
		
			tempoInfoTab[tempoTabLen].tStamp = tempoInfo.tStamp;
			tempoInfoTab[tempoTabLen].microsecPQ = tempoInfo.microsecPQ;
		
			prevTStamp = tempoInfo.tStamp;
			tempoTabLen++;
		}
	}
	
	if (tempoInfoTabOrig != NULL)
		DisposePtr((Ptr)tempoInfoTabOrig);
	
	return True;	
}

#define MICROBEATS2TSCALE(mb) (60*1000000L/(mb))

static Boolean AddTempoChanges(Document *doc, LINKTIMEINFO *docSyncTab, short tabSize) 
{
	if (!CompactTempoTab()) return False;
	
	for (int i = 0; i<tempoTabLen; i++) {
		TEMPOINFO tempoInfo = tempoInfoTab[i];
		LINK relObj = GetTempoRelObj(docSyncTab, tabSize, tempoInfo);
		
		// See Insert.c InsertTempo
		
		if (relObj != NILINK) {
			Str63 tempoStr;
			Str63 metroStr;
			short clickStaff, pitchLev;
			Point pt;
			
			unsigned long microsecPQ = tempoInfo.microsecPQ;
			long microbeats = microsecPQ/DFLT_BEATDUR;
			long tscale = MICROBEATS2TSCALE(microbeats);
			
			short beatdur = DFLT_BEATDUR;
			long tempoValue = tscale / beatdur;
			if (DBG) LogPrintf(LOG_DEBUG, "AddTempoChanges: adding Tempo %ld (time=%ld) at L%u\n",
				tempoValue, tempoInfo.tStamp, relObj);
			
			short dur = QTR_L_DUR;
			
			NumToString(tempoValue, metroStr);
			Pstrcpy((StringPtr)tempoStr, (StringPtr)"\p");
			
			doc->selEndL = doc->selStartL = relObj;
			pitchLev = -2;
			clickStaff = 1;
			pt.h = 84;
			pt.v = 164;
			palChar = 'M';
			
			NewTempo(doc, pt, palChar, clickStaff, pitchLev, True, True, dur, False,
						False, tempoStr, metroStr);
		}
	}
	
	return True;
}


static Boolean AddControlChanges(Document *doc, LINKTIMEINFO *docSyncTab, short tabSize)
{
	if (DBG) PrintDocSyncTab("DocSyncTab", docSyncTab, tabSize);
	
	for (int i = 0; i<ctrlTabLen; i++) 
	{
		CTRLINFO ctrlInfo = ctrlInfoTab[i];		
		LINK relObj = GetCtrlRelObj(docSyncTab, tabSize, ctrlInfo);
		
		// See Insert.c InsertTempo
		
		if (relObj != NILINK) {
			Point pt;
			short clickStaff,pitchLev,voice,hStyleChoice = 0;
			short newSize,newStyle,newEncl;
			Str63 newFont; Str255 string;
			Boolean newRelSize;
			
			doc->selEndL = doc->selStartL = relObj;
			pitchLev = -2;
			clickStaff = 1;
			voice = 1;

			pt.h = 84;
			pt.v = 164;
			
			if (ctrlInfo.ctrlNum == MSUSTAIN) {
				if (ctrlInfo.ctrlVal > 63)
						palChar = 0xB6;			/* Music char. "Ped." (pedal down) */
				else	palChar = 0xFA;			/* Music char. pedal up */
				
				if (ctrlInfo.ctrlVal > 63) {
					string[0] = 3; string[1] = '1'; string[2] = '2'; string[3] = '7';
					newRelSize = doc->relFSizeRM;
					newSize = doc->fontSizeRM;
					newRelSize = True;
					newSize = GRStaffHeight;
					newStyle = doc->fontStyleRM;
					newEncl = ENCL_NONE;
					Pstrcpy((StringPtr)newFont, (StringPtr)doc->fontNameRM);
					
				}
				else {
					string[0] = 1; string[1] = '0';
					newRelSize = doc->relFSizeRM;
					newSize = doc->fontSizeRM;
					newRelSize = True;
					newSize = GRStaffHeight;
					newStyle = doc->fontStyleRM;
					newEncl = ENCL_NONE;
					Pstrcpy((StringPtr)newFont, (StringPtr)doc->fontNameRM);
					
				}
			}
			else if (ctrlInfo.ctrlNum == MPAN) {				
				sprintf((char*)string, "%d", ctrlInfo.ctrlVal);
				CtoPstr(string);
				palChar = 0x7C;					/* MIDI Pan Controller */				
				newRelSize = doc->relFSizeRM;
				newSize = doc->fontSizeRM;
				newStyle = doc->fontStyleRM;
				newEncl = doc->enclosureRM;
				Pstrcpy((StringPtr)newFont, (StringPtr)doc->fontNameRM);
			}
			
			clickStaff = ctrlInfo.track - 1;

			NewGraphic(doc, pt, palChar, clickStaff, voice, 0, newRelSize, newSize, newStyle,
							newEncl, 0, False, False, newFont, string, hStyleChoice);
		}
	}
	
	return True;
}

static Boolean AddRelObjects(Document *doc, LINKTIMEINFO *rawSyncTab, short rawTabSize)
{
//	short rawTabSize = 16;
	
	if (DBG) PrintDocSyncTab("RawSyncTab", rawSyncTab, rawTabSize);
	if (DBG) PrintDocSyncDurs(doc);
	
 	short tabSize;
	LINKTIMEINFO *docSyncTab = DocSyncTab(doc, &tabSize, rawSyncTab, rawTabSize);
	if (docSyncTab == NULL) return False;
	
	if (!AddTempoChanges(doc, docSyncTab, tabSize))
		goto done;
		
	AddControlChanges(doc, docSyncTab, tabSize);

done:
	if (docSyncTab != NULL)
		DisposePtr((Ptr)docSyncTab);
	
	return True;
}


/* ----------------------------------------------------------- Track2Night and friends -- */

/* Initialize for converting a track from MIDNight form to Nightingale object list.

As of this writing (v. 3.5b7), <*pMergeTabSize> controls allocation of 12 bytes/element
(the "merge table"); <*pOneTrackTabSize> controls allocation of 20 bytes/element (the
"one-track table"). So, e.g., increasing the former by 1000 increases memory required by
12K bytes.

To make the table sizes flexible, we allocate a fraction (currently 13 percent) of
available memory to the merge table and the one-track table, giving a fraction of
that (currently 60 percent) to the merge table. At current rates, every additional
megabyte free adds about 78K (6500 objects) to the merge table, and about 52K (2600
objects) to the one-track table. (For unknown reasons, the actual numbers of objects
added seem to be somewhat different than these figures.) NB1: some utility routines
we use expect short counts, so we clip sizes to SHRT_MAX; therefore, giving Nightingale
more than about 6000-7000K doesn't make the tables any larger. NB2: For compatibility,
we should always use numbers at least as large as the Nightingale 3.0 values (3500 for
each) as minima. */

static void InitTrack2Night(Document *doc, long *pMergeTabSize, long *pOneTrackTabSize)
{
	Size bytesFree, grow, bytesToUse, bytesForMergeTab, bytesForOneTrackTab;
	long tryMergeTabSize, tryOneTrackTabSize;
	long mergeTabEltSize, oneTrackTabEltSize;

	/*
	 *	MIDI files don't have to contain explicit time signatures. Their official
	 * default time signature is 4/4.
	 */
	tsNum = 4;
	tsDenom = 4;
	measDur = TimeSigDur(N_OVER_D, tsNum, tsDenom);
	measInfoTab[0].count = 1;
	measInfoTab[0].numerator = tsNum;
	measInfoTab[0].denominator = tsDenom;
	measTabLen = 1;
	timeSigTime = 0L;
	
	/*
	 * From MIDIFSave.c, WriteTiming, under comment 
	 * Write initial tempo ..
	 */
	long timeScale = (long)config.defaultTempoMM*DFLT_BEATDUR;
	long microbeats = TSCALE2MICROBEATS(timeScale);
	
	tempoTabLen = 0;					// No initial tempo; will be overwritten
	tempoInfoTab[0].tStamp = 0;
	tempoInfoTab[0].microsecPQ = microbeats*DFLT_BEATDUR;
	
	ctrlTabLen = 0;
	
	sharpsOrFlats = 0;
	recordFlats = doc->recordFlats;

	*pMergeTabSize = MERGE_TAB_SIZE;
	*pOneTrackTabSize = ONETRACK_TAB_SIZE;
	
	/*
	 * If there's enough available memory, increase table sizes. The constants that
	 * determine <bytesToUse> and <bytesForMergeTab> are pretty arbitrary: if you
	 * want to tweak them, there's no reason not to.
	 */
	mergeTabEltSize = sizeof(LINKTIMEINFO);
	oneTrackTabEltSize = sizeof(LINKTIMEINFO)+sizeof(NOTEAUX);

	bytesFree = MaxMem(&grow);
	bytesToUse = 0.13*bytesFree;						/* Take a fraction of available memory */

	bytesForMergeTab = 0.6*bytesToUse;
	tryMergeTabSize = (long)bytesForMergeTab/mergeTabEltSize;
	*pMergeTabSize = n_max(*pMergeTabSize, tryMergeTabSize);
	*pMergeTabSize = n_min(*pMergeTabSize, (long)SHRT_MAX);
	
	bytesForOneTrackTab = bytesToUse-bytesForMergeTab;
	tryOneTrackTabSize = (long)bytesForOneTrackTab/oneTrackTabEltSize;
	*pOneTrackTabSize = n_max(*pOneTrackTabSize, tryOneTrackTabSize);
	*pOneTrackTabSize = n_min(*pOneTrackTabSize, (long)SHRT_MAX);
}


#define DEBUG_REST if (DBG)	LogPrintf(LOG_DEBUG, " track=%d time=%ld rest fillTime=%ld mult=%d\n",	\
							track, theNote.startTime, fillTime, mult)

#define DEBUG_NOTE if (DBG)	LogPrintf(LOG_DEBUG, "%ctrack=%d time=%ld noteNum=%d dur=%ld mult=%d\n", \
							(chordWithLast? '+' : ' '), track,										 \
							theNote.startTime, theNote.noteNumber, theNote.duration, mult)

#define DEBUG_META if (DBG)	LogPrintf(LOG_DEBUG, " track=%d time=%ld Metaevent type=0x%x%s\n",	\
							track, (long)(MFTicks2NTicks(p->tStamp)), p->data[1],				\
							(p->data[1]==0x51? " (TEMPO)" : "") );

#define DEBUG_PGM  if (DBG)	LogPrintf(LOG_DEBUG, " track=%d time=%ld PgmChange type=0x%x\n",	\
							track, (long)(MFTicks2NTicks(p->tStamp)), p->data[1]);

#define DEBUG_UNK  if (DBG)	LogPrintf(LOG_DEBUG, " track=%d time=%ld (UNKNOWN) type=0x%x\n",	\
							track, (long)(MFTicks2NTicks(p->tStamp)), p->data[1]);


/* Convert one track of a MIDI file to "raw Syncs" (unquantized and unclarified notes and
chords, in parameters) and info on control changes (in ctrlInfoTab). Return OP_COMPLETE,
NOTHING_TO_DO, or FAILURE. */

static short Track2RTStructs(
					Document *doc,
					TRACKINFO trackInfo[],
					short track,
					Boolean isTempoMap,
					long cStartTime, long cStopTime,
					short timeOffset,
					LINKTIMEINFO rawSyncTab[],	/* Output: raw (unquantized and unclarified) NCs */
					unsigned short maxTrkNotes,	/* size of <rawSyncTab> and <rawNoteAux> */
					short *pnSyncs,
					NOTEAUX rawNoteAux[],		/* Output: auxiliary info on raw notes */
					short *pnAux,
					short tripletBias			/* Percent bias towards quant. to triplets >= 2/3 quantum */
					)
{
	Byte command, channel;
	long loc, prevStartTime, chordEndTime, noteEndTime;
	short iSync, nAux;
	long mult;
	MFEvent *p;
	MFNote *pn;
	MNOTE theNote;
	Boolean chordWithLast;
	char fmtStr[256];
	
	loc = 0L;
	prevStartTime = -999999L;
	iSync = -1;
	nAux = -1;
	
	while (loc<trackInfo[track].len) {
	
		/* Point p to the beginning of the next MFEvent */
		p = (MFEvent *)&(trackInfo[track].pChunk[loc]);
		
		command = MCOMMAND(p->data[0]);
		channel = MCHANNEL(p->data[0]);

		if (!isTempoMap && p->tStamp<cStartTime) goto IncrementLoc;
		if (!isTempoMap && p->tStamp>=cStopTime) break;
		
		/* Look for a MIDNight note or other interesting event. If a note starts at exactly
			the same time as the previous note in this track, put it in a chord with that
			note. Any further "deflamming" must wait until we know quantized times, and
			we can't do that till we decide tuplets. */
			
		switch (command) {
			case MNOTEON:
				if (p->data[2]==0) break;								/* Really a Note Off; should never get here */
				if (isTempoMap) {
					MayErrMsg("Track2RTStructs: can't handle a note in the tempo map track (start time %ld).",
									p->tStamp);
					continue;
				}
				pn = (MFNote *)p;
				if (MakeMNote(pn, channel, &theNote)) {
					noteEndTime = theNote.startTime+theNote.duration;
					// FIXME: EXPRESSION ON RIGHT SIDE OF NEXT STMT LOOKS VERY WRONG!
					if (noteEndTime>cStopTime) theNote.duration -= noteEndTime>cStopTime;
				
					nAux++;
					if (nAux>=maxTrkNotes) {
						GetIndCString(fmtStr, MIDIFILE_STRS, 28);		/* "Not enough space to prepare to transcribe track %d (%d notes available)." */
						sprintf(strBuf, fmtStr, track, maxTrkNotes); 
						CParamText(strBuf, "", "", "");
						StopInform(READMIDI_ALRT);
						return FAILURE;
					}
					chordWithLast = (theNote.startTime==prevStartTime);					
					if (!chordWithLast) {
						iSync++;
						if (iSync>=maxTrkNotes) {
							GetIndCString(fmtStr, MIDIFILE_STRS, 29);	/* "Not enough space to prepare to transcribe track %d (%d Syncs available)." */
							sprintf(strBuf, fmtStr, track, maxTrkNotes); 
							CParamText(strBuf, "", "", "");
							StopInform(READMIDI_ALRT);
							return FAILURE;
						}
						rawSyncTab[iSync].link = nAux;				/* Limits <nAux> to fit in sizeof(LINK) */
					}

					rawNoteAux[nAux].noteNumber = theNote.noteNumber;
					rawNoteAux[nAux].onVelocity = theNote.onVelocity;
					rawNoteAux[nAux].offVelocity = theNote.offVelocity;
					rawNoteAux[nAux].duration = theNote.duration;
					rawNoteAux[nAux].first = !chordWithLast;

					mult = theNote.duration;
					if (mult<=0) mult = 1;							/* Avoid possible zero duration */
					DEBUG_NOTE;

					prevStartTime = theNote.startTime;
	
					/* If this note is part of a chord, use the latest end time. */
					
					if (chordWithLast) {
						chordEndTime = prevStartTime+rawSyncTab[iSync].mult;
						if (prevStartTime+theNote.duration>chordEndTime) {
							rawSyncTab[iSync].mult = theNote.duration;
						}
					}
					else {
						rawSyncTab[iSync].mult = mult;
						rawSyncTab[iSync].time = theNote.startTime+timeOffset;
					}			
				}
				break;

			case MNOTEOFF:
				break;												/* Should never get here */

			case MCTLCHANGE:
				ctrlInfoTab[ctrlTabLen].tStamp = p->tStamp;
				ctrlInfoTab[ctrlTabLen].channel = channel;
				ctrlInfoTab[ctrlTabLen].ctrlNum = p->data[1];
				ctrlInfoTab[ctrlTabLen].ctrlVal = p->data[2];
				ctrlInfoTab[ctrlTabLen].track = track;

				ctrlTabLen++;
				break;

			case MPGMCHANGE:
				DEBUG_PGM;
				break;

			case METAEVENT:
				DEBUG_META;
				(void)DoMetaEvent(doc, track, p, tripletBias);
				break;

			default:
				DEBUG_UNK;
		}
		
IncrementLoc:
		/* FIXME: Incrementing <loc> on the assumption that any unidentified command is
		   a metaevent is very dangerous! Our own MIDI files contain MPGMCHANGEs! */
		if (command==MNOTEON)
			loc += ROUND_UP_EVEN(sizeof(p->tStamp)+6);
		else if (command==MCTLCHANGE)								/* control event */
			loc += ROUND_UP_EVEN(sizeof(p->tStamp)+6);
		else if (command==METAEVENT)
			loc += ROUND_UP_EVEN(sizeof(p->tStamp)+3+p->data[2]);
		else {
			AlwaysErrMsg("Track2RTStructs: Unknown MIDI event type.");
			return FAILURE;
		}

	}

	rawNoteAux[++nAux].first = True;								/* Sentinel at end */

	*pnSyncs = iSync+1;
	*pnAux = nAux;
	return (iSync>=0? OP_COMPLETE : NOTHING_TO_DO);
}

/* Convert the given track from MIDNight form to Nightingale object list, with or
without quantization. If quantization is requested, also clarify rhythm, i.e., break
notes into tied sequences to improve readability.  Return OP_COMPLETE if we generate
any notes/rests for the track, FAILURE if we try to generate notes/rests but can't, and
NOTHING_TO_DO if we don't need to do anything. The first call for a given MIDI file _must_
be with track==1 (the timing track, which must not contain any notes, so we'll always
return NOTHING_TO_DO for it).

NB: changing the quantum, maxMeasures, or scoreEndTime between tracks is probably not
a good idea! */

static short Track2Night(
					Document *doc,
					TRACKINFO trackInfo[],
					short track,
					short quantum,		/* For both attacks and releases, in PDUR ticks, or 1=no quantize */
					short tripletBias,	/* Percent bias towards quant. to triplets >= 2/3 quantum */
					short maxMeasures,	/* Transcribe no more than this many measures */
					long scoreEndTime	/* Last end-of-track time for any track, in PDUR ticks */
					)
{
	static long cStartTime=0L;			/* If track!=1, skip events before (PDUR ticks): normally 0 */
	static long cStopTime;				/* If track!=1, skip events after (PDUR ticks) */
	static long mergeTabSize, oneTrackTabSize;
	LINKTIMEINFO *rawSyncTab=NULL, *newSyncTab=NULL, *mergeObjs=NULL;
	NOTEAUX	*rawNoteAux;
	long		len;
	LINK		newFirstSync, measL, firstL, lastL;
	short		nMergeObjs, voice, count, nNewSyncs,
				nRawSyncs, nAux, status, tVErr;
	short		timeOffset=0;						/* in PDUR ticks: normally 0 */
	unsigned short maxNewSyncs;
	Boolean		isTempoMap, isLastTrack, okay=False;
	char		fmtStr[256];

	if (DBG) LogPrintf(LOG_DEBUG, "Track2Night: track %d\n", track);
	isLastTrack = (track==mfNTracks);
	isTempoMap = (track==1);						/* an OK assumption for format 1 MIDI files */
	if (isTempoMap) InitTrack2Night(doc, &mergeTabSize, &oneTrackTabSize);
	//if (isTempoMap && DBG) LogPrintf(LOG_DEBUG, "Track2Night: mergeTabSize=%ld oneTrackTabSize=%ld\n",
	//mergeTabSize, oneTrackTabSize);

	len = oneTrackTabSize*sizeof(LINKTIMEINFO);
	rawSyncTab = (LINKTIMEINFO *)NewPtr(len);
	if (!GoodNewPtr((Ptr)rawSyncTab)) {
		OutOfMemory(len);
		goto Done;
	}
	len = oneTrackTabSize*sizeof(NOTEAUX);
	rawNoteAux = (NOTEAUX *)NewPtr(len);
	if (!GoodNewPtr((Ptr)rawNoteAux)) {
		OutOfMemory(len);
		goto Done;
	}
	
	status = Track2RTStructs(doc,trackInfo,track,isTempoMap,cStartTime,cStopTime,
									 timeOffset,rawSyncTab,oneTrackTabSize,&nRawSyncs,
									 rawNoteAux,&nAux,tripletBias);
	if (status==FAILURE) goto Done;

	voice = track-1;
	newFirstSync = NILINK;

	/* - If this IS the tempo map track, create all Measures and time signatures for
	     the entire score, including extra Measures at the end to fill the score out to
	     <scoreEndTime> time. (Since our empty score already contained one Measure, we'll
	     then have one too many: we need the space between the first two Measures to
	     hold the stuff for each track. Of course, the extra Measure must be deleted when
	     we've done the last track; we leave that to the calling function because we can't
	     tell when that is.)
	   - If this ISN'T the tempo map track, convert any new notes/rests and merge them in. */
	   
	if (isTempoMap) {		
		measDur = TimeSigDur(0, measInfoTab[measTabLen-1].numerator,
										measInfoTab[measTabLen-1].denominator);
		count = (scoreEndTime-timeSigTime)/measDur;
		if (count*measDur<scoreEndTime-timeSigTime) count++;		/* Always round up */
		measInfoTab[measTabLen-1].count = count;
		if (!MFAddMeasures(doc, maxMeasures, &cStopTime)) goto Done;
		cStopTime = NTicks2MFTicks(cStopTime);
		if (DBG) LogPrintf(LOG_DEBUG, "cStart/StopTime=%ld/%ld quantum=%d tripBias=%d tryLev=%d leastSq=%d timeOff=%d\n",
							cStartTime, cStopTime, quantum, tripletBias,
							config.tryTupLevels, config.leastSquares, timeOffset);
	}
	else if (status==OP_COMPLETE) {
	
		/* It's not the tempo track, and we have one or more notes. Convert and merge
		   them. Damned if I can think of a good way to decide what <maxNewSyncs> should
		   be. But it certainly should fit in 16 bits, i.e., be 65535 or less, since the
		   total number of notes/rests in a score has to! */
		   
		maxNewSyncs = n_min(5L*nRawSyncs+MF_MAXPIECES, 65535L);
		newSyncTab = (LINKTIMEINFO *)NewPtr(maxNewSyncs*sizeof(LINKTIMEINFO));
		if (!GoodNewPtr((Ptr)newSyncTab))
			{ NoMoreMemory(); goto Done; }
			
		measL = SSearch(doc->headL, MEASUREtype, GO_RIGHT);
		firstL = RightLINK(measL);

		/* If quantization is called for, quantize and rhythmically clarify the notes and
		   chords we just generated; in any case, build Syncs and get information for
		   merging into <newSyncTab> ??REWRITE. */
		   
		if (DBG && voice==1) {
			DPrintMeasTab("X", measInfoTab, measTabLen);
			LogPrintf(LOG_INFO, "quantum=%d tripBias=%d timeOff=%d  (Track2Night)\n",
								quantum, tripletBias, timeOffset);
		}
		
		newFirstSync = TranscribeVoice(doc, voice, rawSyncTab, oneTrackTabSize, 
								nRawSyncs, rawNoteAux, nAux, quantum, tripletBias, measInfoTab,
								measTabLen, measL, newSyncTab, maxNewSyncs, &nNewSyncs, &tVErr);
		if (!newFirstSync) {
			GetIndCString(fmtStr, MIDIFILE_STRS, 30);    /* "Couldn't transcribe rhythm in track %d (error code=%d)." */
			sprintf(strBuf, fmtStr, track, tVErr); 
			CParamText(strBuf, "", "", "");
			StopInform(READMIDI_ALRT);
			goto Done;
		}
		
		mergeObjs = (LINKTIMEINFO *)NewPtr(mergeTabSize*sizeof(LINKTIMEINFO));
		if (!GoodNewPtr((Ptr)mergeObjs))
			{ NoMoreMemory(); goto Done; }

		nMergeObjs = FillMergeBuffer(firstL, mergeObjs, mergeTabSize, False);
		
		/* FIXME: The following check is currently pointless: if mergeTabSize isn't
			enough, FillMergeBuffer just stops without saying so! */
			
		if (nMergeObjs>mergeTabSize) {
			GetIndCString(fmtStr, MIDIFILE_STRS, 31);    /* "Can't handle MIDI files with more than %d Syncs." */
			sprintf(strBuf, fmtStr, mergeTabSize); 
			CParamText(strBuf, "", "", "");
			StopInform(READMIDI_ALRT);
			goto Done;
		}

		if (!MRMerge(doc, voice, newSyncTab, nNewSyncs, mergeObjs, nMergeObjs, &lastL))
			{ MayErrMsg("MRMerge failed."); goto Done; }

		if (quantum>1) {
			if (!PreflightMem(400))
				{ NoMoreMemory(); goto Done; }
			if (!FillNonemptyMeas(doc, NILINK, NILINK, voice, quantum)) goto Done;
		}
	}
	if (isLastTrack) {
		AddRelObjects(doc, rawSyncTab, nRawSyncs);
	}
	
	okay = True;

Done:
	if (rawSyncTab) DisposePtr((Ptr)rawSyncTab);
	if (rawNoteAux) DisposePtr((Ptr)rawNoteAux);
	if (mergeObjs) DisposePtr((Ptr)mergeObjs);
	if (newSyncTab) DisposePtr((Ptr)newSyncTab);
	
	if (!okay) return FAILURE;
	return (newFirstSync!=NILINK? OP_COMPLETE : NOTHING_TO_DO);
}


/* -------------------------------------------------------------------- MIDNight2Night -- */
/* Convert an entire MIDNight data structure into a Nightingale score. Return True
if we succeed, False if either there's an error or nothing useful in the MIDI file.

NB: Time resolution globals must be initialized first! See MFTicks2NTicks. */

short MIDNight2Night(
			Document *doc,
			TRACKINFO trackInfo[],
			short quantum,				/* For attacks and releases, in PDUR ticks, or 1=no quantize */
			short tripletBias,			/* Percent bias towards quant. to triplets >= 2/3 quantum */
			Boolean delRedundantAccs,	/* Delete redundant accidentals? */
			Boolean clefChanges,		/* Add clef changes to minimize ledger lines? */
			Boolean setPlayDurs,		/* Set note playDurs according to logical durs.? */
			short maxMeasures,			/* Transcribe no more than this many measures */
			long lastEvent				/* End-of-track time, in MIDI file (not PDUR!) ticks */
			)
{
	short t, status; long len, endTime; Boolean gotNotes=False;
	LINK measL;
	char fmtStr[256];
	long tLen;
	long ctlLen;
	
	len = MAX_TSCHANGE*sizeof(MEASINFO);
	measInfoTab = (MEASINFO *)NewPtr(len);
	if (!GoodNewPtr((Ptr)measInfoTab)) {
		OutOfMemory(len);
		goto Done;
	}
	tLen = MAX_MF_TEMPOCHANGE*sizeof(TEMPOINFO);
	tempoInfoTab = (TEMPOINFO *)NewPtr(tLen);
	if (!GoodNewPtr((Ptr)tempoInfoTab)) {
		OutOfMemory(tLen);
		goto Done;
	}
	ctlLen = MAX_CTRLCHANGE*sizeof(CTRLINFO);
	ctrlInfoTab = (CTRLINFO *)NewPtr(ctlLen);
	if (!GoodNewPtr((Ptr)ctrlInfoTab)) {
		OutOfMemory(tLen);
		goto Done;
	}

	/* Make sure the staves of a large system don't go off the bottom of the page. */
	
	FitStavesOnPaper(doc);

	/* Convert one track at a time. */
	
	for (t = 1; t<=mfNTracks; t++) {
		if (trackInfo[t].okay) {
			if (t==1)
				ProgressMsg(TIMINGTRACK_PMSTR, "...");
			else {
				GetIndCString(fmtStr, MIDIFILE_STRS, 32);			/* " %d..." */
				sprintf(strBuf, fmtStr, t); 
				ProgressMsg(OTHERTRACK_PMSTR, strBuf);
			}
			endTime = MFTicks2NTicks(lastEvent);
			status = Track2Night(doc, trackInfo, t, quantum, tripletBias, maxMeasures, endTime);
			if (status==FAILURE) break;
			if (status==OP_COMPLETE) gotNotes = True;
		}
		if (t<mfNTracks && CheckAbort()) {							/* Check for user cancelling */
			ProgressMsg(SKIPTRACKS_PMSTR, "");
			SleepTicks(120L);										/* So user has time to read the msg */
			break;
		}
	}
	ProgressMsg(0, "");
	
	if (status==FAILURE) {
		/* Bail out gracefully. */
			doc->changed = False;
			DoCloseDocument(doc);
		goto Done;
	}

	/*
	 * Clean up. Kill the extra Measure (see comments in Track2Night); try to get rid
	 *	of unisons in chords; and, if desired, delete redundant accidentals, add clef
	 *	clef changes, and set playDurs from logical durs., if there are any. If "auto-
	 *	matic" beaming is wanted, it can be done by the caller.
	 */
	if (gotNotes) {
		measL = SSearch(doc->headL, MEASUREtype, GO_RIGHT);
		measL = LinkRMEAS(measL);
		if (measL) DeleteMeasure(doc, measL);
		UpdateMeasNums(doc, NILINK);

		for (t = 2; t<=mfNTracks; t++)
			AvoidUnisonsInRange(doc, doc->headL, doc->tailL, t-1);

		doc->selStartL = doc->headL;
		doc->selEndL = doc->tailL;

		if (delRedundantAccs) DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);

		if (clefChanges) (void)AddClefChanges(doc);
		
		if (setPlayDurs && quantum>1) SetPDur(doc, config.legatoPct, False);

#ifdef NOTYET
		if (contains empty measures and user asked) NMFillEmptyMeas(doc);	/* ??too high level? */
#endif
	}

Done:
	if (measInfoTab) DisposePtr((Ptr)measInfoTab);
	if (tempoInfoTab) DisposePtr((Ptr)tempoInfoTab);
	if (ctrlInfoTab) DisposePtr((Ptr)ctrlInfoTab);

	if (status==FAILURE) return FAILURE;	
	return (gotNotes? OP_COMPLETE : NOTHING_TO_DO);
}


/* --------------------------------------------------------------------- MFRespAndRfmt -- */

Boolean MFRespAndRfmt(Document *doc, short quantCode)
{
	LINK measL, pL; long spacePercent;
	
	/* If any Measure has too much in it, for now, give up. But it would be MUCH
		better to just add Measures as needed to keep under the limit. */
		
	if (AnyOverfullMeasure(doc)) return False;

	measL = LSSearch(doc->headL, MEASUREtype, 1, GO_RIGHT, False);		/* Start at first measure */
	
	/* If we KNOW there aren't going to be any short notes, save some space. */
	
	spacePercent = 100L;
	if (quantCode==EIGHTH_L_DUR) spacePercent = 85L;
	else if (quantCode<EIGHTH_L_DUR && quantCode!=UNKNOWN_L_DUR) spacePercent = 70L;
	doc->spacePercent = spacePercent;

	RespaceBars(doc, measL, doc->tailL, RESFACTOR*spacePercent, False, True);

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		if (SlurTYPE(pL))
			SetAllSlursShape(doc, pL, True);

	return True;
}
