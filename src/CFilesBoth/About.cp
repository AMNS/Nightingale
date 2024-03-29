/* About.c: routines for the About box, including animating the credits, etc. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* ------------------------------------------------------------------ DoAboutBox et al -- */

#define	CR_LEADING			14		/* Vert. dist. between baselines of credit text */
#define	SCROLL_PAUSE_DELAY	90		/* Ticks to pause at lines begining with PAUSE_CODE before scrolling */
#define	SCROLL_NORM_DELAY	4		/* Approx. ticks to wait before scrolling credit list up 1 pixel */
#define	MAX_PAUSE_LINES		20		/* Max number of lines that can begin with PAUSE_CODE */

/* These codes must begin a line of the TEXT resource. If both are present, PAUSE_CODE
must appear first. */

#define	PAUSE_CODE			0xB9	/* [opt-p in Mac Roman] Pause scrolling (cf. SCROLL_PAUSE_DELAY) */
#define	BOLD_CODE			0xBA	/* [opt-b in Mac Roman] Draw this line in bold */

static pascal Boolean	AboutFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static Boolean			SetupCredits(void);
static void				AnimateCredits(DialogPtr dlog);

enum {
	BUT1_OK = 1,
	BUT2_Special,
	CREDITS_BOX = 3,
	STXT_HINT1,
	STXT_HINT2,
	STXT_VERS
};

static GrafPtr	fullTextPort;
static Rect		creditRect, textSection;
static short	pauseLines[MAX_PAUSE_LINES];
static Boolean	firstAnimateCall;


void DoAboutBox(
	Boolean /*first*/			/* Currently unused */
	)
{
	short			type, itemHit;
	Boolean			okay, keepGoing=True;
	DialogPtr		dlog;
	GrafPtr			oldPort;
	Handle			hndl;
	Rect			box;
	char			fmtStr[256], str[256], serialStr[256], userName[256], userOrg[256];
	Str255			versionPStr;
	ModalFilterUPP	filterUPP;

	/* Build dialog window and install its item values */
	
	GetPort(&oldPort);

	GetIndCString(fmtStr, DIALOG_STRS, 5);   		 		/* "Free memory (RAM): %ldK total, largest block = %ldK" */
	sprintf(str, fmtStr, FreeMem()/1024L, MaxBlock()/1024L);
	CParamText(str, serialStr, userName, userOrg);
	
	filterUPP = NewModalFilterUPP(AboutFilter);
	if (filterUPP == NULL) {
		MissingDialog(ABOUT_DLOG);
		return;
	}
	dlog = GetNewDialog(ABOUT_DLOG, 0L, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(ABOUT_DLOG);
		return;
	}
	
	/* Get userItem rect for printing animated credits list and prepare offsceen port. */
	
	GetDialogItem(dlog, CREDITS_BOX, &type, &hndl, &creditRect);
	textSection = creditRect;
	OffsetRect(&textSection, -creditRect.left, -creditRect.top);
	if (!SetupCredits()) goto broken;
	
	/* Get version number string and display it in a static text item. */
	
	strcpy((char *)versionPStr, applVerStr);
	CToPString((char *)versionPStr);
	PutDlgString(dlog, STXT_VERS, versionPStr, False);

	GetDialogItem(dlog, BUT2_Special, &type, &hndl, &box);
	HiliteControl((ControlHandle)hndl, CTL_INACTIVE);
	HideDialogItem(dlog, BUT2_Special);

	SetPort(GetDialogWindowPort(dlog));
	TextFont(textFontNum);
	TextSize(textFontSmallSize);

	CenterWindow(GetDialogWindow(dlog), 70);
	ShowWindow(GetDialogWindow(dlog));
	
	/* Show the first "screen" of animated text */
	
	firstAnimateCall = True;
	CopyOffScreenBitsToWindow(fullTextPort, GetDialogWindowPort(dlog), textSection, creditRect);
	ArrowCursor();

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP, &itemHit);
		if (itemHit<1) continue;

		GetDialogItem(dlog, itemHit, &type, &hndl, &box);
		switch(itemHit) {
			case BUT1_OK:
				keepGoing = False; okay = True;
				break;
			case BUT2_Special:
				SysBeep(1);		/* For future use; maybe "Debug Check", or "Print" (to log?) */
				break;
		}
	}
	
	DestroyGWorld(fullTextPort);
	HideWindow(GetDialogWindow(dlog));
broken:	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
}


static pascal Boolean AboutFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Boolean		ans=False;
	WindowPtr	w;
	GrafPtr		oldPort;
	int			ch;
	
	AnimateCredits(dlog);
	
	w = (WindowPtr)(evt->message);
	switch(evt->what) {
		case updateEvt:
			if (w == GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));
		
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, BUT1_OK, True);
		
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);

				ans = True; *itemHit = 0;
			}
			break;
		case activateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetDialogWindowPort(dlog));
				*itemHit = 0;
			}
			break;
		case mouseDown:
		case mouseUp:
			break;
		case keyDown:
			ch = (unsigned char)evt->message;
			if (ch=='\r' || ch==CH_ENTER) {
				FlashButton(dlog, BUT1_OK);
				*itemHit = BUT1_OK;
				ans = True;
			}
			break;
	}
	return(ans);
}


