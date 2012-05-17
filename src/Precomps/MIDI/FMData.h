/***************************************************************************************

	FMData.h
	
	Data structures and constants used by FreeMIDI
	
	Created 10/6/94 by Chris Majoros
	
	Copyright © 1994-1997 Mark of the Unicorn, Inc.

***************************************************************************************/

#ifndef	__FMData__
#define	__FMData__

#ifndef	__TYPES__
// MAS : obsolete
//#include	<Types.h>
#endif

#ifndef __FILES__
#include	<Files.h>
#endif

#ifndef __QUICKDRAW__
#include	<QuickDraw.h>
#endif

#ifndef	__FMTypes__
#include	"FMTypes.h"
#endif

// MAS : obsolete
//#include <Timer.h>

//#if defined(powerc) || defined(__powerc)
#pragma options	align=mac68k
//#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GENERATINGCFM
#undef GENERATINGCFM
#define GENERATINGCFM 0
#endif

/* FreeMIDI System Extension creator */
#define	FreeMIDISelector	'FMS_'

#define	FreeMIDIVersionOne			(0x0100)
#define	FreeMIDIVersionOneZeroOne	(0x0101)
#define	FreeMIDIVersionOneZeroTwo	(0x0102)
#define	FreeMIDIVersionOneZeroThree	(0x0103)
#define	FreeMIDIVersionOneZeroFour	(0x0104)
#define	FreeMIDIVersionOneZeroFive	(0x0105)
#define	FreeMIDIVersionOneZeroSix	(0x0106)
#define	FreeMIDIVersionOneZeroSeven	(0x0107)
#define	FreeMIDIVersionOneZeroEight	(0x0108)
#define	FreeMIDIVersionOneOneZero	(0x0110)
#define	FreeMIDIVersionOneOneOne	(0x0111)
#define	FreeMIDIVersionOneOneTwo	(0x0112)
#define	FreeMIDIVersionOneOneThree	(0x0113)
#define	FreeMIDIVersionOneTwoZero	FreeMIDIVersionOneOneThree	// 1.2 actually shipped this way, with no functional differences
#define	FreeMIDIVersionOneTwoOne	(0x0121)
#define	FreeMIDIVersionOneTwoTwo	(0x0122)
#define	FreeMIDIVersionOneTwoThree	FreeMIDIVersionOneTwoTwo
#define	FreeMIDIVersionOneTwoFour	(0x0124)
#define	FreeMIDIVersionOneTwoFive	(0x0125)
#define	FreeMIDIVersionOneTwoSix	(0x0126)
#define	FreeMIDIVersionOneTwoSeven	(0x0127)
#define	FreeMIDIVersionOneTwoEight	(0x0128)
#define FreeMIDIVersionOneThreeZero	(0x0130)
#define FreeMIDIVersionOneThreeOne	(0x0131)
#define FreeMIDIVersionOneThreeTwo	(0x0132)
#define FreeMIDIVersionOneThreeThree	(0x0133)
#define FreeMIDIVersionOneThreeFour	(0x0134)
#define FreeMIDIVersionOneThreeFive	(0x0135)
#define FreeMIDIVersionOneThreeSix	(0x0136)
#define FreeMIDIVersionOneThreeSeven	(0x0137)
#define	FreeMIDICurrentVersion		FreeMIDIVersionOneThreeSeven

/*
	MIDI data and messages are passed in MidiPacket records (see below).
	The first byte of every MidiPacket contains a set of flags

	bits 0-1	00 = new MidiPacket, not continued
			01 = begining of continued MidiPacket
			10 = end of continued MidiPacket
			11 = continuation
	bits 2-3	reserved

	bits 4-6	000 = packet contains MIDI data
			001 = MIDI error
			111 = reserved for application use

	bit 7		0 = MidiPacket has valid stamp
			1 = stamp with current clock
*/
                        
enum {
	fmsContMask = 0x03,
	fmsNoCont = 0x00,
	fmsStartCont = 0x01,
	fmsMidCont = 0x03,
	fmsEndCont = 0x02,
	fmsTypeMask = 0x70,
	fmsMsgType = 0x00,
	fmsErrorType = 0x10,
	fmsApplType = 0x70,
	fmsTimeStampMask = 0x80,
	fmsTimeStampCurrent = 0x80,
	fmsTimeStampValid = 0x00
};

