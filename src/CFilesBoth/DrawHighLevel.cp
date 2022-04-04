/******************************************************************************************
*	FILE:	DrawHighLevel.c
*	PROJ:	Nightingale
*	DESC:	Routines for drawing object-list ranges
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void FrameSysRect(Rect *, Boolean);
static void DrawMasterSystem(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *, short);
static void DrawMasterRange(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *);
static void DrawFormatRange(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *);
static void DrawScoreRange(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *);
static void HiliteScoreRange(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *);
static void SetMusicPort(Document *);


/* ---------------------------------------------------------------------- FrameSysRect -- */

void FrameSysRect(
			Rect *r,
			Boolean /*solid*/			/* unused */
			)
{
	PenPat(NGetQDGlobalsBlack());
	ForeColor(blueColor);
	FrameRect(r);
	PenNormal();
}


/* ------------------------------------------------------------------- DrawPageContent -- */
/* Draw all objects in the specified page. */

void DrawPageContent(Document *doc, short sheetNum, Rect *paper, Rect *updateRect)
{
	PPAGE pPage;  LINK pageL;
	
	pageL = LSSearch(doc->headL, PAGEtype, sheetNum, False, False);
	if (pageL) {
		pPage = GetPPAGE(pageL);
		DrawRange(doc, pageL, pPage->rPage ? pPage->rPage : doc->tailL, paper, updateRect);
	}
}

/* ------------------------------------------------------------------ DrawMasterSystem -- */
/* Draw the objects in the given range, which should be the Master Page system. If
drawing the system the first time, the page object needs to be included in the
range to be drawn. */

static void DrawMasterSystem(Document *doc, LINK fromL, LINK toL, CONTEXT context[],
								Rect *paper, Rect *updateRect, short sysNum)
{
	LINK pL;
	Rect r, result, paperUpdate;
	PSYSTEM pSystem;
	short ground;

	paperUpdate = *updateRect;
	OffsetRect(&paperUpdate, -paper->left, -paper->top);
	ground = (sysNum==1? TOPSYS_STAFF : (sysNum==2? SECONDSYS_STAFF : OTHERSYS_STAFF ));
	
	for (pL=fromL; pL!=toL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case PAGEtype:
				GetAllContexts(doc, context, pL);
				ContextPage(doc, pL, context);
				DrawPAGE(doc, pL, paper, context);
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);
				
				/* Convert systemRect to window-relative pixels, and check if it
				   intersects with update area. */
				   
				D2Rect(&pSystem->systemRect, &r);
				OffsetRect(&r, paper->left, paper->top);
				if ((VISIBLE(pL) && SectRect(&r, updateRect, &result))
													|| outputTo!=toScreen) {
					DrawSYSTEM(doc, pL, paper, context);
					if (doc->frameSystems) FrameSysRect(&r, False);	/* For debugging */
				}
				else return;
				break;
			case STAFFtype:
				if (VISIBLE(pL))
					DrawSTAFF(doc, pL, paper, context, ground, sysNum==1);
				break;
			case CONNECTtype:
				if (VISIBLE(pL)) DrawCONNECT(doc, pL, context, ground);
				break;
			default:
				;
		}
}

static void DrawMasterRange(Document *doc, LINK /*fromL*/, LINK /*toL*/, CONTEXT context[],
										Rect *paper, Rect *updateRect)
{
	LINK sysL, staffL;  DRect oldSysRect;  Rect r;
	DDIST sysHeight, sysOffset;
	short sysNum=0;

	/* If the score has no parts, return without drawing anything. */

	staffL = SSearch(doc->masterHeadL, STAFFtype, GO_RIGHT);
	if (LinkNENTRIES(staffL)==0) return;

	/* Draw the single "real" master system. */
	
	DrawMasterSystem(doc, doc->masterHeadL, doc->masterTailL, context, paper,
							updateRect, ++sysNum);

	sysL = LSSearch(doc->masterHeadL, SYSTEMtype, ANYONE, False, False);
	oldSysRect = SystemRECT(sysL);

	sysHeight = oldSysRect.bottom - oldSysRect.top;
	sysOffset = sysHeight;
	
	/*
	 * Draw as many additional copies of the master system as fit on a page. Be
	 *	careful of overflow.
	 */
	if ((long)SystemRECT(sysL).bottom+(long)sysOffset<=32767L) {
		OffsetDRect(&SystemRECT(sysL), 0, sysOffset);
		while (MasterRoomForSystem(doc, sysL)) {
			DrawMasterSystem(doc, sysL, doc->masterTailL, context, paper, updateRect,
								++sysNum);
			if ((long)SystemRECT(sysL).bottom+(long)sysOffset>32767L) break;
			OffsetDRect(&SystemRECT(sysL), 0, sysOffset);
		}
	}
	doc->nSysMP = sysNum;
	
	SystemRECT(sysL) = oldSysRect;

	/*
	 * Restore the objRect of sysL, which has been modified by DrawSYSTEM
	 * called with the systemRect offset downwards.
	 */
	D2Rect(&SystemRECT(sysL), &r);
	LinkOBJRECT(sysL) = r;
}

