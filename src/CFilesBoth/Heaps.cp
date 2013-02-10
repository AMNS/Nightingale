/* Heaps.c - low-level heap functions for Nightingale */

/*										NOTICE
 *
 *	THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROPERTY OF
 *	ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE SECRET AND IS
 *	NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED WRITTEN
 *	AUTHORIZATION FROM THE OWNER.
 *
 *	Copyright © 1989-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 *	Routines in this file implement various low-level Nightingale generic heap methods.
 *	A heap is where we store and allocate "objects": fixed-length records for all
 *	the various objects kept in a score.  Each heap is associated with one type of
 *	object (since different types have different lengths).
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* This array gives the initial guess at the size, in objects, of each object/subobject
heap.  These can be tweaked however is appropriate. (Note: As of v.82, 2048 initial
SYNC objects was too large for 1MB machines. I've never changed the smaller values
used here because they seem to work fine. --DAB) */

#ifdef LARGE_INITIAL_HEAPS
static short initialNumbers[LASTtype] = {
		32,		/* HEADERtype */
		0,			/* TAILtype */
		1024,		/* SYNCtype */
		8,			/* RPTENDtype */
		0,			/* PAGEtype */
		0,			/* SYSTEMtype */
		128,		/* STAFFtype */
		512,		/* MEASUREtype */
		64,		/* CLEFtype */
		64,		/* KEYSIGtype */
		64,		/* TIMESIGtype */
		512,		/* BEAMSETtype */
		64,		/* CONNECTtype */
		256,		/* DYNAMtype */
		64,		/* MODNRtype */
		128,		/* GRAPHICtype */
		64,		/* OCTAVAtype */
		256,		/* SLURtype */
		256,		/* TUPLETtype */
		128,		/* GRSYNCtype */
		8,			/* TEMPOtype */
		8,			/* SPACEtype */
		8,			/* ENDINGtype */
		16,		/* PSMEAStype */
		256		/* OBJtype */
		};
#else
static short initialNumbers[LASTtype] = {
		8,			/* HEADERtype */
		0,			/* TAILtype */
		64,		/* SYNCtype */
		8,			/* RPTENDtype */
		0,			/* PAGEtype */
		0,			/* SYSTEMtype */
		8,			/* STAFFtype */
		12,		/* MEASUREtype */
		8,			/* CLEFtype */
		8,			/* KEYSIGtype */
		8,			/* TIMESIGtype */
		12,		/* BEAMSETtype */
		8,			/* CONNECTtype */
		8,			/* DYNAMtype */
		8,			/* MODNRtype */
		8,			/* GRAPHICtype */
		8,			/* OCTAVAtype */
		8,			/* SLURtype */
		8,			/* TUPLETtype */
		12,		/* GRSYNCtype */
		8,			/* TEMPOtype */
		8,			/* SPACEtype */
		8,			/* ENDINGtype */
		8,			/* PSMEAStype */
		64			/* OBJtype */
		};
#endif


#ifdef LinkToPtrFUNCTION

/* LinkToPtr(heap,link) delivers the address of the 0'th byte of the link'th
object kept in a given heap.  This address is determined without typing
information by using the heap's own idea of how large the object is in bytes.
The pointer so delivered is only valid as long as the heap block doesn't get
relocated! This is a generic routine that should be avoided whenever it's
possible to use one of the specific ones in MemMacros.h (??IS THIS STILL TRUE?).

N.B. This assembly language function replaces the C macro of the same name.
LinkToPtr is/was called in over 3,000 places (mostly by other macros). Using a
function saves a lot of memory, since it only takes 14 bytes to call (2 MOVEs
to push arguments on stack, JSR, MOVEQ to pop stack), while the macro takes 30
bytes. This version is also much faster than the macro, while a function version
written in C turned out to be considerably slower than the C macro. */
 
