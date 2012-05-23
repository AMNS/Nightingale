/* MIDIMgrUtils.c - MIDI Manager utilities for Nightingale - based on Apple's MIDIArp
and SquidCakes demos. Donald Byrd, AMNS, Sept. 1991; revised for v.3.1. */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1991-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

//#if TARGET_API_MAC_CARBON_MACHO
#include <CoreMIDI/MIDIServices.h>		/* for MIDIPacket */
//#else
//#include <midi.h>						/* for MIDIPacket */
//#endif
#include "CarbonStubs.h"


/* MIDI Manager constants  */

#define myClientID		creatorType

#define timePortID		'Atim'
#define inputPortID		'Bin '
#define outputPortID		'Cout'

#define refCon0				0L
#define noTimeBaseRefNum	0
#define noClient				'    '
#define noReadHook			0L
#define noTimeProc			0L
#define zeroTime				0L
#define zeroPeriod			0L
#define timePortBufSize		0L
#define inputPortBufSize	4096L
#define outputPortBufSize	0L


Boolean		manualPatch;			/* TRUE if not launched by a PatchBay Config. file */

pascal short NightReader(MIDIPacketPtr packetPtr, long refCon);


/* ----------------------------------------------------------------- GiveResError -- */
/* Alert user to Resource Manager Error, if there is one, and kill the program. ??NB:
as of this writing, in several places, this is called after GetResource iff the
resource handle is NULL, so in those cases only it should give an error message and
quit regardless of ResError. (According to Inside Mac, I-119, GetResource can return
NULL without an error that ResError would report under fairly common circumstances.)
This should be cleaned up some day along the lines of ReportBadResource. */