typedef struct MidiPacket
{
	unsigned char	flags;
	unsigned char	len;
	long			tStamp;
	unsigned char	data[250];
} MidiPacket, *MidiPacketPtr;

typedef void	(*FMParamBlockProcPtr)(long procArg);

enum {
	uppFMParamBlockProcInfo = kThinkCStackBased
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long)))
};

#if GENERATINGCFM
typedef UniversalProcPtr	FMParamBlockUPP;

#define	CallFMParamBlockProc(proc, procArg)		\
			CallUniversalProc((UniversalProcPtr)(proc), uppFMParamBlockProcInfo, (procArg))
#define	NewFMParamBlockProc(proc)				\
			(FMParamBlockUPP)NewRoutineDescriptor((ProcPtr)(proc), uppFMParamBlockProcInfo, GetCurrentISA())
#else
typedef FMParamBlockProcPtr	FMParamBlockUPP;

#define	CallFMParamBlockProc(proc, procArg)		\
			(*(proc))((procArg))
#define	NewFMParamBlockProc(proc)				\
			(FMParamBlockUPP)(proc)
#endif

typedef struct midiParamBlock
{
	struct midiParamBlock	*linkP;			/* Used internally (set to NULL) */
	short				queueCount;		/* Used internally (set to zero) */
	void					*dataP;			/* Points to MIDI data (must be at least one complete message) */
	long					dataLen;			/* Length of data pointed to be dataP */
	FMParamBlockUPP		procP;			/* Proc (called at poll level) after block has been transmitted */
	long					procArg;			/* Single argument passed to procP */
	long					tickTime;			/* Time (in Macintosh ticks) when transmission was completed */
} midiParamBlock, *midiParamBlockPtr;

/* Input filter:
	Each field represents 16 channels worth of events; bit 0 corresponding to channel 1.
	The systemMessage field contains one bit for each type of system message.
	A bit is set if an event should be filtered.
*/
typedef union fmsInputFilter
{
	unsigned short			dataFilter[8];			/* Another way to view the next 8 fields */
	struct {
		unsigned short		noteOff;
		unsigned short		noteOn;
		unsigned short		polyPressure;
		unsigned short		controlChange;
		unsigned short		patchChange;
		unsigned short		chanPressure;
		unsigned short		pitchBend;
		unsigned short		systemMessage;
	} data;
} fmsInputFilter, *fmsInputFilterPtr;

typedef struct MidiPort	*MidiPortPtr;
typedef struct MidiInputQ	MidiInputQ;

/* FreeMIDI System modes */
enum {fmsOnlyMode = 0, fmsAllowOthersMode};

typedef struct fmsPreferencesRec
{
	short		mode;
	Boolean		monitorPatchChanges;
	Boolean		systemSync;			/* Same value returned by FMSGetSystemSync() */
	Boolean		generateSensing;
	Boolean		emulateOMS;
	char		reserved14[14];
	Boolean		usePort[1];			/* Array is actually dynamic; index from 0 to CountFMSMidiPorts() - 1 */
} fmsPreferencesRec, **fmsPreferencesHdl;

/* Return values for rxProc */
enum
{
	fmsKeepPacket = 0,			/* Store the packet in the input queue for later processing */
	fmsDiscardPacket			/* Forget this packet now */
};

/* Return values from FMSSendMidi */
#define	outputQNotQueued		(-3)		/* Data was not queued due to invalid destinationID */
#define	outputQTimedOut		(-2)		/* MIDI port is not functioning */
#define	outputQBusy			(-1)		/* FMSSendMidi is not reentrant for the same application and destination */
#define	outputQOK				0		/* Data was queued successfully */
#define	outputQFull			1		/* Output queue was full -- safe to try again later */

// The following defines are used in the 'info' argument to an application's DestinationChangeProc
// in the setupChanged case.  If 'info' includes kFMChangesLimited, the other bits determine which
// parts of the configuration have changed (eg device inputs, virtual outputs, etc).  If the
// kFMChangesLimited bit is not set, assume that anything in the configuration may have changed.
// (FreeMIDI versions prior to 1.3.1 never set any of these bits.)
#define kFMChangesLimited				0x80000000
#define kFMChangesIncludeInputs			1
#define kFMChangesIncludeOutputs		2
#define kFMChangesIncludeDevices		4
#define kFMChangesIncludeApplications	8
#define kFMChangesIncludeVirtual		16

