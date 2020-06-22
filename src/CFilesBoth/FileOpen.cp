/******************************************************************************************
*	FILE:	FileOpen.c
*	PROJ:	Nightingale
*	DESC:	Input of normal Nightingale files
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "FileConversion.h"			/* Must follow Nightingale.precomp.h! */
#include "CarbonPrinting.h"
#include "FileUtils.h"
#include "MidiMap.h"


/* Prototypes for internal functions: */

static void DisplayDocumentHdr(short id, Document *doc);
static Boolean CheckDocumentHdr(Document *doc);
static void DisplayScoreHdr(short id, Document *doc);
static Boolean CheckScoreHdr(Document *doc);

static void SetTimeStamps(Document *);


/* Codes for object types being read/written or for non-read/write call when an I/O
error occurs; note that all are negative. See HeapFileIO.c for additional, positive,
codes. */

enum {
	HEADERobj=-999,
	VERSIONobj,
	SIZEobj,
	CREATEcall,
	OPENcall,				/* -995 */
	CLOSEcall,
	DELETEcall,
	RENAMEcall,
	WRITEcall,
	STRINGobj,
	INFOcall,
	SETVOLcall,
	BACKUPcall,
	MAKEFSSPECcall,
	NENTRIESerr = -899,
	READHEAPScall
};


/* A version code is four characters, specifically 'N' followed by three digits, e.g.,
'N105': N-one-zero-five. Be careful: It's not a valid C or Pascal string! */
static unsigned long version;							/* File version code read/written */

/* --------------------------------------------------------------------- SetTimeStamps -- */
/* Recompute timestamps up to the first Measure that has any unknown durs. in it. */

static void SetTimeStamps(Document *doc)
{
	LINK firstUnknown, stopMeas;
	
	firstUnknown = FindUnknownDur(doc->headL, GO_RIGHT);
	stopMeas = (firstUnknown? SSearch(firstUnknown, MEASUREtype, GO_LEFT)
								:  doc->tailL);
	FixTimeStamps(doc, doc->headL, LeftLINK(stopMeas));
}


/* ----------------------------------------------------------- Utilities for OpenFile  -- */

#define in2d(x)	pt2d(in2pt(x))		/* Convert inches to DDIST */
#define ERR(fn) { nerr++; LogPrintf(LOG_WARNING, " err #%d,", fn); if (firstErr==0) firstErr = fn; }

/* Display almost all Document header fields; there are about 18 in all. */

static void DisplayDocumentHdr(short id, Document *doc)
{
	LogPrintf(LOG_INFO, "Displaying Document header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  (1)origin.h=%d", doc->origin.h);
	LogPrintf(LOG_INFO, "  (2).v=%d", doc->origin.h);
	
	LogPrintf(LOG_INFO, "  (3)paperRect.top=%d", doc->paperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->paperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->paperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->paperRect.right);
	LogPrintf(LOG_INFO, "  (4)origPaperRect.top=%d", doc->origPaperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->origPaperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->origPaperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->origPaperRect.right);

	LogPrintf(LOG_INFO, "  (5)marginRect.top=%d", doc->marginRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->marginRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->marginRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->marginRect.right);

	LogPrintf(LOG_INFO, "  (6)currentSheet=%d", doc->currentSheet);
	LogPrintf(LOG_INFO, "  (7)numSheets=%d", doc->numSheets);
	LogPrintf(LOG_INFO, "  (8)firstSheet=%d", doc->firstSheet);
	LogPrintf(LOG_INFO, "  (9)firstPageNumber=%d", doc->firstPageNumber);
	LogPrintf(LOG_INFO, "  (10)startPageNumber=%d\n", doc->startPageNumber);
	
	LogPrintf(LOG_INFO, "  (11)numRows=%d", doc->numRows);
	LogPrintf(LOG_INFO, "  (12)numCols=%d", doc->numCols);
	LogPrintf(LOG_INFO, "  (13)pageType=%d", doc->pageType);
	LogPrintf(LOG_INFO, "  (14)measSystem=%d\n", doc->measSystem);
	
	LogPrintf(LOG_INFO, "  (15)headerFooterMargins.top=%d", doc->headerFooterMargins.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->headerFooterMargins.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->headerFooterMargins.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->headerFooterMargins.right);
}


