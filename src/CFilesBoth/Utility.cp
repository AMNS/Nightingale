/***************************************************************************
*	FILE:	Utility.c
*	PROJ:	Nightingale
*	DESC:	Miscellaneous utility routines that needed a home
		CalcYStem				GetNoteYStem			GetGRNoteYStem
		ShortenStem				GetCStemInfo			GetStemInfo
		GetCGRStemInfo			GetGRStemInfo
		GetLineAugDotPos		ExtraSysWidth			MyDebugPrintf
		ApplHeapCheck			Char2Dur				Dur2Char
		GetSymTableIndex		SymType					Objtype2Char
		StrToObjRect			GetNFontInfo			NStringWidth
		NPtStringWidth			NPtGraphicWidth			GetNPtStringBBox
		MaxPartNameWidth		PartNameMargin
		SetFont					StringRect
		AllocContext			AllocSpTimeInfo			NewGrafPort
		DisposGrafPort			SamePoint
		D2Rect					Rect2D					PtRect2D
		AddDPt					SetDPt					SetDRect
		OffsetDRect				InsetDRect				DMoveTo
		GCD						RoundDouble				RoundSignedInt
		InterpY					FindIntInString
		BlockCompare			RelIndexToSize			GetTextSize
		FontName2Index			User2HeaderFontNum		Header2UserFontNum
		Rect2Window				Pt2Window
		Pt2Paper				GlobalToPaper			RefreshScreen
		InitSleepMS				SleepMS					SleepTicks
		SleepTicksWaitButton	NMIDIVersion			StdVerNumToStr
		PlayResource
/***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include <ctype.h>
#include "Nightingale.appl.h"


/* ------------------------------------------------------------------- CalcYStem -- */
/*	Calculate optimum stem endpoint for a note. */

DDIST CalcYStem(
			Document *doc,
			DDIST		yhead,			/* position of note head */
			short		nflags,			/* number of flags or beams */
			Boolean	stemDown,		/* TRUE if stem is down, else stem is up */
			DDIST		staffHeight,	/* staff height */
			short		staffLines,
			short		qtrSp,			/* desired "normal" (w/2 or less flags) stem length (qtr-spaces) */
			Boolean	noExtend 		/* TRUE means don't extend stem to midline */
			)
{
	DDIST		midline, dLen, ystem;

	/*
	 * There are two reasons why we may have to make the stem longer than
	 * requested: the stem has to be long enough to accomodate the flags, and
	 * if the stem is pointing towards the center of the staff (the normal case),
	 * it should normally extend at least to the center of the staff.
	 */
// FIXME: Perhaps use flag leading info, instead of just assuming leading == 1 space
	if (MusFontHas16thFlag(doc->musFontInfoIndex)) {
		if (nflags>2) qtrSp += 4*(nflags-2);			/* Every flag after 1st 2 adds a space */
	}
	else {
		if (nflags>1) qtrSp += 4*(nflags-1);			/* Every flag after 1st adds a space */
	}
	dLen = qtrSp*staffHeight/(4*(staffLines-1));
	ystem = (stemDown ? yhead+dLen : yhead-dLen);		/* Initially, set length to <qtrSp> */

	if (!noExtend) {
		/* FIXME: If staffLines is even, extended stem end terminates in mid-space. OK? */
		midline = staffHeight/2;
		if (ABS(yhead-midline)>dLen &&					/* Would ending at midline lengthen */
				ABS(ystem-midline)<ABS(yhead-midline))	/*   without changing direction? */
			ystem = midline;							/* Yes, end there */
	}
	return ystem;
}


/* ----------------------------------------------------------------- GetNoteYStem -- */
/*	Calculate optimum stem endpoint for a note in main data structure, considering voice
role, but assuming note is not in a chord and not considering beaming. Cf. GetNCYStem
in Objects.c. */

DDIST GetNoteYStem(Document *doc, LINK syncL, LINK aNoteL, CONTEXT context)
{
	Boolean stemDown; QDIST qStemLen; DDIST ystem;
	
	stemDown = GetStemInfo(doc, syncL, aNoteL, &qStemLen);
	ystem = CalcYStem(doc, NoteYD(aNoteL), NFLAGS(NoteType(aNoteL)),
							stemDown,
							context.staffHeight, context.staffLines,
							qStemLen, FALSE);
	return ystem;
}


/* -------------------------------------------------------------- GetGRNoteYStem -- */
/*	Calculate optimum stem endpoint for a grace note in main data structure, assuming
normal (one voice per staff) rules, assuming it's not in a chord and not considering
beaming. Cf. GetGRCYStem in Objects.c. */

DDIST GetGRNoteYStem(LINK aGRNoteL, CONTEXT context)
{
	PAGRNOTE aGRNote;
	DDIST stemLen;

	stemLen = qd2d(config.stemLenGrace, context.staffHeight, context.staffLines);
	
	aGRNote = GetPAGRNOTE(aGRNoteL);
	if (GRMainNote(aGRNoteL))
		return GRNoteYD(aGRNoteL)-stemLen;
	else
		return GRNoteYD(aGRNoteL);
}


/* ----------------------------------------------------------------- ShortenStem -- */
/*	Returns TRUE if the given note and stem direction require a shorter-than-normal
stem; intended for use in 2-voice notation. */

Boolean ShortenStem(LINK aNoteL, CONTEXT context, Boolean stemDown)
{
	PANOTE 	aNote;
	short	halfLn, midCHalfLn;
	QDIST	yqpit;
	
	midCHalfLn = ClefMiddleCHalfLn(context.clefType);				/* Get middle C staff pos. */		
	aNote = GetPANOTE(aNoteL);
	yqpit = aNote->yqpit+halfLn2qd(midCHalfLn);						/* For notehead... */
	halfLn = qd2halfLn(yqpit);										/* Half-lines below stftop */
	
	if (halfLn<(0+STRICT_SHORTSTEM) && !stemDown) return TRUE;			/* Above staff so shorten */
	if (halfLn>(2*(context.staffLines-1)-STRICT_SHORTSTEM) && stemDown)	/* Below staff so shorten */
		 return TRUE;
	return FALSE;
}


/* ----------------------------------------------------------------- GetCStemInfo -- */
/* Given a note and a context, return (in <qStemLen>) its normal stem length and (as
function value) whether it should be stem down. Considers voice role but assumes the
note is not in a chord. */

