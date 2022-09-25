/*****************************************************************************************
*	FILE:	DrawNRGR.c
*	PROJ:	Nightingale
*	DESC:	Routines to draw notes, rests, grace notes, and modifers
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

static void DrawSlashes(DDIST, DDIST, short, Boolean, Boolean, CONTEXT *, Boolean);
static void DrawAugDots(Document *, LINK, DDIST, DDIST, PCONTEXT, Boolean);
static void DrawNCLedgers(LINK, PCONTEXT, LINK, DDIST, DDIST, short);
static void DrawNotehead(Document *, unsigned char, Byte, Boolean, DDIST);
static void GetNoteheadRect(unsigned char, Byte, DDIST, Rect *);
static void DrawMBRest(Document *, PCONTEXT, LINK, DDIST, DDIST, short, Boolean, Rect *);
static void ShowNGRSync(Document *, LINK, CONTEXT []);
static void DrawGRAcc(Document *, PCONTEXT, LINK, DDIST, DDIST, Boolean, short);
static void DrawGRNCLedgers(LINK, PCONTEXT, LINK, DDIST, DDIST, short);

/* ----------------------------------------------------------------------- DrawSlashes -- */
/* Draw the specified number of tremolo slashes at the given position. Some editions
vary the angle of the slashes for beamed notes; Gould says that's OK (in _Behind Bars_),
but she recommends a fixed angle, and that's what we do, */

static void DrawSlashes(DDIST xdh, DDIST ydh,
					short nslashes,
					Boolean centerAroundNote,	/* True=horizontally center at note center, else at stem */
					Boolean stemUp,
					PCONTEXT pContext,
					Boolean dim
					)
{
	DDIST slashLeading, slashWidth, slashHeight, dxpos, dypos, slashThick;
	DDIST lnSpace, dEighthLn;
	short xp, yp;
	short i, xslash, yslash, slashLeadingp, dxposp, dyposp;
	
	lnSpace = LNSPACE(pContext);
	dEighthLn = LNSPACE(pContext)/8;

	/* Set the distance between slashes and their width, height, and thickness.
	   According to Gould, they should fill the space between staff lines, but that's
	   much too steep! */
	   
	slashLeading = (stemUp? 6*dEighthLn : -6*dEighthLn);
	slashWidth = HeadWidth(lnSpace);
	slashHeight = lnSpace/2;
	slashThick = (long)(config.tremSlashLW*lnSpace) / 100L;
	
	if (centerAroundNote) dxpos = 0;
	else				  dxpos = (stemUp? 4*dEighthLn : -5*dEighthLn);
	dypos = (stemUp? 8*dEighthLn : -8*dEighthLn);
	

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = pContext->paper.left+d2p(xdh);
			yp = pContext->paper.top+d2p(ydh);
			if (dim) PenPat(NGetQDGlobalsGray());
			PenSize(1, d2p(slashThick)>1? d2p(slashThick) : 1);
			
			/* Some circumnavigations to avoid passing negative args to d2p */
			
			dxposp = (dxpos >= 0 ? d2p(dxpos) : -d2p(-dxpos));
			dyposp = (dypos >= 0 ? d2p(dypos) : -d2p(-dypos));
			xslash = xp+dxposp;
			yslash = yp+dyposp;
			
			slashLeadingp = (slashLeading >= 0 ? d2p(slashLeading) : -d2p(-slashLeading));
		
			for (i = 1; i<=nslashes; i++) {
				MoveTo(xslash, yslash);
				Line(d2p(slashWidth), -d2p(slashHeight));
				yslash += slashLeadingp;
			}

			PenNormal();	
			break;
		case toPostScript:
			for (i = 1; i<=nslashes; i++) {
				PS_LineVT(xdh+dxpos, ydh+dypos, xdh+dxpos+slashWidth,
								ydh+dypos-slashHeight, slashThick);
				ydh += slashLeading;
			}
			break;
		default:
			;
	}

}


/* ------------------------------------------------------------------------ Draw1ModNR -- */

void Draw1ModNR(Document *doc, DDIST xdh, DDIST ydMod, short code, unsigned char glyph,
						CONTEXT *pContext, short sizePercent, Boolean dim)
{
	DDIST yd;
	short oldTxSize, useTxSize, xHead, yMod;
	
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			oldTxSize = GetPortTxSize();
			useTxSize = UseMTextSize(SizePercentSCALE(pContext->fontSize), doc->magnify);

			xHead = pContext->paper.left+d2p(xdh);
			yMod = pContext->paper.top+d2p(pContext->staffTop+ydMod);
			MoveTo(xHead, yMod);
			switch (code) {
				case MOD_FERMATA:
				case MOD_TRILL:
				case MOD_HEAVYACCENT:
				case MOD_ACCENT:
				case MOD_STACCATO:
				case MOD_WEDGE:
				case MOD_TENUTO:
				case MOD_MORDENT:
				case MOD_INV_MORDENT:
				case MOD_TURN:
				case MOD_PLUS:
				case MOD_UPBOW:
				case MOD_DOWNBOW:
				case MOD_HEAVYACC_STACC:
				case MOD_LONG_INVMORDENT:
					if (dim)	DrawMChar(doc, glyph, NORMAL_VIS, True);
					else		DrawChar(glyph);
					break;
				case MOD_CIRCLE:
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
					TextSize(useTxSize);
					if (dim)	DrawMChar(doc, glyph, NORMAL_VIS, True);
					else		DrawChar(glyph);
					TextSize(oldTxSize);
					break;
				default:
					;
			}
			break;
		case toPostScript:
			yd = pContext->staffTop+ydMod;
			switch (code) {
				case MOD_FERMATA:
				case MOD_TRILL:
				case MOD_HEAVYACCENT:
				case MOD_ACCENT:
				case MOD_STACCATO:
				case MOD_WEDGE:
				case MOD_TENUTO:
				case MOD_MORDENT:
				case MOD_INV_MORDENT:
				case MOD_TURN:
				case MOD_PLUS:
				case MOD_UPBOW:
				case MOD_DOWNBOW:
				case MOD_HEAVYACC_STACC:
				case MOD_LONG_INVMORDENT:
				case MOD_CIRCLE:					
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
					PS_MusChar(doc, xdh, yd, glyph, True, sizePercent);
					break;
				default:
					;
			}
			break;
	}
}


/* ------------------------------------------------------------------------- DrawModNR -- */
/* Draw all of <aNoteL>'s note/rest modifiers. Assumes the NOTEheap is locked; does not
assume the MODNRheap is. */

void DrawModNR(Document *doc,
						LINK aNoteL,		/* Subobject (may be note or rest) */
						DDIST xd,			/* Note/rest position */
						CONTEXT	*pContext)
{
	PANOTE		aNote;
	LINK		aModNRL;
	PAMODNR		aModNR;
	DDIST		xdMod, ydMod, ydh, lnSpace;
	unsigned char glyph;
	short		code, sizePercent, xOffset, yOffset;
	Boolean		dim, centerAroundNote, stemUp;
				
	aNote = GetPANOTE(aNoteL);

	aModNRL = aNote->firstMod;
	if (aModNRL) {
		dim = (outputTo==toScreen && !LOOKING_AT(doc, aNote->voice));
		for ( ; aModNRL; aModNRL=NextMODNRL(aModNRL)) {
			aModNR = GetPAMODNR(aModNRL);
			code = aModNR->modCode;
			xdMod = xd+std2d(aModNR->xstd-XSTD_OFFSET, pContext->staffHeight,
								pContext->staffLines);
			ydMod = std2d(aModNR->ystdpit, pContext->staffHeight, pContext->staffLines);
			if (!GetModNRInfo(code, aNote->subType, aNote->small,
									(ydMod<=aNote->yd), &glyph, &xOffset, &yOffset, &sizePercent)) {
				MayErrMsg("DrawModNR: illegal MODNR code %ld in voice %d, note/rest L%ld",
							(long)code, NoteVOICE(aNoteL), (long)aNoteL);
				return;
			}

			lnSpace = LNSPACE(pContext);
			xdMod += (lnSpace/8)*xOffset;
			ydMod += (lnSpace/8)*yOffset;

			glyph = MapMusChar(doc->musFontInfoIndex, glyph);
			xdMod += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
			ydMod += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

			if (code>=MOD_TREMOLO1 && code<=MOD_TREMOLO6) {
				ydh = pContext->staffTop+NoteYSTEM(aNoteL)+ydMod;
				centerAroundNote = (NoteType(aNoteL)<=WHOLE_L_DUR);
				stemUp = (NoteYD(aNoteL)>NoteYSTEM(aNoteL));
				DrawSlashes(xdMod, ydh, code-MOD_TREMOLO1+1, centerAroundNote, stemUp,
								pContext, dim);
			}
			else
				Draw1ModNR(doc, xdMod, ydMod, code, glyph, pContext, sizePercent, dim);
		}
	}
}


/* ----------------------------------------------------------------------- DrawAugDots -- */
/* Draw all the augmentation dots for the given note or rest, if there are any. */

static void DrawAugDots(Document *doc,
					LINK theNoteL,			/* Subobject (note/rest) to draw dots for */
					DDIST xdNorm, DDIST yd,	/* Notehead/rest origin, excluding effect of <otherStemSide> */
					PCONTEXT pContext,
					Boolean	chordNoteToR	/* Note in a chord that's upstemmed w/notes to right of stem? */
					)
{
	PANOTE	theNote;
	DDIST 	xdDots, ydDots, dhalfLn;
	short	ndots, xpDots, yp;
	Boolean	dim;							/* Should it be dimmed bcs in a voice not being looked at? */
	Boolean doNoteheadGraphs;
	Byte	glyph = MapMusChar(doc->musFontInfoIndex, MCH_dot);

	theNote = GetPANOTE(theNoteL);
	if (theNote->ndots==0 || theNote->yMoveDots==0) return;	/* If no dots or dots invisible */

	dhalfLn = LNSPACE(pContext)/2;
	doNoteheadGraphs = (doc->graphMode==GRAPHMODE_NHGRAPHS);
	xdDots = xdNorm + AugDotXDOffset(theNoteL, pContext, chordNoteToR, doNoteheadGraphs);
	ydDots = yd+(theNote->yMoveDots-2)*dhalfLn;
	
	ndots = theNote->ndots;
	dim = (outputTo==toScreen && !LOOKING_AT(doc, theNote->voice));

	xdDots += MusCharXOffset(doc->musFontInfoIndex, glyph, dhalfLn*2);
	ydDots += MusCharYOffset(doc->musFontInfoIndex, glyph, dhalfLn*2);

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			while (ndots>0) {
				xdDots += 2*dhalfLn;
				xpDots = pContext->paper.left+d2p(xdDots);
				yp = pContext->paper.top+d2p(ydDots);
				MoveTo(xpDots, yp);
				DrawMChar(doc, glyph, NORMAL_VIS, dim);
				ndots--;
			}
			break;
		case toPostScript:
			if (doc->srastral==7) ydDots += pt2d(1)/8;				/* Tiny empirical correction */
			while (ndots>0) {
				xdDots += 2*dhalfLn;
				PS_MusChar(doc, xdDots, ydDots, glyph, True, 100);
				ndots--;
			}
			break;
	}
}


