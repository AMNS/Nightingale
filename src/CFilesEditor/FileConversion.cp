/******************************************************************************************
*	FILE:	FileConversion.c
*	PROJ:	Nightingale
*	DESC:	Format conversion and hacking of normal Nightingale files
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2020 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "FileConversion.h"			/* Must follow Nightingale.precomp.h! */

#define SUBOBJ_DETAIL_SHOW False
#define OBJ_DETAIL_SHOW False


/* -------------------------------------------------------------- Convert file headers -- */

void ConvertDocumentHeader(Document *doc, DocumentN105 *docN105)
{
	doc->origin = docN105->origin;
	doc->paperRect = docN105->paperRect;
	doc->origPaperRect = docN105->origPaperRect;
	doc->holdOrigin = docN105->holdOrigin;
	doc->marginRect = docN105->marginRect;
	doc->sheetOrigin = docN105->sheetOrigin;
	
	doc->currentSheet = docN105->currentSheet;
	doc->numSheets = docN105->numSheets;
	doc->firstSheet = docN105->firstSheet;
	doc->firstPageNumber = docN105->firstPageNumber;
	doc->startPageNumber = docN105->startPageNumber;
	doc->numRows = docN105->numRows;
	doc->numCols = docN105->numCols;
	doc->pageType = docN105->pageType;
	doc->measSystem = docN105->measSystem;
	doc->headerFooterMargins = docN105->headerFooterMargins;
	doc->currentPaper = docN105->currentPaper;
}


static void ConvertFontTable(Document *doc, DocumentN105 *docN105);
static void ConvertFontTable(Document *doc, DocumentN105 *docN105)
{
	for (short i = 0; i<doc->nfontsUsed; i++)
		doc->fontTable[i] = docN105->fontTable[i];
	doc->nfontsUsed = docN105->nfontsUsed;
}


void ConvertScoreHeader(Document *doc, DocumentN105 *docN105)
{
	short j, v;
	
	doc->headL = docN105->headL;
	doc->tailL = docN105->tailL;
	doc->selStartL = docN105->selStartL;
	doc->selEndL = docN105->selEndL;
	doc->nstaves = docN105->nstaves;
	doc->nsystems = docN105->nsystems;
	strcpy((char *)doc->comment, (char *)docN105->comment);
	doc->noteInsFeedback = docN105->feedback;
	doc->dontSendPatches = docN105->dontSendPatches;
	doc->saved = docN105->saved;
	doc->named = docN105->named;
	doc->used = docN105->used;
	doc->transposed = docN105->transposed;
	doc->fillerSC1 = docN105->lyricText;
	doc->polyTimbral = docN105->polyTimbral;
	doc->fillerSC2 = docN105->currentPage;
	doc->spacePercent = docN105->spacePercent;
	doc->srastral = docN105->srastral;
	doc->altsrastral = docN105->altsrastral;
	doc->tempo = docN105->tempo;
	doc->channel = docN105->channel;
	doc->velOffset = docN105->velocity;
	doc->headerStrOffset = docN105->headerStrOffset;
	doc->footerStrOffset = docN105->footerStrOffset;
	doc->topPGN = docN105->topPGN;
	doc->hPosPGN = docN105->hPosPGN;
	doc->alternatePGN = docN105->alternatePGN;
	doc->useHeaderFooter = docN105->useHeaderFooter;
	doc->fillerPGN = docN105->fillerPGN;
	doc->fillerMB = docN105->fillerMB;
	doc->filler2 = docN105->filler2;
	doc->dIndentOther = docN105->dIndentOther;
	doc->firstNames = docN105->firstNames;
	doc->otherNames = docN105->otherNames;
	doc->lastGlobalFont = docN105->lastGlobalFont;
	doc->xMNOffset = docN105->xMNOffset;
	doc->yMNOffset = docN105->yMNOffset;
	doc->xSysMNOffset = docN105->xSysMNOffset;
	doc->aboveMN = docN105->aboveMN;
	doc->sysFirstMN = docN105->sysFirstMN;
	doc->startMNPrint1 = docN105->startMNPrint1;
	doc->firstMNNumber = docN105->firstMNNumber;
	doc->masterHeadL = docN105->masterHeadL;
	doc->masterTailL = docN105->masterTailL;
	doc->filler1 = docN105->filler1;
	doc->nFontRecords = docN105->nFontRecords;
	
	Pstrcpy(doc->fontNameMN, docN105->fontNameMN);			/* Measure no. font */
	doc->fillerMN = docN105->fillerMN;
	doc->lyricMN = docN105->lyricMN;
	doc->enclosureMN = docN105->enclosureMN;
	doc->relFSizeMN = docN105->relFSizeMN;
	doc->fontSizeMN = docN105->fontSizeMN;
	doc->fontStyleMN = docN105->fontStyleMN;
	
	Pstrcpy(doc->fontNamePN, docN105->fontNamePN);			/* Part name font */
	doc->fillerPN = docN105->fillerPN;
	doc->lyricPN = docN105->lyricPN;
	doc->enclosurePN = docN105->enclosurePN;
	doc->relFSizePN = docN105->relFSizePN;
	doc->fontSizePN = docN105->fontSizePN;
	doc->fontStylePN = docN105->fontStylePN;
	
	Pstrcpy(doc->fontNameRM, docN105->fontNameRM);			/* Rehearsal mark font */
	doc->fillerRM = docN105->fillerRM;
	doc->lyricRM = docN105->lyricRM;
	doc->enclosureRM = docN105->enclosureRM;
	doc->relFSizeRM = docN105->relFSizeRM;
	doc->fontSizeRM = docN105->fontSizeRM;
	doc->fontStyleRM = docN105->fontStyleRM;
	
	Pstrcpy(doc->fontName1, docN105->fontName1);			/* Regular font 1 */
	doc->fillerR1 = docN105->fillerR1;
	doc->lyric1 = docN105->lyric1;
	doc->enclosure1 = docN105->enclosure1;
	doc->relFSize1 = docN105->relFSize1;
	doc->fontSize1 = docN105->fontSize1;
	doc->fontStyle1 = docN105->fontStyle1;
	
	Pstrcpy(doc->fontName2, docN105->fontName2);			/* Regular font 2 */
	doc->fillerR2 = docN105->fillerR2;
	doc->lyric2 = docN105->lyric2;
	doc->enclosure2 = docN105->enclosure2;
	doc->relFSize2 = docN105->relFSize2;
	doc->fontSize2 = docN105->fontSize2;
	doc->fontStyle2 = docN105->fontStyle2;
	
	Pstrcpy(doc->fontName3, docN105->fontName3);			/* Regular font 3 */
	doc->fillerR3 = docN105->fillerR3;
	doc->lyric3 = docN105->lyric3;
	doc->enclosure3 = docN105->enclosure3;
	doc->relFSize3 = docN105->relFSize3;
	doc->fontSize3 = docN105->fontSize3;
	doc->fontStyle3 = docN105->fontStyle3;
	
	Pstrcpy(doc->fontName4, docN105->fontName4);			/* Regular font 4 */
	doc->fillerR4 = docN105->fillerR4;
	doc->lyric4 = docN105->lyric4;
	doc->enclosure4 = docN105->enclosure4;
	doc->relFSize4 = docN105->relFSize4;
	doc->fontSize4 = docN105->fontSize4;
	doc->fontStyle4 = docN105->fontStyle4;
	
	Pstrcpy(doc->fontNameTM, docN105->fontNameTM);			/* Tempo mark font */
	doc->fillerTM = docN105->fillerTM;
	doc->lyricTM = docN105->lyricTM;
	doc->enclosureTM = docN105->enclosureTM;
	doc->relFSizeTM = docN105->relFSizeTM;
	doc->fontSizeTM = docN105->fontSizeTM;
	doc->fontStyleTM = docN105->fontStyleTM;
	
	Pstrcpy(doc->fontNameCS, docN105->fontNameCS);			/* Chord symbol font */
	doc->fillerCS = docN105->fillerCS;
	doc->lyricCS = docN105->lyricCS;
	doc->enclosureCS = docN105->enclosureCS;
	doc->relFSizeCS = docN105->relFSizeCS;
	doc->fontSizeCS = docN105->fontSizeCS;
	doc->fontStyleCS = docN105->fontStyleCS;
	
	Pstrcpy(doc->fontNamePG, docN105->fontNamePG);			/* Page header/footer/no. font */
	doc->fillerPG = docN105->fillerPG;
	doc->lyricPG = docN105->lyricPG;
	doc->enclosurePG = docN105->enclosurePG;
	doc->relFSizePG = docN105->relFSizePG;
	doc->fontSizePG = docN105->fontSizePG;
	doc->fontStylePG = docN105->fontStylePG;
	
	Pstrcpy(doc->fontName5, docN105->fontName5);			/* Regular font 5 */
	doc->fillerR5 = docN105->fillerR5;
	doc->lyric5 = docN105->lyric5;
	doc->enclosure5 = docN105->enclosure5;
	doc->relFSize5 = docN105->relFSize5;
	doc->fontSize5 = docN105->fontSize5;
	doc->fontStyle5 = docN105->fontStyle5;
	
	Pstrcpy(doc->fontName6, docN105->fontName6);			/* Regular font 6 */
	doc->fillerR6 = docN105->fillerR6;
	doc->lyric6 = docN105->lyric6;
	doc->enclosure6 = docN105->enclosure6;
	doc->relFSize6 = docN105->relFSize6;
	doc->fontSize6 = docN105->fontSize6;
	doc->fontStyle6 = docN105->fontStyle6;
	
	Pstrcpy(doc->fontName7, docN105->fontName7);			/* Regular font 7 */
	doc->fillerR7 = docN105->fillerR7;
	doc->lyric7 = docN105->lyric7;
	doc->enclosure7 = docN105->enclosure7;
	doc->relFSize7 = docN105->relFSize7;
	doc->fontSize7 = docN105->fontSize7;
	doc->fontStyle7 = docN105->fontStyle7;
	
	Pstrcpy(doc->fontName8, docN105->fontName8);			/* Regular font 8 */
	doc->fillerR8 = docN105->fillerR8;
	doc->lyric8 = docN105->lyric8;
	doc->enclosure8 = docN105->enclosure8;
	doc->relFSize8 = docN105->relFSize8;
	doc->fontSize8 = docN105->fontSize8;
	doc->fontStyle8 = docN105->fontStyle8;
	
	Pstrcpy(doc->fontName9, docN105->fontName9);			/* Regular font 9 */
	doc->fillerR9 = docN105->fillerR9;
	doc->lyric9 = docN105->lyric9;
	doc->enclosure9 = docN105->enclosure9;
	doc->relFSize9 = docN105->relFSize9;
	doc->fontSize9 = docN105->fontSize9;
	doc->fontStyle9 = docN105->fontStyle9;
	
	doc->nfontsUsed = docN105->nfontsUsed;
	ConvertFontTable(doc, docN105);
	
	Pstrcpy(doc->musFontName, docN105->musFontName);
	
	doc->magnify = docN105->magnify;
	doc->selStaff = docN105->selStaff;
	doc->otherMNStaff = docN105->otherMNStaff;
	doc->numberMeas = docN105->numberMeas;
	doc->currentSystem = docN105->currentSystem;
	doc->spaceTable = docN105->spaceTable;
	doc->htight = docN105->htight;
	doc->fillerInt = docN105->fillerInt;
	doc->lookVoice = docN105->lookVoice;
	doc->fillerHP = docN105->fillerHP;
	doc->fillerLP = docN105->fillerLP;
	doc->ledgerYSp = docN105->ledgerYSp;
	doc->deflamTime = docN105->deflamTime;
	
	doc->autoRespace = docN105->autoRespace;
	doc->insertMode = docN105->insertMode;
	doc->beamRests = docN105->beamRests;
	doc->graphMode = docN105->pianoroll;
	doc->showSyncs = docN105->showSyncs;
	doc->frameSystems = docN105->frameSystems;
	doc->fillerEM = docN105->fillerEM;
	doc->colorVoices = docN105->colorVoices;
	doc->showInvis = docN105->showInvis;
	doc->showDurProb = docN105->showDurProb;
	doc->recordFlats = docN105->recordFlats;
	
	for (j = 0; j<MAX_L_DUR; j++)
		doc->spaceMap[j] = docN105->spaceMap[j];
	doc->dIndentFirst = docN105->dIndentFirst;
	doc->yBetweenSys = docN105->yBetweenSys;

	for (v = 1; v<=MAXVOICES; v++) {
		/* struct assignment works, e.g., in CopyToClip(); here, it gives a confusing
		   compiler error message, no doubt because the structs aren't quite the same.
		   So copy individual fields. */
		   
		doc->voiceTab[v].partn = docN105->voiceTab[v].partn;
		doc->voiceTab[v].voiceRole = docN105->voiceTab[v].voiceRole;
		doc->voiceTab[v].relVoice = docN105->voiceTab[v].relVoice;
	}
		
	for (j = 0; j<256-(MAXVOICES+1); j++)
		doc->expansion[j].partn = 0;
		doc->expansion[j].voiceRole = 0;
		doc->expansion[j].relVoice = 0;
}


/* --------------------------------------------------------------- HeapFixN105ObjLinks -- */

/* Versions of standard memory macros for use with 'N105'-format objects. The first
six fields of the object header -- right, left, firstSubObj, xd, yd, type -- are
unchanged, so we don't need 'N105'-specific versions of them. */

