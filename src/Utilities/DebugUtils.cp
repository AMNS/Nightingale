/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Copyright © 2017 by Avian Music Notation Foundation.
 * All Rights Reserved.
 */

/* File DebugUtils.c - debugging functions to check the validity of our data structures:
	DBadLink				DCheckHeaps				DCheckHeadTail
	DCheckSyncSlurs
	DCheckMBBox				DCheckMeasSubobjs		DCheckNode
	DCheckNodeSel			DCheckSel				DCheckVoiceTable
	DCheckHeirarchy			DCheckJDOrder			DCheckBeamset
	DCheckBeams				DCheckOttavas			DCheckSlurs
	DCheckTuplets			DCheckHairpins			DCheckObjRect
	DCheckContext
	DCheck1NEntries			DCheckNEntries			DCheck1SubobjLinks
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "DebugUtils.h"

#define DDB

/* These functions implement three levels of checking:
	Most important: messages about problems are prefixed with '•'
	More important: messages about problems are prefixed with '*'
	Less & Least important: messages about problems have no prefix
*/

extern short nerr, errLim;
extern Boolean minDebugCheck;			/* True = don't print Less and Least important checks */

#ifdef DDB

Boolean QDP(char *fmtStr)
{
	Boolean printAll = !minDebugCheck;

	if (printAll || (*fmtStr=='*' || *fmtStr=='•')) {
		SysBeep(8);
		nerr++;
		return True;
	}
	
	return False; 
}

#endif

/* If we're looking at the Clipboard, Undo or Mstr Page, "flag" arg by adding a huge offset */

#define FP(pL)	( abnormal ? 1000000L+(pL) : (pL) )


/* ------------------------------------------------------------------------- DBadLink --- */
/* Given a type and a LINK, check whether the LINK is bad and return True if so, False
if it's OK. Check whether the LINK is larger than the heap currently allows, and if
LINK is on the freelist: in either case, it's not valid. */

Boolean DBadLink(
			Document *doc, short type,
			LINK pL,
			Boolean allowNIL)				/* True=NILINK is an acceptable value */
{
	HEAP *heap;  LINK link;
	Byte *p, *start;

	if (pL==NILINK) return !allowNIL;
	
#if 1
	if (GARBAGEL(type, pL)) return True;
	
	heap = doc->Heap+type;
#else
	heap = doc->Heap+type;
	if (pL>=heap->nObjs) return True;
#endif
	
	/* Look through the freelist for pL. If we find it, it's not valid! */
	
	start = (Byte *)(*heap->block);
	link = heap->firstFree;
	while (link) {
		if (link==pL) return True;
		p = start + ((unsigned long)heap->objSize * (unsigned long)link);
		link = *(LINK *)p;
	}
	
	return False;
}


/* ----------------------------------------------------------------------- DCheckHeaps -- */
/* Check whether <doc>'s Heaps are the current global Heaps. */

Boolean DCheckHeaps(Document *doc)
{
	Boolean bad=False;
	
	if (doc->Heap!=Heap) {
		COMPLAIN2("•DCheckHeaps: doc->Heap=%lx IS NOT EQUAL TO Heap=%lx.\n",
						doc->Heap, Heap);
		return True;	
	}
	else return False;
}



/* -------------------------------------------------------------------- DCheckHeadTail -- */
/* Do consistency check on head or tail of object list. */

Boolean DCheckHeadTail(
				Document *doc,
				LINK pL,
				Boolean /*fullCheck*/			/* False=skip less important checks (unused) */
				)
{
	Boolean		bad;
	LINK		partL;
	PPARTINFO	pPartInfo;
	short		nextStaff;

	bad = False;

	if (pL==doc->headL || pL==doc->undo.headL || pL==doc->masterHeadL) {
		if (LeftLINK(pL)!=NILINK || DBadLink(doc, OBJtype, RightLINK(pL), True))
			COMPLAIN("•DCheckHeadTail: HEADER (L%u) HAS A BAD LINK.\n", pL);
		if (ObjLType(pL)!=HEADERtype)
			COMPLAIN("•DCheckHeadTail: HEADER (L%u) HAS BAD type.\n", pL);
				
		if (pL==doc->headL) {
			if (LinkNENTRIES(pL)<2 || LinkNENTRIES(pL)>MAXSTAVES+1)
				COMPLAIN("•DCheckHeadTail: HEADER (L%u) HAS BAD nparts.\n", pL);
			if (doc->nstaves<1 || doc->nstaves>MAXSTAVES)
				COMPLAIN("•DCheckHeadTail: HEADER (L%u) HAS BAD nstaves.\n", pL);

			partL = FirstSubLINK(pL);										/* Skip zeroth part */
			nextStaff = 1;
			for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL)) {
				pPartInfo = GetPPARTINFO(partL);
				if (pPartInfo->firstStaff!=nextStaff
				||  pPartInfo->lastStaff>doc->nstaves
				||  pPartInfo->firstStaff>pPartInfo->lastStaff)
					COMPLAIN("•DCheckHeadTail: PART WITH partL=L%u HAS BAD FIRST STAFF OR LAST STAFF.\n",
								partL);
				nextStaff = pPartInfo->lastStaff+1;
				if (pPartInfo->transpose<-24 || pPartInfo->transpose>24)
					COMPLAIN2("DCheckHeadTail: part with partL=L%u has suspicious transposition %d.\n",
								partL, pPartInfo->transpose);
			}
			if (nextStaff-1!=doc->nstaves)
				COMPLAIN("•DCheckHeadTail: LAST PART'S LAST STAFF OF %ld DISAGREES WITH nstaves.\n",
					(long)nextStaff-1);
		}
	}

	else if (pL==doc->tailL || pL==doc->undo.tailL || pL==doc->masterTailL) {
		if (DBadLink(doc, OBJtype, LeftLINK(pL), True) || RightLINK(pL)!=NILINK)
			COMPLAIN("•DCheckHeadTail: TAIL (L%u) HAS A BAD LINK.\n", pL);
		if (ObjLType(pL)!=TAILtype)
			COMPLAIN("•DCheckHeadTail: TAIL (L%u) HAS BAD type.\n", pL);
	}
	return bad;
}


/* ------------------------------------------------------------------- DCheckSyncSlurs -- */
/* Given a note, if it claims to be slurred or tied, check for a slur object for its
voice that refers to it. If it's tied, also look for a match for its note number in the
"other" Sync. */

Boolean DCheckSyncSlurs(Document *doc, LINK syncL, LINK aNoteL)
{
	SearchParam pbSearch;
	LINK		prevSyncL, slurL, searchL, otherSyncL;
	short		voice;
	Boolean		bad;

	bad = False;

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;									/* Prepare for search */
	voice = pbSearch.voice = NoteVOICE(aNoteL);

	pbSearch.subtype = False;								/* Slur, not tieset */

	if (NoteSLURREDL(aNoteL)) {
		/* If we start searching for the slur here, we may find one that starts with this
		   Sync, while we want one that ends with this Sync; instead, start searching
		   from the previous Sync in this voice. Exception: if the previous Sync is not
		   in the same System, the slur we want is the second piece of a cross-system
		   one; in this case, search to the right from the previous Measure. Cf.
		   LeftSlurSearch. */
			
		prevSyncL = LVSearch(LeftLINK(syncL), SYNCtype, voice, GO_LEFT, False);
		if (prevSyncL && SameSystem(syncL, prevSyncL)) {
			searchL = prevSyncL;
			slurL = L_Search(searchL, SLURtype, GO_LEFT, &pbSearch);
		}
		else {
			searchL = LSSearch(syncL, MEASUREtype, 1, GO_LEFT, False);
			slurL = L_Search(searchL, SLURtype, GO_RIGHT, &pbSearch);
		}

		if (!slurL) {
			COMPLAIN3("DCheckSyncSlurs: NOTE slurredL IN VOICE %d IN SYNC L%u, MEASURE %d, HAS NO SLUR.\n",
						voice, syncL, GetMeasNum(doc, syncL));
		}
		else {
			if (SlurLASTSYNC(slurL)!=syncL) {
				COMPLAIN3("DCheckSyncSlurs: NOTE slurredL IN VOICE %d IN SYNC L%u, MEASURE %d, HAS BAD SLUR.\n",
							voice, syncL, GetMeasNum(doc, syncL));
			}
		}
	}
	
	if (NoteSLURREDR(aNoteL)) {
		slurL = L_Search(syncL, SLURtype, GO_LEFT, &pbSearch);
		if (!slurL) {
			COMPLAIN3("DCheckSyncSlurs: NOTE slurredR IN VOICE %d IN SYNC L%u, MEASURE %d, HAS NO SLUR.\n",
						voice, syncL, GetMeasNum(doc, syncL));
		}
		else {
			if (SlurFIRSTSYNC(slurL)!=syncL) {
				COMPLAIN3("DCheckSyncSlurs: NOTE slurredR IN VOICE %d IN SYNC L%u, MEASURE %d, HAS BAD SLUR.\n",
							voice, syncL, GetMeasNum(doc, syncL));
			}
		}
	}

	pbSearch.subtype = True;									/* Tieset, not slur */

	if (NoteTIEDL(aNoteL)) {
		/* If we start searching for the slur here, we may find one that starts with this
			Sync, while we want one that ends with this Sync; to avoid that, start
			searching from the previous Sync in this voice. Exception: if the previous
			Sync is not in the same System, the slur we want is the second piece of a
			cross-system one; in that case, search to the right from the previous Measure.
			Cf. LeftSlurSearch. */
			
		prevSyncL = LVSearch(LeftLINK(syncL), SYNCtype, voice, GO_LEFT, False);
		if (prevSyncL && SameSystem(syncL, prevSyncL)) {
			searchL = prevSyncL;
			slurL = L_Search(searchL, SLURtype, GO_LEFT, &pbSearch);
		}
		else {
			searchL = LSSearch(syncL, MEASUREtype, 1, GO_LEFT, False);
			slurL = L_Search(searchL, SLURtype, GO_RIGHT, &pbSearch);
		}

		if (!slurL) {
			COMPLAIN3("DCheckSyncSlurs: NOTE tiedL IN VOICE %d IN SYNC L%u, MEASURE %d, HAS NO TIE.\n",
						voice, syncL, GetMeasNum(doc, syncL));
		}
		else {
			if (SlurLASTSYNC(slurL)!=syncL) {
				COMPLAIN3("DCheckSyncSlurs: NOTE tiedL IN VOICE %d IN SYNC L%u, MEASURE %d, HAS BAD TIE.\n",
							voice, syncL, GetMeasNum(doc, syncL));
			}
			else {
				otherSyncL = SlurFIRSTSYNC(slurL);
				if (SyncTYPE(otherSyncL)) {
					if (!FindNoteNum(otherSyncL, voice, NoteNUM(aNoteL)))
						COMPLAIN2("DCheckSyncSlurs: NOTE IN VOICE %d IN SYNC L%u TIED BUT SYNC IT'S TIED TO DOESN'T MATCH ITS noteNum.\n",
										voice, syncL);
				}
			}
		}
	}
	
	if (NoteTIEDR(aNoteL)) {
		slurL = L_Search(syncL, SLURtype, GO_LEFT, &pbSearch);
		if (!slurL) {
			COMPLAIN3("DCheckSyncSlurs: NOTE tiedR IN VOICE %d IN SYNC L%u, MEASURE %d, HAS NO TIE.\n",
						voice, syncL, GetMeasNum(doc, syncL));
		}
		else {
			if (SlurFIRSTSYNC(slurL)!=syncL) {
				COMPLAIN3("DCheckSyncSlurs: NOTE tiedR IN VOICE %d IN SYNC L%u, MEASURE %d, HAS BAD TIE.\n",
							voice, syncL, GetMeasNum(doc, syncL));
			}
			else {
				otherSyncL = SlurLASTSYNC(slurL);
				if (SyncTYPE(otherSyncL)) {
					if (!FindNoteNum(otherSyncL, voice, NoteNUM(aNoteL)))
						COMPLAIN2("DCheckSyncSlurs: NOTE IN VOICE %d IN SYNC L%u TIED BUT SYNC IT'S TIED TO DOESN'T MATCH ITS noteNum.\n",
										voice, syncL);
				}
			}
		}
	}
	
	return bad;			
}


/* ----------------------------------------------------------------------- DCheckMBBox -- */

Boolean DCheckMBBox(
				Document *doc,
				LINK pL,
				Boolean /*abnormal*/		/* Node in clipboard, Undo or Mstr Page? */
				)
{
	LINK systemL, checkL;
	PSYSTEM pSystem;
	Rect mBBox, sysRect, unRect;
	PMEASURE pMeasure;
	Boolean bad=False;
	char str[256];
	
	systemL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, False);
	pSystem = GetPSYSTEM(systemL);
	D2Rect(&pSystem->systemRect, &sysRect);
	pMeasure = GetPMEASURE(pL);
	mBBox = pMeasure->measureBBox;

	/* If Measure is of width zero, it can't contain anything, nor can it be followed by
	   another Measure until we've started a new System. */
		
	if (mBBox.right<=mBBox.left) {
		for (checkL = RightLINK(pL); !SystemTYPE(checkL); checkL = RightLINK(checkL)) {
			if (SystemTYPE(checkL) || TailTYPE(checkL)) break;
			if (!PageTYPE(checkL)) {
				COMPLAIN3("DCheckMBBox: MEASURE %d (L%u) IS NON-EMPTY (L%u) BUT HAS WIDTH ZERO.\n",
							GetMeasNum(doc, pL), pL, checkL);
				break;
			}
		}
	}

	/* _measureBBox_ and _sysRect_ can be compared because both are in pixels relative
	   to the top left corner of the page. */
	
	UnionRect(&mBBox, &sysRect, &unRect);
//LogPrintf(LOG_DEBUG, "mBBox=%d %d %d %d, systemRect=%d %d %d %d\n", mBBox.top, mBBox.left,
//mBBox.bottom, mBBox.right, sysRect.top, sysRect.left, sysRect.bottom, sysRect.right);
	if (!EqualRect(&sysRect, &unRect)) {
		sprintf(str, "(%s%s%s%s)",
			(mBBox.top<sysRect.top? "top " : ""),
			(mBBox.left<sysRect.left? "left " : ""),
			(mBBox.bottom>sysRect.bottom? "bottom " : ""),
			(mBBox.right>sysRect.right? "right" : "") );
		COMPLAIN3("DCheckMBBox: MEASURE %d (L%u) BBOX GOES BEYOND SYSTEMRECT %s.\n",
				GetMeasNum(doc, pL), pL, str);
	}
	return bad;
}


/* ----------------------------------------------------------------- DCheckMeasSubobjs -- */
/* Assumes the MEASUREheap is locked. */

