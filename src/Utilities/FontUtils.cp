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

void DisplayAvailableFonts(void)
{
	FMFontFamilyIterator			fontFamilyIterator;
	FMFontFamilyInstanceIterator	fontFamilyInstanceIterator;
	FMFontFamily					fontFamily;
	OSStatus						status;
	Str255							fontFamilyName;

	/* Create dummy instance iterator to avoid creating and destroying it in the loop. */
	   
	status = FMCreateFontFamilyInstanceIterator(0, &fontFamilyInstanceIterator);

	/* Create an iterator to enumerate the font families, and interate away. */
	
	status = FMCreateFontFamilyIterator(NULL, NULL, kFMDefaultOptions, 
											&fontFamilyIterator);
	while ( (status = FMGetNextFontFamily(&fontFamilyIterator, &fontFamily)) == noErr) {
		status = FMGetFontFamilyName(fontFamily, fontFamilyName);
		char fontFNameC[256];

		Pstrcpy((unsigned char *)fontFNameC, fontFamilyName);
		LogPrintf(LOG_DEBUG, "DisplayAvailableFonts: fondID=%d fontFamilyName='%s'\n",
						fontFamily, PToCString((unsigned char *)fontFNameC));
						
		/* Sidestep weird bug/limitation of OS 10.5 and 10.6 Console utility. See
		   comments in ConvertObjSubobjs(). */
	   
		SleepMS(3);
	}
}


/* Fill score header's fontTable, which maps internal font numbers to system font
numbers for the computer we're running on, by running through all the fonts the
system knows about and looking for matches to the desired font names. */

void EnumerateDocumentFonts(Document *doc)
{
	FMFontFamilyIterator			fontFamilyIterator;
	FMFontFamilyInstanceIterator	fontFamilyInstanceIterator;
	FMFontFamily					fontFamily;
	OSStatus						status;
	Str255							fontFamilyName;
	
	for (short j = 0; j<doc->nfontsUsed; j++)
		doc->fontTable[j].fontID = -1;
		
	/* Create dummy instance iterator to avoid creating and destroying it in the loop. */
	   
	status = FMCreateFontFamilyInstanceIterator(0, &fontFamilyInstanceIterator);

	/* Create an iterator to enumerate the font families, and interate away. */
	
	status = FMCreateFontFamilyIterator(NULL, NULL, kFMDefaultOptions, 
											&fontFamilyIterator);
	while ( (status = FMGetNextFontFamily(&fontFamilyIterator, &fontFamily)) == noErr) {
		status = FMGetFontFamilyName(fontFamily, fontFamilyName);
		for (short j = 0; j<doc->nfontsUsed; j++)
			if (Pstrneql((StringPtr)doc->fontTable[j].fontName,
							(StringPtr)fontFamilyName, 32)) {
				doc->fontTable[j].fontID = fontFamily;
		}
	}

	if (status!=noErr && status!=kFMIterationCompleted) {
	
		/* Something went wrong. Invalidate all the font references. */
		
		for (short j = 0; j<doc->nfontsUsed; j++)
			doc->fontTable[j].fontID = -1;		
	}
	
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


/* -------------------------------------------- UI2InternalStyle, Internal2UIStyle -- */

short UI2InternalStyle(Document */*doc*/, short styleChoice)
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
			MayErrMsg("UI2InternalStyle: styleChoice %ld is illegal", (long)styleChoice);
			return FONT_THISITEMONLY;
	}
}

short Internal2UIStyle(short styleIndex)
{
	switch (styleIndex) {
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
			MayErrMsg("Internal2UIStyle: styleIndex %ld is illegal", (long)styleIndex);
			return TSThisItemOnlySTYLE;
	}
}