/* Destination (and source) message codes for destinationChangeProc */
enum
{
	destNewPatchSelected = 0,	/* info = channels */
	destPatchListChanged,		/* info = channels */
	destRelocated,				/* info = new destination ID */
	sourceRelocated,			/* info = new source ID */
	destSourceDeleted,			/* info unused */
	
	setupChanged = 200,			/* info unused */
	setupClosing,				/* info unused */
	setupOpened,				/* info unused */
	
	freeMIDIModeChanged = 300	/* info = (long) new mode (fmsOnlyMode, fmsAllowOthersMode) */
};

/* Commands for applTimingHook */
enum
{
	playRequest = 0,			/* arg unused */
	playStart,					/* arg unused.  MAY BE CALLED AT INTERRUPT LEVEL! */
	playStop,					/* arg unused */
	playSeek,					/* arg = seek time */
	playPause,				/* arg unused */
	releaseTimer,				/* arg unused */
	systemSync,				/* arg = (long)TRUE if sync turned on, (long)FALSE if turned off */
	playUnPause,				/* arg unused */
	reclaimTimer,				/* arg unused */
	playUnPauseRequest			/* arg unused */
};

typedef short	(*FMRxProcPtr)(long sourceID, MidiPacketPtr pakP, long refCon);
typedef void	(*FMDestChangeProcPtr)(fmsUniqueID uniqueID, short msgCode, long info, long refCon);
typedef void	(*FMTimingHook)(short command, long arg, long refCon);

enum {
	uppFMRxProcInfo = kThinkCStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(short)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(MidiPacketPtr)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long))),
	uppFMDestChangeProcInfo = kThinkCStackBased
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(fmsUniqueID)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long)))
		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(long))),
	uppFMTimingHookInfo = kThinkCStackBased
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long)))
};

// Rx Proc
#if GENERATINGCFM
typedef UniversalProcPtr	FMRxProcUPP;

#define	CallFMRxProc(proc, sourceID, pakP, refCon)			\
			CallUniversalProc((UniversalProcPtr)(proc), uppFMRxProcInfo, (sourceID), (pakP), (refCon))
#define	NewFMRxProc(proc)								\
			(FMRxProcUPP)NewRoutineDescriptor((ProcPtr)(proc), uppFMRxProcInfo, GetCurrentISA())
#else
typedef FMRxProcPtr	FMRxProcUPP;

#define	CallFMRxProc(proc, sourceID, pakP, refCon)			\
			(*(proc))((sourceID), (pakP), (refCon))
#define	NewFMRxProc(proc)								\
			(FMRxProcUPP)(proc)
#endif

// Destination Change Proc
#if GENERATINGCFM
typedef UniversalProcPtr	FMDestChangeProcUPP;

#define	CallFMDestChangeProc(proc, uniqueID, msgCode, info, refCon)			\
			CallUniversalProc((UniversalProcPtr)(proc), uppFMDestChangeProcInfo, (uniqueID), (msgCode), (info), (refCon))
#define	NewFMDestChangeProc(proc)									\
			(FMDestChangeProcUPP)NewRoutineDescriptor((ProcPtr)(proc), uppFMDestChangeProcInfo, GetCurrentISA())
#else
typedef FMDestChangeProcPtr	FMDestChangeProcUPP;

#define	CallFMDestChangeProc(proc, uniqueID, msgCode, info, refCon)			\
			(*(proc))((uniqueID), (msgCode), (info), (refCon))
#define	NewFMDestChangeProc(proc)								\
			(FMDestChangeProcUPP)(proc)
#endif

// Application Timing Hook
#if GENERATINGCFM
typedef UniversalProcPtr	FMApplTimingHookUPP;

#define	CallFMApplTimingHook(proc, command, arg, refCon)			\
			CallUniversalProc((UniversalProcPtr)(proc), uppFMTimingHookInfo, (command), (arg), (refCon))
#define	NewFMApplTimingHook(proc)							\
			(FMApplTimingHookUPP)NewRoutineDescriptor((ProcPtr)(proc), uppFMTimingHookInfo, GetCurrentISA())
#else
typedef FMTimingHook	FMApplTimingHookUPP;

#define	CallFMApplTimingHook(proc, command, arg, refCon)			\
			(*(proc))((command), (arg), (refCon))
#define	NewFMApplTimingHook(proc)								\
			(FMApplTimingHookUPP)(proc)
#endif

typedef struct appMethods
{
	FMRxProcUPP			rxProc;
	long					rxProcRefCon;
	FMDestChangeProcUPP	destinationChangeProc;
	long					destProcRefCon;
	FMApplTimingHookUPP	applTimingHook;
	long					timingHookRefCon;
} appMethods;

