/* CheckUtils.c for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Local prototypes */

static short ComputeLimitRTop(Document *, LINK, Rect, Rect, short, PCONTEXT);

/* ------------------------------------------------------------------------------------ */
/* Utilities for CheckSYSTEM */

Rect SetStaffRect(LINK aStaffL, PCONTEXT pContext)
{
	Rect aRect;
	DDIST dTop, dLeft, dRight, dBottom;

	/* aStaff = GetPASTAFF(aStaffL); */
	dTop = StaffTOP(aStaffL)+pContext->systemTop;
	dLeft = StaffLEFT(aStaffL)+pContext->systemLeft;
	dBottom = dTop + StaffHEIGHT(aStaffL);
	dRight = StaffRIGHT(aStaffL)+pContext->systemLeft;
	SetRect(&aRect, d2p(dLeft), d2p(dTop), d2p(dRight), d2p(dBottom));
	
	return aRect;
}


/*
 * Space above 1st staffTop, space below last staffBottom, and lnSpace are three
 * DDIST quantities converted to points or pixels in computation of the staffTop
 * of the limitRect for dragging the system in masterPage and showFormat. Of these,
 * only space above staffTop is an absolute quantity. Therefore, space above
 * staffTop is converted to points, and the rest to pixels.
 */

short AboveStfDist(LINK staffL)
{
	LINK aStaffL;  short firstStf;

	firstStf = FirstStaffn(staffL);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==firstStf) {
			return d2pt(StaffTOP(aStaffL));
		}

	return 0;
}

short BelowStfDist(Document */*doc*/, LINK sysL, LINK staffL)
{
	LINK aStaffL;  short lastStf;
	DRect systemRect;  DDIST belowStf;

	lastStf = LastStaffn(staffL);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==lastStf) {
			systemRect = SystemRECT(sysL);
			
			/* Space below bottom staff = systemHeight - bottom of last staff */

			belowStf = (systemRect.bottom-systemRect.top) - 
				(StaffTOP(aStaffL)+StaffHEIGHT(aStaffL));
			return d2pt(belowStf);
		}

	return 0;
}

/* Compute the top of the limit rect passed to DragGrayRgn for dragging systems.
sysL is the system being dragged. sysRect is the Rect which is made into a gray
region to be dragged.
vDiff is the offset of position of mouseLoc relative to top of grayRgn. */

static short ComputeLimitRTop(Document *doc, LINK sysL,
										Rect /*sysRect*/,				/* unused */
										Rect r, short vDiff,
										PCONTEXT pContext)
{
	short limitTop,sysOffset; LINK staffL;

	/* Handle case for Master Page. */
	if (doc->masterView) {
		limitTop = doc->marginRect.top+doc->currentPaper.top+vDiff;

		sysOffset = r.bottom-r.top;
		limitTop += sysOffset;

		staffL = SSearch(sysL, STAFFtype, GO_RIGHT);
	
		/* Subtract the distance top staff is below systemRect.top */
		limitTop -= AboveStfDist(staffL);

		/* Subtract the distance bottom staff is above systemRect.bottom */
		limitTop -= BelowStfDist(doc, sysL, staffL);

		/* Add minimum gap between systems of 2 lineSpaces */
		limitTop += d2pt(2*LNSPACE(pContext));

		return limitTop;
	}

	/* Handle case for first system of page for showFormat. */
	if (FirstSysInPage(sysL)) {
		limitTop = doc->marginRect.top+doc->currentPaper.top+vDiff;

		staffL = SSearch(sysL, STAFFtype, GO_RIGHT);
	
		/* Subtract the distance top staff is below systemRect */
		limitTop -= AboveStfDist(staffL);

		limitTop += d2p(LNSPACE(pContext));
		return limitTop;
	}

	/* Handle case for following systems of page for showFormat.
		Get position of top of systemRect of sysL; includes vDiff
		to offset position of mouseLoc relative to top of grayRgn. */

	limitTop = r.top+vDiff;
	
	staffL = SSearch(sysL, STAFFtype, GO_RIGHT);

	/* Subtract the distance top staff is below systemRect */
	limitTop -= AboveStfDist(staffL);

	/* Subtract distance bottom stf of previous system is above previous system's
		sysRect.bottom.
		Then can drag sysL up to where bottom of bottom staff of previous system
		touches top of top staff of this system. */

	if (LinkLSYS(sysL))
		limitTop -= BelowStfDist(doc, LinkLSYS(sysL), LinkLSTAFF(staffL));
	
	/* Prevent dragging beyond space needed for 1 ledger line on each system. */

	limitTop += d2p((LNSPACE(pContext) << 1));

	return limitTop;
}

