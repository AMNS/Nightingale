/***************************************************************************
*	FILE:	HeapFileIO.c
*	PROJ:	Nightingale, rev. for v.2000
*	DESC:	Routines to read and write object and subobject heaps
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


//#define N104_1TimeConvert	


/* Error codes and error info codes */

#define HDR_TYPE_ERR 217			/* Heap header in file contains incorrect type */
#define HDR_SIZE_ERR 251			/* Heap header in file contains incorrect objSize */
#define MEM_ERRINFO 	99				/* ExpandFreeList failed */

#define EXTRAOBJS	10L

static unsigned short objCount[LASTtype];

/* Local prototypes */

static short WriteObjHeap(Document *, short refNum, LINK *firstSubLINKA, LINK *objA);
static void CreateModTable(Document *, LINK **modA);
static short WriteModHeap(Document *, short refNum, LINK **modA);
static short WriteModSubs(short refNum, LINK aNoteL, LINK link, LINK **modA, unsigned short *nMods);
static short WriteSubHeaps(Document *, short refNum, LINK *firstSubLINKA, LINK *objA, LINK *modA);
static short WriteHeapHdr(Document *, short refNum, short heapIndex);
static short WriteSubObjs(short refNum, short heapIndex, LINK pL, LINK link, LINK *firstSubLINKA,
						LINK *objA, LINK *modA, short *objCount);
static short WriteObject(short refNum, short heapIndex, LINK pL);
static Boolean ComputeObjCounts(Document *, LINK **firstSubLINKA, LINK **objA, LINK **modA);
static short ReadObjHeap(Document *doc, short refNum, long version, Boolean isViewerFile);
static short ReadSubHeaps(Document *doc, short refNum, long version, Boolean isViewerFile);
static short ReadHeapHdr(Document *doc, short refNum, long version, Boolean isViewerFile,
						short heapIndex, unsigned short *pnFObjs);
static void HeapFixLinks(Document *);
static void RebuildFreeList(Document *doc, short heapIndex, unsigned short nFObjs);
static void PrepareClips(void);

/* =========================== Functions for Writing Heaps ======================== */

/* Write all heaps with their headers to the given file. Returns 0 if no error,
else an error code (either a system result code or one of our own codes). */

short WriteHeaps(Document *doc, short refNum)
{
	LINK *firstSubLINKA, *objA, *modA; short errCode;
	
	if (ComputeObjCounts(doc, &firstSubLINKA, &objA, &modA)) {
		CreateModTable(doc, &modA);
		errCode = WriteSubHeaps(doc, refNum, firstSubLINKA, objA, modA);
		if (!errCode) errCode = WriteObjHeap(doc, refNum, firstSubLINKA, objA);
	}
	else
		errCode = 9999;
	
	if (firstSubLINKA) DisposePtr((Ptr)firstSubLINKA);
	if (objA) DisposePtr((Ptr)objA);
	if (modA) DisposePtr((Ptr)modA);
	
	return errCode;
}

/* Write the object heap to the given file. */

