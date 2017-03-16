/***************************************************************************
*	FILE:	NotelistOpen.c
*	PROJ:	Nightingale
*	DESC:	Routines for opening a Nightingale Notelist file, creating a native
*			Nightingale file from it. Written by John Gibson with help from Tim
*			Crawford.
***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "Notelist.h"


/* --------------------------------------------------------------------------------- */

/* Constants */

/* Default qtr-space position relative to staff top of text, lyrics, tempo marks and dynamics. */
#define DFLT_TEMPO_HEIGHT		-24
#define DFLT_LYRIC_HEIGHT		28
#define DFLT_TEXT_HEIGHT		-10
#define DFLT_DYNAMIC_HEIGHT		32

/* Default DDIST position of page-rel graphics, relative to origin (top,left) of page. */
#define DFLT_PGRELGR_XD			250
#define DFLT_PGRELGR_YD			200
/* Tiling offset (in DDISTs) for page-rel graphics. */
#define PGRELGR_XOFFSET			100
#define PGRELGR_LEADING			200


/* Local prototypes */

static short GetShortestDurCode(short voice, LINK startL, LINK endL);
static Boolean NotelistToNight(Document *doc);
static Boolean ConvertNoteRest(Document *doc, NLINK pL);
static Boolean ConvertGrace(Document *doc, NLINK pL);
static Boolean ConvertMods(Document *doc, NLINK firstModID, LINK syncL, LINK aNoteL);
static Boolean NLMIDI2HalfLn(char noteNum, char eAcc, short midCHalfLn, short *pHalfLn);
static Boolean ConvertTuplet(Document *doc, NLINK pL);
static Boolean ConvertBarline(Document *doc, NLINK pL);
static Boolean ConvertClef(Document *doc, NLINK pL);
static Boolean ConvertKeysig(Document *doc, NLINK pL);
static Boolean ConvertTimesig(Document *doc, NLINK pL);
static Boolean ConvertTempo(Document *doc, NLINK pL);
static Boolean ConvertGraphic(Document *doc, NLINK pL);
static Boolean ConvertDynamic(Document *doc, NLINK pL);
static Boolean SetDefaultCoords(Document *doc);
static void FixPlayDurs(Document *doc, LINK startL, LINK endL);

static Boolean SetupNLScore(Document *doc);
static Document *CreateNLDoc(unsigned char *fileName);
static Boolean BuildNLDoc(Document *doc, long *version, short pageWidth,  short pageHt, short
							nStaves, short rastral);
static void DisplayNLDoc(Document *newDoc);

static short NLtoNightKeysig(NLINK keysigL);
static Boolean IsFirstKeysig(NLINK ksL);
static Boolean IsFirstTimesig(NLINK tsL);
static char DynamicGlyph(LINK dynamicL);

/* Functions that call the Macintosh Toolbox */
static void BuildConvertedNLFileName(Str255 fn, Str255 newfn);


/* Globals declared in NotelistParse.c */

extern PNL_NODE		gNodeList;
extern PNL_MOD		gModList;
extern Handle		gHStringPool;

extern NLINK		gNumNLItems;
extern long			gLastTime;
extern SignedByte	gPartStaves[];
extern SignedByte	gNumNLStaves;
extern short		gFirstMNNumber;
extern short		gHairpinCount;
extern Boolean		gDelAccs;						/* delete redundant accidentals? */

/* Local globals */
static long			gCurSyncTime;


/* ------------------------------------------------------------- OpenNotelistFile -- */
/* The top level, public function. Returns TRUE on success, FALSE if there's a problem. */

Boolean OpenNotelistFile(Str255 fileName, NSClientDataPtr pNSD)
{
	Str255		newfn;
	Document	*doc;
	char		configDisableUndo;
	Boolean		result;
	PNL_HEAD	pHead;
	char		fmtStr[256];
	FSSpec		fsSpec;
	
	gHairpinCount = 0;
	fsSpec = pNSD->nsFSSpec;
	result = ParseNotelistFile(fileName, &fsSpec);
	if (!result) return FALSE;
	if (gHairpinCount>0) {
		GetIndCString(fmtStr, NOTELIST_STRS, 3);	/* "Open Notelist can't handle hairpins..." */
		sprintf(strBuf, fmtStr, gHairpinCount);
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
	}

	/* Grab score name from the Notelist header. If that's empty build a filename. */
	pHead = GetPNL_HEAD(0);
	if (pHead->scoreName) {
		result = FetchString(pHead->scoreName, (char *)newfn);
		if (!result) goto buildit;
		CtoPstr((StringPtr)newfn);
	}
	else
buildit:
		BuildConvertedNLFileName(fileName, newfn);

	doc = CreateNLDoc(newfn);
	if (doc==NULL) {
		GetIndCString(strBuf, NOTELIST_STRS, 2);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		goto Error;
	}

	configDisableUndo = config.disableUndo;					/* Save this. */
	config.disableUndo = TRUE;								/* In case we invoke PrepareUndo. */
	
	result = NotelistToNight(doc);
	
	config.disableUndo = configDisableUndo;

	ProgressMsg(0, "");										/* Remove progress window. */

	if (!result) {
		doc->changed = FALSE;
		DoCloseDocument(doc);
	}
	else
		DisplayNLDoc(doc);

Error:
	ProgressMsg(0, "");										/* Remove progress window, if it hasn't already been removed. */
	DisposNotelistMemory();
	return result;
}


/* ------------------------------------------------------------- OpenNotelistFile -- */
/* The top level, public function; FSSpec calling sequence version. As of v. 5.7, this
version is used iff user drops a notelist file on the Nightingale icon. Returns TRUE
on success, FALSE if there's a problem. */

Boolean OpenNotelistFile(Str255 fileName, FSSpec *fsSpec)
{
	Str255		newfn;
	Document	*doc;
	char		configDisableUndo;
	Boolean		result;
	PNL_HEAD	pHead;
	char		fmtStr[256];
	//FSSpec	fsSpec;
	
	LogPrintf(LOG_DEBUG, "In FSSpec version of OpenNF\n");
	gHairpinCount = 0;
	//fsSpec = pNSD->nsFSSpec;
	//result = ParseNotelistFile(fileName, &fsSpec);
	result = ParseNotelistFile(fileName, fsSpec);
	if (!result) return FALSE;
	if (gHairpinCount>0) {
		GetIndCString(fmtStr, NOTELIST_STRS, 3);	/* "Open Notelist can't handle hairpins..." */
		sprintf(strBuf, fmtStr, gHairpinCount);
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
	}
	
	/* Grab score name from the Notelist header. If that's empty build a filename. */
	pHead = GetPNL_HEAD(0);
	if (pHead->scoreName) {
		result = FetchString(pHead->scoreName, (char *)newfn);
		if (!result) goto buildit;
		CtoPstr((StringPtr)newfn);
	}
	else
		buildit:
		BuildConvertedNLFileName(fileName, newfn);
	
	doc = CreateNLDoc(newfn);
	if (doc==NULL) {
		GetIndCString(strBuf, NOTELIST_STRS, 2);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		goto Error;
	}
	
	configDisableUndo = config.disableUndo;					/* Save this. */
	config.disableUndo = TRUE;								/* In case we invoke PrepareUndo. */
	
	result = NotelistToNight(doc);
	
	config.disableUndo = configDisableUndo;
	
	ProgressMsg(0, "");										/* Remove progress window. */
	
	if (!result) {
		doc->changed = FALSE;
		DoCloseDocument(doc);
	}
	else
		DisplayNLDoc(doc);
	
Error:
	ProgressMsg(0, "");										/* Remove progress window, if it hasn't already been removed. */
	DisposNotelistMemory();
	return result;
}


