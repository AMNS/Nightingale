/* ModNRPopUp.c for Nightingale - for use with accompanying graphic MDEF. */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1990-2000 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
/* Includes code for managing key presses that select from a popup menu of note
modifiers. This code parallels that for duration popups in DurationPopUp.c.
(That file also includes the more generic code for creating graphic popups
with the MDEF -- which really should go in its own file.)  Also in the current
file are dialog routines invoked when user double-clicks a modifier or chooses
the Add Modifiers command. I thought it best to keep these here, since they need
static vars. The whole mechanism for keyboard selection should be generalized to
work with all the graphic popups, but no time for that now.  -- John Gibson, 8/4/00 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


extern Boolean TranslatePalChar(short *, short, Boolean);

/* ASCII decimal for the modNR symbols in popDynMod font */
static enum {
	POP_MOD_FINGERING_0 = 97,
	POP_MOD_FINGERING_1,
	POP_MOD_FINGERING_2,
	POP_MOD_FINGERING_3,
	POP_MOD_FINGERING_4,
	POP_MOD_FINGERING_5,
	POP_MOD_STACCATO,
	POP_MOD_WEDGE,
	POP_MOD_HEAVYACCENT,
	POP_MOD_ACCENT,
	POP_MOD_HEAVYACC_STACC,
	POP_MOD_UPBOW,
	POP_MOD_DOWNBOW,
	POP_MOD_TENUTO,
	POP_MOD_TRILL,
	POP_MOD_MORDENT,
	POP_MOD_LONG_INVMORDENT,
	POP_MOD_TURN,
	POP_MOD_FERMATA,
	POP_MOD_INV_MORDENT,
	POP_MOD_TREMOLO,			/* NOTE: this doesn't map to just one mod code; see NTypes.h */
	POP_MOD_PLUS,
	POP_MOD_CIRCLE
} E_ModNRSyms;


typedef struct {
	Byte modCode;					/* from NTypes.h */
	unsigned char symCode;		/* from <symtable> in vars.h */
} MODNR_POPKEY;


/* ---------------------------- ModNRPopupKey & Friends --------------------------- */

/* Allocates 1-based array mapping menu item numbers to MODNR_POPKEY structs.
Caller should dispose this pointer when disposing its associated popop.
Assumes graphic popup already inited.

CAUTION: The symCodes assigned here _must_ be consistent with those that appear
in the <symtable> in vars.h. */

static MODNR_POPKEY *InitModNRPopupKey(PGRAPHIC_POPUP	gp);
static MODNR_POPKEY *InitModNRPopupKey(PGRAPHIC_POPUP	gp)
{
	short				i;
	char				*q;
	MODNR_POPKEY	*pkeys, *p;
	
	if (gp==NULL || gp->numItems==0) return NULL;
	
	pkeys = (MODNR_POPKEY *) NewPtr((Size)(sizeof(MODNR_POPKEY)*(gp->numItems+1)));
	if (pkeys==NULL) goto broken;
	
	p = &pkeys[1];											/* point to 1st real entry */
	q = gp->itemChars;
	for (i=0; i<gp->numItems; i++, q++, p++) {
		switch (*q) {
			case POP_MOD_FINGERING_0:
				p->modCode = 0;
				p->symCode = '0';
				break;
			case POP_MOD_FINGERING_1:
				p->modCode = 1;
				p->symCode = '1';
				break;
			case POP_MOD_FINGERING_2:
				p->modCode = 2;
				p->symCode = '2';
				break;
			case POP_MOD_FINGERING_3:
				p->modCode = 3;
				p->symCode = '3';
				break;
			case POP_MOD_FINGERING_4:
				p->modCode = 4;
				p->symCode = '4';
				break;
			case POP_MOD_FINGERING_5:
				p->modCode = 5;
				p->symCode = '5';
				break;
			case POP_MOD_STACCATO:
				p->modCode = MOD_STACCATO;
				p->symCode = ';';
				break;
			case POP_MOD_WEDGE:
				p->modCode = MOD_WEDGE;
				p->symCode = 0xB7;
				break;
			case POP_MOD_HEAVYACCENT:
				p->modCode = MOD_HEAVYACCENT;
				p->symCode = '^';
				break;
			case POP_MOD_ACCENT:
				p->modCode = MOD_ACCENT;
				p->symCode = '\'';
				break;
			case POP_MOD_HEAVYACC_STACC:
				p->modCode = MOD_HEAVYACC_STACC;
				p->symCode = 0xDF;
				break;
			case POP_MOD_UPBOW:
				p->modCode = MOD_UPBOW;
				p->symCode = 'V';
				break;
			case POP_MOD_DOWNBOW:
				p->modCode = MOD_DOWNBOW;
				p->symCode = 'D';
				break;
			case POP_MOD_TENUTO:
				p->modCode = MOD_TENUTO;
				p->symCode = '-';
				break;
			case POP_MOD_TRILL:
				p->modCode = MOD_TRILL;
				p->symCode = 'u';
				break;
			case POP_MOD_MORDENT:
				p->modCode = MOD_MORDENT;
				p->symCode = 0xB5;
				break;
			case POP_MOD_LONG_INVMORDENT:
				p->modCode = MOD_LONG_INVMORDENT;
				p->symCode = 0xA3;
				break;
			case POP_MOD_TURN:
				p->modCode = MOD_TURN;
				p->symCode = 0xA0;
				break;
			case POP_MOD_FERMATA:
				p->modCode = MOD_FERMATA;
				p->symCode = 'U';
				break;
			case POP_MOD_INV_MORDENT:
				p->modCode = MOD_INV_MORDENT;
				p->symCode = 'm';
				break;
			case POP_MOD_TREMOLO:
				p->modCode = MOD_TREMOLO1;
				p->symCode = '/';					/* NOTE: This is for MOD_TREMOLO1 */
				break;
			case POP_MOD_PLUS:
				p->modCode = MOD_PLUS;
				p->symCode = '+';
				break;
			case POP_MOD_CIRCLE:
				p->modCode = MOD_CIRCLE;
				p->symCode = 'o';
				break;
			
			default:
				p->modCode = 255;
				p->symCode = 0;
				break;	/* "can't happen" */
		}
	}
	
broken:
	return pkeys;
}


