/* FreeMIDIUtils.c - FreeMIDI utilities for Nightingale by John Gibson, August 2001.
These were designed to be as parallel as possible to the API contained in OMSUtils.c,
though there are some differences. */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 2001 by Adept Music Notation Solutions, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* A shorter version of MIDIPacket, with corresponding fields. The <data> field 
is smaller than a MIDIMgr MIDIPacket or FreeMIDI MidiPacket, so be careful when
casting an FMSPacket to either of these that we don't access more than 4 bytes
of data. We call for 68K struct alignment here, because MIDIPackets are done
that way, and we cast FMSPacket to MIDIPacket in a few places. */
//#if TARGET_CPU_PPC
	#pragma options align=mac68k
//#endif
typedef struct FMSPacket {
	Byte	flags;
	Byte	len;	/* size of ENTIRE packet, including 6 bytes (MM_HDR_SIZE) for flags, len, & tStamp */
	long	tStamp;
	Byte	data[4];
} FMSPacket;
//#if TARGET_CPU_PPC
	#pragma options align=reset
//#endif

typedef struct FMSMidiBufferQEl {
	struct FMSMidiBufferQEl	*qLink;
	short							qType;
	FMSPacket					packet;
} FMSMidiBufferQEl, *FMSMidiBufferQElPtr;

/* exported globals */
long gmFMSBufferLength;				/* Our FMSPacket buffer size in bytes */
Boolean gFMSMIDIBufferFull;

/* private globals */
static Boolean gSignedIntoFMS = FALSE;
static short gFMSappID = -1;
static MidiInputQ *gFMSinputQ = NULL;
static int gInputErrorCount = 0;
static int gOutputErrorCount = 0;
static Boolean gRecording = FALSE;
static Boolean gAppOwnsBITimer = FALSE;
static Boolean gUseMIDIThru = TRUE;
static fmsUniqueID selectedInputDevice = noUniqueID;
static Byte selectedInputChannel = 0;
static fmsUniqueID fmsMidiThruDevice = noUniqueID;
static Byte fmsMidiThruChannel = 0;

static QHdrPtr fmsMidiInputQHdrPtr;
static QHdrPtr fmsMidiAvailQHdrPtr;
static FMSMidiBufferQElPtr buffElementPtr;
static FMSMidiBufferQElPtr lastBuffElementPtr;

/* local prototypes */
static Boolean AllocFMSBuffer(void);
static void DeallocFMSBuffer(void);
static short	FMSReadHook(long sourceID, MidiPacketPtr pakP, long refCon);
static short FMSWritePacket(fmsUniqueID device, short length, Byte data[], short priority);
static void DestChangeProc(fmsUniqueID uniqueID, short msgCode, long info, long refCon);



/*---------------------------------------------------------------- AllocFMSBuffer -- */
static Boolean AllocFMSBuffer(void)
{
	Size bytesFree, grow;
	INT16	i, numInputPackets;
	FMSMidiBufferQElPtr buffPtr;

	/* A single note takes two FMSPackets = 20 bytes in the buffer (for Note On and
		Note Off), while Nightingale's object list data structure as of v.3.0 takes
		30 bytes per note plus a great deal of overhead for the Sync, etc. See the
		the comment on AllocRawBuffer for logic behind our buffer size. */

	bytesFree = MaxMem(&grow);
	gmFMSBufferLength = bytesFree/4L;							/* Take a fraction of available memory */
	gmFMSBufferLength = n_min(gmFMSBufferLength, 100000L);	/* Limit its size to make NewPtr faster */

	if (gmFMSBufferLength<5000L) return FALSE;

	/* how many Qels can be made from gmFMSBufferLength */
	numInputPackets = (gmFMSBufferLength - (2 * sizeof(QHdr))) / sizeof(FMSMidiBufferQEl);
	
	fmsMidiInputQHdrPtr = (QHdrPtr)(NewPtr(sizeof(QHdr)));
	if (!GoodNewPtr((Ptr)fmsMidiInputQHdrPtr)) return FALSE;
	fmsMidiAvailQHdrPtr = (QHdrPtr)(NewPtr(sizeof(QHdr)));
	if (!GoodNewPtr((Ptr)fmsMidiAvailQHdrPtr)) return FALSE;
	gmFMSBufferLength = numInputPackets * sizeof(FMSMidiBufferQEl);
	buffElementPtr = (FMSMidiBufferQElPtr)(NewPtr(gmFMSBufferLength));
	if (!GoodNewPtr((Ptr)buffElementPtr)) return FALSE;
	
	/* initialize the QHdrs */
	fmsMidiInputQHdrPtr->qHead = NULL;
	fmsMidiInputQHdrPtr->qTail = NULL;
	fmsMidiAvailQHdrPtr->qHead = NULL;
	fmsMidiAvailQHdrPtr->qTail = NULL;
	
	/* push the buffElements into the Avail Queue */
	for (i = 0, buffPtr = buffElementPtr; i < numInputPackets; i++, buffPtr++) {
		Enqueue((QElemPtr)buffPtr, fmsMidiAvailQHdrPtr);
		;
	}

	return TRUE;
}


/*------------------------------------------------------------- DeallocFMSBuffer -- */
static void DeallocFMSBuffer(void)
{
	if (fmsMidiInputQHdrPtr) DisposePtr((Ptr)fmsMidiInputQHdrPtr);
	if (fmsMidiAvailQHdrPtr) DisposePtr((Ptr)fmsMidiAvailQHdrPtr);
	if (buffElementPtr) DisposePtr((Ptr)buffElementPtr);
}


/* ------------------------------------------------------------ GetFMSMIDIPacket -- */
/* ASSUMPTION: Since buffer element is returned to Queue, it is unlikely to be 
reused before caller has a chance to use the packet. */

MIDIPacket *GetFMSMIDIPacket(void)
{
	FMSMidiBufferQElPtr buffPtr;
	
	buffPtr = (FMSMidiBufferQElPtr)fmsMidiInputQHdrPtr->qHead;
	if (buffPtr) {
		Dequeue((QElemPtr)buffPtr, fmsMidiInputQHdrPtr);
		Enqueue((QElemPtr)buffPtr, fmsMidiAvailQHdrPtr);
		return (MIDIPacket *)&buffPtr->packet;
	}
	return NULL;
}


/* ----------------------------------------------------- PeekAtNextFMSMIDIPacket -- */
MIDIPacket *PeekAtNextFMSMIDIPacket(Boolean first)
{
	if (first)
		lastBuffElementPtr = (FMSMidiBufferQElPtr)fmsMidiInputQHdrPtr->qHead;
	else
		lastBuffElementPtr = lastBuffElementPtr->qLink;
		
	if (lastBuffElementPtr)
		return (MIDIPacket *)&lastBuffElementPtr->packet;
		
	return NULL;
}


/* -------------------------------------------------- DeletePeekedAtFMSMIDIPacket -- */
void DeletePeekedAtFMSMIDIPacket(void)
{
	if (lastBuffElementPtr) {
		Dequeue((QElemPtr)lastBuffElementPtr, fmsMidiInputQHdrPtr);
		Enqueue((QElemPtr)lastBuffElementPtr, fmsMidiAvailQHdrPtr);
	}
}