static short WriteObjHeap(Document *doc, short refNum, LINK *firstSubLINKA,
								LINK *objA)
{
	LINK			pL, oldFirstObj;
	LINK			leftL, rightL, subL, oldLastObj,
					oldFirstSync, oldLastSync, oldLRepeat, oldRRepeat;
	unsigned short j;
	short			ioErr=noErr, hdrErr;
	long			count, startPosition, endPosition, zero=0L, sizeAllObjs;

	PushLock(doc->Heap+OBJtype);

	/*
	 *	First write out the heap header, and 4 bytes that will contain the total number
	 * of bytes written out for all (variable-sized) objects.  We must save the position
	 *	in the file where this information is written so we can go back and fill it in
	 *	after writing the objects.
	 */

	hdrErr = WriteHeapHdr(doc, refNum, OBJtype);
	if (hdrErr) return hdrErr;
	
	GetFPos(refNum,&startPosition);
	count = sizeof(long);
	ioErr = FSWrite(refNum,&count,&zero);
	if (ioErr) { SaveError(TRUE, refNum, ioErr, OBJtype); return ioErr; }
	
	for (j=1, pL=doc->headL; ioErr==noErr && pL != NILINK; j++, pL=RightLINK(pL)) {
	
		/* Store old link values. */
		leftL = LeftLINK(pL); rightL = RightLINK(pL);
		subL = FirstSubLINK(pL);

		/* Write the link values for the in-file objects. */
		FirstSubLINK(pL) = firstSubLINKA[pL];
		LeftLINK(pL) = j-1;
		RightLINK(pL) = j+1;
		if (pL==doc->tailL) RightLINK(pL) = NILINK;
		
		/* Save fields in objects that refer to other objects and temporarily set
		 *	them to the values that are being written out for those objects.  */
		switch (ObjLType(pL)) {
			case SLURtype:
				oldFirstSync = SlurFIRSTSYNC(pL);
				SlurFIRSTSYNC(pL) = objA[SlurFIRSTSYNC(pL)];
				oldLastSync = SlurLASTSYNC(pL);
				SlurLASTSYNC(pL) = objA[SlurLASTSYNC(pL)];
				break;
			case GRAPHICtype:
				oldFirstObj = GraphicFIRSTOBJ(pL);
				GraphicFIRSTOBJ(pL) = objA[GraphicFIRSTOBJ(pL)];
				if (GraphicSubType(pL)==GRDraw) {
					oldLastObj = GraphicLASTOBJ(pL);
					GraphicLASTOBJ(pL) = objA[GraphicLASTOBJ(pL)];
				}
				break;
			case TEMPOtype:
				oldFirstObj = TempoFIRSTOBJ(pL);
				TempoFIRSTOBJ(pL) = objA[TempoFIRSTOBJ(pL)];
				break;
			case DYNAMtype:
				oldFirstSync = DynamFIRSTSYNC(pL);
				DynamFIRSTSYNC(pL) = objA[DynamFIRSTSYNC(pL)];
				if (IsHairpin(pL)) {
					oldLastSync = DynamLASTSYNC(pL);
					DynamLASTSYNC(pL) = objA[DynamLASTSYNC(pL)];
				}
				break;
			case ENDINGtype:
				oldFirstObj = EndingFIRSTOBJ(pL);
				EndingFIRSTOBJ(pL) = objA[EndingFIRSTOBJ(pL)];
				oldLastObj = EndingLASTOBJ(pL);
				EndingLASTOBJ(pL) = objA[EndingLASTOBJ(pL)];
				break;
			case RPTENDtype:
				oldFirstObj = RptEndFIRSTOBJ(pL);
				RptEndFIRSTOBJ(pL) = objA[RptEndFIRSTOBJ(pL)];
				oldLRepeat = RptEndSTARTRPT(pL);
				RptEndSTARTRPT(pL) = objA[RptEndSTARTRPT(pL)];
				oldRRepeat = RptEndENDRPT(pL);
				RptEndENDRPT(pL) = objA[RptEndENDRPT(pL)];
				break;
			default:
				break;
		}

		ioErr = WriteObject(refNum, OBJtype, pL);
		
		LeftLINK(pL) = leftL; RightLINK(pL) = rightL;
		FirstSubLINK(pL) = subL;
		
		/* Set fields that refer to other objects back to their correct values. */
		switch (ObjLType(pL)) {
			case SLURtype:
				SlurFIRSTSYNC(pL) = oldFirstSync;
				SlurLASTSYNC(pL) = oldLastSync;
				break;
			case GRAPHICtype:
				GraphicFIRSTOBJ(pL) = oldFirstObj;
				GraphicLASTOBJ(pL) = oldLastObj;
				break;
			case TEMPOtype:
				TempoFIRSTOBJ(pL) = oldFirstObj;
				break;
			case DYNAMtype:
				DynamFIRSTSYNC(pL) = oldFirstSync;
				if (IsHairpin(pL)) 
					DynamLASTSYNC(pL) = oldLastSync;
				break;
			case ENDINGtype:
				EndingFIRSTOBJ(pL) = oldFirstObj;
				EndingLASTOBJ(pL) = oldLastObj;
				break;
			case RPTENDtype:
				RptEndFIRSTOBJ(pL) = oldFirstObj;
				RptEndSTARTRPT(pL) = oldLRepeat;
				RptEndENDRPT(pL) = oldRRepeat;
				break;
			default:
				break;
		}
	}

	for (pL=doc->masterHeadL; ioErr==noErr && pL != NILINK; j++, pL=RightLINK(pL)) {
	
		/* Store old link values. */
		leftL = LeftLINK(pL); rightL = RightLINK(pL);
		subL = FirstSubLINK(pL);

		/* Write the link values for the in-file objects. */
		FirstSubLINK(pL) = firstSubLINKA[pL];
		LeftLINK(pL) = j-1;
		RightLINK(pL) = j+1;
		if (pL==doc->masterHeadL) LeftLINK(pL) = NILINK;
		if (pL==doc->masterTailL) RightLINK(pL) = NILINK;
		
		/* No special object types in Master Page. */

		ioErr = WriteObject(refNum, OBJtype, pL);
		
		LeftLINK(pL) = leftL; RightLINK(pL) = rightL;
		FirstSubLINK(pL) = subL;
	}

	PopLock(doc->Heap+OBJtype);
	
	/*
	 *	Now get the current file position to find out how much we've written.
	 *	Then back patch the size into the 4-byte slot we pre-allocated above.
	 *	NOTE: We have to take into account the 4 bytes for the count as well.
	 */
	
	if (ioErr==noErr) {
		GetFPos(refNum,&endPosition);
		sizeAllObjs = endPosition - startPosition - sizeof(long);
		ioErr = SetFPos(refNum,fsFromStart,startPosition);
		if (ioErr!=noError) return ioErr;
		count = sizeof(long);
		ioErr = FSWrite(refNum,&count,&sizeAllObjs);
		if (ioErr) { SaveError(TRUE, refNum, ioErr, OBJtype); return ioErr; }
		/* And move back to where we just were so as not to truncate anything */
		ioErr = SetFPos(refNum,fsFromStart,endPosition);
		if (ioErr!=noError) return ioErr;
	}
		
	return ioErr;
}

static void CreateModTable(Document *doc, LINK **modA)
{
	HEAP *myHeap;
	unsigned short j, nMods=0;
	LINK pL, aNoteL, subL;
	PANOTE aNote;

	myHeap = Heap+MODNRtype;

	if (myHeap->nObjs) {

	/* j is a parameter passed to WriteSubObjs to indicate the initial LINK address 
		of the subobj list written by WriteSubObjs. We start at 1 since the zeroth 
		heap object is never used. */

		for (j=1, pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL))
			if (ObjLType(pL)==SYNCtype) {
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->firstMod) {
						nMods = 0;
						for (subL = aNote->firstMod; subL; subL = NextMODNRL(subL))
							nMods++;

						/* Increment the index to be passed to the next call to
							WriteSubObjs() to reflect the number of subobjs just
							written to file, i.e., keep j in sync with the index in
							the file of the next subobject to be written. */

						(*modA)[aNote->firstMod] = j;
						j += nMods;
					}
				}
			}
	}
}


static short WriteModHeap(Document *doc, short refNum, LINK **modA)
{
	HEAP *myHeap;
	short ioErr=noErr;
	unsigned short  j, nMods=0;
	LINK pL, aNoteL;
	PANOTE aNote;

	myHeap = Heap+MODNRtype;

	if (myHeap->nObjs) {
		for (j=1, pL=doc->headL; ioErr==noErr && pL!=doc->tailL; pL=RightLINK(pL))
			if (ObjLType(pL)==SYNCtype) {
				aNoteL = FirstSubLINK(pL);
				for ( ; ioErr==noErr && aNoteL!=NILINK; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->firstMod) {
						ioErr = WriteModSubs(refNum, aNoteL, (LINK)j, modA, &nMods);
						j += nMods;
					}
				}
			}
	}
	return ioErr;
}


