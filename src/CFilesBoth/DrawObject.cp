/******************************************************************************************
*	FILE:	DrawObject.c
*	PROJ:	Nightingale
*	DESC:	General object-drawing routines
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void D2ObjRect(DRect *pdEnclBox, Rect *pobjRect);
static void DrawPageNum(Document *, LINK, Rect *, PCONTEXT);
static void DrawMeasNum(Document *, DDIST, DDIST, short, PCONTEXT);
static void ShadeMeasureInRect(Rect *pMRect, PCONTEXT pContext, Pattern *shadePat);
static void ShadeDurPblmMeasure(Document *, LINK, PCONTEXT);
static void DrawPartName(Document *, LINK, short, SignedByte, DDIST, Rect *, CONTEXT []);
static void DrawInstrInfo(Document *, short, Rect *, CONTEXT []);
static void DrawHairpin(LINK, LINK, PCONTEXT, DDIST, DDIST, Boolean);
static void DrawEnclosure(Document *, short, DRect *, PCONTEXT);
static Boolean GetGraphicDBox(Document *, LINK, Boolean, PCONTEXT, short, short, short, DRect *);
static void DrawGRPICT(Document *, DDIST, DDIST, short, Handle, PCONTEXT, Boolean);
static void DrawArpSign(Document *, DDIST, DDIST, DDIST, short, PCONTEXT, Boolean);
static DDIST DrawGRDraw(Document *, DDIST, DDIST, DDIST, DDIST, short, PCONTEXT, Boolean,
						Boolean, Boolean *); 
static short CountTextLines(StringPtr pString);
static Boolean GetTempoDBox(Document *, LINK, Boolean, short, short, short, DRect *);
static void DrawBarline(Document *, LINK, short, short, CONTEXT [], SignedByte);


/* ------------------------------------------------------------------------- D2ObjRect -- */
/* Convert DRect to Rect. If Rect has zero width or height, move the edges apart
slightly. Intended for computing objRects, which aren't useful if they're empty! But
note that this doesn't guarantee non-empty objRects: if the edges cross, it does
nothing. Perhaps we should swap them in that case?  */

static void D2ObjRect(DRect *pdEnclBox, Rect *pobjRect)
{
	D2Rect(pdEnclBox, pobjRect);
	if (pobjRect->bottom==pobjRect->top) {
		pobjRect->bottom++;
		pobjRect->top--;
	}
	if (pobjRect->right==pobjRect->left) {
		pobjRect->right++;
		pobjRect->left--;
	}
}


/* ------------------------------------------------------------------ DrawHeaderFooter -- */
/* Draw a page header/footer. */

static void DrawHeaderFooter(Document *doc,
								LINK pL,					/* PAGE object */
								Rect *paper,
								CONTEXT context[])
{
	PPAGE	p;
	short	pageNum, fontSize, fontInd, fontID, oldFont, oldSize, oldStyle,
			xp, yp;
	short	lxpt, chxpt=0, rhxpt=0, cfxpt=0, rfxpt=0, hypt, fypt;
	DDIST	lnSpace;
	Str255	lhStr, chStr, rhStr, lfStr, cfStr, rfStr;

	p = GetPPAGE(pL);
	pageNum = p->sheetNum + doc->firstPageNumber;
	if (pageNum < doc->startPageNumber) return;
	
	lnSpace = LNSPACE(&context[1]);
	fontSize = GetTextSize(doc->relFSizePG, doc->fontSizePG, lnSpace);
	fontInd = FontName2Index(doc, doc->fontNamePG);
	fontID = doc->fontTable[fontInd].fontID;

	if (doc->alternatePGN && !ODD(pageNum)) {
		GetHeaderFooterStrings(doc, rhStr, chStr, lhStr, pageNum, True, True);
		GetHeaderFooterStrings(doc, rfStr, cfStr, lfStr, pageNum, True, False);
	}
	else {
		GetHeaderFooterStrings(doc, lhStr, chStr, rhStr, pageNum, True, True);
		GetHeaderFooterStrings(doc, lfStr, cfStr, rfStr, pageNum, True, False);
	}

	hypt = doc->headerFooterMargins.top;
	fypt = doc->origPaperRect.bottom - doc->headerFooterMargins.bottom;

	/* Adjust positions to get approx. correct margin, since this is text baseline */
	
	hypt += fontSize/2;
	fypt += fontSize/2;

	lxpt = doc->headerFooterMargins.left;

	if (Pstrlen(chStr)>0) {
		short width = NPtStringWidth(doc, chStr, fontID, fontSize, doc->fontStylePG);
		chxpt = doc->origPaperRect.right/2 - width/2;
	}
	if (Pstrlen(cfStr)>0) {
		short width = NPtStringWidth(doc, cfStr, fontID, fontSize, doc->fontStylePG);
		cfxpt = doc->origPaperRect.right/2 - width/2;
	}
	if (Pstrlen(rhStr)>0) {
		short width = NPtStringWidth(doc, rhStr, fontID, fontSize, doc->fontStylePG);
		rhxpt = (doc->origPaperRect.right - doc->headerFooterMargins.right) - width;
	}
	if (Pstrlen(rfStr)>0) {
		short width = NPtStringWidth(doc, rfStr, fontID, fontSize, doc->fontStylePG);
		rfxpt = (doc->origPaperRect.right - doc->headerFooterMargins.right) - width;
	}

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			
			SetFontFromTEXTSTYLE(doc, (TEXTSTYLE *)doc->fontNamePG, lnSpace);

			if (Pstrlen(lhStr)>0) {
				xp = pt2p(lxpt); yp = pt2p(hypt);
				MoveTo(paper->left+xp, paper->top+yp);
				DrawString(lhStr);
			}
			if (Pstrlen(chStr)>0) {
				xp = pt2p(chxpt); yp = pt2p(hypt);
				MoveTo(paper->left+xp, paper->top+yp);
				DrawString(chStr);
			}
			if (Pstrlen(rhStr)>0) {
				xp = pt2p(rhxpt); yp = pt2p(hypt);
				MoveTo(paper->left+xp, paper->top+yp);
				DrawString(rhStr);
			}
			if (Pstrlen(lfStr)>0) {
				xp = pt2p(lxpt); yp = pt2p(fypt);
				MoveTo(paper->left+xp, paper->top+yp);
				DrawString(lfStr);
			}
			if (Pstrlen(cfStr)>0) {
				xp = pt2p(cfxpt); yp = pt2p(fypt);
				MoveTo(paper->left+xp, paper->top+yp);
				DrawString(cfStr);
			}
			if (Pstrlen(rfStr)>0) {
				xp = pt2p(rfxpt); yp = pt2p(fypt);
				MoveTo(paper->left+xp, paper->top+yp);
				DrawString(rfStr);
			}
			
			TextFont(oldFont);
			TextSize(oldSize);
			TextFace(oldStyle);
			break;
		case toPostScript:
			if (Pstrlen(lhStr)>0)
				PS_FontString(doc, pt2d(lxpt), pt2d(hypt), lhStr, doc->fontNamePG, fontSize, doc->fontStylePG);
			if (Pstrlen(chStr)>0)
				PS_FontString(doc, pt2d(chxpt), pt2d(hypt), chStr, doc->fontNamePG, fontSize, doc->fontStylePG);
			if (Pstrlen(rhStr)>0)
				PS_FontString(doc, pt2d(rhxpt), pt2d(hypt), rhStr, doc->fontNamePG, fontSize, doc->fontStylePG);
			if (Pstrlen(lfStr)>0)
				PS_FontString(doc, pt2d(lxpt), pt2d(fypt), lfStr, doc->fontNamePG, fontSize, doc->fontStylePG);
			if (Pstrlen(cfStr)>0)
				PS_FontString(doc, pt2d(cfxpt), pt2d(fypt), cfStr, doc->fontNamePG, fontSize, doc->fontStylePG);
			if (Pstrlen(rfStr)>0)
				PS_FontString(doc, pt2d(rfxpt), pt2d(fypt), rfStr, doc->fontNamePG, fontSize, doc->fontStylePG);
			break;
	}
}


/* ----------------------------------------------------------------------- DrawPageNum -- */
/* Draw a page number. */

static void DrawPageNum(Document *doc,
						LINK pL,					/* PAGE object */
						Rect *paper,
						CONTEXT context[])
{
	PPAGE	p;
	short	pageNum, fontSize, oldFont, oldSize, oldStyle,
			xpt, ypt, xp, yp, hPosPgn;
	DDIST	lnSpace;
	Str31	pageNumStr;

	p = GetPPAGE(pL);
	pageNum = p->sheetNum+doc->firstPageNumber;
	if (pageNum<doc->startPageNumber) return;
	
	NumToString(pageNum, pageNumStr);
	lnSpace = LNSPACE(&context[1]);
	fontSize = GetTextSize(doc->relFSizePG, doc->fontSizePG, lnSpace);

	if (doc->topPGN)	ypt = doc->headerFooterMargins.top;
	else				ypt = doc->origPaperRect.bottom-doc->headerFooterMargins.bottom;

	/* Adjust position to get approx. correct margin, since this is text baseline */
	
	ypt += fontSize/2;
	
	if (doc->hPosPGN==CENTER) xpt = doc->origPaperRect.right/2;
	else {
		/* Handle alternating sides of the page, if requested */
		
		hPosPgn = doc->hPosPGN;
		if (doc->alternatePGN && ODD(p->sheetNum))
			hPosPgn = (LEFT_SIDE+RIGHT_SIDE-hPosPgn);
		if (hPosPgn==LEFT_SIDE) xpt = doc->headerFooterMargins.left;
		else					xpt = doc->origPaperRect.right-doc->headerFooterMargins.right;
	}

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			
			SetFontFromTEXTSTYLE(doc, (TEXTSTYLE *)doc->fontNamePG, lnSpace);

			xp = pt2p(xpt); yp = pt2p(ypt);
			MoveTo(paper->left+xp, paper->top+yp);
			DrawString(pageNumStr);
			
			TextFont(oldFont);
			TextSize(oldSize);
			TextFace(oldStyle);
			break;
		case toPostScript:
			PS_FontString(doc, pt2d(xpt), pt2d(ypt), pageNumStr, doc->fontNamePG, fontSize,
								doc->fontStylePG);
			break;
	}
}


/* -------------------------------------------------------------------------- DrawPAGE -- */
/* Draw a PAGE object, including the header/footer/page number, if they should be shown. */

void DrawPAGE(Document *doc, LINK pL, Rect *paper, CONTEXT context[])
{
	if (!doc->masterView) {
		if (doc->useHeaderFooter)
			DrawHeaderFooter(doc, pL, paper, context);
		else
			DrawPageNum(doc, pL, paper, context);
	}
}


/* ------------------------------------------------------------------------ DrawSYSTEM -- */
/* Draw a SYSTEM object from given document, in page whose window coordinates are
specified in <paper>.  */

void DrawSYSTEM(Document *doc,
				LINK pL,					/* SYSTEM object */
				Rect *paper,				/* window coordinates of page */
				CONTEXT context[])
{
	short		i;
	Rect		r, temp;
	PCONTEXT	pContext;					/* ptr to current context[] entry */
	PSYSTEM		p;
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};

	p = GetPSYSTEM(pL);
	pContext = &context[1];
	for (i=1; i<=MAXSTAVES; i++, pContext++) {			/* update context */
		pContext->paper = *paper;
		pContext->systemTop = p->systemRect.top;
		pContext->systemLeft = p->systemRect.left;
		pContext->systemBottom = p->systemRect.bottom;
		pContext->visible = pContext->inMeasure = False;
	}
	switch (outputTo) {									/* draw the System */
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			D2ObjRect(&p->systemRect, &r);
			temp = r; OffsetRect(&r, paper->left, paper->top);
			LinkOBJRECT(pL) = temp;
			break;
		case toPostScript:
			break;
	}
	if (doc->masterView && LinkSEL(pL))
		CheckSYSTEM(doc, pL, context, NULL, SMHilite, stfRange, enlarge);
}


/* -------------------------------------------------------------- SetFontFromTEXTSTYLE -- */
/* For screen only: set the port's text characteristics from the given TEXTSTYLE. */

void SetFontFromTEXTSTYLE(Document *doc, TEXTSTYLE *pTextStyle, DDIST lineSpace)
{
	short fontInd, fontSize;
	
	fontInd = FontName2Index(doc, pTextStyle->fontName);			/* Should never fail */
	TextFont(doc->fontTable[fontInd].fontID);
	
	fontSize = GetTextSize(pTextStyle->relFSize, pTextStyle->fontSize, lineSpace);
	TextSize(UseTextSize(fontSize, doc->magnify));

	TextFace(pTextStyle->fontStyle);
}


/* ---------------------------------------------------------------------- DrawPartName -- */
/* If the given staff is the last staff of its part, draw the part name at the left end
of the system vertically centered between the top and bottom visible staves of the part.
If no staves of the part are visible, do nothing. We draw the part name only for the
last staff of the part to insure it's only drawn once and because, for unclear reasons,
the context for the last staff seems not be known until it's drawn. */


static void DrawPartName(Document *doc, LINK staffL,
						short staffn,
						SignedByte nameCode,		/* NONAMES, ABBREVNAMES, or FULLNAMES */
						DDIST indent,
						Rect *paper,
						CONTEXT	context[]
						)
{
	short		oldFont, oldSize, oldStyle, firstStaff, lastStaff,
				xname, yname, fontInd, fontID, fontSize, nameWidth;
	DDIST		ydTop, ydBot, xd, yd, connWidth;
	LINK		partL;
	PPARTINFO	pPart;
	PCONTEXT	pContext;
	Str255		str;
	
	if (nameCode==NONAMES) return;
	
	pContext = &context[staffn];
	partL = FindPartInfo(doc, Staff2Part(doc, staffn));
	pPart = GetPPARTINFO(partL);
	if (staffn==pPart->lastStaff) {
		VisStavesForPart(doc, staffL, partL, &firstStaff, &lastStaff);
		if (firstStaff<0 && lastStaff<0) return;
		
		ydTop = context[firstStaff].staffTop;
		ydBot = context[lastStaff].staffTop + context[lastStaff].staffHeight;
		yd = ((long)ydTop+(long)ydBot)/2L;

		strcpy((char *)str, (nameCode==ABBREVNAMES? pPart->shortName : pPart->name));
		CToPString((char *)str);
		
		fontSize = GetTextSize(doc->relFSizePN, doc->fontSizePN, LNSPACE(pContext));

		if (config.leftJustPN) {
			/* Get position to left-justify the part name in the left indent area. */
			xd = pContext->systemLeft-indent;
		}
		else {
			/* Get position to horizontally center the part name in the left indent area. */
			fontInd = FontName2Index(doc, doc->fontNamePN);				/* Should never fail */
			fontID = doc->fontTable[fontInd].fontID;
			nameWidth = NPtStringWidth(doc, str, fontID, fontSize, doc->fontStylePN);
			connWidth = ConnectDWidth(doc->srastral, CONNECTCURLY);
			xd = pContext->systemLeft-connWidth-indent/2-pt2d(nameWidth/2);
		}
		
		switch (outputTo) {
			case toScreen:
			case toBitmapPrint:
			case toPICT:
                {
					oldFont = GetPortTxFont();
					oldSize = GetPortTxSize();
					oldStyle = GetPortTxFace();
					
					SetFontFromTEXTSTYLE(doc, (TEXTSTYLE *)doc->fontNamePN, LNSPACE(pContext));
					
					xname = paper->left+d2p(xd);
					
					/* To center vertically, one would expect to need to add txSize/2, but
					   that gives too low a position, maybe bcs of ascender/descender ht. */
					   
					short txSize = GetPortTxSize();
					yname = paper->top+d2p(yd)+(txSize/3);
					MoveTo(xname, yname);
					DrawString(str);
	
					TextFont(oldFont);
					TextSize(oldSize);
					TextFace(oldStyle);
                }
				break;
			case toPostScript:
                {
					/* To center vertically, one would expect to need to add fontSize/2, but
					   that gives too low a position, maybe bcs of ascender/descender ht. */
					   
					yd += pt2d(fontSize/4);
					PS_FontString(doc, xd, yd, str, doc->fontNamePN, fontSize, doc->fontStylePN);
                }
				break;
		}
	}
}


