/******************************************************************************************
*	FILE:	Dragging.c
*	PROJ:	Nightingale
*	DESC:	Bitmap dragging routines
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define DragXD(xd)	( (xd) - p2d(dragOffset) )
#define DragXP(xp)	( (xp) - dragOffset )

/* Local Prototypes */

static DDIST GetXDDiff(Document *doc, LINK pL, PCONTEXT pContext);

/* Drawing for feedback during symbol dragging */

static void SDDrawClef(Document *doc, LINK pL, LINK subObjL, unsigned char dummyglyph, LINK measureL);
static void SDDrawKeySig(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void SDDrawTimeSig(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void SDDrawAugDots(Document *doc, LINK aNoteL, DDIST xd, DDIST yd, DDIST dhalfLn);
static void SDDrawMBRest(Document *doc,PCONTEXT pContext,LINK theRestL,DDIST xd,DDIST yd);
static void SDDrawRest(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void SDDrawNote(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void SDDrawGRNote(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void SDDrawDynamic(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void SDDrawRptEnd(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void MoveAndLineTo(short x1, short y1, short x2, short y2);
static void SDDrawEnding(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void SDDrawMeasure(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void SDDrawPSMeas(Document *doc, LINK pL, LINK subObjL, LINK measureL);
static void SDDrawTuplet(Document *doc, LINK pL, LINK measureL);
static void SDDrawArpSign(Document *, DDIST, DDIST, DDIST, short, PCONTEXT);
static void SDDrawGRDraw(Document *, DDIST, DDIST, DDIST, DDIST, short, PCONTEXT); 
static void SDDrawGraphic(Document *doc, LINK pL, LINK measureL);
static void SDDrawTempo(Document *doc, LINK pL, LINK measureL);
static void SDDrawSpace(Document *doc, LINK pL, LINK measureL);
static void SDDrawOttava(Document *doc, LINK pL, LINK measureL);
static void SDDrawBeam(DDIST xl,DDIST yl,DDIST xr,DDIST yr,DDIST beamThick,short upOrDown, PCONTEXT pContext);
static void SDDrawGRBeamset(Document *doc, LINK pL, LINK measureL);
static DDIST SDCalcXStem(LINK, short, short, DDIST, DDIST, Boolean);
static void SDDrawBeamset(Document *doc, LINK pL, LINK measureL);
static void SDDrawSlur(Document *doc, LINK pL, LINK measureL);

static Boolean SymDragLoop(Document *doc, LINK, LINK, unsigned char, Point, LINK);


/* ====================================================================================== */
/* Functions for drawing individual symbols into the offScreen bitmap picBits, for the
benefit of SymDragLoop. */

static DDIST GetXDDiff(Document *doc, LINK pL, PCONTEXT pContext)
{
	if (LinkBefFirstMeas(pL))
		return GetSysLeft(doc);

	return pContext->measureLeft;
}

static void SDDrawClef(Document *doc, LINK pL, LINK subObjL, unsigned char /*dummyglyph*/,	/* FIXME: remove this arg soon */
								LINK measureL)
{
	DDIST		xd, yd, xdOct, ydOct, lnSpace;
	short		oldtxSize;
	unsigned char glyph;
	CONTEXT 	context[MAXSTAVES+1], localContext;
	Rect		mRect;
	
	GetContext(doc, pL, ClefSTAFF(subObjL), &localContext);
	context[ClefSTAFF(subObjL)] = localContext;
	oldtxSize = GetPortTxSize();
	
	GetClefDrawInfo(doc, pL, subObjL, context, 100, &glyph, &xd, &yd, &xdOct, &ydOct);

	glyph = MapMusChar(doc->musFontInfoIndex, glyph);
	lnSpace = LNSPACE(&localContext);
	xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

	xd -= GetXDDiff(doc,pL,&localContext);

	mRect = SDGetMeasRect(doc, pL, measureL);

	DMoveTo(DragXD(xd), yd-p2d(mRect.top));
	DrawChar(glyph);
	TextSize(oldtxSize);
}


static void SDDrawKeySig(Document *doc, LINK pL, LINK subObjL, LINK measureL)
{
	DDIST xd, yd, KSyd, dTop, height;
	short lines, width;
	CONTEXT context[MAXSTAVES+1], localContext;
	Rect mRect;
	
	GetContext(doc, pL, KeySigSTAFF(subObjL), &localContext);
	context[KeySigSTAFF(subObjL)] = localContext;
	
	GetKeySigDrawInfo(doc, pL, subObjL, context, &xd, &yd, &dTop, &height, &lines);

	xd -= GetXDDiff(doc,pL,&localContext);
	
	mRect = SDGetMeasRect(doc, pL, measureL);
	KSyd = yd-p2d(mRect.top);
 
	DrawKSItems(doc, pL,subObjL,&localContext,DragXD(xd),KSyd,height,lines,&width,SDDraw);
}


static void SDDrawTimeSig(Document *doc, LINK pL, LINK subObjL, LINK measureL)
{
	short xp,subType;
	DDIST dTop, xd, xdN, xdD, ydN, ydD;
	unsigned char nStr[20], dStr[20];
	CONTEXT localContext;
	Rect mRect;

	GetContext(doc, pL, TimeSigSTAFF(subObjL), &localContext);
	GetTimeSigDrawInfo(doc, pL, subObjL, &localContext, &xd, &dTop);
	
	xd -= GetXDDiff(doc,pL,&localContext);

	mRect = SDGetMeasRect(doc, pL, measureL);
	dTop -= p2d(mRect.top);

	subType = FillTimeSig(doc,subObjL,&localContext,nStr,dStr,xd,&xdN,&xdD,dTop,&ydN,&ydD);

	xp = d2p(xdN);
	MoveTo(DragXP(xp), d2p(ydN));
	DrawString(nStr);

	if (subType==N_OVER_D) {
		xp = d2p(xdD);	
		MoveTo(DragXP(xp), d2p(ydD));
		DrawString(dStr);
	}
}

/* --------------------------------------------------------- Notes, rests, grace notes -- */

/* Draw augmentation dots for the note or rest <aNoteL>. */

static void SDDrawAugDots(Document *doc, LINK aNoteL, DDIST xd, DDIST yd, DDIST dhalfLn)
{
	short ndots; PANOTE aNote;
	Byte glyph = MapMusChar(doc->musFontInfoIndex, MCH_dot);

	aNote = GetPANOTE(aNoteL);
	ndots = aNote->ndots;									/* Draw augmentation dots */
	xd += dhalfLn;
	yd += (aNote->yMoveDots-2)*dhalfLn;
// NB: The orig code here doesn't give same result as DrawAugDots anyway!
	xd += MusCharXOffset(doc->musFontInfoIndex, glyph, dhalfLn*2);
	yd += MusCharYOffset(doc->musFontInfoIndex, glyph, dhalfLn*2);
	while (ndots>0) {
		xd += 2*dhalfLn;
		MoveTo(d2p(xd), d2p(yd));
		DrawChar(glyph);
		ndots--;
	}
}

static void SDDrawMBRest(Document *doc, PCONTEXT pContext, LINK theRestL,
									DDIST xd, DDIST yd)
{
	DDIST dTop, lnSpace, dhalfLn, xdNum, ydNum, endBottom, endTop;
	DRect dRestBar;  Rect restBar;  short numWidth;
	unsigned char numStr[20];
	short face;

	GetMBRestBar(-NoteType(theRestL), pContext, &dRestBar);
	OffsetDRect(&dRestBar, xd, yd);

	NumToString((long)(-NoteType(theRestL)), numStr);
	MapMusPString(doc->musFontInfoIndex, numStr);
	face = GetPortTxFace();
	numWidth = NPtStringWidth(doc,numStr,doc->musicFontNum,d2pt(pContext->staffHeight), face);
	xdNum = (dRestBar.right+dRestBar.left)/2-pt2d(numWidth)/2;
	
	lnSpace = LNSPACE(pContext);
	dhalfLn = lnSpace/2;
	endBottom = yd-lnSpace;
	endTop = yd+lnSpace;
	dTop = yd;
	
	/* Set number y-pos., considering that origins of Sonata digits bisect them. */
	
	ydNum = dTop-3*lnSpace-dhalfLn;

	/* Assuming the first element is as wide/tall as any following ones... */
	
	xdNum += MusCharXOffset(doc->musFontInfoIndex, numStr[1], lnSpace);
	ydNum += MusCharYOffset(doc->musFontInfoIndex, numStr[1], lnSpace);

	/* Draw the thick bar first; then the thin vertical lines at each end of the thick
	   bar; and finally the number. */
	   
	D2Rect(&dRestBar, &restBar);
	PaintRect(&restBar);

	MoveTo(restBar.left, d2p(endBottom));
	LineTo(restBar.left, d2p(endTop));
	MoveTo(restBar.right, d2p(endBottom));
	LineTo(restBar.right, d2p(endTop));

	MoveTo(d2p(xdNum), d2p(ydNum));
	DrawString(numStr);
}


static void SDDrawRest(Document *doc, LINK pL, LINK subObjL, LINK measureL)
{
	short	ndots, yMoveDots, glyph;
	CONTEXT	context;
	DDIST	xd, yd, ydNorm, dTop, lnSpace, restxd, restyd;
	PANOTE	aRest;
	Rect	mRect;
	char	l_dur;
	
	mRect = SDGetMeasRect(doc, pL, measureL);
	aRest = GetPANOTE(subObjL);
	ndots = aRest->ndots;
	yMoveDots = aRest->ndots;

	GetContext(doc, pL, aRest->staffn, &context);
	glyph = GetRestDrawInfo(doc,pL,subObjL,&context,&xd,&yd,&ydNorm,&dTop,&l_dur);

	xd = DragXD(xd) - context.measureLeft;
	yd -= p2d(mRect.top);
	ydNorm -= p2d(mRect.top);	
	
	if (aRest->subType<WHOLEMR_L_DUR) {
		SDDrawMBRest(doc, &context, subObjL, xd, yd); return;
	}

	lnSpace = LNSPACE(&context);

	glyph = MapMusChar(doc->musFontInfoIndex, glyph);
	restxd = xd + MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	restyd = yd + MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

	MoveTo(d2p(restxd), d2p(restyd));
	DrawChar(glyph);									/* Draw the rest */

	SDDrawAugDots(doc, subObjL, xd, ydNorm, lnSpace/2);
}


static void SDDrawNote(Document *doc, LINK pL, LINK subObjL, LINK measureL)
{
	short		flagCount, xhead, yhead,
				headWidth, ypStem,
				octaveLength, useTxSize,
				sizePercent, glyph, noteType;
	CONTEXT		context;
	DDIST		xd, yd, dTop, dLeft,	/* abs DDIST position of subobject */
				accXOffset,				/* x-position offset for accidental */
				dhalfLn,				/* Distance between staff half-lines */
				fudgeHeadY,				/* Correction for roundoff in screen font head shape */
				breveFudgeHeadY;		/* Correction for error in Sonata breve origin */
	Boolean		stemDown;
	PANOTE		aNote, extNote;
	LINK		extNoteL;
	Rect		headRect, mRect;
	short		flagLeading, xadjhead, yadjhead, appearance, headRelSize, stemShorten;
	DDIST		offset, lnSpace, dStemLen;
	unsigned char flagGlyph;

PushLock(OBJheap);
PushLock(NOTEheap);

	if (NoteREST(subObjL)) {
		SDDrawRest(doc, pL, subObjL, measureL);
		goto Cleanup;
	}

	mRect = SDGetMeasRect(doc, pL, measureL);

	aNote = GetPANOTE(subObjL);
	GetContext(doc, pL, aNote->staffn, &context);
	dLeft = LinkXD(pL);
	dTop = context.measureTop;
	xd = dLeft + aNote->xd;									/* measure-rel position of note */
	xd = DragXD(xd);
	if (aNote->otherStemSide) {
		for (extNoteL=FirstSubLINK(pL); extNoteL; extNoteL=NextNOTEL(extNoteL)) {
			extNote = GetPANOTE(extNoteL);
			if (extNote->staffn==aNote->staffn && MainNote(extNoteL)) {
				stemDown = (extNote->ystem > extNote->yd);		/* Is stem up or down? */
				break;
			}
		}
		headRect = CharRect(MapMusChar(doc->musFontInfoIndex, MCH_halfNoteHead));
		headWidth = p2d(headRect.right-headRect.left)+1;
		if (stemDown)	xd -= headWidth;
		else			xd += headWidth;
	}
	lnSpace = LNSPACE(&context);
	dhalfLn = lnSpace/2;
	yd = dTop + aNote->yd;
	yd -= p2d(mRect.top);

	noteType = aNote->subType;
	if (noteType>=WHOLE_L_DUR && aNote->doubleDur) noteType--;

	if (noteType==BREVE_L_DUR) breveFudgeHeadY = dhalfLn*BREVEYOFFSET;
	else								breveFudgeHeadY = (DDIST)0;
	glyph = MCH_quarterNoteHead;									/* Set default */
	
	useTxSize = UseMTextSize(context.fontSize, doc->magnify);
	if (aNote->small) useTxSize = SMALLSIZE(useTxSize);			/* A bit crude; cf. Ross */
	sizePercent = (aNote->small? SMALLSIZE(100) : 100);
	TextSize(useTxSize);

	fudgeHeadY = GetYHeadFudge(useTxSize);						/* Get Y-offset for notehead */

	if (aNote->accident!=0) {
		Byte accGlyph = MapMusChar(doc->musFontInfoIndex, SonataAcc[aNote->accident]);
		DDIST xoff = MusCharXOffset(doc->musFontInfoIndex, accGlyph, lnSpace);
		DDIST yoff = MusCharYOffset(doc->musFontInfoIndex, accGlyph, lnSpace);
		accXOffset = SizePercentSCALE(AccXOffset(aNote->xmoveAcc, &context));
		MoveTo(d2p((xd+xoff)-accXOffset), d2p(yd+yoff));
		DrawChar(accGlyph);
	}

	appearance = aNote->headShape;
	if ((appearance==NO_VIS || appearance==NOTHING_VIS) && doc->showInvis)
		appearance = NORMAL_VIS;
	glyph = GetNoteheadInfo(appearance, noteType, &headRelSize, &stemShorten);
	if (glyph=='\0')							/* just use qtr notehead for chord slash */
		glyph = MCH_quarterNoteHead;
	glyph = MapMusChar(doc->musFontInfoIndex, glyph);

	/* xhead,yhead is position of notehead; xadjhead,yadjhead is position of notehead 
		with any offset applied (might be true if music font is not Sonata). */
		
	offset = MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	if (offset) {
		xhead = d2p(xd);
		xadjhead = d2p(xd+offset);
	}
	else
		xhead = xadjhead = d2p(xd);
	offset = MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
	if (offset) {
		short yp = d2p(yd);
		yhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
		yp = d2p(yd+offset);
		yadjhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
	}
	else
		yhead = yadjhead = d2p(yd+breveFudgeHeadY) + fudgeHeadY;

	if (noteType<=WHOLE_L_DUR) {									/* Handle whole/breve/unknown dur */
		MoveTo(xadjhead, yadjhead);									/* position to draw head */
		DrawChar(glyph);
	}
	else {
		MoveTo(xadjhead, yadjhead);									/* position to draw head */

		/* HANDLE STEMMED NOTE. If the note is in a chord and is stemless, skip the
		   entire business of drawing the stem and flags. */
		   
		if (!aNote->inChord || MainNote(subObjL)) {
			short stemSpace;
			
			dStemLen = aNote->ystem-aNote->yd;
			stemDown = (dStemLen>0);
			if (stemDown)
				stemSpace = 0;
			else {
				stemSpace = MusFontStemSpaceWidthPixels(doc, doc->musFontInfoIndex, lnSpace);
				Move(stemSpace, 0);
			}
			if (ABS(dStemLen)>pt2d(1)) {
				Move(0, -1);
				Line(0, d2p(dStemLen)+(stemDown? 0 : 1));
			}

			/*	Suppress flags if the note is beamed. */
			
			flagCount = (aNote->beamed? 0 : NFLAGS(noteType));

			if (flagCount) {											/* Draw any flags */
				ypStem = yhead+d2p(dStemLen);

				if (doc->musicFontNum==sonataFontNum) {
					/* The 8th and 16th note flag characters in Sonata have their origins
					   set so, in theory, they're positioned properly if drawn from the
					   point where the note head was drawn. Unfortunately, the vertical
					   position depends on the stem length, which Adobe incorrectly
					   assumed was always one octave. The "extend flag" characters have
					   their vertical origins set right where they go. */
						
					MoveTo(xhead, ypStem);								/* Position x at head, y at stem end */
					octaveLength = d2p(7*SizePercentSCALE(dhalfLn));
					if (stemDown)						 				/* Adjust for flag origin */
						Move(0, -octaveLength);
					else
						Move(0, octaveLength);
					if (flagCount==1) {									/* Draw one (8th note) flag */
						if (stemDown)
							DrawChar(MCH_eighthFlagDown);
						else
							DrawChar(MCH_eighthFlagUp);
					}
					else if (flagCount>1) {								/* Draw >=2 (16th & other) flags */
						if (stemDown)
							DrawChar(MCH_16thFlagDown);
						else
							DrawChar(MCH_16thFlagUp);
						MoveTo(xhead, ypStem);							/* Position x at head, y at stem end */
						
						/* Scale Factors for FlagLeading are guesswork. */
						
						if (stemDown)
							Move(0, -d2p(13*FlagLeading(lnSpace)/4));
						else
							Move(0, d2p(7*FlagLeading(lnSpace)/2));
						while (flagCount-- > 2) {
							if (stemDown) {
								DrawChar(MCH_extendFlagDown);
								Move(0, -d2p(FlagLeading(lnSpace)));	/* Unclear. */
							}
							else {
								DrawChar(MCH_extendFlagUp);
								Move(0, d2p(FlagLeading(lnSpace))); 	/* Unclear. */
							}
						}
					}
				}
				else {			/* fonts other than Sonata */
					short glyphWidth;  DDIST xoff, yoff;

					if (MusFontUpstemFlagsHaveXOffset(doc->musFontInfoIndex))
						stemSpace = 0;

					MoveTo(xhead+stemSpace, ypStem);						/* x, y of stem end */

					if (flagCount==1) {										/* Draw 8th flag. */
						flagGlyph = MapMusChar(doc->musFontInfoIndex, 
															(stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp));
						xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
						yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
						if (xoff || yoff)
							Move(d2p(xoff), d2p(yoff));
						DrawChar(flagGlyph);
					}
					else if (flagCount==2 && MusFontHas16thFlag(doc->musFontInfoIndex)) {	/* Draw 16th flag using one flag char. */
						flagGlyph = MapMusChar(doc->musFontInfoIndex, 
															(stemDown? MCH_16thFlagDown : MCH_16thFlagUp));
						xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
						yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
						if (xoff || yoff)
							Move(d2p(xoff), d2p(yoff));
						DrawChar(flagGlyph);
					}
					else {													/* Draw using multiple flag chars */
						short count = flagCount;

						/* Draw extension flag(s) */
						
						if (stemDown) {
							flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_extendFlagDown);
							flagLeading = -d2p(DownstemExtFlagLeading(doc->musFontInfoIndex, lnSpace));
						}
						else {
							flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_extendFlagUp);
							flagLeading = d2p(UpstemExtFlagLeading(doc->musFontInfoIndex, lnSpace));
						}
						xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
						yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
						if (yoff || xoff)
							Move(d2p(xoff), d2p(yoff));
						glyphWidth = CharWidth(flagGlyph);
						while (count-- > 1) {
							DrawChar(flagGlyph);
							Move(-glyphWidth, flagLeading);
						}

						/* Draw 8th flag */
						MoveTo(xhead+stemSpace, ypStem);				/* start again from x,y of stem end */
						Move(0, flagLeading*(flagCount-2));				/* flag leadings of all but one of prev flags */
						if (stemDown) {
							flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_eighthFlagDown);
							flagLeading = -d2p(Downstem8thFlagLeading(doc->musFontInfoIndex, lnSpace));
						}
						else {
							flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_eighthFlagUp);
							flagLeading = d2p(Upstem8thFlagLeading(doc->musFontInfoIndex, lnSpace));
						}
						xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
						yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
						if (yoff || xoff)
							Move(d2p(xoff), d2p(yoff));
						Move(0, flagLeading);
						DrawChar(flagGlyph);
					}
				}
			}
		}
		MoveTo(xadjhead, yadjhead);
		DrawChar(glyph);													/* Finally, draw notehead */
	}
	SDDrawAugDots(doc, subObjL, xd, yd, dhalfLn);

Cleanup:
PopLock(OBJheap);
PopLock(NOTEheap);
}


static void SDDrawGRNote(Document *doc, LINK pL, LINK subObjL, LINK measureL)
{
	PAGRNOTE	aGRNote;
	short		flagCount,staffn,
				xhead,yhead,headWidth,
				ypStem, octaveLength,
				glyph,useTxSize,oldtxSize,
				sizePercent;			/* Percent of "normal" size to draw in (for small grace notes) */
	CONTEXT		context;
	DDIST		xd, yd, dTop, dLeft,
				dhalfLn, fudgeHeadY,
				accXOffset,				/* x-position offset for accidental */
				breveFudgeHeadY;		/* Correction for Sonata error in breve origin */
	Boolean		stemDown;
	PAGRNOTE	extGRNote;
	LINK		extGRNoteL;
	Rect		headRect,mRect;
	short		flagLeading, xadjhead, yadjhead, appearance, headRelSize, stemShorten;
	DDIST		offset, lnSpace, dStemLen;
	unsigned char flagGlyph;

	/* FIXME: This function repeatedly uses DrawMChar, though it never dims. SDDrawNote
	   just uses DrawChar, and it seems like SDDrawGRNote and SDDrawNote should agree! */
	 
PushLock(OBJheap);
PushLock(GRNOTEheap);
	mRect = SDGetMeasRect(doc, pL, measureL);

	aGRNote = GetPAGRNOTE(subObjL);
	staffn = GRNoteSTAFF(subObjL);
	GetContext(doc, pL, staffn, &context);
	dLeft = LinkXD(pL);
	dTop = context.measureTop;
	xd = dLeft + aGRNote->xd;										/* abs. position of subobject */
	xd = DragXD(xd);
	if (aGRNote->otherStemSide) {
		extGRNoteL = FirstSubLINK(pL);
		for ( ; extGRNoteL; extGRNoteL = NextGRNOTEL(extGRNoteL)) {
			extGRNote = GetPAGRNOTE(extGRNoteL);
			if (GRNoteVOICE(extGRNoteL)==GRNoteVOICE(subObjL) &&
					GRMainNote(extGRNoteL)) {
				stemDown = (extGRNote->ystem > extGRNote->yd);		/* Is stem up or down? */
				break;
			}
		}
		headRect = CharRect(MapMusChar(doc->musFontInfoIndex, MCH_halfNoteHead));
		headWidth = p2d(headRect.right-headRect.left)+1;
		if (stemDown)	xd -= headWidth;
		else			xd += headWidth;
	}
	lnSpace = LNSPACE(&context);
	dhalfLn = lnSpace/2;
	yd = dTop + aGRNote->yd;
	yd -= p2d(mRect.top);
	if (aGRNote->subType==BREVE_L_DUR)	breveFudgeHeadY = dhalfLn*BREVEYOFFSET;
	else								breveFudgeHeadY = (DDIST)0;

	glyph = MCH_quarterNoteHead;									/* Set default */
	
	sizePercent = (aGRNote->small? SMALLSIZE(GRACESIZE(100)) : GRACESIZE(100));
	oldtxSize = GetPortTxSize();
	useTxSize = UseMTextSize(SizePercentSCALE(context.fontSize), doc->magnify);
	TextSize(useTxSize);

	fudgeHeadY = GetYHeadFudge(useTxSize);						/* Get fine Y-offset for notehead */

	if (aGRNote->accident!=0) {
		Byte accGlyph = MapMusChar(doc->musFontInfoIndex, SonataAcc[aGRNote->accident]);
		DDIST xoff = MusCharXOffset(doc->musFontInfoIndex, accGlyph, lnSpace);
		DDIST yoff = MusCharYOffset(doc->musFontInfoIndex, accGlyph, lnSpace);
		accXOffset = SizePercentSCALE(AccXOffset(aGRNote->xmoveAcc, &context));
		MoveTo(d2p((xd+xoff)-accXOffset), d2p(yd+yoff));
		DrawChar(accGlyph);
	}

	appearance = aGRNote->headShape;
	if ((appearance==NO_VIS || appearance==NOTHING_VIS) && doc->showInvis)
		appearance = NORMAL_VIS;
	glyph = GetNoteheadInfo(appearance, aGRNote->subType, &headRelSize, &stemShorten);
	if (glyph=='\0')							/* just use qtr notehead for chord slash (irrelevant for GRNote anyway) */
		glyph = MCH_quarterNoteHead;
	glyph = MapMusChar(doc->musFontInfoIndex, glyph);

	/* xhead,yhead is position of notehead; xadjhead,yadjhead is position of notehead 
	   with any offset applied (might be true if music font is not Sonata). */
	   
	offset = MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	if (offset) {
		xhead = d2p(xd);
		xadjhead = d2p(xd+offset);
	}
	else
		xhead = xadjhead = d2p(xd);
	offset = MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
	if (offset) {
		short yp = d2p(yd);
		yhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
		yp = d2p(yd+offset);
		yadjhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
	}
	else
		yhead = yadjhead = d2p(yd+breveFudgeHeadY) + fudgeHeadY;

	if (aGRNote->subType<=WHOLE_L_DUR) {							/* Handle whole/breve/unknown dur */
		MoveTo(xadjhead, yadjhead);									/* position to draw head */
		DrawChar(glyph);
	}
	else {
		MoveTo(xadjhead, yadjhead);									/* position to draw head */

		/* HANDLE STEMMED NOTE. If the grace note is in a chord and is stemless, skip the
		   the entire business of drawing the stem and flags. */
		   
		if (!aGRNote->inChord || GRMainNote(subObjL)) {
			short stemSpace;
			
			dStemLen = aGRNote->ystem-aGRNote->yd;
			stemDown = (dStemLen>0);
			if (stemDown)
				stemSpace = 0;
			else {
				stemSpace = MusFontStemSpaceWidthPixels(doc, doc->musFontInfoIndex, lnSpace);
				Move(stemSpace, 0);
			}
			if (ABS(dStemLen)>pt2d(1)) {
				Move(0, -1);
				Line(0, d2p(dStemLen)+(stemDown? 0 : 1));
			}

			/*	Suppress flags if the note is beamed. */
			
			flagCount = (aGRNote->beamed? 0 : NFLAGS(aGRNote->subType));

			if (flagCount) {												/* Draw any flags */
				ypStem = yhead+d2p(dStemLen);

				if (doc->musicFontNum==sonataFontNum) {
					/* The 8th and 16th note flag characters in Sonata have their origins
					   set so, in theory, they're positioned properly if drawn from the
					   point where the note head was drawn. Unfortunately, the vertical
					   position depends on the stem length, which Adobe incorrectly
					   assumed was always one octave. The "extend flag" characters have
					   their vertical origins set right where they go. */
					   
					MoveTo(xhead, ypStem);								/* Position x at head, y at stem end */
					octaveLength = d2p(7*SizePercentSCALE(dhalfLn));
					if (stemDown)						 				/* Adjust for flag origin */
						Move(0, -octaveLength);
					else
						Move(0, octaveLength);
					if (flagCount==1) {									/* Draw one (8th note) flag */
						if (stemDown)
							DrawChar(MCH_eighthFlagDown);
						else
							DrawChar(MCH_eighthFlagUp);
					}
					else if (flagCount>1) {								/* Draw >=2 (16th & other) flags */
						if (stemDown)
							DrawChar(MCH_16thFlagDown);
						else
							DrawChar(MCH_16thFlagUp);
						MoveTo(xhead, ypStem);							/* Position x at head, y at stem end */
						
						/* Scale Factors for FlagLeading are guesswork. */
						if (stemDown)
							Move(0, -d2p(13*FlagLeading(lnSpace)/4));
						else
							Move(0, d2p(7*FlagLeading(lnSpace)/2));
						while (flagCount-- > 2) {
							if (stemDown) {
								DrawChar(MCH_extendFlagDown);
								Move(0, -d2p(FlagLeading(lnSpace)));	/* Unclear. */
							}
							else {
								DrawChar(MCH_extendFlagUp);
								Move(0, d2p(FlagLeading(lnSpace))); 	/* Unclear. */
							}
						}
					}
				}
				else {	/* fonts other than Sonata */
					short glyphWidth;  DDIST xoff, yoff;

					if (MusFontUpstemFlagsHaveXOffset(doc->musFontInfoIndex))
						stemSpace = 0;

					MoveTo(xhead+stemSpace, ypStem);					/* x, y of stem end */

					if (flagCount==1) {									/* Draw 8th flag. */
						flagGlyph = MapMusChar(doc->musFontInfoIndex, 
															(stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp));
						xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
						yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
						if (xoff || yoff)
							Move(d2p(xoff), d2p(yoff));
						DrawChar(flagGlyph);
					}
					else if (flagCount==2 && MusFontHas16thFlag(doc->musFontInfoIndex)) {	/* Draw 16th flag using one flag char. */
						flagGlyph = MapMusChar(doc->musFontInfoIndex, 
															(stemDown? MCH_16thFlagDown : MCH_16thFlagUp));
						xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
						yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
						if (xoff || yoff)
							Move(d2p(xoff), d2p(yoff));
						DrawChar(flagGlyph);
					}
					else {												/* Draw using multiple flag chars */
						short count = flagCount;

						/* Draw extension flag(s) */
						if (stemDown) {
							flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_extendFlagDown);
							flagLeading = -d2p(DownstemExtFlagLeading(doc->musFontInfoIndex, lnSpace));
						}
						else {
							flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_extendFlagUp);
							flagLeading = d2p(UpstemExtFlagLeading(doc->musFontInfoIndex, lnSpace));
						}
						xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
						yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
						if (yoff || xoff)
							Move(d2p(xoff), d2p(yoff));
						glyphWidth = CharWidth(flagGlyph);
						while (count-- > 1) {
							DrawChar(flagGlyph);
							Move(-glyphWidth, flagLeading);
						}

						/* Draw 8th flag */
						MoveTo(xhead+stemSpace, ypStem);				/* start again from x,y of stem end */
						Move(0, flagLeading*(flagCount-2));				/* flag leadings of all but one of prev flags */
						if (stemDown) {
							flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_eighthFlagDown);
							flagLeading = -d2p(Downstem8thFlagLeading(doc->musFontInfoIndex, lnSpace));
						}
						else {
							flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_eighthFlagUp);
							flagLeading = d2p(Upstem8thFlagLeading(doc->musFontInfoIndex, lnSpace));
						}
						xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
						yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
						if (yoff || xoff)
							Move(d2p(xoff), d2p(yoff));
						Move(0, flagLeading);
						DrawChar(flagGlyph);
					}
				}
			}
		}
		MoveTo(xadjhead, yadjhead);
		DrawChar(glyph);													/* Finally, draw notehead */
	}

	PenPat(NGetQDGlobalsBlack()); 											/* Restore after dimming */
	TextSize(oldtxSize);
	
PopLock(OBJheap);
PopLock(GRNOTEheap);
}

#ifdef DRAG_DYNAMIC_OLD_WAY
// SDDrawDynamic IS NOWHERE NEAR FINISHED! It was part of an attempt to handle dragging
// dynamics the way everything else is, instead of having a bunch of extra code in
// DragDynamic.c -- code that's a bit buggy (and I don't remember why we did it that way
// in the first place), but replacing it with this is looking like a major undertaking.
// Oh well.  --DAB, March 2017
static void SDDrawDynamic(Document *doc, LINK pL, LINK subObjL, LINK measureL)
{
	DDIST		xd, yd;
	CONTEXT		context[MAXSTAVES+1];
	Rect		rSub, mRect;
	
	mRect = SDGetMeasRect(doc, pL, measureL);
	
	GetAllContexts(doc, context, pL);
	GetDynamicDrawInfo(doc, pL, subObjL, context, &glyph, &xd, &yd);
		sizePercent = (DynamicSMALL(subObjL)? SMALLSIZE(100) : 100);
		oldTxSize = GetPortTxSize();
		useTxSize = UseMTextSize(SizePercentSCALE(pContext->fontSize), doc->magnify);

		TextSize(useTxSize);
		glyph = MapMusChar(doc->musFontInfoIndex, glyph);
		xd += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace));
		yd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace));
		xp = d2p(xd); yp = d2p(yd);
		aDynamic = GetPADYNAMIC(aDynamicL);
		MoveTo(pContext->paper.left+xp, pContext->paper.top+yp);
		DrawMChar(doc, glyph, NORMAL_VIS, False);
}
#endif


