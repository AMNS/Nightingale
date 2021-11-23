/* ChordSym.c for Nightingale, by John Gibson */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include <ctype.h>					/* for isdigit() */
#include <math.h>					/* for rint() */

static Boolean ParseChordSym(StringPtr, char *, char *, char *, char *, char *, char *, char *);
static Boolean IsCSAcc(char *, unsigned char *);
static short Draw1Extension(Document *, char [], short, short, const unsigned char[], const unsigned char[],
								short, short, short, short, short, Boolean, Boolean, Boolean);
static void BuildCSstring(DialogPtr, unsigned char []);
static void GetCSOptions(DialogPtr, Boolean *);
static void CloseCSDialog(DialogPtr);
static Boolean AnyBadCSvalues(DialogPtr);
static void ShowHideParens(DialogPtr, Boolean);
static void DisplayChordSym(Document *, DialogPtr, Boolean);
static pascal Boolean ChordSymFilter(DialogPtr, EventRecord *, short *);
static Boolean CheckCSuserItems(DialogPtr, Point, short *);
static void DialogAdjustCursor(DialogPtr, EventRecord *, Boolean);

static ModalFilterUPP filterUPP;

#define MORE_ACCURATE_CHAR_WIDTH

#define MAX_FLDLEN	31				/* max number of chars in each (of 6) chord sym chunks */

/* Parse the chord symbol string <csStr> into several component strings, giving:
 * root name, chord quality, extensions that print from left to right after quality,
 * and extensions that print in a vertical stack at end of chord symbol. All strings
 * are allocated by caller. The C strings won't require anything like 256 bytes --
 * 32 should be more than enough. Returns True if parsed successfully; otherwise
 * returns False.
 * Note that if <csStr> contains no delimiters, we assume it's an old-style chord
 * symbol string, copy it into <rootStr> and return True.
 */

Boolean ParseChordSym(
			StringPtr csStr,	/* Pascal string stored by chord symbol graphic */
			char *rootStr,		/* receives note name C string (e.g., "C", "Bb") */
			char *qualStr,		/* receives quality C string (e.g., "Maj", "min", "dim") */
			char *extStr,		/* receives extension C string (e.g., "7", "∆7", "+6", "7#9") */
			char *extStk1Str,	/* receives (bottom) extension stack C string */
			char *extStk2Str,	/* receives (middle) extension stack C string */
			char *extStk3Str,	/* receives (top) extension stack C string */
			char *bassStr		/* receives bass note C string (e.g., "Cm7/Eb") */
			)
{
	register Size	len;
	register char	*p, *q;
	char			tempStr[256];
	
	*rootStr = *qualStr = *extStr = *extStk1Str = *extStk2Str = *extStk3Str = *bassStr = 0;

	len = *(unsigned char *)csStr;
	if (len==0) return False;
	
	/* copy graphic's Pascal string into local C string */
	
	BlockMove(&csStr[1], tempStr, len);
	tempStr[len] = 0;
	
	q = tempStr;
	p = strchr(tempStr, CS_DELIMITER);		/* p will point to first delimiter */
	if (p==NULL) {							/* if there's not one, it's an old-style chord symbol */
		strcpy(rootStr, tempStr);
		return True;						/* old-style is ok */
	}
	*p = 0;									/* terminate first chunk (replace delim with null) */
	strcpy(rootStr, q);						/* copy first chunk into rootStr */

	q = p+1;								/* q points to start of next chunk */
	p = strchr(q, CS_DELIMITER);			/* p will point to next delimiter */
	if (p==NULL) return False;				/* wrong number of delimiters */
	*p = 0;									/* terminate second chunk */
	strcpy(qualStr, q);
	
	q = p+1;								/* same for the other strings */
	p = strchr(q, CS_DELIMITER);
	if (p==NULL) return False;
	*p = 0;
	strcpy(extStr, q);
	
	q = p+1;
	p = strchr(q, CS_DELIMITER);
	if (p==NULL) return False;
	*p = 0;
	strcpy(extStk1Str, q);
	
	q = p+1;
	p = strchr(q, CS_DELIMITER);
	if (p==NULL) return False;
	*p = 0;
	strcpy(extStk2Str, q);
	
	q = p+1;
	p = strchr(q, CS_DELIMITER);
	if (p==NULL) return False;
	*p = 0;
	strcpy(extStk3Str, q);
	
	q = p+1;
	p = strchr(q, CS_DELIMITER);
	if (p) return False;						/* too many delimiters! */
	strcpy(bassStr, q);
	
	return True;
}


/* --------------------------------------------------------------------------- IsCSAcc -- */
/* Is <*p>, a character in the root portion of a chord symbol, an accidental? */

static Boolean IsCSAcc(char */*string*/,			/* C string: currently unused */
							unsigned char *p)		/* ptr to current char in string */
{
	/* If there's a previous char. in the root string and it's a letter name, recognize
	   all accidentals; otherwise recognize only flat and sharp. */
	   
	if (*(p-1)>='A' && *(p-1)<='G')
		return (Char2Acc(*p)>0);
	else
		return (*p==SonataAcc[AC_FLAT] || *p==SonataAcc[AC_SHARP]);
}


/* ---------------------------------------------------------------------- DrawChordSym -- */
/* Parse the given string as a sequence of 6 chunks of text and draw them according
 * to a (rather complicated) chord symbol formatting scheme:
 * 	Chunk			Format											Typical use
 * 	¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
 * 	rootStr		Chord symbol font and size;							Chord sym root,
 * 					accidentals in music font (e.g. Sonata),		harp pedals,
 * 					with size config.chordSymMusSize percent		harmony exam figs.
 * 					of chord symbol font; all accidentals
 * 					recognized for one char following upper case
 * 					root letters, else only flats and sharps.
 * 	qualStr		Chord symbol font, config.chordSymSmallSize			Chord quality
 * 					percent of chord symbol size; no accidentals
 * 					recognized. Digits superscripted (for C7sus4).
 * 	extStr		Chord symbol font, config.chordSymSmallSize %		Extensions
 * 					of chord symbol size; accidentals drawn in
 * 					music font, config.chordSymSmallSize % of
 *					music font size used in rootStr; all
 * 					superscripted by config.chordSymSuperscript.
 * 	extStk1Str	Same fonts, sizes, as extStr; enclosed in "()".		Extension stack,
 * 					enclosed in parentheses.						figured bass,
 * 	extStk2Str	Same as extStk1Str; each level stacked above		cautionary acc.
 * 	extStk3Str	the last; the stack of three extensions taken
 * 					as a whole is left justified. If the bottom
 *					level (extStk1Str) is present, it rests on
 *					the root baseline; else extStk2Str is super-
 *					scripted and extStk3Str is above that.
 * 	bassStr		Bass note name, to be printed following a slash.	Chord sym bass
 *																	applied dominant figs.
 *
 * This function also calculates a bounding box DRect (w/ left,baseline at 0,0).
 * If all the caller wants is the bounding box, surround the call to DrawChordSym
 * with HidePen and ShowPen. NB: The bounding box is NOT calculated when outputTo
 * equals toPostScript: if this isn't a bug, it's certainly a serious nuisance.
 */

