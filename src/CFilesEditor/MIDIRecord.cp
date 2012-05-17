/* MIDIRecord.c for Nightingale - from MIDI input to simple Syncs.
By Donald Byrd, rev.for v. 3.5. FreeMIDI support added by John Gibson for v4.1.
		AllocRawBuffer			SetRecordFlats				RecordMessage
		RawMIDI2MMNote			RecordDlogMsg				OMSPrepareRecord
		RecordDialog			SoundClickNow
		FillMNote				RawMIDI2Packets			MIDI2Night
		RecordBuffer			Record						NoteOnDemon
		Clocks2Dur				StepMIDI2Night				ShowNRCInBox
		MRSetNRCDur				IsOurNoteOn					MPMIDIPoll
		GetAllNoteOns			SRKeyDown					StepRecord
		RTMRecord				PrintNote					PrintBuffer
*/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-2001 by Adept Music Notation Solutions, Inc. All Rights Reserved.
 */
 
/* Nightingale supports MOTU's FreeMIDI, Opcode's OMS, and Apple's MIDI Manager. It
used to support its own "built-in" MIDI routines -- Altech Systems' MIDI Pascal 3.0, 
used here and in MIDIGeneral.c -- but we don't have the right library for CodeWarrior,
so "built-in" MIDI is now disabled. All four run at interrupt time and transfer data
via buffers. Thus, we don't have to worry much about losing data by not paying enough
attention to real-time activity.

MIDI Pascal returns single bytes of MIDI data plus timestamp. MIDI Manager and OMS
do a lot more for the user and returns complete "MIDI Packets", each containing a
complete timestamped MIDI command (except for SysEx messages, which may occupy
several packets).

NB: we used to use the MacTutor MIDI driver instead of MIDI Pascal; unfortunately,
some of the code still reflects that. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

//#include <MIDI.h>
#include "MIDICompat.h"
#include "CarbonStubs.h"
#include "CoreMIDIDefs.h"

extern long		gmFMSBufferLength;	/* FreeMIDI MIDI Buffer size in bytes */
extern Boolean	gFMSMIDIBufferFull;
extern long		gmOMSBufferLength;	/* OMS MIDI Buffer size in bytes */
extern Boolean	gOMSMIDIBufferFull;
/*
extern long		gCMBufferLength;		// CoreMidi MIDI Buffer size in bytes //
extern Boolean	gCMMIDIBufferFull;
extern MIDIUniqueID gSelectedInputDevice;
*/

static Boolean			srFirstNote;
static MIDITimeStamp	srFirstTimeStamp;

long		rawBufferLength;			/* In 4-byte words (longs) */
long		*rawBuffer;					/* Input buffer: word's 1st 3 bytes=time in ms., last byte=MIDI */
INT16		transpose=0,				/* Amount to subtract from MIDI note nos. */
			splitPoint=MIDI_MIDDLE_C,	/* MIDI note no. to split notes between staves */
			otherStaff;					/* Staff no. for split-off notes */
Boolean	printDebug=TRUE,			/* Show debug info in Current Events window? */
			split;						/* Split between selStaff & otherStaff (else evrythng on selStaff)? */
			
#define LAST_REC_MARGIN (4*PDURUNIT)	/* (Unused) Assumed silent time after last rec. note */

/* If recording without an insertion pt is allowed, the ref's to a doc's selStaff
	in USESTAFF must be changed--e.g., to GetSelStaff(), but that may be slow. */
#define USESTAFF(doc, noteNum) 	((split && ((noteNum)<splitPoint==otherStaff>(doc)->selStaff))?	\
												otherStaff : (doc)->selStaff )

typedef struct NotePacket {
	long	tStamp;
	Byte	flags;
	Byte	data[3];
} NotePacket;

static Boolean AllocRawBuffer(void);
static void SetRecordFlats(Document *);
static void RecordMessage(Document *, Boolean);

static void RawMIDI2MMNote(long, Boolean, INT16, INT16 *, MMMIDIPacket *);
static Boolean RecordDialog(Document *, INT16, INT16, INT16, Boolean, Boolean, INT16 *,
							Boolean *, long *);
static void SoundClickNow(Boolean, Boolean);

static Boolean FillMNote(long, long, Byte, MNOTEPTR);
static long RawMIDI2Packets(long);
static void MIDIDataToLong(INT16 midiWord, long timeStamp, long *pmdata);
static void OutOfRawRecMemory(void);
static void OutOfNoteOnRecMemory(void);
static long MIDI2Night(Document *, INT16, long, long);

static void RecordBuffer(Document *, Boolean, Boolean, INT16, Rect, long, long *);

static Boolean NoteOnDemon(INT16, Byte);
static Boolean Clocks2Dur(INT16, INT16, INT16 *, INT16 *);
static LINK StepMIDI2Night(Document *, NotePacket [], INT16, INT16, INT16, INT16);
static void ShowNRCInBox(Document *, LINK, INT16, INT16, Rect *, INT16);

static void PrintNote(MNOTEPTR, Boolean);
static void PrintNBuffer(Document *, NotePacket [], INT16);
static void PrintBuffer(Document *, long);

#ifndef CM_DEBUG
#define CM_DEBUG 1
#endif


/* ========================================== GENERAL AND USER-INTERFACE ROUTINES == */

/* -------------------------------------------------------------- AllocRawBuffer -- */
/* Allocate a buffer for recording into with the built-in MIDI driver, pointed to by
<rawBuffer>. If we have trouble doing so, return FALSE, else TRUE.

A single note takes from 4 to 6 longwords = 16 to 24 bytes in the input buffer
(for both Note On and Note Off), while Nightingale's object list data structure
as of v. 3.0 takes 30 bytes per note plus a great deal of overhead for the Sync, etc.;
thus, the total memory needed to completely handle a note is over three times the
space it occupies in the input buffer. Therefore, considering only notes, the
largest useful buffer size is less than 1/3 of available memory. But Nightingale's
other limitations are such that recording large amounts of data at once isn't
really useful anyway. */

static Boolean AllocRawBuffer()
{
	Size bytesFree, grow;

	/* Don't bother for FreeMIDI and OMS MIDI. */
	if (useWhichMIDI==MIDIDR_FMS || useWhichMIDI==MIDIDR_OMS || useWhichMIDI==MIDIDR_CM) {
		rawBuffer = NULL;
		return TRUE;
	}

	bytesFree = MaxMem(&grow);
	rawBufferLength = bytesFree/4L;							/* Take a fraction of available memory */
	rawBufferLength = n_min(rawBufferLength, 160000L);	/* Limit its size to make NewPtr faster */

	if (rawBufferLength<2000L) return FALSE;
	
	rawBufferLength /= 4L;										/* Convert bytes to longwords */
	rawBuffer = (long *)(NewPtr(rawBufferLength*4L));
	return (GoodNewPtr((Ptr)rawBuffer));
}


/* -------------------------------------------------------------- SetRecordFlats -- */
/* Look for a key signature before the insertion point on its staff and set
<recordFlags> accordingly. If there is no such key signature, use the default. */

static void SetRecordFlats(Document *doc)
{
	LINK keySigL;
	register LINK aKeySigL;
	PAKEYSIG aKeySig;
	
	recordFlats = doc->recordFlats;

	keySigL = LSSearch(doc->selStartL, KEYSIGtype, doc->selStaff, GO_LEFT, FALSE);
	if (keySigL) {
		aKeySigL = KeySigOnStaff(keySigL, doc->selStaff);
		if (aKeySigL) {
			aKeySig = GetPAKEYSIG(aKeySigL);
			if (aKeySig->nKSItems>0) recordFlats = !(aKeySig->KSItem[0].sharp);
		}
	}
}


/* --------------------------------------------------------------- RecordMessage -- */
/* Write a message into the message area saying how to stop recording. */

static void RecordMessage(Document *doc, Boolean paused)
{
	if (paused) GetIndCString(strBuf, MIDIREC_STRS, 1);	/* "Recording -PAUSED-   CLICK TO STOP" */
	else		   GetIndCString(strBuf, MIDIREC_STRS, 2);	/* "Recording    CLICK TO STOP" */
	DrawMessageString(doc, strBuf);
}


/* -------------------------------------------------------------- QuickFlashRect -- */

static void QuickFlashRect(Rect *);
static void QuickFlashRect(Rect *r)
{
	InvertRect(r);
	SleepTicks(HILITE_TICKS/3);			/* A short delay is all we need */
	InvertRect(r);
}


/* --------------------------------------------------------------- RawMIDI2MMNote -- */
/* If called when rawBuffer[rLast] is the LAST byte of a Note On or Note Off,
translates the Note On/Off into the corresponding MIDI Manager MMMIDIPacket. Handles
Running Status. */

static void RawMIDI2MMNote(
				long rLast,
				Boolean isOn,			/* TRUE=Note On, FALSE=Note Off */
				INT16 channel,			/* 0 to 15 */
				INT16 *len,				/* Output */
				MMMIDIPacket *mPacket	/* Output */
				)
{
	mPacket->flags = MM_STD_FLAGS;
	mPacket->len = MM_HDR_SIZE+3;

	mPacket->tStamp = rawBuffer[rLast-1L] >> 8;			/* in milliseconds */
	if (isOn) mPacket->data[0] = MNOTEON | channel;
	else		 mPacket->data[0] = MNOTEOFF | channel;
	mPacket->data[1] = rawBuffer[rLast-1L];
	mPacket->data[2] = rawBuffer[rLast];
	*len = mPacket->len;
}


/* ================================================= REAL-TIME RECORDING ROUTINES == */

/* --------------------------------------------------------------- RecordDlogMsg -- */

#define FLASH_DI 2

/* If which!=0, put up a small window (actually a dialog) containing the message
that recording is in progress, and (CAVEAT!) leave thePort set to that window so the
caller can write into it. If which=0, remove the window and restore thePort to what
it was as of the previous call with which!=0. */

static Boolean RecordDlogMsg(INT16, Rect *);

static Boolean RecordDlogMsg(INT16 which, Rect *flashRect)
{
	static DialogPtr dlog=NULL; static GrafPtr oldPort;
	INT16 anInt; Handle aHdl;
	 
	if (which>0) {
		if (!dlog) {
			dlog = GetNewDialog(RECORDING_DLOG, NULL, BRING_TO_FRONT);
			if (!dlog) return FALSE;
		}

		GetPort(&oldPort);
		SetPort(GetDialogWindowPort(dlog));

		CenterWindow(GetDialogWindow(dlog),112);			/* So top lines of Record dialog are still visible */
		ShowWindow(GetDialogWindow(dlog));
		UpdateDialogVisRgn(dlog);
		GetDialogItem(dlog, FLASH_DI, &anInt, &aHdl, flashRect);
	}

	else if (which==0) {
		if (dlog) {
			HideWindow(GetDialogWindow(dlog));
			DisposeDialog(dlog);
			dlog = NULL;
			SetPort(oldPort);
		}
	}

	return TRUE;
}


/* --------------------------------------------------------------- FMSPrepareRecord -- */
/* Do some initialization of FreeMIDI, and return information needed for recording.
Return FALSE if there's a problem, TRUE if all is well. */

static Boolean FMSPrepareRecord(Document *doc, LINK partL, INT16 *pUseChan, fmsUniqueID *pThruDevice, fmsUniqueID *pInputDevice, INT16 *pThruChannel);