static void SDDrawRptEnd(Document *doc, LINK pL, LINK /*subObjL*/, LINK measureL)
{
	LINK		aRptL;
	PARPTEND	aRpt;
	DDIST		xd, yd;
	CONTEXT		context[MAXSTAVES+1];
	Rect		rSub, mRect;
	
	mRect = SDGetMeasRect(doc, pL, measureL);
	
	GetAllContexts(doc, context, pL);
	GetRptEndDrawInfo(doc, pL, FirstSubLINK(pL), context, &xd, &yd, &rSub);

	aRptL = FirstSubLINK(pL);
	for ( ; aRptL; aRptL = NextRPTENDL(aRptL)) {
		aRpt = GetPARPTEND(aRptL);
		DrawRptBar(doc, pL, RptEndSTAFF(aRptL), aRpt->connStaff, context, LinkXD(pL),
						(char)RptType(pL), SDDraw, False);
	}
}


static void MoveAndLineTo(short x1, short y1, short x2, short y2)
{
	MoveTo(x1,y1);
	LineTo(x2,y2);
}


static void SDDrawEnding(Document *doc, LINK pL, LINK /*subObjL*/, LINK measureL)
{
	Rect mRect;
	DDIST dTop,xd,yd,endxd,lnSpace,rise,measXD,xdNum,ydNum;
	PENDING p;
	char numStr[MAX_ENDING_STRLEN];
	short xp,yp,endxp, oldFont,oldSize,oldStyle,sysLeft,risePxl,
			endNum, fontSize, strOffset;
	CONTEXT	context[MAXSTAVES+1];
	PCONTEXT pContext;
	LINK firstMeasL,lastMeasL;
	
	mRect = SDGetMeasRect(doc, pL, measureL);

	GetAllContexts(doc, context, pL);

 	p = GetPENDING(pL);
 	endNum = p->endNum;
	pContext = &context[p->staffn];
	if (!pContext->visible) return;

	sysLeft = pContext->systemLeft;
	lnSpace = LNSPACE(pContext);
	rise = ENDING_CUTOFFLEN(lnSpace);

	firstMeasL = LSSearch(p->firstObjL,MEASUREtype,ANYONE,GO_LEFT,False);
	lastMeasL = LSSearch(p->lastObjL,MEASUREtype,ANYONE,GO_LEFT,False);
	measXD = (firstMeasL==lastMeasL ? 0 : LinkXD(lastMeasL)-LinkXD(firstMeasL));

	dTop = pContext->measureTop;

	xd = LinkXD(p->firstObjL) + LinkXD(pL);
	xd = DragXD(xd);
	yd = dTop + p->yd;
	endxd = measXD+LinkXD(p->lastObjL)+p->endxd;
	endxd = DragXD(endxd);
	xdNum = xd+lnSpace;
	ydNum = yd+2*lnSpace;
	
	if (endNum!=0) {
		fontSize = d2pt(2*lnSpace-1);
		strOffset = endNum*MAX_ENDING_STRLEN;
		GoodStrncpy(numStr, &endingString[strOffset], MAX_ENDING_STRLEN-1);
	}

	xp = d2p(xd);
	yp = d2p(yd)-mRect.top;
	endxp = d2p(endxd);
	risePxl = d2p(rise);
	oldFont = GetPortTxFont();
	oldSize = GetPortTxSize();
	oldStyle = GetPortTxFace();
	
	if (p->visible || doc->showInvis) {
		if (!p->noLCutoff)
			MoveAndLineTo(xp,yp+risePxl,xp,yp);
		MoveAndLineTo(xp,yp,endxp,yp);
		if (!p->noRCutoff)
			MoveAndLineTo(endxp,yp,endxp,yp+risePxl);

		if (endNum!=0) {
			TextFont(textFontNum);
			TextSize(UseMTextSize(fontSize, doc->magnify));
			TextFace(0);											/* Plain */
		
			MoveTo(xp+d2p(lnSpace), yp+risePxl+2);
			DrawCString(numStr);
			
			TextFont(oldFont);
			TextSize(oldSize);
			TextFace(oldStyle);
		}
	}
}