/* --------------------------------------------------------------------------- DrawAcc -- */
/* Draw an accidental, if there is one, for the given note. If it's a courtesy accidental,
also draw enclosing parentheses. */

void DrawAcc(Document *doc,
				PCONTEXT pContext,
				LINK theNoteL,			/* Subobject (note) to draw accidental for */
				DDIST xdNorm, DDIST yd,	/* Notehead position, excluding effect of <otherStemSide> */
				Boolean dim,			/* Ignored if outputTo==toPostScript */
				short sizePercent,
				Boolean chordNoteToL	/* Note in a chord that's downstemmed w/ notes to left of stem? */
				)
{
	PANOTE theNote;
	DDIST accXOffset, d8thSp, xdAcc, lnSpace;
	DDIST xdLParen=0, xdRParen=0, ydParens=0;
	short xmoveAcc, xp, yp, deltaXR, deltaXL, deltaY, scalePercent=0;
	Byte accGlyph, lParenGlyph, rParenGlyph;

	theNote = GetPANOTE(theNoteL);
	if (theNote->accident==0) return;

	accGlyph = MapMusChar(doc->musFontInfoIndex, SonataAcc[theNote->accident]);

	lnSpace = LNSPACE(pContext);
	d8thSp = LNSPACE(pContext)/8;

	/* Ordinarily, the accidental position is relative to a note on the "normal" side of
	   the stem. But if note is in a chord that's downstemmed and has notes to the left
	   of the stem, its accidental must be moved to the left. */
	
	if (chordNoteToL) xdNorm -= SizePercentSCALE(HeadWidth(LNSPACE(pContext)));

	/* Consider user-specified offset <xmoveAcc>. Also, double flat is wider than the
	   other accidentals; just move it left a bit. */
	 
	xmoveAcc = (theNote->accident==AC_DBLFLAT? theNote->xmoveAcc+2 : theNote->xmoveAcc);
	accXOffset = SizePercentSCALE(AccXDOffset(xmoveAcc, pContext));
 	xdAcc = xdNorm-accXOffset;

 	/* If it's a courtesy accidental, position the (left edge of) right paren from
	   <xmoveAcc>, but closer to the note because the paren is narrower than an accidental.
	   Move the accidental to its left as specified by config.courtesyAccRXD, and the left
	   paren as given by config.courtesyAccLXD. */

	if (theNote->courtesyAcc) {
		DDIST xoffset, yoffset, dAccWidth;

		lParenGlyph = MapMusChar(doc->musFontInfoIndex, MCH_lParen);  
		rParenGlyph = MapMusChar(doc->musFontInfoIndex, MCH_rParen);

		/* FIXME: These adjustments are probably not quite right, due to combination of 
		   sizePercent, scalePercent, and the various config tweaks. */

		scalePercent = (short)(((long)config.courtesyAccPSize*sizePercent)/100L);

		xoffset = MusCharXOffset(doc->musFontInfoIndex, rParenGlyph, lnSpace);
		xdRParen = xdAcc + ((DDIST)(((long)scalePercent*xoffset)/100L));
		dAccWidth = std2d(STD_ACCWIDTH, pContext->staffHeight, pContext->staffLines);
		
		/* A paren is narrower than an accidental; move it to right to compensate */
		
		xdRParen += dAccWidth-(COURTESYACC_PARENWID(dAccWidth));
//LogPrintf(LOG_DEBUG, "DrawAcc: xdAcc=%d xoffset=%d xdRParen=%d\n", xdAcc, xoffset, xdRParen);

		deltaXR = (short)(((long)scalePercent*config.courtesyAccRXD)/100L);
		xdAcc = xdRParen-deltaXR*d8thSp;

		deltaXL = (short)(((long)scalePercent*config.courtesyAccLXD)/100L);
		xdLParen = xdAcc-deltaXL*d8thSp;
//LogPrintf(LOG_DEBUG, "DrawAcc: courtesyAccLXD=%d deltaXL=%d xdLParen=%d deltaXR=%d xdRParen=%d\n",
//config.courtesyAccLXD, deltaXL, xdLParen, deltaXR, xdRParen);
		yoffset = MusCharYOffset(doc->musFontInfoIndex, rParenGlyph, lnSpace);	/* assume both parens have same yoffset */
		deltaY = (short)(((long)scalePercent*config.courtesyAccYD)/100L);
		ydParens = yd + deltaY*d8thSp + ((DDIST)(((long)scalePercent*yoffset)/100L));
	}

	xdAcc += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, accGlyph, lnSpace));
	yd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, accGlyph, lnSpace));
//LogPrintf(LOG_DEBUG, "DrawAcc: config.courtesyAccPSize=%d sizePercent=%d scalePercent=%d xdAcc=%d\n",
//config.courtesyAccPSize, sizePercent, scalePercent, xdAcc);

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = pContext->paper.left+d2p(xdAcc);
			yp = pContext->paper.top+d2p(yd);
			MoveTo(xp, yp);
			DrawMChar(doc, accGlyph, NORMAL_VIS, dim);

			/* If it's a courtesy accidental, draw parentheses around it. */
			
			if (theNote->courtesyAcc) {
				xp = pContext->paper.left+d2p(xdLParen);
				yp = pContext->paper.top+d2p(ydParens);
				MoveTo(xp, yp);
				DrawMChar(doc, lParenGlyph, NORMAL_VIS, dim);
				xp = pContext->paper.left+d2p(xdRParen);
				MoveTo(xp, yp);
				DrawMChar(doc, rParenGlyph, NORMAL_VIS, dim);
			}

			break;
		case toPostScript:
			PS_MusChar(doc, xdAcc, yd, accGlyph, True, sizePercent);

			/* If it's a courtesy accidental, draw parentheses around it. */
			
			if (theNote->courtesyAcc) {
				PS_MusChar(doc, xdLParen, ydParens, lParenGlyph, True, scalePercent);
				PS_MusChar(doc, xdRParen, ydParens, rParenGlyph, True, scalePercent);
			}

			break;
	}
}


/* ---------------------------------------------------------- GetNoteheadInfo and ally -- */
/* Return the symbol drawn for the given note's head. */

static unsigned char NoteGlyph(short appearance, short subType);
static unsigned char NoteGlyph(short appearance, short subType)
{
	unsigned char glyph;
	
	if (appearance==X_SHAPE) glyph = MCH_xShapeHead;
	else if (appearance==HARMONIC_SHAPE) glyph = MCH_harmonicHead;
	else if (appearance==SLASH_SHAPE) glyph = '\0';					/* Not in font, must be drawn */
	else if (appearance==SQUAREH_SHAPE) glyph = MCH_squareHHead;
	else if (appearance==SQUAREF_SHAPE) glyph = MCH_squareFHead;
	else if (appearance==DIAMONDH_SHAPE) glyph = MCH_diamondHHead;
	else if (appearance==DIAMONDF_SHAPE) glyph = MCH_diamondFHead;
	else if (appearance==HALFNOTE_SHAPE) glyph = MCH_halfNoteHead;
	else {															/* Assume NORMAL_ or NO_VIS */
		if (subType==UNKNOWN_L_DUR)
			glyph = MCH_quarterNoteHead;
		else if (subType<=WHOLE_L_DUR)
			glyph = (unsigned char)MCH_notes[subType-1];
		else if (subType==HALF_L_DUR)
			glyph = MCH_halfNoteHead;
		else
			glyph = MCH_quarterNoteHead;
	}
	
	return glyph;
}

/* Given the <appearance> and <subType> of a note or grace note, return the music font 
<glyph> as function value, plus <sizePct> and <stemShorten>. Note that if <sizePct> is
very far from 100, the calling routine should probably adjust horizontal positions of
upstems. */

unsigned char GetNoteheadInfo(short appearance, short subType,
							short *sizePct,				/* percentage of normal size */
							short *stemShorten			/* in eighth-spaces */
							)
{
	unsigned char glyph;
	
	glyph = NoteGlyph(appearance, subType);
	*sizePct = 100;
	
	/* FIXME: Changing sizePct fixes the head size, but makes a mess of upstems on
		notes with harmonic heads. Sigh. */
	/* FIXME: Call a new function in MusicFont.c to tell us whether to enlarge the harmonic
		head for our music font? */
		
	if (glyph==MCH_harmonicHead && !(CapsLockKeyDown() && ShiftKeyDown()))
		*sizePct =  115;

	*stemShorten = 0;
	if (glyph==MCH_xShapeHead) *stemShorten = STEMSHORTEN_XSHAPEHEAD;
	if (glyph=='\0') *stemShorten = STEMSHORTEN_NOHEAD;

	return glyph;
}


/* --------------------------------------------------------------------- DrawNCLedgers -- */
/* Draw any ledger lines needed: if note is in a chord, for the chord's extreme notes
on both sides of the staff; if it's not in a chord, just for the note. Should be called
only once per chord, just for the MainNote. Assumes all staff lines are being shown,
and assumes the OBJECT heap is locked! */

static void DrawNCLedgers(
		LINK		syncL,
		PCONTEXT	pContext,
		LINK		aNoteL,
		DDIST		xd,
		DDIST		dTop,
		short		ledgerSizePct
		)
{
	QDIST	hiyqpit, lowyqpit,			/* y QDIST positions relative to staff top */
			hiyqpitSus, lowyqpitSus,
			yqpit;
	Boolean	stemDown;

	if (!pContext->showLedgers) return;

	stemDown = (NoteYSTEM(aNoteL) > NoteYD(aNoteL));

	GetNCLedgerInfo(syncL, aNoteL, pContext, &hiyqpit, &lowyqpit, &hiyqpitSus, &lowyqpitSus);
	if (NoteINCHORD(aNoteL)) {		
		DrawNoteLedgers(xd, hiyqpit, hiyqpitSus, stemDown, dTop, pContext, ledgerSizePct);
		DrawNoteLedgers(xd, lowyqpit, lowyqpitSus, stemDown, dTop, pContext, ledgerSizePct);
	}

	else {
		/* Not a chord. Just draw any ledger lines the single note needs. */

		yqpit =  (NoteYD(aNoteL)<0? hiyqpit : lowyqpit);
		DrawNoteLedgers(xd, yqpit, 0, stemDown, dTop, pContext, ledgerSizePct);
	}
}


/* ---------------------------------------------------------------------- DrawNotehead -- */
/* QuickDraw only! */

static void DrawNotehead(Document *doc,
					unsigned char glyph,
					Byte appearance,
					Boolean dim,				/* Should character be dimmed? */
					DDIST dhalfLn
					)
{
	DDIST thick;
	
	if (appearance==SLASH_SHAPE) {
		if (dim) PenPat(NGetQDGlobalsGray());
		
		/* The "slash notehead" is like a steeply-sloped beam, so steep we have to
		   increase the vertical thickness so it doesn't look too thin. FIXME: Actually,
		   it should have horizontal instead of vertical cutoffs! */
		   
		thick = 3*dhalfLn/2;
		PenSize(1, d2p(thick));
		Move(0, d2p(2*dhalfLn-thick/2));
		Line(d2p(2*dhalfLn), -d2p(4*dhalfLn));
		PenNormal();
	}
	else
		DrawMChar(doc, glyph, appearance, dim);
}


