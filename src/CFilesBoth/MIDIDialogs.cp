/***************************************************************************
*	FILE:	MIDIDialogs.c																		*
*	PROJ:	Nightingale, minor rev. for v. 3.6											*
*	DESC:	General MIDI dialog routines (no relation to the "General MIDI"	*
*			standard)																			*
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ------------------------------------------------------------------- MIDIDialog -- */
/*	Setup MIDI channel assignment, tempo, port, etc. N.B. Does not handle SCCA
for MIDI Thru! */


static enum				/* Dialog item numbers */
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
} E_MIDIDialogItems;

// Core MIDI

#include "CoreMIDIDefs.h"

// -------------------------------------------------------------
// Input

static Rect					inputDeviceMenuBox;
static OMSDeviceMenuH	omsInputMenuH;
static MenuHandle			cmInputMenuH;
static UserPopUp			cmInputPopup;
static MIDIUniqueIDVector *cmVecDevices;
static Boolean				omsInputDeviceChanged;

static Rect divRect1, divRect2;

static short kCMPopupID = 300;

// -------------------------------------------------------------
// Output

static MenuHandle cmMetroMenuH;
static UserPopUp	cmOutputPopup;

static MIDIUniqueID GetCMOutputDeviceID(void);

// -------------------------------------------------------------

static Boolean ValidCMInputIndex(short idx) {
	return (idx >= 0 && idx <cmVecDevices->size());
}

static MIDIUniqueID GetCMInputDeviceID()
{
	short choice = cmInputPopup.currentChoice-1;
	
	if (ValidCMInputIndex(choice)) {
		return cmVecDevices->at(choice);
	}
	return kInvalidMIDIUniqueID;
}

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

static MenuHandle CreateCMInputMenu(Document *doc, DialogPtr dlog, UserPopUp *p, Rect *box)
{
	Boolean popupOk = InitPopUp(dlog, p,
									INPUT_DEVICE_MENU,	/* Dialog item to set p->box from */
									0,					/* Dialog item to set p->prompt from, or 0=set it empty */
									kCMPopupID,			/* CoreMidi popup ID */
									0					/* Initial choice, or 0=none */
									);
	if (!popupOk || p->menu==NULL)
		return NULL;
		
	cmVecDevices = new MIDIUniqueIDVector();
	long numItems = FillCMSourcePopup(p->menu, cmVecDevices);
	
	short currChoice = GetCMInputDeviceIndex(doc->cmInputDevice);

	if (ValidCMInputIndex(currChoice)) {
		ChangePopUpChoice(&cmInputPopup, currChoice + 1);
	}
	
	return p->menu;
}

static void DrawMyItems(DialogPtr);
static void DrawMyItems(DialogPtr)
{
	MoveTo(divRect1.left, divRect1.top);
	LineTo(divRect1.right, divRect1.top);
	MoveTo(divRect2.left, divRect2.top);
	LineTo(divRect2.right, divRect2.top);
}

/* This filter outlines the OK Button, draws divider lines, and performs standard
key and command-key filtering. */

static pascal Boolean MIDIFilter(DialogPtr, EventRecord *, short *);
static pascal Boolean MIDIFilter(DialogPtr theDialog, EventRecord *theEvent,
											short *item)
{
	GrafPtr	oldPort;
	short		type;
	Handle	hndl;
	Rect		box;
	Point		mouseLoc;
	short		ans = 0;

	switch (theEvent->what) {
		case updateEvt:
			GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
			BeginUpdate(GetDialogWindow(theDialog));
			DrawMyItems(theDialog);
			UpdateDialogVisRgn(theDialog);
			FrameDefault(theDialog,OK,TRUE);
			if (useWhichMIDI == MIDIDR_CM && cmInputMenuH != NULL) {
				DrawPopUp(&cmInputPopup);
			}
			EndUpdate(GetDialogWindow(theDialog));
			SetPort(oldPort);
			*item = 0;
			return TRUE;
			break;
		case mouseDown:
			*item = 0;
			mouseLoc = theEvent->where;
			GlobalToLocal(&mouseLoc);
			
			/* If user hit the OK or Cancel button, bypass the rest of the control stuff below. */
			GetDialogItem (theDialog, OK, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = OK; break;};
			GetDialogItem (theDialog, Cancel, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = Cancel; break;};

			if (useWhichMIDI == MIDIDR_CM && cmInputMenuH != NULL) {
				GetDialogItem (theDialog, INPUT_DEVICE_MENU, &type, &hndl, &box);
				if (PtInRect(mouseLoc, &box))	{
					if (ans = DoUserPopUp(&cmInputPopup)) {
						*item = INPUT_DEVICE_MENU;
					}
				}
			}
			break;

		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, item, FALSE)) return TRUE;
			break;
		default:
			break;
	}
	return FALSE;
}


#define GEN_ACCEPT(field)	if (newval!=(field)) \
						{ (field) = newval;  docDirty = TRUE; }

