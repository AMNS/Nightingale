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

	for (v = 1; v<=MAXVOICES; v++)
		/* struct assignment works, e.g., in CopyToClip(); here, it gives an
		   incomprehensible compiler error message! So copy individual fields. */
		   
		doc->voiceTab[v].partn = doc->voiceTab[v].voiceRole = doc->voiceTab[v].relVoice = 0;
	for (j = 0; j<256-(MAXVOICES+1); j++)
		doc->expansion[j] = 0;	
}


/* ------------------------------------ Convert the content of objects (incl. headers) -- */

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
				// NB: Bad if NightStaffSizer used! See StaffSize2Rastral in ResizeStaff.c
				// from NightStaffSizer code as a way to a possible solution.
				StaffRASTRAL(aStaffL) = doc->srastral;
#endif
			}
		}
	}
}

#endif


/* --------------------------------------------------------------------- ConvertObject -- */

	SUPEROBJECT tmpSuperObj;

/* Object headers in Nightingale 5.9.x contain exactly the same information as in 'N105'
files, so converting them is just a matter of copying the fields to their new locations. */
  
static void ConvertObjHeader(Document *doc, LINK objL);
static void ConvertObjHeader(Document *doc, LINK objL)
{
	typedef struct {
	OBJECTHEADER_5
	} OBJHEADER;
	OBJHEADER tmpObjHeader_5;
	
	/* First, copy the entire object header from the object list to a temporary space. */
	
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


static Boolean ConvertSYSTEM(Document *doc, LINK sysL);
static Boolean ConvertSYSTEM(Document *doc, LINK sysL)
{
	char *pTmpSObj;
	SYSTEM tempSys;
	
	pTmpSObj = (char *)&tmpSuperObj;
	BlockMove(&tmpSuperObj, &tempSys, sizeof(SYSTEM));
	//SysRectTOP(sysL) = (&tempSys)->systemRect.top;
	SysRectTOP(sysL) = 0x1DAB;
	SysRectLEFT(sysL) = (&tempSys)->systemRect.left;
	SysRectBOTTOM(sysL) = (&tempSys)->systemRect.bottom;
	SysRectRIGHT(sysL) = (&tempSys)->systemRect.right;
	
DHexDump(LOG_DEBUG, "ConvertSYSTEM", (unsigned char *)&tempSys, 40, 4, 16);
LogPrintf(LOG_DEBUG, "ConvertSYSTEM: sysL=%u type=%d xd=%d sel=%d vis=%d\n", sysL, ObjLType(sysL),
LinkXD(sysL), LinkSEL(sysL), LinkVIS(sysL));
LogPrintf(LOG_DEBUG, "ConvertSYSTEM: SysRect(t,l,b,r)=%d,%d,%d,%d\n", SysRectTOP(sysL),
SysRectLEFT(sysL), SysRectBOTTOM(sysL), SysRectRIGHT(sysL));
	return True;
}

/* Any file-format-conversion code that doesn't affect the length of the header or
lengths or offsets of fields in objects (or subobjects) should go here. Return True if
all goes well, False if not.

This function assumes that the headers and the entire object list have been read; all
object and subobject links are valid; and all objects and subobjects are the correct
lengths. Tweaks that affect lengths or offsets to the headers should be done in
OpenFile(); to objects or subobjects, in ReadHeaps(). */

#define GetPSUPEROBJECT(link)	(PSUPEROBJECT)GetObjectPtr(OBJheap, link, PSUPEROBJECT)

Boolean ConvertObject(Document *doc, unsigned long version, long /* fileTime */)
{
	HEAP *objHeap;
	LINK pL;
	char *pSObj;

	if (version!='N105') {
		AlwaysErrMsg("Can't convert file of any version but 'N105'.");
		return False;
	}
	
	objHeap = doc->Heap + OBJtype;

	fflush(stdout);
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
	
		/* Copy the object to a separate SUPEROBJECT so we can move fields all over the
		   place without having to worry about clobbering anything. */
		   
		pSObj = (char *)GetPSUPEROBJECT(pL);
//DHexDump(LOG_DEBUG, "ConvertObject1", (unsigned char *)pSObj, 40, 4, 16);
		BlockMove(pSObj, &tmpSuperObj, sizeof(SUPEROBJECT));
DHexDump(LOG_DEBUG, "ConvertObject", (unsigned char *)&tmpSuperObj, 40, 4, 16);

		ConvertObjHeader(doc, pL);
LogPrintf(LOG_DEBUG, "ConvertObject: pL=%u type=%d xd=%d sel=%d vis=%d\n", pL, ObjLType(pL),
LinkXD(pL), LinkSEL(pL), LinkVIS(pL));
		
		switch (ObjLType(pL)) {
#ifdef NOTYET
			case HEADERtype:
				continue;
			case TAILtype:
				continue;
			case SYNCtype:
				if (!ConvertSYNC(doc, version, pL))  ERROR;
				continue;
			case RPTENDtype:
				if (!ConvertRPTEND(doc, pL))  ERROR;
				continue;
			case PAGEtype:
				continue;
#endif
			case SYSTEMtype:
				ConvertSYSTEM(doc, pL);
				continue;
#ifdef NOTYET
			case STAFFtype:
				continue;
			case MEASUREtype:
				if (!ConvertMEASURE(doc, pL))  ERROR;
				continue;
			case CLEFtype:
				if (!ConvertCLEF(doc, pL)) ERROR;
				continue;
			case KEYSIGtype:
				if (!ConvertKEYSIG(doc, pL))  ERROR;
				continue;
			case TIMESIGtype:
				if (!ConvertTIMESIG(doc, pL))  ERROR;
				continue;
			case BEAMSETtype:
				if (!ConvertBEAMSET(doc, pL))  ERROR;
				continue;
			case CONNECTtype:
				continue;
			case DYNAMtype:
				if (!ConvertDYNAMIC(doc, pL))  ERROR;
				continue;
			case GRAPHICtype:
				if (!ConvertGRAPHIC(doc, pL))  ERROR;
				continue;
			case OTTAVAtype:
				if (!ConvertOTTAVA(doc, pL))  ERROR;
				continue;
			case SLURtype:
				if (!ConvertSLUR(doc, pL))  ERROR;
				continue;
			case TUPLETtype:
				if (!ConvertTUPLET(doc, pL))  ERROR;
				continue;
			case GRSYNCtype:
				if (!ConvertGRSYNC(doc, pL))  ERROR;
				continue;
			case TEMPOtype:
				if (!ConvertTEMPO(doc, pL))  ERROR;
				continue;
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
	but the thought makes me nervous, and making them visible here shouldn't cause any
	problems and never has. */

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
LogPrintf(LOG_INFO, "  Timesig L%d\n", pL);
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
