/******************************************************************************************
	FILE:	DSUtils.c
	PROJ:	Nightingale
	DESC:	Utility routines for examining and manipulating the main data
			structure, the object list.

		NRGRInMeasure			IsFakeMeasure			UpdatePageNums
		UpdateSysNums			UpdateMeasNums			GetMeasNum
		IsPtInMeasure			PageRelxd				PageRelyd
		GraphicPageRelxd		LinkToPt				SysRelxd
		Sysxd					PMDist					HasValidxd
		FirstValidxd			DFirstValidxd			ObjWithValidxd
		GetSubXD				ZeroXD					RealignObj
		GetSysWidth				GetSysLeft
		StaffHeight				StaffLength				MeasWidth
		MeasOccupiedWidth		MeasJustWidth			SetMeasWidth
		SetMeasFillSystem		GetStaffGroupBounds
		IsAfter					IsAfterIncl				BetweenIncl
		WithinRange				SamePage				SameSystem
		SameMeasure
		SyncsAreConsec			BeforeFirstMeas			FirstMeasInSys	
		LastMeasInSys			IsLastUsedMeasInSys		LastOnPrevSys
		IsLastInSystem			LastObjInSys
		IsLastSysInPage			GetLastSysInPage		FirstSysInPage
		NSysOnPage				LastObjOnPage			GetNextSystem
		RoomForSystem
		GetCurrentPage			GetMasterPage			GetSysRange
		MoveInMeasure & friends
		CanMoveRestOfSystem
		CountNotesInRange		CountGRNotesInRange		CountNotes
		VCountNotes				CountGRNotes			SVCountNotes
		SVCountGRNotes			CountNoteAttacks		CountObjects
		CountSubobjsByHeap
		HasOtherStemSide		IsNoteLeftOfStem		GetStemUpDown
		GetGRStemUpDown			GetExtremeNotes			GetExtremeGRNotes
		FindMainNote			FindGRMainNote			GetObjectLimits
		InObjectList			GetSubObjStaff			GetSubObjVoice
		ObjOnStaff				CommonStaff				ObjHasVoice
		ObjSelInVoice			StaffOnStaff			ClefOnStaff
		KeySigOnStaff			TimeSigOnStaff			MeasOnStaff
		NoteOnStaff				GRNoteOnStaff			NoteInVoice
		GRNoteInVoice			SyncInVoice				GRSyncInVoice
		SyncVoiceOnStaff		SyncInBEAMSET			SyncInOTTAVA
		PrevTiedNote			FirstTiedNote			ChordNextNR
		GetCrossStaff			SetTempFlags			SetSpareFlags
		IsSyncMultiVoice		IsNeighborhoodMultiVoice TweakSubRects
		CompareScoreFormat		DisposeMODNRs			Staff2PartL
		PartL2Partn				VHasTieAcross			HasSmthgAcross
		LineSpace2Rastral		Rastral2LineSpace		StaffRastral
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017-23 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* --------------------------------------------------------------------- NRGRInMeasure -- */
/* Is there a note, rest, or grace note in Measure <measL>? */

static Boolean NRGRInMeasure(Document *, LINK);
static Boolean NRGRInMeasure(Document *doc, LINK measL)
{
	LINK endObjL, pL;
	
	endObjL = EndMeasSearch(doc, measL);
	
	for (pL = RightLINK(measL); pL!=endObjL; pL = RightLINK(pL))
		if (SyncTYPE(pL) || GRSyncTYPE(pL)) return False;
			
	return True;
}


/* --------------------------------------------------------------------- IsFakeMeasure -- */
/* If <measL>, which must be a Measure obj, doesn't start a real measure, return True.
By "not a real measure", we mean it's at the end of a System and contains no notes,
rests, or grace notes (so it's just ending the previous measure), or it's at the
beginning of a System and the last Measure in the previous System is a real measure
(so we have a measure split across systems). Intended for use in assigning and
displaying measure numbers. */

Boolean IsFakeMeasure(Document *doc, LINK measL)
{
	LINK prevMeasL;

	/* The first Measure of a system isn't real if the last Measure of the previous
	   system is real, i.e., a measure is split across systems. */
	if (FirstMeasInSys(measL))
		if ((prevMeasL = LinkLMEAS(measL))!=NILINK) {
		
			/* Since prevMeasL is the Measure before the first Measure of a system, it
			   must be the last Measure of the previous system. If it's also the first
			   Measure of that system, this is something like an unmeasured passage,
			   where no Measure is less real than any other. */
			   
			if (FirstMeasInSys(prevMeasL)) return False;
			return !NRGRInMeasure(doc, prevMeasL);
		}

	if (!LastMeasInSys(measL)) return False;
	
	return NRGRInMeasure(doc, measL);
}

/* ----------------------------------------------------------- UpdatePage/Sys/MeasNums -- */
/* Update the sheetNum field for all pages in the score by simply re-numbering them,
starting at 0 for the first page and incrementing for all remaining pages. Also update
the header's numSheets field. */

void UpdatePageNums(Document *doc)
{
	LINK pageL; short sheetNum=0;
	
	pageL = LSSearch(doc->headL, PAGEtype, ANYONE, False, False);
	
	for ( ; pageL; pageL = LinkRPAGE(pageL))
		SheetNUM(pageL) = sheetNum++;
		
	doc->numSheets = sheetNum;
}

/* Update the systemNum field for all systems in the score by simply re-numbering them,
starting at 1 for the first system and incrementing for all remaining systems. Also
update the header's nsystems field. */

void UpdateSysNums(Document *doc, LINK headL)
{
	LINK sysL;  short sysNum=1;
	
	sysL = LSSearch(headL, SYSTEMtype, ANYONE, False, False);
	
	for ( ; sysL; sysL = LinkRSYS(sysL))
		SystemNUM(sysL) = sysNum++;
		
	doc->nsystems = sysNum-1;
}

/* Based on Measure and Sync objects, update the fakeMeas field of every Measure object
and the measureNum field of every Measure subobject in the given document from <startL>
or the preceding Measure to the end. If <startL> is NILINK, do the entire document.
Deliver True if we actually change anything.

NB: Updating the fakeMeas flags is very slow. For efficiency, if <startL> is non-NILINK,
we actually update them only until the second Measure of the System after <startL>. */

Boolean UpdateMeasNums(Document *doc, LINK startL)
{
	LINK pL;
	LINK stopFakeMeasL, sysL, stopFakeSysL, aMeasureL, aNoteL;
	Boolean fakeMeas, didAnything=False, updateFakeMeas;
	PAMEASURE aMeasure;
	short measNum,									/* Zero-indexed number of the next measure */
		nMeasRest;

	stopFakeMeasL = NILINK;
	
	if (startL==NILINK) {
		pL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);
		measNum = 0;
	} 
	else {
		pL = EitherSearch(startL, MEASUREtype, ANYONE, GO_LEFT, False);
		
		/* We might be able to use GetMeasNum, but we need internal measure nos. here;
		   also, there might be a performance issue. */
		   
		aMeasureL = FirstSubLINK(pL);
		aMeasure = GetPAMEASURE(aMeasureL);
		measNum = aMeasure->measureNum;
		sysL = LinkRSYS(MeasSYSL(pL));
		if (sysL!=NILINK) {
			stopFakeSysL = LinkRSYS(sysL);
			if (stopFakeSysL!=NILINK) stopFakeMeasL = SSearch(stopFakeSysL, MEASUREtype, GO_RIGHT);
		}
	}

	updateFakeMeas = True;
	for ( ; pL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:
				if (pL==stopFakeMeasL) updateFakeMeas = False;
				if (updateFakeMeas) {
					fakeMeas = IsFakeMeasure(doc, pL);
					if (MeasISFAKE(pL)!=fakeMeas) didAnything = True;
					MeasISFAKE(pL) = fakeMeas;
				}
				else
					fakeMeas = MeasISFAKE(pL);
				
				/* If it's a non-fake Measure, add one to next measure number. */
				
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
					aMeasure = GetPAMEASURE(aMeasureL);
					if (aMeasure->measureNum!=measNum) didAnything = True;
					aMeasure->measureNum = measNum;
				}
				if (!fakeMeas) measNum++;
				break;
			case SYNCtype:
				/* If a multibar rest, add one less than no. of bars to next measure
				   number. Note that all subobjs of the Sync should be identical
				   multibar rests! */
				   
				aNoteL = FirstSubLINK(pL);
				if (NoteREST(aNoteL) && NoteType(aNoteL)<-1) {
					nMeasRest = ABS(NoteType(aNoteL));
					measNum += nMeasRest-1;
				} 
				break;
			default:
				;
		}
	}
	
	return didAnything;
}


/* ------------------------------------------------------------------------ GetMeasNum -- */
/* Get the (user) number of the Measure the given object is in, or of the object itself
if it's a Measure. Does not assume cross-links are valid. */

short GetMeasNum(Document *doc, LINK pL)
{
	LINK		measL, aMeasureL;
	PAMEASURE	aMeasure;
	short		measNum;
	
	measL = SSearch(pL, MEASUREtype, GO_LEFT);
	if (!measL) return doc->firstMNNumber;
	
	aMeasureL = FirstSubLINK(measL);
	aMeasure = GetPAMEASURE(aMeasureL);
	measNum = aMeasure->measureNum+doc->firstMNNumber;
	return measNum;
}


/* --------------------------------------------------------------------- IsPtInMeasure -- */
/* Is the given point within the given Measure? */

Boolean IsPtInMeasure(Document */*doc*/, Point pt, LINK sL)
{
	Rect mRect;

	mRect = MeasureBBOX(sL);
	return (PtInRect(pt, &mRect));
}


/* --------------------------------------------------------------- PageRelxd,PageRelyd -- */
/* Return object's xd and yd relative to the Page. These are not necessarily exact,
and no single value could possibly be correct for all subobjs of Syncs, Slurs, etc.:
they are intended for purposes like setting scaleCenter for enlarging, and the target
position for Go to Selection. */

DDIST PageRelxd(LINK pL, PCONTEXT pContext)
{
	DDIST objXD, xd;  POBJHDR p;
	LINK firstL, lastL;
	PGRAPHIC pGraphic;  PTEMPO pTempo;
	
	objXD = LinkXD(pL);
	
	switch (ObjLType(pL)) {
		case PAGEtype:
			return 0;
		case SYSTEMtype:
		case CONNECTtype:
			return pContext->systemLeft;
		case STAFFtype:
			return pContext->staffLeft;
		case MEASUREtype:
			return pContext->measureLeft;
		case PSMEAStype:
		case SYNCtype:
		case GRSYNCtype:
		case RPTENDtype:
		case SPACERtype:
		case ENDINGtype:
			return pContext->measureLeft + objXD;
		case CLEFtype:
			p = GetPOBJHDR(pL);
			if (((PCLEF)p)->inMeasure) return pContext->measureLeft + objXD;
			
			return pContext->staffLeft + objXD;
		case KEYSIGtype:
			p = GetPOBJHDR(pL);
			if (((PKEYSIG)p)->inMeasure) return pContext->measureLeft + objXD;
			
			return pContext->staffLeft + objXD;
		case TIMESIGtype:
			p = GetPOBJHDR(pL);
			if (((PTIMESIG)p)->inMeasure) return pContext->measureLeft + objXD;
			
			return pContext->staffLeft + objXD;
		case BEAMSETtype:
			firstL = FirstInBeam(pL);
			xd = PageRelxd(firstL, pContext);
			return xd;
		case DYNAMtype:
			xd = PageRelxd(DynamFIRSTSYNC(pL), pContext) + objXD;
			return xd;
		case GRAPHICtype:
			pGraphic = GetPGRAPHIC(pL);
			if (pGraphic->firstObj) {
				if (PageTYPE(pGraphic->firstObj)) return objXD;

				xd = PageRelxd(pGraphic->firstObj, pContext);
				return xd+objXD;
			}
			return objXD;
		case OTTAVAtype:
			firstL = FirstInOttava(pL);
			xd = PageRelxd(firstL, pContext);
			return xd+objXD;
		case SLURtype:
			firstL = SlurFIRSTSYNC(pL);
			lastL = SlurLASTSYNC(pL);
			xd = SyncTYPE(firstL) ? PageRelxd(firstL, pContext) :
										PageRelxd(lastL, pContext);
			return xd+objXD;
		case TUPLETtype:
			firstL = FirstInTuplet(pL);
			lastL = LastInTuplet(pL);
			return PageRelxd(firstL, pContext)/2+PageRelxd(lastL, pContext)/2;
		case TEMPOtype:
			pTempo = GetPTEMPO(pL);
			if (pTempo->firstObjL) {
				if (PageTYPE(pTempo->firstObjL))
					return objXD;

				xd = PageRelxd(pTempo->firstObjL, pContext);
				return xd+objXD;
			}
			return objXD;
		case MODNRtype:
		default:
			return -9999;
	}
}

