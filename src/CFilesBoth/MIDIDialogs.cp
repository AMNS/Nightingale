/******************************************************************************************
*	FILE:	MIDIDialogs.c
*	PROJ:	Nightingale
*	DESC:	General MIDI dialog routines (no relation to the "General MIDI" standard!)
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ------------------------------------------------------------------- MIDIPrefsDialog -- */
/*	Setup MIDI channel assignment, tempo, port, etc. NB: Does not handle SCCA for MIDI
Thru! */


enum				/* Dialog item numbers */
{
	CHANNEL=4,
	MINDUR_DI=6,
	MINVELOCITY_DI=9,
	DEFLAM_TIME_DI=11,
	SHARPS_DI=14,
	FLATS_DI,
	DELREDACC_DI,

	PART_SETTINGS=17,
	SEND_PATCHES,
	NO_PART_SETTINGS,
	MASTERVELOCITY_DI=21,
	TURNPAGES_DI=22,
	FEEDBACK,
	DIVIDER1_DI=25,
	DIVIDER2_DI=27,
	USE_MODIFIER_EFFECTS_DI=28,
	CHANNEL_LABEL=29,
	INPUT_DEVICE_MENU=30
};

#include "CoreMIDIDefs.h"

/* ----------------------------------------------------------------------------- Input -- */

static Rect					inputDeviceMenuBox;
static MenuHandle			cmInputMenuH;
static UserPopUp			cmInputPopup;
static MIDIUniqueIDVector	*cmVecDevices;

static Rect divRect1, divRect2;

static short kCMPopupID = 300;

/* ---------------------------------------------------------------------------- Output -- */

static MenuHandle cmMetroMenuH;
static UserPopUp cmOutputPopup;

static MIDIUniqueID GetCMOutputDeviceID(void);

/* ------------------------------------------------------------------- MIDIPrefsDialog -- */

static Boolean ValidCMInputIndex(short idx);
static Boolean ValidCMInputIndex(short idx)
{
	return (idx>=0 && (unsigned short)idx<cmVecDevices->size());
}

static MIDIUniqueID GetCMInputDeviceID();
static MIDIUniqueID GetCMInputDeviceID()
{
	short choice = cmInputPopup.currentChoice-1;
	
	if (ValidCMInputIndex(choice)) {
		return cmVecDevices->at(choice);
	}
	return kInvalidMIDIUniqueID;
}

/* FIXME: GetCMInputDeviceIndex is in C++, but Nightingale is supposed to be, and is
almost completely, C99, not C++! Maybe we have no choice but to use C++ to talk to Core
MIDI? That seems unlikely. As of April 2022, the construct "MIDIUniqueIDVector::iterator"
appears a total of three times in Nightingale code. */

static short GetCMInputDeviceIndex(MIDIUniqueID dev);
static short GetCMInputDeviceIndex(MIDIUniqueID dev)
{
	long j = 0;
	
	MIDIUniqueIDVector::iterator i = cmVecDevices->begin();
	
	for ( ; i!= cmVecDevices->end(); i++, j++) {
		if (dev == (*i))
			return j;
	}
	
	return -1;
}

static MenuHandle CreateCMInputMenu(Document *doc, DialogPtr dlog, UserPopUp *p, Rect * /* box */);
static MenuHandle CreateCMInputMenu(Document *doc, DialogPtr dlog, UserPopUp *p, Rect * /* box */)
{
	Boolean popupOk = InitPopUp(dlog, p,
									INPUT_DEVICE_MENU,	/* Dialog item to set p->box from */
									0,					/* Dialog item to set p->prompt from, or 0=set it empty */
									kCMPopupID,			/* CoreMidi popup ID */
									0					/* Initial choice, or 0=none */
									);
	if (!popupOk || p->menu==NULL) return NULL;
		
	cmVecDevices = new MIDIUniqueIDVector();
	long numItems = FillCMSourcePopup(p->menu, cmVecDevices);
	
	short currChoice = GetCMInputDeviceIndex(doc->cmInputDevice);

	if (ValidCMInputIndex(currChoice)) {
		ChangePopUpChoice(&cmInputPopup, currChoice + 1);
	}
	
	return p->menu;
}

static void DrawDividers(DialogPtr);
static void DrawDividers(DialogPtr)
{
	MoveTo(divRect1.left, divRect1.top);
	LineTo(divRect1.right, divRect1.top);
	MoveTo(divRect2.left, divRect2.top);
	LineTo(divRect2.right, divRect2.top);
}

/* This filter outlines the OK button, draws divider lines, and performs standard key
and command-key filtering. */

static pascal Boolean MIDIFilter(DialogPtr, EventRecord *, short *);
static pascal Boolean MIDIFilter(DialogPtr theDialog, EventRecord *theEvt, short *item)
{
	GrafPtr	oldPort;
	short	type;
	Handle	hndl;
	Rect	box;
	Point	mouseLoc;
	short	ans = 0;

	switch (theEvt->what) {
		case updateEvt:
			GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
			BeginUpdate(GetDialogWindow(theDialog));
			DrawDividers(theDialog);
			UpdateDialogVisRgn(theDialog);
			FrameDefault(theDialog, OK, True);
			if (useWhichMIDI == MIDIDR_CM && cmInputMenuH != NULL) {
				DrawPopUp(&cmInputPopup);
			}
			EndUpdate(GetDialogWindow(theDialog));
			SetPort(oldPort);
			*item = 0;
			return True;
			break;
		case mouseDown:
			*item = 0;
			mouseLoc = theEvt->where;
			GlobalToLocal(&mouseLoc);
			
			/* If user hit the OK or Cancel button, bypass the rest of the control stuff below. */
			
			GetDialogItem (theDialog, OK, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = OK; break;};
			GetDialogItem (theDialog, Cancel, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = Cancel; break;};

			if (useWhichMIDI == MIDIDR_CM && cmInputMenuH != NULL) {
				GetDialogItem (theDialog, INPUT_DEVICE_MENU, &type, &hndl, &box);
				if (PtInRect(mouseLoc, &box))	{
					ans = DoUserPopUp(&cmInputPopup);
					if (ans) *item = INPUT_DEVICE_MENU;
				}
			}
			break;

		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvt, item, False)) return True;
			break;
		default:
			break;
	}
	return False;
}


