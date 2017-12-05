/******************************************************************************************
*	FILE:	Check.c
*	PROJ:	Nightingale
*	DESC:	Selection-related routines.
		CheckPAGE			CheckSYSTEM			CheckSTAFF
		CheckCONNECT
		CheckCLEF			CheckDYNAMIC		CheckRPTEND
		CheckGRAPHIC		CheckTEMPO			CheckSPACER
		CheckENDING
		CheckKEYSIG			CheckSYNC			CheckGRSYNC
		CheckTIMESIG		CheckMEASURE		CheckBEAMSET
		CheckTUPLET			CheckOTTAVA			CheckSLUR
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* The Check routines do different things depending on their <mode> parameter:
		mode=SMClick	Determines whether *(Point *)ptr is inside any subobject(s)
						(or objects if the object type has no subobjects), toggling
						selection status and	highliting as appropriate.
		=SMDblClick		Open the subobject for editing, if it's editable.
		=SMDrag			Determines whether *(Rect *)ptr encloses any subobject(s);
						selects and highlites as appropriate.
		=SMStaffDrag	Determines whether *(Rect *)ptr encloses any subobject(s)
						on staves within stfRange; selects and highlites as appropriate.
						FIXME: In calls to these routines from FindAndActOnObject(),
						stfRange is undefined!
		=SMSelect		Selects and highlites all subobjects.
		=SMSelectRange	Selects and highlites all subobjects within stfRange.
		=SMSelNoHilite	Selects all subobjects but does no highliting.
		=SMDeselect		Deselects and unhighlites all subobjects.
		=SMHilite		Highlites all selected subobjects.
		=SMSymDrag		Determines whether *(Point *)ptr is inside any subobject(s),
	                 	calling HandleSymDrag if so.
	Not all routines handle all modes, and a couple of routines have an additional
	mode, SMThread.
	
	These routines should normally be called in response to a mouse click.

	For objects with subobjects, in most cases, return value is either NOMATCH or the
	subobject sequence number (NOT staff or voice number!) of the subobject found.

	FIXME: These functions should check <valid> flags before doing a lot of their work:
	e.g., SMDeselect can clear flags but shouldn't HiliteRect if it's not <valid>. */


/* Offset the Rect along the given axis only if it's not set to "infinite" (wide
open); if it is, leave it alone. */

static void COffsetRect(Rect *, short, short);
static void COffsetRect(Rect *r, short h, short v)
{
	if (r->left>-32767 && r->right<32767) {
		r->left += h;
		r->right += h;
	}
	if (r->top>-32767 && r->bottom<32767) {
		r->top += v;
		r->bottom += v;
	}
}


/* ------------------------------------------------------------------------- CheckPAGE -- */
/*	PAGE object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckPAGE(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE /*stfRange*/,	/* unused */
						Point enlarge)
{
	Rect		r;
	PCONTEXT	pContext;
	short		result;

	result = NOMATCH;
	
	if (VISIBLE(pL)) {
		pContext = &context[1];
		r = pContext->paper;
		OffsetRect(&r,-pContext->paper.left,-pContext->paper.top);

		switch (mode) {
			case SMClick:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
					result = 0;
				}
				break;
			case SMDblClick:
				/* Double click goes here */
				break;
			case SMDrag:
				/* No select rectangle for PAGEs */
				break;
			case SMStaffDrag:
				/* No staff selection for PAGEs */
				break;
			case SMSelect:
				if (!LinkSEL(pL)) {
					LinkSEL(pL) = True;
					HiliteRect(&r);
				}
				break;
			case SMSelNoHilite:
				LinkSEL(pL) = True;
				break;
			case SMDeselect:
				if (LinkSEL(pL)) {
					LinkSEL(pL) = False;
					HiliteRect(&r);
				}
				break;
			case SMSymDrag:
				/* No Symbol dragging for PAGEs */
				break;
			case SMFind:
				if (PtInRect(*(Point *)ptr, &r))
					result = 0;
				break;
			case SMHilite:
				if (LinkSEL(pL)) {
					InsetRect(&r, -enlarge.h, -enlarge.v);
					HiliteRect(&r);
				}
				break;
			case SMFlash:
				HiliteRect(&r);
				break;
			default:
				;
		}
	}
	return result;
}


/* ----------------------------------------------------------------------- CheckSYSTEM -- */
/* SYSTEM object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckSYSTEM(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE /*stfRange*/,	/* unused */
						Point enlarge)
{
	PSYSTEM		p;
	Rect		r, paperRect, sysRect, sysObjRect;
	Point		mousePt;
	PCONTEXT	pContext;
	short		result;
	LINK		pageL;
	DDIST		sysOffset;

	p = GetPSYSTEM(pL);
	pageL = SSearch(pL, PAGEtype, True);
	result = NOMATCH;
	if (VISIBLE(pL)) {
		pContext = &context[1];

		/* Convert systemRect to paper-relative pixels */
		D2Rect(&p->systemRect, &r);
		GetSheetRect(doc, SheetNUM(pageL), &paperRect);
		OffsetRect(&r, paperRect.left, paperRect.top);

		switch (mode) {
			case SMClick:
				/* Get the bBox of all the staff subObjRects in the system */
				sysRect = ComputeSysRect(pL, paperRect, pContext);
				mousePt = *(Point *)ptr;
				mousePt.v += pContext->paper.top;
				mousePt.h += pContext->paper.left;

				if (doc->masterView) {
					sysOffset = r.bottom-r.top + d2p(doc->yBetweenSysMP);

					/* Translate the bBox of all the staff subObjRects
						down by 1 system. */
					OffsetRect(&sysRect, 0, sysOffset);

					sysObjRect = LinkOBJRECT(pL);
					OffsetRect(&sysObjRect, 0, sysOffset);

					if (PtInRect(mousePt, &sysRect))
						if (DragGraySysRect(doc, pL, ptr, sysObjRect, sysRect, r, pContext))
							result = 0;
				}
				else if (doc->showFormat) {
					sysObjRect = LinkOBJRECT(pL);
					
					if (EditSysRect(doc,*(Point *)ptr,pL)) {
						FixMeasRectYs(doc, NILINK, False, False, False);
						doc->locFmtChanged = doc->changed = True;
						result = 0;
					}
					else if (PtInRect(mousePt, &sysRect))
						if (DragGraySysRect(doc, pL, ptr, sysObjRect, sysRect, r, pContext))
							result = 0;
				}
				break;
			case SMDblClick:
				/* No double click for SYSTEMs */
				break;
			case SMDrag:
				/* Select rectangle for SYSTEMs goes here */
				break;
			case SMStaffDrag:
				/* No staff selection for SYSTEMs */
				break;
			case SMSelect:
				if (!LinkSEL(pL)) {
					LinkSEL(pL) = True;
					HiliteRect(&r);
				}
				break;
			case SMSelNoHilite:
				LinkSEL(pL) = True;
				break;
			case SMDeselect:
				if (LinkSEL(pL)) {
					LinkSEL(pL) = False;
					HiliteRect(&r);
				}
				break;
			case SMSymDrag:
				/* No Symbol dragging for SYSTEMs */
				break;
			case SMFind:
				{
#ifdef CHECK_FRAMESYSTEMS
					/* Convert to window-relative and frame rect for debugging */
					OffsetRect(&r,pContext->paper.left,pContext->paper.top);
					FrameRect(&r);
#endif
				}
				D2Rect(&p->systemRect, &r);
				InsetRect(&r, -enlarge.h, -enlarge.v);
				if (PtInRect(*(Point *)ptr, &r))
					result = 0;
				break;
			case SMHilite:
				if (LinkSEL(pL)) {
					InsetRect(&r, -enlarge.h, -enlarge.v);
					HiliteRect(&r);
				}
				break;
			case SMFlash:
				HiliteRect(&r);
				break;
			default:
				;
		}
	}
	return result;
}

/* ------------------------------------------------------------------------ CheckSTAFF -- */
/* STAFF object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckSTAFF(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	LINK aStaffL;
	PCONTEXT pContext;
	Rect rSub, wSub, aRect;
	short i, result, staffn;
	DDIST	dTop, dLeft, dBottom, dRight;
	Boolean objSel=False;
	LINK partL;

	/* In some cases, toggle hiliting and deselect staves first. FIXME: This should either
		be moved out to the calling functions, or be controlled by a new parameter. */
		
	if (doc->showFormat && (mode!=SMHilite && mode!=SMSelectRange))
		HiliteAllStaves(doc, True);

	if (doc->masterView && (mode!=SMHilite && mode!=SMSelectRange))
		HiliteStaves(doc, pL, context, True);

	result = NOMATCH;
	if (VISIBLE(pL)) {
		aStaffL = FirstSubLINK(pL);
		for (i = 0; aStaffL; i++, aStaffL=NextSTAFFL(aStaffL)) {
			if (StaffVIS(aStaffL)) {

				staffn = StaffSTAFF(aStaffL);
				pContext = &context[staffn];
				dTop = pContext->staffTop;
				dLeft = pContext->staffLeft;
				dBottom = dTop + pContext->staffHeight;
				dRight = pContext->staffRight;
	
				SetRect(&rSub, d2p(dLeft), d2p(dTop), d2p(dRight), d2p(dBottom));
	
				wSub = rSub;
				OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
				
				switch (mode) {
					case SMClick:
						/* If we are editing the masterPage, and the click is not in the
							first staff subObj then call DragGrayRgn to track the user dragging
							the staff subObj up and down; or if we are in showFormat, and
							in any staff subObj. */
						if (doc->masterView || doc->showFormat) {

							if (DragGrayStaffRect(doc, pL, aStaffL, staffn, ptr, rSub, wSub, context)) {
								result = 0;
								if (doc->showFormat) {
									StaffSEL(aStaffL) = LinkSEL(pL) = True;
									doc->selStartL = pL; doc->selEndL = RightLINK(pL);
									HiliteStaves(doc, pL, context, False);
								}
								else if (doc->masterView) {
									SelectStaff(doc,pL,staffn);	/* Selects all staves in its part! */
									
									doc->selStartL = pL; doc->selEndL = RightLINK(pL);
									HiliteStaves(doc, pL, context, False);
								}
							}
						}
						break;
					case SMDblClick:
						/* Handle double click on staff object in Master Page. First, get the
							part which includes the double clicked staff. Select all staves of
							that part; then Hilite them before calling InstrDialog to prevent
							half a staff being unhilited after exiting it. Finally call
							InstrDialog to edit instrument characteristics of part. N.B.
							Instead of doing all this itself, it should probably drop
							thru to case SMClick, like all the other Check routines. */
							
						if (doc->masterView && (PtInRect(*(Point *)ptr, &rSub))) {
							partL = SelectStaff(doc,pL,staffn);				/* Selects all staves in its part! */

							HiliteStaves(doc, pL, context, False);				/* Call this here to prevent chunky hiliting */
							if (SDInstrDialog(doc,PartFirstSTAFF(partL))) {
								doc->masterChanged = True;
								InvalWindow(doc);
							}
							
							doc->selStartL = pL; doc->selEndL = RightLINK(pL);
						}						
						break;
					case SMDrag:
						/* If the rect the user drags out touches a staff subobject,
							select that staff and any others in its part, and hilite them. */

						if (SectRect(&rSub, (Rect *)ptr, &aRect)) {
							SelectStaff(doc,pL,staffn);			/* Selects all staves in its part! */
							result = i;
						}
						break;
					case SMStaffDrag:
						/* No staff selection for STAFFs */
						break;
					case SMSelect:
						if (!StaffSEL(aStaffL)) {
							StaffSEL(aStaffL) = True;
							HiliteRect(&wSub);
						}
						break;
					case SMSelectRange:
						if (!StaffSEL(aStaffL) && staffn>=stfRange.topStaff && 
					 			staffn<=stfRange.bottomStaff) {
							StaffSEL(aStaffL) = True;
							HiliteRect(&wSub);
						}
						break;
					case SMSelNoHilite:
						LinkSEL(pL) = True;
						break;
					case SMDeselect:
						if (StaffSEL(aStaffL)) {
							StaffSEL(aStaffL) = False;
							HiliteRect(&wSub);
						}
						break;
					case SMSymDrag:
						/* No Symbol dragging for STAFFs */
						break;
					case SMFind:
						if (PtInRect(*(Point *)ptr, &rSub))
							result = i;
						break;
					case SMHilite:
						if (StaffSEL(aStaffL)) {
							InsetRect(&wSub, -enlarge.h, -enlarge.v);
							HiliteRect(&wSub);
						}
						break;
					case SMFlash:
						HiliteRect(&wSub);
						break;
					default:
						;
				}
				if (StaffSEL(aStaffL))
					objSel = True;
			}
		}
	}
	if (mode!=SMFind) LinkSEL(pL) = objSel;
	if (mode==SMDrag) HiliteStaves(doc, pL, context, False);	/* Call this here to prevent chunky hiliting */
	return result;
}


