/* MasterPage.c for Nightingale
 * Functions for creating and manipulating the Master Page - OMRAS MemMacroized.
 * 
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* -------------------------------------------------------------------------------------- */
/* Local prototypes */

static void SetStaffLengths(Document *, LINK);
static void SetAllStaffLengths(Document *, LINK, LINK);
static void UpdateSystemRects(Document *, LINK);

static Boolean DanglingContents(Document *);

static short MaxSysOnPage(Document *);
static Boolean DanglingSystems(Document *);
static Boolean SysVChanged(Document *);
static Boolean SysHChanged(Document *);
static Boolean StaffChanged(Document *);
static Boolean MarginVChanged(Document *);
static Boolean MarginHChanged(Document *);
static Boolean IndentChanged(Document *);
static Boolean RastralChanged(Document *);

static void MPFixSystemRectYs(Document *);
static void MPFixSystemRectXs(Document *);

static void ConformRanges(Document *);
static void SetStaffSizeMP(Document *);
static void SetStaffLinesMP(Document *);
static void ConformLeftEnds(Document *);
static void FixSelStaff(Document *);

static void ChangeStaffTops(Document *, LINK, LINK, long);
static void ResetMasterFields(Document *doc);
static Boolean AnyMPChanged(Document *doc);
static void ResetMasterPage(Document *doc);
static short GetExportAlert(Document *doc);
static void DisposeMasterData(Document *doc);

static Boolean MPChkSysHt(Document *doc, short srastral, short newRastral);

static LINK CopyMasterPage(Document *doc, LINK headL, LINK *newTailL);

static LINK MakeNewPage(Document *);
static LINK MakeNewSystem(Document *, LINK, LINK, DDIST);
static void SetMasterPParts(Document *);
static LINK MakeNewStaff(Document *, LINK, LINK, DDIST);
static LINK MakeNewConnect(Document *, LINK);


/* --------------------------------------------------------- General Editing utilities -- */

/* Update the staffTop field of all staff subObjects in range [headL, tailL). Set all
staffTops to equal the corresponding value in the staffTop array allocated by Master
Page & indexed by staffn. FIXME: Should call UpdateStaffTops. */

void MPUpdateStaffTops(Document *doc, LINK headL, LINK tailL)
{
	LINK pL, aStaffL;
	
	for (pL=headL; pL!=tailL; pL=RightLINK(pL))
		if (StaffTYPE(pL)) {
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
				StaffTOP(aStaffL) = doc->staffTopMP[StaffSTAFF(aStaffL)];
			}
		}
}


/* Update the staffRight field of subObjects of staffL to reflect the current position
position of the rightMargin of doc. Should be called in response to editing or other
change of document marginRect. */

static void SetStaffLengths(Document *doc, LINK staffL)
{
	LINK aStaffL, sysL; PSYSTEM pSystem;
	
	sysL = StaffSYS(staffL);
	pSystem = GetPSYSTEM(sysL);

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		StaffRIGHT(aStaffL) = pt2d(doc->marginRect.right) - pSystem->systemRect.left;
	}
}


/* Set the staffLengths of all staves in range [headL, tailL) to reflect the current
rightMargin of doc. */

static void SetAllStaffLengths(Document *doc, LINK headL, LINK tailL)
{
	LINK pL;

	for (pL=headL; pL!=tailL; pL=RightLINK(pL))
		if (StaffTYPE(pL))
			SetStaffLengths(doc, pL);
}


/* Fix System Rect inside the Master Page object list itself. To be done after dragging
the last staff up or down, in order to keep systemRect consistent with the new position
of the staves, or after adding/removing parts or changing staff size. dv is the amount
by which the height of the System Rect should change. */

void UpdateMPSysRectHeight(Document *doc, DDIST dv)
{
	LINK sysL;

	sysL = SSearch(doc->masterHeadL, SYSTEMtype, False);

	SystemRECT(sysL).bottom += dv;

	LinkOBJRECT(sysL).bottom += d2p(dv);
}


/* Update the systemRect of sysL to reflect doc->marginRect; should be called in
response to editing or other change of doc->marginRect. */

static void UpdateSystemRects(Document *doc, LINK sysL)
{
	DDIST sysLeft; PSYSTEM pSystem; short dv;

	/* Update the system left. */
	sysLeft = MARGLEFT(doc) + doc->dIndentOther;
	pSystem = GetPSYSTEM(sysL);
	pSystem->systemRect.left = sysLeft;
	
	/* Update the system top & bottom. */
	dv = SYS_TOP(doc) - pSystem->systemRect.top;
	OffsetDRect(&pSystem->systemRect, 0, dv);

	/* Update the system right. */
	pSystem->systemRect.right = pt2d(doc->marginRect.right);
	pSystem->systemRect.right += ExtraSysWidth(doc);
}


/* ------------------------------------------------- Routines for Exporting MasterPage -- */

/* Return True if exporting Master Page will result in contents dangling off the end of
one or more systems; this can happen if user has edited marginRect.right or left, or
staff rastral size. */

#define DANGLING_SLOP pt2d(1)

static Boolean DanglingContents(Document *doc)
{
	LINK sysL, endL, masterSysL;
	DDIST endxd, rMargDiff;
	FASTFLOAT endxdf, sizeRatio;

	sysL = SSearch(doc->headL, SYSTEMtype, GO_RIGHT);
	masterSysL = SSearch(doc->masterHeadL, SYSTEMtype, GO_RIGHT);
	
	for ( ; sysL; sysL = LinkRSYS(sysL)) {

		/* Get xd of last obj in system, incorporating changes to staff rastral
		   size, and adding sysRect.left of the system */

		endL = LastObjInSys(doc, RightLINK(sysL));
		sizeRatio = (FASTFLOAT)drSize[doc->srastralMP]/drSize[doc->srastral];
		endxdf = sizeRatio * (SysRectLEFT(sysL)+SysRelxd(endL));
		endxd = endxdf;
		
		/* Include difference of sysRect.left of this sytem and master system, to
		   account for possible editing of marginRect.left */

		rMargDiff = SysRectLEFT(masterSysL) - SysRectLEFT(sysL);
		if (endxd > SysRectRIGHT(masterSysL)-rMargDiff+DANGLING_SLOP) return True;
	}
	return False;
}


/* Return number of systems which will fit on a page, given changes made inside Master
Page. */

static short MaxSysOnPage(Document *doc)
{
	return doc->nSysMP;		/* doc->nSysMP is maintained by drawing routines */
}


/* Return True if exporting Master Page will result in one or more systems not fitting
on their pages. */

