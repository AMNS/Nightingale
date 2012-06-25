/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/* About.c: routines for the About box, animating the credits, etc. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* ------------------------------------------------------------- DoAboutBox et al -- */

#define NO_SERIAL_NUM

#define	CR_LEADING				14		/* Vert. dist. between baselines of credit text */
#define	PAUSE_CODE				'¹'	/* [opt-p] If line of TEXT resource begins with this,
													animation will pause at this line (cf. SCROLL_PAUSE_DELAY). */
#define	BOLD_CODE				'º'	/* [opt-b] If this begins a line of TEXT resource, or follows
													PAUSE_CODE, that line will be drawn in bold. */
#define	SCROLL_PAUSE_DELAY	210	/* Ticks to pause at lines begining with PAUSE_CODE before scrolling */
#define	SCROLL_NORM_DELAY		4		/* Approx. ticks to wait before scrolling credit list up 1 pixel */
#define	MAX_PAUSE_LINES		10		/* Max number of lines that can begin with PAUSE_CODE */

static Boolean			GetSNAndOwner(char [], char [], char []);
static pascal Boolean	AboutFilter(DialogPtr dlog, EventRecord *evt, short *itemHit);
static Boolean			SetupCredits(void);
static void				AnimateCredits(DialogPtr dlog);

static enum {
	BUT1_OK = 1,
	BUT2_Special,
	CREDITS_BOX = 5,
	STXT6_Memory,
	STXT7_CopyNum,
	PICT8_Confident = 8,
	STXT_DEMO = 11,
	PICT_DEMO,
	STXT_HINT1,
	STXT_HINT2,
	STXT_VERS
} E_AboutItems;


/* Get information about this copy of Nightingale: its serial number and the
name and organization of its owner. 
 Ignore serial number for now --chirgwin Mon May 28 16:02:01 PDT 2012
 */

static Boolean GetSNAndOwner(char *serialStr, char *userName, char *userOrg)
{
	serialStr[0] = '\0';
	userName[0] = '\0';
	userOrg[0] = '\0';
	
	/* If the owner's name and organization aren't filled in, it's no big deal--
		they should have the default values. */
	
	GetIndCString(userName, 1000, 1);
	GetIndCString(userOrg, 1000, 2);

	return TRUE;
}

static GrafPtr	fullTextPort;
static Rect		creditRect, textSection;
static short	pauseLines[MAX_PAUSE_LINES];
static Boolean	firstAnimateCall;

void DoAboutBox(
	Boolean /*first*/			/* Currently unused */
	)
{
	short			type, itemHit, curResFile;
	short			x, y;
	Rect			smallRect, bigRect;
	Boolean		okay, keepGoing=TRUE;
	DialogPtr	dlog;
	GrafPtr		oldPort;
	Handle		hndl;
	Rect			box;
	char			fmtStr[256], str[256], serialStr[256], userName[256], userOrg[256];
	ModalFilterUPP	filterUPP;

	/* Build dialog window and install its item values */
	
	GetPort(&oldPort);
	
	GetSNAndOwner(serialStr, userName, userOrg);

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
	
	/* Get userItem rect for printing animated credits list */
	GetDialogItem(dlog, CREDITS_BOX, &type, &hndl, &creditRect);
	textSection = creditRect;
	OffsetRect(&textSection, -creditRect.left, -creditRect.top);
	if (!SetupCredits()) goto broken;

	HideDialogItem(dlog, STXT_DEMO);
	HideDialogItem(dlog, PICT_DEMO);

#ifdef PUBLIC_VERSION
	HideDialogItem(dlog, PICT8_Confident);
#else
	ShowDialogItem(dlog, PICT8_Confident);
#endif

#ifdef NO_SERIAL_NUM
	HideDialogItem(dlog, STXT7_CopyNum);
#else
	ShowDialogItem(dlog, STXT7_CopyNum);
#endif
	
	
	/* Get version number string and display it in a static text item. */
	unsigned char vstr[256], *vers_str;
	const char *bundle_version_str;
	
	/* Get version number from main bundle (Info.plist); 
	   this is the string value for key "CFBundleVersion" aka "Bundle version" 
	 */
	CFStringRef vsr = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), kCFBundleVersionKey);
	bundle_version_str = CFStringGetCStringPtr(vsr, kCFStringEncodingMacRoman);
	vers_str = CToPString((char *) bundle_version_str);
	vstr[0] = 0;
	PStrCopy("\pv. ", vstr);
	PStrCat(vstr, vers_str);
	PutDlgString(dlog, STXT_VERS, vstr, FALSE);

	GetDialogItem(dlog, BUT2_Special, &type, &hndl, &box);
	HiliteControl((ControlHandle)hndl, CTL_INACTIVE);
