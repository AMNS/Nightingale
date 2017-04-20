/***************************************************************************
*	FILE:	DrawUtils.c
*	PROJ:	Nightingale
*	DESC:	Drawing utility routines
****************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void DrawFlat(Document *, short, short, DDIST, DDIST, DDIST, short, short *, PCONTEXT, short);
static void DrawSharp(Document *, short, short, DDIST, DDIST, DDIST, short, short *, PCONTEXT, short);
static void DrawNatural(Document *, short, short, DDIST, DDIST, DDIST, short, short *, PCONTEXT, short);

/* ------------------------------------------------------------- GetSmallerRSize -- */
/* Given a rastral size, get the next smaller "Preferred" rastral size, i.e., the
next smaller size that has an integral no. of points (and therefore an integral
no. of pixels on screen at 100% magnification) between staff lines, if such a
smaller size has at least 2 pixels between staff lines center-to-center. (1 pixel
between staff lines isn't much use, since the staff line itself needs a pixel.) */

short GetSmallerRSize(short rSize)
{
	switch (rSize) {
		case 0:
			return 1;	/* FIXME: Incorrect if rastral 0 has been defined <= to rastral 1! */
		case 1:
			return 2;
		case 2:
		case 3:
		case 4:
			return 5;
		case 5:
		case 6:
			return 7;
		default:
			return MAXRASTRAL;
	}
}


/* ------------------------------------------------------------ ContextedObjRect -- */

Rect ContextedObjRect(Document *doc, LINK pL, short staff, PCONTEXT pContext)
{
	LINK contextL; Rect r; 

	contextL = (LinkBefFirstMeas(pL) ?
						LSSearch(pL,MEASUREtype,ANYONE,GO_RIGHT,FALSE) : pL);

	GetContext(doc, contextL, staff, pContext);
	r = LinkOBJRECT(pL);
	OffsetRect(&r,pContext->paper.left,pContext->paper.top);

	return r;
}


/* Beware: In the following functions, character and string argument types are certainly
not all what you'd expect or like. */

/* -------------------------------------------------------------- DrawPaddedChar -- */
/* Draw the given character with several blanks of padding. This is to alleviate
a long-standing bug in the Font Manager(?) of many Macs that results in characters
being truncated on the right (see comments in DrawMChar): it doesn't completely
avoid the problem. */

void DrawPaddedChar(unsigned char ch)
{
	Point pt; char sch[10];

	GetPen(&pt);
	pt.h += CharWidth(ch);
	
	/* Fill string with a dummy char. and padding, then replace the dummy char. Use
		4 blanks padding, though that's still not enough in all cases. */
	
	strcpy(sch, "X    ");
	sch[0] = ch;
	DrawCString(sch);
	
	MoveTo(pt.h, pt.v);
}


/* ------------------------------------------------------------------ DrawMChar -- */
/* Draw a QuickDraw character, optionally dimmed, in the current font. NB: this
routine uses an off-screen bitmap that's set up for the largest music character
we'll ever draw in the largest staff size at the maximum magnification. That's fine
for music characters, but if this is used for drawing arbitrary text (even in the
music font!), it's possible--though pretty unlikely under ordinary circumstances--
that the bitmap's size will be exceeded; if it is, we'll draw garbage for the
portion of the character that extends beyond the bitmap and might even crash.
Programmers Beware.

Cf. our DrawPatString; perhaps this and that should somehow share code. */

#define TOP_OFFSET 10
#define BOTTOM_OFFSET 10
#define RIGHT_OFFSET 20

extern short gMCharTop;
extern short gMCharLeft;

void DrawMChar(
		Document *doc,					/* Used to get maximum char. size */
		short		mchar,
		short		shape,				/* NO_VIS=invisible, any other=normal */
		Boolean	dim 					/* Should character be dimmed? */
		)
{
	Point pt;
	GrafPtr oldPort; PenState pnState;

	/*
	 * FIXME: The following comment and the code it refers to looks grossly out-of-date.
	 * --DAB, July 2016
	 *	<kludgeFontMgr> refers to a kludge to get around what appears to be a bug in the
	 * Font Manager on many Macs that causes it sometimes to truncate characters on the
	 * right, sometimes into invisibility. It seems to occur only if the character has
	 *	pixels to the right of its declared width: for us, mostly music-font note flags
	 *	and eighth and shorter rests; also some dynamics in some sizes. Specifically:
	 *		(1) with the monitor set to 1 bit/pixel, flags and short rests are truncated
	 *			(flags into invisibility) iff the font is scaled;
	 *		(2) with the monitor set to multiple bits/pixel, they're truncated iff either
	 *			the font is scaled or the size actually displayed is at least 24 points.
	 * The kludge, suggested by Glenn Reid, draws a string containing the desired
	 * character with enough blanks appended to make it wide enough to include all
	 * the pixels. The 4 blanks we use is actually NOT enough for all cases, but it's
	 * close.
	 *
	 * The bug has been seen on the SE/30, straight II, IIci, and much more recent
	 * machines like the PowerMac 7100/66. In extensive use, it's never been seen on
	 * Plus or SE (not that anyone cares anymore). For now, we assume it can occur on
	 * straight II or ANY later machine. N.B. Apple claims System 6.0.5 fixed a bug in
	 * displaying zero-width characters on IIci's; maybe so, but these problems still
	 * occur under System 7.
	 */
	Boolean kludgeFontMgr;
	
	if (shape==NO_VIS) return;

		kludgeFontMgr = TRUE;	/* Maybe should be FALSE if System>=6.0.x for some x? */

		
	if (dim) {
		GetPen(&pt);

		GetPenState(&pnState);
		PenNormal();
		
		GetPort(&oldPort);

		short oldTxMode = GetPortTextMode(oldPort);
		TextMode(grayishTextOr);
		
		DrawChar(mchar);
		
		TextMode(oldTxMode);				
		SetPenState(&pnState);
	}
	else {
		if (kludgeFontMgr) DrawPaddedChar((unsigned char)mchar);
		else					 DrawChar(mchar);
	}
}

/* Nightingale version of DrawString which calls DrawMChar for each char in the string. */

void DrawMString(Document *doc, unsigned char *mstr, short shape, Boolean dim)
{
	short i;
	
	for (i=1; i<=mstr[0]; i++) {
		DrawMChar(doc, mstr[i], shape, dim);
		short wid = CharWidth(mstr[i]);
//LogPrintf(LOG_DEBUG, "DrawMString: CharWidth()=%d\n", wid);
		Move(wid, 0);
	}
}


/* ------------------------------------------------------------------ DrawMColon -- */
/* Draw a QuickDraw colon in the music font. Since Sonata and compatible fonts don't
include a colon, this is the do-it-yourself version: two augmentation dots, one
above or (if italic) above and to the right of the other. The augmentation dot is
bigger than a dot in a colon, so we also reduce the font size temporarily. */

void DrawMColon(Document *doc, Boolean italic, Boolean dim, DDIST lnSpace)
{
	short	oldSize, colonSize, xoffset, yoffset;
	Point	pt;
	Byte	glyph = MapMusChar(doc->musFontInfoIndex, MCH_dot);

	oldSize = GetPortTxSize();
	colonSize = oldSize/2;
	TextSize(colonSize);
	GetPen(&pt);
	xoffset = d2p(MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace)/2);
	yoffset = d2p(MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace)/2);
	Move(xoffset, yoffset);
	if (dim) DrawMChar(doc, glyph, NORMAL_VIS, TRUE);
	else		DrawChar(glyph);
	MoveTo(pt.h+xoffset, pt.v+yoffset);
	Move((italic? colonSize/5 : 0), -2*colonSize/5);
	if (dim) DrawMChar(doc, glyph, NORMAL_VIS, TRUE);
	else		DrawChar(glyph);
	TextSize(oldSize);
}


/* ---------------------------------------------------------------------------------- */
/* GetXXXXDrawInfo functions are intended to be called by CheckXXXX, DrawXXXX,
SDDrawXXXX, etc. They return various information about a subobject necessary to draw
it: always its position (xd,yd), often the music font glyph to draw, sometimes other
information. Caveat: They also change the txSize of the current port, so a call to one
of them should be preceded by code to get the port's txSize, and followed, after any
required drawing, by code to restore it. Some change the port's txSize even if
outputTo==<toPostScript>: very questionable. */

/* For clefs with a little "8" above or below, find the position of the "8". */

static void LCGet8Pos(SignedByte, DDIST, DDIST *,  DDIST *);
static void LCGet8Pos(SignedByte clefType, DDIST dLnHeight, DDIST *xdOctDelta,
								DDIST *ydOctDelta)
{
	/*
	 * Careful: the vertical origin of the italic "8" in Sonata and compatible fonts
	 * is at the bottom, but the Roman's is in the middle!
	 */
	*xdOctDelta = (DDIST)CANCEL_INT;
	
	switch (clefType) {
		case TREBLE8_CLEF:
			*xdOctDelta = (CLEF8_ITAL? (7*dLnHeight)/8 : (7*dLnHeight)/8 );
			*ydOctDelta = (CLEF8_ITAL? (-10*dLnHeight)/2 : (-11*dLnHeight)/2 );
			break;
		case TRTENOR_CLEF:
			*xdOctDelta = (CLEF8_ITAL? dLnHeight/2 : dLnHeight/2 );
			*ydOctDelta = (CLEF8_ITAL? (11*dLnHeight)/4 : (10*dLnHeight)/4 );
			break;
		case BASS8B_CLEF:
			*xdOctDelta = (CLEF8_ITAL? 0 : 0 );
			*ydOctDelta = (CLEF8_ITAL? (1*dLnHeight)/4 : (0*dLnHeight)/4 );
			break;
		default:
			;
	}
}

