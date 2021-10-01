/******************************************************************************************
	FILE:	NotelistParse.c
	PROJ:	Nightingale
	DESC:	Routines for parsing files in Nightingale's Notelist format into
			a temporary data structure. Written by John Gibson.
			For a full description of the original Notelist format, see "The
			Nightingale Notelist" by Tim Crawford et al, in E. Selfridge-Field,
			ed., Beyond MIDI: The Handbook of Musical Codes (MIT Press, 1997).
			The format documented there is as of v. 3.0A. Changes since then
			have been relatively minor: see the document NotelistTechUpdate05.txt
			for details.
*******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include <errno.h>
#include <ctype.h>
#include "Notelist.h"

// MAS
//#include <Fsp_fopen.h>
// MAS

/* -------------------------------------------------------------------------------------- */
/* Local prototypes */

static Boolean FSPreProcessNotelist(short refNum);
static Boolean FSProcessNotelist(short refNum);
static Boolean PostProcessNotelist(void);

static Boolean ParseNRGR(void);
static Boolean ParseTuplet(void);
static Boolean ParseBarline(void);
static Boolean ParseClef(void);
static Boolean ParseKeySig(void);
static Boolean ParseTimeSig(void);
static Boolean ParseTempoMark(void);
static Boolean ParseTextGraphic(void);
static Boolean ParseDynamic(void);
static Boolean ParseStructComment(void);

static Boolean ExtractVal(char *str, long *val);
static Boolean ExtractNoteFlags(char *flagStr, PNL_NRGR pNRGR);
static Boolean ExtractNoteMods(char *modStr, PNL_NRGR pNRGR);

static Boolean CheckNLNoteRests(void);
static Boolean CheckNLTuplets(void);
static Boolean CheckNLBarlines(void);
static Boolean CheckNLTimeSigs(void);
static Boolean AnalyzeNLTuplet(NLINK	tupletL);
static long NLSimpleLDur(NLINK pL);
static long CalcNLNoteLDur(NLINK noteL);

static void ReportParseFailure(char *functionName, short errCode);

/* Functions that call the Macintosh Toolbox */
static short FSNotelistVersion(short refNum);
static void PrintNotelistDS(void);
static NLINK StoreString(char str[]);
static NLINK StoreModifier(PNL_MOD pMod);
static Boolean AllocNotelistMemory(void);


/* -------------------------------------------------------------------------------------- */
/* Globals */

PNL_NODE	gNodeList;					/* list of notelist objects, dynamically allocated */
HNL_MOD		gHModList;					/* relocatable 1-based array of note modifiers */
Handle		gHStringPool;				/* relocatable block of null-terminated strings */

/* An ancient comment (Motorola 68000s have been gone for a long, long time): "Each
string in <gHStringPool> must begin at an even address, so that we won't crash on 68000
machines. Therefore, strings with an even number of chars (not including terminating
null) will require an additional null to pad them. Since we won't know how big this
block must be before parsing, we will have to expand it whenever we add a string. (At
least this is the easiest, if not the fastest, way.)" */

NLINK		gNumNLItems;				/* number of items in notelist, not including HEAD */
NLINK		gNextEmptyNode;				/* index into gNodeList of next empty node; advanced by each parser */
long		gLastTime;					/* value of time field in last Notelist record */
SignedByte	gPartStaves[MAXSTAVES+1];	/* 1-based array giving number of staves (value) in each part (index) */
SignedByte	gNumNLStaves;				/* number of staves in Notelist system */
short		gFirstMNNumber;				/* number of first measure */
Boolean		gDelAccs;					/* delete redundant accidentals? */
short		gHairpinCount;				/* number of hairpins found (and ignored) */

short		gNotelistVersion;			/* notelist format version number (>=0) */

/* Codes for tempo marks. We want to restrict notelist files to ASCII characters, so
some of these are different from the internal codes: cf. TempoGlyph(). */

char		gTempoCode[] = { '\0', 'b', 'w', 'h', 'q', 'e', 's', 'r', 'x', 'y' };

/* Static globals */

#define LINELEN	384					/* CAUTION: the %%header line can have as many as 192 chars w/ a 64-staff score! */
// ????But a graphic string can contain 255 in the string portion alone!!
// And what about really long (word-wrapped) comments?

static char		gInBuf[LINELEN];		/* Carefully read the comments above ReadLine before changing the buffer size. */
static long		gLineCount;

/* -------------------------------------------------------------------------------------- */
/* Other definitions */

/* The following codes refer to consecutive error messages that are in the middle of the
string list: this can make adding or changing messages tricky! Cf. ReportParseFailure(). */

static enum {
	NLERR_MISCELLANEOUS=1,	/* "Miscellaneous error" */
	NLERR_TOOFEWFIELDS,		/* "Less than the minimum number of fields for the opcode" */
	NLERR_INTERNAL,			/* "Internal error in Notelist parsing" */
	NLERR_BADTIME,			/* "t (time) is illegal" */
	NLERR_BADVOICE,			/* "v (voice no.) is illegal" */
	NLERR_BADPART,			/* "npt (part no.) is illegal" */
	NLERR_BADSTAFF,			/* "stf (staff no.) is illegal" */
	NLERR_BADDURCODE,		/* "dur (duration code) is illegal" */
	NLERR_BADDOTS,			/* "dots (no. of aug. dots) is illegal" */
	NLERR_BADNOTENUM,		/* (10) "nn (note number) is illegal" */
	NLERR_BADACC,			/* "acc (accidental code) is illegal" */
	NLERR_BADEACC,			/* "eAcc (effective accidental code) is illegal" */
	NLERR_BADPDUR,			/* "pDur (play duration) is illegal" */
	NLERR_BADVELOCITY,		/* "vel (MIDI velocity) is illegal" */
	NLERR_BADNOTEFLAGS,		/* "Note flags are illegal" */
	NLERR_BADAPPEAR,		/* "appear (appearance code) is illegal" */
	NLERR_BADMODS,			/* "Note modifiers on grace notes or illegal" */
	NLERR_BADNUMDENOM,		/* "num or denom (numerator or denominator) is illegal" */
	NLERR_BADTUPLETVIS,		/* "Numerator/denominator visibility combination is illegal" */
	NLERR_BADTYPE,			/* (20) "type is illegal" */
	NLERR_BADDTYPE,			/* "dType is illegal" */
	NLERR_BADNUMACC,		/* "KS (no. of accidentals) is illegal" */
	NLERR_BADKSACC,			/* "Key signature accidental code is illegal" */
	NLERR_BADDISPL			/* "displ (time signature appearance) is illegal" */
} E_NLErrs;


/* -------------------------------------------------------------------------------------- */
/* High level functions (ParseNotelistFile, PreProcessNotelist, ProcessNotelist,
	PostProcessNotelist). */

/* ----------------------------------------------------------------- ParseNotelistFile -- */

Boolean ParseNotelistFile(Str255 /*fileName*/, FSSpec *fsSpec)
{
	Boolean		ok, printNotelist = True;
	short		refNum;

	short errCode = FSOpenInputFile(fsSpec,&refNum);
	if (errCode != noErr) return False;
	
#if CNTLKEYFORPRINTNOTELIST
	if (!DETAIL_SHOW) printNotelist = False;
#endif

	WaitCursor();
	if (!printNotelist)
		ProgressMsg(CONVERTNOTELIST_PMSTR, " 1...");	/* "Converting Notelist file: step 1..." */

	ok = FSPreProcessNotelist(refNum);
	if (!ok) {
		FSCloseInputFile(refNum);
		goto Err;
	}
	
	ok = FSProcessNotelist(refNum);
	FSCloseInputFile(refNum);		// done with input file
	if (!ok) goto Err;

	ok = PostProcessNotelist();

#if PRINTNOTELIST
	if (printNotelist) PrintNotelistDS();
#endif

	return True;

Err:
	ProgressMsg(0, "");					/* Remove progress window, if it hasn't already been removed. */
	DisposNotelistMemory();

	return False;
}

static Boolean FSPreProcessNotelist(short refNum)
{
	char	firstChar;
	short	ans;
	Boolean	ok;
		
	gNotelistVersion = FSNotelistVersion(refNum);
	if (gNotelistVersion<0) return False;
	
	/* Count the objects we're interested in. */
	gNumNLItems = 0;
	while (FSReadLine(gInBuf, LINELEN, refNum)) {
		gLineCount++;

		ans = sscanf(gInBuf, "%c", &firstChar);
		if (ans<1) continue;							/* probably got a blank line */

		switch (firstChar) {
			case NOTE_CHAR:
			case GRACE_CHAR:
			case REST_CHAR:
			case TUPLET_CHAR:
			case BAR_CHAR:
			case CLEF_CHAR:
			case KEYSIG_CHAR:
			case TIMESIG_CHAR:
			case METRONOME_CHAR:
			case GRAPHIC_CHAR:
			case DYNAMIC_CHAR:
				gNumNLItems++;
				break;
		}
	}
	
	ok = AllocNotelistMemory();
	if (!ok) return False;
	
	FSRewind(refNum);

	return True;
}


/* ------------------------------------------------------------------- ProcessNotelist -- */

#define NOTELISTCODE_ALRT 310

