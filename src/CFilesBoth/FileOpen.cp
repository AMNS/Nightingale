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
	NENTRIESerr = -899
};


/* A version code is four characters, specifically 'N' followed by three digits, e.g.,
'N105': N-one-zero-five. Be careful: It's neither a number nor a valid C string! */
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

/* Display most Document header fields; there are about 18 in all. */

static void DisplayDocumentHdr(short id, Document *doc)
{
	LogPrintf(LOG_INFO, "Displaying Document header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  origin.h=%d", doc->origin.h);
	LogPrintf(LOG_INFO, "  .v=%d", doc->origin.h);
	
	LogPrintf(LOG_INFO, "  paperRect.top=%d", doc->paperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->paperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->paperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->paperRect.right);
	LogPrintf(LOG_INFO, "  origPaperRect.top=%d", doc->origPaperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->origPaperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->origPaperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->origPaperRect.right);
	LogPrintf(LOG_INFO, "  marginRect.top=%d", doc->marginRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->marginRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->marginRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->marginRect.right);

	LogPrintf(LOG_INFO, "  currentSheet=%d", doc->currentSheet);
	LogPrintf(LOG_INFO, "  numSheets=%d", doc->numSheets);
	LogPrintf(LOG_INFO, "  firstSheet=%d", doc->firstSheet);
	LogPrintf(LOG_INFO, "  firstPageNumber=%d", doc->firstPageNumber);
	LogPrintf(LOG_INFO, "  startPageNumber=%d\n", doc->startPageNumber);
	
	LogPrintf(LOG_INFO, "  numRows=%d", doc->numRows);
	LogPrintf(LOG_INFO, "  numCols=%d", doc->numCols);
	LogPrintf(LOG_INFO, "  pageType=%d", doc->pageType);
	LogPrintf(LOG_INFO, "  measSystem=%d\n", doc->measSystem);	
}

/* Do a reality check for Document header values that might be bad. If any problems are
found, say so and Return False. NB: We're not checking anywhere near everything we could! */

static Boolean CheckDocumentHdr(Document *doc)
{
	short nerr = 0, firstErr = 0;
	
#ifdef NOTYET
	if (!RectIsValid(doc->paperRect, 4??, in2pt(5)??)) ERR(1);
	if (!RectIsValid(doc->origPaperRect, 4??, in2pt(5)??))  ERR(2);
	if (!RectIsValid(doc->marginRect, 4??, in2pt(5)??))  ERR(3);
#endif
	if (doc->numSheets<1 || doc->numSheets>250)  ERR(4);
	if (doc->firstPageNumber<0 || doc->firstPageNumber>250)  ERR(5);
	if (doc->startPageNumber<0 || doc->startPageNumber>250)  ERR(6);
	if (doc->numRows < 1 || doc->numRows > 250)  ERR(7);
	if (doc->numCols < 1 || doc->numCols > 250)  ERR(8);
	if (doc->pageType < 0 || doc->pageType > 20)  ERR(9);
	if (nerr==0) {
		LogPrintf(LOG_NOTICE, "No errors found.  (CheckDocumentHdr)\n");
		return True;
	}
	else {
		if (!DETAIL_SHOW) DisplayScoreHdr(4, doc);
		sprintf(strBuf, "%d error(s) found.", nerr);
		CParamText(strBuf, "", "", "");
		LogPrintf(LOG_ERR, " %d error(s) found (first bad field is no. %d).  (CheckDocumentHdr)\n",
					nerr, firstErr);
		StopInform(GENERIC_ALRT);
		return False;
	}
}

/* Display some Score header fields, but nowhere near all: there are about 200. */

