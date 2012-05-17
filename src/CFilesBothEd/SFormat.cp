/*	SFormat.c for Nightingale: lower-level routines for showing/hiding/dragging staves,
as in Work on Format mode. */

/*									NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1989-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean EditFormatMargin(Document *doc, Point pt, INT16 modifiers, INT16 doubleClick);
static void UpdateScoreMargins(Document *doc);

static void SFUpdateStfPos(Document *doc, LINK sysL, long newPos);
static void SFInvalMeasures(Document *doc,LINK sysL);
void MoveSysOnPage(Document *doc, LINK sysL, long newPos);


/* ------------- Routines for graphical updating of objects edited in showFormat -- */

static void UpdateScoreMargins(Document */*doc*/)
{
	/* There's nothing to do. */
}


/* Update the position of all the staves belonging to sysL. New position of any
given staff depends on which system sysL is in the page and which staff the staff
is in the system. */

static void SFUpdateStfPos(Document *doc, LINK sysL, long newPos)
{
	LINK staffL, aStaffL; PASTAFF aStaff;
	DDIST dv,sysTop;

	/* Case for the first system in the page. Clip the systemRect.top to
		the marginRect.top, and move the position of the staves relative to
		the system by the same amount in the opposite direction, in order
		to keep them in the same place. */

	if (FirstSysInPage(sysL)) {
		sysTop = SystemRECT(sysL).top;
		if (sysTop < p2d(doc->marginRect.top)) {
			dv = sysTop - p2d(doc->marginRect.top);
			SystemRECT(sysL).top -= dv;

			staffL = SSearch(sysL, STAFFtype, GO_RIGHT);
		
			aStaffL = FirstSubLINK(staffL);
			for (	; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				aStaff = GetPASTAFF(aStaffL);
				aStaff->staffTop += dv;
			}
		}
		return;
	}

	/* All other systems: move the systemRect.top up by half the amount
		of the system drag, and the systemRect.bottom of the previous system
		down, by the same amount. Move the staves up by the same amount as
		the systemRect.top. */

	dv = p2d(HiWord(newPos)); dv >>= 1;

	staffL = SSearch(sysL, STAFFtype, GO_RIGHT);

	aStaffL = FirstSubLINK(staffL);
	for (	; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		aStaff->staffTop += dv;
	}

	SystemRECT(sysL).top -= dv;
	if (LinkLSYS(sysL))
		SystemRECT(LinkLSYS(sysL)).bottom += dv;
}

/* In effect, call InvalRange for each of the measures in sysL; set the valid
flag of each measure FALSE and the objRect to nilRect. */

static void SFInvalMeasures(Document *doc, LINK sysL)
{
	LINK endL;

	endL = LastObjInSys(doc,RightLINK(sysL));
	InvalRange(sysL,endL);
	UpdateSysMeasYs(doc, sysL, TRUE, FALSE);
}


/* Offset the systemRect of sysL by (dx,dy) pixels converted to DDISTs. Doesn't
erase or inval anything; leaves the objRect of sysL alone. */

void OffsetSystem(LINK sysL, INT16 dx, INT16 dy)
{
	OffsetDRect(&SystemRECT(sysL), p2d(dx), p2d(dy));
}

