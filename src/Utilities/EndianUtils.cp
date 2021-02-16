/*	EndianUtils.c

Nightingale score files are stored in Big Endian form, and so are many of Nightingale's
resources -- the CNFG, MIDI ModNR table, PaletteGlobals resources, etc. If we're
running on a Little Endian processor (Intel or compatible), these functions convert
the byte order in fields of more than one byte to the opposite Endian; if we're on a
Big Endian processor (PowerPC), they do nothing. These should be called immediately
after opening the file or resource to ensure everything is in the appropriate Endian
form internally. They should also be called immediately before saving them them to
ensure everything is saved in Big Endian form, and immediately after to ensure it's
back to internal form.

EndianFixStringPool() does the same thing as these functions for our string pool,
but the internals of the string pool are intentionally hidden from the outside world;
as a result, the EndianFixStringPool code is located in StringPool.c. */

/* THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018-2020 by Avian Music Notation Foundation. All Rights Reserved.
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

static void EndianFixTextstyleRecord(TEXTSTYLE *pTSRec);
static void EndianFixTextstyleRecord(TEXTSTYLE *pTSRec)
{
	FIX_END(pTSRec->lyric);
	FIX_END(pTSRec->enclosure);
	FIX_END(pTSRec->relFSize);
	FIX_END(pTSRec->fontSize);
	FIX_END(pTSRec->fontStyle);
}


/* ---------------------------------------------------------- General Endian functions -- */

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
	FIX_END(doc->headL);
	FIX_END(doc->tailL);
	FIX_END(doc->selStartL);
	FIX_END(doc->selEndL);
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
	FIX_END(doc->aboveMN);
	FIX_END(doc->sysFirstMN);
	FIX_END(doc->startMNPrint1);
	FIX_END(doc->firstMNNumber);
	FIX_END(doc->masterHeadL);
	FIX_END(doc->masterTailL);
	
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontNameMN));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontNamePN));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontNameRM));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontName1));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontName2));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontName3));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontName4));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontNameTM));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontNameCS));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontNamePG));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontName5));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontName6));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontName7));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontName8));
	EndianFixTextstyleRecord((TEXTSTYLE *)&(doc->fontName9));
	
	FIX_END(doc->nfontsUsed);
	for (short j = 0; j<MAX_SCOREFONTS; j++)
		FIX_END(doc->fontTable[j].fontID);
	FIX_END(doc->magnify);
	FIX_END(doc->selStaff);
	FIX_END(doc->currentSystem);
	FIX_END(doc->spaceTable);
	FIX_END(doc->htight);
	FIX_END(doc->lookVoice);
	FIX_END(doc->ledgerYSp);
	FIX_END(doc->deflamTime);
	EndianFixSpaceMap(doc);
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


/* ----------------------------------------- Endian functions for objects & subobjects -- */