#define GEN_ACCEPT(field)	if (newval!=(field)) \
						{ (field) = newval;  docDirty = True; }

void MIDIPrefsDialog(Document *doc)
{
	DialogPtr dlog;
	GrafPtr oldPort;
	short dialogOver;
	short ditem, newval, temp, anInt, groupFlats, groupPartSets;
	Handle aHdl, patchHdl, turnHdl;
	Boolean docDirty = False;
	char fmtStr[256];
	short scratch;
	ModalFilterUPP filterUPP;
	
	ArrowCursor();

/* --- 1. Create the dialog and initialize its contents. --- */

	filterUPP = NewModalFilterUPP(MIDIFilter);
	if (filterUPP == NULL) {
		MissingDialog(MIDISETUP_DLOG);
		return;
	}
	GetPort(&oldPort);

	/* We use the same dialog resource for Core MIDI (and formerly OMS and FreeMIDI). */
	
	if (useWhichMIDI == MIDIDR_CM) {
		dlog = GetNewDialog(CM_MIDISETUP_DLOG, NULL, BRING_TO_FRONT);
		if (dlog == NULL) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(CM_MIDISETUP_DLOG);
			return;
		}
	}
	else {
		dlog = GetNewDialog(MIDISETUP_DLOG, NULL, BRING_TO_FRONT);
		if (dlog == NULL) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(MIDISETUP_DLOG);
			return;
		}
	}
	SetPort(GetDialogWindowPort(dlog));
	
	cmVecDevices = NULL;

	GetDialogItem(dlog, INPUT_DEVICE_MENU, &scratch, &aHdl, &inputDeviceMenuBox);
	if (useWhichMIDI == MIDIDR_CM) {
		if (doc->cmInputDevice == kInvalidMIDIUniqueID) {
			doc->cmInputDevice = gSelectedInputDevice;
			docDirty = True;
		}
		cmInputMenuH = CreateCMInputMenu(doc, dlog, &cmInputPopup, &inputDeviceMenuBox);
	}

	/* Get the divider lines, defined by some user items, and get them out of the way so
	   they don't hide any items underneath. */
	   
	GetDialogItem(dlog,DIVIDER1_DI,&anInt,&aHdl,&divRect1);
	GetDialogItem(dlog,DIVIDER2_DI,&anInt,&aHdl,&divRect2);
	
	HideDialogItem(dlog,DIVIDER1_DI);
	HideDialogItem(dlog,DIVIDER2_DI);

	groupFlats = (doc->recordFlats? FLATS_DI : SHARPS_DI);		/* Set up radio button group */
	PutDlgChkRadio(dlog, groupFlats, True);

	PutDlgWord(dlog, CHANNEL, doc->channel, False);

	PutDlgWord(dlog, MASTERVELOCITY_DI, doc->velocity, False);
	PutDlgChkRadio(dlog, FEEDBACK, doc->noteInsFeedback);
	
	groupPartSets = (doc->polyTimbral? PART_SETTINGS : NO_PART_SETTINGS);	/* Set up radio button group */
	PutDlgChkRadio(dlog, groupPartSets, True);

	patchHdl = PutDlgChkRadio(dlog, SEND_PATCHES, !doc->dontSendPatches);
	if (!doc->polyTimbral) HiliteControl((ControlHandle)patchHdl, CTL_INACTIVE);
	PutDlgWord(dlog, DEFLAM_TIME_DI, doc->deflamTime, False);	
	PutDlgWord(dlog, MINDUR_DI, config.minRecDuration, False);	
	PutDlgWord(dlog, MINVELOCITY_DI, config.minRecVelocity, False);	
	PutDlgChkRadio(dlog, DELREDACC_DI, config.delRedundantAccs);
	turnHdl = PutDlgChkRadio(dlog, TURNPAGES_DI, config.turnPagesInPlay);
	PutDlgChkRadio(dlog, USE_MODIFIER_EFFECTS_DI, config.useModNREffects);

	SelectDialogItemText(dlog, CHANNEL, 0, ENDTEXT);					/* Initial selection */

	CenterWindow(GetDialogWindow(dlog), 100);
	ShowWindow(GetDialogWindow(dlog));
	if (useWhichMIDI == MIDIDR_CM && cmInputMenuH != NULL) {
		DrawPopUp(&cmInputPopup);
	}
	
	OutlineOKButton(dlog,True);
	
