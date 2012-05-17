/* AccModNRMouse.c for Nightingale - handle dragging accidentals and ModNRs */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1992-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static short FindAccModNR(Document *, Point, LINK *, LINK *, LINK *, Rect *);
static LINK FindAccidental(Document *, Point, LINK *, Rect *);
static void GetAccidentalBbox(Document *, LINK, LINK, Rect *);
static void DoAccidentalDrag(Document *, Point, LINK, LINK, Rect *);
static DDIST GetAccXOffset(PANOTE, INT16, PCONTEXT);
static void DrawAccidentalParams(Document *doc, INT16);
static Boolean GetModNRbbox(Document *, LINK, LINK, LINK, Rect *);
static void DoModNRDrag(Document *, Point, LINK, LINK, LINK, Rect *);
static void DrawModNRParams(Document *doc, SHORTSTD, SHORTSTD);

static enum {			/* return values for FindAccModNR */
	NOHIT=0,
	MODNR,
	ACCIDENTAL
} E_ModNRItems;

//#define USE_BITMAP	/* use offscreen bitmap when dragging; otherwise use srcXor drawing mode */


static short FindAccModNR(Document *doc, Point pt,
						LINK *syncL, LINK *noteL,
						LINK *modNRL,			/* ignored if we hit an accidental */
						Rect *bbox				/* paper coords */
						)
{
	LINK		pL, pageL, startL, endL, firstMeasL, measL, aNoteL, aModNRL;
	PANOTE	aNote;
	short		i, aNoteAcc;
		
	pageL = GetCurrentPage(doc);
	endL = RightLINK(LastObjOnPage(doc, pageL));
	
	/* Find the measure the click is in. Note that this measure may not contain
	 * the bbox of the symbol (modNR or accidental) we've clicked on, if that symbol
	 * isn't inside its system rect, or simply isn't inside its measure. If the symbol
	 * is BELOW its system rect or is in the measure to the RIGHT of the one its note is in,
	 * we'll still eventually reach the symbol's note. But if our search finds nothing,
	 * we must start again at the top of the page and search to the original startL, just
	 * in case we've clicked on a symbol that's ABOVE its system rect or is in the measure
	 * to the LEFT of its note's measure.
	 */
	startL = NILINK;
	firstMeasL = LSSearch(pageL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	for (measL = firstMeasL; SamePage(measL, firstMeasL); measL = LinkRMEAS(measL)) {
		if (PtInMeasure(doc, pt, measL)) {
			startL = measL;
			break;
		}
	}
	if (!startL) startL = pageL;			/* in case we've clicked outside of any measure rect */
	
	for (i=0; i<2; i++) {												/* may have to search twice */
		for (pL=startL ; pL!=endL; pL=RightLINK(pL)) {			/* find object that was clicked on */
			if (ObjLType(pL) == SYNCtype) {
				if (VISIBLE(pL)) {
					for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (!LOOKING_AT(doc, aNote->voice)) continue;
						aModNRL = aNote->firstMod;
						aNoteAcc = aNote->accident;
						if (aModNRL) {
							for ( ; aModNRL; aModNRL = NextMODNRL(aModNRL)) {
								if (!GetModNRbbox(doc, pL, aNoteL, aModNRL, bbox))
									continue;
								if (PtInRect(pt, bbox)) {
									*noteL = aNoteL;
									*modNRL = aModNRL;
									*syncL = pL;
									return MODNR;
								}
							}
						}
						if (aNoteAcc && !aNote->rest) {
							GetAccidentalBbox(doc, pL, aNoteL, bbox);
							if (PtInRect(pt, bbox)) {
								*noteL = aNoteL;
								*syncL = pL;
								return ACCIDENTAL;
							}
						}
					}
				}
			}
		}
		/* If we've reached this point, the first search found nothing,
		 * so we must start over at the top of the page.
		 */
		if (startL == pageL) return NOHIT;			/* we've already searched the whole page */
		endL = startL;
		startL = pageL;
	}
	return NOHIT;
}


