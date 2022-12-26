/******************************************************************************************
*	FILE:	InternalInput.c (formerly called FileInput.c)
*	PROJ:	Nightingale
*	DESC:	Routines for creating Nightingale objects via notelist file, AppleScript,
*			etc., rather than via Nightingale's user interface. Written by John Gibson.
*			See also NUIxxx() ("non-UI") functions. (Perhaps those should be here?)
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


static Boolean IICombineInto1KeySig(Document *doc, LINK firstL, short numSubObjs, LINK aSourceKS[]);
static Boolean GetTextStyleRec(Document *doc, short textStyle, PTEXTSTYLE pStyleRec);
static LINK InsertAndInitGraphic(Document *doc, LINK insertBeforeL, short graphicType);
static short IIGetFontIndex(Document *doc, unsigned char *fontName);
static LINK AnchorSearch(Document *doc, LINK dependentL);
static short GuessVoiceRole(Document *doc, short iVoice, LINK startL, LINK endL);


/* -------------------------------------------------------------------- IICreateTUPLET -- */
/* Creates a tuplet object with the specified number of subobjects and other
characteristics. Insert it *before* <beforeL>. NOTE: Does *not* set tpSync fields
in the tuplet. Call IIFixTupletLinks for that after creating all the relevant syncs.
Assumes that the first Sync of the tuplet has just been created.

CAUTION: <beforeL> normally should be the first Sync of the tuplet, but it could be
doc->tailL if the caller inserts items into the data structure sequentially -- from
left to right -- and is sure that the next item inserted will be the first Sync of
the tuplet.

After calling IIFixTupletLinks, call SetTupletYPos ??AFTER beaming!. (IICreateTUPLET
was adapted from CreateTUPLET in Tuplet.c.) */

LINK IICreateTUPLET(Document	*doc,
					  short		voice,
					  short		staffn,
					  LINK		beforeL,
					  short		nInTuple,
					  short		tupleNum,
					  short		tupleDenom,
					  Boolean	numVis,
					  Boolean	denomVis,
					  Boolean	brackVis)
{
	LINK	tupletL;

	if (nInTuple<2) {
		MayErrMsg("IICreateTUPLET: can't make tuplet with only %ld note(s).", (long)nInTuple);
		return NILINK;
	}

	tupletL = InsertNode(doc, beforeL, TUPLETtype, nInTuple);
	if (!tupletL) {
		NoMoreMemory();
		return NILINK;
	}
	
	SetObject(tupletL, 0, 0, False, True, False);
	InitTuplet(tupletL, staffn, voice, tupleNum, tupleDenom, numVis, denomVis, brackVis);

	return tupletL;
}


/* ------------------------------------------------------------------ IIFixTupletLinks -- */
/* In the range [startL,endL), fix the tpSyncs of every Tuplet to agree with the order
of objects in the data structure, i.e., set the first tpSync to the next Sync in its
voice, the next tpSync to the next Sync after that, etc. In addition:
   1) set the inTuplet flag of every note in each of these syncs to True, and
   2) set the performance durations of these notes to their logical duration modified
      by config.legatoPct.
(IIFixTupletLinks was adapted from FixTupletLinks in MIDIRecUtils.c.) */

void IIFixTupletLinks(Document *doc, LINK startL, LINK endL, short voice)
{
	short		i, nInTuplet;
	long		lDur;
	LINK		pL, syncL, aNoteTupleL, aNoteL;
	PANOTETUPLE	aNoteTuple;
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (TupletTYPE(pL) && TupletVOICE(pL)==voice) {
			nInTuplet = LinkNENTRIES(pL);
			aNoteTupleL = FirstSubLINK(pL);
			syncL = pL;
			for (i = 0; i<nInTuplet; i++, aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
				syncL = LVSearch(RightLINK(syncL), SYNCtype, voice, GO_RIGHT, False);
				aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
				aNoteTuple->tpSync = syncL;
				for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					if (NoteVOICE(aNoteL)==voice) {
						NoteINTUPLET(aNoteL) = True;
						lDur = CalcNoteLDur(doc, aNoteL, syncL);
						NotePLAYDUR(aNoteL) = (config.legatoPct*lDur)/100L;
					}
				}
			}
		}
	}
}


/* ------------------------------------------------------------------- IIReplaceKeySig -- */
/* Replace key signature on the given staff with one specified by <sharpsOrFlats>.
CAUTION: In contrast to ReplaceKeySig, IIReplaceKeySig works with keysigs that are
not in the reserved area; that is, it works with keysigs that are inMeasure. Assumes
that there are no notes following this keysig on this staff, so there's no need to fix
up following accidentals. (Adapted from ReplaceKeySig in InsNew.c.) */

Boolean IIReplaceKeySig(Document *doc, LINK keySigL, short staffn, short sharpsOrFlats)
{
	PAKEYSIG	aKeySig;
	KSINFO		oldKSInfo, newKSInfo;
	LINK		endL, aKeySigL;
	
	if (KeySigINMEAS(keySigL)==False) return False;				/* Can't handle this yet */

	endL = keySigL;
	aKeySigL = FirstSubLINK(keySigL);
	for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
		if (KeySigSTAFF(aKeySigL)==staffn) {
			if (sharpsOrFlats) KeySigVIS(aKeySigL) = True;

			aKeySig = GetPAKEYSIG(aKeySigL);
			KEYSIG_COPY((PKSINFO)aKeySig->KSItem, &oldKSInfo);	/* Copy old keysig. info */
			InitKeySig(aKeySigL, KeySigSTAFF(aKeySigL), 0, sharpsOrFlats);
			SetupKeySig(aKeySigL, sharpsOrFlats);
			
			aKeySig = GetPAKEYSIG(aKeySigL);
			KEYSIG_COPY((PKSINFO)aKeySig->KSItem, &newKSInfo);	/* Copy new keysig. info */
			
			FixContextForKeySig(doc, RightLINK(keySigL), 
										KeySigSTAFF(aKeySigL), oldKSInfo, newKSInfo);
		}
	}

	return True;
}


/* ------------------------------------------------------------- IIInsertWholeMeasRest -- */
/* Insert a whole-measure rest before <insertBeforeL> on the given staff in the given
(internal) voice. Does NOT attempt to create a Sync with notes on other staves. Returns
True if OK, False if error. */

Boolean IIInsertWholeMeasRest(Document *doc, LINK insertBeforeL, short staffn,
										short iVoice, Boolean visible)
{
	LINK	syncL, aNoteL;
	PANOTE	aNote;

	syncL = InsertNode(doc, insertBeforeL, SYNCtype, 1);
	if (syncL==NILINK) {
		NoMoreMemory();
		return False;
	}
	
	SetObject(syncL, 0, 0, True, True, False);
	LinkTWEAKED(syncL) = False;
	aNoteL = FirstSubLINK(syncL);
	SetupNote(doc, syncL, aNoteL, staffn, -1/*halfLn*/, WHOLEMR_L_DUR, 0, iVoice,
				True, 0, 0);
	aNote = GetPANOTE(aNoteL);
	aNote->visible = visible;
	
	return True;
}


/* ------------------------------------------------------------------- IIInsertBarline -- */
/* Insert a barline of type <barlineType> before <insertBeforeL> on all staves. Returns
the link of this barline if OK, NILINK if error. */

