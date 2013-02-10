/* SysPageEdit.c for Nightingale - routines for clipboard operations on and for
clearing entire systems and pages - slightly revised for v3.1. */

/*										NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1990-97 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static enum {
	TO_UNDO,			/* unused */
	TO_REDO,			/* unused */
	TO_SCORE,
	TO_CLIP
} E_SysPageEditDest;

/* --------------------------------------------------------------------------------- */
/* Local prototypes */

static void ReplaceHeader(Document *srcDoc, Document *dstDoc);
static LINK EditSysGetStartL(Document *doc, Boolean paste);

static short CountStfMap(Boolean *staffMap);
static void InitStfMap(Boolean *staffMap);
static Boolean KeySigCompare(PCONTEXT oldContext,PCONTEXT newContext);
static LINK CreateClefContext(Document *, LINK, short, Boolean *, PCONTEXT);
static LINK CreateKSContext(Document *, LINK, short, Boolean *, PCONTEXT, PCONTEXT);
static LINK CreateTSContext(Document *, LINK, short, Boolean *, PCONTEXT);
static void FixStfClefContext(LINK prevL);
static RMEASDATA *GetRMeasTable(Document *doc,LINK sysL,short *nMeas);
static void RespaceSystem(Document *doc,LINK sysL,RMEASDATA *rmTable,short nMeas);

static void MoveSystemY(LINK sysL, DDIST dv);
static void PSysInvalSys(Document *doc,LINK sysL);
static void PSysSetFirstIndent(Document *doc,LINK sysL);
static void PSysUpdateSysIndent(Document *doc,LINK sysL);
static void P1stSysUpdateFirstMeas(Document *doc,LINK newSysL);
static void PSysUpdateFirstMeas(Document *doc,LINK newSysL);

static void PSysFixStfSizeParams(LINK newSysL,LINK masterSysL,LINK newStfL,LINK masterStfL);
static void PSysFixStfSize(Document *doc,LINK newSysL,LINK rSys);
static Boolean PSysFixInitContext(Document *,LINK,PCONTEXT,PCONTEXT);
static void PSysFixContext(Document *doc,LINK prevL,LINK succL);
static void P1stSysFixContext(Document *doc, LINK succL);
static void Paste1stSysInPage(Document *,LINK);
static void PFixGraphicFont(Document *, LINK, LINK);
static Boolean GetXSysObj(LINK sysL, Boolean prev);

static Boolean PPageFixInitContext(Document *,LINK, PCONTEXT, PCONTEXT);
static void PPagesFixContext(Document *doc,LINK prevL,LINK succL,Boolean newFirstPage);
static LINK GetPageInsertL(Document *doc, Boolean *newFirstPage);

/* --------------------------------------------------------------------------------- */
/* Utilities for SysPageEdit.c. */

/*
 * Replace the header of dstDoc with that from srcDoc. This is done, for
 * example, when copying a system to the clipboard, so that the part-staff
 * format of the clipboard will be the same as that of the score.
 * Function returns with srcDoc installed.
 */

static void ReplaceHeader(Document *srcDoc, Document *dstDoc)
{
	LINK headRight,copyL;
	
	copyL = DuplicateObject(HEADERtype, srcDoc->headL, FALSE, srcDoc, dstDoc, FALSE);
	
	InstallDoc(dstDoc);

	headRight = RightLINK(dstDoc->headL);
	DeleteNode(dstDoc,dstDoc->headL);	
	dstDoc->headL = copyL;

	RightLINK(copyL) = headRight;
	LeftLINK(copyL) = NILINK;
	LeftLINK(headRight) = copyL;

	InstallDoc(srcDoc);
}


/* Get the proper link from which to determine what in the object list is affected
by clipboard operations on systems. Assumes that no objects are selected, i.e., there's
an insertion point.

If the insertion point is a PAGE, return the last object before the page.

If the insertion point is a SYSTEM, return that system, if it is the first
system on the page; else return the lSystem (and let the caller get the
last object in that SYSTEM).

If <paste> only, if the insertion point is before the first measure of a SYSTEM,
return that system, if it is the first system on the page; else return the
following system.

Otherwise, return the insertion point unchanged.

<paste> should be TRUE for pasting, FALSE for copying or clearing. */

static LINK EditSysGetStartL(Document *doc, Boolean paste)
{
	LINK sysL;

	if (PageTYPE(doc->selStartL))
		return LeftLINK(doc->selStartL);
	
	if (SystemTYPE(doc->selStartL)) {
		if (FirstSysInPage(doc->selStartL))		/* Default value: not obvious how doc->selStartL */
			return doc->selStartL;					/*		could ever be at this LINK. */
		return (LinkLSYS(doc->selStartL));		/* Will clear this or paste after it. */
	}
	
	if (BeforeFirstMeas(doc->selStartL) && paste) {
		sysL = LSSearch(doc->selStartL,SYSTEMtype,ANYONE,GO_LEFT,FALSE);
		if (FirstSysInPage(sysL))
			return (SysPAGE(sysL));
		return (LinkLSYS(sysL));
	}
	
	return doc->selStartL;
}

/* -------------------------------------------------------------------------------- */
/* Utilities for SysPageEdit updating context. */

/*
 * staffMap[] is used to indicate the staves on which context-establishing
 * subObjects need to be inserted.
 */

static short CountStfMap(Boolean *staffMap)
{
	short s,n=0;
	
	for (s=1; s<=MAXSTAVES; s++)
		if (staffMap[s]) n++;

	return n;
}

static void InitStfMap(Boolean *staffMap)
{
	short s;
	
	for (s=0; s<=MAXSTAVES; s++) staffMap[s] = FALSE;
}

/* Return TRUE if the keySigs are the same. */

static Boolean KeySigCompare(PCONTEXT oldContext, PCONTEXT newContext)
{
	short val;

	/* BlockCompare returns non-zero value if the arguments are different */

	val = BlockCompare(oldContext->KSItem,newContext->KSItem,7*sizeof(KSITEM));
	return val==0;
}

/*
 * CreateClefContext, CreateKSContext, and CreateTSContext insert clef,
 * keySig, and timeSig objects with <nEntries> subObjects, before insertL,
 * on staves marked by <staffMap>, obtaining context information from
 * newContext[].
 */

static LINK CreateClefContext(Document *doc, LINK insertL, short nEntries, Boolean *staffMap,
											PCONTEXT newContext)
{
	LINK clefL,aClefL,prevL; short s; DDIST xd;
	
	prevL = LeftLINK(insertL);
	clefL = InsertNode(doc, insertL, CLEFtype, nEntries);
	if (!clefL) { NoMoreMemory(); return NILINK; }
	
	aClefL = FirstSubLINK(clefL);
	for (s=1; s<=MAXSTAVES; s++)
		if (staffMap[s]) {
			InitClef(aClefL, s, 0, newContext[s].clefType);
			aClefL = NextCLEFL(aClefL);
		}

	if (MeasureTYPE(prevL)) {
		xd = SymDWidthRight(doc, prevL, ANYONE, FALSE, newContext[1]);
	}
	else {
		xd = LinkXD(prevL);
		xd += SymDWidthRight(doc, prevL, ANYONE, FALSE, newContext[1]);
	}
	LinkXD(clefL) = xd;
	LinkYD(clefL) = 0;
	ClefINMEAS(clefL) = TRUE;

	return clefL;
}