/* ---------------------------------------------------------------------- CheckCONNECT -- */
/* CONNECT object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckCONNECT(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE /*stfRange*/,	/* unused */
						Point /*enlarge*/)
{
	PACONNECT aConnect;
	LINK aConnectL, staffL, aStaffL, cStaffL;
	Rect r, rSub, wSub, limitR, slopR;
	PCONTEXT pContext;
	short i, j, result, staffAbove, staffBelow;
	DDIST dTop, dLeft, xd, yd, dBottom, dRight, rSubRight;
	Boolean entire,objSel=False;
	RgnHandle connectRgn;  long newPos;
	Point dragPt;

	cStaffL = LSSearch(pL, STAFFtype, ANYONE, GO_LEFT, False);			/* must exist */

	result = NOMATCH;
	if (VISIBLE(pL)) {
		r = LinkOBJRECT(pL);

		aConnectL = FirstSubLINK(pL);
		for (i = 0; aConnectL; i++, aConnectL=NextCONNECTL(aConnectL)) {
			if (!ShouldDrawConnect(doc, pL, aConnectL, cStaffL)) continue;

			aConnect = GetPACONNECT(aConnectL);
			entire = aConnect->connLevel==0;
			pContext = entire ? &context[FirstStaffn(pL)] :
								&context[NextStaffn(doc, pL, True, aConnect->staffAbove)];
			dLeft = pContext->staffLeft;
			dTop = pContext->staffTop;

			pContext = entire ? &context[FirstStaffn(pL)] :
										&context[NextStaffn(doc, pL, False, aConnect->staffBelow)];
			dBottom = pContext->staffTop + pContext->staffHeight;
			dRight = pContext->staffRight;
	
			rSubRight = p2d(6);
			SetRect(&rSub, 0, d2p(dTop), d2p(rSubRight), d2p(dBottom));

			xd = dLeft + aConnect->xd;
			yd = dTop;
			OffsetRect(&rSub, d2p(xd), 0);

			SetRect(&wSub,d2p(dLeft),d2p(dTop),d2p(dRight),d2p(dBottom));
			OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
		
			switch (mode) {
				case SMClick:
					/* If we are editing the masterPage, and the click is in the first system
						of the masterPage, then call DragGrayRgn to track the user dragging
						the connect subObj up and down. */
					if ((doc->masterView || doc->showFormat) && !entire) {		/* Pass by system-level connects */
						if (1 /* aConnect->staffAbove>1 */ ) {			/* Can drag neither 1st part nor 1st stf */
							if (PtInRect(*(Point *)ptr, &rSub)) {
								connectRgn = NewRgn();
								RectRgn(connectRgn, &wSub);
								dragPt = *(Point *)ptr; dragPt.v += pContext->paper.top;

								/* Set the limit Rect and slop Rect for dragging. The dragging
									is limited to 1 staffLine below the staff above staffAbove,
									or 1 staffLine above the staff below staffBelow; if
									staffBelow is the last staff, there is no lower limit,
									since dragging the last staff downwards expands the
									systemRect and gives that staff all the room it needs. */

								limitR = wSub;
								InsetRect(&limitR, -4, 0);
								slopR = doc->currentPaper;

								staffAbove = NextStaffn(doc,pL,False,aConnect->staffAbove-1);
								if (staffAbove>1) {
									pContext = &context[staffAbove];
									limitR.top = d2p(pContext->staffTop+pContext->staffHeight)+
														pContext->paper.top+(dragPt.v-wSub.top)
														+d2p(LNSPACE(pContext));
								}
								else {
									limitR.top = doc->currentPaper.top;
								}
								staffBelow = NextStaffn(doc,pL,True,aConnect->staffBelow+1);
								if (staffBelow>0 && staffBelow<doc->nstaves) {
									pContext = &context[staffBelow];
									limitR.bottom = d2p(pContext->staffTop)+pContext->paper.top
															+(dragPt.v-wSub.bottom)
															-d2p(LNSPACE(pContext));
								}
								else {
									limitR.bottom = doc->currentPaper.bottom;
								}

								newPos = DragGrayRgn(connectRgn, dragPt, &limitR, &slopR,
												vAxisOnly, NULL);

								if (HiWord(newPos) && HiWord(newPos)!=0x8000)
									if (doc->masterView) {
										staffL = SSearch(doc->masterHeadL,STAFFtype,False);
										staffAbove = NextStaffn(doc,pL,True,aConnect->staffAbove);
										staffBelow = NextStaffn(doc,pL,False,aConnect->staffBelow);
										j = staffAbove;
										for ( ; j<=staffBelow; j++) {
											aStaffL = FirstSubLINK(staffL);
											for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
												if (StaffSTAFF(aStaffL)==j)
													UpdateDraggedStaff(doc, staffL, aStaffL, newPos);
										}
										doc->stfChangedMP = True;
									}
									else if (doc->showFormat) {
										staffL = SSearch(pL,STAFFtype,True);
										staffAbove = NextStaffn(doc,pL,True,aConnect->staffAbove);
										staffBelow = NextStaffn(doc,pL,False,aConnect->staffBelow);
										UpdateFormatConnect(doc, staffL,
																staffAbove, staffBelow, newPos);
									}

								result = 0;
							}
						}
					}
					break;
				case SMDblClick:
					/* Double click goes here */
					break;
				case SMDrag:
					/* Select rectangle for CONNECTs goes here */
					break;
				case SMStaffDrag:
					/* No staff selection for CONNECTs */
					break;
				case SMSelect:
					if (!ConnectSEL(aConnectL)) {
						ConnectSEL(aConnectL) = True;
						HiliteRect(&wSub);
					}
					break;
				case SMSelNoHilite:
					LinkSEL(pL) = True;
					break;
				case SMDeselect:
					if (ConnectSEL(aConnectL)) {
						ConnectSEL(aConnectL) = False;
						HiliteRect(&wSub);
					}
					break;
				case SMSymDrag:
					/* No Symbol dragging for CONNECTs */
					break;
				case SMFind:
					if (PtInRect(*(Point *)ptr, &r))
						result = i;
					break;
				case SMHilite:
					break;
				case SMFlash:
					HiliteRect(&wSub);
					break;
				default:
					;
			}
			if (ConnectSEL(aConnectL))
				objSel = True;
		}
	}

	LinkSEL(pL) = objSel;
	return result;
}


/* ------------------------------------------------------------------------- CheckCLEF -- */
/* CLEF object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckCLEF(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	PACLEF			aClef;			/* ptr to current sub object */
	LINK			aClefL;
	short			i,				/* scratch */
					oldtxSize;		/* get & restore the port's textSize, for inMeasure clefs */
	PCONTEXT		pContext;
	DDIST			xd, yd,			/* scratch DDIST coordinates */
					xdOct, ydOct,
					lnSpace;
	unsigned char	glyph;			/* clef symbol */
	short			result;			/* =NOMATCH unless object/subobject clicked in */
	Boolean			objSelected;	/* False unless something in the object is selected */
	Rect			rSub,			/* paper-relative bounding box for sub-object */
					wSub,			/* Window relative of above */
					aRect;			/* scratch */

PushLock(OBJheap);
PushLock(CLEFheap);
	objSelected = False;
	result = NOMATCH;
	oldtxSize = GetPortTxSize();

	aClefL = FirstSubLINK(pL);
	for (i = 0; aClefL; i++, aClefL=NextCLEFL(aClefL)) {
		aClef = GetPACLEF(aClefL);
		GetClefDrawInfo(doc, pL, aClefL, context, 100, &glyph, &xd, &yd, &xdOct, &ydOct);
		pContext = &context[ClefSTAFF(aClefL)];

		glyph = MapMusChar(doc->musFontInfoIndex, glyph);
		lnSpace = LNSPACE(pContext);
		xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
		yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
		
		rSub = charRectCache.charRect[glyph];
		OffsetRect(&rSub, d2p(xd), d2p(yd));
		wSub = rSub; OffsetRect(&wSub, pContext->paper.left, pContext->paper.top);
		
		switch (mode) {
		case SMClick:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				aClef->selected = !aClef->selected;
				HiliteRect(&wSub);
				result = i;
			}
			break;
		case SMDblClick:
			break;
		case SMDrag:
			UnionRect(&rSub, (Rect *)ptr, &aRect);			/* does (Rect *)ptr enclose rSub? */
			if (EqualRect(&aRect, (Rect *)ptr)) {
				aClef->selected = !aClef->selected;
				HiliteRect(&wSub);
				result = i;
			}
			break;
		case SMStaffDrag:
			if (aClef->staffn>=stfRange.topStaff && 
				 aClef->staffn<=stfRange.bottomStaff && !aClef->selected) {
				UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub? */
				if (EqualRect(&aRect, (Rect *)ptr)) {
					aClef->selected = True;
					HiliteRect(&wSub);
					result = stfRange.topStaff;
				}
			}
			break;
		case SMSelect:
			if (!aClef->selected) {
				aClef->selected = True;
				HiliteRect(&wSub);
			}
			break;
		case SMSelNoHilite:
			aClef->selected = True;
			break;
		case SMDeselect:
			if (aClef->selected) {
				aClef->selected = False;
				HiliteRect(&wSub);
			}
			break;
		case SMSymDrag:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				HandleSymDrag(doc, pL, aClefL, *(Point *)ptr, glyph);
				aClef->selected = !aClef->selected;
				HiliteRect(&wSub);
			}
			break;
		case SMFind:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				result = i;
			}
			break;
		case SMHilite:
			if (aClef->selected) {
				InsetRect(&wSub, -enlarge.h, -enlarge.v);
				HiliteRect(&wSub);
			}
			break;
		case SMFlash:
			HiliteRect(&wSub);
			break;
		}
		if (aClef->selected)
			objSelected = True;
	}
	if (mode!=SMFind) LinkSEL(pL) = objSelected;
	TextSize(oldtxSize);

PopLock(OBJheap);
PopLock(CLEFheap);
	return result;
}


/* ---------------------------------------------------------------------- CheckDYNAMIC -- */
/* DYNAMIC object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckDYNAMIC(Document *doc, LINK pL, CONTEXT context[],
							Ptr ptr,
							short mode,
							STFRANGE stfRange,
							Point enlarge)
{
	PADYNAMIC		aDynamic;		/* ptr to current subobject */
	LINK			aDynamicL,
					firstSync, lastSync;
	short			i,				/* scratch */
					staffn;			/* staff number of current subobject */
	PCONTEXT		pContext;
	DDIST			xd, yd;			/* scratch DDIST coordinates */
	unsigned char	glyph;			/* dynamic symbol */
	short			result;			/* =NOMATCH unless object/subobject clicked in */
	Boolean			objSelected;	/* False unless something in the object is selected */
	Rect			rSub,			/* bounding box for sub-object */
					wSub,			/* window-relative of above */
					aRect;			/* scratch */
	DDIST			lnSpace,		/* line height of staffLine */
					sysLeft;

PushLock(OBJheap);
PushLock(DYNAMheap);
	result = NOMATCH;
	objSelected = False;
	aDynamicL = FirstSubLINK(pL);
	for (i = 0; aDynamicL; i++, aDynamicL=NextDYNAMICL(aDynamicL)) {
		aDynamic = GetPADYNAMIC(aDynamicL);
		staffn = aDynamic->staffn;

		GetDynamicDrawInfo(doc, pL, aDynamicL, context, &glyph, &xd, &yd);
		pContext = &context[staffn];
		lnSpace = LNSPACE(pContext);
		sysLeft = pContext->systemLeft;

		if (DynamType(pL)==DIM_DYNAM || DynamType(pL)==CRESC_DYNAM) {
			DDIST endxd;
			DDIST endyd = aDynamic->endyd + pContext->measureTop;
			DDIST offset = qd2d(aDynamic->otherWidth, pContext->staffHeight, pContext->staffLines)/2;
			DDIST rise = qd2d(aDynamic->mouthWidth, pContext->staffHeight, pContext->staffLines)/2;

			if (SystemTYPE(DynamLASTSYNC(pL)))
				endxd = aDynamic->endxd+sysLeft;
			else
				endxd = SysRelxd(DynamLASTSYNC(pL))+aDynamic->endxd+sysLeft;

			GetHairpinBBox(xd, endxd, yd, endyd, rise, offset, DynamType(pL), &rSub);
		}
		else {
			glyph = MapMusChar(doc->musFontInfoIndex, glyph);
			xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
			yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
			rSub = charRectCache.charRect[glyph];
			OffsetRect(&rSub, d2p(xd), d2p(yd));
		}
		wSub = rSub;
		OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
		
		switch (mode) {
		case SMClick:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				aDynamic->selected = !aDynamic->selected;
				HiliteRect(&wSub);
				result = i;
			}
			break;
		case SMDblClick:
			firstSync = DynamFIRSTSYNC(pL);
			lastSync = DynamLASTSYNC(pL);
						
			if (IsHairpin(pL)) {
//				HiliteAttPoints(doc, firstSync, lastSync, staffn);
				InvertTwoSymbolHilite(doc, firstSync, lastSync, staffn);	/* Hiliting on */
				if (aDynamic->selected)	HiliteRect(&wSub);			/* in case 1st click wasn't in rSub (case SMClick) */
				DoHairpinEdit(doc, pL);
				InvertTwoSymbolHilite(doc, firstSync, lastSync, staffn);	/* Hiliting off */
			}
			else {
				SignedByte oldDynamType, newDynamType;
				Boolean change;

				InvertSymbolHilite(doc, firstSync, staffn, True);			/* Hiliting on */
				DisableUndo(doc, False);
				oldDynamType = newDynamType = DynamType(pL);
				change = SetDynamicDialog(&newDynamType);
				if (change && newDynamType!=oldDynamType) {
					/* FIXME: objRects of dynamics aren't right in all cases, so the inval's
					here don't always work. See comments in body of InvalObject. Use
					EraseAndInval on <wsub> instead? */
					InvalObject(doc, pL, True);							/* inval old symbol */
					DynamType(pL) = newDynamType;
					FixContextForDynamic(doc, RightLINK(pL), staffn, oldDynamType, newDynamType);
					InvalObject(doc, pL, True);							/* inval new symbol */
					doc->changed = True;
				}
				InvertSymbolHilite(doc, firstSync, staffn, False);		/* Hiliting off */
			}
			break;
		case SMDrag:
			UnionRect(&rSub, (Rect *)ptr, &aRect);			/* does (Rect *)ptr enclose rSub? */
			if (EqualRect(&aRect, (Rect *)ptr)) {
				aDynamic->selected = !aDynamic->selected;
				HiliteRect(&wSub);
				result = i;
			}
			break;
		case SMStaffDrag:
			if (aDynamic->staffn>=stfRange.topStaff && 
				 aDynamic->staffn<=stfRange.bottomStaff && !aDynamic->selected) {
				UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub? */
				if (EqualRect(&aRect, (Rect *)ptr)) {
					aDynamic->selected = True;
					HiliteRect(&wSub);
					result = stfRange.topStaff;
				}
			}
			break;
		case SMSelect:
			if (!aDynamic->selected) {
				aDynamic->selected = True;
				HiliteRect(&wSub);
			}
			break;
		case SMSelNoHilite:
			aDynamic->selected = True;
			break;
		case SMDeselect:
			if (aDynamic->selected) {
				aDynamic->selected = False;
				HiliteRect(&wSub);
			}
			break;
		case SMSymDrag:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				HandleSymDrag(doc, pL, aDynamicL, *(Point *)ptr, glyph);
				aDynamic->selected = !aDynamic->selected;
				HiliteRect(&wSub);
			}
			break;
		case SMFind:
			if (PtInRect(*(Point *)ptr, &rSub))
				result = i;
			break;
		case SMHilite:
			if (aDynamic->selected) {
				InsetRect(&wSub, -enlarge.h, -enlarge.v);
				HiliteRect(&wSub);
			}
			break;
		case SMFlash:
			HiliteRect(&wSub);
			break;
		}
		if (aDynamic->selected)
			objSelected = True;
	}

	if (mode!=SMFind) LinkSEL(pL) = objSelected;