Boolean DCheckMeasSubobjs(
				Document *doc,
				LINK pL,					/* Measure */
				Boolean /*abnormal*/)
{
	LINK aMeasL;  PAMEASURE aMeas;
	Boolean haveMeas[MAXSTAVES+1], foundConn;
	short s, missing, connStaff[MAXSTAVES+1], nBadConns;
	Boolean bad=False;

	for (s = 1; s<=doc->nstaves; s++)
		haveMeas[s] = False;

	for (aMeasL = FirstSubLINK(pL); aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
		aMeas = GetPAMEASURE(aMeasL);
		if (STAFFN_BAD(doc, aMeas->staffn)) {
			COMPLAIN3("*DCheckMeasSubobjs: SUBOBJ L%u IN MEASURE L%u staffn %d IS BAD.\n",
							aMeasL, pL, aMeas->staffn);
		}
		else {
			haveMeas[aMeas->staffn] = True;
			connStaff[aMeas->staffn] = aMeas->connStaff;
		}

		if (aMeas->measureNum > MAX_FIRSTMEASNUM+MAX_SCORE_MEASURES)
			{ COMPLAIN("DCheckMeasSubobjs: Measure L%u has a suspicious measure number.\n", pL); }

		if (aMeas->staffn==1 && aMeas->connAbove)
			COMPLAIN("*DCheckMeasSubobjs: SUBOBJ ON STAFF 1 IN MEASURE L%u HAS connAbove.\n", pL);
		if (aMeas->connStaff<0 ||
		    (aMeas->connStaff>0 && (aMeas->connStaff<=aMeas->staffn || aMeas->connStaff>doc->nstaves)))
			COMPLAIN2("*DCheckMeasSubobjs: SUBOBJ ON STAFF %d IN MEASURE L%u HAS BAD connStaff.\n",
							aMeas->staffn, pL);
		if (aMeas->clefType<LOW_CLEF ||  aMeas->clefType>HIGH_CLEF)
			COMPLAIN2("*DCheckMeasSubobjs: SUBOBJ L%u IN MEASURE L%u HAS BAD clefType.\n", aMeasL, pL);
		if (aMeas->nKSItems<0 ||  aMeas->nKSItems>7)
			COMPLAIN2("*DCheckMeasSubobjs: SUBOBJ L%u IN MEASURE L%u HAS BAD nKSItems.\n", aMeasL, pL);
		if (TSTYPE_BAD(MeasTIMESIGTYPE(aMeasL)))
			COMPLAIN3("*DCheckMeasSubobjs: SUBOBJ L%u IN MEASURE L%u HAS BAD timeSigType %d.\n",
						aMeasL, pL, MeasTIMESIGTYPE(aMeasL));
		if (TSNUMER_BAD(MeasNUMER(aMeasL)))
			COMPLAIN2("*DCheckMeasSubobjs: SUBOBJ IN MEASURE L%u HAS BAD TIMESIG numerator %d.\n",
						pL, MeasNUMER(aMeasL));
		if (TSDENOM_BAD(MeasDENOM(aMeasL)))
			COMPLAIN2("*DCheckMeasSubobjs: SUBOBJ IN MEASURE L%u HAS BAD TIMESIG denominator %d.\n",
						pL, MeasDENOM(aMeasL));
		if (TSDUR_BAD(MeasNUMER(aMeasL), MeasDENOM(aMeasL)))
			COMPLAIN3("*DCheckMeasSubobjs: SUBOBJ IN MEASURE L%u HAS TIMESIG OF TOO GREAT DURATION %d/%d.\n",
						pL, MeasNUMER(aMeasL), MeasDENOM(aMeasL));
	}

	for (missing = 0, s = 1; s<=doc->nstaves; s++)
		if (!haveMeas[s]) missing++;					
	if (missing>0) {
		COMPLAIN2("*DCheckMeasSubobjs: MEASURE L%u HAS %d MISSING STAFF/STAVES.\n", pL, missing);
	}
	else {
		nBadConns = 0;
		for (aMeasL = FirstSubLINK(pL); aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
			aMeas = GetPAMEASURE(aMeasL);
			if (aMeas->connAbove) {
				for (foundConn = False, s = 1; s<aMeas->staffn; s++)
					if (connStaff[s]>=aMeas->staffn) foundConn = True;
				if (!foundConn) nBadConns++;
			}
		}
		if (nBadConns>0)
			COMPLAIN2("*DCheckMeasSubobjs: %d SUBOBJ(S) IN MEASURE L%u HAVE connAbove BUT NO CONNECTED STAFF.\n",
							nBadConns, pL);
	}

	return bad;
}


/* ------------------------------------------------------------------------ DCheckNode -- */
/* Do legality and simple (context-free or dependent only on "local" context) consistency
checks on an individual node. Returns:
	0 if no problems are found,
	-1 if the left or right link is garbage or inconsistent,
	+1 if less serious problems are found.
*/

/* Set arbitrary but generous limit for likely duration of a legitimate score (in whole
notes), and for number of pages in a legitimate score. */

#define MAX_WHOLES_DUR	6000L
#define MAX_PAGES 1000L

