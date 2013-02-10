/* DurationPopUp.c for Nightingale - for use with accompanying graphic MDEF */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1990-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/* NB: If the app uses AppendResMenu to build a font menu, take this advice from
Inside Mac:
	So that you can have resources of the given type that won't appear in
	the menu, any resource names that begin with a period (.) or a percent
	sign (%) aren't apppended by AppendResMenu.
Change the name of the 'NFNT' and 'FOND' resource, as well as the fontName field of
all 'chgd' resources, accordingly. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "GraphicMDEF.h"

extern Boolean TranslatePalChar(short *, short, Boolean);

static enum {			/* ASCII decimal for symbols in durPop font */
	POP_BREVE_DOT = 33,
	POP_BREVE,
	POP_WHOLE_DOT,
	POP_WHOLE,
	POP_HALF_DOT,
	POP_HALF,
	POP_QTR_DOT,
	POP_QTR,
	POP_8TH_DOT,
	POP_8TH,
	POP_16TH_DOT,
	POP_16TH,
	POP_32ND_DOT,
	POP_32ND,
	POP_64TH_DOT,
	POP_64TH,
	POP_128TH_DOT,
	POP_128TH,
	POP_UNKNOWNDUR,
	POP_BREVE_2DOT,
	POP_WHOLE_2DOT,
	POP_HALF_2DOT,
	POP_QTR_2DOT,
	POP_8TH_2DOT,
	POP_16TH_2DOT,
	POP_32ND_2DOT,
	POP_64TH_2DOT
} E_DurPopupItems;

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


/* --------------------- Functions for managing graphic popups --------------------- */


Boolean InitGPopUp(
	register PGRAPHIC_POPUP	p,					/* pointer to GRAPHIC_POPUP allocated by caller */
	Point							origin,			/* top, left corner in local coords */
	short							menuID,			/* ID for menu AND its 'chgd' resource */
	short							firstChoice		/* item number of initial choice (1-based) */
	)
{
	register Handle		resH;
	register PCHARGRID	cgP;
	register short			i, saveFontSize, saveFontNum, fontNameLen;
	unsigned char			*itemChars;
	GrafPtr					gp;
	FontInfo					finfo;
	
	p->menu = NULL;	p->itemChars = NULL;		/* in case init fails, but DisposeGPopUp(p) called anyway */
	
	/* Copy info from 'chgd' resource into popup struct */
	resH = Get1Resource('chgd', menuID);
	if (resH==NULL)	return FALSE;
	HLock(resH);									/* NewPtr & GetFNum below can move memory */
	cgP = (PCHARGRID) *resH;
	p->numItems = cgP->numChars;
	p->numColumns = cgP->numColumns;
	p->fontSize = cgP->fontSize;
	p->itemChars = (char *) NewPtr((Size)p->numItems);
	if (p->itemChars==NULL) goto broken;

	fontNameLen = cgP->fontName[0] + 1;
	if (odd(fontNameLen)) fontNameLen++;
	itemChars = cgP->fontName + fontNameLen;

	for (i=0; i<p->numItems; i++)
		p->itemChars[i] = itemChars[i];
	
	/* Get popup font number and characteristics */
#ifdef TARGET_API_MAC_CARBON
	FMFontFamily fff = FMGetFontFamilyFromName(cgP->fontName);
	p->fontNum = (short)fff;
#else
	GetFNum(cgP->fontName, &p->fontNum);
#endif

	HUnlock(resH);
	ReleaseResource(resH);

#ifdef TARGET_API_MAC_CARBON
	if (p->fontNum==kInvalidFontFamily) goto broken;
	
#else
	if (p->fontNum==0) goto broken;
#endif

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

	/* Get menu */
	p->currentChoice = firstChoice;
	p->menuID = menuID;
#if TARGET_API_MAC_CARBON
	MenuRef			customMenu;
	MenuDefSpec		defSpec;
	
	defSpec.defType = kMenuDefProcPtr;
	defSpec.u.defProc = NewMenuDefUPP( MyMDefProc );

	// Create a custom menu
	CreateCustomMenu( &defSpec, menuID, 0, &customMenu );
	p->menu = customMenu;
#else
	p->menu = GetMenu(menuID);
#endif
	if (p->menu) 
		;
//		DetachResource((Handle)p->menu);		/* Make sure each popup has its own local copy of menu */
	else
		goto broken;

	return TRUE;
broken:
	ReleaseResource(resH);
	return FALSE;
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

	for (loop=0; loop<ARROW_LINES; ++loop) {				/* Loop through the six lines of the triangle */
		MoveTo(h+loop-ARROW_LINES, v+loop);					/* Move to beginning of each line */
		Line((ARROW_LINES*2)-2 - (loop*2), 0);				/* Draw line of appropriate length */
	}
}