void GiveResError(char *);
void GiveResError(char *)		/* Argument is ignored */
{
	if (ReportResError()) {
		GetIndCString(strBuf, MIDIERRS_STRS, 25);		/* "Fatal error in handling MIDI Manager resources." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		MIDISignOut(myClientID);	
		ExitToShell();
	}
}


/* ------------------------------------------------------------------ NightReader -- */

/* The Read Hook Function. Reads all incoming MIDI data. 

N.B. If FROM_INTRPT is defined, this function saves and restores A5, which is really
necessary only if we might be called at interrupt level. If we're called only at non-
interrupt time, e.g., in response to calls to MIDIPoll, our A5 world is guaranteed to
be intact and this isn't necessary. */

pascal short NightReader(MIDIPacketPtr packetPtr, long /*refCon*/)
{
	char				textMsg[256];
	short				length, i;		
	MIDIPacketPtr	thePacketPtr, p;
	
		
#ifdef FROM_INTRPT
	long				oldA5;
	oldA5 = OurA5();	/* save current A5 and reset A5 to our A5 world */
		/* Necessary only if we might get called at interupt time */
#endif
	
	thePacketPtr = (MIDIPacketPtr)packetPtr;
	length = thePacketPtr->len;	
	
	if (length>=MM_HDR_SIZE) {								/* if not, things are really screwed up! */
		
		/* If we're recording and we have enough room in our buffer to record more: */
		if (recordingNow && mRecIndex+length<mPacketBufferLen-1) {
		
			p = (MIDIPacketPtr) &(mPacketBuffer[mRecIndex]);
			p->flags = thePacketPtr->flags;
			p->len = thePacketPtr->len;
			p->tStamp = thePacketPtr->tStamp;
			
			for (i = 0; i<(length-MM_HDR_SIZE); i++) {
				p->data[i] = thePacketPtr->data[i];
			}
			
			if (mRecIndex == 0) {							/* save the first time stamp */
				mFirstTime = p->tStamp - 100L;			/* with a little extra time buffer in front */
				if (mFirstTime < 0L) mFirstTime = 0L;	/* WHY??? */
			}
			
			mFinalTime = p->tStamp;							/* save the latest time stamp */
			
			/* 
			 * Increment mRecIndex to next MIDIPacket space (plus one if it would be an
			 * odd numbered index, to keep us on word boundaries). NB: We can use
			 * ROUND_UP_EVEN here only if <mRecIndex> is guaranteed to be even! I suspect
			 * it is, but at the moment, I'm not sure.
			 */
			mRecIndex += length + ( ((mRecIndex + length) & 1) ? 1 : 0 );

#ifndef PUBLIC_VERSION
			if (CapsLockKeyDown() && OptionKeyDown()) {
				sprintf(textMsg,"NR:%x TS=%ld data=", thePacketPtr->flags, thePacketPtr->tStamp);	
				if (length>=MM_HDR_SIZE) {							/* if not, things are really screwed up! */
					for (i = 0; i<(length-MM_HDR_SIZE); i++)
						sprintf(&textMsg[strlen(textMsg)], "%x ",
									(unsigned char)thePacketPtr->data[i]);		
					sprintf(&textMsg[strlen(textMsg)], "\n");
				}
				else
					sprintf(textMsg, "ERROR: packet length=%d\n", length);
				
				DebugPrintf((char *)textMsg);
			}
#endif		
		}
	}
	
#ifdef FROM_INTRPT
	TheirA5(oldA5);	/* Restore A5 to its previous state */
#endif
	
	return(midiMorePacket);
}


/* ------------------------------------------------------------------ LoadPatches -- */
/* Get previously-saved connections (port info records) for all ports from our
'port' resource. */

static void LoadPatches(void);
static void LoadPatches()
{
	MIDIPortInfoHdl	portInfoH;	/* Handle to port info record. */
	MIDIPortInfoPtr	portInfoP;	/* Pointer to port info record. */
	short					i, theErr;
		
	/* SET UP TIME PORT CONNECTIONS. */
	
	portInfoH = (MIDIPortInfoHdl) Get1Resource(portResType, time_port);
	if (portInfoH==NULL)
		GiveResError("portResType, time_port");

	HLock((Handle) portInfoH);
	portInfoP = *portInfoH;
	if (GetHandleSize((Handle) portInfoH) != 0)
	{
			/* Were we supposed to be sync'd to another client? */
		if (portInfoP->timeBase.clientID != noClient)
		{		
				/* Yes, so make that client our time base. */
			theErr = MIDIConnectTime(portInfoP->timeBase.clientID, 
						portInfoP->timeBase.portID,
						myClientID, 
						timePortID);
				/* Is the client still signed in? */
			if (theErr != midiVConnectErr) 
			{	
					/* Yes, so set our sync mode to external. */
				MIDISetSync(timeMMRefNum, midiExternalSync);
			}
			
		}
			/* Were we somebody else's time base? */
		for (i=0; i<portInfoP->numConnects; i++)
		{
			MIDIConnectTime(myClientID, 
						timePortID, 
						portInfoP->cList[i].clientID, 
						portInfoP->cList[i].portID);
		}
	}
	HUnlock((Handle) portInfoH);
	ReleaseResource((Handle) portInfoH);
	GiveResError("LoadPatches");
	
#ifndef VIEWER_VERSION
	/* SET UP INPUT PORT CONNECTIONS. */
	
	portInfoH = (MIDIPortInfoHdl) Get1Resource(portResType, input_port);
	if (portInfoH==NULL)
		GiveResError("LoadPatches");

	HLock((Handle) portInfoH);
	portInfoP = *portInfoH;
	if (GetHandleSize((Handle) portInfoH) != 0)
	{
			/* Were we connected to anyone? */
		for (i=0; i<portInfoP->numConnects; i++)
		{
			MIDIConnectData(myClientID, 
						inputPortID, 
						portInfoP->cList[i].clientID, 
						portInfoP->cList[i].portID);
		}
	}
	HUnlock((Handle) portInfoH);
	ReleaseResource((Handle) portInfoH);
	GiveResError("LoadPatches");
#endif
	
	/* SET UP OUTPUT PORT CONNECTIONS. */
	
	portInfoH = (MIDIPortInfoHdl) Get1Resource(portResType, output_port);
	if (portInfoH==NULL)
		GiveResError("LoadPatches");

	HLock((Handle) portInfoH);
	portInfoP = *portInfoH;
	if (GetHandleSize((Handle) portInfoH) != 0)
	{
			/* Were we connected to anyone? */
		for (i=0; i<portInfoP->numConnects; i++)
		{
			MIDIConnectData(myClientID, 
						outputPortID, 
						portInfoP->cList[i].clientID, 
						portInfoP->cList[i].portID);
		}
	}
	HUnlock((Handle) portInfoH);
	ReleaseResource((Handle) portInfoH);
	GiveResError("LoadPatches");
}


/* -------------------------------------------------------------------- SavePatch -- */
/* Save current connections for the given port (a port info record) in a 'port'
resource. */

static void SavePatch(OSType, short, unsigned char *);
static void SavePatch(OSType portID, short portInfoResID, unsigned char *portInfoResName)
{
	Handle		portResH;	/* Handle to 'port' resource. */
	
	WaitCursor();

	/* Remove existing port info resource. */
	
	portResH = Get1Resource(portResType, portInfoResID);
	GiveResError("SavePatch");
	RemoveResource(portResH);
	GiveResError("SavePatch");
	ChangedResource(portResH);
	DisposeHandle(portResH);
	UpdateResFile(CurResFile());
	GiveResError("SavePatch");
	
	/*	Get new configuration. */
	
	portResH = (Handle) MIDIGetPortInfo(myClientID, portID);
	
	/*	Save new configuration. */
	
	AddResource(portResH, portResType, portInfoResID, portInfoResName);
	GiveResError("SavePatch");
	WriteResource(portResH);
	GiveResError("SavePatch");
	UpdateResFile(CurResFile());
	GiveResError("SavePatch");
	ReleaseResource(portResH);
	GiveResError("SavePatch");
	
	InitCursor();
}


/* ----------------------------------------------------------------------- MMInit -- */
/* Initialize the MIDI Manager. If the MIDI Manager is installed, sign into it and
set up time, input, and output ports. Do NOT start the time base clock. If we succeed,
return TRUE, else FALSE; in either case, return in <verNum> the version number in
standard Macintosh version number form. If the MIDI Manager is not installed, just
return FALSE with <verNum> = 0. */

Boolean MMInit(long *verNum)
{
	long				verNumMM;		/* MIDI Manager version (standard Mac Version number) */
	MIDIPortParams	init;				/* MIDI Manager Init data structure  */
	Handle			iconHndl;
	OSErr				theErr;
	Str255			str;
	short				curResFile;
	static MIDIReadHookUPP hookUPP;		/* We're only going to allocate this once */
	
	/* Make sure MIDI Manager is installed and get version number. */
	
	verNumMM = NMIDIVersion();
	if (verNumMM==0) {
		*verNum = 0;
		return FALSE;
	}
	
	*verNum = verNumMM;
	
	/* Sign in to the MIDI Manager. */
	
	iconHndl = GetResource('ICN#', NGALE_ICON);
	if (ReportBadResource(iconHndl)) return FALSE;

	strcpy((char *)str, PROGRAM_NAME);
	CToPString((char *)str);
	theErr = MIDISignIn(myClientID, refCon0, iconHndl, str);
	if (theErr) return FALSE;
	
	/* Assume not a PatchBay configuration. */
	
	manualPatch = TRUE;	

	/* Add time port. */
	
	init.portID = timePortID;
	init.portType = midiPortTypeTime;
	init.timeBase = noTimeBaseRefNum;
	init.readHook = noReadHook;						/* NULL, so doesn't need a routine descriptor */
	init.initClock.syncType = midiInternalSync;
	init.initClock.curTime = zeroTime;
	init.initClock.format = midiFormatMSec;
	init.refCon = SetCurrentA5();
	Pstrcpy(init.name, "\pTimeBase");
	theErr = MIDIAddPort(myClientID, timePortBufSize, &timeMMRefNum, &init);
		
	if (theErr==midiVConnectMade) 
		manualPatch = FALSE;							/* A PatchBay connection has been resolved */
	else if (theErr==memFullErr) {
		GetIndCString(strBuf, MIDIERRS_STRS, 28);		/* "Not enough memory to add time port." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		MIDISignOut(myClientID);	
		return FALSE;
	}
	
#ifndef VIEWER_VERSION
	/* Add an input port. */
	
	init.portID = inputPortID;
	init.portType = midiPortTypeInput;
	init.timeBase = timeMMRefNum;
	init.offsetTime = midiGetNothing;			/* so readHook is never called at interrupt level */
	if (hookUPP==NULL) {
		hookUPP = NewMIDIReadHookUPP(NightReader);
		if (hookUPP==NULL) {
			GetIndCString(strBuf, MIDIERRS_STRS, 29);		/* "Not enough memory to add input port." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			MIDISignOut(myClientID);	
			return FALSE;
			}
		}
	init.readHook = hookUPP;
	init.refCon = SetCurrentA5();
	Pstrcpy(init.name, "\pInputPort");
	theErr = MIDIAddPort(myClientID, inputPortBufSize, &inputMMRefNum, &init);
	
	if (theErr==midiVConnectMade) 
		manualPatch = FALSE;							/* A PatchBay connection has been resolved */
	else if (theErr==memFullErr) {
		GetIndCString(strBuf, MIDIERRS_STRS, 29);		/* "Not enough memory to add input port." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		MIDISignOut(myClientID);	
		return FALSE;
	}
#endif
	
	/* Add an output port. */
	
	init.portID = outputPortID;
	init.portType = midiPortTypeOutput;
	init.timeBase = timeMMRefNum;
	init.offsetTime = midiGetCurrent;
	init.readHook = noReadHook;						/* NULL, so doesn't need a routine descriptor */
	init.refCon = 0L;
	Pstrcpy(init.name, "\pOutputPort");
	theErr = MIDIAddPort(myClientID, outputPortBufSize, &outputMMRefNum, &init);
	
	if (theErr==midiVConnectMade) 
		manualPatch = FALSE;							/* A PatchBay connection has been resolved */
	else if (theErr==memFullErr) {
		GetIndCString(strBuf, MIDIERRS_STRS, 30);		/* "Not enough memory to add output port." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		MIDISignOut(myClientID);	
		return FALSE;
	}
	
	/* If not a PatchBay patch, connect ports as they were when we last quit. */
	
	if (manualPatch) {
		/* 
		 * Get the resources from the Nightingale Setup file, and then restore the
		 * previous current resource file.
		 */
		curResFile = CurResFile();
		UseResFile(setupFileRefNum);
		LoadPatches();
		UseResFile(curResFile);
	}
	
	recordingNow = FALSE;

	return TRUE;
}


/* ------------------------------------------------------------------- MMClose -- */
/* Save our patch (port connection) configuration, if necessary, and sign out from
the MIDI Manager. */

Boolean MMClose()
{
	MIDIPortInfoHdl portInfoH;
	Handle portResH;					/* Handle to 'port' resource. */
	Boolean patchChanged, okay=TRUE;
	size_t resLen;
	short curResFile;
	
	if (manualPatch) {
		/*
		 * Get the 'port' resources from file and compare them to our internal values.
		 * If any has been changed, update the file.
		 */
		portInfoH = (MIDIPortInfoHdl) Get1Resource(portResType, time_port);
		if (ReportBadResource((Handle)portInfoH)) { okay = FALSE; goto SignOut; }
		resLen = GetResourceSizeOnDisk((Handle)portInfoH);
		portResH = (Handle) MIDIGetPortInfo(myClientID, timePortID);
		patchChanged = memcmp( (void *)*portInfoH, (void *)*portResH, resLen);
		if (patchChanged) goto Save;
		
#ifndef VIEWER_VERSION
		portInfoH = (MIDIPortInfoHdl) Get1Resource(portResType, input_port);
		if (ReportBadResource((Handle)portInfoH)) { okay = FALSE; goto SignOut; }
		resLen = GetResourceSizeOnDisk((Handle)portInfoH);
		portResH = (Handle) MIDIGetPortInfo(myClientID, inputPortID);
		patchChanged = memcmp( (void *)*portInfoH, (void *)*portResH, resLen);
		if (patchChanged) goto Save;
#endif
	
		portInfoH = (MIDIPortInfoHdl) Get1Resource(portResType, output_port);
		if (ReportBadResource((Handle)portInfoH)) { okay = FALSE; goto SignOut; }
		resLen = GetResourceSizeOnDisk((Handle)portInfoH);
		portResH = (Handle) MIDIGetPortInfo(myClientID, outputPortID);
		patchChanged = memcmp( (void *)*portInfoH, (void *)*portResH, resLen);
		if (!patchChanged) goto SignOut;
			
Save:
 		if (CautionAdvise(SAVEMM_ALRT)==OK) {
			/* 
			 * Save the resources in the Nightingale Setup file, and then restore the
			 * previous current resource file.
			 */
			curResFile = CurResFile();
			UseResFile(setupFileRefNum);
			if (ReportResError()) { okay = FALSE; goto SignOut; }
			SavePatch(timePortID, time_port, "\ptimePortInfo");
#ifndef VIEWER_VERSION
			SavePatch(inputPortID, input_port, "\pinputPortInfo");
#endif
			SavePatch(outputPortID, output_port, "\poutputPortInfo");
			UseResFile(curResFile);
		}
	}

SignOut:
	MIDISignOut(myClientID);
	return okay;
}