/* Compute sysRect, the Rect used to indicate the system being dragged. sysL is the
system to be dragged; sysRect is the bounding box of all staves contained in the system.
Return sysRect in paper-relative pixels. If the system has no visible staves, return
an empty Rect. */

Rect ComputeSysRect(LINK sysL, Rect paper, PCONTEXT pContext)
{
	LINK staffL, aStaffL;  Rect sysRect, aRect, emptyR = {0,0,0,0};
	short numStaves;  Boolean rectSet = False;

	numStaves = NumVisStaves(sysL);
	if (numStaves<=0) return emptyR;
	
	/* Get the box bounding the staff rects of all staff subObjects in the system. */

	staffL = SSearch(sysL, STAFFtype, False);
	aStaffL = FirstSubLINK(staffL);
	
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		if (StaffVIS(aStaffL)) {
			aRect = SetStaffRect(aStaffL, pContext);
			if (rectSet)
				UnionRect(&aRect, &sysRect, &sysRect);
			else
				{ sysRect = aRect; rectSet = True; }
		}
	}
		
	/* Convert sysRect to paper-relative pixels */

	OffsetRect(&sysRect,paper.left,paper.top);
	
	return sysRect;
}

/*
 * Routine to drag a grayRgn corresponding to the system sysL, when in showFormat
 * or masterPage. ptr is a ptr to the location of the mouseDown, sysL is the system
 * being dragged, sysObjRect is the LinkOBJRECT of sysL, which is in window-relative
 * coordinates, sysRect is the Rect to be converted into the grayRgn for the drag,
 * and r is the systemRect of sysL in paper-relative pixels.
 * The region being dragged, contained in sysRect, is the bounding box of the union
 * of all the staves in the system.
 * Routine creates a grayRgn, offsets down to the location of the second system if
 * user is in masterPage, drags the region, and then updates the location of the
 * system in question.
 */


Boolean DragGraySysRect(Document *doc, LINK sysL, Ptr ptr, Rect sysObjRect,
									Rect sysRect, Rect r, PCONTEXT pContext)
{
	Point dragPt;  short vDiff;  RgnHandle sysRgn;
	Rect limitR, slopR;
	long newPos;  DDIST sysOffset;
	CursHandle upDownCursor;

	dragPt = *(Point *)ptr;
	if (PtInRect(dragPt, &sysObjRect)) {

		vDiff = dragPt.v - sysObjRect.top;

		sysRgn = NewRgn();
		RectRgn(sysRgn, &sysRect);
		dragPt.v += pContext->paper.top;
		
		limitR = r;
		if (doc->masterView) {
			sysOffset = r.bottom-r.top;
			OffsetRect(&limitR, 0, sysOffset);
		}

		InsetRect(&limitR, -4, 0);
		limitR.top = ComputeLimitRTop(doc, sysL, sysRect, r, vDiff, pContext);

		limitR.bottom = doc->currentPaper.bottom;
		slopR = doc->currentPaper;

		upDownCursor = GetCursor(DragUpDownID);
		if (upDownCursor) {
			SetCursor(*upDownCursor);
			newPos = DragGrayRgn(sysRgn, dragPt, &limitR, &slopR, vAxisOnly, NULL);
			if (HiWord(newPos) && HiWord(newPos)!=0x8000) {
				if (doc->masterView) {
					UpdateDraggedSystem(doc, newPos);
					MPDrawParams(doc, sysL, NILINK, doc->yBetweenSysMP, HiWord(newPos));
					doc->sysVChangedMP = True;
				}
				else if (doc->showFormat)
					UpdateFormatSystem(doc, sysL, newPos);
			}
			return True;
		}

	}
	
	return False;
}


/* ------------------------------------------------------------------------------------ */
/* Utilities for CheckSTAFF */

/*
 * Select all subObjects of staff object pL in the part containing the staff <staffn>.
 * Returns the LINK to the part containing <staffn>.
 */