DDIST PageRelyd(LINK pL, PCONTEXT pContext)
{
	DDIST objYD, yd;  POBJHDR p;
	LINK firstL, lastL;
	PGRAPHIC pGraphic;  PTEMPO pTempo;
	
	objYD = LinkYD(pL);
	
	switch (ObjLType(pL)) {
		case PAGEtype:
			return 0;
		case SYSTEMtype:
		case CONNECTtype:
			return pContext->systemTop;
		case STAFFtype:
			return pContext->staffTop;
		case MEASUREtype:
		case PSMEAStype:
		case RPTENDtype:
		case SPACERtype:
			return pContext->measureTop;
		case ENDINGtype:
		case SYNCtype:
		case GRSYNCtype:
			p = GetPOBJHDR(pL);
			return pContext->measureTop + objYD;
		case CLEFtype:
			p = GetPOBJHDR(pL);
			if (((PCLEF)p)->inMeasure) return pContext->measureTop + objYD;
			
			return pContext->staffTop + objYD;
		case KEYSIGtype:
			p = GetPOBJHDR(pL);
			if (((PKEYSIG)p)->inMeasure) return pContext->measureTop + objYD;
			
			return pContext->staffTop + objYD;
		case TIMESIGtype:
			p = GetPOBJHDR(pL);
			if (((PTIMESIG)p)->inMeasure) return pContext->measureTop + objYD;
			
			return pContext->staffTop + objYD;
		case BEAMSETtype:
			firstL = FirstInBeam(pL);
			yd = PageRelyd(firstL, pContext);
			return yd;
		case DYNAMtype:
			return pContext->measureTop;			
		case GRAPHICtype:
			pGraphic = GetPGRAPHIC(pL);
			if (pGraphic->firstObj) {
				if (PageTYPE(pGraphic->firstObj)) return objYD;

				yd = PageRelyd(pGraphic->firstObj, pContext);
				return yd+objYD;
			}
			return objYD;
		case OTTAVAtype:
			firstL = FirstInOttava(pL);
			yd = PageRelyd(firstL, pContext);
			return yd+objYD;
		case SLURtype:
			firstL = SlurFIRSTSYNC(pL);
			lastL = SlurLASTSYNC(pL);
			yd = SyncTYPE(firstL) ? PageRelyd(firstL, pContext) :
										PageRelyd(lastL, pContext);
			return yd+objYD;
		case TUPLETtype:
			firstL = FirstInTuplet(pL);
			lastL = LastInTuplet(pL);
			return PageRelyd(firstL, pContext)/2+PageRelyd(lastL, pContext)/2;
		case TEMPOtype:
			pTempo = GetPTEMPO(pL);
			if (pTempo->firstObjL) {
				if (PageTYPE(pTempo->firstObjL))
					return objYD;

				yd = PageRelyd(pTempo->firstObjL, pContext);
				return yd+objYD;
			}
			return objYD;
		case MODNRtype:
		default:
			return -9999;
	}
}


/* ------------------------------------------------------------------ GraphicPageRelxd -- */
/* Return xd relative to the Page for a Graphic, whose position is relative to a
subobject in its voice or on its staff. */

DDIST GraphicPageRelxd(Document */*doc*/,					/* unused */
						LINK pL, LINK relObjL,
						PCONTEXT pContext)
{
	short staffn, voice;
	DDIST xd;
	LINK aNoteL, aGRNoteL, aClefL, aKeySigL, aTimeSigL;
	
	xd = PageRelxd(relObjL, pContext);
	if (!GraphicTYPE(pL)) return xd;
	
	staffn = GraphicSTAFF(pL);
	voice = GraphicVOICE(pL);
	
	switch (ObjLType(relObjL)) {
		case PAGEtype:
			return 0;
		case SYSTEMtype:
		case CONNECTtype:
			return xd;
		case MEASUREtype:
		case PSMEAStype:
			return xd;
		case SYNCtype:
			aNoteL = FindMainNote(relObjL, voice);
			return xd+NoteXD(aNoteL);
		case GRSYNCtype:
			aGRNoteL = FindGRMainNote(relObjL, voice);
			return xd+GRNoteXD(aGRNoteL);
		case RPTENDtype:
		case SPACERtype:
		case ENDINGtype:
			return xd;
		case CLEFtype:
			aClefL = ClefOnStaff(relObjL, staffn);
			return xd+ClefXD(aClefL);
		case KEYSIGtype:
			aKeySigL = KeySigOnStaff(relObjL, staffn);
			return xd+KeySigXD(aKeySigL);
		case TIMESIGtype:
			aTimeSigL = TimeSigOnStaff(relObjL, staffn);
			return xd+TimeSigXD(aTimeSigL);
		case BEAMSETtype:
		case DYNAMtype:
		case GRAPHICtype:
		case OTTAVAtype:
		case SLURtype:
		case TUPLETtype:
		case TEMPOtype:
		default:
			return xd;
	}
}


/* -------------------------------------------------------------------------- LinkToPt -- */
/* Return pL's location in paper or window-relative coords, depending on toWindow. */

Point LinkToPt(Document *doc, LINK pL, Boolean toWindow)
{
	CONTEXT context;  Point pt;
	
	GetContext(doc, pL, 1, &context);
	
	pt.v = d2p(PageRelyd(pL, &context));
	pt.h = d2p(PageRelxd(pL, &context));
	
	if (toWindow) {
		pt.v += context.paper.top;
		pt.h += context.paper.left;
	}
	
	return pt;
}


/* ---------------------------------------------------------------------- ObjSpaceUsed -- */
/* Return the amount of horizontal space pL occupies. */

DDIST ObjSpaceUsed(Document *doc, LINK pL)
{
	LINK	aNoteL;
	long	maxLen, tempLen;
	short	noteStaff=1;
	CONTEXT	context;
	STDIST	symWidth, space;
	
	switch(ObjLType(pL)) {
		case SYNCtype:
				maxLen = tempLen = space = 0L;
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					tempLen = CalcNoteLDur(doc, aNoteL, pL);
					if (tempLen > maxLen) {
						maxLen = tempLen;
						noteStaff = NoteSTAFF(aNoteL);
					}
				}
				maxLen = n_max(tempLen, maxLen);
				space = IdealSpace(doc, maxLen, RESFACTOR*doc->spacePercent);
				symWidth = SymWidthRight(doc, pL, noteStaff, False);
				GetContext(doc, pL, noteStaff, &context);
				return ((space >= symWidth) ? 
							std2d(space,context.staffHeight, 5) :
							std2d(symWidth,context.staffHeight, 5));
		case TIMESIGtype:
		case KEYSIGtype:
		case DYNAMtype:
		case RPTENDtype:
		case CLEFtype:
		case MEASUREtype:
		case PSMEAStype:
			return (SymWidthRight(doc, pL, 1, False));
		default:
			if (TYPE_BAD(pL))
				MayErrMsg("ObjSpaceNeeded: object L%ld has illegal type %ld",
							(long)pL, (long)ObjLType(pL));
			return 0;
	}
}


/* -------------------------------------------------------------------- SysRelxd, etc. -- */
/* Return object's xd relative to the System, regardless of the object's type. Assumes
pL is in the main object list. */

DDIST SysRelxd(LINK pL)
{
	LINK measL;
	
	if (MeasureTYPE(pL)) return (LinkXD(pL));

	measL = LSISearch(LeftLINK(pL), MEASUREtype, ANYONE, GO_LEFT, False);
	if (!measL) return (LinkXD(pL));					/* before 1st measure of its system */

	return (LinkXD(measL)+LinkXD(pL));
}


/* Return point's xd relative to the System. */

DDIST Sysxd(Point pt, DDIST sysLeft)
{
	return (p2d(pt.h)-sysLeft);
}


DDIST PMDist(LINK pL, LINK qL)
{
	return (SysRelxd(qL)-SysRelxd(pL));
}


/* --------------------------------------------------------------- "Validxd" utilities -- */

Boolean HasValidxd(LINK pL)
{
	return (JustTYPE(pL)!=J_D);
}


/* Return the first object with valid xd, according to HasValidxd, in the direction
goLeft.

If FirstValidxd returns NULL (e.g. when for loop terminates because of !qL), this can
cause an error in the calling routine, for example, PMDist. However, since tailL has a
valid xd, FirstValidxd should return its address before it gets to, e.g.,
RightLINK(tailL). Also, when FirstValidxd is called for RightLINK(pL), since it should
never be possible to select tailL directly, it should never be possible to call
FirstValidxd for a NULL node (RightLINK(tailL)). Given these assurances, it is still
not easy to see how to do error checking in this routine.
 */

LINK FirstValidxd(LINK pL, Boolean goLeft)
{
	LINK qL;
	
	if (!pL) {
		MayErrMsg("FirstValidxd: called with NILINK argument.");
		return NILINK;
	}

	if (goLeft) {
		for (qL = pL; qL; qL = LeftLINK(qL))
			if (HasValidxd(qL)) break;
	}
	else 
		for (qL = pL; qL; qL = RightLINK(qL))
			if (HasValidxd(qL)) break;

	return qL;
}

/* Version of the above where we pass the document in which to compute and the old
document as context to restore. */

LINK DFirstValidxd(Document *oldDoc, Document *fixDoc, LINK pL, Boolean goLeft)
{
	LINK qL;
	
	InstallDoc(fixDoc);
	if (!pL) {
		MayErrMsg("DFirstValidxd: called with NILINK argument.");
		InstallDoc(oldDoc);
		return NILINK;
	}

	if (goLeft) {
		for (qL = pL; qL; qL = LeftLINK(qL))
			if (HasValidxd(qL)) break;
	}
	else 
		for (qL = pL; qL; qL = RightLINK(qL))
			if (HasValidxd(qL)) break;

	InstallDoc(oldDoc);
	return qL;
}


LINK ObjWithValidxd(LINK pL, Boolean measureOK)
{
	LINK vpL = pL;
	
	for ( ; vpL; vpL = RightLINK(vpL))
		switch (ObjLType(vpL)) {
			case BEAMSETtype:
				break;
			case MEASUREtype:
				if (!measureOK) {
					MayErrMsg("ObjWithValidxd: found MEASURE with valid xd before object L%ld",
								(long)pL);
					return NILINK;
				}
				else return vpL;
			default:
				return vpL;
		}
	
	return NILINK;
}


/* ------------------------------------------------------------------ GetSubXD, ZeroXD -- */

/* Get the subobject xd of subobject subObjL of object pL. */

DDIST GetSubXD(LINK pL, LINK subObjL)
{
	switch (ObjLType(pL)) {
		case CLEFtype:
			return ClefXD(subObjL);
		case KEYSIGtype:
			return KeySigXD(subObjL);
		case TIMESIGtype:
			return TimeSigXD(subObjL);
		case SYNCtype:
			return NoteXD(subObjL);
		case GRSYNCtype:
			return GRNoteXD(subObjL);
		case DYNAMtype:
			return DynamicXD(subObjL);
	}

	return 0;
}
	
/* Set the xd of subobject subObjL of object pL to zero. */

void ZeroXD(LINK pL, LINK subObjL)
{
	switch (ObjLType(pL)) {
		case CLEFtype:
			ClefXD(subObjL) = 0;  return;
		case KEYSIGtype:
			KeySigXD(subObjL) = 0;  return;
		case TIMESIGtype:
			TimeSigXD(subObjL) = 0;  return;
		case SYNCtype:
			NoteXD(subObjL) = 0;  return;
		case GRSYNCtype:
			GRNoteXD(subObjL) = 0;  return;
		case DYNAMtype:
			DynamicXD(subObjL) = 0;  return;
	}
}
	
/* ------------------------------------------------------------------------ RealignObj -- */
/* If pL has subobjects, relocate all of them either to the position of the first
selected one, or to the object's position. Specifically, set all subobj xds to
zero. Then, if <useSel>, set the xd of pL to its xd plus the xd of its first
selected subobj. Return True. If pL has no subobjs, do nothing and return False. */

Boolean RealignObj(LINK pL,
					Boolean useSel		/* True=move subobjs to position of 1st one that's selected */
					)
{
	GenSubObj *subObj;  LINK subObjL;  DDIST subXD=0;
	short i;  Boolean haveXD;
	POBJHDR p;  HEAP *tmpHeap;

	if (useSel)
		haveXD = False;
	else {
		haveXD = True;
		subXD = 0;
	}

	p = GetPOBJHDR(pL);

	switch (ObjLType(pL)) {
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case SYNCtype:
		case GRSYNCtype:
		case DYNAMtype:
			tmpHeap = Heap + p->type;		/* Caveat: p may not stay valid during loop */
			
			for (i=0, subObjL=FirstSubObjPtr(p,pL); subObjL; i++, subObjL = NextLink(tmpHeap,subObjL)) {
				subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
				if (subObj->selected && !haveXD) {
					haveXD = True;
					subXD = GetSubXD(pL,subObjL);
				}
				ZeroXD(pL,subObjL);
			}
			break;
		default:
			return False;		
	}
	LinkXD(pL) += subXD;

	return True;
}
	

/* ----------------------------------------------------------- GetSysWidth, GetSysLeft -- */
/* Get the "standard" width of a system, actually the width of the document's last
system. As of v. 5.8b7, used only in Clipboard.c. */

DDIST GetSysWidth(Document *doc)
{
	LINK lastMeasL, lastMeasSubL;
	PAMEASURE lastMeasSub;
	DDIST systemWidth;

	lastMeasL = LSSearch(doc->tailL, MEASUREtype, 1, True, False);
	lastMeasSubL = FirstSubLINK(lastMeasL);
	lastMeasSub = GetPAMEASURE(lastMeasSubL);
	systemWidth = LinkXD(lastMeasL)+lastMeasSub->measSizeRect.right;
	return systemWidth;
}

/* Get the left coordinate of the current system. */

DDIST GetSysLeft(Document *doc)
{
	LINK systemL;
	PSYSTEM pSystem;

	systemL = LSSearch(doc->headL, SYSTEMtype, doc->currentSystem, False, False);
	pSystem = GetPSYSTEM(systemL);
	return pSystem->systemRect.left;
}


/* ---------------------------------------------------------- StaffHeight, StaffLength -- */
/* Get the staffHeight at the specified object on the specified staff. This is done
by searching backwards for a Staff object and getting the information from there. */

DDIST StaffHeight(Document *doc, LINK pL, short theStaff)
{
	PASTAFF	aStaff;
	LINK	staffL, aStaffL;

	if (theStaff<1 || theStaff>doc->nstaves) {
		MayErrMsg("StaffHeight: bad value %ld for theStaff", (long)theStaff);
		return -1L;
	}
	staffL = LSSearch(pL, STAFFtype, theStaff, True, False);
	if (!staffL) {
		MayErrMsg("StaffHeight: Search couldn't find staff %ld", (long)theStaff);
		return -1L;
	}

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		if (aStaff->staffn == theStaff)
			return (aStaff->staffHeight);
	}
	MayErrMsg("StaffHeight: couldn't find an entry for staff %ld", (long)theStaff);
	return -1L;
}

/* Return the length of the staff <pL> is in. */

