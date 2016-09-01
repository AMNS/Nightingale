/* Error.c - Error message routines for Nightingale, rev. for v.3.1 */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE» PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1991-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include <stdarg.h>


/* ================================================= Doug's generic error routines == */

enum {					/* Indices of error strings in Error Strings STR# resource */
		useMsg,			/* Signal to take string from second argument */
		cannotPrint,
		notOurDocument,
		badInit,
		tooManyDocs,
		tooManyPages,
		tooManyRows,
		tooManyColumns,
		pageTooSmall,
		pageTooLarge,
		missingValue,
		badUpperLeft,
		noMoreMemory,
		exhaustedMemory,
		
		eatComma
	};

/* Prototypes for local routines */

static void		ErrorMsg(short index);
static void		ErrorNumber(short index, long num);
static void		ErrorString(short index, unsigned char *msg);
static short	DoGeneralAlert(unsigned char *str);


/* Print an error message.  If index is non-zero, then retrieve the index'th
string from our error strings resource. */

static void ErrorMsg(short index)
	{
		Str255 str;
		
		if (index > 0) GetIndString(str,ErrorStringsID,index);
		else		   	*str = 0;
		(void)DoGeneralAlert(str);
	}

/*
 *	Print an error message with a string argument.  The message is kept
 *	in the index'th string in the error strings resource. The string should
 *	have a ^0 in it where we'll place the argument to the message.
 */

static void ErrorString(short index, unsigned char *msg)
	{
		Str255 str;
		
		GetIndString(str,ErrorStringsID,index);
		InstallArg(str,msg,1);
		(void)DoGeneralAlert(str);
	}

/*
 *	Print an error message with any number (count>0) of long arguments.  The
 *	message is kept in the index'th string in the error strings.
 */

static void ErrorNumber(short index, long num)
	{
		Str255 str; Str31 numStr;
		
		GetIndString(str,ErrorStringsID,index);
		NumToString(num,numStr);
		InstallArg(str,numStr,0);
		(void)DoGeneralAlert(str);
	}

/*
 *	Install a given string into the single ParamText argument in the given
 *	alert.  This routine is broken out in case we want to get fancy and choose
 *	different alerts based on the size of the string.
 */

static short DoGeneralAlert(unsigned char *str)
	{
		ParamText(str,"\p","\p","\p");
		PlaceAlert(errorMsgID,NULL,0,40);
		return (StopAlert(errorMsgID,NULL));
	}

/* Given a string, _str_, an argument string, _msg_, and an index _n_, look for a
three-character string of the form double-< n double-> in str, and substitute msg
for it. (NB: The double-< and -> are really guillemets.) str is expected to be 256
bytes in length. All strings are Pascal. This routine can be called repeatedly to
substitute different strings into the given str. It currently works only for n =
0,1,...9.  FIXME: Actually, I'm not sure it works at all any more! This file was
originally encoded in Macintosh Roman; I converted it to Latin-1 in 2015, and the
compiled-in guillemet chars. look like they got garbled in the process. It'd probably
be easy to fix, but in heavy use of Nightingale for the last six or ten months, I'm
not sure I've ever seen it used, so it's low priority; besides, it'd be hard to test.
--DAB, Feb. 2016 */

