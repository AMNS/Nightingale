/* InstrDialog.c for Nightingale, by Jim Carr, revised by John Gibson, rev. for v.3.5 */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1987-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

/* The Instrument Dialog displays a List Manager list of instruments; selecting
one initializes its full and short name, range, and transposition.  This
information is obtained from STR# resource ID INSTR_STRS, each string of which
is an "instrument record". An instrument record consists of 14 colon-separated
fields that are all on one line (with no extra characters).
 
Here are descriptions and sizes of each field, with each field on a separate line
for readability:
  
 	name (32 chars max):
 	abreviation (12 chars max):
 
 	top of written range MIDI note no. (1-3 chars):
 	note letter name (1 uppercase char):
 	accidental (# or b or n [natural]):
 
 	transposition MIDI note no. showing where middle C sounds (1-3 chars):
 	note letter name (1 uppercase char):
 	accidental (# or b or n):
 
 	bottom of written range MIDI note no. (1-3 chars):
 	note letter name (1 uppercase char):
 	accidental (# or b or n):
 
	channel (1-2 chars):
	patch (1-3 chars):
	velocity (1-3 chars):
	OMS unique device ID (1-5 chars)
NB: There is currently no FreeMIDI support for this resource.
 
Here is a complete correct entry. N.B., all on one line!
 
	Contrabassoon:Contrabsn.:51:E:b:60:C:n:22:B:b:1:71:0:53
 
Either 'n' or <space> can be used for natural.
 
N.B.: It appears that if the MIDI note no. and the note/accidental disagree,
MIDI note no. 0 and C-natural are used instead--not good. */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "CarbonStubs.h"

#include "CoreMIDIDefs.h"

/* Local Prototypes */

static char * 				strcut(char *, char *, INT16);
static rangeHandle			ReadInstr(ListHandle, short, short);
static pascal Boolean		TheFilter(DialogPtr, EventRecord *, INT16 *);
static pascal Boolean		TheCMPMFilter(DialogPtr, EventRecord *, INT16 *);
static Boolean 				GetInstr(rangeHandle, char *);
static Boolean				MpCheck(PARTINFO *);
static void 					NoteErase(INT16, char, char, Boolean);
static void 					NoteDraw(INT16, char, char, Boolean);
static void					DrawTranspChar(Boolean, INT16);
static void 					DrawStaves(void);
static void 					DrawNoteNames(Boolean);
static void 					InitRange(rangeMaster *);
static void 					ShowInit(rangeMaster *);
static void 					InstrMoveRange(struct range *, INT16, INT16);
static void					ShowLSelect(DialogPtr, INT16);
static void 					ClickEraseRange(void);


/* -------------------------- ( FORMERLY iDialogModal.c) -------------------------- */

/* Dialog item numbers */
#define OKBTN			OK
#define CANCELBTN		Cancel
#define TOP_RAD		4
#define TRANS_RAD		5
#define BOT_RAD		6
#define INSTRNAME		8
#define INSTRABRV		10
#define LIST_DITL		11
#define UPBTN			12
#define DOWNBTN		13
#define CHANNEL_DI	16
#define PATCH_DI		17
#define V_BALANCE		18
#define STAFF_BOX		22

#define NOTE_NAME_BOX	23
#define OMS_OUTPUT_MENU 25				/* in OMS version only */
#define FMS_OUTPUT_MENU OMS_OUTPUT_MENU
#define FMS_PATCH_MENU 27				/* in FreeMIDI version only */

#define BTNOFF			0
#define BTNON			1

#define LIST_PARTS	20
#define LIST_OFFSET	15
#define ITEM_HEIGHT	16

#define UP				1
#define DOWN			-1

/* set of radio buttons ??There's no need for this struct; simplify some day! */
static struct radSet 
{
	unsigned char	defaultOn;
	unsigned char	nowOn;
} radSet;

/* list struct */
static struct instr_list
{
	Rect				instr_box;
	ListHandle		instr_name;
	Rect				d_rect;
	INT16				num_instr;
	ControlHandle	text_scroll;
	INT16				last_one;
} I;

static rangeHandle rangeHdl;
static rangeMaster master;	 
struct scaleTblEnt *noteInfo[T_TRNS_B];
static INT16 sLeft, sTop;

static Rect noteNameRect;					/* Valid in all cases, OMS or not */
static OMSDeviceMenuH omsOutputMenuH;

static unsigned long *origMPDevice;		/* can be either MIDIUniqueID, OMSUniqueID or fmsUniqueID */
static Rect deviceMenuBox, patchMenuBox;
static fmsUniqueID fmsDeviceID = noUniqueID;
static short fmsChannel = 0;
static Boolean fmsSendPatchChange = FALSE;

/* default for range */
static rangeMaster dfault =
{
	{
		{(char)108, 'C', ' '},
		{(char)60, 'C', ' '},
		{(char)24, 'C', ' '}
	},
	"Unnamed",
	"Un.",
	1,			/* channel */
	1,			/* patch */
	0,			/* velocity balance */
	0			/* device */
};

// ---------------------------------------------------------------------------------

const short 					kCMPopupID = 300;

static Rect						outputDeviceMenuBox;
static MenuHandle 			cmOutputMenuH;
static UserPopUp				cmOutputPopup;
static MIDIUniqueIDVector 	*cmVecDevices;

static MenuHandle CreateCMOutputMenu(DialogPtr dlog, UserPopUp *p, Rect *box, MIDIUniqueID device);

static Boolean ValidCMDeviceIndex(short idx) {
	return (idx >= 0 && idx <cmVecDevices->size());
}

static short GetCMDeviceIndex(MIDIUniqueID dev) {

	long j = 0;
	
	if (dev != NULL) {
		
		MIDIUniqueIDVector::iterator i = cmVecDevices->begin();
		
		for ( ; i!= cmVecDevices->end(); i++, j++) {
			if (dev == (*i))
				return j;
		}
	}
	
	return -1;
}

static MIDIUniqueID GetCMOutputDeviceID()
{
	short choice = cmOutputPopup.currentChoice-1;
	
	if (ValidCMDeviceIndex(choice)) {
		return cmVecDevices->at(choice);
	}
	return kInvalidMIDIUniqueID;
}

static MenuHandle CreateCMOutputMenu(DialogPtr dlog, UserPopUp *p, Rect *box, MIDIUniqueID device, INT16 item)
{

	Boolean popupOk = InitPopUp(dlog, p,
								item,			/* Dialog item to set p->box from */
								0,				/* Dialog item to set p->prompt from, or 0=set it empty */
								kCMPopupID,		/* CoreMidi popup ID */
								0				/* Initial choice, or 0=none */
								);
								
	if (!popupOk || p->menu==NULL)
		return NULL;
		
	cmVecDevices = new MIDIUniqueIDVector();
	long numItems = FillCMDestinationPopup(p->menu, cmVecDevices);

	short currChoice = GetCMDeviceIndex(device);

	if (ValidCMDeviceIndex(currChoice)) {
		ChangePopUpChoice(&cmOutputPopup, currChoice + 1);
	}

	return p->menu;
}



// ---------------------------------------------------------------------------------


#ifndef VIEWER_VERSION

INT16 CMInstrDialog(Document *doc, PARTINFO *mp, MIDIUniqueID *mpDevice)
{
	origMPDevice = (unsigned long*)mpDevice;
	return InstrDialog(doc, mp);
}


/* InstrDialog returns 1 on OK, 0 on Cancel or if an error occurs. */