DDIST StaffLength(LINK pL)
{
	LINK 	staffL, aStaffL;
	PASTAFF	aStaff;
	
	staffL = LSSearch(pL, STAFFtype, ANYONE, GO_LEFT, False);
	if (!staffL) MayErrMsg("SystemLength: can't find Staff for object L%ld", (long)pL);
	aStaffL = FirstSubLINK(staffL);
	aStaff = GetPASTAFF(aStaffL);
	return (aStaff->staffRight-aStaff->staffLeft);
}


/* ------------------------------------------------------------------ GetLastMeasInSys -- */

LINK GetLastMeasInSys(LINK sysL)
{
	LINK measL, rMeas;

	measL = SSearch(sysL,MEASUREtype,GO_RIGHT);
	for ( ; measL && SameSystem(measL,sysL); measL = LinkRMEAS(measL)) {
		rMeas = LinkRMEAS(measL);
		if (!rMeas) return measL;
		if (!SameSystem(rMeas,measL)) return measL;
	}
	return NILINK;
}


/* ---------------------------------------------------------------------- GetMeasRange -- */
/* Return in parameters the range of measures graphically spanned by <pL>, which must
be either a Graphic or a Tempo. */

void GetMeasRange(Document *doc, LINK pL, LINK *startMeas, LINK *endMeas)
{
	LINK measL, sysL, lastMeas, objMeasL, firstL=NILINK;
	Rect r, objR;

	if (!GraphicTYPE(pL) && !TempoTYPE(pL)) {
		LogPrintf(LOG_WARNING,
			"Object L%u is of type %d, but this function can't handle objects of that type.  (GetMeasRange)\n",
				pL, ObjLType(pL));
		return;
	}
	
	*startMeas = *endMeas = NILINK;

	/* Get objRect of pL. */

	switch(ObjLType(pL)) {
		case GRAPHICtype:
			DrawGRAPHIC(doc, pL, contextA, False);
			break;
		case TEMPOtype:
			DrawTEMPO(doc, pL, contextA, False);
			break;
	}
	
	/* Traverse measures on pL's system, and compare objRect of pL to measureBBox of
	   each measure. */
 
	sysL = EitherSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, False);
	measL = SSearch(sysL, MEASUREtype, GO_RIGHT);
	objR = LinkOBJRECT(pL);
	
	for ( ; measL && SameSystem(measL, sysL); measL = LinkRMEAS(measL)) {
		r = MeasureBBOX(measL);
		if (r.right > objR.right)
			{ *endMeas = measL; break; } 
	}

	lastMeas = GetLastMeasInSys(sysL);
	measL = lastMeas;
	
	for ( ; measL && SameSystem(measL, sysL); measL = LinkLMEAS(measL)) {
		r = MeasureBBOX(measL);
		if (r.left < objR.left)
			{ *startMeas = measL; break; } 
	}
	
	/* Include measure containing object's relative object; otherwise coordinate systems
	   get screwed up, since drawing into offscreen bitmap is relative to this measure. */

	switch(ObjLType(pL)) {
		case GRAPHICtype:
			firstL = GraphicFIRSTOBJ(pL);
			break;
		case TEMPOtype:
			firstL = TempoFIRSTOBJ(pL);
			break;
	}
	objMeasL = EitherSearch(firstL, MEASUREtype, ANYONE, GO_LEFT, False);
	*startMeas = objMeasL;
}


/* ----------------------------------------------------------------- Get measure width -- */
/* If <pL> is a Measure, return its width, otherwise return the width of the Measure
<pL> is in. */

DDIST MeasWidth(LINK pL)
{
	LINK 		measL, aMeasL;
	PAMEASURE	aMeas;
	
	measL = LSSearch(pL, MEASUREtype, ANYONE, GO_LEFT, False);
	if (!measL) MayErrMsg("MeasWidth: can't find Measure for L%ld", (long)pL);

	aMeasL = FirstSubLINK(measL);
	aMeas = GetPAMEASURE(aMeasL);
	return (aMeas->measSizeRect.right-aMeas->measSizeRect.left);
}

/* If <pL> is a Measure, return width of its occupied part, otherwise return the width of
the occupied part of the Measure <pL> is in. The "occupied part" extends to the point
where its ending barline normally "would go", i.e., to the right of the last symbol in the
Measure by the space that symbol normally would need. Intended for use on Measures that
are the last in their Systems, so that their widths extend to the ends of their Systems
simply to fill them out and say nothing about the amount of space the Measure really
needs. */

DDIST MeasOccupiedWidth(Document *doc, LINK pL, long spaceProp)
{
	LINK measL, measTermL, lastXDL;
	CONTEXT context;
	DDIST symWidth, mWidth;
	
	measL = LSSearch(pL, MEASUREtype, ANYONE, GO_LEFT, False);
	measTermL = EndMeasSearch(doc, measL);
	lastXDL = FirstValidxd(LeftLINK(measTermL), GO_LEFT);

	GetContext(doc, measL, 1, &context);
	symWidth = std2d(SymLikelyWidthRight(doc, lastXDL, spaceProp),
			context.staffHeight, context.staffLines);
	mWidth = SysRelxd(lastXDL)+symWidth-SysRelxd(measL);
	return mWidth;
}

/* If <pL> is a Measure, return its "justification width", otherwise return the
justification width of the Measure <pL> is in. Justification width is the space
that should be allocated to the Measure if it's at the end of a right-justified
System. If the Measure is empty, the justification width is simply the width of
its barline; otherwise the justification width is its MeasOccupiedWidth. */

DDIST MeasJustWidth(Document *doc, LINK pL, CONTEXT context)
{
	LINK measL, measTermL;
	DDIST mWidth;
	long spaceProp;
	
	measL = LSSearch(pL, MEASUREtype, ANYONE, True, False);
	measTermL = EndMeasSearch(doc, measL);

	if (measTermL==RightLINK(measL)) {
		mWidth = SymDWidthRight(doc, measL, 1, False, context);
		if (mWidth <= 0) mWidth = 1;
	}
	else {
		spaceProp = MeasSpaceProp(pL);
		mWidth = MeasOccupiedWidth(doc, pL, spaceProp);
	}
		
	return mWidth;
}


/* ----------------------------------------------------------------- Set measure width -- */

/* Set the width of every subobject of the given Measure to the given width. */

Boolean SetMeasWidth(LINK measL, DDIST width)
{
	LINK		aMeasL;
	PAMEASURE	aMeas;
	short		j;

	if (width<=0) return False;
	
	aMeasL = FirstSubLINK(measL);
	for (j=0; j<LinkNENTRIES(measL); j++, aMeasL=NextMEASUREL(aMeasL)) {
		aMeas = GetPAMEASURE(aMeasL);
		aMeas->measSizeRect.left = 0;	
		aMeas->measSizeRect.right =	width;
	}
	return True;
}

/* Set the measureRects of all subobjects of <measL>, which must be a Measure, to
extend to the end of their System (which ends at the same point as the Staff). If
any of the subobjects starts at or past the end of their Staff, return False, else
True. */

Boolean SetMeasFillSystem(LINK measL)
{
	DDIST	staffLen;

	if (!MeasureTYPE(measL)) {
		MayErrMsg("SetMeasFillSystem: L%ld isn't a Measure.", (long)measL);
		return False;
	}
	staffLen = StaffLength(measL);
	if (staffLen-LinkXD(measL)<=0) return False;
	
	SetMeasWidth(measL, staffLen-LinkXD(measL));
	return True;
}


/* --------------------------------------------------------------- GetStaffGroupBounds -- */
/* Get the top and bottom staff numbers of the group containing <staffn>. */

void GetStaffGroupBounds(Document *doc, short staffn, short *pTopStf, short *pBottomStf)
{
	LINK	pL, aMeasL=NILINK, bMeasL;
	short	groupTopStf=0, groupBottomStf=0;	/* Top/bottom staff nos. for staffn's group */
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		if (MeasureTYPE(pL)) {				
			aMeasL = MeasOnStaff(pL, staffn);
			break;
		}
	if (aMeasL==NILINK)	{					/* Should never happen with a valid score */
		AlwaysErrMsg("GetStaffGroupBounds: no Measure found in score. Something is seriously wrong!");
		goto Done;
	}
	
	/* The highest-numbered staff with !<connAbove> whose number is below <staffn> is
	   the top staff of our group; its <connStaff> is the bottom staff. */
		
	groupTopStf = -SHRT_MIN;
	for (short stf = staffn; stf>0; stf--) {
		bMeasL = MeasOnStaff(pL, stf);
		if (!MeasCONNABOVE(bMeasL)) {
			groupTopStf = stf;
			groupBottomStf = MeasCONNSTAFF(bMeasL);
			if (groupBottomStf==0) groupBottomStf = stf;
			break;
		}
	}

Done:
	*pTopStf = groupTopStf;
	*pBottomStf = groupBottomStf;
}


/* ---------------------------------------------------------- Compare order of objects -- */

/* Returns True if obj1 is followed by obj2 in some object list. */

Boolean IsAfter(LINK obj1, LINK obj2)
{
	LINK pL;
	
	for (pL = RightLINK(obj1); pL!=NILINK; pL = RightLINK(pL))
		if (pL==obj2) return True;
	
	return False;
}

/* Returns True if obj1 is the same as obj2 or is followed by it in some object list. */

Boolean IsAfterIncl(LINK obj1, LINK obj2)
{
	LINK pL;
	
	for (pL = obj1; pL!=NILINK; pL = RightLINK(pL))
		if (pL==obj2) return True;
	
	return False;
}

/* Is theObj outside [obj1, obj2]? Assumes all three are in the same object list. */

Boolean IsOutside(LINK theObj, LINK obj1, LINK obj2)
{	
	return (IsAfter(theObj, obj1) || IsAfter(obj2, theObj));
}

/* Is theObj between obj1 and obj2 or identical to one of them? NB: This will misbehave
if obj2 precedes obj1! */

Boolean BetweenIncl(LINK obj1, LINK theObj, LINK obj2)
{	
	if (theObj==obj1 || theObj==obj2)
		return True;
	if (IsAfter(obj1, theObj) && IsAfter(theObj, obj2))
		return True;
	
	return False;
}

/* Is theObj within the range obj1 to obj2 (including the first but not the second)? */

Boolean WithinRange(LINK obj1, LINK theObj, LINK obj2)
{
	if (obj1==obj2)												/* Empty range? */
		return False;
	if (IsAfter(obj2, obj1))									/* Backwards range? */
		return False;
	
	return BetweenIncl(obj1, theObj, LeftLINK(obj2));
}

/* ------------------------------------------------------- SamePage, -System, -Measure -- */
/* These functions do  unoptimized searches so they can be called for debugging before
the score is set up, e.g., during file-format conversion, FIXME: Names of the functions
should be changed to AreInSamePage, AreInSameSystem, and AreInSameMeasure, respectively. */

/* Return True if pL and qL are in the same page. An earlier version of this function
assumed its arguments were both Systems; this version accepts anything and should be
virtually as fast if the arguments do happen to be Systems. */

Boolean SamePage(LINK pL, LINK qL)
{
	LINK pPageL, qPageL;
	
	pPageL = LSUSearch(pL, PAGEtype, ANYONE, GO_LEFT, False);
	qPageL = LSUSearch(qL, PAGEtype, ANYONE, GO_LEFT, False);
	return (pPageL==qPageL);
}

/* Return True if pL and qL are in the same System. Caveat: If either pL or qL is a
Page and the other is in the last System of the previous Page, will return True! This
is either a bug or a pitfall. */

Boolean SameSystem(LINK pL, LINK qL)
{
	LINK pSysL, qSysL;
	
	pSysL = LSUSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, False);
	qSysL = LSUSearch(qL, SYSTEMtype, ANYONE, GO_LEFT, False);
	return (pSysL==qSysL);
}

/* Return True if pL and qL are in the same Measure. Caveat: If either pL or qL is
before the first Measure of the first System of a page and the other is in the last
Measure of the previous Page, will return True! This is either a bug or a pitfall. */

Boolean SameMeasure(LINK pL, LINK qL)
{
	LINK pMeasL, qMeasL;
	
	pMeasL = LSUSearch(pL, MEASUREtype, ANYONE, GO_LEFT, False);
	qMeasL = LSUSearch(qL, MEASUREtype, ANYONE, GO_LEFT, False);
	return (pMeasL==qMeasL);
}

/* -------------------------------------------------------------------- SyncsAreConsec -- */
/* Are syncA and syncB consecutive Syncs in the given staff/voice? Note that
SyncsAreConsec will find consecutive Syncs in different systems, since LSSearch
doesn't set inSystem True for its search and initial system objects (System, Connect,
etc.) are not checked for. */

Boolean SyncsAreConsec(LINK syncA, LINK syncB, short staff, short voice)
{
	LINK pL;
	SearchParam	pbSearch;
	
	if (!SyncTYPE(syncA) || !SyncTYPE(syncB)) {
		MayErrMsg("SyncsAreConsec: syncA L%ld or syncB L%ld isn't SYNCtype",
					(long)syncA, (long)syncB);
		return False;
	}
	
	InitSearchParam(&pbSearch);
	pbSearch.id = staff;
	pbSearch.voice = voice;
	pbSearch.needSelected = False;
	pbSearch.inSystem = False;
	pbSearch.subtype = ANYSUBTYPE;
	pL = L_Search(RightLINK(syncA), SYNCtype, False, &pbSearch);
	return (pL==syncB);
}


/* --------------------------------------------- LinkBefFirstMeas, BeforeFirstPageMeas -- */
/* Is the object (not the selection range ptr or insertion point!) pL before the first
Measure of its System?  Cf. the BeforeFirstMeas() function.

NB: This makes no reference to the selection range. If doc->selStartL indicates an
insertion point at the end of a System, it will be the following system object (or tail);
if called with this object, this function will return True, because the System is before
the first Measure of its system. For selection range and insertion point, use
BeforeFirstMeas(). */
 