short DCheckNode(
			Document *doc,
			LINK pL,
			short where,		/* Which object list: MAIN_DSTR, CLIP_DSTR, UNDO_DSTR, or MP_DSTR */
			Boolean fullCheck	/* False=skip less important checks */
			)
{
	short		minEntries, maxEntries;
	Boolean		bad, terrible, abnormal;
	Boolean		objRectOrdered, lRectOrdered, rRectOrdered;
	POBJHDR		p;
	POBJHDR		apLeft, apRight, pLeft, pRight;
	PSYSTEM		pSystem;
	PSTAFF		pStaff;
	PMEASURE	pMeasure;
	PKEYSIG		pKeySig;
	PANOTE		aNote;
	PASTAFF		aStaff;
	PAPSMEAS	aPSMeas;
	PAKEYSIG	aKeySig;
	PATIMESIG	aTimeSig;
	PANOTETUPLE	aNoteTuple;
	PANOTEOTTAVA	aNoteOct;
	PDYNAMIC	pDynamic;
	PSLUR		pSlur;
	PASLUR		aSlur;
	PACONNECT	aConnect;
	PAMODNR		aModNR;
	LINK		aNoteL;
	LINK		apLeftL, apRightL, aStaffL;
	LINK		aPSMeasL, aClefL, aKeySigL, aTimeSigL, aNoteTupleL;
	LINK		aNoteOctL, aDynamicL, aSlurL, aConnectL, aModNRL;
	SignedByte	minLDur;

	bad = terrible = False;										/* I like it fine so far */
	PushLock(OBJheap);

	p = GetPOBJHDR(pL);
	pLeft = GetPOBJHDR(p->left);
	pRight = GetPOBJHDR(p->right);

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

/* CHECK self, left, and right links. --------------------------------------------- */

		if (DBadLink(doc, OBJtype, pL, False)) {
				COMPLAIN("•DCheckNode: Object L%u LINK IS GARBAGE.\n", pL);
				terrible = True;
		}

		if (LeftLINK(pL)==NILINK || RightLINK(pL)==NILINK
		||  DBadLink(doc, OBJtype, LeftLINK(pL), True)
		||  DBadLink(doc, OBJtype, RightLINK(pL), True)) {
				COMPLAIN("•DCheckNode: Object L%u HAS A GARBAGE L OR R LINK.\n", pL);
				terrible = True;
		}
		else if (pLeft->right!=pL || pRight->left!=pL) {
			COMPLAIN("•DCheckNode: Object L%u HAS AN INCONSISTENT LINK.\n", pL);
			terrible = True;
		}
		
		if (terrible) return -1;									/* Give up now */

		if (TYPE_BAD(pL) || ObjLType(pL)==HEADERtype || ObjLType(pL)==TAILtype)
			COMPLAIN2("•DCheckNode: Object L%u HAS BAD type %d.\n", pL, ObjLType(pL));
			
		if (fullCheck)
			if (DCheck1SubobjLinks(doc, pL)) bad = terrible = True;

		if (DCheck1NEntries(doc, pL)) bad = True;

		if (terrible) return -1;									/* Give up now */

		GetObjectLimits(ObjLType(pL), &minEntries, &maxEntries, &objRectOrdered);
		if (LinkNENTRIES(pL)<minEntries || LinkNENTRIES(pL)>maxEntries)
			COMPLAIN2("•DCheckNode: Object L%u HAS nEntries=%d, ILLEGAL FOR ITS TYPE.\n",
				pL, LinkNENTRIES(pL));
			
/* CHECK object's absolute horizontal position (rather crudely) and objRect. ------------ */

		if (!BeamsetTYPE(pL))									/* Beamset xd is unused */
			if (LinkXD(pL)<LEFT_HLIM(doc, pL) || LinkXD(pL)>RIGHT_HLIM(doc)) {
				COMPLAIN2("DCheckNode: Object L%u IN MEASURE %d HAS BAD HORIZONTAL POSITION.\n",
								pL, GetMeasNum(doc, pL));
			}

		if (!abnormal && LinkVALID(pL)) {
			if (GARBAGE_Q1RECT(p->objRect)) {
				/* It's OK for initial keysigs to be unselectable. */
				
				pKeySig = GetPKEYSIG(pL);						/* FIXME: OR USE KeySigINMEAS? */
				if (!(KeySigTYPE(pL) && !pKeySig->inMeasure))
					COMPLAIN2("DCheckNode: Object L%u IN MEASURE %d IS UNSELECTABLE: IT HAS A GARBAGE objRect.\n",
						pL, GetMeasNum(doc, pL));
			}
			
			/* Valid initial objects, e.g., "deleted", can have zero-width objRects. */
			
			else if (!ClefTYPE(pL) && !KeySigTYPE(pL) && !TimeSigTYPE(pL)
						&& ZERODIM_RECT(p->objRect)) {
				COMPLAIN("DCheckNode: Object L%u IS UNSELECTABLE: IT HAS A ZERO-WIDTH AND/OR HEIGHT objRect.\n",
						pL);
			}

			else if (objRectOrdered) {

/* CHECK the objRect's relative horizontal position. ------------------------------ */
/* We first try find objects in this System to its left and/or right in the object list
that have meaningful objRects. If we find such object(s), we check whether their relative
positions agree with their relative object-list positions. */

				for (lRectOrdered = False, apLeftL = LeftLINK(pL);
						apLeftL!=doc->headL; apLeftL = LeftLINK(apLeftL)) {
					if (ObjLType(apLeftL)==SYSTEMtype) break;
					GetObjectLimits(ObjLType(apLeftL), &minEntries, &maxEntries, &lRectOrdered);
					if (LinkVALID(apLeftL) && lRectOrdered) break; 
				}
				for (rRectOrdered = False, apRightL = RightLINK(pL);
						apRightL!=doc->tailL; apRightL = RightLINK(apRightL)) {
					if (ObjLType(apRightL)==SYSTEMtype) break;
					GetObjectLimits(ObjLType(apRightL), &minEntries, &maxEntries, &rRectOrdered);
					if (LinkVALID(apRightL) && rRectOrdered) break; 
				}
				if (fullCheck) {
					apLeft = GetPOBJHDR(apLeftL);
					if (LinkVALID(apLeftL) && lRectOrdered
					&&  p->objRect.left < apLeft->objRect.left)
						COMPLAIN("DCheckNode: Object L%u: objRect.left relative to previous is suspicious.\n", pL);
					apRight = GetPOBJHDR(apRightL);
					if (LinkVALID(apRightL) && rRectOrdered
					&&  p->objRect.left > apRight->objRect.left)
						COMPLAIN("DCheckNode: Object L%u: objRect.left relative to next is suspicious.\n", pL);
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
					COMPLAIN("*DCheckNode: Object L%u VERTICAL POSITION SHOULD BE ZERO FOR ITS TYPE BUT ISN'T.\n",
									pL);
		}

/* CHECK everything else. Object type dependent. ---------------------------------- */

		switch (ObjLType(pL)) {
		
			case SYNCtype:
			{
				short v;
				short	vNotes[MAXVOICES+1],					/* No. of notes in sync in voice */
						vMainNotes[MAXVOICES+1],				/* No. of stemmed notes in sync in voice */
						vlDur[MAXVOICES+1],						/* l_dur of voice's notes in sync */
						vlDots[MAXVOICES+1],					/* Number of dots on voice's notes in sync */
						vStaff[MAXVOICES+1],					/* Staff the voice is on */
						vOct[MAXVOICES+1],						/* inOttava status for the voice */
						lDurCode;
				Boolean vTuplet[MAXVOICES+1];					/* Is the voice in a tuplet? */
				long	timeStampHere, timeStampBefore, timeStampAfter;
				LINK	tempL;

				/* Do consistency checks on relative times of this Sync and its neighbors. */
				
				if (!abnormal) {
					timeStampHere = SyncTIME(pL);
					if (timeStampHere>MAX_SAFE_MEASDUR) {
						COMPLAIN2("DCheckNode: SYNC L%u: LTIME %ld IS PROBABLY BAD.\n",
										pL, timeStampHere);
					}
					else {
						timeStampHere = SyncAbsTime(pL);
						tempL = LSSearch(LeftLINK(pL), SYNCtype, ANYONE, GO_LEFT, False);
						if (tempL) {
							timeStampBefore = SyncAbsTime(tempL);
							if (timeStampBefore>=0L && timeStampHere>=0L
							&&  timeStampBefore>=timeStampHere)
								COMPLAIN2("DCheckNode: SYNC L%u: TIMESTAMP %ld REL TO PREVIOUS IS WRONG.\n",
												pL, timeStampHere);
						}
						tempL = LSSearch(RightLINK(pL), SYNCtype, ANYONE, GO_RIGHT, False);
						if (tempL) {
							timeStampAfter = SyncAbsTime(tempL);
							if (timeStampAfter>=0L && timeStampHere>=0L
							&&  timeStampAfter<=timeStampHere)
								COMPLAIN2("DCheckNode: SYNC L%u: TIMESTAMP %ld REL TO NEXT IS WRONG.\n",
												pL, timeStampHere);
						}
					}
				}
				
				/* Do subobject count and voice-by-voice consistency checks. */
			
				for (v = 0; v<=MAXVOICES; v++) {
					vNotes[v] = vMainNotes[v] = 0;
					vTuplet[v] = False;
				}
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					vNotes[aNote->voice]++;
					if (MainNote(aNoteL)) vMainNotes[aNote->voice]++;
					vStaff[aNote->voice] = aNote->staffn;
					vlDur[aNote->voice] = aNote->subType;
					vlDots[aNote->voice] = aNote->ndots;
					vOct[aNote->voice] = aNote->inOttava;
					if (aNote->inTuplet) vTuplet[aNote->voice] = True;
				}

				for (v = 0; v<=MAXVOICES; v++) {
					if (vNotes[v]>0 && vMainNotes[v]!=1)
						COMPLAIN2("*DCheckNode: VOICE %d IN SYNC L%u HAS ZERO OR TWO OR MORE MainNotes.\n",
										v, pL);
					if (vTuplet[v])
						if (LVSearch(pL, TUPLETtype, v, True, False)==NILINK)
							COMPLAIN2("*DCheckNode: CAN'T FIND TUPLET FOR VOICE %d IN SYNC L%u.\n",
											v, pL);
				}
				
				/* Check multibar rests: if any voice has one, all voices in this Sync
				   must have the same thing. FIXME: SICheckMeasDur() for the Score Info
				   command implements a completely different consistency check on multibar
				   rests; that check should be done here too. */
				
				for (lDurCode = 0, v = 0; v<=MAXVOICES; v++)
					if (vNotes[v]>0) {
						if (lDurCode==0)
							lDurCode = vlDur[v];
						else {
							if ((lDurCode<WHOLEMR_L_DUR || vlDur[v]<WHOLEMR_L_DUR)
							&&   lDurCode!=vlDur[v])
								COMPLAIN3("DCheckNode: VOICE %d IN SYNC L%u (MEASURE %d) HAS MULTIBAR REST INCONSISTENCY.\n",
												v, pL, GetMeasNum(doc, pL));
						}
					}
				
				/* Check consistency of staff no. in chords, but complain about each no
				   more than once. */
				   
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (vStaff[aNote->voice]!=-999 && aNote->staffn!=vStaff[aNote->voice]) {
						COMPLAIN2("DCheckNode: VOICE %d IN SYNC L%u IS ON DIFFERENT STAVES.\n",
										aNote->voice, pL);
						vStaff[aNote->voice] = -999;
						}
				}

				/* Check consistency of durations in chords, but complain about each no
				   more than once. */
				   
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if ( (vlDur[aNote->voice]!=-999 && aNote->subType!=vlDur[aNote->voice])
					||   (vlDots[aNote->voice]!=-999 && aNote->ndots!=vlDots[aNote->voice]) ) {
						COMPLAIN2("*DCheckNode: VOICE %d IN SYNC L%u HAS VARYING DURATIONS.\n",
										aNote->voice, pL);
						vlDur[aNote->voice] = vlDots[aNote->voice] = -999;
						}
				}
									
				/* Check consistency of octave signs in chords, but complain about each
				   no more than once. */
				   
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (vOct[aNote->voice]!=-999 && aNote->inOttava!=vOct[aNote->voice]) {
						COMPLAIN2("DCheckNode: *VOICE %d IN SYNC L%u HAS VARYING inOttava FLAGS.\n",
										aNote->voice, pL);
						vOct[aNote->voice] = -999;
						}
				}

				/* Do all other checking on SYNCs. */
			
				PushLock(NOTEheap);
				
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (STAFFN_BAD(doc, aNote->staffn))
						COMPLAIN2("*DCheckNode: NOTE/REST IN SYNC L%u staffn %d IS BAD.\n",
										pL, aNote->staffn);
					if (VOICE_BAD(aNote->voice))
						COMPLAIN2("*DCheckNode: NOTE/REST IN SYNC L%u HAS BAD voice %d.\n",
										pL, aNote->voice);
					if (aNote->accident>5)
						COMPLAIN("*DCheckNode: NOTE/REST IN SYNC L%u HAS BAD ACCIDENTAL.\n", pL);

					if (!aNote->rest) {
						/* If noteNum is illegal, complain loudly; if it's simply outside
						   the range of the piano, warn more gently. */
					
						if (aNote->noteNum<1 || aNote->noteNum>MAX_NOTENUM) {
							COMPLAIN3("*DCheckNode: NOTE IN SYNC L%u IN MEASURE %d HAS BAD noteNum %d.\n",
											pL, GetMeasNum(doc, pL), aNote->noteNum);
						}						
						else if (aNote->noteNum<21 || aNote->noteNum>108) {
							COMPLAIN3("DCheckNode: Note in sync l%u in measure %d has suspicious noteNum %d.\n",
											pL, GetMeasNum(doc, pL), aNote->noteNum);
						}

						if (aNote->onVelocity>MAX_VELOCITY)
							COMPLAIN3("*DCheckNode: NOTE IN SYNC L%u IN MEASURE %d HAS BAD onVelocity %d.\n",
											pL, GetMeasNum(doc, pL), aNote->onVelocity);
						if (aNote->offVelocity>MAX_VELOCITY)
							COMPLAIN3("*DCheckNode: NOTE IN SYNC L%u IN MEASURE %d HAS BAD offVelocity %d.\n",
											pL, GetMeasNum(doc, pL), aNote->offVelocity);
						if (fullCheck) {
							if (aNote->onVelocity==0)
								COMPLAIN2("DCheckNode: Note in Sync L%u in measure %d has onVelocity zero.\n",
												pL, GetMeasNum(doc, pL));
							if (aNote->offVelocity==0)
								COMPLAIN2("DCheckNode: Note in Sync L%u in measure %d has offVelocity zero.\n",
												pL, GetMeasNum(doc, pL));
						}
					}
					
					if (aNote->beamed) {
						if (aNote->subType<=QTR_L_DUR)
							COMPLAIN("*DCheckNode: BEAMED QUARTER NOTE/REST OR LONGER IN SYNC L%u.\n", pL);
					}
					minLDur = (aNote->rest ? -127 : UNKNOWN_L_DUR);
					if (aNote->subType<minLDur || aNote->subType>MAX_L_DUR)
						COMPLAIN2("*DCheckNode: NOTE/REST WITH ILLEGAL l_dur IN VOICE %d IN SYNC L%u.\n",
										aNote->voice, pL);
					if ((aNote->inChord && vNotes[aNote->voice]<2)
					||  (!aNote->inChord && vNotes[aNote->voice]>=2))
						COMPLAIN2("*DCheckNode: NOTE/REST IN VOICE %d IN SYNC L%u HAS BAD inChord FLAG.\n",
										aNote->voice, pL);
					if (aNote->inChord && aNote->rest)
						COMPLAIN2("*DCheckNode: REST IN VOICE %d WITH inCHORD FLAG IN SYNC L%u.\n",
										aNote->voice, pL);

					DCheckSyncSlurs(doc, pL, aNoteL);
					
					if (aNote->firstMod) {
						unsigned short nObjs = MODNRheap->nObjs;
						aModNRL = aNote->firstMod;
						for ( ; aModNRL; aModNRL=NextMODNRL(aModNRL)) {
							if (aModNRL >= nObjs) {    /* very crude check on node validity  -JGG */
								COMPLAIN3("*DCheckNode: GARBAGE MODNR LINK %d IN VOICE %d IN SYNC L%u.\n",
												aModNRL, aNote->voice, pL);
								break;						/* prevent possible infinite loop */
							}
							aModNR = GetPAMODNR(aModNRL);
							if (!(aModNR->modCode>=MOD_FERMATA && aModNR->modCode<=MOD_LONG_INVMORDENT)
							&&	 aModNR->modCode>5)
								COMPLAIN3("DCheckNode: Illegal MODNR code %d in voice %d in Sync L%u.\n",
												aModNR->modCode, aNote->voice, pL);
						}
					}
				}
				PopLock(NOTEheap);
			}
				break;


			case PAGEtype:
				/* Arbitrarily assume no legitimate score is likely to have more than a
				   fixed number of pages. */
				   
				if (SheetNUM(pL)<0L || SheetNUM(pL)>MAX_PAGES)
						COMPLAIN2("DCheckNode: PAGE L%u sheetNum %ld IS NEGATIVE OR SUSPICIOUSLY LARGE.\n",
										pL, SheetNUM(pL));
				break;
			
			case SYSTEMtype: {
				PSYSTEM lSystem, rSystem;
					PushLock(SYSTEMheap);
					pSystem = GetPSYSTEM(pL);
					if (pSystem->lSystem) {
						lSystem = GetPSYSTEM(pSystem->lSystem);
						if (lSystem->rSystem!=pL)
							COMPLAIN2("•DCheckNode: SYSTEM L%u HAS INCONSISTENT LEFT SYSTEM LINK L%u.\n",
								pL, lSystem->rSystem);
					}
					if (pSystem->rSystem) {
						rSystem = GetPSYSTEM(pSystem->rSystem);
						if (rSystem->lSystem!=pL)
							COMPLAIN2("•DCheckNode: SYSTEM L%u HAS INCONSISTENT RIGHT SYSTEM LINK L%u.\n",
								pL, rSystem->lSystem);
					}
					if (GARBAGE_Q1RECT(pSystem->systemRect))
							COMPLAIN("*DCheckNode: SYSTEM L%u HAS GARBAGE systemRect.\n", pL);

					if (where!=CLIP_DSTR && d2pt(pSystem->systemRect.bottom)>doc->marginRect.bottom)
							COMPLAIN3("*DCheckNode: SYSTEM L%u RECT BELOW BOTTOM MARGIN (pt%d vs. %d).\n", pL,
								d2pt(pSystem->systemRect.bottom), doc->marginRect.bottom);
							
					/* The expression for checking systemRect.right has always involved
					   subtracting ExtraSysWidth(doc) , the width of the widest barline
					   (final double). That's not enough for documents that have had their
					   staff size reduced because ExtraSysWidth() is smaller, but
					   SetStaffSizeMP() doesn't shorten systemRects. Probably not hard to
					   fix, but subtracting 2*ExtraSysWidth(doc) should sidestep the
					   problem with negligible side effects. --DAB, 10/2015 */
						
					if (where!=CLIP_DSTR && d2pt(pSystem->systemRect.right-2*ExtraSysWidth(doc))>doc->marginRect.right)
							COMPLAIN3("*DCheckNode: SYSTEM L%u RECT EXTENDS INTO RIGHT MARGIN (pt%d vs. %d).\n", pL,
								d2pt(pSystem->systemRect.right), doc->marginRect.right);
//LogPrintf(LOG_DEBUG, "sysR.right=%d=%d pt, ESWidth=%d, margR.right=%d\n", pSystem->systemRect.right, d2pt(pSystem->systemRect.right),
//ExtraSysWidth(doc), doc->marginRect.right);

					if (pSystem->lSystem) {
						lSystem = GetPSYSTEM(pSystem->lSystem);				
						if (pSystem->pageL==lSystem->pageL
						&&  pSystem->systemRect.top<lSystem->systemRect.bottom)
							COMPLAIN("*DCheckNode: SYSTEM L%u RECT NOT ENTIRELY BELOW PREVIOUS SYSTEM.\n", pL);
					}
					if (pSystem->rSystem) {
						rSystem = GetPSYSTEM(pSystem->rSystem);				
						if (pSystem->pageL==rSystem->pageL
						&&  pSystem->systemRect.bottom>rSystem->systemRect.top)
							COMPLAIN("*DCheckNode: SYSTEM L%u RECT NOT ENTIRELY ABOVE FOLLOWING SYSTEM.\n", pL);
					}
					PopLock(SYSTEMheap);
				}
				break;
				
			case STAFFtype: {
				PSTAFF lStaff, rStaff;
				DDIST sysWidth, sysHeight;
				Boolean isFirstStaff;
				
					pStaff = GetPSTAFF(pL);
					if (pStaff->lStaff) {
						lStaff = GetPSTAFF(pStaff->lStaff);
						if (lStaff->rStaff != pL)
							COMPLAIN("•DCheckNode: STAFF L%u LEFT STAFF HAS INCONSISTENT RIGHT LINK.\n", pL);
					}
					if (pStaff->rStaff) {
						rStaff = GetPSTAFF(pStaff->rStaff);
						if (rStaff->lStaff != pL)
							COMPLAIN("•DCheckNode: STAFF L%u RIGHT STAFF HAS INCONSISTENT LEFT LINK.\n", pL);
					}
					isFirstStaff = !pStaff->lStaff;

					pSystem = GetPSYSTEM(pStaff->systemL);
					sysWidth = pSystem->systemRect.right-pSystem->systemRect.left;
					sysHeight = pSystem->systemRect.bottom-pSystem->systemRect.top;

					for (aStaffL=FirstSubLINK(pL); aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
						if (STAFFN_BAD(doc, StaffSTAFF(aStaffL)))
							COMPLAIN2("*DCheckNode: SUBOBJ IN STAFF L%u staffn %d IS BAD.\n",
											pL, StaffSTAFF(aStaffL));
						aStaff = GetPASTAFF(aStaffL);
						if (isFirstStaff) {
							DDIST lnHeight = aStaff->staffHeight/(aStaff->staffLines-1);
							DDIST srLnHeight = drSize[doc->srastral]/(STFLINES-1);
#ifdef NOTYET
							DDIST altsrLnHeight = drSize[doc->altsrastral]/(STFLINES-1);
							if (lnHeight!=srLnHeight && lnHeight!=altsrLnHeight)
								COMPLAIN2("*DCheckNode: SUBOBJ IN FIRST STAFF, L%u (staffn=%d), staffHeight DISAGREES WITH s/altsrastral.\n",
												pL, aStaff->staffn);
#else
							if (lnHeight!=srLnHeight)
								COMPLAIN2("*DCheckNode: SUBOBJ IN FIRST STAFF, L%u (staffn=%d), staffHeight DISAGREES WITH srastral.\n",
												pL, aStaff->staffn);
#endif
						}
						if (aStaff->staffLeft<0 || aStaff->staffLeft>sysWidth)
							COMPLAIN("*DCheckNode: SUBOBJ IN STAFF L%u staffLeft IS ILLEGAL.\n", pL);
						if (aStaff->staffRight<0 || aStaff->staffRight>sysWidth)
							COMPLAIN("*DCheckNode: SUBOBJ IN STAFF L%u staffRight IS ILLEGAL.\n", pL);

						if (!aStaff->visible) {
							if (aStaff->spaceBelow<=0)
								COMPLAIN2("*DCheckNode: SUBOBJ IN STAFF L%u (staffn=%d) spaceBelow IS ILLEGAL.\n",
												pL, aStaff->staffn);
							if (aStaff->spaceBelow>sysHeight-aStaff->staffHeight)
								COMPLAIN2("*DCheckNode: subobj in staff L%u (staffn=%d) spaceBelow is suspicious.\n",
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
							COMPLAIN2("DCheckNode: MEASURE %d (L%u) HAS A GARBAGE BBOX.\n", 
										GetMeasNum(doc, pL), pL);
						}
						else
							 if (DCheckMBBox(doc, pL, abnormal)) bad = True;
						
						if (pMeasure->lMeasure) {
							lMeasure = GetPMEASURE(pMeasure->lMeasure);
							
						/* Can't use standard Search here because it assumes links are OK. */
						
							for (qL = LeftLINK(pL); qL; qL = LeftLINK(qL))
								if (ObjLType(qL)==MEASUREtype) break;
							if (!qL)
								{ COMPLAIN("•DCheckNode: MEASURE L%u HAS A BAD MEASURE LLINK.\n", pL); }
							else if (qL!=pMeasure->lMeasure)
								{ COMPLAIN("•DCheckNode: MEASURE L%u HAS INCONSISTENT MEASURE LLINK.\n", pL); }
							else if (LinkVALID(pL)
								  &&  pMeasure->systemL==lMeasure->systemL
								  &&  pMeasure->measureBBox.left!=lMeasure->measureBBox.right) {
								COMPLAIN3("DCheckNode: MEASURE %d (L%u) BBOX DISAGREES WITH PREVIOUS MEASURE BY %d.\n",
												GetMeasNum(doc, pL), pL,
												pMeasure->measureBBox.left-lMeasure->measureBBox.right);
							}
						}
	
						if (pMeasure->rMeasure) {
							rMeasure = GetPMEASURE(pMeasure->rMeasure);
							
						/* Can't use standard Search here because it assumes links are OK. */
						
							for (qL = RightLINK(pL); qL; qL = RightLINK(qL))
								if (ObjLType(qL)==MEASUREtype) break;
							if (!qL)
								{ COMPLAIN("•DCheckNode: MEASURE L%u HAS A BAD MEASURE RLINK.\n", pL); }
							else if (qL!=pMeasure->rMeasure)
								{ COMPLAIN("•DCheckNode: MEASURE L%u HAS INCONSISTENT MEASURE RLINK.\n", pL); }
							else if (LinkVALID(pL)
								  &&  pMeasure->systemL==rMeasure->systemL
								  &&  pMeasure->measureBBox.right!=rMeasure->measureBBox.left) {
								COMPLAIN3("DCheckNode: MEASURE %d (L%u) BBOX DISAGREES WITH NEXT MEASURE BY %d.\n",
												GetMeasNum(doc, pL), pL,
												pMeasure->measureBBox.right-rMeasure->measureBBox.left);
							}
						}
	
						if (DBadLink(doc, OBJtype, pMeasure->systemL, True)) {
							COMPLAIN("•DCheckNode: MEASURE L%u HAS GARBAGE SYSTEM LINK.\n", pL);
						}
						else {
						/* Can't use standard Search here because it assumes links are OK. */
							for (qL = LeftLINK(pL); qL; qL = LeftLINK(qL))
								if (ObjLType(qL)==SYSTEMtype) break;
							if (!qL)
								{ COMPLAIN("•DCheckNode: MEASURE L%u HAS A BAD SYSTEM LINK.\n", pL); }
							else if (qL!=pMeasure->systemL)
								COMPLAIN("•DCheckNode: MEASURE L%u HAS INCONSISTENT SYSTEM LINK.\n", pL);
							}

						if (DBadLink(doc, OBJtype, pMeasure->staffL, True)) {
							COMPLAIN("•DCheckNode: MEASURE L%u HAS GARBAGE STAFF LINK.\n", pL);
						}
						else {
						/* Can't use standard Search here because it assumes links are OK. */
							for (qL = LeftLINK(pL); qL; qL = LeftLINK(qL))
								if (ObjLType(qL)==STAFFtype) break;
							if (!qL)
								{ COMPLAIN("•DCheckNode: MEASURE L%u HAS A BAD STAFF LINK.\n", pL); }
							else if (qL!=pMeasure->staffL)
								COMPLAIN("•DCheckNode: MEASURE L%u HAS INCONSISTENT STAFF LINK.\n", pL);
							}
					}
	
					/* Arbitrarily assume no legitimate score is likely to be longer
					   than a fixed length. */
					   
					if (pMeasure->lTimeStamp<0L
					||  pMeasure->lTimeStamp>MAX_WHOLES_DUR*1920)
							COMPLAIN2("DCheckNode: MEASURE L%u lTimeStamp %ld IS NEGATIVE OR SUSPICIOUSLY LARGE.\n",
											pL, pMeasure->lTimeStamp);

					if (pMeasure->spacePercent<MINSPACE
					||  pMeasure->spacePercent>MAXSPACE)
							COMPLAIN2("*DCheckNode: MEASURE L%u HAS ILLEGAL spacePercent %d.\n",
											pL, pMeasure->spacePercent);
					
					DCheckMeasSubobjs(doc, pL, abnormal);
					
					syncL = SSearch(pL, SYNCtype, GO_RIGHT);
					if (syncL) {
						timeStamp = SyncTIME(syncL);
						if (timeStamp!=0L && where!=CLIP_DSTR)
							COMPLAIN2("DCheckNode: SYNC L%u IS FIRST OF MEASURE BUT ITS timeStamp IS %ld.\n",
											syncL, timeStamp);
					}

					PopLock(MEASUREheap);
				}
				break;

			case PSMEAStype:
				PushLock(PSMEASheap);
					for (aPSMeasL = FirstSubLINK(pL); aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL)) {
						aPSMeas = GetPAPSMEAS(aPSMeasL);
						if (STAFFN_BAD(doc, aPSMeas->staffn))
							COMPLAIN2("*DCheckNode: SUBOBJ IN PSEUDOMEAS L%u staffn %d IS BAD.\n",
										pL, aPSMeas->staffn);
						if (aPSMeas->staffn==1 && aPSMeas->connAbove)
							COMPLAIN("*DCheckNode: SUBOBJ ON STAFF 1 IN PSEUDOMEAS L%u HAS connAbove.\n", pL);
						if (aPSMeas->connStaff>0 && (aPSMeas->connStaff<=aPSMeas->staffn || aPSMeas->connStaff>doc->nstaves))
							COMPLAIN2("*DCheckNode: SUBOBJ ON STAFF %d IN PSEUDOMEAS L%u HAS BAD connStaff.\n",
											aPSMeas->staffn, pL);
					}
				PopLock(PSMEASheap);
				break;
				
			case CLEFtype:
				for (aClefL=FirstSubLINK(pL); aClefL; aClefL=NextCLEFL(aClefL)) {
					if (STAFFN_BAD(doc, ClefSTAFF(aClefL)))
						COMPLAIN2("*DCheckNode: SUBOBJ IN CLEF L%u staffn %d IS BAD.\n",
									pL, ClefSTAFF(aClefL));
					if (ClefType(aClefL)<LOW_CLEF || ClefType(aClefL)>HIGH_CLEF)
						COMPLAIN2("*DCheckNode: SUBOBJ IN CLEF L%u subType %d IS BAD.\n", pL, ClefType(aClefL));
				}
				break;
			
			case KEYSIGtype:
				PushLock(KEYSIGheap);

				for (aKeySigL=FirstSubLINK(pL); aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
					aKeySig = GetPAKEYSIG(aKeySigL);
					if (STAFFN_BAD(doc, aKeySig->staffn))
						COMPLAIN2("*DCheckNode: SUBOBJ IN KEYSIG L%u staffn %d IS BAD.\n",
									pL, aKeySig->staffn);
					if (aKeySig->nKSItems>7) {
						COMPLAIN3("*DCheckNode: subobj in keysig L%u staffn %d nKSItems %d is suspicious.\n",
									pL, aKeySig->staffn, aKeySig->nKSItems);
					} else if (aKeySig->nKSItems==0 && aKeySig->subType>7) {
						COMPLAIN3("*DCheckNode: subobj in keysig L%u staffn %d subType (nNatItems) %d is suspicious.\n",
									pL, aKeySig->staffn, aKeySig->nKSItems);
					} else {
						short k;  char tempStr[256];
						tempStr[0] = '\0';								/* Set tempStr to empty */
						for (k = 0; k<aKeySig->nKSItems; k++) 			
							if (aKeySig->KSItem[k].letcode>6)
								sprintf(&tempStr[strlen(tempStr)], " %d", k);
						if (strlen(tempStr)>0)
							COMPLAIN3("*DCheckNode: ILLEGAL LETTER CODES FOR SUBOBJ IN KEYSIG L%u staffn %d ACCIDENTAL NO(S).: %s.\n",
								pL, aKeySig->staffn, tempStr);
					}
				}

				PopLock(KEYSIGheap);
				break;
				
			case TIMESIGtype:
				PushLock(TIMESIGheap);

				for (aTimeSigL=FirstSubLINK(pL); aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL)) {
					aTimeSig = GetPATIMESIG(aTimeSigL);		
					if (STAFFN_BAD(doc, aTimeSig->staffn))
						COMPLAIN2("*DCheckNode: SUBOBJ IN TIMESIG L%u staffn %d IS BAD.\n",
								pL, aTimeSig->staffn);
					if (TSTYPE_BAD(aTimeSig->subType))
						COMPLAIN3("*DCheckNode: SUBOBJ IN TIMESIG L%u staffn %d HAS BAD timeSigType %d.\n",
								pL, aTimeSig->staffn, aTimeSig->subType);
					if (TSNUMER_BAD(aTimeSig->numerator))
						COMPLAIN3("*DCheckNode: SUBOBJ IN TIMESIG L%u staffn %d HAS BAD TIMESIG numerator %d.\n",
								pL, aTimeSig->staffn, aTimeSig->numerator);
					if (TSDENOM_BAD(aTimeSig->denominator))
						COMPLAIN3("*DCheckNode: SUBOBJ IN TIMESIG L%u staffn %d  HAS BAD TIMESIG denominator %d.\n",
								pL, aTimeSig->staffn, aTimeSig->denominator);
					if (TSDUR_BAD(aTimeSig->numerator, aTimeSig->denominator))
						COMPLAIN2("*DCheckNode: SUBOBJ IN TIMESIG L%u staffn %d HAS TOO GREAT DURATION.\n",
								pL, aTimeSig->staffn);
				}

				PopLock(TIMESIGheap);
				break;

			case BEAMSETtype:
				LINK lastSyncL;
				PushLock(NOTEBEAMheap);
				
				bad = DCheckBeamset(doc, pL, False, False, &lastSyncL);
				
				PopLock(NOTEBEAMheap);
				break;
			
			case TUPLETtype:
				PushLock(NOTETUPLEheap);
				
				if (STAFFN_BAD(doc, ((PTUPLET)p)->staffn))
						COMPLAIN2("*DCheckNode: TUPLET L%u staffn %d IS BAD.\n",
									pL, ((PTUPLET)p)->staffn);
				if (VOICE_BAD(((PTUPLET)p)->voice))
						COMPLAIN("*DCheckNode: TUPLET L%u HAS BAD voice.\n", pL);

				for (aNoteTupleL=FirstSubLINK(pL);	aNoteTupleL;
						aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
					aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
					if (DBadLink(doc, OBJtype, aNoteTuple->tpSync, True)) {
						COMPLAIN2("•DCheckNode: TUPLET L%u HAS GARBAGE SYNC LINK %d.\n",
										pL, aNoteTuple->tpSync);
					}
					else if (ObjLType(aNoteTuple->tpSync)!=SYNCtype)
						COMPLAIN2("•DCheckNode: TUPLET L%u HAS BAD SYNC LINK %d.\n",
										pL, aNoteTuple->tpSync);
				}

				PopLock(NOTETUPLEheap);
				break;
			
			case CONNECTtype:
				PushLock(CONNECTheap);

				for (aConnectL=FirstSubLINK(pL); aConnectL;
						aConnectL=NextCONNECTL(aConnectL)) {
					aConnect = GetPACONNECT(aConnectL);
					if (aConnect->connLevel!=0) {
						if (STAFFN_BAD(doc, aConnect->staffAbove))
							COMPLAIN2("*DCheckNode: SUBOBJ IN CONNECT L%u staffAbove %d IS BAD.\n",
											pL, aConnect->staffAbove);
						
						if (STAFFN_BAD(doc, aConnect->staffBelow))
							COMPLAIN2("*DCheckNode: SUBOBJ IN CONNECT L%u staffBelow %d IS BAD.\n",
											pL, aConnect->staffBelow);
						if (aConnect->staffAbove>=aConnect->staffBelow)
							COMPLAIN("*DCheckNode: SUBOBJ IN CONNECT L%u staffAbove>=staffBelow.\n", pL);
					}
				}

				PopLock(CONNECTheap);
				break;

			case DYNAMtype:
				PushLock(DYNAMheap);
				pDynamic = GetPDYNAMIC(pL);
				if (DBadLink(doc, OBJtype, pDynamic->firstSyncL, False)) {
					COMPLAIN("•DCheckNode: DYNAMIC L%u HAS GARBAGE firstSyncL.\n", pL);
				}
				else {
					if (ObjLType(pDynamic->firstSyncL)!=SYNCtype) {
						COMPLAIN("•DCheckNode: DYNAMIC L%u firstSyncL IS NOT A SYNC.\n", pL);
					}
				}

				if (DBadLink(doc, OBJtype, pDynamic->lastSyncL, !IsHairpin(pL))) {
					COMPLAIN("•DCheckNode: DYNAMIC L%u HAS GARBAGE lastSyncL.\n", pL);
				}

				for (aDynamicL=FirstSubLINK(pL); aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL)) {
					if (STAFFN_BAD(doc, DynamicSTAFF(aDynamicL))) {
						COMPLAIN2("*DCheckNode: SUBOBJ IN DYNAMIC L%u staffn %d IS BAD.\n",
									pL, DynamicSTAFF(aDynamicL));
					}
					else {
						if (!ObjOnStaff(pDynamic->firstSyncL, DynamicSTAFF(aDynamicL), False))
							COMPLAIN("*DCheckNode: DYNAMIC L%u firstSyncL HAS NO NOTES ON ITS STAFF.\n", pL);
						if (IsHairpin(pL) && !ObjOnStaff(pDynamic->lastSyncL, DynamicSTAFF(aDynamicL), False))
							COMPLAIN("*DCheckNode: DYNAMIC L%u lastSyncL HAS NO NOTES ON ITS STAFF.\n", pL);
					}
				}
				PopLock(DYNAMheap);
				break;
				
			case OTTAVAtype:
				PushLock(NOTEOTTAVAheap);
				if (STAFFN_BAD(doc, ((POTTAVA)p)->staffn))
					COMPLAIN2("*DCheckNode: OTTAVA L%u staffn %d IS BAD.\n", pL,
								((POTTAVA)p)->staffn);

				if (p->nEntries<=1 && ((POTTAVA)p)->xdFirst>((POTTAVA)p)->xdLast)
					COMPLAIN2("DCheckNode: Ottava L%u on staffn %d xd1st >= xdLast.\n",
								pL, ((POTTAVA)p)->staffn);
								
				for (aNoteOctL=FirstSubLINK(pL); aNoteOctL; aNoteOctL=NextNOTEOTTAVAL(aNoteOctL)) {
					aNoteOct = GetPANOTEOTTAVA(aNoteOctL);
					if (DBadLink(doc, OBJtype, aNoteOct->opSync, True)) {
						COMPLAIN2("•DCheckNode: OTTAVA L%u HAS GARBAGE SYNC LINK %u.\n",
										pL, aNoteOct->opSync);
					}
					else if (!SyncTYPE(aNoteOct->opSync) && !GRSyncTYPE(aNoteOct->opSync))
						COMPLAIN2("•DCheckNode: OTTAVA L%u HAS BAD SYNC LINK %u.\n",
										pL, aNoteOct->opSync);
				}
				
				PopLock(NOTEOTTAVAheap);
				break;

			case SLURtype: {
				short theInd, firstNoteNum=-1, lastNoteNum=-1;
				LINK firstNoteL, lastNoteL;
				Boolean foundFirst, foundLast;
				
				PushLock(SLURheap);
				pSlur = GetPSLUR(pL);

				if (STAFFN_BAD(doc, pSlur->staffn))
					COMPLAIN("*DCheckNode: SLUR L%u HAS BAD staffn.\n", pL);
				if (VOICE_BAD(pSlur->voice))
					COMPLAIN("*DCheckNode: SLUR L%u HAS BAD voice.\n", pL);
				if (!pSlur->tie && p->nEntries>1)
					COMPLAIN("*DCheckNode: NON-TIE SLUR L%u WITH MORE THAN ONE SUBOBJECT.\n", pL);
				if (DBadLink(doc, OBJtype, pSlur->firstSyncL, True)
				||  DBadLink(doc, OBJtype, pSlur->lastSyncL, True)) {
					COMPLAIN("•DCheckNode: SLUR L%u HAS GARBAGE SYNC LINK.\n", pL);
					break;
				}

				if (!pSlur->crossSystem) {
					if (ObjLType(pSlur->firstSyncL)!=SYNCtype
					||  ObjLType(pSlur->lastSyncL)!=SYNCtype) {
						COMPLAIN("*DCheckNode: NON-CROSS-SYS SLUR L%u firstSync OR lastSync ISN'T A SYNC.\n", pL);
						break;
					}
				}
				else {
					if (ObjLType(pSlur->firstSyncL)!=MEASUREtype
					&&  ObjLType(pSlur->lastSyncL)!=SYSTEMtype) {
						COMPLAIN("*DCheckNode: CROSS-SYS SLUR L%u firstSync ISN'T A MEASURE AND lastSync ISN'T A SYSTEM.\n", pL);
						break;
					}
				}
				
				if (ObjLType(pSlur->firstSyncL)==SYNCtype
				&&  !SyncInVoice(pSlur->firstSyncL, pSlur->voice)) {
					COMPLAIN("*DCheckNode: SLUR L%u firstSync HAS NOTHING IN SLUR'S VOICE.\n", pL);
					break;
				}

				if (ObjLType(pSlur->lastSyncL)==SYNCtype
				&&  !SyncInVoice(pSlur->lastSyncL, pSlur->voice)) {
					COMPLAIN("*DCheckNode: SLUR L%u lastSync HAS NOTHING IN SLUR'S VOICE.\n", pL);
					break;
				}

				for (aSlurL=FirstSubLINK(pL); aSlurL; aSlurL=NextSLURL(aSlurL)) {
					aSlur = GetPASLUR(aSlurL);
					if (pSlur->tie) {
						/* firstInd and lastInd are used only for ties. */
						
						if (aSlur->firstInd<0 || aSlur->firstInd>=MAXCHORD
						||  aSlur->lastInd<0  || aSlur->lastInd>=MAXCHORD)
							COMPLAIN("*DCheckNode: TIE L%u HAS ILLEGAL NOTE INDEX.\n", pL);

						if (!pSlur->crossSystem) {
							theInd = -1;
							for (firstNoteL = FirstSubLINK(pSlur->firstSyncL); firstNoteL;
									firstNoteL = NextNOTEL(firstNoteL)) {
								if (NoteVOICE(firstNoteL)==pSlur->voice) {
									theInd++;
									if (theInd==aSlur->firstInd) {
										firstNoteNum = NoteNUM(firstNoteL);
										break;
									}
								}
							}
							theInd = -1;
							for (lastNoteL = FirstSubLINK(pSlur->lastSyncL); lastNoteL;
									lastNoteL = NextNOTEL(lastNoteL)) {
								if (NoteVOICE(lastNoteL)==pSlur->voice) {
									theInd++;
									if (theInd==aSlur->lastInd) {
										lastNoteNum = NoteNUM(lastNoteL);
										break;
									}
								}
							}
							if (firstNoteL && !NoteTIEDR(firstNoteL))
								COMPLAIN3("*DCheckNode: TIE IN VOICE %d L%u IN MEASURE %d FIRST NOTE NOT tiedR.\n",
											SlurVOICE(pL), pL, GetMeasNum(doc, pL));
							if (lastNoteL && !NoteTIEDL(lastNoteL))
								COMPLAIN3("*DCheckNode: TIE IN VOICE %d L%u IN MEASURE %d LAST NOTE NOT tiedL.\n",
											SlurVOICE(pL), pL, GetMeasNum(doc, pL));
							if (firstNoteNum!=lastNoteNum)
								COMPLAIN3("*DCheckNode: TIE IN VOICE %d L%u IN MEASURE %d ON NOTES WITH DIFFERENT noteNum.\n",
											SlurVOICE(pL), pL, GetMeasNum(doc, pL));
						}
					}
						
					else {												/* Real slur, not set of ties */
						if (!pSlur->crossSystem) {
							foundFirst = False;
							for (firstNoteL = FirstSubLINK(pSlur->firstSyncL); firstNoteL;
									firstNoteL = NextNOTEL(firstNoteL)) {
								if (NoteVOICE(firstNoteL)==pSlur->voice) {
									if (NoteSLURREDR(firstNoteL)) foundFirst = True;
								}
							}
							foundLast = False;
							for (lastNoteL = FirstSubLINK(pSlur->lastSyncL); lastNoteL;
									lastNoteL = NextNOTEL(lastNoteL)) {
								if (NoteVOICE(lastNoteL)==pSlur->voice) {
									if (NoteSLURREDL(lastNoteL)) foundLast = True;
								}
							}
							if (!foundFirst)
									COMPLAIN3("*DCheckNode: SLUR IN VOICE %d L%u IN MEASURE %d FIRST NOTE NOT slurredR.\n",
												SlurVOICE(pL), pL, GetMeasNum(doc, pL));
							if (!foundLast)
									COMPLAIN3("*DCheckNode: SLUR IN VOICE %d L%u IN MEASURE %d LAST NOTE NOT slurredL.\n",
												SlurVOICE(pL), pL, GetMeasNum(doc, pL));
						}
					}
				}

				PopLock(SLURheap);
				}
				break;
			
			case GRAPHICtype:
				{
					LINK aGraphicL;  PAGRAPHIC aGraphic;
					PGRAPHIC pGraphic;	short len;

					PushLock(GRAPHICheap);
	 				pGraphic = GetPGRAPHIC(pL);
					if (DBadLink(doc, OBJtype, pGraphic->firstObj, False))
						COMPLAIN("•DCheckNode: GRAPHIC L%u HAS GARBAGE firstObj.\n", pL);
					if (!PageTYPE(pGraphic->firstObj))
						if (STAFFN_BAD(doc, pGraphic->staffn)) {
							COMPLAIN2("*DCheckNode: GRAPHIC L%u HAS BAD staffn %d.\n", pL, pGraphic->staffn);
						}
					else {
						if (!ObjOnStaff(pGraphic->firstObj, pGraphic->staffn, False))
							COMPLAIN2("*DCheckNode: GRAPHIC L%u firstObj HAS NO SUBOBJS ON ITS STAFF %d.\n",
											pL, pGraphic->staffn);
					}
 
					switch (pGraphic->graphicType) {
						case GRPICT:
							COMPLAIN("DCheckNode: GRAPHIC L%u is a GRPICT, BUT GRPICTs aren't implemented.\n",
										pL);
							break;
						case GRString:
						case GRLyric:
						case GRRehearsal:
						case GRChordSym:
							if (doc!=clipboard && pGraphic->fontInd>=doc->nfontsUsed)
								COMPLAIN2("*DCheckNode: STRING GRAPHIC L%u HAS BAD FONT NUMBER %d.\n",
														pL, pGraphic->fontInd);
							if (pGraphic->relFSize &&
								(pGraphic->fontSize<GRTiny || pGraphic->fontSize>GRLastSize))
									COMPLAIN2("*DCheckNode: STRING GRAPHIC L%u HAS BAD RELATIVE SIZE %d.\n",
														pL, pGraphic->fontSize);
							aGraphicL = FirstSubLINK(pL);
							aGraphic = GetPAGRAPHIC(aGraphicL);
							if (aGraphic->strOffset<0L
							|| aGraphic->strOffset>=GetHandleSize((Handle)doc->stringPool)) {
								COMPLAIN("*DCheckNode: STRING GRAPHIC L%u string IS BAD.\n", pL);
							}
							else if (!aGraphic->strOffset || PCopy(aGraphic->strOffset)==NULL) {
								COMPLAIN("*DCheckNode: STRING GRAPHIC L%u HAS NO STRING.\n", pL);
							}
							else {
								len = (Byte)(*PCopy(aGraphic->strOffset));
								if (len==0) COMPLAIN("*DCheckNode: STRING GRAPHIC L%u HAS AN EMPTY STRING.\n",
															pL);
							}
							break;
						case GRDraw:
							if (DBadLink(doc, OBJtype, pGraphic->lastObj, False))
								COMPLAIN("•DCheckNode: GRDraw GRAPHIC L%u HAS GARBAGE lastObj.\n", pL);
							if (!ObjOnStaff(pGraphic->lastObj, pGraphic->staffn, False))
								COMPLAIN2("*DCheckNode: GRDraw GRAPHIC L%u lastObj HAS NO SUBOBJS ON ITS STAFF %d.\n",
												pL, pGraphic->staffn);
							if (pGraphic->gu.thickness<10 || pGraphic->gu.thickness>150)
								COMPLAIN2("DCheckNode: GRDraw GRAPHIC L%u thickness of %d is suspicious.\n",
												pL, pGraphic->gu.thickness);
							break;
						default:
							;
					}
					PopLock(GRAPHICheap);
				}
				break;
				
			case TEMPOtype:
				{
					PTEMPO pTempo; short len;
					
					PushLock(TEMPOheap);
					if (DBadLink(doc, OBJtype, ((PTEMPO)p)->firstObjL, False))
						COMPLAIN("•DCheckNode: TEMPO L%u HAS GARBAGE firstObjL.\n", pL);
						
	 				pTempo = GetPTEMPO(pL);
					if (!PageTYPE(pTempo->firstObjL))
						if (STAFFN_BAD(doc, pTempo->staffn)) {
							COMPLAIN("*DCheckNode: TEMPO L%u HAS BAD staffn.\n", pL);
						}
					else {
					if (!ObjOnStaff(pTempo->firstObjL, pTempo->staffn, False))
						COMPLAIN("*DCheckNode: TEMPO L%u firstObjL HAS NO SUBOBJS ON ITS STAFF.\n",
										pL);
					}
	
					/* A Tempo's <string> can be empty, but its <metroStr> can't be. */
					 
					if (pTempo->strOffset<0L
					|| pTempo->strOffset>=GetHandleSize((Handle)doc->stringPool)) {
						COMPLAIN("*DCheckNode: TEMPO L%u string IS BAD.\n", pL);
					}

					if (pTempo->metroStrOffset<0L
					|| pTempo->metroStrOffset>=GetHandleSize((Handle)doc->stringPool)) {
						COMPLAIN("*DCheckNode: TEMPO L%u metroStr IS BAD.\n", pL);
					}
					else if (!pTempo->metroStrOffset || PCopy(pTempo->metroStrOffset)==NULL) {
						COMPLAIN("*DCheckNode: TEMPO L%u HAS NO metroStr.\n", pL);
					}
					else {
						len = (Byte)(*PCopy(pTempo->metroStrOffset));
						if (len==0) COMPLAIN("*DCheckNode: TEMPO L%u HAS AN EMPTY metroStr.\n",
													pL);
					}
					PopLock(TEMPOheap);
				}
				break;


			case SPACERtype:
					PushLock(SPACERheap);
					if (STAFFN_BAD(doc, SpacerSTAFF(pL))) {
						COMPLAIN2("*DCheckNode: SPACER L%u staffn %d IS BAD.\n", pL,
									SpacerSTAFF(pL));
						PopLock(SPACERheap);
						break;
					}
				break;
				
			case ENDINGtype:
				{
					PENDING pEnding;
					
					PushLock(ENDINGheap);
					if (STAFFN_BAD(doc, EndingSTAFF(pL))) {
						COMPLAIN2("*DCheckNode: ENDING L%u staffn %d IS BAD.\n", pL, EndingSTAFF(pL));
						break;
					}
					
					pEnding = GetPENDING(pL);
					if (DBadLink(doc, OBJtype, pEnding->firstObjL, False)) {
						COMPLAIN("•DCheckNode: ENDING L%u HAS GARBAGE firstObjL.\n", pL);
					}
					else if (!ObjOnStaff(pEnding->firstObjL, EndingSTAFF(pL), False))
						COMPLAIN("*DCheckNode: ENDING L%u firstObjL HAS NO SUBOBJS ON ITS STAFF.\n", pL);

					if (DBadLink(doc, OBJtype, pEnding->lastObjL, False)) {
						COMPLAIN("•DCheckNode: ENDING L%u HAS GARBAGE lastObjL.\n", pL);
					}
					else if (!ObjOnStaff(pEnding->lastObjL, EndingSTAFF(pL), False))
						COMPLAIN("*DCheckNode: ENDING L%u lastObjL HAS NO SUBOBJS ON ITS STAFF.\n", pL);

					if (pEnding->endNum>MAX_ENDING_STRINGS) {
						COMPLAIN2("*DCheckNode: ENDING L%u HAS BAD endNum %d.\n", pL,
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


/* --------------------------------------------------------------------- DCheckNodeSel -- */
/* Do consistency check on selection status between object and subobject: if object is
not selected, no subobjects should be selected. Does not check the other type of
consistency, namely if object is selected, at least one subobject should be. Returns
True if it finds a problem. */

Boolean DCheckNodeSel(Document *doc, LINK pL)
{
	LINK	aNoteL, aMeasL, aClefL, aKeySigL, aTimeSigL, aDynamicL,
			aStaffL, aConnectL, aSlurL;
	Boolean	bad;

	if (pL==doc->headL || pL==doc->tailL) return False;
	if (pL==doc->masterHeadL || pL==doc->masterTailL) return False;
	if (LinkSEL(pL)) return False;
	
	bad = False;

	if (TYPE_BAD(pL) || ObjLType(pL)==HEADERtype || ObjLType(pL)==TAILtype) {
			COMPLAIN("•DCheckNodeSel: Object L%u HAS BAD type.\n", pL);
			return bad;
	}

	switch (ObjLType(pL)) {
		case SYNCtype:
			for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL))
					COMPLAIN2("DCheckNodeSel: SELECTED NOTE IN VOICE %d IN UNSELECTED SYNC L%u.\n",
									NoteVOICE(aNoteL), pL);
			break;
		case MEASUREtype:
			aMeasL = FirstSubLINK(pL);
			for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL))
				if (MeasureSEL(aMeasL))
					COMPLAIN2("DCheckNodeSel: SELECTED ITEM ON STAFF %d IN UNSELECTED MEASURE L%u.\n",
									MeasureSTAFF(aMeasL), pL);
			break;
		case CLEFtype:
			for (aClefL = FirstSubLINK(pL); aClefL;  aClefL = NextCLEFL(aClefL))
				if (ClefSEL(aClefL))
					COMPLAIN2("DCheckNodeSel: SELECTED ITEM ON STAFF %d IN UNSELECTED CLEF L%u.\n",
									ClefSTAFF(aClefL), pL);
			break;
		case KEYSIGtype:
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
				if (KeySigSEL(aKeySigL))
					COMPLAIN2("DCheckNodeSel: SELECTED ITEM ON STAFF %d IN UNSELECTED KEYSIG L%u.\n",
									KeySigSTAFF(aKeySigL), pL);
			break;
		case TIMESIGtype:
			aTimeSigL = FirstSubLINK(pL);
			for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
				if (TimeSigSEL(aTimeSigL))
					COMPLAIN2("DCheckNodeSel: SELECTED ITEM ON STAFF %d IN UNSELECTED TIMESIG L%u.\n",
									TimeSigSTAFF(aTimeSigL), pL);
			break;
		case DYNAMtype:
			aDynamicL = FirstSubLINK(pL);
			for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL))
				if (DynamicSEL(aDynamicL))
					COMPLAIN("DCheckNodeSel: SELECTED ITEM IN UNSELECTED DYNAMIC L%u.\n", pL);
			break;
		case STAFFtype:
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
				if (StaffSEL(aStaffL))
					COMPLAIN("DCheckNodeSel: SELECTED ITEM IN UNSELECTED STAFF L%u.\n", pL);
			break;
		case CONNECTtype:
			aConnectL = FirstSubLINK(pL);
			for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL))
				if (ConnectSEL(aConnectL))
					COMPLAIN("DCheckNodeSel: SELECTED ITEM IN UNSELECTED CONNECT L%u.\n", pL);
			break;
		case SLURtype:
			aSlurL = FirstSubLINK(pL);
			for ( ; aSlurL; aSlurL = NextSLURL(aSlurL))
				if (SlurSEL(aSlurL))
					COMPLAIN("DCheckNodeSel: SELECTED ITEM IN UNSELECTED SLUR L%u.\n", pL);
			break;