/* --------------------------------------------------------------------------------- */
/* Functions for translating the Notelist data structure into a Nightingale score */

/* Get the note subtype code for the shortest duration of any note/chord in the given
range in the given voice. */

static short GetShortestDurCode(short voice, LINK startL, LINK endL)
{
	LINK pL, aNoteL;
	short maxDurCode;
	
	maxDurCode = SHRT_MIN;
	for (pL = startL; pL!= endL; pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (voice==ANYONE || NoteVOICE(aNoteL)==voice) {
					maxDurCode = n_max(maxDurCode, NoteType(aNoteL));
					break;						/* Assumes all notes in chord have same duration. */
				}
			}
		}
	return maxDurCode;
}

/* -------------------------------------------------------------- NotelistToNight -- */

static Boolean NotelistToNight(Document *doc)
{
	short		v;
	long		spacePercent;
	LINK		firstMeasL, pL;
	Boolean		ok, result, doReformat;
	PNL_GENERIC	pG;
	NLINK		itemNL;
	char		fmtStr[256];
	short		shortestDurCode;
	
	result = SetupNLScore(doc);
	if (!result) return FALSE;
	
	gCurSyncTime = -1L;
	ConvertGrace(doc, NILINK);										/* Initialization */
	
	for (itemNL=1; itemNL<=gNumNLItems; itemNL++) {
		pG = GetPNL_GENERIC(itemNL);
		switch (pG->objType) {
			case NR_TYPE:
				result = ConvertNoteRest(doc, itemNL);
				break;
			case GRACE_TYPE:
				result = ConvertGrace(doc, itemNL);
				break;
			case TUPLET_TYPE:
				result = ConvertTuplet(doc, itemNL);
				break;
			case BARLINE_TYPE:
				result = ConvertBarline(doc, itemNL);
				break;
			case CLEF_TYPE:
				result = ConvertClef(doc, itemNL);
				break;
			case KEYSIG_TYPE:
				result = ConvertKeysig(doc, itemNL);
				break;
			case TIMESIG_TYPE:
				result = ConvertTimesig(doc, itemNL);
				break;
			case TEMPO_TYPE:
				result = ConvertTempo(doc, itemNL);
				break;
			case GRAPHIC_TYPE:
				result = ConvertGraphic(doc, itemNL);
				break;
			case DYNAMIC_TYPE:
				result = ConvertDynamic(doc, itemNL);
				break;
			default:
				MayErrMsg("NotelistToNight: bad objType: %d.", pG->objType);
		}
		
		/* If there's a problem, give up. This may be overkill: we should be able to
			recover and continue converting. But that has caused problems, so it's safer
			to abort. */
		
		if (!result) {
			GetIndCString(fmtStr, NOTELIST_STRS, 30);	/* "Open Notelist can't convert item..." */
			sprintf(strBuf, fmtStr, gCurSyncTime, pG->part, pG->uVoice, pG->staff);
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
	}

	ProgressMsg(CONVERTNOTELIST_PMSTR, " 2...");	/* "Converting Notelist file: step 2..." */

	/* Fix up LINKs of all Tuplets in the score. Note that FIFixTupletLinks also resets
	 * play durations of notes in tuplets: this is not good, since we've already set
	 * them from the notelist, but it should be easy to get rid of when someone has
	 * a bit of time.
	 */
	for (v = 1; v<=MAXVOICES; v++)
		FIFixTupletLinks(doc, doc->headL, doc->tailL, v);

	FIAnchorAllJDObjs(doc);
	
	FixTimeStamps(doc, doc->headL, NILINK);
	FixPlayDurs(doc, doc->headL, doc->tailL);

/* If requested, delete all redundant accidentals. */
	if (gDelAccs) {
		SelAllNoHilite(doc);
		DelRedundantAccs(doc, ANYONE, DELALL_REDUNDANTACCS_DI);
	}

	/* Create slurs/ties before reformatting, so that we can get cross-sys & cross-pg slurs/ties. */
	CreateAllTies(doc);
	FICreateAllSlurs(doc);
	FIFixAllNoteSlurTieFlags(doc);
	
	/* Combine adjacent keysigs on different staves into the same object. */
	if (gNumNLStaves>1)
		ok = FICombineKeySigs(doc, doc->headL, doc->tailL);
	
	/* Respace entire score, trying to avoid both overcrowding and wasted spoce. */
	SelAllNoHilite(doc);
	firstMeasL = LSSearch(doc->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	doc->autoRespace = TRUE;

	/* If there aren't any short notes, save some paper. Cf. MFRespAndRfmt(). */
	shortestDurCode = GetShortestDurCode(ANYONE, firstMeasL, doc->tailL);
	spacePercent = 100L;
	if (shortestDurCode==EIGHTH_L_DUR) spacePercent = 85L;
	else if (shortestDurCode<EIGHTH_L_DUR && shortestDurCode!=UNKNOWN_L_DUR) spacePercent = 70L;
	LogPrintf(LOG_NOTICE, "shortestDurCode=%d spacePercent=%ld\n", shortestDurCode, spacePercent);
	doc->spacePercent = spacePercent;
	ok = RespaceBars(doc, firstMeasL, doc->tailL, RESFACTOR*spacePercent, FALSE, doReformat=TRUE);

	ProgressMsg(CONTINUENOTELIST_PMSTR, "");

#if DOAUTOBEAM
	SelAllNoHilite(doc);
	ok = AutoBeam(doc);
#endif

	FIAutoMultiVoice(doc, FALSE);

	/* Set tuplet accessory nums to a reasonable vertical position. Do *after* beaming! */
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		if (TupletTYPE(pL))
			SetTupletYPos(doc, pL);

	FIJustifyAll(doc);												/* Justify every system. */
	
	/* Reshape slurs and ties to default; respace/reformat will have deformed many of them. */
	FIReshapeSlursTies(doc);

	FIAnchorAllJDObjs(doc);
	SetDefaultCoords(doc);

	DeselRangeNoHilite(doc, doc->headL, doc->tailL);
	SetDefaultSelection(doc);
	doc->selStaff = 1;
	MEAdjustCaret(doc, FALSE);

	return TRUE;
}


/* -------------------------------------------------------------- ConvertNoteRest -- */
/* Assumes <pL> is a note or rest, not a grace note. */

static Boolean ConvertNoteRest(Document *doc, NLINK pL)
{
	PNL_NRGR	pNR;
	PANOTE		aNote;
	LINK		syncL, aNoteL, partL;
	short		iVoice, halfLn, midCHalfLn;
	Boolean		result, isRest;
	CONTEXT		context;
	static LINK	curSyncL;

	if (gCurSyncTime==-1L) curSyncL = NILINK;				/* Init this on first call of conversion. */
	syncL = NILINK;
	
	pNR = GetPNL_NRGR(pL);
	isRest = pNR->isRest;
	
	/*	If lStartTime of note (or rest) is the same as gCurSyncTime, then this 
		note belongs in a sync already created. Else if lStartTime is after 
		gCurSyncTime, we create a new sync and put this note in it. Else if 
		lStartTime is before gCurSyncTime, we've got an error. (This shouldn't
		happen, because we've already checked the notelist for consistency in
		PostProcessNotelist.)
	*/
	if (pNR->lStartTime==gCurSyncTime) {
		aNoteL = FIAddNoteToSync(doc, curSyncL);
		if (aNoteL==NILINK) return FALSE;					/* probably out of memory */
	}
	else if (pNR->lStartTime>gCurSyncTime) {
		syncL = FIInsertSync(doc, doc->tailL, 1);
		if (syncL==NILINK) return FALSE;
		aNoteL = FirstSubLINK(syncL);
		curSyncL = syncL;
		gCurSyncTime = pNR->lStartTime;
	}
	else {
		MayErrMsg("ConvertNoteRest: Note/rest (%d) has starttime (%ld) before current time (%ld).",
					pL, pNR->lStartTime, gCurSyncTime);
		return FALSE;
	}

	partL = FindPartInfo(doc, pNR->part);
	iVoice = User2IntVoice(doc, (short)pNR->uVoice, partL);
	if (iVoice==0) {
		MayErrMsg("ConvertNoteRest: can't get internal voice number (time=%ld)", pNR->lStartTime);
		return FALSE;
	}

	/* Get the staff-relative halfline that SetupNote expects. */
	if (!isRest) {
		GetContext(doc, curSyncL, pNR->staff, &context);
		midCHalfLn = ClefMiddleCHalfLn(context.clefType);
		result = NLMIDI2HalfLn(pNR->noteNum, pNR->eAcc, midCHalfLn, &halfLn);
		if (!result) {
			sprintf(strBuf, "Can't understand pitch of note (time=%ld). Maybe an inconsistent accidental.", pNR->lStartTime);
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
	}
	else {
		halfLn = 0;						/* unused in SetupNote for rests */
	}

	SetupNote(doc, curSyncL, aNoteL, pNR->staff, halfLn, 
				pNR->durCode, pNR->nDots, iVoice, isRest, pNR->eAcc, 0);

	aNote = GetPANOTE(aNoteL);
	aNote->accident = pNR->acc;
	aNote->inTuplet = pNR->inTuplet;
	aNote->tiedR = pNR->tiedR;							/* flag for subsequent call to CreateAllTies */
	aNote->tiedL = pNR->tiedL;
	aNote->slurredR = pNR->slurredR;					/* flag for subsequent call to CreateAllSlurs */
	aNote->slurredL = pNR->slurredL;
	aNote->headShape = pNR->appear;
	if (!isRest) {
		aNote->onVelocity = pNR->vel;
		aNote->playDur = pNR->pDur;
	}

	if (pNR->inChord && syncL==NILINK)					/* NB: syncL==0 if we're adding to sync */
		result = FixSyncForChord(doc, curSyncL, iVoice,
					FALSE/*beamed*/, 0/*stemUpDown*/, 0/*voices1orMore*/, NULL);
	
	if (pNR->firstMod) {
		result = ConvertMods(doc, pNR->firstMod, curSyncL, aNoteL);
		if (!result) return FALSE;
	}

	return TRUE;
broken:
	MayErrMsg("ConvertNoteRest failed (pL=%d)", pL);
	return FALSE;
}


/* ----------------------------------------------------------------- ConvertGrace -- */
/* Assumes <pL> is a grace note, not a normal note or rest.
CAUTION: At this time we do not support:
	grace note rests,
	ties, slurs or note modifiers attached to grace notes.

Calling ConvertGrace with NILINK for <pL> tells it to initialize a static variable.
Do this once before converting a score.

NB: There is a flaw in the current Notelist spec that makes it impossible to decide
whether two consecutive grace notes are part of the same Sync. If a grace note is
in a (grace) chord, its inChord flag is set, but there's no way to tell *which*
chord it's in. Until this is fixed (probably by making lStartTime meaningful), we
can't do a very smart job of syncing. */

static Boolean ConvertGrace(Document *doc, NLINK pL)
{
	PNL_NRGR	pNR;
	PAGRNOTE	aGRNote;
	LINK		syncL, aGRNoteL, partL;
	short		iVoice, halfLn, midCHalfLn;
	Boolean		result;
	CONTEXT		context;
	static LINK	curGRSyncL;

	if (pL==NILINK) {										/* Init static variable only. */
		curGRSyncL = NILINK;
		return TRUE;
	}
	
	pNR = GetPNL_NRGR(pL);
	
	syncL = NILINK;
	if (curGRSyncL && !pNR->inChord)
		curGRSyncL = NILINK;

	if (curGRSyncL) {
		aGRNoteL = FIAddNoteToSync(doc, curGRSyncL);
		if (aGRNoteL==NILINK) return FALSE;
	}
	else {
		syncL = FIInsertGRSync(doc, doc->tailL, 1);
		if (syncL==NILINK) return FALSE;
		aGRNoteL = FirstSubLINK(syncL);
		curGRSyncL = syncL;
	}

	partL = FindPartInfo(doc, pNR->part);
	iVoice = User2IntVoice(doc, (short)pNR->uVoice, partL);
	if (iVoice==0) {
		MayErrMsg("ConvertGrace: can't get internal voice number (pL=%d)", pL);
		return FALSE;
	}

	/* Get the staff-relative halfline that SetupNote expects. */
	GetContext(doc, curGRSyncL, pNR->staff, &context);
	midCHalfLn = ClefMiddleCHalfLn(context.clefType);
	result = NLMIDI2HalfLn(pNR->noteNum, pNR->eAcc, midCHalfLn, &halfLn);
	if (!result) {
		sprintf(strBuf, "Can't understand pitch of grace note (pL=%d). Maybe an inconsistent accidental", pL);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	SetupGRNote(doc, curGRSyncL, aGRNoteL, pNR->staff, halfLn, 
				pNR->durCode, pNR->nDots, iVoice, pNR->eAcc, 0);

	aGRNote = GetPAGRNOTE(aGRNoteL);
	aGRNote->accident = pNR->acc;
	aGRNote->inTuplet = FALSE;
	aGRNote->tiedR = FALSE;
	aGRNote->tiedL = FALSE;
	aGRNote->slurredR = FALSE;
	aGRNote->slurredL = FALSE;
	aGRNote->headShape = pNR->appear;
	aGRNote->onVelocity = pNR->vel;
	aGRNote->playDur = pNR->pDur;

	if (pNR->inChord && syncL==NILINK)					/* NB: syncL==0 if we're adding to sync */
		result = FixGRSyncForChord(doc, curGRSyncL, iVoice,
					FALSE/*beamed*/, 0/*stemUpDown*/, 0/*voices1orMore*/, NULL);
	
	return TRUE;
broken:
	MayErrMsg("ConvertGrace failed (pL=%d)", pL);
	return FALSE;
}


/* ------------------------------------------------------------------ ConvertMods -- */
/* Convert all the modifiers attached to the given note. Until we have a reasonable
auto-placement algorithm (begun in ModNRPitchLev), do something hopelessly crude.
NB: Don't confuse modifiers in the Notelist temporary data structure with
those in the Nightingale object list!
Return TRUE if ok, FALSE if error. */

static Boolean ConvertMods(Document *doc, NLINK firstModID, LINK syncL, LINK aNoteL)
{
	NLINK	thisModID;
	NL_MOD	aNLMod;
	PAMODNR	pMod;
	LINK	aModL;
	Boolean	result;
	
	for (thisModID = firstModID; thisModID; thisModID = aNLMod.next) {
		result = FetchModifier(thisModID, &aNLMod);
		if (!result) {
			MayErrMsg("ConvertMods: couldn't retrieve modifier (modID = %d).", thisModID);
			return FALSE;
		}
		aModL = AutoNewModNR(doc, aNLMod.code, aNLMod.data, syncL, aNoteL);
		if (!aModL) return FALSE;
		pMod = GetPAMODNR(aModL);
		pMod->ystdpit = 0;		/* ystdpit is clef-independent dist below mid-C in 8th-spaces */
	}
	
	return TRUE;
}


/* ---------------------------------------------------------------- NLMIDI2HalfLn -- */
/* Given a MIDI note number <noteNum>, an effective accidental <eAcc> code, and the
half-line  position of middle C <midCHalfLn>, fill in the note's half-line number in
<pHalfLn> (where top line of staff = 0). Return TRUE if ok, FALSE if error.
Because we're too lazy really to study the issue, we use a brute-force table lookup
algorithm. (Adapted from MIDI2HalfLn in MIDIRecUtils.c.) */

#define _X	-99		/* marker for useless array slot */

static Boolean NLMIDI2HalfLn(char noteNum, char eAcc, short midCHalfLn, short *pHalfLn)
{
	short	pitchClass, halfLines, octave, halfSteps;
	
	static char hLinesTable[5][12] = {
							{ 1,  _X, 2,  3,  _X, 4,  _X, 5,  _X, 6,  7,  _X },		/* AC_DBLFLAT */
		  /* pitchclass: Dbb Db  Ebb Fbb E   Gbb Gb  Abb Ab  Bbb Cbb B */

							{ _X, 1,  _X, 2,  3,  _X, 4,  _X, 5,  _X, 6,  7 },		/* AC_FLAT */
		  /* pitchclass: C   Db  D   Eb  Fb  F   Gb  G   Ab  A   Bb  Cb */

							{ 0,  _X, 1,  _X, 2,  3,  _X, 4,  _X, 5,  _X, 6 },		/* AC_NATURAL */
		  /* pitchclass: C   C#  D   D#  E   F   F#  G   G#  A   A#  B */

							{ -1, 0,  _X, 1,  _X, 2,  3,  _X, 4,  _X, 5,  _X },		/* AC_SHARP */
		  /* pitchclass: B#  C#  D   D#  E   E#  F#  G   G#  A   A#  B */

							{ _X, -1, 0,  _X, 1,  _X, 2,  3,  _X, 4,  _X, 5 } };	/* AC_DBLSHARP */
		  /* pitchclass: C   Bx  Cx  D#  Dx  F   Ex  Fx  G#  Gx  A#  Ax */

	
	*pHalfLn = 0;
	
	if (eAcc<AC_DBLFLAT || eAcc>AC_DBLSHARP) return FALSE;
	if (noteNum<0) return FALSE;
	
	pitchClass = noteNum % 12;					
	halfSteps = hLinesTable[eAcc-AC_DBLFLAT][pitchClass];
	if (halfSteps==_X) return FALSE;
	octave = (noteNum/12) - 5;
	halfLines = octave * 7 + halfSteps;
		
	*pHalfLn = (-halfLines+midCHalfLn);
	return TRUE;
}


/* ---------------------------------------------------------------- ConvertTuplet -- */

static Boolean ConvertTuplet(Document *doc, NLINK pL)
{
	short		iVoice;
	PNL_TUPLET	pT;
	LINK		partL, tupletL;
	
	pT = GetPNL_TUPLET(pL);
	
	partL = FindPartInfo(doc, pT->part);
	iVoice = User2IntVoice(doc, (short)pT->uVoice, partL);
	if (iVoice==0) goto broken;

	tupletL = FICreateTUPLET(doc, iVoice, pT->staff, doc->tailL, pT->nInTuple,
								pT->num, pT->denom, pT->numVis, pT->denomVis, pT->brackVis);
	if (tupletL==NILINK) goto broken;

	return TRUE;
broken:
	MayErrMsg("ConvertTuplet: couldn't make tuplet (pL=%d).", pL);
	return FALSE;
}


/* --------------------------------------------------------------- ConvertBarline -- */

static Boolean ConvertBarline(Document *doc, NLINK pL)
{
	PNL_BARLINE	pBar;
	LINK		measL;
	
	pBar = GetPNL_BARLINE(pL);
	
	measL = FIInsertBarline(doc, doc->tailL, pBar->appear);
	if (measL==NILINK) goto broken;

	return TRUE;
broken:
	MayErrMsg("ConvertBarline: couldn't make barline (pL=%d).", pL);
	return FALSE;
}


/* ------------------------------------------------------------------ ConvertClef -- */

static Boolean ConvertClef(Document *doc, NLINK pL)
{
	PNL_CLEF	pC;
	LINK		clefL, aClefL;
	Boolean		small;
	
	pC = GetPNL_CLEF(pL);

	/* If the staff this clef belongs on already has a clef of that
		type in effect, don't add this one. (We do this because Ngale's
		Save Notelist command saves every clef it finds, including all
		clefs in reserved area.)
	*/
	clefL = LSSearch(doc->tailL, CLEFtype, pC->staff, GO_LEFT, FALSE);
	if (clefL)
		for (aClefL = FirstSubLINK(clefL); aClefL; aClefL = NextCLEFL(aClefL))
			if (ClefSTAFF(aClefL)==pC->staff && ClefType(aClefL)==pC->type)
				return TRUE;

	clefL = FIInsertClef(doc, pC->staff, doc->tailL, pC->type, small=TRUE);
	if (clefL==NILINK) goto broken;

	return TRUE;
broken:
	MayErrMsg("ConvertClef failed (pL=%d)", pL);
	return FALSE;
}


/* ---------------------------------------------------------------- ConvertKeysig -- */

static Boolean ConvertKeysig(Document *doc, NLINK pL)
{
	short		sharpsOrFlats;
	PNL_KEYSIG	pKS;
	LINK		keySigL;
	
	/* We've already converted the first key sig. in the score, during SetupNLScore.
		If <pL> belongs to that key sig., skip it. */
	if (IsFirstKeysig(pL)) return TRUE;

	pKS = GetPNL_KEYSIG(pL);

	sharpsOrFlats = NLtoNightKeysig(pL);

	keySigL = FIInsertKeySig(doc, pKS->staff, doc->tailL, sharpsOrFlats);
	if (keySigL==NILINK) goto broken;

	return TRUE;
broken:
	MayErrMsg("ConvertKeysig failed (pL=%d)", pL);
	return FALSE;
}


/* --------------------------------------------------------------- ConvertTimesig -- */

static Boolean ConvertTimesig(Document *doc, NLINK pL)
{
	PNL_TIMESIG	pTS;
	LINK		timeSigL;
	
	/* We've already converted the first time sig. in the score, during SetupNLScore.
		If <pL> belongs to that time sig., skip it. */
	if (IsFirstTimesig(pL)) return TRUE;

	pTS = GetPNL_TIMESIG(pL);
	if (pTS->staff==NOONE) return TRUE;			/* don't convert it -- already handled by another record */
	
	timeSigL = FIInsertTimeSig(doc, pTS->staff, doc->tailL, pTS->appear, pTS->num, pTS->denom);
	if (timeSigL==NILINK) goto broken;

	return TRUE;
broken:
	MayErrMsg("ConvertTimesig failed (pL=%d)", pL);
	return FALSE;
}


/* ----------------------------------------------------------------- ConvertTempo -- */

static Boolean ConvertTempo(Document *doc, NLINK pL)
{
	PNL_TEMPO	pT;
	Boolean		result;
	char		tempoStr[MAX_TEMPO_CHARS], metroStr[MAX_TEMPO_CHARS];
	LINK		tempoL;
	
	pT = GetPNL_TEMPO(pL);

	if (pT->string) {
		result = FetchString(pT->string, tempoStr);
		if (!result) goto broken;
	}
	else
		tempoStr[0] = '\0';

	if (pT->metroStr) {
		result = FetchString(pT->metroStr, metroStr);
		if (!result) goto broken;
	}
	else
		metroStr[0] = '\0';

	tempoL = FIInsertTempo(doc, pT->staff, doc->tailL, NILINK, pT->durCode,
								pT->dotted, pT->hideMM, tempoStr, metroStr);
	if (tempoL==NILINK) goto broken;
	
	return TRUE;
broken:
	MayErrMsg("ConvertTempo failed (pL=%d)", pL);
	return FALSE;
}


/* --------------------------------------------------------------- ConvertGraphic -- */

static Boolean ConvertGraphic(Document *doc, NLINK pL)
{
	PNL_GRAPHIC	pG;
	Boolean		result, isLyric;
	short		textStyle, iVoice;
	char		str[MAX_CHARS];
	LINK		graphicL, partL, insertBeforeL, anchorL;
	
	pG = GetPNL_GRAPHIC(pL);

	if (!pG->string) return FALSE;					/* fail silently */
	
	result = FetchString(pG->string, str);
	if (!result) goto broken;

	isLyric = (pG->type==GRLyric);
	textStyle = (isLyric? TSRegular4STYLE : TSRegular1STYLE);
	if (pG->style!=TSNoSTYLE)
		textStyle = pG->style;

	if (pG->part==NOONE)
		iVoice = NOONE;
	else {
		partL = FindPartInfo(doc, pG->part);
		iVoice = User2IntVoice(doc, (short)pG->uVoice, partL);
	}
	
	/* Anchor page-rel graphics to first (and, so far, only) page. */
	if (pG->staff==NOONE && iVoice==NOONE) {
		LINK pageL = LSSearch(doc->headL, PAGEtype, ANYONE, GO_RIGHT, FALSE);
		insertBeforeL = RightLINK(pageL);
		anchorL = pageL;
	}
	else {
		insertBeforeL = doc->tailL;
		anchorL = NILINK;							/* filled in later */
	}
	
	graphicL = FIInsertGRString(doc, pG->staff, iVoice, insertBeforeL, 
									anchorL, isLyric, textStyle, NULL, str);
	if (graphicL==NILINK) goto broken;

	return TRUE;
broken:
	MayErrMsg("ConvertGraphic failed (pL=%d)", pL);
	return FALSE;
}


/* --------------------------------------------------------------- ConvertDynamic -- */

static Boolean ConvertDynamic(Document *doc, NLINK pL)
{
	PNL_DYNAMIC	pD;
	LINK		dynamicL;
	char		str[32];
	char		*dynamStrs[] = { "pppp", "", "", "", "", "", "", "", "", "ffff",
									"pi piano", "meno piano", "meno forte", "pi forte", "",
									"fz", "sfz", "rf", "rfz", "fp", "sfp" };

	pD = GetPNL_DYNAMIC(pL);

	switch (pD->type) {
		case PPP_DYNAM:
		case PP_DYNAM:
		case P_DYNAM:
		case MP_DYNAM:
		case MF_DYNAM:
		case F_DYNAM:
		case FF_DYNAM:
		case FFF_DYNAM:
		case SF_DYNAM:
			dynamicL = FIInsertDynamic(doc, pD->staff, doc->tailL, NILINK, pD->type);
			if (dynamicL==NILINK) goto broken;
			break;
		case PPPP_DYNAM:
		case FFFF_DYNAM:
		case PIUP_DYNAM:
		case MENOP_DYNAM:
		case MENOF_DYNAM:
		case PIUF_DYNAM:
		case FZ_DYNAM:
		case SFZ_DYNAM:
		case RF_DYNAM:
		case RFZ_DYNAM:
		case FP_DYNAM:
		case SFP_DYNAM:
			strcpy(str, dynamStrs[pD->type-1]);
			dynamicL = FIInsertGRString(doc, pD->staff, NOONE, doc->tailL, NILINK,
													FALSE, TSRegular2STYLE, NULL, str);
			if (dynamicL==NILINK) goto broken;
//****Now problem is that voice should NOT be NOONE!
			break;
		default:
			goto broken;
	}

	return TRUE;
broken:
	MayErrMsg("ConvertDynamic failed (pL=%d)", pL);
	return FALSE;
}


/* --------------------------------------------------------------- SetDefaultCoords -- */
/* Move every Graphic, Tempo mark and Dynamic to a reasonable default position. Since
the Notelist format does not encode graphic information for these items, we do something
crude and arbitrary here. It'd be good to make this less crude, e.g.,: If staff has any
lyrics, put dynamics above staff instead of below. */

static Boolean SetDefaultCoords(Document *doc)
{
	LINK		pL, firstStaffL, aStaffL, firstL;
	DDIST		staffHeight[MAXSTAVES+1], xd, yd,
				lastPgRelGrXD = 0, lastPgRelGrYD = 0;
	QDIST		height;
	char		staffLines[MAXSTAVES+1], staffn, str[256], glyph;
	short		fontSize[MAXSTAVES+1], wid, textStyle;
	PGRAPHIC	pGraphic;
	PASTAFF		aStaff;
	PADYNAMIC	aDynamic;

	/* First get staff height, number of lines and font size for each staff.
		(GetAllContexts is overkill for our purposes, so we roll our own here.)
	*/
	firstStaffL = LSSearch(doc->headL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
	if (!firstStaffL) goto broken;
	aStaffL = FirstSubLINK(firstStaffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		staffHeight[aStaff->staffn] = aStaff->staffHeight;
		staffLines[aStaff->staffn] = aStaff->staffLines;
		fontSize[aStaff->staffn] = aStaff->fontSize;
	}
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case GRAPHICtype:
				staffn = GraphicSTAFF(pL);
				firstL = GraphicFIRSTOBJ(pL);
				if (PageTYPE(firstL)) {								/* crudely tile page-rel graphics */
					if (lastPgRelGrXD==0) xd = DFLT_PGRELGR_XD;
					else xd = lastPgRelGrXD + PGRELGR_XOFFSET;
					LinkXD(pL) = lastPgRelGrXD = xd;

					if (lastPgRelGrYD==0) yd = DFLT_PGRELGR_YD;
					else yd = lastPgRelGrYD + PGRELGR_LEADING;
					LinkYD(pL) = lastPgRelGrYD = yd;
				}
				else {
					pGraphic = GetPGRAPHIC(pL);
					textStyle = Header2UserFontNum(pGraphic->info);
					switch (textStyle) {
						case TSRegular2STYLE:						/* for dynamics faked with graphics (see ConvertDynamic) */
							height = DFLT_DYNAMIC_HEIGHT;
							CenterTextGraphic(doc, pL);
							break;
						case TSRegular4STYLE:
							height = DFLT_LYRIC_HEIGHT;
							CenterTextGraphic(doc, pL);				/* NB: This assumes lyrics are attached to notes. */
							break;
						case TSRegular1STYLE:
						case TSRegular3STYLE:						/* shouldn't find 3, 5 or thisItemOnly */
						case TSRegular5STYLE:
						case TSThisItemOnlySTYLE:
							height = DFLT_TEXT_HEIGHT;
							break;
						default:
							MayErrMsg("SetDefaultCoords: nonsense textStyle.");
							height = DFLT_TEXT_HEIGHT;
					}
					yd = qd2d(height, staffHeight[staffn], staffLines[staffn]);
					LinkYD(pL) = yd;
				}
				break;
			case TEMPOtype:
				staffn = TempoSTAFF(pL);
				yd = qd2d(DFLT_TEMPO_HEIGHT, staffHeight[staffn], staffLines[staffn]);
				LinkYD(pL) = yd;
				break;
			case DYNAMtype:
				/* NB: Dynamic coordinates are bizarre -- they use the object's 
					xd for horizontal and the subobject's yd for vertical. */
				aDynamic = GetPADYNAMIC(FirstSubLINK(pL));
				staffn = aDynamic->staffn;
				yd = qd2d(DFLT_DYNAMIC_HEIGHT, staffHeight[staffn], staffLines[staffn]);
				aDynamic->yd = yd;

				/* Move dynamic left a bit. */
				glyph = DynamicGlyph(pL);
				str[0] = 1; str[1] = glyph;
				/* This assumes that magnification is 100%. */
				wid = NStringWidth(doc, (unsigned char *)str, doc->musicFontNum, fontSize[staffn], 0);
				xd = -(pt2d(wid) / 2);
				LinkXD(pL) = xd;
				break;
		}
	
	return TRUE;
broken:
	MayErrMsg("Could not set default coordinates for text, dynamics and tempo marks.");
	return FALSE;
}


/* ------------------------------------------------------------- FixPlayDurs -- */
/* Set play duration of every note that now has a zero or negative play dur. to a
reasonable value: this is intended mostly to handle the "default duration" feature
of notelists. */

#define PDURPCT 95		/* percent of logical (full) duration to play for: should this be <config.legatoPct>? */

static void FixPlayDurs(Document *doc, LINK startL, LINK endL)
{
	LINK pL, aNoteL;  long useLDur;
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NotePLAYDUR(aNoteL)>0) continue;
				useLDur = CalcNoteLDur(doc, aNoteL, pL);
				NotePLAYDUR(aNoteL) = (PDURPCT*useLDur)/100L;
			}
		}
}


/* --------------------------------------------------------------------------------- */
/* Functions for creating and displaying the converted Notelist document. */

/* ----------------------------------------------------------------- SetupNLScore -- */

static Boolean SetupNLScore(Document *doc)
{
	short		part, staffn, nStaves, sharpsOrFlats, timeSigNum, timeSigDenom, type;
	LINK		firstClefL, partL, firstTSL, firstKeySigL;
	NLINK		clefL, ksL, tsL;
	PNL_CLEF	pClef;
	PNL_TIMESIG	pTS;
	DDIST		sysTop;

	InstallDoc(doc);
	doc->firstMNNumber = gFirstMNNumber;
	
	/* We now have a default score with one part of two staves. Set it up
		according to the Notelist file structure, and then remove the default part.
		NB: Calling AddPart with staffn>=63 (62??) will blow up; Addpart doesn't
		seem to handle this at all well.
		??Partial solution: delete default part after adding the first part in the loop.
	*/
	for (part=1, staffn=1; part<=MAXSTAVES; part++, staffn+=nStaves) {
		nStaves = gPartStaves[part];
		if (nStaves==0) break;
		if (part==1) {
			partL = AddPart(doc, 2+(staffn-1), nStaves, SHOW_ALL_LINES);
			if (partL==NILINK) return FALSE;
			InitPart(partL, 2+staffn, 2+staffn+(nStaves-1));
			/* Remove default 2-staff part. Must do this before changing attributes below... */
			DeletePart(doc, 1, 2);
		}
		else {
			partL = AddPart(doc, staffn-1, nStaves, SHOW_ALL_LINES);
			if (partL==NILINK) return FALSE;
			InitPart(partL, staffn, staffn+(nStaves-1));
		}
	}

	/* Set up remaining parts with Notelist attributes, including initial clefs & keysigs. */
		
	firstClefL = LSSearch(doc->headL, CLEFtype, 1, GO_RIGHT, FALSE);
	if (firstClefL==NILINK) {
		MayErrMsg("SetupNLScore: can't find first clef!");
		return FALSE;
	}
	firstKeySigL = LSSearch(doc->headL, KEYSIGtype, 1, GO_RIGHT, FALSE);
	if (firstKeySigL==NILINK) {
		MayErrMsg("SetupNLScore: can't find first keysig!");
		return FALSE;
	}

	for (staffn = 1; staffn<=gNumNLStaves; staffn++) {
	
		/* If we find a clef for this staff in the Notelist, use it. Otherwise
			leave the the default clef alone.
		*/
		clefL = NLSearch(1, CLEF_TYPE, ANYONE, staffn, ANYONE, GO_RIGHT);
		if (clefL) {
			pClef = GetPNL_CLEF(clefL);
			ReplaceClef(doc, firstClefL, staffn, pClef->type);
		}

		/* If we find a keysig for this staff in the Notelist, use it. */
		ksL = NLSearch(1, KEYSIG_TYPE, ANYONE, staffn, ANYONE, GO_RIGHT);
		if (ksL) {
			sharpsOrFlats = NLtoNightKeysig(ksL);
			ReplaceKeySig(doc, firstKeySigL, staffn, sharpsOrFlats);
	
			/* Fix horizontal space taken by key sigs. */
			FixInitialKSxds(doc, firstKeySigL, doc->tailL, staffn);
		}
	}
	DeselectNode(firstClefL);						/* or else initial clefs will remain hilited & selected */

	/* Set up initial timesig, for now just using the top staff's timesig for all staves.
		If we can't find a timesig record for that staff in the Notelist, use config defaults.
	*/
	tsL = NLSearch(1, TIMESIG_TYPE, ANYONE, ANYONE, ANYONE, GO_RIGHT);
	if (tsL) {
		pTS = GetPNL_TIMESIG(tsL);
		timeSigNum = pTS->num;
		timeSigDenom = pTS->denom;
		type = pTS->appear;
	}
	else {
		timeSigNum = config.defaultTSNum;
		timeSigDenom = config.defaultTSDenom;
		type = N_OVER_D;
	}

	firstTSL = LSSearch(doc->headL, TIMESIGtype, 1, GO_RIGHT, FALSE);
	if (firstTSL==NILINK) {
		MayErrMsg("SetupNLScore: can't find first time signature!");
		return FALSE;
	}
	ReplaceTimeSig(doc, firstTSL, ANYONE/*staffn*/, type, timeSigNum, timeSigDenom);

	FixMeasRectYs(doc, NILINK, TRUE, TRUE, FALSE);				/* Fix measure & system tops & bottoms */
	Score2MasterPage(doc);

	sysTop = SYS_TOP(doc)+pt2d(config.titleMargin);
	doc->yBetweenSys = 0;

	/* Make sure the staves of a large system don't go off the bottom of the page. */
	
	//(void)FitStavesOnPaper(doc);	??I have no idea why this is commented out :-( . --DAB, March 2017

	return TRUE;
}


/* ------------------------------------------------------------------ CreateNLDoc -- */

#define NL_RASTRAL 5	/* Default staff rastral size */

static Document *CreateNLDoc(unsigned char *fileName)
{
	Document	*newDoc = NULL;
	WindowPtr	w;
	short		rastral;
	long		fileVersion;

	newDoc = FirstFreeDocument();
	if (newDoc==NULL) {
		TooManyDocs(); return NULL;
	}
	
	w = GetNewWindow(docWindowID, NULL, BottomPalette);
	if (!w) return NULL;
	
	newDoc->theWindow = w;
	SetDocumentKind(w);
	//((WindowPeek)w)->spareFlag = TRUE;
	ChangeWindowAttributes(w, kWindowFullZoomAttribute, kWindowNoAttributes);
	
	newDoc->inUse = TRUE;
	Pstrcpy(newDoc->name, fileName);
	newDoc->vrefnum = 0;
	
	rastral = NL_RASTRAL;

	if (!BuildNLDoc(newDoc, &fileVersion, 0/*pageWidth*/, 0/*pageHt*/, 2/*gNumNLStaves*/, rastral)) {
		DoCloseDocument(newDoc);
		return NULL;
	}

	return newDoc;
}


/* ------------------------------------------------------------------- BuildNLDoc -- */
/* Initialise a new Document, untitled and empty. Make the Document the current
grafPort and install it, including magnification. Return TRUE normally,
FALSE in case of error. */

#define COMMENT_NOTELIST "(origin: notelist file)"	/* ??Should be in resource for I18N */

static Boolean BuildNLDoc(
					Document *doc,
					long *fileVersion,
					short /*pageWidth*/,
					short /*pageHt*/,
					short /*nStaves*/,
					short rastral)
{
	WindowPtr	w = doc->theWindow;
	Rect		r;

	SetPort(GetWindowPort(w));
	
	/* Set the initial paper size, margins, etc. */
	
	doc->paperRect.top = doc->paperRect.left = 0;
	//doc->paperRect.bottom = 11*72;
	//doc->paperRect.right = 8.5*72;
	doc->paperRect.bottom = config.paperRect.bottom;
	doc->paperRect.right = config.paperRect.right;
LogPrintf(LOG_DEBUG, "paperRect.bottom=%d paperRect.right=%d\n", doc->paperRect.bottom,
		  doc->paperRect.right);
	
	doc->pageType = 2;		//??Is this even used?
	doc->measSystem = 0;

	doc->origPaperRect = doc->paperRect;

	doc->marginRect.top = doc->paperRect.top+config.pageMarg.top;
	doc->marginRect.left = doc->paperRect.left+config.pageMarg.left;
	doc->marginRect.bottom = doc->paperRect.bottom-config.pageMarg.bottom;
	doc->marginRect.right = doc->paperRect.right-config.pageMarg.right;

	InitDocFields(doc);
	doc->srastral = rastral;								/* Override staff size, from parameter */

	FillSpaceMap(doc, 0);

	/* Add the standard scroll bar controls to Document's window. The scroll
	 *	bars are created with a maximum value of 0 here, but this will have no
	 *	effect if we call RecomputeView, since it resets the max.
	 */
	GetWindowPortBounds(w,&r);
	r.left = r.right - (SCROLLBAR_WIDTH+1);
	r.bottom -= SCROLLBAR_WIDTH;
	r.top--;
	
	doc->vScroll = NewControl(w, &r, "\p", TRUE, doc->origin.h, doc->origin.h, 0, scrollBarProc, 0L);

	GetWindowPortBounds(w,&r);
	r.top = r.bottom - (SCROLLBAR_WIDTH+1);
	r.right -= SCROLLBAR_WIDTH;
	r.left += MESSAGEBOX_WIDTH;
	
	doc->hScroll = NewControl(w, &r, "\p", TRUE, doc->origin.v, doc->origin.v, 0, scrollBarProc, 0L);

	if (!InitAllHeaps(doc)) { NoMoreMemory(); return FALSE; }
	InstallDoc(doc);
	BuildEmptyList(doc, &doc->headL, &doc->tailL);
	
	doc->selStartL = doc->selEndL = doc->tailL;				/* Empty selection  */
	
	/* Set part name showing and corresponding system indents. We'd like to use
	 * PartNameMargin() to get the appropriate indents, but not enough of the data
	 * structure is set up, so do something cruder.
	 */
	doc->firstNames = NONAMES/*FULLNAMES*/;								/* 1st system: full part names */
	doc->firstIndent = qd2d(config.indent1st, drSize[doc->srastral], STFLINES);
	doc->otherNames = NONAMES;									/* Other systems: no part names */
	doc->otherIndent = 0;
	
	*fileVersion = THIS_VERSION;
	NewDocScore(doc);												/* Set up initial staves, clefs, etc. */
	doc->firstSheet = 0;
	doc->currentSheet = 0;
	doc->origin = doc->sheetOrigin;							/* Ignore position recorded in file */

	strcpy((char *)doc->comment, COMMENT_NOTELIST);

	if (!InitDocUndo(doc))
		return FALSE;
	
	doc->yBetweenSysMP = doc->yBetweenSys = 0;
	SetOrigin(doc->origin.h, doc->origin.v);
	GetAllSheets(doc);

	/* Finally, set empty selection just after the first Measure and put caret there.
		N.B. This will not necessarily be on the screen! We should eventually make
		the initial selection agree with the doc's scrolled position. */

	SetDefaultSelection(doc);
	doc->selStaff = 1;
	
	MEAdjustCaret(doc, FALSE);
	
	return TRUE;
}


/* ----------------------------------------------------------------- DisplayNLDoc -- */
/* Set up and display the newly created Notelist document. */

static void DisplayNLDoc(Document *newDoc)
{
	short				palWidth, palHeight;
	Rect				box,bounds;
	WindowPtr		w; 

	/* Replace the Master Page: delete the old Master Page data structure
		here and replace it with a new structure to reflect the part-staff
		structure of the scanned score. */

	// Score2MasterPage(newDoc); ??Why is this commented out?!
	newDoc->docNew = newDoc->changed = TRUE;		/* Has to be set after BuildDocument */
	newDoc->readOnly = FALSE;
	
	/* Place new document window in a non-conflicting position */

	w = newDoc->theWindow;
	WindowPtr palPtr = (*paletteGlobals[TOOL_PALETTE])->paletteWindow;
	GetWindowPortBounds(palPtr,&box);
	palWidth = box.right - box.left;
	palHeight = box.bottom - box.top;
	GetGlobalPort(w,&box);					/* set bottom of window near screen bottom */
	bounds = GetQDScreenBitsBounds();
	if (box.left < bounds.left+4)
		box.left = bounds.left+4;
	if (palWidth < palHeight)
		box.left += palWidth;

	MoveWindow(newDoc->theWindow, box.left, box.top, FALSE);
	AdjustWinPosition(w);
	GetGlobalPort(w, &box);

	bounds = GetQDScreenBitsBounds();
	box.bottom = bounds.bottom - 4;
	if (box.right > bounds.right-4)
		box.right = bounds.right - 4;
	SizeWindow(newDoc->theWindow, box.right-box.left, box.bottom-box.top, FALSE);

	PositionWindow(w, newDoc);
	SetOrigin(newDoc->origin.h, newDoc->origin.v);
//	GetAllSheets(newDoc);											??What was this for?
	RecomputeView(newDoc);
	SetControlValue(newDoc->hScroll, newDoc->origin.h);
	SetControlValue(newDoc->vScroll, newDoc->origin.v);
	SetWTitle(w, newDoc->name);
	
	InstallMagnify(newDoc);
	ShowDocument(newDoc);
}


/* --------------------------------------------------------------------------------- */
/* Notelist utility functions */

/* -------------------------------------------------------------- NLtoNightKeysig -- */

static short NLtoNightKeysig(NLINK keysigL)
{
	PNL_KEYSIG	pKS;
	short			sharpsOrFlats;

	pKS = GetPNL_KEYSIG(keysigL);
	sharpsOrFlats = pKS->numAcc;
	if (!pKS->sharp)
		sharpsOrFlats = -sharpsOrFlats;
	return sharpsOrFlats;
}


/* ---------------------------------------------------------------- IsFirstKeysig -- */
/* Return TRUE if <ksL> is one of the records comprising the first key signature
in the Notelist. (There will be one record for each staff.) */

static Boolean IsFirstKeysig(NLINK ksL)
{
	NLINK		pL;
	Boolean	ksRun = FALSE;
	
	for (pL = 1; pL<=gNumNLItems; pL++) {
		if (GetNL_TYPE(pL)==KEYSIG_TYPE) {
			if (pL==ksL) return TRUE;
			ksRun = TRUE;
		}
		else if (ksRun) break;					/* already searched first key sig. run */
	}
	return FALSE;
}


/* --------------------------------------------------------------- IsFirstTimesig -- */
/* Return TRUE if <tsL> is one of the records comprising the first time signature
in the Notelist. (There will be one record for each staff.) */

static Boolean IsFirstTimesig(NLINK tsL)
{
	NLINK		pL;
	Boolean	tsRun = FALSE;
	
	for (pL = 1; pL<=gNumNLItems; pL++) {
		if (GetNL_TYPE(pL)==TIMESIG_TYPE) {
			if (pL==tsL) return TRUE;
			tsRun = TRUE;
		}
		else if (tsRun) break;					/* already searched first time sig. run */
	}
	return FALSE;
}


/* ----------------------------------------------------------------- DynamicGlyph -- */

char DynamicGlyph(LINK dynamicL)
{
	char	glyph;
	
	switch (DynamType(dynamicL)) {
		case PPP_DYNAM:
			glyph = MCH_ppp;	break;
		case PP_DYNAM:
			glyph = MCH_pp;	break;
		case P_DYNAM:
			glyph = MCH_p;	break;
		case MP_DYNAM:
			glyph = MCH_mp;	break;
		case MF_DYNAM:
			glyph = MCH_mf;	break;
		case F_DYNAM:
			glyph = MCH_f;	break;
		case FF_DYNAM:
			glyph = MCH_ff;	break;
		case FFF_DYNAM:
			glyph = MCH_fff;	break;
		case SF_DYNAM:
			glyph = MCH_sf;	break;
		default:
			MayErrMsg("DynamicGlyph: nonsense dynamic.");
			glyph = MCH_xShapeHead;
	}
	return glyph;
}



/* --------------------------------------------------------------------------------- */
/* Functions that call Macintosh Toolbox routines. */

/* ----------------------------------------------------- BuildConvertedNLFileName -- */
/* Build a filename for the converted score, removing ".nl" suffix, if any, and appending
" (converted)" to the input filename. Note that we use this _only_ if the header of
the Notelist file doesn't contain a score name. */

static void BuildConvertedNLFileName(Str255 fn, Str255 newfn)
{
	unsigned char	str[64];
	unsigned char	*p;
	long			len;

	Pstrcpy(newfn, fn);

	/* Remove ".nl" suffix, if any. */
	len = *(unsigned char *)newfn;
	p = &newfn[len-2];
	if (!strncmp((char *)p, ".nl", (size_t) 3))
		newfn[0] = len-3;

	/* Append " (converted)" to its name. */
	GetIndString(str, MiscStringsID, 5);
	PStrCat(newfn, str);
}