Boolean LinkBefFirstMeas(LINK pL)
{
	LINK qL;

	for (qL=pL; qL; qL=LeftLINK(qL))
		switch(ObjLType(qL)) {
			case HEADERtype:
			case PAGEtype:
			case SYSTEMtype:
			case STAFFtype:
				return True;
			case MEASUREtype:
			case TAILtype:
				return False;
			default:
				return BeforeFirstMeas(pL);
				;
		}

	return False;										/* Should never get here */
}

/* Return True if pL is before the first measure of the first system of its page. */

short BeforeFirstPageMeas(LINK pL)
{
	LINK sysL;
	
	if (BeforeFirstMeas(pL)) {
		sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, False);
		if (!sysL) return True;					/* Before first system of score */
		if (!SamePage(sysL,pL)) return True;	/* Before first system of page */
		return FirstSysInPage(sysL);			/* sysL is pL's system; return True if first on page */
	}
	return False;								/* Not before any first measure. */
}


/* ----------------------------------------------------- Functions for BeforeFirstMeas -- */
static Boolean FirstPageInScore(LINK pL);
static Boolean HasFirstObj(LINK pL);
static LINK GetFirstObj(LINK pL);

static Boolean FirstPageInScore(LINK pL)
{
	return ( !LinkLPAGE(pL) );
}

static Boolean HasFirstObj(LINK pL)
{
	return ( DynamTYPE(pL) || GraphicTYPE(pL) || TempoTYPE(pL) || EndingTYPE(pL) );
}

static LINK GetFirstObj(LINK pL)
{
	switch (ObjLType(pL)) {
		case DYNAMtype:
			return DynamFIRSTSYNC(pL);
		case GRAPHICtype:
			return GraphicFIRSTOBJ(pL);
		case TEMPOtype:
			return TempoFIRSTOBJ(pL);
		case ENDINGtype:
			return EndingFIRSTOBJ(pL);
		default:
			return NILINK;
	}
}

/* ------------------------------------------------------------------- BeforeFirstMeas -- */
/* Return True if selection range ptr or insertion point (not object!) pL is before
the first Measure of its System. Specifically:
	If pL is the head, return True; if pL is the tail, return False.
	If pL is a Page, return True if pL is the first page of the score; return
		False if pL is any succeeding Page, since that represents an insertion
		point at the end of the previous Page.
	If pL is a System, return True if pL is the first System of its Page, but
		False for any succeeding System on the Page, since that represents an
		insertion point at the end of the previous System.
	If pL is the first Measure of its System, return False.

The equivalent function for objects is LinkBefFirstMeas().

BeforeFirstMeas depends upon certain constraints on the ordering of the object
list, as follows:
#1. This will work correctly if there is nothing belonging to a non-FirstInScore
page located prior to that page object.
#2. This will work correctly if there is nothing belonging to a non-FirstInPage
system located prior to that system object.
#3. If before the first system of a succeeding page, Search right for system
from pL will return last system of previous page; its SysPAGE will not be the
same page as pageL, => return True.
Same dependency as #1: However, if belong to the page and before it, the pageL
tested will also be the previous page, and test will fail.
*/

Boolean BeforeFirstMeas(LINK pL)
{
	LINK firstL, pageL, sysL, firstMeasL;
	
	switch (ObjLType(pL)) {
		case HEADERtype:
			return True;									/* Before first meas of score */
		case TAILtype:
			return False;									/* After everything */
		case PAGEtype:
			if (FirstPageInScore(pL)) return True;			/* Before first meas of score */
			return False;									/* At end of prev Page.  Cf. #1. */
		case SYSTEMtype:
			if (FirstSysInPage(pL)) return True;			/* Between Page obj and 1st meas */
			return False;									/* At end of prev System. Cf. #2. */
		default:
			if (J_DTYPE(pL) && HasFirstObj(pL)) {
				firstL = GetFirstObj(pL);
				if (PageTYPE(firstL) || SystemTYPE(firstL)) return True;
			}

			pageL = LSSearch(pL, PAGEtype, ANYONE, GO_LEFT, False);
			if (!pageL) return True;						/* Before any Page */

			sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, False);
			if (!sysL) return True;							/* Before any System */			

			if (SysPAGE(sysL)!=pageL) return True;			/* Before the 1st System of a succeeding page. Cf. #3. */
		
			firstMeasL = LSSearch(sysL, MEASUREtype, ANYONE, GO_RIGHT, False);
			return ( IsAfter(pL,firstMeasL) );				/* Return True if firstMeasL is after pL. */
	}
}

/* -------------------------------------------------------------------- GetCurrentPage -- */
/* Get the current Page object. */

LINK GetCurrentPage(Document *doc)
{
	short sheetNum; LINK pL;
	
	sheetNum  = doc->currentSheet;
	pL = doc->headL;
	while (!PageTYPE(pL)) pL = RightLINK(pL);

	while (SheetNUM(pL) < sheetNum)
		pL = LinkRPAGE(pL);
	return pL;
}


/* --------------------------------------------------------------------- GetMasterPage -- */
/* Get the Master Page's Page object. */

LINK GetMasterPage(Document *doc)
{
	LINK pL;
	
	pL = doc->masterHeadL;
	while (!PageTYPE(pL)) pL = RightLINK(pL);
	return pL;
}


/* ----------------------------------------------------------------------- GetSysRange -- */
/* Find the smallest range of Systems that includes the range of objects
[startL, endL). */

void GetSysRange(Document */*doc*/, LINK startL, LINK endL, LINK *startSysL, LINK *endSysL)
{
	if (PageTYPE(startL))
		startL =  LeftLINK(startL);
	if (SystemTYPE(startL)) {
		if (FirstSysInPage(startL))
			;
		startL =  LeftLINK(startL);
	}
	
	/*
	 *	Start with the System containing startL. If there is no System to the left
	 *	in the object list, startL must be at the very beginning of the score;
	 *	in that case use the first System. Likewise for endL.
	 */
	*startSysL = EitherSearch(startL, SYSTEMtype, ANYONE, GO_LEFT, False);
	*endSysL = EitherSearch(LeftLINK(endL), SYSTEMtype, ANYONE, GO_LEFT, False);
}


/* --------------------------------------- Find or check for "First" of various things -- */

/* Return the first Measure of the System containing pL. pL is regarded as a LINK,
not a selRange ptr. If no System can be found, or no Measure, return NILINK. */

LINK FirstSysMeas(LINK pL)
{
	LINK sysL, firstMeasL;

	sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, False);
	if (!sysL) return NILINK;
	
	firstMeasL = LSSearch(sysL, MEASUREtype, ANYONE, GO_RIGHT, False);
	return firstMeasL;
}

/* Return the first measure of the document. If no measure can be found, return NILINK. */

LINK FirstDocMeas(Document *doc)
{
	return LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);	 
}

/* Is <measL>, which must be a Measure, the first Measure of its System? */

Boolean FirstMeasInSys(LINK measL)
{
	LINK sysL;

	if (!MeasureTYPE(measL)) {
		MayErrMsg("FirstMeasInSys: Object L%ld is not a Measure.", (long)measL);
		return False;
	}
	if (LinkLMEAS(measL)) {
		sysL = LSSearch(measL, SYSTEMtype, ANYONE, True, False);
		if (sysL) 
			return(IsAfter(LinkLMEAS(measL), sysL));
		return False;										/* There's a Measure preceding but no System */
	}
	return True;											/* This is first Measure of entire score */
}


/* -------------------------------------------- Find or check "Last" of various things -- */

/* Is <measL>, which must be a Measure, the last Measure of its System? */

Boolean LastMeasInSys(LINK measL)
{
	LINK sysL;
	
	if (!MeasureTYPE(measL)) {
		MayErrMsg("LastMeasInSys: Object L%ld is not a Measure.", (long)measL);
		return False;
	}
	if (LinkRMEAS(measL)) {
		sysL = LSSearch(measL, SYSTEMtype, ANYONE, False, False);
		if (sysL)
			return(IsAfter(sysL, LinkRMEAS(measL)));
		return False;
	}
	return True;							/* Because last meas. of entire score */
}

/* Is <measL>, which must be a Measure, the last "used" Measure of its System, i.e., is
there nothing following it except possibly a barline? */

Boolean IsLastUsedMeasInSys(Document *doc, LINK measL)
{
	LINK pL;

	if (LastMeasInSys(measL)) return True;

	pL = EndMeasSearch(doc, measL);									/* pL must be a Measure */
	return (EndMeasSearch(doc, pL)==RightLINK(pL));

}

/* If pL is not a System, if there is a system which contains pL, return the last obj
on it. If there is no containing System, return pL (we should be returning doc->headL).
If pL is a System, return the LINK of last object on the previous System in the score,
skipping any intervening Page object if the System in question is the first System on
the Page.  If pL is the first System on the first Page, return the head. */

LINK LastOnPrevSys(LINK pL)
{
	LINK sysL=pL;

	if (!SystemTYPE(sysL)) {
		sysL = LSSearch(sysL, SYSTEMtype, ANYONE, GO_LEFT, False);
		if (!sysL) return pL;

		return LastOnPrevSys(sysL);
	}

	if (FirstSysInPage(sysL))
		return LeftLINK(SysPAGE(sysL));

	return LeftLINK(sysL);
}

/* Return True if the given link is the last object in its System. */

Boolean IsLastInSystem(LINK pL)
{
	if (SystemTYPE(RightLINK(pL)) || PageTYPE(RightLINK(pL))) return True;
	return False;
}


/* Return the last Object of pL's System. If pL is itself a System, will return the last
object of the previous System. */

LINK LastObjInSys(Document *doc, LINK pL)
{
	LINK sysL;
	
	sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_RIGHT, False);	/* Get succeeding System */
	if (!sysL)
		return (LeftLINK(doc->tailL));
		
	if (FirstSysInPage(sysL))									/* Avoid pageRel objs */
		return (LeftLINK(SysPAGE(sysL)));
	
	return (LeftLINK(sysL));
}

/* Determines if sysL is the last system of its page. Assumes that sysL is a SYSTEM. */

Boolean IsLastSysInPage(LINK sysL)
{
	/* Either sysL is the last system of the score, on a different page from the
	   following system, or not the first system of its page. */
	
	if (LinkRSYS(sysL))
		return (SysPAGE(sysL) != SysPAGE(LinkRSYS(sysL)));		/* On different page from following system */
	return True;												/* Last system of score */
}


/* Return the last system in page pageL. */

LINK GetLastSysInPage(LINK pageL)
{
	LINK sysL;

	sysL = SSearch(pageL,SYSTEMtype,GO_RIGHT);

	for ( ; sysL; sysL=LinkRSYS(sysL)) {
		if (IsLastSysInPage(sysL)) return sysL;
	}
	
	return NILINK;
}

/* --------------------------------------------------------- FirstSysInPage,, -InScore -- */
/* Determines if sysL is the first System of its page. Assumes that sysL is a LINK
to an object of type SYSTEM. NB: We cannot assume that System always immediately
follows Page, because of Page-relative Graphics. FIXME: Name should be changed to
IsFirstSysInPage. Similarly for FirstSysInScore. */

Boolean FirstSysInPage(LINK sysL)
{
	/* Either sysL is the first system of the score, on a different page from the
	   previous system, or not the first system of its page. */
	
	if (LinkLSYS(sysL))										
		return (SysPAGE(sysL)!=SysPAGE(LinkLSYS(sysL)));	/* On different page from prev sys */

	return True;											/* First system of score */
}

/* Return True if sysL is the first System of the score. Assumes that sysL is a LINK
to an object of type SYSTEM. */

Boolean FirstSysInScore(LINK sysL)
{
	return (!LinkLSYS(sysL));
}

/* ------------------------------------------------------------------------ NSysOnPage -- */
/* Returns number of Systems on page <pageL>. */

short NSysOnPage(LINK pageL)
{
	LINK sysL;  short nSys;
	
	sysL = SSearch(pageL, SYSTEMtype, False);

	for (nSys=0; sysL && SysPAGE(sysL)==pageL; sysL=LinkRSYS(sysL))
		nSys++;
		
	return nSys;
}


/* --------------------------------------------------------------------- LastObjOnPage -- */
/* If pageL is a page LINK, return the last object on that page; else return NILINK. */

LINK LastObjOnPage(Document *doc, LINK pageL)
{
	LINK sysL, rPage;

	if (!PageTYPE(pageL)) return NILINK;

	rPage = LinkRPAGE(pageL);

	/* If the rPage exists, get the last system prior to it, and the last obj in that
	   system, which is the last obj in pageL. */

	if (rPage) {
		sysL = LSSearch(rPage, SYSTEMtype, ANYONE, True, False);
		return LastObjInSys(doc, RightLINK(sysL));
	}
	return LeftLINK(doc->tailL);
}


/* --------------------------------------------------------------------- GetNextSystem -- */
/* Return the System following pL, if one exists; else doc->tailL. If pL is a
System, return the following System, if any. */

LINK GetNextSystem(Document *doc, LINK pL)
{
	LINK nextSysL;
	
	nextSysL = LSSearch(RightLINK(pL), SYSTEMtype, ANYONE, False, False);
	return (nextSysL ? nextSysL : doc->tailL);
}


/* ----------------------------------------------------- RoomFor-, MasterRoomForSystem -- */
/* Is there room for another System on Page containing pL? Assumes the Page
already contains at least one System. */

Boolean RoomForSystem(Document *doc, LINK pL)
{
	LINK	staffL, aStaffL, systemL, endPageL;
	PASTAFF	aStaff;
	PSYSTEM	pSystem;
	Boolean	canAddSystem;
	DDIST	sysHeight=0; 
	
	/*
	 * Get system height from the last System on the page because Systems
	 * may have varying heights.
	 */
	endPageL = EndPageSearch(doc, pL);
	staffL = LSSearch(endPageL, STAFFtype, ANYONE, True, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==doc->nstaves) {
			aStaff = GetPASTAFF(aStaffL);
			sysHeight = MEAS_BOTTOM(aStaff->staffTop, drSize[doc->srastral]);
			break;
		}
	systemL = LSSearch(endPageL, SYSTEMtype, ANYONE, True, False);
	pSystem = GetPSYSTEM(systemL);
	canAddSystem = (d2pt(pSystem->systemRect.bottom+sysHeight)
						< doc->marginRect.bottom);
	return canAddSystem;
}