static void GetNoteheadRect(unsigned char glyph, Byte appearance, DDIST dhalfLn,
							Rect *pRect)
{
	DRect r;
	
	if (appearance==SLASH_SHAPE) {
		r.top = -2*dhalfLn;
		r.left = 0;
		r.bottom = 2*dhalfLn;
		r.right = 2*dhalfLn;
		D2Rect(&r, pRect);
	}
	else
		*pRect = charRectCache.charRect[glyph];
}


/* QuickDraw only! */
/* Draw a little graph as a notehead: intended to be used to visualize changes during
the note. This version simply draws one or more colored bars side by side, then
sets the note's voice's standard color. */

#define NHGRAPH_COLORS 7	/* Number of colors available for notehead graphs */

static void DrawNoteheadGraph(Document *, Byte, Boolean, DDIST, LINK, PCONTEXT);
static void DrawNoteheadGraph(Document *doc,
				Byte /* appearance */,
				Boolean dim,			/* Should notehead be dimmed? */
				DDIST dhalfLn,
				LINK aNoteL,				
				PCONTEXT pContext		/* Current context[] entry */
				)
{
	short	graphLen,
			nSegs, stripeWidth,
			rDiam,						/* rounded corner diameter */
			xorg, yorg,
			yTop, yBottom;
	QDIST	qdLen;
	Rect	graphRect, segRect;
	Point	pt;
	short	nhSegment[4];
	short segColors[NHGRAPH_COLORS] =
		{ 0, redColor, greenColor, blueColor, cyanColor, magentaColor, yellowColor };

	qdLen = NOTEHEAD_GRAPH_WIDTH;
	qdLen = n_max(qdLen, 4);								/* Insure at least one space wide */
//LogPrintf(LOG_DEBUG, "DrawNoteheadGraph: qdLenStd=%d qdLen=%d\n", qdLenStd, qdLen);
	graphLen = d2p(qd2d(qdLen, pContext->staffHeight, pContext->staffLines));

	GetPen(&pt);
	xorg = pt.h;
	yorg = pt.v;
	yTop = yorg-d2p(dhalfLn);
	yBottom = yorg+d2p(dhalfLn);
	SetRect(&graphRect, xorg, yorg-d2p(dhalfLn), xorg+graphLen, yorg+d2p(dhalfLn));
	rDiam = UseMagnifiedSize(2, doc->magnify);
	
#if 99
	for (short k=0; k<4; k++) 
		NoteSEGMENT(aNoteL, k) = rand() % NHGRAPH_COLORS;		// ??????? TEST !!!!!!!!!!!!!
		//NoteSEGMENT(aNoteL, k) = k;		// ??????? TEST !!!!!!!!!!!!!
#endif
//LogPrintf(LOG_DEBUG, "DrawNoteheadGraph: aNoteL=%u nhSegment[]=%d, %d, %d, %d\n", aNoteL,
//NoteSEGMENT(aNoteL, 0), NoteSEGMENT(aNoteL, 1), NoteSEGMENT(aNoteL, 2), NoteSEGMENT(aNoteL, 3));

	/* Copy the info on segments from <aNoteL>. If there's none (the 1st segment value
	   is 0), assume a single black segment. */
	   
	nSegs = 0;
	for (short k=0; k<4; k++) {
		nhSegment[k] = NoteSEGMENT(aNoteL, k);
		if (nhSegment[k]<=0) break;
		nSegs++;
	}
	if (nSegs==0) { nSegs = 1;  nhSegment[0] = -1; }
if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "DrawNoteheadGraph: aNoteL=%u nSegs=%d, nhSegment[]=%d, %d, %d, %d\n", aNoteL,
nSegs, nhSegment[0], nhSegment[1], nhSegment[2], nhSegment[3]);

	stripeWidth = graphLen/nSegs;
	segRect = graphRect;
	segRect.right = segRect.left+stripeWidth;
	for (short j = 0; j<nSegs; j++) {
		if (nhSegment[j]<0)	ForeColor(blackColor);
		else				ForeColor(segColors[nhSegment[j]]);
		if (dim)	FillRoundRect(&segRect, rDiam, rDiam, NGetQDGlobalsGray());
		else		PaintRoundRect(&segRect, rDiam, rDiam); 		
		segRect.left += stripeWidth;
		segRect.right += stripeWidth;
	}
	ForeColor(Voice2Color(doc, NoteVOICE(aNoteL)));
}


/* QuickDraw only! */
/* Draw a note in pianoroll style: the head, stem, and augmentation dots are replaced
by a (generally long) bar. Coordinates are in pixels. */

void DrawNotePianoroll(Document *doc, LINK aNoteL, PCONTEXT pContext, short xhead,
						short yhead, Boolean dim);
void DrawNotePianoroll(Document *doc, LINK aNoteL, PCONTEXT pContext, short xhead,
						short yhead, Boolean dim)
{
	short	spatialLen,
			rDiam;				/* rounded corner diameter */
	long	resFact;
	QDIST	qdLen, qdLenStd;
	DDIST	dhalfSp;			/* Distance between staff half-lines */
	Rect	spatialRect;
			
	dhalfSp = LNSPACE(pContext)/2;
	MoveTo(xhead, yhead);									/* position to draw head */
	resFact = RESFACTOR*(long)doc->spacePercent;
	qdLenStd = IdealSpace(doc, NotePLAYDUR(aNoteL), resFact);
	qdLen = (long)(config.pianorollLenFact*qdLenStd) / 100L;
//LogPrintf(LOG_DEBUG, "DrawNote: qdLenStd=%d pianorollLenFact=%d qdLen=%d\n", qdLenStd,
//config.pianorollLenFact, qdLen);
	qdLen = n_max(qdLen, 4);								/* Insure at least one space long */
	spatialLen = d2p(qd2d(qdLen, pContext->staffHeight, pContext->staffLines));

	SetRect(&spatialRect, xhead, yhead-d2p(dhalfSp), xhead+spatialLen, yhead+d2p(dhalfSp));
	rDiam = UseMagnifiedSize(4, doc->magnify);
	if (dim)	FillRoundRect(&spatialRect, rDiam, rDiam, NGetQDGlobalsGray());
	else		PaintRoundRect(&spatialRect, rDiam, rDiam); 

}

/* -------------------------------------------------------------------------- DrawNote -- */