/* If user double-clicked on a modNR, invokes a dialog to let user change
the modifier and returns TRUE; otherwise, just returns FALSE. */

Boolean DoOpenModNR(Document *doc, Point pt)
{
	LINK	syncL, noteL, modNRL;
	short	result;
	Byte	modCode;
	Boolean change;
	Rect	bbox, bboxOld, bboxNew;
	
	result = FindAccModNR(doc, pt, &syncL, &noteL, &modNRL, &bboxOld);
	
	if (result==MODNR) {
		DisableUndo(doc, FALSE);
		modCode = ModNRMODCODE(modNRL);
		change = SetModNRDialog(&modCode);
		if (change) {
			ModNRMODCODE(modNRL) = modCode;

			if (GetModNRbbox(doc, syncL, noteL, modNRL, &bboxNew))
				UnionRect(&bboxOld, &bboxNew, &bbox);
			else
				bbox = bboxNew;

			Rect2Window(doc, &bbox);
			InsetRect(&bbox, -1, -4);
			EraseAndInval(&bbox);				/* ??NB: if bbox entirely outside of systemRect, it won't be redrawn */

			doc->changed = TRUE;
		}
		return TRUE;
	}

	return FALSE;
}


/* If user clicked on a modNR or an accidental, calls functions to handle dragging and
returns TRUE; otherwise, just returns FALSE. */

Boolean DoAccModNRClick(Document *doc, Point pt)
{
	LINK	syncL, noteL, modNRL;
	short	result;
	Rect	bbox;
	
	result = FindAccModNR(doc, pt, &syncL, &noteL, &modNRL, &bbox);
	
	if (result==MODNR) {
		DisableUndo(doc, FALSE);
		DoModNRDrag(doc, pt, syncL, noteL, modNRL, &bbox);
		return TRUE;
	}
	if (result==ACCIDENTAL) {
		DisableUndo(doc, FALSE);
		DoAccidentalDrag(doc, pt, syncL, noteL, &bbox);
		return TRUE;
	}
	
	return FALSE;
}


/* ======================================================== Accidental functions == */

/* Returns box surrounding the accidental in paper-rel coordinates.
 * Much code borrowed from DrawACC.
 */
static void GetAccidentalBbox(Document *doc, LINK syncL, LINK noteL, Rect *accBBox)
{
	DDIST		noteXD, noteYD, accXOffset, xdNorm;
	INT16		oldTxSize, useTxSize, sizePercent, charWid, baseLine;
	PANOTE	aNote;
	CONTEXT	context;
	Rect		accCharRect;
	Boolean	chordNoteToL;
	short		accidentalChar, wid, ht;
	DDIST		d8thSp, dIncreaseWid;
	
	GetContext(doc, syncL, NoteSTAFF(noteL), &context);
	
	aNote = GetPANOTE(noteL);
	noteXD = NoteXLoc(syncL, noteL, context.measureLeft, HeadWidth(LNSPACE(&context)), &xdNorm);
	noteYD = context.measureTop + aNote->yd;
	accidentalChar = MapMusChar(doc->musFontInfoIndex, SonataAcc[aNote->accident]);

	sizePercent = (aNote->small? SMALLSIZE(100) : 100);
	chordNoteToL = (!aNote->rest && ChordNoteToLeft(syncL, aNote->voice));
	if (chordNoteToL) xdNorm -= SizePercentSCALE(HeadWidth(LNSPACE(&context)));
	accXOffset = GetAccXOffset(aNote, sizePercent, &context);

	oldTxSize = GetPortTxSize();
	useTxSize = UseMTextSize(SizePercentSCALE(context.fontSize), doc->magnify);
	TextSize(useTxSize);
	BuildCharRectCache(doc);						/* current cache may not be valid for this accidental's size */
	charWid = CharWidth(accidentalChar);		/* more accurate for screen fonts, especially when scaled */
	TextSize(oldTxSize);

	accCharRect = CharRect(accidentalChar);

	accBBox->left = d2p(xdNorm) - d2p(accXOffset);
	accBBox->right = accBBox->left + charWid;

	/*
	 * If it's a courtesy accidental, increase width to allow for the parentheses.
	 * This code doesn't take into account <small> accidentals, whose parens are
	 * closer to the accidental; shouldn't make too much difference, though.
	 */
	aNote = GetPANOTE(noteL);
	if (aNote->courtesyAcc) {
		d8thSp = LNSPACE(&context)/8;
		dIncreaseWid = (config.courtesyAccLXD+config.courtesyAccRXD)*d8thSp;
		accBBox->left -= d2p(dIncreaseWid);
	}

	baseLine = d2p(noteYD);
	accBBox->bottom = baseLine + accCharRect.bottom;
	accBBox->top = baseLine + accCharRect.top - 1;		/* this still doesn't always enclose the char... */
	
	accBBox->top -= 3;											/* so make it taller */
	accBBox->bottom += 2;
	
	/* add more slop if the rect is very small */
	wid = accBBox->right - accBBox->left;
	ht = accBBox->bottom - accBBox->top;
	InsetRect(accBBox, wid<6? -2 : 0, ht<6? -2 : 0);
}