static Boolean FSProcessNotelist(short refNum)
{
	short	ans;
	char	firstChar, secondChar;
	Boolean	ok = True;
	char	fmtStr[256], str[256];
	
	gLineCount = 0L;
	gNextEmptyNode = 0;
	gLastTime = 0L;
	
	while (FSReadLine(gInBuf, LINELEN, refNum)) {
		gLineCount++;

		ans = sscanf(gInBuf, "%c", &firstChar);
		if (ans<1) continue;									/* probably got a blank line */
		
		switch (firstChar) {
			case NOTE_CHAR:
			case GRACE_CHAR:
			case REST_CHAR:			ok = ParseNRGR();			break;
			case TUPLET_CHAR:		ok = ParseTuplet();			break;

			case BAR_CHAR:			ok = ParseBarline();		break;
			case CLEF_CHAR:			ok = ParseClef();			break;
			case KEYSIG_CHAR:		ok = ParseKeySig();			break;
			case TIMESIG_CHAR:		ok = ParseTimeSig();		break;

			case METRONOME_CHAR:	ok = ParseTempoMark();		break;
			case GRAPHIC_CHAR:		ok = ParseTextGraphic();	break;
			case DYNAMIC_CHAR:		ok = ParseDynamic();		break;

			case COMMENT_CHAR:		/* It might be a structured comment... */
				secondChar = '\0';
				sscanf(gInBuf, "%*c%c", &secondChar);
				if (secondChar==COMMENT_CHAR)
					ok = ParseStructComment();
				break;
			
			case BEAM_CHAR:
				GetIndCString(fmtStr, NOTELIST_STRS, 39);		/* "'%c' is an illegal first character ("opcode"). This may be..." */
				sprintf(str, fmtStr, gLineCount, firstChar);
				CParamText(str, "", "", ""); 
				ok = (CautionAdvise(NOTELISTCODE_ALRT)!=Cancel);
				break;
			
			default:
				GetIndCString(fmtStr, NOTELIST_STRS, 29);		/* "'%c' is an illegal first character ("opcode")." */
				sprintf(str, fmtStr, gLineCount, firstChar);
				CParamText(str, "", "", ""); 
				ok = (CautionAdvise(NOTELISTCODE_ALRT)!=Cancel);
		}
		if (!ok) return False;								/* This may be too drastic in some cases. */
	}
	
	LogPrintf(LOG_NOTICE, "Notelist file read: %ld lines.\n", gLineCount);
	return True;
}


/* --------------------------------------------------------------- PostProcessNotelist -- */
/* Fill in various fields that cannot be computed before we read all the Notelist data.
Returns True if ok, False if error. */

static Boolean PostProcessNotelist(void)
{
	Boolean	result, noErrors = True;
	
	result = CheckNLBarlines();
	if (!result) noErrors = False;
	
	result = CheckNLTuplets();							// _Must_ come before CheckNLNoteRests?
	if (!result) noErrors = False;
	
	result = CheckNLNoteRests();
	if (!result) noErrors = False;
	
	result = CheckNLTimeSigs();
	if (!result) noErrors = False;
	
	return noErrors;
}


/* -------------------------------------------------------------------------------------- */
/* Functions for parsing individual Notelist codes */

/* ------------------------------------------------------------------------- ParseNRGR -- */
/* Parse a line giving data for a normal note, grace note, or rest, e.g.:

N t=0 v=1 npt=1 stf=1 dur=4 dots=0 nn=72 acc=0 eAcc=3 pDur=456 vel=75 ...... appear=1
R t=480 v=1 npt=1 stf=1 dur=4 dots=0 ...... appear=1
R t=960 v=1 npt=1 stf=1 dur=4 dots=0 ...... appear=1 mods=10
G t=-1 v=1 npt=1 stf=1 dur=5 dots=0 nn=76 acc=0 eAcc=3 pDur=240 vel=75 . appear=1
N t=1440 v=1 npt=1 stf=1 dur=4 dots=0 nn=76 acc=0 eAcc=3 pDur=456 vel=75 ...... appear=1 mods=10,16

This is a bit tricky because these lines contain a variable number of fields.

If a note, rest or grace note owns any note modifiers, they are listed in a "mods" field
at the end of the line. (NB: As of v. 3.1, Nightingale doesn't support modifiers attached
to grace notes.) The mods field gives a list of modCodes separated by commas, with
optional data value trailing each modCode and separated from it by a colon. E.g.:
			mods=11				[just 1 modifier having modCode=11]
			mods=0,2,3			[3 modifiers]
			mods=3:50,2			[2 modifiers, the first having a data value of 50]

Also, rests omit the fields labelled "nn", "acc", "eAcc", "pDur" and "vel" above.

Finally, notes and rests have six flags, while grace notes have just one, as of
Nightingale 3.5b10. (See comments at ExtractNoteFlags for the meaning of these.) */

static Boolean ParseNRGR()
{
	char		objTypeCode, timeStr[32], voiceStr[32], partStr[32], staffStr[32], durStr[32],
				dotsStr[32], noteNumStr[32], accStr[32], eAccStr[32], pDurStr[32], velStr[32],
				flagsStr[32], appearStr[64], modStr[64];
	short		ans, err;
	long		along;
	PNL_NRGR	pNRGR;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }
	
	ans = sscanf(gInBuf, "%c", &objTypeCode);
	if (ans<1) { err = NLERR_INTERNAL; goto broken; }

	/* in case our record omits these fields... */
	noteNumStr[0] = accStr[0] = eAccStr[0] = pDurStr[0] = velStr[0] = modStr[0] = 0;

	switch (objTypeCode) {
		case NOTE_CHAR:
		case GRACE_CHAR:
			ans = sscanf(gInBuf, "%*c%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
							timeStr, voiceStr, partStr, staffStr, durStr, dotsStr, noteNumStr,
							accStr, eAccStr, pDurStr, velStr, flagsStr, appearStr, modStr);
			if (ans<13) { err = NLERR_TOOFEWFIELDS; goto broken; }
			break;
		case REST_CHAR:
			ans = sscanf(gInBuf, "%*c%s%s%s%s%s%s%s%s%s",
							timeStr, voiceStr, partStr, staffStr, durStr, dotsStr,
							flagsStr, appearStr, modStr);
			if (ans<8) { err = NLERR_TOOFEWFIELDS; goto broken; }
			break;
		default:
			err = NLERR_INTERNAL;
			goto broken;
	}

	pNRGR = GetPNL_NRGR(gNextEmptyNode);

	switch (objTypeCode) {
		case NOTE_CHAR:
		case REST_CHAR:		pNRGR->objType = NR_TYPE;		break;
		case GRACE_CHAR:	pNRGR->objType = GRACE_TYPE;	break;
	}

	pNRGR->isRest = (objTypeCode==REST_CHAR);
	
	err = NLERR_BADTIME;
	if (!ExtractVal(timeStr, &along)) goto broken;
	if (pNRGR->objType!=GRACE_TYPE && along<0L) goto broken;	/* grace note time ignored in Ngale 3.0 */
	pNRGR->lStartTime = along;
	
	err = NLERR_BADVOICE;
	if (!ExtractVal(voiceStr, &along)) goto broken;
	if (along<1L || along>31L) goto broken;						/* NB: user, not internal, voice */
	pNRGR->uVoice = along;
	
	err = NLERR_BADPART;
	if (!ExtractVal(partStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES) goto broken;
	pNRGR->part = along;

	/* ??Code here and elsewhere checks the staff no. is from 1 to MAXSTAVES: this doesn't
		catch cases where it's greater than the actual no. of staves in the score. */
		
	err = NLERR_BADSTAFF;
	if (!ExtractVal(staffStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES) goto broken;
	pNRGR->staff = along;

	err = NLERR_BADDURCODE;
	if (!ExtractVal(durStr, &along)) goto broken;
	switch (objTypeCode) {
		case NOTE_CHAR:
			if (along<(long)UNKNOWN_L_DUR || along>(long)MAX_L_DUR) goto broken;
			break;
		case REST_CHAR:
			if (along<-127L || along==(long)UNKNOWN_L_DUR || along>(long)MAX_L_DUR) goto broken;
			break;
		case GRACE_CHAR:
			if (along<(long)QTR_L_DUR || along>(long)THIRTY2ND_L_DUR) goto broken;
			break;
	}
	pNRGR->durCode = along;
	
	err = NLERR_BADDOTS;
	if (!ExtractVal(dotsStr, &along)) goto broken;
	if (pNRGR->objType==GRACE_TYPE)
		if (along!=0L) goto broken;
	else {
		if (along<0L || along>8L) goto broken;	// FIXME: need to use dots allowable for durCode
	}
	pNRGR->nDots = along;

	if (pNRGR->isRest) {
		pNRGR->noteNum = 0;
		pNRGR->acc = 0;
		pNRGR->eAcc = 0;
		pNRGR->pDur = 0;
		pNRGR->vel = 0;
	}
	else {
		err = NLERR_BADNOTENUM;
		if (!ExtractVal(noteNumStr, &along)) goto broken;
		if (along<0L || along>127L) goto broken;
		pNRGR->noteNum = along;

		err = NLERR_BADACC;
		if (!ExtractVal(accStr, &along)) goto broken;
		if (along<0L || along>(long)AC_DBLSHARP) goto broken;
		pNRGR->acc = along;

		err = NLERR_BADEACC;
		if (!ExtractVal(eAccStr, &along)) goto broken;
		if (along<(long)AC_DBLFLAT || along>(long)AC_DBLSHARP) goto broken;
		pNRGR->eAcc = along;

		err = NLERR_BADPDUR;
		if (!ExtractVal(pDurStr, &along)) goto broken;
		/* <pDur> can be 0: this means "default" (see NotelistToNight()) */
		if (pNRGR->objType!=GRACE_TYPE && (along<0L || along>32000L)) goto broken;	/* see InfoDialog.c */
		pNRGR->pDur = along;

		err = NLERR_BADVELOCITY;
		if (!ExtractVal(velStr, &along)) goto broken;
		if (along<0L || along>127L) goto broken;
		pNRGR->vel = along;
	}

	err = NLERR_BADNOTEFLAGS;
	if (!ExtractNoteFlags(flagsStr, pNRGR)) goto broken;

	err = NLERR_BADAPPEAR;
	if (!ExtractVal(appearStr, &along)) goto broken;
	if (along<(long)NO_VIS || along>(long)NOTHING_VIS) goto broken;
	pNRGR->appear = along;
	
	if (*modStr) {
		err = NLERR_BADMODS;
		if (pNRGR->objType==GRACE_TYPE) goto broken;			/* Ngale doesn't support these yet */
		if (!ExtractNoteMods(modStr, pNRGR)) goto broken;
	}
	else pNRGR->firstMod = 0;

	gNextEmptyNode++;
	return True;
	
broken:
	ReportParseFailure("ParseNRGR", err);
	return False;
}


/* ----------------------------------------------------------------------- ParseTuplet -- */
/* Parse a line like this:
		P v=1 npt=1 num=3 denom=2 appear=101
*/

static Boolean ParseTuplet()
{
	char		voiceStr[32], partStr[32], numStr[32], denomStr[32], appearStr[32], *p;
	short		ans, err;
	long		along;
	PNL_TUPLET	pTuplet;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }

	ans = sscanf(gInBuf, "%*c%s%s%s%s%s", voiceStr, partStr, numStr, denomStr, appearStr);
	if (ans<5) { err = NLERR_TOOFEWFIELDS; goto broken; }
	
	pTuplet = GetPNL_TUPLET(gNextEmptyNode);

	pTuplet->lStartTime = 0L;
	pTuplet->objType = TUPLET_TYPE;

	err = NLERR_BADVOICE;
	if (!ExtractVal(voiceStr, &along)) goto broken;
	if (along<1L || along>31L) goto broken;					/* NB: user, not internal, voice */
	pTuplet->uVoice = along;
	
	err = NLERR_BADPART;
	if (!ExtractVal(partStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES) goto broken;
	pTuplet->part = along;

	pTuplet->staff = NOONE;									/* Filled in later by AnalyzeNLTuplet */

	err = NLERR_BADNUMDENOM;
	if (!ExtractVal(numStr, &along)) goto broken;
	if (along<(long)2 || along>(long)MAX_TUPLENUM) goto broken;
	pTuplet->num = along;

	err = NLERR_BADNUMDENOM;
	if (!ExtractVal(denomStr, &along)) goto broken;
	if (along<(long)2 || along>(long)MAX_TUPLENUM) goto broken;
	pTuplet->denom = along;

	pTuplet->nInTuple = 0;									/* Filled in later by AnalyzeNLTuplet */
	
	/* The appear= field comprises 3 digits with no intervening spaces. Each digit
		is either 1 or 0, representing an on or off setting for three properties:
		numVis, denomVis and brackVis (in that order). For example,
			"111" means numVis, denomVis, brackVis
			"100" means numVis, !denomVis, !brackVis
			"001" means !numVis, !denomVis, brackVis
		and so on. Note that if denomVis is True, numVis must also be True.
		(Nightingale's user interface for this is the Fancy Tuplet dialog.)
	*/
	err = NLERR_BADAPPEAR;
	p = strchr(appearStr, '=');
	if (!p) goto broken;
	p++;
	if (strlen(p)!=3) goto broken;
	pTuplet->numVis = (p[0]=='1');
	pTuplet->denomVis = (p[1]=='1');
	pTuplet->brackVis = (p[2]=='1');

	err = NLERR_BADTUPLETVIS;
	if (pTuplet->denomVis && !pTuplet->numVis) goto broken;

	gNextEmptyNode++;
	return True;
broken:
	ReportParseFailure("ParseTuplet", err);
	return False;
}


/* ---------------------------------------------------------------------- ParseBarline -- */
/* Parse a line like this:
		/ t=1440 type=1
*/

static Boolean ParseBarline()
{
	char			timeStr[32], typeStr[32];
	short			ans, err;
	long			along;
	PNL_BARLINE	pBarline;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }

	typeStr[0] = '\0';						/* might be missing */
	
	ans = sscanf(gInBuf, "%*c%s%s", timeStr, typeStr);
	if (ans<1) { err = NLERR_TOOFEWFIELDS; goto broken; }
	
	pBarline = GetPNL_BARLINE(gNextEmptyNode);

	err = NLERR_BADTIME;
	if (!ExtractVal(timeStr, &along)) goto broken;
	if (along<0L) goto broken;
	pBarline->lStartTime = along;
	
	pBarline->objType = BARLINE_TYPE;
	pBarline->uVoice = NOONE;
	pBarline->part = NOONE;
	pBarline->staff = NOONE;

	if (*typeStr) {
		err = NLERR_BADTYPE;
		if (!ExtractVal(typeStr, &along)) goto broken;
		if (along<(long)BAR_SINGLE || along>(long)BAR_LAST) goto broken;
		pBarline->appear = along;
	}
	else
		pBarline->appear = BAR_SINGLE;

	gNextEmptyNode++;
	return True;
broken:
	ReportParseFailure("ParseBarline", err);
	return False;
}