void DrawChordSym(Document *doc,
					DDIST xd, DDIST yd,
					StringPtr string,					/* Pascal string */
					short auxInfo,						/* currently interpreted as Boolean:
															draw extension parentheses */
					PCONTEXT pContext, Boolean dim,
					DRect *dBox)						/* rect enclosing chord sym */
{
	register unsigned char	*p;
	short					oldFont, oldSize, oldStyle, csFont, csSize, csFace,
							csSmallSize, musSmallSize, xp, yp, musicSize, newTop,
							yTweak, superScript, gap, dblGap, stkLineHt, wid1, wid2, wid3,
							maxWid, start2, start3, parenTweak, baseline, extraLeading;
	char					rootStr[256], qualStr[MAX_FLDLEN+1], extStr[MAX_FLDLEN+1],
							extStk1Str[MAX_FLDLEN+1], extStk2Str[MAX_FLDLEN+1],
							extStk3Str[MAX_FLDLEN+1], bassStr[MAX_FLDLEN+1];
	Boolean					ok, hadAccident, hadSuperscript, showParens;
	Point					pt, newPt, rootPt;
	FontInfo				finfo;
	
	ok = ParseChordSym(string, rootStr, qualStr, extStr, extStk1Str, extStk2Str,
						extStk3Str, bassStr);

	showParens = (Boolean)(auxInfo & 0x0001);

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			
			SetFontFromTEXTSTYLE(doc, (TEXTSTYLE *)doc->fontNameCS, LNSPACE(pContext));
		
			csFont = GetPortTxFont();
			csFace = GetPortTxFace();
			
			/* sizes for root string */
			
			csSize = GetPortTxSize();
			musicSize = config.chordSymMusSize*(long)csSize/100L;
			
			/* sizes for other fields */
			
			csSmallSize = csSize*(long)config.chordSymSmallSize/100L;
			musSmallSize = musicSize*(long)config.chordSymSmallSize/100L;
			
			/* superscript, inter-chunk gap, etc. */
			
			superScript = csSize*(long)config.chordSymSuperscr/100L;
			gap = csSmallSize*(long)CS_GAPBTWFIELDS/100L;
			dblGap = gap*2;
			parenTweak = csSmallSize*(long)CS_PAREN_YOFFSET/100L;
	
			/* begin computation of dBox (for objRect) */
			
			dBox->left = dBox->right = 0;
			dBox->bottom = dBox->top = 0;
			GetFontInfo(&finfo);					/* of csFont, csSize */
			dBox->top -= p2d(finfo.ascent);
			dBox->bottom += p2d(finfo.descent);
			
			xp = d2p(xd);
			yp = d2p(yd);
			MoveTo(pContext->paper.left+xp, pContext->paper.top+yp);
			GetPen(&pt);

			/* Draw root string. */
			hadAccident = False;
			for (p=(unsigned char *)rootStr; *p; p++) {
				if (IsCSAcc(rootStr, p)) {
					/* Switch to the music font for this one character. Sonata's char.
					   sizes and origins are very different from those of text fonts,
					   so we have to fiddle around to make things look good. */
// FIXME: Note that the rest of this stuff doesn't map the acc char, etc.!

					TextFont(doc->musicFontNum);  TextSize(musicSize);  TextFace(0);
					yTweak = *p==SonataAcc[AC_FLAT]? musicSize/6 : musicSize/3;
					Move(musicSize/10, -(yTweak+superScript));
					if (dim) DrawMChar(doc, *p, NORMAL_VIS, True);
					else		DrawChar(*p);
					Move(musicSize/10, yTweak+superScript);
					TextFont(csFont);  TextSize(csSize);  TextFace(csFace);
					hadAccident = True;
				}
				else {
					if (dim)	DrawMChar(doc, *p, NORMAL_VIS, True);
					else		DrawChar(*p);
				}
			}
			
			/* continue computation of dBox (for objRect) */
			if (hadAccident) dBox->top -= p2d(superScript);		/* fudge for Sonata chars poking above top of text */
			GetPen(&newPt);
			dBox->right += p2d(newPt.h-pt.h);
			rootPt = pt;
			pt = newPt;
			
			TextSize(csSmallSize);					/* for rest of chord symbol (until /bass) */
			GetFontInfo(&finfo);					/* of csFont, csSmallSize */

			/* Draw quality string (???no accidentals recognized). */
			
			hadSuperscript = False;
			if (*qualStr) {
				Move(gap, 0);
				for (p=(unsigned char *)qualStr; *p; p++) {
					if (isdigit(*p)) {				/* superscript any digits (crude hack to allow C7sus4, etc.) */
						yTweak = superScript;
						hadSuperscript = True;
					}
					else
						yTweak = 0;
					Move(0, -yTweak);
					if (dim)DrawMChar(doc, *p, NORMAL_VIS, True);
					else	DrawChar(*p);
					Move(0, yTweak);
				}
				
				/* continue computation of dBox (for objRect) */
				
				GetPen(&newPt);
				dBox->right += p2d(newPt.h-pt.h);
				pt = newPt;
			}
			
			Move(0, -superScript);					/* for rest of chord symbol */

			/* Draw extension string (no parentheses). */
			
			if (*extStr) {
				hadSuperscript = True;
				Move(gap, 0);
				for (p=(unsigned char *)extStr; *p; p++) {
					if (Char2Acc(*p)>0) {
// FIXME: Note that the rest of this stuff doesn't map the acc char, etc.!
						TextFont(doc->musicFontNum);  TextSize(musSmallSize);  TextFace(0);
						yTweak = *p==SonataAcc[AC_FLAT]? musSmallSize/4 : musSmallSize/3;
						Move(musSmallSize/10, -yTweak);
						if (dim)	DrawMChar(doc, *p, NORMAL_VIS, True);
						else		DrawChar(*p);
						Move(musSmallSize/10, yTweak);
						TextFont(csFont);  TextSize(csSmallSize);  TextFace(csFace);
					}
					else {
						if (dim)	DrawMChar(doc, *p, NORMAL_VIS, True);
						else		DrawChar(*p);
					}
				}
				
				/* continue computation of dBox (for objRect) */
				
				GetPen(&newPt);
				dBox->right += p2d(newPt.h-pt.h);
				pt = newPt;
			}

			GetPen(&pt);								/* NB: pen reset by every call to Draw1Extension */
			if (*extStk1Str || *extStk2Str || *extStk3Str)
				pt.h += dblGap;							/* double the gap (it needs it) */
			
			/* Draw 3 extension stack strings, each level center justified and enclosed
			   in parentheses. First determine justification by calling Draw1Extension 
			   with draw=False. */
			   
			start2 = start3 = 0;
			wid1 = wid2 = wid3 = 0;

			stkLineHt = finfo.ascent + finfo.descent + finfo.leading; /* finfo of csFont, csSmallSize */
			/* To allow for the greater ascent and descent of accidentals, increase
			   leading by chordSymStkLead percent times chordSymMusSize. */
			
			extraLeading = config.chordSymMusSize*(long)stkLineHt/100L;
			extraLeading = (long)config.chordSymStkLead*extraLeading/100L;
			stkLineHt += extraLeading;
			if (*extStk1Str) {
				baseline = pt.v + superScript;			/* put this on root baseline */
				wid1 = Draw1Extension(doc, extStk1Str, pt.h, baseline, "\p", "\p",
								musSmallSize, csFont, csSmallSize, csFace, parenTweak,
								dim, showParens, True);
			}
			else
				baseline = pt.v + stkLineHt;
			if (*extStk2Str)
				wid2 = Draw1Extension(doc, extStk2Str, pt.h+start2, baseline-stkLineHt,
								"\p", "\p", musSmallSize, csFont, csSmallSize, csFace,
								parenTweak, dim, showParens, True);
			if (*extStk3Str)
				wid3 = Draw1Extension(doc, extStk3Str, pt.h+start3, baseline-(stkLineHt<<1),
								"\p", "\p", musSmallSize, csFont, csSmallSize, csFace,
								parenTweak, dim, showParens, True);

			/* continue computation of dBox (for objRect) */
			maxWid = MAX3(wid1, wid2, wid3);
			if (maxWid) {							/* if any part of extension stack was drawn */
				dBox->right += p2d(maxWid+dblGap);
				pt.h += maxWid;
			}
			if (*extStk3Str) {
				if (*extStk1Str)
					dBox->top = -(p2d(stkLineHt*3));
				else
					dBox->top = -(p2d(superScript+(stkLineHt<<1)));
			}
			else if (*extStk2Str) {
				if (*extStk1Str)
					dBox->top = -(p2d(stkLineHt<<1));
				else
					hadSuperscript = True;
			}
			if (hadSuperscript) {
				newTop = -(p2d(stkLineHt+superScript));
				if (newTop<dBox->top) dBox->top = newTop;
			}

			/* Draw bass note string. */
			if (*bassStr) {
				Boolean hadNewAccident;

				TextSize(csSize);

				/* Draw slash */
				MoveTo(pt.h+dblGap, rootPt.v);
				if (dim)	DrawMChar(doc, '/', NORMAL_VIS, True);
				else		DrawChar('/');
				Move(dblGap+gap, 0);				/* move 3 gaps worth after slash */

				hadNewAccident = False;
				for (p=(unsigned char *)bassStr; *p; p++) {
					if (IsCSAcc(bassStr, p)) {
// FIXME: Note that the rest of this stuff doesn't map the acc char, etc.!
						TextFont(doc->musicFontNum);  TextSize(musicSize);  TextFace(0);
						yTweak = *p==SonataAcc[AC_FLAT]? musicSize/6 : musicSize/3;
						Move(musicSize/10, -(yTweak+superScript));
						if (dim)	DrawMChar(doc, *p, NORMAL_VIS, True);
						else		DrawChar(*p);
						Move(musicSize/10, yTweak+superScript);
						TextFont(csFont);  TextSize(csSize);  TextFace(csFace);
						hadNewAccident = True;
					}
					else {
						if (dim)	DrawMChar(doc, *p, NORMAL_VIS, True);
						else		DrawChar(*p);
					}
				}
				/* complete computation of dBox (for objRect) */
				GetPen(&newPt);
				dBox->right += p2d(newPt.h-pt.h);
				if (hadNewAccident && !hadAccident)
					dBox->top -= p2d(superScript);		/* fudge for Sonata chars poking above top of text */
			}
			
			TextFont(oldFont);  TextSize(oldSize);  TextFace(oldStyle);
			break;

		case toPostScript:
{
			TEXTSTYLE		*pTxStyle;
			DDIST			ydRoot, xTweakD, yTweakD, superScriptD, gapD, dblGapD, extStackWidD;
			short			w;
			unsigned char csName[32], musFontName[64], substr[2];
			short			newFontIndex;
			
			Pstrcpy((StringPtr)musFontName, (StringPtr)doc->musFontName);			
			pTxStyle = (TEXTSTYLE *)doc->fontNameCS;
			Pstrcpy((StringPtr)csName, (StringPtr)pTxStyle->fontName);
			newFontIndex = FontName2Index(doc, csName);			/* in case toPostScript: should never fail */
			csFont = doc->fontTable[newFontIndex].fontID;
			csSize = GetTextSize(pTxStyle->relFSize, pTxStyle->fontSize, LNSPACE(pContext));
			csFace = pTxStyle->fontStyle;
			musicSize = config.chordSymMusSize*(long)csSize/100L;
			/* sizes for other fields */
			csSmallSize = csSize*(long)config.chordSymSmallSize/100L;
			musSmallSize = musicSize*(long)config.chordSymSmallSize/100L;
   
			/* superscript, inter-chunk gap, etc. */
			
			superScriptD = pt2d(csSize*(long)config.chordSymSuperscr/100L);

			/* Need more precision for this; otherwise, it's often zero. */
			
			gapD = pt2d((short)rint((double)csSmallSize*((double)(CS_GAPBTWFIELDS)/100.0)));
			dblGapD = gapD*2;
			parenTweak = csSmallSize*(long)CS_PAREN_YOFFSET/100L;
	
			/* Draw root string. */
			
			for (p=(unsigned char *)rootStr; *p; p++) {
				substr[0] = 1;	substr[1] = *p;
				if (IsCSAcc(rootStr, p)) {
						/* Switch to the music font for this one character. Sonata's char.
						   sizes and origins are very different from those of text fonts,
						   so we have to fiddle around to make things look good. */
						   
					xTweakD = pt2d(musicSize/10);				/* before AND after the accidental */
					yTweakD = *p==SonataAcc[AC_FLAT]? pt2d(musicSize/6) : pt2d(musicSize/3);
					PS_FontString(doc, xd+xTweakD, yd-(yTweakD+superScriptD), substr, musFontName, musicSize, 0);
#ifdef MORE_ACCURATE_CHAR_WIDTH
					w = NPtStringWidth(doc, substr, doc->musicFontNum, musicSize, 0);
#else
					/* Prepare to measure width of character in points */
// FIXME: Note that the previous code doesn't map the acc char, etc.!

					TextFont(doc->musicFontNum);  TextSize(musicSize);  TextFace(0);
#endif
					xTweakD *= 2;								/* include enclosing padding in width. */
				}
				else {
					xTweakD = 0;
					PS_FontString(doc, xd, yd, substr, csName, csSize, doc->fontStyleCS);
#ifdef MORE_ACCURATE_CHAR_WIDTH
					w = NPtStringWidth(doc, substr, csFont, csSize, csFace);
#else
					/* Prepare to measure width of character in points */
					
					TextFont(csFont);  TextSize(csSize);  TextFace(csFace);
#endif
				}
#ifndef MORE_ACCURATE_CHAR_WIDTH
				w = CharWidth(*p);
#endif
				xd += pt2d(w) + xTweakD;
			}

			/* Draw quality string (???no accidentals recognized). */
			
			if (*qualStr) {
				xd += gapD;
				/* Prepare to measure width of characters in points */
				
				TextFont(csFont);  TextSize(csSmallSize);  TextFace(csFace);
				for (p=(unsigned char *)qualStr; *p; p++) {
					substr[0] = 1;	substr[1] = *p;
					yTweakD = isdigit(*p)? superScriptD : 0;
					PS_FontString(doc, xd, yd-yTweakD, substr, csName, csSmallSize, doc->fontStyleCS);
#ifdef MORE_ACCURATE_CHAR_WIDTH
					w = NPtStringWidth(doc, substr, csFont, csSmallSize, csFace);
#else
					w = CharWidth(*p);
#endif
					xd += pt2d(w);
				}
			}
			
			ydRoot = yd;
			yd -= superScriptD;									/* for rest of chord sym (until /bass) */

			/* Draw extension string (no parentheses). */
			
			if (*extStr) {
				xd += gapD;
				for (p=(unsigned char *)extStr; *p; p++) {
					substr[0] = 1;	substr[1] = *p;
					if (Char2Acc(*p)>0) {
						xTweakD = pt2d(musSmallSize/10);		/* before AND after the accidental */
						yTweakD = *p==SonataAcc[AC_FLAT]? pt2d(musSmallSize/4) : pt2d(musSmallSize/3);
						PS_FontString(doc, xd+xTweakD, yd-yTweakD, substr, musFontName, musSmallSize, 0);
#ifdef MORE_ACCURATE_CHAR_WIDTH
						w = NPtStringWidth(doc, substr, doc->musicFontNum, musSmallSize, 0);
#else
						/* Prepare to measure width of character in points */
// FIXME: Note that the previous code doesn't map the acc char, etc.!
						TextFont(doc->musicFontNum);  TextSize(musSmallSize);  TextFace(0);
#endif
						xTweakD *= 2;							/* include enclosing padding in width. */
					}
					else {
						xTweakD = 0;
						PS_FontString(doc, xd, yd, substr, csName, csSmallSize, doc->fontStyleCS);
#ifdef MORE_ACCURATE_CHAR_WIDTH
						w = NPtStringWidth(doc, substr, csFont, csSmallSize, csFace);
#else
						/* Prepare to measure width of character in points */
						
						TextFont(csFont);  TextSize(csSmallSize);  TextFace(csFace);
#endif
					}
#ifndef MORE_ACCURATE_CHAR_WIDTH
					w = CharWidth(*p);
#endif
					xd += pt2d(w) + xTweakD;
				}
			}

			if (*extStk1Str || *extStk2Str || *extStk3Str)
				xd += dblGapD;					/* double the gap (it needs it) */

			/* Draw 3 extension stack strings, each level center justified and enclosed
			   in parentheses. First determine justification by calling Draw1Extension
			   with draw=False. */
			   
			start2 = start3 = 0;
			wid1 = wid2 = wid3 = 0;

			TextFont(csFont);  TextSize(csSmallSize);  TextFace(csFace);
			GetFontInfo(&finfo);
			stkLineHt = finfo.ascent + finfo.descent + finfo.leading;
			/* To allow for the greater ascent and descent of accidentals, increase
				leading by chordSymStkLead percent times chordSymMusSize. */
			
			extraLeading = config.chordSymMusSize*(long)stkLineHt/100L;
			extraLeading = (long)config.chordSymStkLead*extraLeading/100L;
			stkLineHt += extraLeading;
			
			if (*extStk1Str) {
				yd += superScriptD;								/* put this on root baseline */
				baseline = d2pt(yd);
				wid1 = Draw1Extension(doc, extStk1Str, d2pt(xd), baseline, musFontName, csName,
								musSmallSize, csFont, csSmallSize, csFace, parenTweak, dim, showParens, True);
			}
			else
				baseline = d2pt(yd) + stkLineHt;
			if (*extStk2Str)
				wid2 = Draw1Extension(doc, extStk2Str, d2pt(xd)+start2, baseline-stkLineHt, musFontName, csName,
								musSmallSize, csFont, csSmallSize, csFace, parenTweak, dim, showParens, True);
			if (*extStk3Str)
				wid3 = Draw1Extension(doc, extStk3Str, d2pt(xd)+start3, baseline-(stkLineHt<<1), musFontName, csName,
								musSmallSize, csFont, csSmallSize, csFace, parenTweak, dim, showParens, True);

			maxWid = MAX3(wid1, wid2, wid3);
			extStackWidD = pt2d(maxWid);

			/* Draw bass note string. */
			
			if (*bassStr) {
				xd += extStackWidD + (dblGapD+gapD);		/* Seems to need more of a gap. */
				yd = ydRoot;

				/* Draw slash */
				
				substr[0] = 1;	substr[1] = '/';
				PS_FontString(doc, xd, yd, substr, csName, csSize, doc->fontStyleCS);

#ifdef MORE_ACCURATE_CHAR_WIDTH
				w = NPtStringWidth(doc, substr, csFont, csSize, csFace);
#else
				/* Prepare to measure width of character in points */
				
				TextFont(csFont);  TextSize(csSize);  TextFace(csFace);
				w = CharWidth('/');
#endif
				xd += pt2d(w) + (dblGapD+gapD);

				for (p=(unsigned char *)bassStr; *p; p++) {
					substr[0] = 1;	substr[1] = *p;
					if (IsCSAcc(bassStr, p)) {
						/* Switch to the music font for this one character. Sonata's char.
						   sizes and origins are very different from those of text fonts,
						   so we have to fiddle around to make things look good. */
						   
						xTweakD = pt2d(musicSize/10);				/* before AND after the accidental */
						yTweakD = *p==SonataAcc[AC_FLAT]? pt2d(musicSize/6) : pt2d(musicSize/3);
						PS_FontString(doc, xd+xTweakD, yd-(yTweakD+superScriptD), substr, musFontName, musicSize, 0);
#ifdef MORE_ACCURATE_CHAR_WIDTH
						w = NPtStringWidth(doc, substr, doc->musicFontNum, musicSize, 0);
#else
						/* Prepare to measure width of character in points */
// FIXME: Note that the previous code doesn't map the acc char, etc.!

						TextFont(doc->musicFontNum);  TextSize(musicSize);  TextFace(0);
#endif
						xTweakD *= 2;									/* include enclosing padding in width. */
					}
					else {
						xTweakD = 0;
						PS_FontString(doc, xd, yd, substr, csName, csSize, doc->fontStyleCS);
#ifdef MORE_ACCURATE_CHAR_WIDTH
						w = NPtStringWidth(doc, substr, csFont, csSize, csFace);
#else
						/* Prepare to measure width of character in points */
						
						TextFont(csFont);  TextSize(csSize);  TextFace(csFace);
#endif
					}
#ifndef MORE_ACCURATE_CHAR_WIDTH
					w = CharWidth(*p);
#endif
					xd += pt2d(w) + xTweakD;
				}
			}
}
			break;

		default:
		;
	}
}


