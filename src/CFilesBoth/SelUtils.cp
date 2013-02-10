/***************************************************************************
*	FILE:	SelUtils.c																			*
*	PROJ:	Nightingale, rev. for v.3.1													*
*	DESC:	Routines to handle the user interface for selection, get info
	about the selection, etc.
	GetSelStaff				GetStaffFromSel		GetSelPart
	GetVoiceFromSel		GetStfRangeOfSel		Sel2MeasPage
	GetSelMIDIRange		FindSelAcc
	ShellSort				UnemptyRect
	GetStaff					TrackStaffRect			ChangeInvRect
	FixEmptySelection		GetStaffLimits			SelectStaffRect
	DoThreadSelect			InsertSpaceTrackStf
	NotesSel2TempFlags	TempFlags2NotesSel
/***************************************************************************/

/*										NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-97 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

typedef struct {
	unsigned short	visible:1;
	unsigned short	top:15;
} STAFFINFO;

static void ShellSort(short [], short);
static short GetStaff(Document *, Point);
static short GetNextStaffn(Document *, short, Boolean);
static Point TrackStaffRect(Document *, Point, short *, short *, Rect *);
static void ChangeInvRect(Rect *, Rect *);
static void GetStaffLimits(Document *, Point, STAFFINFO [], CONTEXT []);
static void StartThread(void);


/* ------------------------------------------------------------------ GetSelStaff -- */
/* Get the staff number of the insertion point or selection. If anything is selected,
if the first selected object or subobject actually has a staff number (almost always
the case unless it's a page-relative Graphic) return that number; if it doesn't,
return NOONE. */

short GetSelStaff(Document *doc)
{
	LINK pL;
	
	if (doc->selStartL==doc->selEndL)
		return doc->selStaff;
	else
		return GetStaffFromSel(doc, &pL);
}


/* ------------------------------------------------------------- GetStaffFromSel -- */
/* Get staff number from selection. If the first selected object or subobject
actually has a staff number (almost always the case unless it's a page-relative
Graphic) return that number; if it doesn't or if nothing is selected, return
NOONE. Also return in <pSelL> the first selected link (normally doc->selStartL). */

short GetStaffFromSel(Document *doc, LINK *pSelL)
{
	register LINK pL;
	LINK aNoteL, aGRNoteL, aKeySigL, aTimeSigL, aClefL, aDynamicL, aRptEndL, 
			aMeasureL, aPSMeasL;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL)) {
			switch (ObjLType(pL)) {
			case SYNCtype:
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteSEL(aNoteL))
						{ *pSelL = pL; return NoteSTAFF(aNoteL); }
				break;
			case GRSYNCtype:
				for (aGRNoteL = FirstSubLINK(pL); aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
					if (GRNoteSEL(aGRNoteL))
						{ *pSelL = pL; return GRNoteSTAFF(aGRNoteL); }
				break;
			case BEAMSETtype:
				*pSelL = pL;
				return BeamSTAFF(pL);
			case SLURtype:
				*pSelL = pL;
				return SlurSTAFF(pL);
			case CLEFtype:
				for (aClefL = FirstSubLINK(pL); aClefL; aClefL = NextCLEFL(aClefL))
					if (ClefSEL(aClefL))
						{ *pSelL = pL; return ClefSTAFF(aClefL); }
				break;
			case KEYSIGtype:
				aKeySigL = FirstSubLINK(pL);
				for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
					if (KeySigSEL(aKeySigL))
						{ *pSelL = pL; return KeySigSTAFF(aKeySigL); }
				break;
			case TIMESIGtype:
				aTimeSigL = FirstSubLINK(pL);
				for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
					if (TimeSigSEL(aTimeSigL))
						{ *pSelL = pL; return TimeSigSTAFF(aTimeSigL); }
				break;
			case DYNAMtype:
				aDynamicL = FirstSubLINK(pL);
				if (DynamicSEL(aDynamicL))
					{ *pSelL = pL; return DynamicSTAFF(aDynamicL); }
				break;
			case OCTAVAtype:
				*pSelL = pL;
				return OctavaSTAFF(pL);
			case TUPLETtype:
				*pSelL = pL;
				return TupletSTAFF(pL);
			case GRAPHICtype:
				if (GraphicSTAFF(pL)!=NOONE)
					{ *pSelL = pL; return GraphicSTAFF(pL); }
				break;
			case TEMPOtype:
				*pSelL = pL;
				return TempoSTAFF(pL);
			case SPACEtype:
				*pSelL = pL;
				return SpaceSTAFF(pL);
			case ENDINGtype:
				*pSelL = pL;
				return EndingSTAFF(pL);
			case RPTENDtype:
				aRptEndL = FirstSubLINK(pL);
				for ( ; aRptEndL; aRptEndL = NextRPTENDL(aRptEndL)) {
					if RptEndSEL(aRptEndL)
						{  *pSelL = pL; return RptEndSTAFF(aRptEndL); }
				}
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
					if (MeasureSEL(aMeasureL))
						{ *pSelL = pL; return MeasureSTAFF(aMeasureL); }
				}
				break;
			case PSMEAStype:
				aPSMeasL = FirstSubLINK(pL);
				for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL))
					if (PSMeasSEL(aPSMeasL))
						{ *pSelL = pL; return PSMeasSTAFF(aPSMeasL); }
				break;
			case MODNRtype:
				MayErrMsg("GetStaffFromSel: found MODNR at %ld", pL);
				break;
			default:
				;
			}
			break;
		}
	}
	
	*pSelL = NILINK;
	return NOONE;
}