Boolean GetCStemInfo(Document *doc, LINK /*syncL*/, LINK aNoteL, CONTEXT context, 
						short *qStemLen)
{
	Byte voiceRole; PANOTE aNote;
	short halfLn, midCHalfLn; QDIST yqpit;
	Boolean stemDown;
	LINK partL; PPARTINFO pPart;
	
	voiceRole = doc->voiceTab[NoteVOICE(aNoteL)].voiceRole;

	switch (voiceRole) {
		case SINGLE_DI:
			midCHalfLn = ClefMiddleCHalfLn(context.clefType);		/* Get middle C staff pos. */		
			aNote = GetPANOTE(aNoteL);
			yqpit = aNote->yqpit+halfLn2qd(midCHalfLn);
			halfLn = qd2halfLn(yqpit);								/* Number of half lines from stftop */
			stemDown = (halfLn<=context.staffLines-1);
			*qStemLen = QSTEMLEN(TRUE, ShortenStem(aNoteL, context, stemDown));
			break;
		case UPPER_DI:
			stemDown = FALSE;
			*qStemLen = QSTEMLEN(FALSE, ShortenStem(aNoteL, context, stemDown));
			break;
		case LOWER_DI:
			stemDown = TRUE;
			*qStemLen = QSTEMLEN(FALSE, ShortenStem(aNoteL, context, stemDown));
			break;
		case CROSS_DI:
			partL = FindPartInfo(doc, Staff2Part(doc, NoteSTAFF(aNoteL)));
			pPart = GetPPARTINFO(partL);
			stemDown = (NoteSTAFF(aNoteL)==pPart->firstStaff);
			*qStemLen = QSTEMLEN(FALSE, ShortenStem(aNoteL, context, stemDown));
			break;
	}
	
	return stemDown;
}


/* ----------------------------------------------------------------- GetStemInfo -- */
/* Given a note, return (in <qStemLen>) its normal stem length and (as function
value) whether it should be stem down. Considers voice role but assumes the note
is not in a chord. */

Boolean GetStemInfo(Document *doc, LINK syncL, LINK aNoteL, short *qStemLen)
{
	CONTEXT context;
	
	GetContext(doc, syncL, NoteSTAFF(aNoteL), &context);
	return GetCStemInfo(doc, syncL, aNoteL, context, qStemLen);
}


/* --------------------------------------------------------------- GetCGRStemInfo -- */
/* Given a grace note and a context, return (in <qStemLen>) its normal stem length and
(as function value) whether it should be stem down. Considers voice role but assumes
the grace note is not in a chord. */

Boolean GetCGRStemInfo(Document *doc, LINK /*grSyncL*/, LINK aGRNoteL, CONTEXT /*context*/,
							short *qStemLen)
{
	Byte voiceRole; Boolean stemDown;
	LINK partL; PPARTINFO pPart;
	
	voiceRole = doc->voiceTab[GRNoteVOICE(aGRNoteL)].voiceRole;

	switch (voiceRole) {
		case SINGLE_DI:
		case UPPER_DI:
			stemDown = FALSE;
			break;
		case LOWER_DI:
			stemDown = TRUE;
			break;
		case CROSS_DI:
			partL = FindPartInfo(doc, Staff2Part(doc, GRNoteSTAFF(aGRNoteL)));
			pPart = GetPPARTINFO(partL);
			stemDown = (GRNoteSTAFF(aGRNoteL)==pPart->firstStaff);
			break;
	}
	
	*qStemLen = config.stemLenGrace;
	return stemDown;
}


/* ---------------------------------------------------------------- GetGRStemInfo -- */
/* Given a grace note, return (in <qStemLen>) its normal stem length and (as function
value) whether it should be stem down. Considers voice role but assumes the grace note
is not in a chord. */

Boolean GetGRStemInfo(Document *doc, LINK grSyncL, LINK aGRNoteL, short *qStemLen)
{
	CONTEXT context;
	
	GetContext(doc, grSyncL, GRNoteSTAFF(aGRNoteL), &context);
	return GetCGRStemInfo(doc, grSyncL, aGRNoteL, context, qStemLen);
}


/* ------------------------------------------------------------ GetLineAugDotPos -- */
/* Return the normal augmentation dot y-position for a note on a line. */
							
short GetLineAugDotPos(
			short	voiceRole,	/* UPPER_DI, LOWER_DI, CROSS_DI, or SINGLE_DI */
			Boolean	stemDown
			)
{
	if (voiceRole==SINGLE_DI) return 1;
	else					  return (stemDown? 3 : 1);
}


/* --------------------------------------------------------------- ExtraSysWidth -- */
/* Compute extra space needed at the right end of the system to allow space for
objRects of right-justified barlines. In DrawMeasure the width allowed for the
widest barline (final double) is 5 ( ? pixels). A lineSp equals 4 pixels at default
rastral/mag (= 5, 100), so allow 5/4 of a lineSp here, so that user can click on
any part of the widest barline's objRect (e.g. for insertion of graphics). This code
should be reviewed when barline's objRects are properly computed. Empirically,
however, as of v.995, 1 lnSpace is perfect for all barline types. */

DDIST ExtraSysWidth(Document *doc)
{
	DDIST dLineSp;
	
	dLineSp = drSize[doc->srastral]/(STFLINES-1);			/* Space between staff lines */
	return dLineSp;
}


/* --------------------------------------------------------------- ApplHeapCheck -- */
/* Mac-specific: Check the heap at a location of your choice in the code. Breaks into
MacsBug, checks the heap, and continues if the heap is OK. Unfortunately, also makes
the screen flash. */

void ApplHeapCheck()
{
	DebugStr("\p;hc;g");
}


/* -------------------------------------------------------------------- Char2Dur -- */
/*	Convert input character to duration code */

short Char2Dur(char token)
{
	short j;
	
	for (j=0; j<nsyms; j++)
		if (token==symtable[j].symcode)
			return symtable[j].durcode;
	return NOMATCH;									/* Illegal token */
}


/* -------------------------------------------------------------------- Dur2Char -- */
/*	Convert duration code to input character */

