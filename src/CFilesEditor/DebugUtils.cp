/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/* File DebugUtils.c - debugging functions - rev. for Nightingale 99:
	DBadLink				DCheckHeaps			DCheckHeadTail
	DCheckSyncSlurs			
	DCheckMBBox			DCheckMeasSubobjs	DCheckNode
	DCheckNodeSel		DCheckSel			DCheckHeirarchy
	DCheckBeams			DCheckOctaves		DCheckSlurs
	DCheckTuplets		DCheckPlayDurs		DCheckHairpins
	DCheckContext		
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

//#ifndef PUBLIC_VERSION		/* If public, skip this file completely! */

#include "DebugUtils.h"

#define DDB

/* These functions implement three levels of checking:
	Most important: messages about problems are prefixed with '¥'
	More important: messages about problems are prefixed with '*'
	Less & Least important: messages about problems have no prefix
*/

extern INT16 nerr, errLim;
extern Boolean minDebugCheck;			/* TRUE=don't print Less and Least important checks */

#ifdef DDB

Boolean QDP(char *fmtStr)
{
	Boolean printAll = !minDebugCheck;

	if (printAll || (*fmtStr=='*' || *fmtStr=='¥')) {
		SysBeep(8);
		nerr++;
		return TRUE;
	}
	
	return FALSE; 
}

#endif

/* If we're looking at the Clipboard, Undo or Mstr Page, "flag" arg by adding a huge offset */
#define FP(pL)	( abnormal ? 1000000L+(pL) : (pL) )

/* Check whether a Rect is a valid 1st-quadrant rectangle with positive height
(Nightingale uses rectangles of zero width for empty key signatures). */
#define GARBAGE_Q1RECT(r)	(  (r).left>(r).right || (r).top>=(r).bottom		\
									|| (r).left<0 || (r).right<0							\
									|| (r).top<0 || (r).bottom<0)
#define ZERODIM_RECT(r)		(  (r).left==(r).right || (r).top==(r).bottom	)


/* --------------------------------------------------------------------- DBadLink ---- */
/* Given a type and a LINK, check whether the LINK is bad and return TRUE if so,
FALSE if it's OK. Checks whether the LINK is larger than the heap currently allows,
and if LINK is on the freelist: in either case, it's not valid. */

Boolean DBadLink(
				Document *doc, INT16 type,
				LINK pL,
				Boolean allowNIL)				/* TRUE=NILINK is an acceptable value */
{
	HEAP *heap; LINK link;
	Byte *p,*start;

	if (pL==NILINK) return !allowNIL;
	
	heap = doc->Heap+type;
	
	if (pL>=heap->nObjs) return TRUE;
	
	/* Look through the freelist for pL. If we find it, it's not valid! */
	
	start = (Byte *)(*heap->block);
	link = heap->firstFree;
	while (link) {
		if (link==pL) return TRUE;
		p = start + ((unsigned long)heap->objSize * (unsigned long)link);
		link = *(LINK *)p;
	}
	
	return FALSE;
}


/* -------------------------------------------------------------------- DCheckHeaps -- */
/* Check whether <doc>'s Heaps are the current global Heaps. */

Boolean DCheckHeaps(Document *doc)
{
	Boolean bad=FALSE;
	
	if (doc->Heap!=Heap) {
		COMPLAIN2("¥DCheckHeaps: doc->Heap=%lx IS NOT EQUAL TO Heap=%lx.\n",
						doc->Heap, Heap);
		return TRUE;	
	}
	else return FALSE;
}



/* ----------------------------------------------------------------- DCheckHeadTail -- */
/* Do consistency check on head or tail of data structure. */

Boolean DCheckHeadTail(
				Document *doc, LINK pL,
				Boolean /*fullCheck*/			/* FALSE=skip less important checks (unused) */
				)
{
	Boolean		bad;
	LINK			partL;
	PPARTINFO	pPartInfo;
	INT16			nextStaff;

	bad = FALSE;

	if (pL==doc->headL || pL==doc->undo.headL || pL==doc->masterHeadL) {
		if (LeftLINK(pL)!=NILINK || DBadLink(doc, OBJtype, RightLINK(pL), TRUE))
			COMPLAIN("¥DCheckHeadTail: HEADER (AT %u) HAS A BAD LINK.\n", pL);
		if (ObjLType(pL)!=HEADERtype)
			COMPLAIN("¥DCheckHeadTail: HEADER (AT %u) HAS BAD type.\n", pL);
				
		if (pL==doc->headL) {
			if (LinkNENTRIES(pL)<2 || LinkNENTRIES(pL)>MAXSTAVES+1)
				COMPLAIN("¥DCheckHeadTail: HEADER (AT %u) HAS BAD nparts.\n", pL);
			if (doc->nstaves<1 || doc->nstaves>MAXSTAVES)
				COMPLAIN("¥DCheckHeadTail: HEADER (AT %u) HAS BAD nstaves.\n", pL);

			partL = FirstSubLINK(pL);												/* Skip zeroth part */
			nextStaff = 1;
			for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL)) {
				pPartInfo = GetPPARTINFO(partL);
				if (pPartInfo->firstStaff!=nextStaff
				||  pPartInfo->lastStaff>doc->nstaves
				||  pPartInfo->firstStaff>pPartInfo->lastStaff)
					COMPLAIN("¥DCheckHeadTail: PART WITH partL=%d HAS BAD FIRST STAFF OR LAST STAFF.\n",
								partL);
				nextStaff = pPartInfo->lastStaff+1;
				if (pPartInfo->transpose<-24 || pPartInfo->transpose>24)
					COMPLAIN2("DCheckHeadTail: PART WITH partL=%d HAS SUSPICIOUS TRANSPOSITION %d.\n",
								partL, pPartInfo->transpose);
			}
			if (nextStaff-1!=doc->nstaves)
				COMPLAIN("¥DCheckHeadTail: LAST PART'S LAST STAFF OF %ld DISAGREES WITH nstaves.\n",
					(long)nextStaff-1);
		}
	}

	else if (pL==doc->tailL || pL==doc->undo.tailL || pL==doc->masterTailL) {
		if (DBadLink(doc, OBJtype, LeftLINK(pL), TRUE) || RightLINK(pL)!=NILINK)
			COMPLAIN("¥DCheckHeadTail: TAIL (AT %u) HAS A BAD LINK.\n", pL);
		if (ObjLType(pL)!=TAILtype)
			COMPLAIN("¥DCheckHeadTail: TAIL (AT %u) HAS BAD type.\n", pL);
	}
	return bad;
}


/* ---------------------------------------------------------------- DCheckSyncSlurs -- */
/* Given a note, if it claims to be slurred or tied, check for a slur object for its
voice that refers to it. If it's tied, also look for a match for its note number in
the "other" Sync. */

Boolean DCheckSyncSlurs(LINK syncL, LINK aNoteL)
{
	SearchParam pbSearch;
	LINK			prevSyncL, slurL, searchL, otherSyncL;
	INT16			voice;
	Boolean		bad;

	bad = FALSE;

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;										/* Prepare for search */
	voice = pbSearch.voice = NoteVOICE(aNoteL);

	pbSearch.subtype = FALSE;									/* Slur, not tieset */

	if (NoteSLURREDL(aNoteL)) {
		/* If we start searching for the slur here, we may find one that starts with this
			Sync, while we want one that ends with this Sync; instead, start searching
			from the previous Sync in this voice. Exception: if the previous Sync is not
			in the same System, the slur we want is the second piece of a cross-system
			one; in this case, search to the right from the previous Measure.
			Cf. LeftSlurSearch. */
			
		prevSyncL = LVSearch(LeftLINK(syncL), SYNCtype, voice, GO_LEFT, FALSE);
		if (prevSyncL && SameSystem(syncL, prevSyncL)) {
			searchL = prevSyncL;
			slurL = L_Search(searchL, SLURtype, GO_LEFT, &pbSearch);
		}
		else {
			searchL = LSSearch(syncL, MEASUREtype, 1, GO_LEFT, FALSE);
			slurL = L_Search(searchL, SLURtype, GO_RIGHT, &pbSearch);
		}

		if (!slurL) {
			COMPLAIN2("DCheckSyncSlurs: NOTE slurredL IN VOICE %d IN SYNC AT %u HAS NO SLUR.\n",
						voice, syncL);
		}
		else {
			if (SlurLASTSYNC(slurL)!=syncL) {
				COMPLAIN2("DCheckSyncSlurs: NOTE slurredL IN VOICE %d IN SYNC AT %u HAS BAD SLUR.\n",
							voice, syncL);
			}
		}
	}
	
	if (NoteSLURREDR(aNoteL)) {
		slurL = L_Search(syncL, SLURtype, GO_LEFT, &pbSearch);
		if (!slurL) {
			COMPLAIN2("DCheckSyncSlurs: NOTE slurredR IN VOICE %d IN SYNC AT %u HAS NO SLUR.\n",
						voice, syncL);
		}
		else {
			if (SlurFIRSTSYNC(slurL)!=syncL) {
				COMPLAIN2("DCheckSyncSlurs: NOTE slurredR IN VOICE %d IN SYNC AT %u HAS BAD SLUR.\n",
							voice, syncL);
			}
		}
	}

	pbSearch.subtype = TRUE;									/* Tieset, not slur */

	if (NoteTIEDL(aNoteL)) {
		/* If we start searching for the slur here, we may find one that starts with this
			Sync, while we want one that ends with this Sync; instead, start searching
			from the previous Sync in this voice. Exception: if the previous Sync is not
			in the same System, the slur we want is the second piece of a cross-system
			one; in this case, search to the right from the previous Measure.
			Cf. LeftSlurSearch. */
			
		prevSyncL = LVSearch(LeftLINK(syncL), SYNCtype, voice, GO_LEFT, FALSE);
		if (prevSyncL && SameSystem(syncL, prevSyncL)) {
			searchL = prevSyncL;
			slurL = L_Search(searchL, SLURtype, GO_LEFT, &pbSearch);
		}
		else {
			searchL = LSSearch(syncL, MEASUREtype, 1, GO_LEFT, FALSE);
			slurL = L_Search(searchL, SLURtype, GO_RIGHT, &pbSearch);
		}

		if (!slurL) {
			COMPLAIN2("DCheckSyncSlurs: NOTE tiedL IN VOICE %d IN SYNC AT %u HAS NO TIE.\n",
						voice, syncL);
		}
		else {
			if (SlurLASTSYNC(slurL)!=syncL) {
				COMPLAIN2("DCheckSyncSlurs: NOTE tiedL IN VOICE %d IN SYNC AT %u HAS BAD TIE.\n",
							voice, syncL);
			}
			else {
				otherSyncL = SlurFIRSTSYNC(slurL);
				if (SyncTYPE(otherSyncL)) {
					if (!FindNoteNum(otherSyncL, voice, NoteNUM(aNoteL)))
						COMPLAIN2("DCheckSyncSlurs: NOTE IN VOICE %d IN SYNC AT %u TIED BUT SYNC IT'S TIED TO DOESN'T MATCH ITS noteNum.\n",
										voice, syncL);
				}
			}
		}
	}
	
	if (NoteTIEDR(aNoteL)) {
		slurL = L_Search(syncL, SLURtype, GO_LEFT, &pbSearch);
		if (!slurL) {
			COMPLAIN2("DCheckSyncSlurs: NOTE tiedR IN VOICE %d IN SYNC AT %u HAS NO TIE.\n",
						voice, syncL);
		}
		else {
			if (SlurFIRSTSYNC(slurL)!=syncL) {
				COMPLAIN2("DCheckSyncSlurs: NOTE tiedR IN VOICE %d IN SYNC AT %u HAS BAD TIE.\n",
							voice, syncL);
			}
			else {
				otherSyncL = SlurLASTSYNC(slurL);
				if (SyncTYPE(otherSyncL)) {
					if (!FindNoteNum(otherSyncL, voice, NoteNUM(aNoteL)))
						COMPLAIN2("DCheckSyncSlurs: NOTE IN VOICE %d IN SYNC AT %u TIED BUT SYNC IT'S TIED TO DOESN'T MATCH ITS noteNum.\n",
										voice, syncL);
				}
			}
		}
	}
	
	return bad;			
}


/* -------------------------------------------------------------------- DCheckMBBox -- */

Boolean DCheckMBBox(
					LINK pL,
					Boolean /*abnormal*/		/* Node in clipboard, Undo or Mstr Page? */
					)
{
	LINK systemL;
	PSYSTEM pSystem;
	Rect mBBox, sysRect, unRect;
	PMEASURE pMeasure;
	Boolean bad=FALSE;
	char str[256];
	
	systemL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	pSystem = GetPSYSTEM(systemL);
	D2Rect(&pSystem->systemRect, &sysRect);
	pMeasure = GetPMEASURE(pL);
	mBBox = pMeasure->measureBBox;
	UnionRect(&mBBox, &sysRect, &unRect);
	if (!EqualRect(&sysRect, &unRect)) {
		sprintf(str, "(%s%s%s%s)\n",
			(mBBox.top<sysRect.top? "top " : ""),
			(mBBox.left<sysRect.left? "left " : ""),
			(mBBox.bottom>sysRect.bottom? "bottom " : ""),
			(mBBox.right>sysRect.right? "right" : "") );
		COMPLAIN2("DCheckMBBox: MEASURE AT %u BBOX NOT IN SYSTEMRECT %s", pL, str);
	}
	return bad;
}


/* -------------------------------------------------------------- DCheckMeasSubobjs -- */
/* Assumes the MEASUREheap is locked. */

