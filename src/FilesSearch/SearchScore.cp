/* SearchScore.c for the OMRAS version of Nightingale - search a score or group of
scores for a melodic pattern given by another score - new for OMRAS. */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define SEARCH_SCORE_MAIN			/* So we define variables for search modules here */
#include "SearchScore.h"
#include "SearchScorePrivate.h"


#define SEARCH_VOICE 1		/* Voice number to use in the Search Score */
#define NoDEBUG_LOWLEVEL
#define NoDEBUG_MIDLEVEL


/* -------------------------------------------------------------------------------------- */
/* Helper functions for converting the query from object list to our internal form. */

/* If there's a rest in the given voice and range, return its LINK, else return NILINK.
FIXME: This ignores <voice>!! */

static LINK N_VFindRest(INT16 voice, LINK startL, LINK endL);
static LINK N_VFindRest(INT16 voice, LINK startL, LINK endL)
{
	LINK pL, aNoteL;
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (!SyncTYPE(pL)) continue;
		for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			if (NoteREST(aNoteL)) return aNoteL;
		}
	}
	
	return NILINK;
}


/* Return (as function value) the number of chords in the search pattern document; a
rest counts as a chord. Also return (in the parameter) whether the pattern contains any
rests. */

INT16 N_SearchPatternLen(Boolean *pHaveRest)
{
	Document *saveDoc;
	INT16 targetLen;
	
	/* Install <searchPatDoc> to see what's in it, then re-install the previously
	   installed doc. */
	   
	saveDoc = currentDoc;
	InstallDoc(searchPatDoc);
	targetLen = VCountNotes(SEARCH_VOICE, searchPatDoc->headL, searchPatDoc->tailL, True);
	*pHaveRest = (N_VFindRest(SEARCH_VOICE, searchPatDoc->headL, searchPatDoc->tailL)!=NILINK);
	InstallDoc(saveDoc);
	
	return targetLen;
}


/* ------------------------------------------------------------- Simple/TiedLDur, etc. -- */
/* Return total logical duration of <aNoteL> and all following notes tied to
<aNoteL>. If <selectedOnly>, only includes tied notes until the first unselected
one. We use the note's notated duration for all the notes. */

static INT16 N_TiedLDur(LINK, LINK);
static INT16 N_TiedLDur(LINK syncL, LINK aNoteL)
{
	LINK		syncPrevL, aNotePrevL, continL, continNoteL;
	INT16		voice, dur, prevDur;
	
	voice = NoteVOICE(aNoteL);
	
	dur = 0;
	syncPrevL = syncL;
	aNotePrevL = aNoteL;
	while (NoteTIEDR(aNotePrevL)) {
		continL = LVSearch(RightLINK(syncPrevL),		/* Tied note should always exist */
						SYNCtype, voice, GO_RIGHT, False);
		continNoteL = NoteNum2Note(continL, voice, NoteNUM(aNotePrevL));
		if (continNoteL==NILINK) break;						/* Should never happen */

		/* We know this note will be played, so add in the previous note's notated dur. */
		prevDur = SyncAbsTime(continL)-SyncAbsTime(syncPrevL);
		dur += prevDur;

		syncPrevL = continL;
		aNotePrevL = continNoteL;
	}
	dur += SimpleLDur(aNotePrevL);						/* Bad if unknown duration or a multibar rest! */
	return dur;
}


/* If the note/rest is unknown duration or a multibar rest, return its <subType>
(duration code); else compute its "simple" logical duration, ignoring tuplet
membership and whole-measure rests. NB: this depends on the fact that <subType> for
unknown duration and multibar rest are always <=0, while SimpleLDur is always >0. */

static long N_SimpleLDurOrCode(LINK aNoteL);
static long N_SimpleLDurOrCode(LINK aNoteL)
{
	if (NoteType(aNoteL)==UNKNOWN_L_DUR || NoteType(aNoteL)<=WHOLEMR_L_DUR)
		return NoteType(aNoteL);
	else
		return SimpleLDur(aNoteL);
}


/* If the note/rest is unknown duration or a multibar rest, return its <subType>
(duration code); else compute the total "simple" logical duration of it and following
notes it's tied to, ignoring tuplet membership and whole-measure rests. NB: this
depends on the fact that <subType> for unknown duration and multibar rest are always
<=0, while SimpleLDur is always >0. */

static long N_TiedLDurOrCode(LINK syncL, LINK aNoteL);
static long N_TiedLDurOrCode(LINK syncL, LINK aNoteL)
{
	if (NoteType(aNoteL)==UNKNOWN_L_DUR || NoteType(aNoteL)<=WHOLEMR_L_DUR)
		return NoteType(aNoteL);
	else
		return N_TiedLDur(syncL, aNoteL);
}


/* ------------------------------------------------------------- N_SearchScore2Pattern -- */
/* Convert notes and, optionally, rests in the relevant voice of the Search Score into
a pattern. If there are any chords in the voice, use the top note of each. Return False
if there's a problem (pattern too long to handle), else True. */

#define N_TiedDur(doc, syncL, aNoteL, selectedOnly)	TiedDur(doc, syncL, aNoteL, selectedOnly)

Boolean N_SearchScore2Pattern(Boolean includeRests, SEARCHPAT *pSearchPat,
												Boolean matchTiedDur, Boolean *pHaveChord)
{
	INT16 index; LINK pL, aNoteL, dummyNoteL;
	INT16 prevNoteNum;
	Document *saveDoc;
	Boolean okay=True;
	
	saveDoc = currentDoc;
	InstallDoc(searchPatDoc);
	
	index = -1;
	prevNoteNum = -1;
	*pHaveChord = False;
	for (pL = searchPatDoc->headL; pL!=searchPatDoc->tailL; pL = RightLINK(pL)) {
		if (!SyncTYPE(pL)) continue;
		GetExtremeNotes(pL, SEARCH_VOICE, &dummyNoteL, &aNoteL);
		if (!includeRests && NoteREST(aNoteL)) continue;
		if (NoteVOICE(aNoteL)!=SEARCH_VOICE) continue;
		if (matchTiedDur && NoteTIEDL(aNoteL)) continue;
		
		/* We've eliminated all cases we're not interested in: add this note/rest if
		   we have room for it. */
		if (NoteINCHORD(aNoteL)) *pHaveChord = True;
		index++;
		if (index>=MAX_PATLEN) {
			pSearchPat->patLen = MAX_PATLEN;
			okay = False;
			goto Cleanup;
		}

		pSearchPat->noteRest[index] = NoteREST(aNoteL);
		pSearchPat->tiedL[index] = NoteTIEDL(aNoteL);
		pSearchPat->noteNum[index] = NoteNUM(aNoteL);
		
		/* Set <prevNoteNum> from the previous note, if any; if none, it's still -1. NB if
		   the previous "note" is a rest, it doesn't affect <prevNoteNum>. */
		   
		pSearchPat->prevNoteNum[index] = prevNoteNum;
		if (!NoteREST(aNoteL)) prevNoteNum = NoteNUM(aNoteL);

		if (matchTiedDur) {
			pSearchPat->lDur[index] = N_TiedLDurOrCode(pL, aNoteL);
			pSearchPat->durPlay[index] = N_TiedDur(searchPatDoc, pL, aNoteL, False);
		}
		else {
			pSearchPat->lDur[index] = N_SimpleLDurOrCode(aNoteL);
			pSearchPat->durPlay[index] = NotePLAYDUR(aNoteL);
		}
	}
	
	pSearchPat->patLen = index+1;

Cleanup:
	InstallDoc(saveDoc);
	return okay;
}