/* Do a reality check for Document header values that might be bad. If any problems are
found, say so and Return False. NB: We're not checking anywhere near everything we could! */

static Boolean CheckDocumentHdr(Document *doc)
{
	short nerr = 0, firstErr = 0;
	
#ifdef NOTYET
	/* paperRect considers magnification. This is difficult to handle before <doc> is
	   fully installed and ready to draw, so skip checking for now. --DAB, April 2020 */
	   
	if (!RectIsValid(doc->paperRect, pt2p(0), pt2p(in2pt(12)))) ERR(3);
#endif
	if (!RectIsValid(doc->origPaperRect, 0, in2pt(12)))  ERR(4);
	if (!RectIsValid(doc->marginRect, 4, in2pt(12)))  ERR(5);
	if (doc->numSheets<1 || doc->numSheets>250)  ERR(7);
	if (doc->firstPageNumber<0 || doc->firstPageNumber>250)  ERR(9);
	if (doc->startPageNumber<0 || doc->startPageNumber>250)  ERR(10);
	if (doc->numRows < 1 || doc->numRows > 250)  ERR(11);
	if (doc->numCols < 1 || doc->numCols > 250)  ERR(12);
	if (doc->pageType < 0 || doc->pageType > 20)  ERR(13);

	if (nerr==0) {
		LogPrintf(LOG_NOTICE, "No errors found.  (CheckDocumentHdr)\n");
		return True;
	}
	else {
		if (!DETAIL_SHOW) DisplayDocumentHdr(4, doc);
		sprintf(strBuf, "%d error(s) found.", nerr);
		CParamText(strBuf, "", "", "");
		LogPrintf(LOG_ERR, " %d ERROR(S) FOUND (first bad field is no. %d).  (CheckDocumentHdr)\n",
					nerr, firstErr);
		StopInform(GENERIC_ALRT);
		return False;
	}
}


/* Display some Score header fields, but nowhere near all. There are around 200 fields;
about half give information about fonts. */

