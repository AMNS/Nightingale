/* MIDIFSave.c for Nightingale - routines to write MIDI files

A lot of this is pretty closely analogous to the routines for playing scores located
in MIDIPlay.c.
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

//#include "MidiGeneral.h"

static OSErr fRefNum;						/* ID of currently open file */
static OSErr errCode;						/* Latest report from the front */

static Byte midiFileFormat;
static Word mfNTracks, mfTimeBase;
			
static MIDIEvent	eventList[MAXMFEVENTLIST];
static short		lastEvent;

static long 		qtrNTicks,				/* Ticks per quarter in Nightingale */
					trackLength,
					lenPosition;			/* File pos. of length field of current chunk */
static DoubleWord	curTime;

static Boolean	WriteChunkStart(DoubleWord, DoubleWord);
static Boolean	WriteVarLen(DoubleWord);
static Boolean	WriteDeltaTime(DoubleWord);

static void		StartNote(short, short, short, long);
static Boolean	EndNote(short, short, long);


/* ------------------------------------------------------------------- WriteChunkStart -- */

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


/* ----------------------------------------------------------------------- WriteVarLen -- */
/* Write <value> out as a MIDI file "variable-length number" and return True.

Legal variable-length numbers are non-negative and <=2^28-1. If <value> is illegal,
or if we have trouble writing it, return False. */

static Boolean WriteVarLen(DoubleWord value)
{
	long buffer, byteCount;  Byte b;
	
	if (value<0 || value>0x0FFFFFFF) return False;

	buffer = value & 0x7F;
	while ((value >>= 7) > 0) {
		buffer <<=8;						/* move byte up one place */
		buffer |= 0x80;						/* set MSB of lowest byte, next byte of delta value */
		buffer += (value & 0x7F);			/* put 7-bit value in buffer */
	}

	byteCount = 1;	
	while (True) {
		b = (Byte)buffer;					/* truncate to lowest byte */
		errCode = FSWrite(fRefNum, &byteCount, &b); 
		trackLength += byteCount;
		   
		if (buffer & 0x80)	buffer >>= 8;
		else				break;
	}
	
	return (errCode==noError);
}


/* -------------------------------------------------------------------- WriteDeltaTime -- */
/* Write the delta time corresponding to the given absolute time. */

static Boolean WriteDeltaTime(DoubleWord absTime)
{
	long dTime;
	
	dTime = absTime-curTime;
	if (dTime<0L) return False;						/* Negative delta time is illegal */
	if (WriteVarLen(dTime))
		{ curTime = absTime; return True; }
	else
		return False;
}


/* ----------------------------------------------------------------- WriteHeader, etc. -- */

static Boolean WriteHeader(Byte, Word, Word);
static Boolean WriteTrackEnd(void);
static Boolean WriteTempoEvent(unsigned long);
static Boolean WriteTSEvent(short, short, short);
static Boolean WriteTextEvent(Byte, char []);
static Boolean WriteControlChange(Byte channel, Byte ctrlNum, Byte ctrlValue);
static Boolean FillInTrackLength(void);

static Boolean WriteNote(Byte, Byte, Byte);
static Boolean WriteNoteOn(Byte, Byte, Byte);
static Boolean WriteNoteOff(Byte, Byte);

static Boolean cmFSSustainOn[MAXSTAVES + 1];
static Boolean cmFSSustainOff[MAXSTAVES + 1];
static Byte cmFSPanSetting[MAXSTAVES + 1];

static Boolean cmFSAllSustainOn[MAXSTAVES + 1];
static Byte cmFSAllPanSetting[MAXSTAVES + 1];


static Boolean WriteHeader(Byte format, Word nTracks, Word timeBase)
{
	long byteCount;
	Word data[3];
	
	if (!WriteChunkStart('MThd', 6L)) return False;

	data[0] = format;
	data[1] = nTracks;
	data[2] = timeBase;	

	byteCount = 3*sizeof(Word);
	errCode = FSWrite(fRefNum, &byteCount, data);

	return (errCode==noError);
}

static Boolean WriteTrackEnd()
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
	return (errCode==noError);
}	

static Boolean WriteTempoEvent(
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
	return (errCode==noError);
}	

static Boolean WriteTSEvent(short numerator, short denominator, short clocksPerBeat)
{
	long 	byteCount;
	Byte	logDenom, thirtySeconds;
	Byte	buffer[7];
	
	logDenom = 0;
	while (!ODD(denominator)) { logDenom++; denominator /= 2; }

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
	return (errCode==noError);
}	


static Boolean WriteTextEvent(Byte mEventType,
				char text[])				/* C string of <=255 chars. */
{
	long 			byteCount; 
	unsigned char	buffer[259];			/* Be careful: a mixture of binary and char. data! */
	short			i;
	
	/* Write meta byte, 2 bytes identifying text event subtype, text as Pascal string */
	
	buffer[0] = METAEVENT;
	buffer[1] = mEventType;
	buffer[2] = strlen(text);
	for (i = 0; i<buffer[2]; i++) {
		buffer[3+i] = (unsigned char)text[i];
	}

	byteCount = 3L+buffer[2];
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
	return (errCode==noError);
}	

static Boolean WriteControlChange(Byte channel, Byte ctrlNum, Byte ctrlValue)
{
	Byte buffer[3];
	long byteCount;
	
	buffer[0] = MCTLCHANGE+channel-CM_CHANNEL_BASE;
	buffer[1] = ctrlNum;
	buffer[2] = ctrlValue;

	byteCount = 3L;	
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
	return (errCode==noError);
}