/* --------------------------------------------------------------- GetSelPartList -- */
/*	Determine which parts contain selected items. Deliver an array, <partL>, of
links to these parts. <partL> is an array of MAXSTAVES+1, dimensioned by caller.
Caller should walk array until finding a NILINK element, which represents the
end of the list of links.
Note: This code is derived from GetStfRangeOfSel.  -JGG */

void GetSelPartList(Document *doc, LINK partL[])
{
	short			i, j;
	short			selStaves[MAXSTAVES+1];
	short			selParts[MAXSTAVES+1];
	PMEVENT		p;
	LINK 			pL, subObjL;
	HEAP			*tmpHeap;
	GenSubObj	*subObj;
	
	for (i = 0; i<=MAXSTAVES; i++) {
		selStaves[i] = FALSE;
		selParts[i] = FALSE;
		partL[i] = NILINK;
	}

	/* Determine which staves have any selected items. */

	if (doc->selStartL==doc->selEndL) {					/* insertion pt */
		selStaves[doc->selStaff] = TRUE;
	}
	else {
		for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
			if (LinkSEL(pL)) {
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
						tmpHeap = Heap + ObjLType(pL);
						
						for (subObjL = FirstSubObjPtr(p, pL); subObjL;
																	subObjL = NextLink(tmpHeap, subObjL)) {
							subObj = (GenSubObj *)LinkToPtr(tmpHeap, subObjL);
							if (subObj->selected)
								selStaves[subObj->staffn] = TRUE;
						}
						break;
					case BEAMSETtype:
					case TUPLETtype:
					case SLURtype:
					case GRAPHICtype:
					case OCTAVAtype:
					case TEMPOtype:
					case SPACEtype:
					case ENDINGtype:
						p = GetPMEVENT(pL);
						
						if (LinkSEL(pL)) {
							short staffn = ((PEXTEND)p)->staffn;
							selStaves[staffn] = TRUE;
						}
						break;
					default:
						if (TYPE_BAD(pL))
							MayErrMsg("GetSelPartList: object at %ld has illegal type %ld",
										(long)pL, (long)ObjLType(pL));
						break;
				}
			}
		}
	}

	/* Gather the part links whose staves have selected items. (More
		than one staff number can point to the same part.) */

	for (i = 1; i<=MAXSTAVES; i++)
		if (selStaves[i]) {
			j = Staff2Part(doc, i);
			selParts[j] = TRUE;
		}

	for (i = 1, j = 0; i<=MAXSTAVES; i++)
		if (selParts[i])
			partL[j++] = FindPartInfo(doc, i);
}


/* -------------------------------------------------------------------- IsSelPart -- */
/* Does <partL> refer to a part that contains selected items? Before using this
function, call GetSelPartList to build the <partList> array.  -JGG */

Boolean IsSelPart(LINK partL, LINK partList[])
{
	short	i;

	for (i = 0; i<=MAXSTAVES; i++) {
		if (partList[i]==NILINK)		/* end of list */
			break;
		else if (partList[i]==partL)
			return TRUE;
	}
	return FALSE;
}


/* ---------------------------------------------------------------- CountSelParts -- */
/* How many parts contain selected items? Before using this function, call
GetSelPartList to build the <partList> array.  -JGG */

short CountSelParts(LINK partList[])
{
	short	i;

	for (i = 0; i<=MAXSTAVES; i++) {
		if (partList[i]==NILINK)		/* end of list */
			break;
	}
	return i;
}


/* ------------------------------------------------------------------ GetSelPart -- */
/* Get part LINK from the insertion point or selection. */

LINK GetSelPart(Document *doc)
{
	short selStaff; LINK partL;
	
	selStaff = GetSelStaff(doc);
	if (selStaff==NOONE) selStaff = 1;
	partL = FindPartInfo(doc, Staff2Part(doc, selStaff));
		
	return partL;
}


/* ------------------------------------------------------------  GetVoiceFromSel -- */
/* Get (internal) voice number from selection. If the first selected object or
subobject actually has a voice number (see ObjHasVoice for the current list of
types) return that number; otherwise return its staff number (since that's the
default voice number for the staff). If nothing is selected, returns NOONE. */