typedef struct fmsAppInfoRec
{
	OSType		signature;			/* Application signature */
	unsigned short	supportedVersion;	/* FreeMIDI version completely supported by application */
	short		outputQSizes[8];	/* size in bytes */
	short		inputQSize;		/* size in MidiPackets */
	appMethods	methods;
} fmsAppInfoRec, *fmsAppInfoPtr;

typedef struct MidiDriverInfo
{
	OSType		signature;		/* Driver identifier (same as file creator) */
	unsigned short	version;		/* Version number of the driver */
	Handle		iconH;		/* Handle to ICON for this driver */
	Boolean		needsInterface;	/* True if the driver's ports need MIDI Interfaces (FALSE for SampleCell, etc) */
	char			reserved;		/* Presently unused -- set it to zero */
	Str255		name;		/* Driver name */
} MidiDriverInfo, *MidiDriverInfoPtr;

/* Driver types */
#define	interfaceDriverType		'IDvr'
#define	portDriverType			'DDef'

enum {fmsHardwarePortType = 0, fmsApplicationPortType};

/* MIDI transmission characteristics */
enum {freeMIDIPortDoesOutputBit = 0, freeMIDIPortCableizesOutputBit, freeMIDIPortCableizesMTCBit};

/* MIDI reception characteristics */
enum {freeMIDIPortDoesInputBit = 0, freeMIDIPortCableizesInputBit, freeMIDIPortInputIEBit};

typedef struct MidiPortInfo
{
	short		portNum;		/* Port Manager index for this port */
	short		driver;		/* Port Manager index for driver associated with this port */
	Boolean		installed;		/* Is the port normally installed for MIDI? */
	Boolean		inUse;		/* Is the port currently in use by FreeMIDI? */
	char			portType;		/* The kind of port this is (defined above) */
	char			reserved;		/* Reserved for future use */
	short		numInputs;	/* Number of input cables available on this port */
	short		numOutputs;	/* Number of output cables available on this port */
	Handle		iconH;		/* Handle to ICON for this port */
	Handle		sicnH;		/* Handle to SICN (first member only) for this port */
	Str255		name;		/* Port name */
	unsigned char	outputFlags;	/* MIDI transmission characteristics (bit flags) for this port; bits defined above */
	unsigned char	inputFlags;	/* MIDI reception characteristics (bit flags) for this port; bits defined above */
} MidiPortInfo, *MidiPortInfoPtr;

/* These values are used for devices which have been disconnected or no longer exist. */
#define	noDestination				0x07000000		/* destinationID */
#define	noSource					(-1L)			/* sourceID */
#define	noUniqueID				0				/* uniqueID */

#define	allDestinationsMask			noDestination
#define	uniqueIDDestinationMask		0
#define	directDestinationMask		0x01000000

#define	autoChannelBit				0x08000000
#define	autoChannelMask			0xf0000000

/* Macros for doing auto-channelization */
#define	autoChannelizeUniqueID(id, ch)		(((long)(id) & 0xffff) | ((long)(ch) << 28) | autoChannelBit)
#define	autoChannelizeDestinationID(id, ch)	((id) | ((long)(ch) << 28) | autoChannelBit)
#define	getAutoChannel(id)				(short)((id) >> 28)

typedef struct location
{
	unsigned char	port;
	char			filler;
	short		cable;
} location;

/* destinationInfoRec -- generally used only by interface drivers */
enum {directDestinationType = 0, standardDestinationType, interfaceDestinationType, instrumentDestinationType,
	softwareDestinationType, destinationIsVirtual = 0x8000};
#define	destinationTypeMask		0x7fff

typedef struct destinationInfoRec
{
	short		destinationType;
	unsigned short	receiveChannels;
	unsigned short	transmitChannels;
	location		input;
	location		output;
	unsigned long	standardProperties;
	unsigned long	userProperties;
} destinationInfoRec, *destinationInfoPtr;

enum {deviceDestination = 0, virtualDestination, softwareDestination, directDestination};
#define	noDestinationMatch		(-1)

