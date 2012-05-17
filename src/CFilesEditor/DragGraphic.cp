/* DragGraphic.c for Nightingale */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1990-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void InitGraphicBounds(Document *, LINK, Point, Rect *);

static enum {
	NOCONSTRAIN = 0,
	H_CONSTRAIN,
	V_CONSTRAIN
} E_DragGraphicItems;

/* As of v.2.01, DragGraphic is used only for chord symbols. It will need attention
if used for other types of Graphics, especially GRDraws. */

void DragGraphic(Document *doc, LINK graphicL)
{
	Point		oldPt, newPt, grPortOrigin;
	Rect		oldObjRect, destRect, newObjRect, boundsRect, savedPaper;
	GrafPtr	scrnPort, graphicPort;
	long		aLong;
	INT16		dh, dv, oldTxMode, constrain = NOCONSTRAIN;
	Boolean	suppressRedraw = FALSE, firstLoop = TRUE;
	CONTEXT	context[MAXSTAVES+1];
	LINK		measL;
	GRAPHIC	origGraphic, tmpGraphic;
	PGRAPHIC	thisGraphic;
	const BitMap *graphicPortBits = NULL;
	const BitMap *scrnPortBits = NULL;
	Rect 		bounds;
	
	/* Deselect Graphic to prevent flashing when user clicks outside of it,
	 * and to avoid hiliting original Graphic when autoscrolling.
	 */	
	DeselectNode(graphicL);

PushLock(OBJheap);
PushLock(GRAPHICheap);

	thisGraphic = GetPGRAPHIC(graphicL);
	origGraphic = *thisGraphic;
	oldObjRect = LinkOBJRECT(graphicL);				/* paper-rel pixels (dependent on mag) */
	InsetRect(&oldObjRect, -pt2p(4), -pt2p(5));	/* Takes care of Sonata Graphics, whose chars exceed the objRect. Other fonts not significantly affected. */
	
	if (thisGraphic->firstObj && PageTYPE(thisGraphic->firstObj))	/* is page-relative */
		measL = LSSearch(thisGraphic->firstObj, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	else {
		measL = LSSearch(graphicL, MEASUREtype, thisGraphic->staffn, GO_LEFT, FALSE);		/* meas containing Graphic */
		if (measL==NILINK || !SameSystem(measL, thisGraphic->firstObj))						/* Graphic attached to, or to left of, initial barline */
			measL = LSSearch(thisGraphic->firstObj, MEASUREtype, thisGraphic->staffn, GO_RIGHT, FALSE);
	}
	GetAllContexts(doc, context, measL);
	
	GetPaperMouse(&oldPt, &doc->currentPaper);
	InitGraphicBounds(doc, graphicL, oldPt, &boundsRect);

	oldTxMode = GetPortTxMode();

	/* Leave initial Graphic in gray. Have to fake out DrawGRAPHIC by fiddling
	 * with lookVoice value.
	 */
/*#define DRAW_IN_GRAY*/
#ifdef DRAW_IN_GRAY
	TextMode(srcXor);
	DrawGRAPHIC(doc, graphicL, context, TRUE);		/* erase black Graphic */
/* ??GROSS HACK ALERT !! Should have a lookAtNoVoice value for this purpose. */
	lookAtVal = doc->lookVoice;
	doc->lookVoice = MAXVOICES+1;							/* to make DrawGRAPHIC... */
	TextMode(srcOr);
	DrawGRAPHIC(doc, graphicL, context, TRUE);		/* draw it in gray */
	doc->lookVoice = lookAtVal;
#endif

	/* Set up offscreen port to hold image of Graphic being dragged. */
//#undef USE_GWORLDS	/* because can't use srcXor mode with color grafports */
#ifdef USE_GWORLDS
	graphicPort = (GrafPtr)MakeGWorld(oldObjRect.right-oldObjRect.left,
														oldObjRect.bottom-oldObjRect.top, TRUE);
#else
	graphicPort = NewGrafPort(oldObjRect.right-oldObjRect.left, oldObjRect.bottom-oldObjRect.top);
#endif
	if (graphicPort==NULL)
		return;

	GetPort(&scrnPort);
	SetPort(graphicPort);
	
	/* Fiddle with bitmap coordinate system so that DrawGRAPHIC will draw into our tiny rect. */
	grPortOrigin = TOP_LEFT(oldObjRect);
	Pt2Window(doc, &grPortOrigin);
	SetOrigin(grPortOrigin.h, grPortOrigin.v);
	GetPortBounds(graphicPort,&bounds);
	ClipRect(&bounds);

	DrawGRAPHIC(doc, graphicL, context, TRUE);			/* draw Graphic into bitmap */

	SetPort(scrnPort);

	destRect = oldObjRect;
	Rect2Window(doc, &destRect);

	FlushEvents(everyEvent, 0);

	if (StillDown()) while (WaitMouseUp()) {
		GetPaperMouse(&newPt, &doc->currentPaper);

		/* Force the code below to see the mouse as always within boundsRect. */
		aLong = PinRect(&boundsRect, newPt);
		newPt.v = HiWord(aLong);	newPt.h = LoWord(aLong);

		if (EqualPt(newPt, oldPt)) continue;

		if (firstLoop) {
			if (ShiftKeyDown())
				constrain = (ABS(newPt.h-oldPt.h) >= ABS(newPt.v-oldPt.v)) ?
												H_CONSTRAIN : V_CONSTRAIN;
			firstLoop = FALSE;
		}
		
#ifndef DRAW_IN_GRAY
		else {						/* don't erase orignal image */
			graphicPortBits = GetPortBitMapForCopyBits(graphicPort);
			scrnPortBits = GetPortBitMapForCopyBits(scrnPort);
			GetPortBounds(graphicPort,&bounds);
			CopyBits(graphicPortBits, scrnPortBits, &bounds, &destRect, srcXor, NULL);

	//		CopyBits(&graphicPort->portBits, &scrnPort->portBits,		/* erase old Graphic */
	//		 			&(*graphicPort).portBits.bounds, &destRect, srcXor, NULL);
		}

#endif
		dh = newPt.h - oldPt.h;
		dv = newPt.v - oldPt.v;

		if (constrain==H_CONSTRAIN) dv = 0;
		if (constrain==V_CONSTRAIN) dh = 0;
		
		thisGraphic->xd += p2d(dh);
		thisGraphic->yd += p2d(dv);

		/* shift on-screen destination rect */
		destRect.top += dv;		destRect.bottom += dv;
		destRect.left += dh;		destRect.right += dh;

		/* We need to restore temporarily the original location of the Graphic;
		 * otherwise AutoScroll will leave a trail of text images as we scroll.
		 * Also we have to save and restore doc->currentPaper, because AutoScroll
		 * will change it if mouse moves over another sheet of paper.
		 */
		savedPaper = doc->currentPaper;
		tmpGraphic = *thisGraphic;
		*thisGraphic = origGraphic;
		AutoScroll();
		*thisGraphic = tmpGraphic;
		doc->currentPaper = savedPaper;

		graphicPortBits = GetPortBitMapForCopyBits(graphicPort);
		scrnPortBits = GetPortBitMapForCopyBits(scrnPort);
		GetPortBounds(graphicPort,&bounds);
		
		CopyBits(graphicPortBits, scrnPortBits, &bounds, &destRect, srcXor, NULL);
					
//		CopyBits(&graphicPort->portBits, &scrnPort->portBits,		/* draw new Graphic */
//		 			&(*graphicPort).portBits.bounds, &destRect, srcXor, NULL);
		
		oldPt = newPt;
	}
	
	DrawGRAPHIC(doc, graphicL, context, TRUE);		/* draw final black Graphic */

	TextMode(oldTxMode);

	/* Inval old and new location */
	newObjRect = LinkOBJRECT(graphicL);							/* DrawGRAPHIC has updated this */
	Rect2Window(doc, &newObjRect);
	if (BlockCompare(thisGraphic, &origGraphic, sizeof(GRAPHIC))) {
		doc->changed = TRUE;
		LinkTWEAKED(graphicL) = TRUE;									/* Flag to show node was edited */
		Rect2Window(doc, &oldObjRect);
		if (!suppressRedraw)
			EraseAndInval(&oldObjRect);
	}
InsetRect(&newObjRect, -pt2p(2), -pt2p(2));	/* ??seems to fix problem with chord sym objrects */
	EraseAndInval(&newObjRect);					/* must do this even if no change, to keep hiliting in sync */

#ifdef USE_GWORLDS
	DestroyGWorld((GWorldPtr)graphicPort);
#else
	DisposGrafPort(graphicPort);
#endif

	MEHideCaret(doc);
	LinkSEL(graphicL) = TRUE;								/* so that Graphic will hilite after we're done dragging */
	thisGraphic->selected = TRUE;
	doc->selStartL = graphicL;	doc->selEndL = RightLINK(graphicL);

PopLock(OBJheap);
PopLock(GRAPHICheap);
}


/* ------------------------------------------------------------ InitGraphicBounds -- */
/* Set up the rectangle beyond which the user won't be allowed to drag the Graphic.
 *	(Very similar to InitDynamicBounds.) */

#define SYSHT_SLOP 400		/* DDIST */

static void InitGraphicBounds(Document *doc, LINK graphicL,
										Point mousePt, Rect *bounds)	/* paper coords */
{
	INT16			staffn, mouseFromLeft, mouseFromRight, mouseFromTop, mouseFromBottom;
	Rect			objRect;
	Boolean		dragIntoReserved = FALSE;
	CONTEXT		context;
	PAMEASURE	aMeasP;
	DDIST			sysLeft, sysTop, sysRight, sysBot, measWid;
	LINK			firstObjL, inMeasL, firstMeasL, prevMeasL, nextMeasL,
					leftTargetMeasL, rightTargetMeasL;

	objRect = LinkOBJRECT(graphicL);

	mouseFromLeft = objRect.left - mousePt.h;
	mouseFromRight = objRect.right - mousePt.h;
	mouseFromTop = objRect.top - mousePt.v;
	mouseFromBottom = objRect.bottom - mousePt.v;
	
	firstObjL = GraphicFIRSTOBJ(graphicL);
	
	if (PageTYPE(firstObjL)) {							/* handle page relative graphics */
		*bounds = doc->currentPaper;
		OffsetRect(bounds, -doc->currentPaper.left, -doc->currentPaper.top);
	}	
	else {													/* handle all other graphics */
		staffn = GraphicSTAFF(graphicL);
		GetContext(doc, graphicL, staffn, &context);
		sysLeft = context.systemLeft;
		sysTop = context.systemTop;
		sysRight = context.staffRight;
		sysBot = context.systemBottom;
	
		/* largest acceptable rect */
		bounds->left = d2p(sysLeft);
		bounds->top = d2p(sysTop - SYSHT_SLOP);
		bounds->right = d2p(sysRight);
		bounds->bottom = d2p(sysBot + SYSHT_SLOP);
		
		/* Constrain further, so that the Graphic will never be farther than one
		 * measure away from the (first) object it's attached to.
		 */

		/* Find the measure that will be the left boundary */
		if (MeasureTYPE(firstObjL)) {
			inMeasL = firstObjL;
			if (FirstMeasInSys(firstObjL)) leftTargetMeasL = inMeasL;
			else leftTargetMeasL = LinkLMEAS(inMeasL);
		}
		else {
			inMeasL = LSSearch(firstObjL, MEASUREtype, staffn, GO_LEFT, FALSE);			/* meas containing firstObjL */
			if (inMeasL==NILINK || !SameSystem(inMeasL, firstObjL))							/* firstObjL is before 1st meas of its system */
				leftTargetMeasL = inMeasL = LSSearch(firstObjL, MEASUREtype, staffn, GO_RIGHT, FALSE);
			else {
				prevMeasL = LinkLMEAS(inMeasL);														/* meas before meas containing firstObjL */
				if (SameSystem(prevMeasL, inMeasL))
					leftTargetMeasL = prevMeasL;
				else
					leftTargetMeasL = inMeasL;
			}
		}

		/* Allow dragging into reserved area if leftTargetMeasL is first meas of system. */
		firstMeasL = LSSearch(MeasSYSL(leftTargetMeasL), MEASUREtype, staffn, GO_RIGHT, FALSE);	/* 1st meas in system */	
		if (leftTargetMeasL == firstMeasL) {															/* targetMeas is 1st in system */
			bounds->left = d2p(sysLeft);
			dragIntoReserved = TRUE;			/* ??unused below! */
		}
		else
			bounds->left = d2p(LinkXD(leftTargetMeasL) + sysLeft);
	
		/* Find the measure that will be the right boundary */
		nextMeasL = LinkRMEAS(inMeasL);									/* meas after the one containing firstObjL */
		if (nextMeasL==NILINK)												/* nextMeasL is last in file */
			bounds->right = d2p(sysRight);
		else {
			if (SameSystem(nextMeasL, leftTargetMeasL))
				rightTargetMeasL = nextMeasL;
			else
				rightTargetMeasL = inMeasL;
			aMeasP = GetPAMEASURE(FirstSubLINK(rightTargetMeasL));	/* get xd of END of meas */
			measWid = aMeasP->measureRect.right;
			bounds->right = d2p(LinkXD(rightTargetMeasL) + measWid + sysLeft);
		}
	}
	
	/* Offset boundaries depending on mouse position relative to Graphic. (PinRect
	 * in DragGraphic decides whether mousePt, not Graphic, is within bounds.)
	 */
	bounds->left -= mouseFromLeft;
	bounds->right -= mouseFromRight-2;
	bounds->top -= mouseFromTop;
	bounds->bottom -= mouseFromBottom;
}
