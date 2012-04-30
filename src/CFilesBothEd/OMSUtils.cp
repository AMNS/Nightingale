/* OMSUtils.c - Open Music Systems utilities for Nightingale - based on OMS SDK Sample
   App. Ken Brockman, 1995; minor changes by Don Byrd. */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1995-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
/* AllocOMSBuffer			DeallocOMSBuffer			PeekAtNextOMSMIDIPacket
	InitOMSBuffer			SignInToMIDIMgr			DeletePeekedAtOMSMIDIPacket
	GetOMSMIDIPacket		SignOutFromMIDIMgr		OMSSetSelectedInputDevice
	NightOMSReadHook		OMSAllNotesOff2			OMSSetMidiThruDevice
	OMSEndNoteNow			OMSStartNoteNow			CheckSignInOrOutOfMIDIManager
	OMSFBNoteOn				OMSFBNoteOff
	OMSMIDIProgram
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

// MAS #include <MIDI.h>
//JGG #include "OMSTimer.h"
// MAS #include "MIDIPASCAL3.h"

#include "CarbonStubs.h"

//#ifdef MIDI_THRU_ALWAYS	/* MIDI thru all the time, not just when recording. But not ready yet... */

/* In debug versions of Nightingale, we #define in our "Handle Manager", an
intermediate layer of memory management routines: this causes meaningless complaints
from OMSDisposeHandle. Avoid this annoyance by #undef'ing Handle Manager routines. */

#undef DisposeHandle
#undef NewHandle

/* MIDI Manager constants  */

#define OMSNotSignedIn   7
#define myClientID		creatorType
#define timePortID		'Atim'
#define mySignature		creatorType		/*	Creator ID of this application */

#define InputPortID	'in  '
#define OutputPortID	'out '

typedef struct OMSChannelRec {
	short ioRefNum;
	short portRefNum;
} OMSChannelRec;

typedef struct OMSMidiBufferQEl {
	struct OMSMidiBufferQEl *qLink;
	short							qType;
	OMSPacket					packet;
} OMSMidiBufferQEl, *OMSMidiBufferQElPtr;
	

OMSConnectionParams omsInputConnections;

/* Globals */
static Boolean gSignedIntoOMS;
static Boolean	gMIDIMgrExists;			/* is MIDI Manager present? */
static Boolean	gSignedInToMIDIMgr;		/* are we signed into MIDI Manager? */
static Boolean gNodesChanged;
static short	omsInputPortRefNum;			/* refNum of the OMS input port */
static short	omsOutputPortRefNum;			/* refNum of the OMS output port */
static short	gCompatMode;					/* current OMS compatibility mode */
static OMSDeviceMenuH	gInputMenu, gOutputMenu;
static Boolean preOMS2Version;
static Boolean omsWithMMTimer;
static Boolean appOwnsBITimer;
static short   newCompatMode;     		/* mode returned to AppHook */
static Boolean gCompatModeChanged;
static short whichNodesChanged;
static OMSErr omsErr;

static OMSUniqueID selectedInputDevice;
static short       selectedInputIORefNum;
static char	      selectedInputChannel;
static OMSUniqueID omsMidiThruDevice;
static short       omsMidiThruIORefNum;
static char        omsMidiThruChannel;

static QHdrPtr		omsMidiInputQHdrPtr;
static QHdrPtr		omsMidiAvailQHdrPtr;
static OMSMidiBufferQElPtr buffElementPtr;
static OMSMidiBufferQElPtr lastBuffElementPtr;

long			gmOMSBufferLength;		/* OMS MIDI Buffer size in bytes */
Boolean		gOMSMIDIBufferFull;

static Boolean		useMIDIThru;


static OMSMIDIPacket oms2NoteOffPacket = {0,0,MM_STD_FLAGS, 3, 0, 0, 0, 0, 0, 0};
static OMSMIDIPacket oms2NoteOnPacket = {0,0,MM_STD_FLAGS, 3, 0, 0, 0, 0, 0, 0};
static OMSMIDIPacket oms2ProgPacket = {0,0,MM_STD_FLAGS, 2, 0, 0, 0, 0, 0, 0};
									
#define refCon0				0L
#define noTimeBaseRefNum	0
#define noClient				'    '
#define noReadHook			0L
#define noTimeProc			0L
#define zeroTime				0L
#define zeroPeriod			0L
#define timePortBufSize		0L
#define inputPortBufSize	4096L
#define outputPortBufSize	0L

#define OMS_HDR_SIZE 6
#define OMS_NOTE_SIZE	OMS_HDR_SIZE+3
#define OMS_PROG_SIZE	OMS_HDR_SIZE+2


static Boolean	OMSManualPatch;			/* TRUE if not launched by a PatchBay Config. file */

/*  restate external entries for help
void OMSSetSelectedInputIORefNum(short ioRefNum);
void OMSEndNoteNow(INT16 noteNum, char channel, short ioRefNum);
void OMSStartNoteNow(INT16 noteNum, char channel, char velocity, short ioRefNum);
void OMSMIDIProgram(Document *doc, unsigned char *partPatch, 
						unsigned char *partChannel, short *partIORefNum);
void OMSAllNotesOff2(void);
void OMSFBOff(void);
void OMSMIDIFBNoteOff(Document *doc, INT16 noteNum, INT16 channel, short ioRefNum);
void OMSFBOn(void);
void OMSMIDIFBNoteOn(Document *doc, INT16 noteNum, INT16 channel, short ioRefNum);
*/

OMSCALLBACK(void) NightOMSReadHook(OMSPacket *packet, long readHookRefCon);
OMSAPI(void) NightOMSAppHook(OMSAppHookMsg *msg, long appHookRefCon);

void CheckSignInOrOutOfMIDIManager(void);
void	SignInToMIDIMgr(void);
void	SignOutFromMIDIMgr(void);
Boolean OMSSignIntoMMTimer(void);
void OMSCloseMMTimer(void);

/*---------------------------------------------------------------- AllocOMSBuffer -- */