#define DGetPPAGE_5(doc,link)		(PPAGE_5)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPSYSTEM_5(doc,link)		(PSYSTEM_5)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPSTAFF_5(doc,link)		(PSTAFF_5)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPMEASURE_5(doc,link)	(PMEASURE_5)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPPSMEAS_5(doc,link)		(PPSMEAS_5)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)

#define DLinkLPAGE_5(doc,link)		( (DGetPPAGE_5(doc,link))->lPage )
#define DLinkRPAGE_5(doc,link)		( (DGetPPAGE_5(doc,link))->rPage )
#define DSysPAGE_5(doc,link)		( (DGetPSYSTEM_5(doc,link))->pageL )
#define DLinkLSYS_5(doc,link)		( (DGetPSYSTEM_5(doc,link))->lSystem )
#define DLinkRSYS_5(doc,link)		( (DGetPSYSTEM_5(doc,link))->rSystem )

#define DStaffSYS_5(doc,link)		( (DGetPSTAFF_5(doc,link))->systemL )
#define DLinkLSTAFF_5(doc,link)		( (DGetPSTAFF_5(doc,link))->lStaff )
#define DLinkRSTAFF_5(doc,link)		( (DGetPSTAFF_5(doc,link))->rStaff )
#define DMeasSYSL_5(doc,link)		( (DGetPMEASURE_5(doc,link))->systemL )
#define DMeasSTAFFL_5(doc,link)		( (DGetPMEASURE_5(doc,link))->staffL )
#define DLinkLMEAS_5(doc,link)		( (DGetPMEASURE_5(doc,link))->lMeasure )
#define DLinkRMEAS_5(doc,link)		( (DGetPMEASURE_5(doc,link))->rMeasure )


/* Traverse the main and Master Page object lists and fix up the cross pointers. This
is a specialized version of HeapFixLinks() intended to fix links in 'N105' format files
when they're opened, before the contents of objects are converted. (The file could only
have been written on a machine with the same Endianness as the machine we're running on
-- both must be PowerPC's -- so Endianness isn't a problem.) Return 0 if all is
well, else FIX_LINKS_ERR. */

short HeapFixN105ObjLinks(Document *doc)
{
	LINK 	objL, prevPage, prevSystem, prevStaff, prevMeasure;
	Boolean tailFound=False;
	
	prevPage = prevSystem = prevStaff = prevMeasure = NILINK;

#ifdef NOMORE
{	unsigned char *pSObj;
pSObj = (unsigned char *)GetPSUPEROBJ(1);
DHexDump(LOG_DEBUG, "HeapFixLinks1 L1", pSObj, 46, 4, 16);
pSObj = (unsigned char *)GetPSUPEROBJ(2);
DHexDump(LOG_DEBUG, "HeapFixLinks1 L2", pSObj, 46, 4, 16);
}
#endif

	/* First handle the main object list. */

	FIX_END(doc->headL);
	for (objL = doc->headL; !tailFound; objL = DRightLINK(doc, objL)) {
		FIX_END(DRightLINK(doc, objL));
//LogPrintf(LOG_DEBUG, "HeapFixN105ObjLinks: objL=%u type=%d in main obj list\n", objL, DObjLType(doc, objL));
		switch(DObjLType(doc, objL)) {
			case TAILtype:
				doc->tailL = objL;
				if (!doc->masterHeadL) {
					LogPrintf(LOG_ERR, "TAIL of main object list encountered before its HEAD. The file can't be opened.  (HeapFixN105ObjLinks)\n");
					goto Error;
				}
				doc->masterHeadL = objL+1;
				tailFound = True;
				DRightLINK(doc, doc->tailL) = NILINK;
				break;
			case PAGEtype:
				DLinkLPAGE_5(doc, objL) = prevPage;
				if (prevPage) DLinkRPAGE_5(doc, prevPage) = objL;
				DLinkRPAGE_5(doc, objL) = NILINK;
				prevPage = objL;
				break;
			case SYSTEMtype:
				DSysPAGE_5(doc, objL) = prevPage;
				DLinkLSYS_5(doc, objL) = prevSystem;
				if (prevSystem) DLinkRSYS_5(doc, prevSystem) = objL;
				prevSystem = objL;
				break;
			case STAFFtype:
				DStaffSYS_5(doc, objL) = prevSystem;
				DLinkLSTAFF_5(doc, objL) = prevStaff;
				if (prevStaff) DLinkRSTAFF_5(doc, prevStaff) = objL;
				prevStaff = objL;
				break;
			case MEASUREtype:
				DMeasSYSL_5(doc, objL) = prevSystem;
				DMeasSTAFFL_5(doc, objL) = prevStaff;
				DLinkLMEAS_5(doc, objL) = prevMeasure;
				if (prevMeasure) DLinkRMEAS_5(doc, prevMeasure) = objL;
				prevMeasure = objL;
				break;
			case SLURtype:
				break;
			default:
				break;
		}
	}

#ifdef NOMORE
{	unsigned char *pSObj;
pSObj = (unsigned char *)GetPSUPEROBJ(1);
DHexDump(LOG_DEBUG, "HeapFixLinks2 L1", pSObj, 46, 4, 16);
pSObj = (unsigned char *)GetPSUPEROBJ(2);
DHexDump(LOG_DEBUG, "HeapFixLinks2 L2", pSObj, 46, 4, 16);
pSObj = (unsigned char *)GetPSUPEROBJ(3);
DHexDump(LOG_DEBUG, "HeapFixLinks2 L3", pSObj, 46, 4, 16);
}
#endif

	prevPage = prevSystem = prevStaff = prevMeasure = NILINK;

	/* Now do the Master Page list. */

	for (objL = doc->masterHeadL; objL; objL = DRightLINK(doc, objL)) {
		FIX_END(DRightLINK(doc, objL));
//LogPrintf(LOG_DEBUG, "HeapFixN105ObjLinks: objL=%u type=%d in Master Page obj list\n", objL, DObjLType(doc, objL));
		switch(DObjLType(doc, objL)) {
			case HEADERtype:
				DLeftLINK(doc, doc->masterHeadL) = NILINK;
				break;
			case TAILtype:
				doc->masterTailL = objL;
				DRightLINK(doc, doc->masterTailL) = NILINK;
				return 0;
			case PAGEtype:
				DLinkLPAGE_5(doc, objL) = prevPage;
				if (prevPage) DLinkRPAGE_5(doc, prevPage) = objL;
				DLinkRPAGE_5(doc, objL) = NILINK;
				prevPage = objL;
				break;
			case SYSTEMtype:
				DSysPAGE_5(doc, objL) = prevPage;
				DLinkLSYS_5(doc, objL) = prevSystem;
				if (prevSystem) DLinkRSYS_5(doc, prevSystem) = objL;
				prevSystem = objL;
				break;
			case STAFFtype:
				DStaffSYS_5(doc, objL) = prevSystem;
				DLinkLSTAFF_5(doc, objL) = prevStaff;
				if (prevStaff) DLinkRSTAFF_5(doc, prevStaff) = objL;
				prevStaff = objL;
				break;
			case MEASUREtype:
				DMeasSYSL_5(doc, objL) = prevSystem;
				DMeasSTAFFL_5(doc, objL) = prevStaff;
				DLinkLMEAS_5(doc, objL) = prevMeasure;
				if (prevMeasure) DLinkRMEAS_5(doc, prevMeasure) = objL;
				prevMeasure = objL;
				break;
			default:
				break;
		}
	}
		
	LogPrintf(LOG_ERR, "TAIL of Master Page object list not found. The file can't be opened.  (HeapFixN105ObjLinks)\n");

Error:
	/* In case we never got into the Master Page loop or it didn't have a TAIL obj. */
	
	AlwaysErrMsg("Can't set links in memory for the file!  (HeapFixN105ObjLinks)");
	doc->masterTailL = NILINK;
	return FIX_LINKS_ERR;
}


/* -------------------------------------------------- Routines to debug the conversion -- */

/* The conversion process is complex and messy; the code below is to to help debug it. */

#include "DebugUtils.h"

#define BAD_OBJ_TABSIZE 250
static short debugBadObjCount;
static LINK badObjLink[BAD_OBJ_TABSIZE];

static void InitDebugConvert();
static void DebugConvertObj(Document *doc, LINK objL);
static void DebugConvCheckObjs(Document *doc, LINK objL, char *label);

static void InitDebugConvert()
{
	debugBadObjCount = 0;
}

/* Do whatever to help track down bug(s) involving generic object <pL>. Depending on what
the bug is, it may be better to call this after converting the object rather than before. */

static void DebugConvertObj(Document *doc, LINK objL)
{	
	/* Check subobject list, e.g., against the no. of subobjects the object says it has. */
	
	DCheck1NEntries(doc, objL);

	/* Check anything else to help find current bug(s). */
	
#if 1
	{ LINK lastSyncL;
		if (BeamsetTYPE(objL)) (void)DCheckBeamset(doc, objL, False, True, &lastSyncL);
	}
#endif

#ifdef NOMORE
	PASTAFF aStaff = GetPASTAFF(8);
	LogPrintf(LOG_DEBUG, "ConvObjs1/ASTAFF 8: st=%d top,left,ht,rt=d%d,%d,%d,%d clef=%d\n",
				aStaff->staffn, aStaff->staffTop, aStaff->staffLeft, aStaff->staffHeight,
				aStaff->staffRight, aStaff->clefType);
	
	if (OptionKeyDown() && pL>160) HeapBrowser(STAFFtype);
#endif
}


/* Do whatever to help track down type-specific bug(s) involving object <objL> in
context by checking all objects with smaller links: if called during the file-conversion
process, that means all previously converted objects. Intended for tracking down bugs
where converting one object clobbers a previous one. */

typedef struct {
	OBJECTHEADER_5 
} OBJHDR_5;
OBJHDR_5 tmpObjHeader_5;

static void DebugConvCheckObjs(Document *doc, LINK objL, char *label)
{
	LINK lastSyncL;
	Boolean isARepeat;
		
	for (LINK qL=1; qL<objL; qL++) {
		if (debugBadObjCount>=BAD_OBJ_TABSIZE) {
			LogPrintf(LOG_DEBUG, "The number of objRect problems exceeded the limit of %d.\n",
						BAD_OBJ_TABSIZE);
			return;
		}

		/* Report problems with a given object only once! */
		
		isARepeat = False;
		for (short i=0; i<debugBadObjCount; i++)
			if (badObjLink[i]==qL) { isARepeat = True; break; }
		if (isARepeat) continue;
		
		switch (ObjLType(qL)) {
			case BEAMSETtype:
				//if (DCheckBeamset(doc, qL, False, True, &lastSyncL) || rand()/RAND_MAX<0.2) {	// ??TEST!
				if (DCheckBeamset(doc, qL, False, True, &lastSyncL)) {
					LogPrintf(LOG_DEBUG, "****** (%s): Problem (no.%d) found with Beamset L%u, at objL=%Lu. (DebugConvCheckObjs)\n",
								label, debugBadObjCount, qL, objL);
					badObjLink[debugBadObjCount++] = qL;
					
					/* Maybe start the Debugger. Sadly, this seems to do nothing, at least
					   inside Xcode 2.5 under macOS 10.5. */
					
					if (DETAIL_SHOW && OptionKeyDown()) Debugger();
				}
				break;
			case SYNCtype:
			case GRSYNCtype:
				if (DCheckObjRect(doc, qL)) {
					Rect r = LinkOBJRECT(qL);
					LogPrintf(LOG_DEBUG, "****** (%s): Problem (no.%d) found with objRect for L%u, at objL=%Lu. (DebugConvCheckObjs)\n",
								label, debugBadObjCount, qL, objL);
					LogPrintf(LOG_DEBUG, "             objRect/t,l,b,r=p%d,%d,%d,%d\n",
								r.top, r.left, r.bottom, r.right);
					badObjLink[debugBadObjCount++] = qL;
				break;
			default:
				;
			
			}
		}
	}
}


/* -------------------------------- Convert the content of OBJECTS, including headers -- */

#ifdef NOMORE
/* Definitions of four functions to help convert 'N103' format and earlier files formerly
appeared here. We keep the prototypes of all and body of one as examples for future use. */

static void OldGetSlurContext(Document *, LINK, Point [], Point []);
static void ConvertChordSlurs(Document *);
static void ConvertModNRVPositions(Document *, LINK);
static void ConvertStaffLines(LINK startL);

/* Convert old staff <oneLine> field to new <showLines> field. Also initialize new
<showLedgers> field.
		Old ASTAFF had this:
			char			filler:7,
							oneLine:1;
		New ASTAFF has this:
			char			filler:3,
							showLedgers:1,
							showLines:4;
<filler> was initialized to zero in InitStaff, so we can just see if showLines is
non-zero. If it is, then the oneLine flag was set. 	-JGG, 7/22/01 */

static void ConvertStaffLines(LINK startL)
{
	LINK pL, aStaffL;

	for (pL = startL; pL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==STAFFtype) {
			for (aStaffL = FirstSubLINK(pL); aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				Boolean oneLine = (StaffSHOWLINES(aStaffL)!=0);		/* NB: this really is "oneLine"! */
				if (oneLine)
					StaffSHOWLINES(aStaffL) = 1;
				else
					StaffSHOWLINES(aStaffL) = SHOW_ALL_LINES;
				StaffSHOWLEDGERS(aStaffL) = True;
#ifdef STAFFRASTRAL
				// NB: Bad if score has more than one staff size, e.g., via
				// NightStaffSizer! See StaffSize2Rastral in ResizeStaff.c from
				// NightStaffSizer code as a way to a possible solution.
				StaffRASTRAL(aStaffL) = doc->srastral;
#endif
			}
		}
	}
}