void GetClefDrawInfo(
			Document *doc,
			LINK pL,
			LINK aClefL,
			CONTEXT context[],
			short sizePercent,			/* Percent of "normal" size for context to use (PostScript only) */
			unsigned char *glyph,
			DDIST *xd, DDIST *yd,
			DDIST *xdOct, DDIST *ydOct	/* Position of attached "8": *xdOct=CANCEL_INT means none */
			)
{
	PACLEF aClef;
	short staffn, txSize;
	PCONTEXT pContext;
	DDIST dTop, dLeft, dLnHeight, ydR, xdOctDelta, ydOctDelta;
	Boolean small;
	
	staffn = ClefSTAFF(aClefL);
	aClef = GetPACLEF(aClefL);
	small = aClef->small;
	
	pContext = &context[staffn];
	if (ClefINMEAS(pL)) {
		dTop = pContext->measureTop;
		dLeft = pContext->measureLeft;
	}
	else {
		dTop = pContext->staffTop;
		dLeft = pContext->staffLeft;
	}
	pContext->clefType = aClef->subType;
	
	*xd = dLeft + LinkXD(pL) + aClef->xd;
	
	switch (aClef->subType) {
		case TREBLE8_CLEF:
		case TREBLE_CLEF:
		case TRTENOR_CLEF:
			*glyph = MCH_trebleclef;
			break;
		case SOPRANO_CLEF:
		case MZSOPRANO_CLEF:
		case ALTO_CLEF:
		case TENOR_CLEF:
		case BARITONE_CLEF:
			*glyph = MCH_cclef;
			break;
		case BASS_CLEF:
		case BASS8B_CLEF:
			*glyph = MCH_bassclef;
			break;
		case PERC_CLEF:
			*glyph = MCH_percclef;
			break;
		default:
			MayErrMsg("GetClefDrawInfo: illegal clef %ld", (long)aClef->subType);
	}

 	dLnHeight = LNSPACE(pContext);
 	
	if (outputTo!=toPostScript)
		if (small) {
		/*
		 * Override the given <sizePercent> and just choose a nice size for screen fonts.
		 */
		txSize = Staff2MFontSize(drSize[GetSmallerRSize(doc->srastral)]);
		sizePercent = (100*txSize)/pContext->fontSize;
		}

/* Clefs in Sonata and compatible fonts, other than the percussion clef, have their
	origins at the bottom line of a 5-line staff for their standard size and (for C
	clef) "normal" position; however, in the sense of the point on the clef whose staff
	position must be preserved when the clef is drawn in a non-standard size, each
	clef's "real" vertical origin is somewhere else:
		for treble clef, the next-to-bottom (G) line;
		for C clefs, the middle of the clef: middle line for alto, next-to-top for tenor;
		for bass clef, the next-to-top (F) line.
	Percussion clef (the real one, not the one Semipro Composer uses) doesn't even
	have its origin at the bottom line of a 5-line staff; anyway, its "real" origin
	is one line above its bottom.

	The following code computes the Sonata origin so as to preserve the clef's "real" 
	origin by moving down from the top line to the "real" origin, then scaling just the 
	portion of the staff height remaining below that. If the clef is full-size, <ydR> is
	the height of the staff; if it's small, <ydR> is somewhat less.
 */
	switch (aClef->subType) {
		case TREBLE8_CLEF:
		case TREBLE_CLEF:
		case TRTENOR_CLEF:
			ydR = 3*dLnHeight + SizePercentSCALE(1L*dLnHeight);
			break;

		/* The next cases are the C clefs, in order from the bottom line on up */
		case SOPRANO_CLEF:
			ydR = 4*dLnHeight + SizePercentSCALE(0L);
			ydR += SizePercentSCALE(2*dLnHeight);				/* Correct "normal" position */
			break;
		case MZSOPRANO_CLEF:
			ydR = 3*dLnHeight + SizePercentSCALE(1L*dLnHeight);
			ydR += SizePercentSCALE(dLnHeight);					/* Correct "normal" position */
			break;
		case ALTO_CLEF:
			ydR = 2*dLnHeight + SizePercentSCALE(2L*dLnHeight);
			break;
		case TENOR_CLEF:
			ydR = 1*dLnHeight + SizePercentSCALE(3L*dLnHeight);
			ydR -= SizePercentSCALE(dLnHeight);					/* Correct "normal" position */
			break;
		case BARITONE_CLEF:
			ydR = 0*dLnHeight + SizePercentSCALE(4L*dLnHeight);
			ydR -= SizePercentSCALE(2*dLnHeight);				/* Correct "normal" position */
			break;

		case BASS_CLEF:
		case BASS8B_CLEF:
			ydR = 1*dLnHeight + SizePercentSCALE(3L*dLnHeight);
			break;
		case PERC_CLEF:
			ydR = 2*dLnHeight + SizePercentSCALE(1L*dLnHeight);
			/* At least get *this* clef to look reasonable when staff has other than 5 lines. */
			if (pContext->staffLines!=5) {
				DDIST dHalfLnHeight = dLnHeight/2;
				switch (pContext->staffLines) {
					case 2:	ydR -= dHalfLnHeight*3;	break;
					case 3:	ydR -= dHalfLnHeight*2;	break;
					case 4:	ydR -= dHalfLnHeight;	break;
					case 6:	ydR += dHalfLnHeight;	break;
					default:	break;
				}
			}
			break;
	}
		
	/* FIXME: Note that these are obsolete with new music font scheme.   -JGG */
	switch (aClef->subType) {
		case TREBLE8_CLEF:
		case TREBLE_CLEF:
		case TRTENOR_CLEF:
		case PERC_CLEF:
			ydR += (long)(config.trebleVOffset*dLnHeight)/100L;
			break;

		case SOPRANO_CLEF:
		case MZSOPRANO_CLEF:
		case ALTO_CLEF:
		case TENOR_CLEF:
		case BARITONE_CLEF:
			ydR += (long)(config.cClefVOffset*dLnHeight)/100L;
			break;

		case BASS_CLEF:
		case BASS8B_CLEF:
			ydR += (long)(config.bassVOffset*dLnHeight)/100L;
			break;
	}

	if (outputTo!=toPostScript) {
		if (small) {
		/*
		 *	We've tried various "bald-faced kludges" to <ydR> here to correct the QuickDraw
		 * positions of small clefs, which are sometimes too high and sometimes too low,
		 * but none has worked very well. Someday we'll have to figure out what's going on.
		 */
			TextSize(UseMTextSize(txSize, doc->magnify));
		}
		else
			TextSize(UseMTextSize(pContext->fontSize, doc->magnify));
	}
	
	*yd = dTop + LinkYD(pL) + aClef->yd + ydR;
	
	/*
	 *	For clefs with a little "8" above or below, find the position of the "8".
	 */
	LCGet8Pos(aClef->subType, dLnHeight, &xdOctDelta, &ydOctDelta);

	if (xdOctDelta==(DDIST)CANCEL_INT)
		*xdOct = *ydOct = (DDIST)CANCEL_INT;
	else {
		*xdOct = *xd+xdOctDelta;
		ydOctDelta = SizePercentSCALE(ydOctDelta);
		*ydOct = *yd+ydOctDelta;
	}
}


/* Returns bounding box of a hairpin of the specified geometry. Meant to be called
from DrawHairpin and CheckDYNAMIC.  JGG, 3/13/01 */

void GetHairpinBBox(
		DDIST xd, DDIST endxd,		/* page-relative */
		DDIST yd, DDIST endyd,		/* page-relative */
		DDIST rise,					/* half of hairpin <mouthWidth>, converted to DDIST */
		DDIST offset,				/* half of hairpin <otherWidth>, converted to DDIST */
		short dynamType,			/* DIM_DYNAM or CRESC_DYNAM */
		Rect *bbox)					/* bounding box in pixels */
{
	DDIST top, bottom;

	if (dynamType==CRESC_DYNAM) {
		top = n_min(yd-offset, endyd-rise);
		bottom = n_max(yd+offset, endyd+rise);
	}
	else {
		top = n_min(yd-rise, endyd-offset);
		bottom = n_max(yd+rise, endyd+offset);
	}
	SetRect(bbox, d2p(xd), d2p(top), d2p(endxd), d2p(bottom)+1);
}


/* Returns glyph, xd and yd to the calling function. If QuickDraw, set port's txSize. */

void GetDynamicDrawInfo(
			Document *doc,
			LINK pL,
			LINK aDynamicL,
			CONTEXT context[],
			unsigned char *glyph,
			DDIST *xd, DDIST *yd
			)
{
	PDYNAMIC p;
	PADYNAMIC aDynamic;
	short staffn;
	PCONTEXT pContext;
	DDIST dTop, dLeft;
	
	p = GetPDYNAMIC(pL);
	aDynamic = GetPADYNAMIC(aDynamicL);
	staffn = DynamicSTAFF(aDynamicL);
	pContext = &context[staffn];
	pContext->dynamicType = DynamType(pL);
	dTop = pContext->measureTop;
	dLeft = pContext->measureLeft;
	
/* Make the dynamic xd relative to the firstSync of the dynamic. Offset
is contained in subObject; LinkXD(pL) is set to 0 inside NewObjPrepare. */
	*xd = SysRelxd(DynamFIRSTSYNC(pL)) + LinkXD(pL) + aDynamic->xd
					+ pContext->systemLeft;
	*yd = dTop + aDynamic->yd;
	switch (DynamType(pL)) {
		case PPP_DYNAM:
			*glyph = MCH_ppp;	break;
		case PP_DYNAM:
			*glyph = MCH_pp;	break;
		case P_DYNAM:
			*glyph = MCH_p;		break;
		case MP_DYNAM:
			*glyph = MCH_mp;	break;
		case MF_DYNAM:
			*glyph = MCH_mf;	break;
		case F_DYNAM:
			*glyph = MCH_f;		break;
		case FF_DYNAM:
			*glyph = MCH_ff;	break;
		case FFF_DYNAM:
			*glyph = MCH_fff;	break;
		case SF_DYNAM:
			*glyph = MCH_sf;	break;
		case DIM_DYNAM:
		case CRESC_DYNAM:
			break;
		default:
			MayErrMsg("GetDynamicDrawInfo: unknown dynamic type %ld at %ld",
						(long)DynamType(pL), pL);
	}
	
/* FIXME: Is this really necessary? See DrawDYNAMIC. */
	if (outputTo!=toPostScript)
		TextSize(UseMTextSize(pContext->fontSize, doc->magnify));
}


void GetRptEndDrawInfo(
			Document *doc,
			LINK pL,
			LINK aRptL,
			CONTEXT context[],
			DDIST *xd, DDIST *yd,
			Rect *rSub
			)
{
	PRPTEND	p;
	PARPTEND aRpt;
	short staff, lWidth, rWidth;
	PCONTEXT pContext;
	DDIST dTop, dLeft, dBottom;
	
	p = GetPRPTEND(pL);
	switch (p->subType) {
		case RPT_L:
			lWidth = 2; rWidth = 4;
			break;
		case RPT_R:
			lWidth = 4; rWidth = 2;
			break;
		case RPT_LR:
			lWidth = 4; rWidth = 4;
			break;
	}
	staff = RptEndSTAFF(aRptL);
	pContext = &context[staff];

	dTop = pContext->measureTop;
	dLeft = pContext->measureLeft;
	
	*xd = dLeft + LinkXD(pL);
	*yd = dTop;
	
	aRpt = GetPARPTEND(aRptL);
	
	if (!aRpt->connAbove) {
		if (aRpt->connStaff!=0) {
			dBottom = context[aRpt->connStaff].staffTop
							+context[aRpt->connStaff].staffHeight;
			SetRect(rSub, d2p(*xd)-lWidth, d2p(dTop), 
						d2p(*xd)+rWidth, d2p(dBottom));
		}
		else
			SetRect(rSub, d2p(*xd)-lWidth, d2p(dTop), 
						d2p(*xd)+rWidth,
						d2p(dTop+pContext->staffHeight));
	}
	
	if (outputTo!=toPostScript)
		TextSize(UseTextSize(pContext->fontSize, doc->magnify));
}


