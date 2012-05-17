/*
 *	Handle Manager, by Doug McKenna.
 *
 *	Replace every call to NewHandle, etc. in your source code with a call to
 *	OurNewHandle, and every call to DisposeHandle with a call to OurDisposeHandle.
 *	Both these check their arguments for various kinds of illegality.
 */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 *
 * Copyright © 1988-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

// MAS
#include <Carbon/Carbon.h>

#ifdef __MWERKS__
	/* JGJan98Changes. I don't understand this. --DAB */
	#undef NewHandle
	#undef DisposeHandle
	#undef DetachResource
	#undef RemoveResource
	#undef HandToHand
	#undef OpenPicture
	#undef KillPicture
#else
#include "NightMacHeaders.h"
#endif

/* Useful constants */

#ifndef TRUE
#define TRUE		1
#endif
#ifndef FALSE
#define FALSE		0
#endif

/* Number of handles to expand our internal table size by each time */

#define BUMPERSIZE		32

/* Private variables */

static Handle **handleTable;		/* Table of currently allocated Handles, initially NULL */
static long numHandles;				/* Current logical size of Handle table, initially 0 */
static Boolean	skipRegister;		/* If this is TRUE, then we don't register next handle */

/* Public routines */

Handle		OurNewHandle(long size);
void		OurDisposeHandle(Handle theHandle);
void		OurDetachResource(Handle theHandle);
void		OurRemoveResource(Handle theHandle);
OSErr		OurHandToHand(Handle *theHandle);
Boolean		SkipRegistering(Boolean);
PicHandle	OurOpenPicture(Rect *frame);
void		OurKillPicture(PicHandle hndl);
Handle		**GetHandleTable(long *pNumHandles);

/* Private routines */

static void		ExpandHandleTable(long dHandles);
static void		RegisterHandle(Handle theHandle);
static void		SignHandleOut(Handle theHandle, char *msg);
static void		GarbageCollect(void);

/* Error routine to call in outside world */

extern void MayErrMsg(char *msg, ...);
extern Boolean	OptionKeyDown(void);
extern Boolean	CmdKeyDown(void);

/*
 *	Expand our handle table by given number of slots, or die.  If number of slots is < 0,
 *	decrease the size of the table by that amount.
 */

static void ExpandHandleTable(long num) {

	long newNum;
	Handle *h;
	long iHandle;
	
	newNum = numHandles + num;
	if (newNum < 0) newNum = 0;
	
	if (handleTable)
		SetHandleSize((Handle)handleTable,newNum*sizeof(Handle));
	 else
		handleTable = (Handle **)NewHandle(newNum*sizeof(Handle));		/* First call only */
	
	if (MemError()) {
		DisposeHandle((Handle)handleTable);
		MayErrMsg("ExpandHandleTable: Ran out of memory");
		ExitToShell();
		}
	
	/* Now set all newly allocated free slots to empty */
	
	iHandle = numHandles;
	h = (*handleTable) + iHandle;
	while (iHandle++ < newNum)
		*h++ = NULL;
	
	numHandles = newNum;				/* Successful expansion: mark new size */
	}

/*
 *	Allocate a handle for the given size in bytes, and record allocated handle internally
 *	so we can tell if deallocated handles are legal or not.
 */

Handle OurNewHandle(long size) {

	Handle theHandle;

	/* Do reality check on handle size */
	
	if (size < 0) {
		MayErrMsg("OurNewHandle: size < 0");
		size = 0;
		}
	
	/* Allocate the new handle if possible */
	
	theHandle = NewHandle(size);
	if (theHandle == NULL)
		return(NULL);
	
	/* Good allocation: find a free slot and record it in our table */
	
	RegisterHandle(theHandle);
	
	/* And pass the handle back to caller as if they had called NewHandle like they thought they had */
	
	return(theHandle);
	}

/*
 *	Call this to keep subsequent calls to RegisterHandle (from OurNewHandle, or whatever)
 *	from registering handles.  This is useful for times when you know a handle is
 *	going to be disposed from within some compiled part of the program (like a CODE
 *	resource) but allocated via a usual call to OurNewHandle.
 */

