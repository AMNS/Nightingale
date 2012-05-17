/***************************************************************************
*	FILE:	Select.c																				*
*	PROJ:	Nightingale, rev. for v.3.5													*
*	DESC:	Selection-related routines.													*
		
	DeselectSTAFF			DeselectCONNECT		SetDefaultSelection
	DeselAll					DeselRange
	DeselAllNoHilite		DeselRangeNoHilite	DoOpenSymbol
	DoAccumSelect			DoExtendSelect			DoPageSelect
	SetInsPoint				DeselVoice				DoSelect
	SelectAll				SelAllNoHilite			DeselectNode
	SelAllSubObjs			SelectObject			SelectRange
	ExtendSelection		ObjTypeSel
	BoundSelRange			GetOptSelEnds			CountSelection
	ChordSetSel				ExtendSelChords		ChordHomoSel
	ContinSelection		OptimizeSelection		UpdateSelection	
	GetStfSelRange			GetVSelRange			GetNoteSelRange
	BFSelClearable			XLoadSelectSeg
/***************************************************************************/

/*										NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1992-99 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Local variables and functions */

static void DeselectCONNECT(LINK);
static void DoAccumSelect(Document *, LINK, LINK);
static Boolean SetInsPoint(Document *, Point);
static void DoExtendSelect(Document *, LINK, LINK, INT16, short);
static void DoPageSelect(Document *, Point, LINK, LINK, Boolean, Boolean, short);
static Boolean ContinSelChks(Document *doc);
static void SetStfStatus(INT16 *stStatus, INT16 staff, Boolean sel);
static INT16 NextEverSel(Document *doc, Boolean *stEverSel, INT16 s);
static Boolean CSChkSelRanges(LINK s1, LINK e1, LINK s2, LINK e2);
static Boolean OldContinSel(Document *doc, Boolean strict);
static Boolean NewContinSel(Document *doc);
static void SetSelEnds(LINK,LINK *,LINK *);


/* --------------------------------------------------------------- DeselectSTAFF -- */
/* Crude, but all that's needed, since staves can only be selected during
DeleteParts. */

void DeselectSTAFF(LINK pL)
{
	PASTAFF	aStaff;
	LINK		aStaffL;
	
	aStaffL = FirstSubLINK(pL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		aStaff->selected = FALSE;
	}
}


/* ------------------------------------------------------------- DeselectCONNECT -- */
/* Crude, but all that's needed for DeselAll. */

static void DeselectCONNECT(LINK pL)
{
	PACONNECT	aConnect;
	LINK		aConnectL;
	
	aConnectL = FirstSubLINK(pL);
	for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
		aConnect = GetPACONNECT(aConnectL);
		aConnect->selected = FALSE;
	}
}


/* --------------------------------------------------------- SetDefaultSelection -- */
/* Set empty selection (insertion point) just after the first invisible Measure.
After calling this, it may be desirable to set doc->selStaff, and to call
MEAdjustCaret to position the caret. */

void SetDefaultSelection(Document *doc)
{
	doc->selStartL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	doc->selStartL = RightLINK(doc->selStartL);
	doc->selEndL = doc->selStartL;
}

/* -------------------------------------------------------------------- DeselAll -- */
/*	Deselect current selection, make selection range null, and unhilite. */

void DeselAll(Document *doc)
{
	MEHideCaret(doc);
	
	if (!doc->selStartL || doc->selStartL==doc->selEndL)				/* check for empty selection */
		return;

	DeselRange(doc, doc->selStartL, doc->selEndL);
	SetDefaultSelection(doc);
}


/* ------------------------------------------------------------------ DeselRange -- */
/*	Deselect and unhilite the given range. Leaves the selection range endpoints
untouched. */

void DeselRange(Document *doc, LINK startL, LINK endL)
{
	LINK			pL;
	CONTEXT		context[MAXSTAVES+1];
	Boolean		found;
	INT16			index,oldSheet;
	STFRANGE		stfRange={0,0};
	Rect			oldPaper;
	long			soon;

	if (!startL || startL==endL) return;				/* check for empty selection */

	oldSheet = doc->currentSheet;
	oldPaper = doc->currentPaper;
	
	GetAllContexts(doc, context, startL);				/* get context at start of range */
	if (GraphicTYPE(startL)) {
		pL = GraphicFIRSTOBJ(startL);
		if (PageTYPE(pL)) startL = pL;
	}
	
	soon = TickCount()+60L;

	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (TickCount()>soon) {
			WaitCursor();
			soon += 9999L;		/* So we don't waste much time calling WaitCursor constantly */
		}
		ContextObject(doc, pL, context);
		if (LinkSEL(pL)) {
			/* CheckObject does nothing for Connects. */
			if (ConnectTYPE(pL))
				DeselectCONNECT(pL);
			else
				CheckObject(doc, pL, &found, NULL, context, SMDeselect, &index, stfRange);
			LinkSEL(pL) = FALSE;
		}
	}

	doc->currentSheet = oldSheet;
	doc->currentPaper = oldPaper;
}


/* ------------------------------------------------------------ DeselAllNoHilite -- */
/* Deselect current selection, make selection range null. Don't change screen
hiliting. */

void DeselAllNoHilite(Document *doc)
{
	if (!doc->selStartL || doc->selStartL==doc->selEndL)				/* check for empty selection */
		return;

	DeselRangeNoHilite(doc, doc->selStartL, doc->selEndL);
	SetDefaultSelection(doc);
}


/* ---------------------------------------------------------- DeselRangeNoHilite -- */
/* Deselect given range.  Don't change screen hiliting. Leaves the selection
range endpoints untouched. */

void DeselRangeNoHilite(Document */*doc*/, LINK startL, LINK endL)
{
	LINK pL;

	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (LinkSEL(pL))										/* We need LinkSEL bcs DeselectNode can't handle Pages, etc. */
 			DeselectNode(pL);
}


/* ---------------------------------------------------------------- DoOpenSymbol -- */
/* Open symbol for editing, i.e., handle double clicks. Returns LINK of object
opened, or NILINK if no object found for mouse click. */

