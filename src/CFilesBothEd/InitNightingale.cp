/* InitNightingale.c for Nightingale, rev. for v.99 */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

//#include <MIDI.h>
#include <CoreMIDI/MidiServices.h>
// MAS
#include "CarbonTemplates.h"

void 		InitNightingale(void);
static Boolean	DoSplashScreen(void);
static void		InitFinal(void);
static Boolean	InitAllCursors(void);
static void		InitNightFonts(void);
static Boolean	InitNightGlobals(void);
static Boolean	InitTables(void);
static Boolean	ExpireTimeOK(void);
static void 		PrintInfo(void);
static void		InitMusicFontStuff(void);
static Boolean	InitMIDISystem(void);
static void 		CheckScrFonts(void);

void NExitToShell(char *msg);

/* ------------------------------------------------------------- InitNightingale -- */
/* One-time initialization of Nightingale-specific stuff. Assumes the Macintosh
toolbox has already been initialized and the config struct filled in. */

void InitNightingale()
{
	InitNightFonts();									/* Do this first to be ready for dialogs */

	if (!config.fastLaunch)
		if (!DoSplashScreen()) NExitToShell("Splash Screen");
#ifdef COPY_PROTECT
	if (!CopyProtectionOK(TRUE)) NExitToShell("Copy Protect");
#endif
	if (!InitAllCursors()) NExitToShell("Init All Cursors");
	if (!InitNightGlobals()) NExitToShell("Init Night Globals");
	MEInitCaretSystem();
	
	if (!InitTables()) NExitToShell("Init Tables");
	if (!ExpireTimeOK()) NExitToShell("Expire Time");
	PrintInfo();
	InitMusicFontStuff();
	if (!InitMIDISystem()) NExitToShell("Init Midi");
}

void NExitToShell(char *msg)
{
	CParamText(msg, "", "", "");
	StopInform(GENERIC_ALRT);
	ExitToShell();
}

/* Display splash screen. If all OK, return TRUE; if there's a problem, return FALSE.

In Nightingale, the splash screen we display shows the owner's name and organization,
to discourage illegal copying. We also check (but don't display) the copy serial number:
if it's wrong, return FALSE.

In NoteView, the splash screen is purely for the user's benefit. */

#define NAME_DI 1
#define ABOUT_DI 2
#define NO_SERIAL_NUM		/* we no longer show the registration stuff */

static Boolean DoSplashScreen()
{
	DialogPtr dlog; GrafPtr oldPort;
	short aShort; Handle aHdl; Rect aRect;
	char serialStr[256];

	GetPort(&oldPort);
	dlog = GetNewDialog(OWNER_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) return FALSE;
	SetPort(GetDialogWindowPort(dlog));

#ifdef VIEWER_VERSION
	SetDlgFont(dlog, textFontNum, textFontSmallSize, 0);
	CenterWindow(GetDialogWindow(dlog), 65);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();
	ditem = OK-1;
	do {
		ModalDialog(OKButFilter, &ditem);				/* Handle dialog events */
		if (ditem==ABOUT_DI) DoAboutBox(FALSE);
	} while (ditem!=OK);
	WaitCursor();
#else
	GetDialogItem(dlog, NAME_DI, &aShort, &aHdl, &aRect);

	/* If the owner's name and organization aren't filled in, it's no big deal:
		they should have the default values. */
	
 #ifndef NO_SERIAL_NUM
	GetIndCString(userName, 1000, 1);
	GetIndCString(userOrg, 1000, 2);

	GetIndCString(fmtStr, INITERRS_STRS, 14);			/* "This copy is registered to %s, %s" */
	sprintf(strBuf, fmtStr, userName, userOrg); 
	SetDialogItemCText(aHdl, strBuf);
 #endif

	CenterWindow(GetDialogWindow(dlog), 65);
	ShowWindow(GetDialogWindow(dlog));
	UpdateDialogVisRgn(dlog);

	if (!GetSN(serialStr)) return FALSE;				/* The serial no. must check but don't care what it is */

 #ifdef NO_SERIAL_NUM	/* not as much to read */
	SleepTicksWaitButton(120L);							/* So user has time to read it */
 #else
	SleepTicksWaitButton(180L);							/* So user has time to read the message */
 #endif
#endif

	HideWindow(GetDialogWindow(dlog));
	DisposeDialog(dlog);										/* Free heap space */
	SetPort(oldPort);

	return TRUE;
}

/* Initialize the cursor globals and the cursor list. */

static Boolean InitAllCursors()
{
	INT16 i;

	WaitCursor();

	handCursor = GetCursor(RHAND_CURS); if (!handCursor) goto Error;
	threadCursor = GetCursor(THREAD_CURS); if (!threadCursor) goto Error;
	genlDragCursor = GetCursor(GENLDRAG_CURS); if (!genlDragCursor) goto Error;
	
	if (nsyms>MAX_CURSORS) {
		MayErrMsg("InitAllCursors: Need %ld cursors but only allocated %ld",
					(long)nsyms, (long)MAX_CURSORS);
		return FALSE;
	}
	
	for (i = 0;i<nsyms; i++)								/* Fill in list of cursor handles */
		if (symtable[i].cursorID>=100)
			cursorList[i] = GetCursor(symtable[i].cursorID);
		else
			cursorList[i] = (CursHandle)0;
	arrowCursor = currentCursor = GetCursor(ARROW_CURS);
	return TRUE;
	
Error:
	GetIndCString(strBuf, INITERRS_STRS, 7);		/* "Can't find cursor resources" */
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
	return FALSE;
}


/* Set globals describing our standard text font and our standard music font. */