/* FIXME: REPEATEND ALSO NEEDS CHECKING */
		default:
			;
	}

	return bad;
}


/* ------------------------------------------------------------------------- DCheckSel -- */
/* Do consistency checks on the selection: check selection start/end links, that no
node outside the range they describe has its selected flag set, etc. Returns True if
the selection start or end link is garbage or not even in the object list. */
 
Boolean DCheckSel(Document *doc, short *pnInRange, short *pnSelFlag)
{
	LINK pL;
	Boolean bad=False;

	if (DBadLink(doc, OBJtype, doc->selStartL, False)
	||  DBadLink(doc, OBJtype, doc->selEndL, False)) {
		COMPLAIN2("•DCheckSel: selStartL=%d OR selEndL=%d IS A GARBAGE LINK.\n",
						doc->selStartL, doc->selEndL);
		return True;
	}

	CountSelection(doc, pnInRange, pnSelFlag);

	if (!doc->masterView) {
		if (!InObjectList(doc, doc->selStartL, MAIN_DSTR))
			COMPLAIN("•DCheckSel: selStartL=%d NOT IN MAIN OBJECT LIST.\n",
						doc->selStartL);
	
		if (!InObjectList(doc, doc->selEndL, MAIN_DSTR)) {
			COMPLAIN("•DCheckSel: selEndL=%d NOT IN MAIN OBJECT LIST.\n",
						doc->selEndL);
			return True;
		}
		
		if (*pnInRange>0 && *pnSelFlag==0)
			COMPLAIN("DCheckSel: %d object(s) in selection range but none selected.\n",
						*pnInRange);
	
		for (pL = doc->headL; pL!=doc->selStartL; pL = RightLINK(pL)) {
			if (DErrLimit()) break;
			if (LinkSEL(pL))
				COMPLAIN("DCheckSel: object before selection range (L%u) selected.\n", pL);
			if (DBadLink(doc, OBJtype, RightLINK(pL), True) && doc->selStartL) {
				COMPLAIN("•DCheckSel: GARBAGE RightLINK(%d) BEFORE SELECTION RANGE.\n", pL);
				break;
			}
		}
		
			for (pL = doc->selEndL; pL!=doc->tailL; pL = RightLINK(pL)) {
				if (DErrLimit()) break;
				if (LinkSEL(pL))
					COMPLAIN("DCheckSel: object after selection range (L%u) selected.\n", pL);
			}
	}
	
	if (doc->selStartL!=doc->selEndL)
		if (!LinkSEL(doc->selStartL) || !LinkSEL(LeftLINK(doc->selEndL)))
			COMPLAIN2("DCheckSel: SELECTION RANGE (L%u TO L%u) IS NOT OPTIMIZED.\n",
				doc->selStartL, doc->selEndL);
	
	return False;			
}