PopLock(OBJheap);
PopLock(DYNAMheap);
	return result;
}


/* ----------------------------------------------------------------------- CheckRPTEND -- */
/* REPEATEND object selecter/highliter.  Does different things depending on the value
of <mode> (see the list above). */

short CheckRPTEND(Document *doc, LINK pL, CONTEXT context[],
					Ptr ptr,
					short mode,
					STFRANGE stfRange,
					Point enlarge)
{
	LINK			aRptL;
	PARPTEND		aRpt;
	short			i;			
	PCONTEXT		pContext;
	unsigned char dummy=0;				/* for call to HandleSymDrag; not used. */
	short			result;				/* =NOMATCH unless object/subobject clicked in */
	Boolean			objSelected;
	DDIST			xd, yd,
					dBottom;			/* Bottom of subRect for aRpt, if aRpt->connStaff==0 */
	Rect			rSub,				/* bounding box for object */
					wSub,				/* window-relative of above */
					aRect,tempR;

PushLock(OBJheap);
PushLock(RPTENDheap);

	result = NOMATCH;
	objSelected = False;
	aRptL = FirstSubLINK(pL);
	aRpt = GetPARPTEND(aRptL);
	
	for (i=0; aRptL; i++, aRptL = NextRPTENDL(aRptL)) {
		if (VISIBLE(pL)) {
	
			GetRptEndDrawInfo(doc, pL, aRptL, context, &xd, &yd, &rSub);
			pContext = &context[RptEndSTAFF(aRptL)];
			aRpt = GetPARPTEND(aRptL);
			
			if (!aRpt->connAbove) {
				if (aRpt->connStaff!=0) {
					dBottom = context[aRpt->connStaff].staffTop
									+context[aRpt->connStaff].staffHeight;
					rSub.bottom = d2p(dBottom);
				}
				else
					rSub.bottom = d2p(pContext->staffTop+pContext->staffHeight);
			}

			wSub = rSub; OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
			switch (mode) {
				case SMClick:
					if (PtInRect(*(Point *)ptr, &rSub)) {
						aRpt->selected = !aRpt->selected;
						if (!aRpt->connAbove) HiliteRect(&wSub);
						result = i;
					}
					break;
				case SMDblClick:
					break;
				case SMDrag:
					tempR = *(Rect *)ptr;
					COffsetRect(&tempR,pContext->paper.left,pContext->paper.top);
					UnionRect(&rSub, &tempR, &aRect);	/* does (Rect *)ptr enclose rSub? */
					if (EqualRect(&aRect, &tempR)) {
						aRpt->selected = !aRpt->selected;
						if (!aRpt->connAbove) HiliteRect(&wSub);
						result = i;
					}
					break;
				case SMStaffDrag:
					if (aRpt->staffn>=stfRange.topStaff && 
						 aRpt->staffn<=stfRange.bottomStaff && !aRpt->selected) {
						tempR = *(Rect *)ptr;
						UnionRect(&rSub, &tempR, &aRect);	/* does (Rect *)ptr enclose rSub? */
						if (EqualRect(&aRect, &tempR)) {
							aRpt->selected = True;
							if (!aRpt->connAbove) HiliteRect(&wSub);
						}
					}
					break;
				case SMSelect:
					if (!aRpt->selected) {
						aRpt->selected = True;
						if (!aRpt->connAbove) HiliteRect(&wSub);
					}
					break;
				case SMSelNoHilite:
					aRpt->selected = True;
					break;
				case SMDeselect:
					if (aRpt->selected) {
						aRpt->selected = False;
						if (!aRpt->connAbove) HiliteRect(&wSub);
					}
					break;
				case SMSymDrag:
					if (PtInRect(*(Point *)ptr, &rSub)) {
						HandleSymDrag(doc, pL, aRptL, *(Point *)ptr, dummy);
						aRpt->selected = !aRpt->selected;
						if (!aRpt->connAbove) HiliteRect(&wSub);
					}
					break;
				case SMFind:
					if (PtInRect(*(Point *)ptr, &rSub))
						result = i;
					break;
				case SMHilite:
					if (aRpt->selected)
						if (!aRpt->connAbove) {
							InsetRect(&wSub, -enlarge.h, -enlarge.v);
							HiliteRect(&wSub);
						}
					break;
				case SMFlash:
					if (!aRpt->connAbove) HiliteRect(&wSub);
					break;
				default:
					break;
			}
			if (aRpt->selected) objSelected = True;
		}
	}
	if (mode!=SMFind) LinkSEL(pL) = objSelected;

PopLock(OBJheap);
PopLock(RPTENDheap);

	return result;
}


// FIXME: BELONGS ELSEWHERE!

Boolean ChordFrameDialog(Document *doc, Boolean *relFSize, short *size, short *style,
				short *enclosure, unsigned char *fontname, unsigned char *pTheChar);

/* ---------------------------------------------------------------------- CheckGRAPHIC -- */
/* GRAPHIC object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above), plus:
	=SMThread      Determines whether *(Point *)ptr is inside any subobject(s),
	               selecting & highliting if so.
*/

short CheckGRAPHIC(Document *doc, LINK pL, CONTEXT /*context*/[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	PGRAPHIC		p;
	LINK			aGraphicL;
	CONTEXT			context1;
	PCONTEXT		pContext;
	short			result;			/* =NOMATCH unless object clicked in */
	Rect			r,				/* object rectangle */
					aRect,			/* scratch */
					oldObjRect,		/* rects to inval after editing by dblclicking. */
					tempR;
	unsigned char	dummy=0;
	short			fontSize, fontStyle, enclosure,
					newWidth, styleChoice,
					staffn;
	STRINGOFFSET	offset;
	Str63			newFont;
	Str255			string;
	Boolean 		change=False,
					relFSize, lyric, expanded;

PushLock(OBJheap);
PushLock(GRAPHICheap);
	p = GetPGRAPHIC(pL);
	if (PageTYPE(GraphicFIRSTOBJ(pL))) staffn = 1;
	else staffn = p->staffn;

	result = NOMATCH;
	
	if (VISIBLE(pL) && LOOKING_AT(doc, GraphicVOICE(pL))) {
		r = ContextedObjRect(doc, pL, staffn ? staffn : 1, &context1);
		pContext = &context1;

		switch (mode) {
			case SMClick:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
					result = 0;
				}
				break;
			case SMDblClick:
LogPrintf(LOG_DEBUG, "CheckGRAPHIC: <InvalObject 1\n");
				InvalObject(doc, pL, True);								/* Insure objRect is correct */
				oldObjRect = p->objRect;
				if (GraphicSubType(pL)==GRDraw) {
					HiliteAttPoints(doc, p->firstObj, p->lastObj, staffn);
					if (LinkSEL(pL)) HiliteRect(&r);
				}
				else
					InvertSymbolHilite(doc, p->firstObj, staffn, True);			/* Hiliting on */
				while (Button()) ;

				/* Double-clicking all subtypes but GRArpeggio allows editing them. */
				if (p->graphicType==GRArpeggio) {
					InvertSymbolHilite(doc, p->firstObj, staffn, False);		/* Hiliting off */
					break;
				}
				
				PStrncpy((StringPtr)newFont, (StringPtr)doc->fontTable[p->fontInd].fontName, 32);
				Pstrcpy((StringPtr)string, (StringPtr)PCopy(GetPAGRAPHIC(FirstSubLINK(pL))->strOffset));
							
				switch (p->graphicType) {
					case GRString:
					case GRLyric:
						relFSize = p->relFSize;
						fontSize = p->fontSize;
						fontStyle = p->fontStyle;
						enclosure = p->enclosure;
						lyric = (p->graphicType==GRLyric? True : False);
						expanded = (p->info2!=0);
						styleChoice = Header2UserFontNum(p->info);
						change = TextDialog(doc, &styleChoice, &relFSize, &fontSize,
												&fontStyle, &enclosure, &lyric, &expanded,
												newFont, string, pContext);
						if (change) {
							short newFontIndex;
							/*
							 * Get the new font's index, adding it to the table if necessary.
							 * But if the table overflows, give up.
							 */
							newFontIndex = FontName2Index(doc, newFont);
							if (newFontIndex<0) {
								GetIndCString(strBuf, MISCERRS_STRS, 27);    /* "The font won't be changed" */
								CParamText(strBuf, "", "", "");
								StopInform(MANYFONTS_ALRT);
								break;
							}

#ifdef NOTYET
							/* The PrepareUndo below should be all that's needed
							   to implement undoing editing a text graphic, but
							   -- by itself -- it crashes consistently. Adding the
							   stmt to reset p avoids that problem, which makes no
							   sense (since the relevant heaps are locked); but then
							   Undoing often fails, apparently because we're not
							   saving and restoring the string pool. Sigh. */
							PrepareUndo(doc, pL, U_EditText, 51);			/* "Undo Edit Text" */
							p = GetPGRAPHIC(pL);
#else
							DisableUndo(doc, False);
#endif
							p->graphicType = (lyric? GRLyric : GRString);
							p->relFSize = relFSize;
							p->fontSize = fontSize;
							p->fontStyle = fontStyle;
							p->enclosure = enclosure;
							p->fontInd = newFontIndex;
							p->info = User2HeaderFontNum(doc, styleChoice);
							p->info2 = (expanded? 1 : 0);

							p->multiLine = False;
							for (short i = 1; i <= string[0]; i++)
								if (string[i] == CH_CR) {
									p->multiLine = True;
									break;
								}
LogPrintf(LOG_DEBUG, "CheckGRAPHIC: <InvalObject 2/EraseAndInval pL=%u strlen=%d\n", pL, Pstrlen(string));
#if 0
							DrawGRAPHIC(doc, pL, contextA, False);
							r = LinkOBJRECT(pL);
							EraseAndInval(&r);
#elif 0
							InvalObject(doc, pL, True);					/* In case it's gotten larger */
#endif
						}
						break;
					case GRRehearsal:
						change = RehearsalMarkDialog(string);
						if (change) doc->changed = True;
						break;
					case GRMIDIPatch:
						change = PatchChangeDialog(string);
						if (change) doc->changed = True;
						if (string[0]>16-1) string[0] = 16-1;		//if (string[0]>MPATCH_LEN-1) string[0] = MPATCH_LEN-1;
						short patchNum = FindIntInString(string);
						p->info = patchNum;
						break;
					case GRMIDIPan:
						change = PanSettingDialog(string);
						if (change) doc->changed = True;
						if (string[0]>16-1) string[0] = 16-1;		//if (string[0]>MPATCH_LEN-1) string[0] = MPATCH_LEN-1;
						short pansetting = FindIntInString(string);
						p->info = pansetting;
						break;
					case GRChordSym:
						{
							short auxInfo = p->info;
							change = ChordSymDialog(doc, string, &auxInfo);
							if (change)	{
								p->info = auxInfo;
								doc->changed = True;
							}
						}
						break;
					case GRChordFrame:
						{
							short dummySize, dummyStyle, dummyEncl;
							Str63 dummyFont; Boolean dummyRelSize;

							change = ChordFrameDialog(doc, &dummyRelSize, &dummySize, &dummyStyle,
								&dummyEncl, dummyFont, &string[1]);
						}
						if (change) doc->changed = True;
						break;
					case GRDraw:
						DoDrawingEdit(doc, pL);
						break;
					case GRSusPedalDown:
					case GRSusPedalUp:
						InvertSymbolHilite(doc, p->firstObj, staffn, True);
						SleepTicks(30L);
						break;
					default:
						;
				}
				
				if (GraphicSubType(pL)!=GRDraw)
					InvertSymbolHilite(doc, p->firstObj, staffn, False);		/* Hiliting off */
				if (change) {
LogPrintf(LOG_DEBUG, "CheckGRAPHIC: <EraseAndInval\n");
					/* FIXME: EraseAndInval'ing the OLD objRect MAY NOT BE ENUF! */
					EraseAndInval(&oldObjRect);
					aGraphicL = FirstSubLINK(pL);
					offset = PReplace(GraphicSTRING(aGraphicL), string);
					if (offset<0L)
						{ NoMoreMemory(); goto Cleanup; }
					else
						GraphicSTRING(aGraphicL) = offset;
					
					newWidth = GraphicWidth(doc, pL, pContext);
					LinkOBJRECT(pL).right = LinkOBJRECT(pL).left + newWidth;
				}

				break;
			case SMDrag:
				tempR = *(Rect *)ptr;
				COffsetRect(&tempR,pContext->paper.left, pContext->paper.top);
				if (r.top<tempR.top)		tempR.top = r.top;
				if (r.bottom>tempR.bottom) tempR.bottom = r.bottom;
				UnionRect(&r, &tempR, &aRect);				/* does (Rect *)ptr enclose r? */
				if (EqualRect(&aRect, &tempR)) {
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
					result = 0;
				}
				break;
			case SMStaffDrag:
				if (staffn>=stfRange.topStaff && 
					 staffn<=stfRange.bottomStaff && !p->selected) {
					tempR = *(Rect *)ptr;
					COffsetRect(&tempR,pContext->paper.left,pContext->paper.top);
					if (r.top<tempR.top)		tempR.top = r.top;
					if (r.bottom>tempR.bottom) tempR.bottom = r.bottom;
					UnionRect(&r, &tempR, &aRect);			/* does (Rect *)ptr enclose r? */
					if (EqualRect(&aRect, &tempR)) {
						p->selected = True;
						HiliteRect(&r);
						result = stfRange.topStaff;
					}
				}
				break;
			case SMSelect:
				if (!LinkSEL(pL)) {
					LinkSEL(pL) = True;
					HiliteRect(&r);
				}
				break;
			case SMSelNoHilite:
				LinkSEL(pL) = True;
				break;
			case SMDeselect:
				if (LinkSEL(pL)) {
					LinkSEL(pL) = False;
					HiliteRect(&r);
				}
				break;
			case SMThread:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
					if (!EqualRect(&selOldRect, &LinkOBJRECT(pL))) {
						LinkSEL(pL) = !LinkSEL(pL);
						HiliteRect(&r);
						selOldRect = LinkOBJRECT(pL);
					}
					result = 0;
				}
				break;
			case SMSymDrag:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
#ifdef TRY_TO_CONSTRAIN
					InvalObject(doc,pL,False);
#endif
					HandleSymDrag(doc, pL, NILINK, *(Point *)ptr, dummy);
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
				}
				break;
			case SMFind:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL)))
					result = 0;
				break;
			case SMHilite:
				if (LinkSEL(pL)) {
					InsetRect(&r, -enlarge.h, -enlarge.v);
					HiliteRect(&r);
				}
				break;
			case SMFlash:
				HiliteRect(&r);
				break;
			default:
				;
		}
	}