/* ------------------------------------------------------------------------- ParseClef -- */
/* Parse a line like this:
		C stf=1 type=1
*/

static Boolean ParseClef()
{
	char		staffStr[32], typeStr[32];
	short		ans, err;
	long		along;
	PNL_CLEF	pClef;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }

	ans = sscanf(gInBuf, "%*c%s%s", staffStr, typeStr);
	if (ans<2) { err = NLERR_TOOFEWFIELDS; goto broken; }
	
	pClef = GetPNL_CLEF(gNextEmptyNode);

	pClef->lStartTime = 0L;
	pClef->objType = CLEF_TYPE;
	pClef->uVoice = NOONE;
	pClef->part = NOONE;

	err = NLERR_BADSTAFF;
	if (!ExtractVal(staffStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES) goto broken;
	pClef->staff = along;
	
	err = NLERR_BADTYPE;
	if (!ExtractVal(typeStr, &along)) goto broken;
	if (along<(long)LOW_CLEF || along>(long)HIGH_CLEF) goto broken;
	pClef->type = along;

	gNextEmptyNode++;
	return True;
broken:
	ReportParseFailure("ParseClef", err);
	return False;
}


/* ----------------------------------------------------------------------- ParseKeySig -- */
/* Parse a line like this:
		K stf=1 KS=3 b
*/

static Boolean ParseKeySig()
{
	char		staffStr[32], numAccStr[32], acc;
	short		ans, err;
	long		along;
	PNL_KEYSIG	pKS;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }

	ans = sscanf(gInBuf, "%*c%s%s %c", staffStr, numAccStr, &acc);
	if (ans<3) { err = NLERR_TOOFEWFIELDS; goto broken; }
	
	pKS = GetPNL_KEYSIG(gNextEmptyNode);

	pKS->lStartTime = 0L;
	pKS->objType = KEYSIG_TYPE;
	pKS->uVoice = NOONE;
	pKS->part = NOONE;

	err = NLERR_BADSTAFF;
	if (!ExtractVal(staffStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES) goto broken;
	pKS->staff = along;
	
	err = NLERR_BADNUMACC;
	if (!ExtractVal(numAccStr, &along)) goto broken;
	if (along<0L || along>(long)MAX_KSITEMS) goto broken;
	pKS->numAcc = along;

	switch (acc) {
		case '#':	pKS->sharp = True;	break;
		case 'b':	pKS->sharp = False;	break;
		default:		err = NLERR_BADACC;	goto broken;
	}
	
	gNextEmptyNode++;
	return True;
broken:
	ReportParseFailure("ParseKeySig", err);
	return False;
}


/* ---------------------------------------------------------------------- ParseTimeSig -- */
/* Parse a line like this:
		T stf=2 num=3 denom=8 displ=1
	NB: The "displ" field is optional if timesig type is normal.
*/

static Boolean ParseTimeSig()
{
	char		staffStr[32], numStr[32], denomStr[32], appearStr[32];
	short		ans, err;
	long		along;
	PNL_TIMESIG	pTS;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }

	appearStr[0] = 0;										/* might be omitted */
	
	ans = sscanf(gInBuf, "%*c%s%s%s%s", staffStr, numStr, denomStr, appearStr);
	if (ans<3) { err = NLERR_TOOFEWFIELDS; goto broken; }
	
	pTS = GetPNL_TIMESIG(gNextEmptyNode);

	pTS->lStartTime = 0L;
	pTS->objType = TIMESIG_TYPE;
	pTS->uVoice = NOONE;
	pTS->part = NOONE;

	err = NLERR_BADSTAFF;
	if (!ExtractVal(staffStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES) goto broken;
	pTS->staff = along;
	
	err = NLERR_BADNUMDENOM;
	if (!ExtractVal(numStr, &along)) goto broken;
	if (TSNUMER_BAD((short)along)) goto broken;
	pTS->num = along;

	err = NLERR_BADNUMDENOM;
	if (!ExtractVal(denomStr, &along)) goto broken;
	if (TSDENOM_BAD((short)along)) goto broken;
	pTS->denom = along;

	if (*appearStr) {
		err = NLERR_BADDISPL;
		if (!ExtractVal(appearStr, &along)) goto broken;
		if (along<(long)N_OVER_D || along>(long)N_ONLY) goto broken;
		pTS->appear = along;
	}
	else
		pTS->appear = N_OVER_D;

	gNextEmptyNode++;
	return True;
broken:
	ReportParseFailure("ParseTimeSig", err);
	return False;
}


/* -------------------------------------------------------------------- ParseTempoMark -- */
/* Parse a line like this:
		M stf=1 'Allegro' q.=126-132
*/