/* ------------------------------------------------------ DB_Simple/TiedLDur, etc. -- */
/* Return total logical duration of <aNoteL> and all following notes tied to
<aNoteL>. If <selectedOnly>, only includes tied notes until the first unselected
one. We use the note's notated duration for all the notes. */

static INT16 DB_TiedLDur(DB_LINK, DB_LINK);
static INT16 DB_TiedLDur(DB_LINK syncL, DB_LINK aNoteL)
{
	DB_LINK		syncPrevL, aNotePrevL, continL, continNoteL;
	INT16			voice, dur, prevDur;
	
	voice = DB_NoteVOICE(syncL, aNoteL);
	
	dur = 0;
	syncPrevL = syncL;
	aNotePrevL = aNoteL;
	while (DB_NoteTIEDR(aNotePrevL)) {
		continL = DB_LVSSearch(DB_RightLINK(syncPrevL),		/* Tied note should always exist */
						voice, GO_RIGHT);
		continNoteL = DB_NoteNum2Note(continL, voice, DB_NoteNUM(aNotePrevL));
		if (continNoteL==DB_NILINK) break;						/* Should never happen */

		/* We know this note will be played, so add in the previous note's notated dur. */
		
		prevDur = DB_SyncAbsTime(continL)-DB_SyncAbsTime(syncPrevL);
		dur += prevDur;

		syncPrevL = continL;
		aNotePrevL = continNoteL;
	}
	dur += DB_SimpleLDur(aNotePrevL);						/* Bad if unknown duration or a multibar rest! */
	return dur;
}


/* If the note/rest is unknown duration or a multibar rest, return its <subType>
(duration code); else compute the total "simple" logical duration of it and following
notes it's tied to, ignoring tuplet membership and whole-measure rests. NB: this
depends on the fact that <subType> for unknown duration and multibar rest are always
<=0, while SimpleLDur is always >0. */

long DB_TiedLDurOrCode(DB_LINK syncL, DB_LINK aNoteL);
long DB_TiedLDurOrCode(DB_LINK syncL, DB_LINK aNoteL)
{
	if (DB_NoteType(aNoteL)==UNKNOWN_L_DUR || DB_NoteType(aNoteL)<=WHOLEMR_L_DUR)
		return DB_NoteType(aNoteL);
	else
		return DB_TiedLDur(syncL, aNoteL);
}


/* ====================================================================================== */
/* Functions from here on assume the query is already in our pattern form; they deal
with "object lists" for scores being searched in an implementation-independent way
(by using the DB_ functions and macros to access the "object list"). */

/* NUMSIGN(n) = 1, 0, -1 if n is positive, zero, or negative, respectively. */

#define NUMSIGN(n)	(  (n)==0? 0 : ( (n)>0? 1 : -1 ) )

/* Does the given note or rest match the SEARCHPAT item? Return True iff it does.

Note: as of this writing (v. 99b4, late Jan. 2000), the most stack space the Search
commands ever consume is in this function when it's called from DoIRSearchFiles(). */

static Boolean NRMatch(Boolean usePitch, Boolean useDuration, INT16 pitchSearchType, INT16 durSearchType,
						INT16 maxTranspose, INT16 pitchTolerance, Boolean pitchKeepContour,
						Boolean matchTiedDur, DB_LINK syncL, DB_LINK theNoteL, DB_LINK prevSyncL,
						DB_LINK prevNoteL, INT16 prevNoteNum, SEARCHPAT searchPat, INT16 targetPos, ERRINFO *pErrorInfo);
