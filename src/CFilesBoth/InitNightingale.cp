/* InitNightingale.c for Nightingale
One-time initialization of Nightingale-specific stuff. Assumes the Macintosh toolbox
has already been initialized and the config struct filled in. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include <CoreMIDI/MidiServices.h>
// MAS
#include "CarbonTemplates.h"

static void		NExitToShell(char *msg);
static Boolean	DoSplashScreen(void);
static Boolean	InitAllCursors(void);
static Boolean	InitNightGlobals(void);
static Boolean	InitTables(void);
static void 	DisplayInfo(void);
static void		InitNightFonts(void);
static Boolean	InitMusFontTables(void);
static void 	CheckScreenFonts(void);
static void		InitMusicFontStuff(void);
static Boolean	InitMIDISystem(void);
static void		DisplayToolPaletteParams(PaletteGlobals *whichPalette);
static Boolean	CheckToolPaletteParams(PaletteGlobals *whichPalette);
static short	GetToolGrid(PaletteGlobals *whichPalette);
static void		InitPaletteRects(Rect *whichRects, short across, short down, short width,
										short height);
static Boolean	InitToolPalette(PaletteGlobals *whichPalette, Rect *windowRect);

void InitNightingale()
{
	InitNightFonts();							/* Do this first to be ready for dialogs */

	if (!config.fastLaunch)
		if (!DoSplashScreen()) NExitToShell("Splash Screen");
	if (!InitAllCursors()) NExitToShell("Init All Cursors");
	if (!InitNightGlobals()) NExitToShell("Init Night Globals");
	MEInitCaretSystem();
	
	if (!InitTables()) NExitToShell("Init Tables");
	DisplayInfo();
	InitMusicFontStuff();
	if (!InitMIDISystem()) NExitToShell("Init MIDI");
}

static void NExitToShell(char *msg)
{
	CParamText(msg, "", "", "");
	StopInform(GENERIC_ALRT);
	ExitToShell();
}


/* Display splash screen. If all OK, return True; if there's a problem, return False.

In commercial versions of Nightingale, the splash screen we displayed shows the owner's
name and organization, to discourage illegal copying. We also checked (but didn't display)
the copy serial number, and if it was wrong, returned False. */

#define NAME_DI 1
#define ABOUT_DI 2

static Boolean DoSplashScreen()
{
	DialogPtr dlog;  GrafPtr oldPort;
	short aShort;  Handle aHdl;  Rect aRect;

	GetPort(&oldPort);
	dlog = GetNewDialog(OWNER_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) return False;
	SetPort(GetDialogWindowPort(dlog));

	GetDialogItem(dlog, NAME_DI, &aShort, &aHdl, &aRect);

	/* If the owner's name and organization aren't filled in, it's no big deal: they
	   should have the default values. */

	CenterWindow(GetDialogWindow(dlog), 65);
	ShowWindow(GetDialogWindow(dlog));
	UpdateDialogVisRgn(dlog);

	SleepTicksWaitButton(120L);							/* So user has time to read it */

	HideWindow(GetDialogWindow(dlog));
	DisposeDialog(dlog);								/* Free heap space */
	SetPort(oldPort);

	return True;
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
		return False;
	}
	
	for (i = 0;i<nsyms; i++)							/* Fill in list of cursor handles */
		if (symtable[i].cursorID>=100)
			cursorList[i] = GetCursor(symtable[i].cursorID);
		else
			cursorList[i] = (CursHandle)0;
	arrowCursor = currentCursor = GetCursor(ARROW_CURS);
	return True;
	
Error:
	GetIndCString(strBuf, INITERRS_STRS, 7);			/* "Can't find cursor resources" */
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
	return False;
}


/* Allocate <endingString> and get label strings for Endings into it. Return the number
of strings found, or -1 if there's an error (probably out of memory). */

static short InitEndingStrings(void);
static short InitEndingStrings()
{
	char str[256];
	short n, strOffset;
	
	endingString = NewPtr(MAX_ENDING_STRINGS*MAX_ENDING_STRLEN);
	if (!GoodNewPtr(endingString)) return -1;
	
	/* Get strings up to MAX_ENDING_STRINGS from a resource, terminated by an empty
	 * string or the last string present. 1st label is string 0 but index 1. */
	 	
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
	
	/* Since initialization routines can (and as of this writing do) use the coordinate-
	   conversion macros, set up variables used by those macros as if no magnification
	   is in effect. */
	
	magShift = 4;
	magRound = 8;
		
	outputTo = toScreen;								/* Initial output device */
	clickMode = ClickSelect;
	lastCopy = COPYTYPE_CONTENT;

	doNoteheadGraphs = False;							/* Normal noteheads, not tiny graphs */
	playTempoPercent = 100;								/* Play using the tempi as marked */
	unisonsOK = True;									/* Don't complain about perfect unisons */

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
		return False;
	}
	else if (maxEndingNum<5) {
		GetIndCString(fmtStr, INITERRS_STRS, 26);	/* "Found only %d Ending strings: Ending labels may not be shown." */
		sprintf(strBuf, fmtStr, maxEndingNum);
		CParamText(strBuf, "", "", "");
		NoteInform(GENERIC_ALRT);
	}
	
	return True;
}


/* Initialize duration, rastral size, dynam to MIDI velocity, etc. tables. If there's
a serious problem, return False, else True. */

