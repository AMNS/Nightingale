/***************************************************************************
*	FILE:	Score.c
*	PROJ:	Nightingale
*	DESC:	Large-scale score data structure manipulation routines:
		InitFontRecs			FixGraphicFont
		NewDocScore				GetSysHeight
		FixTopSystemYs			PageFixSysRects			FixSystemRectYs
		FillStaffArray			FillStaffTopArray		UpdateStaffTops
		PageFixMeasRects
		FixMeasRectYs			FixMeasRectXs			FixSysMeasRectXs
		SetStaffLength			SetStaffSize			ChangeSysIndent
		IndentSystems			AddSysInsertPt			AddSystem
		InitParts				MakeSystem				MakeStaff
		MakeConnect				MakeClef				MakeKeySig
		MakeTimeSig				MakeMeasure				CreateSysFixContext
		CreateSystem			AddPage					CreatePage
		ScrollToPage			GoTo					GoToSel
/***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "CoreMIDIDefs.h"

static void PageFixMeasRects(Document *, LINK, Boolean, Boolean);

static LINK MakeClef(Document *, LINK, short, CONTEXT [], DDIST);
static LINK MakeKeySig(Document *, LINK, short, CONTEXT [], short *, DDIST);
static LINK MakeTimeSig(Document *, LINK, short, CONTEXT [], DDIST);
static void CreateSysFixContext(Document *, LINK, LINK, short);

static void ScrollToLink(Document *, LINK);


/* ------------------------------------------------------------- InitN103FontRecs -- */
/* Init just the font records introduced in file version N103. Called when converting
older files (in File.c) and from InitFontRecs below.   -JGG, 3/20/01 */

void InitN103FontRecs(Document *doc)
{
	Str255 fontReg1;

	GetIndString(fontReg1, FONT_STRS, 1);    						/* "Times" */

	Pstrcpy((StringPtr)doc->fontName6, (StringPtr)fontReg1);		/* Regular font 6 */
		doc->lyric6 = False;
		doc->enclosure6 = ENCL_NONE;
		doc->relFSize6 = True;													
		doc->fontSize6 = GRMedium;						
		doc->fontStyle6 = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontName7, (StringPtr)fontReg1);		/* Regular font 7 */
		doc->lyric7 = False;
		doc->enclosure7 = ENCL_NONE;
		doc->relFSize7 = True;													
		doc->fontSize7 = GRMedium;						
		doc->fontStyle7 = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontName8, (StringPtr)fontReg1);		/* Regular font 8 */
		doc->lyric8 = False;
		doc->enclosure8 = ENCL_NONE;
		doc->relFSize8 = True;													
		doc->fontSize8 = GRMedium;						
		doc->fontStyle8 = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontName9, (StringPtr)fontReg1);		/* Regular font 9 */
		doc->lyric9 = False;
		doc->enclosure9 = ENCL_NONE;
		doc->relFSize9 = True;													
		doc->fontSize9 = GRMedium;						
		doc->fontStyle9 = 0;										/* Plain */

	doc->fillerR6 = doc->fillerR7 = doc->fillerR8 = doc->fillerR9 = 0;
}


/* ----------------------------------------------------------------- InitFontRecs -- */

void InitFontRecs(Document *doc)
{
	Str255 fontReg1, fontReg2, fontSp1, fontSp2;
	
	doc->nFontRecords = 15;

	GetIndString(fontReg1, FONT_STRS, 1);    						/* "Times" */
	GetIndString(fontReg2, FONT_STRS, 2);    						/* "Helvetica" */
	GetIndString(fontSp1, FONT_STRS, 3);    						/* "Times" */
	GetIndString(fontSp2, FONT_STRS, 4);    						/* "Helvetica" */

	Pstrcpy((StringPtr)doc->fontNameMN, (StringPtr)fontSp1);		/* Measure no. font */
		doc->lyricMN = False;
		doc->enclosureMN = ENCL_NONE;
		doc->relFSizeMN = True;													
		doc->fontSizeMN = GRLarge;						
		doc->fontStyleMN = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontNamePN, (StringPtr)fontSp1);		/* Part name font */
		doc->lyricPN = False;
		doc->enclosurePN = ENCL_NONE;
		doc->relFSizePN = True;													
		doc->fontSizePN = GRLarge;						
		doc->fontStylePN = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontNameRM, (StringPtr)fontSp2); 		/* Rehearsal mark font */
		doc->lyricRM = False;
		doc->enclosureRM = ENCL_BOX;
		doc->relFSizeRM = True;													
		doc->fontSizeRM = GRVLarge;						
		doc->fontStyleRM = bold;

	Pstrcpy((StringPtr)doc->fontName1, (StringPtr)fontReg1);		/* Regular font 1 */
		doc->lyric1 = False;
		doc->enclosure1 = ENCL_NONE;
		doc->relFSize1 = True;													
		doc->fontSize1 = GRMedium;						
		doc->fontStyle1 = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontName2, (StringPtr)fontReg1);		/* Regular font 2 */
		doc->lyric2 = False;
		doc->enclosure2 = ENCL_NONE;
		doc->relFSize2 = True;													
		doc->fontSize2 = GRMedium;						
		doc->fontStyle2 = italic;

	Pstrcpy((StringPtr)doc->fontName3, (StringPtr)"\pSonata");		/* Regular font 3 */
		doc->lyric3 = False;
		doc->enclosure3 = ENCL_NONE;
		doc->relFSize3 = True;													
		doc->fontSize3 = GRStaffHeight;						
		doc->fontStyle3 = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontName4, (StringPtr)fontReg1);		/* Regular font 4 */
		doc->lyric4 = True;
		doc->enclosure4 = ENCL_NONE;
		doc->relFSize4 = True;
		doc->fontSize4 = GRMedium;						
		doc->fontStyle4 = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontNameTM, (StringPtr)fontSp1);		/* Tempo mark font */
		doc->lyricTM = False;
		doc->enclosureTM = ENCL_NONE;
		doc->relFSizeTM = True;
		doc->fontSizeTM = GRVLarge;						
		doc->fontStyleTM = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontNameCS, (StringPtr)fontSp2);		/* Chord symbol font */
		doc->lyricCS = False;
		doc->enclosureCS = ENCL_NONE;
		doc->relFSizeCS = True;
		doc->fontSizeCS = GRLarge;						
		doc->fontStyleCS = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontNamePG, (StringPtr)fontSp1);		/* Page font */
		doc->lyricPG = False;
		doc->enclosurePG = ENCL_NONE;
		doc->relFSizePG = False;													
		doc->fontSizePG = 12;						
		doc->fontStylePG = 0;										/* Plain */

	Pstrcpy((StringPtr)doc->fontName5, (StringPtr)fontReg2);		/* Regular font 5 */
		doc->lyric5 = False;
		doc->enclosure5 = ENCL_NONE;
		doc->relFSize5 = False;
		doc->fontSize5 = 24;
		doc->fontStyle5 = 0;										/* Plain */

	doc->fillerMN = doc->fillerPN = doc->fillerRM = 0;
	doc->fillerR1 = doc->fillerR2 = doc->fillerR3 = 0;
	doc->fillerR4 = doc->fillerTM = doc->fillerCS = 0;
	doc->fillerPG = doc->fillerR5 = 0;

	InitN103FontRecs(doc);
}


/* -------------------------------------------------------------- FixGraphicFont -- */
/* Insure that the given Graphic refers to the correct font in the score's <fontTable>,
adding a new font to fontTable if necessary (in FontName2Index). Intended for use in
pasting and similar operations. */

void FixGraphicFont(Document *doc, LINK pL)
{
	PGRAPHIC p; unsigned char font[64];

	if (GraphicTYPE(pL)) {
		p = GetPGRAPHIC(pL);
		PStrncpy((StringPtr)font, (StringPtr)clipboard->fontTable[p->fontInd].fontName, 32);
		p = GetPGRAPHIC(pL);
		p->fontInd = FontName2Index(doc, font);					/* Should never fail */						
	}
}


/* ------------------------------------------------------------------ NewDocScore -- */
/*	Create a virgin score and (unless doc is the clipboard) Master Page. Assumes
doc->headL and doc->tailL already exist. Return True if we succeed, False if we
find a problem (usually out of memory). */

Boolean NewDocScore(Document *doc)
{
	LINK		pL, qL, headL, tailL;
	PPAGE		pPage;
	DDIST		sysTop;
	short		i;
	
	doc->ledgerYSp = 2*config.defaultLedgers+2;
	initStfTop1 = (short)(doc->ledgerYSp*drSize[doc->srastral]/STFHALFLNS);
	initStfTop2 = (short)(2.5*drSize[doc->srastral]);
	
	doc->currentSystem = 1;

	headL = doc->headL; tailL = doc->tailL;

	/*
	 *	We're called here also if an error occurs reading an existing file, in which
	 *	case we may have read in the heaps, overwriting the headL's LinkNENTRIES field.
	 * Otherwise, NewDocScore is only called with the headL created in InitDocScore.
	 * (LinkNENTRIES = 2).
	 */
	if (LinkNENTRIES(headL)!=2) {							/* Only shrink if necessary. */
		GrowObject(doc, headL, -LinkNENTRIES(headL)+2);		/* Set to 2 subobjects. */
		InitObject(headL, NILINK, tailL, 0, 0, False, False, True);
	}

	doc->feedback = config.midiFeedback;					/* Someday debug and use MIDIConnected */
	doc->polyTimbral = True;
	doc->dontSendPatches = False;
	doc->tempo = 999;										/* No longer used */

	doc->omsInputDevice = config.defaultInputDevice;
	for (i=0; i<MAXSTAVES; i++)
		doc->omsPartDeviceList[i] = config.defaultOutputDevice;

	doc->cmInputDevice = config.cmDefaultInputDevice;
	
	MIDIUniqueID cmDefaultID = config.cmDefaultOutputDevice;
	if (cmDefaultID == kInvalidMIDIUniqueID)
		cmDefaultID = gAuMidiControllerID;
	
	for (i=0; i<MAXSTAVES; i++)
		doc->cmPartDeviceList[i] = gAuMidiControllerID;

	doc->channel = config.defaultChannel;
	doc->velocity = 1;
	doc->changed = doc->saved = False;
	doc->named = doc->used = False;
	doc->hasCaret = False;
	doc->transposed = 0;
	doc->lyricText = False;									/* No longer used */
	doc->spacePercent = 100;
	doc->nstaves = 0;
	
	doc->philler = 0;
	doc->headerStrOffset = doc->footerStrOffset = 0;		/* Empty string */
	doc->fillerPGN = doc->fillerMB = 0;
	doc->filler1 = doc->filler2 = 0;
	doc->fillerInt = doc->fillerHP = doc->fillerLP = 0;
	doc->fillerEM = 0;
	
	doc->comment[0] = 0;									/* (unused) */

	doc->lastGlobalFont = 4;								/* Use regular1 as most recent */

	InitFontRecs(doc);
	
	doc->nfontsUsed = 0;								/* No need to call FillFontTable for empty table */
	
	qL = headL;
	pL = InsertNode(doc, RightLINK(qL), PAGEtype, 0);
	if (!pL) { NoMoreMemory(); return False; }
	
	SetObject(pL, 0, 0, False, True, True);
	LinkTWEAKED(pL) = False;
	qL = RightLINK(headL) = pL;
	pPage = GetPPAGE(pL);
	pPage->lPage = pPage->rPage = NILINK;
	pPage->sheetNum = 0;

	sysTop = SYS_TOP(doc)+pt2d(config.titleMargin);
	if (!CreateSystem(doc, qL, sysTop, FirstSystem)) return False;
	doc->selStartL = doc->selEndL = doc->tailL;
	doc->yBetweenSys = 0;

	if (doc!=clipboard) NewMasterPage(doc, sysTop, False);
	BuildVoiceTable(doc, True);
	
	return True;
}


