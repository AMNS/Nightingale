/********************************************************************************
 *	Consolation.c																*
 *																				*
 *	Version 1.5				April 21, 1995										*
 *																				*
 *	A package of routines for operating a Macintosh Console Window within		*
 *	another program.  This code is written in C using the MW CodeWarrior		*
 *	Development System.  It does not depend on any other libraries for its		*
 *	operation, with the possible exception of the SANE numerics package,		*
 *	which the Macintosh loads automatically when any routine in it is called.	*
 *																				*
 *	Designed and written by Douglas McKenna.  Most of this code is the same		*
 *	as the old Current Events library, updated for System 7 Apple event stuff.	*
 *																				*
 *	© (Copyright) 1986-1995														*
 *	Mathemaesthetics, Inc.														*
 *	PO Box 298 • Boulder • CO • 80306											*
 *	All rights reserved															*
 *																				*
 *******************************************************************************/

#define USE_STD_ARG	/* JGG attempt to fix DebugPrintf on PPC */
#ifdef USE_STD_ARG
#include <stdarg.h>
#endif

// Define various Mac Toolbox types, structs, and constants.

#ifdef TC5
	#include <OSEvents.h>
	#include <Windows.h>
	#include <StandardFile.h>
	#include <AppleEvents.h>
	#include <Fonts.h>
	#include <LowMem.h>
	#include <SegLoad.h>
	#include <ToolUtils.h>
	#include <Desk.h>
#endif


// Tell "Consolation.h" that it is being included
// for the library build, not client use.

#define _CompilingConsolationLibraryOnly_

#include "Consolation.h"

// Ward off unused argument warnings from the compiler
#undef UnusedArg
#define UnusedArg(x)		x = x;


#define kCodeWarrior68KCreator	'MMCC'
#define kCodeWarriorPPCCreator	'MPCC'
#define kSymantecCreator		'KAHL'
#define kAppleMPWCreator		'AMPW'

#define kSourceFileCreator	kCodeWarrior68KCreator

#define NoAPPLE_EVENTS
#define NoHEAP_DISPLAY

#define MARGIN	3			/* Width in pixels of left margin in Debug window */
#define MAXMSG	256			/* Buffer size */
#define MAXROWS	256			/* Larger than imaginable */

#define MAXMASK		20		/* Level, pause, user, etc. and event bits on screen */
#define BUMPEROFF	0
#define LEVELOFF	1		/* Index of Level box in bitRect */
#define PAUSEOFF	2		/* of Pause box */
#define USERFUNCOFF 3		/* of UserFunc box */
#define LOGFILEOFF	4		/* of LogFile box */
#define HELPOFF		5		/* of Help box */
#define SHOWOFF		6		/* of Show Update Region box */
#define MASKOFF 	6		/* Offset to first mask bit rectangle */

#ifndef NIL
#define NIL			((void *)0)			/* The null pointer */
#endif

#ifndef FRONT_WINDOW
	#define FRONT_WINDOW	((WindowPtr)-1)		/* Code for making frontmost window */
#endif

#define inCorner	7417				/* Pseudo part code */
#define zoomer		8					/* Constant to add for a zoom box */

#ifndef TRUE
	#define TRUE		1
	#define FALSE		0
#endif

#define SCROLLBAR_NOT

#ifdef SCROLLBAR
	#define SCROLLBARWIDTH	15
#endif

#define times10(n)	( ((n)<<3) + ((n)<<1) )
#define isDigit(n)	( (n)>='0' && (n)<='9' )
#define plural(n)	( ((n)==1) ? "" : "s" )

#define WaitNextEventTrap	0xA860
#define UnimplementedTrap	0xA89F

#define CONSOLATION_LOGGING

/*
 *	Globals are declared static so as to be invisible to the outside world,
 *	such as the application using this library.
 */

// MAS no globals!
//extern Ptr MacJmp : 0x0120;			/* Debugger jump vector */
//

static const unsigned char *version = "\pConsolation™ 1.5";
static const unsigned char *notice = "\p© 1986-1995 Mathemaesthetics, Inc.  All rights reserved.";

static WindowRecord	sayRecord;			/* Where to store Debug window info */
static WindowPtr	sayWindow;			/* On-screen window */
static WindowPtr	oldWindow;			/* Active window prior to active Debug */
static WindowPtr	foundWindow;		/* Window a mouse click was found in */
#ifdef CARBON_NOTYET
static GrafPort	sayPort;			/* Off-screen port */
#endif
static GrafPtr		offScreen;		/* Off-screen port */
static GrafPtr		sayBoth[2];		/* Off-screen and on-screen port */
static FontInfo		font;				/* Font that messages are printed in */
static EventRecord	postEvent;			/* User-posted event */
static EventRecord	theEvent;			/* Latest event to pass through */
static Handle		offscreenBits;		/* Handle for benefit of heap dumper */

static short	smallMemory,		/* If off-screen port is on-screen */
					quickDebug,			/* Force smallMemory */
					indent,				/* Whether to indent marker width */
					internalComment,	/* Is next print call internal one */
					lastOutside,		/* Was last output call from outside */
					userEvent,			/* Should we deliver user-posted event */
					singleStep,			/* Is user single-stepping messages */
					hidden,				/* Has window been hidden ? */
					helping,			/* TRUE when help is outputing */
					showing,			/* TRUE when showing update regions */
					eraseUpdates,		/* TRUE when black update rgn to be erased */
					userFuncing,		/* TRUE when user function called */	
					logFile,			/* Whether logging output or not */
					pepsi,				/* If pausing for refreshes */
					notified,			/* If IAZNotify called */
			 							/* PrtLong flags (to shorten arg lists) : */
			 		hasMultifinder,		/* WaitNextEvent is implemented() */
			 		appleEventsOK,		/* TRUE when Apple event routines are available */
			 		system7orMore,		/* TRUE when System 7 or above */
			 		balloonHelpOK,		/* TRUE when Balloons are here */
			 		inBackground,		/* We've been juggled out */
			 		wasPaused,			/* Old pause setting while juggled out */
			 		noSign,				/* Unsigned number */
			 		negArg,				/* Number is negative */
			 		upperHex,			/* Do upper case hex */
			 		doZero,				/* Fill with zeros */
			 		doSign,				/* Force a sign even if positive */
			 		doSpace,			/* Fill with spaces */
					leftJustify,		/* Print fields on left rather than right */
			 		doAlternate,		/* Use alternate format for number */
					level = 1,			/* Current Debug output level */
					saveLevel = 1,		/* What to toggle between with level */
					markerLength,		/* In columns, of marker string */
					pauseCount,			/* Rows output since last pause */
					WIDTH,				/* Width in pixels of screen */
					HEIGHT,				/* Height in pixels of screen */
					rowLength[MAXROWS],	/* Individual high water marks */
					rlMax = -1,			/* Current size of rowLength */
					rlNext,				/* Next open slot in array */
					nRows, nCols,		/* Current screen size */
					zoomRows,			/* Number of rows before zooming out */
					nRows1,				/* 1 less than nRows, precomputed */
					row, col,			/* Current screen cursor position */
					lineHeight,			/* In pixels, of Debug font */
					promptLength = 7,	/* Length of pause prompt */
					fieldLength,		/* Length of latest formated field */
					fileRefNum,			/* Of current log file */
					startPort,			/* Index of first port to draw into */
					foundPart;			/* Part a mouse click was found in */

static unsigned long		debugMask;	/* Which events to report on */

typedef pascal OSErr (*EventHandlerProc)(AppleEvent *appleEvent, AppleEvent *reply, long refcon);
static EventHandlerProc		OldHandler;

static long			theHandlerRefcon;	/* Returned by AEGetEventHandler */
static Boolean		theHandlerHeap;		/* FALSE for application; TRUE for system */
static ResType		theHandlerClass,	/* Event class and ID for our substitute handler */
					theHandlerID;
#ifdef SCROLLBAR
static ControlHandle	scrollBar;		/* Vertical scroll bar */
#endif

static char	msg[MAXMSG],		/* Where to build a Debug message */
				buffer[MAXMSG],
				*bufPtr = buffer,				/* Next char in buffer */
				*bufEnd = buffer+MAXMSG-1,		/* Last char in buffer */
				marker[32],						/* Current marker string */
				*pausePrompt = (char *)"\p…more…",
				*firstPausePrompt = (char *)"\p (tap Return or Space bar, or click mouse)";
static int	firstPause = TRUE;

static Str255	title,
				outputFile;			/* Log file name (Pascal str) */

static Handle	theText;				/* Where to store formatted text */
static long	theTextSize,			/* Size of text handle */
				theTextLogSize,		/* Actual size of real text inside handle */
				theTextIncr,			/* Needed to expand theText quickly */
				theFirstChar;			/* Where start of visible text is in handle */

static Rect	startRect = { 240, 40, 480, 420 },	/* Default Debug position */
				moreRect,				/* For highlighting pause */
				bigMoreRect,
				bottomRow,				/* Entire last line of window */
				maskRect,				/* Where mask bit rects are located */
				controlRect,			/* Where bit rects below are located */
				textRect,				/* Where text lines are located */
				growRect,				/* Where the grow icon is */
				scrollRect,				/* Where any scroll bar is on right */
				eraseRect,				/* Erase area to right of mask bits */
				corner[4],				/* 4 corners of screen, plus slop */
				grow1, grow2,			/* Actual grow icon rectangles */
				bitRect[MAXMASK+1];		/* Rectangles for settings in window */

static DebugUserFunc userFunc;		/* User callable routine */
static DebugPrintFunc userPrintf;	/* User-specified printing routine */

// MAS: no globals!
//extern long IAZNotify : 0x33C;		/* Global to restore if goodbyeFunc called */
static long oldGoodBye;				/* Place to keep old goodbye */
static long lastScroll;				/* Time of last scroll */

typedef void (*PaintFunc)(int how);

static void	PaintPause(int how);
static void	PaintLogFile(int how);
static void	PaintHelp(int how);
static void	PaintShowing(int how);
static void	PaintMaskBit(int i);
static void	PaintErase(int how);
static void	PaintUserFunc(int how);
				
static void	PaintMore(int how, GrafPtr oldport);
static void	PaintDebugControls(void);
static void	PaintDebugMask(void);
static void	PaintLevel(void);
static void	PenWhite(void);
static void	PenGray(void);
static void	PenBlack(void);
#ifdef POST
static void	DoDebugPost(Point p, GrafPtr oldport);
#endif
static void	DoDebugDrag(EventRecord *evt);
static void	DebugComment(char *msg);
static void	DebugFlushMsg(void);
static void	DebugString(char *str);
static void	DebugChar(char c);
static void	DebugMove(int row, int col);
static void	DrawDebugLine(int n);
static void	DrawDebugWindow(GrafPtr oldport, int all);
static void	DebugKeyMsg(EventRecord *evt);
static void	DebugMouseMsg(EventRecord *evt, WindowPtr w, int part);
static void	DealWithDebug(EventRecord *evt);
static void	CheckUserPause( void);
static void	DoDebugAppleMenu(Point p);
static void	DealWithMask(Point p, GrafPtr port, int modifiers);
static void	DWMReally(Point p, int how, int modifiers);
static void	EQToFront(QElemPtr evt);
static void	GetRowsCols(Rect *r);
static void	DebugWaitClick( void);
static void	SetRowLength(short col);
static void	SetAllRowLengths(short n);
static void	FillUpdateRgn(WindowPtr w);
static void	PrintFreeMem(void);
		
static int	DoDebugComment(EventRecord *evt);
static int	GetNewDebugSize(void);
static int	ScrollDebugWindow(int i, int how);
static int	CheckPause(void);
static int	DoDebugPause(void);
static int	TrackClickBox(Rect *box, PaintFunc painter);
static int	DoDebugErase(void);
static int	GetDebugFile(void);
static int	DoDebugGrow(Point p, int how);
static int	DoGoAway(WindowPtr w, Point p, int modbits);
static int	DoDebugZoom(WindowPtr w, int i, int how);
static int	GetPauseEvent(short mask, EventRecord *evt, long now);
		
static void	OurNewLine(void);
static void	IndentLevel(int level);
static int	IsPrintable(Byte ch);

static Rect	*GetARect(WindowPtr *w);
static void	LocalToGlobalRect(Rect *r);
static char	*SayList(Point *pt, int n, int negOK);
static char	*PrtLong(unsigned long arg, int base, int width);
static char	*PrtDouble(double *arg, int width, int precision, int style);
static char	*OurPascalToC(Byte *str);
static Byte	*OurCToPascal(char *str);
static char	*StrCpy(char *dst, char *src);
static char	*OurPstrcpy(char *dst, char *src);
static char	*OurPstrapp(char *dst, char *src);
		
static long	OurStrLen(char *str);
static long	CrushLF(char *str);
static short MaxRowLength(void);
static void	PushAppleEventHandler(EventRecord *event);
static pascal OSErr 	DebugReportAppleEvent(AppleEvent *appleEvent, AppleEvent *reply, long refcon);
static void	PrintDescriptor(AEDesc *theList, int doType, long index);

static void	DebugHeapDump(void);

static EvQElPtr EQScan(int mask, long now);

static OSErr	OpenDebugFile(void);

static pascal void AutoClose(void);

/*
 *	Functions that indicate whether modifier keys are down or not right now
 */

#ifdef CONSOLATION_ALREADYDEFD
extern long Keymap : 0x174L;
int CmdKeyDown(void);
int OptionKeyDown(void);
int ShiftKeyDown(void);

static int CmdKeyDown() {
		return(BitTst(&Keymap+1, 16) != 0);
	}

static int OptionKeyDown() {
		return(BitTst(&Keymap+1, 29) != 0);
	}

static int ShiftKeyDown() {
		return(BitTst(&Keymap+1, 31) != 0);
	}
#endif //CONSOLATION_ALREADYDEFD


/**************************************************************************
 *
 *	These are general utility routines, which we implement here so the
 *	user doesn't need to load libraries from elsewhere in order to use
 *	the Debug window.
 *
 *************************************************************************/

/*
 *	If the application program wishes to load the segment containing
 *	Consolation at a known time, it can call this function, which
 *	does nothing.  The act of calling it through the jump table will
 *	load the segment.
 */

void LoadConsolation()
	{
	}

/*
 *	Something to slow things down with.
 *	Sleep for n seconds, or until the mouse is clicked.
 */

void DebugSleep(short /*n*/)
	{
#ifdef CARBON_NOTYET		
		long now,soon; EventRecord evt;
		now = TickCount();
		soon = now + 60*n;
		while (TickCount() < soon) {
			if (GetPauseEvent(mDownMask+mUpMask,&evt,now)) 
				if (evt.what == mouseUp) break;
			}
#endif
	}

void DebugWaitClick()
	{
		DebugSleep(32000);
	}

/*
 *	Convert, in place, a Pascal string, where the first byte is the
 *	length, to a null-terminated C string, and deliver its argument.
 */
 
char *OurPascalToC(Byte *str)
	{
		register unsigned int len;
		register unsigned char *p,*q;
		
		len = *(p=q=(unsigned char *)str);
		while (len--) *p++ = *++q;
		*p = '\0';
		return((char *)str);
	}

/*
 *	And convert in place a C string to Pascal string.  C string should not be
 *	longer than 255 characters.
 */

Byte *OurCToPascal(char *str)
	{
		register unsigned char *p,*q;
		int len;
		
		p = (unsigned char *)str + (len = OurStrLen(str));
		q = p-1;
		while (p != (unsigned char *)str) *p-- = *q--;
		*str = len;
		return((Byte *)str);
	}

/*
 *	This delivers the length of a null-terminated C string, in bytes.
 */

long OurStrLen(register char *str)
	{
		register char *p;
		
		p = str;
		while (*p++) ;
		return ((long)(--p - str));
	}

/*
 *	This delivers the length of a null-terminated C string, in bytes.
 *	It also converts all LF's to CR's in the string.
 */

long CrushLF(register char *str)
	{
		register char *p;
		
		p = str;
		while (*p) if (*p == '\n') *p++ = '\r'; else p++;
		return ((long)(p - str));
	}

/*
 *	Copy the bytes from the null-terminated C string, src, into
 *	the string at dst, and deliver its first argument.
 *	Room at dst is assumed available.
 */

char *StrCpy(register char *dst, register char *src)
	{
		register char *d;
		
		d = dst;
		while ( (*dst++ = *src++) != '\0' ) ;
		return(d);
	}

/*
 *	Copy Pascal string from src to dst; deliver dst.
 */

char *OurPstrcpy(char *dst, char *src)
	{
		char *old = dst;
		register int n;
		
		n = *(unsigned char *)src;
		while (n-- >= 0) *dst++ = *src++;
		return(old);
	}

static void OurNewLine()
	{
		DebugPrintf("\r");
	}

/*
 *	Finish off the string being formated in buffer by DebugPrintf,
 *	and send it on its merry way to be printed in the window.
 */

static void DebugFlushMsg()
	{
		if (sayWindow == NIL) InitDebugWindow(&startRect,TRUE);

#ifdef ONHOLD
		if (system7orMore) {
			if (oldPreferred = GetOutlinePreferred())
				SetOutlinePreferred(FALSE);
			}
#endif
		*bufPtr = '\0';
		if (internalComment) DebugComment(buffer);
		 else				 DebugString(buffer);
		bufPtr = buffer;

#ifdef ONHOLD
		if (system7orMore)
			if (oldPreferred)
				SetOutlinePreferred(TRUE);
#endif
	}

/*
 *	Add another character to the buffer.  If the buffer is full, flush it
 *	first.  bufEnd must take into account the need for one more char: EOS.
 */

static void DebugChar(register char ch)
	{
		if (bufPtr == bufEnd) DebugFlushMsg();
		*bufPtr++ = ch;
	}
		