static void DisplayScoreHdr(short id, Document *doc)
{
	unsigned char tempFontName[32];
	
	LogPrintf(LOG_INFO, "Displaying Score header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  (1)nstaves=%d", doc->nstaves);
	LogPrintf(LOG_INFO, "  (2)nsystems=%d", doc->nsystems);
	LogPrintf(LOG_INFO, "  (3)noteInsFeedback=%d", doc->noteInsFeedback);
	LogPrintf(LOG_INFO, "  (4)used=%d", doc->used);
	LogPrintf(LOG_INFO, "  (5)transposed=%d\n", doc->transposed);
	
	LogPrintf(LOG_INFO, "  (6)polyTimbral=%d", doc->polyTimbral);
	LogPrintf(LOG_INFO, "  (7)spacePercent=%d", doc->spacePercent);
	LogPrintf(LOG_INFO, "  (8)srastral=%d", doc->srastral);	
	LogPrintf(LOG_INFO, "  (9)altsrastral=%d\n", doc->altsrastral);
		
	LogPrintf(LOG_INFO, "  (10)tempo=%d", doc->tempo);
	LogPrintf(LOG_INFO, "  (11)channel=%d", doc->channel);
	LogPrintf(LOG_INFO, "  (12)velocity=%d", doc->velocity);
	LogPrintf(LOG_INFO, "  (13)headerStrOffset=%d", doc->headerStrOffset);
	LogPrintf(LOG_INFO, "  (14)footerStrOffset=%d\n", doc->footerStrOffset);
	
	LogPrintf(LOG_INFO, "  (15)dIndentOther=%d", doc->dIndentOther);
	LogPrintf(LOG_INFO, "  (16)firstNames=%d", doc->firstNames);
	LogPrintf(LOG_INFO, "  (17)otherNames=%d", doc->otherNames);
	LogPrintf(LOG_INFO, "  (18)lastGlobalFont=%d\n", doc->lastGlobalFont);

	LogPrintf(LOG_INFO, "  (19)aboveMN=%c", (doc->aboveMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  (20)sysFirstMN=%c", (doc->sysFirstMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  (21)startMNPrint1=%c", (doc->startMNPrint1? '1' : '0'));
	LogPrintf(LOG_INFO, "  (22)firstMNNumber=%d\n", doc->firstMNNumber);

	LogPrintf(LOG_INFO, "  (23)nfontsUsed=%d", doc->nfontsUsed);
	Pstrcpy(tempFontName, doc->musFontName);
	LogPrintf(LOG_INFO, "  (24)musFontName='%s'\n", PtoCstr(tempFontName));
	
	LogPrintf(LOG_INFO, "  (25)magnify=%d", doc->magnify);
	LogPrintf(LOG_INFO, "  (26)selStaff=%d", doc->selStaff);
	LogPrintf(LOG_INFO, "  (27)currentSystem=%d", doc->currentSystem);
	LogPrintf(LOG_INFO, "  (28)spaceTable=%d", doc->spaceTable);
	LogPrintf(LOG_INFO, "  (29)htight=%d\n", doc->htight);
	
	LogPrintf(LOG_INFO, "  (30)lookVoice=%d", doc->lookVoice);
	LogPrintf(LOG_INFO, "  (31)ledgerYSp=%d", doc->ledgerYSp);
	LogPrintf(LOG_INFO, "  (32)deflamTime=%d", doc->deflamTime);
	LogPrintf(LOG_INFO, "  (33)colorVoices=%d", doc->colorVoices);
	LogPrintf(LOG_INFO, "  (34)dIndentFirst=%d\n", doc->dIndentFirst);
}


/* Do a reality check for Score header values that might be bad. If any problems are
found, say so and Return False. NB: We're not checking anywhere near everything we could!
But we assume we're called as soon as the score header is read, and before the stringPool
size is, so we can't fully check numbers that depend on it. */

static Boolean CheckScoreHdr(Document *doc)
{
	short nerr = 0, firstErr = 0;
	
	if (doc->nstaves<1 || doc->nstaves>MAXSTAVES) ERR(1);
	if (doc->nsystems<1 || doc->nsystems>2000) ERR(2);
	if (doc->spacePercent<MINSPACE || doc->spacePercent>MAXSPACE) ERR(7);
	if (doc->srastral<0 || doc->srastral>MAXRASTRAL) ERR(8);
	if (doc->altsrastral<1 || doc->altsrastral>MAXRASTRAL) ERR(9);
	if (doc->tempo<MIN_BPM || doc->tempo>MAX_BPM) ERR(10);
	if (doc->channel<1 || doc->channel>MAXCHANNEL) ERR(11);
	if (doc->velocity<-127 || doc->velocity>127) ERR(12);

	/* We can't check if headerStrOffset and footerStrOffset are too large: we don't
	   know the size of the stringPool yet. */
	   
	if (doc->headerStrOffset<0) ERR(13);
	if (doc->footerStrOffset<0) ERR(14);
	
	if (doc->dIndentOther<0 || doc->dIndentOther>in2d(5)) ERR(15);
	if (doc->firstNames<0 || doc->firstNames>MAX_NAMES_TYPE) ERR(16);
	if (doc->otherNames<0 || doc->otherNames>MAX_NAMES_TYPE) ERR(17);
	if (doc->lastGlobalFont<FONT_THISITEMONLY || doc->lastGlobalFont>MAX_FONTSTYLENUM) ERR(18);
	if (doc->firstMNNumber<0 || doc->firstMNNumber>MAX_FIRSTMEASNUM) ERR(22);
	if (doc->nfontsUsed<1 || doc->nfontsUsed>MAX_SCOREFONTS) ERR(23);
	if (doc->magnify<MIN_MAGNIFY || doc->magnify>MAX_MAGNIFY) ERR(25);
	if (doc->selStaff<-1 || doc->selStaff>doc->nstaves) ERR(26);
	if (doc->currentSystem<1 || doc->currentSystem>doc->nsystems) ERR(27);
	if (doc->spaceTable<0) ERR(28);
	if (doc->htight<MINSPACE || doc->htight>MAXSPACE) ERR(29);
	if (doc->lookVoice<-1 || doc->lookVoice>MAXVOICES) ERR(30);
	if (doc->ledgerYSp<0 || doc->ledgerYSp>40) ERR(31);
	if (doc->deflamTime<1 || doc->deflamTime>1000) ERR(32);
	if (doc->dIndentFirst<0 || doc->dIndentFirst>in2d(5)) ERR(34);
	
	if (nerr==0) {
		LogPrintf(LOG_NOTICE, "No errors found.  (CheckScoreHdr)\n");
		return True;
	}
	else {
		if (!DETAIL_SHOW) {
			LogPrintf(LOG_WARNING, "\n");
			DisplayScoreHdr(4, doc);
		}
		sprintf(strBuf, "%d error(s) found.", nerr);
		CParamText(strBuf, "", "", "");
		LogPrintf(LOG_ERR, " %d ERROR(S) FOUND (first bad field is no. %d).  (CheckScoreHdr)\n",
					nerr, firstErr);
		StopInform(GENERIC_ALRT);
		return False;
	}
}


/* -------------------------------------------------------------------------- OpenFile -- */
/* Open and read in the specified file. If there's an error, normally (see comments in
OpenError) gives an error message, and returns <errCode>; else returns noErr (0). Also
sets *fileVersion to the Nightingale version that created the file. We assume doc exists
and has had standard fields initialized. FIXME: even though vRefNum is a parameter,
(routines called by) OpenFile assume the volume is already set! This should be changed. */

enum {
	LOW_VERSION_ERR=-999,
	HI_VERSION_ERR,
	HEADER_ERR,
	LASTTYPE_ERR,				/* Value for LASTtype in file is not we expect it to be */
	TOOMANYSTAVES_ERR,
	LOWMEM_ERR,
	BAD_VERSION_ERR
};

extern void StringPoolEndianFix(StringPoolRef pool);
extern short StringPoolProblem(StringPoolRef pool);

#include <ctype.h>

short OpenFile(Document *doc, unsigned char *filename, short vRefNum, FSSpec *pfsSpec,
																	long *fileVersion)
{
	short		errCode, refNum, strPoolErrCode;
	short 		errInfo=0,				/* Type of object being read or other info on error */
				lastType, i;
	long		count, stringPoolSize,
				fileTime;
	Boolean		fileIsOpen;
	OMSSignature omsDevHdr;
	long		fmsDevHdr;
	long		omsBufCount, omsDevSize;
	FInfo		fInfo;
	FSSpec 		fsSpec;
	long		cmHdr, cmBufCount, cmDevSize;
	FSSpec		*pfsSpecMidiMap;
	char		versionCString[5];
	DocumentN105 aDocN105;

	WaitCursor();

	fileIsOpen = False;
	
	fsSpec = *pfsSpec;
	errCode = FSpOpenDF(&fsSpec, fsRdWrPerm, &refNum);			/* Open the file */
	if (errCode == fLckdErr || errCode == permErr) {
		doc->readOnly = True;
		errCode = FSpOpenDF (&fsSpec, fsRdPerm, &refNum);		/* Try again - open the file read-only */
	}
	if (errCode) { errInfo = OPENcall;  goto Error; }
	fileIsOpen = True;

	count = sizeof(version);									/* Read & check file version code */
	errCode = FSRead(refNum, &count, &version);
	FIX_END(version);
	if (errCode) { errInfo = VERSIONobj;  goto Error;}
	*fileVersion = version;
	MacTypeToString(version, versionCString);
	
	/* If user has the secret keys down, pretend file is in current version. */
	
	if (ShiftKeyDown() && OptionKeyDown() && CmdKeyDown() && ControlKeyDown()) {
		LogPrintf(LOG_NOTICE, "IGNORING FILE'S VERSION CODE '%s'.  (OpenFile)\n", versionCString);
		GetIndCString(strBuf, FILEIO_STRS, 6);					/* "IGNORING FILE'S VERSION CODE!" */
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
		version = THIS_FILE_VERSION;
	}

	if (versionCString[0]!='N') { errCode = BAD_VERSION_ERR; goto Error; }
	if ( !isdigit(versionCString[1])
			|| !isdigit(versionCString[2])
			|| !isdigit(versionCString[3]) )
		 { errCode = BAD_VERSION_ERR; goto Error; }
	if (version<FIRST_FILE_VERSION) { errCode = LOW_VERSION_ERR; goto Error; }
	if (version>THIS_FILE_VERSION) { errCode = HI_VERSION_ERR; goto Error; }
	if (version!=THIS_FILE_VERSION) LogPrintf(LOG_NOTICE, "CONVERTING VERSION '%s' FILE.  (OpenFile)\n", versionCString);

//#define DIFF(addr1, addr2)	((long)(&addr1)-(long)(&addr2))
//if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "Offset of aDocN105.comment[0]=%lu, spacePercent=%lu, fillerMB=%lu, nFontRecords=%lu, nfontsUsed=%lu, yBetweenSys=%lu\n",
//DIFF(aDocN105.comment[0], aDocN105.headL), DIFF(aDocN105.spacePercent, aDocN105.headL), DIFF(aDocN105.fillerMB, aDocN105.headL), DIFF(aDocN105.nFontRecords, aDocN105.headL),
//DIFF(aDocN105.nfontsUsed, aDocN105.headL), DIFF(aDocN105.yBetweenSys, aDocN105.headL));

	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "Header size for Document=%ld, for Score=%ld, for N105 Score=%ld.  (OpenFile)\n",
		sizeof(DOCUMENTHDR), sizeof(SCOREHEADER), sizeof(SCOREHEADER_N105));

	count = sizeof(fileTime);									/* Time file was written */
	errCode = FSRead(refNum, &count, &fileTime);
	FIX_END(fileTime);
	if (errCode) { errInfo = VERSIONobj; goto Error; }
//{ long fPos;  GetFPos(refNum, &fPos);  LogPrintf(LOG_DEBUG, "fPos(1)=%ld\n", fPos); }
		
	/* Read and, if necessary, convert Document (i.e., Sheets) and Score headers. */
	
	switch(version) {
		case 'N105':
			count = sizeof(DOCUMENTHDR);
			errCode = FSRead(refNum, &count, &aDocN105.origin);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			ConvertDocumentHeader(doc, &aDocN105);
			if (DETAIL_SHOW) DisplayDocumentHdr(1, doc);
			
			count = sizeof(SCOREHEADER_N105);
			errCode = FSRead(refNum, &count, &aDocN105.headL);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			ConvertScoreHeader(doc, &aDocN105);
			if (DETAIL_SHOW) DisplayScoreHdr(1, doc);
			break;
		
		default:
			count = sizeof(DOCUMENTHDR);
			errCode = FSRead(refNum, &count, &doc->origin);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			
			count = sizeof(SCOREHEADER);
			errCode = FSRead(refNum, &count, &doc->headL);
			if (errCode) { errInfo = HEADERobj; goto Error; }
	}

	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "Fixing headers for CPU's Endian property...  (OpenFile)\n");	
	EndianFixDocumentHdr(doc);
	if (DETAIL_SHOW) DisplayDocumentHdr(2, doc);
	LogPrintf(LOG_NOTICE, "Checking Document header: ");
	if (!CheckDocumentHdr(doc)) {
		if (!DETAIL_SHOW) DisplayDocumentHdr(3, doc);
		sprintf(strBuf, "Error(s) found in Document header.");
		CParamText(strBuf, "", "", "");
		goto HeaderError;
	}
	