char *LinkToPtr(HEAP *heap, LINK link)
//HEAP *heap;
//LINK link;
{
	/*
	 *	Rough C equivalent (this throws away high-order 16 bits, which we need to keep):
	 *		return ( ((char *)(*(heap)->block)) + ((heap)->objSize*(link)) );
	 */
	asm {
		MOVEA.L    heap,A0               
		MOVE.W     OFFSET(HEAP,objSize)(A0),D0               
		MULU.W     link,D0               
		MOVEA.L    OFFSET(HEAP,block)(A0),A0                    
		ADD.L      (A0),D0                    
		}
}
#endif /* LinkToPtrFUNCTION */


/* Allocate all the object heaps for a given document, with initial free list sizes
taken from the initialNumbers array, and deliver TRUE or FALSE according to
success or not. */

Boolean InitAllHeaps(Document *doc)
	{
		short i; HEAP *hp;
		
		/* For each heap in the document's heap array... */
		
		for (hp=doc->Heap,i=FIRSTtype; i<LASTtype; i++,hp++) {
			hp->type = i;
			hp->objSize = subObjLength[i];
			hp->lockLevel = 0;
			hp->block = NULL;
			hp->nObjs = 0;
			hp->nFree = 0;
			hp->firstFree = NILINK;
			hp->block = NewHandle(0L);			/* Initially empty data block */
			if (MemError()) return(FALSE);
			if (!ExpandFreeList(hp,initialNumbers[i])) return(FALSE);
			}
			
		return(TRUE);
	}

/* Dispose of all heap blocks in a given document record.  This routine can be (but
really shouldn't be) called more than once. */

void DestroyAllHeaps(Document *doc)
	{
		short i; HEAP *hp;
		
		/* For each heap in the document's heap array... */
		
		for (hp=doc->Heap,i=FIRSTtype; i<LASTtype; i++,hp++) {
			if (hp->block) DisposeHandle(hp->block);
			hp->block = NULL;
			}
	}

/* This is called to expand the size of a given heap by deltaObjs objects of the
size associated with the given heap.  It links these into the freelist, and
delivers TRUE if okay, FALSE on an error (LINK count overflow or memory wasn't
available).  This routine works whether or not there are any free objects left
in the heap's freelist.

The objects in each heap are all the same objSize, and can initially be treated
as an array of free objects; however, we use the link field (at the same spot
in all object records) to turn the array into a singly-linked list of free objects.
Note that the 0'th object is always unused, so that we can rely on a LINK value of
NILINK to mean boundary conditions of various kinds.

This code depends on the fact that object's right LINK field is the first field
of the object header (so that its address is the same as the whole object's).

Never allow more objects than we can access with a LINK used as an index (USHRT_MAX
= 65535L); in fact, the number of objects should always be at least a few less so
very large values can be used as codes for whatever reason. It's also possible to
limit the number to a much smaller number, e.g., for a "Lite" version. */

/* ??To allow more helpful error msgs, especially for a "Lite" version, if this
fails, it should return info as to the reason! */

#define MAX_HEAPSIZE 65500L								/* Maximum objects of any type */