typedef union destinationMatch
{
	struct
	{
		short		destinationType;	/* One of the types enumerated above */
		char			reserved[20];		/* For alignment of name with other types */
		Str255		name;
	} basic;
	struct
	{
		short		destinationType;	/* deviceDestination */
		fmsUniqueID	uniqueID;
		OSType		portDriver;
		short		portNumber;
		short		output;
		long			mfrCode;
		long			modelCode;
		short		index;
		Str255		name;
	} device;
	struct
	{
		short		destinationType;	/* virtualDestination */
		char			reserved[20];		/* For alignment of name with other types */
		Str255		name;
	} instrument;
	struct
	{
		short		destinationType;	/* softwareDestination */
		OSType		signature;
		char			reserved[16];		/* For alignment of name with other types */
		Str255		name;
	} application;
	struct
	{
		short		destinationType;	/* directDestination */
		fmsUniqueID	uniqueID;
		OSType		portDriver;
		short		portNumber;
		short		output;
		char			reserved[10];
		Str255		name;
	} directDest;
} destinationMatch, *destinationMatchPtr;

/* match levels for MatchFMSDestination() */
enum {
	deviceMatch = 0,		/* The same device was found (same uniqueID).  Other device attributes may have changed. */
	nameMatch,			/* A device with the same name was found. */
	modelMatch,			/* A device with the same manufacturer and model information was found. */
	locationMatch,			/* A device on the same port and cable was found. */
	noMatch				/* No match was found.  A default device was selected. */
};

/* flags used for DrawFMSPopupMenu() and/or RunFMSPopupMenu() */
#define	shadedBoxFlag		1
#define	pullDownFlag		2
#define	multiSelectionFlag	4
#define	gridStyleFlag		8				/* Useful for patch list popups */
#define	menuGeneva9Flag	0x8000

typedef struct menuSelectionRec
{
	short		fmsAppID;
	short		classification;
	unsigned short	menuFlags;		/* Some combination of the flags above, or zero */
	Rect			menuBounds;
	long			selectionID;
	short		selectionInfo;
	Str255		selectionName;
} menuSelectionRec, *menuSelectionPtr;

/*	If (menuFlags & multiSelectionFlag), selectionInfo is the maximum number of allowable selections and selectionID is
	a multiSelectionHdl (defined below) */

typedef struct singleSelection
{
	long			selectionID;
	short		selectionInfo;
} singleSelection;

typedef struct multiSelectionRec
{
	short		selectionCount;		/* The number of current selections */
	singleSelection	selection[1];		/* Array is actually dynamic */
} multiSelectionRec, **multiSelectionHdl;

#define	NewMultiSelectionHandle(selCnt)	((multiSelectionHdl)NewHandleClear(sizeof(short) + sizeof(singleSelection) * selCnt))

typedef struct selectionListRec **selectionListHdl;

typedef struct listEntry
{
	long				selectionID;
	short			selectionInfo;
	selectionListHdl		subListH;
	Str255			name;
} listEntry, *listEntryPtr;

/* popup menu classifications and associated constants */

#define	fmsChannelPatches	3	/* Destination patch list.  selectionID = (long)uniqueID, selectionInfo = channel */
	#define	fmsPatchesNoMIDI			0x0100	/* Don't actually send patch change when selected */
#define	fmsPortDrivers	4	/* Port drivers in use (used internally).  selectionID = (long)driver, selectionInfo = 0 */
#define	fmsProperties		6	/* Device properties.  selectionID = (long)property, selectionInfo = 0 */
#define	fmsManufacturers	7	/* Device manufacturers.  selectionID = (long)manufacturer index, selectionInfo = 0 */
#define	fmsModels		8	/* Device models.  selectionID = (long)model index, selectionInfo = mfr index */
#define	fmsInterfaceChoices	9	/* Interface choices for MIDI ports (used internally) */
#define	fmsMIDIAssignments	10	/* FreeMIDI sources and/or destinations */
	#define	fmsMIDIIncludeSources		0x0100
	#define	fmsMIDIIncludeDestinations	0x0200
	#define	fmsMIDIIncludeVirtual		0x0400
	#define	fmsMIDIIncludeApplications	0x0800
	#define	fmsMIDIShowChannels		0x1000
	#define	fmsMIDIIncludeDevices		0x2000
	#define	fmsMIDIIncludeInactiveApps	0x4000
#define	fmsBankPatches	11	/* Bank patch list.  selectionID = (long)uniqueID, selectionInfo = bankRefNum */