LINK IIInsertBarline(Document *doc, LINK insertBeforeL, short barlineType)
{
	short	sym;
	LINK	measL;
	CONTEXT	context;
	
	NewObjInit(doc, MEASUREtype, &sym, singleBarInChar, ANYONE, &context);
	measL = CreateMeasure(doc, insertBeforeL, -1, sym, context);
	if (barlineType!=BAR_SINGLE) SetMeasureSubType(measL, barlineType);

	return measL;
}


/* ----------------------------------------------------------------- SetMeasureSubType -- */
/* Set all subobjects of the given measure to <subType>, a code indicating the type of
barline (e.g., single bar, double bar, end bar, etc.) Returns True if okay, False if
error. */

Boolean SetMeasureSubType(LINK measL, short subType)
{
	LINK	aMeasL;

	if (measL==NILINK || subType<BAR_SINGLE || subType>BAR_LAST)	/* subType out of range */
		return False;

	for (aMeasL = FirstSubLINK(measL); aMeasL; aMeasL = NextMEASUREL(aMeasL))
		MeasSUBTYPE(aMeasL) = subType;
	
	return True;
}


/* ---------------------------------------------------------------------- IIInsertClef -- */
/* Insert a clef of the given type and size before <insertBeforeL> on the given staff.
Caller should then call FixContextForClef to propagate the effects of the clef change
to any following objects.
NB: Do not use this function to change a clef in the reserved area. (For that purpose,
insert a clef change at the end of the previous system.) IIInsertClef only creates
clefs that are inMeasure.
Returns the link of this clef if OK, NILINK if error. */

LINK IIInsertClef(Document *doc, short staffn, LINK insertBeforeL, short clefType,
					Boolean small)
{
	LINK	clefL, aClefL;
	PACLEF	aClef;
	
	clefL = InsertNode(doc, insertBeforeL, CLEFtype, 1);
	if (clefL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}	
	ClefINMEAS(clefL) = True;
	aClefL = FirstSubLINK(clefL);
	InitClef(aClefL, staffn, 0, clefType);
	SetObject(clefL, 0, 0, False, True, False);
	aClef = GetPACLEF(aClefL);
	aClef->small = small;
	
	return clefL;
}


/* ------------------------------------------------------------------- IIInsertTimeSig -- */
/*	Caller may need to call FixContextForTimeSig and/or FixTimeStamps after calling
this function. */
	
LINK IIInsertTimeSig(Document *doc,
						short staff,					/* or ANYONE for all staves */
						LINK insertBeforeL,
						short type,
						short numerator,
						short denominator)
{
	short		stfCount, i, useStaff;
	LINK		timeSigL, aTimeSigL;
	PTIMESIG	timeSig;
	PATIMESIG	aTimeSig;
	
	stfCount = (staff==ANYONE? doc->nstaves : 1);

	timeSigL = InsertNode(doc, insertBeforeL, TIMESIGtype, stfCount);
	if (timeSigL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}
	
	timeSig = GetPTIMESIG(timeSigL);
	InitObject(timeSigL, timeSig->left, timeSig->right, 0, 0, False, True, False);	
	TimeSigINMEAS(timeSigL) = True;

	aTimeSigL = FirstSubLINK(timeSigL);		
	for (i = 1; i<=stfCount; i++, aTimeSigL = NextTIMESIGL(aTimeSigL)) {
		useStaff = (staff==ANYONE ? i : staff);
		InitTimeSig(aTimeSigL, useStaff, 0, type, numerator, denominator);
		aTimeSig = GetPATIMESIG(aTimeSigL);
		aTimeSig->soft = aTimeSig->selected = False;
	}

	return timeSigL;
}


/* -------------------------------------------------------------------- IIInsertKeySig -- */
/* Caller may need to call FixContextForKeySig and/or FixAccsForKeySig after calling
this function. */
	
LINK IIInsertKeySig(Document *doc,
					  short staff,					/* or ANYONE for all staves */
					  LINK insertBeforeL,
					  short sharpsOrFlats)			/* <0: flats; >0: sharps, 0: cancel prev ks */
{
	short		stfCount, i, useStaff;
	LINK		keySigL, aKeySigL;
	PKEYSIG		keySig;
	PAKEYSIG	aKeySig;
	
	stfCount = (staff==ANYONE? doc->nstaves : 1);

	keySigL = InsertNode(doc, insertBeforeL, KEYSIGtype, stfCount);
	if (keySigL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}
	
	keySig = GetPKEYSIG(keySigL);
	InitObject(keySigL, keySig->left, keySig->right, 0, 0, False, True, False);	
	KeySigINMEAS(keySigL) = True;

	aKeySigL = FirstSubLINK(keySigL);
	for (i = 1; i<=stfCount; i++, aKeySigL = NextKEYSIGL(aKeySigL)) {
		useStaff = (staff==ANYONE ? i : staff);
		InitKeySig(aKeySigL, useStaff, 0, sharpsOrFlats);
		SetupKeySig(aKeySigL, sharpsOrFlats);
		if (sharpsOrFlats==0) {
			CONTEXT context;
			GetContext(doc, LeftLINK(keySigL), useStaff, &context);
			aKeySig = GetPAKEYSIG(aKeySigL);
			aKeySig->subType = context.nKSItems;		/* number of cancelling naturals */
			aKeySig->visible = True;					/* InitKeySig should have set this to False */
		}
		aKeySig = GetPAKEYSIG(aKeySigL);
		aKeySig->soft = aKeySig->selected = False;
	}

	return keySigL;
}


/* ------------------------------------------------------------------ IICombine1KeySig -- */
/* Helper function for IICombineKeySigs */

static Boolean IICombineInto1KeySig(Document *doc,
									LINK firstL,
									short numSubObjs,
									LINK aSourceKS[])
{
	short		s;
	PAKEYSIG	srcKeySig, dstKeySig;
	LINK		secondKSsubL, dstKSsubL, srcKSsubL;
	DDIST		xd;
	
	if (!ExpandNode(firstL, &secondKSsubL, numSubObjs-1)) {
		NoMoreMemory();
		return False;
	}
	xd = KeySigXD(FirstSubLINK(firstL));
	
	dstKSsubL = secondKSsubL;
	for (s = 1; s<=doc->nstaves; s++)
		if (aSourceKS[s] && aSourceKS[s]!=firstL) {
			srcKSsubL = FirstSubLINK(aSourceKS[s]);
			srcKeySig = GetPAKEYSIG(srcKSsubL);
			InitKeySig(dstKSsubL, s, xd, srcKeySig->nKSItems);
			srcKeySig = GetPAKEYSIG(srcKSsubL);
			dstKeySig = GetPAKEYSIG(dstKSsubL);
			KEYSIG_COPY((PKSINFO)srcKeySig->KSItem, (PKSINFO)dstKeySig->KSItem);
			dstKeySig->subType = srcKeySig->subType;
			if (dstKeySig->nKSItems==0 && dstKeySig->subType)
				dstKeySig->visible = True;						/* show cancelling naturals ks */
			dstKSsubL = NextKEYSIGL(dstKSsubL);			
			DeleteNode(doc, aSourceKS[s]);
		}
	return True;
}


/* ------------------------------------------------------------------ IICombineKeySigs -- */
/*	Within the range (startL, endL], combine adjacent keysigs objects into a single
object having multiple subobjects. The series of keysigs so combined must satisfy
these conditions:
	1) each must have exactly one subobject,
	2) each must have a unique staff number within the series, and
	3) the series must be strictly continuous; that is, no object of
		another type may fall between any two members of the series.
Return True if OK, False if out of memory (after alerting user).

CAVEAT: Currently this function ignores any Graphics attached to keysigs. If it deletes
a keysig with an attached Graphic, the Graphic is orphaned. */