void InitNightFonts()
{
	textFontNum = applFont;
	
	/*
	 * The "magic no." in next line should theoretically go away: we should get the
	 * system font size from the Script Manager if it's present (and it should always be). However,
	 * However, in every system I've seen, the system font size is 12, so I doubt it's
	 * a big deal.
	 */
	textFontSize = 12;
	textFontSmallSize = textFontSize-3;

	GetFNum("\pSonata", &sonataFontNum);				/* Get ID of Adobe Sonata font */
#ifdef NOMORE		/* Nothing to do with new music font scheme. */
	if (config.musicFontID!=0) {							/* Get ID of font to use for music chars. */
		musicFontNum = config.musicFontID;
		SetResLoad(FALSE);
		fontHdl = GetResource('FOND', musicFontNum);
		SetResLoad(TRUE);
		GetResInfo(fontHdl, &fontID, &fontType, (unsigned char *)fontName);
		PToCString((unsigned char *)fontName);
		if (strlen(fontName)==0) {
			GetIndCString(notFoundStr, INITERRS_STRS, 19);	/* "NOT FOUND" */
			strcpy(fontName, (char *)notFoundStr);
		}
		if (!config.fastLaunch) {
			GetIndCString(fmtStr, INITERRS_STRS, 15);		/* "The Nightingale Prefs file specifies the font with ID %d (%s) as the music font: Nightingale will use it instead of Sonata." */
			sprintf(strBuf, fmtStr, musicFontNum, fontName);
			CParamText(strBuf, "", "", "");
			NoteInform(GENERIC_ALRT);
		}
	}
	else
		musicFontNum = sonataFontNum;						/* System font if Sonata isn't available! */
#endif
}


/* Allocate <endingString> and get label strings for Endings into it. Return the
number of strings found, or -1 if there's an error (probably out of memory). */

static INT16 InitEndingStrings(void);
static INT16 InitEndingStrings()
{
	char str[256]; INT16 n, strOffset;
	
	endingString = NewPtr(MAX_ENDING_STRINGS*MAX_ENDING_STRLEN);
	if (!GoodNewPtr(endingString)) return -1;
	
	/*
	 * Get strings up to MAX_ENDING_STRINGS from a resource, terminated by an empty
	 * string or the last string present. 1st label is string 0 but index 1.
	 */	
	n = 1;
	do {
		GetIndCString(str, ENDING_STRS, n);
		strOffset = (n-1)*MAX_ENDING_STRLEN;
		GoodStrncpy(&endingString[strOffset], str, MAX_ENDING_STRLEN-1);

	 	/* The 1st label (for code 0) should be ignored, so be sure it doesn't stop things. */
	 	 
		if (n==1) GoodStrncpy(str, "X", MAX_ENDING_STRLEN-1);
		n++;
	} while (strlen(str)>0 && n<MAX_ENDING_STRINGS);
	
	return n-1;
}


/* Initialize Nightingale globals which do not belong to individual documents. */

static Boolean InitNightGlobals()
{
	long		*locZero=0;
	char		fmtStr[256];
	INT16		j;
	
	/*
	 *	Since initialization routines can (and as of this writing do) use the coordinate-
	 * conversion macros, set up variables used by those macros as if no magnification
	 * is in effect.
	 */	
	magShift = 4;
	magRound = 8;
		
	outputTo = toScreen;										/* Initial output device */
	clickMode = ClickSelect;
	lastCopy = COPYTYPE_CONTENT;

	/* Initialize our high-resolution (??but very inaccurate!) delay function */
	
	InitSleepMS();

	/* Find input char. codes for important symbol table entries */
	
	for (j=0; j<nsyms; j++)
		if (symtable[j].objtype==MEASUREtype && symtable[j].subtype==BAR_SINGLE)
			{ singleBarInChar = symtable[j].symcode; break; }

	for (j=0; j<nsyms; j++)
		if (symtable[j].objtype==SYNCtype && symtable[j].subtype==0
		&&  symtable[j].durcode==QTR_L_DUR)
			{ qtrNoteInChar = symtable[j].symcode; break; }

	InitDurStrings();
	
	maxEndingNum = InitEndingStrings();
	if (maxEndingNum<0) {
		NoMoreMemory();
		return FALSE;
	}
	else if (maxEndingNum<5) {
		GetIndCString(fmtStr, INITERRS_STRS, 26);	/* "Found only %d Ending strings: Ending labels may not be shown." */
		sprintf(strBuf, fmtStr, maxEndingNum);
		CParamText(strBuf, "", "", "");
		NoteInform(GENERIC_ALRT);
	}
	
	return TRUE;
}

/* Read 'BBX#', 'MCMp' and 'MCOf' resources for alternative music fonts, and store
their information in newly allocated musFontInfo[]. Return TRUE if OK; FALSE if error.
Assumes that each font, including Sonata, has one of all three types of resource
mentioned above, and that the resource ID's for a font's three types match.  For
example, "Petrucci" should have 'BBX#' 129, 'MCMp' 129 and 'MCOf' 129.
-JGG, 4/25/01 */

