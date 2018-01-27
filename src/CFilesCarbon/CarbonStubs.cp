/* CarbonStubs.cp for Nightingale */

// MAS
#include "Nightingale_Prefix.pch"
// MAS
#include "CarbonStubs.h"

#define True 1
#define False 0

/* Nightingale hasn't supported OMS and FreeMIDI for many years -- they're relics of Mac
Classic (OS 9 and eariler), and it's unlikely it ever will again -- so I've removed a lot
of stuff for them. But there's more to do, both here and in the actual MIDI support files!
--DAB, Oct. 2017 */

/* OMSUtils */

OMSAPI(OMSBool) TestOMSDeviceMenu(OMSDeviceMenuH, OMSPoint )
{ return False; }

OMSAPI(short)	ClickOMSDeviceMenu(OMSDeviceMenuH )
{ return 0; }

OMSAPI(short)	OMSCountDeviceMenuItems(OMSDeviceMenuH)
{ return 0; }

OMSAPI(void)	DisposeOMSDeviceMenu(OMSDeviceMenuH )
{}

OMSAPI(void)	DrawOMSDeviceMenu(OMSDeviceMenuH )
{}

OMSAPI(void)	SetOMSDeviceMenuSelection(OMSDeviceMenuH, unsigned char, OMSUniqueID, OMSStringPtr, OMSBool)
{}

OMSAPI(short)	OMSUniqueIDToRefNum(OMSUniqueID)
{ return 0; }


// -------------------------------------------------------------------------------
// From MIDI.h

OSErr MIDISignIn(OSType, long, Handle, Str255)
{
	return noErr;
}

void MIDISignOut(OSType)
{}

void MIDISetCurTime(short /*refNum*/, long /*curTime*/)
{}	

long MIDIGetCurTime(short /*refNum*/)
{ return 0; }

void MIDIPoll(short /*refnum*/, long /*timeOffset*/)
{}

void MIDIWritePacket(short /*refNum*/, MMMIDIPacket */*mPacket*/)
{}	

void MIDIStartTime(short /*refNum*/)
{}

void MIDIStopTime(short /*refNum*/)
{}
