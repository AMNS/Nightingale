/*	SFormatHighLevel.c for Nightingale: high-level routines for showing/hiding/
dragging staves, as in Work on Format mode. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define VIS		TRUE
#define INVIS	FALSE

static void SetupShowFormat(Document *doc, Boolean);
static void SetupFormatMenu(Document *doc);
static void EnterShowFormat(Document *doc);
static void ExitShowFormat(Document *doc);

static Boolean SFStaffNonempty(LINK, short);
static Boolean CheckSFInvis(Document *doc);

static Boolean SFVisPossible(Document *doc);
static Boolean SFCanVisify(Document *doc);

static void OffsetStaves(Document *, LINK, DDIST, DDIST);


/* Set up for Work on Format. Deactivate the caret upon entering showFormat, and
reactivate when leaving. Deselect all upon both entering and exiting. */

static void SetupShowFormat(Document *doc, Boolean doEnter)
{
	static Boolean oldFrameSystems;
	LINK selL, measL;
	
	if (doEnter) {
		oldFrameSystems = doc->frameSystems;
		doc->frameSystems = TRUE;
	}
	else
		doc->frameSystems = oldFrameSystems;
	
	MEActivateCaret(doc, !doEnter);
	selL = doc->selStartL;
	DeselAll(doc);
	if (!doEnter) {
		/* If there's a Measure after <selL>, move the insertion pt there. */
		
		measL = LSSearch(selL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
		if (measL)
			doc->selStartL = doc->selEndL = RightLINK(measL);
	}
}

/*-------------------------------------------------------------- SetupFormatMenu -- */
/* Replace the Play/Record menu with the format menu. We must guarantee that the
user cannot enter showFormat from MasterPage, or vice versa; inside Master Page
the masterPgMenu and not the playRecordMenu is installed, and likewise inside
showFormat. */

void SetupFormatMenu(Document *doc)
{
	InstallDocMenus(doc);
}


/* Set up for Show Format: allocate objects, insert the Format menu, etc. */

static void EnterShowFormat(Document *doc)
{
	SetupShowFormat(doc,TRUE);		/* Allocate memory for all necessary objects */
	SetupFormatMenu(doc);			/* Replace the Play/Record menu with Format menu */
}


/* Clean up after Show Format: restore the Play/Record menu, etc. */

static void ExitShowFormat(Document *doc)
{
	HiliteAllStaves(doc, TRUE);		/* Deselect all staves which may have been selected during editing. */

	SetupFormatMenu(doc);			/* Replace the Format menu with Play/Record menu */
	SetupShowFormat(doc, FALSE);
}


/* Change to Show Format view, or back to current page */

void DoShowFormat(Document *doc)
{
	WindowPtr w=doc->theWindow;
	Rect portRect;

	doc->showFormat = !doc->showFormat;

	if (doc->showFormat) {
		doc->enterFormat = TRUE;
		MEHideCaret(doc);
		EnterShowFormat(doc);				/* Allocate memory, import values, etc. */
	}
	else {
		ExitShowFormat(doc);				/* Dispose memory, export values, etc. */
		MEAdjustCaret(doc, FALSE);
	}

	GetWindowPortBounds(w, &portRect);
	ClipRect(&portRect);
	InvalWindowRect(w, &portRect);
}


/* ------------------------------------------------------------- SFInvisify et al -- */
/* If there are any "content" objects on the given staff in the system, return TRUE.
If the staff is the other staff of cross-staff objects, don't worry about it.
We use the term "content" here in a rather specialized sense: clef, key signature,
and time signature changes don't count, since the user presumably wouldn't mind
hiding them. Nor do rests; perhaps rests other than whole-measure rests should
count, though. */

static Boolean SFStaffNonempty(LINK pL, short staffn)
{
	LINK sysL, qL, aNoteL;
	
	sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	
	for (qL=RightLINK(sysL); qL && !SystemTYPE(qL); qL=RightLINK(qL)) {
		switch (ObjLType(qL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(qL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)==staffn && !NoteREST(aNoteL)) return TRUE;
				break;
			case GRSYNCtype:
			case DYNAMtype:
			case SLURtype:
			case BEAMSETtype:
			case TUPLETtype:
			case OTTAVAtype:
			case GRAPHICtype:
			case TEMPOtype:
			case SPACERtype:
			case ENDINGtype:
				if (ObjOnStaff(qL, staffn, FALSE)) return TRUE;
				break;
			default:
				;
		}
	}
	
	return FALSE;
}


/* Check whether we want to invisify all selected staff subObjects; if so return TRUE.
If there are any cross-staff objects (as of v.2.1, only beams or slurs) on the
staff in the system, it can't be invisified. Otherwise, if the staff has any notes,
Tempo marks, Graphics, etc., on it, ask the user if they want to hide it anyway. */

static Boolean CheckSFInvis(Document *doc)
{
	LINK pL,aStaffL,partL; short firstStf,lastStf,staffn;
	PPARTINFO pPart;
	
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (StaffTYPE(pL) && LinkSEL(pL)) {
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
				if (StaffSEL(aStaffL)) {
					staffn = StaffSTAFF(aStaffL);
					partL = Staff2PartL(doc,doc->headL,staffn);
					pPart = GetPPARTINFO(partL);
					firstStf = pPart->firstStaff;
					lastStf = pPart->lastStaff;
					
					/* Cross-staff objects can only occur in multi-staff parts */
					if (lastStf>firstStf)
						if (XStfObjOnStaff(doc->selStartL, staffn)) {
							GetIndCString(strBuf, MISCERRS_STRS, 18);    /* "Can't hide this staff: it contains cross-staff..." */
							CParamText(strBuf, "", "", "");
							StopInform(GENERIC_ALRT);
							return FALSE;
						}
					
					if (SFStaffNonempty(doc->selStartL, staffn)) {
						GetIndCString(strBuf, MISCERRS_STRS, 19);    	/* "The staff isn't empty. Hide it anyway?" */
						CParamText(strBuf, "", "", "");
						if (CautionAdvise(GENERIC3_ALRT)==Cancel) return FALSE;
					}
		}
	}

	return TRUE;
}


/* Actually invisify <aStaffL> according to the process outlined in the description
of SFInvisify. Translate all following staff subObjs upward, and return the DDIST
amount of the translation, so that the calling function can translate following
systems. pL is the staff object. */

DDIST SetStfInvis(Document *doc, LINK pL, LINK aStaffL)
{
	PASTAFF aStaff, bStaff;
	LINK bStaffL, sysL;
	DDIST vDiff, prevStfHt;  short staffn, prevBelowDist;

	sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	prevBelowDist = BelowStfDist(doc, sysL, pL);
	aStaff = GetPASTAFF(aStaffL);
	prevStfHt = aStaff->staffHeight;

	aStaff->visible = FALSE;

	/*
	 * ??VisifyAllObjs is really drastic; it'd much better, and probably do the job,
	 * just to invisify the Staves and Measures, so if a staff is made visible later
	 * we'll be back to where we were.
	 */ 
	VisifyAllObjs(doc,pL,aStaffL,INVIS);

	if (StaffSTAFF(aStaffL) < doc->nstaves) {
			
		/* Find the next visible staff below this one. */
		bStaffL = FirstSubLINK(pL);
		for ( ; bStaffL; bStaffL = NextSTAFFL(bStaffL)) {
			staffn = StaffSTAFF(bStaffL);
			if (staffn == NextLimStaffn(doc, pL, TRUE, StaffSTAFF(aStaffL)+1)) {
				bStaff = GetPASTAFF(bStaffL);
				aStaff = GetPASTAFF(aStaffL);
				vDiff = bStaff->staffTop - aStaff->staffTop;
				aStaff->spaceBelow = vDiff;

				SystemRECT(sysL).bottom -= vDiff;
				break;
			}
		}

		/* Translate all following staff subObjs upward by vDiff. */
		bStaffL = FirstSubLINK(pL);
		for ( ; bStaffL; bStaffL = NextSTAFFL(bStaffL))
			if (StaffSTAFF(bStaffL) > StaffSTAFF(aStaffL)) {
				bStaff = GetPASTAFF(bStaffL);
				bStaff->staffTop -= vDiff;
			}

		return vDiff;
	}
	
	/* Invisified staff was the last in the system; no vertical translation is needed
		for staves. We must still move the bottom of system upward by the amount of space
		occupied by the invisified staff, and return the distance to translate following
		systems on the page upward. */

	vDiff = p2d(prevBelowDist) + prevStfHt;
	aStaff = GetPASTAFF(aStaffL);
	aStaff->spaceBelow = vDiff;
	SystemRECT(sysL).bottom -= vDiff;

	return vDiff;
}


/* Invisify selected staves. Invisifying a Staff sets the subobj's <visible> flag FALSE.
It also removes the vertical space occupied by that Staff by decreasing the staffTop of
each Staff below by the distance between that Staff and the one below, and decreasing
the systemRect.bottom by the same amount. If there are any systems below, their
systemRects are translated up by the same amount. The amount is then stored in the
Staff's <spaceBelow> for use in case the Staff is later made visible. If a Staff belongs
to a multi-staff part:
 * if there are any cross-staff beams or slurs in the system, it can't be invisified
 * the Connect is always drawn from the part's top visible Staff to its bottom
   visible Staff, even if they're the same
InvisifySelStaves makes no user-interface assumptions. */

void InvisifySelStaves(Document *doc)
{
	LINK pL, aStaffL, sysL; DDIST vDiff=0;

	/* Find the first selected Staff object. If the selection is continuous,
		as of v.2.1, it'll be the only selected Staff object. */

	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (StaffTYPE(pL) && LinkSEL(pL)) {
			if (NumVisStaves(pL)<=1) return;							/* Should never happen */

			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
				if (StaffSEL(aStaffL))
					vDiff += SetStfInvis(doc,pL,aStaffL);
			break;
		}
	if (!LinkSEL(pL)) return;

	/* If the system containing the Staff which was made invisible is not the last
		on the page, start at the system after it and translate the following systems
		on the page upward by the amount that the invisified Staff occupied. Update
		the measureRects of measures in these systems to accomodate the translation. */
 
	sysL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	UpdateSysMeasYs(doc, sysL, FALSE, FALSE);

	if (!LastSysInPage(sysL))
		for (sysL=LinkRSYS(sysL); sysL; sysL=LinkRSYS(sysL)) {
			OffsetSystem(sysL, 0, -d2p(vDiff));
			UpdateSysMeasYs(doc, sysL, FALSE, FALSE);
			if (LastSysInPage(sysL)) break;
		}
	
	/*
	 * Do NOT set <doc->locFmtChanged> here: it's only for the positioning changes
	 * that leaving Master Page discards under some circumstances.
	 */
	doc->changed = TRUE;
}


void SFInvisify(Document *doc)
{	
	if (!CheckSFInvis(doc))
		return;

	InvisifySelStaves(doc);
		
	InvalRange(doc->headL, doc->tailL);
	InvalWindow(doc);
}

/* ------------------------------------------------------- VisifyStf and SFVisify -- */

/* If the given staff subobject is invisible, visify it and move its systemRect.bottom
and all staves in its system below it down by the space it now occupies. Return that
space. If the staff is ALREADY visible, do nothing and return 0. */

DDIST VisifyStf(LINK pL, LINK aStaffL, short staffn)
{
	DDIST vDiff;  LINK sysL, bStaffL;  PASTAFF aStaff, bStaff;

	if (StaffVIS(aStaffL)) return 0;

	StaffVIS(aStaffL) = TRUE;
	aStaff = GetPASTAFF(aStaffL);
	vDiff = aStaff->spaceBelow;

	sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);		/* Should always exist */
	SystemRECT(sysL).bottom += vDiff;

	bStaffL = FirstSubLINK(pL);
	for ( ; bStaffL; bStaffL = NextSTAFFL(bStaffL))
		if (StaffSTAFF(bStaffL) > staffn) {
			bStaff = GetPASTAFF(bStaffL);
			bStaff->staffTop += vDiff;
		}
		
	return vDiff;
}


