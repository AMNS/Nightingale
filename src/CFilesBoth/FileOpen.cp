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
 * Copyright © 2018-21 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "FileConversion.h"			/* Must follow Nightingale.precomp.h! */
#include "CarbonPrinting.h"
#include "MidiMap.h"


/* Prototypes for internal functions: */

static void SetTimeStamps(Document *);

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

static void VisifyAllNRGRs(Document *doc);
static void VisifyAllNRGRs(Document *doc)
{
	LINK objL, aNoteRL, aGRNoteL;
	
	for (objL = doc->headL; objL!=doc->tailL; objL = RightLINK(objL)) {
		switch (ObjLType(objL)) {
			case SYNCtype:
				aNoteRL = FirstSubLINK(objL);
				for ( ; aNoteRL; aNoteRL = NextNOTEL(aNoteRL)) {
					NoteVIS(aNoteRL) = True;
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(objL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					GRNoteVIS(aGRNoteL) = True;
				}
				break;
			default:
				;
		}
	}
}

/* If any notes, rests, or grace notes are invisible, ask the user if they want them made
visible, and do what they want. This is because a bizarre bug in file conversion used to
occasionally make them invisible. Fixed in v. 6.0 B47, I believe, but let's leave just in
case, at least for now. --DAB, July 2023 */
	   
static Boolean CheckNRGRVisibilityOK(Document *doc);
static Boolean CheckNRGRVisibilityOK(Document *doc)
{
	LINK objL, aNoteRL, aGRNoteL;
	short invisNCount, invisRCount, invisGRCount, itemHit;
	char fmtStr[256];
	
	KludgeOS10p5LogDelay(True);					/* Avoid bug in OS 10.5/10.6 Console */
	invisNCount = invisRCount = invisGRCount = 0;
	for (objL = doc->headL; objL!=doc->tailL; objL = RightLINK(objL)) {
		switch (ObjLType(objL)) {
			case SYNCtype:
				aNoteRL = FirstSubLINK(objL);
				for ( ; aNoteRL; aNoteRL = NextNOTEL(aNoteRL)) {
					if (!NoteVIS(aNoteRL)) {
#define NoDEBUG_EMPTY_OBJRECT
#ifdef DEBUG_EMPTY_OBJRECT
						if (debugLevel[DBG_OPEN]!=0) {
							PANOTE aNoteR = GetPANOTE(aNoteRL);
							DSubobjDump(SYNCtype, (unsigned char *)aNoteR, 0, 0, True);
						}
#endif
						if (NoteREST(aNoteRL))	invisRCount++;
						else					invisNCount++;
					}
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(objL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
#ifdef DEBUG_EMPTY_OBJRECT
						if (debugLevel[DBG_OPEN]!=0) {
							PAGRNOTE aGRNote = GetPAGRNOTE(aGRNoteL);
							DSubobjDump(GRSYNCtype, (unsigned char *)aGRNote, 0, 0, True);
						}
#endif
					if (!GRNoteVIS(aGRNoteL)) invisGRCount++;
				}
				break;
			default:
				;
		}
	}

	if (invisNCount==0 && invisRCount==0 && invisGRCount==0) return True;
	
	GetIndCString(fmtStr, FILEIO_STRS, 21);			/* "%d notes, %d rest(s), and %d grace note(s)" */
	sprintf(strBuf, fmtStr, invisNCount, invisRCount, invisGRCount);
	CParamText(strBuf, "", "", "");
#define INVISIBLES_ALRT 520
	itemHit = CautionAdvise(INVISIBLES_ALRT);
	if (itemHit==OK) VisifyAllNRGRs(doc);
	
	return (invisNCount==0 && invisRCount==0 && invisGRCount==0);
}

/* Open and read in the specified file Nightingale score file. If there's an error,
normally (see comments in OpenError) give an error message, and return <errType>; else
return noErr (0). Also set *fileVersion to the Nightingale version that created the file.
We assume doc exists and has had standard fields initialized. FIXME: even though vRefNum
is a parameter, (routines called by) OpenFile assume the volume is already set! */

enum {
	LOW_VERSION_ERR=-999,
	HI_VERSION_ERR,
	HEADER_ERR,
	LASTTYPE_ERR,				/* Value for LASTtype in file is not we expect it to be */
	TOOMANYSTAVES_ERR,
	LOWMEM_ERR,
	BAD_VERSION_ERR,
	STROFFSET_ERR				/* headerStrOffset or footerStrOffset exceeds stringPoolSize */
};

/* Read and, if necessary, convert Document (i.e., Sheets) and Score headers; leave the
file positioned at the end of the Score header, and return the <errType> value returned
by FSRead. */

static short ReadHeaders(Document *doc, short refNum, short *pErrInfo);
static short ReadHeaders(Document *doc, short refNum, short *pErrInfo)
{
	short errType, errInfo, nErr, firstErr;
	long count, fPos;
	DocumentN105 aDocN105;
		
	switch(version) {
		case 'N105':
			count = sizeof(DOCUMENTHDR);
			errType = FSRead(refNum, &count, &aDocN105.origin);
			if (errType) { errInfo = HEADERobj; goto Error; }
			ConvertDocumentHeader(doc, &aDocN105);
			if (DETAIL_SHOW) DisplayDocumentHdr(1, doc);
			
			count = sizeof(SCOREHEADER_N105);
			errType = FSRead(refNum, &count, &aDocN105.headL);
			if (errType) { errInfo = HEADERobj; goto Error; }
			ConvertScoreHeader(doc, &aDocN105);
			if (DETAIL_SHOW) DisplayScoreHdr(1, doc);
			break;
		
		default:
			count = sizeof(DOCUMENTHDR);
			errType = FSRead(refNum, &count, &doc->origin);
			if (errType) { errInfo = HEADERobj; goto Error; }
			if (MORE_DETAIL_SHOW) { GetFPos(refNum, &fPos);  LogPrintf(LOG_DEBUG, "fPos(1)=%ld\n", fPos); }
			
			count = sizeof(SCOREHEADER);
			errType = FSRead(refNum, &count, &doc->headL);
			if (errType) { errInfo = HEADERobj; goto Error; }
			if (MORE_DETAIL_SHOW) { GetFPos(refNum, &fPos);  LogPrintf(LOG_DEBUG, "fPos(2)=%ld\n", fPos); }
	}

	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "Fixing file headers for CPU's Endian property...  (ReadHeaders)\n");	
	EndianFixDocumentHdr(doc);
	if (DETAIL_SHOW) DisplayDocumentHdr(2, doc);
	LogPrintf(LOG_NOTICE, "Checking Document header: ");
	nErr = CheckDocumentHdr(doc, &firstErr);
	if (nErr==0) {
		LogPrintf(LOG_NOTICE, "No errors found.  (ReadHeaders)\n");
	}
	else {
		if (!DETAIL_SHOW) DisplayDocumentHdr(3, doc);
		LogPrintf(LOG_ERR, " %d ERROR(S) FOUND (first bad field is no. %d).  (ReadHeaders)\n",
					nErr, firstErr);
		sprintf(strBuf, "%d", nErr);
		CParamText(strBuf, "", "", "");
		short itemHit = CautionAlert(504, NULL);
		if (itemHit == Cancel) goto HeaderError;
	}
	
	EndianFixScoreHdr(doc);
	if (DETAIL_SHOW) DisplayScoreHdr(3, doc);
	LogPrintf(LOG_NOTICE, "Checking Score header: ");
	nErr = CheckScoreHdr(doc, &firstErr);
	if (nErr==0) {
		LogPrintf(LOG_NOTICE, "No errors found.  (ReadHeaders)\n");
	}
	else {
		if (!DETAIL_SHOW) DisplayScoreHdr(3, doc);
		LogPrintf(LOG_ERR, " %d ERROR(S) FOUND (first bad field is no. %d).  (ReadHeaders)\n",
					nErr, firstErr);
		sprintf(strBuf, "%d", nErr);
		CParamText(strBuf, "", "", "");
		short itemHit = CautionAlert(506, NULL);
		if (itemHit == Cancel) goto HeaderError;
	}
	
	return noErr;

HeaderError:
	errType = HEADER_ERR;
	errInfo = 0;
	/* drop through */
Error:
	*pErrInfo = errInfo;
	return errType;
}


#include <ctype.h>

short OpenFile(Document *doc, unsigned char *filename, short vRefNum, FSSpec *pfsSpec,
																	long *fileVersion)
{
	short		errType, refNum, strPoolErrCode;
	short 		errInfo=0,				/* Type of object being read or other info on error */
				lastType;
	long		count, stringPoolSize,
				fileTime, fPos;
	Boolean		fileIsOpen;
	FInfo		fInfo;
	FSSpec 		fsSpec;
	long		cmHdr, cmBufCount, cmDevSize;
	FSSpec		*pfsSpecMidiMap;
	char		versionCString[5];

	WaitCursor();

	fileIsOpen = False;
	
	fsSpec = *pfsSpec;
	errType = FSpOpenDF(&fsSpec, fsRdWrPerm, &refNum);			/* Open the file */
	if (errType == fLckdErr || errType == permErr) {
		doc->readOnly = True;
		errType = FSpOpenDF(&fsSpec, fsRdPerm, &refNum);		/* Try again - open the file read-only */
	}
	if (errType) { errInfo = OPENcall;  goto Error; }
	fileIsOpen = True;

	count = sizeof(version);									/* Read & check file version code */
	errType = FSRead(refNum, &count, &version);
	FIX_END(version);
	if (errType) { errInfo = VERSIONobj;  goto Error;}
	*fileVersion = version;
	MacTypeToString(version, versionCString);
	
#ifdef NOT_NOW
	/* If user has the secret keys down, pretend file is in current version. This is
	   almost certainly a waste of time if the file format has changed more than a
	   little bit. As of this writing, the current format is 'N106', which is _far_
	   different from 'N105'; draw your own conclusions! */
	
	if (DETAIL_SHOW && !OptionKeyDown() && CmdKeyDown()) {
		LogPrintf(LOG_NOTICE, "IGNORING FILE'S VERSION CODE '%s'.  (OpenFile)\n", versionCString);
		GetIndCString(strBuf, FILEIO_STRS, 6);					/* "IGNORING FILE'S VERSION CODE!" */
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
		version = THIS_FILE_VERSION;
	}
#endif

	if (versionCString[0]!='N') { errType = BAD_VERSION_ERR; goto Error; }
	if ( !isdigit(versionCString[1])
			|| !isdigit(versionCString[2])
			|| !isdigit(versionCString[3]) )
		 { errType = BAD_VERSION_ERR; goto Error; }

	if (version<FIRST_FILE_VERSION) { errType = LOW_VERSION_ERR; goto Error; }
	if (version>THIS_FILE_VERSION) { errType = HI_VERSION_ERR; goto Error; }
	if (version!=THIS_FILE_VERSION) {
#if TARGET_RT_LITTLE_ENDIAN
		LogPrintf(LOG_NOTICE, "On this computer architecture, Nightingale can't open a '%s' format file.  (OpenFile)\n", versionCString);
		errType = LOW_VERSION_ERR;
		goto Error;
#else
		LogPrintf(LOG_NOTICE, "CONVERTING VERSION '%s' FILE.  (OpenFile)\n", versionCString);
#endif
	}
	
	if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "Header size for Document=%ld, for Score=%ld, for N105 Score=%ld.  (OpenFile)\n",
		sizeof(DOCUMENTHDR), sizeof(SCOREHEADER), sizeof(SCOREHEADER_N105));

	count = sizeof(fileTime);									/* Time file was written */
	errType = FSRead(refNum, &count, &fileTime);
	FIX_END(fileTime);
	if (errType) { errInfo = VERSIONobj; goto Error; }
		
	errType = ReadHeaders(doc, refNum, &errInfo);
	if (errType!=noErr) goto Error;

	count = sizeof(lastType);
	errType = FSRead(refNum, &count, &lastType);
	if (errType) { errInfo = HEADERobj; goto Error; }
	FIX_END(lastType);

	GetFPos(refNum, &fPos);
	if (MORE_DETAIL_SHOW) { LogPrintf(LOG_DEBUG, "lastType=%d fPos(3)=%ld\n", lastType, fPos); }
	if (lastType!=LASTtype) {
		LogPrintf(LOG_ERR, "LAST OBJECT TYPE IS %d BUT SHOULD BE %d.  The file is unreadable. fPos=%ld (OpenFile)\n",
					lastType, LASTtype, fPos);
		errType = LASTTYPE_ERR;
		errInfo = HEADERobj;
		goto Error;
	}

	/* Read and check string pool size and header/footer strings, and read string pool. */

	count = sizeof(stringPoolSize);
	errType = FSRead(refNum, &count, &stringPoolSize);
	if (errType) { errInfo = STRINGobj; goto Error; }
	FIX_END(stringPoolSize);
	LogPrintf(LOG_INFO, "stringPoolSize=%ld  (OpenFile)\n", stringPoolSize);
	if (doc->headerStrOffset > stringPoolSize) {
		LogPrintf(LOG_WARNING, "stringPoolSize is only %ld but headerStrOffset is %ld; setting it to 0.  (OpenFile)\n",
					stringPoolSize, doc->headerStrOffset);
		doc->headerStrOffset = 0;
	}
	if (doc->footerStrOffset > stringPoolSize) {
		LogPrintf(LOG_WARNING, "stringPoolSize is only %ld but footerStrOffset is %ld; setting it to 0.  (OpenFile)\n",
					stringPoolSize, doc->footerStrOffset);
		doc->footerStrOffset = 0;
	}
	if (doc->stringPool) DisposeStringPool(doc->stringPool);
	
	/* Allocate via the StringManager, not NewHandle, in case StringManager is tracking
	   its allocations. Then, since we're going to overwrite everything with stuff from
	   the file below, we can resize it to what it was when saved. */
	
	doc->stringPool = NewStringPool();
	if (doc->stringPool == NULL) { errInfo = STRINGobj; goto Error; }
	SetHandleSize((Handle)doc->stringPool, stringPoolSize);
	
	HLock((Handle)doc->stringPool);
	errType = FSRead(refNum, &stringPoolSize, *doc->stringPool);
	if (errType) { errInfo = STRINGobj; goto Error; }
	HUnlock((Handle)doc->stringPool);
	SetStringPool(doc->stringPool);
	EndianFixStringPool(doc->stringPool);
	if (DETAIL_SHOW) DisplayStringPool(doc->stringPool);
	strPoolErrCode = StringPoolProblem(doc->stringPool);
	if (strPoolErrCode!=0) {
		AlwaysErrMsg("The file appears to be corrupted: string pool is probably bad (code=%ld).  (OpenFile)\n",
						(long)strPoolErrCode);
		errInfo = STRINGobj; goto Error;
	}
	
	errType = FSpGetFInfo(&fsSpec, &fInfo);
	if (errType!=noErr) { errInfo = INFOcall; goto Error; }
	
	/* Read the subobject heaps and the object heap from the rest of the file (handling
	   the CPU's Endian property); then, if necessary, convert the heaps to the current
	   object-list format. */
	
	errType = ReadHeaps(doc, refNum, version, fInfo.fdType);
	if (errType!=noErr) { errInfo = READHEAPScall; goto Error; }

	/* An ancient comment here: "Be sure we have enough memory left for a maximum-size
	   segment and a bit more." Now we require a _lot_ more, though it may be pointless. */
	
	if (!PreflightMem(800)) { NoMoreMemory(); return LOWMEM_ERR; }
	
	/* Do any further conversion needed of both the main and the Master Page object lists
	   in old files. If any notes, rests, or grace notes are invisible, ask the user if
	   they want them made visible, and do what they want. Why? See comments on
	   CheckNRGRVisibilityOK(). */
	   
	if (version=='N105') {
		(void)ConvertObjectList(doc, version, fileTime, False);
		(void)ConvertObjectList(doc, version, fileTime, True);
#ifdef DEBUG_EMPTY_OBJRECT
// The bug leading to the need for CheckNRGRVisibilityOK also caused empty objRects, so...
for (LINK objL = 8; objL!=doc->tailL; objL = RightLINK(objL)) {
	if (ObjLType(objL)==SYNCtype)
		DisplayObject(doc, objL, 700+ObjLType(objL), True, True, True);
}
#endif
		(void)CheckNRGRVisibilityOK(doc);
	}

	if (DETAIL_SHOW) DObjDump("OpenFile", 1, (MORE_DETAIL_SHOW? 20 : 4));

#ifdef DEBUG_EMPTY_OBJRECT
if (debugLevel[DBG_OPEN]!=0) {
	for (LINK objL = doc->headL; objL!=doc->tailL; objL = RightLINK(objL)) {
		LINK aNoteRL;
		switch (ObjLType(objL)) {
			case SYNCtype:
				aNoteRL = FirstSubLINK(objL);
				for ( ; aNoteRL; aNoteRL = NextNOTEL(aNoteRL)) {
					PANOTE aNoteR = GetPANOTE(aNoteRL);
					DSubobjDump(SYNCtype, (unsigned char *)aNoteR, 0, 0, True);
				}
		}
	}
}
#endif

	Pstrcpy(doc->name, filename);				/* Remember filename and vol refnum after scoreHead is overwritten */
	doc->vrefnum = vRefNum;
	doc->changed = False;
	doc->saved = True;							/* Has to be, since we just opened it! */
	doc->named = True;							/* Has to have a name, since we just opened it! */

	ModifyScore(doc, fileTime);					/* Do any hacking needed: ordinarily none */

	/* Read the CoreMIDI device data. */

	cmBufCount = sizeof(long);
	errType = FSRead(refNum, &cmBufCount, &cmHdr);
	if (!errType && cmHdr == 'cmdi') {
		cmBufCount = sizeof(long);
		errType = FSRead(refNum, &cmBufCount, &cmDevSize);
		if (errType) { errInfo = CM_Call; goto Error; }
		if (cmDevSize!=(MAXSTAVES+1)*sizeof(MIDIUniqueID)) return errType;
		errType = FSRead(refNum, &cmDevSize, &(doc->cmPartDeviceList[0]));
		if (errType) { errInfo = CM_Call; goto Error; }
	}
	doc->cmInputDevice = config.cmDfltInputDev;
	LogPrintf(LOG_INFO, "cmInputDevice=%d cmPartDeviceList[]=%d,%d  (OpenFile)\n",
				doc->cmInputDevice, doc->cmPartDeviceList[1], doc->cmPartDeviceList[2]);

	errType = FSClose(refNum);
	if (errType) { errInfo = CLOSEcall; goto Error; }
	
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

	if (ScreenPagesExceedView(doc)) CautionInform(MANYPAGES_ALRT);

	ArrowCursor();	
	return 0;

Error:
//LogPrintf(LOG_DEBUG, "OpenFile: errType=%d\n", errType);
	OpenError(fileIsOpen, refNum, errType, errInfo);
	return errType;
}