static Boolean NRMatch(
						Boolean usePitch, Boolean useDuration,
						INT16 pitchSearchType,
						INT16 durSearchType,
						INT16 maxTranspose,
						INT16 pitchTolerance,			/* Accept matches this many semitones off */
						Boolean pitchKeepContour,		/* ...but this means contour must be preserved (TYPE_PITCHMIDI_REL only) */
						Boolean matchTiedDur,
						DB_LINK syncL,					/* Candidate match */
						DB_LINK theNoteL,				/* Candidate match (note or rest) */
						DB_LINK prevSyncL,				/* Most-recently matched: used only for relative pitch */
						DB_LINK prevNoteL,				/* Most-recently matched: used only for relative pitch */
						INT16 prevNoteNum,				/* MIDI note number of previous note (not rest) in candidate */
						SEARCHPAT searchPat,
						INT16 targetPos,				/* Position in pattern to match */
						ERRINFO *pErrorInfo

					)
{
	Boolean matchHere;
	INT16 intervalWanted, intervalHere, nnDiff;
	INT16 pitchError, durationError;
	
	/* Notes match only notes; rests--even if they're included--match only rests.
	   Also, note continuations (i.e., tied to the left) match only continuations. */
	   
	if (DB_NoteREST(theNoteL)!=searchPat.noteRest[targetPos]) return False;
	if (DB_NoteTIEDL(theNoteL)!=searchPat.tiedL[targetPos]) return False;

	pitchError = 0;
	durationError = 0;

	if (usePitch) {
		/* Compute note-number difference between this note and the corresponding note
		   in the pattern and decide if there's a match. If "note" is a rest, the
		   difference is always zero and there's always a match. */
		 
		if (DB_NoteREST(theNoteL)) {
			matchHere = True;
			goto MatchDur;
		}
		nnDiff = DB_NoteNUM(theNoteL)-searchPat.noteNum[targetPos];

		switch (pitchSearchType) {
			case TYPE_PITCHMIDI_ABS:
				matchHere = (abs(nnDiff)<=pitchTolerance);
				pitchError = abs(nnDiff);
				break;
			case TYPE_PITCHMIDI_ABS_TROCT:
				matchHere = (abs(nnDiff%12)<=pitchTolerance);
				pitchError = abs(nnDiff%12);
				break;
			case TYPE_PITCHMIDI_REL:
				/* Really an interval pattern. For the 1st note, always succeed; for each
				   other note, check the interval from the previous note (not rest!)
				   against the desired interval. */
				   
				if (targetPos<=0) {
					matchHere = True;
					pitchError = 0;
					break;
				}
				intervalWanted = searchPat.noteNum[targetPos]-searchPat.prevNoteNum[targetPos];
				intervalHere = DB_NoteNUM(theNoteL)-prevNoteNum;
if (ControlKeyDown()) DebugPrintf("NRMatch: targetPos=%d, intervalWanted=%d, intervalHere=%d\n",
targetPos, intervalWanted, intervalHere);
				if (abs(nnDiff)>maxTranspose) { matchHere = False;  break; }
				
				/* If we're going up instead of down, or down instead of staying the same,
				   etc., and <pitchKeepContour>, then fail regardless of <pitchTolerance>. */
				   
				if (pitchKeepContour && NUMSIGN(intervalHere)!=NUMSIGN(intervalWanted))
					{ matchHere = False;  break; }
				else
					matchHere = (abs(intervalHere-intervalWanted)<=pitchTolerance);
				
				pitchError = abs(intervalHere-intervalWanted);
				break;
			default:
				;
		}
		
		if (!matchHere) return False;
	}

MatchDur:
if (ControlKeyDown()) DebugPrintf("NRMatch: NoteREST=%d; prevNoteNum=%d nnDiff=%d pitchError=%d\n",
DB_NoteREST(theNoteL), prevNoteNum, nnDiff, pitchError);
	if (useDuration) {
		INT16 durHere, durPrev, durTargetHere, durTargetPrev;
		FASTFLOAT ratioWanted, ratioHere;
		
		switch (durSearchType) {
			case TYPE_DURATION_ABS:
				if (matchTiedDur) durHere = DB_TiedLDurOrCode(syncL, theNoteL);
				else durHere = DB_SimpleLDurOrCode(theNoteL);
				matchHere = (durHere==searchPat.lDur[targetPos]);
				durationError = 0;
				break;

			case TYPE_DURATION_REL:
			case TYPE_DURATION_CONTOUR:
				/* Really a rhythm-ratio pattern or rhythm-relation. For the 1st note,
				   always succeed; for each other note, check the ratio or greater/equal/same
				   relationship from the previous note against the desired ratio. */
				   
				if (targetPos<=0) {
					matchHere = True;
					durationError = 0;
					break;
				}
				if (matchTiedDur) {
					durHere = DB_TiedLDurOrCode(syncL, theNoteL);
					durPrev = DB_TiedLDurOrCode(prevSyncL, prevNoteL);
				}
				else {
					durHere = DB_SimpleLDurOrCode(theNoteL);
					durPrev = DB_SimpleLDurOrCode(prevNoteL);
				}
				durTargetHere = searchPat.lDur[targetPos];
				durTargetPrev = searchPat.lDur[targetPos-1];
				ratioWanted = (FASTFLOAT)durTargetHere/(FASTFLOAT)durTargetPrev;
				ratioHere = (FASTFLOAT)durHere/(FASTFLOAT)durPrev;
if (ControlKeyDown()) DebugPrintf("NRMatch: durHere=%d durPrev=%d targetPos=%d durTargetHere=%d durTargetPrev=%d\n",
durHere, durPrev, targetPos, durTargetHere, durTargetPrev);

				if (durSearchType==TYPE_DURATION_CONTOUR) {
					matchHere = (  (ratioWanted>1.0 && ratioHere>1.0)
					            || (ratioWanted==1.0 && ratioHere==1.0)
					            || (ratioWanted<1.0 && ratioHere<1.0) );
				}
				else {
					matchHere = (ABS(ratioWanted-ratioHere)<.001);
				}

				/* <durationError> is 1 only if contour matching and we don't have the
				   expected duration, otherwise it's 0. */
				   
				durationError = 0;
				if (durSearchType==TYPE_DURATION_CONTOUR
				&&  (ABS(ratioWanted-ratioHere)>=.001))
					durationError = 1;
				
				break;

			default:
				;
		}
		
		if (!matchHere) return False;
	}
	
	/* It matched in all desired respects. */
	
	pErrorInfo->pitchErr = pitchError;
	pErrorInfo->durationErr = durationError;
	return True;
}


/* --------------------------------------------------------------------- WarnHaveChord -- */

#define HAVECHORD_DLOG 816

enum {
	BUT1_OK=1,
	DONT_WARN_AGAIN_DI=4
};

/* Give user an "alert" warning that the Search Pattern contains one or more chords.
We actually use a dialog instead of an alert to facilitate handling a "don't warn
again" button. */

static ModalFilterUPP	filterUPP;

void WarnHaveChord(void)
{
	static Boolean warnAgain=True;
	DialogPtr dlog;
	Boolean keepGoing=True;
	short itemHit;
	GrafPtr savePort;

	if (!warnAgain) return;

	/* Build dialog window and install its item values */
	
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(PASTE_DELREDACCS_DLOG);
		return;
	}
	GetPort(&savePort);
	dlog = GetNewDialog(HAVECHORD_DLOG, NULL, BRING_TO_FRONT);
	if (dlog==NULL) {
		MissingDialog(HAVECHORD_DLOG);
		return;
	}
	SetPort(GetDialogWindowPort(dlog));

	PlaceWindow(GetDialogWindow(dlog),NULL,0,70);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP, &itemHit);
		switch (itemHit) {
			case BUT1_OK:
				keepGoing = False;
				warnAgain = !GetDlgChkRadio(dlog, DONT_WARN_AGAIN_DI);
				break;
			case DONT_WARN_AGAIN_DI:
				PutDlgChkRadio(dlog, DONT_WARN_AGAIN_DI, 
									!GetDlgChkRadio(dlog, DONT_WARN_AGAIN_DI));
				break;
			default:
				;
		}
	}
	
	DisposeDialog(dlog);
	SetPort(savePort);
}


/* -------------------------------------------------------------------------------------- */
/* These functions implement a crude "progress-bar" system for lengthy operations. */

#define NUM_UPDATES 50L		/* Number of updates to show */

static long nPRSoFar, nPRUpdatesSoFar, updatePRSkip;

static Boolean ShowingProgressReport(void);
static void InitProgressReport(long nPRTotal);
static void UpdateProgressReport(void);
static void EndProgressReport(void);


/* Control whether the progress report is actually shown. NB: showing it slows things
down by a surprising amount, about 10%. */

static Boolean ShowingProgressReport(void)
{
	return CapsLockKeyDown();
}

static void InitProgressReport(long nPRTotal)
{
	updatePRSkip = nPRTotal/NUM_UPDATES;

	/* If the update interval is too short, progress reporting is unnecessary and may
	   well add significant overhead to a quick operation. Disable it by setting the
	   update interval to a huge value. */
	   
	if (updatePRSkip<100L) updatePRSkip = 9999999L;
	nPRSoFar = nPRUpdatesSoFar = 0;
}

static void UpdateProgressReport(void)
{
	Boolean showProgress;
	
	nPRSoFar++;
	showProgress = ((nPRSoFar/updatePRSkip)*updatePRSkip==nPRSoFar);
	if (showProgress) {
		nPRUpdatesSoFar++;
		if (!ShowingProgressReport()) return;
		
		if (nPRUpdatesSoFar==1) DebugPrintf("{");
		DebugPrintf("%c", (nPRUpdatesSoFar/10)*10==nPRUpdatesSoFar? '*' : '.');
	}
}

static void EndProgressReport(void)
{
	if (ShowingProgressReport()) DebugPrintf("}");
}


