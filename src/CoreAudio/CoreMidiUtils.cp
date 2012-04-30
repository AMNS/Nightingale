/* 
 * CoreMIDIUtils.c
 * 
 * Implementation of OMSUtils functionality for CoreMIDI
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "NTimeMgr.h"

#define _CORE_MIDIGLOBALS_

#include "CoreMidiDefs.h"
#include "MidiMap.h"

static long MIDIPacketSize(int len);

static Boolean CMIsNoteOnPacket(MIDIPacket *p);
static Boolean CMIsNoteOffPacket(MIDIPacket *p);
static Boolean CMIsActiveSensingPacket(MIDIPacket *p);

static Boolean AllocCMPacketList(void);

static MIDIEndpointRef GetMIDIEndpointByID(MIDIUniqueID id);

static unsigned long defaultOutputDev;

#define CMDEBUG 1			

// -------------------------------------------------------------------------------------
// Midi Callbacks

/* -------------------------------------------------------------- NightCMReadProc -- */

static long MIDIPacketSize(int len)
{
	return CMPKT_HDR_SIZE + len;
}

static Boolean CMEndOfBuffer(MIDIPacket *pkt, int len)
{
	return ((char*)pkt + MIDIPacketSize(len)) > 
				((char*)gMIDIPacketListBuf + kCMBufLen);
}

static Boolean CMAcceptPacket(MIDIPacket *pkt)
{
	if (CMIsNoteOnPacket(pkt)) return TRUE;
	
	if (CMIsNoteOffPacket(pkt)) return TRUE;
	
	return FALSE;
}

static void	NightCMReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
//	if (CMEndOfBuffer()) return;
		
	MIDIPacket *packetToAdd = gCurrPktListEnd;
	
	if (gOutPort != NULL && gDest != NULL) {
		MIDIPacket *packet = (MIDIPacket *)pktlist->packet;	// remove const (!)
		unsigned int j = 0;
		for ( ; j < pktlist->numPackets && !CMEndOfBuffer(MIDIPacketNext(packetToAdd), packet->length); j++) {
		
			// Only take packets that are acceptable to us.
			// For example, active sensing is sent once every 300 Milliseconds
			
			if (CMAcceptPacket(packet)) {
				packetToAdd = MIDIPacketListAdd(gMIDIPacketList, sizeof(gMIDIPacketListBuf), packetToAdd, 
																packet->timeStamp, packet->length, packet->data);
						
			}
			packet = MIDIPacketNext(packet);
		}
	}
	
	gCurrPktListEnd = packetToAdd;
}


// -------------------------------------------------------------------------------------
// Midi Packets

/* ------------------------------------------------------------ AllocCMPacketList -- */

static Boolean AllocCMPacketList(void)
{
	gMIDIPacketList = (MIDIPacketList *)gMIDIPacketListBuf;

	gCurrentPacket = gCurrPktListBegin = gCurrPktListEnd = MIDIPacketListInit(gMIDIPacketList);
	
	return (gCurrentPacket != NULL);
}

/* ------------------------------------------------------------ ResetMIDIPacketList -- */
/* Same implementation as AllocCMPacketList. Fulfill different functions for which
 * implementation will diverge when we dynamically allocate the buffer.
 */

Boolean ResetMIDIPacketList()
{
	gMIDIPacketList = (MIDIPacketList *)gMIDIPacketListBuf;

	gCurrentPacket = gCurrPktListBegin = gCurrPktListEnd = MIDIPacketListInit(gMIDIPacketList);
	
	return (gCurrentPacket != NULL);
}

Boolean CMCurrentPacketValid()
{
	return (gCurrentPacket != NULL && 
				gCurrentPacket < gCurrPktListEnd &&
				!CMIsActiveSensingPacket(gCurrentPacket));
}

void SetCMMIDIPacket()
{
	gCurrentPacket = gCurrPktListBegin;
}

void ClearCMMIDIPacket()
{
	gCurrentPacket = NULL;
}

/* ------------------------------------------------------------ GetCMMIDIPacket -- */
/* ASSUMPTION, since buffer element is returned to Queue, it is unlikely to be reused
  before caller has a chance to use the packet */
  
MIDIPacket *GetCMMIDIPacket(void)
{
	MIDIPacket *pmPkt = NULL;
	
	if (CMCurrentPacketValid()) {
		pmPkt = gCurrentPacket;
		gCurrentPacket = MIDIPacketNext(gCurrentPacket);
	}
	
	return pmPkt;
}

/* ----------------------------------------------------- PeekAtNextOMSMIDIPacket -- */

MIDIPacket *PeekAtNextCMMIDIPacket(Boolean first)
{
	if (!CMCurrentPacketValid())
		gPeekedPkt = NULL;
	else if (first)
		gPeekedPkt = gCurrentPacket;
	else if (!CMIsActiveSensingPacket(gPeekedPkt))
		gPeekedPkt = MIDIPacketNext(gPeekedPkt);
	else
		gPeekedPkt = NULL;
		
	return gPeekedPkt;

/*			
	if (CMValidCurrentPacket()) {
		if (first) {
			gPeekedPkt = gCurrentPacket;
		}
		else if (!IsActiveSensingPacket(gPeekedPkt)) {
			gPeekedPkt = MIDIPacketNext(gPeekedPkt);
		}
		else {
			gPeekedPkt = NULL;
		}
		
	}
	else {
		gPeekedPkt = NULL;
	}
		
	return gPeekedPkt;
*/
}

/* -------------------------------------------------- DeletePeekedAtOMSMIDIPacket -- */

void DeletePeekedAtCMMIDIPacket(void)
{
	if (gPeekedPkt != NULL) {
		gPeekedPkt->data[0] = 0;
	}
}

// -------------------------------------------------------------------------------------
// Packet Types

static Boolean CMIsNoteOnPacket(MIDIPacket *p)
{
	Byte command = p->data[0] & MCOMMANDMASK;
	return (p->length == kNoteOnLen && command == MNOTEON);
}

static Boolean CMIsNoteOffPacket(MIDIPacket *p)
{
	Byte command = p->data[0] & MCOMMANDMASK;
	return (p->length == kNoteOffLen && command == MNOTEOFF);
}

static Boolean CMIsActiveSensingPacket(MIDIPacket *p)
{
	return (p->length == kActiveSensingLen && p->data[0] == kActiveSensingData);
}

/* -------------------------------------------------- DeletePeekedAtOMSMIDIPacket -- */

static MIDIPacket *AddActiveSensingPacket(MIDIPacket *p)
{
	MIDIPacket mPkt;
	
	mPkt.timeStamp = 0;
	mPkt.length = kActiveSensingLen;
	mPkt.data[0] = kActiveSensingData;

	p = MIDIPacketListAdd(gMIDIPacketList, sizeof(gMIDIPacketListBuf), p, 
									mPkt.timeStamp, mPkt.length, mPkt.data);
													
	return p;
}

