/* Nodes.c for Nightingale */

/*										NOTICE
 *
 *	THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROPERTY OF
 *	ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE SECRET AND IS
 *	NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED WRITTEN
 *	AUTHORIZATION FROM THE OWNER.
 *
 *	Copyright © 1989-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 *	Routines in this file implement various mid-level Nightingale generic object methods.
 *	"Nodes" are objects, often with sub-object lists attached, that are doubly-linked
 *	together into the main data structure (the "object list").  These routines know
 *	nothing of any object-specific higher-level crosslinks: those are taken care of at 
 *	the object level.
 *
 *  NOTE: Appearances to the contrary notwithstanding (i.e., the presence of <doc>
 *	parameters), most if not all of these functions assume the correct document's
 *	heaps are installed. Caveat!
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/*
 *	Create a head and tail for an empty node list, allocating the objects out of
 *	the given document's objHeap and linking them to one another.  Delivers TRUE
 *	if success, FALSE if no memory.
 */

Boolean BuildEmptyList(Document *doc, LINK *headL, LINK *tailL)
	{
		Boolean ans;
		
		*headL = NewNode(doc, HEADERtype, 2);			/* Need nparts+1 subobjs */
		*tailL = NewNode(doc, TAILtype, 0);
		
		if (ans = (*headL!=NILINK && *tailL!=NILINK)) {
			SetObject(*headL, 0, 0, FALSE, FALSE, TRUE);
			SetObject(*tailL, 0, 0, FALSE, FALSE, TRUE);
			
			/* Turn two objects into linked sentinel objects for an empty node list */
			
			LeftLINK(*headL) = NILINK;
			RightLINK(*headL) = *tailL;
			LeftLINK(*tailL) = *headL;
			RightLINK(*tailL) = NILINK;
			LinkTWEAKED(*headL) = LinkTWEAKED(*tailL) = FALSE;
			}
		
		return(ans);
	}

/*
 *	Return an entire node list, including head and tail, to memory.  Addresses of
 *	the head and tail LINKs are provided so we can set them to NILINK here.  Objects
 *	in list [headL,tailL] are assumed to belong to the given document's heaps.
 *	DeleteNode/DeleteRange delete only non-header/non-tail type objects.
 */

void DisposeNodeList(Document *doc, LINK *headL, LINK *tailL)
	{
		if (*headL==NILINK || *tailL==NILINK)
			MayErrMsg("DisposeNodeList: Tried to delete NILINK sentinel");
		 else {
			/* Ditch any objects in list between head and tail */
			
			DeleteRange(doc,RightLINK(*headL),*tailL);
			
			/*
			 *	Ditch subobject lists.  Nightingale currently does not
			 *	keep any subobjects in TAIL objects, so the second call
			 *	here does nothing, but we do it anyway because it is
			 *	not this routine's business to decide whether any particular
			 *	object does or does not have a subobject list.
			 */
			
			HeapFree(doc->Heap+HEADERtype,FirstSubLINK(*headL));
			HeapFree(doc->Heap+TAILtype,  FirstSubLINK(*tailL));
			
			/* Ditch head and tail objects (HeapFree takes the whole list) */
			
			HeapFree(doc->Heap+OBJtype,*headL);
			
			/* And reset references in calling routine to NILINK */
			
			*headL = *tailL = NILINK;
			}
	}

/*
 *	Given a <link> to a node, expand the node's subobject list by numEntries subobjects,
 *	and deliver TRUE if success, FALSE is no memory.  The newly allocated subobjects are
 *	appended to the <link>'s subobject list and are left minimally initialized.  The link
 *	to the first appended subobject is delivered in subList.  If numEntries<0, then
 *	ShrinkNode() is called to remove the final -numEntries subobjects from the given
 *	node's list, and TRUE is delivered.  If numEntries==0, nothing happens and TRUE is
 *	delivered.
 */

Boolean ExpandNode(LINK link, LINK *subList, INT16 numEntries)
	{
		HEAP	*subHeap;
		LINK	subHead;
		INT16	type;
	
		if (numEntries > 0) {							/* Append sub-objects */
		
			subHeap = Heap + (type = ObjLType(link));
			subHead = FirstSubLINK(link);
		
			/* Allocate sub-object list to append */
			
			*subList = HeapAlloc(subHeap, numEntries);
			if (*subList == NILINK) return(FALSE);		/* No memory */
			
			InitNode(type,*subList);					/* Minimal setup */
			LinkNENTRIES(link) += numEntries;
			/*
			 *	If sub-object list already exists, append;
			 *	otherwise, just install as new list
			 */
			if (subHead) InsAfterLink(subHeap, subHead, NILINK, *subList);
			 else		 FirstSubLINK(link) = subHead = *subList;
			}
		 else {
			ShrinkNode(link,-numEntries);				/* Does nothing if numEntries==0 */
			*subList = NILINK;
			}
		
		return(TRUE);
	}

