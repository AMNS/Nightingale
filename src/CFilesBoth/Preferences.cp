/***************************************************************************
	FILE:	Preferences.c
	PROJ:	Nightingale, rev. for v.2000
	DESC:	Routines for handling text preferences file
			text file containing a list of key value pairs in format
			key=value
			Nomenclature follows that of config file routines in Initialize.c
***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 *
 * Copyright © 1986-2002 by Adept Music Notation Solutions, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include <errno.h>
#include <ctype.h>
#include "Notelist.h"

// MAS #include <Fsp_fopen.h>
#include "FileUtils.h"

static short rfVRefNum;
static long rfVolDirID;
static OSType prefsTextFileType = 'TEXT';
static StringPoolRef prefsStringPool;
static FSSpec prefsFSSpec;
static Ptr prefsData;
static Ptr prefsTbl;
static long numPrefsValues;

typedef struct {
	char *key;
	char *value;
} KeyValuePair;


/* --------------------------------------------------------------------------------- */
/* Local prototypes */



/*
 * FindTextPreferencesFile
 *
 * Creates FSSpec for Nightingale_X_Prefs.txt
 */

static OSStatus FindTextPreferencesFile(OSType fType, OSType fCreator, FSSpec *prefsSpec) 
{
	short pvol;
	long pdir;
	Str255 name;
	CInfoPBRec cat;
	OSStatus err;
	
	// find the preferences folder
	err = FindFolder( kOnSystemDisk, kPreferencesFolderType, true, &pvol, &pdir);
	if (err != noErr) return err;
	
	// search the folder for the file
	BlockZero(&cat, sizeof(cat));
	cat.hFileInfo.ioNamePtr = name;
	cat.hFileInfo.ioVRefNum = pvol;
	cat.hFileInfo.ioFDirIndex = 1;
	cat.hFileInfo.ioDirID = pdir;
	
	while (PBGetCatInfoSync(&cat) == noErr) 
	{
		if (( (cat.hFileInfo.ioFlAttrib & 16) == 0) &&
				(cat.hFileInfo.ioFlFndrInfo.fdType == fType) &&
				(cat.hFileInfo.ioFlFndrInfo.fdCreator == fCreator) &&
				(PStrCmp(name, SETUP_TEXTFILE_NAME) == TRUE)) 
		{
			// make a fsspec referring to the file
			return FSMakeFSSpec(pvol, pdir, name, prefsSpec);
		}
		
		cat.hFileInfo.ioFDirIndex++;
		cat.hFileInfo.ioDirID = pdir;
	}
	
	err = FSMakeFSSpec(pvol, pdir, PREFS_PATH, prefsSpec);
	return fnfErr;
}

static INT16 AddNewLine(char *buf) 
{
	INT16 len = strlen(buf);
	buf[len++] = '\n';
	buf[len] = '\0';
	return len;
}

static Boolean AddSetupStrings(Handle resH, INT16 refNum)
{
	INT16 	numStrings, i;
	long		count;
	char 		buf[512];
	OSStatus theErr;
	
	numStrings = *(INT16 *)(*resH);
	for (i = 0; i < numStrings; i++) 
	{
		GetIndCString(buf, TEXTPREFS_STRS, i + 1);
		
		// Write the line to the file
		
		count = AddNewLine(buf);
		theErr = FSWrite(refNum, &count, buf);
		if (theErr) return FALSE;		
	}

	return TRUE;
}



/* ---------------------------------------------------------- CreateTextSetupFile -- */
static Boolean CreateTextSetupFile(FSSpec *fsSpec)
{
	ScriptCode	scriptCode = smRoman;
	OSStatus 	theErr;
	Handle		resH;
	INT16			refNum;
	
	// Create a new text preferences file.

	Pstrcpy(fsSpec->name, SETUP_TEXTFILE_PATH);	
	theErr = FSpCreate(fsSpec, creatorType, prefsTextFileType, scriptCode);
	if (theErr) return FALSE;
	
	theErr = FSpOpenDF (fsSpec, fsRdWrPerm, &refNum);
	if (theErr) return FALSE;
		
	/* Now copy the instrument list. */
	resH = GetResource('STR#', TEXTPREFS_STRS);
	
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupStrings(resH, refNum)) return FALSE;
	
	theErr = FSClose(refNum);
	if (theErr) return FALSE;

	return TRUE;
}