short Dur2Char(short dur)
{
	short j;
	
	for (j=0; j<nsyms; j++)
		if (dur==symtable[j].durcode)
			return symtable[j].symcode;
	return NOMATCH;									/* Illegal duration code */
}


/* ------------------------------------------------------------ GetSymTableIndex -- */
/*	Return symtable index for CMN character token. */

short GetSymTableIndex(char token)
{
	short j;
	
	for (j=0; j<nsyms; j++)
		if (token==symtable[j].symcode)
			return j;								/* Found it */
	return NOMATCH;								/* Not found - illegal */
}


/* --------------------------------------------------------------------- SymType -- */
/*	Return the symbol type for the specified character. */

short SymType(char ch)
{
	short index;

	index = GetSymTableIndex(ch);
	if (index==NOMATCH)
		return NOMATCH;
	else
		return symtable[index].objtype;
}


/* ---------------------------------------------------------------- Objtype2Char -- */
/*	Return the (first, if there's more than one) input character for objtype in
symtable. */

char Objtype2Char(SignedByte objtype)
{
	short j;
	
	for (j=0; j<nsyms; j++)
		if (objtype==symtable[j].objtype)
			return symtable[j].symcode;						/* Found it */
	return '\0';											/* Not found - illegal */
}


/* ---------------------------------------------------------------- StrToObjRect -- */
/*	Get the objRect for a string of characters. */

Rect StrToObjRect(unsigned char *string)
{
	short	nchars;
	Rect	strRect, tempR;
	Boolean	rectNotSet = TRUE;
	
	nchars = string[0];
	while (nchars > 0) {
		if (rectNotSet) {
			strRect = CharRect(string[nchars]);
			OffsetRect(&strRect, -strRect.left, 0);
			rectNotSet = FALSE;
		}
		else {
			tempR = CharRect(string[nchars]);
			OffsetRect(&tempR, -tempR.left, 0);
			OffsetRect(&tempR, strRect.right, 0);
			UnionRect(&strRect, &tempR, &strRect);
		}
		nchars--;
	}
	return strRect;
}


/* ---------------------------------------------------------------- GetNFontInfo -- */

void GetNFontInfo(
			short fontID,
			short size,			/* in points, i.e., pixels at 100% magnification */
			short style,
			FontInfo *pfInfo)
{
	short oldFont, oldSize, oldStyle;

	oldFont = GetPortTxFont();
	oldSize = GetPortTxSize();
	oldStyle = GetPortTxFace();

	TextFont(fontID);
	TextSize(size);
	TextFace(style);
		
	GetFontInfo(pfInfo);

	TextFont(oldFont);
	TextSize(oldSize);
	TextFace(oldStyle);
}


/* ---------------------------------------------------------------- NStringWidth -- */
/* Compute and return the StringWidth (in pixels) of the given string in the
given font, size and style. */

short NStringWidth(Document */*doc*/, const Str255 string, short font, short size, short style)
{
	short	oldFont, oldSize, oldStyle, width;

	oldFont = GetPortTxFont();
	oldSize = GetPortTxSize();
	oldStyle = GetPortTxFace();

	TextFont(font);
	TextSize(size);
	TextFace(style);
		
	width = StringWidth(string);

	TextFont(oldFont);
	TextSize(oldSize);
	TextFace(oldStyle);
	
	return width;
}


/* --------------------------------------------------------------- NPtStringWidth -- */
/* Compute and return the StringWidth (in points, not pixels) of the given Pascal
string in the given font, size and style. By far the easiest way to get the width
of a string on the Mac is from QuickDraw, for the screen, but it's limited to screen
resolution. To get greater accuracy, we set magnification as high as possible before
calling NStringWidth, then restore its previous value. Note that since we're
returning a value in integer points, the caller doesn't get maximum accuracy, but
1-point accuracy is enough for Nightingale.

To be honest, nothing is really guaranteed. Tech Note #26: Fond of FONDs (May 1992)
discusses finding the width of a piece of text, and makes it clear that (in the general
case) it's far more difficult than a reasonable person would expect, especially with
bitmapped fonts. */

short NPtStringWidth(
			Document *doc,
			const Str255 string,
			short font,
			short size,			/* in points, i.e., pixels at 100% magnification */
			short style)
{
	short saveMagnify, width;
	
	saveMagnify = doc->magnify;
	doc->magnify = MAX_MAGNIFY;
	InstallMagnify(doc);
	
	size = UseTextSize(size, doc->magnify);
	width = NStringWidth(doc, string, font, size, style);
	width = p2pt(width);
	
	doc->magnify = saveMagnify;
	InstallMagnify(doc);
	
	return width;
}


/* ------------------------------------------------------------- NPtGraphicWidth -- */
/* Compute and return the StringWidth (in points) of the given Graphic. */	

short NPtGraphicWidth(Document *doc, LINK pL, PCONTEXT pContext)
{
	short		font, fontSize, fontStyle;
	PGRAPHIC	p;
	Str255		string;
	DDIST		lineSpace;
	
	PStrCopy((StringPtr)PCopy(GetPAGRAPHIC(FirstSubLINK(pL))->strOffset),
				(StringPtr)string);

	p = GetPGRAPHIC(pL);
	font = doc->fontTable[p->fontInd].fontID;
	fontStyle = p->fontStyle;

	lineSpace = LNSPACE(pContext);
	fontSize = GetTextSize(p->relFSize, p->fontSize, lineSpace);

	return NPtStringWidth(doc, string, font, fontSize, fontStyle);
}


/* ------------------------------------------------------------ GetNPtStringBBox -- */
/* For the given string and font description, return the bounding box in points.
Normally gets the height from the ascent and descent returned by GetNFontInfo, which
calls GetFontInfo. For "normal" fonts, this will often be more than it really should
be, but the difference will rarely be too great; but Sonata and other music fonts
have enormous ascent and descent, so the height would be much too large. To avoid this
problem, if the font is either Sonata OR the current music font, we use the ascent and
descent of the actual characters in the string as given by GetMusicAscDesc instead of
the font's ascent and descent. */

