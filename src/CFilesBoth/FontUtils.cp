/* FontUtils.c - Font utility routines for Nightingale  */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
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
		
	// Create a dummy instance iterator here to avoid creating and destroying 
	// it in the loop.
	status = FMCreateFontFamilyInstanceIterator(0, &fontFamilyInstanceIterator);

	// Create an iterator to enumerate the font families.
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
		// Invalidate all the font references
		for (short j = 0; j<doc->nfontsUsed; j++)
			doc->fontTable[j].fontID = -1;		
	}

	// Dispose of the iterators.
	FMDisposeFontFamilyIterator (&fontFamilyIterator);
	FMDisposeFontFamilyInstanceIterator (&fontFamilyInstanceIterator);
}