/*
 *	This is a version of printf() with and without some fancy formatting
 *	features.  It ought to be expanded to work synonymously with printf().
 *	It takes a variable number of arguments, interpreted according to
 *	the format escape sequences embedded in the format string.  These have
 *	the syntax:
 *
 *		% [modifier(s)] [width] [.precision] [argSize] commandChar
 *
 *	Optional modifiers are any of:
 *
 *			-		left justify within width, pad on right
 *			+		always print leading sign (+ or -)
 *			0		use 0 as pad character (usually space)
 *			space	always print either a minus sign or space for sign
 *			#		use alternate form of conversion
 *			*		get field width from next integer argument
 *
 *	The width is a decimal string of digits.
 *
 *	The precision is a decimal point followed by a (possibly null) decimal
 *	string of digits.
 *
 *	The argSize is one of:
 *
 *			h		argument is a halfword (short)
 *			l		argument is a long word
 *
 *	The commandChar is any of:
 *
 *			b	print an unsigned integer argument, base 2 (binary)
 *			c	print a character
 *			d	print a signed integer argument, base 10 (decimal)
 *			e	print a float/double, exponential format
 *			f	print a float/double
 *			g	print a float/double, general format
 *			o	print an unsigned integer argument, base 8 (octal)
 *			p	print a length-counted Pascal string pointed at by argument
 *			s	print a null-terminated C string pointed at by argument
 *			u	print an unsigned integer argument, base 10
 *			x	print an unsigned integer argument, base 16 (lower case hex)
 *			z	sleep for width seconds
 *			B	print a Boolean
 *			C	print a color triplet: (R,G,B)
 *			P	print the coordinates of a point: (x,y)
 *			R	print the coordinates of a rectangle: (xl,yt,xr,yb)
 *			T	print the 4-character resource or operating system type
 *			W	print the title of the window with given window pointer
 *			X	print an unsigned integer argument, base 16 (upper case hex)
 *			%	print a %
 *			@	Use predefined function
 *
 *	Note that the %b, %B, %p, %W, %P, %R, %T, %z, and %@ commands are not
 *	compatible with the standard Unix printf().
 */	
#if 0

static FILE *fLog = NULL;

static FILE *getLogFile()
{
	if (fLog == NULL)
		fLog = fopen("/Users/crose/Library/Logs/ngale.log", "a");
		
	return fLog;
}

void DebugPrintf(char *msg, ...)
	{
		va_list nxtArg;
		
		va_start(nxtArg, msg);
		
		FILE *f;		
		f = getLogFile();
		if (f != NULL) {
			vfprintf(f, msg, nxtArg);
		}
		else {
			vprintf(msg, nxtArg);
		}
		va_end(nxtArg);
		
		if (OptionKeyDown()) {
			printf("flushing stdout\n");
			fflush(stdout);
		}

	}
#endif

#if 1
void DebugPrintf(char *msg, ...)
	{
		va_list nxtArg;
		
		va_start(nxtArg, msg);
		
			vprintf(msg, nxtArg);

		va_end(nxtArg);
	}		
#else
void DebugPrintf(char *msg, ...)
	{
		int base,width,n,precision; long arg; double darg; OSType type;
		unsigned long argu;
		char *nextArg, str[16]; RGBColor *triplet;
		register char *p; WindowPtr w; Rect *r; Point *pt;
		int wideArg,more,pascalStr,doPrecision;
		
		return;
		
#ifdef USE_STD_ARG
		va_list nxtArg;
		
		va_start(nxtArg, msg);
#else
		nextArg = (char *)(&msg) + sizeof(msg);		/* Second argument */
#endif
		
		while (*msg)
		
			if (*msg != '%') DebugChar(*msg++);
			 else {
			 	
			 	leftJustify = doSign = doSpace = doZero = doAlternate =
			 				  doPrecision = FALSE;
			 	more = TRUE; width = -1; precision = 0;
			 	
			 	 /* First pick off the possible modifiers */
			 	
			 	while (more)
			 		switch(*++msg) {
			 			case ' ':
			 					doSpace = !doSign; break;
			 			case '-':
			 					leftJustify = TRUE; doZero = FALSE; break;
			 			case '+':
			 					doSign = TRUE; doSpace = FALSE; break;
			 			case '0':
			 					doZero = !leftJustify; break;
			 			case '#':
			 					doAlternate = TRUE; break;
			 			case '*':
#ifdef USE_STD_ARG
								width = va_arg(nxtArg, int);
#else
			 					width = *(int *)nextArg;
			 					nextArg += sizeof(int);
#endif
			 					if (width < 0) width = 0;
			 					break;
			 			default:
			 					more = FALSE;
			 					break;
			 			}
			 	
			 	 /* Now get the field width, if not already read */
			 	
			 	if (width < 0) {
			 		width = 0;
			 		while (isDigit(*msg))
			 			width = times10(width) + (*msg++ - '0');
			 		}
			 	
			 	 /* Get the precision, if a period is next */
			 	
			 	doPrecision = (*msg == '.');
			 	if (doPrecision)
			 		if (*++msg == '*') {
#ifdef USE_STD_ARG
			 			precision = va_arg(nxtArg, int);
#else
			 			precision = *(int *)nextArg; nextArg += sizeof(int);
#endif
			 			msg++;
			 			}
			 		 else 
			 			while (isDigit(*msg))
			 				precision = times10(precision) + (*msg++ - '0');
			 		
			 	 /* Note whether this is a long argument: ignore short */
			 	
			 	if (*msg == 'h') msg++;
			 	wideArg = (*msg == 'l');
			 	if (wideArg) msg++;
			 	
			 	negArg = pascalStr = FALSE; noSign = TRUE; fieldLength = 0;

			 	switch(*msg) {
			 		case 'b':
			 			base = 2; goto outInt;
			 		case 'o':
			 			base = 8; goto outInt;
			 		case 'x':
			 			base = 16; upperHex = FALSE; goto outInt;
			 		case 'X':
			 			base = 16; upperHex = TRUE; goto outInt;
			 		case 'u':
			 			base = 10; goto outInt;
			 		case 'd':
			 			base = 10; noSign = FALSE;
			 outInt:	
			 			if (noSign)
			 				if (wideArg) {
#ifdef USE_STD_ARG
			 					argu = va_arg(nxtArg, unsigned long);
#else
			 					argu = *(unsigned long *)nextArg;
			 					nextArg += sizeof(long);
#endif
			 					}
			 				 else {
#ifdef USE_STD_ARG
			 					argu = va_arg(nxtArg, unsigned int);
#else
			 					argu = *(unsigned int *)nextArg;
			 					nextArg += sizeof(int);
#endif
			 					}
			 			 else {
			 				if (wideArg) {
#ifdef USE_STD_ARG
			 					arg = va_arg(nxtArg, long);
#else
			 					arg = *(long *)nextArg;
			 					nextArg += sizeof(long);
#endif
			 					}
			 				 else {
#ifdef USE_STD_ARG
			 					arg = va_arg(nxtArg, short);
#else
			 					arg = *(short *)nextArg;
			 					nextArg += sizeof(short);
#endif
			 					}
			 				argu = (unsigned long)arg;
			 				negArg = (arg < 0);
			 				if (negArg) argu = -argu;
			 				}
			 			p = PrtLong(argu,base,width);
			 			break;
			 		case 'W':
#ifdef USE_STD_ARG
			 			w = va_arg(nxtArg, WindowPtr);
#else
			 			w = *(WindowPtr *)nextArg; nextArg += sizeof(WindowPtr);
#endif
						GetWTitle(w,title); p = (char *)title;
						fieldLength = (unsigned)(*p);
						if (doPrecision) fieldLength = precision;
						pascalStr = TRUE;
			 			break;
			 		case 'T':
#ifdef USE_STD_ARG
			 			type = va_arg(nxtArg, OSType);
#else
			 			type = *(OSType *)nextArg; nextArg += sizeof(OSType);
#endif
			 			fieldLength = 4;
			 			if (doPrecision) fieldLength = precision;
			 			*(OSType *)str = type;	/* Assumes str is on even byte */
			 			str[4] = '\0';
			 			p = str;
			 			break;
			 		case 'B':
#ifdef USE_STD_ARG
			 			arg = va_arg(nxtArg, short);
#else
			 			arg = *(short *)nextArg; nextArg += sizeof(short);	/* Booleans < int */
#endif
			 			if (doAlternate) p = (char *)(arg ? "\pYES" : "\pNO");
			 			 else			 p = (char *)(arg ? "\pTRUE" : "\pFALSE");
			 			fieldLength = (doPrecision) ? precision : *p;
			 			pascalStr = TRUE;
			 			break;
			 		case 'p':
#ifdef USE_STD_ARG
			 			p = va_arg(nxtArg, char *);
#else
			 			p = *(char **)nextArg; nextArg += sizeof(char *);
#endif
			 			fieldLength = (unsigned)(*p);
			 			if (doPrecision) fieldLength = precision;
			 			pascalStr = TRUE;
			 			break;
			 		case 's':
#ifdef USE_STD_ARG
			 			p = va_arg(nxtArg, char *);
#else
			 			p = *(char **)nextArg; nextArg += sizeof(char *);
#endif
			 			fieldLength = OurStrLen(p);
			 			if (doPrecision) fieldLength = precision;
			 			break;
			 		case 'R':
#ifdef USE_STD_ARG
			 			r = va_arg(nxtArg, Rect *);
#else
			 			r = *(Rect **)nextArg; nextArg += sizeof(Rect *);
#endif
			 			p = SayList((Point *)r,4,TRUE);
			 			break;
			 		case 'P':
#ifdef USE_STD_ARG
			 			pt = va_arg(nxtArg, Point *);
#else
			 			pt = *(Point **)nextArg; nextArg += sizeof(Point *);
#endif
			 			p = SayList(pt,2,TRUE);
			 			break;
			 		case 'c':
#ifdef USE_STD_ARG
			 			p = str; p[0] = (char) va_arg(nxtArg, int);
#else
			 			p = str; p[0] = (char) (*(int *)nextArg);
			 			nextArg += sizeof(int); /* chars passed as ints */
#endif
			 			fieldLength = 1;
			 			break;
			 		case 'C':
#ifdef USE_STD_ARG
			 			triplet = va_arg(nxtArg, RGBColor *);
#else
			 			triplet = *(RGBColor **)nextArg; nextArg += sizeof(RGBColor *);
#endif
			 			p = SayList((Point *)triplet,3,FALSE);
			 			break;
			 		case '%':
			 			p = "%"; fieldLength = 1;
			 			break;
			 		case 'g':
			 		case 'G':
			 		case 'e':
			 		case 'E':
			 			/* Above are not implemented, so we map them to 'f's */
			 		case 'f':
#ifdef USE_STD_ARG
			 			darg = va_arg(nxtArg, double);
#else
			 			darg = *(double *)nextArg; nextArg += sizeof(double);
#endif
			 			if (!doPrecision) precision = 6;
			 			p = PrtDouble(&darg,width,precision,'f'/* *msg */);
			 			break;
			 		case 'z':
			 			DebugFlushMsg();
			 			if (width == 0) DebugWaitClick();
			 			 else			DebugSleep(width);
			 			p = ""; width = fieldLength = 0;
			 			break;
			 		case '@':
			 			DebugFlushMsg();
#ifdef USE_STD_ARG
		 			 	if (userPrintf) {
		 			 		void *foo;
		 			 		foo = va_arg(nxtArg, void *);
		 			 		if (width <= 0) width = -1;
							(*userPrintf)(foo,width,precision,doAlternate);
							p = ""; fieldLength = 0;
							}
						 else
							{ p = "<%@:NoPrintFunc>"; fieldLength = 16; }
#else
		 			 	if (userPrintf) {
		 			 		if (width <= 0) width = -1;
							(*userPrintf)(*(void **)nextArg,width,
														precision,doAlternate);
							p = ""; fieldLength = 0;
							}
						 else
							{ p = "<%@:NoPrintFunc>"; fieldLength = 16; }
						nextArg += sizeof(void *);
#endif
			 			width = 0;
			 			break;
			 		default:
			 			p = "<% :BadCmdChar>"; p[2] = *msg; width = 0;
			 			fieldLength = 12;
			 			break;
			 		}
			 	
			 	if (p == NIL)
			 		{ p = "<NIL>"; width = 0; fieldLength = n = 5; }
			 	 else
			 	 	if (pascalStr) fieldLength = n = (unsigned)(*p++);
			 	 	 else		   n = fieldLength;
			 	
		 		if (width!=0 && width>fieldLength) {
		 			if (!leftJustify) while (n++ < width) DebugChar(' ');
		 			if (pascalStr) while (fieldLength-- > 0) DebugChar(*p++);
		 			 else		   while (*p)				 DebugChar(*p++);
		 			if (leftJustify) while (n++ < width) DebugChar(' ');
		 			}
		 		 else
		 			while (fieldLength-- > 0) DebugChar(*p++);

			 	msg++;
				}
		DebugFlushMsg();
#ifdef USE_STD_ARG
		va_end(nxtArg);
#endif
	}
#endif

/*
 *	Convert a long integer to a string and deliver pointer to string.
 *	Get digits in reverse order, and then reverse them so as not to
 *	have the overhead of a lot of recursive calls.
 */

char *PrtLong(register unsigned long arg, int base, int width)
	{
		register unsigned char *dst,*start,*dig;
		register unsigned char tmp;
		static unsigned char *digit = (unsigned char *)"0123456789abcdef";
		static unsigned char *DIGIT = (unsigned char *)"0123456789ABCDEF";
		static unsigned char *bits =  (unsigned char *)".•";
		static unsigned char number[MAXMSG];
		
		dst = number; dig = upperHex ? DIGIT : digit;
		if (doAlternate && base==2) dig = bits;
		
		 /* Get string of digits in reverse order */
		 
		start = dst;
		if (arg == 0L)
			*dst++ = dig[0];
		 else {
		 	if (doSign) {
		 		if (negArg) *dst++ = '-'; else *dst++ = '+';
		 		start = dst;
		 		}
		 	 else
		 		if (!noSign) if (negArg) { *dst++ = '-'; start = dst; }
		 	if (doAlternate) {
		 		if (base==8 | base==16) *dst++ = '0';
		 		if (base == 16)
		 		 	*dst++ = upperHex ? 'X' : 'x';
		 		start = dst;
		 		}
			while (arg) { *dst++ = dig[arg % base]; arg /= base; }
			}
		
		fieldLength = dst - number;
		
		if (doZero && width>0) {
			if (width >= MAXMSG) width = MAXMSG-1;
			while (fieldLength < width) { *dst++ = dig[0]; fieldLength++; }
			}
		*dst-- = '\0';
		
		 /* Now reverse digits, and return pointer to string of digits */
		 
		while (dst > start) { tmp = *dst; *dst-- = *start; *start++ = tmp; }
		return((char *)number);
	}

/*
 *	Deliver pointer to a string of printed double, using given width and
 *	precision, and type of printing as specified by the escape command char.
 *	Currently, all format characters are treated as '%f's, in order to avoid
 *	using the Dec2Str() call (which would make Consolation depend on another
 *	library).  We use the SANE package to do a partial conversion into the
 *	Decimal data structure, and then we take it from there.
 */

static char *PrtDouble(double *arg, int width, int precision, int style)
	{
#ifdef ONHOLD
		static char buffer[256]; int n;
		char dbuffer[256]; register char *dst,*src;
		DecForm dform; Decimal dcmal;
		
		n = precision;
		if (style=='e' | style=='E') n++;
		 else if (style=='g' | style=='G') if (n < 1) n = 1;
		 
		dform.style = (style=='f');		/* Convert binary to decimal record */
		dform.digits = n;
		fp68k(&dform,arg,&dcmal,FFEXT+FOB2D);	/* Use 80 bit extended */

		/*
		 *	The Decimal structure contains a flag in the high order byte of
		 *	the sgn field for the sign of the result; the negative of the
		 *	the number of digits to the left of the decimal point; and a Pascal
		 *	string of the exact digits in the result, broken into a length 
		 *	field and a string data field.
		 *	We format the result string in dbuffer, according to the style
		 *	requested (so far, only %f is implemented).
		 */
		
		dst = dbuffer; src = (char *)dcmal.sig.text;
		
		if ((doSpace|doSign) && *arg>=0) *dst++ = doSign ? '+' : ' ';
		 /* Deal with 0.0 separately for now, since dcmal.exponent is wierdly large */
		if (*arg == 0.0) {
			*dst++ = '0';
			if (precision>0 || doAlternate) {
				*dst++ = '.';
				for (n=0; n<precision; n++) *dst++ = '0';
				}
			}
		 else {
			if (dcmal.sgn & 0x100) *dst++ = '-';
			n = dcmal.sig.length + dcmal.exp;
			if (n <= 0) {
				*dst++ = '0';
				if (precision>0 || doAlternate) {
					*dst++ = '.';
					while (n++ < 0) *dst++ = '0';
					n = dcmal.sig.length;
					while (n-- > 0) *dst++ = *src++;
					}
				}
			 else {
				while (n-- > 0) *dst++ = *src++;
				if (precision>0 || doAlternate) {
					*dst++ = '.';
					n = dcmal.exp;
					while (n++ < 0) *dst++ = *src++;
					}
				}
			}
		fieldLength = dst - dbuffer;
		*dst = '\0';
			 
		/*
		 *	At this point, dbuffer is a C string representing the double.
		 *	Now we make a copy of it in buffer, and
		 *	muck around with the format as we do it, according to the
		 *	style formating character and current flags.
		 */
		
		src = dbuffer;
		dst = buffer;
		
		switch(style) {
			case 'f':
				if (*src=='-' || *src==' ' || *src=='+') *dst++ = *src++;
				if (!leftJustify && (doZero|doSpace)) {
					if (width >= MAXMSG) width = MAXMSG-1;
					while (fieldLength < width)
						{ *dst++ = doZero ? '0' : ' '; fieldLength++; }
					}
				while (*src) *dst++ = *src++;
				break;
			}
		fieldLength = dst - buffer;
		*dst = '\0';
		return(buffer);
#else
		arg = arg;
		width = width;
		precision = precision;
		style = style;
		return("");
#endif
	}