static Boolean DanglingSystems(Document *doc)
{
	short maxOnPage; LINK pageL;

	maxOnPage = MaxSysOnPage(doc);

	pageL = SSearch(doc->headL, PAGEtype, False);

	for ( ; pageL; pageL = LinkRPAGE(pageL))
		if (NSysOnPage(pageL) > maxOnPage) return True;

	return False;
}


/* Return True if either the system rects have been modified inside Master Page or the
top or bottom page margins have been dragged or edited with the margins dialog. FIXME:
Need to decide what to do about the margins: Should editing the top margin require only
vertical translation downwards? How should editing top and bottom margins be
distinguished? */

static Boolean SysVChanged(Document *doc)
{
	return (doc->sysVChangedMP || doc->margVChangedMP);
}


/* Return True if the system rects have been modified inside Master Page: the left/right
margins have been either dragged or edited with the margins dialog. doc->sysHChangedMP
is currently always False. */

static Boolean SysHChanged(Document *doc)
{
	return (doc->sysHChangedMP || doc->margHChangedMP);
}


/* Return True if any of the staves have been dragged vertically, or if parts have
been added or deleted. */

static Boolean StaffChanged(Document *doc)
{
	return doc->stfChangedMP || doc->partChangedMP;
}

/* Return True if the user has edited the marginRect of the document. */

static Boolean MarginVChanged(Document *doc)
{	
	return doc->margVChangedMP;
}

static Boolean MarginHChanged(Document *doc)
{
	return doc->margHChangedMP;
}


/* Return True if the user has changed the indent of either the first system or of the
succeeding systems of the document. */

static Boolean IndentChanged(Document *doc)
{
	return doc->indentChangedMP;
}


static Boolean RastralChanged(Document *doc)
{
	return (doc->srastral!=doc->srastralMP);
}


/* Update systemRectYs of all Systems in doc's main object list according to the current
Master Page settings. */

static void MPFixSystemRectYs(Document *doc)
{
	LINK sysL, masterSysL, firstSysL;
	PSYSTEM pSystem;
	DDIST sysHeight, sysTop, topMargin, firstMargin;
	DDIST masterSysTop, sysBottom, masterSysBottom, yBetweenSys;
	static DDIST lastSysBottom;
	
	yBetweenSys = doc->yBetweenSysMP;

	masterSysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
	pSystem = GetPSYSTEM(masterSysL);

	sysHeight = pSystem->systemRect.bottom - pSystem->systemRect.top;
	sysTop = masterSysTop = pSystem->systemRect.top;
	sysBottom = masterSysBottom = pSystem->systemRect.bottom;

	/* Copy size of Master Page's system into every system in the score. Position the
	   top system of every page according to the score's marginRect and the current
	   titleMargin. */

	topMargin = pt2d(doc->marginRect.top);
	firstMargin = topMargin+pt2d(config.titleMargin);
	firstSysL = SSearch(doc->headL, SYSTEMtype, False);
	
	for (sysL =  firstSysL; sysL; sysL = LinkRSYS(sysL)) {
		if (FirstSysInPage(sysL)) {
			sysTop = (sysL==firstSysL? firstMargin : topMargin); 
			sysBottom = sysTop+(masterSysBottom-masterSysTop);
			SetDRect(&SystemRECT(sysL), SysRectLEFT(sysL), sysTop, SysRectRIGHT(sysL), sysBottom);
		}
		else {
			sysTop = lastSysBottom+yBetweenSys;
			sysBottom = sysTop+sysHeight;
			SetDRect(&SystemRECT(sysL), SysRectLEFT(sysL), sysTop, SysRectRIGHT(sysL), sysBottom);
		}
		lastSysBottom = sysBottom;
		LinkVALID(sysL) = False;
	}
}


/* Update systemRectXs of all Systems in doc's main object list according to the current
Master Page settings. */

static void MPFixSystemRectXs(Document *doc)
{
	LINK sysL, masterSysL; PSYSTEM pSystem;
	DDIST sysLeft, sysRight;
	Boolean firstSys=True;

	masterSysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
	sysLeft = SysRectLEFT(masterSysL);
	sysRight = SysRectRIGHT(masterSysL);

	sysL = SSearch(doc->headL, SYSTEMtype, False);
	for ( ; sysL; sysL = LinkRSYS(sysL)) {
		if (FirstSysInPage(sysL)) {
			SetDRect(&SystemRECT(sysL), sysLeft, SysRectTOP(sysL), sysRight, SysRectBOTTOM(sysL));
			
			if (firstSys) {
				pSystem = GetPSYSTEM(sysL);
				pSystem->systemRect.left += (doc->dIndentFirstMP-doc->dIndentOtherMP);
				firstSys = False;
			}
		}
		else {
			SetDRect(&SystemRECT(sysL), sysLeft, SysRectTOP(sysL), sysRight, SysRectBOTTOM(sysL));
		}
		LinkVALID(sysL) = False;
	}
}


/*
 * Get the ledgerYSp of the document based upon the hiPitchLim above the top staff
 * of the first staff object in the document.
 */

void FixLedgerYSp(Document *doc)
{
	LINK staffL; short hiPitchLim;
	
	staffL = SSearch(doc->headL, STAFFtype, False);
	
	hiPitchLim = GetPitchLim(doc, staffL, 1, True);
	doc->ledgerYSp = -hiPitchLim;
}


/* Conform all ranges [system, connect] in the score to the Master Page object list. */

static void ConformRanges(Document *doc)
{
	Boolean conformSysH, conformSysV, conformStfTops, conformStfLens;
	LINK measL;

	/* Update system rects of systems in doc. */
	conformSysV = SysVChanged(doc);
	if (conformSysV) {
		doc->changed = True;
		doc->yBetweenSys = doc->yBetweenSysMP = 0;
		MPFixSystemRectYs(doc);
	}
	conformSysH = SysHChanged(doc);
	if (conformSysH) {
		doc->changed = True;
		MPFixSystemRectXs(doc);
	}

	/* Update staffTops and staffLengths of all staves in both score and
		Master Page. */
	conformStfTops = ( conformSysV || StaffChanged(doc) || MarginVChanged(doc) );
	if (conformStfTops) {
		doc->changed = True;
		MPUpdateStaffTops(doc, doc->headL, doc->tailL);
		MPUpdateStaffTops(doc, doc->masterHeadL, doc->masterTailL);
	}
	/* Set a flag to fix ledgers and measure rects of measures in the document.
		FixMeasRectYs(doc) should be called in ExportChanges after the Part/Staff
		structure of the document is fixed up. */
	if (conformSysV || conformStfTops) {
		doc->fixMeasRectYs = True;
	}

	conformStfLens = ( MarginHChanged(doc) || IndentChanged(doc) );
	if (conformStfLens) {
		SetAllStaffLengths(doc, doc->headL, doc->tailL);
	}
	if (conformSysH || conformStfLens) {
		measL = SSearch(doc->headL, MEASUREtype, False);
		FixMeasRectXs(measL, NILINK);
	}
}