Boolean IICombineKeySigs(Document *doc,
						 LINK startL,
						 LINK endL)
{
	short	s, staffn, numSubObjs=0;
	Boolean	inSeries = False;
	LINK	pL, firstL, aSourceKS[MAXSTAVES+1];

	firstL = NILINK;
	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (KeySigTYPE(pL)) {
			if (LinkNENTRIES(pL)==1) {
				if (!inSeries) {
					inSeries = True;
					firstL = pL;
					for (s = 1; s<=doc->nstaves; s++)
						aSourceKS[s] = NILINK;
				}
				staffn = KeySigSTAFF(FirstSubLINK(pL));
				if (aSourceKS[staffn]) {
					/* We already have a keysig on this staff, so terminate series. */
					
					inSeries = False;
					pL = LeftLINK(pL);			/* So we'll catch this ks again in next iteration. (It might begin a new ks series.) */
				}
				else {
					aSourceKS[staffn] = pL;
					numSubObjs++;
				}
			}
			else
				inSeries = False;
		}
		else
			inSeries = False;
		
		if (!inSeries && firstL) {
			if (!IICombineInto1KeySig(doc, firstL, numSubObjs, aSourceKS))
				return False;
			firstL = NILINK;
		}	
	}
	
	/* Handle case where score has: ksL - ksL - tailL */
	
	if (firstL && numSubObjs>1)
		if (!IICombineInto1KeySig(doc, firstL, numSubObjs, aSourceKS))
			return False;
	return True;
}


/* ------------------------------------------------------------------- IIInsertDynamic -- */
/*	Insert a (non-hairpin) dynamic of the given type before <insertBeforeL> on the
 given staff. Set the dynamic's firstSyncL to <anchorL>, which can be NILINK if the
 caller prefers to set this later. Caller next should call FixContextForDynamic to
 propagate the effects of the dynamic change to any following objects. Caller also must
 set coordinates, because we just set them to 0 here. Returns the link of this dynamic
 if OK, NILINK if error. */
	
LINK IIInsertDynamic(Document *doc, short staffn, LINK insertBeforeL, LINK anchorL,
						short dynamicType)
{
	LINK		dynamicL;
	PDYNAMIC	dynamic;
	PADYNAMIC	aDynamic;
	
	dynamicL = InsertNode(doc, insertBeforeL, DYNAMtype, 1);
	if (dynamicL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}
	
	/* Init object */
	dynamic = GetPDYNAMIC(dynamicL);
	InitObject(dynamicL, dynamic->left, dynamic->right, 0, 0, False, True, False);
	dynamic = GetPDYNAMIC(dynamicL);
	dynamic->dynamicType = dynamicType;
	dynamic->filler = 0;
	dynamic->crossSys = False;
	dynamic->firstSyncL = anchorL;
	dynamic->lastSyncL = NILINK;
	
	/* Init subobject */
	aDynamic = GetPADYNAMIC(FirstSubLINK(dynamicL));
	aDynamic->staffn = staffn;
	aDynamic->subType = 0;										/* Unused; obj has the type */
	aDynamic->selected = False;
	aDynamic->visible = True;
	aDynamic->soft = False;
	aDynamic->mouthWidth = aDynamic->otherWidth = 0;
	aDynamic->small = 0;
	aDynamic->xd = aDynamic->endxd = 0;
	aDynamic->yd = aDynamic->endyd = 0;

	return dynamicL;
}


/* ------------------------------------------------------------------- GetTextStyleRec -- */
/*	Copy the data from the specified text style (stored in document's SCOREHEADER)
into the supplied TEXTSTYLE record. Return True if OK, False if <textStyle> is out of
range. */
	
static Boolean GetTextStyleRec(Document *doc, short textStyle, PTEXTSTYLE pStyleRec)
{
	char	*address;
	
	switch (textStyle) {
		case TSRegular1STYLE:	address = (char *) doc->fontName1;	break;
		case TSRegular2STYLE:	address = (char *) doc->fontName2;	break;
		case TSRegular3STYLE:	address = (char *) doc->fontName3;	break;
		case TSRegular4STYLE:	address = (char *) doc->fontName4;	break;
		case TSRegular5STYLE:	address = (char *) doc->fontName5;	break;
		default:				return False;
	}
	
	BlockMove(address, pStyleRec, sizeof(TEXTSTYLE));

	return True;
}


/* -------------------------------------------------------------- InsertAndInitGraphic -- */
/*	Insert a graphic of the given type before <insertBeforeL> and initialize all
object and subobject fields. Returns the link of this graphic if OK, NILINK if error. */
	
static LINK InsertAndInitGraphic(Document *doc, LINK insertBeforeL, short graphicType)
{
	LINK		graphicL, aGraphicL;
	PGRAPHIC	pGraphic;
	
	graphicL = InsertNode(doc, insertBeforeL, GRAPHICtype, 1);
	if (graphicL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}

	pGraphic = GetPGRAPHIC(graphicL);
	InitObject(graphicL, pGraphic->left, pGraphic->right, 0, 0, False, True, False);

	pGraphic = GetPGRAPHIC(graphicL);
	pGraphic->staffn = NOONE;
	pGraphic->graphicType = graphicType;
	pGraphic->voice = NOONE;
	pGraphic->enclosure = ENCL_NONE;
	pGraphic->justify = GRJustLeft;
	pGraphic->vConstrain = pGraphic->hConstrain = False;
	pGraphic->multiLine = 0;
	pGraphic->info = 0;
	pGraphic->gu.handle = NULL;
	pGraphic->fontInd = 0;
	pGraphic->relFSize = 0;
	pGraphic->fontSize = 0;
	pGraphic->fontStyle = 0;
	pGraphic->info2 = 0;
	pGraphic->firstObj = NILINK;
	pGraphic->lastObj = NILINK;

	aGraphicL = FirstSubLINK(graphicL);
	GraphicSTRING(aGraphicL) = 0;
	
	return graphicL;
}


/* Like FontName2Index, except if the font table overflows, it gives an error message
and returns the last legal font index. Calling this repeatedly could overwhelm the user
with repeated complaints: it should be used only if the font table overflowing by more
than a few places is very unlikely. */

static short IIGetFontIndex(Document *doc, unsigned char *fontName)
{
	short fontInd;
	
	fontInd = FontName2Index(doc, fontName);
	if (fontInd<0) {
		GetIndCString(strBuf, MISCERRS_STRS, 20);    /* "Will use most recently added font." */
		CParamText(strBuf, "", "", "");
		CautionInform(MANYFONTS_ALRT);
		fontInd = MAX_SCOREFONTS-1;
	}
	
	return fontInd;
}


/* ------------------------------------------------------------------ IIInsertGRString -- */
/*	Insert a graphic of type GRString, or GRLyric if <isLyric>, before <insertBeforeL>.
Set the graphic's firstObj to <anchorL>, which can be NILINK if the caller prefers
to set this later. The caller has the option of specifying one of the five
pre-configured text styles (TSRegularXSTYLE, where 'X' is 1-5) in <textStyle> or
supplying the required text characteristics in <pStyleRec>. If choosing the first
approach, pass NULL for <pStyleRec>; if choosing the second, pass TSNoSTYLE for
<textStyle>. (NB: <isLyric> overrides pStyleRec->lyric.) Returns the link of this
Graphic if OK, NILINK if error. */