/* Write the note modifier subobjects to file, updating the subobject 
<next> links to reflect the new position of the subobjects in their 
in-file heap, and filling in the modA array so that notes can update
their firstMod field in the WriteSubObjs routine. */

static short WriteModSubs(short refNum, LINK aNoteL, LINK	link, LINK **/*modA*/,
									unsigned short *nMods)
{
	LINK 		nextL, subL, tempL;
	HEAP 		*myHeap;
	PANOTE 	aNote;
	short		ioErr = noErr;
	
	/* The links are now being allocated sequentially, so that the <next> link
		of the current link <link> is link+1. */	
	nextL = link+1;
	myHeap = Heap + MODNRtype;
	PushLock(myHeap);
	
	*nMods = 0;
	aNote = GetPANOTE(aNoteL);
	for (subL = aNote->firstMod; ioErr==noErr && subL!=NILINK; subL = tempL) {
		tempL = NextLink(myHeap, subL);
		if (tempL)
			*(LINK *)LinkToPtr(myHeap, subL) = nextL++;

		ioErr = WriteObject(refNum, MODNRtype, subL);

		if (tempL)
			*(LINK *)LinkToPtr(myHeap, subL) = tempL;

		(*nMods)++;
	}
	PopLock(myHeap);
	return ioErr;
}


static void MackeysDiseaseMsg(LINK pL, short subObjCount, char *objString);
static void MackeysDiseaseMsg(LINK pL, short subObjCount, char *objString)
{
	AlwaysErrMsg("FILE IS PROBABLY UNUSABLE. WriteSubHeaps: %s at %ld (type=%ld) has nEntries=%ld but subObjCount=%ld. Please contact AMNS tech support.",
				objString, (long)pL, (long)ObjLType(pL), (long)LinkNENTRIES(pL),
				(long)subObjCount);
}

/* Write all subobjects of all types to the file. For each heap, we write out the
number of subobjects in that heap (as determined by ComputeObjCounts) as part of the
heap header; then we traverse the object list and write out all the subobjects
themselves.

Note that if ComputeObjCounts finds a number for any heap that's different from what
the heap-writing code finds, the results are likely to be disasterous: the file
will be written apparently without problems, but it will probably not be possible to
open it again! Also, if WriteSubObjs finds a number for any object that's different
from what the object's LinkNENTRIES, something is seriously wrong, though I don't know
what the effects would be. To at least alert the user if this happens, we give a
strongly-worded message in the latter case, but not the (at least as serious!) former. */

static short WriteSubHeaps(Document *doc, short refNum, LINK *firstSubLINKA, LINK *objA,
									LINK *modA)
{
	HEAP *myHeap;
	short i, ioErr = noErr, hdrErr, modErr, subObjCount;
	unsigned short j;
	LINK pL;
	
	for (i=FIRSTtype; ioErr==noErr && i<LASTtype-1; i++) {

		hdrErr = WriteHeapHdr(doc, refNum, i);
		if (hdrErr) return hdrErr;

		/* Note modifiers (the only type with subobjects but no objects: note modifier
			subobjects are attached to Sync subobjects) are a special case, so we
			handle them separately. For all other types, traverse the main object list
			and the Master Page object list once for each type and write the subobjects
			of each object of that type we encounter. */
		
		if (i==MODNRtype) {
			modErr = WriteModHeap(doc, refNum, &modA);
			if (modErr) return modErr;
			continue;
		}

		myHeap = Heap + i;
		if (myHeap->nObjs) {

			/* Write the subobjects for all objects in the main object list except the
				tail, which has no subobjects. j is a parameter passed to WriteSubObjs to
				indicate the initial LINK address of the subobj list written by WriteSubObjs.
				We start at 1 since the zeroth heap object is never used. */

			for (j=1, pL=doc->headL; ioErr==noErr && pL!=doc->tailL; pL=RightLINK(pL))
				if (ObjLType(pL)==i) { 
					ioErr = WriteSubObjs(refNum, i, pL, (LINK)j, firstSubLINKA, objA, modA,
													&subObjCount);	
					if (subObjCount!=LinkNENTRIES(pL))
						MackeysDiseaseMsg(pL, subObjCount, "Obj");		/* We're in big trouble */
					
					/* Increment the index to be passed to the next call to WriteSubObjs()
						to reflect the number of subobjs just written to file, i.e., keep j in
						sync with the index in the file of the next subobject to be written. */

					j += subObjCount;
				}

			/* Continue writing the subobjects for all objects in the Master Page data
				structure except the tail. */

			for (pL=doc->masterHeadL; ioErr==noErr && pL!=doc->masterTailL; pL=RightLINK(pL))
				if (ObjLType(pL)==i) { 
					ioErr = WriteSubObjs(refNum, i, pL, (LINK)j, firstSubLINKA, objA, modA,
													&subObjCount);
					if (subObjCount!=LinkNENTRIES(pL))
						MackeysDiseaseMsg(pL, subObjCount, "MP Obj");	/* We're in big trouble */
					
					/* Increment the index to be passed to the next call to WriteSubObjs
						to reflect the number of subobjs just written to file, i.e., keep j in
						sync with the index in the file of the next subobject to be written. */

					j += subObjCount;
				}
		}
	}

	return(ioErr);
}


/* Write out to file <refNum> the number of objects/subobjects in the given heap,
followed by a copy of the HEAP structure (currently used just for error checking).
Return 0 if all OK, else an error code (system I/O error). */