LINK DoOpenSymbol(Document *doc, Point pt)
{
	INT16 index; LINK pL;

	FindStaff(doc, pt);						/* Just to set doc->currentSystem */

	pL = FindObject(doc, pt, &index, SMFind);
	if (pL)
		if (BeamsetTYPE(pL) || SlurTYPE(pL))
			DeselAll(doc);

	pL = FindObject(doc, pt, &index, SMDblClick);
	if (pL) {
		LimitSelRange(doc);
	}

	return pL;
}

/* --------------------------------------------------------------- DoAccumSelect -- */
/*	Handle user cmd-selection: accumulate selected objects into selection range,
optimize selection, and set blinking caret if user deselected everything. The
resulting selection can be discontinuous. */

static void DoAccumSelect(Document	*doc, LINK oldSelStartL, LINK oldSelEndL)
{
	if (IsAfterIncl(oldSelStartL, doc->selStartL))		/* accumulate new selection */
		doc->selStartL = oldSelStartL;
	if (IsAfterIncl(doc->selEndL, oldSelEndL))
		doc->selEndL = oldSelEndL;
	OptimizeSelection(doc);
	if (doc->selStartL==doc->selEndL) MEAdjustCaret(doc,TRUE);
}


/* -------------------------------------------------------------- DoExtendSelect -- */
/*	Handle user shift-selection: accumulate selected objects into selection range,
optimize selection, and set blinking caret if user deselected everything.
The resulting selection will be continuous. */

static void DoExtendSelect(
					Document	*doc,
					LINK		oldSelStartL,
					LINK		oldSelEndL,
					INT16		oldSelStaff,
					short		mode 						/* SMSelect or SMThread */
					)
{
	LINK		pL, selStartPageL, selStartSysL, selStartMeasL, firstMeasL;
	Boolean	found;
	short		sheet, oldSheet;
	INT16		index;
	Rect		paper, oldPaper, selRect;
	STFRANGE	stfRange;
	CONTEXT	context[MAXSTAVES+1];
	long soon;
	
	if (IsAfterIncl(oldSelStartL, doc->selStartL))		/* accumulate new selection */
		doc->selStartL = oldSelStartL;
	if (IsAfterIncl(doc->selEndL, oldSelEndL))
		doc->selEndL = oldSelEndL;

	if (doc->selStartL==doc->selEndL) {
		MEAdjustCaret(doc, TRUE);
		return;														/*???is this right? */
	}
	
	soon = TickCount()+60L;

	/* Extend selection to include J_D symbols to left of sync. */
	if (SyncTYPE(doc->selStartL) || J_DTYPE(doc->selStartL))	{ 			/*???should I include other types? */
		for (pL = LeftLINK(doc->selStartL); J_DTYPE(pL); pL = LeftLINK(pL))
			doc->selStartL = pL;
	}
		
	if (PageTYPE(doc->selStartL))							/* TRUE if called from DoPageSelect */
		selStartPageL = doc->selStartL;
	else
		selStartPageL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_LEFT, FALSE);
	selStartSysL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	firstMeasL = LSSearch(selStartSysL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	GetAllContexts(doc, context, firstMeasL);

	selStartMeasL = LSSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	ContextObject(doc, selStartMeasL, context);

	oldSheet = doc->currentSheet;
	oldPaper = doc->currentPaper;

	/*
	 * We'll use CheckObject with mode SMStaffDrag to select everything in the staff
	 * range. Set graphic bounds to "infinity", since we want it to select regardless
	 * of graphic position. (It might be better to define a new mode that doesn't even
	 * consider graphic restrictions; it'd probably run faster, at least.)
	 */
	SetRect(&selRect, -32767, -32767, 32767, 32767);											

	GetStfRangeOfSel(doc, &stfRange);
	stfRange.topStaff = min3(stfRange.topStaff, oldSelStaff, doc->selStaff);
	stfRange.bottomStaff = max3(stfRange.bottomStaff, oldSelStaff, doc->selStaff);

	pL = doc->selStartL;
	
	if (GraphicTYPE(pL)) {							/* If doc->selStartL is a page-rel graphic... */
		LINK fooL = GraphicFIRSTOBJ(pL);
		if (PageTYPE(fooL)) pL = fooL;			/* ... pL=its page, so that we get correct pg context below */
	}

	for (pL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (PageTYPE(pL)) {
			sheet = SheetNUM(pL);
			GetSheetRect(doc, sheet, &paper);
			doc->currentSheet = sheet;
			doc->currentPaper = paper;
		}

		if (TickCount()>soon) WaitCursor();

		ContextObject(doc, pL, context);
		if (VISIBLE(pL)) {					/* ???but shouldn't invis objs be selected too? */
			if (mode==SMThread)
				if (ObjLType(pL)!=threadableType) continue;
			CheckObject(doc, pL, &found, (Ptr)&selRect, context, SMStaffDrag,
							&index, stfRange);
			if (found) LinkSEL(pL) = TRUE;							/* mark new selection */
		}
	}
	
	doc->currentSheet = oldSheet;
	doc->currentPaper = oldPaper;
}


/* ----------------------------------------------------------------- DoPageSelect -- */
/*	Handle selection of objects attached to page. */

static void DoPageSelect(
					Document	*doc,
					Point 	pt,
					LINK		oldSelStartL,
					LINK		oldSelEndL,
					Boolean	shiftFlag,
					Boolean	cmdFlag,
					short		/*mode*/
					)
{
	LINK			pL;
	INT16			index;
		
	pL = FindObject(doc, pt, &index, SMClick);		/* Look for PAGEs to which to attach GRAPHICs */
	if (pL) {
		doc->selStartL = pL; doc->selEndL = RightLINK(pL);
		if (shiftFlag)
			;														/* Formerly called DoExtendSelect here */
		else if (cmdFlag)
			DoAccumSelect(doc, oldSelStartL, oldSelEndL);
	}
	else if (!(cmdFlag || shiftFlag))
		/* Deselect everything here. We'd prefer to call FixEmptySelection, but it won't
			do anything, since the point isn't in a staff. ??However, we still need to
			call MESetCaret somehow... */
		DeselAll(doc);
}


