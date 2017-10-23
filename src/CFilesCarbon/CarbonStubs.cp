/* CarbonStubs.cp for Nightingale */

// MAS
#include "Nightingale_Prefix.pch"
// MAS
#include "CarbonStubs.h"

#define True 1
#define False 0

/* I'd guess these "Console" functions were for handling a UNIX-style "console" log,
but Nightingale hasn't used them since v. 5.3 at the latest, and I don't even know
where the source code is now. Still, it might be worth keeping the prototypes. --DAB,
Oct. 2017 */

#ifdef NOMORE
short InstallConsole(short /*fd*/)
{ return 0; }

void RemoveConsole(void)
{}

long WriteCharsToConsole(char */*buffer*/, long /*n*/)
{ return 0; }

long WriteCharsToErrorConsole(char */*buffer*/, long /*n*/)
{ return 0; }

long ReadCharsFromConsole(char */*buffer*/, long /*n*/)
{ return 0; }
#endif


/* Nightingale hasn't supported OMS and FreeMIDI for many years, and it's unlikely it
ever will again, so I've removed a lot of stuff for them. But there's more to do, both
here and in the actual MIDI support files! --DAB, Oct. 2017 */

/* OMSUtils */

#ifdef NOMORE
long	gmOMSBufferLength;	/* OMS MIDI Buffer size in bytes */
Boolean	gOMSMIDIBufferFull;
#endif


OMSAPI(OMSBool) TestOMSDeviceMenu(OMSDeviceMenuH , OMSPoint )
{ return False; }

OMSAPI(short)	ClickOMSDeviceMenu(OMSDeviceMenuH )
{ return 0; }

OMSAPI(short)	OMSCountDeviceMenuItems( OMSDeviceMenuH  )
{ return 0; }

OMSAPI(void)	DisposeOMSDeviceMenu(OMSDeviceMenuH )
{}

OMSAPI(void)	DrawOMSDeviceMenu(OMSDeviceMenuH )
{}

OMSAPI(void)	SetOMSDeviceMenuSelection(OMSDeviceMenuH , unsigned char , OMSUniqueID , OMSStringPtr , OMSBool )
{}

OMSAPI(short)	OMSUniqueIDToRefNum(OMSUniqueID)
{ return 0; }

/* FreeMIDIUtils.h for Nightingale */

#ifdef NOMORE
/* exported globals */
long gmFMSBufferLength;				/* Our FMSPacket buffer size in bytes */
Boolean gFMSMIDIBufferFull;
#endif


// -------------------------------------------------------------------------------
// From MIDI.h

OSErr MIDISignIn(OSType , long , Handle , Str255 )
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


/* Miscellaneous */

// Called from DoFileMenu [Menu.c]

Boolean	DoPostScript(Document */*doc*/)
{ return False; }

// MIDIRecord.c

//Boolean StepRecord(Document *, Boolean ) { return True; }

//Boolean Record(Document *) { return True; }

//Boolean RTMRecord(Document *) { return True; }