void DrawNote(Document *doc,
				LINK	pL,				/* Sync note belongs to */
				PCONTEXT pContext,		/* Current context[] entry */
				LINK	aNoteL,			/* Subobject (note) to draw */
				Boolean	*drawn,			/* False until a subobject has been drawn */
				Boolean	*recalc			/* True if we need to recalc enclosing rectangle */
				)	
{
	PANOTE		aNote, bNote;
	LINK		bNoteL;
	short		flagCount,
				staffn, ypStem,
				xhead, yhead,		/* pixel coordinates */
				appearance,
				octaveLength,
				useTxSize,
				oldTxSize,			/* To restore port after small or magnified notes drawn */
				sizePercent,		/* Percent of "normal" size to draw in (for small notes) */
				headRelSize,
				headSizePct,
				ledgerSizePct,		/* Percent of "normal" size for ledger lines */
				yp, noteType,
				stemShorten,
				flagLeading,
				xadjhead, yadjhead;
	unsigned char glyph,			/* notehead symbol actually drawn */
				flagGlyph;
	DDIST		xd, yd,
				xdAdj,
				dStemLen, dStemShorten,
				xdNorm,
				dTop, dLeft,		/* abs DDIST origin */
				dhalfLn,			/* Distance between staff half-lines */
				fudgeHeadY,			/* Correction for roundoff in Sonata screen font head shape */
				breveFudgeHeadY,	/* Correction for error in Sonata breve origin (screen & PS) */
				offset, lnSpace;
	Boolean		stemDown,			/* Does note have a down-turned stem? */
				dim,				/* Should it be dimmed bcs in a voice not being looked at? */
				chordNoteToR, chordNoteToL;
	Rect		rSub;				/* bounding box for subobject */

PushLock(OBJheap);
PushLock(NOTEheap);
	staffn = NoteSTAFF(aNoteL);
	dLeft = pContext->measureLeft + LinkXD(pL);					/* abs. origin of object */
	dTop = pContext->measureTop;

	lnSpace = LNSPACE(pContext);
	dhalfLn = lnSpace/2;
	
	xd = NoteXLoc(pL, aNoteL, pContext->measureLeft, HeadWidth(lnSpace), &xdNorm);

	aNote = GetPANOTE(aNoteL);
	yd = dTop + aNote->yd;
	noteType = aNote->subType;
	if (noteType>=WHOLE_L_DUR && aNote->doubleDur) noteType--;
	
	if (noteType==BREVE_L_DUR) breveFudgeHeadY = dhalfLn*BREVEYOFFSET;
	else								breveFudgeHeadY = (DDIST)0;

	appearance = aNote->headShape;
	if ((appearance==NO_VIS || appearance==NOTHING_VIS) && doc->showInvis)
		appearance = NORMAL_VIS;
	glyph = GetNoteheadInfo(appearance, noteType, &headRelSize, &stemShorten);
	glyph = MapMusChar(doc->musFontInfoIndex, glyph);
	dStemShorten = (dhalfLn*stemShorten)/4;
	
	oldTxSize = GetPortTxSize();
	useTxSize = UseMTextSize(pContext->fontSize, doc->magnify);
	if (aNote->small) useTxSize = SMALLSIZE(useTxSize);			/* A bit crude--cf. Ross */
	sizePercent = (aNote->small? SMALLSIZE(100) : 100);

/* FIXME: Instead of <WIDEHEAD> & <IS_WIDEREST>, we should use smthg like CharRect of
the glyph to get the headwidth! the same goes for DrawMODNR and DrawRest. */

	ledgerSizePct = WIDENOTEHEAD_PCT(WIDEHEAD(noteType), sizePercent);

	/* Suppress flags if the note is beamed. */
	flagCount = (aNote->beamed? 0 : NFLAGS(noteType));

	chordNoteToR = (!aNote->rest && ChordNoteToRight(pL, aNote->voice));
	chordNoteToL = (!aNote->rest && ChordNoteToLeft(pL, aNote->voice));
	
	dStemLen = aNote->ystem-aNote->yd;

	switch (outputTo) {
	case toScreen:
	case toBitmapPrint:
	case toPICT:
		/* We need to fine-tune note Y-positions to compensate for minor inconsistencies
		in the screen fonts. An exception, almost certainly irrelevant now, is "Best
		Quality Print" on a bitmap printer; here, we SHOULD still need fine-tuning,
		though for twice the normal font size, since the ImageWriter driver
		automatically uses twice the font size and spaces the dots half the normal
		distance apart. However, for unknown reasons, best results are obtained with no
		correction. */
		 
		if (outputTo==toBitmapPrint && bestQualityPrint)
			fudgeHeadY = 0;											/* No fine Y-offset for notehead */
		else
			fudgeHeadY = GetYHeadFudge(useTxSize);					/* Get fine Y-offset for notehead */
		
		/* xhead,yhead is the position of notehead; xadjhead,yadjhead is its position 
		   with any offset (relative to the position for the Sonata font) applied. */
		   
		offset = MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
		if (offset) {
			xhead = pContext->paper.left + d2p(xd);
			xadjhead = pContext->paper.left + d2p(xd+offset);
		}
		else
			xhead = xadjhead = pContext->paper.left + d2p(xd);
		offset = MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
		if (offset) {
			yp = pContext->paper.top + d2p(yd);
			yhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
			yp = pContext->paper.top + d2p(yd+offset);
			yadjhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
		}
		else {
			yp = pContext->paper.top + d2p(yd);
			yhead = yadjhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
		}
	
		if (appearance==NOTHING_VIS) goto EndQDrawing;

		/* If note is not in voice being "looked" at, dim it */
		
		dim = (outputTo==toScreen && !LOOKING_AT(doc, aNote->voice));

		TextSize(useTxSize);
		ForeColor(Voice2Color(doc, aNote->voice));
		if (dim) PenPat(NGetQDGlobalsGray());
		
		/* First draw any accidental symbol to left of note */
		
		DrawAcc(doc, pContext, aNoteL, xdNorm, yd, dim, sizePercent, chordNoteToL);

		/* If note is not a non-Main note in a chord, draw any ledger lines needed: for
		   note or entire chord. A chord has exactly one MainNote, so this draws the
		   ledger lines exactly once. */
		
		if (MainNote(aNoteL) || !NoteINCHORD(aNoteL))
			DrawNCLedgers(pL, pContext, aNoteL, xd, dTop, ledgerSizePct);
				 
		if (doc->graphMode==GRAPHMODE_PIANOROLL)
			DrawNotePianoroll(doc, aNoteL, pContext, xhead, yhead, dim);
		else {
			MoveTo(xadjhead, yadjhead);								/* position to draw head */

			/* HANDLE UNKNOWN CMN DURATION/WHOLE/BREVE */

			if (noteType<=WHOLE_L_DUR) {
				if (doc->graphMode==GRAPHMODE_NHGRAPHS)
					DrawNoteheadGraph(doc, appearance, dim, dhalfLn, aNoteL, pContext);
				else
					DrawNotehead(doc, glyph, appearance, dim, dhalfLn);
			}

			/* HANDLE STEMMED NOTE. If the note is in a chord and is stemless, skip entire
			   business of drawing the stem and flags. */

			else {
				if (MainNote(aNoteL)) {
					short stemSpace;

					stemDown = (dStemLen>0);
					
					/* In order for stems to line up with any beamsets that have already
					   been drawn, we have to make sure that the order of computation is
					   the same here as when we drew the beams, since we are not depending
					   on any explicit data to enforce registration.  FIXME: CUT NEXT In
					   particular, the conversion from DDISTs to pixels should be done
					   *after* adding the stemspace to the note's left edge position,
					   rather than before, so that we don't get double round-off errors. 
					   This seems to fix the ancient plague of misregistration problems
					   between note stems and their beams at different magnifications. The
					   only way to ensure that this plague won't reoccur is to keep the
					   position computations for beams and note stems exactly the same (so
					   the same round-off errors occur). Cf. CalcXStem. */

					if (stemDown)
						stemSpace = 0;
					else {
						stemSpace = MusFontStemSpaceWidthPixels(doc, doc->musFontInfoIndex, lnSpace);
						Move(stemSpace, 0);
					}
					if (ABS(dStemLen)>pt2d(1)) {
						Move(0, -1);
						Line(0, d2p(dStemLen)+(stemDown? 0 : 1));				/* Draw stem */
					}
					if (flagCount) {											/* Draw any flags */
						ypStem = yhead+d2p(dStemLen);

						if (doc->musicFontNum==sonataFontNum) {
							/* The 8th and 16th note flag characters in Sonata have their
							   origins set so, in theory, they're positioned properly if
							   drawn from the point where the note head was drawn.
							   Unfortunately, the vertical position depends on the stem
							   length, which Sonata's designers assumed was always one
							   octave. The "extend flag" characters have their vertical
							   origins set right where they go. */
								
							MoveTo(xhead, ypStem);								/* Position x at head, y at stem end */
							octaveLength = d2p(7*SizePercentSCALE(dhalfLn));
							Move(0, (stemDown? -octaveLength : octaveLength));	/* Adjust for flag origin */
							if (flagCount==1)									/* Draw one (8th note) flag */
								DrawMChar(doc, (stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp),
													NORMAL_VIS, dim);
							else if (flagCount>1) {								/* Draw >=2 (16th & other) flags */
								DrawMChar(doc, (stemDown? MCH_16thFlagDown : MCH_16thFlagUp),
													NORMAL_VIS, dim);
								MoveTo(xhead, ypStem);							/* Position x at head, y at stem end */
							
								/* FIXME: SCALE FACTORS FOR FlagLeading BELOW ARE GUESSWORK. */
							
								if (stemDown)
									Move(0, -d2p(13*FlagLeading(lnSpace)/4));
								else
									Move(0, d2p(7*FlagLeading(lnSpace)/2));
								while (flagCount-- > 2) {
									if (stemDown) {
										DrawMChar(doc, MCH_extendFlagDown, NORMAL_VIS, dim);
										Move(0, -d2p(FlagLeading(lnSpace)));	/* FIXME: HUH? */
									}
									else {
										DrawMChar(doc, MCH_extendFlagUp, NORMAL_VIS, dim);
										Move(0, d2p(FlagLeading(lnSpace)));		/* FIXME: HUH? */
									}
								}
							}
						}
						else {	/* fonts other than Sonata */
							short glyphWidth; DDIST xoff, yoff;

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
								DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
							}
							else if (flagCount==2 && MusFontHas16thFlag(doc->musFontInfoIndex)) {	/* Draw 16th flag using one flag char. */
								flagGlyph = MapMusChar(doc->musFontInfoIndex,
																(stemDown? MCH_16thFlagDown : MCH_16thFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								if (xoff || yoff)
									Move(d2p(xoff), d2p(yoff));
								DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
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
									DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
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
								if (yoff || xoff) Move(d2p(xoff), d2p(yoff));
								Move(0, flagLeading);
								DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
							}
						}
					}
				}

				/* At long last, position to draw notehead and draw it, then any modifiers
				   and any augmentation dots. */
				
				MoveTo(xadjhead, yadjhead);
				if (doc->graphMode==GRAPHMODE_NHGRAPHS)
					DrawNoteheadGraph(doc, appearance, dim, dhalfLn, aNoteL, pContext);
				else
					DrawNotehead(doc, glyph, appearance, dim, dhalfLn);
			}

			DrawModNR(doc, aNoteL, xd, pContext);
			DrawAugDots(doc, aNoteL, xdNorm, yd, pContext, chordNoteToR);
		}
	
		PenPat(NGetQDGlobalsBlack()); 													/* Restore full strength after possible dimming for "look" */

EndQDrawing:
		if (*recalc) {
			
			GetNoteheadRect(glyph, appearance, dhalfLn, &rSub);
			
			/* "Proper" size rects for noteheads can be both hard to click on and hard
			   to see hilited, so we just enlarge them a bit. */
			
			InsetRect(&rSub, -1, -1);
			OffsetRect(&rSub, xhead-pContext->paper.left, yhead-pContext->paper.top);
			if (*drawn)
				UnionRect(&LinkOBJRECT(pL), &rSub, &LinkOBJRECT(pL));
			else {
				*drawn = True;
				LinkOBJRECT(pL) = rSub;
			}
		}
		ForeColor(blackColor);
		TextSize(oldTxSize);
		break;
		
	case toPostScript:
	
		if (appearance==NOTHING_VIS) break;

		/* First draw any accidental symbol to left of note */
		 
		DrawAcc(doc, pContext, aNoteL, xdNorm, yd, False, sizePercent, chordNoteToL);

		/* If note is not a non-Main note in a chord, draw any ledger lines needed: for
		   note or entire chord. A chord has exactly one MainNote, so this draws the
		   ledger lines exactly once. */
		   
		if (MainNote(aNoteL) || !NoteINCHORD(aNoteL))
			DrawNCLedgers(pL, pContext, aNoteL, xd, dTop, ledgerSizePct); 
	
	/* Now draw the note head, stem, and flags */
		
		if (doc->graphMode==GRAPHMODE_PIANOROLL)
	/* HANDLE SPATIAL (PIANOROLL) NOTATION */
		{
			;						/* FIXME: TO BE IMPLEMENTED */
		}
		else
		{
			DDIST xoff, yoff;

			headSizePct = headRelSize*sizePercent/100L;

			xoff = MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
			yoff = MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

			/* UNKNOWN DURATION, WHOLE NOTE, OR BREVE */
			
			if (noteType==UNKNOWN_L_DUR)
				PS_NoteStem(doc, xd+xoff, yd+yoff, xd+xoff, glyph, (DDIST)0, (DDIST)0, False,
								appearance!=NO_VIS, headSizePct);
			else if (noteType<=WHOLE_L_DUR)
				PS_NoteStem(doc, xd+xoff, yd+yoff+breveFudgeHeadY, xd+xoff, glyph, (DDIST)0, (DDIST)0, False,
							appearance!=NO_VIS, headSizePct);
			
			else {
				/* STEMMED NOTE. If the note is in a chord and is stemless, skip
				   entire business of drawing the stem and flags. */
				   
				if (MainNote(aNoteL)) {
					stemDown = (dStemLen>0);
					xdAdj = (headRelSize==100? 0 : NHEAD_XD_GLYPH_ADJ(stemDown, headRelSize));
					PS_NoteStem(doc, (xd+xoff)-xdAdj, yd+yoff, xd+xoff, glyph, dStemLen, dStemShorten,
										aNote->beamed, appearance!=NO_VIS, headSizePct);
					if (flagCount) {
						if (doc->musicFontNum==sonataFontNum) {
							/* The 8th and 16th note flag characters in Sonata have their
							   origins set so, in theory, they're positioned properly if
							   drawn from the point where the note head was drawn.
							   Unfortunately, the vertical position depends on the stem
							   length, which Sonata's designers assumed was always one
							   octave. The "extend flag" characters have their vertical
							   origins set right where they go. */

							octaveLength = 7*SizePercentSCALE(dhalfLn);
							ypStem = yd + dStemLen + (stemDown ? -octaveLength : octaveLength);
			
							switch (flagCount) {
								case 1:
									PS_MusChar(doc, xd, ypStem, stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp,
												True, sizePercent);
									break;
								case 2:
									PS_MusChar(doc, xd, ypStem, stemDown? MCH_16thFlagDown : MCH_16thFlagUp, True, sizePercent);
									break;
								default:
									if (stemDown) {
										ypStem = yd + dStemLen;
										while (flagCount-- > 0) {
											PS_MusChar(doc, xd, ypStem, MCH_extendFlagDown, True, sizePercent);
											ypStem -= FlagLeading(lnSpace);
											}
										}
									 else {
										ypStem = yd + dStemLen;
										while (flagCount-- > 0) {
											PS_MusChar(doc, xd, ypStem, MCH_extendFlagUp, True, sizePercent);
											ypStem += FlagLeading(lnSpace);
											}
										}
									break;
							}
						}
						else {	/* fonts other than Sonata */
							DDIST dHeadWidth = 0;
							DDIST ydStem = yd + dStemLen;

							if (!stemDown && !MusFontUpstemFlagsHaveXOffset(doc->musFontInfoIndex))
								dHeadWidth = SizePercentSCALE(MusFontStemSpaceWidthDDIST(doc->musFontInfoIndex, lnSpace));

							if (flagCount==1) {												/* Draw 8th flag. */
								flagGlyph = MapMusChar(doc->musFontInfoIndex, 
																	(stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								PS_MusChar(doc, xd+xoff+dHeadWidth, ydStem+yoff, flagGlyph, True, sizePercent);
							}
							else if (flagCount==2 && MusFontHas16thFlag(doc->musFontInfoIndex)) {	/* Draw 16th flag using one flag char. */
								flagGlyph = MapMusChar(doc->musFontInfoIndex, 
																	(stemDown? MCH_16thFlagDown : MCH_16thFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								PS_MusChar(doc, xd+xoff+dHeadWidth, ydStem+yoff, flagGlyph, True, sizePercent);
							}
							else {
								DDIST ypos;
								short count = flagCount;

								/* Draw extension flag(s) */
								
								if (stemDown) {
									flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_extendFlagDown);
									flagLeading = -DownstemExtFlagLeading(doc->musFontInfoIndex, lnSpace);
								}
								else {
									flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_extendFlagUp);
									flagLeading = UpstemExtFlagLeading(doc->musFontInfoIndex, lnSpace);
								}
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								ypos = ydStem;
								while (count-- > 1) {
									PS_MusChar(doc, xd+xoff+dHeadWidth, ypos+yoff, flagGlyph, True, sizePercent);
									ypos += flagLeading;
								}

								/* Draw 8th flag */
								ypos = ydStem + flagLeading*(flagCount-2);
								if (stemDown) {
									flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_eighthFlagDown);
									flagLeading = -Downstem8thFlagLeading(doc->musFontInfoIndex, lnSpace);
								}
								else {
									flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_eighthFlagUp);
									flagLeading = Upstem8thFlagLeading(doc->musFontInfoIndex, lnSpace);
								}
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								PS_MusChar(doc, xd+xoff+dHeadWidth, ypos+yoff+flagLeading, flagGlyph, True, sizePercent);
							}
						}
					}
				}
				else {	/* Note in chord and doesn't have stem */
				 	if (headRelSize==100)
				 		xdAdj = 0;
				 	else {
						bNoteL = FindMainNote(pL, aNote->voice);
						bNote = GetPANOTE(bNoteL);
						dStemLen = bNote->ystem - bNote->yd;
						stemDown = (dStemLen>0);
						xdAdj = NHEAD_XD_GLYPH_ADJ(stemDown, headRelSize);
				 	}
					PS_NoteStem(doc, (xd+xoff)-xdAdj, yd+yoff, xd+xoff, glyph, (DDIST)0, (DDIST)0, aNote->beamed,
										appearance!=NO_VIS, headSizePct);
				}
			}
			
			/* Note drawn: now add modifiers and augmentation dots, if any */
			DrawModNR(doc, aNoteL, xd, pContext);								/* Draw all modifiers */
			DrawAugDots(doc, aNoteL, xdNorm, yd, pContext, chordNoteToR);
		}
		
		break;
	}
	
PopLock(OBJheap);
PopLock(NOTEheap);
}


/* ------------------------------------------------------------------------ DrawMBRest -- */
/* Draw a multibar rest. */

static void DrawMBRest(Document *doc, PCONTEXT pContext,
						LINK theRestL,					/* Subobject (rest) to draw */
						DDIST xd, DDIST yd,
						short appearance,
						Boolean dim,					/* Ignored if outputTo==toPostScript */
						Rect *prSub						/* Subobject selection/hiliting box */
						)
{
	DDIST dTop, lnSpace, dhalfLn, xdNum, ydNum, endBottom, endTop, endThick;
	DRect dRestBar; Rect restBar; short numWidth;
	unsigned char numStr[20];

	GetMBRestBar(-NoteType(theRestL), pContext, &dRestBar);
	OffsetDRect(&dRestBar, xd, yd);

	NumToString((long)(-NoteType(theRestL)), numStr);
	MapMusPString(doc->musFontInfoIndex, numStr);
	short txFace = GetPortTxFace();
	numWidth = NPtStringWidth(doc, numStr, doc->musicFontNum,
									d2pt(pContext->staffHeight), txFace);
	xdNum = (dRestBar.right+dRestBar.left)/2-pt2d(numWidth)/2;
	
	lnSpace = LNSPACE(pContext);
	dhalfLn = lnSpace/2;
	endBottom = yd-lnSpace;
	endTop = yd+lnSpace;
	dTop = pContext->measureTop;
	
	/* Set number y-pos., considering that origins of Sonata digits bisect them. */
	
	ydNum = dTop-lnSpace-dhalfLn;

	/* Assuming the first element is as wide/tall as any following ones... */
	
	xdNum += MusCharXOffset(doc->musFontInfoIndex, numStr[1], lnSpace);
	ydNum += MusCharYOffset(doc->musFontInfoIndex, numStr[1], lnSpace);

	/* In each imaging model, we draw first the thick bar; then the thin vertical
	   lines at each end of the thick bar; and finally the number. */
	   
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			D2Rect(&dRestBar, &restBar);
			OffsetRect(&restBar, pContext->paper.left, pContext->paper.top);
			if (appearance!=NOTHING_VIS) {
				if (dim) PenPat(NGetQDGlobalsGray());
				PaintRect(&restBar);
	
				MoveTo(restBar.left, pContext->paper.top+d2p(endBottom));
				LineTo(restBar.left, pContext->paper.top+d2p(endTop));
				MoveTo(restBar.right, pContext->paper.top+d2p(endBottom));
				LineTo(restBar.right, pContext->paper.top+d2p(endTop));
	
				MoveTo(pContext->paper.left+d2p(xdNum), pContext->paper.top+d2p(ydNum));
				if (dim) DrawMString(doc, numStr, NORMAL_VIS, True);
				else		DrawString(numStr);
	
				PenNormal();
			}
			
			*prSub = restBar;
			InsetRect(prSub, 0, -3);
			
			break;
			
		case toPostScript:			
			PS_LineVT(dRestBar.left, dRestBar.top, dRestBar.right, dRestBar.top,
						dRestBar.bottom-dRestBar.top);

			endThick = (long)(config.mbRestEndLW*lnSpace) / 100L;
			PS_Line(dRestBar.left, endBottom, dRestBar.left, endTop, endThick);
			PS_Line(dRestBar.right, endBottom, dRestBar.right, endTop, endThick);

			PS_MusString(doc, xdNum, ydNum, numStr,100);
			break;
	}
}