/* ---------------------------------------------------------------- GetSysHeight -- */
/* Get the "normal" height of the given system, either from an adjacent system or
(if it's the only system of the score) from the default. */

DDIST GetSysHeight(Document *doc,
					LINK sysL,
					short where)	/* See comments on CreateSystem */
{
	LINK baseSys;  DRect sysRect;

	if (where==FirstSystem)
		return (MEAS_BOTTOM(initStfTop1+initStfTop2, STHEIGHT));

	baseSys = (where==BeforeFirstSys ? LinkRSYS(sysL) : LinkLSYS(sysL));	
	sysRect = SystemRECT(baseSys);	

	return (sysRect.bottom-sysRect.top);
}


/* -------------------------------------------------------------- FixTopSystemYs -- */
/* Fix the systemRect tops and bottoms for every page top system in the range
[startSysL,endSysL). */

void FixTopSystemYs(Document *doc, LINK startSysL, LINK endSysL)
{
	LINK pL;  PSYSTEM pSys;  DDIST sysHeight;

	for (pL = startSysL; pL!=endSysL; pL = RightLINK(pL))
		if (SystemTYPE(pL))
			if (FirstSysInPage(pL)) {
				pSys = GetPSYSTEM(pL);
				sysHeight = pSys->systemRect.bottom-pSys->systemRect.top;
				pSys->systemRect.top = pt2d(doc->marginRect.top);
				pSys->systemRect.bottom = pSys->systemRect.top+sysHeight;
			}
}


/* ------------------------------------------------------------- PageFixSysRects -- */
/* Using the systemRect.top for the first system, fix up systemRect.tops for all
other systems and systemRect.bottoms for all systems on pageL. Optionally use the
measureRect.bottom for the bottom staff in the first measure of the first system
to reset all systemRect heights (this may be dangerous if not all systems have
the same visibility of staves!). Otherwise preserve the current systemRect heights.
FIXME: It's not obvious this function works correctly if there's only system per
page, and it's not obvious that prevSysBottom and sysHeight get set correctly or at
all in all cases. */

void PageFixSysRects(
			Document *doc,
			LINK pageL,
			Boolean setSysHts 	/* True=reset systemRect heights from first Measure measureRect */
			)
{
	LINK prevSysL, sysL, measL, aMeasL;
	register PSYSTEM pSystem;
	DDIST prevSysBottom, sysHeight;

	sysL = LSSearch(pageL, SYSTEMtype, ANYONE, GO_RIGHT, False);

	for (prevSysL = sysL; sysL; prevSysL=sysL,sysL=LinkRSYS(sysL)) {

		if (SysPAGE(prevSysL)!=SysPAGE(sysL)) break;

		if (FirstSysInPage(sysL)) {
			if (setSysHts) {
				measL = LSSearch(sysL, MEASUREtype, ANYONE, GO_RIGHT, False);
				aMeasL = MeasOnStaff(measL, doc->nstaves);
				sysHeight = MeasMRECT(aMeasL).bottom;
			}
			else {
				pSystem = GetPSYSTEM(sysL);
				sysHeight = pSystem->systemRect.bottom-pSystem->systemRect.top;
			}
		}
		else {
			pSystem = GetPSYSTEM(sysL);
			if (!setSysHts) sysHeight = pSystem->systemRect.bottom-pSystem->systemRect.top;
			pSystem->systemRect.top = prevSysBottom+doc->yBetweenSys;
		}
		
		/* We now have sysHeight, and sysL's systemRect.top is correct, in all cases. */

		pSystem = GetPSYSTEM(sysL);
		pSystem->systemRect.bottom = pSystem->systemRect.top+sysHeight;
		prevSysBottom = pSystem->systemRect.bottom;
		LinkVALID(sysL) = False;
	}
}


/* ------------------------------------------------------------- FixSystemRectYs -- */
/* On each Page, using the systemRect.top for the first system, fix up systemRect
.tops for all other systems and systemRect.bottoms for all systems. Optionally use
the measureRect.bottom for the first measure of the first system to reset all
systemRect heights; otherwise preserve the current systemRect heights. */

void FixSystemRectYs(
		Document *doc,
		Boolean setSysHts 	/* True=reset systemRect heights from top sys measureRect */
		)
{
	LINK pageL;

	pageL = LSSearch(doc->headL, PAGEtype, ANYONE, GO_RIGHT, False);

	for ( ; pageL; pageL = LinkRPAGE(pageL))
		PageFixSysRects(doc, pageL, setSysHts);
}


/* -------------------------------------------------------------- FillStaffArray -- */
/* Fill an array of LINKs to Staff subobjects, indexed by the staffn of the
subobjects. The array is assumed to have space allocated for at least MAXSTAVES+1
LINKs. Search right from startL for the first Staff object; if it is found, use
it to fill the array, and return its LINK; else return NILINK. */

LINK FillStaffArray(Document */*doc*/, LINK startL, LINK staves[])
{
	LINK staffL, aStaffL;

	staffL = LSSearch(startL, STAFFtype, ANYONE, GO_RIGHT, False);
	if (staffL) {
		aStaffL = FirstSubLINK(staffL);
		for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
			staves[StaffSTAFF(aStaffL)] = aStaffL;
		return staffL;
	}
	return NILINK;
}


/* ----------------------------------------------------------- FillStaffTopArray -- */
/* Fill an array of staffTop positions, indexed by the staffn of the staves.
Search right from startL for the first staff object; if it is found, use it
to fill the array, and return True; else return False. */

Boolean FillStaffTopArray(Document */*doc*/, LINK startL, DDIST staffTop[])
{
	LINK staffL, aStaffL;  PASTAFF aStaff;

	staffL = LSSearch(startL, STAFFtype, ANYONE, GO_RIGHT, False);
	if (staffL) {
		aStaffL = FirstSubLINK(staffL);
		for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
			aStaff = GetPASTAFF(aStaffL);
			staffTop[StaffSTAFF(aStaffL)] = aStaff->staffTop;
		}
		return True;
	}
	return False;
}


/* ------------------------------------------------------------- UpdateStaffTops -- */
/* Update the staffTop fields of all Staff subobjects in range [startL, endL): set
each to the corresponding value in the <staffTop> array. FIXME: MasterPage.c should
call this instead of its own version. */

void UpdateStaffTops(Document */*doc*/,					/* unused */
						LINK startL, LINK endL,
						DDIST staffTop[])
{
	LINK pL, aStaffL;  PASTAFF aStaff;
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (StaffTYPE(pL)) {
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				aStaff = GetPASTAFF(aStaffL);
				aStaff->staffTop = staffTop[StaffSTAFF(aStaffL)];
			}
		}
}

/* ------------------------------------------------------------ PageFixMeasRects -- */
/* Fix up measureRect tops and bottoms for all measures on Page <pageL>. We do
this based mostly on Staff positions and sizes. */

static void PageFixMeasRects(
					Document *doc,
					LINK pageL,
					Boolean useLedg,
					Boolean masterPg		/* True=use the masterSystem to fix measRects */
					)
{
	LINK sysL, staffL, staves[MAXSTAVES+1];
	LINK prevMeasL, measL, aMeasL, measures[MAXSTAVES+1];
	PAMEASURE aMeas;  PASTAFF aStaff;  short i;
	DRect sysRect;

	staffL = FillStaffArray(doc, pageL, staves);
	measL = LSSearch(pageL, MEASUREtype, ANYONE, GO_RIGHT, False);

	for (prevMeasL=measL; measL; prevMeasL=measL, measL=LinkRMEAS(measL)) {

		if (MeasPAGE(prevMeasL)!=MeasPAGE(measL)) break;

		LinkVALID(measL) = False;

		aMeasL = FirstSubLINK(measL);
		for (; aMeasL; aMeasL = NextMEASUREL(aMeasL))
			measures[MeasureSTAFF(aMeasL)] = aMeasL;

		/* The bottom of each Measure is at the top of the staff below, except
			for the bottom staff's Measure, which is a special case. Similarly,
			the top of each Measure is at the bottom of the staff above, except
			for the top staff's Measure, which is always at 0. */

		for (i = 1; i<doc->nstaves; i++) {
			aMeas = GetPAMEASURE(measures[i]);
			aStaff = GetPASTAFF(staves[i+1]);
			aMeas->measSizeRect.bottom = aStaff->staffTop;
		}
		aMeas = GetPAMEASURE(measures[doc->nstaves]);
		aStaff = GetPASTAFF(staves[doc->nstaves]);
		if (useLedg)
			aMeas->measSizeRect.bottom = MEAS_BOTTOM(aStaff->staffTop, aStaff->staffHeight);
		else {
			/*
			 * Set the measSizeRect.bottom of the last measure subobj in the system.
			 * measSizeRect is v-relative to systemTop, so the last meas subobj's
			 * measSizeRect.bottom will just be the sysHeight.
			 */
			if (masterPg)
				sysL = SSearch(doc->masterHeadL, SYSTEMtype, GO_RIGHT);
			else
				sysL = LSSearch(measL, SYSTEMtype, ANYONE, GO_LEFT, False);

			sysRect = SystemRECT(sysL);
			aMeas->measSizeRect.bottom = sysRect.bottom - sysRect.top;
		}
	
		for (i = 2; i<=doc->nstaves; i++) {
			aMeas = GetPAMEASURE(measures[i]);
			aStaff = GetPASTAFF(staves[i-1]);
			aMeas->measSizeRect.top = aStaff->staffTop+aStaff->staffHeight;
		}
		aMeas = GetPAMEASURE(measures[1]);
		aMeas->measSizeRect.top = 0;
		
		if (LastMeasInSys(measL)) {
			if (!staffL)
				MayErrMsg("PageFixMeasRects:  staffL=NILINK for measL %ld", (long)measL);

			staffL = FillStaffArray(doc, RightLINK(staffL), staves);
		}
	}
}