Boolean DCheckMeasSubobjs(
					Document *doc,
					LINK pL,					/* Measure */
					Boolean /*abnormal*/)
{
	LINK aMeasL; PAMEASURE aMeas;
	Boolean haveMeas[MAXSTAVES+1], foundConn;
	INT16 s, missing, connStaff[MAXSTAVES+1], nBadConns;
	Boolean bad=FALSE;

	for (s = 1; s<=doc->nstaves; s++)
		haveMeas[s] = FALSE;

	for (aMeasL = FirstSubLINK(pL); aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
		aMeas = GetPAMEASURE(aMeasL);
		if (STAFFN_BAD(doc, aMeas->staffn)) {
			COMPLAIN2("*DCheckMeasSubobjs: SUBOBJ IN MEASURE AT %u HAS BAD staffn=%d.\n",
							pL, aMeas->staffn);
		}
		else {
			haveMeas[aMeas->staffn] = TRUE;
			connStaff[aMeas->staffn] = aMeas->connStaff;
		}

		if (aMeas->staffn==1 && aMeas->connAbove)
			COMPLAIN("*DCheckMeasSubobjs: SUBOBJ ON STAFF 1 IN MEASURE AT %u HAS connAbove.\n", pL);
		if (aMeas->connStaff<0 ||
		    (aMeas->connStaff>0 && (aMeas->connStaff<=aMeas->staffn || aMeas->connStaff>doc->nstaves)))
			COMPLAIN2("*DCheckMeasSubobjs: SUBOBJ ON STAFF %d IN MEASURE AT %u HAS BAD connStaff.\n",
							aMeas->staffn, pL);
		if (aMeas->clefType<LOW_CLEF ||  aMeas->clefType>HIGH_CLEF)
			COMPLAIN("*DCheckMeasSubobjs: SUBOBJ IN MEASURE AT %u HAS BAD clefType.\n", pL);
		if (aMeas->nKSItems<0 ||  aMeas->nKSItems>7)
			COMPLAIN("*DCheckMeasSubobjs: SUBOBJ IN MEASURE AT %u HAS BAD nKSItems.\n", pL);
		if (TSTYPE_BAD(aMeas->timeSigType))
			COMPLAIN("*DCheckMeasSubobjs: SUBOBJ IN MEASURE AT %u HAS BAD timeSigType.\n", pL);
		if (TSNUM_BAD(aMeas->numerator))
			COMPLAIN("*DCheckMeasSubobjs: SUBOBJ IN MEASURE AT %u HAS BAD TIMESIG numerator.\n", pL);
		if (TSDENOM_BAD(aMeas->denominator))
			COMPLAIN("*DCheckMeasSubobjs: SUBOBJ IN MEASURE AT %u HAS BAD TIMESIG denominator.\n", pL);
		if (TSDUR_BAD(aMeas->numerator, aMeas->denominator))
			COMPLAIN("*DCheckMeasSubobjs: SUBOBJ IN MEASURE AT %u HAS TIMESIG OF TOO GREAT DURATION.\n", pL);
	}

	for (missing = 0, s = 1; s<=doc->nstaves; s++)
		if (!haveMeas[s]) missing++;					
	if (missing>0) {
		COMPLAIN2("*DCheckMeasSubobjs: MEASURE AT %u HAS %d MISSING STAVES.\n", pL, missing);
	}
	else {
		nBadConns = 0;
		for (aMeasL = FirstSubLINK(pL); aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
			aMeas = GetPAMEASURE(aMeasL);
			if (aMeas->connAbove) {
				for (foundConn = FALSE, s = 1; s<aMeas->staffn; s++)
					if (connStaff[s]>=aMeas->staffn) foundConn = TRUE;
				if (!foundConn) nBadConns++;
			}
		}
		if (nBadConns>0)
			COMPLAIN2("*DCheckMeasSubobjs: %d SUBOBJS IN MEASURE AT %u HAVE connAbove BUT NO CONNECTED STAFF.\n",
							nBadConns, pL);
	}

	return bad;
}


/* --------------------------------------------------------------------- DCheckNode -- */
/* Do legality and simple (context-free or dependent only on "local" context)
consistency checks on an individual node. Returns:
	0 if no problems are found,
	-1 if the left or right link is garbage or inconsistent,
	+1 if less serious problems are found.
*/