Boolean ExpandFreeList(HEAP *heap,
								long deltaObjs)				/* <=0 = do nothing, else <MAX_HEAPSIZE */ 
	{
		long newSize; unsigned short i;
		char *p; short err;
		
		if (deltaObjs<=0 || heap->objSize<=0) return(TRUE);		/* Do nothing */
		
		/*
		 *	Add extra space for deltaObjs>0 objects at the end of this Heap's block, but
		 *	don't allow too many objects: see comment above. 
		 */
		newSize = ((long)heap->nObjs) + deltaObjs;
		if (newSize>=MAX_HEAPSIZE) {
			char str[256];
			
			GetIndCString(str, ErrorStringsID, 16);	/* "The operation failed because the score requires more objects... */
			CParamText(str, "", "", "");
			StopInform(GENERIC_ALRT);
	 				return(FALSE);
 		}
		
		/* Temporarily unlock the data block, if necessary, and expand it. */
		
		if (heap->lockLevel != 0) {
#ifdef DEBUG
			SysBeep(1);
			AlwaysErrMsg("ExpandFreeList: Heap was locked");
#endif
			HUnlock(heap->block);
			}
		SetHandleSize(heap->block,newSize * heap->objSize);
		err = MemError();
		if (heap->lockLevel != 0) HLock(heap->block);
		if (err) return(FALSE);
		
		/*
		 *	For all but last added object, link object to following object.
		 *	p is left pointing to the last added object.
		 */
		p = (char *)(*heap->block);
		p += ((long)heap->nObjs) * (long)heap->objSize;	/* Addr of first added object */
		
		for (i=heap->nObjs; i<newSize-1; i++) {
			*(LINK *)p = i+1; p += heap->objSize;
			}
			
		/* Now paste these new objects in at the head of the free list */
		
		*(LINK *)p = heap->firstFree;
		heap->firstFree = heap->nObjs;
		heap->nFree += deltaObjs;
		
		/*
		 *	However, if this heap was empty, then we should have one fewer object,
		 *	since 0'th object is never available to be allocated.  So we allocate it
		 *	here and forget it, using the fact that we know list elements are still
		 *	in array order.
		 */
		if (heap->nObjs == 0)
			if (--heap->nFree > 0) heap->firstFree = 1;

		heap->nObjs = newSize;
		return(TRUE);
	}


/* Deliver the index (LINK) of the first object of a linked list of nObjs objects
from a given heap, or NILINK if no more memory.  nObjs must be positive! */

#define GROWFACTOR 4		/* When more memory needed, get a chunk this many times as big */

LINK HeapAlloc(HEAP *heap, unsigned short nObjs)
	{
		LINK link,head;
		char *p,*start;
		
		if (nObjs <= 0) {
		 	MayErrMsg("HeapAlloc: nObjs=%ld is illegal. heap=%ld", (long)nObjs, heap-Heap);
		 	return(NILINK);
		 	}
		if (heap->objSize == 0) {
			MayErrMsg("HeapAlloc: object size is 0.  heap=%ld", heap-Heap);
			return(NILINK);
			}
		
		/* Expand the free list, if necessary */
		
		if (nObjs > heap->nFree)
			if (!ExpandFreeList(heap,GROWFACTOR*nObjs))
				return(NILINK);
			
		/* Find the last of the first nObjs in the free list */
		
		start = (char *)(*heap->block);
		head = link = heap->firstFree;
		/* Snip first nObjs free objects from free list, and deliver head */
		heap->nFree -= nObjs;
		while (nObjs-- > 0) {						/* Find the last object in snipped list */
			p = start + ((unsigned long)heap->objSize * (unsigned long)link);
			link = *(LINK *)p;
			}
		*(LINK *)p = NILINK;							/* Terminate list */

		heap->firstFree = link;						/* Reset the head of the free list */
		return(head);
	}

/* Add a given list, whose first LINK is head, to the freelist of the given heap.
If we're keeping track of the last object in the heap's free list, we can just
tack this list onto the end; otherwise, we have to traverse this list to find
its last object, and set its link field to the current head of the freelist,
and then reset the firstFree field to list.  The empty list is just ignored.
HeapFree always delivers NILINK, so that the calling routine can just assign
HeapFree() to head. */

LINK HeapFree(HEAP *heap, LINK head)
	{
		short count; LINK link;
		char *start,*p;
		
		if (head) {
			
			/* Find the last object in the given list, and get its length */
			
			start = (char *)(*heap->block);
			for (count=0,link=head; link; link = *(LINK *)p,count++)
				p = start + ((unsigned long)heap->objSize * (unsigned long)link);
			
			/*
			 *	Append freelist to list, and repoint the head of the free list for
			 *	this heap to the list.
			 */
			
			*(LINK *)p = heap->firstFree;
			heap->firstFree = head;
			heap->nFree += count;
			}
		return(NILINK);
	}