static Boolean FillInTrackLength()
{
	long byteCount, savePosition;
	
	GetFPos(fRefNum, &savePosition);

	/* Position file pointer at track length (previously filled with dummy value) */
	
	errCode = SetFPos(fRefNum, fsFromStart, lenPosition);
	if (errCode!=noErr) return False;
	byteCount = sizeof(trackLength);
	errCode = FSWrite(fRefNum, &byteCount, &trackLength); 
	if (errCode!=noErr) return False;

	/* Move back to where we just were so as not to truncate anything */
	
	errCode = SetFPos(fRefNum, fsFromStart, savePosition);
	return (errCode==noError);
}	


static Boolean WriteNote(Byte midiEvent, Byte noteNum, Byte veloc)
{
	Byte buffer[3];
	long byteCount;
	
	buffer[0] = midiEvent;
	buffer[1] = noteNum;
	buffer[2] = veloc;

	byteCount = 3L;	
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
	return (errCode==noError);
}	

static Boolean WriteNoteOn(Byte channel, Byte noteNum, Byte velocity)
{
	return (WriteNote(MNOTEON+channel-CM_CHANNEL_BASE, noteNum, velocity));
}

/* FIXME: Instead of writing Note Offs with default velocity, it would be much better
to use the note-off velocity Nightingale has stored for each note! Should be easy,
especially since we already have off-velocity in the MIDIEvent struct. */

static Boolean WriteNoteOff(Byte channel, Byte noteNum)
{
	return (WriteNote(MNOTEOFF+channel-CM_CHANNEL_BASE, noteNum, config.noteOffVel));
}


/* --------------------------------------------------------------- Posting controllers -- */

static Boolean MFSPostMIDISustain(Document * /*doc*/, LINK pL, short staffn, Boolean susOn) 
{
	Boolean posted = False;
						
	short stf = GraphicSTAFF(pL);
	if (stf > 0 && stf == staffn) {
		if (susOn) {
			cmFSSustainOn[stf] = cmFSAllSustainOn[stf] = True;
		}		
		else {
			cmFSSustainOff[stf] = True;			
		}
		posted = True;
	}
	
	return posted;
}

static Boolean MFSPostMIDIPan(Document * /*doc*/, LINK pL, short staffn)
{
	Boolean posted = False;
	
	short stf = GraphicSTAFF(pL);
	if (stf > 0 && stf == staffn) {
		Byte panSetting = GraphicINFO(pL);
		cmFSPanSetting[stf] = cmFSAllPanSetting[stf] = panSetting;
		posted = True;
	}
	
	return posted;
}

static void MFSClearMIDISustain(Boolean susOn) 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		if (susOn) {
			cmFSSustainOn[j] = False;
		}		
		else {
			cmFSSustainOff[j] = False;			
		}
	}	
}

static void MFSClearMIDIPan() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmFSPanSetting[j] = -1;
	}
}

static void MFSClearAllMIDISustainOn() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmFSAllSustainOn[j] = False;
	}
}

static void MFSClearAllMIDIPan() 
{
	for (int j = 1; j<=MAXSTAVES; j++) {
		cmFSAllPanSetting[j] = -1;
	}
}

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

#ifdef NOMORE
static void WriteAllMIDISustains(Document *doc, Byte *partChannel, Boolean susOn, long startTime) 
{
	Byte ctrlNum = MSUSTAIN;
	Byte ctrlVal = GetSustainCtrlVal(susOn);
	
	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "  WriteAllMIDISustains: ctrlNum=%ld ctrlVal=%ld time=%ld\n",
								ctrlNum, ctrlVal, startTime);
	if (susOn) {
		for (int j = 1; j<=MAXSTAVES; j++) {
			if (cmFSSustainOn[j]) {
				short partn = Staff2Part(doc, j);
				short channel = partChannel[partn];
				WriteDeltaTime(startTime);
				if (!WriteControlChange(channel, ctrlNum, ctrlVal)) {
					MayErrMsg("Unable to write Sustain On to MIDI file.  (WriteAllMIDISustains)");
					return;
				}
			}
		}
	}
	else {
		for (int j = 1; j<=MAXSTAVES; j++) {
			if (cmFSSustainOff[j]) {
				short partn = Staff2Part(doc, j);
				short channel = partChannel[partn];
				WriteDeltaTime(startTime);
				if (!WriteControlChange(channel, ctrlNum, ctrlVal)) {
					MayErrMsg("Unable to write Sustain Off to MIDI file.  (WriteAllMIDISustains)");
					return;
				}
			}
		}		
	}
}
#endif

static void WriteMIDISustains(Document *doc, Byte *partChannel, Boolean susOn,
								long startTime, LINK pL, short stf) 
{
	LINK graphicL = LSSearch(pL, GRAPHICtype, stf, GO_LEFT, False);
	
	while (graphicL != NILINK && GraphicFIRSTOBJ(graphicL) == pL) {
		if (susOn == True && IsPedalDown(graphicL) ||
			 susOn == False && isPedalUp(graphicL)) {
			Byte ctrlNum = MSUSTAIN;
			Byte ctrlVal = GetSustainCtrlVal(susOn);
			
			if (DETAIL_SHOW) LogPrintf(LOG_INFO, "  WriteMIDISustains: ctrlNum=%ld ctrlVal=%ld time=%ld\n",
										ctrlNum, ctrlVal, startTime);
			
			short partn = Staff2Part(doc,stf);
			short channel = partChannel[partn];
			WriteDeltaTime(startTime);
			if (!WriteControlChange(channel, ctrlNum, ctrlVal)) {
				MayErrMsg("Unable to write Sustains to MIDI file.  (WriteMIDISustains)");
				return;
			}
		}
		
		graphicL = LSSearch(LeftLINK(graphicL), GRAPHICtype, stf, GO_LEFT, False);
	}
}