static LINK CreateKSContext(Document *doc, LINK insertL, short nEntries, Boolean *staffMap,
										PCONTEXT oldContext, PCONTEXT newContext)
{
	LINK keySigL,aKeySigL,prevL; short s; DDIST xd;
	PAKEYSIG aKeySig;
	
	prevL = LeftLINK(insertL);
	keySigL = InsertNode(doc, insertL, KEYSIGtype, nEntries);
	if (!keySigL) { NoMoreMemory(); return NILINK; }
	
	/* If the newContext nKSItems on staff s is non-zero, insert a
		new keySig with nKSItems sharps or flats to establish the new
		context for the pasted-in system.
		Otherwise, insert a keySig with nKSItems naturals to cancel
		the context of the previous system. */

	aKeySigL=FirstSubLINK(keySigL);
	for (s=1; s<=MAXSTAVES; s++)
		if (staffMap[s]) {
			if (newContext[s].nKSItems!=0) {
				InitKeySig(aKeySigL, s, 0, newContext[s].nKSItems);
				aKeySig = GetPAKEYSIG(aKeySigL);
				KEYSIG_COPY((PKSINFO)newContext[s].KSItem,(PKSINFO)aKeySig->KSItem);
				aKeySig->nKSItems = newContext[s].nKSItems;
			}
			else {
				InitKeySig(aKeySigL, s, 0, oldContext[s].nKSItems);
				aKeySig = GetPAKEYSIG(aKeySigL);
				aKeySig->subType = oldContext[s].nKSItems;
			}
			aKeySigL = NextKEYSIGL(aKeySigL);
		}

	if (MeasureTYPE(prevL)) {
		xd = SymDWidthRight(doc, prevL, ANYONE, FALSE, newContext[1]);
	}
	else {
		xd = LinkXD(prevL);
		xd += SymDWidthRight(doc, prevL, ANYONE, FALSE, newContext[1]);
	}
	LinkXD(keySigL) = xd;
	LinkYD(keySigL) = 0;
	KeySigINMEAS(keySigL) = TRUE;

	return keySigL;
}


static LINK CreateTSContext(Document *doc, LINK insertL, short nEntries, Boolean *staffMap,
										PCONTEXT newContext)
{
	LINK timeSigL,aTimeSigL,prevL; short s; DDIST xd;
	
	prevL = LeftLINK(insertL);
	timeSigL = InsertNode(doc, insertL, TIMESIGtype, nEntries);
	if (!timeSigL) { NoMoreMemory(); return NILINK; }
	
	aTimeSigL = FirstSubLINK(timeSigL);
	for (s=1; s<=MAXSTAVES; s++)
		if (staffMap[s]) {
			InitTimeSig(aTimeSigL,s,0,newContext[s].timeSigType,newContext[s].numerator,newContext[s].denominator);
			aTimeSigL = NextTIMESIGL(aTimeSigL);
		}

	if (MeasureTYPE(prevL)) {
		xd = SymDWidthRight(doc, prevL, ANYONE, FALSE, newContext[1]);
	}
	else {
		xd = LinkXD(prevL);
		xd += SymDWidthRight(doc, prevL, ANYONE, FALSE, newContext[1]);
	}
	LinkXD(timeSigL) = xd;
	LinkYD(timeSigL) = 0;
	TimeSigINMEAS(timeSigL) = TRUE;
	
	return timeSigL;
}

/*
 * Search for the new system and fix up the clefType fields in its staff subObjs.
 */

static void FixStfClefContext(LINK prevL)
{
	LINK newSysL,newStfL,aStaffL,newClefL,aClefL;
	short clefType;

	newSysL = SSearch(prevL,SYSTEMtype,GO_RIGHT);
	newStfL = SSearch(newSysL,STAFFtype,GO_RIGHT);
	aStaffL = FirstSubLINK(newStfL);
	newClefL = SSearch(newStfL,CLEFtype,GO_RIGHT);

	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		aClefL = FirstSubLINK(newClefL);
		for ( ; aClefL; aClefL = NextCLEFL(aClefL))
			if (ClefSTAFF(aClefL)==StaffSTAFF(aStaffL)) {
				clefType = ClefType(aClefL);
				StaffCLEFTYPE(aStaffL) = clefType;
			}
		
	}
}


/*
 * Return an RMEASDATA *rmTable for sysL for the purposes of respacing sysL,
 * and return the number of measures in sysL in <nMeas>.
 */

static RMEASDATA *GetRMeasTable(Document */*doc*/, LINK sysL, short *nMeas)
{
	LINK measL; DDIST mWidth;
	short i=0; RMEASDATA *rmTable;

	rmTable = (RMEASDATA *)NewPtr(MAX_MEASURES*sizeof(RMEASDATA));
	if (!GoodNewPtr((Ptr)rmTable)) { NoMoreMemory(); return NULL; }

	measL = SSearch(sysL,MEASUREtype,GO_RIGHT);
	for ( ; measL && MeasSYSL(measL)==sysL; i++,measL = LinkRMEAS(measL)) {
		mWidth = MeasWidth(measL);
		rmTable[i].measL = measL;
		rmTable[i].width = mWidth;
		rmTable[i].lxd = LinkXD(measL);
	}

	*nMeas = i;
	return rmTable;
}

/*
 * Respace sysL, a system containing nMeas measures, using rmTable, to insure
 * all objects fit properly within it.
 */

static void RespaceSystem(Document *doc, LINK sysL, RMEASDATA *rmTable, short nMeas)
{
	LINK firstMeasL; CONTEXT context;
	long newSpProp;

	firstMeasL = SSearch(sysL,MEASUREtype,GO_RIGHT);

	GetContext(doc, firstMeasL, 1, &context);
	newSpProp = GetJustProp(doc, rmTable, 0, nMeas-1, context);

	PositionSysByTable(doc, rmTable, 0, nMeas-1, newSpProp, context);
}

/* -------------------------------------------------------------------------------- */
/* Editing operations on systems. */

/*
 * Copy the system containing the insertion point from doc to the clipboard.
 *
 * Since doc->selStartL will be the following page, system or doc->tailL if
 * the insertion point is at the end of the system, the routine must call
 * EditSysGetStartL to get the correct system to copy.
 *
 * Routine deletes all material in the clipboard from its system to the
 * tail, copies the system, and then fixes up system numbers, crossSystem
 * objects and selection LINKs in the clipboard.
 */