/*--- 2. Interact with user til they push OK or Cancel. --- */

	dialogOver = 0;
	while (dialogOver==0) {
		do {
			ModalDialog(filterUPP, &ditem);								/* Handle dialog events */
			switch (ditem) {
				case OK:
				case Cancel:
					dialogOver = ditem;
					break;
				case FEEDBACK:
	  				PutDlgChkRadio(dlog, FEEDBACK, !GetDlgChkRadio(dlog, FEEDBACK));
			  		break;
				case PART_SETTINGS:
				case NO_PART_SETTINGS:
					if (ditem!=groupPartSets) SwitchRadio(dlog, &groupPartSets, ditem);
					newval = GetDlgChkRadio(dlog, PART_SETTINGS);					
					HiliteControl((ControlHandle)patchHdl, (newval? CTL_ACTIVE : CTL_INACTIVE));
					if (!newval) PutDlgChkRadio(dlog, SEND_PATCHES, False);
			  		break;
			  	case SEND_PATCHES:
	  				PutDlgChkRadio(dlog, SEND_PATCHES, !GetDlgChkRadio(dlog, SEND_PATCHES));
			  		break;
				case SHARPS_DI:
				case FLATS_DI:
					if (ditem!=groupFlats) SwitchRadio(dlog, &groupFlats, ditem);
					break;
				case DELREDACC_DI:
	  				PutDlgChkRadio(dlog, DELREDACC_DI, !GetDlgChkRadio(dlog, DELREDACC_DI));
					break;
				case TURNPAGES_DI:
	  				PutDlgChkRadio(dlog, TURNPAGES_DI, !GetDlgChkRadio(dlog, TURNPAGES_DI));
					break;				
				case USE_MODIFIER_EFFECTS_DI:
	  				PutDlgChkRadio(dlog, USE_MODIFIER_EFFECTS_DI, !GetDlgChkRadio(dlog,
									USE_MODIFIER_EFFECTS_DI));
					break;				
			  default:
			  	;
			}
		} while (!dialogOver);
	
	/* --- 3. If dialog was terminated with OK, check any new values. --- */
	/* ---    If any are illegal, keep dialog on the screen to try again. - */
	
		if (dialogOver==Cancel) {
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(dlog);										/* Free heap space */
			return;
		}
		else {
			GetDlgWord(dlog, CHANNEL, &newval);
			if (useWhichMIDI == MIDIDR_CM) {
				if (newval<1 || newval>MAXCHANNEL) {
					GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 4);		/* "Channel number must be..." */
					sprintf(strBuf, fmtStr, MAXCHANNEL);
					CParamText(strBuf, "", "", "");
					StopInform(MIDIBADVALUE_ALRT);
					dialogOver = 0;										/* Keep dialog on screen */
				}
				else {
					MIDIUniqueID endptID = GetCMInputDeviceID();
					if (endptID != kInvalidMIDIUniqueID) {
						if (!CMRecvChannelValid(endptID, newval)) {
							GetIndCString(strBuf, MIDIPLAYERRS_STRS, 19);	/* "Channel not valid for device" */
							CParamText(strBuf, "", "", "");
							StopInform(MIDIBADVALUE_ALRT);
							dialogOver = 0;								/* Keep dialog on screen */
						}
					}
				}
			}
			
			GetDlgWord(dlog, MASTERVELOCITY_DI, &newval);
			if (newval<-127 || newval>127) {						/* 127=MAX_VELOCITY */
				GetIndCString(strBuf, MIDIPLAYERRS_STRS, 5);		/* "Master velocity must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(MIDIBADVALUE_ALRT);
				dialogOver = 0;										/* Keep dialog on screen */
			}

			GetDlgWord(dlog, DEFLAM_TIME_DI, &newval);
			if (newval<1 || newval>999) {
				GetIndCString(strBuf, MIDIPLAYERRS_STRS, 6);		/* "Note chording time must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(MIDIBADVALUE_ALRT);
				dialogOver = 0;										/* Keep dialog on screen */
			}

			GetDlgWord(dlog, MINDUR_DI, &newval);
			if (newval<1 || newval>127) {
				GetIndCString(strBuf, MIDIPLAYERRS_STRS, 7);		/* "Minimum dur. not ignored must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(MIDIBADVALUE_ALRT);
				dialogOver = 0;										/* Keep dialog on screen */
			}

			GetDlgWord(dlog, MINVELOCITY_DI, &newval);
			if (newval<1 || newval>127) {	 						/* 127=MAX_VELOCITY */
				GetIndCString(strBuf, MIDIPLAYERRS_STRS, 8);		/* "Minimum vel. not ignored must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(MIDIBADVALUE_ALRT);
				dialogOver = 0;										/* Keep dialog on screen */
			}
		}
	}
	
/* --- 4. Dialog was terminated with OK and all values are legal. --- */

	/* Pick up changes to doc fields and, if any changed, set doc->changed. */
	
	newval = GetDlgChkRadio(dlog, FEEDBACK);
	GEN_ACCEPT(doc->noteInsFeedback);
	
	newval = GetDlgChkRadio(dlog, PART_SETTINGS);
	GEN_ACCEPT(doc->polyTimbral);
	
	newval = !(GetDlgChkRadio(dlog, SEND_PATCHES));
	GEN_ACCEPT(doc->dontSendPatches);

	newval = GetDlgChkRadio(dlog, FLATS_DI);
	GEN_ACCEPT(doc->recordFlats);

	if (useWhichMIDI == MIDIDR_CM) {
		MIDIUniqueID endptID = GetCMInputDeviceID();
		if (endptID != kInvalidMIDIUniqueID) {
			if (doc->cmInputDevice != endptID) {
				doc->cmInputDevice = endptID;
				docDirty = True;
			}
			
			GetDlgWord(dlog, CHANNEL, &newval);
			GEN_ACCEPT(doc->channel);
		}
	}
	else {
		GetDlgWord(dlog, CHANNEL, &newval);
		GEN_ACCEPT(doc->channel);
	}

	GetDlgWord(dlog, MASTERVELOCITY_DI, &newval);
	GEN_ACCEPT(doc->velocity);
	
	GetDlgWord(dlog, DEFLAM_TIME_DI, &newval);
	GEN_ACCEPT(doc->deflamTime);
	
	if (docDirty) doc->changed = True;

	/* Pick up changes to config fields. Checking for changes is done elsewhere. */
	
	GetDlgWord(dlog, MINDUR_DI, &temp);
	config.minRecDuration = temp;
	GetDlgWord(dlog, MINVELOCITY_DI, &temp);
	config.minRecVelocity = temp;
	config.delRedundantAccs = (GetDlgChkRadio(dlog, DELREDACC_DI)!=0? 1 : 0);
	config.turnPagesInPlay = (GetDlgChkRadio(dlog, TURNPAGES_DI)!=0? 1 : 0);
	config.useModNREffects = (GetDlgChkRadio(dlog, USE_MODIFIER_EFFECTS_DI)!=0? 1 : 0);

	if (cmVecDevices != NULL) {
		delete cmVecDevices;
		cmVecDevices = NULL;
	}
	
	DisposePopUp(&cmInputPopup);
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);											/* Free heap space */
	SetPort(oldPort);
	return; 
}