void MIDIDialog(Document *doc)
{
	DialogPtr dlog;
	GrafPtr oldPort;
	short dialogOver;
	short ditem, newval, temp, anInt, groupFlats, groupPartSets;
	Handle aHdl, patchHdl, turnHdl;
	Boolean docDirty = FALSE;
	char fmtStr[256];
	short scratch;
	ModalFilterUPP	filterUPP;
	
	ArrowCursor();

/* --- 1. Create the dialog and initialize its contents. --- */

	filterUPP = NewModalFilterUPP(MIDIFilter);
	if (filterUPP == NULL) {
		MissingDialog(MIDISETUP_DLOG);
		return;
	}
	GetPort(&oldPort);
	omsInputDeviceChanged = FALSE;

	/* We use the same dialog resource for OMS and FreeMIDI.  For the
		latter, we hide the channel edit field. */
	if (useWhichMIDI == MIDIDR_CM) {
		dlog = GetNewDialog(OMS_MIDISETUP_DLOG, NULL, BRING_TO_FRONT);
		if (dlog == NULL) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(OMS_MIDISETUP_DLOG);
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
			docDirty = TRUE;
		}
		cmInputMenuH = CreateCMInputMenu(doc, dlog, &cmInputPopup, &inputDeviceMenuBox);
	}

	/*
	 * Get the divider lines, defined by some user items, and get them out of the way 
	 *	so they don't hide any items underneath.
	 */
	GetDialogItem(dlog,DIVIDER1_DI,&anInt,&aHdl,&divRect1);
	GetDialogItem(dlog,DIVIDER2_DI,&anInt,&aHdl,&divRect2);
	
	HideDialogItem(dlog,DIVIDER1_DI);
	HideDialogItem(dlog,DIVIDER2_DI);

	groupFlats = (doc->recordFlats? FLATS_DI : SHARPS_DI);		/* Set up radio button group */
	PutDlgChkRadio(dlog, groupFlats, 1);

	PutDlgWord(dlog, CHANNEL, doc->channel, FALSE);

	PutDlgWord(dlog, MASTERVELOCITY_DI, doc->velocity, FALSE);
	PutDlgChkRadio(dlog, FEEDBACK, doc->feedback);
	
	groupPartSets = (doc->polyTimbral? PART_SETTINGS : NO_PART_SETTINGS);	/* Set up radio button group */
	PutDlgChkRadio(dlog, groupPartSets, 1);

	patchHdl = PutDlgChkRadio(dlog, SEND_PATCHES, !doc->dontSendPatches);
	if (!doc->polyTimbral) HiliteControl((ControlHandle)patchHdl, CTL_INACTIVE);
	PutDlgWord(dlog, DEFLAM_TIME_DI, doc->deflamTime, FALSE);	
	PutDlgWord(dlog, MINDUR_DI, config.minRecDuration, FALSE);	
	PutDlgWord(dlog, MINVELOCITY_DI, config.minRecVelocity, FALSE);	
	PutDlgChkRadio(dlog, DELREDACC_DI, config.delRedundantAccs);
	turnHdl = PutDlgChkRadio(dlog, TURNPAGES_DI, config.turnPagesInPlay);
	PutDlgChkRadio(dlog, USE_MODIFIER_EFFECTS_DI, config.useModNREffects);

	SelectDialogItemText(dlog, CHANNEL, 0, ENDTEXT);					/* Initial selection */

	CenterWindow(GetDialogWindow(dlog), 100);
	ShowWindow(GetDialogWindow(dlog));
	if (useWhichMIDI == MIDIDR_CM && cmInputMenuH != NULL) {
		DrawPopUp(&cmInputPopup);
	}
	
	OutlineOKButton(dlog,TRUE);
	
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
	  				PutDlgChkRadio(dlog, FEEDBACK,
						!GetDlgChkRadio(dlog, FEEDBACK));
			  		break;
				case PART_SETTINGS:
				case NO_PART_SETTINGS:
					if (ditem!=groupPartSets) SwitchRadio(dlog, &groupPartSets, ditem);
					newval = GetDlgChkRadio(dlog, PART_SETTINGS);					
					HiliteControl((ControlHandle)patchHdl, (newval? CTL_ACTIVE : CTL_INACTIVE));
					if (!newval) PutDlgChkRadio(dlog, SEND_PATCHES, 0);
			  		break;
			  	case SEND_PATCHES:
	  				PutDlgChkRadio(dlog, SEND_PATCHES,
						!GetDlgChkRadio(dlog, SEND_PATCHES));
			  		break;
				case SHARPS_DI:
				case FLATS_DI:
					if (ditem!=groupFlats) SwitchRadio(dlog, &groupFlats, ditem);
					break;
				case DELREDACC_DI:
	  				PutDlgChkRadio(dlog, DELREDACC_DI,
						!GetDlgChkRadio(dlog, DELREDACC_DI));
					break;
				case TURNPAGES_DI:
	  				PutDlgChkRadio(dlog, TURNPAGES_DI,
						!GetDlgChkRadio(dlog, TURNPAGES_DI));
					break;				
				case USE_MODIFIER_EFFECTS_DI:
	  				PutDlgChkRadio(dlog, USE_MODIFIER_EFFECTS_DI,
						!GetDlgChkRadio(dlog, USE_MODIFIER_EFFECTS_DI));
					break;				
			  default:
			  	;
			}
		} while (!dialogOver);
	
	/* --- 3. If dialog was terminated with OK, check any new values. --- */
	/* ---    If any are illegal, keep dialog on the screen to try again. - */
	
		if (dialogOver==Cancel) {
			DisposeModalFilterUPP(filterUPP);
			DisposeDialog(dlog);											/* Free heap space */
			return;
		}
		else {
			GetDlgWord(dlog, CHANNEL, &newval);
			if (useWhichMIDI == MIDIDR_CM) {
				if (newval<1 || newval>MAXCHANNEL) {
					GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 4);	/* "Channel number must be..." */
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
							dialogOver = 0;										/* Keep dialog on screen */
						}
					}
				}
			}
			
			GetDlgWord(dlog, MASTERVELOCITY_DI, &newval);
			if (newval<-127 || newval>127) {						/* 127=MAX_VELOCITY */
				GetIndCString(strBuf, MIDIPLAYERRS_STRS, 5);	/* "Master velocity must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(MIDIBADVALUE_ALRT);
				dialogOver = 0;										/* Keep dialog on screen */
			}

			GetDlgWord(dlog, DEFLAM_TIME_DI, &newval);
			if (newval<1 || newval>999) {
				GetIndCString(strBuf, MIDIPLAYERRS_STRS, 6);	/* "Note chording time must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(MIDIBADVALUE_ALRT);
				dialogOver = 0;										/* Keep dialog on screen */
			}

			GetDlgWord(dlog, MINDUR_DI, &newval);
			if (newval<1 || newval>127) {
				GetIndCString(strBuf, MIDIPLAYERRS_STRS, 7);	/* "Minimum dur. not ignored must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(MIDIBADVALUE_ALRT);
				dialogOver = 0;										/* Keep dialog on screen */
			}

			GetDlgWord(dlog, MINVELOCITY_DI, &newval);
			if (newval<1 || newval>127) {	 						/* 127=MAX_VELOCITY */
				GetIndCString(strBuf, MIDIPLAYERRS_STRS, 8);	/* "Minimum vel. not ignored must be..." */
				CParamText(strBuf, "", "", "");
				StopInform(MIDIBADVALUE_ALRT);
				dialogOver = 0;										/* Keep dialog on screen */
			}
		}
	}
	