#ifdef NOTYET
			case 'e':
			case 'E':
				if ((doSpace|doSign) && *arg>=0) *q++ = doSign ? '+' : ' ';
				if (!doSpace && *p==' ') p++;
				while(*p) {
					if (*p == 'e') {
						*p = style;		/* In case style is 'E' */
						if (doAlternate && precision==0) *q++ = '.';
						}
					*q++ = *p++;
					}
				*q = '\0';
 				break;
			case 'g':
			case 'G':
				if ((doSpace|doSign) && *arg>=0) *q++ = doSign ? '+' : ' ';
				if (!doSpace && *p==' ') p++;
				while(*p) {
					if (*p == 'e') {
						*p = style;		/* In case style is 'E' */
						if (doAlternate && precision==0) *q++ = '.';
						}
					*q++ = *p++;
					}
				*q = '\0';
 				break;
				if (doSign && *arg>=0) *q++ = '+';
				while (*p++ != 'e') ;
				negExp = (*p++ == '-');
				q = p; n = 0;
				while (
				n = _std_decode(&tbufptr);
				if (neg) length *= -1;
			
				if (pound_on)  /* if # flag is on and ... */
				{
					if ((length<=precision)&&(length>=-4))
						goto regf;  /* do regular f routine */
					else
					{
						/* this gives precision-1 digits after
							. instead of precision */
							
						bufptr = (char *)floatbuffer;
						c -= 2;
						goto rege;  /* do regular e routine */
					}
				}
			
			
				if ((length<=precision)&&(length>=-4))
				{register Boolean strip_it = false;
				
					/* convert to f format */
			
					bufptr = cvtf2string(&tempdouble, 1, precision-length, floatbuffer);
			
					/* don't strip string unless therre is a . */
					while (*bufptr)
					{
						strip_it = strip_it?true:(*bufptr == '.');
						bufptr++;
					}
					bufptr -= 1; /* was 2 */
					
					/* strip trailing zeros */
					if (strip_it)
					{
						while (*bufptr == '0') bufptr--;
						if (*bufptr == '.') bufptr--;
						*(++bufptr) = '\0';
					}
					
					bufptr = (char *)floatbuffer;
				
					/* now test for \0 or -\0 and adjust */
					if ((*bufptr == '\0') ||
						((*bufptr=='-')&&(*(bufptr+1)=='\0')))
					{
						*bufptr = '0';
						*(bufptr+1) = '\0';
					}
					
					goto nregf;
					
					break;
				}
			
			
				/* back up before the e and strip trailing zeros */
				
				bufptr -= 3;
				
				while ((*bufptr == '0')||(*bufptr == '.'))
					*(bufptr--) = '@';
					
			 					/*	bufptr = (char *)floatbuffer;
			 						c -= 2;  / convert form g to e, G to E /
			  						goto rege;
			  					*/
			
				if ((space_on == false)&&(floatbuffer[0] == ' '))
					floatbuffer[0] = '@';
					
				if ((tempdouble>=0) && (sign_on)) floatbuffer[0] = '+';
			
				tbufptr = (char *)tempbuffer;
				bufptr = (char *)floatbuffer;
			
				while (*bufptr != 'e')
					if (*bufptr != '@')
						*tbufptr++ = *bufptr++;
					else
						bufptr++;
			
				*bufptr = c-2;  /* convert G to E, g to e */
				*tbufptr++ = *bufptr++;
				*tbufptr++ = *bufptr++;
				tbufptr = check_for_three(bufptr,tbufptr);
				while (*bufptr)
					*tbufptr++ = *bufptr++;
				*tbufptr = '\0';
				dopadding(tempbuffer,left_justify,zero_fill,width);
				break;		
			}
		return(buffer);
	}
#endif

/*
 *	Deliver formatted string of Point, Rectangle, or Color coordinates.
 *	If negOK is TRUE, then use signed output, else unsigned.
 */

static char *SayList(Point *pt, int n, int negOK)
	{
		static char str[80]; unsigned long z;
		register char *p,*q;
		int i,xy[4]; unsigned int uxy[4];
		
		if (pt) {
			if (negOK) {
				xy[0] = pt->h; xy[1] = pt->v;
				if (n == 4) { pt++; xy[2] = pt->h; xy[3] = pt->v; }
				 else if (n == 3) { xy[2] = ((RGBColor *)pt)->blue; }
				}
			 else {
			 	uxy[0] = pt->h; uxy[1] = pt->v;
				if (n == 4) { pt++; uxy[2] = pt->h; uxy[3] = pt->v; }
				 else if (n == 3) {
				 	uxy[2] = ((RGBColor *)pt)->blue;
				 	uxy[0] ^= uxy[1]; uxy[1] ^= uxy[0]; uxy[0] ^= uxy[1];	/* Swap Red/Green */
				 	}
				}
			doAlternate = doZero = noSign = FALSE;
			p = str;
			for (i=0; i<n; i++) {
				if (negOK) {
					negArg = (xy[i] < 0);
					if (negArg) z = -(unsigned long)xy[i];
					 else		z =  (unsigned long)xy[i];
					}
				 else
					{ negArg = FALSE; z = uxy[i]; }
				q = PrtLong(z,10,0);
				while (*q) *p++ = *q++;
				if (i != n-1) *p++ = ',';
				}
			*p = '\0'; fieldLength = p - str;
			return(str);
			}
		return((char *)NIL);
	}
		
/*******************************************************************************
 *
 *	These routines let the application program configure the Debug window
 *	in various ways, such as marking internal sayings, setting the level
 *	of detail to be told, allowing or disallowing certain types of tales,
 *	and so on.  They need not be called explicitly, since there are
 *	standard default values.
 *
 ******************************************************************************/

short SetDebugLevel(short newLevel)		/* Set the Debug output message level */
	{
		short oldLevel;
		
		oldLevel = level; level = newLevel;
		if (level < kConsoleNoMessages) level = kConsoleNoMessages;	/* Keep it legal */
		if (level > kConsoleAllEvents)  level = kConsoleAllEvents;
		if (hidden) saveLevel = level;
		PaintDebugControls();
		return(oldLevel);
	}

short SetDebugPause(short newVal)	/* Enable or disable pausing */
	{
		short oldVal;
		
		oldVal = pepsi; pepsi = newVal; pauseCount = 0;
		PaintDebugControls();
		return(oldVal);
	}

/*
	Each event message, generated by the event to be delivered, can be
	reported or not according to its associated bit in the global Debug
	event mask, debugMask.  This sets the mask and delivers the old value.
	Bit assignments are the same as in standard Mac Toolbox event masks.
	Mask bits can also be set and cleared by the user.
 */
 
unsigned long SetDebugMask(unsigned long newMask)
	{
		unsigned long oldMask;
		
		oldMask = debugMask; debugMask = newMask;
		PaintDebugControls();
		return(oldMask);
	}

/*
	SetDebugMarker(str) allows the application to mark those comments that
	are automatically generated by GetDebugEvent().  str should be a
	null-terminated string of characters to be printed at the start
	of each internally generated comment, such as "• ".
 */

void SetDebugMarker(char *str)
	{
		StrCpy(marker,str); markerLength = OurStrLen(marker);
	}

/*
 *	Declare the address of an outside routine to call everytime the user
 *	clicks on the '*' box control.  Default at startup is a routine that
 *	prints out the number of bytes in free memory.
 */

static void PrintFreeMem()		/* Default user function */
	{
		DebugPrintf("FreeMem() = %ld bytes\r",FreeMem());
	}

DebugUserFunc SetDebugUserFunc(DebugUserFunc func)
	{
		DebugUserFunc old;
		
		old = userFunc; userFunc = func;
		return(old);
	}

DebugPrintFunc SetDebugPrintFunc(DebugPrintFunc func)
	{
		DebugPrintFunc old;
		
		old = userPrintf; userPrintf = func;
		return(old);
	}


/*
 *	This will be called by the Toolbox after the application quits (that is,
 *	when the application heap is re-initialized).  It closes any open log file.
 */

static pascal void AutoClose()
	{
//#ifdef CONSOLATION_NOTYET		
		FSClose(fileRefNum);
//#endif
	// MAS : no globals
//		if (!hasMultifinder)
//			IAZNotify = (long)oldGoodBye;		/* Restore old value for others */
	}

#ifdef CONSOLATION_LOGGING
/*
 *	Declare the name of a file to open now in order to keep a copy of
 *	everything printed.  Deliver operating system error code.
 */

OSErr SetDebugFile(char *fileName)
	{
		OSErr err;
		static int opened;
		if (fileName) {
			if (opened) SetDebugFile((char*)NIL);			/* Close open file first */
			OurCToPascal(StrCpy((char *)&outputFile,fileName));
			err = OpenDebugFile();
			logFile = opened = (err==0);
			if (sayWindow) PaintLogFile(logFile);
			}
		 else 
		 	{
			if (*outputFile) err = FSClose(fileRefNum);
			*outputFile = '\0';
			opened = FALSE;
			}
		return(err);
	}
	

/*
 *	Open file with name in outputFile for logging copy of output.  Deliver err.
 */
static OSErr OpenDebugFile()
	{
		OSErr err; short vRefNum; long dirID;
		if (*outputFile) {
			//err = GetVol(NIL,&vRefNum);
			err = HGetVol(NULL, &vRefNum, &dirID);
			if (err) DebugComment("[OpenDebugFile] Can't GetVol()\r");
			 else {
			 	FSSpec fsSpec;
			 	//err = FSDelete(outputFile,vRefNum);
			 	err = FSMakeFSSpec (vRefNum, dirID, outputFile, &fsSpec);
			 	if (err == noErr) 
			 	{
			 		err = FSpDelete(&fsSpec);
			 	}
			 	else if (err == fnfErr) 
			 	{
			 		err = noErr;			 		
			 	}
				//err = Create(outputFile,vRefNum,kSourceFileCreator,'TEXT');
				if (err == noErr)  {
					ScriptCode scriptCode = smRoman;
					err = FSpCreate (&fsSpec, kSourceFileCreator, 'TEXT', scriptCode);
				}
				if (err == noErr) {
					//err = FSOpen(outputFile,vRefNum,&fileRefNum);
					err = FSpOpenDF (&fsSpec, fsRdWrPerm, &fileRefNum);
					if (err) { 
						DebugComment("[OpenDebugFile] Can't open file\r"); 					
						fileRefNum = 0;
					 }
					 else if (!hasMultifinder)
						if (!notified) {
							// MAS : no globals !
//							oldGoodBye = (long)IAZNotify;
//							IAZNotify = (long)AutoClose;
							notified = TRUE;
							}
					}
				 else
					DebugComment("[OpenDebugFile] Can't create file\r");
				}
			}
		return(err);
	}
	
#endif CONSOLATION_LOGGING

/*
 *	Invoke the standard output file dialog to ask user for name of a file.
 */

static int GetDebugFile()
	{
#ifdef CONSOLATION_NOTYET
		static Point p = { 80, 100 };
		SFReply answer;
		
		OurCToPascal(StrCpy((char *)&outputFile,"Events.log"));
		
		SFPutFile(p,"\pFile to record output in:",outputFile,NIL,&answer);
			
		if (answer.good) {
			SetVol(NIL,answer.vRefNum);
			OurPstrcpy((char *)&outputFile,(char *)answer.fName);
			return(TRUE);
			}
#endif
		*outputFile = 0;
		return(FALSE);
	}

/*
 *	Force Init routine below to NOT allocate offscreen port, for quicker scroll.
 */

void DebugNoBitmap()
	{
		quickDebug = TRUE;
	}

/*
	Initialize library and create debug window with port specified by
	rect.  Must be called internally before other routines, but other routines
	will take care of it.  Application must call it explicitly if it wants
	some other initial position on the screen for the Debug window than
	the default, startRect; or if it should be initially a hidden window.
 */

static char *preamble[] = {
	"«»«»«»«»«» Consolation™ 1.0a1 by Doug McKenna »«»«»«»«»\r",
	"This Mac Debugging Console is available for $32 from\r",
	"  Mathemaesthetics, Inc.\r",
	"  PO Box 298 • Boulder • CO • 80306-0298 • USA\r",
	"  Phone: (303) 440-0707 • Phaxx: (303) 440-0504\r",
	"This is Not Shareware • Please Do Not Give Away Copies\r",
	"«»«»«»«»» Copyright ©1995 • All Rights Reserved »»«»«»«»\r",
	(char*)NIL
	};

void InitDebugWindow(Rect *rect, int visible)
	{
		GrafPtr oldPort; static Rect r,empty;
		BitMap debugMap; int i,slop=3,err; int coke;
		SysEnvRec thisMac;
		
		if (sayWindow == NIL) {

			/* Get info we will need about this machine */
			
			err = SysEnvirons(1,&thisMac);
			if (err) thisMac.machineType = -1;
			if (thisMac.machineType >= 0) {
#ifdef ONHOLD
				hasMultifinder = NGetTrapAddress(WaitNextEventTrap,ToolTrap) !=
								  GetTrapAddress(UnimplementedTrap);
#endif
				hasMultifinder = TRUE;
				}
			
			system7orMore = appleEventsOK = balloonHelpOK = (thisMac.systemVersion >= 0x0700);
						
			GetPort(&oldPort);
			
			if (rect) r = *rect; else r = startRect;
			if (r.right < r.left+50 || r.bottom < r.top+50) r = startRect;
			
			sayWindow = NewWindow(NULL, &r, version, visible, documentProc+zoomer,
							FRONT_WINDOW, TRUE, 0);
							
			/* sayWindow better not be NIL at this point (it's not likely) */
			
			if (sayWindow == NIL) { SysBeep(5); ExitToShell(); }
			hidden = !visible;
			SetWindowKind(sayWindow,kConsolationKind);
			//((WindowPeek)sayWindow)->windowKind = kConsolationKind;
			
			/*
			 *	Find out the size of this Mac's main screen.  Make sure width is
			 *	divisible by 8 (required by various bitmap routines).
			 */
			
			r = GetQDPortBounds();
			//r = qd.screenBits.bounds;
			HEIGHT = r.bottom - r.top;
			WIDTH = r.right - r.left;
			WIDTH -= (WIDTH & 0x07);
			
			/*
			 *	Reset the zoom out rectangle to be no larger than small
			 *	Mac screen, so we keep the window no larger than the
			 *	off-screen bitmap.  This is in case a larger screen is
			 *	being used.
			 *
			 *	This is no longer required, since WIDTH and HEIGHT have been
			 *	changed from constants to variables dependent on the screen.
			 */
			/*
			ws = (WStateData *)(*sayRecord.dataHandle);
			ws->stdState.left = 3; ws->stdState.top = 41;
			ws->stdState.right = WIDTH-3; ws->stdState.bottom = HEIGHT-3;
			*/

#ifdef SCROLLBAR
			scrollRect = sayWindow->portRect;
			scrollRect.left = scrollRect.right - SCROLLBARWIDTH;
			scrollRect.bottom -= 16;
			
			scrollBar = NewControl(sayWindow,&scrollRect,"\p",TRUE,0,0,0,scrollBarProc,0L);
			if (!scrollBar) { SysBeep(5); ExitToShell(); }
#endif
			 /*	Get corners of current screen, for finding corner clicks */
			
			SetRect(corner+0,r.left,r.top,r.left+slop,r.top+slop);
			SetRect(corner+1,r.right-slop,r.top,r.right,r.top+slop);
			SetRect(corner+2,r.left,r.bottom-slop,r.left+slop,r.bottom);
			SetRect(corner+3,r.right-slop,r.bottom-slop,r.right,r.bottom);
			
			sayBoth[0] = sayBoth[1] = GetWindowPort(sayWindow);
			startPort = 1;
			
			/*
			 *	Set up the offscreen bitmap, if enough memory for it.  This
			 *	lets the Debug window work well even when occluded by other
			 *	windows.  If it can't use the offscreen bitmap, messages
			 *	may be clipped by other occluding windows, although
			 *	scrolling will work faster without the offscreen stuff.
			 *	Note that a full-screen bit image is 20-80K bytes or so.
			 *	We keep the two ports in an array, with the offscreen one
			 *	second, so we can index them sequentially from startPort,
			 *	leaving the offscreen one selected.
			 */
			
			offscreenBits = (char**)NIL;
			debugMap.baseAddr = (char*)NIL;
			if (!quickDebug) {
				offscreenBits = NewHandle((Size)HEIGHT * (Size)WIDTH/8);
				if (offscreenBits) {
					MoveHHi(offscreenBits); HLock(offscreenBits);
					debugMap.baseAddr = *offscreenBits;
					}
				}
#ifdef CARBON_NOTYET
			smallMemory = (debugMap.baseAddr == NIL);
			if (!smallMemory) {
				debugMap.rowBytes = WIDTH/8;
				SetRect(&debugMap.bounds,0,0,WIDTH,HEIGHT);
				OpenPort(offScreen = &sayPort);
				SetPortBits(&debugMap);
				sayBoth[1] = offScreen;
				startPort = 0;
				}
			 else 
				SetPort(offScreen = GetWindowPort(sayWindow));
#else
			smallMemory = TRUE;
			SetPort(offScreen = GetWindowPort(sayWindow));
#endif			
			 /* Use a smallish fixed-width font for messages in both ports */
			
			if (system7orMore)
				SetOutlinePreferred(FALSE);
			
			for (i=startPort; i<2; i++) {
				SetPort(sayBoth[i]);
				/* But note comment in recent Fonts.h:
						The following font constants are deprecated.  
						Please use GetFNum() to look up the font ID by name.
 																				--JGG, 5/13/00
				*/
				TextFont(kFontIDMonaco); TextSize(9); TextMode(srcXor);
				GetFontInfo(&font);
				lineHeight = font.ascent + font.descent + font.leading;
				DebugMove(row=0,col=0);	/* Move pen to first line of window */
				}
			
			/* If no user func specified yet, set default */
			
			if (userFunc == NIL)
				userFunc = (DebugUserFunc)DebugHeapDump;	
			
			/* Create our text buffer and leave it empty */
			
			theTextSize = theTextLogSize = theFirstChar = 0L;
			theTextIncr = 2048;
			theText = NewHandle(theTextSize);
			if (theText == NIL) { SysBeep(5); ExitToShell(); }
			
			GetNewDebugSize();				/* Compute port size in rows,cols */
			
			/*
			 *	At this point, we're ready to write to window: send out the
			 *	shareware message, with forced pausing so that the user must
			 *	take some notice of it.  The preamble is short and sweet and
			 *	fits in the default window so that the "Thanks." is the only
			 *	line printed after the pause is dismissed.
			 */
			
			coke = SetDebugPause(TRUE);
			for (i=0; preamble[i]; i++) DebugString(preamble[i]);
			if (smallMemory) DebugComment("Using screen bitmap\n");
			SetDebugPause(coke);
			SetPort(oldPort);
			}
		 else
			DebugComment("? Consolation is already initialized\r");
		 
#if defined(PUBLIC_VERSION) 
		ShowHideDebugWindow(false);
		OSErr fileErr = SetDebugFile("ngale.log");
#endif
	}

WindowPtr DebugWindow()		/* Deliver the WindowPtr for Debug window */
	{
		if (sayWindow == NIL) InitDebugWindow(&startRect,TRUE);
		return(sayWindow);
	}

void ShowHideDebugWindow(int show)
	{
		if (sayWindow == NIL) InitDebugWindow(&startRect,TRUE);
		
		if (show) {
			if (!IsWindowVisible(sayWindow)) {
			//if (!((WindowPeek)sayWindow)->visible) {
				/* Show invisible debug window */
				SetDebugLevel(saveLevel);
				ShowWindow(sayWindow);
				SelectWindow(sayWindow);
				hidden = FALSE;
				}
			}
		 else
			if (IsWindowVisible(sayWindow)) {
			//if (((WindowPeek)sayWindow)->visible) {
				saveLevel = level; level = kConsoleNoMessages;
				HideWindow(sayWindow);
				hidden = TRUE;
				}
	}

