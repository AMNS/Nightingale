/***************************************************************************
*	FILE:	DrawNRGR.c																			*
*	PROJ:	Nightingale, rev. for v.2000													*
*	DESC:	Routines to draw notes, rests, grace notes, and modifers				*	
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

static void DrawSlashes(DDIST, DDIST, INT16, Boolean, Boolean, CONTEXT *, Boolean);
static void DrawAugDots(Document *, LINK, DDIST, DDIST, PCONTEXT, Boolean);
static void DrawNCLedgers(LINK, PCONTEXT, LINK, DDIST, DDIST, INT16);
static void DrawNotehead(Document *, unsigned char, Byte, Boolean, DDIST);
static void GetNoteheadRect(unsigned char, Byte, DDIST, Rect *);
static void DrawMBRest(Document *, PCONTEXT, LINK, DDIST, DDIST, INT16, Boolean, Rect *);
static void ShowNGRSync(Document *, LINK, CONTEXT []);
static void DrawGRAcc(Document *, PCONTEXT, LINK, DDIST, DDIST, Boolean, INT16);
static void DrawGRNCLedgers(LINK, PCONTEXT, LINK, DDIST, DDIST, INT16);

static void DrawNChar(char glyph);
static void DrawNString(char glyph);

static void DrawNChar(char glyph)
{
#ifdef TARGET_API_MAC_CARBON
	short fontNum = GetPortTxFont();
	TextFont(systemFont);
	DrawChar(glyph);
	TextFont(fontNum);
#else
	DrawChar(glyph);
#endif
}

static void DrawNString(Str255 str)
{
#ifdef TARGET_API_MAC_CARBON
	short fontNum = GetPortTxFont();
	TextFont(systemFont);
	DrawString(str);
	TextFont(fontNum);
#else
	DrawString(str);
#endif
}

#ifdef TARGET_API_MAC_CARBON
//#define DrawChar DrawNChar
//#define DrawString DrawNString
#endif


/* ----------------------------------------------------------------- DrawSlashes -- */
/* Draw the specified number of tremolo slashes at the given position. */

void DrawSlashes(DDIST xdh, DDIST ydh,
					INT16 nslashes,
					Boolean noteCenter,		/* TRUE=horizontally center at note center, else at stem */
					Boolean stemUp,
					PCONTEXT pContext,
					Boolean dim
					)
{
	DDIST slashLeading,slashWidth,slashHeight,dxpos,dypos,slashThick;
	DDIST lnSpace, dEighthLn;
	INT16 xp, yp;
	INT16 i,xslash,yslash,slashLeadingp,dxposp,dyposp;
	
	lnSpace = LNSPACE(pContext);
	dEighthLn = LNSPACE(pContext)/8;

	slashLeading = (stemUp? 6*dEighthLn : -6*dEighthLn);
	slashWidth = 10*dEighthLn;
	slashHeight = 6*dEighthLn;
	if (noteCenter) dxpos = 0;
	else				 dxpos = (stemUp? 3*dEighthLn : -5*dEighthLn);
	dypos = (stemUp? 8*dEighthLn : -8*dEighthLn);
	
	slashThick = (long)(config.tremSlashLW*lnSpace) / 100L;

	switch (outputTo) {
		case toScreen:
		case toImageWriter:
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


/* ------------------------------------------------------------------ Draw1ModNR -- */

void Draw1ModNR(Document *doc, DDIST xdh, DDIST ydMod, INT16 code, unsigned char glyph,
						CONTEXT *pContext, INT16 sizePercent, Boolean dim)
{
	DDIST yd;
	INT16 oldTxSize, useTxSize, xHead, yMod;
	
	switch (outputTo) {
		case toScreen:
		case toImageWriter:
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
					if (dim) DrawMChar(doc, glyph, NORMAL_VIS, TRUE);
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
					if (dim) DrawMChar(doc, glyph, NORMAL_VIS, TRUE);
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
					PS_MusChar(doc, xdh, yd, glyph, TRUE, sizePercent);
					break;
				default:
					;
			}
			break;
	}
}


/* ------------------------------------------------------------------- DrawModNR -- */
/* Draw all of <aNoteL>'s note/rest modifiers. Assumes the NOTEheap is locked;
does not assume the MODNRheap is. */

void DrawModNR(Document *doc,
						LINK aNoteL,		/* Subobject (may be note or rest) */
						DDIST xd,			/* Note/rest position */
						CONTEXT	*pContext)
{
	PANOTE	aNote;
	LINK		aModNRL;
	PAMODNR	aModNR;
	DDIST		xdMod, ydMod, ydh, lnSpace;
	unsigned char glyph;
	INT16		code, sizePercent, xOffset, yOffset;
	Boolean	dim, noteCenter, stemUp;
				
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
				MayErrMsg("DrawModNR: illegal MODNR code %ld for note link=%ld",
							(long)code, (long)aNoteL);
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
				noteCenter = (NoteType(aNoteL)<=WHOLE_L_DUR);
				stemUp = (NoteYD(aNoteL)>NoteYSTEM(aNoteL));
				DrawSlashes(xdMod, ydh, code-MOD_TREMOLO1+1, noteCenter, stemUp,
								pContext, dim);
			}
			else
				Draw1ModNR(doc, xdMod, ydMod, code, glyph, pContext, sizePercent, dim);
		}
	}
}


static INT16 noteHeadGraphWidth = 5;		/* Replace noteheads with tiny graphs? ??MUST BE GLOBAL; MOVE TO vars.h(?) */

/* ------------------------------------------------------------------- DrawAugDots -- */
/* Draw all the augmentation dots for the given note or rest. */

static DDIST AugDotXOffset(LINK theNoteL, DDIST yd, PCONTEXT pContext,
				Boolean chordNoteToR,  Boolean doNoteheadGraphs);

static DDIST AugDotXOffset(LINK theNoteL,			/* Subobject (note/rest) to draw dots for */
						   PCONTEXT pContext,
						   Boolean	chordNoteToR,	/* Note in a chord that's upstemmed w/notes to right of stem? */
						   Boolean doNoteheadGraphs
					 	   )
{
	PANOTE	theNote;
	DDIST 	xdStart, xdDots, dhalfLn;
	INT16		ndots;
	char		lDur;

	theNote = GetPANOTE(theNoteL);
	if (theNote->ndots==0 || theNote->ymovedots==0) return 0;	/* If no dots or dots invisible */

	/*
	 *	Ordinarily, the first dot is just to the right of a note on the "normal" side of
	 * the stem. But if note is in a chord that's upstemmed and has notes to the right
	 *	of the stem, its dots must be moved to the right.
	 */
	xdStart = 0;
	if (chordNoteToR) xdStart += HeadWidth(LNSPACE(pContext));

	dhalfLn = LNSPACE(pContext)/2;
	xdDots = xdStart+dhalfLn;
	
	if (theNote->rest) {
		if (theNote->subType==WHOLEMR_L_DUR) lDur = WHOLE_L_DUR;
		else									   	 lDur = theNote->subType;
		if (IS_WIDEREST(lDur)) xdDots += (theNote->small? dhalfLn/2 : dhalfLn);
	}
	else {
		if (!theNote->small) xdDots += dhalfLn/2;
		if (WIDEHEAD(theNote->subType)) xdDots += dhalfLn/2;
	}
		
	xdDots += std2d(STD_LINEHT*(theNote->xmovedots-3)/4,
							pContext->staffHeight,
							pContext->staffLines);

	/* If we're drawing notehead graphs, move aug. dots to the right by the appropriate factor.
	 * (At the moment, the graphs are a bit wider than expected, so move them a bit further.)
	 */
	if (noteHeadGraphWidth>0 && doNoteheadGraphs) 
	{
		xdDots = xdDots*noteHeadGraphWidth;
		// xdDots += 5*noteHeadGraphWidth;
		xdDots += (9*dhalfLn)/10;
	}

	return xdDots;
}

