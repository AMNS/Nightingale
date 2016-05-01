/* NTimeMgr.c - simple Nightingale Time Manager package - D. Byrd, AMNS, 2/96 */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* This code is heavily Macintosh-dependent. More specifically, it requires the
extended (System 7) version of the Time Manager! Before calling these routines,
you should check that the extended Time Manager is available with something like:
		Gestalt(gestaltTimeMgrVersion, &gestaltAnswer);
gestaltAnswer = 3 (gestaltExtendedTimeMgr) for the extended Time Manager. */

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

typedef struct {					/* Time Manager information record */
	TMTask	atmTask;				/* original and revised TMTask record */
	long	tmRefCon;				/* space to pass address of A5 world */
} TMInfo;

typedef TMInfo *TMInfoPtr;

static TMInfo theTMInfo;			/* Our TimeMgr information record */
static long theDelay;				/* No. of milliseconds to delay between "ticks" */
static long theCount;				/* Current timer value */
static Boolean timerRunning;

static pascal void NTMTask(TMTaskPtr tmTaskPtr);

static pascal void NTMTask(TMTaskPtr tmTaskPtr)
{
	/* The heart of the task: just increment the count! */
	if (timerRunning)	theCount++;

	PrimeTime((QElemPtr)tmTaskPtr, theDelay);		/* re-install task to repeat it indefinitely */
}

/* Initialize Nightingale Time Manager. */

void NTMInit()
{
	theTMInfo.atmTask.tmAddr = NewTimerUPP(NTMTask);	/* get UPP for our task */
	theTMInfo.atmTask.tmWakeUp = 0;
	theTMInfo.atmTask.tmReserved = 0;
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