void UpdateSysMeasYs(
			Document *doc,
			LINK sysL,
			Boolean useLedg,	/* TRUE=use doc->ledgerYSp to get the system height */
			Boolean masterPg  /* if !useLedg, TRUE=get the system height from the Master System */
			)
{
	LINK staffL,staves[MAXSTAVES+1],measures[MAXSTAVES+1],prevMeasL,measL,aMeasL;
	PASTAFF aStaff; PAMEASURE aMeas;	DRect sysRect; INT16 i;
	
	staffL = FillStaffArray(doc, sysL, staves);
	measL = SSearch(staffL, MEASUREtype, GO_RIGHT);

	for (prevMeasL=measL; measL; prevMeasL=measL, measL=LinkRMEAS(measL)) {

		if (MeasSYSL(prevMeasL)!=MeasSYSL(measL)) break;

		LinkVALID(measL) = FALSE;

		aMeasL = FirstSubLINK(measL);
		for (; aMeasL; aMeasL = NextMEASUREL(aMeasL))
			measures[MeasureSTAFF(aMeasL)] = aMeasL;

		/* Set the measureRect.bottoms of each Measure in the System but the bottom one:
			each of these is at the top of the one below. Similarly, the top of each
			Measure is at the bottom of the one above, except for the top staff's,
			which is always at 0. */

		for (i = 1; i<doc->nstaves; i++) {						/* All staves but the bottom one */
			aMeas = GetPAMEASURE(measures[i]);
	
			if (StaffVIS(staves[i])) {
				aStaff = GetPASTAFF(staves[NextLimStaffn(doc,staffL,TRUE,i+1)]);
	
				aMeas->measureRect.bottom = aStaff->staffTop;
			}
			else
				aMeas->measureRect.bottom = aMeas->measureRect.top;
		}

		aMeas = GetPAMEASURE(measures[LastStaffn(staffL)]);
		aStaff = GetPASTAFF(staves[LastStaffn(staffL)]);
		/*
		 * Set the measureRect.bottom of the bottom Measure subobj. measureRect is
		 * v-relative to systemTop; => the bottom Measure subobj's measureRect.bottom
		 * will just be the sysHeight.
		 */
		if (useLedg)
			aMeas->measureRect.bottom = MEAS_BOTTOM(aStaff->staffTop,aStaff->staffHeight);
		else {
			if (masterPg)
				sysL = SSearch(doc->masterHeadL, SYSTEMtype, FALSE);
			else
				sysL = LSSearch(measL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);

			sysRect = SystemRECT(sysL);
			aMeas->measureRect.bottom = sysRect.bottom - sysRect.top;
		}
	
		for (i = 2; i<=doc->nstaves; i++) {						/* All staves but the top one */
			aMeas = GetPAMEASURE(measures[i]);
	
			if (StaffVIS(staves[i])) {
				/* ??If the staff above is invisible, setting measureRect.top as follows
					is very questionable! E.g., if i=2 and staff 1 is invisible, we want
					measureRect.top = 0. But it's not clear how to fix this in general. */
				aStaff = GetPASTAFF(staves[NextLimStaffn(doc,staffL,FALSE,i-1)]);
	
				aMeas->measureRect.top = aStaff->staffTop+aStaff->staffHeight;
			}
			else
				aMeas->measureRect.bottom = aMeas->measureRect.top;
		}
		aMeas = GetPAMEASURE(measures[1]);
		aMeas->measureRect.top = 0;
	}	
}

/* Offset all systems after sysL in sysL's page by dh=LoWord(newPos),
dv=HiWord(newPos). Update measure Rects of measures in the system to
reflect the offset. */

void MoveSysOnPage(Document *doc, LINK sysL, long newPos)
{	
	sysL = RSysOnPage(doc, sysL);
	for ( ; sysL; sysL = RSysOnPage(doc, sysL)) {
		OffsetSystem(sysL, 0, HiWord(newPos));
		SFInvalMeasures(doc,sysL);
	}
}

/* Update the main data structure to reflect changes resulting from dragging the
system up or down. */