LINK SelectStaff(Document *doc, LINK pL, short staffn)
{
	short firstStaff,lastStaff;
	LINK partL, aStfL;

	firstStaff = lastStaff = NOONE;
	partL=FirstSubLINK(doc->masterHeadL);
	for (partL=NextPARTINFOL(partL); partL; partL=NextPARTINFOL(partL))		/* Skip unused first partL */
		if (PartFirstSTAFF(partL) <= staffn && PartLastSTAFF(partL) >= staffn) {
			firstStaff = PartFirstSTAFF(partL);
			lastStaff = PartLastSTAFF(partL);
			break;
		}
	
	aStfL = FirstSubLINK(pL);
	for ( ; aStfL; aStfL=NextSTAFFL(aStfL)) {
		if (StaffSTAFF(aStfL) >= firstStaff && StaffSTAFF(aStfL) <= lastStaff) {
			StaffSEL(aStfL) = True;
			LinkSEL(pL) = True;
		}
	}

	return partL;
}

/*
 * Set the limit rect for the call to DragGrayRgn issued by DragGrayStaffRect.
 * If there is only 1 staff in the system, a system drag is implemented, and
 * the limit rect is the marginRect of the document.
 *	If the staff is the first staff, dragging is bounded by the systemRect on
 *	the top and the top of the following staff.
 * If the staff is in between other staves, dragging is bounded on the top and
 * bottom by the staves immediately above and below.
 * If the staff is the last staff, dragging is bounded on the top by the previous
 * staff and on the bottom by the marginRect.bottom.
 */

void SetLimitRect(Document *doc, LINK pL, short staffn, Point dragPt,
							Rect wSub, Rect *limitR, CONTEXT context[])
{
	PCONTEXT pContext;  LINK sysL;  DRect sysRect;
	short firstVis, prevStaffn, nextStaffn;
	DDIST stfHeight, lnSpace;
	
	/* Set limit Rect for system drag */
	if (LinkNENTRIES(pL)==1) {
		*limitR = doc->marginRect;
		OffsetRect(limitR, doc->currentPaper.left, doc->currentPaper.top);
		return;
	}
	
	/* Set limit Rect top */

	firstVis = FirstStaffn(pL);
	if (staffn==firstVis) {
		pContext = &context[1];
		sysL = SSearch(pL, SYSTEMtype, True);
		sysRect = SystemRECT(sysL);
		limitR->top = d2p(sysRect.top)+pContext->paper.top
									+(dragPt.v-wSub.top)
									+d2p(LNSPACE(pContext));
	}
	else {
		prevStaffn = NextLimStaffn(doc,pL,False,staffn-1);
		pContext = &context[prevStaffn];
		stfHeight = pContext->staffHeight;
		lnSpace = LNSPACE(pContext);
		limitR->top = d2p(pContext->staffTop+stfHeight)
							+pContext->paper.top+(dragPt.v-wSub.top)
							+d2p(lnSpace);
	}
	
	/* Set limit Rect bottom */

	if (staffn<doc->nstaves) {
		nextStaffn = NextLimStaffn(doc,pL,True,staffn+1);
		pContext = &context[nextStaffn];
		limitR->bottom = d2p(pContext->staffTop)+pContext->paper.top
								+(dragPt.v-wSub.bottom)
								-d2p(LNSPACE(pContext));
	}
	else {
		if (doc->showFormat) {
			pContext = &context[staffn];
			sysL = SSearch(pL, SYSTEMtype, True);
			sysRect = SystemRECT(sysL);
			limitR->bottom = d2p(sysRect.bottom)+pContext->paper.top
										+(dragPt.v-wSub.bottom)
										-d2p(LNSPACE(pContext));
		}
		else
			limitR->bottom = doc->currentPaper.bottom;
	}
}

/*
 * Set up a gray region rectangle to provide feedback for dragging a staff in either
 * masterPage or showFormat; drag the staff, update its position, and return True;
 * if the mousePt (*(Point *)ptr) was not in any staff, return False.
 *
 * rSub is the convex hull of the staffLines in paper-relative coordinates; wSub
 * is rSub converted to window coordinates.
 */

