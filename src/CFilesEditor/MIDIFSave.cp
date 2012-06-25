/* MIDIFSave.c for Nightingale - routines to write MIDI files - rev. for v.2000

A lot of this is pretty closely analogous to the routines for playing scores located
in MIDIGeneral.c (q.v.).
*/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1992-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

//#include "MidiGeneral.h"

static OSErr fRefNum;				/* ID of currently open file */
static OSErr errCode;				/* Latest report from the front */

static Byte midiFileFormat;
static Word nTracks, timeBase;
			
static MIDIEvent	eventList[MAXMFEVENTLIST];
static short		lastEvent;

static long 		qtrNTicks,				/* Ticks per quarter in Nightingale */
						trackLength,
						lenPosition;			/* File pos. of length field of current chunk */
static DoubleWord curTime;

static Boolean	WriteChunkStart(DoubleWord, DoubleWord);
static Boolean	WriteVarLen(DoubleWord);
Boolean	WriteDeltaTime(DoubleWord);

void		StartNote(short, short, short, long);
Boolean	EndNote(short, short, long);


/* -------------------------------------------------------------- WriteChunkStart -- */

static Boolean WriteChunkStart(DoubleWord chunkType, DoubleWord len)
{
	long byteCount; DoubleWord data[2];
	
	GetFPos(fRefNum, &lenPosition);
	lenPosition += sizeof(DoubleWord);
	
	data[0] = chunkType;
	data[1] = len;
	byteCount = 2*sizeof(DoubleWord);
	errCode = FSWrite(fRefNum, &byteCount, data);
	
	return (errCode==noError);
}


/* ------------------------------------------------------------------ WriteVarLen -- */
/* Write <value> out as a MIDI file "variable-length number" and return TRUE.

Legal variable-length numbers are non-negative and <=2^28-1; if <value> is illegal,
or if we have trouble writing it, return FALSE. */

static Boolean WriteVarLen(DoubleWord value)
{
	long buffer, byteCount; Byte b;
	
	if (value<0 || value>0x0FFFFFFF) return FALSE;

	buffer = value & 0x7F;
	while ((value >>= 7) > 0) {
		buffer <<=8;					/* move byte up one place */
		buffer |= 0x80;				/* set MSB of lowest byte, next byte of delta value */
		buffer += (value & 0x7F);	/* put 7-bit value in buffer */
	}

	byteCount = 1;	
	while (TRUE)
	{
		b = (Byte)buffer;				/* truncate to lowest byte */
		errCode = FSWrite(fRefNum, &byteCount, &b); 
		trackLength += byteCount;
		   
		if (buffer & 0x80) buffer >>= 8;
		else					 break;
	}
	
	return (errCode==noError);
}


/* --------------------------------------------------------------- WriteDeltaTime -- */
/* Write the delta time corresponding to the given absolute time. */

Boolean WriteDeltaTime(DoubleWord absTime)
{
	long dTime;
	
	dTime = absTime-curTime;
	if (dTime<0L) return FALSE;						/* Negative delta time is illegal */
	if (WriteVarLen(dTime))
		{ curTime = absTime; return TRUE; }
	else
		return FALSE;
}


#define METAEVENT 0xFF			/* Meta-event code. Subtypes: */

#define ME_SEQTRACKNAME 0x03	/* Name of the sequence or track */

#define ME_EOT 0x2F				/* End-of-track */
#define ME_TEMPO 0x51			/* Set Tempo: 3 bytes, in microseconds per MIDI qtr-note */
#define ME_SMPTE 0x54			/* SMPTE offset */
#define ME_TIMESIG 0x58			/* Time signature */
#define ME_KEYSIG 0x59			/* Key signature: -7 = 7 flats...7 = 7 sharps */

/* ----------------------------------------------------------- WriteHeader, etc. -- */

static Boolean WriteHeader(Byte, Word, Word);
static void WriteTrackEnd(void);
static void WriteTempoEvent(unsigned long);
static void WriteTextEvent(Byte, char []);
static void WriteTSEvent(short, short, short);

static void FillInTrackLength(void);

static void WriteNote(Byte, Byte, Byte);
static void WriteNoteOn(Byte, Byte, Byte);
static void WriteNoteOff(Byte, Byte);

static Boolean cmFSSustainOn[MAXSTAVES + 1];
static Boolean cmFSSustainOff[MAXSTAVES + 1];
static Byte cmFSPanSetting[MAXSTAVES + 1];

static Boolean cmFSAllSustainOn[MAXSTAVES + 1];
static Byte cmFSAllPanSetting[MAXSTAVES + 1];


static Boolean WriteHeader(Byte format, Word nTracks, Word timeBase)
{
	long byteCount;
	Word data[3];
	
	if (!WriteChunkStart('MThd', 6L)) return FALSE;

	data[0] = format;
	data[1] = nTracks;
	data[2] = timeBase;	

	byteCount = 3*sizeof(Word);
	errCode = FSWrite(fRefNum, &byteCount, data);

	return (errCode==noError);
}

static void WriteTrackEnd()
{
	long byteCount;
	Byte buffer[3];
	
	/* Write meta byte and two bytes for end-of-track */
	buffer[0] = METAEVENT;
	buffer[1] = ME_EOT;
	buffer[2] = (Byte)0;

	byteCount = 3L;
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
}	