static void SDDrawMeasure(Document *doc, LINK pL, LINK subObjL, LINK measureL)
{
	LINK		prevMeasL;
	PAMEASURE	aMeasure;
	CONTEXT		context, context2;
	short		mLeft;
	DDIST		dLeft, dTop, dBottom;
	Rect 		mRect;
	
	mRect = SDGetMeasRect(doc, pL, measureL);

	aMeasure = GetPAMEASURE(subObjL);
	GetContext(doc, pL, aMeasure->staffn, &context);
	dTop = context.staffTop;
	dLeft = context.staffLeft;
	if (aMeasure->connStaff==0)
		dBottom = dTop + context.staffHeight;
	else {
		GetContext(doc, pL, aMeasure->connStaff, &context2);
		dBottom = context2.staffTop + context2.staffHeight;
	}
	prevMeasL = LSSearch(LeftLINK(pL), MEASUREtype, ANYONE, True, False);
	mLeft = d2p(LinkXD(pL) - LinkXD(prevMeasL));

	dTop -= p2d(mRect.top);
	dBottom -= p2d(mRect.top);
	
	MoveTo(DragXP(mLeft), d2p(dTop));
	LineTo(DragXP(mLeft), d2p(dBottom));
}


static void SDDrawPSMeas(Document *doc, LINK pL, LINK subObjL, LINK measureL)
{
	LINK	prevMeasL;
	PAPSMEAS aPSMeas;
	CONTEXT	context, context2;
	short 	mLeft, xp, ypTop, ypBot;
	DDIST	dLeft, dTop, dBottom;
	Rect 	mRect;
	
	mRect = SDGetMeasRect(doc, pL, measureL);

	aPSMeas = GetPAPSMEAS(subObjL);
	GetContext(doc, pL, aPSMeas->staffn, &context);
	dTop = context.staffTop;
	dLeft = context.staffLeft;
	if (aPSMeas->connStaff==0)
		dBottom = dTop + context.staffHeight;
	else {
		GetContext(doc, pL, aPSMeas->connStaff, &context2);
		dBottom = context2.staffTop + context2.staffHeight;
	}
	prevMeasL = LSSearch(LeftLINK(pL), MEASUREtype, ANYONE, True, False);
	mLeft = d2p(LinkXD(pL));

	dTop -= p2d(mRect.top);
	dBottom -= p2d(mRect.top);
	
	xp = DragXP(mLeft);
	ypTop = d2p(dTop);
	ypBot = d2p(dBottom);

	DrawPSMSubType(PSMeasType(subObjL),xp,ypTop,ypBot);
}