/* ---------------------------------------------------------- Core searching functions -- */

static INT16 SyncMatch(SEARCHPARAMS sParm, SEARCHPAT searchPat, INT16 voice, DB_LINK syncL, DB_LINK
						startNoteL, DB_LINK matchedObjA[], DB_LINK matchedSubobjA[],
						DB_LINK *pMayMatchStartL, DB_LINK *pPrevSyncL, DB_LINK *pPrevNoteL,
						INT16 *pPrevNoteNum, INT16 *pTargetPos,
						ERRINFO *pErrInfo);
static INT16 RestartSearchTargetPos(SEARCHPARAMS sParm, SEARCHPAT searchPat, DB_LINK failedMatchStartL,
						DB_LINK failedMatchStartNoteL, DB_LINK failL, INT16 voice,
						DB_LINK *pMayMatchStartL, DB_LINK *pNewPrevSyncL, DB_LINK *pNewPrevNoteL,
						INT16 *pNewPrevNoteNum, DB_LINK newObjA[], DB_LINK newSubobjA[],
						ERRINFO *pTotalErrorInfo);

/* Status values for a voice in a Sync */

enum {
	VOICE_NOTPRESENT,
	VOICE_PRESENT,
	VOICE_MATCHED
};


/* Try to find a match in the given Sync in the given voice for the next note in the search
pattern. Return the VOICE_* status. */

static INT16 SyncMatch(
			SEARCHPARAMS sParm,
			SEARCHPAT searchPat,
			INT16 voice,
			DB_LINK syncL,
			DB_LINK startNoteL,						/* First note of Sync to consider (may be in any voice) */
			DB_LINK matchedObjA[],
			DB_LINK matchedSubobjA[],
			DB_LINK *pMayMatchStartL,				/* Input AND output! For new candidate match, starting DB_LINK */
			DB_LINK *pPrevSyncL,						/* Input AND output! */
			DB_LINK *pPrevNoteL,						/* Input AND output! */
			INT16 *pPrevNoteNum,						/* Input AND output! */
			INT16 *pTargetPos,						/* Pos. in pattern to match. Input AND output! */
			ERRINFO *pErrInfo							/* Output */
		)
{
	INT16 targetPos; DB_LINK aNoteL, mayMatchStartL;
	DB_LINK lowNoteL, hiNoteL;
	INT16 voiceStatus;
	Boolean matchHere;
	
	targetPos = *pTargetPos;
	mayMatchStartL = *pMayMatchStartL;
	voiceStatus = VOICE_NOTPRESENT;
	
	aNoteL = startNoteL;
	for ( ; aNoteL; aNoteL = DB_NextNOTEL(syncL, aNoteL)) {
#ifdef DEBUG_LOWLEVEL
		DebugPrintf("  Trying  aNoteL=%d Rest()=%d NoteNUM()=%d *pPrevNoteNum=%d\n",
		aNoteL, DB_NoteREST(aNoteL), NoteNUM(aNoteL), *pPrevNoteNum);
#endif
		if (!sParm.includeRests && DB_NoteREST(aNoteL)) continue;
		/*
		 * If this note doesn't match the next item in the pattern for our voice,
		 * reset. Then, if it does match the next item of the pattern, if it's the
		 * first note/rest of the pattern, initialize; if it's the end of the
		 * pattern, we're done--though if this function is used as intended, we'll
		 * always reach <failL> and stop before then.
		 */
		if (DB_NoteVOICE(syncL, aNoteL)!=voice) continue;
		
		/*
		 * If we've already matched a note in this Sync and voice, skip remaining notes
		 * in the Sync and voice, so chords don't confuse us.
		 */
		if (voiceStatus==VOICE_MATCHED) continue;
	
		if (sParm.matchTiedDur && DB_NoteTIEDL(aNoteL)) continue;

		/* Check the note's position in its chord, if needed. If it passes that test,
		 * check everything else about it.
		 */
		matchHere = True;

		if (sParm.chordNotes==TYPE_CHORD_TOP) {
			DB_GetExtremeNotes(syncL, voice, &lowNoteL, &hiNoteL);
			if (aNoteL!=hiNoteL) matchHere = False;
		}
		else if (sParm.chordNotes==TYPE_CHORD_OUTER) {
			DB_GetExtremeNotes(syncL, voice, &lowNoteL, &hiNoteL);
			if (aNoteL!=lowNoteL && aNoteL!=hiNoteL) matchHere = False;
		}

#if 0000
DebugPrintf(".chordNotes=%d syncL=%ld voice=%d aNoteL=%ld lowNoteL=%ld hiNoteL=%ld targetPos=%d %s\n",
sParm.chordNotes, syncL, voice, aNoteL, lowNoteL, hiNoteL, targetPos, (matchHere? "Match" : " "));
#endif
		if (matchHere)
			matchHere = NRMatch(sParm.usePitch, sParm.useDuration, sParm.pitchSearchType, sParm.durSearchType,
										sParm.maxTranspose, sParm.pitchTolerance,
										sParm.pitchKeepContour, sParm.matchTiedDur,
										syncL, aNoteL, *pPrevSyncL, *pPrevNoteL,
										*pPrevNoteNum, searchPat, targetPos, pErrInfo);
	
		/* All tests are complete: handle the result. */
		if (!matchHere)
			voiceStatus = VOICE_PRESENT;
	
		if (matchHere) {
			matchedObjA[targetPos] = syncL;								/* Save matched-note info */
			matchedSubobjA[targetPos] = aNoteL;
			targetPos++;
			if (targetPos==1)
				mayMatchStartL = syncL;										/* 1st note: initialize */
			
			if (!DB_NoteREST(aNoteL))
				*pPrevNoteNum = NoteNUM(aNoteL);
#ifdef DEBUG_LOWLEVEL
			DebugPrintf("  Matched aNoteL=%d Rest()=%d NoteNUM()=%d *pPrevNoteNum=%d.\n",
				aNoteL, DB_NoteREST(aNoteL), NoteNUM(aNoteL), *pPrevNoteNum);
#endif
			*pPrevSyncL = syncL;
			*pPrevNoteL = aNoteL;
			
			/* Skip remaining notes in the Sync	in this voice, so chords don't confuse us. */
			voiceStatus = VOICE_MATCHED;
		}
	}
	
	*pTargetPos = targetPos;
	*pMayMatchStartL = mayMatchStartL;
	return voiceStatus;
}


/* Given a voice, plus a starting DB_LINK for a partial match and a DB_LINK at which the
match fails in that voice, return information necessary to look for the next full match
in that voice: a new candidate match's starting DB_LINK, plus (as function value) the
index of the last item matched as of the previous failure point: this must be >0, so that
the new candidate match must work past the point where the old one failed. If we don't
find another possible match, just return function value of 0.

Note that if the pattern repeats anything, it's still possible that a full match is
already in progress at the DB_LINK where the match attempt failed but starting further
along than where we started before. Also, if we're doing a relative-pitch match, and the
voice has a chord where we started, it's possible that a full match is already in progress
starting with a different note of that chord! So restarting from the correct place is not
simple. We handle both complications here. */