/* --------------------------------------------------------------------- DrawInstrInfo -- */
/* If the given staff is the first staff of its part, draw the part's instrument   
information across the left end of the staff. Intended for use in Master Page. */

#define MIN_INSTRINFO_HT 6		/* Min. staff ht to show instr. info (pixels) */

static void DrawInstrInfo(Document *doc, short staffn, Rect *paper, CONTEXT context[])
{
	short		oldFont, oldSize, oldStyle, xname, yname, staffHtPixels, fontSize;
	DDIST		xd, yd;
	LINK		partL;
	PPARTINFO	pPart;
	PCONTEXT	pContext;
	char		fmtStrCP[256], fmtStrB[256], fmtStrT[256];
	
	pContext = &context[staffn];
	partL = Staff2PartL(doc,doc->masterHeadL,staffn);

	pPart = GetPPARTINFO(partL);
	if (staffn==pPart->firstStaff) {
		xd = pContext->systemLeft+pt2d(10);
		yd = context[staffn].staffTop+context[staffn].staffHeight;

		GetIndCString(fmtStrCP, MPGENERAL_STRS, 9);				/* " %s: channel %d patch %d" */
		sprintf(strBuf, fmtStrCP, pPart->name, pPart->channel,
					pPart->patchNum);
		GetIndCString(fmtStrB, MPGENERAL_STRS, 7);				/* ", balance %d" */
		GetIndCString(fmtStrT, MPGENERAL_STRS, 8);				/* ", transpose %d" */
		if (pPart->partVelocity!=0) sprintf(&strBuf[strlen(strBuf)], fmtStrB,
					pPart->partVelocity);
		if (pPart->transpose!=0) sprintf(&strBuf[strlen(strBuf)], fmtStrT,
					pPart->transpose);
		sprintf(&strBuf[strlen(strBuf)], " ");
		
		switch (outputTo) {
			case toScreen:
			case toBitmapPrint:
			case toPICT:
			
				/* To keep things from getting confused-looking, especially considering
				   hiliting, the easiest thing is just to be sure the labelling is always
				   entirely within the staff. */
				
				staffHtPixels = d2p(LNSPACE(&context[staffn])*4);
				if (staffHtPixels>=MIN_INSTRINFO_HT) {
					oldFont = GetPortTxFont();
					oldSize = GetPortTxSize();
					oldStyle = GetPortTxFace();
					
					if (staffHtPixels>=32)		 SetFont(4);
					else if (staffHtPixels>=16) SetFont(1);
					else if (staffHtPixels>=10) SetFont(3);
					else {
					
						/* Probably too small to read but user will be able to see there's
						   something there they can zoom in on. */
						   
						TextFont(textFontNum);
						fontSize = staffHtPixels-3;
						if (fontSize<1) fontSize = 1;
						TextSize(fontSize);
						TextFace(0);									/* Plain */
					}
					TextMode(srcCopy);
					
					xname = paper->left+d2p(xd);
					yname = paper->top+d2p(yd)-(staffHtPixels/8+1);
					MoveTo(xname, yname);
					DrawCString(strBuf);
	
					TextFont(oldFont);
					TextSize(oldSize);
					TextFace(oldStyle);
					TextMode(srcOr);
				}
				break;
			default:
				CToPString(strBuf);
				fontSize = 3*LNSPACE(&context[staffn]);
				PS_FontString(doc, xd, yd-(4*LNSPACE(&context[staffn]))/6, (StringPtr)strBuf,
									"\pHelvetica", fontSize, bold);
				break;
		}
	}
}


/* ------------------------------------------------------------------------ Draw1Staff -- */

void Draw1Staff(Document *doc,
				short /* staffn */,
				Rect *paper,
				PCONTEXT pContext,
				short ground			/* _STAFF foreground/background code; see enum */
				)
{
	DDIST	yd;
	short	nLines,			/* Number of lines in the staff */
			line,
			showLines;		/* 0, 1, or SHOW_ALL_LINES */

	nLines = pContext->staffLines;
	showLines = pContext->showLines;
//LogPrintf(LOG_DEBUG, "Draw1Staff: staffn=%d ground=%d nLines=%d showLines=%d staffTop=%d staffHeight=%d\n",
//		staffn, ground, nLines, showLines, pContext->staffTop, pContext->staffHeight);
	
	switch (outputTo) {
		case toScreen:
			if (ground==BACKGROUND_STAFF) {
				ForeColor(blueColor);
				if (config.solidStaffLines) PenNormal();
				else PenPat(NGetQDGlobalsGray());
			}
			else {
				ForeColor(blackColor);
				if (ground==OTHERSYS_STAFF)
					PenPat(NGetQDGlobalsGray());
				else if (ground==SECONDSYS_STAFF)
					PenPat(NGetQDGlobalsDarkGray());
				else PenNormal();
			}
			if (d2p(LNSPACE(pContext))>15) PenSize(1, 2);
			/* Fall through */
		case toBitmapPrint:
		case toPICT:
			if (showLines>0) {
				for (line=0; line<nLines; line++) {
					yd = pContext->staffTop + 
							halfLn2d(2*line, pContext->staffHeight, nLines);
					if (showLines!=1 || line==(nLines-1)/2) {
						MoveTo(paper->left+d2p(pContext->staffLeft), paper->top+d2p(yd));
						LineTo(paper->left+d2p(pContext->staffRight), paper->top+d2p(yd));
					}
				}
			}
			PenNormal();
			ForeColor(blackColor);
			break;
		case toPostScript:
			/* CER: 7.2005 Staff lines are too thin for 2 line staves. Put a floor of
					24 under the font size.
			   CER 11.2006 Rastral 8 2stf lines => ptSize of 3 */
			
			short ptSize;
			if (nLines == 5) {
				ptSize = d2pt(pContext->staffHeight)+config.musFontSizeOffset;
			}
			else {
				short stfHeight = drSize[doc->srastral];
				ptSize = d2pt(stfHeight)+config.musFontSizeOffset;
			}
			
			if (ptSize < 3) ptSize = 3;
			if (ptSize >= 3)	PS_MusSize(doc, ptSize);
			else				PS_MusSize(doc, 3);
			
			if (showLines>0) {
				for (line=0; line<nLines; line++) {
					yd = pContext->staffTop + 
							halfLn2d(2*line, pContext->staffHeight, nLines);
					if (showLines!=1 || line==(nLines-1)/2)
						PS_StaffLine(yd, pContext->staffLeft, pContext->staffRight);
				}
			}
			
			/* Leave the routine with point size as before calculated. */
			
			PS_MusSize(doc, ptSize);
			break;
		default:
			;
	}
}


/* ------------------------------------------------------------------------- DrawSTAFF -- */
/* Draw all subobjects of a STAFF object and update their context[] entries. */

void DrawSTAFF(Document *doc, LINK pL, Rect *paper,
				CONTEXT context[],
				short ground,				/* _STAFF foreground/background code; see enum */
				short hilite
				)
{
	PASTAFF		aStaff;				/* ptr to current sub item */
	LINK		aStaffL;
	PSYSTEM		pSystem;
	PCONTEXT	pContext;			/* ptr to current context[] entry */
	short		staffn;				/* current staff number */
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};

PushLock(OBJheap);
PushLock(STAFFheap);
	pSystem = GetPSYSTEM(StaffSYS(pL));
	aStaffL = FirstSubLINK(pL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		staffn = StaffSTAFF(aStaffL);
		pContext = &context[staffn];
		
		pContext->staffTop = aStaff->staffTop + pContext->systemTop;
		pContext->staffLeft = aStaff->staffLeft + pContext->systemLeft;
		pContext->staffRight = aStaff->staffRight + pContext->systemLeft;
		pContext->staffVisible = pContext->visible = aStaff->visible;

		pContext->staffLines = aStaff->staffLines;
		pContext->showLines = aStaff->showLines;
		pContext->staffHeight = aStaff->staffHeight;
		pContext->staffHalfHeight = pContext->staffHeight>>1;
		pContext->fontSize = aStaff->fontSize;

		if (aStaff->visible && pContext->staffVisible)
				Draw1Staff(doc, staffn, paper, pContext, ground);

		switch (outputTo) {
			case toScreen:
			case toBitmapPrint:
			case toPICT:
				if (doc->masterView) {
					if (ground==TOPSYS_STAFF)
						DrawInstrInfo(doc, staffn, paper, context);
				}
				else if (pSystem->systemNum==1)
					DrawPartName(doc, pL, staffn, doc->firstNames, doc->dIndentFirst,
											paper, context);
				else
					DrawPartName(doc, pL, staffn, doc->otherNames, doc->dIndentOther,
											paper, context);
				break;
			case toPostScript:
				if (doc->masterView)
					;
				else if (pSystem->systemNum==1)
					DrawPartName(doc, pL, staffn, doc->firstNames, doc->dIndentFirst,
											paper, context);
				else
					DrawPartName(doc, pL, staffn, doc->otherNames, doc->dIndentOther,
											paper, context);
				break;
		}
	}

	if (doc->showFormat && LinkSEL(pL) && hilite)
		CheckSTAFF(doc, pL, context, NULL, SMHilite, stfRange, enlarge);

	if (doc->masterView && LinkSEL(pL) && hilite)
		CheckSTAFF(doc, pL, context, NULL, SMHilite, stfRange, enlarge);

PopLock(OBJheap);
PopLock(STAFFheap);
}


/* ----------------------------------------------------------------------- DrawCONNECT -- */
/* Draw all subobjects of a CONNECT object. */

void DrawCONNECT(Document *doc, LINK pL,
				CONTEXT context[],
				short ground			/* _STAFF foreground/background code; see enum */
				)
{
	PACONNECT	aConnect;				/* ptr to current subobj */
	LINK		aConnectL, staffL;
	PCONTEXT	pContext;				/* ptr to current context[] entry */
	Rect		cBox;					/* bounding box of curly brace */
	short		width,					/* Width of curly brace */
				xwidth, brackHt,
				px, pyTop, pyBot,
				stfA, stfB;
	DDIST 		xd,
				dTop, dBottom;			/* top of above staff and bottom of below staff */
	DDIST		dLeft,					/* left edge of staff */
				curlyWider;
	Boolean		entire;					/* Does connect include the entire system? */
	PicHandle	bracePicH;				/* Handle to brace PICT */
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};

PushLock(OBJheap);
PushLock(CONNECTheap);

	staffL = LSSearch(pL, STAFFtype, ANYONE, GO_LEFT, False);			/* must exist */

	aConnectL = FirstSubLINK(pL);
	for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
		if (!ShouldDrawConnect(doc, pL, aConnectL, staffL)) continue;

		/* We now know at least one staff in the Connect's range is visible. */
		
		aConnect = GetPACONNECT(aConnectL);
		entire = aConnect->connLevel==SystemLevel;

		if (!entire && aConnect->staffBelow>doc->nstaves && !doc->masterView) {
			MayErrMsg("DrawCONNECT: bad staffBelow at L%ld", (long)pL);
			break;
		}

		if (doc->masterView)
			pContext = entire ? &context[FirstStaffn(pL)] :
									&context[stfA=aConnect->staffAbove];
		else
			pContext = entire ? &context[FirstStaffn(pL)] :
								&context[stfA=NextVisStaffn(doc, pL, True, aConnect->staffAbove)];
		dLeft = pContext->staffLeft;
		dTop = pContext->staffTop;

		if (doc->masterView)
			pContext = entire ? &context[LastStaffn(pL)] :
									&context[stfB=aConnect->staffBelow];
		else
			pContext = entire ? &context[LastStaffn(pL)] :
								&context[stfB=NextVisStaffn(doc, pL, False, aConnect->staffBelow)];
		dBottom = pContext->staffTop + pContext->staffHeight;
		xd = dLeft+aConnect->xd;
		
		switch (aConnect->connectType) {
			case CONNECTLINE:
				switch (outputTo) {
					case toScreen:
					case toBitmapPrint:
					case toPICT:
						px = pContext->paper.left+d2p(xd); 
						pyTop = pContext->paper.top+d2p(dTop);
						pyBot = pContext->paper.top+d2p(dBottom);
						MoveTo(px, pyTop); LineTo(px, pyBot);
						break;
					case toPostScript:
						PS_ConLine(dTop, dBottom, xd);
						break;
				}
				break;
			case CONNECTCURLY:
				if (!config.bracketsForBraces) {
#if 0
					curlyWider = ConnectDWidth(doc->srastral, CONNECTCURLY)
										-ConnectDWidth(doc->srastral, CONNECTBRACKET);
					xd -= curlyWider;
#endif
					switch (outputTo) {
						case toScreen:
						case toBitmapPrint:
						case toPICT:
							bracePicH = (PicHandle)GetResource('PICT', BRACE_ID);
							if (!GoodResource((Handle)bracePicH)) {
								SysBeep(10);
								LogPrintf(LOG_WARNING, "Can't get curly brace resource.  (DrawConnect)\n");
								goto Cleanup;
							}
							cBox = (*bracePicH)->picFrame;
							FIX_END(cBox.top); FIX_END(cBox.left);
							FIX_END(cBox.bottom); FIX_END(cBox.right);
							
							/* The BRACE_ID PICT was created from the Sonata 36 brace. */
							
							width = ((cBox.right-cBox.left)*(UseTextSize(pContext->fontSize,
																		doc->magnify))) / 36;
							SetRect(&cBox, d2p(xd), d2p(dTop), d2p(xd)+width, d2p(dBottom));
							OffsetRect(&cBox, pContext->paper.left, pContext->paper.top);
							
							/* FIXME: DrawPicture OVERWRITES ANYTHING ALREADY THERE. Better to
							   "or" in, probably via DrawPicture to bitmap, then CopyBits. */
							   
							DrawPicture(bracePicH, &cBox);
							HUnlock((Handle)bracePicH); HPurge((Handle)bracePicH);
							if (doc->masterView && ground==OTHERSYS_STAFF) {
								PenPat(NGetQDGlobalsGray());
								PenMode(notPatBic);
								PaintRect(&cBox);
								PenNormal();
							}
							break;
						case toPostScript:
							width = pt2d(5);	/* FIXME: WRONG: needs to track current fontsize */
							PS_Brace(doc, xd,dTop,dBottom);
							break;
					}
					break;
				}
				else {
					/* Substituting brackets for curly brace; tweak before falling thru */
					
					curlyWider = ConnectDWidth(doc->srastral, CONNECTCURLY)
										-ConnectDWidth(doc->srastral, CONNECTBRACKET);
					xd += curlyWider;
				}								/* Drop thru... */
			case CONNECTBRACKET:				/* Must immediately follow case CONNECTCURLY */
				switch (outputTo) {
					case toScreen:
					case toBitmapPrint:
					case toPICT:
						px = pContext->paper.left+d2p(xd);
						pyTop = pContext->paper.top+d2p(dTop);
						pyBot = pContext->paper.top+d2p(dBottom);
						xwidth = CharWidth(MCH_topbracket) * .36275;
						
						/* Add fudge factor for weird round-off error at one magnification */
						
						if (doc->magnify == 0) xwidth++;
						PenSize(xwidth>1 ? xwidth : 2, 1);
						MoveTo(px, pyTop);  DrawChar(MCH_topbracket);
						MoveTo(px, pyTop);
						LineTo(px, pyBot);
						Move(0,1);  DrawChar(MCH_bottombracket);
						if (doc->masterView && ground==OTHERSYS_STAFF) {
							PenPat(NGetQDGlobalsGray());
							PenMode(notPatBic);
							
							/* Assume brackets are almost square: good enough for this */
							
							brackHt = CharWidth(MCH_topbracket)+1;
							SetRect(&cBox, px, pyTop-brackHt, px+2*xwidth, pyBot+brackHt);
							PaintRect(&cBox);
						}
						PenNormal();
						break;
					case toPostScript:
						PS_Bracket(doc, xd, dTop, dBottom);
						break;
				}
				break;
		}
	}
	if (doc->masterView && LinkSEL(pL))
		CheckCONNECT(doc, pL, context, NULL, SMHilite, stfRange, enlarge);