static void WriteTempoEvent(
					unsigned long microsecPQ)	/* tempo in microseconds per quarter-note */
{
	long 	byteCount; 
	Byte	buffer[6];
	
	/* Write meta byte, 2 bytes identifying tempo event, etc.: total of 6 bytes */
	buffer[0] = METAEVENT;
	buffer[1] = ME_TEMPO;
	buffer[2] = (Byte)3;
	buffer[3] = (Byte)(microsecPQ>>16);
	buffer[4] = (Byte)(microsecPQ>>8);
	buffer[5] = (Byte)microsecPQ;

	byteCount = 6L;
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
}	

static void WriteTSEvent(short numerator, short denominator, short clocksPerBeat)
{
	long 	byteCount;
	Byte	logDenom, thirtySeconds;
	Byte	buffer[7];
	
	logDenom = 0;
	while (!odd(denominator)) {
		logDenom++; denominator /= 2;
	}

	thirtySeconds = 8;	/* notated 32nds per MIDI quarter-note: 8 is standard */
	
	/* Write meta byte, 2 bytes identifying time sig event, etc.: total of 7 bytes */
	buffer[0] = METAEVENT;
	buffer[1] = ME_TIMESIG;
	buffer[2] = (Byte)4;
	buffer[3] = numerator;
	buffer[4] = logDenom;
	buffer[5] = clocksPerBeat;
	buffer[6] = thirtySeconds;

	byteCount = 7L;	
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
}	


static void WriteTextEvent(Byte mEventType,
									char text[])			/* C string: must be <=255 chars! */
{
	long 				byteCount; 
	unsigned char	buffer[259];	/* Be careful: Mixture of binary and char. data! */
	short				i;
	
	/* Write meta byte, 2 bytes identifying text event subtype, text */
	buffer[0] = METAEVENT;
	buffer[1] = mEventType;
	buffer[2] = strlen(text);
	for (i = 0; i<buffer[2]; i++) {
		buffer[3+i] = (unsigned char)text[i];
	}

	byteCount = 3L+buffer[2];
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
}	

static void FillInTrackLength()
{
	long byteCount, savePosition;
	
	GetFPos(fRefNum, &savePosition);

	/* Position file pointer at track length (previously filled with dummy value) */
	errCode = SetFPos(fRefNum, fsFromStart, lenPosition);
	if (errCode!=noErr) return;
	byteCount = sizeof(trackLength);
	errCode = FSWrite(fRefNum, &byteCount, &trackLength); 
	if (errCode!=noErr) return;

	/* Move back to where we just were so as not to truncate anything */
	errCode = SetFPos(fRefNum, fsFromStart, savePosition);
}	


static void WriteNote(Byte midiEvent, Byte noteNum, Byte veloc)
{
	Byte buffer[3];
	long byteCount;
	
	buffer[0] = midiEvent;
	buffer[1] = noteNum;
	buffer[2] = veloc;

	byteCount = 3L;	
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
}	

static void WriteNoteOn(Byte channel, Byte noteNum, Byte velocity)
{
	WriteNote(MNOTEON+channel-1, noteNum, velocity);
}

/* Instead of writing Note Offs with default velocity , it would be much better
to use the note-off velocity Nightingale has stored for each note! Should be
easy but requires adding off-velocity to the MIDIEvent struct. */

static void WriteNoteOff(Byte channel, Byte noteNum)
{
	WriteNote(MNOTEOFF+channel-1, noteNum, config.noteOffVel);
}

static void WriteCtrlChange(Byte midiEvent, Byte ctrlNum, Byte ctrlValue)
{
	Byte buffer[3];
	long byteCount;
	
	buffer[0] = midiEvent;
	buffer[1] = ctrlNum;
	buffer[2] = ctrlValue;

	byteCount = 3L;	
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
}	

static void WriteControlChange(Byte channel, Byte ctrlNum, Byte ctrlValue)
{
	WriteCtrlChange(MCTLCHANGE+channel-1, ctrlNum, ctrlValue);
}

/* --------------------------------------------------------- Posting controllers -- */

static Boolean MFSPostMidiSustain(Document *doc, LINK pL, short staffn, Boolean susOn) 
{
	Boolean posted = FALSE;
						
	PGRAPHIC pGraphic = GetPGRAPHIC(pL);
	short stf = GraphicSTAFF(pL);
	if (stf > 0 && stf == staffn) 
	{
		if (susOn) {
			cmFSSustainOn[stf] = cmFSAllSustainOn[stf] = TRUE;
		}		
		else {
			cmFSSustainOff[stf] = TRUE;			
		}
		posted = TRUE;
	}
	
	return posted;
}

static Boolean MFSPostMidiPan(Document *doc, LINK pL, short staffn)
{
	Boolean posted = FALSE;
	
	PGRAPHIC pGraphic = GetPGRAPHIC(pL);
	short stf = GraphicSTAFF(pL);
	if (stf > 0 && stf == staffn) 
	{
		Byte panSetting = GraphicINFO(pL);
		cmFSPanSetting[stf] = cmFSAllPanSetting[stf] = panSetting;
		posted = TRUE;
	}
	
	return posted;
}

static void MFSClearMidiSustain(Boolean susOn) 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (susOn) {
			cmFSSustainOn[j] = FALSE;
		}		
		else {
			cmFSSustainOff[j] = FALSE;			
		}
	}	
}

static void MFSClearMidiPan() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmFSPanSetting[j] = -1;
	}
}