void InstallArg(unsigned char *str, unsigned char *msg, short n)
	{
		unsigned char *src,*dst; short strLen,msgLen,len;
		unsigned char *top;
		
		strLen = *str;
		msgLen = *msg;
		n += '0';
		
		/* Scan backwards from last character for ‚ */
		
		dst = str+strLen;
		while (dst > str)
			if (*dst-- == (unsigned char)'é')
				if (dst>str && *dst-- == n)
					if (dst>str && *dst==(unsigned char)'‚') {
						/* Found start of arg insertion point */
						if (msgLen > 3) {
							if (strLen + msgLen - 3 <= 255) {
								/* Make room for argument string */
								dst += 3;			/* 3 = sizeof "‚né" */
								src = str+strLen+1;
								while (--src >= dst) *(src+msgLen-3) = *src;
								/* Overwrite the ‚né */
								dst -= 3;
								src = msg+1;
								*str += msgLen-3;
								while (msgLen-- > 0) *dst++ = *src++;
								}
							}
						 else {
							/* String is getting smaller */
							*str += msgLen-3;
							/* Overwrite ‚né with message */
							src = msg+1;
							len = msgLen;
							while (msgLen-- > 0) *dst++ = *src++;
							/* Shift rest of string down */
							src = dst - len + 3;
							top = str + strLen;
							while (src <= top) *dst++ = *src++;
							}
						break;
						}
	}
	

/* These are specific error routines, whose names indicate their purpose.
Eventually, all these routines can be consolidated into a few small generic
routines, and we can redefine their names as macros, to avoid so many
functions. */

void CannotPrint()							{ ErrorMsg(cannotPrint); }

void NotOurs(unsigned char *name)			{ ErrorString(notOurDocument,name); }

void BadInit()								{ ErrorMsg(badInit); }

void TooManyDocs()							{ ErrorMsg(tooManyDocs); }

void TooManyPages(short limit)			 	{ ErrorNumber(tooManyPages,(long)limit); }

void TooManyRows(short limit)		 		{ ErrorNumber(tooManyRows,(long)limit); }
	
void TooManyColumns(short limit)		 	{ ErrorNumber(tooManyColumns,(long)limit); }

void PageTooSmall()							{ ErrorMsg(pageTooSmall); }

void PageTooLarge()							{ ErrorMsg(pageTooLarge); }

void MissingValue()							{ ErrorMsg(missingValue); }

void NoMoreMemory()							{ ErrorMsg(noMoreMemory); }

void OutOfMemory(long nBytes)				{ ErrorNumber(exhaustedMemory,nBytes); }


/* ================================================================ Miscellaneous == */

/* ---------------------------------------------------------------------- MayErrMsg -- */

char	junk[256];

static void EMDebugPrintf(char *fmt, ... );
static void EMDebugPrintf(char *fmt, ... )
{
	long args[6]; short i;
	va_list ap;
	
	va_start(ap,fmt);
	for (i=0; i<6; i++) args[i] = va_arg(ap,long);
	va_end(ap);
	
	LogPrintf(LOG_WARNING, fmt, args[0], args[1], args[2], args[3], args[4], args[5]);
}

void MayErrMsg(char *fmt, ...)
{
	va_list ap; long arg1,arg2,arg3,arg4,arg5,arg6;
	
	static short alertCount = 2;

	va_start(ap,fmt);
	arg1 = va_arg(ap,long);
	arg2 = va_arg(ap,long);
	arg3 = va_arg(ap,long);
	arg4 = va_arg(ap,long);
	arg5 = va_arg(ap,long);
	arg6 = va_arg(ap,long);
	va_end(ap);
	
	sprintf(junk, "PROGRAM ERROR: ");  
	sprintf(&junk[strlen(junk)], fmt, arg1, arg2, arg3, arg4, arg5, arg6);
	SysBeep(20);

	if (--alertCount>=0 || ShiftKeyDown()) {
		CParamText(junk, "", "", "");
		StopInform(GENERIC_ALRT);
	}

	EMDebugPrintf(fmt, arg1, arg2, arg3, arg4, arg5, arg6);
	EMDebugPrintf("\n");

	if (CmdKeyDown() && ShiftKeyDown() && OptionKeyDown()) DebugStr("\pBREAK IN MayErrMsg");
}