static void SDDrawTuplet(Document *doc, LINK pL, LINK measureL)
{
	PTUPLET		tup;
	short		staff;
	CONTEXT 	context;
	DDIST		dTop, xd, yd, acnxd, acnyd, dTuplWidth;
	DPoint		firstPt, lastPt;
	short		xp, yp, xColon;
	Rect		tupleRect, mRect;
	unsigned char tupleStr[20];

	/* Set Font Size correctly for different staff sizes and magnifications. FIXME:
	   Not clear why this call is not needed for other types of objects. */

	SetTextSize(doc);

	staff = TupletSTAFF(pL);
	GetContext(doc, pL, staff, &context);
	GetTupletInfo(doc, pL, &context, &xd, &yd, &xColon, &firstPt, &lastPt, tupleStr,
						&dTuplWidth, &tupleRect);
	dTop = context.measureTop;
	acnxd = xd; acnyd = dTop+yd;
	mRect = SDGetMeasRect(doc, pL, measureL);
	acnyd -= p2d(mRect.top);

	xp = d2p(acnxd-dTuplWidth/2);
	yp = d2p(acnyd);
	MoveTo(xp, yp);
	DrawString(tupleStr);
	tup = GetPTUPLET(pL);
	if (tup->denomVis || doc->showInvis) {
		MoveTo(xp+xColon+1, yp-1);
		DrawMColon(doc, True, False, LNSPACE(&context));
	}
}


static void SDDrawArpSign(Document *doc, DDIST xd, DDIST yd, DDIST dHeight, short subType,
									PCONTEXT pContext)
{
	DDIST lnSpace = LNSPACE(pContext);

	if (subType==NONARP) 
		DrawNonArp(d2p(DragXD(xd)), d2p(yd), dHeight, lnSpace);
	else {
		Byte glyph = MapMusChar(doc->musFontInfoIndex, MCH_arpeggioSign);
		xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
		yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
		DrawArp(doc, d2p(DragXD(xd)), 0, yd, dHeight, glyph, pContext);
	}
}

static void SDDrawGRDraw(Document */*doc*/, DDIST xd, DDIST yd, DDIST xd2, DDIST yd2,
									short lineLW, PCONTEXT pContext)
{
	short xp, yp, yTop, xp2, yp2; DDIST lnSpace, thick;
	
	lnSpace = LNSPACE(pContext);
	
	/* We interpret thickening as horizontal only: not good unless the line is
	   nearly horizontal, or thickness is small enough that it doesn't matter. */
	   
	thick = (long)(lineLW*lnSpace) / 100L;
	yTop = pContext->paper.top;

	PenSize(1, d2p(thick)>1? d2p(thick) : 1);
	xp = d2p(DragXD(xd));
	yp = d2p(yd);
	xp2 = d2p(DragXD(xd2));
	yp2 = d2p(yd2);
	MoveTo(xp, yp);
	LineTo(xp2, yp2);
	PenNormal();	
}

static void SDDrawGraphic(Document *doc, LINK pL, LINK measureL)
{
	DDIST		xd, yd, dHeight, xd2, yd2;
	short		staffn, lineLW;
	short		oldFont, oldSize, oldStyle, fontID, fontSize, fontStyle;
	PGRAPHIC	pGraphic;
	CONTEXT		relContext, context;
	Rect		mRect;
	unsigned char oneChar[2];			/* Pascal string of a char */

	mRect = SDGetMeasRect(doc, pL, measureL);
	
	pGraphic = GetPGRAPHIC(pL);
 	staffn = GetGraphicOrTempoDrawInfo(doc, pL, pGraphic->firstObj, pGraphic->staffn,
										&xd, &yd, &relContext);
	GetGraphicFontInfo(doc, pL, &relContext, &fontID, &fontSize, &fontStyle);
	GetContext(doc, pL, staffn, &context);
	xd -= GetXDDiff(doc, pL, &context);

	pGraphic = GetPGRAPHIC(pL);
	if (pGraphic->firstObj) {
	}

	yd -= p2d(mRect.top);
	switch (pGraphic->graphicType) {
		case GRPICT:
			MayErrMsg("GRPICTs are not implemented.  (SDDragGraphic)");
			break;
		case GRArpeggio:
			dHeight = qd2d(pGraphic->info, context.staffHeight, context.staffLines);

			oldSize = GetPortTxSize();
			TextSize(UseMTextSize(context.fontSize, doc->magnify));
			
			SDDrawArpSign(doc, xd, yd, dHeight, ARPINFO(pGraphic->info2), &context);
			
			TextSize(oldSize);
			break;
		case GRDraw:
			pGraphic = GetPGRAPHIC(pL);
			lineLW = pGraphic->gu.thickness;
			GetGRDrawLastDrawInfo(doc,pL,pGraphic->lastObj,pGraphic->staffn,&xd2,&yd2);
			xd2 -= GetXDDiff(doc,pL,&context);
			yd2 -= p2d(mRect.top);
			SDDrawGRDraw(doc, xd, yd, xd2, yd2, lineLW, &context);
			break;

		case GRSusPedalDown:
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			
			oneChar[0] = 1;
			oneChar[1] = 0xA1;						// Mac OS Roman keys: shift-option 8
			TextFont(doc->musicFontNum);
			TextSize(UseTextSize(fontSize, doc->magnify));
			MoveTo(d2p(DragXD(xd)), d2p(yd));
			
			DrawString(oneChar);
			break;
			
		case GRSusPedalUp:
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			
			oneChar[0] = 1;
			oneChar[1] = '*';						// Shift 8
			TextFont(doc->musicFontNum);
			TextSize(UseTextSize(fontSize, doc->magnify));
			MoveTo(d2p(DragXD(xd)), d2p(yd));
			
			DrawString(oneChar);
			break;
			
		/* Handle the remaining Graphic subtypes, all of which are some kind of text. */
		
		default:
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			
			TextFont(fontID);				
			TextSize(UseTextSize(fontSize, doc->magnify));
			TextFace(fontStyle);
			MoveTo(d2p(DragXD(xd)), d2p(yd));
			if (pGraphic->graphicType==GRChar) {
				if (pGraphic->info==MCH_fermata) DrawChar(pGraphic->info);
				else DrawChar(pGraphic->info);
			}
			else {
				STRINGOFFSET strOffset = GraphicSTRING(FirstSubLINK(pL));

				if (pGraphic->multiLine) {
					short i, j, len, count, lineHt;
					FontInfo fInfo;

					GetNFontInfo(fontID, UseTextSize(fontSize, doc->magnify), fontStyle, &fInfo);
					lineHt = p2d(fInfo.ascent + fInfo.descent + fInfo.leading);
					Byte *q = PCopy(strOffset);
					len = Pstrlen(q);

					/* Divide the Pascal string q[] into a series of substrings by
					   repeatedly stuffing the length we want into the byte preceding
					   the current substring. */
					   
					j = count = 0;
					for (i = 1; i <= len; i++) {
						count++;
						if (q[i] == CH_CR || i == len) {
							q[j] = count;
							DrawString((StringPtr)&q[j]);
							if (i < len) {
								yd += lineHt;
								MoveTo(d2p(DragXD(xd)), d2p(yd));
							}
							j = i;
							count = 0;
						}
					}
				}
				else {
					Boolean expandN = False;
					Str255 textStr;
					pGraphic = GetPGRAPHIC(pL);
					if (pGraphic->graphicType==GRString) expandN = (pGraphic->info2!=0);
					if (expandN) {
						if (!ExpandPString(textStr, (StringPtr)PCopy(strOffset), EXPAND_WIDER))
							LogPrintf(LOG_WARNING, "ExpandPString failed.  (SDDrawGraphic)\n");
					}
					else Pstrcpy(textStr, (StringPtr)PCopy(strOffset));
					DrawString(textStr);
				}
			}
				
			TextFont(oldFont);
			TextSize(oldSize);
			TextFace(oldStyle);
			break;
	}
}


static void SDDrawTempo(Document *doc, LINK pL, LINK measureL)
{
	PTEMPO p;
	short oldFont, oldSize, oldStyle, useTxSize, tempoStrlen;
	DDIST xd, yd, extraGap, lineSpace, dTop, firstxd;
	Str255 tempoStr;
	char metroStr[256], noteChar;
	LINK contextL;
	CONTEXT context; Rect mRect; FontInfo fInfo;
	StringOffset theStrOffset;
	Boolean expandN=False, beforeFirst;

	beforeFirst = LinkBefFirstMeas(pL);
	contextL = (beforeFirst ? LSSearch(pL, MEASUREtype, ANYONE, GO_RIGHT, False) : pL);
	GetContext(doc, contextL, TempoSTAFF(pL), &context);
	lineSpace = LNSPACE(&context);
	dTop = context.measureTop;
	mRect = SDGetMeasRect(doc, pL, measureL);
	
	/* Don't use DragXD(xd) here: already correctly set.
	   If pL is LinkBefFirstMeas(), the dragPorts are system-relative, so include the
	   xd of firstObj even if it's a measure. */

	if (LinkBefFirstMeas(pL))
		firstxd = LinkXD(TempoFIRSTOBJ(pL));
	else
		firstxd = (MeasureTYPE(TempoFIRSTOBJ(pL)) ? 0 : LinkXD(TempoFIRSTOBJ(pL)));
	xd = LinkXD(pL) + firstxd;
	yd = dTop + LinkYD(pL) - p2d(mRect.top);

	p = GetPTEMPO(pL);
	expandN = p->expanded;
	theStrOffset = p->strOffset;

	/* Save the current font and set the font we want. */
	
	oldFont = GetPortTxFont();
	oldSize = GetPortTxSize();
	oldStyle = GetPortTxFace();
	SetFontFromTEXTSTYLE(doc, (TEXTSTYLE *)doc->fontNameTM, lineSpace);
	GetFontInfo(&fInfo);
	
	if (expandN) {
		if (!ExpandPString(tempoStr, (StringPtr)PCopy(theStrOffset), EXPAND_WIDER))
			LogPrintf(LOG_WARNING, "ExpandPString failed.  (SDDrawTempo)\n");
	}
	else Pstrcpy(tempoStr, (StringPtr)PCopy(theStrOffset));

	MoveTo(d2p(xd), d2p(yd));
	DrawString(tempoStr);
	SetRect(&LinkOBJRECT(pL), d2p(xd), d2p(yd)-fInfo.ascent, 
				d2p(xd)+StringWidth(PCopy(theStrOffset)), d2p(yd)+fInfo.descent);

	if (!TempoNOMM(pL) && (!p->hideMM || doc->showInvis)) {
		//DrawChar(' ');
		tempoStrlen = Pstrlen(tempoStr);
		if (tempoStrlen>0) {
			extraGap = qd2d(config.tempoMarkHGap, context.staffHeight, context.staffLines);
//LogPrintf(LOG_DEBUG, "DRAG extraGap=%d d2p(extraGap)=%d\n", extraGap, d2p(extraGap));
			Move(d2p(extraGap), 0);
		}

		useTxSize = UseTextSize(context.fontSize, doc->magnify);
		useTxSize = METROSIZE(useTxSize);
		TextFace(0);                                 /* Plain */
		TextSize(useTxSize);
		TextFont(doc->musicFontNum);

		noteChar = TempoGlyph(pL);
		noteChar = MapMusChar(doc->musFontInfoIndex, noteChar);

		DrawChar(noteChar);
#ifdef NOTYET
		/* FIXME: This chunk of code not only fails to draw the dot, it results in the
		   metronome number not appearing! --DAB */
		if (p->dotted) {
			short noteWidth;
			DDIST xdNote, xdDot, ydDot;
			Byte dotChar = MapMusChar(doc->musFontInfoIndex, MCH_dot);

			xdNote = xd+p2d(StringWidth(tempoStr))+lineSpace;
			noteWidth = CharWidth(noteChar);
			noteWidth += pt2p(2);
			xdDot = xdNote+p2d(noteWidth);
			xdDot += MusCharXOffset(doc->musFontInfoIndex, dotChar, lineSpace);
			ydDot = yd + MusCharYOffset(doc->musFontInfoIndex, dotChar, lineSpace);
//LogPrintf(LOG_DEBUG, "DRAG xdDot, ydDot=%d, %d  pap.left=%d pap.top=%d\n", xdDot, ydDot,
//		context.paper.left, context.paper.top);
			MoveTo(context.paper.left+d2p(xdDot), context.paper.top+d2p(ydDot));
			DrawChar(dotChar);
		}
#endif
		
		SetFontFromTEXTSTYLE(doc, (TEXTSTYLE *)doc->fontNameTM, lineSpace);
		sprintf(metroStr, " = %s", PtoCstr(PCopy(p->metroStrOffset)));
		DrawCString(metroStr);
	}
	TextFont(oldFont);
	TextSize(oldSize);
	TextFace(oldStyle);
}


