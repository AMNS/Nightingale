/******************************************************************************************
*	FILE:	FileOpen.c
*	PROJ:	Nightingale
*	DESC:	Input of normal Nightingale files, plus file format conversion, etc.
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "FileConversion.h"			/* Must follow Nightingale.precomp.h! */
#include "CarbonPrinting.h"
#include "FileUtils.h"
#include "MidiMap.h"

/* A version code is four characters, specifically 'N' followed by three digits, e.g.,
'N105': N-one-zero-five. Be careful: It's neither a number nor a valid C string! */
static unsigned long version;							/* File version code read/written */

/* Prototypes for internal functions: */
static Boolean ConvertScoreContent(Document *, long);
static void DisplayDocumentHdr(short id, Document *doc);
static short CheckDocumentHdr(Document *doc);
static void DisplayScoreHdr(short id, Document *doc);
static Boolean CheckScoreHdr(Document *doc);

static void ConvertDocumentHeader(Document *doc, DocumentN105 *docN105);
static void ConvertFontTable(Document *doc, DocumentN105 *docN105);
static short ConvertScoreHeader(Document *doc, DocumentN105 *docN105);

static Boolean ModifyScore(Document *, long);
static void SetTimeStamps(Document *);

/* Codes for object types being read/written or for non-read/write call when an I/O
error occurs; note that all are negative. See HeapFileIO.c for additional, positive,
codes. */

enum {
	HEADERobj=-999,
	VERSIONobj,
	SIZEobj,
	CREATEcall,
	OPENcall,				/* -995 */
	CLOSEcall,
	DELETEcall,
	RENAMEcall,
	WRITEcall,
	STRINGobj,
	INFOcall,
	SETVOLcall,
	BACKUPcall,
	MAKEFSSPECcall,
	NENTRIESerr = -899
};


/* ------------------------------------------------------- ConvertScoreContent helpers -- */

#ifdef NOMORE
/* Definitions of four functions to help convert 'N103' format and earlier files formerly
appeared here. We keep the prototypes of all and body of one as examples for future use. */

static void OldGetSlurContext(Document *, LINK, Point [], Point []);
static void ConvertChordSlurs(Document *);
static void ConvertModNRVPositions(Document *, LINK);
static void ConvertStaffLines(LINK startL);

/* Convert old staff <oneLine> field to new <showLines> field. Also initialize new
<showLedgers> field.
		Old ASTAFF had this:
			char			filler:7,
							oneLine:1;
		New ASTAFF has this:
			char			filler:3,
							showLedgers:1,
							showLines:4;
<filler> was initialized to zero in InitStaff, so we can just see if showLines is
non-zero. If it is, then the oneLine flag was set. 	-JGG, 7/22/01 */

static void ConvertStaffLines(LINK startL)
{
	LINK pL, aStaffL;

	for (pL = startL; pL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==STAFFtype) {
			for (aStaffL = FirstSubLINK(pL); aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				Boolean oneLine = (StaffSHOWLINES(aStaffL)!=0);		/* NB: this really is "oneLine"! */
				if (oneLine)
					StaffSHOWLINES(aStaffL) = 1;
				else
					StaffSHOWLINES(aStaffL) = SHOW_ALL_LINES;
				StaffSHOWLEDGERS(aStaffL) = True;
#ifdef STAFFRASTRAL
				// NB: Bad if NightStaffSizer used! See StaffSize2Rastral in ResizeStaff.c
				// from NightStaffSizer code as a way to a possible solution.
				StaffRASTRAL(aStaffL) = doc->srastral;
#endif
			}
		}
	}
}

#endif

/* --------------------------------------------------------------- ConvertScoreContent -- */
/* Any file-format-conversion code that doesn't affect the length of the header or
lengths or offsets of fields in objects (or subobjects) should go here. Return True if
all goes well, False if not.

This function should not be called until the headers and the entire object list have been
read! Tweaks that affect lengths or offsets to the headers should be done in OpenFile; to
objects or subobjects, in ReadHeaps. */

static Boolean ConvertScoreContent(Document *doc, long fileTime)
{
	if (version<='N105') {
		LogPrintf(LOG_NOTICE, "Can't convert 'N105' format files yet.\n");
		// ??DO A LOT!!!!!!!
	}

	/* Make sure all staves are visible in Master Page. They should never be invisible,
	but (as of v.997), they sometimes were, probably because not exporting changes to
	Master Page was implemented by reconstructing it from the 1st system of the score.
	That was fixed in about .998a10!, so it should certainly be safe to remove this call,
	but the thought makes me nervous, and making them visible here shouldn't cause any
	problems and never has. */

	VisifyMasterStaves(doc);
	
	return True;
}


/* ----------------------------------------------------------------------- ModifyScore -- */
/* Any temporary file-content-updating code (a.k.a. hacking) that doesn't affect the
length of the header or lengths of objects should go here. This function should only be
called after the header and entire object list have been read. Return True if all goes
well, False if not.

NB: If code here considers changing something, and especially if it ends up actually
doing so, it should call LogPrintf to display at least one very prominent message
in the console window, and SysBeep to draw attention to it! It should perhaps also
set doc->changed, though this will make it easier for people to accidentally overwrite
the original version.

NB2: Be sure that all of this code is removed or commented out in ordinary versions!
To facilitate that, when done with hacking, add an "#error" line; cf. examples
below. */

#ifdef SWAP_STAVES
#error ATTEMPTED TO COMPILE OLD HACKING CODE for ModifyScore!
static void ShowStfTops(Document *doc, LINK pL, short staffN1, short staffN2);
static void ShowStfTops(Document *doc, LINK pL, short staffN1, short staffN2)
{
	CONTEXT context;
	short staffTop1, staffTop2;
	pL = SSearch(doc->headL, STAFFtype, False);
	GetContext(doc, pL, staffN1, &context);
	staffTop1 =  context.staffTop;
	GetContext(doc, pL, staffN2, &context);
	staffTop2 =  context.staffTop;
	LogPrintf(LOG_INFO, "ShowStfTops(%d): staffTop1=%d, staffTop2=%d\n", pL, staffTop1, staffTop2);
}