/* ------------------------------------------------------------------------- OpenError -- */
/* Handle errors occurring while reading a file. <fileIsOpen> indicates  whether or
not the file was open when the error occurred; if so, OpenError closes it. <errType>
is either an error code return by the file system manager or one of our own codes
(see enum above). <errInfo> indicates at what step of the process the error happened
(CREATEcall, OPENcall, etc.: see enum above), the type of object being read, or some
other additional information on the error.

Note that after a call to OpenError with fileIsOpen, you should not try to keep reading,
since the file will no longer be open! */

void OpenError(Boolean fileIsOpen,
					short refNum,	/* 0 = file's refNum is unknown */
					short errType,
					short errInfo)	/* More info: at what step error happened, type of obj being read, etc. */
{
	char aStr[256], fmtStr[256];
	StringHandle strHdl;
	short strNum;
	
	SysBeep(1);
	LogPrintf(LOG_ERR, "CAN'T OPEN THE FILE. errType=%d errInfo=%d  (OpenError)\n", errType, errInfo);
	if (fileIsOpen && refNum!=0) FSClose(refNum);

	switch (errType) {
	
		/* First handle our own codes that need special treatment. */
		
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
			/* We expect descriptions of the common errors stored by code (negative values
			   for system errors; positive ones for our own I/O errors) in individual
			   'STR ' resources. If we find one for this error, print it, else just print
			   the raw code. */
			   
			strHdl = GetString(errType);
			if (strHdl) {
				Pstrcpy((unsigned char *)strBuf, (unsigned char *)*strHdl);
				PToCString((unsigned char *)strBuf);
				strNum = (errInfo>0? 15 : 16);	/* "%s (heap object type=%d)." : "%s (error info=%d)." */
				GetIndCString(fmtStr, FILEIO_STRS, strNum);
				sprintf(aStr, fmtStr, strBuf, errInfo);
			}
			else {
				strNum = (errInfo>0? 17 : 18);	/* "Error ID=%d (heap object type=%d)." : "Error ID=%d (error info=%d)." */
				GetIndCString(fmtStr, FILEIO_STRS, strNum);
				sprintf(aStr, fmtStr, errType, errInfo);
			}
	}
	LogPrintf(LOG_WARNING, aStr); LogPrintf(LOG_WARNING, "  (OpenError)\n");
	CParamText(aStr, "", "", "");
	StopInform(READ_ALRT);
}