static Boolean InitTables()
{
	MIDIPreferences **midiStuff;
	MIDIModNRPreferences **midiModNRH;
	short i;

	l2p_durs[MAX_L_DUR] = PDURUNIT;					/* Set up lookup table to convert */
	for (i = MAX_L_DUR-1; i>0; i--)					/*   logical to physical durations */
		l2p_durs[i] = 2*l2p_durs[i+1];
		
	pdrSize[0] = config.rastral0size;
	for (i = 0; i<=MAXRASTRAL; i++)					/* Set up DDIST table of rastral sizes */
		drSize[i] = pt2d(pdrSize[i]);
		
	if (LAST_DYNAM!=24) {
		MayErrMsg("InitTables: dynam2velo table setup problem.");
		return False;
	}
	midiStuff = (MIDIPreferences **)GetResource('MIDI', PREFS_MIDI);
	if (midiStuff==NULL || *midiStuff==NULL) {
		GetIndCString(strBuf, INITERRS_STRS, 8);	/* "Can't find MIDI resource" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
	for (i = 1; i<LAST_DYNAM; i++)						/* Set up dynamic-mark-to-MIDI-velocity table */
		dynam2velo[i] = (*midiStuff)->velocities[i-1];

	midiModNRH = (MIDIModNRPreferences **)GetResource('MIDM', PREFS_MIDI_MODNR);
	if (midiModNRH==NULL || *midiModNRH==NULL) {
		GetIndCString(strBuf, INITERRS_STRS, 31);		/* "Can't find MIDM resource" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
	/* Set up MIDI modifier velocity offset and duration factor tables */
	
	for (i = 0; i<32; i++) {
		modNRVelOffsets[i] = (*midiModNRH)->velocityOffsets[i];  FIX_END(modNRVelOffsets[i]);
		modNRDurFactors[i] = (*midiModNRH)->durationFactors[i];  FIX_END(modNRDurFactors[i]);
	}

	if (!InitMusFontTables()) return False;
	
	if (!InitStringPools(256L, 0)) {
		NoMoreMemory();
		return False;
	}
	
	contextA = (CONTEXT *)NewPtr((MAXSTAVES+1)*(Size)sizeof(CONTEXT));
	if (!GoodNewPtr((Ptr)contextA)) {
		OutOfMemory((long)(MAXSTAVES+1)*(Size)sizeof(CONTEXT));
		return False;
	}
	
	return True;	
}


/* Show various information for debugging in the log. */

static void DisplayInfo()
{
#define DEBUG_FONT_PROBLEMS
#ifdef DEBUG_FONT_PROBLEMS
		if (MORE_DETAIL_SHOW) DisplayAvailableFonts();
#endif

#ifdef IDEBUG
	LogPrintf(LOG_DEBUG, "Size of PARTINFO=%ld\n", sizeof(PARTINFO));
	
	LogPrintf(LOG_DEBUG, "Size of HEADER=%ld TAIL=%ld SYNC=%ld RPTEND=%ld PAGE=%ld\n",
		sizeof(HEADER), sizeof(TAIL), sizeof(SYNC), sizeof(RPTEND), sizeof(PAGE));
	LogPrintf(LOG_DEBUG, "Size of SYSTEM=%ld STAFF=%ld MEASURE=%ld CLEF=%ld KEYSIG=%ld\n",
		sizeof(SYSTEM), sizeof(STAFF), sizeof(MEASURE), sizeof(CLEF), sizeof(KEYSIG));
	LogPrintf(LOG_DEBUG, "Size of TIMESIG=%ld BEAMSET=%ld CONNECT=%ld DYNAMIC=%ld\n",
		sizeof(TIMESIG), sizeof(BEAMSET), sizeof(CONNECT), sizeof(DYNAMIC));
	LogPrintf(LOG_DEBUG, "Size of GRAPHIC=%ld OTTAVA=%ld SLUR=%ld TUPLET=%ld GRSYNC=%ld\n",
		sizeof(GRAPHIC), sizeof(OTTAVA), sizeof(SLUR), sizeof(TUPLET), sizeof(GRSYNC));
	LogPrintf(LOG_DEBUG, "Size of TEMPO=%ld SPACER=%ld ENDING=%ld PSMEAS=%ld •SUPEROBJ=%ld\n",
		sizeof(TEMPO), sizeof(SPACER), sizeof(ENDING), sizeof(PSMEAS), sizeof(SUPEROBJ));
#endif
}

/* ----------------------------------------------------------------------------- Fonts -- */

/* Set globals describing our standard text font and our standard music font. */

void InitNightFonts()
{
	textFontNum = applFont;
	
	/* NB: The following comment is pre-OS X and should be taken with a grain of salt!:
	   The "magic no." in next line should theoretically go away: we should get the
	   system font size from the Script Manager if it's present (and it should always
	   be). However, in every system I've seen, the system font size is 12, so I doubt
	   it's a big deal. */
	   
	textFontSize = 12;
	textFontSmallSize = textFontSize-3;

	if (!GetFontNumber("\pSonata", &sonataFontNum)) {	/* Get ID of Adobe Sonata font */
		if (CautionAdvise(MUSFONT_ALRT)==Cancel)
			ExitToShell();
		else
			return;
	}
}


/* Read 'BBX#', 'MCMp', 'MCOf', and 'MFEx' resources for music fonts we can handle and
store their information in newly allocated musFontInfo[]. Return True if OK; False if
error. Assumes that each font has one of all four types of resource mentioned above, and
that the resource ID's for a font's four types match.  For example, "Petrucci" should
have 'BBX#' 129, 'MCMp' 129, 'MCOf' 129, and 'MFEx' 129. -JGG, 4/25/01; rev. by DAB,
2/1/18. */

#define LPD(z)	if (ch==(unsigned short)'&' || ch==(unsigned short)'?') LogPrintf(LOG_DEBUG, "  %d", (z));
#define LPDL(z)	if (ch==(unsigned short)'&' || ch==(unsigned short)'?') LogPrintf(LOG_DEBUG, "  %d\n", (z));

static Boolean InitMusFontTables()
{
	short	i, nRes, resID, curResFile;
	short	*w, index, xl, yt, xr, yb;
	unsigned short ch, chCount, xw;
	unsigned char *b;
	Handle	resH;
	Size	nBytes;
	OSType	resType;
	Str255	resName;
	char	musFontName[32];

	curResFile = CurResFile();
	UseResFile(appRFRefNum);

	nRes = Count1Resources('BBX#');
	numMusFonts = nRes;

	/* Allocate musFontInfo[] large enough to hold all the music fonts we have resources for. */
	
	nBytes = numMusFonts * (Size)sizeof(MusFontRec);
	musFontInfo = (MusFontRec *)NewPtr(nBytes);
	if (!GoodNewPtr((Ptr)musFontInfo)) {
		OutOfMemory((long)nBytes);
		return False;
	}

	/* Read 'BBX#' (character bounding box) info. */
	
	index = 0;
	for (i = 1; i<=nRes; i++) {
		resH = Get1IndResource('BBX#', i); 
		if (!GoodResource(resH)) goto error;
		GetResInfo(resH, &resID, &resType, resName);
		if (ReportResError()) goto error;
		if (resName[0]>31) goto error;				/* font name must have < 32 chars. */
		Pstrcpy(musFontInfo[index].fontName, resName);
		GetFNum(resName, &musFontInfo[index].fontID);

		/* Set all bounding boxes to empty, then fill in actual values for chars. that
		   exist in this font. */
		
		for (ch = 0; ch<256; ch++) {
			musFontInfo[index].cBBox[ch].left = 
			musFontInfo[index].cBBox[ch].top = 
			musFontInfo[index].cBBox[ch].right = 
			musFontInfo[index].cBBox[ch].bottom = 0;
		}

		w = (short *)(*resH);
		chCount = *w++;
		FIX_END(chCount);
		Pstrcpy((StringPtr)musFontName, resName);
		PToCString((StringPtr)musFontName);
		LogPrintf(LOG_NOTICE, "Setting up music font '%s' (font no. %d): %d chars.  (InitMusFontTables)\n",
						musFontName, musFontInfo[index].fontID, chCount);
		while (chCount-- > 0) {
			ch = *w++; FIX_END(ch);
#if 0
			xw = *w++; LPD(xw); FIX_END(xw); LPDL(xw);
			xl = *w++; LPD(xl); FIX_END(xl); LPDL(xl);
			yb = -(*w++); LPD(yb); FIX_END(yb); LPDL(yb);	yb -= 255;
			xr = *w++; LPD(xr); FIX_END(xr); LPDL(xr);
			yt = -(*w++); LPD(yt); FIX_END(yt); LPDL(yt);	yt -=255;
#else
			xw = *w++; FIX_END(xw);
			xl = *w++; FIX_END(xl);
			yb = -(*w++); FIX_END(yb);
			xr = *w++; FIX_END(xr);
			yt = -(*w++); FIX_END(yt);
			
			/* An incredibly weird thing I can only attribute (after much digging) to a
			   bug in the Intel version of the Resource Manager is that values for yb and
			   yt on Intel are consistently too large by 255! So correct for that. (We're
			   only concerned with Intels and PowerPCs, so little Endian is equivalent to
			   Intel.) --DAB */

#if TARGET_RT_LITTLE_ENDIAN
			yb -= 255; yt -= 255;
#endif

			//if (ch==(unsigned short)'&' || ch==(unsigned short)'?') LogPrintf(LOG_DEBUG,
			//		"InitMusFontTables: ch=%c=%u yt=%d yb=%d\n", ch, ch, yt, yb);
#endif
			SetRect(&musFontInfo[index].cBBox[ch], xl, yt, xr, yb);
		}
		ReleaseResource(resH);
		index++;
	}

	/* For the other resources, we use Get1NamedResource instead of Count1Resources
	   because we can't rely on the order of resources in the resource map being the
	   same as with the 'BBX#' resource. */

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
			musFontInfo[i].xd[ch] = *w++; FIX_END(musFontInfo[i].xd[ch]);
			musFontInfo[i].yd[ch] = *w++; FIX_END(musFontInfo[i].yd[ch]);
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
		musFontInfo[i].stemSpaceWidth = *w++; FIX_END(musFontInfo[i].stemSpaceWidth);
		musFontInfo[i].upstemExtFlagLeading = *w++; FIX_END(musFontInfo[i].upstemExtFlagLeading);
		musFontInfo[i].downstemExtFlagLeading = *w++; FIX_END(musFontInfo[i].downstemExtFlagLeading);
		musFontInfo[i].upstem8thFlagLeading = *w++; FIX_END(musFontInfo[i].upstem8thFlagLeading);
		musFontInfo[i].downstem8thFlagLeading = *w++; FIX_END(musFontInfo[i].downstem8thFlagLeading);
		Pstrcpy(musFontInfo[i].postscriptFontName, (StringPtr)w);
		ReleaseResource(resH);
	}

	UseResFile(curResFile);
	return True;
error:
	GetIndCString(strBuf, INITERRS_STRS, 28);		/* "Nightingale can't get its information on music font..." */
	CParamText(strBuf, "", "", "");
	NoteInform(GENERIC_ALRT);
	UseResFile(curResFile);
	return False;
}


/* Check if all the desirable screen fonts are actually present and report what we find. */

static void CheckScreenFonts()
{
	unsigned short origLen, foundSizes=0;

	/* FIXME: The following comment _way_ predates OS X. What's the situation now?
	   --DAB, Mar. 2016
	   "Under System 7, it seems that only the first call to RealFont is meaningful:
	   following calls for any size always return True! So it's not obvious how to find
	   out what sizes are really present without looking at the FONTs or NFNTs." */
		
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
		"Found all %d screen sizes of the Sonata music font.  (CheckScreenFonts)\n", foundSizes);
	else LogPrintf(LOG_WARNING, "%s (CheckScreenFonts)\n", strBuf);
}