/**************************************************************************
 *
 *	These routines implement a scrolling text window within the Debug
 *	window port, with text wraparound, so information is not lost if the
 *	Debug window is narrow.  Normally all text is drawn into the offscreen
 *	port, and the whole BitMap is copied into the window after each line is
 *	printed, with clipping in case of any occlusion.  This also happens
 *	when the Debug window needs to be redrawn during update events.
 *
 *************************************************************************/
/*
	Internal routine that figures out various limits, such as how many rows
	and columns of the current font would fit in the Debug window at its
	current size.  Then it erases everything outside of old text in the
	offscreen port, since this is called after either creation or growing.
	When the offscreen bitmap is allocated from memory, it tends to be
	filled with garbage at first.  Printed information in the Debug window
	will be clipped (and lost) whenever the user makes it smaller, and
	then larger.  Delivers TRUE if window gets larger in Y.
 */
 
static int GetNewDebugSize()
	{
		Rect r; GrafPtr oldPort;
		int i,width,oldbottom,oldCols,oldRows,n;

		GetPort(&oldPort); SetPort(offScreen);

		 /* Erase everywhere to the right of and below old text rectangle */
		 
		SetRect(&r,textRect.right,0,WIDTH,HEIGHT);		/* right */
		EraseRect(&r);
		SetRect(&r,0,textRect.bottom,WIDTH,HEIGHT);		/* below */
		EraseRect(&r);
		
		 /* Find size of new screen: assumes fixed-width font */
		
		//growRect = textRect = r = sayWindow->portRect;
		GetWindowPortBounds(sayWindow,&r);
		growRect = textRect = r;
		
#ifdef SCROLLBAR
		textRect.right -= SCROLLBARWIDTH;
#endif		
		growRect.top = growRect.bottom - 16;
		growRect.left = growRect.right - 16;
		OffsetRect(&growRect,1,1);
		SetRect(&grow1,growRect.left+5,growRect.top+5,
					   growRect.right-2,growRect.bottom-2);
		SetRect(&grow2,growRect.left+3,growRect.top+3,
					   growRect.right-6,growRect.bottom-6);
		
		oldCols = nCols; oldRows = nRows;
		GetRowsCols(&r);
		
		/*
		 *	Put the maximum in for starters, and start updating from beginning.
		 */
		
		n = MaxRowLength();
		rlMax = nRows;
		SetAllRowLengths(n);
		rlNext = 0;
			
		 /*
		  *	Add back in any separator line (for esthetic reasons), but only
		  *	if we've grown wider.
		  */
		
		if (nCols >= oldCols) DrawDebugLine(-2);
		
		if (row > nRows) DebugMove(row=nRows,col=0);
		
		 /* Precompute bottom text row's area of window */
		
		r.top = (nRows1) * lineHeight;
		textRect.bottom = r.bottom = nRows * lineHeight;
		oldbottom = bottomRow.bottom;
		bottomRow = r;
		r.right = ((*pausePrompt)+1) * font.widMax;		
		moreRect = r;
		bigMoreRect = moreRect;
		bigMoreRect.right += (*firstPausePrompt) * font.widMax;
		
		 /* Erase below new text rect, in case it's smaller than old one */
		 
		SetRect(&r,0,textRect.bottom,WIDTH,HEIGHT);
		EraseRect(&r);
		
#ifdef SCROLLBAR
		/* Scroll bar has been erased; so silently reposition it to new spot */
		(*scrollBar)->contrlRect = scrollRect;
#endif
		 /* Compute new positions of bit mask boxes */
		
		GetWindowPortBounds(sayWindow,&r);
		//r = sayWindow->portRect;
		r.bottom -= r.top; r.left -= r.right;
		r.bottom--;
		r.top = r.bottom - lineHeight;
		width = 2 * font.widMax - 1;
		for (i=0; i<=MAXMASK; i++) {
			r.left = MARGIN + i*width;
			r.right = r.left + (width - 2);
			if (i > MASKOFF-3) OffsetRect(&r,1,0);
			if (i > MASKOFF) OffsetRect(&r,16,0);
			bitRect[i] = r;
			}
			
		 /* And compute bounding rectangle around all of them */
		Rect offPortRect;
		GetPortBounds(offScreen,&offPortRect);

		controlRect.top = r.top;
		controlRect.bottom = offPortRect.bottom;
		controlRect.left = bitRect[0].left;
		controlRect.right = bitRect[MAXMASK].right;
		
		 /* And bounding rectangle around just the mask bits */
		 
		maskRect = controlRect;
		maskRect.left = bitRect[MASKOFF].left;
		maskRect.right = bitRect[MAXMASK].right;
		
		 /* Figure where Erase Rectangle should be within controlRect */
		
		SetRect(&eraseRect,growRect.left-42,r.top,growRect.left-5, r.bottom);
		
		PaintDebugMask();		/* Draw controls into the bitmap */
		SetPort(oldPort);
		return(oldbottom < bottomRow.bottom);
	}

/*
 *	Compute the number of rows and columns for a given rectangle.
 */

static void GetRowsCols(Rect *r)
	{
		nRows = (r->bottom - lineHeight - 4) / lineHeight;
		nCols = (r->right - MARGIN) / font.widMax;
		nRows1 = nRows-1;
	}


static void PenWhite()
	{
		PenPat(NGetQDGlobalsWhite());
	}

static void PenGray()
	{
		PenPat(NGetQDGlobalsLightGray());
	}

static void PenBlack()
	{
		PenPat(NGetQDGlobalsBlack());
	}

/*
 *	Update new setting on screen, if it exists (it may not if user has called
 *	one of the above configuration routines prior to calling InitDebugWindow()).
 */

static void PaintDebugControls()
	{
		GrafPtr oldPort;
		
		if (sayWindow) {
			GetPort(&oldPort); SetPort(offScreen);
			PaintDebugMask();
			DrawDebugWindow(oldPort,TRUE);
			}
	}

/*
	This simply draws the various Debug window controls into the bitmap
	associated with sayPort, usually offscreen.  The leftmost box in the
	window indicates the current message level by how much it is colored
	in (outlined --> kConsoleNoMessages, filled --> kConsoleAllEvents).  The oval to its
	right is filled if pausing is enabled.  Then comes the user-callable
	function box.  Further boxes to
	the right correspond to bits in the debugMask, and are labeled with
	letters to remind the user of the event that each one stands for.
 */
 
static void PaintDebugMask(void)
	{
		int i; GrafPtr oldPort; Point oldPos; Rect r;
		
		GetPort(&oldPort); SetPort(offScreen);
		GetPen(&oldPos);
		
		EraseRect(&controlRect);
		
		 /* Draw separator line between controls and text */
		
		MoveTo(textRect.left,controlRect.top-3);
		LineTo(textRect.right,controlRect.top-3);
		
		 /* Color area around controls gray like a scroll bar */
		
		Rect portRect;
		GetWindowPortBounds(sayWindow,&portRect);
		SetRect(&r,textRect.left,controlRect.top-2,
				   growRect.left,portRect.bottom);
		FillRect(&r,(ConstPatternParam) NGetQDGlobalsLightGray());
		 /* Now paint in the controls: bumper, level, pause, user, showing, and mask bits */
		 
		PaintLevel();	
		if (level >= kConsoleOutputOnly) {
			PaintPause(pepsi); PaintUserFunc(userFuncing);
			PaintLogFile(logFile); PaintHelp(helping); PaintShowing(showing);
			}
		if (level >= kConsoleApplicationEvents)
			for (i=MASKOFF+1; i<=MAXMASK; i++) PaintMaskBit(i);
		
		PaintErase(FALSE);
		
		 /* And the grow icon, which has precedence over other stuff */
		
		EraseRect(&growRect); FrameRect(&growRect);
		FrameRect(&grow1);
		EraseRect(&grow2); FrameRect(&grow2);
		
		MoveTo(oldPos.h,oldPos.v);
		SetPort(oldPort);
	}

/*
	These next few routines are for individually painting in various of
	the controls - level, pause, user, logfile, help, and mask bit boxes.
	Painting in each case is done with respect to whatever the current port is.
 */

static void PaintLevel()
	{
		register Rect *r;
		
		r = bitRect + BUMPEROFF;
		
		/* Upper triangle */
		
		MoveTo(r->left+3,r->top);
		Line(0,0);
		Move(2,1); PenWhite(); Line(0,0); PenBlack(); Move(-1,0);
		Line(-2,0);
		Move(-1,0); PenWhite(); Line(0,0); PenBlack();
		Move(0,1); Line(4,0);
		Move(1,1); Line(-6,0);
		Move(-1,1); Line(8,0);
		Move(-3,-1);
		PenWhite();
		Line(-2,0);
		Move(1,-1); Line(0,0);
		Move(-4,3); Line(8,0);
		PenBlack();
		
		/* Lower triangle */
		
		MoveTo(r->left+3,r->bottom-1);
		Line(0,0);
		Move(2,-1); PenWhite(); Line(0,0); PenBlack(); Move(-1,0);
		Line(-2,0);
		Move(-1,0); PenWhite(); Line(0,0); PenBlack();
		Move(0,-1); Line(4,0);
		Move(1,-1); Line(-6,0);
		Move(-1,-1); Line(8,0);
		Move(-3,1);
		PenWhite();
		Line(-2,0);
		Move(1,1); Line(0,0);
		PenBlack();
		
		/* Now do level indicator */
		
		r = bitRect + LEVELOFF;
		EraseRect(r); FrameRect(r);
		
		if (level > kConsoleNoMessages) PaintRect(r);
		MoveTo(r->left+1+(level!=1),r->bottom-2);
		DrawChar("0123"[level]);
	}

static void PaintPause(int how)
	{
		register Rect *r;
		
		EraseOval(r=bitRect+PAUSEOFF);
		if (how) PaintOval(r);
		 else	 FrameOval(r);
		MoveTo(r->left+2,r->bottom-3);
		DrawChar('…');
	}

static void PaintUserFunc(int how)
	{
		register Rect *r;
		
		EraseRect(r=bitRect+USERFUNCOFF);
		if (how) PaintRect(r);
		 else	 FrameRect(r);
		MoveTo(r->left+2,r->bottom-1);
		DrawChar('*');
	}

static void PaintLogFile(int how)
	{
		register Rect *r; register int x,y;
		
		EraseRect(r=bitRect+LOGFILEOFF);
		x = r->right - r->left; y = r->bottom - r->top;
		MoveTo(r->right-4,r->top);
		Line(-(x-4),0); Line(0,y-1); Line(x-1,0); Line(0,-(y-5));
		Line(-3,-4); Line(0,4); Line(3,0);
		PenNormal();
		if (how) PaintRect(r);
		MoveTo(r->left+2,r->bottom-2);
		DrawChar('L');
	}

static void PaintHelp(int onoff)
	{
		register Rect *r;

		EraseRect(r=bitRect+HELPOFF);
		if (onoff) PaintRect(r);
		 else      FrameRect(r);
		MoveTo(r->left+2,r->bottom-2);
		DrawChar('?');
	}

static void PaintShowing(int onoff)
	{
		register Rect *r; Rect box;

		EraseRect(r=bitRect+SHOWOFF);
		box = *r;
		if (onoff) PaintRect(r);
		 else {
			FrameRect(r);
			/*
			while (box.left < box.right) {
				InsetRect(&box,2,2);
				FrameRect(&box);
				}
			*/
			}

		MoveTo(r->left+2,r->bottom-2);
		DrawChar('U');
	}

static void PaintMaskBit(register int i)
	{
		static char *labs = " 0…*L?UMmKkaUIAHD123S";
		register Rect *r; long shifter;
		
		EraseRect(r=bitRect+i);
		shifter = i - MASKOFF;
		if (shifter >= 9) shifter++;	/* Compensate for lack of event type 9 */
		if (debugMask & (1L<<shifter)) PaintRect(r);
		 else						   FrameRect(r);
		MoveTo(r->left+2,r->bottom-2);
		DrawChar(labs[i]);
	}

static void PaintErase(int how)
	{
		EraseRect(&eraseRect);
		if (how) PaintRect(&eraseRect);
		 else	 FrameRect(&eraseRect);
		MoveTo(eraseRect.left+4,eraseRect.bottom-2);
		DrawString("\pCLEAR");
	}

/*
 *	Paint More prompt into a given port.  Leaves port selected.
 */

static void PaintMore(int how, GrafPtr port)
	{
		Point oldPos;
		
		SetPort(port);
		if (how) {
			GetPen(&oldPos);
			PaintRect(firstPause ? &bigMoreRect : &moreRect);
			DebugMove(row=nRows1,col=0);
			DrawString((unsigned char *)pausePrompt);
			if (firstPause) DrawString((unsigned char *)firstPausePrompt);
			MoveTo(oldPos.h,oldPos.v);
			}
		 else
			EraseRect(firstPause ? &bigMoreRect : &moreRect);
		}

/*
	All messages generated by the library are sent through DebugComment(),
	before being sent to DebugPrintf(), so that we can prepend the current
	marker string before printing if we're at the beginning of the
	line.  msg should be a null-terminated string.  Internal messages force
	any currently unended line to end first.
 */

static void DebugComment(char *msg)
	{
		if (lastOutside) {
			if (col != 0) DebugString("\n");
			DebugString(marker);
			}
		 else
			if (col == 0) DebugString(marker);
		indent = TRUE; DebugString(msg); indent = FALSE;
		lastOutside = FALSE;
	}
	
/*
	This prints the string msg beginning at the current location (row,col)
	of the Debug window.  Printing is not allowed outside of the window, so
	wraparound to the next line is implemented, and the window is scrolled
	to accommodate new lines at the bottom.  The special characters '\n' and '\r'
	are interpreted to be a Newline character.  msg must be a null-terminated
	string.  Every pauseCount lines printed, the window can pause to wait
	for the user to dismiss it and continue printing messages.
	Ideally the routine should have the same style arguments as printf().
 */

static void DebugString(char *msg)
	{
		GrafPtr oldPort; long len; int err;
		Str255 str; register char *c,*p;
		
		/*
		 *	Log file output can still proceed even when window output is off.
		 */
		
		if (logFile) {
			logFile = FALSE;		/* Avoid recursive loop if error below */
			len = CrushLF(msg);
			err = FSWrite(fileRefNum,&len,msg);
			if (err) DebugString("\r? Can't write to file\r");
			logFile = TRUE;
			}
			
		if (level > kConsoleNoMessages) {
		
			GetPort(&oldPort); SetPort(offScreen);
			/*
			 * For each character in message, adjust the screen position
			 * (with any scrolling and pausing) just before printing it.
			 * Note that row can be a lot bigger than nRows if the window
			 * has just been re-sized to something smaller.
			 * We also indent by the length of the marker string if this is
			 * a continuation of an internal message from DebugComment().
			 * Print it a rowful at a time for efficiency.
			 * If a pause checker returns TRUE, user set level to 0 while
			 * paused, so we quit early.
			 */
			for (p=msg; *p; ) {

				if (row >= nRows)
					if (ScrollDebugWindow(1,FALSE)) break;
				if (*p=='\n' || *p=='\r') {
					SetRowLength(col);
					if (col == 0) if (CheckPause()) break;
					DebugMove(++row,col=0); pauseCount++; p++;
					}
				 else {
				 	if (col >= nCols) {
				 		pauseCount++; SetRowLength(col);
				 		if (row == nRows1) { if (ScrollDebugWindow(1,TRUE)) break; }
					 	 else			   DebugMove(++row,col=0);
					 	}
					 else if (row==nRows1 & col==0) if (CheckPause()) break;
					if (indent && col==0)
						 if ((col+=markerLength) >= nCols) col = 0;
						  else							   DebugMove(row,col);
					/*
					 * Although we could say "{ DrawChar(*p++); col++; }",
					 * it's more efficient to draw a line at a time:
				 	 * convert rest of line to Pascal-style string, and
				 	 * and draw it all at once.
					 */
					
				 	c = (char *)str + 1; str[0] = 0;
				 	while (*p!='\0' && *p!='\n' && *p!='\r' && col<nCols) {
				 		if (*p == '\t') {
				 			do { *c++ = ' '; (*str)++; } while (++col&3);
				 			p++;
				 			}
				 		 else
				 			{ *c++ = *p++; col++; (*str)++; }
				 		}
				 	DrawString(str);	
				 	}
				}
			
			/* Now paint the window with what we just put in the bitmap */
			
			DrawDebugWindow(oldPort,TRUE);
			lastOutside = TRUE;
			}
	}
	
/*
	This maps rows and columns into the the current port's pen location.
	MARGIN is the number of pixels to the right of column 0.  We assume
	that the port's origin is at (0,0).
 */

static void DebugMove(int row, int col)
	{
		col *= font.widMax; row *= lineHeight;
		MoveTo(MARGIN+col,font.ascent+row);
	}

/*
 *	Each row's length in chars is saved in the array rowLength.
 *	We use this array to calculate
 *	the current highWater mark, for optimizing scrolling on large screens.
 *	The lengths are stored into the array circularly.
 */

static void SetRowLength(short col)
	{
		rowLength[rlNext++] = col;
		if (rlNext >= rlMax) rlNext = 0;
	}

static void SetAllRowLengths(short n)
	{
		short i;
		
		for (i=0; i<rlMax; i++) rowLength[i] = n;
	}

/*
 *	Calculate the longest row, as defined by the values in rowLength.
 *	Just zip through the array, since the order the values were put into
 *	it doesn't matter; we just want the high water mark for scrolling.
 */

static short MaxRowLength()
	{
		register short *rl,*rlEnd,m = 0;
		
		rl = rowLength;
		rlEnd = rl-- + rlMax;
		while (++rl < rlEnd) if (*rl > m) m = *rl;
		return(m);
	}

/*
	When the Debug port fills up with messages towards the bottom line,
	we scroll the whole bitmap upwards one line to make room for the
	next line of characters.  Then the bottom line must be erased before
	the next character can be printed.  Once the new line is opened up at
	the bottom, we can also use it for the pause prompt.  This is faster
	than using the Toolbox routine, ScrollRect(), because that routine
	requires allocating an update region, and updating it, which we're
	doing anyway by blasting the whole bitmap back into the window after
	each line.  If check is TRUE, we check to pause if the screen is full.
	We return FALSE for OK, TRUE if CheckPause() says output has been
	turned off.
 */