/* Return FALSE if visifying all staves will cause the bottom of the system to go below
the bottom margin of the page even if reformatting is done, i.e., if this system will
not fit on the page; return TRUE to indicate all staves can be visified, though
reformatting might still be necessary. */

static Boolean SFVisPossible(Document *doc)
{
	LINK pL, aStaffL, staffL, sysL, pageL;  DDIST vDiff=0, sysHt;
	PASTAFF aStaff; PSYSTEM pSys; DRect sysRect;
	
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (StaffTYPE(pL) && LinkSEL(pL)) {
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				if (!StaffVIS(aStaffL)) {
					aStaff = GetPASTAFF(aStaffL);
					vDiff += aStaff->spaceBelow;
				}
			}
			staffL = pL; break;	
		}

	pageL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_LEFT, FALSE);

	sysL = StaffSYS(staffL);
	pSys = GetPSYSTEM(sysL);
	
	sysRect = SystemRECT(sysL);
	sysHt = sysRect.bottom-sysRect.top;
	
	/* Compare the new system height to the vertical distance between margins. */
	
	return (d2pt(sysHt+vDiff) < doc->marginRect.bottom-doc->marginRect.top);
}

/* Return FALSE if visifying all staves will cause the bottom of the last system
to go below the bottom margin of the page, to indicate reformatting necessary;
else return TRUE to indicate all staves can be visified. */