/* --- 4. Dialog was terminated with OK and all values are legal. --- */

	/* Pick up changes to doc fields and, if any changed, set doc->changed. */
	
	newval = GetDlgChkRadio(dlog, FEEDBACK);
	GEN_ACCEPT(doc->feedback);
	
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
				docDirty = TRUE;
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
	
	if (docDirty) doc->changed = TRUE;

	/* Pick up changes to config fields. Checking for changes is done elsewhere. */
	
	GetDlgWord(dlog, MINDUR_DI, &temp);
	config.minRecDuration = temp;
	GetDlgWord(dlog, MINVELOCITY_DI, &temp);
	config.minRecVelocity = temp;
	config.delRedundantAccs = (GetDlgChkRadio(dlog, DELREDACC_DI)!=0? 1 : 0);
	config.turnPagesInPlay = (GetDlgChkRadio(dlog, TURNPAGES_DI)!=0? 1 : 0);
	config.useModNREffects = (GetDlgChkRadio(dlog, USE_MODIFIER_EFFECTS_DI)!=0? 1 : 0);

	if (omsInputMenuH)
		DisposeOMSDeviceMenu(omsInputMenuH);

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

/* --------------------------------------------------------------- MIDIThruDialog -- */

static enum {
	CHK_UseMIDIThru = 3,
	POP_Device,
	STXT_WhenRecording,
	STXT_WhenNotRecording
} E_MidiThruItems;

static Rect deviceMenuBox;
static fmsUniqueID thruDevice;
static short thruChannel;

pascal Boolean MIDIThruFilter(DialogPtr dlog, EventRecord *theEvent, short *itemHit);
pascal Boolean MIDIThruFilter(DialogPtr dlog, EventRecord *theEvent, short *itemHit)
{		
	GrafPtr	oldPort;
	Point 	mouseLoc;
	
	switch(theEvent->what) {
		case updateEvt:
			if ((WindowPtr)theEvent->message==GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));				
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, OK, TRUE);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
			}
			break;
		case mouseDown:
			mouseLoc = theEvent->where;
			GlobalToLocal(&mouseLoc);
			if (PtInRect(mouseLoc, &deviceMenuBox)) {
				*itemHit = POP_Device;
				return TRUE;
				break;
			}
			break;
		case autoKey:
		case keyDown:
			if (DlgCmdKey(dlog, theEvent, itemHit, FALSE))
				return TRUE;
			break;
		case activateEvt:
			break;
	}
	return FALSE;
}