/* Given a modifier popup and associated MODNR_POPKEY array, return the index
into that array of the given modCode. The index will be a menu item number. */

static short GetModNRPopItem(PGRAPHIC_POPUP p, MODNR_POPKEY *pk, short modCode)
{
	short				i;
	MODNR_POPKEY	*q;
	
	for (i=1, q=&pk[1]; i<=p->numItems; i++, q++)
		if (q->modCode==modCode)
			return i;
	return NOMATCH;
}


/* Given a character code <theChar>, a popup <p> and its associated MODNR_POPKEY
array <pk>, determine if the character should select a new modifier from the popup.
If it should, call SetGPopUpChoice and return TRUE; if not, return FALSE. ModNRPopupKey
calls TranslatePalChar before doing anything, so it recognizes remapped modifier key
equivalents. */

Boolean ModNRPopupKey(PGRAPHIC_POPUP p, MODNR_POPKEY *pk, unsigned char theChar)
{
	short	i, newItem, newModCode;
	short	intChar;
	
	/* remap theChar according to the 'PLMP' resource */
	intChar = (short)theChar;
	TranslatePalChar(&intChar, 0, FALSE);
	theChar = (unsigned char) intChar;
	
	newModCode = NOMATCH;
	for (i=1; i<=p->numItems; i++) {
		if (pk[i].symCode==theChar) {
			newModCode = pk[i].modCode;
			break;
		}
	}
	if (newModCode==NOMATCH)
		return FALSE;										/* no action for this key */
	
	newItem = GetModNRPopItem(p, pk, newModCode);
	if (newItem && newItem!=p->currentChoice)
		SetGPopUpChoice(p, newItem);

	return TRUE;											/* the keystroke was directed at popup */
}



/* --------------------------------------------------------------------------------- */
/* SetModNRDialog and AddModNRDialog */

static GRAPHIC_POPUP	modNRPop, *curPop;
static MODNR_POPKEY	*popKeysModNR;
static short			popUpHilited = TRUE;

#define CurEditField(dlog) (((DialogPeek)(dlog))->editField+1)

static enum {
	MODNR_POP_DI=4			/* same for both dialogs */
} E_ModNRPopItems;


/* ------------------------------------------------------------------ ModNRFilter -- */