static void MFSClearAllMidiSustainOn() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmFSAllSustainOn[j] = FALSE;
	}
}

static void MFSClearAllMidiPan() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmFSAllPanSetting[j] = -1;
	}
}

// From MidiGeneral.h
//
// #define MPAN 0x0A
// #define MSUSTAIN 0x40

static Byte GetSustainCtrlVal(Boolean susOn) 
{
	Byte ctrlVal;
	
	if (susOn) {
		ctrlVal = 127;
	}
	else {
		ctrlVal = 0;		
	}
	return ctrlVal;
}

static void WriteAllMidiSustains(Document *doc, Byte *partChannel, Boolean susOn, long startTime) 
{
	Byte ctrlNum = MSUSTAIN;
	Byte ctrlVal = GetSustainCtrlVal(susOn);
	
	DebugPrintf("Write: ctrlNum=%ld ctrlVal=%ld time=%ld\n",ctrlNum, ctrlVal, startTime);
	if (susOn) {
		for (int j = 1; j<=MAXSTAVES; j++) {
			if (cmFSSustainOn[j]) {
				short partn = Staff2Part(doc,j);
				short channel = partChannel[partn];
				WriteDeltaTime(startTime);
				WriteControlChange(channel, ctrlNum, ctrlVal);									
			}
		}
	}
	else {
		for (int j = 1; j<=MAXSTAVES; j++) {
			if (cmFSSustainOff[j]) {
				short partn = Staff2Part(doc,j);
				short channel = partChannel[partn];
				WriteDeltaTime(startTime);
				WriteControlChange(channel, ctrlNum, ctrlVal);									
			}
		}		
	}
}

static void WriteMidiSustains(Document *doc, Byte *partChannel, Boolean susOn, long startTime, LINK pL, short stf) 
{
	LINK graphicL = LSSearch(pL, GRAPHICtype, stf, GO_LEFT, FALSE);
	
	while (graphicL != NILINK && GraphicFIRSTOBJ(graphicL) == pL) 
	{
		if (susOn == TRUE && IsMidiSustainOn(graphicL) ||
			 susOn == FALSE && IsMidiSustainOff(graphicL)) 
		{
			Byte ctrlNum = MSUSTAIN;
			Byte ctrlVal = GetSustainCtrlVal(susOn);
			
			DebugPrintf("Write: ctrlNum=%ld ctrlVal=%ld time=%ld\n",ctrlNum, ctrlVal, startTime);
			
			short partn = Staff2Part(doc,stf);
			short channel = partChannel[partn];
			WriteDeltaTime(startTime);
			WriteControlChange(channel, ctrlNum, ctrlVal);		
		}
		
		graphicL = LSSearch(LeftLINK(graphicL), GRAPHICtype, stf, GO_LEFT, FALSE);
	}
}

static void WriteAllMidiSustains(Document *doc, Byte *partChannel, Boolean susOn, long startTime, LINK pL, short stf) 
{	
	LINK aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) 
	{
		if (NoteSTAFF(aNoteL) == stf)
			WriteMidiSustains(doc, partChannel, susOn, startTime, pL, NoteSTAFF(aNoteL));
	}
}

// TODO: from MIDIPlay.c

static Boolean ValidPanSetting(Byte panSetting) 
{
	SignedByte sbpanSetting = (SignedByte)panSetting;
	
	return sbpanSetting >= 0;
}

static void WriteAllMidiPans(Document *doc, Byte *partChannel, long startTime) 
{	
	Byte ctrlNum = MPAN;

	for (int j = 1; j<=MAXSTAVES; j++) 
	{
		Byte ctrlVal = cmFSPanSetting[j];
		if (ValidPanSetting(ctrlVal))
		{
			short partn = Staff2Part(doc,j);
			short channel = partChannel[partn];
			WriteDeltaTime(startTime);
//			WriteDeltaTime(0);
			WriteControlChange(channel, ctrlNum, ctrlVal);									
		}
	}
}

static void WriteMidiPans(Document *doc, Byte *partChannel, long startTime, short stf) 
{	
	Byte ctrlNum = MPAN;

	Byte ctrlVal = cmFSPanSetting[stf];
	if (ValidPanSetting(ctrlVal))
	{
		short partn = Staff2Part(doc,stf);
		short channel = partChannel[partn];
		WriteDeltaTime(startTime);
		WriteControlChange(channel, ctrlNum, ctrlVal);
		SignedByte sbpanSetting = -1;
		cmFSPanSetting[stf] = (Byte)sbpanSetting;
	}
}

static void WriteMidiPans(Document *doc, Byte *partChannel, long startTime, LINK pL, short stf) 
{	
	LINK graphicL = LSSearch(pL, GRAPHICtype, stf, GO_LEFT, FALSE);
	while (graphicL != NILINK && GraphicFIRSTOBJ(graphicL) == pL) 
	{
		if (IsMidiPan(graphicL)) 
		{
			Byte ctrlNum = MPAN;
			Byte ctrlVal = GraphicINFO(graphicL);
			
			DebugPrintf("Write: ctrlNum=%ld ctrlVal=%ld time=%ld\n",ctrlNum, ctrlVal, startTime);
			
			short partn = Staff2Part(doc,stf);
			short channel = partChannel[partn];
			WriteDeltaTime(startTime);
			WriteControlChange(channel, ctrlNum, ctrlVal);					
		}
		
		graphicL = LSSearch(LeftLINK(graphicL), GRAPHICtype, stf, GO_LEFT, FALSE);
	}
}