/* ----------------------------------------------------------------- SetInsPoint -- */
/* Set the selection range and staff, plus doc->currentSystem, appropriately for a
click that sets an insertion point at the given point. NB: does NOT check that the
click really is setting an insertion point (i.e., isn't within an object or a click
with modifier keys) or move the caret. Ordinarily, use FixEmptySelection instead. */

Boolean SetInsPoint(Document *doc, Point	pt)
{
	INT16 staffn;

	staffn = FindStaff(doc, pt);						/* Also sets doc->currentSystem */
	if (staffn==NOONE) return FALSE;					/* Click wasn't within any system */

	doc->selStaff = staffn;

	doc->selStartL = doc->selEndL = Point2InsPt(doc, pt);
	return TRUE;
}

/* -------------------------------------------------------------------- DoSelect -- */
/*	Let user indicate what they want selected, and select it. */

void DoSelect(
			Document *doc,
			Point		pt,						/* Mousedown point */
			Boolean	shiftFlag,				/* Accumulate selection */
			Boolean	cmdFlag,					/* Extend selection, as in TextEdit */
			Boolean	/*capsLockFlag*/,		/* unused */
			short		mode 						/* SMThread or SMSelect */
			)
{
	LINK		pL, oldSelStartL, oldSelEndL;
	INT16		index, oldSelStaff;

	oldSelStartL = doc->selStartL;
	oldSelEndL = doc->selEndL;
	oldSelStaff = GetSelStaff(doc);

	if (doc->selStartL==doc->selEndL) cmdFlag = FALSE;		/* Cmd-click with empty selection makes no sense */		

	if (!cmdFlag && !shiftFlag) 
		DeselAll(doc);													/* Deselect previous selection */
	
	if (mode!=SMThread && FindStaff(doc, pt)==NOONE) {		/* Mousedown not inside any staff */
		DoPageSelect(doc, pt, oldSelStartL, oldSelEndL,
									shiftFlag, cmdFlag, mode);		/* Handle selection of pageRel objs */
		return;
	}

	if (mode==SMThread)
		DoThreadSelect(doc, pt);								/* Handle threader selection */
	else {
		if (!SelectStaffRect(doc, pt))						/* Try one-dimensional wipe selection */
		{																/* Mouse not moved horizontally, so */
			pL = FindObject(doc, pt, &index, SMClick);	/*  select object clicked in, if any */
			if (pL) 
				{ doc->selStartL = pL; doc->selEndL = RightLINK(pL); }
			 else {
				if (!shiftFlag) 									/* ??what if cmdFlag? */
					FixEmptySelection(doc, pt);
				else
					SetInsPoint(doc, pt);
			}
		}
	
	}
	if (shiftFlag)									/* extend selection continuously, as in TextEdit */
		DoExtendSelect(doc, oldSelStartL, oldSelEndL, oldSelStaff, mode);
	else if (cmdFlag)								/* accumulate new, possibly discontinuous selection (formerly shiftFlag) */
		DoAccumSelect(doc, oldSelStartL, oldSelEndL);
}


/* -------------------------------------------------------------------- SelectAll -- */
/*	Select and hilite all objects in score. Handles all user-interface details,
including redrawing the message box. */

void SelectAll(register Document *doc)
{
	LINK		pL, pageL;
	Boolean	found;
	CONTEXT	context[MAXSTAVES+1];	/* current context table */
	INT16		index, oldCurrentSheet, sheet;
	Rect		paper,oldPaper;
	STFRANGE	stfRange={0,0};

	MEHideCaret(doc);
	WaitCursor();

	DeselAll(doc);
	pageL = LSSearch(doc->headL, PAGEtype, ANYONE, GO_RIGHT, FALSE);

	oldPaper = doc->currentPaper;
	oldCurrentSheet = doc->currentSheet;

	sheet = SheetNUM(pageL);
	GetSheetRect(doc,sheet,&paper);
	doc->currentSheet = sheet;
	doc->currentPaper = paper;

	pL = doc->selStartL = RightLINK(doc->headL);
	doc->selEndL = doc->tailL;

	while (pL!=doc->selEndL) {
		if (PageTYPE(pL)) {
			sheet = SheetNUM(pL);
			GetSheetRect(doc,sheet,&paper);
			doc->currentSheet = sheet;
			doc->currentPaper = paper;
		}
		ContextObject(doc, pL, context);
		if (VISIBLE(pL)) {
			found = FALSE;
			CheckObject(doc, pL, &found, NULL, context, SMSelect, &index, stfRange);
			if (found) LinkSEL(pL) = TRUE;							/* mark new selection */
		}
		pL = RightLINK(pL);
	}
	ArrowCursor();
	doc->currentSheet = oldCurrentSheet;
	doc->currentPaper = oldPaper;
	EraseAndInvalMessageBox(doc);
}


#ifdef JG_NOTELIST
/* ------------------------------------------------------------- SelRangeNoHilite -- */
/* Select all visible objects in the range (startL, endL]; doesn't change 
hiliting or make any other user-interface assumptions. Written for use 
in file importing routines. */

void SelRangeNoHilite(Document *doc, LINK startL, LINK endL)
{
	LINK		pL;
	Boolean	found;
	CONTEXT	context[MAXSTAVES+1];	/* current context table */
	INT16		index;
	STFRANGE	stfRange={0,0};

	DeselAllNoHilite(doc);
	pL = doc->selStartL = startL;
	doc->selEndL = endL;

	while (pL!=doc->selEndL) {
		ContextObject(doc, pL, context);
		if (VISIBLE(pL)) {
			found = FALSE;
			/* stfRange is garbage (unitialized), but it's not used in mode <SMSelNoHilite>. */
			CheckObject(doc, pL, &found, NULL, context, SMSelNoHilite, &index, stfRange);
			if (found) LinkSEL(pL) = TRUE;							/* mark new selection */
		}
		pL = RightLINK(pL);
	}
}
#endif