static int ScrollDebugWindow(int n, int check)
	{
		Rect dst,src; int h,highWater;
		Rect portRect;
		
		GetWindowPortBounds(sayWindow,&portRect);
		h = n * lineHeight;
		src = dst = portRect;
		dst.top -= h;
		src.bottom = nRows*h;
		dst.bottom = src.bottom - h;
		highWater = 1+MaxRowLength();
		src.right = dst.right = highWater * font.widMax;
#ifdef SCROLLBAR
		src.right -= SCROLLBARWIDTH;
		dst.right -= SCROLLBARWIDTH;
#endif
		DrawDebugLine(0);
		const BitMap *offPortBits = GetPortBitMapForCopyBits(offScreen);
		CopyBits(offPortBits,offPortBits,&src,&dst,srcCopy,(RgnHandle)NIL);
		EraseRect(&bottomRow); DebugMove(row=nRows1,col=0);
		while (n-- > 0) DrawDebugLine(-1);
		DrawDebugLine(-2);
		lastScroll = TickCount();
		return( check ? CheckPause() : FALSE );
	}

/* 
 *	This checks if an automatic pause should happen, and performs it if so.
 *	Returns TRUE if user turned level back to zero while paused.
 */
 
static int CheckPause()
	{
		int off = FALSE;
#ifdef CARBON_NOTYET		
		if (pepsi && pauseCount>=nRows1) off = DoDebugPause();
		 else							 CheckUserPause();
#endif
		return(off);
	}

/*
	Here we paint the entire Debug window with whatever should be
	displayed within it.  The Toolbox will clip to the visible region of
	the port, which is assumed to be set to the Debug window port.  The
	port that should be reset afterwards is the argument, oldPort.
	Note how we must temporarily convert various destination coordinates
	into global screen coordinates.  If we haven't actually got an
	off-screen port, there's nothing to do, since all the previous calls to
	DrawString(), etc. will have drawn directly into the on-screen window.
 */

static void DrawDebugWindow(GrafPtr oldPort, int all)
	{
		Rect r,m,s;
		
		SetPort(GetWindowPort(sayWindow));
		
		if (!smallMemory) {		
			RgnHandle visRgn=NULL;
			r = GetQDPortBounds();
			visRgn = NewRgn();
			GetPortVisibleRegion(GetWindowPort(sayWindow), visRgn);
			
#ifdef SCROLLBAR
			r.right -= SCROLLBARWIDTH;
#endif
			LocalToGlobalRect(&r);
			OffsetRgn(visRgn,r.left,r.top);
			GetWindowPortBounds(sayWindow,&s);
#ifdef SCROLLBAR
			s.right -= SCROLLBARWIDTH;
#endif
			const BitMap *offPortBits = GetPortBitMapForCopyBits(offScreen);
			const BitMap *thePortBits = GetPortBitMapForCopyBits(GetQDGlobalsThePort());
			if (all) {
				CopyBits(offPortBits,thePortBits,&s,&r,srcCopy,visRgn);
				}
 			 else {
 			 	m = maskRect;
 			 	LocalToGlobalRect(&m);
				CopyBits(offPortBits,thePortBits,&maskRect,&m,srcCopy,visRgn);
 				}
 			OffsetRgn(visRgn,-r.left,-r.top);
 			if (visRgn!=(RgnHandle)NULL)
 				DisposeRgn(visRgn);
			}
#ifdef SCROLLBAR
		Draw1Control(scrollBar);
#endif
		FlushWindowPortRect(sayWindow);
 		if (oldPort) SetPort(oldPort);
	}


/*
 *	Convert a rectangle in global coordinates to local ones in current port
 */

static void LocalToGlobalRect(Rect *r)
	{
		Point pt;
		
		pt.h = r->left; pt.v = r->top;
		LocalToGlobal(&pt);
		r->left = pt.h; r->top = pt.v;
		
		pt.h = r->right; pt.v = r->bottom;
		LocalToGlobal(&pt);
		r->right = pt.h; r->bottom = pt.v;
	}
	
		
/**************************************************************************
 *
 *	These routines implement the pause mechanism.  This requires direct
 *	manipulation of the event queue, because we must be careful not to
 *	change any events already queued, while listening for the ones needed
 *	to dismiss the event.  To do this, we must discriminate events in the
 *	queue by the time they were posted, rather than by type.  The Toolbox
 *	routines that deliver events only deliver the first event in the queue
 *	of a given type.  Therefore, the algorithm here is to directly scan
 *	the queue, looking for events after the time the pause prompt was
 *	written, noting the found event's type, swapping the queued event with
 *	that of the event at the head of the queue, and then calling
 *	GetOSEvent() using the type of the swapped event.  This maintains the
 *	order and values of all the previous events in the queue, while letting
 *	the Toolbox routine do the nasty secret work of deleting the event from
 *	the queue.
 *
 *************************************************************************/
/*
	EQToFront() takes a pointer to a queued event, snips the event from
	the queue list, and pastes it in at the head of the line (I've heard
	of cutting into the head of the line, but pasting??).  If the event
	was the last event in the queue, the queue header field is adjusted
	to reflect its disappearance.
	
	NOTE:	This needs to work with interrupts disabled.  If the system
			posts an event while we're accessing the queue tail pointer
			there could be a sad misunderstanding.  I haven't figured out
			how to disable interrupts, yet.  On the other hand, in 2 years
			of using this without disabling interrupts, it has never caused
			any problems, so who knows.
 */
#ifdef CARBON_NOTYET
static void EQToFront(register QElemPtr evt)	/* Bring queued event to front of list */
	{
		register QElemPtr last, e;
		
		if ((e=LMGetEventQueue()->qHead) != evt) {
			while (e != evt) { last = e; e = e->qLink; }
			last->qLink = e->qLink;
			e->qLink = LMGetEventQueue()->qHead;
			LMGetEventQueue()->qHead = e;
			 /* This statement should be uninterruptable by Event Manager!! */
			if (LMGetEventQueue()->qTail == e) LMGetEventQueue()->qTail = last;
			}
	}
#endif
/*
	EQScan() scans the event queue for an event whose time of posting is
	greater than a given time, and delivers a pointer to that queue element,
	or NIL if none found.  Only events matching the types coded for in the
	standard event mask are candidates.  We also ensure that only events
	with legal type codes are checked, since the Event queue Toolbox
	routines may use other undocumented codes (e.g. -1).
 */

#ifdef CARBON_NOTYET
static EvQElPtr EQScan(int mask, long now)	/* Scan for event later than now */
	{
		EvQElPtr evt,tail;
		int what;
		
		evt = (EvQElPtr)(LMGetEventQueue()->qHead);		/* First event in queue */
		tail = (EvQElPtr)(LMGetEventQueue()->qTail);	/* Last event in queue */
		/*
		 *	If an event is posted now, it will be appended in a new link
		 *	beyond where tail now points (since tail is only a local copy here).
		 *	So we won't reach it here until EQScan() is called next time.
		 *	Note that no event can be taken out of queue while we're in
		 *	the scan loop, so we won't have a link pulled out from under us.
		 *	The test for being at the end of the queue is done by comparing
		 *	with tail, not by testing for NIL, since the Toolbox may or may not
		 *	keep extra information beyond tail in the list (not likely, but if
		 *	it's not documented, better to be on the safe side).
		 */
		while (evt) {
			what = evt->evtQWhat;
			if (what>=nullEvent && what<=osEvt)		/* Only legal events */
				if (mask & (1 << what))				/* that match mask */
					if (evt->evtQWhen > now)		/* and are "recent" */
						return(evt);				/* are matched */
			if (evt == tail)
				evt = NIL;							/* Quit if at end */
			 else
				evt = (EvQElPtr)(evt->qLink);		/* Or on to next one */
			}
		return(evt);
	}
#endif
/*
	GetPauseEvent() finds a first event posted prior to a given time, now,
	and returns TRUE or FALSE, and the event, accordingly.  It scans the
	queue for a candidate, swaps it with the head of the queue, and calls
	GetOSEvent(), which knows how to maintain the queue when an event is
	deleted from it, and which does not deliver any pseudo-events such as
	GetNextEvent() does, to deliver the event.  Only events coded for in
	the mask are candidates (usually either a mouse or keyboard event).
 */

#ifdef CARBON_NOTYET
static int GetPauseEvent(short pauseMask, EventRecord *event, long now)
	{
		EvQElPtr evt,head;
		int maskBit;
		/*
		if (hasMultifinder) return(WaitNextEvent(pauseMask,event,0,NIL));
		*/
		head = (EvQElPtr)(LMGetEventQueue()->qHead);	/* First event in queue */
		evt = EQScan(pauseMask,now);
		if (evt) {								/* Pause event found ? */
			EQToFront((QElemPtr) evt); 			/* Swap event with head */
			maskBit = (1 << evt->evtQWhat);		/* Make legal mask bit */
			return(GetOSEvent(maskBit,event));	/* And pull first event */
			}
		return(FALSE);
	}
#endif
/*
	When it's time to pause, we wait for the user to tap a key or mouse.
	Any key other than the space bar will scroll one line at a time before
	pausing again; the space bar lets a whole screenful arrive before the
	next pause.  A mouse up event does also.  Mouse down events are ignored
	since the closely following mouse up event usually won't be close
	enough in time for us to catch it here.  The COMMAND key plus space bar
	will erase the Debug window, but otherwise acts the same as a space.
	We return TRUE if user sets output level to 0 while paused.
 */

#ifdef CARBON_NOTYET
extern Cursor TheCrsr : 0x844;
extern short  CrsrState : 0x8D0;

static int DoDebugPause()
	{
		EventRecord evt; long now,n,lastn; int ch,pauseMask,part;
		int paused = TRUE, skipFirstUp = TRUE, fronted = FALSE, off = FALSE;
		Cursor saveCursor; int cLevel;
		WindowPtr w; Point p;
		
		if (!singleStep) SysBeep(1);		/* Let user know we've halted */
		DrawDebugLine(0);					/* Clean up old separator line */
		PaintMore(TRUE,offScreen);			/* Paint more prompt in black */
		DrawDebugWindow(offScreen,TRUE);	/* and update screen, too */
		lastn = -1;							/* Force pause to start blinking */
		
		pauseMask = mDownMask|mUpMask|keyDownMask|autoKeyMask|updateMask;
		
		saveCursor = TheCrsr;			/* Hold current cursor pattern */
		cLevel = CrsrState;				/* Cursor visibility level */
		SetCursor(&qd.arrow);			/* Set cursor back to arrow */
		CrsrState = 0;					/* Set its visibility to normal */
		
		now = LMGetTicks();					/* Start of pause events */

		while (paused) {
			if (!hasMultifinder) SystemTask();
			n = (LMGetTicks() - now) >> 4;		/* Approximately in 1/4 seconds */
			if (n != lastn) {
				SetPort(sayWindow);		/* Flash pause box directly */
				PaintPause(n & 1L);
				SetPort(offScreen);
				lastn = n;
				}
			 /*
			  *	Eat all events of type pauseMask occurring after now, until
			  *	there are no more.
			  */
			
			while (GetPauseEvent(pauseMask,&evt,now)) {
				p = evt.where;
				switch(evt.what) {
					case keyDown:
					case autoKey:
						ch = (evt.message & charCodeMask);
						if (ch != ' ') singleStep = TRUE;
						 else {
							if (evt.modifiers & cmdKey) DebugErase();
							pauseCount = 0;
							singleStep = FALSE;
							}
						paused = FALSE;
						break;
					case mouseUp:
						if (skipFirstUp) skipFirstUp = FALSE;
						 else {
							pauseCount = 0; paused = singleStep = FALSE;
							if (fronted) SendBehind(sayWindow,NIL);
							}
						break;
					case updateEvt:
						DoDebugUpdate();
						break;
					case mouseDown:
						SetPort(sayWindow);
						GlobalToLocal(&p);
						SetPort(offScreen);
						if (PtInRect(p,bitRect+PAUSEOFF)) {
							DealWithMask(p,offScreen,evt.modifiers);
							paused = pepsi;
							}
						 else if (PtInRect(p,bitRect+LOGFILEOFF))
							DealWithMask(p,offScreen,evt.modifiers);
						 else if (PtInRect(p,bitRect+HELPOFF)) {
						 	if (helping) paused = helping = FALSE;
						 	}
						 else if (PtInRect(p,bitRect+SHOWOFF)) {
						 	if (showing) paused = showing = FALSE;
						 	}
						 else if (PtInRect(p,bitRect+LEVELOFF)) {
							DealWithMask(p,offScreen,evt.modifiers);
							off = level==0;
							if (off) { paused = FALSE; break; }
							}
						 else if (PtInRect(p,bitRect+BUMPEROFF)) {
							DealWithMask(p,offScreen,evt.modifiers);
							off = level==0;
							if (off) { paused = FALSE; break; }
							}
						 else if (PtInRect(p,&eraseRect)) {
						 	if (DoDebugErase()) paused = FALSE;
							}
						 else if (PtInRect(p,&maskRect)) {
							DealWithMask(p,offScreen,evt.modifiers);
							}
						 else {
							part = FindWindow(evt.where,&w);
							switch(part) {
								case inDrag:
									if (w == sayWindow) {
										DoDebugDrag(&evt);
										DrawDebugWindow((GrafPtr)NIL,TRUE);
										}
									break;
								case inGrow:
									if (w == sayWindow) {
										paused = !DoDebugGrow(evt.where,TRUE);
										if (pauseCount > nRows1) pauseCount = nRows1;
										}
									break;
								case inZoomOut:
									if (w == sayWindow) zoomRows = nRows;
									/* Fall through */
								case inZoomIn:
									if (w == sayWindow) {
										if (TrackBox(w,p,part))
											paused = !DoDebugZoom(w,part,TRUE);
										if (pauseCount > nRows1) pauseCount = nRows1;
										}
									break;
								case inGoAway:
									if (w == sayWindow)
										if (DoGoAway(w,p,evt.modifiers))
											{ paused = FALSE; off = TRUE; }
									break;
								default:
									if (sayWindow != FrontWindow()) {
										HiliteWindow(FrontWindow(),FALSE);
										BringToFront(sayWindow);
										HiliteWindow(sayWindow,TRUE);
										GetNextEvent(activMask,&evt);
										GetNextEvent(updateMask,&evt);
										BeginUpdate(sayWindow);
										DrawDebugWindow(offScreen,TRUE);
										EndUpdate(sayWindow);
										fronted = TRUE;
										}
									 else
										skipFirstUp = FALSE;
								}
							}
						break;
					}
				}
			}
		/* 
		 *	Get rid of prompt, leaving line erased; insert separator line,
		 *	and reset pen, etc. as if nothing happened.
		 */
		PaintMore(FALSE,sayWindow); PaintMore(FALSE,offScreen);
		if (!off) {
			PaintPause(pepsi);
			if (pauseCount==0 && row!=0) DrawDebugLine(nRows-1);
			}
		DebugMove(row,col);
		/* Not a good idea if original was color cursor: SetCursor(&saveCursor); */
		while (cLevel != 0)
			if (cLevel < 0) { cLevel++; HideCursor(); }
			 else           { cLevel--; ShowCursor(); }
		
		firstPause = FALSE;
		return(off);
	}
#endif

/*
 *	This is called to see if the user has by any chance clicked the
 *	mouse in the pause oval or elsewhere, which is now set to no pausing.
 *	If they have, we deal with it.  It also calls SystemTask() so
 *	that long printouts in the window don't stop other needed activity
 *	(like a printout being spooled, or whatever).
 */

#ifdef CARBON_NOTYET
static void CheckUserPause()
	{
		EventRecord event; GrafPtr oldPort;
		WindowPtr w; Point p; short part;
		
		if (!hasMultifinder) SystemTask();
		if (OSEventAvail(mDownMask,&event)) {
			part = FindWindow(p=event.where,&w);
			if (part==inContent && w==sayWindow) {
				GetPort(&oldPort); SetPort(sayWindow);
				GlobalToLocal(&p);
				if (PtInRect(p,&controlRect)) {
					GetOSEvent(mDownMask,&event);
					DealWithMask(p,oldPort,event.modifiers);
					}
				 else
					SetPort(oldPort);
				}
			 else if (part==inDrag && w==sayWindow) {
			 	GetOSEvent(mDownMask,&event);
				DoDebugDrag(&event);
			 	}
			 else if (part==inGoAway && w==sayWindow) {
				GetOSEvent(mDownMask,&event);
				DoGoAway(w,p,event.modifiers);
				}
			}
	}
#endif
	
/*
 	This draws or erases the screen separator line between screenfuls.
 	Line is drawn between row and row-1, erased if row is 0.  Erasing is
 	technically not needed, since the line will eventually scroll out of
 	the window, but it looks bad right next to the bottom of the drag bar.
 */
 
static void DrawDebugLine(int row)
	{
		PenState old; static int last = -1;
		
		if (row == -1) last--;
		 else {
			GetPenState(&old);
			if (row >= 0) {
				/* last = row; */
				if (row == 0) PenMode(patBic); else last = row;
				}
			PenGray();
			MoveTo(0,last*lineHeight);
			if (row!=-2 || last!=0) Line(WIDTH,0);
			SetPenState(&old);
			}
	}

/*
 *	Track a click box
 */

static int TrackClickBox(Rect *box, PaintFunc Painter)
	{
		GrafPtr oldPort; Point p; int last = -1;
		
		GetPort(&oldPort); SetPort(GetWindowPort(sayWindow));
		GetMouse(&p);
		if (StillDown())
			while (WaitMouseUp()) {
				GetMouse(&p);
				if (PtInRect(p,box)) { if (last != TRUE) Painter(last=TRUE); }
				 else				 { if (last != FALSE) Painter(last=FALSE); }
				}

#ifdef ONHOLD
		/* Now report on eaten mouse up event if level is kConsoleAllEvents */
		
		if (level == kConsoleAllEvents) {
			event = theEvent;
			event.what = mouseUp;
			event.when = TickCount();
			event.where = p;
			LocalToGlobal(&event.where);
			part = FindWindow(event.where,&w);
			DebugMouseMsg(&event,w,part);		/* Print mouse event msg */
			}
#endif
		SetPort(oldPort);
		return(PtInRect(p,box));
	}

/*
 *	Erase the debug window
 */

void DebugErase()
	{
		GrafPtr oldPort;
		
		GetPort(&oldPort); SetPort(offScreen);
		EraseRect(&textRect);
		DebugMove(row=pauseCount=0,col=0);
		DrawDebugWindow(oldPort,TRUE);
		singleStep = FALSE;
		SetAllRowLengths(0);
	}

/*
 *	Track the CLEAR control and then erase window
 */
	
static int DoDebugErase()
	{
		int ans;
		
		ans = TrackClickBox(&eraseRect,(PaintFunc)PaintErase);
		if (ans) DebugErase();
		return(ans);
	}
	
		
/**************************************************************************
 *
 *  The routines in this section of code implement the event filter,
 *	which listens in on the event stream, comments on all events, and
 * 	deals with those that pertain specifically to the Debug window.  After
 *	commenting by printing a message in the Debug window, the event is
 *	passed back to the caller with no change.  Events pertaining to the
 *	Debug window are NOT passed back to the application, since they are
 *	dealt with internally, and the application should never need to know
 *	about them.
 *
 *************************************************************************/