Boolean DragGrayStaffRect(Document *doc, LINK pL, LINK aStaffL, short staffn, Ptr ptr,
										Rect rSub, Rect wSub, CONTEXT context[])
{
	PCONTEXT pContext;  RgnHandle stfRgn;  Point dragPt;
	Rect limitR, slopR;  long newPos;  CursHandle upDownCursor;

	pContext = &context[staffn];
	if (PtInRect(*(Point *)ptr, &rSub)) {
		stfRgn = NewRgn();
		RectRgn(stfRgn, &wSub);
		dragPt = *(Point *)ptr; dragPt.v += pContext->paper.top;

		/* Set the limit Rect and slop Rect for dragging. The dragging is limited
			to one staffLine below the staff above, or one staffLine above the staff
			below; if it is the last staff, there is no lower limit, since dragging
			the last staff downwards expands the systemRect and gives that staff
			all the room it needs. */
		limitR = wSub;
		InsetRect(&limitR, -4, 0);
		slopR = doc->currentPaper;

		SetLimitRect(doc, pL, staffn, dragPt, wSub, &limitR, context);

		upDownCursor = GetCursor(DragUpDownID);
		if (upDownCursor) {
			SetCursor(*upDownCursor);
			newPos = DragGrayRgn(stfRgn, dragPt, &limitR, &slopR, vAxisOnly, NULL);
			if (HiWord(newPos) && HiWord(newPos)!=0x8000) {
				if (doc->masterView) {
					UpdateDraggedStaff(doc, pL, aStaffL, newPos);
					doc->stfChangedMP = True;
				}
				else
					UpdateFormatStaff(doc, pL, aStaffL, newPos);
			}

 			return True;
		}

	}
	
	return False;
}

/*
 * Toggle the selection hiliting of staff subObjs in pL; if doDeselect is True,
 * deselect pL.
 */

void HiliteStaves(Document *doc, LINK pL, CONTEXT context[], short doDeselect)
{
	LINK aStaffL;  short staffn;  PCONTEXT pContext;
	DDIST dTop,dLeft,dBottom,dRight;  Rect rSub,wSub;

	if (VISIBLE(pL)) {
		aStaffL = FirstSubLINK(pL);
		for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
			if (StaffSEL(aStaffL) && StaffVIS(aStaffL)) {
				staffn = StaffSTAFF(aStaffL);
				pContext = &context[staffn];
				dTop = pContext->staffTop;
				dLeft = pContext->staffLeft;
				dBottom = dTop + pContext->staffHeight;
				dRight = pContext->staffRight;
	
				SetRect(&rSub, d2p(dLeft), d2p(dTop), d2p(dRight), d2p(dBottom));
	
				wSub = rSub;
				OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
			
				HiliteRect(&wSub);
			}
	}

	if (doDeselect)
		DeselectNode(pL);
}

/*
 * Toggle the selection hiliting of all staves in pL; if doDeselect is True,
 * deselect them.
 */

void HiliteAllStaves(Document *doc, short doDeselect)
{
	LINK staffL; CONTEXT context[MAXSTAVES+1];
	
	staffL = LSSearch(doc->headL, STAFFtype, ANYONE, GO_RIGHT, False);
	
	for ( ; staffL; staffL=LinkRSTAFF(staffL)) {
		GetAllContexts(doc, context, staffL);
		HiliteStaves(doc,staffL,context,doDeselect);
	}
}

/* ------------------------------------------------------------------------------------ */
/* Utilities for CheckGRAPHIC */


/* -------------------------------------------------------------------- GraphicWidth -- */
/* Compute and return the StringWidth (in pixels) of the given Graphic in the
current view. */

short GraphicWidth(Document *doc, LINK pL, PCONTEXT pContext)
{
	short		font, fontSize, fontStyle;
	PGRAPHIC	p;
	Str255		string;
	DDIST		lineSpace;
	
	Pstrcpy((StringPtr)string, (StringPtr)PCopy(FirstGraphicSTRING(pL))) ;

	p = GetPGRAPHIC(pL);
	font = doc->fontTable[p->fontInd].fontID;
	fontStyle = p->fontStyle;

	lineSpace = LNSPACE(pContext);
	fontSize = GetTextSize(p->relFSize, p->fontSize, lineSpace);
	fontSize = UseTextSize(fontSize, doc->magnify);

	return NStringWidth(doc, string, font, fontSize, fontStyle);
}


/* ------------------------------------------------------------------------------------ */
/* Utilities for CheckKEYSIG */


/* ---------------------------------------------------------------------- SetSubRect -- */