/* --------------------------------------------------------- CMTimeStampToMillis -- */

long CMTimeStampToMillis(MIDITimeStamp timeStamp)
{
	UInt64 tsNanos = AudioConvertHostTimeToNanos(timeStamp);
	
	UInt64 tsMillis = tsNanos / kNanosToMillis;
	
	long tsLongMillis = (long)tsMillis;
	
	return tsLongMillis;
} 

long CMGetHostTimeMillis()
{
	UInt64 tsNanos = AudioGetCurrentHostTime();

	long hostMillis = CMTimeStampToMillis(AudioGetCurrentHostTime());
	return hostMillis;
}

/* -------------------------------------------------- DeletePeekedAtOMSMIDIPacket -- */

void CMNormalizeTimeStamps()
{
	MIDITimeStamp firstTimeStamp;
	Boolean haveTimeStamp = false;
	
	MIDIPacket *packet = gCurrPktListBegin;

	while (packet != gCurrPktListEnd) {
	
		if (CMIsNoteOnPacket(packet) || CMIsNoteOffPacket(packet) ) {
			if (!haveTimeStamp) {
				haveTimeStamp = TRUE;
				firstTimeStamp = packet->timeStamp;
				packet->timeStamp = 0;
			}
			else {
				packet->timeStamp -= firstTimeStamp;
			}
		}
		packet = MIDIPacketNext(packet);
	}
} 

/* -------------------------------------------------- CloseCoreMidiInput -- */

void CloseCoreMidiInput(void)
{
	gCurrPktListEnd = AddActiveSensingPacket(gCurrPktListEnd);
}


// -------------------------------------------------------------------------------------
// Write packet list (for single note on / note offs.

/* -------------------------------------------------------- AllocCMWritePacketList -- */

static Boolean AllocCMWritePacketList(void)
{
	gMIDIWritePacketList = (MIDIPacketList *)gMIDIWritePacketListBuf;

	gCurrentWritePacket = MIDIPacketListInit(gMIDIWritePacketList);
	
	return (gCurrentWritePacket != NULL);
}

Boolean ResetMIDIWritePacketList()
{
	gMIDIWritePacketList = (MIDIPacketList *)gMIDIWritePacketListBuf;

	gCurrentWritePacket = MIDIPacketListInit(gMIDIWritePacketList);
	
	return (gCurrentWritePacket != NULL);
}



// ----------------------------------------------------------- Timing calls for CoreMidi
// Use the Nightingale Time Manager.

void CMInitTimer(void)
{
	//OMSClaimTimer(myClientID);
	
	appOwnsBITimer = TRUE;
	NTMInit();                              /* initialize timer */
}

void CMLoadTimer(INT16 interruptPeriod)
{
	if (appOwnsBITimer) {
			;												/* There's nothing to do here. */
	}
}

void CMStartTime(void)
{
	//OMSClaimTimer(myClientID);
	
	appOwnsBITimer = TRUE;
	NTMStartTimer(1);
}

long CMGetCurTime(void)
{
	long time;

	time = NTMGetTime();								/* use built-in */

	return time;
}

void CMStopTime(void)
{
	if (appOwnsBITimer) {						/* use built-in */
		NTMStopTimer();							/* we're done with the millisecond timer */
		NTMClose();
		appOwnsBITimer = FALSE;
	}
}



static void InitCoreMidiTimer()
{
	CMInitTimer();
}

// ----------------------------------------------------------- Setup and teardown for CoreMidi
// Setup for playback and teardown after playback

static void CMGetUsedChannels(Document *doc, Byte *partChannel, Byte *activeChannel)
{
	for (int i = 0; i < MAXCHANNEL; i++)
		activeChannel[i] = 0;
		
	for (int j = 1; j < LinkNENTRIES(doc->headL); j++) {
		int channel = partChannel[j];
		
		if (channel >= 0 && channel < MAXCHANNEL)
			activeChannel[channel] = 1;
	}
}

void CMSetup(Document *doc, Byte *partChannel)
{
	CMInitTimer();
	
	if (gCMNoDestinations)
	{
		Byte activeChannel[MAXCHANNEL];
		
		CMGetUsedChannels(doc, partChannel, activeChannel);
		Boolean ok = SetupSoftMIDI(activeChannel);
		
		if (ok)
		{
			gCMSoftMIDIActive = TRUE;
		}
	}
}


void CMTeardown(void)
{
	if (gCMNoDestinations)
	{
		if (gCMSoftMIDIActive)
		{
			TeardownSoftMIDI();
		
			gCMSoftMIDIActive = FALSE;
		}
	}
}


// -------------------------------------------------------------------------------------
// Sending Notes

// Need a 1 packet long list to send
// 	Set the packet's timeStamp to zero [= now, see Core Midi Doc, MIDIPacket]
//		Call MIDISend
//
// Need separate packet list (not gMIDIPacketList)
//		That way we don't have to alloc and init a separate packet list inside these
//			routines
//		Add 1 packet to it
//		Call MIDISend
//		Make sure that packet is released
//
// Need to figure out how to set up the packet

//  -------------------------------------------------------------- CM End/Start Note Now 


OSStatus CMWritePacket(MIDIUniqueID destDevID, MIDITimeStamp tStamp, UInt16 pktLen, Byte *data)
{
	OSStatus err = noErr;
	MIDIEndpointRef destEndPt = NULL;
	
	if (gCMSoftMIDIActive) {
	
		err = SoftMIDISendMIDIEvent(tStamp, pktLen, data);
		
		return err;
	}
	
	destEndPt = GetMIDIEndpointByID(destDevID);
	
	if (destEndPt != NULL) {
		gCurrentWritePacket = MIDIPacketListAdd(gMIDIWritePacketList, 
																sizeof(gMIDIWritePacketListBuf), 
																gCurrentWritePacket, tStamp, pktLen, data);

		err = MIDISend(gOutPort, destEndPt, gMIDIWritePacketList);
										 /*gDest*/
	}
	else {
		err = paramErr;
	}
	
	if (err == noErr) {
		Boolean ok = ResetMIDIWritePacketList();
		
		if (!ok) err = memFullErr;
	}

	return err;
}

OSStatus CMEndNoteNow(MIDIUniqueID destDevID, INT16 noteNum, char channel)
{
	Byte noteOff[] = { 0x90, 60, 0 };
	MIDITimeStamp tStamp = 0;				// Indicates perform NOW.

	noteOff[0] |= channel;
	noteOff[1] = noteNum;
	OSStatus err = CMWritePacket(destDevID, tStamp, 3, noteOff);

	return err;
}

