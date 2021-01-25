/******************************************************************************************
*	FILE:	HeapFileIO.c
*	PROJ:	Nightingale
*	DESC:	Routines to read and write object and subobject heaps
*			For information on Nightingale heaps, see Heaps.c and Tech Note #4, "Nightingale
*			Files and Troubleshooting".
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2020 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "FileConversion.h"			/* Must follow Nightingale.precomp.h! */


#define EXTRAOBJS 10L

/* Subobject sizes in 'N105' format files. */

short subObjLength_5[] = {
		sizeof(PARTINFO),		/* HEADER subobject */
		0,						/* TAIL objects can't have subobjects */
		sizeof(ANOTE_5),		/* SYNC subobject */
		sizeof(ARPTEND_5),		/* etc. */
		0,
		0,
		sizeof(ASTAFF_5),
		sizeof(AMEASURE_5),
		sizeof(ACLEF_5),
		sizeof(AKEYSIG_5),
		sizeof(ATIMESIG_5),
		sizeof(ANOTEBEAM_5),
		sizeof(ACONNECT_5),
		sizeof(ADYNAMIC_5),
		sizeof(AMODNR_5),
		sizeof(AGRAPHIC_5),
		sizeof(ANOTEOTTAVA_5),
		sizeof(ASLUR_5),
		sizeof(ANOTETUPLE),
		sizeof(AGRNOTE_5),
		0,
		0,
		0,
		sizeof(APSMEAS_5),
		sizeof(SUPEROBJECT_N105)
	};
		
/* Object sizes in memory (a fixed size, that of SUPEROBJECT_N105) and in 'N105' format
files (size dependent on object type). */

short objLength_5[] = {
		sizeof(HEADER_5),
		sizeof(TAIL_5),
		sizeof(SYNC_5),
		sizeof(RPTEND_5),
		sizeof(PAGE_5),
		sizeof(SYSTEM_5),
		sizeof(STAFF_5),
		sizeof(MEASURE_5),
		sizeof(CLEF_5),
		sizeof(KEYSIG_5),
		sizeof(TIMESIG_5),
		sizeof(BEAMSET_5),
		sizeof(CONNECT_5),
		sizeof(DYNAMIC_5),
		0, 							/* No MODNR objects */
		sizeof(GRAPHIC_5),
		sizeof(OTTAVA_5),
		sizeof(SLUR_5),
		sizeof(TUPLET_5),
		sizeof(GRSYNC_5),
		sizeof(TEMPO_5),
		sizeof(SPACER_5),
		sizeof(ENDING_5),
		sizeof(PSMEAS_5),
		sizeof(SUPEROBJECT_N105)
	};


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
static short WriteObject(short refNum, LINK pL);
static short WriteSubobj(short refNum, short heapIndex, LINK pL);
static void CountObjSubobjs(Document *doc);
static Boolean InitTrackingLinks(Document *, LINK **firstSubLINKA, LINK **objA, LINK **modA);
static Boolean MoveObjSubobjs(short, long, unsigned short, char *, long sizeAllInHeap);
static short ReadObjHeap(Document *doc, short refNum, long version, Boolean isViewerFile);
static short ReadSubHeap(Document *doc, short refNum, long version, short iHp, Boolean isViewerFile);
static short ReadHeapHdr(Document *doc, short refNum, long version, Boolean isViewerFile,
						short heapIndex, unsigned short *pnFObjs);
static short HeapFixLinks(Document *);
static void RebuildFreeList(Document *doc, short heapIndex, unsigned short nFObjs);
static void PrepareClips(void);


/* ============================== Functions for Writing Heaps =========================== */

/* Write all heaps with their headers to the given file. Returns 0 if no error, else
an error code (either a system result code or one of our own codes). */

