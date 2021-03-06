/******************************************************************************************
*	FILE:	SetCommand.c
*	PROJ:	Nightingale
*	DESC:	Set dialog-handling routines, by John Gibson and DAB
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
/*
		SetFilter				SetStfBadValues			AnyBadValues
		IsSetEnabled			InitSetItems			XableSetItems
		FindMainEnabledItem		SetDialog				DoMainPopChoice
		DoPropPopChoice			DoSetArrowKey			DoSet
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* The "QuickChange" (originally called "Set") dialog looks like this:

  Change ttt ppp to vvv uuu

The main variable things (ttt, ppp, vvv) are said to be the Main, Property, and Value
items. Currently, these are left, middle, and right in a horizontal layout, but this
has changed before and could change again, especially in future localized versions.
uuu is the Units item. ttt is always visible; one or more of the others are sometimes
hidden.

Speaking of localization, the entire UI design--building a command from words/phrases
--is questionable from the internationalization standpoint, but oh well.

ttt = item type:	a popup menu containing Note/Rest, Text, Slur, etc.
ppp = property:		usually a popup menu containing staff, voice, appearance, etc.,
					but may be a static text string
vvv = value:		a popup menu, edittext field, or pair of radio buttons
uuu:				static text giving units, e.g., "qtr-spaces"

In the following description of how to add features, all changes are in SetCommand.c
	unless otherwise stated.

To add an item to the Main menu:
	(To be written.)

To add a Property menu for a given Main-menu item:
	1. Update resources. In the resource file, add the appropriate Property menu; in
			NResourceID.h, add a name for the menu resource.
	2. Add an enum labelled:
			Property position: ppp Property Popup items
	3. Add an item PPP_PROPERTY to the enum for "Popup array indices".
	4. Add a variable for its current value, pppPropertyChoice, to the list.
	5. Add to SetDialog a call to InitPopUp.
	6. Add to SetDialog a statement to initialize pppPropertyChoice.
	7. Add to SetDialog a case pppITEM.

To add an item to the (pre-existing) Property menu for a given Main-menu item:
	1. Write a function SetSelPPP and add it to SetUtil.c.
	2. Update resources. In the resource file, add ppp to the appropriate Property
		menu. Be sure it's enabled.
	3. Look for an enum labelled:
			Property position: ppp Property Popup items	
		If none exists, add it. In any case, now add a name--say, pppITEM--for the
		item's index in that menu.
	4. Be sure there's a variable for its current value, pppChoice, in the list:
			Variables to hold sticky menu item choices and numeric values

	The next step depends on what the new item's Value item will be.
	5a. If the new item's Value item will be a popup menu:
		- Update resources. if there's already a Value menu with the choices you want,
			skip to step 6. Otherwise, in the resource file, add the appropriate Value
			menu; in NResourceID.h, add a name for the menu resource.
		- Add a name for the item's own menu to the enum labelled:
			Popup array indices
		- Add to SetDialog a few lines to call InitPopup, etc.
	5b. If its Value item will be a textedit field, illegal entries are possible;
		to handle them, add checking to AnyBadValues. (Description of other changes
		for textedit fields to be written.)
	6. If different Properties of the (Main) type can have different widgets for the
		Value item (i.e., more than one of popup menu, edittext field, or pair of
		radio buttons), add code to DoPropPopChoice to change appropriately for the
		new iem.
	7. In SetDialog, in code for dialogOver==BUT1_OK: be sure there's
			- a statement to set <pppChoice>
			- a statement to set <*finalVal>
	8. In DoPropPopChoice, add a case to handle pppITEM.
	9. In DoSet, add a case to handle pppITEM by calling SetSelPPP.
*/

/* Main position: Main Set Popup items */	
enum {	
	noteITEM = 1,
	slurTieITEM,
	textITEM,
	lyricITEM,
	chordSymITEM,
	tempoITEM,
	barlineITEM,
	dynamicITEM,
	clefITEM,
	keySigITEM,
	timeSigITEM,
	tupletITEM,
	beamsetITEM,
	drawITEM,
	patchChangeITEM,
	panITEM,
	HOW_MANY_MAIN_ITEMS=panITEM
};
	
/* Property position: Note Property Popup items */
enum {
	voiceITEM = 1,
	staffITEM,
	velocityITEM,
	noteAppearanceITEM,
	noteSizeITEM,
	stemLengthITEM,
	parenITEM
};

/* Property position: Text Property Popup items (also used for Lyrics, Chord Symbols, Tempos) */
enum {
	vPosAboveITEM = 1,
	vPosBelowITEM,
	hPosOffsetITEM,
	styleITEM,
	hideMM_ITEM=styleITEM
};

/* Property position: Tuplet Property Popup items */
enum {
	bracketITEM = 1,
	accNumsITEM
};

/* Value position: Note/rest Shape Popup items */
enum {
	normalITEM = 1,
	_____________1,
	xShapeITEM,
	harmonicITEM,
	slashITEM,
	hollowSquareITEM,
	filledSquareITEM,
	hollowDiamondITEM,
	filledDiamondITEM,
	halfNoteITEM,
	_____________2,
	invisibleITEM,
	allInvisITEM
};

/* Property position: Slur Property Popup items */
enum {
	shapeposITEM = 1,
	slurAppearanceITEM
};

/* Property position: Barline Property Popup items */
enum {
	barlineVisibleITEM = 1,
	barlineTypeITEM
};

/* Property position: PatchChange Property Popup items */
enum {
	patchChangeVisibleITEM = 1,
};

/* Property position: Dynamic Property Popup items */
enum {
	dynamVisibleITEM = 1,
	dynamSmallITEM
};

/* Property position: Beamset Property Popup items */
enum {
	beamsetThicknessITEM = 1
};

/* Symbolic Dialog Item Numbers */

enum {
	BUT1_OK = 1,
	BUT2_Cancel,
	POP3,					/* Main popup useritem */
	POP4,					/* Property popup useritem */
	POP5,					/* Value popup useritem */
	STXT6_Set,
	STXT7_Property,	/* For properties not requiring a popup. */
	STXT8_To,
	EDIT9,
	STXT10_Value,		/* For values not requiring a popup. */
	STXT11_qtr,			/* For "qtr. spaces" or other units. */
	RAD12_fromContext,
	RAD13_to,
	EDIT14,
	LAST_ITEM = EDIT14
};
	
	
/* -------------------------------------------------------------------  static globals -- */

enum {			/* Popup array indices */
	MAIN_SET = 0,
	NOTE_PROPERTY,
	TEXT_PROPERTY,
	CHORDSYM_PROPERTY,
	TEMPO_PROPERTY,
	TUPLET_PROPERTY,
	SLUR_PROPERTY,
	BARLINE_PROPERTY,
	NOTE_APPEARANCE,
	NOTE_SIZE,
	TEXT_STYLE,
	VISIBILITY,
	ACCNUMVIS,
	NOTE_ACC_PARENS,
	SLUR_APPEARANCE,
	BARLINE_TYPE,
	DYNAMIC_PROPERTY,
	DYNAMIC_SIZE,
	BEAMSET_PROPERTY,
	BEAMSET_THICKNESS,
	HOW_MANY_POPS		/* number of structs in popup array */
};
	
/* Current button (item number), for each radio group */
static short group1 = RAD12_fromContext;

static UserPopUp	setPopups[HOW_MANY_POPS];		/* array of Popup structs */

static MenuHandle	textOrLyricMenu;					/* handle to menu not currently in text style popup struct */

static short	hilitedPop = -1;  		/* Array index of popup currently "hilited" with HilitePopUp by using arrow keys. If -1, no hilitedPop. */
static short	currPropertyPop = -1;	/* Array index number of current Property popup. If -1, none visible. */
static short	currValuePop = -1;		/* Array index number of current Value popup. If -1, none visible. */

/* Variables to hold sticky menu item choices and numeric values */
static short	mainSetChoice = 1;

static short	notePropertyChoice = 1;
static short	textPropertyChoice = 1;
static short	chordSymPropertyChoice = 1;
static short	tempoPropertyChoice = 1;
static short	tupletPropertyChoice = 1;
static short	slurPropertyChoice = 1;
static short	barlinePropertyChoice = 1;
static short	dynamicPropertyChoice = 1;
static short	beamsetPropertyChoice = 1;