static INT16 RestartSearchTargetPos(
			SEARCHPARAMS sParm,
			SEARCHPAT searchPat,
			DB_LINK failedMatchStartL,			/* Sync where the partial match started */
			DB_LINK failedMatchStartNoteL,	/* Note where the partial match started: 1st in its chord! */
			DB_LINK failL,							/* A following Sync where the partial match failed */
			INT16 voice,
			DB_LINK *pMayMatchStartL,			/* For new candidate match, starting DB_LINK */
			DB_LINK *pNewPrevSyncL,				/* For new candidate match, most-recently-matched Sync DB_LINK */
			DB_LINK *pNewPrevNoteL,				/* For new candidate match, most-recently-matched note DB_LINK */
			INT16 *pNewPrevNoteNum,
			DB_LINK newObjA[],
			DB_LINK newSubobjA[],
			ERRINFO *pTotalErrorInfo
			)
{
	INT16 targetPos, startNoteCount, k, prevNoteNum;
	DB_LINK pL, aNoteL, mayMatchStartL, prevSyncL, prevNoteL;
	DB_LINK searchStartL, startSyncNoteL, startNoteL[MAXCHORD];
	INT16 voiceStatus;						/* Status of voice for the current Sync */
	ERRINFO oneErrorInfo, totalErrorInfo;
	
#ifdef DEBUG_MIDLEVEL
	DebugPrintf(" RestartSearchTargetPos: look for match. failedMatchStartL=%d voice=%d\n",
	failedMatchStartL, voice);
#endif
	if (failedMatchStartL==DB_NILINK
	||  !DB_IsAfter(failedMatchStartL, failL)) {
		/* Either there was no previous partial match or the range from start to fail
		 * LINKs is empty: we can't restart.
		 */
		*pMayMatchStartL = DB_NILINK;
		*pNewPrevSyncL = DB_NILINK;
		*pNewPrevNoteL = DB_NILINK;
		*pNewPrevNoteNum = -1;
		return 0;
	}
	
	prevSyncL = failedMatchStartL;
	prevNoteL = DB_FindFirstNote(failedMatchStartL, voice);
	prevNoteNum = -1;

	/* If we're doing relative pitch and the failed match started with a chord, we need to
	 * consider a match starting with any of the other notes in that chord. Notice that
	 * with absolute pitch or no pitch matching at all, if we don't find a match starting
	 * with one note of a chord, there can't be one starting with any other note, so this
	 * logic isn't necessary: in that case, just look for a match starting with the next
	 * Sync. On the other hand, as of this writing, with relative pitch, the first note
	 * _always_ matches! However, that might change, and in any case we need to try
	 * matching the rest of the pattern with each note in this chord as the first.
	 *
	 * ??WHAT IF WE'RE DOING RELATIVE DURATION??
	 */
	searchStartL = DB_RightLINK(failedMatchStartL);
	startNoteL[0] = DB_NILINK;
	startNoteCount = 1;
	if (sParm.usePitch && sParm.pitchSearchType==TYPE_PITCHMIDI_REL
		 && DB_VCountNotes(voice, failedMatchStartL, DB_RightLINK(failedMatchStartL), False) > 1) {
		startSyncNoteL =  DB_ChordNextNR(failedMatchStartL, failedMatchStartNoteL);
		if (startSyncNoteL!=DB_NILINK) {
			/* Put remaining notes in its chord into an array to use for following loop indices. */
			k = 0;
			for ( ; startSyncNoteL; startSyncNoteL = DB_ChordNextNR(failedMatchStartL, startSyncNoteL)) {
		 		startNoteL[k] = startSyncNoteL;
		 		k++;
		 	}
			startNoteCount = k;
		}
	}
	
	totalErrorInfo.pitchErr = 0;
	totalErrorInfo.durationErr = 0;

	/* Start looking. If we're doing relative pitch and the failed match started with a
	 * chord, this outer loop iterates thru all remaining notes in its chord. Otherwise,
	 * this outer loop goes thru only a single iteration.
	 */
	for (k = 0; k<startNoteCount; k++) {
		targetPos = 0;
		mayMatchStartL = DB_NILINK;
		if (startNoteL[k]!=DB_NILINK) {
			/* We're doing relative pitch: check next note of chord that failed the match before.
			 * Since this is now the start of a match, it should always succeed.
			 */
			voiceStatus = SyncMatch(sParm, searchPat, voice, failedMatchStartL, startNoteL[k],
								newObjA, newSubobjA, &mayMatchStartL, &prevSyncL, &prevNoteL,
								&prevNoteNum, &targetPos, &oneErrorInfo);
			if (voiceStatus==VOICE_MATCHED) {
				totalErrorInfo.pitchErr += oneErrorInfo.pitchErr;
				totalErrorInfo.durationErr += oneErrorInfo.durationErr;
			}

			if (targetPos>=searchPat.patLen) {
				MayErrMsg("RestartSearchTargetPos: matched entire pattern, at %ld.",
								(long)failedMatchStartL);									/* should never happen */
				goto Cleanup;
			}
			if (voiceStatus!=VOICE_MATCHED) continue;									/* should never happen */
		}
		
		/* To be useful, a new candidate match must match thru <failL>, not just up to it. */
	
		for (pL = searchStartL; pL && pL!=DB_RightLINK(failL); pL = DB_RightLINK(pL)) {
			if (!DB_SyncTYPE(pL)) continue;

			aNoteL = DB_FindFirstNote(pL, voice);
			voiceStatus = SyncMatch(sParm, searchPat, voice, pL, aNoteL, newObjA, newSubobjA,
											&mayMatchStartL, &prevSyncL, &prevNoteL,
											&prevNoteNum, &targetPos, &oneErrorInfo);
			if (voiceStatus==VOICE_MATCHED) {
				totalErrorInfo.pitchErr += oneErrorInfo.pitchErr;
				totalErrorInfo.durationErr += oneErrorInfo.durationErr;
			}
			if (targetPos>=searchPat.patLen)
				goto Cleanup;														/* Matched entire pattern: unusual but possible */
			if (voiceStatus==VOICE_PRESENT) {
				/* The voice had notes in the Sync but didn't match, so--if we've started a
				 * match--restart the search in the simplest, most conservative way. How to do
				 * that depends on whether there are any notes left in failed-match chord.
				 */
				if (targetPos>0) {
					if (k<startNoteCount-1)
						goto NextChordNote;		/* So next iteration will start with next note in failed-match chord */
					else
						pL = mayMatchStartL;		/* So next iteration will start with next DB_LINK */
					targetPos = 0;
					mayMatchStartL = DB_NILINK;
				}
			}
		}
		
		/* We've gotten all the way thru <failL> and everything matched: success. */
		break;
		
NextChordNote:
		;
	}

Cleanup:
	*pMayMatchStartL = mayMatchStartL;
	*pNewPrevSyncL = prevSyncL;
	*pNewPrevNoteL = prevNoteL;
	*pTotalErrorInfo = totalErrorInfo;
	*pNewPrevNoteNum = prevNoteNum;
	
	return targetPos;
}


