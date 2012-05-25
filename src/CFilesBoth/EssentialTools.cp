/*	EssentialTools.c
	These are miscellaneous routines that are generally-useful extensions to the
	Mac Toolbox routines. Doug McKenna, 1989; numerous revisions by Donald Byrd. */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

// MAS : now _Prefix.pch, and already included
// #include "Nightingale.precomp.h"
#include "Nightingale_Prefix.pch"
#include <Carbon/Carbon.h>
// MAS
#include <ctype.h>

#include "Nightingale.appl.h"
#include "NavServices.h"

//#include "ConsolationDefs.h"

static pascal	Boolean OurFilesOnly(ParmBlkPtr paramBlock);
static pascal	Boolean Defilt(DialogPtr dlog, EventRecord *evt, short *itemHit);
static void	DrawSelBox(short index);
static void	SetSelectionBox(Point pt);


#ifdef THINK_C

#include <Sane.h>
pascal void fp68k(...);

/* Given a double and a buffer, format the double as a C string. The obvious way to
do this is with sprintf. But in THINK C, to avoid linking problems, we're using the
"small" version of the ANSI library, in which floating-point conversion isn't
supported. So we use Apple's SANE package to do a partial conversion into the Decimal
data structure, and then we take it from there.  Delivers its first argument. */

char *ftoaTC(char *buffer, double arg);
static char *ftoaTC(char *buffer, double arg)
{
		short n; char *dst,*src; char *point = NULL;
		DecForm dform; Decimal dcmal;
		short double sdTemp;
		
		dform.style = TRUE;			/* Convert binary to decimal record */
		dform.digits = 10;

		/*
		 *	Doug and I can't find the fp68k code for the 12-byte long double format
		 *	we're using elsewhere (and which seems best for THINK C 5), so just
		 *	convert the argument to a (8-byte) short double and pass that to fp68k.
		 */
		sdTemp = arg;
		fp68k(&dform,&sdTemp,&dcmal,FFDBL+FOB2D);

		/*
		 *	The Decimal structure contains a flag in the high order byte of
		 *	the sgn field for the sign of the result; the negative of the
		 *	the number of digits to the left of the decimal point; and a Pascal
		 *	string of the exact digits in the result, broken into a length 
		 *	field and a string data field. We format the result string in buffer.
		 */
		
		dst = buffer; src = (char *)dcmal.sig.text;
		
		 /* Deal with 0.0 separately for now, since dcmal.exp is wierdly large */
		 
		if (arg == 0.0) {
			*dst++ = '0';
			*dst++ = '.';
			*dst++ = '0';
			}
		 else if (src[0]=='N' || src[1]=='N') {
		 	*dst++ = 'N';
		 	*dst++ = 'A';
		 	*dst++ = 'N';
		 	}
		 else {
			if (dcmal.sgn & 0x100) *dst++ = '-';
			n = dcmal.sig.length + dcmal.exp;
			if (n <= 0) {
				*dst++ = '0'; point = dst; *dst++ = '.';
				while (n++ < 0) *dst++ = '0';
				n = dcmal.sig.length;
				while (n-- > 0) *dst++ = *src++;
				}
			 else {
				while (n-- > 0) *dst++ = *src++;
				point = dst; *dst++ = '.';
				n = dcmal.exp;							/* cf supra */
				while (n++ < 0) *dst++ = *src++;
				}
			}

		/* Cut off all trailing 0's past first one after decimal point, if any */
		
		if (point)
			for (src=dst-1; src>point+1; src--)
				if (*src=='0') dst--;
				 else		   break;
		*dst = '\0';
		
		return(buffer);
}

#endif


/* Given a double and a buffer, format the double as a C string. The obvious way to
do this is with sprintf, but in THINK C, we can't, for reasons explained in the
comments on ftoaTC.  Delivers its first argument. */

char *ftoa(char *buffer, double arg)
	{
#ifdef THINK_C
		return ftoaTC(buffer, arg);
#else
		sprintf(buffer, "%.2f", arg);		/* JGG: Just use 2 decimal places */
		return buffer;
#endif
	}


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
		char *p,*q; void *ptr;

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
		outOfMemory = TRUE;
		
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