/* Draw the extension string "(str)" at xp,yp (paper coords) using current font
 * characteristics. Returns the width of the string it drew in pixels.
 */

static short Draw1Extension(
			Document *doc,
			char *str,							/* C string to enclose in parentheses */
			short xp, short yp,					/* paper-rel pixels (QD) or points (PS); ignored if draw==False */
			const unsigned char musFontName[],	/* used only if drawing to PostScript */
			const unsigned char csFontName[],	/* used only if drawing to PostScript */
			short musSmallSize,
			short csFont,
			short csSmallSize,
			short csFace,
			short parenTweak,
			Boolean dim,
			Boolean showParens,
			Boolean draw				/* if True draw the string; else calculate and return width only */
			)
{
	register unsigned char	*p;
	short			yTweak, w=0, wid;
	DDIST			yTweakD, xd, yd;
	Point			pt;
	unsigned char	substr[2];
	
	if (str[0]==0) return 0;

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			MoveTo(xp, yp);
			if (showParens) {
				if (draw) {
					Move(0, -parenTweak);
					if (dim)	DrawMChar(doc, '(', NORMAL_VIS, True);
					else		DrawChar('(');
					Move(0, parenTweak);
				}
				else
					w += CharWidth('(');
			}
			for (p=(unsigned char *)str; *p; p++) {
				if (Char2Acc(*p)>0) {
					TextFont(doc->musicFontNum);  TextSize(musSmallSize);  TextFace(0);
					if (draw) {
						yTweak = *p==SonataAcc[AC_FLAT]? musSmallSize/4 : musSmallSize/3;
						Move(musSmallSize/10, -yTweak);
						if (dim)	DrawMChar(doc, *p, NORMAL_VIS, True);
						else		DrawChar(*p);
						Move(musSmallSize/10, yTweak);
					}
					else
						w += CharWidth(*p) + musSmallSize/10;
					TextFont(csFont);  TextSize(csSmallSize);  TextFace(csFace);
				}
				else {
					if (draw) {
						if (dim)	DrawMChar(doc, *p, NORMAL_VIS, True);
						else		DrawChar(*p);
					}
					else
						w += CharWidth(*p);
				}
			}
			if (showParens) {
				if (draw) {
					Move(0, -parenTweak);
					if (dim)	DrawMChar(doc, ')', NORMAL_VIS, True);
					else		DrawChar(')');
					Move(0, parenTweak);
				}
				else
					w += CharWidth(')');
			}
			if (draw) {
				GetPen(&pt);
				w = pt.h - xp;
			}
			break;
			
		case toPostScript:
			xd = pt2d(xp);	yd = pt2d(yp);
			if (showParens) {
#ifdef MORE_ACCURATE_CHAR_WIDTH
				substr[0] = 1;	substr[1] = '(';
				wid = NPtStringWidth(doc, substr, csFont, csSmallSize, csFace);
#else
				TextFont(csFont);  TextSize(csSmallSize);  TextFace(csFace);
				wid = CharWidth('(');				/* measure width of character in points */
#endif
				if (draw) {
					substr[0] = 1;	substr[1] = '(';
					yTweakD = pt2d(parenTweak);
					PS_FontString(doc, xd, yd-yTweakD, substr, csFontName, csSmallSize, csFace);
					xd += pt2d(wid);
				}
				w += wid;
			}
			for (p=(unsigned char *)str; *p; p++) {
				if (Char2Acc(*p)>0) {
#ifdef MORE_ACCURATE_CHAR_WIDTH
					substr[0] = 1;	substr[1] = *p;
					wid = NPtStringWidth(doc, substr, doc->musicFontNum, musSmallSize, 0) + ((musSmallSize/10)<<1);
#else
					TextFont(doc->musicFontNum);  TextSize(musSmallSize);  TextFace(0);
					wid = CharWidth(*p) + ((musSmallSize/10)<<1);
#endif
					if (draw) {
						substr[0] = 1;	substr[1] = *p;
						yTweakD = *p==SonataAcc[AC_FLAT]? pt2d(musSmallSize/4) : pt2d(musSmallSize/3);
						PS_FontString(doc, xd+pt2d(musSmallSize/10), yd-yTweakD, substr,
																		musFontName, musSmallSize, 0);
						xd += pt2d(wid);
					}
					w += wid;
				}
				else {
#ifdef MORE_ACCURATE_CHAR_WIDTH
					substr[0] = 1;	substr[1] = *p;
					wid = NPtStringWidth(doc, substr, csFont, csSmallSize, csFace);
#else
					TextFont(csFont);  TextSize(csSmallSize);  TextFace(csFace);
					wid = CharWidth(*p);
#endif
					if (draw) {
						substr[0] = 1;	substr[1] = *p;
						PS_FontString(doc, xd, yd, substr, csFontName, csSmallSize, csFace);
						xd += pt2d(wid);
					}
					w += wid;
				}
			}
			if (showParens) {
#ifdef MORE_ACCURATE_CHAR_WIDTH
				substr[0] = 1;	substr[1] = ')';
				w += NPtStringWidth(doc, substr, csFont, csSmallSize, csFace);
#else
				TextFont(csFont);  TextSize(csSmallSize);  TextFace(csFace);
				w += CharWidth(')');
#endif
				if (draw) {
					substr[0] = 1;	substr[1] = ')';
					yTweakD = pt2d(parenTweak);
					PS_FontString(doc, xd, yd-yTweakD, substr, csFontName, csSmallSize, csFace);
				}
			}
			break;
	}
	return w;
}


