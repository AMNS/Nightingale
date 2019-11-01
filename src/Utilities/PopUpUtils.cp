/******************************************************************************************
*	FILE:	PopUpUtils.c
*	PROJ:	Nightingale
*	DESC:	Utility routines for implementing pop-up menus.
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
/*
 		DrawPUpTriangle			DrawPopUp				TruncPopUpString
		InitPopUp				DoUserPopUp				ChangePopUpChoice
		DisposePopUp			HilitePopUp				ResizePopUp
		ShowPopUp
		InitGPopUp				SetGPopUpChoice			DrawGPopUp
		HiliteGPopUp			DoGPopUp				DisposeGPopUp
*/


#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "GraphicMDEF.h"

#define DRAW_POPUP_ARROW 1

#if DRAW_POPUP_ARROW
#define SMALL_ARROW 1
#if SMALL_ARROW
#define EXTRAWID_FOR_POPARROW 18
#define POPUPSYM_OFFSET 8
#else
#define EXTRAWID_FOR_POPARROW 20
#define POPUPSYM_OFFSET 10
#endif
static void DrawPopupSymbol(short, short);
#endif


/* ============================================= Functions for handling graphic popups == */

Boolean InitGPopUp(
	register PGRAPHIC_POPUP	p,				/* pointer to GRAPHIC_POPUP allocated by caller */
	Point					origin,			/* top, left corner in local coords */
	short					menuID,			/* ID for menu AND its 'chgd' resource */
	short					firstChoice		/* item number of initial choice (1-based) */
	)
{
	register Handle		resH;
	register PCHARGRID	chgdP;
	register short		i, saveFontSize, saveFontNum, fontNameLen;
	unsigned char		*itemChars;
	GrafPtr				gp;
	FontInfo			finfo;
	
	p->menu = NULL;	p->itemChars = NULL;		/* in case init fails but DisposeGPopUp(p) called anyway */
	
	/* Copy info from 'chgd' resource into popup struct */
	resH = Get1Resource('chgd', menuID);
	if (resH==NULL)	return False;
	HLock(resH);									/* NewPtr & GetFNum below can move memory */
	chgdP = (PCHARGRID)*resH;
	p->numItems = chgdP->numChars;		FIX_END(p->numItems);
	p->numColumns = chgdP->numColumns;
	p->fontSize = chgdP->fontSize;		FIX_END(p->fontSize);
	p->itemChars = (char *)NewPtr((Size)p->numItems);
//LogPrintf(LOG_DEBUG, "InitGPopUp: numItems=%d numColumns=%d fontSize=%d\n", p->numItems,
//p->numColumns, p->fontSize);
    if (p->itemChars==NULL) {
		LogPrintf(LOG_ERR, "Can't allocate memory for %d items for the graphic pop-up.  (InitGPopUp)\n",
					p->numItems);
        ReleaseResource(resH);
        return False;
    }

	fontNameLen = chgdP->fontName[0] + 1;
	if (odd(fontNameLen)) fontNameLen++;
	itemChars = chgdP->fontName + fontNameLen;

	for (i=0; i<p->numItems; i++)
		p->itemChars[i] = itemChars[i];
//LogPrintf(LOG_DEBUG, "InitGPopUp: itemChars[]=%c%c%c\n", p->itemChars[0], ??????????
	
	/* Get popup font number and characteristics */
	FMFontFamily fff = FMGetFontFamilyFromName(chgdP->fontName);
	p->fontNum = (short)fff;

	HUnlock(resH);
	ReleaseResource(resH);

	if (p->fontNum==kInvalidFontFamily) goto broken;
	
	GetPort(&gp);
	saveFontNum = GetPortTextFont(gp);
	saveFontSize = GetPortTextSize(gp);
	TextFont(p->fontNum);
	TextSize(p->fontSize);
	GetFontInfo(&finfo);
	TextFont(saveFontNum);
	TextSize(saveFontSize);

	/* Calculate number of rows */
	if (p->numItems < p->numColumns) p->numItems = p->numColumns;
	p->numRows = p->numItems / p->numColumns;
	if (p->numItems % p->numColumns) p->numRows++;

	/* Compute rects */
	p->box.top = origin.v;
	p->box.left = origin.h;
	p->box.bottom = origin.v + finfo.ascent;
	p->box.right = origin.h + finfo.widMax;
#if DRAW_POPUP_ARROW
	p->box.right += EXTRAWID_FOR_POPARROW;
#endif
	p->bounds = p->box;
	InsetRect(&p->bounds, -1, -1);
	p->shadow = p->bounds;
	p->shadow.right++; p->shadow.bottom++;

	/* Create a custom menu */
	p->currentChoice = firstChoice;
	p->menuID = menuID;
	MenuRef customMenu;
	MenuDefSpec defSpec;
	
	defSpec.defType = kMenuDefProcPtr;
	defSpec.u.defProc = NewMenuDefUPP(NMDefProc);

	CreateCustomMenu(&defSpec, menuID, 0, &customMenu);
	p->menu = customMenu;
	if (p->menu) 
		;
//		DetachResource((Handle)p->menu);		/* Make sure each popup has its own local copy of menu */
	else
		goto broken;

	return True;
	
broken:
	SysBeep(10);
	LogPrintf(LOG_ERR, "Can't create the graphic pop-up. The font may not be available.  (InitGPopUp)\n");
	ReleaseResource(resH);
	return False;
}


