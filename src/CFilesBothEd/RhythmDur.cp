/***************************************************************************
*	FILE:	RhythmDur.c																			*
*	PROJ:	Nightingale, revised for v.99													*
*	DESC:	Routines for handling rhythm and higher-level aspects of timing	*
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1990-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
/*
	SetPDur					FixNCStem				SetSyncNoteDur
	SetSelNoteDur
	SetSelMBRest			SetAndClarifyDur		QuantizeSelDurs
	BeatStrength			CalcBeatDur
	MetricStrength			FindStrongPoint		NotatableDur
	SetNRCDur				ClearAccAndMods		SyncAtTime
	MeasLastSync			MakeClarifyList		ClarifyFromList
	ClarifyNRCRhythm		ClarifyRhythm
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void FixNCStem(Document *, LINK, INT16, PCONTEXT);
static Boolean SetSyncNoteDur(Document *, LINK, INT16, INT16, Boolean, Boolean *);
static INT16 BeatStrength(INT16, INT16);
static INT16 CalcBeatDur(INT16, Boolean);
static INT16 MetricStrength(INT16, Boolean, INT16, INT16, Boolean, INT16);
static long FindStrongPoint(long, long, INT16, INT16, Boolean, INT16, INT16 *);
static Boolean NotatableDur(NOTEPIECE [], INT16 *, INT16, INT16);
static LINK SyncAtTime(Document *, LINK, long, long *);
static LINK MeasLastSync(Document *, LINK);
static INT16 ClarifyNRCRhythm(Document *, LINK, LINK);


/* ----------------------------------------------------------------- CalcUNoteLDur -- */
/* Compute the "logical duration" of a note as the time till the next note in its
voice or the next barline, whichever comes first. Intended for unknown-duration notes.
Should probably be integrated into CalcNoteLDur. */

static long CalcUNoteLDur(Document *, LINK, LINK);
static long CalcUNoteLDur(Document */*doc*/, LINK aNoteL, LINK syncL)
{
	short voice; LINK pL;
	
	voice = NoteVOICE(aNoteL);
	
	for (pL = RightLINK(syncL); pL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				if (SyncInVoice(pL, voice))
					return SyncAbsTime(pL)-SyncAbsTime(syncL);
				break;
			case MEASUREtype:
				return MeasureTIME(pL)-SyncAbsTime(syncL);
			default:
				;
		}
		
	return BIGNUM;							/* No following notes in the voice or barlines */
}

/* ----------------------------------------------------------------- SetSyncPDur -- */

Boolean SetSyncPDur(Document *doc, LINK syncL, INT16 pDurPct, Boolean needSel)
{
	LINK aNoteL; long useLDur;
	Boolean didAnything=FALSE;
	
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteSEL(aNoteL) || !needSel) {
			if (NoteType(aNoteL)==UNKNOWN_L_DUR) {
				useLDur = CalcUNoteLDur(doc, aNoteL, syncL);
				if (useLDur==BIGNUM) useLDur = CalcNoteLDur(doc, aNoteL, syncL);
			}
			else
				useLDur = CalcNoteLDur(doc, aNoteL, syncL);
			
			NotePLAYDUR(aNoteL) = (pDurPct*useLDur)/100L;
			didAnything = TRUE;
		}

	return didAnything;
}


/* --------------------------------------------------------------------- SetPDur -- */
/* Set the play duration of either EVERY note in the selection range whether
selected or not, or of every selected note, to the given percentage of its logical
duration. */

void SetPDur(Document *doc, INT16 pDurPct, Boolean needSel)
{
	LINK pL; Boolean didAnything=FALSE;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if ((LinkSEL(pL) || !needSel) && SyncTYPE(pL))
			if (SetSyncPDur(doc, pL, pDurPct, TRUE)) didAnything = TRUE;

	if (didAnything) doc->changed = TRUE;
}


/* ------------------------------------------------------------------- FixNCStem -- */
/* Correct the stem of the given note or chord. Makes no attempt to keep other things
consistent, so it should be used only to change stem lengths while keeping their
direction and everything else the same. Also, should not be used on beamed notes/
chords. */

static void FixNCStem(Document *doc, LINK syncL, INT16 voice, PCONTEXT pContext)
{
	LINK aNoteL, mNoteL; INT16 stemUpDown; DDIST ystem;
	
	mNoteL = FindMainNote(syncL, voice);
	stemUpDown = (NoteYSTEM(mNoteL)>NoteYD(mNoteL)? -1 : 1);
	ystem = GetNCYStem(doc, syncL, voice, stemUpDown,
								(doc->voiceTab[voice].voiceRole==SINGLE_DI), pContext);
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice) {
			if (MainNote(aNoteL))
				NoteYSTEM(aNoteL) = ystem;					/* "Main note": set stem as necessary */
			else
				NoteYSTEM(aNoteL) = NoteYD(aNoteL);		/* Other notes: disappear stem */
		}
}
					