/* Common MIDI source/destination classifications */
#define	fmsOutputDevices	(fmsMIDIAssignments + fmsMIDIIncludeDestinations + fmsMIDIIncludeDevices)
#define	fmsOutputChDevices	(fmsOutputDevices + fmsMIDIShowChannels)
#define	fmsAllOutputs		(fmsOutputDevices + fmsMIDIIncludeApplications + fmsMIDIIncludeVirtual)
#define	fmsAllChOutputs	(fmsAllOutputs + fmsMIDIShowChannels)

#define	fmsInputDevices	(fmsMIDIAssignments + fmsMIDIIncludeSources + fmsMIDIIncludeDevices)
#define	fmsInputChDevices	(fmsInputDevices + fmsMIDIShowChannels)
#define	fmsAllInputs		(fmsInputDevices + fmsMIDIIncludeApplications + fmsMIDIIncludeVirtual)
#define	fmsAllChInputs		(fmsAllInputs + fmsMIDIShowChannels)

#define	fmsApplications	(fmsMIDIAssignments + fmsMIDIIncludeDestinations + fmsMIDIIncludeApplications)
#define	fmsAllDevices		(fmsMIDIAssignments + fmsMIDIIncludeDevices)
#define	fmsIODevices		(fmsMIDIAssignments + fmsMIDIIncludeSources + fmsMIDIIncludeDestinations + fmsMIDIIncludeDevices)

/* Configuration Manager data */

/* device types */
enum {noDeviceType = 0, standardDeviceType, interfaceDeviceType, switcherDeviceType, standAloneDeviceType};
#define	numDeviceTypes	standAloneDeviceType

/* type masks */
#define	receiveDeviceMask		2
#define	sendDeviceMask		4
#define	interfaceMask			8
#define	switcherMask			16
#define	allDevicesMask			0xffff

/* bank and patch information */
typedef struct destinationPatchRec
{
	unsigned char	bankNumber0;
	unsigned char	bankNumber32;
	unsigned char	patchNumber;
	unsigned char	reserved;
	unsigned char	numberStr[8];
	Str255		name;
} destinationPatchRec, *destinationPatchPtr;

/* graphics types */
#define	picType		'PICT'
#define	icnType		'ICN#'
#define	icsType		'ics#'

typedef Boolean		(*FMCfgSignOutProcPtr)(long refCon);

enum {
	uppFMCfgSignOutProcInfo = kThinkCStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(Boolean)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long)))
};

#if GENERATINGCFM
typedef UniversalProcPtr	FMCfgSignOutProcUPP;

#define	CallFMCfgSignOutProc(proc, refCon)				\
			CallUniversalProc((UniversalProcPtr)(proc), uppFMCfgSignOutProcInfo, (refCon))
#define	NewFMCfgSignOutProc(proc)					\
			(FMCfgSignOutProcUPP)NewRoutineDescriptor((ProcPtr)(proc), uppFMCfgSignOutProcInfo, GetCurrentISA())
#else
typedef FMCfgSignOutProcPtr	FMCfgSignOutProcUPP;

#define	CallFMCfgSignOutProc(proc, refCon)				\
			(*(proc))(refCon)
#define	NewFMCfgSignOutProc(proc)					\
			(FMCfgSignOutProcPtr)(proc)
#endif

typedef struct configAppMethods
{
	FMCfgSignOutProcUPP	signOutProc;		/* Called when user decides to sign out present config app */
	long					signOutRefCon;
} configAppMethods;

typedef struct configAppInfo
{
	short			fmsAppID;
	short			supportedVersion;
	configAppMethods	methods;
} configAppInfo, *configAppInfoPtr;

/* property sets */
#define	kStandardPropertySet	0
#define	kUserPropertySet		1

/* standard properties (0 - 31) */
enum {kController = 0, kReceivesClock, kReceivesMTC, kTransmitsClock, kTransmitsMTC, 
	  kMIDIMachine, kGeneralMIDI, kBankSelect0, kBankSelect32, kDoesntAcceptProgramChanges,
	  kDoesntPlayNotes, kSampler, kDrumMachine, kEffectsUnit, kMixer, kGeneralMIDICh16Drums,
	  kPanDisruptsStereo};
/* 16 - 31 are still empty */