OSStatus CMStartNoteNow(MIDIUniqueID destDevID, INT16 noteNum, char channel, char velocity)
{
	Byte noteOn[] = { 0x90, 60, 64 };
	MIDITimeStamp tStamp = 0;				// Indicates perform NOW.

	noteOn[0] |= channel;
	noteOn[1] = noteNum;
	noteOn[2] = velocity;
	
	OSStatus err = CMWritePacket(destDevID, tStamp, 3, noteOn);

	return err;
}


//  ------------------------------------------------------------------------ CM Feedback 

static void CMFBNoteOnDevID(Document *doc, INT16 noteNum, INT16 channel, MIDIUniqueID devID);
static void CMFBNoteOffDevID(Document *doc, INT16 noteNum, INT16 channel, MIDIUniqueID devID);

void CMFBOff(Document *doc)
{
	if (doc->feedback) {
		if (appOwnsBITimer) {					/* use built-in */
			CMStopTime();
		}
	}
}

void CMFBOn(Document *doc)
{
	if (doc->feedback) {
		//OMSClaimTimer(myClientID);				/* use built-in */
		appOwnsBITimer = TRUE;
		CMStartTime();
	}
}


void CMMIDIFBNoteOn(Document *doc, INT16 noteNum, INT16 channel, MIDIUniqueID devID)
{
	if (doc->feedback) {
		CMFBNoteOnDevID(doc, noteNum, channel, devID);
		SleepTicks(2L);
	}
}

void CMMIDIFBNoteOff(Document *doc, INT16 noteNum, INT16 channel, MIDIUniqueID devID)
{
	if (doc->feedback) {
		CMFBNoteOffDevID(doc, noteNum, channel, devID);
	}
}


/* ---------------------------------------------------------- CMMIDIFBNoteOn/Off -- */

/* Start MIDI "feedback" note by sending a MIDI NoteOn command for the
specified note and channel. */

static void CMFBNoteOnDevID(Document *doc, INT16 noteNum, INT16 channel, MIDIUniqueID devID)
{
	MIDIUniqueID gDestID = GetMIDIObjectId(gDest);
	
	if (doc->feedback) {
		CMStartNoteNow(devID, noteNum, channel, config.feedbackNoteOnVel);
		SleepTicks(2L);
	}
}

/* End MIDI "feedback" note by sending a MIDI NoteOn command for the
specified note and channel with velocity 0 (which acts as NoteOff). */

static void CMFBNoteOffDevID(Document *doc, INT16 noteNum, INT16 channel, MIDIUniqueID devID)
{
//	MIDIUniqueID gDestID = GetMIDIObjectId(gDest);
	
	
	if (doc->feedback) {
		CMEndNoteNow(devID, noteNum, channel);
		SleepTicks(2L);
	}
}


/* Start MIDI "feedback" note by sending a MIDI NoteOn command for the
specified note and channel. */

void CMFBNoteOn(Document *doc, INT16 noteNum, INT16 channel, short ioRefNum)
{
	MIDIUniqueID gDestID = GetMIDIObjectId(gDest);
	
	if (doc->feedback) {
		CMStartNoteNow(gDestID, noteNum, channel, config.feedbackNoteOnVel);
		SleepTicks(2L);
	}
}

/* End MIDI "feedback" note by sending a MIDI NoteOn command for the
specified note and channel with velocity 0 (which acts as NoteOff). */

void CMFBNoteOff(Document *doc, INT16 noteNum, INT16 channel, short ioRefNum)
{
	MIDIUniqueID gDestID = GetMIDIObjectId(gDest);
	
	if (doc->feedback) {
		CMEndNoteNow(gDestID, noteNum, channel);
		SleepTicks(2L);
	}
}


/* -------------------------------------------------------------- CMAllNotesOff -- */
/* noteOffChannelMap is unused */

void CMAllNotesOff()
{
	MIDIUniqueID	noteOffDeviceList[MAXSTAVES];
	long	 			noteOffChannelMap[MAXSTAVES];
	MIDIUniqueID	*partDevicePtr, endPtID;
	//long				channelMap;
	INT16 			numNoteOffDevices;
	INT16 			noteNum;
	Boolean			deviceFound;
	INT16				i, j;
	OSStatus			err = noErr;
	
	if (currentDoc == NULL) return;
	
	/* First build list of score's part's devices and their channelMaps */

	for (i = MAXSTAVES-1; i>=0; i--) {
		noteOffDeviceList[i] = kInvalidMIDIUniqueID;
		noteOffChannelMap[i] = 0;
	}
	numNoteOffDevices = 0;
	partDevicePtr = currentDoc->cmPartDeviceList;
	for (i = 0; i < MAXSTAVES; i++, partDevicePtr++) {
		if (*partDevicePtr) {
			endPtID = *partDevicePtr;
			deviceFound = FALSE;
			for (j = 0; j < numNoteOffDevices; j++) {
				if (noteOffDeviceList[j] == endPtID) {	/* already listed */
					deviceFound = TRUE;
					break;
				}
			}
			if (!deviceFound) {
				
				noteOffDeviceList[numNoteOffDevices++] = endPtID;
				
			/*
				MIDIEndpointRef endpt = GetMIDIEndpointByID(endPtID);
				if (endpt != NULL) {
					err = MIDIObjectGetIntegerProperty(endpt, kMIDIPropertyReceiveChannels, &channelMap);
				}
				if (err == noErr) {
					noteOffChannelMap[numNoteOffDevices] = channelMap;
					noteOffDeviceList[numNoteOffDevices++] = endPtID;
				}
			*/
			}
		}
	}

	for (i = 0; i < numNoteOffDevices; i++) {
		MIDIUniqueID id = noteOffDeviceList[i];
		if (id != kInvalidMIDIUniqueID) {
		
			MIDIEndpointRef oldGDest = gDest;
			gDest = GetMIDIEndpointByID(id);
			
			for (j = 1; j<=MAXCHANNEL; j++) {
				if (CMTransmitChannelValid(id, j))
				{
					for (noteNum = 0; noteNum<=MAX_NOTENUM; noteNum++) {
						CMEndNoteNow(id, noteNum, j);
					}
				}
			}
			
			gDest = oldGDest;
		}
		
		SleepMS(1L);														/* pause between devices */
	}

}




// -------------------------------------------------------------------------------------

const long kMaxChannels = 16;