static void DrawAugDots(Document *doc,
						LINK theNoteL,				/* Subobject (note/rest) to draw dots for */
						DDIST xdNorm, DDIST yd,	/* Notehead/rest origin, excluding effect of <otherStemSide> */
						PCONTEXT pContext,
						Boolean	chordNoteToR	/* Note in a chord that's upstemmed w/notes to right of stem? */
						)
{
	PANOTE	theNote;
	DDIST 	xdDots, ydDots, dhalfLn;
	INT16		ndots, xpDots, yp;
	char		lDur;
	Boolean	dim;				/* Should it be dimmed bcs in a voice not being looked at? */
	Boolean doNoteheadGraphs;
	Byte		glyph = MapMusChar(doc->musFontInfoIndex, MCH_dot);

	theNote = GetPANOTE(theNoteL);
	if (theNote->ndots==0 || theNote->ymovedots==0) return;	/* If no dots or dots invisible */

	dhalfLn = LNSPACE(pContext)/2;
#ifdef VISUALIZE_PERF
	doNoteheadGraphs = doc->showDurProb;		//  ?? "showDurProb" IS A CRUDE TEMPORARY UI HACK!
	xdDots = xdNorm + AugDotXOffset(theNoteL, pContext, chordNoteToR, doNoteheadGraphs);
#else
	xdDots = xdNorm + AugDotXOffset(theNoteL, pContext, chordNoteToR, FALSE);
#endif

	ydDots = yd+(theNote->ymovedots-2)*dhalfLn;
	
	ndots = theNote->ndots;
	dim = (outputTo==toScreen && !LOOKING_AT(doc, theNote->voice));

	xdDots += MusCharXOffset(doc->musFontInfoIndex, glyph, dhalfLn*2);
	ydDots += MusCharYOffset(doc->musFontInfoIndex, glyph, dhalfLn*2);

	switch (outputTo) {
		case toScreen:
		case toImageWriter:
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
			if (doc->srastral==7) ydDots += pt2d(1)/8;						/* Tiny empirical correction */
			while (ndots>0) {
				xdDots += 2*dhalfLn;
				PS_MusChar(doc, xdDots, ydDots, glyph, TRUE, 100);
				ndots--;
			}
			break;
	}
}


/* ------------------------------------------------------------------- AccXOffset -- */

DDIST AccXOffset(INT16 xmoveAcc, PCONTEXT pContext)
{
	DDIST dAccWidth, xOffset;

	dAccWidth = std2d(STD_ACCWIDTH,
							 pContext->staffHeight,
							 pContext->staffLines);
	xOffset = dAccWidth;												/* Set offset to default */
	xOffset += (dAccWidth*(xmoveAcc-DFLT_XMOVEACC))/4;		/* Fine-tune it */
	return xOffset;
}


/* ---------------------------------------------------------------------- DrawAcc -- */
/* Draw an accidental, if there is one, for the given note. */

#define SCHAR_LPAREN '('		/* Sonata font character */
#define SCHAR_RPAREN ')'

void DrawAcc(Document *doc,
				PCONTEXT pContext,
				LINK theNoteL,					/* Subobject (note) to draw accidental for */
				DDIST xdNorm, DDIST yd,		/* Notehead position, excluding effect of <otherStemSide> */
				Boolean dim,					/* Ignored if outputTo==toPostScript */
				INT16 sizePercent,
				Boolean chordNoteToL			/* Note in a chord that's downstemmed w/notes to left of stem? */
				)
{
	PANOTE theNote;
	DDIST accXOffset, d8thSp, xdAcc, xdLParen, xdRParen, ydParens, lnSpace;
	INT16 xmoveAcc, xp, yp, delta, scalePercent;
	Byte accGlyph, lparenGlyph, rparenGlyph;

	theNote = GetPANOTE(theNoteL);
	if (theNote->accident==0) return;

	accGlyph = MapMusChar(doc->musFontInfoIndex, SonataAcc[theNote->accident]);

	lnSpace = LNSPACE(pContext);
	d8thSp = LNSPACE(pContext)/8;

	/*
	 *	Ordinarily, the accidental position is relative to a note on the "normal" side of
	 * the stem. But if note is in a chord that's downstemmed and has notes to the left
	 *	of the stem, its accidental must be moved to the left.
	 */
	if (chordNoteToL) xdNorm -= SizePercentSCALE(HeadWidth(LNSPACE(pContext)));

	/*
	 * Consider user-specified offset <xmoveAcc>. Also, double flat is wider than the
	 * other accidentals; just move it left a bit.
	 */
	xmoveAcc = (theNote->accident==AC_DBLFLAT? theNote->xmoveAcc+2 : theNote->xmoveAcc);
	accXOffset = SizePercentSCALE(AccXOffset(xmoveAcc, pContext));

 	/* If it's a courtesy accidental, position the parentheses and move the accidental.	*/

 	xdAcc = xdNorm-accXOffset;

	if (theNote->courtesyAcc) {
		DDIST xoffset, yoffset;

		/* ??These adjustments are probably not quite right, due to combination of 
				sizePercent, scalePercent, and the various config tweaks. */

		scalePercent = (INT16)(((long)config.courtesyAccSize*sizePercent)/100L);

		lparenGlyph = MapMusChar(doc->musFontInfoIndex, SCHAR_LPAREN);
		rparenGlyph = MapMusChar(doc->musFontInfoIndex, SCHAR_RPAREN);

		xoffset = MusCharXOffset(doc->musFontInfoIndex, rparenGlyph, lnSpace);
		xdRParen = xdAcc + ((DDIST)(((long)scalePercent*xoffset)/100L));

		delta = (INT16)(((long)scalePercent*config.courtesyAccRXD)/100L);
		xdAcc = xdRParen-delta*d8thSp;

		delta = (INT16)(((long)scalePercent*config.courtesyAccLXD)/100L);
		xdLParen = xdAcc-delta*d8thSp;

		yoffset = MusCharYOffset(doc->musFontInfoIndex, rparenGlyph, lnSpace);	/* assuming both parens have same yoffset */
		delta = (INT16)(((long)scalePercent*config.courtesyAccYD)/100L);
		ydParens = yd + delta*d8thSp + ((DDIST)(((long)scalePercent*yoffset)/100L));;
	}
	xdAcc += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, accGlyph, lnSpace));
	yd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, accGlyph, lnSpace));

	switch (outputTo) {
		case toScreen:
		case toImageWriter:
		case toPICT:
			xp = pContext->paper.left+d2p(xdAcc);
			yp = pContext->paper.top+d2p(yd);
			MoveTo(xp, yp);
			DrawMChar(doc, accGlyph, NORMAL_VIS, dim);

			/* If it's a courtesy accidental, draw parentheses around it. */
			if (theNote->courtesyAcc) {
				yp = pContext->paper.top+d2p(ydParens);
				xp = pContext->paper.left+d2p(xdLParen);
				MoveTo(xp, yp);
				DrawMChar(doc, lparenGlyph, NORMAL_VIS, dim);
				xp = pContext->paper.left+d2p(xdRParen);
				MoveTo(xp, yp);
				DrawMChar(doc, rparenGlyph, NORMAL_VIS, dim);
			}

			break;
		case toPostScript:
			PS_MusChar(doc, xdAcc, yd, accGlyph, TRUE, sizePercent);

			/* If it's a courtesy accidental, draw parentheses around it. */
			if (theNote->courtesyAcc) {
				PS_MusChar(doc, xdLParen, ydParens, lparenGlyph, TRUE, scalePercent);
				PS_MusChar(doc, xdRParen, ydParens, rparenGlyph, TRUE, scalePercent);
			}

			break;
	}
}


/* ------------------------------------------------------------- GetNoteheadInfo -- */
/* Given a notehead's <appearance> and <subType>, return the music font <glyph> as
function value, plus <sizePct> and <stemShorten>. Note that if <sizePct> is very far
from 100, the calling routine may have to adjust horizontal positions of upstems. */

#define STEMSHORTEN_NOHEAD 6			/* in eighth-spaces */
#define STEMSHORTEN_XSHAPEHEAD 3		/* in eighth-spaces */

unsigned char GetNoteheadInfo(INT16 appearance, INT16 subType,
							INT16 *sizePct,				/* percentage of normal size */
							INT16 *stemShorten			/* in eighth-spaces */
							)
{
	unsigned char glyph;
	
	if (appearance==X_SHAPE) glyph = MCH_xShapeHead;
	else if (appearance==HARMONIC_SHAPE) glyph = MCH_harmonicHead;
	else if (appearance==SLASH_SHAPE) glyph = '\0';			/* Not in  font, must be drawn */
	else if (appearance==SQUAREH_SHAPE) glyph = MCH_squareHHead;
	else if (appearance==SQUAREF_SHAPE) glyph = MCH_squareFHead;
	else if (appearance==DIAMONDH_SHAPE) glyph = MCH_diamondHHead;
	else if (appearance==DIAMONDF_SHAPE) glyph = MCH_diamondFHead;
	else if (appearance==HALFNOTE_SHAPE) glyph = MCH_halfNoteHead;
	else {																/* Assume NORMAL_ or NO_VIS */
		if (subType==UNKNOWN_L_DUR)
			glyph = MCH_quarterNoteHead;
		else if (subType<=WHOLE_L_DUR)
			glyph = (unsigned char)MCH_notes[subType-1];
		else if (subType==HALF_L_DUR)
			glyph = MCH_halfNoteHead;
		else
			glyph = MCH_quarterNoteHead;
	}
	
	*sizePct = 100;
	/* ??Changing sizePct fixes the head size, but makes a mess of upstems on
		notes with harmonic heads. Sigh. */
//FIXME: Call a new function in MusicFont.c that tells us whether to enlarge the harmonic head for our music font?
	if (glyph==MCH_harmonicHead && !(CapsLockKeyDown() && ShiftKeyDown()))
		*sizePct =  115;

	*stemShorten = 0;
	if (glyph==MCH_xShapeHead) *stemShorten = STEMSHORTEN_XSHAPEHEAD;
	if (glyph=='\0') *stemShorten = STEMSHORTEN_NOHEAD;

	return glyph;
}