#endif


void HiliteGPopUp(
	PGRAPHIC_POPUP	p,
	short				activ 			/* frame popup box with dotted line, else erase it */
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


/* Invoke a popup menu; return TRUE if new choice made, which will be in
p->currentChoice. */

Boolean DoGPopUp(PGRAPHIC_POPUP p)
{
	register long		choice;
	register Boolean	ans = FALSE;
	Point					pt, mPt;
	GrafPtr				savePort;
	
#if DRAW_POPUP_ARROW
	/* Get local pos of mouse to decide if we're over popup arrow.
	???This should come from the mousedown event that caused DoGPopUp,
	   because mouse could move between that event and now!!
	*/
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
			ans = TRUE;
		}
	}
	
	SetPort(savePort);
	DrawGPopUp(p);
	return ans;
}


void DisposeGPopUp(PGRAPHIC_POPUP p)
{
#ifdef TARGET_API_MAC_CARBON
	if (p->menu) DisposeMenu(p->menu);
#else
	if (p->menu) DisposeHandle((Handle)p->menu);
#endif
	p->menu = NULL;
	if (p->itemChars) DisposePtr(p->itemChars);
}


/* ---------------------------- DurPopupKey & Friends ----------------------------- */

/* Allocates 1-based array mapping menu item numbers to POPKEY structs.
 * Caller should dispose this pointer when disposing its associated popop.
 * Assumes graphic popup already inited.
 */

POPKEY *InitDurPopupKey(PGRAPHIC_POPUP	gp)
{
	register short		i;
	register char		*q;
	register POPKEY	*pkeys, *p;
	
	if (gp==NULL || gp->numItems==0) return NULL;
	
	pkeys = (POPKEY *) NewPtr((Size)(sizeof(POPKEY)*(gp->numItems+1)));
	if (pkeys==NULL) goto broken;
	
	pkeys->durCode = pkeys->numDots = -99;			/* init garbage entry */
	p = &pkeys[1];											/* point to 1st real entry */
	q = gp->itemChars;
	for (i=0; i<gp->numItems; i++, q++, p++) {
		p->numDots = 0;									/* might override below */
		switch (*q) {
			case POP_UNKNOWNDUR:
				p->durCode = UNKNOWN_L_DUR;
				break;
			case POP_BREVE:
			case POP_BREVE_DOT:
			case POP_BREVE_2DOT:
				p->durCode = BREVE_L_DUR;
				if (*q==POP_BREVE_DOT) p->numDots = 1;
				if (*q==POP_BREVE_2DOT) p->numDots = 2;
				break;
			case POP_WHOLE:
			case POP_WHOLE_DOT:
			case POP_WHOLE_2DOT:
				p->durCode = WHOLE_L_DUR;
				if (*q==POP_WHOLE_DOT) p->numDots = 1;
				if (*q==POP_WHOLE_2DOT) p->numDots = 2;
				break;
			case POP_HALF:
			case POP_HALF_DOT:
			case POP_HALF_2DOT:
				p->durCode = HALF_L_DUR;
				if (*q==POP_HALF_DOT) p->numDots = 1;
				if (*q==POP_HALF_2DOT) p->numDots = 2;
				break;
			case POP_QTR:
			case POP_QTR_DOT:
			case POP_QTR_2DOT:
				p->durCode = QTR_L_DUR;
				if (*q==POP_QTR_DOT) p->numDots = 1;
				if (*q==POP_QTR_2DOT) p->numDots = 2;
				break;
			case POP_8TH:
			case POP_8TH_DOT:
			case POP_8TH_2DOT:
				p->durCode = EIGHTH_L_DUR;
				if (*q==POP_8TH_DOT) p->numDots = 1;
				if (*q==POP_8TH_2DOT) p->numDots = 2;
				break;
			case POP_16TH:
			case POP_16TH_DOT:
			case POP_16TH_2DOT:
				p->durCode = SIXTEENTH_L_DUR;
				if (*q==POP_16TH_DOT) p->numDots = 1;
				if (*q==POP_16TH_2DOT) p->numDots = 2;
				break;
			case POP_32ND:
			case POP_32ND_DOT:
			case POP_32ND_2DOT:
				p->durCode = THIRTY2ND_L_DUR;
				if (*q==POP_32ND_DOT) p->numDots = 1;
				if (*q==POP_32ND_2DOT) p->numDots = 2;
				break;
			case POP_64TH:
			case POP_64TH_DOT:
			case POP_64TH_2DOT:									/* disallowed by Ngale */
				p->durCode = SIXTY4TH_L_DUR;
				if (*q==POP_64TH_DOT) p->numDots = 1;
				if (*q==POP_64TH_2DOT) p->numDots = 2;
				break;
			case POP_128TH:
			case POP_128TH_DOT:									/* disallowed by Ngale */
				p->durCode = ONE28TH_L_DUR;
				if (*q==POP_128TH_DOT) p->numDots = 1;
				break;
			default:
				p->durCode = NO_L_DUR;				/* empty item (represented by NULL in 'chgd', not simply a leftover) */
				break;
		}
	}
	
broken:
	return pkeys;
}