static short WriteHeapHdr(Document */*doc*/, short refNum, short heapIndex)
{
	long count;
	short  ioErr = noErr;
	HEAP *myHeap;
	myHeap = Heap + heapIndex;

	/* Write the total number of objects of type heapIndex */
	count = sizeof(short);
	ioErr = FSWrite(refNum, &count, &objCount[heapIndex]);
	if (ioErr) { SaveError(TRUE, refNum, ioErr, heapIndex); return(ioErr); }

	/* Write the HEAP struct header */
	count = sizeof(HEAP);
	ioErr = FSWrite(refNum, &count, (Ptr)myHeap);
	

#ifndef PUBLIC_VERSION
		if (ShiftKeyDown() && OptionKeyDown()) {
			long position;
			const char *ps;
			
			GetFPos(refNum, &position);
			ps = NameHeapType(heapIndex, FALSE);
			LogPrintf(LOG_NOTICE, "WriteHeapHdr: heap %d (%s) nFObjs=%u  objSize=%d type=%d FPos:%ld\n",
							heapIndex, ps, objCount[heapIndex], myHeap->objSize, myHeap->type, position);
		}
#endif

	if (ioErr) SaveError(TRUE, refNum, ioErr, heapIndex);
	return(ioErr);
}


/* Write the subobjects of an object to file, updating the object <firstSubObj>
link and subobject <next> links to reflect the  new position of the subobjects in
their in-file heap. */

static short WriteSubObjs(short refNum, short heapIndex, LINK pL, LINK link,
									LINK *firstSubLINKA, LINK *objA, LINK *modA,
									short *subObjCount)
{
	LINK nextL, subL, tempL, beamSyncL, tupleSyncL, octSyncL, aModNRL;
	HEAP 			*myHeap;
	PANOTE 		aNote;
	PANOTEBEAM 	aNoteBeam;
	PANOTETUPLE aNoteTuple;
	PANOTEOCTAVA aNoteOct;
	short			ioErr=noErr, count;
	
	/* The links are now being allocated sequentially, so that the <next> link
		of the current link <link> is link+1. */
		
		nextL = link+1;
		myHeap = Heap + heapIndex;
		PushLock(myHeap);
		
	/* First write out the subobject links to their respective in-file heap:
		1. Update the <next> field of each subobj.
			If NILINK, leave the subobj alone; the subobj terminates its list.
			Otherwise, use nextL to update each subobj's <next> field so that it
			follows sequentially the previous subL in the new list starting at
			the new FirstSubLINK(pL) value 'link'.
		2. Write the subobject to file. 
		3. Restore the <next> field to its previous value, so that the data
			structure in memory remains valid. */
		
	for (count = 0, subL = FirstSubLINK(pL); ioErr==noErr && subL!=NILINK;
			subL = tempL, count++) {
		tempL = NextLink(myHeap, subL);
		if (tempL) {
			*(LINK *)LinkToPtr(myHeap, subL) = nextL++;
		}
		switch (heapIndex) {
			case SYNCtype:
				aNote = GetPANOTE(subL);
				if (aNote->firstMod) {
					aModNRL = aNote->firstMod;
					aNote->firstMod = modA[aNote->firstMod];
				}
				break;
			case BEAMSETtype:
				aNoteBeam = GetPANOTEBEAM(subL);
				beamSyncL = aNoteBeam->bpSync;
				aNoteBeam->bpSync = objA[aNoteBeam->bpSync];
				break;
			case TUPLETtype:
				aNoteTuple = GetPANOTETUPLE(subL);
				tupleSyncL = aNoteTuple->tpSync;
				aNoteTuple->tpSync = objA[aNoteTuple->tpSync];
				break;
			case OCTAVAtype:
				aNoteOct = GetPANOTEOCTAVA(subL);
				octSyncL = aNoteOct->opSync;
				aNoteOct->opSync = objA[aNoteOct->opSync];
				break;
		}
		ioErr = WriteObject(refNum, heapIndex, subL);
		if (tempL)
			*(LINK *)LinkToPtr(myHeap, subL) = tempL;
		switch (heapIndex) {
			case SYNCtype:
				aNote = GetPANOTE(subL);
				if (aNote->firstMod)
					aNote->firstMod = aModNRL;
				break;
			case BEAMSETtype:
				aNoteBeam = GetPANOTEBEAM(subL);
				aNoteBeam->bpSync = beamSyncL;
				break;
			case TUPLETtype:
				aNoteTuple = GetPANOTETUPLE(subL);
				aNoteTuple->tpSync = tupleSyncL;
				break;
			case OCTAVAtype:
				aNoteOct = GetPANOTEOCTAVA(subL);
				aNoteOct->opSync = octSyncL;
				break;
		}
	}

	/* Then update the firstSubLink field of the owning object. Note that
		none of the links contain their own addresses, so that the firstSubLink of 
		pL never needs any of its own internal fields updated. For now, we use
		a static array so as to preserve the main object list. */

	firstSubLINKA[pL] = link;
	*subObjCount = count;
	PopLock(myHeap);
	return(ioErr);
}


/* Actually write a block, whether object or subobject, to file. Objects are of
varying lengths: only write out the length of the particular type of object.
Returns an I/O Error code, or noErr. Heap must be locked by the calling routine
for the sake of FSWrite. */
 
static short WriteObject(short refNum, short heapIndex, LINK pL)
{
	HEAP *myHeap;
	long count;
	short ioErr;
	char *p;
	
	myHeap = Heap + heapIndex;
	if (heapIndex==OBJtype) count = objLength[ObjLType(pL)];
	else							count = myHeap->objSize;
	
	p = LinkToPtr(myHeap, pL);
	ioErr = FSWrite(refNum, &count, p);

	if (ioErr) SaveError(TRUE, refNum, ioErr, heapIndex);
	return(ioErr);
}


/* Compute the total number of objects/subobjects of each type and the number of note
modifiers in the object list, and store them in the objCount[] array. Also allocate
arrays for keeping track of object and note modifier links. */