static void DoAccidentalDrag(Document *doc, Point pt, LINK syncL, LINK noteL,
									Rect *origAccBBox)				/* in paper coords */
{
	DDIST		noteXD, noteYD, xdNorm, xdNormAdjusted, xOffset, accXOffset, dAccWidth;
	INT16		accCode, oldTxMode, oldTxSize, useTxSize, 
				sizePercent, diffH, accOriginH, xmoveAcc;
	Point		oldPt, newPt;
	Boolean	chordNoteToL, suppressRedraw = FALSE;
	ANOTE		oldNote;	
	PANOTE	aNote;
	CONTEXT	context;
	
PushLock(OBJheap);
PushLock(NOTEheap);

	GetContext(doc, syncL, NoteSTAFF(noteL), &context);
	
	aNote = GetPANOTE(noteL);
	oldNote = *aNote;
	noteXD = NoteXLoc(syncL, noteL, context.measureLeft, HeadWidth(LNSPACE(&context)), &xdNorm);
	noteYD = context.measureTop + aNote->yd;
	accCode = aNote->accident;

	sizePercent = aNote->small? SMALLSIZE(100) : 100;
	chordNoteToL = ChordNoteToLeft(syncL, aNote->voice);
	accXOffset = GetAccXOffset(aNote, sizePercent, &context);

	oldTxMode = GetPortTxMode();
//	oldTxMode = qd.thePort->txMode;
	oldTxSize = GetPortTxSize();
	useTxSize = UseMTextSize(context.fontSize, doc->magnify);
	if (aNote->small) useTxSize = SMALLSIZE(useTxSize);
	TextSize(useTxSize);
	
	TextMode(srcXor);
	DrawAcc(doc, &context, noteL, xdNorm, noteYD,
						FALSE, sizePercent, chordNoteToL);		/* erase original accidental */
	TextMode(srcOr);
	DrawAcc(doc, &context, noteL, xdNorm, noteYD,
						TRUE, sizePercent, chordNoteToL);		/* draw it in gray */
	TextMode(srcXor);

	DrawAccidentalParams(doc, aNote->xmoveAcc);				/* display this now in case we never drag */

	/* Adjust xdNorm for chordNoteToL notes. Don't change xdNorm, because
	 * DrawAcc already does that for us.
	 */
	xdNormAdjusted = chordNoteToL?
			xdNorm-SizePercentSCALE(HeadWidth(LNSPACE(&context))) : xdNorm;
	accOriginH = d2p(xdNormAdjusted) - d2p(accXOffset);
	
	/* <diffH> is the distance between the point where the user clicked
	 * and the origin of the accidental. Set this from pt, rather
	 * than from the current mouseLoc, because the mouse may have
	 * moved between the mouseDown and now (i.e., during FindAccidental).
	 */
	diffH = pt.h - accOriginH;
	GetPaperMouse(&oldPt, &doc->currentPaper);
	
	if (StillDown()) while (WaitMouseUp()) {
		GetPaperMouse(&newPt, &doc->currentPaper);
		if (EqualPt(newPt, oldPt)) continue;
		
		DrawAcc(doc, &context, noteL, xdNorm, noteYD,
							FALSE, sizePercent, chordNoteToL);		/* erase old accidental */
		
		/* This is the reverse of what AccXOffset does. That is, we have the DDIST
		 * offset <xOffset> from the left edge of the note to the accidental origin.
		 * We need to map that into one of the 32 values that <xmoveAcc> can have.
		 */		
		dAccWidth = std2d(STD_ACCWIDTH, context.staffHeight, context.staffLines);		
		xOffset = xdNorm - p2d(newPt.h - diffH);
		if (chordNoteToL)
			xOffset -= SizePercentSCALE(HeadWidth(LNSPACE(&context)));
		if (aNote->small) {
				LONGDDIST xOffsetLong;					/* to avoid DDIST overflow */			
				xOffsetLong = (LONGDDIST)xOffset;
				xmoveAcc = (((((DDIST)((xOffsetLong*100L)/(long)sizePercent)) 
										 - dAccWidth) * 4) / dAccWidth) + DFLT_XMOVEACC;
		}
		else
			xmoveAcc = (((xOffset - dAccWidth) * 4) / dAccWidth) + DFLT_XMOVEACC;
		if (aNote->accident==AC_DBLFLAT)
			xmoveAcc -= 2;							/* ??this isn't quite right */

		if (xmoveAcc >= 0 && xmoveAcc <= 31)
			aNote->xmoveAcc = xmoveAcc;
		else {										/* if outside limits, constrain to limits */
			if (xmoveAcc < 0)
				aNote->xmoveAcc = 0;
			else
				aNote->xmoveAcc = 31;
		}
					
		aNote->accident = 0;			/* temporarily remove this so AutoScroll won't leave accidental turds */
		TextMode(srcOr);				/* so staff lines won't cut through notes */
		AutoScroll();
		TextMode(srcXor);
		TextSize(useTxSize);			/* this might be changed in AutoScroll */
		aNote->accident = accCode;

		/* Now translate our newly-acquired offset into DDISTs before drawing. */
		accXOffset = GetAccXOffset(aNote, sizePercent, &context);
		DrawAcc(doc, &context, noteL, xdNorm, noteYD,
							FALSE, sizePercent, chordNoteToL);		/* draw new accidental */
		
		/* Draw new params in msg box. (Do this after drawing accidental at new pos to reduce flicker.) */
		DrawAccidentalParams(doc, aNote->xmoveAcc);
		
		oldPt = newPt;
	}

	TextMode(srcOr);
	DrawAcc(doc, &context, noteL, xdNorm, noteYD,
							FALSE, sizePercent, chordNoteToL);		/* draw new accidental in normal mode */
	TextSize(oldTxSize);
	TextMode(oldTxMode);

	/* Update modNR in data structure and inval rect if it's changed. */
	if (BlockCompare(aNote, &oldNote, sizeof(ANOTE))) {
		doc->changed = TRUE;
		LinkTWEAKED(syncL) = TRUE;									/* Flag to show node was edited */
		
		Rect2Window(doc, origAccBBox);
		InsetRect(origAccBBox, -1, -4);
		if (!suppressRedraw)
			EraseAndInval(origAccBBox);				/* ??NB: if bbox entirely outside of systemRect, it won't be redrawn */
	}
	else
		*aNote = oldNote;
	MEHideCaret(doc);

PopLock(OBJheap);
PopLock(NOTEheap);
}