void CMDebugPrintXMission()
{
	int n = MIDIGetNumberOfSources();
	for (int i = 0; i < n; ++i) {
		MIDIEndpointRef endpt = MIDIGetSource(i);
		MIDIUniqueID id = GetMIDIObjectId(endpt);
		DebugPrintf("Recv dev=%ld\n", id);
		
		for (long j=1; j<=kMaxChannels; j++) {
			Boolean ok = CMRecvChannelValid(id, j);
			long lOk = (long)ok;
			DebugPrintf(" ch=%ld ok=%ld ", j, lOk);
		}
	}
	
	int n1 = MIDIGetNumberOfDestinations();
	for (int i1 = 0; i1 < n1; ++i1) {
		MIDIEndpointRef endpt1 = MIDIGetDestination(i1);
		MIDIUniqueID id1 = GetMIDIObjectId(endpt1);
		DebugPrintf("\nXMit dev=%ld\n", id1);
		
		for (long j1=1; j1<=kMaxChannels; j1++) {
			Boolean ok1 = CMTransmitChannelValid(id1, j1);
			long lOk1 = (long)ok1;
			DebugPrintf(" ch=%ld ok=%ld ", j1, lOk1);
		}
	}
}

MIDIUniqueID GetMIDIObjectId(MIDIObjectRef obj)
{
	MIDIUniqueID id = kInvalidMIDIUniqueID;
	OSStatus err = noErr;
	
	if (obj != NULL) {
		err = MIDIObjectGetIntegerProperty(obj, kMIDIPropertyUniqueID, &id);
	}
	
	if (err != noErr) {
		id = kInvalidMIDIUniqueID;
	}
	
	return id;
}

static MIDIEndpointRef GetMIDIEndpointByID(MIDIUniqueID id)
{
	if (id == kInvalidMIDIUniqueID)
		return NULL;

	int m = MIDIGetNumberOfSources();
	for (int i = 0; i < m; ++i) {
		MIDIEndpointRef src = MIDIGetSource(i);
		MIDIUniqueID srcID = GetMIDIObjectId(src);
		if (srcID == id)
			return src;
	}
	
	int n = MIDIGetNumberOfDestinations();
	for (int i = 0; i < n; ++i) {
		MIDIEndpointRef dest = MIDIGetDestination(i);
		MIDIUniqueID destID = GetMIDIObjectId(dest);
		if (destID == id)
			return dest;
	}
	
	return NULL;
}

static void GetInitialDefaultInputDevice()
{
	MIDIEndpointRef cfg = GetMIDIEndpointByID(config.cmDefaultInputDevice);
	
	MIDIEndpointRef src = NULL;
	MIDIEndpointRef s = NULL;
	OSStatus err = noErr;
	
	int n = MIDIGetNumberOfSources();
	int i = 0;
	for ( ; i < n; ++i) {
		s = MIDIGetSource(i);
		if (s == cfg) { 
			src = s; break;
		}
	}
	
	if (src == NULL) {
		n = MIDIGetNumberOfSources();
		for (i = 0; i < n; ++i) {
			src = MIDIGetSource(i); break;
		}
	}
	
	gDefaultInputDevID = GetMIDIObjectId(src);
}

static Boolean CMChannelValidRange(int channel)
{
	if (channel < kCMMinMIOIChannel)
		return FALSE;
		
	if (channel > kCMMaxMIDIChannel)
		return FALSE;
		
	return TRUE;
}

Boolean CMRecvChannelValid(MIDIUniqueID endPtID, int channel)
{
	if (!CMChannelValidRange(channel))
		return FALSE;

	MIDIEndpointRef endpt = GetMIDIEndpointByID(endPtID);
	long channelMap, channelValid = 0;
	OSStatus err = -1;
	
	if (endpt != NULL) {
		err = MIDIObjectGetIntegerProperty(endpt, kMIDIPropertyReceiveChannels, &channelMap);
	}
	
	if (err == noErr) {
	
		// For one-based channel:
		channelValid = (channelMap >> (channel-1)) & 0x01;
		
		// For zero-based channel:
		//channelValid = (channelMap >> (channel)) & 0x01;
	}
	else if (err == kMIDIUnknownProperty) {
		channelValid = TRUE;
	}
		
	return channelValid;
}

Boolean CMTransmitChannelValid(MIDIUniqueID endPtID, int channel)
{
	if (!CMChannelValidRange(channel))
		return FALSE;

	// Check cannot be made unless there are destinations.
	// If no destinations, will play with software AudioUnit,
	// which is active on all channels [I think. CER 7/16/2003]
	
	if (gCMNoDestinations)
		return TRUE;
		
	if (endPtID == gAuMidiControllerID)
		return TRUE;
		
	MIDIEndpointRef endpt = GetMIDIEndpointByID(endPtID);
	long channelMap, channelValid = 0;
	OSStatus err = -1;
	
	if (endpt != NULL) {
		err = MIDIObjectGetIntegerProperty(endpt, kMIDIPropertyTransmitChannels, &channelMap);
	}
	
	if (err == noErr) {
	
		// For one-based channel:
		channelValid = (channelMap >> (channel-1)) & 0x01;
		
		// For zero-based channel:
		//channelValid = (channelMap >> (channel)) & 0x01;
	}
	else if (err == kMIDIUnknownProperty) {
		channelValid = TRUE;
	}
		
	return channelValid;
}

static void CheckDefaultInputDevice()
{
	MIDIEndpointRef src;
		
	if (gDefaultInputDevID == kInvalidMIDIUniqueID)
	{
		GetInitialDefaultInputDevice();
	}
	
	src = GetMIDIEndpointByID(gDefaultInputDevID);
	
	if (!CMRecvChannelValid(gDefaultInputDevID, gDefaultChannel))
	{
		int n = MIDIGetNumberOfSources();
		for (int i = 0; i < n; ++i) {
			MIDIEndpointRef endpt = MIDIGetSource(i);
			MIDIUniqueID id = GetMIDIObjectId(endpt);
			
			if (CMRecvChannelValid(id, gDefaultChannel)) {
				gDefaultInputDevID = id;
				break;
			}
		}
	}
	
	DebugPrintf("Default input dev id: %ld\n", gDefaultInputDevID);
	DebugPrintf("Default channel: %ld\n", gDefaultChannel);
}

static void GetInitialDefaultOutputDevice()
{
	MIDIEndpointRef cfg = GetMIDIEndpointByID(config.cmDefaultOutputDevice);
	
	MIDIEndpointRef dest = NULL;
	MIDIEndpointRef d = NULL;
	OSStatus err = noErr;
	
	int n = MIDIGetNumberOfDestinations();
	int i = 0;
	for ( ; i < n; ++i) {
		d = MIDIGetDestination(i);
		if (d == cfg) { 
			dest = d; break;
		}
	}
	
	if (dest == NULL) {
		n = MIDIGetNumberOfDestinations();
		for (i = 0; i < n; ++i) {
			dest = MIDIGetDestination(i); break;
		}
	}
	
	gDefaultOutputDevID = GetMIDIObjectId(dest);
}