short WriteHeaps(Document *doc, short refNum)
{
	LINK *firstSubLINKA=NILINK, *objA=NILINK, *modA=NILINK;
	short errCode;
	const char *ps;
	HEAP *myHeap;

	CountObjSubobjs(doc);
	LogPrintf(LOG_INFO, "Objects and subobjects to be written:\n");
	for (short iHp=FIRSTtype; iHp<LASTtype; iHp++ ) {
		if (objCount[iHp]>0) {
			ps = NameHeapType(iHp, False);
			myHeap = Heap + iHp;
			LogPrintf(LOG_INFO, "    heap %d (%s): %d %s%c objSize=%d  (WriteHeaps)\n",
						iHp, ps, objCount[iHp], (iHp==LASTtype-1? "object" : "subobject"),
						(objCount[iHp]==1? ' ' : 's'), myHeap->objSize);
		}
	}

	if (InitTrackingLinks(doc, &firstSubLINKA, &objA, &modA)) {
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

static short WriteObjHeap(Document *doc, short refNum, LINK *firstSubLINKA, LINK *objA)
{
	LINK			pL, oldFirstObj;
	LINK			leftL, rightL, subL, oldLastObj,
					oldFirstSync, oldLastSync, oldLRepeat, oldRRepeat;
	unsigned short	j;
	short			ioErr=noErr, hdrErr;
	long			count, startPosition, endPosition, zero=0L, sizeAllObjsFile;

	PushLock(doc->Heap+OBJtype);

	/* First write out the heap header, plus 4 bytes that will contain the total number
	   of bytes written out for all (variable-sized) objects.  We must save the position
	   in the file where this information is written so we can go back and fill it in
	   after writing the objects. */

	hdrErr = WriteHeapHdr(doc, refNum, OBJtype);
	if (hdrErr) return hdrErr;
	
	GetFPos(refNum, &startPosition);
	count = sizeof(long);
	ioErr = FSWrite(refNum, &count, &zero);
	if (ioErr) { SaveError(True, refNum, ioErr, OBJtype);  return ioErr; }
	
	for (j=1, pL=doc->headL; ioErr==noErr && pL!=NILINK; j++, pL=RightLINK(pL)) {
	
		/* Store old link values. */
		
		leftL = LeftLINK(pL); rightL = RightLINK(pL);
		subL = FirstSubLINK(pL);

		/* Write the link values for the in-file objects. */
		
		FirstSubLINK(pL) = firstSubLINKA[pL];
		LeftLINK(pL) = j-1;
		RightLINK(pL) = j+1;
		if (pL==doc->headL) LeftLINK(pL) = NILINK;
		if (pL==doc->tailL) RightLINK(pL) = NILINK;
		
		/* Save fields in objects that refer to other objects and temporarily set them
		   to the values that are being written out for those objects.  */
		   
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

		ioErr = WriteObject(refNum, pL);
		
		LeftLINK(pL) = leftL;  RightLINK(pL) = rightL;
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
				if (IsHairpin(pL)) DynamLASTSYNC(pL) = oldLastSync;
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
		
		/* There are no special object types in Master Page. */

		ioErr = WriteObject(refNum, pL);
		
		LeftLINK(pL) = leftL; RightLINK(pL) = rightL;
		FirstSubLINK(pL) = subL;
	}

	PopLock(doc->Heap+OBJtype);
	
	/* Now get the current file position to find out how much we've written; then
	   back patch the size into the 4-byte slot we pre-allocated above. NB: We must
	   take into account the 4 bytes for the count as well. */
	
	if (ioErr==noErr) {
		GetFPos(refNum, &endPosition);
		sizeAllObjsFile = endPosition-startPosition-sizeof(long);
		ioErr = SetFPos(refNum, fsFromStart, startPosition);
		if (ioErr!=noError) return ioErr;
		count = sizeof(long);
		FIX_END(sizeAllObjsFile);
		ioErr = FSWrite(refNum, &count, &sizeAllObjsFile);
		if (ioErr) { SaveError(True, refNum, ioErr, OBJtype); return ioErr; }
		
		/* Move back to where we just were so as not to truncate anything. */
		
		ioErr = SetFPos(refNum, fsFromStart, endPosition);
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

	/* j is a parameter passed to WriteSubObjs to indicate the initial LINK address of
	   the subobj list written by WriteSubObjs. We start at 1 since the zeroth heap
	   object is never used. */

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


/* Write the note modifier subobjects to file, updating the subobject <next> links to
reflect the new position of the subobjects in their in-file heap, and filling in the
modA array so that notes can update their firstMod field in the WriteSubObjs routine. */

static short WriteModSubs(short refNum, LINK aNoteL, LINK link, LINK **/*modA*/,
									unsigned short *nMods)
{
	LINK 	nextL, subL, tempL;
	HEAP 	*myHeap;
	PANOTE 	aNote;
	short	ioErr = noErr;
	
	/* The links are now being allocated sequentially, so that the <next> link of
	   the current link <link> is link+1. */
			
	nextL = link+1;
	myHeap = Heap + MODNRtype;
	PushLock(myHeap);
	
	*nMods = 0;
	aNote = GetPANOTE(aNoteL);
	for (subL = aNote->firstMod; ioErr==noErr && subL!=NILINK; subL = tempL) {
		tempL = NextLink(myHeap, subL);
		if (tempL) *(LINK *)LinkToPtr(myHeap, subL) = nextL++;

		ioErr = WriteSubobj(refNum, MODNRtype, subL);

		if (tempL) *(LINK *)LinkToPtr(myHeap, subL) = tempL;

		(*nMods)++;
	}
	PopLock(myHeap);
	return ioErr;
}


/* "Mackey's Disease" is so called because Steve Mackey ran into it repeatedly in the 1990's. */

static void MackeysDiseaseMsg(LINK pL, short subObjCount, char *objString);
static void MackeysDiseaseMsg(LINK pL, short subObjCount, char *objString)
{
	AlwaysErrMsg("FILE IS PROBABLY UNUSABLE. %s at %ld (type=%ld) has nEntries=%ld but subObjCount=%ld. Contact AMNF for help.  (WriteSubHeaps)",
				objString, (long)pL, (long)ObjLType(pL), (long)LinkNENTRIES(pL),
				(long)subObjCount);
}


/* Write all subobjects of all types to the file. For each heap, we write out the
number of subobjects in that heap (as determined by InitTrackingLinks) as part of the
heap header; then we traverse the object list and write out all the subobjects
themselves.

Note that if InitTrackingLinks finds a number for any heap that's different from what
the heap-writing code finds, the results are likely to be disasterous: the file
will be written apparently without problems, but it will probably not be possible to
open it again! The same is true if WriteSubObjs finds a number for any object that's
different from the object's LinkNENTRIES: this is "Mackey's Disease". To at least
alert the user if this happens, we give a strongly-worded message in the latter case,
but FIXME not the former, which is just as serious! */

static short WriteSubHeaps(Document *doc, short refNum, LINK *firstSubLINKA, LINK *objA,
								LINK *modA)
{
	HEAP *myHeap;
	short ioErr = noErr, hdrErr, modErr, subObjCount;
	unsigned short j;
	LINK pL;
	
	for (short iHp=FIRSTtype; ioErr==noErr && iHp<LASTtype-1; iHp++) {

		/* Write the heap header. The subobjects in each heap are of a fixed size,
		   so -- unlike the object heap -- we don't need to write the total number
		   of bytes written out. */
		
		hdrErr = WriteHeapHdr(doc, refNum, iHp);
		if (hdrErr) return hdrErr;

		/* Note modifiers (the only type with subobjects but no objects: note modifier
		   subobjects are attached to Sync subobjects) are a special case, so we
		   handle them separately. For all other types, traverse the main object list
		   and the Master Page object list once for each type and write the subobjects
		   of each object of that type we encounter. */
		
		if (iHp==MODNRtype) {
			modErr = WriteModHeap(doc, refNum, &modA);
			if (modErr) return modErr;
			continue;
		}

		myHeap = Heap + iHp;
		if (myHeap->nObjs) {

			/* Write the subobjects for all objects in the main object list except the
			   tail, which has no subobjects. j is a parameter passed to WriteSubObjs to
			   indicate the initial LINK address of the subobj list written by WriteSubObjs.
			   We start at 1 since the zeroth heap object is never used. */

			for (j=1, pL=doc->headL; ioErr==noErr && pL!=doc->tailL; pL=RightLINK(pL))
				if (ObjLType(pL)==iHp) { 
					ioErr = WriteSubObjs(refNum, iHp, pL, (LINK)j, firstSubLINKA, objA, modA,
													&subObjCount);	
					if (subObjCount!=LinkNENTRIES(pL))
						MackeysDiseaseMsg(pL, subObjCount, "Obj");		/* We're in big trouble! */
					
					/* Increment the index to be passed to the next call to WriteSubObjs()
					   to reflect the number of subobjs just written to file, i.e., keep j
					   in sync with the index in the file of the next subobject to be
					   written. */

					j += subObjCount;
				}

			/* Now write the subobjects for all objects in the Master Page object list
			   except the tail. which has none. */

			for (pL=doc->masterHeadL; ioErr==noErr && pL!=doc->masterTailL; pL=RightLINK(pL))
				if (ObjLType(pL)==iHp) { 
					ioErr = WriteSubObjs(refNum, iHp, pL, (LINK)j, firstSubLINKA, objA, modA,
													&subObjCount);
					if (subObjCount!=LinkNENTRIES(pL))
						MackeysDiseaseMsg(pL, subObjCount, "MP Obj");	/* We're in big trouble */
					
					/* Increment the index to be passed to the next call to WriteSubObjs()
					   to reflect the number of subobjs just written to file, i.e., keep j
					   in sync with the index in the file of the next subobject to be
					   written. */

					j += subObjCount;
				}
		}
	}

	return(ioErr);
}


/* Write out to file <refNum> the number of objects/subobjects in the given heap,
followed by a copy of the HEAP structure. Return 0 if all OK, else an error code
(system I/O error). */

static short WriteHeapHdr(Document *doc, short refNum, short heapIndex)
{
	long count;
	short  ioErr = noErr;
	HEAP *myHeap;
	myHeap = Heap + heapIndex;

	/* Write the total number of objects/subobjects of type heapIndex */
	
	count = sizeof(short);
	FIX_END(objCount[heapIndex]);						/* Ensure in Big Endian form */
	ioErr = FSWrite(refNum, &count, &objCount[heapIndex]);
	FIX_END(objCount[heapIndex]);						/* Back to processor-specific Endian */
	if (ioErr) { SaveError(True, refNum, ioErr, heapIndex);  return(ioErr); }

	/* Write the HEAP struct header */
	
	count = sizeof(HEAP);
	EndianFixHeapHdr(doc, myHeap);						/* Ensure in Big Endian form */
	ioErr = FSWrite(refNum, &count, (Ptr)myHeap);
	EndianFixHeapHdr(doc, myHeap);						/* Back to processor-specific Endian */

	if (DETAIL_SHOW) {
		long position;
		const char *ps;
		GetFPos(refNum, &position);
		ps = NameHeapType(heapIndex, False);
		LogPrintf(LOG_DEBUG, "WriteHeapHdr: heap %d (%s) nFObjs=%u  objSize=%d type=%d FPos:%ld\n",
						heapIndex, ps, objCount[heapIndex], myHeap->objSize, myHeap->type, position);
	}

	if (ioErr) SaveError(True, refNum, ioErr, heapIndex);
	return(ioErr);
}


/* Write the subobjects of object <pL> to file, updating the object <firstSubObj> link
and subobject <next> links to reflect the new position of the subobjects in their
in-file heap. */

static short WriteSubObjs(short refNum, short heapIndex, LINK pL, LINK link,
									LINK *firstSubLINKA, LINK *objA, LINK *modA,
									short *subObjCount)
{
	LINK nextL, subL, tempL, beamSyncL, tupleSyncL, octSyncL, aModNRL;
	HEAP *myHeap;
	PANOTEBEAM aNoteBeam;
	PANOTETUPLE aNoteTuple;
	PANOTEOTTAVA aNoteOct;
	short ioErr=noErr, count;
	
	/* The links are now being allocated sequentially, so that the <next> link of the
	   current link <link> is link+1. */
		
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
				if (NoteFIRSTMOD(subL)) {
					aModNRL = NoteFIRSTMOD(subL);
					NoteFIRSTMOD(subL) = modA[NoteFIRSTMOD(subL)];
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
			case OTTAVAtype:
				aNoteOct = GetPANOTEOTTAVA(subL);
				octSyncL = aNoteOct->opSync;
				aNoteOct->opSync = objA[aNoteOct->opSync];
				break;
		}

		ioErr = WriteSubobj(refNum, heapIndex, subL);
		if (tempL) *(LINK *)LinkToPtr(myHeap, subL) = tempL;

		switch (heapIndex) {
			case SYNCtype:
				if (NoteFIRSTMOD(subL)) NoteFIRSTMOD(subL) = aModNRL;
				break;
			case BEAMSETtype:
				aNoteBeam = GetPANOTEBEAM(subL);
				aNoteBeam->bpSync = beamSyncL;
				break;
			case TUPLETtype:
				aNoteTuple = GetPANOTETUPLE(subL);
				aNoteTuple->tpSync = tupleSyncL;
				break;
			case OTTAVAtype:
				aNoteOct = GetPANOTEOTTAVA(subL);
				aNoteOct->opSync = octSyncL;
				break;
		}
	}

	/* Now update the firstSubLink field of the owning object. Note that none of the
	   links contain their own addresses, so that the firstSubLink of pL never needs
	   any of its own internal fields updated. For now, we use a static array so as to
	   preserve the main object list. */

	firstSubLINKA[pL] = link;
	*subObjCount = count;
	PopLock(myHeap);
	return(ioErr);
}


/* Actually write an object (not a subobject) to file. Objects are of varying lengths;
we write out only the length of the particular type of object. Returns an I/O Error
code or noErr. NB: The heap must be locked by the calling routine for the sake of
FSWrite. */
 
static short WriteObject(short refNum, LINK objL)
{
	HEAP *myHeap;
	long count;
	short ioErr;
	char *p;
	
	myHeap = Heap + OBJtype;
	count = objLength[ObjLType(objL)];
	
	EndianFixObject(objL);								/* Ensure in Big Endian form */
	p = LinkToPtr(myHeap, objL);
	ioErr = FSWrite(refNum, &count, p);
	EndianFixObject(objL);								/* Back to processor-specific Endian */

	if (ioErr) SaveError(True, refNum, ioErr, OBJtype);
	return(ioErr);
}


static short WriteSubobj(short refNum, short heapIndex, LINK subL)
{
	HEAP *myHeap;
	long count;
	short ioErr;
	char *p;
	
	myHeap = Heap + heapIndex;
	count = myHeap->objSize;
	
	EndianFixSubobj(heapIndex, subL);						/* Ensure in Big Endian form */
	p = LinkToPtr(myHeap, subL);
	ioErr = FSWrite(refNum, &count, p);
	EndianFixSubobj(heapIndex, subL);						/* Back to processor-specific Endian */

	if (ioErr) SaveError(True, refNum, ioErr, heapIndex);
	return(ioErr);
}


/* Compute the total number of objects/subobjects of each type and the number of note
modifiers in the object list, and store them in the objCount[] array. */

static void CountObjSubobjs(Document *doc)
{
	LINK pL, aNoteL, aModNRL;
	unsigned short numMods=0;
	
	for (short iHp=FIRSTtype; iHp<LASTtype; iHp++ )
		objCount[iHp] = 0;
	
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
	   objects: note modifier subobjects are attached to Sync subobjects) in the score.
	   (The Master Page never contains note modifiers.) */
		
	for (pL = doc->headL; pL!=RightLINK(doc->tailL); pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				for (aModNRL = NoteFIRSTMOD(aNoteL); aModNRL; aModNRL = NextMODNRL(aModNRL))
					numMods++;
			}
		}
	objCount[MODNRtype] = numMods;
}


/* Allocate and initialize "arrays" for keeping track of firstSubLINKs and object and
note modifier links. NB: The calling routine is responsible for freeing these chunks of
memory. */

static Boolean InitTrackingLinks(Document *doc, LINK **firstSubLINKA, LINK **objA, LINK **modA)
{
	LINK pL;
	unsigned short j, numMods=0;
		
	/* Allocate (but don't fill in) an array to temporarily hold the values of all
	   firstSubLINKs. */
	
	*firstSubLINKA = (LINK *)NewPtr((Size)(OBJheap->nObjs+1)*sizeof(LINK));
	if (*firstSubLINKA)
		for (j=0; j<OBJheap->nObjs+1; j++)
			(*firstSubLINKA)[j] = NILINK;
	else
		{ OutOfMemory((OBJheap->nObjs+1)*sizeof(LINK));  return False; }

	/* Allocate and fill an array to temporarily hold the values of all object links. */
	
	*objA = (LINK *)NewPtr((Size)(OBJheap->nObjs+1)*sizeof(LINK));
	if (*objA)
		for (j=0; j<OBJheap->nObjs+1; j++)
			(*objA)[j] = NILINK;
	else
		{ OutOfMemory((OBJheap->nObjs+1)*sizeof(LINK));  return False; }
	
	for (j=1, pL = doc->headL; pL!=RightLINK(doc->tailL); j++, pL = RightLINK(pL)) 
		(*objA)[pL] = j;

	for (pL = doc->masterHeadL; pL!=RightLINK(doc->masterTailL); j++, pL = RightLINK(pL)) 
		(*objA)[pL] = j;

	/* Allocate and fill an array of links to temporarily hold the values of all modNR's. */
	
	*modA = (LINK *)NewPtr((Size)(MODNRheap->nObjs+1)*sizeof(LINK));
	if (*modA) {
		for (j=0; j<numMods+1; j++)
			(*modA)[j] = NILINK;
		CreateModTable(doc, modA);
	}
	else
		{ OutOfMemory((MODNRheap->nObjs+1)*sizeof(LINK));  return False; }
		
	return True;
}


/* =============================== Functions for Reading Heaps ========================== */

/* Read all heaps from file <refNum>: one subobject heap for each type of object that
has subobjects (almost all do), and one object heap. Returns 0 if no error, else an
error code (either a system result code or one of our own codes). */

short ReadHeaps(Document *doc, short refNum, long version, OSType fdType)
{
	short iHp, errCode=0;
	Boolean isViewerFile;
		
	isViewerFile = (fdType==DOCUMENT_TYPE_VIEWER);

	PrepareClips();
	LogPrintf(LOG_INFO, "Objects and subobjects read:\n");
 	for (iHp=FIRSTtype; iHp<LASTtype-1; iHp++) {
		errCode = ReadSubHeap(doc, refNum, version, iHp, isViewerFile);
		if (errCode) return errCode;
	}
	errCode = ReadObjHeap(doc, refNum, version, isViewerFile);
	if (errCode) return errCode;

	if (version=='N105')	errCode = HeapFixN105Links(doc);
	else					errCode = HeapFixLinks(doc);
	if (errCode)	return errCode;
	else			return 0;
}


/* Move the contents of a heap -- either the object heap or a subobject heap -- around
so each object or subobject has the space it needs in the current format. We assume the
heap was just read from a file in either 'N105' or the current format. Return True if
we succeed, False if not.

If the file is in the current format, this should be called only for the object heap;
subobjects are already the correct length. If it's in 'N105' format, it should be
called for both object and subobject heaps. In that case, the _content_ of objects
and subobjects will still need more work, which should be done in ConvertObjSubobjs().  */

static Boolean MoveObjSubobjs(short hType, long version, unsigned short nFObjs,
				char *pLink1, long sizeAllInHeap)
{
	static Boolean firstCall=True;
	char *src, *dst, *tempHeap;
	short curType;
	long len, newLen, n;

	tempHeap = NewPtr((Size)sizeAllInHeap);
	
	/* Using if (!*tempHeap)... here misbehaves badly; I have no idea why!  --DAB, Oct. 2020 */
	
	if (!GoodNewPtr((Ptr)tempHeap))
		{ OutOfMemory(sizeAllInHeap);  return False; }
	BlockMove(pLink1, tempHeap, sizeAllInHeap);
	
	/* Copy everything to a separate chunk of memory, then copy objects/subobjects
	   back one at a time, assuming the new sizes. */
	   
	src = tempHeap;
	dst = pLink1;
	for (n = 1; n<=nFObjs; n++) {
		if (hType==OBJtype)	curType = ObjPtrTYPE(src);
		else				curType = hType;
		if (curType<0 || curType>LASTtype) {
			LogPrintf(LOG_ERR, "Object type=%d is illegal. hType=%d  (MoveObjSubobjs)\n", curType, hType);
			return False;
		}
		
		/* Set <len> to the correct object/subobject length for the format of the given
		   version and <newLen> to the length in the current version. */

		if (hType==OBJtype)	{
			len = (version=='N105'? objLength_5[curType] : objLength[curType]);
			newLen = sizeof(SUPEROBJECT);
		}
		else {
			len = (version=='N105'? subObjLength_5[curType] : subObjLength[curType]);
			newLen = subObjLength[curType];
		}
		
#define NoDEBUG_LOOP
#ifdef DEBUG_LOOP
		/* Without the call to SleepMS(), some of the output from the following LogPrintf
		   is likely to be lost (at least with OS 10.5 and 10.6)! See the comment on this
		   issue in ConvertObjSubobjs(). */
		   
		if (firstCall && n==1) {
			LogPrintf(LOG_DEBUG, "****** WORKING SLOWLY! DELAYING EACH TIME THRU LOOP TO AID DEBUGGING.  (MoveObjSubobjs) ******\n");
			firstCall = False;
		}
		SleepMS(3);
		LogPrintf(LOG_DEBUG, "MoveObjSubobjs: n=%d curType=%d src=%lx dst=%lxlen=%d newLen=%d\n",
					n, curType, src, dst, len, newLen);
#endif
		   
		/* Copy obj/subobj of whatever type at <src> to its anointed LINK slot at <dst>,
		   then go on to next obj/subobj and next LINK slot. */
		
		BlockMove(src, dst, len);
		src += len;
		dst += newLen;
	}
	
	DisposePtr((Ptr)tempHeap);
	return True;
}


/* Read the objects from a file and make them into the object heap. Return 0 if all
is well; else return an error code.

Nightingale writes objects out without padding, so they're variable-length; but, for
simplicity and speed, we store them in memory as records of fixed length (like the
subobjects that comprise the other heaps). So, after reading them in, they must be
padded to sizeof(SUPEROBJECT), the maximum length for any object. We do this by
preallocating the entire heap with room for all the objects plus all the padding;
reading the variable-sized objects into it preceded by the total padding; and then
moving the objects, starting at the beginning of the heap, down into their correct
positions, followed by per-object padding as needed. This scheme is necessary because we
are not explicitly recording the lengths of the objects we're writing out, and the only
way we can tell what these lengths are is by looking at the type fields, which are at a
known offset from the beginning of each object record. Thus only a scan forwards through
the block can work.

NB: If the file is in an old format, the objects' fields still need to be converted,
including perhaps moving them within the object! That should be done in
ConvertObjSubobjs(). */

static short ReadObjHeap(Document *doc, short refNum, long version, Boolean isViewerFile)
{
	unsigned short nFObjs;
	short ioErr, hdrErr;
	char *pLink1;
	long count, sizeAllObjsFile, sizeAllObjsHeap, nExpand, position;
	HEAP *objHeap;
	const char *ps;

	objHeap = doc->Heap + OBJtype;
	hdrErr = ReadHeapHdr(doc, refNum, version, isViewerFile, OBJtype, &nFObjs);
	if (hdrErr) return hdrErr;

	/* Calculate the eventual size of the object heap; read the total size of all objects
	   in the file; and expand the object heap to the desired size. */
	   
	sizeAllObjsHeap = nFObjs * (long)sizeof(SUPEROBJECT);
		
	count = sizeof(long);
	FSRead(refNum, &count, &sizeAllObjsFile);
	FIX_END(sizeAllObjsFile);

	ps = NameHeapType(OBJtype, False);
	LogPrintf(LOG_INFO, "    heap %d (%s): %d objects. sizeAllObjsFile=%ld sizeAllObjsHeap=%ld  (ReadObjHeap)\n",
								OBJtype, ps, nFObjs, sizeAllObjsFile, sizeAllObjsHeap);

	if (sizeAllObjsFile>sizeAllObjsHeap) {
		AlwaysErrMsg("File is inconsistent. sizeAllObjsFile=%ld is greater than sizeAllObjsHeap=%ld  (ReadObjHeap)",
					sizeAllObjsFile, sizeAllObjsHeap);
		OpenError(True, refNum, MISC_HEAPIO_ERR, OBJtype);
		return MISC_HEAPIO_ERR;
	}
	
	nExpand = (long)nFObjs - (long)objHeap->nObjs + EXTRAOBJS;
	if (!ExpandFreeList(objHeap, nExpand))
		{ OpenError(True, refNum, MEM_FULL_ERR, MEM_ERRINFO); return MEM_FULL_ERR; }
	
	PushLock(objHeap);					/* Should lock it after expanding free list above */
	
	/* LINK 0 is never used, so set pLink1 to point to LINK 1. */
	
	pLink1 = *(objHeap->block);  pLink1 += objHeap->objSize;
	GetFPos(refNum, &position);
	if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ReadObjHeap: pLink1=%ld FPos:%ld\n", pLink1, position);
 	
	ioErr = FSRead(refNum, &sizeAllObjsFile, pLink1);
	
	/* Move the contents of the object heap around so each object has space for the
	   required SUPEROBJECT size. */
	   
	if (!MoveObjSubobjs(OBJtype, version, nFObjs, pLink1, sizeAllObjsHeap)) {
		OpenError(True, refNum, MISC_HEAPIO_ERR, OBJtype);
		return(MISC_HEAPIO_ERR);
	}
	
	PopLock(objHeap);
	if (ioErr) { OpenError(True, refNum, ioErr, OBJtype); return(ioErr); }
	RebuildFreeList(doc, OBJtype, nFObjs);

	if (DETAIL_SHOW) {
		unsigned char *pSObj;
		pSObj = (unsigned char *)GetPSUPEROBJ(1);
		NHexDump(LOG_DEBUG, "ReadObjHeap: L1", pSObj, 46, 4, 16);
		pSObj = (unsigned char *)GetPSUPEROBJ(2);
		NHexDump(LOG_DEBUG, "ReadObjHeap: L2", pSObj, 46, 4, 16);
	}

	return 0;
}


/* Read one subobject heap from file. NB: If the file is in an old format, some
subobjects may have changed size, and we move the entire subobject accordingly to make
room; but the subobjects' fields still need to be converted, including perhaps moving
them within the subobject! That work should be done in ConvertObjSubobjs(). */

static short ReadSubHeap(Document *doc, short refNum, long version, short iHp, Boolean isViewerFile)
{
	unsigned short nFObjs;
	short ioErr, hdrErr;
	char *pLink1;
	long sizeAllInFile, sizeAllInHeap, nExpand, position;
	HEAP *myHeap;
	const char *ps;
	
	myHeap = doc->Heap + iHp;
	
	/* Read the number of subobjects in the heap and the heap header. We write and read
	   the heap header in all cases, regardless of whether there are any subobjects in
	   the heap. */

	hdrErr = ReadHeapHdr(doc, refNum, version, isViewerFile, iHp, &nFObjs);
	if (hdrErr) return hdrErr;

	if (!nFObjs) return 0;

	/* Calculate the eventual size of the heap; read the total size of all subobjects
	   in the file; and expand the heap to the desired size. */

	sizeAllInHeap = nFObjs * (long)subObjLength[iHp];

//GetFPos(refNum, &position);
//LogPrintf(LOG_DEBUG, "ReadSubHeap: iHp=%d fPos=%ld\n", iHp, position);

	if (version=='N105')	sizeAllInFile = nFObjs*subObjLength_5[iHp];
	else					sizeAllInFile = nFObjs*subObjLength[iHp];


	ps = NameHeapType(iHp, False);
	LogPrintf(LOG_INFO, "    heap %d (%s): %d subobject%s sizeAllInFile=%ld sizeAllInHeap=%ld  (ReadSubHeap)\n",
						iHp, ps, nFObjs, (nFObjs==1? ".": "s."), sizeAllInFile, sizeAllInHeap);
	if (sizeAllInFile>sizeAllInHeap) {
		AlwaysErrMsg("File is inconsistent. sizeAllInFile=%ld is greater than sizeAllInHeap=%ld  (ReadSubHeap)",
					sizeAllInFile, sizeAllInHeap);
		OpenError(True, refNum, MISC_HEAPIO_ERR, OBJtype);
		return MISC_HEAPIO_ERR;
	}
	
	nExpand = (long)nFObjs - (long)myHeap->nObjs + EXTRAOBJS;
	if (!ExpandFreeList(myHeap, nExpand))
		{ OpenError(True, refNum, MEM_FULL_ERR, MEM_ERRINFO); return MEM_FULL_ERR; }

	PushLock(myHeap);			/* Should lock it after expanding free list above */

	/* Set pLink1 to point to LINK 1, since LINK 0 is never used, and read nFObjs+1
	   objects from the file. */

	pLink1 = *(myHeap->block);  pLink1 += myHeap->objSize;
	GetFPos(refNum, &position);
	if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "ReadSubHeap: pLink1=%ld FPos:%ld\n", pLink1, position);
	
	ioErr = FSRead(refNum, &sizeAllInFile, pLink1);
//if (DETAIL_SHOW) NHexDump(LOG_DEBUG, "ReadSubHeap0", (unsigned char *)pLink1, 64, 4, 16);

	/* If file is in an old format, move the contents of the subobject heap around
	   so each subobject has space for any new fields. (Unlike objects, subobjects are
	   written out at full length, so if file is in the current format, nothing needs
	   to be moved.) */
	   
	if (version=='N105')
		if (!MoveObjSubobjs(iHp, version, nFObjs, pLink1, sizeAllInHeap)) {
			OpenError(True, refNum, MISC_HEAPIO_ERR, iHp);
			return MISC_HEAPIO_ERR;
		}

	PopLock(myHeap);
	if (ioErr) { OpenError(True, refNum, ioErr, iHp); return ioErr; }
	RebuildFreeList(doc, iHp, nFObjs);
//if (DETAIL_SHOW) NHexDump(LOG_DEBUG, "ReadSubHeap3", (unsigned char *)pLink1, 78, 4, 16);
	
	return 0;
}