#endif


/* ---------------------------- Convert content of SUBOBJECTS, including their headers -- */

static void KeySigN105Copy(PKSINFO_5 srcKS, PKSINFO dstKS);
static void KeySigN105Copy(PKSINFO_5 srcKS, PKSINFO dstKS)
{
	dstKS->nKSItems = srcKS->nKSItems;		
	for (short i = 0; i<srcKS->nKSItems; i++) {
	
		/* Copy the fields individually. Using struct assignment here looks like it
		   should work; but it fails to compile, saying "no match for 'operator=' in
		   'dstKS->KSINFO::KSItem, blah blah etc.'. Why? Because -- while the structs
		   have identical field names -- the structs really aren't identical. */
		   
		dstKS->KSItem[i].letcode = srcKS->KSItem[i].letcode;
		dstKS->KSItem[i].sharp = srcKS->KSItem[i].sharp;
	}
}


SUPEROBJECT tmpSuperObj;
ANOTE tmpANoteR;
ARPTEND tmpARptEnd;
ASTAFF tmpAStaff;
AMEASURE tmpAMeasure;
ACLEF tmpAClef;
AKEYSIG tmpAKeySig;
ATIMESIG tmpATimeSig;
ANOTEBEAM tmpANotebeam;
ACONNECT tmpAConnect;
ADYNAMIC tmpADynamic;
ASLUR tmpASlur;
AGRNOTE tmpAGRNote;
APSMEAS tmpAPSMeas;
AMODNR tmpModNR;

static void Convert1MODNR(Document *doc, LINK aModNRL);
static Boolean Convert1NOTER(Document *doc, LINK aNoteRL);
static Boolean Convert1STAFF(Document *doc, LINK aStaffL);
static Boolean Convert1MEASURE(Document *doc, LINK aMeasureL);
static Boolean Convert1KEYSIG(Document *doc, LINK aKeySigL);
static Boolean Convert1TIMESIG(Document *doc, LINK aTimeSigL);
static Boolean Convert1CONNECT(Document *doc, LINK aConnectL);
static Boolean Convert1DYNAMIC(Document *doc, LINK aDynamicL);
static Boolean Convert1SLUR(Document *doc, LINK aSlurL);
static Boolean Convert1GRNOTE(Document *doc, LINK aGRNoteRL);
static Boolean Convert1PSMEAS(Document * /* doc */, LINK aPSMeasL);

static void Convert1MODNR(Document * /* doc */, LINK aModNRL)
{
	AMODNR_5 a1ModNR;
	
	BlockMove(&tmpModNR, &a1ModNR, sizeof(ANOTE_5));

	ModNRSEL(aModNRL) = (&a1ModNR)->selected;
	ModNRSOFT(aModNRL) = (&a1ModNR)->soft;
	ModNRVIS(aModNRL) = (&a1ModNR)->visible;

	ModNRXSTD(aModNRL) = (&a1ModNR)->xstd;
	ModNRMODCODE(aModNRL) = (&a1ModNR)->modCode;
	ModNRDATA(aModNRL) = (&a1ModNR)->data;
	ModNRYSTDPIT(aModNRL) = (&a1ModNR)->ystdpit;

	if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1MODNR: aModNRL=%u code=%d xstd=%d ystdpit=%d\n",
	aModNRL, ModNRMODCODE(aModNRL), ModNRXSTD(aModNRL), ModNRYSTDPIT(aModNRL));
}