#ifdef NOTYET
	/* 
	 *	This could be done with pointers, but handles are better because the Memory
	 *	Manager won't work as hard putting the new block in the "best" place, so
	 *	on the average it'll run faster.
	 */
	hndl = NewHandle(nBytes);
	ans = (hndl!=NULL);
	if (ans) {
		DisposeHandle(hndl);
		ReclaimMemory();			/* Refill the "rainy day" memory fund */
	}
	
	return(ans);
#else
	return (FreeMem()>=nBytes);
#endif
}

/*
 *	Display the watch cursor to indicate lengthy operations
 */

void WaitCursor()
	{
		static CursHandle watchHandle;
		static Boolean once = TRUE;
		
		if (once) {
			watchHandle = GetCursor(watchCursor);	/* From system resources */
			once = FALSE;
			}
		if (watchHandle && *watchHandle) SetCursor(*watchHandle);
	}

void ArrowCursor()
	{
		Cursor arrow;
		
		GetQDGlobalsArrow(&arrow);

		SetCursor(&arrow);
	}

Boolean CheckAbort()
	{
		EventRecord evt; int ch; Boolean quit = FALSE;

		if (hasWaitNextEvent) {
			if (!WaitNextEvent(keyDownMask+app4Mask,&evt,0,NULL)) return(quit);
			}
		 else
			if (!GetNextEvent(keyDownMask,&evt)) return(quit);

		switch(evt.what) {
			case keyDown:
	 			ch = evt.message & charCodeMask;
	 			if (ch=='.' && (evt.modifiers&cmdKey)!=0) quit = TRUE;
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
		Rect box; Boolean ans=FALSE;
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

/******************************************************************************
 KeyIsDown	(from THINK C class library)
 
		Determine whether or not the specified key is being pressed. Keys
		are specified by hardware-specific key code (NOT the character).
		Charts of key codes appear in Inside Macintosh, p. V-191.
 ******************************************************************************/

Boolean KeyIsDown(short theKeyCode)
{
	KeyMap theKeys;
	
	GetKeys(theKeys);					/* Get state of each key */
										
		/* Ordering of bits in a KeyMap is truly bizarre. A KeyMap is a
			16-byte (128 bits) array where each bit specifies the state
			of a key (0 = up, 1 = down). We isolate the bit for the
			specified key code by first determining the byte position in
			the KeyMap and then the bit position within that byte.
			Key codes 0-7 are in the first byte (offset 0 from the
			start), codes 8-15 are in the second, etc. The BitTst() trap
			counts bits starting from the high-order bit of the byte.
			For example, for key code 58 (the option key), we look at
			the 8th byte (7 offset from the first byte) and the 5th bit
			within that byte.	*/
		
	return( BitTst( ((char*) &theKeys) + theKeyCode / 8,
					(long) 7 - (theKeyCode % 8) )!=0 );
}

Boolean CmdKeyDown() {
		return (KeyIsDown(55));
	}

#if 0
Boolean OptionKeyDown() {
		return (KeyIsDown(58));
	}

Boolean ShiftKeyDown() {
		return (KeyIsDown(56));
	}

Boolean CapsLockKeyDown() {
		return (KeyIsDown(57));
	}

Boolean ControlKeyDown() {
		return (KeyIsDown(59));
	}
#else
Boolean OptionKeyDown() {
		return (GetCurrentKeyModifiers() & optionKey) != 0;
	}

Boolean ShiftKeyDown() {
		 return (GetCurrentKeyModifiers() & shiftKey) != 0;
	}

Boolean CapsLockKeyDown() {
		 return (GetCurrentKeyModifiers() & alphaLock) != 0;
	}

Boolean ControlKeyDown() {
		 return (GetCurrentKeyModifiers() & controlKey) != 0;
	}
	
Boolean CommandKeyDown() {
		 return (GetCurrentKeyModifiers() & cmdKey) != 0;
	}

#endif


/*
 *	Outline the given item in the given dialog to show it's the default item, or
 *	erase its outline to show its window isn't active.
 */

#if TARGET_API_MAC_CARBON
void FrameDefault(DialogPtr dlog, INT16 item, INT16)
	{
		SetDialogDefaultItem(dlog, item);
	}
#else
void FrameDefault(DialogPtr dlog, INT16 item, INT16 draw)	/* ??<draw> should be Boolean */
	{
		short type; Handle hndl; Rect box;
		GrafPtr oldPort;
		
		GetPort(&oldPort);
		SetPort(GetDialogWindowPort(dlog));
		
		GetDialogItem(dlog,item,&type,&hndl,&box);
		InsetRect(&box,-4,-4);
		PenSize(3,3);
		if (!draw) PenPat(NGetQDGlobalsWhite());
		FrameRoundRect(&box,16,16);
		PenNormal();
		
		SetPort(oldPort);
	}
#endif

/*
 *	Note the current editText item and its selection state, or restore state and item.
 *	This lets us install various editText items in dialogs while they're open,
 *	and especially when the user is typing.
 */

void TextEditState(DialogPtr dlog, Boolean save)
	{
		static short k,n,m; static TEHandle textH;
		
		if (save) {	
			k = GetDialogKeyboardFocusItem(dlog);
			textH = GetDialogTextEditHandle(dlog);			
			n = (*textH)->selStart; m = (*textH)->selEnd;
			/* Force reselection of all if any selected to current end */
			if (m == (*textH)->teLength) {
				if (m == n) n = ENDTEXT;
				m = ENDTEXT;
				}
			}
		 else
			SelectDialogItemText(dlog,k,n,m);
	}

/*
 *	These routines store various usual items into fields of dialogs.
 *	The routines that store deliver the handle of the item.  The
 *	routines that retrieve deliver FALSE or TRUE, according to 
 *	whether the edit field is empty or not.
 */

Handle PutDlgWord(DialogPtr dlog, INT16 item, INT16 val, Boolean sel)
	{
		return(PutDlgLong(dlog,item,(long)val,sel));
	}

Handle PutDlgLong(DialogPtr dlog, INT16 item, long val, Boolean sel)
	{
		unsigned char str[32];
		
		NumToString(val,str);
		return(PutDlgString(dlog,item,str,sel));
	}

Handle PutDlgDouble(DialogPtr dlog, INT16 item, double val, Boolean sel)
	{
		char str[64];
		
		return(PutDlgString(dlog,item,CToPString(ftoa(str,val)),sel));
	}

Handle PutDlgType(DialogPtr dlog, INT16 item, ResType type, Boolean sel)
	{
		unsigned char str[1+sizeof(OSType)];
		
		BlockMove(&type,str+1,sizeof(OSType));
		str[0] = sizeof(OSType);
		return(PutDlgString(dlog,item,str,sel));
	}

/*
 *	Install a string into a given text item of a given dialog and, if item is
 *	an editText item, leave it entirely selected or not, according to <sel>.
 *	In all cases, deliver handle of item.
 */

Handle PutDlgString(DialogPtr dlog, INT16 item, const unsigned char *str, Boolean sel)
	{
		short type; Handle hndl; Rect box; GrafPtr oldPort;
		
		GetPort(&oldPort); SetPort(GetDialogWindowPort(dlog));
		
		GetDialogItem(dlog,item,&type,&hndl,&box);
		SetDialogItemText(hndl,str);
		type &= ~itemDisable;
		
		if (type == editText) {
			SelectDialogItemText(dlog,item,(sel?0:ENDTEXT),ENDTEXT);
			/* It's not clear if the following InvalRect is necessary. */
			InvalWindowRect(GetDialogWindow(dlog),&box);
			}
		
		SetPort(oldPort);
		return(hndl);
	}

Handle PutDlgChkRadio(DialogPtr dlog, INT16 item, INT16 val)
	{
		short type; Handle hndl; Rect box;
		
		GetDialogItem(dlog,item,&type,&hndl,&box);
		SetControlValue((ControlHandle)hndl,val!=0);
		return(hndl);
	}

/*
 *	Retrieval routines return 0 or 1 if edit item is empty or not, and value if any
 *	or 0 if no characters in editText item.
 */

INT16 GetDlgLong(DialogPtr dlog, INT16 item, long *num)
	{
		Str255 str;

		GetDlgString(dlog,item,str);
		*num = 0;
		if (*str>0 && ((str[1]>='0' && str[1]<='9') || str[1]=='-' || str[1]=='+'))
			StringToNum(str,num);
		return(*str != 0);
	}

INT16 GetDlgWord(DialogPtr dlog, INT16 item, INT16 *num)
	{
		Str255 str; long n;

		GetDlgString(dlog,item,str);
		n = 0;
		if (*str>0 && ((str[1]>='0' && str[1]<='9') || str[1]=='-' || str[1]=='+'))
			StringToNum(str,&n);
		*num = n;
		return(*str != 0);
	}

INT16 GetDlgDouble(DialogPtr dlog, INT16 item, double *val)
	{
		Str255 str;

		GetDlgString(dlog,item,str);
		*val = 0.0;
		if (*str>0 && ((str[1]>='0' && str[1]<='9') || str[1]=='-' || str[1]=='+' || str[1]=='.'))
			*val = atof(PToCString(str));
		
		return(*str != 0);
	}

INT16 GetDlgString(DialogPtr dlog, INT16 item, unsigned char *str)
	{
		short type; Handle hndl; Rect box;
		
		GetDialogItem(dlog,item,&type,&hndl,&box);
		GetDialogItemText(hndl,str);
		return(*str != 0);
	}

INT16 GetDlgChkRadio(DialogPtr dlog, INT16 item)
	{
		short type; Handle hndl; Rect box;
		
		GetDialogItem(dlog,item,&type,&hndl,&box);
		return (GetControlValue((ControlHandle)hndl) != 0);
	}

/*
 *	Read a given editText item as a resource type, delivering TRUE if it was a
 *	legal 4-character string; FALSE otherwise.
 */

INT16 GetDlgType(DialogPtr dlog, INT16 item, ResType *type)
	{
		Str255 str;

		GetDlgString(dlog,item,str);
		PToCString(str);
		BlockMove(str+1,&type,sizeof(OSType));
		return(*str == sizeof(OSType));
	}

/*
 *	Deliver the item number of the first empty item in given range, or 0 if none.
 *	Not used in Ngale 3.0.
 */

short AnyEmptyDlgItems(DialogPtr dlog, short fromItem, short toItem)
	{
		short i; Str255 str;

		for (i=fromItem; i<=toItem; i++)
			if (!GetDlgString(dlog,i,str)) break;
		
		if (i >= toItem) i = 0;
		return(i);
	}

/*
 *	Show or hide a given item from a given dialog
 */

void ShowHideItem(DialogPtr dlog, short item, Boolean show)
{
	if (show) ShowDialogItem(dlog, item);
	 else	  HideDialogItem(dlog, item);
}

/*
 *	Deliver the number of the current editText item in given dialog if any text
 *	is selected in it, or 0 if none selected.
 */

short TextSelected(DialogPtr dlog)
	{
		TEHandle textH; short item = 0;
		
		textH = GetDialogTextEditHandle(dlog);			
		if (*textH)
			if ( (*textH)->selStart != (*textH)->selEnd )
				item = GetDialogKeyboardFocusItem(dlog);
		return(item);
	}

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

OSType CanPaste(INT16 n, ...)
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
 *	The icon is assumed the usual 16 by 16 icon.  If drawit is FALSE, erase the area
 *	it appears in but leave box framed. Not used in Ngale 3.0.
 */

void DrawGrowBox(WindowPtr w, Point pt, Boolean drawit)
	{
		Rect r,t,grow; GrafPtr oldPort;
		
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

/*
 *	Given an origin, orig, 0 <= i < n, compute the offset position, ans, for that
 *	index into n.  This is used to position multiple windows being opened at the
 *	same time.  All coords are expected to be global.  All windows are expected
 *	to be standard sized.
 */

void GetIndPosition(WindowPtr w, short i, short n, Point *ans)
	{
		short x,y; Rect box;
		
		GetGlobalPort(w,&box);
		ans->h = box.left;
		ans->v = box.top;
		
		if (n <= 0) n = 8;
		x = i / n;
		y = i % n;
		
		ans->h += 5 + 30*x + 5*y;
		ans->v += 20 + 20*y + 5*x;
	}

/*
 *	Find a nice spot in global coordinates to place a window by looking down
 *	the window list n windows for any conflicts. Larger values of n should give
 *	nicer results at the expense of slightly more running time.
 */

void AdjustWinPosition(WindowPtr w)
	{
		short n=7,i,k,xx,yy; Rect box; WindowPtr ww;
		
		GetGlobalPort(w,&box);
		xx = box.left;
		yy = box.top;
		
		i = 0;
		while (i < n) {
			k = 0;
			for (ww=FrontWindow(); ww!=NULL && k<n; ww=GetNextWindow(ww))
				if (ww!=w && IsWindowVisible(ww) && GetWindowKind(w)==DOCUMENTKIND) {
					k++;
					GetGlobalPort(ww,&box);
					if (box.left==xx && box.top==yy) {
						xx -= 5;
						yy += 20;
						i++;
						break;
						}
					}
			if (ww==NULL || k>=n) break;
			}
			
		if (i < n) MoveWindow(w,xx,yy,FALSE);
	}

/*
 *	Set ans to be the centered version of r with respect to inside.  We do this
 *	by computing the centers of each rectangle, and offseting the centeree by
 *	the distance between the two centers.  If inside is NULL, then we assume it
 *	to be the main screen.
 */

void CenterRect(Rect *r, Rect *inside, Rect *ans)
	{
		short rx,ry,ix,iy;

		rx = r->left + ((r->right - r->left) / 2);
		ry = r->top + ((r->bottom - r->top) / 2);
		
		if (inside == NULL) //inside = &qd.screenBits.bounds;
			GetQDScreenBitsBounds(inside);

		ix = inside->left + ((inside->right - inside->left) / 2);
		iy = inside->top + ((inside->bottom - inside->top) / 2);
		
		*ans = *r;
		OffsetRect(ans,ix-rx,iy-ry);
	}

/*
 *	Force a rectangle to be entirely enclosed by another (presumably) larger one.
 *	If the "enclosed" rectangle is larger, then we leave the bottom right within.
 *	margin is a slop factor to bring the rectangle a little farther in than just
 *	to the outer rectangle's border.
 */

void PullInsideRect(Rect *r, Rect *inside, short margin)
	{
		Rect ans,tmp;
		
		tmp = *inside;
		InsetRect(&tmp,margin,margin);
		
		SectRect(r,&tmp,&ans);
		if (!EqualRect(r,&ans)) {
			if (r->top < tmp.top)
				OffsetRect(r,0,tmp.top-r->top);
			 else if (r->bottom > tmp.bottom)
				OffsetRect(r,0,tmp.bottom-r->bottom);
				
			if (r->left < tmp.left)
				OffsetRect(r,tmp.left-r->left,0);
			 else if (r->right > tmp.right)
				OffsetRect(r,tmp.right-r->right,0);
			}
	}

/*
 *	Deliver TRUE if r is completely inside the bounds rect; FALSE if outside or
 *	partially outside. Not used in Ngale 3.0.
 */

Boolean ContainedRect(Rect *r, Rect *bounds)
	{
		Rect tmp;
		
		UnionRect(r,bounds,&tmp);
		return (EqualRect(bounds,&tmp));
	}

/*
 *	Given a window, compute the global coords of its port
 */

void GetGlobalPort(WindowPtr w, Rect *r)
	{
		GrafPtr oldPort; Point pt;
		
		GetWindowPortBounds(w,r);
		pt.h = r->left; pt.v = r->top;
		GetPort(&oldPort); SetPort(GetWindowPort(w)); LocalToGlobal(&pt); SetPort(oldPort);
		SetRect(r,pt.h,pt.v,pt.h+r->right-r->left,pt.v+r->bottom-r->top);
	}

/*
 *	Given a global rect, r, deliver the bounds of the screen device that best "contains"
 *	that rect. If we're running on a machine that that <hasColorQD> (almost any machine
 *	since the Mac II), we do this by scanning the device list, and for each active
 *	screen device we intersect the rect with the device's bounds, keeping track of
 *	that device whose intersection's area is the largest. If the machine doesn't
 *	<hasColorQD>, we assume it has only one screen and just deliver the usual
 *	qd.screenBits.bounds. The one problem with this is it doesn't handle some old Macs
 *	with more than one screen, e.g., SEs and SE/30s with third-party large monitors.
 * 
 *	The two arguments can point to the same rectangle. Returns the number of screens
 *	with which the given rectangle intersects unless it's a non-<hasColorQD> machine,
 *	in which case it returns 0.
 *
 *	N.B. An earlier version of this routine apparently had a feature whereby, if the
 *	caller wanted the Menu Bar area included, it could set a global <pureScreen> TRUE
 *	before call; this could easily be added again.
 */

short GetMyScreen(Rect *r, Rect *bnds)
	{
		GDHandle dev; Rect test,ans,bounds;
		long area,maxArea=0L; short nScreens = 0;
		
		bounds = GetQDScreenBitsBounds();
		bounds.top += GetMBarHeight();			/* Exclude menu bar */
		if (thisMac.hasColorQD)
			for (dev=GetDeviceList(); dev; dev=GetNextDevice(dev))
				if (TestDeviceAttribute(dev,screenDevice) &&
											TestDeviceAttribute(dev,screenActive)) {
					test = (*dev)->gdRect;
					if (TestDeviceAttribute(dev,mainScreen))
						test.top += GetMBarHeight();
					if (SectRect(r,&test,&ans)) {
						nScreens++;
						OffsetRect(&ans,-ans.left,-ans.top);
						area = (long)ans.right * (long)ans.bottom;
						if (area > maxArea) { maxArea = area; bounds = test; }
						}
					}
		*bnds = bounds;
		return(nScreens);
	}

/*
 *	Given an Alert Resource ID, get its port rectangle and center it within a given window
 *	or with a given offset if (left,top)!=(0,0).  If w is NULL, use screen bounds.  This
 *	routine should be called immediately before displaying the alert, since it might be
 *	purged.  For multi-screen devices, we look for that device on which a given window
 *	is most affiliated, and we don't allow alert outside of that device's global bounds.
 */

void PlaceAlert(short id, WindowPtr w, short left, short top)
	{
		Handle alrt; Rect r,inside,bounds;
		Boolean sect; long maxArea=0L;
		
		/*
		 * This originally used Get1Resource, but it should get the ALRT from any
		 * available resource file.
		 */  
		alrt = GetResource('ALRT',id);
		if (alrt!=NULL && *alrt!=NULL && ResError()==noErr) {
			ArrowCursor();
			LoadResource(alrt);
			r = *(Rect *)(*alrt);
			if (w) {
				GetGlobalPort(w,&inside);
				}
			 else {
			 	inside = GetQDScreenBitsBounds();
				inside.top += 36;
				}
			CenterRect(&r,&inside,&r);
			if (top) OffsetRect(&r,0,(inside.top+top)-r.top);
			if (left) OffsetRect(&r,(inside.left+left)-r.left,0);
			/*
			 *	If we're centering w/r/t a window, don't let alert get out of bounds.
			 *	We look for that graphics device that contains the window more than
			 *	any other, and use its bounds to bring the alert back into if its outside.
			 */
			if (w) {
				GetMyScreen(&inside,&bounds);
				sect = SectRect(&r,&bounds,&inside);
				if ((sect && !EqualRect(&r,&inside)) || !sect) {
					/* if (!sect) */inside = bounds;
					PullInsideRect(&r,&inside,16);
					}
				}
			LoadResource(alrt);	/* Probably not necessary, but we do it for good luck */
			*((Rect *)(*alrt)) = r;
			}
	}

/*
 *	Given a window, dlog, place it with respect to another window, w, or the screen if w is NULL.
 *	If (left,top) == (0,0) center it; otherwise place it at offset (left,top) from the
 *	centering rectangle's upper left corner.  Deliver TRUE if position changed.
 */

Boolean PlaceWindow(WindowPtr dlog, WindowPtr w, short left, short top)
	{
		Rect r,s,inside; short dx=0,dy=0;
		
		if (dlog) {
			GetGlobalPort(dlog,&r);
			s = r;
			if (w) GetGlobalPort(w,&inside);
			 else  GetMyScreen(&r,&inside);
			CenterRect(&r,&inside,&r);
			if (top) OffsetRect(&r,0,(inside.top+top)-r.top);
			if (left) OffsetRect(&r,(inside.left+left)-r.left,0);
			
			GetMyScreen(&r,&inside);
			PullInsideRect(&r,&inside,16);
			
			MoveWindow(dlog,r.left,r.top,FALSE);
			dx = r.left - s.left;
			dy = r.top  - s.top;
			}
		return(dx!=0 || dy!=0);
	}

/*
 * Center w horizontally and place it <top> pixels from the top of the screen.
 */

void CenterWindow(WindowPtr w,
					short top)	/* 0=center vertically, else put top at this position */
	{
		Rect scr,portRect; Point p;
		short margin;
		short rsize,size;

		scr = GetQDScreenBitsBounds();
		SetPort(GetWindowPort(w));
		GetWindowPortBounds(w,&portRect);
		p.h = portRect.left; p.v = portRect.top;
		LocalToGlobal(&p);

		size = scr.right - scr.left;
		rsize = portRect.right - portRect.left;
		margin = size - rsize;
		if (margin > 0) {
			margin >>= 1;
			p.h = scr.left + margin;
			}
		size = scr.bottom - scr.top;
		rsize = portRect.bottom - portRect.top;
		margin = size - rsize;
		if (margin > 0) {
			margin >>= 1;
			p.v = scr.top + margin;
			}
		MoveWindow(w,p.h,(top?top:p.v),FALSE);
	}

/*
 *	Dialogs like to use null events to force any editText items to have a
 *	blinking caret.  However, if our dialogs have special windowKind
 *	fields, they can confuse the toolbox dialog calls.  So we use this routine
 *	to temporarily reset the windowKind field before calling the toolbox to
 *	blink the caret.
 */

void BlinkCaret(DialogPtr dlog, EventRecord *evt)
	{
		short kind,itemHit; DialogPtr foo;
		
		kind = GetWindowKind(GetDialogWindow(dlog));
		SetWindowKind(GetDialogWindow(dlog),dialogKind);
		if (IsDialogEvent(evt)) DialogSelect(evt,&foo,&itemHit);
		SetWindowKind(GetDialogWindow(dlog),kind);
	}

/*
 *	Draw a series of rectangles interpolated between two given rectangles, from
 *	small to big if zoomUp, from big to small if not zoomUp.  As described in
 *	TechNote 194, we draw not into WMgrPort, but into a new port covering the
 *	same area.  Both rectangles should be specified in global screen coordinates.
 */

static short blend1(short sc, short bc);
static Fixed fract,factor,one;

static short blend1(short smallCoord, short bigCoord)
	{
		Fixed smallFix,bigFix,tmpFix;
		
		smallFix = one * smallCoord;		/* Scale up to fixed point */
		bigFix	 = one * bigCoord;
		tmpFix	 = FixMul(fract,bigFix) + FixMul(one-fract,smallFix);
		
		return(FixRound(tmpFix));
	}

/*
 *	Zoom between two rectangles on the desktop (i.e. in global coordinates)
 */

void ZoomRect(Rect *smallRect, Rect *bigRect, Boolean zoomUp)
	{
		short i,zoomSteps = 16;
		Rect r1,r2,r3,r4,rgnBBox;
		GrafPtr oldPort,deskPort;
		
		RgnHandle grayRgn = NewRgn();
		RgnHandle deskPortVisRgn = NewRgn();		
			
  		GetPort(&oldPort);
  		deskPort = CreateNewPort();
  		
  		if (deskPort == (GrafPtr)NIL)
  			Debugger();
  			
		GetPortVisibleRegion(deskPort, deskPortVisRgn);
  		grayRgn = GetGrayRgn();
   		
   	if (grayRgn == (RgnHandle)NIL)
  			Debugger();
 		
  		CopyRgn(grayRgn,deskPortVisRgn);
  		
  		GetRegionBounds(grayRgn,&rgnBBox);  		
  		SetPortBounds(deskPort,&rgnBBox);
  		
  		DisposeRgn(grayRgn);
  		DisposeRgn(deskPortVisRgn);
  		
  		SetPort(deskPort);
  		PenPat(NGetQDGlobalsGray());
  		PenMode(notPatXor);

  		one = 65536;						/* fixed point 'const' */
  		if (zoomUp) {
  			r1 = *smallRect;
  			factor = FixRatio(6,5);			/* Make bigger each time */
  			fract =  FixRatio(541,10000);	/* 5/6 ^ 16 = 0.540877 */
  			}
  		 else {
  			r1 = *bigRect;
  			factor = FixRatio(5,6);			/* Make smaller each time */
  			fract  = one;
  			}
  		
		r2 = r1;
		r3 = r1;
		FrameRect(&r1);						/* Draw initial image */

		for (i=0; i<zoomSteps; i++) {
			r4.left   = blend1(smallRect->left,bigRect->left);
			r4.right  = blend1(smallRect->right,bigRect->right);
			r4.top	  = blend1(smallRect->top,bigRect->top);
			r4.bottom = blend1(smallRect->bottom,bigRect->bottom);
			
			FrameRect(&r4);					/* Draw newest */
			FrameRect(&r1);					/* Erase oldest */
			r1 = r2; r2 = r3; r3 = r4;
			
			fract = FixMul(fract,factor);	/* Bump interpolation factor */
			}
		
		FrameRect(&r1);						/* Erase final image */
		FrameRect(&r2);
		FrameRect(&r3);
		
		PenNormal();
		DisposePort(deskPort);
		SetPort(oldPort);
	}

/*
 *	Enable or disable a menu item
 */

void XableItem(MenuHandle menu, INT16 item, INT16 enable)	/* ??<enable> should be Boolean */
	{
		if (enable) EnableMenuItem(menu,item);
		 else		DisableMenuItem(menu,item);
	}

/*
 *	For disabling or enabling entire menus, we call this any number of times
 *	before calling UpdateMenuBar() to make the changes all at once (if any).
 */

static Boolean menuBarChanged;

void UpdateMenu(MenuHandle menu, Boolean enable)
	{
		if (menu)
			if (enable) {
				if (!IsMenuItemEnabled(menu, 0)) {
//				if (!((**menu).enableFlags & 1)) {		/* Bit 0 is for whole menu */
					EnableMenuItem(menu,0);
					menuBarChanged = TRUE;
					}
				}
			 else
				if (IsMenuItemEnabled(menu, 0)) {
//				if ((**menu).enableFlags & 1) {
					DisableMenuItem(menu,0);
					menuBarChanged = TRUE;
					}
	}

void UpdateMenuBar()
	{
		if (menuBarChanged) DrawMenuBar();
		menuBarChanged = FALSE;
	}

/*
 *	Erase and invalidate a rectangle in the current port
 */

void EraseAndInval(Rect *r)
	{
		GrafPtr port;
		WindowPtr w;
		
		GetPort(&port);
		w = GetWindowFromPort(port);
		
		EraseRect(r);
		InvalWindowRect(w,r);
	}

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
		if (numGetTypes < MAXINPUTTYPE)
			inputType[numGetTypes++] = type;
	}

void ClearStandardTypes()
	{
		numGetTypes = 0;
	}

#ifdef TARGET_API_MAC_CARBON_FILEIO

INT16 GetInputName(char */*prompt*/, Boolean /*newButton*/, unsigned char *name, short */*wd*/, NSClientDataPtr nsData)
	{
		OSStatus 			err = noErr;
		
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
		
		if (anErr != noErr)
			return FALSE;
			
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
			if (stuff.fdType == inputType[i]) return(FALSE);
		return(TRUE);
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
				newButtonHit = TRUE;
				itemHit = ourCancel;
				break;
			default:
#ifdef NOMORE
				newButtonHit = FALSE;
#endif
				break;
			}
		return(itemHit);
	}

/* This filters the events so that we can hilite the default button in our
semi-standard input and output dialogs below, as well as entertain various
keyboard commands. */

pascal Boolean Defilt(DialogPtr dlog, EventRecord *evt, short *itemHit);

static short selEnd = -1;

static pascal Boolean Defilt(DialogPtr dlog, EventRecord *evt, short *itemHit)
	{
		int ch; Boolean ans = FALSE,doHilite = FALSE; WindowPtr w;
		short type; Handle hndl; Rect box;
		
		w = (WindowPtr)(evt->message);
		
		if (evt->what == activateEvt) {
			if (w == GetDialogWindow(dlog))
				SetPort(GetWindowPort(w));
			 else
				DoActivate(evt,(evt->modifiers&activeFlag)!=0,FALSE);
			}
		
		 else if (evt->what == updateEvt) {
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetWindowPort(w));
				if (noNewButton) {
					GetDialogItem(dlog,ourNewNew,&type,&hndl,&box);
#ifndef VIEWER_VERSION
#define DIM_NO_NEW
#endif
#ifdef DIM_NO_NEW
					HiliteControl((ControlHandle)hndl,255);
#else
					HideDialogItem(dlog,ourNewNew);
#endif
					}
				FrameDefault(dlog,OK,TRUE);
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
			 			doHilite = TRUE; *itemHit = ourNewNew;
			 			returnCode = OP_NewFile;
			 		}
			 	 else if (ch=='.' || ch=='q') {
			 	 	doHilite = TRUE;
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

INT16 GetInputName(char *prompt, Boolean newButton, unsigned char *name, short *wd)
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
		newButtonHit = FALSE;
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

/* Ask user for a possible output file; return TRUE if OK to proceed, FALSE if CANCEL.
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

/*
 *	Draw the current selection box into the current port.  This is called
 *	during update and redraw events.  Does nothing if the current selection
 *	rectangle is empty.
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
		Handle vers; unsigned char *p;
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
