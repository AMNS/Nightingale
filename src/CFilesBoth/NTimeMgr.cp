/* NTimeMgr.c - simple Nightingale Time Manager package - D. Byrd, AMNS, 2/96 */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 *
 * Copyright © 1996 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

/* This code is heavily Macintosh-dependent. More specifically, it requires the
extended (System 7) version of the Time Manager! Before calling these routines,
you should check that the extended Time Manager is available with something like:
		Gestalt(gestaltTimeMgrVersion, &gestaltAnswer);
gestaltAnswer = 3 (gestaltExtendedTimeMgr) for the extended Time Manager. */

// MAS we are targeting Mach-O
//#if !TARGET_API_MAC_CARBON_MACHO
//#include <Timer.h>
//#endif
// MAS include Carbon
#include <Carbon/Carbon.h>
// MAS
#include "NTimeMgr.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct {						/* Time Manager information record */
	TMTask	atmTask;					/* original and revised TMTask record */
	long		tmRefCon;				/* space to pass address of A5 world */
} TMInfo;

typedef TMInfo *TMInfoPtr;

static TMInfo theTMInfo;			/* Our TimeMgr information record */
static long theDelay;				/* No. of milliseconds to delay between "ticks" */
static long theCount;				/* Current timer value */
static Boolean timerRunning;

//#if TARGET_CPU_PPC
//#else
//pascal TMInfoPtr GetTMInfo(void)	= 0x2E89;		/* 680x0 "MOVE.L A1,(SP)" */
//#endif

//#if TARGET_CPU_PPC
static pascal void NTMTask(TMTaskPtr tmTaskPtr);
//#else
//static pascal void NTMTask(void);
//#endif

//#if TARGET_CPU_PPC
static pascal void NTMTask(TMTaskPtr tmTaskPtr)
//#else
//static pascal void NTMTask()
//#endif
{
//#if TARGET_CPU_PPC
//#else
//	TMInfoPtr	recPtr;
//	long			oldA5;				/* A5 when task is called */
//
//	recPtr = GetTMInfo();								/* first get our record */
//	oldA5 = SetA5(recPtr->tmRefCon);					/* set A5 to app's A5 world */
//#endif

	/* The heart of the task: just increment the count! */
	if (timerRunning)	theCount++;

//#if TARGET_CPU_PPC
	PrimeTime((QElemPtr)tmTaskPtr, theDelay);		/* re-install task to repeat it indefinitely */
//#else
//	PrimeTime((QElemPtr)recPtr, theDelay);			/* re-install task to repeat it indefinitely */

//	oldA5 = SetA5(oldA5);								/* restore original A5 and ignore result */
//#endif
}

/* Initialize Nightingale Time Manager. */

void NTMInit()
{
	theTMInfo.atmTask.tmAddr = NewTimerUPP(NTMTask);	/* get UPP for our task */
	theTMInfo.atmTask.tmWakeUp = 0;
	theTMInfo.atmTask.tmReserved = 0;
//#if TARGET_CPU_PPC
	/* nothing needed for PPC */
//#else
//	theTMInfo.tmRefCon = SetCurrentA5(); 			/* store A5 world address */
//#endif
	InsXTime((QElemPtr)&theTMInfo);					/* install the task record */
}

/* Close Nightingale Time Manager. If an application initializes NTM and quits
without calling NTMClose, something unpleasant is likely to happen later. */
 
void NTMClose()
{
	RmvTime((QElemPtr)&theTMInfo);
}

/* Start the timer ticking at the given rate. NTMInit must have been called first! */
 
void NTMStartTimer(
	unsigned short delayTime)			/* Timer tick rate, in milliseconds */
{
	theCount = 0L;
	theDelay = delayTime;
	timerRunning = TRUE;
	PrimeTime((QElemPtr)&theTMInfo, theDelay);	/* activate the task record */
}

/* Temporarily or permanently stop the timer, leaving current time unchanged. */
void NTMStopTimer()
{
	timerRunning = FALSE;
}

/* Return the timer's current time, in milliseconds. */

long NTMGetTime()
{
	return theCount;
}
