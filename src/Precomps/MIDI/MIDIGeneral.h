/* MIDIGeneral.h for Nightingale - general-purpose MIDI constants and prototypes */

/* MIDI commands (status bytes and high-order nybbles of status bytes) */

#define MPAN 0x0A
#define MSUSTAIN 0x40
#define MNOTEOFF 0x80
#define MNOTEON 0x90
#define MPOLYPRES 0xA0
#define MCTLCHANGE 0xB0
#define MPGMCHANGE 0xC0
#define MCHANPRES 0xD0
#define MPITCHBEND 0xE0
#define MSYSEX 0xF0
#define MEOX 0xF7					/* End of System Exclusive data */
#define MLOW_SYSREALTIME 0xF8		/* Lowest code for System Real Time command */
#define MHI_SYSREALTIME 0xFF		/* Highest code for System Real Time command */
#define MACTIVESENSE 0xFE

#define MSTATUSMASK 0x80
#define MCOMMANDMASK 0xF0
#define MCHANNELMASK 0x0F

#define COMMAND(mByte)	((mByte)>=MSYSEX? (mByte) : (mByte) & MCOMMANDMASK)
#define CHANNEL(mByte)	((mByte) & MCHANNELMASK)

/* MIDI limits and parameters */

#define MAXCHANNEL 16
#define MAXPATCHNUM 128
#define MAX_VELOCITY 127
#define MIDI_MIDDLE_C 60			/* MIDI note number for middle C (ISO C4) */
#define MAX_NOTENUM 127
#define NO_PATCHNUM 0xFF			/* FMS: Means no valid patch number current for this device & channel */
#define NO_BANKSELECT 0xFF			/* Don't send this bank select value */

/* Constants and utilities for use with Apple MIDI Manager */

#define MM_HDR_SIZE 6
#define MM_STD_FLAGS	0
#define MM_NOTE_SIZE	MM_HDR_SIZE+3
#define MM_NOW 0

/* Constants used in MIDI Pascal 3.0: unfortunately, not in their header file! */

#define IFSPEEDP5MHZ 0
#define IFSPEED1MHZ 1
#define IFSPEED2MHZ 2
#define IFSPEED_FAST 5

#define MODEM_PORT 3
#define PRINTER_PORT 4

/* Constant for use with built-in MIDI (currently MIDI Pascal 3.0) */

#define BIMIDI_SMALLBUFSIZE 30		/* Our standard buffer size for built-in MIDI */


/* Constants for Apple's Core MIDI, from CoreMidiUtils.c. (But now that MIDI is over 30
years old, are these values _still_ not standardized?  --DAB, Sept. 2016) */

//#define CM_PATCHNUM_BASE 1			/* Some synths start numbering at 1, some at 0 */
//#define CM_CHANNEL_BASE 0

// 0-based uses BASE of 1

#define CM_PATCHNUM_BASE 1			/* Some synths start numbering at 1, some at 0 */
#define CM_CHANNEL_BASE 1	

/* Constant and medium-level functions for use with "any" built-in MIDI driver */

#define ONEMILLISEC 782				/* Timer constant for 1 ms. interrupts */

/* Datatypes and public prototypes */

typedef struct						/* Time conversion item: */
{
	long 		pDurTime;			/* time until this tempo begins (PDUR ticks) */
	long		microbeats;			/* tempo in microsec. units per PDUR tick */
	long		realTime;			/* time until this tempo begins (millisec.) */
} TCONVERT;

typedef struct myEvent				/* MIDI event list item: */
{
	SignedByte	note;				/* MIDI note number, in range [0..MAX_NOTENUM]; 0=slot open */
	SignedByte	channel;			/* MIDI channel number of note, in range [1..MAXCHANNEL] */
	long		endTime;			/* Ending time of note, in milliseconds */
	SignedByte	offVel;				/* Note Off velocity */
	short		omsIORefNum;		/* Used by OMS to identify device to receive packet */
} MIDIEvent;

typedef struct myCMEvent			/* MIDI event list item: */
{
	SignedByte	note;				/* MIDI note number, in range [0..MAX_NOTENUM]; 0=slot open */
	SignedByte	channel;			/* MIDI channel number of note, in range [1..MAXCHANNEL] */
	long		endTime;			/* Ending time of note, in milliseconds */
	SignedByte	offVel;				/* Note Off velocity */
	long		cmIORefNum;			/* Used by CoreMIDI to identify device to receive packet */
} CMMIDIEvent;

/* Low- and medium-level MIDI utility routines */

void StopMIDI(void);

void GetPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
							Byte channelPatch[], SignedByte partVelo[]);
void GetNotePlayInfo(Document *doc, LINK aNoteL, short partTransp[],
							Byte partChannel[], SignedByte partVelo[],
							short *pUseNoteNum, short *pUseChan, short *pUseVelo);

OSStatus StartNoteNow(short noteNum, SignedByte channel, SignedByte velocity, short ioRefNum);
OSStatus EndNoteNow(short noteNum, SignedByte channel, short ioRefNum);
Boolean EndNoteLater(short noteNum, SignedByte channel, long endTime, short ioRefNum);
Boolean CMEndNoteLater(short noteNum, SignedByte channel, long endTime, long ioRefNum);

Boolean AllocMPacketBuffer(void);
Boolean MMInit(long *);
Boolean MMClose(void);

void MayInitBIMIDI(short inBufSize, short outBufSize);
void MayResetBIMIDI(Boolean evenIfMIDIThru);
void InitBIMIDITimer(void);

void InitEventList(void);
Boolean CheckEventList(long pageTurnTOffset);
Boolean CMCheckEventList(long pageTurnTOffset);

/* Higher-level MIDI, MIDI-play, and MIDI-utility routines */

LINK NoteNum2Note(LINK, short, short);
short UseMIDIChannel(Document *, short);
long TiedDur(Document *, LINK, LINK, Boolean);
short UseMIDINoteNum(Document *, LINK, short);
Boolean GetModNREffects(LINK, short *, short *, short *);

long Tempo2TimeScale(LINK);
long GetTempoMM(Document *, LINK);
long PDur2RealTime(long,TCONVERT [], short);
short MakeTConvertTable(Document *, LINK, LINK, TCONVERT [], short);

void StartMIDITime(void);
long GetMIDITime(long pageTurnTOffset);
void StopMIDITime(void);

void MMStartNoteNow(short, SignedByte, SignedByte);
void MMEndNoteAtTime(short, SignedByte, long);

Boolean MIDIConnected(void);
void MIDIFBOn(Document *);
void MIDIFBOff(Document *);
void MIDIFBNoteOn(Document *, short, short, short);
void MIDIFBNoteOff(Document *, short, short, short);

Boolean AnyNoteToPlay(Document *doc, LINK syncL, Boolean selectedOnly);
Boolean NoteToBePlayed(Document *doc, LINK aNoteL, Boolean selectedOnly);

/* High (UI)-level MIDI play routines */

Boolean IsMidiSustainOn(LINK pL);
Boolean IsMidiSustainOff(LINK pL);
Boolean IsMidiPan(LINK pL);
Boolean IsMidiController(LINK pL);
Byte GetMidiControlNum(LINK pL);
Byte GetMidiControlVal(LINK pL);

void PlaySequence(Document *, LINK, LINK, Boolean, Boolean);
void PlayEntire(Document *);
void PlaySelection(Document *);