short GetVoiceFromSel(Document *doc)
{
	register LINK pL;
	LINK			aNoteL, aGRNoteL, aClefL, aKeySigL, aTimeSigL, aDynamicL,
					aRptEndL, aMeasureL, aPSMeasL;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (!LinkSEL(pL)) continue;
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteSEL(aNoteL)) return NoteVOICE(aNoteL);
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
					if (GRNoteSEL(aGRNoteL)) return GRNoteVOICE(aGRNoteL);
				break;
			case BEAMSETtype:
				return BeamVOICE(pL);
			case SLURtype:
				return SlurVOICE(pL);
			case CLEFtype:
				aClefL = FirstSubLINK(pL);
				for ( ; aClefL; aClefL = NextCLEFL(aClefL))
					if (ClefSEL(aClefL)) return ClefSTAFF(aClefL);
				break;
			case KEYSIGtype:
				aKeySigL = FirstSubLINK(pL);
				for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
					if (KeySigSEL(aKeySigL)) return KeySigSTAFF(aKeySigL);
				break;				
			case TIMESIGtype:
				aTimeSigL = FirstSubLINK(pL);
				for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
					if (TimeSigSEL(aTimeSigL)) return TimeSigSTAFF(aTimeSigL);
				break;
			case DYNAMtype:
				aDynamicL = FirstSubLINK(pL);
				for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL))
					if (DynamicSEL(aDynamicL)) return DynamicSTAFF(aDynamicL);
				break;
			case OCTAVAtype:
				return OctavaSTAFF(pL);
			case TUPLETtype:
				return TupletVOICE(pL);
			case GRAPHICtype:
				if (GraphicVOICE(pL)!=NOONE)
					return GraphicVOICE(pL);
				else if (GraphicSTAFF(pL)!=NOONE)
					return GraphicSTAFF(pL);
				else
					return 1;							/* Page-rel. Graphic, so be arbitrary */
			case TEMPOtype:
				return TempoSTAFF(pL);
			case SPACEtype:
				return SpaceSTAFF(pL);
			case ENDINGtype:
				return EndingSTAFF(pL);
			case RPTENDtype:
				aRptEndL = FirstSubLINK(pL);
				for ( ; aRptEndL; aRptEndL = NextRPTENDL(aRptEndL))
					if (RptEndSEL(aRptEndL)) return RptEndSTAFF(aRptEndL);
				break;
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL))
					if (MeasureSEL(aMeasureL)) return MeasureSTAFF(aMeasureL);
				break;
			case PSMEAStype:
				aPSMeasL = FirstSubLINK(pL);
				for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL))
					if (PSMeasSEL(aPSMeasL)) return PSMeasSTAFF(aPSMeasL);
				break;
			case MODNRtype:
				MayErrMsg("GetVoiceFromSel: found MODNR at %ld", pL);
				break;
			default:
				;
		}
	}
	return NOONE;										/* Nothing was found selected */
}


/* ------------------------------------------------------------- GetStfRangeOfSel -- */
/*	Return the minimum range of staves that includes everything selected. Fills in a
STFRANGE struct allocated by caller. Depends on validity of doc->selStartL and
doc->selEndL. */

void GetStfRangeOfSel(Document *doc, STFRANGE *stfRange)
{
	PMEVENT		p;
	LINK 			pL, subObjL;
	HEAP			*tmpHeap;
	GenSubObj	*subObj;
	short			topStaffn, botStaffn;
	
	if (doc->selStartL==doc->selEndL) {					/* insertion pt */
		stfRange->topStaff = stfRange->bottomStaff = doc->selStaff;
		return;
	}
		
	topStaffn = SHRT_MAX;	botStaffn = 0;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL)) {
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
						if (subObj->selected) {
							if (subObj->staffn<topStaffn)
								topStaffn = subObj->staffn;
							if (subObj->staffn>botStaffn)
								botStaffn = subObj->staffn;
						}
					}
					break;
				case BEAMSETtype:
				case TUPLETtype:
				case SLURtype:
				case GRAPHICtype:
				case OCTAVAtype:
				case TEMPOtype:
				case SPACEtype:
				case ENDINGtype:
					p = GetPMEVENT(pL);
					
					if (LinkSEL(pL)) {
						if (((PEXTEND)p)->staffn<topStaffn)
							topStaffn = ((PEXTEND)p)->staffn;
						if (((PEXTEND)p)->staffn>botStaffn)
							botStaffn = ((PEXTEND)p)->staffn;
					}
					break;
				default:
					if (TYPE_BAD(pL))
						MayErrMsg("GetStfRangeOfSel: object at %ld has illegal type %ld",
									(long)pL, (long)ObjLType(pL));
					break;
			}
		}
	}
	stfRange->topStaff = topStaffn;
	stfRange->bottomStaff = botStaffn;
}


/* ---------------------------------------------------------------- Sel2MeasPage -- */
/* Get the measure number and page number of the start of the selection in the given
doc. */

void Sel2MeasPage(Document *doc, short *pMeasNum, short *pPageNum)
{
	LINK measL, aMeasureL; PAMEASURE aMeasure;
	
	*pMeasNum = 0;
	*pPageNum = doc->firstPageNumber;

	if (doc->selStartL==doc->headL) return;

	/*
	 * The following is NOT equivalent to just searching left for a Measure
	 * from doc->selStartL: it's different if selStartL==selEndL==a Measure.
	 */
	if (doc->selStartL!=doc->selEndL && MeasureTYPE(doc->selStartL))
		measL = doc->selStartL;
	else
		measL = LSSearch(LeftLINK(doc->selStartL), MEASUREtype, ANYONE,
							GO_LEFT, FALSE);
	if (!measL) return;

	if (BeforeFirstMeas(doc->selStartL)) measL = LinkRMEAS(measL);

	aMeasureL = FirstSubLINK(measL);
	aMeasure = GetPAMEASURE(aMeasureL);
	*pMeasNum = aMeasure->measureNum+doc->firstMNNumber;
	*pPageNum = SheetNUM(MeasPAGE(measL)) + doc->firstPageNumber;
}