static Boolean FMSPrepareRecord(
						Document *doc,
						LINK partL,
						INT16 *pUseChan,						/* input AND output */
						fmsUniqueID *pThruDevice,
						fmsUniqueID *pInputDevice,
						INT16 *pThruChannel)
{
	char	      errStr[256];
	PPARTINFO	pPart;

	*pInputDevice = doc->fmsInputDevice;
	if (!FMSChannelValid(*pInputDevice, *pUseChan)) {
		*pInputDevice = config.defaultInputDevice;
		*pUseChan = config.defaultChannel;
		if (!FMSChannelValid(*pInputDevice, *pUseChan)) {
			GetIndCString(errStr, MIDIERRS_STRS, 33);		/* "FreeMIDI Input Device not Connected." */
			CParamText(errStr, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
	}
	FMSSetSelectedInputDevice(*pInputDevice, *pUseChan);

	if (doc->polyTimbral) {
		*pThruDevice = GetFMSDeviceForPartL(doc, partL);
		pPart = GetPPARTINFO(partL);
		*pThruChannel = pPart->channel;
		if (!FMSChannelValid(*pThruDevice, *pThruChannel)) {
			*pThruDevice = config.defaultOutputDevice;
			*pThruChannel = config.defaultOutputChannel;
		}
	}
	else {
		*pThruDevice = *pInputDevice;
		*pThruChannel = *pUseChan;
	}
	FMSSetMIDIThruDevice(*pThruDevice, *pThruChannel);

	return TRUE;
}


/* --------------------------------------------------------------- OMSPrepareRecord -- */
/* Do some initialization of OMS, and return information needed for recording. Return
FALSE if there's a problem, TRUE if all is well. */

static Boolean OMSPrepareRecord(Document *doc, LINK partL, INT16 *pUseChan, OMSUniqueID *pThruDevice, OMSUniqueID *pInputDevice, INT16 *pThruChannel);

static Boolean OMSPrepareRecord(
						Document *doc,
						LINK partL,
						INT16 *pUseChan,						/* input AND output */
						OMSUniqueID *pThruDevice,
						OMSUniqueID *pInputDevice,
						INT16 *pThruChannel)
{
	OMSErr		errOMS;
	char	      errStr[256], fmtStr[256];
	PPARTINFO	pPart;

	*pInputDevice = doc->omsInputDevice;
	if (!OMSChannelValid(*pInputDevice, *pUseChan)) {
		*pInputDevice = config.defaultInputDevice;
		*pUseChan = config.defaultChannel;
		if (!OMSChannelValid(*pInputDevice, *pUseChan)) {
			GetIndCString(errStr, MIDIERRS_STRS, 32);		/* "OMS Input Device not Connected." */
			CParamText(errStr, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
	}
	OMSSetSelectedInputDevice(*pInputDevice, *pUseChan);

	if (doc->polyTimbral) {
		*pThruDevice = GetOMSDeviceForPartL(doc, partL);
		pPart = GetPPARTINFO(partL);
		*pThruChannel = pPart->channel;
		if (!OMSChannelValid(*pThruDevice, *pThruChannel)) {
			*pThruDevice = config.defaultOutputDevice;
			*pThruChannel = config.defaultOutputChannel;
		}
	}
	else {
		*pThruDevice = *pInputDevice;
		*pThruChannel = *pUseChan;
	}
	OMSSetMidiThruDevice(*pThruDevice, *pThruChannel);
	
	errOMS = OpenOMSInput(*pInputDevice);
	if (errOMS) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 27);		/* "OMS Input Device not Connected..." */
		sprintf(errStr, fmtStr, errOMS);
		CParamText(errStr, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	
	return TRUE;
}

/* --------------------------------------------------------------- CMPrepareRecord -- */
/* Do some initialization of CoreMIDI, and return information needed for recording. Return
FALSE if there's a problem, TRUE if all is well. */

static Boolean CMPrepareRecord(Document *doc, LINK partL, INT16 *pUseChan, MIDIUniqueID *pThruDevice, MIDIUniqueID *pInputDevice, INT16 *pThruChannel);

static Boolean CMPrepareRecord(
						Document *doc,
						LINK partL,
						INT16 *pUseChan,						/* input AND output */
						MIDIUniqueID *pThruDevice,
						MIDIUniqueID *pInputDevice,
						INT16 *pThruChannel)
{
	OSStatus		errCM;
	char	      errStr[256], fmtStr[256];
	PPARTINFO	pPart;

	*pInputDevice = doc->cmInputDevice;
	if (!CMRecvChannelValid(*pInputDevice, *pUseChan)) {
		*pInputDevice = config.defaultInputDevice;
		*pUseChan = config.defaultChannel;
		if (!CMRecvChannelValid(*pInputDevice, *pUseChan)) {
#if CM_DEBUG
			DebugPrintf("doc inputDevice=%ld cfg defltInputDev=%ld defltChannel=%ld\n", doc->cmInputDevice,  config.defaultInputDevice, config.defaultChannel);
#endif			
			GetIndCString(errStr, MIDIERRS_STRS, 35);		/* "CoreMIDI Input Device not Connected." */
			CParamText(errStr, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
	}
	CoreMidiSetSelectedInputDevice(*pInputDevice, *pUseChan);

	if (doc->polyTimbral) {
		*pThruDevice = GetCMDeviceForPartL(doc, partL);
		pPart = GetPPARTINFO(partL);
		*pThruChannel = pPart->channel;
		if (!CMTransmitChannelValid(*pThruDevice, *pThruChannel)) {
			*pThruDevice = config.defaultOutputDevice;
			*pThruChannel = config.defaultOutputChannel;
		}
	}
	else {
		*pThruDevice = *pInputDevice;
		*pThruChannel = *pUseChan;
	}
	CoreMidiSetSelectedMidiThruDevice(*pThruDevice, *pThruChannel);
	
	errCM = OpenCoreMidiInput(*pInputDevice);
	if (errCM) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 34);		/* "CoreMIDI Input Device not Connected..." */
		sprintf(errStr, fmtStr, errCM);
		CParamText(errStr, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	
	return TRUE;
}

/* ---------------------------------------------------------------- RecordDialog -- */
/* Put up the real-time record modal dialog, and let user record via MIDI. If
<playAlong>, play back the existing score while recording (intended for Record
Merge). */

static enum				/* Dialog item numbers */
{
	NOTRANSP_DI=5,
	INSTTRANSP_DI,
	SEMITONES_DI,
	METRONOME_DI=9,
	DEBUG_DI,
	SPLIT_DI,
	SPLITPOINT_DI,
	RECORD_DI,
	RERECORD_DI
} E_RecordDialog;

Boolean RecordDialog(
				Document *doc,
				INT16 staffn, INT16 partn, INT16 useChan,
				Boolean noSplit,
				Boolean	playAlong,	/* TRUE=play back existing score while recording */
				INT16 *pOtherStaff,
				Boolean *pMetronome,
				long *ptLeadInOffset	/* Time occupied by playback lead-in (PDUR ticks) */ 
				)
{
	DialogPtr	dlog;
	GrafPtr		oldPort;
	INT16			ditem, anInt, dialogOver, newval, partTranspose, tempo;
	ControlHandle	debugHdl, instTranspHdl, noTranspHdl,
					splitHdl, recordHdl, reRecordHdl;
	char			param1[256], param2[256], param3[256];
	Rect			aRect, flashRect;
	LINK			partL, tempoL;
	PPARTINFO	pPart;
	PTEMPO		pTempo;
	Boolean		metro;
	long			timeScale;
	OMSUniqueID thruDevice, inputDevice;
	MIDIUniqueID cmThruDevice, cmInputDevice;
	INT16			thruChannel;
	ModalFilterUPP	filterUPP;
			
	ArrowCursor();

/* --- 1. Create the dialog and initialize its contents. --- */

	/* Locking a heap is very unusual for a dialog handler! This is a very unusual
		dialog, since the command's functionality takes place within it, but I don't
		have any idea if this is necessary any more. But it shouldn't hurt. */
		
	PushLock(OBJheap);

	GetPort(&oldPort);

	tempo = config.defaultTempo;
	timeScale = tempo*DFLT_BEATDUR;					/* Default in case no tempo marks found */
	tempoL = LSSearch(doc->selStartL, TEMPOtype, ANYONE, GO_LEFT, FALSE);
	if (tempoL) {
		pTempo = GetPTEMPO(tempoL);
		tempo = pTempo->tempo;
		if (tempoL) timeScale = Tempo2TimeScale(tempoL);
	}
	sprintf(param3, "%d", tempo);

	partL = FindPartInfo(doc, partn);
	pPart = GetPPARTINFO(partL);

	if (useWhichMIDI == MIDIDR_OMS) {
		if (!OMSPrepareRecord(doc, partL, &useChan, &thruDevice, &inputDevice, &thruChannel))
			return FALSE;
		sprintf(param2, "%d", gmOMSBufferLength/1024L);
	}
	else if (useWhichMIDI == MIDIDR_CM) {
		if (doc->cmInputDevice == kInvalidMIDIUniqueID)
			doc->cmInputDevice = gSelectedInputDevice;
		if (!CMPrepareRecord(doc, partL, &useChan, &cmThruDevice, &cmInputDevice, &thruChannel)) {
#if CM_DEBUG
			DebugPrintf("doc inputDevice=%ld cfg defltInputDev=%ld defltChannel=%ld\n", doc->cmInputDevice,  config.defaultInputDevice, config.defaultChannel);
#endif			
			return FALSE;
		}
		sprintf(param2, "%d", kCMBufLen/0x0400); //kCMBufLen/1024L);
	}
	else if (useWhichMIDI == MIDIDR_FMS) {
		/* NB: we aren't using inputDevice and thruChannel */
		if (!FMSPrepareRecord(doc, partL, &useChan, &thruDevice, &inputDevice, &thruChannel))
			return FALSE;
		sprintf(param2, "%d", gmFMSBufferLength/1024L);
	}
	else
		sprintf(param2, "%d", rawBufferLength/1024L);

	sprintf(param1, "%d", useChan);

	split = (!noSplit && pPart->firstStaff!=pPart->lastStaff);
	if (split)
		*pOtherStaff = (staffn==pPart->firstStaff? staffn+1 : staffn-1);
	partTranspose = pPart->transpose;
	strcpy(strBuf, pPart->name);
	CParamText(strBuf, param1, param2, param3);
	
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(RECORD_DLOG);
		PopLock(OBJheap);
		return FALSE;
	}
	dlog = GetNewDialog(RECORD_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(RECORD_DLOG);
		PopLock(OBJheap);
		return FALSE;
	}
	SetPort(GetDialogWindowPort(dlog));
	
	GetDialogItem(dlog, NOTRANSP_DI, &anInt, (Handle *)&noTranspHdl, &aRect);
	GetDialogItem(dlog, INSTTRANSP_DI, &anInt, (Handle *)&instTranspHdl, &aRect);
	if (partTranspose==0)
		HiliteControl(instTranspHdl, CTL_INACTIVE);
	if (doc->transposed && partTranspose!=0) {
		SetControlValue(instTranspHdl, 1);
		SetControlValue(noTranspHdl, 0);
	}
	else {
		SetControlValue(instTranspHdl, 0);
		SetControlValue(noTranspHdl, 1);
	}
	PutDlgWord(dlog, SEMITONES_DI, partTranspose, FALSE);
	PutDlgChkRadio(dlog, METRONOME_DI, *pMetronome);
	
	GetDialogItem(dlog, DEBUG_DI, &anInt, (Handle *)&debugHdl, &aRect);
#ifdef PUBLIC_VERSION
	SetControlValue(debugHdl, 0);
	HideDialogItem(dlog, DEBUG_DI);
#else
	SetControlValue(debugHdl, (printDebug? 1 : 0));
#endif
	GetDialogItem(dlog, SPLIT_DI, &anInt, (Handle *)&splitHdl, &aRect);
	if (split) {
		HiliteControl(splitHdl, CTL_ACTIVE);
		SetControlValue(splitHdl, 1);
		PutDlgWord(dlog, SPLITPOINT_DI, splitPoint, TRUE);
	}
	else {
		//HiliteControl(splitHdl, CTL_INACTIVE);
		HideDialogItem(dlog, SPLIT_DI);
		HideDialogItem(dlog, SPLITPOINT_DI);
	}

	GetDialogItem(dlog, RECORD_DI, &anInt, (Handle *)&recordHdl, &aRect);
	GetDialogItem(dlog, RERECORD_DI, &anInt, (Handle *)&reRecordHdl, &aRect);
	HiliteControl(reRecordHdl, CTL_INACTIVE);

	CenterWindow(GetDialogWindow(dlog), 70);
	ShowWindow(GetDialogWindow(dlog));
	OutlineOKButton(dlog,TRUE);
	
/*--- 2. Interact with user til they push OK or Cancel. --- */

	mRecIndex = 0L;												/* In case user OK's w/o recording */

	dialogOver = 0;
	while (dialogOver==0)
	{
		do
		{
			ModalDialog(filterUPP, &ditem);				/* Handle dialog events */
			switch (ditem) {
				case OK:
				case Cancel:
					dialogOver = ditem;
					break;
				case NOTRANSP_DI:
					SetControlValue(instTranspHdl, FALSE);
					SetControlValue(noTranspHdl, TRUE);
					break;
				case INSTTRANSP_DI:
					SetControlValue(instTranspHdl, TRUE);
					SetControlValue(noTranspHdl, FALSE);
					break;
				case METRONOME_DI:
					PutDlgChkRadio(dlog, METRONOME_DI,
										!GetDlgChkRadio(dlog,METRONOME_DI));
			  		break;
				case DEBUG_DI:
	  				SetControlValue(debugHdl, 1-GetControlValue(debugHdl));		/* Toggle "Debug" */
			  		break;
			  	case SPLIT_DI:
	  				SetControlValue(splitHdl, 1-GetControlValue(splitHdl));		/* Toggle "Split" */
			  		break;
			  	case RECORD_DI:
			  	case RERECORD_DI:
					metro = GetDlgChkRadio(dlog, METRONOME_DI);
			  		RecordDlogMsg(1, &flashRect);
			  		RecordBuffer(doc, playAlong, metro, tempo, flashRect, timeScale,
			  							ptLeadInOffset);
			  		RecordDlogMsg(0, &flashRect);
					HiliteControl(recordHdl, CTL_INACTIVE);
					HiliteControl(reRecordHdl, CTL_ACTIVE);					/* Always "re-record" now */
			  		break;
			 	default:
			  		;
			}
		} while (!dialogOver);
	
	/* --- 3. If dialog was terminated with OK, check any new values. --- */
	/* ---    If any are illegal, keep dialog on the screen to try again. - */

		if (useWhichMIDI == MIDIDR_OMS)
			CloseOMSInput(inputDevice);
		else if (useWhichMIDI == MIDIDR_CM)
			CloseCoreMidiInput();
			
		if (dialogOver==Cancel)
		{
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(dlog);										/* Free heap space */
			PopLock(OBJheap);
			SetPort(oldPort);
			return FALSE;
		}
		else
		{
			if (split) {
				GetDlgWord(dlog,SPLITPOINT_DI,&newval);
				if (newval<1 || newval>127)								/* 127=MAX_NOTENUM */
				{
					GetIndCString(strBuf, MIDIERRS_STRS, 1);			/* "Split point must be..." */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					dialogOver = 0;										/* Keep dialog on screen */
				}
				else
					splitPoint = newval;
			}
		}
	}
	
/* --- 4. Dialog was terminated with OK and all values are legal. --- */

	printDebug = (GetControlValue(debugHdl)!=0);						/* Get new values */
	if (GetControlValue(instTranspHdl)!=0)
		transpose = partTranspose;
	else
		transpose = 0;
	split = (GetControlValue(splitHdl)!=0);
	*pMetronome = GetDlgChkRadio(dlog, METRONOME_DI);
	FMSSetMIDIThruDeviceFromConfig();
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);												/* Free heap space */
	PopLock(OBJheap);
	SetPort(oldPort);
	return TRUE; 
}


Boolean BIMIDINoteAtTime(short noteNum, short channel, short velocity, long time);

/* ---------------------------------------------------------------- SoundClickNow -- */
/* Simple metronome click, either Macintosh internal sound or as a MIDI "note".

We can make a nice click on the Mac by using the Sound Manager via PlayResource and
a sampled sound like "AhChoo.45-.48" or "AhChoo.47-.48" (yes, a very short section
of someone sneezing). (The old Apple "snippet" ClickSound.p makes a lousy metronome
sound, though MPU-401 users might like it.)

If we're using internal sound, we "know" the sound is short, and we play it
synchronously, not returning till it's done. If we're using MIDI, we play the sound
asynchronously, i.e., we start it and return immediately.

Timing the clicks accurately is not easy, especially with internal sounds. The Sound
Manager uses VIA Timer T1: bad for anyone else who's relying on T1, for example a
MIDI driver's timing routines! SysBeep doesn't help much: on most, perhaps all,
recent Macs, with most, perhaps all, sounds, SysBeep actually uses the Sound Manager.
With MIDI Manager's MIDIGetCurTime, there's a different but equally horrendous
problem: even with no other MIDI applications running and no INITs other than MIDI
Manager itself, the "clicks" are extremely uneven, apparently because it's skipping
"beats", often two or three in a row. And TickCount is only fair. What works? The
Time Manager is probably the only way to get truly reliable timing, though it looks
like the extended version is required for really good results. Some day. */

#define CLICK_snd 128

static void SoundClickNow(
					Boolean clickViaMIDI,	/* Click over MIDI instead of internal iff using MIDI Mgr */
					Boolean /*accented*/			/* ignored: see comment below */
					)
{
	Handle sndH; long endTime;
	short ioRefNum;
	
	if (clickViaMIDI) {
		endTime = GetMIDITime(0L)+config.metroDur;
		if (useWhichMIDI==MIDIDR_MM) {
			MMStartNoteNow(config.metroNote, config.metroChannel, config.metroVelo);
			MMEndNoteAtTime(config.metroNote, config.metroChannel, endTime);
		}
		else if (useWhichMIDI==MIDIDR_OMS) {
			ioRefNum = OMSUniqueIDToRefNum(config.metroDevice);
			OMSStartNoteNow(config.metroNote, config.metroChannel, config.metroVelo, ioRefNum);

			/*
			 * ??The following use of SleepMS is not great--the implementation as of v.3.0A
			 * is terrible and may not give accurate enough timing for this use, especially
			 * on a very fast Mac.
			 */
			SleepMS((long)config.metroDur);
			OMSEndNoteNow(config.metroNote, config.metroChannel, ioRefNum);
		}
		else if (useWhichMIDI==MIDIDR_CM) {
		
			MIDIUniqueID gDestID = GetMIDIObjectId(gDest);
		
			CMStartNoteNow(gDestID, config.metroNote, config.metroChannel, config.metroVelo);

			/*
			 * ??The following use of SleepMS is not great--the implementation as of v.3.0A
			 * is terrible and may not give accurate enough timing for this use, especially
			 * on a very fast Mac.
			 */
			SleepMS((long)config.metroDur);
			CMEndNoteNow(gDestID, config.metroNote, config.metroChannel);
		}
		else if (useWhichMIDI==MIDIDR_FMS) {
			FMSStartNoteNow(config.metroNote, config.metroChannel, config.metroVelo, config.metroDevice);

			/*
			 * ??The following use of SleepMS is not great--the implementation as of v.3.0A
			 * is terrible and may not give accurate enough timing for this use, especially
			 * on a very fast Mac.
			 */
			SleepMS((long)config.metroDur);
			FMSEndNoteNow(config.metroNote, config.metroChannel, config.metroDevice);
		}
		else {
			StartNoteNow(config.metroNote, config.metroChannel, config.metroVelo, 0);
			(void)BIMIDINoteAtTime(config.metroNote, config.metroChannel, 0, endTime);
		}
	}
	else {
		/*
		 * You'd think the <accented> option could be implemented with SetSoundVol;
		 * unfortunately, changing from 7 to 1 doesn't reduce the loudness immediately--
		 * it seems to take a few, or maybe a few hundred, milliseconds. This could
		 * easily because of analog hardware limitations, but I've never seen any
		 * comments on this phenomenon.
		 */
		sndH = GetResource('snd ', CLICK_snd);
		if (GoodResource(sndH))
			PlayResource(sndH, TRUE);		/* Play the sound synchronously */
		else
			SysBeep(1);
	}
}


/* ----------------------------------------------------------------- FillFMSMNote -- */

static Boolean FillFMSMNote(MMMIDIPacket *p, Byte channel, MNOTEPTR pMNote)
{
	Byte vNOff, vNOn; 		/* status bytes for Note Off/On on correct channel */
	Boolean	first, done;
	MMMIDIPacket *nextP;
	INT16 command;							
	register long offTime;
	char fmtStr[256];
	
	vNOff = MNOTEOFF+channel;
	vNOn = MNOTEON+channel;

	pMNote->noteNumber = p->data[1];
	pMNote->channel = channel;
	pMNote->startTime = p->tStamp;
	pMNote->onVelocity = p->data[2];
	pMNote->duration = 0;											/* To be filled in later */
	pMNote->offVelocity = config.noteOffVel;

	if (pMNote->onVelocity==0)										/* Really a Note Off */
		return FALSE;

	done = FALSE;
	first = TRUE;
	while (nextP = PeekAtNextFMSMIDIPacket(first)) {
		if (nextP->data[0] & MSTATUSMASK) {
			if (nextP->data[0]==vNOff)			command = MNOTEOFF;	/* Pick up (on our channel) Note Off */
			else if (nextP->data[0]==vNOn)	command = MNOTEON;	/*		and On messages */
			else										command = 0;			/* Ignore all other messages */
		}
		switch (command) {
			case 	MNOTEOFF:
				if (nextP->data[1]==pMNote->noteNumber) {
					offTime = nextP->tStamp;									/* Get milliseconds */
					pMNote->duration = offTime-pMNote->startTime;
					if (pMNote->onVelocity<config.minRecVelocity			/* Too soft and */
					&& pMNote->duration<config.minRecDuration)			/*   too short? */
						 	return FALSE;
					pMNote->offVelocity = nextP->data[2];
					DeletePeekedAtFMSMIDIPacket();
					done = TRUE;
				}
				break;
				
			case MNOTEON:
				if (nextP->data[1]==pMNote->noteNumber) {
					if (nextP->data[2]==0) {									/* Really a Note Off */
						offTime = nextP->tStamp;								/* Get milliseconds */
						pMNote->duration = offTime-pMNote->startTime;
						if (pMNote->onVelocity<config.minRecVelocity		/* Too soft and */
						&& pMNote->duration<config.minRecDuration)		/*   too short? */
						 	return FALSE;
						pMNote->offVelocity = config.noteOffVel;
						DeletePeekedAtFMSMIDIPacket();
						done = TRUE;
					}
				}
				break;
				
			default:
				break;
		}
	first = FALSE;
	}

	if (!done) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 2);					/* "No Note Off for Note On" */
		sprintf(strBuf, fmtStr, pMNote->startTime/1000L, pMNote->startTime%1000L,
								pMNote->noteNumber);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		pMNote->duration = 0;
		pMNote->offVelocity = config.noteOffVel;
	}
	return TRUE;
}


/* ----------------------------------------------------------------- FillOMSMNote -- */

static Boolean FillOMSMNote(MMMIDIPacket *p, Byte channel, MNOTEPTR pMNote)
{
	Byte vNOff, vNOn; 		/* status bytes for Note Off/On on correct channel */
	Boolean	first, done;
	MMMIDIPacket *nextP;
	INT16 command;							
	register long offTime;
	char fmtStr[256];
	
	vNOff = MNOTEOFF+channel;
	vNOn = MNOTEON+channel;

	pMNote->noteNumber = p->data[1];
	pMNote->channel = channel;
	pMNote->startTime = p->tStamp;
	pMNote->onVelocity = p->data[2];
	pMNote->duration = 0;											/* To be filled in later */
	pMNote->offVelocity = config.noteOffVel;

	if (pMNote->onVelocity==0)										/* Really a Note Off */
		return FALSE;

	done = FALSE;
	first = TRUE;
	while (nextP = PeekAtNextOMSMIDIPacket(first)) {
		if (nextP->data[0] & MSTATUSMASK) {
			if (nextP->data[0]==vNOff)			command = MNOTEOFF;	/* Pick up (on our channel) Note Off */
			else if (nextP->data[0]==vNOn)	command = MNOTEON;	/*		and On messages */
			else										command = 0;			/* Ignore all other messages */
		}
		switch (command) {
			case 	MNOTEOFF:
				if (nextP->data[1]==pMNote->noteNumber) {
					offTime = nextP->tStamp;									/* Get milliseconds */
					pMNote->duration = offTime-pMNote->startTime;
					if (pMNote->onVelocity<config.minRecVelocity			/* Too soft and */
					&& pMNote->duration<config.minRecDuration)			/*   too short? */
						 	return FALSE;
					pMNote->offVelocity = nextP->data[2];
					DeletePeekedAtOMSMIDIPacket();
					done = TRUE;
				}
				break;
				
			case MNOTEON:
				if (nextP->data[1]==pMNote->noteNumber) {
					if (nextP->data[2]==0) {									/* Really a Note Off */
						offTime = nextP->tStamp;								/* Get milliseconds */
						pMNote->duration = offTime-pMNote->startTime;
						if (pMNote->onVelocity<config.minRecVelocity		/* Too soft and */
						&& pMNote->duration<config.minRecDuration)		/*   too short? */
						 	return FALSE;
						pMNote->offVelocity = config.noteOffVel;
						DeletePeekedAtOMSMIDIPacket();
						done = TRUE;
					}
				}
				break;
				
			default:
				break;
		}
	first = FALSE;
	}

	if (!done) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 2);					/* "No Note Off for Note On" */
		sprintf(strBuf, fmtStr, pMNote->startTime/1000L, pMNote->startTime%1000L,
								pMNote->noteNumber);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		pMNote->duration = 0;
		pMNote->offVelocity = config.noteOffVel;
	}
	return TRUE;
}