/* ---------------------------------------------------------- ChordSymDialog & friends -- */

/* WARNING: Code in CheckCSuserItems depends on popups following (by 1) their
 * associated edit fields in the item list.
 */
static enum {
	BUT1_OK = 1,
	BUT2_Cancel,
	STXT3_Chord,
	EDIT4_Root,
	POP5_Root,
	EDIT6_Quality,
	POP7_Quality,
	EDIT8_Ext,
	POP9_Ext,
	EDIT10_ExtStk2,		/* middle level of extension stack */
	POP11_ExtStk2,		/* NB: ext1 and ext2 were switched to affect tabbing order (JG) */
	EDIT12_ExtStk1,		/* bottom level of extension stack */
	POP13_ExtStk1,
	EDIT14_ExtStk3,		/* top level of extension stack */
	POP15_ExtStk3,
	EDIT16_Bass,
	POP17_Bass,
	USER18_Display,
	USER19_HelpBox,
	CHK20_ShowParens,
	STXT21_LParen1,		/* NB: ShowHideParens depends on sequence of these */
	STXT22_LParen2,
	STXT23_LParen3,
	STXT24_RParen1,
	STXT25_RParen2,
	STXT26_RParen3,
	STXT27_Slash,
	LASTITEM
} E_ChordSymItems;