/* -------------------------------------------------------------------- MIDIThruDialog -- */

enum {
	CHK_UseMIDIThru = 3,
	POP_Device,
	STXT_WhenRecording,
	STXT_WhenNotRecording
};

static Rect deviceMenuBox;
static fmsUniqueID thruDevice;
static short thruChannel;

pascal Boolean MIDIThruFilter(DialogPtr dlog, EventRecord *theEvt, short *itemHit);
pascal Boolean MIDIThruFilter(DialogPtr dlog, EventRecord *theEvt, short *itemHit)
{		
	GrafPtr	oldPort;
	Point 	mouseLoc;
	
	switch(theEvt->what) {
		case updateEvt:
			if ((WindowPtr)theEvt->message==GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));				
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, OK, True);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
			}
			break;
		case mouseDown:
			mouseLoc = theEvt->where;
			GlobalToLocal(&mouseLoc);
			if (PtInRect(mouseLoc, &deviceMenuBox)) {
				*itemHit = POP_Device;
				return True;
				break;
			}
			break;
		case autoKey:
		case keyDown:
			if (DlgCmdKey(dlog, theEvt, itemHit, False))
				return True;
			break;
		case activateEvt:
			break;
	}
	return False;
}


Boolean MIDIThruDialog()
{
	short			itemHit, dialogOver, scratch;
	DialogPtr		dlog;
	GrafPtr			oldPort;
	Handle			hndl;
	ModalFilterUPP	filterUPP;
	
	if (useWhichMIDI!=MIDIDR_NONE)
		return False;

	filterUPP = NewModalFilterUPP(MIDIThruFilter);
	if (filterUPP==NULL) {
		MissingDialog(MIDITHRU_DLOG);
		return False;
	}
	dlog = GetNewDialog(MIDITHRU_DLOG, 0L, BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(MIDITHRU_DLOG);
		return False;
	}
	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	thruDevice = config.thruDevice;
	thruChannel = (short)config.thruChannel;

	GetDialogItem(dlog, POP_Device, &scratch, &hndl, &deviceMenuBox);
	PutDlgChkRadio(dlog, CHK_UseMIDIThru, config.midiThru);

	CenterWindow(GetDialogWindow(dlog), 100);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	dialogOver = 0;
	while (dialogOver==0) {
		do {	
			ModalDialog(filterUPP, &itemHit);
			if (itemHit<1) continue;								/* in case returned by filter */
			switch (itemHit) {
				case OK:
				case Cancel:
					dialogOver = itemHit;
					break;
				case CHK_UseMIDIThru:
					PutDlgChkRadio(dlog, itemHit, !GetDlgChkRadio(dlog, itemHit));
					break;
			}
		} while (!dialogOver);
	
		if (dialogOver==OK) {
			config.midiThru = (SignedByte)GetDlgChkRadio(dlog, CHK_UseMIDIThru);													
			config.thruDevice = thruDevice;
			config.thruChannel = (SignedByte)thruChannel;
		}
	}

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	return (itemHit==OK);
}


/* ----------------------------------------------------------------------- MetroDialog -- */

/* Symbolic Dialog Item Numbers */

enum {
	BUT1_OK = 1,
	BUT2_Cancel,
	STXT3_Metronome,
	RAD4_Use,
	RAD5_Use,
	STXT6_Channel,
	EDIT7_Chan,
	EDIT8_Note,
	EDIT9_Vel,
	EDIT10_Dur,
	STXT11_Device,
	CM_METRO_MENU,
	LASTITEM
};

static Rect metroMenuBox;

/* Prototypes */

static DialogPtr	OpenMetroDialog(Boolean, short, short, short, short);
static Boolean		MetroDialogItem(DialogPtr dlog, short itemHit);
static Boolean		MetroBadValues(DialogPtr dlog);

/* Current button (item number) for the radio group */

static short group1;

static DialogPtr OpenMetroDialog(
							Boolean viaMIDI,
							short channel, short note, short velocity, short duration)
{
	GrafPtr oldPort;
	DialogPtr dlog;

	GetPort(&oldPort);
	dlog = GetNewDialog(METRO_DLOG,NULL,BRING_TO_FRONT);
	if (dlog==NULL) { MissingDialog(METRO_DLOG); return(NULL); }

	CenterWindow(GetDialogWindow(dlog),70);
	SetPort(GetDialogWindowPort(dlog));

	/* Fill in dialog's values here */

	group1 = (viaMIDI? RAD4_Use : RAD5_Use);
	PutDlgChkRadio(dlog,RAD4_Use,group1==RAD4_Use);
	PutDlgChkRadio(dlog,RAD5_Use,group1==RAD5_Use);

	PutDlgWord(dlog,EDIT8_Note, note, False);
	PutDlgWord(dlog,EDIT9_Vel, velocity, False);
	PutDlgWord(dlog,EDIT10_Dur, duration, False);
	PutDlgWord(dlog,EDIT7_Chan, channel, True);					/* last so it can be selected */

	ShowWindow(GetDialogWindow(dlog));
	return dlog;
}


/* Deal with user clicking on an item in metronome dialog. Returns whether or not the
dialog should be closed (keepGoing). */