/* ----------------------------------------------------------------- FillOMSMNote -- */

static Boolean FillCMMNote(MIDIPacket *p, Byte channel, MNOTEPTR pMNote)
{
	Byte vNOff, vNOn; 		/* status bytes for Note Off/On on correct channel */
	Boolean	first, done;
	MIDIPacket *nextP;
	INT16 command;							
	register long offTime;
	char fmtStr[256];

	vNOff = MNOTEOFF+channel;
	vNOn = MNOTEON+channel;

	pMNote->noteNumber = p->data[1];
	pMNote->channel = channel;
	pMNote->startTime = CMTimeStampToMillis(p->timeStamp);
	pMNote->onVelocity = p->data[2];
	pMNote->duration = 0;											/* To be filled in later */
	pMNote->offVelocity = config.noteOffVel;

	if (pMNote->onVelocity==0)										/* Really a Note Off */
		return FALSE;

	done = FALSE;
	first = TRUE;
	while ((nextP = PeekAtNextCMMIDIPacket(first)) && !done)
	{
		if (nextP->data[0] & MSTATUSMASK) {
			if (nextP->data[0]==vNOff)			command = MNOTEOFF;			/* Pick up (on our channel) Note Off */
			else if (nextP->data[0]==vNOn)	command = MNOTEON;			/*		and On messages */
			else										command = 0;					/* Ignore all other messages */
		}
		switch (command) {
			case 	MNOTEOFF:
				if (nextP->data[1]==pMNote->noteNumber) {
					offTime = CMTimeStampToMillis(nextP->timeStamp);		/* Get milliseconds */
					pMNote->duration = offTime-pMNote->startTime;
					if (pMNote->onVelocity<config.minRecVelocity				/* Too soft and */
							&& pMNote->duration<config.minRecDuration)		/*   too short? */
						 return FALSE;
					pMNote->offVelocity = nextP->data[2];
					DeletePeekedAtCMMIDIPacket();
					done = TRUE;
				}
				break;
				
			case MNOTEON:
				if (nextP->data[1]==pMNote->noteNumber) {
					if (nextP->data[2]==0) {										/* Really a Note Off */
						offTime = CMTimeStampToMillis(nextP->timeStamp);	/* Get milliseconds */
						pMNote->duration = offTime-pMNote->startTime;
						if (pMNote->onVelocity<config.minRecVelocity			/* Too soft and */
								&& pMNote->duration<config.minRecDuration) 	/*   too short? */
						 	return FALSE;
						pMNote->offVelocity = config.noteOffVel;
						DeletePeekedAtCMMIDIPacket();
						done = TRUE;
					}
				}
				break;
				
			default:
				break;
		}
		
		first = FALSE;
	}

	if (!done) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 2);					/* "No Note Off for Note On" */
		sprintf(strBuf, fmtStr, pMNote->startTime/1000L, pMNote->startTime%1000L,
								pMNote->noteNumber);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		pMNote->duration = 0;
		pMNote->offVelocity = config.noteOffVel;
	}
	return TRUE;
}


/* -------------------------------------------------------------------- FillMNote -- */
/* FillMNote enters data into a temporary MIDI struct for the note whose Note On is in
the MIDI Manager packet at mPacketBuffer[startLoc].

It looks forward in mPacketBuffer for the matching Note Off on the specified channel.
(Since a Note Off can be sent as a Note On with 0 velocity, it checks Note Ons as
well.) It returns TRUE if it finds a valid note whose velocity or duration is above
the threshhold, else FALSE. NB: the temporary MIDI struct may be filled in even if we
return FALSE! */						 

static Boolean FillMNote(
						long 			startLoc,	/* mBuffer location of Note On packet */
						long			mRecLen,
						Byte 			channel,
						MNOTEPTR		pMNote
						)
{
	INT16 done, command;							
	Byte vNOff, vNOn; 			/* status bytes for Note Off/On on correct channel */
	register long offTime, loc;
	MMMIDIPacket *p;
	char fmtStr[256];
	
	loc = startLoc;
	
	vNOff = MNOTEOFF+channel;
	vNOn = MNOTEON+channel;
				
	/* Point p to the beginning of the MMMIDIPacket */
	p = (MMMIDIPacket *)&(mPacketBuffer[loc]);

	pMNote->noteNumber = p->data[1];
	pMNote->channel = channel;
	pMNote->startTime = p->tStamp;
	if (loc>mRecLen) return FALSE;
	pMNote->onVelocity = p->data[2];
	pMNote->duration = 0;											/* To be filled in later */
	pMNote->offVelocity = config.noteOffVel;

	if (pMNote->onVelocity==0)										/* Really a Note Off */
		return FALSE;

	done = FALSE;
	loc += ROUND_UP_EVEN(p->len);
	while (!done && loc<=mRecLen) {								/* Look for the desired Note Off	*/
		/* Point p to the beginning of the next MMMIDIPacket and get its length */
		p = (MMMIDIPacket *)&(mPacketBuffer[loc]);
		
		if (p->data[0] & MSTATUSMASK) {
			if (p->data[0]==vNOff)	   command = MNOTEOFF;			/* Pick up (on our channel) Note Off */
			else if (p->data[0]==vNOn) command = MNOTEON;			/*		and On messages */
			else 						 		command = 0;					/* Ignore all other messages */
		}
				
		switch (command) {
			case 	MNOTEOFF:
				if (p->data[1]==pMNote->noteNumber) {
					offTime = p->tStamp;											/* Get milliseconds */
					pMNote->duration = offTime-pMNote->startTime;
					if (pMNote->onVelocity<config.minRecVelocity			/* Too soft and */
					&& pMNote->duration<config.minRecDuration)			/*   too short? */
						 	return FALSE;
					pMNote->offVelocity = p->data[2];
					done = TRUE;
				}
				break;
				
			case MNOTEON:
				if (p->data[1]==pMNote->noteNumber) {
					if (p->data[2]==0) {											/* Really a Note Off */
					offTime = p->tStamp;											/* Get milliseconds */
						pMNote->duration = offTime-pMNote->startTime;
						if (pMNote->onVelocity<config.minRecVelocity		/* Too soft and */
						&& pMNote->duration<config.minRecDuration)		/*   too short? */
						 		return FALSE;
						pMNote->offVelocity = config.noteOffVel;
						done = TRUE;
					}
				}
				break;
				
			default:
				;
		}
		loc += ROUND_UP_EVEN(p->len);
	}
	
	if (!done) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 2);					/* "No Note Off for Note On" */
		sprintf(strBuf, fmtStr, pMNote->startTime/1000L, pMNote->startTime%1000L,
								pMNote->noteNumber);

		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		pMNote->duration = 0;
		pMNote->offVelocity = config.noteOffVel;
	}
	return TRUE;
}