Boolean ComputeObjCounts(Document *doc, LINK **firstSubLINKA, LINK **objA, LINK **modA)
{
	LINK pL, aNoteL, aModNRL;
	PANOTE aNote;
	unsigned short i, j, numMods=0;
	
	for (i=FIRSTtype; i<LASTtype; i++ )
		objCount[i] = 0;
	
	/* Compute the numbers of objects/subobjects in the main object list of each type. */
	for (pL = doc->headL; pL!=RightLINK(doc->tailL); pL = RightLINK(pL)) {
		objCount[OBJtype]++;
		objCount[ObjLType(pL)] += LinkNENTRIES(pL);
	}

	/* Add in the numbers of objects/subobjects in the Master Page of each object type. */
	for (pL = doc->masterHeadL; pL!=RightLINK(doc->masterTailL); pL = RightLINK(pL)) {
		objCount[OBJtype]++;
		objCount[ObjLType(pL)] += LinkNENTRIES(pL);
	}

	/* Compute the numbers of note modifiers (the only type with subobjects but no
		objects: note modifier subobjects are attached to Sync subobjects) in the score
		(the Master Page never contains any). */
	for (pL = doc->headL; pL!=RightLINK(doc->tailL); pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				for (aModNRL = aNote->firstMod; aModNRL; aModNRL = NextMODNRL(aModNRL))
					numMods++;
			}
		}
	objCount[MODNRtype] = numMods;
	
	/* Allocate an array of links to temporarily hold the values of all firstSubLINKs to
	be written to file. */
	
	*firstSubLINKA = (LINK *)NewPtr((Size)(OBJheap->nObjs+1)*sizeof(LINK));
	if (*firstSubLINKA)
		for (j=0; j<OBJheap->nObjs+1; j++)
			(*firstSubLINKA)[j] = NILINK;
	else
		{ OutOfMemory((OBJheap->nObjs+1)*sizeof(LINK)); return FALSE; }

	/* Allocate an array of links to temporarily hold the values of all objLinks to
		be written to file. */
	
	*objA = (LINK *)NewPtr((Size)(OBJheap->nObjs+1)*sizeof(LINK));
	if (*objA)
		for (j=0; j<OBJheap->nObjs+1; j++)
			(*objA)[j] = NILINK;
	else
		{ OutOfMemory((OBJheap->nObjs+1)*sizeof(LINK)); return FALSE; }
	
	for (j=1, pL = doc->headL; pL!=RightLINK(doc->tailL); j++, pL = RightLINK(pL)) 
		(*objA)[pL] = j;

	for (pL = doc->masterHeadL; pL!=RightLINK(doc->masterTailL); j++, pL = RightLINK(pL)) 
		(*objA)[pL] = j;

	/* Allocate an array of links to temporarily hold the values of all modNR's to
		be written to file. */
	
	*modA = (LINK *)NewPtr((Size)(MODNRheap->nObjs+1)*sizeof(LINK));
	if (*modA)
		for (j=0; j<numMods+1; j++)
			(*modA)[j] = NILINK;
	else
		{ OutOfMemory((MODNRheap->nObjs+1)*sizeof(LINK)); return FALSE; }
	
	return TRUE;
}


/* ============================ Functions for Reading Heaps ======================== */

/* Read all heaps from the given file. Returns 0 if no error, else an error code
(either a system result code or one of our own codes). */

short ReadHeaps(Document *doc, short refNum, long version, OSType fdType)
{
	short errCode=0; Boolean isViewerFile;
	
	/* ??I'm not sure if it's a good idea to call HeapFixLinks if there's an error...DB */
	
	isViewerFile = (fdType==DOCUMENT_TYPE_VIEWER);

	PrepareClips();
	errCode = ReadSubHeaps(doc, refNum, version, isViewerFile);
	if (errCode) goto Done;
	errCode = ReadObjHeap(doc, refNum, version, isViewerFile);
Done:
	HeapFixLinks(doc);
	return errCode;
}


/* Given a valid pointer, deliver the type of object that it refers to. Note this
	depends on the type being the 6th field of the object. ??SHOULD USE LINK, NOT
	PTR, IF POSSIBLE, TO GET THIS INFORMATION */
#define ObjPtrType(p)	( *(char *)((5*sizeof(LINK)) + p) )

/* Read the object heap from a file. The objects in the object heap have been
written out without padding, so they're variable-length.  Since (for the sake of
speed) they are stored in memory as fixed-length records (SUPEROBJECTs), the
records read back in must be padded again. We do this by preallocating the entire
heap; reading all the variable-sized objects into it (they are guaranteed to fit)
preceded by the total padding; and then moving the objects, starting at the
beginning of the heap, into their correct positions (followed by per-object padding
if any). This scheme is necessary because we are not explicitly recording the
lengths of the objects we're writing out, and the only way we can tell what these
lengths are is by looking at the type fields, which are at a known offset from the
beginning of each object record.  Thus only a scan forwards through the block can
work. */