static short	noteAppearanceChoice = 1;
static short	noteSizeChoice = 1;
static short	textStyleChoice = 1;
static short	lyricStyleChoice = 1;
static short	visibleChoice = 2;
static short	accNumVisChoice = 1;
static short	accParensChoice = 1;
static short	slurAppearanceChoice = 1;
static short	barlineTypeChoice = 1;
static short	dynamicSizeChoice = 1;
static short	beamsetThicknessChoice = 1;

static short	edit9Value = 1;
static short	edit14Value = 0;

/* For enabling/disabling items in the main popup */
static Boolean haveSync, haveSlur, haveText, haveLyric, haveChordSym,
				haveTempo, haveBarline, haveDynamic, haveClef, haveKeySig,
				haveTimeSig, haveTuplet, haveBeamset, haveDraw, havePatchChange, havePan;

/* ------------------------------------------------------------------------ Prototypes -- */

static pascal	Boolean SetFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static Boolean	AnyBadValues(DialogPtr, Document *);
static Boolean	InitSetItems(Document *, short *);
static void		XableSetItems(Document *);
static short	FindMainEnabledItem(short, Boolean, Boolean);
static void		DoMainPopChoice(DialogPtr dlog,short choice);
static void		DoConstPropPopChoice(DialogPtr dlog,short choice);
static void		DoPropPopChoice(DialogPtr dlog,short choice);
static void 	DoSetArrowKey(DialogPtr dlog, short whichKey);

/* ====================================================================================== */

static Point where;
static short modifiers;

static pascal Boolean SetFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Boolean ans=False; WindowPtr w;
	short ch;	
		
	w = (WindowPtr)(evt->message);
	switch(evt->what) {
		case updateEvt:
			if (w == GetDialogWindow(dlog)) {
				short index;
				
				SetPort(GetWindowPort(w));
				BeginUpdate(GetDialogWindow(dlog));
				UpdateDialogVisRgn(dlog);
				for (index=0;index<HOW_MANY_POPS;index++) {
					DrawPopUp(&setPopups[index]);
				}
				FrameDefault(dlog,BUT1_OK,1);				
				EndUpdate(GetDialogWindow(dlog));
				ans = True; *itemHit = 0;
			}
			break;
		case activateEvt:
			if (w == GetDialogWindow(dlog)) {
				//short activ = (evt->modifiers & activeFlag)!=0;
				SetPort(GetWindowPort(w));
			}
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			modifiers = evt->modifiers;
			
			if (PtInRect(where,&setPopups[MAIN_SET].shadow)) {
				if (hilitedPop>-1) HilitePopUp(&setPopups[hilitedPop],False);
				hilitedPop = -1;
				*itemHit = DoUserPopUp(&setPopups[MAIN_SET]) ? POP3 : 0;
				if (*itemHit) DoMainPopChoice(dlog,setPopups[MAIN_SET].currentChoice);
				ans = True; break;
			}
				
			if (currPropertyPop>-1)
				if (PtInRect(where,&setPopups[currPropertyPop].shadow)) {
					if (hilitedPop>-1) HilitePopUp(&setPopups[hilitedPop],False); 
					hilitedPop = -1;
					*itemHit = DoUserPopUp(&setPopups[currPropertyPop]) ? POP4 : 0;
					if (*itemHit) DoPropPopChoice(dlog,setPopups[currPropertyPop].currentChoice);
					ans = True; break;
				}
				
			if (currValuePop>-1)
				if (PtInRect(where,&setPopups[currValuePop].shadow)) {
					if (hilitedPop>-1) HilitePopUp(&setPopups[hilitedPop],False); 
					hilitedPop = -1;
					*itemHit = DoUserPopUp(&setPopups[currValuePop]) ? POP5 : 0;
					ans = True; break;
				}	
			break;
		case autoKey:
		case keyDown:
			if (DlgCmdKey(dlog, evt, itemHit, False))
				return True;
			else {
				ch = (unsigned char)evt->message;
				switch (ch) {
					case UPARROWKEY:
					case DOWNARROWKEY:
					case LEFTARROWKEY:
					case RIGHTARROWKEY:
						DoSetArrowKey(dlog,ch);
						ans = True; break;						
				}
			}
			break;		
		}	/* switch evt->what */
		
	return(ans);
}


/* ---------------------------------------------------------------------- AnyBadValues -- */

static Boolean SetStfBadValues(Document *, short);
static Boolean SetStfBadValues(Document *doc, short staff)
{
	LINK partL, pL, aNoteL;
	PPARTINFO pPart;
	short firstStaff, lastStaff, maxStaff;

	/* Use the first selected note/rest to find the part, then check that every
	   selected note/rest is on one of that part's staves. */
		
	firstStaff = lastStaff = -1;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteSEL(aNoteL)) {
					if (firstStaff<0) {
						partL = FindPartInfo(doc, Staff2Part(doc, NoteSTAFF(aNoteL)));
						pPart = GetPPARTINFO(partL);
						firstStaff = pPart->firstStaff;
						lastStaff = pPart->lastStaff;
					}
					if (NoteSTAFF(aNoteL)<firstStaff
					|| NoteSTAFF(aNoteL)>lastStaff) {
						GetIndCString(strBuf, SETERRS_STRS, 15);    /* "Can't Set Staff because not all selected notes/rests are in the same part." */
						return True;
					}
				}
			}
		}
	}

	maxStaff = lastStaff-firstStaff+1;
	if (staff<1 || staff>maxStaff) {
		GetIndCString(strBuf, SETERRS_STRS, 16);    /* "Staff number can't be less than 1 or larger than the number of staves in the part." */
		return True;
	}
	
	return False;
}

static Boolean AnyBadValues(DialogPtr dlog, Document *doc)
{
	Boolean	anError=False;
	short	badField, value;
	
	switch (setPopups[MAIN_SET].currentChoice) {
		case noteITEM:
			if (setPopups[NOTE_PROPERTY].currentChoice==velocityITEM
			&& group1==RAD13_to) {
				GetDlgWord(dlog,EDIT14,&value);
				if (value<0 || value>127) {							/* MAX_VELOCITY */
					GetIndCString(strBuf, SETERRS_STRS, 17);		/* "Velocity must be from 0 to 127." */
					anError = True; badField = EDIT14;
				}
			}
			
			/* The other note properties get their value from item EDIT9. */
			
			GetDlgWord(dlog,EDIT9,&value);
			
			if (setPopups[NOTE_PROPERTY].currentChoice==voiceITEM) {
				if (value<1 || value>=31) {							/* VOICEINFO's relVoice is a 5-bit field */
					GetIndCString(strBuf, SETERRS_STRS, 18);		/* "Voice number must be from 1 to 31." */
					anError = True; badField = EDIT9;
				}
			}
			if (setPopups[NOTE_PROPERTY].currentChoice==staffITEM) {
				if (SetStfBadValues(doc, value)) {
					anError = True; badField = EDIT9;
				}
			}
			if (setPopups[NOTE_PROPERTY].currentChoice==stemLengthITEM) {
				if (value<0) {
					GetIndCString(strBuf, SETERRS_STRS, 19);		/* "Stem length can't be negative." */
					anError = True; badField = EDIT9;
				}
			}
			break;
			
		case drawITEM:
			GetDlgWord(dlog,EDIT9,&value);
			if (value<1 || value>127) {
				GetIndCString(strBuf, SETERRS_STRS, 21);			/* "Line thickness must be..." */
				anError = True; badField = EDIT9;
			}
			break;
			
		default:
			;
	}

	if (anError) {
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		SelectDialogItemText(dlog, badField, 0, ENDTEXT);		/* Select bad field so user can easily fix it. */
		return True;											/* Keep dialog on screen */
	}
	else
		return False;
}


/* ---------------------------------------------------------------------- IsSetEnabled -- */
/* Return True if the Set command should be enabled (because something is selected
that it can handle). */

Boolean IsSetEnabled(Document *doc)
{
	LINK pL;

	if (doc==NULL) return False;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL))
			switch (ObjLType(pL)) {
				case SYNCtype:
				case SLURtype:
				case MEASUREtype:
				case DYNAMtype:
				case CLEFtype:
				case KEYSIGtype:
				case TIMESIGtype:
				case TUPLETtype:
				case TEMPOtype:
				case BEAMSETtype:
					return True;
				case GRAPHICtype:
					if (GraphicSubType(pL)==GRString || GraphicSubType(pL)==GRLyric
					||  GraphicSubType(pL)==GRChordSym || GraphicSubType(pL)==GRDraw 
					|| GraphicSubType(pL)==GRMIDIPatch
					//|| GraphicSubType(pL)==GRSusPedalDown
					|| GraphicSubType(pL)==GRMIDIPan)
						return True;
				default:
					;
			}
	}
	
	return False;
}