static void DrawFormatRange(Document *doc, LINK fromL, LINK toL, CONTEXT context[],
										Rect *paper, Rect *updateRect)
{
	LINK		pL, measL;
	PSYSTEM 	pSystem;
	PMEASURE	pMeasure;
	Rect 		r, result,
				paperUpdate;		/* Paper-relative update rect */
	Boolean	drawAll=True;			/* False if we're only drawing measure-spanning objects */
	
	paperUpdate = *updateRect;
	OffsetRect(&paperUpdate, -paper->left, -paper->top);
	if (doc->enterFormat) goto grayPage;
	
	for (pL=fromL; pL!=toL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case PAGEtype:
				/* See explanation of following code in DrawScoreRange. */
				measL = SSearch(pL, MEASUREtype, GO_RIGHT);
				GetAllContexts(doc, context, measL);
				ContextPage(doc, pL, context);
				DrawPAGE(doc, pL, paper, context);
				drawAll = True;
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);
				
				/* Convert systemRect to window-relative pixels, and check if it
				   intersects with update area. */
				   
				D2Rect(&pSystem->systemRect, &r);
				OffsetRect(&r, paper->left, paper->top);
				if ((VISIBLE(pL) && SectRect(&r, updateRect, &result))
													|| outputTo!=toScreen) {
					DrawSYSTEM(doc, pL, paper, context);
					if (doc->frameSystems) FrameSysRect(&r, True);
				}
				else {
					/* Skip ahead to next system: this one's not visible */
					
					pL = pSystem->rSystem;
					if (pL==NILINK || IsAfter(toL, pL)) pL = toL;
					pL = LeftLINK(pL);					/* Rewind for for loop above */
				}
				drawAll = True;
				break;
			case STAFFtype:
				if (VISIBLE(pL)) DrawSTAFF(doc, pL, paper, context, TOPSYS_STAFF, False);
				drawAll = True;
				break;
			case CONNECTtype:
				if (VISIBLE(pL)) DrawCONNECT(doc, pL, context, TOPSYS_STAFF);
				break;
			case MEASUREtype:
				if (CheckZoom(doc)) return;				/* Each measure, look for zoom box hit */
				if (DrawCheckInterrupt(doc)) return;	/* ...and for menu cmd key equivs. */
				DrawMEASURE(doc, pL, context);
				pMeasure = GetPMEASURE(pL);
				if (SectRect(&pMeasure->measureBBox, &paperUpdate, &result)
								|| outputTo!=toScreen)
					drawAll = True;
				else
					drawAll = False;					/* Draw only spanning objects in this measure */
				break;
			case PSMEAStype:
				if (VISIBLE(pL) && drawAll) DrawPSMEAS(doc, pL, context);
				break;
			case CLEFtype:
				if (VISIBLE(pL) && drawAll) DrawCLEF(doc, pL, context);
				break;
			case KEYSIGtype:
				if (VISIBLE(pL) && drawAll) DrawKEYSIG(doc, pL, context);
				break;
			case TIMESIGtype:
				if (VISIBLE(pL) && drawAll) DrawTIMESIG(doc, pL, context);
				break;
			case SYNCtype:
				if (VISIBLE(pL) && drawAll) DrawSYNC(doc, pL, context);
				break;
			case GRSYNCtype:
				if (VISIBLE(pL) && drawAll) DrawGRSYNC(doc, pL, context);
				break;
			case BEAMSETtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result)) {
						if (BeamGRACE(pL))	DrawGRBEAMSET(doc, pL, context);
						else				DrawBEAMSET(doc, pL, context);
					}
				break;
			case TUPLETtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawTUPLET(doc, pL, context);
				break;
			case OTTAVAtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawOTTAVA(doc, pL, context);
				break;
			case DYNAMtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawDYNAMIC(doc, pL, context, True);
				break;
			case RPTENDtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawRPTEND(doc, pL, context);
				break;
			case ENDINGtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawENDING(doc, pL, context);
				break;
			case GRAPHICtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawGRAPHIC(doc, pL, context, True);
				break;
			case TEMPOtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawTEMPO(doc, pL, context, True);
				break;
			case SPACERtype:
				if (VISIBLE(pL) && drawAll) DrawSPACER(doc, pL, context);
				break;
			case SLURtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawSLUR(doc, pL, context);
				break;
			default:
				;
		}

	/* Paint everything with gray and then redraw the staves and Connect in black. */