/* ------------------------------------------------------------------ DCheckVoiceTable -- */
/* Check the voice-mapping table and its relationship to the object list:
	that the voice table contains no empty slots;
	that every default voice belongs to the correct part;
	that the voice no. of every note, rest, and grace note appears in the voice table;
	agreement between voice table part numbers and note, rest, and grace note part
		numbers;
	the legality of voiceRole fields.
We could also check other symbols with voice numbers. */

Boolean DCheckVoiceTable(Document *doc,
			Boolean fullCheck,				/* False=skip less important checks */
			short *pnVoicesUsed)
{
	Boolean bad, foundEmptySlot;
	LINK pL, aNoteL, aGRNoteL;
	LINK voiceUseTab[MAXVOICES+1], partL;
	short v, stf, partn;
	PPARTINFO pPart;
	Boolean voiceInWrongPart;

	bad = voiceInWrongPart = False;

	for (foundEmptySlot = False, v = 1; v<=MAXVOICES; v++) {
		if (doc->voiceTab[v].partn==0) foundEmptySlot = True;
		if (doc->voiceTab[v].partn!=0 && foundEmptySlot) {
				COMPLAIN("*DCheckVoiceTable: VOICE %ld IN TABLE FOLLOWS AN EMPTY SLOT.\n", (long)v);
				break;
		}
	}

	partL = NextPARTINFOL(FirstSubLINK(doc->headL));
	for (partn = 1; partL; partn++, partL=NextPARTINFOL(partL)) {
		pPart = GetPPARTINFO(partL);
		
		if (pPart->firstStaff<1 || pPart->firstStaff>doc->nstaves
		||  pPart->lastStaff<1 || pPart->lastStaff>doc->nstaves) {
			COMPLAIN("•DCheckVoiceTable: PART %ld firstStaff OR lastStaff IS ILLEGAL.\n",
						(long)partn);
			return True;
		}
		
		for (stf = pPart->firstStaff; stf<=pPart->lastStaff; stf++) {
			if (doc->voiceTab[stf].partn!=partn) {
				COMPLAIN2("*DCheckVoiceTable: VOICE %ld SHOULD BELONG TO PART %ld BUT DOESN'T.\n",
								(long)stf, (long)partn);
				voiceInWrongPart = True;
			}
		}
	}

	for (v = 1; v<=MAXVOICES; v++) {
		voiceUseTab[v] = NILINK;

	/* The objects with voice numbers other than notes and grace notes always get their
	   voice numbers from the notes and grace notes they're attached to, so we'll
	   assume they're OK, though it would be better to look. */
	   
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (voiceUseTab[NoteVOICE(aNoteL)]==NILINK)
						voiceUseTab[NoteVOICE(aNoteL)] = pL;
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
					if (voiceUseTab[GRNoteVOICE(aGRNoteL)]==NILINK)
						voiceUseTab[GRNoteVOICE(aGRNoteL)] = pL;
				break;
			default:
				;
		}
	}

	for (v = 1; v<=MAXVOICES; v++) {
		if (voiceUseTab[v]!=NILINK)
			if (doc->voiceTab[v].partn==0)
				COMPLAIN2("*DCheckVoiceTable: VOICE %ld (FIRST USED AT L%u) NOT IN TABLE.\n",
					(long)v, voiceUseTab[v]);
	}
	
	for (v = 1; v<=MAXVOICES; v++) {
		if (fullCheck && doc->voiceTab[v].partn!=0 && voiceUseTab[v]==NILINK)
			COMPLAIN("DCheckVoiceTable: voice %ld has no notes, rests, or grace notes.\n",
				(long)v);
	}

	for (*pnVoicesUsed = 0, v = 1; v<=MAXVOICES; v++)
		if (voiceUseTab[v]!=NILINK) (*pnVoicesUsed)++;

	if (voiceInWrongPart) return bad;				/* Following checks may give 100's of errors! */
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					v = NoteVOICE(aNoteL);
					stf = NoteSTAFF(aNoteL);
					if (doc->voiceTab[v].partn!=Staff2Part(doc, stf))
						COMPLAIN3("*DCheckVoiceTable: NOTE IN SYNC L%u STAFF %d IN ANOTHER PART'S VOICE, %ld.\n",
										pL, stf, (long)v);
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					v = GRNoteVOICE(aGRNoteL);
					stf = GRNoteSTAFF(aGRNoteL);
					if (doc->voiceTab[v].partn!=Staff2Part(doc, stf))
						COMPLAIN3("*DCheckVoiceTable: NOTE IN GRSYNC L%u STAFF %d IN ANOTHER PART'S VOICE, %ld.\n",
										pL, stf, (long)v);
				}
				break;
			default:
				;
		}

	for (v = 1; v<=MAXVOICES; v++) {
		if (doc->voiceTab[v].partn!=0) {
			if (doc->voiceTab[v].voiceRole<VCROLE_UPPER || doc->voiceTab[v].voiceRole>VCROLE_SINGLE)
				COMPLAIN("*DCheckVoiceTable: voiceTab[%ld].voiceRole IS ILLEGAL.\n", (long)v);
			if (doc->voiceTab[v].relVoice<1 || doc->voiceTab[v].relVoice>MAXVOICES)
				COMPLAIN("*DCheckVoiceTable: voiceTab[%ld].relVoice IS ILLEGAL.\n", (long)v);
		}
	}
	
	return bad;
}