/* --------------------------------------------------------------- FixMeasRectYs -- */
/* Set all Measure subobjects' measSizeRect.tops and .bottoms from Staff tops
and heights.  Also optionally set the Systems' systemRect.tops and .bottoms. If
pageL is NILINK, do this for the entire score; otherwise pageL should be a Page
object, and we do it for that page only. */

void FixMeasRectYs(
		Document *doc,
		LINK 	pageL,		/* Page to fix up, or NILINK for entire score */
		Boolean	fixSys,		/* True=fix up systemRects as well */
		Boolean	useLedg,	/* True=set measSizeRect on bottom stf from doc->ledgerYSp */
		Boolean	masterPg	/* True=use the masterSystem to fix meas/sysRects */
		)
{
	if (pageL==NILINK) {
		pageL = LSSearch(doc->headL, PAGEtype, ANYONE, GO_RIGHT, False);
		for ( ; pageL; pageL = LinkRPAGE(pageL)) {
			PageFixMeasRects(doc, pageL, useLedg, masterPg);
			if (fixSys)
				PageFixSysRects(doc, pageL, True);
		}
	}
	else {
		PageFixMeasRects(doc, pageL, useLedg, masterPg);
		if (fixSys)
			PageFixSysRects(doc, pageL, True);
	}
}


/* --------------------------------------------------------------- FixMeasRectXs -- */
/* Fix the measSizeRect.left and .right for every Measure from <startBarL> to
<endBarL> so each ends where the next one begins, except for the last Measures
of Systems, which are made to extend to the end of their System. <startBarL> must
be a Measure, <endBarL> can be either a Measure or NILINK. */

Boolean FixMeasRectXs(LINK startBarL, LINK endBarL)
{
	LINK	barL;
	DDIST	measWidth;
	
	if (!MeasureTYPE(startBarL))
		startBarL = LSSearch(startBarL, MEASUREtype, ANYONE, GO_LEFT, False);
	if (endBarL && !MeasureTYPE(endBarL))
		endBarL = LSSearch(endBarL, MEASUREtype, ANYONE, GO_LEFT, False);
	if (!startBarL) return False;

	for (barL = startBarL; barL; barL = LinkRMEAS(barL)) {
		if (LastMeasInSys(barL))
			MeasFillSystem(barL);
		else {
			measWidth = LinkXD(LinkRMEAS(barL))-LinkXD(barL);
			if (measWidth<=0) {
				MayErrMsg("FixMeasRectXs: XD(%ld)=%ld but XD(next Meas=%ld)=%ld",
							(long)barL, (long)LinkXD(barL), 
							(long)LinkRMEAS(barL), (long)LinkXD(LinkRMEAS(barL)));
				SetMeasWidth(barL, 0);
			}
			else
				SetMeasWidth(barL, measWidth);
		}
		if (barL==endBarL) break;
	}
	
	return True;
}

/* ------------------------------------------------------------ FixSysMeasRectXs -- */
/* Fix the measSizeRect.left and .right for every Measure in sysL. sysL must be
a LINK to a System obj. */

Boolean FixSysMeasRectXs(LINK sysL)
{
	LINK firstMeasL, lastMeasL, endRangeL;
	
	firstMeasL = LSISearch(RightLINK(sysL), MEASUREtype, ANYONE, GO_RIGHT, False);
	if (!firstMeasL) return False;
	endRangeL = LinkRSYS(sysL);

	lastMeasL = endRangeL ? LSSearch(endRangeL, MEASUREtype, ANYONE, GO_LEFT, False) : NILINK;
	
	InvalRange(firstMeasL, lastMeasL);
	return FixMeasRectXs(firstMeasL, lastMeasL);
}

/* -------------------------------------------------------------- SetStaffLength -- */
/* Update the object list appropriately for the new staff length.  If there's
any content in the score, it's dangerous to let this routine decrease the width
of the Systems, since it doesn't worry about redoing system breaks and so on.
All it does is change the page width in the header (since for now we have a
fixed right margin width), the systemRect.right in each System, the staff width
in each Staff subobject, and the measure width in every Measure subobject that
ends a System. */
 
void SetStaffLength(Document *doc, short staffLength)
{
	PSYSTEM		pSys;
	PASTAFF		aStaff;
	PAMEASURE	aMeasure;
	LINK		pL, pMeasL, aStaffL, aMeasureL;
	DDIST		dStaffLength;

	dStaffLength = pt2d(staffLength)-doc->firstIndent;			/* For first System only */
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYSTEMtype:
				pSys = GetPSYSTEM(pL);
				if (pSys->systemNum>1)
					dStaffLength = pt2d(staffLength)-doc->otherIndent;	/* For all other Systems */
				pSys->systemRect.right = pSys->systemRect.left+dStaffLength;
				break;
			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
					aStaff = GetPASTAFF(aStaffL);
					aStaff->staffRight = dStaffLength;
				}
				break;
			default:
				;
		}
		
	pMeasL = LSSearch(doc->headL, MEASUREtype, ANYONE, False, False);
	for ( ; pMeasL; pMeasL = LinkRMEAS(pMeasL))
	{
		if (LastMeasInSys(pMeasL)) {
			aMeasureL = FirstSubLINK(pMeasL);
			for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
				aMeasure = GetPAMEASURE(aMeasureL);
				aMeasure->measSizeRect.right = dStaffLength-LinkXD(pMeasL);
			}
			LinkVALID(pMeasL) = False;							/* So measureBBox will get updated */
		}
	}
}


/* ------------------------------------------------------------------- SetStaffSize -- */
/* Fix values in object list in non-structural objects, Measure widths, and Staffs
for new staff size.  N.B. Does NOT adjust Staff or Measure y-coordinates, measSizeRect
heights, or anything in System objects (e.g., systemRect). */

#define SCALE(v)	((v) = sizeRatio*(v))

void SetStaffSize(Document */*doc*/, LINK headL, LINK tailL, short oldRastral, short newRastral)
{
	FASTFLOAT	sizeRatio;
	PMEVENT		p;
	PGRAPHIC	pGraphic;
	PANOTE		aNote;
	PAGRNOTE	aGRNote;
	PADYNAMIC	aDynamic;
	register PASTAFF aStaff;
	register PASLUR aSlur;	
	PAMEASURE	aMeasure;
	PACLEF		aClef;
	PAKEYSIG	aKeySig;
	PATIMESIG	aTimeSig;
	PACONNECT	aConnect;
	register LINK pL;
	LINK		aNoteL, aGRNoteL, aDynamicL, aStaffL,
				aMeasureL, aConnectL, aClefL, aKeySigL, aTimeSigL;
	DDIST		lnSpace;

	sizeRatio = (FASTFLOAT)drSize[newRastral]/drSize[oldRastral];

	for (pL = headL; pL!=tailL; pL=RightLINK(pL)) {
	
		/* Page-relative Graphics aren't affected by change of staff size. */
		if (GraphicTYPE(pL)) {
 			pGraphic = GetPGRAPHIC(pL);
			if (pGraphic->firstObj)
				if (PageTYPE(pGraphic->firstObj)) continue;
		}
		
		p = GetPMEVENT(pL);
		SCALE(p->xd);										/* Scale horiz. spacing */

		switch (ObjLType(pL)) {

			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					SCALE(aNote->xd);
					SCALE(aNote->yd);
					SCALE(aNote->ystem);
				}
				break;

			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
					aGRNote = GetPAGRNOTE(aGRNoteL);
					SCALE(aGRNote->xd);
					SCALE(aGRNote->yd);
					SCALE(aGRNote->ystem);
				}
				break;

			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
					aStaff = GetPASTAFF(aStaffL);
					/* Take into account number of staff lines when computing new staff height, but  
						set font size as if staff had 5 lines. Set other fields from interline space. */ 
					lnSpace = drSize[newRastral]/(STFLINES-1);
					aStaff->staffHeight = lnSpace * (aStaff->staffLines-1);
					aStaff->fontSize = Staff2MFontSize(drSize[newRastral]);
					aStaff->flagLeading = 0;					/* no longer used */
					aStaff->minStemFree = 0;					/* no longer used */
					aStaff->ledgerWidth = 0;					/* no longer used */
					aStaff->noteHeadWidth = HeadWidth(lnSpace);
					aStaff->fracBeamWidth = FracBeamWidth(lnSpace);
				}
				break;

			case CONNECTtype:
				aConnectL = FirstSubLINK(pL);
				for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
					aConnect = GetPACONNECT(aConnectL);
					SCALE(aConnect->xd);
				}
				break;
				
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
					aMeasure = GetPAMEASURE(aMeasureL);
					SCALE(aMeasure->measSizeRect.right);
				}
				break;
				
			case CLEFtype:
				aClefL = FirstSubLINK(pL);
				for ( ; aClefL; aClefL=NextCLEFL(aClefL)) {
					aClef = GetPACLEF(aClefL);
					SCALE(aClef->xd);
					SCALE(aClef->yd);
				}
				break;
				
			case KEYSIGtype:
				aKeySigL = FirstSubLINK(pL);
				for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
					aKeySig = GetPAKEYSIG(aKeySigL);
					SCALE(aKeySig->xd);
				}
				break;
				
			case TIMESIGtype:
				aTimeSigL = FirstSubLINK(pL);
				for ( ; aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL)) {
					aTimeSig = GetPATIMESIG(aTimeSigL);
					SCALE(aTimeSig->xd);
				}
				break;

			case DYNAMtype:
				aDynamicL = FirstSubLINK(pL);
				for ( ; aDynamicL; aDynamicL=NextDYNAMICL(aDynamicL)) {
					aDynamic = GetPADYNAMIC(aDynamicL);
					SCALE(aDynamic->xd);
					SCALE(aDynamic->yd);
					SCALE(aDynamic->endxd);
					SCALE(aDynamic->endyd);
				}
				break;

			case SLURtype:
				aSlur = GetPASLUR(FirstSubLINK(pL));
				SCALE(aSlur->seg.knot.h);
				SCALE(aSlur->seg.knot.v);
				SCALE(aSlur->seg.c0.h);
				SCALE(aSlur->seg.c0.v);
				SCALE(aSlur->seg.c1.h);
				SCALE(aSlur->seg.c1.v);
				SCALE(aSlur->endKnot.h);
				SCALE(aSlur->endKnot.v);
				break;
				
			case GRAPHICtype:
			case TEMPOtype:
				SCALE(p->yd);
				if (GraphicSubType(pL)==GRDraw) {
 					pGraphic = GetPGRAPHIC(pL);
					SCALE(pGraphic->info);
					SCALE(pGraphic->info2);
				}
				break;
				
			case TUPLETtype:
				SCALE(((PTUPLET)p)->xdFirst);
				SCALE(((PTUPLET)p)->ydFirst);
				SCALE(((PTUPLET)p)->xdLast);
				SCALE(((PTUPLET)p)->ydLast);
				break;
				
			case SPACERtype:
				break;
				
			default:
				;
		}
	}
	
	InvalRange(headL, tailL);								/* Everything you know is wrong */
}


