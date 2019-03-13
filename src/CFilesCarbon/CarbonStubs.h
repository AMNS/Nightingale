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
