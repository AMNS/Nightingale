/* StringUtils.c for Nightingale - string-handling utilities - new for v.3.1 */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-96 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/*
		PToCString				CToPString				Pstrcpy
		streql					strneql					Pstreql		
		PStrCat					PStrCopy				PStrnCopy
		PStrCmp					PStrnCmp				GoodStrncpy
		ExpandString

*/

#include "Nightingale_Prefix.pch"
#include <ctype.h>
#include "Nightingale.appl.h"

/* -------------------------------------------------------- PToCString,CToPString -- */
/* Convert C to Pascal style strings and vice-versa. Very similar, quite possibly
identical, to Symantec's PtoCstr and CtoPstr. */

/* Convert in place a Pascal string to C string */

char *PToCString(unsigned char *str)
{
	unsigned char *p,*q,*end;
	
	end = str + *(unsigned char *)str;
	q = (p=str) + 1;
	while (p != end) *p++ = *q++;
	*p = '\0';
	return((char *)str);
}


/* Convert in place a C string to Pascal string */

unsigned char *CToPString(char *str)
{
	char *p,*q;
	short len;
	
	p = str + (len = strlen(str));
	q = p-1;
	while (p != str) *p-- = *q--;
	*str = (len>255) ? 255 : len;
	return((unsigned char *)str);
}


/* Copy Pascal string from src to dst; return dst */

unsigned char *Pstrcpy(unsigned char *dst, const unsigned char *src)
{
	unsigned char *old = dst;
	unsigned short n;
	
	n = *(unsigned char *)src + 1;
	while (n-- > 0) *dst++ = *src++;
	return(old);
}


/* Are two strings the same or not */

Boolean streql(char *s1, char *s2)
{
	/* ??It would be best to replace this body with a call to strcmp. */
	
	while (*s1) if (*s1++ != *s2++) return(FALSE);
	return(*s1 == *s2);
}


/* Are two strings the same or not up to given length */

Boolean strneql(char *s1, char *s2, short n)
{
	/* ??It would be best to replace this body with a call to strncmp. */
	if (n++ == 0) return(TRUE);
	
	while (*s1 && --n>0) if (*s1++ != *s2++) return(FALSE);
	n--;
	if (n == 0) return(TRUE);
	return(*s1 == *s2);
}


/* Are two Pascal strings the same or not */

Boolean Pstreql(unsigned char *s1, unsigned char *s2)
{
	short len;
	unsigned char *u1=s1,*u2=s2;
	
	len = *u1;
	while (len-- >= 0)
		if (*u1++ != *u2++) return(FALSE);
	
	return(TRUE);
}


/* --------------------------------------------------------------------- PStrCat -- */
/* Concatenates a Pascal string at p2 to the end of p1. NB: if the total length
of the strings is over 255 characters, this will fail but we won't detect it! */

void PStrCat(StringPtr p1, ConstStringPtr p2)
{
	short len;
	
	len = *p1;								/* Point p1 at end of 1st string */
	*p1++ = len + *p2;						/* Add string lengths */
	while (--len>=0) p1++;
	
	len = *p2++;							/* Copy *p2 to *p1 */
	while (--len>=0) *p1++=*p2++;
}


/* ---------------------------------------------------------- PStrCopy, PStrnCopy -- */
/* Pascal string copy routines.  [From LSC miniedit] */

/* Copy a Pascal string from src to dst */

void PStrCopy(ConstStringPtr src, StringPtr dst)
{
	short len;
	
	len = *dst++ = *src++;
	while (--len>=0) *dst++ = *src++;
}

/* Copy the first n chars of a Pascal string from src to dst */

void PStrnCopy(ConstStringPtr src, StringPtr dst, short n)	
{
	short len;
	
	len = *dst++ = *src++;
	while (--len>=0 && --n>=0) *dst++ = *src++;
}


/* ------------------------------------------------------------ PStrCmp, PStrnCmp -- */
/* Pascal string comparison routines. */

/* Are Pascal strings p1 and p2 the same? */

Boolean PStrCmp(ConstStringPtr p1, ConstStringPtr p2)
{
	short len;
	
	len = *p2;
	if (*p2++!=*p1++) return FALSE;			/* Compare length byte */
	while (--len>=0)
		if (*p2++!=*p1++) return FALSE;		/* Compare content */
	return TRUE;
}

/* Are the first n chars of Pascal strings p1 and p2 the same? */

Boolean PStrnCmp(ConstStringPtr p1, ConstStringPtr p2, short n)
{
	short len;
	
	len = *p2;
	if (*p2++!=*p1++) return FALSE;			/* Compare length byte */
	while (--len>=0 && --n>=0)
		if (*p2++!=*p1++) return FALSE;		/* Compare content */
	return TRUE;
}


/* ----------------------------------------------------------------- GoodStrncpy -- */
/* GoodStrncpy is like the ANSI function strncpy, except that it guarantees the
result will be a valid C string even if the length of srcStr exceeds numChars.
Note that it does this by storing into the <numChars+1>st location of <dstStr>! */

void GoodStrncpy(
		char *dstStr, char *srcStr,
		unsigned long numChars)				/* Caution: stores into THIS loc. */
{
	strncpy(dstStr, srcStr, numChars);
	dstStr[numChars] = '\0';
}


/* ----------------------------------------------------------------- ExpandString -- */
/* Stretch the given Pascal string by adding one or two blanks between each pair of
characters. */

Boolean ExpandString(unsigned char *dstStr, const unsigned char *srcStr, Boolean wider)
{
	unsigned short in, out, origLen, maxLenExpanded;
	
	origLen = *(unsigned char *)srcStr;
	Pstrcpy(dstStr, srcStr);
	PToCString(dstStr);
	/* Pascal strings can't be longer than 255 chars. */
	maxLenExpanded = (wider? 255/3 : 255/2);
	if (origLen>maxLenExpanded) return FALSE;
	
	for (in = 1, out = 0; in<=origLen; in++) {
		*(dstStr+out) = *(srcStr+in);
		out++;
		if (in==origLen) break;					/* don't add a blank at the end */
		*(dstStr+out) = ' ';
		out++;
		if (wider) {
			*(dstStr+out) = ' ';
			out++;
		}
	}
	
	*(dstStr+out) = '\0';
	CToPString((char *)dstStr);

	return TRUE;
}