Boolean SkipRegistering(Boolean how) {

		Boolean old = skipRegister;
		
		skipRegister = how;
		return(old);
	}

/*
 *	Put the given handle into our table, growing the table if necessary
 */

static void RegisterHandle(Handle theHandle) {

	long iHandle,topHandle;
	Handle *h;
	static Boolean once = TRUE;
	
	if (skipRegister)
		return;
	
	if (once) {
		ExpandHandleTable(BUMPERSIZE);	/* Initialize the table before searching/stuffing it */
		once = FALSE;
		}
	
	h = *handleTable;
	iHandle = numHandles;
	
	while (iHandle-- > 0)
		if (*h++ == NULL) {
			*--h = theHandle;			/* Stuff it in slot and return */
			return;
			}
	
	/* No free slots: append to table */
	
	topHandle = numHandles;				/* Record it before it gets bumped */
	ExpandHandleTable(BUMPERSIZE);		/* Always returns successfully */
	
	/* Now we're guaranteed to have a free slot: stuff slot and return */
	
	h = (*handleTable) + topHandle;		/* Recompute old position, since memory moved */
	*h = theHandle;						/* Register the handle in our table */
	}

/*
 *	Whenever a resource is converted to a generic handle by DetachResource, we need to
 *	register it in our table also, since it then becomes eligible for DisposeHandle just
 *	like other handles. Disposing of a still attached resource will then get caught.
 */

void OurDetachResource(Handle theHandle) {

		short err;
		
		/* Check to make sure it's legal */
		
		if (theHandle == NULL) {
			MayErrMsg("OurDetachResource: NULL Handle");
			if (OptionKeyDown() || CmdKeyDown()) DebugStr("\p;sc");
			return;
			}
		
		/* Detach it and report any Resource Manager errors */
		
		DetachResource(theHandle);
		if (err = ResError()) {
			unsigned char numStr[16];
			MayErrMsg("OurDetachResource: ResError");
			NumToString((long)err,numStr);
			if (OptionKeyDown() || CmdKeyDown()) DebugStr(numStr);
			}
		
		/* Now it's a normal handle, register it as if we had just allocated it */
		
		RegisterHandle(theHandle);
	}

/*
 *	A resource can be converted to a generic handle by RemoveResource as well as
 *	DetachResource.
 */

void OurRemoveResource(Handle theHandle) {

		short err;
		
		/* Check to make sure it's legal */
		
		if (theHandle == NULL) {
			MayErrMsg("OurRemoveResource: NULL Handle");
			if (OptionKeyDown() || CmdKeyDown()) DebugStr("\p;sc");
			return;
			}
		
		/* Remove it and report any Resource Manager errors */
		
		RemoveResource(theHandle);
		if (err = ResError()) {
			unsigned char numStr[16];
			MayErrMsg("OurRemoveResource: ResError");
			NumToString((long)err,numStr);
			if (OptionKeyDown() || CmdKeyDown()) DebugStr(numStr);
			}
		
		/* Now it's a normal handle, register it as if we had just allocated it */
		
		RegisterHandle(theHandle);
	}

/*
 *	Duplicate the given Handle, and register its duplicate in our table.  Makes
 *	no assumption about the source (it could be a resource).
 */

OSErr OurHandToHand(Handle *theHandle) {

	short err;
	
	if (theHandle == NULL) {
		MayErrMsg("OurHandToHand: NULL Handle");
		if (OptionKeyDown() || CmdKeyDown()) DebugStr("\p;sc");
		return(0);
		}
	
	err = HandToHand(theHandle);
	if (err) {
			unsigned char numStr[16];
		MayErrMsg("OurHandToHand: Memory Manager error");
		NumToString((long)err, numStr);
		if (OptionKeyDown() || CmdKeyDown()) DebugStr(numStr);
		}
	 else
		RegisterHandle(*theHandle);
	
	return(err);
	}

/*
 *	Open a new picture with the given frame, and register the ensuing handle
 */

PicHandle OurOpenPicture(Rect *frame) {

		PicHandle pict;
		
		pict = OpenPicture(frame);
		
		if (pict)
			RegisterHandle((Handle)pict);
		
		return(pict);
	}