/* ------------------------------------------------------------- ChangeSysIndent -- */

Boolean ChangeSysIndent(Document *doc, LINK sysL, DDIST change)
{
	LINK staffL, aStaffL, measL, aMeasL, endL;  DDIST sysWidth;
	PSYSTEM pSystem;  PASTAFF aStaff;  PAMEASURE aMeasure;

	pSystem = GetPSYSTEM(sysL);
	sysWidth = pSystem->systemRect.right-pSystem->systemRect.left;
	if (sysWidth-change<pt2d(POINTSPERIN)) {
		GetIndCString(strBuf, MISCERRS_STRS, 17);			/* "That much indent would make the system less than 1 inch long." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
	pSystem->systemRect.left += change;
	
	staffL = LSSearch(sysL, STAFFtype, ANYONE, GO_RIGHT, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		aStaff->staffRight -= change;
	}

	endL = EndSystemSearch(doc, sysL);
	InvalRange(staffL, endL);

	measL = LSSearch(sysL, MEASUREtype, ANYONE, GO_RIGHT, False);
	for ( ; measL; measL = LinkRMEAS(measL))
		if (LastMeasInSys(measL)) {
			aMeasL = FirstSubLINK(measL);
			for ( ; aMeasL; aMeasL=NextMEASUREL(aMeasL)) {
				aMeasure = GetPAMEASURE(aMeasL);
				aMeasure->measSizeRect.right += change;
			}
			LinkVALID(measL) = False;
			break;											/* Last meas. of this system only */
		}

	return True;
}


/* ------------------------------------------------------------------ IndentSystems -- */
/* Increase indent of system(s) in the Document by <changeIndent>. If <first>, do
the first system only; otherwise do every system but the first. */

Boolean IndentSystems(Document *doc, DDIST changeIndent, Boolean first)
{
	LINK systemL, link;  Boolean didSmthg=False;
	
	if (changeIndent==0) return False;
	
	systemL = LSSearch(doc->headL, SYSTEMtype, ANYONE, GO_RIGHT, False);
	
	if (first) {
		if (ChangeSysIndent(doc, systemL, changeIndent)) didSmthg = True;
	}
	else {
		for (link = LinkRSYS(systemL); link; link = LinkRSYS(link)) {
			if (ChangeSysIndent(doc, link, changeIndent)) didSmthg = True;
		}
	}
	
	if (didSmthg) doc->changed = True;
	return didSmthg;
}


/* -------------------------------------------------------------- AddSysInsertPt -- */
/* Return an insertion point for Add System, when the caller wishes to add a
system at pL. AddSystem must be called with the page, system or tail object
before which to add the system.
Assumes system is to be added before the page or system following the system
containing the insertion point defined by pL, or the system containing pL, if
there is a non-empty selRange, unless pL is beforeFirstMeas, in which case system
is to be added before the system containing pL. If pL is in the final system, add
new system before the tail. If pL is before any system (pathological case, resulting
from Select All), add at the end of the score. */

LINK AddSysInsertPt(Document *doc, LINK pL, short *where)
{
	LINK insertL;
	
	/* If pL is a Page, BeforeFirstMeas will return True if pL is the first page
		of the score; else False. This is accidentally correct here: if we are
		before any system, BeforeFirstMeas returns True, we search right for a
		system, don't find one, and insert before the tail, which is what we want.
		Otherwise, we use the page itself, which does terminate the preceding system,
		which is also what we want. */

	if (BeforeFirstMeas(pL)) {

		/* Add new system before the current system which contains selStartL. Search
			back for the current system. If it is the first system of the page, if the
			sheetNum of its page is 0, we are adding before the first system of the score;
			else adding before the first system of some page following the first page of
			the score. */
		
		insertL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, False);
		if (!insertL)
			*where = SuccSystem;
		else if (FirstSysInPage(insertL))
			*where = SheetNUM(SysPAGE(insertL))!=0 ? FirstOnPage : BeforeFirstSys;
		else
			*where = SuccSystem;
	}
	else if (PageTYPE(pL)) {
		insertL = pL;
		*where = SuccSystem;
	}
	else {
		insertL = LSSearch(pL, SYSTEMtype, ANYONE, False, False);

		if (insertL)
			if (FirstSysInPage(insertL))			/* Found first sys of fllwng pg; return pg */
				insertL = SysPAGE(insertL);

		*where = SuccSystem;
	}

	return (insertL ? insertL : doc->tailL);
}


/* ----------------------------------------------------------- SysOverflowDialog -- */
/* Tell user there's no room to add a system to this page and ask what they want to
do. Return values are 0=Cancel, 1=reformat this page only, 2=reformat to the end. */

static enum {
	BUT1_OK=1,
	BUT2_Cancel,
	RAD4_RfmtPage=4,
	RAD5_RfmtScore
} ESysOverflow;

static short group1;

static short SysOverflowDialog(short);
static short SysOverflowDialog(short oldChoice)
{
	short itemHit;  short code;
	Boolean keepGoing=True;
	DialogPtr dlog;  GrafPtr oldPort;
	ModalFilterUPP filterUPP;

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP==NULL) {
		MissingDialog(SYSOVERFLOW_DLOG);
		return(0);
	}
	GetPort(&oldPort);
	dlog = GetNewDialog(SYSOVERFLOW_DLOG,NULL,BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(SYSOVERFLOW_DLOG);
		return(0);
	}
	SetPort(GetDialogWindowPort(dlog));

	/* Initialize dialog's values */

	group1 = (oldChoice==1? RAD4_RfmtPage : RAD5_RfmtScore);
	PutDlgChkRadio(dlog, RAD4_RfmtPage, (group1==RAD4_RfmtPage));
	PutDlgChkRadio(dlog, RAD5_RfmtScore, (group1==RAD5_RfmtScore));

	PlaceWindow(GetDialogWindow(dlog),(WindowPtr)NULL,0,80);
	ShowWindow(GetDialogWindow(dlog));

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP, &itemHit);
		switch(itemHit) {
			case BUT1_OK:
				keepGoing = False;
				code = (group1==RAD4_RfmtPage) ? 1 : 2;
				break;
			case BUT2_Cancel:
				keepGoing = False;
				code = 0;
				break;
			case RAD4_RfmtPage:
			case RAD5_RfmtScore:
				if (itemHit!=group1) SwitchRadio(dlog, &group1, itemHit);
				break;
			default:
				;
		}
	}
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
	
	return(code);
}


/* ------------------------------------------------------------------- AddSystem -- */
/* Add an empty System before <insertL>. <insertL> must be a Page, a System or
the tail; else there is an error. If there's no room for another system on the
page, ask user what to do; if they say to go ahead, or if there is enough room on
the page, add the system and update Measure and System numbers. Then move any
following Systems on the page down and, if the page overflows, reformat this and
perhaps following page. Returns the new System's LINK if success, else NILINK.
Assumes the specified document is in the active window. N.B. It's not clear if
<where> is correct in call to AddSystem from ReformatRaw. */

