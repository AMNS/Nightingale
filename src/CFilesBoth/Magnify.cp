/***************************************************************************
*	FILE:	Magnify.c																			*
*	PROJ:	Nightingale, new for v.2.1														*
*	DESC:	Routines	for handling screen viewing magnifications					*
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1995 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* NB: Code in various places in Nightingale assumes (1) that doc->magnify=0
means no magnification (i.e., pixels=points); (2) that all non-negative values
scale by multiples of 1/2 (see comments on MagnifyPaper/UnmagnifyPaper).
It CANNOT be assumed that doc->magnify=n is the inverse magnification to
doc->magnify=-n: this was valid in versions of Nightingale thru 2.1, but
not since we support non-integral magnifications. */

/* The coordinate-conversion and magnification-computing utilities below are
used a lot: they need to be fast! Amazingly enough, THINK C as of version 6
STILL isn't smart enough to generate shifts for divides by powers of 2, and
(on MC680x0's, like most processors) shifts are very much faster, so--where
I wanted such divides--I use "divide by" macros that do shifts. NB: I assume
that right shifting only positive numbers divides: ANSI/ISO C (Sec. 6.3.7)
imposes this requirement. I assume that left shifting either positive or
negative numbers multiplies. Unfortunately, ANSI/ISO does not require that
left shifting values of a signed type, positive or negative, perform
multiplication, but both do on the Macintosh (at least with THINK C!). It
seems that they would on any reasonable compiler and processor, e.g., PCs, but
be careful. (NB: I'm pretty sure there are other places in Nightingale that
make similar assumptions.) */

/* ------------------------------------------------------ Coordinate Conversion -- */
/* D2PFunc: same as the d2p macro: convert DDISTs to pixels, rounding.
	D2PXFunc: same as the d2px macro: convert DDISTs to pixels, truncating.
	P2DFunc: same as the p2d macro: convert pixels to DDISTs.
	
	Magnification	25%	37.5%	50%	75%	100%	150%	200%	300%	400%	600%
	magnify			-4		-3		-2		-1		0		1		2		3		4		5
	magShift			6		6		5		5		4		4		3		3		2		2
	magRound			32		32?	16		16?	8		8?		4		4?		2		2?
*/

short D2PFunc(DDIST d)
{
	short nPixels, nPixBase, nPixPlus;
	
	if (odd(magMagnify)) {
		/*
		 * We need extra protection against roundoff because, after truncating,
		 * we're going to increase our value by 50%. Tricky.
		 */
		nPixBase = (d+magRound) >> magShift;
		nPixPlus = (d+magRound+divby2(magRound)) >> magShift;
		nPixels = nPixBase+divby2(nPixBase);			/* ... nPixBase+(nPixBase/2) */
		if (nPixPlus>nPixBase) nPixels++;
	}
	else
		nPixels = (d+magRound) >> magShift;

	return nPixels;
}

short D2PXFunc(DDIST d)
{
	short nPixels;
	
	nPixels = d >> magShift;
	if (odd(magMagnify))
		nPixels += divby2(nPixels);						/* ... += nPixels/2 */

	return nPixels;
}

DDIST P2DFunc(short p)
{
	short dShifted, dby3Shifted;
	
	if (odd(magMagnify)) {
		/* To avoid overflow, shift left by magShift-1, then add in the extra. */
		dShifted = p << (magShift-1);
		dby3Shifted = dShifted/3;							/* Slow but there's no way around it */
		return dShifted+dby3Shifted;
	}
	else
		return p << magShift;
}

/* ------------------------------------------------------------ UseMagnifiedSize -- */
/* Given actual size and magnification, get the size to use. Use this instead of
our coordinate-conversion routines (d2p, d2px, p2d, etc.) when we're computing
arguments for toolbox routines, e.g., to convert font size from points to pixels
(which, as far as toolbox routines know, *are* points). */

short UseMagnifiedSize(
				short size,				/* Actual (unmagnified) size */
				short magnify
				)
{
	short powerOf2, magSize;
	
	powerOf2 = divby2(magnify);
	if (magnify<0) {
		magSize = size>>(-powerOf2);
		if (odd(magnify)) magSize -= divby4(magSize);		/* ... -= magSize/4 */
	}
	else {
		magSize = size<<powerOf2;
		if (odd(magnify)) magSize += divby2(magSize);		/* ... += magSize/2 */
	}
	
	return magSize;
}

/* --------------------------------------------------------- UseRelMagnifiedSize -- */
/* Given size for the maximum magnification, plus another magnification, get
the size to use at the other magnification. */

short UseRelMagnifiedSize(
			short maxMagSize,				/* Size for magnification MAX_MAGNIFY */
			short magnify
			)
{
	short magChange, powerOf2, magSize;
	
	magChange = MAX_MAGNIFY-magnify;
	powerOf2 = magChange/2;
	magSize = maxMagSize>>powerOf2;
	if (odd(magChange)) magSize -= magSize/3;
	
	return magSize;
}

/* ----------------------------------------------------------------- UseTextSize -- */
/* Get the font size to use for the given magnification. */

short UseTextSize(
			short fontSize,			/* Actual (unmagnified) font size */
			short magnify
			)
{
	/*
	 * UseTextSize is currently identical to UseMagnifiedSize, but I can imagine
	 * it needing to do something different, so leave it separate.
	 */
	return UseMagnifiedSize(fontSize, magnify);
}

/* ---------------------------------------------------------------- UseMTextSize -- */
/* Get the music font size to use for the given magnification, taking into account
the silly names the Adobe screen sizes of Sonata have (Sonata 14 is "really" 16,
etc.). ??NB: if the music font isn't Sonata, should probably do nothing! Cf.
<doAdjust>.  */

short UseMTextSize(
			short fontSize,			/* Actual (unmagnified) font size */
			short magnify
			)
{
	static short prevFontSize=-1, prevMagnify, size;
	short actualSize;
	Boolean doAdjust=TRUE;
	
	if (!doAdjust) return fontSize;

	/* If no change to font size or magnification, we already have the answer. */
	
	if (fontSize==prevFontSize && magnify==prevMagnify) return size;
		
	prevFontSize = fontSize;		
	prevMagnify = magnify;
	
	/*
	 * If magnification is integral, don't worry about the silly names. Doing so
	 * actually causes a problem for Sonata-18 at 200 percent: Sonata-36 is
	 * "really" 40 point in some ways, but 36 or even 37(!) in others, so getting
	 * fancy doesn't work.
	 */ 
	if (!odd(magnify)) {
		size = UseTextSize(fontSize, magnify);
		
		return size;
	}

	/* Convert from Sonata to "actual" size, magnify, and convert back. */
	
	actualSize = GetActualFontSize(fontSize);
	size = UseTextSize(actualSize, magnify);
	size = GetMNamedFontSize(size);
	
	return size;
}
