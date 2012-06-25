/* DynamicPopUp.c for Nightingale - for use with accompanying graphic MDEF. */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1990-2000 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
/* Includes code for managing key presses that select from a popup menu of dynamics.
This code parallels that for duration popups in DurationPopUp.c, and (especially)
for modifier popups in ModNRPopUp.c. Also in this file are dialog routines invoked
when user double-clicks a dynamic.                      -- John Gibson, 8/5/00 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


extern Boolean TranslatePalChar(short *, short, Boolean);

/* ASCII decimal for the dynamic symbols in popDynMod font */
static enum {
	POP_PPP_DYNAM = 65,
	POP_PP_DYNAM,
	POP_P_DYNAM,
	POP_MP_DYNAM,
	POP_MF_DYNAM,
	POP_F_DYNAM,
	POP_FF_DYNAM,
	POP_FFF_DYNAM,
	POP_SF_DYNAM
} E_DynPopupItems;


typedef struct {
	SignedByte dynamicType;		/* from NTypes.h */
	unsigned char symCode;		/* from <symtable> in vars.h */
} DYN_POPKEY;


/* ------------------------ DynamicPopupKey & Friends --------------------------- */

/* Allocates 1-based array mapping menu item numbers to DYN_POPKEY structs.
Caller should dispose this pointer when disposing its associated popop.
Assumes graphic popup already inited.

CAUTION: The symCodes assigned here _must_ be consistent with those that appear
in the <symtable> in vars.h. */

static DYN_POPKEY *InitDynamicPopupKey(PGRAPHIC_POPUP	gp);
static DYN_POPKEY *InitDynamicPopupKey(PGRAPHIC_POPUP	gp)
{
	short			i;
	char			*q;
	DYN_POPKEY	*pkeys, *p;
	
	if (gp==NULL || gp->numItems==0) return NULL;
	
	pkeys = (DYN_POPKEY *) NewPtr((Size)(sizeof(DYN_POPKEY)*(gp->numItems+1)));
	if (pkeys==NULL) goto broken;
	
	p = &pkeys[1];											/* point to 1st real entry */
	q = gp->itemChars;
	for (i=0; i<gp->numItems; i++, q++, p++) {
		switch (*q) {
			case POP_PPP_DYNAM:
				p->dynamicType = PPP_DYNAM;
				p->symCode = 0xB8;
				break;
			case POP_PP_DYNAM:
				p->dynamicType = PP_DYNAM;
				p->symCode = 0xB9;
				break;
			case POP_P_DYNAM:
				p->dynamicType = P_DYNAM;
				p->symCode = 'p';
				break;
			case POP_MP_DYNAM:
				p->dynamicType = MP_DYNAM;
				p->symCode = 'P';
				break;
			case POP_MF_DYNAM:
				p->dynamicType = MF_DYNAM;
				p->symCode = 'F';
				break;
			case POP_F_DYNAM:
				p->dynamicType = F_DYNAM;
				p->symCode = 'f';
				break;
			case POP_FF_DYNAM:
				p->dynamicType = FF_DYNAM;
				p->symCode = 0xC4;
				break;
			case POP_FFF_DYNAM:
				p->dynamicType = FFF_DYNAM;
				p->symCode = 0xEC;
				break;
			case POP_SF_DYNAM:
				p->dynamicType = SF_DYNAM;
				p->symCode = 'S';
				break;
			
			default:
				p->dynamicType = -1;
				p->symCode = 0;
				break;	/* "can't happen" */
		}
	}
	
broken:
	return pkeys;
}


/* Given a dynamic popup and associated DYN_POPKEY array, return the index
into that array of the given dynamicType. The index will be a menu item number. */

static short GetDynamicPopItem(PGRAPHIC_POPUP p, DYN_POPKEY *pk, short dynamicType)
{
	short			i;
	DYN_POPKEY	*q;
	
	for (i=1, q=&pk[1]; i<=p->numItems; i++, q++)
		if (q->dynamicType==dynamicType)
			return i;
	return NOMATCH;
}