/* ---------------------------------------------------------------------- InitSetItems -- */
/* Set global variables to indicate what types of objects are selected. Then, if type
*pChoice is disabled, set it to the first one that's enabled. If any of the items are
enabled, return True, else return False. */

static Boolean InitSetItems(Document *doc, short *pChoice)
{
	LINK pL;
	
	haveSync = haveSlur = haveText = haveLyric = haveChordSym = False;
	haveTempo = haveBarline = haveDynamic = haveClef = haveKeySig = False;
	haveTimeSig = haveTuplet = haveDraw = havePatchChange = False, havePan = False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL))
			switch (ObjLType(pL)) {
				case SYNCtype:
					haveSync = True; break;
				case SLURtype:
					haveSlur = True; break;
				case GRAPHICtype:
					if (GraphicSubType(pL)==GRString) haveText = True;
					else if (GraphicSubType(pL)==GRLyric) haveLyric = True;
					else if (GraphicSubType(pL)==GRChordSym) haveChordSym = True;
					else if (GraphicSubType(pL)==GRDraw) haveDraw = True;
					else if (GraphicSubType(pL)==GRMIDIPatch) havePatchChange = True;
					else if (GraphicSubType(pL)==GRMIDIPan) havePan = True;
					break;
				case TEMPOtype:
					haveTempo = True; break;
				case MEASUREtype:
					haveBarline = True; break;
				case DYNAMtype:
					haveDynamic = True; break;
				case CLEFtype:
					haveClef = True; break;
				case KEYSIGtype:
					haveKeySig = True; break;
				case TIMESIGtype:
					haveTimeSig = True; break;
				case TUPLETtype:
					haveTuplet = True; break;
				case BEAMSETtype:
					haveBeamset = True; break;
				default:
					;
			}
	}

	*pChoice = FindMainEnabledItem(*pChoice, False, True);

	return (haveSync || haveSlur || haveText || haveLyric || haveChordSym ||
				haveTempo || haveBarline || haveDynamic || haveClef || haveKeySig ||
				haveTimeSig || haveTuplet || haveBeamset || haveDraw || havePatchChange || havePan);
}


/* --------------------------------------------------------------------- XableSetItems -- */
/* Enable/disable items in the main popup according to what types of objects are
selected (actually, we only have to disable, since they're always initially enabled).
*/

static void XableSetItems(Document */*doc*/)		/* doc is no longer used */
{
	if (!haveSync) DisableMenuItem(setPopups[MAIN_SET].menu, noteITEM);
	if (!haveSlur) DisableMenuItem(setPopups[MAIN_SET].menu, slurTieITEM);
	if (!haveText) DisableMenuItem(setPopups[MAIN_SET].menu, textITEM);
	if (!haveLyric) DisableMenuItem(setPopups[MAIN_SET].menu, lyricITEM);
	if (!haveChordSym) DisableMenuItem(setPopups[MAIN_SET].menu, chordSymITEM);
	if (!haveTempo) DisableMenuItem(setPopups[MAIN_SET].menu, tempoITEM);
	if (!haveBarline) DisableMenuItem(setPopups[MAIN_SET].menu, barlineITEM);
	if (!haveDynamic) DisableMenuItem(setPopups[MAIN_SET].menu, dynamicITEM);
	if (!haveClef) DisableMenuItem(setPopups[MAIN_SET].menu, clefITEM);
	if (!haveKeySig) DisableMenuItem(setPopups[MAIN_SET].menu, keySigITEM);
	if (!haveTimeSig) DisableMenuItem(setPopups[MAIN_SET].menu, timeSigITEM);
	if (!haveTuplet) DisableMenuItem(setPopups[MAIN_SET].menu, tupletITEM);
	if (!haveBeamset) DisableMenuItem(setPopups[MAIN_SET].menu, beamsetITEM);
	if (!haveDraw) DisableMenuItem(setPopups[MAIN_SET].menu, drawITEM);
	if (!havePatchChange) DisableMenuItem(setPopups[MAIN_SET].menu, patchChangeITEM);
	if (!havePan) DisableMenuItem(setPopups[MAIN_SET].menu, panITEM);
}


/* --------------------------------------------------------------- FindMainEnabledItem -- */
/* Find the next enabled item in the main popup, wrapping around if <wrap>. Returns
the enabled item's number, or -1 if none is found without wrapping. Assumes the
<haveXXX> variables have been set, presumably by XableSetItems. */

short FindMainEnabledItem(short currItem, Boolean up, Boolean wrap)
{
	Boolean enabled[HOW_MANY_MAIN_ITEMS+1];
	
	enabled[noteITEM] = haveSync;			enabled[slurTieITEM] = haveSlur;
	enabled[textITEM] = haveText;			enabled[lyricITEM] = haveLyric;
	enabled[chordSymITEM] = haveChordSym;	enabled[tempoITEM] = haveTempo;
	enabled[barlineITEM] = haveBarline;		enabled[dynamicITEM] = haveDynamic;
	enabled[clefITEM] = haveClef;			enabled[keySigITEM] = haveKeySig;
	enabled[timeSigITEM] = haveTimeSig;		enabled[tupletITEM] = haveTuplet;
	enabled[drawITEM] = haveDraw;			enabled[patchChangeITEM] = havePatchChange;
	enabled[panITEM] = havePan;				enabled[beamsetITEM] = haveBeamset;
	
	while (!enabled[currItem]) {
		if (up) {
			currItem--;
			if (currItem<1) {
				if (wrap)
					currItem = HOW_MANY_MAIN_ITEMS;
				else {
					currItem = -1;
					break;
				}
			}
		}
		else {
			currItem++;
			if (currItem>HOW_MANY_MAIN_ITEMS) {
				if (wrap)
					currItem = 1;
				else {
					currItem = -1;
					break;
				}
			}
		}
	}
	return currItem;
}


/* ------------------------------------------------------------------------- SetDialog -- */
/* If OK'ed, sets its parameters to the new value and to describe the object type
and property to set to that value, and returns True. If Cancelled or an error occurs,
returns False. */