/* --------------------------------------------------------------- SelAllNoHilite -- */
/* Select all objects in score; doesn't change hiliting or make any other
user-interface assumptions. Written for use in file importing routines. */

void SelAllNoHilite(register Document *doc)
{
	LINK		pL;
	Boolean	found;
	CONTEXT	context[MAXSTAVES+1];	/* current context table */
	INT16		index;
	STFRANGE	stfRange={0,0};

	DeselAllNoHilite(doc);
	pL = doc->selStartL = RightLINK(doc->headL);
	doc->selEndL = doc->tailL;

	while (pL!=doc->selEndL) {
		ContextObject(doc, pL, context);
		if (VISIBLE(pL)) {
			found = FALSE;
			CheckObject(doc, pL, &found, NULL, context, SMSelNoHilite, &index, stfRange);
			if (found) LinkSEL(pL) = TRUE;							/* mark new selection */
		}
		pL = RightLINK(pL);
	}
}


/* ---------------------------------------------------------------- DeselectNode -- */
/* Deselect pL and all its subobjects, if it has any and they can be selected. */

void DeselectNode(LINK pL)
{	
	HEAP		*tmpHeap;
	LINK		subObjL;
	PMEVENT	p;
	GenSubObj *subObj;
	
	if (LinkSEL(pL))
		LinkSEL(pL) = FALSE;										/* Deselect the object */
		
	switch (ObjLType(pL)) {
		case STAFFtype: {
			LINK aStaffL;
				for (aStaffL=FirstSubLINK(pL); aStaffL; aStaffL=NextSTAFFL(aStaffL))
					StaffSEL(aStaffL) = FALSE;
			}
			break;
		case CONNECTtype: {
			LINK aConnectL;
				for (aConnectL=FirstSubLINK(pL); aConnectL; 
						aConnectL=NextCONNECTL(aConnectL))
					ConnectSEL(aConnectL) = FALSE;
			}
			break;
		case SYNCtype:
		case GRSYNCtype:
		case CLEFtype:
		case TIMESIGtype:
		case KEYSIGtype:
		case MEASUREtype:
		case PSMEAStype:
		case DYNAMtype:
		case RPTENDtype:
			tmpHeap = Heap + ObjLType(pL);		/* p may not stay valid during loop */
			
			for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
				subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
				subObj->selected = FALSE;							/* Deselect all the subobjects */
			}
			break;
		case SLURtype: {
			LINK aSlurL;
				for (aSlurL=FirstSubLINK(pL); aSlurL; aSlurL=NextSLURL(aSlurL))
					SlurSEL(aSlurL) = FALSE;
			}
			break;
			
		/* These types either have no subobjects, or the subobjects can't be selected */
		case BEAMSETtype:
		case TUPLETtype:
		case OCTAVAtype:
		case GRAPHICtype:
		case TEMPOtype:
		case SPACEtype:
		case ENDINGtype:
			break;
			
		default:
			MayErrMsg("DeselectNode: type=%ld at %ld not supported", (long)ObjLType(pL),
							(long)pL);
			break;
	}
}


/* --------------------------------------------------------------- SelAllSubObjs -- */
/* Select all the subObjs of pL. Doesn't change hiliting or make any other
user-interface assumptions. Doesn't select the object itself; for that, call
SelectObject instead.

??Problem: tuplets and octavas should be handled the same way as beamsets, which
currently leave the data structure in an  inconsistent state: the code will select
the sync object without selecting any of its subobjects, and without choosing the
correct subobject to select. */

void SelAllSubObjs(LINK pL)
{
	PMEVENT		p;
	HEAP			*tmpHeap;
	LINK			subObjL;
	GenSubObj 	*subObj;
	
	switch (ObjLType(pL)) {
		case STAFFtype: {
			LINK aStaffL;
				for (aStaffL=FirstSubLINK(pL); aStaffL; aStaffL=NextSTAFFL(aStaffL))
					StaffSEL(aStaffL) = TRUE;
			}
			break;
		case CONNECTtype: {
			LINK aConnectL;
				for (aConnectL=FirstSubLINK(pL); aConnectL; 
						aConnectL=NextCONNECTL(aConnectL))
					ConnectSEL(aConnectL) = TRUE;
			}
			break;
		case SYNCtype:
		case GRSYNCtype:
		case MEASUREtype:
		case PSMEAStype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case DYNAMtype:
		case RPTENDtype:
			tmpHeap = Heap + ObjLType(pL);		/* p may not stay valid during loop */
			
			for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
				subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
				subObj->selected = TRUE;
			}
			break;
		case SLURtype: {
			LINK aSlurL;
				for (aSlurL=FirstSubLINK(pL); aSlurL; aSlurL=NextSLURL(aSlurL))
					SlurSEL(aSlurL) = TRUE;
			}
			break;
		case BEAMSETtype:
		case OCTAVAtype:
		case TUPLETtype:
		case GRAPHICtype:
		case TEMPOtype:
		case SPACEtype:
		case ENDINGtype:
			break;
		default:
			MayErrMsg("SelAllSubObjs: at %ld, type=%ld not supported",
						(long)pL, (long)ObjLType(pL));
			break;
	}
}


/* ---------------------------------------------------------------- SelectObject -- */

void SelectObject(LINK pL)
{
	LinkSEL(pL) = TRUE;
	SelAllSubObjs(pL);
}


/* ----------------------------------------------------------------- SelectRange -- */
/* For every content object in the range of LINKs [startL, endL), SelectRange
selects the object in the INCLUSIVE range of staves [firstStf, lastStf]: if it or
any of its subobjects are in the range of staves, it selects the object and those
subobjects. It does NOT deselect objects that aren't in the range of staves! Also
doesn't change hiliting or make any other user-interface assumptions. Also doesn't
check doc->lookVoice (if it's non-negative, we're Looking at a voice and this
function should probably not be called).

You'd think a loop calling CheckObject could do the same thing, but probably not
--I don't think any of the CheckXXX routine modes quite do this. */