Cleanup:
PopLock(GRAPHICheap);
PopLock(OBJheap);

	return result;
}


/* ------------------------------------------------------------------------ CheckTEMPO -- */
/* TEMPO object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckTEMPO(Document *doc, LINK pL, CONTEXT context[],
					Ptr ptr,
					short mode,
					STFRANGE stfRange,
					Point enlarge)
{
	PTEMPO			p;
	Rect			r, aRect, oldObjRect, newObjRect, tempR;
	short			newWidth, result, dur, staffn;
	long			beatsPM;
	Boolean			useMM, showMM, ok, dotted, expanded;
	Str63			tempoStr, metroStr;
	unsigned char	dummy=0;
	PCONTEXT		pContext;
	STRINGOFFSET	offset;

PushLock(OBJheap);

	p = GetPTEMPO(pL);

	if (PageTYPE(TempoFIRSTOBJ(pL)))
			staffn = 1;
	else  staffn = p->staffn;

	result = NOMATCH;
	r = LinkOBJRECT(pL);
	
	if (VISIBLE(pL)) {
		pContext = (staffn ? &context[staffn] : &context[1]);
		OffsetRect(&r,pContext->paper.left,pContext->paper.top);

		switch (mode) {
			case SMClick:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
					result = 0;
				}
				break;
			case SMDblClick:
				oldObjRect = p->objRect;
				InvertSymbolHilite(doc, p->firstObjL, staffn, True);	/* Hiliting on */
				p = GetPTEMPO(pL);
				Pstrcpy((StringPtr)tempoStr, (StringPtr)PCopy(p->strOffset));
				p = GetPTEMPO(pL);
				Pstrcpy((StringPtr)metroStr, (StringPtr)PCopy(p->metroStrOffset));
				p = GetPTEMPO(pL);
				useMM = !(p->noMM);
				showMM = !(p->hideMM);
				dur = p->subType;
				dotted = p->dotted;
				expanded = p->expanded;
				ok = TempoDialog(&useMM, &showMM, &dur, &dotted, &expanded, tempoStr, metroStr);
				if (tempoStr[0]>63) tempoStr[0] = 63;				/* Limit length for consistency with InsertTempo */
				if (metroStr[0]>63) metroStr[0] = 63;				/* Limit length for consistency with InsertTempo */
				p = GetPTEMPO(pL);
				InvertSymbolHilite(doc, p->firstObjL, staffn, False);	/* Hiliting off */
				if (ok) {
					DisableUndo(doc, False);
					offset = PReplace(p->strOffset, tempoStr);
					if (offset<0L)
						{ NoMoreMemory(); goto Cleanup; }
					else
						p->strOffset = offset;

					offset = PReplace(p->metroStrOffset, metroStr);
					if (offset<0L)
						{ NoMoreMemory(); goto Cleanup; }
					else
						p->metroStrOffset = offset;

					p->noMM = !useMM;
					p->hideMM = !showMM;
					p->subType = dur;
					p->dotted = dotted;
					p->expanded = expanded;
					beatsPM = FindIntInString(metroStr);
					if (beatsPM<0L) beatsPM = config.defaultTempoMM;
					p->tempoMM = beatsPM;

					newWidth = StringWidth(tempoStr);		/* FIXME: BUT MUST SET FONT FIRST! */
					LinkOBJRECT(pL).right = LinkOBJRECT(pL).left + newWidth;
					doc->changed = True;
				}

				newObjRect = p->objRect;
				UnionRect(&oldObjRect, &newObjRect, &newObjRect);
				OffsetRect(&newObjRect, doc->currentPaper.left, doc->currentPaper.top);
				InvalWindowRect(doc->theWindow,&newObjRect);

				break;
			case SMDrag:
#ifdef DRAGRECT_TEMPO
				tempR = *(Rect *)ptr;
				COffsetRect(&tempR,pContext->paper.left, pContext->paper.top);
				if (r.top<tempR.top)			tempR.top = r.top;
				if (r.bottom>tempR.bottom) tempR.bottom = r.bottom;
				UnionRect(&r, &tempR, &aRect);				/* does (Rect *)ptr enclose r? */
				if (EqualRect(&aRect, &tempR)) {
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
					result = 0;
				}
#endif /* DRAGRECT_TEMPO */
				break;
			case SMStaffDrag:
				if (staffn>=stfRange.topStaff && 
					 staffn<=stfRange.bottomStaff && !p->selected) {
					tempR = *(Rect *)ptr;
					COffsetRect(&tempR, pContext->paper.left, pContext->paper.top);
					if (r.top<tempR.top)		tempR.top = r.top;
					if (r.bottom>tempR.bottom)	tempR.bottom = r.bottom;
					UnionRect(&r, &tempR, &aRect);			/* does (Rect *)ptr enclose r? */
					if (EqualRect(&aRect, &tempR)) {
						p->selected = True;
						HiliteRect(&r);
						result = stfRange.topStaff;
					}
				}
				break;
			case SMSelect:
				if (!LinkSEL(pL)) {
					LinkSEL(pL) = True;
					HiliteRect(&r);
				}
				break;
			case SMSelNoHilite:
				LinkSEL(pL) = True;
				break;
			case SMDeselect:
				if (LinkSEL(pL)) {
					LinkSEL(pL) = False;
					HiliteRect(&r);
				}
				break;
			case SMSymDrag:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
					HandleSymDrag(doc, pL, NILINK, *(Point *)ptr, dummy);
					LinkSEL(pL) = !LinkSEL(pL);
					/* HiliteRect(&r); */
				}
				break;
			case SMFind:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL)))
					result = 0;
				break;
			case SMHilite:
				if (LinkSEL(pL)) {
					InsetRect(&r, -enlarge.h, -enlarge.v);
					HiliteRect(&r);
				}
				break;
			case SMFlash:
				HiliteRect(&r);
				break;
			default:
				;
		}
	}

Cleanup:
PopLock(OBJheap);

	return result;
}


/* ----------------------------------------------------------------------- CheckSPACER -- */
/* SPACER object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckSPACER(Document *doc, LINK pL, CONTEXT context[],
					Ptr ptr,
					short mode,
					STFRANGE stfRange,
					Point enlarge)
{
	PSPACER		p;
	Rect		r,aRect,tempR;
	short		result;
	PCONTEXT	pContext;

	p = GetPSPACER(pL);
	result = NOMATCH;
	r = LinkOBJRECT(pL);
	
	if (VISIBLE(pL)) {
		pContext = (p->staffn ? &context[p->staffn] : &context[1]);
		OffsetRect(&r, pContext->paper.left, pContext->paper.top);

		switch (mode) {
			case SMClick:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
					result = 0;
				}
				break;
			case SMDblClick:
				break;
			case SMDrag:
#ifdef DRAGRECT_SPACE
				tempR = *(Rect *)ptr;
				COffsetRect(&tempR,pContext->paper.left,pContext->paper.top);
				if (r.top<tempR.top)		tempR.top = r.top;
				if (r.bottom>tempR.bottom) tempR.bottom = r.bottom;
				UnionRect(&r, &tempR, &aRect);				/* does (Rect *)ptr enclose r? */
				if (EqualRect(&aRect, &tempR)) {
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
					result = 0;
				}
#endif /* DRAGRECT_SPACE */
				break;
			case SMStaffDrag:
				if (p->staffn>=stfRange.topStaff && 
					 p->staffn<=stfRange.bottomStaff && !p->selected) {
					tempR = *(Rect *)ptr;
					COffsetRect(&tempR,pContext->paper.left,pContext->paper.top);
					if (r.top<tempR.top)		tempR.top = r.top;
					if (r.bottom>tempR.bottom) tempR.bottom = r.bottom;
					UnionRect(&r, &tempR, &aRect);			/* does (Rect *)ptr enclose r? */
					if (EqualRect(&aRect, &tempR)) {
						p->selected = True;
						HiliteRect(&r);
						result = stfRange.topStaff;
					}
				}
				break;
			case SMSelect:
				if (!LinkSEL(pL)) {
					LinkSEL(pL) = True;
					HiliteRect(&r);
				}
				break;
			case SMSelNoHilite:
				LinkSEL(pL) = True;
				break;
			case SMDeselect:
				if (LinkSEL(pL)) {
					LinkSEL(pL) = False;
					HiliteRect(&r);
				}
				break;
			case SMSymDrag:

				break;
			case SMFind:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL)))
					result = 0;
				break;
			case SMHilite:
				if (LinkSEL(pL)) {
					InsetRect(&r, -enlarge.h, -enlarge.v);
					HiliteRect(&r);
				}
				break;
			case SMFlash:
				HiliteRect(&r);
				break;
			default:
				;
		}
	}
	return result;
}


/* ----------------------------------------------------------------------- CheckENDING -- */
/* ENDING object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckENDING(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	PENDING p;
	Rect r, aRect, tempR;
	short result, endNum, newNumber, cutoffs, newCutoffs;
	unsigned char dummy=0;								/* for call to HandleSymDrag; not used. */
	PCONTEXT pContext;
	Boolean	okay;

	result = NOMATCH;
	r = LinkOBJRECT(pL);
	
	if (VISIBLE(pL)) {
		pContext = (EndingSTAFF(pL) ? &context[EndingSTAFF(pL)] : &context[1]);
		OffsetRect(&r, pContext->paper.left, pContext->paper.top);

		switch (mode) {
			case SMClick:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
					result = 0;
				}
				break;
			case SMDblClick:
				InvertTwoSymbolHilite(doc, EndingFIRSTOBJ(pL), EndingLASTOBJ(pL), EndingSTAFF(pL)); /* On */
				while (Button()) ;
				
				p = GetPENDING(pL);
				endNum = p->endNum;
				cutoffs = (p->noLCutoff << 1) + p->noRCutoff;
				okay = EndingDialog(endNum, &newNumber, cutoffs, &newCutoffs);

				InvertSymbolHilite(doc, EndingFIRSTOBJ(pL), EndingSTAFF(pL), False);	/* Hiliting off */
				if (EndingFIRSTOBJ(pL)!=EndingLASTOBJ(pL))
					InvertSymbolHilite(doc, EndingLASTOBJ(pL), EndingSTAFF(pL), False);	/* Hiliting off */

				if (okay) {
					p = GetPENDING(pL);
					p->noLCutoff = (newCutoffs & 2) >> 1;
					p->noRCutoff = (newCutoffs & 1);
					p->endNum = newNumber;
					InvalSystem(pL);
					doc->changed = True;
				}
				break;
			case SMDrag:
#ifdef DRAGRECT_ENDINGS
				tempR = *(Rect *)ptr;
				COffsetRect(&tempR,pContext->paper.left,pContext->paper.top);
				if (r.top<tempR.top)		tempR.top = r.top;
				if (r.bottom>tempR.bottom) tempR.bottom = r.bottom;
				UnionRect(&r, &tempR, &aRect);					/* does (Rect *)ptr enclose r? */
				if (EqualRect(&aRect, &tempR)) {
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
					result = 0;
				}
#endif
				break;
			case SMStaffDrag:
				if (EndingSTAFF(pL)>=stfRange.topStaff && 
					 EndingSTAFF(pL)<=stfRange.bottomStaff && !LinkSEL(pL)) {
					tempR = *(Rect *)ptr;
					COffsetRect(&tempR,pContext->paper.left,pContext->paper.top);
					if (r.top<tempR.top)		tempR.top = r.top;
					if (r.bottom>tempR.bottom) tempR.bottom = r.bottom;
					UnionRect(&r, &tempR, &aRect);				/* does (Rect *)ptr enclose r? */
					if (EqualRect(&aRect, &tempR)) {
						LinkSEL(pL) = True;
						HiliteRect(&r);
						result = stfRange.topStaff;
					}
				}
				break;
			case SMSelect:
				if (!LinkSEL(pL)) {
					LinkSEL(pL) = True;
					HiliteRect(&r);
				}
				break;
			case SMSelNoHilite:
				LinkSEL(pL) = True;
				break;
			case SMDeselect:
				if (LinkSEL(pL)) {
					LinkSEL(pL) = False;
					HiliteRect(&r);
				}
				break;
			case SMSymDrag:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
					HandleSymDrag(doc, pL, NILINK, *(Point *)ptr, dummy);
					LinkSEL(pL) = !LinkSEL(pL);
					HiliteRect(&r);
				}
				break;
			case SMFind:
				if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL)))
					result = 0;
				break;
			case SMHilite:
				if (LinkSEL(pL)) {
					InsetRect(&r, -enlarge.h, -enlarge.v);
					HiliteRect(&r);
				}
				break;
			case SMFlash:
				HiliteRect(&r);
				break;
			default:
				;
		}
	}
	
	return result;
}


/* ---------------------------------------------------------------------- DoOpenKeysig -- */
/* Handle a double-click in a keysig subobj rect. Present a dlog letting user
change the keysig on this staff or all staves. Return True if there was any
change, False if not.  <pL> is the keysig object; <aKeySigL> is a LINK to the
subobject the user clicked. Assumes keysig and object heaps have been locked. */

