/******************************************************************************************
*	FILE:	FileSave.c
*	PROJ:	Nightingale
*	DESC:	Output of normal Nightingale files
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018-20 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "CarbonPrinting.h"
#include "FileUtils.h"
#include "MidiMap.h"


/* ----------------------------------------------------- Helper functions for SaveFile -- */

enum {
	SF_SafeSave,
	SF_Replace,
	SF_SaveAs,
	SF_Cancel
};

static long GetOldFileSize(Document *doc);
static long GetFileSize(Document *doc,long vAlBlkSize);
static long GetFreeSpace(Document *doc,long *vAlBlkSize);
static short AskSaveType(Boolean canContinue);
static short GetSaveType(Document *doc,Boolean saveAs);
static short WriteFile(Document *doc,short refNum);
static Boolean SFChkScoreOK(Document *doc);
static Boolean GetOutputFile(Document *doc);

/* Return, in bytes, the physical size of the file in which doc was previously saved:
physical length of data fork + physical length of resource fork.

FIXME: This does not quite give the information needed to tell how much disk space the
file takes: the relevant thing is numbers of allocation blocks, not bytes, and that
number must be computed independently for the data fork and resource fork. It looks
like PreflightFileCopySpace in FileCopy.c in Apple DTS' More Files package does a
much better job. */

static long GetOldFileSize(Document *doc)
{
	HParamBlockRec fInfo;
	short err;
	long fileSize;

	fInfo.fileParam.ioCompletion = NULL;
	fInfo.fileParam.ioFVersNum = 0;
	fInfo.fileParam.ioResult = noErr;
	fInfo.fileParam.ioVRefNum = doc->vrefnum;
	fInfo.fileParam.ioDirID = doc->fsSpec.parID;
	fInfo.fileParam.ioFDirIndex = 0;							/* ioNamePtr is INPUT, not output */
	fInfo.fileParam.ioNamePtr = (unsigned char *)doc->name;
	PBHGetFInfo(&fInfo,False);
	
	err = fInfo.fileParam.ioResult;
	if (err==noErr) {
		fileSize = fInfo.fileParam.ioFlPyLen;
		fileSize += fInfo.fileParam.ioFlRPyLen;
		return fileSize;
	} 

	return (long)err;
}

/* Get the physical size of the file as it will be saved from memory: the total size
of all objects written to disk, rounded up to sector size, plus the total size of all
resources saved, rounded up to sector size. */

static long GetFileSize(Document *doc, long vAlBlkSize)
{
	unsigned short objCount[LASTtype], i, nHeaps;
	long fileSize=0L, strHdlSize, blkMod, rSize;
	LINK pL;
	Handle stringHdl;

	/* Get the total number of objects of each type and the number of note modifiers
	   in both the main and the Master Page object lists. */

	CountSubobjsByHeap(doc, objCount, True);

	for (pL = doc->headL; pL!=RightLINK(doc->tailL); pL = RightLINK(pL))
		fileSize += objLength[ObjLType(pL)];

	for (pL = doc->masterHeadL; pL!=RightLINK(doc->masterTailL); pL = RightLINK(pL))
		fileSize += objLength[ObjLType(pL)];

	for (i=FIRSTtype; i<LASTtype-1; i++) {
		fileSize += subObjLength[i]*objCount[i];
	}
	
	fileSize += 2*sizeof(long);						/* version & file time */	

	fileSize += sizeof(DOCUMENTHDR);
	
	fileSize += sizeof(SCOREHEADER);
	
	fileSize += sizeof((short)LASTtype);

	stringHdl = (Handle)GetStringPool();
	strHdlSize = GetHandleSize(stringHdl);

	fileSize += sizeof(strHdlSize);
	fileSize += strHdlSize;
	
	nHeaps = LASTtype-FIRSTtype+1;

	/* Total number of objects of type heapIndex plus sizeAllObjects, 1 for each heap. */

	fileSize += (sizeof(short)+sizeof(long))*nHeaps;
	
	/* The HEAP struct header, 1 for each heap. */
	
	fileSize += sizeof(HEAP)*nHeaps;

	fileSize += sizeof(long);							/* end marker */
	
	/* Round up to the next higher multiple of allocation blk size */

	blkMod = fileSize % vAlBlkSize;
	if (blkMod)
		fileSize += vAlBlkSize - blkMod;
	
	/* Add size of file's resource fork here, rounded up to the next higher multiple
	   of allocation block size. */
	
	if (doc->hPrint) {
		rSize = GetHandleSize((Handle)doc->hPrint);

		fileSize += rSize;

		blkMod = rSize % vAlBlkSize;
		if (blkMod)
			fileSize += vAlBlkSize - blkMod;
	}
	
	return fileSize;
}