/* --------------------------------------------------------------- InitFMSBuffer -- */
void InitFMSBuffer(void)
{
	FMSMidiBufferQElPtr buffPtr, oldBuffPtr;
	OSErr err;
	
	/* move any old inputs back to avail queue */
	buffPtr = oldBuffPtr = (FMSMidiBufferQElPtr)fmsMidiInputQHdrPtr->qHead;
	while (buffPtr) {
		buffPtr = oldBuffPtr->qLink;
		Dequeue((QElemPtr)oldBuffPtr, fmsMidiInputQHdrPtr);
		Enqueue((QElemPtr)oldBuffPtr, fmsMidiAvailQHdrPtr);
	}

	gUseMIDIThru = (config.midiThru && fmsMidiThruDevice!=noUniqueID);
	gFMSMIDIBufferFull = FALSE;
	mRecIndex = 0;			/* For FreeMIDI, we use this as a packet counter only. */
}


/* --------------------------------------------------------------- FillFMSBuffer -- */
/* We do this here, instead of in FMSReadHook, because the latter is called at
interrupt time and is supposed to be as fast as possible. */

void FillFMSBuffer()
{
	MidiPacketPtr pakP;
	FMSMidiBufferQElPtr buffPtr, inputTail;
	long sourceID;
	short i, dataLen;

	/* Sanity check -- in case we receive MIDI before obtaining
	the input queue address */
	if (gFMSinputQ==NULL)
		return;

	if (gFMSMIDIBufferFull)
		return;

	/* Process any MidiPackets waiting in our queue */
	while ((pakP = ReadFMSInputQueue(gFMSinputQ, &sourceID)) != NULL) {
		if (pakP->len<MM_HDR_SIZE) {						/* packet is damaged? */
			Flush1FMSEvent(gFMSinputQ);
			return;
		}
		dataLen = pakP->len-MM_HDR_SIZE;
		buffPtr = (FMSMidiBufferQElPtr)fmsMidiAvailQHdrPtr->qHead;
		if (buffPtr) {										/* if buffer element avail */
			if (dataLen<=4) {								/* can fit in FMSPacket data field */
				/* Dequeue avail element and enqueue to input. */
				Dequeue((QElemPtr)buffPtr, fmsMidiAvailQHdrPtr);
				Enqueue((QElemPtr)buffPtr, fmsMidiInputQHdrPtr);
								
				/* Copy the input packet to the queue element. We've already
					time-stamped the packet in FMSReadHook. */
				buffPtr->packet.tStamp = pakP->tStamp;
				buffPtr->packet.flags = pakP->flags;
				buffPtr->packet.len = pakP->len;
				for (i = 0; i<dataLen; i++)
					buffPtr->packet.data[i] = pakP->data[i];
//say("pakP=0x%X tStamp=%ld data[0]=0x%x\n", pakP, buffPtr->packet.tStamp, buffPtr->packet.data[0]);
			}
		}
		else {
			recordingNow = FALSE;
			gFMSMIDIBufferFull = TRUE;
			FlushAllFMSEvents(gFMSinputQ);
			break;
		}

		/* We might want to reset the input queue if it keeps overflowing.
			gInputErrorCount is incremented in FMSReadHook */
		if (gInputErrorCount > 5) {
			FlushAllFMSEvents(gFMSinputQ);
			/* ??Might also want to pop an alert here... */
			break;
		}
		else /* otherwise, just go to the next packet */
			Flush1FMSEvent(gFMSinputQ);
	}
	gInputErrorCount = 0;
}


/* ---------------------------------------------------- FMSSetSelectedInputDevice -- */
void FMSSetSelectedInputDevice(fmsUniqueID device, INT16 channel)
{
	if (!gSignedIntoFMS) return;
	if (FMSChannelValid(device, channel)) {
		selectedInputDevice = device;
		selectedInputChannel = channel;
	}
	else {
		selectedInputDevice = config.defaultInputDevice;
		selectedInputChannel = config.defaultChannel;
	}
}


/* --------------------------------------------------------- FMSSetMIDIThruDevice -- */
void FMSSetMIDIThruDevice(fmsUniqueID device, INT16 channel)
{
	if (!gSignedIntoFMS) return;
	if (FMSChannelValid(device, channel)) {
		fmsMidiThruDevice = device;
		fmsMidiThruChannel = channel;
	}
	else {
		fmsMidiThruDevice = config.defaultOutputDevice;
		fmsMidiThruChannel = config.defaultOutputChannel;
	}
}


/* ----------------------------------------------- FMSSetMIDIThruDeviceFromConfig -- */
void FMSSetMIDIThruDeviceFromConfig()
{
	if (!gSignedIntoFMS) return;
	FMSSetMIDIThruDevice(config.thruDevice, (INT16)config.thruChannel);
	gUseMIDIThru = (config.midiThru && fmsMidiThruDevice!=noUniqueID);
}


/* ========================================================= Timing calls for FMS == */

#include "NTimeMgr.h"

void FMSInitTimer(void)
{
	gAppOwnsBITimer = TRUE;
	NTMInit();                              /* initialize timer */
}

void FMSLoadTimer(INT16 interruptPeriod)
{
	if (gAppOwnsBITimer)
		;												/* There's nothing to do here. */
}

void FMSStartTime(void)
{
	gAppOwnsBITimer = TRUE;
	NTMStartTimer(1);
}

long FMSGetCurTime(void)
{
	return NTMGetTime();
}

void FMSStopTime(void)
{
	if (gAppOwnsBITimer) {
		NTMStopTimer();							/* we're done with the millisecond timer */
		NTMClose();
		gAppOwnsBITimer = FALSE;
	}
}


/* ------------------------------------------------------- FMS End/Start Note Now -- */

void FMSEndNoteNow(INT16 noteNum, char channel, fmsUniqueID device)
{
	unsigned char	data[3];
	short				len = 3;
	short				priority = 7;

	if (device==noUniqueID) return;

	data[0] = MNOTEON+channel-1;
	data[1] = noteNum;
	data[2] = 0;			/* for note off */

	FMSWritePacket(device, len, data, priority);
}

void FMSStartNoteNow(INT16 noteNum, char channel, char velocity, fmsUniqueID device)
{
	unsigned char	data[3];
	short				len = 3;
	short				priority = 7;

	if (device==noUniqueID) return;

	data[0] = MNOTEON+channel-1;
	data[1] = noteNum;
	data[2] = velocity;

	FMSWritePacket(device, len, data, priority);
}


/* ----------------------------------------------------------- FMS Feedback setup -- */

void FMSFBOff(Document *doc)
{
	/* ??OMS stops timer, but I don't see why we need a timer for feedback. */
}

void FMSFBOn(Document *doc)
{
	/* ??OMS starts timer, but I don't see why we need a timer for feedback. */
}


/* ---------------------------------------------------------- FMSMIDIFBNoteOn/Off -- */