LINK IIInsertGRString(Document *doc,
						 short staffn,
						 short iVoice,				/* internal voice number */
						 LINK insertBeforeL,
						 LINK anchorL,
						 Boolean isLyric,
						 short textStyle,			/* Ngale "text style" choice, e.g., TSRegular1STYLE */
						 PTEXTSTYLE	pStyleRec,
						 char string[])				/* C string */
{
	LINK			graphicL, aGraphicL;
	PGRAPHIC		pGraphic;
	STRINGOFFSET	strOffset;
	TEXTSTYLE		styleRec;
	Boolean			result;
	
	graphicL = InsertAndInitGraphic(doc, insertBeforeL, (isLyric? GRLyric : GRString));
	if (graphicL==NILINK) return NILINK;
	
PushLock(OBJheap);
	pGraphic = GetPGRAPHIC(graphicL);
	pGraphic->staffn = staffn;
	pGraphic->voice = iVoice;
	
	if (textStyle==TSNoSTYLE) {
		if (pStyleRec==NULL) {
			MayErrMsg("IIInsertGRString: missing text style data. Using style 1.");
			textStyle = TSRegular1STYLE;
			goto getStyle;
		}
		else {
			pGraphic->fontInd = IIGetFontIndex(doc, pStyleRec->fontName);
			pGraphic->enclosure = pStyleRec->enclosure;
			pGraphic->relFSize = pStyleRec->relFSize;
			pGraphic->fontSize = pStyleRec->fontSize;
			pGraphic->fontStyle = pStyleRec->fontStyle;
		}
	}
	else {
getStyle:
		result = GetTextStyleRec(doc, textStyle, &styleRec);
		if (!result) {
			MayErrMsg("IIInsertGRString: bad textStyle. Using style 1 instead.");
			textStyle = TSRegular1STYLE;
			GetTextStyleRec(doc, textStyle, &styleRec);
		}
		pGraphic->fontInd = IIGetFontIndex(doc, styleRec.fontName);
		pGraphic->enclosure = styleRec.enclosure;
		pGraphic->relFSize = styleRec.relFSize;
		pGraphic->fontSize = styleRec.fontSize;
		pGraphic->fontStyle = styleRec.fontStyle;
	}
	pGraphic->info = UI2InternalStyle(doc, textStyle);
	
	pGraphic->firstObj = anchorL;
PopLock(OBJheap);
	
	CToPString(string);
	strOffset = PStore((unsigned char *)string);
	if (strOffset<0L) {
		NoMoreMemory();
		return NILINK;
	}
	else if (strOffset>GetHandleSize((Handle)doc->stringPool)) {
		MayErrMsg("IIInsertGRString: PStore error. string=%ld", strOffset);
		return NILINK;
	}
	aGraphicL = FirstSubLINK(graphicL);
	GraphicSTRING(aGraphicL) = strOffset;

	return graphicL;
}


/* --------------------------------------------------------------------- IIInsertTempo -- */
/*	Insert a tempo mark before <insertBeforeL> on the given staff. Set the tempo's
firstSyncL to <anchorL>, which can be NILINK if the caller prefers to set this later.
Caller must set coordinates, because we just set them to 0 here. Returns the link of
this tempo mark if OK, NILINK if error. */

LINK IIInsertTempo(Document	*doc,
					 short		staffn,
					 LINK		insertBeforeL,
					 LINK		anchorL,
					 char		durCode,
					 Boolean	dotted,
					 Boolean	hideMM,
					 char		tempoStr[],			/* <tempoStr> and <metroStr> are C strings */
					 char		metroStr[])
{
	LINK	tempoL;
	PTEMPO	pTempo;
	long	beatsPM;
	
	tempoL = InsertNode(doc, insertBeforeL, TEMPOtype, 0);
	if (tempoL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}
	
PushLock(OBJheap);
	pTempo = GetPTEMPO(tempoL);
	InitObject(tempoL, pTempo->left, pTempo->right, 0, 0, False, True, False);
	pTempo->staffn = staffn;
	pTempo->subType = durCode;
	pTempo->dotted = dotted;
	pTempo->noMM = False;
	pTempo->filler = 0;
	pTempo->hideMM = hideMM;
	pTempo->firstObjL = anchorL;
	
	CToPString(tempoStr);
	pTempo->strOffset = PStore((unsigned char *)tempoStr);

	CToPString(metroStr);	
	pTempo->metroStrOffset = PStore((unsigned char *)metroStr);
	
	if (pTempo->strOffset<0L || pTempo->metroStrOffset<0L) {
		NoMoreMemory();
		tempoL = NILINK;
	}
	else if (pTempo->strOffset>GetHandleSize((Handle)doc->stringPool)
		  || pTempo->metroStrOffset>GetHandleSize((Handle)doc->stringPool)) {
		MayErrMsg("IIInsertTempo: PStore error. strOffset=%ld metroStrOffset=%ld",
					pTempo->strOffset, pTempo->metroStrOffset);
		tempoL = NILINK;
	}
	beatsPM = FindIntInString((unsigned char *)metroStr);
	if (beatsPM<0L) beatsPM = config.defaultTempoMM;
	pTempo->tempoMM = beatsPM;
PopLock(OBJheap);
	
	return tempoL;
}


/* ---------------------------------------------------------------------- IIInsertSync -- */
/* Insert a Sync containing the given number of subobjects before <insertBeforeL>.
Returns the link of this Sync if OK, NILINK if error. */

LINK IIInsertSync(Document *doc, LINK insertBeforeL, short numSubobjects)
{
	LINK	syncL;
	
	syncL = InsertNode(doc, insertBeforeL, SYNCtype, numSubobjects);
	if (syncL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}

	SetObject(syncL, 0, 0, True, True, False);
	LinkTWEAKED(syncL) = False;
	
	return syncL;
}


/* -------------------------------------------------------------------- IIInsertGRSync -- */
/* Insert a GRSync containing the given number of subobjects before <insertBeforeL>.
Returns the link of this GRSync if OK, NILINK if error. */

LINK IIInsertGRSync(Document *doc, LINK insertBeforeL, short numSubobjects)
{
	LINK	GRSyncL;
	
	GRSyncL = InsertNode(doc, insertBeforeL, GRSYNCtype, numSubobjects);
	if (GRSyncL==NILINK) {
		NoMoreMemory();
		return NILINK;
	}

	SetObject(GRSyncL, 0, 0, True, True, False);
	LinkTWEAKED(GRSyncL) = False;
	
	return GRSyncL;
}


/* ------------------------------------------------------------------- IIAddNoteToSync -- */
/* Add a note to an existing Sync or GRSync. The Sync doesn't need to have any subobjects
when this is called. The caller will have to initialize nearly everything about the new
note on return, by calling SetupNote or SetupGRNote. All this function does is link a new
node into the syncs's subobject list. Returns the note LINK if everything is OK, NILINK
if error. */

LINK IIAddNoteToSync(Document */*doc*/, LINK syncL)
{
	LINK	aNoteL;
	
	if (!ExpandNode(syncL, &aNoteL, 1)) {
		NoMoreMemory();
		return NILINK;
	}
	return aNoteL;
}