void GetNPtStringBBox(
			Document *doc,
			Str255 string,				/* Pascal string */
			short fontID,
			short size,					/* in points, i.e., pixels at 100% magnification */
			short style,
			Boolean multiLine,			/* has multiple lines, delimited by CH_CR */
			Rect *bBox)					/* Bounding box for string, with TOP_LEFT at 0,0 and no margin */
{
	short width, ascent, descent, nLines, lineHt;
	FontInfo fInfo;

	nLines = 0;

	if (multiLine) {							/* Count lines; take width from longest line. */
		short i, j, len, count, wid;
		Str255 str;
		Byte *p;

		PStrCopy(string, str);

		p = str;
		len = str[0];
		j = count = width = 0;
		for (i = 1; i <= len; i++) {
			count++;
			if (str[i] == CH_CR || i == len) {
				str[j] = count;
				wid = NPtStringWidth(doc, &str[j], fontID, size, style);
				if (wid > width)
					width = wid;
				j = i;
				count = 0;
				nLines++;
			}
		}
		if (string[len] == CH_CR)			/* Don't count a new line if last char is CR. */
			nLines--;
	}
	else
		width = NPtStringWidth(doc, string, fontID, size, style);

	bBox->left = 0;
	bBox->right = width;

	if (fontID==doc->musicFontNum || fontID==sonataFontNum) {
		GetMusicAscDesc(doc, string, size, &ascent, &descent);
		if (multiLine) {
			lineHt = ascent + descent;	// FIXME: what about leading?
			bBox->top = -ascent;
			bBox->bottom = descent + (lineHt * (nLines-1));
		}
		else {
			bBox->top = -ascent;
			bBox->bottom = descent;
		}
	}
	else {
		GetNFontInfo(fontID, size, style, &fInfo);
		if (multiLine) {
			lineHt = fInfo.ascent + fInfo.descent + fInfo.leading;
			bBox->top = -fInfo.ascent;
			bBox->bottom = fInfo.descent + (lineHt * (nLines-1));
			LogPrintf(LOG_DEBUG, "GetNPtStringBBox: fontID, Size, Style=%d, %d, %d fInfo.ascent=%d .descent=%d .leading=%d lineHt=%d\n",
					  fontID, size, style, fInfo.ascent, fInfo.descent, fInfo.leading, lineHt);
		}
		else {
			bBox->top = -fInfo.ascent;
			bBox->bottom = fInfo.descent;
		}
	}
}


/* ------------------------------------------------------------ MaxPartNameWidth -- */
/* For the given document and code for the form of part names, return the
maximum width needed by any part name, in points. */

short MaxPartNameWidth(
			Document *doc,
			short nameCode)			/* 0=show none, 1=show abbrev., 2=show full names */
{
	LINK measL, partL;
	PPARTINFO pPart;
	short fontInd, font, fontSize;
	short nameWidth, maxWidth;
	CONTEXT context;
	Str255 string;

	if (nameCode<=0) return 0;
	
	maxWidth = 0;

	measL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	partL = FirstSubLINK(doc->masterHeadL);
	for (partL=NextPARTINFOL(partL); partL; partL=NextPARTINFOL(partL)) {	/* Skip unused first partL */
		pPart = GetPPARTINFO(partL);
		GetContext(doc, measL, pPart->lastStaff, &context);
		pPart = GetPPARTINFO(partL);
		strcpy((char *)string, (nameCode==1? pPart->shortName : pPart->name));
		CToPString((char *)string);

		fontInd = FontName2Index(doc, doc->fontNamePN);							/* Should never fail */
		font = doc->fontTable[fontInd].fontID;
		fontSize = GetTextSize(doc->relFSizePN, doc->fontSizePN, LNSPACE(&context));

		nameWidth = NPtStringWidth(doc, string, font, fontSize, doc->fontStylePN);
		maxWidth = n_max(maxWidth, nameWidth);
	}

	return maxWidth;
}


/* -------------------------------------------------------------- PartNameMargin -- */

double PartNameMargin(
			Document *doc,
			short nameCode)			/* 0=show none, 1=show abbrev., 2=show full names */
{
	short nameWidth;
	double inchDist;
	
	if (nameCode>0) {
		nameWidth = MaxPartNameWidth(doc, nameCode);
		/* Need more space for Connects. Add enough for a CONNECTCURLY as an approximation */
		nameWidth += d2pt(ConnectDWidth(doc->srastral, CONNECTCURLY));
		inchDist = pt2in(nameWidth);
		inchDist = RoundDouble(inchDist, .01);
	}
	else
		inchDist = 0.0;
		
	return inchDist;
}


/* --------------------------------------------------------------------- SetFont -- */
/* Set one of Nightingale's standard fonts. Obsolescent. */

void SetFont(short which)
{
	switch (which) {
		default:
			MayErrMsg("SetFont: illegal argument");	/* and drop thru to... */
		case 0:	
			TextFont(0);								/* Set to the system font */
			TextSize(12);
			break;
		case 1:	
			TextFont(textFontNum);						/* Set to our standard text font */
			TextSize(textFontSmallSize);
			break;
		case 2:	
			TextFont(128);								/* (Unused) Set to small Boston II */
			TextSize(8);
			break;
		case 3:	
			TextFont(32401);							/* Set to tiny LittleNight (really about 7 pt.) */
			TextSize(9);
			break;
		case 4:	
			TextFont(textFontNum);						/* Set to our standard text font */
			TextSize(textFontSize);
			break;
	}
	
	TextFace(0);										/* Plain */
}

/* ------------------------------------------------ AllocContext,AllocSpTimeInfo -- */
/* Allocate arrays on the heap: context[MAXSTAVES+1], spTimeInfo[MAX_MEASNODES]. */

PCONTEXT AllocContext()
{
	PCONTEXT pContext;

	pContext = (PCONTEXT)NewPtr((Size)sizeof(CONTEXT)*(MAXSTAVES+1));
	if (!GoodNewPtr((Ptr)pContext)) {
		OutOfMemory((long)sizeof(CONTEXT)*(MAXSTAVES+1));
		return NULL;
	}
	return pContext;
}

SPACETIMEINFO *AllocSpTimeInfo()
{
	SPACETIMEINFO *spTimeInfo;

	spTimeInfo = (SPACETIMEINFO *)NewPtr((Size)MAX_MEASNODES*sizeof(SPACETIMEINFO));
	if (!GoodNewPtr((Ptr)spTimeInfo)) {
		OutOfMemory((long)MAX_MEASNODES*sizeof(SPACETIMEINFO));
		return NULL;
	}
	return spTimeInfo;
}