void SelectRange(Document *doc, LINK startL, LINK endL, short firstStf, short lastStf)
{
	LINK 			pL,subObjL,aSlurL;
	PMEVENT		p;
	GenSubObj 	*subObj;
	HEAP 			*tmpHeap;

	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		p = GetPMEVENT(pL);
		switch (ObjLType(pL)) {
			case SYNCtype:
			case GRSYNCtype:
			case MEASUREtype:
			case PSMEAStype:
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case DYNAMtype:
			case RPTENDtype:
				tmpHeap = doc->Heap + ObjLType(pL);
				
				for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
					if (subObj->staffn>=firstStf && subObj->staffn<=lastStf) {
						LinkSEL(pL) = TRUE;
						subObj->selected = TRUE;
					}
				}	
				break;
			case BEAMSETtype:
			case TUPLETtype:
			case OCTAVAtype:
			case ENDINGtype:
			case SPACEtype:
			case GRAPHICtype:
			case TEMPOtype:
				if (((PEXTEND)p)->staffn>=firstStf && ((PEXTEND)p)->staffn<=lastStf)
					LinkSEL(pL) = TRUE;
				break;

			case SLURtype:
				if (SlurSTAFF(pL)>=firstStf && SlurSTAFF(pL)<=lastStf) {
					LinkSEL(pL) = TRUE;
					aSlurL = FirstSubLINK(pL);
					for ( ; aSlurL; aSlurL=NextSLURL(aSlurL))
						SlurSEL(aSlurL) = TRUE;
				}
				break;
		}
	}
}


/* ------------------------------------------------------------- ExtendSelection -- */
/* For every object that has at least one subobject selected, select all its
subobjects. Handles all user-interface details, assuming the score is on screen. */

void ExtendSelection(Document *doc)
{
	LINK			pL;
	CONTEXT		context[MAXSTAVES+1];
	Boolean		found;
	INT16			index;
	STFRANGE		stfRange={0,0};

	WaitCursor();

	GetAllContexts(doc, context, doc->selStartL);		/* get context at start of range */
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		ContextObject(doc, pL, context);
		if (LinkSEL(pL))
				CheckObject(doc, pL, &found, NULL, context, SMSelect, &index, stfRange);
	}
}


/* ------------------------------------------------------------------ ObjTypeSel -- */
/* If at least one object of the specified type is selected, deliver the first
one's link, else deliver NILINK. */

LINK ObjTypeSel(Document *doc, INT16 type, INT16 graphicSubType)
{
	LINK	pL;
	
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL))
			switch (type) {
				case GRAPHICtype:
					if (ObjLType(pL)==type)
						if (graphicSubType==0 || GraphicSubType(pL)==graphicSubType)
							return pL;
					break;
				default:
					if (ObjLType(pL)==type) return pL;
			}
			
	return NILINK;
}


/* --------------------------------------------------------------- BoundSelRange -- */
/* Deselect any object outside the selection range. */

void BoundSelRange(Document *doc)
{
	LINK pL;
	
	for (pL = doc->headL; pL!=doc->selStartL; pL = RightLINK(pL)) {
		if (LinkSEL(pL))
			DeselectNode(pL);
	}
	for (pL = doc->selEndL; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (LinkSEL(pL))
			DeselectNode(pL);
	}
}


/* --------------------------------------------------------------- LimitSelRange -- */
/* Bound selection-range endpoint LINKs to actual range of selected objects. First,
deselect any objects outside the range established by the current endpoints, then 
move the endpoint LINKs inward as far as possible. */

void LimitSelRange(Document *doc)
{
	LINK pL;
	
	BoundSelRange(doc);

	pL = doc->selStartL;
	for ( ; pL; doc->selStartL=pL=RightLINK(pL)) {
		if (doc->selStartL==doc->selEndL) return;
		if (LinkSEL(pL)) break;
	}

	pL = doc->selEndL;
	for ( ; pL && !LinkSEL(pL); pL = LeftLINK(pL)) {
		doc->selEndL = pL;
		if (doc->selStartL==doc->selEndL) return;
	}
}

/* --------------------------------------------------------------- GetOptSelEnds -- */
/* Move the endpoints of the selection range in as much as possible and return
the new endpoints. Cf. OptimizeSelection: this doesn't consider subobjects so
it's faster, probably by a lot. */

void GetOptSelEnds(Document *doc, LINK *startL, LINK *endL)
{
	for (*startL = doc->selStartL; *startL!=doc->selEndL; *startL = RightLINK(*startL))
		if (LinkSEL(*startL)) break;

	for (*endL = doc->selEndL; *endL!=doc->selStartL; *endL = LeftLINK(*endL))
		if (LinkSEL(*endL)) break;
	*endL = RightLINK(*endL);
}


/* -------------------------------------------------------------- CountSelection -- */
/* Scan the selection range and count the total number of objects and no. of objects
with selection flags set. */

void CountSelection(Document *doc, INT16 *nInRange, INT16 *nSelFlag)
{
	LINK	pL;
	register INT16 nIn, nSel;
	
	nIn = nSel = 0;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
	{
		if (pL==doc->tailL) {
			MayErrMsg("CountSelection: found tail before selEndL=%ld. selStartL=%ld",
				(long)doc->selEndL, (long)doc->selStartL);
		}
		nIn++;
		if (LinkSEL(pL)) nSel++;
	}

	*nInRange = nIn;
	*nSelFlag = nSel;
}


/* ----------------------------------------------------------------- ChordSetSel -- */
/* If any note in the chord in the given Sync and voice is selected, set all of
them to be selected and return TRUE; if none is selected, just return FALSE. If
the given Sync and voice does not contain a chord, i.e., it contains zero or one
notes, does nothing, but may return either TRUE or FALSE. */

Boolean ChordSetSel(LINK syncL, INT16 voice)
{
	LINK aNoteL;
	Boolean doSelect=FALSE, didAnything=FALSE;
	
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice && NoteSEL(aNoteL)) {
			doSelect = TRUE;
			break;
		}
			
	if (!doSelect) return FALSE;
	
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice && !NoteSEL(aNoteL)) {
			NoteSEL(aNoteL) = TRUE;
			didAnything = TRUE;
		}
			
	return didAnything;
}