/* ------------------------------------------------------------------ DrawRptBar -- */
/* Draw a repeat bar, regardless of whether it's a RptEnd or a Measure subobject. */

void DrawRptBar(Document *doc,
				LINK pL,
				short staff,
				short connStaff,
				CONTEXT context[],
				DDIST dLeft,
				char subType,
				short mode,
				Boolean dotsOnly
				)
{
	PCONTEXT pContext, pContext2;
	DDIST dTop, dBottom, dBotNorm, staffBottom;
	DDIST	xdDots, ydDots, lnSpace;
	short xp, ypTop, ypBot, xOffset, ypStfBottom;
	short xpDots, ypDots, sizePercent, oldTxSize, useTxSize;
	Rect  mBBox;
	LINK prevMeasL;
	Boolean hasRptDots;
	Byte dotsGlyph;
	
	prevMeasL = LSSearch(pL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	mBBox = SDGetMeasRect(doc, pL, prevMeasL);
	pContext = &context[staff];
	switch (mode) {
		case MEDraw:
			dTop = pContext->measureTop = pContext->staffTop;
			break;
		case SDDraw:
		case SWDraw:
			dTop = pContext->measureTop = pContext->staffTop-p2d(mBBox.top);
			break;
	}

	if (connStaff==0) {
		dBotNorm = dBottom = dTop + pContext->staffHeight;			/* Not connected below */
		if (pContext->showLines==1) dBottom -= LNSPACE(pContext);			
	}
	else {
		pContext2 = &context[NextStaffn(doc,pL,FALSE,connStaff)]; /* Connected below */
		//pContext2 = &context[connStaff];									/* Connected below */
		dBottom = pContext2->staffTop + pContext2->staffHeight;
		dBotNorm = dTop + pContext->staffHeight;						/* draw rpt-dots on <staff>, NOT <connStaff> */
		if (pContext2->showLines==1) dBottom -= LNSPACE(pContext2);			
		if (mode==SDDraw || mode==SWDraw) 
			dBottom -= p2d(mBBox.top);
	}

	staffBottom = dTop + pContext->staffHeight;
	if (pContext->showLines==1) dTop += LNSPACE(pContext);

	lnSpace = LNSPACE(pContext);
	oldTxSize = GetPortTxSize();
	hasRptDots = MusFontHasRepeatDots(doc->musFontInfoIndex);
	if (hasRptDots) {
		sizePercent = 100;
		dotsGlyph = MapMusChar(doc->musFontInfoIndex, MCH_rptDots);
		xdDots = dLeft + MusCharXOffset(doc->musFontInfoIndex, dotsGlyph, lnSpace);
		ydDots = staffBottom + MusCharYOffset(doc->musFontInfoIndex, dotsGlyph, lnSpace);
	}
	else {	/* We have to use two augmentation dots, at a larger font size. */
		sizePercent = 150;	/* Aug. dots are generally smaller than the dots in MCH_rptDots, so scale them up. */
		dotsGlyph = MapMusChar(doc->musFontInfoIndex, MCH_dot);
		xdDots = dLeft + SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, dotsGlyph, lnSpace));
		/* Origin of MCH_rptDots is bottom staff line, so have to use higher ydDots for MCH_dot.
			5*lnSpace/2 is 2 and a half spaces -- where the top dot should go. */
		ydDots = (staffBottom - (5*lnSpace/2))
						+ SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, dotsGlyph, lnSpace));
		useTxSize = UseMTextSize(SizePercentSCALE(pContext->fontSize), doc->magnify);
		TextSize(useTxSize);
	}

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = d2p(dLeft);
			ypTop = d2p(dTop);
			ypBot = d2p(dBottom);
			ypStfBottom = d2p(staffBottom);
			xpDots = d2p(xdDots);
			ypDots = d2p(ydDots);
			if (mode==MEDraw) {
				xp += pContext->paper.left;
				ypTop += pContext->paper.top;
				ypBot += pContext->paper.top;
				ypStfBottom += pContext->paper.top; 
				xpDots += pContext->paper.left;
				ypDots += pContext->paper.top;
			}
			if (!dotsOnly) {
				MoveTo(xp, ypTop); LineTo(xp, ypBot);
				MoveTo(xp+2, ypTop); LineTo(xp+2, ypBot);
			}

			xOffset = 3;
			switch (subType) {
				case RPT_L:
					MoveTo(xpDots+xOffset, ypDots);
					DrawChar(dotsGlyph);
					if (!hasRptDots) {
						Move(-CharWidth(dotsGlyph), d2p(lnSpace));
						DrawChar(dotsGlyph);
					}
					break;
				case RPT_R:
					MoveTo(xpDots-4*xOffset/3, ypDots);
					DrawChar(dotsGlyph);
					if (!hasRptDots) {
						Move(-CharWidth(dotsGlyph), d2p(lnSpace));
						DrawChar(dotsGlyph);
					}
					break;
				case RPT_LR:
					MoveTo(xpDots+xOffset, ypDots);
					DrawChar(dotsGlyph);
					if (!hasRptDots) {
						Move(-CharWidth(dotsGlyph), d2p(lnSpace));
						DrawChar(dotsGlyph);
					}
					MoveTo(xpDots-4*xOffset/3, ypDots);
					DrawChar(dotsGlyph);
					if (!hasRptDots) {
						Move(-CharWidth(dotsGlyph), d2p(lnSpace));
						DrawChar(dotsGlyph);
					}
					break;
				default:
					break;
			}
			break;
		case toPostScript:
			PS_Repeat(doc, dTop, dBottom, dBotNorm, dLeft, subType, sizePercent, dotsOnly);
			break;
	}
	TextSize(oldTxSize);
}


/* ---------------------------------------------------------------- GetKSYOffset -- */

short GetKSYOffset(PCONTEXT pContext, KSITEM KSItem)
{
/* Octave-adjusted position for:		   F  E  D  C  B  A  G	(0=top staff line) */
	static SignedByte trebleSharpPos[] = { 0, 1, 2, 3, 4, 5,-1 };
	static SignedByte trebleFlatPos[]  = { 7, 1, 2, 3, 4, 5, 6 };
	
	static SignedByte sopranoSharpPos[] = { 5, 6, 7, 1, 2, 3, 4 };
	static SignedByte sopranoFlatPos[]  = { 5, 6, 7, 1, 2, 3, 4 };

	static SignedByte mzSoprSharpPos[] = { 3, 4, 5, 6, 7, 1, 2 };
	static SignedByte mzSoprFlatPos[]  = { 3, 4, 5, 6, 7, 1, 2 };

	static SignedByte altoSharpPos[]   = { 1, 2, 3, 4, 5, 6, 0 };
	static SignedByte altoFlatPos[]    = { 8, 2, 3, 4, 5, 6, 7 };
	
	static SignedByte tenorSharpPos[]  = { 6, 0, 1, 2, 3, 4, 5 };
	static SignedByte tenorFlatPos[]   = { 6, 0, 1, 2, 3, 4, 5 };
	
	static SignedByte baritoneSharpPos[]  = { 4, 5, 6, 7, 1, 2, 3 };
	static SignedByte baritoneFlatPos[]   = { 4, 5, 6, 7, 1, 2, 3 };
	
	static SignedByte bassSharpPos[]   = { 2, 3, 4, 5, 6, 7, 1 };
	static SignedByte bassFlatPos[]    = { 9, 3, 4, 5, 6, 7, 8 };

	short letcode, halfLn;

  	letcode = KSItem.letcode;
	switch (pContext->clefType) {
		case TREBLE8_CLEF:
		case TREBLE_CLEF:
		case PERC_CLEF:
		  	if (KSItem.sharp) halfLn = trebleSharpPos[letcode];
		  	else 				halfLn = trebleFlatPos[letcode];
		  	break;
	  	case SOPRANO_CLEF:
		  	if (KSItem.sharp) halfLn = sopranoSharpPos[letcode];
		  	else 				halfLn = sopranoFlatPos[letcode];
			break;
		case MZSOPRANO_CLEF:
		  	if (KSItem.sharp) halfLn = mzSoprSharpPos[letcode];
		  	else 				halfLn = mzSoprFlatPos[letcode];
			break;
	  	case ALTO_CLEF:
		  	if (KSItem.sharp) halfLn = altoSharpPos[letcode];
		  	else 				halfLn = altoFlatPos[letcode];
			break;
		case TENOR_CLEF:
			if (KSItem.sharp) halfLn = tenorSharpPos[letcode];
		  	else 				halfLn = tenorFlatPos[letcode];
			break;
		case BARITONE_CLEF:
			if (KSItem.sharp) halfLn = baritoneSharpPos[letcode];
		  	else 				halfLn = baritoneFlatPos[letcode];
			break;
	  	case BASS_CLEF:
	  	case BASS8B_CLEF:
		  	if (KSItem.sharp) halfLn = bassSharpPos[letcode];
		  	else 				halfLn = bassFlatPos[letcode];
			break;
		case TRTENOR_CLEF:
		default:
		  	if (KSItem.sharp) halfLn = trebleSharpPos[letcode];
		  	else 				halfLn = trebleFlatPos[letcode];
  	}
  	
  	return halfLn;
}


/* -------------------------------------------------------------------- DrawFlat -- */
/* Draw a single flat symbol in the designated position. */