void UpdateFormatSystem(Document *doc, LINK sysL, long newPos)
{
	if (FirstSysInPage(sysL)) {
		OffsetSystem(sysL, 0, HiWord(newPos));
		MoveSysOnPage(doc, sysL, newPos);

		/* HiWord(newPos) is dv; if dv<0, system has been dragged upwards.
			SFUpdateStfPos handles first Systems in page specially; cf. comments
			in SFUpdateStfPos. */

		if (HiWord(newPos) < 0)
			SFUpdateStfPos(doc, sysL, newPos);
	}
	else {
		OffsetSystem(sysL, 0, HiWord(newPos));
		MoveSysOnPage(doc, sysL, newPos);

		SFUpdateStfPos(doc, sysL, newPos);
	}
	
	/* If a system has been dragged down, the new upper boundary of its
		sysRect and the lower boundary of the previous system's sysRect
		will each get half of the added space. Update the measureRects
		of both the previous system, if one exists on the page, and this
		system accordingly. */

	if (LinkLSYS(sysL))
		if (SamePage(sysL,LinkLSYS(sysL)))
			UpdateSysMeasYs(doc,LinkLSYS(sysL),FALSE,FALSE);
	UpdateSysMeasYs(doc, sysL, FALSE, FALSE);
	
	/* doc, every page, !fixSys, !useLedg, !masterPg */
	FixMeasRectYs(doc, NILINK, FALSE, FALSE, FALSE);

	EraseAndInval(&doc->viewRect);
	doc->locFmtChanged = doc->changed = TRUE;
}

/* Update the main data structure to reflect changes resulting from dragging the
staff up or down. */

void UpdateFormatStaff(Document *doc, LINK staffL, LINK aStaffL, long newPos)
{
	PASTAFF aStaff; LINK sysL; Rect r;

	/* Treat dragging the staff of a single-staff system as a system drag, not
		as a staff drag. */

	sysL = SSearch(staffL, SYSTEMtype, TRUE);
	if (LinkNENTRIES(staffL)==1) {
		UpdateFormatSystem(doc, sysL, newPos);
		return;
	}

	aStaff = GetPASTAFF(aStaffL);
	aStaff->staffTop += p2d(HiWord(newPos));
	
	InvalSystem(sysL);

	/* Inval the area to the left of the system, containing the Connect and
		the part name. */

	r = LinkOBJRECT(sysL);
	r.left = 0;

	OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
	InvalWindowRect(doc->theWindow,&r);

	InvalRange(sysL, (LinkRSYS(sysL) ? LinkRSYS(sysL) : doc->tailL));
	doc->locFmtChanged = doc->changed = TRUE;
	UpdateSysMeasYs(doc, sysL, TRUE, FALSE);
}


/* Update the main data structure to reflect changes resulting from dragging the
Connect up or down. */

void UpdateFormatConnect(Document *doc, LINK staffL, INT16 stfAbove, INT16 stfBelow,
									long newPos)
{
	LINK aStaffL;

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL) >= stfAbove && StaffSTAFF(aStaffL) <= stfBelow)
			UpdateFormatStaff(doc, staffL, aStaffL, newPos);
}


/* -------------------------------- Routines for showFormat editing and selection -- */

static Boolean EditFormatMargin(Document *, Point, INT16, INT16)
{
	return FALSE;
}


/* Select objects in the showFormat mode. */
	
void DoFormatSelect(Document *doc, Point pt)
{
	INT16 index;

	FindFormatObject(doc, pt, &index, SMClick);
}


/* Discern user's intent here by searching for whatever object the user may have
clicked on while in the showFormat mode.  If one found, let user drag it to
whereever, and return TRUE.  If nothing grabbable, return FALSE.  The pt provided
is expected in paper-relative pixels.  */

Boolean DoEditFormat(Document *doc, Point pt, INT16 modifiers, INT16 doubleClick)
{
	Boolean didSomething=FALSE;

	if (EditFormatMargin(doc,pt,modifiers,doubleClick)) {
		didSomething = TRUE;
		UpdateScoreMargins(doc);
	}
	else {	
		MEHideCaret(doc);
		DoFormatSelect(doc, pt);
		DrawMessageBox(doc, TRUE);
	}
	
	ArrowCursor();
	PenNormal();
	
	return didSomething;
}


/* -------------------------------------------------------------------------------- */
/* Routines to handle menu commands from the Show Format menu. */

/* Return staff number of top visible staff in first System at or to left of <pL>. */

