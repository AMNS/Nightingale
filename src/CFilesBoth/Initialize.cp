/* Initialize.c for Nightingale
One-time initialization of stuff that's not music-notation-specific -- well, mostly.
All one-time-only initialization code should be in this file or InitNightingale.c.
(This was originally so they could be unloaded by the caller after initialization to
reclaim heap space, but with the gigabytes of RAM modern computers have, who cares.
--DAB, Jan. 2020) */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */


#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "Endian.h"
#include "GraphicMDEF.h"

/* Private routines */

static void			InitToolbox(void);
static void			LogScoreHeaderFormatInfo(void);
static void			InitNPalettes(void);
static Boolean		AddPrefsResource(Handle);
static OSStatus		GetPrefsFileSpec(unsigned char *name, OSType fType, OSType fCreator,
									FSSpec *prefsSpec);
static void			DisplayConfig(void);
static Boolean		CheckConfig(void);
static Boolean		GetConfig(void);
static Boolean		InitMemory(short numMasters);
static Boolean		PrepareClipDoc(void);
static void			InstallCoreEventHandlers(void);


/* --------------------------------------------------------------- Initialize and ally -- */
/* Do everything that must be done prior to entering main event loop. A great deal of
this is platform-dependent. */

static void InitToolbox(void)
{
	FlushEvents(everyEvent, 0);
}
		
#define ScoreHdrFieldOffsetQQ(field)	(long)&((field))-(long)&(headL)
#define ScoreHdrFieldOffset(dc, field)	(long)&(dc->field)-(long)&(dc->headL)
#define NoDISP_SCOREHDR_STRUCT

static void LogScoreHeaderFormatInfo(void)
{
#ifdef DISP_SCOREHDR_STRUCT
	Document *tD;
	DocumentN105 *tD5;
	long noteInsFeedbackOff, fontNameMNOff, nfontsUsedOff, magnifyOff, spaceMapOff,
		voiceTabOff, afterEnd;

	/* The following code is intended to be compiled once in a blue moon, when the file
	   format changes, to collect information for Nightingale documentation. That info
	   (as of Jan. 2021, for formats 'N105' and 'N106') is in Nightingale Tech Note #1,
	   the Nightingale Programmer's Quick Reference (NgaleProgQuickRef-TN1.txt). */
	
	noteInsFeedbackOff = ScoreHdrFieldOffset(tD5, comment[MAX_COMMENT_LEN+1]);
	fontNameMNOff = ScoreHdrFieldOffset(tD5, fontNameMN[0]);
	nfontsUsedOff = ScoreHdrFieldOffset(tD5, nfontsUsed);
	magnifyOff = ScoreHdrFieldOffset(tD5, magnify);
	spaceMapOff = ScoreHdrFieldOffset(tD5, spaceMap[0]);
	voiceTabOff = ScoreHdrFieldOffset(tD5, voiceTab[0]);
	afterEnd = ScoreHdrFieldOffset(tD5, expansion[256-(MAXVOICES+1)]);
	LogPrintf(LOG_DEBUG,
		"SCORE HDR OFFSET ('N105' fmt): feedback=%ld fontNameMN=%ld nfontsUsed=%ld magnify=%ld\n",
			noteInsFeedbackOff, fontNameMNOff, nfontsUsedOff, magnifyOff);
	LogPrintf(LOG_DEBUG,
		"      spaceMap=%ld voiceTab=%ld afterEnd=%ld\n",
			spaceMapOff, voiceTabOff, afterEnd);
	noteInsFeedbackOff = ScoreHdrFieldOffset(tD, noteInsFeedback);
	fontNameMNOff = ScoreHdrFieldOffset(tD, fontNameMN[0]);
	nfontsUsedOff = ScoreHdrFieldOffset(tD, nfontsUsed);
	magnifyOff = ScoreHdrFieldOffset(tD, magnify);
	spaceMapOff = ScoreHdrFieldOffset(tD, spaceMap[0]); 
	voiceTabOff = ScoreHdrFieldOffset(tD, voiceTab[0]);
	afterEnd = ScoreHdrFieldOffset(tD, expansion[256-(MAXVOICES+1)]);
	LogPrintf(LOG_DEBUG,
		"SCORE HDR OFFSET ('N106' fmt): noteInsFeedback=%ld fontNameMN=%ld nfontsUsed=%ld magnify=%ld\n",
			noteInsFeedbackOff, fontNameMNOff, nfontsUsedOff, magnifyOff);
	LogPrintf(LOG_DEBUG,
		"      spaceMap=%ld voiceTab=%ld afterEnd=%ld\n",
			spaceMapOff, voiceTabOff, afterEnd);
#endif
}


#define NPALETTE_IMAGE_DIR "/Library/Application\ Support"

#define SHOWWD errno = 0; if (getcwd(cwd, sizeof(cwd))!=NULL) LogPrintf(LOG_DEBUG, "SHOWWD: Current working dir: '%s'. errno=%d\n", \
cwd, errno); \
else { LogPrintf(LOG_DEBUG, "SHOWWD: getcwd() error. errno=%d\n", errno); }

static void InitNPalettes(void)
{
	char cwdSave[PATH_MAX], cwd[PATH_MAX];

	LogPrintf(LOG_NOTICE, "Initializing palettes & palette windows from images in %s...  (Initialize)\n",
				NPALETTE_IMAGE_DIR);
	if (getcwd(cwdSave, sizeof(cwdSave))==NULL) LogPrintf(LOG_ERR,
							"Can't save the current working directory.  (InitNPalettes)\n");
	errno = 0; chdir(NPALETTE_IMAGE_DIR);
	if (errno!=0)  LogPrintf(LOG_ERR, "Can't change to palette image directory. errno=%d  (InitNPalettes)\n",
							errno); SHOWWD;

	if (!NInitPaletteWindows()) { BadInit(); ExitToShell(); }
	if (!InitDynamicPalette()) { BadInit(); ExitToShell(); }
	if (!InitModNRPalette()) { BadInit(); ExitToShell(); }
	if (!InitDurationPalette()) { BadInit(); ExitToShell(); }
	
	chdir(cwdSave);
}

#define STRBUF_SIZE 256

static GrowZoneUPP growZoneUPP;			/* permanent GrowZone UPP */

void Initialize(void)
{
	OSErr err;
	Str63 dummyVolName;
	Str255 versionPStr;
	char bigOrLittleEndian;
	
	InitToolbox();

	/* Save resource file and volume refNum and dir ID of Nightingale application. */

	appRFRefNum = CurResFile();
	dummyVolName[0] = 0;
	err = HGetVol(dummyVolName, &appVRefNum, &appDirID);
	if (err)
		{ BadInit(); ExitToShell(); }
			
	/* We must allocate <strBuf> immediately: it's used to build error messages. */
	
	strBuf = (char *)NewPtr(STRBUF_SIZE);
	if (!GoodNewPtr((Ptr)strBuf))
		{ OutOfMemory((long)STRBUF_SIZE); ExitToShell(); }
	
	creatorType = CREATOR_TYPE_NORMAL;
	documentType = DOCUMENT_TYPE_NORMAL;

	/* Figure out our machine environment before doing anything more. This can't be done
	   until after the various ToolBox calls above. */
	   	
	thisMac.machineType = -1;
	err = SysEnvirons(1, &thisMac);
	if (err) thisMac.machineType = -1;
	
	InstallCoreEventHandlers();
	
	InitLogPrintf();

	/* Fill global _applVerStr_ with our version string, and display it in the log. */
	
	Pstrcpy((unsigned char *)strBuf, VersionString(versionPStr));
	PToCString((unsigned char *)strBuf);
	GoodStrncpy(applVerStr, strBuf, 20);			/* Allow one char. for terminator */
	
#if TARGET_RT_LITTLE_ENDIAN
	bigOrLittleEndian = 'L';
#else
	bigOrLittleEndian = 'B';
#endif

	LogPrintf(LOG_NOTICE, "RUNNING NIGHTINGALE %s-%cE  (Initialize)\n", applVerStr,
				bigOrLittleEndian);

	if (!OpenPrefsFile())							/* needs creatorType */
		{ BadInit(); ExitToShell(); }
	GetConfig();									/* needs the Prefs (Setup) file open */
	if (!OpenTextSetupFile())						/* needs creatorType */
		{ BadInit(); ExitToShell(); }
	GetTextConfig();								/* needs the Prefs file open */
	
#ifdef NOTYET
	/* FIXME: Finish mplementing the Preferences text file! The code in Preferences.c
	   passes simple tests like these. */
	   
	char *foo = GetPrefsValue("foo");
	char *bazz = GetPrefsValue("bazz");
#endif

	if (!InitMemory(config.numMasters))				/* needs the CNFG resource */
		{ BadInit(); ExitToShell(); }

	/* Allocate a large grow zone handle, which gets freed when we run out of memory,
	   so that whoever is asking for memory will probably get what they need without
	   our having to crash or die prematurely except in the rarest occasions. (This is
	   a "rainy day" memory fund. It's ancient code, and I doubt it's useful any more.
	   --DAB, Sept. 2019) */
	   
	memoryBuffer = NewHandle(config.rainyDayMemory*1024L);
	if (!GoodNewHandle(memoryBuffer)) {
		OutOfMemory(config.rainyDayMemory*1024L);
		BadInit();
		ExitToShell();
	}
	outOfMemory = False;
	growZoneUPP = NewGrowZoneUPP(GrowMemory);
	if (growZoneUPP) SetGrowZone(growZoneUPP);		/* Install simple grow zone function */
	
	InitNPalettes();
	InitCursor();
	WaitCursor();

	/* Do Application-specific initialization. */
	
	if (!InitGlobals()) { BadInit();  ExitToShell(); }
	Init_Help();
	
	LogScoreHeaderFormatInfo();
	
	/* See if we have enough memory that the user should be able to do SOMETHING
	   useful, and enough to get back to the main event loop, where we do our regular
	   low-memory checking. (As of v.999, 250K was enough; but now, in the 21st century,
	   we might as well make the minimum a lot higher -- though it's probably not even
	   meaningful anymore!  --DAB, Feb. 2017) */
	   
	if (!PreflightMem(1000))
		{ BadInit(); ExitToShell(); }
}


