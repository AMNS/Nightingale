/* CarbonStubs.cp for Nightingale */

// MAS
#include "Nightingale_Prefix.pch"
// MAS
#include "CarbonStubs.h"

/* Everywhere else in Nightingale they're used, True and False are defined, but I can't
find the #defines;  here they're _not_ defined, and I can't see what the difference is!
Strange.  --DAB, Oct. 2017 */
#define True 1
#define False 0

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


/* OMSUtils */

long	gmOMSBufferLength;	/* OMS MIDI Buffer size in bytes */
Boolean	gOMSMIDIBufferFull;

void SetOMSConfigMenuTitles(void)
{}

Boolean OMSInit(void)
{ return False; }

Boolean OMSClose(void)
{ return False; }

void InitOMSBuffer(void)
{}

MMMIDIPacket *GetOMSMIDIPacket(void)
{ return (MMMIDIPacket *)NULL; }

MMMIDIPacket *PeekAtNextOMSMIDIPacket(Boolean /*first*/)
{ return (MMMIDIPacket *)NULL; }

void DeletePeekedAtOMSMIDIPacket(void)
{}

void OMSInitTimer(void)
{}

void OMSLoadTimer(short /*interrruptPerios*/)
{}

void OMSStartTime(void)
{}

long OMSGetCurTime(void)
{ return 0; }

void OMSStopTime(void)
{}

void LaunchPatchBay(void)
{}

void OMSSetSelectedInputDevice(OMSUniqueID /*device*/, short /*channel*/)
{}

void OMSSetMidiThruDevice(OMSUniqueID /*device*/, short /*channel*/)
{}

void OMSEndNoteNow(short /*noteNum*/, char /*channel*/, short /*ioRefNum*/)
{}

void OMSStartNoteNow(short /*noteNum*/, char /*channel*/, char /*velocity*/, short /*ioRefNum*/)
{}

void OMSMIDIProgram(Document */*doc*/, unsigned char */*partPatch*/, 
						unsigned char */*partChannel*/, short */*partIORefNum*/)
{}

void OMSAllNotesOff2(void)
{}

void OMSFBOff(Document */*doc*/)
{}

void OMSFBNoteOff(Document */*doc*/, short /*noteNum*/, short /*channel*/, short /*ioRefNum*/)
{}

void OMSFBOn(Document */*doc*/)
{}

void OMSFBNoteOn(Document */*doc*/, short /*noteNum*/, short /*channel*/, short /*ioRefNum*/)
{}

void OMSEventLoopAction(void)
{}

void OMSResumeEvent(void)
{}

void OMSSuspendEvent(void)
{}

OMSDeviceMenuH CreateOMSOutputMenu(Rect */*menuBox*/)
{ return (OMSDeviceMenuH)0; }

OMSDeviceMenuH CreateOMSInputMenu(Rect */*menuBox*/)
{ return (OMSDeviceMenuH)0; }

OMSUniqueID GetOMSDeviceForPartn(Document */*doc*/, short /*partn*/)
{ return (OMSUniqueID)0; }

OMSUniqueID GetOMSDeviceForPartL(Document */*doc*/, LINK /*partL*/)
{ return (OMSUniqueID)0; }

void SetOMSDeviceForPartn(Document */*doc*/, short /*partn*/, OMSUniqueID /*device*/)
{}

void SetOMSDeviceForPartL(Document */*doc*/, LINK /*partL*/, OMSUniqueID /*device*/)
{}

void InsertPartnOMSDevice(Document */*doc*/, short /*partn*/, short /*numadd*/)
{}

void InsertPartLOMSDevice(Document */*doc*/, LINK /*partL*/, short /*numadd*/)
{}

void DeletePartnOMSDevice(Document */*doc*/, short /*partn*/)
{}

void DeletePartLOMSDevice(Document */*doc*/, LINK /*partL*/)
{}

Boolean OMSChannelValid(OMSUniqueID /*device*/, short /*channel*/)
{ return False; }

OMSErr OpenOMSInput(OMSUniqueID /*device*/)
{ return noErr; }

void CloseOMSInput(OMSUniqueID /*device*/)
{}

Boolean GetOMSPartPlayInfo(Document *, short [], Byte [],
							Byte [], SignedByte [], short [],
							OMSUniqueID [])
{ return False; }

void GetOMSNotePlayInfo(Document *, LINK , short [],
						Byte [], SignedByte [], short [],
						short *, short *, short *, short *)
{}

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

/* exported globals */
long gmFMSBufferLength;				/* Our FMSPacket buffer size in bytes */
Boolean gFMSMIDIBufferFull;

MMMIDIPacket *GetFMSMIDIPacket(void) { return (MMMIDIPacket*)NULL; }
MMMIDIPacket *PeekAtNextFMSMIDIPacket(Boolean first) { return (MMMIDIPacket*)NULL; }
void DeletePeekedAtFMSMIDIPacket(void) {}
void InitFMSBuffer(void) {}
void FillFMSBuffer(void) {}
void FMSSetSelectedInputDevice(fmsUniqueID device, short channel) {}

void FMSSetMIDIThruDevice(fmsUniqueID device, short channel) {}
void FMSSetMIDIThruDeviceFromConfig(void) {}

void FMSInitTimer(void) {}
void FMSLoadTimer(short interruptPeriod) {}
void FMSStartTime(void) {}
long FMSGetCurTime(void) { return 0; }
void FMSStopTime(void) {}
void FMSEndNoteNow(short noteNum, char channel, fmsUniqueID device) {}
void FMSStartNoteNow(short noteNum, char channel, char velocity, fmsUniqueID device) {}