/* -------------------------------------------------------------- SetSyncNoteDur -- */
/* Set the CMN duration of every selected note/rest in the given Sync. As written,
should not be called for notes/rests in tuplets, nor to set to unknown duration or
whole-measure rest; to handle any of these would require at least replacing
SimplePlayDur with CalcPlayDur, but maybe more. */

Boolean SetSyncNoteDur(
				Document *doc,
				LINK syncL,
				INT16 newDur, INT16 newDots,
				Boolean setPDur,
				Boolean *warnedRests					/* Set performance durations? */
				)
{
	INT16 v;
	LINK aNoteL, mNoteL; PANOTE aNote;
	Boolean didAnything=FALSE;
	Boolean voiceChanged[MAXVOICES+1];
	CONTEXT context;
	
	for (v = 0; v<=MAXVOICES; v++)
		voiceChanged[v] = FALSE;

	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteSEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL);
			if (newDur==UNKNOWN_L_DUR) {
				if (aNote->rest) {
					if (!*warnedRests) {
						GetIndCString(strBuf, RHYTHMERRS_STRS, 1);    /* "You can't set rests to unknown duration." */
						CParamText(strBuf,	"", "", "");
						StopInform(GENERIC_ALRT);
					}
					*warnedRests = TRUE;
					continue;
				}
			}
			aNote->subType = newDur;
			aNote->ndots = newDots;
			
			if (setPDur) {		
				aNote = GetPANOTE(aNoteL);
				aNote->playDur = SimplePlayDur(aNoteL);
			}
			voiceChanged[aNote->voice] = didAnything = TRUE;
		}
		
		/* Update stems for the notes or entire chords modified. */
		
		for (v = 0; v<=MAXVOICES; v++)
			if (voiceChanged[v] && newDur>=HALF_L_DUR) {
				mNoteL = FindMainNote(syncL, v);
				GetContext(doc, syncL, NoteSTAFF(mNoteL), &context);
				FixNCStem(doc, syncL, v, &context);
			}

	if (didAnything) doc->changed = TRUE;
	return didAnything;
}


/* --------------------------------------------------------------- SetSelNoteDur -- */
/* Extend the selection to include every note in every chord that has any note(s)
selected; then set every selected note/rest to the given duration code and number
of aug. dots. Ignores beaming and tuplets, so it should not be called to operate on
anything that is beamed or in a tuplet. */

Boolean SetSelNoteDur(
					Document *doc,
					INT16 newDur, INT16 newDots,
					Boolean setPDur					/* Set performance durations? */
					)
{
	Boolean	didAnything, warnedRests;
	LINK		pL;
	
	if (newDur==WHOLEMR_L_DUR)	MayErrMsg("SetSelNoteDur: can't handle whole-measure rests.");

	/* Extend selection to incl. every note in every chord that has any note(s) selected. */

	if (ExtendSelChords(doc)) InvalWindow(doc);
	
	didAnything = warnedRests = FALSE;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			if (SetSyncNoteDur(doc, pL, newDur, newDots, setPDur, &warnedRests))
				didAnything = TRUE;

	return didAnything;
}


/* ---------------------------------------------------- SetDoubleHalveSelNoteDur -- */