static void WriteAllMidiPans(Document *doc, Byte *partChannel, long startTime, LINK pL, short stf) 
{	
	LINK aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) 
	{
		if (NoteSTAFF(aNoteL) == stf)
			WriteMidiPans(doc, partChannel, startTime, pL, NoteSTAFF(aNoteL));
	}
}

/* --------------------------------------------------------- Event List functions -- */

static Boolean	MFInsertEvent(char, char, long);
static Boolean	MFCheckEventList(long);

/*	Insert the specified note into the event list. */

static Boolean MFInsertEvent(
						char note,
						char channel,		/* 1 to MAXCHANNEL */
						long endTime
						)
{
	short			i;
	MIDIEvent	*pEvent;
	char			fmtStr[256];

	/* Find first free slot in list, which may be at lastEvent (end of list) */
	
	for (i=0, pEvent=eventList; i<lastEvent; i++,pEvent++)
		if (pEvent->note == 0) break;
	
	/* Insert note into free slot, or append to end of list if there's room */
	
	if (i<lastEvent || lastEvent++<MAXMFEVENTLIST) {
		pEvent->note = note;
		pEvent->channel = channel;
		pEvent->endTime = endTime;
		return TRUE;
	}
	else {
		lastEvent--;			
		GetIndCString(fmtStr, MIDIFILE_STRS, 33);			/* "Nightingale can't save MIDI files with more than %d notes playing at once." */
		sprintf(strBuf, fmtStr, MAXMFEVENTLIST); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
}


/*	Checks eventList[] to see if any notes are ready to be turned off; if so, frees
their slots in the eventList and writes out Note Offs. Returns TRUE if the list is
empty. */

static Boolean MFCheckEventList(long time)
{
	Boolean		empty;
	MIDIEvent	*pEvent;
	short			i;
	
	empty = TRUE;
	for (i=0, pEvent = eventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
			empty = FALSE;
			if (pEvent->endTime<=time) {											/* note is done */
					WriteDeltaTime(time);
					WriteNoteOff(pEvent->channel, pEvent->note);
				pEvent->note = 0;													/* slot available now */
			}
		}

	return empty;
}


/* ----------------------------------------------------------- StartNote, EndNote -- */

void StartNote(
				short	noteNum,
				short	channel,			/* 1 to MAXCHANNEL */
				short	velocity,
				long	time
				)
{
	if (noteNum>=0) {
		WriteDeltaTime(time);
		WriteNoteOn(channel, noteNum, velocity);
	}
}

Boolean EndNote(
				short	noteNum,
				short	channel,			/* 1 to MAXCHANNEL */
				long	endTime
				)
{
	return MFInsertEvent(noteNum, channel, endTime);
}


/* ------------------------------------------------------ WriteTrack and helpers -- */

static Boolean WriteTiming(Document *, long);
static short WriteMFNotes(Document *, short, LINK, LINK, long);
static Boolean WriteTrackName(Document *, short);
static Boolean WriteTrack(Document *, short, long, short *);
//static long LastEndTime(Document *, LINK, LINK);
static Boolean WriteMIDIFile(Document *);
static short CheckMeasDur(Document *);

static Boolean WriteTSig(LINK, LINK, long);
static Boolean WriteTSig(
						LINK /*timeSigL*/,	/* Time signature object (unused) */
						LINK aTSL,				/* Time sig. subobject */
						long absTime
						)
{
	PATIMESIG aTS;
	short numer, denom, tempDenom, clocksPerBeat;

	aTS = GetPATIMESIG(aTSL);
	numer = aTS->numerator;
	denom = aTS->denominator;

	tempDenom = denom;
	clocksPerBeat = 24*4;								/* 24 is standard/qtr, and 4 qtrs/whole */
	while (!odd(tempDenom)) {
		clocksPerBeat /= 2; tempDenom /= 2;
	}
	if (COMPOUND(numer)) clocksPerBeat *= 3;

	WriteDeltaTime(absTime);
	WriteTSEvent(numer, denom, clocksPerBeat);

	return TRUE;
}


static Boolean WriteTiming(
						Document *doc,
						long lastEndTime		/* The latest ending time for any track */
						)
{
	long		prevTSTime=-1L;
	LINK		pL, aTSL;//, firstSyncL;
	long		measureTime,
				microbeats,							/* microsec. units per PDUR tick */
				timeScale,							/* PDUR ticks per minute */
				endTime;
	
	/* Write initial tempo, the tempo as of the score's first note. */
	
#if 0
	firstSyncL = SSearch(doc->headL, SYNCtype, GO_RIGHT);
	WriteDeltaTime(0L);
	timeScale = GetTempo(doc, firstSyncL);
	microbeats = TSCALE2MICROBEATS(timeScale);
	WriteTempoEvent((long)microbeats*DFLT_BEATDUR);
//DebugPrintf("WriteTiming: firstSyncL=%d timeScale=%ld\n", firstSyncL, timeScale);
#endif

	measureTime = 0L;
	for (pL = doc->headL; pL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case MEASUREtype:
				measureTime = MeasureTIME(pL);
				break;
			case TEMPOtype:
				/* Write tempo changes. ??When implemented, remove separate code for initial tempo! */
				endTime = LastEndTime(doc, doc->headL, pL);
				WriteDeltaTime(endTime);
				timeScale = GetTempo(doc, pL);
				microbeats = TSCALE2MICROBEATS(timeScale);
				WriteTempoEvent((long)microbeats*DFLT_BEATDUR);
				break;
			case TIMESIGtype:
				/* In view of Nightingale's insistence that all barlines align, for now
					we'll just get get the time sig. from staff 1 and ignore other staves.
					If there's an obvious problem, we should have warned the user (cf.
					CheckMeasDur()). Also, if we've already written a time sig. at this time,
					skip this one: it's probably a time sig. beginning a system with an
					identical preparatory one at the end of the last system. */
					
				aTSL = TimeSigOnStaff(pL, 1);
//DebugPrintf("WriteTiming: TIMESIG pL=%d aTSL=%d measureTime=%ld %c\n", pL, aTSL,
//measureTime, (measureTime!=prevTSTime? ' ' : 'S'));
				if (aTSL && measureTime!=prevTSTime) {
					WriteTSig(pL, aTSL, measureTime);		/* Assume time sig. at beginning of measure! */
					prevTSTime = measureTime;
				}
				break;
			default:
				;
	}
	
	WriteDeltaTime(lastEndTime);
	WriteTrackEnd();
	
	return TRUE;
}


#define USEPARTVELO FALSE

/* Write all notes with nonzero On velocity, in the given range on <staffn> or all
staves, to the current track of the open MIDI file. Analogous to PlaySequence, except
PlaySequence always does all staves. Returns the number of notes skipped because they
have zero On velocity.

NB: if the measures/notes have inconsistent timestamps (as detected by DCheckNode),
this writes nonsense Note Ons--I have no idea why--resulting in tracks that our own
Import MIDI File says are "damaged or incomplete". But this should never happen (though
it did in v.998a20!), so don't worry about it. */

static short WriteMFNotes(
					Document *doc,
					short		staffn,					/* Staff no. or ANYONE */
					LINK		fromL, LINK toL,		/* range to be written */
					long		lastEndTime				/* The latest ending time for any track */
					)
{
	LINK			pL, aNoteL;
	LINK			partL, measL, newMeasL;
	PANOTE		aNote;
	short			i, nZeroVel;
	short			useNoteNum,
					useChan, useVelo, partn;
	long			t,
					toffset,							/* PDUR start time of 1st note played */
					playDur,
					plStartTime, plEndTime,		/* in PDUR ticks */
					startTime, endTime;
	PARTINFO		aPart;
	char			partVelo[MAXSTAVES];
	Byte			partChannel[MAXSTAVES];
	short			partTransp[MAXSTAVES];
	Boolean		anyStaff;
	
	Boolean		patchChangePosted = FALSE;
	Boolean		sustainOnPosted = FALSE;
	Boolean		sustainOffPosted = FALSE;
	Boolean		panPosted = FALSE;
		
	anyStaff = (staffn==ANYONE);

	partL = FirstSubLINK(doc->headL);
	for (i = 0; i<=LinkNENTRIES(doc->headL)-1; i++, partL = NextPARTINFOL(partL)) {
		aPart = GetPARTINFO(partL);
		partVelo[i] = aPart.partVelocity;
		partChannel[i] = UseMIDIChannel(doc, i);
		partTransp[i] = aPart.transpose;
	}

	MFSClearMidiSustain(TRUE);
	MFSClearMidiSustain(FALSE);
	MFSClearMidiPan();
	
	MFSClearAllMidiSustainOn();
	MFSClearAllMidiPan();

	lastEvent = 0;													/* start with empty Event list */

	newMeasL = measL = SSearch(fromL, MEASUREtype, GO_LEFT);	/* starting measure */
	
	t = 0L;
	nZeroVel = 0;
 
	/* The stored play time of the first Sync we're going to write might be any
	 *	positive value but we want written times to start at 0, so we'll pick up
	 *	the first Sync's play time and use it as an offset on all play times.
	 */
	toffset = -1L;													/* Init. time offset to unknown */
	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:
				newMeasL = measL = pL;
				break;
			case SYNCtype:
		  		if (SyncTIME(pL)>MAX_SAFE_MEASDUR)
		  			MayErrMsg("WriteMFNotes: pL=%ld has timeStamp=%ld", (long)pL,
		  							(long)SyncTIME(pL));
#ifdef TDEBUG
				if (toffset<0L) DebugPrintf("toffset=%ld => %ld\n", toffset, MeasureTIME(measL)+SyncTIME(pL));
#endif
		  		plStartTime = MeasureTIME(measL)+SyncTIME(pL);
		  		startTime = plStartTime;
		  		if (toffset<0L) toffset = startTime;
				startTime -= toffset;
	
				/*
				 * Move time forward 'til it's time to "play" (write) pL. While doing so,
				 *	check for notes ending before OR AT pL's time and take care of them.
				 * We have to write notes ending at a given time before those beginning
				 * at the same time because otherwise--if there's a new note of the same
				 * note number--we'll end up saying that a note starts, then ends instantly.
				 */
				for ( ; t<=startTime; t++) {
					if (UserInterrupt()) goto Done;		/* Check for Cancel */
					MFCheckEventList(t);						/* Check for, turn off any notes done */
				}

				if (newMeasL) {
					newMeasL = NILINK;
				}
	
//				if (patchChangePosted) {
//					CMMIDIProgram(doc, partPatch, partChannel);
//					patchChangePosted = FALSE;
//				}
//				if (sustainOnPosted) {
//					WriteAllMidiSustains(doc, partChannel, TRUE, startTime);
//					MFSClearMidiSustain(TRUE);
//					sustainOnPosted = FALSE;
//				}
//				if (sustainOffPosted) {
//					WriteAllMidiSustains(doc, partChannel, FALSE, startTime);
//					MFSClearMidiSustain(FALSE);
//					sustainOffPosted = FALSE;
//				}
//				if (panPosted) {
//					WriteAllMidiPans(doc, partChannel, startTime);
//					MFSClearMidiPan();
//					panPosted = FALSE;
//				}

				WriteAllMidiSustains(doc, partChannel, TRUE, startTime, pL, staffn);
				WriteAllMidiSustains(doc, partChannel, FALSE, startTime, pL, staffn);
				WriteAllMidiPans(doc, partChannel, startTime, pL, staffn);
				
	/* Write all the notes on <staffn> in <pL>, adding them to <eventList[]> as well */
	
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					if (anyStaff || NoteSTAFF(aNoteL)==staffn) {
						if (NoteREST(aNoteL)) continue;
						/*
						 * If note has zero on velocity, it's wierd: skip it completely. If
						 * it's a chord slash, it's not interesting, else count it.
						 */
						aNote = GetPANOTE(aNoteL);						
						if (aNote->onVelocity==0) {
							if (NoteAPPEAR(aNoteL)!=SLASH_SHAPE) nZeroVel++;
							continue;
						}

						partn = Staff2Part(doc,NoteSTAFF(aNoteL));
						/*
						 *	Get note's MIDI note number, including transposition; velocity,
						 *	limited to legal range; channel number; and duration, includ-
						 * ing any tied notes to right.
						 */
						useNoteNum = UseMIDINoteNum(doc, aNoteL, partTransp[partn]);
						useChan = partChannel[partn];
						aNote = GetPANOTE(aNoteL);
						useVelo = doc->velocity+aNote->onVelocity;
						if (USEPARTVELO) useVelo += partVelo[partn];
	
						if (useVelo<1) useVelo = 1;
						if (useVelo>MAX_VELOCITY) useVelo = MAX_VELOCITY;
						
						playDur = TiedDur(doc, pL, aNoteL, FALSE);
						plEndTime = plStartTime+playDur;						
	
						/* If it's a real note (not rest or continuation), send it out */
						
						if (useNoteNum>=0) {
							StartNote(useNoteNum, useChan, useVelo, startTime);
							endTime = plEndTime-toffset;
							if (!EndNote(useNoteNum, useChan, endTime))
								goto Done;
						}
					}
				}
				break;
				
			case GRAPHICtype:			
				if (IsMidiController(pL)) 
				{
#if 0
					Byte ctrlNum = GetMidiControlNum(pL);
					Byte ctrlVal = GetMidiControlVal(pL);
					short stf = GraphicSTAFF(pL);
					//if (anyStaff || stf == staffn) {
					if (stf==staffn) {
						short partn = Staff2Part(doc,stf);
						//short channel = CMGetUseChannel(partChannel, partn);
						short channel = partChannel[partn];

						WriteDeltaTime(startTime);
						WriteControlChange(channel, ctrlNum, ctrlVal);						
					}
#else
					if (useWhichMIDI==MIDIDR_CM) {
									
	//					if (IsMidiPatchChange(pL)) {
	//						patchChangePosted = MFSPostMidiProgramChange(doc, pL, partPatch, partChannel);						
	//					}
	//					else 
						
						if (IsMidiSustainOn(pL)) 
						{
							sustainOnPosted = MFSPostMidiSustain(doc, pL, staffn, TRUE);
						}
						else if (IsMidiSustainOff(pL)) 
						{
							sustainOffPosted = MFSPostMidiSustain(doc, pL, staffn, FALSE);
						}
						else if (IsMidiPan(pL)) 
						{
							panPosted = MFSPostMidiPan(doc, pL, staffn);
						}					
					}
#endif
				}
				break;
								
			default:
				;
		}
	}
			
	for ( ; t<=lastEndTime; t++) {
		if (UserInterrupt()) goto Done;			/* Check for Cancel */
		MFCheckEventList(t);							/* Check for, turn off any notes done */
	}