/* Read the number of objects/subobjects in the heap and the heap header from the file.
NB: The header itself is used only for error checking! Deliver the number of objs/subobjs
in *pnFObjs. Return function value of 0 if all OK, else an error code (either a system
I/O error code or one of our own). */

static short ReadHeapHdr(Document *doc, short refNum, long version, Boolean /*isViewerFile*/,
								short heapIndex, unsigned short *pnFObjs)
{
	long count;
	short  ioErr, expectedSize;
	HEAP *myHeap, tempHeap;
	
	/* Read the total number of objects/subobjects of type heapIndex */
	
	count = sizeof(short);
	ioErr = FSRead(refNum, &count, pnFObjs);
	FIX_END(*pnFObjs);
	if (ioErr) { OpenError(True, refNum, ioErr, heapIndex);  return ioErr; }
	
	/* Read and check the heap header. Some objects/subobjects have changed size with
	   file format, so if file is in an old format, check for the size expected in that
	   format. */
	
 	count = sizeof(HEAP);
	ioErr = FSRead(refNum, &count, &tempHeap);
	if (ioErr) { OpenError(True, refNum, ioErr, heapIndex);  return ioErr; }
	EndianFixHeapHdr(doc, &tempHeap);						/* Ensure in Big Endian form */
	myHeap = doc->Heap + heapIndex;

	if (DETAIL_SHOW) {
		long position;
		const char *ps;
		GetFPos(refNum, &position);
		ps = NameHeapType(heapIndex, False);
		LogPrintf(LOG_DEBUG, "ReadHeapHdr: hp %ld (%s) nFObjs=%u blk=%ld objSize=%ld type=%ld ff=%ld nO=%ld nf=%ld ll=%ld FPos:%ld\n",
				heapIndex, ps, *pnFObjs, tempHeap.block, tempHeap.objSize, tempHeap.type,
				tempHeap.firstFree, tempHeap.nObjs, tempHeap.nFree, tempHeap.lockLevel, position);
	}

	if (myHeap->type!=tempHeap.type) {
		LogPrintf(LOG_ERR, "Header for heap %d type is %d, but expected type %d  (ReadHeapHdr)\n",
			heapIndex, tempHeap.type, myHeap->type);
		OpenError(True, refNum, HDR_TYPE_ERR, heapIndex);
		return HDR_TYPE_ERR;
	}
		
	expectedSize = myHeap->objSize;
	if (version=='N105' && heapIndex==OBJtype) expectedSize = sizeof(SUPEROBJECT_N105);
	else if (version=='N105') expectedSize = subObjLength_5[heapIndex];

	if (tempHeap.objSize!=expectedSize) {
		LogPrintf(LOG_ERR, "Header for heap %d objSize is %d, but expected objSize %d  (ReadHeapHdr)\n",
			heapIndex, tempHeap.objSize, expectedSize);
		OpenError(True, refNum, HDR_SIZE_ERR, heapIndex);
		return HDR_SIZE_ERR;
	}
	
	return 0;
}