static Boolean ParseTempoMark()
{
	char			staffStr[32];
	unsigned char	*p, *q, str[MAX_TEMPO_CHARS];
	short			ans, err;
	long			along;
	NLINK			offset;
	PNL_TEMPO		pTempo;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }

	ans = sscanf(gInBuf, "%*c%s", staffStr);
	if (ans<1) { err = NLERR_TOOFEWFIELDS; goto broken; }
	
	pTempo = GetPNL_TEMPO(gNextEmptyNode);

	pTempo->lStartTime = 0L;
	pTempo->objType = TEMPO_TYPE;
	pTempo->uVoice = NOONE;
	pTempo->part = NOONE;

	err = NLERR_BADSTAFF;
	if (!ExtractVal(staffStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES) goto broken;
	pTempo->staff = along;

	/* Extract tempo string, which is enclosed in single-quotes and can contain whitespace.
		First copy it into a temporary buffer (str); then store into gHStringPool. NB:
		If there is no string, it will be encoded as two single-quotes (''). */
		
	err = NLERR_MISCELLANEOUS;
	p = (unsigned char *)strchr(gInBuf, '\'');
	if (!p) goto broken;
	p++;
	if (*p!='\'') {
		q = str;
		while (*p!='\''&& q-str<MAX_TEMPO_CHARS-1L)
			*q++ = *p++;
		*q = 0;

		offset = StoreString((char *)str);
		if (offset)
			pTempo->string = offset;
		else goto broken;
	}
	else {
		str[0] = 0;
		pTempo->string = NILINK;
	}
	
	/* Point to first non-whitespace char following terminal single-quote. If the
	   tempo mark has its metronome value hidden (hideMM), then we'll reach the null
	   that terminates gInBuf before hitting a non-whitespace char. */
	   
	for (p++; *p; p++)
		if (!isspace((int)*p)) break;
	
	/* Analyze the metronome (beats per minute) string. Extract <durCode> and <dotted>;
	   store <metroStr> into gHStringPool. The <metroStr> gives the tempo Ngale uses
	   for playback. It can comprises arbitrary text, including whitespace, but must
	   contain a valid tempo value. We don't worry about that here. If there _is_ no
	   metronome string, we assign a default, since Ngale requires one.	*/
	   
	if (*p) {
		short	i;
		pTempo->durCode = 0;
		for (i = 1; i<=9; i++)
			if (*p==gTempoCode[i]) pTempo->durCode = i;
		if (pTempo->durCode==0) goto broken;
		if (*++p=='.') {
			pTempo->dotted = True;
			p++;
		}
		else
			pTempo->dotted = False;
		if (*p=='=') p++;
		else goto broken;
		
		if (strlen((char *)p)>MAX_TEMPO_CHARS) goto broken;
		offset = StoreString((char *)p);
		if (offset)
			pTempo->metroStr = offset;
		else goto broken;

		pTempo->hideMM = False;
	}
	else {											/* Assign a default tempo and hide it. */
		Str255	metroStr;

		NumToString(config.defaultTempoMM, metroStr);
		PtoCstr(metroStr);
		offset = StoreString((char *)metroStr);
		if (offset)
			pTempo->metroStr = offset;
		else goto broken;
		
		pTempo->durCode = QTR_L_DUR;
		pTempo->dotted = False;
		pTempo->hideMM = True;
	}
	
	gNextEmptyNode++;
	return True;
broken:
	ReportParseFailure("ParseTempoMark", err);
	return False;
}


/* ------------------------------------------------------------------ ParseTextGraphic -- */
/* Parse a line like this:
		A v=2 npt=2 stf=4 S [3] 'con calore'
The item "3" is shown in brackets because it's not always present: it's required with
v. 2 files, but not allowed with earlier versions. NB: The string can include spaces,
so we can't use sscanf to extract it. */

static Boolean ParseTextGraphic()
{
	char		voiceStr[32], partStr[32], staffStr[64], type, styleCode, *p, *q;
	char		str[MAX_CHARS];
	short		ans, err;
	long		along;
	NLINK		offset;
	PNL_GRAPHIC	pGraphic;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }

	styleCode = '0';
	if (gNotelistVersion>=2) {
		ans = sscanf(gInBuf, "%*c%s%s%s %c%c", voiceStr, partStr, staffStr, &type, &styleCode);
		if (ans<5) { err = NLERR_TOOFEWFIELDS; goto broken; }
	}
	else {
		ans = sscanf(gInBuf, "%*c%s%s%s %c", voiceStr, partStr, staffStr, &type);
		if (ans<4) { err = NLERR_TOOFEWFIELDS; goto broken; }
	}
	
	pGraphic = GetPNL_GRAPHIC(gNextEmptyNode);

	pGraphic->lStartTime = 0L;
	pGraphic->objType = GRAPHIC_TYPE;

	err = NLERR_BADVOICE;
	if (!ExtractVal(voiceStr, &along)) goto broken;
	if (along<1L || along>31L)									/* NB: user, not internal, voice */
		if (along!=(long)NOONE)
			goto broken;
	pGraphic->uVoice = along;
	
	err = NLERR_BADPART;
	if (!ExtractVal(partStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES)
		if (along!=(long)NOONE)
			goto broken;
	pGraphic->part = along;

	err = NLERR_BADSTAFF;
	if (!ExtractVal(staffStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES)
		if (along!=(long)NOONE)
			goto broken;
	pGraphic->staff = along;

	switch (type) {
		case 'S':	pGraphic->type = GRString;	break;
		case 'L':	pGraphic->type = GRLyric;	break;
		default:	err = NLERR_MISCELLANEOUS;	goto broken;
	}

	switch (styleCode) {
		case '1': pGraphic->style = TSRegular1STYLE;  break;
		case '2': pGraphic->style = TSRegular2STYLE;  break;
		case '3': pGraphic->style = TSRegular3STYLE;  break;
		case '4': pGraphic->style = TSRegular4STYLE;  break;
		case '5': pGraphic->style = TSRegular5STYLE;  break;
		default: pGraphic->style = TSNoSTYLE;
	}

	/* Extract string, which is enclosed in single-quotes and can contain whitespace.
		First copy it into a temporary buffer (str); then store into gHStringPool. */
		
	p = strchr(gInBuf, '\'');
	if (!p) goto broken;
	p++;
	q = str;
	while (*p!='\'' && q-str<MAX_CHARS-1L)
		*q++ = *p++;
	*q = 0;
	
	offset = StoreString(str);
	if (offset)
		pGraphic->string = offset;
	else goto broken;
	
	gNextEmptyNode++;
	return True;
broken:
	ReportParseFailure("ParseTextGraphic", err);
	return False;
}


/* ---------------------------------------------------------------------- ParseDynamic -- */
/* Parse a line like this:
		D stf=1 dType=9
*/

static Boolean ParseDynamic()
{
	char		staffStr[32], typeStr[32];
	short		ans, err;
	long		along;
	PNL_DYNAMIC	pDynamic;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }

	ans = sscanf(gInBuf, "%*c%s%s", staffStr, typeStr);
	if (ans<2) { err = NLERR_TOOFEWFIELDS; goto broken; }
	
	pDynamic = GetPNL_DYNAMIC(gNextEmptyNode);

	pDynamic->lStartTime = 0L;
	pDynamic->objType = DYNAMIC_TYPE;
	pDynamic->uVoice = NOONE;
	pDynamic->part = NOONE;

	err = NLERR_BADSTAFF;
	if (!ExtractVal(staffStr, &along)) goto broken;
	if (along<1L || along>(long)MAXSTAVES) goto broken;
	pDynamic->staff = along;
	
	err = NLERR_BADDTYPE;
	if (!ExtractVal(typeStr, &along)) goto broken;
	if (along<(long)PPPP_DYNAM || along>(long)LAST_DYNAM) goto broken;
	if (along>(long)SFP_DYNAM) {
		gHairpinCount++;
		gNumNLItems--;					/* so we later don't try to read empty node at end of list */
		return True;
	}
	pDynamic->type = along;

	gNextEmptyNode++;
	return True;
broken:
	ReportParseFailure("ParseDynamic", err);
	return False;
}


/* ----------------------------------------------------- ParseStructComment and allies -- */

/* Given a pointer to the beginning of one of the optional fields on the header
structured comment, parse it and return a pointer to the char after its end. If we
don't recognize the field, or there's no non-whitespace in the string, just return
NULL. */

static char *ParseField(char *p, Boolean *pOkay);
static char *ParseField(char *p, Boolean *pOkay)
{
	char str[256];
	short ans, number;

	*pOkay = True;
	
	/* Point to first non-whitespace char. If there's nothing following, do nothing. */
	for (p++; *p; p++)
		if (!isspace((int)*p)) break;
	ans = sscanf(p, "%s", str);
	if (ans<1) return NULL;
	
	if (strcmp(str, "delaccs")==0) {
		gDelAccs = True;
		p += strlen(str);
		return p;
	}
	else if (strncmp(str, "startmeas=", strlen("startmeas="))==0) {
		p = strchr(p, '=');										/* Skip to the next '=' in input */
		if (!p) return (char *)0;
		p++;
		ans = sscanf(p, "%s", str);
		p += strlen(str);
		number = (char)strtol(str, (char **)NULL, 10);
		gFirstMNNumber = 	number;
		return p;
	}
	else {
		*pOkay = False;
		return NULL;		/* Unrecognized field */
	}
}


/* Parse a structured comment line like one of these (appearing as a header on every
notelist file):
		%%Notelist-V2 file='BaaBaaBlackSheep'  partstaves=1 1 1 0
		%%Notelist-V2 file='SonataOp111'  partstaves=2 0 startmeas=0
In the future there may be other kinds of structured comment. */

#define COMMENT_SCORE		"%%Score"
#define COMMENT_NOTELIST	"%%Notelist"