/* ------------------------------------------------------ MakeGWorld and friends -- */
/* Functions for managing offscreen graphics ports using Apple's GWorld mechanism.
These make it easy to work with color images.  See Apple documentation
("Offscreen Graphics Worlds.pdf") for more.   JGG, 8/11/01 */

/* ------------------------------------------------------------------ MakeGWorld -- */
/* Create a new graphics world (GWorld) for offscreen drawing, having
the given dimensions.  If <lock> is TRUE, the PixMap of this GWorld will be
locked on return.  Returns the new GWorldPtr, or NULL if error. */

GWorldPtr MakeGWorld(short width, short height, Boolean lock)
{
	CGrafPtr		origGrafPtr;
	GDHandle		origDevH;
	GWorldPtr		theGWorld;
	PixMapHandle	pixMapH;
	Rect			portRect;
	Boolean			result;
	QDErr			err;

	SetRect(&portRect, 0, 0, width, height);

	err = NewGWorld(&theGWorld, 0, &portRect, NULL, NULL, 0);
	if (theGWorld==NULL || err!=noErr) {
		NoMoreMemory();
		return NULL;
	}
	pixMapH = GetGWorldPixMap(theGWorld);
	result = LockPixels(pixMapH);
	if (!result) {			/* This shouldn't happen with the kind of GWorld we make. */
		DisposeGWorld(theGWorld);
		NoMoreMemory();
		return NULL;
	}

	/* Erase pixels */
	GetGWorld(&origGrafPtr, &origDevH);
	SetGWorld(theGWorld, NULL);
		
	Rect portR;
	GetPortBounds(theGWorld, &portR);
	EraseRect(&portR);
	
	//EraseRect(&theGWorld->portRect);
	SetGWorld(origGrafPtr, origDevH);

	if (!lock)
		UnlockPixels(pixMapH);

	return theGWorld;
}


/* --------------------------------------------------------------- DestroyGWorld -- */
void DestroyGWorld(GWorldPtr theGWorld)
{
	PixMapHandle pixMapH;

	pixMapH = GetGWorldPixMap(theGWorld);
	UnlockPixels(pixMapH);		/* Presumably this is OK even if PixMap is unlocked. */
	DisposeGWorld(theGWorld);
}


/* ------------------------------------------------ SaveGWorld and RestoreGWorld -- */
/* NOTE: These are not reentrant!  So be careful to keep things in sync. */

static CGrafPtr savePort = NULL;		/* NB: Can also represent GrafPtr or GWorldPtr */
static GDHandle saveDevice = NULL;

Boolean SaveGWorld()
{
	if (savePort!=NULL || saveDevice!=NULL)
		return FALSE;				/* our temporary data already in use */
	GetGWorld(&savePort, &saveDevice);
	return TRUE;
}

Boolean RestoreGWorld()
{
	if (savePort==NULL || saveDevice==NULL)
		return FALSE;		/* SaveGWorld hasn't been called since last RestoreGWorld. */
	SetGWorld(savePort, saveDevice);
	savePort = NULL;
	saveDevice = NULL;
	return TRUE;
}


/* ------------------------------------------------- LockGWorld and UnlockGWorld -- */
Boolean LockGWorld(GWorldPtr theGWorld)
{
	PixMapHandle	pixMapH;

	pixMapH = GetGWorldPixMap(theGWorld);
	return (LockPixels(pixMapH));
}

void UnlockGWorld(GWorldPtr theGWorld)
{
	PixMapHandle	pixMapH;

	pixMapH = GetGWorldPixMap(theGWorld);
	UnlockPixels(pixMapH);
}


/* ----------------------------------------------------------------- NewGrafPort -- */
/* Create a new GrafPort large enough to hold a bit image of the specified width
and height.  Returns a pointer to the GrafPort, but does NOT set it to be the
current port.

The GrafPort thus created should be disposed of with the DisposGrafPort function. */

GrafPtr NewGrafPort(short width, short height)	/* required size of requested GrafPort */
{

	GrafPtr	oldPort,
			ourPort;
	Rect	ourPortRect;		/* enclosing rectangle for our GrafPort */
	
	GetPort(&oldPort);
	ourPort = CreateNewPort();
	SetPort(ourPort);

	SetRect(&ourPortRect, 0, 0, width, height);
	PortSize(ourPortRect.right, ourPortRect.bottom);
	SetOrigin(0,0);
	ClipRect(&ourPortRect);
	EraseRect(&ourPortRect);
	
	SetPort(oldPort);										/* restore original GrafPort */
	return ourPort;
}


/* -------------------------------------------------------------- DisposGrafPort -- */
/* Deallocate memory used by a GrafPort created with NewGrafPort. */

void DisposGrafPort(GrafPtr aPort)
{
	DisposePort(aPort);
}


/* ---------------------------------------------------------------------- D2Rect -- */
/* Convert DRect to Rect in pixels */

void D2Rect(DRect *dRect, Rect *rRect)
{
	rRect->top = d2p(dRect->top);
	rRect->left = d2p(dRect->left);
	rRect->bottom = d2p(dRect->bottom);
	rRect->right = d2p(dRect->right);
}


/* ---------------------------------------------------------------------- Rect2D -- */
/* Convert Rect in pixels to DRect */

void Rect2D(Rect *rRect, DRect *dRect)
{
	dRect->top = p2d(rRect->top);
	dRect->left = p2d(rRect->left);
	dRect->bottom = p2d(rRect->bottom);
	dRect->right = p2d(rRect->right);
}

/* -------------------------------------------------------------------- PtRect2D -- */
/* Convert Rect in points to DRect */

void PtRect2D(Rect *rRect, DRect *dRect)
{
	dRect->top = pt2d(rRect->top);
	dRect->left = pt2d(rRect->left);
	dRect->bottom = pt2d(rRect->bottom);
	dRect->right = pt2d(rRect->right);
}


/* ---------------------------------------------------------------------- AddDPt -- */
/* Add DPoint <src> to <*dest>. */

void AddDPt(DPoint src, DPoint *dest)
{
	dest->h += src.h;
	dest->v += src.v;
}


/* ---------------------------------------------------------------------- SetDPt -- */
/* Initialize a DPoint with the given DDIST coordinates */