Boolean SetDoubleHalveSelNoteDur(
					Document *doc,
					Boolean doubleDur,	/* TRUE: double durations; FALSE: halve durations */
					Boolean setPDur 		/* Set performance durations? */
					)
{
	Boolean	didAnything, warnedRests;	// ??IGNORES <warnedRests>
	LINK pL, aNoteL;
	INT16 v, newDur, newDots, staff;
	char fmtStr[256];
	CONTEXT context;

	/* Extend selection to incl. every note in every chord that has any note(s) selected. */

	if (ExtendSelChords(doc)) InvalWindow(doc);

	didAnything = warnedRests = FALSE;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			for (v = 1; v<=MAXVOICES; v++)
				if (VOICE_MAYBE_USED(doc, v)) {
					newDots = -999;
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteVOICE(aNoteL)==v && NoteSEL(aNoteL)) {
							newDur = NoteType(aNoteL);
							newDots = NoteNDOTS(aNoteL);
							staff = NoteSTAFF(aNoteL);
							break;
						}
					if (newDots<0) continue;

					/* If we get here, the NRC has at least one sel. note. Double duration
					 * by reducing the dur. code by 1, or halve it by increasing the dur. 
					 * code by 1. Leave no. of dots alone.
					 */
					if (newDur<=WHOLEMR_L_DUR)			/* skip whole- and multi-measure rests */
						continue;
					if (doubleDur) {
						newDur--;
						if (newDur<=UNKNOWN_L_DUR) {
							GetIndCString(fmtStr, RHYTHMERRS_STRS, 7);	/* "Can't double note duration: it's already the maximum." */
							CParamText(fmtStr, "", "", "");
							StopInform(GENERIC_ALRT);
							return didAnything;
						}
					}
					else {
						newDur++;
						if (newDur>=ONE28TH_L_DUR) {
							GetIndCString(fmtStr, RHYTHMERRS_STRS, 8);	/* "Can't halve note duration: it's already the minimum." */
							CParamText(fmtStr, "", "", "");
							StopInform(GENERIC_ALRT);
							return didAnything;
						}
					}

					GetContext(doc, pL, staff, &context);

					SetNRCDur(doc, pL, v, newDur, newDots, &context, setPDur);
					didAnything = TRUE;
				}
		}

	return didAnything;
}


/* ---------------------------------------------------------------- SetSelMBRest -- */
/* Set all selected whole rests, whole-measure rests, or multibar rests to the given
number of measures. */

Boolean SetSelMBRest(Document *doc, INT16 newNMeas)
{
	Boolean	didAnything;
	LINK		pL, aNoteL, startFixTimeL=NILINK, endFixTimeL;
	
	didAnything = FALSE;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteREST(aNoteL))
					if (NoteType(aNoteL)<=WHOLE_L_DUR) {
						if (NoteType(aNoteL)>WHOLEMR_L_DUR) {
							if (!startFixTimeL) startFixTimeL = pL;
							endFixTimeL = pL;
						}
						NoteType(aNoteL) = -newNMeas;
						didAnything = TRUE;
					}

	if (startFixTimeL) FixTimeStamps(doc, startFixTimeL, endFixTimeL);

	return didAnything;
}


#define MAXPIECES 20

/* ------------------------------------------------------------ SetAndClarifyDur -- */
/* Set the given note/rest/chord (NRCs) to duration <lDur> and "clarify" the result,
i.e., if necessary, divide it into a series of (tied, unless they're rests) NRCs for
readability. If we succeed, return the number of NRCs, else FALSE. */

INT16 SetAndClarifyDur(
				Document *doc,
				LINK syncL, LINK aNoteL,
				INT16 lDur 					/* Logical duration in PDUR ticks */
				)
{
	CONTEXT context;
	INT16 voice, staff, kount;
	Boolean compound;
	long startTime;
	NOTEPIECE piece[MAXPIECES];
	
	voice = NoteVOICE(aNoteL);
	staff = NoteSTAFF(aNoteL);

	GetContext(doc, syncL, staff, &context);
	
	/* Use compoundness heuristic. Would be better to have compoundess explicit, though. */
	compound = COMPOUND(context.numerator);
	startTime = GetLTime(doc, syncL);

	if (!MakeClarifyList(startTime, lDur, context.numerator, context.denominator,
					compound, piece, MAXPIECES, &kount)) return 0;

	/*
	 *	<piece> now contains a list of start times and durations for the segments we want
	 *	to divide our note/rest/chord into. Divide it up.
	 */
	ClarifyFromList(doc, syncL, voice, (NoteREST(aNoteL)? 0 : staff), piece, kount,
							&context);
	doc->changed = TRUE;
	return kount;
}


#ifdef NOMORE

/* -------------------------------------------------------------- QuantizeSelDurs -- */
/* Extend the selection vertically to include in their entirety partly-selected
chords and extend it horizontally to include in their entirety partly-selected
beats; then quantize the attack time??YES! and duration of every selected note/rest/chord
to a grid whose points are separated by the duration described by <quantumDur> and
<quantumDots>. If there are pauses between the notes, intersperse rests as
necessary??. */