static void DrawFlat(
				Document	*doc,
				short		tab,			/* item sequence no. of this flat in key signature */
				short		yPos,			/* vertical offset of this flat (half-lines) */
				DDIST		xd, DDIST yd,	/* TOP_LEFT corner of key signature */
				DDIST		staffHeight,	/* height of the staff */
				short		staffLines,		/* number of staff lines in staff */
				short		*width,			/* width of symbol drawn */
				PCONTEXT	pContext,	
				short		drawMode 		/* whether drawing to screen or bitmap for dragging */
				)
{
	DDIST	dWidth;
	short	xp, yp,			/* pixel position or offset */
			paperLeft,
			paperTop;
	Byte	glyph;
	DDIST lnSpace = LNSPACE(pContext);

	glyph = MapMusChar(doc->musFontInfoIndex, MCH_flat);
	xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

	dWidth = std2d(STD_KS_ACCSPACE, staffHeight, staffLines);
	*width = d2p(dWidth);
	paperLeft = (drawMode==MEDraw ? pContext->paper.left : 0);
	paperTop = (drawMode==MEDraw ? pContext->paper.top : 0);
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = paperLeft + d2p(xd) + tab*(*width);
			yp = paperTop + d2p(yd + halfLn2d(yPos, staffHeight, staffLines));
			MoveTo(xp, yp);
			DrawChar(glyph);
			break;
		case toPostScript:
			yp = d2pt(halfLn2d(yPos, staffHeight, staffLines));
			PS_MusChar(doc, xd+tab*dWidth, yd+pt2d(yp), glyph, TRUE, 100);
			break;
		default:
			;
	}
}


/* ------------------------------------------------------------------- DrawSharp -- */
/* Draw a single sharp symbol in the designated position. */

static void DrawSharp(
		Document	*doc,
		short		tab,				/* item sequence no. of this sharp in key signature */
		short		yPos,				/* vertical offset of this sharp (half-lines) */
		DDIST		xd, DDIST yd,		/* TOP_LEFT corner of key signature */
		DDIST		staffHeight,		/* height of the staff */
		short		staffLines,			/* number of staff lines in staff */
		short		*width,				/* width of symbol drawn */
		PCONTEXT	pContext,		
		short		drawMode 			/* whether drawing to screen or bitmap for dragging */
		)
{
	DDIST	dWidth;
	short	xp, yp,			/* pixel position or offset */
			paperLeft,
			paperTop;
	Byte	glyph;
	DDIST	lnSpace = LNSPACE(pContext);

	glyph = MapMusChar(doc->musFontInfoIndex, MCH_sharp);
	xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

	dWidth = std2d(STD_KS_ACCSPACE, staffHeight, staffLines);
	*width = d2p(dWidth);
	paperLeft = (drawMode==MEDraw ? pContext->paper.left : 0);
	paperTop = (drawMode==MEDraw ? pContext->paper.top : 0);
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = paperLeft + d2p(xd) + tab*(*width);
			yp = paperTop + d2p(yd + halfLn2d(yPos, staffHeight, staffLines));
			MoveTo(xp, yp);
			DrawChar(glyph);
			break;
		case toPostScript:
			yp = d2pt(halfLn2d(yPos, staffHeight, staffLines));
			PS_MusChar(doc, xd+tab*dWidth, yd+pt2d(yp), glyph, TRUE, 100);
			break;
		default:
			;
	}
}


/* ----------------------------------------------------------------- DrawNatural -- */
/* Draw a single natural symbol in the designated position. */

static void DrawNatural(
		Document	*doc,
		short		tab,				/* item sequence no. of this natural in (cancelling) key signature */
		short		yPos,				/* vertical offset of this natural (half-lines) */
		DDIST		xd, DDIST yd,		/* TOP_LEFT corner of key signature */
		DDIST		staffHeight,		/* height of the staff */
		short		staffLines,			/* number of staff lines in staff */
		short		*width,				/* width of symbol drawn */
		PCONTEXT	pContext,
		short		drawMode 			/* whether drawing to screen or bitmap for dragging */
		)
{
	DDIST	dWidth;
	short	xp, yp,			/* pixel position or offset */
			paperLeft,
			paperTop;
	Byte	glyph;
	DDIST	lnSpace = LNSPACE(pContext);

	glyph = MapMusChar(doc->musFontInfoIndex, MCH_natural);
	xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

	dWidth = std2d(STD_KS_ACCSPACE, staffHeight, staffLines);
	*width = d2p(dWidth);
	paperLeft = (drawMode==MEDraw ? pContext->paper.left : 0);
	paperTop = (drawMode==MEDraw ? pContext->paper.top : 0);
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = paperLeft + d2p(xd) + tab*(*width);
			yp = paperTop + d2p(yd + halfLn2d(yPos, staffHeight, staffLines));
			MoveTo(xp, yp);
			DrawChar(glyph);
			break;
		case toPostScript:
			yp = d2pt(halfLn2d(yPos, staffHeight, staffLines));
			PS_MusChar(doc, xd+tab*dWidth, yd+pt2d(yp), glyph, TRUE, 100);
			break;
		default:
			;
	}
}


/* ------------------------------------------------------------------ DrawKSItems -- */

void DrawKSItems(Document *doc,
		LINK pL, LINK aKeySigL,
		PCONTEXT pContext,
		DDIST xd, DDIST yd,
		DDIST height,
		short lines,
		short *totalWidth,			/* in pixels */
		short drawMode
		)
{
	LINK prevKSL, aPrevKSL;
	PKEYSIG prevKS;
	PAKEYSIG aKeySig, aPrevKS;
	short staffn, k, yoffset, tab, width;
	
	aKeySig = GetPAKEYSIG(aKeySigL);
	staffn = KeySigSTAFF(aKeySigL);
	tab = 0;
	
  	if (aKeySig->nKSItems>0) {									/* Is it a "real" key sig.? */
		if (aKeySig->visible || doc->showInvis) {
			for (k = 0; k<aKeySig->nKSItems; k++) {				/* For each item... */
				yoffset = GetKSYOffset(pContext, aKeySig->KSItem[k]);
				if (aKeySig->KSItem[k].sharp)
					DrawSharp(doc, tab++, yoffset, xd, yd, height, lines, &width, pContext, drawMode);
				else
					DrawFlat(doc, tab++, yoffset, xd, yd, height, lines, &width, pContext, drawMode);
			}
		}
	}
	else {														/* No, so cancel previous key sig. */
		prevKSL = LSSearch(LeftLINK(pL), KEYSIGtype, staffn, GO_LEFT, FALSE);
		if (prevKSL)	{										/* Anything to change from? */
			prevKS = GetPKEYSIG(prevKSL);
			aPrevKSL = FirstSubLINK(prevKSL);
			for ( ; aPrevKSL; aPrevKSL = NextKEYSIGL(aPrevKSL)) {
				aPrevKS = GetPAKEYSIG(aPrevKSL);
				if (KeySigSTAFF(aPrevKSL)==staffn) break;			/* Get keysig for curr staff */
			}
		  	if (aPrevKS->nKSItems>0) {
		  		if (aKeySig->visible || doc->showInvis) 
					for (k = 0; k<aPrevKS->nKSItems; k++) {		/* For each item... */
							yoffset = GetKSYOffset(pContext, aPrevKS->KSItem[k]);
							DrawNatural(doc, tab++, yoffset, xd, yd, height, 
												lines, &width, pContext, drawMode);
					}
				aKeySig->subType = aPrevKS->nKSItems;
			}
		}
	}

	*totalWidth = tab*width;
}


/* ------------------------------------------------------------- GetKeySigDrawInfo -- */

void GetKeySigDrawInfo(Document *doc,
			LINK pL, LINK aKeySigL,
			CONTEXT context[],
			DDIST *xd, DDIST *yd,
			DDIST *dTop,
			DDIST *height,
			short *lines
			)
{
	PMEVENT p;
	PAKEYSIG aKeySig;
	DDIST dLeft;
	short k;
	CONTEXT localContext;
	
	p = GetPMEVENT(pL);
	aKeySig = GetPAKEYSIG(aKeySigL);
	localContext = context[KeySigSTAFF(aKeySigL)];
	if (((PKEYSIG)p)->inMeasure) {
		*dTop = localContext.measureTop;
		dLeft = localContext.measureLeft;
	}
	else {
		*dTop = localContext.staffTop;
		dLeft = localContext.staffLeft;
	}
	localContext.nKSItems = aKeySig->nKSItems;				/* Copy this key sig. */
	for (k = 0; k<aKeySig->nKSItems; k++)					/* into the localContext */
		localContext.KSItem[k] = aKeySig->KSItem[k];
	*xd = dLeft + LinkXD(pL) + aKeySig->xd;
	*yd = *dTop;
	*height = localContext.staffHeight;
	*lines = localContext.staffLines;
	
	if (outputTo!=toPostScript)
		TextSize(UseMTextSize(localContext.fontSize, doc->magnify));
}


/* ----------------------------------------------------------------- FillTimeSig -- */
/* Fill in numerator and denominator strings for aTimeSig, depending on the
timeSig's subType. Also return x and y position for numerator and denominator.
Return value is the timeSig's subType. */

short  FillTimeSig(Document *doc,
			LINK aTimeSigL,
			PCONTEXT pContext,
			unsigned char *nStr, unsigned char *dStr,
			DDIST xd,
			DDIST *xdN, DDIST *xdD,
			DDIST dTop,
			DDIST *ydN, DDIST *ydD
			)
{
	short subType, staffPtHt, nWidth, dWidth, nWider;
	PATIMESIG aTimeSig;
	DDIST lnSpace = LNSPACE(pContext);
	
	aTimeSig = GetPATIMESIG(aTimeSigL);
	subType = aTimeSig->subType;

	if (subType==N_OVER_D) {
		NumToString(aTimeSig->numerator, nStr);
		NumToString(aTimeSig->denominator, dStr);
	}
	else {
		nStr[0] = 1;
		if (subType==C_TIME)
			nStr[1] = MCH_common;
		else if (subType==CUT_TIME)
			nStr[1] = MCH_cut;
		else if (subType==ZERO_TIME)
			nStr[1] = '0';
		else
			NumToString(aTimeSig->numerator, nStr);			/* Overwrite nStr[0] */
	}
	/* We have to do this here for NPtStringWidth to work correctly below. */
	MapMusPString(doc->musFontInfoIndex, nStr);
	MapMusPString(doc->musFontInfoIndex, dStr);

	*xdN = *xdD = xd;
	if (subType==N_OVER_D) {
		/* Center numerator and denominator in an area as wide as the wider of the two. */
		
		staffPtHt = d2pt(pContext->staffHeight);
		nWidth = NPtStringWidth(doc, nStr, doc->musicFontNum, staffPtHt, 0);
		dWidth = NPtStringWidth(doc, dStr, doc->musicFontNum, staffPtHt, 0);
		nWider = nWidth-dWidth;
		if (nWider>0) *xdD += pt2d(nWider)/2;
		if (nWider<0) *xdN += pt2d(-nWider)/2;
		 
		*ydN = dTop + halfLn2d(2, pContext->staffHeight, pContext->staffLines);
		*ydD = dTop + halfLn2d(6, pContext->staffHeight, pContext->staffLines);
	}
	else
		*ydN = dTop + halfLn2d(4, pContext->staffHeight, pContext->staffLines);
	
	/* Just assuming the first element is as wide/tall as any following ones -- not great. */
	*xdN += MusCharXOffset(doc->musFontInfoIndex, nStr[1], lnSpace);
	*xdD += MusCharXOffset(doc->musFontInfoIndex, nStr[1], lnSpace);
	*ydN += MusCharYOffset(doc->musFontInfoIndex, nStr[1], lnSpace);
	*ydD += MusCharYOffset(doc->musFontInfoIndex, dStr[1], lnSpace);

	return subType;
}

