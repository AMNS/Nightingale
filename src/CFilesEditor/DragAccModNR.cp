/* DragAccModNR.c (formerly AccModNRMouse.c) for Nightingale - handle dragging
accidentals and note modifiers. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static short FindAccModNR(Document *, Point, LINK *, LINK *, LINK *, Rect *);
static void GetAccidentalBbox(Document *, LINK, LINK, Rect *);
static void DragAccidental(Document *, Point, LINK, LINK, Rect *);
static DDIST GetAccXOffset(PANOTE, short, PCONTEXT);
static void ShowAccidentalParams(Document *doc, short);
static Boolean GetModNRBbox(Document *, LINK, LINK, LINK, Rect *);
static void DragModNR(Document *, Point, LINK, LINK, LINK, Rect *);
static void ShowModNRParams(Document *doc, SHORTSTD, SHORTSTD);

enum {			/* return values for FindAccModNR */
	NOHIT=0,
	MODNR,
	ACCIDENTAL
};

//#define USE_BITMAP	/* use offscreen bitmap when dragging; otherwise use srcXor drawing mode */


static short FindAccModNR(Document *doc, Point pt,
						LINK *syncL, LINK *noteL,
						LINK *modNRL,			/* ignored if we hit an accidental */
						Rect *bbox				/* paper coords */
						)
{
	LINK	pL, pageL, startL, endL, firstMeasL, measL, aNoteL, aModNRL;
	PANOTE	aNote;
	short	i, aNoteAcc;
		
	pageL = GetCurrentPage(doc);
	endL = RightLINK(LastObjOnPage(doc, pageL));
	
	/* Find the measure the click is in. Note that this measure may not contain the
	   bbox of the symbol (modNR or accidental) we've clicked on, if that symbol isn't
	   inside its system rect, or simply isn't inside its measure. If the symbol ia
	   BELOW its system rect or is in the measure to the RIGHT of the one its note is
	   in, we'll still eventually reach the symbol's note. But if our search finds nothing,
	   we must start again at the top of the page and search to the original startL, just
	   in case the user clicked on a symbol that's ABOVE its system rect or is in the
	   measure to the LEFT of its note's measure. */
	   
	startL = NILINK;
	firstMeasL = LSSearch(pageL, MEASUREtype, ANYONE, GO_RIGHT, False);
	for (measL = firstMeasL; SamePage(measL, firstMeasL); measL = LinkRMEAS(measL)) {
		if (IsPtInMeasure(doc, pt, measL)) {
			startL = measL;
			break;
		}
	}
	if (!startL) startL = pageL;					/* in case click is outside of any measure rect */
	
	for (i=0; i<2; i++) {									/* may have to search twice */
		for (pL=startL ; pL!=endL; pL=RightLINK(pL)) {		/* find object that was clicked on */
			if (ObjLType(pL) == SYNCtype) {
				if (VISIBLE(pL)) {
					for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (!LOOKING_AT(doc, aNote->voice)) continue;
						aModNRL = aNote->firstMod;
						aNoteAcc = aNote->accident;
						if (aModNRL) {
							for ( ; aModNRL; aModNRL = NextMODNRL(aModNRL)) {
								if (!GetModNRBbox(doc, pL, aNoteL, aModNRL, bbox))
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
		
		/* If we've reached this point, the first search found nothing, so we must start
		   over at the top of the page. */
		   
		if (startL == pageL) return NOHIT;			/* we've already searched the whole page */
		endL = startL;
		startL = pageL;
	}
	
	return NOHIT;
}


/* If user double-clicked on a modNR, invoke a dialog to let them change the modifier and
return True; otherwise, just return False. FIXME: This function belongs in a different
file; therefore so does FindAccModNR(), */

Boolean DoOpenModNR(Document *doc, Point pt)
{
	LINK	syncL, noteL, modNRL;
	short	result;
	Byte	modCode;
	Boolean change;
	Rect	bbox, bboxOld, bboxNew;
	
	result = FindAccModNR(doc, pt, &syncL, &noteL, &modNRL, &bboxOld);
	
	if (result==MODNR) {
		DisableUndo(doc, False);
		modCode = ModNRMODCODE(modNRL);
		change = ModNRDialog(SETMOD_DLOG, &modCode);
		if (change) {
			ModNRMODCODE(modNRL) = modCode;

			if (GetModNRBbox(doc, syncL, noteL, modNRL, &bboxNew))
				UnionRect(&bboxOld, &bboxNew, &bbox);
			else
				bbox = bboxNew;

			Rect2Window(doc, &bbox);
			InsetRect(&bbox, -1, -4);
			EraseAndInval(&bbox);				/* ??NB: if bbox entirely outside of systemRect, it won't be redrawn */

			doc->changed = True;
		}
		return True;
	}

	return False;
}


/* If user clicked on a modNR or an accidental, call functions to handle dragging and
returns True; otherwise, just return False. */

Boolean DoAccModNRClick(Document *doc, Point pt)
{
	LINK	syncL, noteL, modNRL;
	short	result;
	Rect	bbox;
	
	result = FindAccModNR(doc, pt, &syncL, &noteL, &modNRL, &bbox);
	
	if (result==MODNR) {
		DisableUndo(doc, False);
		DragModNR(doc, pt, syncL, noteL, modNRL, &bbox);
		return True;
	}
	if (result==ACCIDENTAL) {
		DisableUndo(doc, False);
		DragAccidental(doc, pt, syncL, noteL, &bbox);
		return True;
	}
	
	return False;
}


/* ============================================================== Accidental functions == */

#define ADD_SLOP_THRESH 6	/* in pixels */
#define SLOP_TO_ADD 2		/* in pixels */

/* Return box surrounding the accidental in paper-relative coordinates. Much of the code
is borrowed from DrawACC. */

static void GetAccidentalBbox(Document *doc, LINK syncL, LINK noteL, Rect *accBBox)
{
	DDIST	noteXD, noteYD, accXOffset, xdNorm;
	short	oldTxSize, useTxSize, sizePercent, charWid, baseLine;
	PANOTE	aNote;
	CONTEXT	context;
	Rect	accCharRect;
	Boolean	chordNoteToL;
	short	accidentalChar, wid, ht;
	DDIST	d8thSp, dIncreaseWid;
	
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
	BuildCharRectCache(doc);				/* current cache may not be valid for this accidental's size */
	charWid = CharWidth(accidentalChar);	/* more accurate for screen fonts, especially when scaled */
	TextSize(oldTxSize);

	accCharRect = CharRect(accidentalChar);

	accBBox->left = d2p(xdNorm) - d2p(accXOffset);
	accBBox->right = accBBox->left + charWid;

	/* If it's a courtesy accidental, increase width to allow for the parentheses. This
	   code doesn't take into account <small> accidentals, whose parens are closer to
	   the accidental; it shouldn't make much difference, though. */
	   
	aNote = GetPANOTE(noteL);
	if (aNote->courtesyAcc) {
		d8thSp = LNSPACE(&context)/8;
		dIncreaseWid = (config.courtesyAccLXD+config.courtesyAccRXD)*d8thSp;
		accBBox->left -= d2p(dIncreaseWid);
	}

	baseLine = d2p(noteYD);
	accBBox->bottom = baseLine + accCharRect.bottom;
	accBBox->top = baseLine + accCharRect.top - 1;		/* this still doesn't always enclose the char... */
	
	accBBox->top -= 3;									/* ...so make it taller */
	accBBox->bottom += 2;
	
	/* Add more slop if the rect is very small */
	
	wid = accBBox->right - accBBox->left;
	ht = accBBox->bottom - accBBox->top;
	InsetRect(accBBox, wid<6? -2 : 0, ht<6? -2 : 0);
}


static void DragAccidental(Document *doc, Point pt, LINK syncL, LINK noteL,
									Rect *origAccBBox)				/* in paper coords */
{
	DDIST	noteXD, noteYD, xdNorm, xdNormAdjusted, xOffset, accXOffset, dAccWidth;
	short	accCode, oldTxMode, oldTxSize, useTxSize, 
			sizePercent, diffH, accOriginH, xmoveAcc;
	Point	oldPt, newPt;
	Boolean	chordNoteToL, suppressRedraw = False;
	ANOTE	oldNote;	
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
	oldTxSize = GetPortTxSize();
	useTxSize = UseMTextSize(context.fontSize, doc->magnify);
	if (aNote->small) useTxSize = SMALLSIZE(useTxSize);
	TextSize(useTxSize);
	
	TextMode(srcXor);
	DrawAcc(doc, &context, noteL, xdNorm, noteYD,
						False, sizePercent, chordNoteToL);		/* erase original accidental */
	TextMode(srcOr);
	DrawAcc(doc, &context, noteL, xdNorm, noteYD,
						True, sizePercent, chordNoteToL);		/* draw it in gray */
	TextMode(srcXor);

	ShowAccidentalParams(doc, aNote->xmoveAcc);					/* display this now in case we never drag */

	/* Adjust xdNorm for chordNoteToL notes. Don't change xdNorm: DrawAcc already does
	   that for us. */
	   
	xdNormAdjusted = chordNoteToL?
			xdNorm-SizePercentSCALE(HeadWidth(LNSPACE(&context))) : xdNorm;
	accOriginH = d2p(xdNormAdjusted) - d2p(accXOffset);
	
	/* <diffH> is the distance between the point where the user clicked and the origin
	   of the accidental. Set it from <pt> rather than from the current mouseLoc
	   because the mouse may have moved between the mouseDown and now (i.e., during
	   FindAccidental). */
	   
	diffH = pt.h - accOriginH;
	GetPaperMouse(&oldPt, &doc->currentPaper);
	
	/* We're ready to go. This loop handles the actual dragging. */
	
	if (StillDown()) while (WaitMouseUp()) {
		GetPaperMouse(&newPt, &doc->currentPaper);
		if (EqualPt(newPt, oldPt)) continue;
		
		DrawAcc(doc, &context, noteL, xdNorm, noteYD, False,
							sizePercent, chordNoteToL);		/* erase old accidental */
		
		/* Do the reverse of what GetAccXOffset does. That is, we have the DDIST offset
		   <xOffset> from the left edge of the note to the accidental origin, and we map
		   it into one of the 32 values that <xmoveAcc> can have. */
		   		
		dAccWidth = std2d(STD_ACCWIDTH, context.staffHeight, context.staffLines);		
		xOffset = xdNorm - p2d(newPt.h - diffH);
		if (chordNoteToL)
			xOffset -= SizePercentSCALE(HeadWidth(LNSPACE(&context)));
		if (aNote->small) {
				LONGDDIST xOffsetLong;							/* to avoid DDIST overflow */			
				xOffsetLong = (LONGDDIST)xOffset;
				xmoveAcc = (((((DDIST)((xOffsetLong*100L)/(long)sizePercent)) 
										- dAccWidth) * 4) / dAccWidth) + DFLT_XMOVEACC;
		}
		else
			xmoveAcc = (((xOffset - dAccWidth) * 4) / dAccWidth) + DFLT_XMOVEACC;
		if (aNote->accident==AC_DBLFLAT)
			xmoveAcc -= 2;									/* FIXME: this isn't quite right */

		if (xmoveAcc >= 0 && xmoveAcc <= 31)
			aNote->xmoveAcc = xmoveAcc;
		else {												/* if outside limits, constrain to limits */
			if (xmoveAcc < 0)
				aNote->xmoveAcc = 0;
			else
				aNote->xmoveAcc = 31;
		}
					
		aNote->accident = 0;					/* temporarily remove it so AutoScroll won't leave accidental junk */
		TextMode(srcOr);						/* so staff lines won't cut through notes */
		AutoScroll();
		TextMode(srcXor);
		TextSize(useTxSize);					/* this might be changed in AutoScroll */
		aNote->accident = accCode;

		/* Now translate our newly-acquired offset into DDISTs before drawing. */
		
		accXOffset = GetAccXOffset(aNote, sizePercent, &context);
		DrawAcc(doc, &context, noteL, xdNorm, noteYD,
							False, sizePercent, chordNoteToL);	/* draw new accidental */
		
		/* Draw new params in msg box (after drawing accidental at new pos to reduce flicker). */
		
		ShowAccidentalParams(doc, aNote->xmoveAcc);
		
		oldPt = newPt;
	}

	TextMode(srcOr);
	DrawAcc(doc, &context, noteL, xdNorm, noteYD, False,
							sizePercent, chordNoteToL);		/* draw new accidental in normal mode */
	TextSize(oldTxSize);
	TextMode(oldTxMode);

	/* Update modNR in object list, and inval rect if it's changed. */
	
	if (BlockCompare(aNote, &oldNote, sizeof(ANOTE))) {
		doc->changed = True;
		LinkTWEAKED(syncL) = True;							/* Flag to show node was edited */
		
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


DDIST GetAccXOffset(PANOTE aNote, short sizePercent, PCONTEXT pContext)
{
	DDIST	accXOffset;
	short	xmoveAcc;
	
	xmoveAcc = (aNote->accident==AC_DBLFLAT? aNote->xmoveAcc+2 : aNote->xmoveAcc);
	accXOffset = SizePercentSCALE(AccXDOffset(xmoveAcc, pContext));
	
	return accXOffset;
}


/* Draws the offset (negative) of the accidental from the notehead into the msg box. */

static void ShowAccidentalParams(Document *doc, short xmoveAcc)
{
	char str[256], fmtStr[256];
	
	GetIndCString(fmtStr, DIALOG_STRS, 10);			/* "horizontal offset = %d" */
	sprintf(str, fmtStr, -xmoveAcc);				/* units are actually STD_ACCWIDTH/4 */
	DrawMessageString(doc, str);
}


/* =================================================================== modNR functions == */

/* Return box surrounding the modNR in paper-relative coordinates. */

static Boolean GetModNRBbox(Document *doc, LINK syncL, LINK noteL, LINK modNRL, Rect *bbox)
{
	DDIST		noteXD, xdMod, ydMod, staffTop, xdNorm, lnSpace;
	short		code, oldTxSize, useTxSize, sizePercent, xOffset, yOffset,
				charWid, baseLine, wid, ht;
	PAMODNR		aModNR;
	PANOTE		aNote;
	CONTEXT		context;
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

	if (!GetModNRInfo(code, aNote->subType, aNote->small, (ydMod<=aNote->yd),
									&glyph, &xOffset, &yOffset, &sizePercent)) {
		MayErrMsg("GetModNRBbox: illegal MODNR code %ld in voice %d, note/rest L%ld",
					(long)code, NoteVOICE(noteL), (long)noteL);
		return False;
	}
   if (glyph==' ') return False;			/* if it's a tremolo slash, can't drag it */

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
	charWid = CharWidth(glyph);				/* more accurate for screen fonts, especially when scaled */
	TextSize(oldTxSize);

	glyphRect = CharRect((short)glyph);

	bbox->left = d2p(xdMod);
	bbox->right = bbox->left + charWid;
	baseLine = d2p(staffTop + ydMod);
	bbox->bottom = baseLine + glyphRect.bottom;
	bbox->top = baseLine + glyphRect.top - 1;	/* this still doesn't always enclose the char. */
	bbox->top -= 2;								/* ...so make it taller */
	bbox->bottom += 2;
		
	/* Add more slop if the rect is very small */
	
	wid = bbox->right - bbox->left;
	ht = bbox->bottom - bbox->top;
	InsetRect(bbox, (wid<ADD_SLOP_THRESH? -SLOP_TO_ADD : 0),
							(ht<ADD_SLOP_THRESH? -SLOP_TO_ADD : 0));

	return True;
}


/* Handle dragging the modifier. */
/* FIXME: Shouldn't let user drag it off the page! */

enum {
	NOCONSTRAIN = 0,
	H_CONSTRAIN,
	V_CONSTRAIN
};

static void DragModNR(Document *doc, Point pt, LINK syncL, LINK noteL, LINK modNRL,
							Rect *origModNRBbox)					/* in paper coords */
{
	DDIST	noteXD, xdMod, ydMod, staffTop, xdNorm, xd, yd, xdModOrig, ydModOrig, lnSpace;
	STDIST	xstd, ystdpit;
	short	code, sizePercent, xOffset, yOffset, constrain = NOCONSTRAIN,
			oldTxSize, useTxSize, oldTxMode;
	Point	oldPt, newPt, modOrigin, diffPt;
	Boolean	firstLoop = True, suppressRedraw = False;
	PAMODNR	aModNR;
	AMODNR	oldModNR;
	PANOTE	aNote;
	CONTEXT	context;
	unsigned char glyph;
	
/* ??Do I really need to lock OBJheap? What about the modNR heap? */

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
	if (!GetModNRInfo(code, aNote->subType, aNote->small, (ydMod<=aNote->yd),
							&glyph, &xOffset, &yOffset, &sizePercent))
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

#define DRAW_IN_GRAY
#ifdef DRAW_IN_GRAY
	TextMode(srcXor);
	Draw1ModNR(doc, xdMod, ydMod, code, glyph, &context,
					sizePercent, False);					/* erase original modNR */
	TextMode(srcOr);
	Draw1ModNR(doc, xdMod, ydMod, code, glyph, &context, 
					sizePercent, True);						/* ...and redraw it in gray */
	TextMode(srcXor);
#endif

	ShowModNRParams(doc, aModNR->xstd-XSTD_OFFSET, aModNR->ystdpit);

	SetPt(&modOrigin, d2p(xdModOrig), d2p(ydModOrig + staffTop));
	SetPt(&diffPt, pt.h - modOrigin.h, pt.v - modOrigin.v);	/* set from pt, before the search and drawing in gray, etc. */
	
	GetPaperMouse(&oldPt, &doc->currentPaper);

	/* We're ready to go. This loop handles the actual dragging. */
	
	if (StillDown()) while (WaitMouseUp()) {
		GetPaperMouse(&newPt, &doc->currentPaper);
		if (EqualPt(newPt, oldPt)) continue;
		
		if (firstLoop) {
			if (ShiftKeyDown())
				constrain = (ABS(newPt.h-oldPt.h) >= ABS(newPt.v-oldPt.v)) ?
												H_CONSTRAIN : V_CONSTRAIN;
			firstLoop = False;
		}
		
		Draw1ModNR(doc, xdMod, ydMod, code, glyph, &context,
						sizePercent, False);				/* erase modNR at old position */
		
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
		
		/* Some symbols (like fermata) have different glyphs depending on whether
		   modNR is above or below its note, so update glyph, etc. now. */
		   
		GetModNRInfo(code, aNote->subType, aNote->small, (ydMod<=aNote->yd),
						&glyph, &xOffset, &yOffset, &sizePercent);
		ydMod = std2d(aModNR->ystdpit, context.staffHeight, context.staffLines);
		ydMod += (LNSPACE(&context)/8)*yOffset;
		glyph = MapMusChar(doc->musFontInfoIndex, glyph);
		xdMod += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
		ydMod += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);

		Draw1ModNR(doc, xdMod, ydMod, code, glyph, &context, 
						sizePercent, False);					/* draw modNR in new position */

		/* Draw new params in msg box. (Do this after drawing modNR at new position to
		   reduce flicker.) */
		   
		ShowModNRParams(doc, aModNR->xstd-XSTD_OFFSET, aModNR->ystdpit);
		
		oldPt = newPt;
	}

	TextMode(srcOr);
	Draw1ModNR(doc, xdMod, ydMod, code, glyph, &context, sizePercent,
					 False);									/* draw in final position in normal mode */

	TextMode(oldTxMode);
	TextSize(oldTxSize);
	
	/* Update modNR in object list, and inval rect if it's changed. */
	
	if (BlockCompare(aModNR, &oldModNR, sizeof(AMODNR))) {
		doc->changed = True;
		LinkTWEAKED(syncL) = True;								/* Flag to show node was edited */
		
		Rect2Window(doc, origModNRBbox);
		InsetRect(origModNRBbox, -1, -4);
		if (!suppressRedraw) EraseAndInval(origModNRBbox);
	}
	else
		*aModNR = oldModNR;
	MEHideCaret(doc);

Cleanup:
PopLock(OBJheap);
PopLock(NOTEheap);
}


/* Draw the (note-relative) x and (staffTop-relative) y values of a modNR into msg box. */

static void ShowModNRParams(Document *doc, SHORTSTD xstd, SHORTSTD ystd)
{
	char str[256], fmtStr[256];
	
	GetIndCString(fmtStr, DIALOG_STRS, 6);				/* "x = %d,  y = %d     (8th spaces)" */
	sprintf(str, fmtStr, (short)xstd, (short)ystd);
	DrawMessageString(doc, str);
}