static void SetStaffSizeMP(Document *doc)
{
	if (doc->srastral!=doc->srastralMP) {
		doc->changed = True;
		SetStaffSize(doc, doc->headL, doc->tailL, doc->srastral, doc->srastralMP);
		doc->srastral = doc->srastralMP;
	}
}


static void SetStaffLinesMP(Document *doc)
{
	short	staffn, staffLines, showLines;
	LINK	masterStaffL, aMasterStaffL, staffL, aStaffL;
	DDIST	lnSpace, staffHeight;
	Boolean	showLedgers;

	if (!doc->stfLinesChangedMP) return;
	
	masterStaffL = SSearch(doc->masterHeadL, STAFFtype, GO_RIGHT);
	aMasterStaffL = FirstSubLINK(masterStaffL);
	for ( ; aMasterStaffL; aMasterStaffL = NextSTAFFL(aMasterStaffL)) {
		staffLines = StaffSTAFFLINES(aMasterStaffL);
		showLines = StaffSHOWLINES(aMasterStaffL);
		showLedgers = StaffSHOWLEDGERS(aMasterStaffL);
		staffn = StaffSTAFF(aMasterStaffL);
		staffL = LSSearch(doc->headL, STAFFtype, ANYONE, GO_RIGHT, False);
		for ( ; staffL; staffL = LinkRSTAFF(staffL)) {
			aStaffL = StaffOnStaff(staffL, staffn);
			if (aStaffL) {
				/* Fix staff height -- compute using interline space. */
				lnSpace = StaffHEIGHT(aStaffL) / (StaffSTAFFLINES(aStaffL)-1);
				staffHeight = lnSpace * (staffLines-1);
				StaffHEIGHT(aStaffL) = staffHeight;

				/* Make changes requested by user. */
				StaffSTAFFLINES(aStaffL) = staffLines;
				StaffSHOWLINES(aStaffL) = showLines;
				StaffSHOWLEDGERS(aStaffL) = showLedgers;
			}
		}
	}
	doc->changed = True;
}


static void ConformLeftEnds(Document *doc)
{
	doc->firstNames = doc->firstNamesMP;
	doc->dIndentFirst = doc->dIndentFirstMP;
	doc->otherNames = doc->otherNamesMP;
	doc->dIndentOther = doc->dIndentOtherMP;
}


static void FixSelStaff(Document *doc)
{
	LINK staffL; short botStaff;
	
	if (doc->selStaff>doc->nstaves) {
		/* doc->selStaff doesn't exist. Use the highest-numbered staff that's visible
			in the system where the insertion point is (or where the selection begins). */
			
		staffL = EitherSearch(doc->selStartL, STAFFtype, ANYONE, GO_LEFT, False);
		botStaff = NextVisStaffn(doc, staffL, True, doc->nstaves);
		doc->selStaff = botStaff;
	}
}


void ExportMasterPage(Document *doc)
{
	/* Update all ranges from SYSTEM to CONNECT in doc to conform with
		corresponding ranges in Master Page object list. */

	ConformRanges(doc);

	if (ExportChanges(doc)) {

		/* FIXME: It's not clear we can handle error situations (nparts = 0, nchanges >
			maxChanges) here since we've already exported some stuff in ConformRanges.
			Must check these _before_ ConformRanges. */
		;
	}
	
	/* Be sure doc->selStaff still exists. */
	
	FixSelStaff(doc);

	/* Export any changes to indent specifications or showing part names at left
		of systems. This must be done AFTER ConformRanges! */
	
	ConformLeftEnds(doc);
	
	/* If the user has set the staff size inside Master Page, export the
		new staff size here. */

	SetStaffSizeMP(doc);
	
	/* If the user has set the staff size inside Master Page, export the
		new staff size here. */

	SetStaffLinesMP(doc);
		
	
	/* Editing margins may have invalidated objRects in the score. */
	InvalRange(doc->headL, doc->tailL);
}


/* ------------------------------------------------------- MasterPage Editing Routines -- */

/*
 * Change the staffTop value in Master Page's staffTop array for the staff
 * just dragged up or down by DragGrayRgn.
 */

static void ChangeStaffTops(Document *doc, LINK /*pL*/, LINK aStaffL, long newPos)
{
	short v;
	
	v = HiWord(newPos);
	doc->staffTopMP[StaffSTAFF(aStaffL)] += p2d(v);
}


/* Update the Master Page to reflect changes resulting from dragging the system up or
down. */

void UpdateDraggedSystem(Document *doc, long newPos)
{
	LINK sysL, staffL, aStaffL;  DRect sysRect;
	short v;
	
	sysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
	sysRect = SystemRECT(sysL);
	SystemRECT(sysL).bottom += p2d(HiWord(newPos));
	
	staffL = SSearch(doc->masterHeadL, STAFFtype, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		v = HiWord(newPos);
		doc->staffTopMP[StaffSTAFF(aStaffL)] += p2d(v/2);
	}
	MPUpdateStaffTops(doc, doc->masterHeadL, doc->masterTailL);

	EraseAndInval(&doc->viewRect);
	doc->masterChanged = True;
}


/* Update the Master Page to reflect changes resulting from dragging a staff up or 
down. */

void UpdateDraggedStaff(Document *doc, LINK pL, LINK aStaffL, long newPos)
{
	short staffn;

	staffn = StaffSTAFF(aStaffL);
	ChangeStaffTops(doc, pL, aStaffL, newPos);
	MPUpdateStaffTops(doc, doc->masterHeadL, doc->masterTailL);
	
	/* If the last staff was dragged up or down, shrink or enlarge the
		systemRects to accomodate the new position of this staff. */
		
	if (staffn==doc->nstavesMP)
		UpdateMPSysRectHeight(doc, p2d(HiWord(newPos)));

	EraseAndInval(&doc->viewRect);
	doc->masterChanged = True;
}


/* Update the Master Page to reflect changes resulting from editing the margin rect. */

void UpdateMasterMargins(Document *doc)
{
	LINK staffL, sysL;
	
	/* Update the system rect: top, bottom, left & right */
	sysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
	UpdateSystemRects(doc, sysL);

	/* Update the staff length: system right and staff right */
	staffL = SSearch(doc->masterHeadL, STAFFtype, False);
	SetStaffLengths(doc, staffL);

	InvalWindow(doc);
}


/* --------------------------------------------------- Routines for exiting MasterPage -- */

/* ------------------------------------------------------------------- FixMasterStfSel -- */
/* Clear any left-over selection from the mastarpage staff. */

