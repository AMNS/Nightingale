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

static void SetTimeStamps(Document *);


/* Codes for object types being read/written or for non-read/write call when an I/O
error occurs. Note that all are negative. See file.h for additional, positive,
codes. FIXME: Move these #defines to file.h! */

#define	HEADERobj -999
#define VERSIONobj -998
#define SIZEobj -997
#define CREATEcall -996
#define OPENcall -995
#define CLOSEcall -994
#define DELETEcall -993
#define RENAMEcall -992
#define WRITEcall -991
#define STRINGobj -990
#define INFOcall -989
#define SETVOLcall -988
#define BACKUPcall -987
#define MAKEFSSPECcall -986
#define NENTRIESerr -899
#define READHEAPScall -898


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
	if (version!=THIS_FILE_VERSION) {
#if TARGET_RT_LITTLE_ENDIAN
		LogPrintf(LOG_NOTICE, "On this computer architecture, Nightingale can't open a '%s' format file.  (OpenFile)\n", versionCString);
		errCode = LOW_VERSION_ERR;
		goto Error;
#else
		LogPrintf(LOG_NOTICE, "CONVERTING VERSION '%s' FILE.  (OpenFile)\n", versionCString);
#endif
	}
	
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
		ConvertObjSubobjs(doc, version, fileTime, False);
		ConvertObjSubobjs(doc, version, fileTime, True);
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
