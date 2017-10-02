/***************************************************************************
*	FILE:	DialogUtils.c
*	PROJ:	Nightingale
*	DESC:	General dialog-handling utilities
/***************************************************************************

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
	OutlineOKButton				FlashButton				OKButFilter
	OKButDragFilter				DlgCmdKey				SwitchRadio
	TrackNumberArrow			ClickUp					ClickDown
	HandleKeyDown				HandleMouseDown
	UseNumberFilter				NumberFilter,etc.
	InitDurStrings				GetHiddenDItemBox		SetDlgFont
*/

#include "Nightingale_Prefix.pch"
#include <ctype.h>
#include "Nightingale.appl.h"


/* -------------------------------------------------------------- OutlineOKButton -- */
/* Outline the OK button to indicate it's the default. */

#ifdef TARGET_API_MAC_CARBON
void OutlineOKButton(DialogPtr dlog, Boolean)
{
	SetDialogDefaultItem(dlog,OK);
}
#else
void OutlineOKButton(DialogPtr theDialog, Boolean wantBlack)
{
	FrameDefault(theDialog, OK, wantBlack);
}
#endif


/* ------------------------------------------------------------------ FlashButton -- */
/* If the given item is a button, flash it to indicate it's been pressed. */

void FlashButton(DialogPtr theDialog, short item)
{
	short type; Handle aHdl; Rect aRect;
 
	GetDialogItem(theDialog, item, &type, &aHdl, &aRect);
	if (type == (btnCtrl+ctrlItem)) {
		HiliteControl((ControlHandle)aHdl, 1);
		SleepTicks(8L);
		HiliteControl((ControlHandle)aHdl, 0);
	}
}
 
 
/* ------------------------------------------------------------------ OKButFilter -- */
/* This filter outlines the OK Button and performs standard key and command-
key filtering. */

pascal Boolean OKButFilter(DialogPtr theDialog, EventRecord *theEvent, short *item)
{
	switch (theEvent->what) {
		case updateEvt:
			BeginUpdate(GetDialogWindow(theDialog));
			UpdateDialogVisRgn(theDialog);
			FrameDefault(theDialog,OK,True);
			EndUpdate(GetDialogWindow(theDialog));
			*item = 0;
			return True;
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, item, False)) return True;
			break;
	}
	return False;
}


/* -------------------------------------------------------------- OKButDragFilter -- */
/* This filter outlines the OK Button, performs standard key and command-key
filtering, and allows dragging the dialog, i.e., does the moving part of "movable
modal" dialogs. */

pascal Boolean OKButDragFilter(DialogPtr theDialog, EventRecord *theEvent, short *item)
{
	short part; WindowPtr wind; Rect bounds;

	switch (theEvent->what) {
		case updateEvt:
			wind = (WindowPtr)theEvent->message;
			if (wind==GetDialogWindow(theDialog)) {
				BeginUpdate(GetDialogWindow(theDialog));
				UpdateDialogVisRgn(theDialog);
				FrameDefault(theDialog,OK,True);
				EndUpdate(GetDialogWindow(theDialog));
			}
			else if (IsDocumentKind(wind) || IsPaletteKind(wind)) {
				DoUpdate(wind);
				ArrowCursor();
			}
			*item = 0;
			return True;
		case mouseDown:
			part = FindWindow(theEvent->where, &wind);
			if (part==inDrag && wind==GetDialogWindow(theDialog)) {
				bounds = GetQDScreenBitsBounds();
				DragWindow(wind, theEvent->where, &bounds);
				*item = 0;
				return True;
			}
		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, item, False)) return True;
			break;
	}
	return False;
}