static void SDDrawSpace(Document */*doc*/, LINK /*pL*/, LINK /*measureL*/)
{
	/* There's nothing to do! */
}


static void SDDrawOttava(Document *doc, LINK pL, LINK measL)
{
	POTTAVA		p;
	short		staff;
	PCONTEXT	pContext;
	CONTEXT		context;
	DDIST		dTop, dLeft, firstxd, firstyd, lastxd, lastyd, lastNoteWidth,
				yCutoffLen, dhalfLn, lnSpace; 
	DDIST		octxdFirst, octxdLast, octydFirst, octydLast;
	DPoint		firstPt, lastPt;
	PANOTE		aNote;
	LINK		aNoteL, firstSyncL, lastSyncL, firstMeas, lastMeas;
	short		octWidth, firstx, firsty, lastx, yCutoff;
	Rect		octRect, mRect;
	unsigned char ottavaStr[20];
	Boolean		bassa;
	long		number;
	short		useTxSize;		/* Insure font is correct size to draw numeral. */
				
PushLock(OBJheap);
	p = GetPOTTAVA(pL);
	staff = p->staffn;
	GetContext(doc, pL, staff, &context);
	pContext = &context;
	dTop = pContext->measureTop;
	dLeft = pContext->measureLeft;
	firstSyncL = FirstInOttava(pL);
	lastSyncL = LastInOttava(pL);
	firstxd = LinkXD(firstSyncL);
	lastxd = LinkXD(lastSyncL);
	
	firstMeas = LSSearch(firstSyncL, MEASUREtype, ANYONE, True, False);
	lastMeas = LSSearch(lastSyncL, MEASUREtype, ANYONE, True, False);
	if (firstMeas!=lastMeas)
		lastxd += LinkXD(lastMeas)-LinkXD(firstMeas);

	lnSpace = LNSPACE(pContext);
	yCutoffLen = OTTAVA_CUTOFFLEN(lnSpace);
	mRect = SDGetMeasRect(doc, pL, measL);
	
	aNoteL = FirstSubLINK(firstSyncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->staffn==staff && MainNote(aNoteL))
			firstyd = aNote->yd;
	}
	aNoteL = FirstSubLINK(lastSyncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->staffn==staff && MainNote(aNoteL))
			lastyd = aNote->yd;
	}

	number = GetOctTypeNum(pL, &bassa);

	/* xdFirst & ydFirst are initialized to zero, and used for re-positioning the
	   ottava when it it dragged (or potentially for Get Info, etc.). */

	p = GetPOTTAVA(pL);

	octxdFirst = firstxd+p->xdFirst;
	octydFirst = dTop+p->ydFirst-p2d(mRect.top);
	lastNoteWidth = SymDWidthRight(doc, lastSyncL, staff, False, *pContext);
	octxdLast = lastxd+lastNoteWidth+p->xdLast;
	octydLast = dTop+p->ydLast-p2d(mRect.top);
	
	SetDPt(&firstPt, octxdFirst, octydFirst);
	SetDPt(&lastPt, octxdLast, octydLast);
	useTxSize = UseTextSize(pContext->fontSize, doc->magnify);
	TextSize(useTxSize);

	NumToSonataStr(number, ottavaStr);
	octRect = StrToObjRect(ottavaStr);
	MoveTo(d2p(DragXD(octxdFirst)), d2p(octydFirst));
	DrawString(ottavaStr);

	firstPt.h = DragXD(firstPt.h);
	lastPt.h = DragXD(lastPt.h);


#define XFUDGE		4			/* in pixels */

	/* Draw bracket unless it's too short. */
	
	dhalfLn = LNSPACE(pContext)/2;
	if (lastPt.h-firstPt.h>4*dhalfLn) {
		if (!bassa) {
			firstPt.v -= 2*dhalfLn;
			lastPt.v -= 2*dhalfLn;
		}
		firstx = d2p(firstPt.h);
		firsty = d2p(firstPt.v);
		lastx = d2p(lastPt.h);
		yCutoff = d2p(yCutoffLen);
		octWidth = octRect.right-octRect.left;
	
		/* Start octWidth to right of firstPt. Draw a dotted line to the end of the
		   ottava, and a solid line up or down yCutoff. */
	
		MoveTo(firstx+octWidth+XFUDGE, firsty);
		QD_DrawDashedLine(firstx+octWidth+XFUDGE, firsty, lastx, firsty);
		if (yCutoff!=0) {
			MoveTo(lastx, firsty);
			LineTo(lastx, firsty+(bassa ? -yCutoff : yCutoff));
		}
	}

PopLock(OBJheap);
}


/* ----------------------------------------------------------------------------- Beams -- */

/* Draw a single beam (not a complete set), for either normal or grace-note beamsets. */

void SDDrawBeam(
		DDIST xl, DDIST yl, DDIST xr, DDIST yr,	/* Absolute left & right end pts. */
		DDIST beamThick,
		short upOrDown,							/* -1 for down, 1 for up */
		PCONTEXT /*pContext*/
		)
{
	short pxlThick,
			yoffset;					/* To compensate for QuickDraw pen hanging below its coords. */
	
	pxlThick = d2p(beamThick);
	PenSize(1, pxlThick);
	yoffset = (upOrDown<0? pxlThick-1 : 0);
	if (yoffset<0) yoffset = 0;
	MoveTo(d2p(xl), d2p(yl)-yoffset);
	LineTo(d2p(xr), d2p(yr)-yoffset);
	PenSize(1, 1);
}

typedef struct {
	LINK		startL;
	DDIST		dLeft;
} BEAMSTACKEL;


/* Draw a grace-note BEAMSET object. */

void SDDrawGRBeamset(Document *doc, LINK pL, LINK measL)
{
	PANOTEBEAM pNoteBeam;
	DDIST yBeam,ydelt,flagYDelta,beamThick,firstBeam,lastBeam, 
			dTop,dLeft,dRight,xStemL,xStemR,firstStfTop,lastStfTop=0;
	short nBeams,upOrDown,staff,voice;
	Boolean crossStaff;
	CONTEXT context,context1; PCONTEXT pContext;
	LINK pNoteBeamL,firstSyncL,lastSyncL,aGRNoteL,firstGRNoteL,lastGRNoteL,
			notesInBeam[MAXINBEAM];
	STFRANGE stfRange; Rect mRect;

	mRect = SDGetMeasRect(doc, pL, measL);
	
	staff = BeamSTAFF(pL);
	voice = BeamVOICE(pL);
	
	GetContext(doc, pL, staff, &context);
	pContext = &context;
	
	crossStaff = BeamCrossSTAFF(pL);
	if (crossStaff) {
		GetBeamGRNotes(pL, notesInBeam);
		GetCrossStaff(LinkNENTRIES(pL), notesInBeam, &stfRange);
		firstStfTop = context.staffTop;
		GetContext(doc,pL,staff+1,&context1);
		lastStfTop = context1.staffTop;
	}

	dTop = pContext->measureTop-p2d(mRect.top);
	dRight = dLeft = 0;										/* Beam cannot cross barline */

	flagYDelta = GRACESIZE(FlagLeading(LNSPACE(pContext)));
	beamThick = LNSPACE(pContext)/2;
	beamThick = GRACESIZE(beamThick);

	/* This is simpler than DrawBEAMSET because grace-note beamsets cannot be cross
	   system or cross barlines, and cannot have secondary or fractional beams. We only
	   have to draw one set of beams all the way from the first GRSync to the last. */

	firstSyncL = FirstInBeam(pL);
	lastSyncL = LastInBeam(pL);
	pNoteBeamL = FirstSubLINK(pL);
	pNoteBeam = GetPANOTEBEAM(pNoteBeamL);
	nBeams = pNoteBeam->startend;

	aGRNoteL = FindGRMainNote(firstSyncL, voice);
	
	upOrDown = ( (GRNoteYSTEM(aGRNoteL) < GRNoteYD(aGRNoteL)) ? 1 : -1 );

	yBeam = GRNoteXStfYStem(aGRNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);
	
	/* FIXME: measLeft PARAM LOOKS WRONG FOR GRBEAMS SPANNING MEASURES. Cf. SDCalcXStem. */
	xStemL = CalcGRXStem(doc, firstSyncL, voice, upOrDown, pContext->measureLeft, pContext, True);
	xStemR = CalcGRXStem(doc, lastSyncL, voice, upOrDown, pContext->measureLeft, pContext, False);
	
	while (nBeams>0) {
		firstGRNoteL = FindGRMainNote(firstSyncL, voice);
		firstBeam = GRNoteXStfYStem(firstGRNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);

		lastGRNoteL = FindGRMainNote(lastSyncL, voice);
		lastBeam = GRNoteXStfYStem(lastGRNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);

		ydelt = flagYDelta*(nBeams-1)*upOrDown;

		SDDrawBeam(dLeft+xStemL+DragXD(0),
				   dTop+firstBeam+ydelt,
				   dRight+xStemR+DragXD(0),
				   dTop+lastBeam+ydelt, beamThick, upOrDown, pContext);

		nBeams--;
	}
}


/* Given a Sync, voice number, stem direction, and standard notehead width, return the
measure-relative x-coordinate of the given end of a beam for the note/chord in that Sync
and voice. The parameter measLeft is being retained in case it turns out to be needed for
some coordinate system adjustment. */

#define STEMWIDTH (p2d(1))			/* Stem thickness. */

DDIST SDCalcXStem(LINK syncL, short voice, short stemDir,
					DDIST measLeft, DDIST headWidth,
					Boolean left			/* True=left end of beam, else right */
					)
{
	DDIST	xd, xdNorm;
	LINK	aNoteL;
	
	measLeft = 0;
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (MainNoteInVOICE(aNoteL,voice)) {
			xd = NoteXLoc(syncL, aNoteL, measLeft, headWidth, &xdNorm);
			break;
	}
	
	if (stemDir>0)	xd += headWidth-(left?  STEMWIDTH : p2d(1));
	else			xd += (left? 0 : STEMWIDTH-p2d(1));
		
	return xd;
}


/* Draw a normal (non-grace-note) BEAMSET object. */