/* Set popup menu to show given choice, or nothing if choice is 0. */

void SetGPopUpChoice(PGRAPHIC_POPUP	p, short choice)
{
	if (choice>0 && choice<=p->numItems)
		p->currentChoice = choice;
	else
		p->currentChoice = 0;
	EraseRect(&p->box);
	DrawGPopUp(p);
}


void DrawGPopUp(PGRAPHIC_POPUP p)
{
	register short font, size;
	
	FrameRect(&p->bounds);
	MoveTo(p->bounds.right, p->shadow.top+2);			/* drop shadow */
	LineTo(p->bounds.right, p->bounds.bottom);
	LineTo(p->shadow.left+2, p->bounds.bottom);
		
	font = GetPortTxFont(); size = GetPortTxSize();
	TextFont(p->fontNum);	TextSize(p->fontSize);

	MoveTo(p->box.left, p->box.bottom);					/* draw the symbol */
	if (p->currentChoice>0 && p->currentChoice<=p->numItems)
		DrawChar(p->itemChars[p->currentChoice-1]);

#if DRAW_POPUP_ARROW
	DrawPopupSymbol(p->box.right-POPUPSYM_OFFSET, (p->box.bottom+p->box.top)/2-3);
#endif
	
	TextFont(font);	TextSize(size);
}


#if DRAW_POPUP_ARROW

/* from Popup CDEF 1.3.1 by Chris Faigle */
/* Draw the downward pointing triangle for new style popups */

#if SMALL_ARROW
#define ARROW_LINES 5
#else
#define ARROW_LINES 6
#endif

static void DrawPopupSymbol(short h, short v)
{
	register short loop;

	for (loop=0; loop<ARROW_LINES; ++loop) {		/* Loop through the six lines of the triangle */
		MoveTo(h+loop-ARROW_LINES, v+loop);			/* Move to beginning of each line */
		Line((ARROW_LINES*2)-2 - (loop*2), 0);		/* Draw line of appropriate length */
	}
}

#endif


void HiliteGPopUp(
	PGRAPHIC_POPUP	p,
	short			activ 			/* frame popup box with dotted line, else erase it */
	)
{
	if (activ) {
		PenState oldPenState;
		GetPenState(&oldPenState);
		PenSize(1, 1);
		PenPat(NGetQDGlobalsGray());
		FrameRect(&p->box);			
		SetPenState(&oldPenState);
	}
	else {
		EraseRect(&p->box);
		DrawGPopUp(p);
	}		
}


/* Invoke a popup menu; return True if new choice made, which will be in p->currentChoice. */

Boolean DoGPopUp(PGRAPHIC_POPUP p)
{
	register long		choice;
	register Boolean	ans = False;
	Point				pt, mPt;
	GrafPtr				savePort;
	
#if DRAW_POPUP_ARROW
	/* Get local pos of mouse to decide if we're over popup arrow. FIXME: This
		should come from the mousedown event that caused DoGPopUp, because mouse
		could move between that event and now!! */
	GetMouse(&mPt);
#endif

	GetPort(&savePort);

	InsertMenu(p->menu, -1);
	CalcMenuSize(p->menu);
	pt = *(Point *)(&p->box);				/* top, left */
	EraseRect(&p->shadow);
	LocalToGlobal(&pt);
#if DRAW_POPUP_ARROW
	if (mPt.h >= (p->box.right-EXTRAWID_FOR_POPARROW))
		pt.h += p->box.right-p->box.left-EXTRAWID_FOR_POPARROW;
#endif
	if (p->currentChoice==0) p->currentChoice = 1;		/* otherwise MDEF freaks out */
	choice = PopUpMenuSelect(p->menu, pt.v, pt.h, p->currentChoice);
	DeleteMenu(p->menuID);		/* ???This causes next invocation of popup not to use its 'mctb'. */
	if (choice) {
		choice = LoWord(choice);
		if (choice != p->currentChoice) {
			SetGPopUpChoice(p, (short)choice);
			ans = True;
		}
	}
	
	SetPort(savePort);
	DrawGPopUp(p);
	return ans;
}