LINK AddSystem(Document *doc, LINK insertL, short where)
{
	LINK		newSysL,prevSysL,sysL,endRfmtL;
	DRect		invalDRect,sysRect;
	Rect		invalRect;
	DDIST		sysTop;
	static short rfmtChoice=1;

	if (!PageTYPE(insertL) && !SystemTYPE(insertL) && !TailTYPE(insertL)) {
		MayErrMsg("AddSystem: called with LINK %ld of bad type %ld",
					(long)insertL, (long)ObjLType(insertL));
		return NILINK;
	}

	DeselAll(doc);

	PrepareUndo(doc, insertL, U_AddSystem, 43);    /* "Add System" */

	if (RoomForSystem(doc, LeftLINK(insertL)))
		rfmtChoice = 0;
	else {
		rfmtChoice = SysOverflowDialog(rfmtChoice);
		if (rfmtChoice==0) return NILINK;
	}

	/* Add the new system before insertL */
	prevSysL = LSSearch(LeftLINK(insertL), SYSTEMtype, ANYONE, GO_LEFT, False);

	if (where == FirstSystem || where == BeforeFirstSys)
		sysTop = SYS_TOP(doc)+pt2d(config.titleMargin);
	else if (where == FirstOnPage)
		sysTop = SYS_TOP(doc);
	else {
		invalDRect = sysRect = SystemRECT(prevSysL);
		sysTop = sysRect.bottom;

		/* Make the inval rect as wide as the page. (Systems can have different l/r indents.) */
		invalDRect.left = 0;
		invalDRect.right = pt2d(doc->origPaperRect.right);

		OffsetDRect(&invalDRect,0,sysRect.bottom - sysRect.top);
		D2Rect(&invalDRect,&invalRect);
	}
	newSysL = CreateSystem(doc, LeftLINK(insertL), sysTop, where);

	if (newSysL!=NILINK) {

		/* Update numbers of all Systems in the entire document and Measures from here
			to the end. Then, if the page overflows, reformat; otherwise just move all
			following Systems on this page down. */

		UpdateSysNums(doc, doc->headL);
		UpdateMeasNums(doc, prevSysL);
		
		if (rfmtChoice!=0) {
			if (rfmtChoice==1) {
				sysL = newSysL;
				while (!IsLastSysInPage(sysL))
					sysL = LinkRSYS(sysL);
				endRfmtL = RightLINK(sysL);
			}
			else
				endRfmtL = doc->tailL;
			Reformat(doc, newSysL, endRfmtL, False, 9999, False, 999, config.titleMargin);
		}
		else if (!IsLastSysInPage(newSysL)) {
			DRect newSysR; long newPos; short hiWord;
			
			newSysR = SystemRECT(newSysL);
			hiWord = d2p(newSysR.bottom - newSysR.top);
			newPos = hiWord; newPos <<= 16;
			MoveSysOnPage(doc, newSysL, newPos);
		}

		if (where == FirstSystem || where == BeforeFirstSys || where == FirstOnPage)
			InvalWindow(doc);
		else {
			LINK pageL; Rect paperRect;
			
			pageL = LSSearch(newSysL,PAGEtype,ANYONE,GO_LEFT,False);
			GetSheetRect(doc,SheetNUM(pageL),&paperRect);
			OffsetRect(&invalRect,paperRect.left,paperRect.top);
			invalRect.left = paperRect.left;			/* Redraw from left edge of page */
			invalRect.bottom = paperRect.bottom;		/* Redraw to bottom of page */
			EraseAndInval(&invalRect);
		}

		doc->changed = doc->used = True;
		doc->selStartL = doc->selEndL = EndSystemSearch(doc, newSysL);
		MEAdjustCaret(doc, False);
		
	}
	return newSysL;
}


/* ------------------------------------------------------------------- InitParts -- */
/* Initialize default Part structures in score header or master page header. */

void InitParts(Document *doc, Boolean master)
{
	LINK partL;  PPARTINFO pPart;

	partL = (master? FirstSubLINK(doc->masterHeadL) : FirstSubLINK(doc->headL));
	InitPart(partL, NOONE, NOONE);		/* Dummy part--should never be used at all */
	pPart = GetPPARTINFO(partL);
	strcpy(pPart->name, "DUMMY");
	strcpy(pPart->shortName, "DUM.");
	
	partL = NextPARTINFOL(partL);
	InitPart(partL, 1, 2);
}


/* ------------------------------------------------------------------- MakeSystem -- */
/* Insert a new System after prevL in some object list belonging to doc. Return
new System's LINK, or NILINK. */

LINK MakeSystem(Document *doc, LINK prevL, LINK prevPageL, LINK prevSysL, DDIST sysTop,
					short where)
{
	LINK pL,nextSysL;  PSYSTEM pSystem;
	DDIST sysHeight, staffLength, indent;
	
	pL = InsertNode(doc, RightLINK(prevL), SYSTEMtype, 0);
	if (!pL) { NoMoreMemory(); return NILINK; }

	SetObject(pL, MARGLEFT(doc), sysTop, False, True, True);
	LinkTWEAKED(pL) = False;
	
	if (where==BeforeFirstSys)
		nextSysL = SSearch(RightLINK(pL), SYSTEMtype, False);
	else
		nextSysL = (prevSysL? LinkRSYS(prevSysL) : NILINK);

	SystemNUM(pL) = prevSysL ? SystemNUM(prevSysL)+1 : 1;
	
	LinkLSYS(pL) = prevSysL;
	LinkRSYS(pL) = nextSysL;
	if (prevSysL!=NILINK) LinkRSYS(prevSysL) = pL;
	if (nextSysL!=NILINK) LinkLSYS(nextSysL) = pL;
	
	SysPAGE(pL) = prevPageL;

	if (where==FirstSystem || where==BeforeFirstSys) {
		indent = doc->firstIndent;
		staffLength = MARGWIDTH(doc)-doc->firstIndent;
	}
	else {
		indent = doc->otherIndent;
		staffLength = MARGWIDTH(doc)-doc->otherIndent;
	}
	sysHeight = GetSysHeight(doc,pL,where);

	pSystem = GetPSYSTEM(pL);
	SetDRect(&pSystem->systemRect, MARGLEFT(doc)+indent, sysTop,
			MARGLEFT(doc)+indent+staffLength, sysTop+sysHeight);

	return pL;
}


/* ----------------------------------------------------------------- MakeStaff -- */
/* Insert a new Staff after prevL in some object list belonging to doc. Return
new Staff's LINK, or NILINK. Does not set the Staff's context fields: the
calling routine must do so.

Currently, this IGNORES the <staffTop> array parameter, and copies either the
previous Staff or the Master Page Staff! */

LINK MakeStaff(Document *doc,
					LINK prevL, LINK prevStaffL, LINK systemL,
					DDIST staffLength,
					short where,
					DDIST /*staffTop*/[])					/* unused */
{
	LINK pL, nextStaffL, aStaffL, copyStaffL;
	PASTAFF aStaff;
	
	if (where==FirstSystem) {
		pL = InsertNode(doc, RightLINK(prevL), STAFFtype, doc->nstaves);
		if (!pL) { NoMoreMemory(); return NILINK; }
	
		SetObject(pL, 0, 0, False, True, True);
		LinkTWEAKED(pL) = False;
		
		LinkLSTAFF(pL) = LinkRSTAFF(pL) = NILINK;
		StaffSYS(pL) = systemL;

		/* Create a default system with 2 default staves. */
	
		aStaffL = FirstSubLINK(pL);
		InitStaff(aStaffL, 1, initStfTop1, 0, staffLength, STHEIGHT, STFLINES, SHOW_ALL_LINES);
		aStaffL = NextSTAFFL(aStaffL);
		InitStaff(aStaffL, 2, initStfTop1+initStfTop2, 0, staffLength, STHEIGHT,
						STFLINES, SHOW_ALL_LINES);
		
		return pL;
	}

	/* Copy the previous Staff, if there is one, otherwise get it from Master Page. */
	
	if (where==BeforeFirstSys) copyStaffL = SSearch(doc->headL,STAFFtype,GO_RIGHT);
	else if (prevStaffL)		copyStaffL = prevStaffL;
	else						copyStaffL = SSearch(doc->masterHeadL,STAFFtype,GO_RIGHT);
	pL = DuplicateObject(STAFFtype, copyStaffL, False, doc, doc, False);
	if (!pL) { NoMoreMemory(); return NILINK; }

	/* ...but pay attention to the requested <staffLength>. */
	
	aStaffL = FirstSubLINK(pL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		aStaff->staffRight = staffLength;
	}
	
	InsNodeInto(pL,RightLINK(prevL));
	nextStaffL = SSearch(RightLINK(pL), STAFFtype, GO_RIGHT);
	
	LinkLSTAFF(pL) = prevStaffL;
	LinkRSTAFF(pL) = nextStaffL;
	if (prevStaffL) LinkRSTAFF(prevStaffL) = pL;
	if (nextStaffL) LinkLSTAFF(nextStaffL) = pL;
	
	StaffSYS(pL) = systemL;
	
	return pL;
}

/* ----------------------------------------------------------------- MakeConnect -- */
/* Insert a new Connect after prevL in some object list belonging to doc. Return
new Connect's LINK, or NILINK. N.B. If where=<FirstSystem>, assumes the system has
exactly one part of two staves!*/

LINK MakeConnect(Document *doc, LINK prevL, LINK prevConnectL, short where)
{
	LINK pL, aConnectL, copyConnL;
	PCONNECT pConnect;  register PACONNECT aConnect;
	
	if (where==FirstSystem) {
		pL = InsertNode(doc, RightLINK(prevL), CONNECTtype, 2);
		if (!pL) { NoMoreMemory(); return NILINK; }

		SetObject(pL, LinkXD(prevL)+pt2d(5), 0, False, True, True);
		LinkTWEAKED(pL) = False;
		pConnect = GetPCONNECT(pL);
		pConnect->connFiller = 0;
		
		aConnectL = FirstSubLINK(pL);
		aConnect = GetPACONNECT(aConnectL);
		aConnect->selected = False;
		aConnect->connLevel = SystemLevel;					/* Connect entire system */
		aConnect->connectType = CONNECTLINE;
		aConnect->staffAbove = NOONE;
		aConnect->staffBelow = NOONE;						/* Ignored if level=SystemLevel */
		aConnect->firstPart = NOONE;
		aConnect->lastPart = NOONE;
		aConnect->xd = 0;
		aConnect->filler = 0;
		
		aConnectL = NextCONNECTL(aConnectL);
		aConnect = GetPACONNECT(aConnectL);
		aConnect->selected = False;
		aConnect->connLevel = PartLevel;					/* Connect the part */
		aConnect->connectType = CONNECTCURLY;
		aConnect->staffAbove = 1;
		aConnect->staffBelow = 2;

		StoreConnectPart(doc->headL,aConnectL);	
		aConnect = GetPACONNECT(aConnectL);
		aConnect->xd = -ConnectDWidth(doc->srastral, CONNECTCURLY);
		aConnect->filler = 0;

		return pL;
	}

	/* Copy the previous Connect, if there is one, otherwise get it from Master Page. */

	if (where==BeforeFirstSys) copyConnL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);
	else if (prevConnectL)		copyConnL = prevConnectL;
	else						copyConnL = SSearch(doc->masterHeadL,CONNECTtype,GO_RIGHT);

	pL = DuplicateObject(CONNECTtype, copyConnL, False, doc, doc, False);
	if (!pL) { NoMoreMemory(); return NILINK; }
	
	InsNodeInto(pL,RightLINK(prevL));
	return pL;
}