void EndianFixObject(LINK pL)
{
	/* First, handle OBJECTHEADER fields, which are common to all objects. */
	
//LogPrintf(LOG_DEBUG, "EndianFixObject1: pL=%u LeftLINK(pL)=%u\n", pL, LeftLINK(pL));
	FIX_END(LeftLINK(pL));
//LogPrintf(LOG_DEBUG, "EndianFixObject2: pL=%u LeftLINK(pL)=%u\n", pL, LeftLINK(pL));
	FIX_END(RightLINK(pL));
	FIX_END(FirstSubLINK(pL));
	FIX_END(LinkXD(pL));
	FIX_END(LinkYD(pL));

	/* Now handle object-type-specific fields. */

	switch (ObjLType(pL)) {
		case HEADERtype:
			break;
		case TAILtype:
			break;
		case SYNCtype:
			FIX_END(SyncTIME(pL));
			break;
		case RPTENDtype:
			FIX_END(RptEndFIRSTOBJ(pL));
			FIX_END(RptEndSTARTRPT(pL));
			FIX_END(RptEndENDRPT(pL));
			break;
		case PAGEtype:
			FIX_END(LinkLPAGE(pL));
			FIX_END(LinkRPAGE(pL));
			FIX_END(SheetNUM(pL));
			break;
		case SYSTEMtype:
			FIX_END(LinkLSYS(pL));
			FIX_END(LinkRSYS(pL));
			FIX_END(SystemNUM(pL));
			EndianFixRect((Rect *)&SystemRECT(pL));
			FIX_END(SysPAGE(pL));
			break;
		case STAFFtype:
			FIX_END(LinkLSTAFF(pL));
			FIX_END(LinkRSTAFF(pL));
			FIX_END(StaffSYS(pL));
			break;
		case MEASUREtype:
			FIX_END(LinkLMEAS(pL));
			FIX_END(LinkRMEAS(pL));
			FIX_END(MeasISFAKE(pL));
			FIX_END(MeasSPACEPCT(pL));
			FIX_END(MeasSTAFFL(pL));
			EndianFixRect(&MeasureBBOX(pL));
			FIX_END(MeasureTIME(pL));
			break;
		case CLEFtype:
			break;
		case KEYSIGtype:
			break;
		case TIMESIGtype:
			break;
		case BEAMSETtype:
			break;
		case CONNECTtype:
			break;
		case DYNAMtype:
			FIX_END(DynamFIRSTSYNC(pL));
			FIX_END(DynamLASTSYNC(pL));
			break;
		case MODNRtype: 		/* There are no objects of this type, only subobjects. */
			break;
		case GRAPHICtype:
			FIX_END(GraphicINFO(pL));
			FIX_END(GraphicTHICKNESS(pL));
			FIX_END(GraphicFONTSTYLE(pL));
			FIX_END(GraphicINFO2(pL));
			FIX_END(GraphicFIRSTOBJ(pL));
			FIX_END(GraphicLASTOBJ(pL));
			break;
		case OTTAVAtype:
			FIX_END(OttavaXDFIRST(pL));
			FIX_END(OttavaYDFIRST(pL));
			FIX_END(OttavaXDLAST(pL));
			FIX_END(OttavaYDLAST(pL));
			break;
		case SLURtype:
			FIX_END(SlurFIRSTSYNC(pL));
			FIX_END(SlurLASTSYNC(pL));
			break;
		case TUPLETtype:
			FIX_END(TupletACNXD(pL));
			FIX_END(TupletACNYD(pL));
			FIX_END(TupletXDFIRST(pL));
			FIX_END(TupletYDFIRST(pL));
			FIX_END(TupletXDLAST(pL));
			FIX_END(TupletYDLAST(pL));
			break;
		case GRSYNCtype:
			break;
		case TEMPOtype:
			FIX_END(TempoMM(pL));
			FIX_END(TempoSTRING(pL));
			FIX_END(TempoFIRSTOBJ(pL));
			FIX_END(TempoMETROSTR(pL));
			break;
		case SPACERtype:
			FIX_END(SpacerSPWIDTH(pL));
			break;
		case ENDINGtype:
			FIX_END(EndingFIRSTOBJ(pL));
			FIX_END(EndingLASTOBJ(pL));
			FIX_END(EndingENDXD(pL));
			break;
		case PSMEAStype:
			break;
		default:
			MayErrMsg("Object at L%ld has illegal type %ld.  (EndianFixObject)",
						(long)pL, (long)ObjLType(pL));
	}
}

void EndianFixSubobj(short heapIndex, LINK subL)
{
	HEAP *myHeap = Heap + heapIndex;

	if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "EndianFixSubobj: heapIndex=%d subL=%d\n",
						heapIndex, subL);

	/* First, handle the <next> field, which is common to all objects. (The other
	   SUBOBJHEADER fields are common to all but HEADER subobjs, but none of the others
	   are multiple bytes, so there's nothing else to do for the SUBOBJHEADER.) */
	
	FIX_END(NextLink(myHeap, subL));

	/* Now handle subobject-type-specific fields. */

	switch (heapIndex) {
		case HEADERtype:
			FIX_END(PartHiKEYNUM(subL));
			FIX_END(PartLoKEYNUM(subL));
			break;
		case TAILtype:								/* No subobjects */
			break;
		case SYNCtype:
			FIX_END(NoteXD(subL));
			FIX_END(NoteYD(subL));
			FIX_END(NoteYSTEM(subL));
			FIX_END(NotePLAYTIMEDELTA(subL));
			FIX_END(NotePLAYDUR(subL));
			FIX_END(NotePTIME(subL));
			FIX_END(NoteFIRSTMOD(subL));
			break;
		case RPTENDtype:							/* No multibyte fields except <next> */
			break;
		case PAGEtype:								/* No subobjects */
			break;
		case SYSTEMtype:							/* No subobjects */
			break;
		case STAFFtype:
			FIX_END(StaffTOP(subL));
			FIX_END(StaffLEFT(subL));
			FIX_END(StaffRIGHT(subL));
			FIX_END(StaffHEIGHT(subL));
			FIX_END(StaffFONTSIZE(subL));
			FIX_END(StaffFLAGLEADING(subL));
			FIX_END(StaffMINSTEMFREE(subL));
			FIX_END(StaffLEDGERWIDTH(subL));
			FIX_END(StaffNOTEHEADWIDTH(subL));
			FIX_END(StaffFRACBEAMWIDTH(subL));
			FIX_END(StaffSPACEBELOW(subL));
			//WHOLE_KSINFO??
			break;
		case MEASUREtype:
			FIX_END(FirstMeasMEASURENUM(subL));
			EndianFixRect((Rect *)&MeasMRECT(subL));
			//WHOLE_KSINFO??
			break;
		case CLEFtype:							/* No multibyte fields except <next> */
			break;
		case KEYSIGtype:
			FIX_END(KeySigXD(subL));
			break;
		case TIMESIGtype:
			FIX_END(TimeSigXD(subL));
			FIX_END(TimeSigYD(subL));
			break;
		case BEAMSETtype:
			FIX_END(NoteBeamBPSYNC(subL));
			break;
		case CONNECTtype:
			FIX_END(ConnectXD(subL));
			FIX_END(ConnectFIRSTPART(subL));
			FIX_END(ConnectLASTPART(subL));
			break;
		case DYNAMtype:
			FIX_END(DynamicXD(subL));
			FIX_END(DynamicYD(subL));
			FIX_END(DynamicENDXD(subL));
			FIX_END(DynamicENDXD(subL));
			break;
		case MODNRtype:							/* No multibyte fields except <next> */
			break;
		case GRAPHICtype:
			FIX_END(GraphicSTRING(subL));
			break;
		case OTTAVAtype:
			FIX_END(NoteOttavaOPSYNC(subL));
			break;
		case SLURtype:
			EndianFixRect(&SlurBOUNDS(subL));
			//??(SlurSEG(subL));
			EndianFixPoint(&SlurSTARTPT(subL));
			EndianFixPoint(&SlurENDPT(subL));
			EndianFixPoint((Point *)&SlurENDKNOT(subL));		
			break;
		case TUPLETtype:
			FIX_END(NoteTupleTPSYNC(subL));
			break;
		case GRSYNCtype:
			FIX_END(GRNoteYQPIT(subL));
			FIX_END(GRNoteXD(subL));
			FIX_END(GRNoteYD(subL));
			FIX_END(GRNoteYSTEM(subL));
			FIX_END(GRNotePLAYTIMEDELTA(subL));
			FIX_END(GRNotePLAYDUR(subL));
			FIX_END(GRNotePTIME(subL));
			FIX_END(GRNoteFIRSTMOD(subL));
			break;
		case TEMPOtype:								/* No subobjects */
			break;
		case SPACERtype:							/* No subobjects */
			break;
		case ENDINGtype:							/* No subobjects */
			break;
		case PSMEAStype:							/* No multibyte fields except <next> */
			break;
		default:
			MayErrMsg("For subobject at L%ld, type %ld is illegal.  (EndianFixSubobj)",
						(long)subL, (long)heapIndex);
	}
}