/* See if room exists for another system in the masterPage. Gets sysHeight
from masterPage's only system. */

Boolean MasterRoomForSystem(Document *doc, LINK sysL)
{
	DRect sysRect;  DDIST sysHeight;

	sysRect = SystemRECT(sysL);
	sysHeight = sysRect.bottom - sysRect.top;

	/* System has already been offset by sysHeight+doc->yBetweenSys by the
		time we have gotten here. */
	return (d2pt(sysRect.bottom) < doc->marginRect.bottom);
}


/* ------------------------------------------------------------------------ NextSystem -- */
/*	Return pointer to the next System object, or tailL if there is none. sysL
must be a System object. */

LINK NextSystem(Document *doc, LINK sysL)
{
	return (LinkRSYS(sysL) ? LinkRSYS(sysL) : doc->tailL);
}


/* --------------------------------------------------------------------------- GetNext -- */
/* Get the next link in the object list to the left or right, as specified by
<goLeft>. */

LINK GetNext(LINK pL, Boolean goLeft)
{
	return (goLeft ? LeftLINK(pL) : RightLINK(pL));
}


/* ------------------------------------------------------------------------ RSysOnPage -- */
/* Return the next System on the Page sysL is in, or NILINK if there is none. */

LINK RSysOnPage(Document */*doc*/, LINK sysL)
{
	LINK rSysL;
	
	rSysL = LinkRSYS(sysL);
	if (rSysL)
		if (SysPAGE(rSysL)==SysPAGE(sysL))
			return rSysL;
	return NILINK;
}


/* ------------------------------------------------- MoveInMeasure, MoveMeasures, etc. -- */
/* MoveInMeasure moves objects [startL,endL) by diffxd. If endL is not in the
same Measure as startL, it'll stop moving at the end of the Measure (since the object
ending the Measure will always have a valid xd). Returns the first object after the
range it moved; to move following Measures by the same distance, use this as the
starting point in a call to MoveMeasures.

Comments on optimization:
	#1. If endL is not after the first object that should be moved, just return endL.
	Usually requires less work than <if (IsAfter(endL,pL))>, since in that case will
	usually be traversing all the way to tailL, but here usually only to endL.
	#2. Loop optimization. Testing with IsAfter(endL,pL) and traversing with
	FirstValidxd requires a traversal to end of specified, and one from pL
	to next node with valid xd, for every iteration through the loop, in the
	ordinary case; this requires one traversal from startL to endL, and one from
	pL to next node with valid xd, for each iteration, in every case. */

LINK MoveInMeasure(LINK startL, LINK endL, DDIST diffxd)
{
	LINK pL,nextL;

	pL = nextL = FirstValidxd(startL, GO_RIGHT);
	if (!IsAfter(pL,endL)) return endL;							/* #1. */

	for ( ; pL!=endL && !MeasureTYPE(pL) && !J_STRUCTYPE(pL); pL=RightLINK(pL))
		if (pL==nextL) {										/* #2. */
			nextL = FirstValidxd(RightLINK(pL), GO_RIGHT);
			LinkXD(pL) += diffxd;
		}

	return pL;
}


/* MoveMeasures moves Measure objects in [startL,endL) by diffxd. Since positions
of objects within Measures are relative to their Measures, this effectively moves the
contents of the Measures as well. Stops moving Measures when it reaches the first
System or Page following startL, even if endL hasn't been reached yet. */

void MoveMeasures(LINK startL, LINK endL, DDIST diffxd)
{
	LINK pL;

	for (pL = startL; pL!=endL && !J_STRUCTYPE(pL); pL=RightLINK(pL))
		if (MeasureTYPE(pL))
			LinkXD(pL) += diffxd;
}

/* MoveAllMeasures moves Measure objects in [startL,endL) by diffxd. Since positions
of objects within Measures are relative to their Measures, this effectively moves the
contents of the Measures as well. Unlike MoveMeasures, doesn't stop moving until
endL, no matter what it encounters along the way. */

void MoveAllMeasures(LINK startL, LINK endL, DDIST diffxd)
{
	LINK pL;

	for (pL = startL; pL!=endL; pL=RightLINK(pL))
		if (MeasureTYPE(pL))
			LinkXD(pL) += diffxd;
}

/* Versions of the above which set the context to fixDoc and restore it to oldDoc. */

LINK DMoveInMeasure(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL,
						DDIST diffxd)
{
	LINK pL,nextL;
	
	InstallDoc(fixDoc);
	pL = nextL = FirstValidxd(startL, GO_RIGHT);
	if (!IsAfter(pL,endL)) return endL;							/* See #1 for MoveInMeasure. */

	for ( ; pL!=endL && !MeasureTYPE(pL) && !J_STRUCTYPE(pL); pL=RightLINK(pL))
		if (pL==nextL) {										/* See #2. */
			nextL = FirstValidxd(RightLINK(pL), GO_RIGHT);
			LinkXD(pL) += diffxd;
		}

	InstallDoc(oldDoc);
	return pL;
}


void DMoveMeasures(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL,
						DDIST diffxd)
{
	LINK pL;

	InstallDoc(fixDoc);
	for (pL = startL; pL!=endL && !J_STRUCTYPE(pL); pL=RightLINK(pL))
		if (MeasureTYPE(pL))
			LinkXD(pL) += diffxd;

	InstallDoc(oldDoc);
}


/* Check to see if the operation of MoveMeasures starting at pStartL by diffxd will 
cause the contents of the system to extend past the system boundaries. The range of
MoveMeasures is restricted to objects within a single system, so this function also
considers only that range. */

Boolean CanMoveMeasures(Document *doc, LINK pStartL, DDIST diffxd)
{
	LINK pSystemL, lastObjL;  PSYSTEM pSystem;
	DDIST sysWidth;

	pSystemL = LSSearch(pStartL, SYSTEMtype, doc->currentSystem, True, False);
	pSystem = GetPSYSTEM(pSystemL);
	sysWidth = pSystem->systemRect.right - pSystem->systemRect.left;
	lastObjL = LastObjInSys(doc, pStartL);
	if ((SysRelxd(lastObjL) + diffxd) > p2d(sysWidth)) return False;
	return True;
}


/* --------------------------------------------------------------- CanMoveRestOfSystem -- */
/* Given a Measure and the width of that Measure, if there's anything following
in that Measure's System, move that following stuff left or right as appropriate
so it starts where the Measure ends. Be careful to stop moving at the end of the
System. Intended for use by routines that change the width of a Measure or Measures. */

Boolean CanMoveRestOfSystem(Document *doc, LINK measL, DDIST measWidth)
{
	LINK nextMeasL, endSysL, lastMeasL;
	DDIST diffxd;
	
	nextMeasL = LinkRMEAS(measL);
	if (nextMeasL) {
		/*
		 *	Distance to shift is the right end of the Measure minus the left
		 *	end of the Measure following.
		 */
		diffxd = LinkXD(measL)+measWidth-LinkXD(nextMeasL);
		if (diffxd!=0) {
			endSysL = EndSystemSearch(doc, measL);
			if (!IsAfter(endSysL, nextMeasL)) {
				MoveMeasures(nextMeasL, endSysL, diffxd);
				lastMeasL = LSSearch(endSysL, MEASUREtype, ANYONE, True, False);
				if (!SetMeasFillSystem(lastMeasL)) return False;
			}
		}
	}
	return True;
}


/* ----------------------------------------------------------------------- SyncAbsTime -- */

long SyncAbsTime(LINK syncL)
{
	LINK measL; long absTime;
	
	measL = SSearch(syncL, MEASUREtype, GO_LEFT);
	if (measL) {
		absTime = SyncTIME(syncL);
		return absTime+MeasureTIME(measL);
	}
	return -1L;
}


/* ----------------------------------------------- MoveTimeInMeasure, MoveTimeMeasures -- */
/* MoveTimeInMeasure changes the timeStamps of Syncs in [startL,endL) by diffTime.
If endL is not in the same Measure as startL, it'll stop moving at the end of the
Measure. */

LINK MoveTimeInMeasure(LINK startL, LINK endL, long diffTime)
{
	LINK pL;

	for (pL = startL; pL && pL!=endL && !MeasureTYPE(pL); pL = RightLINK(pL))
		if (SyncTYPE(pL))
			SyncTIME(pL) += diffTime;

	return pL;
}


/* MoveTimeMeasures changes the lTimeStamps of Measures in [startL,endL) by diffTime.
Since timeStamps of Syncs are relative to their Measures, this effectively moves
the contents of the Measures as well. */

void MoveTimeMeasures(LINK startL, LINK endL, long diffTime)
{
	LINK pL;

	for (pL = startL; pL && pL!=endL; pL = RightLINK(pL))
		if (MeasureTYPE(pL)) MeasureTIME(pL) += diffTime;
}


/* ===================================================== Object and Subobject Counting == */

/* ------------------------------------------------ CountNotesInRange, -GRNotesInRange -- */
/* Counts notes in range [startL,endL) on staff <staff>, not including rests, and
with each chord counting as 1 note. */

unsigned short CountNotesInRange(short staff, LINK startL, LINK endL, Boolean selectedOnly)
{
	unsigned short numNotes;
	LINK pL, aNoteL;

	for (numNotes = 0, pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSTAFF(aNoteL)==staff && !NoteREST(aNoteL))																/* Yes */
					if (MainNote(aNoteL) && (NoteSEL(aNoteL) || !selectedOnly))
						numNotes++;
		}
	return numNotes;
}


/* Counts GRNotes in range [startL,endL) on staff <staff> (there are no GRRrests),
and with each chord counting as 1 GRNote. */

unsigned short CountGRNotesInRange(short staff, LINK startL, LINK endL, Boolean selectedOnly)
{
	unsigned short numGRNotes;
	LINK pL, aGRNoteL;

	for (numGRNotes = 0, pL = startL; pL!=endL; pL = RightLINK(pL))
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
				if (GRNoteSTAFF(aGRNoteL)==staff)																/* Yes */
					if (GRMainNote(aGRNoteL) && (GRNoteSEL(aGRNoteL) || !selectedOnly))
						numGRNotes++;
		}
	return numGRNotes;
}


/* --------------------------------------------------------- CountNotes -- */
/* Counts notes/rests in range on staff, with each Sync (not chord: they can be
different if there are multiple voices on the staff) optionally counting as 1 note.
Cf. CountNotesInRange: should perhaps be combined with it, but NB: that function ignores
rests, and it gives a count of chords (not notes or Syncs). Also cf. SVCountNotes. */

unsigned short CountNotes(short staff,
					LINK startL, LINK endL,
					Boolean syncs						/* True=each Sync counts as 1 note */
					)
{
	unsigned short noteCount;
	LINK pL, aNoteL;
	Boolean	anyStaff;
	
	anyStaff = (staff==ANYONE);

	noteCount = 0;
	for (pL = startL; pL!= endL; pL=RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (anyStaff || NoteSTAFF(aNoteL)==staff) {
					noteCount++;
					if (syncs) break;
				}
		}
	return noteCount;
}

/* ----------------------------------------------------------------------- VCountNotes -- */
/* Counts notes/rests in range in voice. Cf. CountNotesInRange. */

unsigned short VCountNotes(short v,
						LINK startL, LINK endL,
						Boolean chords					/* True=each chord counts as 1 note */
						)
{
	unsigned short noteCount;
	LINK pL, aNoteL;
	Boolean	anyVoice;
	
	anyVoice = (v==ANYONE);

	noteCount = 0;
	for (pL = startL; pL!= endL; pL=RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (anyVoice || NoteVOICE(aNoteL)==v) {
					noteCount++;
					if (chords) break;
				}
		}
	return noteCount;
}


/* ---------------------------------------------------------------------- CountGRNotes -- */
/* Counts grace notes in range on staff, with each GRSync (not chord: they can be
different if there are multiple voices on the staff) optionally counting as 1 note. */

unsigned short CountGRNotes(short staff, 
						LINK startL, LINK endL,
						Boolean syncs				/* True=each GRSync counts as 1 note */
						)
{
	unsigned short noteCount;
	LINK pL, aGRNoteL;
	Boolean anyStaff;
	
	anyStaff = (staff==ANYONE);

	noteCount = 0;
	for (pL = startL; pL!= endL; pL=RightLINK(pL))
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
				if (anyStaff || GRNoteSTAFF(aGRNoteL)==staff) {
					noteCount++;
					if (syncs) break;
				}
		}
	return noteCount;
}


/* ---------------------------------------------------------------------- SVCountNotes -- */
/* Counts notes/rests in range in voice and staff. Cf. CountNotesInRange. */

unsigned short SVCountNotes(short staff, short v, LINK startL,
							LINK endL,
							Boolean chords			/* True=each chord counts as 1 note */
							)
{
	Boolean	anyVoice = (v<0);
	unsigned short noteCount;
	LINK pL, aNoteL;
	
	noteCount = 0;
	for (pL = startL; pL!= endL; pL=RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				if ((anyVoice || NoteVOICE(aNoteL)==v) && NoteSTAFF(aNoteL)==staff) {
					noteCount++;
					if (chords) break;
				}
		}
	return noteCount;
}

/* -------------------------------------------------------------------- SVCountGRNotes -- */
/* Counts grace notes in range in voice and staff. Cf. CountNotesInRange. */

unsigned short SVCountGRNotes(short staff, short v,
								LINK startL, LINK endL,
								Boolean chords				/* True=each chord counts as 1 note */
								)
{
	Boolean	anyVoice = (v<0);
	unsigned short noteCount;
	LINK pL, aGRNoteL;
	
	noteCount = 0;
	for (pL = startL; pL!= endL; pL=RightLINK(pL))
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
				if ((anyVoice || GRNoteVOICE(aGRNoteL)==v) && GRNoteSTAFF(aGRNoteL)==staff) {
					noteCount++;
					if (chords) break;
				}
		}
	return noteCount;
}