static void WriteAllMIDISustains(Document *doc, Byte *partChannel, Boolean susOn,
									long startTime, LINK pL, short stf) 
{	
	LINK aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteSTAFF(aNoteL) == stf)
			WriteMIDISustains(doc, partChannel, susOn, startTime, pL, NoteSTAFF(aNoteL));
	}
}

// TODO: from MIDIPlay.c

static Boolean ValidPanSetting(Byte panSetting) 
{
	SignedByte sbpanSetting = (SignedByte)panSetting;
	
	return sbpanSetting >= 0;
}

#ifdef NOMORE
static void WriteAllMIDIPans(Document *doc, Byte *partChannel, long startTime) 
{	
	Byte ctrlNum = MPAN;

	for (int j = 1; j<=MAXSTAVES; j++) {
		Byte ctrlVal = cmFSPanSetting[j];
		if (ValidPanSetting(ctrlVal)) {
			short partn = Staff2Part(doc,j);
			short channel = partChannel[partn];
			WriteDeltaTime(startTime);
//			WriteDeltaTime(0);
			if (!WriteControlChange(channel, ctrlNum, ctrlVal)) {
				MayErrMsg("Unable to write all pans to MIDI file.  (WriteAllMIDIPans)");
				return;
			}
		}
	}
}

static void WriteMIDIPans(Document *doc, Byte *partChannel, long startTime, short stf)
{	
	Byte ctrlNum = MPAN;

	Byte ctrlVal = cmFSPanSetting[stf];
	if (ValidPanSetting(ctrlVal)) {
		short partn = Staff2Part(doc,stf);
		short channel = partChannel[partn];
		WriteDeltaTime(startTime);
		WriteControlChange(channel, ctrlNum, ctrlVal);
		SignedByte sbpanSetting = -1;
		cmFSPanSetting[stf] = (Byte)sbpanSetting;
	}
}
#endif

static void WriteMIDIPans(Document *doc, Byte *partChannel, long startTime, LINK pL, short stf)
{	
	LINK graphicL = LSSearch(pL, GRAPHICtype, stf, GO_LEFT, False);
	while (graphicL != NILINK && GraphicFIRSTOBJ(graphicL) == pL) {
		if (IsMIDIPan(graphicL)) {
			Byte ctrlNum = MPAN;
			Byte ctrlVal = GraphicINFO(graphicL);
			
			LogPrintf(LOG_INFO, "  ctrlNum=%ld ctrlVal=%ld time=%ld  (WriteMIDIPans)\n",
						ctrlNum, ctrlVal, startTime);
			
			short partn = Staff2Part(doc,stf);
			short channel = partChannel[partn];
			WriteDeltaTime(startTime);
			WriteControlChange(channel, ctrlNum, ctrlVal);					
		}
		
		graphicL = LSSearch(LeftLINK(graphicL), GRAPHICtype, stf, GO_LEFT, False);
	}
}

static void WriteAllMIDIPans(Document *doc, Byte *partChannel, long startTime, LINK pL, short stf)
{	
	LINK aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteSTAFF(aNoteL) == stf)
			WriteMIDIPans(doc, partChannel, startTime, pL, NoteSTAFF(aNoteL));
	}
}

/* -------------------------------------------------------------- Event List functions -- */

static Boolean	MFInsertEvent(char, char, long);
static void		MFCheckEventList(long);

/*	Insert the specified note into the event list. Return True normally, False in case
of trouble. */