Boolean MIDIThruDialog()
{
	short			itemHit, dialogOver, scratch;
	DialogPtr	dlog;
	GrafPtr		oldPort;
	Handle		hndl;
	ModalFilterUPP	filterUPP;
	
	/* ??Could support OMS later, but need a channel text field? */
//	if (useWhichMIDI!=MIDIDR_FMS)
	if (useWhichMIDI!=MIDIDR_NONE)
		return FALSE;

	filterUPP = NewModalFilterUPP(MIDIThruFilter);
	if (filterUPP==NULL) {
		MissingDialog(MIDITHRU_DLOG);
		return FALSE;
	}
	dlog = GetNewDialog(MIDITHRU_DLOG, 0L, BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(MIDITHRU_DLOG);
		return FALSE;
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

broken:
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	return (itemHit==OK);
}

/* ------------------------------------------------------------------ MetroDialog -- */

/* Symbolic Dialog Item Numbers */

static enum {
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
	OMS_METRO_MENU,
	FMS_METRO_MENU = OMS_METRO_MENU,
	CM_METRO_MENU = OMS_METRO_MENU,
	LASTITEM
	} E_MetroItems;

static OMSDeviceMenuH omsMetroMenuH;
static Rect metroMenuBox;
static fmsUniqueID fmsMetroDevice = noUniqueID;
static short fmsMetroChannel = 0;

/* Prototypes */

static DialogPtr	OpenOMSMetroDialog(Boolean, short, short, short, short, OMSUniqueID);
static DialogPtr	OpenMetroDialog(Boolean, short, short, short, short);
static Boolean		MetroDialogItem(DialogPtr dlog, short itemHit);
static Boolean		MetroBadValues(DialogPtr dlog);

/* Current button (item number) for the radio group */

static short group1;

/* Build this dialog's window on desktop, and install initial item values. Return
the dlog opened, or NULL if error (no resource, no memory). */

static DialogPtr OpenOMSMetroDialog(
								Boolean viaMIDI,
								short channel, short note, short velocity, short duration,
								OMSUniqueID device)
{
	Handle hndl; GrafPtr oldPort;
	short scratch; Str255 deviceStr; DialogPtr dlog;

	GetPort(&oldPort);
	dlog = GetNewDialog(OMS_METRO_DLOG,NULL,BRING_TO_FRONT);
	if (dlog==NULL) { MissingDialog(OMS_METRO_DLOG); return(NULL); }
	
	CenterWindow(GetDialogWindow(dlog),70);
	SetPort(GetDialogWindowPort(dlog));

	GetDialogItem(dlog, OMS_METRO_MENU, &scratch, &hndl, &metroMenuBox);
	if (omsMetroMenuH==NULL) { SysBeep(5); SetPort(oldPort); return(NULL);}

	/* Fill in dialog's values here */

	GetIndString(deviceStr, MIDIPLAYERRS_STRS, 16);			/* "No devices available" */
	SetOMSDeviceMenuSelection(omsMetroMenuH, 0, device, deviceStr, TRUE);

	group1 = (viaMIDI? RAD4_Use : RAD5_Use);
	PutDlgChkRadio(dlog,RAD4_Use,group1==RAD4_Use);
	PutDlgChkRadio(dlog,RAD5_Use,group1==RAD5_Use);

	PutDlgWord(dlog,EDIT8_Note, note, FALSE);
	PutDlgWord(dlog,EDIT9_Vel, velocity, FALSE);
	PutDlgWord(dlog,EDIT10_Dur, duration, FALSE);
	PutDlgWord(dlog,EDIT7_Chan, channel, TRUE);			/* last so it can be selected */

	ShowWindow(GetDialogWindow(dlog));
	DrawOMSDeviceMenu(omsMetroMenuH);
	TextFont(0); TextSize(0); /* Prevent OMS menu's font from infecting the other items. */
	return dlog;
}

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

	PutDlgWord(dlog,EDIT8_Note, note, FALSE);
	PutDlgWord(dlog,EDIT9_Vel, velocity, FALSE);
	PutDlgWord(dlog,EDIT10_Dur, duration, FALSE);
	PutDlgWord(dlog,EDIT7_Chan, channel, TRUE);			/* last so it can be selected */

	ShowWindow(GetDialogWindow(dlog));
	return dlog;
}


/* Deal with user clicking on an item in metronome dialog. Returns whether or not
the dialog should be closed (keepGoing). */

