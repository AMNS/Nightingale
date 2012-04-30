// MAS #include "MIDIPASCAL3.h"
#include "OMSCompat.h"
//#include "MIDI.h"
#include <CoreMIDI/MidiServices.h>

#if 1
void MIDIWritePacket(short refNum, MMMIDIPacket *mPacket);		

void MIDIStartTime(short refNum);
void MIDISetCurTime(short refNum, long curTime);	
long MIDIGetCurTime(short refNum);
void MIDIStopTime(short refNum);
	
OSErr MIDISignIn(OSType clientId, long refCon, Handle iconHdl, Str255 str);
void MIDISignOut(OSType);

void MIDIPoll(short refnum, long timeOffset);
#endif

// From OMS.h

OMSAPI(short)	OMSUniqueIDToRefNum(OMSUniqueID uniqueID);

// From FMData.h

#if 1
/* match levels for MatchFMSDestination() */
enum {
	deviceMatch = 0,		/* The same device was found (same uniqueID).  Other device attributes may have changed. */
	nameMatch,			/* A device with the same name was found. */
	modelMatch,			/* A device with the same manufacturer and model information was found. */
	locationMatch,			/* A device on the same port and cable was found. */
	noMatch				/* No match was found.  A default device was selected. */
};
#endif

// From MIDI.h

#if TARGET_API_MAC_CARBON_MACHO

typedef MMMIDIPacket *                    MMMIDIPacketPtr;
typedef CALLBACK_API( short , MIDIReadHookProcPtr )(MMMIDIPacketPtr myPacket, long myRefCon);

typedef STACK_UPP_TYPE(MIDIReadHookProcPtr)                     MIDIReadHookUPP;

struct MIDIClkInfo {
  short               syncType;               /*synchronization external/internal*/
  long                curTime;                /*current value of port's clock*/
  short               format;                 /*time code format*/
};
typedef struct MIDIClkInfo              MIDIClkInfo;
struct MIDIIDRec {
  OSType              clientID;
  OSType              portID;
};
typedef struct MIDIIDRec                MIDIIDRec;
struct MIDIPortInfo {
  short               portType;               /*type of port*/
  MIDIIDRec           timeBase;               /*MIDIIDRec for time base*/
  short               numConnects;            /*number of connections*/
  MIDIIDRec           cList[1];               /*ARRAY [1..numConnects] of MIDIIDRec*/
};
typedef struct MIDIPortInfo             MIDIPortInfo;
typedef MIDIPortInfo *                  MIDIPortInfoPtr;
typedef MIDIPortInfoPtr *               MIDIPortInfoHdl;
typedef MIDIPortInfoPtr *               MIDIPortInfoHandle;

struct MIDIPortParams {
  OSType              portID;                 /*ID of port, unique within client*/
  short               portType;               /*Type of port - input, output, time, etc.*/
  short               timeBase;               /*refnum of time base, 0 if none*/
  long                offsetTime;             /*offset for current time stamps*/
  MIDIReadHookUPP     readHook;               /*routine to call when input data is valid*/
  long                refCon;                 /*refcon for port (for client use)*/
  MIDIClkInfo         initClock;              /*initial settings for a time base*/
  Str255              name;                   /*name of the port, This is a real live string, not a ptr.*/
};
typedef struct MIDIPortParams           MIDIPortParams;
typedef MIDIPortParams *                MIDIPortParamsPtr;

#endif

// MAS disabling; no MIDIReadHookUPP
//EXTERN_API_C( MIDIReadHookUPP )
//NewMIDIReadHookUPP(MIDIReadHookProcPtr userRoutine);
#if !OPAQUE_UPP_TYPES
//  enum { uppMIDIReadHookProcInfo = 0x000003E0 };  /* pascal 2_bytes Func(4_bytes, 4_bytes) */
  #ifdef __cplusplus
//    inline MIDIReadHookUPP NewMIDIReadHookUPP(MIDIReadHookProcPtr userRoutine) { return (MIDIReadHookUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppMIDIReadHookProcInfo, GetCurrentArchitecture()); }
  #else
//    #define NewMIDIReadHookUPP(userRoutine) (MIDIReadHookUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppMIDIReadHookProcInfo, GetCurrentArchitecture())
  #endif
#endif

EXTERN_API( OSErr )
MIDIConnectData(
  OSType   srcClID,
  OSType   srcPortID,
  OSType   dstClID,
  OSType   dstPortID)	FOURWORDINLINE(0x203C, 0x0024, 0x0004, 0xA800);

EXTERN_API( OSErr )
MIDIConnectTime(
  OSType   srcClID,
  OSType   srcPortID,
  OSType   dstClID,
  OSType   dstPortID)	FOURWORDINLINE(0x203C, 0x002C, 0x0004, 0xA800);

EXTERN_API( void )
MIDISetSync(
  short   refnum,
  short   sync)         FOURWORDINLINE(0x203C, 0x0054, 0x0004, 0xA800);

// MAS disabling
//EXTERN_API( MIDIPortInfoHandle )
//MIDIGetPortInfo(
//  OSType   clientID,
//  OSType   portID)      FOURWORDINLINE(0x203C, 0x0020, 0x0004, 0xA800);

//EXTERN_API( OSErr )
//MIDIAddPort(
//  OSType              clientID,
//  short               BufSize,
//  short *             refnum,
//  MIDIPortParamsPtr   init)	FOURWORDINLINE(0x203C, 0x001C, 0x0004, 0xA800);