static Boolean DoOpenKeysig(Document *doc, LINK pL, LINK aKeySigL);
static Boolean DoOpenKeysig(Document *doc, LINK pL, LINK aKeySigL)
{
	short sharps, flats, oldSharpsOrFlats, newSharpsOrFlats;
	LINK initKSL, endL;
	PAKEYSIG aKeySig;
	KSINFO oldKSInfo, newKSInfo;
	Boolean beforeFirstMeas, change;
	static Boolean onAllStaves = True;

	/* Don't let user replace gutter keysigs other than the first. */
	beforeFirstMeas = LinkBefFirstMeas(pL);
	if (beforeFirstMeas && doc->currentSystem!=1)
		return False;

	aKeySig = GetPAKEYSIG(aKeySigL);

	KeySigCopy((PKSINFO)aKeySig->KSItem, &oldKSInfo);
	oldSharpsOrFlats = XGetSharpsOrFlats(&oldKSInfo);

	sharps = flats = 0;
	if (oldSharpsOrFlats>=0)
		sharps = oldSharpsOrFlats;
	else
		flats = -oldSharpsOrFlats;

	change = KeySigDialog(&sharps, &flats, &onAllStaves, False);
	if (change) {
		if (sharps>0) newSharpsOrFlats = sharps;
		else if (flats>0) newSharpsOrFlats = -flats;
		else newSharpsOrFlats = 0;

		DisableUndo(doc, False);

		endL = pL;
		
		if (onAllStaves) {
			LINK qL;

			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
				aKeySig = GetPAKEYSIG(aKeySigL);
				KeySigCopy((PKSINFO)aKeySig->KSItem, &oldKSInfo);
				oldSharpsOrFlats = XGetSharpsOrFlats(&oldKSInfo);
				SetupKeySig(aKeySigL, newSharpsOrFlats);
				KeySigCopy((PKSINFO)aKeySig->KSItem, &newKSInfo);
				if (beforeFirstMeas)
					UpdateBFKSStaff(pL, aKeySig->staffn, newKSInfo);
				FixContextForKeySig(doc, RightLINK(pL), aKeySig->staffn,
											oldKSInfo, newKSInfo);
				qL = FixAccsForKeySig(doc, pL, aKeySig->staffn, oldKSInfo, newKSInfo);
				if (qL)
					if (IsAfter(endL, qL)) endL = qL;
			}
		}
		else {
			SetupKeySig(aKeySigL, newSharpsOrFlats);
			KeySigCopy((PKSINFO)aKeySig->KSItem, &newKSInfo);
			if (beforeFirstMeas)
				UpdateBFKSStaff(pL, aKeySig->staffn, newKSInfo);
			FixContextForKeySig(doc, RightLINK(pL), aKeySig->staffn,
											oldKSInfo, newKSInfo);
			endL = FixAccsForKeySig(doc, pL, aKeySig->staffn, oldKSInfo, newKSInfo);
		}

		/* Update the space before the initial Measure on all systems affected by
		the keysig change. This range includes the system containing <pL>, iff
		<pL> is the first keysig in the score. */

		if (beforeFirstMeas)
			initKSL = pL;
		else
			initKSL = LSSearch(RightLINK(pL), KEYSIGtype, ANYONE, GO_RIGHT, False);
		if (initKSL) {
			short staff = onAllStaves? ANYONE : aKeySig->staffn;
			FixInitialKSxds(doc, initKSL, endL, staff);
		}

		InvalSystems(pL, (endL? endL : pL));

		if (doc->autoRespace) {
			long resFact = RESFACTOR*(long)doc->spacePercent;
			endL = EndMeasSearch(doc, pL);
			RespaceBars(doc, LeftLINK(pL), endL, resFact, False, False);
		}
		doc->changed = True;
	}

	return change;
}


/* ----------------------------------------------------------------------- CheckKEYSIG -- */
/* KEYSIG object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckKEYSIG(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	PAKEYSIG	aKeySig;
	LINK		aKeySigL;
	PCONTEXT	pContext;
	short		i, width;		/* total pixel width of key sig. */
	short 		lines;
	short		result;			/* =NOMATCH unless object/subobject clicked in */
	Boolean		objSelected;	/* False unless something in the object is selected */
	DDIST		xd, yd,			/* scratch DDIST coordinates */
				dTop,			/* absolute DDIST position of origin (staff or measure) */
				height;			/* height of current staff */
	Rect		rSub,			/* subobject rectangle */
				wSub,			/* window-relative of above */
				aRect;			/* scratch */
	unsigned char dummy=0;

PushLock(OBJheap);
PushLock(KEYSIGheap);

	objSelected = False;
	result = NOMATCH;
	aKeySigL = FirstSubLINK(pL);
	for (i = 0; aKeySigL; i++, aKeySigL=NextKEYSIGL(aKeySigL)) {
		aKeySig = GetPAKEYSIG(aKeySigL);
		GetKeySigDrawInfo(doc, pL, aKeySigL, context, &xd, &yd, &dTop, &height, &lines);
		pContext = &context[aKeySig->staffn];
		width = GetKSWidth(aKeySigL, pContext->staffHeight, pContext->staffLines);

		rSub = SetSubRect(xd, dTop, width, pContext);
		wSub = rSub;
		OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
		
		switch (mode) {
		case SMClick:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				aKeySig->selected = !aKeySig->selected;
				HiliteRect(&wSub);
				result = i;
			}
			break;
		case SMDblClick:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				if (DoOpenKeysig(doc, pL, aKeySigL))
					goto Cleanup;
			}
			break;
		case SMDrag:
			UnionRect(&rSub, (Rect *)ptr, &aRect);			/* does (Rect *)ptr enclose rSub? */
			if (EqualRect(&aRect, (Rect *)ptr)) {
				aKeySig->selected = !aKeySig->selected;
				HiliteRect(&wSub);
				result = i;
			}
			break;
		case SMStaffDrag:
			if (aKeySig->staffn>=stfRange.topStaff &&
				 aKeySig->staffn<=stfRange.bottomStaff && !aKeySig->selected) {
				UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub? */
				if (EqualRect(&aRect, (Rect *)ptr)) {
					aKeySig->selected = True;
					HiliteRect(&wSub);
					result = stfRange.topStaff;
				}
			}
			break;
		case SMSelect:
			if (!aKeySig->selected) {
				aKeySig->selected = True;
				HiliteRect(&wSub);
			}
			break;
		case SMSelNoHilite:
			aKeySig->selected = True;
			break;
		case SMDeselect:
			if (aKeySig->selected) {
				aKeySig->selected = False;
				HiliteRect(&wSub);
			}
			break;
		case SMSymDrag:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				HandleSymDrag(doc, pL, aKeySigL, *(Point *)ptr, dummy);
				aKeySig->selected = !aKeySig->selected;
				HiliteRect(&wSub);
			}
			break;
		case SMFind:
			if (PtInRect(*(Point *)ptr, &rSub))
				result = i;
			break;
		case SMHilite:
			if (aKeySig->selected) {
				InsetRect(&wSub, -enlarge.h, -enlarge.v);
				HiliteRect(&wSub);
			}
			break;
		case SMFlash:
			HiliteRect(&wSub);
			break;
		}
		if (aKeySig->selected)
			objSelected = True;
	}
	if (mode!=SMFind) LinkSEL(pL) = objSelected;
	
Cleanup:
PopLock(OBJheap);
PopLock(KEYSIGheap);

	return result;
}


#define SUSPICIOUS_WREL_RECT(r)             \
	(  r.left<-24000 || r.left>-22000        \
	|| r.top<-24000 || r.top>-22000          \
	|| r.right<-24000 || r.right>-22000      \
	|| r.bottom<-24000 || r.bottom>-22000 )

/* ------------------------------------------------------------------------- CheckSYNC -- */
/* SYNC object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above), plus:
	=SMThread      Determines whether *(Point *)ptr is inside any subobject(s),
	               selecting & highliting if so.
	=SMFindNote		Used by InsertSlur to find the note that will have a new
					slur. 
*/

short CheckSYNC(Document *doc, LINK pL, CONTEXT context[],
					Ptr ptr,
					short mode,
					STFRANGE stfRange,
					Point enlarge,
					Point enlargeSpecial)
{
	PANOTE		aNote;				/* ptr to current subobject */
	LINK		aNoteL, bNoteL;
	short		i;					/* scratch */
	PCONTEXT	pContext;
	DDIST		xd, yd,				/* scratch DDIST coordinates */
				dTop, dLeft,		/* absolute DDIST position of origin (staff or measure) */
				lnSpace;
	short		width;				/* dist to offset for otherStemSide */
	short		glyph;				/* symbol */
	short		result,				/* =NOMATCH unless object/subobject clicked in */
				oldtxSize;			/* get & restore the port's textSize */
	Boolean		objSelected,		/* False unless something in the object is selected */
				upOrDown,			/* needed for otherStemSide to offset rect correctly */
				noteFound;			/* for SMThread */
	Rect		rSub,				/* paper-relative bounding box for subobject */
				wSub,				/* window-relative of above */
				aRect;				/* scratch */
	DRect		dRestBar;

PushLock(OBJheap);
PushLock(NOTEheap);
	oldtxSize = GetPortTxSize();

	objSelected = False;
	noteFound = False;
	result = NOMATCH;
	aNoteL = FirstSubLINK(pL);
	for (i = 0; aNoteL; i++, aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if ((aNote->visible || doc->showInvis) &&
				LOOKING_AT(doc, aNote->voice) && !noteFound) {
			pContext = &context[aNote->staffn];
			dLeft = pContext->measureLeft + LinkXD(pL);	/* absolute origin of object */
			dTop = pContext->measureTop;
			xd = dLeft + aNote->xd;							/* absolute position of subobject */
			yd = dTop + aNote->yd;

			if (aNote->rest)									/* "note" is really a rest */
				if (aNote->subType<=WHOLEMR_L_DUR)
					glyph = MCH_rests[WHOLE_L_DUR-1];
				else
					glyph = MCH_rests[aNote->subType-1];
			else {												/* "note" is really a note */
				if (aNote->subType==UNKNOWN_L_DUR)
					glyph = MCH_quarterNoteHead;
				else if (aNote->subType<=WHOLE_L_DUR)
					glyph = (unsigned char)MCH_notes[aNote->subType-1];
				else if (aNote->subType==HALF_L_DUR)
					glyph = MCH_halfNoteHead;
				else
					glyph = MCH_quarterNoteHead;
			}
			glyph = MapMusChar(doc->musFontInfoIndex, glyph);
			lnSpace = LNSPACE(pContext);
			xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
			yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
			if (aNote->rest && aNote->subType<WHOLEMR_L_DUR) {
				GetMBRestBar(-aNote->subType, pContext, &dRestBar);
				D2Rect(&dRestBar, &rSub);
				InsetRect(&rSub, 0, -3);
			}
			else {
				rSub = charRectCache.charRect[glyph];
				TweakSubRects(&rSub, aNoteL, pContext);
			}

			OffsetRect(&rSub, d2p(xd), d2p(yd));
			if (aNote->otherStemSide)	{					/* adjust note for wrong stem side in chord */
				width = rSub.right-rSub.left;
				for (bNoteL=FirstSubLINK(pL);bNoteL;bNoteL=NextNOTEL(bNoteL))
					if (NoteVOICE(bNoteL)==NoteVOICE(aNoteL) && MainNote(bNoteL)) {	/* get stem up/down from note in voice w/ stem */
						upOrDown = NoteYD(bNoteL)>NoteYSTEM(bNoteL);
						break;
					}

				if (upOrDown) 									/* stem up */
					OffsetRect(&rSub, width, 0);
				else											/* stem down */
					OffsetRect(&rSub, -width, 0);
			}
			wSub = rSub;
			OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);

			switch (mode) {
			case SMClick:
				aRect = rSub;
				InsetRect(&aRect, -(1+enlarge.h), -enlarge.v);
				if (PtInRect(*(Point *)ptr, &aRect)) {
					aNote->selected = !aNote->selected;
					InsetRect(&wSub, -(1+enlargeSpecial.h), -enlargeSpecial.v);
					HiliteRect(&wSub);
					result = i;
				}
				break;
			case SMDrag:
				InsetRect(&rSub, -(1+enlarge.h), -enlarge.v);
				UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub? */
				if (EqualRect(&aRect, (Rect *)ptr)) {
					aNote->selected = !aNote->selected;
					InsetRect(&wSub, -(1+enlargeSpecial.h), -enlargeSpecial.v);
					HiliteRect(&wSub);
					result = i;
				}
				break;
			case SMDblClick:
				aRect = rSub;
				InsetRect(&aRect, -(1+enlarge.h), -enlarge.v);
				if (PtInRect(*(Point *)ptr, &aRect))
					HiliteAttPoints(doc, pL, NILINK, NoteSTAFF(aNoteL));
				break;
			case SMStaffDrag:
				if (aNote->staffn>=stfRange.topStaff && 
					 aNote->staffn<=stfRange.bottomStaff && !aNote->selected) {
					InsetRect(&rSub, -(1+enlarge.h), -enlarge.v);
					UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub? */
					if (EqualRect(&aRect, (Rect *)ptr)) {
						aNote->selected = True;
						InsetRect(&wSub, -(1+enlargeSpecial.h), -enlargeSpecial.v);
						HiliteRect(&wSub);
						result = stfRange.topStaff;
					}
				}
				break;
			case SMSelect:
				if (!aNote->selected) {
					aNote->selected = True;
					InsetRect(&wSub, -(1+enlargeSpecial.h), -enlargeSpecial.v);
					HiliteRect(&wSub);
				}
				break;
			case SMSelNoHilite:
				aNote->selected = True;
				break;
			case SMDeselect:
				if (aNote->selected) {
					aNote->selected = False;
					InsetRect(&wSub, -(1+enlargeSpecial.h), -enlargeSpecial.v);
					if (CapsLockKeyDown() && OptionKeyDown())
						if (SUSPICIOUS_WREL_RECT(wSub))
							LogPrintf(LOG_INFO, " %d:N/R %d,%d,%d,%d   %d,%d\n", pL, wSub.left, wSub.top, wSub.right, wSub.bottom,
								enlargeSpecial.h, enlargeSpecial.v);
					HiliteRect(&wSub);
				}
				break;
			case SMThread:
				aRect = rSub;
				InsetRect(&aRect, -(1+enlarge.h), -enlarge.v);
				if (PtInRect(*(Point *)ptr, &aRect)) {
					if (!EqualRect(&selOldRect,&aRect)) {
						aNote->selected = !aNote->selected;
						InsetRect(&wSub, -(1+enlargeSpecial.h), -enlargeSpecial.v);
						HiliteRect(&wSub);
						selOldRect = aRect;
					}
					result = i;
					noteFound = True;					/* Look at no more subobjects! */
				}
				break;
			case SMSymDrag:
				aRect = rSub;
				InsetRect(&aRect, -(1+enlarge.h), -enlarge.v);
				if (PtInRect(*(Point *)ptr, &rSub)) {
					HandleSymDrag(doc, pL, aNoteL, *(Point *)ptr, glyph);
					aNote->selected = !aNote->selected;
					InsetRect(&wSub, -(1+enlargeSpecial.h), -enlargeSpecial.v);
					HiliteRect(&wSub);
				}
				break;
			case SMFind:
				if (PtInRect(*(Point *)ptr, &rSub))
					result = i;
				break;
			case SMFindNote:

				/* CheckSYNC w/ SMFindNote is called by InsertSlur to find the note
					which will have a new slur; the second parameter is different to 
					correct for the optical illusion of click on a small noteHead. */

				InsetRect(&rSub, -(1+enlarge.h), -3*enlarge.v);
				if (PtInRect(*(Point *)ptr, &rSub))
					result = i;
				break;
			case SMHilite:
				if (aNote->selected) {
					InsetRect(&wSub, -(1+enlargeSpecial.h), -enlargeSpecial.v);
					HiliteRect(&wSub);
				}
				break;
			case SMFlash:
				InsetRect(&wSub, -(1+enlargeSpecial.h), -enlargeSpecial.v);
				HiliteRect(&wSub);
				break;
			}
			if (aNote->selected)
				objSelected = True;
		}
	}
	if (mode!=SMFind) LinkSEL(pL) = objSelected;
	TextSize(oldtxSize);