static Boolean MFInsertEvent(
					char note,
					char channel,		/* 1 to MAXCHANNEL */
					long endTime
					)
{
	short		i;
	MIDIEvent	*pEvent;
	char		fmtStr[256];

	/* Find first free slot in list, which may be at lastEvent (end of list) */
	
	for (i=0, pEvent=eventList; i<lastEvent; i++,pEvent++)
		if (pEvent->note == 0) break;
	
	/* Insert note into free slot, or append to end of list if there's room */
	
	if (i<lastEvent || lastEvent++<MAXMFEVENTLIST) {
		pEvent->note = note;
		pEvent->channel = channel;
		pEvent->endTime = endTime;
		return True;
	}
	else {
		lastEvent--;			
		GetIndCString(fmtStr, MIDIFILE_STRS, 33);	/* "Nightingale can't save MIDI files with more than %d notes playing at once." */
		sprintf(strBuf, fmtStr, MAXMFEVENTLIST); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
}


/*	Checks eventList[] to see if any notes are ready to be turned off; if so, frees
their slots in the eventList and writes out Note Offs. */

static void MFCheckEventList(long time)
{
	MIDIEvent	*pEvent;
	short		i;
	
	for (i=0, pEvent = eventList; i<lastEvent; i++, pEvent++)
		if (pEvent->note) {
			if (pEvent->endTime<=time) {						/* note is done */
					WriteDeltaTime(time);
					if (!WriteNoteOff(pEvent->channel, pEvent->note)) {
						MayErrMsg("Unable to write Note Off to MIDI file.  (MFCheckEventList)");
						return;
					}
				pEvent->note = 0;								/* slot available now */
			}
		}
}


/* ---------------------------------------------------------------- StartNote, EndNote -- */

static void StartNote(
			short	noteNum,
			short	channel,			/* 1 to MAXCHANNEL */
			short	velocity,
			long	time
			)
{
	if (noteNum>=0) {
		WriteDeltaTime(time);
		if (!WriteNoteOn(channel, noteNum, velocity)) {
				MayErrMsg("Unable to write Note On to MIDI file.");
				return;
		}
	}
}

static Boolean EndNote(
			short	noteNum,
			short	channel,			/* 1 to MAXCHANNEL */
			long	endTime
			)
{
	return MFInsertEvent(noteNum, channel, endTime);
}


/* ------------------------------------------------------------ WriteTrack and helpers -- */

static Boolean WriteTSig(LINK, LINK, long);
static Boolean WriteTimingTrack(Document *, long);
static Boolean WriteTrackName(Document *, short);
static Boolean WriteTrackPatch(Document *doc, short staffn);
static short WriteMFNotes(Document *, short, LINK, LINK, long);
static Boolean WriteTrack(Document *, short, long, short *);

static Boolean WriteTSig(
					LINK /*timeSigL*/,		/* Time signature object (unused) */
					LINK aTSL,				/* Time sig. subobject */
					long absTime
					)
{
	short numer, denom, tempDenom, clocksPerBeat;

	numer = TimeSigNUMER(aTSL);
	denom = TimeSigDENOM(aTSL);
	if (denom<=0) {
		MayErrMsg("Time signature L%ld denom %ld is illegal.  (WriteTSig)", (long)aTSL,
					(long)denom);
		return False;
	}

	tempDenom = denom;
	clocksPerBeat = 24*4;							/* 24 is standard/qtr, and 4 qtrs/whole */
	while (!ODD(tempDenom)) {
		clocksPerBeat /= 2; tempDenom /= 2;
	}
	if (COMPOUND(numer)) clocksPerBeat *= 3;

	WriteDeltaTime(absTime);
	if (!WriteTSEvent(numer, denom, clocksPerBeat)) {
		MayErrMsg("Unable to write time signature L%ld.  (WriteTSig)", (long)denom);
		return False;
	}

	return True;
}


/* Write out all timing information, presumably for the timing track. */

static Boolean WriteTimingTrack(
					Document *doc,
					long trkLastEndTime		/* The latest ending time for any track */
					)
{
	long	prevTSTime=-1L;
	LINK	pL, aTSL, syncL, syncMeasL;
	long	measureTime,
			microbeats,							/* microsec. units per PDUR tick */
			timeScale,							/* PDUR ticks per minute */
			tempoTime, prevTempoTime;
	short 	measNum;

	LogPrintf(LOG_NOTICE, "trkLastEndTime=%ld  (WriteTimingTrack)\n", trkLastEndTime);
	measureTime = 0L;
	prevTempoTime = -1L;
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
if (MORE_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "  WriteTimingTrack: pL=%u \n", pL);
		switch (ObjLType(pL)) {
			case MEASUREtype:
				measureTime = MeasureTIME(pL);
				break;
			case TEMPOtype:
				if (TempoNOMM(pL)) break;			/* Ignore Tempo objects with no M.M. */
				
//LogPrintf(LOG_DEBUG, "  WriteTimingTrack 1: TEMPO pL=%u \n", pL);
				/* Calculate its effective time and write initial tempo or tempo change.
				   If there are no notes following -- very unlikely, but possible --
				   give up. We could probably continue, but it's not worth the trouble.  */
					
				syncL = SSearch(pL, SYNCtype, GO_RIGHT);
				if (syncL) syncMeasL = SSearch(syncL, MEASUREtype, GO_LEFT);
				if (!syncL || !syncMeasL) {
					MayErrMsg("No Sync found after TEMPO L%ld.  (WriteTimingTrack).", (long)pL);
					return False;
				}
				tempoTime = MeasureTIME(syncMeasL)+SyncTIME(syncL);
				if (tempoTime==prevTempoTime) {
					measNum = GetMeasNum(doc, pL);
					LogPrintf(LOG_WARNING, "Tempo change L%u in measure %d at same time as a previous tempo change.  (WriteTimingTrack)\n",
						pL, measNum);
					}
//LogPrintf(LOG_DEBUG, "  WriteTimingTrack 2: TEMPO pL=%u \n", pL);
				WriteDeltaTime(tempoTime);
				timeScale = GetTempoMM(doc, pL);
				microbeats = TSCALE2MICROBEATS(timeScale);
				if (!WriteTempoEvent((long)microbeats*DFLT_BEATDUR)) {
					MayErrMsg("Unable to write a tempo event to the MIDI file.  (WriteTimingTrack)");
					return False;
				}
				if (DETAIL_SHOW) LogPrintf(LOG_INFO, "  WriteTimingTrack: TEMPO pL=%d tempoTime=%ld timeScale=%ld\n",
									pL, tempoTime, timeScale);
				prevTempoTime = tempoTime;
				break;
			case TIMESIGtype:
				/* In view of Nightingale's insistence that all barlines align, for now
					we'll just get get the time sig. from staff 1 and ignore other staves.
					If there's an obvious problem, we should have warned the user (cf.
					CheckMeasDur()). Also, if we've already written a time sig. at this
					time, skip this one: it's probably a time sig. beginning a system with
					an identical cautionary one at the end of the last system. */
					
				aTSL = TimeSigOnStaff(pL, 1);
//LogPrintf(LOG_DEBUG, "  WriteTimingTrack: TIMESIG pL=%u aTSL=%d measureTime=%ld %c\n", pL, aTSL,
//measureTime, (measureTime!=prevTSTime? ' ' : 'S'));
				if (aTSL && measureTime!=prevTSTime) {
					WriteTSig(pL, aTSL, measureTime);	/* Assume time sig. is at beginning of measure! */
					prevTSTime = measureTime;
				}
				break;
			default:
				;
		}
	}
	
	WriteDeltaTime(trkLastEndTime);
	if (!WriteTrackEnd()) {
		MayErrMsg("Unable to complete MIDI file timing track.  (WriteTimingTrack)");
		return False;
	}
	
	return True;
}