/* Get the amount of free space on the volume: number of free blocks * block size. */

static long GetFreeSpace(Document *doc, long *vAlBlkSize)
{
	HParamBlockRec vInfo;
	short err;
	long vFreeSpace;

	vInfo.volumeParam.ioCompletion = NULL;
	vInfo.volumeParam.ioResult = noErr;
	vInfo.volumeParam.ioVRefNum = doc->vrefnum;
	vInfo.volumeParam.ioVolIndex = 0;					/* Use ioVRefNum alone to find vol. */
	vInfo.volumeParam.ioNamePtr = NULL;
	PBHGetVInfo(&vInfo,False);
	
	err = vInfo.volumeParam.ioResult;
	if (err==noErr) {
		vFreeSpace = ((unsigned short)vInfo.volumeParam.ioVFrBlk) * 
                                   vInfo.volumeParam.ioVAlBlkSiz;
		*vAlBlkSize = vInfo.volumeParam.ioVAlBlkSiz;
		return vFreeSpace;
	} 

	return (long)err;
}

/* Tell the user we can't do a safe save and ask what they want to do. Two different
alerts, one if we can save on the specified volume at all, one if not. Coded so that
the two alerts must have identical item nos. for Save As and Cancel. */

static short AskSaveType(Boolean canContinue)
{
	short saveType;

	if (canContinue)
		saveType = StopAlert(SAFESAVE_ALRT, NULL);
	else
		saveType = StopAlert(SAFESAVE1_ALRT, NULL);

	if (saveType==1) return SF_SaveAs;
	if (saveType==3) return SF_Replace;
	return SF_Cancel;
}

/* Check to see if a file can be saved safely, saved only unsafely, or not saved at
all, onto the current volume. If it cannot be saved safely, ask user what to do. */

static short GetSaveType(Document *doc, Boolean saveAs)
{
	Boolean canContinue;
	long fileSize, freeSpace, oldFileSize, vAlBlkSize=-9999;
	
	/* If doc->new, there's no previous document to protect from the save operation. If
	   saveAs, the previous doc will not be replaced, but a new one will be created. */

	if (doc->docNew || saveAs)
		oldFileSize = 0L;
	else {
	
		/* Get the amount of space physically allocated to the old file, and the
		   amount of space available on doc's volume. Return value < 0 indicates
		   FS Error: should forget safe saving. FIXME: GetOldFileSize() doesn't do a
		   very good job: see comments on it. */
	
		oldFileSize = GetOldFileSize(doc);
		if (oldFileSize<0L) {
			GetIndCString(strBuf, FILEIO_STRS, 13);    /* "Can't get the existing file's size." */
			CParamText(strBuf, "", "", "");
			StopInform(SAFESAVEINFO_ALRT);
			return AskSaveType(True);					/* Let them try to save on this vol. anyway */
		}
	}

	freeSpace = GetFreeSpace(doc, &vAlBlkSize);
	if (freeSpace<0L) {
		GetIndCString(strBuf, FILEIO_STRS, 14);			/* "Can't get the disk free space." */
		CParamText(strBuf, "", "", "");
		StopInform(SAFESAVEINFO_ALRT);
		return AskSaveType(True);						/* Let them try to save on this vol. anyway */
	}

	fileSize = GetFileSize(doc, vAlBlkSize);

	/* If we have enough space to save safely, we're done (although if the doc is new
	   or it's a Save As, we tell the caller they don't _need_ to do a safe save). If
	   not, ask the user what to do. If canContinue, they can still replace the previous
	   file; otherwise, they can only Save As or Cancel. */

	if (fileSize <= freeSpace)
		return ((doc->docNew || saveAs)? SF_Replace : SF_SafeSave);
	
	canContinue = (fileSize <= freeSpace + oldFileSize);
	return AskSaveType(canContinue);
}