/* -------------------------------------------------------------------------- DrawRest -- */

void DrawRest(Document *doc,
				LINK pL,
				PCONTEXT pContext,	/* current context[] entry */
				LINK aRestL,		/* current subobject */
				Boolean *drawn,		/* False until a subobject has been drawn */
				Boolean *recalc		/* True if we need to recalc enclosing rectangle */
				)
{
	PANOTE		aRest;
	short		xp, yp,			/* pixel coordinates */
				xpLedg, ypLedg,
				appearance,
				glyph,			/* rest symbol */
				qStemLen,
				useTxSize,
				oldTxSize,		/* To restore port after small or magnified rests drawn */
				sizePercent,	/* Percent of "normal" size to draw in (for small rests) */
				restxp, restyp;
	DDIST		xd, yd,
				dStemLen,
				ydNorm,			/* Ignoring offset for short rests */
				dTop,
				lnSpace, 		/* Distance between staff lines */
				fudgeRestY,		/* Correction for roundoff in screen font rest shape */
				yrest,
				xledg, yledg,	/* Relative offset to start of ledger line from rest origin */
				restxd, restyd;
	Rect		rSub;			/* bounding box for subobject */
	char		lDur;
	Boolean		stemDown,
				dim;			/* Should it be dimmed bcs in a voice not being looked at? */

	if (doc->graphMode==GRAPHMODE_PIANOROLL) return;	/* FIXME: REALLY DO NOTHING, E.G., WITH OBJRECT? */

PushLock(OBJheap);
PushLock(NOTEheap);
	oldTxSize = GetPortTxSize();
	glyph = GetRestDrawInfo(doc,pL,aRestL,pContext,&xd,&yd,&ydNorm,&dTop,&lDur);
	useTxSize = GetPortTxSize();			/* Set by GetRestDrawInfo */
	
	lnSpace = LNSPACE(pContext);

	aRest = GetPANOTE(aRestL);
	appearance = aRest->headShape;
	if ((appearance==NO_VIS || appearance==NOTHING_VIS) && doc->showInvis)
		appearance = NORMAL_VIS;
		
	/*
	 * Compute position of the rest's "ledger line", if it needs one. Using
	 * <config.ledgerLLen> and <config.ledgerLOtherLen> here for the length
	 * (and therefore horizontal position) is tempting, but doesn't really work.
	 * It'd be very nice to control the length of ledger lines for rests but
	 * it needs to be done a bit differently.
	 */
	xledg = yledg = (DDIST)0;
	
	if (lDur==HALF_L_DUR) {
		yrest = yd-dTop;
		if (yrest<0 || yrest>pContext->staffHeight) {
			xledg = LedgerOtherLen(lnSpace) - (lnSpace/12);
			yledg -= (lnSpace/10);
		}
	}
	 else if (lDur==WHOLE_L_DUR) {
		yrest = yd-dTop-lnSpace;
		if (yrest<0 || yrest>pContext->staffHeight) {
			xledg = LedgerOtherLen(lnSpace) - (lnSpace/12);
			yledg = lnSpace + (lnSpace/10);
		}
	 }
	
	restxd = xd;
	restyd = yd;

	sizePercent = (aRest->small? SMALLSIZE(100) : 100);

	switch (outputTo) {
	case toScreen:
	case toBitmapPrint:
	case toPICT:
		fudgeRestY = GetYRestFudge(useTxSize, lDur);			/* Get fine Y-offset for rest */
		
		/* If rest's voice is not being "looked" at, dim it */
		
		dim = (outputTo==toScreen && !LOOKING_AT(doc, aRest->voice));
		ForeColor(Voice2Color(doc, aRest->voice));

		/* Draw the rest, either multibar (ignore modifiers and aug. dots) or normal
		   (with modifiers and aug. dots), and get its bounding box. */
		   
		if (aRest->subType<WHOLEMR_L_DUR) {
			DrawMBRest(doc, pContext, aRestL, xd, yd, appearance, dim, &rSub);
			if (*recalc)
				OffsetRect(&rSub, -pContext->paper.left, -pContext->paper.top);
		}
		else {
			Byte unmappedGlyph = glyph;									/* save for below */
			glyph = MapMusChar(doc->musFontInfoIndex, glyph);
			restxd += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace));
			restyd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace));

			xp = pContext->paper.left+d2p(xd);
			yp = pContext->paper.top+d2p(yd)+fudgeRestY;
			restxp = pContext->paper.left+d2p(restxd);
			restyp = pContext->paper.top+d2p(restyd)+fudgeRestY;

			if (appearance!=NOTHING_VIS) {
				MoveTo(restxp, restyp);									/* Position pen */
				DrawMChar(doc, glyph, appearance, dim);					/* Draw the rest */
	
				if (xledg) {											/*	Draw any pseudo-ledger line */
					MoveTo(xpLedg=xp-d2p(xledg),ypLedg=yp-d2p(yledg));
					Line(d2p(LedgerLen(lnSpace)+LedgerOtherLen(lnSpace)),0);
				}
				DrawModNR(doc, aRestL, xd, pContext);					/* Draw all modifiers */
				DrawAugDots(doc, aRestL, xd, ydNorm, pContext, False);
	
				/* If we're supposed to draw stemlets on beamed rests, do so. */
				
				if (aRest->beamed && REST_STEMLET_LEN>=0 && config.drawStemlets) {
					short stemSpace = MusFontStemSpaceWidthPixels(doc, doc->musFontInfoIndex, lnSpace);

					qStemLen = 2*NFLAGS(aRest->subType)-1+REST_STEMLET_LEN;
					dStemLen = lnSpace*qStemLen/4;
					MoveTo(xp, pContext->paper.top+d2p(dTop+aRest->ystem));
					stemDown = (aRest->ystem > aRest->yd);
					if (!stemDown) Move(stemSpace, 0);
					Line(0, d2p(stemDown? -dStemLen : dStemLen));		/* Draw from stem toward rest */
				}
			}

			if (*recalc) {
				rSub = charRectCache.charRect[glyph];
				/* FIXME: The following #if/else/endif is a mess: the commented-out code looks
					better, but it's been the way it is now for years, and I don't remember
					anything about this. Clean up some day. -Don B., 11/96 */
#if 1
				switch (unmappedGlyph) {
					case EIGHth:
					case SIXTEENth:
					case THIRTY2nd:
					case SIXTY4th:
					case ONETWENTY8th:
						InsetRect(&rSub, 0, 
							d2p(std2d(-2, pContext->staffHeight, pContext->staffLines)));
						break;
					default:
						break;
				}
#else
				restInset = restObjRTweak[lDur-1];
				InsetRect(&rSub, 0, 
					d2p(std2d(restInset, pContext->staffHeight, pContext->staffLines)));
#endif
				OffsetRect(&rSub, restxp-pContext->paper.left, restyp-pContext->paper.top);
			}
		}
		
		if (*recalc) {
			if (*drawn)
				UnionRect(&LinkOBJRECT(pL), &rSub, &LinkOBJRECT(pL));
			else {
				*drawn = True;
				LinkOBJRECT(pL) = rSub;
			}
		}
		ForeColor(blackColor);
		break;
		
	case toPostScript:
		if (appearance==NOTHING_VIS) break;
	
		if (appearance!=NO_VIS) {
			if (aRest->subType<WHOLEMR_L_DUR)
				DrawMBRest(doc, pContext, aRestL, xd, yd, appearance, False, &rSub);
			else {
				glyph = MapMusChar(doc->musFontInfoIndex, glyph);
				restxd += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace));
				restyd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace));
				PS_MusChar(doc, restxd, restyd, glyph, True, sizePercent);
				if (xledg)
					PS_LedgerLine(yd-yledg,xd-xledg,LedgerLen(lnSpace)+LedgerOtherLen(lnSpace));

				/*	Rest drawn: now add augmentation dots, if any */
				
				DrawAugDots(doc, aRestL, xd, ydNorm, pContext, False);
			}
		}
		
		DrawModNR(doc, aRestL, xd, pContext);						/* Draw all modifiers */
		break;
	}

	TextSize(oldTxSize);