typedef struct deviceInfoRec
{
	short		deviceType;
	unsigned short	receiveChannels;	/* bit 0 corresponds to channel 1 */
	unsigned short	transmitChannels;
	short		numInputs;
	short		numOutputs;
	short		mfrIndex;
	short		modelIndex;
	short		deviceID;			/* actual MIDI value for deviceID.  stdDeviceInfoRec has offset value for display purposes */
	unsigned char	swProgram;		/* used only when device's output is connected to a switcher, high bit indicates "none" */
	unsigned char	dfltProgram;		/* used only when device is a switcher, high bit indicates "none" */
	short		delayMs;			/* number of milliseconds to wait after switching program, used for switchers only */
	short		graphicsSet;
	unsigned long	standardProperties;	/* bitfield of properties (defined above) */
	unsigned long	userProperties;	/* bitfield of user-defined properties */
	Str255		name;
} deviceInfoRec, *deviceInfoPtr;

typedef struct connectionRec
{
	fmsUniqueID	sourceUniqueID;
	short		output;
	fmsUniqueID	destinationUniqueID;
	short		input;
} connectionRec, *connectionPtr;

typedef struct deviceConnects
{
	short		numConnects;
	connectionRec	theConnections[1];	/* Array is actually dynamic (0 to numConnects - 1) */
} deviceConnects, **deviceConnectsHdl;

/* Connection masks for GetFMSDeviceConnections() */
#define		outConnectsMask		1
#define		inConnectsMask			2
#define		thruConnectsMask		4
#define		allConnectsMask		0xffff

typedef struct deviceModelInfo
{
	short		deviceType;
	unsigned char	mfrID[3];
	char			filler;
	unsigned char	family[2];
	unsigned char	member[2];
	unsigned short	receiveChannels;
	unsigned short	transmitChannels;
	short		numInputs;
	short		numOutputs;
	short		deviceIDRange[2];	/* range of possible device IDs for a device of this type */
	short		deviceIDOffset;		/* offset added to actual MIDI device ID for display purposes */
	short		dfltProgram;		/* for switchers only, default switcher program; -1 means "none" */
	short		delayMs;			/* for switchers only, milliseconds to wait after switching program */
	short		graphicsSet;
	unsigned long	standardProperties;	/* bitfield of properties */
	Str255		manufacturerName;
	Str255		modelName;
} deviceModelInfo, *deviceModelInfoPtr;

/* Database Manager data */

/* Patchlist formats */
enum {freeMidiPatchZeroBased = 0, freeMidiPatchOneBased, freeMidiPatchRolandBased};

/*	These requests are made by FreeMIDI when the user changes information about an opened device in the configuration
	application.  Currently, there is only one request message sent to database applications. */
#define	deviceIDChange		0		/* info = (long)new MIDI device ID */

typedef Boolean		(*FMDBDeviceProcPtr)(short request, fmsUniqueID uniqueID, long info, long refCon);

enum {
	uppFMDBDeviceProcInfo = kThinkCStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(Boolean)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(fmsUniqueID)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long)))
		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(long)))
};

#if GENERATINGCFM
typedef UniversalProcPtr	FMDBDeviceProcUPP;

#define	CallFMDBDeviceProc(proc, request, uniqueID, info, refCon)			\
			CallUniversalProc((UniversalProcPtr)(proc), uppFMDBDeviceProcInfo, (request), (uniqueID), (info), (refCon))
#define	NewFMDBDeviceProc(proc)									\
			(FMDBDeviceProcUPP)NewRoutineDescriptor((ProcPtr)(proc), uppFMDBDeviceProcInfo, GetCurrentISA())
#else
typedef FMDBDeviceProcPtr	FMDBDeviceProcUPP;

#define	CallFMDBDeviceProc(proc, request, uniqueID, info, refCon)			\
			(*(proc))((request), (uniqueID), (info), (refCon))
#define	NewFMDBDeviceProc(proc)									\
			(FMDBDeviceProcPtr)(proc)
#endif

typedef struct dbAppMethods
{
	FMDBDeviceProcUPP	deviceProc;
	long				deviceProcRefCon;
} dbAppMethods;

typedef struct dbAppInfoRec
{
	short		fmsAppID;	
	dbAppMethods	methods;
} dbAppInfoRec, *dbAppInfoPtr;

typedef struct stdDeviceInfoRec
{
	unsigned char	mfrCode[3];
	char			filler;
	unsigned char	familyCode[2];
	unsigned char	memberCode[2];
	short		deviceIDOffset;		/* offset added to device's deviceID for display purposes */
	Str255		mfrName;
	Str255		modelName;
} stdDeviceInfoRec, *stdDeviceInfoPtr;

enum {patchBankType = 0, noteNameBankType, noBankType = -1};