static Boolean SFCanVisify(Document *doc)
{
	LINK pL, aStaffL, sysL, pageL;  DDIST vDiff=0;
	PASTAFF aStaff;  PSYSTEM pSys;
	
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (StaffTYPE(pL) && LinkSEL(pL)) {
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				if (!StaffVIS(aStaffL)) {
					aStaff = GetPASTAFF(aStaffL);
					vDiff += aStaff->spaceBelow;
				}
			}
			break;	
		}
		
	pageL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_LEFT, FALSE);

	sysL = GetLastSysInPage(pageL);
	pSys = GetPSYSTEM(sysL);

	/* Compare the position the bottom of the last system would have to the
		bottom margin position. */
	
	return (d2pt(pSys->systemRect.bottom+vDiff) < doc->marginRect.bottom);
}


/* If there are any invisible staves in the given Staff object, show all of them,
move all systems below on its page down accordingly, and return the System the Staff
is in. Otherwise just return NILINK. */

LINK VisifyAllStaves(Document *doc, LINK staffL)
{
	LINK aStaffL, sysL, theSysL;
	DDIST vDiff=0;
	Boolean didSomething=FALSE;
	
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		if (!StaffVIS(aStaffL)) {
			vDiff += VisifyStf(staffL, aStaffL, StaffSTAFF(aStaffL));
			VisifyAllObjs(doc, staffL, aStaffL, VIS);
			InvisFirstMeas(staffL);
			didSomething = TRUE;
		}
	}
	if (!didSomething) return NILINK;

	/* If the system containing the staves which were made visible is not the last
		on the page, start at the system after it and translate the following systems
		on the page downward by the amount that the visified staves now occupy. Update
		the measureRects of measures in these systems to accomodate the translation. */
 
	theSysL = LSSearch(staffL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);		/* Should always succeed */

	if (!LastSysInPage(theSysL))
		for (sysL=LinkRSYS(theSysL); sysL; sysL=LinkRSYS(sysL)) {
			OffsetSystem(sysL, 0, d2p(vDiff));
			if (LastSysInPage(sysL)) break;
		}
	
	UpdateSysMeasYs(doc,theSysL,FALSE,FALSE);
	if (!LastSysInPage(theSysL))
		for (sysL=LinkRSYS(theSysL); sysL; sysL=LinkRSYS(sysL)) {
			UpdateSysMeasYs(doc, sysL, FALSE, FALSE);
			if (LastSysInPage(sysL)) break;
		}

	return theSysL;
}