static Boolean ParseStructComment()
{
	char		str[64], *p, *q, nstaves;
	short		ans, i, err;
	NLINK		offset;
	PNL_HEAD	pHead;
	Boolean		okay;

	if (gNextEmptyNode>gNumNLItems) { err = NLERR_INTERNAL; goto broken; }

	if (gNotelistVersion<=1)
		okay = (strncmp(gInBuf, COMMENT_SCORE, strlen(COMMENT_SCORE))==0);
	else
		okay = (strncmp(gInBuf, COMMENT_NOTELIST, strlen(COMMENT_NOTELIST))==0);

	pHead = GetPNL_HEAD(gNextEmptyNode);

	pHead->lStartTime = 0L;
	pHead->objType = HEAD_TYPE;
	pHead->uVoice = NOONE;
	pHead->part = NOONE;
	pHead->staff = NOONE;

	/* Set defaults for values set by optional fields at the end of the line. */
	gFirstMNNumber = 	1;
	gDelAccs = False;

	/* Extract score name, which is enclosed in single-quotes and can contain whitespace.
	   First copy it into a temporary buffer (str); then store into gHStringPool. It must
	   not be longer than 31 chars (excluding terminating null). */

	err = NLERR_MISCELLANEOUS;
	p = strchr(gInBuf, '\'');
	if (!p) goto broken;
	p++;
	q = str;
	while (*p!='\''&& q-str<31L)
		*q++ = *p++;
	*q = 0;

	offset = StoreString(str);
	if (offset)
		pHead->scoreName = offset;
	else goto broken;

	/* Construct a 1-based array giving the arrangement of parts and staves. Indices
	   represent part numbers; values represent the number of staves in a part. */
	   
	gNumNLStaves = 0;
	for (i=0; i<=MAXSTAVES; i++)
		gPartStaves[i] = 0;
	p = strchr(p, '=');											/* Skip to the next '=' in input */
	if (!p) goto broken;
	for (p++, i=1; i<=MAXSTAVES; i++, p++) {
		ans = sscanf(p, "%s", str);
		if (ans<1) goto broken;
		nstaves = (char)strtol(str, (char **)NULL, 10);
		if (nstaves<=0) {
			/* Found the terminator for the part/staff info: skip over it. */
			
			p = strchr(p, ' ');
			if (!p) goto done;
			break;
		}
		gPartStaves[i] = nstaves;
		gNumNLStaves += nstaves;
		if (gNumNLStaves>=MAXSTAVES) {
			sprintf(str, "Nightingale can't handle scores with more than %d staves.",
						MAXSTAVES);									// ??I18N BUG!
			CParamText(str, "", "", ""); 
			StopInform(GENERIC_ALRT);
			goto broken;
		}
		p = strchr(p, ' ');
		if (!p) goto done;
	}

	/* Parse and make use of any optional fields that appear at the end of the line. */
	
	do
		p = ParseField(p, &okay);
	while (p);
	if (!okay) {
		sprintf(str, "The Notelist's header line contained unrecognized field(s).");	// ??I18N BUG!
		CParamText(str, "", "", ""); 
		CautionInform(GENERIC_ALRT);
	}

done:
	gNextEmptyNode++;
	return True;
broken:
	ReportParseFailure("ParseStructComment", err);
	return False;
}


/* ------------------------------------------------------------------------ ExtractVal -- */
/* Given a string having the format "label=xxx", converts the "xxx" part to a numerical
(long int) value. Returns True if ok, False if error. */

static Boolean ExtractVal(char *str,			/* source string */
							long *val)			/* pass back extracted value */
{
	char	*p;
	short	ans;
	
	p = strchr(str, '=');				/* p will point to '=' */
	if (p) {
		p++;							/* advance pointer to character following '=' */
		ans = sscanf(p, "%ld", val);	/* read numerical value in string as a long */
										//???Should I use atol instead? Remember NCust OMSdeviceID problem!
		if (ans > 0)
			return True;
	}
	
	/* no '=' found in str, or sscanf error */
	*val = 0L;
	return False;
}


/* ------------------------------------------------------------------ ExtractNoteFlags -- */
/* Function for decoding the flag field of a note, rest or grace note. Assigns values
to the object passed in pNRGR. The function has a complete check for illegal characters
in flagStr. Returns True if ok, False if error.

Here's how the flags work for notes:
								flag numbers:
								"......"
								 0|||||
								  1||||
								   2|||
									3||
									 4|
									  5

				flagNum	flagName			values		meaning
					
				0			mCode			'.'			is not in a chord
												'-'		is in a chord and is not main note
												'+'		is in a chord and is main note
				1			tiedL				'.'		is not tiedL
												')'		is tiedL
				2			tiedR				'.'		is not tiedR
												'('		is tiedR
				3			slurredL			'.'		is not slurredL
												'>'		is slurredL
				4			slurredR			'.'		is not slurredR
												'<'		is slurredR
				5			inTuplet			'.'		is not in tuplet
												'T'		is in tuplet

The mCode for rests is ignored. (It's always '.'.)

Grace notes have only one flag (for mCode) at the moment. (This is likely to change.) */

static Boolean ExtractNoteFlags(char *flagStr,
									PNL_NRGR pNRGR)
{
	char	*p;
	
	p = flagStr;
	
	switch (*p) {
		case '.':									/* not in chord; not main note */
			pNRGR->inChord = False;
			pNRGR->mainNote = False;
			break;
		case '-':									/* in chord; not main note */
			pNRGR->inChord = True;
			pNRGR->mainNote = False;
			break;
		case '+':									/* in chord; main note */
			pNRGR->inChord = True;
			pNRGR->mainNote = True;
			break;
		default:									/* illegal char */
			goto illegal;
	}

	if (pNRGR->objType==GRACE_TYPE) return True;
	
	switch (*++p) {
		case '.':		pNRGR->tiedL = False;	break;
		case ')':		pNRGR->tiedL = True;	break;
		default:			goto illegal;
	}

	switch (*++p) {
		case '.':		pNRGR->tiedR = False;	break;
		case '(':		pNRGR->tiedR = True;	break;
		default:			goto illegal;
	}

	switch (*++p) {
		case '.':		pNRGR->slurredL = False;	break;
		case '>':		pNRGR->slurredL = True;		break;
		default:			goto illegal;
	}

	switch (*++p) {
		case '.':		pNRGR->slurredR = False;	break;
		case '<':		pNRGR->slurredR = True;		break;
		default:			goto illegal;
	}

	switch (*++p) {
		case '.':		pNRGR->inTuplet = False;	break;
		case 'T':		pNRGR->inTuplet = True;		break;
		default:		goto illegal;
	}

	return True;

illegal:
	AlwaysErrMsg("Illegal character %c in note flag string.\n", *p);
	return False;
}


/* ------------------------------------------------------------------- ExtractNoteMods -- */
/* Parses modifier strings like these:
	mods=2				[one modifier]
	mods=3,10,12		[multiple modifiers]
	mods=2:127,12		[two modifiers, one with non-zero data field]

Stores each modifier parsed into gHModList, filling in the <next> field of these
modifiers to form a chain. Stores the index of the first of these in the <firstMod>
field of the given note (NRGR). If error, return False without giving a message (but
see comments below), else return True. */

#define MAX_MODNRS 50		/* Max. modifiers per note/rest we can handle */

static Boolean ExtractNoteMods(char	*modStr, PNL_NRGR pNRGR)
{
	char	*p, *q;
	short	modCount=0, ashort;
	NLINK	offset = NILINK;
	NL_MOD	tmpMod;
	
	if (strncmp(modStr, "mods=", (size_t)5))		/* it's not a mod string */
		return False;
	
	pNRGR->firstMod = NILINK;
	
	p = strchr(modStr, '=');						/* p will point to '=' */
	if (!p) goto broken;
	p++;											/* advance pointer to character following '=' */
	
	p = strtok(p, ",");
	do {
		q = strchr(p, ':');
		if (q) {									/* there's a data field */
			*q++ = 0;
			errno = 0;
			ashort = atoi(p);
			if (errno==ERANGE) goto broken;
			tmpMod.code = ashort;
			
			errno = 0;
			ashort = atoi(q);
			if (errno==ERANGE) goto broken;
			tmpMod.data = ashort;
		}
		else {
			errno = 0;
			ashort = atoi(p);
			if (errno==ERANGE) goto broken;
			tmpMod.code = ashort;
			tmpMod.data = 0;
		}
		tmpMod.next = NILINK;
		
		offset = StoreModifier(&tmpMod);
		if (!offset) return False;					/* Error message already given */
		
		/* If this is the first modifier, store its index (into gHModList) into
			the owning note. Otherwise, store its index into the <next> field of the
			previous modifier.
		*/
		if (pNRGR->firstMod==NILINK)
			pNRGR->firstMod = offset;
		else
			(*gHModList)[offset-1].next = offset;
		
		p = strtok(NULL, ",");
		modCount++;
		if (modCount==MAX_MODNRS) break;			/* not likely */
	} while (p);

	return True;

broken:
	/* It would be nice someday to give more information about the problem here, e.g.,
		pNRGR->lStartTime, pNRGR->part, pNRGR->uVoice, pNRGR->staff. */
		
	return False;
}



/* -------------------------------------------------------------------------------------- */
/* Notelist search and utility functions */

/* -------------------------------------------------------------------------- NLSearch -- */
/* Modelled on LSSearch in Search.c, but used for searching the temporary Notelist data
structure. NB: startL is included in the search. So, for example, if <startL> is the same
object type as <type>, then NLSearch will deliver <startL> without looking any further.
If search fails to find a suitable target, returns NILINK. */

NLINK NLSearch(NLINK		startL,		/* Place to start looking */
					short	type,		/* target type (or ANYTYPE) */
					char	part,		/* target part number (or ANYONE) */
					char	staff,		/* target staff number (or ANYONE) */
					char	uVoice,		/* target user voice number (or ANYONE) */
					Boolean	goLeft)		/* True if we should search left */
{
	PNL_GENERIC	pG;
	NLINK pL;
	short increment;
	
	increment = (goLeft? -1 : 1);
	
	for (pL=startL; pL>0 && pL<=gNumNLItems; pL+=increment) {
		pG = GetPNL_GENERIC(pL);
		if (type==pG->objType || type==ANYONE)
			if (staff==pG->staff || staff==ANYONE)
				if (part==pG->part || part==ANYONE)
					if (uVoice==pG->uVoice || uVoice==ANYONE)
						return pL;
	}
	return NILINK;
}


/* ------------------------------------------------------------------ CheckNLNoteRests -- */

