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

#include <CoreMIDI/MidiServices.h>
// MAS
#include "CarbonTemplates.h"

void			InitNightingale(void);
static Boolean	DoSplashScreen(void);
static Boolean	InitAllCursors(void);
static void		InitNightFonts(void);
static Boolean	InitNightGlobals(void);
static Boolean	InitTables(void);
static void 	PrintInfo(void);
static void 	CheckScreenFonts(void);
static void		InitMusicFontStuff(void);
static Boolean	InitMIDISystem(void);

void			NExitToShell(char *msg);

/* ------------------------------------------------------------- InitNightingale -- */
/* One-time initialization of Nightingale-specific stuff. Assumes the Macintosh
toolbox has already been initialized and the config struct filled in. */

void InitNightingale()
{
	InitNightFonts();									/* Do this first to be ready for dialogs */

	if (!config.fastLaunch)
		if (!DoSplashScreen()) NExitToShell("Splash Screen");
	if (!InitAllCursors()) NExitToShell("Init All Cursors");
	if (!InitNightGlobals()) NExitToShell("Init Night Globals");
	MEInitCaretSystem();
	
	if (!InitTables()) NExitToShell("Init Tables");
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

In commercial versions of Nightingale, the splash screen we displayed shows the owner's
name and organization, to discourage illegal copying. We also checked (but didn't display)
the copy serial number: if it's wrong, return FALSE. */

#define NAME_DI 1
#define ABOUT_DI 2

static Boolean DoSplashScreen()
{
	DialogPtr dlog; GrafPtr oldPort;
	short aShort; Handle aHdl; Rect aRect;

	GetPort(&oldPort);
	dlog = GetNewDialog(OWNER_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) return FALSE;
	SetPort(GetDialogWindowPort(dlog));

	GetDialogItem(dlog, NAME_DI, &aShort, &aHdl, &aRect);

	/* If the owner's name and organization aren't filled in, it's no big deal:
		they should have the default values. */

	CenterWindow(GetDialogWindow(dlog), 65);
	ShowWindow(GetDialogWindow(dlog));
	UpdateDialogVisRgn(dlog);

	SleepTicksWaitButton(120L);							/* So user has time to read it */

	HideWindow(GetDialogWindow(dlog));
	DisposeDialog(dlog);										/* Free heap space */
	SetPort(oldPort);

	return TRUE;
}


/* Initialize the cursor globals and the cursor list. */

static Boolean InitAllCursors()
{
	short i;

	WaitCursor();

	handCursor = GetCursor(RHAND_CURS); if (!handCursor) goto Error;
	threadCursor = GetCursor(THREAD_CURS); if (!threadCursor) goto Error;
	genlDragCursor = GetCursor(GENLDRAG_CURS); if (!genlDragCursor) goto Error;
	
	if (nsyms>MAX_CURSORS) {
		MayErrMsg("InitAllCursors: Need %ld cursors but only allocated %ld",
					(long)nsyms, (long)MAX_CURSORS);
		return FALSE;
	}
	
	for (i = 0;i<nsyms; i++)							/* Fill in list of cursor handles */
		if (symtable[i].cursorID>=100)
			cursorList[i] = GetCursor(symtable[i].cursorID);
		else
			cursorList[i] = (CursHandle)0;
	arrowCursor = currentCursor = GetCursor(ARROW_CURS);
	return TRUE;
	
Error:
	GetIndCString(strBuf, INITERRS_STRS, 7);			/* "Can't find cursor resources" */
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
	return FALSE;
}


/* Set globals describing our standard text font and our standard music font. */

void InitNightFonts()
{
	textFontNum = applFont;
	
	/*
	 * NB: The following comment is pre-OS X.
	 * The "magic no." in next line should theoretically go away: we should get the
	 * system font size from the Script Manager if it's present (and it should always be).
	 * However, in every system I've seen, the system font size is 12, so I doubt it's
	 * a big deal.
	 */
	textFontSize = 12;
	textFontSmallSize = textFontSize-3;

	GetFNum("\pSonata", &sonataFontNum);				/* Get ID of Adobe Sonata font */
}


/* Allocate <endingString> and get label strings for Endings into it. Return the
number of strings found, or -1 if there's an error (probably out of memory). */

static short InitEndingStrings(void);
static short InitEndingStrings()
{
	char str[256]; short n, strOffset;
	
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
	char	fmtStr[256];
	short	j;
	
	/*
	 *	Since initialization routines can (and as of this writing do) use the coordinate-
	 * conversion macros, set up variables used by those macros as if no magnification
	 * is in effect.
	 */	
	magShift = 4;
	magRound = 8;
		
	outputTo = toScreen;								/* Initial output device */
	clickMode = ClickSelect;
	lastCopy = COPYTYPE_CONTENT;

	doNoteheadGraphs = FALSE;							/* Normal noteheads, not tiny graphs */
	playTempoPercent = 100;								/* Play using the tempi as marked */
	unisonsOK = TRUE;									/* Don't complain about perfect unisons */

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
	short	i, nRes, resID, curResFile;
	short	*w, ch, count, xw, xl, yt, xr, yb, index;
	unsigned char *b;
	Handle	resH;
	Size	nBytes;
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

		w = (short *)(*resH);
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
		the resource map being the same as with the 'BBX#' resource. */

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
		w = (short *)(*resH);
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
		w = (short *)(*resH);
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
	MIDIModNRPreferences **midiModNRH;
	short i;

	l2p_durs[MAX_L_DUR] = PDURUNIT;					/* Set up lookup table to convert; assign 15 to 128th for tuplets */
	for (i = MAX_L_DUR-1; i>0; i--)					/*   logical to physical durations */
		l2p_durs[i] = 2*l2p_durs[i+1];
		
	pdrSize[0] = config.rastral0size;
	for (i = 0; i<=MAXRASTRAL; i++)					/* Set up DDIST table of rastral sizes */
		drSize[i] = pt2d(pdrSize[i]);
		
	if (LAST_DYNAM!=24) {
		MayErrMsg("InitTables: dynam2velo table setup problem.");
		return FALSE;
	}
	midiStuff = (MIDIPreferences **)GetResource('MIDI',PREFS_MIDI);
	if (midiStuff==NULL || *midiStuff==NULL) {
		GetIndCString(strBuf, INITERRS_STRS, 8);	/* "Can't find MIDI resource" */
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


/* Print various information for debugging. */

static void PrintInfo()
{
#ifndef PUBLIC_VERSION
#ifdef IDEBUG

	LogPrintf(LOG_NOTICE, "Size of PARTINFO=%ld\n", sizeof(PARTINFO));
	
	LogPrintf(LOG_NOTICE, "Size of HEADER=%ld TAIL=%ld SYNC=%ld RPTEND=%ld PAGE=%ld\n",
		sizeof(HEADER), sizeof(TAIL), sizeof(SYNC), sizeof(RPTEND), sizeof(PAGE));
	LogPrintf(LOG_NOTICE, "Size of SYSTEM=%ld STAFF=%ld MEASURE=%ld CLEF=%ld KEYSIG=%ld\n",
		sizeof(SYSTEM), sizeof(STAFF), sizeof(MEASURE), sizeof(CLEF), sizeof(KEYSIG));
	LogPrintf(LOG_NOTICE, "Size of TIMESIG=%ld BEAMSET=%ld CONNECT=%ld DYNAMIC=%ld\n",
		sizeof(TIMESIG), sizeof(BEAMSET), sizeof(CONNECT), sizeof(DYNAMIC));
	LogPrintf(LOG_NOTICE, "Size of GRAPHIC=%ld OTTAVA=%ld SLUR=%ld TUPLET=%ld GRSYNC=%ld\n",
		sizeof(GRAPHIC), sizeof(OTTAVA), sizeof(SLUR), sizeof(TUPLET), sizeof(GRSYNC));
	LogPrintf(LOG_NOTICE, "Size of TEMPO=%ld SPACER=%ld ENDING=%ld PSMEAS=%ld •SUPEROBJ=%ld\n",
		sizeof(TEMPO), sizeof(SPACER), sizeof(ENDING), sizeof(PSMEAS), sizeof(SUPEROBJ));
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

static void CheckScreenFonts()
{
	short	origLen, foundSizes=0;
	short	fontNum;

	if (!GetFontNumber("\pSonata", &fontNum)) {
		if (CautionAdvise(MUSFONT_ALRT)==Cancel)
			ExitToShell();
		else
			return;
	}

	/* FIXME: The following comment obviously predates OS X. What's the situation now?
		--DAB, Mar. 2016
		Under System 7, it seems that only the first call to RealFont is meaningful:
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

	if (strlen(strBuf)<=origLen) LogPrintf(LOG_NOTICE,
		"CheckScreenFonts: found all %d screen sizes of the Sonata music font.\n", foundSizes);
	else LogPrintf(LOG_WARNING, "CheckScreenFonts: %s\n", strBuf);
}


/* Handle all initialization tasks relating to the music font: create an offscreen
bitmap for drawing music characters in style other than black, and initialize
screen fonts. */

void InitMusicFontStuff()
{
	short maxPtSize, sonataFontNum;
	
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
	CheckScreenFonts();
}


#ifdef TARGET_API_MAC_CARBON_MIDI

static Boolean InitChosenMIDISystem()
{
	Boolean midiOK = TRUE;
	
	if (useWhichMIDI == MIDIDR_CM)
		midiOK = InitCoreMIDI();
	
	LogPrintf(LOG_NOTICE, "useWhichMIDI=%d midiOK=%d\n", useWhichMIDI, midiOK);
	return midiOK;
}

Boolean InitMIDISystem()
{
	/* For now, only support MacOS Core MIDI */
	useWhichMIDI = MIDIDR_CM;

	return InitChosenMIDISystem();
}

#endif // TARGET_API_MAC_CARBON_MIDI