static void ClearMasterStfSel(Document *doc) 
{
	LINK staffL = SSearch(doc->masterHeadL, STAFFtype, GO_RIGHT);
	DeselectSTAFF(staffL);	
	LinkSEL(staffL) = False;
}


/* ----------------------------------------------------------------- ResetMasterFields -- */
/* Reset Master Page fields to default values. */

static void ResetMasterFields(Document *doc)
{
	/*	JGG did this as part of lifting restrictions on deleting parts. The old
		selection may no longer be in the object list. */
	if (InObjectList(doc, doc->oldSelStartL, MAIN_DSTR)
			&& InObjectList(doc, doc->oldSelEndL, MAIN_DSTR)) {
		doc->selStartL = doc->oldSelStartL;
		doc->selEndL = doc->oldSelEndL;
	}
	OptimizeSelection(doc);

	doc->masterChanged = False;		/* Reset master changed flags */
	doc->partChangedMP = False;
	doc->grpChangedMP = False;
	doc->sysVChangedMP = False;
	doc->sysHChangedMP = False;
	doc->stfChangedMP = False;
	doc->stfLinesChangedMP = False;
	doc->margVChangedMP = False;
	doc->margHChangedMP = False;
	doc->indentChangedMP = False;
	doc->fixMeasRectYs = False;

	doc->prevMargin = doc->marginRect;
	doc->srastralMP = doc->srastral;

}


/* Return True if any of:
		parts added/deleted
		system dragged
		staff dragged
		margin edited
		indent changed
		measure rects not valid
		staff size changed.
 */

static Boolean AnyMPChanged(Document *doc)
{
	return ( doc->partChangedMP ||
				doc->sysVChangedMP ||
				doc->sysHChangedMP ||
				doc->stfChangedMP ||
				doc->stfLinesChangedMP ||
				doc->margVChangedMP ||
				doc->margHChangedMP ||
				doc->indentChangedMP ||
				doc->fixMeasRectYs ||
				doc->grpChangedMP ||
				doc->srastralMP != doc->srastral ||
									!EqualRect(&doc->prevMargin, &doc->marginRect) );
}


/* ------------------------------------------------------------------- ResetMasterPage -- */
/* Revert Master Page fields and replace Master Page object list. FIXME: Need to
decide what to do about Master Page fields handled in ResetMasterFields. */

static void ResetMasterPage(Document *doc)
{
	doc->yBetweenSysMP = doc->yBetweenSys = 0;
	doc->srastralMP = doc->srastral;
	doc->marginRect = doc->prevMargin;
	doc->nChangeMP = 0;
	ReplaceMasterPage(doc);
}


/* -------------------------------------------------------------------- GetExportAlert -- */
/* Determine whether local format changes (from Work on Format) will be lost when
exporting changes made inside Master Page. If so, return EXPORTFMT_ALRT; otherwise, 
return EXPORT_ALRT. */

static short GetExportAlert(Document *doc)
{
	if (doc->locFmtChanged) {

		if (SysVChanged(doc) ||
			 StaffChanged(doc) ||
			 MarginVChanged(doc) ||
			 RastralChanged(doc) ||
			 SysHChanged(doc) ||
			 MarginHChanged(doc) ||
			 doc->partChangedMP)

				return EXPORTFMT_ALRT;
	}
	return EXPORT_ALRT;
}


static void DisposeMasterData(Document *doc)
{
	DisposePtr((Ptr)doc->staffTopMP);
	doc->staffTopMP = NILINK;
	
	DisposeNodeList(doc, &doc->oldMasterHeadL, &doc->oldMasterTailL);
	doc->oldMasterHeadL =
		doc->oldMasterTailL = NILINK;
}


static enum {
	RFMT_NONE=0,
	RFMT_KeepSBreaks,
	RFMT_ChangeSBreaks
} E_ReformatItems;

/* -------------------------------------------------------------------- ExitMasterView -- */
/* Do everything necessary to leave Master Page. Call ExportMasterPage to export
all of Master Page's state variables to the main object list, if needed; then reset
all of them.

If the user discards changes, we must call ReplaceMasterPage to regenerate Master
Page object list, since Add/DeletePart may have altered it. */

/* Reformat parameters */
#define CARE_MEASPERSYS False
#define MEASPERSYS 4
#define CARE_SYSPERPAGE False
#define SYSPERPAGE 6

/* Alert item numbers */
#define APPLY_REFORMAT_DI 1
#define APPLY_NOREFORMAT_DI 2
#define DISCARD_DI 3

Boolean ExitMasterView(Document *doc)
{
	short stayInMP, saveChanges, nstaves, exportAlertID, reformat;
	LINK staffL;

 	if (doc->masterChanged) {
		if (!AnyMPChanged(doc)) {
			/*
			 * Something has been changed but not much--I think only the descriptions of
			 * existing parts--is this right, Charlie?
			 */
			exportAlertID = GetExportAlert(doc);
			PlaceAlert(exportAlertID, doc->theWindow,  0,  30);
	 		saveChanges = CautionAdvise(exportAlertID);
	 		WaitCursor();
	 		if (saveChanges==OK) {
				ExportPartList(doc);
				doc->changed = True;
				doc->locFmtChanged = False;		/* Score has no local format changes */
				goto quit;
			}
			else
	 			ResetMasterPage(doc);
		}
		staffL = SSearch(doc->masterHeadL, STAFFtype, GO_RIGHT);
		nstaves = LinkNENTRIES(staffL);
		if (nstaves <= 0) {
			PlaceAlert(NOPARTS_ALRT, doc->theWindow, 0, 30);
 			stayInMP = CautionAdvise(NOPARTS_ALRT);
 			if (stayInMP==OK)
 				return False;
 			else {
 				ResetMasterPage(doc);
				goto quit;
			}
		}

		reformat = RFMT_NONE;
		if (DanglingSystems(doc)) {
			PlaceAlert(DANGLING_ALRT, doc->theWindow, 0, 30);
 			saveChanges = CautionAdvise(DANGLING_ALRT);
 			switch (saveChanges) {
 				case APPLY_REFORMAT_DI:
 					reformat = RFMT_KeepSBreaks;
 					break;
 				case APPLY_NOREFORMAT_DI:
 					reformat = RFMT_NONE;
 					break;
 				case DISCARD_DI:
 					ResetMasterPage(doc);
 					goto quit;
 				default:
 					;
  			}
	 	}
 		if (DanglingContents(doc)) {
			PlaceAlert(DANGL_CONTENTS_ALRT, doc->theWindow, 0, 60);
 			saveChanges = CautionAdvise(DANGL_CONTENTS_ALRT);
 			switch (saveChanges) {
 				case APPLY_REFORMAT_DI:
 					reformat = RFMT_ChangeSBreaks;
 					break;
 				case APPLY_NOREFORMAT_DI:
 					reformat = RFMT_NONE;
 					break;
 				case DISCARD_DI:
 					ResetMasterPage(doc);
 					goto quit;
 				default:
 					;
  			}
	 	}

		exportAlertID = GetExportAlert(doc);
		PlaceAlert(exportAlertID, doc->theWindow, 0, 30);
 		saveChanges = CautionAdvise(exportAlertID);
 		WaitCursor();
 		if (saveChanges==OK) {
			DisableUndo(doc, False);			/* Since we can't Undo across a structural change */
			doc->changed = True;
			doc->locFmtChanged = False;			/* Score has no local format changes */
			ExportMasterPage(doc);				/* Export all editing changes to score */
			if (reformat!=RFMT_NONE) {
				Reformat(doc, RightLINK(doc->headL), doc->tailL,
							(reformat==RFMT_ChangeSBreaks),
							(CARE_MEASPERSYS? MEASPERSYS : 9999), False,
							(CARE_SYSPERPAGE? SYSPERPAGE : 999), config.titleMargin);
			}
		}
		else
 			ResetMasterPage(doc);
	}
	
quit:
	ClearMasterStfSel(doc);
	ResetMasterFields(doc);
	SetupMasterMenu(doc, False);		/* Restore play/record menu and delete Master Page menu */
	DisposeMasterData(doc);
	return True;
}