static short ReadObjHeap(Document *doc, short refNum, long version, Boolean isViewerFile)
{
	unsigned short nFObjs;
	short ioErr, hdrErr;
	char *p;
	long count, sizeAllObjs, heapSizeAllObjs, blockSize, nExpand;
	HEAP *myHeap;
	char *startPos;
	char *src,*dst; short type; long len,n;

	myHeap = doc->Heap + OBJtype;
	hdrErr = ReadHeapHdr(doc, refNum, version, isViewerFile, OBJtype, &nFObjs);
	if (hdrErr) return hdrErr;

	heapSizeAllObjs = nFObjs * (long)sizeof(SUPEROBJECT);
	
	/* Read in the 4 bytes that give the exact size of all objects written out. */
	
	count = sizeof(long);
	FSRead(refNum,&count,&sizeAllObjs);
	fix_end(sizeAllObjs);
	if (sizeAllObjs>heapSizeAllObjs) {
		AlwaysErrMsg("ReadObjHeap: sizeAllObjs=%ld but nFObjs=%ld gives heapSizeAllObjs=%ld",
					sizeAllObjs, (long)nFObjs, heapSizeAllObjs);
		ioErr = -9999;
		OpenError(TRUE, refNum, ioErr, OBJtype);
		return(ioErr);
	}
	
	nExpand = (long)nFObjs - (long)myHeap->nObjs + EXTRAOBJS;
	if (!ExpandFreeList(myHeap, nExpand))
		{ OpenError(TRUE, refNum, memFullErr, MEM_ERRINFO); return memFullErr; }
	
	PushLock(myHeap);			/* Should lock it after expanding free list above */
	
	/* Set p to point to LINK 1, since LINK 0 is never used. */
	
	blockSize = GetHandleSize(myHeap->block);
	p = *(myHeap->block); p += myHeap->objSize;
	startPos = p + (heapSizeAllObjs - sizeAllObjs);
	
	ioErr = FSRead(refNum, &sizeAllObjs, startPos);
	
	/*
	 *	In files of version >= 'N094', only the useful part of each object is written
	 *	out, so this code moves the contents of the object heap around so each object
	 *	is filled out to the expected SUPEROBJECT size.
	 */
	src = startPos;
	dst = p;
	n = nFObjs;
	while (n-- > 0) {
		/* Move object of whatever type at src down to its anointed LINK slot at dst */
		type = ObjPtrType(src);
		len  = objLength[type];
		
		/*
		 * <len> is now the correct object length for the current file format. If any
		 * object lengths were different in previous file formats, adjust <len> to
		 *	compensate here. (The CONTENT of the object should be fixed in ConvertObjList.) 
		 */
		if (version<='N099') {
			if (type==MEASUREtype) len -= 4;
			else if (type==SYNCtype) len += 2;
		}
		if (version<='N100') {
			if (type==TEMPOtype) len -= 4;
			if (type==BEAMSETtype) len += 8;
		}

		BlockMove(src,dst,len);
		/* And go on to next object and next LINK slot */
		src += len;
		dst += sizeof(SUPEROBJECT);
		/* NOTE: Padding is (sizeof(SUPEROBJECT)-len) bytes, and contains garbage */
	}
	
	PopLock(myHeap);
	if (ioErr)
		{ OpenError(TRUE, refNum, ioErr, OBJtype); return(ioErr); }
	RebuildFreeList(doc, OBJtype, nFObjs);
	return(ioErr);
}

/* 
 * Read all subobject heaps from file. 
 *
 * Note: CER 9/5/2003, PARTINFO conversion, HEADERtype heap.
 * 		N103 conversion to current N105 format.
 *			N103 appears to have been written with 2-byte alignment, yielding 
 *			sizeof(destinationMatch) = 278.
 *			N105 files are written in environment with 4-byte alignment, yielding
 *			sizeof(destinationMatch) = 280.
 *			So newPartFieldsSize and partObjSizeN103 will actually be off by 2 bytes
 *			when computed in the different environments.
 *			The test version<='N103' is therefore not an authoritative test, but
 *			depends on this incidental occurrence, the alignment environment setting
 *			in force in the build environment of the apps which write the files of the
 *			respective formats.
 */