static Boolean MetroDialogItem(DialogPtr dlog, short itemHit)
{
	short type,val; Boolean okay=False,keepGoing=True;
	Handle hndl; Rect box;

	if (itemHit<1 || itemHit>=LASTITEM)
		return(keepGoing);										/* Only legal items, please */

	GetDialogItem(dlog,itemHit,&type,&hndl,&box);
	switch(type) {
		case ctrlItem+btnCtrl:
			switch(itemHit) {
				case BUT1_OK:
					keepGoing = False; okay = True;
					break;
				case BUT2_Cancel:
					keepGoing = False;
					break;
			}
			break;
		case ctrlItem+radCtrl:
			switch(itemHit) {
				case RAD4_Use:									/* Group 1 */
				case RAD5_Use:
					if (itemHit!=group1) {
						SetControlValue((ControlHandle)hndl,val=!GetControlValue((ControlHandle)hndl));
						GetDialogItem(dlog,group1,&type,&hndl,&box);
						SetControlValue((ControlHandle)hndl,False);
						group1 = itemHit;
					}
					break;
			}
			break;
		default:
			;
	}

	if (okay) keepGoing = MetroBadValues(dlog);
	return(keepGoing);
}


/* Pull values out of dialog items and deliver True if any of them are illegal or
inconsistent; otherwise deliver False.  If any values are bad, tell user about the
problem before delivering True. */

static Boolean MetroBadValues(DialogPtr dlog)
{
	short val;  Boolean bad=False;

	GetDlgWord(dlog,EDIT7_Chan,&val);
	if (useWhichMIDI == MIDIDR_CM) {
		MIDIUniqueID device = GetCMOutputDeviceID();
		if (cmMetroMenuH && !CMTransmitChannelValid(device, val)) {
			GetIndCString(strBuf, MIDIPLAYERRS_STRS, 19);	/* "Channel not valid for device" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			bad = True;										/* Keep dialog on screen */
		}
	}

	GetDlgWord(dlog,EDIT8_Note,&val);
	if (val<1 || val>127) {									/* 127=MAX_NOTENUM */
		GetIndCString(strBuf, MIDIPLAYERRS_STRS, 9);		/* "Note number must be..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		bad = True;
	}

	GetDlgWord(dlog,EDIT9_Vel,&val);
	if (val<1 || val>127) {									/* 127=MAX_VELOCITY */
		GetIndCString(strBuf, MIDIPLAYERRS_STRS, 10);		/* "Velocity must be..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		bad = True;
	}

	GetDlgWord(dlog,EDIT10_Dur,&val);
	if (val<1 || val>999) {
		GetIndCString(strBuf, MIDIPLAYERRS_STRS, 11);		/* "Duration must be..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		bad = True;
	}

	return bad;
}


/* -------------------------------------------------------------------------------------- */

//static MenuHandle cmMetroMenuH;
//static UserPopUp	cmOutputPopup;

static MIDIUniqueID GetCMOutputDeviceID()
{
	short choice = cmOutputPopup.currentChoice-1;
	
	if (ValidCMInputIndex(choice)) {
		return cmVecDevices->at(choice);
	}
	return kInvalidMIDIUniqueID;
}

static MenuHandle CreateCMOutputMenu(DialogPtr dlog, UserPopUp *p, Rect * /*box*/,
					MIDIUniqueID device)
{

	Boolean popupOk = InitPopUp(dlog, p,
									CM_METRO_MENU,	/* Dialog item to set p->box from */
									0,				/* Dialog item to set p->prompt from, or 0=set it empty */
									kCMPopupID,		/* CoreMidi popup ID */
									0				/* Initial choice, or 0=none */
									);
	if (!popupOk || p->menu==NULL) return NULL;
		
	cmVecDevices = new MIDIUniqueIDVector();
	long numItems = FillCMDestinationPopup(p->menu, cmVecDevices);

	
	short currChoice = GetCMInputDeviceIndex(device);

	if (ValidCMInputIndex(currChoice)) {
		ChangePopUpChoice(&cmOutputPopup, currChoice + 1);
	}

	return p->menu;
}


/* Build this dialog's window on desktop, and install initial item values. Return the
dialog opened, or NULL if error (no resource, no memory). */

static DialogPtr OpenCMMetroDialog(
							Boolean viaMIDI,
							short channel, short note, short velocity, short duration,
							MIDIUniqueID device)
{
	Handle hndl;  GrafPtr oldPort;
	short scratch;  Str255 deviceStr;
	DialogPtr dlog;

	GetPort(&oldPort);
	dlog = GetNewDialog(CM_METRO_DLOG, NULL, BRING_TO_FRONT);
	if (dlog==NULL) { MissingDialog(CM_METRO_DLOG); return(NULL); }
	
	CenterWindow(GetDialogWindow(dlog), 70);
	SetPort(GetDialogWindowPort(dlog));

	GetDialogItem(dlog, CM_METRO_MENU, &scratch, &hndl, &metroMenuBox);
	cmMetroMenuH = CreateCMOutputMenu(dlog, &cmOutputPopup, &metroMenuBox, device);
	if (cmMetroMenuH==NULL) { SysBeep(5); SetPort(oldPort); return(NULL); }

	/* Fill in dialog's values here */

	if (!CMTransmitChannelValid(device, channel)) {
		device = config.cmDfltOutputDev;
		channel = config.cmDfltOutputChannel;
	}
	GetIndString(deviceStr, MIDIPLAYERRS_STRS, 16);			/* "No devices available" */

	group1 = (viaMIDI? RAD4_Use : RAD5_Use);
	PutDlgChkRadio(dlog,RAD4_Use, group1==RAD4_Use);
	PutDlgChkRadio(dlog,RAD5_Use, group1==RAD5_Use);

	PutDlgWord(dlog,EDIT8_Note, note, False);
	PutDlgWord(dlog,EDIT9_Vel, velocity, False);
	PutDlgWord(dlog,EDIT10_Dur, duration, False);
	PutDlgWord(dlog,EDIT7_Chan, channel, True);				/* last so it can be selected */

	ShowWindow(GetDialogWindow(dlog));
	DrawPopUp(&cmOutputPopup);
	return dlog;
}

/* ------------------------------------------------------------ CMMetroFilter, -Dialog -- */

/* This filter outlines the OK Button and performs standard key and command-key
filtering. */

pascal Boolean CMMetroFilter(DialogPtr theDialog, EventRecord *theEvt, short *item);
pascal Boolean CMMetroFilter(DialogPtr theDialog, EventRecord *theEvt, short *item)
{
	GrafPtr	oldPort;
	short	type;
	Handle	hndl;
	Rect	box;
	Point 	mouseLoc;
	short	ans;

	switch (theEvt->what) {
		case updateEvt:
			GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
			BeginUpdate(GetDialogWindow(theDialog));
			UpdateDialogVisRgn(theDialog);
			FrameDefault(theDialog,OK,True);
			if (cmMetroMenuH != NULL) DrawPopUp(&cmOutputPopup);
			EndUpdate(GetDialogWindow(theDialog));
			SetPort(oldPort);
			*item = 0;
			return True;
			break;
			
		case mouseDown:
			mouseLoc = theEvt->where;
			GlobalToLocal(&mouseLoc);
			
			/* If user hit the OK or Cancel button, bypass the rest of the control stuff below. */
			
			GetDialogItem(theDialog, BUT1_OK, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = BUT1_OK; break;};
			GetDialogItem(theDialog, BUT2_Cancel, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = BUT2_Cancel; break;};
			
			if (cmMetroMenuH != NULL) {
				GetDialogItem(theDialog, CM_METRO_MENU, &type, &hndl, &box);
				
				if (PtInRect(mouseLoc, &box)) {
					ans = DoUserPopUp(&cmOutputPopup);
					if (ans) *item = CM_METRO_MENU;
				}
			}
			break;

		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvt, item, False)) return True;
			break;
	}
	return False;
}