INT16 DCheckNode(
				Document *doc,
				LINK pL,
				INT16 where,		/* Which object list: MAIN_DSTR,CLIP_DSTR,UNDO_DSTR,or MP_DSTR */
				Boolean fullCheck	/* FALSE=skip less important checks */
				)
{
	INT16			minEntries, maxEntries;
	Boolean		bad;
	Boolean		terrible, abnormal,
					objRectOrdered, lRectOrdered, rRectOrdered;
	PMEVENT		p;
	PMEVENT		apLeft, apRight, pLeft, pRight;
	PSYSTEM		pSystem;
	PSTAFF		pStaff;
	PMEASURE		pMeasure;
	PANOTE		aNote;
	PASTAFF		aStaff;
	PAPSMEAS		aPSMeas;
	PAKEYSIG		aKeySig;
	PATIMESIG	aTimeSig;
	PANOTEBEAM	aNoteBeam;
	PANOTETUPLE	aNoteTuple;
	PANOTEOCTAVA	aNoteOct;
	PDYNAMIC		pDynamic;
	PSLUR			pSlur;
	PASLUR		aSlur;
	PACONNECT	aConnect;
	PAMODNR		aModNR;
	LINK			aNoteL;
	LINK			apLeftL, apRightL, aStaffL;
	LINK			aPSMeasL, aClefL, aKeySigL, aTimeSigL, aNoteBeamL, aNoteTupleL;
	LINK			aNoteOctL, aDynamicL, aSlurL, aConnectL, aModNRL;
	SignedByte	minLDur;

	bad = terrible = FALSE;													/* I like it fine so far */
	PushLock(OBJheap);

	p = GetPMEVENT(pL);
	pLeft = GetPMEVENT(p->left);
	pRight = GetPMEVENT(p->right);

	abnormal = (where!=MAIN_DSTR);
	
	if (where==MAIN_DSTR && (pL==doc->headL || pL==doc->tailL))
		DCheckHeadTail(doc, pL, fullCheck);
	else if (where==UNDO_DSTR && (pL==doc->undo.headL || pL==doc->undo.tailL))
		DCheckHeadTail(doc, pL, fullCheck);
	else if (where==MP_DSTR && (pL==doc->masterHeadL || pL==doc->masterTailL))
		DCheckHeadTail(doc, pL, fullCheck);
	else if (where==CLIP_DSTR && (pL==clipboard->headL || pL==clipboard->tailL))
		DCheckHeadTail(clipboard, pL, fullCheck);		/* clipboard is a complete separate doc */

	else {

/* CHECK self, left, and right links. ------------------------------------------- */

		if (DBadLink(doc, OBJtype, pL, FALSE)) {
				COMPLAIN("¥DCheckNode: NODE AT %u IS A GARBAGE LINK.\n", pL);
				terrible = TRUE;
		}

		if (LeftLINK(pL)==NILINK || RightLINK(pL)==NILINK
		||  DBadLink(doc, OBJtype, LeftLINK(pL), TRUE)
		||  DBadLink(doc, OBJtype, RightLINK(pL), TRUE)) {
				COMPLAIN("¥DCheckNode: NODE AT %u HAS A GARBAGE L OR R LINK.\n", pL);
				terrible = TRUE;
		}
		else if (pLeft->right!=pL || pRight->left!=pL) {
			COMPLAIN("¥DCheckNode: NODE AT %u HAS AN INCONSISTENT LINK.\n", pL);
			terrible = TRUE;
		}
		
		if (terrible) return -1;												/* Give up now */

		if (TYPE_BAD(pL) || ObjLType(pL)==HEADERtype || ObjLType(pL)==TAILtype)
			COMPLAIN2("¥DCheckNode: NODE AT %u HAS BAD type %d.\n", pL, ObjLType(pL));
			
		if (fullCheck)
			if (DCheck1SubobjLinks(doc, pL)) bad = terrible = TRUE;

		if (DCheck1NEntries(doc, pL)) bad = terrible = TRUE;

		if (terrible) return -1;												/* Give up now */

		GetObjectLimits(ObjLType(pL), &minEntries, &maxEntries, &objRectOrdered);
		if (LinkNENTRIES(pL)<minEntries || LinkNENTRIES(pL)>maxEntries)
			COMPLAIN("¥DCheckNode: NODE AT %u HAS BAD nEntries FOR ITS TYPE.\n", pL);
			
/* CHECK object's absolute horizontal position (rather crudely). ---------- */

		if (!BeamsetTYPE(pL))						/* Beamset xd is unused */
			if (LinkXD(pL)<LEFT_HLIM(doc, pL) || LinkXD(pL)>RIGHT_HLIM(doc))
				{ COMPLAIN("DCheckNode: NODE AT %u HAS BAD HORIZONTAL POSITION.\n", pL); }

		if (!abnormal && LinkVALID(pL)) {
			if (GARBAGE_Q1RECT(p->objRect)) {
				COMPLAIN("DCheckNode: NODE AT %u HAS A GARBAGE (UNSELECTABLE) objRect.\n", pL);
			}
			
			/* Valid initial objects, e.g., "deleted", can have zero-width objRects. */
			else if (!ClefTYPE(pL) && !KeySigTYPE(pL) && !TimeSigTYPE(pL)
						&& ZERODIM_RECT(p->objRect)) {
				COMPLAIN("DCheckNode: NODE AT %u HAS A ZERO-WIDTH/HEIGHT objRect.\n", pL);
			}

			else if (objRectOrdered) {

/* CHECK the objRect's relative horizontal position. ----------------------------
 *	We first try find objects in this System to its left and/or right in the data
 *	structure that have meaningful objRects. If we find such object(s), we check whether
 *	their relative graphic positions agree with their relative data structure positions.
 */
				for (lRectOrdered = FALSE, apLeftL = LeftLINK(pL);
						apLeftL!=doc->headL; apLeftL = LeftLINK(apLeftL)) {
					if (ObjLType(apLeftL)==SYSTEMtype) break;
					GetObjectLimits(ObjLType(apLeftL), &minEntries, &maxEntries, &lRectOrdered);
					if (LinkVALID(apLeftL) && lRectOrdered) break; 
				}
				for (rRectOrdered = FALSE, apRightL = RightLINK(pL);
						apRightL!=doc->tailL; apRightL = RightLINK(apRightL)) {
					if (ObjLType(apRightL)==SYSTEMtype) break;
					GetObjectLimits(ObjLType(apRightL), &minEntries, &maxEntries, &rRectOrdered);
					if (LinkVALID(apRightL) && rRectOrdered) break; 
				}
				if (fullCheck) {
					apLeft = GetPMEVENT(apLeftL);
					if (LinkVALID(apLeftL) && lRectOrdered
					&&  p->objRect.left < apLeft->objRect.left)
						COMPLAIN("DCheckNode: NODE AT %u: objRect.left REL TO PREVIOUS IS SUSPICIOUS.\n", pL);
					apRight = GetPMEVENT(apRightL);
					if (LinkVALID(apRightL) && rRectOrdered
					&&  p->objRect.left > apRight->objRect.left)
						COMPLAIN("DCheckNode: NODE AT %u: objRect.left REL TO NEXT IS SUSPICIOUS.\n", pL);
				}
			}
		}
		
/* CHECK object's relative vertical position: should be 0 for most types. ---------- */

		switch (ObjLType(pL)) {
			case SYSTEMtype:
			case GRAPHICtype:
			case TEMPOtype:
			case ENDINGtype:
				break;
			default:
				if (LinkYD(pL)!=0)
					COMPLAIN("*DCheckNode: NODE AT %u VERTICAL POSITION SHOULD BE ZERO FOR ITS TYPE BUT ISN'T.\n",
									pL);
		}

/* CHECK everything else. Object type dependent. ----------------------------- */

		switch (ObjLType(pL)) {
		
			case SYNCtype:
			{
				INT16 v;
				INT16	vNotes[MAXVOICES+1],							/* No. of notes in sync in voice */
						vMainNotes[MAXVOICES+1],					/* No. of stemmed notes in sync in voice */
						vlDur[MAXVOICES+1],							/* l_dur of voice's notes in sync */
						vlDots[MAXVOICES+1],							/* Number of dots on voice's notes in sync */
						vStaff[MAXVOICES+1],							/* Staff the voice is on */
						vOct[MAXVOICES+1],							/* inOctava status for the voice */
						lDurCode;
				Boolean vTuplet[MAXVOICES+1];						/* Is the voice in a tuplet? */
				long	timeStampHere, timeStampBefore, timeStampAfter;
				LINK	tempL;

			/* Do consistency checks on relative times of this Sync and its neighbors. */
				if (!abnormal) {
					timeStampHere = SyncTIME(pL);
					if (timeStampHere>MAX_SAFE_MEASDUR) {
						COMPLAIN2("DCheckNode: SYNC AT %u: LTIME %ld IS PROBABLY BAD.\n",
										pL, timeStampHere);
					}
					else {
						timeStampHere = SyncAbsTime(pL);
						tempL = LSSearch(LeftLINK(pL), SYNCtype, ANYONE, GO_LEFT, FALSE);
						if (tempL) {
							timeStampBefore = SyncAbsTime(tempL);
							if (timeStampBefore>=0L && timeStampHere>=0L
							&&  timeStampBefore>=timeStampHere)
								COMPLAIN2("DCheckNode: SYNC AT %u: TIMESTAMP %ld REL TO PREVIOUS IS WRONG.\n",
												pL, timeStampHere);
						}
						tempL = LSSearch(RightLINK(pL), SYNCtype, ANYONE, GO_RIGHT, FALSE);
						if (tempL) {
							timeStampAfter = SyncAbsTime(tempL);
							if (timeStampAfter>=0L && timeStampHere>=0L
							&&  timeStampAfter<=timeStampHere)
								COMPLAIN2("DCheckNode: SYNC AT %u: TIMESTAMP %ld REL TO NEXT IS WRONG.\n",
												pL, timeStampHere);
						}
					}
				}
				
			/* Do subobject count and voice-by-voice consistency checks. */
				for (v = 0; v<=MAXVOICES; v++) {
					vNotes[v] = vMainNotes[v] = 0;
					vTuplet[v] = FALSE;
				}
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					vNotes[aNote->voice]++;
					if (MainNote(aNoteL)) vMainNotes[aNote->voice]++;
					vStaff[aNote->voice] = aNote->staffn;
					vlDur[aNote->voice] = aNote->subType;
					vlDots[aNote->voice] = aNote->ndots;
					vOct[aNote->voice] = aNote->inOctava;
					if (aNote->inTuplet) vTuplet[aNote->voice] = TRUE;
				}

				for (v = 0; v<=MAXVOICES; v++) {
					if (vNotes[v]>0 && vMainNotes[v]!=1)
						COMPLAIN2("*DCheckNode: VOICE %d IN SYNC AT %u HAS ZERO OR TWO OR MORE MainNotes.\n",
										v, pL);
					if (vTuplet[v])
						if (LVSearch(pL, TUPLETtype, v, TRUE, FALSE)==NILINK)
							COMPLAIN2("*DCheckNode: CAN'T FIND TUPLET FOR VOICE %d IN SYNC AT %u.\n",
											v, pL);
				}
				
				/* Check multibar rests: if any voice has one, all must be the same. */
				for (lDurCode = 0, v = 0; v<=MAXVOICES; v++)
					if (vNotes[v]>0) {
						if (lDurCode==0)
							lDurCode = vlDur[v];
						else {
							if ((lDurCode<-1 || vlDur[v]<-1)
							&&   lDurCode!=vlDur[v])
								COMPLAIN2("DCheckNode: VOICE %d IN SYNC AT %u SHOWS MULTIBAR REST INCONSISTENCY.\n",
												v, pL);
						}
					}
				
				/* Check consistency of staff no. in chords, but complain about each no more than once. */
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (vStaff[aNote->voice]!=-999 && aNote->staffn!=vStaff[aNote->voice]) {
						COMPLAIN2("DCheckNode: VOICE %d IN SYNC AT %u ON DIFFERENT STAVES.\n",
										aNote->voice, pL);
						vStaff[aNote->voice] = -999;
						}
				}

				/* Check consistency of durations in chords, but complain about each no more than once. */
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if ( (vlDur[aNote->voice]!=-999 && aNote->subType!=vlDur[aNote->voice])
					||   (vlDots[aNote->voice]!=-999 && aNote->ndots!=vlDots[aNote->voice]) ) {
						COMPLAIN2("*DCheckNode: VOICE %d IN SYNC AT %u HAS VARYING DURATIONS.\n",
										aNote->voice, pL);
						vlDur[aNote->voice] = vlDots[aNote->voice] = -999;
						}
				}
									
				/* Check consistency of octave signs in chords, but complain about each no more than once. */
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (vOct[aNote->voice]!=-999 && aNote->inOctava!=vOct[aNote->voice]) {
						COMPLAIN2("DCheckNode: *VOICE %d IN SYNC AT %u HAS VARYING inOctava FLAGS.\n",
										aNote->voice, pL);
						vOct[aNote->voice] = -999;
						}
				}

			/* Do all other checking on SYNCs. */
				PushLock(NOTEheap);
				
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (STAFFN_BAD(doc, aNote->staffn))
						COMPLAIN2("*DCheckNode: NOTE/REST IN SYNC AT %u HAS BAD staffn %d.\n",
										pL, aNote->staffn);
					if (VOICE_BAD(aNote->voice))
						COMPLAIN2("*DCheckNode: NOTE/REST IN SYNC AT %u HAS BAD voice %d.\n",
										pL, aNote->voice);
					if (aNote->accident<0 || aNote->accident>5)
						COMPLAIN("*DCheckNode: NOTE/REST IN SYNC AT %u HAS BAD ACCIDENTAL.\n", pL);

					if (!aNote->rest) {
						if (aNote->noteNum<1 || aNote->noteNum>MAX_NOTENUM) {
							COMPLAIN2("*DCheckNode: NOTE IN SYNC AT %u HAS BAD noteNum %d.\n",
											pL, aNote->noteNum);
						}

						/* If noteNum is outside the range of the piano, warn more gently. */
						else if (aNote->noteNum<21 || aNote->noteNum>108) {
							COMPLAIN2("DCheckNode: NOTE IN SYNC AT %u HAS SUSPICIOUS noteNum %d.\n",
											pL, aNote->noteNum);
						}

						if (aNote->onVelocity<0 || aNote->onVelocity>MAX_VELOCITY)
							COMPLAIN2("*DCheckNode: NOTE IN SYNC AT %u HAS BAD onVelocity %d.\n",
											pL, aNote->onVelocity);
						if (aNote->offVelocity<0 || aNote->offVelocity>MAX_VELOCITY)
							COMPLAIN2("*DCheckNode: NOTE IN SYNC AT %u HAS BAD offVelocity %d.\n",
											pL, aNote->offVelocity);
						if (fullCheck) {
							if (aNote->onVelocity==0)
								COMPLAIN("DCheckNode: NOTE IN SYNC AT %u HAS ZERO onVelocity.\n",
												pL);
							if (aNote->offVelocity==0)
								COMPLAIN("DCheckNode: NOTE IN SYNC AT %u HAS ZERO offVelocity.\n",
												pL);
						}
					}
					
					if (aNote->beamed) {
						if (aNote->subType<=QTR_L_DUR)
							COMPLAIN("*DCheckNode: BEAMED QUARTER NOTE/REST OR LONGER IN SYNC AT %u.\n", pL);
					}
					minLDur = (aNote->rest ? -127 : UNKNOWN_L_DUR);
					if (aNote->subType<minLDur || aNote->subType>MAX_L_DUR)
						COMPLAIN2("*DCheckNode: NOTE/REST WITH ILLEGAL l_dur IN VOICE %d IN SYNC AT %u.\n",
										aNote->voice, pL);
					if ((aNote->inChord && vNotes[aNote->voice]<2)
					||  (!aNote->inChord && vNotes[aNote->voice]>=2))
						COMPLAIN2("*DCheckNode: NOTE/REST IN VOICE %d IN SYNC AT %u HAS BAD inChord FLAG.\n",
										aNote->voice, pL);
					if (aNote->inChord && aNote->rest)
						COMPLAIN2("*DCheckNode: REST IN VOICE %d WITH inCHORD FLAG IN SYNC AT %u.\n",
										aNote->voice, pL);

					DCheckSyncSlurs(pL, aNoteL);
					
					if (aNote->firstMod) {
						unsigned INT16 nObjs = MODNRheap->nObjs;
						aModNRL = aNote->firstMod;
						for ( ; aModNRL; aModNRL=NextMODNRL(aModNRL)) {
							if (aModNRL >= nObjs) {    /* very crude check on node validity  -JGG */
								COMPLAIN3("DCheckNode: GARBAGE MODNR LINK %d IN VOICE %d IN SYNC AT %u.\n",
												aModNRL, aNote->voice, pL);
								break;						/* prevent possible infinite loop */
							}
							aModNR = GetPAMODNR(aModNRL);
							if (!(aModNR->modCode>=MOD_FERMATA && aModNR->modCode<=MOD_LONG_INVMORDENT)
							&&	 !(aModNR->modCode>=0 && aModNR->modCode<=5) )
								COMPLAIN3("DCheckNode: ILLEGAL MODNR CODE %d IN VOICE %d IN SYNC AT %u.\n",
												aModNR->modCode, aNote->voice, pL);
						}
					}
				}
				PopLock(NOTEheap);
			}
				break;

			case SYSTEMtype: {
				PSYSTEM lSystem, rSystem;
					PushLock(SYSTEMheap);
					pSystem = GetPSYSTEM(pL);
					if (pSystem->lSystem) {
						lSystem = GetPSYSTEM(pSystem->lSystem);
						if (lSystem->rSystem!=pL)
							COMPLAIN("¥DCheckNode: SYSTEM AT %u HAS INCONSISTENT LEFT SYSTEM LINK.\n", pL);
					}
					if (pSystem->rSystem) {
						rSystem = GetPSYSTEM(pSystem->rSystem);
						if (rSystem->lSystem!=pL)
							COMPLAIN("¥DCheckNode: SYSTEM AT %u HAS INCONSISTENT RIGHT SYSTEM LINK.\n", pL);
					}
					if (GARBAGE_Q1RECT(pSystem->systemRect))
							COMPLAIN("*DCheckNode: SYSTEM AT %u HAS GARBAGE systemRect.\n", pL);

					if (d2pt(pSystem->systemRect.bottom)>doc->marginRect.bottom)
							COMPLAIN("*DCheckNode: SYSTEM AT %u RECT BELOW BOTTOM MARGIN.\n", pL);
					if (d2pt(pSystem->systemRect.right-ExtraSysWidth(doc))>doc->marginRect.right)
							COMPLAIN("*DCheckNode: SYSTEM AT %u RECT PAST RIGHT MARGIN.\n", pL);

					if (pSystem->lSystem) {
						lSystem = GetPSYSTEM(pSystem->lSystem);				
						if (pSystem->pageL==lSystem->pageL
						&&  pSystem->systemRect.top<lSystem->systemRect.bottom)
							COMPLAIN("*DCheckNode: SYSTEM AT %u RECT NOT BELOW PREVIOUS SYSTEM.\n", pL);
					}
					if (pSystem->rSystem) {
						rSystem = GetPSYSTEM(pSystem->rSystem);				
						if (pSystem->pageL==rSystem->pageL
						&&  pSystem->systemRect.bottom>rSystem->systemRect.top)
							COMPLAIN("*DCheckNode: SYSTEM AT %u RECT NOT ABOVE FOLLOWING SYSTEM.\n", pL);
					}
					PopLock(SYSTEMheap);
				}
				break;
				
			case STAFFtype: {
				PSTAFF	lStaff, rStaff;
				DDIST		sysWidth, sysHeight;
				Boolean	isFirstStaff;
				
					pStaff = GetPSTAFF(pL);
					if (pStaff->lStaff) {
						lStaff = GetPSTAFF(pStaff->lStaff);
						if (lStaff->rStaff != pL)
							COMPLAIN("¥DCheckNode: STAFF AT %u HAS INCONSISTENT STAFF LINK.\n", pL);
					}
					if (pStaff->rStaff) {
						rStaff = GetPSTAFF(pStaff->rStaff);
						if (rStaff->lStaff != pL)
							COMPLAIN("¥DCheckNode: STAFF AT %u HAS INCONSISTENT STAFF LINK.\n", pL);
					}
					isFirstStaff = !pStaff->lStaff;

					pSystem = GetPSYSTEM(pStaff->systemL);
					sysWidth = pSystem->systemRect.right-pSystem->systemRect.left;
					sysHeight = pSystem->systemRect.bottom-pSystem->systemRect.top;

					for (aStaffL=FirstSubLINK(pL); aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
						if (STAFFN_BAD(doc, StaffSTAFF(aStaffL)))
							COMPLAIN2("*DCheckNode: SUBOBJ IN STAFF AT %u HAS BAD staffn=%d.\n",
											pL, StaffSTAFF(aStaffL));
						aStaff = GetPASTAFF(aStaffL);
						if (isFirstStaff) {
							DDIST lnHeight = aStaff->staffHeight/(aStaff->staffLines-1);
							DDIST srLnHeight = drSize[doc->srastral]/(STFLINES-1);
							DDIST altsrLnHeight = drSize[doc->altsrastral]/(STFLINES-1);
#ifdef NOTYET
							if (lnHeight!=srLnHeight && lnHeight!=altsrLnHeight)
								COMPLAIN2("*DCheckNode: SUBOBJ IN FIRST STAFF, AT %u (staffn=%d), staffHeight DISAGREES WITH s/altsrastral.\n",
												pL, aStaff->staffn);
#else
							if (lnHeight!=srLnHeight)
								COMPLAIN2("*DCheckNode: SUBOBJ IN FIRST STAFF, AT %u (staffn=%d), staffHeight DISAGREES WITH srastral.\n",
												pL, aStaff->staffn);
#endif
						}
						if (aStaff->staffLeft<0 || aStaff->staffLeft>sysWidth)
							COMPLAIN("*DCheckNode: SUBOBJ IN STAFF AT %u staffLeft IS ILLEGAL.\n", pL);
						if (aStaff->staffRight<0 || aStaff->staffRight>sysWidth)
							COMPLAIN("*DCheckNode: SUBOBJ IN STAFF AT %u staffRight IS ILLEGAL.\n", pL);

						if (!aStaff->visible) {
							if (aStaff->spaceBelow<=0)
								COMPLAIN2("*DCheckNode: SUBOBJ IN STAFF AT %u (staffn=%d) spaceBelow IS ILLEGAL.\n",
												pL, aStaff->staffn);
							if (aStaff->spaceBelow>sysHeight-aStaff->staffHeight)
								COMPLAIN2("*DCheckNode: SUBOBJ IN STAFF AT %u (staffn=%d) spaceBelow IS SUSPICIOUS.\n",
												pL, aStaff->staffn);
						}
					}
				}
				break;

			case MEASUREtype: {
				PMEASURE lMeasure, rMeasure;
				LINK qL, syncL;
				long timeStamp;
				
					PushLock(MEASUREheap);
					pMeasure = GetPMEASURE(pL);
					if (!abnormal && LinkVALID(pL)) {
					
						/* For Measures, the <valid> flag also applies to the <measureBBox>. */
						if (GARBAGE_Q1RECT(pMeasure->measureBBox)) {
							COMPLAIN("DCheckNode: MEASURE AT %u HAS A GARBAGE BBOX.\n", pL);
						}
						else
							 if (DCheckMBBox(pL, abnormal)) bad = TRUE;
						
						if (pMeasure->lMeasure) {
							lMeasure = GetPMEASURE(pMeasure->lMeasure);
						/* Can't use standard Search here because it assumes links are OK */
							for (qL = LeftLINK(pL); qL; qL = LeftLINK(qL))
								if (ObjLType(qL)==MEASUREtype) break;
							if (!qL)
								{ COMPLAIN("¥DCheckNode: MEASURE AT %u HAS A BAD MEASURE LLINK.\n", pL); }
							else if (qL!=pMeasure->lMeasure)
								{ COMPLAIN("¥DCheckNode: MEASURE AT %u HAS INCONSISTENT MEASURE LLINK.\n", pL); }
							else if (LinkVALID(pL)
								  &&  pMeasure->systemL==lMeasure->systemL
								  &&  pMeasure->measureBBox.left!=lMeasure->measureBBox.right) {
								COMPLAIN2("DCheckNode: MEASURE AT %u BBOX DISAGREES WITH PREVIOUS MEASURE BY %d.\n",
												pL, pMeasure->measureBBox.left-lMeasure->measureBBox.right);
							}
						}
	
						if (pMeasure->rMeasure) {
							rMeasure = GetPMEASURE(pMeasure->rMeasure);
						/* Can't use standard Search here because it assumes links are OK */
							for (qL = RightLINK(pL); qL; qL = RightLINK(qL))
								if (ObjLType(qL)==MEASUREtype) break;
							if (!qL)
								{ COMPLAIN("¥DCheckNode: MEASURE AT %u HAS A BAD MEASURE RLINK.\n", pL); }
							else if (qL!=pMeasure->rMeasure)
								{ COMPLAIN("¥DCheckNode: MEASURE AT %u HAS INCONSISTENT MEASURE RLINK.\n", pL); }
							else if (LinkVALID(pL)
								  &&  pMeasure->systemL==rMeasure->systemL
								  &&  pMeasure->measureBBox.right!=rMeasure->measureBBox.left) {
								COMPLAIN2("DCheckNode: MEASURE AT %u BBOX DISAGREES WITH NEXT MEASURE BY %d.\n",
												pL, pMeasure->measureBBox.right-rMeasure->measureBBox.left);
							}
						}
	
						if (DBadLink(doc, OBJtype, pMeasure->systemL, TRUE)) {
							COMPLAIN("¥DCheckNode: MEASURE AT %u HAS GARBAGE SYSTEM LINK.\n", pL);
						}
						else {
						/* Can't use standard Search here because it assumes links are OK */
							for (qL = LeftLINK(pL); qL; qL = LeftLINK(qL))
								if (ObjLType(qL)==SYSTEMtype) break;
							if (!qL)
								{ COMPLAIN("¥DCheckNode: MEASURE AT %u HAS A BAD SYSTEM LINK.\n", pL); }
							else if (qL!=pMeasure->systemL)
								COMPLAIN("¥DCheckNode: MEASURE AT %u HAS INCONSISTENT SYSTEM LINK.\n", pL);
							}

						if (DBadLink(doc, OBJtype, pMeasure->staffL, TRUE)) {
							COMPLAIN("¥DCheckNode: MEASURE AT %u HAS GARBAGE STAFF LINK.\n", pL);
						}
						else {
						/* Can't use standard Search here because it assumes links are OK */
							for (qL = LeftLINK(pL); qL; qL = LeftLINK(qL))
								if (ObjLType(qL)==STAFFtype) break;
							if (!qL)
								{ COMPLAIN("¥DCheckNode: MEASURE AT %u HAS A BAD STAFF LINK.\n", pL); }
							else if (qL!=pMeasure->staffL)
								COMPLAIN("¥DCheckNode: MEASURE AT %u HAS INCONSISTENT STAFF LINK.\n", pL);
							}
					}
	
					/* Arbitrarily assume no legitimate score is longer than 6000 whole notes. */
					if (pMeasure->lTimeStamp<0L
					||  pMeasure->lTimeStamp>6000L*1920)
							COMPLAIN2("DCheckNode: MEASURE AT %u HAS SUSPICIOUSLY LARGE lTimeStamp %ld.\n",
											pL, pMeasure->lTimeStamp);

					if (pMeasure->spacePercent<MINSPACE
					||  pMeasure->spacePercent>MAXSPACE)
							COMPLAIN2("*DCheckNode: MEASURE AT %u HAS ILLEGAL spacePercent %d.\n",
											pL, pMeasure->spacePercent);
					
					DCheckMeasSubobjs(doc, pL, abnormal);
					
					syncL = SSearch(pL, SYNCtype, GO_RIGHT);
					if (syncL) {
						timeStamp = SyncTIME(syncL);
						if (timeStamp!=0L)
							COMPLAIN2("DCheckNode: SYNC AT %u IS FIRST OF MEASURE BUT ITS timeStamp IS %ld.\n",
											syncL, timeStamp);
					}

					PopLock(MEASUREheap);
				}
				break;

			case PSMEAStype:
				PushLock(PSMEASheap);
					for (aPSMeasL = FirstSubLINK(pL); aPSMeasL;
							aPSMeasL = NextPSMEASL(aPSMeasL)) {
						aPSMeas = GetPAPSMEAS(aPSMeasL);
						if (STAFFN_BAD(doc, aPSMeas->staffn))
							COMPLAIN("*DCheckNode: SUBOBJ IN PSEUDOMEAS AT %u HAS BAD staffn.\n", pL);
						if (aPSMeas->staffn==1 && aPSMeas->connAbove)
							COMPLAIN("*DCheckNode: SUBOBJ ON STAFF 1 IN PSEUDOMEAS AT %u HAS connAbove.\n", pL);
						if (aPSMeas->connStaff>0 && (aPSMeas->connStaff<=aPSMeas->staffn || aPSMeas->connStaff>doc->nstaves))
							COMPLAIN2("*DCheckNode: SUBOBJ ON STAFF %d IN PSEUDOMEAS AT %u HAS BAD connStaff.\n",
											aPSMeas->staffn, pL);
					}
				PopLock(PSMEASheap);
				break;
				
			case CLEFtype:
				for (aClefL=FirstSubLINK(pL); aClefL; aClefL=NextCLEFL(aClefL)) {
					if (STAFFN_BAD(doc, ClefSTAFF(aClefL)))
						COMPLAIN("*DCheckNode: SUBOBJ IN CLEF AT %u HAS BAD staffn.\n", pL);
					if (ClefType(aClefL)<LOW_CLEF || ClefType(aClefL)>HIGH_CLEF)
						COMPLAIN("*DCheckNode: SUBOBJ IN CLEF AT %u HAS BAD subType.\n", pL);
				}
				break;
			
			case KEYSIGtype:
				PushLock(KEYSIGheap);

				for (aKeySigL=FirstSubLINK(pL); aKeySigL; 
						aKeySigL=NextKEYSIGL(aKeySigL)) {
					aKeySig = GetPAKEYSIG(aKeySigL);		
					if (STAFFN_BAD(doc, aKeySig->staffn))
						COMPLAIN("*DCheckNode: SUBOBJ IN KEYSIG AT %u HAS BAD staffn.\n", pL);
					if (aKeySig->nKSItems>7)
						COMPLAIN("*DCheckNode: SUBOBJ IN KEYSIG AT %u HAS SUSPICIOUS nKSItems.\n", pL);
					if (aKeySig->nKSItems==0 && aKeySig->subType>7)
						COMPLAIN("*DCheckNode: SUBOBJ IN KEYSIG AT %u HAS SUSPICIOUS subType (nNatItems).\n", pL);
				}

				PopLock(KEYSIGheap);
				break;
				
			case TIMESIGtype:
				PushLock(TIMESIGheap);

				for (aTimeSigL=FirstSubLINK(pL); aTimeSigL; 
						aTimeSigL=NextTIMESIGL(aTimeSigL)) {
					aTimeSig = GetPATIMESIG(aTimeSigL);		
					if (STAFFN_BAD(doc, aTimeSig->staffn))
						COMPLAIN("*DCheckNode: SUBOBJ IN TIMESIG AT %u HAS BAD staffn.\n", pL);
					if (TSTYPE_BAD(aTimeSig->subType))
						COMPLAIN("*DCheckNode: SUBOBJ IN TIMESIG AT %u HAS BAD timeSigType.\n", pL);
					if (TSDENOM_BAD(aTimeSig->denominator))
						COMPLAIN("*DCheckNode: SUBOBJ IN TIMESIG AT %u HAS BAD TIMESIG denom.\n", pL);
					if (TSDUR_BAD(aTimeSig->numerator, aTimeSig->denominator))
						COMPLAIN("*DCheckNode: SUBOBJ IN TIMESIG AT %u HAS TOO GREAT DURATION.\n", pL);
				}

				PopLock(TIMESIGheap);
				break;

			case BEAMSETtype: {
				INT16 beamsNow;
				
				PushLock(NOTEBEAMheap);

				if (STAFFN_BAD(doc, BeamSTAFF(pL)))
						COMPLAIN("*DCheckNode: BEAMSET AT %u HAS BAD staffn.\n", pL);
				beamsNow = 0;
				for (aNoteBeamL=FirstSubLINK(pL); aNoteBeamL; 
						aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
					aNoteBeam = GetPANOTEBEAM(aNoteBeamL);
					if (DBadLink(doc, OBJtype, aNoteBeam->bpSync, FALSE)) {
						COMPLAIN("¥DCheckNode: BEAMSET AT %u HAS GARBAGE SYNC LINK.\n", pL);
					}
					else if (!((PBEAMSET)p)->grace && ObjLType(aNoteBeam->bpSync)!=SYNCtype) {
						COMPLAIN("¥DCheckNode: BEAMSET AT %u HAS A BAD SYNC LINK.\n", pL);
					}
					else if (((PBEAMSET)p)->grace && ObjLType(aNoteBeam->bpSync)!=GRSYNCtype)
						COMPLAIN("¥DCheckNode: BEAMSET AT %u HAS A BAD GRSYNC LINK.\n", pL);

					beamsNow += aNoteBeam->startend;
					if ((NextNOTEBEAML(aNoteBeamL) && beamsNow<=0)
					||  beamsNow<0)
						COMPLAIN("*DCheckNode: BEAMSET AT %u HAS BAD startend SEQUENCE.\n", pL);
					}
				if (beamsNow!=0)
					COMPLAIN("*DCheckNode: BEAMSET AT %u HAS BAD startend SEQUENCE.\n", pL);
				}

				PopLock(NOTEBEAMheap);
				break;
			
			case TUPLETtype:
				PushLock(NOTETUPLEheap);
				
				if (STAFFN_BAD(doc, ((PTUPLET)p)->staffn))
						COMPLAIN("*DCheckNode: TUPLET AT %u HAS BAD staffn.\n", pL);
				if (VOICE_BAD(((PTUPLET)p)->voice))
						COMPLAIN("*DCheckNode: TUPLET AT %u HAS BAD voice.\n", pL);

				for (aNoteTupleL=FirstSubLINK(pL);	aNoteTupleL;
						aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
					aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
					if (DBadLink(doc, OBJtype, aNoteTuple->tpSync, TRUE)) {
						COMPLAIN2("¥DCheckNode: TUPLET AT %u HAS GARBAGE SYNC LINK %d.\n",
										pL, aNoteTuple->tpSync);
					}
					else if (ObjLType(aNoteTuple->tpSync)!=SYNCtype)
						COMPLAIN2("¥DCheckNode: TUPLET AT %u HAS BAD SYNC LINK %d.\n",
										pL, aNoteTuple->tpSync);
				}

				PopLock(NOTETUPLEheap);
				break;
			
			case OCTAVAtype:
				PushLock(NOTEOCTAVAheap);
				if (STAFFN_BAD(doc, ((POCTAVA)p)->staffn))
					COMPLAIN("*DCheckNode: OCTAVA AT %u HAS BAD staffn.\n", pL);

				for (aNoteOctL=FirstSubLINK(pL);	aNoteOctL;
						aNoteOctL=NextNOTEOCTAVAL(aNoteOctL)) {
					aNoteOct = GetPANOTEOCTAVA(aNoteOctL);
					if (DBadLink(doc, OBJtype, aNoteOct->opSync, TRUE)) {
						COMPLAIN2("¥DCheckNode: OCTAVA AT %u HAS GARBAGE SYNC LINK %d.\n",
										pL, aNoteOct->opSync);
					}
					else if (!SyncTYPE(aNoteOct->opSync) && !GRSyncTYPE(aNoteOct->opSync))
						COMPLAIN2("¥DCheckNode: OCTAVA AT %u HAS BAD SYNC LINK %d.\n",
										pL, aNoteOct->opSync);
				}
				
				PopLock(NOTEOCTAVAheap);
				break;

			case CONNECTtype:
				PushLock(CONNECTheap);

				for (aConnectL=FirstSubLINK(pL); aConnectL;
						aConnectL=NextCONNECTL(aConnectL)) {
					aConnect = GetPACONNECT(aConnectL);
					if (aConnect->connLevel!=0)
						if (STAFFN_BAD(doc, aConnect->staffAbove)
						||  STAFFN_BAD(doc, aConnect->staffBelow)) {
							COMPLAIN3("*DCheckNode: SUBOBJ IN CONNECT AT %u HAS BAD STAFF %d OR %d.\n",
											pL, aConnect->staffAbove, aConnect->staffBelow);
						}
						else if (aConnect->staffAbove>=aConnect->staffBelow)
							COMPLAIN("*DCheckNode: SUBOBJ IN CONNECT AT %u staffAbove>=staffBelow.\n", pL);
				}

				PopLock(CONNECTheap);
				break;

			case DYNAMtype:
				PushLock(DYNAMheap);
				pDynamic = GetPDYNAMIC(pL);
				if (DBadLink(doc, OBJtype, pDynamic->firstSyncL, FALSE)) {
					COMPLAIN("¥DCheckNode: DYNAMIC AT %u HAS GARBAGE firstSyncL.\n", pL);
				}
				else {
					if (ObjLType(pDynamic->firstSyncL)!=SYNCtype) {
						COMPLAIN("¥DCheckNode: DYNAMIC AT %u firstSyncL IS NOT A SYNC.\n", pL);
					}
				}

				if (DBadLink(doc, OBJtype, pDynamic->lastSyncL, !IsHairpin(pL))) {
					COMPLAIN("¥DCheckNode: DYNAMIC AT %u HAS GARBAGE lastSyncL.\n", pL);
				}

				for (aDynamicL=FirstSubLINK(pL); aDynamicL;
						aDynamicL = NextDYNAMICL(aDynamicL)) {
					if (STAFFN_BAD(doc, DynamicSTAFF(aDynamicL))) {
						COMPLAIN("*DCheckNode: SUBOBJ IN DYNAMIC AT %u HAS BAD staffn.\n", pL);
					}
					else {
						if (!ObjOnStaff(pDynamic->firstSyncL, DynamicSTAFF(aDynamicL), FALSE))
							COMPLAIN("*DCheckNode: DYNAMIC AT %u firstSyncL HAS NO NOTES ON ITS STAFF.\n", pL);
						if (IsHairpin(pL) && !ObjOnStaff(pDynamic->lastSyncL, DynamicSTAFF(aDynamicL), FALSE))
							COMPLAIN("*DCheckNode: DYNAMIC AT %u lastSyncL HAS NO NOTES ON ITS STAFF.\n", pL);
					}
				}
				PopLock(DYNAMheap);
				break;
				
			case SLURtype: {
				INT16 theInd, firstNoteNum, lastNoteNum;
				PANOTE 	firstNote, lastNote;
				LINK		firstNoteL, lastNoteL;
				Boolean	foundFirst, foundLast;
				
				PushLock(SLURheap);
				pSlur = GetPSLUR(pL);

				if (STAFFN_BAD(doc, pSlur->staffn))
					COMPLAIN("*DCheckNode: SLUR AT %u HAS BAD staffn.\n", pL);
				if (VOICE_BAD(pSlur->voice))
					COMPLAIN("*DCheckNode: SLUR AT %u HAS BAD voice.\n", pL);
				if (!pSlur->tie && p->nEntries>1)
					COMPLAIN("*DCheckNode: NON-TIE SLUR AT %u WITH MORE THAN ONE SUBOBJECT.\n", pL);
				if (DBadLink(doc, OBJtype, pSlur->firstSyncL, TRUE)
				||  DBadLink(doc, OBJtype, pSlur->lastSyncL, TRUE)) {
					COMPLAIN("¥DCheckNode: SLUR AT %u HAS GARBAGE SYNC LINK.\n", pL);
					break;
				}

				if (!pSlur->crossSystem) {
					if (ObjLType(pSlur->firstSyncL)!=SYNCtype
					||  ObjLType(pSlur->lastSyncL)!=SYNCtype) {
						COMPLAIN("*DCheckNode: NON-CROSS-SYS SLUR AT %u firstSync OR lastSync NOT A SYNC.\n", pL);
						break;
					}
				}
				else {
					if (ObjLType(pSlur->firstSyncL)!=MEASUREtype
					&&  ObjLType(pSlur->lastSyncL)!=SYSTEMtype) {
						COMPLAIN("*DCheckNode: CROSS-SYS SLUR AT %u firstSync NOT A MEASURE AND lastSync NOT A SYSTEM.\n", pL);
						break;
					}
				}
				
				if (ObjLType(pSlur->firstSyncL)==SYNCtype
				&&  !SyncInVoice(pSlur->firstSyncL, pSlur->voice)) {
					COMPLAIN("*DCheckNode: SLUR AT %u firstSync HAS NOTHING IN SLUR'S VOICE.\n", pL);
					break;
				}

				if (ObjLType(pSlur->lastSyncL)==SYNCtype
				&&  !SyncInVoice(pSlur->lastSyncL, pSlur->voice)) {
					COMPLAIN("*DCheckNode: SLUR AT %u lastSync HAS NOTHING IN SLUR'S VOICE.\n", pL);
					break;
				}

				for (aSlurL=FirstSubLINK(pL); aSlurL; aSlurL=NextSLURL(aSlurL)) {
					aSlur = GetPASLUR(aSlurL);
					if (pSlur->tie) {
						/* firstInd and lastInd are used only for ties. */
						
						if (aSlur->firstInd<0 || aSlur->firstInd>=MAXCHORD
						||  aSlur->lastInd<0  || aSlur->lastInd>=MAXCHORD)
							COMPLAIN("*DCheckNode: TIE AT %u HAS ILLEGAL NOTE INDEX.\n", pL);

						if (!pSlur->crossSystem) {
							theInd = -1;
							for (firstNoteL = FirstSubLINK(pSlur->firstSyncL); firstNoteL;
									firstNoteL = NextNOTEL(firstNoteL)) {
								firstNote = GetPANOTE(firstNoteL);
								if (firstNote->voice==pSlur->voice) {
									theInd++;
									if (theInd==aSlur->firstInd) {
										firstNoteNum = firstNote->noteNum;
										break;
									}
								}
							}
							theInd = -1;
							for (lastNoteL = FirstSubLINK(pSlur->lastSyncL);lastNoteL;
									lastNoteL = NextNOTEL(lastNoteL)) {
								lastNote = GetPANOTE(lastNoteL);
								if (lastNote->voice==pSlur->voice) {
									theInd++;
									if (theInd==aSlur->lastInd) {
										lastNoteNum = lastNote->noteNum;
										break;
									}
								}
							}
							if (!firstNote->tiedR)
								COMPLAIN2("*DCheckNode: TIE IN VOICE %d AT %u FIRST NOTE NOT tiedR.\n",
											SlurVOICE(pL), pL);
							if (!lastNote->tiedL)
								COMPLAIN2("*DCheckNode: TIE IN VOICE %d AT %u LAST NOTE NOT tiedL.\n",
											SlurVOICE(pL), pL);
							if (firstNoteNum!=lastNoteNum)
								COMPLAIN2("*DCheckNode: TIE IN VOICE %d AT %u ON NOTES WITH DIFFERENT noteNum.\n",
											SlurVOICE(pL), pL);
						}
					}
						
					else {													/* Slur, not set of ties */
						if (!pSlur->crossSystem) {
							foundFirst = FALSE;
							for (firstNoteL = FirstSubLINK(pSlur->firstSyncL); firstNoteL;
									firstNoteL = NextNOTEL(firstNoteL)) {
								firstNote = GetPANOTE(firstNoteL);
								if (firstNote->voice==pSlur->voice) {
									if (firstNote->slurredR) foundFirst = TRUE;
								}
							}
							foundLast = FALSE;
							for (lastNoteL = FirstSubLINK(pSlur->lastSyncL); lastNoteL;
									lastNoteL = NextNOTEL(lastNoteL)) {
								lastNote = GetPANOTE(lastNoteL);
								if (lastNote->voice==pSlur->voice) {
									if (lastNote->slurredL) foundLast = TRUE;
								}
							}
							if (!foundFirst)
									COMPLAIN2("*DCheckNode: SLUR IN VOICE %d AT %u FIRST NOTE NOT slurredR.\n",
												SlurVOICE(pL), pL);
							if (!foundLast)
									COMPLAIN2("*DCheckNode: SLUR IN VOICE %d AT %u LAST NOTE NOT slurredL.\n",
												SlurVOICE(pL), pL);
						}
					}
				}

				PopLock(SLURheap);
				}
				break;
			
			case GRAPHICtype:
				{
					LINK aGraphicL;		PAGRAPHIC aGraphic;
					PGRAPHIC pGraphic;	INT16 len;

					PushLock(GRAPHICheap);
	 				pGraphic = GetPGRAPHIC(pL);
					if (DBadLink(doc, OBJtype, pGraphic->firstObj, FALSE))
						COMPLAIN("¥DCheckNode: GRAPHIC AT %u HAS GARBAGE firstObj.\n", pL);
					if (!PageTYPE(pGraphic->firstObj))
						if (STAFFN_BAD(doc, pGraphic->staffn)) {
							COMPLAIN2("*DCheckNode: GRAPHIC AT %u HAS BAD staffn %d.\n", pL, pGraphic->staffn);
						}
					else {
						if (!ObjOnStaff(pGraphic->firstObj, pGraphic->staffn, FALSE))
							COMPLAIN2("*DCheckNode: GRAPHIC AT %u firstObj HAS NO SUBOBJS ON ITS STAFF %d.\n",
											pL, pGraphic->staffn);
					}
 
					switch (pGraphic->graphicType) {
						case GRString:
						case GRLyric:
						case GRRehearsal:
						case GRChordSym:
							if (doc!=clipboard && pGraphic->fontInd>=doc->nfontsUsed)
								COMPLAIN2("*DCheckNode: STRING GRAPHIC AT %u HAS BAD FONT NUMBER %d.\n",
														pL, pGraphic->fontInd);
							if (pGraphic->relFSize &&
								(pGraphic->fontSize<GRTiny || pGraphic->fontSize>GRLastSize))
									COMPLAIN2("*DCheckNode: STRING GRAPHIC AT %u HAS BAD RELATIVE SIZE %d.\n",
														pL, pGraphic->fontSize);
							aGraphicL = FirstSubLINK(pL);
							aGraphic = GetPAGRAPHIC(aGraphicL);
							if (aGraphic->string<0L
							|| aGraphic->string>=GetHandleSize((Handle)doc->stringPool)) {
								COMPLAIN("*DCheckNode: STRING GRAPHIC AT %u string IS BAD.\n", pL);
							}
							else if (!aGraphic->string || PCopy(aGraphic->string)==NULL) {
								COMPLAIN("*DCheckNode: STRING GRAPHIC AT %u HAS NO STRING.\n", pL);
							}
							else {
								len = (Byte)(*PCopy(aGraphic->string));
								if (len==0) COMPLAIN("*DCheckNode: STRING GRAPHIC AT %u HAS AN EMPTY STRING.\n",
															pL);
							}
							break;
						case GRDraw:
							if (DBadLink(doc, OBJtype, pGraphic->lastObj, FALSE))
								COMPLAIN("¥DCheckNode: GRDraw GRAPHIC AT %u HAS GARBAGE lastObj.\n", pL);
							if (!ObjOnStaff(pGraphic->lastObj, pGraphic->staffn, FALSE))
								COMPLAIN2("*DCheckNode: GRDraw GRAPHIC AT %u lastObj HAS NO SUBOBJS ON ITS STAFF %d.\n",
												pL, pGraphic->staffn);
							break;
						default:
							;
					}
					PopLock(GRAPHICheap);
				}
				break;
				
			case TEMPOtype:
				{
					PTEMPO pTempo; INT16 len;
					
					PushLock(TEMPOheap);
					if (DBadLink(doc, OBJtype, ((PTEMPO)p)->firstObjL, FALSE))
						COMPLAIN("¥DCheckNode: TEMPO AT %u HAS GARBAGE firstObjL.\n", pL);
						
	 				pTempo = GetPTEMPO(pL);
					if (!PageTYPE(pTempo->firstObjL))
						if (STAFFN_BAD(doc, pTempo->staffn)) {
							COMPLAIN("*DCheckNode: TEMPO AT %u HAS BAD staffn.\n", pL);
						}
					else {
					if (!ObjOnStaff(pTempo->firstObjL, pTempo->staffn, FALSE))
						COMPLAIN("*DCheckNode: TEMPO AT %u firstObjL HAS NO SUBOBJS ON ITS STAFF.\n",
										pL);
					}
	
					/* A Tempo's <string> can be empty, but its <metroStr> can't be. */
					 
					if (pTempo->string<0L
					|| pTempo->string>=GetHandleSize((Handle)doc->stringPool)) {
						COMPLAIN("*DCheckNode: TEMPO AT %u string IS BAD.\n", pL);
					}

					if (pTempo->metroStr<0L
					|| pTempo->metroStr>=GetHandleSize((Handle)doc->stringPool)) {
						COMPLAIN("*DCheckNode: TEMPO AT %u metroStr IS BAD.\n", pL);
					}
					else if (!pTempo->metroStr || PCopy(pTempo->metroStr)==NULL) {
						COMPLAIN("*DCheckNode: TEMPO AT %u HAS NO metroStr.\n", pL);
					}
					else {
						len = (Byte)(*PCopy(pTempo->metroStr));
						if (len==0) COMPLAIN("*DCheckNode: TEMPO AT %u HAS AN EMPTY metroStr.\n",
													pL);
					}
					PopLock(TEMPOheap);
				}
				break;


			case ENDINGtype:
				{
					PENDING pEnding;
					
					PushLock(ENDINGheap);
					if (STAFFN_BAD(doc, EndingSTAFF(pL))) {
						COMPLAIN("*DCheckNode: SUBOBJ IN ENDING AT %u HAS BAD staffn.\n", pL);
						break;
					}
					
					pEnding = GetPENDING(pL);
					if (DBadLink(doc, OBJtype, pEnding->firstObjL, FALSE)) {
						COMPLAIN("¥DCheckNode: ENDING AT %u HAS GARBAGE firstObjL.\n", pL);
					}
					else if (!ObjOnStaff(pEnding->firstObjL, EndingSTAFF(pL), FALSE))
						COMPLAIN("*DCheckNode: ENDING AT %u firstObjL HAS NO SUBOBJS ON ITS STAFF.\n",
										pL);

					if (DBadLink(doc, OBJtype, pEnding->lastObjL, FALSE)) {
						COMPLAIN("¥DCheckNode: ENDING AT %u HAS GARBAGE lastObjL.\n", pL);
					}
					else if (!ObjOnStaff(pEnding->lastObjL, EndingSTAFF(pL), FALSE))
						COMPLAIN("*DCheckNode: ENDING AT %u lastObjL HAS NO SUBOBJS ON ITS STAFF.\n",
										pL);

					if (pEnding->endNum>MAX_ENDING_STRINGS) {
						COMPLAIN2("*DCheckNode: ENDING AT %u HAS BAD endNum %d.\n", pL,
										pEnding->endNum);
					}


					PopLock(ENDINGheap);
				}
				break;



			default:
				;
		}
	}

	PopLock(OBJheap);
	return bad;			
}