static Boolean AllocOMSBuffer(void);
static Boolean AllocOMSBuffer(void)
{
	Size bytesFree, grow;
	INT16	i, num_elements;
	OMSMidiBufferQElPtr buffPtr;

	/* A single note takes two MIDI Packets padded to even length = 20 bytes in the buffer
	 * (for Note On and Note Off), while Nightingale's object list data structure as of
	 * v.3.0 takes 30 bytes per note plus a great deal of overhead for the Sync, etc. See
	 * the comment on AllocRawBuffer.
	 */
	bytesFree = MaxMem(&grow);
	gmOMSBufferLength = bytesFree/4L;							/* Take a fraction of available memory */
	gmOMSBufferLength = n_min(gmOMSBufferLength, 100000L);	/* Limit its size to make NewPtr faster */

	if (gmOMSBufferLength<5000L) return FALSE;

	/* how many Qels can be made from gmOMSBufferLength */
	num_elements = (gmOMSBufferLength - (2 * sizeof(QHdr))) / sizeof(OMSMidiBufferQEl);
	
	omsMidiInputQHdrPtr = (QHdrPtr)(NewPtr(sizeof(QHdr)));
	if (!GoodNewPtr((Ptr)omsMidiInputQHdrPtr)) return FALSE;
	omsMidiAvailQHdrPtr = (QHdrPtr)(NewPtr(sizeof(QHdr)));
	if (!GoodNewPtr((Ptr)omsMidiAvailQHdrPtr)) return FALSE;
	gmOMSBufferLength = num_elements * sizeof(OMSMidiBufferQEl);
	buffElementPtr = (OMSMidiBufferQElPtr)(NewPtr(gmOMSBufferLength));
	if (!GoodNewPtr((Ptr)buffElementPtr)) return FALSE;
	
	/* initialize the QHdrs */
	omsMidiInputQHdrPtr->qHead = 0;
	omsMidiInputQHdrPtr->qTail = 0;
	omsMidiAvailQHdrPtr->qHead = 0;
	omsMidiAvailQHdrPtr->qTail = 0;
	
	/* push the buffElements into the Avail Queue */
	for (i = 0, buffPtr = buffElementPtr; i < num_elements; i++, buffPtr++)
		Enqueue((QElemPtr)buffPtr, omsMidiAvailQHdrPtr);
	
	return TRUE;
}

/*------------------------------------------------------------- DeallocOMSBuffer -- */

static void DeallocOMSBuffer(void);
static void DeallocOMSBuffer(void)
{
	if (omsMidiInputQHdrPtr) DisposePtr((Ptr)omsMidiInputQHdrPtr);
	if (omsMidiAvailQHdrPtr) DisposePtr((Ptr)omsMidiAvailQHdrPtr);
	if (buffElementPtr) DisposePtr((Ptr)buffElementPtr);
}

/* ------------------------------------------------------------ GetOMSMIDIPacket -- */
/* ASSUMPTION, since buffer element is returned to Queue, it is unlikely to be reused
  before caller has a chance to use the packet */
  
MIDIPacket *GetOMSMIDIPacket(void)
{
	OMSMidiBufferQElPtr buffPtr;
	
	buffPtr = (OMSMidiBufferQElPtr)omsMidiInputQHdrPtr->qHead;
	if (buffPtr) {
		Dequeue((QElemPtr)buffPtr, omsMidiInputQHdrPtr);
		Enqueue((QElemPtr)buffPtr, omsMidiAvailQHdrPtr);
		return (MIDIPacket *)&buffPtr->packet;
	}
	return 0;
}

/* ----------------------------------------------------- PeekAtNextOMSMIDIPacket -- */

MIDIPacket *PeekAtNextOMSMIDIPacket(Boolean first)
{
	if (first)
		lastBuffElementPtr = (OMSMidiBufferQElPtr)omsMidiInputQHdrPtr->qHead;
	else
		lastBuffElementPtr = lastBuffElementPtr->qLink;
		
	if (lastBuffElementPtr)
		return (MIDIPacket *)&lastBuffElementPtr->packet;
		
	return 0;
}

/* -------------------------------------------------- DeletePeekedAtOMSMIDIPacket -- */

void DeletePeekedAtOMSMIDIPacket(void)
{
	if (lastBuffElementPtr) {
		Dequeue((QElemPtr)lastBuffElementPtr, omsMidiInputQHdrPtr);
		Enqueue((QElemPtr)lastBuffElementPtr, omsMidiAvailQHdrPtr);
	}
}

/* --------------------------------------------------------------- InitOMSBuffer -- */

void InitOMSBuffer(void)
{
	OMSMidiBufferQElPtr buffPtr, oldBuffPtr;
	OSErr err;
	
	/* move any old inputs back to avail queue */
	buffPtr = oldBuffPtr = (OMSMidiBufferQElPtr)omsMidiInputQHdrPtr->qHead;
	while (buffPtr) {
		buffPtr = oldBuffPtr->qLink;
		Dequeue((QElemPtr)oldBuffPtr, omsMidiInputQHdrPtr);
		Enqueue((QElemPtr)oldBuffPtr, omsMidiAvailQHdrPtr);
	}

	useMIDIThru = (config.midiThru && omsMidiThruIORefNum != OMSInvalidRefNum);
	gOMSMIDIBufferFull = FALSE;
	mRecIndex = 0;		/* OMS uses this a packet counter only */
}


/* --------------------------------------------------- set IORefNums for readHook -- */

void OMSSetSelectedInputDevice(OMSUniqueID device, INT16 channel)
{
	if (!gSignedIntoOMS) return;									/* can't be using this call */
	if (OMSChannelValid(device, channel)) {
		selectedInputDevice = device;
		selectedInputChannel = channel;
	}
	else {
		selectedInputDevice = config.defaultInputDevice;
		selectedInputChannel = config.defaultChannel;
	}
	if (OMSChannelValid(selectedInputDevice, selectedInputChannel))
		selectedInputIORefNum = OMSUniqueIDToRefNum(selectedInputDevice);
	else
		selectedInputIORefNum = OMSInvalidRefNum;
}

void OMSSetMidiThruDevice(OMSUniqueID device, INT16 channel)
{
	if (!gSignedIntoOMS) return;									/* can't be using this call */
	if (OMSChannelValid(device, channel)) {
		omsMidiThruDevice = device;
		omsMidiThruChannel = channel;
	}
	else {
		omsMidiThruDevice = config.defaultOutputDevice;
		omsMidiThruChannel = config.defaultOutputChannel;
	}
	if (OMSChannelValid(omsMidiThruDevice, omsMidiThruChannel))
		omsMidiThruIORefNum = OMSUniqueIDToRefNum(omsMidiThruDevice);
	else
		omsMidiThruIORefNum = OMSInvalidRefNum;
}

/* --------------------------------------------------------------- SignInToMIDIMgr -- */

void	SignInToMIDIMgr(void)
{
	/* whatever you want to do */
	if (OMSSignIntoMMTimer())
		gSignedInToMIDIMgr = TRUE;
}

/*------------------------------------------------------------ SignOutFromMIDIMgr -- */

void	SignOutFromMIDIMgr(void)
{
	/* whatever you want to do */
	OMSCloseMMTimer();
	gSignedInToMIDIMgr = FALSE;
}

/* ----------------------------------------------- CheckSignInOrOutOfMIDIManager -- */

void	CheckSignInOrOutOfMIDIManager(void)
{
	/* Make sure MIDI Manager is installed; it's possible (but unlikely) that OMS will
		say that the compatibility mode is omsModeUseMIDIMgr though MIDI Manager is
		not present */
	if (!gMIDIMgrExists)
		return;
	if (gCompatMode == omsModeUseMIDIMgr) {
		if (!gSignedInToMIDIMgr)
			SignInToMIDIMgr();
	} else {
		if (omsWithMMTimer && gSignedInToMIDIMgr)
			SignOutFromMIDIMgr();
	}
}