PopLock(OBJheap);
PopLock(NOTEheap);
}


/* ----------------------------------------------------------------------- ShowNGRSync -- */
/* Draw a dotted vertical line from near the bottom to near the top of the objRect
of the given object, but only if it's tall enough to help clarify what's related to
what. Intended for Syncs and GRSyncs, but should work for any non-J_STRUC object. */

static void ShowNGRSync(Document *doc, LINK pL, CONTEXT context[])
{
	Rect r;
	short ypTop, ypBottom;
	DDIST dhalfLn;
	
	r = LinkOBJRECT(pL);
	
	/* Convert r to window coords */
	
	OffsetRect(&r,doc->currentPaper.left,doc->currentPaper.top);
	
	// It'd be nice to use a different color, but ForeColor() here does nothing. ??
	
	PenMode(patXor);
	PenPat(NGetQDGlobalsGray());
	dhalfLn = LNSPACE(&context[1])/2;
	ypTop = r.top+d2p(dhalfLn);
	ypBottom = r.bottom-d2p(dhalfLn);
	if (ypBottom-ypTop>d2p(2*dhalfLn)) {
		MoveTo(r.left+2, ypTop);
		LineTo(r.left+2, ypBottom);
	}
	PenNormal();													/* Restore normal drawing */
}


/* -------------------------------------------------------------------------- DrawSYNC -- */
/* Draw a SYNC object, i.e., all of its notes and rests with their augmentation dots,
modifiers, and accidentals. */

void DrawSYNC(Document *doc, LINK pL, CONTEXT context[])
{
	PANOTE		aNote;
	LINK		aNoteL;
	PCONTEXT	pContext;
	Boolean		drawn;				/* False until a subobject has been drawn */
	Boolean		recalc;				/* True if we need to recalc enclosing rectangle */
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};
	
	drawn = False;
	recalc = (!LinkVALID(pL) && outputTo==toScreen);
	
	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->visible) {
			pContext = &context[NoteSTAFF(aNoteL)];
			MaySetPSMusSize(doc, pContext);
			if (NoteREST(aNoteL))
				DrawRest(doc, pL, pContext, aNoteL, &drawn, &recalc);
			else 
				DrawNote(doc, pL, pContext, aNoteL, &drawn, &recalc);
		}
	}

	if (recalc) LinkVALID(pL) = True;
	if (LinkSEL(pL))
		if (doc->showFormat)
			CheckSYNC(doc, pL, context, NULL, SMFind, stfRange, enlarge, enlarge);
		else
#ifdef USE_HILITESCORERANGE
			CheckSYNC(doc, pL, context, NULL, SMHilite, stfRange, enlarge, enlarge);
#else
			;
#endif
	if (doc->showSyncs) ShowNGRSync(doc, pL, context);
}


/* ------------------------------------------------------------------------- DrawGRAcc -- */
/* Draw accidental, if any, for the given grace note. */

static void DrawGRAcc(Document *doc,
						PCONTEXT pContext,
						LINK theGRNoteL,	/* Subobject (grace note) to draw accidental for */
						DDIST xd, DDIST yd,	/* Notehead position, including effect of <otherStemSide> */
						Boolean dim,		/* Ignored if outputTo==toPostScript */
						short sizePercent	/* Of normal note size */
						)
{
	PAGRNOTE theGRNote;
	DDIST accXOffset, d8thSp, xdAcc, lnSpace;
	DDIST xdLParen=0, xdRParen=0, ydParens=0;
	short xmoveAcc, xp, yp, delta, scalePercent=0;
	Byte accGlyph, lParenGlyph, rParenGlyph;

	theGRNote = GetPAGRNOTE(theGRNoteL);
	if (theGRNote->accident==0) return;

	accGlyph = MapMusChar(doc->musFontInfoIndex, SonataAcc[theGRNote->accident]);

	lnSpace = LNSPACE(pContext);
	d8thSp = LNSPACE(pContext)/8;

	/*
	 * Consider user-specified offset <xmoveAcc>. Also, double flat is wider than the
	 * other accidentals; just move it left a bit.
	 */
	xmoveAcc = (theGRNote->accident==AC_DBLFLAT?
						theGRNote->xmoveAcc+1 : theGRNote->xmoveAcc);
	accXOffset = SizePercentSCALE(AccXDOffset(xmoveAcc, pContext));

 	/* If it's a courtesy accidental, position the parentheses and move the accidental.	*/

 	xdAcc = xd-accXOffset;
	if (theGRNote->courtesyAcc) {
		DDIST xoffset, yoffset;

		/* FIXME: These adjustments are probably not quite right, due to combination of 
		   sizePercent, scalePercent, and the various config tweaks. */

		scalePercent = (short)(((long)config.courtesyAccPSize*sizePercent)/100L);

		lParenGlyph = MapMusChar(doc->musFontInfoIndex, MCH_lParen);
		rParenGlyph = MapMusChar(doc->musFontInfoIndex, MCH_rParen);

		xoffset = MusCharXOffset(doc->musFontInfoIndex, rParenGlyph, lnSpace);
		xdRParen = xdAcc + ((DDIST)(((long)scalePercent*xoffset)/100L));

		delta = (short)(((long)scalePercent*config.courtesyAccRXD)/100L);
		xdAcc = xdRParen-delta*d8thSp;

		delta = (short)(((long)scalePercent*config.courtesyAccLXD)/100L);
		xdLParen = xdAcc-delta*d8thSp;

		yoffset = MusCharYOffset(doc->musFontInfoIndex, rParenGlyph, lnSpace);	/* assuming both parens have same yoffset */
		delta = (short)(((long)scalePercent*config.courtesyAccYD)/100L);
		ydParens = yd + delta*d8thSp + ((DDIST)(((long)scalePercent*yoffset)/100L));;
	}
	xdAcc += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, accGlyph, lnSpace));
	yd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, accGlyph, lnSpace));

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = pContext->paper.left+d2p(xdAcc);
			yp = pContext->paper.top+d2p(yd);
			MoveTo(xp, yp);
			DrawMChar(doc, accGlyph, NORMAL_VIS, dim);

			/* If it's a courtesy accidental, draw parentheses around it. */
			if (theGRNote->courtesyAcc) {
				yp = pContext->paper.top+d2p(ydParens);
				xp = pContext->paper.left+d2p(xdLParen);
				MoveTo(xp, yp);
				DrawMChar(doc, lParenGlyph, NORMAL_VIS, dim);
				xp = pContext->paper.left+d2p(xdRParen);
				MoveTo(xp, yp);
				DrawMChar(doc, rParenGlyph, NORMAL_VIS, dim);
			}
			
			break;
		case toPostScript:
			PS_MusChar(doc, xdAcc, yd, accGlyph, True, sizePercent);

			/* If it's a courtesy accidental, draw parentheses around it. */
			
			if (theGRNote->courtesyAcc) {
				PS_MusChar(doc, xdLParen, ydParens, lParenGlyph, True, scalePercent);
				PS_MusChar(doc, xdRParen, ydParens, rParenGlyph, True, scalePercent);
			}
			
			break;
	}
}

/* ------------------------------------------------------------------- DrawGRNCLedgers -- */
/* Draw any ledger lines needed: if grace note is in a chord, for the chord's
extreme notes on both sides of the staff; if it's not in a chord, just for itself.
Should be called only once per chord. Assumes the GRNOTE and OBJECT heaps are locked! */