/* ---------------------------------------------------------------------- IIInsertSlur -- */
/* Insert a slur just before <firstSyncL>, and init it appropriately. Both
<firstSyncL> and <lastSyncL> must be Syncs (not GRSyncs!) in the same system;
<firstNoteL> and <lastNoteL> must be in the same voice. If <setShapeNow>, set the
control points of the slur to a  default shape; otherwise leave the shape alone. (In
the latter case, the caller probably wants to reshape all slurs at once later on,
using IIReshapeSlursTies.) We give up and return NILINK if:
	<firstSyncL> and <lastSyncL> are not both Syncs,
	<firstSyncL> and <lastSyncL> are in different systems,
	<firstNoteL> and <lastNoteL> are in different voices, or
	there isn't enough memory to create a new slur. */

LINK IIInsertSlur(Document *doc,
					LINK firstSyncL,
					LINK firstNoteL,
					LINK lastSyncL,
					LINK lastNoteL,
					Boolean	setShapeNow)
{
	short	staffn1, staffn2, iVoice;
	LINK	slurL, aSlurL;
	PSLUR	pSlur;
	PASLUR	aSlur;
	PANOTE	aNote;
	
	/* Give up if either Sync is not really a Sync. */
	
	if (!SyncTYPE(firstSyncL) || !SyncTYPE(lastSyncL)) return NILINK;
	
	/* Give up if asked to create a cross-system tie. */
	
	if (!SameSystem(firstSyncL, lastSyncL)) return NILINK;
	
	/* Give up if the given notes are not in the same voice. */
	
	if (NoteVOICE(firstNoteL)!=NoteVOICE(lastNoteL)) return NILINK;
	
	slurL = InsertNode(doc, firstSyncL, SLURtype, 1);
	if (!slurL) {
		NoMoreMemory();
		return NILINK;
	}

	/* Initialize object */

	pSlur = GetPSLUR(slurL);
	InitObject(slurL, pSlur->left, pSlur->right, 0, 0, False, True, False);	
	
	aNote = GetPANOTE(firstNoteL);
	staffn1 = aNote->staffn;
	iVoice = aNote->voice;
	staffn2 = NoteSTAFF(lastNoteL);
	
	pSlur = GetPSLUR(slurL);
	pSlur->staffn = staffn1;
	pSlur->voice = iVoice;
	pSlur->philler = 0;
 	pSlur->crossStaff = (staffn1!=staffn2);
 	pSlur->crossStfBack = (pSlur->crossStaff && staffn1>staffn2);	// ??What if staffn1 is lower num but also lower in score pos?
 	pSlur->crossSystem = False;
	pSlur->tempFlag = 0;
	pSlur->used = False;
	pSlur->tie = False;
 	pSlur->firstSyncL = firstSyncL;
 	pSlur->lastSyncL = lastSyncL;

	/* Initialize subobject. (Assumes there is only one.) */

	aSlurL = FirstSubLINK(slurL);
	aSlur = GetPASLUR(aSlurL);
	aSlur->selected = False;
	aSlur->visible = True;
	aSlur->soft = False;
	aSlur->dashed = False;								/* Caller can override this. */
	aSlur->filler = 0;
	SetRect(&aSlur->bounds, 0, 0, 0, 0);
	aSlur->firstInd = 0;
	aSlur->lastInd = 0;
	aSlur->reserved = 0L;
	aSlur->seg.knot.h = aSlur->seg.knot.v = 0;
	aSlur->seg.c0.h = aSlur->seg.c0.v = 0;
	aSlur->seg.c1.h = aSlur->seg.c1.v = 0;
	aSlur->startPt.h = aSlur->startPt.v = 0;
	aSlur->endPt.h = aSlur->endPt.v = 0;
	aSlur->endKnot.h = aSlur->endKnot.v = 0;

	if (setShapeNow) SetAllSlursShape(doc, slurL, True);

	return slurL;
}


/* ------------------------------------------------------------------ IICreateAllSlurs -- */
/*	Examines the slurredL and slurredR flags of all notes in the score and creates
slurs accordingly. Does not set slur shapes; the caller should call IIReshapeSlursTies
later. It will refuse to create a cross-system slur. It does not alert the user to
this (or any other) kind of error, but returns the number of errors encountered. If
there are errors, the caller should clean up the slurredL and slurredR flags by
calling IIFixAllNoteSlurFlags. CAVEAT: It will create a nested slur if the slurredL
and slurredR flags are set that way, even though Nightingale doesn't allow nested
slurs. */

short IICreateAllSlurs(Document *doc)
{
	LINK	pL, qL, prevL, slurL, aNoteL, bNoteL;
	PANOTE	bNote;
	short	voice, errCnt = 0;
	Boolean	found;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSLURREDR(aNoteL)) {
					voice = NoteVOICE(aNoteL);
					found = False;
					for (prevL = pL; ; prevL = qL) {
						qL = LVSearch(RightLINK(prevL), SYNCtype, voice, GO_RIGHT, False);
						if (qL) {
							bNoteL = FirstSubLINK(qL);
							for ( ; bNoteL; bNoteL = NextNOTEL(bNoteL)) {
								bNote = GetPANOTE(bNoteL);
								if (bNote->voice==voice && bNote->slurredL) {
									slurL = IIInsertSlur(doc, pL, aNoteL, qL, bNoteL, False);
									if (!slurL) errCnt++;		/* probably cross-system */
									found = True;
								}
							}
							if (found) break;
						}
						else {
							errCnt++;				/* Couldn't find a subsequent slurredL note in same voice */
							break;
						}
					}
				}
		}

	return errCnt;
}


/* ---------------------------------------------------------------- IIReshapeSlursTies -- */
/* Set all slurs and ties in the entire score to default shape. */

void IIReshapeSlursTies(Document *doc)
{
	LINK	pL;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		if (SlurTYPE(pL))
			SetAllSlursShape(doc, pL, True);
}


/* ---------------------------------------------------------- IIFixAllNoteSlurTieFlags -- */
/* For every note in the score that claims to be TIEDL, TIEDR, SLURREDL or SLURREDR,
see if there really is a tie that starts or ends on that note. If there isn't, clear
the appropriate flag (tiedL or tiedR), so that the data structure will be consistent.
(Adapted from DCheckSyncSlurs.) */