/* ------------------------------------------------------------------------ Prefs file -- */

static OSStatus GetPrefsFileSpec(unsigned char *fileName, OSType fType, OSType fCreator,
								FSSpec *prefsSpec) 
{
	short pvol;
	long pdir;
	Str255 name;
	CInfoPBRec cat;
	OSStatus err;
	
	/* Find the preferences folder, normally ~/Library/Preferences */
	
	err = FindFolder(kOnSystemDisk, kPreferencesFolderType, True, &pvol, &pdir);
	//LogPrintf(LOG_DEBUG, "FindFolder: err=%d  (FindPrefsFile)\n", err);
	if (err!=noErr) return err;
	
	/* Search the folder for the file */
	
	BlockZero(&cat, sizeof(cat));
//LogPrintf(LOG_DEBUG, "FindFolder: name='%s'  (FindPrefsFile)\n", cat.hFileInfo.ioNamePtr);
	cat.hFileInfo.ioNamePtr = name;
	cat.hFileInfo.ioVRefNum = pvol;
	cat.hFileInfo.ioFDirIndex = 1;
	cat.hFileInfo.ioDirID = pdir;
	
	while (PBGetCatInfoSync(&cat) == noErr) {
		if (( (cat.hFileInfo.ioFlAttrib & 16) == 0) &&
				(cat.hFileInfo.ioFlFndrInfo.fdType == fType) &&
				(cat.hFileInfo.ioFlFndrInfo.fdCreator == fCreator) &&
				Pstreql(name, fileName)) {
			/* Found it. Make a fsspec referring to the file */
			
			return FSMakeFSSpec(pvol, pdir, name, prefsSpec);
		}
		
		cat.hFileInfo.ioFDirIndex++;
		cat.hFileInfo.ioDirID = pdir;
	}
	
	/* We couldn't find it: tell the calling routine. Call FSMakeFSSpec first in the
	   hope of giving the user (or a tech support person) helpful information. */
	
	err = FSMakeFSSpec(pvol, pdir, PREFS_PATH, prefsSpec);
	LogPrintf(LOG_INFO, "FSMakeFSSpec: err=%d (fnfErr=%d)  (FindPrefsFile)\n", err, fnfErr);
	return fnfErr;
} 


/* Create a new Nightingale Prefs file. (We used to call the "Prefs" file the "Setup"
file, and some names may still reflect that.) If we succeed, return True; if we fail,
return False without giving an error message. */

static short rfVRefNum;
static long rfVolDirID;
static OSType prefsFileType = 'NSET';

Boolean CreatePrefsFile(FSSpec *rfSpec)
{
	Handle			resH;
	OSType			theErr;
	short			nSPTB, i;
	ScriptCode		scriptCode = smRoman;
	
	/* Create a new file in the ~/Library/Preferences and give it a resource fork. If
	   there is no Preferences folder in ~/Library, create it. */
	
	Pstrcpy(rfSpec->name, PREFS_FILE_PATH);	
	FSpCreateResFile(rfSpec, creatorType, 'NSET', scriptCode);
	theErr = ResError();
	if (theErr) return False;

	if (theErr!=noErr) return False;
	
	/* Open it, setting global setupFileRefNum. NB: HOpenResFile makes the file it
	   opens the current resource file. */
		
	setupFileRefNum = FSpOpenResFile(rfSpec, fsRdWrPerm);
	
	theErr = ResError();
	if (theErr!=noErr) return False;				/* Otherwise we'll write in the app's resource fork! */
	
	/* Get various resources and copy them to the new Prefs file.  Since the Prefs
	   file has nothing in it yet, GetResource will look thru the other open resource
	   files for each resource we Get, and presumably find them in the application.
	   First copy the config and its template. */
	   
	resH = GetResource('CNFG', THE_CNFG);
	if (!GoodResource(resH)) return False;	
	if (!AddPrefsResource(resH)) return False;

	resH = GetNamedResource('TMPL', "\pCNFG");
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	/* Now copy the instrument list. */
	
	resH = GetResource('STR#', INSTR_STRS);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	/* Next, the MIDI velocity table and its template. */
	
	resH = GetResource('MIDI', PREFS_MIDI);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	resH = GetNamedResource('TMPL', "\pMIDI");
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	/* The MIDI modifier prefs table and its template. */
	
	resH = GetResource('MIDM', PREFS_MIDI_MODNR);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	resH = GetNamedResource('TMPL', "\pMIDM");
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	/* Now copy the palette, palette char. map, and palette translation map and template. */
	
	resH = GetResource('PICT', ToolPaletteID);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	resH = GetResource('PLCH', ToolPaletteID);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	resH = GetResource('PLMP', THE_PLMP);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;
	
	resH = GetNamedResource('TMPL', "\pPLMP");
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	/* Now copy the MIDI Manager port resources. */
	
	resH = GetResource('port', time_port);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;
	
	resH = GetResource('port', input_port);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	resH = GetResource('port', output_port);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	/* And finally copy the built-in MIDI parameter resource. */
	
	resH = GetResource('BIMI', THE_BIMI);
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	/* Copy all the spacing tables we can find into the Prefs file. We use Get1IndResource
	   instead of GetIndResource to avoid getting a resource from the Prefs file we've
	   just written out! (Of course, this means we have to be sure the current resource
	   file is the application; we're relying on AddPrefsResource to take care of that.)
	   Also copy the template. */
	
	nSPTB = CountResources('SPTB');
	for (i = 1; i<=nSPTB; i++) {
		resH = Get1IndResource('SPTB', i);
		if (!GoodResource(resH)) return False;		
		if (!AddPrefsResource(resH)) return False;
	}
	
	resH = GetNamedResource('TMPL', "\pSPTB");
	if (!GoodResource(resH)) return False;		
	if (!AddPrefsResource(resH)) return False;

	/* Finally, close the Prefs file. */

	CloseResFile(setupFileRefNum);
	
	return True;
}


/* Open the resource fork of the Prefs file.  If we can't find a Prefs file in the
System Folder, generate a new one using resources from the application and open it.
Also make the Prefs file the current resource file.

If we find a problem, including the Prefs file resource fork already open with write
permission, we give an error message and return False. (If the Prefs file resource
fork is open but with only read permission, I suspect we won't detect that and it
won't be made the current resource file: bad, but unlikely.) If all goes well, return
True. */

Boolean OpenPrefsFile(void)
{
	OSErr result;  Boolean okay=True;
	Str63 volName;
	StringHandle strHdl;  char fmtStr[256];
	Str255 prefsFileName;

	short oldVRefNum, vRefNum;
	long oldDirID, dirID;

	FSSpec rfSpec; OSErr theErr;
	
//	short oldVol; 
//	GetVol(volName,&oldVol);
//	SetVol(volName,thisMac.sysVRefNum);		/* fromSysEnvirons */

	theErr = FindFolder(kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder, &vRefNum, &dirID);
	
	HGetVol(volName, &oldVRefNum, &oldDirID);
	HSetVol(volName, vRefNum, dirID);				/* Need to specify a location for setup file */

	rfVRefNum = vRefNum;
	rfVolDirID = dirID;
	
	/* Try to open the Prefs file. Under "classic" Mac OS (9 and before), it was normally
	   in the System Folder; under OS X, it's in ~/Library/Preferences . */
	
	Pstrcpy(prefsFileName, PREFS_FILE_NAME);

	theErr = GetPrefsFileSpec(prefsFileName, prefsFileType, creatorType, &rfSpec);
	LogPrintf(LOG_INFO, "GetPrefsFileSpec (filename '%s') returned theErr=%d  (OpenPrefsFile)\n",
				PToCString(prefsFileName), theErr);
	if (theErr==noErr) setupFileRefNum = FSpOpenResFile(&rfSpec, fsRdWrPerm);
	
	/* If we can't open it, create a new one using app's own CNFG and other resources. */
	
	if (theErr==noErr)	result = ResError();
	else				result = theErr;
		
//	if (result==fnfErr || result==dirNFErr) {
	if (result==fnfErr) {
		/* FIXME: Note that the progress message has the setup filename embedded in it.
		   This is dumb -- it should use PREFS_FILE_NAME, of course -- but ProgressMsg()
		   can't handle that!*/
		   
		LogPrintf(LOG_NOTICE, "Can\'t find a '%s' (Preferences) file: creating a new one.  (OpenPrefsFile)\n",
					PToCString(prefsFileName));
		ProgressMsg(CREATE_PREFS_PMSTR, "");
		SleepTicks((unsigned)(5*60L));					/* Give user time to read the msg */
		if (!CreatePrefsFile(&rfSpec)) {
			GetIndCString(strBuf, INITERRS_STRS, 2);	/* "Can't create Prefs file" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			okay = False;
			goto done;
		}
		
		/* Try opening the brand-new Prefs file. */
		
		setupFileRefNum = FSpOpenResFile(&rfSpec, fsRdWrPerm);

		ProgressMsg(0, "");
	}

	result = ResError();								/* Careful: this is not redundant! */
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
		okay = False;
	}
	
done:
	HSetVol(volName, oldVRefNum, oldDirID);
	return okay;
}