INT16 FirstStaffn(LINK pL)
{
	LINK staffL,aStaffL; INT16 firstStaffn=MAXSTAVES+1;
	
	staffL = LSSearch(pL, STAFFtype, ANYONE, GO_LEFT, FALSE);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
		if (StaffVIS(aStaffL))
			firstStaffn = n_min(firstStaffn,StaffSTAFF(aStaffL));
	
	return firstStaffn;
}


/* Return staff number of bottom visible staff in first System at or to left of <pL>. */

INT16 LastStaffn(LINK pL)
{
	LINK staffL,aStaffL; INT16 lastStaffn=0;
	
	staffL = LSSearch(pL, STAFFtype, ANYONE, GO_LEFT, FALSE);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
		if (StaffVIS(aStaffL))
			lastStaffn = n_max(lastStaffn,StaffSTAFF(aStaffL));
	
	return lastStaffn;
}


/* ---------------------------------------------------- NextStaffn,NextLimStaffn -- */
/* Get the staff no. of the next visible staff subObject in staff obj <pL>, starting
from staffn <base>, going in direction <up>. <up> means in direction of increasing
staffn, physically downward. E.g., to get the staffn of the first visible staff at
or after aConnect->staffAbove, call NextStaffn(doc,pL,TRUE,aConnect->staffAbove);
to get the staffn of the first visible staff at or before aConnect->staffBelow, call
NextStaffn(doc,pL,FALSE,aConnect->staffBelow).

Returns 0 if there is no visible staff satisfying the given conditions. */

INT16 NextStaffn(Document *doc, LINK pL, INT16 up, INT16 base)
{
	LINK staffL,aStaffL; INT16 theStaffn;
	
	if (base<1 || base>doc->nstaves) return 0;
	
	staffL = LSSearch(pL, STAFFtype, ANYONE, GO_LEFT, FALSE);
	aStaffL = FirstSubLINK(staffL);

	if (up) {
		theStaffn = MAXSTAVES+1;
		for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
			if (StaffVIS(aStaffL) && StaffSTAFF(aStaffL)>=base)
				theStaffn = n_min(theStaffn,StaffSTAFF(aStaffL));
	}
	else {
		theStaffn = 0;
		for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
			if (StaffVIS(aStaffL) && StaffSTAFF(aStaffL)<=base)
				theStaffn = n_max(theStaffn,StaffSTAFF(aStaffL));
	}

	return (theStaffn>MAXSTAVES? 0 : theStaffn);
}

/* Get the staff no. of the next visible staff subObject in staff obj <pL>, starting
from staffn <base>, going in direction <up>. Identical to NextStaffn, except if
there's no visible staff satisfying the conditions, this returns the extreme staff
no. in the given direction. */

INT16 NextLimStaffn(Document *doc, LINK pL, INT16 up, INT16 base)
{
	INT16 theStaffn;
	
	theStaffn = NextStaffn(doc,pL,up,base);
	if (theStaffn==0) theStaffn = up? doc->nstaves : 1;

	return theStaffn;
}


INT16 NumVisStaves(LINK pL)
{
	LINK staffL,aStaffL,sysL; INT16 numVis=0;
	
	sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	staffL = LSSearch(sysL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
		if (StaffVIS(aStaffL)) numVis++;
		
	return numVis;
}

#define VIS		TRUE
#define INVIS	FALSE

/* --------------------------------------------------------------- VisifySubObjs -- */
/* Visify any subObjs of pL on staffn, or pL itself if it is on staffn,
according to vis: TRUE if make them visible, FALSE if invisible. */

static void VisifySubObjs(LINK pL, INT16 staffn, INT16 vis)
{
	PMEVENT		p;
	HEAP			*tmpHeap;
	LINK			subObjL;
	GenSubObj 	*subObj;
	
	switch (ObjLType(pL)) {
		case STAFFtype:
		case CONNECTtype:
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
				if (subObj->staffn==staffn)
					subObj->visible = vis;
			}
			break;
		case SLURtype:
		case BEAMSETtype:
		case TUPLETtype:
		case OCTAVAtype:
		case GRAPHICtype:
		case ENDINGtype:
			p = GetPMEVENT(pL);
			if (((PEXTEND)p)->staffn==staffn)
				p->visible = vis;
			break;
		case TEMPOtype:
			if (TempoSTAFF(pL)==staffn)
				LinkVIS(pL) = vis;
			break;
		case SPACEtype:
			if (SpaceSTAFF(pL)==staffn)
				LinkVIS(pL) = vis;
			break;
		default:
			MayErrMsg("VisifySubObjs: at %ld, type=%ld not supported",
						(long)pL, ObjLType(pL));
			break;
	}
}