static Boolean MetroDialogItem(DialogPtr dlog, short itemHit)
{
	short type,val; Boolean okay=FALSE,keepGoing=TRUE;
	Handle hndl; Rect box;

	if (itemHit<1 || itemHit>=LASTITEM)
		return(keepGoing);												/* Only legal items, please */

	GetDialogItem(dlog,itemHit,&type,&hndl,&box);
	switch(type) {
		case ctrlItem+btnCtrl:
			switch(itemHit) {
				case BUT1_OK:
					keepGoing = FALSE; okay = TRUE;
					break;
				case BUT2_Cancel:
					keepGoing = FALSE;
					break;
			}
			break;
		case ctrlItem+radCtrl:
			switch(itemHit) {
				case RAD4_Use:												/* Group 1 */
				case RAD5_Use:
					if (itemHit!=group1) {
						SetControlValue((ControlHandle)hndl,val=!GetControlValue((ControlHandle)hndl));
						GetDialogItem(dlog,group1,&type,&hndl,&box);
						SetControlValue((ControlHandle)hndl,FALSE);
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


/* Pull values out of dialog items and deliver TRUE if any of them are illegal or
inconsistent; otherwise deliver FALSE.  If any values are bad, tell user about the
problem before delivering TRUE. */

static Boolean MetroBadValues(DialogPtr dlog)
{
	short val; Boolean bad=FALSE;

	GetDlgWord(dlog,EDIT7_Chan,&val);
	if (useWhichMIDI == MIDIDR_CM) {
		MIDIUniqueID device = GetCMOutputDeviceID();
		if (cmMetroMenuH && !CMTransmitChannelValid(device, val)) {
			GetIndCString(strBuf, MIDIPLAYERRS_STRS, 19);	/* "Channel not valid for device" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			bad = TRUE;													/* Keep dialog on screen */
		}
	}

	GetDlgWord(dlog,EDIT8_Note,&val);
	if (val<1 || val>127) {											/* 127=MAX_NOTENUM */
		GetIndCString(strBuf, MIDIPLAYERRS_STRS, 9);			/* "Note number must be..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		bad = TRUE;
	}

	GetDlgWord(dlog,EDIT9_Vel,&val);
	if (val<1 || val>127) {											/* 127=MAX_VELOCITY */
		GetIndCString(strBuf, MIDIPLAYERRS_STRS, 10);		/* "Velocity must be..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		bad = TRUE;
	}

	GetDlgWord(dlog,EDIT10_Dur,&val);
	if (val<1 || val>999) {
		GetIndCString(strBuf, MIDIPLAYERRS_STRS, 11);		/* "Duration must be..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		bad = TRUE;
	}

	return bad;
}

/* --------------------------------------------------------------- FMSMetroFilter -- */

pascal Boolean FMSMetroFilter(DialogPtr theDialog, EventRecord *theEvent, short *item);
pascal Boolean FMSMetroFilter(DialogPtr theDialog, EventRecord *theEvent, short *item)
{
	GrafPtr		oldPort;
	short			type;
	Handle		hndl;
	Rect			box;
	Point mouseLoc;

	switch (theEvent->what) {
		case updateEvt:
			GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
			BeginUpdate(GetDialogWindow(theDialog));
			UpdateDialogVisRgn(theDialog);
			FrameDefault(theDialog,OK,TRUE);
			EndUpdate(GetDialogWindow(theDialog));
			SetPort(oldPort);
			*item = 0;
			return TRUE;
			break;
			
		case mouseDown:
			mouseLoc = theEvent->where;
			GlobalToLocal(&mouseLoc);
			
			/* If we've hit the OK or Cancel button, bypass the rest of the control stuff below. */
			GetDialogItem(theDialog, BUT1_OK, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = BUT1_OK; break;};
			GetDialogItem(theDialog, BUT2_Cancel, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = BUT2_Cancel; break;};
			
			if (PtInRect(mouseLoc, &metroMenuBox)) {
				*item = FMS_METRO_MENU;
				break;
			}
			break;

		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, item, FALSE)) return TRUE;
			break;
	}
	return FALSE;
}

/*
 *	Display the Metronome dialog.  Return TRUE if OK, FALSE if Cancel or error.
 */

Boolean FMSMetroDialog(SignedByte *viaMIDI, SignedByte *channel, SignedByte *note,
								SignedByte *velocity, short *duration, fmsUniqueID *device)
{
	short				aNote, vel, dur;
	short				scratch, itemHit, okay, keepGoing=TRUE;
	Handle			hndl;
	DialogPtr		dlog;
	GrafPtr			oldPort;
	ModalFilterUPP	filterUPP;

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(FMSMetroFilter);
	if (filterUPP==NULL) {
		MissingDialog(OMS_METRO_DLOG);
		return FALSE;
	}
	/* NB: We use the OMS dlog for FreeMIDI, but we hide the channel field */
	dlog = GetNewDialog(OMS_METRO_DLOG, NULL, BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(OMS_METRO_DLOG);
		return FALSE;
	}
	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	HideDialogItem(dlog, EDIT7_Chan);
	HideDialogItem(dlog, EDIT7_Chan-1);		/* static text "channel" label */

	GetDialogItem(dlog, OMS_METRO_MENU, &scratch, &hndl, &metroMenuBox);
	metroMenuBox.right += 50;		/* widen this, since we've hidden channel field */
	fmsMetroDevice = *device;
	fmsMetroChannel = *channel;

	/* Fill in dialog's values here */

	group1 = (*viaMIDI? RAD4_Use : RAD5_Use);
	PutDlgChkRadio(dlog, RAD4_Use, (group1==RAD4_Use));
	PutDlgChkRadio(dlog, RAD5_Use, (group1==RAD5_Use));

	PutDlgWord(dlog, EDIT9_Vel, *velocity, FALSE);
	PutDlgWord(dlog, EDIT10_Dur, *duration, FALSE);
	PutDlgWord(dlog, EDIT8_Note, *note, TRUE);			/* last so it can be selected */

	CenterWindow(GetDialogWindow(dlog), 100);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP, &itemHit);
		keepGoing = MetroDialogItem(dlog,itemHit);
	}
	
	if (okay = (itemHit==BUT1_OK)) {
		*viaMIDI = GetDlgChkRadio(dlog, RAD4_Use);
		GetDlgWord(dlog, EDIT8_Note, &aNote); *note = aNote;
		GetDlgWord(dlog, EDIT9_Vel, &vel); *velocity = vel;
		GetDlgWord(dlog, EDIT10_Dur, &dur); *duration = dur;
		*device = fmsMetroDevice;
		*channel = fmsMetroChannel;
	}

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	return okay;
}


/* --------------------------------------------------------------- OMSMetroFilter -- */
/* This filter outlines the OK Button and performs standard key and command-
key filtering. */

pascal Boolean OMSMetroFilter(DialogPtr theDialog, EventRecord *theEvent, short *item);
pascal Boolean OMSMetroFilter(DialogPtr theDialog, EventRecord *theEvent, short *item)
{
	GrafPtr		oldPort;
	short			type;
	Handle		hndl;
	Rect			box;
	Point mouseLoc;
	short omsMenuItem;

	switch (theEvent->what) {
		case updateEvt:
			GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
			BeginUpdate(GetDialogWindow(theDialog));
			UpdateDialogVisRgn(theDialog);
			FrameDefault(theDialog,OK,TRUE);
			if (omsMetroMenuH != NULL)
				DrawOMSDeviceMenu(omsMetroMenuH);
			TextFont(0); TextSize(0); /* Prevent OMS menu's font from infecting the other items. */
			EndUpdate(GetDialogWindow(theDialog));
			SetPort(oldPort);
			*item = 0;
			return TRUE;
			break;
			
		case mouseDown:
			mouseLoc = theEvent->where;
			GlobalToLocal(&mouseLoc);
			
			/* If we've hit the OK or Cancel button, bypass the rest of the control stuff below. */
			GetDialogItem(theDialog, BUT1_OK, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = BUT1_OK; break;};
			GetDialogItem(theDialog, BUT2_Cancel, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = BUT2_Cancel; break;};
			
			if (omsMetroMenuH != 0) {
				if (TestOMSDeviceMenu(omsMetroMenuH, mouseLoc)) {
					*item = OMS_METRO_MENU;
					omsMenuItem = ClickOMSDeviceMenu(omsMetroMenuH);
					TextFont(0); TextSize(0); /* Prevent OMS menu's font from infecting the other items. */
					break;
				}
			}
			break;

		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, item, FALSE)) return TRUE;
			break;
	}
	return FALSE;
}