/* Visify all invisible staves in a selected Staff object. We assume there's no more
than one Staff object in the selection range (this is guaranteed only if the selection
is continuous!). If we encounter one, visify all staff subObjects and all the invisified
objects on each staff.  ??It'd be better not to VisifyAllObjs; maybe just the Measure
objs, since anything else hidden was probably hidden specifically by the user, but
changing this probably requires changing the call to VisifyAllObjs in SetStfInvis...DAB.
*/

void SFVisify(Document *doc)
{
	LINK pL, sysL;
	short rfmt=Cancel;
	
	if (!SFVisPossible(doc)) {
		StopInform(SFVIS_ALL_ALRT1);
		return;
	}
	else if (!SFCanVisify(doc)) {
		rfmt = CautionAdvise(SFVIS_ALL_ALRT);
		if (rfmt!=OK) return;
	}

	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (StaffTYPE(pL) && LinkSEL(pL)) {
			sysL = VisifyAllStaves(doc, pL);
			break;
		}
	if (!sysL) return;											/* If no staves were invisible */

	if (rfmt==OK)
		Reformat(doc, sysL, doc->tailL, FALSE, 9999, FALSE, 999, config.titleMargin);

	doc->changed = TRUE;
	InvalRange(doc->headL, doc->tailL);
	InvalWindow(doc);
}