Boolean SetDialog(
				Document	*doc,
				short		*objType,			/* Type of object to be set (ttt) */
				short		*param,				/* Property of object to be set (ppp) */
				short		*finalVal 			/* Return value (vvv) */
				)
{
	short itemHit, type, okay=False, dialogOver, value, index;
	Handle hndl;  Rect box;
	DialogPtr dlog;  GrafPtr oldPort;		
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(SetFilter);
	if (filterUPP == NULL) {
		MissingDialog(SET_DLOG);
		return False;
	}

	GetPort(&oldPort);
	
	dlog = GetNewDialog(SET_DLOG, NULL, BRING_TO_FRONT);
	if (dlog == NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SET_DLOG);
		return False;
	}

	CenterWindow(GetDialogWindow(dlog), 80);
	SetPort(GetDialogWindowPort(dlog));

	/* Initialize all PopUps. First, though, we have to set <mainSetChoice>. */
	 
	if (!InitSetItems(doc, &mainSetChoice)) return(False);
	
	/* ------------------ Initialize PopUps for Main position. ------------------ */
	
	if (!InitPopUp(dlog,&setPopups[MAIN_SET],POP3,STXT6_Set, MAINSET_POPUP,
						mainSetChoice))
		goto broken;
	if (ResizePopUp(&setPopups[MAIN_SET]))
		ChangePopUpChoice(&setPopups[MAIN_SET],mainSetChoice);
		
	/* ----------------- Initialize PopUps for Property position. ----------------- */

	if (!InitPopUp(dlog,&setPopups[NOTE_PROPERTY],POP4,STXT6_Set, NOTEPROPERTY_POPUP,
						notePropertyChoice))
		goto broken;
	if (ResizePopUp(&setPopups[NOTE_PROPERTY]))
		ChangePopUpChoice(&setPopups[NOTE_PROPERTY],notePropertyChoice);
		
	if (!InitPopUp(dlog,&setPopups[TEXT_PROPERTY],POP4,STXT6_Set, TEXTPROPERTY_POPUP,
						textPropertyChoice))
		goto broken;
	if (ResizePopUp(&setPopups[TEXT_PROPERTY]))
		ChangePopUpChoice(&setPopups[TEXT_PROPERTY],textPropertyChoice);
		
	if (!InitPopUp(dlog,&setPopups[CHORDSYM_PROPERTY],POP4,STXT6_Set, CHORDSYMPROPERTY_POPUP,
						chordSymPropertyChoice))
		goto broken;
	if (ResizePopUp(&setPopups[CHORDSYM_PROPERTY]))
		ChangePopUpChoice(&setPopups[CHORDSYM_PROPERTY],chordSymPropertyChoice);

	if (!InitPopUp(dlog,&setPopups[TEMPO_PROPERTY],POP4,STXT6_Set, TEMPOPROPERTY_POPUP,
						tempoPropertyChoice))
		goto broken;
	if (ResizePopUp(&setPopups[TEMPO_PROPERTY]))
		ChangePopUpChoice(&setPopups[TEMPO_PROPERTY],tempoPropertyChoice);


	if (!InitPopUp(dlog,&setPopups[TUPLET_PROPERTY],POP4,STXT6_Set, TUPLETPROPERTY_POPUP,
						tupletPropertyChoice))
		goto broken;
	if (ResizePopUp(&setPopups[TUPLET_PROPERTY]))
		ChangePopUpChoice(&setPopups[TUPLET_PROPERTY],tupletPropertyChoice);
		
	if (!InitPopUp(dlog,&setPopups[SLUR_PROPERTY],POP4,STXT6_Set, SLURPROPERTY_POPUP,
						slurPropertyChoice))
		goto broken;
	if (ResizePopUp(&setPopups[SLUR_PROPERTY]))
		ChangePopUpChoice(&setPopups[SLUR_PROPERTY],slurPropertyChoice);

	if (!InitPopUp(dlog,&setPopups[BARLINE_PROPERTY],POP4,STXT6_Set, BARLINE_PROPERTY_POPUP,
						barlinePropertyChoice))
		goto broken;
	if (ResizePopUp(&setPopups[BARLINE_PROPERTY]))
		ChangePopUpChoice(&setPopups[BARLINE_PROPERTY],barlinePropertyChoice);

	if (!InitPopUp(dlog,&setPopups[DYNAMIC_PROPERTY],POP4,STXT6_Set, DYNAMIC_PROPERTY_POPUP,
				   dynamicPropertyChoice))
		goto broken;
	
	if (ResizePopUp(&setPopups[DYNAMIC_PROPERTY]))
		ChangePopUpChoice(&setPopups[DYNAMIC_PROPERTY],dynamicPropertyChoice);
	
	if (!InitPopUp(dlog,&setPopups[BEAMSET_PROPERTY],POP4,STXT6_Set, BEAMSET_PROPERTY_POPUP,
				   beamsetPropertyChoice))
		goto broken;
	
	if (ResizePopUp(&setPopups[BEAMSET_PROPERTY]))
		ChangePopUpChoice(&setPopups[BEAMSET_PROPERTY],beamsetPropertyChoice);
	
	
	/* ------------------ Initialize PopUps for Value position. ------------------ */

	if (!InitPopUp(dlog,&setPopups[NOTE_APPEARANCE],POP5,STXT8_To, HEADSHAPE_POPUP,
						noteAppearanceChoice))
		goto broken;
	if (ResizePopUp(&setPopups[NOTE_APPEARANCE]))
		ChangePopUpChoice(&setPopups[NOTE_APPEARANCE],noteAppearanceChoice);
		
	if (!InitPopUp(dlog,&setPopups[NOTE_SIZE],POP5,STXT8_To, NOTESIZE_POPUP,
						noteSizeChoice))
		goto broken;
	if (ResizePopUp(&setPopups[NOTE_SIZE]))
		ChangePopUpChoice(&setPopups[NOTE_SIZE],noteSizeChoice);
		
	if (!InitPopUp(dlog,&setPopups[TEXT_STYLE],POP5,STXT8_To, TEXTSTYLE_POPUP,
						textStyleChoice))
		goto broken;
	if (ResizePopUp(&setPopups[TEXT_STYLE]))
		ChangePopUpChoice(&setPopups[TEXT_STYLE],textStyleChoice);
	
	/* Set up lyric style menu for later substitution into text style popup */
	textOrLyricMenu = GetMenu(LYRICSTYLE_POPUP);
	if (!textOrLyricMenu) goto broken;

	if (!InitPopUp(dlog,&setPopups[VISIBILITY],POP5,0, VISIBILITY_POPUP, visibleChoice))
		goto broken;
	setPopups[VISIBILITY].bounds = setPopups[VISIBILITY].box; InsetRect(&setPopups[VISIBILITY].bounds,-1,-1);
	setPopups[VISIBILITY].shadow = setPopups[VISIBILITY].bounds;
	setPopups[VISIBILITY].shadow.right++; setPopups[VISIBILITY].shadow.bottom++;
	if (ResizePopUp(&setPopups[VISIBILITY])) ChangePopUpChoice(&setPopups[VISIBILITY],visibleChoice);
	
	if (!InitPopUp(dlog,&setPopups[ACCNUMVIS],POP5,STXT8_To, ACCNUMVIS_POPUP,
						accNumVisChoice))
		goto broken;
	if (ResizePopUp(&setPopups[ACCNUMVIS]))
		ChangePopUpChoice(&setPopups[ACCNUMVIS],accNumVisChoice);
	
	if (!InitPopUp(dlog,&setPopups[NOTE_ACC_PARENS],POP5,STXT8_To, NOTE_ACC_PARENS_POPUP,
						accParensChoice))
		goto broken;
	if (ResizePopUp(&setPopups[NOTE_ACC_PARENS]))
		ChangePopUpChoice(&setPopups[NOTE_ACC_PARENS],accParensChoice);

	if (!InitPopUp(dlog,&setPopups[SLUR_APPEARANCE],POP5,STXT8_To, SLUR_APPEARANCE_POPUP,
						slurAppearanceChoice))
		goto broken;
	if (ResizePopUp(&setPopups[SLUR_APPEARANCE]))
		ChangePopUpChoice(&setPopups[SLUR_APPEARANCE],slurAppearanceChoice);

	if (!InitPopUp(dlog,&setPopups[BARLINE_TYPE],POP5,STXT8_To, BARLINE_TYPE_POPUP,
						barlineTypeChoice))
		goto broken;
	if (ResizePopUp(&setPopups[BARLINE_TYPE]))
		ChangePopUpChoice(&setPopups[BARLINE_TYPE],barlineTypeChoice);

	if (!InitPopUp(dlog,&setPopups[DYNAMIC_SIZE],POP5,STXT8_To, DYNAMIC_SIZE_POPUP,
				   dynamicSizeChoice))
		goto broken;
	if (ResizePopUp(&setPopups[DYNAMIC_SIZE]))
		ChangePopUpChoice(&setPopups[DYNAMIC_SIZE],dynamicSizeChoice);
	
	if (!InitPopUp(dlog,&setPopups[BEAMSET_THICKNESS],POP5,STXT8_To, BEAMSET_THICKNESS_POPUP,
				   beamsetThicknessChoice))
		goto broken;
	if (ResizePopUp(&setPopups[BEAMSET_THICKNESS]))
		ChangePopUpChoice(&setPopups[BEAMSET_THICKNESS],beamsetThicknessChoice);
	
	XableSetItems(doc);
	DoMainPopChoice(dlog,mainSetChoice); 		/* Set up items after mainSetPopup to agree with sticky choice. */		
		
	PutDlgWord(dlog,EDIT9,edit9Value,False);
	PutDlgChkRadio(dlog,RAD12_fromContext,(group1==RAD12_fromContext? True:False));
	PutDlgChkRadio(dlog,RAD13_to,(group1==RAD13_to? True:False));
	PutDlgWord(dlog,EDIT14,edit14Value,False);
	if (notePropertyChoice==velocityITEM)
		SelectDialogItemText(dlog,EDIT14,0,(group1==RAD13_to? ENDTEXT:0));
	else SelectDialogItemText(dlog,EDIT9,0,ENDTEXT);
			
	ShowWindow(GetDialogWindow(dlog));		
	FrameDefault(dlog,BUT1_OK,1);
	ArrowCursor();

	dialogOver = 0;
	while (dialogOver==0) {
		do {
			ModalDialog(filterUPP,&itemHit);
			GetDialogItem(dlog,itemHit,&type,&hndl,&box);
			switch(type) {
				case ctrlItem+btnCtrl:
					switch(itemHit) {
						case BUT1_OK:
						case BUT2_Cancel:
							dialogOver = itemHit;
							break;
						}
					break;
				case ctrlItem+radCtrl:
					switch(itemHit) {
						case RAD12_fromContext:		/* Group 1 */
						case RAD13_to:
							if (itemHit != group1) {
								SetControlValue((ControlHandle)hndl,value=!GetControlValue((ControlHandle)hndl));
								GetDialogItem(dlog,group1,&type,&hndl,&box);
								SetControlValue((ControlHandle)hndl,False);
								group1 = itemHit;
								/* If "to" radio hit, select its edit field; else deselect field */
								SelectDialogItemText(dlog,EDIT14,0,group1==RAD13_to?ENDTEXT:0);
							}
							break;
						}
					break;
				case editText:
					switch(itemHit) {
						case EDIT14:
							/* If edit field hit, choose its radio button */
							GetDialogItem(dlog,RAD12_fromContext,&type,&hndl,&box);
							SetControlValue((ControlHandle)hndl,False);
							GetDialogItem(dlog,group1=RAD13_to,&type,&hndl,&box);
							SetControlValue((ControlHandle)hndl,True);
							break;
						}
					break;
			}
		} while (!dialogOver);

		if (dialogOver==BUT1_OK)
			if (AnyBadValues(dlog, doc)) dialogOver = 0;
	}

	if (dialogOver==BUT1_OK) {
		/* Save current menu choices and edit field value for next invocation
		of dialog. */
				
		mainSetChoice = setPopups[MAIN_SET].currentChoice;

		notePropertyChoice = setPopups[NOTE_PROPERTY].currentChoice;
		textPropertyChoice = setPopups[TEXT_PROPERTY].currentChoice;
		chordSymPropertyChoice = setPopups[CHORDSYM_PROPERTY].currentChoice;
		tempoPropertyChoice = setPopups[TEMPO_PROPERTY].currentChoice;
		tupletPropertyChoice = setPopups[TUPLET_PROPERTY].currentChoice;
		slurPropertyChoice = setPopups[SLUR_PROPERTY].currentChoice;
		barlinePropertyChoice = setPopups[BARLINE_PROPERTY].currentChoice;
		dynamicPropertyChoice = setPopups[DYNAMIC_PROPERTY].currentChoice;

		noteAppearanceChoice = setPopups[NOTE_APPEARANCE].currentChoice;
		noteSizeChoice = setPopups[NOTE_SIZE].currentChoice;
		textStyleChoice = setPopups[TEXT_STYLE].currentChoice;
		accNumVisChoice = setPopups[ACCNUMVIS].currentChoice;
		visibleChoice = setPopups[VISIBILITY].currentChoice;
		accParensChoice = setPopups[NOTE_ACC_PARENS].currentChoice;
		slurAppearanceChoice = setPopups[SLUR_APPEARANCE].currentChoice;
		barlineTypeChoice = setPopups[BARLINE_TYPE].currentChoice;
		dynamicSizeChoice = setPopups[DYNAMIC_SIZE].currentChoice;
		beamsetThicknessChoice = setPopups[BEAMSET_THICKNESS].currentChoice;

		if (GetDlgWord(dlog,EDIT9,&value)) edit9Value = value;
		if (GetDlgWord(dlog,EDIT14,&value)) edit14Value = value;

		*finalVal = edit9Value;									/* Unless overridden below */
		*objType = mainSetChoice;
		switch (mainSetChoice) {
			case noteITEM:
				*param = notePropertyChoice;
				if (notePropertyChoice==noteAppearanceITEM) *finalVal = noteAppearanceChoice;
				else if (notePropertyChoice==noteSizeITEM) *finalVal = noteSizeChoice;
				/*
				 *	To tell the caller we're setting velocity "from context", we use an out-
				 *	of-range value that's unlikely to be legal even in a (future) MIDI 2.0.
				 */
				else if (notePropertyChoice==velocityITEM)
					*finalVal = (group1==RAD13_to ? edit14Value : -999);
				else if (notePropertyChoice==parenITEM) *finalVal = accParensChoice;
				break;
			case slurTieITEM:
				*param = slurPropertyChoice;
				if (slurPropertyChoice==slurAppearanceITEM) *finalVal = slurAppearanceChoice;				
				break;
			case textITEM:
			case lyricITEM:
				*param = textPropertyChoice;
				if (textPropertyChoice==styleITEM) *finalVal = textStyleChoice;
				break;
			case chordSymITEM:
				*param = chordSymPropertyChoice;
				break;
			case tempoITEM:
				*param = tempoPropertyChoice;
				if (tempoPropertyChoice==hideMM_ITEM) *finalVal = visibleChoice;
//LogPrintf(LOG_NOTICE, "tempoItem: visibleChoice=%d *finalVal=%d\n", visibleChoice, *finalVal);
				break;
			case barlineITEM:
				*param = barlinePropertyChoice;
				if (barlinePropertyChoice==barlineTypeITEM)	*finalVal = barlineTypeChoice;
				else										*finalVal = visibleChoice;
				break;
			case dynamicITEM:
				*param = dynamicPropertyChoice;
				if (dynamicPropertyChoice==dynamSmallITEM)	*finalVal = dynamicSizeChoice;
				else										*finalVal = visibleChoice;
				break;
			case clefITEM:
				*param = 1;
				*finalVal = visibleChoice;
				break;
			case keySigITEM:
				*param = 1;
				*finalVal = visibleChoice;
				break;
			case timeSigITEM:
				*param = 1;
				*finalVal = visibleChoice;
				break;
			case tupletITEM:
				*param = tupletPropertyChoice;
				if (tupletPropertyChoice==bracketITEM)	*finalVal = visibleChoice;
				else									*finalVal = accNumVisChoice;
				break;
			case beamsetITEM:
				*param = beamsetPropertyChoice;
				if (beamsetPropertyChoice==beamsetThicknessITEM) *finalVal = beamsetThicknessChoice;
				break;
			case drawITEM:
				*param = 1;
				break;
			case patchChangeITEM:
				*param = 1;
				*finalVal = visibleChoice;
				break;
			case panITEM:
				*param = 1;
				*finalVal = visibleChoice;
				break;
			default:
				;
			}

		}
		
	hilitedPop = -1;

	for (index=0;index<HOW_MANY_POPS;index++) {
		DisposePopUp(&setPopups[index]); }
		
	okay = (dialogOver==BUT1_OK);