static Boolean Convert1NOTER(Document *doc, LINK aNoteRL)
{
	ANOTE_5 a1NoteR;
	LINK aModNRL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpANoteR, &a1NoteR, sizeof(ANOTE_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	NoteSEL(aNoteRL) = (&a1NoteR)->selected;
	NoteVIS(aNoteRL) = (&a1NoteR)->visible;
	NoteSOFT(aNoteRL) = (&a1NoteR)->soft;
#define DEBUG_EMPTY_OBJRECT
#ifdef DEBUG_EMPTY_OBJRECT
if (debugLevel[DBG_CONVERT]!=0) LogPrintf(LOG_DEBUG, "    Convert1NOTER: aNoteRL=%u sel=%d vis=%d soft=%d\n",
aNoteRL, (&a1NoteR)->selected, (&a1NoteR)->visible, (&a1NoteR)->soft);
#endif
	/* Now for the ANOTE-specific fields. */
	
	NoteINCHORD(aNoteRL) = (&a1NoteR)->inChord;
	NoteREST(aNoteRL) = (&a1NoteR)->rest;
	NoteUNPITCHED(aNoteRL) = (&a1NoteR)->unpitched;
	NoteBEAMED(aNoteRL) = (&a1NoteR)->beamed;
	NoteOtherStemSide(aNoteRL) = (&a1NoteR)->otherStemSide;
	NoteYQPIT(aNoteRL) = (&a1NoteR)->yqpit;
	NoteXD(aNoteRL) = (&a1NoteR)->xd;
	NoteYD(aNoteRL) = (&a1NoteR)->yd;
	NoteYSTEM(aNoteRL) = (&a1NoteR)->ystem;
	NotePLAYTIMEDELTA(aNoteRL) = (&a1NoteR)->playTimeDelta;
	NotePLAYDUR(aNoteRL) = (&a1NoteR)->playDur;
	NotePTIME(aNoteRL) = (&a1NoteR)->pTime;
	NoteNUM(aNoteRL) = (&a1NoteR)->noteNum;
	NoteONVELOCITY(aNoteRL) = (&a1NoteR)->onVelocity;
	NoteOFFVELOCITY(aNoteRL) = (&a1NoteR)->offVelocity;
	NoteTIEDL(aNoteRL) = (&a1NoteR)->tiedL;
	NoteTIEDR(aNoteRL) = (&a1NoteR)->tiedR;
	NoteYMOVEDOTS(aNoteRL) = (&a1NoteR)->ymovedots;
	NoteNDOTS(aNoteRL) = (&a1NoteR)->ndots;
	NoteVOICE(aNoteRL) = (&a1NoteR)->voice;
	NoteRSPIGNORE(aNoteRL) = (&a1NoteR)->rspIgnore;
	NoteACC(aNoteRL) = (&a1NoteR)->accident;
	NoteACCSOFT(aNoteRL) = (&a1NoteR)->accSoft;
	NotePLAYASCUE(aNoteRL) = (&a1NoteR)->playAsCue;
	NoteMICROPITCH(aNoteRL) = (&a1NoteR)->micropitch;
	NoteXMOVEACC(aNoteRL) = (&a1NoteR)->xmoveAcc;
	NoteMERGED(aNoteRL) = (&a1NoteR)->merged;
	NoteCOURTESYACC(aNoteRL) = (&a1NoteR)->courtesyAcc;
	NoteDOUBLEDUR(aNoteRL) = (&a1NoteR)->doubleDur;
	NoteAPPEAR(aNoteRL) = (&a1NoteR)->headShape;
	NoteXMOVEDOTS(aNoteRL) = (&a1NoteR)->xmovedots;
	NoteFIRSTMOD(aNoteRL) = (&a1NoteR)->firstMod;
	NoteSLURREDL(aNoteRL) = (&a1NoteR)->slurredL;
	NoteSLURREDR(aNoteRL) = (&a1NoteR)->slurredR;
	NoteINTUPLET(aNoteRL) = (&a1NoteR)->inTuplet;
	NoteINOTTAVA(aNoteRL) = (&a1NoteR)->inOttava;
	NoteSMALL(aNoteRL) = (&a1NoteR)->small;
	NoteTEMPFLAG(aNoteRL) = (&a1NoteR)->tempFlag;
	
	/* The following fields are nonexistent or unused in 'N105' format. */
	
	NoteARTHARMONIC(aNoteRL) = 0;
	for (short k = 0; k<6; k++) NoteSEGMENT(aNoteRL, k) = 0; 
	NoteUSERID(aNoteRL) = 0;
	NoteRESERVEDN(aNoteRL) = 0L;
	
#ifdef DEBUG_EMPTY_OBJRECT
if (debugLevel[DBG_CONVERT]!=0) LogPrintf(LOG_DEBUG, "    Convert1NOTER: aNoteRL=%u voice=%d vis=%d yqpit=%d xd=%d yd=%d playDur=%d\n",
aNoteRL, NoteVOICE(aNoteRL), NoteVIS(aNoteRL), NoteYQPIT(aNoteRL), NoteXD(aNoteRL), NoteYD(aNoteRL), NotePLAYDUR(aNoteRL));
#endif

	/* AMODNR subobjs are attached directly to ANOTEs, so convert them here. */

	aModNRL = NoteFIRSTMOD(aNoteRL);
	for ( ; aModNRL; aModNRL = NextMODNRL(aModNRL)) {
		/* Copy the subobj to a separate AMODNR so we can move fields all over the place
		   without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(MODNRheap, aModNRL, PAMODNR);
		BlockMove(pSSubObj, &tmpModNR, sizeof(AMODNR));

		Convert1MODNR(doc, aModNRL);
	}
	
	return True;
}


static Boolean Convert1RPTEND(Document * /* doc */, LINK aRptEndL)
{
	ARPTEND_5 a1RptEnd;
	
	BlockMove(&tmpARptEnd, &a1RptEnd, sizeof(ARPTEND_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	RptEndSEL(aRptEndL) = (&a1RptEnd)->selected;
	RptEndVIS(aRptEndL) = (&a1RptEnd)->visible;

	/* Now for the ARPTEND-specific fields. */
	
	RptEndCONNABOVE(aRptEndL) = (&a1RptEnd)->connAbove;
	RptEndFILLER(aRptEndL) = (&a1RptEnd)->filler;
	RptEndCONNSTAFF(aRptEndL) = (&a1RptEnd)->connStaff;
	
if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1RPTEND: aRptEndL=%u connAbove=%d connStaff=%d\n",
aRptEndL, RptEndCONNABOVE(aRptEndL), RptEndCONNSTAFF(aRptEndL));
		return True;
}


static Boolean Convert1STAFF(Document * /* doc */, LINK aStaffL)
{
	ASTAFF_5 a1Staff;
	
	BlockMove(&tmpAStaff, &a1Staff, sizeof(ASTAFF_5));
	
	StaffSTAFFN(aStaffL) = (&a1Staff)->staffn;
	StaffSEL(aStaffL) =	(&a1Staff)->selected;
	StaffVIS(aStaffL) = (&a1Staff)->visible;
	StaffFILLERSTF(aStaffL) = 0;
	StaffTOP(aStaffL) = (&a1Staff)->staffTop;
	StaffLEFT(aStaffL) = (&a1Staff)->staffLeft;
	StaffRIGHT(aStaffL) = (&a1Staff)->staffRight;
	StaffHEIGHT(aStaffL) = (&a1Staff)->staffHeight;
	StaffSTAFFLINES(aStaffL) = (&a1Staff)->staffLines;
	StaffFONTSIZE(aStaffL) = (&a1Staff)->fontSize;
	StaffFLAGLEADING(aStaffL) = (&a1Staff)->flagLeading;
	StaffMINSTEMFREE(aStaffL) = (&a1Staff)->minStemFree;
	StaffLEDGERWIDTH(aStaffL) = (&a1Staff)->ledgerWidth;
	StaffNOTEHEADWIDTH(aStaffL) = (&a1Staff)->noteHeadWidth;
	StaffFRACBEAMWIDTH(aStaffL) = (&a1Staff)->fracBeamWidth;
	StaffSPACEBELOW(aStaffL) = (&a1Staff)->spaceBelow;
	StaffCLEFTYPE(aStaffL) = (&a1Staff)->clefType;
	StaffDynamType(aStaffL) = (&a1Staff)->dynamicType;
	KeySigN105Copy((PKSINFO_5)a1Staff.KSItem, (PKSINFO)(StaffKSITEM(aStaffL)));
	StaffTIMESIGTYPE(aStaffL) = (&a1Staff)->timeSigType;
	StaffNUMER(aStaffL) = (&a1Staff)->numerator;
	StaffDENOM(aStaffL) = (&a1Staff)->denominator;
	StaffFILLER(aStaffL) = 0;
	StaffSHOWLEDGERS(aStaffL) = (&a1Staff)->showLedgers;
	StaffSHOWLINES(aStaffL) = (&a1Staff)->showLines;

	{ PASTAFF aStaff;
	aStaff = GetPASTAFF(aStaffL);
	if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG,
			"    Convert1STAFF: st=%d top,left,ht,rt=d%d,%d,%d,%d lines=%d fontSz=%d %c%c clef=%d TS=%d,%d/%d\n",
		aStaff->staffn, aStaff->staffTop,
		aStaff->staffLeft, aStaff->staffHeight,
		aStaff->staffRight, aStaff->staffLines,
		aStaff->fontSize,
		(aStaff->selected? 'S' : '.') ,
		(aStaff->visible? 'V' : '.'),
		aStaff->clefType,
		aStaff->timeSigType,
		aStaff->numerator,
		aStaff->denominator );
	}
	return True;
}


static Boolean Convert1MEASURE(Document * /* doc */, LINK aMeasureL)
{
	AMEASURE_5 a1Measure;
	
	BlockMove(&tmpAMeasure, &a1Measure, sizeof(AMEASURE_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	MeasureSEL(aMeasureL) = (&a1Measure)->selected;
	MeasureVIS(aMeasureL) = (&a1Measure)->visible;
	MeasureSOFT(aMeasureL) = (&a1Measure)->soft;

	/* Now for the AMEASURE-specific fields. */
	
	MeasureMEASUREVIS(aMeasureL) = (&a1Measure)->measureVisible;
	MeasCONNABOVE(aMeasureL) = (&a1Measure)->connAbove;
	MeasFILLER1(aMeasureL) = 0;
	MeasFILLER2(aMeasureL) = 0;
	MeasureRESERVED(aMeasureL) = 0;
	MeasMEASURENUM(aMeasureL) = (&a1Measure)->measureNum;
	MeasMRECT(aMeasureL) = (&a1Measure)->measSizeRect;
	MeasCONNSTAFF(aMeasureL) = (&a1Measure)->connStaff;
	MeasCLEFTYPE(aMeasureL) = (&a1Measure)->clefType;
	MeasDynamType(aMeasureL) = (&a1Measure)->dynamicType;
	KeySigN105Copy((PKSINFO_5)a1Measure.KSItem, (PKSINFO)(MeasKSITEM(aMeasureL)));
	MeasTIMESIGTYPE(aMeasureL) = (&a1Measure)->timeSigType;
	MeasNUMER(aMeasureL) = (&a1Measure)->numerator;
	MeasDENOM(aMeasureL) = (&a1Measure)->denominator;
	MeasXMNSTDOFFSET(aMeasureL) = (&a1Measure)->xMNStdOffset;
	MeasYMNSTDOFFSET(aMeasureL) = (&a1Measure)->yMNStdOffset;

	if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1MEASURE: aMeasureL=%u staffn=%d measNum=%d connStaff=%d\n",
	aMeasureL, MeasureSTAFFN(aMeasureL), MeasMEASURENUM(aMeasureL), MeasCONNSTAFF(aMeasureL));

	return True;
}


static Boolean Convert1CLEF(Document * /* doc */, LINK aClefL)
{
	ACLEF_5 a1Clef;
	
	BlockMove(&tmpAClef, &a1Clef, sizeof(ACLEF_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	ClefSEL(aClefL) = (&a1Clef)->selected;
	ClefVIS(aClefL) = (&a1Clef)->visible;
	ClefSOFT(aClefL) = (&a1Clef)->soft;

	/* Now for the ACLEF-specific fields. */
	
	ClefFILLER1(aClefL) = 0;
	ClefSMALL(aClefL) = (&a1Clef)->small;
	ClefFILLER2(aClefL) = 0;
	ClefXD(aClefL) = (&a1Clef)->xd;
	ClefYD(aClefL) = (&a1Clef)->yd;

if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1CLEF: aClefL=%u xd=%d\n", aClefL, ClefXD(aClefL));
	return True;
}


static Boolean Convert1KEYSIG(Document * /* doc */, LINK aKeySigL)
{
	AKEYSIG_5 a1KeySig;
	
	BlockMove(&tmpAKeySig, &a1KeySig, sizeof(AKEYSIG_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	KeySigSEL(aKeySigL) = (&a1KeySig)->selected;
	KeySigVIS(aKeySigL) = (&a1KeySig)->visible;
	KeySigSOFT(aKeySigL) = (&a1KeySig)->soft;

	/* Now for the AKEYSIG-specific fields. */
	
	KeySigSUBTYPE(aKeySigL) = (&a1KeySig)->subType;	
	KeySigNONSTANDARD(aKeySigL) = (&a1KeySig)->nonstandard;	
	KeySigFILLER1(aKeySigL) = 0;
	KeySigSMALL(aKeySigL) = (&a1KeySig)->small;
	KeySigFILLER2(aKeySigL) = 0;
	KeySigXD(aKeySigL) = (&a1KeySig)->xd;
	KeySigN105Copy((PKSINFO_5)a1KeySig.KSItem, (PKSINFO)(KeySigKSITEM(aKeySigL)));

	if (SUBOBJ_DETAIL_SHOW) {
		LogPrintf(LOG_DEBUG, "    Convert1KEYSIG: aKeySigL=%u small=%d xd=%d", aKeySigL,
		KeySigSMALL(aKeySigL), KeySigXD(aKeySigL));
		DKeySigPrintf((PKSINFO)(KeySigKSITEM(aKeySigL)));
	}
	return True;
}


static Boolean Convert1TIMESIG(Document * /* doc */, LINK aTimeSigL)
{
	ATIMESIG_5 a1TimeSig;
	
	BlockMove(&tmpATimeSig, &a1TimeSig, sizeof(ATIMESIG_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	TimeSigSEL(aTimeSigL) = (&a1TimeSig)->selected;
	TimeSigVIS(aTimeSigL) = (&a1TimeSig)->visible;
	TimeSigSOFT(aTimeSigL) = (&a1TimeSig)->soft;

	/* Now for the ATIMESIG-specific fields. */
	
	TimeSigFILLER(aTimeSigL) = 0;
	TimeSigSMALL(aTimeSigL) = (&a1TimeSig)->small;
	TimeSigCONNSTAFF(aTimeSigL) = (&a1TimeSig)->connStaff;
	TimeSigXD(aTimeSigL) = (&a1TimeSig)->xd;
	TimeSigYD(aTimeSigL) = (&a1TimeSig)->yd;

	TimeSigNUMER(aTimeSigL) = (&a1TimeSig)->numerator;
	TimeSigDENOM(aTimeSigL) = (&a1TimeSig)->denominator;

if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1TIMESIG: aTimeSigL=%u numer=%d denom=%d\n", aTimeSigL,
TimeSigNUMER(aTimeSigL), TimeSigDENOM(aTimeSigL));
	return True;
}


static Boolean Convert1BEAMSET(Document * /* doc */, LINK aBeamsetL)
{
	ANOTEBEAM_5 a1Notebeam;
	
	BlockMove(&tmpANotebeam, &a1Notebeam, sizeof(ANOTEBEAM_5));
	
	NoteBeamBPSYNC(aBeamsetL) = (&a1Notebeam)->bpSync;
	NoteBeamSTARTEND(aBeamsetL) = (&a1Notebeam)->startend;
	NoteBeamFRACS(aBeamsetL) = (&a1Notebeam)->fracs;
	NoteBeamFRACGOLEFT(aBeamsetL) = (&a1Notebeam)->fracGoLeft;
	NoteBeamFILLER(aBeamsetL) = 0;

if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1BEAMSET: aBeamsetL=%u startend=%d fracs=%d\n",
aBeamsetL, NoteBeamSTARTEND(aBeamsetL), NoteBeamFRACS(aBeamsetL));
	return True;
}


static Boolean Convert1CONNECT(Document * /* doc */, LINK aConnectL)
{
	ACONNECT_5 a1Connect;
	
	BlockMove(&tmpAConnect, &a1Connect, sizeof(ACONNECT_5));
	
	ConnectSEL(aConnectL) = (&a1Connect)->selected;	
	ConnectFILLER(aConnectL) = 0;
	ConnectCONNLEVEL(aConnectL) = (&a1Connect)->connLevel;
	ConnectCONNTYPE(aConnectL) = (&a1Connect)->connectType;
	ConnectSTAFFABOVE(aConnectL) = (&a1Connect)->staffAbove;
	ConnectSTAFFBELOW(aConnectL) = (&a1Connect)->staffBelow;
	ConnectXD(aConnectL) = (&a1Connect)->xd;

if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1CONNECT: aConnectL=%u staffAbove=%d staffBelow=%d\n",
aConnectL, ConnectSTAFFABOVE(aConnectL), ConnectSTAFFBELOW(aConnectL));
	return True;
}


static Boolean Convert1DYNAMIC(Document * /* doc */, LINK aDynamicL)
{
	ADYNAMIC_5 a1Dynamic;
	
	BlockMove(&tmpADynamic, &a1Dynamic, sizeof(ADYNAMIC_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	DynamicSEL(aDynamicL) = (&a1Dynamic)->selected;
	DynamicVIS(aDynamicL) = (&a1Dynamic)->visible;
	DynamicSOFT(aDynamicL) = (&a1Dynamic)->soft;

	/* Now for the ADYNAMIC-specific fields. */
	
	DynamicMOUTHWIDTH(aDynamicL) = (&a1Dynamic)->mouthWidth;
	DynamicSMALL(aDynamicL) = (&a1Dynamic)->small;
	DynamicOTHERWIDTH(aDynamicL) = (&a1Dynamic)->otherWidth;
	DynamicXD(aDynamicL) = (&a1Dynamic)->xd;
	DynamicYD(aDynamicL) = (&a1Dynamic)->yd;
	DynamicENDXD(aDynamicL) = (&a1Dynamic)->endxd;
	DynamicENDYD(aDynamicL) = (&a1Dynamic)->endyd;

if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1DYNAMIC: aDynamicL=%u mouthWidth=%d xd=%d\n",
aDynamicL, DynamicMOUTHWIDTH(aDynamicL), DynamicXD(aDynamicL));
	return True;
}


static Boolean Convert1SLUR(Document * /* doc */, LINK aSlurL)
{
	ASLUR_5 a1Slur;
	
	BlockMove(&tmpASlur, &a1Slur, sizeof(ASLUR_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	SlurSEL(aSlurL) = (&a1Slur)->selected;
	SlurVIS(aSlurL) = (&a1Slur)->visible;
	SlurSOFT(aSlurL) = (&a1Slur)->soft;

	/* Now for the ASLUR-specific fields. */
	
	SlurDASHED(aSlurL) = (&a1Slur)->dashed;	
	SlurFILLER(aSlurL) = (&a1Slur)->filler;	
	SlurBOUNDS(aSlurL) = (&a1Slur)->bounds;	
	SlurFIRSTIND(aSlurL) = (&a1Slur)->firstInd;	
	SlurLASTIND(aSlurL) = (&a1Slur)->lastInd;	
	SlurRESERVED(aSlurL) = (&a1Slur)->reserved;	
	SlurSEG(aSlurL)	 = (&a1Slur)->seg;
	SlurSTARTPT(aSlurL) = (&a1Slur)->startPt;	
	SlurENDPT(aSlurL)	 = (&a1Slur)->endPt;
	SlurENDKNOT(aSlurL) = (&a1Slur)->endKnot;	
//	SlurKNOT(aSlurL) = (&a1Slur)->seg.knot); ??
	
if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1SLUR: aSlurL=%u dashed=%d firstInd=%d lastInd=%d\n",
aSlurL, SlurDASHED(aSlurL), SlurFIRSTIND(aSlurL), SlurLASTIND(aSlurL));
	return True;
}


static Boolean Convert1GRNOTE(Document * /* doc */, LINK aGRNoteL)
{
	AGRNOTE_5 a1GRNote;
	
	BlockMove(&tmpAGRNote, &a1GRNote, sizeof(AGRNOTE_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	NoteSEL(aGRNoteL) = (&a1GRNote)->selected;
	NoteVIS(aGRNoteL) = (&a1GRNote)->visible;
	NoteSOFT(aGRNoteL) = (&a1GRNote)->soft;

	/* Now for the AGRNOTE-specific fields. */
	
	GRNoteINCHORD(aGRNoteL) = (&a1GRNote)->inChord;
	GRNoteUNPITCHED(aGRNoteL) = (&a1GRNote)->unpitched;
	GRNoteBEAMED(aGRNoteL) = (&a1GRNote)->beamed;
	GRNoteOtherStemSide(aGRNoteL) = (&a1GRNote)->otherStemSide;
	GRNoteYQPIT(aGRNoteL) = (&a1GRNote)->yqpit;
	GRNoteXD(aGRNoteL) = (&a1GRNote)->xd;
	GRNoteYD(aGRNoteL) = (&a1GRNote)->yd;
	GRNoteYSTEM(aGRNoteL) = (&a1GRNote)->ystem;
	//GRNotePLAYTIMEDELTA(aGRNoteL) = (&a1GRNote)->playTimeDelta;
	GRNotePLAYDUR(aGRNoteL) = (&a1GRNote)->playDur;
	GRNotePTIME(aGRNoteL) = (&a1GRNote)->pTime;
	GRNoteNUM(aGRNoteL) = (&a1GRNote)->noteNum;
	//GRNoteONVELOCITY(aGRNoteL) = (&a1GRNote)->onVelocity;
	//GRNoteOFFVELOCITY(aGRNoteL) = (&a1GRNote)->offVelocity;
	GRNoteTIEDL(aGRNoteL) = (&a1GRNote)->tiedL;
	GRNoteTIEDR(aGRNoteL) = (&a1GRNote)->tiedR;
	GRNoteYMOVEDOTS(aGRNoteL) = (&a1GRNote)->ymovedots;
	GRNoteNDOTS(aGRNoteL) = (&a1GRNote)->ndots;
	GRNoteVOICE(aGRNoteL) = (&a1GRNote)->voice;
	GRNoteACC(aGRNoteL) = (&a1GRNote)->accident;
	GRNoteACCSOFT(aGRNoteL) = (&a1GRNote)->accSoft;
	//GRNoteMICROPITCH(aGRNoteL) = (&a1GRNote)->micropitch;
	GRNoteXMOVEACC(aGRNoteL) = (&a1GRNote)->xmoveAcc;
	//GRNoteMERGED(aGRNoteL) = (&a1GRNote)->merged;
	GRNoteCOURTESYACC(aGRNoteL) = (&a1GRNote)->courtesyAcc;
	GRNoteDOUBLEDUR(aGRNoteL) = (&a1GRNote)->doubleDur;
	GRNoteAPPEAR(aGRNoteL) = (&a1GRNote)->headShape;
	GRNoteXMOVEDOTS(aGRNoteL) = (&a1GRNote)->xmovedots;
	GRNoteFIRSTMOD(aGRNoteL) = (&a1GRNote)->firstMod;
	//GRNoteSLURREDL(aGRNoteL) = (&a1GRNote)->slurredL;
	//GRNoteSLURREDR(aGRNoteL) = (&a1GRNote)->slurredR;
	//GRNoteINTUPLET(aGRNoteL) = (&a1GRNote)->inTuplet;
	GRNoteINOTTAVA(aGRNoteL) = (&a1GRNote)->inOttava;
	GRNoteSMALL(aGRNoteL) = (&a1GRNote)->small;
	GRNoteTEMPFLAG(aGRNoteL) = (&a1GRNote)->tempFlag;

	/* The following fields are nonexistent or unused in 'N105' format. */
	
	GRNoteARTHARMONIC(aGRNoteL) = 0;
	for (short k = 0; k<4; k++) GRNoteSEGMENT(aGRNoteL, k) = 0;
	GRNoteUSERID(aGRNoteL) = 0;
	GRNoteRESERVEDN(aGRNoteL) = 0L;
	
if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1GRNOTE: aGRNoteL=%u voice=%d yqpit=%d xd=%d yd=%d playDur=%d\n",
aGRNoteL, GRNoteVOICE(aGRNoteL), GRNoteYQPIT(aGRNoteL), GRNoteXD(aGRNoteL),
GRNoteYD(aGRNoteL), GRNotePLAYDUR(aGRNoteL));

	return True;
}


static Boolean Convert1PSMEAS(Document * /* doc */, LINK aPSMeasL)
{
	APSMEAS_5 a1PSMeas;
	
	BlockMove(&tmpAPSMeas, &a1PSMeas, sizeof(APSMEAS_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	PSMeasSEL(aPSMeasL) = (&a1PSMeas)->selected;
	PSMeasVIS(aPSMeasL) = (&a1PSMeas)->visible;
	PSMeasSOFT(aPSMeasL) = (&a1PSMeas)->soft;

	/* Now for the APSMEAS-specific fields. */
	
	PSMeasCONNABOVE(aPSMeasL) = (&a1PSMeas)->connAbove;	
	PSMeasFILLER1(aPSMeasL) = (&a1PSMeas)->filler1;	
	PSMeasCONNSTAFF(aPSMeasL) = (&a1PSMeas)->connStaff;	
	
if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    Convert1PSMEAS: aPSMeasL=%u staffn=%d connAbove=%d connStaff=%d\n",
aPSMeasL, PSMeasSTAFFN(aPSMeasL), PSMeasCONNABOVE(aPSMeasL), PSMeasCONNSTAFF(aPSMeasL));
		return True;
}


/* ------------------------------- Convert content of objects, including their headers -- */

/* Convert the header of the object in <tmpSuperObj>, which is assumed to be in 'N105'
format (equivalent to Nightingale 5.8 and earlier), to Nightingale 5.9.x format in
<objL>. Object headers in the new format contain exactly the same information as in the
old format, so converting them is just a matter of copying the fields to their new
locations. */
  
static void ConvertObjHeader(Document *doc, LINK objL);
static void ConvertObjHeader(Document * /* doc */, LINK objL)
{
	/* Copy the entire object header from the object list to a temporary space to
	   avoid clobbering anything before it's copied. */
	
	BlockMove(&tmpSuperObj, &tmpObjHeader_5, sizeof(OBJHDR_5));
	
	/* The first six fields of the header are in the same location in 'N105' and 'N106'
	   format, so there's no need to do anything with them. Copy the others. */
	   
	LinkSEL(objL) = tmpObjHeader_5.selected;
	LinkVIS(objL) = tmpObjHeader_5.visible;
	LinkSOFT(objL) = tmpObjHeader_5.soft;
	LinkVALID(objL) = tmpObjHeader_5.valid;
	LinkTWEAKED(objL) = tmpObjHeader_5.tweaked;
	LinkSPAREFLAG(objL) = tmpObjHeader_5.spareFlag;
	LinkOBJRECT(objL) = tmpObjHeader_5.objRect;
	LinkNENTRIES(objL) = tmpObjHeader_5.nEntries;
#ifdef DEBUG_EMPTY_OBJRECT
if ((ObjLType(objL)==SYNCtype || ObjLType(objL)==GRSYNCtype) && debugLevel[DBG_CONVERT]!=0)
LogPrintf(LOG_DEBUG, "  ConvertObjHeader: objL=L%u objRect/t,l,b,r=p%d,%d,%d,%d\n",
objL, LinkOBJRECT(objL).top, LinkOBJRECT(objL).left,
LinkOBJRECT(objL).bottom, LinkOBJRECT(objL).right);
#endif
}


/* Convert the bodies of objects of each type. */

static Boolean ConvertHEADER(Document *doc, LINK headL);
static Boolean ConvertSYNC(Document *doc, LINK syncL);
static Boolean ConvertRPTEND(Document *doc, LINK rptendL);
static Boolean ConvertPAGE(Document *doc, LINK pageL);
static Boolean ConvertSYSTEM(Document *doc, LINK sysL);
static Boolean ConvertSTAFF(Document *doc, LINK staffL);
static Boolean ConvertMEASURE(Document *doc, LINK measL);
static Boolean ConvertKEYSIG(Document *doc, LINK keySigL);
static Boolean ConvertTIMESIG(Document *doc, LINK timeSigL);
static Boolean ConvertCLEF(Document *doc, LINK clefL);
static Boolean ConvertBEAMSET(Document *doc, LINK beamsetL);
static Boolean ConvertCONNECT(Document *doc, LINK connectL);
static Boolean ConvertDYNAMIC(Document *doc, LINK dynamicL);
static Boolean ConvertGRAPHIC(Document *doc, LINK graphicL);
static Boolean ConvertOTTAVA(Document *doc, LINK ottavaL);
static Boolean ConvertSLUR(Document *doc, LINK slurL);
static Boolean ConvertTUPLET(Document *doc, LINK tupletL);
static Boolean ConvertGRSYNC(Document *doc, LINK grSyncL);
static Boolean ConvertTEMPO(Document *doc, LINK tempoL);
static Boolean ConvertSPACER(Document *doc, LINK spacerL);


static Boolean ConvertHEADER(Document * /* doc */, LINK headL)
{
	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertHEADER: headL=L%u\n", headL);

	/* The HEADER itself has nothing but the OBJECTHEADER, so there's nothing to do here
	   for that. And its subobjects, PARTINFOs, are identical in N105 and N106 formats,
	   so there's nothing to do here for them. */
	   
	return True;
}


static Boolean ConvertSYNC(Document *doc, LINK syncL)
{
	SYNC_5 aSync;
	LINK aNoteRL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aSync, sizeof(SYNC_5));
	
	SyncTIME(syncL) = (&aSync)->timeStamp;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertSYNC: syncL=L%u timeStamp=%d\n", syncL, SyncTIME(syncL)); 

	aNoteRL = FirstSubLINK(syncL);
	for ( ; aNoteRL; aNoteRL = NextNOTEL(aNoteRL)) {
		/* Copy the subobj to a separate ANOTE so we can move fields all over the place
		   without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(NOTEheap, aNoteRL, PANOTE);
//LogPrintf(LOG_DEBUG, "->block=%ld aNoteRL=%d sizeof(ANOTE)*aNoteRL=%d pSSubObj=%ld\n", (NOTEheap)->block,
//aNoteRL, sizeof(ANOTE)*aNoteRL, pSSubObj);
		BlockMove(pSSubObj, &tmpANoteR, sizeof(ANOTE));

		Convert1NOTER(doc, aNoteRL);
	}

	return True;
}


static Boolean ConvertRPTEND(Document *doc, LINK rptEndL)
{
	RPTEND_5 aRptEnd;
	LINK aRptEndL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aRptEnd, sizeof(RPTEND_5));
	
	RptEndFIRSTOBJ(rptEndL) = (&aRptEnd)->firstObj;
	RptEndSTARTRPT(rptEndL) = (&aRptEnd)->startRpt;
	RptEndENDRPT(rptEndL) = (&aRptEnd)->endRpt;
	RptType(rptEndL) = (&aRptEnd)->subType;
	RptEndCOUNT(rptEndL) = (&aRptEnd)->count;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertRPTEND: rptEndL=L%u firstObj=L%u subType=%d count=%d\n", rptEndL,
				RptEndFIRSTOBJ(rptEndL), RptType(rptEndL), RptEndCOUNT(rptEndL)); 

	aRptEndL = FirstSubLINK(rptEndL);
	for ( ; aRptEndL; aRptEndL = NextRPTENDL(aRptEndL)) {
		/* Copy the subobj to a separate ARPTEND so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(RPTENDheap, aRptEndL, PARPTEND);
//LogPrintf(LOG_DEBUG, "->block=%ld aRptEndL=%d sizeof(ARPTEND)*aRptEndL=%d pSSubObj=%ld\n", (RPTENDheap)->block,
//aRptEndL, sizeof(ARPTEND)*aRptEndL, pSSubObj);
		BlockMove(pSSubObj, &tmpARptEnd, sizeof(ARPTEND));

		Convert1RPTEND(doc, aRptEndL);
	}

	return True;
}


static Boolean ConvertPAGE(Document * /* doc */, LINK pageL)
{
	PAGE_5 aPage;
	
	BlockMove(&tmpSuperObj, &aPage, sizeof(PAGE_5));
	
	LinkLPAGE(pageL) = (&aPage)->lPage;
	LinkRPAGE(pageL) = (&aPage)->rPage;
	SheetNUM(pageL) = (&aPage)->sheetNum;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertPAGE: pageL=L%u lPage=L%u rPage=L%u sheetNum=%d\n", pageL,
				LinkLPAGE(pageL), LinkRPAGE(pageL), SheetNUM(pageL)); 
	return True;
}

static Boolean ConvertSYSTEM(Document * /* doc */, LINK sysL)
{
	SYSTEM_5 aSys;
	
	BlockMove(&tmpSuperObj, &aSys, sizeof(SYSTEM_5));
	
	LinkLSYS(sysL) = (&aSys)->lSystem;
	LinkRSYS(sysL) = (&aSys)->rSystem;
	SysPAGE(sysL) = (&aSys)->pageL;
	SystemNUM(sysL) = (&aSys)->systemNum;
	SysRectTOP(sysL) = (&aSys)->systemRect.top;
	SysRectLEFT(sysL) = (&aSys)->systemRect.left;
	SysRectBOTTOM(sysL) = (&aSys)->systemRect.bottom;
	SysRectRIGHT(sysL) = (&aSys)->systemRect.right;
	
	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertSYSTEM: sysL=L%u type=%d xd=%d sel=%d vis=%d\n", sysL, ObjLType(sysL),
				LinkXD(sysL), LinkSEL(sysL), LinkVIS(sysL));
	return True;
}


static Boolean ConvertSTAFF(Document *doc, LINK staffL)
{
	STAFF_5 aStaff;
	LINK aStaffL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aStaff, sizeof(STAFF_5));
	
	LinkLSTAFF(staffL) = (&aStaff)->lStaff;
	LinkRSTAFF(staffL) = (&aStaff)->rStaff;
	StaffSYS(staffL) = (&aStaff)->systemL;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertSTAFF: staffL=L%u nEntries=%d lStaff=L%u rStaff=L%u StaffSYS=L%u\n",
				staffL, LinkNENTRIES(staffL), LinkLSTAFF(staffL), LinkRSTAFF(staffL), StaffSYS(staffL)); 

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		/* Copy the subobj to a separate ASTAFF so we can move fields all over the place
		   without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(STAFFheap, aStaffL, PASTAFF);
		BlockMove(pSSubObj, &tmpAStaff, sizeof(ASTAFF));

		Convert1STAFF(doc, aStaffL);
	}

	return True;
}


static Boolean ConvertMEASURE(Document *doc, LINK measL)
{
	MEASURE_5 aMeasure;
	LINK aMeasureL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aMeasure, sizeof(MEASURE_5));
	
	LinkLMEAS(measL) = (&aMeasure)->lMeasure;
	LinkRMEAS(measL) = (&aMeasure)->rMeasure;
	MeasSYSL(measL) = (&aMeasure)->systemL;
	MeasSTAFFL(measL) = (&aMeasure)->staffL;
	MeasISFAKE(measL) = (&aMeasure)->fakeMeas;
	MeasSPACEPCT(measL) = (&aMeasure)->spacePercent;
	MeasureBBOX(measL) = (&aMeasure)->measureBBox;
	MeasureTIME(measL ) = (&aMeasure)->lTimeStamp;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertMEASURE: measL=L%u lMeasure=L%u rMeasure=L%u systemL=L%u\n",
				measL, LinkLMEAS(measL), LinkRMEAS(measL), MeasSYSL(measL));

	aMeasureL = FirstSubLINK(measL);
	for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
		/* Copy the subobj to a separate AMEASURE so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(MEASUREheap, aMeasureL, PAMEASURE);
//LogPrintf(LOG_DEBUG, "->block=%ld aMeasureL=%d sizeof(AMEASURE)*aMeasureL=%d pSSubObj=%ld\n", (MEASUREheap)->block,
//aMeasureL, sizeof(AMEASURE)*aMeasureL, pSSubObj);
		BlockMove(pSSubObj, &tmpAMeasure, sizeof(AMEASURE));

		Convert1MEASURE(doc, aMeasureL);
	}
	return True;
}


static Boolean ConvertCLEF(Document *doc, LINK clefL)
{
	CLEF_5 aClef;
	LINK aClefL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aClef, sizeof(CLEF_5));
	
	ClefINMEAS(clefL) = (&aClef)->inMeasure;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertCLEF: clefL=L%u inMeasure=%d\n", clefL, ClefINMEAS(clefL));

aClefL = FirstSubLINK(clefL);
	for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
		/* Copy the subobj to a separate ACLEF so we can move fields all over the place
		   without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(CLEFheap, aClefL, PACLEF);
//LogPrintf(LOG_DEBUG, "->block=%ld aClefL=%d sizeof(ACLEF)*aClefL=%d pSSubObj=%ld\n",
//(CLEFheap)->block, aClefL, sizeof(ACLEF)*aClefL, pSSubObj);
		BlockMove(pSSubObj, &tmpAClef, sizeof(ACLEF));

		Convert1CLEF(doc, aClefL);
	}

	return True;
}


static Boolean ConvertKEYSIG(Document *doc, LINK keySigL)
{
	KEYSIG_5 aKeySig;
	LINK aKeySigL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aKeySig, sizeof(KEYSIG_5));
	
	KeySigINMEAS(keySigL) = (&aKeySig)->inMeasure;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertKEYSIG: keySigL=L%u inMeasure=%d\n", keySigL,
							KeySigINMEAS(keySigL));

	aKeySigL = FirstSubLINK(keySigL);
	for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
		/* Copy the subobj to a separate AKEYSIG so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(KEYSIGheap, aKeySigL, PAKEYSIG);
//LogPrintf(LOG_DEBUG, "->block=%ld aKeySigL=%d sizeof(AKEYSIG)*aKeySigL=%d pSSubObj=%ld\n",
//(KEYSIGheap)->block, aKeySigL, sizeof(AKEYSIG)*aKeySigL, pSSubObj);
		BlockMove(pSSubObj, &tmpAKeySig, sizeof(AKEYSIG));

		Convert1KEYSIG(doc, aKeySigL);
	}

	return True;
}


static Boolean ConvertTIMESIG(Document *doc, LINK timeSigL)
{
	TIMESIG_5 aTimeSig;
	LINK aTimeSigL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aTimeSig, sizeof(TIMESIG_5));
	
	TimeSigINMEAS(timeSigL) = (&aTimeSig)->inMeasure;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertTIMESIG: timeSigL=L%u inMeasure=%d\n", timeSigL,
				TimeSigINMEAS(timeSigL));

	aTimeSigL = FirstSubLINK(timeSigL);
	for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
		/* Copy the subobj to a separate ATIMESIG so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(TIMESIGheap, aTimeSigL, PATIMESIG);
//LogPrintf(LOG_DEBUG, "->block=%ld aTimeSigL=%d sizeof(ATIMESIG)*aTimeSigL=%d pSSubObj=%ld\n",
//(TIMESIGheap)->block, aTimeSigL, sizeof(ATIMESIG)*aTimeSigL, pSSubObj);
		BlockMove(pSSubObj, &tmpATimeSig, sizeof(ATIMESIG));

		Convert1TIMESIG(doc, aTimeSigL);
	}

	return True;
}


static Boolean ConvertBEAMSET(Document *doc, LINK beamsetL)
{
	BEAMSET_5 aBeamset;
	LINK aBeamsetL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aBeamset, sizeof(BEAMSET_5));
	
	BeamSTAFF(beamsetL) = (&aBeamset)->staffn;		/* EXTOBJHEADER */

	BeamVOICE(beamsetL) = (&aBeamset)->voice;
	BeamTHIN(beamsetL) = (&aBeamset)->thin;
	BeamRESTS(beamsetL) = (&aBeamset)->beamRests;
	BeamFEATHER(beamsetL) = (&aBeamset)->feather;
	BeamGRACE(beamsetL) = (&aBeamset)->grace; 
	BeamFirstSYS(beamsetL) = (&aBeamset)->firstSystem;
	BeamCrossSTAFF(beamsetL) = (&aBeamset)->crossStaff;
	BeamCrossSYS(beamsetL) = (&aBeamset)->crossSystem;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertBEAMSET: beamsetL=L%u\n", beamsetL);

	aBeamsetL = FirstSubLINK(beamsetL);
	for ( ; aBeamsetL; aBeamsetL = NextNOTEBEAML(aBeamsetL)) {
		/* Copy the subobj to a separate ABEAMSET so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(NOTEBEAMheap, aBeamsetL, PANOTEBEAM);
//LogPrintf(LOG_DEBUG, "->block=%ld aBeamsetL=%d sizeof(ABEAMSET)*aBeamsetL=%d pSSubObj=%ld\n",
//(NOTEBEAMheap)->block, aBeamsetL, sizeof(ANOTEBEAM)*aBeamsetL, pSSubObj);
		BlockMove(pSSubObj, &tmpANotebeam, sizeof(ANOTEBEAM));

		Convert1BEAMSET(doc, aBeamsetL);
	}

	return True;
}


static Boolean ConvertCONNECT(Document *doc, LINK connectL)
{
	CONNECT_5 aConnect;
	LINK aConnectL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aConnect, sizeof(CONNECT_5));
	
	/* There's nothing to convert in the CONNECT object but its header. */

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertCONNECT: connectL=L%u\n", connectL);

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
		/* Copy the subobj to a separate ACONNECT so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(CONNECTheap, aConnectL, PACONNECT);
		BlockMove(pSSubObj, &tmpAConnect, sizeof(ACONNECT));

		Convert1CONNECT(doc, aConnectL);
	}

	return True;
}


static Boolean ConvertDYNAMIC(Document *doc, LINK dynamicL)
{
	DYNAMIC_5 aDynamic;
	LINK aDynamicL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aDynamic, sizeof(DYNAMIC_5));
	
	DynamType(dynamicL) = (&aDynamic)->dynamicType;
	DynamFILLER(dynamicL) = (&aDynamic)->filler;
	DynamCrossSYS(dynamicL) = (&aDynamic)->crossSys;
	DynamFIRSTSYNC(dynamicL) = (&aDynamic)->firstSyncL;
	DynamLASTSYNC(dynamicL) = (&aDynamic)->lastSyncL;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertDYNAMIC: dynamicL=L%u dynamicType=%d\n", dynamicL,
				DynamType(dynamicL));

aDynamicL = FirstSubLINK(dynamicL);
	for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL)) {
		/* Copy the subobj to a separate ADYNAMIC so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(DYNAMheap, aDynamicL, PADYNAMIC);
//LogPrintf(LOG_DEBUG, "->block=%ld aDynamicL=%d sizeof(ADYNAMIC)*aDynamicL=%d pSSubObj=%ld\n",
//(DYNAMheap)->block, aDynamicL, sizeof(ADYNAMIC)*aDynamicL, pSSubObj);
		BlockMove(pSSubObj, &tmpADynamic, sizeof(ADYNAMIC));

		Convert1DYNAMIC(doc, aDynamicL);
	}

	return True;
}


static Boolean ConvertGRAPHIC(Document * /* doc */, LINK graphicL)
{
	GRAPHIC_5 aGraphic;
	
	BlockMove(&tmpSuperObj, &aGraphic, sizeof(GRAPHIC_5));
	
	GraphicSTAFF(graphicL) = (&aGraphic)->staffn;		/* EXTOBJHEADER */

	GraphicSubType(graphicL) = (&aGraphic)->graphicType;
	GraphicVOICE(graphicL) = (&aGraphic)->voice;
	GraphicENCLOSURE(graphicL) = (&aGraphic)->enclosure;
	GraphicJUSTIFY(graphicL) = (&aGraphic)->justify;
	GraphicVCONSTRAIN(graphicL) = (&aGraphic)->vConstrain;
	GraphicHCONSTRAIN(graphicL) = (&aGraphic)->hConstrain;
	GraphicMULTILINE(graphicL) = (&aGraphic)->multiLine;
	GraphicINFO(graphicL) = (&aGraphic)->info;
	GraphicTHICKNESS(graphicL) = (&aGraphic)->gu.thickness;
	GraphicFONTIND(graphicL) = (&aGraphic)->fontInd;
	GraphicRELFSIZE(graphicL) = (&aGraphic)->relFSize;
	GraphicFONTSIZE(graphicL) = (&aGraphic)->fontSize;
	GraphicFONTSTYLE(graphicL) = (&aGraphic)->fontStyle;
	GraphicINFO2(graphicL) = (&aGraphic)->info2;
	GraphicFIRSTOBJ(graphicL) = (&aGraphic)->firstObj;
	GraphicLASTOBJ(graphicL) = (&aGraphic)->lastObj;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertGRAPHIC: graphicL=L%u staff=%d type=%d info=%d thick=%d\n",
			graphicL, GraphicSTAFF(graphicL), GraphicSubType(graphicL),
			GraphicINFO(graphicL), GraphicTHICKNESS(graphicL)); 

#define NoDEBUG_SKIPPING_LINKS
#ifdef DEBUG_SKIPPING_LINKS
	LINK aGraphicL = FirstSubLINK(graphicL);
	if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    ConvertGRAPHIC subobj: aGraphicL=%u\n", aGraphicL);
#endif

	return True;
}


static Boolean ConvertOTTAVA(Document * /* doc */, LINK ottavaL)      
{
	OTTAVA_5 aOttava;
	LINK aOttavaL;
	
	BlockMove(&tmpSuperObj, &aOttava, sizeof(OTTAVA_5));
	
	OttavaSTAFF(ottavaL) = (&aOttava)->staffn;		/* EXTOBJHEADER */

	OttavaNOCUTOFF(ottavaL) = (&aOttava)->noCutoff;
	OttavaCROSSSTAFF(ottavaL) = (&aOttava)->crossStaff;
	OttavaCROSSSYSTEM(ottavaL) = (&aOttava)->crossSystem;
	OttavaType(ottavaL) = (&aOttava)->octSignType;
	OttavaFILLER(ottavaL) = 0;
	OttavaNUMBERVIS(ottavaL) = (&aOttava)->numberVis;
	OttavaUNUSED1(ottavaL) = 0;
	OttavaBRACKVIS(ottavaL) = (&aOttava)->brackVis;
	OttavaUNUSED2(ottavaL) = 0;

	OttavaNXD(ottavaL) = (&aOttava)->nxd;
	OttavaNYD(ottavaL) = (&aOttava)->nyd;
	OttavaXDFIRST(ottavaL) = (&aOttava)->xdFirst;
	OttavaXDLAST(ottavaL) = (&aOttava)->xdLast;
	OttavaYDFIRST(ottavaL) = (&aOttava)->ydFirst;
	OttavaYDLAST(ottavaL) = (&aOttava)->ydLast;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertOTTAVA: ottavaL=L%u staff=%d type=%d nVis=%d xdFirst=%d xdLast=%d\n",
				ottavaL, OttavaSTAFF(ottavaL), OttavaType(ottavaL), OttavaNUMBERVIS(ottavaL),
				OttavaXDFIRST(ottavaL), OttavaXDLAST(ottavaL)); 

	aOttavaL = FirstSubLINK(ottavaL);
	for ( ; aOttavaL; aOttavaL = NextNOTEOTTAVAL(aOttavaL)) {
#ifdef DEBUG_SKIPPING_LINKS
if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    ConvertOTTAVA subobj: aOttavaL=%u\n", aOttavaL);
#endif
	}

	return True;
}


static Boolean ConvertSLUR(Document *doc, LINK slurL)
{
	SLUR_5 aSlur;
	LINK aSlurL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aSlur, sizeof(SLUR_5));
	
//DebugConvCheckObjs(doc, slurL, "ConvertSLUR1");

	SlurSTAFF(slurL) = (&aSlur)->staffn;		/* EXTOBJHEADER */

	SlurVOICE(slurL) = (&aSlur)->voice;
	SlurPHILLER(slurL) = 0;
	SlurCrossSTAFF(slurL) = (&aSlur)->crossStaff;
	SlurCrossSTFBACK(slurL) = (&aSlur)->crossStfBack;			
	SlurCrossSYS(slurL) = (&aSlur)->crossSystem;
	SlurTempFLAG(slurL) = (&aSlur)->tempFlag;
	SlurUSED(slurL) = (&aSlur)->used;
	SlurTIE(slurL) = (&aSlur)->tie;

	SlurFIRSTSYNC(slurL) = (&aSlur)->firstSyncL;
	SlurLASTSYNC(slurL) = (&aSlur)->lastSyncL;

//DebugConvCheckObjs(doc, slurL, "ConvertSLUR2");

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertSLUR: slurL=L%u staff=%d voice=%d tie=%d\n", slurL,
				SlurSTAFF(slurL), SlurVOICE(slurL), SlurTIE(slurL)); 

	aSlurL = FirstSubLINK(slurL);
	for ( ; aSlurL; aSlurL = NextSLURL(aSlurL)) {
		/* Copy the subobj to a separate ASLUR so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(SLURheap, aSlurL, PASLUR);
//LogPrintf(LOG_DEBUG, "->block=%ld aSlurL=%d sizeof(ASLUR)*aSlurL=%d pSSubObj=%ld\n", (SLURheap)->block,
//aSlurL, sizeof(ASLUR)*aSlurL, pSSubObj);
		BlockMove(pSSubObj, &tmpASlur, sizeof(ASLUR));

		Convert1SLUR(doc, aSlurL);
	}

	return True;
}


static Boolean ConvertTUPLET(Document * /* doc */, LINK tupletL)
{
	TUPLET_5 aTuplet;
	LINK aTupletL;
	
	BlockMove(&tmpSuperObj, &aTuplet, sizeof(TUPLET_5));
	
	TupletSTAFF(tupletL) = (&aTuplet)->staffn;		/* EXTOBJHEADER */

	TupletACCNUM(tupletL) = (&aTuplet)->accNum;
	TupletACCDENOM(tupletL) = (&aTuplet)->accDenom;
	TupletVOICE(tupletL) = (&aTuplet)->voice;
	TupletNUMVIS(tupletL) = (&aTuplet)->numVis;
	TupletDENOMVIS(tupletL) = (&aTuplet)->denomVis;
	TupletBRACKVIS(tupletL) = (&aTuplet)->brackVis;
	TupletSMALL(tupletL) = (&aTuplet)->small;
	TupletFILLER(tupletL) = (&aTuplet)->filler;
	TupletACNXD(tupletL) = (&aTuplet)->acnxd;
	TupletACNYD(tupletL) = (&aTuplet)->acnyd;
	TupletXDFIRST(tupletL) = (&aTuplet)->xdFirst;
	TupletYDFIRST(tupletL) = (&aTuplet)->ydFirst;
	TupletXDLAST(tupletL) = (&aTuplet)->xdLast;
	TupletYDLAST(tupletL) = (&aTuplet)->ydLast;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertTUPLET: tupletL=L%u accNum=%d accDenom=%d staff=%d\n",
				tupletL, TupletACCNUM(tupletL), TupletACCDENOM(tupletL), TupletSTAFF(tupletL)); 

	aTupletL = FirstSubLINK(tupletL);
	for ( ; aTupletL; aTupletL = NextNOTETUPLEL(aTupletL)) {
#ifdef DEBUG_SKIPPING_LINKS
		if (SUBOBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "    ConvertTUPLET subobj: aTupletL=%u\n", aTupletL);
#endif
	}

	return True;
}


static Boolean ConvertGRSYNC(Document *doc, LINK grSyncL)
{
	GRSYNC_5 aGRSync;
	LINK aGRNoteL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aGRSync, sizeof(GRSYNC_5));
	
	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertGRSYNC: grSyncL=L%u\n", grSyncL); 

	aGRNoteL = FirstSubLINK(grSyncL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		/* Copy the subobj to a separate AGRNOTE so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(GRNOTEheap, aGRNoteL, PAGRNOTE);
//LogPrintf(LOG_DEBUG, "->block=%ld aGRNoteL=%d sizeof(AGRNOTE)*aGRNoteL=%d pSSubObj=%ld\n", (NOTEheap)->block,
//aGRNoteL, sizeof(AGRNOTE)*aGRNoteL, pSSubObj);
		BlockMove(pSSubObj, &tmpAGRNote, sizeof(AGRNOTE));

		Convert1GRNOTE(doc, aGRNoteL);
	}

	return True;
}


static Boolean ConvertTEMPO(Document * /* doc */, LINK tempoL)
{
	TEMPO_5 aTempo;
	
	BlockMove(&tmpSuperObj, &aTempo, sizeof(TEMPO_5));
	
	TempoSTAFF(tempoL) = (&aTempo)->staffn;		/* EXTOBJHEADER */

	TempoSUBTYPE(tempoL) = (&aTempo)->subType;
	TempoEXPANDED(tempoL) = (&aTempo)->expanded;
	TempoNOMM(tempoL) = (&aTempo)->noMM;
	TempoFILLER(tempoL) = (&aTempo)->filler;
	TempoDOTTED(tempoL) = (&aTempo)->dotted;
	TempoHIDEMM(tempoL) = (&aTempo)->hideMM;
	TempoMM(tempoL) = (&aTempo)->tempoMM;
	TempoSTRING(tempoL) = (&aTempo)->strOffset;
	TempoFIRSTOBJ(tempoL) = (&aTempo)->firstObjL;
	TempoMETROSTR(tempoL) = (&aTempo)->metroStrOffset;
	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertTEMPO: tempoL=L%u tempoType=%d\n", tempoL, DynamType(tempoL));

	return True;
}


static Boolean ConvertSPACER(Document * /* doc */, LINK spacerL)
{
	SPACER_5 aSpacer;
	
	BlockMove(&tmpSuperObj, &aSpacer, sizeof(SPACER_5));
	
	SpacerSTAFF(spacerL) = (&aSpacer)->staffn;		/* EXTOBJHEADER */

	SpacerBOTSTAFF(spacerL) = (&aSpacer)->bottomStaff;
	SpacerSPWIDTH(spacerL) = (&aSpacer)->spWidth;
	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertSPACER: spacerL=L%u bottomStaff=%d spWidth=%d\n", spacerL,
				SpacerBOTSTAFF(spacerL), SpacerSPWIDTH(spacerL));

	return True;
}


static Boolean ConvertENDING(Document * /* doc */, LINK endingL)
{
	ENDING_5 aEnding;
	
	BlockMove(&tmpSuperObj, &aEnding, sizeof(ENDING_5));
	
	EndingSTAFF(endingL) = (&aEnding)->staffn;		/* EXTOBJHEADER */

	EndingFIRSTOBJ(endingL) = (&aEnding)->firstObjL;
	EndingLASTOBJ(endingL) = (&aEnding)->lastObjL;
	EndingNoLCUTOFF(endingL) = (&aEnding)->noLCutoff;
	EndingNoRCUTOFF(endingL) = (&aEnding)->noRCutoff;
	EndingENDNUM(endingL) = (&aEnding)->endNum;
	EndingENDXD(endingL) = (&aEnding)->endxd;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertENDING: endingL=L%u staff=%d endNum=%d endxd=%d\n",
				endingL, EndingSTAFF(endingL), EndingENDNUM(endingL), EndingENDXD(endingL)); 

	return True;
}


static Boolean ConvertPSMEAS(Document *doc, LINK psMeasL)
{
	PSMEAS_5 aPsMeas;
	LINK aPsMeasL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aPsMeas, sizeof(PSMEAS_5));
	
	PSMeasFILLER(psMeasL) = 0;

	if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertPSMEAS: psMeasL=L%u\n", psMeasL); 

	aPsMeasL = FirstSubLINK(psMeasL);
	for ( ; aPsMeasL; aPsMeasL = NextPSMEASL(aPsMeasL)) {
		/* Copy the subobj to a separate APSMEAS so we can move fields all over the place
		   without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(PSMEASheap, aPsMeasL, PAPSMEAS);
//LogPrintf(LOG_DEBUG, "->block=%ld aPsMeasL=%d sizeof(APSMEAS)*aPsMeasL=%d pSSubObj=%ld\n", (PSMEASheap)->block,
//aPsMeasL, sizeof(APSMEAS)*aPsMeasL, pSSubObj);
		BlockMove(pSSubObj, &tmpAPSMeas, sizeof(APSMEAS));

		Convert1PSMEAS(doc, aPsMeasL);
	}

	return True;
}


/* ----------------------------------------------------------------- ConvertObjectList -- */

/* Convert the headers and bodies of objects in either the main or the Master Page
object list read from 'N105' format files to the current structure for that type of
object. Any file-format-conversion code that doesn't affect the length of the header
or lengths or offsets of fields in objects (or subobjects) should go here. ??HOW ABOUT
SUBOBJECTS? Return True if all goes well, False if not.

This function assumes that the headers and the entire object list have been read; all
object and subobject links are valid; and all objects and subobjects are the correct
lengths. Tweaks that affect lengths or offsets (1) to the headers should be done before
calling this, in OpenFile(); (2) to objects or subobjects, before calling it, in
ReadHeaps(). */

#if SUBOBJ_DETAIL_SHOW || OBJ_DETAIL_SHOW
#define DEBUG_LOOP 2
#else
#define DEBUG_LOOP 0
#endif

// FIXME: REMOVE GetPSUPEROBJECT() & CHANGE ALL TO GetPSUPEROBJ()!
#define GetPSUPEROBJECT(link)	(PSUPEROBJECT)GetObjectPtr(OBJheap, link, PSUPEROBJECT)

Boolean ConvertObjectList(Document *doc, unsigned long version, long /* fileTime */,
			Boolean doMasterList)
{
	HEAP *objHeap;
	LINK objL, startL, prevL;
	unsigned char *pSObj;
	short objCount;

	if (version!='N105') {
		AlwaysErrMsg("Can't convert file of any version but 'N105'.  (ConvertObjectList)");
		return False;
	}
	
	objHeap = doc->Heap + OBJtype;	
	startL = (doMasterList?  doc->masterHeadL :  doc->headL);

	InitDebugConvert();

	/* If we're debugging file conversion, sidestep the disappearing-message bug in the
	   OS 10.5 Console utility by adding a delay before each message. */

	if (debugLevel[DBG_CONVERT]!=0) KludgeOS10p5LogDelay(True);
	fflush(stdout);
	objCount = 0;
	prevL = startL-1;
	for (objL = startL; objL; objL = RightLINK(objL)) {
		objCount++;
		if (debugLevel) {
			if (debugLevel[DBG_CONVERT]!=0>1 || objCount%50==1) LogPrintf(LOG_DEBUG,
					"**************** ConvertObjectList: objL=%u prevL=%u type='%s'\n",
					objL, prevL, NameObjType(objL));
		}

		/* If this function is called in the process of opening a file, the only situation
		   where it should be, consecutive objects must have sequential links; check that. */
		 
		if (objL!=prevL+1) MayErrMsg("PROGRAM ERROR: objL=%ld BUT prevL=%ld INSTEAD OF %ld!  (ConvertObjectList)",
									(long)objL, (long)prevL, (long)prevL-1);
		prevL = objL;

		/* Copy the object to a separate SUPEROBJECT so we can move fields all over the
		   place without having to worry about clobbering anything. */
		   
		pSObj = (unsigned char *)GetPSUPEROBJECT(objL);
		BlockMove(pSObj, &tmpSuperObj, sizeof(SUPEROBJECT));

		/* Convert the object header now so type-specific functions don't have to. */
		
		ConvertObjHeader(doc, objL);
#ifdef DEBUG_EMPTY_OBJRECT
if (debugLevel[DBG_CONVERT]!=0) DebugConvCheckObjs(doc, objL, "ConvertObjectList");
#endif
		switch (ObjLType(objL)) {
			case HEADERtype:
				ConvertHEADER(doc, objL);
				continue;
			case TAILtype:
				/* TAIL objects have no subobjs & no fields but header, so there's nothing
				   to do except to stop: we're at the end of this object list. */
				   
				if (OBJ_DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ConvertObjectList: objL=%u is TAIL.\n",
										objL);
				break;
			case SYNCtype:
				ConvertSYNC(doc, objL);
				continue;
			case RPTENDtype:
				ConvertRPTEND(doc, objL);
				continue;
			case PAGEtype:
				ConvertPAGE(doc, objL);
				continue;
			case SYSTEMtype:
				ConvertSYSTEM(doc, objL);
				continue;
			case STAFFtype:
				ConvertSTAFF(doc, objL);
				continue;
			case MEASUREtype:
				ConvertMEASURE(doc, objL);
				continue;
			case CLEFtype:
				ConvertCLEF(doc, objL);
				continue;
			case KEYSIGtype:
				ConvertKEYSIG(doc, objL);
				continue;
			case TIMESIGtype:
				ConvertTIMESIG(doc, objL);
				continue;
			case BEAMSETtype:
				ConvertBEAMSET(doc, objL);
				continue;
			case CONNECTtype:
				ConvertCONNECT(doc, objL);
				continue;
			case DYNAMtype:
				ConvertDYNAMIC(doc, objL);
				continue;
			case GRAPHICtype:
				ConvertGRAPHIC(doc, objL);
				continue;
			case OTTAVAtype:
				ConvertOTTAVA(doc, objL);
				continue;
			case SLURtype:
				ConvertSLUR(doc, objL);
				continue;
			case TUPLETtype:
				ConvertTUPLET(doc, objL);
				continue;
			case GRSYNCtype:
				ConvertGRSYNC(doc, objL);
				continue;
			case TEMPOtype:
				ConvertTEMPO(doc, objL);
				continue;
			case SPACERtype:
				ConvertSPACER(doc, objL);
				continue;
			case ENDINGtype:
				ConvertENDING(doc, objL);
				continue;
			case PSMEAStype:
				ConvertPSMEAS(doc, objL);
				continue;
			default:
				MayErrMsg("PROGRAM ERROR: OBJECT L%ld TYPE %ld IS ILLEGAL.  (ConvertObjectList)",
							(long)objL, (long)ObjLType(objL));
		}

		DebugConvertObj(doc, objL);
	}

	/* Make sure all staves are visible in Master Page. They should never be invisible,
	   but long ago (v.997), they sometimes were, probably because not exporting changes to
	   Master Page was implemented by reconstructing it from the 1st system of the score.
	   That was fixed in about .998a10, so it should certainly be safe to remove this
	   call! But the thought makes me nervous, and making them visible here really
	   shouldn't cause any problems (and, as far as I know, it never has). */

	VisifyMasterStaves(doc);
	
if (debugLevel[DBG_CONVERT]!=0) DebugConvCheckObjs(doc, doc->tailL, "ConvertObjectList");

	return True;
}


/* ----------------------------------------------------------------------- ModifyScore -- */
/* Any temporary file-content-updating code (a.k.a. hacking) that doesn't affect the
length of the header or lengths of objects should go here. This function should only be
called after the header and entire object list have been read. Return True if all goes
well, False if not.

FIXME: If code here considers changing something, and especially if it ends up actually
doing so, it should both alert the user and call LogPrintf to display at least one
prominent message in the console window! It should perhaps also set doc->changed, though
that will make it easier for people to accidentally overwrite the original version.

NB2: Be sure that all of this code is removed or commented out in ordinary versions!
To facilitate that, when done with hacking, add an "#error" line; cf. examples
below. */

#ifdef SWAP_STAVES
#error ATTEMPTED TO COMPILE OLD HACKING CODE for ModifyScore!
static void ShowStfTops(Document *doc, LINK pL, short staffN1, short staffN2);
static void ShowStfTops(Document *doc, LINK pL, short staffN1, short staffN2)
{
	CONTEXT context;
	short staffTop1, staffTop2;
	
	pL = SSearch(doc->headL, STAFFtype, False);
	GetContext(doc, pL, staffN1, &context);
	staffTop1 =  context.staffTop;
	GetContext(doc, pL, staffN2, &context);
	staffTop2 =  context.staffTop;
	LogPrintf(LOG_INFO, "ShowStfTops(%d): staffTop1=%d, staffTop2=%d\n", pL, staffTop1, staffTop2);
}

static void SwapStaves(Document *doc, LINK pL, short staffN1, short staffN2);
static void SwapStaves(Document *doc, LINK pL, short staffN1, short staffN2)
{
	LINK aStaffL, aMeasureL, aPSMeasL, aClefL, aKeySigL, aTimeSigL, aNoteL, aTupletL,
			aRptEndL, aDynamicL, aGRNoteL;
	CONTEXT context;
	short staffTop1, staffTop2;

	switch (ObjLType(pL)) {
		case HEADERtype:
			break;

		case PAGEtype:
			break;

		case SYSTEMtype:
			break;

		case STAFFtype:
LogPrintf(LOG_INFO, "  Staff L%d\n", pL);
			GetContext(doc, pL, staffN1, &context);
			staffTop1 =  context.staffTop;
			GetContext(doc, pL, staffN2, &context);
			staffTop2 =  context.staffTop;
LogPrintf(LOG_INFO, "    staffTop1, 2=%d, %d\n", staffTop1, staffTop2);
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				if (StaffSTAFF(aStaffL)==staffN1) {
					StaffSTAFF(aStaffL) = staffN2;
					StaffTOP(aStaffL) = staffTop2;
				}
				else if (StaffSTAFF(aStaffL)==staffN2) {
					StaffSTAFF(aStaffL) = staffN1;
					StaffTOP(aStaffL) = staffTop1;
				}
//GetContext(doc, pL, staffN1, &context);
//LogPrintf(LOG_INFO, "(1)    pL=%d staffTop1=%d\n", pL, staffTop1);
			}
//GetContext(doc, pL, staffN1, &context);
//LogPrintf(LOG_INFO, "(2)    pL=%d staffTop1=%d\n", pL, staffTop1);
//ShowStfTops(doc, pL, staffN1, staffN2);
			break;

		case CONNECTtype:
			break;

		case MEASUREtype:
LogPrintf(LOG_INFO, "  Measure L%d\n", pL);
			aMeasureL = FirstSubLINK(pL);
			for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
				if (MeasureSTAFF(aMeasureL)==staffN1) MeasureSTAFF(aMeasureL) = staffN2;
				else if (MeasureSTAFF(aMeasureL)==staffN2) MeasureSTAFF(aMeasureL) = staffN1;
			}
			break;

		case PSMEAStype:
LogPrintf(LOG_INFO, "  PSMeas L%d\n", pL);
			aPSMeasL = FirstSubLINK(pL);
			for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL)) {
				if (PSMeasSTAFF(aPSMeasL)==staffN1) PSMeasSTAFF(aPSMeasL) = staffN2;
				else if (PSMeasSTAFF(aPSMeasL)==staffN2) PSMeasSTAFF(aPSMeasL) = staffN1;
			}
			break;

		case CLEFtype:
LogPrintf(LOG_INFO, "  Clef L%d\n", pL);
			aClefL = FirstSubLINK(pL);
			for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
				if (ClefSTAFF(aClefL)==staffN1) ClefSTAFF(aClefL) = staffN2;
				else if (ClefSTAFF(aClefL)==staffN2) ClefSTAFF(aClefL) = staffN1;
			}
			break;

		case KEYSIGtype:
LogPrintf(LOG_INFO, "  Keysig L%d\n", pL);
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
				if (KeySigSTAFF(aKeySigL)==staffN1) KeySigSTAFF(aKeySigL) = staffN2;
				else if (KeySigSTAFF(aKeySigL)==staffN2) KeySigSTAFF(aKeySigL) = staffN1;
			}
			break;

		case TIMESIGtype:
LogPrintf(LOG_INFO, "  TimeSig L%d\n", pL);
			aTimeSigL = FirstSubLINK(pL);
			for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
				if (TimeSigSTAFF(aTimeSigL)==staffN1) TimeSigSTAFF(aTimeSigL) = staffN2;
				else if (TimeSigSTAFF(aTimeSigL)==staffN2) TimeSigSTAFF(aTimeSigL) = staffN1;
			}
			break;

		case SYNCtype:
LogPrintf(LOG_INFO, "  Sync L%d\n", pL);
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				if (NoteSTAFF(aNoteL)==staffN1) {
					NoteSTAFF(aNoteL) = staffN2;
					NoteVOICE(aNoteL) = staffN2;		/* Assumes only 1 voice on the staff */
				}
				else if (NoteSTAFF(aNoteL)==staffN2) {
					NoteSTAFF(aNoteL) = staffN1;
					NoteVOICE(aNoteL) = staffN1;		/* Assumes only 1 voice on the staff */
				}
			}
			break;

		case BEAMSETtype:
LogPrintf(LOG_INFO, "  Beamset L%d\n", pL);
			if (BeamSTAFF(pL)==staffN1) BeamSTAFF((pL)) = staffN2;
			else if (BeamSTAFF((pL))==staffN2) BeamSTAFF((pL)) = staffN1;
			break;

		case TUPLETtype:
LogPrintf(LOG_INFO, "  Tuplet L%d\n", pL);
			if (TupletSTAFF(pL)==staffN1) TupletSTAFF((pL)) = staffN2;
			else if (TupletSTAFF((pL))==staffN2) TupletSTAFF((pL)) = staffN1;
			break;

/*
		case RPTENDtype:
			?? = FirstSubLINK(pL);
			for (??) {
			}
			break;

		case ENDINGtype:
			?!
			break;
*/
		case DYNAMtype:
LogPrintf(LOG_INFO, "  Dynamic L%d\n", pL);
			aDynamicL = FirstSubLINK(pL);
			for ( ; aDynamicL; aDynamicL=NextDYNAMICL(aDynamicL)) {
				if (DynamicSTAFF(aDynamicL)==staffN1) DynamicSTAFF(aDynamicL) = staffN2;
				else if (DynamicSTAFF(aDynamicL)==staffN2) DynamicSTAFF(aDynamicL) = staffN1;
			}
			break;

		case GRAPHICtype:
LogPrintf(LOG_INFO, "  Graphic L%d\n", pL);
			if (GraphicSTAFF(pL)==staffN1) GraphicSTAFF((pL)) = staffN2;
			else if (GraphicSTAFF((pL))==staffN2) GraphicSTAFF((pL)) = staffN1;
			break;

/*
		case OTTAVAtype:
			?!
			break;
*/
		case SLURtype:
LogPrintf(LOG_INFO, "  Slur L%d\n", pL);
			if (SlurSTAFF(pL)==staffN1) SlurSTAFF((pL)) = staffN2;
			else if (SlurSTAFF((pL))==staffN2) SlurSTAFF((pL)) = staffN1;
			break;

/*
		case GRSYNCtype:
			?? = FirstSubLINK(pL);
			for (??) {
			}
			break;

		case TEMPOtype:
			?!
			break;

		case SPACERtype:
			?!
			break;
*/

		default:
			break;	
	}
}
#endif


Boolean ModifyScore(Document * /*doc*/, long /*fileTime*/)
{
#ifdef SWAP_STAVES
#error ModifyScore: ATTEMPTED TO COMPILE OLD HACKING CODE!
	/* DAB carelessly put a lot of time into orchestrating his violin concerto with a
		template having the trumpet staff above the horn; this was intended to correct
		that.
		NB: To swap two staves, in addition to running this code, use Master Page to:
			1. Fix the staves' vertical positions
			2. If the staves are Grouped, un-Group and re-Group
			3. Swap the Instrument info
		NB2: If there's more than one voice on either of the staves, this is not
		likely to work at all well. In any case, it never worked well enough to be
		useful for the purpose I intended it for.
																--DAB, Jan. 2016 */
	
	short staffN1 = 5, staffN2 = 6;
	LINK pL;
	
	SysBeep(1);
	LogPrintf(LOG_NOTICE, "ModifyScore: SWAPPING STAVES %d AND %d (of %d) IN MASTER PAGE....\n",
				staffN1, staffN2, doc->nstaves);
	for (pL = doc->masterHeadL; pL!=doc->masterTailL; pL = RightLINK(pL)) {
		SwapStaves(doc, pL, staffN1, staffN2);
	}
	LogPrintf(LOG_NOTICE, "ModifyScore: SWAPPING STAVES %d AND %d (of %d) IN SCORE OBJECT LIST....\n",
				staffN1, staffN2, doc->nstaves);
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		SwapStaves(doc, pL, staffN1, staffN2);
	}
  	doc->changed = True;