static short ReadSubHeaps(Document *doc, short refNum, long version, Boolean isViewerFile)
{
	HEAP *myHeap;
	short i, ioErr, hdrErr;
	short dynamObjSizeN102, dynamObjSizeN103;
	short partObjSizeN102, partObjSizeN103, partObjSizeN105;
	unsigned short nFObjs;
	long count, nExpand, newPartFieldsSize;
	char *p,*q,*r;
	short sizeDiff,newPartFieldsChunk1,destinationMatchSize2Byte;
	
	partObjSizeN105 = (doc->Heap+HEADERtype)->objSize;
	partObjSizeN103 = partObjSizeN105-2;
	newPartFieldsChunk1 = (sizeof(Byte)*2) + sizeof(fmsUniqueID);
	newPartFieldsSize = newPartFieldsChunk1 + sizeof(destinationMatch);
	partObjSizeN102 = partObjSizeN105 - newPartFieldsSize;	/* lacked bank select & FreeMIDI fields */
//say("oldSize=%d, newSize=%d\n", partObjSizeN102, partObjSizeN103);

	dynamObjSizeN103 = (doc->Heap+DYNAMtype)->objSize;
	dynamObjSizeN102 = dynamObjSizeN103 - sizeof(DDIST);		/* lacked endyd field */
//say("oldSize=%d, newSize=%d\n", dynamObjSizeN102, dynamObjSizeN103);

 	for (i=FIRSTtype; i<LASTtype-1; i++) {
		myHeap = doc->Heap + i;
		
		/* Read the number of subobjects in the heap and the heap header. We must write
			and read the heap header in all cases, regardless of whether there are any
			objects/subobjects in the heap: we can't know beforehand whether there are any
			in a given heap. If file version is old, ignore new fields soon to be added to
			some subobjs when reading heap size. */

		if (version<='N102' && i==HEADERtype) {
			myHeap->objSize = partObjSizeN102;			/* So as not to trigger error in ReadHeapHdr. */
			hdrErr = ReadHeapHdr(doc, refNum, version, isViewerFile, i, &nFObjs);
			myHeap->objSize = partObjSizeN105;
			if (hdrErr) return hdrErr;
		}
		else if (version =='N103' && i==HEADERtype) {
			myHeap->objSize = partObjSizeN103;			/* So as not to trigger error in ReadHeapHdr. */
			hdrErr = ReadHeapHdr(doc, refNum, version, isViewerFile, i, &nFObjs);
			myHeap->objSize = partObjSizeN105;
			if (hdrErr) return hdrErr;
		}
		else if (version<='N102' && i==DYNAMtype) {
			myHeap->objSize = dynamObjSizeN102;			/* So as not to trigger error in ReadHeapHdr. */
			hdrErr = ReadHeapHdr(doc, refNum, version, isViewerFile, i, &nFObjs);
			myHeap->objSize = dynamObjSizeN103;
			if (hdrErr) return hdrErr;
		}
		else {
			hdrErr = ReadHeapHdr(doc, refNum, version, isViewerFile, i, &nFObjs);
			if (hdrErr) return hdrErr;
		}

		if (nFObjs) {
			/* Having found out how many objs are in the heap in the file, we can
				expand, if necessary, the memory pool to accomodate them. */

			if (nFObjs+1 >= myHeap->nObjs) {
				nExpand = (long)nFObjs - (long)myHeap->nObjs + EXTRAOBJS;
				if (!ExpandFreeList(myHeap, nExpand))
					{ OpenError(TRUE, refNum, memFullErr, MEM_ERRINFO); return memFullErr; }
			}

			/* Read nFObjs+1 objects from the file, since the zeroth object is not used. */
			if (version<='N102' && i==HEADERtype) {
				count = (unsigned long)partObjSizeN102 * (unsigned long)nFObjs;
				p = *(myHeap->block); p += myHeap->objSize + (nFObjs*newPartFieldsSize);
			}
			else if (version=='N103' && i==HEADERtype) {
				count = (unsigned long)partObjSizeN103 * (unsigned long)nFObjs;
				sizeDiff = partObjSizeN105-partObjSizeN103;
				p = *(myHeap->block); p += myHeap->objSize + (nFObjs*sizeDiff);
			}
			else if (version<='N102' && i==DYNAMtype) {
				count = (unsigned long)dynamObjSizeN102 * (unsigned long)nFObjs;
				p = *(myHeap->block); p += myHeap->objSize + (nFObjs*sizeof(DDIST));
			}
			else {
				count = (unsigned long)myHeap->objSize * (unsigned long)(nFObjs);
				/* Set p to point to LINK 1, since LINK 0 is never used. */
				p = *(myHeap->block); p += myHeap->objSize;
			}
			PushLock(myHeap);
			ioErr = FSRead(refNum, &count, p);

			/* To make room for new fields, we use the technique described in the comments
				for ReadObjHeap. */
			if (version<='N102') {
				if (i==HEADERtype || i==DYNAMtype) {
					unsigned long j; char *src, *dst;
					short oldSize, newSize;

					switch (i) {
						case HEADERtype:
							oldSize = partObjSizeN102;
							newSize = partObjSizeN105;	// NB: Must track revision number when this size changes
							break;
						case DYNAMtype:
							oldSize = dynamObjSizeN102;
							newSize = dynamObjSizeN103;
							break;
					}
					src = p;
					dst = *(myHeap->block) + newSize;
					for (j = 0; j < (unsigned long)nFObjs; j++) {
						BlockMove(src, dst, oldSize);
						src += oldSize;
						dst += newSize;
					}
				}
			}
			else if (version == 'N103')
			{
				if (i==HEADERtype) {
					unsigned long j; char *src, *dst;
					short oldSize, newSize;

					switch (i) {
						case HEADERtype:
							oldSize = partObjSizeN103;
							newSize = partObjSizeN105;
							break;
					}
					src = p;
					dst = *(myHeap->block) + newSize;
					for (j = 0; j < (unsigned long)nFObjs; j++) {
						BlockMove(src, dst, oldSize);
						
						/*
						 * Code to realign destinationMatch depends on incidental aspect of
						 * build process that N103 was built with 2-byte alignment. Caveat programmer.
						 */
						q = dst + partObjSizeN102 + newPartFieldsChunk1;
						r = q + 2;
						destinationMatchSize2Byte = sizeof(destinationMatch) - 2;
						
						BlockMove(q, r, destinationMatchSize2Byte);
						
						src += oldSize;
						dst += newSize;
					}
				}
			}
			PopLock(myHeap);

			if (ioErr) 
				{ OpenError(TRUE, refNum, ioErr, i); return(ioErr); }
			RebuildFreeList(doc, i, nFObjs);
		}
	}
	
	return 0;
}

/* Read the number of objects/subobjects in the heap and the heap header from the file.
The header is used only for error checking. Return 0 if all OK, else an error code
(either a system I/O error code or one of our own). */

static short ReadHeapHdr(Document *doc, short refNum, long /*version*/, Boolean /*isViewerFile*/,
								short heapIndex, unsigned short *pnFObjs)
{
	long count;
	short  ioErr;
	HEAP *myHeap, tempHeap;
	
	/* Read the total number of objects of type heapIndex */
	count = sizeof(short);
	ioErr = FSRead(refNum, &count, pnFObjs);
	fix_end(*pnFObjs);
	if (ioErr)
		{ OpenError(TRUE, refNum, ioErr, heapIndex); return ioErr; }
	
 	count = sizeof(HEAP);
	ioErr = FSRead(refNum,&count,&tempHeap);
	fix_end(tempHeap.type);
	fix_end(tempHeap.objSize);
	if (ioErr)
		{ OpenError(TRUE, refNum, ioErr, heapIndex); return ioErr; }
	
	myHeap = doc->Heap + heapIndex;

//#ifndef PUBLIC_VERSION
		if (ShiftKeyDown() && OptionKeyDown()) {
			long position;
			const char *ps;
			GetFPos(refNum, &position);
			ps = NameHeapType(heapIndex, FALSE);
			LogPrintf(LOG_NOTICE, "RdHpHdr: hp %ld (%s) nFObjs=%u blk=%ld objSize=%ld type=%ld ff=%ld nO=%ld nf=%ld ll=%ld FPos:%ld\n",
							heapIndex, ps, *pnFObjs, tempHeap.block, tempHeap.objSize, tempHeap.type, tempHeap.firstFree, tempHeap.nObjs, tempHeap.nFree, tempHeap.lockLevel, position);
		}
//#endif

	if (myHeap->type!=tempHeap.type)
		{ OpenError(TRUE, refNum, HDR_TYPE_ERR, heapIndex); return HDR_TYPE_ERR; }
		
	if (myHeap->objSize!=tempHeap.objSize)
		{ OpenError(TRUE, refNum, HDR_SIZE_ERR, heapIndex); return HDR_SIZE_ERR; }

	return 0;
}

