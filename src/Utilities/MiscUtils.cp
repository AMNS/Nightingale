/*	MiscUtils.c
These are miscellaneous routines that are generally-useful extensions to the Mac
Toolbox routines. Nightingale formerly contained a module called EssentialTools.c,
original version by Doug McKenna, 1989. Most of the functions that were originally
in that file were for dialog or other user-interface support, and they've been moved
to DialogUtils.c or UIFUtils.c; other functions have been moved to DSUtils.c, etc. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include <Carbon/Carbon.h>
// MAS
#include <ctype.h>

#include "Nightingale.appl.h"
#include "NavServices.h"

/* ------------------------------------------------- Pointers, memory management, etc. -- */

/* Set a block of nBytes bytes, starting at m, to zero.  m must be even. Cf. FillMem. */

void ZeroMem(void *m, long nBytes)
{
	char *p;

	p = (char *)m;
	while (nBytes-- > 0) *p++ = 0;
}

/*
 *	For checking validity of pointers and handles just allocated
 */

Boolean GoodNewPtr(Ptr p)
{
	return( (p!=NULL) && (MemError()==noErr) );
}

Boolean GoodNewHandle(Handle hndl)
{
	return( (hndl!=NULL) && GoodNewPtr(*hndl) );
}

Boolean GoodResource(Handle hndl)
{
	return( hndl!=NULL && ResError()==noErr && *hndl!=NULL );
}

/* The below functions date back to the 1980's, when machines had just a few megabytes
of memory. Now that machines have gigabytes, running out of memory has hardly worth
worrying about, but I doubt if the code (which to my knowledge has worked for decades
now) is worth touching.  --DAB, May 2020 */

/* This requests a non-relocatable n-byte block of memory from the Toolbox, and zeroes
it prior to delivering.  If not enough memory, it goes to error code -- which may not
work, since the error dialog needs memory too. */

void *NewZPtr(Size n)		/* Allocate n bytes of zeroed memory; deliver ptr */
{
	char *p, *q;  void *ptr;

	ptr = NewPtr(n);
	if (!ptr) { SysBeep(1);  NoMoreMemory(); }
	 else {
		p = (char *)ptr;  q = p + n;
		while (p < q) *p++ = '\0';
	}
	return(ptr);
}

/* GrowMemory() is a very simple grow zone function designed to prevent the ROM or
resident software from generating out-of-memory errors. GrowMemory assumes we've
pre-allocated an otherwise unused block of memory (this should normally be done at
initialize time); we free it here when the Memory Manager calls this routine as the
installed Grow Zone procedure. We also note that memory is low for the benefit of
possible error messages to the user. See Inside Macintosh vol. 2, p.42-43. */

pascal long GrowMemory(Size /*nBytes*/)		/* nBytes is unused */
{
	long bufferSize=0L;
	long oldA5 = SetCurrentA5();
	
	if (memoryBuffer!=NULL) {
		bufferSize = GetHandleSize(memoryBuffer);
		DisposeHandle(memoryBuffer);
		memoryBuffer = NULL;
	}
	outOfMemory = True;
	
	SetA5(oldA5);
	return(bufferSize);
}


/* Is the given amount of memory available? This can be checked simply by calling
FreeMem, or by actually trying to allocate the desired amount of memory, then
immediately freeing it. Calling FreeMem is surely faster but is not very meaningful,
since it gives the total amount of memory available right now, (1) not worrying about
fragmentation, and (2) not taking into account the Memory Manager's tricks for
getting more memory. (Another way would be to call MaxMem; this also doesn't seem
as good as trying to allocate the desired amount but I'm not sure.) */

Boolean PreflightMem(short nKBytes)		/* if nKBytes<=0, assume max. size of a segment */
{
	long nBytes;
	
	if (nKBytes<=0) nKBytes = 32;
	nBytes = 1024L*(long)nKBytes;

	return (FreeMem()>=nBytes);
}


/* Count the number of set bits in a block of memory. Intended for debugging, specifically
for comparing two bitmaps that should be identical. */