broken:
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	return(okay);
}
	

/*	Sets up other items to correspond to the main Set popup choice. */	

static void DoMainPopChoice(DialogPtr dlog, short choice)
{
	short		type;
	Handle		hndl;
	Rect		box;
	Str255		str;
	
	switch (choice)	{
		case noteITEM:
			DoPropPopChoice(dlog,setPopups[NOTE_PROPERTY].currentChoice);
			break;
		case slurTieITEM:
			DoPropPopChoice(dlog,setPopups[SLUR_PROPERTY].currentChoice);
			GetDialogItem(dlog,STXT10_Value,&type,&hndl,&box);
			GetIndString(str,SET_STRS,1);								/* "default" */
			SetDialogItemText(hndl,str);
			ShowDialogItem(dlog,STXT10_Value);
			break;
		case textITEM:
		case lyricITEM:
			DoPropPopChoice(dlog,setPopups[TEXT_PROPERTY].currentChoice);
			break;
		case chordSymITEM:
			DoPropPopChoice(dlog,setPopups[CHORDSYM_PROPERTY].currentChoice);
			break;
		case tempoITEM:
			DoPropPopChoice(dlog,setPopups[TEMPO_PROPERTY].currentChoice);
			break;
		case tupletITEM:
			DoPropPopChoice(dlog,setPopups[TUPLET_PROPERTY].currentChoice);
			break;
		case barlineITEM:
			DoPropPopChoice(dlog,setPopups[BARLINE_PROPERTY].currentChoice);
			break;
		case dynamicITEM:
			DoPropPopChoice(dlog,setPopups[DYNAMIC_PROPERTY].currentChoice);
			break;
		case clefITEM:
			DoConstPropPopChoice(dlog,choice);
			break;
		case keySigITEM:
			DoConstPropPopChoice(dlog,choice);
			break;
		case timeSigITEM:
			DoConstPropPopChoice(dlog,choice);
			break;
		case beamsetITEM:
			DoPropPopChoice(dlog,setPopups[BEAMSET_PROPERTY].currentChoice);
			break;
		case drawITEM:
			DoConstPropPopChoice(dlog,choice);
			break;
		case patchChangeITEM:
			DoConstPropPopChoice(dlog,choice);
			break;
		case panITEM:
			DoConstPropPopChoice(dlog,choice);
			break;
		default:
			;
	}
}	
	
	
static void DoConstPropPopChoice(DialogPtr dlog, short choice)
{
	register short index;	
	short		type;
	Handle		hndl;
	Rect		box;
	Str255		str;

			for (index=1;index<HOW_MANY_POPS;index++) {
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False);
			}
			currPropertyPop = -1;
			HideDialogItem(dlog,EDIT9);
			HideDialogItem(dlog,STXT10_Value);
			HideDialogItem(dlog,STXT11_qtr);
			HideDialogItem(dlog,RAD12_fromContext);
			HideDialogItem(dlog,RAD13_to);
			HideDialogItem(dlog,EDIT14);

			if (choice==drawITEM) {
				GetDialogItem(dlog,STXT7_Property,&type,&hndl,&box);
				GetIndString(str,SET_STRS,2);							/* "thickness" */
				SetDialogItemText(hndl,str);
				ShowDialogItem(dlog,STXT7_Property);
				ShowDialogItem(dlog,EDIT9);			
				SelectDialogItemText(dlog,EDIT9,0,ENDTEXT);
			}
			else {
				HideDialogItem(dlog,STXT7_Property);
				ShowPopUp(&setPopups[VISIBILITY],True);
				DrawPopUp(&setPopups[VISIBILITY]);
				currValuePop = VISIBILITY;
			}
}