/* ----------------------------------------------------------------- EditSysRect -- */

#define SLOP	1		/* For recognizing mousedown on system boundary (pixels) */
#define GAP		4		/* Don't let margin closer to edge than this (pixels) */

typedef enum {
	LMARG,
	TMARG,
	RMARG,
	BMARG
} MarginType;

static void OffsetStaves(Document */*doc*/, LINK sysL, DDIST dy, DDIST dx)
{
	LINK staffL,aStaffL;  PASTAFF aStaff;
	
	staffL = LSSearch(sysL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		aStaff->staffTop += dy;
		aStaff->staffRight += dx;
	}
}


/* Let user drag the top or bottom boundary of sysL's systemRect. Return TRUE if
they actually drag it, FALSE if there's a problem or they leave it where it is.
(Rev. by JGG to allow left and right margin dragging, 3/20/01.) */

Boolean EditSysRect(Document *doc, Point pt, LINK sysL)
{
	Boolean		didSomething=FALSE;
	Point		origPt;
	Rect		topMargin, bottomMargin, leftMargin, rightMargin, margin, origMargin, sysRect;
	CursHandle	cursorH;
	short		oldVal, dy, dx, minVal, maxVal, xMove, yMove, staffn;
	LINK		lSys, rSys, staffL, aStaffL;
	MarginType	marginType;
	
	/*	Define the bounding boxes of the margin lines, whose positions on the
		screen are assumed to have been defined by a call such as
		FrameRect(&doc->marginRect).  We use these bounding boxes to determine
		if the user has clicked on one of the margin lines. */

	origPt = pt;
	D2Rect(&SystemRECT(sysL), &origMargin);
	margin = origMargin;

	topMargin = margin;
	topMargin.bottom = topMargin.top + 1;
	
	bottomMargin = margin;
	bottomMargin.top = bottomMargin.bottom - 1;
	
	leftMargin = margin;
	leftMargin.right = leftMargin.left + 1;

	rightMargin = margin;
	rightMargin.left = rightMargin.right - 1;

	/* Add a slop factor for each rectangle */
	
	InsetRect(&topMargin, 0, -SLOP);
	InsetRect(&bottomMargin, 0, -SLOP);
	InsetRect(&leftMargin, -SLOP, 0);
	InsetRect(&rightMargin, -SLOP, 0);
	
	/*	Check to see if point is within our sloppy margin rectangles.  If so,
		then use the applicable cursor, but muck with its hotspot first so
		that user sees it drawn directly on margin line.  We therefore have
		to muck with a copy of the cursor. */

	if (PtInRect(pt, &topMargin))
		marginType = TMARG;
	else if (PtInRect(pt, &bottomMargin))
		marginType = BMARG;
	else if (PtInRect(pt, &leftMargin))
		marginType = LMARG;
	else if (PtInRect(pt, &rightMargin))
		marginType = RMARG;
	else
		return FALSE;

	if (marginType==TMARG || marginType==BMARG) {
		cursorH = GetCursor(DragUpDownID);
		if (!cursorH)
			return FALSE;
		HandToHand((Handle *)&cursorH);
		if (MemError() != noErr)
			{ NoMoreMemory(); return FALSE; }

		/* Shift the cursor's hotspot over by distance from line */
		
		dy = pt.v - (SLOP + ((marginType==TMARG) ? topMargin.top : (bottomMargin.top+1)));
		(*cursorH)->hotSpot.v += dy;
		SetCursor(*cursorH);

		/* Don't let user drag margin closer than GAP pixels to appropriate edge
			of paper, or GAP pixels to prev or next systems, etc. */

		if (marginType==TMARG) {
			if (FirstSysInPage(sysL))
				minVal = doc->paperRect.top + GAP;
			else {
				Rect prevSysRect;
				LINK prevSysL = LinkLSYS(sysL);
				D2Rect(&SystemRECT(prevSysL), &prevSysRect);
				staffn = LastStaffn(RightLINK(prevSysL));
				staffL = LSSearch(prevSysL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
				aStaffL = FirstSubLINK(staffL);
				for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
					if (StaffSTAFF(aStaffL)==staffn)
						break;
				}
				/* bottom of bottom staff of previous system */
				minVal = prevSysRect.top + d2p(StaffTOP(aStaffL) + StaffHEIGHT(aStaffL)) + GAP;
			}

			staffn = FirstStaffn(RightLINK(sysL));
			staffL = LSSearch(sysL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
			aStaffL = FirstSubLINK(staffL);
			for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
				if (StaffSTAFF(aStaffL)==staffn)
					break;
			}
			maxVal = origMargin.top + d2p(StaffTOP(aStaffL)) - GAP;		/* top of top staff of cur sys */
		}
		else {
			staffn = LastStaffn(RightLINK(sysL));
			staffL = LSSearch(sysL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
			aStaffL = FirstSubLINK(staffL);
			for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
				if (StaffSTAFF(aStaffL)==staffn)
					break;
			}
			/* bottom of bottom staff of current system */
			minVal = origMargin.top + d2p(StaffTOP(aStaffL) + StaffHEIGHT(aStaffL)) + GAP;

			if (LastSysInPage(sysL))
				maxVal = doc->paperRect.bottom - GAP;
			else {
				Rect nextSysRect;
				LINK nextSysL = LinkRSYS(sysL);
				D2Rect(&SystemRECT(nextSysL), &nextSysRect);
				staffn = FirstStaffn(RightLINK(nextSysL));
				staffL = LSSearch(nextSysL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
				aStaffL = FirstSubLINK(staffL);
				for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
					if (StaffSTAFF(aStaffL)==staffn)
						break;
				}
				maxVal = nextSysRect.top + d2p(StaffTOP(aStaffL)) - GAP;		/* top of top staff of next sys */
			}
		}
		minVal += doc->currentPaper.top;
		maxVal += doc->currentPaper.top;
	}
	else {		/* LMARG or RMARG */
		cursorH = GetCursor(DragRightLeftID);
		if (!cursorH)
			return FALSE;
		HandToHand((Handle *)&cursorH);
		if (MemError() != noErr)
			{ NoMoreMemory(); return FALSE; }

		/* Shift the cursor's hotspot over by distance from line */
		
		dx = pt.h - (SLOP + ((marginType==LMARG) ? leftMargin.left : (rightMargin.left+1)));
		(*cursorH)->hotSpot.h += dx;
		SetCursor(*cursorH);

		/* Don't let user drag margin closer than GAP pixels to appropriate edge
			of paper, or GAP*4 pixels to other end of system. */

		if (marginType==LMARG) {
			minVal = doc->paperRect.left + GAP;
			maxVal = (doc->paperRect.left + origMargin.right) - (GAP * 4);
		}
		else {
			minVal = (doc->paperRect.left + origMargin.left) + (GAP * 4);
			maxVal = doc->paperRect.right - GAP;
		}
		minVal += doc->currentPaper.left;
		maxVal += doc->currentPaper.left;
	}

	/*	Head into dragging loop. We have to track SystemRECT(sysL) as well, so that
		we can have proper AutoScroll updating. Coordinates within this loop are all
		window-relative pixels. */
		
	OffsetRect(&margin, doc->currentPaper.left, doc->currentPaper.top);	/* convert to window coords. */
	
	PenMode(patXor);

	if (marginType==TMARG || marginType==BMARG) {
		oldVal = pt.v + doc->currentPaper.top;
		if (StillDown()) while (WaitMouseUp()) {
			GetMouse(&pt);
			if (pt.v < minVal)
				pt.v = minVal;
			else if (pt.v > maxVal)
				pt.v = maxVal;
		 	if (pt.v != oldVal) {

				/* Erase old margin, set and draw new margin. */
				
				FrameRect(&margin);
				oldVal = pt.v;
		 		if (marginType==TMARG)
					margin.top = pt.v - dy;
				else
					margin.bottom = pt.v - dy;
				FrameRect(&margin);

				/*	Before calling AutoScroll(), make sure doc->marginRect is converted
					back to paper-relative coordinates, for the benefit of DrawDocumentView(). */

				sysRect = margin;
				OffsetRect(&sysRect, -doc->currentPaper.left, -doc->currentPaper.top);
				Rect2D(&sysRect, &SystemRECT(sysL));
				didSomething = TRUE;

				/* Bottom of sysRect of prev system or top of sysRect of following system
					tracks the movement of dragged sysRect boundary. */

		 		if (marginType==TMARG) {
					lSys = LinkLSYS(sysL);
					if (lSys && SamePage(lSys, sysL))
						SystemRECT(lSys).bottom = SystemRECT(sysL).top;
				}
				else {
					rSys = LinkRSYS(sysL);
					if (rSys && SamePage(rSys, sysL))
						SystemRECT(rSys).top = SystemRECT(sysL).bottom;
				}

				/* MagnifyPaper(&doc->marginRect, &doc->marginRect, -doc->magnify); */

				AutoScroll();
				SleepTicks(2L);									/* Avoid too much flicker */
			}
		}
	}
	else {		/* LMARG or RMARG */
		oldVal = pt.h + doc->currentPaper.left;
		if (StillDown()) while (WaitMouseUp()) {
			GetMouse(&pt);
			if (pt.h < minVal)
				pt.h = minVal;
			else if (pt.h > maxVal)
				pt.h = maxVal;
		 	if (pt.h != oldVal) {

				/* Erase old margin, set and draw new margin. */
				
				FrameRect(&margin);
				oldVal = pt.h;
		 		if (marginType==LMARG)
					margin.left = pt.h - dx;
				else
					margin.right = pt.h - dx;
				FrameRect(&margin);

				/*	Before calling AutoScroll(), make sure doc->marginRect is converted
					back to paper-relative coordinates, for the benefit of DrawDocumentView(). */

				sysRect = margin;
				OffsetRect(&sysRect, -doc->currentPaper.left, -doc->currentPaper.top);
				Rect2D(&sysRect, &SystemRECT(sysL));
				didSomething = TRUE;

				/* MagnifyPaper(&doc->marginRect, &doc->marginRect, -doc->magnify); */

				AutoScroll();
				SleepTicks(2L);									/* Avoid too much flicker */
			}
		}
	}

	if (didSomething) {
		/* Mouse button up: doc->marginRect is ??? */
		D2Rect(&SystemRECT(sysL), &sysRect);
		if (!EqualRect(&origMargin, &sysRect)) {
			InvalWindow(doc);

			switch (marginType) {
				case TMARG:
					yMove = origMargin.top - sysRect.top;
					OffsetStaves(doc, sysL, p2d(yMove), 0);
					break;
				case BMARG:
					rSys = LinkRSYS(sysL);
					if (rSys && SamePage(rSys, sysL)) {
						yMove = origMargin.bottom - sysRect.bottom;
						OffsetStaves(doc, rSys, p2d(yMove), 0);
					}
					break;
				case LMARG:
					xMove = origMargin.left - sysRect.left;
					OffsetStaves(doc, sysL, 0, p2d(xMove));
					break;
				case RMARG:
					xMove = sysRect.right - origMargin.right;
					OffsetStaves(doc, sysL, 0, p2d(xMove));
					break;
			}
		}
	}
	DisposeHandle((Handle)cursorH);
		
	return (didSomething);
}