Boolean QuantizeSelDurs(Document *doc, INT16 quantumDur, INT16 quantumDots,
								Boolean tuplet)
{
	LINK			pL, aNoteL;
	PANOTE		aNote;
	INT16			quantum, mult;
	Boolean		didAnything;
	
	didAnything = FALSE;
	if (ExtendSelChords(doc)) InvalWindow(doc);

	quantum = Code2LDur(quantumDur, quantumDots);
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && MainNote(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->subType==UNKNOWN_L_DUR)
						mult = (aNote->playDur+PDURUNIT-1)/quantum;
					else
						mult = SimpleLDur(aNoteL)/quantum;
					/*
					 *	It's not really obvious what should be done with notes/rests/chords
					 * short enough that their durations would normally be rounded to zero.
					 * Reasonable possibilities include setting them to the quantum,
					 *	discarding them, and, for notes/chords, make them grace notes/chords.
					 *	For now, we'll go the first way.
					 */
					if (mult==0) mult = 1;
					if (SetAndClarifyDur(doc, pL, aNoteL, mult*quantum)>0)
						didAnything = TRUE;
				}
		}
	return didAnything;
}

#endif

/* ---------------------------------------------------------------- BeatStrength -- */

#define SUPERBEAT 9				/* Greatest possible metric strength */
#define WIMP -9					/* Least possible metric strength */

#define MAXBEATS 11

SignedByte beats[MAXBEATS-1][MAXBEATS-1] = {
	{0,0,0, 0,0,0,0, 0,0,0},	/* 2 beats per measure */
	{0,0,0, 0,0,0,0, 0,0,0},	/* 3 beats */
	{0,1,0, 0,0,0,0, 0,0,0},	/* 4 beats */
	{0,1,0, 0,0,0,0, 0,0,0},	/* 5 beats */
	{0,0,1, 0,0,0,0, 0,0,0},	/* 6 beats */
	{0,1,0, 1,0,0,0, 0,0,0},	/* 7 beats */
	{0,1,0, 2,0,1,0, 0,0,0},	/* 8 beats */
	{0,0,1, 0,0,1,0, 0,0,0},	/* 9 beats */
	{0,1,0, 0,2,0,1, 0,0,0},	/* 10 beats */
	{0,1,0, 0,2,0,0, 1,0,0},	/* 11 beats */
};

/* Deliver the metric strength (in the system of Byrd's dissertation, Sec. 4.4.1)
of beat <beatNum> in a meter with <nBeats> beats. <beatNum>=0 is the downbeat.
Consider <beatNum> modulo <nBeats>, i.e., in case the beat is past where the measure
should end, act as if there are barlines at the intervals the meter indicates. */

static INT16 BeatStrength(INT16 beatNum, INT16 nBeats)
{
	beatNum = beatNum % nBeats;
	if (beatNum==0) return SUPERBEAT;
	
	if (nBeats<=1 || nBeats>MAXBEATS) return 0;
		
	return beats[nBeats-2][beatNum-1];
}


/* ----------------------------------------------------------------- CalcBeatDur -- */

INT16 CalcBeatDur(INT16 timeSigDenom, Boolean compound)
{
	INT16 wholeDur, beatDur;
	
	wholeDur = Code2LDur(WHOLE_L_DUR, 0);
	beatDur = wholeDur/timeSigDenom;
	if (compound) beatDur *= 3;
	return beatDur;
}


/* ------------------------------------------------------------- MetricStrength -- */
/* Given an elapsed time within the measure and a description of the current time
signature, deliver the metric strength of that elapsed time in the system described
in Byrd's dissertation, Sec. 4.4.1. Essentially, the higher the value the stronger,
and the weakest beat has a strength of 0. So positive values represent strong beats
and negative values represent levels below the beat. */

INT16 MetricStrength(
			INT16		time,							/* Elapsed time in playDur units */
			Boolean	tuplet,						/* Is it within a tuplet? */
			INT16		timeSigNum,
			INT16		timeSigDenom,
			Boolean	compound,					/* TRUE=compound meter ==> 3 denom.units/beat, etc. */
			INT16		errMax 						/* Must be >0 */
			)
{
	INT16 beatDur, remainder, fracBeatDur;
	INT16 beatNum, level;
	
	if (tuplet) return WIMP;
	
	beatDur = CalcBeatDur(timeSigDenom, compound);
	
	beatNum = time/beatDur;
	remainder = time-beatNum*beatDur;
	if (remainder<errMax) 
		return BeatStrength(beatNum, (compound? timeSigNum/3 : timeSigNum));
	
	fracBeatDur = (compound? beatDur/3 : beatDur/2);
	for (level=-1; fracBeatDur>1; level--,fracBeatDur/=2) {
		if (remainder>=fracBeatDur) remainder -=fracBeatDur;
		if (compound && remainder>=fracBeatDur) remainder -=fracBeatDur;
		if (remainder<errMax) break;
	}
	
	return (compound? level-1 : level);				/* In compound meter, skip level -1 */
}