Done:	
	WriteDeltaTime(t);
	WriteTrackEnd();
	
	return nZeroVel;
}


static Boolean WriteTrackName(Document *doc, short staffn)
{
	LINK partL; char str[256]; PPARTINFO aPart;

	partL = Staff2PartL(doc, doc->headL, staffn);

	aPart = GetPPARTINFO(partL);
	strcpy(str, aPart->name);
	WriteDeltaTime(0L);
	WriteTextEvent(ME_SEQTRACKNAME, str);
	
	return TRUE;
}


static Boolean WriteTrack(
						Document *doc,
						short track,
						long lastEndTime,			/* The latest ending time for any track */
						short *pnZeroVel
						)
{
	short nZeroVel;
	
	trackLength = 0L;
	curTime = 0L;

	WriteChunkStart('MTrk', 0L);						/* Dummy tracklength: fill in later */

	if (track==1)
		WriteTiming(doc, lastEndTime);
	else {
		WriteTrackName(doc, track-1);
		nZeroVel = WriteMFNotes(doc, track-1, doc->headL, doc->tailL, lastEndTime);	/* stf = trk-1 */
	}

	FillInTrackLength();
	
	if (errCode!=noErr) ReportIOError(errCode, SAVEMF_ALRT);
	
	*pnZeroVel = nZeroVel;
	return (errCode==noErr);
}