#ifdef NO_SERIAL_NUM
	HideDialogItem(dlog, BUT2_Special);
#endif

	SetPort(GetDialogWindowPort(dlog));
	TextFont(textFontNum);
	TextSize(textFontSmallSize);

	y = GetMBarHeight()+10; x = 25;
	SetRect(&smallRect, x, y, x+2, y+2);
	CenterWindow(GetDialogWindow(dlog), 70);
	GetWindowPortBounds(GetDialogWindow(dlog),&bigRect);
	LocalToGlobal(&TOP_LEFT(bigRect));
	LocalToGlobal(&BOT_RIGHT(bigRect));
	//ZoomRect(&smallRect, &bigRect, TRUE);
	ShowWindow(GetDialogWindow(dlog));
	
	/* Show the first "screen" of animated text */
	firstAnimateCall = TRUE;
	
	const BitMap *ftpPortBits = GetPortBitMapForCopyBits(fullTextPort);
	const BitMap *dlogPortBits = GetPortBitMapForCopyBits(GetDialogWindowPort(dlog));
	CopyBits(ftpPortBits, dlogPortBits, &textSection, &creditRect, srcCopy, NULL);

//	CopyBits(&fullTextPort->portBits, &dlog->portBits,	&textSection,
//					&creditRect, srcCopy, NULL);
	
	ArrowCursor();

	/* Entertain filtered user events until dialog is dismissed */
	while (keepGoing) {
		ModalDialog(filterUPP, &itemHit);

		if (itemHit<1) continue;

		GetDialogItem(dlog, itemHit, &type, &hndl, &box);
		switch(itemHit) {
			case BUT1_OK:
				keepGoing = FALSE; okay = TRUE;
				break;
			case BUT2_Special:
#ifdef NOTYET
				{
					Document *doc=GetDocumentFromWindow(TopDocument);
					if (doc!=NULL)
						DCheckEverything(doc, maxCheck, minCheck);	/* ??Requires Consolationª */
				}
#else
				SysBeep(1);		/* For future use; maybe a simple "Debug Check" */
#endif
				break;
		}
	}
	
	DestroyGWorld(fullTextPort);
	
	HideWindow(GetDialogWindow(dlog));
	//ZoomRect(&smallRect, &bigRect, FALSE);
broken:	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
}


static pascal Boolean AboutFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Boolean ans=FALSE;
	WindowPtr		w;
	GrafPtr			oldPort;
	int				ch;
	
	AnimateCredits(dlog);
	
	w = (WindowPtr)(evt->message);
	switch(evt->what) {
		case updateEvt:
			if (w == GetDialogWindow(dlog)) {
				GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
				BeginUpdate(GetDialogWindow(dlog));
		
				UpdateDialogVisRgn(dlog);
				FrameDefault(dlog, BUT1_OK, TRUE);
		
				EndUpdate(GetDialogWindow(dlog));
				SetPort(oldPort);

				ans = TRUE; *itemHit = 0;
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
				ans = TRUE;
			}
			break;
	}
	return(ans);
}


/* Prepare an offscreen port holding lines of text read from a TEXT resource.
	AnimateCredits() will copy a sliding rectangle from this port onto the 
	screen every few ticks (specified by SCROLL_NORM_DELAY). */
	