/* ------------------------------------------------------------------ DCheckNodeSel -- */
/* Do consistency check on selection status between object and subobject:
if object is not selected, no subobjects should be selected. Does not check
the other type of consistency--if object is selected, at least one subobject
should be. Returns TRUE if it finds a problem. */

Boolean DCheckNodeSel(Document *doc, LINK pL)
{
	LINK			aNoteL, aMeasL, aClefL, aKeySigL, aTimeSigL, aDynamicL,
					aStaffL, aConnectL, aSlurL;
	Boolean		bad;

	if (pL==doc->headL || pL==doc->tailL) return FALSE;
	if (pL==doc->masterHeadL || pL==doc->masterTailL) return FALSE;
	if (LinkSEL(pL)) return FALSE;
	
	bad = FALSE;

	if (TYPE_BAD(pL) || ObjLType(pL)==HEADERtype || ObjLType(pL)==TAILtype) {
			COMPLAIN("¥DCheckNodeSel: NODE AT %u HAS BAD type.\n", pL);
			return bad;
	}

	switch (ObjLType(pL)) {
		case SYNCtype:
			for (aNoteL = FirstSubLINK(pL);aNoteL;aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL))
					COMPLAIN2("DCheckNodeSel: SELECTED NOTE IN VOICE %d IN UNSELECTED SYNC AT %u.\n",
									NoteVOICE(aNoteL), pL);
			break;
		case MEASUREtype:
			aMeasL = FirstSubLINK(pL);
			for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL))
				if (MeasureSEL(aMeasL))
					COMPLAIN2("DCheckNodeSel: SELECTED ITEM ON STAFF %d IN UNSELECTED MEASURE AT %u.\n",
									MeasureSTAFF(aMeasL), pL);
			break;
		case CLEFtype:
			for (aClefL = FirstSubLINK(pL);aClefL; aClefL = NextCLEFL(aClefL))
				if (ClefSEL(aClefL))
					COMPLAIN2("DCheckNodeSel: SELECTED ITEM ON STAFF %d IN UNSELECTED CLEF AT %u.\n",
									ClefSTAFF(aClefL), pL);
			break;
		case KEYSIGtype:
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
				if (KeySigSEL(aKeySigL))
					COMPLAIN2("DCheckNodeSel: SELECTED ITEM ON STAFF %d IN UNSELECTED KEYSIG AT %u.\n",
									KeySigSTAFF(aKeySigL), pL);
			break;
		case TIMESIGtype:
			aTimeSigL = FirstSubLINK(pL);
			for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
				if (TimeSigSEL(aTimeSigL))
					COMPLAIN2("DCheckNodeSel: SELECTED ITEM ON STAFF %d IN UNSELECTED TIMESIG AT %u.\n",
									TimeSigSTAFF(aTimeSigL), pL);
			break;
		case DYNAMtype:
			aDynamicL = FirstSubLINK(pL);
			for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL))
				if (DynamicSEL(aDynamicL))
					COMPLAIN("DCheckNodeSel: SELECTED ITEM IN UNSELECTED DYNAMIC AT %u.\n", pL);
			break;
		case STAFFtype:
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
				if (StaffSEL(aStaffL))
					COMPLAIN("DCheckNodeSel: SELECTED ITEM IN UNSELECTED STAFF AT %u.\n", pL);
			break;
		case CONNECTtype:
			aConnectL = FirstSubLINK(pL);
			for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL))
				if (ConnectSEL(aConnectL))
					COMPLAIN("DCheckNodeSel: SELECTED ITEM IN UNSELECTED CONNECT AT %u.\n", pL);
			break;
		case SLURtype:
			aSlurL = FirstSubLINK(pL);
			for ( ; aSlurL; aSlurL = NextSLURL(aSlurL))
				if (SlurSEL(aSlurL))
					COMPLAIN("DCheckNodeSel: SELECTED ITEM IN UNSELECTED SLUR AT %u.\n", pL);
			break;