/*
 *	Display the Metronome dialog.  Return TRUE if OK, FALSE if Cancel or error.
 */

Boolean OMSMetroDialog(SignedByte *viaMIDI, SignedByte *channel, SignedByte *note,
								SignedByte *velocity, short *duration, OMSUniqueID *device)
{
	short chan, aNote, vel, dur;
	short itemHit,okay,keepGoing=TRUE;
	DialogPtr dlog; GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(OMSMetroFilter);
	if (filterUPP==NULL) {
		MissingDialog(OMS_METRO_DLOG);
		return FALSE;
	}
	GetPort(&oldPort);
	dlog = OpenOMSMetroDialog((Boolean)*viaMIDI, *channel, *note, *velocity,
									*duration, *device);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		return FALSE;
	}

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		keepGoing = MetroDialogItem(dlog,itemHit);
	}
	
	/*
	 *	Export item values to caller. MetroDialogItem() should have already called
	 *	MetroBadValues().
	 */
	
	if (okay = (itemHit==BUT1_OK)) {
		*viaMIDI = GetDlgChkRadio(dlog, RAD4_Use);
		GetDlgWord(dlog,EDIT7_Chan,&chan); *channel = chan;
		GetDlgWord(dlog,EDIT8_Note,&aNote); *note = aNote;
		GetDlgWord(dlog,EDIT9_Vel,&vel); *velocity = vel;
		GetDlgWord(dlog,EDIT10_Dur,&dur); *duration = dur;
		*device = 0;
	}

	/* That's all, folks! */
	
	DisposeOMSDeviceMenu(omsMetroMenuH);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);								/* Return dialog to primordial mists */
	SetPort(oldPort);
	
	return okay;
}

// ----------------------------------------------------------------------------------------

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

static MenuHandle CreateCMOutputMenu(DialogPtr dlog, UserPopUp *p, Rect *box, MIDIUniqueID device)
{

	Boolean popupOk = InitPopUp(dlog, p,
											CM_METRO_MENU,	/* Dialog item to set p->box from */
											0,							/* Dialog item to set p->prompt from, or 0=set it empty */
											kCMPopupID,				/* CoreMidi popup ID */
											0							/* Initial choice, or 0=none */
											);
	if (!popupOk || p->menu==NULL)
		return NULL;
		
	cmVecDevices = new MIDIUniqueIDVector();
	long numItems = FillCMDestinationPopup(p->menu, cmVecDevices);

	
	short currChoice = GetCMInputDeviceIndex(device);

	if (ValidCMInputIndex(currChoice)) {
		ChangePopUpChoice(&cmOutputPopup, currChoice + 1);
	}

	return p->menu;
}


/* Build this dialog's window on desktop, and install initial item values. Return
the dlog opened, or NULL if error (no resource, no memory). */

static DialogPtr OpenCMMetroDialog(
								Boolean viaMIDI,
								short channel, short note, short velocity, short duration,
								MIDIUniqueID device)
{
	Handle hndl; GrafPtr oldPort;
	short scratch; Str255 deviceStr; DialogPtr dlog;

	GetPort(&oldPort);
	dlog = GetNewDialog(OMS_METRO_DLOG,NULL,BRING_TO_FRONT);
	if (dlog==NULL) { MissingDialog(OMS_METRO_DLOG); return(NULL); }
	
	CenterWindow(GetDialogWindow(dlog),70);
	SetPort(GetDialogWindowPort(dlog));

	GetDialogItem(dlog, CM_METRO_MENU, &scratch, &hndl, &metroMenuBox);
	cmMetroMenuH = CreateCMOutputMenu(dlog, &cmOutputPopup, &metroMenuBox, device);
	if (cmMetroMenuH==NULL) { SysBeep(5); SetPort(oldPort); return(NULL);}

	/* Fill in dialog's values here */

	if (!CMTransmitChannelValid(device, channel)) {
		device = config.cmDefaultOutputDevice;
		channel = config.cmDefaultOutputChannel;
	}
	GetIndString(deviceStr, MIDIPLAYERRS_STRS, 16);			/* "No devices available" */

	group1 = (viaMIDI? RAD4_Use : RAD5_Use);
	PutDlgChkRadio(dlog,RAD4_Use,group1==RAD4_Use);
	PutDlgChkRadio(dlog,RAD5_Use,group1==RAD5_Use);

	PutDlgWord(dlog,EDIT8_Note, note, FALSE);
	PutDlgWord(dlog,EDIT9_Vel, velocity, FALSE);
	PutDlgWord(dlog,EDIT10_Dur, duration, FALSE);
	PutDlgWord(dlog,EDIT7_Chan, channel, TRUE);			/* last so it can be selected */

	ShowWindow(GetDialogWindow(dlog));
	DrawPopUp(&cmOutputPopup);
	return dlog;
}

/* --------------------------------------------------------------- OMSMetroFilter -- */
/* This filter outlines the OK Button and performs standard key and command-
key filtering. */