/* Calculate relevance estimate for a match described by <errInfo>, given the
pitch tolerance and the length of the pattern. */

INT16 CalcRelEstimate(ERRINFO errInfo, INT16 pitchTolerance, FASTFLOAT pitchWeight,
										INT16 patLen)
{
	short maxPError, maxDError;
	FASTFLOAT relPAccuracy, relDAccuracy, relAccuracy;
	
	relPAccuracy = 1.0;
	maxPError = pitchTolerance*patLen;
	if (maxPError>0)
		relPAccuracy = 1.0-((FASTFLOAT)errInfo.pitchErr/maxPError);

	relDAccuracy = 1.0;
	maxDError = patLen;
	if (maxDError>0)
		relDAccuracy = 1.0-((FASTFLOAT)errInfo.durationErr/maxDError);
	
	relAccuracy = pitchWeight*relPAccuracy + (1.0-pitchWeight)*relDAccuracy;
	return (INT16)100.0*relAccuracy;
}


static void DebugShowSSInfo(INT16 searchPatLen, LINK pL, INT16 voice, INT16 voiceStatus[],
						INT16 targetPos[], ERRINFO totalErrorInfo[]);
static void DebugShowSSInfo(INT16 searchPatLen, LINK pL, INT16 voice, INT16 voiceStatus[],
						INT16 targetPos[], ERRINFO totalErrorInfo[])
{
	char statusChar = '?';
	
	if (voiceStatus[voice]==VOICE_NOTPRESENT) statusChar = 'n';
	if (voiceStatus[voice]==VOICE_PRESENT) statusChar = 'p';
	if (voiceStatus[voice]==VOICE_MATCHED) statusChar = 'M';
	DebugPrintf("  SS>SyncMatch(v=%d,L%ld) status=%c: Pos=%d totalPitchError=%d",
			voice, (long)pL, statusChar, targetPos[voice], totalErrorInfo[voice].pitchErr);
	if (targetPos[voice]>=searchPatLen) DebugPrintf(" MATCH\n");
	else                                DebugPrintf("\n");
}


/* Search the given score for the contents of the Search Pattern score, starting at
the given DB_LINK. If we find a match, fill the arrays of match details and voice-found
flags with all matches starting with the DB_LINK where it starts, and return the
starting DB_LINK; else return DB_NILINK. We leave highlighting, etc., to the caller.

We consider only notes and rests; match note/rest status and, for notes, MIDI note
numbers; and we look for melodic patterns only. This function may not handle 1-note
queries properly: see comment below. */

DB_LINK SearchGetHitSet(
				DB_Document *doc,
				DB_LINK startL,
				SEARCHPAT searchPat,
				SEARCHPARAMS sParm,
				Boolean foundVoiceA[],						/* Voices in which pattern was found */
				ERRINFO totalErrorInfo[],					/* Array of match details, by voice */
				DB_LINK matchedObjA[][MAX_PATLEN],			/* Array of matched objects, by voice */
				DB_LINK matchedSubobjA[][MAX_PATLEN]		/* Array of match subobjs, by voice */
				)
{
	INT16 targetPos[MAXVOICES+1];					/* 0 = not matching now, else no. of items matched so far */
	INT16 prevNoteNum[MAXVOICES+1];					/* -1 = no previous note */
	INT16 v, v2;
	ERRINFO oneErrorInfo;
	DB_LINK mayMatchStartL[MAXVOICES+1], fullMatchStartL, pL;
	DB_LINK prevSyncL[MAXVOICES+1], prevNoteL[MAXVOICES+1];
	INT16 voiceStatus[MAXVOICES+1];					/* Status of each voice for the current Sync */

	DB_LINK newMayMatchStartL, newPrevSyncL, newPrevNoteL;
	DB_LINK newObjA[MAX_PATLEN], newSubobjA[MAX_PATLEN];
	INT16 newPrevNoteNum;
	Boolean done;
	long nObjsToSearch;

#ifdef DEBUG_MIDLEVEL
	DebugPrintf("SearchGetHitSet: look for matches. startL=%d\n", startL);
#endif

	for (v = 1; v<=MAXVOICES; v++) {
		targetPos[v] = 0;
		prevNoteNum[v] = -1;
		mayMatchStartL[v] = DB_NILINK;
		prevNoteL[v] = DB_NILINK;
		foundVoiceA[v] = False;
		totalErrorInfo[v].pitchErr = 0;
		totalErrorInfo[v].durationErr = 0;
	}
	fullMatchStartL = DB_NILINK;
		
	/* Prepare to show a crude progress indicator. */
	for (nObjsToSearch = 0, pL = startL; pL!=doc->tailL; pL = DB_RightLINK(pL))
		nObjsToSearch++;
	InitProgressReport(nObjsToSearch);

	/*
	 * In essence, we repeatedly look for the next note/rest and see if it matches the
	 * next item in the search string for its voice. Chords complicate things
	 * considerably. Notice especially that, if we ever allow matches that skip Syncs
	 * with notes in their voice (e.g., for proximity matching), the sequence of Syncs
	 * to test against may not be known in advance: in the relative-pitch or -duration
	 * case, chords not only complicate the logic, they can lead to a combinatorial
	 * explosion in the number of possible matches to test--though there might be a
	 * way to avoid that.
	 */
	for (pL = startL; pL!=doc->tailL; pL = DB_RightLINK(pL)) {
		UpdateProgressReport();
		
		/* If we have a match in at least one voice and no voice has a match in
		 * progress, we're done.
		 */
		if (fullMatchStartL!=DB_NILINK) {
			for (done = True, v = 1; v<=MAXVOICES; v++)
				if (targetPos[v]>0 && targetPos[v]<searchPat.patLen) {
					done = False;
					break;
				}
			if (done) goto Cleanup;
		}

		if (!DB_SyncTYPE(pL)) continue;
		for (v = 1; v<=MAXVOICES; v++)
			voiceStatus[v] = VOICE_NOTPRESENT;

		for (v = 1; v<=MAXVOICES; v++) {
			if (!DB_VOICE_MAYBE_USED(doc, v)) continue;
		
			/* If we're Looking At a Voice, ignore all other voices. */
			if (doc->lookVoice>=0 && v!=doc->lookVoice) continue;
			
			/* If we've already found a match in this voice, ignore this voice. */
			if (foundVoiceA[v]) continue;
			
			/* If we've already found a match in any voice, don't start new matches in
			 * any voice. NB: I think this could result in losing matches for 1-note
			 * queries, but such queries probably shouldn't be allowed anyway (and, as
			 * of this writing, aren't; see the calling functions).
			 */
			if (fullMatchStartL!=DB_NILINK && targetPos[v]==0) continue;
			
			voiceStatus[v] = SyncMatch(sParm, searchPat, v, pL, DB_FindFirstNote(pL, v), matchedObjA[v],
						matchedSubobjA[v], &mayMatchStartL[v], &prevSyncL[v], &prevNoteL[v],
						&prevNoteNum[v], &targetPos[v], &oneErrorInfo);
			if (voiceStatus[v]==VOICE_MATCHED) {
				totalErrorInfo[v].pitchErr += oneErrorInfo.pitchErr;
				totalErrorInfo[v].durationErr += oneErrorInfo.durationErr;
			}
			else if (voiceStatus[v]==VOICE_PRESENT)
				targetPos[v] = 0;
			if (CapsLockKeyDown() && ShiftKeyDown())
				DebugShowSSInfo(searchPat.patLen, pL, v, voiceStatus, targetPos, totalErrorInfo);
			if (targetPos[v]>=searchPat.patLen) {
				/* We've matched the whole pattern in this voice. If it's the first full
				 * match in any voice, save its starting point and disallow any matches
				 * starting later in other voices.
				 */
#ifdef DEBUG_MIDLEVEL
				DebugPrintf(" SearchGetHitSet: startL=%d pL=%d. matched full pattern in voice=%d.\n", startL, pL, v);
#endif
				foundVoiceA[v] = True;
				if (fullMatchStartL==DB_NILINK) {
					fullMatchStartL = mayMatchStartL[v];
					for (v2 = 1; v2<=MAXVOICES; v2++) {
						if (targetPos[v2]>0 && mayMatchStartL[v2]!=fullMatchStartL)
							targetPos[v2] = 0;
					}
				}
			}
		}

		/* We've looked at all voices. If we haven't found a match in any voice yet,
		 * then for any voice that had notes in the Sync but didn't match, restart the
		 * search. Restarting from the correct place is not at all easy: see comments in
		 * RestartSearchTargetPos.
		 */
		for (v = 1; v<=MAXVOICES; v++)
			if (voiceStatus[v]==VOICE_PRESENT && fullMatchStartL==DB_NILINK) {
				short n;
				
				/* If we're Looking At a Voice, ignore notes in all other voices. */
				if (doc->lookVoice>=0 && v!=doc->lookVoice) continue;
		
				targetPos[v] = RestartSearchTargetPos(sParm, searchPat, mayMatchStartL[v],
																	matchedSubobjA[v][0], pL, v,
																	&newMayMatchStartL,
																	&newPrevSyncL, &newPrevNoteL,
																	&newPrevNoteNum,
																	newObjA, newSubobjA, &totalErrorInfo[v]);
				if (CapsLockKeyDown() && ShiftKeyDown())
					DebugPrintf("  SS>RestartSTP(v=%d,L%ld) newMayStart=L%ld: Pos=%d totalPitchError=%d\n",
						v, (long)mayMatchStartL[v], (long)newMayMatchStartL, targetPos[v],
						totalErrorInfo[v].pitchErr);
				for (n = 0; n<targetPos[v]; n++) {
					matchedObjA[v][n] = newObjA[n];
					matchedSubobjA[v][n] = newSubobjA[n];
				}
				mayMatchStartL[v] = newMayMatchStartL;
				prevSyncL[v] = newPrevSyncL;
				prevNoteL[v] = newPrevNoteL;
				prevNoteNum[v] = newPrevNoteNum;
				if (targetPos[v]>=searchPat.patLen) {
						DebugPrintf("RestartSTP FOUND IT!\n");
						fullMatchStartL = mayMatchStartL[v];				/* Matched entire pattern */
				}
		}
	}
	
Cleanup:
	EndProgressReport();
	return fullMatchStartL;
}