typedef struct bankInfoRec
{
	short		bankType;			/* one of the types enumerated above */
	OSType		bankCreator;		/* identifies application which created bank */
	unsigned short	channelMask;		/* identifies channels to which the bank is assigned */
	short		patchCount;
	unsigned char	midiBankNumber0;
	unsigned char	midiBankNumber32;
	Str255		bankName;
	short		bankCommandLen;	/* use 0 for standard bank select */
	unsigned char	*bankCommandP;
} bankInfoRec, *bankInfoPtr;

typedef struct patchInfoRec
{
	fmsUniqueID	uniqueID;
	short		bankRefNum;
	short		patchNumber;
	unsigned char	midiBankNumber0;
	unsigned char	midiBankNumber32;
	unsigned char	midiPatchNumber;
	char			reserved;
	short		bankTrigger;
	unsigned char	numberStr[8];
	Str255		patchName;
} patchInfoRec, *patchInfoPtr;

/* Timebase Mgr data */

/* Play modes */
enum {kStopped = 0, kPlaying, kPaused = 0x4000, kWaiting = 0x8000};

/* Sync modes */
#define	kUseSystemTimer		2		/* Application uses FreeMIDI clock (or other app's clock) for timing */
#define	kApplicationTimer		0		/* Application uses its own clock and updates FreeMIDI time */

#define	kRemoteOperation		1		/* Application's transport controls are linked with other synced apps */
#define	kNoRemoteOperation		0		/* Application's controls are not linked */

/* The following definitions provided for translating previously used constants */
#define	kTimerOff		(kNoRemoteOperation + kApplicationTimer)
#define	kApplSync		(kRemoteOperation + kApplicationTimer)
#define	kNoSync		(kNoRemoteOperation + kUseSystemTimer)
#define	kSystemSync	(kRemoteOperation + kUseSystemTimer)

/* Priority levels for FMSSetSyncMode() */
#define	kExternalSyncPriority	(-1)		/* Use this only when slaving to external device */
#define	kSyncPriority1			0		/* Professional sequencer priority */
#define	kSyncPriority2			1
#define	kSyncPriority3			2
#define	kSyncPriority4			3		/* Makeshift sequencer priority */
#define	kSyncNoPriority		4		/* Use this if your application never provides timing */

/* FreeMIDI error codes */
enum
{
	fmsUnknownPortTypeErr = 600,
	fmsAlreadyInitializedErr,
	fmsNotInitializedErr,
	fmsInvalidNumberErr,
	fmsPortNotInstalledErr,
	fmsPortTimeoutErr,
	fmsPortAlreadyInstalledErr,
	fmsUserCancelErr,
	fmsDuplicateIDErr,
	fmsInvalidClientErr,
	fmsDeviceIsOpenErr,
	fmsLimitReachedErr,
	fmsNotAvailableInThisSystemErr,
	fmsPlayingErr,
	fmsAlreadyConnectedErr,
	fmsInvalidTypeErr,
	fmsInvalidVersionErr,
	fmsCantDeleteInterfaceErr,
	fmsVersionMismatchErr
};

/* FreeMIDI Timing Services */

typedef enum FMSTMPriority {
	kFMSTMHighPriority = 0,
	kFMSTMMidPriority,
	kFMSTMLowPriority,
	kFMSTMNumberOfPriorityLevels
} FMSTMPriority;

enum { kFMSTMMaxTime = 0x7FFFFFFF };


//	This is an augmented structure which is used in place of TMTask.
//	Before calling FMSInsTime, you should fill in all the fields in fTask, as well as fNativeCallback, and fPriority.
//	If your task is native, put a pointer to it in fNativeCallback, as well as a UPP in fTask.tmAddr.
typedef struct FMSTMTask
{
	TMTask				fTask;
	FMSTMNativeCallback	fNativeCallback;	// PowerPC only; otherwise it should be NULL.
	unsigned short		fPriority;			// must be < kNumberOfPriorityLevels.

	//	During the callback, this is true if we're being polled, false if being called back by Time Manager.
	Boolean				fPolling;

	//	If you are called back with fPolling true and you still want to get the original
	//	wakeup you asked for, just set this to true.  Otherwise, don't touch it.
	Boolean				fPrimed;
} FMSTMTask;

#ifdef __cplusplus
}
#endif

#if defined(powerc) || defined(__powerc)
#pragma options	align=reset
#endif

#endif	/* __FMData__ */

/************************************ end FMData.h ************************************/