/* To avoid disasterous problems, mostly overwriting a valid file with a bad one, do
any validity checks we want on the doc about to be saved. Originally written to call
DCheckNEntries(), to check nEntries fields in object list, because of "Mackey's Disease",
which made it impossible to open some saved files. */

static Boolean SFChkScoreOK(Document */*doc*/)
{
	return True;
}

/* Actually write the file. Write header info, write the string pool, call WriteHeaps
to write data structure objects, and write the EOF marker. If any error is encountered,
return that error without continuing. Otherwise, return noErr. */

static short WriteFile(Document *doc, short refNum)
{
	short			errType, strPoolErrCode;
	short			lastType;
	long			count, blockSize, strHdlSizeFile, strHdlSizeInternal;
	unsigned long	fileTime;
	Handle			stringHdl;
	OMSSignature	omsDevHdr;
	long			omsDevSize, fmsDevHdr;
	long			cmDevSize, cmHdr;	
	unsigned long	version;							/* File version code read/written */
	Document		tempDoc;

	/* Write version code using possibly Endian-fixed (to make Big Endian) local copy. */
	
	version = THIS_FILE_VERSION;  FIX_END(version);
	count = sizeof(version);
	errType = FSWrite(refNum, &count, &version);
	if (errType) return VERSIONobj;

	/* Write current date and time using possibly Endian-fixed local copy. */
	
	GetDateTime(&fileTime);  FIX_END(fileTime);
	count = sizeof(fileTime);
	errType = FSWrite(refNum, &count, &fileTime);
	if (errType) return VERSIONobj;

	/* Write Document and Score headers using possibly Endian-fixed local copy. */

	if (DETAIL_SHOW) DisplayDocumentHdr(0, doc);
	if (DETAIL_SHOW) DisplayScoreHdr(0, doc);

	count = sizeof(tempDoc);
LogPrintf(LOG_DEBUG, "WriteFile1: count=%ld ->origin=%d,%d ->littleEndian=%u ->firstMNNumber=%d\n",
			count, doc->origin.v, doc->origin.h, (short)(doc->littleEndian), doc->firstMNNumber);
	BlockMove(doc, &tempDoc, count);

	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "Fixing file headers for CPU's Endian property...  (WriteFile)\n");	
	EndianFixDocumentHdr(&tempDoc);
	EndianFixScoreHdr(&tempDoc);