/* Adds the resource pointed to by resH to the current resource file. Returns True if
OK, False if error. */

static Boolean AddPrefsResource(Handle resH)
{
	Handle	tempH;
	OSType	type;
	short	id;
	Str255	name;
	
	UseResFile(setupFileRefNum);				/* safeguard against writing in app's res fork */

	GetResInfo(resH, &id, &type, name);
	if (ReportResError()) return False;
	
	tempH = resH;
	HandToHand(&tempH);	
	ReleaseResource(resH);
	if (ReportResError()) return False;
	AddResource(tempH, type, id, name);
	if (ReportResError()) return False;
	WriteResource(tempH);
	if (ReportResError()) return False;
	ReleaseResource(tempH);
	if (ReportResError()) return False;
	
	UseResFile(appRFRefNum);
	return True;
}


/* --------------------------------------------------------------------------- Config -- */

static void DisplayConfig(void)
{
	LogPrintf(LOG_INFO, "Displaying CNFG:\n");
	LogPrintf(LOG_INFO, "  (1)maxDocuments=%d", config.maxDocuments);
	LogPrintf(LOG_INFO, "  (2)stemLenNormal=%d", config.stemLenNormal);
	LogPrintf(LOG_INFO, "  (3)stemLen2v=%d", config.stemLen2v);
	LogPrintf(LOG_INFO, "  (4)stemLenOutside=%d\n", config.stemLenOutside);
	LogPrintf(LOG_INFO, "  (5)stemLenGrace=%d", config.stemLenGrace);

	LogPrintf(LOG_INFO, "  (6)spAfterBar=%d", config.spAfterBar);
	LogPrintf(LOG_INFO, "  (7)minSpBeforeBar=%d", config.minSpBeforeBar);
	LogPrintf(LOG_INFO, "  (8)minRSpace=%d\n", config.minRSpace);
	LogPrintf(LOG_INFO, "  (9)minLyricRSpace=%d", config.minLyricRSpace);
	LogPrintf(LOG_INFO, "  (10)minRSpace_KSTS_N=%d", config.minRSpace_KSTS_N);

	LogPrintf(LOG_INFO, "  (11)hAccStep=%d", config.hAccStep);

	LogPrintf(LOG_INFO, "  (12)stemLW=%d\n", config.stemLW);
	LogPrintf(LOG_INFO, "  (13)barlineLW=%d", config.barlineLW);
	LogPrintf(LOG_INFO, "  (14)ledgerLW=%d", config.ledgerLW);
	LogPrintf(LOG_INFO, "  (15)staffLW=%d", config.staffLW);
	LogPrintf(LOG_INFO, "  (16)enclLW=%d\n", config.enclLW);

	LogPrintf(LOG_INFO, "  (17)beamLW=%d", config.beamLW);
	LogPrintf(LOG_INFO, "  (18)hairpinLW=%d", config.hairpinLW);
	LogPrintf(LOG_INFO, "  (19)dottedBarlineLW=%d", config.dottedBarlineLW);
	LogPrintf(LOG_INFO, "  (20)mbRestEndLW=%d\n", config.mbRestEndLW);
	LogPrintf(LOG_INFO, "  (21)nonarpLW=%d", config.nonarpLW);

	LogPrintf(LOG_INFO, "  (22)graceSlashLW=%d", config.graceSlashLW);
	LogPrintf(LOG_INFO, "  (23)slurMidLW=%d", config.slurMidLW);
	LogPrintf(LOG_INFO, "  (24)tremSlashLW=%d\n", config.tremSlashLW);
	LogPrintf(LOG_INFO, "  (25)slurCurvature=%d", config.slurCurvature);
	LogPrintf(LOG_INFO, "  (26)tieCurvature=%d", config.tieCurvature);
	LogPrintf(LOG_INFO, "  (27)relBeamSlope=%d", config.relBeamSlope);

	LogPrintf(LOG_INFO, "  (28)hairpinMouthWidth=%d\n", config.hairpinMouthWidth);
	LogPrintf(LOG_INFO, "  (29)mbRestBaseLen=%d", config.mbRestBaseLen);
	LogPrintf(LOG_INFO, "  (30)mbRestAddLen=%d", config.mbRestAddLen);
	LogPrintf(LOG_INFO, "  (31)barlineDashLen=%d", config.barlineDashLen);
	LogPrintf(LOG_INFO, "  (32)crossStaffBeamExt=%d\n", config.crossStaffBeamExt);
	LogPrintf(LOG_INFO, "  (33)titleMargin=%d", config.titleMargin);

	LogPrintf(LOG_INFO, "  (34)paperRect.top=%d", config.paperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", config.paperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", config.paperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", config.paperRect.right);

	LogPrintf(LOG_INFO, "  (35-36)pageMarg.top=%d", config.pageMarg.top);
	LogPrintf(LOG_INFO, "  .left=%d", config.pageMarg.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", config.pageMarg.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", config.pageMarg.right);

	LogPrintf(LOG_INFO, "  (37)pageNumMarg.top=%d", config.pageNumMarg.top);
	LogPrintf(LOG_INFO, "  .left=%d", config.pageNumMarg.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", config.pageNumMarg.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", config.pageNumMarg.right);

	LogPrintf(LOG_INFO, "  (38)defaultLedgers=%d", config.defaultLedgers);
	LogPrintf(LOG_INFO, "  (39)defaultTSNum=%d", config.defaultTSNum);
	LogPrintf(LOG_INFO, "  (40)defaultTSDenom=%d", config.defaultTSDenom);
	LogPrintf(LOG_INFO, "  (41)defaultRastral=%d\n", config.defaultRastral);
	LogPrintf(LOG_INFO, "  (42)rastral0size=%d", config.rastral0size);

	LogPrintf(LOG_INFO, "  (43)minRecVelocity=%d", config.minRecVelocity);
	LogPrintf(LOG_INFO, "  (44)minRecDuration=%d\n", config.minRecDuration);
	LogPrintf(LOG_INFO, "  (45)midiThru=%d", config.midiThru);
	LogPrintf(LOG_INFO, "  (46)defaultTempoMM=%d", config.defaultTempoMM);

	LogPrintf(LOG_INFO, "  (47)lowMemory=%d", config.lowMemory);
	LogPrintf(LOG_INFO, "  (48)minMemory=%d\n", config.minMemory);

	LogPrintf(LOG_INFO, "  (49)numRows=%d", config.numRows);
	LogPrintf(LOG_INFO, "  (50)numCols=%d", config.numCols);
	LogPrintf(LOG_INFO, "  (51)maxRows=%d", config.maxRows);
	LogPrintf(LOG_INFO, "  (52)maxCols=%d\n", config.maxCols);
	LogPrintf(LOG_INFO, "  (53)hPageSep=%d", config.hPageSep);
	LogPrintf(LOG_INFO, "  (54)vPageSep=%d", config.vPageSep);
	LogPrintf(LOG_INFO, "  (55)hScrollSlop=%d", config.hScrollSlop);
	LogPrintf(LOG_INFO, "  (56)vScrollSlop=%d\n", config.vScrollSlop);

	LogPrintf(LOG_INFO, "  (57)maxSyncTolerance=%d", config.maxSyncTolerance);
	LogPrintf(LOG_INFO, "  (58)enlargeHilite=%d", config.enlargeHilite);
	LogPrintf(LOG_INFO, "  (59)infoDistUnits=%d", config.infoDistUnits);
	LogPrintf(LOG_INFO, "  (60)mShakeThresh=%d\n", config.mShakeThresh);
	
	LogPrintf(LOG_INFO, "  (61)musicFontID=%d", config.musicFontID);
	LogPrintf(LOG_INFO, "  (62)numMasters=%d", config.numMasters);
	LogPrintf(LOG_INFO, "  (63)indentFirst=%d", config.indentFirst);
	LogPrintf(LOG_INFO, "  (64)mbRestHeight=%d\n", config.mbRestHeight);
	LogPrintf(LOG_INFO, "  (65)chordSymMusSize=%d", config.chordSymMusSize);

	LogPrintf(LOG_INFO, "  (66)enclMargin=%d", config.enclMargin);
	LogPrintf(LOG_INFO, "  (67)legatoPct=%d", config.legatoPct);

	LogPrintf(LOG_INFO, "  (68)defaultPatch=%d\n", config.defaultPatch);
	LogPrintf(LOG_INFO, "  (69)whichMIDI=%d", config.whichMIDI);

	LogPrintf(LOG_INFO, "  (70)musFontSizeOffset=%d", config.musFontSizeOffset);
	LogPrintf(LOG_INFO, "  (71)fastScreenSlurs=%d", config.fastScreenSlurs);
	LogPrintf(LOG_INFO, "  (72)restMVOffset=%d\n", config.restMVOffset);
	LogPrintf(LOG_INFO, "  (73)autoBeamOptions=%d", config.autoBeamOptions);
	
	LogPrintf(LOG_INFO, "  (74)noteOffVel=%d", config.noteOffVel);
	LogPrintf(LOG_INFO, "  (75)feedbackNoteOnVel=%d", config.feedbackNoteOnVel);
	LogPrintf(LOG_INFO, "  (76)defaultChannel=%d\n", config.defaultChannel);
	LogPrintf(LOG_INFO, "  (77)enclWidthOffset=%d", config.enclWidthOffset);	
	LogPrintf(LOG_INFO, "  (78)rainyDayMemory=%d", config.rainyDayMemory);
	LogPrintf(LOG_INFO, "  (79)tryTupLevels=%d", config.tryTupLevels);
	LogPrintf(LOG_INFO, "  (80)justifyWarnThresh=%d\n", config.justifyWarnThresh);

	LogPrintf(LOG_INFO, "  (81)metroChannel=%d", config.metroChannel);
	LogPrintf(LOG_INFO, "  (82)metroNote=%d", config.metroNote);
	LogPrintf(LOG_INFO, "  (83)metroVelo=%d", config.metroVelo);
	LogPrintf(LOG_INFO, "  (84)metroDur=%d\n", config.metroDur);

	LogPrintf(LOG_INFO, "  (85)chordSymSmallSize=%d", config.chordSymSmallSize);
	LogPrintf(LOG_INFO, "  (86)chordSymSuperscr=%d", config.chordSymSuperscr);
	LogPrintf(LOG_INFO, "  (87)chordSymStkLead=%d\n", config.chordSymStkLead);

	LogPrintf(LOG_INFO, "  (88)tempoMarkHGap=%d", config.tempoMarkHGap);
	LogPrintf(LOG_INFO, "  (89)trebleVOffset=%d", config.trebleVOffset);
	LogPrintf(LOG_INFO, "  (90)cClefVOffset=%d", config.cClefVOffset);
	LogPrintf(LOG_INFO, "  (91)bassVOffset=%d\n", config.bassVOffset);

	LogPrintf(LOG_INFO, "  (92)tupletNumSize=%d", config.tupletNumSize);
	LogPrintf(LOG_INFO, "  (93)tupletColonSize=%d", config.tupletColonSize);
	LogPrintf(LOG_INFO, "  (94)octaveNumSize=%d", config.octaveNumSize);
	LogPrintf(LOG_INFO, "  (95)lineLW=%d\n", config.lineLW);
	LogPrintf(LOG_INFO, "  (96)ledgerLLen=%d", config.ledgerLLen);
	LogPrintf(LOG_INFO, "  (97)ledgerLOtherLen=%d", config.ledgerLOtherLen);
	LogPrintf(LOG_INFO, "  (98)slurDashLen=%d", config.slurDashLen);
	LogPrintf(LOG_INFO, "  (99)slurSpaceLen=%d\n", config.slurSpaceLen);

	LogPrintf(LOG_INFO, "  (100)courtesyAccLXD=%d", config.courtesyAccLXD);
	LogPrintf(LOG_INFO, "  (101)courtesyAccRXD=%d", config.courtesyAccRXD);
	LogPrintf(LOG_INFO, "  (102)courtesyAccYD=%d", config.courtesyAccYD);
	LogPrintf(LOG_INFO, "  (103)courtesyAccPSize=%d\n", config.courtesyAccPSize);

	LogPrintf(LOG_INFO, "  (104)quantizeBeamYPos=%d", config.quantizeBeamYPos);
	LogPrintf(LOG_INFO, "  (105)enlargeNRHiliteH=%d", config.enlargeNRHiliteH);
	LogPrintf(LOG_INFO, "  (106)enlargeNRHiliteV=%d\n", config.enlargeNRHiliteV);

	LogPrintf(LOG_INFO, "  (107)pianorollLenFact=%d", config.pianorollLenFact);
	LogPrintf(LOG_INFO, "  (108)useModNREffects=%d", config.useModNREffects);
	LogPrintf(LOG_INFO, "  (109)thruChannel=%d", config.thruChannel);
	LogPrintf(LOG_INFO, "  (110)thruDevice=%d\n", config.thruDevice);

	LogPrintf(LOG_INFO, "  (111)cmMetroDev=%ld", config.cmMetroDev);
	LogPrintf(LOG_INFO, "  (112)cmDfltInputDev=%ld", config.cmDfltInputDev );
	LogPrintf(LOG_INFO, "  (113)cmDfltOutputDev=%ld", config.cmDfltOutputDev);
	LogPrintf(LOG_INFO, "  (114)cmDfltOutputChannel=%d", config.cmDfltOutputChannel);
	LogPrintf(LOG_INFO, "\n");
}


#define ERR(fn) { nerr++; LogPrintf(LOG_WARNING, "err #%d, ", fn); if (firstErr==0) firstErr = fn; }

/* Do a reality check for config values that might be bad. We can't easily check origin,
toolsPosition, font IDs, or the fields that represent Boolean values or MIDI system info.
Almost all other fields are checked, though not all as strictly as possible -- and of
course most don't have well-defined limits. For each problem case we find, give an error
message and set the field to a reasonable default value. If no problems are found,
return TRUE. If problems are found, tell user and give them a chance to quit immediately;
if they decline, return FALSE. */

static Boolean CheckConfig(void)
{
	short nerr, firstErr;
	char fmtStr[256];

	LogPrintf(LOG_NOTICE, "Checking CNFG: ");
	nerr = 0;
	firstErr = 0;

	if (config.maxDocuments < 1 || config.maxDocuments > 150)
		{ config.maxDocuments = 10; ERR(1); }

#define STEMLEN_NORMAL_MIN 1
#define STEMLEN_NORMAL_DFLT 14

#define STEMLEN_2V_MIN 1
#define STEMLEN_2V_DFLT 12

#define STEMLEN_OUTSIDE_MIN 1
#define STEMLEN_OUTSIDE_DFLT 12

#define STEMLEN_GRACE_MIN 1
#define STEMLEN_GRACE_DFLT 10

/* Assume compiled-in minimum values for these are OK: currently they're all 0 or 1. */

#define SP_AFTER_BAR_DFLT 6
#define MINSP_BEFORE_BAR_DFLT 5
#define MINRSP_DFLT 2
#define MINLYRICRSP_DFLT 4
#define MINRSP_KSTS_DFLT 8
#define HACCSTEP_DFLT 4
	if (config.stemLenNormal < STEMLEN_NORMAL_MIN) { config.stemLenNormal = STEMLEN_NORMAL_DFLT; ERR(2); }
	if (config.stemLen2v < STEMLEN_2V_MIN) { config.stemLen2v = STEMLEN_2V_DFLT; ERR(3); }
	if (config.stemLenOutside < STEMLEN_OUTSIDE_MIN) { config.stemLenOutside = STEMLEN_OUTSIDE_DFLT; ERR(4); }
	if (config.stemLenGrace < STEMLEN_GRACE_MIN) { config.stemLenGrace = STEMLEN_GRACE_DFLT; ERR(5); }

	if (config.spAfterBar < 0) { config.spAfterBar = SP_AFTER_BAR_DFLT; ERR(6); }
	if (config.minSpBeforeBar < 0) { config.minSpBeforeBar = MINSP_BEFORE_BAR_DFLT; ERR(7); }
	if (config.minRSpace < 0) { config.minRSpace = MINRSP_DFLT;  ERR(8); }
	if (config.minLyricRSpace < 0) { config.minLyricRSpace = MINLYRICRSP_DFLT;  ERR(9); }
	if (config.minRSpace_KSTS_N < 0) { config.minRSpace_KSTS_N = MINLYRICRSP_DFLT; ERR(10); }

	if (config.hAccStep < 0 || config.hAccStep > 31) { config.hAccStep = HACCSTEP_DFLT; ERR(11); }

#define STEMLW_DFLT 8
#define BARLINELW_DFLT 10
#define LEDGERLW_DFLT 13
#define STAFFLW_DFLT 8
#define ENCLLW_DFLT 4
	if (config.stemLW < 0) { config.stemLW = STEMLW_DFLT;  ERR(12); }
	if (config.barlineLW < 0) { config.barlineLW = BARLINELW_DFLT;  ERR(13); }
	if (config.ledgerLW < 0) { config.ledgerLW = LEDGERLW_DFLT;  ERR(14); }
	if (config.staffLW < 0) { config.staffLW = STAFFLW_DFLT;  ERR(15); }
	if (config.enclLW < 0) { config.enclLW = ENCLLW_DFLT; ERR(16); }

#define BEAMLW_DFLT 50
#define HAIRPINLW_DFLT 12
#define DTBARLINELW_DFLT 12
#define MBRESTENDLW_DFLT 12
#define NONARPLW_DFLT 12
	if (config.beamLW < 1) { config.beamLW = BEAMLW_DFLT; ERR(17); }
	if (config.hairpinLW < 1) { config.hairpinLW = HAIRPINLW_DFLT; ERR(18); }
	if (config.dottedBarlineLW < 1) { config.dottedBarlineLW = DTBARLINELW_DFLT; ERR(19); }
	if (config.mbRestEndLW < 1) { config.mbRestEndLW = MBRESTENDLW_DFLT; ERR(20); }
	if (config.nonarpLW < 1) { config.nonarpLW = NONARPLW_DFLT; ERR(21); }

#define GRACESLASHLW_DFLT 12
#define SLURMIDLW_DFLT 30
#define TREMSLASHLW_DFLT 38
#define SLURCURVELW_DFLT 100
#define TIECURVELW_DFLT 100
#define BEAMSLOPELW_DFLT 25
	if (config.graceSlashLW < 0) { config.graceSlashLW = GRACESLASHLW_DFLT; ERR(22); }
	if (config.slurMidLW < 0) { config.slurMidLW = SLURMIDLW_DFLT; ERR(23); }
	if (config.tremSlashLW < 0) { config.tremSlashLW = TREMSLASHLW_DFLT; ERR(24); }
	if (config.slurCurvature < 1) { config.slurCurvature = SLURCURVELW_DFLT; ERR(25); }
	if (config.tieCurvature < 1) { config.tieCurvature = TIECURVELW_DFLT; ERR(26); }
	if (config.relBeamSlope < 0) { config.relBeamSlope = BEAMSLOPELW_DFLT; ERR(27); }

#define HAIRPINMOUTH_DFLT 5
#define MBREST_BASELEN_DFLT 6
#define MBREST_ADDLEN_DFLT 1
#define BARLINE_DASHLEN_DFLT 4
#define XSTAFF_BEAMEXT_DFLT 5
	if (config.hairpinMouthWidth < 0 || config.hairpinMouthWidth > 31)
			{ config.hairpinMouthWidth = HAIRPINMOUTH_DFLT; ERR(28); }
	if (config.mbRestBaseLen < 1) { config.mbRestBaseLen = MBREST_BASELEN_DFLT;  ERR(29); }
	if (config.mbRestAddLen < 0) { config.mbRestAddLen = MBREST_ADDLEN_DFLT;  ERR(30); }
	if (config.barlineDashLen < 1) { config.barlineDashLen = BARLINE_DASHLEN_DFLT; ERR(31); }
	if (config.crossStaffBeamExt < 0) { config.crossStaffBeamExt = XSTAFF_BEAMEXT_DFLT;  ERR(32); }

#define TITLEMARGIN_DFLT in2pt(1)
#define PAPERRECT_WIDTH_DFLT in2pt(17)/2
#define PAPERRECT_HT_DFLT in2pt(11)
	if (config.titleMargin < 0) { config.titleMargin = TITLEMARGIN_DFLT;  ERR(33); };

	if (config.paperRect.top!=0 || config.paperRect.left!=0)
		{ SetRect(&config.paperRect, 0, 0, PAPERRECT_WIDTH_DFLT, PAPERRECT_HT_DFLT); ERR(34); }
	if (config.paperRect.bottom<in2pt(3) || config.paperRect.bottom>in2pt(30)
		|| config.paperRect.right<in2pt(3) || config.paperRect.right>in2pt(30))
		{ SetRect(&config.paperRect, 0, 0, PAPERRECT_WIDTH_DFLT, PAPERRECT_HT_DFLT); ERR(34); }

#define PAGEMARG_TOP_DFLT in2pt(1)/2
#define PAGEMARG_LEFT_DFLT in2pt(1)/2
#define PAGEMARG_BOTTOM_DFLT in2pt(1)/2
#define PAGEMARG_RIGHT_DFLT in2pt(1)/2

	/* Check for ridiculously small or large margins. */
	
	if (!RectIsValid(config.pageMarg, 4, in2pt(5))) {
		if (config.pageMarg.top<4 || config.pageMarg.top>in2pt(5))
			{ config.pageMarg.top = PAGEMARG_TOP_DFLT; }
		if (config.pageMarg.left<4 || config.pageMarg.left>in2pt(5))
			{ config.pageMarg.left = PAGEMARG_LEFT_DFLT; }
		if (config.pageMarg.bottom<4 || config.pageMarg.bottom>in2pt(5))
			{ config.pageMarg.bottom = PAGEMARG_BOTTOM_DFLT; }
		if (config.pageMarg.right<4 || config.pageMarg.right>in2pt(5))
			{ config.pageMarg.right = PAGEMARG_RIGHT_DFLT; }
		ERR(35);
	}
	/* Check for margins crossing. */
	
	if (config.pageMarg.top+config.pageMarg.bottom>in2pt(6)) {
		config.pageMarg.top = PAGEMARG_TOP_DFLT;
		config.pageMarg.bottom = PAGEMARG_BOTTOM_DFLT;
		ERR(36);
	}
	if (config.pageMarg.left+config.pageMarg.right>in2pt(5)) {
		config.pageMarg.left = PAGEMARG_LEFT_DFLT;
		config.pageMarg.right = PAGEMARG_RIGHT_DFLT;
		ERR(36);
	}

#define PAGENUMMARG_TOP_DFLT in2pt(1)/2
#define PAGENUMMARG_LEFT_DFLT in2pt(1)/2
#define PAGENUMMARG_BOTTOM_DFLT in2pt(1)/2
#define PAGENUMMARG_RIGHT_DFLT in2pt(1)/2
	if (!RectIsValid(config.pageNumMarg, 4, in2pt(5))) {
		if (config.pageNumMarg.top<4 || config.pageNumMarg.top>in2pt(5))
			{ config.pageNumMarg.top = PAGENUMMARG_TOP_DFLT; }
		if (config.pageNumMarg.left<4 || config.pageNumMarg.left>in2pt(5))
			{ config.pageNumMarg.left = PAGENUMMARG_LEFT_DFLT; }
		if (config.pageNumMarg.bottom<4 || config.pageNumMarg.bottom>in2pt(5))
			{ config.pageNumMarg.bottom = PAGENUMMARG_BOTTOM_DFLT; }
		if (config.pageNumMarg.right<4 || config.pageNumMarg.right>in2pt(5))
			{ config.pageNumMarg.right = PAGENUMMARG_RIGHT_DFLT; }
		ERR(37);
	}
	
#define LEDGERS_DFLT 6
#define TSNUM_DFLT 4
#define TSDENOM_DFLT 4
#define RASTRAL_DFLT 5
#define RASTRAL0SIZE_DFLT pdrSize[0]
	if (config.defaultLedgers < 1 || config.defaultLedgers > MAX_LEDGERS)
			{ config.defaultLedgers = LEDGERS_DFLT; ERR(38); }
	if (TSNUMER_BAD(config.defaultTSNum)) 	{ config.defaultTSNum = TSNUM_DFLT; ERR(39); }
	if (TSDENOM_BAD(config.defaultTSDenom)) { config.defaultTSDenom = TSDENOM_DFLT; ERR(40); }
	if (config.defaultRastral < 0 || config.defaultRastral > MAXRASTRAL)
			{ config.defaultRastral = RASTRAL_DFLT; ERR(41); }
	if (config.rastral0size < 4 || config.rastral0size > 72)
			{ config.rastral0size = RASTRAL0SIZE_DFLT; ERR(42); }

#define MIN_RECVELOCITY_DFLT 10
#define MIN_RECDURATION_DFLT 50
#define MIDIDTHRU_DFLT 0
	if (config.minRecVelocity < 1 || config.minRecVelocity > MAX_VELOCITY)
			{ config.minRecVelocity = MIN_RECVELOCITY_DFLT; ERR(43); }
	if (config.minRecDuration < 1) { config.minRecDuration = MIN_RECDURATION_DFLT; ERR(44); }
#define MIDI_THRU
#ifdef MIDI_THRU
	/* FIXME: Does MIDI Thru work with Core MIDI? Needs thought/testing. */
	if (config.midiThru < 0) { config.midiThru = MIDIDTHRU_DFLT; ERR(45); }
#else
	config.midiThru = 0;
#endif		

#define DEFAULT_TEMPO_MM_DFLT 96
#define LOWMEMORY_DFLT 800
#define MINMEMORY_DFLT 320
	if (config.defaultTempoMM < MIN_BPM || config.defaultTempoMM > MAX_BPM)
			{ config.defaultTempoMM = DEFAULT_TEMPO_MM_DFLT; ERR(46); }
	if (config.lowMemory < 320 || config.lowMemory > 4096) { config.lowMemory = LOWMEMORY_DFLT; ERR(47); }
	if (config.lowMemory < config.minMemory) { config.lowMemory = LOWMEMORY_DFLT; ERR(47); }
	if (config.minMemory < 320 || config.minMemory > 4096) { config.minMemory = MINMEMORY_DFLT; ERR(48); }
#define NUMROWS_DFLT 1
#define NUMCOLS_DFLT 20
#define MAXROWS_DFLT 256
#define MAXCOLS_DFLT 256
	if (config.numRows < 1 || config.numRows > 100) { config.numRows = NUMROWS_DFLT; ERR(49); }
	if (config.numCols < 1 || config.numCols > 100) { config.numCols = NUMCOLS_DFLT; ERR(50); }
	if (config.maxRows < 4 || config.maxRows > 256) { config.maxRows = MAXROWS_DFLT; ERR(51); }
	if (config.numRows > config.maxRows) { config.maxRows = config.numRows; ERR(51); }
	if (config.maxCols < 4 || config.maxCols > 256) { config.maxCols = MAXCOLS_DFLT; ERR(52); }
	if (config.numCols > config.maxCols) { config.maxCols = config.numCols; ERR(52); }
#define HPAGESEP_DFLT 8
#define VPAGESEP_DFLT 8
#define HSCROLLSLOP_DFLT 16
#define VSCROLLSLOP_DFLT 16
	if (config.hPageSep < 0 || config.hPageSep > 80) { config.hPageSep = HPAGESEP_DFLT; ERR(53); }
	if (config.vPageSep < 0 || config.vPageSep > 80) { config.vPageSep = VPAGESEP_DFLT; ERR(54); }
	if (config.hScrollSlop < 0 || config.hScrollSlop > 80) { config.hScrollSlop = HSCROLLSLOP_DFLT; ERR(55); }
	if (config.vScrollSlop < 0 || config.vScrollSlop > 80) { config.vScrollSlop = VSCROLLSLOP_DFLT; ERR(56); }

#define MAX_SYNC_TOLERANCE_DFLT 60
#define ENLARGE_HILITE_DFLT 1
#define INFO_DISTUNITS_DFLT 0
#define MSHAKE_THRESH_DFLT 0
	if (config.maxSyncTolerance < 0) { config.maxSyncTolerance = MAX_SYNC_TOLERANCE_DFLT;  ERR(57); }
	if (config.enlargeHilite < 0 || config.enlargeHilite > 10)
			{ config.enlargeHilite = ENLARGE_HILITE_DFLT; ERR(58); }
	if (config.infoDistUnits < 0 || config.infoDistUnits > 2)		/* Max. from enum in InfoDialog.c */
			{ config.infoDistUnits = INFO_DISTUNITS_DFLT; ERR(59); }
	if (config.mShakeThresh < 0) { config.mShakeThresh = MSHAKE_THRESH_DFLT; ERR(60); }

#define NUMMASTERS_DFLT 64
	if (config.numMasters < 64) { config.numMasters = NUMMASTERS_DFLT; ERR(62); }
	
#define INDENT_FIRST_DFLT 47
#define MBREST_HT_DFLT 2
#define CHORDSYM_MUS_SIZE_DFLT 150
#define ENCLMARGIN_DFLT 2
	if (config.indentFirst < 0) { config.indentFirst = INDENT_FIRST_DFLT; ERR(63); }
	if (config.mbRestHeight < 1) { config.mbRestHeight = MBREST_HT_DFLT; ERR(64); }
	if (config.chordSymMusSize < 20 || config.chordSymMusSize > 400) { config.chordSymMusSize = CHORDSYM_MUS_SIZE_DFLT; ERR(65); }
	if (config.enclMargin < 0) { config.enclMargin = ENCLMARGIN_DFLT; ERR(66); }

#define LEGATO_PCT_DFLT 95
#define DEFAULTPATCH_DFLT 1
#define WHICH_MIDI_DFLT MIDISYS_CM
	if (config.legatoPct < 1) { config.legatoPct = LEGATO_PCT_DFLT; ERR(67); }
	if (config.defaultPatch < 1 || config.defaultPatch > MAXPATCHNUM)
			{ config.defaultPatch = DEFAULTPATCH_DFLT; ERR(68); }
			
	if (config.whichMIDI < 0 || config.whichMIDI > MIDISYS_NONE) { config.whichMIDI = WHICH_MIDI_DFLT; ERR(69); }
	
#define MUS_FONTSIZE_DFLT 0
#define REST_MV_OFFSET_DFLT 2
#define AUTOBEAM_OPTIONS_DFLT 0
#define NOTEOFF_VELOCITY_DFLT 64
#define FEEDBACK_NOTEON_VELOCITY_DFLT 64
#define DEFAULT_CHANNEL_DFLT 1
#define RAINYDAY_MEMORY_DFLT 640
#define TRY_TUP_LEVELS_DFLT 21
#define JUSTIFY_WARN_THRESH 15
	/* Set min. for musFontSizeOffset so even if rastral 0 is only 4 pts, will still give >0 PS size */
	if (config.musFontSizeOffset < -3) { config.musFontSizeOffset = MUS_FONTSIZE_DFLT; ERR(70); }

	if (config.restMVOffset < 0 || config.restMVOffset > 20)
			{ config.restMVOffset = REST_MV_OFFSET_DFLT; ERR(72); }
	if (config.autoBeamOptions < 0 || config.autoBeamOptions > 3)
			{ config.autoBeamOptions = AUTOBEAM_OPTIONS_DFLT; ERR(73); }
	if (config.noteOffVel < 1 || config.noteOffVel > MAX_VELOCITY)
			{ config.noteOffVel = NOTEOFF_VELOCITY_DFLT; ERR(74); }
	if (config.feedbackNoteOnVel < 1 || config.feedbackNoteOnVel > MAX_VELOCITY)
			{ config.feedbackNoteOnVel = FEEDBACK_NOTEON_VELOCITY_DFLT; ERR(75); }
	if (config.defaultChannel < 1 || config.defaultChannel > MAXCHANNEL)
			{ config.defaultChannel = DEFAULT_CHANNEL_DFLT; ERR(76); }
	if (config.rainyDayMemory < 320 || config.rainyDayMemory > 4096)
			{ config.rainyDayMemory = RAINYDAY_MEMORY_DFLT; ERR(78); }
	if (config.tryTupLevels < 1 || config.tryTupLevels > 321)
			{ config.tryTupLevels = TRY_TUP_LEVELS_DFLT; ERR(79); }
	if (config.justifyWarnThresh < 10) { config.justifyWarnThresh = JUSTIFY_WARN_THRESH; ERR(80); }

#define METRO_CHANNEL_DFLT 1
#define METRO_NOTENUM_DFLT 77
#define METRO_VELO_DFLT 90
#define METRO_DUR_DFLT 50
	if (config.metroChannel < 1 || config.metroChannel > MAXCHANNEL)
			{ config.metroChannel = METRO_CHANNEL_DFLT; ERR(81); }
	if (config.metroNote < 1 || config.metroNote > MAX_NOTENUM)
			{ config.metroNote = METRO_NOTENUM_DFLT; ERR(82); }
	if (config.metroVelo < 1 || config.metroVelo > MAX_VELOCITY)
			{ config.metroVelo = METRO_VELO_DFLT; ERR(83); }
	if (config.metroDur < 1 || config.metroDur > 999) { config.metroDur = METRO_DUR_DFLT; ERR(84); }

#define CHORDSYM_SMALLSIZE_DFLT 1
#define CHORDSYM_SUPERSCR_DFLT 1
#define CHORDSYM_STKLEAD_DFLT 10
	if (config.chordSymSmallSize < 1 || config.chordSymSmallSize > 127)
			{ config.chordSymSmallSize = CHORDSYM_SMALLSIZE_DFLT; ERR(85); }
	if (config.chordSymSuperscr < 0 || config.chordSymSuperscr > 127)
			{ config.chordSymSuperscr = CHORDSYM_SUPERSCR_DFLT; ERR(86); }
	if (config.chordSymStkLead < -20 || config.chordSymStkLead > 127)
			{ config.chordSymStkLead = CHORDSYM_STKLEAD_DFLT; ERR(87); }

#define TUPLETNUMSIZE_DFLT 110
#define TUPLETCOLONSIZE_DFLT 60
#define OCTAVENUMSIZE_DFLT 110
	if (config.tupletNumSize < 0 || config.tupletNumSize > 127)
			{ config.tupletNumSize = TUPLETNUMSIZE_DFLT; ERR(92); }
	if (config.tupletColonSize < 0 || config.tupletColonSize > 127)
			{ config.tupletColonSize = TUPLETCOLONSIZE_DFLT; ERR(93); }
	if (config.octaveNumSize < 0 || config.octaveNumSize > 127)
			{ config.octaveNumSize = OCTAVENUMSIZE_DFLT; ERR(94); }

#define LINE_LW_DFLT 25
#define LEDGERL_LEN_DFLT 48
#define LEDGERL_OTHERLEN_DFLT 12
#define SLUR_DASHLEN_DFLT 3
#define SLUR_SPACELEN_DFLT 3
	if (config.lineLW < 5 || config.lineLW > 127) { config.lineLW = LINE_LW_DFLT; ERR(95); }
	if (config.ledgerLLen < 32) { config.ledgerLLen = LEDGERL_LEN_DFLT; ERR(96); }
	if (config.ledgerLOtherLen < 0) { config.ledgerLOtherLen = LEDGERL_OTHERLEN_DFLT; ERR(97); }
	if (config.slurDashLen < 1) { config.slurDashLen = SLUR_DASHLEN_DFLT; ERR(98); }
	if (config.slurSpaceLen < 1) { config.slurSpaceLen = SLUR_SPACELEN_DFLT; ERR(99); }

#define COURTESYACC_LXD_DFLT 6
#define COURTESYACC_RXD_DFLT 8
#define COURTESYACC_YD_DFLT 8
#define COURTESYACC_SIZE_DFLT 100
	if (config.courtesyAccLXD < 0) { config.courtesyAccLXD = COURTESYACC_LXD_DFLT; ERR(100); }
	if (config.courtesyAccRXD < 0) { config.courtesyAccRXD = COURTESYACC_RXD_DFLT; ERR(101); }
	if (config.courtesyAccYD < 0) { config.courtesyAccYD = COURTESYACC_YD_DFLT; ERR(102); }
	if (config.courtesyAccPSize < 20) { config.courtesyAccPSize = COURTESYACC_SIZE_DFLT; ERR(103); }

#define QUANTIZEBEAMYPOS_DFLT 3
	if (config.quantizeBeamYPos<0) { config.quantizeBeamYPos = QUANTIZEBEAMYPOS_DFLT; ERR(104); };

#define ENLARGE_NRHILITE_H_DFLT 1
#define ENLARGE_NRHILITE_V_DFLT 1
	if (config.enlargeNRHiliteH < 0 || config.enlargeNRHiliteH > 10)
			{ config.enlargeNRHiliteH = ENLARGE_NRHILITE_H_DFLT; ERR(105); }
	if (config.enlargeNRHiliteV < 0 || config.enlargeNRHiliteV > 10)
			{ config.enlargeNRHiliteV = ENLARGE_NRHILITE_V_DFLT; ERR(106); }

	/* No validity check at this time for MIDI fields. */

	if (nerr>0) {
		LogPrintf(LOG_NOTICE, "\n");
        LogPrintf(LOG_WARNING, "%d ERROR(S) FOUND (first bad field is no. %d).\n", nerr, firstErr);
		GetIndCString(fmtStr, INITERRS_STRS, 5);		/* "CNFG resource in Prefs has [n] illegal value(s) (first bad field is no. %d)" */
		sprintf(strBuf, fmtStr, nerr, firstErr);
		CParamText(strBuf, "", "", "");
		if (CautionAdvise(CNFG_ALRT)==OK) ExitToShell();
		return False;
	}
    else
        LogPrintf(LOG_NOTICE, "No errors found.  (CheckConfig)\n");
		return True;
}


/* Install our configuration data from the Prefs file; also check for, report, and
correct any illegal values. Assumes the Prefs file is the current resource file. */

static Boolean GetConfig(void)
{
	Handle cnfgH;
	long cnfgSize;
	
	cnfgH = Get1Resource('CNFG', THE_CNFG);
	if (!GoodResource(cnfgH)) {
		GetIndCString(strBuf, INITERRS_STRS, 4);		/* "Can't find the CNFG resource" */
		CParamText(strBuf, "", "", "");
		if (CautionAdvise(CNFG_ALRT)==OK) ExitToShell();
		
		/*
		 * Since we couldn't find the configuration data, set all checkable fields to
		 * illegal values so they'll be caught and corrected below; set the few 
		 * uncheckable ones to reasonable values.
		 */
		config.maxDocuments = -1;

		config.stemLenNormal = config.stemLen2v = -1;
		config.stemLenOutside = config.stemLenGrace = -1;

		config.spAfterBar = config.minSpBeforeBar = -1;
		config.minRSpace = config.minLyricRSpace = -1;
		config.minRSpace_KSTS_N = -1;

		config.hAccStep = -1;

		config.stemLW = config.barlineLW = config.ledgerLW = config.staffLW = -1;
		config.enclLW = -1;

		config.beamLW = config.hairpinLW = config.dottedBarlineLW = -1;
		config.mbRestEndLW = config.nonarpLW = -1;

		config.graceSlashLW = config.slurMidLW = config.tremSlashLW = -1;
		config.slurCurvature = config.tieCurvature = -1;
		config.relBeamSlope = -1;
		
		config.hairpinMouthWidth = -1;
		config.mbRestBaseLen = config.mbRestAddLen = -1;
		config.barlineDashLen = 0;
		config.crossStaffBeamExt = -1;
		
		config.slashGraceStems = 0;

		config.titleMargin = -1;

		SetRect(&config.paperRect, 0, 0, 0, 0);
		SetRect(&config.pageMarg, 0, 0, 0, 0);
		SetRect(&config.pageNumMarg, 0, 0, 0, 0);
		
		config.defaultLedgers = 0;
		config.defaultTSNum = config.defaultTSDenom = -1;
		config.defaultRastral = -1;
		config.rastral0size = 0;
		
		config.defaultTempoMM = 0;
		config.minRecVelocity = 0;
		config.minRecDuration = 0;
		config.midiThru = -1;
		
		config.lowMemory = config.minMemory = -1;
		
		config.toolsPosition.h = -83;
		config.toolsPosition.v = -8;

		config.numRows = config.numCols = -1;
		config.maxRows = config.maxCols = -1;
		config.hPageSep = config.vPageSep = -1;
		config.hScrollSlop = config.vScrollSlop = -1;
		config.origin.h = config.origin.v = -20000;
		
		config.maxSyncTolerance = -1;
		config.dblBarsBarlines = 0;
		config.enlargeHilite = -1;
		config.delRedundantAccs = 1;
		config.infoDistUnits = -1;
		config.mShakeThresh = -1;

		config.numMasters = -1;
		
		config.indentFirst = -1;
		config.chordSymMusSize = 0;
		config.fastScreenSlurs = 0;
		config.legatoPct = -1;
		config.defaultPatch = -1;
		config.whichMIDI = -1;
		config.enclMargin = -1;
		config.mbRestHeight = -1;
		config.musFontSizeOffset = -127;
		config.restMVOffset = -1;
		config.autoBeamOptions = -1;
		config.noteOffVel = -1;
		config.feedbackNoteOnVel = -1;
		config.defaultChannel = -1;
		config.defaultInputDevice = 0;
		config.defaultOutputChannel = -1;
		config.defaultOutputDevice = 0;
		
		config.rainyDayMemory = -1;
		config.tryTupLevels = -1;
		config.justifyWarnThresh = -1;
		
		config.metroChannel = -1;
		config.metroNote = -1;
		config.metroVelo = -1;
		config.metroDur = -1;
		config.metroDevice = 0;
		
		config.chordSymSmallSize = -1;	
		config.chordSymSuperscr = -1;
		config.chordSymStkLead = -127;

		config.tupletNumSize = 110;
		config.tupletColonSize = 60;
		config.octaveNumSize = 120;
		config.lineLW = -1;
		config.ledgerLLen = config.ledgerLOtherLen = -1;
		config.slurDashLen = config.slurSpaceLen = -1;

		config.courtesyAccLXD = config.courtesyAccRXD = -127;
		config.courtesyAccYD = -127;
		config.courtesyAccPSize = -127;

		/* FIXME: What about Core MIDI fields (cmMetroDev thru cmDfltOutputChannel)? */
		
		config.quantizeBeamYPos = -1;

		config.enlargeNRHiliteH = -1;
		config.enlargeNRHiliteV = -1;

		config.thruChannel = 0;
		config.thruDevice = noUniqueID;
	}
	 else {
		cnfgSize = GetResourceSizeOnDisk(cnfgH);
		if (cnfgSize!=(long)sizeof(Configuration))
			if (CautionAdvise(CNFGSIZE_ALRT)==OK) ExitToShell();
		config = *(Configuration *)(*cnfgH);

		EndianFixConfig();
		
		if (OptionKeyDown() && ControlKeyDown() && CmdKeyDown()) {
			GetIndCString(strBuf, INITERRS_STRS, 11);		/* "Skipping checking the CNFG" */
			CParamText(strBuf, "", "", "");
			NoteInform(GENERIC_ALRT);
			goto Finish;
		}

	}

	/* Display CNFG fields in the log. Then check for problems, and, if any are found,
		display the (presumably corrected) values. */
	if (DETAIL_SHOW) DisplayConfig();
	if (!CheckConfig()) {
		LogPrintf(LOG_WARNING, "Illegal fields have been set to default values.  (GetConfig)\n");
		DisplayConfig();
	}
	
Finish:
	/* Make any final adjustments. NB: Must undo these in UpdateSetupFile()! */
	
	config.maxDocuments++;								/* to allow for clipboard doc */

	return True;
}


/* Make the heap _almost_ as large as possible (so we have a little extra stack space),
and allocate as many Master Pointers as possible up to numMasters. Return True if all
went well, False if not. */

static Boolean InitMemory(short numMasters)
{
#ifdef CARBON_NOMORE
	THz thisZone;  long heapSize;  short orig;
	
	/* Increase stack size by decreasing heap size (thanks to Dave Winzler for the method). */
	heapSize = (long)GetApplLimit()-(long)GetZone();
	heapSize -= 32000;
	SetApplLimit((Ptr)ApplicationZone()+heapSize);

	MaxApplZone();
	thisZone = ApplicationZone();
	orig = thisZone->moreMast;
	thisZone->moreMast = numMasters;
	do {
		MoreMasters();
	} while (MemError()!=noErr && (thisZone->moreMast=(numMasters >>= 1))!=0);
	thisZone->moreMast = orig;
#endif
	
	return(numMasters >= 64);
}


/* -------------------------------------------------------------------------------------- */

/* Provide the pre-allocated clipboard document with default values. */
	
static Boolean PrepareClipDoc(void)
{
	WindowPtr w;  char title[256];
	LINK pL;  long junkVersion;

	clipboard = documentTable;
	clipboard->inUse = True;
	w = GetNewWindow(docWindowID, NULL, (WindowPtr)-1);
	if (w == NULL) return(False);
	
	clipboard->theWindow = w;
	SetWindowKind(w, DOCUMENTKIND);

	//		((WindowPeek)w)->spareFlag = True;
	ChangeWindowAttributes(w, kWindowFullZoomAttribute, kWindowNoAttributes);

	if (!BuildDocument(clipboard, NULL, 0, NULL, &junkVersion, True))
		return False;
	for (pL=clipboard->headL; pL!=clipboard->tailL; pL=DRightLINK(clipboard, pL))
		if (DObjLType(clipboard, pL)==MEASUREtype)
			{ clipFirstMeas = pL; break; }
	clipboard->canCutCopy = False;
	GetIndCString(title, MiscStringsID, 2);
	SetWCTitle((WindowPtr)clipboard, title);
	return True;
}
	
Boolean BuildEmptyDoc(Document *doc) 
{
	if (!InitAllHeaps(doc)) { NoMoreMemory(); return False; }
	InstallDoc(doc);
	BuildEmptyList(doc,&doc->headL,&doc->tailL);
	
	doc->selStartL = doc->selEndL = doc->tailL;			/* Empty selection  */	
	return True;
}

	
//#define TEST_MDEF_CODE
#ifdef TEST_MDEF_CODE
// add some interesting sample items

static void AddSampleItems(MenuRef menu);
static void AddSampleItems(MenuRef menu)
{
	MenuItemIndex	item;
	
	AppendMenuItemTextWithCFString(menu, CFSTR("Checkmark"), 0, 0, &item);
	CheckMenuItem(menu, item, True);
	AppendMenuItemTextWithCFString(menu, CFSTR("Dash"), 0, 0, &item);
	SetItemMark(menu, item, '-');
	AppendMenuItemTextWithCFString(menu, CFSTR("Diamond"), 0, 0, &item);
	SetItemMark(menu, item, kDiamondCharCode);
	AppendMenuItemTextWithCFString(menu, CFSTR("Bullet"), 0, 0, &item);
	SetItemMark(menu, item, kBulletCharCode);
	AppendMenuItemTextWithCFString(menu, NULL, kMenuItemAttrSeparator, 0, NULL);
	AppendMenuItemTextWithCFString(menu, CFSTR("Section Header"), kMenuItemAttrSectionHeader, 0, NULL);
	AppendMenuItemTextWithCFString(menu, CFSTR("Indented item 1"), 0, 0, &item);
	SetMenuItemIndent(menu, item, 1);
	AppendMenuItemTextWithCFString(menu, CFSTR("Indented item 2"), 0, 0, &item);
	SetMenuItemIndent(menu, item, 1);
}
#endif

/* Initialize application-specific globals, etc. */

Boolean InitGlobals(void)
	{
		long size;
		Document *doc;
		
		InitNightingale();

		/* Create an empty Document table for max number of docs in configuration */
		
		size = (long)sizeof(Document) * config.maxDocuments;
		documentTable = (Document *)NewPtr(size);
		if (!GoodNewPtr((Ptr)documentTable))
			{ OutOfMemory(size);  return False; }
		
		topTable = documentTable + config.maxDocuments;
		for (doc=documentTable; doc<topTable; doc++) doc->inUse = False;
		
		/* Preallocate the clipboard and search documents */
		
		if (!PrepareClipDoc()) return False;
			
		/* Set up the global scrap reference */
		
		GetCurrentScrap(&gNightScrap);
	
		/* Allocate various non-relocatable things first */
		
		tmpStr = (unsigned char *)NewPtr(256);
		if (!GoodNewPtr((Ptr)tmpStr))
			{ OutOfMemory(256L); return False; }
		
		updateRectTable = (Rect *)NewPtr(MAX_UPDATE_RECTS * sizeof(Rect));
		if (!GoodNewPtr((Ptr)updateRectTable))
			{ OutOfMemory((long)MAX_UPDATE_RECTS * sizeof(Rect)); return False; }
		
		/* Pull the permanent menus in from resources and install them in menu bar. */
		
		appleMenu = GetMenu(appleID);		if (!appleMenu) return False;
		AppendResMenu(appleMenu, 'DRVR');
		InsertMenu(appleMenu, 0);
		
		fileMenu = GetMenu(fileID);			if (!fileMenu) return False;
		InsertMenu(fileMenu, 0);
		
		editMenu = GetMenu(editID);			if (!editMenu) return False;
		InsertMenu(editMenu, 0);
		
#ifdef PUBLIC_VERSION
		if (CmdKeyDown() && OptionKeyDown()) {
			AppendMenu(editMenu, "\p(-");
			AppendMenu(editMenu, "\pBrowser");
			AppendMenu(editMenu, "\pDebug...");
			AppendMenu(editMenu, "\pDelete Objects");
		}
#else
		testMenu = GetMenu(testID);			if (!testMenu) return False;
		InsertMenu(testMenu, 0);
#endif
		
		scoreMenu = GetMenu(scoreID);		if (!scoreMenu) return False;
		InsertMenu(scoreMenu, 0);
		
		notesMenu = GetMenu(notesID);		if (!notesMenu) return False;
		InsertMenu(notesMenu, 0);
		
		groupsMenu = GetMenu(groupsID);		if (!groupsMenu) return False;
		InsertMenu(groupsMenu, 0);
		
		viewMenu = GetMenu(viewID);			if (!viewMenu) return False;
		InsertMenu(viewMenu, 0);
		
		magnifyMenu = GetMenu(magnifyID);	if (!magnifyMenu) return False;
		InsertMenu(magnifyMenu, hierMenu);
		
		/* We install only one of the following menus at a time. */
		
		playRecMenu = GetMenu(playRecID);	if (!playRecMenu) return False;
		InsertMenu(playRecMenu, 0);
		
#ifdef TEST_MDEF_CODE
		MenuRef			customMenu;
		MenuDefSpec		defSpec;
		
		defSpec.defType = kMenuDefProcPtr;
		defSpec.u.defProc = NewMenuDefUPP(NMDefProc);

		// Create a custom menu
		//CreateCustomMenu( &defSpec, 1201, 0, &customMenu );
		//SetMenuTitleWithCFString( customMenu, CFSTR("Sample [Custom]") );
		//InsertMenu( customMenu, 0 );
		//AddSampleItems( customMenu );
		CreateCustomMenu(&defSpec, TWODOTDUR_MENU, 0, &customMenu);
		SetMenuTitleWithCFString(customMenu, CFSTR("Graphic MDEF"));
		InsertMenu(customMenu, 0);
		AddSampleItems(customMenu);
#endif
		
		masterPgMenu = GetMenu(masterPgID);	if (!masterPgMenu) return False;
		
		formatMenu = GetMenu(formatID);		if (!formatMenu) return False;
		
		DrawMenuBar();
		
		/* Pull in other miscellaneous things from resources. */
		
		GetIndPattern(&paperBack, MiscPatternsID, 1);
		GetIndPattern(&diagonalDkGray, MiscPatternsID, 2);
		GetIndPattern(&diagonalLtGray, MiscPatternsID, 3);
		GetIndPattern(&otherLtGray, MiscPatternsID, 4);
		
		/* Call stub routines to load all segments needed for selection, so user
		doesn't have to wait for them to load when they make the the first selection. */

		XLoadEditScoreSeg();
		
		return True;
	}


/* Install the addresses of the functions that the Event Manager will call when any of
the Apple events we handle (for now, only the core events) come in over the transom. */

static AEEventHandlerUPP oappUPP, odocUPP, pdocUPP, quitUPP;
#ifdef FUTUREEVENTS
static AEEventHandlerUPP editUPP, closUPP;
#endif

static void InstallCoreEventHandlers(void)
{
	OSErr err = noErr;
	
	/* Leave these UPP's permanently allocated. */
	
	oappUPP = NewAEEventHandlerUPP(HandleOAPP);
	odocUPP = NewAEEventHandlerUPP(HandleODOC);
	pdocUPP = NewAEEventHandlerUPP(HandlePDOC);
	quitUPP = NewAEEventHandlerUPP(HandleQUIT);

	if (quitUPP) {
		err = AEInstallEventHandler(kCoreEventClass,kAEOpenApplication,oappUPP,0,False);
		if (err) goto broken;
		err = AEInstallEventHandler(kCoreEventClass,kAEOpenDocuments,odocUPP,0,False);
		if (err) goto broken;
		err = AEInstallEventHandler(kCoreEventClass,kAEPrintDocuments,pdocUPP,0,False);
		if (err) goto broken;
		err = AEInstallEventHandler(kCoreEventClass,kAEQuitApplication,quitUPP,0,False);
		if (err) goto broken;
	}
	
#ifdef FUTUREEVENTS
	/*	See the note where <myAppType> is set on distinguishing run-time env. */

	editUPP = NewAEEventHandlerUPP(HandleEdit);
	closUPP = NewAEEventHandlerUPP(HandleClose);

	if (closUPP) {
		err = AEInstallEventHandler(myAppType,'Edit',editUPP,0,False);
		if (err) goto broken;
		err = AEInstallEventHandler(myAppType,'Clos',closeUPP,0,False);
	}
#endif

broken:
	if (err!=noErr) {
		LogPrintf(LOG_ERR, "AEInstallEventHandler failed.  (InstallCoreEventHandlers)\n");
	}
}
