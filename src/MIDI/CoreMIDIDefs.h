#ifndef _CORE_MIDIDEFS_
#define _CORE_MIDIDEFS_

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h> 	//for AUGraph
// MAS use vector, not vector.h ; use namespace to avoid redefining all the vectors
#include <vector>
using namespace std;

#ifdef _CORE_MIDIGLOBALS_
#define CORE_MIDIGLOBAL
#else
#define CORE_MIDIGLOBAL extern
#endif

#ifndef True
	#define True		1
#endif
#ifndef False
	#define False		0
#endif

typedef vector<MIDIUniqueID> MIDIUniqueIDVector;

const MIDIUniqueID kInvalidMIDIUniqueID = 0;

const long kCMBufLen 			= 0x18000;			// 98304;
const long kCMWriteBufLen 		= 0x0200;			// 512

const long kCMMinMIOIChannel	= 1;
const long kCMMaxMIDIChannel	= 16;

const Byte kActiveSensingData	= 0xFE;

const UInt64 kNanosToMillis 	= 1000000;

const int kActiveSensingLen 	= 1;
const int kNoteOnLen 			= 3;
const int kNoteOffLen 			= 3;

CORE_MIDIGLOBAL Byte			gMIDIPacketListBuf[kCMBufLen];
CORE_MIDIGLOBAL Byte			gMIDIWritePacketListBuf[kCMWriteBufLen];

CORE_MIDIGLOBAL MIDIPacketList	*gMIDIPacketList;
CORE_MIDIGLOBAL MIDIPacketList	*gMIDIWritePacketList;

CORE_MIDIGLOBAL MIDIPacket		*gCurrentPacket;
CORE_MIDIGLOBAL MIDIPacket		*gCurrPktListBegin;
CORE_MIDIGLOBAL MIDIPacket		*gCurrPktListEnd;
CORE_MIDIGLOBAL MIDIPacket		*gCurrentWritePacket;

CORE_MIDIGLOBAL Boolean			gSignedIntoCoreMIDI;
CORE_MIDIGLOBAL Boolean			gCMNoDestinations;
CORE_MIDIGLOBAL Boolean			gCMSoftMIDIActive;
CORE_MIDIGLOBAL Boolean			appOwnsBITimer;

CORE_MIDIGLOBAL long			gCMBufferLength;	// CM MIDI Buffer size in bytes
CORE_MIDIGLOBAL Boolean			gCMMIDIBufferFull;

CORE_MIDIGLOBAL MIDIUniqueID	gSelectedInputDevice;
CORE_MIDIGLOBAL int				gSelectedInputChannel;
CORE_MIDIGLOBAL MIDIUniqueID	gSelectedThruDevice;
CORE_MIDIGLOBAL int				gSelectedThruChannel;
CORE_MIDIGLOBAL MIDIUniqueID	gSelectedOutputDevice;
CORE_MIDIGLOBAL int				gSelectedOutputChannel;

// ---------------------------------------------------------------------
// AudioUnit globals [CASoftMIDI.cpp]

// some MIDI constants:

enum {
	kMidiMessage_ControlChange 		= 0xB,
	kMidiMessage_ProgramChange 		= 0xC,
	kMidiMessage_BankMSBControl 	= 0,
	kMidiMessage_BankLSBControl		= 32,
	kMidiMessage_NoteOn 			= 0x9
};

CORE_MIDIGLOBAL AUMIDIControllerRef auMidiController;
CORE_MIDIGLOBAL AUGraph 			graph;
CORE_MIDIGLOBAL AudioUnit 			synthUnit;

CORE_MIDIGLOBAL MIDIUniqueID		gAuMidiControllerID;

// ------------

#ifdef _CORE_MIDIGLOBALS_

MIDIPacket					*gPeekedPkt = NULL;

MIDIClientRef 				gClient = NULL;
MIDIPortRef					gInPort = NULL;
MIDIPortRef					gOutPort = NULL;
MIDIEndpointRef				gDest = NULL;

MIDIUniqueID				gDefaultInputDevID = 0;
MIDIUniqueID				gDefaultOutputDevID = 0;
MIDIUniqueID				gDefaultMetroDevID = 0;
short						gDefaultChannel = 1;

#else

extern MIDIPacket			*gPeekedPkt;

extern MIDIClientRef		gClient;
extern MIDIPortRef			gInPort;
extern MIDIPortRef			gOutPort;
extern MIDIEndpointRef		gDest;

extern MIDIUniqueID			gDefaultInputDevID;
extern MIDIUniqueID			gDefaultOutputDevID;
extern MIDIUniqueID			gDefaultMetroDevID;
extern short				gDefaultChannel;

#endif

#endif	// _CORE_MIDIDEFS_