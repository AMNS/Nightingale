/* FreeMIDIUtils.h for Nightingale
   Copyright © 2001 by Adept Music Notation Solutions, Inc. All Rights Reserved. */

#ifndef _FREEMIDIUTILS_
#define _FREEMIDIUTILS_

// MAS : need to find Xcode equivalent of TARGET_API_...
//#if TARGET_API_MAC_CARBON_MACHO
// MAS : redundant, already included in applicationTypes
//#include <CoreMIDI/MIDIServices.h>		/* for MIDIPacket */
// MAS
//#else
//#include <midi.h>						/* for MIDIPacket */
//#endif
// MAS

MMMIDIPacket *GetFMSMIDIPacket(void);
MMMIDIPacket *PeekAtNextFMSMIDIPacket(Boolean first);
void DeletePeekedAtFMSMIDIPacket(void);
void InitFMSBuffer(void);
void FillFMSBuffer(void);
void FMSSetSelectedInputDevice(fmsUniqueID device, INT16 channel);
void FMSSetMIDIThruDevice(fmsUniqueID device, INT16 channel);
void FMSSetMIDIThruDeviceFromConfig(void);
void FMSInitTimer(void);
void FMSLoadTimer(INT16 interruptPeriod);
void FMSStartTime(void);
long FMSGetCurTime(void);
void FMSStopTime(void);
void FMSEndNoteNow(INT16 noteNum, char channel, fmsUniqueID device);
void FMSStartNoteNow(INT16 noteNum, char channel, char velocity, fmsUniqueID device);
void FMSFBOn(Document *doc);
void FMSFBOff(Document *doc);
void FMSFBNoteOn(Document *doc, INT16 noteNum, INT16 channel, fmsUniqueID device);
void FMSFBNoteOff(Document *doc, INT16 noteNum, INT16 channel, fmsUniqueID device);
void FMSMIDIOneProgram(fmsUniqueID device, Byte channel,
						Byte patchNum, Byte bankSelect0, Byte bankSelect32);
void FMSMIDIProgram(Document *doc);
void FMSAllNotesOff(void);

void FMSSetInputDestinationMatch(Document *doc);
short FMSGetInputDestinationMatch(Document *doc);
void FMSSetOutputDestinationMatch(Document *doc, LINK partL);
short FMSGetOutputDestinationMatch(Document *doc, LINK partL, Boolean replace);
short FMSCheckPartDestinations(Document *doc, Boolean replace);
void FMSSetNewDocRecordDevice(Document *doc);
void FMSSetNewPartDevice(LINK partL);
short FMSCheckInputDevice(fmsUniqueID *device, short *channel, Boolean replace);
short FMSCheckOutputDevice(fmsUniqueID *device, short *channel, Boolean replace);
short FMSCheckMetroDevice(Boolean replace);
short FMSCheckThruDevice(Boolean replace);
void FMSCheckDocOutputDestinations(Document *doc);
void FMSCheckAllInputDestinations(void);
void FMSCheckAllOutputDestinations(void);

void OpenFMSConfiguration(void);
void SetFMSConfigMenuTitles(void);
Boolean FMSInit(void);
Boolean FMSClose(void);
void FMSResumeEvent(void);
void FMSSuspendEvent(void);
void DrawFMSDeviceMenu(const Rect *boundsRect, const fmsUniqueID device, const short channel);
void DrawFMSPatchMenu(const Rect *boundsRect, const fmsUniqueID device, const short channel);
Boolean RunFMSDeviceMenu(const Rect *boundsRect, fmsUniqueID *device, short *channel);
Boolean RunFMSPatchMenu(const Rect *boundsRect, const fmsUniqueID device, const short channel);
Boolean GetFMSCurrentPatchInfo(const fmsUniqueID device, const short channel,
							short *patchNum, short *bankNumber0, short *bankNumber32);
fmsUniqueID GetFMSDeviceForPartn(Document *doc, INT16 partn);
fmsUniqueID GetFMSDeviceForPartL(Document *doc, LINK partL);
void SetFMSDeviceForPartn(Document *doc, INT16 partn, fmsUniqueID device);
void SetFMSDeviceForPartL(Document *doc, LINK partL, fmsUniqueID device);
Boolean FMSChannelValid(fmsUniqueID device, INT16 channel);
Boolean GetFMSPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
						Byte partPatch[], Byte partBankSel0[], Byte partBankSel32[],
						SignedByte partVelo[], fmsUniqueID partDevice[]);
void GetFMSNotePlayInfo(Document *doc, LINK aNoteL, short partTransp[],
				Byte partChannel[], SignedByte partVelo[], fmsUniqueID partDevice[],
				INT16 *pUseNoteNum, INT16 *pUseChan, INT16 *pUseVelo, fmsUniqueID *puseDevice);

#endif /* _FREEMIDIUTILS_ */