//DisplayScoreHdr(2, doc);		// ??TEMPORARY, TO DEBUG INTEL VERSION!!!!
	EndianFixScoreHdr(doc);
	if (DETAIL_SHOW) DisplayScoreHdr(3, doc);
	LogPrintf(LOG_NOTICE, "Checking Score header: ");
	if (!CheckScoreHdr(doc)) {
		if (!DETAIL_SHOW) DisplayScoreHdr(3, doc);
		sprintf(strBuf, "Error(s) found in Score header.");
		CParamText(strBuf, "", "", "");
		goto HeaderError;
	}

	count = sizeof(lastType);
	errCode = FSRead(refNum, &count, &lastType);
//{ long fPos;  GetFPos(refNum, &fPos);  LogPrintf(LOG_DEBUG, "fPos(4)=%ld\n", fPos); }
	if (errCode) { errInfo = HEADERobj; goto Error; }
	FIX_END(lastType);

	if (lastType!=LASTtype) {
		LogPrintf(LOG_ERR, "LAST OBJECT TYPE IS %d BUT SHOULD BE %d.  (OpenFile)\n", lastType, LASTtype);	
		errCode = LASTTYPE_ERR;
		errInfo = HEADERobj;
		goto Error;
	}

	/* Read and check string pool size, and read string pool. */

	count = sizeof(stringPoolSize);
	errCode = FSRead(refNum, &count, &stringPoolSize);
	if (errCode) { errInfo = STRINGobj; goto Error; }
	FIX_END(stringPoolSize);
	LogPrintf(LOG_INFO, "stringPoolSize=%ld  (OpenFile)\n", stringPoolSize);
	if (doc->headerStrOffset > stringPoolSize) {
		LogPrintf(LOG_ERR, "headerStrOffset is %ld but stringPoolSize is only %ld.  (OpenFile)\n",
					doc->headerStrOffset, stringPoolSize);
		errInfo = STRINGobj; goto Error;
	}
	if (doc->footerStrOffset > stringPoolSize) {
		LogPrintf(LOG_ERR, "footerStrOffset is %ld but stringPoolSize is only %ld.  (OpenFile)\n",
					doc->footerStrOffset, stringPoolSize);
		errInfo = STRINGobj; goto Error;
	}
	if (doc->stringPool) DisposeStringPool(doc->stringPool);
	
	/* Allocate from the StringManager, not NewHandle, in case StringManager is tracking
	   its allocations. Then, since we're going to overwrite everything with stuff from
	   the file below, we can resize it to what it was when saved. */
	
	doc->stringPool = NewStringPool();
	if (doc->stringPool == NULL) { errInfo = STRINGobj; goto Error; }
	SetHandleSize((Handle)doc->stringPool, stringPoolSize);
	
	HLock((Handle)doc->stringPool);
	errCode = FSRead(refNum, &stringPoolSize, *doc->stringPool);
	if (errCode) { errInfo = STRINGobj; goto Error; }
	HUnlock((Handle)doc->stringPool);
	SetStringPool(doc->stringPool);
	StringPoolEndianFix(doc->stringPool);
	strPoolErrCode = StringPoolProblem(doc->stringPool);
	if (strPoolErrCode!=0) {
		AlwaysErrMsg("The string pool is probably bad (code=%ld).  (OpenFile)\n", (long)strPoolErrCode);
		errInfo = STRINGobj; goto Error;
	}
	
	errCode = FSpGetFInfo(&fsSpec, &fInfo);
	if (errCode!=noErr) { errInfo = INFOcall; goto Error; }
	
	/* Read the subobject heaps and the object heap in the rest of the file, and if
	   necessary, convert them to the current format. */
	
	errCode = ReadHeaps(doc, refNum, version, fInfo.fdType);