/* -------------------------------------------------------------------- DlgCmdKey -- */
/*	Adapted from DlgCmdKeyFilter by Douglas Wyatt. Intended to be called by dialog
filter functions for keyDown and autoKey events, this recognizes Return and Enter
as synonyms for clicking OK; command-. , X, C, V as Cancel, Cut, Copy, and Paste;
and command keys at the end of any control title as clicks on that control. If it
finds something it recognizes, it returns True with *item set to the number of the
item that the dialog function should consider to have been clicked on; for Cut,
Copy, and Paste it returns with *item=0 (caveat!).

N.B. Command keys at the end of control title strings take precedence over the
standard meanings of '.', 'X', 'C', and 'V'. */

Boolean DlgCmdKey(register DialogPtr dlog, EventRecord *evt,
				short *item,
				Boolean hotKeys	/* True=don't require cmd key to recognize keystrokes as ctl clicks */
				)
{
	register short i, key;
	short nitems, itemtype;
	ControlHandle hc;
	Rect box;
	short contrlHilite;
	Str255 sT;
	
	if (evt->what != keyDown && evt->what != autoKey)			/* in case caller passed wrong event type */
		return False;
	
	key = evt->message & charCodeMask;
	
	if (key == CH_CR || key == CH_ENTER) {
		FlashButton(dlog, OK);
		*item = OK;
		return True;
	}

	if (islower(key)) key = toupper(key);

	if (hotKeys || (evt->modifiers & cmdKey)) {
		nitems = CountDITL(dlog);	
		for (i=1; i<=nitems; ++i) {
			GetDialogItem(dlog, i, &itemtype, (Handle *)&hc, &box);
			if (itemtype & ctrlItem) {
				GetControlTitle(hc,sT);
				if (sT[sT[0] - 1] == CMD_KEY && sT[sT[0]] == key) {
					contrlHilite = GetControlHilite(hc);
					if (contrlHilite==CTL_ACTIVE) {			/* If the item is active */
						FlashButton(dlog, i);
						*item = i;
						return True;
					}
				}
			}
		}
	}

	if (evt->modifiers & cmdKey) {
		switch(key) {
			case 'X':
				if (TextSelected(dlog))
					{ ClearCurrentScrap(); DialogCut(dlog); TEToScrap(); }
				*item = 0;
				return True;
			case 'C':
				if (TextSelected(dlog))
					{ ClearCurrentScrap(); DialogCopy(dlog); TEToScrap(); }
				*item = 0;
				return True;
			case 'V':
				if (CanPaste(1,'TEXT'))
					{ TEFromScrap(); DialogPaste(dlog); }
				*item = 0;
				return True;
			case '.':
				FlashButton(dlog, Cancel);
				*item = Cancel;
				return True;
		}
	}
	
	return False;
}


/* ------------------------------------------------------------------ SwitchRadio -- */
/* Change the state of a set of radio buttons, and set the variables accordingly. */

void SwitchRadio(DialogPtr dialogp, short *curButton, short newButton)
{
	PutDlgChkRadio(dialogp,*curButton,False);
	PutDlgChkRadio(dialogp,newButton,True);
	*curButton = newButton;
}


#define MOUSETHRESHTIME 24			/* ticks before arrows auto-repeat */
#define MOUSEREPEATTIME 3			/* ticks between auto-repeats */

/* ------------------------------------------------------------- TrackNumberArrow -- */
/*	Track arrow (elevator button) control, calling <actionProc> after MOUSETHRESHTIME
ticks, and repeating every MOUSEREPEATTIME ticks thereafter, as long as the mouse is
still down inside of <arrowRect>. Modified from DBW's TrackArrow. */

void TrackNumberArrow(Rect *arrowRect, TrackNumberFunc actionProc, short limit,
								DialogPtr dialogp)
{
	long	t;
	Point	pt;

	(*actionProc)(limit, dialogp);								/* do it once */
	t = TickCount();													/* delay until auto-repeat */
	while (StillDown() && TickCount() < t+MOUSETHRESHTIME)
		;
	GetMouse(&pt);
	if (StillDown() && PtInRect(pt, arrowRect))
		(*actionProc)(limit, dialogp);							/* do it again */
	while (StillDown()) {
		t = TickCount();												/* auto-repeat rate */
		while (StillDown() && TickCount() < t+MOUSEREPEATTIME)
			;
		GetMouse(&pt);
		if (StillDown() && PtInRect(pt, arrowRect))
			(*actionProc)(limit, dialogp);						/* keep doing it */
	}
}