/* Start MIDI "feedback" note by sending a MIDI NoteOn command for the
specified note and channel. */

void FMSFBNoteOn(Document *doc, INT16 noteNum, INT16 channel, fmsUniqueID device)
{
	if (doc->feedback) {
		FMSStartNoteNow(noteNum, channel, config.feedbackNoteOnVel, device);
		SleepTicks(2L);
	}
}

/* End MIDI "feedback" note by sending a MIDI NoteOn command for the
specified note and channel with velocity 0 (which acts as NoteOff). */

void FMSFBNoteOff(Document *doc, INT16 noteNum, INT16 channel, fmsUniqueID device)
{
	if (doc->feedback) {
		FMSEndNoteNow(noteNum, channel, device);
		SleepTicks(2L);
	}
}


/* ----------------------------------------------------------- FMSMIDIOneProgram -- */
/* Send one patch change (and bank select commands if necessary). */

void FMSMIDIOneProgram(fmsUniqueID device, Byte channel,
						Byte patchNum, Byte bankSelect0, Byte bankSelect32)
{
	unsigned char	data[3];
	short				len;
	short				priority = 7;

	if (device==noUniqueID) return;
	if (patchNum==NO_PATCHNUM) return;

	if (bankSelect0!=NO_BANKSELECT) {
		len = 3;
		data[0] = MCTLCHANGE+channel-1;
		data[1] = 0;
		data[2] = bankSelect0;
		FMSWritePacket(device, len, data, priority);
	}
	if (bankSelect32!=NO_BANKSELECT) {
		len = 3;
		data[0] = MCTLCHANGE+channel-1;
		data[1] = 32;
		data[2] = bankSelect32;
		FMSWritePacket(device, len, data, priority);
	}
	len = 2;
	data[0] = MPGMCHANGE+channel-1;
	data[1] = patchNum;
	FMSWritePacket(device, len, data, priority);
#if 0
	say("FMSMIDIOneProgram sent: patchNum=%d bankSel0=%d bankSel32=%d over dev=%u, chan=%d\n",
				patchNum, bankSelect0, bankSelect32, device, channel);
#endif
}


/* -------------------------------------------------------------- FMSMIDIProgram -- */
/* Send a patch change (and bank select commands, if necessary) for each part. */

void FMSMIDIProgram(Document *doc)
{
	LINK			partL;
	PPARTINFO	pPart;

	if (!gSignedIntoFMS) return;

	if (doc->polyTimbral && !doc->dontSendPatches) {
		partL = NextPARTINFOL(FirstSubLINK(doc->headL));
		for ( ; partL; partL = NextPARTINFOL(partL)) {
			pPart = GetPPARTINFO(partL);
#ifdef CARBON_NOTYET
			FMSMIDIOneProgram(pPart->fmsOutputDevice, pPart->channel,
						pPart->patchNum, pPart->bankNumber0, pPart->bankNumber32);
#else
			FMSMIDIOneProgram(pPart->fmsOutputDevice, pPart->channel,
						pPart->patchNum, 0, 0);
#endif
		}
	}
}


/* -------------------------------------------------------------- FMSAllNotesOff -- */
void FMSAllNotesOff()
{
	FMSPanic();
}


/* ----------------------------------------------------------------- FMSReadHook -- */
/* This is called by FreeMIDI at interrupt time for each packet it receives. We
discard any packets we're not interested in, echo everything except program change
and bank select commands if MIDI thru is on, and store a timestamp for note data.
The <tStamp> field of an incoming MidiPacket will not be valid, unless we were to
use the FreeMIDI Timebase Manager. Instead we use our own time manager (in
NTimeMgr.c) and store the curren time into the <tStamp> field. (We can do this
because the packet passed to FMSReadHook, and stored in our input queue, is a
copy of the data FreeMIDI receives from the interface driver.)  */

static short FMSReadHook(long sourceID, MidiPacketPtr pakP, long refCon)
{
	Byte				command;
	short				retval;
	unsigned char	flags;
	long				oldA5;

	oldA5 = SetA5(refCon);									/* set up A5 world (for 68K only) */
	flags = pakP->flags;
	retval = fmsDiscardPacket;								/* default: ignore packet */
	if (flags & fmsMsgType)
		gInputErrorCount++;
	else if (flags & fmsContMask)
		;															/* donÕt echo or keep sysex */
	else if (pakP->data[0]<MCOMMANDMASK) {
		command = COMMAND(pakP->data[0]);

		if (gUseMIDIThru && fmsMidiThruDevice!=noUniqueID) {
			Boolean echo = TRUE;
			short priority = 3;
			if (command!=MPGMCHANGE) {						/* don't echo or keep program change */
				if (command==MCTLCHANGE) {
					Byte cntlNum = pakP->data[1];
					if (cntlNum==0 || cntlNum==32)		/* don't echo or keep bank select */
						echo = FALSE;
				}
				if (echo) {
					Byte statusByte = pakP->data[0];
					pakP->data[0] = command+fmsMidiThruChannel-1;	/* channelize */
					FMSWritePacket(fmsMidiThruDevice, pakP->len-MM_HDR_SIZE, pakP->data, priority);
					pakP->data[0] = statusByte;			/* restore */
				}
			}
		}

		if (recordingNow) {
			if (command==MNOTEON || command==MNOTEOFF) {
				pakP->tStamp = FMSGetCurTime();
				retval = fmsKeepPacket;
//say("FMSReadHook: tStamp=%ld\n", pakP->tStamp);
			}
		}
	}
	SetA5(oldA5);
	return retval;			/* NOTE: Don't return earlier, or we'll skip SetA5. */
}


/* -------------------------------------------------------------- FMSWritePacket -- */
static short FMSWritePacket(
		fmsUniqueID device, short length, Byte data[], short priority)
{
	short result;

	/* We loop here in case a call to FMSSendMidi returns outputQFull (1).
		FreeMIDI API docs say we can retry repeatedly in this case.  But if
		the result is negative, we shouldn't continue. */
	result = 1;
	while (result>0)
		result = FMSSendMidi(gFMSappID, (long)device, length, data, priority);
	if (result!=noErr)
		gOutputErrorCount++;

	return result;
}


/* -------------------------------------------------- FMSSetInputDestinationMatch -- */
/* Fill in the document's input destination record from its input device ID. */

void FMSSetInputDestinationMatch(Document *doc)
{
	OSErr	err;

	/* From FreeMIDI API docs...
		OSErr GetFMSSourceMatch(fmsUniqueID uniqueID, destinationMatchPtr matchP):
		GetFMSSourceMatch returns information in matchP about a MIDI source,
		which can be saved to file and used to obtain a matching source when the 
		file is reopened. Presently, this routine will return fmsInvalidNumberErr 
		if uniqueID is not an existant unique ID. (Exception: noErr will be returned 
		if GetFMSSourceMatch is called with noUniqueID.) In this case, the information 
		returned will not describe a valid source and calling MatchFMSSource with it 
		will return noUniqueID. */

	err = GetFMSSourceMatch(doc->fmsInputDevice, &doc->fmsInputDestination);
// ??And do we know this device sends on doc->channel?

	if (err!=noErr || doc->fmsInputDevice==noUniqueID) {
		/* ??What to do? */
	}
}