PopLock(OBJheap);
PopLock(NOTEheap);
	return result;
}


/* ----------------------------------------------------------------------- CheckGRSYNC -- */
/* GRSYNC object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above), plus:
	=SMThread      Determines whether *(Point *)ptr is inside any subobject(s),
	               selecting & highliting if so.
*/

short CheckGRSYNC(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	PAGRNOTE		aGRNote;		/* ptr to current sub object */
	LINK			aGRNoteL, bGRNoteL;
	short			i;				/* scratch */
	PCONTEXT		pContext;
	DDIST			xd, yd,			/* scratch DDIST coordinates */
					dTop, dLeft;	/* absolute DDIST position of origin (staff or measure) */
	short			width;			/* dist to offset for otherStemSide */
	short			glyph;			/* symbol */
	short			result,			/* =NOMATCH unless object/subobject clicked in */
					oldtxSize;		/* get & restore the port's textSize */
	Boolean			objSelected,	/* False unless something in the object is selected */
					upOrDown,		/* needed for otherStemSide to offset rect correctly */
					noteFound;		/* for SMThread */
	Rect			rSub,			/* bounding box for sub-object */
					wSub,			/* window-relative of above */
					aRect;			/* scratch */

PushLock(OBJheap);
PushLock(GRNOTEheap);
	oldtxSize = GetPortTxSize();

	objSelected = False;
	noteFound = False;
	result = NOMATCH;
	aGRNoteL = FirstSubLINK(pL);
	for (i = 0; aGRNoteL; i++, aGRNoteL=NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if ((aGRNote->visible || doc->showInvis) &&
					LOOKING_AT(doc, aGRNote->voice) && !noteFound) {
			pContext = &context[aGRNote->staffn];
			dLeft = pContext->measureLeft + LinkXD(pL);	/* absolute origin of object */
			dTop = pContext->measureTop;
			xd = dLeft + aGRNote->xd;							/* absolute position of subobject */
			yd = dTop + aGRNote->yd;

			if (aGRNote->subType <= WHOLE_L_DUR)
				glyph=(unsigned char)MCH_notes[aGRNote->subType-1];
			else if (aGRNote->subType==HALF_L_DUR)
				glyph=MCH_halfNoteHead;
			else
				glyph=MCH_quarterNoteHead;
			rSub = charRectCache.charRect[glyph];

			OffsetRect(&rSub, d2p(xd), d2p(yd));
			if (aGRNote->otherStemSide)	{				/* adjust note for wrong stem side in chord */
				width = rSub.right-rSub.left;
				for (bGRNoteL=FirstSubLINK(pL);bGRNoteL;bGRNoteL=NextGRNOTEL(bGRNoteL))
					if (GRNoteVOICE(bGRNoteL)==GRNoteVOICE(aGRNoteL) && GRMainNote(bGRNoteL)) {		/* get stem up/down from note in voice w/ stem */
						upOrDown = GRNoteYD(bGRNoteL)>GRNoteYSTEM(bGRNoteL);
						break;
					}

				if (upOrDown) 									/* stem up */
					OffsetRect(&rSub, width, 0);
				else												/* stem down */
					OffsetRect(&rSub, -width, 0);
			}
			wSub = rSub;
			OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);

			switch (mode) {
			case SMClick:
				aRect = rSub;
				InsetRect(&aRect, -1, -1);
				if (PtInRect(*(Point *)ptr, &aRect)) {
					aGRNote->selected = !aGRNote->selected;
					HiliteRect(&wSub);
					result = i;
				}
				break;
			case SMDblClick:
				break;
			case SMDrag:
				UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub? */
				if (EqualRect(&aRect, (Rect *)ptr)) {
					aGRNote->selected = !aGRNote->selected;
					HiliteRect(&wSub);
					result = i;
				}
				break;
			case SMStaffDrag:
				if (aGRNote->staffn>=stfRange.topStaff && 
					 aGRNote->staffn<=stfRange.bottomStaff && !aGRNote->selected) {
					UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub? */
					if (EqualRect(&aRect, (Rect *)ptr)) {
						aGRNote->selected = True;
						HiliteRect(&wSub);
						result = stfRange.topStaff;
					}
				}
				break;
			case SMSelect:
				if (!aGRNote->selected) {
					aGRNote->selected = True;
					HiliteRect(&wSub);
				}
				break;
			case SMSelNoHilite:
				aGRNote->selected = True;
				break;
			case SMDeselect:
				if (aGRNote->selected) {
					aGRNote->selected = False;
					HiliteRect(&wSub);
				}
				break;
			case SMThread:
				aRect = rSub;
				InsetRect(&aRect, -1, -1);
				if (PtInRect(*(Point *)ptr, &aRect)) {
					if (!EqualRect(&selOldRect,&aRect)) {
						aGRNote->selected = !aGRNote->selected;
						HiliteRect(&wSub);
						selOldRect = aRect;
					}
					result = i;
					noteFound = True;					/* Look at no more subobjects! */
				}
				break;
			case SMSymDrag:
				aRect = rSub;
				InsetRect(&aRect, -1, -1);
				if (PtInRect(*(Point *)ptr, &rSub)) {
					HandleSymDrag(doc, pL, aGRNoteL, *(Point *)ptr, glyph);
					aGRNote->selected = !aGRNote->selected;
					HiliteRect(&wSub);
				}
				break;
			case SMFind:
				if (PtInRect(*(Point *)ptr, &rSub))
					result = i;
				break;
			case SMFindNote:
				InsetRect(&rSub, -1, -3);
				if (PtInRect(*(Point *)ptr, &rSub))
					result = i;
				break;
			case SMHilite:
				if (aGRNote->selected) {
					InsetRect(&wSub, -enlarge.h, -enlarge.v);
					HiliteRect(&wSub);
				}
				break;
			case SMFlash:
				HiliteRect(&wSub);
				break;
			}
			if (aGRNote->selected)
				objSelected = True;
		}
	}
	if (mode!=SMFind) LinkSEL(pL) = objSelected;
	TextSize(oldtxSize);
PopLock(OBJheap);
PopLock(GRNOTEheap);
	return result;
}


/* --------------------------------------------------------------------- DoOpenTimesig -- */
/* Handle a double-click in a timesig subobj rect. Present a dlog letting user
change the timesig on this staff or all staves. Return True if there was any
change, False if not.  <pL> is the timesig object; <aTimeSigL> is a LINK to the
subobject the user clicked. Assumes timesig and object heaps have been locked. */

static Boolean DoOpenTimesig(Document *doc, LINK pL, LINK aTimeSigL);
static Boolean DoOpenTimesig(Document *doc, LINK pL, LINK aTimeSigL)
{
	short subType, numerator, denominator;
	PATIMESIG aTimeSig;
	Boolean beforeFirstMeas, change;
	static Boolean onAllStaves = True;

	beforeFirstMeas = LinkBefFirstMeas(pL);
	aTimeSig = GetPATIMESIG(aTimeSigL);
	subType = aTimeSig->subType;
	numerator = aTimeSig->numerator;
	denominator = aTimeSig->denominator;
	change = TimeSigDialog(&subType, &numerator, &denominator, &onAllStaves, False);
	if (change) {
		TSINFO timeSigInfo;

		timeSigInfo.TSType = subType;
		timeSigInfo.numerator = numerator;
		timeSigInfo.denominator = denominator;

		DisableUndo(doc, False);

		if (onAllStaves) {
			aTimeSigL = FirstSubLINK(pL);
			for ( ; aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL)) {
				aTimeSig = GetPATIMESIG(aTimeSigL);
				aTimeSig->subType = subType;
				aTimeSig->numerator = numerator;
				aTimeSig->denominator = denominator;

				if (beforeFirstMeas)
					UpdateBFTSStaff(pL, aTimeSig->staffn, subType, numerator, denominator);
				FixContextForTimeSig(doc, RightLINK(pL), aTimeSig->staffn, timeSigInfo);
			}
		}
		else {
			aTimeSig->subType = subType;
			aTimeSig->numerator = numerator;
			aTimeSig->denominator = denominator;

			if (beforeFirstMeas)
				UpdateBFTSStaff(pL, aTimeSig->staffn, subType, numerator, denominator);
			FixContextForTimeSig(doc, RightLINK(pL), aTimeSig->staffn, timeSigInfo);
		}
		FixTimeStamps(doc, pL, NILINK);			/* In case of whole-meas. rests */
		InvalObject(doc, pL, True);
		doc->changed = True;
	}
	return change;
}


/* ---------------------------------------------------------------------- CheckTIMESIG -- */
/* TIMESIG object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckTIMESIG(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	PTIMESIG		p;
	PATIMESIG		aTimeSig;
	LINK			aTimeSigL;
	PCONTEXT		pContext;
	short			i, width;
	short			result;			/* =NOMATCH unless object/subobject clicked in */
	Boolean			objSelected;	/* False unless something in the object is selected */
	short			xp, yp;			/* scratch pixel coordinates */
	DDIST			dTop, dLeft;	/* absolute DDIST position of origin (staff or measure) */
	Rect			rSub,			/* paper-relative subobject rectangle */
					wSub,			/* window-relative of above */
					aRect;
	char			nStr[20], dStr[20];
	unsigned char	dummy=0;

PushLock(OBJheap);
PushLock(TIMESIGheap);

	p = GetPTIMESIG(pL);
	objSelected = False;
	result = NOMATCH;
	aTimeSigL = FirstSubLINK(pL);
	for (i = 0; aTimeSigL; i++, aTimeSigL=NextTIMESIGL(aTimeSigL)) {
		aTimeSig = GetPATIMESIG(aTimeSigL);
		pContext = &context[aTimeSig->staffn];
		if (p->inMeasure) {
			dTop = pContext->measureTop;
			dLeft = pContext->measureLeft;
		}
		else {
			dTop = pContext->staffTop;
			dLeft = pContext->staffLeft;
		}
		xp = d2p(dLeft + LinkXD(pL) + aTimeSig->xd);
		yp = d2p(dTop + p->yd + aTimeSig->yd);
		switch (aTimeSig->subType) {
			case N_OVER_D:
				sprintf(nStr, "%d", aTimeSig->numerator);
				sprintf(dStr, "%d", aTimeSig->denominator);
				width = n_max(CStringWidth(nStr), CStringWidth(dStr));
				break;
			case C_TIME:
			case CUT_TIME:
				width = CharWidth(MCH_common);	/* Width of common and cut time sigs. are the same */
				break;
			case ZERO_TIME:
				width = CharWidth('0');
				break;
			case N_ONLY:
				sprintf(nStr, "%d", aTimeSig->numerator);
				width = CStringWidth(nStr);
				break;
			default:
					;
		}
		SetRect(&rSub, xp, yp, xp+width, yp+d2p(pContext->staffHeight));
		wSub = rSub;
		OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);

		switch (mode) {
		case SMClick:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				aTimeSig->selected = !aTimeSig->selected;
				HiliteRect(&wSub);
				result = i;
			}
			break;
		case SMDblClick:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				if (DoOpenTimesig(doc, pL, aTimeSigL))
					goto Cleanup;
			}
			break;
		case SMDrag:
			UnionRect(&rSub, (Rect *)ptr, &aRect);			/* does (Rect *)ptr enclose rSub? */
			if (EqualRect(&aRect, (Rect *)ptr)) {
				aTimeSig->selected = !aTimeSig->selected;
				HiliteRect(&wSub);
				result = i;
			}
			break;
		case SMStaffDrag:
			if (aTimeSig->staffn>=stfRange.topStaff && 
				 aTimeSig->staffn<=stfRange.bottomStaff && !aTimeSig->selected) {
				UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub? */
				if (EqualRect(&aRect, (Rect *)ptr)) {
					aTimeSig->selected = True;
					HiliteRect(&wSub);
					result = stfRange.topStaff;
				}
			}
			break;
		case SMSelect:
			if (!aTimeSig->selected) {
				aTimeSig->selected = True;
				HiliteRect(&wSub);
			}
			break;
		case SMSelNoHilite:
			aTimeSig->selected = True;
			break;
		case SMDeselect:
			if (aTimeSig->selected) {
				aTimeSig->selected = False;
				HiliteRect(&wSub);
			}
			break;
		case SMSymDrag:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				HandleSymDrag(doc, pL, aTimeSigL, *(Point *)ptr, dummy);
				aTimeSig->selected = !aTimeSig->selected;
				HiliteRect(&wSub);
			}
			break;
		case SMFind:
			if (PtInRect(*(Point *)ptr, &rSub))
				result = i;
			break;
		case SMHilite:
			if (aTimeSig->selected) {
				InsetRect(&wSub, -enlarge.h, -enlarge.v);
				HiliteRect(&wSub);
			}
			break;
		case SMFlash:
			HiliteRect(&wSub);
			break;
		}
		if (aTimeSig->selected)
			objSelected = True;
	}
	if (mode!=SMFind) LinkSEL(pL) = objSelected;
	
Cleanup:
PopLock(OBJheap);
PopLock(TIMESIGheap);
	return result;
}


/* ---------------------------------------------------------------------- CheckMEASURE -- */
/* MEASURE object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckMEASURE(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	PMEASURE	p;
	PAMEASURE	aMeasure;
	LINK		aMeasureL;
	PCONTEXT	pContext;
	short		i,
				halfWidth,			/* half of pixel width */
				groupTopStf, groupBottomStf;	/* Top/bottom staff nos. for current group */
	short		result,				/* =NOMATCH unless object/subobject clicked in */
				measureStf, connStaff;
	Boolean		objSelected,		/* False unless something in the object is selected */
				measDrag;
	DDIST		xd,					/* scratch DDIST coordinates */
				dTop, dLeft,		/* absolute DDIST position of origin (staff or measure) */
				dBottom;
	Rect		rSub,				/* paper-relative subobject rectangle */
				wSub,				/* window-relative of above */
				aRect;				/* scratch */
	unsigned char dummy=0;