static Boolean CheckNLNoteRests(void)
{
	NLINK		thisL, nr1L, nr2L, barL;
	PNL_NRGR	pNR1, pNR2;
	PNL_BARLINE	pBar;
	long		lDur, endTime;
	Boolean		inSameChord;
	char		fmtStr[256], str[256];
	
	for (thisL=0; thisL<=gNumNLItems; thisL = nr1L) {
		nr1L = NLSearch(thisL+1, NR_TYPE, ANYONE, ANYONE, ANYONE, GO_RIGHT);
		if (!nr1L) break;
		pNR1 = GetPNL_NRGR(nr1L);
		lDur = CalcNLNoteLDur(nr1L);
		if (lDur==0L) {
			/* ??This and following messages refer to "notes" but apparently apply to rests too. */
			
			GetIndCString(fmtStr, NOTELIST_STRS, 32);	/* "Note at time %ld has bad lDur." */
			sprintf(str, fmtStr, pNR1->lStartTime);
			CParamText(str, "", "", ""); 
			CautionInform(GENERIC_ALRT);
			return False;
		}
		if (lDur==-1L) {
			GetIndCString(fmtStr, NOTELIST_STRS, 33);	/* "Note at time %ld claims to be in a tuplet..." */
			sprintf(str, fmtStr, pNR1->lStartTime);
			CParamText(str, "", "", ""); 
			CautionInform(GENERIC_ALRT);
			return False;
		}
		endTime = pNR1->lStartTime + lDur;

		barL = NLSearch(nr1L+1, BARLINE_TYPE, ANYONE, ANYONE, ANYONE, GO_RIGHT);
		if (barL) {
			pBar = GetPNL_BARLINE(barL);
			if (pBar->lStartTime<endTime) {
				GetIndCString(fmtStr, NOTELIST_STRS, 34);	/* "Note at time %ld extends over barline..." */
				sprintf(str, fmtStr, pNR1->lStartTime, pBar->lStartTime);
				CParamText(str, "", "", ""); 
				CautionInform(GENERIC_ALRT);
			}
		}
		
		nr2L = NLSearch(nr1L+1, NR_TYPE, pNR1->part, ANYONE, pNR1->uVoice, GO_RIGHT);
		if (!nr2L) break;
		pNR2 = GetPNL_NRGR(nr2L);
		inSameChord = (pNR1->lStartTime==pNR2->lStartTime);
		if (!inSameChord && pNR2->lStartTime<endTime) {
			GetIndCString(fmtStr, NOTELIST_STRS, 35);		/* "Consecutive notes in same voice overlap..." */
			sprintf(str, fmtStr, pNR1->lStartTime, pNR2->lStartTime);
			CParamText(str, "", "", ""); 
			CautionInform(GENERIC_ALRT);
			return False;
		}
	}
	return True;
}


/* -------------------------------------------------------------------- CheckNLTuplets -- */

static Boolean CheckNLTuplets(void)
{
	NLINK	thisL, tupL;
	Boolean	result, noErrors = True;
	
	for (thisL=0; thisL<=gNumNLItems; thisL = tupL) {
		tupL = NLSearch(thisL+1, TUPLET_TYPE, ANYONE, ANYONE, ANYONE, GO_RIGHT);
		if (!tupL) break;
		result = AnalyzeNLTuplet(tupL);
		if (!result) noErrors = False;
	}
	return noErrors;
}


/* ------------------------------------------------------------------- CheckNLBarlines -- */
/* Check the barlines in the Notelist data structure for timing consistency. If any
barline's lStartTime comes before that of the previous barline or after that of the
following barline, give an error message and return False; else return True. */

static Boolean CheckNLBarlines(void)
{
	NLINK		thisL, bar1L, bar2L;
	PNL_BARLINE	pBar1, pBar2;
	char fmtStr[256], str[256];
	
	for (thisL=0; thisL<=gNumNLItems; thisL = bar1L) {
		bar1L = NLSearch(thisL+1, BARLINE_TYPE, ANYONE, ANYONE, ANYONE, GO_RIGHT);
		if (!bar1L) break;
		pBar1 = GetPNL_BARLINE(bar1L);
		bar2L = NLSearch(bar1L+1, BARLINE_TYPE, ANYONE, ANYONE, ANYONE, GO_RIGHT);
		if (!bar2L) break;
		pBar2 = GetPNL_BARLINE(bar2L);
		if (pBar1->lStartTime>pBar2->lStartTime) {
			GetIndCString(fmtStr, NOTELIST_STRS, 31);		/* "Consecutive barline times..." */
			sprintf(str, fmtStr, pBar1->lStartTime, pBar2->lStartTime);
			CParamText(str, "", "", ""); 
			CautionInform(GENERIC_ALRT);
			return False;
		}
	}
	return True;
}


/* ------------------------------------------------------------------- CheckNLTimeSigs -- */
/* Examine the time signatures in the Notelist data structure. We're trying to avoid a
problem that would crop up if we simply converted all time sig. records into Nightingale
objects. Consider a score in which there is a time signature change on all staves at the
same time. When Ngale writes a Notelist	of such a score, it writes a separate time
sig. record for each staff. This would cause a simple-minded converter to create a
separate object for each time sig., rather than one object with a subobject for each
staff. The former arrangement would cause spacing problems in Nightingale -- time
signatures that ought to be aligned vertically would have different horizontal offsets.
To avoid this, we have to massage the Notelist data a bit. Specifically, whenever there
are <gNumNLStaves> consecutive time sig. records in the Notelist, we set the <staff> of
the first one to ANYONE and the <staff> of the others to NOONE. Doing so lets the
converter create a single time sig. object with a subobject for each staff.
(ConvertTimesig skips Notelist records whose <staff> is NOONE.) NB: The numerator and
denominator used for all staves will be those stored into the first time sig. Return
True if all is well, False if error. FIXME: ALWAYS RETURNS True! */

static Boolean CheckNLTimeSigs(void)
{
	short		count;
	NLINK		pL, qL, saveL;
	PNL_TIMESIG	pTS;
	Boolean		endOfRun;
	
	if (gNumNLStaves==1) return True;				/* There's no problem to solve. */

	count = 0;
	saveL = 1;										/* Should never be neccessary, but just in case */
	for (pL = 1; pL<=gNumNLItems; pL++) {
		if (GetNL_TYPE(pL)==TIMESIG_TYPE) {
			if (count==0) {
				saveL = pL;
				endOfRun = False;
			}
			count++;
		}
		else endOfRun = True;
		
		if (count==gNumNLStaves) {
			pTS = GetPNL_TIMESIG(saveL);
			pTS->staff = ANYONE;
			for (qL = saveL+1; qL<saveL+gNumNLStaves; qL++) {
				pTS = GetPNL_TIMESIG(qL);
				pTS->staff = NOONE;
			}
			count = 0;
		}
		if (endOfRun) count = 0;
	}
	return True;
}


/* ------------------------------------------------------------------- AnalyzeNLTuplet -- */
/* Analyze the given Notelist tuplet record, fill in its <nInTuple> field, and check its
<num> and <denom> fields for consistency with the notes that follow it. Also, fill in
its staff field to match the staff of its first note. If error, give a message and
return False, else return True. NB: we assume that note duration fields have already
been checked, so if NLSimpleLDur has trouble, it's an internal error. */

static Boolean AnalyzeNLTuplet(NLINK tupletL)
{
	short		part, uVoice, type, nInTuple;
	long		totLDur, lDur, curTime;
	NLINK		thisL;
	PNL_TUPLET	pTuplet, pT;
	PNL_NRGR	pNR;
	char		fmtStr[256], str[256];

	pTuplet = GetPNL_TUPLET(tupletL);
	part = pTuplet->part;
	uVoice = pTuplet->uVoice;

	/*	Search for notes that could be members of our tuplet. They must be consecutive,
		must be in the same voice as this tuplet, and must have their <inTuplet> flag set.
		We'll stop searching for notes when we reach another tuplet in the same voice, a
		note in this voice that is not inTuplet, or the next barline. (This is because
		Nightingale doesn't support nested tuplets or tuplets spanning barlines.) The
		durations of the notes we find must be consistent with the numerator and
		denominator of our tuplet. (Note that the algorithm depends on the links in the
		notelist data structure being in ascending order (from left to right in the
		score).) Fill in the tuplet's staff from its first note: this should be safe
		because the tuplet can't be cross-staff ??or can it?
		NB: Assumes that notes that are consecutive in the data structure do not
		overlap (because CheckNLNoteRests has not been called yet).
	*/
	nInTuple = 0;
	totLDur = 0L;
	curTime = -1L;
	for (thisL = tupletL+1; thisL<=gNumNLItems; thisL++) {
		type = GetNL_TYPE(thisL);
		if (type==NR_TYPE) {
			pNR = GetPNL_NRGR(thisL);
			if (pNR->part==part && pNR->uVoice==uVoice) {
				if (pNR->inTuplet) {
					if (pNR->lStartTime>curTime)
						curTime = pNR->lStartTime;
					else continue;							/* in same chord as prev note, so don't count it */					
					lDur = NLSimpleLDur(thisL);
					if (lDur==0L) goto broken;
					totLDur += lDur;
					if (!nInTuple)
						pTuplet->staff = pNR->staff;
					nInTuple++;
				}
				else break;
			}
		}
		else if (type==TUPLET_TYPE) {
			pT = GetPNL_TUPLET(thisL);
			if (pT->part==part && pT->uVoice==uVoice)
				break;
		}
		else if (type==BARLINE_TYPE)
			break;
	}

	/*	Now we have the number of notes in the tuplet (nInTuple) and the total
		logical duration represented by their note values (totLDur). (NB: This
		is of course longer than the actual duration of the tuplet.) Check that
		these two pieces of information square with the numerator and denominator
		of our tuplet.
	*/
	if (nInTuple<2) {
		GetIndCString(fmtStr, NOTELIST_STRS, 36);		/* "Tuplet...has only one note." */
		sprintf(str, fmtStr, curTime, part, uVoice);
		CParamText(str, "", "", ""); 
		CautionInform(GENERIC_ALRT);
		return False;
	}
	else {
		pTuplet->nInTuple = nInTuple;
		if (GetDurUnit((short)totLDur, pTuplet->num, pTuplet->denom)==0) {
			GetIndCString(fmtStr, NOTELIST_STRS, 37);	/* "Tuplet...duration doesn't make sense." */
			sprintf(str, fmtStr, curTime, part, uVoice);
			CParamText(str, "", "", ""); 
			CautionInform(GENERIC_ALRT);
			return False;
		}
	}
	
	return True;
	
broken:
	MayErrMsg("AnalyzeNLTuplet: error computing tuplet (tupletL=%d)", tupletL);
	return False;
}