/* ----------------------------------------------- Routines for setting up Master Page -- */

/* ------------------------------------------------------------------- EnterMasterView -- */
/* Call all routines needed to get into masterView. */

void EnterMasterView(Document *doc)
{
	SetupMasterView(doc);			/* Allocate memory for all necessary objects */
	SetupMasterMenu(doc, True);		/* Replace the play/record menu with Master Page menu */
	ImportMasterPage(doc);			/* Get score parameters for Master Page use */
}


/* ------------------------------------------------------------------ ImportMasterPage -- */
/* Import score parameters: array of staffTops, yBetweenSys, etc. */

void ImportMasterPage(Document *doc)
{
	LINK staffL, aStaffL;

	/* Fill in doc->staffTopMP, the array of staffTop coordinates. */
	
	doc->staffTopMP[0] = 0;
	staffL = LSSearch(doc->masterHeadL, STAFFtype, ANYONE, False, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		doc->staffTopMP[StaffSTAFF(aStaffL)] = StaffTOP(aStaffL);
	}

	doc->yBetweenSysMP = doc->yBetweenSys = 0;
	doc->prevMargin = doc->marginRect;
	doc->srastralMP = doc->srastral;

	doc->firstNamesMP = doc->firstNames;
	doc->dIndentFirstMP = doc->dIndentFirst;
	doc->otherNamesMP = doc->otherNames;
	doc->dIndentOtherMP = doc->dIndentOther;
	doc->oldSelStartL = doc->selStartL;
	doc->oldSelEndL = doc->selEndL;

	doc->partChangedMP = False;
	doc->grpChangedMP = False;
	doc->sysVChangedMP = False;
	doc->sysHChangedMP = False;
	doc->stfChangedMP = False;
	doc->stfLinesChangedMP = False;
	doc->margVChangedMP = False;
	doc->margHChangedMP = False;
	doc->indentChangedMP = False;
	doc->fixMeasRectYs = False;
	
	ClearMasterStfSel(doc);
}


/* Check if a single system will fit on a single page in Master Page with rastral
size newRastral, assuming the current size is srastral and "proportional respacing".
This check is necessary because if at least one system will fit, we can reformat to
satisfaction upon exiting Master Page; otherwise, we won't be able to reformat, with
error resulting. */

static Boolean MPChkSysHt(Document *doc, short srastral, short newRastral)
{
	LINK sysL;  DRect sysRect;
	double sysSize, sysOffset;
	long docPageHt;  LONGDDIST pageHt;
	FASTFLOAT fact;

	fact = (FASTFLOAT)drSize[newRastral]/drSize[srastral];

	sysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
	sysRect = SystemRECT(sysL);
		
	sysSize = sysRect.bottom-sysRect.top;
	sysOffset = (fact-1.0)*sysSize;
	sysRect.bottom += sysOffset;
	
	sysSize = sysRect.bottom-sysRect.top;
	docPageHt = doc->marginRect.bottom-doc->marginRect.top;
	pageHt = pt2d(docPageHt);
	
	return (sysSize < pageHt);
}


void DoMasterStfSize(Document *doc)
{
	short			i, newRastral, srastral;
	Boolean			propRespace, partsSelected;
	LINK			sysL;
	DDIST			ymove;
	DRect			sysRect;
	FASTFLOAT		fact;
	double			sysSize, sysOffset;
	char			fmtStr[256];
	static Boolean	selPartsOnly=True;

	partsSelected = PartIsSel(doc);
	srastral = doc->srastralMP;

	newRastral = RastralDialog(partsSelected, srastral, &propRespace, &selPartsOnly);
	if (newRastral>CANCEL_INT && newRastral!=srastral)	{
		if (!propRespace || MPChkSysHt(doc, srastral, newRastral)) {
			fact = (FASTFLOAT)drSize[newRastral]/drSize[srastral];
			ymove = (fact-1.0)*doc->staffTopMP[1];
			for (i = 1; i<=doc->nstavesMP; i++)					/* Keep ledger space above staff 1 */
				doc->staffTopMP[i] += ymove;
	
			/* If doing "proportional respace", change the distance between staves as
				well as the staves themselves. Don't change the top margin, so start 
				moving with staff 2. */
	
			if (propRespace) {
				for (i=2; i<=doc->nstavesMP; i++)
					doc->staffTopMP[i] = doc->staffTopMP[1] +
						fact * (doc->staffTopMP[i]-doc->staffTopMP[1]);
			}
	
			SetStaffSize(doc, doc->masterHeadL, doc->masterTailL, srastral, newRastral);
			MPUpdateStaffTops(doc, doc->masterHeadL, doc->masterTailL);
	
			if (propRespace) {
				sysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
				sysRect = SystemRECT(sysL);
				
				sysSize = sysRect.bottom-sysRect.top;
				sysOffset = (fact-1.0)*sysSize;
				UpdateMPSysRectHeight(doc, (DDIST)sysOffset);
			}

			doc->srastralMP = newRastral;
			doc->masterChanged = True;
			doc->stfChangedMP = True;
			doc->sysVChangedMP = True;
	
			InvalWindowRect(doc->theWindow, &doc->viewRect);
		}
		else {
			GetIndCString(fmtStr, MPERRS_STRS, 1);		/* "Can't change to staff size %d" */
			sprintf(strBuf, fmtStr, newRastral);
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
		}
	}
}