/*	Sets up other items to correspond to object property popup choice. */	

static void DoPropPopChoice(DialogPtr dlog, short choice)
{
	register short 	index;
	MenuHandle		tempMH;
	short currMainChoice = setPopups[MAIN_SET].currentChoice;
	
	HideDialogItem(dlog,STXT7_Property);
	HideDialogItem(dlog,STXT10_Value);
	HideDialogItem(dlog,RAD12_fromContext);
	HideDialogItem(dlog,RAD13_to);
	HideDialogItem(dlog,EDIT14);

	switch (currMainChoice) {
		case noteITEM:
			for (index=2;index<HOW_MANY_POPS;index++) {		/* Erase all but note property popup. */
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False); }		
			ShowPopUp(&setPopups[NOTE_PROPERTY],True);
			DrawPopUp(&setPopups[NOTE_PROPERTY]);
			currPropertyPop = NOTE_PROPERTY;
			ShowDialogItem(dlog,STXT8_To);
						
			switch (choice)	{
				case voiceITEM:
				case staffITEM:
					HideDialogItem(dlog,STXT11_qtr);
					currValuePop = -1;	
					ShowDialogItem(dlog,EDIT9);
					SelectDialogItemText(dlog,EDIT9,0,ENDTEXT);
					break;
				case velocityITEM:
					HideDialogItem(dlog, STXT8_To);
					HideDialogItem(dlog, EDIT9);
					HideDialogItem(dlog, STXT11_qtr);
					currValuePop = -1;	
					ShowDialogItem(dlog, RAD12_fromContext);
					ShowDialogItem(dlog, RAD13_to);
					ShowDialogItem(dlog, EDIT14);
					SelectDialogItemText(dlog, EDIT14, 0, group1==RAD13_to? ENDTEXT:0);
					break;
				case noteAppearanceITEM:
					HideDialogItem(dlog,EDIT9);
					HideDialogItem(dlog,STXT11_qtr);
					ShowPopUp(&setPopups[NOTE_APPEARANCE],True);
					DrawPopUp(&setPopups[NOTE_APPEARANCE]);
					currValuePop = NOTE_APPEARANCE;
					break;
				case noteSizeITEM:
					HideDialogItem(dlog,EDIT9);
					HideDialogItem(dlog,STXT11_qtr);				
					ShowPopUp(&setPopups[NOTE_SIZE],True);
					DrawPopUp(&setPopups[NOTE_SIZE]);
					currValuePop = NOTE_SIZE;				
					break;
				case stemLengthITEM:
					currValuePop = -1;
					ShowDialogItem(dlog,EDIT9);										
					SelectDialogItemText(dlog,EDIT9,0,ENDTEXT);
					ShowDialogItem(dlog,STXT11_qtr);
					break;
				case parenITEM:
					HideDialogItem(dlog,EDIT9);
					HideDialogItem(dlog,STXT11_qtr);				
					ShowPopUp(&setPopups[NOTE_ACC_PARENS],True);
					DrawPopUp(&setPopups[NOTE_ACC_PARENS]);
					currValuePop = NOTE_ACC_PARENS;				
					break;
			}
			break;

		case textITEM:
		case lyricITEM:
			for (index=1;index<HOW_MANY_POPS;index++) {		/* Erase all but main popup. */
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False);
			}
			ShowDialogItem(dlog,STXT8_To);
			ShowPopUp(&setPopups[TEXT_PROPERTY],True);
			DrawPopUp(&setPopups[TEXT_PROPERTY]);
			currPropertyPop = TEXT_PROPERTY;	
						
			switch (choice)	{
				case styleITEM:
					HideDialogItem(dlog,EDIT9);
					HideDialogItem(dlog,STXT11_qtr);
					
					if (currMainChoice==textITEM) {
						/* If lyric menu is in popup struct, swap in text menu and choice. */
						if (setPopups[TEXT_STYLE].menuID==LYRICSTYLE_POPUP) {
							setPopups[TEXT_STYLE].menuID = TEXTSTYLE_POPUP;
							lyricStyleChoice = setPopups[TEXT_STYLE].currentChoice;
							setPopups[TEXT_STYLE].currentChoice = textStyleChoice;
							tempMH = setPopups[TEXT_STYLE].menu;
							setPopups[TEXT_STYLE].menu = textOrLyricMenu;
							textOrLyricMenu = tempMH;
						}
					}
					/* ...likewise if lyric is main choice */
					else {
						if (setPopups[TEXT_STYLE].menuID==TEXTSTYLE_POPUP) {
							setPopups[TEXT_STYLE].menuID = LYRICSTYLE_POPUP;
							textStyleChoice = setPopups[TEXT_STYLE].currentChoice;
							setPopups[TEXT_STYLE].currentChoice = lyricStyleChoice;
							tempMH = setPopups[TEXT_STYLE].menu;
							setPopups[TEXT_STYLE].menu = textOrLyricMenu;
							textOrLyricMenu = tempMH;
						}
					}
					ResizePopUp(&setPopups[TEXT_STYLE]);
					ChangePopUpChoice(&setPopups[TEXT_STYLE],setPopups[TEXT_STYLE].currentChoice);
					ShowPopUp(&setPopups[TEXT_STYLE],True);
					DrawPopUp(&setPopups[TEXT_STYLE]);
					currValuePop = TEXT_STYLE;
					break;
				case vPosAboveITEM:
				case vPosBelowITEM:
				case hPosOffsetITEM:
					currValuePop = -1;
					ShowDialogItem(dlog,EDIT9);
					SelectDialogItemText(dlog,EDIT9,0,ENDTEXT);
					ShowDialogItem(dlog,STXT11_qtr);
					break;
			}
			break;
			
		case chordSymITEM:
			for (index=1;index<HOW_MANY_POPS;index++) {		/* Erase all but main popup. */
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False);
			}
			ShowDialogItem(dlog,STXT8_To);
			ShowPopUp(&setPopups[CHORDSYM_PROPERTY],True);
			DrawPopUp(&setPopups[CHORDSYM_PROPERTY]);
			currPropertyPop = CHORDSYM_PROPERTY;

			currValuePop = -1;
			ShowDialogItem(dlog,EDIT9);
			SelectDialogItemText(dlog,EDIT9,0,ENDTEXT);
			ShowDialogItem(dlog,STXT11_qtr);
			break;
			
		case tempoITEM:
			for (index=1;index<HOW_MANY_POPS;index++) {		/* Erase all but main popup. */
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False);
			}
			ShowDialogItem(dlog,STXT8_To);
			ShowPopUp(&setPopups[TEMPO_PROPERTY],True);
			DrawPopUp(&setPopups[TEMPO_PROPERTY]);
			currPropertyPop = TEMPO_PROPERTY;

			switch (choice)	{
				case vPosAboveITEM:
				case vPosBelowITEM:
				case hPosOffsetITEM:
					currValuePop = -1;
					ShowDialogItem(dlog,EDIT9);
					SelectDialogItemText(dlog,EDIT9,0,ENDTEXT);
					ShowDialogItem(dlog,STXT11_qtr);
					break;
				case hideMM_ITEM:
					ShowPopUp(&setPopups[VISIBILITY],True);
					DrawPopUp(&setPopups[VISIBILITY]);
					currValuePop = VISIBILITY;
					HideDialogItem(dlog,EDIT9);
					HideDialogItem(dlog,STXT11_qtr);
					break;
			}

			break;

		case tupletITEM:
			for (index=1;index<HOW_MANY_POPS;index++) {		/* Erase all but main popup. */
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False);
			}
			ShowDialogItem(dlog,STXT8_To);
			ShowPopUp(&setPopups[TUPLET_PROPERTY],True);
			DrawPopUp(&setPopups[TUPLET_PROPERTY]);
			currPropertyPop = TUPLET_PROPERTY;	
			HideDialogItem(dlog,EDIT9);
			HideDialogItem(dlog,STXT11_qtr);
						
			switch (choice)	{
				case bracketITEM:
					ShowPopUp(&setPopups[VISIBILITY],True);
					DrawPopUp(&setPopups[VISIBILITY]);
					currValuePop = VISIBILITY;
					break;
				case accNumsITEM:
					ShowPopUp(&setPopups[ACCNUMVIS],True);
					DrawPopUp(&setPopups[ACCNUMVIS]);
					currValuePop = ACCNUMVIS;
					break;
			}
			break;

		case slurTieITEM:
			for (index=1;index<HOW_MANY_POPS;index++) {		/* Erase all but main popup. */
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False);
			}
			ShowDialogItem(dlog,STXT8_To);
			ShowPopUp(&setPopups[SLUR_PROPERTY],True);
			DrawPopUp(&setPopups[SLUR_PROPERTY]);
			currPropertyPop = SLUR_PROPERTY;	
			HideDialogItem(dlog,EDIT9);
			HideDialogItem(dlog,STXT11_qtr);
						
			switch (choice)	{
				short type; Handle hndl; Rect box; Str255 str;
				
				case shapeposITEM:
					GetDialogItem(dlog,STXT10_Value,&type,&hndl,&box);
					GetIndString(str,SET_STRS,1);						/* "default" */
					SetDialogItemText(hndl,str);
					ShowDialogItem(dlog,STXT10_Value);
					currValuePop = -1;
					break;
				case slurAppearanceITEM:
					ShowPopUp(&setPopups[SLUR_APPEARANCE],True);
					DrawPopUp(&setPopups[SLUR_APPEARANCE]);
					currValuePop = SLUR_APPEARANCE;
					break;
			}
			break;
			
		case barlineITEM:
			for (index=1;index<HOW_MANY_POPS;index++) {		/* Erase all but main popup. */
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False);
			}
			ShowDialogItem(dlog,STXT8_To);
			ShowPopUp(&setPopups[BARLINE_PROPERTY],True);
			DrawPopUp(&setPopups[BARLINE_PROPERTY]);
			currPropertyPop = BARLINE_PROPERTY;	
			HideDialogItem(dlog,EDIT9);
			HideDialogItem(dlog,STXT11_qtr);
						
			switch (choice)	{
				case barlineVisibleITEM:
					ShowPopUp(&setPopups[VISIBILITY],True);
					DrawPopUp(&setPopups[VISIBILITY]);
					currValuePop = VISIBILITY;
					break;
				case barlineTypeITEM:
					ShowPopUp(&setPopups[BARLINE_TYPE],True);
					DrawPopUp(&setPopups[BARLINE_TYPE]);
					currValuePop = BARLINE_TYPE;
					break;
			}
			break;

		case dynamicITEM:									/* no property popup for this yet */
			for (index=1;index<HOW_MANY_POPS;index++) {		/* Erase all but main popup. */
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False);
			}
			ShowDialogItem(dlog,STXT8_To);
			ShowPopUp(&setPopups[DYNAMIC_PROPERTY],True);
			DrawPopUp(&setPopups[DYNAMIC_PROPERTY]);
			currPropertyPop = DYNAMIC_PROPERTY;	
			HideDialogItem(dlog,EDIT9);
			HideDialogItem(dlog,STXT11_qtr);
			switch (choice)	{
				case dynamVisibleITEM:
					ShowPopUp(&setPopups[VISIBILITY],True);
					DrawPopUp(&setPopups[VISIBILITY]);
					currValuePop = VISIBILITY;
					break;
				case dynamSmallITEM:
					ShowPopUp(&setPopups[DYNAMIC_SIZE],True);
					DrawPopUp(&setPopups[DYNAMIC_SIZE]);
					currValuePop = DYNAMIC_SIZE;
					break;
			}
			break;
			
		case beamsetITEM:
			for (index=1;index<HOW_MANY_POPS;index++) {		/* Erase all but main popup. */
				EraseRect(&setPopups[index].shadow);
				ShowPopUp(&setPopups[index],False);
			}
			ShowDialogItem(dlog,STXT8_To);
			ShowPopUp(&setPopups[BEAMSET_PROPERTY],True);
			DrawPopUp(&setPopups[BEAMSET_PROPERTY]);
			currPropertyPop = BEAMSET_PROPERTY;	
			HideDialogItem(dlog,EDIT9);
			HideDialogItem(dlog,STXT11_qtr);
			switch (choice)	{
				case beamsetThicknessITEM:
					ShowPopUp(&setPopups[BEAMSET_THICKNESS],True);
					DrawPopUp(&setPopups[BEAMSET_THICKNESS]);
					currValuePop = BEAMSET_THICKNESS;
					break;
			}
			break;

		case clefITEM:					/* no property popup for this yet */
			break;

		case keySigITEM:				/* no property popup for this yet */
			break;

		case timeSigITEM:				/* no property popup for this yet */
			break;
			
		case patchChangeITEM:			/* no property popup for this yet */
			break;

		case panITEM:					/* no property popup for this yet */
			break;

	}
}	
	
	
/* Lets user select popup items using arrow keys. Calls FindMainEnabledItem to skip
disabled items in the main popup; otherwise, doesn't work if the menu has any
disabled items, unless they are the '--------' kind and are not the first or last
items. */

