/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1986-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/*
 * 						  (*)   N I G H T I N G A L E ®   (*)
 * 
 *	Donald Byrd, Charles Rose, Douglas McKenna, Dave Winzler, and John Gibson
 *							Advanced Music Notation Systems, Inc.
 *                					Version 3.1
 * 									May 1986-Sept. 1997
 *
 * Also incorporates OMS code by Ken Brockman, a very small amount of code by each of
 * Ray Spears and Jim Carr, and a help system by Joe Pillera.
 *
 *	Thanks to Bill Brinkley, Mike Brockman, Bill Buxton, John Davis, Ric Ford,
 *	John Gibson, David Gottlieb, Doug Hofstadter, Scott Kim, Jan Knoefel, Paul Lansky,
 *	Katharine McKenna, Malcolm and Priscilla McKenna, Scott McCormick, Jef Raskin, Steve
 * Sacco, Paul Sadowski, Kim Stickney, and Christopher Yavelow. Special thanks to John
 * Maxwell and Severo Ornstein, without whom I could not have imagined this program. And
 * very special thanks to my father, Eugene Byrd, and my wife, Susan Schneider, without
 * whom it certainly would never have been finished...
 *
 *	Nightingale is very loosely based on several earlier programs. The approach owes
 *	a great deal to John Maxwell and Severo Ornstein's Mockingbird; to the Macintosh
 * standard user interface and its implementation in various earlier programs; and, to
 *	a lesser extent, Donald Byrd's SMUT. A few of the algorithms are from SMUT, as doc-
 *	umented in Byrd's dissertation "Music Notation by Computer" (Indiana University,
 *	1984; UMI number 8506091).
 */


#include "Nightingale_Prefix.pch"
#define MAIN                    /* Global variables will be allocated here */
#include "Nightingale.appl.h"

#ifdef USE_PROFILER
#include <profiler.h>
#endif

static void		FileStartup(void);


/* Main routine and entry point for application */

int main()
{
#ifndef PUBLIC_VERSION
		if (CapsLockKeyDown() && ShiftKeyDown() && OptionKeyDown())
			DebugStr("\pBREAK AT MAIN");
#endif
#ifdef USE_PROFILER
		if (ProfilerInit(collectDetailed, bestTimeBase,
						/*numfuncs*/ 500, /*stackdepth*/ 20) != noErr)
			return 0;
		ProfilerSetStatus(FALSE);	/* Turn it off until later */
#endif
		Initialize();
#ifndef TARGET_API_MAC_CARBON
		UnloadSeg(&Initialize);		/* Reclaim space, since it won't be called again */
#endif
		FileStartup();
		
		while (DoEvent()) ;			/* Life until event-ual death */

		Finalize();

#ifdef USE_PROFILER
		if (ProfilerDump("\pniteprofile") != noErr)
			SysBeep(20);
		ProfilerTerm();
#endif
		return 0;
}

/* Do the standard startup action after initializing.  We don't do this in Initialize()
since FileStartup usually allocates non-relocatable memory and loads other segments,
which means that the UnloadSeg(Initialize) above can leave a hole in the heap.
If Apple events are available, then they will run the application startup stuff from
the handlers. */

static void FileStartup()
	{
		if (!appleEventsOK) {
			/*
			 *	If we're launching due to the user double-clicking on a file, open
			 *	each document file in selection; otherwise, simulate the standard
			 *	Open command.
			 */
		
			AnalyzeWindows();
			}
		DoOpenApplication(!appleEventsOK);
	}

void DoOpenApplication(Boolean askForFile)
	{
		if (askForFile)
			DoFileMenu(FM_Open);
		
		AnalyzeWindows();
#ifndef VIEWER_VERSION
		if (TopDocument)
			DoViewMenu(VM_SymbolPalette);
#endif
	}
