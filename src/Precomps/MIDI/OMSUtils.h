/* OMSUtils.h for Nightingale
   Copyright © 1988-97 by Advanced Music Notation Systems, Inc. All Rights Reserved. */

#ifndef _OMSUTILS_
#define _OMSUTILS_
#pragma once

//#if TARGET_API_MAC_CARBON_MACHO
#include <CoreMIDI/MIDIServices.h>		/* for MIDIPacket */
//#else
//#include <midi.h>						/* for MIDIPacket */
//#endif

void SetOMSConfigMenuTitles(void);
Boolean OMSInit(void);
Boolean OMSClose(void);
void InitOMSBuffer(void);
MMMIDIPacket *GetOMSMIDIPacket(void);
MMMIDIPacket *PeekAtNextOMSMIDIPacket(Boolean first);
void DeletePeekedAtOMSMIDIPacket(void);
void OMSInitTimer(void);
void OMSLoadTimer(INT16 interruptPeriod);
void OMSStartTime(void);
long OMSGetCurTime(void);
void OMSStopTime(void);
void LaunchPatchBay(void);
void OMSSetSelectedInputDevice(OMSUniqueID device, INT16 channel);
void OMSSetMidiThruDevice(OMSUniqueID device, INT16 channel);
void OMSEndNoteNow(INT16 noteNum, char channel, short ioRefNum);
void OMSStartNoteNow(INT16 noteNum, char channel, char velocity, short ioRefNum);
void OMSMIDIProgram(Document *doc, unsigned char *partPatch, 
						unsigned char *partChannel, short *partIORefNum);
void OMSAllNotesOff2(void);

void OMSFBOff(Document *doc);
void OMSFBNoteOff(Document *doc, INT16 noteNum, INT16 channel, short ioRefNum);
void OMSFBOn(Document *doc);
void OMSFBNoteOn(Document *doc, INT16 noteNum, INT16 channel, short ioRefNum);

void OMSEventLoopAction(void);
void OMSResumeEvent(void);
void OMSSuspendEvent(void);
OMSDeviceMenuH CreateOMSOutputMenu(Rect *menuBox);
OMSDeviceMenuH CreateOMSInputMenu(Rect *menuBox);
OMSUniqueID GetOMSDeviceForPartn(Document *doc, INT16 partn);
OMSUniqueID GetOMSDeviceForPartL(Document *doc, LINK partL);
void SetOMSDeviceForPartn(Document *doc, INT16 partn, OMSUniqueID device);
void SetOMSDeviceForPartL(Document *doc, LINK partL, OMSUniqueID device);
void InsertPartnOMSDevice(Document *doc, INT16 partn, INT16 numadd);
void InsertPartLOMSDevice(Document *doc, LINK partL, INT16 numadd);
void DeletePartnOMSDevice(Document *doc, INT16 partn);
void DeletePartLOMSDevice(Document *doc, LINK partL);
Boolean OMSChannelValid(OMSUniqueID device, INT16 channel);
OMSErr OpenOMSInput(OMSUniqueID device);
void CloseOMSInput(OMSUniqueID device);
Boolean GetOMSPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
							Byte partPatch[], SignedByte partVelo[], short partIORefNum[],
							OMSUniqueID partDevice[]);
void GetOMSNotePlayInfo(Document *doc, LINK aNoteL, short partTransp[],
						Byte partChannel[], SignedByte partVelo[], short partIORefNum[],
						INT16 *pUseNoteNum, INT16 *pUseChan, INT16 *pUseVelo, short *puseIORefNum);

#endif