void CopySystem(Document *doc)
{
	LINK sysL,clipSys,lastL,startL,pL;
	short nSystems=0;
	
	ReplaceHeader(doc,clipboard);
	SetupClipDoc(doc,FALSE);

	startL = EditSysGetStartL(doc,FALSE);
	sysL = LSSearch(startL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	lastL = LastObjInSys(doc, RightLINK(sysL));

	InstallDoc(clipboard);
	clipSys = SSearch(clipboard->headL,SYSTEMtype,GO_RIGHT);
	DeleteRange(clipboard, clipSys, clipboard->tailL);
	
	CopyRange(doc, clipboard, sysL, DRightLINK(doc,lastL), clipboard->tailL, TO_CLIP);
	
	InstallDoc(clipboard);
	
	for (pL=clipboard->headL; pL; pL = RightLINK(pL))
		if (SystemTYPE(pL))
			{ SystemNUM(pL) = 1; break; }
	clipboard->numSheets = 1;
	clipboard->nsystems = 1;

	clipboard->selEndL = clipboard->selStartL =
		LSUSearch(clipboard->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);

	FixDelCrossSysObjs(clipboard);

	CopyFontTable(doc);

	InstallDoc(doc);
}

/*
 * Position sysL graphically: offset the systemRect and all measureBBoxes of
 * measures contained in the system by dy.
 *
 * Similar function: OffsetSystem() in ShowFormat.c
 */

static void MoveSystemY(LINK sysL, DDIST dy)
{
	LINK measL;

	OffsetDRect(&SystemRECT(sysL), 0, dy);

	measL = SSearch(sysL,MEASUREtype,FALSE);
	for ( ; measL; measL=LinkRMEAS(measL)) {
		if (MeasSYSL(measL)!=sysL) break;
		OffsetRect(&MeasureBBOX(measL), 0, d2p(dy));
	}
}

/*
 * Translate SystemRECT of pasted-in system to window-relative coordinates
 * and erase and inval it.
 */

static void PSysInvalSys(Document *doc, LINK sysL)
{
	LINK pageL;
	DRect invalDRect; Rect invalRect,paperRect;

	invalDRect = SystemRECT(sysL);
	OffsetDRect(&invalDRect,0,invalDRect.bottom - invalDRect.top);
	D2Rect(&invalDRect,&invalRect);
	invalRect.left = 0;

	pageL = LSSearch(sysL,PAGEtype,ANYONE,GO_LEFT,FALSE);
	GetSheetRect(doc,SheetNUM(pageL),&paperRect);
	OffsetRect(&invalRect,paperRect.left,paperRect.top);
	EraseAndInval(&invalRect);
}


/* 
 * Update the system left indent to set the pasted-in system's left indent
 *	to doc->firstIndent.
 */

static void PSysSetFirstIndent(Document *doc, LINK sysL)
{
	LINK masterSysL; DDIST change,firstIndent;
	PSYSTEM pMasterSys,pSys;

	masterSysL = SSearch(doc->masterHeadL,SYSTEMtype,GO_RIGHT);
	pMasterSys = GetPSYSTEM(masterSysL);

	firstIndent = doc->firstIndent - doc->otherIndent;
	pSys = GetPSYSTEM(sysL);
	change = pMasterSys->systemRect.left - pSys->systemRect.left + firstIndent;

	ChangeSysIndent(doc, sysL, change);
}


/*
 * Update sysL's left indent to move system to same indent as that of the
 * master system.
 */

static void PSysUpdateSysIndent(Document *doc, LINK sysL)
{
	LINK masterSysL; DDIST change;
	PSYSTEM pMasterSys,pSys;

	masterSysL = SSearch(doc->masterHeadL,SYSTEMtype,GO_RIGHT);
	pMasterSys = GetPSYSTEM(masterSysL);

	pSys = GetPSYSTEM(sysL);
	change = pMasterSys->systemRect.left - pSys->systemRect.left;

	ChangeSysIndent(doc, sysL, change);
}

/*
 * Update the xd of the first invisible measure to compensate
 * for possible visification of initial timeSig.
 * Clef will always be visible, and keySig will always be accounted for,
 * so the only initial obj to deal with is the timeSig.
 */

static void P1stSysUpdateFirstMeas(Document *doc, LINK newSysL)
{
	LINK ksL,tsL; DDIST haveWidth,needWidth,change;

	ksL = SSearch(newSysL,KEYSIGtype,GO_RIGHT);
	tsL = SSearch(ksL,TIMESIGtype,GO_RIGHT);

	haveWidth = LinkXD(tsL) - LinkXD(ksL);
	needWidth = GetKeySigWidth(doc,ksL,1);

	change = needWidth-haveWidth;
	if (change) ChangeSpaceBefFirst(doc,ksL,change,FALSE);
}

/*
 * Update the xd of the first invisible measure to compensate
 * for possible invisification of initial timeSig.
 * Clef will always be visible, and keySig will always be accounted for,
 * so the only initial obj to deal with is the timeSig.
 */

static void PSysUpdateFirstMeas(Document *doc, LINK newSysL)
{
	LINK ksL,tsL,firstMeasL; DDIST haveWidth,needWidth,change;

	ksL = SSearch(newSysL,KEYSIGtype,GO_RIGHT);
	tsL = SSearch(ksL,TIMESIGtype,GO_RIGHT);

	LinkXD(tsL) = LinkXD(ksL);

	firstMeasL = SSearch(ksL,MEASUREtype,GO_RIGHT);
	haveWidth = LinkXD(firstMeasL)-LinkXD(ksL);
	needWidth = GetKeySigWidth(doc,ksL,1);

	change = needWidth-haveWidth;
	if (change) ChangeSpaceBefFirst(doc,ksL,change,FALSE);
}

/*
 * Update fields in system and staff objects newSysL,newStfL for new
 * new staff size. 
 */

static void PSysFixStfSizeParams(LINK newSysL, LINK masterSysL, LINK newStfL,
											LINK masterStfL)
{
	LINK aStaffL,mStaffL;
	PASTAFF aStaff,mStaff;
	DDIST sysHeight;

	aStaffL = FirstSubLINK(newStfL);
	mStaffL = FirstSubLINK(masterStfL);
	for ( ; mStaffL && aStaffL; mStaffL=NextSTAFFL(mStaffL),
										 aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		mStaff = GetPASTAFF(mStaffL);
		aStaff->staffTop = mStaff->staffTop;
		aStaff->staffLeft = mStaff->staffLeft;
		aStaff->staffRight = mStaff->staffRight;
		aStaff->staffHeight = mStaff->staffHeight;
	}
	
	sysHeight = SysRectBOTTOM(masterSysL) - SysRectTOP(masterSysL);
	SysRectBOTTOM(newSysL) = SysRectTOP(newSysL) + sysHeight;
}

/*
 * Update the staff size of the pasted-in system.
 */

static void PSysFixStfSize(Document *doc, LINK newSysL, LINK rSys)
{
	LINK masterStfL,masterSysL,newStfL;

	/* Update the staff rastral size for the pasted-in system. */
	
	masterSysL = SSearch(doc->masterHeadL,SYSTEMtype,GO_RIGHT);

	if (clipboard->srastral!=doc->srastral) {
		PasteFixStfSize(doc, newSysL, rSys ? rSys : doc->tailL);

		masterStfL = SSearch(masterSysL,STAFFtype,GO_RIGHT);
		newStfL = SSearch(newSysL,STAFFtype,GO_RIGHT);		
		PSysFixStfSizeParams(newSysL,masterSysL,newStfL,masterStfL);
	}
}

/*
 * Fix up the initial context of a system inserted after prevL by adding
 * context objects to the end of the previous system.
 */

static Boolean PSysFixInitContext(Document *doc, LINK prevL, PCONTEXT oldContext,
												PCONTEXT newContext)
{
	short s,nEntries,nMeas;
	Boolean staffMap[MAXSTAVES+1];
	LINK firstMeasL,insertL,rightL,sysL;
	RMEASDATA *rmTable;

	/* Get the new system and fix up the clefType fields in its staff subObjs. */

	FixStfClefContext(prevL);

	ZeroMem(oldContext,(long)sizeof(CONTEXT)*(MAXSTAVES+1));
	ZeroMem(newContext,(long)sizeof(CONTEXT)*(MAXSTAVES+1));

	/* The old context is the context in force at the end of the page
		before the contexted page, i.e., at prevL. */
	insertL = rightL = RightLINK(prevL);
	GetAllContexts(doc,oldContext,prevL);
	
	/* The new context is gotten at the first invisible measure of the
		contexted page. */
	firstMeasL = LSSearch(rightL,MEASUREtype,ANYONE,GO_RIGHT,FALSE);
	GetAllContexts(doc,newContext,firstMeasL);

	/* Determine the staves for which the clef context has changed,
		and mark those staves in the staffMap. Then use the staffMap
		and old and new clef contexts to create a clef object with
		one subObj on each marked staff, whose clefType is the same
		as that of the new context. */
		
	InitStfMap(staffMap);
	for (s=1; s<=MAXSTAVES; s++) {
		if (oldContext[s].clefType!=newContext[s].clefType)
			staffMap[s] = TRUE;
	}
	nEntries = CountStfMap(staffMap);
	if (nEntries>0) {
		insertL = CreateClefContext(doc,rightL,nEntries,staffMap,newContext);
		if (!insertL) return FALSE;
	}

	/* Do same for keySigs as for clefs. */
	InitStfMap(staffMap);
	for (s=1; s<=MAXSTAVES; s++) {
		if (!KeySigCompare(&oldContext[s],&newContext[s]))
			staffMap[s] = TRUE;
	}
	nEntries = CountStfMap(staffMap);
	if (nEntries>0) {
		insertL = CreateKSContext(doc,rightL,nEntries,staffMap,oldContext,newContext);
		if (!insertL) return FALSE;
	}

	/* Do same for timeSigs as for clefs. */
	InitStfMap(staffMap);
	for (s=1; s<=MAXSTAVES; s++) {
		if (oldContext[s].timeSigType!=newContext[s].timeSigType ||
				oldContext[s].numerator!=newContext[s].numerator ||
				oldContext[s].denominator!=newContext[s].denominator)
			staffMap[s] = TRUE;
	}
	nEntries = CountStfMap(staffMap);
	if (nEntries>0)
		if (!CreateTSContext(doc,rightL,nEntries,staffMap,newContext)) return FALSE;
	
	sysL = LSSearch(prevL,SYSTEMtype,ANYONE,GO_LEFT,FALSE);
	rmTable = GetRMeasTable(doc,sysL,&nMeas);
	if (rmTable) {
		RespaceSystem(doc,sysL,rmTable,nMeas);
		DisposePtr((Ptr)rmTable);
	}

	InvalSystem(prevL);
	return TRUE;
}


/*
 * Fix up the context for a system inserted between prevL and succL by fixing
 * up the context at the boundary of the inserted system and preceding and
 * following systems.
 */

static void PSysFixContext(Document *doc, LINK prevL, LINK succL)
{
	PCONTEXT oldContext,newContext;
	LINK rightL,nextSysL;

	rightL = RightLINK(prevL);

	oldContext = AllocContext();
	if (!oldContext) goto done;
	newContext = AllocContext();
	if (!newContext) goto done;

	/* Fix up context boundary of previous system and inserted system. */

	if (!PSysFixInitContext(doc,prevL,oldContext,newContext))
		goto done;
		
	/* If no system after the system to be inserted, nothing more to
		fix up. Otherwise, fix up context boundary of inserted system
		and following system. */
	nextSysL = LSSearch(succL,SYSTEMtype,ANYONE,GO_RIGHT,FALSE);
	if (nextSysL)
		PSysFixInitContext(doc,LeftLINK(succL),oldContext,newContext);

done:
	if (oldContext) DisposePtr((Ptr)oldContext);
	if (newContext) DisposePtr((Ptr)newContext);
}


/*
 * Fix up the context for a system inserted at the beginning of the
 * score by fixing up the context at the boundary between that system
 * and the following system.
 */

static void P1stSysFixContext(Document *doc, LINK succL)
{
	PCONTEXT oldContext,newContext;
	LINK nextSysL;

	oldContext = AllocContext();
	if (!oldContext) goto done;
	newContext = AllocContext();
	if (!newContext) goto done;

	/* Verify that a system exists after the system to be inserted. If not,
		return. Otherwise, fix up boundary of inserted system and following
		system. */
	nextSysL = LSSearch(succL,SYSTEMtype,ANYONE,GO_RIGHT,FALSE);
	if (nextSysL)
		PSysFixInitContext(doc,LeftLINK(succL),oldContext,newContext);

done:
	if (oldContext) DisposePtr((Ptr)oldContext);
	if (newContext) DisposePtr((Ptr)newContext);
}


/*
 * Paste a system in the first slot in the page: between the page object
 * and the first system in the page. <pageL> is the page object.
 */

static void Paste1stSysInPage(Document *doc, LINK pageL)
{
	LINK prevL,insertL,clipSys,lastL,succSys,rSys,rStaff,rMeas,lSys,
			lStaff,sysL,staffL,measL,newSys,newStf,newMeas,newFirstMeas,
			firstLMeas,timeSigL,clipTimeSigL;
	DRect sysRect,prevSysRect; DDIST sysHeight;
	Boolean firstInScore;
	SearchParam pbSearch;

	/* Insert before the system following pageL. */

	insertL = sysL = LSSearch(pageL, SYSTEMtype, ANYONE, GO_RIGHT, FALSE);
	prevL = LeftLINK(sysL);
	firstInScore = !LinkLSYS(sysL);
	if (firstInScore) {
		InitSearchParam(&pbSearch);
		pbSearch.voice = ANYONE;
		pbSearch.needSelected = FALSE;
		pbSearch.inSystem = TRUE;
		timeSigL = L_Search(RightLINK(sysL), TIMESIGtype, GO_RIGHT, &pbSearch);

		if (TimeSigINMEAS(timeSigL)) timeSigL = NILINK;
	}

	rSys = sysL;	
	rStaff = staffL = SSearch(sysL,STAFFtype,GO_RIGHT);
	measL = SSearch(staffL,MEASUREtype,GO_RIGHT);

	InstallDoc(clipboard);

	clipSys = SSearch(clipboard->headL, SYSTEMtype, GO_RIGHT);
	CopyRange(clipboard, doc, clipSys, clipboard->tailL, insertL, TO_SCORE);

	InitSearchParam(&pbSearch);
	pbSearch.voice = ANYONE;
	pbSearch.needSelected = FALSE;
	pbSearch.inSystem = TRUE;
	clipTimeSigL = L_Search(RightLINK(clipSys), TIMESIGtype, GO_RIGHT, &pbSearch);

	if (TimeSigINMEAS(clipTimeSigL)) clipTimeSigL = NILINK;

	InstallDoc(doc);

	/* Fix cross linkages to objects following the inserted
		system. */

	newSys = SSearch(pageL,SYSTEMtype,GO_RIGHT);
	newStf = SSearch(newSys,STAFFtype,GO_RIGHT);
	newMeas = SSearch(rSys,MEASUREtype,GO_LEFT);			/* Fix cross links for the */
																		/*		last meas in new system */

	rMeas = SSearch(rSys,MEASUREtype,GO_RIGHT);			/* First meas in new rSys */

	SysPAGE(newSys) = pageL;
	LinkRSYS(newSys) = rSys;
	StaffSYS(newStf) = newSys;
	LinkRSTAFF(newStf) = rStaff;
	LinkRMEAS(newMeas) = rMeas;

	LinkLSYS(rSys) = newSys;
	LinkLSTAFF(rStaff) = newStf;
	LinkLMEAS(rMeas) = newMeas;

	sysRect = SystemRECT(newSys);
	sysHeight = sysRect.bottom-sysRect.top;
	prevSysRect = SystemRECT(rSys);

	/* Translate newly added system to location of previous
		1st system in page. */

	MoveSystemY(newSys,prevSysRect.top-sysRect.top);
	InvalRange(newSys,LastObjInSys(doc,RightLINK(newSys)));
	
	/* Move systemRects down for systems following the inserted
		system. */

	for (succSys=rSys; succSys; succSys = LinkRSYS(succSys)) {
		if (!SamePage(succSys, newSys)) break;			/* Avoid passing NILINK to SamePage */
		MoveSystemY(succSys,sysHeight);
		InvalSystem(succSys);
		InvalRange(succSys,LastObjInSys(doc,RightLINK(succSys)));
	}

	/* Fix cross linkages from objects before the inserted
		system. */

	lSys = LSSearch(LeftLINK(newSys),SYSTEMtype,ANYONE,GO_LEFT,FALSE);
	lStaff = lSys ? SSearch(lSys,SYSTEMtype,GO_RIGHT) : NILINK;

	newFirstMeas = SSearch(newSys,MEASUREtype,GO_RIGHT);
	firstLMeas = SSearch(newSys,MEASUREtype,GO_LEFT);

	LinkLSYS(newSys) = lSys;
	LinkLSTAFF(newStf) = lStaff;
	LinkLMEAS(newFirstMeas) = firstLMeas;

	if (lSys) {
		LinkRSYS(lSys) = newSys;
		LinkRSTAFF(lStaff) = newStf;
		LinkRMEAS(firstLMeas) = newFirstMeas;
	}

	/* If the newly pasted system is the first in the score, and this system
		had no timesig, move the timeSig from the previous firstInScore system
		to the gutter of the new system.
		If the newly pasted system is the first in the score, and this system
		had a timesig, and the previous first system also had a timesig, 
		cut the timeSig from the previous firstInScore system. */

	if (firstInScore && timeSigL && !clipTimeSigL) {
		MoveNode(timeSigL,newFirstMeas);
	}
	else if (firstInScore && timeSigL && clipTimeSigL) {
		CutNode(timeSigL);
	}

	/* Update system nums, measure nums, pTimes, handle
		selection range. */

	UpdateSysNums(doc,doc->headL);
	UpdateMeasNums(doc,NILINK);
	FixTimeStamps(doc, newSys,NILINK);
	doc->changed = TRUE;

	DeselRangeNoHilite(doc, doc->headL, doc->tailL);
	lastL = LastObjInSys(doc,RightLINK(newSys));
	doc->selStartL = doc->selEndL = RightLINK(lastL);

	/* Fix context and cross system objects. */

	FixDelCrossSysObjs(doc);
	P1stSysFixContext(doc,insertL);

	/* Update the staff rastral size for the pasted-in system. */
	
	PSysFixStfSize(doc,newSys,rSys);
	
	/* Update the system left indent to set the pasted-in system's
		left indent to doc->firstIndent. */
		
	PSysSetFirstIndent(doc,newSys);

	/* Update the system left indent for the system following the new
		first sys. */
		
	PSysUpdateSysIndent(doc,rSys);
	
	/* Update the position of the first invisible measure. */
	
	P1stSysUpdateFirstMeas(doc,newSys);

	/* Get region to inval and inval pasted-in system's sysRect. */

	PSysInvalSys(doc,sysL);

#define SHOULDNT_NEED_THIS
#ifdef SHOULDNT_NEED_THIS
	InvalWindow(doc);
#endif

	UpdateVoiceTable(doc, TRUE);
	MEAdjustCaret(doc,TRUE);
}


static void PFixGraphicFont(Document *doc, LINK startL, LINK endL)
{
	LINK pL;
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (GraphicTYPE(pL)) FixGraphicFont(doc, pL);
}


/*
 * Paste the system in the clipboard into the document before the page,
 * system or tail following the system in doc containing the insertion
 * point. Note the contrast with PastePages, which pastes the pages BEFORE
 * the page containing the insertion point.
 *
 *	Function copies the system into the document, positions it graphically,
 * and fixes up cross links and other status information such as selection
 * LINKs and system numbers.
 */

void PasteSystem(Document *doc)
{
	LINK prevL,insertL,startL,clipSys,lastL,succSys,newTS,
		rSys,rStaff,rMeas,newPage,
		sysL,staffL,measL,newSys,newStf,newMeas,newFirstMeas,firstLMeas;
	short nSystems=0; DRect sysRect,prevSysRect;
	DDIST sysHeight;

	/* System pasted in must have same part-staff format as destination doc. */
	
	if (!CompareScoreFormat(clipboard,doc,0)) {
		GetIndCString(strBuf, SYSPE_STRS, 3);		/* "can't Paste System into a score with different format" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}

	PrepareUndo(doc, doc->selStartL, U_PasteSystem, 48);		/* "Undo Paste System" */

	MEHideCaret(doc);

	/* EditSysGetStartL deals with the fact that doc->selStartL could be any of
		several types of objects, including a PAGE or SYSTEM. These situations could
		arise, for example, if the insertion point is at the end of a system.
		EditSysGetStartL returns, for the given types of objects and situations:
	
		1. A PAGE: the last object before the page.
		2. A SYSTEM: the previous system unless the system is the first on the page, in
			which case the system itself.
		3. If <paste> only, anything before the first measure of a SYSTEM: the system
			before the system containing doc->selStartL unless the system containing
			doc->selStartL is the first on the page, in which case the page.
		4. Anything else (including the tail): doc->selStartL.
	*/

	startL = EditSysGetStartL(doc,TRUE);
	if (PageTYPE(startL)) {
		Paste1stSysInPage(doc, startL);
		return;
	}

	sysL = LSSearch(startL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	prevL = lastL = LastObjInSys(doc, RightLINK(sysL));
	insertL = RightLINK(lastL);

	rSys = LinkRSYS(sysL);
	
	staffL = SSearch(sysL,STAFFtype,GO_RIGHT);
	rStaff = LinkRSTAFF(staffL);

	measL = SSearch(staffL,MEASUREtype,GO_RIGHT);

	InstallDoc(clipboard);

	clipSys = SSearch(clipboard->headL, SYSTEMtype, GO_RIGHT);
	CopyRange(clipboard, doc, clipSys, clipboard->tailL, insertL, TO_SCORE);

	InstallDoc(doc);

	newSys = SSearch(RightLINK(sysL),SYSTEMtype,GO_RIGHT);
	newPage = SSearch(newSys,PAGEtype,GO_LEFT);
	newStf = SSearch(newSys,STAFFtype,GO_RIGHT);
	newMeas = rSys ? SSearch(rSys,MEASUREtype,GO_LEFT) :
							SSearch(doc->tailL,MEASUREtype,GO_LEFT);

	rMeas = rSys ? SSearch(rSys,MEASUREtype,GO_RIGHT) : NILINK;

	SysPAGE(newSys) = newPage;
	LinkRSYS(newSys) = rSys;
	LinkRSTAFF(newStf) = rStaff;
	LinkRMEAS(newMeas) = rMeas;

	/* Get the system height, and move all following systems on its page up. ACCURATE?? */

	sysRect = SystemRECT(newSys);
	sysHeight = sysRect.bottom-sysRect.top;
	prevSysRect = SystemRECT(sysL);

	MoveSystemY(newSys,prevSysRect.bottom-sysRect.top);
	InvalRange(newSys,LastObjInSys(doc,RightLINK(newSys)));

	if (rSys) {
		LinkLSYS(rSys) = newSys;
		LinkLSTAFF(rStaff) = newStf;
		LinkLMEAS(rMeas) = newMeas;
		for (succSys=rSys; succSys; succSys = LinkRSYS(succSys)) {
			if (!SamePage(succSys, newSys)) break;			/* Avoid passing NILINK to SamePage */
			MoveSystemY(succSys,sysHeight);
			InvalSystem(succSys);
			InvalRange(succSys,LastObjInSys(doc,RightLINK(succSys)));
		}
	}
	newFirstMeas = SSearch(newSys,MEASUREtype,GO_RIGHT);
	firstLMeas = SSearch(newSys,MEASUREtype,GO_LEFT);

	LinkLSYS(newSys) = sysL;
	LinkLSTAFF(newStf) = staffL;
	LinkLMEAS(newFirstMeas) = firstLMeas;

	LinkRSYS(sysL) = newSys;
	LinkRSTAFF(staffL) = newStf;
	LinkRMEAS(firstLMeas) = newFirstMeas;

	UpdateSysNums(doc,doc->headL);
	UpdateMeasNums(doc,NILINK);

	newTS = SSearch(newSys,TIMESIGtype,GO_RIGHT);
	if (newTS)
		if (SameSystem(newTS,newSys))
			DeleteNode(doc,newTS);

	FixTimeStamps(doc,newSys,NILINK);
	doc->changed = TRUE;

	DeselRangeNoHilite(doc, doc->headL, doc->tailL);
	lastL = LastObjInSys(doc,RightLINK(newSys));
	doc->selStartL = doc->selEndL = RightLINK(lastL);

	FixDelCrossSysObjs(doc);

	/* Update the context, staff size, and indent for the pasted-in system. */

	PSysFixContext(doc,prevL,insertL);
	PSysFixStfSize(doc,newSys,rSys);
	PSysUpdateSysIndent(doc,newSys);

	/* Update the position of the first invisible measure. */
	
	PSysUpdateFirstMeas(doc,newSys);

	/* Inval the new system's rect */

	if (SamePage(sysL,newSys))
		PSysInvalSys(doc,sysL);
	else
		InvalWindow(doc);

	/* Update the Document's font-mapping table for Graphics in the pasted-in system. */

	PFixGraphicFont(doc,newSys,rSys);

	UpdateVoiceTable(doc, TRUE);
	MEAdjustCaret(doc,TRUE);
}

	
/* If prev, return TRUE if there is a xSys obj (Beamset or Slur) extending from
the previous system to sysL; if not prev, return TRUE if a xSys obj extending from
sysL to the following sys. */

static Boolean GetXSysObj(LINK sysL, Boolean prev)
{
	LINK rSys,pL;
	
	/* Traverse to next sys or NILINK (rightL of tail) */
	rSys = LinkRSYS(sysL);

	for (pL=sysL; pL!=rSys; pL=RightLINK(pL)) {
		if (BeamsetTYPE(pL) && BeamCrossSYS(pL)) {
			if (prev && !BeamFirstSYS(pL)) return TRUE;
			if (!prev && BeamFirstSYS(pL)) return TRUE;
		}
		if (SlurTYPE(pL)) {
			if (prev && SlurLastSYSTEM(pL)) return TRUE;
			if (!prev && SlurFirstSYSTEM(pL)) return TRUE;
		}
	}
	return FALSE;
}

/*
 * Clear the system containing the insertion point from doc. Leaves the insertion
 * point immediately following the first measure of the system after the one
 * cleared, or at the tail. The first system of the score is hard to handle, so we
 * disallow clearing it.
 */

void ClearSystem(Document *doc)
{
	LINK sysL,lastL,startL,pageL,lSys,rSys,staffL,lStaff,rStaff,lMeas,rMeas,succSys;
	short nSys,samePage; DRect sysRect; DDIST sysHeight;
	
	MEHideCaret(doc);

	/* If there is an insertion point at the end of the system, doc->selStartL
		will be the following system or page object. Call EditSysGetStartL to get
		startL. */
	startL = EditSysGetStartL(doc,FALSE);
	sysL = LSSearch(startL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	lastL = LastObjInSys(doc, RightLINK(sysL));

	if (LinkLSYS(sysL)==NILINK) {
		GetIndCString(strBuf, SYSPE_STRS, 1);		/* "can't Clear the first system" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}

	pageL = SysPAGE(sysL);
	nSys = NSysOnPage(pageL);
	if (nSys==1) {
		GetIndCString(strBuf, SYSPE_STRS, 2);		/* "can't Clear the only system on the page" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}

	PrepareUndo(doc, NILINK, U_ClearSystem, 40);						/* "Undo Clear System" */

	/* Get the system height in order to move following systems on its page up. */
	sysRect = SystemRECT(sysL);
	sysHeight = sysRect.bottom - sysRect.top;

	/* Set the selStartL one link to the right of the last object in
		the system to be deleted. */
	lSys = LinkLSYS(sysL);
	rSys = LinkRSYS(sysL);

	staffL = SSearch(sysL,STAFFtype,GO_RIGHT);
	rStaff = LinkRSTAFF(staffL);
	lStaff = LinkLSTAFF(staffL);

	rMeas = rSys ? SSearch(rSys,MEASUREtype,GO_RIGHT) : NILINK;
	lMeas = lSys ? LSSearch(sysL,MEASUREtype,ANYONE,GO_LEFT,FALSE) : NILINK;

	/* Inval the system before deleting it while its systemRect still
		exists. */
	InvalSystem(sysL);
	if (lSys) {
		if (GetXSysObj(sysL,TRUE))
			InvalSystem(lSys);
	}
	if (rSys) {
		if (GetXSysObj(sysL,FALSE))
			InvalSystem(rSys);
	}
	
	/* Determine whether sysL is last system on its page before
		deleting it. */
	samePage = rSys ? SamePage(sysL, rSys) : FALSE;

	DeleteRange(doc,sysL,RightLINK(lastL));
	
	if (lSys) {
		LinkRSYS(lSys) = rSys;
		LinkRSTAFF(lStaff) = rStaff;
		LinkRMEAS(lMeas) = rMeas;
	}
	if (rSys) {
		LinkLSYS(rSys) = lSys;
		LinkLSTAFF(rStaff) = lStaff;
		LinkLMEAS(rMeas) = lMeas;
		succSys = rSys;
		if (samePage)
			for ( ; succSys; succSys = LinkRSYS(succSys)) {
				if (!SamePage(succSys, rSys)) break;			/* Avoid passing NILINK to SamePage */
				MoveSystemY(succSys,-sysHeight);
				InvalSystem(succSys);
				InvalRange(succSys,LastObjInSys(doc,RightLINK(succSys)));
			}

		doc->selStartL = LSSearch(rSys,MEASUREtype,ANYONE,GO_RIGHT,FALSE);
		doc->selStartL = RightLINK(doc->selStartL);
	}
	else
		doc->selStartL = doc->tailL;

	doc->selEndL = doc->selStartL;
	doc->changed = TRUE;
	UpdateSysNums(doc, doc->headL);
	UpdateMeasNums(doc,NILINK);
	FixTimeStamps(doc,(lSys? EndSystemSearch(doc, lSys) : pageL),NILINK);
	
	FixDelCrossSysObjs(doc);
	MEAdjustCaret(doc,TRUE);
}


/* --------------------------------------------------------------------------------- */
/* Editing operations on pages. */

/*
 * Fix up the initial context of a page inserted after prevL by adding context
 * objects to the end of the previous page.
 */

static Boolean PPageFixInitContext(Document *doc, LINK prevL, PCONTEXT oldContext,
												PCONTEXT newContext)
{
	short s,nEntries,nMeas;
	Boolean staffMap[MAXSTAVES+1];
	LINK firstMeasL,lastMeasL,insertL,rightL,sysL;
	RMEASDATA *rmTable;

	ZeroMem(oldContext,(long)sizeof(CONTEXT)*(MAXSTAVES+1));
	ZeroMem(newContext,(long)sizeof(CONTEXT)*(MAXSTAVES+1));

	/* The old context is the context in force at the end of the page
		before the contexted page, i.e., at prevL. */
	insertL = rightL = RightLINK(prevL);
	GetAllContexts(doc,oldContext,prevL);
	
	/* The new context is gotten at the first invisible measure of the
		contexted page. */
	firstMeasL = LSSearch(rightL,MEASUREtype,ANYONE,GO_RIGHT,FALSE);
	GetAllContexts(doc,newContext,firstMeasL);

	/* Determine the staves for which the clef context has changed,
		and mark those staves in the staffMap. Then use the staffMap
		and old and new clef contexts to create a clef object with
		one subObj on each marked staff, whose clefType is the same
		as that of the new context. */
		
	InitStfMap(staffMap);
	for (s=1; s<=MAXSTAVES; s++) {
		if (oldContext[s].clefType!=newContext[s].clefType)
			staffMap[s] = TRUE;
	}
	nEntries = CountStfMap(staffMap);
	if (nEntries>0) {
		insertL = CreateClefContext(doc,rightL,nEntries,staffMap,newContext);
		if (!insertL) return FALSE;
	}

	/* Do same for keySigs as for clefs. */
	InitStfMap(staffMap);
	for (s=1; s<=MAXSTAVES; s++) {
		if (!KeySigCompare(&oldContext[s],&newContext[s]))
			staffMap[s] = TRUE;
	}
	nEntries = CountStfMap(staffMap);
	if (nEntries>0) {
		insertL = CreateKSContext(doc,rightL,nEntries,staffMap,oldContext,newContext);
		if (!insertL) return FALSE;
	}

	/* Do same for timeSigs as for clefs. */
	InitStfMap(staffMap);
	for (s=1; s<=MAXSTAVES; s++) {
		if (oldContext[s].timeSigType!=newContext[s].timeSigType ||
				oldContext[s].numerator!=newContext[s].numerator ||
				oldContext[s].denominator!=newContext[s].denominator)
			staffMap[s] = TRUE;
	}
	nEntries = CountStfMap(staffMap);
	if (nEntries>0)
		if (!CreateTSContext(doc,rightL,nEntries,staffMap,newContext)) return FALSE;
	
	/* Respace the previous system to make room for added context objects,
		in case they extend off the end of the system. */
	sysL = LSSearch(prevL,SYSTEMtype,ANYONE,GO_LEFT,FALSE);
	rmTable = GetRMeasTable(doc,sysL,&nMeas);
	if (rmTable) {
		RespaceSystem(doc,sysL,rmTable,nMeas);
		DisposePtr((Ptr)rmTable);
	}

	/* Fix up measure rect x's for added system. Not clear why this is needed. */
	
	sysL = LSSearch(prevL,SYSTEMtype,ANYONE,GO_RIGHT,FALSE);
	firstMeasL = SSearch(sysL,MEASUREtype,GO_RIGHT);
	lastMeasL = GetLastMeasInSys(sysL);
	FixMeasRectXs(firstMeasL,lastMeasL);

	InvalSystem(prevL);
	return TRUE;
}

/*
 * Fix up the context for one or more pages inserted between prevL and succL
 * by calling PPageFixInitContext to fix the context boundaries at the start
 * of the first and end of the last inserted pages.
 */

static void PPagesFixContext(Document *doc, LINK prevL, LINK succL, Boolean newFirstPage)
{
	PCONTEXT oldContext,newContext;
	LINK rightL,nextPageL;

	rightL = RightLINK(prevL);

	oldContext = AllocContext();
	if (!oldContext) goto done;
	newContext = AllocContext();
	if (!newContext) goto done;

	/* Fix up boundary of previous and first inserted page, unless page was
		inserted at start of score. */
	
	if (!newFirstPage)
		if (!PPageFixInitContext(doc,prevL,oldContext,newContext))
			goto done;
		
	/* Fix up boundary of last inserted and following page, unless page was
		inserted at end of score. */
		
	nextPageL = LSSearch(succL,PAGEtype,ANYONE,GO_RIGHT,FALSE);
	if (nextPageL)
		PPageFixInitContext(doc,LeftLINK(succL),oldContext,newContext);

done:
	if (oldContext) DisposePtr((Ptr)oldContext);
	if (newContext) DisposePtr((Ptr)newContext);
}


/*
 * Copy the page(s) containing the insertion point or sel. range to the clipboard.
 */

void CopyPages(Document *doc)
{
	LINK pageL,lastL,pL,insertL,lastPageL; short nSystem, sheetNum;
	
	WaitCursor();
	
	ReplaceHeader(doc,clipboard);
	SetupClipDoc(doc,FALSE);

	pageL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_LEFT, FALSE);
	lastPageL = LSSearch(LeftLINK(doc->selEndL), PAGEtype, ANYONE, GO_LEFT, FALSE);
	lastL = LastObjOnPage(doc, lastPageL);
	insertL = RightLINK(lastL);
	
	InstallDoc(clipboard);
	DeleteRange(clipboard, RightLINK(clipboard->headL), clipboard->tailL);
	
	CopyRange(doc, clipboard, pageL, insertL, clipboard->tailL, TO_CLIP);
	
	InstallDoc(clipboard);
	
	for (sheetNum = 0, pL=clipboard->headL; pL; pL = RightLINK(pL))
		if (PageTYPE(pL)) {
			SheetNUM(pL) = sheetNum++;
		}

	for (nSystem = 1, pL=clipboard->headL; pL; pL = RightLINK(pL))
		if (SystemTYPE(pL)) {
			SystemNUM(pL) = nSystem++;
		}

	clipboard->numSheets = sheetNum;
	clipboard->nsystems = nSystem-1;

	clipboard->selEndL = clipboard->selStartL =
		LSUSearch(clipboard->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);

	CopyFontTable(doc);

	FixDelCrossSysObjs(clipboard);
	InstallDoc(doc);
}

/*
 * Get the LINK before which to insert a page. If the insertion point is before
 *	the first measure of a page, we want to insert the page before that page;
 *	therefore, return that page. Otherwise, we want to insert the page after the
 * insertion point's page, so return the page following its page.
 */

static LINK GetPageInsertL(Document *doc, Boolean *newFirstPage)
{
	LINK pageL;

	*newFirstPage = FALSE;

	/* Insertion point is at end of last page; return tailL in order to
		insert page at end of score. */
	if (TailTYPE(doc->selStartL))
		return doc->selStartL;
	
	/* Insertion point is at end of a page; doc->selStartL is the
		following page object. */
	if (PageTYPE(doc->selStartL))
		return doc->selStartL;
	
	if (BeforeFirstPageMeas(doc->selStartL)) {
		pageL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_LEFT, FALSE);
		if (!LinkLPAGE(pageL))
			*newFirstPage = TRUE;
	}
	else
		pageL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_RIGHT, FALSE);
	
	/* If found no page, insertion point was after the first measure
		on the last page, so insert at end of score. */
	return (pageL ? pageL : doc->tailL);
}

/*
 * Paste the page(s) from the clipboard into the document. The pages in the
 * clipboard do not have to have been put there by means of Copy Page, i.e.,
 * this function can be called after Copy or Copy System. If insertion point
 * or selstartL is before the first measure of a page, the pages will be
 * pasted before that page, else after it.
 */

void PastePages(Document *doc)
{
	LINK prevL,insertL,oldFirstSys,pageL,sysL,masterSysL,
			timeSigL,clipTimeSigL,clipSys,newFirstMeas;
	Boolean newFirstPage=FALSE;
	DDIST change;
	SearchParam pbSearch;
	Rect paper, result; short pageNum; Boolean appending;

	/* The pages to be pasted in must contain systems with same part-staff format as
		the destination doc. */
	if (!CompareScoreFormat(clipboard,doc,0)) {
		GetIndCString(strBuf, SYSPE_STRS, 5);		/* "can't Paste Pages into a score with different format" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}

	DisableUndo(doc, FALSE);

	MEHideCaret(doc);

	insertL = GetPageInsertL(doc,&newFirstPage);
	appending = TailTYPE(insertL);
	prevL = LeftLINK(insertL);
	if (newFirstPage) {
		oldFirstSys = LSSearch(insertL,SYSTEMtype,ANYONE,GO_RIGHT,FALSE);

		InitSearchParam(&pbSearch);
		pbSearch.voice = ANYONE;
		pbSearch.needSelected = FALSE;
		pbSearch.inSystem = TRUE;
		timeSigL = L_Search(RightLINK(oldFirstSys), TIMESIGtype, GO_RIGHT, &pbSearch);

		if (TimeSigINMEAS(timeSigL)) timeSigL = NILINK;
	}

	InstallDoc(clipboard);

	/* Copy the clipboard into the score between <prevL> and <insertL>. */
	
	CopyRange(clipboard, doc, RightLINK(clipboard->headL), clipboard->tailL, insertL, TO_SCORE);

	clipSys = SSearch(clipboard->headL, SYSTEMtype, GO_RIGHT);

	InitSearchParam(&pbSearch);
	pbSearch.voice = ANYONE;
	pbSearch.needSelected = FALSE;
	pbSearch.inSystem = TRUE;
	clipTimeSigL = L_Search(RightLINK(clipSys), TIMESIGtype, GO_RIGHT, &pbSearch);

	if (TimeSigINMEAS(clipTimeSigL)) clipTimeSigL = NILINK;

	InstallDoc(doc);
	
	/* Put the insertion point at the end of the pages pasted in. */
	doc->selStartL = doc->selEndL = insertL;

	UpdatePageNums(doc);
	UpdateSysNums(doc,doc->headL);
	UpdateMeasNums(doc,NILINK);
	FixTimeStamps(doc,prevL,NILINK);
	doc->changed = TRUE;

	/* Update the staff rastral size for the pasted-in pages. */

	masterSysL = SSearch(doc->masterHeadL,SYSTEMtype,GO_RIGHT);

	if (clipboard->srastral!=doc->srastral) {
		LINK masterStfL,newSysL,newStfL,rPageL;

		pageL = SSearch(prevL,PAGEtype,GO_RIGHT);
		rPageL = SSearch(LeftLINK(insertL),PAGEtype,GO_RIGHT);

		PasteFixStfSize(doc, pageL, rPageL ? rPageL : doc->tailL);
		masterStfL = SSearch(masterSysL,STAFFtype,GO_RIGHT);
		
		newSysL = SSearch(pageL,SYSTEMtype,GO_RIGHT);
		
		for ( ; newSysL && SysPAGE(newSysL)!=rPageL; newSysL=LinkRSYS(newSysL)) {
			newStfL = SSearch(newSysL,STAFFtype,GO_RIGHT);
			PSysFixStfSizeParams(newSysL,masterSysL,newStfL,masterStfL);
		}
	}
		
	/* Here, fix up system indents for the old first page which was moved
		down to the position of the second page; if not inserting a new first
		page, fix up the sys indent for the (possibly) previous first system
		in the page pasted in. If that system was not a first-in-score, fixing
		it up here will not mess up its position. */

	if (newFirstPage)
		ChangeSysIndent(doc, oldFirstSys, doc->otherIndent-doc->firstIndent);
	else {
		pageL = SSearch(prevL,PAGEtype,GO_RIGHT);
		sysL = SSearch(pageL,SYSTEMtype,GO_RIGHT);

		masterSysL = SSearch(doc->masterHeadL,SYSTEMtype,GO_RIGHT);

		change = SysRectLEFT(masterSysL) - SysRectLEFT(sysL);
		ChangeSysIndent(doc, sysL, change);
	}

	FixDelCrossSysObjs(doc);
	
	/* If the newly pasted system is the first in the score, and this system
		had no timesig, move the timesig from the previous firstInScore system
		to the gutter of the new system.
		If the newly pasted system is the first in the score, and this system
		had a timesig, and the previous first system also had a timesig, 
		cut the timesig from the previous firstInScore system. */

	if (newFirstPage && timeSigL && !clipTimeSigL) {
		newFirstMeas = SSearch(doc->headL,MEASUREtype,GO_RIGHT);
		MoveNode(timeSigL,newFirstMeas);
	}
	else if (newFirstPage && timeSigL && clipTimeSigL)
		CutNode(timeSigL);

	PPagesFixContext(doc,prevL,insertL,newFirstPage);

	/* Update the Document's font-mapping table for Graphics in the pasted-in pages. */

	PFixGraphicFont(doc,pageL,insertL);

	UpdateVoiceTable(doc, TRUE);

	DeselRangeNoHilite(doc,doc->headL,doc->tailL);
	InvalRange(prevL,insertL);
	InvalWindow(doc);
	GetAllSheets(doc);
	RecomputeView(doc);		
	MEAdjustCaret(doc,TRUE);

	GetSheetRect(doc,SheetNUM(pageL),&paper);
	SectRect(&paper, &doc->viewRect, &result);
	if (!appending || !EqualRect(&paper, &result)) {
		pageNum = SheetNUM(pageL)+doc->firstPageNumber;
		ScrollToPage(doc, pageNum);
	}
}


/*
 * Clear the page(s) in doc containing the insertion point or selection. The
 * first page of the score is hard to handle, so we disallow clearing it.
 *
 * Function must re-set the crosslinks for systems, staves and measures in
 * the previous page and the following page if it exists. To do this, it
 * should call FixStructureLinks, but it can't because FixStructureLinks
 * is designed to fix up only ranges whose startL is preceded by a system.
 * ??This should be checked.
 */

void ClearPages(Document *doc)
{
	LINK pageL,sysL,staffL,measL,lastL,startL,endL,currSys,
			rPageL,rSysL,rStaffL,rMeasL,lPageL,lSysL,lStaffL,lMeasL;

	WaitCursor();
	MEHideCaret(doc);

	/* If there is an insertion point at the end of the page, doc->selStartL
		will equal the following page object; in this case, start the search
		one link to the left. */

	startL = PageTYPE(doc->selStartL) ? LeftLINK(doc->selStartL) : doc->selStartL;

	pageL = LSSearch(startL, PAGEtype, ANYONE, GO_LEFT, FALSE);
	lPageL = LinkLPAGE(pageL);
	if (!lPageL) {
		GetIndCString(strBuf, SYSPE_STRS, 4);		/* "can't Clear the first page of the score" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		MEAdjustCaret(doc,TRUE);
		return;
	}
	
	sysL = SSearch(pageL,SYSTEMtype,GO_RIGHT);
	staffL = SSearch(sysL,STAFFtype,GO_RIGHT);
	measL = SSearch(staffL,MEASUREtype,GO_RIGHT);

	lSysL = LinkLSYS(sysL);
	lStaffL = LinkLSTAFF(staffL);
	lMeasL = LinkLMEAS(measL);

	rPageL = LSSearch(LeftLINK(doc->selEndL), PAGEtype, ANYONE, GO_LEFT, FALSE);
	rPageL = LinkRPAGE(rPageL);

	doc->selStartL = pageL;
	doc->selEndL = rPageL ? rPageL : doc->tailL;
	PrepareUndo(doc, NILINK, U_ClearPages, 41);						/* "Undo Clear Pages" */

	PPageFixDelSlurs(doc);

	if (rPageL) {
		rSysL = SSearch(rPageL,SYSTEMtype,GO_RIGHT);
		rStaffL = SSearch(rSysL,STAFFtype,GO_RIGHT);
		rMeasL = SSearch(rStaffL,MEASUREtype,GO_RIGHT);

	/* If the rPageL exists, get the last system prior to it, and the last obj
		in that system, which is the last obj in pageL. */

		sysL = LSSearch(rPageL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
		lastL = LastObjInSys(doc, RightLINK(sysL));
		doc->selStartL = rPageL;
	}
	else {
		rSysL = rStaffL = rMeasL = NILINK;
		lastL = LeftLINK(doc->tailL);
		doc->selStartL = doc->tailL;
	}

	DeleteRange(doc,pageL,RightLINK(lastL));

	LinkRPAGE(lPageL) = rPageL;
	LinkRSYS(lSysL) = rSysL;
	LinkRSTAFF(lStaffL) = rStaffL;
	LinkRMEAS(lMeasL) = rMeasL;
	if (rPageL) {
		LinkLPAGE(rPageL) = lPageL;
		LinkLSYS(rSysL) = lSysL;
		LinkLSTAFF(rStaffL) = lStaffL;
		LinkLMEAS(rMeasL) = lMeasL;
	}

	UpdatePageNums(doc);
	doc->currentSheet = SheetNUM(lPageL);

	UpdateSysNums(doc,doc->headL);
	currSys = LSSearch(doc->selStartL,SYSTEMtype,ANYONE,GO_LEFT,FALSE);
	doc->currentSystem = SystemNUM(currSys);

	UpdateMeasNums(doc,NILINK);
	FixTimeStamps(doc,doc->selStartL,NILINK);

	doc->selEndL = doc->selStartL;
	doc->changed = TRUE;

	/* Set selection range for FixDelCrossSysObjs. */

	endL = rPageL ? ( LinkRPAGE(rPageL) ? LinkRPAGE(rPageL) : doc->tailL) : doc->tailL;
	doc->selStartL = lPageL;
	doc->selEndL = endL;

	FixDelCrossSysObjs(doc);
	GetAllSheets(doc);
	RecomputeView(doc);

	InvalWindow(doc);
	doc->selEndL = doc->selStartL = doc->tailL;
	MEAdjustCaret(doc,TRUE);
}
