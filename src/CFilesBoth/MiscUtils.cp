/*	MiscUtils.c
	These are miscellaneous routines that are generally-useful extensions to the Mac
	Toolbox routines; formerly called EssentialTools.c. Doug McKenna, 1989; numerous
	revisions by Donald Byrd. Most of the functions that were originally in this file
	have been moved to DialogUtils.c or UIFUtils.c. */

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

static void	DrawSelBox(short index);

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

/*
 *	This requests a non-relocatable n-byte block of memory from the Toolbox,
 *	and zeroes it prior to delivering.  If not enough memory, it goes to
 *	error code, which may or may not work, since the error dialog needs
 *	memory too.
 */

void *NewZPtr(Size n)		/* Allocate n bytes of zeroed memory; deliver ptr */
{
	char *p, *q;  void *ptr;

	ptr = NewPtr(n);
	if (!ptr) /* NoMemory(memFullErr) */ SysBeep(1);
	 else {
		p = (char *)ptr; q = p + n;
		while (p < q) *p++ = '\0';
		}
	return(ptr);
}

/*
 *	GrowMemory() is a very simple grow zone function designed to prevent the ROM or
 *	resident software such as desk accessories from generating out-of-memory errors.
 *	GrowMemory assumes we've pre-allocated an otherwise unused block of memory (this
 *	should normally be done at initialize time); we free it here when the Memory Manager
 *	calls this routine as the installed Grow Zone procedure. We also note that memory
 *	is low for the benefit of possible error messages to the user. See Inside
 *	Macintosh v2, p.42-3.
 */

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

/*
 *	Determine if a mouse click is a double click, using given position tolerance,
 *	all in current port.
 */