void AlwaysErrMsg(char *fmt, ...)
{
	va_list ap; long arg1,arg2,arg3,arg4,arg5,arg6;
	
	va_start(ap,fmt);
	arg1 = va_arg(ap,long);
	arg2 = va_arg(ap,long);
	arg3 = va_arg(ap,long);
	arg4 = va_arg(ap,long);
	arg5 = va_arg(ap,long);
	arg6 = va_arg(ap,long);
	va_end(ap);
	
	sprintf(junk, "PROGRAM ERROR: ");  
	sprintf(&junk[strlen(junk)], fmt, arg1, arg2, arg3, arg4, arg5, arg6);
	SysBeep(20);

	CParamText(junk, "", "", "");
	StopInform(GENERIC_ALRT);

	EMDebugPrintf(fmt, arg1, arg2, arg3, arg4, arg5, arg6);
	EMDebugPrintf("\n");

	if (CmdKeyDown() && ShiftKeyDown() && OptionKeyDown()) DebugStr("\pBREAK IN AlwaysErrMsg");
}


/* ---------------------------------------------------------------- ReportIOError -- */
/* Alert user to the given I/O error, using the given dialog. */

Boolean ReportIOError(short errCode, short dlog)
{
	StringHandle strHdl; char fmtStr[256];
	
	if (errCode==noError) return FALSE;

	strHdl = GetString(errCode);
	if (strHdl) {
		Pstrcpy((unsigned char *)strBuf, *strHdl);
		PToCString((unsigned char *)strBuf);
	}
	else {
		GetIndCString(fmtStr, ErrorStringsID, 14);		/* "System I/O error number %d." */
		sprintf(strBuf, fmtStr, errCode);
	}
	CParamText(strBuf, "", "", "");
	StopInform(dlog);
	return TRUE;
}


/* --------------------------------------------------------------- ReportResError -- */
/* Check for a Resource Manager error and alert user to it, if there is one. Return
TRUE if there's an error, FALSE if not. NB: Calling this after, e.g., GetResource
is not good enough: even if the GetResource failed, ResError() can return noErr!
(Cf. Inside Macintosh, I-119.) To check validity of newly-allocated resources, use
ReportBadResource. */

Boolean ReportResError()
{
	OSErr theErr; char buf[256];
	
	if ( (theErr = ResError()) != noErr) 
	{
		/* It's nice to keep error messages in resources, but too dangerous here! */
		
		sprintf(buf,"Resource Manager error number %d.", theErr);
		CParamText(buf, "", "", "");
		StopInform(GENERIC_ALRT);
		return TRUE;
	}
	return FALSE;
}


/* ------------------------------------------------------------ ReportBadResource -- */
/* Check for validity of the given resource, presumably just allocated, and alert
user if there is a problem. Return TRUE if there's an error, FALSE if not. */

Boolean ReportBadResource(Handle resH)
{
	if (!GoodResource(resH)) {
		/* There's a problem. If ReportResError doesn't find it, say so ourselves. */ 
		if (!ReportResError()) {
			/* It's nice to keep error messages in resources, but too dangerous here! */
		
			CParamText("Couldn't get a resource.", "", "", "");
			StopInform(GENERIC_ALRT);
		}
		return TRUE;
	}
	
	return FALSE;
}


/* --------------------------------------------------------------- MissingDialog -- */
/* Alert user when a dialog resource cannot be found. For now, just beep and put a
complied-in message in the log. We could also give a compiled-in error message, but
it's probably too dangerous to try to get one from a resource in this situation. */

void MissingDialog(short dlogID)
{
	SysBeep(1);
	LogPrintf(LOG_WARNING, "MissingDialog: can't find dialog resource %d.\n", dlogID);
}


/* ------------------------------------------------------------- AppleEventError -- */
/* Alert user when an Apple event handler gets an error code. */

void AppleEventError(char */*handlerName*/, short errCode)
	{
		char fmtStr[256], buf[256];
		
		GetIndCString(fmtStr, ErrorStringsID, 15);		/* "Error %d in an Apple Event Handler (%s)." */
		sprintf(buf, fmtStr, errCode);
		CParamText(buf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