/* ----------------------------------------------------------- GetTimeSigDrawInfo -- */
/* Get basic x and y position for the given time signature. To get the exact
positions of the numerator and denominator, pass these values to FillTimeSig.
If QuickDraw, set the port's font size. */

void GetTimeSigDrawInfo(Document *doc,
				LINK pL, LINK aTimeSigL,
				PCONTEXT pContext,
				DDIST *xd, DDIST *yd 			/* Basic time signature position */
				)
{
	PTIMESIG p;
	PATIMESIG aTimeSig;
	DDIST dLeft, dTop;
	
	p = GetPTIMESIG(pL);
	if (p->inMeasure) {
		dTop = pContext->measureTop;
		dLeft = pContext->measureLeft;
	}
	else {
		dTop = pContext->staffTop;
		dLeft = pContext->staffLeft;
	}
	aTimeSig = GetPATIMESIG(aTimeSigL);
	pContext->timeSigType = aTimeSig->subType;
	pContext->numerator = aTimeSig->numerator;
	pContext->denominator = aTimeSig->denominator;

	if (outputTo!=toPostScript)
		TextSize(UseMTextSize(pContext->fontSize, doc->magnify));

	*xd = dLeft + LinkXD(pL) + aTimeSig->xd;
	*yd = dTop + p->yd + aTimeSig->yd;
}


/* -------------------------------------------------------------- GetRestDrawInfo -- */

short GetRestDrawInfo(Document *doc,
				LINK pL, LINK aRestL,
				PCONTEXT pContext,
				DDIST *xd, DDIST *yd,
				DDIST *ydNorm,
				DDIST *dTop,
				char *lDur
				)
{
	PANOTE aRest;
	DDIST dLeft,lnSpace,dhalfLn;
	short useTxSize,glyph;
	Boolean breveWholeMeasure;

	/* Allow whole-measure rests in some time signatures to look like breve rests. */

	breveWholeMeasure = WholeMeasRestIsBreve(pContext->numerator, pContext->denominator);

	aRest = GetPANOTE(aRestL);
	
	if (aRest->subType<=WHOLEMR_L_DUR)		*lDur = (breveWholeMeasure? BREVE_L_DUR : WHOLE_L_DUR);
	else if (aRest->subType==UNKNOWN_L_DUR)	*lDur = BREVE_L_DUR; /* Should never happen */
	else									*lDur = aRest->subType;

	*dTop = pContext->measureTop;
	dLeft = pContext->measureLeft;

	useTxSize = UseMTextSize(pContext->fontSize, doc->magnify);
	if (aRest->small) useTxSize = SMALLSIZE(useTxSize);			/* A bit crude: cf. Ross */
	if (outputTo!=toPostScript) TextSize(useTxSize);

	*xd = dLeft + LinkXD(pL) + aRest->xd;
	*yd = *ydNorm = *dTop + aRest->yd;
	
	lnSpace = LNSPACE(pContext);
	dhalfLn = lnSpace/2;
	*yd += dhalfLn*restYOffset[*lDur];
	
	glyph = MCH_rests[*lDur-1];
	return glyph;
}


/* ---------------------------------------------------------------- GetModNRInfo -- */
/* GetModNRInfo is analagous to the various GetXXXDrawInfo routines in that it
provides information necessary to draw a specififed note modifier. Returns TRUE if
<code> is legal, else FALSE.

NB: In the Sonata font, The PostScript fingerings are smaller than the bitmapped ones;
the PostScript circle is larger than the tiny bitmapped one. Check both PostScript and
bitmapped chars. before adjusting sizes! Of course, this procedure could also return
different values depending on <outputTo> (and the bitmapped circle is so small, that
might be a good idea for it). But none of this may be true for other fonts, even
Sonata compatible fonts. Oh well. */

Boolean GetModNRInfo(
				short code,
				short noteType,
				Boolean	small,						/* TRUE=note/rest modNR is attached to is small */
				Boolean	above,						/* TRUE=modNR is above its note/rest */
				unsigned char *glyph,				/* Blank=not a char. in music font, else the char. */
				short *xOffset, short *yOffset,		/* in eighth-spaces */
				short *sizePct
				)
{
	short xOff, yOff;
	
	xOff = yOff = 0;
	*sizePct = (small? SMALLSIZE(100) : 100);
	
	switch (code) {
		case MOD_FERMATA:
			*glyph = (above? MCH_fermata : MCH_fermataBelow);
			xOff = -5; yOff = (above? 4 : -4);
			break;
		case MOD_TRILL:
			*glyph = MCH_fancyTrill;
			yOff = 4;
			break;
		case MOD_ACCENT:
			*glyph = MCH_accent;
			yOff = 3;
			break;
		case MOD_HEAVYACCENT:
			*glyph = (above? MCH_heavyAccent : MCH_heavyAccentBelow);
			yOff = 5;
			break;
		case MOD_STACCATO:
			*glyph = MCH_staccato;
			xOff = 4;					/* 3 looks better on screen in some sizes, 4 in others */
			break;
		case MOD_WEDGE:
			*glyph = (above? MCH_wedge : MCH_wedgeBelow);
			xOff = 2; yOff = 2;
			break;
		case MOD_TENUTO:
			*glyph = MCH_tenuto;
			break;
		case MOD_MORDENT:
			*glyph = MCH_mordent;
			xOff = -4; yOff = 4;
			break;
		case MOD_INV_MORDENT:
			*glyph = MCH_invMordent;
			xOff = -4; yOff = 4;
			break;
		case MOD_TURN:
			*glyph = MCH_turn;
			xOff = -4;
			break;
		case MOD_PLUS:
			*glyph = MCH_plus;
			break;
		case MOD_CIRCLE:
			*glyph = MCH_circle;
			xOff = 2;
			*sizePct = (small? SMALLSIZE(CIRCLE_SIZEPCT) : CIRCLE_SIZEPCT);
			break;
		case MOD_UPBOW:
			*glyph = MCH_upbow;
			xOff = 1; yOff = 6;
			break;
		case MOD_DOWNBOW:
			*glyph = MCH_downbow;
			yOff = 5;
			break;
		case MOD_HEAVYACC_STACC:
			*glyph = (above? MCH_heavyAccAndStaccato : MCH_heavyAccAndStaccatoBelow);
			yOff = 5;
			break;
		case MOD_LONG_INVMORDENT:
			*glyph = MCH_longInvMordent;
			xOff = -6; yOff = 4;
			break;
		
		/* The following modifiers are special cases in one way or another. */
		
		case MOD_TREMOLO1:
		case MOD_TREMOLO2:
		case MOD_TREMOLO3:
		case MOD_TREMOLO4:
		case MOD_TREMOLO5:
		case MOD_TREMOLO6:
			*glyph = ' ';					/* These are not chars., they're drawn */
			break;
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			*glyph = '0'+code;
			xOff = (code==1? 2 : 1);		/* '1' is narrower than the other digits */
			*sizePct = (small? SMALLSIZE(FINGERING_SIZEPCT) : FINGERING_SIZEPCT);
			break;
		default:
			return FALSE;
	}
	
	/* Heads for long notes are wider, which affects centering */
	if (WIDEHEAD(noteType)) xOff++;
	*xOffset = xOff;
	*yOffset = yOff;
	
	return TRUE;
}


/* -------------------------------------------------------------------- NoteXLoc -- */
/* Return the horizontal position of the left edge of the given note or rest. Also
return the "normal" horizontal position of the note, which is also the "normal"
position of its accidental, if any. */

DDIST NoteXLoc(
			LINK syncL, LINK aNoteL,
			DDIST measLeft,
			DDIST headWidth,
			DDIST *pxdNorm 			/* "Normal" position of note (origin of accidental) */
			)
{
	PANOTE aNote;
	DDIST dLeft, xd;
	LINK mainNoteL;
	Boolean stemDown;
	
	aNote = GetPANOTE(aNoteL);
	dLeft = measLeft + LinkXD(syncL);					/* abs. origin of Sync */
	xd = *pxdNorm = dLeft + aNote->xd;					/* abs. position of note/rest */
	if (aNote->otherStemSide) {
		mainNoteL = FindMainNote(syncL, NoteVOICE(aNoteL));
		stemDown = (NoteYSTEM(mainNoteL) > NoteYD(mainNoteL));
		if (stemDown) xd -= headWidth;
		else		  	  xd += headWidth;
	}
	
	return xd;
}


/* ------------------------------------------------------------------ GRNoteXLoc -- */
/* Return the horizontal position of the left edge of the given grace note. Also
return the horizontal origin of its accidental, if any. */

DDIST GRNoteXLoc(
			LINK syncL, LINK aGRNoteL,
			DDIST measLeft,
			DDIST headWidth,
			DDIST *pxdNorm 					/* Origin of accidental */
			)
{
	PAGRNOTE aGRNote;
	DDIST dLeft, xd;
	LINK mainGRNoteL;
	Boolean stemDown;
	
	aGRNote = GetPAGRNOTE(aGRNoteL);
	dLeft = measLeft + LinkXD(syncL);					/* abs. origin of GRSync */
	xd = *pxdNorm = dLeft + aGRNote->xd;				/* abs. position of grace note */
	if (aGRNote->otherStemSide) {
		mainGRNoteL = FindGRMainNote(syncL, GRNoteVOICE(aGRNoteL));
		stemDown = (GRNoteYSTEM(mainGRNoteL) > GRNoteYD(mainGRNoteL));
		if (stemDown) xd -= headWidth;
		else		  xd += headWidth;
	}
	
	return xd;
}