/* ------------------------------------------------------------- ExtendSelChords -- */
/* Extend the selection to include all notes in every chord that has any note(s)
selected. If it does anything, return TRUE, else FALSE. */

Boolean ExtendSelChords(Document *doc)
{
	LINK pL, aNoteL;
	Boolean didAnything=FALSE;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (MainNote(aNoteL) && NoteINCHORD(aNoteL)) {
					if (ChordSetSel(pL, NoteVOICE(aNoteL))) didAnything = TRUE;
				}
				
	return didAnything;
}


/* ---------------------------------------------------------------- ChordHomoSel -- */
/* Return TRUE iff every note in the voice in <pL> (which must be a Sync) has the
specified selection status. */

Boolean ChordHomoSel(LINK pL, INT16 voice, Boolean selected)
{
	LINK 		aNoteL;
	
	for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice)
			if (NoteSEL(aNoteL)!=selected) return FALSE;
	return TRUE;
}


/* ====================================================== Continuity of Selection == */

static Boolean ContinSelChks(Document *doc)
{
	Boolean someSel=FALSE;
	LINK pL;

	/* First, simple checks: that the sel. range is nonempty and that some link is selected. */

	if (doc->selStartL==doc->selEndL) return FALSE;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL)) someSel = TRUE;
	if (!someSel) return FALSE;

	if (!SameSystem(doc->selStartL, LeftLINK(doc->selEndL))) return FALSE;
	
	return TRUE;
}

/* If this is the 1st subobject for this staff, just set staff's status to this
object's; otherwise, unselected objects don't affect the staff's status. */

static void SetStfStatus(INT16 *stStatus, INT16 staff, Boolean sel)
{
	if (stStatus[staff]<0)
		stStatus[staff] = sel;
	else
		if (sel) stStatus[staff] = 1;
}

/* Return the staffn of the staff if there is a staff with non-empty selRange
at or after staffn s. */

static INT16 NextEverSel(Document *doc, Boolean *stEverSel, INT16 s)
{
	INT16 i;
	
	for (i=s; i<=doc->nstaves; i++)
		if (stEverSel[i]) return i;
		
	return -1;
}

/* Return TRUE if staff ranges [s1,e1) and [s2,e2) overlap. */

static Boolean CSChkSelRanges(LINK s1, LINK e1, LINK s2, LINK e2)
{
	if (IsAfter(e2, s1)) return FALSE;			/* is startL1 after endL2 */
	if (IsAfter(e1, s2)) return FALSE;			/* is startL2 after endL1 */
	
	return TRUE;
}
		
static Boolean OldContinSel(Document *doc, Boolean strict)
{
	PMEVENT		p;
	register LINK pL;
	LINK			subObjL, aMeasL, aPSMeasL;
	register INT16 s;
	HEAP			*tmpHeap;
	GenSubObj	*subObj;
	Boolean		stEverSel[MAXSTAVES+1],				/* Found anything in the staff sel. yet? */
					stPrevSel[MAXSTAVES+1],				/* Was the previous thing in staff sel.? */
					stNowSel[MAXSTAVES+1];				/* Is this thing in staff sel.? */
	INT16			stStatus[MAXSTAVES+1];				/* -1=this not in stf, 1= sel., 0= unsel. */
	LINK			startL1, endL1, startL2, endL2;	/* Chking stf selRange adjacency */
	INT16			s1,s2;
	
	if (!ContinSelChks(doc)) return FALSE;

	for (s = 1; s<=doc->nstaves; s++) {
		stEverSel[s] = FALSE;
		stPrevSel[s] = FALSE;
		stNowSel[s] = FALSE;
	}
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		for (s = 1; s<=doc->nstaves; s++)
			stStatus[s] = -1;
			
		switch (ObjLType(pL)) {
			case DYNAMtype:
				if (!strict) break;
			case SYNCtype:
			case GRSYNCtype:
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case RPTENDtype:
				tmpHeap = Heap + ObjLType(pL);							/* Heap?? may not stay valid during loop */
				
				for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);

					SetStfStatus(stStatus,subObj->staffn,subObj->selected);
				}
				break;
			case BEAMSETtype:
				if (strict) stStatus[BeamSTAFF(pL)] = LinkSEL(pL);
				break;
			case GRAPHICtype:
				if (strict) stStatus[GraphicSTAFF(pL)] = LinkSEL(pL);
				break;
			case OCTAVAtype:
				if (strict) stStatus[OctavaSTAFF(pL)] = LinkSEL(pL);
				break;
			case SLURtype:
				if (strict) stStatus[SlurSTAFF(pL)] = LinkSEL(pL);
				break;
			case TUPLETtype:
				if (strict) stStatus[TupletSTAFF(pL)] = LinkSEL(pL);
				break;
			case TEMPOtype:
				if (strict) stStatus[TempoSTAFF(pL)] = LinkSEL(pL);
				break;
			case ENDINGtype:
				if (strict) stStatus[EndingSTAFF(pL)] = LinkSEL(pL);
				break;
			case SPACEtype:
				stStatus[SpaceSTAFF(pL)] = LinkSEL(pL);
				break;
			/*
			 *	Treat Measures specially. It is possible for a wipe selection, in a part
			 *	with more than one staff, to leave all Measure subobjects unselected. In
			 * this case, we still want to allow clipboard operations that require
			 *	continuity, so we block the Measure's selection status from preventing
			 *	continuity. This in turn requires the clipboard-handling routines to
			 *	special case measures to maintain consistency. Also note that other functions
			 *	that call ContinSelection may have to take this peculiarity into account.
			 *
			 *	Since transitions in selection status are what prevent continuity, what we
			 *	have to do here is not cause transitions for unselected Measure subobjects.
			 */
			case MEASUREtype:
				aMeasL = FirstSubLINK(pL);
				for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL))
					if (MeasureSEL(aMeasL)) stNowSel[MeasureSTAFF(aMeasL)] = TRUE;
				break;
			case PSMEAStype:
				aPSMeasL = FirstSubLINK(pL);
				for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL))
					if (PSMeasSEL(aPSMeasL)) stNowSel[PSMeasSTAFF(aPSMeasL)] = TRUE;
				break;
			default:																/* Object w/no subobjects */
				;
		}
		
		/* Update "now" status for staves that appeared in this object. */
		for (s = 1; s<=doc->nstaves; s++)
			if (stStatus[s]>=0) stNowSel[s] = (stStatus[s]==1);

		/*
		 *	If, in any staff, anything earlier was selected and the previous thing
		 *	was not selected and the current thing is selected, the selection is
		 *	discontinuous.
		 */
		for (s = 1; s<=doc->nstaves; s++) {
			if (stEverSel[s] && !stPrevSel[s] && stNowSel[s]) {
				return FALSE;
			}
			stPrevSel[s] = stNowSel[s];
			if (stNowSel[s]) stEverSel[s] = TRUE;
		}
	}

	/* Pairwise continuity check on staves with non-empty selRanges; checks
		if selRanges on staves selRange adjacent overlap. Optimizes to only
		get selRange on any staff once. */

	s1 = NextEverSel(doc,stEverSel,1);
	if (s1<0) return TRUE;
	s2 = NextEverSel(doc,stEverSel,s1+1);
	if (s2<0) return TRUE;

	GetStfSelRange(doc, s1, &startL1, &endL1);
	GetStfSelRange(doc, s2, &startL2, &endL2);
	
	if (!CSChkSelRanges(startL1,endL1,startL2,endL2)) return FALSE;

	for (s = s2; s<doc->nstaves; s=s2) {
		s2 = NextEverSel(doc,stEverSel,s+1);
		if (s2<0) break;

		startL1 = startL2; endL1 = endL2;
		GetStfSelRange(doc, s2, &startL2, &endL2);

		if (!CSChkSelRanges(startL1,endL1,startL2,endL2)) return FALSE;
	}

	return TRUE;
}