/* Handle all initialization tasks relating to the music font: create an offscreen
bitmap for drawing music characters in style other than black; and check for presence
of and initialize screen fonts. */

void InitMusicFontStuff()
{
#ifdef NOMORE
	/* This code hasn't been used since v. 5.7, if not longer; still, it seems worth
	   keeping for possible future use. --DAB, July 2021
	   
	short maxPtSize;
	
	/* We need a grafPort large enough for the largest music character in the largest
	   possible size: rastral 0 or 1 at MAX_MAGNIFY magnification. With no magnification,
	   the best known lower bound is 60 pixels for the standard size of rastral 0, i.e.,
	   28 points; however, the actual point size of rastral 0 is variable: it might even
	   be smaller than rastral 1. So, we find the largest possible point size (either
	   rastral 0 or 1) and scale it by 60/28, then magnify. */
	
	maxPtSize = n_max(config.rastral0size, pdrSize[1]);
	maxMCharWid = (60.0/28.0)*maxPtSize;
	maxMCharWid = UseMagnifiedSize(maxMCharWid, MAX_MAGNIFY);
	maxMCharHt = maxMCharWid;
	
#ifdef USE_GWORLDS
	/* Lock the pixmap inside MakeGWorld and leave it that way. */
	
	fontPort = (GrafPtr)MakeGWorld(maxMCharWid, maxMCharHt, True);
#else
	fontPort = NewGrafPort(maxMCharWid, maxMCharHt);
#endif
#endif

	CheckScreenFonts();
}