static void DrawGRNCLedgers(LINK syncL, PCONTEXT pContext, LINK aGRNoteL, DDIST xd,
										DDIST dTop, short ledgerSizePct)
{
	LINK		bGRNoteL, mainGRNoteL;
	PAGRNOTE	aGRNote, bGRNote;
	QDIST		yqpitRel,				/* y QDIST position relative to staff top */
				hiyqpit, lowyqpit;
	Boolean		stemDown;

	if (!pContext->showLedgers)
		return;

	mainGRNoteL = FindGRMainNote(syncL, GRNoteVOICE(aGRNoteL));
	stemDown = (GRNoteYSTEM(mainGRNoteL) > GRNoteYD(mainGRNoteL));
	
	aGRNote = GetPAGRNOTE(aGRNoteL);
	if (aGRNote->inChord) {
		hiyqpit = 9999;
		lowyqpit = -9999;
		bGRNoteL = FirstSubLINK(syncL);
		for ( ; bGRNoteL; bGRNoteL = NextGRNOTEL(bGRNoteL)) {
			bGRNote = GetPAGRNOTE(bGRNoteL);
			if (GRNoteVOICE(bGRNoteL)==GRNoteVOICE(aGRNoteL)) {
				if (bGRNote->yqpit<hiyqpit) hiyqpit = bGRNote->yqpit;
				if (bGRNote->yqpit>lowyqpit) lowyqpit = bGRNote->yqpit;
			}
		}
		yqpitRel = lowyqpit+
						halfLn2qd(ClefMiddleCHalfLn(pContext->clefType));
		DrawNoteLedgers(xd, yqpitRel, 0, stemDown, dTop, pContext, ledgerSizePct);
		yqpitRel = hiyqpit+
						halfLn2qd(ClefMiddleCHalfLn(pContext->clefType));
		DrawNoteLedgers(xd, yqpitRel, 0, stemDown, dTop, pContext, ledgerSizePct);
	}
	else if (aGRNote->yd<0 || aGRNote->yd>pContext->staffHeight) {
		yqpitRel = aGRNote->yqpit+
						halfLn2qd(ClefMiddleCHalfLn(pContext->clefType));
		DrawNoteLedgers(xd, yqpitRel, 0, stemDown, dTop, pContext, ledgerSizePct);
	}
}


/* ------------------------------------------------------------------------ DrawGRNote -- */