long LastEndTime(Document *doc, LINK fromL, LINK toL)
{
	LINK 		measL, pL, aNoteL;
	PANOTE 	aNote;
	long		lastET, endTime, playDur,
				toffset,							/* PDUR start time of 1st note played */
				plStartTime, plEndTime;		/* in PDUR ticks */
	
	lastET = 0L;

	measL = SSearch(fromL, MEASUREtype, GO_LEFT);					/* starting measure */

	/* The stored play time of the first Sync we're going to write might be any
	 *	positive value but we want written times to start at 0, so we'll pick up
	 *	the first Sync's play time and use it as an offset on all play times.
	 */
	toffset = -1L;													/* Init. time offset to unknown */
	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:
				measL = pL;
				break;
			case SYNCtype:
		  		if (SyncTIME(pL)>MAX_SAFE_MEASDUR)
		  			MayErrMsg("LastEndTime: pL=%ld has timeStamp=%ld", (long)pL,
		  							(long)SyncTIME(pL));
		  		plStartTime = MeasureTIME(measL)+SyncTIME(pL);
		  		if (toffset<0L) toffset = plStartTime;
	
				/* Find the last ending time of all sounding notes in <pL> */
	
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
									
					/* If it's a real note (not rest or continuation or vel. 0), consider it */
					
					aNote = GetPANOTE(aNoteL);
					if (!aNote->rest && !aNote->tiedL &&  aNote->onVelocity!=0) {
						playDur = TiedDur(doc, pL, aNoteL, FALSE);
						plEndTime = plStartTime+playDur;						
						endTime = plEndTime-toffset;
						lastET = n_max(endTime, lastET);
					}
				}
				break;
			default:
				;
		}
	}

	return lastET;
}