PushLock(OBJheap);
PushLock(MEASUREheap);

	/*
	 *	From the user's standpoint, the first Measure (barline) of every System behaves
	 *	as if it doesn't exist: it's unselectable as well as invisible, and nothing
	 *	can be explicitly attached to it. (This is not ideal: it'd be very nice to be
	 *	able to select it so it could be moved with Get Info, at least if Show Invisibles
	 *	is on, and it might be good to let the user make it visible, etc. But we'd still
	 *	want to prevent deleting it!) So if this is the first Measure of its System, do
	 *	nothing.
	 */
	if (FirstMeasInSys(pL)) return NOMATCH;

	objSelected = False;
	measDrag = True;
	result = NOMATCH;
	halfWidth = 2;
	p = GetPMEASURE(pL);
	aMeasureL = FirstSubLINK(pL);
	for (i = 0; aMeasureL; i++, aMeasureL=NextMEASUREL(aMeasureL)) {
		aMeasure = GetPAMEASURE(aMeasureL);
		groupTopStf = groupBottomStf = aMeasure->staffn;
		measureStf = NextLimStaffn(doc,pL,True,aMeasure->staffn);
		pContext = &context[measureStf];
		dTop = pContext->staffTop;
		dLeft = pContext->staffLeft;
		xd = dLeft+LinkXD(pL);
		/*		
		 * Measure subobjects are unusual in that they may be grouped as indicated by
		 * connAbove and connStaff, and groups can only be selected as a whole. If this
		 * measure is the top one of a group, set rSub and groupTopStf/groupBottomStf
		 * to include the entire group; if it's a lower one of a group, they will then
		 * already be set correctly. Notice that this code assumes that when a lower
		 * subobject of a group is encountered, we've already set rSub from the top one
		 * of the group; this is safe as long as we insert subobjects for all staves at
		 * once and in order (FIXME: they really may _not_ be in order!) and don't
		 * allow deleting anything but the entire object.
		 */ 
		if (!aMeasure->connAbove) {
			groupTopStf = measureStf;
			if (aMeasure->connStaff!=0) {
				connStaff = NextLimStaffn(doc,pL,False,aMeasure->connStaff);
				dBottom = context[connStaff].staffTop
								+context[connStaff].staffHeight;
				switch (aMeasure->subType) {
					case BAR_SINGLE:
						SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
									d2p(xd)+halfWidth, d2p(dBottom));
						break;
					case BAR_DOUBLE:
						SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
									d2p(xd)+halfWidth+2, d2p(dBottom));
						break;
					case BAR_FINALDBL:
						SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
									d2p(xd)+halfWidth+3, d2p(dBottom));
						break;
					case BAR_RPT_L:
					case BAR_RPT_R:
					case BAR_RPT_LR:
				/* FIXME: NEED GetMeasureDrawInfo THAT SHARES CODE WITH GetRptEndDrawInfo */
						SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
									d2p(xd)+halfWidth+2, d2p(dBottom));
						break;
					default:
						break;
				}
				groupBottomStf = connStaff;
			}
			else {
				switch (aMeasure->subType) {
					case BAR_SINGLE:
						SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
									d2p(xd)+halfWidth,
									d2p(dTop+pContext->staffHeight));
						break;
					case BAR_DOUBLE:
						SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
									d2p(xd)+halfWidth+2,
									d2p(dTop+pContext->staffHeight));
						break;
					case BAR_FINALDBL:
						SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
									d2p(xd)+halfWidth+3,
									d2p(dTop+pContext->staffHeight));
						break;
					case BAR_RPT_L:
					case BAR_RPT_R:
					case BAR_RPT_LR:
				/* ?FIXME: NEED GetMeasureDrawInfo THAT SHARES CODE WITH GetRptEndDrawInfo */
						SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
									d2p(xd)+halfWidth+2,
									d2p(dTop+pContext->staffHeight));
						break;
					default:
						break;
				}
				groupBottomStf = measureStf;
			}
		}
		
		wSub = rSub; OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
		switch (mode) {
			case SMClick:
				if (PtInRect(*(Point *)ptr, &rSub)) {
					aMeasure->selected = !aMeasure->selected;
					if (!aMeasure->connAbove) HiliteRect(&wSub);
					result = i;
				}
				break;
			case SMDblClick:
				break;
			case SMDrag:
				UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub (the group)? */
				if (EqualRect(&aRect, (Rect *)ptr)) {
					aMeasure->selected = !aMeasure->selected;
					if (!aMeasure->connAbove) HiliteRect(&wSub);
					result = i;
				}
				break;
			case SMStaffDrag:
				if (groupTopStf>=stfRange.topStaff
				&&  groupBottomStf<=stfRange.bottomStaff
				&& !aMeasure->selected) {
					UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub (the group)? */
					if (EqualRect(&aRect, (Rect *)ptr)) {
						aMeasure->selected = True;
						if (!aMeasure->connAbove) HiliteRect(&wSub);
						result = stfRange.topStaff;				/* ??OR SHOULD IT BE groupTopStf? */
					}
				}
				break;
			case SMSelect:
				if (!aMeasure->selected) {
					aMeasure->selected = True;
					if (!aMeasure->connAbove) HiliteRect(&wSub);
				}
				break;
			case SMSelNoHilite:
				aMeasure->selected = True;
				break;
			case SMDeselect:
				if (aMeasure->selected) {
					aMeasure->selected = False;
					if (CapsLockKeyDown() && OptionKeyDown())
						if (SUSPICIOUS_WREL_RECT(wSub))
							if (!aMeasure->connAbove)
						LogPrintf(LOG_NOTICE, " %d:Meas %d,%d,%d,%d\n", pL, wSub.left, wSub.top, wSub.right, wSub.bottom);
					if (!aMeasure->connAbove) HiliteRect(&wSub);
				}
				break;
			case SMSymDrag:
				/* Given a measure with n subobjects, we'll get here n times. To keep
					the measure from being moved n times as far as the user actually
					dragged it, use measDrag to insure only one call to HandleSymDrag. */
					
				if (PtInRect(*(Point *)ptr, &rSub) && measDrag) {
					HandleSymDrag(doc, pL, aMeasureL, *(Point *)ptr, dummy);
					measDrag = False;
					aMeasure->selected = !aMeasure->selected;
					if (!aMeasure->connAbove) HiliteRect(&wSub);
				}
				break;
			case SMFind:
				if (PtInRect(*(Point *)ptr, &rSub))
					result = i;
				break;
			case SMHilite:
				if (aMeasure->selected)
					if (!aMeasure->connAbove) {
						InsetRect(&wSub, -enlarge.h, -enlarge.v);
						HiliteRect(&wSub);
					}
				break;
			case SMFlash:
				if (!aMeasure->connAbove) HiliteRect(&wSub);
				break;
			default:
				break;
		}
		if (aMeasure->selected)
			objSelected = True;
	}

	if (mode!=SMFind) LinkSEL(pL) = objSelected;
	if (mode==SMSymDrag && objSelected) SelAllSubObjs(pL);
PopLock(OBJheap);
PopLock(MEASUREheap);
	return result;
}

/* ----------------------------------------------------------------------- CheckPSMEAS -- */
/* PSMEAS object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckPSMEAS(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	PAPSMEAS		aPSMeas;
	LINK			aPSMeasL;
	PCONTEXT		pContext;
	short			i,
					halfWidth,			/* half of pixel width */
					groupTopStf, groupBottomStf;	/* Top/bottom staff nos. for current group */
	short			result,				/* =NOMATCH unless object/subobject clicked in */
					measureStf,connStaff;
	Boolean			objSelected,		/* False unless something in the object is selected */
					measDrag;
	DDIST			xd,					/* scratch DDIST coordinates */
					dTop, dLeft,		/* absolute DDIST position of origin (staff or measure) */
					dBottom;
	Rect			rSub,				/* paper-relative subobject rectangle */
					wSub,				/* window-relative of above */
					aRect;				/* scratch */
	unsigned char	dummy=0;

	objSelected = False;
	measDrag = True;
	result = NOMATCH;
	halfWidth = 2;
	aPSMeasL = FirstSubLINK(pL);
	for (i = 0; aPSMeasL; i++, aPSMeasL=NextPSMEASL(aPSMeasL)) {
		aPSMeas = GetPAPSMEAS(aPSMeasL);
		groupTopStf = groupBottomStf = aPSMeas->staffn;
		if (aPSMeas->visible || doc->showInvis) {
			measureStf = NextLimStaffn(doc,pL,True,aPSMeas->staffn);
			pContext = &context[measureStf];
			dTop = pContext->staffTop;
			dLeft = pContext->measureLeft;
			xd = dLeft+LinkXD(pL);
/*		
 * PSMeasure subobjects are unusual in that they may be grouped as indicated by
 *	connAbove and connStaff, and groups can only be selected as a whole. If this
 *	measure is the top one of a group, set rSub and groupTopStf/groupBottomStf
 * to include the entire group; if it's a lower one of a group, they will then
 *	already be set correctly. Notice that this code assumes that when a lower
 *	subobject of a group is encountered, we've already set rSub from the top one
 *	of the group; this is safe as long as we insert subobjects for all staves at
 * once and in order, and don't allow deleting anything but the entire object.
 *  FIXME: staff subobjects may _not_ be in order!
 */ 
			if (!aPSMeas->connAbove) {
				groupTopStf = measureStf;
				if (aPSMeas->connStaff!=0) {
					connStaff = NextLimStaffn(doc,pL,False,aPSMeas->connStaff);
					dBottom = context[connStaff].staffTop+context[connStaff].staffHeight;
					switch (aPSMeas->subType) {
						case PSM_DOTTED:
							SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
										d2p(xd)+halfWidth, d2p(dBottom));
							break;
						case PSM_DOUBLE:
							SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
										d2p(xd)+halfWidth+2, d2p(dBottom));
							break;
						case PSM_FINALDBL:
							SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
										d2p(xd)+halfWidth+3, d2p(dBottom));
							break;
						default:
							break;
					}
					groupBottomStf = connStaff;
				}
				else {
					switch (aPSMeas->subType) {
						case PSM_DOTTED:
							SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
										d2p(xd)+halfWidth,
										d2p(dTop+pContext->staffHeight));
							break;
						case PSM_DOUBLE:
							SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
										d2p(xd)+halfWidth+2,
										d2p(dTop+pContext->staffHeight));
							break;
						case PSM_FINALDBL:
							SetRect(&rSub, d2p(xd)-halfWidth, d2p(dTop), 
										d2p(xd)+halfWidth+3,
										d2p(dTop+pContext->staffHeight));
							break;
						default:
							break;
					}
					groupBottomStf = measureStf;
				}
			}
		}
		wSub = rSub; OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
		switch (mode) {
			case SMClick:
				if (PtInRect(*(Point *)ptr, &rSub)) {
					aPSMeas->selected = !aPSMeas->selected;
					if (!aPSMeas->connAbove) HiliteRect(&wSub);
					result = i;
				}
				break;
			case SMDblClick:
				break;
			case SMDrag:
				UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub (the group)? */
				if (EqualRect(&aRect, (Rect *)ptr)) {
					aPSMeas->selected = !aPSMeas->selected;
					if (!aPSMeas->connAbove) HiliteRect(&wSub);
					result = i;
				}
				break;
			case SMStaffDrag:
				if (groupTopStf>=stfRange.topStaff && groupBottomStf<=stfRange.bottomStaff &&
					!aPSMeas->selected) {
					UnionRect(&rSub, (Rect *)ptr, &aRect);		/* does (Rect *)ptr enclose rSub (the group)? */
					if (EqualRect(&aRect, (Rect *)ptr)) {
						aPSMeas->selected = True;
						if (!aPSMeas->connAbove) HiliteRect(&wSub);
						result = stfRange.topStaff;				/* ??OR SHOULD IT BE groupTopStf? */
					}
				}
				break;
			case SMSelect:
				if (!aPSMeas->selected) {
					aPSMeas->selected = True;
					if (!aPSMeas->connAbove) HiliteRect(&wSub);
				}
				break;
			case SMSelNoHilite:
				aPSMeas->selected = True;
				break;
			case SMDeselect:
				if (aPSMeas->selected) {
					aPSMeas->selected = False;
					if (!aPSMeas->connAbove) HiliteRect(&wSub);
				}
				break;
			case SMSymDrag:

				/* Given a measure with n subobjects, HandleSymDrag will be called
					n times, resulting in the measure being translated n times as
					far as the user actually dragged it. => use measDrag to insure
					only one call to HandleSymDrag. */

				if (PtInRect(*(Point *)ptr, &rSub) && measDrag) {
					HandleSymDrag(doc, pL, aPSMeasL, *(Point *)ptr, dummy);
					measDrag = False;
					aPSMeas->selected = !aPSMeas->selected;
					if (!aPSMeas->connAbove) HiliteRect(&wSub);
				}
				break;
			case SMFind:
				if (PtInRect(*(Point *)ptr, &rSub))
					result = i;
				break;
			case SMHilite:
				if (aPSMeas->selected)
					if (!aPSMeas->connAbove) {
						InsetRect(&wSub, -enlarge.h, -enlarge.v);
						HiliteRect(&wSub);
					}
				break;
			case SMFlash:
				if (!aPSMeas->connAbove) HiliteRect(&wSub);
				break;
			default:
				break;
		}
	if (aPSMeas->selected)
		objSelected = True;
	}
	if (mode!=SMFind) LinkSEL(pL) = objSelected;
	return result;
}


/* ---------------------------------------------------------------------- CheckBEAMSET -- */
/* BEAMSET object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckBEAMSET(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange)
{
	PBEAMSET 	p;
	short		result;			/* =NOMATCH unless object/subobject clicked in */
	Rect		rSub,			/* bounding box for sub-object */
				wSub,			/* window-relative of above */
				aRect;			/* scratch */
	CONTEXT		*pContext;
	unsigned char dummy=0;