/*
	This routine is intended as a substitute in the application code for
	the Toolbox routine, GetNextEvent(), which is called in most Mac
	programs.  Arguments to GetDebugEvent() are the same as those of
	GetNextEvent(): the event mask, and the address of the event record.
	We only return the next event if it has not been classified by the
	commenter as an internal event, i.e. one pertaining to the Debug window.
	Note how it does deliver null events, which happen most of the time,
	so that the application can continue looping, calling SystemTask(), or
	whatever.
 */
 
int GetDebugEvent(short mask, EventRecord *event)
	{
		int answer,internal;
		
		if (sayWindow == NIL) InitDebugWindow(&startRect,TRUE);
		
		do {
		
			answer = userEvent;
			if (answer) {
				*event = postEvent;
				userEvent = FALSE;
				}
			 else
				answer = GetNextEvent(mask,event);
			
			theEvent = *event;
			internal = FALSE;
			if (answer) {
				internal = DoDebugComment(event);
				if (internal)
					DealWithDebug(event);
				}
				
			} while (internal);
		   
		return(answer);
	}

/*
	This routine is intended as a substitute in the application code for
	the Toolbox routine, WaitNextEvent(), which is called in most Mac programs
	under Multifinder.  Arguments to WaitDebugEvent() are the same as those of
	WaitNextEvent(): the event mask, the address of the event record, the time
	that the system should return back to us here, and a mouse region.
	We only return the next event if it has not been classified by the
	commenter as an internal event, i.e. one pertaining to the Debug window.
 */
 
int WaitDebugEvent(short mask, EventRecord *event, long nTicks, RgnHandle rgn)
	{
		int answer,internal; GrafPtr oldPort; Point p;
		
		if (sayWindow == NIL) InitDebugWindow(&startRect,TRUE);
		
		do {
			answer = userEvent;
			if (answer) {
				*event = postEvent;
				userEvent = FALSE;
				}
			 else
				answer = WaitNextEvent(mask,event,nTicks,rgn);
			theEvent = *event;
			internal = FALSE;
			if (answer) {
				internal = DoDebugComment(event);
				if (internal)
					DealWithDebug(event);
				}
			 else {
				/* Null event, change to arrow if over any part of Consolation window */
				RgnHandle visRgn=NewRgn();
				GetPort(&oldPort); SetPort(GetWindowPort(sayWindow));
				p = event->where;
				GlobalToLocal(&p);
				SetPort(oldPort);
				GetPortVisibleRegion(GetWindowPort(sayWindow), visRgn);
				if (PtInRect(p,&controlRect) && PtInRgn(p,visRgn)) {

					Cursor arrow;
					GetQDGlobalsArrow(&arrow);
					SetCursor(&arrow);
					}
				if (visRgn!=(RgnHandle)NULL)
					DisposeRgn(visRgn);
				}

			} while (internal);
		   
		return(answer);
	}
	
/*
	If the event is classified as pertaining to the Debug window,
	we deal with it here.  This is where we take care of selecting,
	dragging, resizing, and updating the Debug window.  Clicking the mouse
	alternately activates and brings to the front the Debug window, or
	deactivates and buries the window.  The window receives and scrolls
	messages whether it's active or not.  When we resize the window, we
	keep a copy of the previous window size, so that GetNewDebugSize()
	can know what to erase in the offscreen port's bitmap.
 */

static void DealWithDebug(EventRecord *event)
	{
		Point p; int ch;
		WindowPtr w; GrafPtr oldPort;
		
		switch(event->what) {
			case nullEvent:
				break;
			case mouseDown:
				p = event->where;
				w = foundWindow;
				switch(foundPart) {
					case inMenuBar:
						DoDebugAppleMenu(event->where);
						break;
					case inCorner:			/* Pseudo part code */
						if (hidden) {
							ShowHideDebugWindow(TRUE);
#ifdef NOMORE				
							SetDebugLevel(saveLevel);
							ShowWindow(sayWindow);
							SelectWindow(sayWindow);
							hidden = FALSE;
#endif				
							break;
							}
						/* Fall thru in order to bring window to front */
					case inContent:
						GetPort(&oldPort); SetPort(GetWindowPort(sayWindow));
						GlobalToLocal(&p);
						if (PtInRect(p,&growRect))
							DoDebugGrow(event->where,FALSE);
						 else if (PtInRect(p,&eraseRect))
							DoDebugErase();
						 else if (PtInRect(p,&controlRect))
							DealWithMask(p,oldPort,event->modifiers);
						 else
							if (sayWindow != (w=FrontWindow())) {
								oldWindow = w;
								SelectWindow(sayWindow);
								}
						 	 else {
						 		SendBehind(sayWindow,(WindowPtr)NIL);
						 	 	if (oldWindow) SelectWindow(oldWindow);
						 		}
						SetPort(oldPort);
						break;
					case inDrag:
						DoDebugDrag(event);
						break;
					case inGrow:
						DoDebugGrow(event->where,FALSE);
						if (pauseCount > nRows1) pauseCount = nRows1;
						break;
					case inZoomOut:
						zoomRows = nRows;
						/* Fall through */
					case inZoomIn:
						if (TrackBox(w,p,foundPart))
							DoDebugZoom(w,foundPart,FALSE);
						if (pauseCount > nRows1) pauseCount = nRows1;
						break;
					case inGoAway:
						DoGoAway(w,p,event->modifiers);
						break;
					}
				break;
			case keyDown:
				ch = (event->message & charCodeMask);
				if (ch==' ' && (event->modifiers & cmdKey)!=0)
					DebugErase();
				break;
			case updateEvt:
				DoDebugUpdate();
				break;
			case activateEvt:
#ifdef MENUMUNG
				DoAboutFace((event->modifiers & activeFlag) != 0);
#endif
				break;
			case osEvt:
				w = FrontWindow();
				if (event->message & 1) {		/* Resume */
					if (sayWindow == w) {
						SetPort(GetWindowPort(sayWindow));
						}
					/* SetDebugPause(wasPaused); */
					inBackground = FALSE;
					}
				 else {							/* Suspend */
					/* wasPaused = SetDebugPause(FALSE); */
					inBackground = TRUE;
					}
				break;
			}
	}

/*
 *	Swap "About ..." Apple menu item with one for the Consolation
 *	window whenever it becomes the active window.  This routine is no
 *	longer used, since it's impossible to do this transparently, and
 *	because it mucks with the user's idea of the universe.  But the code
 *	works and is here in case I need it in the future.
 */

#ifdef MENUMUNG

void DoAboutFace(int activate)
	{
		static Str255 itemStr;
		static int lastChange = -1;
		int locked;
		MenuListInfo *mList;
			
		if (activate != lastChange) {
		
			/*
			 *	If no menu list has been allocated, or no menus are in the
			 *	list, just tiptoe back out of here with no changes.
			 */
			
			if (!MenuList) return;
			mList = (MenuListInfo *)*MenuList;
			if (mList->offset < 6)	return;
			
			/*
			 *	Good menu list: find out if it's locked or not so we can
			 *	transparently lock it here while setting items.  The
			 *	handle's master pointer's highest-order bit (sign bit) is
			 *	the lock bit (see Inside Macintosh, II-25).  I have no
			 *	idea whether this is necessary, but better to be safe than
			 *	crashed.
			 */
			
			locked = ( (long)(*MenuList) < 0);
			HLock(MenuList);
			if (activate) {
				GetItem(mList->firstMenu,1,itemStr);
				SetItem(mList->firstMenu,1,"\PAbout Consolation™... ");
				}
			 else {
			 	SetItem(mList->firstMenu,1,itemStr);
			 	}
			if (!locked) HUnlock(MenuList);
			
			/*
			 *	Remember we were here so we won't repeat without alternating
			 *	activate/deactivate events; otherwise we'll lose the value
			 *	of the original application menu item.
			 */
			
			lastChange = activate;
			}
	}

#endif

/*
 *	Handle a grow window mouse down click; return TRUE if larger.
 */

static int DoDebugGrow(Point p, int paused)
	{
		Rect limit; long newSize; int x,y,n,oldRows; int larger;
		
		SetRect(&limit, 30, 50, WIDTH, HEIGHT);
		newSize = GrowWindow(sayWindow,p,&limit);
		if (newSize) {
			x = LoWord(newSize); y = HiWord(newSize);
			oldRows = nRows;
			SizeWindow(sayWindow,x,y,FALSE);
			Rect portRect; GetWindowPortBounds(sayWindow,&portRect);
			GetRowsCols(&portRect);
			SetPort(offScreen);
			if (paused) EraseRect(&bottomRow);
			if (nRows < oldRows) {
				n = row - nRows + paused;
				if (n > 0) {
					ScrollDebugWindow(n,FALSE);
					DebugMove(row=nRows,col=0);
					}
				}
			larger = GetNewDebugSize();
			if (!larger && paused) {
				EraseRect(&bottomRow);
				PaintMore(TRUE,offScreen);
				DebugMove(row=nRows1,col=0);
				}
			DrawDebugWindow(offScreen,TRUE);
			return(larger);
			}
		return(FALSE);
	}

/*
 *	Handle a zoom window command; return TRUE if larger.  If the screen becomes
 *	smaller, we scroll the contents so that the most recent messages stay
 *	visible.
 */

static int DoDebugZoom(WindowPtr w, int part, int paused)
	{
		int larger; int n;
		
		if (part == inZoomIn) {
			n = row - zoomRows + paused;
			if (n > 0) ScrollDebugWindow(n,FALSE);
			}
		 else
			if (paused) PaintMore(FALSE,offScreen);
		Rect portRect; GetWindowPortBounds(w,&portRect);
		SetPort(GetWindowPort(w)); EraseRect(&portRect);
		ZoomWindow(w,part,FALSE);
		larger = GetNewDebugSize();
		DrawDebugWindow(offScreen,TRUE);
		if (!larger && paused) {
			DebugMove(row=nRows1,col=0);
			}
		return(larger);
	}

/*
 * Handle a click in goAway box
 */

static int DoGoAway(WindowPtr w, Point p, int modbits)
	{
		int ans = FALSE;
		
		if (TrackGoAway(w,p))
			if (modbits & optionKey) ExitToShell();
		 	else {
		 		ShowHideDebugWindow(FALSE);
#ifdef NOMORE
				saveLevel = level; level = kConsoleNoMessages;
				HideWindow(sayWindow);
				hidden = ans = TRUE;
#endif
				ans = TRUE;
				}
		return(ans);
	}

/*
 *	Handle a drag window mouse down click
 */

static void DoDebugDrag(EventRecord *evt)
	{
		Rect rect,bbox;
#if 0		
		RgnHandle grayRgn = NewRgn();
		grayRgn = GetGrayRgn();
		GetRegionBounds(grayRgn,&bbox);
		DisposeRgn(grayRgn);
#else
		bbox = GetQDScreenBitsBounds();
#endif
		
		if ((evt->modifiers & cmdKey) == 0)
			if (sayWindow != FrontWindow()) oldWindow = FrontWindow();
		rect = bbox; InsetRect(&rect,10,10);
		DragWindow(sayWindow,evt->where,&rect);
	}

/*
 *	Handle an update event
 */

void DoDebugUpdate()
	{
		GrafPtr oldPort;
		
		GetPort(&oldPort);
		BeginUpdate(sayWindow);
		DrawDebugWindow(oldPort,TRUE);
		if (smallMemory) PaintDebugMask();
#ifdef SCROLLBAR
		DrawGrowIcon(sayWindow);
#endif
		EndUpdate(sayWindow);
	}

/*
 *	Handle a mouse down event in the Apple menu.
 */

static void DoDebugAppleMenu(Point p)
	{
		Point q = p;
#ifdef MENUMUNG
		WindowPtr w;
		long menuChoice; int menu,choice;
		
		menuChoice = MenuSelect(p);
		menu = HiWord(menuChoice); choice = LoWord(menuChoice);
		/*
		 * etc.  We need to look up the menu ID in the menuList to check
		 * that it is the first menu, then deal with it.
		 * However, MenuSelect() keeps control while the user transfers
		 * among menus, and it's too late here to deliver the appropriate
		 * information to the application in a transparent way, so that's
		 * why there's no code here.  I gave up.
		 */
		HiliteMenu(0);
#endif
	}


/*
 *	Given a point, p, within the mask rectangle, we find which bit, if
 *	any it corresponds to, and set or clear it in the mask and in
 *	the window.
 */

static void DealWithMask(Point p, GrafPtr oldPort, int modifiers)
	{
		int once = TRUE; 
		
		SetPort(GetWindowPort(sayWindow));
		
		if (StillDown())
			while (WaitMouseUp())
				{ GetMouse(&p); DWMReally(p,once,modifiers); once = FALSE; }
		 else
		 	DWMReally(p,once,modifiers);
		 	
		if (!smallMemory) PaintDebugMask();
		SetPort(oldPort);
	}

/*
 *	Really deal with mouse click in the mask rectangle of sayWindow.
 */

static char *help[] = {
#if 0
	"\rThese notes describe Consolation’s interactive features.\r",
	"Please see the manual for more information on how to\r",
	"install it into your main event loop easily, how to call\r",
	"Consolation library routines from your application, etc.\r\r",

	"The control bar at the bottom of Consolation’s window\r",
	"contains a series of small labeled controls.  To operate\r",
	"the controls, simply click on them.  They are always\r",
	"active, whether Consolation is the front window or not.\r\r",
	
	"From left to right, the controls are:\r\r",
	
	"  [0-3]\tWINDOW OUTPUT LEVEL\r",
	"\t\t[0] Suppress all output from being printed\r",
	"\t\t\t(log file output will still be written).\r",
	"\t\t[1] Print only application-generated output\r",
	"\t\t\t(that is, from calls to DebugPrintf()).\r",
	"\t\t[2] Print application output and report all\r",
	"\t\t\tapplication-related events whose mask\r",
	"\t\t\tbits are enabled in the control bar.\r",
	"\t\t[3] Same as [2], but also report events\r",
	"\t\t\tpertaining to the Consolation window.\r",
	"\t\tTo adjust the level up or down, click on the\r",
	"\t\tup or down arrow to the left of the indicator.\r",
	"\t\tTo turn window output on or off, click directly\r",
	"\t\ton the level indicator to toggle between [0]\r",
	"\t\tand your last non-[0] level.\r\r",
	
	"  (…)\tSET PAUSE ON/OFF\r",
	"\t\tWith Pause off, output scrolls off the top\r",
	"\t\tof the window.  When on, output pauses after\r",
	"\t\teach screenful, the Pause control blinks, and\r",
	"\t\ta highlighted “…more…” prompt is displayed.\r",
	"\t\tYou can then dismiss the pause by doing any of:\r",
	"\t\t\tClick in (…) to turn Pausing back off again;\r",
	"\t\t\tTap SPACE for another screenful;\r",
	"\t\t\tTap COMMAND SPACE to erase first;\r",
	"\t\t\tTap RETURN to output one more line;\r",
	"\t\t\tClick in the CLEAR box to erase first;\r",
	"\t\t\tClick mouse to unbury window; and then\r",
	"\t\t\tagain to bury it and dismiss the pause.\r",
	"\t\tWhile paused, click controls remain active.\r\r",
	
	"  [*]\tINVOKE APPLICATION-DEFINED FUNCTION\r",
	"\t\tIf application has provided a function to call,\r",
	"\t\tConsolation calls it when you click this box,\r",
	"\t\twith modifier key info passed as its argument.\r",
	"\t\tIf not, [*] drops you into MacsBug or other low\r",
	"\t\tlevel debugger, and issues a Heap Check\r",
	"\t\tcommand.  Type ‘G’<return> to continue.\r\r",
	
	"  [L]\tLOG FILE ON/OFF\r",
	"\t\tIf there is no current output file, you are\r",
	"\t\tasked to specify one, and subsequent output\r",
	"\t\tto the screen is also copied to the file.\r",
	"\t\tIf a log file is already open, output to it\r",
	"\t\tis turned off or on when you click [L] again.\r",
	"\t\tOption-Click closes any current output file,\r",
	"\t\tand then asks you for another one to open.\r",
	"\t\tNOTE: Setting the window output level to [0]\r",
	"\t\t      doesn't turn off output to the log file.\r\r",
	
	"  [?]\tHELP (this message)\r",
	"\t\tYou can use [L] to log this help output into a\r",
	"\t\tfile for later reading or printing.\r\r",
	
	"  [U]\tSHOW UPDATE REGIONS\r",
	"\t\tShow the Update Region of any window about to\r",
	"\t\treceive an Update Event by filling with black.\r",
	"\t\tOption-clicking sets Consolation to erase the\r",
	"\t\tblackened update region before delivering the\r",
	"\t\tupdate event to your main event loop.  You can\r",
	"\t\thold the Shift key down during update painting\r",
	"\t\tto postpone the event delivery until you let go.\r\r",
	
	"When Consolation’s output level is [2] or greater,\r",
	"a series of boxes appears allowing you to control\r",
	"the bits in a Report Event mask.  You can click\r",
	"and drag over these bits to set or clear them.\r",
	"Consolation automatically reports in readable\r",
	"text any events whose bits in the mask are set.\r",
	"If the event has to do with Consolation itself,\r",
	"it is taken care of automatically.  Otherwise,\r"
	"the event is delivered to your application’s\r",
	"event loop normally.  These mask bits are:\r",
	"\t[M]\tReport Mouse Down event\r",
	"\t[m]\tReport Mouse Up event\r",
	"\t[K]\tReport Key Down event\r",
	"\t[k]\tReport Key Up event (see Note below)\r",
	"\t[a]\tReport Autokey event\r",
	"\t[U]\tReport Update event\r",
	"\t[I]\tReport Insert Disk event\r",
	"\t[A]\tReport Activate event\r",
	"\t[H]\tReport High Level events\r",
	"\t[D]\tReport Driver event (obsolete)\r",
	"\t[1]\tReport Application 1 event\r",
	"\t[2]\tReport Application 2 event\r",
	"\t[3]\tReport Application 3 event\r",
	"\t[S]\tReport Suspend/Resume OS events\r",
	"NOTE: Any events, such as Key Up Events [k],\r",
	"      that are disabled by the System Event\r",
	"      Mask are never reported.\r\r",
	
	"The CLEAR box clears all text from the window.\r\r",
	
	"Clicking in the GoAway Box sets the window output\r",
	"level to [0] and hides the window.  Option-clicking\r",
	"in the GoAway Box terminates your application\r",
	"with extreme prejudice.\r\r",
	
	"Clicking anywhere within the visible content of\r",
	"the Consolation window, other than the control bar,\r",
	"pulls it in front of other windows, or buries it if\r",
	"it was already in front.\r\r",
	
	"If Consolation is hidden or completely covered\r",
	"by other windows, you can bring it to the front by\r",
	"clicking the mouse in any corner of the Desktop.\r\r",
#endif
	(char*)NIL
	};