/*------------------------------------------------------------------ NoteLedgers -- */
/*	Draw zero or more ledger lines appropriate for one or two notes, one on the normal
side of the stem (required), the other "suspended", i.e., on the other side of the
the stem (and optional). Starting with the ledger line closest to the staff, we draw
ledger lines both to the right and the left as far as the suspended note, then only
to the normal side with a (presumably short) extension the other way as far as the
normal note.

If there is a suspended note, the normal note cannot be closer to the staff than it. 
Also, one note cannot be above the staff and the other below the staff! Therefore,
drawing all the ledger lines for a chord may require two calls to this function. */

void NoteLedgers(
			DDIST		xd,				/* DDIST horizontal position of normal-side notes */
			QDIST		yqpit,			/* QDIST vertical position rel. to staff top of normal note, */
			QDIST		yqpitSus,		/* 		of suspended (other side of stem) note (0=none) */
			Boolean		stemDown,
			DDIST		staffTop,		/* abs DDIST position of staff top */
			PCONTEXT	pContext,		/* for staff height and paper */
			short		sizePercent 	/* Percent of "normal" ledger length for the staff height */
			)
{
	QDIST	lineqd, midlineqd, startqd;
	short	leftNow, lenNow, ox, oy, nlines, showLines;
	DDIST	staffHt, dLLen, dLOtherLen, dShortLen, dLongLen,
			dLLeft, dLSusLeft, dLeftNow, dLenNow, dSticksOut, dHeadWidth;
	long	longDLLen;
	DDIST	dLnHeight;

	staffHt = pContext->staffHeight;
	nlines = pContext->staffLines;
	showLines = pContext->showLines;
	
	/* Scale the normal-side length by <sizePercent> but not the other-side length. */
	
 	dLnHeight = LNSPACE(pContext);
	longDLLen = (long)(config.ledgerLLen*dLnHeight)/32L;
	dLLen = SizePercentSCALE(longDLLen);
	dLOtherLen = (long)(config.ledgerLOtherLen*dLnHeight)/32L;

	dShortLen = dLLen+dLOtherLen;								/* Notes on one side of stem */
	dLongLen = 2*dLLen;											/* Notes on both sides */
	dHeadWidth = SizePercentSCALE(HeadWidth(dLnHeight));
	dSticksOut = dLLen-dHeadWidth;
	dLLeft = (stemDown? xd-dLOtherLen : xd-dSticksOut);			/* Normal */
	dLSusLeft = (stemDown? xd-dLLen : xd-dSticksOut);			/* For suspended notes */

	midlineqd = (nlines/2)*4;

	switch(outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			ox = pContext->paper.left;
			oy = pContext->paper.top;
			if (yqpit<=midlineqd) {									/* Above staff */
				if (showLines==SHOW_ALL_LINES)
					startqd = -4;									/* 1st ledger above staff */
				else {
					startqd = (showLines==0)? 8 : 4;
					if (yqpitSus==0) yqpitSus = midlineqd;
				}
				/* The 4's below are the QDIST distance between staff lines. */
				for (lineqd = startqd; lineqd>=yqpit; lineqd -= 4) {
					if (lineqd>=yqpitSus) {
						leftNow = d2p(dLSusLeft); lenNow = d2p(dLongLen)-1;
					}
					else {
						leftNow = d2p(dLLeft); lenNow = d2p(dShortLen)-1;
					}
					MoveTo(ox+leftNow, oy+d2p(staffTop+qd2d(lineqd, staffHt, nlines)));
					Line(lenNow, 0);
				}
			}
			else {													/* Below staff */
				if (showLines==SHOW_ALL_LINES)
					startqd = 4*nlines;								/* 1st ledger below staff */
				else
					startqd = (showLines==0)? 8 : 12;
				for (lineqd = startqd; lineqd<=yqpit; lineqd += 4) {
					if (lineqd<=yqpitSus) {
						leftNow = d2p(dLSusLeft); lenNow = d2p(dLongLen)-1;
					}
					else {
						leftNow = d2p(dLLeft); lenNow = d2p(dShortLen)-1;
					}
					MoveTo(ox+leftNow, oy+d2p(staffTop+qd2d(lineqd, staffHt, nlines)));
					Line(lenNow, 0);
				}
			}
			break;
			
		case toPostScript:
			if (yqpit<0)												/* Above staff */
				for (lineqd = -4; lineqd>=yqpit; lineqd -= 4) {
					if (lineqd>=yqpitSus) {
						dLeftNow = dLSusLeft; dLenNow = dLongLen;
					}
					else {
						dLeftNow = dLLeft; dLenNow = dShortLen;
					}
					PS_LedgerLine(staffTop+qd2d(lineqd, staffHt, nlines), dLeftNow,
										dLenNow);
				}
			else														/* Below staff */
				for (lineqd = 4*nlines; lineqd<=yqpit; lineqd += 4) {
					if (lineqd<=yqpitSus) {
						dLeftNow = dLSusLeft; dLenNow = dLongLen;
					}
					else {
						dLeftNow = dLLeft; dLenNow = dShortLen;
					}
					PS_LedgerLine(staffTop+qd2d(lineqd, staffHt, nlines), dLeftNow,
										dLenNow);
				}
			break;
	}
}


/*---------------------------------------------------------------- InsertLedgers -- */
/*	Draw pseudo-ledger lines for insertion feedback for notes and other
symbols that are vertically positionable at the appropriate half-line number */

void InsertLedgers(
			DDIST		xd,			/* DDIST horizontal position of note rel. to page */
			short		halfLine,	/* half-line number */
			PCONTEXT	pContext 	/* current context */
			)
{
	DDIST		staffTop,
				staffHt;
	short		l,					/* current half-line number */
				staffLines,	
				showLines,	
				ledgerLeft,			/* left coordinate of ledger line */
				ledgerLen;			/* length of ledger line */

	PenNormal();
	staffTop = pContext->staffTop;
	staffHt = pContext->staffHeight;
	staffLines = pContext->staffLines;
	showLines = pContext->showLines;
	ledgerLen = d2p(InsLedgerLen(LNSPACE(pContext)));
	ledgerLeft = pContext->paper.left + d2p(xd) - ledgerLen/2;
	if (showLines!=SHOW_ALL_LINES) {		/* 5-line staff, but showing 0 or 1 lines */
		if (halfLine<4) {
			short startLn = (showLines==0)? 4 : 2;
			for (l=startLn; l>=halfLine; l-=2) {
				MoveTo(ledgerLeft,
					pContext->paper.top+d2p(staffTop+halfLn2d(l, staffHt, staffLines)));
				Line(ledgerLen, 0);
			}
		}
		else {
			short startLn = (showLines==0)? 4 : 6;
			for (l=startLn; l<=halfLine; l+=2) {
				MoveTo(ledgerLeft,
					pContext->paper.top+d2p(staffTop+halfLn2d(l, staffHt, staffLines)));
				Line(ledgerLen, 0);
			}
		}
	}
	else {
		if (halfLine<0)
			for (l=-2; l>=halfLine; l-=2) {
				MoveTo(ledgerLeft,
					pContext->paper.top+d2p(staffTop+halfLn2d(l, staffHt, staffLines)));
				Line(ledgerLen, 0);
			}
		else
			for (l=2*staffLines; l<=halfLine; l+=2) {
				MoveTo(ledgerLeft,
					pContext->paper.top+d2p(staffTop+halfLn2d(l, staffHt, staffLines)));
				Line(ledgerLen, 0);
			}
	}
}


/* ----------------------------------------------------------------- GetMBRestBar -- */
/* Get the bounding DRect of the horizontal bar for a multibar rest. */

void GetMBRestBar(short nMeasure, PCONTEXT pContext, DRect *pdBar)
{
	DDIST dTop, dQuarterSp, dBarHt, dBarLen;

	dTop = pContext->measureTop;
	dQuarterSp = LNSPACE(pContext)/4;

	dBarHt = config.mbRestHeight*dQuarterSp;
	dBarLen = (config.mbRestBaseLen+(nMeasure-2)*config.mbRestAddLen)*dQuarterSp;
	SetDRect(pdBar, 0, -dBarHt/2, dBarLen, dBarHt/2);
}


/* ---------------------------------------------------- VisStavesInRange/ForPart -- */

static void VisStavesInRange(Document *, LINK, short, short, short *, short *);
static void VisStavesInRange(
				Document *doc,
				LINK staffL,
				short r1stStaff, short rLastStaff,		/* Input: first and last staves of range */
				short *firstVisStf, short *lastVisStf 	/* Output: first and last visible staves in range */
				)
{
	*firstVisStf = NextStaffn(doc, staffL, TRUE, r1stStaff);
	if (*firstVisStf<=0 || *firstVisStf>rLastStaff) *firstVisStf = -1;
	
	*lastVisStf = NextStaffn(doc, staffL, FALSE, rLastStaff);
	if (*lastVisStf<=0 || *lastVisStf<r1stStaff) *lastVisStf = -1;
}

/* For the given Staff object, return the numbers of the top and bottom visible
staves in the given part. If no staves in the part are visible, return -1 for
both top and bottom. */

void VisStavesForPart(
			Document *doc,
			LINK staffL,
			LINK partL,
			short *firstStaff, short *lastStaff
			)
{
	PPARTINFO pPart;
	
	pPart = GetPPARTINFO(partL);
	VisStavesInRange(doc, staffL, pPart->firstStaff, pPart->lastStaff,
								firstStaff, lastStaff);
}


/* ----------------------------------------------------------- ShouldDrawConnect -- */
/* For the given CONNECT subobject, if we should actually draw it, return TRUE;
else return FALSE. */

Boolean ShouldDrawConnect(
			Document *doc,
			LINK	pL,
			LINK	aConnectL,
			LINK	staffL 		/* Staff the Connect is attached to */
			)
{
	LINK partL; Boolean drawThisOne; PACONNECT aConnect;
	short firstStaff,lastStaff;
	
	if (doc->masterView)
		drawThisOne = TRUE;
	else {
		/*
		 *	CMN convention is usually to not show Connects if they include only one
		 *	staff, except on groups (instrument choirs, etc.), where they're shown if
		 * ANY staves are visible.
		 */
		aConnect = GetPACONNECT(aConnectL);
		switch (aConnect->connLevel) {
			case SystemLevel:
				/* Zero visible staves in the system should never happen. */
				drawThisOne = (NumVisStaves(pL)>1 || config.show1StfSysConn);
				break;
			case GroupLevel:
				VisStavesInRange(doc, staffL, aConnect->staffAbove, aConnect->staffBelow,
										&firstStaff, &lastStaff);
				drawThisOne = (firstStaff>=0 || lastStaff>=0);
				break;
			case PartLevel:
				partL = FindPartInfo(doc, Staff2Part(doc, aConnect->staffAbove));
				VisStavesForPart(doc, staffL, partL, &firstStaff, &lastStaff);
				drawThisOne = (	(firstStaff>=0 || lastStaff>=0)
									&& (firstStaff!=lastStaff || config.show1StfPartConn) );
				break;
		}
	}
	
	return drawThisOne;
}

