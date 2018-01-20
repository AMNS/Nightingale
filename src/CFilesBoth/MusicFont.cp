/******************************************************************************************
	FILE:	MusicFont.c
	PROJ:	Nightingale, revised for v.2.1
	DESC:	Routines for dealing with the music font or characters in it as a font.
			"The music font" is ordinarily Sonata: it can be set to another font, but
			many of these routines (as well as some other parts of Nightingale) just
			assume it's Sonata. So results with Sonata-compatible music fonts may be a
			bit strange; non-Sonata-compatible fonts are likely to be unusuable.
			
		SetTextSize				MusCharRect			BuildCharRectCache
		CharRect									NumToSonataStr
		GetMusicAscDesc			GetMNamedFontSize	Staff2MFontSize
		GetMFontSizeIndex		GetActualFontSize	GetYHeadFudge
		GetYRestFudge
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void MusCharRect(Rect [], unsigned char, long, Rect *);


/* ---------------------------------------------------------------- PrintMusFontTables -- */

void PrintMusFontTables(Document *doc)
{
#ifndef PUBLIC_VERSION
	short	i, j;

	say("Top document's music font is \"%p\".\n\n", doc->musFontName);

	say("Music font records (%d fonts)...\n\n", numMusFonts);

	for (i = 0; i<numMusFonts; i++) {
		say("\"%p\" (ID=%d, PS name: \"%p\")\n", musFontInfo[i].fontName, musFontInfo[i].fontID,
																musFontInfo[i].postscriptFontName);
		say("\nCharacter bounding boxes:\n");
		for (j = 0; j<256; j++)
			say("\t%d: %d %d %d %d\n", j,
						musFontInfo[i].cBBox[j].top,
						musFontInfo[i].cBBox[j].left,
						musFontInfo[i].cBBox[j].bottom,
						musFontInfo[i].cBBox[j].right);
		say("\nCharacter mapping:\n");
		for (j = 0; j<256; j++)
			say("\tSonata %d => %p %d\n", j, musFontInfo[i].fontName, musFontInfo[i].cMap[j]);
		say("\nCharacter offsets:\n");
		for (j = 0; j<256; j++)
			say("\t%d: xd=%d, yd=%d\n", j, musFontInfo[i].xd[j], musFontInfo[i].yd[j]);
		say("\n");
	}
#endif /* !PUBLIC_VERSION */
}


/* Given a character in the current music font, return a character transformed by the
mapping table for that font.  We use this, for example, in case the ASCII code of the
treble clef in our music font is different from Sonata. */

Byte MapMusChar(short musFontInfoIndex, Byte mchar)
{
	return musFontInfo[musFontInfoIndex].cMap[mchar];
}


/* Modify in place the given Pascal string so that all its chars are mapped according
to the current music font. */

void MapMusPString(short musFontInfoIndex, Byte str[])
{
	short i;

	for (i = str[0]; i>0; i--)
		str[i] = musFontInfo[musFontInfoIndex].cMap[str[i]];
}


/* Return the horizontal offset for the character given as <glyph>, relative to the
corresponding Sonata character (whose horizontal offset is always zero). <lnSpace> is
interline space, computed by caller as LNSPACE(pContext).  The offset returned is a
DDIST value, derived from the offset table, whose values are in percent of interline
space.  Assumes that <glyph> has already been remapped. */

DDIST MusCharXOffset(short musFontInfoIndex, Byte glyph, DDIST lnSpace)
{
	DDIST xd = musFontInfo[musFontInfoIndex].xd[glyph];
	if (xd)
		return ((long)xd * (long)lnSpace) / 100L;
	else
		return 0;
}


/* Return the vertical offset for the character given as <glyph>, relative to the
corresponding Sonata character (whose vertical offset is always zero). <lnSpace> is
interline space, computed by caller as LNSPACE(pContext).  The offset returned is a
DDIST value, derived from the offset table, whose values are in percent of interline
space.  Assumes that <glyph> has already been remapped. */

DDIST MusCharYOffset(short musFontInfoIndex, Byte glyph, DDIST lnSpace)
{
	DDIST yd = musFontInfo[musFontInfoIndex].yd[glyph];
	if (yd)
		return ((long)yd * (long)lnSpace) / 100L;
	else
		return 0;
}


Boolean MusFontHas16thFlag(short musFontInfoIndex)
{
	return musFontInfo[musFontInfoIndex].has16thFlagChars;
}


Boolean MusFontHasCurlyBraces(short musFontInfoIndex)
{
	return musFontInfo[musFontInfoIndex].hasCurlyBraceChars;
}


Boolean MusFontHasRepeatDots(short musFontInfoIndex)
{
	return musFontInfo[musFontInfoIndex].hasRepeatDotsChar;
}