static UserPopUp	rootPop, qualPop, extPop, extstk1Pop, extstk2Pop, extstk3Pop, bassPop;
static Rect			displayBox;
static Document		*localDoc;


/* -------------------------------------------------------------------- ChordSymDialog -- */

Boolean ChordSymDialog(Document *doc, StringPtr string, short *auxInfo)
{
	short		item, type;
	Boolean		showParens, keepGoing=True;
	Handle		hndl;
	UserPopUp	*p;
	Str255		str;
	char		qualStr[MAX_FLDLEN+1], extStr[MAX_FLDLEN+1],
				extStk1Str[MAX_FLDLEN+1], extStk2Str[MAX_FLDLEN+1],
				extStk3Str[MAX_FLDLEN+1], bassStr[MAX_FLDLEN+1];
	DialogPtr	dlog;
	GrafPtr		oldPort;

	WaitCursor();

	filterUPP = NewModalFilterUPP(ChordSymFilter);
	if (filterUPP == NULL) {
		MissingDialog(CHORDSYM_DLOG);
		return False;
	}
	GetPort(&oldPort);
	dlog = GetNewDialog(CHORDSYM_DLOG, NULL, BRING_TO_FRONT);
	if (dlog == NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(CHORDSYM_DLOG);
		return False;
	}

	localDoc = doc;						/* make doc available to DisplayChordSym via ChordSymFilter */
	SetPort(GetDialogWindowPort(dlog));
	CenterWindow(GetDialogWindow(dlog), 110);	/* higher num. than usual: try to keep popups closer to middle of screen */

	/* Unpack options from auxInfo, and set appropriate dlog items. */
	showParens = (Boolean)(*auxInfo & 0x0001);
	PutDlgChkRadio(dlog, CHK20_ShowParens, showParens);
	ShowHideParens(dlog, showParens);

	/* initialize edit fields */
	ParseChordSym(string, (char *)str, qualStr, extStr, extStk1Str, extStk2Str, extStk3Str, bassStr);
	PutDlgString(dlog, EDIT6_Quality, (unsigned char *)CToPString(qualStr), True);
	PutDlgString(dlog, EDIT8_Ext, (unsigned char *)CToPString(extStr), True);
	PutDlgString(dlog, EDIT10_ExtStk2, (unsigned char *)CToPString(extStk2Str), True);
	PutDlgString(dlog, EDIT12_ExtStk1, (unsigned char *)CToPString(extStk1Str), True);
	PutDlgString(dlog, EDIT14_ExtStk3, (unsigned char *)CToPString(extStk3Str), True);
	PutDlgString(dlog, EDIT16_Bass, (unsigned char *)CToPString(bassStr), True);
	CToPString((char *)str);
	if (str[0] > MAX_FLDLEN) str[0] = MAX_FLDLEN;		/* limit old-style chord sym to 31 chars */
	PutDlgString(dlog, EDIT4_Root, str, True);

	/* initialize popups */
	rootPop.menu = qualPop.menu = extPop.menu = extstk1Pop.menu =
								extstk2Pop.menu = extstk3Pop.menu = bassPop.menu = NULL;
	if (!InitPopUp(dlog, &rootPop, POP5_Root, 0, 200, 0)) goto broken;
	if (!InitPopUp(dlog, &qualPop, POP7_Quality, 0, 201, 0)) goto broken;
	if (!InitPopUp(dlog, &extPop, POP9_Ext, 0, 202, 1)) goto broken;
	if (!InitPopUp(dlog, &extstk2Pop, POP11_ExtStk2, 0, 203, 0)) goto broken;
	if (!InitPopUp(dlog, &extstk1Pop, POP13_ExtStk1, 0, 203, 0)) goto broken;
	if (!InitPopUp(dlog, &extstk3Pop, POP15_ExtStk3, 0, 203, 0)) goto broken;
	if (!InitPopUp(dlog, &bassPop, POP17_Bass, 0, 200, 0)) goto broken;

	/* save chord symbol display rect */
	GetDialogItem(dlog, USER18_Display, &type, &hndl, &displayBox);

	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	while (keepGoing) {
		ModalDialog(filterUPP, &item);
		switch (item) {
			case OK:
				if (!AnyBadCSvalues(dlog)) keepGoing = False;
				break;
			case Cancel:
				keepGoing = False;
				break;
			case POP5_Root:		p = &rootPop;		goto loadField;
			case POP7_Quality:	p = &qualPop;		goto loadField;
			case POP9_Ext:		p = &extPop;		goto loadField;
			case POP11_ExtStk2:	p = &extstk2Pop;	goto loadField;
			case POP13_ExtStk1:	p = &extstk1Pop;	goto loadField;
			case POP15_ExtStk3:	p = &extstk3Pop;	goto loadField;
			case POP17_Bass:	p = &bassPop;		goto loadField;
			
			case EDIT4_Root:
			case EDIT6_Quality:
			case EDIT8_Ext:
			case EDIT10_ExtStk2:
			case EDIT12_ExtStk1:
			case EDIT14_ExtStk3:
			case EDIT16_Bass:
				DisplayChordSym(doc, dlog, False);
				break;
			case CHK20_ShowParens:
				PutDlgChkRadio(dlog, CHK20_ShowParens, !GetDlgChkRadio(dlog, CHK20_ShowParens));
				showParens = GetDlgChkRadio(dlog, CHK20_ShowParens);
				ShowHideParens(dlog, showParens);
				DisplayChordSym(doc, dlog, True);
				break;
		}
		continue;
loadField:
		GetMenuItemText(p->menu, p->currentChoice, str);
		if (str[0] > MAX_FLDLEN) str[0] = MAX_FLDLEN;		/* limit to 31 chars */
		PutDlgString(dlog, item-1, str, True);
		DisplayChordSym(doc, dlog, False);
	}
	if (item==OK) {
		BuildCSstring(dlog, (unsigned char *)string);
		GetCSOptions(dlog, &showParens);
		*auxInfo = showParens? 1 : 0;
	}
	CloseCSDialog(dlog);
	SetPort(oldPort);
	return (item==OK);
broken:
	CloseCSDialog(dlog);
	SetPort(oldPort);
	NoMoreMemory();
	return False;
}