/* Visify everything from <pL> to the end of its system on <aStaffL>'s staff. */

void VisifyAllObjs(Document *doc, LINK pL, LINK aStaffL, INT16 vis)
{
	LINK endL,qL; INT16 staffn;
	
	endL = LastObjInSys(doc, pL);
	staffn = StaffSTAFF(aStaffL);
	
	for (qL=RightLINK(pL); qL!=RightLINK(endL); qL=RightLINK(qL)) {
		VisifySubObjs(qL,staffn,vis);
	}
}


/* Set the first invisible measure of staffL invisible; make the object and all
subobjects invisible. Note that when we visify a staff, we have no way of knowing
whether the first invisible measure on that staff was intended to be invisible
or not. The caller simply calls this routine to unconditionally insure its
invisibility. */

void InvisFirstMeas(LINK staffL)
{
	LINK measL, aMeasL;
	
	measL = LSSearch(staffL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	LinkVIS(measL) = FALSE;

	aMeasL = FirstSubLINK(measL);
	for ( ; aMeasL; aMeasL=NextMEASUREL(aMeasL))
		MeasureVIS(aMeasL) = FALSE;
}


/*  If there are any cross-staff objects on the given staff in the system containing
the given object, return TRUE. ??BUG: doesn't check for cross-staff tuplets! Cf.
checking for cross-staff objs for the Split Part command. */

Boolean XStfObjOnStaff(LINK pL, INT16 staffn)
{
	LINK sysL, qL; PSLUR pSlur; PBEAMSET pBeam;
	
	sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	
	for (qL=RightLINK(sysL); qL && !SystemTYPE(qL); qL=RightLINK(qL)) {
		if (SlurTYPE(qL) && (SlurSTAFF(qL)==staffn || SlurSTAFF(qL)==staffn-1)) {
			pSlur = GetPSLUR(qL);
			if (pSlur->crossStaff) return TRUE;
		}
		
		if (BeamsetTYPE(qL) && (BeamSTAFF(qL)==staffn || BeamSTAFF(qL)==staffn-1)) {
			pBeam = GetPBEAMSET(qL);
			if (pBeam->crossStaff) return TRUE;
		}
	}
	return FALSE;
}


/* -------------------------------------------------------------- GrayFormatPage -- */
/* Paint the sheetRect with gray in penMode notPatBic. This should be called during
the drawing loop in ShowFormat after all the objects have been drawn, in order to
gray out the objects in the page. */

void GrayFormatPage(Document *doc, LINK fromL, LINK toL)
{
	LINK startPage, endPage, pL; Rect aRect,paper;

	PenPat(NGetQDGlobalsGray());
	startPage = LSSearch(fromL, PAGEtype, ANYONE, TRUE, TRUE);
	endPage = LSSearch(toL, PAGEtype, ANYONE, TRUE, TRUE);
	for (pL=startPage; pL!=RightLINK(endPage); pL=RightLINK(pL)) {
		if (PageTYPE(pL)) {
			GetSheetRect(doc, SheetNUM(pL), &paper);
			InsetRect(&paper, 1, 1);
			SectRect(&paper, &doc->viewRect, &aRect);

			PenMode(notPatBic);
			PaintRect(&aRect);
		}
	}
	PenNormal();
}