/* --------------------------------------------------------------- DrawNCLedgers -- */
/* Draw any ledger lines needed: if note is in a chord, for the chord's extreme
notes on both sides of the staff; if it's not in a chord, just for the note. Should
be called only once per chord, e.g., just for the MainNote. Assumes the NOTE and
OBJECT heaps are locked! */

static void DrawNCLedgers(
		LINK		syncL,
		PCONTEXT	pContext,
		LINK		aNoteL,
		DDIST		xd,
		DDIST		dTop,
		INT16		ledgerSizePct
		)
{
	LINK		bNoteL, mainNoteL;
	PANOTE	aNote, bNote;
	QDIST		yqpitRel, yqpitRelSus,		/* y QDIST positions relative to staff top */
				hiyqpit, lowyqpit,			/* "hi" is pitch, i.e., low y-coord. */
				hiyqpitSus, lowyqpitSus;
	Boolean	stemDown;

	if (!pContext->showLedgers)
		return;

	mainNoteL = FindMainNote(syncL, NoteVOICE(aNoteL));
	stemDown = (NoteYSTEM(mainNoteL) > NoteYD(mainNoteL));
	
	aNote = GetPANOTE(aNoteL);
	if (aNote->inChord) {
		hiyqpit = hiyqpitSus = 9999;
		lowyqpit = lowyqpitSus = -9999;
		bNoteL = FirstSubLINK(syncL);

		/*
		 * Find the chord's extreme notes above and below staff, on the normal side of
		 *	the stem and "suspended" (on the other side).
		 */
		for ( ; bNoteL; bNoteL = NextNOTEL(bNoteL)) {
			bNote = GetPANOTE(bNoteL);
			if (NoteVOICE(bNoteL)==NoteVOICE(aNoteL)) {
				if (bNote->yqpit<hiyqpit) hiyqpit = bNote->yqpit;
				if (bNote->yqpit<hiyqpitSus && bNote->otherStemSide) hiyqpitSus = bNote->yqpit;
				if (bNote->yqpit>lowyqpit) lowyqpit = bNote->yqpit;
				if (bNote->yqpit>lowyqpitSus && bNote->otherStemSide) lowyqpitSus = bNote->yqpit;
			}
		}
		
		/*
		 *	Convert to staff-rel. coords. and draw the ledger lines, first below the
		 *	staff, then above.
		 */
		yqpitRel = lowyqpit + halfLn2qd(ClefMiddleCHalfLn(pContext->clefType));
		yqpitRelSus = (lowyqpitSus==-9999? 0
							: lowyqpitSus + halfLn2qd(ClefMiddleCHalfLn(pContext->clefType)));
		NoteLedgers(xd, yqpitRel, yqpitRelSus, stemDown, dTop, pContext, ledgerSizePct);
		
		yqpitRel = hiyqpit + halfLn2qd(ClefMiddleCHalfLn(pContext->clefType));
		yqpitRelSus = (hiyqpitSus==9999? 0
							: hiyqpitSus + halfLn2qd(ClefMiddleCHalfLn(pContext->clefType)));
		NoteLedgers(xd, yqpitRel, yqpitRelSus, stemDown, dTop, pContext, ledgerSizePct);
	}
	
	else if (pContext->showLines!=SHOW_ALL_LINES ||
									aNote->yd<0 || aNote->yd>pContext->staffHeight) {
	
		/* Not a chord. Just draw ledger lines for the single note. */
		
		yqpitRel = aNote->yqpit + halfLn2qd(ClefMiddleCHalfLn(pContext->clefType));
		NoteLedgers(xd, yqpitRel, 0, stemDown, dTop, pContext, ledgerSizePct);
	}
}


/* --------------------------------------------------------------- DrawNotehead -- */
/* QuickDraw only */

void DrawNotehead(Document *doc,
					unsigned char glyph,
					Byte appearance,
					Boolean dim,				/* Should character be dimmed? */
					DDIST dhalfLn
					)
{
	DDIST thick;
	
	if (appearance==SLASH_SHAPE) {
		if (dim) PenPat(NGetQDGlobalsGray());
		/*
		 * The "slash notehead" is like a steeply-sloped beam, so steep we
		 * have to increase the vertical thickness so it doesn't look too thin.
		 * (Actually, it should have horizontal instead of vertical cutoffs.)
		 */
		thick = 3*dhalfLn/2;
		PenSize(1, d2p(thick));
		Move(0, d2p(2*dhalfLn-thick/2));
		Line(d2p(2*dhalfLn), -d2p(4*dhalfLn));
		PenNormal();
	}
	else
		DrawMChar(doc, glyph, appearance, dim);
}