static void CheckDefaultOutputDevice()
{
	MIDIEndpointRef dest;
		
	if (gDefaultOutputDevID == kInvalidMIDIUniqueID)
	{
		GetInitialDefaultOutputDevice();
	}
	
	dest = GetMIDIEndpointByID(gDefaultOutputDevID);
	
	if (!CMTransmitChannelValid(gDefaultOutputDevID, gDefaultChannel))
	{
		int n = MIDIGetNumberOfDestinations();
		for (int i = 0; i < n; ++i) {
			MIDIEndpointRef endpt = MIDIGetDestination(i);
			MIDIUniqueID id = GetMIDIObjectId(endpt);
			
			if (CMTransmitChannelValid(id, gDefaultChannel)) {
				gDefaultOutputDevID = id;
				break;
			}
		}
	}
	
	DebugPrintf("Default output dev id: %ld\n", gDefaultOutputDevID);
	DebugPrintf("Default channel: %ld\n", gDefaultChannel);
}

static void CheckDefaultMetroDevice()
{
	if (gDefaultMetroDevID == kInvalidMIDIUniqueID)
	{
		gDefaultMetroDevID = gDefaultOutputDevID;
	}
	else if (!CMTransmitChannelValid(gDefaultMetroDevID, gDefaultChannel))
	{
		gDefaultMetroDevID = gDefaultOutputDevID;
	}
}


static void CheckDefaultDevices()
{
	CheckDefaultInputDevice();
	
	CheckDefaultOutputDevice();
	
	CheckDefaultMetroDevice();
}


// These calls use a selected input / midi thru device & channel, check if the
// device / channel combos are valid, and get IORefNums for each for the MidiReadProc.
//
// Does Core Midi have an IORefNum for its read proc that needs to do this?

void CoreMidiSetSelectedInputDevice(MIDIUniqueID inputDevice, INT16 inputChannel)
{
	gSelectedInputDevice = inputDevice;
	gSelectedInputChannel = inputChannel;
}

void CoreMidiSetSelectedMidiThruDevice(MIDIUniqueID thruDevice, INT16 thruChannel)
{
	gSelectedThruDevice = thruDevice;
	gSelectedThruChannel = thruChannel;
}

void CoreMidiSetSelectedOutputDevice(MIDIUniqueID outputDevice, INT16 outputChannel)
{
	gSelectedOutputDevice = outputDevice;
	gSelectedOutputChannel = outputChannel;
}

MIDIUniqueID CoreMidiGetSelectedOutputDevice(INT16 *outputChannel)
{
	*outputChannel = gSelectedOutputChannel;
	return gSelectedOutputDevice;
}

OSStatus OpenCoreMidiInput(MIDIUniqueID inputDevice)
{
	return noErr;
}

// --------------------------------------------------------------------------------------
// MIDI Controllers

OSStatus CMMIDIController(MIDIUniqueID destDevID, char channel, Byte ctrlNum, Byte ctrlVal, MIDITimeStamp tStamp)
{
	Byte controller[] = { MCTLCHANGE, 0, 0 };
	//MIDITimeStamp tStamp = 0;				// Indicates perform NOW.

	controller[0] = MCTLCHANGE;
	controller[0] |= channel;
	controller[1] = ctrlNum;
	controller[2] = ctrlVal;
	
	OSStatus err = CMWritePacket(destDevID, tStamp, 3, controller);

	return err;
}

OSStatus CMMIDISustainOn(MIDIUniqueID destDevID, char channel, MIDITimeStamp tStamp) 
{
	Byte ctrlNum = MSUSTAIN;
	Byte ctrlVal = 127;
	
	OSStatus err = CMMIDIController(destDevID, channel, ctrlNum, ctrlVal, tStamp);
	return err;
}

OSStatus CMMIDISustainOff(MIDIUniqueID destDevID, char channel, MIDITimeStamp tStamp) 
{
	Byte ctrlNum = MSUSTAIN;
	Byte ctrlVal = 0;
	
	OSStatus err = CMMIDIController(destDevID, channel, ctrlNum, ctrlVal, tStamp);
	return err;
}

OSStatus CMMIDIPan(MIDIUniqueID destDevID, char channel, Byte panSetting, MIDITimeStamp tStamp) 
{
	Byte ctrlNum = MPAN;
	Byte ctrlVal = panSetting;
	
	OSStatus err = CMMIDIController(destDevID, channel, ctrlNum, ctrlVal, tStamp);
	return err;
}

OSStatus CMMIDIController(MIDIUniqueID destDevID, char channel, Byte ctrlNum, Byte ctrlVal)
{
	Byte controller[] = { MCTLCHANGE, 0, 0 };
	MIDITimeStamp tStamp = 0;				// Indicates perform NOW.

	controller[0] = MCTLCHANGE;
	controller[0] |= channel;
	controller[1] = ctrlNum;
	controller[2] = ctrlVal;
	
	OSStatus err = CMWritePacket(destDevID, tStamp, 3, controller);

	return err;
}

OSStatus CMMIDISustainOn(MIDIUniqueID destDevID, char channel) 
{
	OSStatus err = CMMIDISustainOn(destDevID, channel, 0);
	return err;
}

OSStatus CMMIDISustainOff(MIDIUniqueID destDevID, char channel) 
{
	OSStatus err = CMMIDISustainOff(destDevID, channel, 0);
	return err;
}

OSStatus CMMIDIPan(MIDIUniqueID destDevID, char channel, Byte panSetting) 
{
	OSStatus err = CMMIDIPan(destDevID, channel, panSetting, 0);
	return err;
}


/* ---------------------------------------------------------- CreateOMSInputMenu --

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
 */

/* ---------------------------------------------------------- GetCMDeviceForPart -- */

MIDIUniqueID GetCMDeviceForPartn(Document *doc, INT16 partn)
{
	if (!doc) doc = currentDoc;
	return doc->cmPartDeviceList[partn];
}

MIDIUniqueID GetCMDeviceForPartL(Document *doc, LINK partL)
{
	INT16 partn;
	
	if (!doc) doc = currentDoc;
	partn = (INT16)PartL2Partn(doc, partL);
	return doc->cmPartDeviceList[partn];
}

/* ---------------------------------------------------------- SetCMDeviceForPart -- */

void SetCMDeviceForPartn(Document *doc, INT16 partn, MIDIUniqueID device)
{
	if (!doc) doc = currentDoc;
	doc->cmPartDeviceList[partn] = device;
}

void SetCMDeviceForPartL(Document *doc, LINK partL, INT16 partn, MIDIUniqueID device)
{
	//INT16 partn;
	
	if (!doc) doc = currentDoc;
	//partn = (INT16)PartL2Partn(doc, partL);
	doc->cmPartDeviceList[partn] = device;
}

/* --------------------------------------------------------- InsertPartnCMDevice -- */
/* Insert a null device in front of partn's device */