grayPage:
	GrayFormatPage(doc, fromL, toL);

	for (pL=fromL; pL!=toL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case PAGEtype:
				/* See explanation of following code in DrawScoreRange. */
				measL = SSearch(pL, MEASUREtype, GO_RIGHT);
				GetAllContexts(doc, context, measL);
				ContextPage(doc, pL, context);
				DrawPAGE(doc, pL, paper, context);
				drawAll = True;
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);
				
				/* Convert systemRect to window-relative pixels, and check if it
				   intersects with update area. */
				   
				D2Rect(&pSystem->systemRect, &r);
				OffsetRect(&r, paper->left, paper->top);
				if ((VISIBLE(pL) && SectRect(&r, updateRect, &result))
													|| outputTo!=toScreen) {
					DrawSYSTEM(doc, pL, paper, context);
					if (doc->frameSystems) FrameSysRect(&r, False);			/* For debugging */
				}
				else {
					/* Skip ahead to next system: this one's not visible */
					
					pL = pSystem->rSystem;
					if (pL==NILINK || IsAfter(toL, pL)) pL = toL;
					pL = LeftLINK(pL);					/* Rewind for for loop above */
				}
				drawAll = True;
				break;
			case STAFFtype:
				if (VISIBLE(pL)) DrawSTAFF(doc, pL, paper, context, TOPSYS_STAFF, True);
				drawAll = True;
				break;
			case CONNECTtype:
				if (VISIBLE(pL)) DrawCONNECT(doc, pL, context, TOPSYS_STAFF);
				break;
			default:
				break;
		}
}


static void DrawScoreRange(Document *doc, LINK fromL, LINK toL, CONTEXT context[],
								Rect *paper, Rect *updateRect)
{
	LINK		pL, measL;
	PSYSTEM 	pSystem;
	PMEASURE	pMeasure;
	Rect 		r, result,
				paperUpdate;			/* Paper-relative update rect */
	Boolean		drawAll=True;			/* False if we're drawing only measure-spanning objects */
	
	paperUpdate = *updateRect;
	OffsetRect(&paperUpdate, -paper->left, -paper->top);
	
	if (DETAIL_SHOW)
		LogPrintf(LOG_DEBUG, "DrawScoreRange: fromL=%u toL=%u outputTo=%d\n", fromL, toL, outputTo);
	
	for (pL=fromL; pL!=toL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case PAGEtype:
				/*
				 * We have to find the first Measure after this Page to get context at
				 *	in order to have a complete valid context before we get to drawing
				 * that Measure; otherwise we'll have problems with Graphics before the
				 * first Measure. (It would be better for GetAllContexts (and GetContext)
				 * to do this themselves, but that needs more careful thinking.)
				 */
				measL = SSearch(pL, MEASUREtype, GO_RIGHT);
				GetAllContexts(doc, context, measL);
				ContextPage(doc, pL, context);
				DrawPAGE(doc, pL, paper, context);
				drawAll = True;
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);
				
				/* Convert systemRect to window-relative pixels, and check if it
				   intersects with update area. */
				   
				D2Rect(&pSystem->systemRect, &r);
				OffsetRect(&r, paper->left, paper->top);
				if ((VISIBLE(pL) && SectRect(&r, updateRect, &result))
													|| outputTo!=toScreen) {
					DrawSYSTEM(doc, pL, paper, context);
					if (doc->frameSystems) FrameSysRect(&r, False);	/* For debugging */
				}
				else {
					/* Skip ahead to next system: this one's not visible */
					
					pL = pSystem->rSystem;
					if (pL==NILINK || IsAfter(toL, pL)) pL = toL;
					pL = LeftLINK(pL);					/* Rewind for for loop above */
				}
				drawAll = True;
				break;
			case STAFFtype:
				if (VISIBLE(pL)) DrawSTAFF(doc, pL, paper, context, BACKGROUND_STAFF, False);
				drawAll = True;
				break;
			case CONNECTtype:
				if (VISIBLE(pL)) DrawCONNECT(doc, pL, context, TOPSYS_STAFF);	/* FIXME: BACKGROUND_STAFF?? */
				break;
			case MEASUREtype:
				if (CheckZoom(doc)) return;				/* Each measure, look for zoom box hit */
				if (DrawCheckInterrupt(doc)) return;	/* ...and for menu cmd key equivs. */
				DrawMEASURE(doc, pL, context);
				pMeasure = GetPMEASURE(pL);
				if (SectRect(&pMeasure->measureBBox, &paperUpdate, &result)
									|| outputTo!=toScreen)
					drawAll = True;
				else
					drawAll = False;					/* Draw only spanning objects in this measure */	
				break;
			case PSMEAStype:
				if (VISIBLE(pL) && drawAll) DrawPSMEAS(doc, pL, context);
				break;
			case CLEFtype:
				if (VISIBLE(pL) && drawAll) DrawCLEF(doc, pL, context);
				break;
			case KEYSIGtype:
				if (VISIBLE(pL) && drawAll) DrawKEYSIG(doc, pL, context);
				break;
			case TIMESIGtype:
				if (VISIBLE(pL) && drawAll) DrawTIMESIG(doc, pL, context);
				break;
			case SYNCtype:
				if (VISIBLE(pL) && drawAll) DrawSYNC(doc, pL, context);
				break;
			case GRSYNCtype:
				if (VISIBLE(pL) && drawAll) DrawGRSYNC(doc, pL, context);
				break;
			case BEAMSETtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result)) {
						if (BeamGRACE(pL))	DrawGRBEAMSET(doc, pL, context);
						else				DrawBEAMSET(doc, pL, context);
					}
				break;
			case TUPLETtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawTUPLET(doc, pL, context);
				break;
			case OTTAVAtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawOTTAVA(doc, pL, context);
				break;
			case DYNAMtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawDYNAMIC(doc, pL, context, True);
				break;
			case RPTENDtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawRPTEND(doc, pL, context);
				break;
			case ENDINGtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawENDING(doc, pL, context);
				break;
			case GRAPHICtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawGRAPHIC(doc, pL, context, True);
				break;
			case TEMPOtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawTEMPO(doc, pL, context, True);
				break;
			case SPACERtype:
				if (VISIBLE(pL) && drawAll) DrawSPACER(doc, pL, context);
				break;
			case SLURtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
								|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawSLUR(doc, pL, context);
				break;
			default:
				;
		}
}