/* Construct the delimited chord symbol string <str> from edit fields in <dlog>. */

static void BuildCSstring(DialogPtr dlog, unsigned char *str)
{
	unsigned char delim[2];
	Str255 tmp;
	
	delim[0] = 1;	delim[1] = CS_DELIMITER;
	str[0] = 0;
	
	/* CAUTION: order of EDIT fields below is critical! */
	
	GetDlgString(dlog, EDIT4_Root, tmp);
	PStrCat((StringPtr)str, tmp);
	PStrCat((StringPtr)str, (StringPtr)delim);
	
	GetDlgString(dlog, EDIT6_Quality, tmp);
	PStrCat((StringPtr)str, tmp);
	PStrCat((StringPtr)str, (StringPtr)delim);
	
	GetDlgString(dlog, EDIT8_Ext, tmp);
	PStrCat((StringPtr)str, tmp);
	PStrCat((StringPtr)str, (StringPtr)delim);

	GetDlgString(dlog, EDIT12_ExtStk1, tmp);
	PStrCat((StringPtr)str, tmp);
	PStrCat((StringPtr)str, (StringPtr)delim);
	
	GetDlgString(dlog, EDIT10_ExtStk2, tmp);
	PStrCat((StringPtr)str, tmp);
	PStrCat((StringPtr)str, (StringPtr)delim);
	
	GetDlgString(dlog, EDIT14_ExtStk3, tmp);
	PStrCat((StringPtr)str, tmp);
	PStrCat((StringPtr)str, (StringPtr)delim);
	
	GetDlgString(dlog, EDIT16_Bass, tmp);
	PStrCat((StringPtr)str, tmp);
}