Boolean MusFontUpstemFlagsHaveXOffset(short musFontInfoIndex)
{
	return musFontInfo[musFontInfoIndex].upstemFlagsHaveXOffset;
}


short MusFontStemSpaceWidthPixels(Document *doc, short musFontInfoIndex, DDIST lnSpace)
{
	if (musFontInfo[musFontInfoIndex].hasStemSpaceChar) {
		Byte glyph = MapMusChar(doc->musFontInfoIndex, MCH_stemspace);
		return CharWidth(glyph);
	}
	else {
		DDIST width = (long)(musFontInfo[musFontInfoIndex].stemSpaceWidth * lnSpace) / 100L;
		return d2p(width);
	}
}


DDIST MusFontStemSpaceWidthDDIST(short musFontInfoIndex, DDIST lnSpace)
{
	return (long)(musFontInfo[musFontInfoIndex].stemSpaceWidth * lnSpace) / 100L;
}


DDIST UpstemExtFlagLeading(short musFontInfoIndex, DDIST lnSpace)
{
	return (long)(musFontInfo[musFontInfoIndex].upstemExtFlagLeading * lnSpace) / 100L;
}


DDIST DownstemExtFlagLeading(short musFontInfoIndex, DDIST lnSpace)
{
	return (long)(musFontInfo[musFontInfoIndex].downstemExtFlagLeading * lnSpace) / 100L;
}


DDIST Upstem8thFlagLeading(short musFontInfoIndex, DDIST lnSpace)
{
	return (long)(musFontInfo[musFontInfoIndex].upstem8thFlagLeading * lnSpace) / 100L;
}


DDIST Downstem8thFlagLeading(short musFontInfoIndex, DDIST lnSpace)
{
	return (long)(musFontInfo[musFontInfoIndex].downstem8thFlagLeading * lnSpace) / 100L;
}


/* Return the index into the global musFontInfo table for the font whose font number
is <fontNum>, or -1 if this font name isn't in the table. */

static short GetMusFontIndex(short fontNum)
{
	short	i;

	for (i = 0; i < numMusFonts; i++)
		if (musFontInfo[i].fontID==fontNum)
			return i;

	return -1;
}


/* Use the doc's stored music font name to find the index into the global music font table
of this font; store this index into doc->musFontInfoIndex. If that fails, use Sonata
instead and notify the user. */

void InitDocMusicFont(Document *doc)
{
	short	fNum, index;
	char	fmtStr[256];

	GetFNum(doc->musFontName, &fNum);
	if (fNum==0) {
		GetIndCString(fmtStr, INITERRS_STRS, 29);	/* "This document uses %s as the music font, but ..." */
		sprintf(strBuf, fmtStr, doc->musFontName);
		CParamText(strBuf, "", "", "");
		NoteInform(GENERIC_ALRT);

		fNum = sonataFontNum;						/* Use Sonata instead, but don't change doc's font string. */
	}
	index = GetMusFontIndex(fNum);
	if (index==-1) {
		GetIndCString(fmtStr, INITERRS_STRS, 30);	/* "The music font used by this document (%s) is not supported by this copy of Nightingale." */
		sprintf(strBuf, fmtStr, doc->musFontName);
		CParamText(strBuf, "", "", "");
		NoteInform(GENERIC_ALRT);

		fNum = sonataFontNum;
		index = GetMusFontIndex(sonataFontNum);
	}
	doc->musicFontNum = fNum;
	doc->musFontInfoIndex = index;
}


/* ----------------------------------------------------------------------- SetTextSize -- */
/* Set the text size of the port, presumably in preparation for drawing the score. Get
the first staff object of the list to be drawn and use its fontSize field to determine
the correct text size. NB: It assumes all staves are the same size! To handle scores
with more than one staff size, it'd need to be told which staff to consider. */

void SetTextSize(Document *doc)
{
	LINK pL, headL, aStaffL;  PASTAFF aStaff;
	
	headL = doc->masterView ? doc->masterHeadL : doc->headL;
	pL = LSSearch(headL, STAFFtype, ANYONE, False, False);
	aStaffL = FirstSubLINK(pL);
	aStaff = GetPASTAFF(aStaffL);
	TextSize(UseTextSize(aStaff->fontSize, doc->magnify));
}


/* ----------------------------------------------------------------------- MusCharRect -- */

static void MusCharRect(Rect bbox[], unsigned char ch, long scale, Rect *r)
{
	long lTemp;
	
	lTemp = (scale*bbox[ch].top)/100000L;
	r->top = lTemp;
	lTemp = (scale*bbox[ch].left)/100000L;
	r->left = lTemp;
	lTemp = (scale*bbox[ch].bottom)/100000L;
	r->bottom = lTemp;
	lTemp = (scale*bbox[ch].right)/100000L;
	r->right = lTemp;
}