/* -------------------------------------------------------------- FindStrongPoint -- */
/* Try to find a point within the given range of times whose metric strength is at
least <minStrength>; if there isn't any, look for a point whose metric strength is
SUPERBEAT (this is the downbeat of the next measure), regardless of <minStrength>.
If either type of point is found, set *pStrength to its actual strength and return
its playDur elapsed time; if there is no point of either type, return -1. */

#define TRUNC(value, mod)	(((value)/(mod))*(mod))

long FindStrongPoint(
			long startTime, long endTime,		/* PDUR times relative to beginning of measure */
			INT16 timeSigNum, INT16 timeSigDenom,
			Boolean compound,						/* TRUE=compound meter (6/8, 9/8, etc.) */
			INT16 minStrength,
			INT16 *pStrength
			)
{
	INT16 durIncr, beatDur, strength;
	long time, timeSigDur, startMeasTime;
	
	/* Look for a strong enough point within the measure */
	
	beatDur = CalcBeatDur(timeSigDenom, compound);
	durIncr = beatDur;
	if (minStrength<0) {
		INT16 s = minStrength;
		while (s++<0)
			durIncr /= 2;
	}
	
	time = TRUNC(startTime, durIncr);
	if (time<startTime) time += durIncr;
	for ( ; time<endTime; time += durIncr) {
		strength = MetricStrength(time, FALSE, timeSigNum, timeSigDenom, compound,
											PDURUNIT-1);
		if (strength>=minStrength) {
			*pStrength = strength;
			return time;
		}
	}

	/* Nothing within the measure. See if the range extends into the next measure */
	
 	timeSigDur = TimeSigDur(0, timeSigNum, timeSigDenom);
 	startMeasTime = (startTime/timeSigDur)*timeSigDur;
	if (endTime>startMeasTime+timeSigDur) {
		*pStrength = SUPERBEAT;
		return startMeasTime+timeSigDur;
	}

	return -1L;
}


/* ---------------------------------------------------------------- NotatableDur -- */
/* Given <piece>, a table of attack times of "pieces"--tied notes--that make up what
is conceptually a single note, convert <piece[this]> thru <piece[this+1]> to "logical"
duration equivalent. If the duration isn't notatable with a single note, break it down
into two pieces, if possible. For example, if the duration is 5 eighths, we replace
it with a half followed by an eighth. In either case, return TRUE; if there's no way
to represent the piece with an error of no more than MAX_ERR, return FALSE. */

#define MAX_DOTS 1	/* Maximum no. of augmentation dots allowed for any duration */
#define MAX_ERR (PDURUNIT-1)

static Boolean NotatableDur(
						NOTEPIECE piece[],
						INT16 *pKount,					/* number of pieces */
						INT16 iPiece,					/* index of piece to work on */
						INT16 maxPieces
						)
{
	char	newDur, newDots;
	INT16	i, thisDur;
		
	if (*pKount>=maxPieces) return FALSE;							/* Illegal call */

	if (LDur2Code(piece[iPiece+1].time-piece[iPiece].time, MAX_ERR, MAX_DOTS,
						&newDur, &newDots)) {
		piece[iPiece+1].lDur = newDur;								/* Can notate as one piece */
		piece[iPiece+1].nDots = newDots;
	}
	else {
		if (newDur>MAX_L_DUR) return FALSE;							/* Not notable at all */
		
		for (i = *pKount; i>iPiece; i--)								/* Break into two pieces */
			piece[i+1] = piece[i];
		(*pKount)++;
		thisDur = Code2LDur(newDur, newDots);
		piece[iPiece+1].time = piece[iPiece].time+thisDur;		/* Start time of the new piece */
		piece[iPiece+1].lDur = newDur;
		piece[iPiece+1].nDots = newDots;
	}
	return TRUE;
}


/* ------------------------------------------------------------------- SetNRCDur -- */
/* Set the logical and physical duration of the given note/rest/chord. Will not work
within tuplets or for whole-measure rests or unknown duration! */

void SetNRCDur(
				Document *doc,
				LINK syncL,
				INT16 voice,
				char durCode, char nDots,
				PCONTEXT pContext,
				Boolean setPDur
				)
{
	LINK aNoteL, vNoteL;
	PANOTE aNote;
	long pDur;
	
	pDur = Code2LDur(durCode, nDots);

	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice) {
			vNoteL = aNoteL;
			aNote = GetPANOTE(aNoteL);
			aNote->subType = durCode;
			aNote->ndots = nDots;
			if (setPDur) aNote->playDur = pDur;
		}

	if (!NoteREST(vNoteL)) FixNCStem(doc, syncL, NoteVOICE(vNoteL), pContext);
}