/*
 *	Given a <link> to a node, shrink the node's subobject list by numToSnip subobjects.
 *	The final numToSnip subobjects are removed from the subobject list and returned to
 *	the associated heap's freelist.  If numToSnip <= 0, nothing happens.  Does no error
 *	checking!
 */

void ShrinkNode(LINK link, INT16 numToSnip)
	{
		INT16	numLeft;
		LINK	next,tail,subHead;
		HEAP	*subHeap;
		
	 	if (numToSnip > 0) {
	 		
	 		subHeap = Heap + ObjLType(link);
		 	subHead = FirstSubLINK(link);
	 		
	 		/* Get the number of entries left after snipping, and update node's field */
	 		
		 	numLeft = LinkNENTRIES(link) - numToSnip;
		 	if (numLeft < 0) numLeft = 0;
		 	LinkNENTRIES(link) = numLeft;
		 	
		 	if (numLeft == 0) {
		 		/* Delete entire sublist */
		 		FirstSubLINK(link) = HeapFree(subHeap,subHead);
		 		}
		 	 else if (subHead) {				/* Should always be defined */
		 		/*
		 		 *	Find subobject in sublist that points to head of subobjects to be snipped.
		 		 *	Note that numLeft>0 means we'll iterate this loop at least once.
		 		 */
		 		for (next=subHead; next && numLeft-->0; next=NextLink(subHeap,next)) tail = next;
		 		/* De-allocate rest of list and terminate the tail with a NILINK */
		 		NextLink(subHeap,tail) = HeapFree(subHeap,next);
		 		}
			}
	}

/*
 *	Minimally initialize the generic subobjects in given list of given type.
 */

void InitNode(INT16 type, LINK subList)
	{
		GenSubObj *subObj;
		LINK subL;
		HEAP *subHeap;
		
		subHeap = Heap + type;
		
		switch (type) {
			case HEADERtype:
			case TAILtype:
			case STAFFtype:
			case CONNECTtype:
				break;
			case MEASUREtype:
			case PSMEAStype:
			case SYNCtype:
			case GRSYNCtype:
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case DYNAMtype:
			case RPTENDtype:
				/*
				 *	For those subobjects that have a a SUBOBJHEADER which
				 *	contains the selected bit, etc. we initialize some of
				 *	its fields.
				 */
				for (subL=subList; subL; subL=NextLink(subHeap,subL)) {
					subObj = (GenSubObj *)LinkToPtr(subHeap,subL);
					subObj->selected = subObj->soft = FALSE;
					subObj->visible = TRUE;
					}
				break;
			case GRAPHICtype:
			case BEAMSETtype:
			case TUPLETtype:
			case OCTAVAtype:
			case TEMPOtype:
			case SPACEtype:
			case ENDINGtype:
			default:
				break;
			}
	}

/*
 *	Allocate an object with the specified type and number of subobjects, insert it
 *	into the object Heap before <link>, and return its LINK.  Returns NILINK if
 *	memory problem. Note: before calling this function, you should normally do
 *	something like "link = LocateInsertPt(link)": cf. InsNoteInto and InsNodeIntoSlot.
 */

LINK InsertNode(Document *doc, LINK link, INT16 type, INT16 subCount)
	{
		LINK obj, prevL;
	
		if (type==HEADERtype || type==TAILtype) {
			MayErrMsg("InsertNode: can't insert Head or Tail");
			return(NILINK);
			}
		
		if (obj = NewNode(doc, type, subCount)) {

			if (link == NILINK) {
				/* Append obj to end of Object list, or create new object list */
				prevL = NILINK;
				for (link=doc->headL; link; prevL=link,link=RightLINK(link)) ;
				if (prevL) {
					/* Append after prevL */
					RightLINK(prevL) = obj;
					LeftLINK(obj) = prevL;
					}
				 else {
					/* headL was empty */
					doc->headL = obj;
					}
				}
			 else {
				if (prevL = LeftLINK(link)) {
					RightLINK(obj) = link;
					RightLINK(prevL) = obj;
					LeftLINK(obj) = prevL;
					LeftLINK(link) = obj;
					}
				 else {
					/* Insert obj in front of headL (should never happen) */
					RightLINK(obj) = doc->headL;
					doc->headL = obj;
					}
				}
			}

		return(obj);
	}

/*
 *	Cut all objects in a node list from startL up to but not including endL.
 *	The resulting sublist remains well formed and the caller can continue to refer to
 *	it at startL for whatever purpose.  startL must be prior to endL in the node list.
 */