LogPrintf(LOG_DEBUG, "WriteFile3: .origin=%d,%d .littleEndian=%u .firstMNNumber=%d\n",
			tempDoc.origin.v, tempDoc.origin.h, (short)(tempDoc.littleEndian), tempDoc.firstMNNumber);

	count = sizeof(DOCUMENTHDR);
	errType = FSWrite(refNum, &count, &tempDoc.origin);
	if (errType) return HEADERobj;
	
	count = sizeof(SCOREHEADER);
	errType = FSWrite(refNum, &count, &tempDoc.headL);
	if (errType) return HEADERobj;
	
	/* Write LASTtype using possibly Endian-fixed local copy. */
	
	lastType = LASTtype;  FIX_END(lastType);
	count = sizeof(lastType);
	errType = FSWrite(refNum, &count, &lastType);
	if (errType) return HEADERobj;

	/* Write string pool size using possibly Endian-fixed local copy. Write string pool,
	   possibly Endian-fixed; if Endian-fixed, then undo by redoing it (since the operation is
	   its own inverse). */
	
	stringHdl = (Handle)GetStringPool();
	HLock(stringHdl);

	strHdlSizeInternal = strHdlSizeFile = GetHandleSize(stringHdl);
	if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "WriteFile: strHdlSizeInternal=%ld strHdlSizeFile=%ld\n",
								strHdlSizeInternal, strHdlSizeFile);
	FIX_END(strHdlSizeFile);								/* Convert to Big Endian if needed */
	count = sizeof(strHdlSizeFile);
	errType = FSWrite(refNum, &count, &strHdlSizeFile);
	if (errType) return STRINGobj;

	HUnlock((Handle)doc->stringPool);
	SetStringPool(doc->stringPool);
	EndianFixStringPool(doc->stringPool);					/* Convert to Big Endian if needed */

	errType = FSWrite(refNum, &strHdlSizeInternal, *stringHdl);
	HUnlock(stringHdl);
	if (errType) return STRINGobj;

	EndianFixStringPool(doc->stringPool);					/* Back to platform-specific Endian */
	if (DETAIL_SHOW) DisplayStringPool(doc->stringPool);
	strPoolErrCode = StringPoolProblem(doc->stringPool);
	if (strPoolErrCode!=0) {
		AlwaysErrMsg("The string pool is probably bad (code=%ld).  (WriteFile)\n",
						(long)strPoolErrCode);
		DisplayStringPool(doc->stringPool);
		return STRINGobj;
	}

	/* Write heaps. NB they must be Endian fixed before they're written, and fixed back
	   before continuing!  */
	
	if (DETAIL_SHOW) NObjDump("WriteFile", 1, (MORE_DETAIL_SHOW? 30 : 4));
	errType = WriteHeaps(doc, refNum);
	if (errType) return errType;

	/* Write info for OMS. FIXME: OMS is obsolete and this code should be removed! */

	count = sizeof(OMSSignature);
	omsDevHdr = 'devc';
	errType = FSWrite(refNum, &count, &omsDevHdr);
	if (errType) return SIZEobj;
	count = sizeof(long);
	omsDevSize = (MAXSTAVES+1) * sizeof(OMSUniqueID);
	errType = FSWrite(refNum, &count, &omsDevSize);
	if (errType) return SIZEobj;
	errType = FSWrite(refNum, &omsDevSize, &(doc->omsPartDeviceList[0]));
	if (errType) return SIZEobj;

	/* Write info for FreeMIDI input device:
		1) long having the value 'FMS_' (just a marker)
		2) fmsUniqueID (unsigned short) giving input device ID
		3) fmsDestinationMatch union giving info about input device
	   FIXME: FreeMIDI is obsolete and this code should be removed! */
			
	count = sizeof(long);
	fmsDevHdr = FreeMIDISelector;
	errType = FSWrite(refNum, &count, &fmsDevHdr);
	if (errType) return SIZEobj;
	count = sizeof(fmsUniqueID);
	errType = FSWrite(refNum, &count, &doc->fmsInputDevice);
	if (errType) return SIZEobj;
	count = sizeof(fmsDestinationMatch);
	errType = FSWrite(refNum, &count, &doc->fmsInputDestination);
	if (errType) return SIZEobj;
	
	/* Write info for CoreMIDI (file version >= 'N105') */
	
	count = sizeof(long);
	cmHdr = 'cmdi';
	errType = FSWrite(refNum, &count, &cmHdr);
	if (errType) return SIZEobj;
	count = sizeof(long);
	cmDevSize = (MAXSTAVES+1) * sizeof(MIDIUniqueID);
	errType = FSWrite(refNum, &count, &cmDevSize);
	if (errType) return SIZEobj;
	errType = FSWrite(refNum, &cmDevSize, &(doc->cmPartDeviceList[0]));
	if (errType) return SIZEobj;	

	blockSize = 0L;													/* mark end w/ 0x00000000 */
	count = sizeof(blockSize);
	errType = FSWrite(refNum, &count, &blockSize);
	if (errType) return SIZEobj;

	return noErr;
}