/* ----------------------------------------------------- NumberFilter and friends -- */

/* Globals for modal dialogs with elevator buttons */

/* Before using NumberFilter, caller must set these externals. ??It would be
MUCH better to make these explicit, e.g., as parameters to UseNumberFilter. */
short minVal, maxVal;					/* Number range */

static Rect upRect, downRect;
static short locDurItem;

static void ClickUp(short, DialogPtr);
static void ClickDown(short, DialogPtr);

static void ClickUp(short maxVal, DialogPtr theDialog)
{
	short		pcNum;
	
	GetDlgWord(theDialog, locDurItem, &pcNum);
	if (pcNum<maxVal) pcNum++;
	PutDlgWord(theDialog, locDurItem, pcNum, True);
}

static void ClickDown(short minVal, DialogPtr theDialog)
{
	short		pcNum;
	
	GetDlgWord(theDialog, locDurItem, &pcNum);
	if (pcNum>minVal) pcNum--;
	PutDlgWord(theDialog, locDurItem, pcNum, True);
}


Boolean HandleKeyDown(EventRecord *theEvent, short minVal, short maxVal,
								DialogPtr theDialog)
{
	char	theChar;

	theChar =theEvent->message & charCodeMask;
	if (theChar==UPARROWKEY) {
		ClickUp(maxVal, theDialog);
		return True;
	}
	else if (theChar==DOWNARROWKEY) {
		ClickDown(minVal, theDialog);
		return True;
	}
	else
		return False;
}


Boolean HandleMouseDown(EventRecord *theEvent, short minVal, short maxVal,
								DialogPtr theDialog)
{
	Point	where;

	where = theEvent->where;
	GlobalToLocal(&where);
	if (PtInRect(where, &upRect)) {
		SelectDialogItemText(theDialog, locDurItem, 0, ENDTEXT);						/* Select & unhilite number */
		TrackNumberArrow(&upRect, &ClickUp, maxVal, theDialog);
		return True;
	}
	else if (PtInRect(where, &downRect)) {
		SelectDialogItemText(theDialog, locDurItem, 0, ENDTEXT);						/* Select & unhilite number */
		TrackNumberArrow(&downRect, &ClickDown, minVal, theDialog);
		return True;
	}
	else
		return False;
}

/*
 *	Before using NumberFilter in a call to ModalDialog, this routine should be
 *	called to declare the item numbers of the duration label item and the up and
 * down buttons.
 */

void UseNumberFilter(DialogPtr dialogp, short durItem, short upItem, short downItem)
	{
		short itype; Handle tHdl;
		
		locDurItem = durItem;
		GetDialogItem(dialogp, upItem, &itype, &tHdl, &upRect);	
		GetDialogItem(dialogp, downItem, &itype, &tHdl, &downRect);
	}