void CutRange(LINK startL, LINK endL)
	{
		LINK endMoveL;
		
		endMoveL = LeftLINK(endL);
	
		RightLINK(LeftLINK(startL)) = endL;			/* Remove range from old location. */
		LeftLINK(endL) = LeftLINK(startL);
		
		LeftLINK(startL) = NILINK;					/* Get rid of boundary references so */
		RightLINK(endMoveL) = NILINK;				/* that cut nodes are well-formed list */
	}

/*
 *	Cut a single node out of the object list. node cannot be HEADER or TAIL.
 *	Does no error checking!
 */

void CutNode(LINK node)
	{
		CutRange(node,RightLINK(node));
	}

/*
 *	Insert a single node before beforeL.  Use in place of MoveNode when the node
 *	is not in the object list. Does no error checking! Note: this function should
 *	almost never be called directly: use InsNodeIntoSlot instead.
 */

void InsNodeInto(LINK node, LINK beforeL)
	{
		LeftLINK(node) = LeftLINK(beforeL);
		RightLINK(node) = beforeL;
		RightLINK(LeftLINK(beforeL)) = node;
		LeftLINK(beforeL) = node;
	}

/*
 *	Insert a single node at the start of beforeL's slot in the object list. NB:
 *	This function is higher-level than the others in this file.
 */

void InsNodeIntoSlot(LINK node, LINK beforeL)
	{
		LINK insL;

		insL = LocateInsertPt(beforeL);
		InsNodeInto(node,insL);
	}

/*
 *	Remove node pL from its location in the object list and reinsert it
 *	directly in front of (to the left of) beforeL.  This will only work for
 *	internal objects, not for a head or tail object, which are being used
 *	as sentinels for the main list. Does no error checking!
 */

void MoveNode(LINK pL, LINK beforeL)
	{
		if (pL!=beforeL) MoveRange(pL,RightLINK(pL),beforeL);
	}

/*
 *	Remove the range of nodes from startL up to but not including endL from
 *	its current location in the object list, and reinsert the nodes directly
 *	in front of (to the left of) beforeL. If the range is empty, does nothing;
 *	otherwise, endL must come after startL in the object list and neither
 *	can be the head or tail object in the list.  Also, if beforeL is in the
 *	range, will probably do something unpleasant. Does no error checking!
 */

void MoveRange(LINK startL, LINK endL, LINK beforeL)
	{
		LINK endMoveL;
		
		if (startL==endL) return; 				/* Range is empty */
		if (endL==beforeL) return; 				/* Range is already before <beforeL> */		
		 
		endMoveL = LeftLINK(endL);				/* Get last node in range */
		CutRange(startL,endL);					/* Snip range from node list */

		LeftLINK(startL) = LeftLINK(beforeL);	/* Insert at new location */
		RightLINK(endMoveL) = beforeL;
	
		RightLINK(LeftLINK(beforeL)) = startL;
		LeftLINK(beforeL) = endMoveL;
	}
	
/*
 *	Delete the range of nodes from startL up to but not including endL in the main
 *	object list.  All nodes (both objects and each object's subobject list) are
 *	returned to the freelists of the heaps of their origin.  endL must be later in
 *	the object list than startL.  The routine delivers NILINK. doc's heaps must
 *	be installed.
 */

LINK DeleteRange(Document *doc, LINK startL, LINK endL)
	{
		LINK pL; INT16 type;
		HEAP *objHeap,*subHeap;
		
		if (startL == NILINK)
			MayErrMsg("DeleteRange: Tried to delete NILINK node");
		 else
			if (startL != endL) {				/* Non-empty ranges only, please */
			
				type = ObjLType(startL);
				if (type==HEADERtype || type==TAILtype)
					MayErrMsg("DeleteRange: Tried to delete Head or Tail");
				 else {
					/* Return all note modifiers to the free list. */
					DisposeMODNRs(startL,endL);

				 	/* Pull range out of node list */
					CutRange(startL,endL);
					
					/* Delete each node's subobject list, leaving nodes still in a list */
					objHeap = doc->Heap + OBJtype;
					for (pL=startL; pL; pL=NextLink(objHeap,pL)) {
						subHeap = doc->Heap + ObjLType(pL);
						HeapFree(subHeap,FirstSubLINK(pL));			/* Empty list is legal */
						}
					
					/* Finally, deliver the backbone back to its maker */
					return(HeapFree(objHeap,startL));
					}
				}
		return(NILINK);
	}

/*
 *	Delete a single node in object list. Just turn it into a range and do it that way.
 *	Delivers NILINK.
 */