#define WARN_IGNORED (CapsLockKeyDown() && ShiftKeyDown())

/* ------------------------------------------------------------- RawMIDI2Packets -- */
/* Find all Note Ons and Offs in the (MacTutor driver format) "raw" buffer and convert
them to MIDI Manager packets. Return the total length of the MIDI packets. */

static long RawMIDI2Packets(long rBufLen)
{
	Byte midiByte, command, currentChan;
	register long r, m;
	long ignored;
	MMMIDIPacket *p;
	INT16 len;
	char fmtStr[256];

	command = 0;
	r = m = 0L;
	ignored = 0L;
	
	/* Scan the MacTutor-format input buffer, looking for Note Ons and Note Offs. */
	while (r<rBufLen) {
		midiByte = rawBuffer[r];
		if (midiByte & MSTATUSMASK) { 							/* Is it a status byte? */
			command = midiByte & MCOMMANDMASK;
			currentChan = midiByte & MCHANNELMASK;
			r++;
		}
		else {															/* Not status byte, must be data */
			switch (command) {
				case MNOTEON:
				case MNOTEOFF:
					p = (MMMIDIPacket *)&mPacketBuffer[m];
					RawMIDI2MMNote(r+1, (command==MNOTEON), currentChan, &len, p);
					m += len + ((len & 1) ? 1 : 0);
					r += 2L;
					break;
				default:
					ignored++;
					r++;
					break;
			}
		}
	}

	if (ignored>0 && WARN_IGNORED) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 3);		/* "Ignored %ld non-Note data bytes" */
		sprintf(strBuf, fmtStr, ignored);
		CParamText(strBuf, "", "", "");
		StopInform(SMALL_GENERIC_ALRT);
	}

	return m;
}


static void *GetNextMidiPacket(long mRecLen, long loc)
{
	/* Point p to the beginning of the next MMMIDIPacket and get its length */
	if (useWhichMIDI == MIDIDR_OMS) {
		return GetOMSMIDIPacket();
	}
	if (useWhichMIDI == MIDIDR_FMS) {
		return GetFMSMIDIPacket();
	}
	if (useWhichMIDI == MIDIDR_CM) {
		return GetCMMIDIPacket();
	}
	if (loc < mRecLen) {
		return &(mPacketBuffer[loc]);
	}
	
	return NULL;
}

/* ------------------------------------------------------------------- MIDI2Night -- */
/* Build primitive Nightingale Syncs from Note Ons and Offs in the MMMIDIPacket
buffer, ignoring any that are not on the channel(s) requested and any that start
during the lead-in time (considering STARTTIME_SLOP), and insert them into the main
object list before <selStartL>. Return the end time of the last note if we insert
anything, else returns -1L. */

// ••
// •• Need to rewrite to handle the fact that we are using MIDIPackets, not MMMIDIPackets.
// ••

#define TIME_SCALEFACTOR 8	/* Large enuf to avoid overflow, small enuf to keep precision */

/* Convert milliseconds to scaled PDUR ticks at a constant tempo */
#define MS2PDUR(msec, timeScale)	( ((msec)*(timeScale))/(60*1000L) )

/* Keep notes that start during lead-in if they're this close to lead-in end (PDUR ticks) */
#define STARTTIME_SLOP (5*PDURUNIT/TIME_SCALEFACTOR)

static long MIDI2Night(
					Document *doc,
					INT16 useChan,					/* <0 = accept notes on any channel */
					long mRecLen,
					long tLeadInOffset			/* Duration of lead-in time (PDUR ticks); -1L=to 1st note */
					)
{
	Byte		command, channel;
	MNOTE 	theNote;
	long		loc, prevStartTime,
				timeShift,					/* PDUR tick time of first recorded note */
				firstTime, ignored,
				timeScale, lastEndTime,
				scaledTLeadInOffset;
	LINK		lSync, oldSelStart, firstSync, aNoteL;
	Boolean	anyChan;
	MMMIDIPacket *pMM;
	MIDIPacket *pCM;
	char		fmtStr[256];
	Boolean	noteFilled;
	
	if (printDebug) PrintBuffer(doc, mRecLen);

	anyChan = (useChan<0);
	oldSelStart = doc->selStartL;
	command = 0;
	prevStartTime = -999999L;
	lSync = firstSync = NILINK;
	loc = 0L;
	ignored = 0L;
	
	timeScale = GetTempo(doc, doc->selStartL);
	timeScale /= TIME_SCALEFACTOR;							/* To avoid overflow: see comment above */
	if (tLeadInOffset<0L)
		scaledTLeadInOffset = 0L;
	else
		scaledTLeadInOffset = tLeadInOffset/TIME_SCALEFACTOR;

	/* Look for Note Ons. When we find one, try to make a Nightingale note out of it
	 * by looking for its corresponding Note Off. If it starts within <deflamTime> of
	 * the previous note, put it in a chord with the previous note.
	 */
	 
	if (useWhichMIDI == MIDIDR_CM) {
		SetCMMIDIPacket();
		CMNormalizeTimeStamps();
	}
	 
	while (TRUE) {
		/* Point p to the beginning of the next MMMIDIPacket and get its length */
		if (useWhichMIDI == MIDIDR_CM) {
			pCM = (MIDIPacket *)GetNextMidiPacket(mRecLen, loc);
			if (!pCM) break;
			command = pCM->data[0] & MCOMMANDMASK;
			channel = pCM->data[0] & MCHANNELMASK;
		}
		else {
			pMM = (MMMIDIPacket *)GetNextMidiPacket(mRecLen, loc);
			if (!pMM) break;
			command = pMM->data[0] & MCOMMANDMASK;
			channel = pMM->data[0] & MCHANNELMASK;
		}
		
		if (command==MNOTEON && (anyChan || channel==useChan)) {
			if (useWhichMIDI == MIDIDR_CM)
				noteFilled = FillCMMNote(pCM, channel, &theNote);
			else if (useWhichMIDI == MIDIDR_OMS)
				noteFilled = FillOMSMNote(pMM, channel, &theNote);
			else if (useWhichMIDI == MIDIDR_FMS)
				noteFilled = FillFMSMNote(pMM, channel, &theNote);
			else
				noteFilled = FillMNote(loc, mRecLen, channel, &theNote);
				
			if (noteFilled) {
				if (printDebug) PrintNote(&theNote, FALSE);

				/*
				 * Convert time (milliseconds to PDUR ticks). If the note started during
				 * the lead in, ignore it; else do transposition and go on.
				 */
				theNote.startTime = MS2PDUR(theNote.startTime, timeScale);
				theNote.startTime -= scaledTLeadInOffset;
				if (theNote.startTime>=(-STARTTIME_SLOP) && theNote.startTime<0)
					theNote.startTime = 0;
				if (theNote.startTime<0)
					goto NextEvent;

				theNote.duration = MS2PDUR(theNote.duration, timeScale);
							
				theNote.startTime *= TIME_SCALEFACTOR;
				theNote.duration *= TIME_SCALEFACTOR;

				theNote.noteNumber -= transpose;
	
				/*
				 *	We need to offset start times of recorded notes so they'll play
				 * properly after previous notes in the score. In order to do this,
				 * we set <timeShift> so CreateSync will put the Syncs into the object
				 * list with appropriate timestamps.
				 */
				if (!firstSync) {
					firstTime = GetLTime(doc, doc->selStartL);
					if (tLeadInOffset<0L)
						timeShift = -firstTime+theNote.startTime;
					else
						timeShift = -firstTime;
				}
#ifndef PUBLIC_VERSION
				if (printDebug) DebugPrintf("; start-shift=%ld ticks",
					theNote.startTime-timeShift);
#endif
				if (theNote.startTime-prevStartTime<doc->deflamTime)
					aNoteL = AddNoteToSync(doc, theNote, lSync,
												USESTAFF(doc, theNote.noteNumber),
												UNKNOWN_L_DUR, 0,
												USEVOICE(doc, USESTAFF(doc, theNote.noteNumber)),
												FALSE, timeShift);
				else {
					aNoteL = CreateSync(doc, theNote, &lSync,
												USESTAFF(doc, theNote.noteNumber), 
												UNKNOWN_L_DUR, 0,
												USEVOICE(doc, USESTAFF(doc, theNote.noteNumber)),
												FALSE, timeShift);
					lastEndTime = theNote.startTime+theNote.duration;
#ifndef PUBLIC_VERSION
					if (printDebug) DebugPrintf(" L%d", lSync);
#endif
				}
				if (!firstSync) firstSync = lSync;
				prevStartTime = theNote.startTime;
NextEvent:
#ifdef PUBLIC_VERSION
				;
#else
				if (printDebug) DebugPrintf("\n");
#endif
			}

		}
		if (useWhichMIDI == MIDIDR_MM)
			loc += ROUND_UP_EVEN(pMM->len);
	}

	if (ignored>0 && WARN_IGNORED) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 3);			/* "Ignored %ld non-Note data bytes" */
		sprintf(strBuf, fmtStr, ignored);
		CParamText(strBuf, "", "", "");
		StopInform(SMALL_GENERIC_ALRT);
	}

	/*
	 * Clean up. Set the selection range to everything we've recorded; try to get rid
	 * of unisons in chords; and, optionally, delete redundant accidentals and set
	 * note velocities to default value for the range.
	 */
	if (firstSync) {
		doc->selStartL = firstSync;								/* Set sel range to everything just recorded */
		doc->selEndL = oldSelStart;

		AvoidUnisonsInRange(doc, doc->selStartL, doc->selEndL, doc->selStaff);
		if (split) AvoidUnisonsInRange(doc, doc->selStartL, doc->selEndL, otherStaff);

		if (config.delRedundantAccs) DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);
		if (config.ignoreRecVel) SetVelFromContext(doc, TRUE);

		return (lastEndTime-timeShift>=0L? lastEndTime-timeShift : 0L);
	}
	else
		return -1L;
}	


#include "MIDIPASCAL3.h"

/* Convert MIDI Pascal-format data to MacTutor format. Expedient: I don't have time to
rewrite higher-level routines that assume MacTutor format. But even more repulsive
than it looks at first, since the MacTutor-format buffer will then be converted
to MIDI Manager format! Oh well. */

void MIDIDataToLong(INT16 midiWord, long timeStamp, long *pmdata)
{
	long macTutorLong;
	
	/* Move the 2nd, 3rd, and 4th bytes of <timeStamp> to the 1st 3 bytes of <*pmdata>. */
	
	macTutorLong = timeStamp << 8;
	macTutorLong |= (Byte)midiWord;
	*pmdata = macTutorLong;
}

void OutOfRawRecMemory()
{
	char fmtStr[256];

	GetIndCString(fmtStr, MIDIERRS_STRS, 8);			/* "Out of memory. Raw record buffer full." */
	sprintf(strBuf, fmtStr, rawBufferLength);
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
}

void OutOfNoteOnRecMemory()
{
	GetIndCString(strBuf, MIDIERRS_STRS, 9);			/* "Out of memory. Note On Buf exceeded." */
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
}


/* Prototypes for play-while-recording routines, which are defined elsewhere. */

INT16 RecPreparePlayback(Document *doc, long msPerBeat, long *ptLeadInOffset);
Boolean RecPlayNotes(unsigned INT16	outBufSize);

#define BIMIDI_BUFSIZE 3000			/* Enough for any situation I've heard of */


/* ---------------------------------------------------------------- RecordBuffer -- */
/* Real-time record into <rawBuffer>. Optionally handles metronome, either via MIDI
or internal sound, and plays back the existing content of the score. */