Cleanup:

PopLock(OBJheap);
PopLock(CONNECTheap);
}


/* -------------------------------------------------------------------------- DrawCLEF -- */
/* Draw a CLEF object. Handles zero-sized objRects for invisible clefs. */

void DrawCLEF(Document *doc, LINK pL, CONTEXT context[])
{
	PACLEF		aClef;				/* ptr to current subobject */
	LINK		aClefL;
	short		i;					/* scratch */
	PCONTEXT	pContext;			/* ptr to current context[] entry */
	DDIST		xd, yd,				/* scratch DDIST coordinates */
				xdOct, ydOct,
				lnSpace;
	short		xp, yp, yp2,		/* pixel coordinates */
				xpObj, ypObj,
				xp2Obj, yp2Obj,
		 		oldTxSize,
				sizePercent;		/* Percent of "normal" size to draw in */
	unsigned char glyph;			/* clef symbol */
	Boolean		drawn,				/* False until a subobject has been drawn */
				clefVisible;		/* True if clef contains a visible subobject */
	Point		pt;
	Byte		octGlyph;

PushLock(OBJheap);
PushLock(CLEFheap);

	aClefL = FirstSubLINK(pL);
	drawn = clefVisible = False;
	oldTxSize = GetPortTxSize();
	
	for (i=0; i<LinkNENTRIES(pL); i++, aClefL=NextCLEFL(aClefL)) {

		pContext = &context[ClefSTAFF(aClefL)];
		if (!pContext->staffVisible) continue;
		MaySetPSMusSize(doc, pContext);

		aClef = GetPACLEF(aClefL);
		sizePercent = (aClef->small? SMALLSIZE(100) : 100);
		GetClefDrawInfo(doc, pL, aClefL, context, sizePercent, &glyph, &xd, &yd, &xdOct,
								&ydOct);
		glyph = MapMusChar(doc->musFontInfoIndex, glyph);
		octGlyph = MapMusChar(doc->musFontInfoIndex, CLEF8_ITAL? MCH_idigits[8] : '8');
		lnSpace = LNSPACE(pContext);
		xd += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace));
		yd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace));
		xdOct += MusCharXOffset(doc->musFontInfoIndex, octGlyph, lnSpace);
		ydOct += MusCharYOffset(doc->musFontInfoIndex, octGlyph, lnSpace);

		aClef = GetPACLEF(aClefL);
		switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = d2p(xd);
			MoveTo(pContext->paper.left+xp, pContext->paper.top+d2p(yd));
			if (aClef->visible || doc->showInvis) {
				clefVisible = True;
				DrawChar(glyph);
				if (xdOct>(DDIST)NRV_CANCEL) {
					MoveTo(pContext->paper.left+d2p(xdOct), pContext->paper.top+d2p(ydOct));
					DrawChar(octGlyph);
				}
			}
			if (!LinkVALID(pL) && outputTo==toScreen) {
				GetPen(&pt);
				pt.h -= pContext->paper.left;
				pt.v -= pContext->paper.top;
				yp = d2p(pContext->staffTop - pContext->staffHalfHeight);
				yp2=d2p(pContext->staffTop+pContext->staffHeight+pContext->staffHalfHeight);
				if (drawn) {
					if (xp<xpObj) xpObj = xp;
					if (pt.h>xp2Obj) xp2Obj = pt.h;
					if (yp<ypObj) ypObj = yp;
					if (yp2>yp2Obj) yp2Obj = yp2;
				}
				else {
					xpObj = xp;
					xp2Obj = pt.h;
					ypObj = yp;
					yp2Obj = yp2;
					drawn = True;
				}
			}
			break;
		case toPostScript:
			if (aClef->visible || doc->showInvis) {
				PS_MusChar(doc, xd, yd, glyph, True, sizePercent);
				if (xdOct>(DDIST)NRV_CANCEL)
					PS_MusChar(doc, xdOct, ydOct, octGlyph, True, sizePercent);
			}
			break;
		}
	}
	
	if (outputTo==toScreen && !LinkVALID(pL)) {
		if (clefVisible) {
			SetRect(&LinkOBJRECT(pL), xpObj, ypObj, xp2Obj, yp2Obj);
			LinkVALID(pL) = True;
		}
		else {
			SetRect(&LinkOBJRECT(pL), xpObj, ypObj, xpObj, yp2Obj);
			LinkVALID(pL) = True;
		}
	}

	aClefL = FirstSubLINK(pL);
	aClef = GetPACLEF(aClefL);
	if (aClef->small) TextSize(oldTxSize);

PopLock(OBJheap);
PopLock(CLEFheap);
}


/* ------------------------------------------------------------------------ DrawKEYSIG -- */
/* Draw a KEYSIG object */

void DrawKEYSIG(Document *doc, LINK pL, CONTEXT context[])
{
	LINK		aKeySigL;
	PAKEYSIG	aKeySig;
	PCONTEXT	pContext;		/* ptr to current context[] entry */
	short		width,			/* pixel width of entire key signature */
				lines;			/* number of lines in current staff */
	DDIST		xd, yd,			/* scratch DDIST coordinates */
				dTop,
				height;			/* height of current staff */
	Rect		subRect;		/* enclosing rect for key signature */
	Boolean		drawn;			/* False until something's drawn */

PushLock(OBJheap);
PushLock(KEYSIGheap);
	aKeySigL = FirstSubLINK(pL);
	drawn = False;
	for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
	
		pContext = &context[KeySigSTAFF(aKeySigL)];
		if (!pContext->staffVisible) continue;
		MaySetPSMusSize(doc, pContext);

		GetKeySigDrawInfo(doc, pL, aKeySigL, context, &xd, &yd, &dTop, &height, &lines);

		DrawKSItems(doc, pL, aKeySigL, pContext, xd, yd, height, lines, &width, MEDraw);
	
		aKeySig = GetPAKEYSIG(aKeySigL);
		if (!LinkVALID(pL) && outputTo==toScreen) {
			SetRect(&subRect,
						d2p(xd),
						d2p(pContext->staffTop - pContext->staffHalfHeight),
						d2p(xd) + width,
						d2p(pContext->staffTop + pContext->staffHeight + 
								pContext->staffHalfHeight));
			if (drawn)
				UnionRect(&LinkOBJRECT(pL), &subRect, &LinkOBJRECT(pL));
			else {
				LinkOBJRECT(pL) = subRect;
				drawn = True;
			}
		}
	}

	if (outputTo==toScreen)
		LinkVALID(pL) = True;

PopLock(OBJheap);
PopLock(KEYSIGheap);
}


/* ----------------------------------------------------------------------- DrawTIMESIG -- */
/* Draw a TIMESIG object */

void DrawTIMESIG(Document *doc, LINK pL, CONTEXT context[])
{
	PATIMESIG	aTimeSig;
	LINK		aTimeSigL;
	PCONTEXT	pContext;
	short		subType,
				npLeft, npRight,		/* left and right pixel coordinates, numerator & denominator */
				dpLeft, dpRight,
				npTop, dpTop,
				left, right,			/* enclosing positions of current subobject */
				top, bottom;
	DDIST		xd, yd, xdN, xdD,
				ydN, ydD;
	Str31		nStr, dStr;				/* numerator & denominator */
	Boolean		drawn,					/* False until a subobject has been drawn */
				recalc;					/* True if we need to recalculate object rectangles */
	Rect		rObj;					/* bounding box */
	Point		pt;						/* scratch point */

PushLock(OBJheap);
PushLock(TIMESIGheap);

	drawn = False;
	recalc = (!LinkVALID(pL) && outputTo==toScreen);
	aTimeSigL = FirstSubLINK(pL);
	for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {

		pContext = &context[TimeSigSTAFF(aTimeSigL)];
		if (!pContext->staffVisible) continue;
		MaySetPSMusSize(doc, pContext);

		aTimeSig = GetPATIMESIG(aTimeSigL);

		GetTimeSigDrawInfo(doc, pL, aTimeSigL, pContext, &xd, &yd);
		subType = FillTimeSig(doc,aTimeSigL,pContext,nStr,dStr,xd,&xdN,&xdD,yd,&ydN,&ydD);

		/* At this point, xdN is the "numerator" left edge, ydN is the numerator
		   y-baseline, and nStr the numerator character string. Also, if subType is
		   N_OVER_D, xdD, ydD, and dStr are the same for the "denominator". */
		   
		switch (outputTo) {
			case toScreen:
			case toBitmapPrint:
			case toPICT:
				npLeft = d2p(xdN);
				dpLeft = d2p(xdD);
				npTop = d2p(ydN);
				MoveTo(pContext->paper.left+npLeft, pContext->paper.top+npTop);
				if (aTimeSig->visible || doc->showInvis)
					DrawString(nStr);
				if (recalc) {
					GetPen(&pt);
					npRight = pt.h-pContext->paper.left;
				}
				dpTop = d2p(ydD);
				MoveTo(pContext->paper.left+dpLeft, pContext->paper.top+dpTop);
				if (aTimeSig->subType==N_OVER_D && (aTimeSig->visible || doc->showInvis))
					DrawString(dStr);
				if (recalc) {
					GetPen(&pt);
					if (aTimeSig->subType==N_OVER_D) {
						dpRight = pt.h-pContext->paper.left;
						left = n_min(npLeft, dpLeft);
						right = n_max(npRight, dpRight);
					}
					else {
						left = npLeft;
						right = npRight;
					}
					
					top = d2p(yd);
					bottom = top + d2p(pContext->staffHeight);
					if (!drawn) {
						rObj.top = top;
						rObj.left = left;
						rObj.bottom = bottom;
						rObj.right = right;
						LinkOBJRECT(pL) = rObj;
						drawn = True;
					}
					else {
						rObj.top = n_min(rObj.top, top);
						rObj.left = n_min(rObj.left, left);
						rObj.bottom = n_max(rObj.bottom, bottom);
						rObj.right = n_max(rObj.right, right);
						LinkOBJRECT(pL) = rObj;
					}
				}
				break;
				
			case toPostScript:
				if (aTimeSig->visible || doc->showInvis) {
					PS_MusString(doc, xdN, ydN, nStr, 100);
					if (aTimeSig->subType==N_OVER_D)
						PS_MusString(doc, xdD, ydD, dStr, 100);
				}
				break;
		}
	}

	if (recalc)	LinkVALID(pL) = True;

PopLock(OBJheap);
PopLock(TIMESIGheap);
}


/* ----------------------------------------------------------------------- DrawHairpin -- */
/* Draw a hairpin dynamic. X-coord. of left end = xd; x-coord. of right end (relative
to the symbol it's attached to) = aDynamic->endxd. */

static void DrawHairpin(LINK pL, LINK aDynamicL, PCONTEXT pContext, DDIST xd, DDIST yd,
						Boolean reallyDraw)
{
	DDIST		lnSpace, offset, rise,
				endxd, endyd, hairThick,
				sysLeft;						/* left margin of current system */
	PADYNAMIC	aDynamic;
	short		xp, yp, endxp, endyp, papyp, papendyp,
				penThick;						/* vertical pen size in pixels */

	lnSpace = LNSPACE(pContext);
	sysLeft = pContext->systemLeft;
	aDynamic = GetPADYNAMIC(aDynamicL);

	if (SystemTYPE(DynamLASTSYNC(pL)))
		endxd = aDynamic->endxd+sysLeft;
	else
		endxd = SysRelxd(DynamLASTSYNC(pL))+aDynamic->endxd+sysLeft;
	endyd = pContext->measureTop + aDynamic->endyd;
	offset = qd2d(aDynamic->otherWidth, pContext->staffHeight, pContext->staffLines)/2;
	rise = qd2d(aDynamic->mouthWidth, pContext->staffHeight, pContext->staffLines)/2;

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			/* If staff size is large, thicken hairpin. Decide thickness in a crude way
			   similar to what we do for slurs; we could easily do better for hairpins
			   and probably should, though again, precise registration of anything against
			   hairpins is rarely important. */
			   
			penThick = d2p(lnSpace/6);
			if (penThick<1) penThick = 1;
			if (penThick>2) penThick = 2;
			PenSize(1, penThick);

			switch (DynamType(pL)) {
				case DIM_DYNAM:
					xp = d2p(xd); yp = d2p(yd); endxp = d2p(endxd); endyp = d2p(endyd);

					if (reallyDraw) {
						papyp = pContext->paper.top+yp;
						papendyp = pContext->paper.top+endyp;
						MoveTo(pContext->paper.left+xp, papyp+d2p(rise));
						LineTo(pContext->paper.left+endxp, papendyp+d2p(offset));
						MoveTo(pContext->paper.left+xp, papyp-d2p(rise));
						LineTo(pContext->paper.left+endxp, papendyp-d2p(offset));
					}
					if (!LinkVALID(pL) && outputTo==toScreen)
						GetHairpinBBox(xd, endxd, yd, endyd, rise, offset, DIM_DYNAM,
																		&LinkOBJRECT(pL));
					break;
				case CRESC_DYNAM:
					xp = d2p(xd); yp = d2p(yd); endxp = d2p(endxd); endyp = d2p(endyd);

					if (reallyDraw) {
						papyp = pContext->paper.top+yp;
						papendyp = pContext->paper.top+endyp;
						MoveTo(pContext->paper.left+xp, papyp+d2p(offset));
						LineTo(pContext->paper.left+endxp, papendyp+d2p(rise));
						MoveTo(pContext->paper.left+xp, papyp-d2p(offset));
						LineTo(pContext->paper.left+endxp, papendyp-d2p(rise));
					}
					if (!LinkVALID(pL) && outputTo==toScreen)
						GetHairpinBBox(xd, endxd, yd, endyd, rise, offset, CRESC_DYNAM,
																		&LinkOBJRECT(pL));
					break;
				default:
					;
			}
			PenNormal();
			break;
		case toPostScript:
			hairThick = (long)(config.hairpinLW*lnSpace) / 100L;
			switch (DynamType(pL)) {
				case DIM_DYNAM:
					PS_Line(xd, yd+rise, endxd, endyd+offset, hairThick);
					PS_Line(xd, yd-rise, endxd, endyd-offset, hairThick);
					break;
				case CRESC_DYNAM:
					PS_Line(xd, yd+offset, endxd, endyd+rise, hairThick);
					PS_Line(xd, yd-offset, endxd, endyd-rise, hairThick);
					break;
				default:
					;
			}
			break;
		default:
			;			
	}
}