pascal Boolean CMMetroFilter(DialogPtr theDialog, EventRecord *theEvent, short *item);
pascal Boolean CMMetroFilter(DialogPtr theDialog, EventRecord *theEvent, short *item)
{
	GrafPtr		oldPort;
	short			type;
	Handle		hndl;
	Rect			box;
	Point 		mouseLoc;
	short			ans;

	switch (theEvent->what) {
		case updateEvt:
			GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
			BeginUpdate(GetDialogWindow(theDialog));
			UpdateDialogVisRgn(theDialog);
			FrameDefault(theDialog,OK,TRUE);
			if (cmMetroMenuH != NULL)
				DrawPopUp(&cmOutputPopup);
			EndUpdate(GetDialogWindow(theDialog));
			SetPort(oldPort);
			*item = 0;
			return TRUE;
			break;
			
		case mouseDown:
			mouseLoc = theEvent->where;
			GlobalToLocal(&mouseLoc);
			
			/* If we've hit the OK or Cancel button, bypass the rest of the control stuff below. */
			GetDialogItem(theDialog, BUT1_OK, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = BUT1_OK; break;};
			GetDialogItem(theDialog, BUT2_Cancel, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	{*item = BUT2_Cancel; break;};
			
			if (cmMetroMenuH != NULL) {
				GetDialogItem(theDialog, CM_METRO_MENU, &type, &hndl, &box);
				
				if (PtInRect(mouseLoc, &box)) {
					if (ans = DoUserPopUp(&cmOutputPopup)) {
						*item = CM_METRO_MENU;
					}
				}
			}
			break;

		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, item, FALSE)) return TRUE;
			break;
	}
	return FALSE;
}

/*
 *	Display the Metronome dialog.  Return TRUE if OK, FALSE if Cancel or error.
 */