/* --------------------------------------- WriteMIDIFile, SaveMIDIFile and helpers -- */

static Boolean WriteMIDIFile(Document *doc)
{
	short t, nZeroVel;
	long lastEndTime;								/* The latest ending time for any track */

	/* Write the MIDI file header, then the tracks, one for timing info plus one
		for each staff of the score. */
	
	midiFileFormat = 1;
	nTracks = doc->nstaves+1;
	qtrNTicks = Code2LDur(QTR_L_DUR, 0);
	timeBase = qtrNTicks;

	if (!WriteHeader(midiFileFormat, nTracks, timeBase)) {
		MayErrMsg("Unable to write MIDI file header.");
		return FALSE;
	}
	
	lastEndTime = LastEndTime(doc, doc->headL, doc->tailL);
	
	for (t = 1; t<=nTracks; t++)
		/* Ignore any zero-velocity notes: assume the user has already been warned! */
		
		if (!WriteTrack(doc, t, lastEndTime, &nZeroVel)) return FALSE;
	
	return TRUE;
}


/* Check the actual duration of notes in every measure, considered as a whole, against
its time signature on every staff; if any irregularities are found, return the measure
number of the first, else return -1. Identical to DCheckMeasDur, except this returns
a measure number and doesn't give error messages if it finds problems, it ignores
completely empty measures, and it doesn't check the very last measure. */

static short CheckMeasDur(Document *doc)
{
	LINK pL, barTermL;
	long measDurNotated, measDurActual;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (MeasureTYPE(pL) && !FakeMeasure(doc, pL)) {
			barTermL = EndMeasSearch(doc, pL);
			if (barTermL && barTermL!=doc->tailL) {
				measDurNotated = NotatedMeasDur(doc, barTermL);
				if (measDurNotated<0) return FALSE;
				measDurActual = GetMeasDur(doc, barTermL);
				if (measDurActual!=0 && measDurNotated!=measDurActual)
					return GetPAMEASURE(FirstSubLINK(pL))->measureNum+doc->firstMNNumber;
			}
		}
	}
		
	return -1;
}


/* Check the given range of the score for zero-on-velocity notes, which are wierd and
cannot be written safely to a MIDI file. If any are found, return the number found as
function value, plus info on where the offending notes were. */

static short AnyZeroVelNotes(Document *doc, LINK fromL, LINK toL, short *pStartStaff,
								short *pEndStaff, short *pStartMeasNum, short *pEndMeasNum);
static short AnyZeroVelNotes(Document *doc, LINK fromL, LINK toL, short *pStartStaff,
								short *pEndStaff, short *pStartMeasNum, short *pEndMeasNum)
{
	short measNum, nZeroVel;
	short startStaff, endStaff, startMeasNum, endMeasNum;
	LINK pL, aNoteL;
	PANOTE aNote;

	nZeroVel = 0;
	startMeasNum = startStaff = SHRT_MAX;
	endMeasNum = endStaff = -1;
	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:
				measNum = GetMeasNum(doc, pL);
				break;
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					if (NoteREST(aNoteL)) continue;
					/*
					 * If note has zero on velocity, it's wierd: skip it completely. If
					 * it's a chord slash, it's not interesting, else inform the user.
					 */
					aNote = GetPANOTE(aNoteL);						
					if (aNote->onVelocity==0 && NoteAPPEAR(aNoteL)!=SLASH_SHAPE) {
						nZeroVel++;
						if (measNum<startMeasNum) startMeasNum = measNum;
						if (measNum>endMeasNum) endMeasNum = measNum;
						if (NoteSTAFF(aNoteL)<startStaff) startStaff = NoteSTAFF(aNoteL);
						if (NoteSTAFF(aNoteL)>endStaff) endStaff = NoteSTAFF(aNoteL);
					}
				}
		}
	}
	
	*pStartStaff = startStaff;
	*pEndStaff = endStaff;
	*pStartMeasNum = startMeasNum;
	*pEndMeasNum = endMeasNum;
	
	return nZeroVel;
}