/* ??REPEATEND ALSO NEEDS CHECKING */
		default:
			;
	}
	if (bad) return TRUE;
	else	   return FALSE;		
}


/* ---------------------------------------------------------------------- DCheckSel -- */
/* Do consistency checks on the selection: check selection start/end links,
that no node outside the range they describe has its selected flag set, etc.
Returns TRUE if the selection start or end link is garbage or not even
in the data structure. */
 
Boolean DCheckSel(Document *doc, INT16 *pnInRange, INT16 *pnSelFlag)
{
	LINK pL;
	Boolean bad=FALSE;

	if (DBadLink(doc, OBJtype, doc->selStartL, FALSE)
	||  DBadLink(doc, OBJtype, doc->selEndL, FALSE)) {
		COMPLAIN2("¥DCheckSel: selStartL=%d OR selEndL=%d IS A GARBAGE LINK.\n",
						doc->selStartL, doc->selEndL);
		return TRUE;
	}

	CountSelection(doc, pnInRange, pnSelFlag);

	if (!doc->masterView) {
		if (!InDataStruct(doc, doc->selStartL, MAIN_DSTR))
			COMPLAIN("¥DCheckSel: selStartL=%d NOT IN MAIN DATA STRUCTURE.\n",
						doc->selStartL);
	
		if (!InDataStruct(doc, doc->selEndL, MAIN_DSTR)) {
			COMPLAIN("¥DCheckSel: selEndL=%d NOT IN MAIN DATA STRUCTURE.\n",
						doc->selEndL);
			return TRUE;
		}
		
		if (*pnInRange>0 && *pnSelFlag==0)
			COMPLAIN("DCheckSel: %d NODES IN SELECTION RANGE BUT NONE SELECTED.\n",
						*pnInRange);
	
		for (pL = doc->headL; pL!=doc->selStartL; pL = RightLINK(pL)) {
			if (DErrLimit()) break;
			if (LinkSEL(pL))
				COMPLAIN("DCheckSel: NODE BEFORE SELECTION RANGE (AT %u) SELECTED.\n", pL);
			if (DBadLink(doc, OBJtype, RightLINK(pL), TRUE) && doc->selStartL) {
				COMPLAIN("¥DCheckSel: GARBAGE RightLINK(%d) BEFORE SELECTION RANGE.\n", pL);
				break;
			}
		}
		
			for (pL = doc->selEndL; pL!=doc->tailL; pL = RightLINK(pL)) {
				if (DErrLimit()) break;
				if (LinkSEL(pL))
					COMPLAIN("DCheckSel: NODE AFTER SELECTION RANGE (AT %u) SELECTED.\n", pL);
			}
	}
	
	if (doc->selStartL!=doc->selEndL)
		if (!LinkSEL(doc->selStartL) || !LinkSEL(LeftLINK(doc->selEndL)))
			COMPLAIN("DCheckSel: SELECTION RANGE IS NOT OPTIMIZED.\n", pL);
	
	return FALSE;			
}