/* ------------------------------------------------------------------ CountNoteAttacks -- */

unsigned short CountNoteAttacks(Document *doc)
{
	LINK pL, aNoteL;
	short nNoteAttacks = 0;
	
	for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (!NoteREST(aNoteL) && !NoteTIEDL(aNoteL)) nNoteAttacks++;
	}
	return nNoteAttacks;
}

/* ---------------------------------------------------------------------- CountObjects -- */
/* Count objects in range of the given type or of all types. */

unsigned short CountObjects(LINK startL, LINK endL,
							short type)					/* Object type or ANYTYPE */
{
	unsigned short count;  LINK pL;
	
	for (count = 0, pL = startL; pL && pL!=endL; pL = RightLINK(pL))
		if (type==ANYTYPE || ObjLType(pL)==type)
			count++;
	
	return count;
}


/* ---------------------------------------------------------------- CountSubobjsByHeap -- */
/* Count the subobjects in each heap of the given score. NB: The numbers it returns may
not agree with the user's concept of things: in particular, they include context-setting
KeySig subobjects at the beginning of each system, which are invisible if there's not a
"real" key signature in effect. */

void CountSubobjsByHeap(Document *doc,
					unsigned short subobjCount[],
					Boolean includeMP				/* True=include Master Page object list */
					)
{
	unsigned short h, numMods=0;
	LINK pL, aNoteL, aModNRL;  PANOTE aNote;	

	for (h = FIRSTtype; h<LASTtype; h++ )
		subobjCount[h] = 0;
	
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		subobjCount[ObjLType(pL)] += LinkNENTRIES(pL);
	}

	/* Optionally add in the numbers of objects in the Master Page of each object type. */

	if (includeMP)
		for (pL = doc->masterHeadL; pL; pL = RightLINK(pL)) {
			subobjCount[ObjLType(pL)] += LinkNENTRIES(pL);
		}

	/* Compute the numbers of note modifiers in the score. (This is the only type with
		subobjects but no objects: note modifier subobjects are attached to Sync
		subobjects.) */

	for (pL = doc->headL; pL!=RightLINK(doc->tailL); pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				aModNRL = aNote->firstMod;
				if (aModNRL) {
					for ( ; aModNRL; aModNRL = NextMODNRL(aModNRL))
						numMods++;
				}
			}
		}
	subobjCount[MODNRtype] = numMods;
}


/* ------------------------------------------------------------- Note/grace note stems -- */

Boolean HasOtherStemSide(LINK syncL,
							short staff)			/* staff no. or ANYONE */
{
	LINK aNoteL; PANOTE aNote;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if ((staff==ANYONE || aNote->staffn==staff) && aNote->otherStemSide) return True;
	}
	return False;
}

/* Is the given note on the left side of its stem or its chord's stem? */

Boolean IsNoteLeftOfStem(LINK /*pL*/,			/* Sync note belongs to */
						LINK theNoteL,			/* Note */
						Boolean stemDown
						)
{
	PANOTE	theNote;
	
	theNote = GetPANOTE(theNoteL);
	return (stemDown==theNote->otherStemSide);
}

/* Deliver 1 if the note/chord for the given voice and Sync is stem up, -1
if stem down. If the voice and Sync has a rest or nothing, return 0. */

short GetStemUpDown(LINK syncL, short voice)
{
	LINK aNoteL;
	PANOTE aNote;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (MainNote(aNoteL) && NoteVOICE(aNoteL)==voice) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->rest) return 0;
			if (aNote->ystem < aNote->yd) return 1;
			return -1;
		}
		
	return 0;				/* No MainNote in voice */
}

/* Deliver 1 if the note/chord for the given voice and GRSync is stem up, -1
if stem down. If the voice and GRSync has nothing, return 0. */

short GetGRStemUpDown(LINK grSyncL, short voice)
{
	LINK aGRNoteL;
	PAGRNOTE aGRNote;
	
	aGRNoteL = FirstSubLINK(grSyncL);
	for ( ; aGRNoteL; aGRNoteL = NextNOTEL(aGRNoteL))
		if (GRMainNote(aGRNoteL) && GRNoteVOICE(aGRNoteL)==voice) {
			aGRNote = GetPAGRNOTE(aGRNoteL);
			if (aGRNote->ystem < aGRNote->yd) return 1;
			return -1;
		}
		
	return 0;				/* No MainNote in voice */
}


/* ----------------------------------------------------------- GetExtremeNotes/GRNotes -- */
/* Return the highest and lowest (in terms of y-position, not pitch!) notes of the
chord in a given Sync and voice. If there's only one note in that Sync/voice,
return that note as both highest and lowest; if there are no notes in the Sync/voice,
return NILINK for each. ??FindExtremeNote should call it (and FindExtremeGRNote
should call a similar function). */

void GetExtremeNotes(LINK syncL,
						short voice,
						LINK *pLowNoteL,
						LINK *pHiNoteL)
{
	DDIST maxy, miny;
	LINK aNoteL, lowNoteL, hiNoteL;
	
	maxy = (DDIST)(-9999);
	miny = (DDIST)9999;
	lowNoteL = NILINK;
	hiNoteL = NILINK;
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			if (NoteYD(aNoteL)>maxy) {
				maxy = NoteYD(aNoteL);
				lowNoteL = aNoteL;							/* yd's increase going downward */
			}
			if (NoteYD(aNoteL)<miny) {
				miny = NoteYD(aNoteL);
				hiNoteL = aNoteL;
			}
		}
	}
	
	*pLowNoteL = lowNoteL;
	*pHiNoteL = hiNoteL;
}

/* Return the highest and lowest (in terms of y-position, not pitch!) grace notes of
the chord in a given GRSync and voice. If there's only one grace note in that
GRSync/voice, return that note as both highest and lowest; if there are no grace notes
in the GRSync/voice, return NILINK for each. FIXME: FindExtremeGRNote should call it. */

void GetExtremeGRNotes(LINK grSyncL,
							short voice,
							LINK *pLowGRNoteL,
							LINK *pHiGRNoteL)
{
	DDIST maxy, miny;
	LINK aGRNoteL, lowGRNoteL, hiGRNoteL;
	
	maxy = (DDIST)(-9999);
	miny = (DDIST)9999;
	lowGRNoteL = NILINK;
	hiGRNoteL = NILINK;
	aGRNoteL = FirstSubLINK(grSyncL);
	for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
		if (GRNoteVOICE(aGRNoteL)==voice) {
			if (GRNoteYD(aGRNoteL)>maxy) {
				maxy = GRNoteYD(aGRNoteL);
				lowGRNoteL = aGRNoteL;							/* yd's increase going downward */
			}
			if (GRNoteYD(aGRNoteL)<miny) {
				miny = GRNoteYD(aGRNoteL);
				hiGRNoteL = aGRNoteL;
			}
		}
	}
	
	*pLowGRNoteL = lowGRNoteL;
	*pHiGRNoteL = hiGRNoteL;
}


/* --------------------------------------------------------------- FindMain/GRMainNote -- */
/* Return the MainNote/GRMainNote of a chord (the one with a stem) in the given voice and
Sync/GRSync, if there is one; else return NILINK. */

LINK FindMainNote(LINK pL,			/* Sync or GRSync */
					short voice
					)
{
	LINK aNoteL, aGRNoteL;

	if (SyncTYPE(pL)) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			if (NoteVOICE(aNoteL)==voice && MainNote(aNoteL)) return aNoteL;
		}
	}
	else if (GRSyncTYPE(pL)) {
		aGRNoteL = FirstSubLINK(pL);
		for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
			if (GRNoteVOICE(aGRNoteL)==voice && GRMainNote(aGRNoteL)) return aGRNoteL;
		}
	}

	return NILINK;
}

/* Return the GRMainNote (the one with a stem) of a chord in the given voice and GRSync,
if there is one; else return NILINK. */

LINK FindGRMainNote(LINK pL,		/* GRSync */
					short voice
					)
{
	LINK aGRNoteL;

	aGRNoteL = FirstSubLINK(pL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		if (GRNoteVOICE(aGRNoteL)==voice && GRMainNote(aGRNoteL))
			return aGRNoteL;
	}
	return NILINK;
}

/* Return the MainNote of a chord (the one with a stem) in the given voice and Sync,
or the only note in the voice and Syncif there is one; if there are no notes in the voice
and Sync, return NILINK. */

LINK FindMainOrOnlyNote(LINK pL,			/* Sync */
						short voice
						)
{
	LINK aNoteL;

	if (SyncTYPE(pL)) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			if (NoteVOICE(aNoteL)==voice && !NoteINCHORD(aNoteL)) return aNoteL;
			if (NoteVOICE(aNoteL)==voice && MainNote(aNoteL)) return aNoteL;
		}
	}

	return NILINK;
}



/* ------------------------------------------------------------------- GetObjectLimits -- */

void GetObjectLimits(short type,
						short *minEntries, short *maxEntries,
						Boolean *objRectOrdered	/* True=objRect meaningful & its .left should be in order */
						)
{
	short i, loc;

	for (loc = -1, i = FIRSTtype; i<LASTtype; i++)
		if (objTable[i].objType==type)
			{ loc = i; break; }
	if (loc<0) {
		MayErrMsg("GetObjectLimits: unknown type %ld", (long)type);
		*minEntries = 1;
		*maxEntries = MAXSTAVES;
		*objRectOrdered = True;
	}
	else {
		*minEntries = objTable[i].minEntries;
		*maxEntries = objTable[i].maxEntries;
		*objRectOrdered = objTable[i].objRectOrdered;
	}
	return;
}


/* ---------------------------------------------------------------------- InObjectList -- */
/* See if pL is in the object list indicated by whichList. */

Boolean InObjectList(Document *doc, LINK pL,
						short whichList				/* Whether main, clipboard, or Undo */
						)
{
	LINK qL;
	
	switch (whichList) {
		case MAIN_DSTR:
			for (qL = doc->headL; qL; qL=RightLINK(qL))
				if (pL==qL) return True;
			return False;
		case CLIP_DSTR:
			for (qL = clipboard->headL; qL; qL=RightLINK(qL))
				if (pL==qL) return True;
			return False;
		case UNDO_DSTR:
			for (qL = doc->undo.headL; qL!=doc->undo.tailL; qL=RightLINK(qL))
				if (pL==qL) return True;
			return False;
		default:
			;
	}

	MayErrMsg("InObjectList: illegal type %ld.", whichList);
	return False;
}


/* -------------------------------------------------------------------- GetSubObjStaff -- */
/* Given an index into the subobj list of an object, return the staffn of the subobj
corresponding to that index, or NOONE if no such subobj exists. For objects of type
PEXTEND, ignores index and returns the staffn of the object as a whole. Default gives
error message and returns NOONE. PAGEs return NOONE. */

short GetSubObjStaff(LINK pL, short index)
{
	short		i;
	GenSubObj 	*subObj;
	LINK		subObjL;
	POBJHDR		p;
	HEAP		*tmpHeap;

	p = GetPOBJHDR(pL);

	switch (ObjLType(pL)) {
		case PAGEtype:
			return NOONE;
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case SYNCtype:
		case GRSYNCtype:
		case MEASUREtype:
		case PSMEAStype:
		case DYNAMtype:
		case RPTENDtype:
			tmpHeap = Heap + p->type;		/* p may not stay valid during loop */
			
			for (i=0,subObjL=FirstSubObjPtr(p,pL); subObjL; i++, subObjL = NextLink(tmpHeap,subObjL)) {
				if (i==index) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
					return subObj->staffn;
				}
			}
			return NOONE;	
		case SLURtype:
		case BEAMSETtype:
		case TUPLETtype:
		case OTTAVAtype:
		case GRAPHICtype:
		case ENDINGtype:
		case TEMPOtype:
		case SPACERtype:
			return ((PEXTEND)p)->staffn;
		default:
			MayErrMsg("GetSubObjStaff: can't handle object type %ld at L%ld",
						(long)ObjLType(pL), (long)pL);
			return NOONE;
	}
}


/* -------------------------------------------------------------------- GetSubObjVoice -- */
/* Given an index into the subobj list of an object, return the voice of the
subobj corresponding to that index, or NOONE if no such subobj exists or if
that type of object doesn't have voice numbers. Default gives error message and
returns NOONE. */

short GetSubObjVoice(LINK pL, short index)
{
	short	i;
	LINK	aNoteL, aGRNoteL;

	switch (ObjLType(pL)) {
		case PAGEtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case MEASUREtype:
		case PSMEAStype:
		case DYNAMtype:
		case RPTENDtype:
		case ENDINGtype:
		case TEMPOtype:
		case SPACERtype:
		case OTTAVAtype:
			return NOONE;
		case SYNCtype:
			aNoteL = FirstSubLINK(pL);
			for (i = 0; aNoteL; i++, aNoteL = NextNOTEL(aNoteL))
				if (i==index) return NoteVOICE(aNoteL);
			return NOONE;	
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(pL);
			for (i = 0; aGRNoteL; i++, aGRNoteL = NextGRNOTEL(aGRNoteL))
				if (i==index) return GRNoteVOICE(aGRNoteL);
			return NOONE;	
		case SLURtype:
			return SlurVOICE(pL);
		case BEAMSETtype:
			return BeamVOICE(pL);
		case TUPLETtype:
			return TupletVOICE(pL);
		case GRAPHICtype:
			return GraphicVOICE(pL);
		default:
			MayErrMsg("GetSubObjVoice: can't handle object type %ld at L%ld",
						(long)ObjLType(pL), (long)pL);
			return NOONE;
	}
}


/* ------------------------------------------------------------------------ ObjOnStaff -- */
/* Return True if pL or one of its subobjects is on the given staff. Does not consider
cross-staff objects: a cross-staff slur with staffn=3 is in some sense also on staff 4,
but this function will not detect that. */