/* ---------------------------------------------------------------------- NLSimpleLDur -- */
/*	Given a note or rest in the Notelist data structure, return its logical duration.
Return 0L if there's a problem. (Based on SimpleLDur in SpaceTime.c.) */

static long NLSimpleLDur(NLINK noteL)
{
	PNL_NRGR	pNR;
	
	pNR = GetPNL_NRGR(noteL);
	if (pNR->durCode<=UNKNOWN_L_DUR) return 0L;
	return Code2LDur(pNR->durCode, pNR->nDots);
}


/* -------------------------------------------------------------------- CalcNLNoteLDur -- */
/*	Compute the logical duration of a Notelist note/rest, taking into account tuplet
membership. For whole-measure rests, return one measure's duration. For multibar rests,
return the total duration of the meaures spanned by the rest. Return -1 if there's a
problem with tuplet membership. (Based on -- but not quite analogous to -- CalcNoteLDur
in SpaceTime.c.) */

static long CalcNLNoteLDur(NLINK noteL)
{
	NLINK		tupL, tsL;
	PNL_NRGR	pNR;
	PNL_TUPLET	pT;
	PNL_TIMESIG	pTS;
	short		tsNum, tsDenom;
	long		tempDur, noteDur;
	
	pNR = GetPNL_NRGR(noteL);

	if (pNR->isRest && pNR->durCode<=WHOLEMR_L_DUR) {
		tsL = NLSearch(noteL-1, TIMESIG_TYPE, ANYONE, pNR->staff, ANYONE, GO_LEFT);
		if (tsL) {
			pTS = GetPNL_TIMESIG(tsL);
			tsNum = pTS->num;
			tsDenom = pTS->denom;
		}
		else {
			tsNum = config.defaultTSNum;
			tsDenom = config.defaultTSDenom;
		}
		noteDur = TimeSigDur(N_OVER_D/*ignored*/, tsNum, tsDenom);
		noteDur *= ABS(pNR->durCode);				/* durCode gives negative of number of measures of rest */
	}
	else
		noteDur = NLSimpleLDur(noteL);
	
	if (pNR->inTuplet) {
		tupL = NLSearch(noteL-1, TUPLET_TYPE, pNR->part, ANYONE, pNR->uVoice, GO_LEFT);
		if (!tupL) return -1L;
		pT = GetPNL_TUPLET(tupL);
		tempDur = noteDur*pT->denom;
		noteDur = tempDur/pT->num;
	}
	return noteDur;
}


/* ---------------------------------------------------------------- ReportParseFailure -- */

#define STRNUM_OFFSET 4

static void ReportParseFailure(
				char */*functionName*/,				/* C string, for debugging only */
				short errCode)						/* NLERR_XXX code */
{
	char fmtStr[256], str1[256], str2[256];
	
	GetIndCString(fmtStr, NOTELIST_STRS, 4);				/* "Problem in line number..." */
	sprintf(str1, fmtStr, gLineCount, gInBuf);
	GetIndCString(str2, NOTELIST_STRS, errCode+STRNUM_OFFSET);
	CParamText(str1, str2, "", ""); 
	StopInform(OPENNOTELIST_ALRT);
}


/* -------------------------------------------------------------------------------------- */
/* Functions that call Macintosh Toolbox routines. */

/* ------------------------------------------------------------------- NotelistVersion -- */
/* If the input file is a version of Notelist that we support, return a nonnegative
integer version number, else return -1. Assumes that the file position indicator is at
the top of the file. NB: This function advances the file position indicator.

If you change any of the COMMENT_NLHEADER's or add a new one, cf. ParseStructComment(). */

#define COMMENT_NLHEADER0	"%%Score file="			/* start of structured comment: before Ngale 3.1 */
#define COMMENT_NLHEADER1	"%%Score-V1 file="		/* start of structured comment: Ngale 3.1 thru early 99 */
#define COMMENT_NLHEADER2	"%%Notelist-V2 file="	/* start of structured comment: Ngale 99 */
#define COMMENT_LINELEN		1024

static short FSNotelistVersion(short refNum)
{
	int		c;
	Boolean	ok;
	char	dummybuf[COMMENT_LINELEN];
	char	fmtStr[256], errString[256];
	short	index = 1;						/* index of error string in NOTELIST_STRS 'STR#' */ 
	short	errCode,errStage=0;
	char	headerVerString[256];

	/* Skip over any whitespace at beginning of file. */
	while (True) {
		errCode = FSReadChar(refNum, (char*)&c);		
		if (errCode != noErr || c==EOF) { errStage = 1; goto Err; }
		else if (c==COMMENT_CHAR) {
			errCode = FSReadChar(refNum, (char*)&c);		
			if (errCode != noErr)  { errStage = 2; goto Err; }
			if (c==COMMENT_CHAR) {				/* found structured comment */
				long fPos;
				GetFPos(refNum,&fPos);
				printf("readComment: fpos %ld", fPos);
				
				errCode = FSUngetChar(refNum);		
				if (errCode != noErr)  { errStage = 3; goto Err; }
				errCode = FSUngetChar(refNum);		
				if (errCode != noErr)  { errStage = 4; goto Err; }
				
				GetFPos(refNum,&fPos);
				printf("readComment after FSUngetChar: fpos %ld", fPos);
				
				break;
			}
			else {								/* normal comment precedes structured comment */
				ok = FSReadLine(dummybuf, COMMENT_LINELEN, refNum);
				if (!ok) return -1;
			}
		}
		else if (!isspace(c)) {					/* found structured comment */
			errCode = FSUngetChar(refNum);		
			if (errCode != noErr)  { errStage = 5; goto Err; }
			break;
		}
	}
	
	/* See if file starts with a Notelist structured comment header (e.g.,
		"%%Score file='Mozart clart quintet'  partstaves=1 1 1 1 1 0")
	*/
	if (!FSReadLine(gInBuf, LINELEN, refNum))  { errStage = 6; goto Err; }
	
	GoodStrncpy(headerVerString, gInBuf, strlen(COMMENT_NLHEADER2));
	LogPrintf(LOG_NOTICE, "Notelist header string='%s'\n", headerVerString);

	if (strncmp(gInBuf, COMMENT_NLHEADER0, strlen(COMMENT_NLHEADER0))==0)
		return 0;
	if (strncmp(gInBuf, COMMENT_NLHEADER1, strlen(COMMENT_NLHEADER1))==0)
		return 1;
	if (strncmp(gInBuf, COMMENT_NLHEADER2, strlen(COMMENT_NLHEADER2))==0)
		return 2;

Err:
	printf(gInBuf);
	printf("error stage %d", errStage);
	
	/* There's something wrong with the Notelist header structured comment. Say so,
	 * but if user gives the "secret code", go ahead and open it anyway, assuming a
	 * recent version.
	 */
	GetIndCString(fmtStr, NOTELIST_STRS, index);
	GoodStrncpy(errString, gInBuf, 30);
	sprintf(strBuf, fmtStr, errString);
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
	if (ControlKeyDown()) return 2;
	return -1;
}


/* ------------------------------------------------------------------- PrintNotelistDS -- */