void DoMasterStfLines(Document *doc)
{
	short showLines, staffLines;
	Boolean partsSelected;
	LINK staffL, aStaffL;
	static short apparentStaffLines=STFLINES;
	static Boolean showLedgers=True, selPartsOnly=True;

	partsSelected = PartIsSel(doc);
	if (partsSelected) {
		staffL = SSearch(doc->masterHeadL, STAFFtype, GO_RIGHT);
		for (aStaffL = FirstSubLINK(staffL); aStaffL; aStaffL = NextSTAFFL(aStaffL))
			if (StaffSEL(aStaffL))
				break;							/* has to happen if partsSelected */
		staffLines = StaffSTAFFLINES(aStaffL);
		showLines = StaffSHOWLINES(aStaffL);
		if (showLines==0 || showLines==1)
			apparentStaffLines = showLines;
		else
			apparentStaffLines = staffLines;
		showLedgers = StaffSHOWLEDGERS(aStaffL);
	}
	else {
		apparentStaffLines = STFLINES;
		showLedgers = True;
	}
	if (StaffLinesDialog(partsSelected, &apparentStaffLines, &showLedgers, &selPartsOnly)) {
		if (apparentStaffLines==0 || apparentStaffLines==1) {
			showLines = apparentStaffLines;
			staffLines = STFLINES;				/* data structure representation of staff lines */
		}
		else {
			showLines = SHOW_ALL_LINES;
			staffLines = apparentStaffLines;
		}
		staffL = SSearch(doc->masterHeadL, STAFFtype, GO_RIGHT);
		for (aStaffL = FirstSubLINK(staffL); aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
			if (StaffSEL(aStaffL) || (!partsSelected || !selPartsOnly)) {
				DDIST lnSpace = StaffHEIGHT(aStaffL) / (StaffSTAFFLINES(aStaffL)-1);
				if (StaffSTAFFLINES(aStaffL)!=staffLines) {
					StaffSTAFFLINES(aStaffL) = staffLines;
					doc->stfLinesChangedMP = True;
				}
				if (StaffSHOWLINES(aStaffL)!=showLines) {
					StaffSHOWLINES(aStaffL) = showLines;
					doc->stfLinesChangedMP = True;
				}
				if (StaffSHOWLEDGERS(aStaffL)!=showLedgers) {
					StaffSHOWLEDGERS(aStaffL) = showLedgers;
					doc->stfLinesChangedMP = True;
				}
				/* Fix staff height -- compute using interline space. */
				StaffHEIGHT(aStaffL) = lnSpace * (staffLines-1);
			}
		}
		if (doc->stfLinesChangedMP) {
			doc->masterChanged = True;
			InvalWindowRect(doc->theWindow, &doc->viewRect);
		}
	}
}


void MPEditMargins(Document *doc)
{
	short lMarg, tMarg, rMarg, bMarg;

	lMarg = doc->marginRect.left;
	tMarg = doc->marginRect.top;
	rMarg = doc->origPaperRect.right-doc->marginRect.right;
	bMarg = doc->origPaperRect.bottom-doc->marginRect.bottom;

 	if (MarginsDialog(doc, &lMarg, &tMarg, &rMarg, &bMarg)) {
		doc->masterChanged = True;
		doc->margVChangedMP = (doc->margVChangedMP || tMarg!=doc->marginRect.top || bMarg != doc->origPaperRect.bottom-doc->marginRect.bottom);
		doc->margHChangedMP = (doc->margHChangedMP || lMarg!=doc->marginRect.left || rMarg != doc->origPaperRect.right-doc->marginRect.right);
		doc->marginRect.left = lMarg;
		doc->marginRect.top = tMarg;
		doc->marginRect.right = doc->origPaperRect.right-rMarg;
		doc->marginRect.bottom = doc->origPaperRect.bottom-bMarg;
		UpdateMasterMargins(doc);
		InvalWindowRect(doc->theWindow, &doc->viewRect);
 	}
}


/* ------------------------------------------------------------------- SetupMasterMenu -- */
/* Replace the play/record menu with the Master Page menu. */

void SetupMasterMenu(Document *doc, Boolean /*enter*/)
{
	InstallDocMenus(doc);
}


/* -------------------------------------------------------------------- CopyMasterPage -- */
/* Copy the Master Page object list at headL, and return the headL of the copy. Not
valid for general object list: does minimal updating necessary for Master Page object
list, and is not guaranteed for more. All copying is in doc's heaps. */

static LINK CopyMasterPage(Document *doc, LINK headL, LINK *newTailL)
{
	LINK masterHeadL, pL, prevL, copyL=NILINK;

	masterHeadL = DuplicateObject(HEADERtype, headL, False, doc, doc, False);
	if (!masterHeadL) return NILINK;

	pL = RightLINK(headL);
	prevL = masterHeadL;
	for ( ; pL; pL=RightLINK(pL)) {
		copyL = DuplicateObject(ObjLType(pL),  pL,  False,  doc,  doc,  False);
		if (!copyL) return NILINK;

		RightLINK(prevL) = copyL;
		LeftLINK(copyL) = prevL;

		prevL = copyL;
  	}

	*newTailL = copyL;
	RightLINK(copyL) = NILINK;
	FixStructureLinks(doc, doc, masterHeadL, copyL);
	
	return masterHeadL;
}


/* ------------------------------------------------------------------- SetupMasterView -- */
/* Allocate memory for all necessary objects in Master Page view. */

Boolean SetupMasterView(Document *doc)
{
	doc->nstavesMP = doc->nstaves;
	doc->partChangedMP = False;

	doc->staffTopMP = (DDIST *)NewPtr((Size)(MAXSTAVES+1)*sizeof(DDIST));
	if (!GoodNewPtr((Ptr)doc->staffTopMP)) goto broken;

	doc->oldMasterHeadL = CopyMasterPage(doc, doc->masterHeadL, &doc->oldMasterTailL);
	
	return True;

broken:
	OutOfMemory((long)(MAXSTAVES+1)*sizeof(DDIST));
	return False;
}


/* -------------------------------------------------- Routines to re-create MasterPage -- */

/* Replace the Master Page for doc with one created from the current Master Page object
list; used to re-generate the Master Page if, for example, if the structure of the score
is changed by MPImport. This is not something that should be done lightly: under normal
circumstances, the content of the score should never affect the Master Page. (Formerly
called ReplaceMasterPage.) */

void Score2MasterPage(Document *doc)
{
	DDIST sysTop;

	DisposeNodeList(doc, &doc->masterHeadL, &doc->masterTailL);
	
	sysTop = SYS_TOP(doc);
	NewMasterPage(doc, sysTop, True);
}


/* Replace the Master Page for doc with a copy of a previously-saved Master Page
object list. */