void IIFixAllNoteSlurTieFlags(Document *doc)
{
	SearchParam pbSearch;
	LINK		pL, aNoteL, prevSyncL, tieL, slurL, searchL;
	short		voice;
	Boolean		tiedL, tiedR, slurredL, slurredR;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		if (SyncTYPE(pL))
			for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				tiedL = NoteTIEDL(aNoteL);
				tiedR = NoteTIEDR(aNoteL);
				if (tiedL || tiedR) {
					InitSearchParam(&pbSearch);
					pbSearch.id = ANYONE;							/* Prepare for search */
					voice = pbSearch.voice = NoteVOICE(aNoteL);
					pbSearch.subtype = True;						/* Tieset, not slur */
			
					if (tiedL) {
						/* If we start searching for the tie here, we may find one that
						   starts with this Sync, while we want one that ends with this
						   Sync. Instead, start searching from the previous Sync in this
						   voice. Exception: if the previous Sync is not in the same
						   System, the tie we want is the second piece of a cross-system
						   one; in this case, search to right from the previous Measure.
						   Cf. LeftSlurSearch. */
							
						prevSyncL = LVSearch(LeftLINK(pL), SYNCtype, voice, GO_LEFT, False);
						if (prevSyncL && SameSystem(pL, prevSyncL)) {
							searchL = prevSyncL;
							tieL = L_Search(searchL, SLURtype, GO_LEFT, &pbSearch);
						}
						else {
							searchL = LSSearch(pL, MEASUREtype, 1, GO_LEFT, False);
							tieL = L_Search(searchL, SLURtype, GO_RIGHT, &pbSearch);
						}
				
						if (!tieL || SlurLASTSYNC(tieL)!=pL)
							NoteTIEDL(aNoteL) = False;
					}
				
					if (tiedR) {
						tieL = L_Search(pL, SLURtype, GO_LEFT, &pbSearch);
						if (!tieL || SlurFIRSTSYNC(tieL)!=pL)
							NoteTIEDR(aNoteL) = False;
					}
				}
				slurredL = NoteSLURREDL(aNoteL);
				slurredR = NoteSLURREDR(aNoteL);				
				if (slurredL || slurredR) {
					InitSearchParam(&pbSearch);
					pbSearch.id = ANYONE;
					voice = pbSearch.voice = NoteVOICE(aNoteL);
					pbSearch.subtype = False;							/* Slur, not tieset */
					if (slurredL) {
						/* See comments for ties above. */
						
						prevSyncL = LVSearch(LeftLINK(pL), SYNCtype, voice, GO_LEFT, False);
						if (prevSyncL && SameSystem(pL, prevSyncL)) {
							searchL = prevSyncL;
							slurL = L_Search(searchL, SLURtype, GO_LEFT, &pbSearch);
						}
						else {
							searchL = LSSearch(pL, MEASUREtype, 1, GO_LEFT, False);
							slurL = L_Search(searchL, SLURtype, GO_RIGHT, &pbSearch);
						}
				
						if (!slurL || SlurLASTSYNC(slurL)!=pL)
							NoteSLURREDL(aNoteL) = False;
					}
					if (slurredR) {
						slurL = L_Search(pL, SLURtype, GO_LEFT, &pbSearch);
						if (!slurL || SlurFIRSTSYNC(slurL)!=pL)
							NoteSLURREDR(aNoteL) = False;
					}
				}
			}
}


/* ---------------------------------------------------------------------- AnchorSearch -- */
/* Given a Dynamic, Graphic, Ottava, Tempo or Ending, return the closest (in the object
list) eligible anchor symbol; if there is none, return NILINK.
	
	<dependentL> type			eligible anchor types
	-----------------------------------------------------------
	DYNAMtype					SYNCtype
	GRAPHICtype					SYNCtype, RPTENDtype, PAGEtype, MEASUREtype, CLEFtype,
								KEYSIGtype, TIMESIGtype, SPACERtype, PSMEAStype
	OTTAVAtype					SYNCtype, GRSYNCtype
[	SLURtype					SYNCtype, SYSTEMtype --slurs and tuplets not handled here]
[	TUPLETtype					SYNCtype ]
	TEMPOtype					SYNCtype, RPTENDtype, MEASUREtype, CLEFtype, KEYSIGtype,
								TIMESIGtype, PSMEAStype
	ENDINGtype					SYNCtype, MEASUREtype, PSMEAStype
	
	Notes:
		1.	Page-relative graphics come right *after* their page anchor objects
			in the object list, not before them, as is the case with other
			types of anchor objects.
		2. Some dependent symbols have secondary anchors, such as the lastObjL
			of an ending. AnchorSearch does not look for these secondary anchors;
			the caller must do so.
*/

static LINK AnchorSearch(Document *doc, LINK dependentL)
{
	short	dType,		/* type of dependent symbol */
			staff,		/* staff of dependent symbol */
			iVoice;		/* internal voice number of dependent symbol */
	LINK	pL;
	
	iVoice = NOONE;
	
	dType = ObjLType(dependentL);
	switch (dType) {
		case DYNAMtype:
			staff = DynamicSTAFF(FirstSubLINK(dependentL));
			break;
		case GRAPHICtype:
			staff = GraphicSTAFF(dependentL);
			iVoice = GraphicVOICE(dependentL);
			
			/* Handle page-rel graphics right here. */
			
			if (staff==NOONE && iVoice==NOONE)
				return SSearch(LeftLINK(dependentL), PAGEtype, GO_LEFT);
			break;
		case OTTAVAtype:
			staff = OttavaSTAFF(dependentL);
			break;
		case TEMPOtype:
			staff = TempoSTAFF(dependentL);
			break;
		case ENDINGtype:
			staff = EndingSTAFF(dependentL);
			break;
		default:
			MayErrMsg("AnchorSearch: unsupported dependent symbol type at %d.", dependentL);
			return NILINK;
	}

	for (pL = RightLINK(dependentL); pL!=doc->tailL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
				if (iVoice!=NOONE && SyncInVoice(pL, iVoice))
					return pL;
				else if (NoteOnStaff(pL, staff))
					return pL;
				break;
			case RPTENDtype:
				if (dType==GRAPHICtype || dType==TEMPOtype)
					if (ObjOnStaff(pL, staff, False))
						return pL;
				break;
			case MEASUREtype:
				if (dType==GRAPHICtype || dType==TEMPOtype || dType==ENDINGtype)
					if (MeasOnStaff(pL, staff))
						return pL;
				break;
			case CLEFtype:
				if (dType==GRAPHICtype || dType==TEMPOtype)
					if (ClefOnStaff(pL, staff))
						return pL;
				break;
			case KEYSIGtype:
				if (dType==GRAPHICtype || dType==TEMPOtype)
					if (KeySigOnStaff(pL, staff))
						return pL;
				break;
			case TIMESIGtype:
				if (dType==GRAPHICtype || dType==TEMPOtype)
					if (TimeSigOnStaff(pL, staff))
						return pL;
				break;
			case GRSYNCtype:
				if (dType==OTTAVAtype && GRNoteOnStaff(pL, staff))
					return pL;
				break;
			case SPACERtype:
				if (dType==GRAPHICtype && SpacerSTAFF(pL)==staff)
					return pL;
				break;
			case PSMEAStype:
				if (dType==GRAPHICtype || dType==TEMPOtype || dType==ENDINGtype)
					if (ObjOnStaff(pL, staff, False))
						return pL;
				break;
		}		
	}
	
	return NILINK;
}


/* --------------------------------------------------------------- IIAnchorAllJDObjs -- */
/* FIXME: Big problems: If a symbol should be attached to an anchor in the reserved
area, this will fail, because during conversion the symbol will have been added to the
data structure *after* the reserved anchor symbol. (This applies to page-rel graphics
also.) Or, since we're reformatting after conversion, is this a problem only with the
first system?? */

Boolean IIAnchorAllJDObjs(Document *doc)
{
	LINK	pL, anchorL;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
//??don't need this!->		if (J_DTYPE(pL))
			switch (ObjLType(pL)) {
				case GRAPHICtype:
					if (GraphicFIRSTOBJ(pL)==NILINK) {
						anchorL = AnchorSearch(doc, pL);
						if (!anchorL) goto broken;
						GraphicFIRSTOBJ(pL) = anchorL;
//??Here's where I should move the graphic in the data structure if it's not just
//before anchorL
					}
					break;
				case TEMPOtype:
					if (TempoFIRSTOBJ(pL)==NILINK) {
						anchorL = AnchorSearch(doc, pL);
						if (!anchorL) goto broken;
						TempoFIRSTOBJ(pL) = anchorL;
					}
					break;
				case DYNAMtype:
					if (DynamFIRSTSYNC(pL)==NILINK) {
						anchorL = AnchorSearch(doc, pL);
						if (!anchorL) goto broken;
						DynamFIRSTSYNC(pL) = anchorL;
					}
					break;
			}
	return True;
	
broken:
	MayErrMsg("IIAnchorAllJDObjs: can't find an anchor for dependent symbol at %d.", pL);
	return False;
}


