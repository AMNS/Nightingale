/***************************************************************************
*	FILE:	DrawHighLevel.c																	*
*	PROJ:	Nightingale, rev. for v.99														*
*	DESC:	Routines	for drawing object-list ranges									*
***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void FrameSysRect(Rect *, Boolean);
static void DrawMasterSystem(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *, INT16);
static void DrawMasterRange(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *);
static void DrawFormatRange(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *);
static void DrawScoreRange(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *);
static void HiliteScoreRange(Document *, LINK, LINK, CONTEXT [], Rect *, Rect *);
static void SetMusicPort(Document *);


/* ---------------------------------------------------------------- FrameSysRect -- */

void FrameSysRect(
					Rect *r,
					Boolean /*solid*/			/* unused */
					)
{
#if 0
	PenPat(solid? NGetQDGlobalsBlack() : diagonalDkGray);
#else
	PenPat(NGetQDGlobalsBlack());
#endif
	FrameRect(r);
	PenNormal();
}


/* -------------------------------------------------------------- DrawPageContent -- */
/* Draw all objects in the specified page. */

void DrawPageContent(Document *doc, INT16 sheetNum, Rect *paper, Rect *updateRect)
{
	PPAGE pPage; LINK	pageL;
	
	if (pageL = LSSearch(doc->headL, PAGEtype, sheetNum, FALSE, FALSE)) {
		pPage = GetPPAGE(pageL);
		DrawRange(doc, pageL, pPage->rPage ? pPage->rPage : doc->tailL, paper, updateRect);
	}
}

/* ------------------------------------------------------------- DrawMasterSystem -- */
/* Draw the objects in the given range, which should be the Master Page system. If
drawing the system the first time, the page object needs to be included in the
range to be drawn. */

