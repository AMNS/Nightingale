/* CoreMIDIUtils.c */

//#include <vector.h> //
// MAS: vector.h is now just vector. Use std namespace to avoid having to rename all uses of vector to std::vector
#include <vector>
using namespace std;
// MAS

#define CMInvalidRefNum		-1
#define CMPKT_HDR_SIZE			6

Boolean ResetMIDIPacketList();
void SetCMMIDIPacket(void);
void ClearCMMIDIPacket(void);
MIDIPacket *GetCMMIDIPacket(void);
MIDIPacket *PeekAtNextCMMIDIPacket(Boolean first);
void DeletePeekedAtCMMIDIPacket(void);
long CMTimeStampToMillis(MIDITimeStamp timeStamp);
void CMNormalizeTimeStamps(void);
void CloseCoreMidiInput(void);

void CMInitTimer(void);
void CMLoadTimer(short interruptPeriod);
void CMStartTime(void);
long CMGetCurTime(void);
void CMStopTime(void);
static void InitCoreMidiTimer();

void CMSetup(Document *doc, Byte *partChannel);
void CMTeardown(void);

OSStatus CMWritePacket(MIDIUniqueID destDevID, MIDITimeStamp tStamp, Byte *data);
OSStatus CMEndNoteNow(MIDIUniqueID destDevID, short noteNum, char channel);
OSStatus CMStartNoteNow(MIDIUniqueID destDevID, short noteNum, char channel, char velocity);
void CMFBOff(Document *doc);
void CMFBOn(Document *doc);

void CMMIDIFBNoteOn(Document *, short, short, MIDIUniqueID);
void CMMIDIFBNoteOff(Document *, short, short, MIDIUniqueID);

void CMFBNoteOn(Document *doc, short noteNum, short channel, short ioRefNum);
void CMFBNoteOff(Document *doc, short noteNum, short channel, short ioRefNum);
void CMAllNotesOff(void);

void CMDebugPrintXMission(void);

MIDIUniqueID GetMIDIObjectId(MIDIObjectRef obj);

Boolean CMRecvChannelValid(MIDIUniqueID devID, int channel);
Boolean CMTransmitChannelValid(MIDIUniqueID devID, int channel);

void CoreMidiSetSelectedInputDevice(MIDIUniqueID inputDevice, short inputChannel);
void CoreMidiSetSelectedMidiThruDevice(MIDIUniqueID thruDevice, short thruChannel);
OSStatus OpenCoreMidiInput(MIDIUniqueID inputDevice);

OSStatus CMMIDIController(MIDIUniqueID destDevID, char channel, Byte ctrlNum, Byte ctrlVal, MIDITimeStamp tstamp);
OSStatus CMMIDISustainOn(MIDIUniqueID destDevID, char channel, MIDITimeStamp tstamp);
OSStatus CMMIDISustainOff(MIDIUniqueID destDevID, char channel, MIDITimeStamp tstamp);
OSStatus CMMIDIPan(MIDIUniqueID destDevID, char channel, Byte panSetting, MIDITimeStamp tstamp);
OSStatus CMMIDIController(MIDIUniqueID destDevID, char channel, Byte ctrlNum, Byte ctrlVal);
OSStatus CMMIDISustainOn(MIDIUniqueID destDevID, char channel);
OSStatus CMMIDISustainOff(MIDIUniqueID destDevID, char channel);
OSStatus CMMIDIPan(MIDIUniqueID destDevID, char channel, Byte panSetting);

MIDIUniqueID GetCMDeviceForPartn(Document *doc, short partn);
MIDIUniqueID GetCMDeviceForPartL(Document *doc, LINK partL);
void SetCMDeviceForPartn(Document *doc, short partn, MIDIUniqueID device);
void SetCMDeviceForPartL(Document *doc, LINK partL, short partn, MIDIUniqueID device);
void InsertPartnCMDevice(Document *doc, short partn, short numadd);
void InsertPartLCMDevice(Document *doc, LINK partL, short numadd);
void DeletePartnCMDevice(Document *doc, short partn);
void DeletePartLCMDevice(Document *doc, LINK partL);

OSErr CMMIDIProgram(Document *, unsigned char *, unsigned char *);

short CMGetUseChannel(Byte partChannel[], short partn);

void CMGetNotePlayInfo(Document *doc, LINK aNoteL, short partTransp[],
						Byte partChannel[], SignedByte partVelo[],
						short *pUseNoteNum, short *pUseChan, short *pUseVelo);

Boolean GetCMPartPlayInfo(Document *doc, short partTransp[], Byte partChannel[],
							Byte partPatch[], SignedByte partVelo[], short partIORefNum[],
							MIDIUniqueID partDevice[]);
							
void GetCMNotePlayInfo(Document *doc, LINK aNoteL, short partTransp[],
								Byte partChannel[], SignedByte partVelo[], short partIORefNum[],
								short *pUseNoteNum, short *pUseChan, short *pUseVelo, short *puseIORefNum);

long FillCMSourcePopup(MenuHandle menu, vector<MIDIUniqueID> *vecDevices);
long FillCMDestinationPopup(MenuHandle menu, vector<MIDIUniqueID> *vecDevices);
long FillCMDestinationVec(vector<MIDIUniqueID> *vecDevices);

Boolean InitCoreMIDI();

/* CASoftMIDI.c */

Boolean SetupSoftMIDI(Byte *partChannel);
void TeardownSoftMIDI(void);
OSStatus SoftMIDISendMIDIEvent(MIDITimeStamp tStamp, UInt16 pktLen, Byte *data);