/* ----------------------------------------------------------------------- DrawDYNAMIC -- */
/* Draw a DYNAMIC object, a set (as of v. 5.7, always one) of hairpins and/or "simple"
dynamics, and if necessary recompute its objRect. */

void DrawDYNAMIC(
			Document *doc,
			LINK pL,
			CONTEXT context[],
			Boolean doDraw)		/* True=draw if dynam. is visible, False=never draw */
{
	PADYNAMIC	aDynamic;
	LINK		aDynamicL;
	short		oldTxSize, useTxSize,
				sizePercent;	/* Percent of "normal" size to draw in */
	DDIST		xd, yd,
				lnSpace;
	short		xp, yp;			/* pixel coordinates */
	unsigned char glyph;		/* dynamic symbol */
	Boolean		drawn;			/* False until a subobject has been drawn */
	Rect		rSub;			/* bounding boxes for subobject and entire object */
	PCONTEXT	pContext;
	Boolean		reallyDraw;

PushLock(OBJheap);
PushLock(DYNAMheap);

	drawn = False;

	aDynamicL = FirstSubLINK(pL);
	pContext = &context[DynamicSTAFF(aDynamicL)];
	if (!pContext->staffVisible) goto Cleanup;
	MaySetPSMusSize(doc, pContext);
	lnSpace = LNSPACE(pContext);

	for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL)) {
		GetDynamicDrawInfo(doc, pL, aDynamicL, context, &glyph, &xd, &yd);
		aDynamic = GetPADYNAMIC(aDynamicL);
		reallyDraw = ((aDynamic->visible || doc->showInvis) && doDraw);
		
		sizePercent = (aDynamic->small? SMALLSIZE(100) : 100);
		oldTxSize = GetPortTxSize();
		useTxSize = UseMTextSize(SizePercentSCALE(pContext->fontSize), doc->magnify);

		switch (outputTo) {
			case toScreen:
			case toBitmapPrint:
			case toPICT:
				switch (DynamType(pL)) {
					case DIM_DYNAM:
					case CRESC_DYNAM:
						DrawHairpin(pL, aDynamicL, pContext, xd, yd, reallyDraw);
						break;
					default:
						TextSize(useTxSize);
						glyph = MapMusChar(doc->musFontInfoIndex, glyph);
						xd += SizePercentSCALE(MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace));
						yd += SizePercentSCALE(MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace));
						xp = d2p(xd); yp = d2p(yd);
						aDynamic = GetPADYNAMIC(aDynamicL);
						if (reallyDraw) {
							MoveTo(pContext->paper.left+xp, pContext->paper.top+yp);
							DrawMChar(doc, glyph, NORMAL_VIS, False);
						}
						if (!LinkVALID(pL) && outputTo==toScreen) {
							rSub = charRectCache.charRect[glyph];
							OffsetRect(&rSub, xp, yp);
							if (drawn)
								UnionRect(&LinkOBJRECT(pL), &rSub, &LinkOBJRECT(pL));
							else {
								drawn = True;
								LinkOBJRECT(pL) = rSub;
							}
						}
						TextSize(oldTxSize);
						break;
				}
				break;

			case toPostScript:
				if (reallyDraw)
					switch (DynamType(pL)) {
						case DIM_DYNAM:
						case CRESC_DYNAM:
							DrawHairpin(pL, aDynamicL, pContext, xd, yd, doDraw);
							break;
						default:
							glyph = MapMusChar(doc->musFontInfoIndex, glyph);
							xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
							yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
							PS_MusChar(doc, xd, yd, glyph, True, sizePercent);
					}
				break;
		}
	}

	if (outputTo==toScreen)
		LinkVALID(pL) = True;

Cleanup:

PopLock(OBJheap);
PopLock(DYNAMheap);
}


/* ------------------------------------------------------------------------ DrawRPTEND -- */
/* Draw a REPEATEND object */

void DrawRPTEND(Document *doc, LINK pL, CONTEXT context[])
{
	DDIST		xd, yd;
	Boolean		drawn,
				dotsOnly;		/* True=don't draw barline proper, only repeat dots */
	LINK		aRptL;
	PARPTEND	aRpt;
	Rect		rSub;
	PCONTEXT	pContext;
	short		connStaff;

PushLock(OBJheap);

	drawn = False;
	aRptL = FirstSubLINK(pL);
	for ( ; aRptL; aRptL = NextRPTENDL(aRptL)) {
		aRpt = GetPARPTEND(aRptL);
		
		if (aRpt->visible) {
			pContext = &context[aRpt->staffn];
			if (!pContext->staffVisible) continue;

			dotsOnly = !ShouldREDrawBarline(doc, pL, aRptL, &connStaff);
			GetRptEndDrawInfo(doc, pL, aRptL, context, &xd, &yd, &rSub);
						
			switch (outputTo) {
				case toScreen:
				case toBitmapPrint:
				case toPICT:
					DrawRptBar(doc, pL, RptEndSTAFF(aRptL), connStaff, context, xd,
									RptType(pL), MEDraw, dotsOnly);
					if (!LinkVALID(pL) && outputTo==toScreen) {		/* Set objRect here. */
						if (drawn)
							UnionRect(&LinkOBJRECT(pL), &rSub, &LinkOBJRECT(pL));
						else {
							drawn = True;
							LinkOBJRECT(pL) = rSub;
						}
					}
					break;
				case toPostScript:
					DrawRptBar(doc, pL, RptEndSTAFF(aRptL), connStaff, context, xd,
									RptType(pL), MEDraw, dotsOnly);
					break;
			}
		}
	}
	if (outputTo==toScreen)
		LinkVALID(pL) = True;
	
PopLock(OBJheap);
}


/* ------------------------------------------------------------------------ DrawENDING -- */
/* Draw an ENDING object: bracket and optional label. */

#define MOVE_AND_LINE(x1, y1, x2, y2)	{ MoveTo((x1), (y1)); LineTo((x2), (y2)); }

void DrawENDING(Document *doc, LINK pL, CONTEXT context[])
{
	DDIST dTop, xd, yd, endxd, lnSpace, rise, endThick, xdNum, ydNum;
	PENDING p;
	char numStr[MAX_ENDING_STRLEN];
	short xp, yp, endxp,  oldFont, oldSize, oldStyle, sysLeft, risePxl,
			endNum, fontSize, papLeft, papTop, strOffset;
	PCONTEXT pContext;
	CONTEXT firstContext;

 	p = GetPENDING(pL);
 	endNum = p->endNum;
	pContext = &context[p->staffn];
	if (!pContext->visible) return;

PushLock(OBJheap);

	if (LinkBefFirstMeas(pL)) {
		GetContext(doc, p->firstObjL, p->staffn, &firstContext);
		pContext = &firstContext;
	}

	sysLeft = pContext->systemLeft;
	lnSpace = LNSPACE(pContext);
	rise = ENDING_CUTOFFLEN(lnSpace);

	xd = SysRelxd(p->firstObjL)+p->xd+sysLeft;
	dTop = pContext->measureTop;
	yd = dTop+p->yd;
	endxd = SysRelxd(p->lastObjL)+p->endxd+sysLeft;
	xdNum = xd+lnSpace;
	ydNum = yd+2*lnSpace;
	fontSize = d2pt(2*lnSpace-1);
	
	if (endNum!=0 && endNum<maxEndingNum) {
		strOffset = endNum*MAX_ENDING_STRLEN;
		GoodStrncpy(numStr, &endingString[strOffset], MAX_ENDING_STRLEN-1);
	}

	/* Traditionally, ending numerals are in the "music font", i.e., the same font as
	   time signature digits, tuplet numerals, etc. However, the origins and sizes of
	   characters in Sonata and compatibles (at least) are very non-standard, so for
	   now, do something much simpler: use a standard screen font on the screen, and
	   Times on PostScript output. */
	   
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = d2p(xd);
			yp = d2p(yd);
			endxp = d2p(endxd);
			risePxl = d2p(rise);
			
			if (p->visible || doc->showInvis) {
				papLeft = pContext->paper.left;
				papTop = pContext->paper.top;
				if (!p->noLCutoff) MOVE_AND_LINE(papLeft+xp, papTop+yp+risePxl, papLeft+xp,
																			papTop+yp);
				MOVE_AND_LINE(papLeft+xp, papTop+yp, papLeft+endxp, papTop+yp);
				if (!p->noRCutoff) MOVE_AND_LINE(papLeft+endxp, papTop+yp, papLeft+endxp,
																		papTop+yp+risePxl);

				if (endNum!=0 && endNum<maxEndingNum) {
					oldFont = GetPortTxFont();
					oldSize = GetPortTxSize();
					oldStyle = GetPortTxFace();

					TextFont(SYSFONTID_SANSSERIF);
					TextSize(UseMTextSize(fontSize, doc->magnify));
					TextFace(0);										/* Plain */
		
					MoveTo(papLeft+d2p(xdNum), papTop+d2p(ydNum));
					DrawCString(numStr);
					TextFont(oldFont);
					TextSize(oldSize);
					TextFace(oldStyle);
				}
			}

			/* Make the objRect a little smaller than the enclosing Rect so it's
			   easier to select unrelated symbols that happen to be nestled inside. */
			   
			SetRect(&LinkOBJRECT(pL), xp, yp, endxp, yp+3*risePxl/4);
			break;
		case toPostScript:
			endThick = ENDING_THICK(lnSpace);
			if (!p->noLCutoff) PS_Line(xd, yd+rise, xd, yd, endThick);
			PS_Line(xd, yd, endxd, yd, endThick);
			if (!p->noRCutoff) PS_Line(endxd, yd, endxd, yd+rise, endThick);
			if (endNum!=0) {
				CToPString(numStr);
				PS_FontString(doc, xdNum, ydNum, (unsigned char *)numStr, "\pTimes",
								fontSize, 0);
			}
			break;
	}
PopLock(OBJheap);
}


/* --------------------------------------------------------------------- DrawEnclosure -- */
/* Draw an enclosure, e.g., around a rehearsal mark. As of v. 5.8.x, the only type is
a rectangle. */

static void DrawEnclosure(Document */*doc*/,
							short enclType,
							DRect *dBox,		/* bounding box for the object, with no margin */
							PCONTEXT pContext
							)
{
	DDIST	dEnclMargin, dEnclWidthOffset, dEnclThick;
	short	enclMargin, enclThick;
	Rect	boxRect;
	
	dEnclMargin = pt2d(config.enclMargin);
	dEnclWidthOffset = pt2d(config.enclWidthOffset);
	dEnclThick = pt2d(config.enclLW)/4;

	dBox->right += dEnclWidthOffset;

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			D2Rect(dBox, &boxRect);
			
			enclMargin = d2p(dEnclMargin); if (enclMargin<1) enclMargin = 1;
			enclThick = d2p(dEnclThick); if (enclThick<1) enclThick = 1;
		
			InsetRect(&boxRect,-enclMargin,-enclMargin);
			PenSize(enclThick, enclThick);
			OffsetRect(&boxRect,pContext->paper.left, pContext->paper.top);
			if (enclType==ENCL_BOX) FrameRect(&boxRect);
#ifdef NOTYET
			else if (enclType==ENCL_CIRCLE) FrameOval(&boxRect);
#endif
			PenNormal();
			break;
		case toPostScript:
			if (enclType==ENCL_BOX) {
				InsetDRect(dBox, -dEnclMargin, -dEnclMargin);
				PS_FrameRect(dBox, dEnclThick);
			}
			break;
	}
}


/* -------------------------------------------------------------------- GetGraphicDBox -- */
/* Return the DDIST bounding box for the given Graphic, with origin at (0,0). If the
Graphic is a text type and _expandN_, it's stretched out. As function value, return
True normally, False if we can't compute the bounding box. */

static Boolean GetGraphicDBox(Document *doc,
					LINK pL,
					Boolean expandN,
					PCONTEXT pContext,			/* NILINK iff Graphic is attached to Page */
					short fontID, short fontSize, short fontStyle,
					DRect *pDBox				/* Bounding box (TOP_LEFT at 0,0 and no margin) */
					)
{
 	PGRAPHIC p;  DDIST dHeight;
	Str255 string;
	StringPtr pStr;
	LINK aGraphicL;  PAGRAPHIC aGraphic;
	StringOffset theStrOffset;
	Rect bBox;
	
	p = GetPGRAPHIC(pL);
	switch (GraphicSubType(pL)) {
		case GRPICT:
			/* There used to be a lot of code here, but GRPICTs have never worked, and
			   Apple's PICT format is now obsolete, so don't even try to compute a
			   bounding box. --DAB, March 2020 */

			return False;
		case GRArpeggio:
			if (pContext==NILINK) {
				LogPrintf(LOG_WARNING, "Can't handle arpeggio sign attached to Page.  (GetGraphicDBox)\n");
				return False;
			}
			dHeight = qd2d(p->info, pContext->staffHeight, pContext->staffLines);
			SetDRect(pDBox, 0, 0, pt2d(3), dHeight);		/* Width is crude but seems acceptable */
			return True;
		case GRChar:
			string[0] = 1;
			string[1] = p->info;
			pStr = string;
			break;
		case GRSusPedalDown:
			string[0] = 1;
			string[1] = 0xA1;								/* Mac OS Roman keys: shift-option 8 */
			pStr = string;
			fontID = doc->musicFontNum;
			break;
		case GRSusPedalUp:
			string[0] = 2;
			string[1] = '*';								/* Shift 8 */
			string[2] = '*';
			pStr = string;
			fontID = doc->musicFontNum;
			break;
		case GRChordSym:
			return False;									/* Handled by DrawChordSym */
		default:
			aGraphicL = FirstSubLINK(pL);
			aGraphic = GetPAGRAPHIC(aGraphicL);
			theStrOffset = aGraphic->strOffset;
			
			if (expandN) {
				if (!ExpandPString(string, (StringPtr)PCopy(theStrOffset), EXPAND_WIDER)) {
					LogPrintf(LOG_WARNING, "ExpandPString failed.  (GetGraphicDBox)\n");
					return False;
				}
				
			}
			else Pstrcpy(string, (StringPtr)PCopy(theStrOffset));
			pStr = string;
	}
	
	/* If we get here, the Graphic is some sort of text. */
	p = GetPGRAPHIC(pL);
	GetNPtStringBBox(doc, pStr, fontID, fontSize, fontStyle, p->multiLine, &bBox);
//char cStr[256]; Pstrcpy((unsigned char *)cStr, pStr); PToCString((unsigned char *)cStr);
//LogPrintf(LOG_DEBUG, "GetGraphicDBox: count lines=%d fontID, Size, Style=%d, %d, %d str='%s'\n",
//CountTextLines(string), fontID, fontSize, fontStyle, cStr);

	/* Pure fudgery: for mysterious reasons, if the style is not plain, what we have
	   now is too short by an amount that seems to increase with the length of the
	   string, so add a little. 1 point per char. is slightly too much, but the code
	   below doesn't seem to be enough. */
	   
	if (fontStyle!=0) bBox.right += 2*(*pStr)/3;

	PtRect2D(&bBox, pDBox);	
//LogPrintf(LOG_DEBUG, "GetGraphicDBox: pDBox=%d,%d,%d,%d\n", pDBox->top, pDBox->left,
//		pDBox->bottom, pDBox->right);
	return True;
}

