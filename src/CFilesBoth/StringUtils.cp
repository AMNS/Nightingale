/* StringUtils.c for Nightingale - string-handling utilities - new in v.3.1 */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Copyright © 2016 by Avian Music Notation Foundation.
 * All Rights Reserved.
 */

/*
		PToCString				CToPString				Pstrcpy
		PStrncpy				streql					strneql
		Pstreql					Pstrneql				Pstrlen
		PStrCat					GoodStrncpy
		ExpandPString			GetFinalSubstring		GetInitialSubstring
*/

#include "Nightingale_Prefix.pch"
#include <ctype.h>
#include "Nightingale.appl.h"

/* ======================================================== Pascal string "toolbox" == */
/* The following are mostly versions of standard C librariy functions that expect
Pascal strings instead of C strings, plus some general string utilities.  Cf.
StringToolbox.c.

/* --------------------------------------------------------- PToCString, CToPString -- */
/* Convert C to Pascal style strings and vice-versa. Very similar, quite possibly
identical, to Symantec's PtoCstr and CtoPstr. */

/* Convert in place a Pascal string to C string */

char *PToCString(StringPtr str)
{
	StringPtr p, q, end;
	
	end = str + *str;
	q = (p=str) + 1;
	while (p != end) *p++ = *q++;
	*p = '\0';
	return (char *)str;
}


/* Convert in place a C string to Pascal string */

StringPtr CToPString(char *str)
{
	char *p, *q;
	short len;
	
	p = str + (len = strlen(str));
	q = p-1;
	while (p != str) *p-- = *q--;
	*str = (len>255) ? 255 : len;
	return (StringPtr)str;
}


/* Copy Pascal string from src to dst; return dst */

StringPtr Pstrcpy(StringPtr dst, ConstStringPtr src)
{
	unsigned char *old = dst;
	unsigned short n;
	
	n = *(unsigned char *)src + 1;
	while (n-- > 0) *dst++ = *src++;
	return old;
}


/* Copy the first n chars of a Pascal string from src to dst */

void PStrncpy(StringPtr dst, ConstStringPtr src, short n)	
{
	short len;
	
	len = *dst++ = *src++;
	while (--len>=0 && --n>=0) *dst++ = *src++;
}


/* Are two C strings the same or not */

Boolean streql(char *s1, char *s2)
{
	/* FIXME: It would be best to replace this body with a call to strcmp. */
	
	while (*s1) if (*s1++ != *s2++) return false;
	return (*s1 == *s2);
}


/* Are two C strings the same or not up to given length */

Boolean strneql(char *s1, char *s2, short n)
{
	/* FIXME: It would be best to replace this body with a call to strncmp. */
	if (n++ == 0) return true;
	
	while (*s1 && --n>0) if (*s1++ != *s2++) return false;
	n--;
	if (n == 0) return true;
	return (*s1 == *s2);
}


/* Are two Pascal strings the same or not */

Boolean Pstreql(StringPtr s1, StringPtr s2)
{
	short len;
	unsigned char *u1=s1, *u2=s2;
	
	len = *u1;
	while (len-- >= 0)
		if (*u1++ != *u2++) return false;
	
	return true;
}

/* Are the first n chars of two Pascal strings the same? */

Boolean Pstrneql(StringPtr p1, StringPtr p2, short n)
{
	short len;
	
	len = *p2;
	if (*p2++!=*p1++) return false;			/* Compare length byte */
	while (--len>=0 && --n>=0)
		if (*p2++!=*p1++) return false;		/* Compare content */
	return true;
}

/* Length of a Pascal string */

short Pstrlen(ConstStringPtr str)
{
	return (short)str[0];
}


/* ------------------------------------------------------------------------ PStrCat -- */
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


/* -------------------------------------------------------------------- GoodStrncpy -- */
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


/* ------------------------------------------------------------------ ExpandPString -- */
/* Stretch the given Pascal string by adding one or two blanks between each pair of
characters. */

Boolean ExpandPString(StringPtr dstStr, StringPtr srcStr, bool wider)
{
	unsigned short in, out, origLen, maxLenExpanded;
	
	origLen = *srcStr;
	Pstrcpy(dstStr, srcStr);
	PToCString(dstStr);
	/* Pascal strings can't be longer than 255 chars. */
	maxLenExpanded = (wider? 255/3 : 255/2);
	if (origLen>maxLenExpanded) return false;
	
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

	return true;
}


/* ------------------------------------------------------------ Substring functions -- */
/* C's standard string functions don't handle substrings well, especially if you don't
want to do pointer arithmetic. Here are a couple of substring functions, designed for
handling filenames but potentially useful for other purposes. */

Boolean GetFinalSubstring(char *str, char *substr, char delimChar) {
    const char *delim = strrchr(str, delimChar);
    if (!delim || delim==str) {
        strcpy(substr, "");
        return false;
    }
    else {
        strcpy(substr, delim+1);
        return true;
    }
}


Boolean GetInitialSubstring(char *str, char *substr, short len)
{
	if (len>strlen(str)) {
		*substr = '\0';
		return false;
	}
	
	strcpy(substr, str);
	*(substr+len) = '\0';
	return true;
}