/* ---------------------------------------------------------------- DCheckHeirarchy -- */
/* Check:
	(1) that the numbers of PAGEs and of SYSTEMs are what the header says they are;
	(2) that nodes appear on each page in the order (ignoring GRAPHICs and maybe
			others) PAGE, SYSTEM, STAFF, everything else;
	(3) that every staff number that should, and none that shouldn't, appears
		somewhere in the data structure;
	(4) that every STAFF has at least one MEASURE, and that nothing appears between
		the STAFF and its first MEASURE except Clefs, KeySigs, TimeSigs, and Connects;
	(5) that Clef, KeySig, and TimeSig inMeasure flags agree with their position in
		the object list;
	(6) that at least one each Clef, KeySig, and TimeSig precede every other "content"
		object. */

Boolean DCheckHeirarchy(Document *doc)
{
	LINK 		pL, aStaffL, pageL, systemL;
	INT16		nMissing, i,
				nsystems, numSheets;
	Boolean	aStaffFound[MAXSTAVES+1],				/* Found the individual staff? */
				foundPage, foundSystem, foundStaff,	/* Found any PAGE, SYSTEM, STAFF obj yet? */
				foundMeasure,								/* Found a MEASURE since the last STAFF? */
				foundClef, foundKeySig, foundTimeSig;	/* Found any CLEF, KEYSIG, TIMESIG yet? */
	Boolean	bad=FALSE;
	PCLEF		pClef; PKEYSIG pKeySig; PTIMESIG pTimeSig;
				
	foundPage = foundSystem = foundStaff = FALSE;
	for (i = 1; i<=doc->nstaves; i++)
		aStaffFound[i] = FALSE;
	nsystems = numSheets = 0;
		
	foundClef = foundKeySig = foundTimeSig = FALSE;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		switch (ObjLType(pL)) {
			case SYNCtype:
				if (!foundPage || !foundSystem || !foundStaff)
					COMPLAIN("¥DCheckHeirarchy: SYNC AT %u PRECEDES PAGE, SYSTEM OR STAFF.\n", pL);
				if (!foundClef || !foundKeySig || !foundTimeSig)
					COMPLAIN("¥DCheckHeirarchy: NODE AT %u PRECEDES CLEF, KEYSIG, OR TIMESIG.\n", pL);
				break;
			case PAGEtype:
				foundPage = TRUE;
				foundSystem = foundStaff = FALSE;
				pageL = pL;
				numSheets++;
				break;
			case SYSTEMtype:
				foundSystem = TRUE;
				systemL = pL;
				nsystems++;
				if (SysPAGE(pL)!=pageL)
					COMPLAIN("¥DCheckHeirarchy: SYSTEM AT %u HAS WRONG pageL.\n", pL);
				break;
			case STAFFtype:
				if (!foundPage || !foundSystem)
					COMPLAIN("¥DCheckHeirarchy: STAFF AT %u PRECEDES PAGE OR SYSTEM.\n", pL);
				foundStaff = TRUE;
				for (aStaffL=FirstSubLINK(pL); aStaffL; 
							aStaffL=NextSTAFFL(aStaffL)) {
					if (STAFFN_BAD(doc, StaffSTAFF(aStaffL)))
						COMPLAIN("*DCheckHeirarchy: STAFF AT %u HAS BAD staffn.\n", pL)
					else
						aStaffFound[StaffSTAFF(aStaffL)] = TRUE;
				}
				if (StaffSYS(pL)!=systemL)
					COMPLAIN("¥DCheckHeirarchy: STAFF AT %u HAS WRONG systemL.\n", pL);
				break;
			case MEASUREtype:
				if (MeasSYSL(pL)!=systemL)
					COMPLAIN("¥DCheckHeirarchy: MEASURE AT %u HAS WRONG systemL.\n", pL);
				goto ChkAll;
			case CLEFtype:
				foundClef = TRUE;
				goto ChkPageSysStaff;
			case KEYSIGtype:
				foundKeySig = TRUE;
				goto ChkPageSysStaff;
			case TIMESIGtype:
				foundTimeSig = TRUE;
				goto ChkPageSysStaff;
			case BEAMSETtype:
			case DYNAMtype:
			case OCTAVAtype:
	ChkAll:
				if (!foundClef || !foundKeySig || !foundTimeSig)
					COMPLAIN("¥DCheckHeirarchy: NODE AT %u PRECEDES CLEF, KEYSIG, OR TIMESIG.\n", pL);
	ChkPageSysStaff:
				if (!foundPage || !foundSystem || !foundStaff)
					COMPLAIN("¥DCheckHeirarchy: NODE AT %u PRECEDES PAGE, SYSTEM OR STAFF.\n", pL);
				break;
			case CONNECTtype:
				if (!foundPage || !foundSystem)
					COMPLAIN("¥DCheckHeirarchy: CONNECT AT %u PRECEDES PAGE OR SYSTEM.\n", pL);
				break;
			case GRAPHICtype:
				break;
			default:
				break;
		}
	}
	
	if (numSheets!=doc->numSheets) COMPLAIN2("*DCheckHeirarchy: FOUND %d PAGES BUT HEADER HAS numSheets=%d.\n",
			numSheets, doc->numSheets);
	if (nsystems!=doc->nsystems) COMPLAIN2("*DCheckHeirarchy: FOUND %d SYSTEMS BUT HEADER HAS nsystems=%d.\n",
			nsystems, doc->nsystems);

	for (nMissing = 0, i = 1; i<=doc->nstaves; i++)
		if (!aStaffFound[i]) nMissing++;
	if (nMissing!=0)	
		COMPLAIN("¥DCheckHeirarchy: %ld STAVES NOT FOUND.\n", (long)nMissing);

	foundMeasure = foundSystem = TRUE;
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
	switch (ObjLType(pL)) {
		case PAGEtype:
			if (!foundSystem) {
				COMPLAIN("¥DCheckHeirarchy: PAGE AT %u CONTAINS NO SYSTEM.\n", LinkLPAGE(pL));
			}
			foundSystem = FALSE;
			break;
		case SYSTEMtype:
			if (!foundMeasure) {
				COMPLAIN("¥DCheckHeirarchy: SYSTEM AT %u CONTAINS NO MEASURE.\n", LinkLSYS(pL));
			}
			foundMeasure = FALSE;
			foundSystem = TRUE;
			break;
		case MEASUREtype:
			foundMeasure = TRUE;
		case STAFFtype:							/* These types can be outside of Measures */
		case CONNECTtype:
		case GRAPHICtype:
		case TEMPOtype:
			break;
		case CLEFtype:
			pClef = GetPCLEF(pL);
			if (pClef->inMeasure!=foundMeasure)
				COMPLAIN("*DCheckHeirarchy: CLEF AT %u inMeasure FLAG DISAGREES WITH OBJECT ORDER.\n",
								pL);				
			break;
		case KEYSIGtype:
			pKeySig = GetPKEYSIG(pL);
			if (pKeySig->inMeasure!=foundMeasure)
				COMPLAIN("*DCheckHeirarchy: KEYSIG AT %u inMeasure FLAG DISAGREES WITH OBJECT ORDER.\n",
								pL);				
			break;
		case TIMESIGtype:
			pTimeSig = GetPTIMESIG(pL);
			if (pTimeSig->inMeasure!=foundMeasure)
				COMPLAIN("*DCheckHeirarchy: TIMESIG AT %u inMeasure FLAG DISAGREES WITH OBJECT ORDER.\n",
								pL);				
			break;
#if 1
		case ENDINGtype:
			if (!CapsLockKeyDown()) break;
#endif
		default:
			if (!foundMeasure)
				COMPLAIN("¥DCheckHeirarchy: OBJECT AT %u PRECEDES ITS STAFF'S 1ST MEASURE.\n", pL);
			break;
	}
	if (!foundMeasure) {															/* Any Measures in the last System? */
		pL = LSSearch(doc->tailL, SYSTEMtype, ANYONE, TRUE, FALSE);	/* No */
		COMPLAIN("¥DCheckHeirarchy: SYSTEM AT %u CONTAINS NO MEASURE.\n", pL);
	}		

	return bad;			
}


/* ------------------------------------------------------------------ DCheckJDOrder -- */
/* Should eventually check that every JD object is in the "slot" preceding its
relObj or firstObj. For now, checks only Graphics. */

Boolean DCheckJDOrder(Document *doc)
{
	LINK pL, qL, attL;
	Boolean bad=FALSE;
				
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
	switch (ObjLType(pL)) {
		case GRAPHICtype:
			attL = GraphicFIRSTOBJ(pL);
			if (PageTYPE(attL)) break;
			for (qL = RightLINK(pL); qL!=attL; qL = RightLINK(qL))
				if (!J_DTYPE(qL)) {
					if (!InDataStruct(doc, attL, MAIN_DSTR)) {
						COMPLAIN3("¥DCheckJDOrder: GRAPHIC ON STAFF %d AT %u firstObj=%d NOT IN MAIN DATA STRUCTURE.\n",
									GraphicSTAFF(pL), pL, attL);
					}
					else {
						COMPLAIN3("*DCheckJDOrder: GRAPHIC ON STAFF %d AT %u IN WRONG ORDER RELATIVE TO firstObj=%d.\n",
										GraphicSTAFF(pL), pL, attL);
					}
					break;
				}
			break;
		default:
			;
	}
	
	return bad;
}


/* -------------------------------------------------------------------- DCheckBeams -- */
/* Check consistency of Beamset objects with notes/rests or grace notes they should
be referring to. Also check that, after a non-grace Beamset that's the first of a
cross-system pair, the next non-grace Beamset in that voice is the second of a cross-
system pair. Notes/rests are checked against their Beamset more carefully than
grace notes are. */
 