static short BitCount(unsigned char ch);
static short BitCount(unsigned char ch)
{
	short count;
	
	for (count = 0; ch!=0; ch >>= 1)
		if (ch & 01) count++;
		
	return count;
}

long MemBitCount(unsigned char *pCh, long n)
{
	long count;
	
	for (count = 0; n>0; n--) {
  		//printf("BitCount(%d) = %d\n",  *pCh, BitCount(*pCh));
		count += BitCount(*pCh);
		pCh++;
	}
		
	return count;
}


/* ----------------------------------------------------------- Special keyboard things -- */

Boolean CheckAbort()
{
	EventRecord evt;  int ch;  Boolean quit = False;

	if (!WaitNextEvent(keyDownMask+app4Mask, &evt, 0, NULL)) return(quit);

	switch(evt.what) {
		case keyDown:
			ch = evt.message & charCodeMask;
			if (ch=='.' && (evt.modifiers&cmdKey)!=0) quit = True;
			break;
		case app4Evt:
			DoSuspendResume(&evt);
			break;
	}
	
	return(quit);
}

/* Determine if a mouse click is a double click, using given position tolerance, all
in current port. */

Boolean IsDoubleClick(Point pt, short tol, long now)
{
	Rect box;  Boolean ans=False;
	static long lastTime;  static Point thePt;
	
	if ((unsigned long)(now-lastTime)<GetDblTime()) {
		SetRect(&box,thePt.h-tol,thePt.v-tol,thePt.h+tol+1,thePt.v+tol+1);
		ans = PtInRect(pt,&box);
		lastTime = 0;
		thePt.h = thePt.v = 0;
	}
	else {
		lastTime = now;
		thePt = pt;
	}
	return ans;
}


/* -------------------------------------------------------------------- Other routines -- */

/* If any of the variable argument scrap types are available for pasting from the scrap,
deliver the first one.  Otherwise, deliver 0.  For example,

    if (whichType = CanPaste(3,'TEXT','PICT','STUF')) ...

There can be any number of types in the list, as long as the preceding count, <n>, is
correct. Caveat: the code for picking up the following arguments assumes they're
stored in order after the count and are all the correct length! FIXME: This should
REALLY be replaced by the ANSI/ISO variable arguments mechanism declared in <stdarg.h>.

Carbon Note:
GetScrapFlavorFlags can return 
  noScrapErr                    = -100, // No scrap exists error
  noTypeErr                     = -102  // No object of that type in scrap

NB: The line
		if (err >= -1) return(*nextType);
testing the result of GetScrap does not square with documentation, which states:

	function result [of GetScrap]
	The length (in bytes) of the data or a negative function result that indicates the error. 	
*/

OSType CanPaste(short n, ...)
{
	OSType *nextType;
	ScrapRef scrap;
	OSStatus err;
	
	GetCurrentScrap(&scrap);
	
	nextType = (OSType *) ( ((char *)(&n)) + sizeof(n) );	/* Second argument */
	while (n-- > 0) {
		ScrapFlavorFlags flavorFlags;
		ScrapFlavorType flavorType = (ScrapFlavorType)*nextType;

		err = GetScrapFlavorFlags(scrap, flavorType, &flavorFlags);
		if (err >= noErr) return(*nextType);
		nextType++;	
	}
	return(0L);
}


/* ----------------------------------------------------------------------- Files, etc. -- */

/* UseStandardType should be called any number of times with a file type argument just
before calling GetInputName(), below.  It declares for the benefit of GetInputName()
those types of document files to display in the standard input dialog. */

static short numGetTypes = 0;
static OSType inputType[MAXINPUTTYPE];

void UseStandardType(OSType type)
{
	if (numGetTypes < MAXINPUTTYPE) inputType[numGetTypes++] = type;
}

void ClearStandardTypes()
{
	numGetTypes = 0;
}