/* ------------------------------------------------------------- GetSelMIDIRange -- */
/* Return the lowest and highest MIDI note numbers in selected notes in the given
score. */

void GetSelMIDIRange(Document *doc, short *pLow, short *pHi)
{
	LINK		pL, aNoteL;
	PANOTE	aNote;
	short		low, hi;
	
	low = MAX_NOTENUM;
	hi = 0;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (SyncTYPE(pL) && LinkSEL(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && !NoteREST(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->noteNum<low) low = aNote->noteNum;
					if (aNote->noteNum>hi) hi = aNote->noteNum;
				}
		}
	*pLow = low;
	*pHi = hi;
}


/* ------------------------------------------------------------------ FindSelAcc -- */
/* Look for a selected note or grace note with the given accidental. If it's found,
return it, else return NILINK.*/

LINK FindSelAcc(Document *doc, short acc)
{
	LINK pL, aNoteL, aGRNoteL;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL)) {
			switch (ObjLType(pL)) {
				case SYNCtype:
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteSEL(aNoteL) && !NoteREST(aNoteL))
							if (NoteACC(aNoteL)==acc) return aNoteL;
					break;
				case GRSYNCtype:
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
						if (GRNoteSEL(aGRNoteL))
							if (GRNoteACC(aGRNoteL)==acc) return aGRNoteL;
					break;
				default:
					;
			}
		}

	return NILINK;
}


/* ------------------------------------------------------------------- ShellSort -- */
/* ShellSort does a Shell (diminishing increment) sort on the given array,
putting it into ascending order.  The increments we use are powers of 2,
which does not give the fastest possible execution, though the difference
should be negligible for a few hundred elements or less. See Knuth, The Art
of Computer Programming, vol. 2, pp. 84-95. */

static void ShellSort(short array[], short nsize)
{
	short nstep, ncheck, i, n, temp;
	
	for (nstep = nsize/2; nstep>=1; nstep = nstep/2)
	{
/* Sort <nstep> subarrays by simple insertion */
		for (i = 0; i<nstep; i++)
		{
			for (n = i+nstep; n<nsize; n += nstep) {			/* Look for item <n>'s place */
				temp = array[n];										/* Save it */
				for (ncheck = n-nstep; ncheck>=0;
					  ncheck -= nstep)
					if (temp<array[ncheck])
						array[ncheck+nstep] = array[ncheck];
					else {
						array[ncheck+nstep] = temp;
						break;
					}
					array[ncheck+nstep] = temp;
			}
		}
	}
}


/* ------------------------------------- Help Functions, etc. for TrackStaffRect -- */

STAFFINFO staffInfo[MAXSTAVES+1+1];			/* Need 1 extra fake "staff" at bottom! */

#define SWAP(a, b)	{	short temp; temp = (a); (a) = (b); (b) = temp; }

/* Make rect non-empty, if possible, by swapping boundaries */

void UnemptyRect(register Rect *r)
{
	if (r->right<r->left) SWAP(r->right, r->left);
	if (r->bottom<r->top) SWAP(r->bottom, r->top);
}


/* ----------------------------------------------------- GetStaff, GetNextStaffn -- */
/* Functions which find the staffn of the next visible staff, in relation to
mousePt or staffn, using staff location and visibility as contained in the
staffInfo array. */
 
/* Return the staff number of the staff containing the mousePt pt. Looks for the
topmost visible staffTop at or below pt. If it finds one, it returns the first
visible staff above the one it found, if there is a visible staff above the one
it found; otherwise it returns the one it found. Otherwise, it returns the
bottommost visible staff. */

static short GetStaff(Document *doc, Point pt)
{
	short i, j;
				
	for (i = 1; i<doc->nstaves; i++)
		if (staffInfo[i+1].visible && pt.v<=staffInfo[i+1].top) {
			for (j = i; j>0; j--)
				if (staffInfo[j].visible) return j;
			MayErrMsg("GetStaff: no visible staff at or above mouse.");
		}

	for (i = doc->nstaves; i>=1; i--)
		if (staffInfo[i].visible) return i;

	return 1;														/* Should never get here */
}


/* Return the staff number of the next visible staff subobject in staffInfo, starting
from staff no. <base>, going in direction <up>. Does not consider <base> itself unless
it fails to find a visible staff, in which case it returns <base>. Notice that this is
wrong if staff <base> is invisible. <up> means in direction of increasing staff no.,
i.e., downward on the page. */