Rect SetSubRect(DDIST xd, DDIST dTop, short width, PCONTEXT pContext)
{
	Rect rSub;

	SetRect(&rSub,
				d2p(xd),
				d2p(dTop - pContext->staffHalfHeight),
				d2p(xd) + width,
				d2p(dTop + pContext->staffHeight + pContext->staffHalfHeight/2));

	return rSub;
}


/* ------------------------------------------------------------------------------------ */
/* FindObject and utilities */


/* ------------------------------------------------------------------- ContextObject -- */
/* During traversal of data structure, update the context array by calling the
 * appropriate Context routine at any structural object encountered.
 */

void ContextObject(Document *doc, LINK pL, CONTEXT context[])
{
	switch (ObjLType(pL)) {
		case PAGEtype:
			ContextPage(doc, pL, context);
			break;
		case SYSTEMtype:
			ContextSystem(pL, context);
			break;
		case STAFFtype:
			ContextStaff(pL, context);
			break;
		case MEASUREtype:
			ContextMeasure(pL, context);
			break;
		case CONNECTtype:
			break;
		default:
			break;
	}
}


/* --------------------------------------------------------------------- CheckObject -- */
/* Perform the action selected by <mode> on object <pL>. See Check.h for a list of
possible values for <mode>. stfRange passes the staff parameters for SMStaffDrag
mode. */