/* --------------------------------------------------------- ShouldREDrawBarline -- */
/* For the given RPTEND subobject, if we should draw draw the barline proper as well
as the repeat dots (if there are any), return TRUE with *connStf set. If we should
draw only the repeat dots (if any), simply return FALSE. */

Boolean ShouldREDrawBarline(
		Document *doc,
		LINK rptEndObjL,		/* RptEnd object */
		LINK theRptEndL,		/* RptEnd subobject */
		short *connStf 			/* If function returns TRUE, staff no. to draw down to */
		)
{
	PARPTEND theRptEnd, aRptEnd; LINK aRptEndL, topRptEndL;
	
	theRptEnd = GetPARPTEND(theRptEndL);

	/* If this the top staff of a group or not in a group, draw barline. */
	
	if (theRptEnd->connStaff!=0 || !theRptEnd->connAbove)
		{ topRptEndL = theRptEndL; goto DrawAll; }
	
	/*
	 * This is a "subordinate" staff, i.e., one in a group and not the top staff
	 * of the group. Draw Dots Only unless all staves above this one in its group
	 * are invisible.
	 */
	aRptEndL = FirstSubLINK(rptEndObjL);
	for ( ; aRptEndL; aRptEndL = NextRPTENDL(aRptEndL)) {
		if (aRptEndL==theRptEndL) break;
		aRptEnd = GetPARPTEND(aRptEndL);
		if (aRptEnd->connStaff!=0)
			topRptEndL = aRptEndL;
	}
	for (aRptEndL = topRptEndL; aRptEndL; aRptEndL = NextRPTENDL(aRptEndL)) {
		if (aRptEndL==theRptEndL) break;
		aRptEnd = GetPARPTEND(aRptEndL);
		if (aRptEnd->visible || doc->showInvis) return FALSE;
	}
	
DrawAll:
	aRptEnd = GetPARPTEND(topRptEndL);
	*connStf = aRptEnd->connStaff;
	return TRUE;
}


/* ----------------------------------------------------------- ShouldDrawBarline -- */
/* For the given MEASURE subobject,if we should draw the barline proper as well as
the repeat dots (if there are any), return TRUE with the staff number to draw to;
if we should draw only the repeat dots (if any), return FALSE. We consider staff
visibility as well as barline grouping of staves.

NB: This function should really ignore doc->showInvis, since "Show Invisibles"
doesn't affect invisible staves; not ignoring it makes Show Invisibles sometimes
draw barlines to random endpoints. (Likewise for its siblings, ShouldREDrawBarline
and ShouldPSMDrawBarline.) However, ignoring doc->showInvis also prevents Show
Invisibles from drawing barlines that are invisible when their staves are visible 
(including the first barline of every system). A proper fix will be messy. */

Boolean ShouldDrawBarline(
				Document *doc,
				LINK measObjL,		/* Measure object */
				LINK theMeasL,		/* Measure subobject */
				short *connStf 		/* If function returns TRUE, staff no. to draw down to */
				)
{
	PAMEASURE theMeas, aMeas; LINK aMeasL;
	short theStf, gTopStf, gBottomStf;
	
	theMeas = GetPAMEASURE(theMeasL);
	if (!theMeas->visible && !doc->showInvis) return FALSE;

	/* If this the top staff of a group or not in a group, draw barline. */
	
	if (theMeas->connStaff!=0 || !theMeas->connAbove)
			{ gBottomStf = theMeas->connStaff; goto DrawAll; }
	
	/*
	 * This is a "subordinate" staff, i.e., one in a group and not the top staff
	 * of the group. Draw dots only unless all staves above this one in its group
	 * are invisible.
	 */
	theStf = MeasureSTAFF(theMeasL);
	aMeasL = FirstSubLINK(measObjL);
	for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
		aMeas = GetPAMEASURE(aMeasL);
		if (aMeas->connStaff!=0)
			if (MeasureSTAFF(aMeasL)<theStf && aMeas->connStaff>=theStf) {
				gTopStf = MeasureSTAFF(aMeasL);
				gBottomStf = aMeas->connStaff;
			}
	}

	/*
	 * We know the top staff of the group; now see if it and all staves below it and
	 *	above <theStf> are invisible.
	 */
	aMeasL = FirstSubLINK(measObjL);
	for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
		if (MeasureSTAFF(aMeasL)>=gTopStf && MeasureSTAFF(aMeasL)<theStf) {
			aMeas = GetPAMEASURE(aMeasL);
			if (aMeas->visible || doc->showInvis) return FALSE;
		}
	}
	
DrawAll:
	*connStf = gBottomStf;
	return TRUE;
}


/* --------------------------------------------------------- ShouldPSMDrawBarline -- */
/* For the given Pseudomeasure subobject,if we should draw the barline proper as well
as any staff-specific stuff (if there is any), return TRUE with the staff number to
draw to; if we should draw only the staff-specific stuff (if any), return  FALSE. */

Boolean ShouldPSMDrawBarline(
		Document *doc,
		LINK measObjL,			/* Pseudomeasure object */
		LINK thePSMeasL,		/* Pseudomeasure subobject */
		short *connStf 			/* If function returns TRUE, staff no. to draw down to */
		)
{
	PAPSMEAS thePSMeas, aPSMeas; LINK aPSMeasL;
	short theStf, gTopStf, gBottomStf;
	
	/* If this the top staff of a group or not in a group, draw barline. */
	
	thePSMeas = GetPAPSMEAS(thePSMeasL);
	if (thePSMeas->connStaff!=0 || !thePSMeas->connAbove)
		{ gBottomStf = thePSMeas->connStaff; goto DrawAll; }
	
	/*
	 * This is a "subordinate" staff, i.e., one in a group and not the top staff
	 * of the group. Skip barline unless all staves above this one in its group
	 * are invisible.
	 */
	theStf = PSMeasSTAFF(thePSMeasL);
	aPSMeasL = FirstSubLINK(measObjL);
	for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL)) {
		aPSMeas = GetPAPSMEAS(aPSMeasL);
		if (aPSMeas->connStaff!=0)
			if (PSMeasSTAFF(aPSMeasL)<theStf && aPSMeas->connStaff>=theStf) {
				gTopStf = PSMeasSTAFF(aPSMeasL);
				gBottomStf = aPSMeas->connStaff;
			}
	}

	/*
	 * We know the top staff of the group; now see if it and all staves below it and
	 *	above <theStf> are invisible.
	 */
	aPSMeasL = FirstSubLINK(measObjL);
	for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL)) {
		if (PSMeasSTAFF(aPSMeasL)>=gTopStf && PSMeasSTAFF(aPSMeasL)<theStf) {
			aPSMeas = GetPAPSMEAS(aPSMeasL);
			if (aPSMeas->visible || doc->showInvis) return FALSE;
		}
	}
	
DrawAll:
	*connStf = gBottomStf;
	return TRUE;
}


/* ------------------------------------------------------------ ShouldDrawMeasNum -- */
/* For the given Measure subobject,if we should draw the measure number above or below
it, return TRUE. The rules are, draw a measure number if measure numbers are wanted
at all; this is a "real" measure (see the IsFakeMeasure function); this is the top
or bottom (depending on doc->aboveMN) visible staff of the system; and this
particular measure is one whose number should be shown, given the current settings of
doc->startMNPrint1 and of the interval between numbers or "start of system only". */

Boolean ShouldDrawMeasNum(
		Document	*doc,
		LINK		measObjL,		/* Object */
		LINK		theMeasL 		/* Subobject */
		)
{
	PAMEASURE aMeasure; short measureNum, topVisStf, botVisStf; LINK staffL;
	
	if (doc->numberMeas==0) return FALSE;	

	if (MeasISFAKE(measObjL)) return FALSE;

	aMeasure = GetPAMEASURE(theMeasL);
	measureNum = aMeasure->measureNum+doc->firstMNNumber;

	staffL = LSSearch(measObjL, STAFFtype, ANYONE, GO_LEFT, FALSE);	/* Must always exist */
	topVisStf = NextStaffn(doc, staffL, TRUE, 1);
	botVisStf = NextStaffn(doc, staffL, FALSE, doc->nstaves);
	if (doc->aboveMN && MeasureSTAFF(theMeasL)!=topVisStf) return FALSE;
	if (!doc->aboveMN && MeasureSTAFF(theMeasL)!=botVisStf) return FALSE;
	
	if (measureNum<(doc->startMNPrint1? 1 : 2)) return FALSE;
	
	if (doc->numberMeas>0? (measureNum % doc->numberMeas)!=0
									: !FirstMeasInSys(measObjL)) return FALSE;
	
	return TRUE;
}


/* ------------------------------------------------------------- DrawVDashedLine -- */
/* Draw a vertical dashed line from (x1,y1) to (x2,y2) (QuickDraw only). */

#define DASH_LEN 2	/* pixels */

void DrawVDashedLine(short, short, short, short);
void DrawVDashedLine(short x1, short y1, short /*x2*/, short y2)
{
	short i;
	
	MoveTo(x1, y1);
	for (i = y1; i+DASH_LEN<y2; i+=2*DASH_LEN) {
		LineTo(x1, i+DASH_LEN);			/* Draw dash */
		MoveTo(x1, i+2*DASH_LEN);		/* Move to start of next dash */
	}
	LineTo(x1, y2);						/* Close out dashed line. */
}


/* --------------------------------------------------------------- DrawPSMSubType -- */
/* Draw PseudoMeas subTypes (QuickDraw only). */

void DrawPSMSubType(short subType, short xp, short ypTop, short ypBot)
{
	short betweenBars;
	
	betweenBars = 2;
	switch (subType) {
		case PSM_DOTTED:
 			DrawVDashedLine(xp, ypTop, xp, ypBot);
			break;
		case PSM_DOUBLE:
			MoveTo(xp, ypTop);
			LineTo(xp, ypBot);
			MoveTo(xp+betweenBars, ypTop);
			LineTo(xp+betweenBars, ypBot);
			break;
		case PSM_FINALDBL:
			MoveTo(xp, ypTop);
			LineTo(xp, ypBot);
			PenSize(2,1);
			MoveTo(xp+betweenBars, ypTop);
			LineTo(xp+betweenBars, ypBot);
			PenNormal();
			break;
		default:
			break;
	}
}