DDIST GetAccXOffset(PANOTE aNote, INT16 sizePercent, PCONTEXT pContext)
{
	DDIST	accXOffset;
	short	xmoveAcc;
	
	xmoveAcc = (aNote->accident==AC_DBLFLAT? aNote->xmoveAcc+2 : aNote->xmoveAcc);
	accXOffset = SizePercentSCALE(AccXOffset(xmoveAcc, pContext));
	
	return accXOffset;
}


/* Draws the offset (negative) of the accidental from the notehead into the msg box. */

static void DrawAccidentalParams(Document *doc, INT16 xmoveAcc)
{
	char str[256], fmtStr[256];
	
	GetIndCString(fmtStr, DIALOG_STRS, 10);			/* "horizontal offset = %d" */
	sprintf(str, fmtStr, -xmoveAcc);						/* units are actually STD_ACCWIDTH/4 */
	DrawMessageString(doc, str);
}


/* ============================================================ modNR functions == */

/* Returns box surrounding the modNR in paper-rel coordinates. */

static Boolean GetModNRbbox(Document *doc, LINK syncL, LINK noteL, LINK modNRL,
										Rect *bbox)
{
	DDIST		noteXD, xdMod, ydMod, staffTop, xdNorm, lnSpace;
	INT16		code, oldTxSize, useTxSize, sizePercent,
				xOffset, yOffset, charWid, baseLine, wid, ht;
	PAMODNR	aModNR;
	PANOTE	aNote;
	CONTEXT	context;
	unsigned char glyph;
	Rect		glyphRect;
	
	GetContext(doc, syncL, NoteSTAFF(noteL), &context);
	staffTop = context.staffTop;
	
	aNote = GetPANOTE(noteL);
	noteXD = NoteXLoc(syncL, noteL, context.measureLeft, HeadWidth(LNSPACE(&context)), &xdNorm);

	aModNR = GetPAMODNR(modNRL);
	code = aModNR->modCode;
	xdMod = noteXD + std2d(aModNR->xstd-XSTD_OFFSET, context.staffHeight, context.staffLines);
	ydMod = std2d(aModNR->ystdpit, context.staffHeight, context.staffLines);

	if (!GetModNRInfo(code, aNote->subType, aNote->small,
							(ydMod<=aNote->yd), &glyph, &xOffset, &yOffset, &sizePercent)) {
		MayErrMsg("GetModNRbbox: illegal MODNR code %ld for note link=%ld",
					(long)code, (long)noteL);
		return FALSE;
	}
   if (glyph==' ') return FALSE;			/* if it's a tremolo slash, can't drag it (yet) */

	xdMod += (LNSPACE(&context)/8)*xOffset;
	ydMod += (LNSPACE(&context)/8)*yOffset;

	glyph = MapMusChar(doc->musFontInfoIndex, glyph);
	lnSpace = LNSPACE(&context);
	xdMod += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	ydMod += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

	oldTxSize = GetPortTxSize();
	useTxSize = UseMTextSize(context.fontSize, doc->magnify);
	if (aNote->small) useTxSize = SMALLSIZE(useTxSize);
	TextSize(useTxSize);
	BuildCharRectCache(doc);				/* current cache may not be valid for this modifier's size */
	charWid = CharWidth(glyph);			/* more accurate for screen fonts, especially when scaled */
	TextSize(oldTxSize);

	glyphRect = CharRect((INT16)glyph);

	bbox->left = d2p(xdMod);
	bbox->right = bbox->left + charWid;
	baseLine = d2p(staffTop + ydMod);
	bbox->bottom = baseLine + glyphRect.bottom;
	bbox->top = baseLine + glyphRect.top - 1;		/* this still doesn't always enclose the char... */
	
	bbox->top -= 2;										/* so make it taller */
	bbox->bottom += 2;
		
	/* add more slop if the rect is very small */
	wid = bbox->right - bbox->left;
	ht = bbox->bottom - bbox->top;
	InsetRect(bbox, wid<6? -2 : 0, ht<6? -2 : 0);

	return TRUE;
}