pascal Boolean NumberFilter(register DialogPtr theDialog, EventRecord *theEvent,
										short *itemHit)
{
	switch (theEvent->what) {
		case updateEvt:
			WindowPtr w = GetDialogWindow(theDialog);
			if (w == (WindowPtr)theEvent->message) {
				BeginUpdate(GetDialogWindow(theDialog));
				TextFace(0); TextFont(systemFont);
				DrawDialog(theDialog);
				OutlineOKButton(theDialog,True);
				EndUpdate(GetDialogWindow(theDialog));
				}
			 else
				DoUpdate((WindowPtr)theEvent->message);
			*itemHit = 0;
			return True;
			break;
		case mouseDown:
			/*
			 * Mouse down in elevator button. Handle it, select and hilite the number,
			 * and make it look like a number was typed, just in case the caller cares.
			 */
			if (HandleMouseDown(theEvent, minVal, maxVal, theDialog)) {
				SelectDialogItemText(theDialog, locDurItem, 0, ENDTEXT);
				*itemHit = locDurItem;
				return True;
			}
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(theDialog, theEvent, itemHit, False))
				return True;
			else {
				/*
				 * Arrow key was typed. Handle it, then select and hilite the number.
				 */
				if (HandleKeyDown(theEvent, minVal, maxVal, theDialog)) {
					SelectDialogItemText(theDialog, locDurItem, 0, ENDTEXT);
					return True;
				}
			}
			break;
		default:
			;
	}
	return False;
}


/* -------------------------------------------------------------- InitDurStrings -- */

char durStrs[MAX_L_DUR+2][16];	/* Plural forms of dur. units: "DUMMY", "breves", wholes"", etc. */

void InitDurStrings(void)
{
	static Boolean firstCall=True;
	short n;
	
	if (firstCall) {
		for (n = 0; n<MAX_L_DUR+2; n++)
			GetIndCString(&durStrs[n][0], DURATION_STRS, n+1);
		firstCall = False;
	}
}


/* ----------------------------------------------------------------- XableControl -- */ 
/* Enable or disable a control item in the given dialog.  Assumes that <item>
refers to a control!  JGG, 2/24/01 */

void XableControl(DialogPtr dlog, short item, Boolean enable)
{
	short type, active; Handle hndl; Rect box;

	GetDialogItem(dlog, item, &type, &hndl, &box);
	active = enable? CTL_ACTIVE : CTL_INACTIVE;
	HiliteControl((ControlHandle)hndl, active);
}


/* ------------------------------------------------------------ MoveDialogControl -- */
/* Move the control dialog item to the given <left>, <top> position, without
changing its width or height.  JGG, 2/24/01 */

void MoveDialogControl(DialogPtr dlog, short item, short left, short top)
{
	short type, width, height; Handle hndl; Rect box;

	GetDialogItem(dlog, item, &type, &hndl, &box);
	width = box.right - box.left;
	height = box.bottom - box.top;
	box.left = left;
	box.right = left + width;
	box.top = top;
	box.bottom = top + height;
	SetDialogItem(dlog, item, type, hndl, &box);
	MoveControl((ControlHandle)hndl, left, top);				/* Have to do this, too. */
}


/* ------------------------------------------------------------ GetHiddenDItemBox -- */ 
/* Given a dialog and item no., get the on-screen position of the bounding box for
the item. Intended to be used for items hidden with HideDialogItem, though also works
correctly for visible items. */

void GetHiddenDItemBox(DialogPtr dlog, short item, Rect *visRect)
{
	short aShort; Handle anHdl; Rect box;
	
	GetDialogItem(dlog, item, &aShort, &anHdl, &box);
	*visRect = box;
	if (visRect->left>8192) {
		visRect->left -= 16384;				/* Adjust for HideDialogItem: see Inside Mac, IV-59 */
		visRect->right -= 16384;
	}
}


/* ------------------------------------------------------------------ SetDlgFont -- */ 
/* Set the font and size to use for static text and edit text items in the given
dialog. Does not affect buttons. */

void SetDlgFont(DialogPtr dlog,
				short font,
				short size,
				short style)						/* If <0, don't change */
{
	TEHandle		textHdl;

	/* Set the DialogRecord's TextEdit font. */
	
	textHdl = GetDialogTextEditHandle(dlog);
	(**textHdl).txFont = font;
	(**textHdl).txSize = size;
	if (style>=0) (**textHdl).txFace = style;
	
	/* To be safe, set the port's font as well. Seems to be necessary for AV Macs. */
	
	TextFont(font);
	TextSize(size);
	if (style>=0) TextFace(style);
}