/* ------------------------------------------------------------------------ DrawGRPICT -- */
/* Draw the GRPICT Graphic with the given resource ID or handle. NB: PostScript
drawing was never implemented, but GRPICTs have never worked anyway, so this function
should probably be removed one of these days. --DAB, March 2020 */

static void DrawGRPICT(Document */*doc*/,
				DDIST xd, DDIST yd,
				short picID,			/* PICT resource ID */
				Handle picH,			/* handle to PICT resource, or NULL */
				PCONTEXT pContext,
				Boolean /*dim*/			/* ignored */
				)
{
	Rect r;  short width, height;
	short xp, yp, yTop;  DDIST lnSpace;
	
	lnSpace = LNSPACE(pContext);
	yTop = pContext->paper.top;

	if (picH==NULL) picH = (Handle)GetPicture(picID);
	if (picH==NULL) return;

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = pContext->paper.left + d2p(xd);
			yp = yTop + d2p(yd);
			r = (*(PicHandle)picH)->picFrame;
			width = r.right - r.left;						/* calculate width and height */
			height = r.bottom - r.top;
			SetRect(&r, 0, 0, width, height);
			OffsetRect(&r, xp, yp);							/* move into position */
			DrawPicture((PicHandle)picH,  &r);
			break;
		case toPostScript:
			LogPrintf(LOG_WARNING, "PostScript drawing is not implemented.  (DrawGRPICT)\n");
			break;
	}
}


/* ----------------------------------------------------------------------- DrawArpSign -- */
/* Draw an arpeggio or non-arpeggio sign. */

static void DrawArpSign(Document *doc, DDIST xd, DDIST yd, DDIST dHeight,
					short subType, PCONTEXT pContext, Boolean /*dim*/)	/* <dim> is ignored */
{
	short xp, yp, yTop;  DDIST lnSpace, nonarpThick;  Byte glyph;
	
	lnSpace = LNSPACE(pContext);
	yTop = pContext->paper.top;

	if (subType==NONARP) {
		switch (outputTo) {
			case toScreen:
			case toBitmapPrint:
			case toPICT:
				xp = pContext->paper.left + d2p(xd);
				yp = yTop + d2p(yd);
				DrawNonArp(xp, yp, dHeight, lnSpace);
				break;
			case toPostScript:
				nonarpThick = (long)(config.nonarpLW*lnSpace) / 100L;
				PS_Line(xd, yd, xd+NONARP_CUTOFF_LEN(lnSpace), yd, nonarpThick);
				PS_Line(xd, yd, xd, yd+dHeight, nonarpThick);
				PS_Line(xd, yd+dHeight, xd+NONARP_CUTOFF_LEN(lnSpace), yd+dHeight,
								nonarpThick);
				break;
		}
		return;
	}

	/* subType must be ARP. */
	glyph = MapMusChar(doc->musFontInfoIndex, MCH_arpeggioSign);
	xd += MusCharXOffset(doc->musFontInfoIndex, glyph, lnSpace);
	yd += MusCharYOffset(doc->musFontInfoIndex, glyph, lnSpace);
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = pContext->paper.left + d2p(xd);
			DrawArp(doc, xp, yTop, yd, dHeight, glyph, pContext);
			break;
		case toPostScript:
			PS_ArpSign(doc, xd, yd, dHeight, 100, glyph, lnSpace);
			break;
	}
}


/* ------------------------------------------------------------------------ DrawGRDraw -- */
/* Draw a MiniDraw-subtype Graphic. Returns half its line thickness.  */

static DDIST DrawGRDraw(Document */*doc*/,
				DDIST xd, DDIST yd,		/* Center position of endpt */
				DDIST xd2, DDIST yd2,	/* Center position of endpt */
				short lineLW,			/* in percent of linespace */
				PCONTEXT pContext,
				Boolean dim,
				Boolean doDraw,			/* True=really draw, False=just return value */
				Boolean *pVertical		/* Output: is line vertical or nearly vertical? */
				)
{
	short xp, yp, yTop, xp2, yp2; DDIST lnSpace, dThick;
	Boolean vertical;
	
	lnSpace = LNSPACE(pContext);
	/*
	 * If the line is much closer to vertical than horizontal, we thicken horizontally
	 * only; otherwise we thicken vertically only. This won't give the desired
	 * thickness unless the line is nearly vertical or horizontal, or thickness is small
	 * enough that it doesn't matter. FIXME: AT THE MOMENT, QD ONLY.
	 */
	dThick = (long)(lineLW*lnSpace) / 100L;
	vertical = (ABS(yd-yd2) > ABS(2*(xd-xd2)));			/* Is line "nearly" vertical? */
	if (!doDraw) goto Done;
	
	if (vertical) {
		xd -= dThick/2;
		xd2 -= dThick/2;
	}
	else {
		yd -= dThick/2;
		yd2 -= dThick/2;
	}

	yTop = pContext->paper.top;

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			if (dim) PenPat(NGetQDGlobalsGray());
			if (vertical)
				PenSize(d2p(dThick)>1? d2p(dThick) : 1, 1);
			else
				PenSize(1, d2p(dThick)>1? d2p(dThick) : 1);
			xp = pContext->paper.left + d2p(xd);
			yp = yTop + d2p(yd);
			xp2 = pContext->paper.left + d2p(xd2);
			yp2 = yTop + d2p(yd2);
			MoveTo(xp, yp);
			LineTo(xp2, yp2);
			PenNormal();
			break;
		case toPostScript:
			PS_Line(xd, yd, xd2, yd2, dThick);
			break;
	}
	
Done:
	*pVertical = vertical;
	return dThick/2;
}


/* --------------------------------------------------------------------- DrawTextBlock -- */

static short CountTextLines(StringPtr pString)
{
	short len, i, nLines;
	Byte *q = pString;
	
	len = Pstrlen(pString);
	nLines = 1;
	for (i = 1; i <= len; i++) {
		if (q[i] == CH_CR) nLines++;
	}
	
	return nLines;
}


/* Draw a "block" of strings: really a single Pascal string, perhaps with embedded
CRs, which we interpret as starting new lines. Intended for use with text Graphics
and the verbal part of Tempos. Return True normally, False if there's a problem. */

Boolean DrawTextBlock(Document *doc, DDIST xd, DDIST yd, LINK pL, PCONTEXT pContext,
						Boolean dim, short fontID, short fontSize, short fontStyle)
{
	CONTEXT			relContext;					/* context of relative object */
	short			oldFont, oldSize, oldStyle,
					xp, yp, voice;
	DDIST			dLineSp;
	Str255			str, strToDraw;
	StringOffset	theStrOffset;
	Boolean			expandN=False;				/* Stretch string out? */
	Boolean			multiLine=False;
	FontInfo		fInfo;
	
	if (ObjLType(pL)==GRAPHICtype) {
		PGRAPHIC p;
		LINK aGraphicL;  PAGRAPHIC aGraphic;
		
		if (GraphicSubType(pL)!=GRLyric && GraphicSubType(pL)!=GRString) {
			SysBeep(10);
			LogPrintf(LOG_WARNING, "Graphic L%u isn't a GRLyric or GRString.  (DrawTextBlock)\n",
					  pL);
			return False;
		}
 		p = GetPGRAPHIC(pL);
		if (p->graphicType==GRString) expandN = (p->info2!=0);
		multiLine = p->multiLine;
		voice = p->voice;
		
		aGraphicL = FirstSubLINK(pL);
		aGraphic = GetPAGRAPHIC(aGraphicL);
		theStrOffset = aGraphic->strOffset;
		GetNFontInfo(fontID, fontSize, fontStyle, &fInfo);
		dLineSp = pt2d(fInfo.ascent + fInfo.descent + fInfo.leading);
		(void)GetGraphicOrTempoDrawInfo(doc, pL, GraphicFIRSTOBJ(pL), GraphicSTAFF(pL),
											&xd, &yd, &relContext);
		Pstrcpy(str, (StringPtr)PCopy(theStrOffset));
	}
	else if (ObjLType(pL)==TEMPOtype) {
		PTEMPO p = GetPTEMPO(pL);
		expandN = p->expanded;
		theStrOffset = p->strOffset;
		Pstrcpy(str, (StringPtr)PCopy(theStrOffset));
		multiLine = (CountTextLines((StringPtr)str)>1);
		voice = -1;
		GetNFontInfo(fontID, fontSize, fontStyle, &fInfo);
		dLineSp = pt2d(fInfo.ascent + fInfo.descent + fInfo.leading);
//		LogPrintf(LOG_DEBUG, "TEMPO lines=%d dim=%d  (DrawTextBlock)\n", CountTextLines(str), dim);
	}
	else {
		SysBeep(10);
		LogPrintf(LOG_WARNING, "object L%u isn't a Graphic or Tempo.  (DrawTextBlock)\n", pL);
		return False;
	}
	
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			
			TextFont(fontID);				
			TextSize(UseTextSize(fontSize, doc->magnify));
			TextFace(fontStyle);
			if (voice>=0) ForeColor(Voice2Color(doc, voice));
			xp = d2p(xd);
			yp = d2p(yd);
			MoveTo(pContext->paper.left+xp, pContext->paper.top+yp);
			
//char strC[256];	Pstrcpy((unsigned char *)strC, str); PToCString((unsigned char *)strC);
//LogPrintf(LOG_DEBUG, "DrawTextBlock: len=%d expandN=%d multiLine=%d dim=%d str='%s'\n",
//Pstrlen(str), expandN, multiLine, dim, strC);
			if (multiLine) {
				short i, j, len, count;
				len = Pstrlen(str);
				Byte *q = str;
				
				/* Divide the Pascal string q[] into a series of substrings by repeatedly
				   stuffing the length we want into the byte preceding the current
				   substring. */
				   
				j = count = 0;
				for (i = 1; i <= len; i++) {
					count++;
					if (q[i] == CH_CR || i == len) {
						q[j] = count;
						if (dim)
							DrawMString(doc, (StringPtr)&q[j], NORMAL_VIS, True);
						else
							DrawString((StringPtr)&q[j]);
						if (i < len) {
							yd += dLineSp;
							yp = d2p(yd);
							MoveTo(pContext->paper.left+xp, pContext->paper.top+yp);
						}
						j = i;
						count = 0;
					}
				}
			}
			else {
				if (expandN) {
					if (!ExpandPString(strToDraw, str, EXPAND_WIDER)) {
						LogPrintf(LOG_WARNING, "ExpandPString failed.  (DrawTextBlock)\n");
						break;
					}
				}
				else Pstrcpy(strToDraw, str);
				
				if (dim) DrawMString(doc, strToDraw, NORMAL_VIS, True);
				else DrawString(strToDraw);
			}
			
			ForeColor(blackColor);
			TextFont(oldFont);
			TextSize(oldSize);
			TextFace(oldStyle);
			return True;
			
		case toPostScript:
			Str255 fontName;
			if (!FontID2Name(doc, fontID, fontName)) {
				SysBeep(10);
				LogPrintf(LOG_WARNING, "Can't find font with ID=%d in font table.  (DrawTextBlock)\n");
				return False;
			}
			if (multiLine) {
				short i, j, len, count;
				len = Pstrlen(str);
				
				/* Divide the Pascal string q[] into a series of substrings by repeatedly
				   stuffing the length we want into the byte preceding the current
				   substring. */
				   
				Byte *q = str;
				j = count = 0;
				for (i = 1; i <= len; i++) {
					count++;
					if (q[i] == CH_CR || i == len) {
						q[j] = count;
						PS_FontString(doc, xd, yd, &q[j], fontName, fontSize, fontStyle);
						if (i < len)
							yd += dLineSp;
						j = i;
						count = 0;
					}
				}
			}
			else {
				if (expandN) {
					if (!ExpandPString(strToDraw, str, EXPAND_WIDER)) {
						SysBeep(10);
						LogPrintf(LOG_WARNING, "ExpandPString failed.  (DrawTextBlock)\n");
						return False;
					}
				}
				else Pstrcpy(strToDraw, (StringPtr)str);
				
				PS_FontString(doc, xd, yd, strToDraw, fontName, fontSize, fontStyle);
			}			
			return True;
	}
	
	return False;										/* Should never reach here */
}


/* ----------------------------------------------------------------------- DrawGRAPHIC -- */
/* Draw a GRAPHIC object, including its enclosure, if it needs one, and/or
recompute its objRect. (Thus far, our UI doesn't support entering strings over
255 chars., so we don't need to support longer strings here.  --DAB, Nov. 2017) */

#define SWAP(a, b)	{ short temp; temp = (a); (a) = (b); (b) = temp; }