/* -------------------------------------------------- FMSGetInputDestinationMatch -- */
/* Given an input destination record for a document, get a matching device ID and
store this into the document. Returns a FreeMIDI matchLevel code, which the caller
can use to show an appropriate error message. */

short FMSGetInputDestinationMatch(Document *doc)
{
	short	matchLevel;

	/* From FreeMIDI API docs...
		fmsUniqueID MatchFMSSource(short fmsAppID, destinationMatchPtr matchP, short *matchLevel):
		Attempts to find the MIDI source which most closely matches that described 
		by matchP. Before calling this routine, set matchLevel to the desired match
		level, which will normally be deviceMatch. MatchFMSSource will always return
		a valid source unless none exist, in which case it will return noUniqueID.
		On return, matchLevel will describe how closely the returned uniqueID
		ID matches the information pointed to by matchP. */

	matchLevel = deviceMatch;
	doc->fmsInputDevice = MatchFMSSource(gFMSappID, &doc->fmsInputDestination, &matchLevel);

	if (matchLevel>deviceMatch) {
		/* Get a new destination match record for this device. */
		OSErr err = GetFMSSourceMatch(doc->fmsInputDevice, &doc->fmsInputDestination);
		if (err!=noErr || doc->fmsInputDevice==noUniqueID)
			;		/* ?? Not likely, but what to do? */
		doc->changed = TRUE;
	}

	return matchLevel;
}


/* ------------------------------------------------- FMSSetOutputDestinationMatch -- */
/* Fill in the part's output destination record from its output device ID. */

void FMSSetOutputDestinationMatch(Document *doc, LINK partL)
{
	PPARTINFO	pPart;
	OSErr			err;

PushLock(PARTINFOheap);
	pPart = GetPPARTINFO(partL);

	err = GetFMSDestinationMatch(pPart->fmsOutputDevice, &pPart->fmsOutputDestination);
// ??And do we know this device receives on pPart->channel?

	if (err!=noErr || pPart->fmsOutputDevice==noUniqueID) {
		/* ??What to do? */
	}
PopLock(PARTINFOheap);
}


/* ------------------------------------------------- FMSGetOutputDestinationMatch -- */
/* Given an output destination record for a part, see if there is an exact match
for the corresponding device ID in the current FreeMIDI configuration. If there
is not, FreeMIDI will return a device ID that should work. If <replace> is TRUE,
store this into the part. Returns a FreeMIDI matchLevel code, which the caller
can use to show an appropriate error message. If matchLevel is greater than
deviceMatch, it means the old device ID is no longer valid. */

short FMSGetOutputDestinationMatch(Document *doc, LINK partL, Boolean replace)
{
	LINK			pMPartL;
	PPARTINFO	pPart, pMPart;
	short			matchLevel, firstStaff;
	fmsUniqueID	device;
	OSErr			err;

PushLock(PARTINFOheap);
	pPart = GetPPARTINFO(partL);
	matchLevel = deviceMatch;
	device = MatchFMSDestination(gFMSappID,
											&pPart->fmsOutputDestination, &matchLevel);
	if (matchLevel>deviceMatch && replace) {
		/* Get a new destination match record for this device. */
		err = GetFMSDestinationMatch(device, &pPart->fmsOutputDestination);
		if (err!=noErr || device==noUniqueID)
			;		/* ?? Not likely, but what to do? Don't pop an alert. */
		pPart->fmsOutputDevice = device;
// ??How do we know that pPart->channel is valid with this device?
//say("FMSGetOutputDestinationMatch: replace: partL=%u dev=%u\n", partL, device);
		/* Copy new part data to master page part. */
		firstStaff = PartFirstSTAFF(partL);
		pMPartL = Staff2PartL(doc, doc->masterHeadL, firstStaff);
		pMPart = GetPPARTINFO(pMPartL);
		*pMPart = *pPart;
	}
PopLock(PARTINFOheap);
	return matchLevel;
}


/* ----------------------------------------------------- FMSCheckPartDestinations -- */
/* Check that all the parts of <doc> have output device ID's that match 
the current FreeMIDI configuration. If they don't, and if <replace> is
TRUE, let FreeMIDI find a suitable replacement for each, and store it
into the part. Return a FreeMIDI matchLevel code, which the caller can 
use to show an appropriate error message. */

short FMSCheckPartDestinations(Document *doc, Boolean replace)
{
	LINK	partL;
	short	matchLevel, thisMatchLevel;

	matchLevel = deviceMatch;
	partL = NextPARTINFOL(FirstSubLINK(doc->headL));
	for ( ; partL; partL = NextPARTINFOL(partL)) {
		thisMatchLevel = FMSGetOutputDestinationMatch(doc, partL, replace);
		if (thisMatchLevel>matchLevel)
			matchLevel = thisMatchLevel;
	}

	if (matchLevel>deviceMatch && replace)
		doc->changed = TRUE;

	return matchLevel;
}


/* ----------------------------------------------------- FMSSetNewDocRecordDevice -- */
void FMSSetNewDocRecordDevice(Document *doc)
{
	OSErr	err;

	(void)FMSCheckInputDevice(&doc->fmsInputDevice, &doc->channel, TRUE);

	/* Get a new destination match record for this device. */
	err = GetFMSSourceMatch(doc->fmsInputDevice, &doc->fmsInputDestination);
	if (err!=noErr || doc->fmsInputDevice==noUniqueID)
		;		/* ?? Not likely, but what to do? */
}


/* ---------------------------------------------------------- FMSSetNewPartDevice -- */
void FMSSetNewPartDevice(LINK partL)
{
	short					channel;
	fmsUniqueID			device;
	PPARTINFO			pPart;
	destinationMatch	destMatch;
	OSErr					err;

	if (!gSignedIntoFMS) return;

	/* Get a valid output device. */
	device = noUniqueID;
	channel = config.defaultChannel;
	(void)FMSCheckOutputDevice(&device, &channel, TRUE);

	/* Get a new destination match record for this device. */
	err = GetFMSDestinationMatch(device, &destMatch);
	if (err!=noErr || device==noUniqueID)
		;		/* ?? Not likely, but what to do? */

	pPart = GetPPARTINFO(partL);
	if (pPart==NILINK) return;
	pPart->channel = channel;
	pPart->fmsOutputDevice = device;
	pPart->fmsOutputDestination = destMatch;
}


/* ---------------------------------------------------------- FMSCheckInputDevice -- */
short FMSCheckInputDevice(fmsUniqueID *device, short *channel, Boolean replace)
{
	short					matchLevel;
	fmsUniqueID			aDevice;
	destinationMatch	destMatch;
	OSErr					err;

	matchLevel = deviceMatch;
	destMatch.basic.destinationType = 0;
	destMatch.basic.name[0] = 0;
	err = GetFMSSourceMatch(*device, &destMatch);
	if (err!=noErr || *device==noUniqueID) {
		/* <device> is invalid, so try to get something that works. */
		destMatch.basic.destinationType = 0;
		destMatch.basic.name[0] = 0;
		aDevice = MatchFMSSource(gFMSappID, &destMatch, &matchLevel);
		if (replace)
			*device = aDevice;
// ??How do we know that <channel> is valid for this device?
	}

	return matchLevel;
}