LINK DeleteNode(Document *doc, LINK node)
	{
		INT16 type; LINK l,r;
		
		if (node) {
			type = ObjLType(node);
			if (type==HEADERtype || type==TAILtype) {
				HeapFree(doc->Heap+type,FirstSubLINK(node));
				if (r = RightLINK(node)) LeftLINK(r) = LeftLINK(node);
				if (l = LeftLINK(node))  RightLINK(l) = RightLINK(node);
				RightLINK(node) = NILINK;
				HeapFree(doc->Heap+OBJtype,node);
				}
			 else
				DeleteRange(doc,node,RightLINK(node));
			}
		 else
			MayErrMsg("DeleteNode: Tried to delete NILINK node");
		return(NILINK);
	}

/*
 *	Allocate an object from the object heap, and subCount subobjects of the given
 *	type, linked in a list.  Link the subobject list to the object, set the object
 *	to be the same type and deliver the link of the compound object allocated.
 *	Deliver NILINK if error (no memory).  Various generic fields of the subobject
 *	list and object are set to default values.
 *
 *	This code uses memory macros that assume particular positions in the object
 *	header area, and initializes them.  Other less generic fields in the objects
 *	have to be filled in by the caller, and are allocated with garbage in them.
 */

LINK NewNode(Document *doc, INT16 type, INT16 subCount)
	{
		LINK	objHead,subHead,subL;
		HEAP	*objHeap, *subHeap;
		
		objHeap = doc->Heap + OBJtype;
		subHeap = doc->Heap + type;
		
		if (objHead = HeapAlloc(objHeap,1)) {
		
			/* Got the object: now get the list of subCount subobjects */
			
			subHead = NILINK;
			if (subCount)
				if (!(subHead = HeapAlloc(subHeap,subCount)))
					objHead = HeapFree(objHeap,objHead);			/* objHead = NILINK */
			
			if (objHead) {
			
				/* Initialise various OBJECTHEADER fields of newly allocated object. */
				
				DLeftLINK(doc,objHead) = NILINK;			/* Not attached to any list yet */
				DRightLINK(doc,objHead) = NILINK;
				
				DFirstSubLINK(doc,objHead) = subHead;		/* Attach subobjects to object */
				DLinkNENTRIES(doc,objHead) = subCount;
				
				DObjLType(doc,objHead) = type;				/* Compound object is of given type */
				DLinkSEL(doc,objHead) = FALSE;
				DLinkSOFT(doc,objHead) = FALSE;				/* Not user created */
				DLinkVIS(doc,objHead) = TRUE;

				DLinkTWEAKED(doc,objHead) = FALSE;
				DLinkYD(doc,objHead) = 0;
				
				/* Initial objRect is empty and needs to be recomputed */
				
				SetRect(&DLinkOBJRECT(doc,objHead), 0, 0, 0, 0);
				DLinkVALID(doc,objHead) = FALSE;
	
				/* And initialize various other fields according to type */
				
				switch (type) {
					case HEADERtype:
					case TAILtype:
						DLinkSOFT(doc,objHead) = TRUE;
						break;
					case STAFFtype:
					case CONNECTtype:
						break;
					case MEASUREtype:
					case SYNCtype:
					case GRSYNCtype:
					case CLEFtype:
					case KEYSIGtype:
					case TIMESIGtype:
					case DYNAMtype:
					case RPTENDtype:
						for (subL=subHead; subL; subL=NextLink(subHeap,subL)) {
							GenSubObj *subObj = (GenSubObj *)LinkToPtr(subHeap,subL);
							subObj->selected = subObj->soft = FALSE;
							subObj->visible = TRUE;
							}
						break;
					case GRAPHICtype:
					case TEMPOtype:
					case SPACEtype:
					case BEAMSETtype:
					case TUPLETtype:
					case OCTAVAtype:
					case ENDINGtype:
						break;
					default:
						break;
					}
				}
			}
		return(objHead);
	}


/* ----------------------------------------------------------------- MergeObject -- */
/* Merge pL into qL by appending pL's subobject list to qL's. Preserves qL; this
has significance for later operations which expect LINKs in the merged range to
retain their identity. Discards pL. */

LINK MergeObject(Document *doc, LINK pL, LINK qL)
{
	LINK subL; HEAP *tmpHeap;

	tmpHeap = doc->Heap + ObjLType(pL);
	
	/* InsAfterLink(heap,head,after,objlist):
		Insert <objlist> after link <after> in list <head>.  after is NILINK,
		so append objList to head's list. */

	subL = FirstSubLINK(pL);
	InsAfterLink(doc->Heap+ObjLType(pL),FirstSubLINK(qL),NILINK,subL);
	LinkNENTRIES(qL) += LinkNENTRIES(pL);
	HeapFree(OBJheap, pL);

	return qL;
}