/* ---------------------------------------------------------------- OpenSetupFile -- */
/* Open the resource fork of the Prefs file.  If we can't find a Prefs file in the
System Folder, generate a new one using resources from the application and open it.
Also make the Prefs file the current resource file.

If we find a problem, including the Prefs file resource fork already open with
write permission, we give an error message and return FALSE. (If the Prefs file
resource fork is open but with only read permission, I suspect we won't detect that
and it won't be made the current resource file--bad but unlikely. Note that
NightCustomizer doesn't keep the Prefs file open while it's running.) If all goes
well, return TRUE. */

Boolean OpenTextSetupFile()
{
	OSErr result; Boolean okay=TRUE; Str63 volName;
	StringHandle strHdl; char fmtStr[256];
	
	short oldVRefNum; long oldDirID;
	short vRefNum; long dirID;

	OSErr theErr;
	
	theErr = FindFolder(kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder, &vRefNum, &dirID);
	
	HGetVol(volName,&oldVRefNum,&oldDirID);
	HSetVol(volName,vRefNum,dirID);		// Need to specify a location for setup file

	rfVRefNum = vRefNum;
	rfVolDirID = dirID;
	
	/* Try to open the Prefs file in the System Folder. */
	
	theErr = FindTextPreferencesFile(prefsTextFileType, creatorType, &prefsFSSpec);
	if (theErr == noErr) {
		theErr = FSpOpenDF(&prefsFSSpec, fsRdWrPerm, &setupFileRefNum);
	}
	result = theErr;
		
	if (result==fnfErr) {
		/* Note that the progress message has the setup filename embedded in it: this is
		 * lousy--it should use SETUP_FILE_NAME, of course--but ProgressMsg() can't
		 * handle that!
		 */
		ProgressMsg(CREATESETUP_PMSTR, "");
		SleepTicks(60L);								/* Be sure user can read the msg */
		if (!CreateTextSetupFile(&prefsFSSpec)) {
			GetIndCString(strBuf, INITERRS_STRS, 2);	/* "Can't create Prefs file" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			okay = FALSE;
			goto done;
		}
		/* Try opening the brand-new one. */
		theErr = FSpOpenDF(&prefsFSSpec, fsRdWrPerm, &setupFileRefNum);

		ProgressMsg(0, "");
	}

	result = theErr;
	if (result!=noErr) {
		strHdl = GetString(result);
		if (strHdl) {
			Pstrcpy((unsigned char *)strBuf, (unsigned char *)*strHdl);
			PToCString((unsigned char *)strBuf);
		}
		else {
			GetIndCString(fmtStr, INITERRS_STRS, 3);	/* "Resource Manager error %d" */
			sprintf(strBuf, fmtStr, result);
		}
		CParamText(strBuf, "", "", "");
		StopInform(OPENPREFS_ALRT);
		okay = FALSE;
	}
	
done:
	HSetVol(volName,oldVRefNum,oldDirID);
	return okay;
}

Boolean ValidChar(char c) 
{
	if (c == ' ' || c == '\t') return FALSE;
	return TRUE;
}

Ptr CleanData(Ptr data, long count) 
{
	Ptr newData = NewPtr(count + 2);
	if (!GoodNewPtr(newData)) return NULL;
	
	// ensure that we have null-terminated string which ends with
	// a newline
	
	newData[count] = '\n';
	newData[count+1] = '\0';
	
	
	char *p = data;
	long i = 0;
	
	for ( ; i<count; i++) 
	{
		if (*p == '\r') {
			*p++ = '\n';
		}
	}
	
	for (i=0,p=data; i<count-1; i++) 
	{	
		if (*p == '\n') {
			
			char *r = p+1;
			if (*r == '\n') {
				*r = ' ';				
			}
		}
	}
	
	char *q = newData;
	long j = 0;
	for (i=0,p=data; i<count; i++) 
	{	
		if (ValidChar(*p)) {
			*q++ = *p;
			j++;
		}
		*p++;
	}
	for ( ; j<count; j++) {
		*q++ = '\0';
	}
	
	return newData;	
}

long CountLines(char *data) 
{
	long numLines = 0;
	
	if (strlen(data) == 0) return 0;
	
	char *p = data;
	char *q = strchr(p, '\n');
	char *r = NULL;
	
	while (q != NULL) {
		*q = '\0';
		
		if (strlen(p) >= 3) {
			r = strchr(p, '=');
			if (r != NULL && p!=r) {
				numLines++;
			}
		}
		
		*q = '\n';
		p = q + 1;
		q = strchr(q + 1, '\n');
	}
	
	return numLines;
}

static Boolean ReadPrefsTbl(char *data, KeyValuePair *tbl, long numLines) 
{
	prefsData = data;
	char *p = data;
	char *q = strchr(p, '\n');
	char *r = NULL;
	long line = 0;
	
	while (q != NULL) {
		*q = '\0';
		
		if (strlen(p) >= 3) {
			r = strchr(p, '=');
			if (r != NULL && p!=r) {
				*r = '\0';
				tbl[line].key = p;
				tbl[line].value = r+1;
				line++;
			}
		}		
		if (line > numLines) return FALSE;
		
		p = q + 1;
		q = strchr(q + 1, '\n');		
	}
	
	return TRUE;
}

Boolean GetTextConfig()
{
	INT16 refNum;
	OSStatus theErr;
	long count, numLines;
	Ptr data, newData;
	Boolean ok = TRUE;
	
//	prefsStringPool = NewStringPool();
//	if (prefsStringPool==NULL)
//		return FALSE;
	
//	theErr = FSpOpenDF (&prefsFSSpec, fsRdWrPerm, &refNum);
//	if (theErr) return FALSE;

	refNum = setupFileRefNum;
	
	theErr = GetEOF(refNum, &count);
	if (theErr) return FALSE;
	
	data = NewPtr(count);
	if (!GoodNewPtr(data)) return FALSE;
	
	theErr = FSRead(refNum, &count, data);
	if (theErr) return FALSE;
	
	newData = CleanData(data, count);
	if (newData == NULL) {
		ok = FALSE; goto done;
	}
	
	numLines = CountLines(newData);
	if (numLines == 0) goto done;
	numPrefsValues = numLines;
	
	prefsTbl = NewPtr(numLines * sizeof(KeyValuePair));
	if (!GoodNewPtr(prefsTbl)) {
		ok = FALSE; goto done;
	}
	
	ok = ReadPrefsTbl(newData, (KeyValuePair *)prefsTbl, numLines);
	
done:
	if (data)
		DisposePtr(data);
	
	theErr = FSClose(refNum);
	if (theErr) ok = FALSE;
	
	if (ok == FALSE) {
	
		if (newData)
			DisposePtr(newData);
		
		if (prefsTbl) {
			DisposePtr(prefsTbl);
			prefsTbl = NULL;	
		}
					
		numPrefsValues = 0;		
	}
	
	return ok;
}

char *GetPrefsValue(char *key) 
{
	long i = 0;
	
	for ( ; i<numPrefsValues; i++) {
		KeyValuePair *pkvp = (KeyValuePair *)prefsTbl;
		KeyValuePair kvp = pkvp[i];
		if (!strcmp(key, kvp.key)) {
			return kvp.value;
		}
	}
	
	return NULL;
}

/*
	OpenTextSetupFile
	
	GetTextConfig
	
		Reads lines
		
		gets values from strchr('=')
		
		stores in ???
		
		ProcessNotelist(FILE *f)
		
			while (ReadLine(gInBuf, LINELEN, f)) {
			
				// split line on '='
				
				p = strchr(gInBuf, '=');						// p will point to '=' 
				if (!p) continue;
*/