/*	Display the Metronome dialog.  Return True if OK, False if Cancel or error. */

Boolean CMMetroDialog(SignedByte *viaMIDI, SignedByte *channel, SignedByte *note,
							SignedByte *velocity, short *duration, MIDIUniqueID *device)
{
	short chan, aNote, vel, dur;
	short itemHit, okay, keepGoing=True;
	DialogPtr dlog;  GrafPtr oldPort;
	ModalFilterUPP filterUPP;

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(CMMetroFilter);
	if (filterUPP==NULL) {
		MissingDialog(CM_METRO_DLOG);
		return False;
	}
	GetPort(&oldPort);
	dlog = OpenCMMetroDialog((Boolean)*viaMIDI, *channel, *note, *velocity,
									*duration, *device);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		return False;
	}

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		keepGoing = MetroDialogItem(dlog,itemHit);
	}
	
	/*	Export item values to caller. MetroDialogItem() should have already called
	    MetroBadValues(). */
	
	okay = (itemHit==BUT1_OK);
	if (okay) {
		*viaMIDI = GetDlgChkRadio(dlog, RAD4_Use);
		GetDlgWord(dlog,EDIT7_Chan,&chan); *channel = chan;
		GetDlgWord(dlog,EDIT8_Note,&aNote); *note = aNote;
		GetDlgWord(dlog,EDIT9_Vel,&vel); *velocity = vel;
		GetDlgWord(dlog,EDIT10_Dur,&dur); *duration = dur;
		
		MIDIUniqueID endptID = GetCMOutputDeviceID();
		if (endptID != kInvalidMIDIUniqueID) {
			*device = endptID;
		}
		else {
			*device = kInvalidMIDIUniqueID;
		}
	}

	/* That's all, folks! */
	
	if (cmVecDevices != NULL) {
		delete cmVecDevices;
		cmVecDevices = NULL;
	}

	DisposePopUp(&cmOutputPopup);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);								/* Return dialog to primordial mists */
	SetPort(oldPort);
	
	return okay;
}


/* ----------------------------------------------------------------------- MetroDialog -- */

Boolean MetroDialog(SignedByte *viaMIDI, SignedByte *channel, SignedByte *note,
							SignedByte *velocity, short *duration)
{
	short chan, aNote, vel, dur;
	short itemHit, okay, keepGoing=True;
	DialogPtr dlog;  GrafPtr oldPort;
	ModalFilterUPP filterUPP;

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP==NULL) {
		MissingDialog(METRO_DLOG);
		return False;
	}
	GetPort(&oldPort);
	dlog = OpenMetroDialog((Boolean)*viaMIDI, *channel, *note, *velocity, *duration);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		return False;
	}

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		keepGoing = MetroDialogItem(dlog,itemHit);
	}
	
	/* Export item values to caller. MetroDialogItem() should have already called
	   MetroBadValues(). */
	
	okay = (itemHit==BUT1_OK);
	if (okay) {
		*viaMIDI = GetDlgChkRadio(dlog, RAD4_Use);
		GetDlgWord(dlog,EDIT7_Chan,&chan); *channel = chan;
		GetDlgWord(dlog,EDIT8_Note,&aNote); *note = aNote;
		GetDlgWord(dlog,EDIT9_Vel,&vel); *velocity = vel;
		GetDlgWord(dlog,EDIT10_Dur,&dur); *duration = dur;
	}

	/* That's all, folks! */
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);								/* Return dialog to primordial mists */
	SetPort(oldPort);
	
	return okay;
}

/* ------------------------------------------------------------------- MIDIDynamDialog -- */

static Boolean ItemValInRange(DialogPtr, short, short, short);
static Boolean ItemValInRange(DialogPtr dlog, short DI, short minVal, short maxVal)
{
	short value;
	
	GetDlgWord(dlog,DI,&value);
	return (value>=minVal && value<=maxVal);
}

enum {
	FIRST_VEL_DI=4,			/* Item number for PPP_DYNAM */
	APPLY_DI=22
};

#define DYN_LEVELS 8		/* No. of consecutive dynamics in dialog, starting w/PPP_DYNAM */