/* --------------------------------------------------------- FMSCheckOutputDevice -- */
short FMSCheckOutputDevice(fmsUniqueID *device, short *channel, Boolean replace)
{
	short					matchLevel;
	fmsUniqueID			aDevice;
	destinationMatch	destMatch;
	OSErr					err;

	if (!gSignedIntoFMS) return deviceMatch;

	matchLevel = deviceMatch;
	destMatch.basic.destinationType = 0;
	destMatch.basic.name[0] = 0;
	err = GetFMSDestinationMatch(*device, &destMatch);
	if (err!=noErr || *device==noUniqueID) {
		/* <device> is invalid, so try to get something that works. */
		destMatch.basic.destinationType = 0;
		destMatch.basic.name[0] = 0;
		aDevice = MatchFMSDestination(gFMSappID, &destMatch, &matchLevel);
		if (replace)
			*device = aDevice;
// ??How do we know that <channel> is valid for this device?
	}

	return matchLevel;
}


/* ---------------------------------------------------------- FMSCheckMetroDevice -- */
/* Check that the config's metronome output device ID is valid in the current
FreeMIDI configuration. If it isn't, and if <replace> is TRUE, let FreeMIDI
find a suitable replacement, and store it into the config. Return a FreeMIDI
matchLevel code, which the caller can use to show an appropriate error message. */

short FMSCheckMetroDevice(Boolean replace)
{
	short			matchLevel, channel;
	fmsUniqueID	device;

	if (!gSignedIntoFMS) return deviceMatch;

	device = config.metroDevice;
	channel = (short)config.metroChannel;
	matchLevel = FMSCheckOutputDevice(&device, &channel, replace);
	if (replace) {
		config.metroDevice = device;
		config.metroChannel = (SignedByte)channel;
	}

	return matchLevel;
}


/* ----------------------------------------------------------- FMSCheckThruDevice -- */
/* Check that the config's MIDI thru output device ID is valid in the current
FreeMIDI configuration. If it isn't, and if <replace> is TRUE, let FreeMIDI
find a suitable replacement, and store it into the config. Return a FreeMIDI
matchLevel code, which the caller can use to show an appropriate error message. */

short FMSCheckThruDevice(Boolean replace)
{
	short			matchLevel, channel;
	fmsUniqueID	device;

	if (!gSignedIntoFMS) return deviceMatch;

	device = config.thruDevice;
	channel = (short)config.thruChannel;
	matchLevel = FMSCheckOutputDevice(&device, &channel, replace);
	if (replace) {
		config.thruDevice = device;
		config.thruChannel = (SignedByte)channel;
	}

	return matchLevel;
}


/* ------------------------------------------------ FMSCheckDocOutputDestinations -- */
/* Check that all the parts of this document have output device ID's
that match the current FreeMIDI configuration. If they don't, let FreeMIDI
find a suitable replacement for each, and give an error message. */

void FMSCheckDocOutputDestinations(Document *doc)
{
	short		matchLevel;
	Boolean	replace;

	matchLevel = deviceMatch;

	/* We check the document twice, the first time without changing
		it. Then if matchLevel is no longer deviceMatch, we ask the user
		whether they want to change the devices in the document. */

	matchLevel = FMSCheckPartDestinations(doc, FALSE);

	/* This is pretty lame. It'd be much better to offer the user a 
		chance to remap devices, as in the high-end sequencers, but
		we don't have time to deal with this now... */
	if (matchLevel>nameMatch) {
		PlaceAlert(FMS_DOC_OUTPUTDEST_MISMATCH_ALRT, NULL, 0, 80);
		replace = (CautionAdvise(FMS_DOC_OUTPUTDEST_MISMATCH_ALRT)==OK);
		if (replace) {
			(void)FMSCheckPartDestinations(doc, TRUE);
		}
	}
}


/* ------------------------------------------------- FMSCheckAllInputDestinations -- */
/* Check that all open documents have input device ID's that match the current
FreeMIDI configuration. If they don't, let FreeMIDI find a suitable replacement
for each, and give an error message. NOTE: Unlike with FMSCheckAllOutputDestinations,
we don't give the user the option of replacing the device or leaving the original
one alone. */

void FMSCheckAllInputDestinations()
{
	Document	*doc;
	short		matchLevel, thisMatchLevel;

	matchLevel = deviceMatch;
	for (doc = documentTable; doc<topTable; doc++)
		if (doc->inUse && doc!=clipboard) {
			thisMatchLevel = FMSGetInputDestinationMatch(doc);
			if (thisMatchLevel>matchLevel)
				matchLevel = thisMatchLevel;
		}

	if (matchLevel>nameMatch) {
		PlaceAlert(FMS_INPUTDEST_MISMATCH_ALRT, NULL, 0, 80);
		StopInform(FMS_INPUTDEST_MISMATCH_ALRT);
	}
}


/* ------------------------------------------------ FMSCheckAllOutputDestinations -- */
/* Check that all the parts of all open documents have output device ID's
that match the current FreeMIDI configuration. If they don't, let FreeMIDI
find a suitable replacement for each, and give an error message. */

void FMSCheckAllOutputDestinations()
{
	Document	*doc;
	short		matchLevel, thisMatchLevel;
	Boolean	replace;

	matchLevel = deviceMatch;

	/* We check each document twice, the first time without changing
		them. Then if matchLevel is no longer deviceMatch, we ask the user
		whether they want to change the devices in the document(s). If
		they do, we loop through the docs again, this time changing them. */

	for (doc = documentTable; doc<topTable; doc++)
		if (doc->inUse && doc!=clipboard) {
			thisMatchLevel = FMSCheckPartDestinations(doc, FALSE);
			if (thisMatchLevel>matchLevel)
				matchLevel = thisMatchLevel;
		}

	/* This is pretty lame. It'd be much better to offer the user a 
		chance to remap devices, as in the high-end sequencers, but
		we don't have time to deal with this now... */
	if (matchLevel>nameMatch) {
		PlaceAlert(FMS_ALL_OUTPUTDEST_MISMATCH_ALRT, NULL, 0, 80);
		replace = (CautionAdvise(FMS_ALL_OUTPUTDEST_MISMATCH_ALRT)==OK);
		if (replace) {
			for (doc = documentTable; doc<topTable; doc++)
				if (doc->inUse && doc!=clipboard)
					(void)FMSCheckPartDestinations(doc, TRUE);
		}
	}
}


/* ------------------------------------------------ FMSCheckMetroThruDestinations -- */
/* Check that the global metronome and MIDI thru output device ID's match the 
current FreeMIDI configuration. If they don't, let FreeMIDI find a suitable 
replacement for each, and give an error message. */