static void SwapStaves(Document *doc, LINK pL, short staffN1, short staffN2);
static void SwapStaves(Document *doc, LINK pL, short staffN1, short staffN2)
{
	LINK aStaffL, aMeasureL, aPSMeasL, aClefL, aKeySigL, aTimeSigL, aNoteL,
			aTupletL, aRptEndL, aDynamicL, aGRNoteL;
	CONTEXT context;
	short staffTop1, staffTop2;

	switch (ObjLType(pL)) {
		case HEADERtype:
			break;

		case PAGEtype:
			break;

		case SYSTEMtype:
			break;

		case STAFFtype:
LogPrintf(LOG_INFO, "  Staff L%d\n", pL);
			GetContext(doc, pL, staffN1, &context);
			staffTop1 =  context.staffTop;
			GetContext(doc, pL, staffN2, &context);
			staffTop2 =  context.staffTop;
LogPrintf(LOG_INFO, "    staffTop1, 2=%d, %d\n", staffTop1, staffTop2);
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				if (StaffSTAFF(aStaffL)==staffN1) {
					StaffSTAFF(aStaffL) = staffN2;
					StaffTOP(aStaffL) = staffTop2;
				}
				else if (StaffSTAFF(aStaffL)==staffN2) {
					StaffSTAFF(aStaffL) = staffN1;
					StaffTOP(aStaffL) = staffTop1;
				}
//GetContext(doc, pL, staffN1, &context);
//LogPrintf(LOG_INFO, "(1)    pL=%d staffTop1=%d\n", pL, staffTop1);
			}
//GetContext(doc, pL, staffN1, &context);
//LogPrintf(LOG_INFO, "(2)    pL=%d staffTop1=%d\n", pL, staffTop1);
//ShowStfTops(doc, pL, staffN1, staffN2);
			break;

		case CONNECTtype:
			break;

		case MEASUREtype:
LogPrintf(LOG_INFO, "  Measure L%d\n", pL);
			aMeasureL = FirstSubLINK(pL);
			for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
				if (MeasureSTAFF(aMeasureL)==staffN1) MeasureSTAFF(aMeasureL) = staffN2;
				else if (MeasureSTAFF(aMeasureL)==staffN2) MeasureSTAFF(aMeasureL) = staffN1;
			}
			break;

		case PSMEAStype:
LogPrintf(LOG_INFO, "  PSMeas L%d\n", pL);
			aPSMeasL = FirstSubLINK(pL);
			for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL)) {
				if (PSMeasSTAFF(aPSMeasL)==staffN1) PSMeasSTAFF(aPSMeasL) = staffN2;
				else if (PSMeasSTAFF(aPSMeasL)==staffN2) PSMeasSTAFF(aPSMeasL) = staffN1;
			}
			break;

		case CLEFtype:
LogPrintf(LOG_INFO, "  Clef L%d\n", pL);
			aClefL = FirstSubLINK(pL);
			for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
				if (ClefSTAFF(aClefL)==staffN1) ClefSTAFF(aClefL) = staffN2;
				else if (ClefSTAFF(aClefL)==staffN2) ClefSTAFF(aClefL) = staffN1;
			}
			break;

		case KEYSIGtype:
LogPrintf(LOG_INFO, "  Keysig L%d\n", pL);
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
				if (KeySigSTAFF(aKeySigL)==staffN1) KeySigSTAFF(aKeySigL) = staffN2;
				else if (KeySigSTAFF(aKeySigL)==staffN2) KeySigSTAFF(aKeySigL) = staffN1;
			}
			break;

		case TIMESIGtype:
LogPrintf(LOG_INFO, "  Timesig L%d\n", pL);
			aTimeSigL = FirstSubLINK(pL);
			for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
				if (TimeSigSTAFF(aTimeSigL)==staffN1) TimeSigSTAFF(aTimeSigL) = staffN2;
				else if (TimeSigSTAFF(aTimeSigL)==staffN2) TimeSigSTAFF(aTimeSigL) = staffN1;
			}
			break;

		case SYNCtype:
LogPrintf(LOG_INFO, "  Sync L%d\n", pL);
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				if (NoteSTAFF(aNoteL)==staffN1) {
					NoteSTAFF(aNoteL) = staffN2;
					NoteVOICE(aNoteL) = staffN2;		/* Assumes only 1 voice on the staff */
				}
				else if (NoteSTAFF(aNoteL)==staffN2) {
					NoteSTAFF(aNoteL) = staffN1;
					NoteVOICE(aNoteL) = staffN1;		/* Assumes only 1 voice on the staff */
				}
			}
			break;

		case BEAMSETtype:
LogPrintf(LOG_INFO, "  Beamset L%d\n", pL);
			if (BeamSTAFF(pL)==staffN1) BeamSTAFF((pL)) = staffN2;
			else if (BeamSTAFF((pL))==staffN2) BeamSTAFF((pL)) = staffN1;
			break;

		case TUPLETtype:
LogPrintf(LOG_INFO, "  Tuplet L%d\n", pL);
			if (TupletSTAFF(pL)==staffN1) TupletSTAFF((pL)) = staffN2;
			else if (TupletSTAFF((pL))==staffN2) TupletSTAFF((pL)) = staffN1;
			break;

/*
		case RPTENDtype:
			?? = FirstSubLINK(pL);
			for (??) {
			}
			break;

		case ENDINGtype:
			?!
			break;
*/
		case DYNAMtype:
LogPrintf(LOG_INFO, "  Dynamic L%d\n", pL);
			aDynamicL = FirstSubLINK(pL);
			for ( ; aDynamicL; aDynamicL=NextDYNAMICL(aDynamicL)) {
				if (DynamicSTAFF(aDynamicL)==staffN1) DynamicSTAFF(aDynamicL) = staffN2;
				else if (DynamicSTAFF(aDynamicL)==staffN2) DynamicSTAFF(aDynamicL) = staffN1;
			}
			break;

		case GRAPHICtype:
LogPrintf(LOG_INFO, "  Graphic L%d\n", pL);
			if (GraphicSTAFF(pL)==staffN1) GraphicSTAFF((pL)) = staffN2;
			else if (GraphicSTAFF((pL))==staffN2) GraphicSTAFF((pL)) = staffN1;
			break;

/*
		case OTTAVAtype:
			?!
			break;
*/
		case SLURtype:
LogPrintf(LOG_INFO, "  Slur L%d\n", pL);
			if (SlurSTAFF(pL)==staffN1) SlurSTAFF((pL)) = staffN2;
			else if (SlurSTAFF((pL))==staffN2) SlurSTAFF((pL)) = staffN1;
			break;

/*
		case GRSYNCtype:
			?? = FirstSubLINK(pL);
			for (??) {
			}
			break;

		case TEMPOtype:
			?!
			break;

		case SPACERtype:
			?!
			break;
*/

		default:
			break;	
	}
}
#endif


static Boolean ModifyScore(Document * /*doc*/, long /*fileTime*/)
{
#ifdef SWAP_STAVES
#error ModifyScore: ATTEMPTED TO COMPILE OLD HACKING CODE!
	/* DAB carelessly put a lot of time into orchestrating his violin concerto with a
		template having the trumpet staff above the horn; this is intended to correct
		that.
		NB: To swap two staves, in addition to running this code, use Master Page to:
			1. Fix the staves' vertical positions
			2. If the staves are Grouped, un-Group and re-Group
			3. Swap the Instrument info
		NB2: If there's more than one voice on either of the staves, this is not
		likely to work at all well. It never worked well enough to be useful for the
		score I wrote it for.
																--DAB, Jan. 2016 */
	
	short staffN1 = 5, staffN2 = 6;
	LINK pL;
	
	SysBeep(1);
	LogPrintf(LOG_NOTICE, "ModifyScore: SWAPPING STAVES %d AND %d (of %d) IN MASTER PAGE....\n",
				staffN1, staffN2, doc->nstaves);
	for (pL = doc->masterHeadL; pL!=doc->masterTailL; pL = RightLINK(pL)) {
		SwapStaves(doc, pL, staffN1, staffN2);
	}
	LogPrintf(LOG_NOTICE, "ModifyScore: SWAPPING STAVES %d AND %d (of %d) IN SCORE OBJECT LIST....\n",
				staffN1, staffN2, doc->nstaves);
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		SwapStaves(doc, pL, staffN1, staffN2);
	}
  	doc->changed = True;