INT16 InstrDialog(Document *doc, PARTINFO *mp)
{
	extern rangeHandle rangeHdl;	
	extern rangeMaster master, dfault;
	char fmtStr[256];
	
	DialogPtr	theDialog;
	GrafPtr		oldPort;
	Handle		instruments, hndl, itemHdl, radHdl;
	Str255		str, deviceStr;
	Rect			list_rect, itemBox, radBox;
	Point			cell_size;
	INT16			itemHit, theType, scratch, val;
	short			saveResFile;
	INT16			dialogOver;
	Boolean		gotValue;
	ModalFilterUPP	filterUPP;
	
	GetPort(&oldPort);

	filterUPP = NewModalFilterUPP(TheFilter);
	if (filterUPP == NULL) {
		MissingDialog(INSTRUMENT_DLOG);
		return(0);
	}

	if (useWhichMIDI == MIDIDR_CM) {
		theDialog = GetNewDialog(CM_INSTRUMENT_DLOG, 0L, BRING_TO_FRONT);
		if (!theDialog) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(CM_INSTRUMENT_DLOG);
			return(0);
		}
	}
	else {
		theDialog = GetNewDialog(INSTRUMENT_DLOG, 0L, BRING_TO_FRONT);
		if (!theDialog) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(INSTRUMENT_DLOG);
			return(0);
		}
	}

	SetPort(GetDialogWindowPort(theDialog));
	
	saveResFile = CurResFile();
	UseResFile(setupFileRefNum);

	/* Set up rect and sizes for list */
	
	instruments = GetResource('STR#', INSTR_STRS);
	if (!instruments) {
		MayErrMsg("InstrDialog: unable to get instrument definition STR# resource");
		itemHit = 0;	/* forces return(0) */
		goto broken;
	}

	I.num_instr = *(INT16 *)(*instruments);
	
	GetDialogItem(theDialog, LIST_DITL, &scratch, &hndl, &I.instr_box);
	InsetRect(&I.instr_box, 1, 1);
	
	I.instr_box.right -= LIST_OFFSET;
	SetRect(&list_rect, 0, 0, 1, I.num_instr);
	SetPt(&cell_size, I.instr_box.right - I.instr_box.left, ITEM_HEIGHT);
	
	/* Create the list */
	I.instr_name = LNew(&I.instr_box, &list_rect, cell_size, 0, GetDialogWindow(theDialog),
				FALSE, FALSE, FALSE, TRUE);
	if (I.instr_name==NULL) {
		itemHit = 0;
		goto broken;
	}
				
	(*I.instr_name)->selFlags = lOnlyOne;
	InsetRect(&I.instr_box, -1, -1);
	
	/* 
	 * Get list data into rangeHdl structure. If a NULL handle comes back, return and
	 * pass back an error code. 
	 */
	 
	if (ReadInstr(I.instr_name, I.num_instr, INSTR_STRS) == (rangeHandle)NULL) {
		NoMoreMemory();
		itemHit = 0;	/* forces return(0) */
		goto broken;
	}

	if (useWhichMIDI==MIDIDR_CM) {
		/* create the popup menu of available output devices */
		GetDialogItem(theDialog, OMS_OUTPUT_MENU, &scratch, &hndl, &deviceMenuBox);
		cmOutputMenuH = CreateCMOutputMenu(theDialog, &cmOutputPopup, &deviceMenuBox, NULL, OMS_OUTPUT_MENU);
	}

	/* Added following to genericize noteName placement as part of USING_OMS changes */
	GetDialogItem(theDialog, NOTE_NAME_BOX, &scratch, &hndl, &noteNameRect);

	radSet.defaultOn = TOP_RAD;
	
	if (radSet.nowOn == 0) radSet.nowOn = radSet.defaultOn;
	GetDialogItem(theDialog, radSet.nowOn, &theType, &radHdl,&radBox);
	
	SetControlValue((ControlHandle)radHdl, BTNON);

	GetDialogItem(theDialog, STAFF_BOX, &scratch, &hndl, &itemBox);
	sLeft = itemBox.left;
	sTop = itemBox.top;
	 
	if (MpCheck(mp))
	{
		/* Assign MIDInote data. Notice this code assumes MIDI_MIDDLE_C is an integral
		 * number of octaves, i.e., a multiple of 12--a pretty safe assumption! */
		
		master.ttb[TOP].midiNote = mp->hiKeyNum;
		master.ttb[TRANS].midiNote = mp->transpose+MIDI_MIDDLE_C;			
		master.ttb[BOT].midiNote = mp->loKeyNum;

		/* initial note name and accidental set here, 
		   sanity checked in InitRange() */

		master.ttb[TOP].name = mp->hiKeyName;
		master.ttb[TRANS].name = mp->tranName;			
		master.ttb[BOT].name = mp->loKeyName;
		
		master.ttb[TOP].acc = mp->hiKeyAcc;
		master.ttb[TRANS].acc = mp->tranAcc;			
		master.ttb[BOT].acc = mp->loKeyAcc;
				
		/* instrument name and abbrv. */		
		GoodStrncpy(master.name, mp->name, (unsigned long)NM_SIZE - 1);
		GoodStrncpy(master.abbr, mp->shortName, (unsigned long)AB_SIZE - 1);
		
		/* channel, patch number and velocity balance */
		master.channel = mp->channel;
		master.patchNum = mp->patchNum;
		master.velBalance = mp->partVelocity;

		/* set unique device ID for OMS
			See code in GetOMSPartPlayInfo for a similar situation.  -JGG, 7/23/00 */
		if (useWhichMIDI == MIDIDR_CM) {
			/* Validate device / channel combination. */
			if (CMTransmitChannelValid(*origMPDevice, (INT16)(mp->channel)))
				master.cmDevice = *origMPDevice;
			else
				master.cmDevice = config.cmDefaultOutputDevice;
			/* It's possible our device has changed, so validate again. */
			if (CMTransmitChannelValid(master.cmDevice, (INT16)(mp->channel)))
				master.channel = mp->channel;
			else
				master.channel = config.cmDefaultOutputChannel;

			if ((master.channel != mp->channel) || master.cmDevice != *origMPDevice)
				master.changedDeviceChannel = 1;
		}
	}
	else {
		GetIndCString(strBuf, INSTRERRS_STRS, 1);					/* "Illegal instrument description; will use default." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		master = dfault;
	}	
	
	InitRange(&master);
	CenterWindow(GetDialogWindow(theDialog), 100);
	ShowWindow(GetDialogWindow(theDialog));
	ArrowCursor();
	
	PutDlgWord(theDialog, V_BALANCE, master.velBalance, FALSE);

	CToPString(strcpy((char *)str, master.abbr));
	PutDlgString(theDialog, INSTRABRV, str, FALSE);

	if (useWhichMIDI==MIDIDR_CM && cmOutputMenuH!=NULL) {
		GetIndString(deviceStr, MIDIPLAYERRS_STRS, 16);
		short currChoice = GetCMDeviceIndex(master.cmDevice);

		if (ValidCMDeviceIndex(currChoice)) {
			ChangePopUpChoice(&cmOutputPopup, currChoice + 1);
		}
		else {
			ChangePopUpChoice(&cmOutputPopup, 1);
		}
		DrawPopUp(&cmOutputPopup);
	}

	CToPString(strcpy((char *)str, master.name));
	PutDlgString(theDialog, INSTRNAME, str, TRUE);			/* Do last so we can select it */
		
	dialogOver = 0;
	while (dialogOver==0)
	{
		do
		{	
			ModalDialog(filterUPP, &itemHit);
			if (itemHit<1) continue;									/* in case returned by filter */
			GetDialogItem(theDialog, itemHit, &theType, &itemHdl, &itemBox);
			switch(theType)
			{
				case ctrlItem+btnCtrl:
					switch(itemHit) {
						case OKBTN:
							dialogOver = OKBTN;
							break;
						case CANCELBTN:
							dialogOver = CANCELBTN;
							break;
					}
					break;
				
				case radCtrl+ctrlItem:				/* radio button was hit */
					GetDialogItem(theDialog, radSet.nowOn, &theType,
						&radHdl, &radBox);										/* get handle to button now on */
					SetControlValue((ControlHandle)radHdl, BTNOFF);		/* old button off */
					SetControlValue((ControlHandle)itemHdl, BTNON);		/* new button on */
					radSet.nowOn = itemHit;										/* update structure */
					break;
			}
		} while (!dialogOver);
	
		if (dialogOver==OKBTN) {
			if (useWhichMIDI==MIDIDR_CM) {
				gotValue = GetDlgWord(theDialog, CHANNEL_DI, &val);
				master.cmDevice = GetCMOutputDeviceID();
				if ((gotValue && val != mp->channel) || master.cmDevice != *origMPDevice)
					master.changedDeviceChannel = 1;
				if (gotValue && CMTransmitChannelValid(master.cmDevice, val)) {
					*origMPDevice = master.cmDevice;
					mp->channel = val;
					master.validDeviceChannel = 1;
				}
				else {
					GetIndCString(strBuf, INSTRERRS_STRS, 8);    	/* "Channel number and device are inconsistant" */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					GetDialogItem(theDialog, OKBTN, &theType, &hndl, &itemBox);
					HiliteControl((ControlHandle)hndl,0);			/* so OK btn doesn't stay hilited */
					dialogOver = 0;
					continue;										/* Loop back for correction */
				}					
			}
			else {
				/* Copy modified channel, if valid. If not, loop back. */
				gotValue = GetDlgWord(theDialog, CHANNEL_DI, &val);
				if (gotValue && val > 0 && val <= MAXCHANNEL)
					mp->channel = val;
				else {
					GetIndCString(fmtStr, INSTRERRS_STRS, 4);    /* "Channel number must be between 1 and %d." */
					sprintf(strBuf, fmtStr, MAXCHANNEL); 
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					GetDialogItem(theDialog, OKBTN, &theType, &hndl, &itemBox);
					HiliteControl((ControlHandle)hndl,0);			/* so OK btn doesn't stay hilited */
					dialogOver = 0;
					continue;									/* Loop back for correction */
				}
			}

			gotValue = GetDlgWord(theDialog, V_BALANCE, &val);
			if (gotValue && val >= -127 && val <= 127)							/* MAX_VELOCITY */
				mp->partVelocity = val;
			else {
				GetIndCString(strBuf, INSTRERRS_STRS, 6);    /* "Velocity balance must be between -127 and 127." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				GetDialogItem(theDialog, OKBTN, &theType, &hndl, &itemBox);
				HiliteControl((ControlHandle)hndl,0);			/* so OK btn doesn't stay hilited */
				dialogOver = 0;
				continue;										/* Loop back for correction */
			}
													
			/*
			 * copy modified range (top & bottom), transposition,
			 * notename, accidental, instrument name and abbrv.
			 */
			 
			mp->hiKeyNum = master.ttb[TOP].midiNote;
			mp->transpose = master.ttb[TRANS].midiNote-MIDI_MIDDLE_C;			
			mp->loKeyNum = master.ttb[BOT].midiNote;
		
			mp->hiKeyName  = master.ttb[TOP].name;
			mp->tranName = master.ttb[TRANS].name;			
			mp->loKeyName = master.ttb[BOT].name;
				
			mp->hiKeyAcc = master.ttb[TOP].acc;
			mp->tranAcc = master.ttb[TRANS].acc;			
			mp->loKeyAcc = master.ttb[BOT].acc;
						
			GetDlgString(theDialog, INSTRNAME, str);		/* Get strings directly from edit fields */
			GoodStrncpy(mp->name, PToCString(str), (unsigned long)NM_SIZE - 1);
			GetDlgString(theDialog, INSTRABRV, str);
			GoodStrncpy(mp->shortName, PToCString(str), (unsigned long)AB_SIZE - 1);
			
			radSet.nowOn = radSet.defaultOn;				/* so Top radio selected next time */
			
		}	/* if (dialogOver==OKBTN) */
	}		/* while (dialogOver==0) */
	
	DisposeHandle((Handle)rangeHdl);
	
	if (useWhichMIDI==MIDIDR_CM)
	{
		if (master.validDeviceChannel && master.changedDeviceChannel) {
			/* Ask user if they want to save the entered device/channel/patch as default
			 * for instrument, or... could add a new instrument type (need more flags
			 * to know when this has happened).  If so, then pack up a new instrument
			 * string and either insert/append a new instrument or overlay an existing one.
			 */
		}
		if (cmVecDevices != NULL) {
			delete cmVecDevices;
			cmVecDevices = NULL;
		}
		DisposePopUp(&cmOutputPopup);
	}

broken:
	UseResFile(saveResFile);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(theDialog);
	SetPort(oldPort);
		
	return (itemHit == OK? 1 : 0);
}