Boolean SetupCredits()
{
	Handle		creditText;
	char			*textPtr, *strP, *thisStr;
	short			numLines, lineNum, strWid, offset, pauseLineCount, i;
	long			textLen;
	short			portWid;
	
	/* Get credit text from TEXT resource. (Get1Resource isn't worth the trouble.) */
	creditText = GetResource('TEXT', ABOUT_TEXT);
	if (!creditText) {
		MayErrMsg("AboutBox: Can't get credit text.");
		return FALSE;
	}
	textLen = GetResourceSizeOnDisk(creditText);
	
	/* Lock and dereference handle to text */
	HLock(creditText);
	textPtr = (char *) *creditText;
		
	/* How many lines in text? */
	for (i=0, numLines=0; i<textLen; i++, textPtr++) {
		if (*textPtr == CH_CR) numLines++;
	}
	textPtr = (char *) *creditText;						/* reset ptr */

	SaveGWorld();
	
	/* Create offscreen port to hold formatted text */

	portWid = creditRect.right - creditRect.left;					/* creditRect is static global declared above */
	GWorldPtr gwPtr = MakeGWorld(portWid, numLines * CR_LEADING, TRUE);
	SetGWorld(gwPtr, NULL);
	
	TextFont(textFontNum);
	TextSize(textFontSmallSize);

	/* Initialize pauseLines[], so that we won't pause inadvertantly
		on a line whose number happens to be given by garbage. */
	for (i=0; i<MAX_PAUSE_LINES; i++)
		pauseLines[i] = -1;
	
	/* Break text into lines, draw them into offscreen port, interpreting
		the codes that might begin a line.
		NB: the pause code must precede the bold code if both are used in
		the same line.  After the first MAX_PAUSE_LINES pause codes have been
		interpreted, any following ones are ignored. */
	thisStr = strP = textPtr;
	for (i=0, lineNum=1, pauseLineCount=0; i<textLen; i++) {
		if (*textPtr == CH_CR) {
			*strP = '\0';													/* this string complete */
			/* Parse next line for codes */
			if (*thisStr==PAUSE_CODE) {
				if (pauseLineCount<=MAX_PAUSE_LINES)
					pauseLines[pauseLineCount++] = lineNum;		/* put this line number in array of lines to pause on */
				thisStr++;
			}
			if (*thisStr==BOLD_CODE) {
				TextFace(bold);											/* style is bold */
				thisStr++;
			}
			else TextFace(0);												/* style is plain */
			strWid = TextWidth(thisStr, 0, strlen(thisStr));	/* compute position for centered text */
			offset = (portWid - strWid) / 2;
			MoveTo(offset, lineNum * CR_LEADING);					/* draw text */
			DrawText(thisStr, 0, strlen(thisStr));
			lineNum++;														/* advance line number */
			thisStr = strP = ++textPtr;
		}
		else *strP++ = *textPtr++;
	}

	fullTextPort = gwPtr;
	
	UnlockGWorld(gwPtr);
	RestoreGWorld();
	
	HUnlock(creditText);
	ReleaseResource(creditText);	
	
	return TRUE;
}


/* Slide the list of credits up one pixel.  Copies desired rectangle from offscreen
	port containing the complete list to the userItem rect on screen. If we've hit a
	line beginning with PAUSE_CODE, we wait SCROLL_PAUSE_DELAY ticks before resuming
	animation. */
		
void AnimateCredits(DialogPtr dlog)
{
	long				thisTime;
	static long		lastTime;
	static short	pixelCount;
	short				i, thisLineNum;
	Boolean			doPause=FALSE;

	thisTime = TickCount();
	
	/* Is this first time called since the dialog was put up?
		[AboutBox() initializes firstAnimateCall to TRUE.]
		If so, we must initialize some static variables. */
	if (firstAnimateCall == TRUE) {
		firstAnimateCall = FALSE;
		lastTime = thisTime;
		pixelCount = CR_LEADING;
	}
	
	/* Compute current line number */
	thisLineNum = pixelCount / CR_LEADING;

	/* If line number has changed, check array to see if we should pause on this line. */
	if (!(pixelCount % CR_LEADING)) {
		for (i=0; i<MAX_PAUSE_LINES; i++) {
			if (pauseLines[i]==thisLineNum) doPause = TRUE;
		}
	}
			
	if (thisTime < (lastTime + 
			(doPause ? SCROLL_PAUSE_DELAY : SCROLL_NORM_DELAY))) return;
			
	/* If we're about to run off the bottom, start at top again */
	Rect ftPortRect;
	GetPortBounds(fullTextPort,&ftPortRect);
	if (textSection.bottom > ftPortRect.bottom) {
		textSection = creditRect;
		OffsetRect(&textSection, -creditRect.left, -creditRect.top);
		pixelCount = CR_LEADING-1;
		/* Incremented to CR_LEADING below, before we get back to line number computation.
			Array search, and thus pause, will be skipped if this computation yields a remainder. */
	}
		
	const BitMap *ftpPortBits = GetPortBitMapForCopyBits(fullTextPort);
	const BitMap *dlogPortBits = GetPortBitMapForCopyBits(GetDialogWindowPort(dlog));
	CopyBits(ftpPortBits, dlogPortBits, &textSection, &creditRect, srcCopy, NULL);

//	CopyBits(&fullTextPort->portBits, &dlog->portBits,
//					&textSection, &creditRect, srcCopy, NULL);
					
	textSection.top++; textSection.bottom++;
	pixelCount++;

	lastTime = thisTime;
}