/*
 *	Hilite the range [fromL,toL) to reflect the selection status of the objects in
 * the range.
 */

static void HiliteScoreRange(Document *doc, LINK fromL, LINK toL, CONTEXT context[],
										Rect *paper, Rect *updateRect)
{
	LINK		pL, measL;
	PSYSTEM 	pSystem;
	PMEASURE	pMeas;
	Rect 		r, result,
				paperUpdate;		/* Paper-relative update rect */
	Boolean	drawAll=True;			/* False if we're only drawing measure-spanning objects */
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};
	Point		enlargeNR;
	
	SetPt(&enlargeNR, config.enlargeNRHiliteH, config.enlargeNRHiliteV);

	paperUpdate = *updateRect;
	OffsetRect(&paperUpdate, -paper->left, -paper->top);
	
	for (pL=fromL; pL!=toL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case PAGEtype:
				/* See explanation of following code in DrawScoreRange. */
				
				measL = SSearch(pL, MEASUREtype, GO_RIGHT);
				GetAllContexts(doc, context, measL);
				ContextPage(doc, pL, context);
				drawAll = True;
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);

				/* Convert systemRect to window-relative pixels, and check if it
				   intersects with update area. */
				   
				D2Rect(&pSystem->systemRect, &r);
				OffsetRect(&r, paper->left, paper->top);
				if ((VISIBLE(pL) && SectRect(&r, updateRect, &result)) || outputTo!=toScreen)
					ContextSystem(pL, context);
				else {
					/* Skip ahead to next system: this one's not visible */
					
					pL = LinkRSYS(pL);
					if (pL==NILINK || IsAfter(toL, pL)) pL = toL;
					pL = LeftLINK(pL);					/* Rewind for for loop above */
				}
				drawAll = True;
				break;
			case STAFFtype:
				drawAll = True;
				ContextStaff(pL, context);
				break;
			case CONNECTtype:
				break;
			case MEASUREtype:
				if (CheckZoom(doc)) return;				/* Each measure, look for zoom box hit */
				if (DrawCheckInterrupt(doc)) return;	/* ...and for menu cmd key equivs. */
				
				ContextMeasure(pL, context);

				if (LinkSEL(pL))
					CheckMEASURE(doc, pL, context, NULL, SMHilite, stfRange, enlarge);

				pMeas = GetPMEASURE(pL);
				if (SectRect(&pMeas->measureBBox, &paperUpdate, &result) || outputTo!=toScreen)
						drawAll = True;
				else	drawAll = False;				/* Draw only spanning objects in this measure */
				break;
			case PSMEAStype:
				if (VISIBLE(pL) && drawAll)
					if (LinkSEL(pL))
						CheckPSMEAS(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case CLEFtype:
				if (VISIBLE(pL) && drawAll)
					if (LinkSEL(pL))
						CheckCLEF(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case KEYSIGtype:
				if (VISIBLE(pL) && drawAll)
					if (LinkSEL(pL))
						CheckKEYSIG(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case TIMESIGtype:
				if (VISIBLE(pL) && drawAll)
					if (LinkSEL(pL))
						CheckTIMESIG(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case SYNCtype:
				if (VISIBLE(pL) && drawAll)
					if (LinkSEL(pL))
						CheckSYNC(doc, pL, context, NULL, SMHilite, stfRange, enlargeNR, enlargeNR);
				break;
			case GRSYNCtype:
				if (VISIBLE(pL) && drawAll)
					if (LinkSEL(pL))
						CheckGRSYNC(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case BEAMSETtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL),&paperUpdate,&result)) {
						if (BeamGRACE(pL) && LinkSEL(pL))
							CheckBEAMSET(doc, pL, context, NULL, SMHilite, stfRange);
						else if (LinkSEL(pL))
							CheckBEAMSET(doc, pL, context, NULL, SMHilite, stfRange);
					}
				break;
			case TUPLETtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL),&paperUpdate,&result))
						if (LinkSEL(pL))
							CheckTUPLET(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case OTTAVAtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL),&paperUpdate,&result))
						if (LinkSEL(pL))
							CheckOTTAVA(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case DYNAMtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						if (LinkSEL(pL))
							CheckDYNAMIC(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case RPTENDtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						if (LinkSEL(pL))
							CheckRPTEND(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case ENDINGtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						if (LinkSEL(pL))
							CheckENDING(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case GRAPHICtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						if (LinkSEL(pL))
							CheckGRAPHIC(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case TEMPOtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						if (LinkSEL(pL))
							CheckTEMPO(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case SPACERtype:
				if (VISIBLE(pL) && drawAll)
					if (LinkSEL(pL))
						CheckSPACER(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
				break;
			case SLURtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						if (LinkSEL(pL))
							CheckSLUR(doc, pL, context, NULL, SMHilite, stfRange);
				break;
			default:
				;
		}
}


/* ---------------------------------------------------------------------- SetMusicPort -- */
/* Set general characteristics of the port, presumably in preparation for drawing
the score. Set the font, the font size, and the magnification globals, and build the
charRect cache. NB: if staves are not all the same size, this approach will have
problems! Cf. SetTextSize. However, Nightingale sets the font size before drawing
many symbols anyway, effectively overriding the size this sets. This should be
cleaned up, of course. */

static void SetMusicPort(Document *doc)
{
	TextFont(doc->musicFontNum);
	SetTextSize(doc);

	InstallMagnify(doc);

	BuildCharRectCache(doc);
}


/* ------------------------------------------------------------------------- DrawRange -- */
/*	Draw all objects and subobjects in the specified range that
	1. are in measures that overlap the updateRect, and
	2. are supposed to be visible, i.e., have their "visible" flags set.
In measures that aren't visible, it still searches thru the measure and
draws objects that might be visible because they span across to another
measure. */

void DrawRange(Document *doc, LINK fromL, LINK toL, Rect *paper, Rect *updateRect)
{
	CONTEXT context[MAXSTAVES+1];
	short oldCurrSheet;
	Rect oldCurrPaper;
	
	SetMusicPort(doc);				/* Set the font, fontSize, and build the charRect cache. */
	
	if (!PageTYPE(fromL))
		GetAllContexts(doc, context, fromL);	/* else it will get called in first pass through loop below */
	
	oldCurrSheet = doc->currentSheet;
	oldCurrPaper = doc->currentPaper;

	if (doc->masterView)
		DrawMasterRange(doc, fromL, toL, context, paper, updateRect);
	else if (doc->showFormat)
		DrawFormatRange(doc, fromL, toL, context, paper, updateRect);
	else {
		DrawScoreRange(doc, fromL, toL, context, paper, updateRect);
		if (doc!=clipboard)
			HiliteScoreRange(doc, fromL, toL, context, paper, updateRect);
	}

	oldCurrSheet = doc->currentSheet;
	oldCurrPaper = doc->currentPaper;
}