static Boolean WriteTrackName(Document *doc, short staffn)
{
	LINK aPartL;  char str[256];

	aPartL = Staff2PartL(doc, doc->headL, staffn);
	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "  WriteTrackName: staff=%d name='%s'\n",
								staffn, PartNAME(aPartL));

	strcpy(str, PartNAME(aPartL));
	WriteDeltaTime(0L);
	if (!WriteTextEvent(ME_SEQTRACKNAME, str)) {
		MayErrMsg("Unable to write track name to MIDI file.  (WriteTrackName)");
		return False;
	}
	
	return True;
}

static Boolean WriteTrackPatch(Document *doc, short staffn)
{
	LINK			partL;
	PARTINFO		aPart;
	static Byte		partPatch[MAXSTAVES];
	static Byte		partChannel[MAXSTAVES];
	static Boolean	firstCall=True;

	Byte buffer[2];
	long byteCount;
	
	if (firstCall) {
		partL = FirstSubLINK(doc->headL);
		for (int i = 0; i<=LinkNENTRIES(doc->headL)-1; i++, partL = NextPARTINFOL(partL)) {
			aPart = GetPARTINFO(partL);
			partPatch[i] = aPart.patchNum;
			partChannel[i] = UseMIDIChannel(doc, i);
		}
		firstCall = False;
	}
	
	short partn = Staff2Part(doc, staffn);
	short patch = partPatch[partn];
	short channel = partChannel[partn];
	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "  WriteTrackPatch: staff=%d patch=%d channel=%d\n",
								staffn, patch, channel);
	
	WriteDeltaTime(0L);

	buffer[0] = MPGMCHANGE+channel-CM_CHANNEL_BASE;
	buffer[1] = patch;
	byteCount = 2L;	
	errCode = FSWrite(fRefNum, &byteCount, buffer); 
	trackLength += byteCount;
	if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "  WriteTrackPatch: staffn=%d buffer[]=%d %d errCode=%d\n",
								staffn, buffer[0], buffer[1], errCode);
	return (errCode==noError);
}	


#define USEPARTVELO False				/* Use parts' balance velocities? */

/* In the given range on <staffn> or all staves, write to the current track of the
open MIDI file all notes with nonzero On velocity that aren't in a muted part. Similar
to PlaySequence, except PlaySequence plays either selected notes only or all notes in
unmuted parts on all staves. Returns the number of notes skipped because they have zero
On velocity.

NB: if the measures/notes have inconsistent timestamps (as detected by DCheckNode),
this writes nonsense Note Ons -- I have no idea why -- resulting in tracks that our own
Import MIDI File says are "damaged or incomplete"! */

static short WriteMFNotes(
					Document	*doc,
					short		staffn,				/* Staff no. or ANYONE */
					LINK		fromL, LINK toL,	/* range to be written */
					long		trkLastEndTime		/* The latest ending time for any track */
					)
{
	LINK		pL, aNoteL;
	LINK		partL, measL, newMeasL;
	short		i, nZeroVel;
	short		useNoteNum,
				useChan, useVelo, partn;
	long		t,
				toffset,							/* PDUR start time of 1st note played */
				playDur,
				plStartTime, plEndTime,				/* in PDUR ticks */
				startTime, endTime;
	PARTINFO	aPart;
	char		partVelo[MAXSTAVES];
	Byte		partChannel[MAXSTAVES];
	short		partTransp[MAXSTAVES];
	Boolean		anyStaff;
	
	Boolean		sustainOnPosted = False;
	Boolean		sustainOffPosted = False;
	Boolean		panPosted = False;
		
	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "  WriteMFNotes: staff=%d trkLastEndTime=%d\n", \
								staffn, trkLastEndTime);

	anyStaff = (staffn==ANYONE);

	partL = FirstSubLINK(doc->headL);
	for (i = 0; i<=LinkNENTRIES(doc->headL)-1; i++, partL = NextPARTINFOL(partL)) {
		aPart = GetPARTINFO(partL);
		partVelo[i] = aPart.partVelocity;
		partChannel[i] = UseMIDIChannel(doc, i);
		partTransp[i] = aPart.transpose;
	}

	MFSClearMIDISustain(True);
	MFSClearMIDISustain(False);
	MFSClearMIDIPan();
	
	MFSClearAllMIDISustainOn();
	MFSClearAllMIDIPan();

	lastEvent = 0;												/* start with empty Event list */

	newMeasL = measL = SSearch(fromL, MEASUREtype, GO_LEFT);	/* starting measure */
	
	t = 0L;
	nZeroVel = 0;
 
	/* The stored play time of the first Sync we're going to write might be any non-
	   negative value, but we want written times to start at 0, so we pick up the first
	   the Sync's play time and use it as an offset on all play times. */
	   
	toffset = -1L;												/* Init. time offset to unknown */
	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:
				newMeasL = measL = pL;
				break;
			case SYNCtype:
			  	if (!AnyNoteToPlay(doc, pL, False)) continue;
		  		if (SyncTIME(pL)>MAX_SAFE_MEASDUR)
		  			MayErrMsg("pL=%ld has suspicious timeStamp %ld  (WriteMFNotes)", (long)pL,
		  							(long)SyncTIME(pL));