static short GetNextStaffn(Document *doc, short base, Boolean up)
{
	short i;
	
	if (up) {
		for (i = base+1; i<=doc->nstaves+1; i++)		/* Consider fake staff at bottom */
			if (staffInfo[i].visible) return i;
		MayErrMsg("GetNextStaff: didn't find it.");
		return 1;
	}
	else {
		for (i = base-1; i>0; i--)
			if (staffInfo[i].visible) return i;
		return base;
	}
}


/* -------------------------------------------------------------- TrackStaffRect -- */
/* Track mouse motion and give visual feedback by inverting a rectangle in the
same way most Mac word processors do, but on one or more staves instead of one
or more lines. Vertical positions are quantized to the staff level, inclusive;
horizontal positions are taken at face value.  If the mouse is never moved more
than 1 pixel horizontally, TrackStaffRect signals by returning pt.h equal to
CANCEL_INT. Designed for "wipe" selection, e.g., selecting everything on one or
more staves in an arbitrary horizontal extent. */

/* For any page, wipe selections on more than one system will involve inverting up
to three rects, which need to be available to DrawTheSweepRects() below as well as
as here.  For single-system sweeps, though (all we allow now), we only use topRect. */

static Rect topRect,midRect,botRect;

static Point TrackStaffRect(
						Document *doc,
						Point startPt,								/* mousedown point */
						short	*topStf, short *bottomStf,		/* Top and bottom staves of selection (inclusive) */
						Rect	*paper
						)
{
	Point		pt; long ans;
	Rect		aR, oldR,paperOrig;
	short		startStf, topStartStf, stf;
	Boolean	cancelThis;
	
	GrafPtr	oldPort;
	GrafPtr	docPort;
	
	GetPort(&oldPort);
	
	SetPort(docPort=GetDocWindowPort( doc));
	LockPortBits(docPort);
	theSelectionType = SWEEPING_RECTS;
	
	topStartStf = GetStaff(doc, startPt);
	SetRect(&oldR, 0, 0, 0, 0);								/* Nothing hilited yet */
	cancelThis = TRUE;
	paperOrig = *paper;
	OffsetRect(&paperOrig,-paperOrig.left,-paperOrig.top);
	while (Button())
	{
		GetPaperMouse(&pt,paper);
		if (ABS(pt.h-startPt.h)>=2) {
			cancelThis = FALSE;
		}
		ans = PinRect(&paperOrig,pt);
		pt.h = LoWord(ans); pt.v = HiWord(ans);

		stf = GetStaff(doc, pt);									/* Staff boundary above pt */
		startStf = topStartStf;										/* Staff boundary above startPt */
		if (stf<startStf)	startStf = GetNextStaffn(doc, startStf, TRUE);	/* We want the inclusive */
		else					stf = GetNextStaffn(doc, stf, TRUE);				/*   range of staves */
		SetRect(&aR, startPt.h, staffInfo[startStf].top, pt.h,
							staffInfo[stf].top);
		OffsetRect(&aR,paper->left,paper->top);
		ChangeInvRect(&oldR, &aR);									/* Update the area hilited */
		oldR = aR;
		AutoScroll();
	}
	SetRect(&aR, 0, 0, 0, 0);
	ChangeInvRect(&oldR, &aR);										/* Turn off hiliting */
	
	if (cancelThis)
		pt.h = CANCEL_INT;
	else {
		*topStf = 	 (startStf>stf? stf : startStf);
		*bottomStf = (startStf>stf? GetNextStaffn(doc, startStf, FALSE)
										: GetNextStaffn(doc, stf, FALSE));
	}
	
	UnlockPortBits(docPort);
	SetPort(oldPort);

	return pt;
}


/* --------------------------------------------------------------- ChangeInvRect -- */
/* Change the InvertRect display of <pr1> to a display of <pr2> without
unsightly flashing. N.B. If either rect has negative size along either axis,
it is not considered empty; rather its bounds on that axis are assumed to be
reversed.*/

static void ChangeInvRect(Rect *pr1, Rect *pr2)
{
	Rect	r1, r2, tRect, gridBox;
	short	h, v, hBound[4], vBound[4], gridBoxUsed;
	
	r1 = *pr1;	r2 = *pr2;
	UnemptyRect(&r1);	UnemptyRect(&r2);			/* If needed, reverse axes to make nonempty */
	topRect = r2;
	hBound[0] = r1.left;		hBound[1] = r1.right;
	hBound[2] = r2.left;		hBound[3] = r2.right;
	ShellSort(hBound, 4);
	vBound[0] = r1.top;		vBound[1] = r1.bottom;
	vBound[2] = r2.top;		vBound[3] = r2.bottom;
	ShellSort(vBound, 4);

	/* 
	 *	Look at each box in a grid whose gridpoints are at each boundary of either
	 *	r1 or r2, so that each box is either filled by both, filled by one, or is
	 * empty. If it's filled by both, we know it's already hilited; if by neither,
	 *	we know it's already unhilited. If it's filled by just one, invert it.
	 */
 	for (v = 1; v<4; v++)
 		for (h = 1; h<4; h++) {
 		SetRect(&gridBox, hBound[h-1], vBound[v-1], hBound[h], vBound[v]);
 		gridBoxUsed = 0;
 		if (SectRect(&r1, &gridBox, &tRect)) gridBoxUsed++;
 		if (SectRect(&r2, &gridBox, &tRect)) gridBoxUsed++;
 		if (gridBoxUsed==1) {
 			InvertRect(&gridBox);
 			FlushPortRect(&gridBox);
 		}
 	}
}