/* ---------------------------------------------------------------- BuildCharRectCache -- */
/*	If the given document's window's current font is the music font, builds a table of
CharRects for all characters in the current size, using metric information in the
cBBox array. If the document's current font is not the music font, does nothing. */

#define REFERENCE_CODE MCH_trebleclef		/* Code for reference symbol */
#define REFERENCE_SIZE 36.0					/* Point size of reference symbol */
#define REFERENCE_HEIGHT 61.0				/* Height of reference symbol in ref. size */
	
void BuildCharRectCache(Document *doc)
{
	Rect		*bbox;
	short		ic, refCode;
	short		actualSize;
	long		scale;
	WindowPtr	ourPort=doc->theWindow;

	if (GetWindowTxFont(ourPort)!=doc->musicFontNum) return;

	/* If we've already cached the music font in this size, we don't need to do it again. */
	
	if (GetWindowTxSize(ourPort)==charRectCache.fontSize) return;

	refCode = MapMusChar(doc->musFontInfoIndex, REFERENCE_CODE);
	bbox = musFontInfo[doc->musFontInfoIndex].cBBox;

	charRectCache.fontNum = GetWindowTxFont(ourPort);
	charRectCache.fontSize = GetWindowTxSize(ourPort);
	actualSize = GetActualFontSize(charRectCache.fontSize);
	scale = 100000.*(REFERENCE_HEIGHT*(actualSize/REFERENCE_SIZE)
			/(bbox[refCode].bottom-bbox[refCode].top));
	if (scale<200) MayErrMsg("BuildCharRectCache: scaling roundoff error 1/%ld",
									scale);

	for (ic = 0; ic<256; ic++) {
		MusCharRect(bbox, (unsigned char)ic, scale, &charRectCache.charRect[ic]);
		
		if (doc->musicFontNum==sonataFontNum) {
			if (ic==MCH_wholeNoteHead || ic==MCH_halfNoteHead || ic==MCH_quarterNoteHead)
				InsetRect(&charRectCache.charRect[ic], 0, -1);
		}
	}
}


/* -------------------------------------------------------------------------- CharRect -- */
/*	Get the enclosing rectangle for a given Sonata character in the size cached. */

Rect CharRect(short ic)
{
	Rect	r;

#ifdef MFDEBUG
	if (qd.thePort->txFont!=charRectCache.fontNum)
		LogPrintf(LOG_DEBUG, "CharRect: port's font is %ld but font cached is %ld.",
				(long)qd.thePort->txFont, (long)charRectCache.fontNum);
	if (qd.thePort->txSize!=charRectCache.fontSize)
		LogPrintf(LOG_DEBUG, "CharRect: port's font size is %ld but size cached is %ld.",
				(long)qd.thePort->txSize, (long)charRectCache.fontSize);
#endif
	r = charRectCache.charRect[ic];
	return r;
}


/* -------------------------------------------------------------------- NumToSonataStr -- */
/*	Convert an integer to a Pascal string of Sonata italic digits, e.g., for tuplet
accessory numerals. */

void NumToSonataStr(long number, unsigned char *string)
{
	short	nchars;
	
	NumToString(number, string);
	nchars = string[0];
	while (nchars>0) {
		string[nchars] = MCH_idigits[string[nchars]-'0'];
		nchars--;
	}
}


/* ------------------------------------------------------------------- GetMusicAscDesc -- */
/* Get the ascender height and descender height of the given string in the current
music font. */

void GetMusicAscDesc(
			Document *doc,
			unsigned char *string,			/* Pascal string */
			short size,						/* in points, i.e., pixels at 100% magnification */
			short *pAsc, short *pDesc		/* in points */
			)
{
	short			ascent, descent, refCode;
	unsigned short	n, k;
	long			scale;
	Rect			r, *bbox;
	
	refCode = MapMusChar(doc->musFontInfoIndex, REFERENCE_CODE);
	bbox = musFontInfo[doc->musFontInfoIndex].cBBox;

	scale = 100000.*(REFERENCE_HEIGHT*(size/REFERENCE_SIZE)
					/(bbox[refCode].bottom-bbox[refCode].top));

	ascent = descent = 0;
	n = (unsigned)string[0];
	for (k = 1; k<=n; k++) {
		MusCharRect(bbox, (unsigned char)string[k], scale, &r);
		ascent = n_max(ascent, -r.top);
		descent = n_max(descent, r.bottom);
	}
	
	*pAsc = ascent;
	*pDesc = descent;
}


/* ------------------------------------- Actual size/Font Manager font size conversion -- */