static Boolean InitMusFontTables()
{
	short		i, nRes, resID, curResFile;
	INT16		*w, ch, count, xw, xl, yt, xr, yb, index;
	unsigned char *b;
	Handle	resH;
	Size		nBytes;
	OSType	resType;
	Str255	resName;

	curResFile = CurResFile();
	UseResFile(appRFRefNum);

	nRes = Count1Resources('BBX#');
	numMusFonts = nRes;

	/* Allocate musFontInfo[] large enough to hold all the music fonts we have resources for. */
	nBytes = numMusFonts * (Size)sizeof(MusFontRec);
	musFontInfo = (MusFontRec *)NewPtr(nBytes);
	if (!GoodNewPtr((Ptr)musFontInfo)) {
		OutOfMemory((long)nBytes);
		return FALSE;
	}

	/* Read 'BBX#' (character bounding box) info. */
	index = 0;
	for (i = 1; i<=nRes; i++) {
		resH = Get1IndResource('BBX#', i);
		if (!GoodResource(resH)) goto error;
		GetResInfo(resH, &resID, &resType, resName);
		if (ReportResError()) goto error;
		if (resName[0]>31)			/* font name must have < 32 chars */
			goto error;
		Pstrcpy(musFontInfo[index].fontName, resName);
		GetFNum(resName, &musFontInfo[index].fontID);

		for (ch = 0; ch<256; ch++)
			musFontInfo[index].cBBox[ch].left = 
			musFontInfo[index].cBBox[ch].top = 
			musFontInfo[index].cBBox[ch].right = 
			musFontInfo[index].cBBox[ch].bottom = 0;

		w = (INT16 *)(*resH);
		count = *w++;
		while (count-- > 0) {
			ch = *w++;
			xw = *w++;
			xl = *w++; yb = -(*w++); xr = *w++; yt = -(*w++);
			if (ch>=0 && ch<256)
				SetRect(&musFontInfo[index].cBBox[ch], xl, yt, xr, yb);
			else
				MayErrMsg("InitMusFontTables: 'BBX#' %d: ch=%d IS ILLEGAL\n", resID);
		}
		ReleaseResource(resH);
		index++;
	}

	/* NOTE: For the other resources, we use Get1NamedResource instead of 
		Count1Resources, because we can't rely on the order of resources in
		the resource map to be the same as with the 'BBX#' resource. */

	/* Read 'MCMp' (music character mapping) info. */
	for (i = 0; i < numMusFonts; i++) {
		resH = Get1NamedResource('MCMp', musFontInfo[i].fontName);
		if (!GoodResource(resH)) goto error;
		b = (unsigned char *)(*resH);
		for (ch = 0; ch<256; ch++)
			musFontInfo[i].cMap[ch] = *b++;
		ReleaseResource(resH);
	}

	/* Read 'MCOf' (music character x,y offset) info. */
	for (i = 0; i < numMusFonts; i++) {
		resH = Get1NamedResource('MCOf', musFontInfo[i].fontName);
		if (!GoodResource(resH)) goto error;
		w = (INT16 *)(*resH);
		for (ch = 0; ch<256; ch++) {
			musFontInfo[i].xd[ch] = *w++;
			musFontInfo[i].yd[ch] = *w++;
		}
		ReleaseResource(resH);
	}

	/* Read 'MFEx' (music font extra) info. */
	for (i = 0; i < numMusFonts; i++) {
		resH = Get1NamedResource('MFEx', musFontInfo[i].fontName);
		if (!GoodResource(resH)) goto error;
		w = (INT16 *)(*resH);
		musFontInfo[i].sonataFlagMethod = (*w++ != 0);
		musFontInfo[i].has16thFlagChars = (*w++ != 0);
		musFontInfo[i].hasCurlyBraceChars = (*w++ != 0);
		musFontInfo[i].hasRepeatDotsChar = (*w++ != 0);
		musFontInfo[i].upstemFlagsHaveXOffset = (*w++ != 0);
		musFontInfo[i].hasStemSpaceChar = (*w++ != 0);
		musFontInfo[i].stemSpaceWidth = *w++;
		musFontInfo[i].upstemExtFlagLeading = *w++;
		musFontInfo[i].downstemExtFlagLeading = *w++;
		musFontInfo[i].upstem8thFlagLeading = *w++;
		musFontInfo[i].downstem8thFlagLeading = *w++;
		PStrCopy((StringPtr)w, musFontInfo[i].postscriptFontName);
		ReleaseResource(resH);
	}

	UseResFile(curResFile);
	return TRUE;
error:
	GetIndCString(strBuf, INITERRS_STRS, 28);		/* "Trouble reading alternative music font information." */
	CParamText(strBuf, "", "", "");
	NoteInform(GENERIC_ALRT);
	UseResFile(curResFile);
	return FALSE;
}


/* Initialize duration, rastral size, dynam to MIDI velocity, etc. tables. If
there's a serious problem, return FALSE, else TRUE. */

