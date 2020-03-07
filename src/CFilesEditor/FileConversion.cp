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

static void ConvertFontTable(Document *doc, DocumentN105 *docN105)
{
	short i;
	
	for (i = 0; i<doc->nfontsUsed; i++)
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
	doc->feedback = docN105->feedback;
	doc->dontSendPatches = docN105->dontSendPatches;
	doc->saved = docN105->saved;
	doc->named = docN105->named;
	doc->used = docN105->used;
	doc->transposed = docN105->transposed;
	doc->lyricText = docN105->lyricText;
	doc->polyTimbral = docN105->polyTimbral;
	doc->currentPage = docN105->currentPage;
	doc->spacePercent = docN105->spacePercent;
	doc->srastral = docN105->srastral;
	doc->altsrastral = docN105->altsrastral;
	doc->tempo = docN105->tempo;
	doc->channel = docN105->channel;
	doc->velocity = docN105->velocity;
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
	doc->pianoroll = docN105->pianoroll;
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
		/* struct assignment works, e.g., in CopyToClip(); here, it gives an
		   incomprehensible compiler error message! So copy individual fields. */
		   
		doc->voiceTab[v].partn = docN105->voiceTab[v].partn;
		doc->voiceTab[v].voiceRole = docN105->voiceTab[v].voiceRole;
		doc->voiceTab[v].relVoice = docN105->voiceTab[v].relVoice;
	}
		
	for (j = 0; j<256-(MAXVOICES+1); j++)
		doc->expansion[j] = 0;	
}

/* ------------------------------------------------------------------ HeapFixN105Links -- */

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
is a specialized version of HeapFixLinks() to fix links in 'N105' format files when
they're opened, before the contents of objects are converted. NB: This code assumes
that headL is always at LINK 1. */

short HeapFixN105Links(Document *doc)
{
	LINK 	pL, prevPage, prevSystem, prevStaff, prevMeasure;
	Boolean tailFound=False;
	
	prevPage = prevSystem = prevStaff = prevMeasure = NILINK;

{	unsigned char *pSObj;
#define GetPSUPEROBJECT(link)	(PSUPEROBJECT)GetObjectPtr(OBJheap, link, PSUPEROBJECT)
pSObj = (unsigned char *)GetPSUPEROBJECT(1);
NHexDump(LOG_DEBUG, "HeapFixLinks1 L1", pSObj, 46, 4, 16);
//pSObj = (unsigned char *)GetPSUPEROBJECT(2);
//NHexDump(LOG_DEBUG, "HeapFixLinks1 L2", pSObj, 46, 4, 16);
//pSObj = (unsigned char *)GetPSUPEROBJECT(3);
//NHexDump(LOG_DEBUG, "HeapFixLinks1 L3", pSObj, 46, 4, 16);
pSObj = (unsigned char *)GetPSUPEROBJECT(4);
NHexDump(LOG_DEBUG, "HeapFixLinks1 L4", pSObj, 46, 4, 16);
}

	/* First handle the main object list. */

	FIX_END(doc->headL);
	for (pL = doc->headL; !tailFound; pL = DRightLINK(doc, pL)) {
		FIX_END(DRightLINK(doc, pL));
		switch(DObjLType(doc, pL)) {
			case TAILtype:
				doc->tailL = pL;
				if (!doc->masterHeadL) goto Error;
				doc->masterHeadL = pL+1;
				tailFound = True;
				DRightLINK(doc, doc->tailL) = NILINK;
				break;
			case PAGEtype:
				DLinkLPAGE_5(doc, pL) = prevPage;
				if (prevPage) DLinkRPAGE_5(doc, prevPage) = pL;
				DLinkRPAGE_5(doc, pL) = NILINK;
				prevPage = pL;
				break;
			case SYSTEMtype:
				DSysPAGE_5(doc, pL) = prevPage;
				DLinkLSYS_5(doc, pL) = prevSystem;
				if (prevSystem) DLinkRSYS_5(doc, prevSystem) = pL;
				prevSystem = pL;
				break;
			case STAFFtype:
				DStaffSYS_5(doc, pL) = prevSystem;
				DLinkLSTAFF_5(doc, pL) = prevStaff;
				if (prevStaff) DLinkRSTAFF_5(doc, prevStaff) = pL;
				prevStaff = pL;
				break;
			case MEASUREtype:
				DMeasSYSL_5(doc, pL) = prevSystem;
				DMeasSTAFFL_5(doc, pL) = prevStaff;
				DLinkLMEAS_5(doc, pL) = prevMeasure;
				if (prevMeasure) DLinkRMEAS_5(doc, prevMeasure) = pL;
				prevMeasure = pL;
				break;
			case SLURtype:
				break;
			default:
				break;
		}
	}

{	unsigned char *pSObj;
#define GetPSUPEROBJECT(link)	(PSUPEROBJECT)GetObjectPtr(OBJheap, link, PSUPEROBJECT)
//pSObj = (unsigned char *)GetPSUPEROBJECT(1);
//NHexDump(LOG_DEBUG, "HeapFixLinks2 L1", pSObj, 46, 4, 16);
//pSObj = (unsigned char *)GetPSUPEROBJECT(2);
//NHexDump(LOG_DEBUG, "HeapFixLinks2 L2", pSObj, 46, 4, 16);
//pSObj = (unsigned char *)GetPSUPEROBJECT(3);
//NHexDump(LOG_DEBUG, "HeapFixLinks2 L3", pSObj, 46, 4, 16);
pSObj = (unsigned char *)GetPSUPEROBJECT(4);
NHexDump(LOG_DEBUG, "HeapFixLinks2 L4", pSObj, 46, 4, 16);
}
	prevPage = prevSystem = prevStaff = prevMeasure = NILINK;

	/* Now do the Master Page list. */

	for (pL = doc->masterHeadL; pL; pL = DRightLINK(doc, pL))
		switch(DObjLType(doc, pL)) {
			case HEADERtype:
				DLeftLINK(doc, doc->masterHeadL) = NILINK;
				break;
			case TAILtype:
				doc->masterTailL = pL;
				DRightLINK(doc, doc->masterTailL) = NILINK;
				return 0;
			case PAGEtype:
				DLinkLPAGE_5(doc, pL) = prevPage;
				if (prevPage) DLinkRPAGE_5(doc, prevPage) = pL;
				DLinkRPAGE_5(doc, pL) = NILINK;
				prevPage = pL;
				break;
			case SYSTEMtype:
				DSysPAGE_5(doc, pL) = prevPage;
				DLinkLSYS_5(doc, pL) = prevSystem;
				if (prevSystem) DLinkRSYS_5(doc, prevSystem) = pL;
				prevSystem = pL;
				break;
			case STAFFtype:
				DStaffSYS_5(doc, pL) = prevSystem;
				DLinkLSTAFF_5(doc, pL) = prevStaff;
				if (prevStaff) DLinkRSTAFF_5(doc, prevStaff) = pL;
				prevStaff = pL;
				break;
			case MEASUREtype:
				DMeasSYSL_5(doc, pL) = prevSystem;
				DMeasSTAFFL_5(doc, pL) = prevStaff;
				DLinkLMEAS_5(doc, pL) = prevMeasure;
				if (prevMeasure) DLinkRMEAS_5(doc, prevMeasure) = pL;
				prevMeasure = pL;
				break;
			default:
				break;
		}
		
	/* If we reach here, something is wrong: drop through. */

Error:
	/* In case we never got into the Master Page loop or it didn't have a TAIL obj. */
	
	AlwaysErrMsg("Can't set links in memory for the file!  (HeapFixN105Links)");
	doc->masterTailL = NILINK;
	return FIX_LINKS_ERR;
}