/* ------------------------------------------------------------------------------ MIDI -- */

static Boolean InitChosenMIDISystem()
{
	Boolean midiOK = True;
	
	if (useWhichMIDI == MIDIDR_CM) midiOK = InitCoreMIDI();
	
	LogPrintf(LOG_INFO, "useWhichMIDI=%d midiOK=%d  (InitChosenMIDISystem)\n", useWhichMIDI, midiOK);
	return midiOK;
}

static Boolean InitMIDISystem()
{
	/* For the foreseeable future, only support MacOS Core MIDI.  --DAB, April 2021 */
	
	useWhichMIDI = MIDIDR_CM;

	return InitChosenMIDISystem();
}


/* ------------------------------------------------------------------------ Palette(s) -- */

static void DisplayToolPaletteParams(PaletteGlobals *whichPalette)
{
	LogPrintf(LOG_INFO, "Displaying Tool Palette parameters:\n");
	LogPrintf(LOG_INFO, "  maxAcross=%d", whichPalette->maxAcross);
	LogPrintf(LOG_INFO, "  maxDown=%d", whichPalette->maxDown);
	LogPrintf(LOG_INFO, "  across=%d", whichPalette->across);
	LogPrintf(LOG_INFO, "  down=%d", whichPalette->down);
	LogPrintf(LOG_INFO, "  oldAcross=%d", whichPalette->oldAcross);
	LogPrintf(LOG_INFO, "  oldDown=%d", whichPalette->oldDown);
	LogPrintf(LOG_INFO, "  firstAcross=%d", whichPalette->firstAcross);
	LogPrintf(LOG_INFO, "  firstDown=%d\n", whichPalette->firstDown);
}

/* Do a reality check for palette values that might be bad. If no problems are found,
return True, else False. */

static Boolean CheckToolPaletteParams(PaletteGlobals *whichPalette)
{
	if (whichPalette->maxAcross<4 || whichPalette->maxAcross>150) return False;
	if (whichPalette->maxDown<4 || whichPalette->maxDown>150) return False;
	if (whichPalette->across<1 || whichPalette->across>80) return False;
	if (whichPalette->down<1 || whichPalette->down>80) return False;
	if (whichPalette->oldAcross<1 || whichPalette->oldAcross>80) return False;
	if (whichPalette->oldDown<1 || whichPalette->oldDown>80) return False;
	if (whichPalette->firstAcross<1 || whichPalette->firstAcross>80) return False;
	if (whichPalette->firstDown<1 || whichPalette->firstDown>80) return False;
	
	return True;
}

/* Allocate a grid of characters from the 'PLCH' resource; uses GridRec dataType to
store the information for a maximal maxRow by maxCol grid, where the dimensions are
stored in the PLCH resource itself and should match the BMP that is being used to draw
the palette.  We also initialize PaletteGlobals fields read from the resource and check
them. Returns the item number of the default tool (normally arrow), or 0 if problem. */