/* ------------------------------------------------------------------ IIAutoMultiVoice -- */
/* For the entire document, guess the roles of all voices on a measure-by-measure basis,
and apply the appropriate multi-voice rules. */

void IIAutoMultiVoice(Document	*doc,
						Boolean	doSingle)		/* Call DoMultivoiceRules even when a voice is VCROLE_SINGLE */
{
	short	voiceRole, v;
	LINK	startL, endL;
	
	startL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, False);

	for (endL = NILINK; startL!=doc->tailL; startL = endL) {
		endL = EndMeasSearch(doc, RightLINK(startL));
		for (v = 1; v<=MAXVOICES; v++)
			if (VOICE_MAYBE_USED(doc, v)) {
				voiceRole = GuessVoiceRole(doc, v, startL, endL);
				if (voiceRole!=VCROLE_SINGLE || doSingle) {
					doc->lookVoice = v;
					SelRangeNoHilite(doc, startL, endL);
					DoMultivoiceRules(doc, voiceRole, True, False);
					DeselRangeNoHilite(doc, doc->headL, doc->tailL);
				}
			}
	}
	doc->lookVoice = ANYONE;							/* look at all voices */
}


/* -------------------------------------------------------------------- GuessVoiceRole -- */
/* Guess the role of the given voice over the range (startL, endL], and return
VCROLE_UPPER, VCROLE_LOWER, VCROLE_CROSS, or VCROLE_SINGLE. If the range contains
no notes in the given voice, return VCROLE_SINGLE. If the voice occurs on more than one
staff, return VCROLE_CROSS. If more than one voice shares iVoice's staff, give up and
return VCROLE_SINGLE. Specify <iVoice> with an internal, not a part-relative, voice
number. NB: the algorithm for guessing voice role is quite crude, but should usually 
be satisfactory when there are no more than 2 voices on a staff. The caller will get
the best results if the range comprises exactly one measure's worth of notes. */
	
static short GuessVoiceRole(Document */*doc*/, short iVoice, LINK startL, LINK endL)
{
	LINK	pL, aNoteL;
	PANOTE	aNote;
	short 	v, ourStaff, voiceCnt, otherVoice, ourNoteCnt, otherNoteCnt;
	long	ourPitchTotal, otherPitchTotal;
	float	ourAvg, otherAvg;
	Boolean	sharingVoices[MAXVOICES+1];

	/* Does <iVoice> span more than one staff over the given range? If so, return VCROLE_CROSS. */
	ourStaff = 0;
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->voice==iVoice) {
					if (ourStaff==0)
						ourStaff = aNote->staffn;
					else if (aNote->staffn!=ourStaff)
						return VCROLE_CROSS;
				}
			}
		}
	if (ourStaff==0) return VCROLE_SINGLE;				/* range contains no notes in the given voice */
	
	/* Does <iVoice> share its staff with other voices? If not, return VCROLE_SINGLE.
	   If it shares with more than one voice, give up and return VCROLE_SINGLE. */
	
	for (v = 1; v<=MAXVOICES; v++)
		sharingVoices[v] = False;
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->staffn==ourStaff)
					sharingVoices[aNote->voice] = True;
			}
		}
	voiceCnt = otherVoice = 0;
	for (v = 1; v<=MAXVOICES; v++)
		if (sharingVoices[v]) {
			if (v!=iVoice) otherVoice = v;
			voiceCnt++;
		}
	if (voiceCnt==1) return VCROLE_SINGLE;
	if (voiceCnt>2) return VCROLE_SINGLE;

	/* Loop through the range once more to find out which voice, iVoice or otherVoice,
	   is higher in pitch on average. If iVoice is higher, return VCROLE_UPPER; if
	   lower, return VCROLE_LOWER. */
	
	ourNoteCnt = otherNoteCnt = 0;
	ourPitchTotal = otherPitchTotal = 0L;
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (aNote->voice==iVoice) {
					ourPitchTotal += aNote->yqpit;
					ourNoteCnt++;
				}
				if (aNote->voice==otherVoice) {
					otherPitchTotal += aNote->yqpit;
					otherNoteCnt++;
				}
			}
		}
	ourAvg = (float)ourPitchTotal / ourNoteCnt;
	otherAvg = (float)otherPitchTotal / otherNoteCnt;
	
	if (ourAvg<otherAvg)	return VCROLE_UPPER;		/* NB: yqpit goes up when pitch goes down */
	else					return VCROLE_LOWER;					
}


/* ------------------------------------------------------------------- IIJustifySystem -- */
/* Justify one system using a "smart" justification algorithm:
	1) justify normally,
	2) repace using the new space percentage (stored in first measure of system),
	3) then justify again.
This produces better looking spacing than the normal justify command, especially when
there isn't much content on a system. NB: Does not complain if the space percentage
exceeds the limit. Returns True if OK, False if error. */

Boolean IIJustifySystem(Document *doc, LINK systemL)
{
	long		justProp;
	DDIST		lastMeasWidth, staffWidth;
	FASTFLOAT	justFact;
	LINK		firstMeasL, termSysL, lastMeasL;
	PMEASURE	pMeas;
	Boolean		ok;

	/* Get first and last Measure of System and object ending it. */
	
	firstMeasL = LSSearch(systemL, MEASUREtype, ANYONE, GO_RIGHT, False);
	termSysL = EndSystemSearch(doc, systemL);
	lastMeasL = LSSearch(termSysL, MEASUREtype, ANYONE, GO_LEFT, False);
	
	/* If the System isn't empty (e.g., the invisible barline isn't the last barline in
	   the System), then we can justify the measures in it. */
	
	if (lastMeasL!=firstMeasL) {
		justFact = SysJustFact(doc, firstMeasL, lastMeasL, &staffWidth, &lastMeasWidth);
		justProp = (long)(justFact*100L*RESFACTOR);
		ok = StretchToSysEnd(doc, firstMeasL, lastMeasL, (long)justProp, staffWidth,
								lastMeasWidth);
		
		/* Respace, using the percentage that JustifySystem placed in first meas of this
		   system. */
		
		pMeas = GetPMEASURE(firstMeasL);
		ok = RespaceBars(doc, firstMeasL, termSysL, RESFACTOR*pMeas->spacePercent, False, False);
	
		/* Now justify again! */
		
		justFact = SysJustFact(doc, firstMeasL, lastMeasL, &staffWidth, &lastMeasWidth);
		justProp = (long)(justFact*100L*RESFACTOR);
		ok = StretchToSysEnd(doc, firstMeasL, lastMeasL, (long)justProp, staffWidth,
								lastMeasWidth);
	}
	return True;
}


/* ---------------------------------------------------------------------- IIJustifyAll -- */
/* Justify the entire score using the "smart" justification algorithm described in
IIJustifySystem. Returns True if OK, False if error. */