Boolean SearchIsLegal(Boolean usePitch, Boolean useDuration)
{
	Boolean haveRest;

	if (N_SearchPatternLen(&haveRest)==0) {
		CParamText("There's nothing to search for: the Search Pattern score is empty. (To see and/or set it, use the Show Search Pattern command.)",
						"", "", "");										// ??I18N BUG
		StopInform(GENERIC_ALRT);
		return False;
	}
	
	if (N_SearchPatternLen(&haveRest)==1) {
		CParamText("The Search Pattern score contains only one note/chord; the minimum is two. (To see and/or set it, use the Show Search Pattern command.)",
						"", "", "");										// ??I18N BUG
		StopInform(GENERIC_ALRT);
		return False;
	}
	
	if (!usePitch && !useDuration) {
		CParamText("Can't search: neither pitch nor duration matching was specified.",
						"", "", "");										// ??I18N BUG
		StopInform(GENERIC_ALRT);
		return False;
	}

	return True;
}


void DebugShowObjAndSubobj(char *label, DB_LINK matchedObjFA[][MAX_PATLEN],
							DB_LINK matchedSubobjFA[][MAX_PATLEN], INT16 patLen,
							INT16 nFound);
void DebugShowObjAndSubobj(
			char *label,						/* Identifying string to display */
			DB_LINK matchedObjFA[][MAX_PATLEN],		/* Array of hit details, by voice */
			DB_LINK matchedSubobjFA[][MAX_PATLEN],	/* Array of hit details, by voice */
			INT16 patLen,
			INT16 nFound
			)
{
	short n, m;

	for (n = 0; n<5 & n<nFound; n++) {
		DebugPrintf("%s: matchedObjFA[%d]=", label, n);
		for (m = 0; m<patLen; m++)
			DebugPrintf("%d ", matchedObjFA[n][m]);
		DebugPrintf("\n");

		DebugPrintf("%s: matchedSubobjFA[%d]=", label, n);
		for (m = 0; m<patLen; m++)
			DebugPrintf("%d ", matchedSubobjFA[n][m]);
		DebugPrintf("\n");
	}
}


void FormatReportString(SEARCHPARAMS sParm, SEARCHPAT searchPat, char *findLabel, char *str)
{
	char weightStr[256];
	char chP, chD;

	switch (sParm.pitchSearchType) {
		case TYPE_PITCHMIDI_ABS:
			chP = 'A'; break;
		case TYPE_PITCHMIDI_ABS_TROCT:
			chP = '8'; break;
		case TYPE_PITCHMIDI_REL:
			chP = 'R'; break;
		default:
			chP = '?';
	}

	switch (sParm.durSearchType) {
		case TYPE_DURATION_ABS:
			chD = 'A'; break;
		case TYPE_DURATION_REL:
			chD = 'R'; break;
		case TYPE_DURATION_CONTOUR:
			chD = 'C'; break;
		default:
			chD = '?';
	}
	
	weightStr[0] = '\0';
	if (sParm.usePitch && sParm.useDuration) sprintf(weightStr, ",W%d%%",	// ??WHY IS useDuration RELEVANT?
				(INT16)(sParm.pitchWeight*100.0));
	sprintf(str, "%s: %s(%c,maxTr=%d,tol=%d/%s%s), %s(%c), %s-%s-%s. %d notes/rests",
				findLabel, (sParm.usePitch? "Pitch" : "noPitch"),
				chP, sParm.maxTranspose, sParm.pitchTolerance,
				(sParm.pitchKeepContour? "contour" : "noContour"), weightStr,
				(sParm.useDuration? "Dur" : "noDur"), chD,
				(sParm.includeRests? "Rests" : "skipRests"),
				(sParm.matchTiedDur? "Ties" : "noTies"),
				(sParm.chordNotes==TYPE_CHORD_TOP? "topN"
					: (sParm.chordNotes==TYPE_CHORD_OUTER? "outerN" : "allN")),
					searchPat.patLen);
}