void DoSetArrowKey(DialogPtr dlog, short whichKey)
{
	short lastItemNum, newChoice, oldHilitedPop, newHilitedPop;
	UserPopUp *p;
	Str255 str;
					
	switch (whichKey) {
		case UPARROWKEY:
		case DOWNARROWKEY:
			if (hilitedPop==-1) {
				hilitedPop = MAIN_SET;
				HilitePopUp(&setPopups[MAIN_SET], True);
			}
			else {
				p = &setPopups[hilitedPop];
				newChoice = p->currentChoice;
				lastItemNum = CountMenuItems(p->menu);
				if (whichKey == UPARROWKEY) {				
					newChoice -= 1;
					if (newChoice == 0) return;					/* Top of menu reached; don't wrap around. */
        			GetMenuItemText(p->menu, newChoice, str);
        			if (str[1] == '-') newChoice -= 1;			/* Skip over the dash item */
					if (hilitedPop == MAIN_SET)
						newChoice = FindMainEnabledItem(newChoice, True, False);
					if (newChoice <= 0) return;					/* Top of menu reached; don't wrap around. */
				}
				else {
					newChoice += 1;
					if (newChoice == lastItemNum + 1) return;	/* Bottom of menu reached; don't wrap around. */
        			GetMenuItemText(p->menu, newChoice, str);
        			if (str[1] == '-') newChoice += 1;			/* Skip over the dash item */
					if (hilitedPop == MAIN_SET)
						newChoice = FindMainEnabledItem(newChoice, False, False);
					if (newChoice <= 0) return;					/* Bottom of menu reached; don't wrap around. */
				}

				ChangePopUpChoice(p,newChoice);

				/* Make subordinate items agree with the new choice. */
				if (p == &setPopups[MAIN_SET]) DoMainPopChoice(dlog,newChoice);
				else if (p == &setPopups[NOTE_PROPERTY]
				||       p == &setPopups[TEXT_PROPERTY]
				||       p == &setPopups[CHORDSYM_PROPERTY]
				||       p == &setPopups[TEMPO_PROPERTY]
				||       p == &setPopups[TUPLET_PROPERTY]
				||       p == &setPopups[BARLINE_PROPERTY]
				||       p == &setPopups[SLUR_PROPERTY]
				||       p == &setPopups[DYNAMIC_PROPERTY]
				||       p == &setPopups[BEAMSET_PROPERTY])
								DoPropPopChoice(dlog,newChoice);
				HilitePopUp(p,True);
			}
			return;									/* avoid the statements after switch(whichKey) */
						
		case LEFTARROWKEY:
			if (hilitedPop == -1) {					/* If no popup is hilited, hilite main popup. */
				oldHilitedPop = -1;
				newHilitedPop = MAIN_SET;
			}
			else if (hilitedPop == currPropertyPop) {
				oldHilitedPop = currPropertyPop;
				newHilitedPop = MAIN_SET;						/* Move to main popup. */
			}
			else if (hilitedPop == currValuePop) {
				oldHilitedPop = currValuePop;
				if (currPropertyPop>-1) newHilitedPop = currPropertyPop;	/* Move to current property popup if it's visible. */
				else newHilitedPop = MAIN_SET;					/* Otherwise move to main popup. */
			}
			else if (hilitedPop == MAIN_SET) return;			/* Keep it hilited. */
			break;
			
		case RIGHTARROWKEY:
			if (hilitedPop == -1) {	/* If no popup is hilited, hilite rightmost (Value) popup. */
				oldHilitedPop = -1;
				if (currValuePop>-1) newHilitedPop = currValuePop;
				else if (currPropertyPop>-1) newHilitedPop = currPropertyPop;
				else newHilitedPop = MAIN_SET;
			}
			else if (hilitedPop == MAIN_SET) {
				oldHilitedPop = MAIN_SET;
				if (currPropertyPop>-1) newHilitedPop = currPropertyPop;
				else if (currValuePop>-1) newHilitedPop = currValuePop;
				else return;									/* Main popup stays hilited. */
			}
			else if (hilitedPop == currPropertyPop) {
				if (currValuePop>-1) {
					oldHilitedPop = currPropertyPop;
					newHilitedPop = currValuePop;
				}
				else return;									/* Don't remove hiliting. */
			}
			else if (hilitedPop == currValuePop)
				return;											/* Don't remove hiliting. */
			break;			
	}	/* end switch (whichKey) */
				
	if (oldHilitedPop>-1) HilitePopUp(&setPopups[oldHilitedPop],False);
	HilitePopUp(&setPopups[newHilitedPop],True);
	hilitedPop = newHilitedPop;
}