void SetDPt(DPoint *dPoint, DDIST dx, DDIST dy)
{
	dPoint->h = dx;
	dPoint->v = dy;
}


/* -------------------------------------------------------------------- SetDRect -- */
/* Initialize a DRect with the given DDIST coordinates */

void SetDRect(DRect *dRect, DDIST dLeft, DDIST dTop, DDIST dRight, DDIST dBottom)
{
	dRect->top = dTop;
	dRect->left = dLeft;
	dRect->bottom = dBottom;
	dRect->right = dRight;
}


/* ----------------------------------------------------------------- OffsetDRect -- */
/* Offset a DRect by the specified DDIST amounts */

void OffsetDRect(DRect *dRect, DDIST dx, DDIST dy)
{
	dRect->top += dy;
	dRect->left += dx;
	dRect->bottom += dy;
	dRect->right += dx;
}


/* ------------------------------------------------------------------ InsetDRect -- */
/* Inset a DRect by the specified DDIST amounts */

void InsetDRect(DRect *dRect, DDIST dx, DDIST dy)
{
	dRect->top += dy;
	dRect->left += dx;
	dRect->bottom -= dy;
	dRect->right -= dx;
}


/* --------------------------------------------------------------------- DMoveTo -- */
/* MoveTo with DDIST args. */

void DMoveTo(DDIST xd, DDIST yd)
{
	MoveTo(d2p(xd), d2p(yd));
}


/* ------------------------------------------------------------------------- GCD -- */
/* Euclid's algorithm for GCD of two integers, from Knuth, v.1 p.2. */

short GCD(short m, short n)
{
	short r;
		
	if (!n) {
		MayErrMsg("GCD called with n = 0");
		return 0;
	}

	r = 1;
	for ( ; ; m = n, n = r) {
		r = m % n;
		if (!r) break;
	}
	
	return n;
}


/* ----------------------------------------------------------------- RoundDouble -- */
/* Round <value> to the nearest multiple of <quantum>. Since it uses integer
arithmetic, overflow is possible, but we don't do any checking. */

double RoundDouble(double value, double quantum)
{
	long multiple;
	
	multiple = (value/quantum)+.5;
	return multiple*quantum;
}


/* --------------------------------------------------------------- RoundSignedInt -- */
/* Given a signed <value> and a non-zero <quantum>, round <value> to the nearest
multiple of <quantum>. FIXME: Not sure this will work if <quantum> is negative! */

short RoundSignedInt(short value, short quantum)
{
	short uRound, sRound, multiple;
	
	uRound = quantum/2;
	sRound = (value>0? uRound : -uRound);
	multiple = (value+sRound)/quantum;
	value = multiple*quantum;
	return value;
}


/* ---------------------------------------------------------------------- InterpY -- */
/* Use linear interpolation to find the y-coord. of a point on the line from (x0,y0)
to (x1,y1) corresponding to x-coord. ptx. This is a fast version for integer
coordinates, done in homebrew fixed-point arithmetic. */

short InterpY(short x0, short y0, short x1, short y1, short ptx)
{
	long prop, dyBig; short y;

	if (x1==x0) return y1;										/* Avoid possible divide by 0 */

	prop = 10000L*(long)(ptx-x0)/(long)(x1-x0);
	dyBig = prop*(y1-y0);
	y = y0+(dyBig/10000L);
	return y;
}

/* ------------------------------------------------------------- FindIntInString -- */
/* Find the first recognizable unsigned (long) integer, simply a string of digits,
in the given string. If the string contains no digits, return -1. */

long FindIntInString(unsigned char *string)
{
	long number, digit; short i; Boolean foundDigits;
	
	number = 0L;
	foundDigits = FALSE;
	for (i = 1; i<=string[0]; i++) {
		if (isdigit(string[i])) {
			if (number>BIGNUM/10L) return number;			/* Avoid overflow */
			digit = string[i]-'0';
			number = (10L*number)+digit;
			foundDigits = TRUE;
		}
		else if (foundDigits)
			return number;
		else ;												/* No digits found yet: keep going */
	}
	
	return (foundDigits? number : -1L);
}


/* ---------------------------------------------------------------- BlockCompare -- */
/* Compare two structures byte by byte for a given length until either they match,
in which case we deliver 0, or until the first is less than or greater than
the second, in which case we deliver -1 or 1 respectively. */

short BlockCompare(void *blk1, void *blk2, short len)
	{
		Byte *b1, *b2;
		
		b1 = (Byte *)blk1; b2 = (Byte *)blk2;
		while (len-- > 0) {
			if (*b1 < *b2) return -1;
			if (*b1++ > *b2++) return 1;
			}
		return 0;
	}


/* -------------------------------------------------------------- RelIndexToSize -- */
/*	Deliver the absolute size (in points) that the Tiny...Jumbo...StaffHeight
menu item for text Graphics represents, or 0 if out of bounds. */

short RelIndexToSize(short index, DDIST lineSpace)
	{
		short theRelSize = 0;
		
		if (index>=GRTiny && index<=GRLastSize) {
			theRelSize = relFSizeTab[index]*lineSpace;
			theRelSize = d2pt(theRelSize);
		}
		return theRelSize;
	}


/* ----------------------------------------------------------------- GetTextSize -- */
/* Given the relative/absolute size flag, nominal (Nightingale internal) size, and
space between staff lines, return the point size to use. */

short GetTextSize(Boolean relFSize, short fontSize, DDIST lineSpace)
{
	short pointSize;
	
	if (relFSize)
		pointSize = RelIndexToSize(fontSize, lineSpace);
	else
		pointSize = fontSize;

	if (pointSize<MIN_TEXT_SIZE) pointSize = MIN_TEXT_SIZE;
	
	return pointSize;
}


/* ---------------------------------------------------------------- FontName2Index -- */
/* Get the fontname's index into our stored-to-system font number table; if the
fontname doesn't appear in the table, add it. In all cases, return the index of the
fontname. Exception: if the table overflows, return -1. */