#if PRINTNOTELIST
static void PrintNotelistDS(void)
{
	long		time, len;
	Size		hSize;
	short		j, staves;
	char		npt, uV, stf, type, *p, *z, str[MAX_CHARS];
	NLINK		i, offset, numSlots;
	PNL_GENERIC	pG;
	PNL_HEAD	pH;
	PNL_NRGR	pN;
	PNL_TUPLET	pT;
	PNL_BARLINE	pB;
	PNL_CLEF	pC;
	PNL_KEYSIG	pKS;
	PNL_TIMESIG	pTS;
	PNL_TEMPO	pTM;
	PNL_GRAPHIC	pGR;
	PNL_DYNAMIC	pD;
	PNL_MOD		pMod;
	
	for (i=0; i<=gNumNLItems; i++) {
		pG = GetPNL_GENERIC(i);
		time = pG->lStartTime;
		npt = pG->part;
		uV = pG->uVoice;
		stf = pG->staff;
		type = pG->objType;

		switch (type) {
			case HEAD_TYPE:
				pH = GetPNL_HEAD(i);
				say("(%4d) HEAD:\tstrL=%d\n", i, pH->scoreName);
				for (j=1; j<=MAXSTAVES; j++) {
					staves = gPartStaves[j];
					if (staves==0) break;
					say ("\t\tPart %d: %d %s\n", j, staves, staves==1? "staff" : "staves");
				}
				break;
			case NR_TYPE:
			case GRACE_TYPE:
				pN = GetPNL_NRGR(i);
				say("(%4d) %s ", i, type==GRACE_TYPE? "GRACE:" : (pN->isRest? "REST: " : "NOTE: "));
				say("t=%ld, npt=%d, uV=%d, stf=%d, durCd=%d, dots=%d, acc=%d, eAcc=%d, nn=%d, 1stmod=%d\n",
						time, npt, uV, stf, pN->durCode, pN->nDots, pN->acc, pN->eAcc, pN->noteNum, pN->firstMod);
				break;
			case TUPLET_TYPE:
				pT = GetPNL_TUPLET(i);
				say("(%4d) TUPL:  t=%ld, npt=%d, uV=%d, stf=%d, num=%d, denom=%d%s%s%s\n",
						i, time, npt, uV, stf, pT->num, pT->denom,
						pT->numVis? ", numV" : "", pT->denomVis? ", denomV" : "", pT->brackVis? ", brackV" : "");
				break;
			case BARLINE_TYPE:
				pB = GetPNL_BARLINE(i);
				say("(%4d) BAR:   t=%ld, npt=%d, uV=%d, stf=%d, appear=%d\n",
						i, time, npt, uV, stf, pB->appear);
				break;
			case CLEF_TYPE:
				pC = GetPNL_CLEF(i);
				say("(%4d) CLEF:  t=%ld, npt=%d, uV=%d, stf=%d, type=%d\n",
						i, time, npt, uV, stf, pC->type);
				break;
			case KEYSIG_TYPE:
				pKS = GetPNL_KEYSIG(i);
				say("(%4d) KSIG:  t=%ld, npt=%d, uV=%d, stf=%d, numAcc=%d, sf=%c\n",
						i, time, npt, uV, stf, pKS->numAcc, pKS->sharp? '#' : 'b');
				break;
			case TIMESIG_TYPE:
				pTS = GetPNL_TIMESIG(i);
				say("(%4d) TSIG:  t=%ld, npt=%d, uV=%d, stf=%d, %d/%d, appear=%d\n",
						i, time, npt, uV, stf, pTS->num, pTS->denom, pTS->appear);
				break;
			case TEMPO_TYPE:
				pTM = GetPNL_TEMPO(i);
				say("(%4d) TEMPO: t=%ld, npt=%d, uV=%d, stf=%d, durCode=%d, dot=%d, bpmL=%d, strL=%d%s\n",
						i, time, npt, uV, stf, pTM->durCode, pTM->dotted, pTM->metroStr, pTM->string, pTM->hideMM? ", hideMM" : "");
				break;
			case GRAPHIC_TYPE:
				pGR = GetPNL_GRAPHIC(i);
				say("(%4d) GRAPH: t=%ld, npt=%d, uV=%d, stf=%d, type=%d, strL=%d\n",
						i, time, npt, uV, stf, pGR->type, pGR->string);
				break;
			case DYNAMIC_TYPE:
				pD = GetPNL_DYNAMIC(i);
				say("(%4d) DYN:   t=%ld, npt=%d, uV=%d, stf=%d, code=%d\n",
						i, time, npt, uV, stf, pD->type);
				break;
			default:
				say("(%4d) Unknown type! (%d)\n", i, type);
		}
	}

	/* Print note modifier list. */
	say("\n\nNOTE MODIFIERS.....................................................\n");
	hSize = GetHandleSize((Handle)gHModList);
	numSlots = hSize/sizeof(NL_MOD);
	HLock((Handle)gHModList);
	pMod = *gHModList;
	for (i=1, ++pMod; i<numSlots; i++, pMod++)
		say("(%4d)  code=%d, data=%d, next=%d\n", i, pMod->code, pMod->data, pMod->next);
	HUnlock((Handle)gHModList);
	
	/* Print string pool. */
	say("\n\nSTRING POOL........................................................\n");
	hSize = GetHandleSize(gHStringPool);
	HLock(gHStringPool);
	p = (char *)*gHStringPool;
	z = p + hSize;
	p += FIRST_OFFSET;
	do {
		len = strlen(p);
		offset = (NLINK)(p - (char *)*gHStringPool);
		if (len>MAX_CHARS-1L) {
			say("(%4d)  String too long! (len=%ld)\n", offset, len);
			continue;
		}
		BlockMove(p, str, (Size)len);
		str[len] = 0;
		say("(%4d)  \"%s\"\n", offset, str);
		p += len;
		if (!*p) p++;
		if (!*p) p++;
	} while (p<z);
	HUnlock(gHStringPool);
}
#endif /* #if PRINTNOTELIST */


/* ----------------------------------------------------------------------- StoreString -- */
/*	Store the C-string <str> into our string pool handle, gHStringPool. If <str> contains
an even number of bytes, not including its terminating null, add one extra null byte
at the end. (This is to avoid odd address accesses on 68000 Macs.)
Returns the offset of the start of the string within gHStringPool. Returns NILINK in
case of one of the following errors:
	1) <str> contains more than MAX_CHARS bytes,	including terminating null,
	2) we can't expand gHStringPool, or
	3) accommodating <str> would make offset larger than MAX_OFFSET. 
Note that gHStringPool begins with two bytes not used by any string, so a string will
never have an offset less than FIRST_OFFSET. */

static NLINK StoreString(char str[])
{
	NLINK	offset;
	long	len, bytesToAdd;
	Size	hSize;
	char	*p;
	
	len = strlen(str);								/* <len> doesn't include terminating null */
	if (len>MAX_CHARS-1L) return NILINK;
	bytesToAdd = odd(len)? len+1 : len+2;

	hSize = GetHandleSize(gHStringPool);
	if (hSize>MAX_OFFSET) return NILINK;
	SetHandleSize(gHStringPool, hSize+bytesToAdd);
	if (MemError()) {
		NoMoreMemory();
		return False;
	}
	offset = (NLINK) hSize;
	
	HLock(gHStringPool);
	p = (char *)*gHStringPool;
	p += offset;
	BlockMove(str, p, (Size)len);
	p[len] = 0;
	if (!odd(len)) p[len+1] = 0;
	HUnlock(gHStringPool);

	return offset;
}


/* ----------------------------------------------------------------------- FetchString -- */
/*	Make a copy of the C-string beginning at <offset> bytes from the start of gHStringPool.
Copy into <str>, which must be large enough to hold MAX_CHARS (including terminating null).
Returns False if <offset> is odd or out of range; otherwise returns True.
Note that gHStringPool begins with two bytes not used by any string, so a string will
never have an offset less than FIRST_OFFSET. */

Boolean FetchString(NLINK	offset,		/* offset of requested string from start of gHStringPool */
					char	str[])		/* buffer to hold C-string; must be MAX_CHARS long */
{
	char	*p;
	long	len;
	Size	size;

	size = GetHandleSize(gHStringPool);
	if (offset<FIRST_OFFSET || offset>size-1 || odd(offset)) return False;
	
	HLock(gHStringPool);
	p = (char *)*gHStringPool;
	p += offset;
	len = strlen(p);
	if (len>MAX_CHARS-1L) return False;
	BlockMove(p, str, (Size)len);
	str[len] = '\0';
	HUnlock(gHStringPool);
	return True;
}


/* --------------------------------------------------------------------- StoreModifier -- */
/*	Store the given note modifier in gHModList, first expanding the block to accommodate it.
Return index into this 1-based array if ok, NILINK if error. */

static NLINK StoreModifier(PNL_MOD pMod)
{
	NLINK	offset;
	PNL_MOD	p;
	Size	size;

	size = GetHandleSize((Handle)gHModList);
	
	SetHandleSize((Handle)gHModList, size+sizeof(NL_MOD));
	if (MemError()) {
		NoMoreMemory();
		return NILINK;
	}

	offset = size/sizeof(NL_MOD);
	p = *gHModList + (long)offset;
	p->next = pMod->next;
	p->code = pMod->code;
	p->data = pMod->data;

	return offset;
}


/* --------------------------------------------------------------------- FetchModifier -- */
/*	Fill in the given modifier struct from the specified modifier in the gHModList array.
(NB: gHModList is a relocatable 1-based array.) Return True if ok, False if error. */

Boolean FetchModifier(NLINK modL, PNL_MOD pMod)
{
	Size	size;
	PNL_MOD	p;

	size = GetHandleSize((Handle)gHModList);
	if (modL<1 || modL>(size/sizeof(NL_MOD))-1) return False;

	p = *gHModList + modL;
	
	pMod->next = p->next;
	pMod->code = p->code;
	pMod->data = p->data;

	return True;
}


/* --------------------------------------------------------------- AllocNotelistMemory -- */
/* Allocate memory for the intermediate Notelist data structure.
Returns True if ok, False if error (after giving NoMoreMemory alert).
NB: In the case of an error here, a function higher in the calling chain should dispose
of whatever memory was allocated before the error by calling DisposNotelistMemory.
CRUCIAL: We use NewPtrClear rather than NewPtr to insure that all fields will be set
to zero. We allocate minimal blocks for gHModList and gHStringPool, since we don't know
how big they'll have to be at this point. We expand them later as needed. */

static Boolean AllocNotelistMemory(void)
{
	Ptr		p;
	Handle	h;
	
	/* Must initialize to NULL so that if we fail partway through allocation, the
	   subsequent call to DisposNotelistMemory will be safe. */
	   
	gNodeList = (PNL_NODE)NULL;
	gHModList = (HNL_MOD)NULL;
	gHStringPool = NULL;

	p = NewPtrClear((Size) ((gNumNLItems+1) * sizeof(NL_NODE)) );
	if (!GoodNewPtr(p)) goto broken;
	gNodeList = (PNL_NODE)p;

	h = NewHandle((Size) sizeof(NL_MOD));
	if (!GoodNewHandle(h)) goto broken;
	gHModList = (HNL_MOD)h;

	h = NewHandle(2L);
	if (!GoodNewHandle(h)) goto broken;
	gHStringPool = h;
	
	return True;
	
broken:
	NoMoreMemory();
	return False;
}


/* -------------------------------------------------------------- DisposNotelistMemory -- */

void DisposNotelistMemory(void)
{
	if (gNodeList) DisposePtr((char *)gNodeList);
	if (gHModList) DisposeHandle((Handle)gHModList);
	if (gHStringPool) DisposeHandle(gHStringPool);

	gNodeList = (PNL_NODE)NULL;
	gHModList = (HNL_MOD)NULL;
	gHStringPool = NULL;
}