PushLock(OBJheap);
	p = GetPBEAMSET(pL);
	if (!LOOKING_AT(doc, p->voice)) {
		LinkSEL(pL) = False;
		PopLock(OBJheap);
		return NOMATCH;
	}

	result = NOMATCH;
	rSub = LinkOBJRECT(pL);
	pContext = &context[p->staffn];
	wSub = rSub;
	OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
			

	switch (mode) {
	case SMClick:
		if (PtInRect(*(Point *)ptr, &rSub)) {
			LinkSEL(pL) = !LinkSEL(pL);
			HiliteRect(&wSub);
			result = 1;
		}
		break;
	case SMDblClick:
		if (PtInRect(*(Point *)ptr, &rSub)) {
			PrepareUndo(doc, pL, U_EditBeam, 4);			/* "Undo Edit Beam" */
			DoBeamEdit(doc, pL);
			result = 1;
		}
		break;
	case SMDrag:
		UnionRect(&rSub, (Rect *)ptr, &aRect);				/* does (Rect *)ptr enclose rSub? */
		if (EqualRect(&aRect, (Rect *)ptr)) {
			LinkSEL(pL) = !LinkSEL(pL);
			HiliteRect(&wSub);
			result = 1;
		}
		break;
	case SMStaffDrag:
		if (p->staffn>=stfRange.topStaff && 
			 p->staffn<=stfRange.bottomStaff && !p->selected) {
			UnionRect(&rSub, (Rect *)ptr, &aRect);			/* does (Rect *)ptr enclose rSub? */
			if (EqualRect(&aRect, (Rect *)ptr)) {
				p->selected = True;
				HiliteRect(&wSub);
				result = stfRange.topStaff;
			}
		}
		break;
	case SMSelect:
		if (!LinkSEL(pL)) {
			LinkSEL(pL) = True;
			HiliteRect(&wSub);
		}
		break;
	case SMSelNoHilite:
		LinkSEL(pL) = True;
		break;
	case SMDeselect:
		if (LinkSEL(pL)) {
			LinkSEL(pL) = False;
			if (CapsLockKeyDown() && OptionKeyDown())
				if (SUSPICIOUS_WREL_RECT(wSub))
					LogPrintf(LOG_NOTICE, " %d:Beam %d,%d,%d,%d\n", pL, wSub.left, wSub.top, wSub.right, wSub.bottom);
			HiliteRect(&wSub);
		}
		break;
	case SMSymDrag:
		if (PtInRect(*(Point *)ptr, &LinkOBJRECT(pL))) {
			HandleSymDrag(doc, pL, NILINK, *(Point *)ptr, dummy);
			LinkSEL(pL) = !LinkSEL(pL);
			HiliteRect(&wSub);
		}
		break;
	case SMFind:
		if (PtInRect(*(Point *)ptr, &rSub))
			result = 1;
		break;
	case SMHilite:
		if (LinkSEL(pL))
			HiliteRect(&wSub);
		break;
	case SMFlash:
		HiliteRect(&wSub);
		break;
	}
PopLock(OBJheap);
	return result;
}


/* ----------------------------------------------------------------------- CheckTUPLET -- */
/* TUPLET object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckTUPLET(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	PTUPLET		p;
	short		result,			/* =NOMATCH unless object/subobject clicked in */
				totalDur;
	Rect		rSub,			/* bounding box for subobject */
				wSub,			/* window-relative of above */
				oldObjRect,
				aRect;
	CONTEXT		*pContext;
	TupleParam	tParam;
	Boolean		okay;

PushLock(OBJheap);
	p = GetPTUPLET(pL);
	if (!LOOKING_AT(doc, p->voice)) {
		LinkSEL(pL) = False;
		PopLock(OBJheap);
		return NOMATCH;
	}

	result = NOMATCH;
	rSub = LinkOBJRECT(pL);
	pContext = &context[p->staffn];
	wSub = rSub;
	OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);

	switch (mode) {
	case SMClick:
		if (PtInRect(*(Point *)ptr, &rSub)) {
			LinkSEL(pL) = !LinkSEL(pL);
			HiliteRect(&wSub);
			result = 1;
		}
		break;
	case SMDblClick:
		InvertTwoSymbolHilite(doc, FirstInTuplet(pL), LastInTuplet(pL), p->staffn);	/* On */
		while (Button()) ;

		oldObjRect = p->objRect;
		
		tParam.accNum = p->accNum;
		tParam.accDenom = p->accDenom;
		totalDur = TupletTotDir(pL);
		tParam.durUnit = GetDurUnit(totalDur, tParam.accNum, tParam.accDenom);
		tParam.numVis = p->numVis;
		tParam.denomVis = p->denomVis;
		tParam.brackVis = p->brackVis;
		okay = TupletDialog(doc, &tParam, False);

		InvertSymbolHilite(doc, FirstInTuplet(pL), p->staffn, False);		/* Hiliting off */
		if (FirstInTuplet(pL)!=LastInTuplet(pL))
			InvertSymbolHilite(doc, LastInTuplet(pL), p->staffn, False);	/* Hiliting off */

		if (okay) {
			p->accNum = tParam.accNum;
			p->accDenom = tParam.accDenom;
			p->numVis = tParam.numVis;
			p->denomVis = tParam.denomVis;
			p->brackVis = tParam.brackVis;
			doc->changed = True;
			/* FIXME: THIS IS EXCESSIVE */
			InvalMeasure(pL, TupletSTAFF(pL));
		}
		break;
	case SMDrag:
		UnionRect(&rSub, (Rect *)ptr, &aRect);				/* does (Rect *)ptr enclose rSub? */
		if (EqualRect(&aRect, (Rect *)ptr)) {
			LinkSEL(pL) = !LinkSEL(pL);
			HiliteRect(&wSub);
			result = 1;
		}
		break;
	case SMStaffDrag:
		if (p->staffn>=stfRange.topStaff && 
			 p->staffn<=stfRange.bottomStaff && !p->selected) {
			UnionRect(&rSub, (Rect *)ptr, &aRect);			/* does (Rect *)ptr enclose rSub? */
			if (EqualRect(&aRect, (Rect *)ptr)) {
				p->selected = True;
				HiliteRect(&wSub);
				result = stfRange.topStaff;
			}
		}
		break;
	case SMSelect:
		if (!LinkSEL(pL)) {
			LinkSEL(pL) = True;
			HiliteRect(&wSub);
		}
		break;
	case SMSelNoHilite:
		LinkSEL(pL) = True;
		break;
	case SMDeselect:
		if (LinkSEL(pL)) {
			LinkSEL(pL) = False;
			HiliteRect(&wSub);
		}
		break;
	case SMSymDrag:
		if (PtInRect(*(Point *)ptr, &rSub)) {
			HandleSymDrag(doc, pL, NILINK,  *(Point *)ptr, (unsigned char)0);
			LinkSEL(pL) = !LinkSEL(pL);
			HiliteRect(&wSub);
		}
		break;
	case SMFind:
		if (PtInRect(*(Point *)ptr, &rSub))
			result = 1;
		break;
	case SMHilite:
		if (LinkSEL(pL)) {
			InsetRect(&wSub, -enlarge.h, -enlarge.v);
			HiliteRect(&wSub);
		}
		break;
	case SMFlash:
		HiliteRect(&wSub);
		break;
	}
PopLock(OBJheap);
	return result;
}


/* ----------------------------------------------------------------------- CheckOTTAVA -- */
/* OTTAVA object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckOTTAVA(Document *doc, LINK pL, CONTEXT context[],
						Ptr ptr,
						short mode,
						STFRANGE stfRange,
						Point enlarge)
{
	POTTAVA		p;
	short		result;			/* =NOMATCH unless object/subobject clicked in */
	Rect		rSub,			/* bounding box for sub-object */
				wSub,			/* window-relative of above */
				aRect;			/* scratch */
	CONTEXT		*pContext;

PushLock(OBJheap);
	result = NOMATCH;
	rSub = LinkOBJRECT(pL);
	pContext = &context[OttavaSTAFF(pL)];
	wSub = rSub;
	OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);

	switch (mode) {
	case SMClick:
		if (PtInRect(*(Point *)ptr, &rSub)) {
			LinkSEL(pL) = !LinkSEL(pL);
			HiliteRect(&wSub);
			result = 1;
		}
		break;
	case SMDblClick:
		InvertSymbolHilite(doc, FirstInOttava(pL), OttavaSTAFF(pL), True);		/* Hiliting on */
		SleepTicks(30L);
		InvertSymbolHilite(doc, FirstInOttava(pL), OttavaSTAFF(pL), True);		/* Hiliting off */
		break;
	case SMDrag:
		UnionRect(&rSub, (Rect *)ptr, &aRect);				/* does (Rect *)ptr enclose rSub? */
		if (EqualRect(&aRect, (Rect *)ptr)) {
			LinkSEL(pL) = !LinkSEL(pL);
			HiliteRect(&wSub);
			result = 1;
		}
		break;
	case SMStaffDrag:
		p = GetPOTTAVA(pL);
		if (p->staffn>=stfRange.topStaff && 
			 p->staffn<=stfRange.bottomStaff && !p->selected) {
			UnionRect(&rSub, (Rect *)ptr, &aRect);			/* does (Rect *)ptr enclose rSub? */
			if (EqualRect(&aRect, (Rect *)ptr)) {
				p->selected = True;
				HiliteRect(&wSub);
				result = stfRange.topStaff;
			}
		}
		break;
	case SMSelect:
		if (!LinkSEL(pL)) {
			LinkSEL(pL) = True;
			HiliteRect(&wSub);
		}
		break;
	case SMSelNoHilite:
		LinkSEL(pL) = True;
		break;
	case SMDeselect:
		if (LinkSEL(pL)) {
			LinkSEL(pL) = False;
			HiliteRect(&wSub);
		}
		break;
	case SMSymDrag:
		if (PtInRect(*(Point *)ptr, &rSub)) {
			HandleSymDrag(doc, pL, NILINK, *(Point *)ptr, (unsigned char)0);
			LinkSEL(pL) = !LinkSEL(pL);
			HiliteRect(&wSub);
		}
		break;
	case SMFind:
		if (PtInRect(*(Point *)ptr, &rSub))
			result = 1;
		break;
	case SMHilite:
		if (LinkSEL(pL)) {
			InsetRect(&wSub, -enlarge.h, -enlarge.v);
			HiliteRect(&wSub);
		}
		break;
	case SMFlash:
		HiliteRect(&wSub);
		break;
	}
PopLock(OBJheap);
	return result;
}


/* ------------------------------------------------------------------------- CheckSLUR -- */
/* SLUR object selecter/highliter.  Does different things depending on the value of
<mode> (see the list above). */

short CheckSLUR(Document *doc, LINK pL, CONTEXT context[],
					Ptr ptr,
					short mode,
					STFRANGE stfRange)
{
	PSLUR		p;
	short		i, result;			/* =NOMATCH unless object/subobject clicked in */
	Rect		rSub,				/* bounding box for sub-object */
				wSub,				/* window-relative of above */
				aRect;				/* scratch */
	CONTEXT		*pContext;
	PASLUR		aSlur;
	LINK		aSlurL;
	Boolean		objSelected;

	if (!LOOKING_AT(doc, SlurVOICE(pL))) {
		LinkSEL(pL) = False;
		return NOMATCH;
	}
	
PushLock(OBJheap);

	p = GetPSLUR(pL);
	result = NOMATCH;
	objSelected = False;
	pContext = &context[SlurSTAFF(pL)];
	aSlurL = FirstSubLINK(pL);

	for (i = 0; aSlurL; i++, aSlurL=NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
		rSub = aSlur->bounds;
		wSub = rSub;
		OffsetRect(&wSub,pContext->paper.left,pContext->paper.top);
		switch (mode) {
		case SMClick:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				aSlur->selected = !aSlur->selected;
				HiliteRect(&wSub);
				result = 1;
			}
			break;
		case SMDblClick:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				PrepareUndo(doc, pL, U_EditSlur, 11);			/* "Undo Edit Slur" */
				HiliteSlurNodes(doc, pL);
				SelectSlur(&pContext->paper,aSlurL);
				DoSlurEdit(doc, pL, aSlurL, i);
				result = 1;
			}
			break;
		case SMDrag:
			UnionRect(&rSub, (Rect *)ptr, &aRect);				/* does (Rect *)ptr enclose rSub? */
			if (EqualRect(&aRect, (Rect *)ptr)) {
				aSlur->selected = !aSlur->selected;
				HiliteRect(&wSub);
				result = 1;
			}
			break;
		case SMStaffDrag:
			if (p->staffn>=stfRange.topStaff && 
				 p->staffn<=stfRange.bottomStaff && !aSlur->selected) {
				UnionRect(&rSub, (Rect *)ptr, &aRect);			/* does (Rect *)ptr enclose rSub? */
				if (EqualRect(&aRect, (Rect *)ptr)) {
					aSlur->selected = True;
					HiliteRect(&wSub);
					result = stfRange.topStaff;
				}
			}
			break;
		case SMSelect:
			if (!aSlur->selected) {
				aSlur->selected = True;
				HiliteRect(&wSub);
			}
			break;
		case SMSelNoHilite:
			aSlur->selected = True;
			break;
		case SMDeselect:
			if (aSlur->selected) {
				aSlur->selected = False;
				if (CapsLockKeyDown() && OptionKeyDown())
					if (SUSPICIOUS_WREL_RECT(wSub))
						LogPrintf(LOG_NOTICE, " %d:Slur %d,%d,%d,%d\n", pL, wSub.left, wSub.top, wSub.right, wSub.bottom);
				HiliteRect(&wSub);
			}
			break;
		case SMSymDrag:
			if (PtInRect(*(Point *)ptr, &rSub)) {
				aSlur->selected = True;			/* This & only this slur must be selected */
												/* 	so that SymDragLoop knows which one to drag */
				HandleSymDrag(doc, pL, NILINK, *(Point *)ptr, (unsigned char)0);
				LinkSEL(pL) = !LinkSEL(pL);
				aSlur->selected = LinkSEL(pL);
				HiliteRect(&wSub);
			}
			break;
		case SMFind:
			if (PtInRect(*(Point *)ptr, &rSub))
				result = i;
			break;
		case SMHilite:
			if (aSlur->selected)
				HiliteRect(&wSub);
			break;
		case SMFlash:
			HiliteRect(&wSub);
			break;
		}
		if (aSlur->selected) objSelected = True;
	}

	if (mode!=SMFind) LinkSEL(pL) = objSelected;
PopLock(OBJheap);
	return result;
}


/* Null function to allow loading or unloading Check.c's segment. */

void XLoadCheckSeg()
{
}