#endif

#ifdef FIX_MASTERPAGE_SYSRECT
#error ModifyScore: ATTEMPTED TO COMPILE OLD FILE-HACKING CODE!
	/* This is to fix a score David Gottlieb is working on, in which Ngale draws
	 * _completely_ blank pages. Nov. 1999.
	 */
	 
	LINK sysL, staffL, aStaffL; DDIST topStaffTop; PASTAFF aStaff;

	LogPrintf(LOG_NOTICE, "ModifyScore: fixing Master Page sysRects and staffTops...\n");
	// Browser(doc,doc->masterHeadL, doc->masterTailL);
	sysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
	SystemRECT(sysL).bottom = SystemRECT(sysL).top+pt2d(72);

	staffL = SSearch(doc->masterHeadL, STAFFtype, False);
	aStaffL = FirstSubLINK(staffL);
	topStaffTop = pt2d(24);
	/* The following assumes staff subobjects are in order from top to bottom! */
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		aStaff->staffTop = topStaffTop;
		topStaffTop += pt2d(36);
	}
  	doc->changed = True;
#endif

#ifdef FIX_UNBEAMED_FLAGS_AUGDOT_PROB
#error ModifyScore: ATTEMPTED TO COMPILE OLD FILE-HACKING CODE!
	short alteredCount; LINK pL;

  /* From SetupNote in Objects.c:
   * "On upstemmed notes with flags, xmovedots should really 
   * be 2 or 3 larger [than normal], but then it should change whenever 
   * such notes are beamed or unbeamed or their stem direction or 
   * duration changes." */

  /* TC Oct 7 1999 (modified by DB, 8 Oct.): quick and dirty hack to fix this problem
   * for Bill Hunt; on opening file, look at all unbeamed dotted notes with subtype >
   * quarter note and if they have stem up, increase xmovedots by 2; if we find any
   * such notes, set doc-changed to True then simply report to user. It'd probably
   * be better to consider stem length and not do this for notes with very long
   * stems, but we don't consider that. */

  LogPrintf(LOG_NOTICE, "ModifyScore: fixing augdot positions for unbeamed upstemmed notes with flags...\n");
  alteredCount = 0;
  for (pL = doc->headL; pL; pL = RightLINK(pL)) 
    if (ObjLType(pL)==SYNCtype) {
      LINK aNoteL = FirstSubLINK(pL);
      for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
        PANOTE aNote = GetPANOTE(aNoteL);
        if((aNote->subType > QTR_L_DUR) && (aNote->ndots))
          if(!aNote->beamed)
            if(MainNote(aNoteL) && (aNote->yd > aNote->ystem)) {
              aNote->xmovedots = 3 + WIDEHEAD(aNote->subType) + 2;
              alteredCount++;
            }
      }
    }
  
  if (alteredCount) {
  	doc->changed = True;

    NumToString((long)alteredCount,(unsigned char *)strBuf);
    ParamText((unsigned char *)strBuf, 
      (unsigned char *)"\p unbeamed notes with stems up found; aug-dots corrected. NB Save the file with a new name!", 
        (unsigned char *)"", (unsigned char *)"");
    CautionInform(GENERIC2_ALRT);
  }
#endif

#ifdef FIX_BLACK_SCORE
#error ModifyScore: ATTEMPTED TO COMPILE OLD FILE-HACKING CODE!
	/* Sample trashed-file-fixing hack: in this case, to fix an Arnie Black score. */
	
	for (pL = doc->headL; pL; pL = RightLINK(pL)) 
		if (pL>=492 && ObjLType(pL)==SYNCtype) {
			LINK aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				NoteBEAMED(aNoteL) = False;
		}
#endif

	return True;
}


static void ConvertDocumentHeader(Document *doc, DocumentN105 *docN105)
{
	doc->origin = docN105->origin;
	doc->paperRect = docN105->paperRect;
	doc->origPaperRect = docN105->origPaperRect;
	doc->holdOrigin = docN105->holdOrigin;
	doc->marginRect = docN105->marginRect;
	doc->sheetOrigin = docN105->sheetOrigin;
	
	doc->currentSheet = docN105->currentSheet;
	doc->numSheets = docN105->numSheets;
	doc->firstSheet = docN105->firstSheet;
	doc->firstPageNumber = docN105->firstPageNumber;
	doc->startPageNumber = docN105->startPageNumber;
	doc->numRows = docN105->numRows;
	doc->numCols = docN105->numCols;
	doc->pageType = docN105->pageType;
	doc->measSystem = docN105->measSystem;
	doc->headerFooterMargins = docN105->headerFooterMargins;
	doc->currentPaper = docN105->currentPaper;
}

static void ConvertFontTable(Document *doc, DocumentN105 *docN105)
{
	short i;
	
	for (i = 0; i<doc->nfontsUsed; i++)
		doc->fontTable[i] = docN105->fontTable[i];
	doc->nfontsUsed = docN105->nfontsUsed;
}