/* Invert the current set of sweep rects.  When there is no current actively sweeping
selection, the rectangles should be empty, since this routine is called for every
update event as well as for AutoScroll() (which is what it's really for). */

void DrawTheSweepRects()
{
	InvertRect(&topRect);
	/* And any others */
}


/* ----------------------------------------------------------- FixEmptySelection -- */
/* If the selection is empty, set doc->selStaff to the staff <pt.v> is on or
closest to, and set selStartL and selEndL to create an insertion point near <pt>;
also move the caret there, but avoid putting it right in the middle of a symbol.
If the selection isn't empty, do nothing. */  

void FixEmptySelection(Document *doc, Point	pt)
{
	LINK maySectL, selL; short staffn;
	Rect r;

	/* If selection range is empty, process mouse click to set an insertion point
		which doesn't conflict graphically with any symbol of most types. */

	if (doc->selStartL==doc->selEndL) {						
		staffn = FindStaff(doc, pt);							/* Also sets doc->currentSystem */
		if (staffn==NOONE) return;							/* Click wasn't on any staff; return */

		doc->selStaff = staffn;

		maySectL = LeftLINK(FindSymRight(doc, pt, TRUE, FALSE));
		if (JustTYPE(maySectL)!=J_D) {
			r = LinkOBJRECT(maySectL);
			if (pt.h>=r.left &&  pt.h<=r.right)
				pt.h = r.right+1;									/* Move caret if in middle of symbol */
		}

		/* Make sure the insertion point is after the system's first measure. */
		
		selL = MESetCaret(doc, pt, FALSE);
		if (LinkBefFirstMeas(LeftLINK(selL))) {
			selL = SSearch(selL, MEASUREtype, GO_RIGHT);
			doc->selStartL = doc->selEndL = RightLINK(selL);
		}
		else {
			selL = LocateInsertPt(selL);
			doc->selStartL = doc->selEndL = selL;
		}
		MEAdjustCaret(doc, TRUE);
	}
}

/* --------------------------------------------------------------- GetStaffLimits -- */
/* Fills in the staffInfo array with pixel vertical coordinates of the points
halfway between corresponding staves in the system enclosing <pt> and visibility
flags for the staves. */

static void GetStaffLimits(Document *doc, Point pt, STAFFINFO staffInfo[],
									CONTEXT context[])
{
	short 	staff, s, staffAbove;
	LINK		systemL, staffL;
	DDIST		temp;
	PSYSTEM	pSystem;
	Rect		sysRect;

	staff = FindStaff(doc, pt);									/* FindStaff also sets currentSystem */
	if (staff==NOONE) MayErrMsg("GetStaffLimits: couldn't find staff");

	/* Find the staff and system objects enclosing <pt>. */
	systemL = LSSearch(doc->headL, SYSTEMtype, doc->currentSystem, FALSE, FALSE);
	staffL = LSSearch(systemL, STAFFtype, ANYONE, FALSE, FALSE);

	/* Get the top point for the staff, which is halfway between the bottom line of
	 *	the visible staff above and the top line of this staff, or, for the boundary 
	 *	cases, the top or bottom of the systemRect.
	 */
	ContextSystem(systemL, context);
	ContextStaff(staffL, context);
	pSystem = GetPSYSTEM(systemL);
	D2Rect(&pSystem->systemRect, &sysRect);

	staffInfo[FirstStaffn(staffL)].top = sysRect.top;
	for (s = FirstStaffn(staffL)+1; s<=LastStaffn(staffL); s++) {
		staffAbove = NextStaffn(doc, staffL, FALSE, s-1);
		temp = ((long)context[staffAbove].staffTop + context[staffAbove].staffHeight
						+ context[s].staffTop) / 2L;
		staffInfo[s].top = d2p(temp);
	}

	for (s = 1; s<=doc->nstaves; s++)
		staffInfo[s].visible = context[s].staffVisible;
		
	/* Put a fake staff at the bottom in case nothing above is found. */
	staffInfo[doc->nstaves+1].top = sysRect.bottom;
	staffInfo[doc->nstaves+1].visible = TRUE;
}


/* -------------------------------------------------------------- SelectStaffRect -- */
/* Track mouse dragging, give feedback by inverting one or more complete staves in
the horizontal area, and (when the button is released) select enclosed symbols.
Intended for "wipe" (one-dimensional) selection. Returns FALSE if the mouse was
never moved horizontally more than a couple pixels. */