static void FMSCheckMetroThruDestinations(void);
static void FMSCheckMetroThruDestinations()
{
	short		matchLevel, thisMatchLevel;

	matchLevel = deviceMatch;

	thisMatchLevel = FMSCheckMetroDevice(FALSE);
	if (thisMatchLevel>matchLevel)
		matchLevel = thisMatchLevel;

	thisMatchLevel = FMSCheckThruDevice(FALSE);
	if (thisMatchLevel>matchLevel)
		matchLevel = thisMatchLevel;

	if (matchLevel>nameMatch) {
		PlaceAlert(FMS_METROTHRUDEST_MISMATCH_ALRT, NULL, 0, 80);
		StopInform(FMS_METROTHRUDEST_MISMATCH_ALRT);
		(void)FMSCheckMetroDevice(TRUE);
		(void)FMSCheckThruDevice(TRUE);
	}
}


/* --------------------------------------------------------------- DestChangeProc -- */
/* A callback passed to FreeMIDI that gets called when the configuration is edited.
This will never get called at interrupt level, but it may get called when our
app is not the front process. */

static void
DestChangeProc(fmsUniqueID uniqueID, short msgCode, long info, long refCon)
{
	long	oldA5;

	switch (msgCode) {
		case setupChanged:
			/* We get this message when our application gets a resume event and
				something in the config has changed since we last suspended.  Since
				we always get this in the foreground, we can delay processing other
				messages until now if we depend on being the active process when
				doing so (for current heap zone, open resource files, etc). */
		case setupOpened:
			/* Like setupChanged, but we will get this (on resume only) when a new
				config has been opened since the last time we suspended. */
			
			/* After setting up our A5 world (for 68K only), validate our device
				assigment. */
			oldA5 = SetA5(refCon);
			FMSCheckMetroThruDestinations();
			FMSCheckAllInputDestinations();
			FMSCheckAllOutputDestinations();
			SetA5(oldA5);
			break;
		case destNewPatchSelected:
			/* New patch selected for the device.  info is a bitmask identifying
				the affected channels. */
//??Not sure what to do here.
			break;

		/* Messages we don't care about... */
		case destPatchListChanged:
			/* The patch list(s) for the device have changed.  info is a bitmask
				identifying the affected channels. */
			break;
		case destSourceDeleted:
			/* A MIDI source or destination was deleted. */
			break;
		case destRelocated:
			/* A destination (device, application, etc) was moved to a different
				location.  We would only care about this if we used "direct"
				destinations. */
			break;
		case sourceRelocated:
			/* A source was moved to a different location.  We would only care
				about this if we monitored data from particular devices. */
			break;
		case setupClosing:
			/* The user is in the process of closing the current config. */
			break;
		case freeMIDIModeChanged:
			/* The user has switched between fmsOnlyMode and fmsAllowOthersMode.
				info contains the new mode. */
			break;
	}
}


/* --------------------------------------------------------- OpenFMSConfiguration -- */
void OpenFMSConfiguration()
{
	OSErr err;

	err = EditFMSConfiguration(gFMSappID);
	if (err!=noErr)
		;	/* ??What to do? */
}


/* ------------------------------------------------------- SetFMSConfigMenuTitles -- */
/* Set titles of menu items for opening FreeMIDI Setup and interface dialog.
These names are the standards suggested in FreeMIDI API docs. Can't do this
in FMSInit, because menus haven't been setup yet. */

void SetFMSConfigMenuTitles()
{
	Str255 str;

	if (useWhichMIDI!=MIDIDR_FMS) return;

	GetIndString(str, MENUCMD_STRS, 25);				/* "Interface SettingsÉ" */
	SetMenuItemText(playRecMenu, PL_OpenExtMIDISetup1, str);
	GetIndString(str, MENUCMD_STRS, 26);				/* "Edit FreeMIDI ConfigurationÉ" */
	SetMenuItemText(playRecMenu, PL_OpenExtMIDISetup2, str);
}


/* ---------------------------------------------------------------------- FMSInit -- */
/* Initialize the FreeMIDI System. If FreeMIDI is installed, sign into it.
If we succeed, return TRUE, else FALSE. If FreeMIDI is not installed, just
return FALSE. */