void InsertPartnCMDevice(Document *doc, INT16 partn, INT16 numadd)
{
	register INT16 i, j;
	INT16 curLastPartn;
	
	if (!doc) doc = currentDoc;
	curLastPartn = LinkNENTRIES(doc->headL) - 1;
	/* shift list from partn through curLastPartn up by numadd */
	for (i=curLastPartn, j=i+numadd;i >= partn;)
		doc->cmPartDeviceList[j--] = doc->cmPartDeviceList[i--];
	for (i=partn;i<partn+numadd;)
		doc->cmPartDeviceList[i++] = 0;
}

void InsertPartLCMDevice(Document *doc, LINK partL, INT16 numadd)
{
	INT16 partn;
	
	if (!doc) doc = currentDoc;
	partn = (INT16)PartL2Partn(doc, partL);
	InsertPartnCMDevice(doc, partn, numadd);
}


/* --------------------------------------------------------- DeletePartnCMDevice -- */

void DeletePartnCMDevice(Document *doc, INT16 partn)
{
	register INT16 i, j;
	INT16 curLastPartn;
	
	if (!doc) doc = currentDoc;
	curLastPartn = LinkNENTRIES(doc->headL) - 1;
	/* Shift list from partn+1 through curLastPartn down by 1 */
	for (i=partn+1, j=i-1;i >= curLastPartn;)
		doc->cmPartDeviceList[i++] = doc->cmPartDeviceList[j++];
	doc->cmPartDeviceList[curLastPartn] = 0;
}

void DeletePartLCMDevice(Document *doc, LINK partL)
{
	INT16 partn;

	if (!doc) doc = currentDoc;
	partn = (INT16)PartL2Partn(doc, partL);
	DeletePartnCMDevice(doc, partn);
}


// --------------------------------------------------------------------------------------
// MIDI Programs & Patches

//#define CM_PATCHNUM_BASE 1			/* Some synths start numbering at 1, some at 0 */
//#define CM_CHANNEL_BASE 0

// 0-based uses BASE of 1

#define CM_PATCHNUM_BASE 1			/* Some synths start numbering at 1, some at 0 */
#define CM_CHANNEL_BASE 1	

/*

 * This provides a conditional channel base depending on whether we are using
 * a hardware or software device.
 * Currently not using this, as both hardware and software are 0-based and use
 * CM_CHANNEL_BASE 1.
 
static int CMGetChannelBase(Document *doc, INT16 partn)
{
	MIDIUniqueID destDevID = GetCMDeviceForPartn(doc, partn);
	
	if (destDevID == gAuMidiControllerID)
		return 1;
		
	return 0;
}
*/

/* -------------------------------------------------------------- UseMIDIChannel -- */
/* Return the MIDI channel number to use for the given part. */

/*

 * UseMIDIChannel will get the channel from the partInfo record (entered in the
 * range [1,16], and pin it inside the range [1,MAXCHANNEL] where MAXCHANNEL = 16.
 * This is always correct, regardless of the channel base; so we do not need to
 * define CMUseMIDIChannel here to take into account CM_CHANNEL_BASE.
 * The channel will be translated to its proper base when constructing the packet
 * either from CMMIDIProgram or using CMGetNotePlayInfo.
 
INT16 UseMIDIChannel(Document *doc, INT16	partn)
{
	INT16				useChan;
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
*/

OSErr CMMIDIProgram(Document *doc, unsigned char *partPatch, unsigned char *partChannel)
{
	INT16 i, channel, patch;
	//INT16 channelBase;
	Byte programChange[] = { 0xC0, 0x00 };
	MIDITimeStamp tStamp = 0;				// Indicates perform NOW.
	OSStatus err = noErr;
	
	MIDIUniqueIDVector *cmVecDevs = new MIDIUniqueIDVector();
	long numItems = FillCMDestinationVec(cmVecDevs);
	MIDIUniqueIDVector::iterator itr = cmVecDevs->begin();
	MIDIUniqueID defaultID = *itr;
	
	if (doc->polyTimbral && !doc->dontSendPatches && err==noErr) {
		for (i = 1; i<=LinkNENTRIES(doc->headL) - 1 && err == noErr; i++) {		// skip dummy part			
			//channelBase = CMGetChannelBase(doc, i);
			channel = partChannel[i] - CM_CHANNEL_BASE; // channelBase;
			patch = partPatch[i] - CM_PATCHNUM_BASE;
			programChange[0] = MPGMCHANGE + channel;
			programChange[1] = patch;
			MIDIUniqueID destDevID = GetCMDeviceForPartn(doc, i);
			err = CMWritePacket(destDevID, tStamp, 2, programChange);
			
			if (err != noErr) {
				destDevID = defaultID;
				err = CMWritePacket(destDevID, tStamp, 2, programChange);
				if (err == noErr) {
					SetCMDeviceForPartn(doc, i, destDevID);
				}
			}
		}
	}
	
	return err;
}

/* ------------------------------------------------------------- CMGetUseChannel -- */
INT16 CMGetUseChannel(Byte partChannel[], INT16 partn) 
{
	INT16 useChan = partChannel[partn] - CM_CHANNEL_BASE;
	return useChan;
}

/* ------------------------------------------------------------- CMGetNotePlayInfo -- */
/* Given a note and tables of part transposition, channel, and offset velocity, return
the note's MIDI note number, including transposition; channel number; and velocity,
limited to legal range. */

void CMGetNotePlayInfo(Document *doc, LINK aNoteL, short partTransp[],
						Byte partChannel[], SignedByte partVelo[],
						INT16 *pUseNoteNum, INT16 *pUseChan, INT16 *pUseVelo)
{
	INT16 partn;
	PANOTE aNote;
	//PPARTINFO pPart;
	//INT16 channelBase;

	partn = Staff2Part(doc,NoteSTAFF(aNoteL));
	*pUseNoteNum = UseMIDINoteNum(doc, aNoteL, partTransp[partn]);
	//channelBase = CMGetChannelBase(doc, partn);
	*pUseChan = partChannel[partn] - CM_CHANNEL_BASE; // channelBase;
	aNote = GetPANOTE(aNoteL);
	*pUseVelo = doc->velocity+aNote->onVelocity;
	if (doc->polyTimbral) *pUseVelo += partVelo[partn];
	
	if (*pUseVelo<1) *pUseVelo = 1;
	if (*pUseVelo>MAX_VELOCITY) *pUseVelo = MAX_VELOCITY;
}

/* ----------------------------------------------------------- GetCMPartPlayInfo -- */
/* Similar to the non-CM GetPartPlayInfo. */