static short GetToolGrid(PaletteGlobals *whichPalette)
	{
		short maxRow, maxCol, row, col, item, defItem = 0; 
		short curResFile;
		GridRec *pGrid;
		Handle hdl;
		Byte *p;										/* Contains both binary and char data */
		
		curResFile = CurResFile();
		UseResFile(setupFileRefNum);

		hdl = GetResource('PLCH', ToolPaletteID);		/* get info on and char map of palette */
		if (!GoodResource(hdl)) {
			GetIndCString(strBuf, INITERRS_STRS, 6);	/* "can't get Tool Palette resource" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			UseResFile(curResFile);
			return(0);
			}
		
		UseResFile(curResFile);

		/* Pull in the maximum and suggested sizes for palette. */
		
		p = (unsigned char *)*hdl;
		whichPalette->maxAcross = *p++;
		whichPalette->maxDown = *p++;
		whichPalette->firstAcross = whichPalette->oldAcross = whichPalette->across = *p++;
		whichPalette->firstDown   = whichPalette->oldDown   = whichPalette->down   = *p++;
		
		/* FIXME: This comment (written by me between Oct. 2017 and Aug. 2018) makes no
		   sense: {We must _not_ "Endian fix" these values: they're read from one-byte
		   fields!} What was I thinking of? WHAT values?? <PaletteGlobals> has no one-
		   byte fields.  --DAB */
		
		if (DETAIL_SHOW) DisplayToolPaletteParams(whichPalette);
		LogPrintf(LOG_NOTICE, "Checking Tool Palette parameters: ");
		if (CheckToolPaletteParams(whichPalette)) LogPrintf(LOG_NOTICE, "No errors found.  (GetToolGrid)\n");
		else {
			if (!DETAIL_SHOW) DisplayToolPaletteParams(whichPalette);
			GetIndCString(strBuf, INITERRS_STRS, 34);	/* "Your Prefs file has illegal values for the tool palette..." */
			CParamText(strBuf, "", "", "");
			LogPrintf(LOG_ERR, "%s\n", strBuf);
			StopInform(GENERIC_ALRT);
			return 0;
		}
		
		/* Create storage for 2-D table of char/positions for palette grid */
		
		maxCol = whichPalette->maxAcross;
		maxRow = whichPalette->maxDown;
		
		grid = (GridRec *)NewPtr((Size)maxRow*maxCol*sizeof(GridRec));
		if (!GoodNewPtr((Ptr)grid)) {
			OutOfMemory((long)maxRow*maxCol*sizeof(GridRec));
			return 0;
			}

		/* And scan through resource to install grid cells */
		
		p = (unsigned char *) (*hdl + 4*sizeof(long));			/* Skip header stuff */
		pGrid = grid;
		item = 1;
		for (row=0; row<maxRow; row++)							/* initialize grid[] */
			for (col=0; col<maxCol; col++, pGrid++, item++) {
				SetPt(&pGrid->cell, col, row);					/* Doesn't move memory */
				if ((pGrid->ch = *p++) == CH_ENTER)
					if (defItem == 0) defItem = item; 
				}
		
		GetToolZoomBounds();
		return defItem;
	}


/* Install the rectangles for the given pallete rects array and its dimensions */

static void InitPaletteRects(Rect *whichRects, short itemsAcross, short itemsDown,
								short itemWidth, short itemHeight)
{
	short across, down;
	
	whichRects->left = 0;
	whichRects->top = 0;
	whichRects->right = 0;
	whichRects->bottom = 0;

	whichRects++;
	for (down=0; down<itemsDown; down++)
		for (across=0; across<itemsAcross; across++, whichRects++) {
			whichRects->left = across * itemWidth;
			whichRects->top = down * itemHeight;
			whichRects->right = (across + 1) * itemWidth;
			whichRects->bottom = (down + 1) * itemHeight;
			OffsetRect(whichRects, TOOLS_MARGIN, TOOLS_MARGIN);
		}
}


/* Initialise the tool palette: ??REWRITE! IGNORE THE PICT!!
We use the size of the PICT in resources as well as
the grid dimensions in the PLCH resource to determine the size of the palette, with a
margin around the cells so that there is space in the lower right corner for a grow box.
The PICT should have a frame size that is (TOOLS_ACROSS * TOOLS_CELL_WIDTH)-1 pixels
wide and (TOOLS_DOWN * TOOLS_CELL_HEIGHT) - 1 pixels high, where TOOLS_ACROSS and
TOOLS_DOWN are the values taken from the first two bytes of the PLCH resource. This
picture will be displayed centered in the palette window with a TOOLS_MARGIN pixel
margin on all sides. */

#define TOOL_PALETTE_FN		"ToolPaletteNB1b.bmp"

#define BITMAP_SPACE 30000

Byte bitmapTools[BITMAP_SPACE];

void CopyGWorldBitsToWindow(GWorldPtr offScreenGW, GrafPtr windPort, Rect offRect,
			Rect windRect)
{
	GrafPtr offScreenPort = 00000;		// ??????????????????WHAT??????????????????????????
	const BitMap *offPortBits = GetPortBitMapForCopyBits(offScreenPort);
	const BitMap *windPortBits = GetPortBitMapForCopyBits(windPort);
	CopyBits(offPortBits, windPortBits, &offRect, &windRect, srcCopy, NULL);
}


static Boolean InitToolPalette(PaletteGlobals *whichPalette, Rect *windowRect)
{
	short defaultToolItem, nRead, width, bWidth, bWidthPadded, height;
	FILE *bmpf;
	Rect maxToolsRect;
	long pixOffset, nBytesToRead;
	
	/* Allocate a grid of characters from the 'PLCH' resource and initialize the
	   PaletteGlobals fields stored in the resource. */
	
	defaultToolItem = GetToolGrid(whichPalette);
	if (!defaultToolItem) { BadInit(); ExitToShell(); }
		
	/* Now open the BMP file and read the actual bitmap image. */

	ParamText("\ptool", "\p", "\p", "\p");					/* In case of any of several problems */
	bmpf = NOpenBMPFile(TOOL_PALETTE_FN, &pixOffset, &width, &bWidth, &bWidthPadded, &height);
	if (!bmpf) {
		LogPrintf(LOG_ERR, "Bitmap image file '%s' is missing or can't be opened.  (InitToolPalette)\n", TOOL_PALETTE_FN);
		//if (CautionAdvise(BMP_ALRT)==OK) return False;
		return (CautionAdvise(BMP_ALRT)!=OK);
	}
	if (fseek(bmpf, pixOffset, SEEK_SET)!=0) {
		LogPrintf(LOG_ERR, "fseek to offset %ld in bitmap image file failed.  (InitToolPalette)\n",
					pixOffset);
		if (CautionAdvise(BMP_ALRT)==OK) return False;
	}

	nBytesToRead = bWidthPadded*height;
	LogPrintf(LOG_INFO, "bWidth=%d bWidthPadded=%d height=%d nBytesToRead=%d  (InitToolPalette)\n",
				bWidth, bWidthPadded, height, nBytesToRead);
	if (nBytesToRead>BITMAP_SPACE) {
		LogPrintf(LOG_ERR, "Bitmap needs %ld bytes but Nightingale allocated only %ld bytes.  (InitToolPalette)\n",
					nBytesToRead, BITMAP_SPACE);
		if (CautionAdvise(BMP_ALRT)==OK) return False;
	}
	nRead = fread(bitmapTools, nBytesToRead, 1, bmpf);
	if (nRead!=1) {
		LogPrintf(LOG_ERR, "Couldn't read the bitmap from image file.  (InitToolPalette)\n");
		if (CautionAdvise(BMP_ALRT)==OK) return False;
	}
	
	SetRect(&maxToolsRect, 0, 0, width, height);
	
	*windowRect = maxToolsRect;
		
	/* windowRect now has its top left corner at (0, 0). Use it to set toolRects[], the
	   bounding boxes for all the cells in the palette. Then resize windowRect for the
	   area that's initially visible, with a margin of TOOLS_MARGIN on every side. */
	
	InitPaletteRects(toolRects,
			whichPalette->maxAcross, whichPalette->maxDown,
			toolCellWidth = (windowRect->right + 1) / whichPalette->maxAcross,
			toolCellHeight = (windowRect->bottom + 1) / whichPalette->maxDown);
	
	windowRect->right -= (whichPalette->maxAcross-whichPalette->firstAcross) * toolCellWidth;
	windowRect->bottom -= (whichPalette->maxDown-whichPalette->firstDown) * toolCellHeight;
	windowRect->right += 2*TOOLS_MARGIN;
	windowRect->bottom += 2*TOOLS_MARGIN;
	//toolsFrame = *windowRect;
	//InsetRect(&toolsFrame, TOOLS_MARGIN, TOOLS_MARGIN);	/* FIXME: This is never used! */

	/* Avoid Issue #67, where the tool palette's initial position is sometimes at the
	   very top of screen, underneath the menu bar. FIXME: This is certainly not the
	   right fix! The problem occurs randomly on some computers, consistently on others,
	   so it's probably an uninitialized variable. */
	
	OffsetRect(windowRect, 0, 300);
	
	/* Finish initializing the PaletteGlobals structure. */
	
	whichPalette->currentItem = defaultToolItem;
	whichPalette->drawMenuProc = (void (*)())DrawToolPalette;
	whichPalette->findItemProc = (short (*)())FindToolItem;
	whichPalette->hiliteItemProc = (void (*)())HiliteToolItem;
		
LogPrintf(LOG_DEBUG, "InitToolPalette: windowRect tlbr=%d,%d,%d,%d toolCellWidth,Height=%d,%d\n",
windowRect->top, windowRect->left, windowRect->bottom, windowRect->right, toolCellWidth,
toolCellHeight);

	/* Put bitmap image into offscreen port so that any rearrangements can be saved. */

	SaveGWorld();
	
	GWorldPtr gwPtr = MakeGWorld(windowRect->right, windowRect->bottom, True);
	SetGWorld(gwPtr, NULL);

#ifdef DBG_TOOLS
//LogPrintf(LOG_DEBUG, "InitToolPalette: maxToolsRect tlbr=%d,%d,%d,%d\n", maxToolsRect.top,
//maxToolsRect.left, maxToolsRect.bottom, maxToolsRect.right);
//DHexDump(LOG_DEBUG, "ToolPal", bitmapTools, 4*16, 4, 16, True);
	short startLoc;
	for (short nRow = n_min(height, 800); nRow>=0; nRow--) {
		startLoc = nRow*bWidthPadded;
		DPrintRow(bitmapTools, nRow, n_min(bWidth, 20), startLoc, False, True);
		printf("\n");
	}
	ForeColor(yellowColor);
	SetRect(&maxToolsRect, 10, 60, 200, 200);
	FillRect(&maxToolsRect, NGetQDGlobalsGray());
	ForeColor(blackColor);
	SetRect(&maxToolsRect, 0, 0, width, height);
#endif

	//CopyGWorldBitsToWindow(gwPtr, GetQDGlobalsThePort(), *windowRect, *windowRect);
	//CopyOffScreenBitsToWindow(gwPtr, GetQDGlobalsThePort(), *windowRect, *windowRect);
	DrawBMP(bitmapTools, bWidth, bWidthPadded, height, maxToolsRect);
	toolPalPort = gwPtr;
#if 0
{
PixMapHandle portPixMapH = GetPortPixMap(toolPalPort); PixMapPtr portPixMap = *portPixMapH;
LogPixMapInfo("InitToolPalette2", portPixMap, 1000);
}
#endif
//	UnlockGWorld(gwPtr);
	RestoreGWorld();
	return True;
}


/* Initialize everything about the palettes used in floating windows. ??REWRITE OR REMOVE
FOLLOWING We've kept vestigal code for a help palette (which seems unlikely ever to be
used) and for a clavier palette (which really might be useful someday). */

Boolean NInitPaletteWindows()
{
	short idx, wdefID;
	PaletteGlobals *whichPalette;
	Rect windowRects[TOTAL_PALETTES];
	short totalPalettes = TOTAL_PALETTES;
	
	for (idx=0; idx<totalPalettes; idx++) {
	
		/* Get a handle to a PaletteGlobals structure for this palette */
		
		paletteGlobals[idx] = (PaletteGlobals **)GetResource('PGLB', ToolPaletteWDEF_ID+idx);
		if (!GoodResource((Handle)paletteGlobals[idx])) return False;
		EndianFixPaletteGlobals(idx);

		MoveHHi((Handle)paletteGlobals[idx]);
		HLock((Handle)paletteGlobals[idx]);
		whichPalette = *paletteGlobals[idx];
		
		switch(idx) {
			case TOOL_PALETTE:
				if (!InitToolPalette(whichPalette, &windowRects[idx])) return False;
				wdefID = ToolPaletteWDEF_ID;
				break;
			case HELP_PALETTE:
				wdefID = ToolPaletteWDEF_ID;						// ??REALLY?
				break;
			case CLAVIER_PALETTE:
				break;
		}

		wdefID = floatGrowProc;
		
		palettes[idx] = (WindowPtr)NewCWindow(NULL, &windowRects[idx], "\p", False,
								wdefID, BRING_TO_FRONT, True, (long)idx);
		if (!GoodNewPtr((Ptr)palettes[idx])) return False;
	
		//	((WindowPeek)palettes[idx])->spareFlag = (idx==TOOL_PALETTE || idx==HELP_PALETTE);
		
		/* Add a zoom box to tools and help palette */
		
		if (idx==TOOL_PALETTE || idx==HELP_PALETTE)
			ChangeWindowAttributes(palettes[idx], kWindowFullZoomAttribute, kWindowNoAttributes);

		/* Finish initializing the TearOffMenuGlobals structure. */
		
		whichPalette->environment = &thisMac;
		whichPalette->paletteWindow = palettes[idx];
		SetWindowKind(palettes[idx], PALETTEKIND);
		
		HUnlock((Handle)paletteGlobals[idx]);
	}

	return True;
}


/* --------------------------------------------------- Initialize palettes for dialogs -- */

#define DYNAMIC_PALETTE_FN		"DynamicsNB1b.bmp"

Boolean InitDynamicPalette()
{
	short nRead, width, bWidth, bWidthPadded, height;
	FILE *bmpf;
	long pixOffset, nBytesToRead;

	/* Open the BMP file and read the actual bitmap image. */

	ParamText("\pdynamics", "\p", "\p", "\p");					/* In case of any of several problems */
	bmpf = NOpenBMPFile(DYNAMIC_PALETTE_FN, &pixOffset, &width, &bWidth, &bWidthPadded,
						&height);
	if (!bmpf) {
		LogPrintf(LOG_ERR, "Bitmap image file '%s' is missing or can't be opened.  (InitDynamicPalette)\n",
					DYNAMIC_PALETTE_FN);
		if (CautionAdvise(BMP_ALRT)==OK) return False;
		else {
			/* Use reasonable values for the bitmap parameters. */
		
			bmpDynamicPal.bWidth = 15;
			bmpDynamicPal.bWidthPadded = 16;
			bmpDynamicPal.height = 52;
			return True;
		}
	}

	if (fseek(bmpf, pixOffset, SEEK_SET)!=0) {
		LogPrintf(LOG_ERR, "fseek to offset %ld in bitmap image file '%s' failed.  (InitDynamicPalette)\n",
					pixOffset, DYNAMIC_PALETTE_FN);
		if (CautionAdvise(BMP_ALRT)==OK) return False;
	}

	nBytesToRead = bWidthPadded*height;
	LogPrintf(LOG_INFO, "bWidth=%d bWidthPadded=%d height=%d nBytesToRead=%d  (InitDynamicPalette)\n",
				bWidth, bWidthPadded, height, nBytesToRead);
	if (nBytesToRead>BITMAP_SPACE) {
		LogPrintf(LOG_ERR, "Bitmap needs %ld bytes but Nightingale allocated only %ld bytes.  (InitDynamicPalette)\n",
					nBytesToRead, BITMAP_SPACE);
		if (CautionAdvise(BMP_ALRT)==OK) return False;
	}
	nRead = fread(bmpDynamicPal.bitmap, nBytesToRead, 1, bmpf);
	if (nRead!=1) {
		LogPrintf(LOG_ERR, "Couldn't read the bitmap from image file.  (InitDynamicPalette)\n");
		if (CautionAdvise(BMP_ALRT)==OK) return False;
	}

	bmpDynamicPal.bWidth = bWidth;
	bmpDynamicPal.bWidthPadded = bWidthPadded;
	bmpDynamicPal.height = height;
	return True;
}


#define MODNR_PALETTE_FN		"NRModifierNB1b.bmp"

Boolean InitModNRPalette()
{
	short nRead, width, bWidth, bWidthPadded, height;
	FILE *bmpf;
	long pixOffset, nBytesToRead;

	/* Open the BMP file and read the actual bitmap image. */

	ParamText("\pnote/rest modifier", "\p", "\p", "\p");				/* In case of any of several problems */
	bmpf = NOpenBMPFile(MODNR_PALETTE_FN, &pixOffset, &width, &bWidth, &bWidthPadded,
						&height);
	if (!bmpf) {
		LogPrintf(LOG_ERR, "Bitmap image file '%s' is missing or can't be opened.  (InitModNRPalette)\n",
					MODNR_PALETTE_FN);
		if (CautionAdvise(BMP_ALRT)==OK) return False;
		else {
			/* Use reasonable values for the bitmap parameters. */
		
			bmpModifierPal.bWidth = 18;
			bmpModifierPal.bWidthPadded = 20;
			bmpModifierPal.height = 104;
			return True;
		}
	}

	if (fseek(bmpf, pixOffset, SEEK_SET)!=0) {
		LogPrintf(LOG_ERR, "fseek to offset %ld in bitmap image file '%s' failed.  (InitModNRPalette)\n",
					pixOffset, MODNR_PALETTE_FN);
		if (CautionAdvise(BMP_ALRT)==OK) return False;
	}

	nBytesToRead = bWidthPadded*height;
	LogPrintf(LOG_INFO, " bWidth=%d bWidthPadded=%d height=%d nBytesToRead=%d  (InitModNRPalette)\n",
	bWidth, bWidthPadded, height, nBytesToRead);
	if (nBytesToRead>BITMAP_SPACE) {
		LogPrintf(LOG_ERR, "Bitmap needs %ld bytes but Nightingale allocated only %ld bytes.  (InitModNRPalette)\n",
					nBytesToRead, BITMAP_SPACE);
		if (CautionAdvise(BMP_ALRT)==OK) return False;
	}
	nRead = fread(bmpModifierPal.bitmap, nBytesToRead, 1, bmpf);
	if (nRead!=1) {
		LogPrintf(LOG_ERR, "Couldn't read the bitmap from image file.  (InitModNRPalette)\n");
		if (CautionAdvise(BMP_ALRT)==OK) return False;
	}

	bmpModifierPal.bWidth = bWidth;
	bmpModifierPal.bWidthPadded = bWidthPadded;
	bmpModifierPal.height = height;
	return True;
}


#define DURATIONS_PALETTE_FN		"Duration_2dotsNB1b.bmp"

Boolean InitDurationPalette()
{
	FILE *bmpf;
	short nRead, width, bWidth, bWidthPadded, height;
	long pixOffset, nBytesToRead;

	/* Open the BMP file and read the actual bitmap image. */

	ParamText("\pnote/rest duration", "\p", "\p", "\p");				/* In case of any of several problems */
	bmpf = NOpenBMPFile(DURATIONS_PALETTE_FN, &pixOffset, &width, &bWidth, &bWidthPadded,
						&height);
	if (!bmpf) {
		LogPrintf(LOG_ERR, "Bitmap image file '%s' is missing or can't be opened.  (InitDurationPalette)\n",
					DURATIONS_PALETTE_FN);
		if (CautionAdvise(BMP_ALRT)==OK) return False;
		else {
			/* Use reasonable values for the bitmap parameters. */
		
			bmpDurationPal.bWidth = 27;
			bmpDurationPal.bWidthPadded = 28;
			bmpDurationPal.height = 78;
			return True;
		}
	}

	if (fseek(bmpf, pixOffset, SEEK_SET)!=0) {
		LogPrintf(LOG_ERR, "fseek to offset %ld in bitmap image file failed.  (InitDurationPalette)\n",
					pixOffset);
		return False;
	}
		
	nBytesToRead = bWidthPadded*height;
	if (nBytesToRead>BITMAP_SPACE) {
		LogPrintf(LOG_ERR, "Bitmap needs %ld bytes but Nightingale allocated only %ld bytes.  (InitDurationPalette)\n",
					nBytesToRead, BITMAP_SPACE);
		return False;
	}
	nRead = fread(bmpDurationPal.bitmap, nBytesToRead, 1, bmpf);
	if (nRead!=1) {
		LogPrintf(LOG_ERR, "Couldn't read the bitmap from image file.  (InitDurationPalette)\n");
		return False;
	}

	bmpDurationPal.bWidth = bWidth;
	bmpDurationPal.bWidthPadded = bWidthPadded;
	bmpDurationPal.height = height;
//DHexDump(LOG_DEBUG, "Duration", bmpDurationPal.bitmap, 4*16, 4, 16, True);
	LogPrintf(LOG_INFO, "bWidth=%d bWidthPadded=%d height=%d nBytesToRead=%d  (InitDurationPalette)\n",
				bmpDurationPal.bWidth, bmpDurationPal.bWidthPadded, bmpDurationPal.height);
	return True;
}