/* Get the state of various chord symbol options (currently only whether
	to draw parentheses around extensions) from the dlog items. */

static void GetCSOptions(DialogPtr dlog, Boolean *showParens)
{
	*showParens = GetDlgChkRadio(dlog, CHK20_ShowParens);
}


static void CloseCSDialog(DialogPtr dlog)
{
	DisposePopUp(&rootPop);
	DisposePopUp(&qualPop);
	DisposePopUp(&extPop);
	DisposePopUp(&extstk1Pop);
	DisposePopUp(&extstk2Pop);
	DisposePopUp(&extstk3Pop);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	ArrowCursor();
}


/* All this does now is make sure dialog has at least one non-empty field. */

static Boolean AnyBadCSvalues(DialogPtr dlog)
{
	Str255	str;
	short	hasText=0;

	hasText = GetDlgString(dlog, EDIT4_Root, str);
	hasText |= GetDlgString(dlog, EDIT6_Quality, str);
	hasText |= GetDlgString(dlog, EDIT8_Ext, str);
	hasText |= GetDlgString(dlog, EDIT10_ExtStk2, str);
	hasText |= GetDlgString(dlog, EDIT12_ExtStk1, str);
	hasText |= GetDlgString(dlog, EDIT14_ExtStk3, str);
	hasText |= GetDlgString(dlog, EDIT16_Bass, str);
	
	if (hasText==0) {
		GetIndCString(strBuf, MISCERRS_STRS, 12);		/* "At least one text box must contain text" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return True;
	}
	return False;
}


static void ShowHideParens(DialogPtr dlog, Boolean show)
{
	short	item, type;
	Rect	box;
	Handle	hndl;

	for (item = STXT21_LParen1; item <= STXT26_RParen3; item++) {
		GetDialogItem(dlog, item, &type, &hndl, &box);
		if (show)
			ShowDialogItem(dlog, item);
		else
			HideDialogItem(dlog, item);
	}
}


/* Display in the USER18_Display rectangle the chord symbol specified by edit fields in <dlog>. */

static void DisplayChordSym(
				Document *doc,
				DialogPtr dlog,
				Boolean alwaysDraw	/* if False, draw only if chord sym has changed */
				)
{
	short			saveMagnify, saveSize, saveRelSize;
	Str255			fullStr;
	static Str255	saveFullStr = "\p";
	DDIST			xd, yd;
	CONTEXT			context;
	Rect			box;
	DRect			dBox;
	RgnHandle		rgnHdl;
	Boolean			showParens;
	short			auxInfo;
		
	BuildCSstring(dlog, fullStr);

	GetCSOptions(dlog, &showParens);
	auxInfo = showParens? 1 : 0;				/* Change if we have to encode more options later */

	if (!alwaysDraw)
		if (Pstreql((StringPtr)fullStr, (StringPtr)saveFullStr)) return;

	/* fiddle with chord symbol font size */
	saveRelSize = doc->relFSizeCS;
	doc->relFSizeCS = False;
	saveSize = doc->fontSizeCS;
	doc->fontSizeCS = 18;
	
	saveMagnify = doc->magnify;
	doc->magnify = 0;							/* set to 100% */
	InstallMagnify(doc);
	
	/* Create a phony context with just enough info to please DrawChordSym. */
	context.paper = displayBox;
	context.staffHeight = 256;					/* for rastral 5 (irrelevant because relFSizeCS==False) */
	context.staffLines = 5;
	
	/* Get bounding box (with left,baseline at 0,0). */ 
	HidePen();
	DrawChordSym(doc, 0, 0, fullStr, auxInfo, &context, False, &dBox);
	ShowPen();
	
	rgnHdl = NewRgn();							/* save current clipping region */
	GetClip(rgnHdl);
	ClipRect(&displayBox);						/* limit it to our box */

	box = displayBox;
	box.left++; box.top++;						/* don't erase part of frame */
	EraseRect(&box);
	
	/* Center chord sym horizontally; set vpos from a constant. */
	yd = p2d(displayBox.bottom-displayBox.top-CS_BASELINE_FROMBOT);
	xd = (p2d(displayBox.right-displayBox.left) - dBox.right)>>1;
	
	DrawChordSym(doc, xd, yd, fullStr, auxInfo, &context, False, &dBox);

	SetClip(rgnHdl);							/* restore clipping rgn */

	doc->relFSizeCS = saveRelSize;				/* restore chord symbol font size */
	doc->fontSizeCS = saveSize;

	doc->magnify = saveMagnify;					/* restore magnification */
	InstallMagnify(doc);
	
	Pstrcpy((StringPtr)saveFullStr, (StringPtr)fullStr);
}


/* -------------------------------------------------------------------- ChordSymFilter -- */

static pascal Boolean ChordSymFilter(DialogPtr dlog, EventRecord *evt, short *item)
{
	short			type, oldFont, oldSize, oldStyle;
	Handle			hndl;
	Rect			box,portRect;
	Point			where;
	unsigned char	ch;
	Str255			helpStr;

	/* This should be in response to a nullEvent, but we don't receive those reliably here. */
	DialogAdjustCursor(dlog, evt, True);
	
	switch (evt->what) {
		case updateEvt:
			BeginUpdate(GetDialogWindow(dlog));
			UpdateDialogVisRgn(dlog);

			/* Have to restrict clipping rgn, otherwise DrawPopUp will
			 * draw outside of our exceedingly narrow popups.
			 */
			ClipRect(&rootPop.shadow);		DrawPopUp(&rootPop);
			ClipRect(&qualPop.shadow);		DrawPopUp(&qualPop);
			ClipRect(&extPop.shadow);		DrawPopUp(&extPop);
			ClipRect(&extstk1Pop.shadow);	DrawPopUp(&extstk1Pop);
			ClipRect(&extstk2Pop.shadow);	DrawPopUp(&extstk2Pop);
			ClipRect(&extstk3Pop.shadow);	DrawPopUp(&extstk3Pop);
			ClipRect(&bassPop.shadow);		DrawPopUp(&bassPop);
			
			portRect = GetQDPortBounds();
			ClipRect(&portRect);								/* restore clipping rgn */

			/* draw help text */
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			TextFont(textFontNum);  TextSize(textFontSmallSize);  TextFace(0);
			GetDialogItem(dlog, USER19_HelpBox, &type, &hndl, &box);
			GetIndString(helpStr, DIALOG_STRS, 7);    			/* "Click and hold arrows to choose from suggestions, or type into text boxes." */
			TETextBox(&helpStr[1], (long)helpStr[0], &box, 0);
			TextFont(oldFont);  TextSize(oldSize);  TextFace(oldStyle);
			
			FrameDefault(dlog, OK, True);

			/* draw dotted line around chord symbol display */
			PenPat(NGetQDGlobalsGray());
			MoveTo(displayBox.left, displayBox.bottom);
			LineTo(displayBox.left, displayBox.top);
			LineTo(displayBox.right, displayBox.top);
			LineTo(displayBox.right, displayBox.bottom);
			LineTo(displayBox.left, displayBox.bottom);
			PenPat(NGetQDGlobalsBlack());

			DisplayChordSym(localDoc, dlog, True);

			EndUpdate(GetDialogWindow(dlog));
			*item = 0;
			return True;
			break;
		case mouseDown:
			DialogAdjustCursor(dlog, evt, True);
			where = evt->where;
			GlobalToLocal(&where);
			if (CheckCSuserItems(dlog, where, item)) return True;
			break;
		case keyDown:
		case autoKey:
			if (DlgCmdKey(dlog, evt, (short *)item, False)) {
				Str255	str;
				short	curField;
				
				curField = GetDialogKeyboardFocusItem(dlog);
				GetDlgString(dlog, curField, str);
				if (str[0]>MAX_FLDLEN) {
					str[0] = MAX_FLDLEN;								/* in case pasted too many chars */
					PutDlgString(dlog, curField, str, True);
				}
				DisplayChordSym(localDoc, dlog, False);
				return True;
			}
			ch = evt->message & charCodeMask;
			if (ch==CS_DELIMITER) return True;							/* filter out delimiter! */
			TEHandle textH = GetDialogTextEditHandle(dlog);
			if ((*textH)->teLength >= MAX_FLDLEN)
			//if ((**(*(DialogPeek)dlog).textH).teLength >= MAX_FLDLEN)	/* can't add any more chars */
				if (ch!=CH_BS && ch!='\t' && ch!=LEFTARROWKEY &&
						ch!=RIGHTARROWKEY && ch!=UPARROWKEY && ch!=DOWNARROWKEY) {
					GetIndCString(strBuf, MISCERRS_STRS, 13);			/* "text has reached its maximum length" */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					return True;
				}
			break;
	}

	return False;
}


/* ------------------------------------------------------------------ CheckCSuserItems -- */

static Boolean CheckCSuserItems(DialogPtr dlog, Point where, short *itemHit)
{
	register short		i, numItems;
	register UserPopUp	*p;
	register Boolean	changeMenu=False;
	Str255				eStr, mStr;
	Rect 				portRect;
	
	if (PtInRect(where, &rootPop.shadow))
		{ p = &rootPop; *itemHit = POP5_Root; }
	else if (PtInRect(where, &qualPop.shadow))
		{ p = &qualPop; *itemHit = POP7_Quality; }
	else if (PtInRect(where, &extPop.shadow))
		{ p = &extPop; *itemHit = POP9_Ext; }
	else if (PtInRect(where, &extstk2Pop.shadow))
		{ p = &extstk2Pop; *itemHit = POP11_ExtStk2; }
	else if (PtInRect(where, &extstk1Pop.shadow))
		{ p = &extstk1Pop; *itemHit = POP13_ExtStk1; }
	else if (PtInRect(where, &extstk3Pop.shadow))
		{ p = &extstk3Pop; *itemHit = POP15_ExtStk3; }
	else if (PtInRect(where, &bassPop.shadow))
		{ p = &bassPop; *itemHit = POP17_Bass; }
	else { *itemHit = 0; return False; }
	
	ClipRect(&p->shadow);								/* prevent drawing outside of popup */

	/* If the edit field to the left of the popup is empty, no menu item
	 * will be checked. If there's something in the edit field, see if it's
	 * the same as the text of a menu item. If so, set that item to be the
	 * current choice, with a check beside it. If not, insert at the top of
	 * the menu a new item containing the text of the edit field, followed
	 * by a new divider item. Set the top item to be the current choice,
	 * with a check beside it. After calling DoUserPopUp, adjust the menu's
	 * current choice so that ChordSymDialog will fill the edit field with
	 * the text of the item the user chose. (cf. Find cmd. in Sys7 Finder.)
	 */	
	SetItemMark(p->menu, p->currentChoice, 0);

	if (GetDlgString(dlog, *itemHit-1, eStr)) {			/* NB: assumes editfld item = popup item - 1 */
		numItems = CountMenuItems(p->menu);
		for (i=1; i<=numItems; i++) {
			GetMenuItemText(p->menu, i, mStr);
			if (Pstreql(eStr, mStr)) {
				ChangePopUpChoice(p, i);
				goto skipInsMenu;
			}
		}
		changeMenu = True;
		InsertMenuItem(p->menu, "\p ", 0);				/* use SetMenuItemText to ignore any meta-chars in eStr */
		if (eStr[1]=='-') eStr[1] = '–';				/* prevent dotted line in menu! */
		SetMenuItemText(p->menu, 1, eStr);
		InsertMenuItem(p->menu, "\p(-", 1);
		ChangePopUpChoice(p, 1);
	}
	else
		ChangePopUpChoice(p, 0);
	
skipInsMenu:
	if (!DoUserPopUp(p))	*itemHit = 0;
	
	if (changeMenu) {
		SetItemMark(p->menu, p->currentChoice, 0);
		DeleteMenuItem(p->menu, 1);						/* top item */
		DeleteMenuItem(p->menu, 1);						/* divider */
		ChangePopUpChoice(p, p->currentChoice-2);
	}
	
	/* This makes it unnecessary to have a separate menu handle for each popup that
	 * shares a menu resource.
	 */
	SetItemMark(p->menu, p->currentChoice, 0);

	portRect = GetQDPortBounds();
	ClipRect(&portRect);
	return True;
}


/* If mouse is over an edit field, and the field is either currently open or
 * <anyField> is True, set cursor to I-beam. Otherwise, set to arrow.
 */

void DialogAdjustCursor(DialogPtr dlog, EventRecord *evt, Boolean anyField)
{
	CursHandle	iBeam;
	Handle		hndl;
	short		curItem, curField, type;
	Rect		box;
	Point		localWhere;
	
	localWhere = evt->where;
	GlobalToLocal(&localWhere);

	curItem = FindDialogItem(dlog, localWhere) + 1;
	if (curItem == 0)											/* not over any item */
		goto setArrow;

	GetDialogItem(dlog, curItem, &type, &hndl, &box);
	if (type==editText && ((type&itemDisable)==0)) {			/* an enabled editText item */
		curField = GetDialogKeyboardFocusItem(dlog);
		if (curItem==curField || anyField) {
			if (PtInRect(localWhere, &box)) {
				iBeam = GetCursor(iBeamCursor);
				if (iBeam != NULL) {
					SetCursor(*iBeam);
					return;
				}
			}
		}
	}
	
setArrow:
	ArrowCursor();
}
