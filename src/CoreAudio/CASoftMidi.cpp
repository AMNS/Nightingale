// MAS
#include "Nightingale_Prefix.pch"

// This is a simple test case of making an simple graph with a DLSSynth, Limiter and Ouput unit

// we're going to use a graph because its easier for it to just handle the setup and connections, etc...

#include <CoreServices/CoreServices.h> //for file stuff
//#include <AudioUnit/AudioUnit.h>
//#include <AudioToolbox/AudioToolbox.h> //for AUGraph
#include <pthread.h> // used for usleep...

#include "CoreMidiDefs.h"

OSStatus CreateAUMIDIController()
{
	OSStatus result = noErr;
	
	result = AUMIDIControllerCreate(CFSTR("DLS_MIDIController"), &auMidiController);

	return result;
}

// This call creates the Graph and the Synth unit...

OSStatus CreateAUGraph (AUGraph &outGraph, AudioUnit &outSynth)
{
	OSStatus result;
	AUNode synthNode, limiterNode, outNode;	//create the nodes of the graph
	
	ComponentDescription cd;
	cd.componentManufacturer = kAudioUnitManufacturer_Apple;
	cd.componentFlags = 0;
	cd.componentFlagsMask = 0;

	require_noerr (result = NewAUGraph (&outGraph), home);

	cd.componentType = kAudioUnitType_MusicDevice;
	cd.componentSubType = kAudioUnitSubType_DLSSynth;

	require_noerr (result = AUGraphNewNode (outGraph, &cd, 0, NULL, &synthNode), home);

	cd.componentType = kAudioUnitType_Effect;
	cd.componentSubType = kAudioUnitSubType_PeakLimiter;  

	require_noerr (result = AUGraphNewNode (outGraph, &cd, 0, NULL, &limiterNode), home);

	cd.componentType = kAudioUnitType_Output;
	cd.componentSubType = kAudioUnitSubType_DefaultOutput;  
	require_noerr (result = AUGraphNewNode (outGraph, &cd, 0, NULL, &outNode), home);
	
	require_noerr (result = AUGraphOpen (outGraph), home);
	
	require_noerr (result = AUGraphConnectNodeInput (outGraph, synthNode, 0, limiterNode, 0), home);
	require_noerr (result = AUGraphConnectNodeInput (outGraph, limiterNode, 0, outNode, 0), home);
	
		// ok we're good to go - get the Synth Unit...
		
	require_noerr (result = AUGraphGetNodeInfo(outGraph, synthNode, 0, 0, 0, &outSynth), home);

home:
	return result;
}


static OSStatus PathToFSSpec(const char *filename, FSSpec &outSpec)
{	
	FSRef fsRef;
	OSStatus result;
	require_noerr (result = FSPathMakeRef ((const UInt8*)filename, &fsRef, 0), home);
	require_noerr (result = FSGetCatalogInfo(&fsRef, kFSCatInfoNone, NULL, NULL, &outSpec, NULL), home);

home:
	return result;
}