static pascal Boolean ModNRFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static pascal Boolean ModNRFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	WindowPtr		w;
	short				ch, ans;
	Point				where;
	GrafPtr			oldPort;
	
	w = (WindowPtr)(evt->message);
	switch (evt->what) {
		case updateEvt:
			if (w==GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));			
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, OK, TRUE);
				DrawGPopUp(curPop);		
				HiliteGPopUp(curPop, popUpHilited);
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);
				*itemHit = 0;
				return TRUE;
			}
			break;
		case activateEvt:
			if (w==GetDialogWindow(dlog))
				SetPort(GetDialogWindowPort(dlog));
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			if (PtInRect(where, &curPop->box)) {
				DoGPopUp(curPop);
				*itemHit = MODNR_POP_DI;
				return TRUE;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, FALSE))
				return TRUE;
			ch = (unsigned char)evt->message;
			ans = ModNRPopupKey(curPop, popKeysModNR, ch);
			*itemHit = ans? MODNR_POP_DI : 0;
			HiliteGPopUp(curPop, TRUE);
			return TRUE;
			break;
	}
	
	return FALSE;
}


/* --------------------------------------------------------------- SetModNRDialog -- */

Boolean SetModNRDialog(Byte *modCode)
{	
	DialogPtr dlog;
	short ditem=Cancel,type,oldResFile;
	short choice;
	Boolean dialogOver;
	Handle hndl; Rect box;
	GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(ModNRFilter);
	if (filterUPP == NULL) {
		MissingDialog(SETMOD_DLOG);
		return FALSE;
	}
	
	dlog = GetNewDialog(SETMOD_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SETMOD_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);									/* popup code uses Get1Resource */

	modNRPop.menu = NULL;										/* NULL makes any goto broken safe */
	modNRPop.itemChars = NULL;
	popKeysModNR = NULL;

	GetDialogItem(dlog, MODNR_POP_DI, &type, &hndl, &box);
	if (!InitGPopUp(&modNRPop, TOP_LEFT(box), MODIFIER_MENU, 1)) goto broken;
	popKeysModNR = InitModNRPopupKey(&modNRPop);
	if (popKeysModNR==NULL) goto broken;
	curPop = &modNRPop;
	
	choice = GetModNRPopItem(curPop, popKeysModNR, *modCode);
	if (choice==NOMATCH) choice = 1;
	SetGPopUpChoice(curPop, choice);

	CenterWindow(GetDialogWindow(dlog), 100);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	dialogOver = FALSE;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
			case Cancel:
				dialogOver = TRUE;
				break;
			default:
				;
		}
	}
	if (ditem==OK)
		*modCode = popKeysModNR[curPop->currentChoice].modCode;
		
broken:
	DisposeGPopUp(&modNRPop);
	if (popKeysModNR) DisposePtr((Ptr)popKeysModNR);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}


/* --------------------------------------------------------------- AddModNRDialog -- */
/* (Maybe silly to keep this separate from SetModNRDialog, but we might want to
	change it someday.)  -JGG */

Boolean AddModNRDialog(Byte *modCode)
{	
	DialogPtr dlog;
	short ditem=Cancel,type,oldResFile;
	short choice;
	Boolean dialogOver;
	Handle hndl; Rect box;
	GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(ModNRFilter);
	if (filterUPP == NULL) {
		MissingDialog(ADDMOD_DLOG);
		return FALSE;
	}
	
	dlog = GetNewDialog(ADDMOD_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(ADDMOD_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);									/* popup code uses Get1Resource */

	modNRPop.menu = NULL;										/* NULL makes any goto broken safe */
	modNRPop.itemChars = NULL;
	popKeysModNR = NULL;

	GetDialogItem(dlog, MODNR_POP_DI, &type, &hndl, &box);
	if (!InitGPopUp(&modNRPop, TOP_LEFT(box), MODIFIER_MENU, 1)) goto broken;
	popKeysModNR = InitModNRPopupKey(&modNRPop);
	if (popKeysModNR==NULL) goto broken;
	curPop = &modNRPop;
	
	choice = GetModNRPopItem(curPop, popKeysModNR, *modCode);
	if (choice==NOMATCH) choice = 1;
	SetGPopUpChoice(curPop, choice);

	CenterWindow(GetDialogWindow(dlog), 100);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	
	dialogOver = FALSE;
	while (!dialogOver) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
			case Cancel:
				dialogOver = TRUE;
				break;
			default:
				;
		}
	}
	if (ditem==OK)
		*modCode = popKeysModNR[curPop->currentChoice].modCode;
		
broken:
	DisposeGPopUp(&modNRPop);
	if (popKeysModNR) DisposePtr((Ptr)popKeysModNR);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}