Boolean SelectStaffRect(Document *doc, Point	pt)
{
	LINK		pL,firstMeasL,sysL,pageL,lastL,lastPageL;
	Point		endPt;
	Rect		selRect, aRect;
	short		index, topStf, bottomStf;
	Boolean	found;
	CONTEXT	context[MAXSTAVES+1];
	STFRANGE stfRange;												/* Range of staves to pass to ContextObject */

	GetStaffLimits(doc, pt, staffInfo, context);				/* and set doc->currentSystem */

	pageL = GetCurrentPage(doc);
	pL = sysL = LSSearch(pageL, SYSTEMtype, doc->currentSystem, GO_RIGHT, FALSE);
	firstMeasL = LSSearch(sysL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	lastL = LastObjInSys(doc,RightLINK(sysL));
	GetAllContexts(doc,context,firstMeasL);
	
	endPt = TrackStaffRect(doc, pt, &topStf, &bottomStf, &doc->currentPaper);	/* Give user feedback while selecting */
	
	if (endPt.h==CANCEL_INT) {
		return FALSE;
	}
	
	/* User dragged off the current page; set an insertion point at the end
		of the system in the direction of the mouse drag, and return TRUE. */

	lastPageL = GetCurrentPage(doc);
	if (lastPageL!=pageL) {
		if (SheetNUM(lastPageL) > SheetNUM(pageL))
			doc->selStartL = doc->selEndL = RightLINK(lastL);
		else
			doc->selStartL = doc->selEndL = RightLINK(firstMeasL);
		MEAdjustCaret(doc, TRUE);
		ArrowCursor();
		return TRUE;
	}

	/*
	 * Set vertical bounds to "infinity", since what wiping selects is determined
	 *	by logical position (staff and system), not graphic position.
	 */
	SetRect(&selRect, pt.h, -32767, endPt.h, 32767);											
	UnemptyRect(&selRect);

	stfRange.topStaff = topStf;
	stfRange.bottomStaff = bottomStf;

	doc->selStartL = doc->selEndL = NILINK;			/* This routine must change these before exiting! */ 
	
	while (pL!=RightLINK(lastL)) {
		ContextObject(doc, pL, context);
		if (VISIBLE(pL))
			if (SectRect(&selRect, &LinkOBJRECT(pL), &aRect)) {
				found = FALSE;
				CheckObject(doc, pL, &found, (Ptr)&selRect, context, SMStaffDrag, &index, stfRange);
				if (found) {
					LinkSEL(pL) = TRUE;									/* update selection */
					if (!doc->selStartL)
						doc->selStartL = pL;
					doc->selEndL = RightLINK(pL);
				}
	  		}
			pL = RightLINK(pL);
			if (SystemTYPE(pL) || PageTYPE(pL)) break;
	}		

	FixEmptySelection(doc, pt);
	if (doc->selStartL==NILINK || doc->selEndL==NILINK) {
		GetIndCString(strBuf, MISCERRS_STRS, 11);					/* "Nightingale is confused" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		doc->selStartL = doc->selEndL = RightLINK(firstMeasL);
		MEAdjustCaret(doc, TRUE);
	}
		
	ArrowCursor();											/* Can only get here if cursor should become arrow */
	return TRUE;		
}


/* --------------------------------------------------- DoThreadSelect and helper -- */

static void StartThread()
{
	SetRect(&selOldRect, 0, 0, 0, 0);
}


/* Handle threader selection (touching symbol with cursor selects) mode. Can select
only certain types of symbols (normally only notes and rests). */

void DoThreadSelect(Document *doc,
							Point	/*pt*/)		/* no longer used */
{
	register LINK pL;
	LINK		pageL, systemL, endL;
	Point		newPt, newPtW;
	Boolean	found, overSheet;
	CONTEXT	context[MAXSTAVES+1];
	short		index, sheetNum, type;
	STFRANGE stfRange = {0,0};
	
#define KLUDGE		/* ??THE CRUDEST POSSIBLE INTERFACE! */
#ifdef KLUDGE
	threadableType = (CapsLockKeyDown() && ShiftKeyDown()? GRAPHICtype : SYNCtype);
#endif

	pageL = GetCurrentPage(doc);
	systemL = LSSearch(pageL, SYSTEMtype, ANYONE, GO_RIGHT, FALSE);
	endL = LastObjOnPage(doc, pageL);
	endL = RightLINK(endL);
	ContextPage(doc, pageL, context);

	/*
	 *	Track the mouse and toggle selection of everything of the appropriate type(s)
	 * that it touches.
	 */
	StartThread();
	while (StillDown()) {
		found = FALSE;
		
		AutoScroll();
		GetPaperMouse(&newPt, &doc->currentPaper);

		newPtW = newPt;
		Pt2Window(doc, &newPtW);
		overSheet = FindSheet(doc, newPtW, &sheetNum);
		if (overSheet && sheetNum!=doc->currentSheet) {			/* we've changed pages */
			doc->currentSheet = sheetNum;
			pageL = GetCurrentPage(doc);
			systemL = LSSearch(pageL, SYSTEMtype, ANYONE, GO_RIGHT, FALSE);
			endL = LastObjOnPage(doc, pageL);
			endL = RightLINK(endL);
			ContextPage(doc, pageL, context);
		}
		
		for (pL=systemL; pL!=endL; pL=RightLINK(pL)) {
			ContextObject(doc, pL, context);
			type = ObjLType(pL);
			if (type==threadableType) {
				if (PtInRect(newPt, &LinkOBJRECT(pL)))
 		 			CheckObject(doc, pL, &found, (Ptr)&newPt, context, SMThread, &index, stfRange);
 		 		if (found) break;
 		 	}
		}
		if (!found) StartThread();
	}
	
	/* Set the selection range. ??Better to set sel. wide open and call OptimizeSelection. */
	
	for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL)) {
		if (LinkSEL(pL)) {
			doc->selStartL = pL;
			break;
		}
	}
	for (pL=doc->tailL; pL!=doc->headL; pL=LeftLINK(pL)) {
		if (LinkSEL(pL)) {
			doc->selEndL = RightLINK(pL);
			break;
		}
	}
}