static Boolean NewContinSel(Document *doc)
{
	register LINK pL;
	LINK			aNoteL;
	PANOTE		aNote;
	register INT16 v;
	Boolean		voiceEverSel[MAXVOICES+1],			/* Found anything in the voice sel. yet? */
					voicePrevSel[MAXVOICES+1],			/* Was the previous thing in voice sel.? */
					voiceNowSel[MAXVOICES+1];			/* Is this thing in voice sel.? */
	
	if (!ContinSelChks(doc)) return FALSE;

	for (v = 1; v<=MAXVOICES; v++) {
		voiceEverSel[v] = FALSE;
		voicePrevSel[v] = FALSE;
	}
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->inChord) {
					if (MainNote(aNoteL)) {
						if (!ChordHomoSel(pL, aNote->voice, aNote->selected))
							return FALSE;
						
						voiceNowSel[aNote->voice] = aNote->selected;
					}
				}
				else
					voiceNowSel[aNote->voice] = aNote->selected;
			}
		}
		/*
		 *	If, in this voice, anything earlier was selected and the previous subobject
		 *	was not selected and the current subobject is selected, the selection is
		 *	discontinuous.
		 */
		for (v = 1; v<=MAXVOICES; v++) {
			if (voiceEverSel[v] && !voicePrevSel[v] && voiceNowSel[v])
				return FALSE;
			voicePrevSel[v] = voiceNowSel[v];
			if (voiceNowSel[v]) voiceEverSel[v] = TRUE;
			voiceNowSel[v] = FALSE;
		}
	}
	
	return TRUE;
}

/* ------------------------------------------------------------- ContinSelection -- */
/* Check whether the selection is nonempty, entirely within one system, and
continuous in each staff (current version) or voice (someday); if so, return TRUE. */

Boolean ContinSelection(Document *doc,
								Boolean strict)	/* TRUE=use unreasonably strict <v.996 checks */
{
#ifndef NEWCONTTEST

	return OldContinSel(doc,strict);
#endif

	return NewContinSel(doc);
}

/* ----------------------------------------------------------- OptimizeSelection -- */
/* Assuming everything selected is between doc->selStartL and selEndL, clear 
selected flags for every object with no selected subobjects, and move selStartL
and selEndL inward as far as possible. If nothing is selected, set an insertion
point: doc->selStartL = doc->selEndL = the original selStartL. Cf. GetOptSelEnds.

NB: should probably call BoundSelRange. */

void OptimizeSelection(Document *doc)
{
	PMEVENT		p;
	LINK			pL, oldStartL, subObjL;
	HEAP			*tmpHeap;
	GenSubObj	*subObj;
	
	if (doc->selStartL==doc->selEndL)							/* Return if empty sel. range. */
		return;

	/* There are two steps:
	 *	1. For every object in the selection range whose selected flag is set, if
	 *		none of its subobjects is selected, deselect the object.
	 *	2. Move selStartL and selEndL inward to (respectively) the first and last
	 *		selected objects.
	 */
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
	{
		if (LinkSEL(pL))
		{
			switch (ObjLType(pL))
			{
			case SYNCtype:
			case MEASUREtype:
			case PSMEAStype:
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case DYNAMtype:
			case RPTENDtype:
				tmpHeap = Heap + ObjLType(pL);				/* p may not stay valid during loop */
				
				for (subObjL = FirstSubObjPtr(p, pL); subObjL;
					  	subObjL = NextLink(tmpHeap, subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap, subObjL);
					if (subObj->selected) break;
				}
				if (!subObjL) LinkSEL(pL) = FALSE;			/* Gotten to end, nothing selected */
				break;
			default:
				;
			}
		}
	}

	oldStartL = doc->selStartL;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL)) break;
	doc->selStartL = pL;
	for (pL = doc->selEndL; pL!=LeftLINK(doc->selStartL); pL = LeftLINK(pL))
		if (LinkSEL(pL)) break;
	doc->selEndL = RightLINK(pL);
	if (doc->selStartL==doc->selEndL) doc->selStartL = doc->selEndL = oldStartL;
}


