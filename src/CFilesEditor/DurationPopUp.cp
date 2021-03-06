/* DurationPopUp.c for Nightingale - for use with accompanying graphic MDEF */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* NB: If the app uses AppendResMenu to build a font menu, take this advice from
Inside Macintosh:
	So that you can have resources of the given type that won't appear in
	the menu, any resource names that begin with a period (.) or a percent
	sign (%) aren't apppended by AppendResMenu.
Change the name of the 'NFNT' and 'FOND' resource, as well as the fontName field of
all 'chgd' resources, accordingly. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

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


/* ------------------------------- DurPopupKey & Friends -------------------------------- */

/* Allocates 1-based array mapping menu item numbers to POPKEY structs. Caller should
dispose this pointer when disposing its associated popop. Assumes graphic popup already
inited. */

POPKEY *InitDurPopupKey(PGRAPHIC_POPUP	gp)
{
	register short	i;
	register char	*q;
	register POPKEY	*pkeys, *p;
	
	if (gp==NULL || gp->numItems==0) return NULL;
	
	pkeys = (POPKEY *) NewPtr((Size)(sizeof(POPKEY)*(gp->numItems+1)));
	if (pkeys==NULL) goto broken;
	
	pkeys->durCode = pkeys->numDots = -99;				/* init garbage entry */
	p = &pkeys[1];										/* point to 1st real entry */
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
			case POP_64TH_2DOT:								/* disallowed by Ngale */
				p->durCode = SIXTY4TH_L_DUR;
				if (*q==POP_64TH_DOT) p->numDots = 1;
				if (*q==POP_64TH_2DOT) p->numDots = 2;
				break;
			case POP_128TH:
			case POP_128TH_DOT:								/* disallowed by Ngale */
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


/* Given a duration popup and associated POPKEY array, return the index into that
array of the given durCode and numDots combination. The index will be a menu item
number. */

short GetDurPopItem(PGRAPHIC_POPUP p, POPKEY *pk, short durCode, short numDots)
{
	register short		i;
	register POPKEY	*q;
	
	for (i=1, q=&pk[1]; i<=p->numItems; i++, q++)
		if (q->durCode==durCode && q->numDots==numDots)
			return i;
	return NOMATCH;
}


/* -------------------------------------------------------------------- DurPopChar2Dur -- */
/*	local version of Char2Dur that matches durs only for notes (not for rests or grace notes) */

/* from symtable: durKeys[durcode] = symcode */
static unsigned char durKeys[MAX_L_DUR+1] = {
	0,		/* dummy */
	0xDD,	/* breve */
	'w',	/* whole */
	'h',	/* half */
	'q',	/* quarter */
	'e',	/* 8th */
	'x',	/* 16th */
	'r',	/* 32nd */
	't',	/* 64th */
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
call SetGPopUpChoice and return True; if not, return False. In addition to the familiar
duration key equivalents (symcodes in the symtable), DurPopupKey recognizes '.' as
a way to cycle through the number of aug dots allowed by the popup, and '?' as an
equivalent for the unknown duration item (if it's in the popup). (The latter is
undesirable, since the symtable has no unknown duration symbol and thus no standard
symcode; '?' is the standard symcode for bass clef.) DurPopupKey calls TranslatePalChar
before doing anything, so it recognizes remapped duration key equivalents (unless one
of them is '?'!). NB: If the symcodes in symtable are ever changed, make corresponding
changes to the static durKeys array above. */

Boolean DurPopupKey(PGRAPHIC_POPUP p, POPKEY *pk, unsigned char theChar)
{
	register short	i, curItem, newItem=0, newDurCode, curDurCode, numItems, curNumDots;
	Boolean			isDotChar=False;
	
	/* remap theChar according to the 'PLMP' resource */
	if (theChar!='?') {									/* because hard-wired below! */
		short intChar = (short)theChar;
		TranslatePalChar(&intChar, 0, False);
		theChar = (unsigned char)intChar;
	}
	
	numItems = p->numItems;
	curItem = p->currentChoice;
	curDurCode = pk[curItem].durCode;
	curNumDots = pk[curItem].numDots;
	
	newDurCode = DurPopChar2Dur(theChar);
	if (newDurCode==NOMATCH) {						/* maybe it's another char we recognize */
		if (theChar=='.')							/* same as aug dot's symcode in symtable */
			isDotChar = True;
		else if (theChar=='?')						/* This certainly shouldn't be hard-wired, */
			newDurCode = UNKNOWN_L_DUR;				/* but there's no symcode for it! */
		else
			return False;							/* no action for this key */
	}
	
	if (isDotChar) {
		for (i=curItem+1; ; i++) {					/* NB: do not use newDurCode in this block */
			if (i>numItems) i = 1;
			if (pk[i].durCode==curDurCode) {		/* loop 'till we match durCodes */
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
	return True;									/* the keystroke was directed at popup */
}