void GetNoteheadRect(unsigned char glyph, Byte appearance, DDIST dhalfLn,
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


static void PaintRoundLeftRect(Rect *paRect, INT16 ovalWidth, INT16 ovalHeight);
void PaintRoundLeftRect(Rect *paRect, INT16 ovalWidth, INT16 ovalHeight)
{
	Rect		leftEndRect, otherRect;

	leftEndRect = *paRect;
	leftEndRect.right = leftEndRect.left+ovalWidth;
	otherRect = *paRect;
	otherRect.left = leftEndRect.left+ovalWidth/2;
	
	PaintRoundRect(&leftEndRect, ovalWidth, ovalHeight);
	PaintRect(&otherRect);
}


static void PaintRoundRightRect(Rect *paRect, INT16 ovalWidth, INT16 ovalHeight);
void PaintRoundRightRect(Rect *paRect, INT16 ovalWidth, INT16 ovalHeight)
{
	Rect		rightEndRect, otherRect;

	rightEndRect = *paRect;
	rightEndRect.left = rightEndRect.right-ovalWidth;
	otherRect = *paRect;
	otherRect.right = rightEndRect.right-ovalWidth/2;
	
	PaintRoundRect(&rightEndRect, ovalWidth, ovalHeight);
	PaintRect(&otherRect);
}


static void DrawNoteheadGraph(Document *, unsigned char, Byte, Boolean, DDIST);

/* QuickDraw only */
/* Draw a little graph as a notehead: intended to be used to visualize changes during the note.
This version draws a series of colored bars side by side.
??INSTEAD OF appearance, WHICH CAN'T BE MORE THAN 5 BITS (+ 8 IF I ALSO USE THE SPARE BYTE
PER NOTE), HOW ABOUT USING A NOTE MODIFIER TO CONTROL WHAT'S DRAWN?? I THINK AMODNR HAS A LOT
MORE THAN 13 BITS AVAILABLE, & IT SHOULD BE EASY TO GET THE INFO IN VIA NOTELIST! E.G., modCode =
60 => draw graph, OR 61:80 => draw graph OF 1:20 SEGMENTS. */

void DrawNoteheadGraph(Document *doc,
					unsigned char glyph,
					Byte appearance,
					Boolean dim,			/* Should notehead be dimmed? */
					DDIST dhalfLn,
					LINK aNoteL,				
					PCONTEXT pContext		/* Current context[] entry */
					)
{
	PANOTE		aNote;
	INT16		graphLen,
				nSegs,
				rDiam,					/* rounded corner diameter */
				xorg, yorg,
				yTop, yBottom;
	QDIST		qdLen;
	Rect		graphRect, segRect,
				rSub;					/* bounding box for subobject */
	long		resFact;
	Point		pt;
	INT16		segColor[50];

	aNote = GetPANOTE(aNoteL);
	resFact = RESFACTOR*(long)doc->spacePercent;
#if 1
	qdLen = noteHeadGraphWidth*4;
#else
	qdLen = IdealSpace(doc, aNote->playDur, resFact);
	qdLen = (long)(config.pianorollLenFact*qdLen) / 100L;
	qdLen = n_max(qdLen, 4);								/* Insure at least one space wide */
#endif
	graphLen = d2p(qd2d(qdLen, pContext->staffHeight,
							pContext->staffLines));

	GetPen(&pt);
//		LineTo(pt.h+12, pt.v+12);
	xorg = pt.h;
	yorg = pt.v;
//		SetRect(&graphRect, xorg, yorg-20, xorg+80, yorg+10);  // ???TESTING
//		PaintRoundRect(&graphRect, 4, 4);								  // ???TESTING
	yTop = yorg-d2p(dhalfLn);
	yBottom = yorg+d2p(dhalfLn);
	SetRect(&graphRect, xorg, yorg-d2p(dhalfLn),
		xorg+graphLen, yorg+d2p(dhalfLn));
	rDiam = UseMagnifiedSize(4, doc->magnify);
#if 1
	nSegs = 1;									// ??TEMP
	switch (appearance) 
	{
		case 1:
			segColor[1] = blueColor;					// ???TEMP
			break;
		case 2:
			segColor[1] = yellowColor;					// ???TEMP
			break;
		case 3:
			nSegs = 2;									// ??TEMP
			segColor[1] = magentaColor;
			segColor[2] = blueColor;
			break;
		case 4:
			segColor[1] = magentaColor;					// ???TEMP
		case 5:
			nSegs = 3;									// ??TEMP
			segColor[1] = magentaColor;
			segColor[2] = yellowColor;
			segColor[3] = blueColor;
			break;

		default:
			;
	}

	switch (nSegs) 
	{
		case 1:
			ForeColor(segColor[1]);
			if (dim) FillRoundRect(&graphRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&graphRect, rDiam, rDiam); 
			break;
		case 2:
			ForeColor(segColor[1]);
			segRect = graphRect;
			segRect.right = segRect.left+graphLen/2;
			if (dim) FillRoundRect(&segRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundLeftRect(&segRect, rDiam, rDiam);

			ForeColor(segColor[2]);
			segRect = graphRect;
			segRect.left = segRect.right-graphLen/2;
			if (dim) FillRoundRect(&segRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRightRect(&segRect, rDiam, rDiam);
			break;
		case 3:
			ForeColor(segColor[1]);
			segRect = graphRect;
			segRect.right = segRect.left+graphLen/3;
			if (dim) FillRoundRect(&segRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundLeftRect(&segRect, rDiam, rDiam);

			ForeColor(segColor[2]);
			segRect = graphRect;
			segRect.left += graphLen/3;
			segRect.right -= graphLen/3;
			if (dim) FillRect(&segRect, NGetQDGlobalsGray());
			else		PaintRect(&segRect);

			ForeColor(segColor[3]);
			segRect = graphRect;
			segRect.left = segRect.right-graphLen/3;
			if (dim) FillRoundRect(&segRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRightRect(&segRect, rDiam, rDiam);
			break;
		
		default:
			if (dim) FillRoundRect(&graphRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&graphRect, rDiam, rDiam);		
	}
	ForeColor(Voice2Color(doc, aNote->voice));				// ??TEMP

#if 0
	SetRect(&graphRect, xorg, yorg-14*d2p(dhalfLn),
		xorg+1.5*graphLen, yorg-11*d2p(dhalfLn));				// ?? VERY TEMP! TEST PaintRoundLeftRect !!!!
	if ((appearance & 0x1)==0) PaintRoundLeftRect(&graphRect, 2*rDiam, 2*rDiam);
	else PaintRoundRightRect(&graphRect, 2*rDiam, 2*rDiam);
#endif

#else
	switch (appearance) 
	{
		case 1:
			ForeColor(blueColor);									// ???TEMP
			if (dim) FillRoundRect(&graphRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&graphRect, rDiam, rDiam); 
			break;
		case 2:
			ForeColor(yellowColor);									// ???TEMP
			if (dim) FillRoundRect(&graphRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&graphRect, rDiam, rDiam); 
			break;
		case 3:
			segRect = graphRect;
			segRect.right = segRect.left+graphLen/2;
			ForeColor(magentaColor);								// ???TEMP
			if (dim) FillRoundRect(&segRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&segRect, rDiam, rDiam); 
			segRect = graphRect;
			segRect.left = segRect.right-graphLen/2;
			ForeColor(blueColor);								// ???TEMP
			if (dim) FillRoundRect(&segRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&segRect, rDiam, rDiam); 
			break;
		default:
			if (dim) FillRoundRect(&graphRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&graphRect, rDiam, rDiam); 
			;
	}
	ForeColor(Voice2Color(doc, aNote->voice));				// ??TEMP
#endif
}

/* ---------------------------------------------------------------------- DrawNote -- */

#define STEMWIDTH (p2d(1))			/* QuickDraw stem thickness. ??Make a macro w/staff ht as arg.? */

/* Adjust <width> for given WIDEHEAD code. NB: watch out for integer overflow! */
#define WIDEHEAD_VALUE(whCode, width) (whCode==2? 160*(width)/100 : \
													(whCode==1? 135*(width)/100 : (width)) )

/* If this shape of notehead is wider or narrow than normal and note is upstemmed,
move it left or right so its right edge touches the stem. */

#define XD_GLYPH_ADJ(stemDn, headRSize)		\
	(!stemDn? ((headRSize-100L)*HeadWidth(lnSpace))/100L : 0L)

void DrawNote(Document *doc,
				LINK pL,						/* Sync note belongs to */
				PCONTEXT pContext,		/* Current context[] entry */
				LINK		aNoteL,			/* Subobject (note) to draw */
				Boolean	*drawn,			/* FALSE until a subobject has been drawn */
				Boolean	*recalc			/* TRUE if we need to recalc enclosing rectangle */
				)	
{
	PANOTE	aNote;
	PANOTE	bNote;
	LINK		bNoteL;
	INT16		flagCount,
				staffn, ypStem,
				xhead, yhead,	/* pixel coordinates */
				appearance,
				octaveLength,
				spatialLen,
				rDiam,			/* rounded corner diameter for pianoroll */
				useTxSize,
				oldTxSize,		/* To restore port after small or magnified notes drawn */
				sizePercent,	/* Percent of "normal" size to draw in (for small notes) */
				headRelSize,
				headSizePct,
				ledgerSizePct,	/* Percent of "normal" size for ledger lines */
				yp, noteType,
				stemShorten,
				flagLeading,
				xadjhead, yadjhead;
	unsigned char glyph,		/* notehead symbol actually drawn */
				flagGlyph;
	DDIST		xd, yd,
				xdAdj,
				dStemLen, dStemShorten,
				xdNorm,
				dTop, dLeft,	/* abs DDIST origin */
				dhalfLn,			/* Distance between staff half-lines */
				fudgeHeadY,		/* Correction for roundoff in Sonata screen font head shape */
				breveFudgeHeadY,	/* Correction for error in Sonata breve origin (screen & PS) */
				offset, lnSpace;
	QDIST		qdLen;
	Boolean	stemDown,		/* Does note have a down-turned stem? */
				dim,				/* Should it be dimmed bcs in a voice not being looked at? */
				chordNoteToR, chordNoteToL;
	Rect		spatialRect,
				rSub;				/* bounding box for subobject */
	long		resFact;

PushLock(OBJheap);
PushLock(NOTEheap);
	staffn = NoteSTAFF(aNoteL);
	dLeft = pContext->measureLeft + LinkXD(pL);				/* abs. origin of object */
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

/* ??INSTEAD OF <WIDEHEAD> & <IS_WIDEREST>, WE SHOULD USE SMTHG LIKE CharRect OF glyph
TO GET HEADWIDTH! THE SAME GOES FOR DrawMODNR AND DrawRest. */
	ledgerSizePct = WIDEHEAD_VALUE(WIDEHEAD(noteType), sizePercent);

	/*	Suppress flags if the note is beamed. */
	flagCount = (aNote->beamed? 0 : NFLAGS(noteType));

	chordNoteToR = (!aNote->rest && ChordNoteToRight(pL, aNote->voice));
	chordNoteToL = (!aNote->rest && ChordNoteToLeft(pL, aNote->voice));
	
	dStemLen = aNote->ystem-aNote->yd;

	switch (outputTo) {
	case toScreen:
	case toImageWriter:
	case toPICT:
		/*
		 *	In every case but one, we need to fine-tune note Y-positions to compensate
		 *	for minor inconsistencies in the screen fonts. The exception is "Best Quality
		 *	Print" on the ImageWriter; here, we SHOULD still need fine-tuning, though
		 *	for twice the normal font size, since the ImageWriter driver automatically
		 *	uses twice the font size and spaces the dots half the normal distance apart.
		 *	However, for unknown reasons, best results are obtained with NO correction.
		 */ 
		if (outputTo==toImageWriter && bestQualityPrint)
			fudgeHeadY = 0;													/* No fine Y-offset for notehead */
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

		/* If note is not in voice being "looked" at, dim it */
		dim = (outputTo==toScreen && !LOOKING_AT(doc, aNote->voice));

		TextSize(useTxSize);
		ForeColor(Voice2Color(doc, aNote->voice));
		if (dim) PenPat(NGetQDGlobalsGray());
		
		/* First draw any accidental symbol to left of note */
		
		DrawAcc(doc, pContext, aNoteL, xdNorm, yd, dim, sizePercent, chordNoteToL);

		/*
		 *	If note is not a non-Main note in a chord, draw any ledger lines needed: for
		 * note or entire chord. A chord has only one MainNote, so this draws the ledger
		 * lines exactly once.
		 */
		if (MainNote(aNoteL))
			DrawNCLedgers(pL, pContext, aNoteL, xd, dTop, ledgerSizePct); 

				 
		if (doc->pianoroll)	{												/* Handle pianoRoll notation */
			MoveTo(xhead, yhead);												/* position to draw head */
			resFact = RESFACTOR*(long)doc->spacePercent;
			qdLen = IdealSpace(doc, aNote->playDur, resFact);
			qdLen = (long)(config.pianorollLenFact*qdLen) / 100L;
			qdLen = n_max(qdLen, 4);								/* Insure at least one space long */
			spatialLen = d2p(qd2d(qdLen, pContext->staffHeight,
									pContext->staffLines));

			SetRect(&spatialRect, xhead, yhead-d2p(dhalfLn),
				xhead+spatialLen, yhead+d2p(dhalfLn));
			rDiam = UseMagnifiedSize(4, doc->magnify);
			if (dim) FillRoundRect(&spatialRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&spatialRect, rDiam, rDiam); 
		}
		else
		{
			MoveTo(xadjhead, yadjhead);										/* position to draw head */
			if (noteType<=WHOLE_L_DUR)
	/* HANDLE UNKNOWN CMN DURATION/WHOLE/BREVE */
				DrawNotehead(doc, glyph, appearance, dim, dhalfLn);
			else {
	/* HANDLE STEMMED NOTE. If the note is in a chord and is stemless, skip the
	 *	entire business of drawing the stem and flags.
	 */
				if (MainNote(aNoteL)) {
					INT16 stemSpace;

					stemDown = (dStemLen>0);
					/*
					 *	In order for stems to line up with any beamsets that have already been
					 *	drawn, we have to make sure that the order of computation is the same
					 *	here as when we drew the beams, since we are not depending on any explicit
					 *	data to enforce registration.  ??CUT NEXT In particular, the conversion from
					 *	DDISTs to pixels should be done *after* adding the stemspace to the
					 *	note's left edge position, rather than before, so that we don't get
					 *	double round-off errors.  This seems to fix the ancient plague of
					 *	misregistration problems between note stems and their beams at different
					 *	magnifications.  The only way to ensure that this plague won't reoccur
					 *	is to keep the position computations for beams and note stems
					 *	exactly the same (so the same round-off errors occur). Cf. CalcXStem.
					 */
					if (stemDown)
						stemSpace = 0;
					else {
						stemSpace = MusFontStemSpaceWidthPixels(doc, doc->musFontInfoIndex, lnSpace);
						Move(stemSpace, 0);
					}
					if (ABS(dStemLen)>pt2d(1)) {
						Move(0, -1);
						Line(0, d2p(dStemLen)+(stemDown? 0 : 1));					/* Draw stem */
					}
					if (flagCount) {														/* Draw any flags */
						ypStem = yhead+d2p(dStemLen);

						if (doc->musicFontNum==sonataFontNum) {
							/* The 8th and 16th note flag characters in Sonata have their origins set so,
								in theory, they're positioned properly if drawn from the point where the note
								head was drawn. Unfortunately, the vertical position depends on the stem
								length, which Adobe incorrectly assumed was always one octave. The "extend
								flag" characters have their vertical origins set right where they go. */
							MoveTo(xhead, ypStem);										/* Position x at head, y at stem end */
							octaveLength = d2p(7*SizePercentSCALE(dhalfLn));
							Move(0, (stemDown? -octaveLength : octaveLength));	/* Adjust for flag origin */
							if (flagCount==1)												/* Draw one (8th note) flag */
								DrawMChar(doc, (stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp),
													NORMAL_VIS, dim);
							else if (flagCount>1) {										/* Draw >=2 (16th & other) flags */
								DrawMChar(doc, (stemDown? MCH_sxFlagDown : MCH_sxFlagUp),
													NORMAL_VIS, dim);
								MoveTo(xhead, ypStem);									/* Position x at head, y at stem end */
				/* ??N.B. SCALE FACTORS FOR FlagLeading BELOW ARE GUESSWORK. */
								if (stemDown)
									Move(0, -d2p(13*FlagLeading(lnSpace)/4));
								else
									Move(0, d2p(7*FlagLeading(lnSpace)/2));
								while (flagCount-- > 2) {
									if (stemDown) {
										DrawMChar(doc, MCH_extendFlagDown, NORMAL_VIS, dim);
										Move(0, -d2p(FlagLeading(lnSpace))); /* ??HUH? */
									}
									else {
										DrawMChar(doc, MCH_extendFlagUp, NORMAL_VIS, dim);
										Move(0, d2p(FlagLeading(lnSpace))); /* ??HUH? */
									}
								}
							}
						}
						else {	/* fonts other than Sonata */
							INT16 glyphWidth; DDIST xoff, yoff;

							if (MusFontUpstemFlagsHaveXOffset(doc->musFontInfoIndex))
								stemSpace = 0;

							MoveTo(xhead+stemSpace, ypStem);								/* x, y of stem end */

							if (flagCount==1) {												/* Draw 8th flag. */
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
																(stemDown? MCH_sxFlagDown : MCH_sxFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								if (xoff || yoff)
									Move(d2p(xoff), d2p(yoff));
								DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
							}
							else {																/* Draw using multiple flag chars */
								INT16 count = flagCount;

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
								MoveTo(xhead+stemSpace, ypStem);					/* start again from x,y of stem end */
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
				MoveTo(xadjhead, yadjhead);									/* position to draw head */
#ifdef VISUALIZE_PERF
				if (noteHeadGraphWidth>0 && doc->showDurProb)	//  ?? "showDurProb" IS A CRUDE TEMPORARY UI HACK!
					DrawNoteheadGraph(doc, glyph, appearance, dim, dhalfLn, aNoteL, pContext);	/* Finally, draw notehead */
				else
					DrawNotehead(doc, glyph, appearance, dim, dhalfLn);	/* Finally, draw notehead */
#else
				DrawNotehead(doc, glyph, appearance, dim, dhalfLn);	/* Finally, draw notehead */
#endif
			}

			DrawModNR(doc, aNoteL, xd, pContext);						/* Draw all modifiers */
			DrawAugDots(doc, aNoteL, xdNorm, yd, pContext, chordNoteToR);
		}
	
		PenPat(NGetQDGlobalsBlack()); 													/* Restore full strength after possible dimming for "look" */

EndQDrawing:
		if (*recalc) {
			
			GetNoteheadRect(glyph, appearance, dhalfLn, &rSub);
			/*
			 *	"Proper" size rects for noteheads can be both hard to click on and hard
			 *	to see hilited, so we just enlarge them a bit.
			 */
			InsetRect(&rSub, -1, -1);
			OffsetRect(&rSub, xhead-pContext->paper.left, yhead-pContext->paper.top);
			if (*drawn)
				UnionRect(&LinkOBJRECT(pL), &rSub, &LinkOBJRECT(pL));
			else {
				*drawn = TRUE;
				LinkOBJRECT(pL) = rSub;
			}
		}
		ForeColor(blackColor);
		TextSize(oldTxSize);
		break;
		
	case toPostScript:
	
		if (appearance==NOTHING_VIS) break;

		/* First draw any accidental symbol to left of note */
		 
		DrawAcc(doc, pContext, aNoteL, xdNorm, yd, FALSE, sizePercent, chordNoteToL);

		/*
		 *	If note is not a stemless note in a chord, add any ledger lines needed:
		 *	if it's part of a chord, for the extreme notes on both sides of the staff.
		 * All but one note of every chord has its stem length set to 0, so this
		 *	draws the ledger lines exactly once.
		 */
		
		if (MainNote(aNoteL))
			DrawNCLedgers(pL, pContext, aNoteL, xd, dTop, ledgerSizePct); 
	
	/* Now draw the note head, stem, and flags */
		
		if (doc->pianoroll)
	/* HANDLE SPATIAL (PIANOROLL) NOTATION */
		{
			;						/* ??TO BE IMPLEMENTED */
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
			if (noteType==UNKNOWN_L_DUR)
				PS_NoteStem(doc, xd+xoff, yd+yoff, xd+xoff, glyph, (DDIST)0, (DDIST)0, FALSE,
								appearance!=NO_VIS, headSizePct);
			else if (noteType<=WHOLE_L_DUR)
				PS_NoteStem(doc, xd+xoff, yd+yoff+breveFudgeHeadY, xd+xoff, glyph, (DDIST)0, (DDIST)0, FALSE,
							appearance!=NO_VIS, headSizePct);
			
			else {
			
				/*
				 *	STEMMED NOTE. If the note is in a chord and is stemless, skip
				 *	entire business of drawing the stem and flags.
				 */
				 
				if (MainNote(aNoteL)) {
					stemDown = (dStemLen>0);
					xdAdj = (headRelSize==100? 0 : XD_GLYPH_ADJ(stemDown, headRelSize));
					PS_NoteStem(doc, (xd+xoff)-xdAdj, yd+yoff, xd+xoff, glyph, dStemLen, dStemShorten,
										aNote->beamed, appearance!=NO_VIS, headSizePct);
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
												TRUE, sizePercent);
									break;
								case 2:
									PS_MusChar(doc, xd, ypStem, stemDown? MCH_sxFlagDown : MCH_sxFlagUp, TRUE, sizePercent);
									break;
								default:
									if (stemDown) {
										ypStem = yd + dStemLen;
										while (flagCount-- > 0) {
											PS_MusChar(doc, xd, ypStem, MCH_extendFlagDown, TRUE, sizePercent);
											ypStem -= FlagLeading(lnSpace);
											}
										}
									 else {
										ypStem = yd + dStemLen;
										while (flagCount-- > 0) {
											PS_MusChar(doc, xd, ypStem, MCH_extendFlagUp, TRUE, sizePercent);
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
								PS_MusChar(doc, xd+xoff+dHeadWidth, ydStem+yoff, flagGlyph, TRUE, sizePercent);
							}
							else if (flagCount==2 && MusFontHas16thFlag(doc->musFontInfoIndex)) {	/* Draw 16th flag using one flag char. */
								flagGlyph = MapMusChar(doc->musFontInfoIndex, 
																	(stemDown? MCH_sxFlagDown : MCH_sxFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								PS_MusChar(doc, xd+xoff+dHeadWidth, ydStem+yoff, flagGlyph, TRUE, sizePercent);
							}
							else {
								DDIST ypos;
								INT16 count = flagCount;

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
									PS_MusChar(doc, xd+xoff+dHeadWidth, ypos+yoff, flagGlyph, TRUE, sizePercent);
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
								PS_MusChar(doc, xd+xoff+dHeadWidth, ypos+yoff+flagLeading, flagGlyph, TRUE, sizePercent);
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
						xdAdj = XD_GLYPH_ADJ(stemDown, headRelSize);
				 	}
					PS_NoteStem(doc, (xd+xoff)-xdAdj, yd+yoff, xd+xoff, glyph, (DDIST)0, (DDIST)0, aNote->beamed,
										appearance!=NO_VIS, headSizePct);
				}
			}
		/*
		 *	Note drawn: now add modifiers and augmentation dots, if any
		 */
			DrawModNR(doc, aNoteL, xd, pContext);								/* Draw all modifiers */
			DrawAugDots(doc, aNoteL, xdNorm, yd, pContext, chordNoteToR);
		}
		
		break;
	}
	
PopLock(OBJheap);
PopLock(NOTEheap);
}


/* ----------------------------------------------------------------- DrawMBRest -- */
/* Draw a multibar rest. */

static void DrawMBRest(Document *doc, PCONTEXT pContext,
						LINK theRestL,					/* Subobject (rest) to draw */
						DDIST xd, DDIST yd,
						INT16 appearance,
						Boolean dim,					/* Ignored if outputTo==toPostScript */
						Rect *prSub						/* Subobject selection/hiliting box */
						)
{
	DDIST dTop, lnSpace, dhalfLn, xdNum, ydNum, endBottom, endTop, endThick;
	DRect dRestBar; Rect restBar; INT16 numWidth;
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

	/*
	 * In each imaging model, we draw first the thick bar; then the thin vertical
	 * lines at each end of the thick bar; and finally the number.
	 */
	switch (outputTo) {
		case toScreen:
		case toImageWriter:
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
				if (dim) DrawMString(doc, numStr, NORMAL_VIS, TRUE);
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


/* ------------------------------------------------------------------- DrawRest -- */


#define STEMLET_LEN 4		/* For beamed rests (quarter-spaces) */

void DrawRest(Document *doc,
				LINK pL,
				PCONTEXT pContext,	/* current context[] entry */
				LINK aRestL,			/* current subobject */
				Boolean *drawn,		/* FALSE until a subobject has been drawn */
				Boolean *recalc		/* TRUE if we need to recalc enclosing rectangle */
				)
{
	PANOTE	aRest;
	INT16		xp, yp,			/* pixel coordinates */
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
	Rect		rSub;				/* bounding box for subobject */
	char		lDur;
	Boolean	stemDown,
				dim;				/* Should it be dimmed bcs in a voice not being looked at? */

	if (doc->pianoroll) return;			/* ??REALLY DO NOTHING, E.G., WITH OBJRECT?? */

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
	case toImageWriter:
	case toPICT:
		fudgeRestY = GetYRestFudge(useTxSize, lDur);			/* Get fine Y-offset for rest */
		
		/* If rest's voice is not being "looked" at, dim it */
		dim = (outputTo==toScreen && !LOOKING_AT(doc, aRest->voice));
		ForeColor(Voice2Color(doc, aRest->voice));

		/*
		 * Draw the rest, either multibar (ignore modifiers and aug. dots) or normal
		 * (with modifiers and aug. dots), and get its bounding box.
		 */
		if (aRest->subType<WHOLEMR_L_DUR) {
			DrawMBRest(doc, pContext, aRestL, xd, yd, appearance, dim, &rSub);
			if (*recalc)
				OffsetRect(&rSub, -pContext->paper.left, -pContext->paper.top);
		}
		else {
			Byte unmappedGlyph = glyph;										/* save for below */
			glyph = MapMusChar(doc->musFontInfoIndex, glyph);
			restxd += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace));
			restyd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace));

			xp = pContext->paper.left+d2p(xd);
			yp = pContext->paper.top+d2p(yd)+fudgeRestY;
			restxp = pContext->paper.left+d2p(restxd);
			restyp = pContext->paper.top+d2p(restyd)+fudgeRestY;

			if (appearance!=NOTHING_VIS) {
				MoveTo(restxp, restyp);											/* Position pen */
				DrawMChar(doc, glyph, appearance, dim);					/* Draw the rest */
	
				if (xledg) {														/*	Draw any pseudo-ledger line */
					MoveTo(xpLedg=xp-d2p(xledg),ypLedg=yp-d2p(yledg));
					Line(d2p(LedgerLen(lnSpace)+LedgerOtherLen(lnSpace)),0);
				}
				DrawModNR(doc, aRestL, xd, pContext);						/* Draw all modifiers */
				DrawAugDots(doc, aRestL, xd, ydNorm, pContext, FALSE);
	
				/* If we're supposed to draw stemlets on beamed rests, do so. */
				
				if (aRest->beamed && STEMLET_LEN>=0 && config.drawStemlets) {
					INT16 stemSpace = MusFontStemSpaceWidthPixels(doc, doc->musFontInfoIndex, lnSpace);

					qStemLen = 2*NFLAGS(aRest->subType)-1+STEMLET_LEN;
					dStemLen = lnSpace*qStemLen/4;
					MoveTo(xp, pContext->paper.top+d2p(dTop+aRest->ystem));
					stemDown = (aRest->ystem > aRest->yd);
					if (!stemDown) Move(stemSpace, 0);
					Line(0, d2p(stemDown? -dStemLen : dStemLen));	/* Draw from stem toward rest */
				}
			}

			if (*recalc) {
				rSub = charRectCache.charRect[glyph];
				/* ??The following #if/else/endif is a mess: the commented-out code looks
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
				*drawn = TRUE;
				LinkOBJRECT(pL) = rSub;
			}
		}
		ForeColor(blackColor);
		break;
		
	case toPostScript:
		if (appearance==NOTHING_VIS) break;
	
		if (appearance!=NO_VIS) {
			if (aRest->subType<WHOLEMR_L_DUR)
				DrawMBRest(doc, pContext, aRestL, xd, yd, appearance, FALSE, &rSub);
			else {
				glyph = MapMusChar(doc->musFontInfoIndex, glyph);
				restxd += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace));
				restyd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace));
				PS_MusChar(doc, restxd, restyd, glyph, TRUE, sizePercent);
				if (xledg)
					PS_LedgerLine(yd-yledg,xd-xledg,LedgerLen(lnSpace)+LedgerOtherLen(lnSpace));

				/*	Rest drawn: now add augmentation dots, if any */
				DrawAugDots(doc, aRestL, xd, ydNorm, pContext, FALSE);
			}
		}
		
		DrawModNR(doc, aRestL, xd, pContext);						/* Draw all modifiers */
		break;
	}

	TextSize(oldTxSize);

PopLock(OBJheap);
PopLock(NOTEheap);
}


/* ----------------------------------------------------------------- ShowNGRSync -- */
/* Draw a gray vertical line from near the bottom to near the top of the objRect
of the objRect of the given object, but only if it's tall enough to help clarify
what's related to what. Intended for Syncs and GRSyncs, but should work for any
non-J_STRUC object. */

void ShowNGRSync(Document *doc, LINK pL, CONTEXT context[])
{
	Rect r;
	INT16 ypTop, ypBottom;
	DDIST dhalfLn;
	
	r = LinkOBJRECT(pL);
	/* Convert r to window coords */
	OffsetRect(&r,doc->currentPaper.left,doc->currentPaper.top);
	PenMode(patXor);
	PenPat(NGetQDGlobalsGray());
	dhalfLn = LNSPACE(&context[1])/2;
	ypTop = r.top+d2p(dhalfLn);
	ypBottom = r.bottom-d2p(dhalfLn);
	if (ypBottom-ypTop>d2p(2*dhalfLn)) {
		MoveTo(r.left+2, ypTop);
		LineTo(r.left+2, ypBottom);
	}
	PenNormal();														/* Restore normal drawing */
}


/* -------------------------------------------------------------------- DrawSYNC -- */
/* Draw a SYNC object, i.e., all of its notes and rests with their augmentation dots,
modifiers, and accidentals. */

void DrawSYNC(Document *doc, LINK pL, CONTEXT context[])
{
	PANOTE		aNote;
	LINK			aNoteL;
	PCONTEXT		pContext;
	Boolean		drawn;			/* FALSE until a subobject has been drawn */
	Boolean		recalc;			/* TRUE if we need to recalc enclosing rectangle */
	STFRANGE		stfRange = {0,0};
	Point			enlarge = {0,0};
	
	drawn = FALSE;
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

	if (recalc) LinkVALID(pL) = TRUE;
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


/* ------------------------------------------------------------------- DrawGRAcc -- */
/* Draw accidental, if any, for the given grace note. */

static void DrawGRAcc(Document *doc,
						PCONTEXT pContext,
						LINK theGRNoteL,		/* Subobject (grace note) to draw accidental for */
						DDIST xd, DDIST yd,	/* Notehead position, including effect of <otherStemSide> */
						Boolean dim,			/* Ignored if outputTo==toPostScript */
						INT16 sizePercent		/* Of normal note size */
						)
{
	PAGRNOTE theGRNote;
	DDIST accXOffset, d8thSp, xdAcc, xdLParen, xdRParen, ydParens, lnSpace;
	INT16 xmoveAcc, xp, yp, delta, scalePercent;
	Byte accGlyph, lparenGlyph, rparenGlyph;

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
	accXOffset = SizePercentSCALE(AccXOffset(xmoveAcc, pContext));

 	/* If it's a courtesy accidental, position the parentheses and move the accidental.	*/

 	xdAcc = xd-accXOffset;
	if (theGRNote->courtesyAcc) {
		DDIST xoffset, yoffset;

		/* ??These adjustments are probably not quite right, due to combination of 
				sizePercent, scalePercent, and the various config tweaks. */

		scalePercent = (INT16)(((long)config.courtesyAccSize*sizePercent)/100L);

		lparenGlyph = MapMusChar(doc->musFontInfoIndex, SCHAR_LPAREN);
		rparenGlyph = MapMusChar(doc->musFontInfoIndex, SCHAR_RPAREN);

		xoffset = MusCharXOffset(doc->musFontInfoIndex, rparenGlyph, lnSpace);
		xdRParen = xdAcc + ((DDIST)(((long)scalePercent*xoffset)/100L));

		delta = (INT16)(((long)scalePercent*config.courtesyAccRXD)/100L);
		xdAcc = xdRParen-delta*d8thSp;

		delta = (INT16)(((long)scalePercent*config.courtesyAccLXD)/100L);
		xdLParen = xdAcc-delta*d8thSp;

		yoffset = MusCharYOffset(doc->musFontInfoIndex, rparenGlyph, lnSpace);	/* assuming both parens have same yoffset */
		delta = (INT16)(((long)scalePercent*config.courtesyAccYD)/100L);
		ydParens = yd + delta*d8thSp + ((DDIST)(((long)scalePercent*yoffset)/100L));;
	}
	xdAcc += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, accGlyph, lnSpace));
	yd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, accGlyph, lnSpace));

	switch (outputTo) {
		case toScreen:
		case toImageWriter:
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
				DrawMChar(doc, lparenGlyph, NORMAL_VIS, dim);
				xp = pContext->paper.left+d2p(xdRParen);
				MoveTo(xp, yp);
				DrawMChar(doc, rparenGlyph, NORMAL_VIS, dim);
			}
			
			break;
		case toPostScript:
			PS_MusChar(doc, xdAcc, yd, accGlyph, TRUE, sizePercent);

			/* If it's a courtesy accidental, draw parentheses around it. */
			if (theGRNote->courtesyAcc) {
				PS_MusChar(doc, xdLParen, ydParens, lparenGlyph, TRUE, scalePercent);
				PS_MusChar(doc, xdRParen, ydParens, rparenGlyph, TRUE, scalePercent);
			}
			
			break;
	}
}

/* --------------------------------------------------------------- DrawGRNCLedgers -- */
/* Draw any ledger lines needed: if grace note is in a chord, for the chord's
extreme notes on both sides of the staff; if it's not in a chord, just for itself.
Should be called only once per chord. Assumes the GRNOTE and OBJECT heaps are locked! */

static void DrawGRNCLedgers(LINK syncL, PCONTEXT pContext, LINK aGRNoteL, DDIST xd,
										DDIST dTop, INT16 ledgerSizePct)
{
	LINK		bGRNoteL, mainGRNoteL;
	PAGRNOTE	aGRNote, bGRNote;
	QDIST		yqpitRel,				/* y QDIST position relative to staff top */
				hiyqpit, lowyqpit;
	Boolean	stemDown;

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
		NoteLedgers(xd, yqpitRel, 0, stemDown, dTop, pContext, ledgerSizePct);
		yqpitRel = hiyqpit+
						halfLn2qd(ClefMiddleCHalfLn(pContext->clefType));
		NoteLedgers(xd, yqpitRel, 0, stemDown, dTop, pContext, ledgerSizePct);
	}
	else if (aGRNote->yd<0 || aGRNote->yd>pContext->staffHeight) {
		yqpitRel = aGRNote->yqpit+
						halfLn2qd(ClefMiddleCHalfLn(pContext->clefType));
		NoteLedgers(xd, yqpitRel, 0, stemDown, dTop, pContext, ledgerSizePct);
	}
}


/* ------------------------------------------------------------------ DrawGRNote -- */

#define SLASH_YDELTA(len) (3*(len)/4)

void DrawGRNote(Document *doc,
						LINK pL,					/* GRSYNC grace note belongs to */
						PCONTEXT pContext,	/* current context[] entry */
						LINK aGRNoteL,			/* Subobject (grace note) to draw */
						Boolean	*drawn,		/* FALSE until a subobject has been drawn */
						Boolean	*recalc)		/* TRUE if we need to recalc enclosing rectangle */
{
	PAGRNOTE	aGRNote;
	INT16		flagCount,		/* number of flags on grace note */
				staffn,
				voice;
	INT16		xhead, yhead,	/* pixel coordinates */
				appearance,
				ypStem, yp,
				octaveLength,
				spatialLen,
				rDiam,			/* rounded corner diameter for pianoroll */
				useTxSize,
				oldTxSize,		/* To restore port after small or magnified notes drawn */
				sizePercent,	/* Percent of "normal" (non-grace) note size to draw in */
				headRelSize,
				headSizePct,
				ledgerSizePct,	/* Percent of "normal" size for ledger lines */
				stemShorten,
				flagLeading,
				xadjhead, yadjhead;
	unsigned char glyph,		/* notehead symbol actually drawn */
				flagGlyph;
	DDIST		xd, yd,			/* scratch */
				dStemLen,
				xdStem2Slash,
				xdSlash, ydSlash,
				slashLen, slDiff,
				xdNorm,
				dTop, dLeft,	/* abs DDIST origin */
				lnSpace, dhalfLn,	/* Distance between staff lines and half of that */
				fudgeHeadY,		/* Correction for roundoff in screen font head shape */
				breveFudgeHeadY,	/* Correction for Sonata error in breve origin */
				dGrSlashThick,
				offset;
	QDIST		qdLen;
	Boolean	stemDown,		/* Does grace note have a down-turned stem? */
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
	if (aGRNote->subType==BREVE_L_DUR) breveFudgeHeadY = dhalfLn*BREVEYOFFSET;
	else									 	  breveFudgeHeadY = (DDIST)0;

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
		slDiff = SLASH_YDELTA(slashLen);
	}

/* ??INSTEAD OF <WIDEHEAD> & <IS_WIDEREST>, WE SHOULD USE SMTHG LIKE CharRect OF glyph
TO GET HEADWIDTH! THE SAME GOES FOR DrawMODNR AND DrawRest. */
	ledgerSizePct = (WIDEHEAD(aGRNote->subType)==2? 15*sizePercent/10 :
							(WIDEHEAD(aGRNote->subType)==1? 13*sizePercent/10 :
																			sizePercent) );		

	/*	Suppress flags if the grace note is beamed. */
	flagCount = (aGRNote->beamed? 0 : NFLAGS(aGRNote->subType));

	slashStem = FALSE;
	if (config.slashGraceStems!=0) {
		if (flagCount==1) slashStem = TRUE;
		else {
			if (config.slashGraceStems>1 && aGRNote->beamed) {
				voice = GRNoteVOICE(aGRNoteL);
				beamL = LVSearch(pL, BEAMSETtype, voice, GO_LEFT, FALSE);	/* Should always succeed */
				if (pL==FirstInBeam(beamL)) slashStem = TRUE;
			}
		}
	}

	TextSize(useTxSize);

	dStemLen = aGRNote->ystem - aGRNote->yd;

	switch (outputTo) {
	case toScreen:
	case toImageWriter:
	case toPICT:
		/* See the comment on fine-tuning note Y-positions in DrawNote. */
		if (outputTo==toImageWriter && bestQualityPrint)
			fudgeHeadY = 0;													/* No fine Y-offset for notehead */
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
		 *	If grace note is not a non-Main note in a chord, draw any ledger lines needed:
		 * for grace note or entire chord. A chord has only one GRMainNote, so this draws
		 *	the ledger lines exactly once.
		 */
		if (GRMainNote(aGRNoteL))
			DrawGRNCLedgers(pL, pContext, aGRNoteL, xd, dTop, ledgerSizePct);
			
		if (doc->pianoroll)
		/* HANDLE SPATIAL (PIANOROLL) NOTATION */
		{
			MoveTo(xhead,yhead);												/* position to draw head */
			resFact = RESFACTOR*(long)doc->spacePercent;
			qdLen = IdealSpace(doc, aGRNote->playDur, resFact);
			spatialLen = d2p(qd2d(qdLen, pContext->staffHeight,
									pContext->staffLines));
			spatialLen = n_max(spatialLen, 4);								/* Insure long enuf to be visible */
			SetRect(&spatialRect, xhead, yhead-d2p(dhalfLn),
				xhead+spatialLen, yhead+d2p(dhalfLn));
			rDiam = UseMagnifiedSize(4, doc->magnify);
			if (dim) FillRoundRect(&spatialRect, rDiam, rDiam, NGetQDGlobalsGray());
			else		PaintRoundRect(&spatialRect, rDiam, rDiam); 
		}
		else
		{
			MoveTo(xadjhead, yadjhead);										/* position to draw head */
			if (aGRNote->subType==UNKNOWN_L_DUR)
		/* HANDLE UNKNOWN CMN DURATION: Show a solid notehead alone. */
				DrawMChar(doc, glyph, appearance, dim);
			else if (aGRNote->subType<=WHOLE_L_DUR)
		/* HANDLE WHOLE/BREVE */
				DrawMChar(doc, glyph, appearance, dim);
			else {
		/* HANDLE STEMMED NOTE. If the grace note is in a chord and is stemless, skip the
		 *	entire business of drawing the stem and flags.
		 */
				if (GRMainNote(aGRNoteL)) {
					INT16 stemSpace;

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
						Line(d2p(slashLen), -d2p(slDiff));
					}

					if (flagCount) {														/* Draw any flags */
						ypStem = yhead+d2p(dStemLen);

						if (doc->musicFontNum==sonataFontNum) {
							/* The 8th and 16th note flag characters in Sonata have their origins set so,
								in theory, they're positioned properly if drawn from the point where the note
								head was drawn. Unfortunately, the vertical position depends on the stem
								length, which Adobe incorrectly assumed was always one octave. The "extend
								flag" characters have their vertical origins set right where they go. */
							MoveTo(xhead, ypStem);										/* Position x at head, y at stem end */
							octaveLength = d2p(7*SizePercentSCALE(dhalfLn));
							Move(0, (stemDown? -octaveLength : octaveLength));	/* Adjust for flag origin */
							if (flagCount==1)												/* Draw one (8th note) flag */
								DrawMChar(doc, (stemDown? MCH_eighthFlagDown : MCH_eighthFlagUp),
													NORMAL_VIS, dim);
							else if (flagCount>1) {										/* Draw >=2 (16th & other) flags */
								DrawMChar(doc, (stemDown? MCH_sxFlagDown : MCH_sxFlagUp),
													NORMAL_VIS, dim);
								MoveTo(xhead, ypStem);									/* Position x at head, y at stem end */
				/* ??N.B. SCALE FACTORS FOR FlagLeading BELOW ARE GUESSWORK. */
								if (stemDown)
									Move(0, -d2p(13*FlagLeading(lnSpace)/4));
								else
									Move(0, d2p(7*FlagLeading(lnSpace)/2));
								while (flagCount-- > 2) {
									if (stemDown) {
										DrawMChar(doc, MCH_extendFlagDown, NORMAL_VIS, dim);
										Move(0, -d2p(FlagLeading(lnSpace))); /* ??HUH? */
									}
									else {
										DrawMChar(doc, MCH_extendFlagUp, NORMAL_VIS, dim);
										Move(0, d2p(FlagLeading(lnSpace))); /* ??HUH? */
									}
								}
							}
						}
						else {	/* fonts other than Sonata */
							INT16 glyphWidth; DDIST xoff, yoff;

							if (MusFontUpstemFlagsHaveXOffset(doc->musFontInfoIndex))
								stemSpace = 0;

							MoveTo(xhead+stemSpace, ypStem);								/* x, y of stem end */

							if (flagCount==1) {												/* Draw 8th flag. */
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
																(stemDown? MCH_sxFlagDown : MCH_sxFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								if (xoff || yoff)
									Move(d2p(xoff), d2p(yoff));
								DrawMChar(doc, flagGlyph, NORMAL_VIS, dim);
							}
							else {																/* Draw using multiple flag chars */
								INT16 count = flagCount;

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
								MoveTo(xhead+stemSpace, ypStem);					/* start again from x,y of stem end */
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
				DrawMChar(doc, glyph, appearance, dim);  			/* Finally, draw notehead */
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
				*drawn = TRUE;
				LinkOBJRECT(pL) = rSub;
			}
		}
		ForeColor(blackColor);
		break;
		
	case toPostScript:
	
		if (appearance==NOTHING_VIS) break;

		/* First draw any accidental symbol to left of grace note */
		 
		DrawGRAcc(doc, pContext, aGRNoteL, xdNorm, yd, FALSE, sizePercent);

		/*
		 *	If grace note is not a stemless note in a chord, draw any ledger lines needed
		 * for grace note or entire chord. All but one note of every chord has its stem
		 * length set to 0, so this draws the ledger lines exactly once.
		 */
		
		if (GRMainNote(aGRNoteL))
			DrawGRNCLedgers(pL, pContext, aGRNoteL, xd, dTop, ledgerSizePct);
	
	/* Now draw the grace note head, stem, and flags */
		
		if (doc->pianoroll)
	/* HANDLE SPATIAL (PIANOROLL) NOTATION */
		{
			;						/* ??TO BE IMPLEMENTED */
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
												TRUE, sizePercent);
									break;
								case 2:
									PS_MusChar(doc, xd, ypStem, stemDown? MCH_sxFlagDown : MCH_sxFlagUp, TRUE, sizePercent);
									break;
								default:
									if (stemDown) {
										ypStem = yd + dStemLen;
										while (flagCount-- > 0) {
											PS_MusChar(doc, xd, ypStem, MCH_extendFlagDown, TRUE, sizePercent);
											ypStem -= FlagLeading(lnSpace);
											}
										}
									 else {
										ypStem = yd + dStemLen;
										while (flagCount-- > 0) {
											PS_MusChar(doc, xd, ypStem, MCH_extendFlagUp, TRUE, sizePercent);
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
								PS_MusChar(doc, xd+xoff+dHeadWidth, ydStem+yoff, flagGlyph, TRUE, sizePercent);
							}
							else if (flagCount==2 && MusFontHas16thFlag(doc->musFontInfoIndex)) {	/* Draw 16th flag using one flag char. */
								flagGlyph = MapMusChar(doc->musFontInfoIndex, 
																	(stemDown? MCH_sxFlagDown : MCH_sxFlagUp));
								xoff = MusCharXOffset(doc->musFontInfoIndex, flagGlyph, lnSpace);
								yoff = SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, flagGlyph, lnSpace));
								PS_MusChar(doc, xd+xoff+dHeadWidth, ydStem+yoff, flagGlyph, TRUE, sizePercent);
							}
							else {
								DDIST ypos;
								INT16 count = flagCount;

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
									PS_MusChar(doc, xd+xoff+dHeadWidth, ypos+yoff, flagGlyph, TRUE, sizePercent);
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
								PS_MusChar(doc, xd+xoff+dHeadWidth, ypos+yoff+flagLeading, flagGlyph, TRUE, sizePercent);
							}
						}
					}

 					/* Normally, draw a slash across the stem. */
					if (slashStem) {
						dGrSlashThick = ((config.graceSlashLW*lnSpace)/100L);
						PS_Line(xdSlash, ydSlash, xdSlash+slashLen, ydSlash-slDiff,
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

/* ------------------------------------------------------------------ DrawGRSYNC -- */
/* Draw a GRSYNC object, i.e., all of its grace notes with their augmentation dots
and accidentals. */

void DrawGRSYNC(Document *doc, LINK pL, CONTEXT context[])
{
	PAGRNOTE		aGRNote;
	LINK			aGRNoteL;
	PCONTEXT		pContext;
	Boolean		drawn;			/* FALSE until a subobject has been drawn */
	Boolean		recalc;			/* TRUE if we need to recalc enclosing rectangle */
	Point			enlarge = {0,0};
	
	drawn = FALSE;
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

	if (recalc) LinkVALID(pL) = TRUE;
#ifdef USE_HILITESCORERANGE
	if (LinkSEL(pL)) CheckGRSYNC(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
#endif
	if (doc->showSyncs) ShowNGRSync(doc, pL, context);
}