Boolean GetCMPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
							Byte partPatch[], SignedByte partVelo[], short partIORefNum[],
							MIDIUniqueID partDevice[])
{
	INT16 i, channel; LINK partL; PARTINFO aPart;
	
	partL = FirstSubLINK(doc->headL);
	for (i = 0; i<=LinkNENTRIES(doc->headL)-1; i++, partL = NextPARTINFOL(partL)) {
		if (i==0) continue;					/* skip dummy partn = 0 */
		aPart = GetPARTINFO(partL);
		partVelo[i] = aPart.partVelocity;
		
		// Do not need CMUseMIDIChannel here [cf comments above].
		partChannel[i] = UseMIDIChannel(doc, i);
		partPatch[i] = aPart.patchNum;
		partTransp[i] = aPart.transpose;
		if (doc->polyTimbral || doc->cmInputDevice != 0)
			partDevice[i] = GetCMDeviceForPartn(doc, i);
		else
			partDevice[i] = doc->cmInputDevice;

		/* Validate device / channel combination. */
		if (!CMTransmitChannelValid(partDevice[i], (INT16)partChannel[i])) {
			partDevice[i] = CoreMidiGetSelectedOutputDevice(&channel);
			//partDevice[i] = config.cmDefaultOutputDevice;
		}

		/* It's possible our device has changed, so validate again (and again, if necc.). */
		if (!CMTransmitChannelValid(partDevice[i], (INT16)partChannel[i])) {
			partChannel[i] = config.defaultChannel;
			if (!CMTransmitChannelValid(partDevice[i], (INT16)partChannel[i])) {
				if (CautionAdvise(NO_OMS_DEVS_ALRT)==1) return FALSE;			/* Cancel playback button (item 1 in this ALRT!) */
			}
		}
		
#ifdef CMDEBUG			
//		DebugPrintf("GetCMPartPlayInfo: i=%ld partChannel[i]=%ld partDevice[i]=%ld\n", 
//			i, partChannel[i], partDevice[i]);
#endif

		partIORefNum[i] = CMInvalidRefNum;		// No IORefNums for CoreMIDI
	}
	return TRUE;
}


/* ----------------------------------------------------------- GetCMNotePlayInfo -- */
/* Given a note and tables of part transposition, channel, and offset velocity, return
the note's MIDI note number, including transposition; channel number; and velocity,
limited to legal range. Similar to the non-CM GetNotePlayInfo. */