void DrawGRAPHIC(Document *doc,
				LINK pL,
				CONTEXT context[],
				Boolean doDraw		/* True=really draw, False=just compute objRect */
				)
{
	PGRAPHIC		p;
	LINK			aGraphicL;
	PAGRAPHIC		aGraphic;
	CONTEXT			relContext;				/* context of relative object */
	PCONTEXT		pContext;				/* ptr to context of this object */
	DRect			dEnclBox;
	Rect			objRect;
	DDIST			xd, yd,					/* point the Graphic is relative to */
					xd2, yd2,				/* point the other end of Graphic is rel. to (GRDraw) */
					dHeight;
	short			oldFont, oldSize, oldStyle,
					fontID, fontSize, fontStyle,
					xp, yp, staffn,
					lineLW;
	unsigned char	oneChar[2];				/* Pascal string of a char */
	StringOffset	theStrOffset;
	Boolean			expandN;				/* Stretch string out? */
	Boolean			dim=False;

	/* Compute Graphic's absolute position from its relative object's position. NOTE:
	   For the time being, with PICTs, we ignore the firstObj field and position them
	   relative to the page. */
	   
PushLock(OBJheap);
PushLock(GRAPHICheap);
 	p = GetPGRAPHIC(pL);
 	
 	staffn = GetGraphicOrTempoDrawInfo(doc, pL, p->firstObj, p->staffn, &xd, &yd, &relContext);
	GetGraphicFontInfo(doc, pL, &relContext, &fontID, &fontSize, &fontStyle);
	
	if (staffn==NOONE) pContext = NILINK;
	else {
		pContext = &context[staffn];
		if (!pContext->staffVisible && !PageTYPE(p->firstObj)) goto Cleanup;
	}

	expandN = False;
	if (p->graphicType==GRString) expandN = (p->info2!=0);
	
	/* DrawChordSym gets the bounding box of chord symbols as well as drawing them, so
	   GetGraphicDBox doesn't handle chord symbols. */
	   
	if (p->graphicType!=GRChordSym &&
			GetGraphicDBox(doc, pL, expandN, pContext, fontID, fontSize, fontStyle, &dEnclBox))
		OffsetDRect(&dEnclBox, xd, yd);

	dim = (outputTo==toScreen && !LOOKING_AT(doc, p->voice));

	/* Handle the odd (mostly non-text) subtypes of Graphics separately. */
	
	switch (p->graphicType) {
		Boolean vertical;  DDIST dHalfThick;
		case GRLyric:
		case GRString:
			if (doDraw) DrawTextBlock(doc, xd, yd, pL, pContext, dim, fontID, fontSize, fontStyle);
			if (outputTo==toScreen) D2ObjRect(&dEnclBox, &objRect);
			goto DrawEnclosure;
		case GRPICT:
			if (doDraw)
				DrawGRPICT(doc, xd, yd, p->info, p->gu.handle, &relContext, dim);
			if (outputTo==toScreen) D2ObjRect(&dEnclBox, &objRect);
			goto DrawEnclosure;
		case GRArpeggio:
			if (doDraw) {
				dHeight = qd2d(p->info, relContext.staffHeight, relContext.staffLines);
				/* FIXME: It looks as if we should set the TextSize before drawing. */
				DrawArpSign(doc, xd, yd, dHeight, ARPINFO(p->info2), &relContext, dim);
			}
			if (outputTo==toScreen) D2ObjRect(&dEnclBox, &objRect);
			goto DrawEnclosure;
		case GRDraw:
			lineLW = p->gu.thickness;
			GetGRDrawLastDrawInfo(doc,pL,p->lastObj,p->staffn,&xd2,&yd2);
			dHalfThick = DrawGRDraw(doc, xd, yd, xd2, yd2, lineLW, pContext, dim, doDraw,
											&vertical);
			SetDRect(&dEnclBox, xd, yd, xd2, yd2);
	
			if (vertical && lineLW>20) {
				/*
				 * The line is thick and roughly vertical: the objRect may be too narrow
				 * unless we widen it. At this point, <dEnclBox> has a width of zero, so we
				 * have to thicken a bit more than you'd expect; however, the value used
				 * below is purely empirical and may not be ideal.
				 */
				DDIST dWidenRect;
				
				dWidenRect = dHalfThick+pt2d(1)/4;
				dEnclBox.left -= dWidenRect;
				dEnclBox.right += dWidenRect;	 
			}
	
			if (dEnclBox.bottom<dEnclBox.top) SWAP(dEnclBox.bottom, dEnclBox.top);
			dEnclBox.top -= dHalfThick;
			dEnclBox.bottom += dHalfThick;
			if (outputTo==toScreen) D2ObjRect(&dEnclBox, &objRect);
			goto DrawEnclosure;	
		default:
			;
	}
 
	/* Handle the remaining Graphic subtypes, all of which are some kind of text. */
	
	expandN = (p->info2!=0);
	aGraphicL = FirstSubLINK(pL);
	aGraphic = GetPAGRAPHIC(aGraphicL);
	theStrOffset = aGraphic->strOffset;

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
					
			TextFont(fontID);				
			TextSize(UseTextSize(fontSize, doc->magnify));
			TextFace(fontStyle);
			ForeColor(Voice2Color(doc, p->voice));
			pContext = &context[staffn];
			xp = d2p(xd);
			yp = d2p(yd);
			MoveTo(pContext->paper.left+xp, pContext->paper.top+yp);

			if (doDraw)
				switch (p->graphicType) {
					case GRChar:
						if (dim) DrawMChar(doc, p->info, NORMAL_VIS, True);
						else	 DrawChar(p->info);
						break;
					case GRChordSym:
						DrawChordSym(doc, xd, yd, PCopy(theStrOffset), p->info, pContext, dim, &dEnclBox);
						OffsetDRect(&dEnclBox, xd, yd);
						break;
					case GRChordFrame:
					case GRRehearsal:
					case GRMIDIPatch:
						if (dim)	DrawMString(doc, (StringPtr)PCopy(theStrOffset),
														NORMAL_VIS, True);
						else		DrawString(PCopy(theStrOffset));
						break;
					case GRMIDIPan:
						TextFace(italic);
						if (dim)	DrawMString(doc, (StringPtr)PCopy(theStrOffset),
														NORMAL_VIS, True);
						else		DrawString(PCopy(theStrOffset));
						break;
					case GRSusPedalDown:
						oneChar[0] = 1;
						oneChar[1] = 0xA1;					/* Mac OS Roman keys: shift-option 8 */
						TextFace(normal);
						TextFont(doc->musicFontNum);
						DrawString(oneChar);
						break;
					case GRSusPedalUp:
						oneChar[0] = 1;
						oneChar[1] = '*';					/* Shift 8 */
						TextFace(normal);
						TextFont(doc->musicFontNum);
						DrawString(oneChar);
						break;
				}
			else if (p->graphicType==GRChordSym) {
				HidePen();
				DrawChordSym(doc, xd, yd, PCopy(theStrOffset), p->info, pContext, dim, &dEnclBox);
				OffsetDRect(&dEnclBox, xd, yd);
				ShowPen();
			}
					
			if (outputTo==toScreen) D2ObjRect(&dEnclBox, &objRect);
			
			ForeColor(blackColor);
			TextFont(oldFont);
			TextSize(oldSize);
			TextFace(oldStyle);
			break;
		case toPostScript:
			switch (p->graphicType) {
				case GRChar:
					oneChar[0] = 1; oneChar[1] = p->info;
					PS_FontString(doc, xd, yd, oneChar, doc->fontTable[p->fontInd].fontName,
										fontSize, fontStyle);
					break;
				case GRChordSym:
					DrawChordSym(doc, xd, yd, PCopy(theStrOffset), p->info, pContext, dim, &dEnclBox);
					objRect = LinkOBJRECT(pL);				/* FIXME: Why do this if <toPostScript>? */
					break;
				case GRChordFrame:
					PS_FontString(doc, xd, yd, PCopy(theStrOffset),
										"\pSeville",					// FIXME: TEMPORARY
										fontSize, fontStyle);
					break;
				case GRRehearsal:
				case GRMIDIPatch:
					PS_FontString(doc, xd, yd, PCopy(theStrOffset),
										doc->fontTable[p->fontInd].fontName,
										fontSize, fontStyle);
					break;
				case GRMIDIPan:
					PS_FontString(doc, xd, yd, PCopy(theStrOffset),
										doc->fontTable[p->fontInd].fontName,
										fontSize, fontStyle);
					break;
				/* For pedal down and up, always use the document's music font. */
				case GRSusPedalDown:
					oneChar[0] = 1;
					oneChar[1] = 0xA1;								/* Mac OS Roman keys: shift-option 8 */
					PS_FontString(doc, xd, yd,oneChar,
										doc->musFontName,
										fontSize, fontStyle);
					break;
				case GRSusPedalUp:
					oneChar[0] = 1;
					oneChar[1] = '*';								/* Shift 8 */
					PS_FontString(doc, xd, yd,oneChar,
										doc->musFontName,
										fontSize, fontStyle);
					break;
			}
			break;
	}
	
DrawEnclosure:
	if (doDraw && p->enclosure!=ENCL_NONE)
		DrawEnclosure(doc, p->enclosure, &dEnclBox, pContext);

Cleanup:
	/* Strings in Sonata have been seen with zero-height objRects. That's not good, so
	   avoid the possibility. */

	if (outputTo==toScreen && objRect.bottom==objRect.top) objRect.bottom++;
	LinkOBJRECT(pL) = objRect;

PopLock(OBJheap);
PopLock(GRAPHICheap);
}


/* ---------------------------------------------------------------------- GetTempoDBox -- */
/* Return the DDIST bounding box for the given Tempo object, with origin at (0,0).
If _expandN_, it's stretched out. */

static Boolean GetTempoDBox(Document *doc,
							LINK pL,
							Boolean expandN,
							short fontID, short fontSize, short fontStyle,
							DRect *dBox			/* Bounding box (TOP_LEFT at 0,0 and no margin) */
							)
{
 	PTEMPO p;
	Str255 string, tempStr;
	StringOffset theStrOffset;
	Boolean multiLine;
	Rect bBox;
	
	p = GetPTEMPO(pL);
	theStrOffset = p->strOffset;
	Pstrcpy(tempStr, (StringPtr)PCopy(theStrOffset));
	multiLine = (CountTextLines(tempStr)>1);
	
	if (expandN) {
		if (!ExpandPString(string, tempStr, EXPAND_WIDER)) {
			LogPrintf(LOG_WARNING, "GetTempoDBox: ExpandPString failed.\n");
			return False;
		}
	}
	else Pstrcpy(string, (StringPtr)PCopy(theStrOffset));
	
	GetNPtStringBBox(doc, string, fontID, fontSize, fontStyle, multiLine, &bBox);
//char cStr[256]; Pstrcpy((unsigned char *)cStr, string); PToCString((unsigned char *)cStr);
//LogPrintf(LOG_DEBUG, "GetTempoDBox: count lines=%d fontID, Size, Style=%d, %d, %d str='%s'\n",
//			CountTextLines(string), fontID, fontSize, fontStyle, cStr);
	
	/* 
	 *	Pure fudgery: for mysterious reasons, if the style is not plain, what we have
	 *	now is too short by an amount that seems to increase with the length of the
	 *	string, so add a little. 1 point per char. is slightly too much, but the code
	 * below doesn't seem to be enough.
	 */
	if (fontStyle!=0) bBox.right += 2*(Pstrlen(string))/3;
	
	PtRect2D(&bBox, dBox);
	return True;
}


/* ------------------------------------------------------------------------- DrawTEMPO -- */
/* Draw a TEMPO object: verbal tempo string and/or metronome mark (consisting of a
duration-unit note, perhaps dotted, and an M.M. number string). */

void DrawTEMPO(Document *doc,
				LINK pL,
				CONTEXT context[],
				Boolean doDraw					/* True=really draw, False=just set objRect */
				)
{
	PTEMPO p;
	PCONTEXT pContext;  CONTEXT relContext;
	DRect dEnclBox;  Rect objRect, lineBBox;
	short oldFont, oldSize, oldStyle, useTxSize, fontSize, fontInd, fontID, fontStyle;
	short staffn, noteWidth, tempoStrlen;
	StringOffset theStrOffset;
	Str255 tempoStr;
	char metroStr[256], noteChar;
	DDIST lnSpace, extraHorizGap, dEnclBoxHeight, dTextLineHeight;
	DDIST xd, yd, xdMM, xdDot, xdMMNumber, ydMM, ydDot;
	LINK firstObjL;
	Boolean expandN, doDrawMM, metroIsBelow;
	Byte dotChar = MapMusChar(doc->musFontInfoIndex, MCH_dot);

PushLock(OBJheap);
PushLock(TEMPOheap);
 	p = GetPTEMPO(pL);

	firstObjL = p->firstObjL;
	if (!firstObjL) {
		MayErrMsg("DrawTEMPO: tempo obj at L%ld has NILINK firstObjL", (long)pL);
		goto Cleanup;
	}

	staffn = GetGraphicOrTempoDrawInfo(doc, pL, firstObjL, p->staffn, &xd, &yd, &relContext);
	pContext = &context[staffn];
	if (!pContext->staffVisible) goto Cleanup;
	lnSpace = LNSPACE(pContext);

	fontSize = GetTextSize(doc->relFSizeTM, doc->fontSizeTM, lnSpace);
	fontInd = FontName2Index(doc, doc->fontNameTM);
	fontID = doc->fontTable[fontInd].fontID;
	fontStyle = doc->fontStyleTM;

	/* Prepare tempo string. */
	theStrOffset = p->strOffset;
	expandN = p->expanded;
	if (expandN) {
		if (!ExpandPString(tempoStr, (StringPtr)PCopy(theStrOffset), EXPAND_WIDER)) {
			LogPrintf(LOG_WARNING, "DrawTEMPO: ExpandPString failed.\n");
			goto Cleanup;
		}
	}
	else Pstrcpy(tempoStr, (StringPtr)PCopy(theStrOffset));
	tempoStrlen = Pstrlen(tempoStr);
	
	/* If the tempo string ends with a new line, the metronome mark goes below the
	   bottom line of the tempo mark; else it goes to the right of the top line. (Of
	   course _dEnclBox_, the object's bounding box, should reflect this.) */
		
	metroIsBelow = (tempoStr[tempoStrlen]==CH_CR);
	if (GetTempoDBox(doc, pL, expandN, fontID, fontSize, fontStyle, &dEnclBox)) {
		dEnclBoxHeight = dEnclBox.bottom-dEnclBox.top;
		OffsetDRect(&dEnclBox, xd, yd);
	}
	else goto Cleanup;
		
	/* Prepare M.M. duration unit note and M.M. string and find their positions. */
	
	noteChar = TempoGlyph(pL);
	noteChar = MapMusChar(doc->musFontInfoIndex, noteChar);
	sprintf(metroStr," = %s", PToCString(PCopy(p->metroStrOffset)));
	
	if (metroIsBelow) {
		xdMM = xd;
		GetNPtStringBBox(doc, tempoStr, fontID, fontSize, fontStyle, False, &lineBBox);
		dTextLineHeight = pt2d(lineBBox.bottom-lineBBox.top);
		ydMM = yd+dEnclBoxHeight-dTextLineHeight;
	}
	else {
		extraHorizGap = 0;
		xdMM = xd;
		if (tempoStrlen>0) {
			extraHorizGap = qd2d(config.tempoMarkHGap, pContext->staffHeight, pContext->staffLines);
			xdMM = dEnclBox.right+lnSpace+extraHorizGap;
		}
		ydMM = yd;
	}
	
	/* We'll cheat and get the width of the note and dot in the current font/size/style
	   instead of the ones it'll be drawn in. FIXME: This shouldn't make much difference,
	   but it would be better to to do smthg like what MaxPartNameWidth does, or to share
	   code with SymWidthRight (q.v.) -- or just call GetNPtStringBBox(). And using
	   pixel-based calculations is really bad, esp. for PostScript! */
	
	noteWidth = CharWidth(noteChar);
	if (p->dotted) {
		noteWidth += pt2p(2);
		noteWidth += CharWidth(dotChar);
		xdDot = xdMM+p2d(noteWidth);
		xdDot += MusCharXOffset(doc->musFontInfoIndex, dotChar, lnSpace);
		ydDot = ydMM + MusCharYOffset(doc->musFontInfoIndex, dotChar, lnSpace);
	}
	else if (NFLAGS(p->subType)>0) noteWidth += d2p(lnSpace);	/* maybe a bit too small */
	
	//xdNote = xdMM+MusCharXOffset(doc->musFontInfoIndex, noteChar, lnSpace)+p2d(noteWidth);
	//ydNote = ydMM+MusCharYOffset(doc->musFontInfoIndex, noteChar, lnSpace);
	
	xdMMNumber = xdMM+p2d(noteWidth);
	
	/* Decide whether to actually draw the metronome mark. */
	if (p->noMM)	doDrawMM = False;
	else			doDrawMM = ((!p->hideMM || doc->showInvis) && doDraw);

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			/* Perhaps draw the verbal tempo string. If the metronome mark is visible
			   and to its right, update the object's bounding box; otherwise, the bounding
			   box is already correct. */
			   		
			if (doDraw) DrawTextBlock(doc, xd, yd, pL, pContext, False, fontID, fontSize,
										fontStyle);
			if (outputTo==toScreen) {
				D2ObjRect(&dEnclBox, &objRect);
				LinkOBJRECT(pL) = objRect;
				if (doDrawMM && !metroIsBelow)
					LinkOBJRECT(pL).right += noteWidth+d2p(extraHorizGap)+CStringWidth(metroStr);
			}	
		
			/* Perhaps draw the duration unit (note and possibly dot) for the metronome mark. */
			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			if (doDrawMM && p->subType!=NO_L_DUR) {
				MoveTo(pContext->paper.left+d2p(xdMM), pContext->paper.top+d2p(ydMM));
				useTxSize = UseTextSize(pContext->fontSize, doc->magnify);
				useTxSize = METROSIZE(useTxSize);
				TextSize(useTxSize);
				TextFont(doc->musicFontNum);
				TextFace(0);											/* Plain */
				DrawChar(noteChar);
				if (p->dotted) {
					MoveTo(pContext->paper.left+d2p(xdDot), pContext->paper.top+d2p(ydDot));
					DrawChar(dotChar);
				}
			}
			
			/* Perhaps draw the metronome mark number; maybe widen the bounding box */
			
			SetFontFromTEXTSTYLE(doc, (TEXTSTYLE *)doc->fontNameTM, lnSpace);
			if (doDrawMM) {
				short tempoSize, mmSize;
				MoveTo(pContext->paper.left+d2p(xdMMNumber), pContext->paper.top+d2p(ydMM));
				tempoSize = GetPortTxSize();
				mmSize = METROSIZE(tempoSize);
				TextSize(mmSize);
				DrawCString(metroStr);
			}
			
			TextFont(oldFont);
			TextSize(oldSize);
			TextFace(oldStyle);
			break;
		case toPostScript:
			/* Draw the verbal tempo string and perhaps metronome mark */
			fontSize = GetTextSize(doc->relFSizeTM, doc->fontSizeTM, lnSpace);
			DrawTextBlock(doc, xd, yd, pL, pContext, False, fontID, fontSize, fontStyle);
			if (doDrawMM) {
				PS_MusChar(doc, xdMM, ydMM, noteChar, True, METROSIZE(100));
				if (p->dotted) {
					/* FIXME: Why is ydDot defined differently here from bitmap drawing? */
					ydDot = yd + MusCharYOffset(doc->musFontInfoIndex, dotChar, lnSpace);
					PS_MusChar(doc, xdDot, ydDot, dotChar, True, METROSIZE(100));
				}
				PS_FontString(doc, xdMMNumber, ydMM, CToPString(metroStr),
									doc->fontNameTM, fontSize, doc->fontStyleTM);
			}
			break;
	}