Boolean MIDIDynamDialog(Document */*doc*/, Boolean *apply)
{
	DialogPtr		dlog;
	short			i, ditem, velo;
	GrafPtr			oldPort;
	Boolean			badValue;
	char			fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(MIDIVELTABLE_DLOG);
		return False;
	}	
	GetPort(&oldPort);
	dlog = GetNewDialog(MIDIVELTABLE_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		
		PutDlgChkRadio(dlog, APPLY_DI, *apply);
		for (i = 1; i<DYN_LEVELS; i++)
			PutDlgWord(dlog, FIRST_VEL_DI+2*i, dynam2velo[i+PPP_DYNAM], False);
			
		/* Do level 0 last so we can leave it hilited. */
		
		PutDlgWord(dlog, FIRST_VEL_DI, dynam2velo[PPP_DYNAM], True);

		CenterWindow(GetDialogWindow(dlog), 45);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		do {
			while (True) {
				ModalDialog(filterUPP, &ditem);
				if (ditem==OK || ditem==Cancel) break;
				if (ditem==APPLY_DI) 
					PutDlgChkRadio(dlog, APPLY_DI, !GetDlgChkRadio(dlog,APPLY_DI));
			}

			if (ditem==OK) {
				for (badValue = False, i = 0; i<DYN_LEVELS; i++)
				if (!ItemValInRange(dlog, FIRST_VEL_DI+2*i, 1, MAX_VELOCITY))
					badValue = True;
				if (badValue) {
					GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 12);		/* "Velocities must be..." */
					sprintf(strBuf, fmtStr, 1, MAX_VELOCITY);
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					velo = -9999;
				}
				else {
					for (i = 0; i<DYN_LEVELS; i++) {
						GetDlgWord(dlog,FIRST_VEL_DI+2*i,&velo);
						dynam2velo[i+PPP_DYNAM] = velo;
					}
					*apply = GetDlgChkRadio(dlog,APPLY_DI);
				}
			}
			else /* ditem==Cancel */
				break;			
		} while (velo<1 || velo>MAX_VELOCITY);
			
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
	}
	else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(MIDIVELTABLE_DLOG);
	}

	SetPort(oldPort);

	return (ditem==OK);
}


/* ---------------------------------------------------------------- MIDIModifierDialog -- */

enum {
	STAC_VEL_DI=7,
	STAC_DUR_DI,
	WEDGE_VEL_DI,
	WEDGE_DUR_DI,
	ACCENT_VEL_DI,
	ACCENT_DUR_DI,
	HVYACCENT_VEL_DI,
	HVYACCENT_DUR_DI,
	HVYACCENTSTAC_VEL_DI,
	HVYACCENTSTAC_DUR_DI,
	TENUTO_VEL_DI,
	TENUTO_DUR_DI,
	LAST_MDI
};

Boolean MIDIModifierDialog(Document */*doc*/)
{
	DialogPtr		dlog;
	short			i, ditem, value;
	Boolean			valuesOK;
	GrafPtr			oldPort;
	char			fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(MIDIMODTABLES_DLOG);
		return False;
	}	
	GetPort(&oldPort);
	dlog = GetNewDialog(MIDIMODTABLES_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		
		PutDlgWord(dlog, STAC_VEL_DI, modNRVelOffsets[MOD_STACCATO], False);
		PutDlgWord(dlog, STAC_DUR_DI, modNRDurFactors[MOD_STACCATO], False);
		PutDlgWord(dlog, WEDGE_VEL_DI, modNRVelOffsets[MOD_WEDGE], False);
		PutDlgWord(dlog, WEDGE_DUR_DI, modNRDurFactors[MOD_WEDGE], False);
		PutDlgWord(dlog, ACCENT_VEL_DI, modNRVelOffsets[MOD_ACCENT], False);
		PutDlgWord(dlog, ACCENT_DUR_DI, modNRDurFactors[MOD_ACCENT], False);
		PutDlgWord(dlog, HVYACCENT_VEL_DI, modNRVelOffsets[MOD_HEAVYACCENT], False);
		PutDlgWord(dlog, HVYACCENT_DUR_DI, modNRDurFactors[MOD_HEAVYACCENT], False);
		PutDlgWord(dlog, HVYACCENTSTAC_VEL_DI, modNRVelOffsets[MOD_HEAVYACC_STACC], False);
		PutDlgWord(dlog, HVYACCENTSTAC_DUR_DI, modNRDurFactors[MOD_HEAVYACC_STACC], False);
		PutDlgWord(dlog, TENUTO_VEL_DI, modNRVelOffsets[MOD_TENUTO], False);
		PutDlgWord(dlog, TENUTO_DUR_DI, modNRDurFactors[MOD_TENUTO], False);

		SelectDialogItemText(dlog, STAC_VEL_DI, 0, ENDTEXT);

		CenterWindow(GetDialogWindow(dlog), 45);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		while (True) {
			ModalDialog(filterUPP, &ditem);
			if (ditem==OK) {
				valuesOK = True;
				for (i = STAC_VEL_DI; i<LAST_MDI; i+=2) {
					GetDlgWord(dlog, i, &value);
					if (value<-128 || value>127) {
						GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 17);	/* "Velocity offsets must be..." */
						sprintf(strBuf, fmtStr, -128, MAX_VELOCITY);
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						SelectDialogItemText(dlog, i, 0, ENDTEXT);
						valuesOK = False;
						break;
					}
				}
				for (i = STAC_DUR_DI; i<LAST_MDI; i+=2) {
					GetDlgWord(dlog, i, &value);
					if (value<1 || value>10000) {
						GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 18);	/* "Duration change percent must be..." */
						sprintf(strBuf, fmtStr, 1, 10000);
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						SelectDialogItemText(dlog, i, 0, ENDTEXT);
						valuesOK = False;
						break;
					}
				}
				if (valuesOK) {
					GetDlgWord(dlog, STAC_VEL_DI, &value);
					modNRVelOffsets[MOD_STACCATO] = value;
					GetDlgWord(dlog, STAC_DUR_DI, &value);
					modNRDurFactors[MOD_STACCATO] = value;
					GetDlgWord(dlog, WEDGE_VEL_DI, &value);
					modNRVelOffsets[MOD_WEDGE] = value;
					GetDlgWord(dlog, WEDGE_DUR_DI, &value);
					modNRDurFactors[MOD_WEDGE] = value;
					GetDlgWord(dlog, ACCENT_VEL_DI, &value);
					modNRVelOffsets[MOD_ACCENT] = value;
					GetDlgWord(dlog, ACCENT_DUR_DI, &value);
					modNRDurFactors[MOD_ACCENT] = value;
					GetDlgWord(dlog, HVYACCENT_VEL_DI, &value);
					modNRVelOffsets[MOD_HEAVYACCENT] = value;
					GetDlgWord(dlog, HVYACCENT_DUR_DI, &value);
					modNRDurFactors[MOD_HEAVYACCENT] = value;
					GetDlgWord(dlog, HVYACCENTSTAC_VEL_DI, &value);
					modNRVelOffsets[MOD_HEAVYACC_STACC] = value;
					GetDlgWord(dlog, HVYACCENTSTAC_DUR_DI, &value);
					modNRDurFactors[MOD_HEAVYACC_STACC] = value;
					GetDlgWord(dlog, TENUTO_VEL_DI, &value);
					modNRVelOffsets[MOD_TENUTO] = value;
					GetDlgWord(dlog, TENUTO_DUR_DI, &value);
					modNRDurFactors[MOD_TENUTO] = value;
					break;
				}
			}
			else if (ditem==Cancel)
				break;
		}
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
	}
	else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(MIDIMODTABLES_DLOG);
	}

	SetPort(oldPort);

	return (ditem==OK);
}