Boolean IIJustifyAll(Document *doc)
{
	LINK	sysL, startSysL;
	Boolean	ok = False;
	
	startSysL = LSSearch(doc->headL, SYSTEMtype, ANYONE, GO_RIGHT, False);
	for (sysL = startSysL; sysL; sysL = LinkRSYS(sysL)) {
		ok = IIJustifySystem(doc, sysL);
		if (!ok) break;
	}
	return ok;
}


/* --------------------------------------------------------------------- OpenInputFile -- */

FILE *OpenInputFile(Str255 macfName, short vRefNum)
{
	OSErr	result;
//	short	volNum;
//	long	aProcID, dirID;
	char	ansifName[256];
	FILE	*f;

// FIXME: What was this call intended for?	
//	result = GetWDInfo(vRefNum, &volNum, &dirID, &aProcID);
//	if (result!=noErr) goto err;
	
	result = HSetVol(NULL, vRefNum,0);
	if (result!=noErr) goto err;
	
	sprintf(ansifName, "%#s", (char *)macfName);
	f = fopen(ansifName, "r");
	if (f==NULL) {
		result = ioErr;		// FIXME: how do I get the *real* Mac I/O error?
		goto err;
	}
	
	return f;
err:
	ReportIOError(result, OPENTEXTFILE_ALRT);
	return NULL;
}


/* -------------------------------------------------------------------- CloseInputFile -- */

short CloseInputFile(FILE *f)
{
	return fclose(f);
}


/* -------------------------------------------------------------- ReadLine, IIReadLine -- */
/* Read a line of text from the given stream <f> and place it in the given buffer
<inBuf>. Read chars until we find a newline (or return). Replace the newline char with
a null. If no newline appears after reading <maxChars> - 1 chars, store a null into the
last byte of <inBuf>. (Does not append a newline to the string.)  The purpose of this
function is to avoid using the standard C library's gets, which doesn't guarantee that
the given buffer will not be overwritten by a very long line.

If the read was successful, return True. If we reach EOF without reading a character,
or if there is some kind of error, return False.

For example, if the stream contains "now is the time for all bytes to stand up and
be counted\n", and <maxChars> is 9, ReadLine will not reach the newline and will
deliver an array containing "now is t'\0'". The file position pointer will be advanced
so that the next character read  from the stream will be 'h', and so the next call to
ReadLine will deliver "he time '\0'". Each call to ReadLine delivers 8, not 9, chars,
followed by a null.

NB: The below comment refer to obsolete Mac IDEs and must date back to the very early
21st century; I don't know if they're still at all relevant!  --DAB, May 2017

"CAUTION: The behavior of ReadLine depends upon Metrowerks' implementation of the C
library function, fgets. I don't know if THINK C works the same way, but it probably
does.

"From Metrowerks CW6 "C Library Reference"...
     char *fgets(char *s, int n, FILE *stream);
  The fgets() function reads characters sequentially from stream
  beginning at the current file position, and assembles them into s as a
  character array. The function stops reading characters when n
  characters have been read. The fgets() function finishes reading
  prematurely if it reaches a newline ('\n') character or the
  end-of-file. Unlike the gets() function, fgets() appends the newline
  character ('\n') to s. It also null terminates the character array.

"This description is too vague. If it null terminates the array, after reading n chars,
does the null replace the last of those chars? No. From observation in the MWDebugger,
I conclude that the final byte of the given buffer <s> is reserved for a terminating
null. So fgets never reads more than <n> - 1 bytes." */

#ifdef NOMORE
Boolean ReadLine1(char inBuf[], short maxChars, FILE *f)
{
	char	*ptr;
	
	ptr = fgets(inBuf, maxChars, f);
	if (ptr) {
		long len = strlen(inBuf);		/* length of string, including any terminating newline, but not including the terminating null */
		if (inBuf[len-1]=='\n')			/* if last char of string is newline, replace with null */
			inBuf[len-1] = '\0';
		return True;
	}
	else
		return False;
}
#endif

/* Linebreak chars/sequences:
	\n		OS X
	\r		Unix
	\r\n	Windows
*/

Boolean ReadLine(char inBuf[], short maxChars, FILE *f)
{
	int i = 0;
	char c = fgetc(f);
	char *p = inBuf;
	
	for (; i<maxChars-1 && c != EOF; i++, c = fgetc(f)) {
		if (c == '\n') {
			*p++ = '\n';		
			*p = '\0';
			return (c != EOF);
		}
		else if (c == '\r') {
			char cc = fgetc(f);				/* advance past windows terminator char if necessary */
			if (cc != '\n' && cc != EOF)	/* can't unget EOF */
				ungetc(cc, f);
				
			*p++ = '\n';		
			*p = '\0';			
			return (c != EOF);
		}
		else {
			*p++ = c;
		}
	}
	
	*p = '\0';
	return (c != EOF);
}


/* --------------------------------------------------------------------- Miscellaneous -- */

short IIOpenInputFile(FSSpec *fsSpec, short *refNum, Boolean readOnly)
{
	short errCode;
	
	if (readOnly)
		errCode = FSpOpenDF(fsSpec, fsRdPerm, refNum);			/* Open the file */
	else {
		errCode = FSpOpenDF(fsSpec, fsRdWrPerm, refNum);		/* Open the file */
		if (errCode == fLckdErr || errCode == permErr)
			errCode = FSpOpenDF(fsSpec, fsRdPerm, refNum);		/* Try again: open the file read-only */
	}
	
	return errCode;
}

short IIReadChar(short refNum, char *c)
{
	short errCode = noErr;
	long count = 1;
	
	errCode = FSRead(refNum, &count, c);
	return errCode;
}

short IIUngetChar(short refNum)
{
	short errCode = noErr;
	long fPos = 0;
	errCode = GetFPos(refNum, &fPos); 
	if (errCode == noErr && fPos > 0) {
		errCode = SetFPos(refNum, fsFromMark, -1);
	}
	return errCode;
}

Boolean IIReadLine(char inBuf[], short maxChars, short refNum)
{
	int i = 0;
	short errCode = noErr;
	char c,cc;
	
	errCode = IIReadChar(refNum, &c);
	if (errCode != noError) return False;
	
	char *p = inBuf;
	
	for (; i<maxChars-1 && c != EOF; ) {
		if (c == '\n') {
			*p++ = '\n';		
			*p = '\0';
			return (c != EOF);
		}
		else if (c == '\r') {
			errCode = IIReadChar(refNum, &cc);		/* advance past Windows terminator char if necessary */
			if (errCode != noError) goto errorReturn;
			
			if (cc != '\n' && cc != EOF) {			/* can't unget EOF */
				errCode = IIUngetChar(refNum);
				if (errCode != noError) goto errorReturn;
			}
				
			*p++ = '\n';		
			*p = '\0';			
			return (c != EOF);
		}
		else {
			*p++ = c;
		}
		i++;
		errCode = IIReadChar(refNum, &c);
		if (errCode != noError) goto errorReturn;
	}
	
	*p = '\0';
	
	return (c != EOF);
	
errorReturn:
	return False;		/* don't keep reading file if error */
}

short IICloseInputFile(short refNum)
{
	short errCode = noErr;
	errCode = FSClose(refNum);
	return errCode;
}

short IIRewind(short refNum)
{
	short errCode;
	
	errCode = SetFPos(refNum, fsFromStart, 0);
	return errCode;
}