static Boolean InitTables()
{
	MIDIPreferences **midiStuff;
	MIDIModNRPreferences	**midiModNRH;
	INT16 i;

	l2p_durs[MAX_L_DUR] = PDURUNIT;						/* Set up lookup table to convert; assign 15 to 128th for tuplets */
	for (i = MAX_L_DUR-1; i>0; i--)						/*   logical to physical durations */
		l2p_durs[i] = 2*l2p_durs[i+1];
		
	pdrSize[0] = config.rastral0size;
	for (i = 0; i<=MAXRASTRAL; i++)						/* Set up DDIST table of rastral sizes */
		drSize[i] = pt2d(pdrSize[i]);
		
	if (LAST_DYNAM!=24) {
		MayErrMsg("InitTables: dynam2velo table setup problem.");
		return FALSE;
	}
	midiStuff = (MIDIPreferences **)GetResource('MIDI',PREFS_MIDI);
	if (midiStuff==NULL || *midiStuff==NULL) {
		GetIndCString(strBuf, INITERRS_STRS, 8);		/* "Can't find MIDI resource" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	for (i = 1; i<LAST_DYNAM; i++)						/* Set up dynamic-mark-to-MIDI-velocity table */
		dynam2velo[i] = (*midiStuff)->velocities[i-1];

	midiModNRH = (MIDIModNRPreferences **)GetResource('MIDM',PREFS_MIDI_MODNR);
	if (midiModNRH==NULL || *midiModNRH==NULL) {
		GetIndCString(strBuf, INITERRS_STRS, 31);		/* "Can't find MIDM resource" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	/* Set up MIDI modifier velocity offset and duration factor tables */
	for (i = 0; i<32; i++) {
		modNRVelOffsets[i] = (*midiModNRH)->velocityOffsets[i];
		modNRDurFactors[i] = (*midiModNRH)->durationFactors[i];
	}

	if (!InitMusFontTables())
		return FALSE;
	if (!InitStringPools(256L, 0)) {
		NoMoreMemory();
		return FALSE;
	}
	
	contextA = (CONTEXT *)NewPtr((MAXSTAVES+1)*(Size)sizeof(CONTEXT));
	if (!GoodNewPtr((Ptr)contextA)) {
		OutOfMemory((long)(MAXSTAVES+1)*(Size)sizeof(CONTEXT));
		return FALSE;
	}
	
	return TRUE;	
}


/* If this copy's expiration date is past, say so and return FALSE. */

static Boolean ExpireTimeOK()
{
	DateTimeRec expire;				/* Expiration date of this copy of program */
	unsigned long secsNow, secsExpire;

	//CER 10/23/2003, 5.06a2
	if (config.noExpire==33) return TRUE;				/* For Emergency Use Only */
	/*
	 * For a bit more security, bury these dates in the code, and avoid having
	 * the year appear explicitly as a constant. But caveat: this makes setting
	 * expire.year pretty tricky!
	 */
#if defined(VIEWER_VERSION)
	expire.year = 1899;
	expire.month = 7;
#elif defined(PUBLIC_VERSION)
	#define NoEVAL_SUBVERSION		/* for 3.0A academic evaluation version */
	#ifdef EVAL_SUBVERSION
		expire.year = 1895;
		expire.month = 10;
	#else
		return TRUE;
	#endif
#else										/* Not VIEWER_, not PUBLIC_VERSION = beta */
	expire.year = 1911;
	expire.month = 12;
#endif
	expire.day = 01;
	expire.year += 101;				/* This is what makes setting expire.year tricky! */
	expire.hour = expire.minute = expire.second = 0;
	GetDateTime(&secsNow);
	DateToSeconds(&expire, &secsExpire);

	if (secsNow>=secsExpire) {
		GetIndCString(strBuf, INITERRS_STRS, 12);	/* "...this beta copy of Nightingale..."/"This copy of NoteView..." */
		CParamText(strBuf, "", "", "");
#ifdef VIEWER_VERSION
		StopInform(BIG_GENERIC_ALRT);
		return TRUE;
#else
		StopInform(GENERIC_ALRT);
		return FALSE;
#endif
	}
	
	return TRUE;
}


/* Print various information for debugging. */

static void PrintInfo()
{
#ifndef PUBLIC_VERSION
#ifdef IDEBUG

	DebugPrintf("Size of PARTINFO=%ld\n", sizeof(PARTINFO));
	
	DebugPrintf("Size of HEADER=%ld TAIL=%ld SYNC=%ld RPTEND=%ld PAGE=%ld\n",
		sizeof(HEADER), sizeof(TAIL), sizeof(SYNC), sizeof(RPTEND), sizeof(PAGE));
	DebugPrintf("Size of SYSTEM=%ld STAFF=%ld MEASURE=%ld CLEF=%ld KEYSIG=%ld\n",
		sizeof(SYSTEM), sizeof(STAFF), sizeof(MEASURE), sizeof(CLEF), sizeof(KEYSIG));
	DebugPrintf("Size of TIMESIG=%ld BEAMSET=%ld CONNECT=%ld DYNAMIC=%ld\n",
		sizeof(TIMESIG), sizeof(BEAMSET), sizeof(CONNECT), sizeof(DYNAMIC));
	DebugPrintf("Size of GRAPHIC=%ld OCTAVA=%ld SLUR=%ld TUPLET=%ld GRSYNC=%ld\n",
		sizeof(GRAPHIC), sizeof(OCTAVA), sizeof(SLUR), sizeof(TUPLET), sizeof(GRSYNC));
	DebugPrintf("Size of TEMPO=%ld SPACE=%ld ENDING=%ld PSMEAS=%ld •SUPEROBJ=%ld\n",
		sizeof(TEMPO), sizeof(SPACE), sizeof(ENDING), sizeof(PSMEAS), sizeof(SUPEROBJ));
#endif
#endif
}


/* GetFontNumber returns in the <fontNum> parameter the number for the font with
the given font name. If there's no such font, it returns TRUE. From Inside Mac VI,
12-16. */

Boolean GetFontNumber(const Str255, short *);
Boolean GetFontNumber(const Str255 fontName, short *pFontNum)
{
	Str255 systemFontName;
	
	GetFNum(fontName, pFontNum);
	if (*pFontNum==0) {
		/* Either the font was not found, or it is the system font. */
		GetFontName(0, systemFontName);
		return EqualString(fontName, systemFontName, FALSE, FALSE);
	}
	else
		return TRUE;
}


/* Check to see if all the desirable screen fonts are actually present. */

static void CheckScrFonts()
{
	INT16		origLen, foundSizes=0;
	short		fontNum;

	if (!GetFontNumber("\pSonata", &fontNum)) {
		if (CautionAdvise(MUSFONT_ALRT)==Cancel)
			ExitToShell();
		else
			return;
	}

	/* ??Under System 7, it seems that only the first call to RealFont is meaningful:
		following calls for any size always return TRUE! So it's not obvious how to find
		out what sizes are really present without looking at the FONTs or NFNTs. */
		
	GetIndCString(strBuf, INITERRS_STRS, 16);			/* "Screen versions of the Sonata music font not available in size(s):" */
	origLen = strlen(strBuf);
	foundSizes = 0;
	if (!RealFont(sonataFontNum, 7)) sprintf(&strBuf[strlen(strBuf)], " %d", 7);
	else									  foundSizes++;
	if (!RealFont(sonataFontNum, 12)) sprintf(&strBuf[strlen(strBuf)], " %d", 12);
	else									  foundSizes++;
	if (!RealFont(sonataFontNum, 14)) sprintf(&strBuf[strlen(strBuf)], " %d", 14);
	else									  foundSizes++;
	if (!RealFont(sonataFontNum, 18)) sprintf(&strBuf[strlen(strBuf)], " %d", 18);
	else									  foundSizes++;
	if (!RealFont(sonataFontNum, 24)) sprintf(&strBuf[strlen(strBuf)], " %d", 24);
	else									  foundSizes++;
	if (!RealFont(sonataFontNum, 28)) sprintf(&strBuf[strlen(strBuf)], " %d", 28);
	else									  foundSizes++;
	if (!RealFont(sonataFontNum, 36)) sprintf(&strBuf[strlen(strBuf)], " %d", 36);
	else									  foundSizes++;

	if (foundSizes==0) {
		GetIndCString(strBuf, INITERRS_STRS, 13);		/* "None of the screen sizes Nightingale uses of the Sonata font is available..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	else if (strlen(strBuf)>origLen) {
		GetIndCString(&strBuf[strlen(strBuf)], INITERRS_STRS, 17);		/* ". In some staff sizes, this will make music on the screen hard to read." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
}


/* Handle all initialization tasks relating to the music font: create an offscreen
bitmap for drawing music characters in style other than black, and initialize
screen fonts. */

void InitMusicFontStuff()
{
	INT16 maxPtSize, sonataFontNum;
	
	/*
	 *	We need a grafPort large enough for the largest Sonata character in the
	 *	largest possible size: rastral 0 at 400 percent. Currently, with no magnification,
	 * the best known lower bound is 60 pixels for the standard size of rastral 0, i.e.,
	 *	28 points; however, we can't just use 60 pixels and magnify it because the actual
	 *	point size of rastral 0 is variable--it might even be smaller than rastral 1. So,
	 *	we find the largest possible point size (either rastral 0 or 1) and scale it by
	 *	60/28, then magnify.
	 */
	 
	maxPtSize = n_max(config.rastral0size, pdrSize[1]);
	maxMCharWid = (60.0/28.0)*maxPtSize;
	maxMCharWid = UseMagnifiedSize(maxMCharWid, MAX_MAGNIFY);
	maxMCharHt = maxMCharWid;
#ifdef USE_GWORLDS
	/* We lock the pixmap inside MakeGWorld and leave it that way. */
	fontPort = (GrafPtr)MakeGWorld(maxMCharWid, maxMCharHt, TRUE);
#else
	fontPort = NewGrafPort(maxMCharWid, maxMCharHt);
#endif
	GetFNum("\pSonata", &sonataFontNum);				/* Get ID of standard music font */
	CheckScrFonts();
}


/* ---------------------------------------------------------- MIDI initialization -- */

#ifdef NOMORE
static INT16 GetMIDIChoiceFromUser(long verNumMM, long verNumOMS);
#else
static INT16 GetMIDIChoiceFromUser(long verNumMM, long verNumOMS, long verNumFMS);
#endif
static INT16 ChooseMIDISystem(void);
static Boolean InitChosenMIDISystem(void);
//static Boolean AllocMPacketBuffer(void);
static Boolean GetBIMIDISetup(void);

/* Allocate a buffer for MIDI Manager-compatible MIDI Packets, pointed to by
<mPacketBuffer>. If we have trouble doing so, return FALSE, else TRUE. */

/*static*/ Boolean AllocMPacketBuffer()
{
	Size bytesFree, grow;

	/* A single note takes two MIDI Packets padded to even length = 20 bytes in the buffer
	 * (for Note On and Note Off), while Nightingale's object list data structure as of
	 * v.2.1 takes 30 bytes per note plus a great deal of overhead for the Sync, etc. See
	 * the comment on AllocRawBuffer.
	 */
	bytesFree = MaxMem(&grow);
	mPacketBufferLen = bytesFree/4L;							/* Take a fraction of available memory */
	mPacketBufferLen = n_min(mPacketBufferLen, 100000L);	/* Limit its size to make NewPtr faster */

	if (mPacketBufferLen<5000L) return FALSE;				/* Impose an arbitrary minimum */
	
	mPacketBuffer = (unsigned char *)(NewPtr(mPacketBufferLen));
	return (GoodNewPtr((Ptr)mPacketBuffer));
}


static Boolean GetBIMIDISetup(void)
{
	short curResFile; Handle hBIMIRes;
	
	curResFile = CurResFile();
	UseResFile(setupFileRefNum);
	hBIMIRes = Get1Resource('BIMI',THE_BIMI);
	UseResFile(curResFile);
	if (!GoodResource(hBIMIRes)) {
		GetIndCString(strBuf, INITERRS_STRS, 20);				/* "Can't find the BIMI resource" */
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
		return FALSE;
	}
	
	portSettingBIMIDI = ((BIMIDI *)*hBIMIRes)->portSetting;
	interfaceSpeedBIMIDI = ((BIMIDI *)*hBIMIRes)->interfaceSpeed;
	return TRUE;
}


#ifdef NOMORE

/* NOTE: Since the MIDI Pascal driver is not compatible with CodeWarrior, the ALRTs
invoked by this function have been changed. Now, instead of offering to use Built-in
MIDI, they offer to disable MIDI entirely. The old ALRTs are still in place for
reference.  -JGG, 20-June-00 */

static INT16 GetMIDIChoiceFromUser(long verNumMM, long verNumOMS)
{
	char verStrMM[256];	/* MIDI Manager version (standard version no. string) */
	char verStrOMS[64];	/* OMS version (standard version no. string) */
	short response;
	INT16 useWhichMIDI;
	
	if (verNumMM!=0) {
		StdVerNumToStr(verNumMM, verStrMM);
	}
	if (verNumOMS!=0) {
		StdVerNumToStr(verNumOMS, verStrOMS);
	}

	if (verNumMM!=0 && verNumOMS!=0) {
		CParamText(verStrOMS, verStrMM, "", "");
#ifdef ENABLE_BIMIDI
		PlaceAlert(USEOMS_MM_ALRT, NULL, 0, 80);
		response = CautionAdvise(USEOMS_MM_ALRT);
#else
		PlaceAlert(USEOMS_MM_NOBI_ALRT, NULL, 0, 80);
		response = CautionAdvise(USEOMS_MM_NOBI_ALRT);
#endif
		if (response == 1) {						/* OMS Item */
			useWhichMIDI = MIDIDR_OMS;
		}
		else if (response == 2) {				/* MIDI Manager Item */
			useWhichMIDI = MIDIDR_MM;
		}
#ifdef ENABLE_BIMIDI
		else {										/* Built In Item */
			useWhichMIDI = MIDIDR_BI;
		}
#else
		else {										/* Disable MIDI Item */
			useWhichMIDI = MIDIDR_NONE;
		}
#endif
	}
	else if (verNumOMS!=0) {
		CParamText(verStrOMS, "", "", "");
#ifdef ENABLE_BIMIDI
		PlaceAlert(USEOMS_ALRT, NULL, 0, 80);
		useWhichMIDI = (CautionAdvise(USEOMS_ALRT)==OK? MIDIDR_OMS : MIDIDR_BI);
#else
		PlaceAlert(USEOMS_NOBI_ALRT, NULL, 0, 80);
		useWhichMIDI = (CautionAdvise(USEOMS_NOBI_ALRT)==OK? MIDIDR_OMS : MIDIDR_NONE);
#endif
	}
	else if (verNumMM!=0) {
		CParamText(verStrMM, "", "", "");
#ifdef ENABLE_BIMIDI
		PlaceAlert(USEMM_ALRT, NULL, 0, 80);
		useWhichMIDI = (CautionAdvise(USEMM_ALRT)==OK? MIDIDR_MM : MIDIDR_BI);
#else
		PlaceAlert(USEMM_NOBI_ALRT, NULL, 0, 80);
		useWhichMIDI = (CautionAdvise(USEMM_NOBI_ALRT)==OK? MIDIDR_MM : MIDIDR_NONE);
#endif
	}
	else {
#ifdef ENABLE_BIMIDI
		GetIndCString(strBuf, INITERRS_STRS, 21);		/* "OMS and MIDI Manager are not available, so built-in MIDI will be used" */
#else
		GetIndCString(strBuf, INITERRS_STRS, 27);		/* "OMS and MIDI Manager are not available, so MIDI will be disabled" */
#endif
		CParamText(strBuf, "", "", "");
		PlaceAlert(GENERIC_ALRT, NULL, 0, 80);
		StopInform(GENERIC_ALRT);
#ifdef ENABLE_BIMIDI
		useWhichMIDI = MIDIDR_BI;
#else
		useWhichMIDI = MIDIDR_NONE;
#endif
	}
	
	return useWhichMIDI;
}

#else /* !NOMORE */

/* -------------------------------------------------------- GetMIDIChoiceFromUser -- */
static enum {
	FMS_RAD_DI = 2,
	OMS_RAD_DI,
	MM_RAD_DI,
	NONE_RAD_DI
} E_MIDI_CHOICE;

#ifdef TARGET_API_MAC_CARBON_MIDI

static INT16 GetMIDIChoiceFromUser(long /*verNumMM*/, long /*verNumOMS*/, long /*verNumFMS*/)
{
	useWhichMIDI = MIDIDR_CM;
	return useWhichMIDI;
}

#else

static INT16 GetMIDIChoiceFromUser(long verNumMM, long verNumOMS, long verNumFMS)
{
	INT16 useWhichMIDI;
	
	useWhichMIDI = MIDIDR_NONE;

	if (verNumMM || verNumOMS || verNumFMS>=FreeMIDICurrentVersion) {
		INT16				ditem, radio;
		GrafPtr			oldPort;
		DialogPtr		dlog;
		ModalFilterUPP filterUPP;
	
		filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP==NULL) {
			MissingDialog(USEMIDIDR_DLOG);
			return MIDIDR_NONE;
		}
		dlog = GetNewDialog(USEMIDIDR_DLOG, NULL, BRING_TO_FRONT);
		if (dlog==NULL) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(USEMIDIDR_DLOG);
			return MIDIDR_NONE;
		}

		/* Enable radio buttons if their drivers are available. In the case of FreeMIDI,
			we require better than a certain version (FreeMIDICurrentVersion). Select
			a default radio button, preferably FreeMIDI or one of the other drivers. */
		
		radio = NONE_RAD_DI;
		if (verNumMM!=0)
			radio = MM_RAD_DI;
		else
			XableControl(dlog, MM_RAD_DI, FALSE);
		if (verNumOMS!=0)
			radio = OMS_RAD_DI;
		else
			XableControl(dlog, OMS_RAD_DI, FALSE);
		if (verNumFMS>=FreeMIDICurrentVersion)		/* Must be >= v.1.3.7 (as of Ngale 4.1b9) */
			radio = FMS_RAD_DI;
		else
			XableControl(dlog, FMS_RAD_DI, FALSE);

		PutDlgChkRadio(dlog, radio, TRUE);
		
		CenterWindow(GetDialogWindow(dlog), 100);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
		
		while (1) {
			ModalDialog(filterUPP, &ditem);	
			if (ditem==OK)
				break;
			switch (ditem) {
				case FMS_RAD_DI:
				case OMS_RAD_DI:
				case MM_RAD_DI:
				case NONE_RAD_DI:
					if (ditem!=radio)
						SwitchRadio(dlog, &radio, ditem);
					break;
				default:
					break;
			}
		}
		switch (radio) {
			case FMS_RAD_DI:
				useWhichMIDI = MIDIDR_FMS;
				break;
			case OMS_RAD_DI:
				useWhichMIDI = MIDIDR_OMS;
				break;
			case MM_RAD_DI:
				useWhichMIDI = MIDIDR_MM;
				break;
			default:
				break;
		}
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
		SetPort(oldPort);
	}
	
	return useWhichMIDI;
}

#endif // TARGET_API_MAC_CARBON_MIDI

#endif /* !NOMORE */

#ifdef TARGET_API_MAC_CARBON_MIDI

static INT16 ChooseMIDISystem()
{
	GetMIDIChoiceFromUser(0,0,0);
	
	return useWhichMIDI;
}

static Boolean InitChosenMIDISystem()
{
	Boolean midiOk = TRUE;
	
	if (useWhichMIDI == MIDIDR_CM)
		midiOk = InitCoreMIDI();
	
	return midiOk;
}

Boolean InitMIDISystem()
{
	ChooseMIDISystem();

	return InitChosenMIDISystem();
}

#else

/* Based on config.whichMIDI, whether the MIDI driver it specifies is available on
this system, and perhaps what the user says, set global <useWhichMIDI>. */

static INT16 ChooseMIDISystem()
{
	INT16 useWhichMIDI;
	/*
	 * MIDIVersion() and OMSVersion() both in fact return <NumVersion>, but the headers
	 * are a mess; see comments in NMIDIVersion.
	 */
	long verNumMM, verNumOMS, verNumFMS;

	if (config.whichMIDI==MIDISYS_NONE)
		return MIDIDR_NONE;

	verNumMM = NMIDIVersion();
	verNumOMS = OMSVersion();
	verNumFMS = FreeMIDILongVersion();

	useWhichMIDI = config.whichMIDI;
#ifndef ENABLE_BIMIDI
	if (config.whichMIDI==MIDISYS_BI)
		useWhichMIDI = MIDISYS_ASK;
#endif

	/*
	 * Ask user what driver to use if user wanted to be asked, or if the appropriate
	 * modifier key is down. Let them choose only from drivers that are available! 
	 */
	if (useWhichMIDI==MIDISYS_ASK || (ShiftKeyDown())) {
		return GetMIDIChoiceFromUser(verNumMM, verNumOMS, verNumFMS);	
	}

	/*
	 * Ask user what driver to use if the driver they wanted isn't available. Let
	 * them choose only from drivers that are available!
	 */
	if (useWhichMIDI==MIDISYS_FMS && verNumFMS<FreeMIDICurrentVersion) {
		GetIndCString(strBuf, INITERRS_STRS, 32);		/* "The Prefs file specifies FreeMIDI, but..." */
		CParamText(strBuf, "", "", "");
		PlaceAlert(GENERIC_ALRT, NULL, 0, 80);
		StopInform(GENERIC_ALRT);
		return GetMIDIChoiceFromUser(verNumMM, verNumOMS, verNumFMS);	
	}

	if (useWhichMIDI==MIDISYS_OMS && !verNumOMS) {
		GetIndCString(strBuf, INITERRS_STRS, 24);		/* "The Prefs file specifies OMS, but..." */
		CParamText(strBuf, "", "", "");
		PlaceAlert(GENERIC_ALRT, NULL, 0, 80);
		StopInform(GENERIC_ALRT);
		return GetMIDIChoiceFromUser(verNumMM, verNumOMS, verNumFMS);	
	}

	if (useWhichMIDI==MIDISYS_MM && !verNumMM) {
		GetIndCString(strBuf, INITERRS_STRS, 25);		/* "The Prefs file specifies MIDI Manager, but..." */
		CParamText(strBuf, "", "", "");
		PlaceAlert(GENERIC_ALRT, NULL, 0, 80);
		StopInform(GENERIC_ALRT);
		return GetMIDIChoiceFromUser(verNumMM, verNumOMS, verNumFMS);	
	}

	return useWhichMIDI;
}

#include "MIDIPASCAL3.h"

static Boolean InitChosenMIDISystem()
{
	long verNum;
	INT16 progStrNum;
	
	/*
	 *	If we're going to use either MIDI Manager or our built-in MIDI driver, we'll need
	 *	mPacketBuffer. OMSInit takes care of buffers for OMS.
	 */
	if (useWhichMIDI==MIDIDR_MM || useWhichMIDI==MIDIDR_BI) {	
		if (!AllocMPacketBuffer()) {
			GetIndCString(strBuf, INITERRS_STRS, 9);		/* "Can't allocate buffer for MIDI" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
	}

	UseResFile(appRFRefNum);
	switch (useWhichMIDI) {
		case MIDIDR_FMS:
			progStrNum = MIDIDR_FMS_PMSTR;
			break;
		case MIDIDR_OMS:
			progStrNum = MIDIDR_OMS_PMSTR;
			break;
		case MIDIDR_MM:
			progStrNum = MIDIDR_MM_PMSTR;
			break;
		case MIDIDR_BI:
			progStrNum = MIDIDR_BI_PMSTR;
			break;
		default:
			return FALSE;
	}

	if (!config.fastLaunch)
		ProgressMsg(progStrNum, "");

	/*
	 * Both MIDI Manager and our built-in MIDI stuff modify the interrupt vector, so
	 * if Nightingale terminates abnormally while they're in use, the results are
	 * likely to be disasterous, i.e., the user will have to reboot. Therefore, to
	 * reduce the risk of this, it'd be best to initialize MIDI just before using it
	 * and quit it just after each use. Unfortunately, this isn't practical with
	 * MIDI Manager because (1) logging in and out takes quite a bit of time, (2) the
	 * user wouldn't have a chance to do manual patching in PatchBay, (3) the data
	 * structures it allocates would fragment memory. So if we're using MIDI Manager,
	 *	we just initialize it now and quit it at the end of execution.
	 */
	if (useWhichMIDI==MIDIDR_MM) {
		if (!MMInit(&verNum)) {
			if (verNum<=0L) {
				MayErrMsg("MIDI Manager is not installed.");		/* Should never happen */
				return FALSE;
			}
			else {
				GetIndCString(strBuf, INITERRS_STRS, 10);		/* "Trouble signing into MIDI Manager" */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
#ifdef NOTYET
				/*
				 * Before giving up, try signing out, then back in. ??Unfortunately, this
				 * code hangs. Maybe there's a simple fix...maybe just wait a few sec.
				 *	before MMINIT? Try it someday.
				 */
				MIDISignOut(creatorType);
				if (!MMInit(&verNum)) return FALSE;
#else
				return FALSE;
#endif
			}
		}
	}
	else if (useWhichMIDI==MIDIDR_OMS) {
		if (thisMac.systemVersion<0x0700) {					/* ??Really should check Time Mgr version */
			GetIndCString(strBuf, INITERRS_STRS, 23);		/* "To use OMS, Nightingale requires System version 7 or higher." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}

		if (!OMSInit()) {
			GetIndCString(strBuf, INITERRS_STRS, 22);		/* "Trouble signing into OMS" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
	}
	else if (useWhichMIDI==MIDIDR_FMS) {
		if (!FMSInit()) {
			GetIndCString(strBuf, INITERRS_STRS, 33);		/* "Trouble signing into FreeMIDI" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
	}
	else {															/* Assume built-in */
		initedBIMIDI = FALSE;
		(void)GetBIMIDISetup();
#define MIDI_THRU
#ifdef MIDI_THRU

		/*
		 *	MidiThru can't do anything till you call MayInitBIMIDI, so we have to do both
		 *	here. Unfortunately, MayInitBIMIDI modifiers the interrupt vector, and that 
		 *	means we have to be sure to call MayResetBIMIDI before quitting to avoid a
		 *	probable system crash after we've quit.
		 */
		if (config.midiThru!=0) {
			MayInitBIMIDI(BIMIDI_SMALLBUFSIZE, BIMIDI_SMALLBUFSIZE);	/* initialize serial chip */
			MidiThru(1);
		}
#endif
	}
	
	if (!config.fastLaunch) {
		SleepTicksWaitButton(75L);										/* So user has time to read the message */
		if (useWhichMIDI==MIDIDR_BI && portSettingBIMIDI==PRINTER_PORT)
				CautionInform(BIMIDI_PRINTERPORT_ALRT);	/* "WARNING: If you use the modem port for communications while..." */
		ProgressMsg(0, "");
	}
	
#ifndef PUBLIC_VERSION
	DebugPrintf("Initialized MIDI system %d, midiThru=%d\n", useWhichMIDI,
						config.midiThru);
#endif
	return TRUE;
}

/* Based on config.whichMIDI, whether the MIDI driver it specifies is available on
this system, and perhaps what the user says, initialize a MIDI driver or no MIDI and
set global <useWhichMIDI> accordingly. Return TRUE if success, FALSE if a problem
occurs.

As of v.2.1, built-in MIDI is MIDI Pascal 3.0, which works well (but see comment on
CodeWarrior below). Formerly, we used the MacTutor driver, which was very unreliable,
so we discouraged users from using it by the simple mechanism of not telling them how
to use it.

As of v.3.0, we support built-in MIDI, Apple MIDI Manager, and (using our own, Apple
Time Manager-based, routines for timing) OMS. Of these, only built-in MIDI is always
available--except that it's a THINK C Library, so it doesn't work with versions
built with CodeWarrior. ??NEED TO HANDLE THIS. */

Boolean InitMIDISystem()
{
	useWhichMIDI = ChooseMIDISystem();
	
	/* If we're not going to use any MIDI driver, we're done. */
	
	if (useWhichMIDI==MIDIDR_NONE && !config.fastLaunch) {
		ProgressMsg(MIDIDR_NONE_PMSTR, "");
		SleepTicksWaitButton(90L);								/* So user has time to read the message */
		ProgressMsg(0, "");
		return TRUE;
	}

	return InitChosenMIDISystem();
}

#endif // TARGET_API_MAC_CARBON_MIDI