/* Traverse the main and Master Page object lists and fix up the cross pointers.  Return
0 if all is well, else return FIX_LINKS_ERR. */

static short HeapFixLinks(Document *doc)
{
	LINK 	pL, prevPage, prevSystem, prevStaff, prevMeasure;
	Boolean tailFound=False;
	
	prevPage = prevSystem = prevStaff = prevMeasure = NILINK;

	/* First handle the main object list. */
	
	//FIX_END(doc->headL);
	for (pL = doc->headL; !tailFound; pL = DRightLINK(doc, pL)) {
		FIX_END(DRightLINK(doc, pL));
		//LogPrintf(LOG_DEBUG, "HeapFixLinks: pL=%u type=%d in main obj list\n", pL, DObjLType(doc, pL));
		switch(DObjLType(doc, pL)) {
			case TAILtype:
				doc->tailL = pL;
				if (!doc->masterHeadL) {
					LogPrintf(LOG_ERR, "TAIL of main object list encountered before its HEAD.  (HeapFixLinks)\n");
					goto Error;
				}
				doc->masterHeadL = pL+1;
				tailFound = True;
				DRightLINK(doc, doc->tailL) = NILINK;
				break;
			case PAGEtype:
				DLinkLPAGE(doc, pL) = prevPage;
				if (prevPage) DLinkRPAGE(doc, prevPage) = pL;
				DLinkRPAGE(doc, pL) = NILINK;
				prevPage = pL;
				break;
			case SYSTEMtype:
				DSysPAGE(doc, pL) = prevPage;
				DLinkLSYS(doc, pL) = prevSystem;
				if (prevSystem) DLinkRSYS(doc, prevSystem) = pL;
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
				if (prevMeasure) DLinkRMEAS(doc, prevMeasure) = pL;
				prevMeasure = pL;
				break;
			case SLURtype:
				break;
			default:
				break;
		}
	}

{	//unsigned char *pSObj;
//pSObj = (unsigned char *)GetPSUPEROBJ(1);
//NHexDump(LOG_DEBUG, "HeapFixLinks L1", pSObj, 46, 4, 16);
//pSObj = (unsigned char *)GetPSUPEROBJ(2);
//NHexDump(LOG_DEBUG, "HeapFixLinks L2", pSObj, 46, 4, 16);
}
	prevPage = prevSystem = prevStaff = prevMeasure = NILINK;

	/* Now do the Master Page list. */

	for (pL = doc->masterHeadL; pL; pL = DRightLINK(doc, pL)) {
		FIX_END(DRightLINK(doc, pL));
		//LogPrintf(LOG_DEBUG, "HeapFixLinks: pL=%u type=%d in Master Page obj list\n", pL, DObjLType(doc, pL));
		switch(DObjLType(doc, pL)) {
			case HEADERtype:
				DLeftLINK(doc, doc->masterHeadL) = NILINK;
				break;
			case TAILtype:
				doc->masterTailL = pL;
				DRightLINK(doc, doc->masterTailL) = NILINK;
				return 0;
			case PAGEtype:
				DLinkLPAGE(doc, pL) = prevPage;
				if (prevPage) DLinkRPAGE(doc, prevPage) = pL;
				DLinkRPAGE(doc, pL) = NILINK;
				prevPage = pL;
				break;
			case SYSTEMtype:
				DSysPAGE(doc, pL) = prevPage;
				DLinkLSYS(doc, pL) = prevSystem;
				if (prevSystem) DLinkRSYS(doc, prevSystem) = pL;
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
				if (prevMeasure) DLinkRMEAS(doc, prevMeasure) = pL;
				prevMeasure = pL;
				break;
			default:
				break;
		}
	}

	LogPrintf(LOG_ERR, "TAIL of Master Page object list not found.  (HeapFixLinks)\n");
	
Error:
	AlwaysErrMsg("Can't set links in memory for the file!  (HeapFixLinks)");
	doc->masterTailL = NILINK;
	return FIX_LINKS_ERR;
}