/* ------------------------------------------------------ MakeClef/KeySig/TimeSig -- */

static LINK MakeClef(
				Document *doc,
				LINK prevL,
				short where,
				CONTEXT context[],
				DDIST spBefore)		/* Taken as clef's position, since nothing precedes it */
{
	LINK pL, prevClefL, aClefL;  short i;

	/* Create and fill in the Clef object. */

	if (where==BeforeFirstSys) {
		prevClefL = SSearch(prevL, CLEFtype, False);

		pL = DuplicateObject(CLEFtype, prevClefL, False, doc, doc, False);
		if (!pL) { NoMoreMemory(); return NILINK; }
		
		InsNodeInto(pL,RightLINK(prevL));
		return pL;
	}

	pL = InsertNode(doc, RightLINK(prevL), CLEFtype, doc->nstaves);
	if (pL==NILINK) { NoMoreMemory(); return NILINK; }

	SetObject(pL, spBefore, 0, False, True, True);
	ClefINMEAS(pL) = False;
	LinkTWEAKED(pL) = False;

	aClefL = FirstSubLINK(pL);
	if (where==FirstSystem) {
		/* Initialize the clefs in standard way, regardless of staff context */
		InitClef(aClefL, 1, 0, TREBLE_CLEF);
		aClefL = NextCLEFL(aClefL);
		InitClef(aClefL, 2, 0, BASS_CLEF);					/* Exceptional initial clef */
	}
	else 		
		for (i = 1; i<=doc->nstaves; i++, aClefL=NextCLEFL(aClefL))
			InitClef(aClefL, i, 0, context[i].clefType);

	return pL;
}


static LINK MakeKeySig(Document *doc, LINK prevL, short where, CONTEXT context[],
							short *maxSofonStf, DDIST spBefore)
{
	LINK pL, prevKeySigL, aKeySigL;  PAKEYSIG aKeySig;
	short i, k, maxSofon=0, maxSofonStaff=0;

	/* Create and fill in the KeySig object. */

	if (where==BeforeFirstSys) {
		prevKeySigL = SSearch(prevL, KEYSIGtype, False);

		pL = DuplicateObject(KEYSIGtype, prevKeySigL, False, doc, doc, False);
		if (!pL) { NoMoreMemory(); return NILINK; }

		InsNodeInto(pL,RightLINK(prevL));
	
		aKeySigL = FirstSubLINK(pL);
		for (i = 1; i<=doc->nstaves; i++,aKeySigL = NextKEYSIGL(aKeySigL)) {
			aKeySig = GetPAKEYSIG(aKeySigL);
			if (aKeySig->nKSItems>maxSofon) {
				maxSofonStaff = i;
				maxSofon = aKeySig->nKSItems;
			}
		}

		*maxSofonStf = maxSofonStaff;
		return pL;
	}

	pL = InsertNode(doc, RightLINK(prevL), KEYSIGtype, doc->nstaves);
	if (pL==NILINK) { NoMoreMemory(); return NILINK; }
	SetObject(pL, LinkXD(prevL)+spBefore, 0, False, True, True);
	LinkTWEAKED(pL) = False;
	KeySigINMEAS(pL) = False;
	if (where==FirstSystem) {
		aKeySigL = FirstSubLINK(pL);
		InitKeySig(aKeySigL, 1, 0, 0);
		aKeySigL = NextKEYSIGL(aKeySigL);
		InitKeySig(aKeySigL, 2, 0, 0);
	}
	else {
		aKeySigL = FirstSubLINK(pL);
		for (i = 1; i<=doc->nstaves; i++) {
			InitKeySig(aKeySigL, i, 0, 0);
			aKeySig = GetPAKEYSIG(aKeySigL);
			aKeySig->nKSItems = context[i].nKSItems;
			if (aKeySig->nKSItems>maxSofon) {
				maxSofonStaff = i;
				maxSofon = aKeySig->nKSItems;
			}
			for (k = 0; k<aKeySig->nKSItems; k++)
				aKeySig->KSItem[k] = context[i].KSItem[k];
			aKeySig->visible = (aKeySig->nKSItems!=0);			/* N.B. No sharps/flats is invisible! */
			aKeySigL = NextKEYSIGL(aKeySigL);
		}
	}

	*maxSofonStf = maxSofonStaff;

	return pL;
}


static LINK MakeTimeSig(Document *doc, LINK prevL, short where, CONTEXT context[],
							DDIST spBefore)
{
	LINK pL, prevTimeSigL, aTimeSigL;

	if (where==BeforeFirstSys) {
		prevTimeSigL = SSearch(prevL, TIMESIGtype, False);

		pL = DuplicateObject(TIMESIGtype, prevTimeSigL, False, doc, doc, False);
		if (!pL) { NoMoreMemory(); return NILINK; }

		InsNodeInto(pL,RightLINK(prevL));
		return pL;
	}

	pL = InsertNode(doc, RightLINK(prevL), TIMESIGtype, doc->nstaves);
	if (pL==NILINK) { NoMoreMemory(); return NILINK; }
	SetObject(pL, LinkXD(prevL)+spBefore, 0, False, True, True);
	TimeSigINMEAS(pL) = False;
	LinkTWEAKED(pL) = False;

	aTimeSigL = FirstSubLINK(pL);
	InitTimeSig(aTimeSigL, 1, 0, context[1].timeSigType,
				context[1].numerator,
				context[1].denominator);
	aTimeSigL = NextTIMESIGL(aTimeSigL);
	InitTimeSig(aTimeSigL, 2, 0, context[2].timeSigType,
				context[2].numerator,
				context[2].denominator);

	return pL;
}

/* ------------------------------------------------------------------- MakeMeasure -- */
/* Insert a new Measure after prevL in an object list belonging to doc. Deliver new
Measure's LINK if it succeeds, else NILINK. Does not depend on validity of cross-links.
Does not set the Measure's context fields: the calling routine must do so. */

LINK MakeMeasure(Document *doc, LINK prevL, LINK prevMeasL, LINK staffL, LINK systemL,
					DDIST spBefore, DDIST staffTop[], short where)
{
	LINK pL, nextMeasL, aMeasureL, aPrevMeasL, partL, connectL, aConnectL,
			endMeasL, aStaffL;
	PMEASURE pMeasure; 
	PPARTINFO pPart;  PACONNECT aConnect;
	short i, j, connStaff, measureNum;  Boolean connAbove;
	DDIST mTop, mBottom, sysHeight, staffLength, xd;
	DRect sysRect;
	
	pL = InsertNode(doc, RightLINK(prevL), MEASUREtype, doc->nstaves);
	if (pL==NILINK) { NoMoreMemory(); return NILINK; }

	if (where==BeforeFirstSys)
		nextMeasL = SSearch(RightLINK(pL), MEASUREtype, False);
	else
		nextMeasL = (prevMeasL? LinkRMEAS(prevMeasL) : NILINK);

	LinkLMEAS(pL) = prevMeasL;
	LinkRMEAS(pL) = nextMeasL;
	if (prevMeasL!=NILINK) LinkRMEAS(prevMeasL) = pL;
	if (nextMeasL!=NILINK) LinkLMEAS(nextMeasL) = pL;
	
	MeasSYSL(pL) = systemL;
	MeasSTAFFL(pL) = staffL;

	xd = (where==SuccMeas? LinkXD(prevMeasL)+spBefore : LinkXD(prevL)+spBefore);
	SetObject(pL, xd, 0, False, False, True);
	LinkTWEAKED(pL) = False;
	pMeasure = GetPMEASURE(pL);
	SetRect(&pMeasure->measureBBox, 0, 0, 0, 0);			/* Will be computed when it's drawn */ 
	pMeasure->spacePercent = doc->spacePercent;
	
	if (where==FirstSystem || where==BeforeFirstSys)
		MeasureTIME(pL) = 0L;
	else if (prevMeasL!=NILINK) {
		endMeasL = EndMeasSearch(doc, prevMeasL);
		MeasureTIME(pL) = MeasureTIME(prevMeasL)+GetMeasDur(doc, endMeasL, ANYONE);
	}
	else
		MeasureTIME(pL) = 7734L;							/* Should never happen */

	sysRect = SystemRECT(systemL);
	sysHeight = sysRect.bottom-sysRect.top;

	aMeasureL = FirstSubLINK(pL);
	if (where==FirstSystem) {
		staffLength = MARGWIDTH(doc)-doc->firstIndent;
		InitMeasure(aMeasureL, 1, pt2d(0), pt2d(0), staffLength-LinkXD(pL),
									initStfTop1+initStfTop2, False, False, 2, 0);
		aMeasureL = NextMEASUREL(aMeasureL);
		InitMeasure(aMeasureL, 2, pt2d(0), initStfTop1+STHEIGHT, staffLength-LinkXD(pL),
									sysHeight, False, True, 0, 0);
	}
	else {
		staffLength = MARGWIDTH(doc)-doc->otherIndent;
		for (i = 1; i<=doc->nstaves; i++, aMeasureL=NextMEASUREL(aMeasureL)) {

			partL=FirstSubLINK(doc->headL);
			for (j=0; partL; j++,partL=NextPARTINFOL(partL))
				if (j==Staff2Part(doc, i)) break;

			pPart = GetPPARTINFO(partL);
			connAbove = i>pPart->firstStaff;
			connStaff = (i==pPart->firstStaff && pPart->lastStaff>pPart->firstStaff) ?
									pPart->lastStaff : 0;

			if (i==pPart->firstStaff) {
				connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);
				aConnectL = FirstSubLINK(connectL);
				for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {	
					aConnect = GetPACONNECT(aConnectL);
					if (aConnect->connLevel==GroupLevel &&
							aConnect->staffAbove==pPart->firstStaff &&
							aConnect->staffBelow>=pPart->lastStaff) {

						connStaff = aConnect->staffBelow;
						break;
					}
				}
				aConnectL = FirstSubLINK(connectL);
				for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {	
					aConnect = GetPACONNECT(aConnectL);
					if (aConnect->connLevel==GroupLevel &&
							aConnect->staffAbove<pPart->firstStaff &&
							aConnect->staffBelow>=pPart->lastStaff) {
						connAbove = True;
						break;
					}
				}
			}
			
			aStaffL = StaffOnStaff(staffL,i);
			if (StaffVIS(aStaffL)) {
				mTop = (i==1) ? pt2d(0) : staffTop[NextLimStaffn(doc,pL,False,i-1)]+STHEIGHT;
				mBottom = (i==doc->nstaves) ? sysHeight : staffTop[NextLimStaffn(doc,pL,True,i+1)];
			}
			else
				mTop = mBottom = 0;

			if (prevMeasL!=NILINK) {
				aPrevMeasL = FirstSubLINK(prevMeasL);
				 measureNum = MeasMEASURENUM(aPrevMeasL)+1;
			}
			else
				measureNum = 0;
			InitMeasure(aMeasureL, i, pt2d(0), mTop, staffLength-LinkXD(pL), mBottom,
							False, connAbove, connStaff, measureNum);
		}
	}

	return pL;
}

