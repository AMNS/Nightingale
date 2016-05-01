/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/*
 * 						  (*)   N I G H T I N G A L E ®   (*)
 * 
 *	by Donald Byrd, Charles Rose, Douglas McKenna, Dave Winzler, and John Gibson
 *							Advanced Music Notation Systems, Inc.
 *							   Adept Music Notation Solutions
 *							   Avian Music Notation Foundation
 *                					
 *
 * Nightingale also incorporates OMS code by Ken Brockman, a help system by Joe Pillera,
 * and work by Jim Carr, Geoff Chirgwin, Michel Salim, and Ray Spears.
 *
 * Thanks to Bill Brinkley, Mike Brockman, Bill Buxton, John Davis, Ric Ford,
 * John Gibson, David Gottlieb, Doug Hofstadter, Scott Kim, Jan Knoefel, Paul Lansky,
 * Katharine McKenna, Malcolm and Priscilla McKenna, Scott McCormick, Jef Raskin, Steve
 * Sacco, Paul Sadowski, Kim Stickney, and Christopher Yavelow. Special thanks to John
 * Maxwell and Severo Ornstein, without whom I could not have imagined this program. And
 * very special thanks to my father, Eugene Byrd, and my wife, Susan Schneider, without
 * whom version 1.0 certainly would never have existed...
 *
 * Nightingale is very loosely based on several earlier programs. The approach owes
 * a great deal to John Maxwell and Severo Ornstein's Mockingbird; to the Macintosh
 * standard user interface and its implementation in various earlier programs; and, to
 * a lesser extent, Donald Byrd's SMUT. A few of the algorithms are from SMUT, as doc-
 * umented in Byrd's dissertation "Music Notation by Computer" (Indiana University,
 * 1984; UMI number 8506091).
 */


#include "Nightingale_Prefix.pch"
#define MAIN                    /* Global variables will be allocated here */
#include "Nightingale.appl.h"


static void		FileStartup(void);


/* Main routine and entry point for application */

int main()
{
#ifndef PUBLIC_VERSION
		if (CapsLockKeyDown() && ShiftKeyDown() && OptionKeyDown())
			DebugStr("\pBREAK AT MAIN");
#endif
		Initialize();
		FileStartup();
		
		while (DoEvent()) ;			/* Life until event-ual death */

		Finalize();

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
		if (TopDocument)
			DoViewMenu(VM_SymbolPalette);
	}