/*
int PlaySoftMIDI (int argc, const char * argv[]) {
	AUGraph graph = 0;
	AudioUnit synthUnit;
	OSStatus result;
	char* bankPath = 0;
	
	UInt8 midiChannelInUse = 0; //we're using MIDI channel 1...
	
		// this is the only option to main that we have...
		// just the full path of the sample bank...
		
		// On OS X there are known places were sample banks can be stored
		// Library/Audio/Sounds/Banks - so you could scan this directory and give the user options
		// about which sample bank to use...
	if (argc > 1)
		bankPath = const_cast<char*>(argv[1]);
	
	require_noerr (result = CreateAUGraph (graph, synthUnit), home);
	
	// if the user supplies a sound bank, we'll set that before we initialize and start playing
	if (bankPath) 
	{
		FSSpec soundBankSpec;
		require_noerr (result = PathToFSSpec (bankPath, soundBankSpec), home);
		
		printf ("Setting Sound Bank:%s\n", bankPath);
		
		require_noerr (result = AudioUnitSetProperty (synthUnit,
											kMusicDeviceProperty_SoundBankFSSpec,
											kAudioUnitScope_Global, 0,
											&soundBankSpec, sizeof(soundBankSpec)), home);
    
	}
	
	// ok we're set up to go - initialize and start the graph
	require_noerr (result = AUGraphInitialize (graph), home);

		//set our bank
	require_noerr (result = MusicDeviceMIDIEvent(synthUnit, 
								kMidiMessage_ControlChange << 4 | midiChannelInUse, 
								kMidiMessage_BankMSBControl, 0,
								0											//sample offset
								), home);

	require_noerr (result = MusicDeviceMIDIEvent(synthUnit, 
								kMidiMessage_ControlChange << 4 | midiChannelInUse, 
								kMidiMessage_BankLSBControl, 0,
								0											//sample offset
								), home);
								
	Byte b0 = kMidiMessage_ProgramChange << 4 | midiChannelInUse;

	require_noerr (result = MusicDeviceMIDIEvent(synthUnit, 
								kMidiMessage_ProgramChange << 4 | midiChannelInUse, 
								74, 										//prog change num
								0,
								0											//sample offset
								), home);

	CAShow (graph); 		// prints out the graph so we can see what it looks like...
	
	require_noerr (result = AUGraphStart (graph), home);

	// we're going to play an octave of MIDI notes: one a half second
	for (int i = 0; i < 13; i++) {
		UInt32 noteNum = i + 60;
		UInt32 onVelocity = 100;
		UInt32 noteOnCommand = 	kMidiMessage_NoteOn << 4 | midiChannelInUse;
			
			printf ("Playing Note: Status: 0x%lX, Note: %ld, Vel: %ld\n", noteOnCommand, noteNum, onVelocity);
		
		require_noerr (result = MusicDeviceMIDIEvent(synthUnit, noteOnCommand, noteNum, onVelocity, 0), home);
		
			// sleep for a half second
		usleep (1 * 500 * 1000);

		require_noerr (result = MusicDeviceMIDIEvent(synthUnit, noteOnCommand, noteNum, 0, 0), home);
	}
	
	// ok we're done now

	require_noerr (result = MusicDeviceMIDIEvent(synthUnit, 
								kMidiMessage_ProgramChange << 4 | midiChannelInUse, 
								1, 										//prog change num
								0,
								0											//sample offset
								), home);

	// we're going to play an octave of MIDI notes: one a half second
	for (int j = 12; j >=0; j--) {
		UInt32 noteNum = j + 60;
		UInt32 onVelocity = 127;
		UInt32 noteOnCommand = 	kMidiMessage_NoteOn << 4 | midiChannelInUse;
			
			printf ("Playing Note: Status: 0x%lX, Note: %ld, Vel: %ld\n", noteOnCommand, noteNum, onVelocity);
		
		require_noerr (result = MusicDeviceMIDIEvent(synthUnit, noteOnCommand, noteNum, onVelocity, 0), home);
		
			// sleep for a half second
		usleep (1 * 500 * 1000);

		require_noerr (result = MusicDeviceMIDIEvent(synthUnit, noteOnCommand, noteNum, 0, 0), home);
	}
	
home:
	if (graph) {
		AUGraphStop (graph); // stop playback - AUGraphDispose will do that for us but just showing you what to do
		DisposeAUGraph (graph);
	}
	return result;
}
*/