static void RecordBuffer(
					Document *doc,
					Boolean playAlong,
					Boolean metronome,
					INT16 tempo,			/* in beats per minute */
					Rect flashRect,
					long timeScale,		/* tempo in PDUR ticks per minute */
					long *ptLeadInOffset	/* output: duration of lead-in time (PDUR ticks); -1L=to 1st note */
					)
{
	long				msPerBeat, mdata, ignoredRTM, microbeats, maxMS, elapsedMS;
	long				oldRecIndex, timeMS, nextClickMS, startClickMS,
						beatCount, timeStamp;
	register long	r;
	Boolean			done;
	Byte				midiByte;
	INT16				midiWord;
	char				fmtStr[256];
	unsigned INT16	outBufSize;
	INT16				npBufSize;

	msPerBeat = (1000*60L)/tempo;
	beatCount = 0L;
	nextClickMS = 0L;
	microbeats = TSCALE2MICROBEATS(timeScale);
	maxMS = PDUR2MS(MAX_SAFE_MEASDUR, microbeats);
	r = mRecIndex = 0L;

	if (playAlong && useWhichMIDI==MIDIDR_BI) {
		npBufSize = RecPreparePlayback(doc, msPerBeat, ptLeadInOffset);
		if (npBufSize<=0) return;
	}
	else
		*ptLeadInOffset = -1L;

	ArrowCursor();

/* *** BEGIN SEMI-REALTIME, LOW-LEVEL CODE. Change carefully! *** */

	/*
	 * Initialize low-level MIDI stuff, start timer, etc. For MIDI Manager, flush
	 * its buffer and set the recording-in-progress flag. For built-in MIDI, get a
	 * buffer large enough to play along everything.
	 */
	switch (useWhichMIDI) {
		case MIDIDR_FMS:
			InitFMSBuffer();
			oldRecIndex = 0L;
			FMSInitTimer();
			break;
		case MIDIDR_OMS:
			InitOMSBuffer();
			oldRecIndex = 0L;
			OMSInitTimer();
			break;
		case MIDIDR_CM:
			// •• InitCMBuffer();
			oldRecIndex = 0L;
			CMInitTimer();
			break;
		case MIDIDR_BI:
			outBufSize = 3*(npBufSize+1);
			MayInitBIMIDI(BIMIDI_BUFSIZE, outBufSize);		/* initialize hardware, allocate buffer */
			InitBIMIDITimer();										/* initialize millisecond timer */
			break;
		default:
			break;
	}
	StartMIDITime();
	
	if (playAlong && useWhichMIDI==MIDIDR_BI)
		if (!RecPlayNotes(outBufSize)) MayErrMsg("RecPlayNotes failed.");

	if (useWhichMIDI==MIDIDR_MM) MIDIPoll(inputMMRefNum, midiGetEverything);
	recordingNow = TRUE;

	InvertRect(&flashRect);									/* Confirm recording is in progress */

	/* Get MIDI data, one byte plus time stamp at a time. Since System Real Time
	 *	messages can occur anywhere, even between the bytes of another message, and
	 *	since they have no effect on Running Status and detecting them can be done
	 *	quickly enough to avoid losing any incoming data, it's convenient to ignore
	 *	them here--until such time as Nightingale can start using them.
	 */
 	ignoredRTM = 0L;
	done = FALSE;
	while (!done) {
		if (metronome) {
			timeMS = GetMIDITime(0L);
			if (timeMS>=nextClickMS) {
				if (!(config.metroViaMIDI && playAlong && useWhichMIDI==MIDIDR_BI))
					SoundClickNow(config.metroViaMIDI, FALSE);
				if (beatCount==0L) startClickMS = timeMS;
				beatCount++;
				elapsedMS = beatCount*msPerBeat;
				nextClickMS = startClickMS+elapsedMS;
			}
		}

		if (GetMIDITime(0L)>maxMS) {
			GetIndCString(strBuf, MIDIERRS_STRS, 4);		/* "You've reached the time limit" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			done = TRUE;
		}

		switch (useWhichMIDI) {
			case MIDIDR_FMS:
				if (gFMSMIDIBufferFull) {
					GetIndCString(fmtStr, MIDIERRS_STRS, 26);		/* "Reached the limit of %d MIDI packets in one take" */
					sprintf(strBuf, fmtStr, mRecIndex);
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					done = TRUE;
				}
				else {
					FillFMSBuffer();
					if (mRecIndex > oldRecIndex) {
						QuickFlashRect(&flashRect);					/* Flash to reassure user */
						oldRecIndex = mRecIndex;
					}
				}
				break;
			case MIDIDR_OMS:
				/* Nothing but UI to do here: all else is done at interrupt level in NightOMSReadHook */
				if (gOMSMIDIBufferFull) {
					GetIndCString(fmtStr, MIDIERRS_STRS, 26);		/* "Reached the limit of %d MIDI packets in one take" */
					sprintf(strBuf, fmtStr, mRecIndex);
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					done = TRUE;
				}
				else if (mRecIndex > oldRecIndex) {
					QuickFlashRect(&flashRect);						/* Flash to reassure user */
					oldRecIndex = mRecIndex;
				}
				break;
			case MIDIDR_CM:
				/* Nothing but UI to do here: all else is done at interrupt level in NightOMSReadHook */
				if (gCMMIDIBufferFull) {
					GetIndCString(fmtStr, MIDIERRS_STRS, 26);		/* "Reached the limit of %d MIDI packets in one take" */
					sprintf(strBuf, fmtStr, mRecIndex);
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					done = TRUE;
				}
				else if (mRecIndex > oldRecIndex) {
					QuickFlashRect(&flashRect);						/* Flash to reassure user */
					oldRecIndex = mRecIndex;
				}
				break;
			case MIDIDR_MM:
				oldRecIndex = mRecIndex;
				MIDIPoll(inputMMRefNum, midiGetCurrent);
				if (mRecIndex>oldRecIndex)
					QuickFlashRect(&flashRect);						/* Flash to reassure user */
				break;
			case MIDIDR_BI:
				MidiIn((int *)&midiWord, &timeStamp);
				if (midiWord>=0) {
					MIDIDataToLong(midiWord, timeStamp, &mdata);	/* Convert MIDI Pascal to MacTutor format! */
					midiByte = midiWord;
					if (midiByte<MLOW_SYSREALTIME						/* skip System Real Time msgs */
					||  midiByte>MHI_SYSREALTIME) {
						rawBuffer[r++] = mdata;
						if (r>=rawBufferLength) {
							OutOfRawRecMemory();
							done = TRUE;
						}
						if ((r % 10)==1) QuickFlashRect(&flashRect);	/* Flash every so often to reassure user */
					}
					else
						ignoredRTM++;
				}
				break;
			default:
				break;
		}

		if (Button()) {
			done = TRUE;
		}
	}
	
	recordingNow = FALSE;
	StopMIDITime();

/* *** END SEMI-REALTIME, LOW-LEVEL CODE. *** */

	if (useWhichMIDI==MIDIDR_BI) mRecIndex = RawMIDI2Packets(r);

	if (ignoredRTM && WARN_IGNORED) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 5);			/* "Ignored %ld Real Time messages" */
		sprintf(strBuf, fmtStr, ignoredRTM);
		CParamText(strBuf, "", "", "");
		StopInform(SMALL_GENERIC_ALRT);
	}
	FlushEvents(mDownMask, 0);									/* Discard mouse click that ended recording */

	/*
	 * If any notes were playing when the user cancelled recording, they don't get
	 * turned off; I have no idea why. Solutions less drastic than this (Midi(0),
	 * MayResetBIMIDI(), etc.--done before the call to StopMIDITime) have all failed;
	 * this seems to work solidly, but the situation isn't too satisfying. (I haven't
	 * tried shorter delays before AllNotesOff.)
	 */
	if (useWhichMIDI==MIDIDR_BI) {
		SleepMS(200L);
		AllNotesOff();
		ArrowCursor();
	}
}


/* ------------------------------------------------------- Record help functions -- */

static LINK RNewMeasure(Document *, LINK);
LINK LocateRInsertPt(LINK);
Boolean EncloseSel(Document *);

/* Add a new Measure object just before <pL> and leave an insertion point set just
before <pL>. Similar but not identical to MFNewMeasure. */

static LINK RNewMeasure(Document *doc, LINK pL)
{
	LINK measL; INT16 sym; CONTEXT context;

	NewObjInit(doc, MEASUREtype, &sym, singleBarInChar, ANYONE, &context);
	measL = CreateMeasure(doc, pL, -1, sym, context);
	if (!measL) return NILINK;
	
	doc->selStartL = doc->selEndL = pL;
	return measL;
}

/* On each end of the selection range, add a barline, unless there's already one there.
Assumes all Syncs in the selection range are selected when it's called, and leaves the
selection set to all these Syncs and all their constituent notes and rests; the
surrounding barlines are left unselected. */

Boolean EncloseSel(Document *doc)
{
	LINK startL, endL, prevL, measL, nextL, pL, aNoteL;
	
	startL = doc->selStartL; endL = doc->selEndL;
	
	prevL = FirstValidxd(LeftLINK(startL), GO_LEFT);
	if (!prevL || !MeasureTYPE(prevL)) {
		measL = RNewMeasure(doc, startL);				/* Destroys the selection range */
		if (!measL) return FALSE;
		DeselRangeNoHilite(doc, measL, RightLINK(measL));
	}

	nextL = FirstValidxd(endL, GO_RIGHT);
	if (!nextL || !MeasureTYPE(nextL)) {
		measL = RNewMeasure(doc, endL);					/* Destroys the selection range */
		if (!measL) return FALSE;
		DeselRangeNoHilite(doc, measL, RightLINK(measL));
	}
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				NoteSEL(aNoteL) = TRUE;
	
	doc->selStartL = startL; doc->selEndL = endL;
	OptimizeSelection(doc);									/* At least if we added end barline */
	
	return TRUE;
}

/* ---------------------------------------------------------------------- Record -- */
/* Record data from MIDI, generate Nightingale notes of unknown duration, and
add them to the object list at the insertion point. Returns TRUE if it adds
at least one note to the object list, else FALSE (due to the user cancelling
the record, an error condition, or just no note on the right channel).

Record does not format the material recorded so it can be drawn; to do so, after
calling Record, call RespAndRfmtRaw or RespaceBars.

One problem with this version: only Note On and Off are recognized by
MIDI2Night, but all other non-System-Real-Time status bytes--e.g., pitch bend,
controller changes, System Exclusive--get into the buffer, taking up valuable
space; pitch bend and other continuous controllers are particularly piggish.
(NOTE: This is not true for FreeMIDI, since we discard these packets in
FMSReadHook.) */

Boolean Record(Document *doc)
{
	static Boolean metronome=TRUE;
	long timeChange, tLeadInOffset;
	INT16 partn, useChan;
	LINK endMeasL;
	
	DebugPrintf("1. Starting to record\n");
	
	if (doc->selStartL!=doc->selEndL) {
		GetIndCString(strBuf, MIDIERRS_STRS, 6);		/* "Can't record without insertion pt" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	timeChange = -1L;

	WaitCursor();
	if (!AllocRawBuffer()) {
		NoMoreMemory();
		goto Finished;
	}

	if (useWhichMIDI == MIDIDR_CM) {
		if (!ResetMIDIPacketList()) {
			NoMoreMemory();
			goto Finished;
		}
	}
		
	DebugPrintf("2. CoreMIDI set up\n");
	
	SetRecordFlats(doc);
	
	partn = Staff2Part(doc, doc->selStaff);
	useChan = doc->channel;
	if (!RecordDialog(doc, doc->selStaff, partn, useChan, FALSE, FALSE, &otherStaff,
							&metronome, &tLeadInOffset)) {
#if CMDEBUG
		DebugPrintf("doc inputDev=%ld useChan=%ld\n", doc->cmInputDevice, (long)useChan);
		CMDebugPrintXMission();
#endif
		goto Finished;
	}
	
	DebugPrintf("3. Record Dialog done\n");
	
	WaitCursor();
	timeChange = MIDI2Night(doc, useChan-1, mRecIndex, -1L);	/* Build Nightingale object list */

	DebugPrintf("3. Generating Nightingale data structure\n");
	
	if (timeChange>=0L) {
		WaitCursor();
		
		DebugPrintf("4. Fixing up Nightingale data structure\n");
	
		
		endMeasL = EndMeasSearch(doc, doc->selEndL);
		MoveTimeInMeasure(doc->selEndL, doc->tailL, timeChange);	/* Move remainder of meas. */		
		MoveTimeMeasures(doc->selEndL, doc->tailL, timeChange);	/* Move following meas. */
		UpdateMeasNums(doc, NILINK);
		UpdateVoiceTable(doc, TRUE);

		/*
		 * Reformatting may destroy selection status, so we use notes' tempFlags to
		 * save and restore it.
		 */
		NotesSel2TempFlags(doc);
		if (RespAndRfmtRaw(doc, doc->selStartL, doc->selEndL,
						MeasSpaceProp(doc->selStartL)))
			TempFlags2NotesSel(doc);

		/*
		 *	Be sure the the recorded material has barlines around it, since the Transcribe
		 *	Recording command works on the contents of whole measures; then respace, and
		 * move following measures on the system accordingly.
		 */
		EncloseSel(doc);
		RespaceBars(doc, LeftLINK(doc->selStartL), endMeasL,
						MeasSpaceProp(doc->selStartL), FALSE, FALSE);
	}
	else {
		DebugPrintf("4. No Nightingale objects created\n");
	}

Finished:
	if (rawBuffer != NULL)
		DisposePtr((Ptr)rawBuffer);
	return (timeChange>=0L);
}


/* ===================================================== Step Recording functions == */

/* ------------------------------------------------------------------ NoteOnDemon -- */
/* If NoteOnDemon is called faithfully with every byte of MIDI data <midiByte>
and is told what channel number to look for, <useChan>, it will deliver TRUE
when it finds the last byte of a "real" Note On (i.e., the second data byte of
a Note On with non-zero velocity) on that channel. It understands Running Status. */

static Boolean NoteOnDemon(INT16 useChan,				/* 0 to 15 */
									Byte midiByte)
{
	static Byte		prevChan=255;			/* Guarantee we initialize on 1st call */
	static Byte		command;
	static INT16	noteOnBNum;
	Byte				currentChan;

	/* On the first call and any time <useChan> has changed, reinitialize <command>. */
	
	if (useChan!=prevChan) command = 0;
	prevChan = useChan;

	if (midiByte & MSTATUSMASK) {   					/* See if it's a status byte */
		currentChan = midiByte & MCHANNELMASK;
		if (currentChan==useChan) command = midiByte & MCOMMANDMASK;
		else							  command = 0;
		if (command==MNOTEON) noteOnBNum = 0;
		return FALSE;
	}
	else if (command==MNOTEON) {
			noteOnBNum++;
			if (noteOnBNum==2) {							/* Note data byte 2 is velocity */
				noteOnBNum = 0;
				return (midiByte!=0);					/* Velocity 0 means really a Note Off */
			}
			else
				return FALSE;
		}
	else
		return FALSE;
}


/* ------------------------------------------------------------------- Clocks2Dur -- */
/* Given a number of clock ticks, >=1, and the logical duration 1 tick corresponds to,
deliver the logical duration and number of dots for that many ticks. */

static Boolean Clocks2Dur(INT16 clocks, INT16 clockLDur, INT16 *pLDur, INT16 *pnDots)
{
	Boolean allOK=TRUE; INT16 k,nOnes,highestBit;
	
	if (clocks<1) { allOK = FALSE; clocks = 1; }

	/* Find highest order bit number */
	for (k=15; k>=0; k--)
		if (clocks & (1<<k)) break;			/* Has to break if clocks is non-zero */
	
	nOnes = 1;										/* We've found at least one 1 */
	highestBit = k;								/* Keep track of highest order bit */

	/* Now scan down from k looking for first 0 bit to count number of consecutive ones */
	while (--k >= 0) {
		if ((clocks & (1<<k)) == 0) break;	/* Doesn't have to happen if rest of clocks is all 1's */
		nOnes++;
		}
	
	*pLDur = clockLDur-highestBit;
	*pnDots = nOnes-1;

	if (*pLDur<BREVE_L_DUR) { allOK = FALSE; *pLDur = BREVE_L_DUR; }

	if (!allOK) SysBeep(1);
	return allOK;
}


/* ------------------------------------------------------------- StepMIDI2Night -- */
/* Generate Nightingale objects (notes, rests, or Measures) from notes in the Note
On buffer, all of which should be for the correct channel, etc. If any are found:
	If symType==SR_NOTE, create a new Sync at the insertion point; put all the notes
		into it as a chord with their logical durations set to clocks*<lDur> (e.g.,
		lDur=eighth	note and clocks=3 gives dotted quarters); and return a link to
		the Sync.
	If symType==SR_REST, likewise, but ignore the note numbers of the notes found and
		insert a	single rest instead. 
	If symType==SR_BARLINE, create a new Measure object at the insertion point and
		return a link to it.
If no notes, rest, or measure are created, either because none are needed or
because of an error, return NILINK.

All commands other than Note Ons are ignored.
*/

enum {
	SR_NOTE,
	SR_REST,
	SR_BARLINE
};

static LINK StepMIDI2Night(
						Document *doc,
						NotePacket nOnBuffer[],
						INT16 nChord,
						INT16 symIndex,	/* symbol table index of Nightingale symbol to generate */
						INT16 noteDur,
						INT16 clocks
						)
{
	Byte			command, channel;
	MNOTE 		theNote;
	Boolean		firstNote;
	LINK			link, aNoteL;
	INT16			lDur, ndots, symType, i;
	CONTEXT  	context;

	if (symtable[symIndex].objtype==MEASUREtype)
		symType = SR_BARLINE;
	else if (symtable[symIndex].subtype!=0)
		symType = SR_REST;
	else
		symType = SR_NOTE;

	Clocks2Dur(clocks, noteDur, &lDur, &ndots);

	firstNote = TRUE;
	link = NILINK;

	for (i = 0; i<nChord; i++) {
		
		command = nOnBuffer[i].data[0] & MCOMMANDMASK;
		channel = nOnBuffer[i].data[0] & MCHANNELMASK;
		
		/* Ignore everything but "real" (non-zero-vel) Note Ons for correct channel */
		if (command==MNOTEON && nOnBuffer[i].data[2]!=0) {
			theNote.noteNumber = nOnBuffer[i].data[1]-transpose;
			theNote.channel = channel;
			theNote.startTime = nOnBuffer[i].tStamp;
			theNote.onVelocity = nOnBuffer[i].data[2];
			theNote.duration = 0;
			theNote.offVelocity = config.noteOffVel;

			if (printDebug) PrintNote(&theNote, TRUE);
			if (firstNote) {
				if (symType==SR_BARLINE) {
					GetContext(doc, LeftLINK(doc->selStartL), 1, &context);
					link = CreateMeasure(doc,doc->selStartL,-1,symIndex,context);
					if (!link) return NILINK;
				}
				else {
					/* We don't use CreateSync's <timeShift>: caller must FixTimeStamps */
					aNoteL = CreateSync(doc, theNote, &link,
												USESTAFF(doc, theNote.noteNumber), lDur,
												ndots,
												USEVOICE(doc, USESTAFF(doc, theNote.noteNumber)),
												(symType==SR_REST), 0L);
					if (!aNoteL) return NILINK;
				}
			}
			else {
				aNoteL = AddNoteToSync(doc, theNote, link,
											USESTAFF(doc, theNote.noteNumber), lDur,
											ndots,
											USEVOICE(doc, USESTAFF(doc, theNote.noteNumber)),
											FALSE, 0L);
				if (!aNoteL) return NILINK;
			}
			firstNote = FALSE;
		}

		/* If we want something other than notes and we already created one, we're done. */
		
		if (symType!=SR_NOTE && !firstNote) break;
	}

	return link;
}


/* ---------------------------------------------------------------- ShowNRCInBox -- */
/* For feedback purposes, show the note, rest, or chord in the given voice of
<syncL> inside a box positioned at the insertion point on the given staff. The
box is wide enough for several notes/rests/chords. If mode is <ShowADD>, slide
its contents to the left far enough to make room for one note/rest/chord at the
right end, then display the new note/rest/chord at the right end. In any case,
pixels underneath the box are destroyed. */

enum {						/* Values for <mode>: */
	ShowINIT,				/* Initialize and add a note/rest/chord to display */
	ShowADD,					/* Add a note/rest/chord to display */
	ShowREPLACE				/* Replace the last note/rest/chord added */
};

#define NRC_WIDTH 8		/* space for each note/rest/chord in linespaces */
#define BOX_WIDTH 3		/* space for entire box in NRC_WIDTHs */

static void ShowNRCInBox(
					Document *doc,
					LINK syncL,
					INT16 voice, INT16 staff,
					Rect *clipRect,			/* Clipping area to restore */
					INT16 mode					/* Which operation should the routine perform: see enum */
					)
{
	static Rect box, lastNRCBox;
	static INT16 position=-1;
	static RgnHandle updateRgn;
	static CONTEXT context;
	static DDIST lineSpace;
	DDIST savexd;
	INT16 slideStep, i, xNRC;
	LINK aNoteL;
	Boolean drawn=FALSE, recalc=FALSE;
		
	switch (mode) {
		case ShowINIT:
			position = -1;													/* Drop thru... */
		case ShowADD:
			/*
			 *	If position is negative, initialize; if it's at or past the right edge
			 *	of our box, slide existing stuff left; otherwise do nothing.
			 */
			if (position<0) {
				GetContext(doc, syncL, staff, &context);
				lineSpace = LNSPACE(&context);
			
				box.top = d2p(context.staffTop-4*lineSpace);
				box.bottom = d2p(context.staffTop+context.staffHeight+4*lineSpace);
				box.left = doc->caret.left;
				box.right = box.left+d2p(BOX_WIDTH*NRC_WIDTH*lineSpace);
				OffsetRect(&box, 0, context.paper.top);
				/*
				 *	Draw two boxes, one inside the other, like a modal dialog window, to
				 *	emphasize that this is not just a segment of the staff with a box around it.
			 	*/
				EraseRect(&box);
				FrameRect(&box);
				InsetRect(&box, 2, 2);
				FrameRect(&box);
			
				InsetRect(&box, 1, 1);
				
				updateRgn = NewRgn();
			}
			else if (position>=BOX_WIDTH-1) {
				slideStep = -d2p(NRC_WIDTH*lineSpace)/3;
				//for (i = 1; i<=3; i++) {
				for (i = 1; i<=4; i++) {
					ScrollRect(&box, slideStep, 0, updateRgn);
					//SleepTicks(2L);
				}
			}
				
			break;
		case ShowREPLACE:
			lastNRCBox = box;
			lastNRCBox.left = box.left+d2p(position*NRC_WIDTH*lineSpace);
			EraseRect(&lastNRCBox);
			break;
		default:
			;
	}
	
	ClipRect(&box);
	
	context.showLines = SHOW_ALL_LINES;		/* ??regardless of what the staff says */
	Draw1Staff(doc, staff, &context.paper, &context, BACKGROUND_STAFF);

	if (mode!=ShowREPLACE) position++;
	if (position>=BOX_WIDTH) position = BOX_WIDTH-1;

	/* Temporarily set the Sync's xd to trick DrawNote and DrawRest into drawing
	 *	into our box, then change it back.
	 */
	savexd = LinkXD(syncL);
	xNRC = doc->caret.left-context.paper.left;
	xNRC += d2p(position*NRC_WIDTH*lineSpace);
	LinkXD(syncL) = p2d(xNRC)-context.measureLeft+4*lineSpace;
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice) {
			if (NoteREST(aNoteL))
				DrawRest(doc, syncL, &context, aNoteL, &drawn, &recalc);
			else
				DrawNote(doc, syncL, &context, aNoteL, &drawn, &recalc);
		}
	LinkXD(syncL) = savexd;
	//FlushPortRect(&box);

	ClipRect(clipRect);
}