short FontName2Index(Document *doc, unsigned char *fontName)
{
	short i, nfontsUsed;

	nfontsUsed = doc->nfontsUsed;

	/* If font is already in the table, just return its index. */
	
	for (i = 0; i<nfontsUsed; i++)
		if (PStrnCmp((StringPtr)fontName, 
						 (StringPtr)doc->fontTable[i].fontName, 32))
			return i;
			
	/* It's not in the table: if there's room, add it and return its index. */
	
	if (nfontsUsed>=MAX_SCOREFONTS) {
		LogPrintf(LOG_WARNING, "FontName2Index: limit of %d fonts exceeded.\n",
				  MAX_SCOREFONTS);
		return -1;
	}
	else {
		PStrnCopy((StringPtr)fontName, 
					(StringPtr)doc->fontTable[nfontsUsed].fontName, 32);
		GetFNum(fontName, (short *)&doc->fontTable[nfontsUsed].fontID);
		doc->nfontsUsed++;
		return nfontsUsed;
	}
}


/* --------------------------------------- User2HeaderFontNum,Header2UserFontNum -- */

short User2HeaderFontNum(Document */*doc*/, short styleChoice)
{
	switch (styleChoice) {
		case TSRegular1STYLE:
			return FONT_R1;
		case TSRegular2STYLE:
			return FONT_R2;
		case TSRegular3STYLE:
			return FONT_R3;
		case TSRegular4STYLE:
			return FONT_R4;
		case TSRegular5STYLE:
			return FONT_R5;
		case TSRegular6STYLE:
			return FONT_R6;
		case TSRegular7STYLE:
			return FONT_R7;
		case TSRegular8STYLE:
			return FONT_R8;
		case TSRegular9STYLE:
			return FONT_R9;
		case TSThisItemOnlySTYLE:
			return FONT_THISITEMONLY;
		default:
			MayErrMsg("User2HeaderFontNum: styleChoice %ld is illegal", (long)styleChoice);
			return FONT_THISITEMONLY;
	}
}

short Header2UserFontNum(short globalFontIndex)
{
	switch (globalFontIndex) {
		case FONT_R1:
			return TSRegular1STYLE;
		case FONT_R2:
			return TSRegular2STYLE;
		case FONT_R3:
			return TSRegular3STYLE;
		case FONT_R4:
			return TSRegular4STYLE;
		case FONT_R5:
			return TSRegular5STYLE;
		case FONT_R6:
			return TSRegular6STYLE;
		case FONT_R7:
			return TSRegular7STYLE;
		case FONT_R8:
			return TSRegular8STYLE;
		case FONT_R9:
			return TSRegular9STYLE;
		case FONT_THISITEMONLY:
			return TSThisItemOnlySTYLE;
		default:
			MayErrMsg("Header2UserFontNum: globalFontIndex %ld is illegal",
						(long)globalFontIndex);
			return TSThisItemOnlySTYLE;
	}
}


/* ---------------------------------------------- Convert to/from screen coords. -- */
/* Convert rect r from paper to window coordinates. */

void Rect2Window(Document *doc, Rect *r)
{
	OffsetRect(r, doc->currentPaper.left, doc->currentPaper.top);
}

/* Convert point <pt> from paper to window coordinates. */

void Pt2Window(Document *doc, Point *pt)
{
	pt->h += doc->currentPaper.left;
	pt->v += doc->currentPaper.top;
}

/* Convert point <pt> from window to paper coordinates. */

void Pt2Paper(Document *doc, Point *pt)
{
	pt->h -= doc->currentPaper.left;
	pt->v -= doc->currentPaper.top;
}

/* Convert in place global (screen coords) pt to paper-relative coordinates. */

void GlobalToPaper(Document *doc, Point *pt)
{
	GlobalToLocal(pt);
	pt->h -= doc->currentPaper.left;
	pt->v -= doc->currentPaper.top;
}

void RefreshScreen()
{
	Document *doc=GetDocumentFromWindow(TopDocument);

	if (doc) InvalWindow(doc);
}


/* ---------------------------------------------------- SleepMS, SleepTicks, etc. -- */
/* Find a loop count that'll give roughly a 1 millisec. delay. Of course we could
get more precise timing by using hardware timer V1, but it's used by Sound Manager,
MIDI Manager, and the MacTutor MIDI driver: for situations where we need shorter
intervals than TickCount provides (1/60 sec.) but don't need precise timing, a simple-
minded timing loop avoids any possible conflicts. For example, this might be used
for a delay time in an All Notes Off function to avoid overloading old synthesizers.
DO NOT USE IF TIMING NEEDS TO BE AT ALL ACCURATE!

The count in THIS function's loop should be large enough so we get something
meaningful on any (well, almost any) machine, but small enough so it doesn't take
too long on a slow one. This is not easy, since of course machines get faster all
the time. For now, we have a fixed loop count but adjust it down for very old
(slow) machines. A much better solution would be to try a small loop count (say
100,000); if that doesn't run long enough to get reasonable accuracy, try a larger
count (say 4 times as large); if that still doesn't run long enough, try a still
larger count, etc.

It would seem that, instead of looping a pre-determined number of times, we could
loop for a certain predefined amount of time; the trouble with this is the
potentially large overhead from checking the elapsed time!

For reference, compiling this with THINK C 5, an SE/30 (16MHz 68030) gets a bit
over 25,000 iterations per tick, giving oneMSDelayCount = about 1510. (NB: this
is about twice as fast as older comments here said: I assume the difference is
THINK C 5's improved code generation.) With THINK C 6, a PowerMac 7100/66 (in
emulation, of course) gets about 37,500 iterations per tick, so oneMSDelayCount =
about 2250.

(Actually, there's a solution to this problem that's probably much better: use the
Time Manager; but it'd also be much more complex. There's a much better and very
simple solution: compute <oneMSDelayCount> from low-memory global TimeDBRA=0x0D00.
Of course, both of these are much more machine-dependent. Yet another idea: do
this while the splash screen is up, so it would be zero overhead.) */
 
#define NORMAL_LOOP_COUNT 1500000L		/* For a "normal" (68020 or above) Mac */