#if 0
{	unsigned char *pSObj;
pSObj = (unsigned char *)GetPSUPEROBJ(1);
NHexDump(LOG_DEBUG, "OpenFile L1", pSObj, 46, 4, 16);
pSObj = (unsigned char *)GetPSUPEROBJ(2);
NHexDump(LOG_DEBUG, "OpenFile L2", pSObj, 46, 4, 16);
pSObj = (unsigned char *)GetPSUPEROBJ(3);
NHexDump(LOG_DEBUG, "OpenFile L3", pSObj, 46, 4, 16);
}
#endif
	if (errCode!=noErr) { errInfo = READHEAPScall; goto Error; }

	/* An ancient comment here: "Be sure we have enough memory left for a maximum-size
	   segment and a bit more." Now we require a _lot_ more, though it may be pointless. */
	
	if (!PreflightMem(400)) { NoMoreMemory(); return LOWMEM_ERR; }
	
	/* Do any further conversion needed of both the main and the Master Page object lists
	   in old files. */
	if (version=='N105') {
		ConvertObjects(doc, version, fileTime, False);
		ConvertObjects(doc, version, fileTime, True);
	}

	Pstrcpy(doc->name, filename);				/* Remember filename and vol refnum after scoreHead is overwritten */
	doc->vrefnum = vRefNum;
	doc->changed = False;
	doc->saved = True;							/* Has to be, since we just opened it! */
	doc->named = True;							/* Has to have a name, since we just opened it! */

	ModifyScore(doc, fileTime);					/* Do any hacking needed: ordinarily none */

	/* Read the document's OMS device numbers for each part. if "devc" signature block not
	   found at end of file, pack the document's omsPartDeviceList with default values. */
	
	omsBufCount = sizeof(OMSSignature);
	errCode = FSRead(refNum, &omsBufCount, &omsDevHdr);
	if (!errCode && omsDevHdr == 'devc') {
		omsBufCount = sizeof(long);
		errCode = FSRead(refNum, &omsBufCount, &omsDevSize);
		if (errCode) return errCode;
		if (omsDevSize!=(MAXSTAVES+1)*sizeof(OMSUniqueID)) return errCode;
		errCode = FSRead(refNum, &omsDevSize, &(doc->omsPartDeviceList[0]));
		if (errCode) return errCode;
	}
	else {
		for (i = 1; i<=LinkNENTRIES(doc->headL)-1; i++)
			doc->omsPartDeviceList[i] = config.defaultOutputDevice;
		doc->omsInputDevice = config.defaultInputDevice;
	}

	/* Read the FreeMIDI input device data. */
	
	doc->fmsInputDevice = noUniqueID;
	
	/* We're probably not supposed to play with these fields, but FreeMIDI is obsolete
	   anyway, so the only change worth making is to remove these statements some day.
	   --DAB, December 2019 */
	
	doc->fmsInputDestination.basic.destinationType = 0,
	doc->fmsInputDestination.basic.name[0] = 0;
	count = sizeof(long);
	errCode = FSRead(refNum, &count, &fmsDevHdr);
	if (!errCode) {
		if (fmsDevHdr==FreeMIDISelector) {
			count = sizeof(fmsUniqueID);
			errCode = FSRead(refNum, &count, &doc->fmsInputDevice);
			if (errCode) return errCode;
			count = sizeof(fmsDestinationMatch);
			errCode = FSRead(refNum, &count, &doc->fmsInputDestination);
			if (errCode) return errCode;
		}
	}
	
	if (version >= 'N105') {
		cmBufCount = sizeof(long);
		errCode = FSRead(refNum, &cmBufCount, &cmHdr);
		if (!errCode && cmHdr == 'cmdi') {
			cmBufCount = sizeof(long);
			errCode = FSRead(refNum, &cmBufCount, &cmDevSize);
			if (errCode) return errCode;
			if (cmDevSize!=(MAXSTAVES+1)*sizeof(MIDIUniqueID)) return errCode;
			errCode = FSRead(refNum, &cmDevSize, &(doc->cmPartDeviceList[0]));
			if (errCode) return errCode;
		}
	}
	else {
		for (i = 1; i<=LinkNENTRIES(doc->headL)-1; i++)
			doc->cmPartDeviceList[i] = config.cmDfltOutputDev;
	}
	doc->cmInputDevice = config.cmDfltInputDev;

	errCode = FSClose(refNum);
	if (errCode) { errInfo = CLOSEcall; goto Error; }
	
	if (!IsDocPrintInfoInstalled(doc)) InstallDocPrintInfo(doc);
	
	GetPrintHandle(doc, version, vRefNum, pfsSpec);		/* doc->name must be set before this */
	
	GetMidiMap(doc, pfsSpec);
	
	pfsSpecMidiMap = GetMidiMapFSSpec(doc);
	if (pfsSpecMidiMap != NULL) {
		OpenMidiMapFile(doc, pfsSpecMidiMap);
		ReleaseMidiMapFSSpec(doc);		
	}

	FillFontTable(doc);

	InitDocMusicFont(doc);

	SetTimeStamps(doc);									/* Up to first meas. w/unknown durs. */


	/* Assume that no information in the score having to do with window-relative
	   positions is valid. Besides clearing the object <valid> flags to indicate this,
	   protect ourselves against functions that might not check the flags properly by
	   setting all such positions to safe values now. */
	
	InvalRange(doc->headL, doc->tailL);					/* Force computing all objRects */

	doc->hasCaret = False;									/* Caret must still be set up */
	doc->selStartL = doc->headL;							/* Set selection range */								
	doc->selEndL = doc->tailL;								/*   to entire score, */
	DeselAllNoHilite(doc);									/*   deselect it */
	SetTempFlags(doc, doc, doc->headL, doc->tailL, False);	/* Just in case */

	if (ScreenPagesExceedView(doc))
		CautionInform(MANYPAGES_ALRT);

	ArrowCursor();	
	return 0;