/* -------------------------------------------------------------------- MutePartDialog -- */

enum {
	RAD3_MutePart = 3,
	RAD4_Unmute,
	STXT6_PartName = 6
};

static short group2;

Boolean MutePartDialog(Document *doc)
{
	short itemHit, partNum;
	Boolean okay, keepGoing=True;
	DialogPtr dlog;  GrafPtr oldPort;
	ModalFilterUPP filterUPP;
	LINK partL;
	PPARTINFO pPart;
	char partNameStr[256];		/* C string */

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(MUTEPART_DLOG);
		return 0;
	}
	GetPort(&oldPort);
	dlog = GetNewDialog(MUTEPART_DLOG,NULL,BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeRoutineDescriptor(filterUPP);
		MissingDialog(MUTEPART_DLOG);
		return(0);
	}
	SetPort(GetDialogPort(dlog));

	/* Change static text item to number and name of the selected part */
	
	partL = Sel2Part(doc);
	pPart = GetPPARTINFO(partL);
	partNum = PartL2Partn(doc, partL);
	sprintf(partNameStr, "part %d, %s", partNum, (strlen(pPart->name)>18?
			pPart->shortName : pPart->name));
	PutDlgString(dlog, STXT6_PartName, CToPString(partNameStr), False);

	group2 = RAD3_MutePart;
	PutDlgChkRadio(dlog, RAD3_MutePart, (group2==RAD3_MutePart));
	PutDlgChkRadio(dlog, RAD4_Unmute, (group2==RAD4_Unmute));

	PlaceWindow(GetDialogWindow(dlog), NULL, 0, 80);
	ShowWindow(GetDialogWindow(dlog));

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		switch(itemHit) {
			case BUT1_OK:
				if (group2==RAD3_MutePart) {
					partNum = PartL2Partn(doc, partL);
					doc->mutedPartNum = partNum;
				}
				else
					doc->mutedPartNum = 0;
				keepGoing = False;
				okay = True;
				break;
			case BUT2_Cancel:
				keepGoing = False;
				okay = False;
				break;
			case RAD3_MutePart:
			case RAD4_Unmute:
				if (itemHit!=group2) SwitchRadio(dlog, &group2, itemHit);
				break;
		}
	}
	
	DisposeRoutineDescriptor(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	
	return(okay);
}


/* ---------------------------------------------------------------- SetPlaySpeedDialog -- */

extern short minDlogVal, maxDlogVal;

enum {
	 SPS_PERCENT_DI=3,
	 UP_SPS_DI=5,
	 DOWN_SPS_DI
}; 

Boolean SetPlaySpeedDialog(void)
{
	DialogPtr dlog;
	GrafPtr oldPort;
	short newPercent, ditem;
	Boolean done;
	ModalFilterUPP filterUPP;

	filterUPP = NewModalFilterUPP(NumberFilter);
	if (filterUPP == NULL) {
		MissingDialog(PLAYSPEED_DLOG);
		return False;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(PLAYSPEED_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		
		newPercent = playTempoPercent;
		PutDlgWord(dlog, SPS_PERCENT_DI, newPercent,True);
		
		UseNumberFilter(dlog, SPS_PERCENT_DI, UP_SPS_DI, DOWN_SPS_DI);
		minDlogVal = 10;
		maxDlogVal = 500;
		
		CenterWindow(GetDialogWindow(dlog), 50);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
		
		done = False;
		while (!done) {
			ModalDialog(filterUPP, &ditem);
			switch (ditem) {
				case OK:
					GetDlgWord(dlog, SPS_PERCENT_DI, &newPercent);
					if (newPercent<minDlogVal || newPercent>maxDlogVal) {
						GetIndCString(strBuf, DIALOGERRS_STRS, 22);		/* "Play tempo percent must be between..." */
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
					}
					else {
						playTempoPercent = newPercent;
						done = True;
					}
					break;
				case Cancel:
					done = True;
					break;
			}
		}
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
	}
	else {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(PLAYSPEED_DLOG);
	}
	SetPort(oldPort);
	return (ditem==OK);
}