Boolean CMMetroDialog(SignedByte *viaMIDI, SignedByte *channel, SignedByte *note,
								SignedByte *velocity, short *duration, MIDIUniqueID *device)
{
	short chan, aNote, vel, dur;
	short itemHit,okay,keepGoing=TRUE;
	DialogPtr dlog; GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(CMMetroFilter);
	if (filterUPP==NULL) {
		MissingDialog(OMS_METRO_DLOG);
		return FALSE;
	}
	GetPort(&oldPort);
	dlog = OpenCMMetroDialog((Boolean)*viaMIDI, *channel, *note, *velocity,
									*duration, *device);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		return FALSE;
	}

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		keepGoing = MetroDialogItem(dlog,itemHit);
	}
	
	/*
	 *	Export item values to caller. MetroDialogItem() should have already called
	 *	MetroBadValues().
	 */
	
	if (okay = (itemHit==BUT1_OK)) {
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


Boolean MetroDialog(SignedByte *viaMIDI, SignedByte *channel, SignedByte *note,
							SignedByte *velocity, short *duration)
{
	short chan, aNote, vel, dur;
	short itemHit,okay,keepGoing=TRUE;
	DialogPtr dlog; GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP==NULL) {
		MissingDialog(METRO_DLOG);
		return FALSE;
	}
	GetPort(&oldPort);
	dlog = OpenMetroDialog((Boolean)*viaMIDI, *channel, *note, *velocity, *duration);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		return FALSE;
	}

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		keepGoing = MetroDialogItem(dlog,itemHit);
	}
	
	/*
	 *	Export item values to caller. MetroDialogItem() should have already called
	 *	MetroBadValues().
	 */
	
	if (okay = (itemHit==BUT1_OK)) {
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

/* -------------------------------------------------------------- MIDIDynamDialog -- */

static Boolean ItemValInRange(DialogPtr, short, short, short);
static Boolean ItemValInRange(DialogPtr dlog, short DI, short minVal, short maxVal)
{
	short value;
	
	GetDlgWord(dlog,DI,&value);
	return (value>=minVal && value<=maxVal);
}

static enum {
	FIRST_VEL_DI=4,			/* Item number for PPP_DYNAM */
	APPLY_DI=22
} E_MIDIDynamItems;

#define DYN_LEVELS 8		/* No. of consecutive dynamics in dialog, starting w/PPP_DYNAM */

Boolean MIDIDynamDialog(Document */*doc*/, Boolean *apply)
{
	DialogPtr	dlog;
	short			i, ditem, velo;
	GrafPtr		oldPort;
	Boolean		badValue;
	char			fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(MIDIVELTABLE_DLOG);
		return FALSE;
	}	
	GetPort(&oldPort);
	dlog = GetNewDialog(MIDIVELTABLE_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		
		PutDlgChkRadio(dlog, APPLY_DI,*apply);
		for (i = 1; i<DYN_LEVELS; i++)
			PutDlgWord(dlog,FIRST_VEL_DI+2*i, dynam2velo[i+PPP_DYNAM], FALSE);
		/* Do level 0 last so we can leave it hilited. */
		PutDlgWord(dlog, FIRST_VEL_DI, dynam2velo[PPP_DYNAM], TRUE);

		CenterWindow(GetDialogWindow(dlog), 45);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		do {
			while (TRUE) {
				ModalDialog(filterUPP, &ditem);
				if (ditem==OK || ditem==Cancel) break;
				if (ditem==APPLY_DI) 
					PutDlgChkRadio(dlog,APPLY_DI,!GetDlgChkRadio(dlog,APPLY_DI));
			}

			if (ditem==OK) {
				for (badValue = FALSE, i = 0; i<DYN_LEVELS; i++)
				if (!ItemValInRange(dlog, FIRST_VEL_DI+2*i, 1, MAX_VELOCITY))
					badValue = TRUE;
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


/* ----------------------------------------------------------- MIDIModifierDialog -- */

static enum {
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
	LAST_ITEM
} E_MIDIModifierItems;

Boolean MIDIModifierDialog(Document */*doc*/)
{
	DialogPtr	dlog;
	short			i, ditem, value;
	Boolean		valuesOK;
	GrafPtr		oldPort;
	char			fmtStr[256];
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(MIDIMODTABLES_DLOG);
		return FALSE;
	}	
	GetPort(&oldPort);
	dlog = GetNewDialog(MIDIMODTABLES_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		
		PutDlgWord(dlog, STAC_VEL_DI, modNRVelOffsets[MOD_STACCATO], FALSE);
		PutDlgWord(dlog, STAC_DUR_DI, modNRDurFactors[MOD_STACCATO], FALSE);
		PutDlgWord(dlog, WEDGE_VEL_DI, modNRVelOffsets[MOD_WEDGE], FALSE);
		PutDlgWord(dlog, WEDGE_DUR_DI, modNRDurFactors[MOD_WEDGE], FALSE);
		PutDlgWord(dlog, ACCENT_VEL_DI, modNRVelOffsets[MOD_ACCENT], FALSE);
		PutDlgWord(dlog, ACCENT_DUR_DI, modNRDurFactors[MOD_ACCENT], FALSE);
		PutDlgWord(dlog, HVYACCENT_VEL_DI, modNRVelOffsets[MOD_HEAVYACCENT], FALSE);
		PutDlgWord(dlog, HVYACCENT_DUR_DI, modNRDurFactors[MOD_HEAVYACCENT], FALSE);
		PutDlgWord(dlog, HVYACCENTSTAC_VEL_DI, modNRVelOffsets[MOD_HEAVYACC_STACC], FALSE);
		PutDlgWord(dlog, HVYACCENTSTAC_DUR_DI, modNRDurFactors[MOD_HEAVYACC_STACC], FALSE);
		PutDlgWord(dlog, TENUTO_VEL_DI, modNRVelOffsets[MOD_TENUTO], FALSE);
		PutDlgWord(dlog, TENUTO_DUR_DI, modNRDurFactors[MOD_TENUTO], FALSE);

		SelectDialogItemText(dlog, STAC_VEL_DI, 0, ENDTEXT);

		CenterWindow(GetDialogWindow(dlog), 45);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		while (TRUE) {
			ModalDialog(filterUPP, &ditem);
			if (ditem==OK) {
				valuesOK = TRUE;
				for (i = STAC_VEL_DI; i<LAST_ITEM; i+=2) {
					GetDlgWord(dlog, i, &value);
					if (value<-128 || value>127) {
						GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 17);	/* "Velocity offsets must be..." */
						sprintf(strBuf, fmtStr, -128, MAX_VELOCITY);
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						SelectDialogItemText(dlog, i, 0, ENDTEXT);
						valuesOK = FALSE;
						break;
					}
				}
				for (i = STAC_DUR_DI; i<LAST_ITEM; i+=2) {
					GetDlgWord(dlog, i, &value);
					if (value<1 || value>10000) {
						GetIndCString(fmtStr, MIDIPLAYERRS_STRS, 18);	/* "Duration change percent must be..." */
						sprintf(strBuf, fmtStr, 1, 10000);
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						SelectDialogItemText(dlog, i, 0, ENDTEXT);
						valuesOK = FALSE;
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


/* ---------------------------------------------------------------- MutePartDialog -- */

static enum {
	RAD3_MutePart = 3,
	RAD4_Unmute,
	STXT6_PartName = 6,
	} E_MutePartItems;

static short group2;

Boolean MutePartDialog(Document *doc)
{
	short itemHit, type, partNum;
	Boolean okay, keepGoing=TRUE;
	DialogPtr dlog; GrafPtr oldPort;
	ModalFilterUPP	filterUPP;
	Handle hndl;
	Rect box;
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
	PutDlgString(dlog, STXT6_PartName, CToPString(partNameStr), FALSE);

	group2 = RAD3_MutePart;
	PutDlgChkRadio(dlog, RAD3_MutePart, (group2==RAD3_MutePart));
	PutDlgChkRadio(dlog, RAD4_Unmute, (group2==RAD4_Unmute));

	PlaceWindow(GetDialogWindow(dlog),NULL,0,80);
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
				keepGoing = FALSE;
				okay = TRUE;
				break;
			case BUT2_Cancel:
				keepGoing = FALSE;
				okay = FALSE;
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


/* ------------------------------------------------------------- SetPlaySpeedDialog -- */

extern short minVal, maxVal;

static enum {
	 SPS_PERCENT_DI=3,
	 UP_SPS_DI=5,
	 DOWN_SPS_DI
} E_SetPlaySpeedItems; 

Boolean SetPlaySpeedDialog(void)
{
	DialogPtr dlog;
	GrafPtr oldPort;
	short newPercent, ditem;
	Boolean done;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(NumberFilter);
	if (filterUPP == NULL) {
		MissingDialog(PLAYSPEED_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(PLAYSPEED_DLOG, NULL, BRING_TO_FRONT);
	if (dlog) {
		SetPort(GetDialogWindowPort(dlog));
		
		newPercent = playTempoPercent;
		PutDlgWord(dlog, SPS_PERCENT_DI, newPercent,TRUE);
		
		UseNumberFilter(dlog, SPS_PERCENT_DI, UP_SPS_DI, DOWN_SPS_DI);
		minVal = 10;
		maxVal = 500;
		
		CenterWindow(GetDialogWindow(dlog), 50);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
		
		done = FALSE;
		while (!done) {
			ModalDialog(filterUPP, &ditem);
			switch (ditem) {
				case OK:
					GetDlgWord(dlog, SPS_PERCENT_DI, &newPercent);
					if (newPercent<minVal || newPercent>maxVal) {
						GetIndCString(strBuf, DIALOGERRS_STRS, 22);			/* Play tempo percent must be between..." */
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
					}
					else {
						playTempoPercent = newPercent;
						done = TRUE;
					}
					break;
				case Cancel:
					done = TRUE;
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