/* ------------------------------------------------------------------ DrawNonArp -- */
/* Draw a non-arpeggio sign (QuickDraw only). */

void DrawNonArp(short xp, short yp, DDIST dHeight, DDIST lnSpace)
{
	MoveTo(xp, yp);
	Line(d2p(NONARP_CUTOFF_LEN(lnSpace)), 0);
	MoveTo(xp, yp);
	Line(0, d2p(dHeight));
	Line(d2p(NONARP_CUTOFF_LEN(lnSpace)), 0);
}

/* --------------------------------------------------------------------- DrawArp -- */
/* Draw an arpeggio sign (QuickDraw only). Sonata has a nice arpeggio-section
character we use.  */

void DrawArp(Document *doc, short xp, short yTop, DDIST yd, DDIST dHeight,
			 Byte glyph, PCONTEXT pContext)
{

	DDIST lnSpace,charHeight,ydHere;
	short oldFont,oldStyle; short yp;

	lnSpace = LNSPACE(pContext);
	charHeight = 2*lnSpace;				/* Sonata arpeggio char. is 2 spaces high */

	oldFont = GetPortTxFont();
	oldStyle = GetPortTxFace();

	TextFont(doc->musicFontNum);
	TextFace(0);

	for (ydHere = yd; ydHere-yd<dHeight; ydHere += charHeight) {
		yp = d2p(ydHere+charHeight);
		MoveTo(xp, yTop+yp);
		DrawChar(glyph);
	}

	TextFont(oldFont);
	TextFace(oldStyle);
}


/* ------------------------------------------------------------------ TempoGlyph -- */
/* Return the glyph to draw for the given Tempo subType. We use noteheads with
(for stemmed durations) stem up. */

char TempoGlyph(LINK pL)
{
	PTEMPO p;

	p = GetPTEMPO(pL);
	switch (p->subType) {
		case BREVE_L_DUR:
			return 0xDD;			/* On Mac, '›'=shift-option 4 */
		case WHOLE_L_DUR:
			return 'w';
		case HALF_L_DUR:
			return 'h';
		case QTR_L_DUR:
			return 'q';
		case EIGHTH_L_DUR:
			return 'e';
		case SIXTEENTH_L_DUR:
			return 'x';
		case THIRTY2ND_L_DUR:
			return 'r';
		case SIXTY4TH_L_DUR:
			return 0xC6;			/* On Mac, '∆'=option j */
		case ONE28TH_L_DUR:
			return 0x8D;			/* On Mac, 'ç'=option c */
		case NO_L_DUR:
		default:
			return '\0';
	}
}


/* ------------------------ Functions to get drawing info for Graphics and Tempos -- */
/* GetXXXDrawInfo function for Graphics and Tempos. For Tempos, this simply uses the
position of the object the Graphic or Tempo is attached to, not the subobject; for
Graphics, it uses the position of the subobj in the Graphic's voice or on its staff
to get the xd. If the Graphic or Tempo is attached to the page, it always returns 1;
otherwise it just returns _staffn_. */

short GetGraphicOrTempoDrawInfo(
			Document *doc,
			LINK pL, LINK relObjL,
			short staffn,
			DDIST *xd, DDIST *yd,
			PCONTEXT pRelContext					/* Context at <relObjL> */
			)
{
	LINK measL;

	if (PageTYPE(relObjL)) {
		measL = LSSearch(relObjL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
		GetContext(doc, measL, 1, pRelContext);
		*xd = LinkXD(pL);
		*yd = LinkYD(pL);
		return 1;
	}

	GetContext(doc, relObjL, staffn, pRelContext);
	if (GraphicTYPE(pL))
		*xd = GraphicPageRelxd(doc, pL, relObjL, pRelContext);
	else
		*xd = PageRelxd(relObjL, pRelContext);
	*yd = PageRelyd(relObjL, pRelContext);

	*xd += LinkXD(pL);
	*yd += LinkYD(pL);
	return staffn;
}

/* GetXXXDrawInfo function for the right end of GRDraw Graphics. */

short GetGRDrawLastDrawInfo(
			Document *doc,
			LINK graphicL, LINK lastObjL,
			short staffn,
			DDIST *xd2, DDIST *yd2
			)
{
	CONTEXT context; PGRAPHIC p;

	if (PageTYPE(lastObjL)) {
		*xd2 = LinkXD(graphicL);
		*yd2 = LinkYD(graphicL);
		return 1;
	}

	GetContext(doc, lastObjL, staffn, &context);
	*xd2 = GraphicPageRelxd(doc, graphicL,lastObjL, &context);
	*yd2 = PageRelyd(lastObjL, &context);

	p = GetPGRAPHIC(graphicL);
	*xd2 += p->info;
	*yd2 += p->info2;
	return staffn;
}

/* Get font information for the given Graphic object. Meaningful only for subtypes that
are characters or strings: not GRDraw, e.g. */

void GetGraphicFontInfo(
			Document *doc,
			LINK pL,
			PCONTEXT pRelContext,
			short *pFontID,
			short *pFontSize,					/* in points, ignoring magnification */
			short *pFontStyle
			)
{
	DDIST lnSpace; PGRAPHIC p;

	lnSpace = LNSPACE(pRelContext);
	if (GraphicSubType(pL)==GRChordFrame) {
		*pFontID = config.chordFrameFontID;
		if (*pFontID<=0) *pFontID = 123;					/* 123 is default, Seville */
		*pFontSize = GetTextSize(config.chordFrameRelFSize, config.chordFrameFontSize, lnSpace);
		*pFontStyle = 0;									/* Plain */
	}
	else {
		p = GetPGRAPHIC(pL);
		*pFontID = doc->fontTable[p->fontInd].fontID;
		*pFontSize = GetTextSize(p->relFSize, p->fontSize, lnSpace);
		*pFontStyle = p->fontStyle;
	}
}


/* ----------------------------------------------------------------- Voice2Color -- */
/* Return an appropriate color for drawing the given internal voice number. If it's
a default voice or an error is found, use black; otherwise, within each part's
voices, cycle among a number of colors. Nightingale doesn't use Color QuickDraw,
so this version uses original QuickDraw colors, and only a few colors with enough
contrast are available (besides black and white). */

#define COLOR_CYCLE_LEN 4

short Voice2Color(Document *doc, short iVoice)
{
	short colors[COLOR_CYCLE_LEN] = { redColor, greenColor, cyanColor, magentaColor };
	LINK aPartL;
	short userVoice;
	short colorIndex, nPartStaves, extraVoice;
	
	if (doc->colorVoices==0 || iVoice<0)
		return blackColor;

	/*
	 * There should never be a problem converting internal voice number to user voice
	 * number, but if there is, don't worry about it, just use black.
	 */
	if (Int2UserVoice(doc, iVoice, &userVoice, &aPartL)) {
		if (doc->colorVoices==2)
			extraVoice = userVoice-1;
		else {
			nPartStaves = PartLastSTAFF(aPartL)-PartFirstSTAFF(aPartL)+1;
			extraVoice = userVoice-nPartStaves;
		}
	}
	else
		extraVoice = 0;	

	if (extraVoice<=0) return blackColor;
	else {
		colorIndex = (extraVoice-1) % COLOR_CYCLE_LEN;
		return colors[colorIndex];
	}
}

/* ------------------------------------------------------------------- CheckZoom -- */
/* If there is a mouse down event pending in the given Document's zoom box,
deliver TRUE; otherwise FALSE. */

static Boolean doingCheckZoom = TRUE;

Boolean CheckZoom(Document *doc)
{
	EventRecord event; Boolean gotZoom = FALSE;
	short part; WindowPtr w;
	
	if (doingCheckZoom)
		if (EventAvail(mDownMask,&event)) {
			part = FindWindow(event.where,&w);
			if (w==doc->theWindow && (part==inZoomIn || part==inZoomOut))
				gotZoom = TRUE;
		}
	return(gotZoom);
}

/* ---------------------------------------------------------- DrawCheckInterrupt -- */
/*
This scans the Operating System event queue for any of various command keys that
should interrupt drawing. If one is found while the corresponding menu command is
enabled, return TRUE to signify that the user thinks they have issued that command.
Otherwise, we continue searching down the event queue for some other command that
might be enabled.  If none are found, we return FALSE.  The problem this solves is,
for instance, that of changing magnification during the middle of updating. We
want to stop drawing at the old magnification as soon as possible and start over
at the new magnification. This is tricky for the following reason:

If the user issues a magnify command that is disabled due to the document view
already being at its extreme magnification, she may then immediately issue the
opposite command to magnify in the other direction.  This places two events into
the queue, and we must not stop scanning the queue at the first command key match
without also including the enabling/disabling conditions in the match criteria.
This is a bit messy, but worth the loss of frustration for the user during lengthy
updating.  This explanation is a bit messier, but worth the loss of confusion
to you, the programmer (the user is the programee), during lengthy debugging.

Rather than hard-coding the standard key equivalents, as this version does, it
would be much better to determine the key equivalents at run time, so users
could remap commands and still be able to interrupt drawing. Of course it'd be
best to get the actual key equivalents, but that doesn't sound easy; a compromise
would be to get the key equivalents we look for from a STR# resource (presumably
in the application itself, since that's where the key equivs. are set!), with
separate strings for the keys that have different enabling conditions. */

Boolean DrawCheckInterrupt(Document */*doc*/)
{
	return FALSE;
}


/* ------------------------------------------------------------- MaySetPSMusSize -- */
/* If we're writing PostScript and the score includes any nonstandard staff sizes,
i.e., if it might have staves in two or more sizes, set the PostScript staff size. */

void MaySetPSMusSize(Document *doc, PCONTEXT pContext)
{
	if (doc->nonstdStfSizes && outputTo==toPostScript) {
		short lines = pContext->staffLines;
	
		if (lines == 5) {
			PS_MusSize(doc, d2pt(pContext->staffHeight)+config.musFontSizeOffset);
		}
		else {
			short stfHeight = drSize[doc->srastral];
			PS_MusSize(doc, d2pt(stfHeight)+config.musFontSizeOffset);			
		}
		
		LogPrintf(LOG_NOTICE, "MaySetPSMusSize: calling PS_MusSize, stfHt %ld, musFontSzOffst %ld\n", pContext->staffHeight,
																						config.musFontSizeOffset);		
	}
}