Boolean DCheckBeams(Document *doc)
{
	PANOTE				aNote, aGRNote;
	LINK					pL, aNoteL, aGRNoteL;
	LINK					syncL, measureL, noteBeamL, qL;
	INT16					staff, voice, v, n, nEntries;
	LINK					beamSetL[MAXVOICES+1], grBeamSetL[MAXVOICES+1];
	Boolean				expect2ndPiece[MAXVOICES+1], beamNotesOkay;
	PBEAMSET				pBS;
	PANOTEBEAM			pNoteBeam;
	SearchParam 		pbSearch;
	Boolean				foundRest, grace, bad;

	bad = FALSE;
	
	for (v = 0; v<=MAXVOICES; v++) {
		beamSetL[v] = NILINK;
		expect2ndPiece[v] = FALSE;
	}
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;
		
		switch (ObjLType(pL)) {

		 case BEAMSETtype:
		 	pBS = GetPBEAMSET(pL);
		 	grace = pBS->grace;
		 	
			nEntries = LinkNENTRIES(pL);
			staff = BeamSTAFF(pL);
			voice = BeamVOICE(pL);
			if (VOICE_BAD(voice))
				COMPLAIN2("*DCheckBeams: BEAMSET AT %u HAS BAD voice %d.\n", pL, voice)
			else if (grace)
				grBeamSetL[voice] = pL;
			else
				beamSetL[voice] = pL;

		 	pBS = GetPBEAMSET(pL);
			measureL = LSSearch(pL, MEASUREtype, staff,	GO_RIGHT, 	/* Is 1st note in same */
											FALSE);									/*   meas. as BEAMSET? */
			if (measureL) {
				pNoteBeam = GetPANOTEBEAM(pBS->firstSubObj);
				if (IsAfter(measureL, pNoteBeam->bpSync))
					COMPLAIN("*DCheckBeams: BEAMSET AT %u IN DIFFERENT MEASURE FROM ITS 1ST SYNC.\n", pL);
			}
			
			foundRest = FALSE;
			InitSearchParam(&pbSearch);
			pbSearch.id = ANYONE;														/* Prepare for search */
			pbSearch.voice = voice;
			pbSearch.needSelected = pbSearch.inSystem = FALSE;
			noteBeamL = pBS->firstSubObj;
			syncL = pL;
			beamNotesOkay = TRUE;
			for (n=1; noteBeamL; n++, noteBeamL=NextNOTEBEAML(noteBeamL))	{	/* For each SYNC with a note in BEAMSET... */
Next:
					syncL = L_Search(RightLINK(syncL), (grace? GRSYNCtype : SYNCtype),
											GO_RIGHT, &pbSearch);
					if (DBadLink(doc, OBJtype, syncL, TRUE)) {
						COMPLAIN2("*DCheckBeams: BEAMSET %d: TROUBLE FINDING %sSYNCS.\n", pL,
										(grace? "GR" : ""));
						beamNotesOkay = FALSE;
						break;
					}
					if (!SameSystem(pL, syncL)) {
						COMPLAIN3("*DCheckBeams: BEAMSET %d: %sSYNC %d NOT IN SAME SYSTEM.\n", pL,
										(grace? "GR" : ""), syncL);
						beamNotesOkay = FALSE;
						break;
					}
					aNoteL = pbSearch.pEntry;
					if (!grace && NoteREST(aNoteL)) foundRest = TRUE;
					
					/* If this is a rest that could not be in the Beamset, keep looking. */
					
					if (!grace && NoteREST(aNoteL)
					&& 	(!doc->beamRests && n>1 && n<nEntries) ) goto Next;
					pNoteBeam = GetPANOTEBEAM(noteBeamL);
					/*
					 * If <foundRest>, a disagreement between the Sync we just found and the one the
					 *	Beamset refers to may simply be due to the Beamset having been created with
					 *	<beamRests> set the other way from its current setting, and not because of
			 		 *	any error.
					 */
					if (pNoteBeam->bpSync!=syncL) {
						COMPLAIN2("(*)DCheckBeams: BEAMSET %d SYNC LINK INCONSISTENT%s.\n", pL,
										(foundRest? " (WITH RESTS;PROBABLY OK)" : "") );
						beamNotesOkay = FALSE;
					}
			}

			if (beamNotesOkay)
				for (qL = RightLINK(pL); qL!=syncL; qL = RightLINK(qL))
					if (BeamsetTYPE(qL))
						if (BeamVOICE(qL)==BeamVOICE(pL)) {
							COMPLAIN2("*DCheckBeams: BEAMSET %d IN SAME VOICE AS UNFINISHED BEAMSET %d.\n",
										qL, pL);
							break;
						}
			if (!grace) {
				if (expect2ndPiece[voice] && (!BeamCrossSYS(pL) || BeamFirstSYS(pL)))
					COMPLAIN("*DCheckBeams: BEAMSET AT %u ISN'T THE 2ND PIECE OF A CROSS-SYS BEAM.\n",
									pL);

				expect2ndPiece[voice] = (BeamCrossSYS(pL) && BeamFirstSYS(pL));
			}
			
			break;

		 case SYNCtype:
		 	for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		 		aNote = GetPANOTE(aNoteL);
				if (aNote->beamed) {
					if (!beamSetL[aNote->voice]) {
						COMPLAIN2("*DCheckBeams: BEAMED NOTE IN SYNC %d VOICE %d WITHOUT BEAMSET.\n",
										pL, aNote->voice);
					}
					else if (!SyncInBEAMSET(pL, beamSetL[aNote->voice]))
						COMPLAIN2("*DCheckBeams: BEAMED NOTE IN SYNC %d VOICE %d NOT IN VOICE'S LAST BEAMSET.\n",
										pL, aNote->voice);
				}
			}
			break;

		 case GRSYNCtype:
		 	for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
		 		aGRNote = GetPAGRNOTE(aGRNoteL);
				if (aGRNote->beamed) {
					if (!grBeamSetL[aGRNote->voice]) {
						COMPLAIN2("*DCheckBeams: BEAMED NOTE IN GRSYNC %d VOICE %d WITHOUT BEAMSET.\n",
										pL, aGRNote->voice);
					}
					else if (!SyncInBEAMSET(pL, grBeamSetL[aGRNote->voice]))
						COMPLAIN2("*DCheckBeams: BEAMED NOTE IN GRSYNC %d VOICE %d NOT IN VOICE'S LAST BEAMSET.\n",
										pL, aGRNote->voice);
				}
			}
			break;

		 default:
			;
		}
	}
	
	return bad;			
}


/* -----------------------------------------------------  DCheckOctaves and helpers -- */

static LINK FindNextSyncGRSync(LINK, INT16);
static INT16 CountSyncVoicesOnStaff(LINK, INT16);

static LINK FindNextSyncGRSync(LINK pL, INT16 staff)
{
	for ( ; pL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				if (NoteOnStaff(pL, staff)) return pL;
				break;
			case GRSYNCtype:
				if (GRNoteOnStaff(pL, staff)) return pL;
				break;
			default:
				;
		}
		
	return NILINK;
}


INT16 CountSyncVoicesOnStaff(LINK syncL, INT16 staff)
{
	LINK aNoteL; INT16 v, count;
	Boolean vInSync[MAXVOICES+1];
	
	for (v = 1; v<=MAXVOICES; v++)
		vInSync[v] = FALSE;
		
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteSTAFF(aNoteL)==staff)
			vInSync[NoteVOICE(aNoteL)] = TRUE;

	for (count = 0, v = 1; v<=MAXVOICES; v++)
		if (vInSync[v]) count++;
	return count;	
}


/* Check consistency of octave sign objects with notes/rests and grace notes they
should be referring to.

Note that we don't keep track of when the octave signs end: therefore, adding
checking that Notes that don't have <inOctava> flags really shouldn't isn't as
easy as it might look. */
 
Boolean DCheckOctaves(Document *doc)
{
	PANOTE				aNote;
	LINK					pL, aNoteL, syncL, measureL, noteOctL;
	INT16					staff, s, nVoice, j;
	LINK					octavaL[MAXSTAVES+1];
	POCTAVA				pOct;
	PANOTEOCTAVA		pNoteOct;
	Boolean				bad;

	bad = FALSE;
	
	for (s = 1; s<=doc->nstaves; s++)
		octavaL[s] = NILINK;
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {

		 case OCTAVAtype:		 	
			staff = OctavaSTAFF(pL);
			if (STAFFN_BAD(doc, staff))
				COMPLAIN2("*DCheckOctaves: OCTAVA AT %u HAS BAD staff %d.\n", pL, staff)
			else
				octavaL[staff] = pL;

			measureL = LSSearch(pL, MEASUREtype, staff,	GO_RIGHT, FALSE);
			if (measureL) {
		 		pOct = GetPOCTAVA(pL);
				pNoteOct = GetPANOTEOCTAVA(pOct->firstSubObj);
				if (IsAfter(measureL, pNoteOct->opSync))
					COMPLAIN("*DCheckOctaves: OCTAVA AT %u IN DIFFERENT MEASURE FROM ITS 1ST SYNC.\n", pL);
			}
			
		 	pOct = GetPOCTAVA(pL);
			noteOctL = pOct->firstSubObj;

			syncL = pL;
			for ( ; noteOctL; noteOctL = NextNOTEOCTAVAL(noteOctL))	{	/* For each SYNC with a note in OCTAVA... */
Next:
				syncL = FindNextSyncGRSync(RightLINK(syncL), staff);
				if (DBadLink(doc, OBJtype, syncL, TRUE)) {
					COMPLAIN("*DCheckOctaves: OCTAVA %d: TROUBLE FINDING SYNCS/GRSYNCS.\n", pL);
					break;
				}
				
				/* If this is a rest, it could not be in the octave sign, so keep looking. */
				
				aNoteL = NoteOnStaff(syncL, staff);
				if (NoteREST(aNoteL)) goto Next;
					
				/*
				 * If a Sync has 2 voices under the octave sign, the octave sign object
				 * has 2 subobjects pointing to that Sync.
				 */
				nVoice = CountSyncVoicesOnStaff(syncL, staff);
				for (j = 1; j<=nVoice; j++) {
					if (j>1) noteOctL = NextNOTEOCTAVAL(noteOctL);
					pNoteOct = GetPANOTEOCTAVA(noteOctL);
					if (pNoteOct->opSync!=syncL)
						COMPLAIN("*DCheckOctaves: OCTAVA %d SYNC/GRSYNC LINK INCONSISTENT.\n", pL);
				}
			}
			break;

		 case SYNCtype:
		 	for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		 		aNote = GetPANOTE(aNoteL);
				if (aNote->inOctava) {
					if (!octavaL[aNote->staffn]) {
						COMPLAIN2("*DCheckOctaves: OCTAVA'D NOTE IN SYNC %d STAFF %d WITHOUT OCTAVA.\n",
										pL, aNote->staffn);
					}
					else if (!SyncInOCTAVA(pL, octavaL[aNote->staffn]))
						COMPLAIN2("*DCheckOctaves: OCTAVA'D NOTE IN SYNC %d STAFF %d NOT IN OCTAVA.\n",
										pL, aNote->staffn);
				}
			}
			break;

		 default:
			;
		}
	
	return bad;			
}


/* -------------------------------------------------------------------- DCheckSlurs -- */
/* For each slur, check:
	-that the slur object (including ties) and its firstSyncL and lastSyncL appear in
		the correct order in the data structure.
	-that the next Sync after the slur has notes in the slur's voice.
	-since we currently (as of v.997) do not allow nested or overlapping slurs or ties
		(though we do allow a tie nested within a slur), check for nesting or overlapping.
	-that cross-system slurs have their first piece on one system and their second on
		the next system.
	-that the Syncs joined by ties are consecutive in the tie's voice. (For the 2nd
		piece of a cross-system tie, we actually check something slightly different; see
		below.)

This function doesn't check for consistency between slurs/ties and their notes (or, for
cross-system ones, Measures or Systems), but DCheckSyncSlurs does. It would also be
nice to check that non-cross-system slurs are entirely on one system. */

Boolean DCheckSlurs(Document *doc)
{
	register LINK		pL;
	register INT16		i;
	INT16					staff, voice;
	LINK					slurEnd[MAXVOICES+1], tieEnd[MAXVOICES+1], endL;
	LINK					slurSysL, otherSlurL, otherSlurSysL, nextSyncL, afterNextSyncL; 
	Boolean				slur2FirstOK, slur2LastOK;
	Boolean				bad;

	bad = FALSE;
	
	for (i = 0; i<=MAXVOICES; i++) {
		slurEnd[i] = tieEnd[i] = NILINK;
	}
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
	{
		for (i = 0; i<=MAXVOICES; i++) {
			if (pL==slurEnd[i]) slurEnd[i] = NILINK;
			if (pL==tieEnd[i]) tieEnd[i] = NILINK;
		}
		
		if (ObjLType(pL)==SLURtype) {
			staff = SlurSTAFF(pL);
			voice = SlurVOICE(pL);
			if (VOICE_BAD(voice))
				COMPLAIN("*DCheckSlurs: SLUR AT %u HAS BAD voice.\n", pL)
			else {
			/* There should be no Syncs between a slur and its Sync. */
			
				nextSyncL = SSearch(pL, SYNCtype, GO_RIGHT);
				if (!NoteInVoice(nextSyncL, voice, FALSE))
					COMPLAIN2("*DCheckSlurs: NEXT SYNC AFTER SLUR/TIE AT %u HAS NO NOTES IN VOICE %d.\n",
										pL, voice);

			/*
			 * Handle slurs and ties separately. For each one, if there's currently one of
			 *	that type in progress and it doesn't end with the next Sync in the voice,
			 *	we have an error; otherwise store the end address of this slur/tie for
			 *	future use.
 			 */
				nextSyncL = LVSearch(pL, SYNCtype, voice, GO_RIGHT, FALSE);
				if (SlurTIE(pL)) {
					if (tieEnd[voice] && tieEnd[voice]!=nextSyncL)
						{ COMPLAIN2("*DCheckSlurs: TIE IN VOICE %d AT %u WITH TIE ALREADY IN PROGRESS.\n",
										voice, pL);
						}
					else
						if (SlurFirstSYSTEM(pL)) {
							endL = LSSearch(pL, SYSTEMtype, ANYONE, GO_RIGHT, FALSE);
							tieEnd[voice] = endL;
						}
						else {
							tieEnd[voice] = SlurLASTSYNC(pL);
							if (SlurCrossSYS(pL) && !SlurFirstSYSTEM(pL))
								afterNextSyncL = nextSyncL;
							else
								afterNextSyncL = LVSearch(RightLINK(nextSyncL), SYNCtype, voice,
																	GO_RIGHT, FALSE);
							if (SlurLASTSYNC(pL)!=afterNextSyncL)
								COMPLAIN2("*DCheckSlurs: TIE IN VOICE %d AT %u lastSyncL IS NOT THE NEXT SYNC IN VOICE.\n",
												voice, pL);
						}
				}
				else {
					if (slurEnd[voice] && slurEnd[voice]!=nextSyncL)
						{ COMPLAIN2("*DCheckSlurs: SLUR IN VOICE %d AT %u WITH SLUR ALREADY IN PROGRESS.\n",
										voice, pL); }
					else
						if (SlurFirstSYSTEM(pL)) {
							endL = LSSearch(pL, SYSTEMtype, ANYONE, GO_RIGHT, FALSE);
							slurEnd[voice] = endL;
						}
						else
							slurEnd[voice] = SlurLASTSYNC(pL);
				}
			}
			
			/*
			 * Check that the slur and its first and last objects are in the correct order.
			 * They should be in the order:
			 *		last, slur, first		for the first piece of cross-system slurs
			 *		first, slur, last		for the last piece of cross-system slurs
			 *	 	slur, first, last		for non-cross-system slurs
			 */

			if (SlurCrossSYS(pL) && SlurFirstSYSTEM(pL)) {
				slur2FirstOK = IsAfter(pL, SlurFIRSTSYNC(pL));
				slur2LastOK = IsAfter(SlurLASTSYNC(pL), pL);
			}
			else if (SlurCrossSYS(pL) && !SlurFirstSYSTEM(pL)) {
				slur2FirstOK = IsAfter(SlurFIRSTSYNC(pL), pL);
				slur2LastOK = IsAfter(pL, SlurLASTSYNC(pL));
			}
			else {
				slur2FirstOK = IsAfter(pL, SlurFIRSTSYNC(pL));
				slur2LastOK = IsAfter(pL, SlurLASTSYNC(pL));
			}

			if (!slur2FirstOK)
				COMPLAIN("*DCheckSlurs: SLUR AT %u AND ITS firstSyncL ARE IN WRONG ORDER.\n", pL);			 

			if (!slur2LastOK)
				COMPLAIN("*DCheckSlurs: SLUR AT %u AND ITS lastSyncL ARE IN WRONG ORDER.\n", pL);			 

			/*
			 * If the slur is the first piece of a cross-system pair, check that the next
			 * cross-system slur piece of the same subtype in the voice exists and is on
			 * the next System.
			 */
			if (SlurCrossSYS(pL) && SlurFirstSYSTEM(pL)) {
				otherSlurL = XSysSlurMatch(pL);
				if (!otherSlurL)
					{ COMPLAIN("*DCheckSlurs: SLUR AT %u IS 1ST PIECE OF CROSS-SYS BUT VOICE HAS NO CROSS-SYS SLURS FOLLOWING.\n",
									pL); }
				else {
					slurSysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
					otherSlurSysL = LSSearch(otherSlurL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
					if (LinkRSYS(slurSysL)!=otherSlurSysL)
						{ COMPLAIN2("*DCheckSlurs: SLUR AT %u IS 1ST PIECE OF CROSS-SYS BUT NEXT SLUR IN VOICE (AT %u) ISN'T IN NEXT SYSTEM.\n",
									pL, otherSlurL); }

				}
			}
		}
	}
	
	return bad;			
}


/* -------------------------------------------------------------- LegalTupletTotDur -- */
/* If the given tuplet contains an unknown or whole-measure duration note/rest, return
-1; else return the tuplet's total duration. NB: in  case of chords, checks only one
note/rest of the chord. */

INT16 LegalTupletTotDur(LINK);
INT16 LegalTupletTotDur(LINK tupL)
{
	LINK pL, aNoteL, endL; INT16 voice;
	
	voice = TupletVOICE(tupL);
	endL = LastInTuplet(tupL);
	for (pL = FirstInTuplet(tupL); pL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL = NoteInVoice(pL, voice, FALSE);
			if (aNoteL)
				if (NoteType(aNoteL)==UNKNOWN_L_DUR || NoteType(aNoteL)<=WHOLEMR_L_DUR)
					return -1;
		}
		if (pL==endL) break;
	}
	
	return TupletTotDir(tupL);
}