Boolean IsDoubleClick(Point pt, short tol, long now)
{
	Rect box; Boolean ans=False;
	static long lastTime; static Point thePt;
	
	if (now-lastTime<GetDblTime()) {
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

/* If any of the variable argument scrap types are available for pasting from
the scrap, deliver the first one.  Otherwise, deliver 0.  For example,

    if (whichType = CanPaste(3,'TEXT','PICT','STUF')) ...

There can be any number of types in the list, as long as the preceding count,
n, is correct. Note that the code for picking up the following arguments
assumes they're stored in order after the count (a safe assumption in THINK C)
and are all the correct length. ??This should REALLY be replaced by the ANSI/ISO
variable arguments mechanism declared in <stdarg.h>. 

Carbon Note:
GetScrapFlavorFlags can return 
  noScrapErr                    = -100, // No scrap exists error
  noTypeErr                     = -102  // No object of that type in scrap

NB:  
The line
			if (err >= -1) return(*nextType);
testing result of GetScrap does not square with documentation, which states:

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
#if TARGET_API_MAC_CARBON
		ScrapFlavorFlags flavorFlags;
		ScrapFlavorType flavorType = (ScrapFlavorType)*nextType;

		err = GetScrapFlavorFlags(scrap, flavorType, &flavorFlags);
		if (err >= noErr) return(*nextType);
		nextType++;	
#else
		err = GetScrap(NULL, *nextType, &offset);
		if (err >= -1) return(*nextType);
		nextType++;	
#endif
	}
	return(0L);
}

/*
 *	Draw a grow icon at given position in given port.  Doesn't change the current port.
 *	The icon is assumed the usual 16 by 16 icon.  If drawit is False, erase the area
 *	it appears in but leave box framed. This function is not used in Ngale 5.7.
 */

void DrawGrowBox(WindowPtr w, Point pt, Boolean drawit)
{
	Rect r,t,grow;  GrafPtr oldPort;
	
	GetPort(&oldPort); SetPort(GetWindowPort(w));
	
	SetRect(&grow,pt.h,pt.v,pt.h+SCROLLBAR_WIDTH+1,pt.v+SCROLLBAR_WIDTH+1);
	t = grow; InsetRect(&t,1,1);
	EraseRect(&t);
	FrameRect(&grow);
	if (drawit) {
		SetRect(&r,3,3,10,10);
		SetRect(&t,5,5,14,14);
		OffsetRect(&r,grow.left,grow.top);
		OffsetRect(&t,grow.left,grow.top);
		FrameRect(&t);
		EraseRect(&r); FrameRect(&r);
	}
	SetPort(oldPort);
}


/* ----------------------------------------------------------------------- Files, etc. -- */

/*
 *	This routine should be called any number of times with a file type argument
 *	just prior to calling GetInputName(), below.  It declares for the benefit
 *	of GetInputName() those types of document files to display in the standard
 *	input dialog.
 */

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

#ifdef TARGET_API_MAC_CARBON_FILEIO

short GetInputName(char */*prompt*/, Boolean /*newButton*/, unsigned char *name, short */*wd*/, NSClientDataPtr nsData)
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

Boolean GetOutputName(short /*promptsID*/, short /*promptInd*/, unsigned char *name, short */*wd*/, NSClientDataPtr nsData)
{
	OSStatus anErr;
	CFStringRef	defaultName;
	
	//defaultName = CFSTR("Nightingale Document");
	defaultName = CFStringCreateWithPascalString(NULL,name,smRoman);

	anErr = SaveFileDialog( NULL, defaultName, 'TEXT', 'BYRD', nsData );
	
	if (anErr != noErr) return False;
		
	return (!nsData->nsOpCancel);
}
	
#else


/*
 *	Filter function that allows only document files of our type to be displayed
 *	in Standard Input dialog.
 */

static pascal Boolean OurFilesOnly(ParmBlkPtr paramBlock)
{
	FileParam *file; FInfo stuff; short i;
	
	file = (FileParam *)paramBlock;					/* Access block as file parameters */
	stuff = file->ioFlFndrInfo;						/* Get file's finder info */
	/* Filter away all but our input files of interest */
	for (i=0; i<numGetTypes; i++)
		if (stuff.fdType == inputType[i]) return(False);
	return(True);
}

enum {
	/* getOpen = 1, */
	ourNew = 2,
	ourCancel = 3,
	getDisk = 4,
	/* getEject = 5,
	getDrive,
	getNmList,
	getScroll, */
	getLine = 9,
	getPrompt,
	getNullEvt = 100,
	getClipboard = 10,
	ourNewNew,
	getEatComma
};

/*
 *	This routine is called by standard GetInput package so we can have extra buttons
 */

static Boolean newButtonHit,noNewButton;
static short returnCode;

static pascal short DlgHook(short itemHit, DialogPtr dlog);

static pascal short DlgHook(short itemHit, DialogPtr /*dlog*/)
{
	switch(itemHit) {
		case ourNew:
		case ourNewNew:
			newButtonHit = True;
			itemHit = ourCancel;
			break;
		default:
			break;
	}
	return(itemHit);
}

/* This filters the events so that we can hilite the default button in our semi-standard
input and output dialogs below, as well as entertain various keyboard commands. */

static pascal Boolean Defilt(DialogPtr dlog, EventRecord *evt, short *itemHit);

static short selEnd = -1;

static pascal Boolean Defilt(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	int ch; Boolean ans = False,doHilite = False; WindowPtr w;
	short type; Handle hndl; Rect box;
	
	w = (WindowPtr)(evt->message);
	
	if (evt->what == activateEvt) {
		if (w == GetDialogWindow(dlog))
			SetPort(GetWindowPort(w));
		 else
			DoActivate(evt,(evt->modifiers&activeFlag)!=0,False);
	}
	
	 else if (evt->what == updateEvt) {
		if (w == GetDialogWindow(dlog)) {
			SetPort(GetWindowPort(w));
			if (noNewButton) {
				GetDialogItem(dlog,ourNewNew,&type,&hndl,&box);
				HideDialogItem(dlog,ourNewNew);
				}
			FrameDefault(dlog,OK,True);
			if (selEnd >= 0) { SelectDialogItemText(dlog,putName,0,selEnd); selEnd = -1; }
		}
		 else
			DoUpdate(w);
	}
		
	 else if (evt->what == keyDown) {
		ch = (unsigned char)evt->message;
		if (evt->modifiers & cmdKey) {
			if (isupper(ch)) ch = tolower(ch);
			if (ch == 'n' && !noNewButton) {
					doHilite = True; *itemHit = ourNewNew;
					returnCode = OP_NewFile;
			}
			 else if (ch=='.' || ch=='q') {
				doHilite = True;
				*itemHit = ourCancel;
				returnCode = (ch=='q' ? OP_QuitInstead : OP_Cancel);
			}
			ans = doHilite;
			if (ans) {
				GetDialogItem(dlog,*itemHit,&type,&hndl,&box);
				HiliteControl((ControlHandle)hndl,1);
			}
		}
	}

	return(ans);
}
	
	
/* Get the name and working directory of a file from the user. We use a
homegrown input dialog, which lets us support a New button. The function
returns an OP_ code indicating the user's wishes. */

short GetInputName(char *prompt, Boolean newButton, unsigned char *name, short *wd)
	{
		static Point pt; Rect r;
		static SFTypeList list = { 0L, 0L, 0L, 0L };
		static SFReply answer;
		short allTypes = -1,id;
		Handle dlog;
		GrafPtr oldPort;
		FileFilterUPP fileFilterUPP;
		DlgHookUPP hookUPP;
		ModalFilterUPP filterUPP;
		
		fileFilterUPP = NewFileFilterUPP(OurFilesOnly);
		hookUPP = NewDlgHookUPP(DlgHook);
		filterUPP = NewModalFilterUPP(Defilt);
		if (filterUPP == NULL) {
			if (hookUPP == NULL)
				DisposeDlgHookUPP(hookUPP);
			if (fileFilterUPP == NULL)
				DisposeFileFilterUPP(fileFilterUPP);
			MissingDialog(OPENFILE_DLOG);
			return(OP_Cancel);
			}
		
		GetPort(&oldPort);

		/* Configure according to type of input dialog desired */

		id = OPENFILE_DLOG;
		noNewButton = !newButton;
		
		/* Get dialog resource, and if all is OK, center dialog before it gets drawn */
		
		dlog = GetResource('DLOG',id);
		if (dlog==NULL || *dlog==NULL || ResError()!=noErr) {
			DisposeModalFilterUPP(filterUPP);
			DisposeDlgHookUPP(hookUPP);
			DisposeFileFilterUPP(fileFilterUPP);
			MissingDialog(id);
			return(OP_Cancel);
		}
		else {
			WindowPtr w; Rect scrn,r;

			LoadResource(dlog);
			r = *(Rect *)(*dlog);
			w = TopDocument;
			if (w)
				GetMyScreen(&w->portRect,&scrn);
			else
				scrn = qd.screenBits.bounds;
			CenterRect(&r,&scrn,&r);
			pt.h = r.left; pt.v = r.top;
		}
		
		/* Entertain dialog: a homebrew one with prompt and New button */
		
		returnCode = OP_Cancel;
		
		CParamText(prompt,"","","");
		newButtonHit = False;
		SFPCGetFile(pt,prompt,fileFilterUPP,allTypes,list,hookUPP,&answer,id,
						filterUPP);
		if (newButtonHit) returnCode = OP_NewFile;
		
		/* Return the file name and working directory if user said okay */
		
		if (answer.good) {
			SetVol(NULL,*wd = answer.vRefNum);
			Pstrcpy(name,answer.fName);
			UpdateAllWindows();
			returnCode = OP_OpenFile;
			}
		 else {
			/* returnCode set by filter functions */
			}
		
		numGetTypes = 0;		/* Prepare for next time */
		SetPort(oldPort);
		
		DisposeModalFilterUPP(filterUPP);
		DisposeDlgHookUPP(hookUPP);
		DisposeFileFilterUPP(fileFilterUPP);
		return(returnCode);
	}

/* Ask user for a possible output file; return True if OK to proceed, False if CANCEL.
Change the output file prompt according to id and i, and place name and working
directory in name and wd.  When called, name has a default name in it that can be
modified according to the prompt type before displaying in the dialog (item 7). */

Boolean GetOutputName(short promptsID, short promptInd, unsigned char *name, short *wd)
	{
		Point pt; Rect r,scrn; short id,len;
		static SFReply answer; WindowPtr w;
		Handle dlog; Str255 str;
		void *hookFunc = NULL;

		dlog = GetResource('DLOG',putDlgID);
		if (dlog) {
			r = *(Rect *)(*dlog);
			w = TopDocument;
			if (w)
				GetMyScreen(&w->portRect,&scrn);
			else
				scrn = qd.screenBits.bounds;
			CenterRect(&r,&scrn,&r);
			pt.h = r.left; pt.v = r.top;
			}
		
		/* Entertain dialog */
		
		GetIndString(str,promptsID,promptInd);
		
		ParamText("\p","\p","\p","\p");
		
		SFPutFile(pt,str,name,hookFunc,&answer);
		
		/* Return the file name and working directory, and set it as well */
		
		if (answer.good) {
			SetVol(NULL,*wd = answer.vRefNum);
			Pstrcpy(name,answer.fName);
			UpdateAllWindows();
			answer.good = 1;
			}

		return(answer.good);
	}

#endif // TARGET_API_MAC_CARBON_FILEIO

/* -------------------------------------------------------------------------------------- */

/*
 *	Draw the current selection box into the current port.  This is called during
 *	update and redraw events.  Does nothing if the current selection rectangle is empty.
 */

void DrawTheSelection()
{
	switch (theSelectionType) {
		case MARCHING_ANTS:
			PenMode(patXor);
			DrawSelBox(0);
			PenNormal();
			break;
		case SWEEPING_RECTS:
			PenMode(patXor);
			DrawTheSweepRects();
			PenNormal();
			break;
		case SLURSOR:
			PenMode(patXor);
			DrawTheSlursor();
			PenNormal();
			break;
		default:
			;
	}
}

/*
 *	Frame the current selection box, using the index'th pattern, or 0 for the
 *	last pattern used.
 */

static void DrawSelBox(short index)
{
	static Pattern pat;
			
	if (index) GetIndPattern(&pat,MarchingAntsID,index);
	PenPat(&pat);
	
	FrameRect(&theSelection);
}

/*
 *	Return the address of a static copy of the 'vers' resource's short string (Pascal).
 *	We assume a 'vers' resource ID of 1.  Delivers "\p??" if it finds a problem.
 */

typedef struct {
	char bytes[4];
	short countryCode;
	/* char shortStr[256]; */
} versHeader;

#define MAXVERS 32

unsigned char *VersionString()
{
	Handle vers;
	unsigned char *p;
	static unsigned char versStr[MAXVERS];
	short curResFile;

	curResFile = CurResFile();
	UseResFile(appRFRefNum);

	Pstrcpy(versStr,"\p??");		/* Start with default for any errors */
	
	vers = GetResource('vers',1);
	if (vers) {
		LoadResource(vers);
		if (*vers!=NULL && ResError()==noErr) {
			/* Truncate short string to MAXVERS-1 chars */
			p = (unsigned char *)((*vers) + sizeof(versHeader));
			if (*(unsigned char *)p >= MAXVERS) *(unsigned char *)p = MAXVERS-1;
			/* Copy short string to our local storage */
			Pstrcpy(versStr,p);
		}
	}

	UseResFile(curResFile);
	
	return(versStr);				/* Deliver copy of Pascal version string */
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
	
	// This selector is not available in Carbon. :(
	
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
		
	// This selector does not even exist for Gestalt. :( :(
	
//	err = Gestalt(gestaltSysVRefNum, &gestaltResponse);
//	if (err == noErr)
//		theWorld->sysVRefNum = gestaltResponse;
	theWorld->sysVRefNum = 0;
	
 	return noErr;
}