LINK CheckObject(Document *doc, LINK pL, Boolean *found, Ptr ptr, CONTEXT context[],
						short mode, short *pIndex, STFRANGE stfRange)
{
	Rect r;  Point enlarge = {0,0};  Point enlargeNR;
	
	SetPt(&enlargeNR, config.enlargeNRHiliteH, config.enlargeNRHiliteV);

	if (ObjLType(pL)!=threadableType && mode==SMThread) return NILINK;

	*found = False;
	switch (ObjLType(pL)) {
		case PAGEtype:
		case SYSTEMtype:
		case CONNECTtype:
		case TAILtype:
			break;
		case STAFFtype:
			if (mode==SMDeselect) DeselectSTAFF(pL);
			break;
		case CLEFtype:
			if ((*pIndex = CheckCLEF(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case DYNAMtype:
			if ((*pIndex = CheckDYNAMIC(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case RPTENDtype:
			if ((*pIndex = CheckRPTEND(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case ENDINGtype:
			if ((*pIndex = CheckENDING(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case KEYSIGtype:
			if ((*pIndex = CheckKEYSIG(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case TIMESIGtype:
			if ((*pIndex = CheckTIMESIG(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case MEASUREtype:
			if ((*pIndex = CheckMEASURE(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case PSMEAStype:
			if ((*pIndex = CheckPSMEAS(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case SYNCtype:
			if ((*pIndex = CheckSYNC(doc, pL, context, ptr, mode, stfRange, enlarge, enlargeNR))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case GRSYNCtype:
			if ((*pIndex = CheckGRSYNC(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case SLURtype:
			if ((*pIndex = CheckSLUR(doc, pL, context, ptr, mode, stfRange))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case BEAMSETtype:
			if ((*pIndex = CheckBEAMSET(doc, pL, context, ptr, mode, stfRange))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case TUPLETtype:
			if ((*pIndex = CheckTUPLET(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case GRAPHICtype:
			if ((*pIndex = CheckGRAPHIC(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case TEMPOtype:
			if ((*pIndex = CheckTEMPO(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case SPACERtype:
			if ((*pIndex = CheckSPACER(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case OTTAVAtype:
			if ((*pIndex = CheckOTTAVA(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		default:
			switch(mode) {
				case SMClick:
					*pIndex = 0;
					return pL;
					break;
				case SMDrag:
				case SMSelect:
					*found = True;
				case SMDeselect:
					r = LinkOBJRECT(pL);
					InsetRect(&r, -1, -1);
					InvertRect(&r);
					InsetRect(&r, 1, 1);
					break;
			}
			break;
	}
	return NILINK;
}

/* ---------------------------------------------------------------------- ObjectTest -- */
/* Determine if mousedown at <pt> is a candidate for Check routine to handle selection
of pL or one of pL's subObjects. */

Boolean ObjectTest(Rect *paper, Point pt, LINK pL)
{
	Rect r, tempRect;
	LINK aSlurL;
	
	switch (ObjLType(pL)) {
		case PAGEtype:
		case SYSTEMtype:
		case STAFFtype:
		case CONNECTtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case MEASUREtype:
		case PSMEAStype:
		case GRSYNCtype:
		case TEMPOtype:
		case SPACERtype:
		case DYNAMtype:
		case GRAPHICtype:
		case RPTENDtype:
		case ENDINGtype:
		case BEAMSETtype:
		case TUPLETtype:
		case OTTAVAtype:
			tempRect = LinkOBJRECT(pL);
			InsetRect(&tempRect, -1, -1);
			return(PtInRect(pt, &tempRect));
		case SYNCtype:
			tempRect = LinkOBJRECT(pL);
			InsetRect(&tempRect, -ENLARGE_NR_SELH, -ENLARGE_NR_SELV);
			return(PtInRect(pt, &tempRect));
		case SLURtype:
			r = LinkOBJRECT(pL);
			if (PtInRect(pt, &r)) {
				aSlurL = FirstSubLINK(pL);
				for ( ; aSlurL; aSlurL=NextSLURL(aSlurL))
					if (FindSLUR(paper, pt, aSlurL)) return True;
			}
			return False;
		default:
			return False;
	}
}


/* ---------------------------------------------------------------------- FindObject -- */
/*	Find the object that <pt> is within, optionally set its selected flag, and
return its LINK, or just return NILINK if point is not within any object.  The
calling routine is responsible for updating the selection range. */

LINK FindObject(Document *doc, Point pt, short *pIndex, short checkMode)
{
	LINK		pL, firstMeas, endL;
	CONTEXT		context[MAXSTAVES+1];
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};
	Point		enlargeNR;

	SetPt(&enlargeNR, config.enlargeNRHiliteH, config.enlargeNRHiliteV);

	BuildCharRectCache(doc);						/* ensure charRectCache valid */
	
	pL = GetCurrentPage(doc); 
	endL = LinkRPAGE(pL);
	if (endL == NILINK) endL = doc->tailL;
	
	for ( ; pL!=endL; pL=RightLINK(pL)) {			/* find object that was clicked in */
		switch (ObjLType(pL)) {
			case PAGEtype:
				firstMeas = LSSearch(pL, MEASUREtype, ANYONE, False, False);
				GetAllContexts(doc, context, firstMeas);
				break;
			case SYSTEMtype:
				ContextSystem(pL, context);
				break;
			case STAFFtype:
				ContextStaff(pL, context);
				break;
			case MEASUREtype:
				ContextMeasure(pL, context);
				break;
			case CONNECTtype:
				break;
			default:
				break;
		}
		if (VISIBLE(pL))
			if (ObjectTest(&context->paper,pt,pL)) {
				switch (ObjLType(pL)) {
					case PAGEtype:
					case SYSTEMtype:
					case STAFFtype:
					case CONNECTtype:
						break;
					case CLEFtype:
						if ((*pIndex = CheckCLEF(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case KEYSIGtype:
						if ((*pIndex = CheckKEYSIG(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case TIMESIGtype:
						if ((*pIndex = CheckTIMESIG(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case MEASUREtype:
						if ((*pIndex = CheckMEASURE(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case PSMEAStype:
						if ((*pIndex = CheckPSMEAS(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case SYNCtype:
						if ((*pIndex = CheckSYNC(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge, enlargeNR))!=NOMATCH)
							return pL;
						break;
					case GRSYNCtype:
						if ((*pIndex = CheckGRSYNC(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case GRAPHICtype:
						if ((*pIndex = CheckGRAPHIC(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case TEMPOtype:
						if ((*pIndex = CheckTEMPO(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case SPACERtype:
						if ((*pIndex = CheckSPACER(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case DYNAMtype:
						if ((*pIndex = CheckDYNAMIC(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case RPTENDtype:
						if ((*pIndex = CheckRPTEND(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case ENDINGtype:
						if ((*pIndex = CheckENDING(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;
					case BEAMSETtype: 
						if ((*pIndex = CheckBEAMSET(doc, pL, context, (Ptr)&pt, checkMode, stfRange))!=NOMATCH)
							return pL;
						break;					
					case TUPLETtype: 
						if ((*pIndex = CheckTUPLET(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;					
					case OTTAVAtype: 
						if ((*pIndex = CheckOTTAVA(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
							return pL;
						break;	
					case SLURtype: 
						if ((*pIndex = CheckSLUR(doc, pL, context, (Ptr)&pt, checkMode, stfRange))!=NOMATCH)
							return pL;
						break;	
								
					default:
						*pIndex = 0;
						return pL;
				}
		}
	}

	return NILINK;
}


/* ------------------------------------------------------------------- FindRelObject -- */
/* Version of FindObject called when we need to find a relative object, e.g., the
firstObj of a GRAPHIC, etc. */

LINK FindRelObject(Document *doc, Point pt, short *pIndex, short checkMode)
{
	LINK		pL,qL,firstMeas,endL,sL;
	CONTEXT		context[MAXSTAVES+1];
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};

	pL = FindObject(doc, pt, pIndex, checkMode);
	if (pL) return pL;
	
	/* Desperately seeking places for GRAPHICs */
	if (checkMode==SMFind) {
		pL=GetCurrentPage(doc);
		endL = LinkRPAGE(pL);
		firstMeas = LSSearch(pL, MEASUREtype, ANYONE, False, False);
		GetAllContexts(doc,context,firstMeas);
		if (CheckPAGE(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge)!=NOMATCH) {
			for (qL=pL; qL!=endL; qL=RightLINK(qL))					/* find object that was clicked in */
				if (SystemTYPE(qL)) {
					enlarge.v = 4;									/* avoid filtering out extreme positions */
					if (CheckSYSTEM(doc, qL, context, (Ptr)&pt, checkMode, stfRange, enlarge)!=NOMATCH) {
						sL = SSearch(qL,MEASUREtype,GO_RIGHT);
						for ( ; sL && MeasSYSL(sL)==qL; sL = LinkRMEAS(sL)) {
							if (IsPtInMeasure(doc, pt, sL)) return sL;
						}
						return qL;
					}
				}
			*pIndex = 0;
			return pL;
		}
	}

	return NILINK;
}

/* ------------------------------------------------------------------------------------ */
/* FindMasterObject and utilities */


/* --------------------------------------------------------------- CheckMasterObject -- */
/* Perform the action selected by <mode> on object <pL>. See Check.h for a list of
possible values for <mode>. stfRange passes the staff parameters for SMStaffDrag mode. */

LINK CheckMasterObject (Document *doc, LINK pL, Boolean *found, Ptr ptr,
							CONTEXT context[], short mode, short *pIndex, STFRANGE stfRange)
{
	Point enlarge = {0,0};

	*found = False;
	switch (ObjLType(pL)) {
		case PAGEtype:
			if ((*pIndex=CheckPAGE(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case SYSTEMtype:
			if ((*pIndex=CheckSYSTEM(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case STAFFtype:
			if ((*pIndex=CheckSTAFF(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
		case CONNECTtype:
			if ((*pIndex=CheckCONNECT(doc, pL, context, ptr, mode, stfRange, enlarge))!=NOMATCH) {
				*found = True;
				return pL;
			}
			break;
	}
	return NILINK;
}


/* ------------------------------------------------------------ MasterObjectTest -- */
/* Determine if mousedown at <pt> is a candidate for Check routine to handle
selection of pL or one of pL's subObjects. */

Boolean MasterObjectTest(
						Rect */*paper*/,				/* unused */
						Point pt, LINK pL)
{
	Rect tempRect; LINK sysL;
	
	switch (ObjLType(pL)) {
		case PAGEtype:
		case SYSTEMtype:
		case CONNECTtype:
			tempRect = LinkOBJRECT(pL);
			InsetRect(&tempRect, -1, -1);
			return(PtInRect(pt, &tempRect));

		case STAFFtype:
			sysL = SSearch(pL, SYSTEMtype, True);
			tempRect = LinkOBJRECT(sysL);
			InsetRect(&tempRect, -1, -1);
			return(PtInRect(pt, &tempRect));

		default:
			return False;
	}
}


/* --------------------------------------------------------------- FindMasterObject -- */
/* Find the object and subObject in the masterPage containing the mousedown <pt>.
Traverse the masterPage data structure from tailL to headL, in order to correctly
handle the spatial nesting of the objRects of connects, staves, systems and pages.
That is, PAGEs enclose SYSTEMs, and SYSTEMs enclose STAFFs, so if we traverse from
masterHeadL to masterTailL, STAFFs will be masked by SYSTEMs and SYSTEMs by PAGEs,
with the net result that PAGEs mask everything. But a traversal from tailL to headL
will correctly find STAFFs before their enclosing SYSTEMs, and SYSTEMs before their
enclosing PAGEs. */

LINK FindMasterObject(Document *doc, Point pt, short *pIndex, short checkMode)
{
	LINK		pL, endL;
	CONTEXT		context[MAXSTAVES+1];
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};

	pL = doc->masterTailL;
	endL = GetMasterPage(doc); 
	
	for ( ; pL!=endL; pL=LeftLINK(pL)) {
		switch (ObjLType(pL)) {
			case PAGEtype:
			case SYSTEMtype:
			case STAFFtype:
				break;
			case CONNECTtype:
				GetAllContexts(doc,context,pL);
				break;
			default:
				break;
		}
		
		if ( /* MasterObjectTest(&context->paper,pt,pL) */ 1 ) {
			switch (ObjLType(pL)) {
				case PAGEtype:
					if ((*pIndex=CheckPAGE(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
						return pL;
					break;
				case SYSTEMtype:
					if ((*pIndex=CheckSYSTEM(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
						return pL;
					break;
				case STAFFtype:
					if ((*pIndex=CheckSTAFF(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
						return pL;
					break;
				case CONNECTtype:
					if ((*pIndex=CheckCONNECT(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
						return pL;
					break;
			}
		}
	}
	
	return NILINK;
}


/* ------------------------------------------------------------------------------------ */
/* FindFormatObject and utilities */

/* ---------------------------------------------------------------- FormatObjectTest -- */
/* Determine if mousedown at <pt> is a candidate for Check routine to handle
selection of pL or one of pL's subObjects. ??As of v. 5.7, this is unused. */

Boolean FormatObjectTest(
					Rect */*paper*/,			/* unused */
					Point pt, LINK pL)
{
	Rect tempRect; LINK sysL;
	
	switch (ObjLType(pL)) {
		case PAGEtype:
		case SYSTEMtype:
		case CONNECTtype:
			tempRect = LinkOBJRECT(pL);
			InsetRect(&tempRect, -1, -1);
			return(PtInRect(pt, &tempRect));

		case STAFFtype:
			sysL = SSearch(pL, SYSTEMtype, True);
			tempRect = LinkOBJRECT(sysL);
			InsetRect(&tempRect, -1, -1);
			return(PtInRect(pt, &tempRect));

		default:
			return False;
	}
}


/* ---------------------------------------------------------------- FindFormatObject -- */
/* In the showFormat mode, find the object and subObject containing the mousedown
<pt>.

Traverse the main data structure from tailL to headL, in order to correctly handle
the spatial nesting of the objRects of connects, staves, systems and pages.
That is, PAGEs enclose SYSTEMs, and SYSTEMs enclose STAFFs, so if we traverse from
headL to tailL, STAFFs will be masked by SYSTEMs and SYSTEMs by PAGEs, with the
net result that PAGEs mask everything. But a traversal from tailL to headL will
correctly find STAFFs before their enclosing SYSTEMs, and SYSTEMs before their
enclosing PAGEs. */

LINK FindFormatObject(Document *doc, Point pt, short *pIndex, short checkMode)
{
	LINK		pL, endL;
	CONTEXT		context[MAXSTAVES+1];
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};

	endL = GetCurrentPage(doc); 
	pL = (LinkRPAGE(endL) ? LinkRPAGE(endL) : doc->tailL);

	for (pL=LeftLINK(pL); pL!=endL; pL=LeftLINK(pL)) {
		switch (ObjLType(pL)) {
			case PAGEtype:
			case SYSTEMtype:
			case STAFFtype:
				break;
			case CONNECTtype:
				GetAllContexts(doc, context, pL);
				break;
			default:
				break;
		}
		
		/* ??I have no idea why the call to FormatObjectTest is replaced with True! */
		if ( /* FormatObjectTest(&context->paper,pt,pL) */ 1 ) {
			switch (ObjLType(pL)) {
				case PAGEtype:
					if ((*pIndex=CheckPAGE(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
						return pL;
					break;
				case SYSTEMtype:
					if ((*pIndex=CheckSYSTEM(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
						return pL;
					break;
				case STAFFtype:
					if ((*pIndex=CheckSTAFF(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
						return pL;
					break;
				case CONNECTtype:
					if ((*pIndex=CheckCONNECT(doc, pL, context, (Ptr)&pt, checkMode, stfRange, enlarge))!=NOMATCH)
						return pL;
					break;
			}
		}
	}

	return NILINK;
}