static void SDDrawBeamset(Document *doc, LINK pL, LINK measL)
{
	PBEAMSET	p;
	PANOTEBEAM	pNoteBeam;
	BEAMSTACKEL	beamStack[MAX_L_DUR],*beamEl;			/* Really only needs to be [max. no. of beams] */
	DDIST		yBeam, ydelt, frac_xend,
				flagYDelta, beamThick, firstBeam, lastBeam, 
				dTop, dLeft, beamLeft,
				dRight, hDiff, vDiff,
				firstystem, lastystem, xStem,
				xStemL, xStemR,
				firstStfTop, lastStfTop;				/* For cross staff beams */
	short		nb_startend,							/* number of beams starting or ending at a Sync */
				nb_frac, nestLevel, upOrDown, iel,
				stemDirL, stemDirR,						/* stem directions: +or-1 */
				staff, voice, j;
	Boolean		haveSlope,crossStaff,
				crossSys, firstSys;
	LINK		noteBeamL, syncL, firstSyncL, lastSyncL, aNoteL, bNoteL,
				firstNoteL, lastNoteL,notesInBeam[MAXINBEAM], staffL, aStaffL;
	PCONTEXT	pContext;
	CONTEXT		tempContext,context,context1;
	FASTFLOAT	slope;
	STFRANGE	stfRange;
	Rect		mRect;

	beamEl = beamStack;
	for (j=0;j<MAX_L_DUR;j++)
		beamEl->startL = beamEl->dLeft = 0;

	mRect = SDGetMeasRect(doc, pL, measL);
	
	staff = BeamSTAFF(pL);
	voice = BeamVOICE(pL);
	
	/* Fill notesInBeam[], an array of the notes in the beamVoice in the bpSyncs, in
	   order to call GetCrossStaff to get the stfRange. */

	p = GetPBEAMSET(pL);
	crossSys = p->crossSystem;
	firstSys = p->firstSystem;

	crossStaff = p->crossStaff;
	if (crossStaff) {
		GetBeamNotes(pL, notesInBeam);
		GetCrossStaff(LinkNENTRIES(pL), notesInBeam, &stfRange);
	}
	staffL = LSSearch(pL,STAFFtype,ANYONE,GO_LEFT,False);
	if (staffL) {
		aStaffL = FirstSubLINK(staffL);
		for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
			if (StaffSTAFF(aStaffL)==staff) {
				Context1Staff(aStaffL, &context);
				break;
			}
	}
	GetContext(doc, pL, staff, &context);
	pContext = &context;
	
	dTop = pContext->measureTop-p2d(mRect.top);
	dLeft = 0;
	if (crossStaff) {
		firstStfTop = context.staffTop;
		GetContext(doc,pL,staff+1,&context1);
		lastStfTop = context1.staffTop;
	}
	flagYDelta = FlagLeading(LNSPACE(pContext));
	beamThick = pContext->staffHeight/(2*(pContext->staffLines-1));
	if (beamThick<p2d(2)) beamThick = p2d(2);
	/*
	 * The main part of this routine is an interpreter for three commands
	 * that may be stored in a NOTEBEAM, namely "start non-fractional beam",
	 * "do fractional beam", and "end non-fractional beam". They are processed
	 * in that order. Fractional beams require no additional information, so
	 * they are drawn as soon as they are encountered. Non-fractional beams,
	 * i.e., primary and secondary beams, can't be drawn until their ends are
	 * encountered, so the necessary information from their start NOTEBEAMs is
	 * kept in the stack <beamStack>.
	 */
	nestLevel = -1;

	noteBeamL = FirstSubLINK(pL);
	for (iel = 0; iel<LinkNENTRIES(pL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		pNoteBeam = GetPANOTEBEAM(noteBeamL);
		syncL = pNoteBeam->bpSync;
		haveSlope = False;	
		aNoteL = FindMainNote(syncL, voice);
		upOrDown = ( (NoteYSTEM(aNoteL) < NoteYD(aNoteL)) ? 1 : -1 );

		if ((nb_startend = pNoteBeam->startend) > 0)			/* Any beams starting here? */
			while (nb_startend-->0) {							/* Save ptr in stack */
				beamStack[++nestLevel].startL = syncL;
				GetContext(doc, syncL, staff, &tempContext);	/* Get left end context */
				beamStack[nestLevel].dLeft=tempContext.measureLeft;
			}

		if ((nb_frac = pNoteBeam->fracs)>0)	{					/* Handle fractional beams. */
			if (!haveSlope) {
				firstSyncL = FirstInBeam(pL);
				lastSyncL = LastInBeam(pL);
				bNoteL = FirstSubLINK(firstSyncL);
				for ( ; bNoteL; bNoteL = NextNOTEL(bNoteL)) {
					if (NoteVOICE(bNoteL)==voice && MainNote(bNoteL))
						firstystem = NoteXStfYStem(bNoteL,stfRange,
													firstStfTop,lastStfTop,crossStaff);
				}
				bNoteL = FirstSubLINK(lastSyncL);
				for ( ; bNoteL; bNoteL = NextNOTEL(bNoteL)) {
					if (NoteVOICE(bNoteL)==voice && MainNote(bNoteL))
						lastystem = NoteXStfYStem(bNoteL,stfRange,
													firstStfTop,lastStfTop,crossStaff);
				}
				hDiff = SysRelxd(lastSyncL)-SysRelxd(firstSyncL);
				slope = (FASTFLOAT)(lastystem-firstystem)/hDiff;
				vDiff = FracBeamWidth(LNSPACE(pContext))*slope;
				haveSlope = True;
			}
			yBeam = NoteXStfYStem(aNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);
			pNoteBeam = GetPANOTEBEAM(noteBeamL);
			xStem = SDCalcXStem(syncL, voice, upOrDown, pContext->measureLeft,
									HeadWidth(LNSPACE(pContext)), pNoteBeam->fracGoLeft);
			frac_xend = (pNoteBeam->fracGoLeft) ?
						  		xStem-FracBeamWidth(LNSPACE(pContext)) :
								xStem+FracBeamWidth(LNSPACE(pContext));
			GetContext(doc, syncL, staff, &tempContext);		/* Get right end context, just */
			dRight = tempContext.measureLeft;					/*   in case beam crosses barline */
			beamLeft = beamStack[nestLevel].dLeft;
			dRight -= beamLeft;
			beamLeft = 0;
			while (nb_frac>0) {
				ydelt = flagYDelta*(nestLevel+nb_frac)*upOrDown;

				SDDrawBeam(dRight+xStem+DragXD(0),
						  dTop+yBeam+ydelt,
						  dRight+frac_xend+DragXD(0),
						  dTop+yBeam+ydelt+(pNoteBeam->fracGoLeft ? -vDiff : vDiff),
						  beamThick, upOrDown, pContext);

				nb_frac--;
			}
		}		

		if ((nb_startend = pNoteBeam->startend) < 0)	{		/* Draw any beams ending */
			yBeam = NoteXStfYStem(aNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);
			GetContext(doc, syncL, staff, &tempContext);		/* Get right end context, just */
			dRight = tempContext.measureLeft;					/*   in case beam crosses barline */
						
			while (nb_startend++ < 0) {
				firstSyncL = beamStack[nestLevel].startL;
				firstNoteL = FirstSubLINK(firstSyncL);
				for ( ; firstNoteL; firstNoteL = NextNOTEL(firstNoteL))
					if (NoteVOICE(firstNoteL)==voice && MainNote(firstNoteL))
						break;

				firstBeam = NoteXStfYStem(firstNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);

				lastNoteL = FirstSubLINK(syncL);
				for ( ; lastNoteL; lastNoteL = NextNOTEL(lastNoteL))
					if (NoteVOICE(lastNoteL)==voice && MainNote(lastNoteL))
						break;

				lastBeam = NoteXStfYStem(lastNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);

				ydelt = flagYDelta*nestLevel*upOrDown;
				stemDirL = ( (NoteYSTEM(firstNoteL) < NoteYD(firstNoteL)) ? 1 : -1 );
				stemDirR = ( (NoteYSTEM(lastNoteL) < NoteYD(lastNoteL)) ? 1 : -1 );
				
				xStemL = SDCalcXStem(beamStack[nestLevel].startL, voice, stemDirL,
											pContext->measureLeft, HeadWidth(LNSPACE(pContext)), True);
				xStemR = SDCalcXStem(syncL, voice, stemDirR, pContext->measureLeft,
											HeadWidth(LNSPACE(pContext)), False);

				beamLeft = beamStack[nestLevel].dLeft;

				if (crossSys) {
					if (firstSys)
							dRight += p2d(5);
					else	beamLeft -= p2d(5);
				}
				else {
					dRight = tempContext.measureLeft;
					dRight -= beamLeft; beamLeft = 0;
				}

				SDDrawBeam(beamLeft+xStemL+DragXD(0),
						   dTop+firstBeam+ydelt,
						   dRight+xStemR+DragXD(0),
						   dTop+lastBeam+ydelt, beamThick, upOrDown, pContext);

				nestLevel--;
			}
		}
	}
}


/* -------------------------------------------------------------------------------------- */

static void SDDrawSlur(Document *doc, LINK pL, LINK measL)
{
	PSLUR p;  PASLUR aSlur;  LINK aSlurL;
	Rect paper;  CONTEXT localContext;
	short j, penThick;								/* vertical pen size in pixels */
	DDIST xdFirst, ydFirst, xdLast, ydLast,			/* DDIST positions of end notes */
			slurDiff;								/* fudges for slurs */
	Point startPt[MAXCHORD], endPt[MAXCHORD];
	DPoint start,end,c0,c1;	

	PushLock(OBJheap);
	PushLock(SLURheap);

	p = GetPSLUR(pL);
	GetContext(doc, p->firstSyncL, SlurSTAFF(pL), &localContext);

	GetSlurContext(doc, pL, startPt, endPt);				/* Get absolute positions, in DPoints */
	
	paper = SDGetMeasRect(doc,pL,measL);
	paper.left = -paper.left;
	paper.top = -paper.top;
	
	aSlurL = FirstSubLINK(pL);
	for (j=0; aSlurL; j++, aSlurL = NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
		if (aSlur->selected) {
			aSlur->startPt = startPt[j];
			aSlur->endPt = endPt[j];
			xdFirst = p2d(aSlur->startPt.h); xdLast = p2d(aSlur->endPt.h);
			ydFirst = p2d(aSlur->startPt.v); ydLast = p2d(aSlur->endPt.v);
		
			xdFirst += aSlur->seg.knot.h;					/* abs. position of slur start */
			ydFirst += aSlur->seg.knot.v;
			xdLast += aSlur->endKnot.h;						/* abs. position of slur end */
			ydLast += aSlur->endKnot.v;
			
			penThick = d2p(LNSPACE(&localContext)/5);
			if (penThick<1) penThick = 1;
			if (penThick>2) penThick = 2;
			PenSize(1, penThick);
			slurDiff = xdLast-xdFirst;
	
			GetSlurPoints(aSlurL,&start,&c0,&c1,&end);
			StartSlurBounds();
			DrawSlursor(&paper, &start, &end, &c0, &c1, S_Whole, aSlur->dashed);
			EndSlurBounds(&paper,&aSlur->bounds);
	
			PenNormal();
		}
	}

	PopLock(OBJheap);
	PopLock(SLURheap);
}


/* ====================================================================================== */

/* --------------------------------------------------------------------- HandleSymDrag -- */
/* Should be called by all CheckXXX routines to handle normal dragging. See comments on
DoSymbolDrag for a description of the torturous route we take to get here. */

Boolean HandleSymDrag(Document *doc, LINK pL, LINK subObjL, Point pt, unsigned char glyph)
{
	LINK measL;
	
	measL = LSSearch(pL, MEASUREtype, 1, True, False);
	return SymDragLoop(doc, pL, subObjL, glyph, pt, measL);
}


/* ----------------------------------------------------------------------- SymDragLoop -- */
/* The routine that is actually responsible for dragging an object. First calls the
SDDraw routine for the object's type to draw the to-be-dragged object into the offscreen
bitmap; then enters the loop where the user drags the object; and finally updates the
object's position and other attributes with the SetFields routine for the type of the
dragged object.

<glyph> is the char to be drawn only for Clefs and Dynamics FIXME: for no good reason--
should eliminate this param!. The measure containing the object is used to correct the
coordinate system if the measure is partially off the screen.

Returns True if the symbol was actually dragged along either axis, else False. */

static Boolean SymDragLoop(
				Document *doc,
				LINK pL, LINK subObjL,			/* object/subobject to drag */
				unsigned char glyph,
				Point pt,						/* initial mousedown pt (paper-relative) */
				LINK measureL					/* containing the object */
				)
{
	PANOTE		aNote;
	PAGRNOTE	aGRNote;
	LINK		firstMeasL, octL, partL, staffL;
	GrafPtr		oldPort, oldPort1;
	GrafPtr		accPort=NULL;
	Rect		theRect, theClipRect, mRect, bounds, accBox, saveBox;
	Point		oldPt;
	short		xp,yp,staff,halfLn,oldHalfLn,halfLnDiff,
				accy,noteLeft,newAcc,partn,useChan,octType,octTransp,
				transp,prevAccident,middleCHalfLn,noteNum;
	short		useIORefNum=0;			/* NB: both OMSUniqueID and fmsUniqueID are unsigned short */
	PPARTINFO	pPart;
	DDIST		xdDiff,ydDiff,v,leftLim,rightLim,dxTotal;
	CONTEXT		context;
	Boolean 	horiz, vert, stillWithinSlop, vQuantize, isClef, isRest;
	long		spaceFactor;			/* Amount by which to respace dragged measures */
	short 		dx, dy;
	Point 		newPt;
	Rect 		dstRect;
	WindowPtr	w=doc->theWindow;
	
	const BitMap *wPortBits = NULL;
	const BitMap *accPortBits = NULL;
	GWorldPtr gwPtr = NULL;
	
	isClef = isRest = False;
	GetPort(&oldPort);
	SetPort(picBits);
	SetRect(&theClipRect, -32000, -32000, 32000, 32000);
	ClipRect(&theClipRect);
	GetPortBounds(underBits,&theRect);
	TextFont(doc->musicFontNum);
	staffL = LSSearch(pL, STAFFtype, ANYONE, True, False);
	spaceFactor = 0L;												/* Normally unused */
	
	switch (ObjLType(pL)) {
		case CLEFtype:
			isClef = True;
			staff = ClefSTAFF(subObjL);
			SDDrawClef(doc, pL, subObjL, glyph, measureL);
			GetContext(doc, pL, staff, &context);
			GetHDragLims(doc,pL,subObjL,staff,context,&leftLim,&rightLim);
			break;
			
		case KEYSIGtype:
			SDDrawKeySig(doc, pL, subObjL, measureL);
			staff = KeySigSTAFF(subObjL);
			GetContext(doc, pL, staff, &context);
			GetHDragLims(doc,pL,subObjL,staff,context,&leftLim,&rightLim);
			break;
			
		case TIMESIGtype:
			SDDrawTimeSig(doc, pL, subObjL, measureL);
			staff = TimeSigSTAFF(subObjL);
			GetContext(doc, pL, staff, &context);
			GetHDragLims(doc,pL,subObjL,staff,context,&leftLim,&rightLim);
			break;
			
		case SYNCtype:
			SDDrawNote(doc, pL, subObjL, measureL);
			staff = NoteSTAFF(subObjL);
			GetContext(doc, pL, staff, &context);
			if (NoteREST(subObjL)) isRest = True;
			if (!isRest) {

				/* Set up the accidental box for vertical note dragging. */
				
				noteLeft = d2p(PageRelxd(pL, &context) + NoteXD(subObjL));
				newAcc = 0;
				accy = d2p(context.staffTop+context.staffHeight/2);
				accBox.left = noteLeft-2-ACCBOXWIDE;		
				accBox.right = noteLeft-2;
				accBox.bottom = accy+ACCBOXWIDE/2+1;
				accBox.top = accy-3*ACCBOXWIDE/4-1;					/* Allow for flat ascent */
				OffsetRect(&accBox,doc->currentPaper.left,doc->currentPaper.top);
				accy += doc->currentPaper.top;
				SetRect(&saveBox, 0, 0, accBox.right-accBox.left,
						accBox.bottom-accBox.top);
				SaveGWorld();
				
				gwPtr = MakeGWorld(saveBox.right+1, saveBox.bottom+1, True);
				if (!gwPtr) goto broken;
				accPort = gwPtr;

				wPortBits = GetPortBitMapForCopyBits(GetWindowPort(w));
				accPortBits = GetPortBitMapForCopyBits(accPort);
				CopyBits(wPortBits, accPortBits, &accBox, &saveBox, srcCopy, NULL);
				
				RestoreGWorld();

//				CopyBits(&w->portBits, &(accPort->portBits),
//						 &accBox, &saveBox, srcCopy, NULL);
	
				/* Set up for MIDI feedback. */
				
				partn = Staff2Part(doc, staff);
				useChan = UseMIDIChannel(doc, partn);					/* Set feedback channel no. */

				octL = OctOnStaff(doc->selStartL, staff);
				if (octL) {
					octType = OttavaType(octL);
					octTransp = (octType>0? noteOffset[octType-1] : 0);	/* Set feedback transposition */
				}
				else {
					octType = -1;
					octTransp = 0;
				}
				partL = FindPartInfo(doc, partn);
				pPart = GetPPARTINFO(partL);
				if (doc->transposed) transp = pPart->transpose;
				else						transp = 0;
	
				prevAccident = 0;
				middleCHalfLn = ClefMiddleCHalfLn(context.clefType);
				aNote = GetPANOTE(subObjL);
				halfLn = qd2halfLn(aNote->yqpit);
				if (ACC_IN_CONTEXT) {
					GetPitchTable(doc, accTable, pL, staff);					/* Get pitch mod. situation */
					prevAccident = accTable[halfLn-middleCHalfLn+ACCTABLE_OFF];	/* Effective acc. here */			
				}
			}

			/* Get left and right dragging limits for horizontal drag of note or rest. */
			
			GetHDragLims(doc,pL,subObjL,staff,context,&leftLim,&rightLim);
			break;

		case GRSYNCtype:
			SDDrawGRNote(doc, pL, subObjL, measureL);
			staff = GRNoteSTAFF(subObjL);
			GetContext(doc, pL, staff, &context);

			/* Set up the accidental box for vertical note dragging. */
			
			noteLeft = d2p(PageRelxd(pL, &context) + GRNoteXD(subObjL));
			newAcc = 0;
			accy = d2p(context.staffTop+context.staffHeight/2);
			accBox.left = noteLeft-2-ACCBOXWIDE;		
			accBox.right = noteLeft-2;
			accBox.bottom = accy+ACCBOXWIDE/2+1;
			accBox.top = accy-3*ACCBOXWIDE/4-1;							/* Allow for flat ascent */
			OffsetRect(&accBox,doc->currentPaper.left,doc->currentPaper.top);
			accy += doc->currentPaper.top;
			SetRect(&saveBox, 0, 0, accBox.right-accBox.left,
					accBox.bottom-accBox.top);
					
			SaveGWorld();
			
			gwPtr = MakeGWorld(saveBox.right+1, saveBox.bottom+1, True);
			if (!gwPtr) goto broken;
			accPort = gwPtr;

			wPortBits = GetPortBitMapForCopyBits(GetWindowPort(w));
			accPortBits = GetPortBitMapForCopyBits(accPort);
			CopyBits(wPortBits, accPortBits, &accBox, &saveBox, srcCopy, NULL);
			
			RestoreGWorld();

//			CopyBits(&w->portBits, &(accPort->portBits),
//					 &accBox, &saveBox, srcCopy, NULL);

			/* Set up for MIDI feedback. */
			partn = Staff2Part(doc, staff);
			useChan = UseMIDIChannel(doc, partn);					/* Set feedback channel no. */

			octL = OctOnStaff(doc->selStartL, staff);
			if (octL) {
				octType = OttavaType(octL);
				octTransp = (octType>0? noteOffset[octType-1] : 0);	/* Set feedback transposition */
			}
			else {
				octType = -1;
				octTransp = 0;
			}
			partL = FindPartInfo(doc, partn);
			pPart = GetPPARTINFO(partL);
			if (doc->transposed)	transp = pPart->transpose;
			else					transp = 0;

			prevAccident = 0;
			middleCHalfLn = ClefMiddleCHalfLn(context.clefType);
			aGRNote = GetPAGRNOTE(subObjL);
			halfLn = qd2halfLn(aGRNote->yqpit);
			if (ACC_IN_CONTEXT) {
				GetPitchTable(doc, accTable, pL, staff);					/* Get pitch mod. situation */
				prevAccident = accTable[halfLn-middleCHalfLn+ACCTABLE_OFF];	/* Effective acc. here */			
			}

			GetHDragLims(doc,pL,subObjL,staff,context,&leftLim,&rightLim);
			break;
			
		case DYNAMtype:
#ifdef DRAG_DYNAMIC_OLD_WAY
			SDDrawDynamic(doc, pL, subObjL, glyph, measureL);
#endif
			break;															/* Should never get here */
			
		case RPTENDtype:
			SDDrawRptEnd(doc, pL, subObjL, measureL);
			break;

		case ENDINGtype:
			SDDrawEnding(doc, pL, subObjL, measureL);
			break;

		case MEASUREtype:
			SDDrawMeasure(doc, pL, subObjL, measureL);
			break;

		case PSMEAStype:
			SDDrawPSMeas(doc, pL, subObjL, measureL);
			break;

		case TUPLETtype:
			SDDrawTuplet(doc, pL, measureL);
			break;

		case GRAPHICtype:
			SDDrawGraphic(doc, pL, measureL);
			break;
			
		case TEMPOtype:
			if (MeasureTYPE(TempoFIRSTOBJ(pL))) {
				measureL = TempoFIRSTOBJ(pL);
			}
			SDDrawTempo(doc, pL, measureL);
			break;

		case SPACERtype:
			SDDrawSpace(doc, pL, measureL);
			break;

		case OTTAVAtype:
			SDDrawOttava(doc, pL, measureL);
			break;

		case BEAMSETtype:
			if (BeamGRACE(pL))	SDDrawGRBeamset(doc, pL, measureL);
			else				SDDrawBeamset(doc, pL, measureL);
			break;

		case SLURtype:
			SDDrawSlur(doc, pL, measureL);
			break;
			
		default:
			MayErrMsg("SymDragLoop: type %d not supported", ObjLType(pL));
			return False;
	}
	
	SetPort(oldPort);
	
	dstRect = theRect;
	mRect = SDGetMeasRect(doc, pL, measureL);
	bounds = mRect; InsetRect(&bounds, 2, 2);
	OffsetRect(&mRect,doc->currentPaper.left,doc->currentPaper.top);
	firstMeasL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);
	
	oldPt = newPt = pt;
	stillWithinSlop = True;
	horiz = vert = True;
	vQuantize = False;
	
	if (StillDown())
		while (WaitMouseUp()) {
			GetPaperMouse(&newPt, &doc->currentPaper);
			if (!EqualPt(pt,newPt)) {
			
				/* If we've gone outside the measure boundary, do nothing. */
				
				if (horiz && (newPt.h<bounds.left || newPt.h>bounds.right)) continue;
				if (!horiz && (newPt.v<bounds.top || newPt.v>bounds.bottom)) continue;

				/* Get how far cursor has moved since last time, if non-zero */
				
				dx = newPt.h - pt.h; dy = newPt.v - pt.v;
				
				/* If we're still within slop bounds, don't do anything */
				
				if (stillWithinSlop) {
					if (ABS(dx)<2 && ABS(dy)<2) continue;
					
					/* User has left slop bounds; find horizontal/vertical for notes & clefs */
					
					switch (ObjLType(pL)) {
						case SYNCtype:
						case GRSYNCtype:
							horiz = ABS(dx) > ABS(dy);				/* Interpret 45-deg. mvmt as vertical */
							vQuantize = vert = !horiz;
							oldHalfLn = CalcPitchLevel(pt.v, &context, staffL, staff);
							if ((!NoteREST(subObjL) || GRSyncTYPE(pL)) && vert)
								goto noteDragDone;						
							break;
						case CLEFtype:
							switch (ClefType(subObjL)) {
								case SOPRANO_CLEF:
								case MZSOPRANO_CLEF:
								case ALTO_CLEF:
								case TENOR_CLEF:
								case BARITONE_CLEF:
									horiz = ABS(dx) > ABS(dy);		/* Interpret 45-deg. mvmt as vertical */
									vQuantize = vert = !horiz;
									oldHalfLn = CalcPitchLevel(pt.v, &context, staffL, staff);
									break;
								default:
									horiz = True;
									vert = !horiz;
									break;	
								
							}
							break;
						case KEYSIGtype:
						case TIMESIGtype:
						case MEASUREtype:
						case PSMEAStype:
						case RPTENDtype:
							horiz = True;
							vert = !horiz;
							break;
						case BEAMSETtype:
							vert = True;
							horiz = !vert;
							break;
						default:
							if (ShiftKeyDown()) {
								horiz = ABS(dx) > ABS(dy);			/* Interpret 45-deg. mvmt as vertical */
								vert = !horiz;
							}
					}
					/* And don't ever come back, you hear! */
					
					stillWithinSlop = False;
				}
				
				if (vQuantize) {
					halfLn = CalcPitchLevel(newPt.v, &context, staffL, staff);
					if (halfLn == oldHalfLn) continue;
					halfLnDiff = halfLn - oldHalfLn;
					if (odd(halfLnDiff) && (isClef || isRest)) continue;
					v = halfLn2d(halfLnDiff, context.staffHeight, context.staffLines);

					/* In magnified sizes, dstRect position picks up a small cumulative
					   error each time thru the loop, resulting in the bitmap being
					   drawn a little too low. Use d2px, which doesn't add ROUND to the
					   computation of dy. */
					   
					if (doc->magnify!=0)	dy = d2px(v);
					else					dy = d2p(v);

					if (!isClef) {
						GetPort(&oldPort1);
						SetPort(underBits);
						SDInsertLedgers(pL, subObjL, halfLn, &context);
						SetPort(oldPort1);
					}
					oldHalfLn = halfLn;
				}
				
				/* Vertical dragging of notes and grace notes involves accidentals
				   and MIDI feedback. */
				   
				if (((SyncTYPE(pL) && !NoteREST(subObjL)) || GRSyncTYPE(pL)) && vert) {
					if (ShiftKeyDown()) {
						newAcc = SDSetAccidental(doc, accPort, accBox, saveBox, accy);

						/* If the mouse isn't still down, user has exited the dragging
						   loop with the shift key down; the last part of the action
						   has been to change the accidental. Set the final mouse position
						   to what it was before the shift key was depressed, and exit
						   the loop. Also, need to recalculate halfLn, which is picked
						   up higher in the loop from newPt.v. Otherwise, the user is
						   continuing to drag the note vertically; set newAcc to 0 so as
						   not to pick up the accidental from SDSetAccidental, which is
						   no longer wanted. */

						if (!StillDown()) {
							newPt = pt;
							halfLn = CalcPitchLevel(newPt.v, &context, staffL, staff);
							goto setAccDone;
						}
						else
							newAcc = 0;
					}

					SDMIDIFeedback(doc, &noteNum, useChan, newAcc, transp, middleCHalfLn,
										halfLn, octTransp, useIORefNum);
				}

				/* Set left and right limits for dragging. Currently: use the xd of the
				   immediately preceding and following objects with valid xd, ignoring
				   subObj offsets of either the objects in the sync being dragged or the
				   right and left objects. Limit the dragging so that the note/rest cannot
				   be dragged past the limit object. */

				if (horiz) {
					if (SyncTYPE(pL) || ClefTYPE(pL) ||
					  KeySigTYPE(pL) || TimeSigTYPE(pL) || GRSyncTYPE(pL)) {
						if (newPt.h>oldPt.h)	{				/* Dragging to right */
							if (rightLim>=0) {					/* Next object to right not the tail */
								dxTotal = newPt.h-oldPt.h;
								if (dxTotal>d2p(rightLim)) {
									dx -= (dxTotal-d2p(rightLim));
									dxTotal = d2p(rightLim);
									newPt.h = oldPt.h+dxTotal;
									
								}
							}
						}
						else {									/* Dragging to left */
							dxTotal = oldPt.h-newPt.h;
							if (dxTotal>d2p(leftLim)) {
								dx += (dxTotal-d2p(leftLim));
								dxTotal = d2p(leftLim);
								newPt.h = oldPt.h-dxTotal;
							}
						}
					}
				}

				OffsetRect(&dstRect,(horiz ? dx : 0), (vert ? dy : 0));
					
				const BitMap *underPortBits = GetPortBitMapForCopyBits(underBits);
				const BitMap *offScrPortBits = GetPortBitMapForCopyBits(offScrBits);
				const BitMap *picPortBits = GetPortBitMapForCopyBits(picBits);
				wPortBits = GetPortBitMapForCopyBits(GetWindowPort(w));
				
				RgnHandle wVisRgn = NewRgn();
				GetPortVisibleRegion(GetWindowPort(w), wVisRgn);
				
				CopyBits(underPortBits, offScrPortBits, &theRect, &theRect, srcCopy, NULL);				
				CopyBits(picPortBits, offScrPortBits, &theRect, &dstRect, srcOr, NULL);
				CopyBits(offScrPortBits, wPortBits, &theRect, &mRect, srcCopy, wVisRgn);
				
				DisposeRgn(wVisRgn);

//				CopyBits(&underBits->portBits, &offScrBits->portBits,
//						 &theRect, &theRect, srcCopy, NULL);
//				CopyBits(&picBits->portBits, &offScrBits->portBits,
//						 &theRect, &dstRect, srcOr, NULL);
//				CopyBits(&offScrBits->portBits, &w->portBits,
//						 &theRect, &mRect, srcCopy, w->visRgn);
				
				pt = newPt;
			}
		}															/* end of the "WaitMouseUp" loop */

	/* Dragging is finished. If user dragged outside of the measure, use the most
	   recent point inside it. */

	if (!PtInRect(newPt, &bounds)) newPt = pt;
	
	/* Go to here if user let mouse up while tracking accidental, so as not to pick up
	   the newPt; under these circumstances, that's where the user dragged the mouse to
	   set the accidental, not to drag the note vertically. */
	   
setAccDone:

	if (vQuantize) {

		/* If we're quantizing, we want to use the first and last quantized positions
		   rather than the first last cursor points. */

		oldHalfLn = CalcPitchLevel(oldPt.v, &context, staffL, staff);
		oldPt.v = context.staffTop +
							halfLn2d(oldHalfLn,context.staffHeight,context.staffLines);
		oldPt.v = d2p(oldPt.v);
		newPt.v = context.staffTop +
							halfLn2d(halfLn,context.staffHeight,context.staffLines);
		newPt.v = d2p(newPt.v);
	}
	
	xdDiff = (horiz? p2d(xp=newPt.h-oldPt.h) : 0);
	ydDiff = (vert? p2d(yp=newPt.v-oldPt.v) : 0);

	if (xdDiff==0 && ydDiff==0) {
		SDCleanUp(doc, oldPort, 100L, pL, False, 0, 0);
		return False;
	}

	/* Symbol was really moved on one or both axes. Update the object list. */
	
	switch (ObjLType(pL)) {
		case CLEFtype:
			SetClefFields(doc, pL, subObjL, xdDiff, ydDiff, xp, yp, vert);
			break;

		case KEYSIGtype:
			SetKeySigFields(pL, subObjL, xdDiff, xp);
			break;

		case TIMESIGtype:
			SetTimeSigFields(pL, subObjL, xdDiff, xp);
			break;

		case SYNCtype:
			if (doc->noteInsFeedback && !NoteREST(subObjL) && vert) {

			}
			SetNoteFields(doc, pL, subObjL, xdDiff, ydDiff, xp, yp, vert, False, newAcc);
			break;

		case GRSYNCtype:
			if (doc->noteInsFeedback && vert) {
			}
			SetGRNoteFields(doc, pL, subObjL, xdDiff, ydDiff, xp, yp, vert, False, newAcc);
			break;

		case DYNAMtype:
			SetDynamicFields(pL, subObjL, xdDiff, ydDiff, xp, yp);
			break;

		case RPTENDtype:
			SetRptEndFields(pL, subObjL, xdDiff, xp);
			break;

		case ENDINGtype:
			SetEndingFields(doc, pL, subObjL, xdDiff, ydDiff, xp, yp);
			break;

		case MEASUREtype:
			spaceFactor = SetMeasureFields(doc, pL, xdDiff);
			break;

		case PSMEAStype:
			SetPSMeasFields(doc, pL, xdDiff);
			break;

		case TUPLETtype:
			SetTupletFields(pL, xdDiff, ydDiff, xp, yp);
			break;

		case OTTAVAtype:
			SetOttavaFields(pL, xdDiff, ydDiff, xp, yp);
			break;

		case GRAPHICtype:
			SetGraphicFields(pL, xdDiff, ydDiff, xp, yp);
			break;

		case TEMPOtype:
			SetTempoFields(pL, xdDiff, ydDiff, xp, yp);
			break;

		case SPACERtype:
			SetSpaceFields(pL, xdDiff, ydDiff, xp, yp);
			break;

		case BEAMSETtype:
			SetBeamFields(pL, xdDiff, ydDiff, xp, yp);
			break;

		case SLURtype:
			SetSlurFields(pL, xdDiff, ydDiff, xp, yp);
			break;
	}
	doc->scaleCenter.h += d2p(xdDiff);
	doc->scaleCenter.v += d2p(ydDiff);
	
	SDCleanUp(doc, oldPort, spaceFactor, pL, True, xdDiff, ydDiff);
	
	if (gwPtr != NULL) DestroyGWorld(gwPtr);
	return True;
	
broken:	
	SDCleanUp(doc, oldPort, spaceFactor, pL, False, 0, 0);
	
	if (gwPtr != NULL) DestroyGWorld(gwPtr);
	return False;

noteDragDone:
	{
	short pitchLev, acc, symIndex=-1;
	
		if (InsTrackPitch(doc, pt, &symIndex, pL, staff, &pitchLev, &acc, octType)) {
			SetForNewPitch(doc, pL, subObjL, context, pitchLev, acc);
			SDCleanUp(doc, oldPort, spaceFactor, pL, True, 0, 0);
			return True;
		}
		else {
			SDCleanUp(doc, oldPort, spaceFactor, pL, False, 0, 0);
			InvalMeasure(pL, staff);
			return False;
		}
	}
}