Boolean FMSInit(void)
{
	fmsAppInfoRec	info;
	fmsInputFilter	filter;
	long				refcon, gestResp;
	OSErr				err;

	/* If configAppSig==(OSType)0, then user hasn't run FreeMIDI Setup; else
		if configAppRunning, then user is currently running FreeMIDI Setup.
		fileSpec->name contains name of current FreeMIDI setup document or
		an empty string if there is no such document. */
#ifdef CRASHES
	/* NOTE: GetFMSConfigurationApplication crashes hard, and the FreeMIDI API
		docs aren't consistent with the header files, so let's forget this. 
		And, to make things worse, GetFMSConfiguration dies also. Grrr... */
	FSSpec			fileSpec;
	OSType			configAppSig;
	Str255			configAppName;
	Boolean			configAppRunning;

	configAppSig = GetFMSConfigurationApplication(configAppName, &configAppRunning);
	err = GetFMSConfiguration(&fileSpec);
	if (err!=noErr || fileSpec.name[0]==0) {
		PlaceAlert(FMS_CONFIG_MISSING_ALRT, NULL, 0, 80);
		StopInform(FMS_CONFIG_MISSING_ALRT);
		return FALSE;
	}
#endif

	canLaunchFMSorOMSSetup = FALSE;
	if (Gestalt(gestaltOSAttr, &gestResp) == noErr)
		if (gestResp & (1<<gestaltLaunchCanReturn))
			canLaunchFMSorOMSSetup = TRUE;

	if (!AllocFMSBuffer()) {
		GetIndCString(strBuf, INITERRS_STRS, 9);		/* "Can't allocate buffer for MIDI" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	/* Initialize the fmsAppInfoRec. */
	info.signature = CREATOR_TYPE_NORMAL;					/* application signature */
	info.supportedVersion = FreeMIDICurrentVersion;		/* oldest version supported */

	/* Output queue sizes in bytes, highest priority first. High priority queues should
		be used for sync data or interrupt-level queueing. Applications that are more data
		intensive will probably want to create output queues of a few hundred bytes.
		Note that if you are queueing data from two or more independent sources (say
		one from poll level and one from interrupt level), you should have an equivalent
		number of output queues, so it is never possible to send a message (at interrupt
		level) while in the process of sending another (at poll level) to the same queue. */
	info.outputQSizes[0] = 0;		/* highest priority */
	info.outputQSizes[1] = 0;
	info.outputQSizes[2] = 0;
	info.outputQSizes[3] = 256;
	info.outputQSizes[4] = 0;
	info.outputQSizes[5] = 0;
	info.outputQSizes[6] = 0;
	info.outputQSizes[7] = 0;		/* lowest priority */

	/* Each application gets one input queue, whose size is in packets. If this is set
		to zero or less, the application will not receive MIDI input. A size of twenty
		or so is fine for most applications. */
	info.inputQSize = 20;

#ifdef CARBON_NOTYET
	refcon = (long)LMGetCurrentA5();
#else
	refcon = 0L;
#endif

	info.methods.rxProc = NewFMRxProc(FMSReadHook);
	info.methods.rxProcRefCon = refcon;

	/* Set up our callback for detecting changes to the FreeMIDI configuration. */
	info.methods.destinationChangeProc = NewFMDestChangeProc(DestChangeProc);
	info.methods.destProcRefCon = refcon;

	info.methods.applTimingHook = NULL;			/* We don't implement transport controls. */
	info.methods.timingHookRefCon = 0;
	
	/* Give FreeMIDI our application name and the fmsAppInfoRec;
		it returns our application ID in gFMSappID. Then store a pointer
		to the input queue. */
	err = FreeMidiSignIn(LMGetCurApName(), &gFMSappID, &info);
	if (err==noErr) {
		gFMSinputQ = GetFMSInputQueue(gFMSappID);
		if (gFMSinputQ==NULL)
			return FALSE;
		gSignedIntoFMS = TRUE;

/* ??But if we're doing MIDI thru, we need to echo almost everything! */
		/* Install our packet filter. We only want note data. */
		filter.data.noteOff = 0x0000;					/* receive on all channels */
		filter.data.noteOn = 0x0000;
		filter.data.polyPressure = 0x0000;
		filter.data.controlChange = 0x0000;
		filter.data.patchChange = 0xFFFF;			/* filter this out (because of MIDI thru) */
		filter.data.chanPressure = 0x0000;
		filter.data.pitchBend = 0x0000;
		filter.data.systemMessage = 0xFFFF;

		SetFMSInputFilter(gFMSappID, &filter);

		recordingNow = FALSE;

		(void)FMSCheckMetroDevice(TRUE);
		(void)FMSCheckThruDevice(TRUE);
		FMSSetMIDIThruDeviceFromConfig();

		return TRUE;
	}
	else {
		/* Something went wrong and we couldn't sign in.  We could be smarter
			about handling the error code from FreeMidiSignIn.
			 -	fmsDuplicateIDErr means your application (or another with the
				same signature) is already signed in.
			 -	fmsInvalidVersionErr means your supportedVersion argument contained 
				a version that was too old or the installed version of FreeMIDI is
				older than your application expects. */
		if (fmsDuplicateIDErr) {
//FIXME:
		}
	}

	return FALSE;
}


/* ------------------------------------------------------------------- FMSClose -- */
Boolean FMSClose(void)
{
	if (useWhichMIDI!=MIDIDR_FMS)
		return TRUE;

	if (gSignedIntoFMS) {
		FMSStopTime();												/* in case it's still on */
		FreeMidiSignOut(gFMSappID);
		gSignedIntoFMS = FALSE;
	}
	
	DeallocFMSBuffer();

	return TRUE;
}


/* -------------------------------------------------------------- FMSResumeEvent -- */
void FMSResumeEvent(void)
{
	if (gSignedIntoFMS) {
		FMSSuspendResume(gFMSappID, TRUE);
	}
}


/* ------------------------------------------------------------- FMSSuspendEvent -- */
void FMSSuspendEvent(void)
{
	if (gSignedIntoFMS) {
		FMSSuspendResume(gFMSappID, FALSE);
	}
}


/* -------------------------------------------------------------------------------- */
/* Functions for handling the FreeMIDI popup menus that let the user choose
device, channel and patch. */

/* ----------------------------------------------------------- DrawFMSDeviceMenu -- */
/* Draw the FreeMIDI device popup menu having the given bounds rect, device ID
and (one-based) channel number. */

void DrawFMSDeviceMenu(
		const Rect			*boundsRect,
		const fmsUniqueID	device,
		const short			channel)
{
	menuSelectionRec	menuRec;

	if (!gSignedIntoFMS) return;

	menuRec.menuBounds = *boundsRect;
	menuRec.fmsAppID = gFMSappID;
	menuRec.classification = fmsAllChOutputs;
	menuRec.menuFlags = shadedBoxFlag | pullDownFlag;
	menuRec.selectionID = (long)device;
	menuRec.selectionInfo = channel-1;
	menuRec.selectionName[0] = 0;		/* Make FreeMIDI look up the name for us. */

	DrawFMSPopupMenu(&menuRec);
}

/* ------------------------------------------------------------ DrawFMSPatchMenu -- */
/* Draw the FreeMIDI patch popup menu having the given bounds rect, device ID
and (one-based) channel number. */

void DrawFMSPatchMenu(
		const Rect			*boundsRect,
		const fmsUniqueID	device,
		const short			channel)
{
	menuSelectionRec	menuRec;

	if (!gSignedIntoFMS) return;

	menuRec.menuBounds = *boundsRect;
	menuRec.fmsAppID = gFMSappID;
	menuRec.classification = fmsChannelPatches;
	menuRec.menuFlags = shadedBoxFlag | pullDownFlag;
	menuRec.selectionID = (long)device;
	menuRec.selectionInfo = channel-1;
	menuRec.selectionName[0] = 0;		/* Make FreeMIDI look up the name for us. */

	DrawFMSPopupMenu(&menuRec);
}


/* ------------------------------------------------------------ RunFMSDeviceMenu -- */
/* Respond to a click in a FreeMIDI device popup menu having the given bounds
rect, device ID and (one-based) channel number. Returns TRUE if user made a
new selection; FALSE if they didn't, or if we haven't signed into FreeMIDI.
Passes back the new device and (one-based) channel number. */

Boolean RunFMSDeviceMenu(
		const Rect		*boundsRect,
		fmsUniqueID		*device,
		short				*channel)
{
	menuSelectionRec	menuRec;
	Boolean				newSelection = FALSE;

	if (!gSignedIntoFMS) return FALSE;

	menuRec.menuBounds = *boundsRect;
	menuRec.fmsAppID = gFMSappID;
	menuRec.classification = fmsAllChOutputs;
	menuRec.menuFlags = shadedBoxFlag | pullDownFlag;
	menuRec.selectionID = (long)*device;
	menuRec.selectionInfo = *channel-1;
	menuRec.selectionName[0] = 0;		/* Make FreeMIDI look up the name for us. */

	if (RunFMSPopupMenu(&menuRec, NULL, 0)) {
		DrawFMSPopupMenu(&menuRec);
		*device = (fmsUniqueID)menuRec.selectionID;
		*channel = menuRec.selectionInfo+1;
		newSelection = TRUE;
	}
	
	return newSelection;
}


/* ------------------------------------------------------------- RunFMSPatchMenu -- */
/* Respond to a click in a FreeMIDI patch popup menu having the given bounds
rect, device ID and (one-based) channel number. Returns TRUE if user made a
new selection; FALSE if they didn't, or if we haven't signed into FreeMIDI. */

Boolean RunFMSPatchMenu(
		const Rect			*boundsRect,
		const fmsUniqueID	device,
		const short			channel)
{
	menuSelectionRec	menuRec;
	Boolean				newSelection = FALSE;

	if (!gSignedIntoFMS) return FALSE;

	menuRec.menuBounds = *boundsRect;
	menuRec.fmsAppID = gFMSappID;
	menuRec.classification = fmsChannelPatches;
	menuRec.menuFlags = shadedBoxFlag | pullDownFlag;
	menuRec.selectionID = (long)device;
	menuRec.selectionInfo = channel-1;
	menuRec.selectionName[0] = 0;		/* Make FreeMIDI look up the name for us. */

	if (RunFMSPopupMenu(&menuRec, NULL, 0)) {
		DrawFMSPopupMenu(&menuRec);
		newSelection = TRUE;
	}
	
	return newSelection;
}


/* ------------------------------------------------------ GetFMSCurrentPatchInfo -- */
/* Given a device and (one-based) channel number, return MIDI information
about the current patch for that device and channel: the patch number, the
value for the bank select 0 controller, and the value for the bank select 32
controller.

From the FreeMIDI API docs:

"BankNumber0, bankNumber32, and patchNumber are used to hold the MIDI bank
and patch numbers of the program change in question; a value of hex FF means
the bank and/or patch is unknown or irrelevant. (If the device uses
controller zero for bank select, the bank number is in bankNumber0; if it
uses controller 32, its bank number is in bankNumber32. An application can
determine what bank select controller(s) a device uses by calling
GetFMSDeviceInfo and checking the kBankSelect0 and kBankSelect32 bits of
the standardProperties. Many devices use one or the other exclusively, while
some use both.) The name field is used to hold a string which is either
the bank or patch name."

We go ahead and store these 0xFF's in the part data structure.  Code that
actually uses them to send patches should check.
*/

Boolean GetFMSCurrentPatchInfo(
		const fmsUniqueID	device,
		const short			channel,					/* one-based */
		short					*patchNum,
		short					*bankNumber0,
		short					*bankNumber32
		)
{
	short						bankRefNum;			/* we don't consult this */
	destinationPatchRec	patchRec;

	bankRefNum = GetFMSCurrentPatch(device, channel-1, &patchRec);

	*patchNum = (short)patchRec.patchNumber;
	*bankNumber0 = (short)patchRec.bankNumber0;
	*bankNumber32 = (short)patchRec.bankNumber32;

	return TRUE;
}


/* ---------------------------------------------------------- GetFMSDeviceForPart -- */

fmsUniqueID GetFMSDeviceForPartn(Document *doc, INT16 partn)
{
	LINK partL;

	if (!doc) doc = currentDoc;
	partL = Partn2PartL(doc, partn);
	if (partL)
		return GetFMSDeviceForPartL(doc, partL);
	return noUniqueID;		/* an error */
}

/* NB: assumes partL is valid */
fmsUniqueID GetFMSDeviceForPartL(Document *doc, LINK partL)
{
	PPARTINFO pPart = GetPPARTINFO(partL);
	return pPart->fmsOutputDevice;
}

/* ---------------------------------------------------------- SetFMSDeviceForPart -- */

void SetFMSDeviceForPartn(Document *doc, INT16 partn, fmsUniqueID device)
{
	LINK partL;

	if (!doc) doc = currentDoc;
	partL = Partn2PartL(doc, partn);
	if (partL)
		SetFMSDeviceForPartL(doc, partL, device);
}

/* NB: assumes partL is valid */
void SetFMSDeviceForPartL(Document *doc, LINK partL, fmsUniqueID device)
{
	PPARTINFO pPart = GetPPARTINFO(partL);
	pPart->fmsOutputDevice = device;
}


/* -------------------------------------------------------------- FMSChannelValid -- */

Boolean FMSChannelValid(
		fmsUniqueID device,
		INT16 channel)			/* one-based channel number */
{
	if (!gSignedIntoFMS) return FALSE;

	if (device==noUniqueID || (channel<1 || channel>16))
		return FALSE;

	return TRUE;
}


/* ----------------------------------------------------------- GetFMSPartPlayInfo -- */

Boolean GetFMSPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
							Byte partPatch[], Byte partBankNum0[], Byte partBankNum32[],
							SignedByte partVelo[], fmsUniqueID partDevice[])
{
	INT16 i; LINK partL; PARTINFO aPart;
	
	partL = FirstSubLINK(doc->headL);
	for (i = 0; i<=LinkNENTRIES(doc->headL)-1; i++, partL = NextPARTINFOL(partL)) {
		if (i==0) continue;					/* skip dummy partn = 0 */
		aPart = GetPARTINFO(partL);
		partVelo[i] = aPart.partVelocity;
		partChannel[i] = UseMIDIChannel(doc, i);
		partPatch[i] = aPart.patchNum;
#ifdef CARBON_NOTYET
		partBankNum0[i] = aPart.bankNumber0;
		partBankNum32[i] = aPart.bankNumber32;
#else
		partBankNum0[i] = 0;
		partBankNum32[i] = 0;
#endif
		partTransp[i] = aPart.transpose;

		if (doc->polyTimbral || doc->fmsInputDevice != noUniqueID)
			partDevice[i] = GetFMSDeviceForPartn(doc, i);
		else
			partDevice[i] = doc->fmsInputDevice;

#ifdef NOTYET
		/* Validate device / channel combination. */
		if (!FMSChannelValid(partDevice[i], (INT16)partChannel[i]))
			partDevice[i] = config.defaultOutputDevice;

		/* It's possible our device has changed, so validate again (and again, if necc.). */
		if (!FMSChannelValid(partDevice[i], (INT16)partChannel[i])) {
			partChannel[i] = config.defaultChannel;
			if (!FMSChannelValid(partDevice[i], (INT16)partChannel[i])) {
				if (CautionAdvise(NO_OMS_DEVS_ALRT)==1) return FALSE;			/* Cancel playback button (item 1 in this ALRT!) */
				partIORefNum[i] = OMSInvalidRefNum;
			}
		}
		else
			partIORefNum[i] = OMSUniqueIDToRefNum(partDevice[i]);
#endif
	}
	return TRUE;
}


/* ----------------------------------------------------------- GetFMSNotePlayInfo -- */
/* Given a note and tables of part transposition, channel, and offset velocity, return
the note's MIDI note number, including transposition; channel number; and velocity,
limited to legal range. */

void GetFMSNotePlayInfo(
				Document *doc, LINK aNoteL, short partTransp[],
				Byte partChannel[], SignedByte partVelo[], fmsUniqueID partDevice[],
				INT16 *pUseNoteNum, INT16 *pUseChan, INT16 *pUseVelo, fmsUniqueID *puseDevice)
{
	INT16 partn;
	PANOTE aNote;

	partn = Staff2Part(doc, NoteSTAFF(aNoteL));
	*pUseNoteNum = UseMIDINoteNum(doc, aNoteL, partTransp[partn]);
	*pUseChan = partChannel[partn];
	aNote = GetPANOTE(aNoteL);
	*pUseVelo = doc->velocity+aNote->onVelocity;
	if (doc->polyTimbral) *pUseVelo += partVelo[partn];
	
	if (*pUseVelo<1) *pUseVelo = 1;
	if (*pUseVelo>MAX_VELOCITY) *pUseVelo = MAX_VELOCITY;
	*puseDevice = partDevice[partn];
}