/* Given a duration popup and associated POPKEY array, return the 
 * index into that array of the given durCode and numDots combination.
 * The index will be a menu item number.
 */

short GetDurPopItem(PGRAPHIC_POPUP p, POPKEY *pk, short durCode, short numDots)
{
	register short		i;
	register POPKEY	*q;
	
	for (i=1, q=&pk[1]; i<=p->numItems; i++, q++)
		if (q->durCode==durCode && q->numDots==numDots)
			return i;
	return NOMATCH;
}


/* --------------------------------------------------------------- DurPopChar2Dur -- */
/*	local version of Char2Dur that matches durs only for notes (not for rests or grace notes) */

/* from symtable: durKeys[durcode] = symcode */
static unsigned char durKeys[MAX_L_DUR+1] = {
	0,			/* dummy */
	0xDD,		/* breve */
	'w',		/* whole */
	'h',		/* half */
	'q',		/* quarter */
	'e',		/* 8th */
	'x',		/* 16th */
	'r',		/* 32nd */
	't',		/* 64th */
	'y'		/* 128th */
};

static short DurPopChar2Dur(char);
static short DurPopChar2Dur(char token)
{
	register short j;
	
	for (j=1; j<=MAX_L_DUR; j++)
		if (token==(char)durKeys[j])
			return j;
	return NOMATCH;									/* Illegal token */
}


/* Given a character code <theChar>, a popup <p> and its associated POPKEY array <pk>,
determine if the character should select a new duration from the popup. If it should,
call SetGPopUpChoice and return TRUE; if not, return FALSE. In addition to the familiar
duration key equivalents (symcodes in the symtable), DurPopupKey recognizes '.' as
a way to cycle through the number of aug dots allowed by the popup, and '?' as an
equivalent for the unknown duration item (if it's in the popup). (The latter is
undesirable, since the symtable has no unknown duration symbol and thus no standard
symcode -- '?' is the symcode for bass clef.) DurPopupKey calls TranslatePalChar
before doing anything, so it recognizes remapped duration key equivalents (unless one
of them is '?'!). NB: If the symcodes in symtable are ever changed, make corresponding
changes to the static durKeys array above. */

Boolean DurPopupKey(PGRAPHIC_POPUP p, POPKEY *pk, unsigned char theChar)
{
	register short	i, curItem, newItem=0, newDurCode, curDurCode, numItems, curNumDots;
	Boolean			isDotChar=FALSE;
	
	/* remap theChar according to the 'PLMP' resource */
	if (theChar!='?') {									/* because hard-wired below! */
		short intChar = (short)theChar;
		TranslatePalChar(&intChar, 0, FALSE);
		theChar = (unsigned char) intChar;
	}
	
	numItems = p->numItems;
	curItem = p->currentChoice;
	curDurCode = pk[curItem].durCode;
	curNumDots = pk[curItem].numDots;
	
	newDurCode = DurPopChar2Dur(theChar);
	if (newDurCode==NOMATCH) {							/* maybe it's another char we recognize */
		if (theChar=='.')									/* same as aug dot's symcode in symtable */
			isDotChar = TRUE;
		else if (theChar=='?')							/* This certainly shouldn't be hard-wired, */
			newDurCode = UNKNOWN_L_DUR;				/* but there's no symcode for it! */
		else
			return FALSE;									/* no action for this key */
	}
	
	if (isDotChar) {
		for (i=curItem+1; ; i++) {						/* NB: do not use newDurCode in this block */
			if (i>numItems) i = 1;
			if (pk[i].durCode==curDurCode) {			/* loop 'till we match durCodes */
				newItem = i;
				goto setChoice;
			}
		}
	}
	else {
		for (i=1; i<=numItems; i++)					/* don't bother maintaining number of dots */
			if (pk[i].durCode==newDurCode && pk[i].numDots==0) {
				newItem = i;
				goto setChoice;
			}
	}
	
setChoice:	
	if (newItem && newItem!=curItem)
		SetGPopUpChoice(p, newItem);
	return TRUE;											/* the keystroke was directed at popup */
}