/* NB: Choosing the appropriate screen font size is complicated by the facts that (1)
the standard Sonata screen font sizes are named in a very misleading way, and (2) the
Font Manager won't tell you what RealFont size it will scale to generate non-RealFonts.
Solutions that might be better than the following code: (1) rename the sizes so they're
not misleading, (2) simulate the Font Manager's scaling rules. Unfortunately, (1) has
compatibility problems with people's existing installed fonts, and (2) is difficult and
might change. */
	
#define SFSIZES 7				/* No. of sizes of music screen font to check */

/* N.B. The Sonata-36 staff is actually 37 points high! Beware. */

static short mscrnRSize[SFSIZES] =	  {8, 12, 16, 20, 24, 32, 36};	/* Actual sizes of Sonata screen fonts */
static short mscrnFontSize[SFSIZES] = {7, 12, 14, 18, 24, 28, 36};	/* Corresponding names of sizes */

/* Given "actual" size, get Sonata screen font size in Font Manager terms  */

short GetMNamedFontSize(short screenSize)
{
	short j, deltaSize, size;
	
	size = mscrnFontSize[0];
	for (j = SFSIZES-1; j>=0; j--)
		if (screenSize >= mscrnRSize[j]) {
			deltaSize = screenSize-mscrnRSize[j];
			size = mscrnFontSize[j]+deltaSize;
			break;
		}
	return size;
}

/* Given staff size, get music screen font size in Font Manager terms */

short Staff2MFontSize(DDIST staffHeight)
{
	short pointStaffHt, j, deltaSize, size;
	
	pointStaffHt = d2pt(staffHeight);
	size = mscrnFontSize[0];
	for (j = SFSIZES-1; j>=0; j--)
		if (pointStaffHt >= mscrnRSize[j]) {
			deltaSize = pointStaffHt-mscrnRSize[j];
			size = mscrnFontSize[j]+deltaSize;
			break;
		}
	return size;
}

/* Given font size in Font Manager terms, get index into list of music screen font sizes */

short GetMFontSizeIndex(short fontSize)
{
	short j;
	
	for (j = SFSIZES-1; j>=0; j--)
		if (fontSize >= mscrnFontSize[j])
			return j;
	return 0;												/* Smaller than the smallest */
}

/* Given Sonata screen font size in Font Manager terms, get "actual" size */

short GetActualFontSize(short fMgrSize)
{
	short j, deltaSize, size;
	
	size = mscrnRSize[0];
	for (j = SFSIZES-1; j>=0; j--)
		if (fMgrSize >= mscrnFontSize[j]) {
			deltaSize = fMgrSize-mscrnFontSize[j];
			size = mscrnRSize[j]+deltaSize;
			break;
		}
	return size;
}


/* ------------------------------------------------------ GetYHeadFudge, GetYRestFudge -- */
/* These routines compute corrections for minor discrepancies in vertical origins of
symbols in the screen fonts. The problems with noteheads seem to be simple roundoff
error of at most 1 pixel; the problems with rests, however, result at least partly
from inconsistencies. Specifically, the origins of the rests agree with that of the
staff line character, which we don't use, since we draw the staff (in gray);
unfortunately, the bottom line of the staff character is above the origin in Sonata
14, 24, and 36, but below it in Sonata 18. Oh well. */

/* fudgeHeadY and fudgeRestY parallel the list of available Sonata screen font sizes. */
static short fudgeHeadY[SFSIZES] = {			/* In pixels downward */
/* Sonata  7 12 14 18 24  28 36 */
			  0, 1, 1, 1, 1, 0, 0
};

static short fudgeRestY[6][SFSIZES] = {			/* In pixels downward */
/* Sonata  7 12 14 18 24 28 36 */
			{ 1, 1, 1, 0, 1, 1, 0 },			/* Breve */
			{ 0, 0, 1, 0, 0, 1, 0 },			/* Whole */
			{ 0, 1, 0, 0, 1, 0, 0 },			/* Half */
			{ 0, 1, 0, 0, 0, 0, 0 },			/* Quarter */
			{ 0, 1, 1, 0, 0, 0, 0 },			/* Eighth */
			{ 0, 1, 1, 0, 0, 0, 0 }				/* 16th */
};

/* Get pixel Y-offset for noteheads */

short GetYHeadFudge(short fontSize)
{
	short	mFSizeIndex;						/* Index of font size in list of music screen fonts */

	mFSizeIndex = GetMFontSizeIndex(fontSize);
	return fudgeHeadY[mFSizeIndex];
}

/* Get pixel Y-offset for rests */

short GetYRestFudge(short fontSize, short durCode)
{
	short	mFSizeIndex;						/* Index of font size in list of music screen fonts */

	mFSizeIndex = GetMFontSizeIndex(fontSize);
	if (durCode>6) durCode = 6;						/* Treat shorter durs. as 16ths */
	return fudgeRestY[durCode-1][mFSizeIndex];
}