static pascal Boolean TheFilter(DialogPtr theDialog, EventRecord *theEvent, INT16 *itemHit)
{		
	GrafPtr			oldPort;
	ControlHandle 	cntrlHndl;
	Point 			mouseLoc;
	Handle			hndl, upBtnHdl, dwnBtnHdl;
	Rect			box, upBox, dwnBox;
	short			type, ans=0;
	Boolean			filterVal = FALSE;
	Cell 			lSelection;
	INT16			range, part;
	short			omsMenuItem;
	
	switch(theEvent->what)
	{
		case updateEvt:
			if ((WindowPtr)theEvent->message == GetDialogWindow(theDialog))
			{
				GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
				BeginUpdate(GetDialogWindow(theDialog));
				
				UpdateDialogVisRgn(theDialog);
				FrameRect(&I.instr_box);
				LUpdateDVisRgn(theDialog, I.instr_name);
				DrawStaves();
				ShowInit(&master);
				FrameDefault(theDialog,OKBTN,TRUE);
				if (useWhichMIDI==MIDIDR_CM && cmOutputMenuH!=NULL) {
					DrawPopUp(&cmOutputPopup);
				}
				EndUpdate(GetDialogWindow(theDialog));
				SetPort(oldPort);
			}
			break;
			
		case mouseDown:
			mouseLoc = theEvent->where;
			GlobalToLocal(&mouseLoc);
			
				/* If we've hit the OK or Cancel button, bypass the rest of the control stuff below. */
			GetDialogItem (theDialog, OKBTN, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	break;
			GetDialogItem (theDialog, CANCELBTN, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	break;

			if (useWhichMIDI==MIDIDR_CM && cmOutputMenuH != NULL) {
				if (PtInRect(mouseLoc, &deviceMenuBox)) {
					if (ans = DoUserPopUp(&cmOutputPopup)) {
						*itemHit = OMS_OUTPUT_MENU;
					}
					break;
				}
			}

			part = FindControl(mouseLoc, GetDialogWindow(theDialog), &cntrlHndl);
						
			if (part == kControlButtonPart)
			{					
				/* Code in "do" loop below needs this switch to set "range" local var. */
				switch(radSet.nowOn)
				{
					case TOP_RAD:
						range = TOP; break;
					case TRANS_RAD:
						range = TRANS; break;
					case BOT_RAD:
						range = BOT; break;
					default:
						range = TOP; break;
				}
				
				GetDialogItem (theDialog, UPBTN, &type, &upBtnHdl, &upBox);
				GetDialogItem (theDialog, DOWNBTN, &type, &dwnBtnHdl, &dwnBox);

				do
				{	
					if (PtInRect(mouseLoc, &upBox))
					{
						HiliteControl((ControlHandle)upBtnHdl, BTNON);
						InstrMoveRange(&master.ttb[range], range, UP);
						/* in case notes move too fast */
						SleepTicks(4L);
					}
					else if (PtInRect(mouseLoc, &dwnBox))
					{
						HiliteControl((ControlHandle)dwnBtnHdl, BTNON);
						InstrMoveRange(&master.ttb[range], range, DOWN);
						/* in case notes move too fast */
						SleepTicks(4L);				
					}
				} while (Button());
					/* Buttons get un-hilited automatically */
			}
					
			if (part >= LIST_PARTS)
			{
				if (cntrlHndl == (*I.instr_name)->vScroll)
				{
					/* we are in the list of instruments */
					LClick(mouseLoc, theEvent->modifiers, I.instr_name);
				}
			}
			
			else if (PtInRect(mouseLoc, &(*I.instr_name)->rView))
			{ 
				LClick(mouseLoc, theEvent->modifiers, I.instr_name);
				lSelection.h = lSelection.v = 0;
				LGetSelect(TRUE, &lSelection, I.instr_name);
				ShowLSelect(theDialog, lSelection.v);
				master.changedDeviceChannel = 1;
			}
			break;
			
		case autoKey:
		case keyDown:
			if (DlgCmdKey(theDialog, theEvent, itemHit, FALSE))
				return TRUE;
			break;
			
		case activateEvt:
			LActivate((Boolean)TRUE, I.instr_name);
			break;
	}
		
	return(filterVal);
}


/* Set text boxes and range graphic to instrument from List selection */

static void ShowLSelect(DialogPtr theDialog, INT16 cell)
{
	extern rangeHandle rangeHdl;
	extern rangeMaster master;
	rangeMaster *selectedInstr;
	Str255 theText, deviceStr;
	INT16 r;
	
	if (cell < 0) return;								/* minimal sanity check */
			
	MoveHHi((Handle)rangeHdl);							/* compact heap, if possible */
	HLock((Handle)rangeHdl);							/* remember to unlock below!! */
	
	/* get data refered to by cell click from heap	*/
	selectedInstr = *rangeHdl + cell;
	
	/*
	 * Initialize master rangeMaster struct and use it to set current range display.
	 * Struct rangeMaster master has values that change for each click in the list.
	 */
	 
	ClickEraseRange();
	
	for (r = 0; r < T_TRNS_B; r++)
	{
		master.ttb[r].midiNote = selectedInstr->ttb[r].midiNote;
		master.ttb[r].name = selectedInstr->ttb[r].name;
		master.ttb[r].acc = selectedInstr->ttb[r].acc;
	}
			 
	strcpy(master.name, selectedInstr->name);
	strcpy(master.abbr, selectedInstr->abbr);
	
	master.channel = selectedInstr->channel;
	master.patchNum = selectedInstr->patchNum;
	master.velBalance = selectedInstr->velBalance;

	InitRange(&master);
	
	/* put new values and strings in the text boxes */
	PutDlgWord(theDialog, CHANNEL_DI, master.channel, FALSE);
	PutDlgWord(theDialog, PATCH_DI, master.patchNum, FALSE);
	PutDlgWord(theDialog, V_BALANCE, master.velBalance, FALSE);

	CToPString(strcpy((char *)theText, master.abbr));
	PutDlgString(theDialog, INSTRABRV, theText, FALSE);
	CToPString(strcpy((char *)theText, master.name));
	PutDlgString(theDialog, INSTRNAME, theText, TRUE);			/* Do last so we can select it */
			
	HUnlock((Handle)rangeHdl);
}

#endif /* not VIEWER_VERSION */


/* -------------------------- ( FORMERLY iDialogMove.c) -------------------------- */


#define HORIZONTAL			40
#define NOTEHEAD				(char)MCH_quarterNoteHead
#define MUSICFONT_SIZE		18			/* Music font size to use */
#define ACCIDENTAL_OFFSET	6			/* pixels from acc H position to note H position */
#define C_LINE_VPOS			99				
#define TRANS_OFFSET			-15		/* horiz offset of transpose note from extrema */

#define STAVES					2
#define LINES					5
#define V_SPACE 				5
#define NOTE_SPACE			5			/* pixels between staff lines */

#define TOP_LEDGER			4			/* vertical */
#define LEDGERTOPMAX			14
#define TOP_TRANS_LEDGER	7			/* number of ledger lines above staff for transpose */
#define MAX_TRANS_MIDI		103		/* corresponding max MIDI note num for transpose */
#define BOT_LEDGER			129		/* vertical */
#define LEDGERBOTMAX			12
#define BOT_TRANS_LEDGER	7			/* number of ledger lines below staff for transpose */
#define MIN_TRANS_MIDI		17			/* corresponding min MIDI note num for transpose */
#define START					74			/* vertical */
#define STAFF_START			3			/* horizontal */
#define STAFF_END				65
#define LEDG_RANGE_STRT		45
#define LEDG_TRANS_STRT		30
#define LEDGER_WID			7
#define CLEF_START			6
#define TREB_V					93	
#define BASS_V					124		/* vertical */
#define TREB_CLEF				'&'
#define BASS_CLEF				'?'

#define TRANS_NOT_WID		6			/* dimensions of transpose 'note' */
#define TRANS_NOT_HT			5

#define MIN_RANGE				3			/* minimum range in semitones */
	

#ifndef VIEWER_VERSION

static void DrawStaves()
{
	INT16	staff, line, yStart;
	
	TextFont(sonataFontNum);
	TextSize(MUSICFONT_SIZE);
	TextMode(srcOr);
	
	/* ledger lines above */
	for (line = 0, yStart = sTop+TOP_LEDGER; line < LEDGERTOPMAX;
							line++, yStart += NOTE_SPACE)
	{
		if (line >= LEDGERTOPMAX - TOP_TRANS_LEDGER) {
			MoveTo(sLeft+LEDG_TRANS_STRT, yStart);
			LineTo(sLeft+LEDG_TRANS_STRT+LEDGER_WID, yStart);
		}
		MoveTo(sLeft+LEDG_RANGE_STRT, yStart);
		LineTo(sLeft+LEDG_RANGE_STRT+LEDGER_WID, yStart);
	}
	
	/* staves, clefs, and "system connect" */
	for (staff = 0, yStart = sTop+START; staff < STAVES; staff++, yStart += V_SPACE)
	{
		for (line = 0; line < LINES; line++, yStart += NOTE_SPACE)
		{
			MoveTo(sLeft+STAFF_START, yStart);
			LineTo(sLeft+STAFF_END, yStart); 
		}
	}
	
	MoveTo(sLeft+CLEF_START, sTop+TREB_V);
 	DrawChar(TREB_CLEF);
	MoveTo(sLeft+STAFF_START, sTop+TREB_V);
 	
 	MoveTo(sLeft+CLEF_START, sTop+BASS_V);
 	DrawChar(BASS_CLEF);
 	MoveTo(sLeft+STAFF_START, sTop+BASS_V);
 	
 	MoveTo(sLeft+STAFF_START, sTop+START);
 	LineTo(sLeft+STAFF_START, sTop+BASS_V); 
 	
	/* middle C ledger lines */
 	MoveTo(sLeft+LEDG_TRANS_STRT, sTop+C_LINE_VPOS);
 	LineTo(sLeft+LEDG_TRANS_STRT+LEDGER_WID, sTop+C_LINE_VPOS);
 	MoveTo(sLeft+LEDG_RANGE_STRT, sTop+C_LINE_VPOS);
 	LineTo(sLeft+LEDG_RANGE_STRT+LEDGER_WID, sTop+C_LINE_VPOS);

	/* ledger lines below */
	for (line = 0, yStart = sTop+BOT_LEDGER; line < LEDGERBOTMAX;
							line++, yStart += NOTE_SPACE)
	{
		if (line < BOT_TRANS_LEDGER) {
			MoveTo(sLeft+LEDG_TRANS_STRT, yStart);
			LineTo(sLeft+LEDG_TRANS_STRT+LEDGER_WID, yStart); 
		}
		MoveTo(sLeft+LEDG_RANGE_STRT, yStart);
		LineTo(sLeft+LEDG_RANGE_STRT+LEDGER_WID, yStart);
	}	
}


static void InstrMoveRange(
							struct range *t,	/* where we find current range */	
							INT16 i,				/* index into noteInfo[] */
							INT16 direction	/* up == 1, down == -1 */
							)
{

	extern struct scaleTblEnt scaleTbl[];
	extern struct scaleTblEnt *noteInfo[3];
	extern rangeMaster master;
	extern INT16 octTbl[];
	struct scaleTblEnt *oldPtr;
	INT16 oct, m;
		
	/* Check range bounds and top/bottom overlap; return when reached.
		Handles different number of ledger lines for transpose note. */	
	if (direction == UP && (t->midiNote == MAXMIDI ||
				(radSet.nowOn == TRANS_RAD && t->midiNote == MAX_TRANS_MIDI)) ) {
			return;
	}
	if (direction == DOWN && (t->midiNote == MINMIDI ||
				(radSet.nowOn == TRANS_RAD && t->midiNote == MIN_TRANS_MIDI)) ) {
			return;
	}

	if (radSet.nowOn == TOP_RAD) {				/* prevent overlap */
		if ((t->midiNote == master.ttb[BOT].midiNote + MIN_RANGE)
														&& direction == DOWN) return; }
	if (radSet.nowOn == BOT_RAD) {
		if ((t->midiNote == master.ttb[TOP].midiNote - MIN_RANGE)
														&& direction == UP) return; }
	TextFont(sonataFontNum);
	TextSize(MUSICFONT_SIZE);
	
	oldPtr = noteInfo[i];
	
	/* erase only the currently active note to avoid flashing */
	switch (radSet.nowOn) {
		case TOP_RAD:
			m = TOP; break;
		case TRANS_RAD:
			m = TRANS; break;
		case BOT_RAD:
			m = BOT; break; }

	oct = master.ttb[m].midiNote / TWELVE;
	NoteErase(octTbl[oct] + noteInfo[m]->v_pos, NOTEHEAD, noteInfo[m]->n.acc,
		(m == TRANS ? TRUE : FALSE) );	/* last param for offsetting transpose note from extrema */
	
	/* we are moving an external pointer here! */ 
	noteInfo[i] += direction; 							/* here is where we move! */
	
	/* if needed, wrap newPtr around in scaleTbl regardless of direction */
	/* we are moving an external pointer here! */
	 
	if (noteInfo[i] < scaleTbl)
		noteInfo[i] = &scaleTbl[MAXSCALE];
	else if (noteInfo[i] > &scaleTbl[MAXSCALE])
		noteInfo[i] = scaleTbl;
	
	if (oldPtr->n.midiNote != noteInfo[i]->n.midiNote)
	{
		t->midiNote += direction;			/* new MIDI note in master struct */
	}
	/* name and acc may change even if MIDI note does not! */
	t->acc = noteInfo[i]->n.acc;
	t->name = noteInfo[i]->n.name;
	 
	DrawStaves();
		
	/* draw all */
	for (m = 0; m < T_TRNS_B; m++)
	{
		oct = master.ttb[m].midiNote / TWELVE;	
		NoteDraw(octTbl[oct] + noteInfo[m]->v_pos, NOTEHEAD, noteInfo[m]->n.acc,
			(m == TRANS ? TRUE : FALSE) );	/* last param for offsetting transpose note from extrema */
	}
	DrawNoteNames(FALSE);		/* update name and accidental for current note only */
	
}


static void ClickEraseRange()
{
	extern struct scaleTblEnt scaleTbl[];
	extern struct scaleTblEnt *noteInfo[3];
	extern rangeMaster master;
	extern INT16 octTbl[];
	
	INT16 oct, m;

	TextFont(sonataFontNum);
	TextSize(MUSICFONT_SIZE);	
	
	/* erase all */
	for (m = 0; m < T_TRNS_B; m++)
	{
		oct = master.ttb[m].midiNote / TWELVE;
		NoteErase(octTbl[oct] + noteInfo[m]->v_pos, NOTEHEAD, noteInfo[m]->n.acc,
			(m == TRANS ? TRUE : FALSE));	/* last param is kludge to offset transp from extrema */
	}
	DrawStaves();
}


static void NoteDraw(
					INT16 v,
					char /*noteChar*/,			/* ignored for now */
					char accChar,
					Boolean isTrans		/* for horiz. offsetting transpose note from extrema */
					)
{
	INT16 h_pos;
	
	h_pos = (isTrans ? sLeft+HORIZONTAL+TRANS_OFFSET : sLeft+HORIZONTAL);
	TextMode(srcOr);
	
	MoveTo(h_pos, sTop+v);
 	DrawPaddedChar(accChar);
 		
 	MoveTo(h_pos+ACCIDENTAL_OFFSET, sTop+v);
 	if (isTrans) DrawTranspChar(TRUE, v);
	else DrawChar(NOTEHEAD);
}


void NoteErase(
				INT16 v,
				char /*noteChar*/,		/* ignored for now */
				char accChar,
				Boolean isTrans		/* for horiz. offsetting transpose note from extrema */
				)
{
	INT16 h_pos;
	
	h_pos = (isTrans ? sLeft+HORIZONTAL+TRANS_OFFSET : sLeft+HORIZONTAL);
	TextMode(srcBic);

	MoveTo(h_pos, sTop+v);
 	DrawPaddedChar(accChar);
 		
 	MoveTo(h_pos+ACCIDENTAL_OFFSET, sTop+v);
 	if (isTrans) DrawTranspChar(FALSE, v);
	else DrawChar(NOTEHEAD);
}

/* Draws or erases the square notehead for transpose "note" */

void DrawTranspChar(
				Boolean draw,			/* TRUE: draw, FALSE: erase */
				INT16 vPos)
{
	Rect trnspNoteRect;
	
	SetRect(&trnspNoteRect,sLeft+LEDG_TRANS_STRT+1, sTop+vPos-3,
		(sLeft+LEDG_TRANS_STRT+1)+TRANS_NOT_WID, sTop+(vPos-3)+TRANS_NOT_HT);
	if (draw) PaintRect(&trnspNoteRect);
	else EraseRect(&trnspNoteRect);
}

			
static void ShowInit(rangeMaster *M)
{
	extern struct scaleTblEnt *noteInfo[3];
	extern INT16 octTbl[];
	INT16 oct, i;
	
	TextFont(sonataFontNum);
	TextSize(MUSICFONT_SIZE);
	
	for (i = 0; i < T_TRNS_B; i++)
	{
		oct = M->ttb[i].midiNote / TWELVE;	
		NoteDraw(octTbl[oct] + noteInfo[i]->v_pos, NOTEHEAD,
				noteInfo[i]->n.acc, (i==TRANS ? TRUE : FALSE));	/* last param is kludge to offset trans from extrema */
	}
 	DrawNoteNames(TRUE);	
}


#define NAME_HORIZONTAL	441
#define NAME_V				85
#define A_V					82
#define NV_OFFSET			25
#define A_OFFSET			10
	
/* Put the note names and accidentals in the dialog (naturals not shown). */

static void DrawNoteNames(
					Boolean drawAll)		/* If false, draw only currently moving note. */
{
	
	extern struct scaleTblEnt *noteInfo[3];
	extern rangeMaster master;
	static char acc[T_TRNS_B];		/* holds current accidentals */
	static char name[T_TRNS_B];	/* holds current note names */
	INT16 i;
	long trans;
	static char tranStr[20];
	short nameHorizontal;
	short nameAccV;
	short nameV;
	
	/* Determine placement relative to dialog item so different dialogs can use this routine. */

	nameHorizontal = noteNameRect.left;
	nameAccV = noteNameRect.top+MUSICFONT_SIZE-6;
	nameV = noteNameRect.top+12+1;

	/*
	 * This function repeatedly switches between the music font (for accidentals) and
	 *	the system font (for note names). It also repeatedly switches between drawing
	 *	in "BIC" mode (for erasing) and "OR" mode (for actual drawing.)
	 */
	 
	/* Erase old accidentals, if necessary. */
	TextFont(sonataFontNum);
	TextSize(MUSICFONT_SIZE);
	TextMode(srcBic);
	
	if (radSet.nowOn == TOP_RAD || drawAll) {
		MoveTo(nameHorizontal + A_OFFSET, nameAccV);
		DrawPaddedChar(acc[TOP]); }
	if (radSet.nowOn == BOT_RAD || drawAll) {
		MoveTo(nameHorizontal + A_OFFSET, nameAccV + (2*NV_OFFSET));
		DrawPaddedChar(acc[BOT]); }
	
	/* Erase old note names and transpose interval, if necessary. */	
	TextFont(systemFont);
	TextSize(12);
	
	if (radSet.nowOn == TOP_RAD || drawAll) {
		MoveTo(nameHorizontal, nameV);
		DrawChar(name[TOP]); }
	if (radSet.nowOn == TRANS_RAD || drawAll) {
		MoveTo(nameHorizontal, nameV + NV_OFFSET);
		DrawCString(tranStr); }
	if (radSet.nowOn == BOT_RAD || drawAll) {
		MoveTo(nameHorizontal, nameV + (2*NV_OFFSET));
		DrawChar(name[BOT]); }
	
	/* Now we will write the new stuff, in music font */	
	TextFont(sonataFontNum);
	TextSize(MUSICFONT_SIZE);
	TextMode(srcOr);
	
	/* N.B., noteInfo array holds addresses of current table settings */	
	MoveTo(nameHorizontal + A_OFFSET, nameAccV);
	DrawPaddedChar(noteInfo[TOP]->n.acc);
	MoveTo(nameHorizontal + A_OFFSET, nameAccV + (2*NV_OFFSET));
	DrawPaddedChar(noteInfo[BOT]->n.acc);
	
	TextFont(systemFont);
	TextSize(12);
	
	MoveTo(nameHorizontal, nameV);
	DrawChar(noteInfo[TOP]->n.name); 
	/* Draw transpose semitone offset from middle C */
	trans = master.ttb[TRANS].midiNote - MIDI_MIDDLE_C;
	sprintf(tranStr, "%ld", trans); 
	MoveTo(nameHorizontal, nameV + NV_OFFSET);
	DrawCString(tranStr); 
	MoveTo(nameHorizontal, nameV + (2*NV_OFFSET));
	DrawChar(noteInfo[BOT]->n.name);
	
	/* Save current settings for erasing next time */	
	for (i = 0; i < T_TRNS_B; i++) {
		acc[i] = noteInfo[i]->n.acc;
		name[i] = noteInfo[i]->n.name;
	}
}

#endif /* not VIEWER_VERSION */

/* -------------------------- ( FORMERLY iDialogRange.c) -------------------------- */

/* 
 * This table gives the vertical position of C for a given octave.
 */

INT16 octTbl[MAXOCT+1] = {	185,
							170, /* -18= */
							153, /* -18= */
							135, /* -18= */
							118, /* -18= */
							100, /* -17= */
							 83, /* -18= */ 
							 65, /* -17= */ 
							 48, /* -17= */
							 30, /* -17= */ 
							 13, /* -18= */
						 	 -5	};
/*
 *	This table lets us keep track of only one octave (and simple enharmonics:
 * sorry harpists, no Cb!) as midinote, name, accidental, and vertical 
 * pixel offset from current C octave position. See also octTbl[] above.
 */
 
struct scaleTblEnt scaleTbl[MAXSCALE+1] = 
{
	{ {(char)0,'C', ' ',}, 0},
	{ {(char)1,'C', '#',}, 0},
	{ {(char)1,'D', 'b',}, -3},
	{ {(char)2,'D', ' ',}, -3},
	{ {(char)3,'D', '#',}, -3},
	{ {(char)3,'E', 'b',}, -5},
	{ {(char)4,'E', ' ',}, -5},
	{ {(char)5,'F', ' ',}, -8},
	{ {(char)6,'F', '#',}, -8},
	{ {(char)6,'G', 'b',}, -10},
	{ {(char)7,'G', ' ',}, -10},
	{ {(char)8,'G', '#',}, -10},
	{ {(char)8,'A', 'b',}, -13},
	{ {(char)9,'A', ' ',}, -13},
	{ {(char)10,'A', '#',}, -13},
	{ {(char)10,'B', 'b',}, -15},
	{ {(char)11,'B', ' ',}, -15}
};


static rangeHandle ReadInstr(
						ListHandle iList,		/* handle for list struct */
						short i_tot,			/* number of instruments in resource */
						short rsrc_num			/* which resource */
						)
{
	rangeMaster *tmpRngPtr;
	INT16 i;
	char anInstr[256];
	Point theCell;
	Boolean badInstr=FALSE;
	
	/*
	 * Allocate array of rangeMaster structs for data from resource.
	 * This stuff needs to sit in memory as long as the dialog is active.
	 *
	 * We need some serious error checking on this call! For now, inform
	 *	caller that allocation failed by returning a NULL rangeHandle.
	 * The handle is then locked until all initializing is done.
	 */
	 
	rangeHdl = (rangeHandle)
						NewHandle(i_tot * (sizeof(rangeMaster)));
	
	if (rangeHdl == (rangeHandle)NULL)
		return((rangeHandle)NULL);
		
	MoveHHi((Handle)rangeHdl);
	HLock((Handle)rangeHdl);								/* remember to unlock!! */
		
	tmpRngPtr = *rangeHdl;									/* save start of struct array */
	
	/*
	 * For each instrument in the resource, get a string from the resource and
	 * convert it to a C string. If we can parse it, store in struct array.
	 * Then set it in list and move on to next available (blank) struct in array.
	 */
		 
	for (i = 0; i < i_tot; i++) 
	{
		GetIndCString(anInstr, rsrc_num, i + 1);
	 	if (GetInstr(rangeHdl, (char *)anInstr)) 
	 	{
			SetPt(&theCell, 0, i);
			LSetCell((*rangeHdl)->name, strlen((*rangeHdl)->name),
						theCell, iList);
			(*rangeHdl)++;										/* incr to next struct */
		}
		else badInstr = TRUE;
	}
	
	if (badInstr) {
		GetIndCString(strBuf, INSTRERRS_STRS, 2);    /* "One or more illegal instrument description(s) in list; will ignore them." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	
	*rangeHdl = tmpRngPtr;									/* restore master pointer to start of array */
	
	HUnlock((Handle)rangeHdl);								/* unlock it */
	
	/*
	 * No List item selected at first unless this is turned on...Don says no.
	 * 
	 * SetPt(&theCell, 0, 0);
	 * LSetSelect((Boolean)TRUE, theCell, iList); 
	 */
	
	LSetDrawingMode((Boolean)TRUE, iList);
	return(rangeHdl);
}


/* Get information on the given instrument from the resource; try to parse it and
store its values into the given rangeMaster. Return TRUE if we succeed, FALSE if
there's a problem. */

static Boolean GetInstr(rangeHandle rHdl, char *strPtr)
{
	char tmp_str[256];
	INT16 i;
	
	/*
	 * the record layout expected from the resource is as follows:
	 *
	 * instrname:abrv:topMIDI:name:acc:transMIDI:name:acc:botMIDI:name:acc:chan:patch:velBalance
	 * (and for OMS) :device
	 *
	 * Where instrname and abrv are the name and abreviated name of the instrument
	 * up to NM_SIZE - 1 or  AB_SIZE -1, and [top|trans|bot]MIDI is the MIDI note
	 * value of the top of range, transposition, and bottom of range, respectively.
	 *
	 * Due to the fact some ranges are expressed in flats and some (such as
	 * harp and bass trombone) in sharps, we need the note name (C, D, E...)
	 * and the accidental (#, b, n) for each MIDI note value. This will be used
	 * in the display.
	 *   
	 * This function gets substring from strPtr into tmp_str and saves start of
	 * next substring in strPtr. Copy tmp_str to appropriate struct member in
	 * in range or rangeMaster. Get instrument name and abrv: if missing, return
	 * FALSE.
	 */
	 
	/* instrument name */		
	if ((strPtr = strcut(tmp_str, strPtr, NM_SIZE)) != NULL)
		strcpy((*rHdl)->name, (char *)tmp_str);
	else
		return(FALSE);
	 
	/* instrument abbreviation */		
	if ((strPtr = strcut(tmp_str, strPtr, NM_SIZE)) != NULL)
		strcpy((*rHdl)->abbr, (char *)tmp_str);
	else
		return(FALSE);	
	
	for (i = TOP; i <= BOT; i++)
	{
		/* range */
		if ((strPtr = strcut(tmp_str, strPtr, RSIZE)) != NULL)	
			/* ??With improper input, atoi can crash. We should use strtol instead. */
			(*rHdl)->ttb[i].midiNote = (char)atoi(tmp_str);
		else
			return(FALSE);
			
		/* note name */
		if ((strPtr = strcut(tmp_str, strPtr, 1)) != NULL)	
			(*rHdl)->ttb[i].name = tmp_str[0];
		else
			return(FALSE);
		
		/* accidental */
		if ((strPtr = strcut(tmp_str, strPtr, 1)) != NULL)	{
			if (tmp_str[0] == 'n') tmp_str[0] = ' ';	/* substitute <space> for 'n' (natural) */
			(*rHdl)->ttb[i].acc = tmp_str[0]; }
		else
			return(FALSE);
	}
	
	/* channel */
	if ((strPtr = strcut(tmp_str, strPtr, 2)) != NULL)	
		(*rHdl)->channel = (char)atoi(tmp_str);
	else
		return(FALSE);

	/* patch number */
	if ((strPtr = strcut(tmp_str, strPtr, 3)) != NULL)
		(*rHdl)->patchNum = (char)atoi(tmp_str);
	else
		return(FALSE);

	/* velocity bias */
	if ((strPtr = strcut(tmp_str, strPtr, 3)) != NULL)
		(*rHdl)->velBalance = (char)atoi(tmp_str);
	else
		return(FALSE);
			
	return(TRUE);
}


#define SEPARATOR		':'
#define STRBUF_SIZE	256
	 
/* Break a substring off of string s at SEPARATOR and copy it to r. Return pointer to
next substring head. Initial NULL ptr or NULL byte or size >= STRBUF_SIZE || size <= 0
causes NULL ptr to be returned.

??Should probably use strtok instead. */

char *strcut(char *r,		/* ptr to temp buffer where data will go */ 
					char *s,		/* ptr to NULLendian C string (text)     */
					INT16 size)	/* max length not counting NULL byte     */
{
	INT16 i = 0;
	
	if (s == NULL || r == NULL 
		|| *s == (char)NULL || size <= 0 || size >= STRBUF_SIZE)
		return(NULL);
	else
	{
		/* 
		 * check for max size, NULL byte, separation char, and if ok, copy to temp buf.
		 */
		 
		while (i++ <= size)
		{
		 	switch(*s)
		 	{
		 		case SEPARATOR:
		 		
		 		/* mark end of temp buf and return ptr
				 * skipping SEPARATOR so this will now point to
				 * next substring head or (char)NULL.
				 */
		 			*r = (char)NULL;	
					return(++s);
					
				case (char)NULL:		
				 /* end of input */
				 
		 			*r = (char)NULL;
		 			return(s);
		 			
		 		default:
		 		/* char is copied as we seek separator or end of input */
		 				
					*r++ = *s++;
					break;
			}
		}
		
		/* 
		 * If we are here, substring was longer than maximum size...this will
		 * cause all subsequent substrings to be lost! No other checks seem
		 * worth the trouble, because the resource has been improperly edited:
		 * who can guess what is now there? So, clean-up as best we can.
		 * Mark end of temp buf in case anyone uses it. 
		 */
		  			
		*r = (char)NULL;	
		return(NULL);
	}		 
}


#ifndef VIEWER_VERSION

void InitRange(
			rangeMaster *M)	/* ptr to data we get copied from PARTINFO * or list */
{	
	extern struct scaleTblEnt scaleTbl[17];

	/*
	 * scaleTbl[] has the names, accidentals, and position information for
	 * one octave of the ranges that may appear in the dialog box. The
	 * remaining (8va) positional info is kept in octTbl[].  The two added 
	 * together in InstrMoveRange() create the actual vertical drawing coordinates.
	 */
	 
	extern struct scaleTblEnt *noteInfo[3];
	
	/*
	 * one noteInfo each for TOP, BOT, TRANS. noteInfo[] ptrs will keep track of what
	 * is currently displayed  by the drawing functions.
	 */
	 
	extern INT16 octTbl[12];
	struct scaleTblEnt *savedp;
	INT16 r, i, pc, found;
	Boolean useDefault;
	Boolean anyDefault=FALSE;
	
	/* default all noteInfo[r] to C in scaleTbl */
	noteInfo[TOP] = noteInfo[TRANS] = noteInfo[BOT] = scaleTbl;
	
	/*
	 * To check on the validity of incoming data, we sanity-check each struct range
	 *	element in M->ttb[] against either the transposition or the range MIDI note
	 * number limits.
	 */
 	 
	for (r = 0; r < T_TRNS_B; r++)
	{
		useDefault = FALSE;
		if (r==TRANS && (M->ttb[r].midiNote > MAX_TRANS_MIDI ||
                        M->ttb[r].midiNote < MIN_TRANS_MIDI))
			useDefault = TRUE;
		else if (M->ttb[r].midiNote > MAXMIDI || M->ttb[r].midiNote < MINMIDI)
			useDefault = TRUE;
		else            /* MIGHT be sane...keep checking! */
		{	
			/*
			 * We want to see if the noteName and accidental are valid too...
			 * find an index into scaleTbl (assign to noteInfo[r]) for matching name, 
			 * accidental, and pitchclass of input with linear search 
			 * in scaleTbl[]. Because midiNote was valid, this may succeed.
			 */
			 pc =  M->ttb[r].midiNote % TWELVE;
			 savedp = noteInfo[r];
			 for (i = 0, found = FALSE; i <= MAXSCALE && found != TRUE; i++)
			 {
			 	if (M->ttb[r].name == noteInfo[r]->n.name 
			 		 && pc == noteInfo[r]->n.midiNote
			 	    && M->ttb[r].acc == noteInfo[r]->n.acc )
			 		found = TRUE;							/* leave loop */
			 	else
			 		noteInfo[r]++;							/* try next scaleTbl[] entry */
			 }
			 /* sanity check */
			 if (!found)									/* can't match name and acc with pc */
			 {
			 	noteInfo[r] = savedp;
			 	useDefault = TRUE;
			 }
		}
		if (useDefault) {
			anyDefault = TRUE;
			M->ttb[r].name = dfault.ttb[r].name;
		 	M->ttb[r].acc = dfault.ttb[r].acc;
		 	M->ttb[r].midiNote = dfault.ttb[r].midiNote; }
	}
	if (anyDefault) {
		GetIndCString(strBuf, INSTRERRS_STRS, 3);    /* "Some MIDI note number(s) in instrument list out of range or inconsistent with note name(s); will use defaults." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	ShowInit(M);
}

#endif /* not VIEWER_VERSION */


/* Return TRUE if p is not NULL and midinote values are safe and sane. */

Boolean MpCheck(PARTINFO *p)
{
	short minPatchNum;

	minPatchNum = 1;

	if (p!=NULL)
	{
		if (useWhichMIDI==MIDIDR_CM && !CMTransmitChannelValid(*origMPDevice , p->channel)) {
			master.cmDevice = config.cmDefaultOutputDevice;
			master.channel = config.cmDefaultOutputChannel;
		}

		if (p->hiKeyNum<=MAXMIDI
			&& p->hiKeyNum>=MINMIDI
			&& p->hiKeyNum+p->transpose<=MAXMIDI
			&& p->loKeyNum+p->transpose>=MINMIDI
			&& p->loKeyNum<=MAXMIDI
			&& p->loKeyNum>=MINMIDI
			&& p->patchNum<=MAXPATCHNUM
			&& p->patchNum>=minPatchNum
			&& p->partVelocity<=MAX_VELOCITY
			&& p->partVelocity>=-127 )
			return TRUE;	/* OK to use this data */
	}
	return FALSE;
}


/* --------------------------------------------------------------- PartMIDIDialog -- */

/* Dialog item numbers */
#define PM_INSTRNAME		5
#define PM_INSTRABRV		7
#define PM_CHANNEL		8
#define PM_PATCH			9
#define PM_V_BALANCE		10
#define PM_DEVICE_OMS	16
#define PM_DEVICE_CM		16

#define PM_PATCH_FMS		8		/* For FreeMIDI dlog, these used instead of */
#define PM_DEVICE_FMS	9		/* PM_CHANNEL, PM_PATCH, and PM_DEVICE_OMS */

#define PM_ALLPARTS		17


Boolean CMPartMIDIDialog(Document *doc, PARTINFO *mp, MIDIUniqueID *mpDevice, Boolean *allParts)
{
	origMPDevice = (unsigned long *)mpDevice;
	return PartMIDIDialog(doc, mp, allParts);
}

Boolean PartMIDIDialog(Document *doc, PARTINFO *mp, Boolean *allParts)
{
	Str255		str, deviceStr;
	char			fmtStr[256];
	DialogPtr	theDialog;
	GrafPtr		oldPort;
	Handle		hndl;
	Rect			itemBox;
	INT16			itemHit, theType, val;
	INT16			dialogOver;
	Boolean		gotValue;
	INT16			scratch;
	ModalFilterUPP	filterUPP;
	
	GetPort(&oldPort);

	if (useWhichMIDI==MIDIDR_CM) {
		filterUPP = NewModalFilterUPP(TheCMPMFilter);
		if (filterUPP == NULL) {
			MissingDialog(OMS_PARTMIDI_DLOG);
			return(0);
		}
		theDialog = GetNewDialog(OMS_PARTMIDI_DLOG, 0L, BRING_TO_FRONT);
		if (!theDialog) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(OMS_PARTMIDI_DLOG);
			return(0);
		}
	}
	else {
		filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP == NULL) {
			MissingDialog(PARTMIDI_DLOG);
			return(0);
		}
		theDialog = GetNewDialog(PARTMIDI_DLOG, 0L, BRING_TO_FRONT);
		if (!theDialog) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(PARTMIDI_DLOG);
			return(0);
		}
	}

	SetPort(GetDialogWindowPort(theDialog));;

	if (useWhichMIDI==MIDIDR_CM) {
		/* create the popup menu of available output devices */
		GetDialogItem(theDialog, PM_DEVICE_CM, &scratch, &hndl, &deviceMenuBox);
		cmOutputMenuH = CreateCMOutputMenu(theDialog, &cmOutputPopup, &deviceMenuBox, NULL, PM_DEVICE_CM);
	}

	if (MpCheck(mp)) {
		/* instrument name and abbrv. */		
		GoodStrncpy(master.name, mp->name, (unsigned long)NM_SIZE - 1);
		GoodStrncpy(master.abbr, mp->shortName, (unsigned long)AB_SIZE - 1);
		
		/* channel, patch number and velocity balance */
		master.channel = mp->channel;
		master.patchNum = mp->patchNum;
		master.velBalance = mp->partVelocity;

		/* set unique device ID for OMS.
			See code in GetOMSPartPlayInfo for a similar situation.  -JGG, 7/23/00 */
		if (useWhichMIDI == MIDIDR_CM) {
			/* Validate device / channel combination. */
			if (CMTransmitChannelValid(*origMPDevice, (INT16)(mp->channel)))
				master.cmDevice = *origMPDevice;
			else
				master.cmDevice = config.cmDefaultOutputDevice;
			/* It's possible our device has changed, so validate again. */
			if (CMTransmitChannelValid(master.cmDevice, (INT16)(mp->channel)))
				master.channel = mp->channel;
			else
				master.channel = config.cmDefaultOutputChannel;

			if ((master.channel != mp->channel) || master.cmDevice != *origMPDevice)
				master.changedDeviceChannel = 1;
		}
	}
	else {
		GetIndCString(strBuf, INSTRERRS_STRS, 1);					/* "Illegal instrument description; will use default." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		master = dfault;
	}	
	
	PutDlgWord(theDialog, PM_V_BALANCE, master.velBalance, FALSE);

	CToPString(strcpy((char *)str, master.abbr));
	PutDlgString(theDialog, PM_INSTRABRV, str, FALSE);

	CToPString(strcpy((char *)str, master.name));
	PutDlgString(theDialog, PM_INSTRNAME, str, TRUE);

	if (useWhichMIDI==MIDIDR_CM && cmOutputMenuH!=NULL) {
		GetIndString(deviceStr, MIDIPLAYERRS_STRS, 16);
		short currChoice = GetCMDeviceIndex(master.cmDevice);

		if (ValidCMDeviceIndex(currChoice)) {
			ChangePopUpChoice(&cmOutputPopup, currChoice + 1);
		}
		else {
			ChangePopUpChoice(&cmOutputPopup, 1);
		}
		DrawPopUp(&cmOutputPopup);
	}

	// 5_17r4: we are now using MIDIDR_CM and the selected text is the patch text, 
	// not the channel
	
	if (useWhichMIDI==MIDIDR_CM)
		SelectDialogItemText(theDialog, PM_PATCH, 0, ENDTEXT);
	else
		SelectDialogItemText(theDialog, PM_CHANNEL, 0, ENDTEXT);

	CenterWindow(GetDialogWindow(theDialog), 100);
	ShowWindow(GetDialogWindow(theDialog));
	ArrowCursor();
	
	*allParts = FALSE;
	dialogOver = 0;
	while (dialogOver==0) {
		do {	
			ModalDialog(filterUPP, &itemHit);
			if (itemHit<1) continue;									/* in case returned by filter */
			switch (itemHit) {
				case OKBTN:
				case CANCELBTN:
				case PM_ALLPARTS:
					dialogOver = itemHit;
					break;
			}
		} while (!dialogOver);
	
		if (dialogOver==OKBTN) {
	
			/* Copy modified channel, patch number, and velocity, if valid. If not, loop back. */

			if (useWhichMIDI==MIDIDR_CM) {
				gotValue = GetDlgWord(theDialog, PM_CHANNEL, &val);
				master.cmDevice = GetCMOutputDeviceID();
				if ((gotValue && val != mp->channel) || master.cmDevice != *origMPDevice)
					master.changedDeviceChannel = 1;
				if (gotValue && CMTransmitChannelValid(master.cmDevice, val)) {
					*origMPDevice = master.cmDevice;
					mp->channel = val;
					master.validDeviceChannel = 1;
				}
				else {
					GetIndCString(strBuf, INSTRERRS_STRS, 8);    	/* "Channel number and device are inconsistant" */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					GetDialogItem(theDialog, OKBTN, &theType, &hndl, &itemBox);
					HiliteControl((ControlHandle)hndl,0);			/* so OK btn doesn't stay hilited */
					dialogOver = 0;
					continue;										/* Loop back for correction */
				}					
			}
			else {
				gotValue = GetDlgWord(theDialog, PM_CHANNEL, &val);
				if (gotValue && val > 0 && val <= MAXCHANNEL)
					mp->channel = val;
				else {
					GetIndCString(fmtStr, INSTRERRS_STRS, 4);    /* "Channel number must be between 1 and %d." */
					sprintf(strBuf, fmtStr, MAXCHANNEL); 
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					GetDialogItem(theDialog, OKBTN, &theType, &hndl, &itemBox);
					HiliteControl((ControlHandle)hndl,0);			/* so OK btn doesn't stay hilited */
					dialogOver = 0;
					continue;										/* Loop back for correction */
				}
			}
			
			gotValue = GetDlgWord(theDialog, PM_V_BALANCE, &val);
			if (gotValue && val >= -127 && val <= MAX_VELOCITY)
				mp->partVelocity = val;
			else {
				GetIndCString(strBuf, INSTRERRS_STRS, 6);    /* "Velocity balance must be between -127 and 127." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				GetDialogItem(theDialog, OKBTN, &theType, &hndl, &itemBox);
				HiliteControl((ControlHandle)hndl,0);			/* so OK btn doesn't stay hilited */
				dialogOver = 0;
				continue;										/* Loop back for correction */
			}
													
		}	/* if (dialogOver==OKBTN) */
		else if (dialogOver==PM_ALLPARTS) {
	
			/* Copy modified channel, patch number, and velocity, if valid. If not, loop back. */

			if (useWhichMIDI==MIDIDR_CM) {
				gotValue = GetDlgWord(theDialog, PM_CHANNEL, &val);
				master.cmDevice = GetCMOutputDeviceID();
				if ((gotValue && val != mp->channel) || master.cmDevice != *origMPDevice)
					master.changedDeviceChannel = 1;
				if (gotValue && CMTransmitChannelValid(master.cmDevice, val)) {
					*origMPDevice = master.cmDevice;
					mp->channel = val;
					master.validDeviceChannel = 1;
				}
				else {
					GetIndCString(strBuf, INSTRERRS_STRS, 8);    	/* "Channel number and device are inconsistant" */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					GetDialogItem(theDialog, OKBTN, &theType, &hndl, &itemBox);
					HiliteControl((ControlHandle)hndl,0);			/* so OK btn doesn't stay hilited */
					dialogOver = 0;
					continue;												/* Loop back for correction */
				}					
			}
			
			gotValue = GetDlgWord(theDialog, PM_V_BALANCE, &val);
			if (gotValue && val >= -127 && val <= MAX_VELOCITY)
				mp->partVelocity = val;
			else {
				GetIndCString(strBuf, INSTRERRS_STRS, 6);    /* "Velocity balance must be between -127 and 127." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				GetDialogItem(theDialog, OKBTN, &theType, &hndl, &itemBox);
				HiliteControl((ControlHandle)hndl,0);			/* so OK btn doesn't stay hilited */
				dialogOver = 0;
				continue;										/* Loop back for correction */
			}
			
			*allParts = TRUE;		
		}
	}		/* while (dialogOver==0) */

	if (useWhichMIDI==MIDIDR_CM)
	{
		if (master.validDeviceChannel && master.changedDeviceChannel) {
			/* Ask user if they want to save the entered device/channel/patch as default
			 * for instrument, or... could add a new instrument type (need more flags
			 * to know when this has happened).  If so, then pack up a new instrument
			 * string and either insert/append a new instrument or overlay an existing one.
			 */
		}
		if (cmVecDevices != NULL) {
			delete cmVecDevices;
			cmVecDevices = NULL;
		}
		DisposePopUp(&cmOutputPopup);
	}

broken:
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(theDialog);
	SetPort(oldPort);
		
	return ((itemHit==OK || itemHit==PM_ALLPARTS) ? 1 : 0);
}


pascal Boolean TheOMSPMFilter(DialogPtr theDialog, EventRecord *theEvent, INT16 *itemHit)
{		
	GrafPtr		oldPort;
	Point 		mouseLoc;
	Boolean		filterVal = FALSE;
	short			omsMenuItem;
	INT16			type;
	Handle		hndl;
	Rect			box;
	
	switch(theEvent->what)
	{
		case updateEvt:
			if ((WindowPtr)theEvent->message == GetDialogWindow(theDialog))
			{
				GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
				BeginUpdate(GetDialogWindow(theDialog));	
				UpdateDialogVisRgn(theDialog);			
				FrameDefault(theDialog,OKBTN,TRUE);


				EndUpdate(GetDialogWindow(theDialog));
				SetPort(oldPort);
			}
			break;
			
		case mouseDown:
			mouseLoc = theEvent->where;
			GlobalToLocal(&mouseLoc);
			
				/* If we've hit the OK or Cancel button, bypass the rest of the control stuff below. */
			GetDialogItem(theDialog, OKBTN, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	break;
			GetDialogItem(theDialog, CANCELBTN, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	break;

		case autoKey:
		case keyDown:
			if (DlgCmdKey(theDialog, theEvent, itemHit, FALSE))
				return TRUE;
			break;
			
		case activateEvt:
			break;
	}
		
	return(filterVal);
}

pascal Boolean TheCMPMFilter(DialogPtr theDialog, EventRecord *theEvent, INT16 *itemHit)
{		
	GrafPtr		oldPort;
	Point 		mouseLoc;
	Boolean		filterVal = FALSE;
	INT16		type;
	Handle		hndl;
	Rect		box;
	short		ans = 0;
	
	switch(theEvent->what)
	{
		case updateEvt:
			if ((WindowPtr)theEvent->message == GetDialogWindow(theDialog))
			{
				GetPort(&oldPort); SetPort(GetDialogWindowPort(theDialog));
				BeginUpdate(GetDialogWindow(theDialog));	
				UpdateDialogVisRgn(theDialog);			
				FrameDefault(theDialog,OKBTN,TRUE);

				if (useWhichMIDI==MIDIDR_CM && cmOutputMenuH!=NULL) {
					DrawPopUp(&cmOutputPopup);
				}

				EndUpdate(GetDialogWindow(theDialog));
				SetPort(oldPort);
			}
			break;
			
		case mouseDown:
			mouseLoc = theEvent->where;
			GlobalToLocal(&mouseLoc);
			
				/* If we've hit the OK or Cancel button, bypass the rest of the control stuff below. */
			GetDialogItem(theDialog, OKBTN, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	break;
			GetDialogItem(theDialog, CANCELBTN, &type, &hndl, &box);
			if (PtInRect(mouseLoc, &box))	break;

			if (useWhichMIDI==MIDIDR_CM && cmOutputMenuH != NULL) {
				GetDialogItem(theDialog, PM_DEVICE_CM, &type, &hndl, &box);
				if (PtInRect(mouseLoc, &box)) {
					if (ans = DoUserPopUp(&cmOutputPopup)) {
						*itemHit = PM_DEVICE_CM;
					}
				}
			}

		case autoKey:
		case keyDown:
			if (DlgCmdKey(theDialog, theEvent, itemHit, FALSE))
				return TRUE;
			break;
			
		case activateEvt:
			break;
	}
		
	return(filterVal);
}

/* --------------------------------------------------------- XLoadInstrDialogSeg -- */
/* Null function to allow loading or unloading InstrDialog.c's segment. */

void XLoadInstrDialogSeg()
{
}