/* ---------------------------------------------- Other StepRecord help functions -- */

static INT16 MRSetNRCDur(Document *, LINK, INT16, INT16, INT16, INT16, INT16);
static short PowerOf2Floor(short num);
static void HiliteRecSync(Document *, long, LINKTIMEINFO [], INT16);
static void InvertMsgRect(Document *);
static void QuickFlashMsgRect(Document *);
static Boolean IsOurNoteOn(void *, INT16);
static INT16 GetAllNoteOns(NotePacket [], INT16, INT16);
static INT16 FindDeflamGap(Document *, NotePacket [], INT16, long);
static Boolean SRKeyDown(Document *doc, EventRecord theEvent, LINK syncL, INT16 voice,
						short *pSymIndex, short *pNoteDur, short *pClocks, short *pNAugDots);

/* Set the logical duration of the given note or rest or every note in the given
chord to clocks*<lDur>, except that if nAugDots>0, it overrides the number of dots
computed from clocks. For example:
	lDur=8th note, clocks=2, nAugDots=1 gives dotted quarters.
	lDur=8th note, clocks=3, nAugDots=0 gives dotted quarters.
	lDur=8th note, clocks=3, nAugDots=2 gives double-dotted quarters.
Return value is the number of aug. dots it ends up using.
This is a special-purpose routine intended for use in step recording; for a general
approach, see SetSelNoteDur. */

static INT16 MRSetNRCDur(
					Document */*doc*/,
					LINK syncL,
					INT16 voice,
					INT16 /*symIndex*/,	/* symbol table index of Nightingale symbol if clocks==1 */
					INT16 noteDur,
					INT16 clocks,		/* Used to compute basic duration and no. of aug. dots */
					INT16 nAugDots		/* Override no. of aug. dots */
					)
{
	INT16 lDur, ndots;
	LINK aNoteL;
	PANOTE aNote;
	
	if (!syncL) { SysBeep(30); return 0; }
	
	Clocks2Dur(clocks, noteDur, &lDur, &ndots);
	if (nAugDots>0) ndots = nAugDots;
	
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice) {
			aNote = GetPANOTE(aNoteL);
			aNote->subType = lDur;
			aNote->ndots = ndots;
			aNote->playDur = SimplePlayDur(aNoteL);
		}
	
	return ndots;
}


short PowerOf2Floor(short num)
{
	short shiftCount = -1;
	
	if (num<0) MayErrMsg("PowerOf2Floor: num<0.");
	
	while (num>0) {
		num >>= 1;
		shiftCount++;
	}
	
	return 1<<shiftCount;
}


/* Look for a Sync with the given absolute (i.e., from the beginning of the score)
timestamp. If we find one, call HiliteInsertNode on it. */

static void HiliteRecSync(Document *doc, long absTimeStamp, LINKTIMEINFO syncs[],
									INT16 nSyncs)
{
	static LINK firstMeas=NILINK;
	INT16 i;
		
	for (i = 0; i<nSyncs; i++)
		if (SyncTYPE(syncs[i].link))
			if (syncs[i].time==absTimeStamp) {
				HiliteInsertNode(doc, syncs[i].link, doc->selStaff, FALSE);
				SleepTicks(HILITE_TICKS);
				HiliteInsertNode(doc, syncs[i].link, doc->selStaff, FALSE);
				return;
			}
}


static void InvertMsgRect(Document *doc)
{
	register WindowPtr w=doc->theWindow;
	Rect messageRect,portRect;

	GetWindowPortBounds(w,&portRect);
	SetRect(&messageRect, portRect.left, 
					portRect.bottom-(SCROLLBAR_WIDTH-1),
					portRect.left+MESSAGEBOX_WIDTH-1,
					portRect.bottom);

	ClipRect(&messageRect);
	InvertRect(&messageRect);

	ClipRect(&doc->viewRect);	
}


static void QuickFlashMsgRect(Document *doc)
{
	InvertMsgRect(doc);
//	SleepTicks(HILITE_TICKS/3);			/* A short delay is all we need */
	InvertMsgRect(doc);
}


/* Return TRUE if and only if the given MMMIDIPacket is a (non-zero-velocity) Note On
for the given channel. */

static Boolean IsOurNoteOn(void *pv,
									INT16 useChan)		/* 0 to 15 */
{
	Byte command, channel;
	
	MMMIDIPacket *pMM;
	MIDIPacket *pCM;
	
	if (useWhichMIDI == MIDIDR_CM) {
		pCM = (MIDIPacket *)pv;
		command = pCM->data[0] & MCOMMANDMASK;
		channel = pCM->data[0] & MCHANNELMASK;
		return (command==MNOTEON && pCM->data[2]!=0 && channel==useChan);
	}
	
	pMM = (MMMIDIPacket *)pv;
	command = pMM->data[0] & MCOMMANDMASK;
	channel = pMM->data[0] & MCHANNELMASK;

	return (command==MNOTEON && pMM->data[2]!=0 && channel==useChan);
}


/* MIDI Pascal equivalent of MIDI Manager's MIDIPoll: add as many complete "real" (non-
zero velocity) Note Ons as possible to <mPacketBuffer>, our global buffer of MIDIPackets.
Designed to be called any time MIDI is active, so it may find any legal sequence of
MIDI bytes starting at any point: two complete and one unfinished Note Ons, just a
velocity byte, etc. Or it may find nothing. Uses <rawBuffer> as a working area.

Return TRUE if all OK or nothing to do, FALSE if there's an error (<rawBuffer> full). */

#ifdef DEBUG_1195
typedef struct dSS {
	long midiByte;
	INT16 r;
} DSS;
#endif

Boolean MPMIDIPoll(INT16);

Boolean MPMIDIPoll(INT16 channel)					/* 0 to 15 */
{
	long				mdata, timeStamp;
	Byte				midiByte;
	INT16				midiWord, len;
	MMMIDIPacket		*p;
	static long		r;
	static Byte		prevChan=255;				/* Guarantee we initialize on 1st call */
#ifdef DEBUG_1195
	#define DEBUGS_SIZE 1000
	static DSS *debugSave=NULL;
	static long dS=0L, d;

	if (!debugSave) {
		long dSEltSize=sizeof(DSS);
		debugSave = (DSS *)NewPtr(DEBUGS_SIZE*dSEltSize);
		if (!GoodNewPtr((Ptr)debugSave)) MayErrMsg("MPMIDIPoll: CAN'T ALLOC debugSave.");
	}
#endif

	/* On the first call and any time <channel> has changed, reinitialize <r>. */
	
	if (channel!=prevChan) r = 0L;
	prevChan = channel;
	MidiIn((int *)&midiWord, &timeStamp);
	
	while (midiWord>=0) {
		MIDIDataToLong(midiWord, timeStamp, &mdata);	/* Convert MIDI Pascal to MacTutor format! */
		midiByte = midiWord;
		if (midiByte<MLOW_SYSREALTIME						/* Skip System Real Time msgs */
		||  midiByte>MHI_SYSREALTIME) {
			rawBuffer[r++] = mdata;
			if (r>=rawBufferLength)
				return FALSE;
			
#ifdef DEBUG_1195
				if (dS<DEBUGS_SIZE) {
					debugSave[dS].midiByte = midiByte;
					debugSave[dS].r = r-1L;
					dS++;
				}
#endif
			/*
			 *	If a new Note On (with non-zero velocity!) has arrived on the correct
			 *	channel, add it to the MMMIDIPacket buffer.
			 */
			if (NoteOnDemon(channel, midiByte)) {
				p = (MMMIDIPacket *)&mPacketBuffer[mRecIndex];
#ifdef DEBUG_1195
				if (r-1L<=0L) {
					MayErrMsg("MPMIDIPoll: CALLING RawMIDI2MMNote WITH 1stParam=%ld. midiByte=0x%x",
								r-1L, midiByte);
					for (d=0; d<dS; d++) {
						DebugPrintf("<%ld %lx %d> ", d, debugSave[d].midiByte, debugSave[d].r);
						if (d%6==5 || d==dS-1) DebugPrintf("\n");
					}
				}
#endif
				RawMIDI2MMNote(r-1L, TRUE, channel, &len, p);
				/* 
				 * Set position for next MMMIDIPacket: for some CPUs, must be an even (word) offset.
				 */
				mRecIndex += ROUND_UP_EVEN(len);
				r = 0L;
			}
		}
		MidiIn((int *)&midiWord, &timeStamp);
	}
	/*
	 * The following delay made the old MacTutor MIDI driver's RxMIDI behave much
	 * better. It's probably not needed anymore, but I don't think it hurts
	 * anything. -DB, 10/95
	 */
	SleepMS(10L);

	return TRUE;
}