/*
 *	 Return the given picture to its primordial soup, and deregister the handle
 */

void OurKillPicture(PicHandle thePict) {

		short err;
		
		/* Check for NULL handle */
		
		if (thePict == NULL) {
			MayErrMsg("OurKillPicture: Attempt to dispose NULL handle");
			if (OptionKeyDown() || CmdKeyDown()) DebugStr("\p; sc");
			return;
			}
		
		/* Ditch the given handle's data */
		
		KillPicture(thePict);
		
		if (err = MemError()) {
			/* Probably wasn't a handle */
			unsigned char numStr[16];
			NumToString((long)err,numStr);
			MayErrMsg("OurKillPicture: MemError from KillPicture");
			if (OptionKeyDown() || CmdKeyDown()) DebugStr(numStr);
			return;
			}
		
		SignHandleOut((Handle)thePict,"OurKillPicture: Picture disposed but unrecognized");
	}
			
/*
 *	Dispose of a registered handle, with reality checks.
 */

void OurDisposeHandle(Handle theHandle) {

		short err;
		
		/* Check for NULL handle */
		
		if (theHandle == NULL) {
			MayErrMsg("OurDisposeHandle: Attempt to dispose NULL handle");
			if (OptionKeyDown() || CmdKeyDown()) DebugStr("\p; sc");
			return;
			}
		
		/* Ditch the given handle's data */
		
		DisposeHandle(theHandle);
		
		if (err = MemError()) {
			/* Probably wasn't a handle */
			unsigned char numStr[16];
			NumToString((long)err,numStr);
			MayErrMsg("OurDisposeHandle: MemError from DisposeHandle");
			if (OptionKeyDown() || CmdKeyDown()) DebugStr(numStr);
			return;
			}
		
		SignHandleOut(theHandle,"OurDisposeHandle: Handle disposed but unrecognized");
	}

/*
 *	Remove a registered handle expected to be in our table.  Also count up number of
 *	free slots at end of table so we can do compaction every once in a while.
 */

static void SignHandleOut(Handle theHandle, char *msg) {

	long iHandle,numEmpty;
	Handle *h;
	
	if (skipRegister) return;
	
	/* Check for existence in Handle table, starting at end of table */
	
	numEmpty = 0;
	iHandle = numHandles;
	h = (*handleTable) + iHandle;
	while (iHandle-- > 0)
		if (*--h == theHandle) {
			/* Good match: it was registered in table so check it out and be done */
			*h = NULL;
			if (++numEmpty > BUMPERSIZE)
				GarbageCollect();
			return;
			}
		 else
			if (*h == NULL)
				numEmpty++;				/* Count up free slots we pass during search */
	
	/* Couldn't find it in table: complain bitterly */
	
	MayErrMsg(msg);
	if (OptionKeyDown() || CmdKeyDown()) DebugStr("\pBREAK IN SignHandleOut;sc");
	}

/*
 *	Compact the table to reclaim all unfilled slots.  The next registering will cause
 *	the table to expand again.
 */

static void GarbageCollect() {

	Handle *src,*dst,*top;
	
	/* Compact by copying all allocated handles down into free slots */
	
	dst = *handleTable;
	top = dst + numHandles;
	
	src = dst - 1;				/* Wind up for the loop */
	while (++src < top)			/* *handleTable <= src < top ... */
		if (*src)
			*dst++ = *src;		/* Copy down defined entries only */
	
	/* Adjust table size and our local size so as to reclaim memory */
	
	numHandles -= (top - dst);
	SetHandleSize((Handle)handleTable,numHandles*sizeof(Handle));
	}

/*
 *	Deliver the handle to the handle table, and return in the argument the size of the
 *	handle table. If the size of the handle table is zero, the handle is invalid, so we
 *	just deliver NULL in that case. You might want to call this function before quitting,
 *	for example, so you can see if any handles weren't disposed.
 */

Handle **GetHandleTable(long *pNumHandles)
	{
		*pNumHandles = numHandles;
		return(numHandles? handleTable : NULL);
	}