Boolean SetupSoftMIDI(Byte *activeChannel)
{
	Boolean debug = false;
	
	if (debug) {
 		//PlaySoftMIDI(0, NULL);
 	}
 	
	OSStatus result = noErr;
	UInt8 midiChannelInUse = 0; 								// we're using MIDI channel 1...
	UInt8 midiChannel;
	
	require_noerr (result = CreateAUMIDIController (), home);
	
	require_noerr (result = CreateAUGraph (graph, synthUnit), home);
	
	if (debug) {
		LogPrintf(LOG_NOTICE, "1. created AU graph\n");
	}	
	
		// ok we're set up to go - initialize and start the graph
	
	require_noerr (result = AUGraphInitialize (graph), home);

		//set our bank
		
	for (int i = 0; i<=MAXCHANNEL; i++)
		if (activeChannel[i]) 
		{
			midiChannel = i;
			midiChannelInUse |= midiChannel;
		}
			
	require_noerr (result = MusicDeviceMIDIEvent(synthUnit, 
								kMidiMessage_ControlChange << 4 | midiChannelInUse, 
								kMidiMessage_BankMSBControl, 0,
								0),										//sample offset
								home);

	// This will be reset
	
	require_noerr (result = MusicDeviceMIDIEvent(synthUnit, 
								kMidiMessage_ProgramChange << 4 | midiChannelInUse, 
								0, 										//prog change num
								0,
								0), 									//sample offset
								home);

	CAShow (graph); 										// prints out the graph so we can see what it looks like...

	if (debug) {
		LogPrintf(LOG_NOTICE, "2. set soundbank\n");
	}
		
	require_noerr (result = AUGraphStart (graph), home);
	
	if (debug) {
		LogPrintf(LOG_NOTICE, "3. AU graph started. result = %ld\n", result);
	}
	
	if (debug) {
		UInt32 instrCount = 0;
		UInt32 ioDataSize = 4;
											
		require_noerr (result = AudioUnitGetProperty(synthUnit,
											kMusicDeviceProperty_InstrumentCount,
											kAudioUnitScope_Global,
											0,
											&instrCount,
											&ioDataSize
											), home);
	}
	
	require_noerr (result = AUMIDIControllerMapChannelToAU(auMidiController, -1, synthUnit, -1, true), home);
	
home:
	return (result == noErr);
}


Boolean SetupSoftMIDI(const char *bankPath, Byte *activeChannel)
{
	OSStatus result;
	
	UInt8 midiChannelInUse = 0; 							// we're using MIDI channel 1...
	
		// bankPath is the full path of the sample bank...
		//		
		// On OS X there are known places were sample banks can be stored
		// Library/Audio/Sounds/Banks - so you could scan this directory and give the user options
		// about which sample bank to use...

	require_noerr (result = CreateAUGraph (graph, synthUnit), home);
	
	// if the user supplies a sound bank, we'll set that before we initialize and start playing
	
	if (bankPath) 
	{
		FSSpec soundBankSpec;
		require_noerr (result = PathToFSSpec (bankPath, soundBankSpec), home);
		
		printf ("Setting Sound Bank:%s\n", bankPath);
		
		require_noerr (result = AudioUnitSetProperty (synthUnit,
											kMusicDeviceProperty_SoundBankFSSpec,
											kAudioUnitScope_Global, 0,
											&soundBankSpec, sizeof(soundBankSpec)), home);
    
	}
	
	// ok we're set up to go - initialize and start the graph
	
	require_noerr (result = AUGraphInitialize (graph), home);

	for (int i = 0; i<MAXCHANNEL; i++)
		if (activeChannel[i])
			midiChannelInUse |= i;
			
		//set our bank
		
	require_noerr (result = MusicDeviceMIDIEvent(synthUnit, 
								kMidiMessage_ControlChange << 4 | midiChannelInUse, 
								kMidiMessage_BankMSBControl, 0,
								0), 										//sample offset
								home);

	require_noerr (result = MusicDeviceMIDIEvent(synthUnit, 
								kMidiMessage_ProgramChange << 4 | midiChannelInUse, 
								0, 										//prog change num
								0,
								0), 										//sample offset
								home);

	//CAShow (graph); // prints out the graph so we can see what it looks like...
	
	require_noerr (result = AUGraphStart (graph), home);
 
home:
	return (result == noErr);
}


void TeardownSoftMIDI(void)
{
	if (graph) {
		AUGraphStop (graph); // stop playback - AUGraphDispose will do that for us but just showing you what to do
		DisposeAUGraph (graph);
		
		graph = 0;
	}
}


OSStatus SoftMIDISendMIDIEvent(MIDITimeStamp tStamp, UInt16 pktLen, Byte *data)
{
	Byte b0,b1,b2=0;
	
	b0 = data[0];
	b1 = data[1];
	
	if (pktLen > 2)
		b2 = data[2];
		
	OSStatus result = MusicDeviceMIDIEvent(synthUnit, b0, b1, b2, 0);
	
	return result;
}	