/* ---------------------------------------------------------------------- DoSymbolDrag -- */
/* This is the entry point for all dragging routines handling normal dragging. If symbol
is beforeFirstMeas, handle it as a special case and return. Otherwise, set up the three
offscreen bitmaps and call CheckObject, which should call the appropriate CheckXXX
routine with mode SMSymDrag; the CheckXXX routines should in turn call HandleSymDrag.
<pt> is the mouseDown pt in window-local coordinates.

Returns True if it found something to drag, regardless of whether the user tried to drag
it and regardless of whether anything went wrong. */

Boolean DoSymbolDrag(Document *doc, Point pt)
{
	short 		index;
	LINK		pL, measureL, startMeas, endMeas;
	Boolean		found;
	short		pIndex;
	STFRANGE	stfRange={0,0};
	
	/* If there is no object, do nothing. */

	pL = FindAndActOnObject(doc, pt, &index, SMFind);
	if (!pL) return False;
	
	DisableUndo(doc, False);
	
	/* If the object is before the first Measure of its System, call DragBeforeFirst.
	   Exception: If object is a Graphic or is attached to a page-relative Graphic,
	   handle in this function and don't call DragBeforeFirst. */
	
	if (LinkBefFirstMeas(pL)) {
		if (!GraphicTYPE(pL) || !PageTYPE(GraphicFIRSTOBJ(pL))) {
			GetMeasRange(doc, pL, &startMeas, &endMeas);

			if (startMeas && !endMeas) {					/* Cf. comments for Graphics */
				endMeas = EndSystemSearch(doc,startMeas);
				endMeas = LSSearch(endMeas,MEASUREtype,ANYONE,GO_LEFT,False);
			}
			drag1stMeas = startMeas;
			dragLastMeas = endMeas;

			DragBeforeFirst(doc, pL, pt);
			return True;
		}
	}

	/* Not a special case. Get the following barline, and set up grafPorts for giving
	   visual feedback during the drag. */
	
	measureL = LSSearch(pL, MEASUREtype, 1, True, False);
	
	switch (ObjLType(pL)) {
		case MEASUREtype:
			/* Barlines must be draggable back into the previous measure or forward
				into the next measure. */
			if (!Setup2MeasPorts(doc, measureL)) {
				ErrDisposPorts();
				return True;
			}
			break;
		case DYNAMtype:
			if (IsHairpin(pL)) {
				DragHairpin(doc, pL);
				return True;						/* FIXME: Skip HandleSymDrag called from CheckObject below. */
			}
			else {
#ifdef DRAG_DYNAMIC_OLD_WAY
				if (!SetupMeasPorts(doc, measureL)) {
					ErrDisposPorts();
					return True;
				}
#else
				DragDynamic(doc, pL);
				return True;						/* FIXME: Skip HandleSymDrag called from CheckObject below. */
#endif
			}
			break;
		case BEAMSETtype:
			if (!SetupNMeasPorts(doc, FirstInBeam(pL), LastInBeam(pL))) {
				ErrDisposPorts();
				return True;
			}
			break;
		case OTTAVAtype:
			if (!SetupNMeasPorts(doc, FirstInOttava(pL), LastInOttava(pL))) {
				ErrDisposPorts();
				return True;
			}
			break;
		case TUPLETtype:
			if (!SetupNMeasPorts(doc, FirstInTuplet(pL), LastInTuplet(pL))) {
				ErrDisposPorts();
				return True;
			}
			break;
		case SLURtype:
			if (!SetupNMeasPorts(doc, SlurFIRSTSYNC(pL), SlurLASTSYNC(pL))) {
				ErrDisposPorts();
				return True;
			}
			break;
		case GRAPHICtype:
			if (GraphicSubType(pL)==GRChordSym) {
				DragGraphic(doc, pL);
				return True;					/* Skip HandleSymDrag called from CheckObject below. */
			}
			if (PageTYPE(GraphicFIRSTOBJ(pL))) {
				PageRelDrag(doc, pL, pt);
				return True;
			}
			else {
				if (MeasureTYPE(GraphicFIRSTOBJ(pL))) {
					GetMeasRange(doc, pL, &startMeas, &endMeas);
					if (startMeas && LinkLMEAS(startMeas))				/* Don't pass NILINK to SameSystem() */
						if (SameSystem(startMeas, LinkLMEAS(startMeas)))
							startMeas = LinkLMEAS(startMeas);
				}
				else
					GetMeasRange(doc, pL, &startMeas, &endMeas);

				/* If GetMeasRange returns a start measure and not an end measure, Graphic
					has been dragged to the right edge of the page, probably off the 
					screen. In this case, set up a grafport extending to the right end
					of the system. */
				if (startMeas && !endMeas) {
					endMeas = EndSystemSearch(doc, startMeas);
					endMeas = LSSearch(endMeas, MEASUREtype, ANYONE, GO_LEFT, False);
				}

				drag1stMeas = startMeas;
				dragLastMeas = endMeas;
				if (startMeas && endMeas && (startMeas != endMeas)) {
					if (!SetupNMeasPorts(doc, startMeas, endMeas)) {
						ErrDisposPorts();
						return True;
					}
				}
				else if (!SetupMeasPorts(doc, measureL)) {
					ErrDisposPorts();
					return True;
				}
			}
			break;
		case TEMPOtype:
			if (MeasureTYPE(TempoFIRSTOBJ(pL)))
				measureL = TempoFIRSTOBJ(pL);
			GetMeasRange(doc, pL, &startMeas, &endMeas);

			if (startMeas && !endMeas) {					/* Cf. comments for Graphics */
				endMeas = EndSystemSearch(doc, startMeas);
				endMeas = LSSearch(endMeas, MEASUREtype, ANYONE, GO_LEFT, False);
			}
			drag1stMeas = startMeas;
			dragLastMeas = endMeas;
			if (startMeas && endMeas && (startMeas != endMeas)) {
				if (!SetupNMeasPorts(doc, startMeas, endMeas)) {
					ErrDisposPorts();
					return True;
				}
			}
			else if (!SetupMeasPorts(doc, measureL)) {
				ErrDisposPorts();
				return True;
			}
			break;
		default:
			if (!SetupMeasPorts(doc, measureL)) {
				ErrDisposPorts();
				return True;
			}
			break;
	}

	GetAllContexts(doc, contextA, pL);
	CheckObject(doc, pL, &found, (Ptr)&pt, contextA, SMSymDrag, &pIndex, stfRange);
	
	doc->selStartL = pL;
	doc->selEndL = RightLINK(pL);
	DisposMeasPorts();
	
	return True;
}
