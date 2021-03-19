/*	EndianUtils.c

Nightingale score files are stored in Big Endian form, and so are many of Nightingale's
resources -- the CNFG, MIDI ModNR table, PaletteGlobals resources, etc. If we're
running on a Little Endian processor (Intel or compatible), these functions convert
the byte order in fields of more than one byte to the opposite Endian; if we're on a
Big Endian processor (PowerPC), they do nothing. These should be called immediately
after opening the file or resource to ensure everything is in the appropriate Endian
form internally. They should also be called immediately before saving them them to
ensure everything is saved in Big Endian form, and immediately after to ensure it's
back to internal form. Note that there's no way to tell what the Endian-ness is of a
value, so, on a Little Endian processor, it must be changed exactly once! 

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

static void EndianFixDPoint(DPoint *pPoint);
static void EndianFixDPoint(DPoint *pPoint)
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

void EndianFixObject(LINK objL)
{
	/* First, handle OBJECTHEADER fields, which are common to all objects. */
	
//LogPrintf(LOG_DEBUG, "EndianFixObject1: objL=%u LeftLINK(objL)=%u\n", objL, LeftLINK(objL));
	FIX_END(LeftLINK(objL));
//LogPrintf(LOG_DEBUG, "EndianFixObject2: objL=%u LeftLINK(objL)=%u\n", objL, LeftLINK(objL));
	FIX_END(RightLINK(objL));
	FIX_END(FirstSubLINK(objL));
	FIX_END(LinkXD(objL));
	FIX_END(LinkYD(objL));

	/* Now handle object-type-specific fields. */

	switch (ObjLType(objL)) {
		case HEADERtype:
			break;
		case TAILtype:
			break;
		case SYNCtype:
			FIX_END(SyncTIME(objL));
			break;
		case RPTENDtype:
			FIX_END(RptEndFIRSTOBJ(objL));
			FIX_END(RptEndSTARTRPT(objL));
			FIX_END(RptEndENDRPT(objL));
			break;
		case PAGEtype:
			FIX_END(LinkLPAGE(objL));
			FIX_END(LinkRPAGE(objL));
			FIX_END(SheetNUM(objL));
			break;
		case SYSTEMtype:
			FIX_END(LinkLSYS(objL));
			FIX_END(LinkRSYS(objL));
			FIX_END(SystemNUM(objL));
			EndianFixRect((Rect *)&SystemRECT(objL));
			FIX_END(SysPAGE(objL));
			break;
		case STAFFtype:
			FIX_END(LinkLSTAFF(objL));
			FIX_END(LinkRSTAFF(objL));
			FIX_END(StaffSYS(objL));
			break;
		case MEASUREtype:
			FIX_END(LinkLMEAS(objL));
			FIX_END(LinkRMEAS(objL));
			FIX_END(MeasISFAKE(objL));
			FIX_END(MeasSPACEPCT(objL));
			FIX_END(MeasSTAFFL(objL));
			EndianFixRect(&MeasureBBOX(objL));
			FIX_END(MeasureTIME(objL));
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
			FIX_END(DynamFIRSTSYNC(objL));
			FIX_END(DynamLASTSYNC(objL));
			break;
		case MODNRtype: 		/* There are no objects of this type, only subobjects. */
			break;
		case GRAPHICtype:
			FIX_END(GraphicINFO(objL));
			FIX_END(GraphicTHICKNESS(objL));
			FIX_END(GraphicFONTSTYLE(objL));
			FIX_END(GraphicINFO2(objL));
			FIX_END(GraphicFIRSTOBJ(objL));
			FIX_END(GraphicLASTOBJ(objL));
			break;
		case OTTAVAtype:
			FIX_END(OttavaXDFIRST(objL));
			FIX_END(OttavaYDFIRST(objL));
			FIX_END(OttavaXDLAST(objL));
			FIX_END(OttavaYDLAST(objL));
			break;
		case SLURtype:
			FIX_END(SlurFIRSTSYNC(objL));
			FIX_END(SlurLASTSYNC(objL));
			break;
		case TUPLETtype:
			FIX_END(TupletACNXD(objL));
			FIX_END(TupletACNYD(objL));
			FIX_END(TupletXDFIRST(objL));
			FIX_END(TupletYDFIRST(objL));
			FIX_END(TupletXDLAST(objL));
			FIX_END(TupletYDLAST(objL));
			break;
		case GRSYNCtype:
			break;
		case TEMPOtype:
			FIX_END(TempoMM(objL));
			FIX_END(TempoSTRING(objL));
			FIX_END(TempoFIRSTOBJ(objL));
			FIX_END(TempoMETROSTR(objL));
			break;
		case SPACERtype:
			FIX_END(SpacerSPWIDTH(objL));
			break;
		case ENDINGtype:
			FIX_END(EndingFIRSTOBJ(objL));
			FIX_END(EndingLASTOBJ(objL));
			FIX_END(EndingENDXD(objL));
			break;
		case PSMEAStype:
			break;
		default:
			MayErrMsg("Object at L%ld has illegal type %ld.  (EndianFixObject)",
						(long)objL, (long)ObjLType(objL));
	}
}