void ReplaceMasterPage(Document *doc)
{
	DisposeNodeList(doc, &doc->masterHeadL, &doc->masterTailL);
	
	doc->masterHeadL = CopyMasterPage(doc, doc->oldMasterHeadL, &doc->masterTailL);
}


/* ----------------------------------------------------- Routines to create MasterPage -- */

/* Visify invisible staves in the Master system. Master Page staves should _never_
be invisible!; this can be called whenever there's a chance they might be, e.g., after
constructing a Master Page object list from an ordinary system. */

void VisifyMasterStaves(Document *doc)
{
	LINK pL, aStaffL;
	
	for (pL=doc->masterHeadL; pL!=doc->masterTailL; pL=RightLINK(pL))
		if (StaffTYPE(pL)) {
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				if (!StaffVIS(aStaffL))
					VisifyStf(pL, aStaffL, StaffSTAFF(aStaffL));
			}
			break;
		}
}


/* Create a new Master Page object list, and return True if OK; False if not. If fromDoc
is True, create a Master Page from a document that doesn't have one, so copy its first
system; otherwise, create a new default system for the Master Page.

The list consists of head and tail, the head followed by a single range from page to
connect inclusive. The range [system, connect] will be re-drawn repeatedly offset
downwards by sysHeight+doc->yBetweenSysMP to display a page full of systems. */

Boolean NewMasterPage(Document *doc, DDIST sysTop, Boolean fromDoc)
{
	LINK qL, qPageL, qSystemL, endL, sysL;

	if (fromDoc) {
		/* DuplicateObject automatically copies all partInfo subObjs */

		doc->masterHeadL = DuplicateObject(HEADERtype, doc->headL, False, doc, doc, False);
		doc->masterTailL = NewNode(doc, TAILtype, 0);
		if (!doc->masterHeadL || !doc->masterHeadL) goto broken;

		RightLINK(doc->masterHeadL) = doc->masterTailL;
		LeftLINK(doc->masterTailL) = doc->masterHeadL;
		LeftLINK(doc->masterHeadL) = RightLINK(doc->masterTailL) = NILINK;

		endL = LSSearch(doc->headL, CONNECTtype, ANYONE, False, False);

		if (!CopyMasterRange(doc, RightLINK(doc->headL), RightLINK(endL), doc->masterTailL))
			goto broken;

		FixStructureLinks(doc, doc, doc->masterHeadL, doc->masterTailL);

		sysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
		UpdateSystemRects(doc, sysL);
		SetAllStaffLengths(doc, doc->masterHeadL, doc->masterTailL);

	}
	else {
		if (!BuildEmptyList(doc, &doc->masterHeadL, &doc->masterTailL)) goto broken;

		/* Handle the first system */
		
		qPageL = qSystemL = NILINK;

		qPageL = qL = MakeNewPage(doc);
		if (!qL) goto broken;
		qSystemL = qL = MakeNewSystem(doc, qL, qPageL, sysTop);
		if (!qL) goto broken;
		SetMasterPParts(doc);						/* Only call this for first system */
		
		qL = MakeNewStaff(doc, qL, qSystemL, sysTop);
		if (!qL) goto broken;
		qL = MakeNewConnect(doc, qL);
		if (!qL) goto broken;

		RightLINK(qL) = doc->masterTailL;
		LeftLINK(doc->masterTailL) = qL;
		/* doc->selStartL = doc->selEndL = doc->masterTailL; */
	}

	VisifyMasterStaves(doc);

	return True;

broken:
	NoMoreMemory();
	return False;
}


/*
 * A version of CopyRange for the creation of the Master Page. Assumes all copying is
 * from/to objects in the same heap. Does not use a COPYMAP, since it only copies
 * objects of type J_STRUC and CONNECTs.
 */

Boolean CopyMasterRange(Document *doc, LINK srcStartL, LINK srcEndL, LINK insertL)
{
	LINK pL, prevL, copyL, initL;

	initL = prevL = LeftLINK(insertL);

	for (pL=srcStartL; pL!=srcEndL; pL=RightLINK(pL)) {

		if (JustTYPE(pL)==J_D && !ConnectTYPE(pL)) continue;
		copyL = DuplicateObject(ObjLType(pL),  pL, False, doc, doc, False);
		if (!copyL) return False;					/* Memory error or some other problem */

		RightLINK(copyL) = insertL;
		LeftLINK(insertL) = copyL;
		RightLINK(prevL) = copyL;
		LeftLINK(copyL) = prevL;

		prevL = copyL;
  	}
 	
	return True;									/* Successful copy */
}


/*
 *	Insert a new page at head of Master Page object list, deliver its LINK, or NILINK.
 */

static LINK MakeNewPage(Document *doc)
{
	LINK pL; PPAGE pPage; 

	pL = InsertNode(doc, RightLINK(doc->masterHeadL), PAGEtype, 0);
	if (pL) {
		SetObject(pL, 0, 0, False, True, True);
		LinkTWEAKED(pL) = False;

		pPage = GetPPAGE(pL);
		pPage->lPage = pPage->rPage = NILINK;
		pPage->sheetNum = 0;
	}
		
	return pL;
}


/* Insert a new system after qL in some object list belonging to doc.  Deliver new
system's LINK, or NILINK. */

static LINK MakeNewSystem(Document *doc, LINK qL, LINK qPageL, DDIST sysTop)
{
	LINK pL; PSYSTEM pSystem; DDIST sysLeft;
	
	pL = InsertNode(doc, RightLINK(qL), SYSTEMtype, 0);
	if (pL) {
		sysLeft = MARGLEFT(doc)+doc->dIndentOther;
		SetObject(pL, sysLeft, sysTop, False, True, True);
		LinkTWEAKED(pL) = False;
		
		pSystem = GetPSYSTEM(pL);
		pSystem->systemNum = 1;					/* Only one System for Master Page */
		pSystem->lSystem = pSystem->rSystem = NILINK;
		pSystem->pageL = qPageL;
	}
	
	return pL;
}


/* This sets up the PARTINFO objects for the masterHead. It should be called only
when creating the first system in the entire Master Page object list. */
	
static void SetMasterPParts(Document *doc)
{
	LINK partL;  PPARTINFO pPart;

	partL = FirstSubLINK(doc->masterHeadL);

	InitPart(partL, NOONE, NOONE);				/* Dummy part: should never be used at all */
	pPart = GetPPARTINFO(partL);
	strcpy(pPart->name, "DUMMY");
	strcpy(pPart->shortName, "DUM.");
	
	partL = NextPARTINFOL(partL);
	InitPart(partL, 1, 2);
}


#define DFLT_NUMER_MP 4
#define DFLT_DENOM_MP 4

