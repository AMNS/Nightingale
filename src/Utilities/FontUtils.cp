/* FontUtils.c - Font utility routines for Nightingale  */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

void EnumerateFonts(Document *doc)
{
	FMFontFamilyIterator         fontFamilyIterator;
	FMFontFamilyInstanceIterator fontFamilyInstanceIterator;
	FMFontFamily                 fontFamily;
	OSStatus					status;
	Str255						fontFamilyName;
	
	for (short j = 0; j<doc->nfontsUsed; j++)
		doc->fontTable[j].fontID = -1;
		
	/* Create a dummy instance iterator here to avoid creating and destroying it in
	   the loop. */
	   
	status = FMCreateFontFamilyInstanceIterator(0, &fontFamilyInstanceIterator);

	/* Create an iterator to enumerate the font families and interate away. */
	
	status = FMCreateFontFamilyIterator(NULL, NULL, kFMDefaultOptions, 
											&fontFamilyIterator);

	while ( (status = FMGetNextFontFamily(&fontFamilyIterator, &fontFamily)) == noErr) {
		status = FMGetFontFamilyName (fontFamily, fontFamilyName);

		for (short j = 0; j<doc->nfontsUsed; j++)
			if (Pstrneql((StringPtr)doc->fontTable[j].fontName,
							 (StringPtr)fontFamilyName, 32)) {
				doc->fontTable[j].fontID = fontFamily;
		}
	}

	if (status != noErr && status != kFMIterationCompleted) {
	
		/* Invalidate all the font references */
		
		for (short j = 0; j<doc->nfontsUsed; j++)
			doc->fontTable[j].fontID = -1;		
	}

	/* Dispose of the iterators. */
	
	FMDisposeFontFamilyIterator (&fontFamilyIterator);
	FMDisposeFontFamilyInstanceIterator (&fontFamilyInstanceIterator);
}


/* ---------------------------------------------------------------------- GetNFontInfo -- */
/* Get FontInfo for the specified font, size, and style (as opposed to the current font
in its current size and style). */

void GetNFontInfo(
			short fontID,
			short size,			/* in points, i.e., pixels at 100% magnification */
			short style,
			FontInfo *pfInfo)
{
	short oldFont, oldSize, oldStyle;

	oldFont = GetPortTxFont();
	oldSize = GetPortTxSize();
	oldStyle = GetPortTxFace();

	TextFont(fontID);
	TextSize(size);
	TextFace(style);
		
	GetFontInfo(pfInfo);

	TextFont(oldFont);
	TextSize(oldSize);
	TextFace(oldStyle);
}


/* --------------------------------------------------------------------- GetFontNumber -- */
/* GetFontNumber returns in the <fontNum> parameter the number for the font with the
given font name. If there's no such font, it returns False. From Inside Mac VI, 12-16. */

Boolean GetFontNumber(const Str255 fontName, short *pFontNum)
{
	Str255 systemFontName;
	
	GetFNum(fontName, pFontNum);
	if (*pFontNum==0) {
		/* Either the font was not found, or it is the system font. */
		
		GetFontName(0, systemFontName);
		return EqualString(fontName, systemFontName, False, False);
	}
	else
		return True;
}


/* ---------------------------------------------------- FontName2Index, FontIndex2Name -- */

/* Get the fontname's index into our stored-to-system font number table; if the
fontname doesn't appear in the table, add it. In either case, return the index of
the fontname. Exception: if the table overflows, return -1. */

short FontName2Index(Document *doc, StringPtr fontName)
{
	short i, nfontsUsed;

	nfontsUsed = doc->nfontsUsed;

	/* If font is already in the table, just return its index. */
	
	for (i = 0; i<nfontsUsed; i++)
		if (Pstrneql((StringPtr)fontName, 
						 (StringPtr)doc->fontTable[i].fontName, 32))
			return i;
			
	/* It's not in the table: if there's room, add it and return its index. */
	
	if (nfontsUsed>=MAX_SCOREFONTS) {
		LogPrintf(LOG_WARNING, "FontName2Index: limit of %d fonts exceeded.\n",
				  MAX_SCOREFONTS);
		return -1;
	}
	else {
		PStrncpy((StringPtr)doc->fontTable[nfontsUsed].fontName, (StringPtr)fontName, 32);
		GetFNum(fontName, (short *)&doc->fontTable[nfontsUsed].fontID);
		doc->nfontsUsed++;
		return nfontsUsed;
	}
}

/* If the font with the given system font ID is in our font number table, deliver its
<fontName> and return True; else just return False. */ 

Boolean FontID2Name(Document *doc, short fontID, StringPtr fontName)
{
	short i, nfontsUsed;
	
	nfontsUsed = doc->nfontsUsed;
	for (i = 0; i<nfontsUsed; i++) {
		if (doc->fontTable[i].fontID==fontID) {
			PStrncpy((StringPtr)fontName, (StringPtr)doc->fontTable[i].fontName, 32);
			return True;
		}
	}
	
	return False;
}


/* -------------------------------------------- User2HeaderFontNum, Header2UserFontNum -- */

/* FIXME: These names are dumb! Change 'em to User2StyleFontNum and Style2UserFontNum. */

short User2HeaderFontNum(Document */*doc*/, short styleChoice)
{
	switch (styleChoice) {
		case TSRegular1STYLE:
			return FONT_R1;
		case TSRegular2STYLE:
			return FONT_R2;
		case TSRegular3STYLE:
			return FONT_R3;
		case TSRegular4STYLE:
			return FONT_R4;
		case TSRegular5STYLE:
			return FONT_R5;
		case TSRegular6STYLE:
			return FONT_R6;
		case TSRegular7STYLE:
			return FONT_R7;
		case TSRegular8STYLE:
			return FONT_R8;
		case TSRegular9STYLE:
			return FONT_R9;
		case TSThisItemOnlySTYLE:
			return FONT_THISITEMONLY;
		default:
			MayErrMsg("User2HeaderFontNum: styleChoice %ld is illegal", (long)styleChoice);
			return FONT_THISITEMONLY;
	}
}

short Header2UserFontNum(short globalFontIndex)
{
	switch (globalFontIndex) {
		case FONT_R1:
			return TSRegular1STYLE;
		case FONT_R2:
			return TSRegular2STYLE;
		case FONT_R3:
			return TSRegular3STYLE;
		case FONT_R4:
			return TSRegular4STYLE;
		case FONT_R5:
			return TSRegular5STYLE;
		case FONT_R6:
			return TSRegular6STYLE;
		case FONT_R7:
			return TSRegular7STYLE;
		case FONT_R8:
			return TSRegular8STYLE;
		case FONT_R9:
			return TSRegular9STYLE;
		case FONT_THISITEMONLY:
			return TSThisItemOnlySTYLE;
		default:
			MayErrMsg("Header2UserFontNum: globalFontIndex %ld is illegal",
						(long)globalFontIndex);
			return TSThisItemOnlySTYLE;
	}
}