short GetInputName(char */*prompt*/, Boolean /*newButton*/, unsigned char *name,
					short */*wd*/, NSClientDataPtr nsData)
{
	OSStatus err = noErr;
	
	//err = OpenFileDialog(CREATOR_TYPE_NORMAL, numGetTypes, inputType, nsData);
	err = OpenFileDialog(kNavGenericSignature, numGetTypes, inputType, nsData);
	
	strncpy((char *)name, nsData->nsFileName, 255);
	CToPString((char *)name);
	
	if (err == noErr) {
		if (nsData->nsOpCancel)
			return OP_Cancel;
			
		return OP_OpenFile;
	}
		
	return OP_Cancel;
}

Boolean GetOutputName(short /*promptsID*/, short /*promptInd*/, unsigned char *name,
						short */*wd*/, NSClientDataPtr nsData)
{
	OSStatus anErr;
	CFStringRef	defaultName;
	
	//defaultName = CFSTR("Nightingale Document");
	defaultName = CFStringCreateWithPascalString(NULL, name, smRoman);

	anErr = SaveFileDialog( NULL, defaultName, 'TEXT', 'BYRD', nsData );
	
	if (anErr != noErr) return False;
		
	return (!nsData->nsOpCancel);
}


/* Return the version number string (in Pascal string form), from Info.plist. */

StringPtr VersionString(StringPtr verPStr)
{
	char verStr[64];
	const char *bundleVersionStr;
	
	/* Get version number from main bundle (Info.plist); this is the string value
	   for key "CFBundleVersion", a.k.a. "Bundle version". */
	
	CFStringRef verRef = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
										kCFBundleVersionKey);
	bundleVersionStr = CFStringGetCStringPtr(verRef, kCFStringEncodingMacRoman);
	strcpy(verStr, "v. ");
	strcat(verStr, bundleVersionStr);
	CToPString(verStr);
	Pstrcpy(verPStr, (StringPtr)verStr);
	return verPStr;
}


/*
struct SysEnvRec {
  short               environsVersion;
  short               machineType;
  short               systemVersion;
  short               processor;
  Boolean             hasFPU;
  Boolean             hasColorQD;
  short               keyBoardType;
  short               atDrvrVersNum;
  short               sysVRefNum;
};

OSErr Gestalt (
    OSType selector, 
    SInt32 *response
);

*/

OSErr SysEnvirons(
  short        versionRequested,
  SysEnvRec *  theWorld)
{
	SInt32 gestaltResponse;
	OSErr err = noErr;
   
	theWorld->environsVersion = versionRequested;
	
	/* This selector is not available in Carbon. :-(  */
	
	err = Gestalt(gestaltMachineType, &gestaltResponse);
	if (err == noErr)
		theWorld->machineType = gestaltResponse;
	else
		theWorld->machineType = gestaltPowerMacG3;
		
	err = Gestalt(gestaltSystemVersion, &gestaltResponse);
	if (err == noErr)
		theWorld->systemVersion = gestaltResponse;
	
	err = Gestalt(gestaltProcessorType, &gestaltResponse);
	if (err == noErr)
		theWorld->processor = gestaltResponse;
		
	err = Gestalt(gestaltFPUType, &gestaltResponse);
	if (err == noErr)
		theWorld->hasFPU = gestaltResponse>0;
		
	err = Gestalt(gestaltQuickdrawVersion, &gestaltResponse);
	if (err == noErr)
		theWorld->hasColorQD = gestaltResponse>gestaltOriginalQD;

	err = Gestalt(gestaltKeyboardType, &gestaltResponse);
	if (err == noErr)
		theWorld->keyBoardType = gestaltResponse;
			
	err = Gestalt(gestaltATalkVersion, &gestaltResponse);
	if (err == noErr) {
		char *p = (char *)&theWorld->atDrvrVersNum;
		char *q = (char *)&gestaltResponse;
		*p++ = *q++; *p++ = *q++;
	}
		
	/* This selector does not even exist for Gestalt. :-( :-(  */
	
//	err = Gestalt(gestaltSysVRefNum, &gestaltResponse);
//	if (err == noErr) theWorld->sysVRefNum = gestaltResponse;

	theWorld->sysVRefNum = 0;
	
 	return noErr;
}