void EndianFixSubobjs(LINK objL)
{
	LINK partL, aNoteL, aStaffL, aMeasL, aKeySigL, aTimeSigL, aNoteBeamL, aConnectL,
			aDynamicL, aGraphicL, anOttavaL, aSlurL, aGRNoteL;
	short heapIndex = ObjLType(objL);
if (objL<10) LogPrintf(LOG_DEBUG, "EndianFixSubobjs: objL=%u heapIndex=%d FirstSubLINK()=%u\n",
				objL, heapIndex, FirstSubLINK(objL));

	switch (heapIndex) {
		case HEADERtype:
			partL = FirstSubLINK(objL);									/* Skip zeroth part */
			for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL))
				EndianFixSubobj(heapIndex, partL);
			break;
		case SYNCtype:
			for (aNoteL = FirstSubLINK(objL); aNoteL; aNoteL = NextNOTEL(aNoteL))
				EndianFixSubobj(heapIndex, aNoteL);
			break;
		case STAFFtype:
			aStaffL = FirstSubLINK(objL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
				EndianFixSubobj(heapIndex, aStaffL);
			break;
		case MEASUREtype:
			aMeasL = FirstSubLINK(objL);
			for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL))
				EndianFixSubobj(heapIndex, aMeasL);
			break;
		case KEYSIGtype:
			aKeySigL = FirstSubLINK(objL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
				EndianFixSubobj(heapIndex, aKeySigL);
			break;
		case TIMESIGtype:
			aTimeSigL = FirstSubLINK(objL);
			for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
				EndianFixSubobj(heapIndex, aTimeSigL);
			break;
		case BEAMSETtype:
			aNoteBeamL = FirstSubLINK(objL);
			for (; aNoteBeamL; aNoteBeamL=NextNOTEBEAML(aNoteBeamL))
				EndianFixSubobj(heapIndex, aNoteBeamL);
			break;
		case CONNECTtype:
			aConnectL = FirstSubLINK(objL);
			for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL))
				EndianFixSubobj(heapIndex, aConnectL);
			break;
		case DYNAMtype:
			aDynamicL = FirstSubLINK(objL);
			for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL))
				EndianFixSubobj(heapIndex, aDynamicL);
			break;
		case GRAPHICtype:
			aGraphicL = FirstSubLINK(objL);			/* Never more than one subobject */
				EndianFixSubobj(heapIndex, aGraphicL);
			break;
		case OTTAVAtype:
			anOttavaL = FirstSubLINK(objL);
			for ( ; anOttavaL; anOttavaL = NextNOTEOTTAVAL(anOttavaL))
				EndianFixSubobj(heapIndex, anOttavaL);
			break;
		case SLURtype:
			aSlurL = FirstSubLINK(objL);
			for ( ; aSlurL; aSlurL = NextSLURL(aSlurL))
				EndianFixSubobj(heapIndex, aSlurL);
			break;
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(objL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
				EndianFixSubobj(heapIndex, aGRNoteL);
			break;
		default:
			;
	}
}