/* Rebuild the free list of the heapIndex'th heap. */

static void RebuildFreeList(Document *doc, short heapIndex, unsigned short nFObjs)
{
	char *p;
	HEAP *myHeap;
	unsigned short i;
	
	/* Set the firstFree object to be the next one past the total number already read in. */
	
	myHeap = doc->Heap + heapIndex;
	myHeap->firstFree = nFObjs+1;
	
	/* Set p to point to the next object past the ones already read in. */
	
	p = *(myHeap->block);
	*(LINK *)p = 1;									/* The <next> field of zeroth object points to first obj */
	p += (unsigned long)myHeap->objSize*(unsigned long)(nFObjs+1);
	
	for (i=nFObjs+1; i<myHeap->nObjs-1; i++) {		/* Skip first nFObjs(??) objects */
		*(LINK *)p = i+1; p += myHeap->objSize;
	}
	*(LINK *)p = NILINK;
	myHeap->nFree = myHeap->nObjs-(nFObjs+1);		/* Since the 0'th object is always unused */
}


/* A comment that's been here for many years: "Preserve the clipboard, and reset the undo
and tempUndo clipboards, across a call to OpenFile. Note that we don't have to delete or
free any of the nodes in any of the clipboards, since the call to RebuildFreeList()
re-initializes all heaps." This function may have done that once, but it obviously doesn't
do anything now and it hasn't for many years. --DAB, Feb. 2020 */

static void PrepareClips()
{
}