/* ------------------------------------------------------------------- DCheckHeirarchy -- */
/* Check:
	(1) that the numbers of PAGEs and of SYSTEMs are what the header says they are;
	(2) that nodes appear on each page in the order (ignoring GRAPHICs and maybe
			others) PAGE, SYSTEM, STAFF, everything else;
	(3) that every staff number that should, and none that shouldn't, appears
		somewhere in the object list;
	(4) that every STAFF has at least one MEASURE, and that nothing appears between
		the STAFF and its first MEASURE except Clefs, KeySigs, TimeSigs, and Connects;
	(5) that Clef, KeySig, and TimeSig inMeasure flags agree with their position in
		the object list;
	(6) that at least one each Clef, KeySig, and TimeSig precede every other "content"
		object. */

Boolean DCheckHeirarchy(Document *doc)
{
	LINK 		pL, aStaffL, pageL=NILINK, systemL=NILINK;
	short		nMissing, i,
				nsystems, numSheets;
	Boolean aStaffFound[MAXSTAVES+1],					/* Found the individual staff? */
				foundPage, foundSystem, foundStaff,		/* Found any PAGE, SYSTEM, STAFF obj yet? */
				foundMeasure,							/* Found a MEASURE since the last STAFF? */
				foundClef, foundKeySig, foundTimeSig;	/* Found any CLEF, KEYSIG, TIMESIG yet? */
	Boolean	bad=False;
	PCLEF		pClef;  PKEYSIG pKeySig;  PTIMESIG pTimeSig;
				
	foundPage = foundSystem = foundStaff = False;
	for (i = 1; i<=doc->nstaves; i++)
		aStaffFound[i] = False;
	nsystems = numSheets = 0;
		
	foundClef = foundKeySig = foundTimeSig = False;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		switch (ObjLType(pL)) {
			case SYNCtype:
				if (!foundPage || !foundSystem || !foundStaff)
					COMPLAIN("•DCheckHeirarchy: SYNC L%u PRECEDES PAGE, SYSTEM OR STAFF.\n", pL);
				if (!foundClef || !foundKeySig || !foundTimeSig)
					COMPLAIN("•DCheckHeirarchy: Object L%u PRECEDES CLEF, KEYSIG, OR TIMESIG.\n", pL);
				break;
			case PAGEtype:
				foundPage = True;
				foundSystem = foundStaff = False;
				pageL = pL;
				numSheets++;
				break;
			case SYSTEMtype:
				foundSystem = True;
				systemL = pL;
				nsystems++;
				if (SysPAGE(pL)!=pageL)
					COMPLAIN("•DCheckHeirarchy: SYSTEM L%u HAS WRONG pageL.\n", pL);
				break;
			case STAFFtype:
				if (!foundPage || !foundSystem)
					COMPLAIN("•DCheckHeirarchy: STAFF L%u PRECEDES PAGE OR SYSTEM.\n", pL);
				foundStaff = True;
				for (aStaffL=FirstSubLINK(pL); aStaffL; 
							aStaffL=NextSTAFFL(aStaffL)) {
					if (STAFFN_BAD(doc, StaffSTAFF(aStaffL)))
						COMPLAIN2("*DCheckHeirarchy: STAFF L%u HAS BAD STAFF NUMBER %d.\n", pL, StaffSTAFF(aStaffL))
					else
						aStaffFound[StaffSTAFF(aStaffL)] = True;
				}
				if (StaffSYS(pL)!=systemL)
					COMPLAIN("•DCheckHeirarchy: STAFF L%u HAS WRONG systemL.\n", pL);
				break;
			case MEASUREtype:
				if (MeasSYSL(pL)!=systemL)
					COMPLAIN("•DCheckHeirarchy: MEASURE L%u HAS WRONG systemL.\n", pL);
				goto ChkAll;
			case CLEFtype:
				foundClef = True;
				goto ChkPageSysStaff;
			case KEYSIGtype:
				foundKeySig = True;
				goto ChkPageSysStaff;
			case TIMESIGtype:
				foundTimeSig = True;
				goto ChkPageSysStaff;
			case BEAMSETtype:
			case DYNAMtype:
			case OTTAVAtype:
	ChkAll:
				if (!foundClef || !foundKeySig || !foundTimeSig)
					COMPLAIN("•DCheckHeirarchy: Object L%u PRECEDES CLEF, KEYSIG, OR TIMESIG.\n", pL);
	ChkPageSysStaff:
				if (!foundPage || !foundSystem || !foundStaff)
					COMPLAIN("•DCheckHeirarchy: Object L%u PRECEDES PAGE, SYSTEM OR STAFF.\n", pL);
				break;
			case CONNECTtype:
				if (!foundPage || !foundSystem)
					COMPLAIN("•DCheckHeirarchy: CONNECT L%u PRECEDES PAGE OR SYSTEM.\n", pL);
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
		COMPLAIN("•DCheckHeirarchy: %ld STAVES NOT FOUND.\n", (long)nMissing);

	foundMeasure = foundSystem = True;
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
	switch (ObjLType(pL)) {
		case PAGEtype:
			if (!foundSystem) {
				COMPLAIN("•DCheckHeirarchy: PAGE L%u CONTAINS NO SYSTEMS.\n", LinkLPAGE(pL));
			}
			foundSystem = False;
			break;
		case SYSTEMtype:
			if (!foundMeasure) {
				COMPLAIN("•DCheckHeirarchy: SYSTEM L%u CONTAINS NO MEASURES.\n", LinkLSYS(pL));
			}
			foundMeasure = False;
			foundSystem = True;
			break;
		case MEASUREtype:
			foundMeasure = True;
		case STAFFtype:							/* These types can be outside of Measures */
		case CONNECTtype:
		case GRAPHICtype:
		case TEMPOtype:
			break;
		case CLEFtype:
			pClef = GetPCLEF(pL);
			if (pClef->inMeasure!=foundMeasure)
				COMPLAIN("*DCheckHeirarchy: CLEF L%u inMeasure FLAG DISAGREES WITH OBJECT ORDER.\n",
								pL);				
			break;
		case KEYSIGtype:
			pKeySig = GetPKEYSIG(pL);
			if (pKeySig->inMeasure!=foundMeasure)
				COMPLAIN("*DCheckHeirarchy: KEYSIG L%u inMeasure FLAG DISAGREES WITH OBJECT ORDER.\n",
								pL);				
			break;
		case TIMESIGtype:
			pTimeSig = GetPTIMESIG(pL);
			if (pTimeSig->inMeasure!=foundMeasure)
				COMPLAIN("*DCheckHeirarchy: TIMESIG L%u inMeasure FLAG DISAGREES WITH OBJECT ORDER.\n",
								pL);				
			break;
		case ENDINGtype:
			if (!CapsLockKeyDown()) break;
		default:
			if (!foundMeasure)
				COMPLAIN("•DCheckHeirarchy: OBJECT L%u PRECEDES ITS STAFF'S 1ST MEASURE.\n", pL);
			break;
	}
	if (!foundMeasure) {												/* Any Measures in the last System? */
		pL = LSSearch(doc->tailL, SYSTEMtype, ANYONE, True, False);		/* No */
		COMPLAIN("•DCheckHeirarchy: SYSTEM L%u CONTAINS NO MEASURES.\n", pL);
	}		

	return bad;			
}


/* --------------------------------------------------------------------- DCheckJDOrder -- */
/* For now, checks only Graphics. FIXME: Should eventually check that every JD object is 
in the "slot" preceding its relObj or firstObj. */

Boolean DCheckJDOrder(Document *doc)
{
	LINK pL, qL, attL;
	Boolean bad=False;
				
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
	switch (ObjLType(pL)) {
		case GRAPHICtype:
			attL = GraphicFIRSTOBJ(pL);
			if (PageTYPE(attL)) break;
			for (qL = RightLINK(pL); qL!=attL; qL = RightLINK(qL))
				if (!J_DTYPE(qL)) {
					if (!InObjectList(doc, attL, MAIN_DSTR)) {
						COMPLAIN3("•DCheckJDOrder: GRAPHIC ON STAFF %d L%u firstObj=%d NOT IN MAIN OBJECT LIST.\n",
									GraphicSTAFF(pL), pL, attL);
					}
					else {
						COMPLAIN3("*DCheckJDOrder: GRAPHIC ON STAFF %d L%u IN WRONG ORDER RELATIVE TO firstObj=%d.\n",
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


/* -------------------------------------------------------- DCheckBeamset, DCheckBeams -- */

/* Check the given Beamset. If <openingFile>, assume we're in the process of opening a
file and objects in the file are being processed in order. Syncs and GRSyncs in a Beamset
always follow the Beamset itself, so in that case we can't expect their flags to be
correct; but we _can_ expect their LINKs to be monotonically increasing. If <!openingFile>,
what we can expect is exactly the opposite. Return True iff we find a problem. */

Boolean DCheckBeamset(Document *doc, LINK beamL,
								Boolean maxCheck,			/* False=skip less important checks */
								Boolean openingFile,
								LINK *pLastSyncL)
{
	PBEAMSET		pBS;
	PANOTEBEAM		pNoteBeam;
	SearchParam 	pbSearch;
	LINK			aNoteL;
	Boolean			foundRest, grace, bad;
	short			staff, voice, n, nEntries;
	LINK			syncL, measureL, noteBeamL;
	Boolean			beamNotesOkay;

	bad = False;

	grace = BeamGRACE(beamL);
	nEntries = LinkNENTRIES(beamL);
	staff = BeamSTAFF(beamL);
	voice = BeamVOICE(beamL);
	if (VOICE_BAD(voice))
		COMPLAIN2("*DCheckBeamset: BEAMSET L%u HAS BAD voice %d.\n", beamL, voice);
	pBS = GetPBEAMSET(beamL);
	
	/* Is 1st note in same measure as BEAMSET? Do unoptimized search so this can be
	   used, e.g., by file-conversion functions. */
	
	measureL = LSUSearch(beamL, MEASUREtype, staff,	GO_RIGHT, False);
	if (measureL) {
		pNoteBeam = GetPANOTEBEAM(pBS->firstSubObj);
		if (IsAfter(measureL, pNoteBeam->bpSync))
			COMPLAIN("*DCheckBeamset: BEAMSET L%u IN DIFFERENT MEASURE FROM ITS 1ST SYNC.\n", beamL);
	}

	PANOTEBEAM aNoteBeam;
	short beamsNow = 0;
	for (LINK aNoteBeamL=FirstSubLINK(beamL); aNoteBeamL; aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
		aNoteBeam = GetPANOTEBEAM(aNoteBeamL);
		if (DBadLink(doc, OBJtype, aNoteBeam->bpSync, False)) {
			COMPLAIN("•DCheckBeamset: BEAMSET L%u HAS GARBAGE SYNC LINK.\n", beamL);
		}
		else if (!grace && ObjLType(aNoteBeam->bpSync)!=SYNCtype) {
			COMPLAIN("•DCheckBeamset: BEAMSET L%u HAS A BAD SYNC LINK.\n", beamL);
		}
		else if (grace && ObjLType(aNoteBeam->bpSync)!=GRSYNCtype)
			COMPLAIN("•DCheckBeamset: BEAMSET L%u HAS A BAD GRSYNC LINK.\n", beamL);

		beamsNow += aNoteBeam->startend;
		if ((NextNOTEBEAML(aNoteBeamL) && beamsNow<=0)
		||  beamsNow<0) {
			COMPLAIN2("*DCheckBeamset: BEAMSET L%u HAS BAD startend SEQUENCE (mid): beamsNow=%d.\n",
				beamL, beamsNow);
			break;
			}
		}

	if (beamsNow!=0) {
		COMPLAIN2("*DCheckBeamset: BEAMSET L%u HAS BAD startend SEQUENCE (end): beamsNow=%d.\n",
			beamL, beamsNow);
	}

	if (openingFile) {
		/* Check that LINKs of Syncs in the Beamset are monotonically increasing. */

		short lastLoc = beamL;
		noteBeamL = pBS->firstSubObj;
		for (n=1; noteBeamL; n++, noteBeamL=NextNOTEBEAML(noteBeamL))	{	/* For each SYNC with a note in BEAMSET... */
			LINK thisLoc = NoteBeamBPSYNC(noteBeamL);
//LogPrintf(LOG_DEBUG, "DCheckBeamset0: Beamset=L%u lastLoc=L%u thisLoc=L%u\n", beamL, lastLoc, thisLoc);
			if (thisLoc<=beamL)
				COMPLAIN2("*DCheckBeamset: BEAMSET L%u SYNC L%u LINK DOESN'T FOLLOW BEAMSET.\n", beamL, thisLoc)
			else if (thisLoc<=lastLoc)
				COMPLAIN2("*DCheckBeamset: BEAMSET L%u SYNC L%u LINK DOESN'T FOLLOW PREVIOUS SYNC.\n", beamL, thisLoc);
			lastLoc = thisLoc;
		}
		return bad;
	}
	
	foundRest = False;
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;											/* Prepare for search */
	pbSearch.voice = voice;
	pbSearch.needSelected = pbSearch.inSystem = False;
	pbSearch.optimize = False;
	noteBeamL = pBS->firstSubObj;
	syncL = beamL;
	beamNotesOkay = True;
	
	for (n=1; noteBeamL; n++, noteBeamL=NextNOTEBEAML(noteBeamL))	{	/* For each SYNC with a note in BEAMSET... */
Next:
			syncL = L_Search(RightLINK(syncL), (grace? GRSYNCtype : SYNCtype),
									GO_RIGHT, &pbSearch);
			if (DBadLink(doc, OBJtype, syncL, True)) {
				COMPLAIN2("*DCheckBeamset: BEAMSET L%u: TROUBLE FINDING %sSYNCS.\n", beamL,
								(grace? "GR" : ""));
				beamNotesOkay = False;
				break;
			}
			if (!SameSystem(beamL, syncL)) {
				COMPLAIN3("*DCheckBeamset: BEAMSET L%u: %sSYNC L%u NOT IN SAME SYSTEM.\n",
								beamL, (grace? "GR" : ""), syncL);
				beamNotesOkay = False;
				break;
			}
			
			aNoteL = pbSearch.pEntry;
			if (!grace && NoteREST(aNoteL)) foundRest = True;
			
			/* If this is a rest that could not be in the Beamset, keep looking. */
			
			if (!grace && NoteREST(aNoteL) && (!doc->beamRests && n>1 && n<nEntries) )
				goto Next;
			pNoteBeam = GetPANOTEBEAM(noteBeamL);
			
			/* If <foundRest>, a disagreement between the Sync we just found and the
			   one the Beamset refers to may simply be due to the Beamset having been
			   created with <beamRests> set the other way from its current setting,
			   and not because of any error. */
			   
			if (pNoteBeam->bpSync!=syncL) {
				if (foundRest && maxCheck) {
					COMPLAIN("DCheckBeamset: BEAMSET L%u SYNC LINK INCONSISTENT (WITH RESTS; PROBABLY OK).\n", beamL);
					beamNotesOkay = False;
				}
				else if (!foundRest) {
					COMPLAIN("*DCheckBeamset: BEAMSET L%u SYNC LINK INCONSISTENT.\n", beamL);
					beamNotesOkay = False;
				}
			}
	}
	
	*pLastSyncL = syncL;
	return bad;
}


/* Check: that Beamsets in a voice don't overlap, and that Beamset objects are consistent
with notes/rests or grace notes they should be referring to. For cross-system Beamsets,
also check that, after a non-grace Beamset that's the first of a cross-system pair, the
next non-grace Beamset in that voice is the second of a pair. (Notes/rests are checked
against their Beamset more carefully than grace notes are.) Return True iff we find a
problem.

Caveat: We don't do context-free checks of individual Beamset objects! For that, use a
loop that calls DCheckBeamset for each Beamset. (As of Sept. 2020, DCheckNode also calls
DCheckBeamset.) */

Boolean DCheckBeams(
				Document *doc,
				Boolean /*maxCheck*/		/* False=skip less important checks */
				)
{
	PANOTE		aNote, aGRNote;
	LINK		pL, aNoteL, aGRNoteL, lastSyncL, qL;
	short		voice;
	LINK		beamSetL[MAXVOICES+1], grBeamSetL[MAXVOICES+1];
	Boolean		expect2ndPiece[MAXVOICES+1];
	Boolean		grace, bad;
	
	bad = False;

	for (short v = 0; v<=MAXVOICES; v++) {
		beamSetL[v] = NILINK;
		grBeamSetL[v] = NILINK;
		expect2ndPiece[v] = False;
	}
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;
		
		switch (ObjLType(pL)) {

		 case BEAMSETtype:
			grace = BeamGRACE(pL);
			voice = BeamVOICE(pL);
			if (!VOICE_BAD(voice)) {
				if (grace)	grBeamSetL[voice] = pL;
				else		beamSetL[voice] = pL;
			}

			lastSyncL = LastInBeam(pL);

			for (qL = RightLINK(pL); qL!=lastSyncL; qL = RightLINK(qL))
				if (BeamsetTYPE(qL))
					if (BeamVOICE(qL)==BeamVOICE(pL)) {
						COMPLAIN2("*DCheckBeams: BEAMSET L%u IN SAME VOICE AS UNFINISHED BEAMSET L%u.\n",
									qL, pL);
						break;
					}

			if (!grace) {
				if (expect2ndPiece[voice] && (!BeamCrossSYS(pL) || BeamFirstSYS(pL)))
					COMPLAIN2("*DCheckBeams: BEAMSET L%u EXPECTED TO BE 2ND PIECE OF CROSS-SYS BEAM FOR VOICE %d BUT ISN'T.\n",
									pL, voice);

				expect2ndPiece[voice] = (BeamCrossSYS(pL) && BeamFirstSYS(pL));
			}
			
			break;

		 case SYNCtype:
		 	for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		 		aNote = GetPANOTE(aNoteL);
				if (aNote->beamed) {
					if (!beamSetL[NoteVOICE(aNoteL)]) {
						COMPLAIN3("*DCheckBeams: BEAMED NOTE IN SYNC L%u (MEASURE %d) VOICE %d WITHOUT BEAMSET.\n",
										pL, GetMeasNum(doc, pL), NoteVOICE(aNoteL));
					}
					else if (!SyncInBEAMSET(pL, beamSetL[aNote->voice]))
						COMPLAIN3("*DCheckBeams: BEAMED NOTE IN SYNC L%u (MEASURE %d) VOICE %d NOT IN VOICE'S LAST BEAMSET.\n",
										pL, GetMeasNum(doc, pL), NoteVOICE(aNoteL));
				}
			}
			break;

		 case GRSYNCtype:
		 	for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
		 		aGRNote = GetPAGRNOTE(aGRNoteL);
				if (aGRNote->beamed) {
					if (!grBeamSetL[aGRNote->voice]) {
						COMPLAIN2("*DCheckBeams: BEAMED NOTE IN GRSYNC L%u VOICE %d WITHOUT BEAMSET.\n",
										pL, aGRNote->voice);
					}
					else if (!SyncInBEAMSET(pL, grBeamSetL[aGRNote->voice]))
						COMPLAIN2("*DCheckBeams: BEAMED NOTE IN GRSYNC L%u VOICE %d NOT IN VOICE'S LAST BEAMSET.\n",
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


/* --------------------------------------------------------  DCheckOttavas and helpers -- */

static LINK FindNextSyncGRSync(LINK, short);
static short CountSyncVoicesOnStaff(LINK, short);

static LINK FindNextSyncGRSync(LINK pL, short staff)
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


short CountSyncVoicesOnStaff(LINK syncL, short staff)
{
	LINK aNoteL; short v, count;
	Boolean vInSync[MAXVOICES+1];
	
	for (v = 1; v<=MAXVOICES; v++)
		vInSync[v] = False;
		
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteSTAFF(aNoteL)==staff)
			vInSync[NoteVOICE(aNoteL)] = True;

	for (count = 0, v = 1; v<=MAXVOICES; v++)
		if (vInSync[v]) count++;
	return count;	
}


/* Check consistency of octave sign objects with notes/rests and grace notes they
should be referring to.

Note that we don't keep track of when the octave signs end: therefore, adding checking
that Notes that don't have <inOttava> flags really shouldn't isn't as easy as it might
look. */
 
Boolean DCheckOttavas(Document *doc)
{
	PANOTE			aNote;
	LINK			pL, aNoteL, syncL, measureL, noteOctL;
	short			staff, s, nVoice, j;
	LINK			ottavaL[MAXSTAVES+1];
	POTTAVA			pOct;
	PANOTEOTTAVA	pNoteOct;
	Boolean			bad;

	bad = False;
	
	for (s = 1; s<=doc->nstaves; s++)
		ottavaL[s] = NILINK;
		
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {

		 case OTTAVAtype:		 	
			staff = OttavaSTAFF(pL);
			if (STAFFN_BAD(doc, staff))
				COMPLAIN2("*DCheckOttavas: OTTAVA L%u HAS BAD STAFF NUMBER %d.\n", pL, staff)
			else
				ottavaL[staff] = pL;

			measureL = LSSearch(pL, MEASUREtype, staff,	GO_RIGHT, False);
			if (measureL) {
		 		pOct = GetPOTTAVA(pL);
				pNoteOct = GetPANOTEOTTAVA(pOct->firstSubObj);
				if (IsAfter(measureL, pNoteOct->opSync))
					COMPLAIN("*DCheckOttavas: OTTAVA L%u IN DIFFERENT MEASURE FROM ITS 1ST SYNC.\n", pL);
			}
			
		 	pOct = GetPOTTAVA(pL);
			noteOctL = pOct->firstSubObj;

			syncL = pL;
			for ( ; noteOctL; noteOctL = NextNOTEOTTAVAL(noteOctL))	{	/* For each SYNC with a note in OTTAVA... */
Next:
				syncL = FindNextSyncGRSync(RightLINK(syncL), staff);
				if (DBadLink(doc, OBJtype, syncL, True)) {
					COMPLAIN("*DCheckOttavas: OTTAVA L%u: TROUBLE FINDING SYNCS/GRSYNCS.\n", pL);
					break;
				}
				
				/* If this is a rest, it could not be in the octave sign, so keep looking. */
				
				aNoteL = NoteOnStaff(syncL, staff);
				if (NoteREST(aNoteL)) goto Next;
					
				/* If a Sync has 2 voices under the octave sign, the octave sign object
				   has 2 subobjects pointing to that Sync. */
				   
				nVoice = CountSyncVoicesOnStaff(syncL, staff);
				for (j = 1; j<=nVoice; j++) {
					if (j>1) noteOctL = NextNOTEOTTAVAL(noteOctL);
					pNoteOct = GetPANOTEOTTAVA(noteOctL);
					if (pNoteOct->opSync!=syncL)
						COMPLAIN2("*DCheckOttavas: OTTAVA L%u IN MEASURE %d SYNC/GRSYNC LINK INCONSISTENT.\n",
									pL, GetMeasNum(doc, pL));
				}
			}
			break;

		 case SYNCtype:
		 	for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		 		aNote = GetPANOTE(aNoteL);
				if (aNote->inOttava) {
					if (!ottavaL[aNote->staffn]) {
						COMPLAIN2("*DCheckOttavas: OTTAVA'D NOTE IN SYNC L%u STAFF %d WITHOUT OTTAVA.\n",
										pL, aNote->staffn);
					}
					else if (!SyncInOTTAVA(pL, ottavaL[aNote->staffn]))
						COMPLAIN2("*DCheckOttavas: OTTAVA'D NOTE IN SYNC L%u STAFF %d NOT IN OTTAVA.\n",
										pL, aNote->staffn);
				}
			}
			break;

		 default:
			;
		}
	
	return bad;			
}


/* ----------------------------------------------------------------------- DCheckSlurs -- */
/* For each slur, check:
	-that the slur object (including ties) and its firstSyncL and lastSyncL appear in
		the correct order in the object list.
	-that the next Sync after the slur has notes in the slur's voice.
	-since we currently (as of v. 5.8.x) do not allow nested or overlapping slurs or ties
		(though we do allow a tie nested within a slur), that there's no other nesting
		or overlapping.
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
	LINK	pL;
	short	i;
	short	staff, voice;
	LINK	slurEnd[MAXVOICES+1], tieEnd[MAXVOICES+1], endL;
	LINK	slurSysL, otherSlurL, otherSlurSysL, nextSyncL, afterNextSyncL; 
	Boolean	slur2FirstOK, slur2LastOK;
	Boolean	bad;

	bad = False;
	
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
				COMPLAIN("*DCheckSlurs: SLUR L%u HAS BAD voice.\n", pL)
			else {
			/* There should be no Syncs in the same voice between a slur and its first Sync. */
			
				nextSyncL = SSearch(pL, SYNCtype, GO_RIGHT);
				if (!NoteInVoice(nextSyncL, voice, False))
					COMPLAIN2("*DCheckSlurs: NEXT SYNC AFTER SLUR/TIE L%u HAS NO NOTES IN VOICE %d.\n",
										pL, voice);

			/* Handle slurs and ties separately. For each one, if there's currently one of
			   that type in progress and it doesn't end with the next Sync in the voice,
			   we have an error; otherwise store the end address of this slur/tie for
			   future use. */
			   
				nextSyncL = LVSearch(pL, SYNCtype, voice, GO_RIGHT, False);
				if (SlurTIE(pL)) {
					if (tieEnd[voice] && tieEnd[voice]!=nextSyncL)
						{ COMPLAIN2("*DCheckSlurs: TIE IN VOICE %d L%u WITH TIE ALREADY IN PROGRESS.\n",
										voice, pL);
						}
					else
						if (SlurFirstIsSYSTEM(pL)) {
							endL = LSSearch(pL, SYSTEMtype, ANYONE, GO_RIGHT, False);
							tieEnd[voice] = endL;
						}
						else {
							tieEnd[voice] = SlurLASTSYNC(pL);
							if (SlurCrossSYS(pL) && !SlurFirstIsSYSTEM(pL))
								afterNextSyncL = nextSyncL;
							else
								afterNextSyncL = LVSearch(RightLINK(nextSyncL), SYNCtype, voice,
																	GO_RIGHT, False);
							if (SlurLASTSYNC(pL)!=afterNextSyncL)
								COMPLAIN2("*DCheckSlurs: TIE IN VOICE %d L%u lastSyncL IS NOT THE NEXT SYNC IN VOICE.\n",
												voice, pL);
						}
				}
				else {
					if (slurEnd[voice] && slurEnd[voice]!=nextSyncL)
						{ COMPLAIN2("*DCheckSlurs: SLUR IN VOICE %d L%u WITH SLUR ALREADY IN PROGRESS.\n",
										voice, pL); }
					else
						if (SlurFirstIsSYSTEM(pL)) {
							endL = LSSearch(pL, SYSTEMtype, ANYONE, GO_RIGHT, False);
							slurEnd[voice] = endL;
						}
						else
							slurEnd[voice] = SlurLASTSYNC(pL);
				}
			}
			
			/* Check that the slur and its first and last objects are in the correct order.
			   They should be in the order:
				- last, slur, first		for the first piece of cross-system slurs
				- first, slur, last		for the last piece of cross-system slurs
				- slur, first, last		for non-cross-system slurs
			 */

			if (SlurCrossSYS(pL) && SlurFirstIsSYSTEM(pL)) {
				slur2FirstOK = IsAfter(pL, SlurFIRSTSYNC(pL));
				slur2LastOK = IsAfter(SlurLASTSYNC(pL), pL);
			}
			else if (SlurCrossSYS(pL) && !SlurFirstIsSYSTEM(pL)) {
				slur2FirstOK = IsAfter(SlurFIRSTSYNC(pL), pL);
				slur2LastOK = IsAfter(pL, SlurLASTSYNC(pL));
			}
			else {
				slur2FirstOK = IsAfter(pL, SlurFIRSTSYNC(pL));
				slur2LastOK = IsAfter(pL, SlurLASTSYNC(pL));
			}

			if (!slur2FirstOK)
				COMPLAIN2("*DCheckSlurs: SLUR L%u IN MEASURE %d AND ITS firstSyncL ARE IN WRONG ORDER.\n",
							pL, GetMeasNum(doc, pL));

			if (!slur2LastOK)
				COMPLAIN2("*DCheckSlurs: SLUR L%u IN MEASURE %d AND ITS lastSyncL ARE IN WRONG ORDER.\n",
							pL, GetMeasNum(doc, pL));

			/* If the slur is the first piece of a cross-system pair, check that the next
			   cross-system slur piece of the same subtype in the voice exists and is on
			   the next System. */
			   
			if (SlurCrossSYS(pL) && SlurFirstIsSYSTEM(pL)) {
				otherSlurL = XSysSlurMatch(pL);
				if (!otherSlurL)
					{ COMPLAIN("*DCheckSlurs: SLUR L%u IS 1ST PIECE OF CROSS-SYS BUT VOICE HAS NO CROSS-SYS SLURS FOLLOWING.\n",
									pL); }
				else {
					slurSysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, False);
					otherSlurSysL = LSSearch(otherSlurL, SYSTEMtype, ANYONE, GO_LEFT, False);
					if (LinkRSYS(slurSysL)!=otherSlurSysL)
						{ COMPLAIN2("*DCheckSlurs: SLUR L%u IS 1ST PIECE OF CROSS-SYS BUT NEXT SLUR IN VOICE (L%u) ISN'T IN NEXT SYSTEM.\n",
									pL, otherSlurL); }

				}
			}
		}
	}
	
	return bad;			
}


/* ----------------------------------------------------------------- LegalTupletTotDur -- */
/* If the given tuplet contains an unknown or whole-measure duration note/rest, return
-1; else return the tuplet's total duration. NB: in  case of chords, checks only one
note/rest of the chord. */

short LegalTupletTotDur(LINK);
short LegalTupletTotDur(LINK tupL)
{
	LINK pL, aNoteL, endL; short voice;
	
	voice = TupletVOICE(tupL);
	endL = LastInTuplet(tupL);
	for (pL = FirstInTuplet(tupL); pL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL = NoteInVoice(pL, voice, False);
			if (aNoteL)
				if (NoteType(aNoteL)==UNKNOWN_L_DUR || NoteType(aNoteL)<=WHOLEMR_L_DUR)
					return -1;
		}
		if (pL==endL) break;
	}
	
	return TupletTotDir(tupL);
}


/* --------------------------------------------------------------------- DCheckTuplets -- */
/* Check that tuplet objects are in the same measures as their last (and therefore all of
their) Syncs, and check consistency of tuplet objects with Syncs they should be referring
to. Also warn about any tuplets whose notes are subject to roundoff error in their
durations, since that can easily cause problems in syncing. Finally, check that total
durations make sense for the numerators. */

Boolean DCheckTuplets(
				Document *doc,
				Boolean maxCheck		/* False=skip less important checks */
				)
{
	LINK		pL, syncL;
	LINK		measureL, noteTupL;
	PTUPLET		pTuplet;
	PANOTETUPLE	noteTup;
	Boolean		bad;
	short		staff, voice, tupUnit, tupledUnit, totalDur;

	bad = False;

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case TUPLETtype:			 	
				staff = TupletSTAFF(pL);
				voice = TupletVOICE(pL);	
				measureL = LSSearch(pL, MEASUREtype, staff,	GO_RIGHT, False);
				if (measureL) {
					syncL = LastInTuplet(pL);
					if (IsAfter(measureL, syncL))
						COMPLAIN("*DCheckTuplets: TUPLET L%u IN DIFFERENT MEASURE FROM ITS LAST SYNC.\n",
										pL);
				}
				if (maxCheck) {
					pTuplet = GetPTUPLET(pL);
					tupUnit = GetMaxDurUnit(pL);
					tupledUnit = pTuplet->accDenom*tupUnit/pTuplet->accNum;
					if (pTuplet->accNum*tupledUnit/pTuplet->accDenom!=tupUnit)
						COMPLAIN("DCheckTuplets: DURATIONS OF NOTES IN TUPLET L%u MAY SHOW ROUNDOFF ERROR.\n",
										pL);
				}

				syncL = pL;
				noteTupL = FirstSubLINK(pL);
				for ( ; noteTupL; noteTupL = NextNOTETUPLEL(noteTupL))	{
					syncL = LVSearch(RightLINK(syncL), SYNCtype, voice, GO_RIGHT, False);
					if (DBadLink(doc, OBJtype, syncL, True)) {
						COMPLAIN("*DCheckTuplets: TUPLET L%u: HAD TROUBLE FINDING SYNCS.\n",
										pL);
						break;
					}
					noteTup = GetPANOTETUPLE(noteTupL);
					if (noteTup->tpSync!=syncL)
						COMPLAIN("*DCheckTuplets: TUPLET L%u SYNC LINK INCONSISTENT.\n",
										pL);
				}

				totalDur = LegalTupletTotDur(pL);
				if (totalDur<=0) {
					COMPLAIN("*DCheckTuplets: TUPLET L%u HAS AN UNKNOWN OR ILLEGAL DURATION NOTE.\n",
									pL);
				}
				else {
					pTuplet = GetPTUPLET(pL);
					if (GetDurUnit(totalDur, pTuplet->accNum, pTuplet->accDenom)==0)
						COMPLAIN2("*DCheckTuplets: TUPLET L%u TOTAL DURATION OF %d DOESN'T MAKE SENSE.\n",
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



/* -------------------------------------------------------------------- DCheckHairpins -- */
/* For each hairpin, check that the Dynamic object and its firstSyncL and lastSyncL
appear in the correct order in the object list. */

Boolean DCheckHairpins(Document *doc)
{
	LINK	pL;
	Boolean	bad;

	bad = False;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (DErrLimit()) break;

		if (ObjLType(pL)==DYNAMtype && IsHairpin(pL)) {
			
			/* Check that the hairpin and its first and last objects are in the correct
			   order. They should be in the order <hairpin, first, last>, except <first>
			   and <last> can be identical. */
			   
			if (!IsAfter(pL, DynamFIRSTSYNC(pL)))
				COMPLAIN("*DCheckHairpins: HAIRPIN L%u AND ITS firstSyncL ARE IN WRONG ORDER.\n",
								pL);			 
			if (DynamFIRSTSYNC(pL)!=DynamLASTSYNC(pL)
			&&  !IsAfter(DynamFIRSTSYNC(pL), DynamLASTSYNC(pL)))
				COMPLAIN("*DCheckHairpins: HAIRPIN L%u firstSyncL AND lastSyncL ARE IN WRONG ORDER.\n",
								pL);			 
		}
	}
	
	return bad;			
}


/* --------------------------------------------------------------------- DCheckObjRect -- */

Boolean DCheckObjRect(Document * /* doc */, LINK objL)
{
	PKEYSIG pKeySig;  Boolean bad=FALSE;
	
	POBJHDR p = GetPOBJHDR(objL);
	if (GARBAGE_Q1RECT(p->objRect)) {
		/* It's OK for initial keysigs to be unselectable. */
		
		pKeySig = GetPKEYSIG(objL);						/* FIXME: OR USE KeySigINMEAS? */
		if (!(KeySigTYPE(objL) && !pKeySig->inMeasure)) {
			LogPrintf(LOG_WARNING, "Object L%u IS UNSELECTABLE: IT HAS A GARBAGE objRect.  (DCheckObjRect)\n",
				objL);
			bad = True;
		}
		
	}
	
	/* Valid initial objects, e.g., "deleted", can have zero-width objRects. */
	
	else if (!ClefTYPE(objL) && !KeySigTYPE(objL) && !TimeSigTYPE(objL)
				&& ZERODIM_RECT(p->objRect)) {
		LogPrintf(LOG_WARNING, "Object L%u IS UNSELECTABLE: IT HAS A ZERO-WIDTH AND/OR HEIGHT objRect.  (DCheckObjRect)\n",
					objL);
		bad = True;
	}
	
	return bad;
}


/* --------------------------------------------------------------------- DCheckContext -- */
/* Do consistency checks on the entire score to see if changes of clef, key signature,
meter and dynamics in context fields of STAFFs and MEASUREs agree with appearances of
actual CLEF, KEYSIG, TIMESIG and DYNAM objects. */
 
Boolean DCheckContext(Document *doc)
{
	short		i;
	PASTAFF		aStaff;
	PAMEASURE	aMeas;
	PACLEF		aClef;
	PAKEYSIG	aKeySig;
	PATIMESIG	aTimeSig;
	PADYNAMIC	aDynamic;
	PCLEF		pClef;
	PKEYSIG		pKeySig;
	LINK		pL;
	LINK		aStaffL, aMeasL, aClefL, aKeySigL,
				aTimeSigL, aDynamicL;
	SignedByte	clefType[MAXSTAVES+1];				/* Current context: clef, */
	short		nKSItems[MAXSTAVES+1];				/*   sharps & flats in key sig., */
	SignedByte	timeSigType[MAXSTAVES+1],			/*   time signature, */
				numerator[MAXSTAVES+1],
				denominator[MAXSTAVES+1],
				dynamicType[MAXSTAVES+1];			/*		dynamic mark */
	Boolean		aStaffFound[MAXSTAVES+1],
				aMeasureFound[MAXSTAVES+1];
	Boolean		bad;

	bad = False;
		
	for (i = 1; i<=doc->nstaves; i++) {
		aStaffFound[i] = False;
		aMeasureFound[i] = False;
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
						aStaffFound[aStaff->staffn] = True;
					}
					else {														/* Check staff info */
						if (clefType[aStaff->staffn]!=aStaff->clefType) {
							COMPLAIN3("*DCheckContext: FOR STAFF %d IN STAFF L%u, INCONSISTENT clefType: EXPECTED %d.\n",
											aStaff->staffn, pL, clefType[aStaff->staffn]);
							aStaff = GetPASTAFF(aStaffL);
						}
						if (nKSItems[aStaff->staffn]!=aStaff->nKSItems) {
							COMPLAIN2("*DCheckContext: nKSItems FOR STAFF %d IN STAFF L%u INCONSISTENCY.\n",
											aStaff->staffn, pL);
							aStaff = GetPASTAFF(aStaffL);
						}
						if (timeSigType[aStaff->staffn]!=aStaff->timeSigType
						||  numerator[aStaff->staffn]!=aStaff->numerator
						||  denominator[aStaff->staffn]!=aStaff->denominator) {
							COMPLAIN2("*DCheckContext: TIMESIG FOR STAFF %d IN STAFF L%u INCONSISTENCY.\n",
											aStaff->staffn, pL);
							aStaff = GetPASTAFF(aStaffL);
						}
						if (dynamicType[aStaff->staffn]!=aStaff->dynamicType)
							COMPLAIN2("*DCheckContext: dynamicType FOR STAFF %d IN STAFF L%u INCONSISTENCY.\n",
											aStaff->staffn, pL);
					}
				}
				break;

			case MEASUREtype:	
				for (aMeasL=FirstSubLINK(pL); aMeasL; 
						aMeasL=NextMEASUREL(aMeasL)) {
					aMeas = GetPAMEASURE(aMeasL);
					if (clefType[aMeas->staffn]!=aMeas->clefType) {
						COMPLAIN3("*DCheckContext: clefType FOR STAFF %d IN MEASURE %d (L%u) INCONSISTENCY.\n",
										aMeas->staffn, GetMeasNum(doc, pL), pL);
						aMeas = GetPAMEASURE(aMeasL);
					}
					if (nKSItems[aMeas->staffn]!=aMeas->nKSItems) {
						COMPLAIN3("*DCheckContext: nKSItems FOR STAFF %d IN MEASURE %d (L%u) INCONSISTENCY.\n",
										aMeas->staffn, GetMeasNum(doc, pL), pL);
						aMeas = GetPAMEASURE(aMeasL);
					}
					if (timeSigType[aMeas->staffn]!=aMeas->timeSigType
					||  numerator[aMeas->staffn]!=aMeas->numerator
					||  denominator[aMeas->staffn]!=aMeas->denominator) {
						COMPLAIN3("*DCheckContext: TIMESIG FOR STAFF %d IN MEASURE %d (L%u) INCONSISTENCY.\n",
										aMeas->staffn, GetMeasNum(doc, pL), pL);
						aMeas = GetPAMEASURE(aMeasL);
					}
					if (dynamicType[aMeas->staffn]!=aMeas->dynamicType)
						COMPLAIN3("*DCheckContext: dynamicType FOR STAFF %d IN MEASURE %d (L%u) INCONSISTENCY.\n",
										aMeas->staffn, GetMeasNum(doc, pL), pL);
					aMeasureFound[aMeas->staffn] = True;
				}
				break;

			case CLEFtype:
				for (aClefL=FirstSubLINK(pL); aClefL; aClefL=NextCLEFL(aClefL)) {
					aClef = GetPACLEF(aClefL);
					pClef = GetPCLEF(pL);
					if (!pClef->inMeasure								/* System initial clef? */
					&& aMeasureFound[aClef->staffn]						/* After the 1st System? */
					&&  clefType[aClef->staffn]!=aClef->subType)		/* Yes, check the clef */
						COMPLAIN3("*DCheckContext: FOR STAFF %d IN CLEF L%u, INCONSISTENT clefType: EXPECTED %d.\n",
											aClef->staffn, pL, clefType[aClef->staffn]);
					clefType[aClef->staffn] = aClef->subType;
				}
				break;

			case KEYSIGtype:
				for (aKeySigL=FirstSubLINK(pL); aKeySigL;
						aKeySigL = NextKEYSIGL(aKeySigL))  {
					aKeySig = GetPAKEYSIG(aKeySigL);
					pKeySig = GetPKEYSIG(pL);
					if (!pKeySig->inMeasure								/* System initial key sig.? */
					&& aMeasureFound[aKeySig->staffn]					/* After the 1st System? */
					&&  nKSItems[aKeySig->staffn]!=aKeySig->nKSItems)	/* Yes, check it */
						COMPLAIN2("*DCheckContext: nKSItems FOR STAFF %d IN KEYSIG L%u INCONSISTENCY.\n",
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
				aDynamicL = FirstSubLINK(pL);							/* Only one subobj, so far */
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


/* ------------------------------------------------------------------- DCheck1NEntries -- */
/* Check the given object's nEntries field for agreement with the length of its list
of subobjects. */

Boolean DCheck1NEntries(
				Document */*doc*/,		/* unused */
				LINK pL)
{
	LINK subL, tempL;  Boolean bad;  short subCount;
	HEAP *myHeap;
	
	bad = False;
		
	if (TYPE_BAD(pL)) {
		COMPLAIN2("•DCheck1NEntries: OBJ L%u HAS BAD type %d.\n", pL, ObjLType(pL));
		return bad;
	}

	myHeap = Heap + ObjLType(pL);
	for (subCount = 0, subL = FirstSubLINK(pL); subL!=NILINK;
			subL = tempL, subCount++) {
		if (subCount>255) {
			COMPLAIN2("•DCheck1NEntries: OBJ L%u HAS nEntries=%d BUT SEEMS TO HAVE OVER 255 SUBOBJECTS.\n",
							pL, LinkNENTRIES(pL));
			break;
		}
		tempL = NextLink(myHeap, subL);
	}

	if (LinkNENTRIES(pL)!=subCount)
		COMPLAIN3("•DCheck1NEntries: OBJ L%u HAS nEntries=%d BUT %d SUBOBJECTS.\n",
						pL, LinkNENTRIES(pL), subCount);
	
	return bad;
}


/* -------------------------------------------------------------------- DCheckNEntries -- */
/* For the entire main object list, Check objects' nEntries fields for agreement with
the lengths of their lists of subobjects. This function is designed to be called
independently of Debug when things are bad, e.g., to check for Mackey's Disease, so it
protects itself against other simple data structure problems. */

Boolean DCheckNEntries(Document *doc)
{
	LINK pL;  Boolean bad;
	unsigned long soon=TickCount()+60L;
	
	bad = False;
		
	for (pL = doc->headL; pL && pL!=doc->tailL; pL = RightLINK(pL)) {
		if (TickCount()>soon) WaitCursor();
		if (DCheck1NEntries(doc, pL)) { bad = True; break; }
	}
	
	return bad;
}


/* ---------------------------------------------------------------- DCheck1SubobjLinks -- */
/* Check subobject links for the given object. */

Boolean DCheck1SubobjLinks(Document *doc, LINK pL)
{
	POBJHDR p;  HEAP *tmpHeap;
	LINK subObjL, badLink;  Boolean bad;
	
	bad = False;
	
	switch (ObjLType(pL)) {
		case HEADERtype: {
			LINK aPartL;
			
			for (aPartL=FirstSubLINK(pL); aPartL; aPartL=NextPARTINFOL(aPartL))
				if (DBadLink(doc, ObjLType(pL), aPartL, False))
					{ badLink = aPartL; bad = True; break; }
			break;
		}
		case STAFFtype: {
			LINK aStaffL;
			
				for (aStaffL=FirstSubLINK(pL); aStaffL; aStaffL=NextSTAFFL(aStaffL))
				if (DBadLink(doc, ObjLType(pL), aStaffL, False))
					{ badLink = aStaffL; bad = True; break; }
			}
			break;
		case CONNECTtype: {
			LINK aConnectL;
			
				for (aConnectL=FirstSubLINK(pL); aConnectL; aConnectL=NextCONNECTL(aConnectL))
				if (DBadLink(doc, ObjLType(pL), aConnectL, False))
					{ badLink = aConnectL; bad = True; break; }
			}
			break;
		case SYNCtype:
		case GRSYNCtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case MEASUREtype:
		case PSMEAStype:
		case DYNAMtype:
		case RPTENDtype:
			tmpHeap = Heap + ObjLType(pL);		/* p may not stay valid during loop */
			
			for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL=NextLink(tmpHeap,subObjL))
				if (DBadLink(doc, ObjLType(pL), subObjL, False))
					{ badLink = subObjL; bad = True; break; }
			break;
		case SLURtype: {
			LINK aSlurL;
			
				for (aSlurL=FirstSubLINK(pL); aSlurL; aSlurL=NextSLURL(aSlurL))
				if (DBadLink(doc, ObjLType(pL), aSlurL, False))
					{ badLink = aSlurL; bad = True; break; }
			}
			break;
			
		case BEAMSETtype: {
			LINK aNoteBeamL;
			
			aNoteBeamL = FirstSubLINK(pL);
			for ( ; aNoteBeamL; aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
				if (DBadLink(doc, ObjLType(pL), aNoteBeamL, False))
					{ badLink = aNoteBeamL; bad = True; break; }
			}
			break;
		}
		case OTTAVAtype: {
			LINK aNoteOctL;
			
			aNoteOctL = FirstSubLINK(pL);
			for ( ; aNoteOctL; aNoteOctL=NextNOTEOTTAVAL(aNoteOctL)) {
				if (DBadLink(doc, ObjLType(pL), aNoteOctL, False))
					{ badLink = aNoteOctL; bad = True; break; }
			}
			break;
		}
		case TUPLETtype: {
			LINK aNoteTupleL;
			
			aNoteTupleL = FirstSubLINK(pL);
			for ( ; aNoteTupleL; aNoteTupleL=NextNOTETUPLEL(aNoteTupleL)) {
				if (DBadLink(doc, ObjLType(pL), aNoteTupleL, False))
					{ badLink = aNoteTupleL; bad = True; break; }
			}
			break;
		}

		case GRAPHICtype:
		case TEMPOtype:
		case SPACERtype:
		case ENDINGtype:
			break;
			
		default:
			;
			break;
	}
	
	if (bad) COMPLAIN2("•DCheck1SubobjLinks: A SUBOBJ OF OBJ L%u HAS A BAD LINK OF L%u.\n",
								pL, badLink);
	
	return bad;
}