/*
 *	Given a list of subobjects of object objL, remove the given object, obj,
 *	from the list (if it's there, otherwise error).  The object removed can
 *	still be refered to by the caller, since it has not yet been freed back
 *	to its heap's free list.  Delivers new head of list, or NILINK, if obj
 *	was not found.
 *
 *	This function should only be called for subObjects. If a similar 
 *	function is required for object links, the assignment below of 
 *	FirstSubLINK(objL) must be modified.
 */

LINK RemoveLink(LINK objL, HEAP *heap, LINK head, LINK obj)
	{
		LINK prev,next,link;
		
		for (prev=NILINK,link=head; link; prev=link,link=next) {
			next = NextLink(heap,link);
			if (link == obj) {
				*(LINK *)LinkToPtr(heap,link) = NILINK; 	/* Snip obj away from rest of list */
				if (prev) {											/* obj was not first */
					*(LINK *)LinkToPtr(heap,prev) = next;	/* prev->next = next */
					return(head);
					}
				 else {												/* Removing head of list */
				 	FirstSubLINK(objL) = next;
					return(next);
					}
				}
			}
		return(NILINK);
	}

/* Insert a given list before another given object in a given list.
If before is NILINK, then append obj to list.  Delivers new head of list,
or NILINK if error.
This code assumes that objlist is a well-formed (NILINK-terminated) list. */

LINK InsertLink(HEAP *heap, LINK head, LINK before, LINK objlist)
	{
		LINK prev,next,link,tail;
		
		if (objlist == NILINK) return(head);
		
		/* Search for before in list at head */
		
		for (prev=NILINK,link=head; link; prev=link,link=next) {
			next = NextLink(heap,link);
			
			if (link == before) {
			
				if (prev)											/* before was not first */
					*(LINK *)LinkToPtr(heap,prev) = objlist;
				 else												/* before is head of list */
				 	head = objlist;
				
				/* Find tail of objlist */
				for (tail=link=objlist; link; tail=link,link=NextLink(heap,link)) ;
				*(LINK *)LinkToPtr(heap,tail) = before;
				
				return(head);
				}
			}
		
		/* If before was NILINK, append objlist to list at head */
		
		if (head!=NILINK && before==NILINK) {
			*(LINK *)LinkToPtr(heap,prev) = objlist;
			return(head);
			}
		
		/* Otherwise, error condition */
		
		return(NILINK);
	}

/* Insert <objlist> after link <after> in list <head>.  If after is NILINK,
then append objList to head's list.  Delivers new head of list, or NILINK if error.
This code assumes that objlist is a well-formed (NILINK-terminated) list. */

LINK InsAfterLink(HEAP *heap, LINK head, LINK after, LINK objlist)
	{
		LINK prev,next,link,tail,temp;
		
		if (objlist == NILINK) return(head);
		
		/* Search for after in list at head */
		
		for (prev=NILINK,link=head; link; prev=link,link=next) {
			next = NextLink(heap,link);
			
			if (link == after) {
			
				if (prev) {								/* <after> was not first */
					temp = *(LINK *)LinkToPtr(heap,after);
					*(LINK *)LinkToPtr(heap,after) = objlist;
					}
				 else	{								/* <after> is head of list */
				 	temp = *(LINK *)LinkToPtr(heap,head);
				 	*(LINK *)LinkToPtr(heap,head) = objlist;
				 	}
				
				/* Find tail of objlist */
				for (tail=link=objlist; link; tail=link,link=NextLink(heap,link)) ;
				*(LINK *)LinkToPtr(heap,tail) = temp;	/* tail->next = after->next */
				
				return(head);
				}
			}
		
		/* If after was NILINK, append objlist to end of list at head */
		
		if (head!=NILINK && after==NILINK) {
			/* Find tail of list at head */
			for (tail=link=head; link; tail=link,link=NextLink(heap,link)) ;
			*(LINK *)LinkToPtr(heap,tail) = objlist;

			return(head);
			}
		
		/* Otherwise, error condition */
		
		return(NILINK);
	}