void DisposeGPopUp(PGRAPHIC_POPUP p)
{
	if (p->menu) DisposeMenu(p->menu);
	p->menu = NULL;
	if (p->itemChars) DisposePtr(p->itemChars);
}


/* ============================== Utilities for handling regular (non-graphic) pop-ups == */
/* The following utilities for handling pop-ups were written by Resorcerer and are
included by its kind permission and permission of its agent, Mr. Douglas M. McKenna. */

#define L_MARGIN 13
#define R_MARGIN 5
#define B_MARGIN 5

#define TRIANGLE_MARGIN 16

static void DrawPUpTriangle(Rect *bounds);
static void DrawPUpTriangle(Rect *bounds)
	{
		Rect box;
		
		box = *bounds;
		box.left = box.right - TRIANGLE_MARGIN;
		InsetRect(&box,1,1);
		EraseRect(&box);
		
		/* Draw triangle in right side of popup bounds, centered vertically */
		
		MoveTo(box.left,bounds->top+((bounds->bottom-bounds->top)-6)/2);
		Line(10,0); Move(-1,1);
		Line(-8,0); Move(1,1);
		Line(6,0); Move(-1,1);
		Line(-4,0); Move(1,1);
		Line(2,0); Move(-1,1);
		Line(0,0);
	}


/* Draw a given popup user item in its normal (un-popped-up) state */

void DrawPopUp(UserPopUp *p)
	{
		FrameRect(&p->bounds);
		/* And its drop shadow */
		MoveTo(p->bounds.right,p->shadow.top+2);
		LineTo(p->bounds.right,p->bounds.bottom);
		LineTo(p->shadow.left+2,p->bounds.bottom);
		/* And draw the current setting str in */
		MoveTo(p->bounds.left+L_MARGIN,p->bounds.bottom-B_MARGIN);
		EraseRect(&p->box);
		DrawString(p->str);
		DrawPUpTriangle(&p->bounds);
	}


/* TruncPopUpString() gets the string for the current setting and fits it into
the popup item's bounding box, if necessary truncating it and adding the
ellipsis character. */

void TruncPopUpString(UserPopUp *p)
	{
		short width,space,len;

		if (p->currentChoice)
			GetMenuItemText(p->menu,p->currentChoice,p->str);
		 else
			*p->str = 0;
		space = p->bounds.right - p->bounds.left - (L_MARGIN+R_MARGIN);

		/* We want the StringWidth ignoring trailing blanks */
		len = *(unsigned char *)p->str;
		while (len > 0) if (p->str[len]!=' ') break; else len--;
		*p->str = len;

		width = StringWidth(p->str);
		if (width > space) {
			len = *p->str;
			width -= CharWidth('‚Ä¶');
			while (len>0 && width>space)
				width -= CharWidth(p->str[len--]);
			p->str[++len] = '‚Ä¶';
			*p->str = len;
			}
	}


/* Initialise a UserPopUp data structure; return False if error. If firstChoice=0, no
item is initially chosen. */

short InitPopUp(
			DialogPtr dlog,
			UserPopUp *p,
			short item,				/* Dialog item to set p->box from */
			short pItem,			/* Dialog item to set p->prompt from, or 0=set it empty */
			short menuID,
			short firstChoice		/* Initial choice, or 0=none */
			)
	{
		short type;  Handle hndl;

		if (pItem)	GetDialogItem(dlog,pItem,&type,&hndl,&p->prompt);
		 else		SetRect(&p->prompt,0,0,0,0);
		
		GetDialogItem(dlog,item,&type,&hndl,&p->box);
		p->bounds = p->box; InsetRect(&p->bounds,-1,-1);
		p->shadow = p->bounds;
		p->shadow.right++; p->shadow.bottom++;
		p->menu = GetMenu(p->menuID = menuID);
		if (!p->menu) { SysBeep(1); return(False); }
		p->currentChoice = firstChoice;
		TruncPopUpString(p);

		return(True);
	}


/* Invoke a popup menu; return True if new choice was made */

short DoUserPopUp(UserPopUp *p)
	{
		long choice; Point pt; short ans = False;

		InvertRect(&p->prompt);
		InsertMenu(p->menu,-1);
		CalcMenuSize(p->menu);
		pt = *(Point *)(&p->box);
		LocalToGlobal(&pt);
		choice = PopUpMenuSelect(p->menu,pt.v,pt.h,p->currentChoice);
		InvertRect(&p->prompt);
		DeleteMenu(p->menuID);
		if (choice) {
			choice = LoWord(choice);
			if (choice != p->currentChoice) {
				ChangePopUpChoice(p,(short)choice);
				ans = True;
				}
			}
		return(ans);
	}