#ifdef TDEBUG
				if (toffset<0L) LogPrintf(LOG_DEBUG, "  toffset=%ld => %ld\n", toffset, MeasureTIME(measL)+SyncTIME(pL));
#endif
		  		plStartTime = MeasureTIME(measL)+SyncTIME(pL);
		  		startTime = plStartTime;
		  		if (toffset<0L) toffset = startTime;
				startTime -= toffset;
	
				/* Move time forward 'til it's time to "play" (write) pL. While doing so,
				   check for notes ending before _or at_ pL's time and take care of them.
				   We have to write notes ending at a given time before those beginning
				   at the same time because otherwise--if there's a new note of the same
				   note number--we'll end up making a note start, then end instantly. */
				   
				for ( ; t<=startTime; t++) {
					if (UserInterrupt()) goto Done;		/* Check for Cancel */
					MFCheckEventList(t);				/* Check for and turn off any notes that are done */
				}

				if (newMeasL) {
					newMeasL = NILINK;
				}
	
//				if (patchChangePosted) {
//					CMMIDIProgram(doc, partPatch, partChannel);
//					patchChangePosted = False;
//				}
//				if (sustainOnPosted) {
//					WriteAllMIDISustains(doc, partChannel, True, startTime);
//					MFSClearMIDISustain(True);
//					sustainOnPosted = False;
//				}
//				if (sustainOffPosted) {
//					WriteAllMIDISustains(doc, partChannel, False, startTime);
//					MFSClearMIDISustain(False);
//					sustainOffPosted = False;
//				}
//				if (panPosted) {
//					WriteAllMIDIPans(doc, partChannel, startTime);
//					MFSClearMIDIPan();
//					panPosted = False;
//				}

				WriteAllMIDISustains(doc, partChannel, True, startTime, pL, staffn);
				WriteAllMIDISustains(doc, partChannel, False, startTime, pL, staffn);
				WriteAllMIDIPans(doc, partChannel, startTime, pL, staffn);
				
				/* Write all the notes in <<pL> we're supposed to, adding them to
				   <eventList[]> as well. */
	
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					if (anyStaff || NoteSTAFF(aNoteL)==staffn) {
						if (!NoteToBePlayed(doc, aNoteL, False)) continue;
						
						/* If note has zero on velocity, skip writing it. If such a
						   note is a chord slash, it's not interesting, else count it. */
						   
						if (NoteONVELOCITY(aNoteL)==0) {
							if (NoteAPPEAR(aNoteL)!=SLASH_SHAPE) nZeroVel++;
							continue;
						}

						partn = Staff2Part(doc, NoteSTAFF(aNoteL));
						
						/* Get note's MIDI note number, including transposition; velocity,
						   limited to legal range; channel number; and duration, includ-
						   ing any tied notes to right. */
						   
						useNoteNum = UseMIDINoteNum(doc, aNoteL, partTransp[partn]);
						useChan = partChannel[partn];
						useVelo = doc->velOffset+NoteONVELOCITY(aNoteL);
						if (USEPARTVELO) useVelo += partVelo[partn];
						if (useVelo<1) useVelo = 1;
						if (useVelo>MAX_VELOCITY) useVelo = MAX_VELOCITY;
if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "  WriteMFNotes: pL=%u aNoteL=%u velOffset=%d ONVELOCITY=%d useVelo=%d\n",
pL, aNoteL, doc->velOffset, NoteONVELOCITY(aNoteL), useVelo);
						
						playDur = TiedDur(doc, pL, aNoteL, False);
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
				if (IsMIDIController(pL)) {
#if 0
					Byte ctrlNum = GetMIDIControlNum(pL);
					Byte ctrlVal = GetMIDIControlVal(pL);
					short stf = GraphicSTAFF(pL);
					//if (anyStaff || stf == staffn) {
					if (stf==staffn) {
						short partn = Staff2Part(doc, stf);
						//short channel = CMGetUseChannel(partChannel, partn);
						short channel = partChannel[partn];

						WriteDeltaTime(startTime);
						WriteControlChange(channel, ctrlNum, ctrlVal);						
					}
#else
					if (useWhichMIDI==MIDIDR_CM) {
									
	//					if (IsMidiPatchChange(pL)) {
	//						patchChangePosted = MFSPostMIDIProgramChange(doc, pL, partPatch, partChannel);						
	//					}
	//					else 
						
						if (IsPedalDown(pL)) {
							sustainOnPosted = MFSPostMIDISustain(doc, pL, staffn, True);
						}
						else if (isPedalUp(pL)) {
							sustainOffPosted = MFSPostMIDISustain(doc, pL, staffn, False);
						}
						else if (IsMIDIPan(pL)) {
							panPosted = MFSPostMIDIPan(doc, pL, staffn);
						}					
					}
#endif
				}
				break;
								
			default:
				;
		}
	}
			
	for ( ; t<=trkLastEndTime; t++) {
		if (UserInterrupt()) goto Done;				/* Check for Cancel */
		MFCheckEventList(t);						/* Check for and turn off any notes that are done */
	}