/* ------------------------------------------------------------- UpdateSelection -- */
/* Clear object selected flags where appropriate for their subobjects, and set
selStartL, selEndL to the narrowest range that includes all selected objects.
Does NOT assume the current values of selStartL, selEndL are meaningful.
NB: at the moment, if nothing is selected, sets selStartL = selEndL = headL;
this is almost certainly a bug!
JGG: Yes, it causes the "MEAdjustCaret: can't handle type 0" error msg. Trying to
protect against this after call to OptimizeSelection below, but this may not be
the right fix. */

void UpdateSelection(Document *doc)
{
	doc->selStartL = doc->headL;
	doc->selEndL = doc->tailL;
	OptimizeSelection(doc);
	if (doc->selStartL==doc->selEndL && HeaderTYPE(doc->selStartL))
		doc->selStartL = doc->selEndL = RightLINK(doc->selStartL);
}


/* ------------------------------------------------------------------ SetSelEnds -- */
/* Accumulate the selection range pinned at startL. */

static void SetSelEnds(LINK pL, LINK *startL, LINK *endL)
{
	if (!*startL) *startL = pL;
	*endL = pL;
}


/* -------------------------------------------------------------- GetStfSelRange -- */
/*  Return the minimum range that includes everything selected on a staff. If
nothing is selected on that staff, returns NILINKs.

Note: We could lump the last 8 cases into one by use of GetSubObjStaff(pL, 1).
	However, GetSubObjStaff is misnamed, since it gets objStaff half the time,
	and casts Graphics, Tempos, Slurs and Space objects to PEXTENDs, which
	they are not; NTypes.h had damn well better know about this if we don't
	want to risk breaking things by moving fields around in Graphics, etc.
*/

void GetStfSelRange(Document *doc, INT16 staff, LINK *startL, LINK *endL)
{
	PMEVENT	p;
	LINK 		pL, subObjL;
	HEAP		*tmpHeap;
	GenSubObj *subObj;
	
	*startL = *endL = NILINK;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (!LinkSEL(pL)) continue;
		
		switch (ObjLType(pL)) {
			case SYNCtype:
			case GRSYNCtype:
			case MEASUREtype:
			case PSMEAStype:
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case DYNAMtype:
			case RPTENDtype:
				tmpHeap = Heap + ObjLType(pL);		/* p may not stay valid during loop */
				
				for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
					if (subObj->staffn==staff && subObj->selected) {
						SetSelEnds(pL,startL,endL); break;
					}
				}
				break;
			case BEAMSETtype:
			case TUPLETtype:
			case SLURtype:
			case OCTAVAtype:
				p = GetPMEVENT(pL);
				if (((PEXTEND)p)->staffn==staff)
					SetSelEnds(pL,startL,endL);
				break;
			case GRAPHICtype:
				if (GraphicSTAFF(pL)==staff)
					SetSelEnds(pL,startL,endL);
				break;
			case TEMPOtype:
				if (TempoSTAFF(pL)==staff)
					SetSelEnds(pL,startL,endL);
				break;
			case SPACEtype:
				if (SpaceSTAFF(pL)==staff)
					SetSelEnds(pL,startL,endL);
				break;
			case ENDINGtype:
				if (EndingSTAFF(pL)==staff)
					SetSelEnds(pL,startL,endL);
				break;
			default:
				;
		}
	}
	
	if (*endL) *endL = RightLINK(*endL);
}


/* ---------------------------------------------------------------- GetVSelRange -- */
/*  Return the minimum range that includes everything selected in a voice. If
nothing is selected in that voice, returns NILINKs. */

void GetVSelRange(Document *doc, INT16 v, LINK *startL, LINK *endL)
{
	LINK 		pL;
	
	*startL = *endL = NILINK;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (ObjSelInVoice(pL, v)) {
			if (!*startL) *startL = pL;
			*endL = pL;
		}
		
	if (*endL) *endL = RightLINK(*endL);
}


/* ------------------------------------------------------------- GetNoteSelRange -- */
/* Return the minimum range that includes all selected notes/rests/grace notes in
a voice. If nothing is selected in that voice, returns NILINKs. */

void GetNoteSelRange(
				Document *doc,
				INT16 voice,
				LINK *startL, LINK *endL,
				Boolean notesGraceNotes)	/* Notes/rests only, grace notes only, or both */
{
	LINK 		pL, aNoteL, aGRNoteL;
	
	*startL = *endL = NILINK;

	if (VOICE_MAYBE_USED(doc, voice)) {
		for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
			if (LinkSEL(pL)) {
				if (notesGraceNotes!=GRNOTES_ONLY && SyncTYPE(pL)) {
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteVOICE(aNoteL)==voice && NoteSEL(aNoteL)) {
							if (!*startL) *startL = pL;
							*endL = pL;
						}
				}
				
				if (notesGraceNotes!=NOTES_ONLY && GRSyncTYPE(pL)) {
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
						if (GRNoteVOICE(aGRNoteL)==voice && GRNoteSEL(aGRNoteL)) {
							if (!*startL) *startL = pL;
							*endL = pL;
						}
				}
			}
	
		if (*endL) *endL = RightLINK(*endL);
	}
}


/* -------------------------------------------------------------- BFSelClearable -- */
/* Considering whether the selection is before the first Measure of its System,
which System that is, and what object types are in the selection, is it clearable?
Does not consider whether the selection is continuous, whether Master Page is in
effect, etc. */

Boolean BFSelClearable(
				Document *doc,
				Boolean beforeFirstMeas 	/* TRUE=selection is before 1st Meas of its System */
				)
{
	LINK pL;
	
	/*
	 * If selection is before the first Measure of any System other than the first,
	 *	and it contains anything other than Graphics and tempo marks, it's not
	 *	clearable; otherwise it is.
	 */
	if (doc->currentSystem<=1 || !beforeFirstMeas)
			return TRUE;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL))
			if (!GraphicTYPE(pL) && !TempoTYPE(pL) && !EndingTYPE(pL))
				return FALSE;
		
	return TRUE;
}


/* Null function to allow loading or unloading Select.c's segment. */

void XLoadSelectSeg()
{
}