static short ConvertScoreHeader(Document *doc, DocumentN105 *docN105)
{
	doc->headL = docN105->headL;
	doc->tailL = docN105->tailL;
	doc->selStartL = docN105->selStartL;
	doc->selEndL = docN105->selEndL;
	doc->nstaves = docN105->nstaves;
	doc->nsystems = docN105->nsystems;
	strcpy((char *)doc->comment, (char *)docN105->comment);
	doc->feedback = docN105->feedback;
	doc->dontSendPatches = docN105->dontSendPatches;
	doc->saved = docN105->saved;
	doc->named = docN105->named;
	doc->used = docN105->used;
	doc->transposed = docN105->transposed;
	doc->lyricText = docN105->lyricText;
	doc->polyTimbral = docN105->polyTimbral;
	doc->currentPage = docN105->currentPage;
	doc->spacePercent = docN105->spacePercent;
	doc->srastral = docN105->srastral;
	doc->altsrastral = docN105->altsrastral;
	doc->tempo = docN105->tempo;
	doc->channel = docN105->channel;
	doc->velocity = docN105->velocity;
	doc->headerStrOffset = docN105->headerStrOffset;
	doc->footerStrOffset = docN105->footerStrOffset;
	doc->topPGN = docN105->topPGN;
	doc->hPosPGN = docN105->hPosPGN;
	doc->alternatePGN = docN105->alternatePGN;
	doc->useHeaderFooter = docN105->useHeaderFooter;
	doc->fillerPGN = docN105->fillerPGN;
	doc->fillerMB = docN105->fillerMB;
	doc->filler2 = docN105->filler2;
	doc->dIndentOther = docN105->dIndentOther;
	doc->firstNames = docN105->firstNames;
	doc->otherNames = docN105->otherNames;
	doc->lastGlobalFont = docN105->lastGlobalFont;
	doc->xMNOffset = docN105->xMNOffset;
	doc->yMNOffset = docN105->yMNOffset;
	doc->xSysMNOffset = docN105->xSysMNOffset;
	doc->aboveMN = docN105->aboveMN;
	doc->sysFirstMN = docN105->sysFirstMN;
	doc->startMNPrint1 = docN105->startMNPrint1;
	doc->firstMNNumber = docN105->firstMNNumber;
	doc->masterHeadL = docN105->masterHeadL;
	doc->masterTailL = docN105->masterTailL;
	doc->filler1 = docN105->filler1;
	doc->nFontRecords = docN105->nFontRecords;
	
	Pstrcpy(doc->fontNameMN, docN105->fontNameMN);			/* Measure no. font */
	doc->fillerMN = docN105->fillerMN;
	doc->lyricMN = docN105->lyricMN;
	doc->enclosureMN = docN105->enclosureMN;
	doc->relFSizeMN = docN105->relFSizeMN;
	doc->fontSizeMN = docN105->fontSizeMN;
	doc->fontStyleMN = docN105->fontStyleMN;
	
	Pstrcpy(doc->fontNamePN, docN105->fontNamePN);			/* Part name font */
	doc->fillerPN = docN105->fillerPN;
	doc->lyricPN = docN105->lyricPN;
	doc->enclosurePN = docN105->enclosurePN;
	doc->relFSizePN = docN105->relFSizePN;
	doc->fontSizePN = docN105->fontSizePN;
	doc->fontStylePN = docN105->fontStylePN;
	
	Pstrcpy(doc->fontNameRM, docN105->fontNameRM);			/* Rehearsal mark font */
	doc->fillerRM = docN105->fillerRM;
	doc->lyricRM = docN105->lyricRM;
	doc->enclosureRM = docN105->enclosureRM;
	doc->relFSizeRM = docN105->relFSizeRM;
	doc->fontSizeRM = docN105->fontSizeRM;
	doc->fontStyleRM = docN105->fontStyleRM;
	
	Pstrcpy(doc->fontName1, docN105->fontName1);			/* Regular font 1 */
	doc->fillerR1 = docN105->fillerR1;
	doc->lyric1 = docN105->lyric1;
	doc->enclosure1 = docN105->enclosure1;
	doc->relFSize1 = docN105->relFSize1;
	doc->fontSize1 = docN105->fontSize1;
	doc->fontStyle1 = docN105->fontStyle1;
	
	Pstrcpy(doc->fontName2, docN105->fontName2);			/* Regular font 2 */
	doc->fillerR2 = docN105->fillerR2;
	doc->lyric2 = docN105->lyric2;
	doc->enclosure2 = docN105->enclosure2;
	doc->relFSize2 = docN105->relFSize2;
	doc->fontSize2 = docN105->fontSize2;
	doc->fontStyle2 = docN105->fontStyle2;
	
	Pstrcpy(doc->fontName3, docN105->fontName3);			/* Regular font 3 */
	doc->fillerR3 = docN105->fillerR3;
	doc->lyric3 = docN105->lyric3;
	doc->enclosure3 = docN105->enclosure3;
	doc->relFSize3 = docN105->relFSize3;
	doc->fontSize3 = docN105->fontSize3;
	doc->fontStyle3 = docN105->fontStyle3;
	
	Pstrcpy(doc->fontName4, docN105->fontName4);			/* Regular font 4 */
	doc->fillerR4 = docN105->fillerR4;
	doc->lyric4 = docN105->lyric4;
	doc->enclosure4 = docN105->enclosure4;
	doc->relFSize4 = docN105->relFSize4;
	doc->fontSize4 = docN105->fontSize4;
	doc->fontStyle4 = docN105->fontStyle4;
	
	Pstrcpy(doc->fontNameTM, docN105->fontNameTM);			/* Tempo mark font */
	doc->fillerTM = docN105->fillerTM;
	doc->lyricTM = docN105->lyricTM;
	doc->enclosureTM = docN105->enclosureTM;
	doc->relFSizeTM = docN105->relFSizeTM;
	doc->fontSizeTM = docN105->fontSizeTM;
	doc->fontStyleTM = docN105->fontStyleTM;
	
	Pstrcpy(doc->fontNameCS, docN105->fontNameCS);			/* Chord symbol font */
	doc->fillerCS = docN105->fillerCS;
	doc->lyricCS = docN105->lyricCS;
	doc->enclosureCS = docN105->enclosureCS;
	doc->relFSizeCS = docN105->relFSizeCS;
	doc->fontSizeCS = docN105->fontSizeCS;
	doc->fontStyleCS = docN105->fontStyleCS;
	
	Pstrcpy(doc->fontNamePG, docN105->fontNamePG);			/* Page header/footer/no. font */
	doc->fillerPG = docN105->fillerPG;
	doc->lyricPG = docN105->lyricPG;
	doc->enclosurePG = docN105->enclosurePG;
	doc->relFSizePG = docN105->relFSizePG;
	doc->fontSizePG = docN105->fontSizePG;
	doc->fontStylePG = docN105->fontStylePG;
	
	Pstrcpy(doc->fontName5, docN105->fontName5);			/* Regular font 5 */
	doc->fillerR5 = docN105->fillerR5;
	doc->lyric5 = docN105->lyric5;
	doc->enclosure5 = docN105->enclosure5;
	doc->relFSize5 = docN105->relFSize5;
	doc->fontSize5 = docN105->fontSize5;
	doc->fontStyle5 = docN105->fontStyle5;
	
	Pstrcpy(doc->fontName6, docN105->fontName6);			/* Regular font 6 */
	doc->fillerR6 = docN105->fillerR6;
	doc->lyric6 = docN105->lyric6;
	doc->enclosure6 = docN105->enclosure6;
	doc->relFSize6 = docN105->relFSize6;
	doc->fontSize6 = docN105->fontSize6;
	doc->fontStyle6 = docN105->fontStyle6;
	
	Pstrcpy(doc->fontName7, docN105->fontName7);			/* Regular font 7 */
	doc->fillerR7 = docN105->fillerR7;
	doc->lyric7 = docN105->lyric7;
	doc->enclosure7 = docN105->enclosure7;
	doc->relFSize7 = docN105->relFSize7;
	doc->fontSize7 = docN105->fontSize7;
	doc->fontStyle7 = docN105->fontStyle7;
	
	Pstrcpy(doc->fontName8, docN105->fontName8);			/* Regular font 8 */
	doc->fillerR8 = docN105->fillerR8;
	doc->lyric8 = docN105->lyric8;
	doc->enclosure8 = docN105->enclosure8;
	doc->relFSize8 = docN105->relFSize8;
	doc->fontSize8 = docN105->fontSize8;
	doc->fontStyle8 = docN105->fontStyle8;
	
	Pstrcpy(doc->fontName9, docN105->fontName9);			/* Regular font 9 */
	doc->fillerR9 = docN105->fillerR9;
	doc->lyric9 = docN105->lyric9;
	doc->enclosure9 = docN105->enclosure9;
	doc->relFSize9 = docN105->relFSize9;
	doc->fontSize9 = docN105->fontSize9;
	doc->fontStyle9 = docN105->fontStyle9;
	
	doc->nfontsUsed = docN105->nfontsUsed;
	ConvertFontTable(doc, docN105);
	
	Pstrcpy(doc->musFontName, docN105->musFontName);
	
	doc->magnify = docN105->magnify;
	doc->selStaff = docN105->selStaff;
	doc->otherMNStaff = docN105->otherMNStaff;
	doc->numberMeas = docN105->numberMeas;
	doc->currentSystem = docN105->currentSystem;
	doc->spaceTable = docN105->spaceTable;
	doc->htight = docN105->htight;
	doc->fillerInt = docN105->fillerInt;
	doc->lookVoice = docN105->lookVoice;
	doc->fillerHP = docN105->fillerHP;
	doc->fillerLP = docN105->fillerLP;
	doc->ledgerYSp = docN105->ledgerYSp;
	doc->deflamTime = docN105->deflamTime;
	
	doc->autoRespace = docN105->autoRespace;
	doc->insertMode = docN105->insertMode;
	doc->beamRests = docN105->beamRests;
	doc->pianoroll = docN105->pianoroll;
	doc->showSyncs = docN105->showSyncs;
	doc->frameSystems = docN105->frameSystems;
	doc->fillerEM = docN105->fillerEM;
	doc->colorVoices = docN105->colorVoices;
	doc->showInvis = docN105->showInvis;
	doc->showDurProb = docN105->showDurProb;
	doc->recordFlats = docN105->recordFlats;
	
	//longspaceMap[MAX_L_DUR]
	doc->dIndentFirst = docN105->dIndentFirst;
	doc->yBetweenSys = docN105->yBetweenSys;
	//voiceTab[MAXVOICES+1]
	//expansion[256-(MAXVOICES+1)]

	return 0;	// return 1729;		// ??TO BE IMPLEMENTED!!!!!!!
}