void InitSleepMS()
{
	long startTicks, elapsedTicks, countPerTick, loopCount, l;

	loopCount = NORMAL_LOOP_COUNT;
	/* If this is an early, slow CPU, reduce <loopCount> so it doesn't take too long. */ 

	startTicks = TickCount();
	
	for (l = 0L; l<=loopCount; l++)			/* The compiler better not optimize this away! */
		;
	elapsedTicks = TickCount()-startTicks;

	if (elapsedTicks<1L) elapsedTicks = 1L;		/* In case this machine is REALLY fast! */
	countPerTick = loopCount/elapsedTicks;
	oneMSDelayCount = (60L*countPerTick)/1000L;	/* Convert "per tick" to "per ms." */
#ifndef PUBLIC_VERSION
	LogPrintf(LOG_NOTICE, "loopCount=%ld elapsedTicks=%ld oneMSDelayCount=%ld\n", loopCount,
						elapsedTicks, oneMSDelayCount);
#endif
}


/* Do nothing for <millisec> milliseconds, using a  simple delay loop. See above for
comments on why this exists. <oneMSDelayCount> MUST be initialized before calling this!

Note we don't take into account overhead for calling and returning from this
function, which might be significant on a slow computer if millisec=1 or 2. But,
as mentioned, this SHOULD NOT BE USED IF PRECISION IS NEEDED, anyway.*/

void SleepMS(long millisec)
{
	long delayCount, count;

	/*
	 * Precompute the loop limit, don't put it in the "for" statement. With THINK
	 * C 4 on an SE/30, putting this computation in the "for" makes each iteration
	 * about TEN TIMES slower!
	 */
	delayCount = millisec*oneMSDelayCount;

	/* The compiler had better not optimize this loop away! */

	for (count = 1L; count<delayCount; count++)
		;
}


/* Do nothing for <ticks> ticks; then, if the mouse button is down, wait til it's up. */

void SleepTicksWaitButton(unsigned long ticks)
{
	SleepTicks(ticks);
	while (Button())
		;
}

/* Do nothing for <ticks> ticks. Written because I got tired of debugging crashes
caused by my calling Delay and forgetting to put the "&" in front of the second
argument (which I never have any use for, anyway). */

void SleepTicks(unsigned long ticks)
{
	unsigned long aLong;
	
	Delay(ticks, &aLong);
}


/* --------------------------------------------------------------- NMIDIVersion -- */
/* Return MIDI Manager version numbers as a <long>. Why is this function useful?
MIDIVersion() and OMSVersion() both return <NumVersion>. The CodeWarrior header
indeed declares MIDIVersion that way, but THINK's declares it <unsigned long>;
this function is just to insulate calling routines from that ugliness. (And Opcode's
header declares OMSVersion as returning <long>. Yeech.) */

#ifdef TARGET_API_MAC_CARBON_MIDI

long NMIDIVersion()
{
	return -1L;
}

#else

long NMIDIVersion()
{
	long verNumMM;
	verNumMM = MIDIVersion();

	return verNumMM;
}

#endif

/* --------------------------------------------------------------- StdVerNumToStr -- */
/* Get String representation of given Version Number. See Mac Tech Note #189 for
details. */

char *StdVerNumToStr(long verNum, char *verStr)
{
	char	*retVal;
	char	majVer, minVer, verStage, verRev, bugFixVer = 0;
	
	if (verNum==0)
		retVal = NULL;
	else
	{
		majVer 		= (verNum & 0xFF000000) >> 24;
		minVer 		= (verNum & 0x00FF0000) >> 16;
		verStage 	= (verNum & 0x0000FF00) >> 8;
		verRev 		= (verNum & 0x000000FF) >> 0;
		bugFixVer	=  minVer & 0x0F;
		
		switch (verStage)
		{
			case 0x20:
				verStage = 'd';
				break;
			case 0x40:
				verStage = 'a';
				break;
			case 0x60:
				verStage = 'b';
				break;
			case 0x80:
				verStage = '\0';
				break;
			default:
				verStage = '?';
				break;
		}
		
	/* Apple's version does this...
	 *		if (bugFixVer==0)
	 *			sprintf(verStr,"%X.%X%c%X", majVer, minVer>>4, verStage, verRev);
	 *		else
	 *			sprintf(verStr,"%X.%X.%X%c%X", majVer, minVer>>4, minVer & 0x0F, verStage,
	 *						verRev);
	 *	but (at least for MIDI Manager) verStage and verRev are less than helpful, so we
	 *	omit them.
	 */

		if (bugFixVer==0)
			sprintf(verStr,"%X.%X", majVer, minVer>>4);
		else
			sprintf(verStr,"%X.%X.%X", majVer, minVer>>4, minVer & 0x0F);
		
		retVal = verStr;
	}
		
	return retVal;
}


/* ---------------------------------------------------------------- PlayResource -- */


/* Play a 'snd ' resource, as found in the given handle.  If sync is TRUE, play it
synchronously here and return when it's done.  If sync is FALSE, start the sound
playing and return before it's finished.  Deliver any error.  To stop a currently
playing sound, call this with snd == NULL.

Disposing of the sound handle is the caller's responsibility.  If you call this
with sync==FALSE, then at some point in the future, it is the caller's responsibility
to call again with snd==NULL, to unlock the locked sound.  This probably ought to
be done in a completion routine somewhere. */

short PlayResource(Handle snd, Boolean sync)
	{
		short err = noErr;
		static SndChannel *theChanPtr;			/* Channel storage must come from heap */
		static Handle theSound;					/* Initially NULL */
		
		/* First turn any current sound off if we're called for any reason */
		
		if (theSound) {
			err = SndDisposeChannel(theChanPtr,TRUE);
			HUnlock(theSound);
			theSound = NULL;
			}
		if (snd==NULL || err!=noErr) return(err);
		
		MoveHHi(snd); HLock(snd);							/* Get data out of the way (may not be necessary) */

		if (sync) {
			err = SndPlay(NULL,(SndListHandle)snd,FALSE);	/* Doesn't return until sound is finished */
			HUnlock(snd);
			}
		 else {
		 	/*
		 	 *	Fill in our channel record with new channel, no synths, no init, no callback
		 	 */
		 	theChanPtr = (SndChannel *)NewPtr(sizeof(SndChannel));
			if (!(err = SndNewChannel(&theChanPtr,0,0L,NULL))) {
			
				err = SndPlay(theChanPtr,(SndListHandle)snd,TRUE);	/* Channel initialized: start it up */
				
				/* Continues playing after we return: we'll unlock sound when channel is disposed */
				
				if (err) HUnlock(snd);							/* Unless there was an error */
				 else	 theSound = snd;
				}
			}
		return(err);
	}