//typedef struct {
//	Handle block;					/* Handle to floating array of objects */
//	short objSize;					/* Size in bytes of each object in array */
//	short type;						/* Type of object for this heap */
//	LINK firstFree;				/* Index of head of free list */
//	unsigned short nObjs;		/* Maximum number of objects in heap block */
//	unsigned short nFree;		/* Size of the free list */
//	short lockLevel;				/* Nesting lock level: >0 ==> locked */
//} HEAP;




/* Traverse the main object list and fix up the cross pointers. This code 
assumes that headL is always at LINK 1. */

void HeapFixLinks(Document *doc)
{
	LINK 	pL, prevPage, prevSystem, prevStaff, prevMeasure;
	Boolean tailFound=FALSE;
	
	prevPage = prevSystem = prevStaff = prevMeasure = NILINK;

	fix_end(doc->headL);
	for (pL = doc->headL; !tailFound; pL = DRightLINK(doc, pL)) {
		fix_end(DRightLINK(doc, pL));
		switch(DObjLType(doc, pL)) {
			case TAILtype:
				doc->tailL = pL;
				
				/* If there is no Master Page object list (I think this should happen
					only with ancient scores), then doc->masterHeadL will have just
					been read in as NILINK; do not set it here and confuse the for
					(pL = doc->masterHeadL... ) loop below. */

				if (doc->masterHeadL)
					doc->masterHeadL = pL+1;
				tailFound = TRUE;
				DRightLINK(doc, doc->tailL) = NILINK;
				break;
			case PAGEtype:
				DLinkLPAGE(doc, pL) = prevPage;
				if (prevPage)
					DLinkRPAGE(doc, prevPage) = pL;
				DLinkRPAGE(doc, pL) = NILINK;
				prevPage = pL;
				break;
			case SYSTEMtype:
				DSysPAGE(doc, pL) = prevPage;
				DLinkLSYS(doc, pL) = prevSystem;
				if (prevSystem)
					DLinkRSYS(doc, prevSystem) = pL;
				prevSystem = pL;
				break;
			case STAFFtype:
				DStaffSYS(doc, pL) = prevSystem;
				DLinkLSTAFF(doc, pL) = prevStaff;
				if (prevStaff) DLinkRSTAFF(doc, prevStaff) = pL;
				prevStaff = pL;
				break;
			case MEASUREtype:
				DMeasSYSL(doc, pL) = prevSystem;
				DMeasSTAFFL(doc, pL) = prevStaff;
				DLinkLMEAS(doc, pL) = prevMeasure;
				if (prevMeasure)
					DLinkRMEAS(doc, prevMeasure) = pL;
				prevMeasure = pL;
				break;
			case SLURtype:
				break;
			default:
				break;
		}
	}

	prevPage = prevSystem = prevStaff = prevMeasure = NILINK;

	for (pL = doc->masterHeadL; pL; pL = DRightLINK(doc, pL))
		switch(DObjLType(doc, pL)) {
			case HEADERtype:
				DLeftLINK(doc, doc->masterHeadL) = NILINK;
				break;
			case TAILtype:
				doc->masterTailL = pL;
				DRightLINK(doc, doc->masterTailL) = NILINK;
				return;
			case PAGEtype:
				DLinkLPAGE(doc, pL) = prevPage;
				if (prevPage)
					DLinkRPAGE(doc, prevPage) = pL;
				DLinkRPAGE(doc, pL) = NILINK;
				prevPage = pL;
				break;
			case SYSTEMtype:
				DSysPAGE(doc, pL) = prevPage;
				DLinkLSYS(doc, pL) = prevSystem;
				if (prevSystem)
					DLinkRSYS(doc, prevSystem) = pL;
				prevSystem = pL;
				break;
			case STAFFtype:
				DStaffSYS(doc, pL) = prevSystem;
				DLinkLSTAFF(doc, pL) = prevStaff;
				if (prevStaff) DLinkRSTAFF(doc, prevStaff) = pL;
				prevStaff = pL;
				break;
			case MEASUREtype:
				DMeasSYSL(doc, pL) = prevSystem;
				DMeasSTAFFL(doc, pL) = prevStaff;
				DLinkLMEAS(doc, pL) = prevMeasure;
				if (prevMeasure)
					DLinkRMEAS(doc, prevMeasure) = pL;
				prevMeasure = pL;
				break;
			default:
				break;
		}
		
	/*
	 *	In case we never got into the loop due to missing Master Page object list
	 *	(normally, we return from within the loop when we get to the TAIL object).
	 */
	
	doc->masterTailL = NILINK;
}


/* Rebuild the free list of the heapIndex'th heap */

static void RebuildFreeList(Document *doc, short heapIndex, unsigned short nFObjs)
{
	char *p;
	HEAP *myHeap;
	unsigned short i;
	
	/*
	 *	Set the firstFree object to be the next one past the total number
	 *	already read in.
	 */
	myHeap = doc->Heap + heapIndex;
	myHeap->firstFree = nFObjs+1;
	
	/* Set p to point to the next object past the ones already read in. */
	
	p = *(myHeap->block);
	*(LINK *)p = 1;										/* The <next> field of zeroth object points to first obj */
	p += (unsigned long)myHeap->objSize*(unsigned long)(nFObjs+1);
	
	for (i=nFObjs+1; i<myHeap->nObjs-1; i++) {		/* Skip first nFObjs(??) objects */
		*(LINK *)p = i+1; p += myHeap->objSize;
	}
	*(LINK *)p = NILINK;
	myHeap->nFree = myHeap->nObjs-(nFObjs+1);		/* Since the 0'th object is always unused */
}


/* Preserve the clipboard, and reset the undo and tempUndo clipboards, across
a call to OpenFile. Note that we don't have to delete or free any of the nodes
in any of the clipboards, since the call to RebuildFreeList() re-initializes
all heaps. */

static void PrepareClips()
{
}