void EndianFixSplineSeg(SplineSeg *seg);
void EndianFixSplineSeg(SplineSeg *seg)
{
	EndianFixDPoint(&seg->knot);
	EndianFixDPoint(&seg->c0);
	EndianFixDPoint(&seg->c1);
}

/* Switch Endian-ness of the given subobject. Return False if we find a problem. */

Boolean EndianFixSubobj(short heapIndex, LINK subL)
{
	HEAP *myHeap = Heap + heapIndex;

//LogPrintf(LOG_DEBUG, "EndianFixSubobj: heapIndex=%d subL=%d\n", heapIndex, subL);
	if (GARBAGEL(heapIndex, subL)) {
		LogPrintf(LOG_ERR, "IN HEAP %d, LINK %u IS GARBAGE! (EndianFixSubobj)\n",
				heapIndex, subL);
		return False;
	}

	/* First, handle the <next> field, which is common to all objects. (The other
	   SUBOBJHEADER fields are common to all but HEADER subobjs, but none of them
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
			/* KSINFO has no multibyte fields */
			break;
		case MEASUREtype:
			FIX_END(MeasMEASURENUM(subL));
			EndianFixRect((Rect *)&MeasMRECT(subL));
			/* KSINFO has no multibyte fields */
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
			FIX_END(DynamicENDYD(subL));
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
			EndianFixSplineSeg(&SlurSEG(subL));
			EndianFixPoint(&SlurSTARTPT(subL));
			EndianFixPoint(&SlurENDPT(subL));
			EndianFixPoint((Point *)&SlurENDKNOT(subL));		
			break;
		case TUPLETtype:
			FIX_END(NoteTupleTPSYNC(subL));
			break;
		case GRSYNCtype:
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
			return False;
	}

	return True;
}


static void EndianFixModNRs(LINK aNoteRL);
static void EndianFixModNRs(LINK aNoteRL)
{
	LINK aModL = NoteFIRSTMOD(aNoteRL);

if (aModL) LogPrintf(LOG_DEBUG, "EndianFixModNRs: aNoteRL=L%u NoteFIRSTMOD=L%u\n", aNoteRL, NoteFIRSTMOD(aNoteRL));

	/* Since the LINK to the first ModNR is embedded in the note/rest, it should have
	   already been fixed, so we start with the second LINK. */
	   
	for ( ; aModL; aModL = NextMODNRL(aModL)) {
//LogPrintf(LOG_DEBUG, "EndianFixModNRs<: NextMODNRL(aModL)=L%u\n", NextMODNRL(aModL));
		FIX_END(NextMODNRL(aModL));
//LogPrintf(LOG_DEBUG, "EndianFixModNRs>: NextMODNRL(aModL)=L%u\n", NextMODNRL(aModL));
	}
}

/* Switch Endian-ness of all the subobjects of a given object. MODNR subobjects are
attached to other subobjects -- notes or rests -- rather than objects, so we also
handle them when do the notes/rests they're attached. */
 