void FMSFBOn(Document *)
{}
void FMSFBOff(Document *)
{}

void FMSFBNoteOn(Document *, short , short , fmsUniqueID )
{}
void FMSFBNoteOff(Document *, short , short , fmsUniqueID )
{}

void FMSMIDIOneProgram(fmsUniqueID , Byte , Byte , Byte , Byte )
{}

void FMSMIDIProgram(Document *doc) {}
void FMSAllNotesOff(void) {}

void FMSSetInputDestinationMatch(Document *doc) {}
short FMSGetInputDestinationMatch(Document *doc) { return 0; }

void FMSSetOutputDestinationMatch(Document *, LINK)
{}

//short FMSGetOutputDestinationMatch(Document *doc, LINK partL, Boolean replace);

short FMSCheckPartDestinations(Document *, Boolean)
{ return 0; }

void FMSSetNewDocRecordDevice(Document *)
{}

void FMSSetNewPartDevice(LINK)
{}

//short FMSCheckInputDevice(fmsUniqueID *device, short *channel, Boolean replace);

short FMSCheckOutputDevice(fmsUniqueID *, short *, Boolean )
{ return 0; }

//short FMSCheckMetroDevice(Boolean replace);
//short FMSCheckThruDevice(Boolean replace);
//void FMSCheckDocOutputDestinations(Document *doc);
//void FMSCheckAllInputDestinations(void);
//void FMSCheckAllOutputDestinations(void);

//void OpenFMSConfiguration(void);

void SetFMSConfigMenuTitles(void)
{}

//Boolean FMSInit(void);
//Boolean FMSClose(void);

void FMSResumeEvent(void)
{}

void FMSSuspendEvent(void)
{}

void DrawFMSDeviceMenu(const Rect *, const fmsUniqueID , const short )
{}

void DrawFMSPatchMenu(const Rect *, const fmsUniqueID , const short )
{}

Boolean RunFMSDeviceMenu(const Rect *, fmsUniqueID *, short *)
{ return True; }

Boolean RunFMSPatchMenu(const Rect *, const fmsUniqueID , const short )
{ return True; }

Boolean GetFMSCurrentPatchInfo(const fmsUniqueID , const short , short *, short *, short *)
{ return True; }

fmsUniqueID GetFMSDeviceForPartn(Document *, short)
{ return (fmsUniqueID)0; }

fmsUniqueID GetFMSDeviceForPartL(Document *doc, LINK partL) { return (fmsUniqueID)0; }
//void SetFMSDeviceForPartn(Document *doc, short partn, fmsUniqueID device);

void SetFMSDeviceForPartL(Document *, LINK, fmsUniqueID)
{}

Boolean FMSChannelValid(fmsUniqueID device, short channel) { return True; }

Boolean GetFMSPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
						Byte partPatch[], Byte partBankSel0[], Byte partBankSel32[],
						SignedByte partVelo[], fmsUniqueID partDevice[]) { return True; }
						
void GetFMSNotePlayInfo(Document *doc, LINK aNoteL, short partTransp[],
				Byte partChannel[], SignedByte partVelo[], fmsUniqueID partDevice[],
				short *pUseNoteNum, short *pUseChan, short *pUseVelo, fmsUniqueID *puseDevice) {}

// -------------------------------------------------------------------------------

/* MP2.2.h header file for MidiPascal 3.0 MIDI Driver, used with LSC4.0
	(c) 1990 James Chandler Jr. and Altech Systems.
	This version dated 7/30/90
*/

//pascal void InitMidi(unsigned int InSize, unsigned int OutSize)
pascal void InitMidi(unsigned int , unsigned int )
{}

//pascal void MidiIn(int * MidiByte, long * TimeStamp)
pascal void MidiIn(int * , long * )
{}

//pascal void MidiOut(int Midibyte, long TimeStamp, int * Result)
pascal void MidiOut(int , long , int * )
{}

//pascal void MidiNow(int Statusbyte, int Databyte1, int Databyte2, int * Result)
pascal void MidiNow(int , int , int , int * )
{}

//pascal void GetMidi(unsigned char * MidiString, long RequestCount, long * ReturnCount)
pascal void GetMidi(unsigned char * , long , long * )
{}

//pascal void SendMidi(unsigned char *MidiString, long SendCount, int *Result)
pascal void SendMidi(unsigned char *, long , int *)
{}

//pascal void InCount(unsigned int *Count)
pascal void InCount(unsigned int *)
{}

//pascal void OutCount(unsigned int *Count)
pascal void OutCount(unsigned int *)
{}

pascal void MidiThru(int)
{}

pascal void ReChannel(int)
{}

//pascal void Midi(int Arg)
pascal void Midi(int )
{}

//pascal void MidiPort(int Arg)
pascal void MidiPort(int )
{}

//pascal void MidiFilter(int EnaDisa, int Lower, int Upper)
pascal void MidiFilter(int , int , int )
{}

//pascal void MidiTime(unsigned int ticklength)
pascal void MidiTime(unsigned int )
{}

pascal void MidiStopTime(void)
{}

pascal void MidiStartTime(void)
{}

//pascal void MidiSetTime(long timestamp)
pascal void MidiSetTime(long )
{}

//pascal void MidiGetTime(long * timestamp)
pascal void MidiGetTime(long * )
{}

//pascal void MidiSync(int mode, int syncticks)
pascal void MidiSync(int , int )
{}

pascal void QuitMidi(void)
{}

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