#define NONBUF_SIZE 100		/* Size of Note On buffer <nOnBuffer>, in NotePackets */

/* Add all the Note Ons for the given channel we can to <nOnBuffer>; return index
of the first unused slot in <nOnBuffer>. Intended for step recording, so pays
attention only to "real" Note Ons (those with non-zero-velocity).

We also use an intermediate MMMIDIPacket (MIDI Manager-format) buffer <mPacketBuffer>
and--if we're using the built-in MIDI driver--the "raw" MIDI data buffer <rawBuffer>.
If any of the buffers overflows, we give an error message. NB: <nOnBuffer> is small,
but I've never heard of it overflowing except in test versions when we're printing
debug information. */

// ••
// •• Need to rewrite to handle the fact that we are using MIDIPackets, not MMMIDIPackets.
// ••

static long cmNowTime;

INT16 GetAllNoteOns(NotePacket nOnBuffer[],
						INT16 nOnBufPos,			/* On entry, first unused slot in <nOnBuffer> */
						INT16 channel, long kk)				/* 0 to 15 */
{
	long				loc;
	Byte				command;
	INT16				i, n;
//	MMMIDIPacket	*p;
	MIDIPacket		*pCM;
	MIDITimeStamp	cmTimeStamp;

	mRecIndex = 0L;
	
	/* Copy non-zero-velocity Note Ons for our channel from MMMIDIPacket buffer to nOnBuffer. */
	
	loc = 0L; n = nOnBufPos;

	if (useWhichMIDI == MIDIDR_CM) {
		while (pCM = GetCMMIDIPacket()) {
			command = pCM->data[0] & MCOMMANDMASK;
			if (IsOurNoteOn(pCM, channel)) {
				if (n>=NONBUF_SIZE) {
					GetIndCString(strBuf, MIDIERRS_STRS, 9);		/* "Out of memory. Note On Buf exceeded." */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					return NONBUF_SIZE;
				}
				if (srFirstNote) {
					srFirstNote = FALSE;
					cmTimeStamp = 0;
					srFirstTimeStamp = pCM->timeStamp;
				}
				else {
					cmTimeStamp = pCM->timeStamp - srFirstTimeStamp;
				}
				nOnBuffer[n].tStamp = CMTimeStampToMillis(cmTimeStamp);
				for (i = 0; i<pCM->length; i++)
					nOnBuffer[n].data[i] = pCM->data[i];
					
				DebugPrintf("kk %ld nOnBufPos %ld n %ld tstamp %ld\n", kk, nOnBufPos, n, nOnBuffer[n].tStamp);
				
				n++;
			}
		}
	}

	return n;
}

long CMGetHostTimeMillis();

/* Look for a gap between consecutive Note Ons of at least the score's deflamTime. */

INT16 FindDeflamGap(Document *doc, NotePacket nOnBuffer[], INT16 nOnBufLen, long nowTime, int jj)
{
	INT16 i;
	int nChord = 0;
	
	for (i = 1; i<nOnBufLen; i++) {
		long noteOnGap = nOnBuffer[i].tStamp-nOnBuffer[i-1].tStamp;
		if (noteOnGap >= doc->deflamTime) {
			return i;
		}
	}
	
	if (useWhichMIDI == MIDIDR_CM) {
		long lastTimeStamp = nOnBuffer[nOnBufLen-1].tStamp;
		
		long startBufferTime = CMTimeStampToMillis(srFirstTimeStamp);
		long nowTime = CMGetHostTimeMillis() - startBufferTime;
		long nowGap = nowTime-lastTimeStamp;
		
		if (nowGap>doc->deflamTime) 
			nChord = nOnBufLen;
		else 
			nChord = 0;
		
		DebugPrintf("jj %ld lastTimeStamp %ld startBufferTime %ld nowTime %ld nowGap %ld nChord %ld \n", 
						 jj, lastTimeStamp, startBufferTime, nowTime, nowGap, nChord);
		return nChord;
	}
		
	if (nowTime-nOnBuffer[nOnBufLen-1].tStamp>doc->deflamTime) return nOnBufLen;
	else return 0;
}


/* -------------------------------------------------------- SRKeyDown, StepRecord -- */

#define FEEDBACK_TAB_SIZE 600		/* Max. Syncs and Measures in table for merge feedback */
#define CLOCK_CHAR ' '				/* "Clock" (increase duration) */
#define AUGDOT_CHAR '.'				/* Add an augmentation dot */
#define PAUSE_CHAR CH_ESC			/* Pause recording */

static Boolean paused;
static long timeRecorded;
static WindowPtr wDoc;

/* Handle a keyDown event while Step Recording. Special case: if *pNoteDur==NOMATCH,
initialize (and ignore <theEvent>). If all is well, return TRUE; if there's a
problem (reached the recording time limit), return FALSE. */

Boolean SRKeyDown(
	Document *doc,
	EventRecord theEvent,
	LINK syncL,
	INT16 voice,
	short *pSymIndex,
	short *pNoteDur,				/* Note duration code (LDur) */
	short *pClocks,				/* Multiplier for note duration */
	short *pNAugDots)
{
	static INT16 lastDurChar=0;
	short symIndex, noteDur, clocks, nAugDots, addClocks; 
	char theChar;
	INT16 intChar, sym, ch, key;
	Boolean okay=TRUE;
	Rect portRect;

	if (*pNoteDur==NOMATCH) {
		/*
		 * If the current palette character is a note or rest, use its duration; otherwise,
		 * use the character last used in SRKeyDown, or if this is the first call to
		 * SRKeyDown, use quarter-note duration.
		 */
		*pNoteDur = Char2Dur(palChar);
		if (*pNoteDur==NOMATCH || *pNoteDur==UNKNOWN_L_DUR) {
			if (lastDurChar==0) lastDurChar = Dur2Char(QTR_L_DUR);
			DoToolKeyDown(lastDurChar, -1, -1);
			*pNoteDur = Char2Dur(palChar);
		}
		*pSymIndex = GetSymTableIndex(palChar);
		lastDurChar = palChar;
		
		*pClocks = 1;
		*pNAugDots = 0;
		
		return TRUE;
	}

	symIndex = *pSymIndex;
	noteDur = *pNoteDur;
	clocks = *pClocks;
	nAugDots = *pNAugDots;
	
	/*
	 *	If char. typed is key equivalent of a note/rest duration, switch to that
	 *	duration. If char. is "clock", lengthen the current note/rest/chord (if
	 * there is one!) by the current duration.
	 */
	if ((theEvent.modifiers & !btnState)==0) {
		theChar = (char)theEvent.message & charCodeMask;
		intChar = (INT16)theChar;
		if (TranslatePalChar(&intChar, 0, FALSE))
			theChar = (unsigned char)intChar;
		sym = GetSymTableIndex(theChar);
		
		if (symtable[sym].objtype==SYNCtype) {
			ch = theEvent.message & charCodeMask;
			key = (theEvent.message>>8) & charCodeMask;
			DoToolKeyDown(ch, key, theEvent.modifiers);
			noteDur = Char2Dur(palChar);
			symIndex = GetSymTableIndex(palChar);
			lastDurChar = palChar;
		}
#ifdef RespAndRfmtRaw_NO_PROBLEM
		else if (symtable[sym].objtype==MEASUREtype
						|| symtable[sym].subtype==BAR_SINGLE) {
			ch = theEvent.message & charCodeMask;
			key = (theEvent.message>>8) & charCodeMask;
			DoToolKeyDown(ch, key, theEvent.modifiers);
			symIndex = GetSymTableIndex(palChar);
		}
#endif
		else if ((theChar==CLOCK_CHAR || theChar==AUGDOT_CHAR) && syncL!=NILINK) {
			if (theChar==CLOCK_CHAR) {
				/*
				 * The user hit the clock key. Increase the number of clocks.
				 * Also set <nAugDots> to reflect as closely as possible the
				 * current number of clocks.
				 */ 
				clocks++;
				nAugDots = MRSetNRCDur(doc, syncL, voice, symIndex, noteDur,
								clocks, 0);
				if (nAugDots>2) nAugDots = 2;
			}
			else {
				/*
				 * The user hit the augmentation-dot key. Increase the count of
				 * augmentation dots (modulo 3); discard all current aug. dots;
				 * then add in the current number of dots. Also set <clocks>
				 * to reflect as closely as possible the current number of
				 * aug. dots.
				 */ 
				clocks = PowerOf2Floor(clocks);
				nAugDots++;
				if (nAugDots>2) nAugDots = 0;
				MRSetNRCDur(doc, syncL, voice, symIndex, noteDur, clocks, nAugDots);
				if (nAugDots>0) {
					addClocks = clocks/2;
					if (nAugDots>1) addClocks += clocks/4;
					clocks += addClocks;
				}
			}

			GetWindowPortBounds(wDoc,&portRect);
			ShowNRCInBox(doc, syncL, voice, doc->selStaff, &portRect, ShowREPLACE);
			/*
			 * Incrementing <timeRecorded> unconditionally here will get feedback
			 * out of sync if user ends up with a no. of clocks that's not a CMN
			 * duration, e.g., 5. ??This should be fixed.
			 */
			timeRecorded += Code2LDur(noteDur, 0);
			QuickFlashMsgRect(doc);							/* Flash to reassure user */
			if (timeRecorded>MAX_SAFE_MEASDUR) {
				GetIndCString(strBuf, MIDIERRS_STRS, 4);		/* "You've reached the time limit" */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				okay = FALSE;
			}
		}
		else if (theChar==PAUSE_CHAR) {
			paused = !paused;
			recordingNow = !paused;							/* turn off NightOMSReadHook processing */
			RecordMessage(doc, paused);
			if (!paused) InvertMsgRect(doc);				/* Confirm recording is in progress */
		}
	}
	
	*pNoteDur = noteDur;
	*pClocks = clocks;
	*pSymIndex = symIndex;
	*pNAugDots = nAugDots;
	
	return okay;
}

static int TranslateBuffer(int nChord, int nOnBufLen, NotePacket *nOnBuffer)
{
	nOnBufLen -= nChord;
	for (int i = 0; i<nOnBufLen; i++)
		nOnBuffer[i] = nOnBuffer[i+nChord];
		
	return nOnBufLen;
}

static void SlideBuffer(NotePacket *nOnBuffer, int nOnBufLen, long time2Xlate)
{
	for (int i = 0; i<nOnBufLen; i++)
		nOnBuffer[i].tStamp += time2Xlate;
}


/* Record data from MIDI, and generate Nightingale notes with durations specified by
user on the Mac keyboard as they play, giving visual feedback for the notes. Also
allow the user to specify they want rests instead of notes, and to switch back and
forth. Depending on <merge>, either add new Syncs containing the notes to the object
list at the insertion point, or (based on duration) merge the notes into the object
list. Returns TRUE if we add at least one note or rest to the object list, else FALSE
(due either to an error condition, or just no note data on the right channel).

StepRecord does not format the material recorded so it can be drawn; to do so, after
calling StepRecord, call RespAndRfmtRaw or RespaceBars. */