Boolean ObjOnStaff(LINK pL, short staff, Boolean selectedOnly)
{
	GenSubObj	*subObj;
	LINK		subObjL;
	POBJHDR		p;
	HEAP		*myHeap;

	switch (ObjLType(pL)) {
		case TAILtype:
			return False;
		case SYNCtype:
		case GRSYNCtype:
		case DYNAMtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case MEASUREtype:
		case PSMEAStype:
		case RPTENDtype:
			myHeap = Heap + ObjLType(pL);
			for (subObjL = FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(myHeap,subObjL)) {
				subObj = (GenSubObj *)LinkToPtr(myHeap,subObjL);
				if (subObj->staffn==staff)
					if (subObj->selected || !selectedOnly)
						return True;
			}
			return False;
		case SLURtype:
		case BEAMSETtype:
		case TUPLETtype:
		case OTTAVAtype:
		case GRAPHICtype:
		case TEMPOtype:
		case SPACERtype:
		case ENDINGtype:
			if (!LinkSEL(pL) && selectedOnly)
				return False;

			p = GetPOBJHDR(pL);
			return (((PEXTEND)p)->staffn==staff);
			
		/* Ignore <selectedOnly> for structural objects following */
		
		case SYSTEMtype:							/* This acts as if it's on EVERY staff */
		case PAGEtype:								/* This ditto */
			return True;
		default:
			if (TYPE_BAD(pL))
				MayErrMsg("ObjOnStaff: object L%ld has illegal type %ld",
							(long)pL, (long)ObjLType(pL));
			else
				MayErrMsg("ObjOnStaff: can't handle type %ld object at L%ld",
							(long)ObjLType(pL), (long)pL);
			return False;
	}
}


/* ----------------------------------------------------------------------- CommonStaff -- */
/* If there are any staves on which both objects have subobjects, return the
lowest-numbered staff they have in common; otherwise return NOONE. */

short CommonStaff(Document *doc, LINK obj1, LINK obj2)
{
	short s;
	
	for (s = 1; s<=doc->nstaves; s++)
		if (ObjOnStaff(obj1, s, False) && ObjOnStaff(obj2, s, False)) return s;
	
	return NOONE;
}


/* -------------------------------------------------------- ObjHasVoice, ObjSelInVoice -- */
/* Return True if pL belongs to a voice. */		

Boolean ObjHasVoice(LINK pL)
{
	switch (ObjLType(pL)) {
		case HEADERtype:
		case TAILtype:
		case PAGEtype:
		case SYSTEMtype:
		case STAFFtype:
		case CONNECTtype:
		case MEASUREtype:
		case PSMEAStype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case OTTAVAtype:
		case DYNAMtype:
		case MODNRtype:
		case RPTENDtype:
		case TEMPOtype:
		case SPACERtype:
		case ENDINGtype:
			return False;

		case SYNCtype:
		case GRSYNCtype:
		case BEAMSETtype:
		case TUPLETtype:
		case SLURtype:
			return True;
			
		case GRAPHICtype:
			return (GraphicVOICE(pL)!=NOONE);
		
		default:
			return False;
	}
}

/* If pL has a voice, if it is selected in the voice or one of its subObjects is,
return the obj or subObj. FIXME: Should make calling sequence analogous to ObjOnStaff
and rename it ObjInVoice. */

LINK ObjSelInVoice(LINK pL, short v)
{
	LINK link=NILINK, aNoteL;

	if (ObjHasVoice(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = NoteInVoice(pL, v, True);
				if (aNoteL) return aNoteL;
				return NILINK;
			case GRSYNCtype:
				aNoteL = GRNoteInVoice(pL, v, True);
				if (aNoteL) return aNoteL;
				return NILINK;
			case BEAMSETtype:
				if (BeamVOICE(pL) == v) link = pL; break;
			case TUPLETtype:
				if (TupletVOICE(pL) == v) link = pL; break;
			case GRAPHICtype:
				if (GraphicVOICE(pL) == v) link = pL; break;
			case SLURtype:
				if (SlurVOICE(pL) == v) link = pL; break;
			default:
				return NILINK;
		}
	}
	
	if (link==NILINK)	return NILINK;
	else				return (LinkSEL(link) ? link : NILINK);
}


/* --------------------------------------------------------------- "OnStaff" utilities -- */

/* If staffL has a subobj on staff s, return it. staffL must be a Staff. */

LINK StaffOnStaff(LINK staffL, short s)
{
	LINK aStaffL;
	
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		if (StaffSTAFF(aStaffL)==s) return aStaffL;
	}
	return NILINK;
}			

/* If clefL has a subobject on staff s, return it. clefL must be a Clef. */

LINK ClefOnStaff(LINK clefL, short s)
{
	LINK aClefL;

	aClefL = FirstSubLINK(clefL);
	for ( ; aClefL; aClefL=NextCLEFL(aClefL)) {
		if (ClefSTAFF(aClefL)==s) return aClefL;
  	}
  	return NILINK;
}

/* If ksL has a subobject on staff s, return it. ksL must be a KeySig. */

LINK KeySigOnStaff(LINK ksL, short s)
{
	LINK aKeySigL;

	aKeySigL = FirstSubLINK(ksL);
	for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
		if (KeySigSTAFF(aKeySigL)==s) return aKeySigL;
  	}
  	return NILINK;
}

/* If tsL has a timeSig on staff s, return it. tsL must be a TimeSig. */

LINK TimeSigOnStaff(LINK tsL, short s)
{
	LINK aTimeSigL;

	aTimeSigL = FirstSubLINK(tsL);
	for ( ; aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL)) {
		if (TimeSigSTAFF(aTimeSigL)==s) return aTimeSigL;
  	}
  	return NILINK;
}

/* If measL has a subobject on staff s, return it. measL must be a Measure. */

LINK MeasOnStaff(LINK measL, short s)
{
	LINK aMeasL;

	aMeasL = FirstSubLINK(measL);
	for ( ; aMeasL; aMeasL=NextMEASUREL(aMeasL)) {
		if (MeasureSTAFF(aMeasL)==s) return aMeasL;
  	}
  	return NILINK;
}

/* If pL has a subobj (note or rest) on staff s, return the first one. pL must be a Sync. */

LINK NoteOnStaff(LINK pL, short s)
{
	LINK aNoteL;
	
	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteSTAFF(aNoteL)==s) return aNoteL;
	}
	return NILINK;
}			

/* If pL has a subobj (grace note) on staff s, return the first one. pL must be a GRSync. */

LINK GRNoteOnStaff(LINK pL, short s)
{
	LINK aGRNoteL;
	
	aGRNoteL = FirstSubLINK(pL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		if (GRNoteSTAFF(aGRNoteL)==s) return aGRNoteL;
	}
	return NILINK;
}			

/* --------------------------------------------------------------- "InVoice" utilities -- */
/* If syncL has a subobj (note or rest) in voice v, return the first one. */

LINK NoteInVoice(LINK syncL, short v, Boolean needSel)
{
	LINK aNoteL;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==v && (NoteSEL(aNoteL) || !needSel)) return aNoteL;
	}
	return NILINK;
}			

/* If grSyncL has a subobj (grace note) in voice v, return the first one. */

LINK GRNoteInVoice(LINK grSyncL, short v, Boolean needSel)
{
	LINK aGRNoteL;
	
	aGRNoteL = FirstSubLINK(grSyncL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		if (GRNoteVOICE(aGRNoteL)==v && (GRNoteSEL(aGRNoteL) || !needSel)) return aGRNoteL;
	}
	return NILINK;
}			

/* Return True if pL (which must be a Sync) has a note/rest in the given voice. */

Boolean SyncInVoice(LINK pL, short voice)
{
	return (NoteInVoice(pL, voice, False)!=NILINK);
}			

/* Return True if pL (which must be a GRSync) has a grace note in the given voice. */

Boolean GRSyncInVoice(LINK pL, short voice)
{
	return (GRNoteInVoice(pL, voice, False)!=NILINK);
}			


/* ------------------------------------------------------------------ SyncVoiceOnStaff -- */
/* If the given Sync has one or more subobjects on the given staff, return the
first one's voice number, otherwise return NOONE. */

short SyncVoiceOnStaff(LINK syncL, short staff)
{
	LINK aNoteL;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		if (NoteSTAFF(aNoteL)==staff)
			return NoteVOICE(aNoteL);
	}
	return NOONE;
}

/* -------------------------------------------------------------- SyncInBEAMSET/OTTAVA -- */
/* Is the given Sync or GRSync in the given Beamset? */

Boolean SyncInBEAMSET(LINK syncL, LINK beamSetL)
{
	PANOTEBEAM	pNoteBeam;					/* ptr to current BEAMSET subobject */
	LINK		noteBeamL;

	noteBeamL = FirstSubLINK(beamSetL);
	for ( ; noteBeamL; noteBeamL = NextNOTEBEAML(noteBeamL)) {
		pNoteBeam = GetPANOTEBEAM(noteBeamL);
		if (pNoteBeam->bpSync==syncL) return True;
	}
	return False;
}

/* Is the given Sync or GRSync in the given Ottava? */

Boolean SyncInOTTAVA(LINK syncL, LINK ottavaL)
{
	PANOTEOTTAVA	pNoteOttava;		/* ptr to current OTTAVA subobject */
	LINK			noteOctL;

	noteOctL = FirstSubLINK(ottavaL);
	for (; noteOctL; noteOctL = NextNOTEOTTAVAL(noteOctL)) {
		pNoteOttava = GetPANOTEOTTAVA(noteOctL);
		if (pNoteOttava->opSync==syncL) return True;
	}
	return False;
}


/* ---------------------------------------------------------------------- PrevTiedNote -- */
/* Assuming that the given note in the given Sync is tiedL, find the immediately
preceding note it's tied to. */

Boolean PrevTiedNote(LINK syncL, LINK aNoteL, LINK *pFirstSyncL,
										LINK *pFirstNoteL)
{
	PANOTE aNote; short noteNum, voice;

	aNote = GetPANOTE(aNoteL);
	noteNum = aNote->noteNum;
	voice = aNote->voice;
	*pFirstSyncL = LVSearch(LeftLINK(syncL),						/* Tied note should always exist */
									SYNCtype, voice, GO_LEFT, False);
	*pFirstNoteL = NoteNum2Note(*pFirstSyncL, voice, noteNum);
	return True;
}


/* --------------------------------------------------------------------- FirstTiedNote -- */
/* Find the first in a series of tied notes. If the given note in the given Sync is
tiedL, follow the chain of ties backward and return the first note and Sync of the
series; if the given note isn't tiedL, return it unchanged. */

Boolean FirstTiedNote(LINK syncL, LINK aNoteL,							/* Starting Sync/note */
							LINK *pFirstSyncL, LINK *pFirstNoteL)		/* First Sync/note */
{
	short noteNum, voice;
	LINK continL, continNoteL;
	
	noteNum = NoteNUM(aNoteL);
	voice = NoteVOICE(aNoteL);
	continL = syncL;
	continNoteL = aNoteL;
	
	while ((GetPANOTE(continNoteL))->tiedL) {
		continL = LVSearch(LeftLINK(continL),						/* Tied note should always exist */
						SYNCtype, voice, GO_LEFT, False);
		continNoteL = NoteNum2Note(continL, voice, noteNum);
		if (continNoteL==NILINK) {
			MayErrMsg("FirstTiedNote: can't find tied note to left for Sync L%ld note L%ld",
						(long)syncL, (long)aNoteL);
			return False;											/* Should never happen */
		}
	}

	*pFirstSyncL = continL;
	*pFirstNoteL = continNoteL;
	return True;
}


/* ----------------------------------------------------------------------- ChordNextNR -- */

/* Given a Sync and a note/rest in it, return the next subobject in the Sync in that
 note/rest's voice, if there is one; else return NILINK. */

LINK ChordNextNR(LINK syncL, LINK theNoteL)
{
	short voice;  LINK aNoteL;
	Boolean foundTheNote=False;
	
	voice = NoteVOICE(theNoteL);
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (aNoteL==theNoteL)
			foundTheNote = True;
		else
			if (foundTheNote && NoteVOICE(aNoteL)==voice) return aNoteL;
	}

	return NILINK;
}


/* --------------------------------------------------------------------- GetCrossStaff -- */
/* If the given notes are all on the same staff, return False; else fill in the
stfRange and return True. In the latter case, we assume the notes are on only
two different staves: we don't check this, nor that the staves are consecutive. */

Boolean GetCrossStaff(short nElem, LINK notes[], STFRANGE *stfRange)
{
	short i, firstStaff, stf;  STFRANGE theRange;
	Boolean crossStaff=False;
	
	firstStaff = NoteSTAFF(notes[0]);
	theRange.topStaff = theRange.bottomStaff = firstStaff;
	for (i=0; i<nElem; i++)
		if ((stf = NoteSTAFF(notes[i]))!=firstStaff) {
			crossStaff = True;
			if (stf<theRange.topStaff) theRange.topStaff = stf;
			else if (stf>theRange.bottomStaff) theRange.bottomStaff = stf;
		}
	*stfRange = theRange;
	return crossStaff;
}


/* ----------------------------------------- InitialClefExists, BFKeySig/TimeSigExists -- */ 

/* Return True if any subobjects of the initial clef <clefL> are visible. */

Boolean InitialClefExists(LINK clefL)
{
	LINK aClefL;

	aClefL = FirstSubLINK(clefL);
	for ( ; aClefL; aClefL=NextCLEFL(aClefL))
		if (ClefVIS(aClefL))
			return True;
			
	return False;
}


/* Return True if any subobjects of the initial keysig <keySigL> are visible. */

Boolean BFKeySigExists(LINK keySigL)
{
	LINK aKeySigL;

	aKeySigL = FirstSubLINK(keySigL);
	for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL))
		if (KeySigVIS(aKeySigL))
			return True;

	return False;
}


/* Return True if any subobjects of the initial timesig <timeSigL> are visible. We
know timeSigL is beforeFirst, and wish to know if it has been made non-existent
??HUH? through invisification. */

Boolean BFTimeSigExists(LINK timeSigL)
{
	LINK aTimeSigL;

	aTimeSigL = FirstSubLINK(timeSigL);
	for ( ; aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL))
		if (TimeSigVIS(aTimeSigL))
			return True;
			
	return False;
}