Done:	
	WriteDeltaTime(t);
	WriteTrackEnd();
	
	return nZeroVel;
}


/* Write one track of a MIDI file, either the timing track or a normal track. Each
track corresponds to a staff; see WriteMIDIFile(). */

static Boolean WriteTrack(
				Document *doc,
				short trackn,
				long trkLastEndTime,			/* The latest ending time for any track */
				short *pnZeroVel
				)
{
	LINK aPartL;
	short staffn, nZeroVel, partPatch, channel;
	PARTINFO aPart;
	
	trackLength = 0L;
	curTime = 0L;

	WriteChunkStart('MTrk', 0L);					/* Dummy tracklength: fill in later */

	if (trackn==1)
		WriteTimingTrack(doc, trkLastEndTime);
	else {
		staffn = trackn-1;
		WriteTrackName(doc, staffn);
		WriteTrackPatch(doc, staffn);
		nZeroVel = WriteMFNotes(doc, staffn, doc->headL, doc->tailL, trkLastEndTime);
		aPartL = Staff2PartL(doc, doc->headL, staffn);
		aPart = GetPARTINFO(aPartL);
		partPatch = aPart.patchNum;
		channel = aPart.channel;
		LogPrintf(LOG_NOTICE, "Wrote track for staff %d ('%s') patch=%d, channel=%d, trkLastEndTime=%d",
			staffn, PartNAME(aPartL), partPatch, channel, trkLastEndTime);
		if (nZeroVel>0) LogPrintf(LOG_NOTICE, ", %d zero-vel. note(s).", nZeroVel);
		LogPrintf(LOG_NOTICE, "  (WriteTrack)\n");
	}

	FillInTrackLength();
	
	if (errCode!=noErr) ReportIOError(errCode, SAVEMF_ALRT);
	
	*pnZeroVel = nZeroVel;
	return (errCode==noErr);
}


