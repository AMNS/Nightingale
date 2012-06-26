#include "OMSCompat.h"
#include <CoreMIDI/MidiServices.h>

void MIDIWritePacket(short refNum, MMMIDIPacket *mPacket);		

void MIDIStartTime(short refNum);
void MIDISetCurTime(short refNum, long curTime);	
long MIDIGetCurTime(short refNum);
void MIDIStopTime(short refNum);
	
OSErr MIDISignIn(OSType clientId, long refCon, Handle iconHdl, Str255 str);
void MIDISignOut(OSType);

void MIDIPoll(short refnum, long timeOffset);

// From OMS.h

OMSAPI(short)	OMSUniqueIDToRefNum(OMSUniqueID uniqueID);

// From FMData.h

/* match levels for MatchFMSDestination() */
enum {
	deviceMatch = 0,		/* The same device was found (same uniqueID).  Other device attributes may have changed. */
	nameMatch,			/* A device with the same name was found. */
	modelMatch,			/* A device with the same manufacturer and model information was found. */
	locationMatch,			/* A device on the same port and cable was found. */
	noMatch				/* No match was found.  A default device was selected. */
};

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