/* --------------------------------------------------------------- LaunchPatchBay -- */
/* Launch the patchBay application if possible, otherwise disable menu item. ??At
the moment, does nothing! */

void LaunchPatchBay(void)
{
}

/* ----------------------------------------------------------------- GiveResError -- */
/* Alert user to Resource Manager Error, if there is one, and kill the program. ??NB:
as of this writing, in several places, this is called after GetResource iff the
resource handle is NULL, so in those cases only it should give an error message and
quit regardless of ResError. (According to Inside Mac, I-119, GetResource can return
NULL without an error that ResError would report under fairly common circumstances.)
This should be cleaned up some day along the lines of ReportBadResource. */

void GiveOMSResError(char *);
void GiveOMSResError(char *Msg)		/* Argument is ignored */
{
	if (ReportResError()) {
		GetIndCString(strBuf, MIDIERRS_STRS, 31);		/* "Fatal error in handling OMS resources." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		OMSSignOut(mySignature);	
		ExitToShell();
	}
}

void GiveMMResError(char *);
void GiveMMResError(char *Msg)		/* Argument is ignored */
{
	if (ReportResError()) {
		GetIndCString(strBuf, MIDIERRS_STRS, 25);		/* "Fatal error in handling MIDI Manager resources." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		OMSSignOut(mySignature);	
		ExitToShell();
	}
}


/* ========================================================= Timing calls for OMS == */

#include "NTimeMgr.h"

void OMSInitTimer(void)
{
	if (!omsWithMMTimer) {
		OMSClaimTimer(myClientID);
		appOwnsBITimer = TRUE;
		NTMInit();                              /* initialize timer */
	}
}

void OMSLoadTimer(INT16 interruptPeriod)
{
	if (!omsWithMMTimer) {
		if (appOwnsBITimer)
			;												/* There's nothing to do here. */
	}
}

void OMSStartTime(void)
{
	if (omsWithMMTimer) {
		MIDISetCurTime(timeMMRefNum, 0L);
		MIDIStartTime(timeMMRefNum);
	}
	else {												/* use built-in */
		OMSClaimTimer(myClientID);
		appOwnsBITimer = TRUE;
		NTMStartTimer(1);
	}
}

long OMSGetCurTime(void)
{
	long time;

	if (omsWithMMTimer)
		time = MIDIGetCurTime(timeMMRefNum);
	else													/* use built-in */
		time = NTMGetTime();

	return time;
}

void OMSStopTime(void)
{
	if (omsWithMMTimer)
		MIDIStopTime(timeMMRefNum);
	else {												/* use built-in */
		if (appOwnsBITimer) {
			NTMStopTimer();							/* we're done with the millisecond timer */
			NTMClose();
			appOwnsBITimer = FALSE;
		}
	}
}


/* ------------------------------------------------------- OMS End/Start Note Now -- */

void OMSEndNoteNow(INT16 noteNum, char channel, short ioRefNum)
{
	OMSPacket packet = {MM_STD_FLAGS, OMS_NOTE_SIZE, 0, 0, 0, 0, 0, 0};

	packet.data[0] = MNOTEON+channel-1;
	packet.data[1] = noteNum;
	if (ioRefNum != OMSInvalidRefNum)
		OMSWritePacket(&packet, ioRefNum, omsOutputPortRefNum);
}

void OMSStartNoteNow(INT16 noteNum, char channel, char velocity, short ioRefNum)
{
	OMSPacket packet = {MM_STD_FLAGS, OMS_NOTE_SIZE, 0, 0, 0, 0, 0, 0};

	packet.data[0] = MNOTEON+channel-1;
	packet.data[1] = noteNum;
	packet.data[2] = velocity;
	if (ioRefNum != OMSInvalidRefNum)
		OMSWritePacket(&packet, ioRefNum, omsOutputPortRefNum);
}


/* ----------------------------------------------------------- OMS Feedback setup -- */

void OMSFBOff(Document *doc)
{
	if (doc->feedback) {
		if (omsWithMMTimer)
			MIDIStopTime(timeMMRefNum);
		else {											/* use built-in */
			if (appOwnsBITimer) {
				MidiStopTime();
			}
		}
	}
}

void OMSFBOn(Document *doc)
{
	if (doc->feedback) {
		if (omsWithMMTimer) {
 			MIDISetCurTime(timeMMRefNum, 0L);
			MIDIStartTime(timeMMRefNum);
 		}
		else {											/* use built-in */
			OMSClaimTimer(myClientID);
			appOwnsBITimer = TRUE;
			MidiStartTime();
		}
	}
}

/* ---------------------------------------------------------- OMSMIDIFBNoteOn/Off -- */

/* Start MIDI "feedback" note by sending a MIDI NoteOn command for the
specified note and channel. */

void OMSFBNoteOn(Document *doc, INT16 noteNum, INT16 channel, short ioRefNum)
{
	if (doc->feedback) {
		OMSStartNoteNow(noteNum, channel, config.feedbackNoteOnVel, ioRefNum);
		SleepTicks(2L);
	}
}

/* End MIDI "feedback" note by sending a MIDI NoteOn command for the
specified note and channel with velocity 0 (which acts as NoteOff). */

void OMSFBNoteOff(Document *doc, INT16 noteNum, INT16 channel, short ioRefNum)
{
	if (doc->feedback) {
		OMSEndNoteNow(noteNum, channel, ioRefNum);
		SleepTicks(2L);
	}
}


/* -------------------------------------------------------------- OMSMIDIProgram -- */

#define PATCHNUM_BASE 1			/* Some synths start numbering at 1, some at 0 */

/* Loop through part settings (patch, channel, and OMS IORefNum tables) and send
 * patch change packet to each */
void OMSMIDIProgram(Document *doc, unsigned char *partPatch, 
						unsigned char *partChannel, short *partIORefNum)
{
	INT16 i;
	OMSPacket packet = {MM_STD_FLAGS, OMS_PROG_SIZE, 0, 0, 0, 0, 0, 0};

	if (!gSignedIntoOMS) return;										/* can't be using this call */
	if (doc->polyTimbral && !doc->dontSendPatches) {
		for (i = 1; i<=LinkNENTRIES(doc->headL) -1; i++) {		/* skip dummy part */
			packet.data[0] = MPGMCHANGE+ partChannel[i] -1;
			packet.data[1] = partPatch[i] - PATCHNUM_BASE;
			if (partIORefNum[i] != OMSInvalidRefNum)
				OMSWritePacket(&packet, partIORefNum[i], omsOutputPortRefNum);
		}

#if 0 // JGG: testing for dumb QT MusInsts...
// none of this helps, even though stopping here with the debugger for 
// "long enough" -- and not executing the following code -- works.
		{
			EventRecord evt;
//			SleepTicks(60L);
			WaitNextEvent(everyEvent, &evt, 60L, NULL); // also tried OSEventAvail, EventAvail
//			SleepTicks(60L);
		}
#endif
	}
}


/* ------------------------------------------------------------- OMSAllNotesOff2 -- */

void OMSAllNotesOff2(void)
{
	INT16 noteNum;
	short ioRefNum;
	register INT16 i, j;
	OMSNodeInfoListH nodeInfoListH;
	OMSUniqueID noteOffDeviceList[MAXSTAVES];
	short 		noteOffChannelMap[MAXSTAVES];
	INT16 numNoteOffDevices;
	OMSUniqueID *partDevicePtr;
	Boolean deviceFound;

	if (!gSignedIntoOMS) return;										/* can't be using this call */
	WaitCursor();

	OMSAllNotesOff();	 				/* OMS provided, sends MIDI Note Off msg to all devices */
	
	/* Since above call is not supported by all hardware, loop through notes
		(pausing between each) and for each note send a OMSEndNoteNow to every
		valid output device and its channels.
	*/
	
	/* First build list of score's part's devices and their channelMaps */

	for (i = MAXSTAVES-1; i>=0; i--) {
		noteOffDeviceList[i] = 0;
		noteOffChannelMap[i] = 0;
	}
	numNoteOffDevices = 0;
	partDevicePtr = currentDoc->omsPartDeviceList;
	for (i = 0; i < MAXSTAVES; i++, partDevicePtr++) {
		if (*partDevicePtr) {
			deviceFound = FALSE;
			for (j = 0; j < numNoteOffDevices; j++) {
				if (noteOffDeviceList[j] == *partDevicePtr) {	/* already listed */
					deviceFound = TRUE;
					break;
				}
			}
			if (!deviceFound) {
				if (nodeInfoListH = OMSGet1NodeInfo(*partDevicePtr))	{
					noteOffChannelMap[numNoteOffDevices] = (*(*nodeInfoListH)->info[0].deviceH)->midiChannels;
					noteOffDeviceList[numNoteOffDevices++] = *partDevicePtr;
					OMSDisposeHandle(nodeInfoListH);
				}
			}
		}
	}

	/* Now loop through each note and send a Note Off to each device and its channels */
	for (noteNum = 0; noteNum<=MAX_NOTENUM; noteNum++) {
		for (i = 0; i < numNoteOffDevices; i++) {
			ioRefNum = OMSUniqueIDToRefNum(noteOffDeviceList[i]);
			if (ioRefNum != OMSInvalidRefNum)
				for (j = 1; j<=MAXCHANNEL; j++)
					if (BitTst(&noteOffChannelMap[i], j-1))
						OMSEndNoteNow(noteNum, j, ioRefNum);
		}
		SleepMS(1L);														/* pause between notes */
	}
}


/* ------------------------------------------------------------ NightOMSReadHook -- */

/* The Read Hook Function. Reads all incoming MIDI data. 
N.B. This function saves and restores A5 since we are called at interrupt level. */

OMSCALLBACK(void) NightOMSReadHook(OMSPacket *packet, long readHookRefCon)
{
	short				length, i;		
	long				oldA5;
	OMSMidiBufferQElPtr buffPtr, inputTail;
	OSErr				err;
	long				time_stamp;
	OMSPacketPtr	recvPkt;
	
	/*	Set up A5 for access to globals during this routine. */
	long olda5 = SetA5(readHookRefCon);
	
	recvPkt = (OMSPacketPtr)packet;
	length = recvPkt->len;
	
	if (length>=OMS_HDR_SIZE) {							/* if not, things are really screwed up! */
		
		/* If we're recording and we have more Queue elements available: */
		/* at this time, this routine requires that only one input stream exist */
		/* in future, could use thePacketPtr->srcIORefNum to select input buffer */
		/* for packets from the selected input node */

#ifdef MIDI_THRU_ALWAYS
		if (useMIDIThru)	/* midiThru desired and omsMidiThruIORefNum is valid */
			OMSWritePacket(packet, omsMidiThruIORefNum, omsOutputPortRefNum);
#endif

		if (recordingNow) {
			buffPtr = (OMSMidiBufferQElPtr)omsMidiAvailQHdrPtr->qHead;
			if (buffPtr) {										/* if buffer element avail */

				/* dequeue avail element and enqueue to input */
				Dequeue((QElemPtr)buffPtr, omsMidiAvailQHdrPtr);
				Enqueue((QElemPtr)buffPtr, omsMidiInputQHdrPtr);
								
				/* copy the input packet to the queue element and time stamp it */
				buffPtr->packet = *recvPkt;
				time_stamp = OMSGetCurTime();
				((MIDIPacket *)(&buffPtr->packet))->tStamp = time_stamp;
				
#ifndef MIDI_THRU_ALWAYS
				if (useMIDIThru)	/* midiThru desired and omsMidiThruIORefNum is valid */
					OMSWritePacket(packet, omsMidiThruIORefNum, omsOutputPortRefNum);
#endif
				if (mRecIndex++ == 0) {							/* save the first time stamp */
					mFirstTime = time_stamp - 100L;		/* with a little extra time buffer in front */
					if (mFirstTime < 0L) mFirstTime = 0L;
				}
				mFinalTime = time_stamp;						/* save the latest time stamp */
			}
			else {													/* no avail packets */
				recordingNow = FALSE;
				gOMSMIDIBufferFull = TRUE;
			}
		}
	}
	SetA5(olda5);
}


/* ------------------------------------------------------------------ LoadPatches -- */
/* Get previously-saved connections (port info records) for all ports from our
'port' resource. */

void MMLoadTimerPatches(void);
void MMLoadTimerPatches()
{
	MIDIPortInfoHdl	portInfoH;	/* Handle to port info record. */
	MIDIPortInfoPtr	portInfoP;	/* Pointer to port info record. */
	short					i, theErr;
		
	/* SET UP TIME PORT CONNECTIONS. */
	
	portInfoH = (MIDIPortInfoHdl) Get1Resource(portResType, time_port);
	if (portInfoH==NULL)
		GiveMMResError("portResType, time_port");

	HLock((Handle) portInfoH);
	portInfoP = *portInfoH;
	if (GetHandleSize((Handle) portInfoH) != 0)
	{
			/* Were we supposed to be sync'd to another client? */
		if (portInfoP->timeBase.clientID != noClient)
		{		
				/* Yes, so make that client our time base. */
			theErr = MIDIConnectTime(portInfoP->timeBase.clientID, 
						portInfoP->timeBase.portID,
						myClientID, 
						timePortID);
				/* Is the client still signed in? */
			if (theErr != midiVConnectErr) 
			{	
					/* Yes, so set our sync mode to external. */
				MIDISetSync(timeMMRefNum, midiExternalSync);
			}
			
		}
			/* Were we somebody else's time base? */
		for (i=0; i<portInfoP->numConnects; i++)
		{
			MIDIConnectTime(myClientID, 
						timePortID, 
						portInfoP->cList[i].clientID, 
						portInfoP->cList[i].portID);
		}
	}
	HUnlock((Handle) portInfoH);
	ReleaseResource((Handle) portInfoH);
	GiveMMResError("LoadPatches");
}


/* ------------------------------------------------------------------ MMSavePatch -- */
/* Save current connections for the given port (a port info record) in a 'port'
resource. */

void MMSavePatch(OSType, short, unsigned char *);
void MMSavePatch(OSType portID, short portInfoResID, unsigned char *portInfoResName)
{
	Handle		portResH;	/* Handle to 'port' resource. */
	
	WaitCursor();

	/* Remove existing port info resource. */
	
	portResH = Get1Resource(portResType, portInfoResID);
	GiveMMResError("MMSavePatch");
	RemoveResource(portResH);
	GiveMMResError("MMSavePatch");
	ChangedResource(portResH);
	DisposeHandle(portResH);
	UpdateResFile(CurResFile());
	GiveMMResError("MMSavePatch");
	
	/*	Get new configuration. */
	
	portResH = (Handle) MIDIGetPortInfo(myClientID, portID);
	
	/*	Save new configuration. */
	
	AddResource(portResH, portResType, portInfoResID, portInfoResName);
	GiveMMResError("MMSavePatch");
	WriteResource(portResH);
	GiveMMResError("MMSavePatch");
	UpdateResFile(CurResFile());
	GiveMMResError("MMSavePatch");
	ReleaseResource(portResH);
	GiveMMResError("MMSavePatch");
	
	InitCursor();
}


/* -------------------------------------------------------------- NightOMSAppHook -- */
/* Used to communicate environment changes from OMS to the application. */

OMSAPI(void) NightOMSAppHook(OMSAppHookMsg *msg, long appHookRefCon)
{
	OMSUniqueID deletedDestUniqueId;
	short deletedDestIoRefNum;
	
#ifdef THINK_C
	/*	Set up A5 for access to globals during this routine. */
	asm {
		move.l	a5,-(sp)		; save previous A5
		move.l	appHookRefCon,a5		; set up A5 from myRefCon
	}
#elif __MWERKS__
	long olda5 = SetA5(appHookRefCon);
#endif

	switch (msg->msgType) {
		case omsMsgModeChanged:
			/* Respond to compatibility mode having changed */
			gCompatModeChanged = TRUE;
			gCompatMode = msg->u.modeChanged.newMode;
			/* this will cause side effects in the event loop */
			break;
		case omsMsgDestDeleted: /* never at interrupt, usually in background.  NodesChanged will be sent later */
			/* check against list and if used, invalidate it so as to stop sending MIDI to it*/
			deletedDestIoRefNum = msg->u.nodeDeleted.ioRefNum;
			deletedDestUniqueId = msg->u.nodeDeleted.uniqueID;
			gNodesChanged = TRUE;
			break;
		case omsMsgNodesChanged: /* bits show which type of node. Rebuild menus. all deveice handles are invalid */
			gNodesChanged = TRUE;
			whichNodesChanged = msg->u.nodesChanged.changedNodeBits;
			break;
		case omsMsgTimerClaimed: /* never called at interrupt and only if in backgound and last to claimTimer */
			appOwnsBITimer = FALSE;
			break;
		case omsMsgDifferentStudioSetup:
			/* save for operator actions */
			gNodesChanged = TRUE;
			break;
		case omsMsgConnectsChanged: /* someone opened or closed a connection.  msg is list of updated connections */
			break;
	}
	
#ifdef THINK_C
	asm {
		move.l	(sp)+,a5		; restore previous A5
	}
#elif __MWERKS__
	SetA5(olda5);
#endif
}


/* ------------------------------------------------------- CheckOMSDefaultDevices -- */

void CheckOMSDefaultDevices(void);
void CheckOMSDefaultDevices(void)
{
	INT16				i,j,k,l;
	short 			channelMap;
	OMSNodeInfoListH tempNodeInfoListH;
	unsigned char	flags1, flags2;
	short 			flags3;
	unsigned short flagsMask;

	
	/* Set up default input and output devices and channels */
	/* If current default input is valid, use it. Otherwise, preset default to none,
		get list of inputs, looking for real ones first and then for virtual ones, find
		first connected controller device (i.e., keyboard) with available channel
	*/
	
	if (!OMSChannelValid(config.defaultInputDevice, (INT16)config.defaultChannel)) {
		config.defaultInputDevice = 0;
		config.defaultChannel = 1;
		tempNodeInfoListH = OMSGetNodeInfo(omsIncludeInputs + omsIncludeReal);
		if ((*tempNodeInfoListH)->numNodes != 0) {			/* no input devices available */
			flagsMask = kIsReal;
			for (l = 2; l != 0; l--) {
				k = -1;
				for (j = 0; j < (*tempNodeInfoListH)->numNodes; j++) {
					if (((*tempNodeInfoListH)->info[j].flags & flagsMask)) {
						if ((*tempNodeInfoListH)->info[j].deviceH) {
							flags1 = (*(*tempNodeInfoListH)->info[j].deviceH)->flags1;
							if ((flags1 & kIsController) && (flags1 & kOutConnectedToTopLevel) && (flags1 & kOutConnected)) {
								channelMap = channelMap = (*(*tempNodeInfoListH)->info[j].deviceH)->midiChannels;
								k = -1;
								for( i = 0; i < 16; i++)
									if (BitTst(&channelMap, i)) {k = i; break;}	/* found a channel */
								if (k >= 0) {
									config.defaultChannel = k+1;
									config.defaultInputDevice = (*tempNodeInfoListH)->info[j].uniqueID;
								}
							}
						}
					}
				}
				if (k >= 0) break;									/* found a real node, use it */
				flagsMask = -1;										/* all types of nodes */
			}
		}
		OMSDisposeHandle(tempNodeInfoListH);
	}

	/* If current default output is valid, use it.
		Otherwise, preset default to none, get list of inputs, looking for real ones
		first and then for virtual ones, find first connected receiver device (but not a
		patcher) with available channel.
	*/
	
	if (!OMSChannelValid(config.defaultOutputDevice, (INT16)config.defaultOutputChannel)) {
		config.defaultOutputDevice = 0;
		config.defaultOutputChannel = 1;
		tempNodeInfoListH = OMSGetNodeInfo(omsIncludeOutputs + omsIncludeReal);
		if ((*tempNodeInfoListH)->numNodes != 0) {		/* no output devices available */
			flagsMask = kIsReal;
			for (l = 2; l != 0; l--) {							/* two loop, first is Real, second anything */
				k = -1;
				for (j = 0; j < (*tempNodeInfoListH)->numNodes; j++) {
					if (((*tempNodeInfoListH)->info[j].flags & flagsMask)) {
						if ((*tempNodeInfoListH)->info[j].deviceH) {
							flags1 = (*(*tempNodeInfoListH)->info[j].deviceH)->flags1;
							if ((flags1 & kIsReceiver) && (flags1 & kInConnectedToTopLevel) && (flags1 & kInConnected) && !(flags1 & kIsPatcher)) {
								channelMap = channelMap = (*(*tempNodeInfoListH)->info[j].deviceH)->midiChannels;
								k = -1;
								for( i = 0; i < 16; i++)
									if (BitTst(&channelMap, i)) {k = i; break;}
								if (k >= 0) {
									config.defaultOutputChannel = k+1;
									config.defaultOutputDevice = (*tempNodeInfoListH)->info[j].uniqueID;
								}
							}
						}
					}
				}
				if (k >= 0) break;
				flagsMask = -1;
			}
		}
		OMSDisposeHandle(tempNodeInfoListH);
	}

	/* Test and set up default metronome output device and channel */

	if (!OMSChannelValid(config.metroDevice, (INT16)config.metroChannel)) {
		config.metroDevice = config.defaultOutputDevice;
		config.metroChannel = config.defaultOutputChannel;
	}
}


/* ---------------------------------------------------------- OMSSignIntoMMTimer -- */

Boolean OMSSignIntoMMTimer(void)
{
	long				verNumMM;		/* MIDI Manager version */
	MIDIPortParams	init;				/* MIDI Manager Init data structure  */
	Handle			iconHndl;
	OSErr				theErr;
	Str255			pStrBuf;
	short				curResFile;

	iconHndl = GetResource('ICN#', NGALE_ICON);
	if (ReportBadResource(iconHndl)) return FALSE;

	verNumMM = NMIDIVersion();
	if (verNumMM == 0)
		return FALSE;
	omsWithMMTimer = TRUE;
	gMIDIMgrExists = TRUE;

	strcpy((char *)pStrBuf, PROGRAM_NAME);
	CToPString((char *)pStrBuf);
	theErr = MIDISignIn(myClientID, refCon0, iconHndl, pStrBuf);
	if (theErr) return FALSE;
	
	gSignedInToMIDIMgr = TRUE;
	/* need to determine if system can support patchbay launching */
	/* needs to be manually launched ala old style */
	canLaunchPatchBay = FALSE;
	/* Assume not a PatchBay configuration. */
	
	OMSManualPatch = TRUE;	

	/* Add time port. */
	
	init.portID = timePortID;
	init.portType = midiPortTypeTime;
	init.timeBase = noTimeBaseRefNum;
	init.readHook = noReadHook;
	init.initClock.syncType = midiInternalSync;
	init.initClock.curTime = zeroTime;
	init.initClock.format = midiFormatMSec;
	init.refCon = SetCurrentA5();
	Pstrcpy(init.name, "\pTimeBase");
	theErr = MIDIAddPort(myClientID, timePortBufSize, &timeMMRefNum, &init);
		
	if (theErr==midiVConnectMade) 
		OMSManualPatch = FALSE;							/* A PatchBay connection has been resolved */
	else if (theErr==memFullErr) {
		GetIndCString(strBuf, MIDIERRS_STRS, 28);		/* "Not enough memory to add time port." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		MIDISignOut(myClientID);	
		return FALSE;
	}
	/* If not a PatchBay patch, connect ports as they were when we last quit. */
	
	if (OMSManualPatch) {
		/* 
		 * Get the resources from the Nightingale Setup file, and then restore the
		 * previous current resource file.
		 */
		curResFile = CurResFile();
		UseResFile(setupFileRefNum);
		MMLoadTimerPatches();
		UseResFile(curResFile);
	}
	
	return TRUE;
}


/* ------------------------------------------------------------- OMSCloseMMTimer -- */

void OMSCloseMMTimer(void);
void OMSCloseMMTimer(void)
{
	MIDIPortInfoHdl portInfoH;
	Handle portResH;					/* Handle to 'port' resource. */
	Boolean patchChanged;
	size_t resLen;
	short curResFile;

	OMSStopTime();															/* in case it's still on */

	if (OMSManualPatch) {
		/*
		 * Get the 'port' resources from file and compare them to our internal values.
		 * If any has been changed, update the file.
		 */
		portInfoH = (MIDIPortInfoHdl) Get1Resource(portResType, time_port);
		if (ReportBadResource((Handle)portInfoH)) goto SignOut;
		resLen = GetResourceSizeOnDisk((Handle)portInfoH);
		portResH = (Handle) MIDIGetPortInfo(myClientID, timePortID);
		patchChanged = memcmp( (void *)*portInfoH, (void *)*portResH, resLen);
		if (patchChanged) {
	 		if (CautionAdvise(SAVEMM_ALRT)==OK) {
				/* 
				 * Save the resources in the Nightingale Setup file, and then restore the
				 * previous current resource file.
				 */
				curResFile = CurResFile();
				UseResFile(setupFileRefNum);
				if (ReportResError()) goto SignOut;
				MMSavePatch(timePortID, time_port, "\ptimePortInfo");
				UseResFile(curResFile);
			}
		}
	}

SignOut:
		MIDISignOut(mySignature);
		omsWithMMTimer = FALSE;
		gSignedInToMIDIMgr = FALSE;
}


/* --------------------------------------------------------- OMSInitBuiltInTimer -- */

void OMSInitBuiltInTimer(void);
void OMSInitBuiltInTimer(void)
{
	omsWithMMTimer = FALSE;
}

/* -------------------------------------------------------- OMSCloseBuiltInTimer -- */

void OMSCloseBuiltInTimer(void);
void OMSCloseBuiltInTimer(void)
{
	OMSStopTime();															/* in case it's still on */
}


/* ------------------------------------------------------- SetOMSConfigMenuTitles -- */
/* Set titles of menu items for opening OMS Setup application. Can't do this
in OMSInit, because menus haven't been setup yet. */

void SetOMSConfigMenuTitles()
{
	Str255 str;

	if (useWhichMIDI!=MIDIDR_OMS) return;

	GetIndString(str, MENUCMD_STRS, 23);				/* "OMS MIDI SetupÉ" */
	SetMenuItemText(playRecMenu, PL_OpenExtMIDISetup1, str);
	GetIndString(str, MENUCMD_STRS, 24);				/* "Open Current OMS Studio SetupÉ" */
	SetMenuItemText(playRecMenu, PL_OpenExtMIDISetup2, str);
}


/* ---------------------------------------------------------------------- OMSInit -- */
/* Initialize the OMS Manager. If the OMS Manager is installed, sign into it and
set up time, input, and output ports. Do NOT start the time base clock. If we succeed,
return TRUE, else FALSE. If the OMS is not installed, just return FALSE. */

Boolean OMSInit(void)
{
	long				OMSVerNum;
	OSErr				theErr;
	OMSAppHookUPP  appHook;
	OMSReadHookUPP readHook;
	long           gestResp;
	OMSString		appName;
	
	appHook = NewOMSAppHook(NightOMSAppHook);
	readHook = NewOMSReadHook(NightOMSReadHook);

	/* Check config versus available resources, sign in, and set up variables */
	
	OMSVerNum = OMSVersion();
	preOMS2Version = TRUE;									/* temporarily ensure only pre2 calls */

	canLaunchFMSorOMSSetup = FALSE;
	if (OMSVerNum == 0) return FALSE;
	
	if (OMSVerNum >= 0x01200000)
		if (Gestalt(gestaltOSAttr, &gestResp) == noErr)
			if (gestResp & (1<<gestaltLaunchCanReturn))
				canLaunchFMSorOMSSetup = TRUE;
	
	if (!AllocOMSBuffer()) {
		GetIndCString(strBuf, INITERRS_STRS, 9);		/* "Can't allocate buffer for MIDI" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
		
	gSignedIntoOMS = FALSE;
	
	/*
	 * According to Ken, OMS v.2 always returns gCompatMode = 0, i.e., not MIDI Manager
	 * compatible!
	 */
	 
	theErr = OMSSignIn(mySignature, SetCurrentA5(), LMGetCurApName(), appHook, &gCompatMode);
	if (theErr == omsDupClientErr) {
		OMSSignOut(mySignature);
	theErr = OMSSignIn(mySignature, SetCurrentA5(), LMGetCurApName(), appHook, &gCompatMode);
		if (theErr) { OMSSignOut(mySignature); return theErr; }
	}
	gSignedIntoOMS = TRUE;
	
	/*	Add an input port. Nothing is connected to it yet, though. */
	theErr = OMSAddPort(mySignature, InputPortID, omsPortTypeInput, (OMSReadHook2UPP)readHook, 
		SetCurrentA5(), &omsInputPortRefNum);
	if (theErr) {OMSSignOut(mySignature); return theErr;}
	
	/*	Add an output port */
	theErr = OMSAddPort(mySignature, OutputPortID, omsPortTypeOutput, NULL, 0L,
		 &omsOutputPortRefNum);
	if (theErr) {OMSSignOut(mySignature); return theErr;}

	CheckOMSDefaultDevices();
		
	/* Set up the timing support; use MIDI Mgr or built-in for initial go round */
	omsWithMMTimer = FALSE;
	if (gCompatMode == omsModeUseMIDIMgr) {
		if (NMIDIVersion()!=0)
			gMIDIMgrExists = TRUE;
		if (!OMSSignIntoMMTimer())
			OMSInitBuiltInTimer();
	}
	else
		OMSInitBuiltInTimer();
		
	recordingNow = FALSE;
	OMSSetSelectedInputDevice(0, 0);
#ifdef MIDI_THRU_ALWAYS
	OMSSetMidiThruDevice(config.thruDevice, config.thruChannel);
	useMIDIThru = (config.midiThru && omsMidiThruIORefNum != OMSInvalidRefNum);
#endif
	return TRUE;
}


/* ------------------------------------------------------------------- OMSClose -- */
/* Save our patch (port connection) configuration, if necessary, and sign out from
the MIDI Manager. */

Boolean OMSClose(void)
{
	Boolean okay = TRUE;
	MIDIPortInfoHdl portInfoH;
	Handle portResH;					/* Handle to 'port' resource. */
	Boolean patchChanged;
	size_t resLen;
	short curResFile;

	if (useWhichMIDI != MIDIDR_OMS) return okay;

	OMSStopTime();															/* in case it's still on */
	if (gSignedInToMIDIMgr)
		OMSCloseMMTimer();
	else
		OMSCloseBuiltInTimer();

	if (gSignedIntoOMS) {
		OMSSignOut(mySignature);
		gSignedIntoOMS = FALSE;
	}
	
	DeallocOMSBuffer();
	
	return okay;
}


/* ---------------------------------------------------------- OMSEventLoopAction -- */

void OMSEventLoopAction(void)
{
	Boolean curSignedInToMIDIMgr;

	if (gSignedIntoOMS) {
	
		if (gNodesChanged) {
			/* need to look for default devices, if not already valid */
			/* docs will check themselves prior to use of dev/chan */
			CheckOMSDefaultDevices();
			gNodesChanged = FALSE;
		}
		
		if (gCompatModeChanged) {
			curSignedInToMIDIMgr = gSignedInToMIDIMgr;
			CheckSignInOrOutOfMIDIManager();
			if (curSignedInToMIDIMgr != gSignedInToMIDIMgr) {		/* changed timer status */
				if (omsWithMMTimer)											/* now using MM */
					OMSCloseBuiltInTimer();
				else
					OMSInitBuiltInTimer();
			}
			gCompatModeChanged = FALSE;
		}
	}
}


/* -------------------------------------------------------------- OMSResumeEvent -- */

void OMSResumeEvent(void)
{
	if (gSignedIntoOMS)
		OMSResume(mySignature);
}

/* ------------------------------------------------------------- OMSSuspendEvent -- */

void OMSSuspendEvent(void)
{
	if (gSignedIntoOMS)
		OMSSuspend(mySignature);
}


/* ---------------------------------------------------------- CreateOMSOutputMenu -- */

OMSDeviceMenuH CreateOMSOutputMenu(Rect *menuBox)
{
	if (!gSignedIntoOMS) return NULL;
	return NewOMSDeviceMenu(
					NULL,
					odmFrameBox,
					menuBox,
					omsIncludeOutputs + omsIncludeReal + omsIncludeVirtual,
					NULL);
}

/* ---------------------------------------------------------- CreateOMSInputMenu -- */

OMSDeviceMenuH CreateOMSInputMenu(Rect *menuBox)
{
	if (!gSignedIntoOMS) return NULL;
	return NewOMSDeviceMenu(
					NULL,
					odmFrameBox,
					menuBox,
					omsIncludeInputs + omsIncludeReal,
					NULL);
}

/* ---------------------------------------------------------- GetOMSDeviceForPart -- */

OMSUniqueID GetOMSDeviceForPartn(Document *doc, INT16 partn)
{
	if (!doc) doc = currentDoc;
	return doc->omsPartDeviceList[partn];
}

OMSUniqueID GetOMSDeviceForPartL(Document *doc, LINK partL)
{
	INT16 partn;
	
	if (!doc) doc = currentDoc;
	partn = (INT16)PartL2Partn(doc, partL);
	return doc->omsPartDeviceList[partn];
}

/* ---------------------------------------------------------- SetOMSDeviceForPart -- */

void SetOMSDeviceForPartn(Document *doc, INT16 partn, OMSUniqueID device)
{
	if (!doc) doc = currentDoc;
	doc->omsPartDeviceList[partn] = device;
}

void SetOMSDeviceForPartL(Document *doc, LINK partL, OMSUniqueID device)
{
	INT16 partn;
	
	if (!doc) doc = currentDoc;
	partn = (INT16)PartL2Partn(doc, partL);
	doc->omsPartDeviceList[partn] = device;
}

/* --------------------------------------------------------- InsertPartnOMSDevice -- */
/* Insert a null device in front of partn's device */

void InsertPartnOMSDevice(Document *doc, INT16 partn, INT16 numadd)
{
	register INT16 i, j;
	INT16 curLastPartn;
	
	if (!doc) doc = currentDoc;
	curLastPartn = LinkNENTRIES(doc->headL) - 1;
	/* shift list from partn through curLastPartn up by numadd */
	for (i=curLastPartn, j=i+numadd;i >= partn;)
		doc->omsPartDeviceList[j--] = doc->omsPartDeviceList[i--];
	for (i=partn;i<partn+numadd;)
		doc->omsPartDeviceList[i++] = 0;
}

void InsertPartLOMSDevice(Document *doc, LINK partL, INT16 numadd)
{
	INT16 partn;
	
	if (!doc) doc = currentDoc;
	partn = (INT16)PartL2Partn(doc, partL);
	InsertPartnOMSDevice(doc, partn, numadd);
}


/* --------------------------------------------------------- DeletePartnOMSDevice -- */

void DeletePartnOMSDevice(Document *doc, INT16 partn)
{
	register INT16 i, j;
	INT16 curLastPartn;
	
	if (!doc) doc = currentDoc;
	curLastPartn = LinkNENTRIES(doc->headL) - 1;
	/* Shift list from partn+1 through curLastPartn down by 1 */
	for (i=partn+1, j=i-1;i >= curLastPartn;)
		doc->omsPartDeviceList[i++] = doc->omsPartDeviceList[j++];
	doc->omsPartDeviceList[curLastPartn] = 0;
}

void DeletePartLOMSDevice(Document *doc, LINK partL)
{
	INT16 partn;

	if (!doc) doc = currentDoc;
	partn = (INT16)PartL2Partn(doc, partL);
	DeletePartnOMSDevice(doc, partn);
}

/* -------------------------------------------------------------- OMSChannelValid -- */

Boolean OMSChannelValid(OMSUniqueID device, INT16 channel)
{
	OMSNodeInfoListH nodeInfoListH;
	short channelMap;

	if (!gSignedIntoOMS) return FALSE;
	if (device == 0 || channel == 0) return FALSE;
	nodeInfoListH = OMSGet1NodeInfo(device);
	if ((*nodeInfoListH)->numNodes == 0) return FALSE;
	channelMap = (*(*nodeInfoListH)->info[0].deviceH)->midiChannels;
	OMSDisposeHandle(nodeInfoListH);
	return (BitTst(&channelMap, channel-1));
}

/* ----------------------------------------------------------------- OpenOMSInput -- */

OMSErr OpenOMSInput(OMSUniqueID device)
{
	if (device == 0) device = config.defaultInputDevice;
	if (device == 0) return FALSE;								/* ??FALSE is not an OMSErr */
	
	if (!gSignedIntoOMS) return OMSNotSignedIn;
	omsInputConnections.nodeUniqueID = device;
	omsInputConnections.appRefCon = 1; /* anything for now, returned in OMS Packets */
	
	return OMSOpenConnections(mySignature, InputPortID, 1, &omsInputConnections, FALSE);
}

/* ---------------------------------------------------------------- CloseOMSInput -- */

void CloseOMSInput(OMSUniqueID device)
{
	if (!gSignedIntoOMS) return;
	OMSCloseConnections(mySignature, InputPortID, 1, &omsInputConnections);
}


/* ----------------------------------------------------------- GetOMSPartPlayInfo -- */
/* Similar to the non-OMS GetPartPlayInfo. */

Boolean GetOMSPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
							Byte partPatch[], SignedByte partVelo[], short partIORefNum[],
							OMSUniqueID partDevice[])
{
	INT16 i; LINK partL; PARTINFO aPart;
	
	partL = FirstSubLINK(doc->headL);
	for (i = 0; i<=LinkNENTRIES(doc->headL)-1; i++, partL = NextPARTINFOL(partL)) {
		if (i==0) continue;					/* skip dummy partn = 0 */
		aPart = GetPARTINFO(partL);
		partVelo[i] = aPart.partVelocity;
		partChannel[i] = UseMIDIChannel(doc, i);
		partPatch[i] = aPart.patchNum;
		partTransp[i] = aPart.transpose;
		if (doc->polyTimbral || doc->omsInputDevice != 0)
			partDevice[i] = GetOMSDeviceForPartn(doc, i);
		else
			partDevice[i] = doc->omsInputDevice;

		/* Validate device / channel combination. */
		if (!OMSChannelValid(partDevice[i], (INT16)partChannel[i]))
			partDevice[i] = config.defaultOutputDevice;

		/* It's possible our device has changed, so validate again (and again, if necc.). */
		if (!OMSChannelValid(partDevice[i], (INT16)partChannel[i])) {
			partChannel[i] = config.defaultChannel;
			if (!OMSChannelValid(partDevice[i], (INT16)partChannel[i])) {
				if (CautionAdvise(NO_OMS_DEVS_ALRT)==1) return FALSE;			/* Cancel playback button (item 1 in this ALRT!) */
				partIORefNum[i] = OMSInvalidRefNum;
			}
		}
		else
			partIORefNum[i] = OMSUniqueIDToRefNum(partDevice[i]);
	}
	return TRUE;
}


/* ----------------------------------------------------------- GetOMSNotePlayInfo -- */
/* Given a note and tables of part transposition, channel, and offset velocity, return
the note's MIDI note number, including transposition; channel number; and velocity,
limited to legal range. Similar to the non-OMS GetNotePlayInfo. */

void GetOMSNotePlayInfo(
				Document *doc, LINK aNoteL, short partTransp[],
				Byte partChannel[], SignedByte partVelo[], short partIORefNum[],
				INT16 *pUseNoteNum, INT16 *pUseChan, INT16 *pUseVelo, short *puseIORefNum)
{
	INT16 partn;
	PANOTE aNote;

	partn = Staff2Part(doc,NoteSTAFF(aNoteL));
	*pUseNoteNum = UseMIDINoteNum(doc, aNoteL, partTransp[partn]);
	*pUseChan = partChannel[partn];
	aNote = GetPANOTE(aNoteL);
	*pUseVelo = doc->velocity+aNote->onVelocity;
	if (doc->polyTimbral) *pUseVelo += partVelo[partn];
	
	if (*pUseVelo<1) *pUseVelo = 1;
	if (*pUseVelo>MAX_VELOCITY) *pUseVelo = MAX_VELOCITY;
	*puseIORefNum = partIORefNum[partn];
}