/* ---------------------------------------------------------- CreateSysFixContext -- */

static void CreateSysFixContext(Document *doc, LINK staffL, LINK measL, short where)
{
	LINK aStaffL,aMeasL,theMeasL;  CONTEXT theContext;  short i;

	/* Fill in clef/key sig./time sig. context fields of the initial Staff.
		See comments on horrible way to establish context in CreateSystem. */

	aStaffL = FirstSubLINK(staffL);
	if (where==FirstSystem) {
		GetContext(doc, LeftLINK(staffL), 1, &theContext);
		FixStaffContext(aStaffL, &theContext);
		aStaffL = NextSTAFFL(aStaffL);
		GetContext(doc, LeftLINK(staffL), 2, &theContext);
		FixStaffContext(aStaffL, &theContext);
	}
	else if (where==BeforeFirstSys) {

		/* Update the context of the staff object in the system which
			was added. */
		for (i=1; i<=doc->nstaves; i++, aStaffL=NextSTAFFL(aStaffL)) {
			GetContext(doc, LinkRSTAFF(staffL), StaffSTAFF(aStaffL), &theContext);
			FixStaffContext(aStaffL, &theContext);
		}

		/* Update the context for the staff of the system before which
			the new system was added. */
		aStaffL = FirstSubLINK(LinkRSTAFF(staffL));
		for (i=1; i<=doc->nstaves; i++, aStaffL=NextSTAFFL(aStaffL)) {
			theMeasL = SSearch(LinkRSTAFF(staffL), MEASUREtype, False);
			GetContext(doc, theMeasL, StaffSTAFF(aStaffL), &theContext);
			FixStaffContext(aStaffL, &theContext);
		}

	}
	else
		for (i=1; i<=doc->nstaves; i++, aStaffL=NextSTAFFL(aStaffL)) {
			GetContext(doc, LeftLINK(staffL), StaffSTAFF(aStaffL), &theContext);
			FixStaffContext(aStaffL, &theContext);
		}

	/* Fill in clef/key sig./time sig. context fields of the initial Measure. */

	aMeasL = FirstSubLINK(measL);
	if (where==FirstSystem) {
		GetContext(doc, LeftLINK(measL), 1, &theContext);
		FixMeasureContext(aMeasL, &theContext);
		aMeasL = NextMEASUREL(aMeasL);
		GetContext(doc, LeftLINK(measL), 2, &theContext);
		FixMeasureContext(aMeasL, &theContext);
	}
	else if (where==BeforeFirstSys)
		for (i=1; i<=doc->nstaves; i++, aMeasL=NextMEASUREL(aMeasL)) {
			GetContext(doc, LinkRMEAS(measL), MeasureSTAFF(aMeasL), &theContext);
			FixMeasureContext(aMeasL, &theContext);
		}
	else
		for (i=1; i<=doc->nstaves; i++, aMeasL=NextMEASUREL(aMeasL)) {
			GetContext(doc, LeftLINK(measL), MeasureSTAFF(aMeasL), &theContext);
			FixMeasureContext(aMeasL, &theContext);
		}
}


/* ---------------------------------------------------------------- CreateSystem -- */
/* Create a new System, with a minimal legal set of objects in it--Staff, Connect,
Clef, KeySig, TimeSig, Measure--and link it into the object list after prevL.
The Clef, KeySig, and TimeSig are properly initialized from the context. If there is
anything following the new objects, the calling routine is responsible for updating
it appropriately, aside from links: CreateSystem does not update system numbers,
coordinates, etc. Delivers LINK to the new System. Uses enum following comment
for <where> to determine where to add the system. N.B. If <FirstSystem>, assumes
the system is exactly one part of two staves!
*/

LINK CreateSystem(Document *doc, LINK prevL, DDIST sysTop, short where)
{
	register LINK pL;
	LINK 		qPageL, systemL, qSystemL, staffL, qStaffL,
				qConnectL, qMeasureL, timeSigL, aStaffL;
	DDIST		dLineSp, staffLength, spBefore;
	short		i, maxSofonStaff;
	CONTEXT		context[MAXSTAVES+1];
	DDIST 		staffTop[MAXSTAVES+1];

	qPageL = LSSearch(prevL, PAGEtype, ANYONE, True, False);

	if (where==FirstSystem) {
		qSystemL = qStaffL = qMeasureL = qConnectL = NILINK;
		staffLength = MARGWIDTH(doc)-doc->firstIndent;
		doc->nstaves = 2;
		doc->nsystems = 1;
		InitParts(doc, False);
	}
	else if (where==BeforeFirstSys) {
		qSystemL = qStaffL = qMeasureL = qConnectL = NILINK;
		staffLength = MARGWIDTH(doc)-doc->firstIndent;
		FillStaffTopArray(doc, doc->headL, staffTop);
		doc->nsystems++;
	}
	else {
		qSystemL = LSSearch(prevL, SYSTEMtype, ANYONE, True, False);
		qStaffL = LSSearch(prevL, STAFFtype, ANYONE, True, False);
		qMeasureL = LSSearch(prevL, MEASUREtype, ANYONE, True, False);
		qConnectL = LSSearch(prevL, CONNECTtype, ANYONE, True, False);
		staffLength = MARGWIDTH(doc)-doc->otherIndent;
		FillStaffTopArray(doc, doc->headL, staffTop);
		doc->nsystems++;
	}

	/* Create and fill in the System object. */

	pL = MakeSystem(doc, prevL, qPageL, qSystemL, sysTop, where);
	if (pL==NILINK) return NILINK;
	systemL = pL;
	SystemRECT(systemL).right += ExtraSysWidth(doc);		/* Allow for right-just. barlines */

	/* Create and fill in the Staff object. NB: currently ignores <staffTop>! */

	pL = MakeStaff(doc, pL, qStaffL, systemL, staffLength, where, staffTop);
	if (pL==NILINK) return NILINK;
	staffL = pL;

	/* Create and fill in the Connect object. */

	pL = MakeConnect(doc, pL, qConnectL, where);
	if (pL==NILINK) return NILINK;

	/* GetContext needs legal staves to get the context on; but the staves don't yet
		have any context info to give to GetContext, so we start GetContext to the left
		of the staff object so it gets context info from headL.
		This is a horrible way to establish initial context: e.g. for clefType there
		are no explicit fields in the header so it is left up to GetContext to fill in
		DEFLT_CLEF by default, and there isn't even any comment to indicate that this
		is the way it is being done. */
	
 	if (where==FirstSystem) {
		GetContext(doc, LeftLINK(staffL), 1, &context[1]);
		GetContext(doc, LeftLINK(staffL), 2, &context[2]);
		context[2].clefType = BASS_CLEF;
	}
	else if (where==BeforeFirstSys)
		for (i = 1; i<=doc->nstaves; i++)
			GetContext(doc, LinkRSTAFF(staffL), i, &context[i]);
	else
		for (i = 1; i<=doc->nstaves; i++)
			GetContext(doc, LeftLINK(staffL), i, &context[i]);

	dLineSp = STHEIGHT/(STFLINES-1);							/* Space between staff lines */

	/* Create and fill in the Clef and KeySig objects. */

	pL = MakeClef(doc, pL, where, context, dLineSp);
	if (pL==NILINK) return NILINK;

	/* According to Ross, p. 145, if there's a key signature, its left edge should be
		3 and 1/2 spaces after the left edge of the clef; if not, the time signature's
		left edge should be at the same point. */
		
	pL = MakeKeySig(doc, pL, where, context, &maxSofonStaff, 7*dLineSp/2);
	if (pL==NILINK) return NILINK;

	if (maxSofonStaff!=0) {
		spBefore = dLineSp/2;
		spBefore += std2d(SymWidthRight(doc, pL, maxSofonStaff, False), STHEIGHT, 5);
	}
	else
		spBefore = 0;

	/* Create and fill in the TimeSig object, but only if this is first System of score.
		If we are inserting beforeFirstSys, add TimeSig to this system, and remove the
		one from the previous first system. */

	if (where==FirstSystem || where==BeforeFirstSys) {
		pL = MakeTimeSig(doc, pL, where, context, spBefore);
		if (pL==NILINK) return NILINK;
		spBefore = 3*dLineSp;
	}

	/* Create and fill in the Measure object. */

	pL = MakeMeasure(doc, pL, qMeasureL, staffL, systemL, spBefore, staffTop, where);
	if (pL==NILINK) return NILINK;	
	
	if (where==FirstSystem) 
		InitObject(doc->tailL, pL, NILINK, 0xFFFF, 0, False, False, True);	/* xd will be set by UpdateTailxd */
	
	if (where==BeforeFirstSys) 
		(void)ChangeSysIndent(doc, LinkRSYS(systemL), -doc->firstIndent);
	
	CreateSysFixContext(doc, staffL, pL, where);
	
	/* Must not set staff's clefType until here, or it will be overwritten by
		CreateSysFixContext */

 	if (where==FirstSystem) {
		for (aStaffL=FirstSubLINK(staffL); aStaffL; aStaffL=NextSTAFFL(aStaffL))
			if (StaffSTAFF(aStaffL)==2)
				{ StaffCLEFTYPE(aStaffL) = BASS_CLEF; break; }
	}

	/* Delete the timeSig in the system which was once the first system. Don't
		delete it until now so as to preserve context information until all context
		updating is done. */

	if (where==BeforeFirstSys) {
		timeSigL = SSearch(LinkRSYS(systemL), TIMESIGtype, False);
		DeleteNode(doc, timeSigL);
	}
	return systemL;
}