/* ------------------------------------------------------------- ClearAccAndMods -- */

void ClearAccAndMods(Document */*doc*/, LINK syncL, INT16 voice)
{
	LINK aNoteL;
	PANOTE aNote;
	
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice) {
			aNote = GetPANOTE(aNoteL);
			aNote->accident = 0;
			aNote->firstMod = NILINK;
		}
}


/* ------------------------------------------------------------------ SyncAtTime -- */
/* Look for a Sync at or after time <lTime> in pL's measure. */

static LINK SyncAtTime(Document *doc, LINK pL, long lTime, long *pFoundTime)
{
	LINK measL, syncL;
	
	measL = LSSearch(pL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	
	/*
	 * Search right for a Sync at lTime after pL's Measure. TimeSearchRight returns
	 * the first Sync at or after the given time.
	 */
	syncL = TimeSearchRight(doc, RightLINK(measL), SYNCtype, lTime, MIN_TIME);
	if (syncL) {
		*pFoundTime = GetLTime(doc, syncL);
		return syncL;
	}

	return NILINK;
}


/* ---------------------------------------------------------------- MeasLastSync -- */
/* Deliver the last Sync in pL's Measure. Assumes that pL is in a Measure and that
the Measure contains at least one Sync! */

static LINK MeasLastSync(Document *doc, LINK pL)
{
	LINK endMeasL;
	
	endMeasL = EndMeasSearch(doc, pL);
	return (LSSearch(endMeasL, SYNCtype, ANYONE, GO_LEFT, FALSE));
}


/* ------------------------------------------------------------- MakeClarifyList -- */
/* Make a list of (tied, for notes and chords) segments into which a single
note/rest/chord should be divided for rhythmic clarity. The algorithm is discussed
in detail in Sec. 4.4.1 of Byrd's dissertation. Returns TRUE if it succeeds, FALSE
if it fails (due to an error). */

Boolean MakeClarifyList(
				long startTime,
				INT16 lDur,
				INT16 numerator, INT16 denominator,	/* of time signature */
				Boolean compound,							/* TRUE=compound meter (6/8, 9/8, etc.) */
				NOTEPIECE piece[],
				INT16 maxPieces,
				INT16 *pKount
				)
{
	INT16 startStrength, endStrength, strength, kount, k;
	long endTime, searchFrom, strongerPoint;
	long mayDivPoint;
	Boolean keepGoing;
	char fmtStr[256];
	
	endTime = startTime+lDur;

	startStrength = MetricStrength(startTime, FALSE, numerator, denominator,
												compound, PDURUNIT-1);
	endStrength = MetricStrength(endTime, FALSE, numerator, denominator,
												compound, PDURUNIT-1);

	/* Fill in the <piece[].time>s with tentative attack times of pieces. */
	
	searchFrom = piece[0].time = startTime;
	kount = 0; strongerPoint = 0;
	do {
		keepGoing = FALSE;
		if (kount>=maxPieces-2) {
			GetIndCString(fmtStr, RHYTHMERRS_STRS, 4);    /* "Can't clarify rhythm of a note/rest/chord..." */
			sprintf(strBuf, fmtStr, maxPieces, startTime, endTime); 
			CParamText(strBuf,	"", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}
		
		/*
		 *	Search for a point within the note that's metrically stronger than the point
		 *	where it starts, and consider dividing the note there. Rule numbers refer to
		 *	the algorithm in Sec. 4.4.1 (p. 130) of Byrd's dissertation.
		 */
		mayDivPoint = FindStrongPoint(searchFrom+1, endTime, numerator,
								denominator, compound, startStrength+1, &strength);
		if (mayDivPoint>0) {
			if (strength==SUPERBEAT														/* End measure */
			||  strength>=startStrength+2) {											/* Rule 1 */
				piece[++kount].time = mayDivPoint;
				startStrength = strength;
				strongerPoint = 0;
				}
			else {
				if (strongerPoint>0														/* Rule 2 */
				|| endStrength<startStrength) {										/* Rule 3 */
						piece[++kount].time = mayDivPoint;
						startStrength = strength;
						strongerPoint = 0;
					}
				else strongerPoint = mayDivPoint;
			}
			searchFrom = mayDivPoint;
			keepGoing = TRUE;
		}
	} while (keepGoing);
	
	piece[++kount].time = endTime;			/* For convenience, add endTime to piece array */
	
	/*
	 * Try to convert each of the pieces we've generated from start time/end time to
	 * "logical" duration. If any of the pieces isn't a legal duration, break it down
	 * further. (In that case the loop limit, <kount>, will increase as we go.) 
	 */
	for (k = 0; k<kount; k++) {
		if (!NotatableDur(piece, &kount, k, maxPieces)) {
			MayErrMsg("MakeClarifyList: can't notate piece at %ld for startTime=%ld endTime=%ld.",
						(long)piece[k].time, (long)startTime, (long)endTime);
			return FALSE;
		}
	}
#ifdef RDEBUG
for (k=0; k<kount; k++)
	DebugPrintf(">Piece %d time=%ld dur=%d dots=%d\n", k, piece[k].time,
						(k>0? piece[k].lDur : -1), (k>0? piece[k].nDots : -1));
DebugPrintf(">End     time=%ld dur=%d dots=%d\n", piece[kount].time,piece[kount].lDur,piece[kount].nDots);
#endif

	*pKount = kount;
	return TRUE;
}


/* ------------------------------------------------------------- ClarifyFromList -- */
/* Given a note/rest/chord and a list <piece> of start times and durations for
segments we want to divide it into, divide the note/rest/chord (henceforth, "NRC")
up. Merge the segments into existing Syncs when possible, otherwise put them into
brand-new Syncs. If the NRC is tied to the right, also updates the Slur's firstSyncL
to point to the last segment.*/

Boolean ClarifyFromList(
				Document *doc,
				LINK		syncL,
				INT16		voice,
				INT16		tieStaff,	/* Staff no. for ties, or <=0 for no ties (use with rests) */
				NOTEPIECE piece[],
				INT16		kount,
				PCONTEXT pContext
				)
{
	LINK newL; LINK qL, matchL, lastSyncL, aNoteL, tieL, prevL;
	INT16 k; long foundTime; SearchParam pbSearch;
	
	/*
	 *	Strategy: set the existing NRC's duration to the first segment's, and add
	 *	new NRCs for the other segments.
	 */
	SetNRCDur(doc, syncL, voice, piece[1].lDur, piece[1].nDots, pContext, TRUE);
	doc->changed = TRUE;

	if (kount<2) return TRUE;
	
	aNoteL = FindMainNote(syncL, voice);
	if (NoteTIEDR(aNoteL)) {
		InitSearchParam(&pbSearch);
		pbSearch.subtype = TRUE;											/* = tie */
		pbSearch.id = ANYONE;
		pbSearch.voice = voice;
		pbSearch.needSelected = FALSE;
		pbSearch.inSystem = FALSE;
		tieL = L_Search(syncL, SLURtype, GO_LEFT, &pbSearch);
	}
	else
		tieL = NILINK; 
	prevL = syncL;

	for (qL = syncL, k = 2; k<=kount; k++) {
		newL = DuplicNC(doc, qL, voice);
		if (!newL) return FALSE;
		
		/*
		 *	Look for a Sync at or after the time for this piece. If we find one at exactly
		 *	the desired time and it has nothing in our NRC's voice, merge into it;
		 *	otherwise insert before it. If we don't find one at all, this segment goes
		 * after the last existing Sync in the Measure.
		 */
		matchL = SyncAtTime(doc, syncL, piece[k-1].time, &foundTime);
		if (matchL) {
			if (foundTime==piece[k-1].time && !SyncInVoice(matchL, voice)) {
				MergeObject(doc, newL, matchL);
				newL = matchL;
			}
			else
				InsNodeIntoSlot(newL, matchL);
		}
		else {
			lastSyncL = MeasLastSync(doc, syncL);
			InsNodeIntoSlot(newL, RightLINK(lastSyncL));
		}

		SetNRCDur(doc, newL, voice, piece[k].lDur, piece[k].nDots, pContext, TRUE);
		LinkSEL(newL) = TRUE;
		if (tieStaff>0)
			if (!AddNoteTies(doc, qL, newL, tieStaff, voice)) return FALSE;
		ClearAccAndMods(doc, newL, voice);
		prevL = qL = newL;
	}
	
	if (tieL) {
		MoveNode(tieL, prevL);
		SlurFIRSTSYNC(tieL) = prevL;
	}

	return TRUE;
}


/* ------------------------------------------------------------ ClarifyNRCRhythm -- */
/* Clarify the rhythm of a single note/rest/chord by dividing it, if necessary, into
two or more tied notes/rests/chords. Return values are:
	-1		Miscellaneous error
	0		Note/rest/chord (henceforth, "NRC") was not divided because it's in a tuplet
	1		NRC didn't need to be divided
	>1		NRC sucessfully divided into this many pieces
*/

INT16 ClarifyNRCRhythm(Document *doc, LINK pL, LINK aNoteL)
{
	long startTime;
	NOTEPIECE piece[MAXPIECES];
	CONTEXT context;
	Boolean compound;
	INT16 lDur, voice, staff, kount;
	
	if (NoteINTUPLET(aNoteL)) return 0;

	voice = NoteVOICE(aNoteL);
	staff = NoteSTAFF(aNoteL);

	GetContext(doc, pL, staff, &context);
	
/* Use compoundness heuristic. Would be better to have compoundess stored, though. */
	compound = COMPOUND(context.numerator);
	startTime = GetLTime(doc, pL);
	lDur = SimpleLDur(aNoteL);

	if (!MakeClarifyList(startTime, lDur, context.numerator, context.denominator,
					compound, piece, MAXPIECES, &kount)) return -1;

	if (kount<=1) return 1;							/* The existing note/chord is OK */

	/*
	 *	<piece> now contains a list of start times and durations for the segments we
	 *	want to divide our NRC into. Divide it up.
	 */
	ClarifyFromList(doc, pL, voice, (NoteREST(aNoteL)? 0 : staff), piece, kount, &context);
	
	return kount;
}


/* --------------------------------------------------------------- ClarifyRhythm -- */
/* Extend the selection to include every note in every chord that has any note(s)
selected; then clarify the rhythm of all selected notes/rests/chords.
Comment in FixExtCrossLinks if fixing up ptrs for Beams, Octavas and Tuplets is
needed.
*/

Boolean ClarifyRhythm(Document *doc)
{
	LINK pL, aNoteL;
	Boolean didAnything=FALSE, warnedTuplets=FALSE, warnedBadDur=FALSE;
	INT16 v;
	INT16 clarified;
	
	/* Extend selection to incl. every note in every chord that has any note(s) selected. */

	WaitCursor();
	DisableUndo(doc, FALSE);	
	if (ExtendSelChords(doc)) InvalWindow(doc);
	
	/*
	 *	Now, for every voice in the score that's in use, traverse the selection range, and
	 *	in every selected Sync, try to clarify the rhythm of the voice's note/rest/chord.
	 */
	for (v = 1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {
			for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
				if (LinkSEL(pL) && SyncTYPE(pL)) {
					aNoteL = FindMainNote(pL, v);
					if (aNoteL==NILINK) continue;
					if (!NoteSEL(aNoteL)) continue;

					if (NoteType(aNoteL)<=UNKNOWN_L_DUR) {
						if (!warnedBadDur) {
							GetIndCString(strBuf, RHYTHMERRS_STRS, 2);    /* "Nightingale can't clarify unknown duration notes or..." */
							CParamText(strBuf,	"", "", "");
							StopInform(GENERIC_ALRT);
							warnedBadDur = TRUE;
						}
						continue;
					}

					clarified = ClarifyNRCRhythm(doc, pL, aNoteL);
					if (clarified==0 && !warnedTuplets) {
						GetIndCString(strBuf, RHYTHMERRS_STRS, 3);    /* "Nightingale can't clarify notes/rests in tuplets..." */
						CParamText(strBuf,	"", "", "");
						StopInform(GENERIC_ALRT);
						warnedTuplets = TRUE;
					}
					else if (clarified>1)
						didAnything = TRUE;
				}
						
		}

		if (didAnything) {
			/*
			 *	Extend selection range to include all newly-created slurs in the
			 *	slot of the first currently-selected object, so their shapes will
			 * be reset below. Actually this may extend the range to include pre-
			 * existing slurs, but their "selected" flags won't be set, so it
			 * won't matter.
			 */
			while (SlurTYPE(LeftLINK(doc->selStartL)))
				doc->selStartL = LeftLINK(doc->selStartL);
			doc->selEndL = doc->tailL;
			OptimizeSelection(doc);
#ifdef RDEBUG
			FixExtCrossLinks(doc,doc,doc->selStartL,doc->selEndL);
#endif
			if (doc->autoRespace) {
				RespaceBars(doc, doc->selStartL, doc->selEndL, 0L, FALSE, FALSE);

				for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
					if (LinkSEL(pL) && SlurTYPE(pL))
						SetAllSlursShape(doc, pL, TRUE);
			}
			else
				InvalWindow(doc);
			FixTimeStamps(doc, doc->selStartL, NILINK);
		}

		return didAnything;
}