Cleanup:

PopLock(OBJheap);
PopLock(TEMPOheap);
}


/* ------------------------------------------------------------------------ DrawSPACER -- */
/* Draw a SPACER object */

void DrawSPACER(Document *doc, LINK pL, CONTEXT context[])
{
	DDIST dLeft, dTop, xd, yd;  short xp, yp, staffHeight, spaceWidth;  PSPACER p;
	PCONTEXT pContext;  Rect r;

 	p = GetPSPACER(pL);
	pContext = &context[p->staffn];
	if (!pContext->staffVisible) return;

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			dLeft = pContext->measureLeft;
			dTop = pContext->measureTop;
			xd = dLeft + LinkXD(pL);
			yd = dTop;
			xp = d2p(xd);
			yp = d2p(yd);
			staffHeight = d2p(pContext->staffHeight);
			spaceWidth = d2p(std2d(p->spWidth,pContext->staffHeight,pContext->staffLines));
			SetRect(&LinkOBJRECT(pL), xp, yp, xp+spaceWidth, yp+staffHeight);
			if (doc->showInvis) {
				r = LinkOBJRECT(pL);
				Rect2Window(doc, &r);
				FrameRect(&r);
			}
			break;
		case toPostScript:
			break;
	}
}


/* ------------------------------------------------------------------ DrawMeasNum -- */
/* Draw measure number, including its enclosure, if it needs one. */

static void DrawMeasNum(Document *doc, DDIST xdMN, DDIST ydMN, short measureNum,
								PCONTEXT pContext)
{
	short		xp, yp,
				oldFont, oldSize, oldStyle,	/* Text characteristics */
				fontSize, fontInd, fontID;
	DDIST		lineSpace;
	Str15		nStr;
	Rect		measNumBox;
	DRect		dEnclBox;
	
	NumToString(measureNum, nStr);

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = pContext->paper.left+d2p(xdMN);
			yp = pContext->paper.top+d2p(ydMN);
			MoveTo(xp, yp);

			oldFont = GetPortTxFont();
			oldSize = GetPortTxSize();
			oldStyle = GetPortTxFace();
			
			lineSpace = LNSPACE(pContext);
			SetFontFromTEXTSTYLE(doc, (TEXTSTYLE *)doc->fontNameMN, lineSpace);
			DrawString(nStr);
			
			TextFont(oldFont);
			TextSize(oldSize);
			TextFace(oldStyle);
			break;
		case toPostScript:
			fontSize = GetTextSize(doc->relFSizeMN, doc->fontSizeMN, LNSPACE(pContext));
			PS_FontString(doc, xdMN, ydMN, nStr, doc->fontNameMN, fontSize, doc->fontStyleMN);
			break;
	}

	if (doc->enclosureMN!=ENCL_NONE) {
		fontInd = FontName2Index(doc, doc->fontNameMN);					/* Should never fail */
		fontID = doc->fontTable[fontInd].fontID;
		fontSize = GetTextSize(doc->relFSizeMN, doc->fontSizeMN, LNSPACE(pContext));
		GetNPtStringBBox(doc, nStr, fontID, fontSize, doc->fontStyleMN, False, &measNumBox);
		measNumBox.right += fontSize/4;									/* Pure fudgery */
		PtRect2D(&measNumBox, &dEnclBox);
		OffsetDRect(&dEnclBox, xdMN, ydMN);
		DrawEnclosure(doc, doc->enclosureMN, &dEnclBox, pContext);
	}
}


/* --------------------------------------------------------------- Shade Measure areas -- */

static void ShadeMeasureInRect(Rect *pMRect, PCONTEXT pContext, Pattern *shadePat)
{
	OffsetRect(pMRect, pContext->paper.left, pContext->paper.top);
	
	/* Indent shading area horizontally to avoid covering up the barlines, and
	   vertically to separate this measure from those in adjacent systems. The standard
	   ltGray pattern covers up some things drawn after it, e.g., it often makes the
	   blinking caret almost invisible, so we use our own diagonal stripes. */
	   
	InsetRect(pMRect, 1, 2);
	PenPat(shadePat);
	PenMode(patOr);
	PaintRect(pMRect);
	PenNormal();
}


/* Check whether this measure's notated (time signature-based) and actual durations
across all staves agree. If not, shade over the entire measure across all staves; if
they do, perform the same check on individual staves, and shade the measure on just
those staves that don't agree. The latter condition effectively means that staves for
which the measure's notes/rests don't extend to the "real" end of the measure get
shaded, but only when at least one other staff has a complete measure. */

static void ShadeDurPblmMeasure(Document *doc, LINK measureL, PCONTEXT pContext)
{
	LINK barTermL, sysL, aMeasL, aNoteL, aSyncL; 
	Boolean okay, isMBRest;
	long measDurFromTS, measDurActual, measDurOnStaff;
	Rect r;  PMEASURE p;
	short staffn, xd;  DRect mrect;

	if (IsFakeMeasure(doc, measureL)) return;
	
	barTermL = EndMeasSearch(doc, measureL);
	if (barTermL) {
		okay = True;
		measDurFromTS = GetTimeSigMeasDur(doc, barTermL);
		if (measDurFromTS<0) okay = False;
		else {
			measDurActual = GetMeasDur(doc, barTermL, ANYONE);
			/* If the first Sync following the Measure object is a multibar rest, assume
			   the "measure" is OK. (NB that if the "measure" we're checking contains no
			   Syncs, the first Sync following won't be in it, but that shouldn't cause
			   any problems.) */
			   
			aSyncL = SSearch(RightLINK(measureL), SYNCtype, GO_RIGHT);
			if (aSyncL!=NILINK) {
				aNoteL = FirstSubLINK(aSyncL);
				isMBRest = (NoteType(aNoteL)<-1);
//LogPrintf(LOG_DEBUG, "ShadeDurPblmMeasure: for SyncL=%u, aNoteL=%u, NoteType=%d\n",
//RightLINK(measureL), aNoteL, NoteType(aNoteL));
				if (isMBRest) return;
			}
			if (measDurActual!=0 && ABS(measDurFromTS-measDurActual)>=PDURUNIT)
				okay = False;
		}

		if (okay) {
			/* Overall measure duration is okay. Look for individual staves with
			   incomplete contents. */
				
			p = GetPMEASURE(measureL);
			xd = p->xd;
			sysL = LSSearch(measureL, SYSTEMtype, ANYONE, GO_LEFT, False);
			aMeasL = FirstSubLINK(measureL);
			for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
				staffn = MeasureSTAFF(aMeasL);			
				measDurOnStaff = GetMeasDur(doc, barTermL, staffn);
				if (measDurOnStaff!=0 && ABS(measDurFromTS-measDurOnStaff)>=PDURUNIT) {
					mrect = MeasMRECT(aMeasL);
					OffsetDRect(&mrect, xd, 0);
					InsetDRect(&mrect, 0, pt2d(10));		/* Reduce ht of shading to emphasize staff */
					DRect2ScreenRect(mrect, SystemRECT(sysL), doc->paperRect, &r);
					ShadeMeasureInRect(&r, pContext, &otherLtGray);
				}
			}
		}
		else {
			p = GetPMEASURE(measureL);
			r = p->measureBBox;
			ShadeMeasureInRect(&r, pContext, &diagonalLtGray);
		}
	}
}


/* ----------------------------------------------------------------------- DrawBarline -- */
/* Draw a barline (not a repeat bar) across one or more staves. */

void DrawBarline(Document *doc,
					LINK pL,			/* Measure or PSEUDOMEAS object */
					short staff,
					short connStaff,
					CONTEXT context[],
					SignedByte subType
					)
{
	PCONTEXT	pContext,				/* ptr to relevant context[] entries */
				pContext2;
	short		xp, ypTop, ypBot, betweenBars, thickBarWidth;
	DDIST		dLeft, dTop, dBottom,	/* top of above staff and bottom of below staff */
				lnSpace, dashLen,
				barThick;
	
	pContext = &context[staff];
	lnSpace = LNSPACE(pContext);

	if (MeasureTYPE(pL)) {
		dTop = pContext->measureTop = pContext->staffTop;
		dLeft = pContext->measureLeft = pContext->staffLeft + LinkXD(pL);
	}
	else {
		dTop = pContext->staffTop;
		dLeft = pContext->measureLeft + LinkXD(pL);
	}

	if (connStaff==0) {
		dBottom = dTop + pContext->staffHeight;						/* Not connected below */
		if (pContext->showLines==1) dBottom -= lnSpace;			
	}
	else {
		pContext2 = &context[NextVisStaffn(doc,pL,False,connStaff)]; /* Connected below */
		dBottom = pContext2->staffTop + pContext2->staffHeight;
		if (pContext2->showLines==1) dBottom -= LNSPACE(pContext2);			
	}
	if (pContext->showLines==1) dTop += lnSpace;

	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xp = pContext->paper.left + d2p(dLeft);
			ypTop = pContext->paper.top + d2p(dTop);
			ypBot = pContext->paper.top + d2p(dBottom);

#ifdef NOTYET
			/* The "-1"s below are purely empirical and to be conservative. FIXME: Changing
			   this also requires changing code in CheckMEASURE for hit-testing and
			   hiliting. We should also make the analogous changes to repeat bars. */
			   
			betweenBars = d2p(lnSpace/2)-1;
			if (betweenBars<2) betweenBars = 2;

			thinBarWidth = betweenBars/2-1;
			if (thinBarWidth<1) thinBarWidth = 1;

			thickBarWidth = betweenBars-1;
			if (thickBarWidth<2) thickBarWidth = 2;
#else
			betweenBars = 2;
#endif

			switch (subType) {
				case BAR_SINGLE:
					MoveTo(xp, ypTop);
					LineTo(xp, ypBot);
					break;
				case BAR_DOUBLE:
					MoveTo(xp, ypTop);
					LineTo(xp, ypBot);
					MoveTo(xp+betweenBars, ypTop);
					LineTo(xp+betweenBars, ypBot);
					break;
				case BAR_FINALDBL:
					MoveTo(xp, ypTop);
					LineTo(xp, ypBot);
					thickBarWidth = betweenBars;
					PenSize(thickBarWidth, 1);
					MoveTo(xp+betweenBars, ypTop);
					LineTo(xp+betweenBars, ypBot);
					PenNormal();
					break;
				case PSM_DOTTED:
				case PSM_DOUBLE:
				case PSM_FINALDBL:
					DrawPSMSubType(subType, xp, ypTop, ypBot);
					break;
				default:
					;
			}
			break;
		case toPostScript:
			switch (subType) {
				case BAR_SINGLE:
				case BAR_DOUBLE:
				case BAR_FINALDBL:
					PS_BarLine(dTop, dBottom, dLeft, subType);
					break;
				case PSM_DOTTED:
					dashLen = std2d(config.barlineDashLen, pContext->staffHeight,
										pContext->staffLines);
					barThick = (long)(config.dottedBarlineLW*lnSpace) / 100L;
					PS_VDashedLine(dLeft, dTop, dBottom, barThick, dashLen);
					break;
				case PSM_DOUBLE:
					PS_BarLine(dTop, dBottom, dLeft, BAR_DOUBLE);
					break;
				case PSM_FINALDBL:
					PS_BarLine(dTop, dBottom, dLeft, BAR_FINALDBL);
					break;
				default:
					;
			}
			break;
	}
}


/* ----------------------------------------------------------------------- DrawMEASURE -- */
/* Draw the graphical manifestation of a MEASURE object, i.e., all its barlines or
repeat bars. If !LinkVALID(pL) and we're drawing to the screen, also update the MEASURE's
<measureBBox> based on the <measSizeRect>s of the subobjects. */

void DrawMEASURE(Document *doc, LINK pL, CONTEXT context[])
{
	PMEASURE	p;
	LINK		aMeasureL, prevL;
	PAMEASURE	aMeasure;			/* ptr to current subitem */
	PCONTEXT	pContext;			/* ptr to current context[] entry */
	PCONTEXT	lastContext;
	DRect		aDRect;				/* scratch */
	Rect		measureRect,		/* Rect of current measure */
				barlineRect;		/* Rect of current barline */
	short		staff, connStaff,
				k, measureNum, prevRight;
	DDIST		dTop,				/* top of above staff */
				dLeft,				/* left edge of staff */
				xdMN, ydMN, yOffset;
	Boolean		drawn,				/* False until a subobject has been drawn */
				recalc,				/* True if we need to recalc object's enclosing rectangle */
				drawBar;			/* True=draw barline proper as well as any repeat dots */
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};

