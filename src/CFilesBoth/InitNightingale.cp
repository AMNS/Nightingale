/* InitNightingale.c for Nightingale
One-time initialization of Nightingale-specific stuff. Assumes the Macintosh
toolbox has already been initialized and the config struct filled in. */

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
static void		InitNightFonts(void);
static Boolean	InitNightGlobals(void);
static Boolean	InitTables(void);
static void 	DisplayInfo(void);
static void 	CheckScreenFonts(void);
static void		InitMusicFonts(void);
static void 	CheckUIFonts(void);
static Boolean	InitMIDISystem(void);

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
	InitMusicFonts();
	CheckUIFonts();
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
the copy serial number: if it was wrong, return False. */

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


/* Set globals describing our standard text font and our standard music font. */

void InitNightFonts()
{
	textFontNum = applFont;
	
	/* NB: The following comment is pre-OS X.
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
			if (ch<256)
				SetRect(&musFontInfo[index].cBBox[ch], xl, yt, xr, yb);
			else
				MayErrMsg("Info on font '%s' refers to illegal char code %u.  (InitMusFontTables)\n",
							musFontName, ch);
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

void InitMusicFonts()
{
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
	CheckScreenFonts();
}


/* Check if the user-interface fonts are present and report what we find. NB: Nightingale
actually gets the names of the fonts from 'chgd' resources. The compiled-in font names
this checks for may not be correct! But I tried to get the names from the resources;
that mysteriously failed, and it's not worth much time, since they'll change only once
in many blue moons. ??FIX! USE DUR_MENU_FONTNAME ETC., NOT 'chgd'!! */

static void CheckUIFonts()
{
	short fontNum;
	Str255 durMenuFontname, dynModMenuFontname;
	char fontNameC[256];
	Boolean missingFont=False;
#if 0
	Handle resH;
	PCHARGRID chgdP;
	
	resH = Get1Resource('chgd', NODOTDUR_MENU);
	if (resH==NULL)	return False;
	chgdP = (PCHARGRID)*resH;
	Pstrcpy((unsigned char *)fontNameC, chgdP->fontName);
	ETC. ETC.
#else
	Pstrcpy(durMenuFontname, DUR_MENU_FONTNAME);
	Pstrcpy(dynModMenuFontname, DYNMOD_MENU_FONTNAME);
#endif

	/* Duration font, for Set Duration command graphic pop-up (NODOTDUR_MENU & ff. resources) */
	
	if (!GetFontNumber(durMenuFontname, &fontNum)) {
		Pstrcpy((unsigned char *)fontNameC, durMenuFontname);
		LogPrintf(LOG_WARNING, "UI font '%s' for Set Duration not found.  (CheckUIFonts)\n",
			PToCString((unsigned char *)fontNameC));
		missingFont = True;
	}

	/* Dynamics and note modifers font, for change dynamic and Add Modifier graphic pop-ups
	   (DYNAMIC_MENU and MODIFIER_MENU resources) */

	if (!GetFontNumber(dynModMenuFontname, &fontNum)) {
		Pstrcpy((unsigned char *)fontNameC, dynModMenuFontname);
		LogPrintf(LOG_WARNING, "UI font '%s' for [change dynamic] & Add Modifier not found.  (CheckUIFonts)\n",
			PToCString((unsigned char *)fontNameC));
		missingFont = True;
	}

	if (!missingFont) LogPrintf(LOG_NOTICE, "Found all User Interface fonts.  (CheckUIFonts)\n");
}


static Boolean InitChosenMIDISystem()
{
	Boolean midiOK = True;
	
	if (useWhichMIDI == MIDIDR_CM)
		midiOK = InitCoreMIDI();
	
	LogPrintf(LOG_INFO, "useWhichMIDI=%d midiOK=%d  (InitChosenMIDISystem)\n", useWhichMIDI, midiOK);
	return midiOK;
}

Boolean InitMIDISystem()
{
	/* For the foreseeable future, only support MacOS Core MIDI.  --DAB, April 2021 */
	
	useWhichMIDI = MIDIDR_CM;

	return InitChosenMIDISystem();
}