void GetCMNotePlayInfo(
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



// --------------------------------------------------------------------------------------


static void DisplayMidiDevices()
{
	CFStringRef pname, pmanuf, pmodel;
	char name[64], manuf[64], model[64];
	
	int n = MIDIGetNumberOfDevices();
	for (int i = 0; i < n; ++i) {
		MIDIDeviceRef dev = MIDIGetDevice(i);
		
		MIDIObjectGetStringProperty(dev, kMIDIPropertyName, &pname);
		MIDIObjectGetStringProperty(dev, kMIDIPropertyManufacturer, &pmanuf);
		MIDIObjectGetStringProperty(dev, kMIDIPropertyModel, &pmodel);
		
		CFStringGetCString(pname, name, sizeof(name), 0);
		CFStringGetCString(pmanuf, manuf, sizeof(manuf), 0);
		CFStringGetCString(pmodel, model, sizeof(model), 0);
		CFRelease(pname);
		CFRelease(pmanuf);
		CFRelease(pmodel);

		printf("name=%s, manuf=%s, model=%s\n", name, manuf, model);
	}
}

long FillCMSourcePopup(MenuHandle menu, vector<MIDIUniqueID> *vecDevices)
{
	CFStringRef pname, pmanuf, pmodel;
	char name[64], manuf[64], model[64], device[255];
	
	long n = MIDIGetNumberOfSources();
	//for (long i = 0; i < n; ++i) {
	for (long i = n-1; i >=0; i--) {
		MIDIEndpointRef dev = MIDIGetSource(i);
		
		MIDIObjectGetStringProperty(dev, kMIDIPropertyName, &pname);
		MIDIObjectGetStringProperty(dev, kMIDIPropertyManufacturer, &pmanuf);
		MIDIObjectGetStringProperty(dev, kMIDIPropertyModel, &pmodel);
		
		CFStringGetCString(pname, name, sizeof(name), 0);
		CFStringGetCString(pmanuf, manuf, sizeof(manuf), 0);
		CFStringGetCString(pmodel, model, sizeof(model), 0);
		CFRelease(pname);
		CFRelease(pmanuf);
		CFRelease(pmodel);
		
		strcpy(device, name);
		strcat(device, " ");
		strcat(device, manuf);
		strcat(device, " ");
		strcat(device, model);
		
		AppendMenu(menu, CToPString(device));
		vecDevices->push_back(GetMIDIObjectId(dev));
	}
	
	return n;
}


long FillCMDestinationPopup(MenuHandle menu, vector<MIDIUniqueID> *vecDevices)
{
	CFStringRef pname, pmanuf, pmodel;
	char name[64], manuf[64], model[64], device[255];
	
	long n = MIDIGetNumberOfDestinations();
	//for (long i = 0; i < n; ++i) {
	for (long i = n-1; i>=0; i--) {
		MIDIEndpointRef dev = MIDIGetDestination(i);
		
		OSStatus nameStat = MIDIObjectGetStringProperty(dev, kMIDIPropertyName, &pname);
		OSStatus manufStat = MIDIObjectGetStringProperty(dev, kMIDIPropertyManufacturer, &pmanuf);
		OSStatus modelStat = MIDIObjectGetStringProperty(dev, kMIDIPropertyModel, &pmodel);
		
		/* If we have a virtual destination, the manuf property is null. */
		
		name[0] = '\0';
		manuf[0] = '\0';
		model[0] = '\0';
		
		if (pname && nameStat == noErr) {
			CFStringGetCString(pname, name, sizeof(name), 0);
			CFRelease(pname);
		}
		
		if (pmanuf && manufStat == noErr) {
			CFStringGetCString(pmanuf, manuf, sizeof(manuf), 0);
			CFRelease(pmanuf);
		}
		
		if (pmodel && modelStat == noErr) {
			CFStringGetCString(pmodel, model, sizeof(model), 0);
			CFRelease(pmodel);
		}
		
		strcpy(device, name);
		strcat(device, " ");
		strcat(device, manuf);
		strcat(device, " ");
		strcat(device, model);
		
		AppendMenu(menu, CToPString(device));
		vecDevices->push_back(GetMIDIObjectId(dev));
	}
	
	return n;
}

long FillCMDestinationVec(vector<MIDIUniqueID> *vecDevices)
{
	CFStringRef pname, pmanuf, pmodel;
	char name[64], manuf[64], model[64], device[255];
	
	long n = MIDIGetNumberOfDestinations();
	//for (long i = 0; i < n; ++i) {
	for (long i = n-1; i>=0; i--) {
		MIDIEndpointRef dev = MIDIGetDestination(i);
		
		OSStatus nameStat = MIDIObjectGetStringProperty(dev, kMIDIPropertyName, &pname);
		OSStatus manufStat = MIDIObjectGetStringProperty(dev, kMIDIPropertyManufacturer, &pmanuf);
		OSStatus modelStat = MIDIObjectGetStringProperty(dev, kMIDIPropertyModel, &pmodel);
		
		/* If we have a virtual destination, the manuf property is null. */
		
		name[0] = '\0';
		manuf[0] = '\0';
		model[0] = '\0';
		
		if (pname && nameStat == noErr) {
			CFStringGetCString(pname, name, sizeof(name), 0);
			CFRelease(pname);
		}
		
		if (pmanuf && manufStat == noErr) {
			CFStringGetCString(pmanuf, manuf, sizeof(manuf), 0);
			CFRelease(pmanuf);
		}
		
		if (pmodel && modelStat == noErr) {
			CFStringGetCString(pmodel, model, sizeof(model), 0);
			CFRelease(pmodel);
		}
		
		strcpy(device, name);
		strcat(device, " ");
		strcat(device, manuf);
		strcat(device, " ");
		strcat(device, model);
		
		vecDevices->push_back(GetMIDIObjectId(dev));
	}
	
	return n;
}

static void GetDLSControllerID()
{
	CFStringRef pname;
	char name[64];
	
	name[0] = 0;
	
	long n = MIDIGetNumberOfDestinations();
	for (long i = 0; i < n; ++i) {
		MIDIEndpointRef dev = MIDIGetDestination(i);
		
		OSStatus nameStat = MIDIObjectGetStringProperty(dev, kMIDIPropertyName, &pname);
		
		if (nameStat == noErr)
		{
			CFStringGetCString(pname, name, sizeof(name), 0);		
			CFRelease(pname);
			
			if (!strcmp(name, "DLS_MIDIController"))
			{
				gAuMidiControllerID = GetMIDIObjectId(dev);
			}
		}
	}
}


Boolean InitCoreMIDI()
{
	int i, n;
	
	if (!gCoreMIDIInited) {
	
		if (FALSE && OptionKeyDown()) {
			ShowHideDebugWindow(TRUE);
			long lgDefaultChannel = (long)gDefaultChannel;
			DebugPrintf("gDefaultChannel = %ld %ld %ld\n", gDefaultChannel, (long)gDefaultChannel, lgDefaultChannel);
		}
			
					
		gCMBufferLength = kCMBufLen;		/* CoreMidi MIDI Buffer size in bytes */
		gCMMIDIBufferFull = FALSE;
		
		gCMNoDestinations = FALSE;			/* For SoftMIDI playback */
		gCMSoftMIDIActive = FALSE;

		if (!AllocCMPacketList()) {
			DebugPrintf("Out of memory allocating global packet list\n");
			return FALSE;
		}
			
		if (!AllocCMWritePacketList()) {
			DebugPrintf("Out of memory allocating playNote packet list\n");
			return FALSE;		
		}	
			
		// create client and ports
		OSStatus stat = MIDIClientCreate(CFSTR("MIDI Nightingale"), NULL, NULL, &gClient);
		if (stat != noErr || gClient == NULL) {
			DebugPrintf("Error creating MIDI Client\n");
		}
		else {
			DebugPrintf("Creating MIDI Client: gClient = %ld\n", gClient);
		}
		
		stat = MIDIInputPortCreate(gClient, CFSTR("Input port"), NightCMReadProc, NULL, &gInPort);
		if (stat != noErr || gInPort == NULL) {
			DebugPrintf("Error creating MIDI Input Port\n");
		}
		else {
			DebugPrintf("Creating MIDI Input Port: gInPort = %ld\n", gInPort);
		}
		stat = MIDIOutputPortCreate(gClient, CFSTR("Output port"), &gOutPort);
		if (stat != noErr || gOutPort == NULL) {
			DebugPrintf("Error creating MIDI Output Port\n");
		}
		else {
			DebugPrintf("Creating MIDI Output Port: gOutPort = %ld\n", gOutPort);
		}
		
		if (config.cmDefaultOutputChannel > 0 && 
				config.cmDefaultOutputChannel <= MAXCHANNEL) {
			gDefaultChannel = config.cmDefaultOutputChannel;
		}
		else {
			DebugPrintf("No config setting for default channel; using: gDefaultChannel = %ld\n", gDefaultChannel);
			
			config.cmDefaultOutputChannel = 1;
			DebugPrintf("Setting config val for default channel to 1\n");
		}

		// Set up the DLS Synth audio unit and its Midi controller here with virtual endpoint
		// before we check the midi destinations.
		
		Byte activeChannel[MAXCHANNEL+1];
		
		// Send sound bank control changes and program changes on all channels.
		
		for (int i = 0; i<=MAXCHANNEL; i++)
			activeChannel[i] = 1;

		SetupSoftMIDI(activeChannel);
		
		CheckDefaultDevices();
		
		// open connections from all sources
		stat = noErr;
		MIDIEndpointRef src = NULL;
		
		n = MIDIGetNumberOfSources();
		//printf("%d sources\n", n);
		DebugPrintf("%ld sources\t\t", n);
		for (i = 0; i < n && stat==noErr; ++i) {
			src = MIDIGetSource(i);
			stat = MIDIPortConnectSource(gInPort, src, NULL);
		}
		
		if (stat!=noErr) {
			DebugPrintf("Error connecting source: src = %ld \n", src);
		}	
		
		// find the first destination
		n = MIDIGetNumberOfDestinations();
		DebugPrintf("%ld destinations\n", n);
		if (n > 0) {
			gDest = MIDIGetDestination(0);
			
			DebugPrintf("Got MIDI Destination: gDest = %ld\n", gDest);
		}
		else {
			gCMNoDestinations = TRUE;

			DebugPrintf("No available MIDI Destinations\n");
		}
		
		GetDLSControllerID();
		
		InitCoreMidiTimer();
		
		gSelectedInputDevice = 0L;
		gSelectedInputChannel = 0;
		gSelectedThruDevice = 0L;
		gSelectedThruChannel = 0;
		
		CoreMidiSetSelectedInputDevice(gDefaultInputDevID, gDefaultChannel);		
		CoreMidiSetSelectedMidiThruDevice(gDefaultOutputDevID, gDefaultChannel);
		CoreMidiSetSelectedOutputDevice(gDefaultOutputDevID, gDefaultChannel);
		
		//CoreMidiSetSelectedInputDevice(config.cmDefaultInputDevice, config.defaultChannel);		
		//CoreMidiSetSelectedMidiThruDevice(config.cmMetroDevice, config.defaultChannel);
		
		//DisplayMidiDevices();
		
		CMDebugPrintXMission();

		gCoreMIDIInited = true;
	}
	
	return TRUE;
}