Boolean StepRecord(
				Document *doc,
				Boolean merge				/* TRUE=merge with existing stuff, else add new Syncs */
				)
{
	long				ignoredRTM, nowTime,
						insertTime, diffTime;
	Boolean			done, gotSomething;
	INT16				useChan,
						nMergeObjs, voice;
	short				symIndex, noteDur, clocks, nAugDots; 
	LINK				insSyncL, oldSelStart, firstSync, syncL=NILINK, lastL;
	LINK				partL;
	OMSUniqueID		thruDevice, inputDevice;
	MIDIUniqueID	cmThruDevice, cmInputDevice;
	char	      	fmtStr[256];
	INT16				thruChannel;
	EventRecord		theEvent;
	LINKTIMEINFO	*mergeObjs=NULL, *newSyncs=NULL;
	NotePacket		nOnBuffer[NONBUF_SIZE];
	INT16				nOnBufLen, nChord, i, maxNewSyncs, nNewSyncs;
	INT16				syncsSoFar=0;
	Rect				portRect;
	
	// MAS definitions moved here, before first jump to 'Finished'
	int k = 0;
	long midiNow = 0;
	bool first = TRUE;
	
	if (doc->selStartL!=doc->selEndL) {
		GetIndCString(strBuf, MIDIERRS_STRS, 7);		/* "Can't Step Record w/o insertion pt" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	wDoc = (WindowPtr)doc;
	printDebug = CapsLockKeyDown() && OptionKeyDown();
	SetRecordFlats(doc);
	gotSomething = FALSE;

	WaitCursor();
	if (!AllocRawBuffer()) {
		NoMoreMemory();
		goto Finished;
	}

	if (useWhichMIDI == MIDIDR_CM) {
		if (!ResetMIDIPacketList()) {
			NoMoreMemory();
			goto Finished;
		}
		srFirstNote = TRUE;
		srFirstTimeStamp = 0;
	}
	
	//doc->deflamTime = 200;
		
	paused = FALSE;
	RecordMessage(doc, paused);
	
	if (merge) {
		/*
		 *	Find the time at the insertion pt and make a table of Syncs and Measures
		 * starting there. NB: the time at the insertion pt may not be where the user
		 * would expect new notes to start, e.g., two quarters on one staff, eighth on
		 * another; click after the eighth and Step Record: the insertion pt is a quarter
		 * into the measure, but the user might expect the recorded notes to start an
		 * eighth in. But it's easier to do this way.
		 */
		insSyncL = SSearch(doc->selStartL, SYNCtype, GO_RIGHT);
		insertTime = (insSyncL? SyncAbsTime(insSyncL) : BIGNUM);
		if (insSyncL) {
			mergeObjs = (LINKTIMEINFO *)NewPtr(FEEDBACK_TAB_SIZE*sizeof(LINKTIMEINFO));
			if (!GoodNewPtr((Ptr)mergeObjs)) {
				OutOfMemory(FEEDBACK_TAB_SIZE*sizeof(LINKTIMEINFO));
				goto Finished;
			}
			nMergeObjs = FillMergeBuffer(insSyncL, mergeObjs, FEEDBACK_TAB_SIZE, FALSE);
		}
		else
			merge = FALSE;													/* Nothing to merge with */
	}
		
	oldSelStart = doc->selStartL;
	voice = USEVOICE(doc, doc->selStaff);
	useChan = doc->channel;

	if (useWhichMIDI == MIDIDR_OMS) {
		partL = GetSelPart(doc);
		if (!OMSPrepareRecord(doc, partL, &useChan, &thruDevice, &inputDevice, &thruChannel))
			return FALSE;
	}
	if (useWhichMIDI == MIDIDR_CM) {
		partL = GetSelPart(doc);
		if (doc->cmInputDevice == kInvalidMIDIUniqueID)
			doc->cmInputDevice = gSelectedInputDevice;
		if (!CMPrepareRecord(doc, partL, &useChan, &cmThruDevice, &cmInputDevice, &thruChannel)) {
			return FALSE;
		}
	}
	else if (useWhichMIDI == MIDIDR_FMS) {
		/* NB: we aren't using inputDevice and thruChannel */
		partL = GetSelPart(doc);
		if (!FMSPrepareRecord(doc, partL, &useChan, &thruDevice, &inputDevice, &thruChannel))
			return FALSE;
	}
	
	split = FALSE;															/* No split pt in Step Record */

	noteDur = NOMATCH;													/* To initialize SRKeyDown */
	theEvent.what = nullEvent;											/* So compiler doesn't think it's uninitialized */
	(void)SRKeyDown(doc, theEvent, syncL, voice, &symIndex, &noteDur,
					&clocks, &nAugDots);
	
	SetCursor(*currentCursor);

 	ignoredRTM = 0L;
	timeRecorded = 0L;
	nOnBufLen = 0;
	firstSync = NILINK;
	
/* *** BEGIN SEMI-REALTIME, LOW-LEVEL CODE. Handle with care! *** */

	switch (useWhichMIDI) {
		case MIDIDR_FMS:
			FMSInitTimer();
			InitFMSBuffer();
			break;
		case MIDIDR_OMS:
			OMSInitTimer();
			InitOMSBuffer();
			break;
		case MIDIDR_CM:
			CMInitTimer();
			// •• InitCMBuffer();
			break;
		case MIDIDR_BI:
			MayInitBIMIDI(BIMIDI_SMALLBUFSIZE, BIMIDI_SMALLBUFSIZE);	/* initialize serial chip for MIDI port */
			InitBIMIDITimer();													/* initialize millisecond timer */
			break;
		default:
			break;
	}

	StartMIDITime();
	
	/* Flush MIDI Manager's buffer, then set the recording-in-progress flag */
	if (useWhichMIDI==MIDIDR_MM) MIDIPoll(inputMMRefNum, midiGetEverything);
	recordingNow = TRUE;

	InvertMsgRect(doc);												/* Confirm recording is in progress */
	done = FALSE;
	while (!done)
	{		
		/* Look for and handle a relevant operating system event. */
		if (k % 20 == 0) {
			GetNextEvent(keyDownMask, &theEvent);
			switch (theEvent.what) {
				case mouseDown:
					/* ??We should handle click in palette here, though it's not very practical.
						Cf. DoContent, case PALETTEKIND. */
					break;
				case keyDown:
					if (!SRKeyDown(doc, theEvent, syncL, voice, &symIndex, &noteDur,
						&clocks, &nAugDots)) done = TRUE;
					break;
				default:
					break;
			}
		}
		k++;
			
		/*
		 * Add as many non-zero-velocity Note Ons for our channel as we can to nOnBuffer.
		 * If we have any (both pre-existing and new), look for a long enough gap,
		 *	either within or after the notes, to demarcate the end of a chord. If we don't
		 * find such a gap, wait until there's been a long enough gap after the last
		 * note, add any notes that came in while we were waiting, and look for a gap
		 * again. If we have a gap now, build a Sync for the chord preceding it.
		 */
		if (first) {
			midiNow = GetMIDITime(0L); first = FALSE;
		}
		nOnBufLen = GetAllNoteOns(nOnBuffer, nOnBufLen, useChan-1, 1);
		if (nOnBufLen>0) {
			if (printDebug) PrintBuffer(doc, mRecIndex);
			if (printDebug) PrintNBuffer(doc, nOnBuffer, nOnBufLen);
			nowTime = GetMIDITime(0L);
			nChord = FindDeflamGap(doc, nOnBuffer, nOnBufLen, nowTime-midiNow, 10);
#if 1
			if (nChord<=0) {
				long tStamp = nOnBuffer[nOnBufLen-1].tStamp + midiNow;
				//long endTime = tStamp+doc->deflamTime;
				long endTime = nowTime+doc->deflamTime + 100;
				//DebugPrintf("timenow %ld endTime %ld\n", nowTime, endTime);
				while (GetMIDITime(0L)<endTime)
					//;
				nOnBufLen = GetAllNoteOns(nOnBuffer, nOnBufLen, useChan-1, 2);
				long nowTime1 = GetMIDITime(0L);
				nChord = FindDeflamGap(doc, nOnBuffer, nOnBufLen, nowTime1-midiNow, 20);
				if (nChord > 0)
					DebugPrintf("midiNow %ld tStamp %ld nowTime %ld endTime %ld nOnBufLen %ld \n", midiNow, tStamp, nowTime, endTime, nOnBufLen);
					
			}
#endif
#ifdef NOTYET
			if (printDebug) PrintNBuffer(nOnBuffer, nOnBufLen);
#endif

			if (nChord>0) {
				int k = 0;
				
				DebugPrintf("midiNow %ld nowTime %ld - ",midiNow, nowTime);
				for ( ; k<nOnBufLen; k++) {
					DebugPrintf("k %ld tstamp %ld   ",k,nOnBuffer[k].tStamp);
				}
				DebugPrintf("nOnBufLen %ld nChord %ld\n", nOnBufLen, nChord);
				
				if (!paused) {
					syncL = StepMIDI2Night(doc, nOnBuffer, nChord, symIndex, noteDur, 1);
					if (syncL) {
						/* We successfully built a Sync of the <nChord> notes. */
						gotSomething = TRUE;
						GetWindowPortBounds(wDoc,&portRect);
						ShowNRCInBox(doc, syncL, voice, doc->selStaff, &portRect,
												(firstSync? ShowADD : ShowINIT));
		
						if (merge)
							HiliteRecSync(doc, insertTime+timeRecorded, mergeObjs, nMergeObjs);
						timeRecorded += Code2LDur(noteDur, 0);
						if (timeRecorded>MAX_SAFE_MEASDUR) {
							GetIndCString(strBuf, MIDIERRS_STRS, 4);	/* "You've reached the time limit" */
							CParamText(strBuf, "", "", "");
							StopInform(GENERIC_ALRT);
							done = TRUE;
						}

						syncsSoFar++;
						if (syncsSoFar>=MAX_MEASNODES-2) {
							GetIndCString(fmtStr, MIDIERRS_STRS, 10);	/* "Reached the limit of %d new notes/chords..." */
							sprintf(strBuf, fmtStr, MAX_MEASNODES-2);
							CParamText(strBuf, "", "", "");
							StopInform(GENERIC_ALRT);
							done = TRUE;
						}
						
						QuickFlashMsgRect(doc);							/* Flash to reassure user */
						if (!firstSync) firstSync = syncL;
					}
				}

				nOnBufLen -= nChord;
				for (i = 0; i<nOnBufLen; i++)
					nOnBuffer[i] = nOnBuffer[i+nChord];
				clocks = 1;
				nAugDots = 0;
			}
		}
	
		/* If mouse button is down, we're done. */
		
		if (Button()) done = TRUE;
	}

#ifdef NOTYET
	if (printDebug) PrintBuffer(doc, mRecIndex);
#endif

	/*
	 * If user quit recording less than <deflamTime> after the last note, there may
	 * still be notes in the buffer. If so, handle them here.
	 */
	if (!paused && timeRecorded<=MAX_SAFE_MEASDUR) {
		syncL = StepMIDI2Night(doc, nOnBuffer, nOnBufLen, symIndex, noteDur, clocks);
		if (syncL) {
			gotSomething = TRUE;
			if (!firstSync) firstSync = syncL;
		}
	}

	recordingNow = FALSE;
	StopMIDITime();
	
	if (useWhichMIDI == MIDIDR_OMS)
		CloseOMSInput(inputDevice);
	
/* *** END SEMI-REALTIME, LOW-LEVEL CODE. *** */

	if (ignoredRTM && WARN_IGNORED) {
		GetIndCString(fmtStr, MIDIERRS_STRS, 5);		/* "Ignored %ld Real Time messages" */
		sprintf(strBuf, fmtStr, ignoredRTM);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	
	if (gotSomething) {
		AvoidUnisonsInRange(doc, firstSync, oldSelStart, doc->selStaff);
		if (merge) {
		/*
		 * The following call to FixTimeStamps is necessary so MRMerge knows what to
		 * merge, but it'll screw up timestamps for the rest of endL's measure. But
		 *	that doesn't hurt anything since timestamps must be fixed after merging
		 *	anyway. NB: we have to further adjust timestamps after calling FixTimeStamps
		 *	to avoid a possible discrepancy between the time at the insertion pt and the
		 *	time of the voice recorded into: see above.
		 */
			FixTimeStamps(doc, firstSync, LeftLINK(oldSelStart));
			diffTime = insertTime-SyncAbsTime(firstSync);
			if (diffTime!=0) MoveTimeInMeasure(firstSync, oldSelStart, diffTime);

			maxNewSyncs = CountObjects(firstSync, oldSelStart, SYNCtype);
			newSyncs = (LINKTIMEINFO *)NewPtr(maxNewSyncs*sizeof(LINKTIMEINFO));
			if (!GoodNewPtr((Ptr)newSyncs)) {
				OutOfMemory(maxNewSyncs*sizeof(LINKTIMEINFO));
				goto Finished;
			}
			nNewSyncs = FillMergeBuffer(firstSync, newSyncs, maxNewSyncs, TRUE);

			MRMerge(doc, voice, newSyncs, nNewSyncs, mergeObjs, nMergeObjs, &lastL);
		}
		else {
			doc->selStartL = firstSync;							/* Set sel range to everything just recorded */
			doc->selEndL = oldSelStart;
		}
		if (config.delRedundantAccs) DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);
		if (config.ignoreRecVel) SetVelFromContext(doc, TRUE);

		FixTimeStamps(doc, doc->selStartL, doc->selEndL);
	
		WaitCursor();
		UpdateMeasNums(doc, NILINK);
		UpdateVoiceTable(doc, TRUE);
	}
	DrawMessageBox(doc, TRUE);									/* Also sets ClipRect to viewRect */
	FlushEvents(mDownMask, 0);									/* Discard mouse click that ended recording */

Finished:
	FMSSetMIDIThruDeviceFromConfig();

	if (rawBuffer) DisposePtr((Ptr)rawBuffer);
	if (mergeObjs) DisposePtr((Ptr)mergeObjs);
	
	return gotSomething;
}


/* ------------------------------------------------------------------- RTMRecord -- */
/* Record data from MIDI and generate Nightingale notes of unknown duration.
Returns TRUE if it adds at least one note to the object list, else FALSE
(due to the user cancelling the record or the transcribe, an error condition, or
simply no note on the right channel). Intended for use by the Record Merge command.

One problem with this version: only Note On and Off are recognized by
MIDI2Night, but all other non-System-Real-Time status bytes--e.g., pitch bend,
controller changes, System Exclusive--get into the buffer, taking up valuable
space; pitch bend and other continuous controllers are particularly piggish. */

Boolean RTMRecord(Document *doc)
{
	static Boolean metronome=TRUE;
	long timeChange, tLeadInOffset;
	INT16 partn, useChan;
	
	timeChange = -1L;

	WaitCursor();
	if (!AllocRawBuffer()) {
		NoMoreMemory();
		goto Finished;
	}

	if (useWhichMIDI == MIDIDR_CM) {
		if (!ResetMIDIPacketList()) {
			NoMoreMemory();
			goto Finished;
		}
	}
		
	SetRecordFlats(doc);
		
	partn = Staff2Part(doc, doc->selStaff);
	useChan = doc->channel;
	if (!RecordDialog(doc, doc->selStaff, partn, useChan, TRUE, TRUE, &otherStaff,
							&metronome, &tLeadInOffset))
		goto Finished;
	
	WaitCursor();
	timeChange = MIDI2Night(doc, useChan-1, mRecIndex, tLeadInOffset);	/* Build Nightingale object list */

	if (timeChange>=0L) {
		UpdateVoiceTable(doc, TRUE);	/* ??DO THIS LATER? OR NOT AT ALL? */
	}

Finished:
	DisposePtr((Ptr)rawBuffer);
	return (timeChange>=0L);
}


/* ----------------------------------------------------------  Debugging Routines -- */

static void PrintNote(MNOTEPTR pMNote, Boolean newLine)
{
#ifndef PUBLIC_VERSION
	DebugPrintf("note=%3d chan=%2d ",pMNote->noteNumber,pMNote->channel);
	DebugPrintf("vel=%3d/%3d ",pMNote->onVelocity,pMNote->offVelocity);
	DebugPrintf("start=%5ld,dur=%4ld ms",pMNote->startTime, pMNote->duration);
	if (newLine) DebugPrintf("\n");
#endif
}


static void PrintNBuffer(Document *doc, NotePacket nOnBuffer[], INT16 nOnBufLen)
{
#ifndef PUBLIC_VERSION
	INT16 n, i;
	
	DebugPrintf("PNB:(%s) deflamTime=%d minRecVel=%d minRecDur=%d nOnBufLen=%d:\n",
					(useWhichMIDI==MIDIDR_MM? "MIDIMgr" : "built-in MIDI"),
					doc->deflamTime, config.minRecVelocity, config.minRecDuration, nOnBufLen);
	for (n = 0; n<nOnBufLen; n++) {
		DebugPrintf("  %x TS=%ld data=", nOnBuffer[n].flags, nOnBuffer[n].tStamp);
		for (i = 0; i<3; i++)
			DebugPrintf("%x ", nOnBuffer[n].data[i]);
		DebugPrintf("\n");
	}
#endif
}


#define NPERGROUP 6
#define NPERLINE (3*NPERGROUP)

static void PrintBuffer(Document *doc, long mBufLen)
{
#ifndef PUBLIC_VERSION
	MMMIDIPacket *p;
	long loc; INT16 i, len;
	
	DebugPrintf("PB:(%s) deflamTime=%d minRecVel=%d minRecDur=%d mBufLen=%ld:\n",
					(useWhichMIDI==MIDIDR_MM? "MIDIMgr" : "built-in MIDI"),
					doc->deflamTime, config.minRecVelocity, config.minRecDuration, mBufLen);
	loc = 0L;
	while (loc<mBufLen) {
		p = (MMMIDIPacket *)&mPacketBuffer[loc];
		len = p->len;
		DebugPrintf("  %x len=%d TS=%ld data=", p->flags, p->len, p->tStamp);
		for (i = 0; i<p->len-MM_HDR_SIZE; i++)
			DebugPrintf("%x ", p->data[i]);
		DebugPrintf("\n");
		/* 
		 * Increment position to the next MMMIDIPacket, making sure we move to an EVEN
		 * (word) offset.
		 */
		loc += ROUND_UP_EVEN(len);
	}
#endif
}