/* Given an interval of time, return string equivalent in seconds and centiseconds
(NOT milliseconds) without using a floating-point format. Useful when full ANSI C
formatting capabilities may not be available (e.g., as of this writing, with the
standard Symantec C Nightingale project). */

static void FormatTime(long timeMilliSecs, char *str);
static void FormatTime(
		long timeMilliSecs,				/* Interval of time (in milliseconds) */
		char *str)						/* C string, allocated by caller */
{
	INT16 wholeSecs, milliSecs, centiSecs;
	
	wholeSecs = (INT16)(timeMilliSecs/1000.0);
	milliSecs = timeMilliSecs-(1000*wholeSecs);
	centiSecs = (milliSecs+5)/10;

	sprintf(str, "%d.%.2d", wholeSecs, centiSecs);
}


/* SortMatches does a simple insertion sort on the given array, putting it into
descending order of .relEst . (The native version does the same thing, and also sorts
the other arrays on the assumption they're parallel.)

Insertion sort is stable, i.e., if two elements have the same .relEst, their order
is not changed. On the other hand, it's pretty slow--its speed is O(n^2)--but that's
not significant for a few hundred elements or less. */

static void SortMatches(MATCHINFO matchInfoA[], DB_LINK matchedObjFA[][MAX_PATLEN],
					DB_LINK matchedSubobjFA[][MAX_PATLEN], INT16 patLen, INT16 nsize);
static void SortMatches(MATCHINFO matchInfoA[],
					DB_LINK matchedObjFA[][MAX_PATLEN],		/* Array of hit details, by voice */
					DB_LINK matchedSubobjFA[][MAX_PATLEN],	/* Array of hit details, by voice */
					INT16 patLen,
					INT16 nsize)
{
	INT16 i, j, k;
	MATCHINFO temp; DB_LINK tempObj[MAX_PATLEN], tempSubobj[MAX_PATLEN];
	
	if (matchedObjFA==NILINK) return;
	
	for (i = 1; i<nsize; i++) {
		temp = matchInfoA[i];
		for (k = 0; k<patLen; k++) {
			tempObj[k] = matchedObjFA[i][k];
			tempSubobj[k] = matchedSubobjFA[i][k];
		}

		for (j = i-1; j>=0 && matchInfoA[j].relEst < temp.relEst;
				j--) {
			matchInfoA[j+1] = matchInfoA[j];
			for (k = 0; k<patLen; k++) {
				matchedObjFA[j+1][k] = matchedObjFA[j][k];
				matchedSubobjFA[j+1][k] = matchedSubobjFA[j][k];
			}
		}
		
		matchInfoA[j+1] = temp;
		for (k = 0; k<patLen; k++) {
			matchedObjFA[j+1][k] = tempObj[k];
			matchedSubobjFA[j+1][k] = tempSubobj[k];
		}
	}
}


void ListMatches(MATCHINFO matchInfoA[],
					DB_LINK matchedObjFA[][MAX_PATLEN],		/* Array of hit details, by voice */
					DB_LINK matchedSubobjFA[][MAX_PATLEN],	/* Array of hit details, by voice */
					INT16 patLen,
					INT16 nFound,
					long elapsedTicks,
					SEARCHPARAMS sParm,
					Boolean showScoreName
					)
{
	INT16 n;
	char label[256], str[256];
	long elapsedMillisecs;
	char strTime[256];
	Boolean sortByRelEst;

	elapsedMillisecs = TICKS2MS(elapsedTicks);
	FormatTime(elapsedMillisecs, strTime);

	if (nFound==0) {
		CParamText("The pattern was not found.", "", "", "");	// ??I18N BUG
		NoteInform(GENERIC_ALRT);
		DebugPrintf("Time %s sec. Not found.\n", strTime);
		return;
	}
		
	/* At this point, the matches are in the order in which they were found.
	 * Optionally sort them by relevance estimate before displaying them. NB:
	 * we want a stable sort, i.e., one that preserves the order of matches with
	 * equal error; speed isn't important, since we'll rarely if ever be sorting
	 * more than a few hundred items.
	 */
	sortByRelEst = (sParm.usePitch && sParm.pitchTolerance!=0 && !(ShiftKeyDown() && CmdKeyDown()));
	if (sortByRelEst)
		SortMatches(matchInfoA, matchedObjFA, matchedSubobjFA, patLen, nFound);

	sprintf(label, "Time %s sec. %d matches", strTime, nFound);
	sprintf(&label[strlen(label)], " (in order %s):", (sortByRelEst? "of accuracy" : "found"));
	DebugPrintf("%s\n", label);

	//DebugShowObjAndSubobj("ListMatches", (DB_LINK (*)[MAX_PATLEN])matchedObjFA,
	//						(DB_LINK (*)[MAX_PATLEN])matchedSubobjFA, patLen, nFound);

	for (n = 0; n<nFound; n++) {
		Boolean showPitchErr, showDurErr;

		sprintf(str, "  %d: ", n+1);

		if (showScoreName)
			sprintf(&str[strlen(str)], "%s: ", matchInfoA[n].scoreName);

		if (strlen(matchInfoA[n].locStr)>0)
			sprintf(&str[strlen(str)], "m.%d (%s), %s",
					matchInfoA[n].measNum, matchInfoA[n].locStr, matchInfoA[n].vInfoStr);	// ??I18N BUG
		else
			sprintf(&str[strlen(str)], "m.%d, %s",
					matchInfoA[n].measNum, matchInfoA[n].vInfoStr);		// ??I18N BUG

		if (CapsLockKeyDown())
			sprintf(&str[strlen(str)], " (L%ld V%d)",
					(long)matchInfoA[n].foundL, matchInfoA[n].foundVoice);		// ??I18N BUG

		showPitchErr = (sParm.usePitch && sParm.pitchTolerance!=0);
		showDurErr = (sParm.useDuration && sParm.durSearchType==TYPE_DURATION_CONTOUR);
		if (showPitchErr || showDurErr) {

			if (showPitchErr && showDurErr)
				sprintf(&str[strlen(str)], ", err=p%dd%d (%d%%)",
							matchInfoA[n].totalError.pitchErr,
							matchInfoA[n].totalError.durationErr, matchInfoA[n].relEst);
			else if (showPitchErr)
				sprintf(&str[strlen(str)], ", err=p%d (%d%%)",
							matchInfoA[n].totalError.pitchErr,
							matchInfoA[n].relEst);
			else if (showDurErr)
				sprintf(&str[strlen(str)], ", err=d%d (%d%%)",
							matchInfoA[n].totalError.durationErr,
							matchInfoA[n].relEst);

		}

		DebugPrintf("%s\n", str);
	}
}