#ifdef DEMO_VERSION

void SaveMIDIFile(Document *doc)
{
	GetIndCString(strBuf, MIDIFILE_STRS, 9);			/* "Sorry, this demo version of Nightingale can't Export MIDI files." */
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
}

#else

#define SUFINDEX 13

static Point SFPwhere = { 106, 104 };	/* Where we want SFPutFile dialog */

void SaveMIDIFile(Document *doc)
{
	short		firstBadMeas, len, vRefNum, suffixLen, ch;
	short		totalZeroVel, startStaff, endStaff, startMeasNum, endMeasNum;
	Str255	filename/*, prompt*/;
//	SFReply	reply;
	char		strFirst[256], strLast[256], fmtStr[256];

	firstBadMeas = CheckMeasDur(doc);
	if (firstBadMeas>=0) {
		sprintf(strBuf, "%d", firstBadMeas);
		CParamText(strBuf, "", "", "");
		CautionInform(SAVEMF_TIMESIG_ALRT);
	}

	totalZeroVel = AnyZeroVelNotes(doc, doc->headL, doc->tailL, &startStaff, &endStaff,
												&startMeasNum, &endMeasNum);
	if (totalZeroVel>0) {
		Staff2UserStr(doc, startStaff, strFirst);
		Staff2UserStr(doc, endStaff, strLast);
		GetIndCString(fmtStr, MIDIFILE_STRS, 39);		/* "Skipped %d notes with On velocity of zero..." */
		sprintf(strBuf, fmtStr, totalZeroVel, startMeasNum, endMeasNum,
					strFirst, strLast);
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
	}

	/*
	 *	Create a default MIDI filename by looking up the suffix string and appending
	 *	it to the current name.  If the current name is so long that there would not
	 *	be room to append the suffix, we truncate the file name before appending the
	 *	suffix so that we don't run the risk of overwriting the original score file.
	 */
	
	GetIndString(filename,MiscStringsID,SUFINDEX);	/* Get suffix length */
	suffixLen = *filename;

	/* Get current name and its length, and truncate name to make room for suffix */
	
	if (doc->named) PStrCopy((StringPtr)doc->name,(StringPtr)filename);
	else				 GetIndString(filename,MiscStringsID,1);		/* "Untitled" */
	len = *filename;
	if (len >= (64-suffixLen)) len = (64-suffixLen);	/* 64 is max file name size */
	
	/* Finally append suffix */
	
	ch = filename[len];										/* Hold last character of name */
	GetIndString(filename+len,MiscStringsID,SUFINDEX);	/* Append suffix, obliterating last char */
	filename[len] = ch;										/* Overwrite length byte with saved char */
	*filename = (len + suffixLen);						/* And ensure new string knows new length */
	
	/* Ask user where to put this MIDI file */
#ifdef CARBON_NOMORE	
	GetIndString(prompt, MiscStringsID, 14);
	SFPutFile(SFPwhere, prompt, filename, NULL, &reply);
	if (!reply.good) return;
	PStrCopy(reply.fName, (StringPtr)filename);
	vRefNum = reply.vRefNum;
	
	errCode = FSDelete(filename, vRefNum);									/* Delete existing file */
	if (errCode!=noErr && errCode!=fnfErr)									/* Ignore "file not found" */
		{ ReportIOError(errCode, SAVEMF_ALRT); return; }
		
	errCode = Create(filename, vRefNum, creatorType, 'Midi'); 		/* Create new file */
	if (errCode!=noErr) { ReportIOError(errCode, SAVEMF_ALRT); return; }
	
	errCode = FSOpen(filename, vRefNum, &fRefNum);						/* Open it */
	if (errCode!=noErr) { ReportIOError(errCode, SAVEMF_ALRT); return; }
#else
	NSClientData	nscd;
	Boolean			keepGoing;
	FSSpec 			fsSpec;
	ScriptCode		scriptCode = smRoman;
	
	keepGoing = GetOutputName(MiscStringsID,3,filename,&vRefNum,&nscd);
	if (!keepGoing) return;
	
	fsSpec = nscd.nsFSSpec;
			
	errCode = FSpDelete(&fsSpec);												/* Delete existing file */
	if (errCode!=noErr && errCode!=fnfErr)									/* Ignore "file not found" */
		{ ReportIOError(errCode, SAVEMF_ALRT); return; }
		
	errCode = FSpCreate (&fsSpec, creatorType, 'Midi', scriptCode);
	if (errCode!=noErr) { ReportIOError(errCode, SAVEMF_ALRT); return; }
		
	errCode = FSpOpenDF (&fsSpec, fsRdWrPerm, &fRefNum);				/* Open the file */
	if (errCode!=noErr) { ReportIOError(errCode, SAVEMF_ALRT); return; }
#endif	

	WaitCursor();
	
	WriteMIDIFile(doc);
	
	ArrowCursor();
	errCode = FSClose(fRefNum);
}

#endif