#endif

#ifdef FIX_MASTERPAGE_SYSRECT
#error ModifyScore: ATTEMPTED TO COMPILE OLD FILE-HACKING CODE!
	/* This is to fix a score David Gottlieb is working on, in which Ngale draws
	 * _completely_ blank pages. Nov. 1999.
	 */
	 
	LINK sysL, staffL, aStaffL; DDIST topStaffTop; PASTAFF aStaff;

	LogPrintf(LOG_NOTICE, "ModifyScore: fixing Master Page sysRects and staffTops...\n");
	// Browser(doc,doc->masterHeadL, doc->masterTailL);
	sysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
	SystemRECT(sysL).bottom = SystemRECT(sysL).top+pt2d(72);

	staffL = SSearch(doc->masterHeadL, STAFFtype, False);
	aStaffL = FirstSubLINK(staffL);
	topStaffTop = pt2d(24);
	/* The following assumes staff subobjects are in order from top to bottom! */
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		aStaff->staffTop = topStaffTop;
		topStaffTop += pt2d(36);
	}
  	doc->changed = True;
#endif

#ifdef FIX_UNBEAMED_FLAGS_AUGDOT_PROB
#error ModifyScore: ATTEMPTED TO COMPILE OLD FILE-HACKING CODE!
	short alteredCount; LINK pL;

  /* From SetupNote in Objects.c:
   * "On upstemmed notes with flags, xMoveDots should really 
   * be 2 or 3 larger [than normal], but then it should change whenever 
   * such notes are beamed or unbeamed or their stem direction or 
   * duration changes." */

  /* TC Oct 7 1999 (modified by DAB, 8 Oct.): quick and dirty hack to fix this problem
   * for Bill Hunt; on opening file, look at all unbeamed dotted notes with subtype >
   * quarter note and if they have stem up, increase xMoveDots by 2; if we find any
   * such notes, set doc-changed to True then simply report to user. It'd probably
   * be better to consider stem length and not do this for notes with very long
   * stems, but we don't consider that. */

  LogPrintf(LOG_NOTICE, "ModifyScore: fixing augdot positions for unbeamed upstemmed notes with flags...\n");
  alteredCount = 0;
  for (pL = doc->headL; pL; pL = RightLINK(pL)) 
    if (ObjLType(pL)==SYNCtype) {
      LINK aNoteL = FirstSubLINK(pL);
      for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
        PANOTE aNote = GetPANOTE(aNoteL);
        if((aNote->subType > QTR_L_DUR) && (aNote->ndots))
          if(!aNote->beamed)
            if(MainNote(aNoteL) && (aNote->yd > aNote->ystem)) {
              aNote->xMoveDots = 3 + WIDEHEAD(aNote->subType) + 2;
              alteredCount++;
            }
      }
    }
  
  if (alteredCount) {
  	doc->changed = True;

    NumToString((long)alteredCount,(unsigned char *)strBuf);
    ParamText((unsigned char *)strBuf, 
      (unsigned char *)"\p unbeamed notes with stems up found; aug-dots corrected. NB Save the file with a new name!", 
        (unsigned char *)"", (unsigned char *)"");
    CautionInform(GENERIC2_ALRT);
  }
#endif

#ifdef FIX_BLACK_SCORE
#error ModifyScore: ATTEMPTED TO COMPILE OLD FILE-HACKING CODE!
	/* Sample trashed-file-fixing hack: in this case, to fix an Arnie Black score. */
	
	for (pL = doc->headL; pL; pL = RightLINK(pL)) 
		if (pL>=492 && ObjLType(pL)==SYNCtype) {
			LINK aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				NoteBEAMED(aNoteL) = False;
		}
#endif

	return True;
}