static Boolean GetOutputFile(Document *doc)
{
	Str255 name;  short vrefnum;
	OSErr result;
	FInfo fInfo;
	FSSpec fsSpec;
	NSClientData nscd;
	
	Pstrcpy(name,doc->name);
	if (GetOutputName(MiscStringsID, 3, name, &vrefnum, &nscd)) {
	
		fsSpec = nscd.nsFSSpec;
		//result = FSMakeFSSpec(vrefnum, 0, name, &fsSpec);
		
		//result = GetFInfo(name, vrefnum, &fInfo);
		//if (result == noErr)
			result = FSpGetFInfo (&fsSpec, &fInfo);
		
		/* This check and error message also appear in DoSaveAs. */
		if (result == noErr && fInfo.fdType != documentType) {
			GetIndCString(strBuf, FILEIO_STRS, 11);    /* "You can replace only files created by Nightingale." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return False;
		}

		//HSetVol(NULL, vrefnum, 0);
		result = HSetVol(NULL, fsSpec.vRefNum, fsSpec.parID);
		
		/* Save the file under this name */
		
		Pstrcpy(doc->name, name);
		doc->vrefnum = vrefnum;
		SetWTitle((WindowPtr)doc, name);
		doc->changed = False;
		doc->named = True;
		doc->readOnly = False;
	}
	
	return True;
}


/* -------------------------------------------------------------------------- SaveFile -- */
/*	Routine that actually saves the specifed file.  If there's an error, gives an
error message and returns <errType>; else returns 0 if successful, NRV_CANCEL if
operation is cancelled. */

#define TEMP_FILENAME "\p**NightTemp**"

/* Given a filename and a suffix for a variant of the file, append the suffix to the
filename, truncating if necessary to make the result a legal filename. NB: for
systems whose filenames have extensions, like MS Windows, we should probably replace
the extension, though even here some disagree. For other systems, like MacOS and UNIX,
it's really unclear what to do: this is particularly unfortunate because if a file
exists with the resulting name, the calling routine may be planning to overwrite it!
Return True if we can do the job without truncating the given filename, False if we
have to truncate (this is the more dangerous case). */

Boolean MakeVariantFilename(Str255 filename, char *suffix, Str255 bkpName);
Boolean MakeVariantFilename(
			Str255 filename,			/* Pascal string */
			char *suffix,				/* C string */
			Str255 bkpName				/* Pascal string */
			)
{
	short bkpNameLen;
	Boolean truncated = False;
	
	Pstrcpy(bkpName, filename);
	PToCString(bkpName);

	if (strlen((char *)bkpName)+strlen(suffix)>FILENAME_MAXLEN) {
		bkpNameLen = FILENAME_MAXLEN-strlen(suffix);
		bkpName[bkpNameLen] = '\0';
		truncated = True;
	}

	strcat((char *)bkpName, suffix);
	CToPString((char *)bkpName);
	
	return truncated;
}

static short RenameAsBackup(Str255 filename, short vRefNum, Str255 bkpName);
static short RenameAsBackup(Str255 filename, short vRefNum, Str255 bkpName)
{
	short errType;
	FSSpec fsSpec;
	FSSpec bkpFSSpec;

	MakeVariantFilename(filename, ".bkp", bkpName);
		
	errType = FSMakeFSSpec(vRefNum, 0, bkpName, &bkpFSSpec);
	if (errType && errType!=fnfErr)
		return errType;

	//errType = FSDelete(bkpName, vRefNum);						/* Delete old file */
	errType = FSpDelete(&bkpFSSpec);							/* Delete old file */
	if (errType && errType!=fnfErr)								/* Ignore "file not found" */
		return errType;

	errType = FSMakeFSSpec(vRefNum, 0, filename, &fsSpec);
	if (errType)
		return errType;

	//errType = Rename(filename,vRefNum,bkpName);
	errType =  FSpRename (&fsSpec, bkpName);

	return errType;
}


short SaveFile(
			Document *doc,
			Boolean saveAs			/* True if a Save As operation, False if a Save */
			)
{
	Str255			filename, bkpName;
	short			vRefNum;
	short			errType, refNum;
	short 			errInfo=noErr;				/* Type of object being read or other info on error */
	short			saveType;
	Boolean			fileIsOpen;
	const unsigned char	*tempName;
	ScriptCode		scriptCode = smRoman;
	FSSpec 			fsSpec;
	FSSpec 			tempFSSpec;
	
		
	if (!SFChkScoreOK(doc))
		{ errInfo = NENTRIESerr; goto Error; }

	Pstrcpy(filename,doc->name);
	vRefNum = doc->vrefnum;
	fsSpec = doc->fsSpec;
	fileIsOpen = False;

TryAgain:
	WaitCursor();
	saveType = GetSaveType(doc,saveAs);

	if (saveType==SF_Cancel) return NRV_CANCEL;					/* User cancelled or FSErr */

	if (saveType==SF_SafeSave) {
		/* Create and open a temporary file */
		tempName = TEMP_FILENAME;

		errType = FSMakeFSSpec(vRefNum, fsSpec.parID, tempName, &tempFSSpec);
		if (errType && errType!=fnfErr)
			{ errInfo = MAKEFSSPECcall; goto Error; }
		
		/* Without the following, if TEMP_FILENAME exists, Safe Save gives an error. */
		errType = FSpDelete(&tempFSSpec);						/* Delete any old temp file */
		if (errType && errType!=fnfErr)							/* Ignore "file not found" */
			{ errInfo = DELETEcall; goto Error; }

		errType = FSpCreate (&tempFSSpec, creatorType, documentType, scriptCode);
		if (errType) { errInfo = CREATEcall; goto Error; }
		
		errType = FSpOpenDF (&tempFSSpec, fsRdWrPerm, &refNum );	/* Open the temp file */
		if (errType) { errInfo = OPENcall; goto Error; }
	}
	else if (saveType==SF_Replace) {
		//errType = FSMakeFSSpec(vRefNum, 0, filename, &fsSpec);
		//if (errType && errType!=fnfErr)
		//	{ errInfo = MAKEFSSPECcall; goto Error; }
		fsSpec = doc->fsSpec;
		
		errType = FSpDelete(&fsSpec);							/* Delete old file */
		if (errType && errType!=fnfErr)							/* Ignore "file not found" */
			{ errInfo = DELETEcall; goto Error; }
			
		errType = FSpCreate (&fsSpec, creatorType, documentType, scriptCode);
		if (errType) { errInfo = CREATEcall; goto Error; }
		
		errType = FSpOpenDF (&fsSpec, fsRdWrPerm, &refNum );	/* Open the file */
		if (errType) { errInfo = OPENcall; goto Error; }
	}
	else if (saveType==SF_SaveAs) {
		if (!GetOutputFile(doc)) return NRV_CANCEL;				/* User cancelled or FSErr */

		/* The user may have just told us a new disk to save on, so start over. */
		
		saveAs = True;
		Pstrcpy(filename, doc->name);
		vRefNum = doc->vrefnum;
		fsSpec = doc->fsSpec;
		goto TryAgain;
	}

	/* At this point, the file we want to write (either the temporary file or the
	   "real" one) is open and <refNum> is set for it. */
		
	doc->docNew = False;
	doc->vrefnum = vRefNum;
	doc->fsSpec	= fsSpec;
	fileIsOpen = True;

	errType = WriteFile(doc, refNum);
	if (errType) { errInfo = WRITEcall; goto Error; };

	if (saveType==SF_SafeSave) {
		/* Close the temporary file; rename as backup or simply delete file at filename
		   on vRefNum (the old version of the file); and rename the temporary file to
		   filename and vRefNum. Note that, from the user's standpoint, this replaces
		   the file's creation date with the current date: unfortunate.
		   
		   FIXME: Inside Mac VI, 25-9ff, points out that System 7 introduced FSpExchangeFiles
		   and PBExchangeFiles, which simplify a safe save by altering the catalog entries
		   for two files to swap their contents. However, if we're keeping a backup copy,
		   this would result in the backup having the current date as its creation date!
		   One solution would be to start the process of saving the new file by making a
		   copy of the old version instead of a brand-new file; this would result in a
		   fair amount of extra I/O, but it might be the best solution. */

		if (config.makeBackup) {
			errType = RenameAsBackup(filename, vRefNum, bkpName);
			if (errType && errType!=fnfErr)							/* Ignore "file not found" */
				{ errInfo = BACKUPcall; goto Error; }
		}
		else {
			FSSpec oldFSSpec;
		
			//errType = FSMakeFSSpec(vRefNum, 0, filename, &oldFSSpec);
			//if (errType && errType!=fnfErr)
			//	{ errInfo = MAKEFSSPECcall; goto Error; }
			
			oldFSSpec = fsSpec;
		
			errType = FSpDelete(&oldFSSpec);						/* Delete old file */
			if (errType && errType!=fnfErr)							/* Ignore "file not found" */
				{ errInfo = DELETEcall; goto Error; }
		}

		errType = FSClose(refNum);									/* Close doc's file */
		if (errType) { errInfo = CLOSEcall; goto Error; }
		
		errType =  FSpRename (&tempFSSpec, filename);
		if (errType) { errInfo = RENAMEcall; goto Error; }			/* Rename temp to old filename */
			
	}
	else if (saveType==SF_Replace) {
		errType = FSClose(refNum);									/* Close doc's file */
		if (errType) { errInfo = CLOSEcall; goto Error; }
	}
	else if (saveType==SF_SaveAs) {
		errType = FSClose(refNum);									/* Close the newly created file */
		if (errType) { errInfo = CLOSEcall; goto Error; }
	}

	doc->changed = False;											/* Score isn't dirty */
	doc->saved = True;												/* ...and it's been saved */

	/* Save any print record or other resources in the resource fork */

	WritePrintHandle(doc);											/* Ignore any errors */
	
	/* Save the MIDI map in the resource fork */

	SaveMidiMap(doc);												/* Ignore any errors */

	FlushVol(NULL, vRefNum);
	return 0;

Error:
	SaveError(fileIsOpen, refNum, errType, errInfo);
	return errType;
}


/* ------------------------------------------------------------------------- SaveError -- */
/* Handle errors occurring while writing a file. Parameters are the same as those
for OpenError(). */

void SaveError(Boolean fileIsOpen,
					short refNum,
					short errType,
					short errInfo)		/* Details: see File.h for codes */
{
	char aStr[256], fmtStr[256];
	StringHandle strHdl;
	short strNum;

	SysBeep(1);
	LogPrintf(LOG_ERR, "Error saving the file. errType=%d errInfo=%d  (SaveError)\n",
		errType, errInfo);
	if (fileIsOpen) FSClose(refNum);

	/* We expect descriptions of the common errors stored by code (negative values,
	   for system errors; positive ones for our own I/O errors) in individual 'STR '
	   resources. If we find one for this error, print it, else just print the raw
	   code. */
	   
	strHdl = GetString(errType);
	if (strHdl) {
		Pstrcpy((unsigned char *)strBuf, (unsigned char *)*strHdl);
		PToCString((unsigned char *)strBuf);
		strNum = (errInfo>0? 15 : 16);		/* "%s (heap object type=%d)." : "%s (error code=%d)." */
		GetIndCString(fmtStr, FILEIO_STRS, strNum);
		sprintf(aStr, fmtStr, strBuf, errInfo);
	}
	else {
		strNum = (errInfo>0? 17 : 18);		/* "Error ID=%d (heap object type=%d)." : "Error ID=%d (error code=%d)." */
		GetIndCString(fmtStr, FILEIO_STRS, strNum);
		sprintf(aStr, fmtStr, errType, errInfo);
	}

	LogPrintf(LOG_ERR, aStr); LogPrintf(LOG_ERR, "\n");
	CParamText(aStr, "", "", "");
	StopInform(SAVE_ALRT);
}
