/* NANSI.c - ANSI library routines for Nightingale that aren't part of THINK C 7's
"ANSI-small" library. We can't easily switch to the full ANSI library because
it needs more data segment space (over 3K bytes vs. 1396), and we don't have
room for it in the 32K available in the data segment. But nothing like this
should be needed with any other development environment--I hope.

Parts of this are copyright (c) 1989 Symantec Corporation.  All rights reserved. */

#ifdef THINK_C

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "math.h"
#include "errno.h"
#include <SANE.h>


/* ------------------------------------------------------------- atof (stdlib.h) -- */

#ifndef OS_MAC
#error THIS IS MAC OS-ONLY CODE.
#else
double atof(const char *s)
	{
		short double sdTemp;
		Decimal	dec;
		short	index=0;
		Boolean	 read_error;
		double	x;
		
		CStr2Dec(s,&index,&dec,&read_error);
		/*
		 *	Doug and I can't find the fp68k code for the 12-byte long double format
		 *	we're using elsewhere (and which seems best for THINK C 5), so just get a
		 *	(8-byte) short double from fp68k and convert it here.
		 */
		fp68k(&dec,&sdTemp,FFDBL|FOD2B);
		x = sdTemp;
		
		return(x);
	}
#endif


	/*  local constants  */

static double Zero = 0.0;


	/*  environmental control  */

static short env;

#define Reset()				{ fp68k(&env, FPROCENTRY); }
#define RoundUp()			{ env = 0x2000; fp68k(&env, FSETENV); }
#define RoundDown()			{ env = 0x4000; fp68k(&env, FSETENV); }


	/*  error checking  */

#define DomainCheck(test, result)					\
	if (test) {										\
		errno = EDOM;								\
		return(result);								\
	}

#define RangeCheck(target, result)					\
	if ((fp68k(&env, FGETENV), env) & 0x0F00) {		\
		errno = ERANGE;								\
		target = result;							\
	}


/* --------------------------------------------------------------- sqrt (math.h) -- */
/* Homebrew square root function. This is a vanilla implementation of the
Newton-Raphson method. (The version we used to use, which called fp68k,
failed badly in some cases. Of course it was also totally machine-dependent
and this should work on any system, but we should be able to use the standard
C library version in the future.) */

#define FABS(x)			((x)<0.0? -(x) : (x))
#define MAX_ITERATIONS	25
#define REL_ERROR_LIMIT	1e-7

double sqrt(double arg);
double sqrt(double arg)
{
	short i;
	double approx, lastApprox, changeFactor;

	lastApprox = approx = arg;

	for (i = 0; i < MAX_ITERATIONS; i++)
	{
		approx = (0.5 * (approx + arg / approx));
		changeFactor = 1.0-lastApprox/approx;
		if (FABS(changeFactor)<REL_ERROR_LIMIT) break;
		lastApprox = approx;
	}
	
	return approx;
}

#endif	/* THINK_C */