/* -------------------------------- Convert the content of objects, including headers -- */

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


/* ---------------------------- Convert content of subobjects, including their headers -- */

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
AGRNOTE tmpAGRNote;

static Boolean Convert1NOTER(Document *doc, LINK aNoteRL);
static Boolean Convert1STAFF(Document *doc, LINK aStaffL);
static Boolean Convert1MEASURE(Document *doc, LINK aMeasureL);
static Boolean Convert1KEYSIG(Document *doc, LINK aKeySigL);
static Boolean Convert1TIMESIG(Document *doc, LINK aTimeSigL);
static Boolean Convert1CONNECT(Document *doc, LINK aConnectL);
static Boolean Convert1DYNAMIC(Document *doc, LINK aDynamicL);
static Boolean Convert1GRNOTE(Document *doc, LINK aGRNoteRL);


static Boolean Convert1NOTER(Document * /* doc */, LINK aNoteRL)
{
	ANOTE_5 a1NoteR;
	
	BlockMove(&tmpANoteR, &a1NoteR, sizeof(ANOTE_5));
	
	/* The first three fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	NoteSEL(aNoteRL) = (&a1NoteR)->selected;
	NoteVIS(aNoteRL) = (&a1NoteR)->visible;
	NoteSOFT(aNoteRL) = (&a1NoteR)->soft;

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
	
//NHexDump(LOG_DEBUG, "Convert1NOTER", (unsigned char *)&tempSys, 46, 4, 16);
LogPrintf(LOG_DEBUG, "Convert1NOTER: aNoteRL=%u voice=%d yqpit=%d xd=%d yd=%d playDur=%d\n",
aNoteRL, NoteVOICE(aNoteRL), NoteYQPIT(aNoteRL), NoteXD(aNoteRL), NoteYD(aNoteRL), NotePLAYDUR(aNoteRL));
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
	
//NHexDump(LOG_DEBUG, "Convert1RPTEND", (unsigned char *)&tempSys, 46, 4, 16);
LogPrintf(LOG_DEBUG, "Convert1RPTEND: aRptEndL=%u connAbove=%d connStaff=%d\n",
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
	// ??NEED TO HANDLE WHOLE_KSINFO!!!!! */
	StaffTIMESIGTYPE(aStaffL) = (&a1Staff)->timeSigType;
	StaffNUMER(aStaffL) = (&a1Staff)->numerator;
	StaffDENOM(aStaffL) = (&a1Staff)->denominator;
	StaffFILLER(aStaffL) = 0;
	StaffSHOWLEDGERS(aStaffL) = (&a1Staff)->showLedgers;
	StaffSHOWLINES(aStaffL) = (&a1Staff)->showLines;

//NHexDump(LOG_DEBUG, "Convert1STAFF", (unsigned char *)&tempSys, 46, 4, 16);
//LogPrintf(LOG_DEBUG, "Convert1STAFF: aStaffL=%u staffn=%d staffTop=%d staffHeight=%d staffLines=%d\n",
//aStaffL, StaffSTAFFN(aStaffL), StaffTOP(aStaffL), StaffHEIGHT(aStaffL), StaffSTAFFLINES(aStaffL));
{ PASTAFF			aStaff;
aStaff = GetPASTAFF(aStaffL);
LogPrintf(LOG_INFO, "Convert1STAFF: st=%d top,left,ht,rt=d%d,%d,%d,%d lines=%d fontSz=%d %c%c TS=%d,%d/%d\n",
	aStaff->staffn, aStaff->staffTop,
	aStaff->staffLeft, aStaff->staffHeight,
	aStaff->staffRight, aStaff->staffLines,
	aStaff->fontSize,
	(aStaff->selected? 'S' : '.') ,
	(aStaff->visible? 'V' : '.'),
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
	MeasOLDFAKEMEAS(aMeasureL) = (&a1Measure)->oldFakeMeas,
	MeasMEASURENUM(aMeasureL) = (&a1Measure)->measureNum;
	MeasMRECT(aMeasureL) = (&a1Measure)->measSizeRect;
	MeasCONNSTAFF(aMeasureL) = (&a1Measure)->connStaff;
	MeasCLEFTYPE(aMeasureL) = (&a1Measure)->clefType;
	MeasDynamType(aMeasureL) = (&a1Measure)->dynamicType;
	// ??NEED TO HANDLE WHOLE_KSINFO!!!!! */
	MeasTIMESIGTYPE(aMeasureL) = (&a1Measure)->timeSigType;
	MeasNUMER(aMeasureL) = (&a1Measure)->numerator;
	MeasDENOM(aMeasureL) = (&a1Measure)->denominator;
	MeasXMNSTDOFFSET(aMeasureL) = (&a1Measure)->xMNStdOffset;
	MeasYMNSTDOFFSET(aMeasureL) = (&a1Measure)->yMNStdOffset;

//NHexDump(LOG_DEBUG, "Convert1MEASURE", (unsigned char *)&tempSys, 46, 4, 16);
//LogPrintf(LOG_DEBUG, "Convert1MEASURE: aMeasureL=%u staffn=%d staffTop=%d staffHeight=%d staffLines=%d\n",
//aMeasureL, MeasureMEASUREN(aMeasureL), MeasureTOP(aMeasureL), MeasureHEIGHT(aMeasureL), MeasureMEASURELINES(aMeasureL));
#if 0
{ PAMEASURE			aMeasure;
aMeasure = GetPAMEASURE(aMeasureL);
LogPrintf(LOG_INFO, "Convert1MEASURE: st=%d top,left,ht,rt=d%d,%d,%d,%d lines=%d fontSz=%d %c%c TS=%d,%d/%d\n",
blah lorem ipsum );
}
#endif
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

//NHexDump(LOG_DEBUG, "Convert1CLEF", (unsigned char *)&tempSys, 46, 4, 16);
LogPrintf(LOG_DEBUG, "Convert1CLEF: aClefL=%u xd=%d\n", aClefL, ClefXD(aClefL));
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
	
	KeySigFILLER1(aKeySigL) = 0;
	KeySigSMALL(aKeySigL) = (&a1KeySig)->small;
	KeySigFILLER2(aKeySigL) = 0;
	KeySigXD(aKeySigL) = (&a1KeySig)->xd;

//NHexDump(LOG_DEBUG, "Convert1KEYSIG", (unsigned char *)&tempSys, 46, 4, 16);
LogPrintf(LOG_DEBUG, "Convert1KEYSIG: aKeySigL=%u small=%d xd=%d\n", aKeySigL,
KeySigSMALL(aKeySigL), KeySigXD(aKeySigL));
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

//NHexDump(LOG_DEBUG, "Convert1TIMESIG", (unsigned char *)&tempSys, 46, 4, 16);
LogPrintf(LOG_DEBUG, "Convert1TIMESIG: aTimeSigL=%u numer=%d denom=%d\n", aTimeSigL,
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

//NHexDump(LOG_DEBUG, "Convert1BEAMSET", (unsigned char *)&tempSys, 46, 4, 16);
LogPrintf(LOG_DEBUG, "Convert1BEAMSET: aBeamsetL=%u startend=%d fracs=%d\n",
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

//NHexDump(LOG_DEBUG, "Convert1CONNECT", (unsigned char *)&tempSys, 46, 4, 16);
LogPrintf(LOG_DEBUG, "Convert1CONNECT: aConnectL=%u staffAbove=%d staffBelow=%d\n",
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

//NHexDump(LOG_DEBUG, "Convert1DYNAMIC", (unsigned char *)&tempSys, 46, 4, 16);
LogPrintf(LOG_DEBUG, "Convert1DYNAMIC: aDynamicL=%u mouthWidth=%d xd=%d\n",
aDynamicL, DynamicMOUTHWIDTH(aDynamicL), DynamicXD(aDynamicL));
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
	
//NHexDump(LOG_DEBUG, "Convert1GRNOTE", (unsigned char *)&tempSys, 46, 4, 16);
//LogPrintf(LOG_DEBUG, "Convert1GRNOTE: aGRNoteL=%u voice=%d yqpit=%d xd=%d yd=%d playDur=%d\n",
//aGRNoteL, GRNoteVOICE(aGRNoteL), GRNoteYQPIT(aGRNoteL), GRNoteXD(aGRNoteL), GRNoteYD(aGRNoteL), GRNotePLAYDUR(aGRNoteL));
	return True;
}


/* Convert the header of the object in <tmpSuperObj>, which is assumed to be in 'N105'
format (equivalent to Nightingale 5.8 and earlier), to Nightingale 5.9.x format in
<objL>. Object headers in the new format contain exactly the same information as in the
old format, so converting them is just a matter of copying the fields to their new
locations. */
  
static void ConvertObjHeader(Document *doc, LINK objL);

static void ConvertObjHeader(Document * /* doc */, LINK objL)
{
	typedef struct {
		OBJECTHEADER_5 
	} OBJHEADER;
	OBJHEADER tmpObjHeader_5;
	
	/* Copy the entire object header from the object list to a temporary space to
	   avoid clobbering anything before it's copied. */
	
	BlockMove(&tmpSuperObj, &tmpObjHeader_5, sizeof(OBJHEADER));
	
	/* The first six fields of the header are in the same location in 'N105' and 'N106'
	   format, so no need to do anything with them. Copy the others. */
	   
	LinkSEL(objL) = tmpObjHeader_5.selected;				
	LinkVIS(objL) = tmpObjHeader_5.visible;
	LinkSOFT(objL) = tmpObjHeader_5.soft;
	LinkVALID(objL) = tmpObjHeader_5.valid;
	LinkTWEAKED(objL) = tmpObjHeader_5.tweaked;
	LinkSPAREFLAG(objL) = tmpObjHeader_5.spareFlag;
	LinkOBJRECT(objL) = tmpObjHeader_5.objRect;
	LinkNENTRIES(objL) = tmpObjHeader_5.nEntries;
}


/* Convert the bodies of objects of each type. */

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
static Boolean ConvertTUPLET(Document *doc, LINK tupletL);
static Boolean ConvertGRSYNC(Document *doc, LINK grSyncL);
static Boolean ConvertTEMPO(Document *doc, LINK tempoL);

static Boolean ConvertSYNC(Document *doc, LINK syncL)
{
	SYNC_5 aSync;
	LINK aNoteRL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aSync, sizeof(SYNC_5));
	
	SyncTIME(syncL) = (&aSync)->timeStamp;

//NHexDump(LOG_DEBUG, "ConvertSYNC", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertSYNC: timeStamp=%d\n", syncL, SyncTIME(syncL)); 

	aNoteRL = FirstSubLINK(syncL);
	for ( ; aNoteRL; aNoteRL = NextNOTEL(aNoteRL)) {
		/* Copy the subobj to a separate ANOTE so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(NOTEheap, aNoteRL, PANOTE);
LogPrintf(LOG_DEBUG, "->block=%ld aNoteRL=%d sizeof(ANOTE)*aNoteRL=%d pSSubObj=%ld\n", (NOTEheap)->block,
aNoteRL, sizeof(ANOTE)*aNoteRL, pSSubObj);
		BlockMove(pSSubObj, &tmpANoteR, sizeof(ANOTE));
//NHexDump(LOG_DEBUG, "ConvertSYNC", (unsigned char *)&tmpANoteR, sizeof(ANOTE_5), 4, 16);

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

//NHexDump(LOG_DEBUG, "ConvertRPTEND", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertRPTEND: firstObj=L%u subType=%d count=%d\n", RptEndFIRSTOBJ(rptEndL),
RptType(rptEndL), RptEndCOUNT(rptEndL)); 

#if 1
	aRptEndL = FirstSubLINK(rptEndL);
	for ( ; aRptEndL; aRptEndL = NextRPTENDL(aRptEndL)) {
		/* Copy the subobj to a separate ARPTEND so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(RPTENDheap, aRptEndL, PARPTEND);
LogPrintf(LOG_DEBUG, "->block=%ld aRptEndL=%d sizeof(ARPTEND)*aRptEndL=%d pSSubObj=%ld\n", (RPTENDheap)->block,
aRptEndL, sizeof(ARPTEND)*aRptEndL, pSSubObj);
		BlockMove(pSSubObj, &tmpARptEnd, sizeof(ARPTEND));
//NHexDump(LOG_DEBUG, "ConvertRPTEND", (unsigned char *)&tmpARptEnd, sizeof(ARPTEND_5), 4, 16);

		Convert1RPTEND(doc, aRptEndL);
	}
#endif

	return True;
}


static Boolean ConvertPAGE(Document * /* doc */, LINK pageL)
{
	PAGE_5 aPage;
	
	BlockMove(&tmpSuperObj, &aPage, sizeof(PAGE_5));
	
	LinkLPAGE(pageL) = (&aPage)->lPage;
	LinkRPAGE(pageL) = (&aPage)->rPage;
	SheetNUM(pageL) = (&aPage)->sheetNum;

//NHexDump(LOG_DEBUG, "ConvertPAGE", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertPAGE: pageL=%u lPage=%u rPage=%u sheetNum=%d\n", pageL,
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
	
//NHexDump(LOG_DEBUG, "ConvertSYSTEM", (unsigned char *)&aSys, 44, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertSYSTEM: sysL=%u type=%d xd=%d sel=%d vis=%d\n", sysL, ObjLType(sysL),
LinkXD(sysL), LinkSEL(sysL), LinkVIS(sysL));
//LogPrintf(LOG_DEBUG, "ConvertSYSTEM: SysRect(t,l,b,r)=%d,%d,%d,%d\n", SysRectTOP(sysL),
//SysRectLEFT(sysL), SysRectBOTTOM(sysL), SysRectRIGHT(sysL));
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

//NHexDump(LOG_DEBUG, "ConvertSTAFF", (unsigned char *)&aStaff, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertSTAFF: nEntries=%d staffL=%u lStaff=%u rStaff=%u StaffSYS=%u\n",
LinkNENTRIES(staffL), staffL, LinkLSTAFF(staffL), LinkRSTAFF(staffL), StaffSYS(staffL)); 

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		/* Copy the subobj to a separate ASTAFF so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(STAFFheap, aStaffL, PASTAFF);
LogPrintf(LOG_DEBUG, "->block=%ld aStaffL=%d sizeof(ASTAFF)*aStaffL=%d pSSubObj=%ld\n", (STAFFheap)->block,
aStaffL, sizeof(ASTAFF)*aStaffL, pSSubObj);
		BlockMove(pSSubObj, &tmpAStaff, sizeof(ASTAFF));
//NHexDump(LOG_DEBUG, "ConvertSTAFF", (unsigned char *)&tmpAStaff, sizeof(ASTAFF_5), 4, 16);

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

//NHexDump(LOG_DEBUG, "ConvertMEASURE", (unsigned char *)&tempSys, 46, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertMEASURE: measL=%u lMeasure=%u rMeasure=%u systemL=%u\n", measL,
LinkLMEAS(measL), LinkRMEAS(measL), MeasSYSL(measL));

	aMeasureL = FirstSubLINK(measL);
	for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
		/* Copy the subobj to a separate AMEASURE so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(MEASUREheap, aMeasureL, PAMEASURE);
//LogPrintf(LOG_DEBUG, "->block=%ld aMeasureL=%d sizeof(AMEASURE)*aMeasureL=%d pSSubObj=%ld\n", (MEASUREheap)->block,
//aMeasureL, sizeof(AMEASURE)*aMeasureL, pSSubObj);
		BlockMove(pSSubObj, &tmpAMeasure, sizeof(AMEASURE));
//NHexDump(LOG_DEBUG, "ConvertMEASURE", (unsigned char *)&tmpAMeasure, sizeof(AMEASURE_5), 4, 16);

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

//NHexDump(LOG_DEBUG, "ConvertCLEF", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertCLEF: clefL=%u inMeasure=%d\n", clefL,
ClefINMEAS(clefL));

aClefL = FirstSubLINK(clefL);
	for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
		/* Copy the subobj to a separate ACLEF so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(CLEFheap, aClefL, PACLEF);
LogPrintf(LOG_DEBUG, "->block=%ld aClefL=%d sizeof(ACLEF)*aClefL=%d pSSubObj=%ld\n",
(CLEFheap)->block, aClefL, sizeof(ACLEF)*aClefL, pSSubObj);
		BlockMove(pSSubObj, &tmpAClef, sizeof(ACLEF));
NHexDump(LOG_DEBUG, "ConvertCLEF", (unsigned char *)&tmpAClef, sizeof(ACLEF_5), 4, 16);

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

//NHexDump(LOG_DEBUG, "ConvertKEYSIG", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertKEYSIG: keySigL=%u inMeasure=%d\n", keySigL,
KeySigINMEAS(keySigL));

	aKeySigL = FirstSubLINK(keySigL);
	for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
		/* Copy the subobj to a separate AKEYSIG so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(KEYSIGheap, aKeySigL, PAKEYSIG);
LogPrintf(LOG_DEBUG, "->block=%ld aKeySigL=%d sizeof(AKEYSIG)*aKeySigL=%d pSSubObj=%ld\n",
(KEYSIGheap)->block, aKeySigL, sizeof(AKEYSIG)*aKeySigL, pSSubObj);
		BlockMove(pSSubObj, &tmpAKeySig, sizeof(AKEYSIG));
NHexDump(LOG_DEBUG, "ConvertKEYSIG", (unsigned char *)&tmpAKeySig, sizeof(AKEYSIG_5), 4, 16);

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

//NHexDump(LOG_DEBUG, "ConvertTIMESIG", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertTIMESIG: timeSigL=%u inMeasure=%d\n", timeSigL,
TimeSigINMEAS(timeSigL));

	aTimeSigL = FirstSubLINK(timeSigL);
	for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
		/* Copy the subobj to a separate ATIMESIG so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(TIMESIGheap, aTimeSigL, PATIMESIG);
LogPrintf(LOG_DEBUG, "->block=%ld aTimeSigL=%d sizeof(ATIMESIG)*aTimeSigL=%d pSSubObj=%ld\n",
(TIMESIGheap)->block, aTimeSigL, sizeof(ATIMESIG)*aTimeSigL, pSSubObj);
		BlockMove(pSSubObj, &tmpATimeSig, sizeof(ATIMESIG));
NHexDump(LOG_DEBUG, "ConvertTIMESIG", (unsigned char *)&tmpATimeSig, sizeof(ATIMESIG_5), 4, 16);

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

//NHexDump(LOG_DEBUG, "ConvertBEAMSET", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertBEAMSET: beamsetL=%u\n", beamsetL);

	aBeamsetL = FirstSubLINK(beamsetL);
	for ( ; aBeamsetL; aBeamsetL = NextNOTEBEAML(aBeamsetL)) {
		/* Copy the subobj to a separate ABEAMSET so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(NOTEBEAMheap, aBeamsetL, PANOTEBEAM);
LogPrintf(LOG_DEBUG, "->block=%ld aBeamsetL=%d sizeof(ABEAMSET)*aBeamsetL=%d pSSubObj=%ld\n",
(NOTEBEAMheap)->block, aBeamsetL, sizeof(ANOTEBEAM)*aBeamsetL, pSSubObj);
		BlockMove(pSSubObj, &tmpANotebeam, sizeof(ANOTEBEAM));
NHexDump(LOG_DEBUG, "ConvertBEAMSET", (unsigned char *)&tmpANotebeam, sizeof(ANOTEBEAM_5), 4, 16);

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

//NHexDump(LOG_DEBUG, "ConvertCONNECT", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertCONNECT: connectL=%u\n", connectL);

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
		/* Copy the subobj to a separate ACONNECT so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(CONNECTheap, aConnectL, PACONNECT);
LogPrintf(LOG_DEBUG, "->block=%ld aConnectL=%d sizeof(ACONNECT)*aConnectL=%d pSSubObj=%ld\n",
(CONNECTheap)->block, aConnectL, sizeof(ACONNECT)*aConnectL, pSSubObj);
		BlockMove(pSSubObj, &tmpAConnect, sizeof(ACONNECT));
NHexDump(LOG_DEBUG, "ConvertCONNECT", (unsigned char *)&tmpAConnect, sizeof(ACONNECT_5), 4, 16);

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

//NHexDump(LOG_DEBUG, "ConvertDYNAMIC", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertDYNAMIC: dynamicL=%u dynamicType=%d\n", dynamicL, DynamType(dynamicL));

aDynamicL = FirstSubLINK(dynamicL);
	for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL)) {
		/* Copy the subobj to a separate ADYNAMIC so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(DYNAMheap, aDynamicL, PADYNAMIC);
LogPrintf(LOG_DEBUG, "->block=%ld aDynamicL=%d sizeof(ADYNAMIC)*aDynamicL=%d pSSubObj=%ld\n",
(DYNAMheap)->block, aDynamicL, sizeof(ADYNAMIC)*aDynamicL, pSSubObj);
		BlockMove(pSSubObj, &tmpADynamic, sizeof(ADYNAMIC));
NHexDump(LOG_DEBUG, "ConvertDYNAMIC", (unsigned char *)&tmpADynamic, sizeof(ADYNAMIC_5), 4, 16);

		Convert1DYNAMIC(doc, aDynamicL);
	}

	return True;
}


static Boolean ConvertTUPLET(Document *doc, LINK tupletL)
{
	TUPLET_5 aTuplet;
	LINK aTupletL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aTuplet, sizeof(TUPLET_5));
	
	TupletSTAFF(tupletL) = (&aTuplet)->staffn;		/* EXTOBJHEADER */

	TupletACCNUM(tupletL) = (&aTuplet)->accNum;
	TupletACCDENOIM(tupletL) = (&aTuplet)->accDenom;
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

//NHexDump(LOG_DEBUG, "ConvertTUPLET", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertTUPLET: accNum=%d accDenom=%d staff=%d\n", TupletACCNUM(tupletL),
TupletACCDENOIM(tupletL), TupletSTAFF(tupletL)); 

#if 0
	aTupletL = FirstSubLINK(tupletL);
	for ( ; aTupletL; aTupletL = NextTUPLETL(aTupletL)) {
		/* Copy the subobj to a separate ATUPLET so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(TUPLETheap, aTupletL, PATUPLET);
LogPrintf(LOG_DEBUG, "->block=%ld aTupletL=%d sizeof(ATUPLET)*aTupletL=%d pSSubObj=%ld\n", (TUPLETheap)->block,
aTupletL, sizeof(ATUPLET)*aTupletL, pSSubObj);
		BlockMove(pSSubObj, &tmpATuplet, sizeof(ATUPLET));
//NHexDump(LOG_DEBUG, "ConvertTUPLET", (unsigned char *)&tmpATuplet, sizeof(ATUPLET_5), 4, 16);

		Convert1TUPLET(doc, aTupletL);
	}
#endif

	return True;
}


static Boolean ConvertGRSYNC(Document *doc, LINK grSyncL)
{
	GRSYNC_5 aGRSync;
	LINK aGRNoteRL;
	unsigned char *pSSubObj;
	
	BlockMove(&tmpSuperObj, &aGRSync, sizeof(GRSYNC_5));
	
//NHexDump(LOG_DEBUG, "ConvertGRSYNC", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertGRSYNC: grSyncL=L%u\n", grSyncL); 

	aGRNoteRL = FirstSubLINK(grSyncL);
	for ( ; aGRNoteRL; aGRNoteRL = NextGRNOTEL(aGRNoteRL)) {
		/* Copy the subobj to a separate AGRNOTE so we can move fields all over
		the place without having to worry about clobbering anything. */

		pSSubObj = (unsigned char *)GetObjectPtr(NOTEheap, aGRNoteRL, PAGRNOTE);
LogPrintf(LOG_DEBUG, "->block=%ld aGRNoteRL=%d sizeof(AGRNOTE)*aGRNoteRL=%d pSSubObj=%ld\n", (NOTEheap)->block,
aGRNoteRL, sizeof(AGRNOTE)*aGRNoteRL, pSSubObj);
		BlockMove(pSSubObj, &tmpANoteR, sizeof(AGRNOTE));
//NHexDump(LOG_DEBUG, "ConvertGRSYNC", (unsigned char *)&tmpANoteR, sizeof(AGRNOTE_5), 4, 16);

		Convert1GRNOTE(doc, aGRNoteRL);
	}

	return True;
}




static Boolean ConvertTEMPO(Document *doc, LINK tempoL)
{
	TEMPO_5 aTempo;
	
	BlockMove(&tmpSuperObj, &aTempo, sizeof(TEMPO_5));
	
	TempoSTAFF(tempoL) = (&aTempo)->staffn;		/* EXTOBJHEADER */

	TempoEXPANDED(tempoL) = (&aTempo)->expanded;
	TempoNOMM(tempoL) = (&aTempo)->noMM;
	TempoFILLER(tempoL) = (&aTempo)->filler;
	TempoDOTTED(tempoL) = (&aTempo)->dotted;
	TempoHIDEMM(tempoL) = (&aTempo)->hideMM;
	TempoMM(tempoL) = (&aTempo)->tempoMM;
	TempoSTRING(tempoL) = (&aTempo)->strOffset;
	TempoFIRSTOBJ(tempoL) = (&aTempo)->firstObjL;
	TempoMETROSTR(tempoL) = (&aTempo)->metroStrOffset;
//NHexDump(LOG_DEBUG, "ConvertTEMPO", (unsigned char *)&tempSys, 38, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertTEMPO: tempoL=%u tempoType=%d\n", tempoL, DynamType(tempoL));

	return True;
}


/* -------------------------------------------------------------------- ConvertObjects -- */

/* Convert the headers and bodies of objects in either the main or the Master Page
object list. Any file-format-conversion code that doesn't affect the length of the header
or lengths or offsets of fields in objects (or subobjects) should go here. ??HOW ABOUT
SUBOBJECTS? Return True if all goes well, False if not.

This function assumes that the headers and the entire object list have been read; all
object and subobject links are valid; and all objects and subobjects are the correct
lengths. Tweaks that affect lengths or offsets to the headers should be done in
OpenFile(); to objects or subobjects, in ReadHeaps(). */

#define GetPSUPEROBJECT(link)	(PSUPEROBJECT)GetObjectPtr(OBJheap, link, PSUPEROBJECT)

Boolean ConvertObjects(Document *doc, unsigned long version, long /* fileTime */, Boolean
			doMasterList)
{
	HEAP *objHeap;
	LINK pL, startL;
	unsigned char *pSObj;

	if (version!='N105') {
		AlwaysErrMsg("Can't convert file of any version but 'N105'.  (ConvertObjects)");
		return False;
	}
	
	objHeap = doc->Heap + OBJtype;

	fflush(stdout);

	startL = (doMasterList?  doc->masterHeadL :  doc->headL);
	for (pL = startL; pL; pL = RightLINK(pL)) {
		/* Copy the object to a separate SUPEROBJECT so we can move fields all over the
		   place without having to worry about clobbering anything. */
		   
		pSObj = (unsigned char *)GetPSUPEROBJECT(pL);
		BlockMove(pSObj, &tmpSuperObj, sizeof(SUPEROBJECT));
//NHexDump(LOG_DEBUG, "ConvObjs1", (unsigned char *)&tmpSuperObj, 46, 4, 16);

		/* Convert the object header now so type-specific functions don't have to. */
		
		ConvertObjHeader(doc, pL);
//NHexDump(LOG_DEBUG, "ConvObjs2", pSObj, 46, 4, 16);
		
		switch (ObjLType(pL)) {
#ifdef NOTYET
			case HEADERtype:
				continue;
#endif
			case TAILtype:
				/* TAIL objects have no subobjs & no fields but header, so there's nothing to do. */
				continue;
			case SYNCtype:
				ConvertSYNC(doc, pL);
				continue;
			case RPTENDtype:
				ConvertRPTEND(doc, pL);
				continue;
			case PAGEtype:
				ConvertPAGE(doc, pL);
				continue;
			case SYSTEMtype:
				ConvertSYSTEM(doc, pL);
				continue;
			case STAFFtype:
				ConvertSTAFF(doc, pL);
				continue;
			case MEASUREtype:
				ConvertMEASURE(doc, pL);
				continue;
			case CLEFtype:
				ConvertCLEF(doc, pL);
				continue;
			case KEYSIGtype:
				ConvertKEYSIG(doc, pL);
				continue;
			case TIMESIGtype:
				ConvertTIMESIG(doc, pL);
				continue;
			case BEAMSETtype:
				ConvertBEAMSET(doc, pL);
				continue;
			case CONNECTtype:
				ConvertCONNECT(doc, pL);
				continue;
			case DYNAMtype:
				ConvertDYNAMIC(doc, pL);
				continue;
#ifdef NOTYET
			case GRAPHICtype:
				if (!ConvertGRAPHIC(doc, pL))  ERROR;
				continue;
			case OTTAVAtype:
				if (!ConvertOTTAVA(doc, pL))  ERROR;
				continue;
			case SLURtype:
				if (!ConvertSLUR(doc, pL))  ERROR;
				continue;
#endif
			case TUPLETtype:
				ConvertTUPLET(doc, pL);
				continue;
			case GRSYNCtype:
				ConvertGRSYNC(doc, pL);
				continue;
			case TEMPOtype:
				ConvertTEMPO(doc, pL);
				continue;
#ifdef NOTYET
			case SPACERtype:
				if (!ConvertSPACER(doc, pL))  ERROR;
				continue;
			case ENDINGtype:
				if (!ConvertENDING(doc, pL))  ERROR;
				continue;
			case PSMEAStype:
				if (!ConvertPSMEAS(doc, pL))  ERROR;
				continue;
#endif
			default:
				;
		}
	}


	/* Make sure all staves are visible in Master Page. They should never be invisible,
	but (as of v.997), they sometimes were, probably because not exporting changes to
	Master Page was implemented by reconstructing it from the 1st system of the score.
	That was fixed in about .998a10!, so it should certainly be safe to remove this call,
	but the thought makes me nervous, and making them visible here really shouldn't cause
	any problems (and, as far as I know, it never has). */

	VisifyMasterStaves(doc);
	
	return True;
}


/* ----------------------------------------------------------------------- ModifyScore -- */
/* Any temporary file-content-updating code (a.k.a. hacking) that doesn't affect the
length of the header or lengths of objects should go here. This function should only be
called after the header and entire object list have been read. Return True if all goes
well, False if not.

NB: If code here considers changing something, and especially if it ends up actually
doing so, it should call LogPrintf to display at least one very prominent message
in the console window, and SysBeep to draw attention to it! It should perhaps also
set doc->changed, though this will make it easier for people to accidentally overwrite
the original version.

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
	LINK aStaffL, aMeasureL, aPSMeasL, aClefL, aKeySigL, aTimeSigL, aNoteL,
			aTupletL, aRptEndL, aDynamicL, aGRNoteL;
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
   * "On upstemmed notes with flags, xmovedots should really 
   * be 2 or 3 larger [than normal], but then it should change whenever 
   * such notes are beamed or unbeamed or their stem direction or 
   * duration changes." */

  /* TC Oct 7 1999 (modified by DAB, 8 Oct.): quick and dirty hack to fix this problem
   * for Bill Hunt; on opening file, look at all unbeamed dotted notes with subtype >
   * quarter note and if they have stem up, increase xmovedots by 2; if we find any
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
              aNote->xmovedots = 3 + WIDEHEAD(aNote->subType) + 2;
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