/* --------------------------------------------------------------------- SetTimeStamps -- */
/* Recompute timestamps up to the first Measure that has any unknown durs. in it. */

static void SetTimeStamps(Document *doc)
{
	LINK firstUnknown, stopMeas;
	
	firstUnknown = FindUnknownDur(doc->headL, GO_RIGHT);
	stopMeas = (firstUnknown? SSearch(firstUnknown, MEASUREtype, GO_LEFT)
								:  doc->tailL);
	FixTimeStamps(doc, doc->headL, LeftLINK(stopMeas));
}


/* ----------------------------------------------------------- Utilities for OpenFile  -- */

#define in2d(x)	pt2d(in2pt(x))		/* Convert inches to DDIST */

/* Display most Document header fields; there are about 18 in all. */

static void DisplayDocumentHdr(short id, Document *doc)
{
	LogPrintf(LOG_INFO, "Displaying Document header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  origin.h=%d", doc->origin.h);
	LogPrintf(LOG_INFO, "  .v=%d", doc->origin.h);
	
	LogPrintf(LOG_INFO, "  paperRect.top=%d", doc->paperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->paperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->paperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->paperRect.right);
	LogPrintf(LOG_INFO, "  origPaperRect.top=%d", doc->origPaperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->origPaperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->origPaperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->origPaperRect.right);
	LogPrintf(LOG_INFO, "  marginRect.top=%d", doc->marginRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->marginRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->marginRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->marginRect.right);

	LogPrintf(LOG_INFO, "  currentSheet=%d", doc->currentSheet);
	LogPrintf(LOG_INFO, "  numSheets=%d", doc->numSheets);
	LogPrintf(LOG_INFO, "  firstSheet=%d", doc->firstSheet);
	LogPrintf(LOG_INFO, "  firstPageNumber=%d", doc->firstPageNumber);
	LogPrintf(LOG_INFO, "  startPageNumber=%d\n", doc->startPageNumber);
	
	LogPrintf(LOG_INFO, "  numRows=%d", doc->numRows);
	LogPrintf(LOG_INFO, "  numCols=%d", doc->numCols);
	LogPrintf(LOG_INFO, "  pageType=%d", doc->pageType);
	LogPrintf(LOG_INFO, "  measSystem=%d\n", doc->measSystem);	
}

/* Do a reality check for Document header values that might be bad. Return the number of
problems found. NB: We're not checking anywhere near everything we could! */

static short CheckDocumentHdr(Document *doc)
{
	short nerr = 0;
	
	//if (!RectIsValid(doc->paperRect, 4??, in2pt(5)??)) nerr++;
	//if (!RectIsValid(doc->origPaperRect, 4??, in2pt(5)??)) nerr++;
	//if (!RectIsValid(doc->marginRect, 4??, in2pt(5)??)) nerr++;
	if (doc->numSheets<1 || doc->numSheets>250) nerr++;
	if (doc->firstPageNumber<0 || doc->firstPageNumber>250) nerr++;
	if (doc->startPageNumber<0 || doc->startPageNumber>250) nerr++;
	if (doc->numRows < 1 || doc->numRows > 250) nerr++;
	if (doc->numCols < 1 || doc->numCols > 250) nerr++;
	if (doc->pageType < 0 || doc->pageType > 20) nerr++;
	
	return nerr;
}

/* Display some Score header fields, but nowhere near all: there are about 200. */