/* ----------------------------------------------------------------------------- DoSet -- */
/* Handle the "QuickChange" (originally "Set") menu item. */

void DoSet(Document *doc)
{
	short newTypeSet, newParamSet, newValSet, appearance, subtype;
	register Boolean didAnything; 

	if (SetDialog(doc, &newTypeSet, &newParamSet, &newValSet)) {
		PrepareUndo(doc, doc->selStartL, U_Set, 34);			/* "Undo QuickChange" */
		WaitCursor();
		didAnything = False;

		switch (newTypeSet) {
			case noteITEM:
				switch (newParamSet) {
					case voiceITEM:
						didAnything = SetSelVoice(doc, newValSet);
						break;
					case staffITEM:
						didAnything = SetSelStaff(doc, newValSet);
						break;
					case velocityITEM:
						if (newValSet<0)
							didAnything = SetVelFromContext(doc, True);
						else
							didAnything = SetSelVelocity(doc, newValSet);
						break;
					case noteAppearanceITEM:
						if (newValSet==normalITEM) appearance = NORMAL_VIS;
						else if (newValSet==xShapeITEM) appearance = X_SHAPE;
						else if (newValSet==harmonicITEM) appearance = HARMONIC_SHAPE;
						else if (newValSet==slashITEM) appearance = SLASH_SHAPE;
						else if (newValSet==hollowSquareITEM) appearance = SQUAREH_SHAPE;
						else if (newValSet==filledSquareITEM) appearance = SQUAREF_SHAPE;
						else if (newValSet==hollowDiamondITEM) appearance = DIAMONDH_SHAPE;
						else if (newValSet==filledDiamondITEM) appearance = DIAMONDF_SHAPE;
						else if (newValSet==halfNoteITEM) appearance = HALFNOTE_SHAPE;
						else if (newValSet==invisibleITEM) appearance = NO_VIS;
						else if (newValSet==allInvisITEM) appearance = NOTHING_VIS;
						else {
							LogPrintf(LOG_WARNING, "DoSet: illegal noteAppearanceITEM %d\n",
										newValSet);
							SysBeep(1);
							return;
						}
						didAnything = SetSelNRAppear(doc, appearance);
						break;
					case noteSizeITEM:
						didAnything = SetSelNRSmall(doc, newValSet-1);
						break;
					case stemLengthITEM:
						didAnything = SetSelStemlen(doc, (unsigned STDIST)qd2std(newValSet));
						break;
					case parenITEM:
						didAnything = SetSelNRParens(doc, newValSet-1);
						break;
					default:
						;
				}
				break;
				
			case barlineITEM:
				switch (newParamSet) {
					case barlineVisibleITEM:
						didAnything = SetSelMeasVisible(doc, newValSet);
						break;
					case barlineTypeITEM:
						{
							/* BAR_HEAVYDBL isn't used, so it's not in the popup. */
							short measSubType = newValSet;
							if (measSubType>=BAR_HEAVYDBL) measSubType++;
							didAnything = SetSelMeasType(doc, measSubType);
						}
						break;
					default:
						;
				}
				break;
				
			case dynamicITEM:
				switch (newParamSet) {
					case dynamVisibleITEM:
						didAnything = SetSelDynamVisible(doc, newValSet);
						break;
					case dynamSmallITEM:
						didAnything = SetSelDynamSmall(doc, newValSet-1);
						break;
					default:
						;
				}
				break;
				
			case clefITEM:
				didAnything = SetSelClefVisible(doc, newValSet);
				break;
				
			case keySigITEM:
				didAnything = SetSelKeySigVisible(doc, newValSet);
				break;
				
			case timeSigITEM:
				didAnything = SetSelTimeSigVisible(doc, newValSet);
				break;
				
			case tupletITEM:
				switch (newParamSet) {
					case bracketITEM:
						didAnything = SetSelTupBracket(doc, newValSet);
						break;
					case accNumsITEM:
						didAnything = SetSelTupNums(doc, newValSet);
						break;
					default:
						;
				}
				break;
				
			case beamsetITEM:
				didAnything = SetSelBeamsetThin(doc, newValSet==2);
				break;
				
			case textITEM:
			case lyricITEM:
				subtype = (newTypeSet==textITEM? GRString : GRLyric);
				switch (newParamSet) {
					case vPosAboveITEM:
					case vPosBelowITEM:
						didAnything = SetSelGraphicY(doc, qd2std(newValSet), subtype,
																(newParamSet==vPosAboveITEM));
						break;
					case hPosOffsetITEM:
						didAnything = SetSelGraphicX(doc, qd2std(newValSet), subtype);
						break;
					case styleITEM:
						didAnything = SetSelGraphicStyle(doc, newValSet, subtype);
						break;
					default:
						;
				}
				break;
				
			case chordSymITEM:
				switch (newParamSet) {
					case vPosAboveITEM:
					case vPosBelowITEM:
						didAnything = SetSelGraphicY(doc, qd2std(newValSet), GRChordSym,
																(newParamSet==vPosAboveITEM));
						break;
					case hPosOffsetITEM:
						didAnything = SetSelGraphicX(doc, qd2std(newValSet), GRChordSym);
						break;
					default:
						;
				}
				break;
				
			case tempoITEM:
//LogPrintf(LOG_NOTICE, "DoSet: tempoITEM: newValSet=%d\n", newValSet);
				switch (newParamSet) {
					case vPosAboveITEM:
					case vPosBelowITEM:
						didAnything = SetSelTempoY(doc, qd2std(newValSet),
														(newParamSet==vPosAboveITEM));
						break;
					case hPosOffsetITEM:
						didAnything = SetSelTempoX(doc, qd2std(newValSet));
						break;
					case hideMM_ITEM:
						didAnything = SetSelTempoVisible(doc, (newValSet==1));
						break;
					default:
						;
				}
				break;
				
			case slurTieITEM:
				switch (newParamSet) {
					case shapeposITEM:
						didAnything = SetSelSlurShape(doc);
						break;
					case slurAppearanceITEM:
						didAnything = SetSelSlurAppear(doc, newValSet-1);
						break;
					default:
						;
				}
				break;
			case drawITEM:
					didAnything = SetSelLineThickness(doc, newValSet); 
				break;
			case patchChangeITEM:
					didAnything = SetSelPatchChangeVisible(doc, newValSet); 
				break;
			case panITEM:
					didAnything = SetSelPanVisible(doc, newValSet); 
				break;
			default:
				;
		}
		
		if (didAnything) {
			doc->changed = True;
			InvalWindow(doc);
		}
	}
}