/* ------------------------------------------------------------------ DCheckTuplets -- */
/* Check that tuplet objects are in the same measures as their last (and therefore
all of their) Syncs, and check consistency of tuplet objects with Syncs they should
be referring to. Also warn about any tuplets whose notes are subject to roundoff
error in their durations, since that can easily cause problems in syncing. Finally,
check that total durations make sense for the numerators. */

Boolean DCheckTuplets(
					Document *doc,
					Boolean maxCheck		/* FALSE=skip less important checks */
					)
{
	register LINK		pL, syncL;
	LINK					measureL, noteTupL;
	PTUPLET				pTuplet;
	PANOTETUPLE			noteTup;
	Boolean				bad;
	INT16					staff, voice, tupUnit, tupledUnit, totalDur;

	bad = FALSE;

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case TUPLETtype:			 	
				staff = TupletSTAFF(pL);
				voice = TupletVOICE(pL);	
				measureL = LSSearch(pL, MEASUREtype, staff,	GO_RIGHT, FALSE);
				if (measureL) {
					syncL = LastInTuplet(pL);
					if (IsAfter(measureL, syncL))
						COMPLAIN("*DCheckTuplets: TUPLET AT %u IN DIFFERENT MEASURE FROM ITS LAST SYNC.\n",
										pL);
				}
				if (maxCheck) {
					pTuplet = GetPTUPLET(pL);
					tupUnit = GetMaxDurUnit(pL);
					tupledUnit = pTuplet->accDenom*tupUnit/pTuplet->accNum;
					if (pTuplet->accNum*tupledUnit/pTuplet->accDenom!=tupUnit)
						COMPLAIN("DCheckTuplets: DURATIONS OF NOTES IN TUPLET AT %u MAY SHOW ROUNDOFF ERROR.\n",
										pL);
				}

				syncL = pL;
				noteTupL = FirstSubLINK(pL);
				for ( ; noteTupL; noteTupL = NextNOTETUPLEL(noteTupL))	{
					syncL = LVSearch(RightLINK(syncL), SYNCtype, voice, GO_RIGHT, FALSE);
					if (DBadLink(doc, OBJtype, syncL, TRUE)) {
						COMPLAIN("*DCheckTuplets: TUPLET AT %u: TROUBLE FINDING SYNCS.\n",
										pL);
						break;
					}
					noteTup = GetPANOTETUPLE(noteTupL);
					if (noteTup->tpSync!=syncL)
						COMPLAIN("*DCheckTuplets: TUPLET AT %u SYNC LINK INCONSISTENT.\n",
										pL);
				}

				totalDur = LegalTupletTotDur(pL);
				if (totalDur<=0) {
					COMPLAIN("*DCheckTuplets: TUPLET AT %u HAS AN UNKNOWN OR ILLEGAL DURATION NOTE.\n",
									pL);
				}
				else {
					pTuplet = GetPTUPLET(pL);
					if (GetDurUnit(totalDur, pTuplet->accNum, pTuplet->accDenom)==0)
						COMPLAIN2("*DCheckTuplets: TUPLET AT %u TOTAL DURATION OF %d DOESN'T MAKE SENSE.\n",
										pL, totalDur);
				}
				break;
				
			case SYNCtype:
				break;
				
			default:
				;
		}
	
	return bad;
}


/* ----------------------------------------------------------------- DCheckPlayDurs -- */
/* Check that playDurs of notes appear reasonable for their logical durations. */

Boolean DCheckPlayDurs(
					Document *doc,
					Boolean maxCheck		/* FALSE=skip less important checks */
					)
{
	register LINK pL, aNoteL;
	PANOTE aNote; PTUPLET pTuplet;
	register Boolean bad;
	INT16 v, shortDurThresh, tupletNum[MAXVOICES+1], tupletDenom[MAXVOICES+1];
	long lDur;

	bad = FALSE;

	shortDurThresh = PDURUNIT/2;
	for (v = 0; v<=MAXVOICES; v++)
		tupletNum[v] = 0;

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		switch (ObjLType(pL)) {
			case TUPLETtype:			 	
				v = TupletVOICE(pL);
				pTuplet = GetPTUPLET(pL);
				tupletNum[v] = pTuplet->accNum;
				tupletDenom[v] = pTuplet->accDenom;
				break;
				
			case SYNCtype:
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->subType!=UNKNOWN_L_DUR && aNote->subType>WHOLEMR_L_DUR) {
						if (aNote->playDur<shortDurThresh) {
							COMPLAIN2("DCheckPlayDurs: NOTE IN VOICE %d IN SYNC AT %u HAS EXTREMELY SHORT playDur.\n",
											aNote->voice, pL);
						}
						else {
							if (maxCheck) {	/* bcs Create Tuplet bug makes this VERY common right now (v.998a4) */
								lDur = SimpleLDur(aNoteL);
								if (aNote->inTuplet) lDur = (lDur*tupletDenom[v])/tupletNum[v];
								if (aNote->playDur>lDur)
									COMPLAIN2("DCheckPlayDurs: NOTE IN VOICE %d IN SYNC AT %u HAS playDur LONGER THAN LOGICAL DUR.\n",
													aNote->voice, pL);
							}
						}
					}
				}
				break;
				
			default:
				;
		}
	}
	
	return bad;
}


/* ----------------------------------------------------------------- DCheckHairpins -- */
/* For each hairpin, check that the Dynamic object and its firstSyncL and lastSyncL
appear in the correct order in the data structure. */

Boolean DCheckHairpins(Document *doc)
{
	register LINK		pL;
	Boolean				bad;

	bad = FALSE;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		if (ObjLType(pL)==DYNAMtype && IsHairpin(pL)) {
			
/* Check that the hairpin and its first and last objects are in the correct order. 
They should be in the order <hairpin, first, last>, except <first> and <last> can be
identical. */
			if (!IsAfter(pL, DynamFIRSTSYNC(pL)))
				COMPLAIN("*DCheckHairpins: HAIRPIN AT %u AND ITS firstSyncL ARE IN WRONG ORDER.\n",
								pL);			 
			if (DynamFIRSTSYNC(pL)!=DynamLASTSYNC(pL)
			&&  !IsAfter(DynamFIRSTSYNC(pL), DynamLASTSYNC(pL)))
				COMPLAIN("*DCheckHairpins: HAIRPIN AT %u firstSyncL AND lastSyncL ARE IN WRONG ORDER.\n",
								pL);			 
		}
	}
	
	return bad;			
}


/* ------------------------------------------------------------------ DCheckContext -- */
/* Do consistency checks on the entire score to see if changes of clef, key
signature, meter and dynamics in context fields of STAFFs and MEASUREs agree
with appearances of actual CLEF, KEYSIG, TIMESIG and DYNAM objects. */
 
Boolean DCheckContext(Document *doc)
{
	register INT16	i;
	register PASTAFF aStaff;
	register PAMEASURE aMeas;
	PACLEF		aClef;
	PAKEYSIG		aKeySig;
	PATIMESIG	aTimeSig;
	PADYNAMIC	aDynamic;
	PCLEF			pClef;
	PKEYSIG		pKeySig;
	register LINK pL;
	LINK			aStaffL, aMeasL, aClefL, aKeySigL,
					aTimeSigL, aDynamicL;
	SignedByte	clefType[MAXSTAVES+1];			/* Current context: clef, */
	INT16			nKSItems[MAXSTAVES+1];			/*   sharps & flats in key sig., */
	SignedByte	timeSigType[MAXSTAVES+1],		/*   time signature, */
					numerator[MAXSTAVES+1],
					denominator[MAXSTAVES+1],
					dynamicType[MAXSTAVES+1];		/*		dynamic mark */
	Boolean		aStaffFound[MAXSTAVES+1],
					aMeasureFound[MAXSTAVES+1];
	register Boolean bad;

	bad = FALSE;
		
	for (i = 1; i<=doc->nstaves; i++) {
		aStaffFound[i] = FALSE;
		aMeasureFound[i] = FALSE;
	}

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		switch (ObjLType(pL)) {

			case STAFFtype:
				for (aStaffL=FirstSubLINK(pL); aStaffL; 
						aStaffL=NextSTAFFL(aStaffL)) {
					aStaff = GetPASTAFF(aStaffL);
					if (!aStaffFound[aStaff->staffn]) {							/* Init. staff info */		
						clefType[aStaff->staffn] = aStaff->clefType;
						nKSItems[aStaff->staffn] = aStaff->nKSItems;
						timeSigType[aStaff->staffn] = aStaff->timeSigType;
						numerator[aStaff->staffn] = aStaff->numerator;
						denominator[aStaff->staffn] = aStaff->denominator;
						dynamicType[aStaff->staffn] = aStaff->dynamicType;
						aStaffFound[aStaff->staffn] = TRUE;
					}
					else {																/* Check staff info */
						if (clefType[aStaff->staffn]!=aStaff->clefType) {
							COMPLAIN2("*DCheckContext: clefType FOR STAFF %d IN STAFF AT %u INCONSISTENCY.\n",
											aStaff->staffn, pL);
							aStaff = GetPASTAFF(aStaffL);
						}
						if (nKSItems[aStaff->staffn]!=aStaff->nKSItems) {
							COMPLAIN2("*DCheckContext: nKSItems FOR STAFF %d IN STAFF AT %u INCONSISTENCY.\n",
											aStaff->staffn, pL);
							aStaff = GetPASTAFF(aStaffL);
						}
						if (timeSigType[aStaff->staffn]!=aStaff->timeSigType
						||  numerator[aStaff->staffn]!=aStaff->numerator
						||  denominator[aStaff->staffn]!=aStaff->denominator) {
							COMPLAIN2("DCheckContext: TIMESIG FOR STAFF %d IN STAFF AT %u INCONSISTENCY.\n",
											aStaff->staffn, pL);
							aStaff = GetPASTAFF(aStaffL);
						}
						if (dynamicType[aStaff->staffn]!=aStaff->dynamicType)
							COMPLAIN2("DCheckContext: dynamicType FOR STAFF %d IN STAFF AT %u INCONSISTENCY.\n",
											aStaff->staffn, pL);
					}
				}
				break;

			case MEASUREtype:	
				for (aMeasL=FirstSubLINK(pL); aMeasL; 
						aMeasL=NextMEASUREL(aMeasL)) {
					aMeas = GetPAMEASURE(aMeasL);
					if (clefType[aMeas->staffn]!=aMeas->clefType) {
						COMPLAIN2("*DCheckContext: clefType FOR STAFF %d IN MEAS AT %u INCONSISTENCY.\n",
										aMeas->staffn, pL);
						aMeas = GetPAMEASURE(aMeasL);
					}
					if (nKSItems[aMeas->staffn]!=aMeas->nKSItems) {
						COMPLAIN2("*DCheckContext: nKSItems FOR STAFF %d IN MEAS AT %u INCONSISTENCY.\n",
										aMeas->staffn, pL);
						aMeas = GetPAMEASURE(aMeasL);
					}
					if (timeSigType[aMeas->staffn]!=aMeas->timeSigType
					||  numerator[aMeas->staffn]!=aMeas->numerator
					||  denominator[aMeas->staffn]!=aMeas->denominator) {
						COMPLAIN2("DCheckContext: TIMESIG FOR STAFF %d IN MEAS AT %u INCONSISTENCY.\n",
										aMeas->staffn, pL);
						aMeas = GetPAMEASURE(aMeasL);
					}
					if (dynamicType[aMeas->staffn]!=aMeas->dynamicType)
						COMPLAIN2("DCheckContext: dynamicType FOR STAFF %d IN MEAS AT %u INCONSISTENCY.\n",
										aMeas->staffn, pL);
					aMeasureFound[aMeas->staffn] = TRUE;
				}
				break;

			case CLEFtype:
				for (aClefL=FirstSubLINK(pL); aClefL; aClefL=NextCLEFL(aClefL)) {
					aClef = GetPACLEF(aClefL);
					pClef = GetPCLEF(pL);
					if (!pClef->inMeasure										/* System initial clef? */
					&& aMeasureFound[aClef->staffn]							/* After the 1st System? */
					&&  clefType[aClef->staffn]!=aClef->subType)			/* Yes, check the clef */
						COMPLAIN2("*DCheckContext: clefType FOR STAFF %d IN CLEF AT %u INCONSISTENCY.\n",
										aClef->staffn, pL);
					clefType[aClef->staffn] = aClef->subType;
				}
				break;

			case KEYSIGtype:
				for (aKeySigL=FirstSubLINK(pL); aKeySigL;
						aKeySigL = NextKEYSIGL(aKeySigL))  {
					aKeySig = GetPAKEYSIG(aKeySigL);
					pKeySig = GetPKEYSIG(pL);
					if (!pKeySig->inMeasure										/* System initial key sig.? */
					&& aMeasureFound[aKeySig->staffn]						/* After the 1st System? */
					&&  nKSItems[aKeySig->staffn]!=aKeySig->nKSItems)	/* Yes, check it */
						COMPLAIN2("*DCheckContext: nKSItems FOR STAFF %d IN KEYSIG AT %u INCONSISTENCY.\n",
										aKeySig->staffn, pL);
					nKSItems[aKeySig->staffn] = aKeySig->nKSItems;
				}
				break;

			case TIMESIGtype:
				for (aTimeSigL=FirstSubLINK(pL); aTimeSigL;
						aTimeSigL = NextTIMESIGL(aTimeSigL))  {
					aTimeSig = GetPATIMESIG(aTimeSigL);
					timeSigType[aTimeSig->staffn] = aTimeSig->subType;
					numerator[aTimeSig->staffn] = aTimeSig->numerator;
					denominator[aTimeSig->staffn] = aTimeSig->denominator;
				}
				break;
				
			case DYNAMtype:
				aDynamicL = FirstSubLINK(pL);									/* Only one subobj, so far */
				aDynamic = GetPADYNAMIC(aDynamicL);
				if (DynamType(pL)<FIRSTHAIRPIN_DYNAM)
					dynamicType[aDynamic->staffn] = DynamType(pL);
				break;

			default:
				;
		}
	}
		
	return bad;
}

//#endif /* PUBLIC_VERSION */