HeaderError:
	StopInform(GENERIC_ALRT);
	errCode = HEADER_ERR;
	errInfo = 0;
	/* drop through */
Error:
	OpenError(fileIsOpen, refNum, errCode, errInfo);
	return errCode;
}


/* ------------------------------------------------------------------------- OpenError -- */
/* Handle errors occurring while reading a file. <fileIsOpen> indicates  whether or
not the file was open when the error occurred; if so, OpenError closes it. <errCode>
is either an error code return by the file system manager or one of our own codes
(see enum above). <errInfo> indicates at what step of the process the error happened
(CREATEcall, OPENcall, etc.: see enum above), the type of object being read, or some
other additional information on the error.

Note that after a call to OpenError with fileIsOpen, you should not try to keep reading,
since the file will no longer be open! */

void OpenError(Boolean fileIsOpen,
					short refNum,	/* 0 = file's refNum unknown */
					short errCode,
					short errInfo)	/* More info: at what step error happened, type of obj being read, etc. */
{
	char aStr[256], fmtStr[256];
	StringHandle strHdl;
	short strNum;
	
	SysBeep(1);
	LogPrintf(LOG_ERR, "CAN'T OPEN THE FILE. errCode=%d errInfo=%d  (OpenError)\n", errCode, errInfo);
	if (fileIsOpen && refNum!=0) FSClose(refNum);

	switch (errCode) {
		/*
		 * First handle our own codes that need special treatment.
		 */
		case BAD_VERSION_ERR:
			GetIndCString(fmtStr, FILEIO_STRS, 7);			/* "file version is illegal" */
			sprintf(aStr, fmtStr, ACHAR(version, 3), ACHAR(version, 2),
						 ACHAR(version, 1), ACHAR(version, 0));
			break;
		case LOW_VERSION_ERR:
			GetIndCString(fmtStr, FILEIO_STRS, 8);			/* "too old for this version of Nightingale" */
			sprintf(aStr, fmtStr, ACHAR(version, 3), ACHAR(version, 2),
						 ACHAR(version, 1), ACHAR(version, 0));
			break;
		case HI_VERSION_ERR:
			GetIndCString(fmtStr, FILEIO_STRS, 9);			/* "newer than this version of Nightingale" */
			sprintf(aStr, fmtStr, ACHAR(version, 3), ACHAR(version, 2),
						 ACHAR(version, 1), ACHAR(version, 0));
			break;
		case TOOMANYSTAVES_ERR:
			GetIndCString(fmtStr, FILEIO_STRS, 10);		/* "this version can handle only %d staves" */
			sprintf(aStr, fmtStr, errInfo, MAXSTAVES);
			break;
		default:
			/*
			 * We expect descriptions of the common errors stored by code (negative
			 * values, for system errors; positive ones for our own I/O errors) in
			 * individual 'STR ' resources. If we find one for this error, print it,
			 * else just print the raw code.
			 */
			strHdl = GetString(errCode);
			if (strHdl) {
				Pstrcpy((unsigned char *)strBuf, (unsigned char *)*strHdl);
				PToCString((unsigned char *)strBuf);
				strNum = (errInfo>0? 15 : 16);	/* "%s (heap object type=%d)." : "%s (error code=%d)." */
				GetIndCString(fmtStr, FILEIO_STRS, strNum);
				sprintf(aStr, fmtStr, strBuf, errInfo);
			}
			else {
				strNum = (errInfo>0? 17 : 18);	/* "Error ID=%d (heap object type=%d)." : "Error ID=%d (error code=%d)." */
				GetIndCString(fmtStr, FILEIO_STRS, strNum);
				sprintf(aStr, fmtStr, errCode, errInfo);
			}
	}
	LogPrintf(LOG_WARNING, aStr); LogPrintf(LOG_WARNING, "  (OpenError)\n");
	CParamText(aStr, "", "", "");
	StopInform(READ_ALRT);
}