static void DisplayScoreHdr(short id, Document *doc)
{
	unsigned char tempFontName[32];
	
	LogPrintf(LOG_INFO, "Displaying Score header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  nstaves=%d", doc->nstaves);
	LogPrintf(LOG_INFO, "  nsystems=%d", doc->nsystems);		
	LogPrintf(LOG_INFO, "  spacePercent=%d", doc->spacePercent);
	LogPrintf(LOG_INFO, "  srastral=%d", doc->srastral);				
	LogPrintf(LOG_INFO, "  altsrastral=%d\n", doc->altsrastral);
		
	LogPrintf(LOG_INFO, "  tempo=%d", doc->tempo);		
	LogPrintf(LOG_INFO, "  channel=%d", doc->channel);			
	LogPrintf(LOG_INFO, "  velocity=%d", doc->velocity);		
	LogPrintf(LOG_INFO, "  headerStrOffset=%d", doc->headerStrOffset);	
	LogPrintf(LOG_INFO, "  footerStrOffset=%d", doc->footerStrOffset);	
	LogPrintf(LOG_INFO, "  dIndentOther=%d\n", doc->dIndentOther);
	
	LogPrintf(LOG_INFO, "  firstNames=%d", doc->firstNames);
	LogPrintf(LOG_INFO, "  otherNames=%d", doc->otherNames);
	LogPrintf(LOG_INFO, "  lastGlobalFont=%d\n", doc->lastGlobalFont);

	LogPrintf(LOG_INFO, "  aboveMN=%c", (doc->aboveMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  sysFirstMN=%c", (doc->sysFirstMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  startMNPrint1=%c", (doc->startMNPrint1? '1' : '0'));
	LogPrintf(LOG_INFO, "  firstMNNumber=%d\n", doc->firstMNNumber);

	LogPrintf(LOG_INFO, "  nfontsUsed=%d", doc->nfontsUsed);
	Pstrcpy(tempFontName, doc->musFontName);
	LogPrintf(LOG_INFO, "  musFontName='%s'\n", PtoCstr(tempFontName));
	
	LogPrintf(LOG_INFO, "  magnify=%d", doc->magnify);
	LogPrintf(LOG_INFO, "  selStaff=%d", doc->selStaff);
	LogPrintf(LOG_INFO, "  currentSystem=%d", doc->currentSystem);
	LogPrintf(LOG_INFO, "  spaceTable=%d", doc->spaceTable);
	LogPrintf(LOG_INFO, "  htight=%d\n", doc->htight);
	
	LogPrintf(LOG_INFO, "  lookVoice=%d", doc->lookVoice);
	LogPrintf(LOG_INFO, "  ledgerYSp=%d", doc->ledgerYSp);
	LogPrintf(LOG_INFO, "  deflamTime=%d", doc->deflamTime);
	LogPrintf(LOG_INFO, "  colorVoices=%d", doc->colorVoices);
	LogPrintf(LOG_INFO, "  dIndentFirst=%d\n", doc->dIndentFirst);
}

#define ERR(fn) { nerr++; LogPrintf(LOG_WARNING, " err #%d,", fn); if (firstErr==0) firstErr = fn; }

/* Do a reality check for Score header values that might be bad. Return False if any
problems are found. NB: We're not checking anywhere near everything we could! */

static Boolean CheckScoreHdr(Document *doc)
{
	short nerr = 0, firstErr = 0;
	
	if (doc->nstaves<1 || doc->nstaves>MAXSTAVES) ERR(1);
	if (doc->nsystems<1 || doc->nsystems>2000) ERR(2);
	if (doc->spacePercent<MINSPACE || doc->spacePercent>MAXSPACE) ERR(3);
	if (doc->srastral<0 || doc->srastral>MAXRASTRAL) ERR(4);
	if (doc->altsrastral<1 || doc->altsrastral>MAXRASTRAL) ERR(5);
	if (doc->tempo<MIN_BPM || doc->tempo>MAX_BPM) ERR(6);
	if (doc->channel<1 || doc->channel>MAXCHANNEL) ERR(7);
	if (doc->velocity<-127 || doc->velocity>127) ERR(8);

	//if (doc->headerStrOffset<?? || doc->headerStrOffset>??) ERR(9);
	//if (doc->footerStrOffset<?? || doc->footerStrOffset>??) ERR(10);
	
	if (doc->dIndentOther<0 || doc->dIndentOther>in2d(5)) ERR(11);
	if (doc->firstNames<0 || doc->firstNames>MAX_NAMES_TYPE) ERR(12);
	if (doc->otherNames<0 || doc->otherNames>MAX_NAMES_TYPE) ERR(13);
	if (doc->lastGlobalFont<FONT_THISITEMONLY || doc->lastGlobalFont>MAX_FONTSTYLENUM) ERR(14);
	if (doc->firstMNNumber<0 || doc->firstMNNumber>MAX_FIRSTMEASNUM) ERR(15);
	if (doc->nfontsUsed<1 || doc->nfontsUsed>MAX_SCOREFONTS) ERR(16);
	if (doc->magnify<MIN_MAGNIFY || doc->magnify>MAX_MAGNIFY) ERR(17);
	if (doc->selStaff<-1 || doc->selStaff>doc->nstaves) ERR(18);
	if (doc->currentSystem<1 || doc->currentSystem>doc->nsystems) ERR(19);
	if (doc->spaceTable<0 || doc->spaceTable>32767) ERR(20);
	if (doc->htight<MINSPACE || doc->htight>MAXSPACE) ERR(21);
	if (doc->lookVoice<-1 || doc->lookVoice>MAXVOICES) ERR(22);
	if (doc->ledgerYSp<0 || doc->ledgerYSp>40) ERR(23);
	if (doc->deflamTime<1 || doc->deflamTime>1000) ERR(24);
	if (doc->dIndentFirst<0 || doc->dIndentFirst>in2d(5)) ERR(25);
	
	if (nerr==0) {
		LogPrintf(LOG_NOTICE, "No errors found.  (CheckScoreHdr)\n");
		return True;
	}
	else {
		if (!DETAIL_SHOW) DisplayScoreHdr(4, doc);
		sprintf(strBuf, "%d error(s) found.", nerr);
		CParamText(strBuf, "", "", "");
		LogPrintf(LOG_ERR, " %d error(s) found.  (CheckScoreHdr)\n", nerr);
		StopInform(GENERIC_ALRT);
		return False;
	}
}


/* -------------------------------------------------------------------------- OpenFile -- */
/* Open and read in the specified file. If there's an error, normally (see comments in
OpenError) gives an error message, and returns <errCode>; else returns noErr (0). Also
sets *fileVersion to the Nightingale version that created the file. We assume doc exists
and has had standard fields initialized. FIXME: even though vRefNum is a parameter,
(routines called by) OpenFile assume the volume is already set! This should be changed. */

enum {
	LOW_VERSION_ERR=-999,
	HI_VERSION_ERR,
	HEADER_ERR,
	LASTTYPE_ERR,				/* Value for LASTtype in file is not we expect it to be */
	TOOMANYSTAVES_ERR,
	LOWMEM_ERR,
	BAD_VERSION_ERR
};

extern void StringPoolEndianFix(StringPoolRef pool);
extern short StringPoolProblem(StringPoolRef pool);

#include <ctype.h>

short OpenFile(Document *doc, unsigned char *filename, short vRefNum, FSSpec *pfsSpec,
																	long *fileVersion)
{
	short		errCode, refNum, strPoolErrCode;
	short 		errInfo=0,				/* Type of object being read or other info on error */
				lastType;
	long		count, stringPoolSize,
				fileTime;
	Boolean		fileIsOpen;
	OMSSignature omsDevHdr;
	long		fmsDevHdr;
	long		omsBufCount, omsDevSize;
	short		i, nDocErr;
	FInfo		fInfo;
	FSSpec 		fsSpec;
	long		cmHdr, cmBufCount, cmDevSize;
	FSSpec		*pfsSpecMidiMap;
	char		versionCString[5];
	DocumentN105 aDocN105;

	WaitCursor();

	fileIsOpen = False;
	
	fsSpec = *pfsSpec;
	errCode = FSpOpenDF(&fsSpec, fsRdWrPerm, &refNum );			/* Open the file */
	if (errCode == fLckdErr || errCode == permErr) {
		doc->readOnly = True;
		errCode = FSpOpenDF (&fsSpec, fsRdPerm, &refNum );		/* Try again - open the file read-only */
	}
	if (errCode) { errInfo = OPENcall; goto Error; }
	fileIsOpen = True;

	count = sizeof(version);									/* Read & check file version code */
	errCode = FSRead(refNum, &count, &version);
	FIX_END(version);
	if (errCode) { errInfo = VERSIONobj; goto Error;}
	*fileVersion = version;
	MacTypeToString(version, versionCString);
	
	/* If user has the secret keys down, pretend file is in current version. */
	
	if (ShiftKeyDown() && OptionKeyDown() && CmdKeyDown() && ControlKeyDown()) {
		LogPrintf(LOG_NOTICE, "IGNORING FILE'S VERSION CODE '%s'.\n", versionCString);
		GetIndCString(strBuf, FILEIO_STRS, 6);					/* "IGNORING FILE'S VERSION CODE!" */
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
		version = THIS_FILE_VERSION;
	}

	if (versionCString[0]!='N') { errCode = BAD_VERSION_ERR; goto Error; }
	if ( !isdigit(versionCString[1])
			|| !isdigit(versionCString[2])
			|| !isdigit(versionCString[3]) )
		 { errCode = BAD_VERSION_ERR; goto Error; }
	if (version<FIRST_FILE_VERSION) { errCode = LOW_VERSION_ERR; goto Error; }
	if (version>THIS_FILE_VERSION) { errCode = HI_VERSION_ERR; goto Error; }
	if (version!=THIS_FILE_VERSION) LogPrintf(LOG_NOTICE, "CONVERTING VERSION '%s' FILE.\n", versionCString);

//#define DIFF(addr1, addr2)	((long)(&addr1)-(long)(&addr2))
//if (DETAIL_SHOW) LogPrintf(LOG_DEBUG, "Offset of aDocN105.comment[0]=%lu, spacePercent=%lu, fillerMB=%lu, nFontRecords=%lu, nfontsUsed=%lu, yBetweenSys=%lu\n",
//DIFF(aDocN105.comment[0], aDocN105.headL), DIFF(aDocN105.spacePercent, aDocN105.headL), DIFF(aDocN105.fillerMB, aDocN105.headL), DIFF(aDocN105.nFontRecords, aDocN105.headL),
//DIFF(aDocN105.nfontsUsed, aDocN105.headL), DIFF(aDocN105.yBetweenSys, aDocN105.headL));

	if (DETAIL_SHOW) LogPrintf(LOG_INFO, "Header size for Document=%ld, for Score=%ld, for N105 Score=%ld\n",
		sizeof(DOCUMENTHDR), sizeof(SCOREHEADER), sizeof(SCOREHEADER_N105));

	count = sizeof(fileTime);									/* Time file was written */
	errCode = FSRead(refNum, &count, &fileTime);
	FIX_END(fileTime);
	if (errCode) { errInfo = VERSIONobj; goto Error; }
		
	/* Read and, if necessary, convert Document (i.e., Sheets) and Score headers. */
	
	switch(version) {
		case 'N104':
		case 'N105':
			count = sizeof(DOCUMENTHDR);
			errCode = FSRead(refNum, &count, &aDocN105.origin);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			ConvertDocumentHeader(doc, &aDocN105);
			if (DETAIL_SHOW) DisplayDocumentHdr(1, doc);
			
			count = sizeof(SCOREHEADER_N105);
			errCode = FSRead(refNum, &count, &aDocN105.headL);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			nDocErr = ConvertScoreHeader(doc, &aDocN105);
			if (DETAIL_SHOW) DisplayScoreHdr(1, doc);
			if (nDocErr!=0) {
				sprintf(strBuf, "%d error(s) found in Score header.", nDocErr);
				CParamText(strBuf, "", "", "");
				LogPrintf(LOG_ERR, "%d error(s) found in Score header.  (OpenFile)\n", nDocErr);
				goto HeaderError;
			}
			break;
		
		default:
			;
	}

	EndianFixDocumentHdr(doc);
	if (DETAIL_SHOW) DisplayDocumentHdr(2, doc);
	LogPrintf(LOG_NOTICE, "Checking Document header: ");
	nDocErr = CheckDocumentHdr(doc);
	if (nDocErr==0)
		LogPrintf(LOG_NOTICE, "No errors found.  (OpenFile)\n");
	else {
		if (!DETAIL_SHOW) DisplayDocumentHdr(3, doc);
		sprintf(strBuf, "%d error(s) found in Document header.", nDocErr);
		CParamText(strBuf, "", "", "");
		LogPrintf(LOG_ERR, "%d error(s) found in Document header.  (OpenFile)\n", nDocErr);
		goto HeaderError;
	}
	
//DisplayScoreHdr(2, doc);		// ??TEMPORARY, TO DEBUG INTEL VERSION!!!!
	EndianFixScoreHdr(doc);
	if (DETAIL_SHOW) DisplayScoreHdr(3, doc);
	LogPrintf(LOG_NOTICE, "Checking Score header: ");
	if (!CheckScoreHdr(doc)) {
		errCode = HEADER_ERR;
		errInfo = 0;
		goto Error;
	}

	count = sizeof(lastType);
	errCode = FSRead(refNum, &count, &lastType);
	if (errCode) { errInfo = HEADERobj; goto Error; }
	FIX_END(lastType);

	if (lastType!=LASTtype) {
		LogPrintf(LOG_ERR, "LAST OBJECT TYPE=%d BUT SHOULD BE %d.\n", lastType, LASTtype);	
		errCode = LASTTYPE_ERR;
		errInfo = HEADERobj;
		goto Error;
	}

	/* Read the string pool. */

	count = sizeof(stringPoolSize);
	errCode = FSRead(refNum, &count, &stringPoolSize);
	if (errCode) { errInfo = STRINGobj; goto Error; }
	FIX_END(stringPoolSize);
	LogPrintf(LOG_INFO, "stringPoolSize=%ld  (OpenFile)\n", stringPoolSize);
	if (doc->stringPool) DisposeStringPool(doc->stringPool);
	
	/* Allocate from the StringManager, not NewHandle, in case StringManager is tracking
	   its allocations. Then, since we're going to overwrite everything with stuff from
	   file below, we can resize it to what it was when saved. */
	
	doc->stringPool = NewStringPool();
	if (doc->stringPool == NULL) { errInfo = STRINGobj; goto Error; }
	SetHandleSize((Handle)doc->stringPool, stringPoolSize);
	
	HLock((Handle)doc->stringPool);
	errCode = FSRead(refNum, &stringPoolSize, *doc->stringPool);
	if (errCode) { errInfo = STRINGobj; goto Error; }
	HUnlock((Handle)doc->stringPool);
	SetStringPool(doc->stringPool);
	StringPoolEndianFix(doc->stringPool);
	strPoolErrCode = StringPoolProblem(doc->stringPool);
	if (strPoolErrCode!=0) {
		AlwaysErrMsg("The string pool is probably bad (code=%ld).  (OpenFile)", (long)strPoolErrCode);
		errInfo = STRINGobj; goto Error;
	}
	
	errCode = FSpGetFInfo(&fsSpec, &fInfo);
	if (errCode!=noErr) { errInfo = INFOcall; goto Error; }
	
	/* Read the subobject heaps and the object heap in the rest of the file, and if
	   necessary, convert them to the current format. */
	
	errCode = ReadHeaps(doc, refNum, version, fInfo.fdType);
	if (errCode) return errCode;

	/* An ancient comment here: "Be sure we have enough memory left for a maximum-size
	   segment and a bit more." Now we require a _lot_ more, though it may be pointless. */
	
	if (!PreflightMem(400)) { NoMoreMemory(); return LOWMEM_ERR; }
	
	ConvertScoreContent(doc, fileTime);			/* Do any conversion of old files needed */

	Pstrcpy(doc->name, filename);				/* Remember filename and vol refnum after scoreHead is overwritten */
	doc->vrefnum = vRefNum;
	doc->changed = False;
	doc->saved = True;							/* Has to be, since we just opened it! */
	doc->named = True;							/* Has to have a name, since we just opened it! */

	ModifyScore(doc, fileTime);					/* Do any hacking needed, ordinarily none */

	/* Read the document's OMS device numbers for each part. if "devc" signature block not
	   found at end of file, pack the document's omsPartDeviceList with default values. */
	
	omsBufCount = sizeof(OMSSignature);
	errCode = FSRead(refNum, &omsBufCount, &omsDevHdr);
	if (!errCode && omsDevHdr == 'devc') {
		omsBufCount = sizeof(long);
		errCode = FSRead(refNum, &omsBufCount, &omsDevSize);
		if (errCode) return errCode;
		if (omsDevSize!=(MAXSTAVES+1)*sizeof(OMSUniqueID)) return errCode;
		errCode = FSRead(refNum, &omsDevSize, &(doc->omsPartDeviceList[0]));
		if (errCode) return errCode;
	}
	else {
		for (i = 1; i<=LinkNENTRIES(doc->headL)-1; i++)
			doc->omsPartDeviceList[i] = config.defaultOutputDevice;
		doc->omsInputDevice = config.defaultInputDevice;
	}

	/* Read the FreeMIDI input device data. */
	
	doc->fmsInputDevice = noUniqueID;
	
	/* We're probably not supposed to play with these fields, but FreeMIDI is obsolete
	   anyway, so the only change worth making is to remove these statements some day.
	   --DAB, December 2019 */
	
	doc->fmsInputDestination.basic.destinationType = 0,
	doc->fmsInputDestination.basic.name[0] = 0;
	count = sizeof(long);
	errCode = FSRead(refNum, &count, &fmsDevHdr);
	if (!errCode) {
		if (fmsDevHdr==FreeMIDISelector) {
			count = sizeof(fmsUniqueID);
			errCode = FSRead(refNum, &count, &doc->fmsInputDevice);
			if (errCode) return errCode;
			count = sizeof(fmsDestinationMatch);
			errCode = FSRead(refNum, &count, &doc->fmsInputDestination);
			if (errCode) return errCode;
		}
	}
	
	if (version >= 'N105') {
		cmBufCount = sizeof(long);
		errCode = FSRead(refNum, &cmBufCount, &cmHdr);
		if (!errCode && cmHdr == 'cmdi') {
			cmBufCount = sizeof(long);
			errCode = FSRead(refNum, &cmBufCount, &cmDevSize);
			if (errCode) return errCode;
			if (cmDevSize!=(MAXSTAVES+1)*sizeof(MIDIUniqueID)) return errCode;
			errCode = FSRead(refNum, &cmDevSize, &(doc->cmPartDeviceList[0]));
			if (errCode) return errCode;
		}
	}
	else {
		for (i = 1; i<=LinkNENTRIES(doc->headL)-1; i++)
			doc->cmPartDeviceList[i] = config.cmDfltOutputDev;
	}
	doc->cmInputDevice = config.cmDfltInputDev;

	errCode = FSClose(refNum);
	if (errCode) { errInfo = CLOSEcall; goto Error; }
	
	if (!IsDocPrintInfoInstalled(doc))
		InstallDocPrintInfo(doc);
	
	GetPrintHandle(doc, version, vRefNum, pfsSpec);		/* doc->name must be set before this */
	
	GetMidiMap(doc, pfsSpec);
	
	pfsSpecMidiMap = GetMidiMapFSSpec(doc);
	if (pfsSpecMidiMap != NULL) {
		OpenMidiMapFile(doc, pfsSpecMidiMap);
		ReleaseMidiMapFSSpec(doc);		
	}

	FillFontTable(doc);

	InitDocMusicFont(doc);

	SetTimeStamps(doc);									/* Up to first meas. w/unknown durs. */


	/* Assume that no information in the score having to do with window-relative
	   positions is valid. Besides clearing the object <valid> flags to indicate this,
	   protect ourselves against functions that might not check the flags properly by
	   setting all such positions to safe values now. */
	
	InvalRange(doc->headL, doc->tailL);					/* Force computing all objRects */

	doc->hasCaret = False;									/* Caret must still be set up */
	doc->selStartL = doc->headL;							/* Set selection range */								
	doc->selEndL = doc->tailL;								/*   to entire score, */
	DeselAllNoHilite(doc);									/*   deselect it */
	SetTempFlags(doc, doc, doc->headL, doc->tailL, False);	/* Just in case */

	if (ScreenPagesExceedView(doc))
		CautionInform(MANYPAGES_ALRT);

	ArrowCursor();	
	return 0;

HeaderError:
	sprintf(strBuf, "%d error(s) found in Document header.", nDocErr);
	CParamText(strBuf, "", "", "");
	LogPrintf(LOG_ERR, "%d error(s) found in Document header.  (OpenFile)\n", nDocErr);
	StopInform(GENERIC_ALRT);
	errCode = HEADER_ERR;
	errInfo = 0;
	/* drop through */
Error:
	OpenError(fileIsOpen, refNum, errCode, errInfo);
	return errCode;
}


/* ------------------------------------------------------------------------- OpenError -- */
/* Handle errors occurring while reading a file. <fileIsOpen> indicates  whether or
not the file was open when the error occurred; if so, OpenError closes it. <errCode>
is either an error code return by the file system manager or one of our own codes
(see enum above). <errInfo> indicates at what step of the process the error happened
(CREATEcall, OPENcall, etc.: see enum above), the type of object being read, or
some other additional information on the error. NB: If errCode==0, this will close
the file but not give an error message; I'm not sure that's a good idea.

Note that after a call to OpenError with fileIsOpen, you should not try to keep reading,
since the file will no longer be open! */

void OpenError(Boolean fileIsOpen,
					short refNum,
					short errCode,
					short errInfo)	/* More info: at what step error happened, type of obj being read, etc. */
{
	char aStr[256], fmtStr[256];
	StringHandle strHdl;
	short strNum;
	
	SysBeep(1);
	LogPrintf(LOG_ERR, "Can't open the file. errCode=%d errInfo=%d  (OpenError)\n", errCode, errInfo);
	if (fileIsOpen) FSClose(refNum);

	if (errCode!=0) {
		switch (errCode) {
			/*
			 * First handle our own codes that need special treatment.
			 */
			case BAD_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 7);			/* "file version is illegal" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case LOW_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 8);			/* "too old for this version of Nightingale" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case HI_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 9);			/* "newer than this version of Nightingale" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case TOOMANYSTAVES_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 10);		/* "this version can handle only %d staves" */
				sprintf(aStr, fmtStr, errInfo, MAXSTAVES);
				break;
			default:
				/*
				 * We expect descriptions of the common errors stored by code (negative
				 * values, for system errors; positive ones for our own I/O errors) in
				 * individual 'STR ' resources. If we find one for this error, print it,
				 * else just print the raw code.
				 */
				strHdl = GetString(errCode);
				if (strHdl) {
					Pstrcpy((unsigned char *)strBuf, (unsigned char *)*strHdl);
					PToCString((unsigned char *)strBuf);
					strNum = (errInfo>0? 15 : 16);	/* "%s (heap object type=%d)." : "%s (error code=%d)." */
					GetIndCString(fmtStr, FILEIO_STRS, strNum);
					sprintf(aStr, fmtStr, strBuf, errInfo);
				}
				else {
					strNum = (errInfo>0? 17 : 18);	/* "Error ID=%d (heap object type=%d)." : "Error ID=%d (error code=%d)." */
					GetIndCString(fmtStr, FILEIO_STRS, strNum);
					sprintf(aStr, fmtStr, errCode, errInfo);
				}
		}
		LogPrintf(LOG_WARNING, aStr); LogPrintf(LOG_WARNING, "\n");
		CParamText(aStr, "", "", "");
		StopInform(READ_ALRT);
	}
}