/* Handle dragging the modifier. */
/* ??NB: Shouldn't let user drag it off the page! */

static enum {
	NOCONSTRAIN = 0,
	H_CONSTRAIN,
	V_CONSTRAIN
} E_ModNRConstrainItems;

static void DoModNRDrag(Document *doc, Point pt, LINK syncL, LINK noteL, LINK modNRL,
													Rect *origModNRbbox)		/* in paper coords */
{
	DDIST		noteXD, xdMod, ydMod, staffTop, xdNorm, xd, yd, xdModOrig, ydModOrig, lnSpace;
	STDIST	xstd, ystdpit;
	INT16		code, sizePercent, xOffset, yOffset, constrain = NOCONSTRAIN,
				oldTxSize, useTxSize, oldTxMode;
	Point		oldPt, newPt, modOrigin, diffPt;
	Boolean	firstLoop = TRUE, suppressRedraw = FALSE;
	PAMODNR	aModNR;
	AMODNR	oldModNR;
	PANOTE	aNote;
	CONTEXT	context;
	unsigned char glyph;
#ifdef USE_BITMAP
	GrafPtr	scrnPort, modPort;
	Rect		destRect;
	Point		grPortOrigin;
	INT16		dh, dv, fontID;
#endif
	
/* ??Do I really need to lock these? What about the modNR heaps? */
PushLock(OBJheap);
PushLock(NOTEheap);

	GetContext(doc, syncL, NoteSTAFF(noteL), &context);
	staffTop = context.staffTop;
	
	aNote = GetPANOTE(noteL);
	noteXD = NoteXLoc(syncL, noteL, context.measureLeft, HeadWidth(LNSPACE(&context)), &xdNorm);

	aModNR = GetPAMODNR(modNRL);
	oldModNR = *aModNR;
	code = aModNR->modCode;
	xstd = aModNR->xstd - XSTD_OFFSET;
	ystdpit = aModNR->ystdpit;
	xdMod = noteXD + std2d(xstd, context.staffHeight, context.staffLines);
	ydMod = std2d(ystdpit, context.staffHeight, context.staffLines);
	if (!GetModNRInfo(code, aNote->subType, aNote->small,
							(ydMod<=aNote->yd), &glyph, &xOffset, &yOffset, &sizePercent))
		goto Cleanup;

	lnSpace = LNSPACE(&context);
	xdMod += (lnSpace/8)*xOffset;
	ydMod += (lnSpace/8)*yOffset;

	xdModOrig = xdMod;
	ydModOrig = ydMod;
	glyph = MapMusChar(doc->musFontInfoIndex, glyph);
	xdMod += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	ydMod += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

	oldTxSize = GetPortTxSize();
	useTxSize = UseMTextSize(context.fontSize, doc->magnify);
	if (aNote->small) useTxSize = SMALLSIZE(useTxSize);
	TextSize(useTxSize);
	oldTxMode = GetPortTxMode();
//	oldTxMode = qd.thePort->txMode;

#define DRAW_IN_GRAY
#ifdef DRAW_IN_GRAY
	TextMode(srcXor);
	Draw1ModNR(doc, xdMod, ydMod, code, glyph, 
					&context, sizePercent, FALSE);			/* erase original modNR */
	TextMode(srcOr);
	Draw1ModNR(doc, xdMod, ydMod, code, glyph, 
					&context, sizePercent, TRUE);				/* draw it in gray */
	TextMode(srcXor);
#endif

	DrawModNRParams(doc, aModNR->xstd-XSTD_OFFSET, aModNR->ystdpit);

	SetPt(&modOrigin, d2p(xdModOrig), d2p(ydModOrig + staffTop));
	SetPt(&diffPt, pt.h - modOrigin.h, pt.v - modOrigin.v);	/* set from pt, before the search and drawing in gray, etc. */
	
#ifdef USE_BITMAP
	modPort = NewGrafPort(origModNRbbox->right-origModNRbbox->left,
									origModNRbbox->bottom-origModNRbbox->top);
	if (modPort==NULL)
		return;

	GetPort(&scrnPort);
	fontID = (qd.thePort)->txFont;
	SetPort(modPort);
	TextFont(fontID);
	TextSize(useTxSize);

	/* Fiddle with bitmap coordinate system so that Draw1ModNR will draw into our tiny rect. */
	grPortOrigin = TOP_LEFT(*origModNRbbox);
	Pt2Window(doc, &grPortOrigin);
	SetOrigin(grPortOrigin.h, grPortOrigin.v);
	ClipRect(&(*modPort).portBits.bounds);

	Draw1ModNR(doc, xdMod, ydMod, code, glyph, &context, sizePercent, FALSE);

	SetPort(scrnPort);

	destRect = *origModNRbbox;
	Rect2Window(doc, &destRect);
#endif

	GetPaperMouse(&oldPt, &doc->currentPaper);

	if (StillDown()) while (WaitMouseUp()) {
		GetPaperMouse(&newPt, &doc->currentPaper);
		if (EqualPt(newPt, oldPt)) continue;
		
		if (firstLoop) {
			if (ShiftKeyDown())
				constrain = (ABS(newPt.h-oldPt.h) >= ABS(newPt.v-oldPt.v)) ?
												H_CONSTRAIN : V_CONSTRAIN;
			firstLoop = FALSE;
		}
		
#ifdef USE_BITMAP
		CopyBits(&modPort->portBits, &scrnPort->portBits,		/* erase old modNR */
		 			&(*modPort).portBits.bounds, &destRect, srcXor, NULL);
#else
		Draw1ModNR(doc, xdMod, ydMod, code, glyph, 
						&context, sizePercent, FALSE);		/* erase old modNR */
#endif
		
		xd = p2d(newPt.h - diffPt.h);
		xstd = d2std(xd - noteXD - ((LNSPACE(&context)/8)*xOffset),
								context.staffHeight, context.staffLines) + XSTD_OFFSET;
		yd = p2d(newPt.v - diffPt.v) - staffTop;
		ystdpit = d2std(yd - ((LNSPACE(&context)/8)*yOffset), context.staffHeight, context.staffLines);

		if (constrain != V_CONSTRAIN) {
			if (xstd >= -16+XSTD_OFFSET && xstd <= 15+XSTD_OFFSET)
				aModNR->xstd = xstd;
			else {										/* if outside limits, constrain to limits */
				if (xstd < -16+XSTD_OFFSET)
					aModNR->xstd = -16+XSTD_OFFSET;
				else
					aModNR->xstd = 15+XSTD_OFFSET;
			}
		}
		if (constrain != H_CONSTRAIN) {
			if (ystdpit >= -128 && ystdpit <= 127)
				aModNR->ystdpit = ystdpit;
			else {
				if (ystdpit < -128)
					aModNR->ystdpit = -128;
				else
					aModNR->ystdpit = 127;
			}
		}
		
		xdMod = noteXD + std2d(aModNR->xstd-XSTD_OFFSET, context.staffHeight, context.staffLines);
		ydMod = std2d(aModNR->ystdpit, context.staffHeight, context.staffLines);
		xdMod += (LNSPACE(&context)/8)*xOffset;
		ydMod += (LNSPACE(&context)/8)*yOffset;
		
#if 0
		/* To keep AutoScroll from leaving turds on the screen when it tries to
		 * draw our modNR, temporarily set the modNR's code to an unused value,
		 * so that Draw1ModNR won't do anything. For this purpose I put MOD_NULL
		 * in the modCode enum (NTypes.h), and made DrawModNR ignore it.
		 */
		aModNR->modCode = MOD_NULL;
		TextMode(srcOr);						/* so staff lines won't cut through notes */
		AutoScroll();
		TextMode(srcXor);
		TextSize(useTxSize);					/* this might be changed in AutoScroll */
		aModNR->modCode = code;
#endif

		/* Some symbols (like fermata) have different glyphs, depending on whether
		 * modNR is above or below its note, so update glyph, etc. now.
		 */
		GetModNRInfo(code, aNote->subType, aNote->small, (ydMod<=aNote->yd),
						&glyph, &xOffset, &yOffset, &sizePercent);
		ydMod = std2d(aModNR->ystdpit, context.staffHeight, context.staffLines);
		ydMod += (LNSPACE(&context)/8)*yOffset;
		glyph = MapMusChar(doc->musFontInfoIndex, glyph);
		xdMod += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
		ydMod += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

#ifdef USE_BITMAP
		/* shift on-screen destination rect */
// Problem here is that this doesn't take into account the constraints applied
// to the modNR position above.
// Also, we're not able to flip a mod (e.g. fermata) when it crosses the staff midpoint!
		dh = newPt.h - oldPt.h;
		dv = newPt.v - oldPt.v;
		destRect.top += dv;		destRect.bottom += dv;
		destRect.left += dh;		destRect.right += dh;

		CopyBits(&modPort->portBits, &scrnPort->portBits,		/* draw new modNR */
		 			&(*modPort).portBits.bounds, &destRect, srcXor, NULL);
#else
		Draw1ModNR(doc, xdMod, ydMod, code, glyph, 
						&context, sizePercent, FALSE);	/* draw new modNR */
#endif		

		/* Draw new params in msg box. (Do this after drawing modNR at new pos to reduce flicker.) */
		DrawModNRParams(doc, aModNR->xstd-XSTD_OFFSET, aModNR->ystdpit);
		
		oldPt = newPt;
	}

#ifdef USE_BITMAP
	Draw1ModNR(doc, xdMod, ydMod, code, glyph, 
					&context, sizePercent, FALSE);			/* draw final black modNR */
	DisposGrafPort(modPort);
#else
	TextMode(srcOr);
	Draw1ModNR(doc, xdMod, ydMod, code, glyph, 
					&context, sizePercent, FALSE);			/* draw new modNR in normal mode */
#endif		

	TextMode(oldTxMode);
	TextSize(oldTxSize);
	
	/* Update modNR in data structure and inval rect if it's changed. */
	if (BlockCompare(aModNR, &oldModNR, sizeof(AMODNR))) {
		doc->changed = TRUE;
		LinkTWEAKED(syncL) = TRUE;									/* Flag to show node was edited */
		
		Rect2Window(doc, origModNRbbox);
		InsetRect(origModNRbbox, -1, -4);
		if (!suppressRedraw)
			EraseAndInval(origModNRbbox);
	}
	else
		*aModNR = oldModNR;
	MEHideCaret(doc);

#ifdef USE_BITMAP
	EraseAndInval(&destRect);
#endif
	
Cleanup:
PopLock(OBJheap);
PopLock(NOTEheap);
}


/* Draws the (note-rel) x and (staffTop-rel) y values of a modNR into the msg box. */

static void DrawModNRParams(Document *doc, SHORTSTD xstd, SHORTSTD ystd)
{
	char str[256], fmtStr[256];
	
	GetIndCString(fmtStr, DIALOG_STRS, 6);				/* "x = %d,  y = %d     (8th spaces)" */
	sprintf(str, fmtStr, (short)xstd, (short)ystd);
	DrawMessageString(doc, str);
}