/* --------------------------------------------------------- InsertSpaceTrackStf -- */
/* Track the staff rect to provide feedback for insertion of space objects. */

Point InsertSpaceTrackStf(Document *doc, Point pt, short *topStf, short *bottomStf)
{
	CONTEXT context[MAXSTAVES+1];
	
	GetStaffLimits(doc, pt, staffInfo, context);
	return TrackStaffRect(doc, pt, topStf, bottomStf, &doc->currentPaper);
}


/* ----------------------------------------------------------------- GetUserRect -- */
/* This routine is used to drag an anchored rectangle around within a window.
When the mouse is clicked down, you can call this with the cursor position
in pt, the anchored rectangle corner in other, an offset (xoff,yoff) from the
current mouse position that will be the corner of the rectangle that tracks
the mouse, and a pointer to the rectangle into which the answer should be
placed when the mouse button is let up.  The outline of the rectangle is
drawn in gray using XOR so that nothing is disturbed graphically, and we
wait until a tick just happens before updating, which helps reduce flicker
for large rectangles.

For simple clicking and dragging, pt and other should initially be the
same; when they are different, you can use the routine to "pick out and
stretch" an already existing rectangle on the screen by setting pt and
other to opposite corners of the starting rectangle.

The offsets are used when you want some slop in picking up a corner of an
already existing rectangle, as in the case of grow handles for objects, etc. */

void GetUserRect(Document *doc, Point pt, Point other, short xoff, short yoff, Rect *box)
{
	Point last; Rect r,lastr,limit,portRect; PenState pn;
	long now;
	
	SetRect(&r,pt.h,pt.v,other.h+xoff,other.v+yoff);
	lastr = r; last = other;
	GetWindowPortBounds(doc->theWindow,&portRect);
	limit = portRect;
	GetPenState(&pn);
	PenPat(NGetQDGlobalsGray()); PenMode(patXor);
	FrameRect(&r);
	
	if (StillDown())
		while (WaitMouseUp()) {
			GetPaperMouse(&other, &doc->currentPaper);
			other.h += xoff; other.v += yoff;

			if (other.h!=last.h || other.v!=last.v) {
				if (other.h < pt.h)
					if (other.v < pt.v) {
						SetRect(&r,other.h,other.v,pt.h,pt.v);
						}
					 else {
						SetRect(&r,other.h,pt.v,pt.h,other.v);
						}
				 else
					if (other.v < pt.v) {
						SetRect(&r,pt.h,other.v,other.h,pt.v);
						}
					 else {
						SetRect(&r,pt.h,pt.v,other.h,other.v);
						}
						
				OffsetRect(&r,doc->currentPaper.left,doc->currentPaper.top);
				/*
				 *	Wait until a tick, which is synchronised with the start of a
				 *	vertical raster scan. There is undoubtedly a better way to do
				 *	this, but this is quick (and dirty) and reduces flicker some.
				 */
				now = TickCount(); while (now == TickCount());
				FrameRect(&lastr);
				FrameRect(&r);
				lastr = r;
				last = other;
				}
			}
	FrameRect(&lastr);
	SetPenState(&pn);
	
	SectRect(&r,&limit,box);
}


/* Set tempFlags of notes and rests, but NOT grace notes, from their selection
status, for the entire score. */

void NotesSel2TempFlags(Document *doc)
{
	LINK pL, aNoteL;
	PANOTE aNote;
	
	for (pL = doc->headL; pL!=doc->tailL; pL=RightLINK(pL))
		if (SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				aNote->tempFlag = aNote->selected;
			}
}

/* Set selection status of notes and rests, but NOT grace notes, from their tempFlags,
for the entire score. */

void TempFlags2NotesSel(Document *doc)
{
	LINK pL, aNoteL;
	register PANOTE aNote;
	Boolean anySel;
	
	for (pL = doc->headL; pL!=doc->tailL; pL=RightLINK(pL))
		if (SyncTYPE(pL)) {
			anySel = FALSE;
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				aNote->selected = aNote->tempFlag;
				if (aNote->selected) anySel = TRUE;
			}
		LinkSEL(pL) = anySel;
		}

	UpdateSelection(doc);
	if (doc->selStartL==doc->selEndL) MEAdjustCaret(doc,TRUE);
}