/* Given a character code <theChar>, a popup <p> and its associated DYN_POPKEY
array <pk>, determine if the character should select a new dynamic from the popup.
If it should, call SetGPopUpChoice and return TRUE; if not, return FALSE.
DynamicPopupKey calls TranslatePalChar before doing anything, so it recognizes
remapped modifier key equivalents. */

Boolean DynamicPopupKey(PGRAPHIC_POPUP p, DYN_POPKEY *pk, unsigned char theChar)
{
	short	i, newItem, newDynamicType;
	short	intChar;
	
	/* remap theChar according to the 'PLMP' resource */
	intChar = (short)theChar;
	TranslatePalChar(&intChar, 0, FALSE);
	theChar = (unsigned char) intChar;
	
	newDynamicType = NOMATCH;
	for (i=1; i<=p->numItems; i++) {
		if (pk[i].symCode==theChar) {
			newDynamicType = pk[i].dynamicType;
			break;
		}
	}
	if (newDynamicType==NOMATCH)
		return FALSE;										/* no action for this key */
	
	newItem = GetDynamicPopItem(p, pk, newDynamicType);
	if (newItem && newItem!=p->currentChoice)
		SetGPopUpChoice(p, newItem);

	return TRUE;											/* the keystroke was directed at popup */
}



/* --------------------------------------------------------------------------------- */
/* SetDynamicDialog */

static GRAPHIC_POPUP	dynamicPop, *curPop;
static DYN_POPKEY		*popKeysDynamic;
static short			popUpHilited = TRUE;

static enum {
	DYNAM_POP_DI=4
} E_SetDynItems;


static pascal Boolean DynamicFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static pascal Boolean DynamicFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
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
				*itemHit = DYNAM_POP_DI;
				return TRUE;
			}
			break;
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, FALSE))
				return TRUE;
			ch = (unsigned char)evt->message;
			ans = DynamicPopupKey(curPop, popKeysDynamic, ch);
			*itemHit = ans? DYNAM_POP_DI : 0;
			HiliteGPopUp(curPop, TRUE);
			return TRUE;
			break;
	}
	
	return FALSE;
}


Boolean SetDynamicDialog(SignedByte *dynamicType)
{	
	DialogPtr dlog;
	short ditem=Cancel,type,oldResFile;
	short choice;
	Boolean dialogOver;
	Handle hndl; Rect box;
	GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(DynamicFilter);
	if (filterUPP == NULL) {
		MissingDialog(SETDYN_DLOG);
		return FALSE;
	}
	
	dlog = GetNewDialog(SETDYN_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SETDYN_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	SetPort(GetDialogWindowPort(dlog));

	oldResFile = CurResFile();
	UseResFile(appRFRefNum);									/* popup code uses Get1Resource */

	dynamicPop.menu = NULL;										/* NULL makes any goto broken safe */
	dynamicPop.itemChars = NULL;
	popKeysDynamic = NULL;

	GetDialogItem(dlog, DYNAM_POP_DI, &type, &hndl, &box);
	if (!InitGPopUp(&dynamicPop, TOP_LEFT(box), DYNAMIC_MENU, 1)) goto broken;
	popKeysDynamic = InitDynamicPopupKey(&dynamicPop);
	if (popKeysDynamic==NULL) goto broken;
	curPop = &dynamicPop;
	
	choice = GetDynamicPopItem(curPop, popKeysDynamic, *dynamicType);
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
		*dynamicType = popKeysDynamic[curPop->currentChoice].dynamicType;
		
broken:
	DisposeGPopUp(&dynamicPop);
	if (popKeysDynamic) DisposePtr((Ptr)popKeysDynamic);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	
	UseResFile(oldResFile);
	SetPort(oldPort);
	return (ditem==OK);
}