void EndianFixSubobjs(LINK objL)
{
	LINK partL, aNoteL, aRptL, aStaffL, aMeasL, aClefL, aKeySigL, aTimeSigL, aNoteBeamL,
			aConnectL, aDynamicL, aGraphicL, anOttavaL, aSlurL, aTupletL, aGRNoteL,
			aPSMeasL;
	short heapIndex = ObjLType(objL);
if (DETAIL_SHOW && objL<10) LogPrintf(LOG_DEBUG, "EndianFixSubobjs: objL=%u heapIndex=%d FirstSubLINK()=%u\n",
				objL, heapIndex, FirstSubLINK(objL));

	switch (heapIndex) {
		case HEADERtype:
			for (partL = FirstSubLINK(objL); partL; partL = NextPARTINFOL(partL))
				if (!EndianFixSubobj(heapIndex, partL)) return;
			break;
		case SYNCtype:
			for (aNoteL = FirstSubLINK(objL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (!EndianFixSubobj(heapIndex, aNoteL)) return;
				EndianFixModNRs(aNoteL);
			}
			break;
		case RPTENDtype:
			for (aRptL = FirstSubLINK(objL); aRptL; aRptL = NextRPTENDL(aRptL))
				if (!EndianFixSubobj(heapIndex, aRptL)) return;
			break;
		case STAFFtype:
			aStaffL = FirstSubLINK(objL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
				if (!EndianFixSubobj(heapIndex, aStaffL)) return;
			break;
		case MEASUREtype:
			aMeasL = FirstSubLINK(objL);
			for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL))
				if (!EndianFixSubobj(heapIndex, aMeasL)) return;
			break;
		case CLEFtype:
			aClefL = FirstSubLINK(objL);
			for ( ; aClefL; aClefL = NextCLEFL(aClefL))
				if (!EndianFixSubobj(heapIndex, aClefL)) return;
			break;
		case KEYSIGtype:
			aKeySigL = FirstSubLINK(objL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
				if (!EndianFixSubobj(heapIndex, aKeySigL)) return;
			break;
		case TIMESIGtype:
			aTimeSigL = FirstSubLINK(objL);
			for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
				if (!EndianFixSubobj(heapIndex, aTimeSigL)) return;
			break;
		case BEAMSETtype:
			aNoteBeamL = FirstSubLINK(objL);
			for (; aNoteBeamL; aNoteBeamL=NextNOTEBEAML(aNoteBeamL))
				if (!EndianFixSubobj(heapIndex, aNoteBeamL)) return;
			break;
		case CONNECTtype:
			aConnectL = FirstSubLINK(objL);
			for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL))
				if (!EndianFixSubobj(heapIndex, aConnectL)) return;
			break;
		case DYNAMtype:
			aDynamicL = FirstSubLINK(objL);
			for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL))
				if (!EndianFixSubobj(heapIndex, aDynamicL)) return;
			break;
		case MODNRtype:
			/* There are no objects of MODNRtype, so we should never reach here. */
			
			AlwaysErrMsg("OBJECT L%ld IS OF MODNRtype!  (EndianFixSubobjs)", (long)objL);
			break;
		case GRAPHICtype:
			aGraphicL = FirstSubLINK(objL);			/* Never has more than one subobject */
				if (!EndianFixSubobj(heapIndex, aGraphicL)) return;
			break;
		case OTTAVAtype:
			anOttavaL = FirstSubLINK(objL);
			for ( ; anOttavaL; anOttavaL = NextNOTEOTTAVAL(anOttavaL))
				if (!EndianFixSubobj(heapIndex, anOttavaL)) return;
			break;
		case SLURtype:
			aSlurL = FirstSubLINK(objL);
			for ( ; aSlurL; aSlurL = NextSLURL(aSlurL))
				if (!EndianFixSubobj(heapIndex, aSlurL)) return;
			break;
		case TUPLETtype:
			aTupletL = FirstSubLINK(objL);
			for ( ; aTupletL; aTupletL = NextNOTETUPLEL(aTupletL))
				if (!EndianFixSubobj(heapIndex, aTupletL)) return;
			break;
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(objL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
				if (!EndianFixSubobj(heapIndex, aGRNoteL)) return;
			break;
		case PSMEAStype:
			aPSMeasL = FirstSubLINK(objL);
			for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL))
				if (!EndianFixSubobj(heapIndex, aPSMeasL)) return;
			break;			
		default:
			;
	}
}