static void DWMReally(Point p, int once, int modifiers)
	{
		short i,j; Rect r;
		static int f0,f1,f2,f3,f4,f5,f6,first,reset,draw;

		if (PtInRect(p,&maskRect) && modifiers&cmdKey) {
#ifdef POST
			DoDebugPost(p,oldPort);
#else
			SysBeep(1);
#endif
			return;
			}

		if (once) f0 = f1 = f2 = f3 = f4 = f5 = f6 = first = draw = TRUE;
		
 	 	for (i=0; i<=MAXMASK; i++)
 	 		if (PtInRect(p,bitRect+i))
 	 			if (i == BUMPEROFF) {
 	 				draw = f0;
 	 				if (draw) {
 	 					r = bitRect[BUMPEROFF];
 	 					r.top += 1+((r.bottom-r.top)>>1);
 	 					if (PtInRect(p,&r))
 	 						{ if (level > kConsoleNoMessages) level--; }
 	 					 else
  	 						{ if (level < kConsoleAllEvents) level++; }
	 					f0 = f1 = f2 = f3 = f4 = f5 = f6 = first = FALSE;
	 					PaintDebugMask();
 	 					DrawDebugWindow(GetWindowPort(sayWindow),TRUE);
 	 					}
  	 				}
 	 			 else if (i == LEVELOFF) {				/* Level */
 	 				draw = f1;
 	 				if (draw) {
	 					if (level == kConsoleNoMessages) level = saveLevel;
 	 					 else { saveLevel = level; level = kConsoleNoMessages; }
	 					f0 = f1 = f2 = f3 = f4 = f5 = f6 = first = FALSE;
	 					PaintDebugMask();
 	 					DrawDebugWindow(GetWindowPort(sayWindow),TRUE);
 	 					}
 	 				}
 	 			 else if (i==PAUSEOFF && level>kConsoleNoMessages) {		/* Pause */
 	 				draw = f2;
 	 			 	if (draw) {
 	 			 		f0 = f1 = f2 = f3 = f4 = f5 = f6 = first = FALSE;
 	 			 		pepsi = !pepsi; PaintDebugControls();
 	 			 		if (pepsi)
 	 			 			if ((TickCount()-lastScroll)/60 > 10) pauseCount = 0;
 	 			 		}
 	 			 	}
 	 			 else if (i==USERFUNCOFF && level>kConsoleNoMessages) {		/* User function */
 	 				draw = f3;
 	 			 	if (draw) {
 	 			 		if (!userFuncing && TrackClickBox(bitRect+USERFUNCOFF,(PaintFunc)PaintUserFunc)) {
 	 			 			userFuncing = TRUE; PaintDebugControls();
	 	 			 		if (userFunc) (*userFunc)(modifiers);
	 	 			 		 else {
	  	 			 			PrintFreeMem();
	  	 			 			DebugHeapDump();
	  	 			 			}
	 	 			 		userFuncing = FALSE; PaintDebugControls();
	 	 			 		}
	  	 			 	 else
	  	 			 		SysBeep(1);
 	 			 		f0 = f1 = f2 = f3 = f4 = f5 = f6 = first = FALSE;
 	 			 		}
 	 			 	}
 	 			 else if (i==LOGFILEOFF && level>kConsoleNoMessages) {		/* Log file */
 	 				draw = f4;
 	 			 	if (draw) {
 	 			 		if (modifiers & optionKey) SetDebugFile((char*)NIL);
 	 			 		if (*outputFile == 0) {
 	 			 			logFile = TRUE; PaintDebugControls();
 	 			 			logFile = GetDebugFile() && OpenDebugFile()==0;
 	 			 			}
 	 			 		 else
 	 			 			logFile = !logFile;
 	 			 		f0 = f1 = f2 = f3 = f4 = f5 = f6 = first = FALSE;
 	 			 		PaintDebugControls();
 	 			 		}
 	 			 	}
 	 			 else if (i==HELPOFF && level>kConsoleNoMessages) {		/* Help */
 	 				draw = f5;
 	 			 	if (draw) {
 	 			 		if (!helping) 
 	 			 			if (TrackClickBox(bitRect+HELPOFF,(PaintFunc)PaintHelp)) {
#ifdef ONHOLD
 "To see the next screenful of help, tap the SPACE BAR.\r\r",
#endif
		 			 				helping = TRUE; SetDebugPause(TRUE);
								for (j=0; preamble[j] && helping; j++)
									DebugString(preamble[j]);
 	 			 				for (j=0; help[j] && helping; j++)
 	 			 					DebugString(help[j]);
	 			 				}
	 			 		f0 = f1 = f2 = f3 = f4 = f5 = f6 = first = helping = FALSE;
 	 			 		PaintDebugControls();
 	 			 		}
 	 			 	}
 	 			 else if (i==SHOWOFF && level>kConsoleNoMessages) {		/* Update viewing */
 	 				draw = f6;
 	 			 	if (draw) {
 	 			 		if (TrackClickBox(bitRect+SHOWOFF,(PaintFunc)PaintShowing))
 	 			 			showing = !showing;
 	 			 		eraseUpdates = showing && (modifiers&optionKey)!=0;
	 			 		f0 = f1 = f2 = f3 = f4 = f5 = f6 = first = FALSE;
 	 			 		PaintDebugControls();
 	 			 		}
 	 			 	}
 	 			 else if (level >= kConsoleApplicationEvents) {
 	 			 	long shifter,bit;
 	 			 	shifter = i - MASKOFF;
 	 			 	if (shifter >= 9) shifter++;	/* Compensate for no #9 event */
 	 			 	bit = (1L << shifter);
 	 			 	if (first) {
 	 			 		reset = (debugMask & bit) != 0;
 	 			 		first = f0 = f1 = f2 = f3 = f4 = f5 = f6 = FALSE;
 	 			 		}
 	 				if (reset) {
 	 					draw = (debugMask & bit) != 0;
 	 					debugMask &= ~bit;
 	 					}
 	 				 else {
 	 				 	draw = (debugMask & bit) == 0;
 	 				 	debugMask |= bit;
 	 				 	}
 	 				if (draw) PaintMaskBit(i);
					}
	}

#ifdef POST

/*
 *	Post an event specified by which mask bit is pointed at.
 */

static void DoDebugPost(Point p,GrafPtr oldPort)
	{
		int i,code,key; long now; WindowPtr w; Rect *r;
		static char *lab[] = {
			" Null", " Mouse Down", " Mouse Up", " Key Down", " Key Up",
			"n AutoKey", "n Update", "n Insert Disk", "n Activate", "",
			" Network", " Driver", "n Application 1", "n Application 2",
			"n Application 3", "n Application 4" };
		
		
		if (level < kConsoleApplicationEvents) return;
		internalComment = TRUE;
		
 	 	for (i=MASKOFF+1; i<=MAXMASK; i++)
 	 	
 	 		if (PtInRect(p,bitRect+i)) {
 	 		
 	 			userEvent = TRUE;
 	 			code = i-MASKOFF; if (code >= 9) code++;
 	 			DebugPrintf("Posting a%s event:\n",lab[code]);
 	 			
 	 			if (code==updateEvt || code==activateEvt) {
 	 				DebugPrintf("Specify window%s with mouse...\n",
 	 					(code==updateEvt) ? " and rectangle" : "");
 	 				r = GetARect(&w);
 	 				if (postEvent.message = (long)w) {
 	 					if (code == updateEvt) {
 	 				 		if (EmptyRect(&r)) r = &w->portRect;
 	 				 		SetPort(w);
 	 				 		InvalRect(&r);
 	 				 		}
 	 				 	}
	 				 else {
 	 					DebugPrintf("Mouse must be pointing at a window\n");
 	 					SysBeep(4);
 	 					userEvent = FALSE;
 	 					}
 	 				}
 	 			 else {
 	 				DebugPrintf("Please position mouse and tap %s...\n",
 	 						(code==keyDown || code==keyUp || code==autoKey) ?
 	 									"key" : "SPACE");
					now = Ticks;
					while (!GetPauseEvent(keyDownMask,&postEvent,now)) ;
					key = postEvent.message & charCodeMask;
 		 			postEvent.what = code;
 		 			}
 	 			break;
				}
				
		internalComment = FALSE;
		SetPort(oldPort);
	}

/*
 *	Use mouse to specify a rectangle for window w
 */

Rect *GetARect(WindowPtr *w)
	{
		GrafPtr oldPort; static Rect r; PenState state; long now;
		Point p,q;
		
		now = Ticks;
	 	while (!GetPauseEvent(mDownMask,&postEvent,now)) ;
 	 	FindWindow(postEvent.where,w);
		SetRect(&r,0,0,0,0);

 	 	if (*w) {
			GetPort(&oldPort); SetPort(*w);
			GetPenState(&state);
			PenMode(srcXor);
			now = Ticks;
			p = postEvent.where; GlobalToLocal(&p);
			SetRect(&r,p.h,p.v,p.h,p.v);
			FrameRect(&r);
			while (!GetPauseEvent(mUpMask,&postEvent,now)) {
				GetMouse(&q);
				if (!EqualPt(&p,&q)) {
					FrameRect(&r);
					r.right = q.h; r.bottom = q.v;
					FrameRect(&r);
					}
				}
			FrameRect(&r);
			SetPenState(&state);
			SetPort(oldPort);
			}
		return(&r);
	}
#endif

/*
	Here we generate a report on an event, print it in the Debug window,
	and classify it as internal if it pertains to the Debug window itself.
	Printing may or may not occur according to the current message level,
	and whether the event is selected for in the current debug event mask.
	If it's an internal Debug window event, we may suppress the message
	or not, depending also on the current message output level.
 */

int DoDebugComment(EventRecord *event)
	{
		short part, what, i, internal;
		WindowPtr w;

		 /*
		  *	First classify the event according to its associated window.
		  *	If a mouse down event is in one of the four "corners", we classify
		  *	it with an internal part code.
		  */
		
		w = (WindowPtr)NIL; what = event->what;
		if (what==mouseDown || what==mouseUp) {
			part = FindWindow(event->where,&w);
			if (part==inMenuBar || part==inDesk)
				for (i=0; i<4; i++)
					if (PtInRect(event->where,corner+i))
						{ w = sayWindow; part = inCorner; break; }
			}
		 else if (what==activateEvt || what==updateEvt)
		 	w = (WindowPtr)(event->message);
		 
		/*
			In case we ever want to deal with the keyboard
			
		 else if (what==keyDown || what==autoKey || what==keyUp) {
		 	if (level > kConsoleNoMessages) w = FrontWindow();
			}
		*/

		internal = (w==sayWindow && what!=kHighLevelEvent);
		foundPart = part; foundWindow = w;
		
		 /*
		  *	Get the debug mask bit from the corresponding part number,
		  * and use it along with the current output level and internal
		  *	classification to either print out a message or not.
		  */

		internalComment = TRUE;		/* Special DebugPrintf() calls ahead */
		
		if (what == kHighLevelEvent) what = highLevelEvt;
		
		if (debugMask & (1L<<what))
		   if (level==kConsoleAllEvents || (level==kConsoleApplicationEvents && !internal))
		   		switch(what) {
					case keyUp:
					case autoKey:
					case keyDown:
						DebugKeyMsg(event);
						break;
					case mouseUp:
					case mouseDown:
						DebugMouseMsg(event,w,part);
						break;
					case activateEvt:
						DebugPrintf("%sctivating \"%W\"\n",
							(event->modifiers & activeFlag) ? "A" : "Dea",w);
						break;
					case updateEvt:
						DebugPrintf("Updating \"%W\"\n",w);
						break;
					case diskEvt:
						DebugPrintf("Disk in drive %d: result(%d)\n",
								LoWord(event->message),HiWord(event->message));
						break;
					case highLevelEvt:
						DebugPrintf("Receiving high level event: class=‘%T’ ID=‘%T’\n",
										event->message,event->where);
						PushAppleEventHandler(event);
						break;
					case driverEvt:
						DebugPrintf("Driver event (obsolete in Sys7)\n");
						break;
					case app1Evt:
					case app2Evt:
					case app3Evt:
						DebugPrintf("Application event %d: %ld = hex(%lx)\n",
									what-app1Evt+1,event->message,event->message);
						break;
					case osEvt:
						switch((event->message >> 24) & 0xFF) {
							case mouseMovedMessage:
								DebugPrintf("Mouse-moved event: message=0x%08lx\n",
										event->message);
								break;
							case suspendResumeMessage:
								DebugPrintf("%s event: message=0x%08lx",
									(event->message & resumeFlag) ? "Resume" : "Suspend",
										event->message);
								if (event->message & resumeFlag)
#ifdef CARBON_NOMORE
									if (event->message & convertClipboardFlag)
										DebugPrintf(" (clipboard changed)");
#else
									;
#endif
								OurNewLine();
								break;
							}
						break;
					default:
						DebugPrintf("? Unknown event.what = %d\n",what);
						break;
					}
					
		/* If we're supposed to be showing update regions, do it here (after message) */
		
		if (showing && what==updateEvt)
			if (internal) {
				if (level == kConsoleAllEvents)
					FillUpdateRgn(sayWindow);
				}
			 else
				FillUpdateRgn(w);

		internalComment = FALSE;
		return(internal);
	}


/*
 *	When a high-level Apple event comes in, we record its handler from the
 *	application dispatch table in a temporary variable, and then install
 *	our own handler, which will be called by AEProcessEvent().  Our own
 *	handler writes event information out, re-installs the old handler, and
 *	calls the old handler.
 */

static void PushAppleEventHandler(EventRecord *evt)
	{
		char *hType = (char *)NIL;
		evt = evt;
		
#ifdef APPLE_EVENTS
		OSErr err;
		if (appleEventsOK) {
			theHandlerClass = (ResType)evt->message;
			theHandlerID    = *(ResType *) (&evt->where);
			err = AEGetEventHandler(theHandlerClass,theHandlerID,(EventHandlerProcPtr)&OldHandler,&theHandlerRefcon,theHandlerHeap=FALSE);
			if (err == errAEHandlerNotFound) {
				err = AEGetEventHandler(theHandlerClass,theHandlerID,(EventHandlerProcPtr)&OldHandler,&theHandlerRefcon,theHandlerHeap=TRUE);
				if (err == errAEHandlerNotFound)
					hType = "No handler found";
				 else
					hType = "Calling system handler";
				}
			 else
				hType = "Calling application handler";
			
			DebugPrintf("%s for this Apple event\r",hType);

			if (err == noErr)	{
				/* Insert our own handler temporarily as an application heap handler */
				err = AEInstallEventHandler(theHandlerClass,theHandlerID,DebugReportAppleEvent,theHandlerRefcon,FALSE);
				if (err) OldHandler = NIL;
				}
			 else
				OldHandler = NIL;
			}
#endif
	}

#ifdef APPLE_EVENTS
static short aeLevel;

static void IndentLevel(int level)
	{
		/* Tab over indent level */
		while (level-- > 0) DebugPrintf("  ");
	}

static int IsPrintable(Byte ch)
	{
		return(ch>=' ' && ch<=126);
	}
	
/*
 *	Examine a given Apple event, print information about it, re-install old handler,
 *	if any, and call it, delivering its error code.
 */

pascal OSErr DebugReportAppleEvent(AppleEvent *appleEvent, AppleEvent *reply, long refcon)
	{
		OSErr err = noErr, hErr, once;
		AEDesc aDesc; long size,i;
		static DescType knownAttributeKeys[] = {
				keyTransactionIDAttr,
				keyReturnIDAttr,
				keyEventClassAttr,
				keyEventIDAttr,
				keyAddressAttr,
				keyOptionalKeywordAttr,
				keyTimeoutAttr,
				keyInteractLevelAttr,
				keyEventSourceAttr,
				keyMissedKeywordAttr
				};
		static DescType knownParameterKeys[] = {
				keyDirectObject,
				keyErrorNumber,
				keyErrorString,
				keyProcessSerialNumber,
				keyPreDispatch,
				keySelectProc
				};
		
		if (!appleEventsOK) return(err);
		
		/* See which keys are defined, and report their values if possible */
		
		size = sizeof(knownAttributeKeys) / sizeof(DescType);
		for (i=0,once=TRUE; i<size; i++) {
			err = AEGetKeyDesc(appleEvent,knownAttributeKeys[i],typeWildCard,&aDesc);
			if (err == noErr) {
				if (once) { say("Attributes:\r"); once = FALSE; }
				say("  key=‘%T’:\r",knownAttributeKeys[i]);
				PrintDescriptor(&aDesc,TRUE,0);
				AEDisposeDesc(&aDesc);
				}
			}
		
		size = sizeof(knownParameterKeys) / sizeof(DescType);
		for (i=0,once=TRUE; i<size; i++) {
			err = AEGetKeyDesc(appleEvent,knownParameterKeys[i],typeWildCard,&aDesc);
			if (err == noErr) {
				if (once) { say("Parameters:\r"); once = FALSE; }
				say("  key=‘%T’:\r",knownParameterKeys[i]);
				PrintDescriptor(&aDesc,TRUE,0);
				AEDisposeDesc(&aDesc);
				}
			}
		
		if (OldHandler) {
			/* Take ourselves out as temporary application heap handler */
			hErr = AERemoveEventHandler(theHandlerClass,theHandlerID,DebugReportAppleEvent,FALSE);
			/* Re-install old handler, and call it and return its error code */
			hErr = AEInstallEventHandler(theHandlerClass,theHandlerID,OldHandler,theHandlerRefcon,theHandlerHeap);
			err = OldHandler(appleEvent,reply,refcon);
			}
		
		return(err);
	}

/*
 *	Recursively traverse an AEDesc, printing out its entire structure, indented
 */

