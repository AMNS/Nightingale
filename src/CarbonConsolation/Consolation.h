/*
 *	Consolation.h
 *	Version 2.0a1		November 16, 1994
 *
 *	© (Copyright) 1986-1995
 *	Mathemaesthetics, Inc.
 *	PO Box 298 • Boulder • CO • 80306
 *	All rights reserved
 *
 *	This header should be included in any source file that wishes to use
 *	the Consolation library to report on events delivered by any explicit
 *	calls to GetNextEvent() or WaitNextEvent() in the same source file.
 */

#ifndef _Consolation_
#define _Consolation_

// MAS
#include "Nightingale_Prefix.pch"
// MAS

/*
 *	The method of installing Consolation into your application is to include
 *	the library in your project, and include this header file in the file with
 *	your main event loop in it.  Recompile and run the project.  In most cases,
 *	this is all you have to do.  However, if your code assumes that all windows
 *	in the current window list have been created by your application, there
 *	may be problems that you can work around by using various of the other
 *	routines in the Consolation library.
 *
 *	The library essentially inserts itself as an event filter in any event
 *	loop between your code and the system.  Rather than patching system traps,
 *	the installation is done symbolically at compile time with the following
 *	two macros.
 *
 *	These two definitions will cause all instances of the routines GetNextEvent()
 *	or WaitNextEvent() to be replaced with equivalent calls to the library
 *	routines, GetDebugEvent() or WaitDebugEvent(), as long as you're including
 *	this file for debugging.  Note that this does not patch the trap directly;
 *	it just places some filtering routines between your application and the
 *	toolbox calls it normally relies on.  This gives you more control over when
 *	to involve the debugging window, and keeps it out of system-related event
 *	loops (such is within a call to ModalDialog).
 *
 *	Since the Consolation library uses this header file also, we must not
 *	make these definitions internally when we compile the library.  Therefore,
 *	library source code should always define _CompilingConsolationLibraryOnly_
 *	before including this file, and user's application source code should
 *	never have this symbol defined.
 */

#ifndef _CompilingConsolationLibraryOnly_

//	#define GetNextEvent	GetDebugEvent
//	#define WaitNextEvent	WaitDebugEvent

#endif

/*
 *	You may want to make the following definition, to save time and space
 *	when adding debug printing statements to your code:
 *
 *	#define say		DebugPrintf
 *
 *	Now you can write:	say("val = %d\r",n);
 *	instead of:			DebugPrintf("val = %d\r",n);
 */

#ifndef say

	#define say		DebugPrintf
	
#endif

/*
 *	When the Consolation window is created, the constant kConsolationKind is
 *	placed into the window's windowKind field.  Assuming no other window has this
 *	value in its windowKind field, you can use it to distinguish the Consolation
 *	window from any other of your own windows.
 *
 *	The Consolation library makes *no* use of this number for itself, so if the
 *	constant conflicts with some application windowKind that you are using, you can
 *	put some other more appropriate value into the windowKind field by redefining
 *	the constant below.
 *
 *	The Consolation library also makes *no* use of the window's refCon field either,
 *	so that if the windowKind field can't be used, you may be able to use the refCon
 *	field instead.  Just change the isConsolationKind() macro here to reflect whatever
 *	method you are using.
 *
 *	For more robust checking, use the isConsolationWindow() macro.
 */

#define kConsolationKind		(userKind)				// userKind == 8
#define isConsolationKind(w)	(((WindowPeek)w)->windowKind == kConsolationKind)
#define isConsolationWindow(w)  ((w)!=NIL && isConsolationKind(w))


/*
 *	Possible Consolation window output levels, which are passed to SetDebugLevel()
 */

enum {
	kConsoleNoMessages = 0,
	kConsoleOutputOnly,
	kConsoleApplicationEvents,
	kConsoleAllEvents
	};

/*
 *	The Mac header file, <Events.h>, defines “highLevelEventMask” as 1024, which means
 *	that it would correspond to an event.what value of 10, which used to be “networkEvt”.
 *	However, this is now obsolete; but there currently is no replacement constant.
 *	So we brew our own if needed.  You can use highLevelEventMask when building an
 *	argument to SetDebugMask().
 */

#ifndef highLevelEventMask
	#define highLevelEventMask	1024
#endif
#ifndef highLevelEvt
	#define highLevelEvt	10		// Same as old networkEvt, now obsolete
#endif

/*
 *	Prototypes for public functions in Consolation library available to your code.
 */

typedef void (*DebugUserFunc)(short modifiers);
typedef void (*DebugPrintFunc)(void *data, short width, short precision, short alternate);

int				GetDebugEvent(short mask, EventRecord *evt);
int				WaitDebugEvent(short mask, EventRecord *evt, long ticks, RgnHandle rgn);

void			DebugPrintf(char *str, ...);				// Enhanced Mac printf routine

void			LoadConsolation(void);						// For segment loading (does nothing)
void			InitDebugWindow(Rect *rect, int visible);
WindowPtr		DebugWindow(void);							// of Consolation's window
WindowPtr		DebugRealWindow(void);

short			SetDebugLevel(short newLevel);				// Returns old level
unsigned long	SetDebugMask(unsigned long newMask);		// Returns old mask
short		 	SetDebugPause(short newPause);				// Returns old pause value
void			SetDebugMarker(char *markerStr);			// C string
OSErr			SetDebugFile(char *name);					// C string
DebugUserFunc	SetDebugUserFunc(DebugUserFunc func);		// Returns old function
DebugPrintFunc 	SetDebugPrintFunc(DebugPrintFunc func);		// Returns old function

void			DebugNoBitmap(void);
void			DebugSleep(short n);
void			DebugErase(void);
void			DoDebugUpdate(void);

void			ShowHideDebugWindow(int how);

#if 0
#ifdef DebugPrintf

#undef DebugPrintf

//#define DebugPrintf printf

#endif

#define DebugPrintf printf
#endif

#endif		// _Consolation_