/* Change a given popup to a given choice, regardless of the current popup choice.
This means unchecking the old choice, if any, and checking the new one, resetting
the display string, and redisplaying. */

void ChangePopUpChoice(UserPopUp *p, short choice)
	{
		if (p->currentChoice)
			SetItemMark(p->menu,p->currentChoice,0);
		if (choice>=1 && choice<=CountMenuItems(p->menu))
			SetItemMark(p->menu,choice,(char)checkMark);
		p->currentChoice = choice;
		
		TruncPopUpString(p);
		EraseRect(&p->box);
		DrawPopUp(p);
	}


/* Do end-of-dialog cleanup. */

void DisposePopUp(UserPopUp *p)
	{
		if (p->menu) DisposeMenu(p->menu);
		p->menu = NULL;
	}


/* Hilite or remove hiliting from the un-popped-up popup to show if it's active. */

void HilitePopUp(UserPopUp *p, short activ)
	{
		EraseRect(&p->box);

		/* Redraw the menu string */
		MoveTo(p->bounds.left+L_MARGIN,p->bounds.bottom-B_MARGIN);
		DrawString(p->str);
		DrawPUpTriangle(&p->bounds);

		if (activ) {
			PenPat(NGetQDGlobalsGray()); PenSize(2,1);
			FrameRect(&p->box);			
			PenNormal();
		}
	}


/* ResizePopUp changes the widths of an existing popup's rects to accommodate its
longest menu item string, and returns 1. The top left point isn't changed. If no change
is necessary, ResizePopUp returns 0. */
 
short ResizePopUp(UserPopUp *p)
	{
		short thisWidth,maxWidth = 0,space,lastItemNum,menuItem;
		Str255 str;
		
		lastItemNum = CountMenuItems(p->menu);
		
		for (menuItem=1;menuItem<=lastItemNum;menuItem++) {
			GetMenuItemText(p->menu,menuItem,str);
			thisWidth = StringWidth(str);
			maxWidth = n_max(maxWidth,thisWidth);
		}

		maxWidth += TRIANGLE_MARGIN-4;		/* <maxWidth> is generous so we can cheat a bit */
		
		space = p->bounds.right - p->bounds.left - (L_MARGIN+R_MARGIN);
		if (space==maxWidth) return(0);
		
		p->bounds.right = p->bounds.left + maxWidth + (L_MARGIN+R_MARGIN);
		p->box = p->bounds; InsetRect(&p->box,1,1);
		p->shadow = p->bounds;
		p->shadow.right++; p->shadow.bottom++;
		return(1);
	}


/*	ShowPopUp does the job for pop-ups that HideDialogItem and ShowDialogItem do for normal
dialog items. */

void ShowPopUp(UserPopUp *p, short vis)
	{
		switch (vis)	{
			case True:
				if (p->box.left > 16000)	{
					p->box.left -= 16384;
					p->box.right -= 16384;}
				break;
			case False:
				if (p->box.left < 16000)	{
					p->box.left += 16384;
					p->box.right += 16384;}	
				break;
		}			
		p->bounds = p->box; InsetRect(&p->bounds,-1,-1);
		p->shadow = p->bounds;
		p->shadow.right++; p->shadow.bottom++;
	}


/* Let the user select popup items using arrow keys. Doesn't work if the menu has any
disabled items, unless they are the '--------' kind and are not the first or last items.
Probably by John Gibson, not Resorcerer. */

void HiliteArrowKey(DialogPtr /*dlog*/, short whichKey, UserPopUp *pPopup,
										Boolean *pHilitedItem)
{
	short lastItemNum, newChoice;
	UserPopUp *p;
					
	if (!*pHilitedItem) {
		*pHilitedItem = True;
		HilitePopUp(pPopup, True);
	}
	else {
		p = pPopup;
		newChoice = p->currentChoice;
		lastItemNum = CountMenuItems(p->menu);
		if (whichKey == UPARROWKEY) {				
			newChoice -= 1;
			if (newChoice == 0) return;					/* Top of menu reached; don't wrap around. */
			GetMenuItemText(p->menu, newChoice, p->str);
			if (p->str[1] == '-') newChoice -= 1;		/* Skip over the dash item */ 						
		}
		else {
			newChoice += 1;
			if (newChoice == lastItemNum + 1) return;	/* Bottom of menu reached; don't wrap around. */
			GetMenuItemText(p->menu, newChoice, p->str);
			if (p->str[1] == '-') newChoice += 1;		/* Skip over the dash item */ 						
		}
		ChangePopUpChoice (p, newChoice);
		HilitePopUp(p,True);
	}
}