static void PrintDescriptor(AEDesc *theDesc, int doType, long index)
	{
		long i,itemCount,size; OSErr err;
		int addReturn = TRUE; long data;
		DescType class,ID,target,returnType;
		AEDescList theList;
		
		aeLevel++;
		IndentLevel(aeLevel);
		
		if (index) DebugPrintf("%ld. ",index);
		if (doType) DebugPrintf("‘%T’",theDesc->descriptorType);

		switch(theDesc->descriptorType) {

			case typeAERecord:
			case typeAEList:
				err = AECountItems(theDesc,&itemCount);
				if (err == noErr) {
					DebugPrintf(" %ld item%s\r",itemCount,plural(itemCount));
					addReturn = FALSE;
					for (i=1; i<=itemCount; i++) {
						AEDesc theItem; AEKeyword keyword;
						err = AEGetNthDesc(theDesc,i,typeWildCard,&keyword,&theItem);
						if (err == noErr) {
							PrintDescriptor(&theItem,TRUE,i);
							AEDisposeDesc(&theItem);
							}
						}
					}
				break;
			
			case 'aevt':
				break;
				
			case typeBoolean:
			case typeChar:
			case typeLongInteger:
			case typeShortInteger:
			case typeShortFloat:
			case typeLongFloat:
			case typeExtended:
			case typeComp:
			case typeMagnitude:
				break;
				
			case typeTrue:
			case typeFalse:
				break;
			case typeAlias:
			case typeFSS:
				break;
			case typeEnumerated:
				break;
				
			case typeType:
				break;
			
			case typeAppParameters:
			case typeProperty:
			case typeKeyword:
			case typeSectionH:
			case typeWildCard:
			case typeApplSignature:
			case typeSessionID:
			case typeTargetID:
			case typeProcessSerialNumber:
			case typeNull:
				break;
				
			default:
				break;
			}
		
		if (addReturn)
			OurNewLine();
		
		aeLevel--;
	}

#endif APPLE_EVENTS

/*
	Print a message describing a key event, including which key
	was pressed and what the current associated character code was.
*/

static void DebugKeyMsg(EventRecord *event)		/* Print message about key event */
	{
		const char *wh, *mo, *mcl, *ms, *mc, *mb;
		int modBits,ch;
		
		modBits = event->modifiers;
		ch = event->message & charCodeMask;
		if (event->what == keyDown) wh = "Key down";
		 else if (event->what == keyUp) wh = "Key up";
		 else wh = "Autokey";
		DebugPrintf((char*)((ch>=' ' & ch<='~') ? "%s:'%c'" : "%s:   "),wh,ch);
		DebugPrintf(" char($%x=%d) key($%lx)",ch,ch,
								(event->message & keyCodeMask)>>8);
		mo = (modBits & optionKey) ? " Option" : "";
		mcl= (modBits & alphaLock) ? " CapsLock" : "";
		ms = (modBits & shiftKey)  ? " Shift" : "";
		mc = (modBits & cmdKey)    ? " Command" : "";
		mb = (!(modBits & btnState))  ? " ButtonDown" : "";
		DebugPrintf("%s%s%s%s%s\n",mc,mo,ms,mcl,mb);		
	}

/*
	This routine is just to make the switch() statement in
	DoDebugComment() more readable.  Here we format and print out a
	message about a mouse event.  If the event occurred in the Content
	or Grow regions of a window, the mouse position is translated into
	the local coordinate system before being printed.  We also include
	the title of the applicable window, if there was one.
*/

static void DebugMouseMsg(EventRecord *event, WindowPtr w, int part)	
	{
		const char *partStr,*upDownStr;
		GrafPtr oldPort; Point p;
		static char *parts[] =
			{ "Desk Top", "Menu Bar", "System", "Content", "Drag Bar",
			  "Grow Box", "Go Away", "Zoom In", "Zoom Out" };
		
		upDownStr = (event->what == mouseUp) ? "Mouse up" : "Mouse down";
		partStr = (part>=inDesk && part<=inZoomOut) ? parts[part] :
					(part == inCorner) ? w=(WindowPtr)NIL,"Corner" : "????";
		p = event->where;
		if (part==inContent || part==inGrow) {
			GetPort(&oldPort); SetPort(GetWindowPort(w)); GlobalToLocal(&p); SetPort(oldPort);
			}
		if (w == NIL)
			DebugPrintf("%s: (%d,%d) in %s\n",upDownStr,p.h,p.v,partStr);
		 else
			DebugPrintf("%s: (%d,%d) %s \"%W\"\n",upDownStr,p.h,p.v,
															partStr,w);
	}

/*
 *	For a given window about to be updated, blacken its update region
 *	for just a short time.  This has to do the same stuff as BeginUpdate/
 *	EndUpdate, except that it doesn't empty the update region.
 */

static void FillUpdateRgn(WindowPtr w)
	{
		GrafPtr oldPort; long soon;
		RgnHandle updateRgn=NewRgn();
		PenState pen; Point pt;
		
		GetPort(&oldPort); SetPort(GetWindowPort(w));
		pt.h = pt.v = 0;
		LocalToGlobal(&pt);		
		GetWindowRegion(w, kWindowUpdateRgn, updateRgn);
		OffsetRgn(updateRgn,-pt.h,-pt.v);
		GetPenState(&pen); PenBlack();
		PaintRgn(updateRgn);
		/* Wait a half second or until Shift key is no longer down */
		soon = TickCount() + 30;
		while (TickCount()<soon || ShiftKeyDown()) ;
		if (eraseUpdates) EraseRgn(updateRgn);
		OffsetRgn(updateRgn,pt.h,pt.v);
		SetPenState(&pen);
		
#ifdef NOMORE		
		if (updateRgn = NewRgn()) {

			/* Save window's update region */
			CopyRgn(((WindowPeek)w)->updateRgn,updateRgn);
			
			/* Paint the update region black */
			GetPenState(&pen); PenBlack();
			BeginUpdate(w);
			PaintRect(&w->portRect);
			
			/* Wait a half second */
			soon = TickCount() + 30;
			while (TickCount() < soon) ;
			
			if (eraseUpdates) EraseRect(&w->portRect);
			
			EndUpdate(w);
			SetPenState(&pen);
			
			/* Restore the old update region */
			if (((WindowPeek)w)->updateRgn)
				CopyRgn(updateRgn,((WindowPeek)w)->updateRgn);
			DisposeRgn(updateRgn);
			}
#endif
		if (updateRgn!=(RgnHandle)NULL)
			DisposeRgn(updateRgn);
		SetPort(oldPort);
	}

/********************************************************************/
/*						HEAP DUMPER ROUTINES						*/
/********************************************************************/

void DebugHeapDump()
	{
//		if (MacJmp) {
//			DebugStr("\p; hc");
//			}
//		 else
			say("No Debugger Installed\r");
	}

#ifdef HEAP_DISPLAY

/*
 *	These routines traverse the current heap zone and print its map
 */

#define MasterPtr long
/* typedef long MasterPtr; */

typedef struct {
				unsigned long  tag :  8,
							  size : 24;
				long offset;
				int data;
				} BlockHeader;

typedef struct {
				unsigned typ : 2;		/* 0 for free; 1 for pointer; 2 for handle */
				unsigned skp : 2;		/* Unused */
				unsigned cor : 4;		/* Size correction */
				unsigned lck : 1;		/* locked handle */
				unsigned prg : 1;		/* Purgeable handle */
				unsigned res : 1;		/* Resource handle */
				unsigned mbk : 1;		/* This is a Master Ptr block */
				unsigned char tag;		/* Tag byte */
				long size;				/* Logical size */
				void *adr;				/* Address of block start */
				MasterPtr mptr;			/* Master pointer if handle */
				} BlockInfo;

enum { freeType=0, pointerType, handleType };

#define isHandle(b)		( ((b)->tag & 0xC0) == 0x80 )
#define isPointer(b)	( ((b)->tag & 0xC0) == 0x40 )
#define isFree(b)		( ((b)->tag & 0xC0) == 0 )
#define physicalSize(b)	(  (b)->size )
#define correction(b)	(  (b)->tag & 0x0F )
#define logicalSize(b)	( physicalSize(b) - correction(b) - 8)

#define isLocked(mp)	( (((long)mp) & (1L<<31)) != 0 )
#define isPurgeable(mp)	( (((long)mp) & (1L<<30)) != 0 )
#define isResource(mp)	( (((long)mp) & (1L<<29)) != 0 )


char	*FindHType(MasterPtr mptr);
char	*FindPType(WindowPeek w);
char	*ResString(MasterPtr mptr);


void DebugHeapDump()
	{
		if (MacJmp) {
			DebugStr("\p; hc");
			}
		 else
			say("No Debugger Installed\r");
		
#ifdef ONHOLD
		THz zone; MasterPtr p,q;
		long size,hCount,pCount,fCount,count,i,nH,nP,nF;
		register BlockHeader *block; register int *mptr;
		BlockInfo **snapshot; register BlockInfo *b,*bb,*btmp;
		register long offset,j;
		BlockInfo tmp;
		char *hex = "0123456789abcdef",*name;
		static BlockInfo zeroInfo;
		
		say("Current Heap Zone:\n  Header Information\n");
		say("Not implemented\n");

		zone = GetZone();
		
		size = zone->bkLim - (Ptr)zone + 12;
		say("\tTotal Size: %ld = 0x%lX bytes\n",size,size);
		
		/* Traverse the Master Pointer free list to find out how many are left */
		
		count = 0;
		p = (MasterPtr)zone->hFstFree;
		while (p) {
			count++;
			q = (MasterPtr) (((long)p) & 0x00FFFFFF);
			p = *(MasterPtr *)q;
			}
		say("\t%ld free Master Pointer%s\n",(long)count,plural(count));
		
		say("\t%ld free byte%s\n",(long)zone->zcbFree,plural(zone->zcbFree));
		
		say("\t%sGrow Zone Procedure\n",zone->gzProc ? "Has " : "No ");
		say("\t%sPurge Warning Procedure\n",zone->purgeProc ? "Has " : "No ");
		
		say("\tAllocates %ld master pointer%s at a time\n\n",
									(long)zone->moreMast,plural(zone->moreMast));
		
		/* Now traverse blocks for a roundup of information */
		
		block = (BlockHeader *)(&zone->heapData);
		hCount = pCount = fCount = 0;
		while (block < (BlockHeader *)zone->bkLim) {
			if (isHandle(block))		hCount++;
			 else if (isPointer(block)) pCount++;
			 else						fCount++;
			block = (BlockHeader *) (((char *)block) + physicalSize(block));
			}
		
		/*
		 *	Allocate a relocatable block to hold snapshot of heap.  The snapshot
		 *	is an array of BlockInfo's with an extra slot since we don't know
		 *	whether the Memory manager adds an extra block or not when it allocates
		 *	the snapshot.
		 */
		
		count = hCount + pCount + fCount + 1;
		snapshot = (BlockInfo **)NewHandle((Size)count*sizeof(BlockInfo));
		if (MemError()) {
			say("Sorry, can't allocate heap snapshot\n");
			return;
			}
		
		/* Fill snapshot with info about all blocks in heap by traversing again */
		
		b = *snapshot;		/* No Toolbox or other calls during loop */
		i = count;
		while (i-- > 0) *b++ = zeroInfo;
		
		b = *snapshot;
		block = (BlockHeader *)(&zone->heapData);
		i = hCount = pCount = fCount = 0;
		while (i<count && block<(BlockHeader *)zone->bkLim) {
			b->tag = block->tag;
			b->size = logicalSize(block);
			b->cor = correction(block);
			b->adr = &block->data;
			if (isHandle(block)) {
				p = b->mptr = (MasterPtr) (((char *)zone) + block->offset);
				b->typ = handleType;
				b->lck = isLocked(*(long *)p);
				b->prg = isPurgeable(*(long *)p);
				b->res = isResource(*(long *)p);
				hCount++;
				}
			 else if (isPointer(block)) {
			 	b->typ = pointerType;
			 	pCount++;
			 	}
			 else {
				b->typ = freeType;
				fCount++;
				}
			block = (BlockHeader *) (((char *)block) + physicalSize(block));
			b++;
			i++;
			}
		
		count = i;
		
		/* Now mark all Pointer blocks in which a Master Pointer can be found */
		
		for (i=0,b=*snapshot; i<count; i++,b++)		/* For each handle in snapshot */
			if (isHandle(b)) {
				mptr = (int *)b->mptr;
				for (j=0,bb=*snapshot; j<count-1; j++,bb++)		/* Search heap for ptr blocks */
					if (bb->typ == pointerType)
						if (bb->adr<=mptr && mptr<(bb+1)->adr) {
							bb->mbk = TRUE;
							break;
							}
				}
		
		/* Got snapshot.  Now it's okay to use toolbox to display all blocks. */
		
		say("Locke%-16s    Logical size\n","d or non-relocatable");
		say("| Pur%-16s        | Correction  Handle's\n","geable");
		say("| |  %-16s        | |  Data     Master Ptr\n","Description");
		say("| |  %-16s        | |  |        |\n\n","|");

		offset = nH = nP = nF = 0;
		for (i=0; i<count; i++) {
			tmp = (*snapshot)[i];
			switch(tmp.typ) {
				case handleType:
					name = tmp.mptr == (MasterPtr)snapshot ? "(this snapshot)" :
						   tmp.mptr == (MasterPtr)offscreenBits ? "(offscreen bits)" :
						   tmp.mptr == (MasterPtr)TopMapHndl ? "TopMapHndl" :
						   tmp.mptr == (MasterPtr)MenuList ? "MenuList" :
						   tmp.mptr == (MasterPtr)TEScrpHandle ? "TextEdit Scrap" :
						   tmp.res ? ResString(tmp.mptr) : FindHType(tmp.mptr);
					say("%c %c  %-16s %8lx%c%c @%5lx <- %lx\n",
										tmp.lck ? '•' : ' ',
										tmp.prg ? 'P' : ' ',
										name,
										tmp.size,
										tmp.cor ? '+' : ' ',
										tmp.cor ? hex[tmp.cor] : ' ',
										tmp.adr,
										tmp.mptr);
					break;
				case pointerType:
					name = tmp.mbk ? "Master Pointers" : FindPType((WindowPeek)tmp.adr);
			 		say("•    %-16s %8lx%c%c @%5lx\n",
			 							name,
			 							tmp.size,
 										tmp.cor ? '+' : ' ',
 										tmp.cor ? hex[tmp.cor] : ' ',
 										tmp.adr);
			 		break;
			 	case freeType:
					say("     %-16s %8lx%c%c @%5lx\n",
										"----",
										tmp.size,
										tmp.cor ? '+' : ' ',
										tmp.cor ? hex[tmp.cor] : ' ',
										tmp.adr);
					break;
				}
			}
		
		say("\n\t%ld Handles, %ld Pointers, and %ld Free blocks\n",hCount,pCount,fCount);
		
		/* Finished with our snapshot, return it to memory */
		
		DisposHandle(snapshot);
#endif
	}

typedef struct {
		char header[16];
		Handle nextMap;
		int fileRef;
		int fileAttr;
		int typesOffset;
		int namesOffset;
		} ResMapHeader;

typedef struct {
		int id;
		int offset;
		unsigned long attr : 8,
					  lenOffset : 24;
		MasterPtr rsrc;
		} ResRef;

/*
 *	Traverse all open resource maps in the system's list of open resource file maps
 *	searching for a resource whose Handle matches the given handle.
 */

static char *ResString(MasterPtr mptr)
	{
		static char str[32]; Handle nextMap; char tmp;
		register char *p,*q; char *typesList; ResMapHeader *hdr; register ResRef *ref;
		register int nTypes,nRefs,id; ResType type;
		
		if (TopMapHndl == NIL) return("HANDLE\t");
		
		nextMap = TopMapHndl;
		while (nextMap) {
		
			p = (char *)(*nextMap);
			hdr = (ResMapHeader *)p;
			
			p += hdr->typesOffset;
			typesList = p;				/* Beginning of types list */
			
			nTypes = *(unsigned int *)p; p += sizeof(int);
			while (nTypes-- >= 0) {
				type = *(ResType *)p; p += sizeof(ResType);
				nRefs = *(unsigned int *)p; p += sizeof(int);
				q = typesList + *(unsigned int *)p; p += sizeof(int);
				while (nRefs-- >= 0) {
					ref = (ResRef *)q; q += sizeof(ResRef);
					if (ref->rsrc == mptr) {
						*(ResType *)str = type;
						str[4] = str[3]; str[3] = str[2]; str[2] = str[1]; str[1] = str[0];
						str[0] = str[5] = '\'';
						p = str + 6;
						*p++ = ' ';
						id = ref->id;
						if (id < 0) { *p++ = '-'; id = -id; }
						q = p;
						do { *p++ = "0123456789"[id%10]; id /= 10; } while (id);
						*p-- = '\0';
						while (p > q) { tmp = *p; *p-- = *q; *q++ = tmp; }
						while (*q) q++;
						*q++ = ' ';
						*q++ = '[';
						id = hdr->fileRef;
						if (id < 0) { *p++ = '-'; id = -id; }
						p = q;
						do { *q++ = "0123456789"[id%10]; id /=10; } while (id);
						*q-- = '\0';
						while (q > p) { tmp = *q; *q-- = *p; *p++ = tmp; }
						while (*p) p++;
						*p++ = ']';
						*p = '\0';
						return(str);
						}
					}
				}
			nextMap = hdr->nextMap;
			}
		
		return("Resource");
	}

static char *FindPType(register WindowPeek wp)
	{
		register WindowPeek w;
		
		if (wp == (WindowPeek)sayPort.portBits.baseAddr) return("(bitmap bits)");
		
		w = (WindowPeek)WindowList;
		while (w) {
			if (w == wp) return("Window");
			w = w->nextWindow;
			}
		return("ptr");
	}

/*
 *	Given a handle, search through all windows to see if it matches
 *	any handles in the GrafPorts or WindowRecords;
 */

static char *FindHType(register MasterPtr mptr)
	{
		register WindowPtr w; register WindowPeek wp;
		
		wp = WindowList;
		while (wp) {
			if (mptr == (MasterPtr)wp->strucRgn) return("strucRgn");
			if (mptr == (MasterPtr)wp->contRgn) return("contRgn");
			if (mptr == (MasterPtr)wp->updateRgn) return("updateRgn");
			if (mptr == (MasterPtr)wp->windowDefProc) return("windowDefProc");
			if (mptr == (MasterPtr)wp->dataHandle) return("dataHandle");
			if (mptr == (MasterPtr)wp->titleHandle) return("titleHandle");
			if (mptr == (MasterPtr)wp->controlList) return("controlList");
			if (mptr == (MasterPtr)wp->windowPic) return("windowPic");
			w = (WindowPtr)w;
			if (mptr == (MasterPtr)w->visRgn) return("visRgn");
			if (mptr == (MasterPtr)w->clipRgn) return("clipRgn");
			if (mptr == (MasterPtr)w->picSave) return("picSave");
			if (mptr == (MasterPtr)w->rgnSave) return("rgnSave");
			if (mptr == (MasterPtr)w->polySave) return("polySave");
			wp = wp->nextWindow;
			}
		return("HANDLE");
	}

#endif // HEAP_DISPLAY
