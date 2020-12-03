/*	EndianUtils.c

Nightingale score files are stored in Big Endian form; so are many of Nightingale's
resources -- the CNFG, MIDI ModNR table, PaletteGlobals resources, etc. If we're
running on a Little Endian processor (Intel or compatible), these functions correct
the byte order in fields of more than one byte; if we're on a Big Endian processor
(PowerPC), they do nothing. These should be called immediately after opening the file
or resource to ensure everything is in the appropriate Endian form internally, and
immediately before saving them them to ensure everything is saved in Big Endian form.

EndianFixStringPool() does the same thing as these functions for our string pool,
but the internals of the string pool are intentionally hidden from the outside world;
as a result, the EndianFixStringPool code is located in StringPool.c. */

/* THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include <Carbon/Carbon.h>

#include "Nightingale.appl.h"

/* ---------------------------------------------------- Utilities for Endian functions -- */

void EndianFixRect(Rect *pRect)
{
	FIX_END(pRect->top);	FIX_END(pRect->left);
	FIX_END(pRect->bottom);	FIX_END(pRect->right);
}

void EndianFixPoint(Point *pPoint)
{
	FIX_END(pPoint->v);
	FIX_END(pPoint->h);
}

static unsigned short EndianFix_13BitField(unsigned short value);
static unsigned short EndianFix_13BitField(unsigned short value)
{
	unsigned short temp = value;
	FIX_END(temp);
	return temp;
}

/* ------------------------------------------------------------------ Endian functions -- */

void EndianFixConfig()
{
	FIX_END(config.maxDocuments);
	
	EndianFixRect(&config.paperRect);
	EndianFixRect(&config.pageMarg);
	EndianFixRect(&config.pageNumMarg);

	FIX_END(config.defaultTempoMM);
	FIX_END(config.lowMemory);
	FIX_END(config.minMemory);
	
	EndianFixPoint(&config.toolsPosition);

	FIX_END(config.numRows);			FIX_END(config.numCols);
	FIX_END(config.maxRows);			FIX_END(config.maxCols);
	FIX_END(config.vPageSep);			FIX_END(config.hPageSep);
	FIX_END(config.vScrollSlop);		FIX_END(config.hScrollSlop);
	EndianFixPoint(&config.origin);

	FIX_END(config.musicFontID);
	FIX_END(config.numMasters);
	
	FIX_END(config.chordSymMusSize);
	FIX_END(config.defaultPatch);
	FIX_END(config.rainyDayMemory);
	FIX_END(config.tryTupLevels);
	
	FIX_END(config.metroDur);
	
	FIX_END(config.trebleVOffset);
	FIX_END(config.cClefVOffset);
	FIX_END(config.bassVOffset);
	
	FIX_END(config.chordFrameFontID);
	FIX_END(config.thruDevice);
	
	FIX_END(config.cmMetroDev);
	FIX_END(config.cmDfltInputDev);
	FIX_END(config.cmDfltOutputDev);
}

void EndianFixMIDIModNRTable()
{
	for (short i = 0; i<32; i++) {
		FIX_END(modNRVelOffsets[i]);
		FIX_END(modNRDurFactors[i]);
	}
}

void EndianFixPaletteGlobals(short idx)
{
	FIX_END((*paletteGlobals[idx])->currentItem);
	FIX_END((*paletteGlobals[idx])->itemHilited);
	FIX_END((*paletteGlobals[idx])->maxAcross);
	FIX_END((*paletteGlobals[idx])->maxDown);
	FIX_END((*paletteGlobals[idx])->across);
	FIX_END((*paletteGlobals[idx])->down);
	FIX_END((*paletteGlobals[idx])->oldAcross);
	FIX_END((*paletteGlobals[idx])->oldDown);
	FIX_END((*paletteGlobals[idx])->firstAcross);
	FIX_END((*paletteGlobals[idx])->firstDown);

	EndianFixRect(&((*paletteGlobals[idx])->origPort));
	EndianFixPoint(&((*paletteGlobals[idx])->position));
	FIX_END((*paletteGlobals[idx])->zoomAcross);
	FIX_END((*paletteGlobals[idx])->zoomDown);
}

void EndianFixSpaceMap(Document *doc)
{
	for (short i = 0; i<MAX_L_DUR; i++)
		FIX_END(doc->spaceMap[i]);
}

unsigned short uTmp;
unsigned long ulTmp;

#define FIX_USHRT_END(x)	uTmp = (x); FIX_END(uTmp); (x) = uTmp;
#define FIX_ULONG_END(x)	ulTmp = (x); FIX_END(ulTmp); (x) = ulTmp;

void EndianFixDocumentHdr(Document *doc)
{
	EndianFixPoint(&doc->origin);	

	EndianFixRect(&doc->paperRect);
	EndianFixRect(&doc->origPaperRect);

	EndianFixPoint(&doc->holdOrigin);
	
	EndianFixRect(&doc->marginRect);

	EndianFixPoint(&doc->sheetOrigin);

	FIX_USHRT_END(doc->numSheets);
	FIX_USHRT_END(doc->firstSheet);
	FIX_USHRT_END(doc->firstPageNumber);
	FIX_USHRT_END(doc->startPageNumber);
	FIX_USHRT_END(doc->numRows);
	FIX_USHRT_END(doc->numCols);
	FIX_USHRT_END(doc->pageType);
	FIX_USHRT_END(doc->measSystem);

	EndianFixRect(&doc->headerFooterMargins);
	EndianFixRect(&doc->currentPaper);
}

void EndianFixScoreHdr(Document *doc)
{
	FIX_END(doc->nstaves);
	FIX_END(doc->nsystems);
	FIX_END(doc->spacePercent);
	FIX_END(doc->srastral);
	FIX_END(doc->altsrastral);
	FIX_END(doc->tempo);
	FIX_END(doc->channel);
	FIX_END(doc->velocity);
	FIX_ULONG_END(doc->headerStrOffset);
	FIX_ULONG_END(doc->footerStrOffset);
	FIX_END(doc->dIndentOther);
//LogPrintf(LOG_DEBUG, "firstMNNumber=%d=0x%x\n", doc->firstMNNumber, doc->firstMNNumber);
	doc->firstMNNumber = EndianFix_13BitField(doc->firstMNNumber);	/* Special treatment for bitfield */
//LogPrintf(LOG_DEBUG, "firstMNNumber=%d=0x%x\n", doc->firstMNNumber, doc->firstMNNumber);
	FIX_END(doc->nfontsUsed);
	FIX_END(doc->magnify);
	FIX_END(doc->selStaff);
	FIX_END(doc->currentSystem);
	FIX_END(doc->htight);
	FIX_END(doc->ledgerYSp);
	FIX_END(doc->deflamTime);
	FIX_END(doc->dIndentFirst);
}

void EndianFixHeapHdr(Document * /* doc */, HEAP *heap)
{
	FIX_END((long)heap->block);
	FIX_END(heap->objSize);
	FIX_END(heap->type);
	FIX_END(heap->firstFree);
	FIX_USHRT_END(heap->nObjs);
	FIX_USHRT_END(heap->nFree);
	FIX_END(heap->lockLevel);	
}