//LogPrintf(LOG_DEBUG, "DrawMEASURE(%u)\n", pL);
PushLock(OBJheap);
PushLock(MEASUREheap);
	p = GetPMEASURE(pL);
	aMeasureL = FirstSubLINK(pL);
	drawn = False;
	recalc = (!LinkVALID(pL) && outputTo==toScreen);
	for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
		drawBar = ShouldDrawBarline(doc, pL, aMeasureL, &connStaff);
		aMeasure = GetPAMEASURE(aMeasureL);

		staff = MeasureSTAFF(aMeasureL);
		pContext = &context[staff];
		lastContext = pContext;
		pContext->measureVisible =	pContext->visible = 		/* update context */
			(aMeasure->measureVisible && pContext->staffVisible);

		dTop = pContext->measureTop = pContext->staffTop;
		if (pContext->showLines==1) dTop += LNSPACE(pContext);
		dLeft = pContext->measureLeft = pContext->staffLeft + LinkXD(pL);
		pContext->inMeasure = True;

		pContext->clefType = aMeasure->clefType;
		pContext->nKSItems = aMeasure->nKSItems;				/* Copy this key sig. */
		for (k = 0; k<aMeasure->nKSItems; k++)					/*    into the context */
			pContext->KSItem[k] = aMeasure->KSItem[k];
		pContext->timeSigType = aMeasure->timeSigType;
		pContext->numerator = aMeasure->numerator;
		pContext->denominator = aMeasure->denominator;
		pContext->dynamicType = aMeasure->dynamicType;

		/* FIXME: This code should be re-written to be independent of rastral/magnification.
			When this is done, refer to note #1 in MakeSystem() in Score.c */

		if (recalc) {
			aDRect = aMeasure->measSizeRect;
			OffsetDRect(&aDRect, pContext->measureLeft, pContext->systemTop);
			D2Rect(&aDRect, &measureRect);

/* FIXME: SHOULD WE DO objRect STUFF IF STAFF ISN'T VISIBLE? */
			SetRect(&barlineRect, measureRect.left-2, measureRect.top, measureRect.left,
						measureRect.bottom);
			switch (aMeasure->subType) {
				case BAR_SINGLE:
					barlineRect.right += 2;
					break;
				case BAR_DOUBLE:
					barlineRect.right += 4;
					break;
				case BAR_FINALDBL:
					barlineRect.right += 5;
					break;
				case BAR_RPT_L:
				case BAR_RPT_R:
				case BAR_RPT_LR:
					/* FIXME: TEMPORARY--NEED GetMeasureDrawInfo THAT SHARES CODE WITH GetRptEndDrawInfo */
					barlineRect.right += 4;
					break;
			}

			if (!drawn) {
				LinkOBJRECT(pL) = barlineRect;
				p->measureBBox = measureRect;
				drawn = True;
			}
			else {
				UnionRect(&LinkOBJRECT(pL), &barlineRect, &LinkOBJRECT(pL));
				UnionRect(&p->measureBBox, &measureRect, &p->measureBBox);
			}
			LinkVALID(pL) = True;
		}
		
		if (pContext->visible || doc->showInvis) {
			if (aMeasure->visible || doc->showInvis) {
				switch (aMeasure->subType) {
					case BAR_SINGLE:
					case BAR_DOUBLE:
					case BAR_FINALDBL:
						if (drawBar) DrawBarline(doc, pL, staff, connStaff, context,
											aMeasure->subType);
						break;
					case BAR_RPT_L:
					case BAR_RPT_R:
					case BAR_RPT_LR:
						DrawRptBar(doc, pL, staff, connStaff, context, dLeft,
										aMeasure->subType, MEDraw, !drawBar);
						break;
					default:
						break;
				}
			}
			
			/* Measure numbers should be drawn regardless of whether the Measure is
			   visible, i.e., whether the barline is drawn. The exact conditions are
			   complex (must be a "real" measure, must be the top or bottom visible
			   staff of the system, etc.), so ask a specialist function. */
			   
			if (ShouldDrawMeasNum(doc, pL, aMeasureL)) {
				measureNum = aMeasure->measureNum+doc->firstMNNumber;
				if (FirstMeasInSys(pL) && doc->sysFirstMN)
					xdMN = pContext->staffLeft+halfLn2d(doc->xSysMNOffset,
														pContext->staffHeight,
														pContext->staffLines);
				else
					xdMN = dLeft+halfLn2d(doc->xMNOffset, pContext->staffHeight,
												pContext->staffLines);
				yOffset = (doc->aboveMN? -doc->yMNOffset :
												2*pContext->staffLines+doc->yMNOffset);
				ydMN = dTop+halfLn2d(yOffset, pContext->staffHeight,
												pContext->staffLines);

				xdMN += std2d(aMeasure->xMNStdOffset, pContext->staffHeight,
												pContext->staffLines);
				ydMN += std2d(aMeasure->yMNStdOffset, pContext->staffHeight,
												pContext->staffLines);

				if (!doc->showFormat)
					DrawMeasNum(doc, xdMN, ydMN, measureNum, pContext);
			}
		}
	}
	
	/* If this is the last measure of a system, just set its measureBBox to end exactly
	   at the end of the system. If in addition the previous measure ends at the same
	   point, this measure's width is zero, and we set its measureBBox to start at that
	   point as well. */
	   
	if (recalc) {
		if (LastMeasInSys(pL)) {
//LogPrintf(LOG_DEBUG, "DrawMEASURE(%u): update measureBBox\n", pL);
			/*  FIXME: This code seems way more complicated than it should be. */
			LINK sysL;  DRect sysDRect;  short sysLeft, boxRight;
						
			sysL = LSSearch(pL, SYSTEMtype, ANYONE, True, False);
			sysDRect = SystemRECT(sysL);
			sysLeft = d2pt(sysDRect.left);
			boxRight = sysLeft+STAFF_LEN(doc);
			if (LinkLSYS(sysL)==NILINK)
				boxRight -= d2pt(doc->dIndentFirst);
			else
				boxRight -= d2pt(doc->dIndentOther);
			p->measureBBox.right = UseMagnifiedSize(boxRight, doc->magnify);
//LogPrintf(LOG_DEBUG, "DrawMEASURE(%u): 1 measureBBox.left=%d right=%d\n", pL,
//p->measureBBox.left, p->measureBBox.right);
			
			prevL = LSSearch(LeftLINK(pL), MEASUREtype, ANYONE, GO_LEFT, False);
			if (prevL && SameSystem(pL, prevL)) {
				prevRight = MeasureBBOX(prevL).right;
				if (prevRight==p->measureBBox.right)
					p->measureBBox.left = p->measureBBox.right;
//LogPrintf(LOG_DEBUG, "DrawMEASURE(%u): 2 measureBBox.left=%d right=%d\n", pL,
//p->measureBBox.left, p->measureBBox.right);
			}
		}
	}

	/* If we're showing measure duration problems, check whether this measure's notated
	   and actual durations agree, and if they don't, shade over the measure. FIXME:
	   If any staves are invisible, this is unreliable! */
	   
	if (doc->showDurProb) ShadeDurPblmMeasure(doc, pL, lastContext);

	if (LinkSEL(pL))
		if (doc->showFormat)
			CheckMEASURE(doc, pL, context, NULL, SMFind, stfRange, enlarge);

PopLock(OBJheap);
PopLock(MEASUREheap);
}


/* ------------------------------------------------------------------------ DrawPSMEAS -- */
/* Draw all subobjects of a PSMEAS object. */

void DrawPSMEAS(Document *doc, LINK pL, CONTEXT context[])
{
	PAPSMEAS	aPSMeas;		/* ptr to current sub item */
	LINK		aPSMeasL;
	PCONTEXT	pContext,		/* ptr to relevant context[] entries */
				pContext2;
	DRect		aDRect;			/* scratch */
	Rect		barlineRect;	/* Rect of current barline */
	short		connStaff;
	DDIST		dTop, dBottom,	/* top of above staff and bottom of below staff */
				dLeft,			/* left edge of staff */
				lnSpace;
	Boolean		drawn,			/* False until a subobject has been drawn */
				recalc;			/* True if we need to recalc object's enclosing rectangle */
	STFRANGE	stfRange = {0,0};
	Point		enlarge = {0,0};

PushLock(OBJheap);
PushLock(PSMEASheap);
	aPSMeasL = FirstSubLINK(pL);
	drawn = False;
	recalc = (!LinkVALID(pL) && outputTo==toScreen);
	for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL)) {
		aPSMeas = GetPAPSMEAS(aPSMeasL);
		
		pContext = &context[NextLimVisStaffn(doc, pL, True, PSMeasSTAFF(aPSMeasL))];
		dTop = pContext->staffTop;
		dLeft = pContext->measureLeft + LinkXD(pL);
		lnSpace = LNSPACE(pContext);

		if (aPSMeas->connStaff==0) {
			dBottom = dTop + pContext->staffHeight;							/* Not connected below */
			if (pContext->showLines==1) dBottom -= lnSpace;			
		}
		else {
			pContext2 = &context[NextLimVisStaffn(doc,pL,False,aPSMeas->connStaff)]; /* Connected below */
			dBottom = pContext2->staffTop + pContext2->staffHeight;
			if (pContext2->showLines==1) dBottom -= LNSPACE(pContext2);			
		}
		if (pContext->showLines==1) dTop += lnSpace;

		if (recalc) {
			SetDRect(&aDRect, dLeft-2, dTop, dLeft, dBottom);
			D2Rect(&aDRect,&barlineRect);

			switch (aPSMeas->subType) {
				case PSM_DOTTED:
					barlineRect.right += 2;
					break;
				case PSM_DOUBLE:
					barlineRect.right += 4;
					break;
				case PSM_FINALDBL:
					barlineRect.right += 5;
					break;
			}

			if (!drawn) {
				LinkOBJRECT(pL) = barlineRect;
				drawn = True;
			}
			else {
				UnionRect(&LinkOBJRECT(pL), &barlineRect, &LinkOBJRECT(pL));
			}
			LinkVALID(pL) = True;
		}
		if (pContext->visible || doc->showInvis) {
			if (aPSMeas->visible || doc->showInvis) {
				if (ShouldPSMDrawBarline(doc, pL, aPSMeasL, &connStaff))
					DrawBarline(doc, pL, PSMeasSTAFF(aPSMeasL), connStaff, context,
									PSMeasType(aPSMeasL));
			}
		}
	}

	if (LinkSEL(pL))
		if (doc->showFormat)
			CheckPSMEAS(doc, pL, context, NULL, SMFind, stfRange, enlarge);

PopLock(OBJheap);
PopLock(PSMEASheap);
}


/* --------------------------------------------------------------------------k DrawSLUR -- */
/* Draw a SLUR object, i.e., a slur or set of ties. */

void DrawSLUR(Document *doc, LINK pL, CONTEXT context[])
{
	PSLUR		p;
	CONTEXT		localContext;
	short		j,
				penThick;				/* vertical pen size in pixels */
	DDIST		xdFirst, ydFirst,		/* DDIST positions of end notes */
				xdLast, ydLast,
				xdCtl0, ydCtl0,			/* ...and of control points */
				xdCtl1, ydCtl1,
				lnSpace;
	Point 		startPt[MAXCHORD], endPt[MAXCHORD];
	DPoint		start, end, c0, c1;
	PASLUR		aSlur;
	LINK		aSlurL;
	Boolean		drawn,					/* False until a subobject has been drawn */
				dim;					/* Should it be dimmed bcs in a voice not being looked at? */
	STFRANGE	stfRange = {0,0};
	Rect		paper;

PushLock(OBJheap);
PushLock(SLURheap);
	p = GetPSLUR(pL);
	GetContext(doc, p->firstSyncL, SlurSTAFF(pL), &localContext);
	if (!localContext.staffVisible) goto Cleanup;
	MaySetPSMusSize(doc, &localContext);

	GetSlurContext(doc, pL, startPt, endPt);			/* Get absolute positions, in DPoints */
	drawn = False;
	
	paper = localContext.paper;

	aSlurL = FirstSubLINK(pL);
	for (j=0; aSlurL; j++, aSlurL = NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
		aSlur->startPt = startPt[j];
		aSlur->endPt = endPt[j];
		xdFirst = p2d(aSlur->startPt.h);  xdLast = p2d(aSlur->endPt.h);
		ydFirst = p2d(aSlur->startPt.v);  ydLast = p2d(aSlur->endPt.v);
	
		xdFirst += aSlur->seg.knot.h;					/* abs. position of slur start */
		ydFirst += aSlur->seg.knot.v;
		xdLast += aSlur->endKnot.h;					/* abs. position of slur end */
		ydLast += aSlur->endKnot.v;
		
		switch (outputTo) {
			case toBitmapPrint:
			case toPICT:
				/* Convert context to paper-relative coords */
				OffsetRect(&paper, -paper.left, -paper.top);
				/* Fall through */
			case toScreen:
				/* If slur is not in voice being "looked" at, dim it */
				p = GetPSLUR(pL);
				dim = (outputTo==toScreen && !LOOKING_AT(doc, p->voice));
				if (dim) PenPat(NGetQDGlobalsGray());
				
				/* If staff size is large, thicken slur very crudely. We don't try to do
				   better because (1) it'll never look good at screen resolution unless
				   the staff size is enormous (e.g., at 400% magnification or above), (2)
				   doing anything that looks really good (including tapering ends!)  will
				   probably slow drawing way down, and (3) precise registration of anything
				   against slurs is rarely important. */
				   
				lnSpace = LNSPACE(&context[SlurSTAFF(pL)]);
				penThick = d2p(lnSpace/5);
				if (penThick<1) penThick = 1;
				if (penThick>2) penThick = 2;
				PenSize(1, penThick);
				ForeColor(Voice2Color(doc, p->voice));

				GetSlurPoints(aSlurL, &start, &c0, &c1, &end);
				StartSlurBounds();
				DrawSlursor(&paper, &start, &end, &c0, &c1, S_Whole, aSlur->dashed);
				EndSlurBounds(&paper, &aSlur->bounds);

				PenNormal();
				ForeColor(blackColor);
				if (drawn)
					UnionRect(&LinkOBJRECT(pL), &aSlur->bounds, &LinkOBJRECT(pL));
				else
					LinkOBJRECT(pL) = aSlur->bounds;
				LinkVALID(pL) = True;
				drawn = True;
				break;
			case toPostScript:
				xdCtl0 = xdFirst + aSlur->seg.c0.h;
				ydCtl0 = ydFirst + aSlur->seg.c0.v;
				xdCtl1 = xdLast + aSlur->seg.c1.h;
				ydCtl1 = ydLast + aSlur->seg.c1.v;
				PS_Slur(xdFirst, ydFirst, xdCtl0, ydCtl0, xdCtl1, ydCtl1, xdLast,
							ydLast, aSlur->dashed);
				break;
		}
	}

	if (outputTo==toScreen && LinkOBJRECT(pL).top>=LinkOBJRECT(pL).bottom)
			LinkOBJRECT(pL).top = LinkOBJRECT(pL).bottom-1;
	if (LinkSEL(pL))
		if (doc->showFormat)
			CheckSLUR(doc, pL, context, NULL, SMFind, stfRange);

Cleanup:

PopLock(OBJheap);
PopLock(SLURheap);
}