/* Prepare an offscreen port holding lines of text read from a TEXT resource.
AnimateCredits() should copy a sliding rectangle from this port onto the screen every
few ticks (specified by SCROLL_NORM_DELAY). */
	
Boolean SetupCredits()
{
	Handle	hCreditText;
	char	*textPtr, *strP, *thisStr;
	short	numLines, lineNum, strWid, offset, pauseLineCount, i;
	long	textLen;
	short	portWid;
	
	/* Get credit text from TEXT resource. (Get1Resource isn't worth the trouble.) */
	
	hCreditText = GetResource('TEXT', ABOUT_TEXT);
	if (!hCreditText) {
		MayErrMsg("AboutBox: Can't get credit text.");
		return False;
	}
	textLen = GetResourceSizeOnDisk(hCreditText);
	
	/* Lock and dereference handle to text and count the lines in it */
	
	HLock(hCreditText);
	textPtr = (char *) *hCreditText;
	
	for (i=0, numLines=0; i<textLen; i++, textPtr++)
		if (*textPtr == CH_CR) numLines++;
	textPtr = (char *) *hCreditText;								/* reset ptr */

	/* Save the current graphics environment and switch to an offscreen port to hold
	   formatted text. */

	SaveGWorld();
	
	portWid = creditRect.right - creditRect.left;			/* creditRect is static global declared above */
	GWorldPtr gwPtr = MakeGWorld(portWid, numLines * CR_LEADING, True);
	SetGWorld(gwPtr, NULL);
	
	TextFont(textFontNum);
	TextSize(textFontSmallSize);

	/* Initialize pauseLines[] so that we won't pause inadvertently on a line whose
	   number happens to be given by garbage. */
	   
	for (i=0; i<MAX_PAUSE_LINES; i++)
		pauseLines[i] = -1;
	
	/* Break text into lines and draw them into offscreen port, interpreting the codes
	   that might begin a line. NB: the pause code must precede the bold code if both
	   are used in the same line. After the first MAX_PAUSE_LINES pause codes have been
	   interpreted, any following ones are ignored. */
	   
	thisStr = strP = textPtr;
	for (i=0, lineNum=1, pauseLineCount=0; i<textLen; i++) {
		if (*textPtr == CH_CR) {
			*strP = '\0';										/* this string complete */

			/* Parse next line for codes and handle any we find. */

			if (*thisStr==(char)PAUSE_CODE) {
				if (pauseLineCount<=MAX_PAUSE_LINES)
					pauseLines[pauseLineCount++] = lineNum;
				thisStr++;
			}
			if (*thisStr==(char)BOLD_CODE) {
				TextFace(bold);									/* style is bold */
				thisStr++;
			}
			else TextFace(0);									/* style is plain */
			
			/* Compute position for centering, draw text, and advance line number. */
			
			strWid = TextWidth(thisStr, 0, strlen(thisStr));
			offset = (portWid - strWid) / 2;
			MoveTo(offset, lineNum * CR_LEADING);
			DrawText(thisStr, 0, strlen(thisStr));
			lineNum++;
			thisStr = strP = ++textPtr;
		}
		else *strP++ = *textPtr++;
	}

	fullTextPort = gwPtr;
	
	UnlockGWorld(gwPtr);
	RestoreGWorld();
	
	HUnlock(hCreditText);
	ReleaseResource(hCreditText);	
	
	return True;
}


/* Slide the list of credits up one pixel by copying desired rectangle from offscreen
port containing the complete list to the userItem rect on screen. If we've hit a line
starting with PAUSE_CODE, we wait SCROLL_PAUSE_DELAY ticks before resuming animation. */
		
void AnimateCredits(DialogPtr dlog)
{
	long			thisTime;
	static long		lastTime;
	static short	pixelCount;
	short			i, thisLineNum;
	Boolean			doPause=False;

	thisTime = TickCount();
	
	/* If this is the first time called since the dialog was put up, initialize some
	   static variables. */
		
	if (firstAnimateCall == True) {
		firstAnimateCall = False;
		lastTime = thisTime;
		pixelCount = CR_LEADING;
	}
	
	/* If line number changed, check to see if we should pause on this line. */
	   
	thisLineNum = pixelCount / CR_LEADING;

	if (!(pixelCount % CR_LEADING)) {
		for (i=0; i<MAX_PAUSE_LINES; i++) {
			if (pauseLines[i]==thisLineNum) doPause = True;
		}
	}
			
	if (thisTime < (lastTime + 
			(doPause ? SCROLL_PAUSE_DELAY : SCROLL_NORM_DELAY))) return;
			
	/* If we're about to run off the bottom, start at top again */
	
	Rect ftPortRect;
	GetPortBounds(fullTextPort, &ftPortRect);
	if (textSection.bottom > ftPortRect.bottom) {
		textSection = creditRect;
		OffsetRect(&textSection, -creditRect.left, -creditRect.top);
		pixelCount = CR_LEADING-1;
		
		/* Incremented to CR_LEADING below, before we get back to line number computation.
		   Skip array search, and thus pause, if this computation yields a remainder. */
	}
		
	CopyOffScreenBitsToWindow(fullTextPort, GetDialogWindowPort(dlog), textSection, creditRect);
					
	textSection.top++;  textSection.bottom++;
	pixelCount++;

	lastTime = thisTime;
}