void DrawGRNote(Document *doc,
						LINK pL,			/* GRSYNC grace note belongs to */
						PCONTEXT pContext,	/* current context[] entry */
						LINK aGRNoteL,		/* Subobject (grace note) to draw */
						Boolean	*drawn,		/* False until a subobject has been drawn */
						Boolean	*recalc)	/* True if we need to recalc enclosing rectangle */
{
	PAGRNOTE	aGRNote;
	short		flagCount,			/* number of flags on grace note */
				staffn,
				voice;
	short		xhead, yhead,		/* pixel coordinates */
				appearance,
				ypStem, yp,
				octaveLength,
				spatialLen,
				rDiam,				/* rounded corner diameter for pianoroll */
				useTxSize,
				oldTxSize,			/* To restore port after small or magnified notes drawn */
				sizePercent,		/* Percent of "normal" (non-grace) note size to draw in */
				headRelSize,
				headSizePct,
				ledgerSizePct,		/* Percent of "normal" size for ledger lines */
				stemShorten,
				flagLeading,
				xadjhead, yadjhead;
	unsigned char glyph,			/* notehead symbol actually drawn */
				flagGlyph;
	DDIST		xd, yd,				/* scratch */
				dStemLen,
				xdStem2Slash,
				xdSlash, ydSlash,
				slashLen, slYDelta,
				xdNorm,
				dTop, dLeft,		/* abs DDIST origin */
				lnSpace, dhalfLn,	/* Distance between staff lines and half of that */
				fudgeHeadY,			/* Correction for roundoff in screen font head shape */
				breveFudgeHeadY,	/* Correction for Sonata error in breve origin */
				dGrSlashThick,
				offset;
	QDIST		qdLen;
	Boolean		stemDown,			/* Does grace note have a down-turned stem? */
				dim,				/* Should it be dimmed bcs in a voice not being looked at? */
				slashStem;
	Rect		spatialRect,
				rSub;				/* bounding box for subobject */
	long		resFact;
	Point		pt;
	LINK		beamL;

PushLock(OBJheap);
PushLock(GRNOTEheap);
	staffn = GRNoteSTAFF(aGRNoteL);
	dLeft = pContext->measureLeft + LinkXD(pL);					/* abs. origin of object */
	dTop = pContext->measureTop;

	lnSpace = LNSPACE(pContext);
	dhalfLn = lnSpace/2;

	xd = GRNoteXLoc(pL, aGRNoteL, pContext->measureLeft, HeadWidth(lnSpace), &xdNorm);
	
	aGRNote = GetPAGRNOTE(aGRNoteL);
	yd = dTop + aGRNote->yd;
	if (aGRNote->subType==BREVE_L_DUR)	breveFudgeHeadY = dhalfLn*BREVEYOFFSET;
	else								breveFudgeHeadY = (DDIST)0;

	appearance = aGRNote->headShape;
	if ((appearance==NO_VIS || appearance==NOTHING_VIS) && doc->showInvis)
		appearance = NORMAL_VIS;
	glyph = GetNoteheadInfo(appearance, aGRNote->subType, &headRelSize, &stemShorten);
	glyph = MapMusChar(doc->musFontInfoIndex, glyph);
	
	sizePercent = (aGRNote->small? SMALLSIZE(GRACESIZE(100)) : GRACESIZE(100));
	oldTxSize = GetPortTxSize();
	useTxSize = UseMTextSize(SizePercentSCALE(pContext->fontSize), doc->magnify);

	if (GRMainNote(aGRNoteL)) {
		stemDown = (aGRNote->ystem > aGRNote->yd);
		xdStem2Slash = -3*dhalfLn/4;
		xdSlash = xd+(stemDown? 0 : SizePercentSCALE(HeadWidth(lnSpace)));
		xdSlash += xdStem2Slash;
		ydSlash = dTop+aGRNote->ystem+(stemDown? -2*dhalfLn : 7*dhalfLn/2);
		slashLen = 5*dhalfLn/2;
		slYDelta = GRNOTE_SLASH_YDELTA(slashLen);
	}

/* FIXME: INSTEAD OF <WIDEHEAD> & <IS_WIDEREST>, WE SHOULD USE SMTHG LIKE CharRect OF
glyph TO GET HEADWIDTH! THE SAME GOES FOR DrawMODNR AND DrawRest. */

	ledgerSizePct = (WIDEHEAD(aGRNote->subType)==2? 15*sizePercent/10 :
							(WIDEHEAD(aGRNote->subType)==1? 13*sizePercent/10 :
										sizePercent) );		

	/*	Suppress flags if the grace note is beamed. */
	
	flagCount = (aGRNote->beamed? 0 : NFLAGS(aGRNote->subType));

	slashStem = False;
	if (config.slashGraceStems!=0) {
		if (flagCount==1) slashStem = True;
		else {
			if (config.slashGraceStems>1 && aGRNote->beamed) {
				voice = GRNoteVOICE(aGRNoteL);
				beamL = LVSearch(pL, BEAMSETtype, voice, GO_LEFT, False);	/* Should always succeed */
				if (pL==FirstInBeam(beamL)) slashStem = True;
			}
		}
	}

	TextSize(useTxSize);

	dStemLen = aGRNote->ystem - aGRNote->yd;

	switch (outputTo) {
	case toScreen:
	case toBitmapPrint:
	case toPICT:
		/* See the comment on fine-tuning note Y-positions in DrawNote. */
		
		if (outputTo==toBitmapPrint && bestQualityPrint)
			fudgeHeadY = 0;												/* No fine Y-offset for notehead */
		else
			fudgeHeadY = GetYHeadFudge(useTxSize);						/* Get fine Y-offset for notehead */
		
		/* xhead,yhead is position of notehead; xadjhead,yadjhead is position of notehead 
		   with any offset applied (might be true if music font is not Sonata). */
			
		offset = MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
		if (offset) {
			xhead = pContext->paper.left + d2p(xd);
			xadjhead = pContext->paper.left + d2p(xd+offset);
		}
		else
			xhead = xadjhead = pContext->paper.left + d2p(xd);
		offset = MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
		if (offset) {
			yp = pContext->paper.top + d2p(yd);
			yhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
			yp = pContext->paper.top + d2p(yd+offset);
			yadjhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
		}
		else {
			yp = pContext->paper.top + d2p(yd);
			yhead = yadjhead = yp + fudgeHeadY + d2p(breveFudgeHeadY);
		}

		if (appearance==NOTHING_VIS) goto EndQDrawing;

		/* If grace note is not in voice being "looked" at, dim it */
		
		dim = (outputTo==toScreen && !LOOKING_AT(doc, aGRNote->voice));
		if (dim) PenPat(NGetQDGlobalsGray());
		ForeColor(Voice2Color(doc, aGRNote->voice));
		
		/* First draw any accidental symbol to left of grace note */

		DrawGRAcc(doc, pContext, aGRNoteL, xdNorm, yd, dim, sizePercent);

		/*
		 * If grace note is not a non-Main note in a chord, draw any ledger lines needed:
		 * for grace note or entire chord. A chord has only one GRMainNote, so this draws
		 * the ledger lines exactly once.
		 */
		if (GRMainNote(aGRNoteL))
			DrawGRNCLedgers(pL, pContext, aGRNoteL, xd, dTop, ledgerSizePct);
			
		if (doc->graphMode==GRAPHMODE_PIANOROLL) {
		/* HANDLE SPATIAL (PIANOROLL) NOTATION */

			MoveTo(xhead,yhead);										/* position to draw head */
			resFact = RESFACTOR*(long)doc->spacePercent;
			qdLen = IdealSpace(doc, aGRNote->playDur, resFact);
			spatialLen = d2p(qd2d(qdLen, pContext->staffHeight,
									pContext->staffLines));
			spatialLen = n_max(spatialLen, 4);							/* Insure long enuf to be visible */
			SetRect(&spatialRect, xhead, yhead-d2p(dhalfLn),
				xhead+spatialLen, yhead+d2p(dhalfLn));
			rDiam = UseMagnifiedSize(4, doc->magnify);
			if (dim) FillRoundRect(&spatialRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&spatialRect, rDiam, rDiam); 
		}
		else {
			MoveTo(xadjhead, yadjhead);									/* position to draw head */

			/* HANDLE UNKNOWN CMN DURATION or WHOLE/BREVE: Show notehead alone. */
		
			if (aGRNote->subType==UNKNOWN_L_DUR)
				DrawMChar(doc, glyph, appearance, dim);
			else if (aGRNote->subType<=WHOLE_L_DUR)
				DrawMChar(doc, glyph, appearance, dim);
				
			/* HANDLE STEMMED NOTE. If the grace note is in a chord and is stemless, skip
			   entire business of drawing the stem and flags. */
		   
			else {
				if (GRMainNote(aGRNoteL)) {
					short stemSpace;

					stemDown = (aGRNote->ystem > aGRNote->yd);
					if (stemDown)
						stemSpace = 0;
					else {
						stemSpace = MusFontStemSpaceWidthPixels(doc, doc->musFontInfoIndex, lnSpace);
						Move(stemSpace, 0);
					}
					Move(0, -1);
					Line(0, d2p(aGRNote->ystem-aGRNote->yd)+(stemDown? 0 : 1));		/* Draw stem */

	 				/* Normally, draw a slash across the stem. The Sonata slash char. is
	 				   useless, so we really do draw it. */
	 					
					if (slashStem) {
						GetPen(&pt);
						yp = pContext->paper.top+d2p(ydSlash);
						MoveTo(pt.h+d2p(xdStem2Slash), yp);
						Line(d2p(slashLen), -d2p(slYDelta));
					}

					if (flagCount) {											/* Draw any flags */
						ypStem = yhead+d2p(dStemLen);

						if (doc->musicFontNum==sonataFontNum) {
							/* The 8th and 16th note flag characters in Sonata have their origins set so,
							   in theory, they're positioned properly if drawn from the point where the note
							   head was drawn. Unfortunately, the vertical position depends on the stem
							   length, which Adobe incorrectly assumed was always one octave. The "extend
							   flag" characters have their vertical origins set right where they go. */

							MoveTo(xhead, ypStem);								/* Position x at head, y at stem end */
							octaveLength = d2p(7*SizePercentSCALE(dhalfLn));
							Move(0, (stemDown? -octaveLength : octaveLength));	/* Adjust for flag origin */
							if (flagCount==1)									/* Draw one (8th note) flag */
								DrawMChar(doc, (stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp),
													NORMAL_VIS, dim);
							else if (flagCount>1) {								/* Draw >=2 (16th & other) flags */
								DrawMChar(doc, (stemDown? MCH_16thFlagDown : MCH_16thFlagUp),
													NORMAL_VIS, dim);
								MoveTo(xhead, ypStem);							/* Position x at head, y at stem end */
				
								/* FIXME: SCALE FACTORS FOR FlagLeading BELOW ARE GUESSWORK. */
								
								if (stemDown)
									Move(0, -d2p(13*FlagLeading(lnSpace)/4));
								else
									Move(0, d2p(7*FlagLeading(lnSpace)/2));
								while (flagCount-- > 2) {
									if (stemDown) {
										DrawMChar(doc, MCH_extendFlagDown, NORMAL_VIS, dim);
										Move(0, -d2p(FlagLeading(lnSpace))); /* FIXME: HUH? */
									}
									else {
										DrawMChar(doc, MCH_extendFlagUp, NORMAL_VIS, dim);
										Move(0, d2p(FlagLeading(lnSpace))); /* FIXME: HUH? */
									}
								}
							}
						}
						else {	/* fonts other than Sonata */
							short glyphWidth; DDIST xoff, yoff;

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
								DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
							}
							else if (flagCount==2 && MusFontHas16thFlag(doc->musFontInfoIndex)) {	/* Draw 16th flag using one flag char. */
								flagGlyph = MapMusChar(doc->musFontInfoIndex,
																(stemDown? MCH_16thFlagDown : MCH_16thFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								if (xoff || yoff) Move(d2p(xoff), d2p(yoff));
								DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
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
									DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
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
								DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
							}
						}
					}
				}
				MoveTo(xadjhead, yadjhead);
				DrawMChar(doc, glyph, appearance, dim);  					/* Finally, draw notehead */
			}
		}

		/* Restore full strength after possible dimming for "look" */
		
		PenPat(NGetQDGlobalsBlack());

EndQDrawing:
		if (*recalc) {
			rSub = charRectCache.charRect[glyph];
			OffsetRect(&rSub, xhead-pContext->paper.left, yhead-pContext->paper.top);
			if (*drawn)
				UnionRect(&LinkOBJRECT(pL), &rSub, &LinkOBJRECT(pL));
			else {
				*drawn = True;
				LinkOBJRECT(pL) = rSub;
			}
		}
		ForeColor(blackColor);
		break;
		
	case toPostScript:
	
		if (appearance==NOTHING_VIS) break;

		/* First draw any accidental symbol to left of grace note */
		 
		DrawGRAcc(doc, pContext, aGRNoteL, xdNorm, yd, False, sizePercent);

		/*
		 * If grace note is not a stemless note in a chord, draw any ledger lines needed
		 * for grace note or entire chord. All but one note of every chord has its stem
		 * length set to 0, so this draws the ledger lines exactly once.
		 */
		if (GRMainNote(aGRNoteL))
			DrawGRNCLedgers(pL, pContext, aGRNoteL, xd, dTop, ledgerSizePct);
	
	/* Now draw the grace note head, stem, and flags */
		
		if (doc->graphMode==GRAPHMODE_PIANOROLL)
	/* HANDLE SPATIAL (PIANOROLL) NOTATION */
		{
			;						/* FIXME: TO BE IMPLEMENTED */
		}
		else
		{
			DDIST xoff, yoff;

			headSizePct = headRelSize*sizePercent/100L;

			xoff = MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
			yoff = MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

			/*
			 *	UNKNOWN DURATION, WHOLE NOTE, OR BREVE
			 */
			if (aGRNote->subType==UNKNOWN_L_DUR)
				PS_MusChar(doc, xd+xoff, yd+yoff, glyph, appearance!=NO_VIS, headSizePct);
			else if (aGRNote->subType<=WHOLE_L_DUR)
				PS_MusChar(doc, xd+xoff, yd+yoff+breveFudgeHeadY,
							glyph, appearance!=NO_VIS, headSizePct);
			
			else {
			
				/*
				 *	STEMMED NOTE. If the note is in a chord and is stemless, skip
				 *	entire business of drawing the stem and flags.
				 */
				 
				if (GRMainNote(aGRNoteL)) {
					stemDown = (dStemLen>0);
					PS_NoteStem(doc, xd+xoff, yd+yoff, xd+xoff, glyph, dStemLen, (DDIST)0,
										aGRNote->beamed, appearance!=NO_VIS, headSizePct);
					if (flagCount) {
						if (doc->musicFontNum==sonataFontNum) {
							/*
							 *	The 8th and 16th note flag characters in Sonata have their origins set so,
							 *	in theory, they're positioned properly if drawn from the point where the note
							 *	head was drawn. Unfortunately, the vertical position depends on the stem
							 *	length, which Adobe incorrectly assumed was always one octave. The "extend
							 *	flag" characters have their vertical origins set right where they go.
							 */
							octaveLength = 7*SizePercentSCALE(dhalfLn);
							ypStem = yd + dStemLen + (stemDown ? -octaveLength : octaveLength);
			
							switch (flagCount) {
								case 1:
									PS_MusChar(doc, xd, ypStem, stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp,
												True, sizePercent);
									break;
								case 2:
									PS_MusChar(doc, xd, ypStem, stemDown? MCH_16thFlagDown : MCH_16thFlagUp, True, sizePercent);
									break;
								default:
									if (stemDown) {
										ypStem = yd + dStemLen;
										while (flagCount-- > 0) {
											PS_MusChar(doc, xd, ypStem, MCH_extendFlagDown, True, sizePercent);
											ypStem -= FlagLeading(lnSpace);
											}
										}
									 else {
										ypStem = yd + dStemLen;
										while (flagCount-- > 0) {
											PS_MusChar(doc, xd, ypStem, MCH_extendFlagUp, True, sizePercent);
											ypStem += FlagLeading(lnSpace);
											}
										}
									break;
							}
						}
						else {	/* fonts other than Sonata */
							DDIST dHeadWidth = 0;
							DDIST ydStem = yd + dStemLen;

							if (!stemDown && !MusFontUpstemFlagsHaveXOffset(doc->musFontInfoIndex))
								dHeadWidth = SizePercentSCALE(MusFontStemSpaceWidthDDIST(doc->musFontInfoIndex, lnSpace));

							if (flagCount==1) {												/* Draw 8th flag. */
								flagGlyph = MapMusChar(doc->musFontInfoIndex, 
																	(stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								PS_MusChar(doc, xd+xoff+dHeadWidth, ydStem+yoff, flagGlyph, True, sizePercent);
							}
							else if (flagCount==2 && MusFontHas16thFlag(doc->musFontInfoIndex)) {	/* Draw 16th flag using one flag char. */
								flagGlyph = MapMusChar(doc->musFontInfoIndex, 
																	(stemDown? MCH_16thFlagDown : MCH_16thFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								PS_MusChar(doc, xd+xoff+dHeadWidth, ydStem+yoff, flagGlyph, True, sizePercent);
							}
							else {
								DDIST ypos;
								short count = flagCount;

								/* Draw extension flag(s) */
								
								if (stemDown) {
									flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_extendFlagDown);
									flagLeading = -DownstemExtFlagLeading(doc->musFontInfoIndex, lnSpace);
								}
								else {
									flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_extendFlagUp);
									flagLeading = UpstemExtFlagLeading(doc->musFontInfoIndex, lnSpace);
								}
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								ypos = ydStem;
								while (count-- > 1) {
									PS_MusChar(doc, xd+xoff+dHeadWidth, ypos+yoff, flagGlyph, True, sizePercent);
									ypos += flagLeading;
								}

								/* Draw 8th flag */
								ypos = ydStem + flagLeading*(flagCount-2);
								if (stemDown) {
									flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_eighthFlagDown);
									flagLeading = -Downstem8thFlagLeading(doc->musFontInfoIndex, lnSpace);
								}
								else {
									flagGlyph = MapMusChar(doc->musFontInfoIndex, MCH_eighthFlagUp);
									flagLeading = Upstem8thFlagLeading(doc->musFontInfoIndex, lnSpace);
								}
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								PS_MusChar(doc, xd+xoff+dHeadWidth, ypos+yoff+flagLeading, flagGlyph, True, sizePercent);
							}
						}
					}

 					/* Normally, draw a slash across the stem. */
					
					if (slashStem) {
						dGrSlashThick = ((config.graceSlashLW*lnSpace)/100L);
						PS_Line(xdSlash, ydSlash, xdSlash+slashLen, ydSlash-slYDelta,
									dGrSlashThick);
					}

				}
				else {	/* Grace note in chord and doesn't have stem */
					PS_NoteStem(doc, xd+xoff, yd+yoff, xd+xoff, glyph, (DDIST)0, (DDIST)0, aGRNote->beamed,
										appearance!=NO_VIS, headSizePct);
				}
			}
		/*
		 *	Note drawn: if modifiers are allowed on grace notes, draw them here.
		 */
		}
		
		break;
	}
	TextSize(oldTxSize);
	
PopLock(OBJheap);
PopLock(GRNOTEheap);
}


/* ------------------------------------------------------------------------ DrawGRSYNC -- */
/* Draw a GRSYNC object, i.e., all of its grace notes with their augmentation dots and
accidentals. */

void DrawGRSYNC(Document *doc, LINK pL, CONTEXT context[])
{
	PAGRNOTE	aGRNote;
	LINK		aGRNoteL;
	PCONTEXT	pContext;
	Boolean		drawn;			/* False until a subobject has been drawn */
	Boolean		recalc;			/* True if we need to recalc enclosing rectangle */
	
	drawn = False;
	recalc = (!LinkVALID(pL) && outputTo==toScreen);
	
	aGRNoteL = FirstSubLINK(pL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (aGRNote->visible) {
			pContext = &context[GRNoteSTAFF(aGRNoteL)];
			MaySetPSMusSize(doc, pContext);
			DrawGRNote(doc, pL, pContext, aGRNoteL, &drawn, &recalc);
		}
	}

	if (recalc) LinkVALID(pL) = True;
#ifdef USE_HILITESCORERANGE
	if (LinkSEL(pL)) CheckGRSYNC(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
#endif
	if (doc->showSyncs) ShowNGRSync(doc, pL, context);
}