static void DisplayScoreHdr(short id, Document *doc)
{
	unsigned char tempFontName[32];
	
	LogPrintf(LOG_INFO, "Displaying Score header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  nstaves=%d", doc->nstaves);
	LogPrintf(LOG_INFO, "  nsystems=%d", doc->nsystems);		
	LogPrintf(LOG_INFO, "  spacePercent=%d", doc->spacePercent);
	LogPrintf(LOG_INFO, "  srastral=%d", doc->srastral);				
	LogPrintf(LOG_INFO, "  altsrastral=%d\n", doc->altsrastral);
		
	LogPrintf(LOG_INFO, "  tempo=%d", doc->tempo);		
	LogPrintf(LOG_INFO, "  channel=%d", doc->channel);			
	LogPrintf(LOG_INFO, "  velocity=%d", doc->velocity);		
	LogPrintf(LOG_INFO, "  headerStrOffset=%d", doc->headerStrOffset);	
	LogPrintf(LOG_INFO, "  footerStrOffset=%d", doc->footerStrOffset);	
	LogPrintf(LOG_INFO, "  dIndentOther=%d\n", doc->dIndentOther);
	
	LogPrintf(LOG_INFO, "  firstNames=%d", doc->firstNames);
	LogPrintf(LOG_INFO, "  otherNames=%d", doc->otherNames);
	LogPrintf(LOG_INFO, "  lastGlobalFont=%d\n", doc->lastGlobalFont);

	LogPrintf(LOG_INFO, "  aboveMN=%c", (doc->aboveMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  sysFirstMN=%c", (doc->sysFirstMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  startMNPrint1=%c", (doc->startMNPrint1? '1' : '0'));
	LogPrintf(LOG_INFO, "  firstMNNumber=%d\n", doc->firstMNNumber);

	LogPrintf(LOG_INFO, "  nfontsUsed=%d", doc->nfontsUsed);
	Pstrcpy(tempFontName, doc->musFontName);
	LogPrintf(LOG_INFO, "  musFontName='%s'\n", PtoCstr(tempFontName));
	
	LogPrintf(LOG_INFO, "  magnify=%d", doc->magnify);
	LogPrintf(LOG_INFO, "  selStaff=%d", doc->selStaff);
	LogPrintf(LOG_INFO, "  currentSystem=%d", doc->currentSystem);
	LogPrintf(LOG_INFO, "  spaceTable=%d", doc->spaceTable);
	LogPrintf(LOG_INFO, "  htight=%d\n", doc->htight);
	
	LogPrintf(LOG_INFO, "  lookVoice=%d", doc->lookVoice);
	LogPrintf(LOG_INFO, "  ledgerYSp=%d", doc->ledgerYSp);
	LogPrintf(LOG_INFO, "  deflamTime=%d", doc->deflamTime);
	LogPrintf(LOG_INFO, "  colorVoices=%d", doc->colorVoices);
	LogPrintf(LOG_INFO, "  dIndentFirst=%d\n", doc->dIndentFirst);
}

/* Do a reality check for Score header values that might be bad. If any problems are
found, say so and Return False. NB: We're not checking anywhere near everything we could! */

static Boolean CheckScoreHdr(Document *doc)
{
	short nerr = 0, firstErr = 0;
	
	if (doc->nstaves<1 || doc->nstaves>MAXSTAVES) ERR(1);
	if (doc->nsystems<1 || doc->nsystems>2000) ERR(2);
	if (doc->spacePercent<MINSPACE || doc->spacePercent>MAXSPACE) ERR(3);
	if (doc->srastral<0 || doc->srastral>MAXRASTRAL) ERR(4);
	if (doc->altsrastral<1 || doc->altsrastral>MAXRASTRAL) ERR(5);
	if (doc->tempo<MIN_BPM || doc->tempo>MAX_BPM) ERR(6);
	if (doc->channel<1 || doc->channel>MAXCHANNEL) ERR(7);
	if (doc->velocity<-127 || doc->velocity>127) ERR(8);

	//if (doc->headerStrOffset<?? || doc->headerStrOffset>??) ERR(9);
	//if (doc->footerStrOffset<?? || doc->footerStrOffset>??) ERR(10);
	
	if (doc->dIndentOther<0 || doc->dIndentOther>in2d(5)) ERR(11);
	if (doc->firstNames<0 || doc->firstNames>MAX_NAMES_TYPE) ERR(12);
	if (doc->otherNames<0 || doc->otherNames>MAX_NAMES_TYPE) ERR(13);
	if (doc->lastGlobalFont<FONT_THISITEMONLY || doc->lastGlobalFont>MAX_FONTSTYLENUM) ERR(14);
	if (doc->firstMNNumber<0 || doc->firstMNNumber>MAX_FIRSTMEASNUM) ERR(15);
	if (doc->nfontsUsed<1 || doc->nfontsUsed>MAX_SCOREFONTS) ERR(16);
	if (doc->magnify<MIN_MAGNIFY || doc->magnify>MAX_MAGNIFY) ERR(17);
	if (doc->selStaff<-1 || doc->selStaff>doc->nstaves) ERR(18);
	if (doc->currentSystem<1 || doc->currentSystem>doc->nsystems) ERR(19);
	if (doc->spaceTable<0 || doc->spaceTable>32767) ERR(20);
	if (doc->htight<MINSPACE || doc->htight>MAXSPACE) ERR(21);
	if (doc->lookVoice<-1 || doc->lookVoice>MAXVOICES) ERR(22);
	if (doc->ledgerYSp<0 || doc->ledgerYSp>40) ERR(23);
	if (doc->deflamTime<1 || doc->deflamTime>1000) ERR(24);
	if (doc->dIndentFirst<0 || doc->dIndentFirst>in2d(5)) ERR(25);
	
	if (nerr==0) {
		LogPrintf(LOG_NOTICE, "No errors found.  (CheckScoreHdr)\n");
		return True;
	}
	else {
		if (!DETAIL_SHOW) DisplayScoreHdr(4, doc);
		sprintf(strBuf, "%d error(s) found.", nerr);
		CParamText(strBuf, "", "", "");
		LogPrintf(LOG_ERR, " %d error(s) found (first bad field is no. %d).  (CheckScoreHdr)\n",
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
				lastType;
	long		count, stringPoolSize,
				fileTime;
	Boolean		fileIsOpen, scoreHdrOK;
	OMSSignature omsDevHdr;
	long		fmsDevHdr;
	long		omsBufCount, omsDevSize;
	short		i, nDocErr;
	FInfo		fInfo;
	FSSpec 		fsSpec;
	long		cmHdr, cmBufCount, cmDevSize;
	FSSpec		*pfsSpecMidiMap;
	char		versionCString[5];
	DocumentN105 aDocN105;

	WaitCursor();

	fileIsOpen = False;
	
	fsSpec = *pfsSpec;
	errCode = FSpOpenDF(&fsSpec, fsRdWrPerm, &refNum );			/* Open the file */
	if (errCode == fLckdErr || errCode == permErr) {
		doc->readOnly = True;
		errCode = FSpOpenDF (&fsSpec, fsRdPerm, &refNum );		/* Try again - open the file read-only */
	}
	if (errCode) { errInfo = OPENcall; goto Error; }
	fileIsOpen = True;

	count = sizeof(version);									/* Read & check file version code */
	errCode = FSRead(refNum, &count, &version);
	FIX_END(version);
	if (errCode) { errInfo = VERSIONobj; goto Error;}
	*fileVersion = version;
	MacTypeToString(version, versionCString);
	
	/* If user has the secret keys down, pretend file is in current version. */
	
	if (ShiftKeyDown() && OptionKeyDown() && CmdKeyDown() && ControlKeyDown()) {
		LogPrintf(LOG_NOTICE, "IGNORING FILE'S VERSION CODE '%s'.\n", versionCString);
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
	if (version!=THIS_FILE_VERSION) LogPrintf(LOG_NOTICE, "CONVERTING VERSION '%s' FILE.\n", versionCString);

//#define DIFF(addr1, addr2)	((long)(&addr1)-(long)(&addr2))
//if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "Offset of aDocN105.comment[0]=%lu, spacePercent=%lu, fillerMB=%lu, nFontRecords=%lu, nfontsUsed=%lu, yBetweenSys=%lu\n",
//DIFF(aDocN105.comment[0], aDocN105.headL), DIFF(aDocN105.spacePercent, aDocN105.headL), DIFF(aDocN105.fillerMB, aDocN105.headL), DIFF(aDocN105.nFontRecords, aDocN105.headL),
//DIFF(aDocN105.nfontsUsed, aDocN105.headL), DIFF(aDocN105.yBetweenSys, aDocN105.headL));

	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "Header size for Document=%ld, for Score=%ld, for N105 Score=%ld\n",
		sizeof(DOCUMENTHDR), sizeof(SCOREHEADER), sizeof(SCOREHEADER_N105));

	count = sizeof(fileTime);									/* Time file was written */
	errCode = FSRead(refNum, &count, &fileTime);
	FIX_END(fileTime);
	if (errCode) { errInfo = VERSIONobj; goto Error; }
		
	/* Read and, if necessary, convert Document (i.e., Sheets) and Score headers. */
	
	switch(version) {
		case 'N104':
		case 'N105':
			count = sizeof(DOCUMENTHDR);
			errCode = FSRead(refNum, &count, &aDocN105.origin);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			ConvertDocumentHeader(doc, &aDocN105);
			if (DETAIL_SHOW) DisplayDocumentHdr(1, doc);
			
			count = sizeof(SCOREHEADER_N105);
			errCode = FSRead(refNum, &count, &aDocN105.headL);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			scoreHdrOK = ConvertScoreHeader(doc, &aDocN105);
			if (DETAIL_SHOW) DisplayScoreHdr(1, doc);
			if (!scoreHdrOK) {
				sprintf(strBuf, "%d error(s) found in converting Score header.", nDocErr);
				CParamText(strBuf, "", "", "");
				LogPrintf(LOG_ERR, "%d error(s) found in converting Score header.  (OpenFile)\n", nDocErr);
				goto HeaderError;
			}
			break;
		
		default:
			;
	}

	EndianFixDocumentHdr(doc);
	if (DETAIL_SHOW) DisplayDocumentHdr(2, doc);
	LogPrintf(LOG_NOTICE, "Checking Document header: ");
	if (CheckDocumentHdr(doc))
		LogPrintf(LOG_NOTICE, "No errors found.  (OpenFile)\n");
	else {
		if (!DETAIL_SHOW) DisplayDocumentHdr(3, doc);
		sprintf(strBuf, "%d error(s) found in Document header.", nDocErr);
		CParamText(strBuf, "", "", "");
		goto HeaderError;
	}
	
//DisplayScoreHdr(2, doc);		// ??TEMPORARY, TO DEBUG INTEL VERSION!!!!
	EndianFixScoreHdr(doc);
	if (DETAIL_SHOW) DisplayScoreHdr(3, doc);
	LogPrintf(LOG_NOTICE, "Checking Score header: ");
	if (CheckScoreHdr(doc))
		LogPrintf(LOG_NOTICE, "No errors found.  (OpenFile)\n");
	else {
		if (!DETAIL_SHOW) DisplayScoreHdr(3, doc);
		sprintf(strBuf, "%d error(s) found in Score header.", nDocErr);
		CParamText(strBuf, "", "", "");
		goto HeaderError;
	}

	count = sizeof(lastType);
	errCode = FSRead(refNum, &count, &lastType);
	if (errCode) { errInfo = HEADERobj; goto Error; }
	FIX_END(lastType);

	if (lastType!=LASTtype) {
		LogPrintf(LOG_ERR, "LAST OBJECT TYPE=%d BUT SHOULD BE %d.\n", lastType, LASTtype);	
		errCode = LASTTYPE_ERR;
		errInfo = HEADERobj;
		goto Error;
	}

	/* Read the string pool. */

	count = sizeof(stringPoolSize);
	errCode = FSRead(refNum, &count, &stringPoolSize);
	if (errCode) { errInfo = STRINGobj; goto Error; }
	FIX_END(stringPoolSize);
	LogPrintf(LOG_INFO, "stringPoolSize=%ld  (OpenFile)\n", stringPoolSize);
	if (doc->stringPool) DisposeStringPool(doc->stringPool);
	
	/* Allocate from the StringManager, not NewHandle, in case StringManager is tracking
	   its allocations. Then, since we're going to overwrite everything with stuff from
	   file below, we can resize it to what it was when saved. */
	
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
		AlwaysErrMsg("The string pool is probably bad (code=%ld).  (OpenFile)", (long)strPoolErrCode);
		errInfo = STRINGobj; goto Error;
	}
	
	errCode = FSpGetFInfo(&fsSpec, &fInfo);
	if (errCode!=noErr) { errInfo = INFOcall; goto Error; }
	
	/* Read the subobject heaps and the object heap in the rest of the file, and if
	   necessary, convert them to the current format. */
	
	errCode = ReadHeaps(doc, refNum, version, fInfo.fdType);
	if (errCode) return errCode;

	/* An ancient comment here: "Be sure we have enough memory left for a maximum-size
	   segment and a bit more." Now we require a _lot_ more, though it may be pointless. */
	
	if (!PreflightMem(400)) { NoMoreMemory(); return LOWMEM_ERR; }
	
	ConvertObjContent(doc, version, fileTime);	/* Do any further conversion of old files needed */

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
	
	if (!IsDocPrintInfoInstalled(doc))
		InstallDocPrintInfo(doc);
	
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
(CREATEcall, OPENcall, etc.: see enum above), the type of object being read, or
some other additional information on the error. NB: If errCode==0, this will close
the file but not give an error message; I'm not sure that's a good idea.

Note that after a call to OpenError with fileIsOpen, you should not try to keep reading,
since the file will no longer be open! */

void OpenError(Boolean fileIsOpen,
					short refNum,
					short errCode,
					short errInfo)	/* More info: at what step error happened, type of obj being read, etc. */
{
	char aStr[256], fmtStr[256];
	StringHandle strHdl;
	short strNum;
	
	SysBeep(1);
	LogPrintf(LOG_ERR, "Can't open the file. errCode=%d errInfo=%d  (OpenError)\n", errCode, errInfo);
	if (fileIsOpen) FSClose(refNum);

	if (errCode!=0) {
		switch (errCode) {
			/*
			 * First handle our own codes that need special treatment.
			 */
			case BAD_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 7);			/* "file version is illegal" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case LOW_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 8);			/* "too old for this version of Nightingale" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case HI_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 9);			/* "newer than this version of Nightingale" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
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
		LogPrintf(LOG_WARNING, aStr); LogPrintf(LOG_WARNING, "\n");
		CParamText(aStr, "", "", "");
		StopInform(READ_ALRT);
	}
}