/* ------------------------------------------------------------- AddPageInsertPt -- */
/* Return an insertion point for add page, when the caller wishes to add a
page at pL. AddPage must be called with the page or tail object before
which to add the page.
Assumes page is to be added after the page containing pL (before the page
following the page containing pL), unless pL is before the first measure of
the first system in its page, in which case page is to be added before the
page containing pL. If pL is in the final page, add new page before the tail.
If pL is before any system or any page (pathological case, resulting from
Select All), add at the end of the score. */

LINK AddPageInsertPt(Document *doc, LINK pL)
{
	LINK insertL, sysL;

	sysL = LSSearch(pL, SYSTEMtype, ANYONE, True, False);
	if (!sysL) return doc->tailL;

	if (BeforeFirstMeas(pL) && FirstSysInPage(sysL))
			insertL = LSSearch(pL, PAGEtype, ANYONE, True, False);
	else	insertL = LSSearch(pL, PAGEtype, ANYONE, False, False);

	return (insertL ? insertL : doc->tailL);
}


/* --------------------------------------------------------------------- AddPage -- */
/* Add an empty page before <insertL>. <insertL> must be a page or the tail; else
there is an error. Update measure, system, and page numbers. Returns the new Page's
LINK if success, else NILINK.

Assumes the specified document is in the active window. */

LINK AddPage(Document *doc, LINK insertL)
{
	LINK newPageL, prevPageL;  Rect paper, result;  short pageNum;
	Boolean appending;  static Boolean alreadyWarned=False;

	if (!PageTYPE(insertL) && !TailTYPE(insertL)) {
		MayErrMsg("AddPage: called with LINK %ld of bad type %ld",
					(long)insertL, (long)ObjLType(insertL));
		return NILINK;
	}

	if (ScreenPagesCanOverflow(doc, doc->magnify, doc->numSheets+1)) {
		GetIndCString(strBuf, MISCERRS_STRS, 25);			/* "The page will not be added." */
		CParamText(strBuf, "", "", "");
		WarnScreenPagesOverflow(doc);
		return NILINK;
	}

	appending = TailTYPE(insertL);
	DeselAll(doc);

	/* Add the new page before insertL */
	newPageL = CreatePage(doc, LeftLINK(insertL));
	if (newPageL!=NILINK) {
		if (ScreenPagesExceedView(doc) && !alreadyWarned) {
			CautionInform(MANYPAGES_ALRT);
			alreadyWarned = True;
		}

		UpdateSysNums(doc, doc->headL);
		UpdatePageNums(doc);
		prevPageL = LinkLPAGE(newPageL);
		UpdateMeasNums(doc, (prevPageL? LeftLINK(prevPageL) : NILINK));

		GetAllSheets(doc);
		RecomputeView(doc);
		MEAdjustCaret(doc,False);
		
		/* FIXME: The following has a problem: if GetSheetRect doesn't return
			INARRAY_INRANGE, <paper> is meaningless. In that case, the new page is
			off-screen, and we should probably unconditionally ScrollToPage. */

		GetSheetRect(doc,doc->numSheets-1,&paper);
		InsetRect(&paper, -2, -2);
		InvalWindowRect(doc->theWindow,&paper);
		DisableUndo(doc, False);
		
		doc->changed = doc->used = True;
		doc->selStartL = doc->selEndL =								
			LinkRPAGE(newPageL) ? LinkRPAGE(newPageL) : doc->tailL;
		SectRect(&paper, &doc->viewRect, &result);
		if (!appending || !EqualRect(&paper, &result)) {
			pageNum = SheetNUM(newPageL)+doc->firstPageNumber;
			ScrollToPage(doc, pageNum);
		}
	}
	return newPageL;
}


/* ------------------------------------------------------------------ CreatePage -- */
/* Create a new Page and link it into the object list AFTER prevL. */

LINK CreatePage(Document *doc, LINK prevL)
{
	LINK	pageL, lPage, rPage, sysL;
	
	lPage = LSSearch(prevL, PAGEtype, ANYONE, True, False);
	rPage = LSSearch(RightLINK(prevL), PAGEtype, ANYONE, False, False);

	pageL = InsertNode(doc, RightLINK(prevL), PAGEtype, 0);
	if (pageL==NILINK) { NoMoreMemory(); return NILINK; }
	SetObject(pageL, 0, 0, False, True, True);
	LinkTWEAKED(pageL) = False;

	LinkLPAGE(pageL) = lPage;
	if (lPage) LinkRPAGE(lPage) = pageL;

	if (rPage) LinkLPAGE(rPage) = pageL;
	LinkRPAGE(pageL) = rPage;

	sysL = LSSearch(pageL, SYSTEMtype, ANYONE, True, False);
	doc->currentSystem = sysL ? SystemNUM(sysL) : 1;
	SheetNUM(pageL) = lPage ? SheetNUM(lPage)+1 : 0;
	doc->numSheets++;

	if (lPage)
		CreateSystem(doc, pageL, SYS_TOP(doc), FirstOnPage);
	else
		CreateSystem(doc, pageL, SYS_TOP(doc)+pt2d(config.titleMargin), BeforeFirstSys);
	doc->selStartL = doc->selEndL =
						LinkRPAGE(pageL) ? LinkRPAGE(pageL) : doc->tailL;
						
	InvalWindow(doc);
	return pageL;
}


/* ----------------------------------------------------------- Functions for GoTo -- */

void ScrollToPage(Document *doc, short pageNum)
{
	short sheetNum,x,y;  Rect sheet;  Boolean inval;
	
	sheetNum = pageNum - doc->firstPageNumber;
	
 	/* Scroll to upper left corner of sheet the user picked */
 	
	inval = (GetSheetRect(doc,sheetNum,&sheet)!=INARRAY_INRANGE);
	if (inval) {
		/* Sheet is not in visible array or is so far away its coords. overflow, so we
			have to change array origin */
		doc->firstSheet = sheetNum;
		if (doc->firstSheet < 0) doc->firstSheet = 0;
		if (doc->firstSheet >= doc->numSheets) doc->firstSheet = doc->numSheets-1;
		GetAllSheets(doc);
		RecomputeView(doc);
		GetSheetRect(doc,sheetNum,&sheet);
		}
	x = sheet.left - config.hPageSep;
	y = sheet.top  - config.vPageSep;
	
	QuickScroll(doc,x,y,False,!inval);
	if (inval) InvalWindowRect(doc->theWindow,&doc->viewRect);
	doc->currentSheet = sheetNum;
	GetSheetRect(doc,sheetNum,&doc->currentPaper);
	/*  FIXME: PLACE INS. POINT AT TOP OF PAGE & MOVE CARET THERE WITH MEAdjustCaret() */
	DrawMessageBox(doc, True);
}

static void ScrollToLink(Document *doc, LINK pL)
{
	Point pt;  short x, y;

	if (pL!=NILINK) {
		pt = LinkToPt(doc, pL, True);
		/* Keep coordinates multiples of 8 so that background pattern doesn't shift */
		x = pt.h - (pt.h & 7);
		y = pt.v - (pt.v & 7);
		/* Place destination in upper center of view */
		x -= (doc->viewRect.right - doc->viewRect.left) / 2;
		y -= (doc->viewRect.bottom - doc->viewRect.top) / 4;
		QuickScroll(doc,x,y,False,True);
	}
}

void GoTo(Document *doc)
{
	short pageNum, newPageNum, measNum, gotoType;  PGRAPHIC pMark;
	LINK measL, markL, newPageL;  Boolean beforeFirst;
	
	pageNum = doc->currentSheet+doc->firstPageNumber;

	/* Search right for a measure if selStartL is before the first measure,
		else search left */
	beforeFirst = BeforeFirstMeas(doc->selStartL);
	measL = MNSearch(doc, doc->selStartL, ANYONE, !beforeFirst, True);
	measNum = GetPAMEASURE(FirstSubLINK(measL))->measureNum+doc->firstMNNumber;

	markL = RMSearch(doc, doc->selStartL, "\p", True);

	gotoType = GoToDialog(doc,&pageNum,&measNum,&markL);
	if (gotoType == goDirectlyToJAIL) return; 
	DoUpdate(doc->theWindow);
	
	switch (gotoType) {
		case gotoPAGE:
			ScrollToPage(doc, pageNum);
			break;
		case gotoBAR:
			measL = MNSearch(doc, doc->headL, measNum-doc->firstMNNumber, GO_RIGHT, True);
			newPageL = SysPAGE(MeasSYSL(measL));
			newPageNum = SheetNUM(newPageL)+doc->firstPageNumber;
			if (newPageNum!=pageNum) {
				ScrollToPage(doc, newPageNum);
			}
			ScrollToLink(doc, measL);
			break;
		case gotoREHEARSALMARK:
			if (markL!=NILINK) {
				pMark = GetPGRAPHIC(markL);
				ScrollToLink(doc, pMark->firstObj);
			}
			break;
		default:
			;
	}
	/*
	 * Set initial zoom origin to a point that's not on any page so SheetMagnify
	 * will just use the current page.
	 */
	doc->scaleCenter.h = doc->scaleCenter.v = SHRT_MAX;
}


void GoToSel(Document *doc)
{
	short pageNum;  LINK pageL;  Rect sheet;
	
	/* if (doc->selStartL in view) should maybe just return; on the other hand,
		this way, it "touches up" the display, which unfortunately may be useful. */

	pageL = EitherSearch(doc->selStartL, PAGEtype, ANYONE, GO_LEFT, False);
	pageNum = SheetNUM(pageL)+doc->firstPageNumber;
	if (GetSheetRect(doc,SheetNUM(pageL),&sheet)==NOT_INARRAY)
		ScrollToPage(doc, pageNum);

	ScrollToLink(doc, doc->selStartL);
}


/* ---------------------------------------------------------------- FindPartInfo -- */

LINK FindPartInfo(Document *doc, short partn)
{
	short 	np;
	LINK	partL;
	
	if (partn>LinkNENTRIES(doc->headL)) return NILINK;
	
	for (np = 0, partL = FirstSubLINK(doc->headL); np<partn; 
			np++, partL = NextPARTINFOL(partL))	
		;
	return partL;
}