long LastEndTime(Document *doc, LINK fromL, LINK toL)
{
	LINK 	measL, pL, aNoteL;
	long	lastET, endTime, playDur,
			toffset,								/* PDUR start time of 1st note played */
			plStartTime, plEndTime;					/* in PDUR ticks */
	
	lastET = 0L;

	measL = SSearch(fromL, MEASUREtype, GO_LEFT);				/* starting measure */
 
	/* The stored play time of the first Sync we're going to write might be any positive
	   value but we want written times to start at 0, so we'll pick up the first Sync's
	   play time and use it as an offset on all play times. */
	   
	toffset = -1L;												/* Init. time offset to unknown */
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
									
					/* If it's a "real" note (not rest or continuation or vel. 0), consider it */
					
					if (!NoteREST(aNoteL) && !NoteTIEDL(aNoteL) && NoteONVELOCITY(aNoteL)!=0) {
						playDur = TiedDur(doc, pL, aNoteL, False);
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

static Boolean WriteMIDIFile(Document *);
static short CheckMeasDur(Document *);
static short CountZeroVelNotes(Document *doc, LINK fromL, LINK toL, short *pStartStaff,
								short *pEndStaff, short *pStartMeasNum, short *pEndMeasNum);

static Boolean WriteMIDIFile(Document *doc)
{
	short t, nZeroVel;
	long trkLastEndTime;							/* The latest ending time for any track */

	/* Write the MIDI file header, then the tracks, one for timing info plus one for
	   each staff of the score. */
	
	midiFileFormat = 1;
	mfNTracks = doc->nstaves+1;
	qtrNTicks = Code2LDur(QTR_L_DUR, 0);
	mfTimeBase = qtrNTicks;

	if (!WriteHeader(midiFileFormat, mfNTracks, mfTimeBase)) {
		MayErrMsg("Unable to write MIDI file header.  (WriteMIDIFile)");
		return False;
	}
	
	trkLastEndTime = LastEndTime(doc, doc->headL, doc->tailL);
	
	for (t = 1; t<=mfNTracks; t++)		
		if (!WriteTrack(doc, t, trkLastEndTime, &nZeroVel)) return False;
	
	return True;
}


/* Check the actual duration of notes in every measure, considered as a whole, against
its time signature on every staff; if any irregularities are found, return the measure
number of the first, else return -1. Identical (as of Sept. 2016) to DCheckMeasDur,
except this returns a measure number and doesn't give error messages if it finds problems.
It's also less strict: it ignores completely empty measures, and it doesn't check the
very last measure. */

static short CheckMeasDur(Document *doc)
{
	LINK pL, barTermL;
	long measDurFromTS, measDurActual;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (MeasureTYPE(pL) && !IsFakeMeasure(doc, pL)) {
			barTermL = EndMeasSearch(doc, pL);
			if (barTermL && barTermL!=doc->tailL) {
				measDurFromTS = GetTimeSigMeasDur(doc, barTermL);
				if (measDurFromTS<0) return False;
				measDurActual = GetMeasDur(doc, barTermL, ANYONE);
				if (measDurActual!=0 && measDurFromTS!=measDurActual)
					return GetPAMEASURE(FirstSubLINK(pL))->measureNum+doc->firstMNNumber;
			}
		}
	}
		
	return -1;
}


/* Check the given range of the score for zero-on-velocity notes, which are wierd and
cannot be written safely to a MIDI file. If any are found, return the number found as
function value, plus info on where the offending notes were. */

static short CountZeroVelNotes(Document *doc, LINK fromL, LINK toL, short *pStartStaff,
								short *pEndStaff, short *pStartMeasNum, short *pEndMeasNum)
{
	short measNum=-1, nZeroVel;
	short startStaff, endStaff, startMeasNum, endMeasNum;
	LINK pL, aNoteL;

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
					
					/* If a zero on-velocity note is a chord slash, it's not really
					   interesting; else count it. */
					   
					if (NoteONVELOCITY(aNoteL)==0 && NoteAPPEAR(aNoteL)!=SLASH_SHAPE) {
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


#define MF_SUFINDEX 13

Boolean SaveMIDIFile(Document *doc)
{
	short	firstProblemMeas, len, vRefNum, suffixLen, ch;
	short	totalZeroVel, startStaff, endStaff, startMeasNum, endMeasNum;
	Str255	filename;
	char	strFirst[256], strLast[256], fmtStr[256];

	firstProblemMeas = CheckMeasDur(doc);
	if (firstProblemMeas>=0) {
		sprintf(strBuf, "%d", firstProblemMeas);
		CParamText(strBuf, "", "", "");
		CautionInform(SAVEMF_TIMESIG_ALRT);			/* "Score contains measure(s) that have time sig. disagreements..." */
	}

	/* Do some error checking. NB that passing these checks does not guarantee that
		anything useful will be written to the MIDI file; it could be that no part,
		or no part other than a muted one, contains any notes of nonzero velocity.
	 */
	if (doc->mutedPartNum!=0) {
		if (LinkNENTRIES(doc->headL)<=2) {
			GetIndCString(strBuf, MIDIFILE_STRS, 43);		/* "The only part is muted, so no notes..." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return False;
		}
		else {
			GetIndCString(strBuf, MIDIFILE_STRS, 42);		/* "A part is muted; its notes..." */
			CParamText(strBuf, "", "", "");
			CautionInform(GENERIC_ALRT);
		}
	}

	totalZeroVel = CountZeroVelNotes(doc, doc->headL, doc->tailL, &startStaff, &endStaff,
											&startMeasNum, &endMeasNum);
	if (totalZeroVel>0) {
		Staff2UserStr(doc, startStaff, strFirst);
		Staff2UserStr(doc, endStaff, strLast);
		GetIndCString(fmtStr, MIDIFILE_STRS, 39);		/* "%d note(s) with On velocity of zero..." */
		sprintf(strBuf, fmtStr, totalZeroVel, startMeasNum, endMeasNum, strFirst,
					strLast);
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
	}

	/* Create a default MIDI filename by looking up the suffix string and appending
	   it to the current name.  If the current name is so long that there would not
	   be room to append the suffix, we truncate the file name before appending the
	   suffix so that we don't run the risk of overwriting the original score file. */
	   
	GetIndString(filename, MiscStringsID, MF_SUFINDEX);		/* Get suffix length */
	suffixLen = *filename;

	/* Get current name and its length, and truncate name to make room for suffix */
	
	if (doc->named)	Pstrcpy((StringPtr)filename, (StringPtr)doc->name);
	else			GetIndString(filename, MiscStringsID, 1);		/* "Untitled" */
	len = *filename;
		
	if (len >= (FILENAME_MAXLEN-suffixLen)) len = (FILENAME_MAXLEN-suffixLen);
	
	/* Finally append suffix */
	
	ch = filename[len];										/* Hold last character of name */
	GetIndString(filename+len, MiscStringsID, MF_SUFINDEX);	/* Append suffix, obliterating last char */
	filename[len] = ch;										/* Overwrite length byte with saved char */
	*filename = (len + suffixLen);							/* And ensure new string knows new length */
	
	/* Ask user where to put this MIDI file */
	
	NSClientData	nscd;
	Boolean			keepGoing;
	FSSpec 			fsSpec;
	ScriptCode		scriptCode = smRoman;
	
	keepGoing = GetOutputName(MiscStringsID, 3, filename, &vRefNum, &nscd);
	if (!keepGoing) return False;
	
	fsSpec = nscd.nsFSSpec;
			
	errCode = FSpDelete(&fsSpec);										/* Delete existing file */
	if (errCode!=noErr && errCode!=fnfErr)								/* Ignore "file not found" */
		{ ReportIOError(errCode, SAVEMF_ALRT); return False; }
		
	errCode = FSpCreate(&fsSpec, creatorType, 'Midi', scriptCode);
	if (errCode!=noErr) { ReportIOError(errCode, SAVEMF_ALRT); return False; }
		
	errCode = FSpOpenDF(&fsSpec, fsRdWrPerm, &fRefNum);					/* Open the new file */
	if (errCode!=noErr) { ReportIOError(errCode, SAVEMF_ALRT); return False; }

	WaitCursor();
	
	if (!WriteMIDIFile(doc)) return False;
	
	ArrowCursor();
	errCode = FSClose(fRefNum);
	return True;
}