/* ---------------------------------------------------------------------- SetTempFlags -- */
/* Set the tempFlags for all notes and GRNotes in range [startL, endL). */

void SetTempFlags(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL,
						Boolean value)
{
	LINK aNoteL, pL, aGRNoteL;
	PANOTE aNote;  PAGRNOTE aGRNote;
	
	InstallDoc(fixDoc);
	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				aNote->tempFlag = value;
			}
		}
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL);
				aGRNote->tempFlag = value;
			}
		}
	}
	InstallDoc(oldDoc);
}


/* --------------------------------------------------------------------- SetSpareFlags -- */
/* Set the spareFlags for all objects in range [startL, endL). */

void SetSpareFlags(LINK startL, LINK endL, Boolean value)
{
	LINK pL;

	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		LinkSPAREFLAG(pL) = value;
}


/* -------------------------------------------------------------------- IsXXMultiVoice -- */

/* Determine if syncL has notes or rests in multiple voices on <staff>. */

Boolean IsSyncMultiVoice(LINK syncL, short staff)
{
	LINK aNoteL;
	Boolean multiVoice=False;
	short lastVoice = -1;

	if (ObjLType(syncL) == SYNCtype) {
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			if (NoteSTAFF(aNoteL)==staff) {
				if (lastVoice!=NoteVOICE(aNoteL)) {
					if (lastVoice>0) {
						multiVoice = True;
						break;
					}
				}
				lastVoice = NoteVOICE(aNoteL);
			}
		}
	}

	return multiVoice;
}

/* Should 2-voice notation be used for the notes in the given voice and staff of syncL?
The proper musical criteria are ill-defined. We return True, i.e., use 2-voice notation:
(1) if note(s) in syncL in the given voice are beamed, and any of the notes/chords in
that beam are in Syncs with multiple voices on <staff>.
If they're not beamed, use 2-voice notation:
(2) if syncL or either of the adjacent Syncs on <staff> have multiple voices on <staff>, or
(3) if _both_ adjacent Syncs on <staff> involving <voice> have multiple voices. */

Boolean IsNeighborhoodMultiVoice(LINK syncL, short staff, short voice)
{
	LINK aNoteL, beamL, aNoteBeamL, pL, lSyncL, rSyncL;
	Boolean multiVoice;
	short i;
	
	if (ObjLType(syncL)!=SYNCtype) return False;
	
	aNoteL = FindMainNote(syncL, voice);
	if (NoteBEAMED(aNoteL)) {
		/* Criterion #1 */
		beamL = LVSearch(syncL, BEAMSETtype, voice, True, False);
		aNoteBeamL = FirstSubLINK(beamL);
		for (i=0; aNoteBeamL; i++, aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
			pL = NoteBeamBPSYNC(aNoteBeamL);
			multiVoice = IsSyncMultiVoice(pL, staff);
			if (multiVoice) return True;
		}
	}
	else {
		/* Criterion #2 */
//LogPrintf(LOG_DEBUG, "IsNeighborhoodMultiVoice #2: syncL=%u staff=%d voice=%d\n",
//			syncL, staff, voice);
		if (IsSyncMultiVoice(syncL, staff)) return True;
		lSyncL = LSSearch(LeftLINK(syncL), SYNCtype, staff, GO_LEFT, False);
		if (lSyncL && IsSyncMultiVoice(lSyncL, staff)) return True;
		rSyncL = LSSearch(RightLINK(syncL), SYNCtype, staff, GO_RIGHT, False);
		if (rSyncL && IsSyncMultiVoice(rSyncL, staff)) return True;		

		/* Criterion #3 */
//LogPrintf(LOG_DEBUG, "IsNeighborhoodMultiVoice #3: syncL=%u staff=%d voice=%d\n",
//			syncL, staff, voice);
		lSyncL = LVSearch(LeftLINK(syncL), SYNCtype, voice, GO_LEFT, False);
		if (lSyncL && IsSyncMultiVoice(lSyncL, staff)) return True;
		rSyncL = LVSearch(RightLINK(syncL), SYNCtype, voice, GO_RIGHT, False);
		if (rSyncL && IsSyncMultiVoice(rSyncL, staff)) return True;		
	}

	return False;
}

	
/* ----------------------------------------------------------------- GetSelectionStaff -- */
/* If all selected notes/rests are on one staff, returns that staff, else returns NOONE.
Ignores other selected objects. */

short GetSelectionStaff(Document *doc)
{
	LINK pL, aNoteL;
	short firstStaff=NOONE;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL)) {
					if (firstStaff == NOONE)
						firstStaff = NoteSTAFF(aNoteL);
					else if (NoteSTAFF(aNoteL)!=firstStaff)
						return NOONE;
				}
		}
	return firstStaff;
}


/* --------------------------------------------- IsSelInTuplet, -InTupletNotTotallySel -- */

/* Return True if any selected notes (or rests) are in tuplets. */

Boolean IsSelInTuplet(Document *doc)
{
	LINK pL, aNoteL;

	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteINTUPLET(aNoteL) && NoteSEL(aNoteL))
					return True;
	
	return False;			
}


/* Return True if any selected notes (or rests) are in a tuplet, but not all the notes of
the tuplet are selected. */

Boolean IsSelInTupletNotAllSel(Document *doc)
{
	LINK pL, aNoteL, voice, aTupletL, tpSyncL;
	short numSelNotes, numNotSelNotes;

	pL = LSSearch(doc->selStartL, MEASUREtype, ANYONE, GO_LEFT, False);
	if (pL==NILINK)
		pL = doc->selStartL;

	for ( ; pL!=doc->selEndL; pL=RightLINK(pL))
		if (TupletTYPE(pL)) {
			voice = TupletVOICE(pL);
			numSelNotes = numNotSelNotes = 0;
			aTupletL = FirstSubLINK(pL);
			for ( ; aTupletL; aTupletL=NextNOTETUPLEL(aTupletL)) {
				tpSyncL = NoteTupleTPSYNC(aTupletL);
				aNoteL = FirstSubLINK(tpSyncL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteVOICE(aNoteL)==voice) {
						if (NoteSEL(aNoteL))	numSelNotes++;
						else					numNotSelNotes++;
					}
			}
			if (numSelNotes>0 && numNotSelNotes>0)
				return True;
		}

	return False;			
}


/* --------------------------------------------------------------------- TweakSubRects -- */
/* Fine-tune vertical positions of subObj rects of notes/rests that need it:
breve notes and short rests. */

static SignedByte S_restOffset[MAX_L_DUR+1] =	{ 0, 0, 0, 0, 0, -4, 4, 4, 12, 12 };
static SignedByte S_noteOffset[MAX_L_DUR+1] =	{ 0, 8, 0, 0, 0, 0, 0, 0, 0, 0 };

void TweakSubRects(Rect *r, LINK aNoteL, CONTEXT *pContext)
{
	STDIST stOffset;
	PANOTE aNote;
	
	aNote = GetPANOTE(aNoteL);
	if (aNote->subType<0)
		stOffset = 0;
	else if (aNote->rest)
		stOffset = (STDIST)S_restOffset[aNote->subType];
	else
		stOffset = (STDIST)S_noteOffset[aNote->subType];
	
	if (stOffset!=0)
		OffsetRect(r, 0, 
			d2p(std2d(stOffset, pContext->staffHeight, pContext->staffLines)));
}

/* ---------------------------------------------------------------- CompareScoreFormat -- */
/* Return True if doc1,doc2 have identical part-staff formats, else return False. */

Boolean CompareScoreFormat(Document *doc1,
							Document *doc2,
							short /*type*/)			/* unused */
{
	LINK head1,head2,part1,part2; PPARTINFO pPart1,pPart2;

	head1 = doc1->headL;
	head2 = doc2->headL;

	part1 = DFirstSubLINK(doc1,head1);
	part2 = DFirstSubLINK(doc2,head2);

	/* Return False if any of the parts start or end with different staves. */

	for ( ; part1 && part2;
			part1=DNextPARTINFOL(doc1,part1),part2=DNextPARTINFOL(doc2,part2)) {
		pPart1 = DGetPPARTINFO(doc1,part1);
		pPart2 = DGetPPARTINFO(doc2,part2);
		if ( (pPart1->firstStaff!=pPart2->firstStaff) ||
				(pPart1->lastStaff!=pPart2->lastStaff) )
			return False;
	}

	/* Return False if one of the document formats has more parts than the
		other. */
	if (part1 || part2) return False;

	return True;
}


/* --------------------------------------------------------------------- DisposeMODNRs -- */
/* Dispose of all MODNRs in the range [startL,endL). */

void DisposeMODNRs(LINK startL, LINK endL)
{
	LINK aNoteL,pL; PANOTE aNote;

	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->firstMod) {
					HeapFree(MODNRheap,aNote->firstMod);			/* Empty list is legal */
					aNote = GetPANOTE(aNoteL);
					aNote->firstMod = NILINK;
				}
			}
		}
}


/* ---------------------------------------------------- Staff2PartL, PartL2Partn, etc. -- */

/* Return the LINK to the part containing staff <stf> in the object list
starting at headL. */

LINK Staff2PartL(Document */*doc*/, LINK headL, short stf)
{
	LINK partL;

	partL = FirstSubLINK(headL);
	for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL))
		if (PartFirstSTAFF(partL) <= stf && PartLastSTAFF(partL) >= stf)
			return partL;
	
	return NILINK;	
}


short PartL2Partn(Document *doc, LINK partL)
{
	LINK pL; short partn;
	
	pL = FirstSubLINK(doc->headL);
	for (partn = 1, pL = NextPARTINFOL(pL); pL; partn++, pL = NextPARTINFOL(pL))
		if (pL==partL) return partn;

	return NOONE;
}

LINK Partn2PartL(Document *doc, short partn)
{
	LINK partL; short n;

	partL = FirstSubLINK(doc->headL);
	for (n = 1, partL = NextPARTINFOL(partL); n<=partn && partL;
			n++, partL = NextPARTINFOL(partL)) {
		if (n==partn) return partL;
	}
	
	return NILINK;	
}


/* ------------------------------------------------------------ HasXXXAcross utilities -- */

/* Determines if there is a tie-subtype Slur in <voice> across the point just before
<link>.  If so, returns the LINK to the Slur, else NILINK. */

LINK VHasTieAcross(LINK link, short voice)	
{
	LINK lSyncL, rSyncL, lNoteL, rNoteL, slurL;
	
	if (link==NILINK) return NILINK;	

	/* Get the Syncs in <voice> immediately to the right and left of the link. */

	lSyncL = LVSearch(LeftLINK(link), SYNCtype, voice, GO_LEFT, False);
	rSyncL = LVSearch(link, SYNCtype, voice, GO_RIGHT, False);
	
	if (lSyncL==NILINK || rSyncL==NILINK) return NILINK;

	/* LVSearch in voice <voice> guarantees that if both Syncs exists, both have
		main notes in voice <voice>. */

	lNoteL = FindMainNote(lSyncL,voice);
	if (!NoteTIEDR(lNoteL)) return NILINK;

	rNoteL = FindMainNote(rSyncL,voice);
	if (!NoteTIEDL(rNoteL)) return NILINK;

	/* Left and right notes are both tied. Search for the right one's tie. If it also
	   belongs to the left Sync, return it, otherwise return NILINK. */

	slurL = LeftSlurSearch(rSyncL, voice, True);
	if (SlurFIRSTSYNC(slurL)==lSyncL) 
		return slurL;

	return NILINK;	
}


/* If any "extended object" has a range that crosses the point just before the given
link, return True and a message string. Intended for use before recording, so it ignores
slurs and considers ties only in the default voice for doc->selStaff. */

Boolean HasSmthgAcross(
				Document *doc,
				LINK link,
				char *str)		/* user-friendly string describing the problem voice or staff */
{
	short voice, staff, number;
	Boolean isVoice, foundSmthg=False;
	
	/* Slurs are never a problem, and ties in voices other than the one we're recording
	   into aren't a problem. */
		
	voice = USEVOICE(doc, doc->selStaff);
	if (VHasTieAcross(link, voice)) {
		isVoice = True;
		number = voice;
		foundSmthg = True;
		goto Finish;
	}

	for (staff = 1; staff<=doc->nstaves && !foundSmthg; staff++) {
		if (HasBeamAcross(link, staff)
		||  HasTupleAcross(link, staff)
		||  HasOttavaAcross(link, staff) ) {
			isVoice = False;
			number = staff;
			foundSmthg = True;
			goto Finish;
		}
	}

Finish:
	if (foundSmthg) {
		if (isVoice)	Voice2UserStr(doc, number, str);
		else			Staff2UserStr(doc, number, str);
	}

	return foundSmthg;
}


/* ----------------------------------------------------------------- Rastral utilities -- */

/* Given interline space in DDIST, return the rastral of a staff that would have
that interline space, or return -1 if error.  -JGG, 7/31/00 */

short LineSpace2Rastral(DDIST lnSpace)
{
	short	i;
	DDIST	stfHeight;		/* for a 5-line staff */

	stfHeight = lnSpace*(STFLINES-1);

	for (i = 0; i<=MAXRASTRAL; i++) {
		if (drSize[i]==stfHeight)		/* staff height of a 5-line staff */
			return i;
	}
	return -1;		/* If this happens, caller has problems. */
}


/* Given a rastral, return interline space in DDIST.  -JGG, 7/31/00 */

DDIST Rastral2LineSpace(short rastral)
{
	return (drSize[rastral]/(STFLINES-1));
}


/* Given a staff subobject, return its rastral, or -1 if error.  -JGG, 7/31/01 */

short StaffRastral(LINK aStaffL)
{
	short	staffLines;
	DDIST	staffHeight, lnSpace;

	staffHeight = StaffHEIGHT(aStaffL);
	staffLines = StaffSTAFFLINES(aStaffL);
	lnSpace = staffHeight/(staffLines-1);
	return LineSpace2Rastral(lnSpace);
}
