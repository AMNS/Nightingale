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
	FMFontFamilyIterator         	fontFamilyIterator;
	FMFontFamilyInstanceIterator 	fontFamilyInstanceIterator;
	FMFontFamily                 	fontFamily;
	OSStatus						status;
	Str255							fontFamilyName;
	
	for (short j = 0; j<doc->nfontsUsed; j++)
		doc->fontTable[j].fontID = -1;
		
	/* Create a dummy instance iterator here to avoid creating and destroying it in
	   the loop. */
	status = FMCreateFontFamilyInstanceIterator(0, &fontFamilyInstanceIterator);

	/* Create an iterator to enumerate the font families. */
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
/* Get FontInfo for the specified font, size, and style (as opposed to the current
font in its current size and style). */

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