static LINK MakeNewStaff(Document *doc, LINK qL, LINK qSystemL, DDIST sysTop)
{
	LINK aStaffL;
	short MPinitStfTop1, MPinitStfTop2, MPledgerYSp;
	DDIST staffLength, sysHeight, indent;
	PSYSTEM qSystem;  PSTAFF pStaff;

	MPledgerYSp = doc->ledgerYSp;
	MPinitStfTop1 = (short)(MPledgerYSp*drSize[doc->srastral]/STFHALFLNS);
	MPinitStfTop2 = (short)(2.5*drSize[doc->srastral]);

	staffLength = MARGWIDTH(doc)-doc->dIndentOther;
	sysHeight = MEAS_BOTTOM(MPinitStfTop1+MPinitStfTop2, STHEIGHT);

	qSystem = GetPSYSTEM(qSystemL);
	indent = doc->dIndentOther;
	SetDRect(&qSystem->systemRect, MARGLEFT(doc)+indent, sysTop,
			MARGLEFT(doc)+indent+staffLength, sysTop+sysHeight);

	if ((qL = InsertNode(doc, RightLINK(qL), STAFFtype, 2))) {
		SetObject(qL, 0, 0, False, True, True);
		LinkTWEAKED(qL) = False;
	
		pStaff = GetPSTAFF(qL);
		pStaff->lStaff = pStaff->rStaff = NILINK;
		pStaff->systemL = qSystemL;
	
		aStaffL = FirstSubLINK(qL);
		InitStaff(aStaffL, 1,  MPinitStfTop1,  0, staffLength, STHEIGHT, STFLINES, SHOW_ALL_LINES);
		StaffCLEFTYPE(aStaffL) = DFLT_CLEF;
		StaffTIMESIGTYPE(aStaffL) = DFLT_TSTYPE;
		StaffNUMER(aStaffL) = DFLT_NUMER_MP;
		StaffDENOM(aStaffL) = DFLT_DENOM_MP;
		StaffDynamType(aStaffL) = DFLT_DYNAMIC;
	
		aStaffL = NextSTAFFL(aStaffL);
		InitStaff(aStaffL, 2, MPinitStfTop1+MPinitStfTop2, 0, staffLength, STHEIGHT,
						STFLINES, SHOW_ALL_LINES);
		StaffCLEFTYPE(aStaffL) = DFLT_CLEF;
		StaffTIMESIGTYPE(aStaffL) = DFLT_TSTYPE;
		StaffNUMER(aStaffL) = DFLT_NUMER_MP;
		StaffDENOM(aStaffL) = DFLT_DENOM_MP;
		StaffDynamType(aStaffL) = DFLT_DYNAMIC;
	}
	return qL;
}


static LINK MakeNewConnect(Document *doc, LINK qL)
{
	LINK pL, aConnectL;
	DDIST halfpt, dLineSp, width;
	PCONNECT pConnect;

	dLineSp = STHEIGHT/(STFLINES-1);							/* Space between staff lines */
	width = pt2d(5);
	halfpt = pt2d(1)/2;

	pL = InsertNode(doc, RightLINK(qL), CONNECTtype, 2);
	if (pL) {
		SetObject(pL, LinkXD(qL)+width, 0, False, True, True);
		LinkTWEAKED(pL) = False;
	
		pConnect = GetPCONNECT(pL);
		pConnect->connFiller = 0;
	
		aConnectL = FirstSubLINK(pL);
		ConnectSEL(aConnectL) = False;
		ConnectCONNLEVEL(aConnectL) = SystemLevel;				/* Connect entire system */
		ConnectCONNTYPE(aConnectL) = CONNECTLINE;
		ConnectSTAFFABOVE(aConnectL) = NOONE;
		ConnectSTAFFBELOW(aConnectL) = NOONE;					/* Ignored if level=SystemLevel */
		ConnectFIRSTPART(aConnectL) = NOONE;
		ConnectLASTPART(aConnectL) = NOONE;
		ConnectXD(aConnectL) = 0;
		ConnectFILLER(aConnectL) = 0;
	
		aConnectL = NextCONNECTL(aConnectL);
		ConnectSEL(aConnectL) = False;
		ConnectCONNLEVEL(aConnectL) = PartLevel;				/* Connect the part */
		ConnectCONNTYPE(aConnectL) = CONNECTCURLY;
		ConnectXD(aConnectL) = -ConnectDWidth(doc->srastral, CONNECTCURLY);
	
		ConnectSTAFFABOVE(aConnectL) = 1;
		ConnectSTAFFBELOW(aConnectL) = 2;
	
		StoreConnectPart(doc->masterHeadL, aConnectL);
	
		ConnectFILLER(aConnectL) = 0;
	}

	return pL;
}


/* --------------------------------------------------------------------- Miscellaneous -- */

/* Store the PARTINFO LINK to the part connected by aConnectL. The firstStaff and
lastStaff fields of the connect subObj must be init'ed, and the connLevel must be
PartLevel. */

void StoreConnectPart(LINK headL, LINK aConnectL)
{
	LINK partL;
	short firstStf, lastStf;
	
	if (ConnectCONNLEVEL(aConnectL)!=PartLevel) return;

	firstStf = ConnectSTAFFABOVE(aConnectL);
	lastStf = ConnectSTAFFBELOW(aConnectL);

	partL = NextPARTINFOL(FirstSubLINK(headL));

	for ( ; partL; partL = NextPARTINFOL(partL))
		if (PartFirstSTAFF(partL)==firstStf && PartLastSTAFF(partL)==lastStf)
			{ ConnectFIRSTPART(aConnectL) = partL; break; }

	ConnectLASTPART(aConnectL) = NILINK;
}


/* Call StoreConnectPart for all subObjects of all connects in the object list
beginning at headL. */
	
void StoreAllConnectParts(LINK headL)
{
	LINK connectL, aConnectL;

	connectL = SSearch(headL, CONNECTtype, GO_RIGHT);
	
	for ( ; connectL; connectL=LSSearch(RightLINK(connectL), CONNECTtype, ANYONE, GO_RIGHT, False)) {
		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL))
			StoreConnectPart(headL, aConnectL);
	}
}


void MPSetInstr(Document *doc, PPARTINFO pPart)
{
	LINK partL, nextPartL; PPARTINFO mpPart;
	
	partL = FirstSubLINK(doc->masterHeadL);
	for ( ; partL; partL = NextPARTINFOL(partL))
		if (PartFirstSTAFF(partL) == pPart->firstStaff) {
			mpPart = GetPPARTINFO(partL);
			nextPartL = NextPARTINFOL(partL);

			BlockMove(pPart, mpPart, (long)subObjLength[HEADERtype]);
			mpPart->next = nextPartL;
			
			break;
		}
}