static void DrawMasterSystem(Document *doc, LINK fromL, LINK toL, CONTEXT context[],
Rect *paper, Rect *updateRect, INT16 sysNum)
{
	LINK pL; Rect r,result,paperUpdate;
	PSYSTEM pSystem;
	INT16 ground;

	paperUpdate = *updateRect;
	OffsetRect(&paperUpdate,-paper->left,-paper->top);
	ground = (sysNum==1? TOPSYS_STAFF :
					(sysNum==2? SECONDSYS_STAFF :
						OTHERSYS_STAFF ));
	
	for (pL=fromL; pL!=toL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case PAGEtype:
				GetAllContexts(doc, context, pL);
				ContextPage(doc, pL, context);
				DrawPAGE(doc, pL, paper, context);
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);
				/* Convert systemRect to paper-relative pixels */
				D2Rect(&pSystem->systemRect, &r);
				/* Convert to window-relative and check for intersection with update area */
				OffsetRect(&r,paper->left,paper->top);
				
				if (VISIBLE(pL) && SectRect(&r,updateRect,&result) || outputTo!=toScreen) {
					DrawSYSTEM(doc, pL, paper, context);
					if (doc->frameSystems) FrameSysRect(&r, FALSE);	/* For debugging */
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
	LINK sysL, staffL; DRect oldSysRect; Rect r;
	DDIST sysHeight, sysOffset;
	INT16 sysNum=0;

	/* If the score has no parts, return without drawing anything. */

	staffL = SSearch(doc->masterHeadL,STAFFtype,GO_RIGHT);
	if (LinkNENTRIES(staffL)==0) return;

	/* Draw the single "real" master system. */
	
	DrawMasterSystem(doc,doc->masterHeadL,doc->masterTailL,context,paper,
							updateRect,++sysNum);

	sysL = LSSearch(doc->masterHeadL, SYSTEMtype, ANYONE, FALSE, FALSE);
	oldSysRect = SystemRECT(sysL);

	sysHeight = oldSysRect.bottom - oldSysRect.top;
	sysOffset = sysHeight;
	
	/*
	 * Draw as many additional copies of the master system as fit on a page. Be
	 *	careful of overflow.
	 */
	if ((long)SystemRECT(sysL).bottom+(long)sysOffset<=32767L) {
		OffsetDRect(&SystemRECT(sysL), 0, sysOffset);
		while (MasterRoomForSystem(doc,sysL)) {
			DrawMasterSystem(doc,sysL,doc->masterTailL,context,paper,updateRect,++sysNum);
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
	PMEASURE pMeasure;
	Rect 		r,result,
				paperUpdate;			/* Paper-relative update rect */
	Boolean	drawAll=TRUE;			/* FALSE if we're only drawing measure-spanning objects */
	
	paperUpdate = *updateRect;
	OffsetRect(&paperUpdate,-paper->left,-paper->top);
	if (doc->enterFormat) goto grayPage;
	
	for (pL=fromL; pL!=toL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case PAGEtype:
				/* See explanation of following code in DrawScoreRange. */
				measL = SSearch(pL, MEASUREtype, GO_RIGHT);
				GetAllContexts(doc, context, measL);
				ContextPage(doc, pL, context);
				DrawPAGE(doc, pL, paper, context);
				drawAll = TRUE;
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);
				/* Convert systemRect to paper-relative pixels */
				D2Rect(&pSystem->systemRect, &r);
				/* Convert to window-relative and check for intersection with update area */
				OffsetRect(&r,paper->left,paper->top);
				
				if (VISIBLE(pL) && SectRect(&r, updateRect, &result)
														|| outputTo!=toScreen) {
					DrawSYSTEM(doc, pL, paper, context);
					if (doc->frameSystems) FrameSysRect(&r, TRUE);
				}
				else {
					/* Skip ahead to next system: this one's not visible */
					pL = pSystem->rSystem;
					if (pL==NILINK || IsAfter(toL, pL)) pL = toL;
					/* Rewind for increment part of for loop above */
					pL = LeftLINK(pL);
				}
				drawAll = TRUE;
				break;
			case STAFFtype:
				if (VISIBLE(pL)) DrawSTAFF(doc, pL, paper, context, TOPSYS_STAFF, FALSE);
				drawAll = TRUE;
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
					drawAll = TRUE;
				else
					drawAll = FALSE;		/* draw only spanning objects in this measure */
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
						if (GraceBEAM(pL))
							DrawGRBEAMSET(doc, pL, context);
						else
							DrawBEAMSET(doc, pL, context);
					}
				break;
			case TUPLETtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
									|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawTUPLET(doc, pL, context);
				break;
			case OCTAVAtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
									|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawOCTAVA(doc, pL, context);
				break;
			case DYNAMtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
									|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawDYNAMIC(doc, pL, context, TRUE);
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
						DrawGRAPHIC(doc, pL, context, TRUE);
				break;
			case TEMPOtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
									|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawTEMPO(doc, pL, context, TRUE);
				break;
			case SPACEtype:
				if (VISIBLE(pL) && drawAll) DrawSPACE(doc, pL, context);
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
				drawAll = TRUE;
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);
				/* Convert systemRect to paper-relative pixels */
				D2Rect(&pSystem->systemRect, &r);
				/* Convert to window-relative and check for intersection with update area */
				OffsetRect(&r,paper->left,paper->top);
				
				if (VISIBLE(pL) && SectRect(&r,updateRect,&result) || outputTo!=toScreen) {
					DrawSYSTEM(doc, pL, paper, context);
					if (doc->frameSystems) FrameSysRect(&r, FALSE);			/* For debugging */
				}
				else {
					/* Skip ahead to next system: this one's not visible */
					pL = pSystem->rSystem;
					if (pL==NILINK || IsAfter(toL, pL)) pL = toL;
					/* Rewind for increment part of for loop above */
					pL = LeftLINK(pL);
				}
				drawAll = TRUE;
				break;
			case STAFFtype:
				if (VISIBLE(pL)) DrawSTAFF(doc, pL, paper, context, TOPSYS_STAFF, TRUE);
				drawAll = TRUE;
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
	PMEASURE pMeasure;
	Rect 		r,result,
				paperUpdate;			/* Paper-relative update rect */
	Boolean	drawAll=TRUE;			/* FALSE if we're only drawing measure-spanning objects */
	
	paperUpdate = *updateRect;
	OffsetRect(&paperUpdate,-paper->left,-paper->top);
	
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
				drawAll = TRUE;
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);
				/* Convert systemRect to paper-relative pixels */
				D2Rect(&pSystem->systemRect, &r);
				/* Convert to window-relative and check for intersection with update area */
				OffsetRect(&r,paper->left,paper->top);
				
				if (VISIBLE(pL) && SectRect(&r, updateRect, &result) || outputTo!=toScreen) {
					DrawSYSTEM(doc, pL, paper, context);
					if (doc->frameSystems) FrameSysRect(&r, FALSE);	/* For debugging */
				}
				else {
					/* Skip ahead to next system: this one's not visible */
					pL = pSystem->rSystem;
					if (pL==NILINK || IsAfter(toL, pL)) pL = toL;
					/* Rewind for increment part of for loop above */
					pL = LeftLINK(pL);
				}
				drawAll = TRUE;
				break;
			case STAFFtype:
				if (VISIBLE(pL)) DrawSTAFF(doc, pL, paper, context, BACKGROUND_STAFF, FALSE);
				drawAll = TRUE;
				break;
			case CONNECTtype:
				if (VISIBLE(pL)) DrawCONNECT(doc, pL, context, TOPSYS_STAFF);	/* ??BACKGROUND_STAFF?? */
				break;
			case MEASUREtype:
				if (CheckZoom(doc)) return;				/* Each measure, look for zoom box hit */
				if (DrawCheckInterrupt(doc)) return;	/* ...and for menu cmd key equivs. */
				DrawMEASURE(doc, pL, context);
				pMeasure = GetPMEASURE(pL);
				if (SectRect(&pMeasure->measureBBox, &paperUpdate, &result)
														|| outputTo!=toScreen)
					drawAll = TRUE;
				else
					drawAll = FALSE;		/* draw only spanning objects in this measure */	
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
						if (GraceBEAM(pL))
							DrawGRBEAMSET(doc, pL, context);
						else
							DrawBEAMSET(doc, pL, context);
					}
				break;
			case TUPLETtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
									|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawTUPLET(doc, pL, context);
				break;
			case OCTAVAtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL)
									|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawOCTAVA(doc, pL, context);
				break;
			case DYNAMtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
									|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawDYNAMIC(doc, pL, context, TRUE);
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
						DrawGRAPHIC(doc, pL, context, TRUE);
				break;
			case TEMPOtype:
				if (VISIBLE(pL))
					if (drawAll || !LinkVALID(pL)
									|| SectRect(&LinkOBJRECT(pL), &paperUpdate, &result))
						DrawTEMPO(doc, pL, context, TRUE);
				break;
			case SPACEtype:
				if (VISIBLE(pL) && drawAll) DrawSPACE(doc, pL, context);
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
	PMEASURE pMeas;
	Rect 		r,result,
				paperUpdate;			/* Paper-relative update rect */
	Boolean	drawAll=TRUE;			/* FALSE if we're only drawing measure-spanning objects */
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};
	Point		enlargeNR;
	
	SetPt(&enlargeNR, config.enlargeNRHiliteH, config.enlargeNRHiliteV);

	paperUpdate = *updateRect;
	OffsetRect(&paperUpdate,-paper->left,-paper->top);
	
	for (pL=fromL; pL!=toL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case PAGEtype:
				/* See explanation of following code in DrawScoreRange. */
				measL = SSearch(pL, MEASUREtype, GO_RIGHT);
				GetAllContexts(doc, context, measL);
				ContextPage(doc, pL, context);
				drawAll = TRUE;
				break;
			case SYSTEMtype:
				pSystem = GetPSYSTEM(pL);
				/* Convert systemRect to paper-relative pixels */
				D2Rect(&pSystem->systemRect, &r);

				/* Convert to window-relative and check for intersection with update area */
				OffsetRect(&r,paper->left,paper->top);
				
				if (VISIBLE(pL) && SectRect(&r,updateRect,&result) || outputTo!=toScreen)
					ContextSystem(pL, context);
				else { 															/* Skip ahead to next system: this one's not visible */
					pL = LinkRSYS(pL);
					if (pL==NILINK || IsAfter(toL, pL)) pL = toL;	/* Rewind for increment part of for loop above */
					pL = LeftLINK(pL);
				}
				drawAll = TRUE;
				break;
			case STAFFtype:
				drawAll = TRUE;
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
				if (SectRect(&pMeas->measureBBox,&paperUpdate,&result) || outputTo!=toScreen)
						drawAll = TRUE;
				else	drawAll = FALSE;						/* draw only spanning objects in this measure */
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
						if (GraceBEAM(pL) && LinkSEL(pL))
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
			case OCTAVAtype:
				if (VISIBLE(pL) && !doc->pianoroll)
					if (drawAll || !LinkVALID(pL) ||
							SectRect(&LinkOBJRECT(pL),&paperUpdate,&result))
						if (LinkSEL(pL))
							CheckOCTAVA(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
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
			case SPACEtype:
				if (VISIBLE(pL) && drawAll)
					if (LinkSEL(pL))
						CheckSPACE(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
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


/* ---------------------------------------------------------------- SetMusicPort -- */
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


/* ------------------------------------------------------------------ DrawRange -- */
/*	Draw all objects and subobjects in the specified range that
	1. are in measures that overlap the updateRect, and
	2. are supposed to be visible, i.e., have their "visible" flags set.
In measures that aren't visible, it still searches thru the measure and
draws objects that might be visible because they span across to another
measure. */

void DrawRange(Document *doc, LINK fromL, LINK toL, Rect *paper, Rect *updateRect)
{
	CONTEXT context[MAXSTAVES+1];
	INT16 oldCurrSheet;
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
