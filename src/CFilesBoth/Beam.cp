/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* Beam.c for Nightingale - beaming-related functions for regular notes/rests, plus
functions that are common to grace notes and regular notes/rests.
	DoBeam					DoUnbeam				Unbeam
	CreateBeams				RangeChkBeam			Rebeam
	GetBeamEndYStems		TopStaff2Staff			Staff2TopStaff
	FixSyncInBeamset		GetBeamNotes
	GetCrossStaff			AddNewBeam				CheckBeamStaves
	SetBeamXStf				Extend1Stem				ExtendStems
	SlantBeamNoteStems
	FillSlantBeam			FillHorizBeam			CreateXSysBEAMSET
	CheckBeamVars			CreateNonXSysBEAMSET	CreateBEAMSET
	UnbeamV					Unbeamx					FixNCStem
	UnbeamRange				BeamHasSelNote			FixBeamsOnStaff
	FixBeamsInVoice			UnbeamFixNotes			RemoveBeam
	FlipFractional			DoFlipFractionalBeam

	CalcXStem				NoteXStfYStem			AnalyzeBeamset
	BuildBeamDrawTable		SetBeamRect				DrawBEAMSET
	Draw1Beam

	GetBeamSyncs			SelBeamedNote			CountXSysBeamable
	CountBeamable			GetCrossStaffYLevel		CalcBeamYLevel
	VHasBeamAcross			HasBeamAcross			VCheckBeamAcross
	VCheckBeamAcrossIncl	FirstInBeam				LastInBeam
	XLoadBeamSeg
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Prototypes for internal functions */

static void CreateBeams(Document *, short);
static Boolean RangeChkBeam(short, LINK, LINK);
static DDIST TopStaff2Staff(DDIST, short, STFRANGE, DDIST);
static void FixSyncInBeamset(Document *, LINK, short, DDIST, STFRANGE, Boolean);
static void Unbeamx(Document *, LINK, LINK, short);
static void FixNCStem(Document *, LINK, LINK, CONTEXT);
static Boolean BeamHasSelNote(LINK);
static void UnbeamFixNotes(Document *, LINK, LINK, short);
static Boolean IsBeamStemUp(LINK beamL);


/* ======================================================= CREATING/REMOVING/MODIFYING == */

/* ---------------------------------------------------------------------------- DoBeam -- */
/* Beam the selection, both regular notes/rests and grace notes. User-interface
level. */

void DoBeam(Document *doc)
{
   short v;

	PrepareUndo(doc, doc->selStartL, U_Beam, 2);				/* "Undo Beam" */

	for (v = 1; v<=MAXVOICES; v++) {
		if (VOICE_MAYBE_USED(doc, v)) {
			CreateBeams(doc, v);
			CreateGRBeams(doc, v);
		}
	}
}


/* -------------------------------------------------------------------------- DoUnbeam -- */
/* Unbeam the selection. Remove both regular and grace beams. User-interface level. */

void DoUnbeam(Document *doc)
{
	LINK startL, endL;
	
	WaitCursor();
	GetOptSelEnds(doc, &startL, &endL);
	PrepareUndo(doc, startL, U_Unbeam, 3);						/* "Undo Unbeam" */

	Unbeam(doc);
}

/* ---------------------------------------------------------------------------- Unbeam -- */
/* Unbeam the selection. Remove both regular and grace beams. */

void Unbeam(Document *doc)
{
	LINK vStartL, vEndL;
	short v;
	
	for (v = 1; v<=MAXVOICES; v++) {
		if (VOICE_MAYBE_USED(doc, v)) {
			GetNoteSelRange(doc, v, &vStartL, &vEndL, NOTES_BOTH);
			if (vStartL && vEndL) {
				if (RangeChkBeam(v, vStartL, vEndL)) {
					UnbeamV(doc, vStartL, vEndL, v);
					GetNoteSelRange(doc, v, &vStartL, &vEndL, NOTES_BOTH);
				}
			}
		}
	}
}


/* ----------------------------------------------------------------------- CreateBeams -- */
/* Beam the selection range in the given voice. */

static void CreateBeams(Document *doc, short voice)
{
	LINK beamL; 
	short nBeamable;
	
	beamL=NILINK;
	nBeamable = CountBeamable(doc, doc->selStartL, doc->selEndL, voice, True);
	if (nBeamable>1)
		beamL = CreateBEAMSET(doc, doc->selStartL, doc->selEndL, voice,
						nBeamable, True, doc->voiceTab[voice].voiceRole);

	if (beamL)
		if (doc->selStartL==FirstInBeam(beamL))				/* Does sel. start at beam's 1st note? */
			doc->selStartL = beamL;							/* Yes, add the beam to the selection */
}


/* ---------------------------------------------------------------------- RangeChkBeam -- */
/* Are any notes, rests, or grace notes in the given voice and range beamed? */

static Boolean RangeChkBeam(short voice, LINK vStartL, LINK vEndL)
{
	LINK pL, aNoteL, aGRNoteL;
	
	for (pL = vStartL; pL!=vEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL) && ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteVOICE(aNoteL)==voice) break;
			}
			if (NoteBEAMED(aNoteL)) return True;
		}
		
		if (LinkSEL(pL) && ObjLType(pL)==GRSYNCtype) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				if (GRNoteVOICE(aGRNoteL)==voice) break;
			}
			if (GRNoteBEAMED(aGRNoteL)) return True;
		}
	}

	return False;
}


/* ---------------------------------------------------------------------------- Rebeam -- */
/* Rebeam the notes/rests beamed by <beamL>. Handles both grace and regular beams. */

LINK Rebeam(Document *doc, LINK beamL)
{
	short nInBeam, voice, voiceRole;
	LINK firstSyncL, lastSyncL;
	Boolean stemUp;

	nInBeam = LinkNENTRIES(beamL);
	firstSyncL = FirstInBeam(beamL);
	lastSyncL = LastInBeam(beamL);

	voice = BeamVOICE(beamL);
	if (GraceBEAM(beamL)) {
		RemoveGRBeam(doc, beamL, voice, False);
		return CreateGRBEAMSET(doc, firstSyncL, RightLINK(lastSyncL), voice,
							nInBeam, False, False);
	}

	/* If notes in the beam are involved in multivoice notation, preserve the beam's
		stem up or down characteristic. Otherwise, use the voice's current voiceRole,
		though that can change and it may not be desirable for this beam. FIXME:
		use single-voice rules! */
	if (IsNeighborhoodMultiVoice(firstSyncL, BeamSTAFF(beamL), voice)) {
		stemUp = IsBeamStemUp(beamL);
		voiceRole = (stemUp? VCROLE_UPPER : VCROLE_LOWER);  //??NO! USE SINGLE VOICE RULE//
	}
	else
		voiceRole = doc->voiceTab[voice].voiceRole;
	RemoveBeam(doc, beamL, voice, False);
	return CreateBEAMSET(doc, firstSyncL, RightLINK(lastSyncL), voice, nInBeam, False,
							voiceRole);
}


/* ------------------------------------------------------------------ GetBeamEndYStems -- */
/* Get the slant to use when creating a beamset, and find the positions to use for the
endpoints of stems beginning and ending the beam. Slope used is a percentage given by
config.relBeamSlope of the angle from ystem of firstInBeam to ystem of lastInBeam.
baseL is the bpSync whose ystem is the extreme stem in the direction of the beamset,
and which is the base for computing stem positions; ystem is the ystem it will have in
the beamset; *pFirstystem and *pLastystem are returned to be used to compute the stem
ends of all the notes in the beamset. NB: Uses the staff height of the first note's
staff. */

Boolean GetBeamEndYStems(
				Document *doc, short nInBeam,
				LINK bpSync[], LINK noteInSync[],
				LINK baseL, DDIST ystem,
				short voiceRole,			/* VCROLE_UPPER, etc. (see Multivoice.h) */
				Boolean stemUp,				/* True if up */
				DDIST *pFirstystem,			/* Output */
				DDIST *pLastystem			/* Output */
				)
{
	DDIST endDiff, firstystem1, lastystem1, baseLength, beamLength, yd;
	short staff;
	FASTFLOAT fEndDiff, fSlope, fBaseScale, fBaseOffset;
	CONTEXT context;
	char subType, stemLen;
	
	staff = NoteSTAFF(noteInSync[0]);
	GetContext(doc, bpSync[0], staff, &context);
	yd = NoteYD(noteInSync[0]); subType = NoteType(noteInSync[0]);
	stemLen = (voiceRole==VCROLE_SINGLE? config.stemLenNormal : config.stemLen2v);
	firstystem1 = CalcYStem(doc, yd, NFLAGS(subType), !stemUp,
											context.staffHeight, context.staffLines,
					  						stemLen, False);

	yd = NoteYD(noteInSync[nInBeam-1]); subType = NoteType(noteInSync[nInBeam-1]);
	lastystem1 = CalcYStem(doc, yd, NFLAGS(subType), !stemUp,
											context.staffHeight, context.staffLines,
					  						stemLen, False);

	endDiff = firstystem1 - lastystem1;
	fEndDiff = endDiff;
	fSlope = fEndDiff*config.relBeamSlope/100.0;
	
	beamLength = SysRelxd(bpSync[nInBeam-1]) - SysRelxd(bpSync[0]);
	if (beamLength==0) {
		MayErrMsg("GetBeamSlant: can't find slope of beamset of zero length");
		return False;
	}
	baseLength = SysRelxd(baseL) - SysRelxd(bpSync[0]);
	fBaseScale = (FASTFLOAT)baseLength / beamLength;
	fBaseOffset = fSlope * fBaseScale;
	
	*pFirstystem = ystem + (DDIST)fBaseOffset;
	*pLastystem = *pFirstystem - (DDIST)fSlope;

	return True;
}


/* ---------------------------------------------------- TopStaff2Staff, Staff2TopStaff -- */

/* Given yd relative to the top staff for something in a cross-staff object, get
yd relative to the given staff. If the staff is the top staff of the stfRange,
return yd unchanged; if it's the bottom staff, correct for the position of its
staff relative to the top staff. */

static DDIST TopStaff2Staff(DDIST yd,
						short staffn,
						STFRANGE theRange,
						DDIST heightDiff	/* (Positive) difference between staffTops of the two staves */
						)
{
	if (staffn==theRange.topStaff)	return yd;
	else							return (yd-heightDiff);
}

/* Given yd relative its own staff for something in a cross-staff object, get
yd relative to the top staff. If the staff is the top staff of the stfRange,
return yd unchanged; if it's the bottom staff, correct for the position of its
staff relative to the top staff. */

DDIST Staff2TopStaff(DDIST yd, short staffn,
						STFRANGE theRange,
						DDIST heightDiff	/* (Positive) difference between staffTops of the two staves */
						)
{
	if (staffn==theRange.topStaff)
			return yd;
	else	return (yd+heightDiff);
}


/* ------------------------------------------------------------------ FixSyncInBeamset -- */
/* Given a voice in a beamed Sync, fix stem end coordinates and (if the voice has
a chord) stem sides and accidental positions for the note(s). If the voice has a
chord, set all its stems to zero length except the MainNote's. */

static void FixSyncInBeamset(Document *doc, LINK bpSyncL, short voice,
							 DDIST ystem, STFRANGE theRange, Boolean crossStaff)
{
	LINK mainNoteL;
	Boolean stemDown;
	CONTEXT topContext, bottomContext;
	
	if (crossStaff) {
		mainNoteL = FindMainNote(bpSyncL, voice);
		stemDown = (NoteSTAFF(mainNoteL)==theRange.topStaff);
		GetContext(doc, bpSyncL, theRange.topStaff, &topContext);
		GetContext(doc, bpSyncL, theRange.bottomStaff, &bottomContext);
		ystem = TopStaff2Staff(ystem, NoteSTAFF(mainNoteL), theRange, 
					bottomContext.staffTop-topContext.staffTop);
	}
	else {
		mainNoteL = FindMainNote(bpSyncL, voice);
		stemDown = (NoteYD(mainNoteL) < ystem);
	}
	
	if (NoteINCHORD(mainNoteL))
		FixChordForYStem(bpSyncL, voice, (stemDown? -1 : 1), ystem);
	else
		NoteYSTEM(mainNoteL) = ystem;
}


/* --------------------------------------------------------------------- GetBeamNotes -- */
/* Fill in an array of notes/chords contained in the given beamset. */

void GetBeamNotes(LINK beamL, LINK notes[])
{
	LINK aNoteBeamL, syncL, aNoteL;
	short i, voice;

	voice = BeamVOICE(beamL);
	aNoteBeamL = FirstSubLINK(beamL);

	for (i=0; aNoteBeamL; i++, aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
		syncL = NoteBeamBPSYNC(aNoteBeamL);
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==voice) {
				notes[i] = aNoteL;
				break;
			}
	}
}


/* ----------------------------------------------------------- CreateBEAMSET utilities -- */
/* Functions used by CreateBEAMSET. */

#define PROG_ERR 	-3		/* Code for program error */
#define USR_ALERT -2		/* Code for user alert */
#define F_ABORT	-1			/* Code for function abort */
#define NF_OK		1		/* Code for successful completion */
#define BAD_CODE(returnCode) ((returnCode)<0) 

static Boolean CheckBeamStaves(LINK, LINK, short);
static Boolean SetBeamXStf(LINK, short, LINK[], STFRANGE *);
static DDIST Extend1Stem(Boolean, short, short, short, DDIST, DDIST);
static void FillHorizBeam(Document *, LINK, short, short, LINK[], LINK[], DDIST,
								STFRANGE, Boolean);

/* Add the beamset to the data structure and initialize the appropriate fields. */

LINK AddNewBeam(Document *doc, LINK startL, short voice,
				short nInBeam, short *staff,
				Boolean needSel,					/* True=beaming only selected notes */
				Boolean leaveSel)					/* True=leave the new Beamset selected */
{
	SearchParam pbSearch; LINK beamL; PBEAMSET beamp;

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = needSel;
	pbSearch.inSystem = False;
	startL = L_Search(startL, SYNCtype, GO_RIGHT, &pbSearch); 	/* Start at 1st Sync in range */
	*staff = NoteSTAFF(pbSearch.pEntry);						/* Pick up beamset's staff no. */

	beamL = InsertNode(doc, startL, BEAMSETtype, nInBeam);
	if (beamL) {
		SetObject(beamL, 0, 0, False, True, False);
		beamp = GetPBEAMSET(beamL);
		beamp->tweaked = False;
		beamp->staffn = *staff;
		beamp->voice = voice;
		if (leaveSel) beamp->selected = True;
		beamp->feather = beamp->thin = 0;
		beamp->grace = False;
		beamp->beamRests = doc->beamRests;
		
		doc->changed = True;
	}
	else
		NoMoreMemory();
	
	return beamL;
}


/* Check whether a Beamset in the given range and voice covers a legal range of
staves, i.e., the top and bottom staves are adjacent or identical. Does not
check that the staves are in the same part, since if they're not, they can't
have notes in the same voice. */
 
static Boolean CheckBeamStaves(LINK startL, LINK endL, short voice)
{
	LINK pL, aNoteL; short minStaff=999, maxStaff=-999;

	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = NoteInVoice(pL, voice, False);
			if (aNoteL) {
				minStaff = n_min(minStaff, NoteSTAFF(aNoteL));
				maxStaff = n_max(maxStaff, NoteSTAFF(aNoteL));
			}
		}
	
	if (maxStaff>minStaff+1) {
			GetIndCString(strBuf, BEAMERRS_STRS, 1);		/* "Nightingale cannot beam notes on non-adjacent staves." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return False;
		}

	return True;
}


/* If the beamset is cross-staff, set its crossStaff flag and return (in <theRange>)
the range of staves. */

Boolean SetBeamXStf(LINK beamL, short bel, LINK noteInSync[], STFRANGE *theRange)
{
	Boolean crossStaff;

	crossStaff = GetCrossStaff(bel, noteInSync, theRange);
	if (crossStaff) {
		BeamSTAFF(beamL) = theRange->topStaff;
		BeamCrossSTAFF(beamL) = True;
	}
	else
		BeamCrossSTAFF(beamL) = False;

	return crossStaff;
}


/* ----------------------------------------------------------------------- Extend1Stem -- */

static DDIST Extend1Stem(Boolean stemDown, short nPrimary, short nSecsA, short nSecsB,
													DDIST beamThick, DDIST flagYDelta)
{
	short nLevels;  DDIST extension;
	
	if (stemDown) {
		nLevels = nSecsB;								/* Secondaries above have no effect */
		extension = nLevels*flagYDelta;
	}
	else {
		nLevels = nPrimary-1;
		nLevels += nSecsA;								/* Secondaries below have no effect */
		extension = -(beamThick+nLevels*flagYDelta);
	}
	
	return extension;
}


/* ----------------------------------------------------------------------- ExtendStems -- */
/* Extend every stem in the given beamset by the appropriate distance so they
extend to all secondary beams:
	for a downstem, if it has secondary beams below the primaries, extend it to
		the bottom secondary;
	for an upstem, if it has secondary beams above the primaries, to the top of
	the top secondary beam; otherwise, to the top of its top primary beam.

Intended for use with center beams, since they require downstems and upstems to
overlap. */

void ExtendStems(Document *doc, LINK beamL, short nInBeam,
						short nPrimary, short nSecsA[], short nSecsB[])
{
	CONTEXT context;  DDIST beamThick, flagYDelta, extension;
	short voice, iel;  Boolean stemDown;
	LINK aNoteBeamL, syncL, aNoteL;  PBEAMSET pB;
	
	GetContext(doc, beamL, BeamSTAFF(beamL), &context);
	beamThick = LNSPACE(&context)/2;
	pB = GetPBEAMSET(beamL);
	if (pB->thin) beamThick /= 2;
	flagYDelta = FlagLeading(LNSPACE(&context));
	voice = BeamVOICE(beamL);

	aNoteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<nInBeam; iel++, aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
		syncL = NoteBeamBPSYNC(aNoteBeamL);
		aNoteL = FindMainNote(syncL, voice);
		
		stemDown = NoteYSTEM(aNoteL)>NoteYD(aNoteL);
		extension = Extend1Stem(stemDown, nPrimary, nSecsA[iel], nSecsB[iel],
											beamThick, flagYDelta);
		NoteYSTEM(aNoteL) += extension;
	}
}


/* ---------------------------------------------------------------- SlantBeamNoteStems -- */
/* Set each note/chord's stem to end at the beam, which extends diagonally from
height <firstystem> to <lastystem>. */

void SlantBeamNoteStems(Document *doc, LINK bpSync[],
						short voice, short nInBeam,
						DDIST firstystem, DDIST lastystem,		/* Relative to BeamSTAFF(beamL) */
						STFRANGE theRange,
						Boolean crossStaff
						)
{
	short iel;
	DDIST ystem, endDiff, beamLength, baseLength;
	FASTFLOAT fBaseOffset, fBaseScale, fEndDiff;

	endDiff = lastystem - firstystem;
	fEndDiff = endDiff;
	for (iel = 0; iel<nInBeam; iel++) {
		beamLength = SysRelxd(bpSync[nInBeam-1]) - SysRelxd(bpSync[0]);
		baseLength = SysRelxd(bpSync[iel]) - SysRelxd(bpSync[0]);
		
		fBaseScale = (FASTFLOAT)baseLength / beamLength;
		fBaseOffset = fEndDiff * fBaseScale;
		ystem = firstystem + (DDIST)fBaseOffset;

		FixSyncInBeamset(doc, bpSync[iel], voice, ystem, theRange, crossStaff);
	}
}
	

/* --------------------------------------------------------------------- FillSlantBeam -- */
/* Fill fields for a slanted beam, both in the BEAMSET and in the notes of the
beam (their ystems).

N.B. Where fractional beams occur, if there are no secondary beams to force their
direction, this version simply points them to the right if they're on the first
note of the beamset, else to the left. See Byrd's dissertation, Sec. 4.4.2, for
a much better, rhythm-based method. */
 
void FillSlantBeam(Document *doc, LINK beamL, short voice, short nInBeam,
					LINK bpSync[], LINK noteInSync[],
					DDIST firstystem, DDIST lastystem,		/* Relative to BeamSTAFF(beamL) */
					STFRANGE theRange,
					Boolean crossStaff
					)
{
	short iel, delt_left, delt_right, nPrimary, nSecsA[MAXINBEAM], nSecsB[MAXINBEAM];
	SignedByte stemUpDown[MAXINBEAM];
	LINK aNoteL, aNoteBeamL, leftNoteL, rightNoteL;

	/* For each note except those on the ends, set delt_left to the change in the no.
	 *	of beams from the note to the left to this one; likewise for delt_right.  If
	 * both are positive, fractional beams are required.  Also, if one is greater than
	 * the other, this note shares secondary beam(s) with that one...more-or-less.
	 */

	aNoteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<nInBeam; iel++, aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
		aNoteL = noteInSync[iel];								/* Ptr to note in SYNC */
		NoteBeamBPSYNC(aNoteBeamL) = bpSync[iel];
		NoteBeamFILLER(aNoteBeamL) = 0;

		if (iel==0)
			delt_left = NFLAGS(NoteType(aNoteL));
		else {
			leftNoteL = noteInSync[iel-1];			
			/* leftNote = GetPANOTE(leftNoteL); */
			delt_left = NFLAGS(NoteType(aNoteL))-NFLAGS(NoteType(leftNoteL));
		}

		if (iel==nInBeam-1)
			delt_right = NFLAGS(NoteType(aNoteL));
		else {
			rightNoteL = noteInSync[iel+1];			
			/* rightNote = GetPANOTE(rightNoteL); */
			delt_right = NFLAGS(NoteType(aNoteL))-NFLAGS(NoteType(rightNoteL));
		}

		if (delt_left>0 && delt_right>0)	{					/* Need frac. beam(s)? */
			NoteBeamFRACS(aNoteBeamL) = n_min(delt_left, delt_right);
			delt_left  -= NoteBeamFRACS(aNoteBeamL);
			delt_right -= NoteBeamFRACS(aNoteBeamL);		
		}
		else 
			NoteBeamFRACS(aNoteBeamL) = 0;						/* No */

		if (delt_left>0) {										/* Secondary beams to right */
			NoteBeamSTARTEND(aNoteBeamL) = delt_left;
			NoteBeamFRACGOLEFT(aNoteBeamL) = False;
		}
		else if (delt_right>0) {								/* Secondary beams to left */
			NoteBeamSTARTEND(aNoteBeamL) = -delt_right;
			NoteBeamFRACGOLEFT(aNoteBeamL) = True;
		}
		else {													/* No secondary beams */
			NoteBeamSTARTEND(aNoteBeamL) = 0;
			NoteBeamFRACGOLEFT(aNoteBeamL) = (iel>0);
		}
	}
	
	SlantBeamNoteStems(doc, bpSync, voice, nInBeam, firstystem, lastystem, theRange,
								crossStaff);
	
	/* If stems aren't all pointing in the same direction, DrawBEAMSET will draw the
	 *	beam above the ends of "normal" upward stems, so extend the stems. This will
	 *	work only if DrawBEAMSET knows that the stems are extended in this case!
	 */
	if (AnalyzeBeamset(beamL, &nPrimary, nSecsA, nSecsB, stemUpDown)==0) 
		ExtendStems(doc, beamL, nInBeam, nPrimary, nSecsA, nSecsB);
}


/* --------------------------------------------------------------------- FillHorizBeam -- */
/* Fill fields for a horizontal beam, both in the BEAMSET and in the notes of the
beam (their ystems). Set each note/chord's stem to end at the beam, which is
horizontal at height <ystem>.

N.B. Where fractional beams occur, if there are no secondary beams to force their
direction, this version simply points them to the right if they're on the first
note of the beamset, else to the left. See Byrd's dissertation, Sec. 4.4.2, for
a much better, rhythm-based method. */
 
static void FillHorizBeam(Document *doc, LINK beamL, short voice, short nInBeam,
							LINK bpSync[], LINK noteInSync[],
							DDIST ystem,
							STFRANGE theRange,
							Boolean crossStaff
							)
{
	short iel, delt_left, delt_right, nPrimary, nSecsA[MAXINBEAM], nSecsB[MAXINBEAM];
	SignedByte stemUpDown[MAXINBEAM];
	LINK aNoteL, aNoteBeamL, leftNoteL, rightNoteL;

	/* For each note except those on the ends, set delt_left to the change in the no.
	 *	of beams from the note to the left to this one; likewise for delt_right.  If
	 * both are positive, fractional beams are required.  Also, if one is greater than
	 * the other, this note shares secondary beam(s) with that one...more-or-less.
	 */

	aNoteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<nInBeam; iel++, aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
		aNoteL = noteInSync[iel];								/* Ptr to note in SYNC */
		NoteBeamBPSYNC(aNoteBeamL) = bpSync[iel];
		NoteBeamFILLER(aNoteBeamL) = 0;

		if (iel==0)
			delt_left = NFLAGS(NoteType(aNoteL));
		else {
			leftNoteL = noteInSync[iel-1];			
			/* leftNote = GetPANOTE(leftNoteL); */	
			delt_left = NFLAGS(NoteType(aNoteL))-NFLAGS(NoteType(leftNoteL));
		}

		if (iel==nInBeam-1)
			delt_right = NFLAGS(NoteType(aNoteL));
		else {
			rightNoteL = noteInSync[iel+1];			
			/* rightNote = GetPANOTE(rightNoteL); */	
			delt_right = NFLAGS(NoteType(aNoteL))-NFLAGS(NoteType(rightNoteL));
		}

		if (delt_left>0 && delt_right>0)	{					/* Need frac. beam(s)? */
			NoteBeamFRACS(aNoteBeamL) = n_min(delt_left, delt_right);
			delt_left  -= NoteBeamFRACS(aNoteBeamL);
			delt_right -= NoteBeamFRACS(aNoteBeamL);		
		}
		else 
			NoteBeamFRACS(aNoteBeamL) = 0;						/* No */

		if (delt_left>0) {										/* Secondary beams to right */
			NoteBeamSTARTEND(aNoteBeamL) = delt_left;
			NoteBeamFRACGOLEFT(aNoteBeamL) = False;
		}
		else if (delt_right>0) {								/* Secondary beams to left */
			NoteBeamSTARTEND(aNoteBeamL) = -delt_right;
			NoteBeamFRACGOLEFT(aNoteBeamL) = True;
		}
		else {													/* No secondary beams */
			NoteBeamSTARTEND(aNoteBeamL) = 0;
			NoteBeamFRACGOLEFT(aNoteBeamL) = (iel>0);
		}
	}

	for (iel = 0; iel<nInBeam; iel++)
		FixSyncInBeamset(doc, bpSync[iel], voice, ystem, theRange, crossStaff);

	/* If stems aren't all pointing in the same direction, DrawBEAMSET will draw the
	 *	beam above the ends of "normal" upward stems, so extend the stems. This will
	 *	work only if DrawBEAMSET knows that the stems are extended in this case!
	 */
	if (AnalyzeBeamset(beamL, &nPrimary, nSecsA, nSecsB, stemUpDown)==0) 
		ExtendStems(doc, beamL, nInBeam, nPrimary, nSecsA, nSecsB);
}


/* ----------------------------------------------------------------- CreateXSysBEAMSET -- */
/* If the range to be beamed spans two systems, create a cross system beamset with
a piece in the first system and one in the second. */

LINK CreateXSysBEAMSET(Document *doc, LINK startL, LINK endL, short voice,
						short nInBeam1, short nInBeam2, Boolean needSel, Boolean doBeam)
{
	short staff, stfLines;
	DDIST ystem, stfHeight;  PBEAMSET pBeam;
	LINK sysL, beamL1, beamL2, staffL, aStaffL, baseL;
	LINK bpSync[MAXINBEAM], noteInSync[MAXINBEAM];
	STFRANGE theRange;  Boolean crossStaff, stemUp;
	
	sysL = LSSearch(startL, SYSTEMtype, ANYONE, False, False);
	
	beamL1 = AddNewBeam(doc, startL, voice, nInBeam1, &staff, needSel, doBeam);
	pBeam = GetPBEAMSET(beamL1);
	pBeam->crossSystem = True;
	pBeam->firstSystem = True;
	if (!GetBeamSyncs(doc, startL, sysL, voice, nInBeam1, bpSync, noteInSync, needSel)) 
			return NILINK;

	/* Now create and initialize the beamset in the second system. */
	crossStaff = SetBeamXStf(beamL1, nInBeam1, noteInSync, &theRange);

	/* Get the y level of the first beam set and set the note stems of the beamset's notes. */
	staffL = LSSearch(startL, STAFFtype, staff, True, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		if (StaffSTAFF(aStaffL) == staff) {
			stfHeight = StaffHEIGHT(aStaffL);
			stfLines = StaffSTAFFLINES(aStaffL);
		}
	} 
	ystem = CalcBeamYLevel(doc, nInBeam1, bpSync, noteInSync, &baseL, stfHeight, stfLines,
									crossStaff, doc->voiceTab[voice].voiceRole, &stemUp);
	FillHorizBeam(doc, beamL1, voice, nInBeam1, bpSync, noteInSync, ystem,
									theRange, crossStaff);

	beamL2 = AddNewBeam(doc, sysL, voice, nInBeam2, &staff, needSel, doBeam);
	pBeam = GetPBEAMSET(beamL2);
	pBeam->crossSystem = True;
	pBeam->firstSystem = False;
	if (!GetBeamSyncs(doc, sysL, endL, voice, nInBeam2, bpSync, noteInSync, needSel))
			return NILINK;

	crossStaff = SetBeamXStf(beamL2, nInBeam2, noteInSync, &theRange);
	
	/* Get the y level of the second beam set and set the note stems of the beamset's notes. */
	staffL = LSSearch(startL, STAFFtype, staff, True, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		if (StaffSTAFF(aStaffL) == staff) {
			stfHeight = StaffHEIGHT(aStaffL);
			stfLines = StaffSTAFFLINES(aStaffL);
		}
	} 
	ystem = CalcBeamYLevel(doc, nInBeam2, bpSync, noteInSync, &baseL, stfHeight, stfLines,
										crossStaff, doc->voiceTab[voice].voiceRole, &stemUp);
	FillHorizBeam(doc, beamL2, voice, nInBeam2, bpSync, noteInSync, ystem, theRange,
									crossStaff);

	InvalMeasures(startL, endL, staff);						/* Force redrawing all affected measures */
	return beamL1;
}


/* Check whether the beamset described is legal. Eventually, we should allow cross-
system beam pieces of only one note, but for now, every beamset must contain at
least two notes. */

short CheckBeamVars(short nInBeam, Boolean crossSystem, short nBeamable1, short nBeamable2)
{
	char fmtStr[256];

	if (nInBeam>MAXINBEAM) {
		GetIndCString(fmtStr, BEAMERRS_STRS, 2);		/* "The maximum number of notes/chords in a beam is %d." */
		sprintf(strBuf, fmtStr, MAXINBEAM);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return USR_ALERT;
	}
	if (nInBeam<2) {
		MayErrMsg("CheckBeamVars: can't make beam of only %ld notes.", (long)nInBeam);
		return PROG_ERR;
	}
	if (crossSystem && (nBeamable1<2 || nBeamable2<2)) {
		MayErrMsg("CheckBeamVars: can't make cross-system beam with segments of only %ld and %ld notes.",
					(long)nBeamable1, (long)nBeamable2);
		return PROG_ERR;
	}
	return NF_OK;
}


/* -------------------------------------------------------------- CreateNonXSysBEAMSET -- */

LINK CreateNonXSysBEAMSET(Document *doc, LINK startL, LINK endL, short voice,
							short nInBeam, Boolean needSel, Boolean doBeam,
							short voiceRole			/* VCROLE_UPPER, etc. (see Multivoice.h) */
							)
{
	short		staff;
	LINK		beamL, baseL;
	LINK		bpSync[MAXINBEAM];
	LINK		noteInSync[MAXINBEAM];
	PBEAMSET	pBeam;
	DDIST		ystem, firstystem, lastystem;
	Boolean		crossStaff;
	Boolean		stemUp;
	STFRANGE	theRange;					/* Range of staves occupied by a cross-staff beam */
	CONTEXT		context;

	beamL = AddNewBeam(doc, startL, voice, nInBeam, &staff, needSel, doBeam);
	if (beamL) {
		pBeam = GetPBEAMSET(beamL);
		pBeam->crossSystem = False;
		pBeam->firstSystem = False;
		
		/* Fill in the arrays of sync and note LINKs for the benefit of beam subObjs. */
		if (!GetBeamSyncs(doc, startL, endL, voice, nInBeam, bpSync, noteInSync, needSel))
				return NILINK;
	
		/* Handle cross-staff conditions. */
		crossStaff = SetBeamXStf(beamL, nInBeam, noteInSync, &theRange);
	
		/* Get the y-level of the beamset and set the note stems of the beamset's notes.
		 * For simplicity, create cross-staff beamsets horizontal, others slanted.
		 */
		GetContext(doc, startL, staff, &context);
		ystem = CalcBeamYLevel(doc, nInBeam, bpSync, noteInSync, &baseL, context.staffHeight,
										context.staffLines, crossStaff, voiceRole, &stemUp);

		if (crossStaff)
			firstystem = lastystem = ystem;
		else
			(void)GetBeamEndYStems(doc, nInBeam, bpSync, noteInSync, baseL, ystem,
											voiceRole, stemUp, &firstystem, &lastystem);
		
		FillSlantBeam(doc, beamL, voice, nInBeam, bpSync, noteInSync, firstystem,
										lastystem, theRange, crossStaff);
	
		InvalMeasures(startL, endL, staff);						/* Force redrawing all affected measures */
	}
	
	return beamL;
}


/* --------------------------------------------------------------------- CreateBEAMSET -- */
/* Create a beamset, i.e., a primary beam with appropriate secondary and frac-
tional beams, for the specified range. The range must contain <nInBeam> notes 
(counting each chord as one note under the usual assumption that every chord has
exactly one MainNote), all of which  must be (possibly dotted) eighths or
shorter. The range may also contain any  number of rests, which will be ignored if
desired (cf. !beamRests), and any other non-notes, which will always be ignored.
If <needSel>, all notes/rests to be beamed must be in selected Syncs.

As of v. 5.7, Nightingale doesn't allow beams in a voice to skip Syncs in that
voice, so we don't need a <needSel> parameter: we just pass False to the lower-
level routines that expect <needSel>. */

LINK CreateBEAMSET(
		Document *doc,
		LINK startL,
		LINK endL,
		short voice,
		short nInBeam,
		Boolean	doBeam,	/* True if we're explicitly beaming notes (so Beamset will be selected) */
		short voiceRole	/* VCROLE_UPPER, etc. (see Multivoice.h) */
		)
{
	Boolean	crossSystem;
	short	returnCode, nBeamable1, nBeamable2;
	
	/* Decide whether it'll be cross-system and check the parameters. */

	if (!CheckBeamStaves(startL, endL, voice)) return NILINK;
	crossSystem = CountXSysBeamable(doc, startL, endL, voice, &nBeamable1, &nBeamable2,
									False);
	returnCode = CheckBeamVars(nInBeam, crossSystem, nBeamable1, nBeamable2);
	if (BAD_CODE(returnCode)) return NILINK;

	if (crossSystem)
		return CreateXSysBEAMSET(doc, startL, endL, voice, nBeamable1, nBeamable2,
											False, doBeam);
	else
		return CreateNonXSysBEAMSET(doc, startL, endL, voice, nInBeam, False, doBeam,
											voiceRole);
}


/* --------------------------------------------------------------------- UnbeamV et al -- */
/* Remove all beams in the specified range for the specified voice and set the
selection range. */
	
void UnbeamV(Document *doc, LINK fromL, LINK toL, short voice)
{
	LINK newSelStart, pL;

	if (BetweenIncl(fromL, doc->selStartL, toL) && ObjLType(doc->selStartL)==BEAMSETtype)
		newSelStart = FirstInBeam(doc->selStartL);
	else
		newSelStart = doc->selStartL;
	Unbeamx(doc, fromL, toL, voice);								/* Remove normal beams */
	GRUnbeamx(doc, fromL, toL, voice);								/* Remove grace beams */
	if (InObjectList(doc, newSelStart, MAIN_DSTR))
		doc->selStartL = newSelStart;
	else 
		for (pL=doc->headL; pL; pL = RightLINK(pL))
			if (LinkSEL(pL))
				{ doc->selStartL = pL; break; }

	BoundSelRange(doc);
}


/* Unbeamx removes all normal-note beams in the specified range for the specified voice.
If any beams cross the ends of the range, Unbeamx rebeams the portions outside of the
range, if that's possible. It should ordinarily be called only via Unbeam, not directly,
since it doesn't worry about leaving a legal selection range.

Unbeamx assumes the document is on the screen, so it Invals the measures containing the
beams. It may call InvalMeasures several times, which is slightly inefficient, but
easier than figuring out the range for a single call (and perhaps returning them to
the caller). */
	
static void Unbeamx(Document *doc, LINK fromL, LINK toL, short voice)
{
	LINK beamFromL, beamToL, lBeamL, rBeamL, newBeamL;
	short nInBeam;
	PBEAMSET lBeam, rBeam, newBeam;
	Boolean crossSys, firstSys;

	/* If any beams cross the endpoints of the range, remove them first and, if
	possible, rebeam the portion outside the range. */
	
	if ( (rBeamL = VHasBeamAcross(toL, voice)) != NILINK)			/* Get beam across right end. */
		beamToL = RightLINK(LastInBeam(rBeamL));
	if ( (lBeamL = VHasBeamAcross(fromL, voice)) != NILINK) {		/* Remove beam across left end. */
		lBeam = GetPBEAMSET(lBeamL);
		crossSys = lBeam->crossSystem;
		firstSys = lBeam->firstSystem;
		beamFromL = FirstInBeam(lBeamL);
		if (UnbeamRange(doc, beamFromL, RightLINK(LastInBeam(lBeamL)), voice))
			InvalMeasures(beamFromL, RightLINK(LastInBeam(lBeamL)), ANYONE);		/* Force redrawing all affected measures */
		if ( (nInBeam = CountBeamable(doc, beamFromL, fromL, voice, False)) >=2 ) {	/* Beam remainder */
			newBeamL = CreateBEAMSET(doc, beamFromL, fromL, voice, nInBeam, False,
												doc->voiceTab[voice].voiceRole);
			if (crossSys && !firstSys) {
				newBeam = GetPBEAMSET(newBeamL);
				newBeam->crossSystem = True;
				newBeam->firstSystem = False;
			}
					
		}
	}	
	if (rBeamL != NILINK) {											/* Remove beam across right end. */
		if (rBeamL!=lBeamL) {
			rBeam = GetPBEAMSET(lBeamL);
			crossSys = rBeam->crossSystem;
			firstSys = rBeam->firstSystem;
			if (UnbeamRange(doc, FirstInBeam(rBeamL), beamToL, voice))
				InvalMeasures(FirstInBeam(rBeamL), beamToL, ANYONE);			/* Force redrawing all affected measures */
		}
		if ( (nInBeam = CountBeamable(doc, toL, beamToL, voice, False)) >=2 ) {	/* Beam remainder */
			newBeamL = CreateBEAMSET(doc, toL, beamToL, voice, nInBeam, False,
												doc->voiceTab[voice].voiceRole);
			if (crossSys && firstSys) {
				newBeam = GetPBEAMSET(newBeamL);
				newBeam->crossSystem = newBeam->firstSystem = True;
			}
		}		
	}

	/* Main loop: unbeam every beamed note within the range. */
	
	if (UnbeamRange(doc, fromL, toL, voice))
		InvalMeasures(fromL, toL, ANYONE);								/* Force redrawing all affected measures */
}


/* Fix stems, head stem sides, etc. for the given unbeamed note or its chord. */
		
void FixNCStem(Document *doc, LINK syncL, LINK noteL, CONTEXT context)
{
	Boolean stemDown; QDIST qStemLen;
	
		if (NoteINCHORD(noteL))
			FixSyncForChord(doc, syncL, NoteVOICE(noteL), False, 0, 0, &context);
		else {
			stemDown = GetStemInfo(doc, syncL, noteL, &qStemLen);
			NoteYSTEM(noteL) = CalcYStem(doc, NoteYD(noteL), NFLAGS(NoteType(noteL)),
											stemDown, context.staffHeight,
											context.staffLines, qStemLen, False);
		}
}


/* Unbeam every beamed note within the range and voice. If we actually find anything
to do, return True. */

Boolean UnbeamRange(Document *doc, LINK fromL, LINK toL, short voice)
{
	LINK		pL, aNoteL, beamL, noteBeamL, syncL, bNoteL;
	SearchParam	pbSearch;
	Boolean		didAnything=False, needContext=True;
	CONTEXT		context;
	
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = False;
	pbSearch.inSystem = False;

	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				if (NoteVOICE(aNoteL)==voice && NoteBEAMED(aNoteL)) {
					pbSearch.subtype = NoteBeam;
					beamL = L_Search(pL, BEAMSETtype, GO_LEFT, &pbSearch);
					if (beamL==NILINK) {
						MayErrMsg("UnbeamRange: can't find Beamset for note in sync %ld voice %ld",
									(long)pL, (long)voice);
						return didAnything;
					}

					if (needContext) {
						GetContext(doc, beamL, BeamSTAFF(beamL), &context);
						needContext = False;
					}

					noteBeamL = FirstSubLINK(beamL);
					for ( ; noteBeamL; noteBeamL = NextNOTEBEAML(noteBeamL)) {
						/* pNoteBeam = GetPANOTEBEAM(noteBeamL); */
						syncL = NoteBeamBPSYNC(noteBeamL);
						bNoteL = FirstSubLINK(syncL);
						for ( ; bNoteL; bNoteL = NextNOTEL(bNoteL)) {
							if (NoteVOICE(bNoteL)==voice) {									/* This note in desired voice? */
								NoteBEAMED(bNoteL) = False;									/* Yes */
								if (MainNote(bNoteL))
									FixNCStem(doc, syncL, bNoteL, context);
							}
						}
					}
					DeleteNode(doc, beamL);												/* Delete beam */		
					didAnything = True;
				}
			}
		}
	}
	
	if (didAnything) doc->changed = True;
	return didAnything;
}


/* -------------------------------------------------------------------- BeamHasSelNote -- */
/* Does the given Beamset contain any selected notes? */

static Boolean BeamHasSelNote(LINK beamL)
{
	LINK	noteBeamL, syncL, aNoteL;
	short	voice;
	
	voice = BeamVOICE(beamL);
	noteBeamL = FirstSubLINK(beamL);
	for ( ; noteBeamL; noteBeamL = NextNOTEBEAML(noteBeamL)) {
		syncL = NoteBeamBPSYNC(noteBeamL);
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==voice && NoteSEL(aNoteL)) return True;
	}
	return False;
}


/* ------------------------------------------------------------------- FixBeamsOnStaff -- */
/* Remove and recreate all Beamsets in or partly in the range [startL, endL) on the
given staff. Handles both grace and regular Beamsets. */

void FixBeamsOnStaff(Document *doc,
					LINK startL, LINK endL,
					short staffn,
					Boolean needSelected		/* True if we only want selected items */
					)
{
	LINK	firstBeamL, lastBeamL, pL;
	
	firstBeamL = LSSearch(startL, BEAMSETtype, staffn, GO_LEFT, False);	/* 1st beam that might be affected */
	if (firstBeamL) {
		if (IsAfter(LastInBeam(firstBeamL), startL))					/* Is it really affected? */
			firstBeamL = startL;										/* No */
	}
	else
		 firstBeamL = startL;											/* No beams before range on staff */
	lastBeamL = LSSearch(endL, BEAMSETtype, staffn, GO_RIGHT, False);	/* 1st beam that's definitely NOT affected */

//LogPrintf(LOG_DEBUG, "FixBeamsOnStaff: startL=%u endL=%u staffn=%d firstBeamL=%u\n",
//			startL, endL, staffn, firstBeamL);
	for (pL = firstBeamL; pL!=lastBeamL; pL=RightLINK(pL))
		if (BeamsetTYPE(pL))
			if (BeamSTAFF(pL)==staffn && (!needSelected || BeamHasSelNote(pL)))
				Rebeam(doc, pL);
}


/* ------------------------------------------------------------------- FixBeamsInVoice -- */
/* Remove and recreate all Beamsets in or partly in the range [startL, endL) in the
given voice. Handles both grace and regular Beamsets. */

void FixBeamsInVoice(Document *doc,
					LINK startL, LINK endL,
					short voice,
					Boolean needSelected		/* True if we only want selected items */
					)
{
	LINK	firstBeamL, lastBeamL, pL;
	
	firstBeamL = LVSearch(startL, BEAMSETtype, voice, GO_LEFT, False);	/* 1st beam that might be affected */
	if (firstBeamL) {
		if (IsAfter(LastInBeam(firstBeamL), startL))					/* Is it really affected? */
			firstBeamL = startL;										/* No */
	}
	else
		 firstBeamL = startL;											/* No beams before range on staff */
	lastBeamL = LVSearch(endL, BEAMSETtype, voice, GO_RIGHT, False);	/* 1st beam that's definitely NOT affected */

//LogPrintf(LOG_DEBUG, "FixBeamsInVoice: startL=%u endL=%u voice=%d firstBeamL=%u\n",
//			startL, endL, voice, firstBeamL);
	for (pL = firstBeamL; pL!=lastBeamL; pL=RightLINK(pL))
		if (BeamsetTYPE(pL))
			if (BeamVOICE(pL)==voice && (!needSelected || BeamHasSelNote(pL)))
				Rebeam(doc, pL);
}


/* -------------------------------------------------------------------- UnbeamFixNotes -- */
/* Fix stems, note stem sides, etc. of notes, rests, and grace notes in the given
range, presumably because they used to be beamed. N.B. Should probably call
FixNCStem instead of doing the fixing itself. */

static void UnbeamFixNotes(Document *doc, LINK vStartL, LINK vEndL, short voice)
{
	CONTEXT		context;
	LINK		pL, aNoteL, aGRNoteL;
	Boolean		stemDown;
	QDIST		qStemLen;
	
	for (pL = vStartL; pL!=vEndL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			aNoteL = FindMainNote(pL, voice);
			if (aNoteL) {
				GetContext(doc, vStartL, NoteSTAFF(aNoteL), &context);
				if (NoteBEAMED(aNoteL))
					MayErrMsg("UnbeamFixNotes: note in voice %ld in sync at %ld is still beamed.",
								(long)voice, (long)pL);
				else if (NoteINCHORD(aNoteL))
					FixSyncForChord(doc, pL, voice, False, 0, 0, NULL);
				else {
					stemDown = GetStemInfo(doc, pL, aNoteL, &qStemLen);
					NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL), NFLAGS(NoteType(aNoteL)),
													stemDown,
													context.staffHeight, context.staffLines,
													qStemLen, False);
				}
			}
		}
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FindGRMainNote(pL, voice);
			if (aGRNoteL) {
				GetContext(doc, vStartL, GRNoteSTAFF(aGRNoteL), &context);
				if (GRNoteBEAMED(aGRNoteL))
					MayErrMsg("UnbeamFixNotes: GRNote in voice %ld in GRSync at %ld is still beamed.",
								(long)voice, (long)pL);
				else if (GRNoteINCHORD(aGRNoteL))
					FixGRSyncForChord(doc, pL, voice, False, 0, 0, NULL);
				else
					FixGRSyncNote(doc, pL, voice, &context);
			}
		}

	}
}


/* ------------------------------------------------------------------------ RemoveBeam -- */
/* Truncated version of Unbeam which simply sets NoteBEAMED(aNoteL) False for all
notes in Beamset, deletes the Beamset, and optionally updates stems of notes/chords
in the Beamset. */

void RemoveBeam(Document *doc, LINK beamL, short voice, Boolean fixStems)
{
	LINK firstSyncL, lastSyncL, pL, aNoteL;

	firstSyncL = FirstInBeam(beamL);
	lastSyncL = LastInBeam(beamL);

	for (pL = firstSyncL; pL!=RightLINK(lastSyncL); pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				if (NoteVOICE(aNoteL)==voice) NoteBEAMED(aNoteL) = False;
			}
		}
	DeleteNode(doc, beamL);
	
	if (fixStems) UnbeamFixNotes(doc, firstSyncL, RightLINK(lastSyncL), voice);
}


/* -------------------------------------------------------------------- FlipFractional -- */
/* Given a Beamset <beamL> and a Sync <syncL> (or GRSync), find out if the beam
subobject whose bpSync is <syncL> has a fractional beam; if not, return
NOTHING_TO_DO. Otherwise, if this subobject either starts or ends a primary
or secondary beam (i.e., if startend!=0), return FAILURE. Otherwise, flip the
fractional beam and return OP_COMPLETE. Takes no user-interface actions. */

static short FlipFractional(LINK, LINK);
static short FlipFractional(LINK beamL, LINK syncL)
{
	register short	nInBeam, i;
	register LINK	aNoteBeamL;
	
	nInBeam = LinkNENTRIES(beamL);
	aNoteBeamL = FirstSubLINK(beamL);
	
	for (i=0; i<nInBeam; i++, aNoteBeamL = NextNOTEBEAML(aNoteBeamL)) {
		if (NoteBeamBPSYNC(aNoteBeamL)==syncL) {
			if (NoteBeamFRACS(aNoteBeamL)==0) return NOTHING_TO_DO;
			
			if (NoteBeamSTARTEND(aNoteBeamL)!=0) return FAILURE;
			
			NoteBeamFRACGOLEFT(aNoteBeamL) = !NoteBeamFRACGOLEFT(aNoteBeamL);
			return OP_COMPLETE;
		}
	}
	
	return NOTHING_TO_DO;
}


/* -------------------------------------------------------------- DoFlipFractionalBeam -- */
/* Try to flip fractional beams for every selected note/chord. Handles user
interface on the assumption the given document is on the screen. */

void DoFlipFractionalBeam(Document *doc)
{
	register LINK		pL, beamL;
	LINK				partL;
	short				v, userVoice;
	short				status;
	register PPARTINFO	pPart;
	char				vStr[20], partName[256];
	
	DisableUndo(doc, False);
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			for (v=1; v<=MAXVOICES; v++) {
				if (VOICE_MAYBE_USED(doc, v))
					if (NoteInVoice(pL, v, True)) {
						beamL = LVSearch(pL, BEAMSETtype, v, GO_LEFT, False);
						if (beamL) {
							status = FlipFractional(beamL, pL);
							if (status==OP_COMPLETE) {
								doc->changed = True;
								InvalMeasures(pL, pL, BeamSTAFF(beamL));
							}
							else if (status==FAILURE) {
								if (Int2UserVoice(doc, v, &userVoice, &partL)) {
									sprintf(vStr, "%d", userVoice);
									pPart = GetPPARTINFO(partL);
									strcpy(partName, (strlen(pPart->name)>14? pPart->shortName : pPart->name));
								}
								GetIndCString(strBuf, BEAMERRS_STRS, 4);		/* " it would point outside of its group." */
								CParamText(vStr, partName, strBuf, "");
								if (CautionAdvise(FLIPFRACBEAM_ALRT)==Cancel) return;
							}
						}
					}
			}
		}
}


/* =========================================================================== DRAWING == */

/* ------------------------------------------------------------------------- CalcXStem -- */
/* Given a Sync, voice number, stem direction, and standard notehead width,
return the page-relative x-coordinate of the given end of a beam for the
note/chord in that Sync and voice. */

DDIST CalcXStem(Document *doc, LINK syncL, short voice, short stemDir,
				DDIST measLeft, PCONTEXT pContext,
				Boolean /*left*/					/* True=left end of beam, else right */
				)
{
	DDIST	dHeadWidth, xd, xdNorm;
	LINK	aNoteL;
	short	headWidth, xpStem;
	DDIST	lnSpace = LNSPACE(pContext);
	
	aNoteL = FindMainNote(syncL, voice);
	if (!aNoteL) return 0;
	
	xd = NoteXLoc(syncL, aNoteL, measLeft, HeadWidth(LNSPACE(pContext)), &xdNorm);
	
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xpStem = pContext->paper.left+d2p(xd);
			if (stemDir>0) {
				headWidth = MusFontStemSpaceWidthPixels(doc, doc->musFontInfoIndex, lnSpace);
				if (NoteSMALL(aNoteL)) headWidth = SMALLSIZE(headWidth);
				xpStem += headWidth;
			}
			xd = p2d(xpStem-pContext->paper.left);
			break;

		case toPostScript:
			if (stemDir>0) {
				dHeadWidth = HeadWidth(LNSPACE(pContext))+pt2d(1)/2;
				if (NoteSMALL(aNoteL)) dHeadWidth = SMALLSIZE(dHeadWidth);
				xd += dHeadWidth;
			}
			break;
		default:
			;
	}
		
	return xd;
}


/* --------------------------------------------------------------------- NoteXStfYStem -- */
/* Return a ystem value for <aNoteL>, including an offset if the note is in
a cross-staff beamset and is not on the top staff of the range. */

DDIST NoteXStfYStem(LINK aNoteL, STFRANGE stfRange, DDIST firstStfTop, DDIST lastStfTop,
							Boolean crossStaff)
{
	DDIST yBStem;

	if (crossStaff)
		yBStem = Staff2TopStaff(NoteYSTEM(aNoteL),NoteSTAFF(aNoteL),stfRange,
											lastStfTop-firstStfTop);
	else
		yBStem = NoteYSTEM(aNoteL);

	return yBStem;
}


/* -------------------------------------------------------------------- AnalyzeBeamset -- */
/* Get information about the given BEAMSET. Return (in *nPrimary) the number of
primary beams; (in nSecsA[i]) the number of secondary beams in progress and above
primaries at the i'th note of the beamset; (in nSecsB[i]) the number of secondary
beams in progress and below primaries at the i'th note of the beamset. Finally,
return as function value the direction of stems in the BEAMSET: -1=all down, +1=all
up, 0=in both directions, i.e., it's a center beam. */

#define ANTI_CORNER True

short AnalyzeBeamset(LINK beamL, short *nPrimary,
							short nSecsA[], short nSecsB[],		/* Outputs */
							SignedByte stemUpDown[])			/* Output */
{
	short nSStart[MAXINBEAM], nSEnd[MAXINBEAM];
	LINK noteBeamL, syncL, aNoteL;
	register short iel;
	short startend, firstStemUpDown, nestLevel, nPrim, i, n;
	short stemsWay, iStart;
	Boolean stemUp;
	
	/* Fill in a table of stem directions for all elements of the beam. */
	
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		/* pNoteBeam = GetPANOTEBEAM(noteBeamL); */
		syncL = NoteBeamBPSYNC(noteBeamL);
		aNoteL = FindMainNote(syncL, BeamVOICE(beamL));
		stemUpDown[iel] = ( (NoteYSTEM(aNoteL) < NoteYD(aNoteL)) ? 1 : -1 );
	}

	/* Decide <stemsWay>, and compute the number of primary beams. */
	
	stemsWay = firstStemUpDown = stemUpDown[0];
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		/* pNoteBeam = GetPANOTEBEAM(noteBeamL); */	
		startend = NoteBeamSTARTEND(noteBeamL);
		if (iel==0)
			nPrim = nestLevel = startend;
		else {
			if (stemUpDown[iel]!=firstStemUpDown) stemsWay = 0;
			if (iel<LinkNENTRIES(beamL)-1) {
				nestLevel += startend;
				if (nestLevel<nPrim) nPrim = nestLevel;
			}
		}
	}

	/* At each element, figure out how many secondary beams start and end. */
		
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		/* pNoteBeam = GetPANOTEBEAM(noteBeamL); */
		nSStart[iel] = nSEnd[iel] = 0;
		if (NoteBeamSTARTEND(noteBeamL)>0) nSStart[iel] = NoteBeamSTARTEND(noteBeamL);
		if (iel==0) nSStart[iel] -= nPrim;
		
		if (NoteBeamSTARTEND(noteBeamL)<0) nSEnd[iel] = -NoteBeamSTARTEND(noteBeamL);
		if (iel==LinkNENTRIES(beamL)-1) nSEnd[iel] -= nPrim;
	}

	/* Decide how many secondaries at each element should go above and how many below
		the primaries. To decide, use the stem direction of the last note in each
		secondary, unless the secondary goes to the end of the entire beamset and
		ANTI_CORNER is on; in that case, use the stem direction of the first note in
		the secondary. This should avoid unnecessary corners in all cases, but if it
		causes problems, ANTI_CORNER can be turned off. */
	
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL))
		nSecsA[iel] = nSecsB[iel] = 0;
	
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		if (nSEnd[iel]>0) {
			for (n = 0; n<nSEnd[iel]; n++) {
				for (i = iel; i>0 && nSStart[i]==0; i--)
					;
				nSStart[i]--;									/* This one's been taken care of */
				iStart = i;
				if (iel==LinkNENTRIES(beamL)-1 && ANTI_CORNER)
					stemUp = (stemUpDown[iStart]>0);
				else
					stemUp = (stemUpDown[iel]>0);
	
	
				for (i = iel; i>=iStart; i--) {
					if (stemUp)	nSecsB[i]++;
					else		nSecsA[i]++;
				}
			}
		}
	}
	
	*nPrimary = nPrim;
	return stemsWay;
}


#define BEAMTABLEN 300	/* Should be enough: even 127 alternate 128ths & 8ths need only 257 */

/* ---------------------------------------------------------------- BuildBeamDrawTable -- */
/* Given a Beamset and information about it, normally compiled by AnalyzeBeamset,
build a table of beam segments to be drawn by DrawBEAMSET or by the beam-dragging
feedback routine. Returns -1 if there's a problem, else the number of segments to
draw. */

short BuildBeamDrawTable(
				LINK beamL,
				short whichWay,				/* -1=all stems down, +1=all up, 0=in both directions */
				short nPrimary, short nSecsA[], short nSecsB[],
				SignedByte stemUpDown[],
				BEAMINFO beamTab[],			/* Output */
				short tabLen				/* Max. size of beamTab */
				)
{
	LINK noteBeamL, syncL, noteL;
	short startend[MAXINBEAM], nFrac[MAXINBEAM];
	Boolean fracGoLeft; char level[BEAMTABLEN];
	short i, k, direction, curLevel, minUsedLev, maxUsedLev, prevNSecsA, prevNSecsB;
	short iel, count;

	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		/* pNoteBeam = GetPANOTEBEAM(noteBeamL); */
		startend[iel] = NoteBeamSTARTEND(noteBeamL);
		nFrac[iel] = NoteBeamFRACS(noteBeamL);
	}
	
	for (i = 0; i<tabLen; i++)
		beamTab[i].stop = -1;							/* Initialize all segments to unfinished */
		
	direction = (whichWay? whichWay : -1);
	
	/* First put primary beams into the table of beam segments (BEAMINFOs). */
	
	curLevel = -direction;
	for (count = 0; count<nPrimary; count++) {
		curLevel += direction;
		beamTab[count].start = 0;
		level[count] = curLevel;
	}
	startend[0] -= nPrimary;
	if (direction>0)	{ minUsedLev = 0; maxUsedLev = nPrimary-1; }
	else				{ maxUsedLev = 0; minUsedLev = -(nPrimary-1); }
	
	/*
	 * Build the rest of the table by combining the beam stacking information computed
	 * by AnalyzeBeamset with the three commands that are stored in NOTEBEAMs, namely
	 * "start non-fractional beams", "do fractional beams", and "end non-fractional
	 * beams". The commands are processed in that order. Fractional beams require no
	 * additional information, so their BEAMINFOs are completely filled in as soon as
	 * they're encountered.
	 */

	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		if (startend[iel]>0) {									/* Start new BEAMINFOs */
			prevNSecsA = (iel==0? 0 : nSecsA[iel-1]);
			prevNSecsB = (iel==0? 0 : nSecsB[iel-1]);
			/* If neither above nor below has an increase, we must be starting new
			 *	secondaries after a secondary beam break, and <direction> is as of the
			 * the last starting, so it's already correct.
			 */
			if (nSecsA[iel]>prevNSecsA) direction = -1;
			else if (nSecsB[iel]>prevNSecsB) direction = 1;
			for (i = 0; i<startend[iel]; i++) {
				curLevel = (direction>0? ++maxUsedLev : --minUsedLev);
				beamTab[count].start = iel;
				level[count] = curLevel;
				count++;
				if (count>=tabLen) {
					MayErrMsg("BuildBeamDrawTable: table size exceeded.");
					return -1;
				}
			}
		}
		
		if (nFrac[iel]>0) {										/* Fill BEAMINFO for frac.beam */
			/* pNoteBeam = GetPANOTEBEAM(noteBeamL); */
			syncL = NoteBeamBPSYNC(noteBeamL);
			fracGoLeft = NoteBeamFRACGOLEFT(noteBeamL);
			noteL = FindMainNote(syncL, BeamVOICE(beamL));				
			direction = ( (NoteYSTEM(noteL) < NoteYD(noteL)) ? 1 : -1 );
			for (i = 0; i<nFrac[iel]; i++) {
				curLevel = (direction>0? maxUsedLev+(i+1) : minUsedLev-(i+1));
				beamTab[count].start = beamTab[count].stop = iel;
				level[count] = curLevel;
				beamTab[count].fracGoLeft = fracGoLeft;			
				count++;
			}
		}
		
		if (startend[iel]<0) {									/* Finish up existing BEAMINFOs */
			k = count-1;
			for (i = 0; i<-startend[iel]; i++) {
				for ( ; k>=-1; k--)
					if (beamTab[k].stop<0) break;
				if (k<0) {
					MayErrMsg("BuildBeamDrawTable: couldn't find an unfinished segment.");
					return -1;
				}
				if (level[k]>=0) maxUsedLev--;
				else minUsedLev++;
				beamTab[k].stop = iel;
				beamTab[k].fracGoLeft = 0;					/* Will be ignored */
			}
		}
	}

	/* At this point, beam levels are relative to the top primary beam. For ease of
			drawing, convert them to be relative to each stem end. */
			
		for (i = 0; i<count; i++) {
			beamTab[i].startLev = beamTab[i].stopLev = level[i];
			
			iel = beamTab[i].start;
			if (stemUpDown[iel]<0 && nSecsB[iel]>0)			beamTab[i].startLev -= nSecsB[iel];
			else if (stemUpDown[iel]>0 && nSecsA[iel]>0)	beamTab[i].startLev += nSecsA[iel];
			
			if (beamTab[iel].start!=beamTab[iel].stop) {
				iel = beamTab[i].stop;
				if (stemUpDown[iel]<0 && nSecsB[iel]>0)		beamTab[i].stopLev -= nSecsB[iel];
				else if (stemUpDown[iel]>0 && nSecsA[iel]>0) beamTab[i].stopLev += nSecsA[iel];
			}
		}

	for (i = tabLen-1; i>=0; i--)
		if (beamTab[i].stop>=0) return i+1;
	
	return -1;											/* Found no beam segments: something is wrong */
}


/* ----------------------------------------------------------------------- SetBeamRect -- */

void SetBeamRect(Rect *r, short x0, short y0, short x1, short y1,
				Boolean topEdge,		/* True=y0->y1 is top edge of Rect, else bottom */
				DDIST beamThick
				)
{
	if (topEdge) {
		if (y0>y1)	y0 += beamThick;
		else		y1 += beamThick;
	}
	else {
		if (y0<y1)	y0 -= beamThick;
		else		y1 -= beamThick;
	}
	SetRect(r, x0, y0, x1, y1);
	UnemptyRect(r);					/* So we don't have to worry about new relationship of y0,y1 */
}


/* ----------------------------------------------------------------------- DrawBEAMSET -- */
/* Draw all primary, secondary, and fractional beam segments of a BEAMSET object. NB:
sets QuickDraw port parameters even if drawing to PostScript, very likely for no
reason! */

void DrawBEAMSET(Document *doc, LINK beamL, CONTEXT context[])
{
	PBEAMSET		pB;
	DDIST			dTop, ydelt, xFracEnd,
					flagYDelta,
					beamThick, qdBeamThick, stdBeamThick,
					firstStfTop, lastStfTop, 		/* For cross-staff beams */
					hPrimDiff, vPrimDiff,
					hFrac, vFrac, yStem,
					startXStem, startYStem,
					stopXStem, stopYStem,
					dLeft[MAXINBEAM],
					xoffStart, xoffStop, 
					dQuarterSp, extension;
	register short	n, iel;
	short			whichWay, beamCount,
					startUOD, stopUOD,
					staff, voice,
					nPrimary, nSecsA[MAXINBEAM], nSecsB[MAXINBEAM];
	LINK			noteBeamL,
					startSyncL, startNoteL,
					stopSyncL, stopNoteL,
					notesInBeam[MAXINBEAM], beamSyncs[MAXINBEAM];
	register PCONTEXT pContext;
	CONTEXT			tempContext;
	Rect			beamRect, oldBeamRect,			/* Aux. rects for computing objRect. N.B. Really DRects. */
					newBeamRect;
	Rect			tempRect;
	Boolean			rectSet,						/* True if objRect aux. rects inited */
					haveSlope, crossStaff,
					dim,							/* Should it be dimmed bcs in a voice not being looked at? */
					crossSys, firstSys,				/* Whether beam is crossSystem, on firstSystem */
					allOneWay;						/* True if all stems are up or all stems are down */
	FASTFLOAT		slope;
	STFRANGE		stfRange;
	BEAMINFO		beamTab[BEAMTABLEN];
	SignedByte		stemUpDown[MAXINBEAM];

	staff = BeamSTAFF(beamL);
	voice = BeamVOICE(beamL);
	
	/* Fill notesInBeam[], an array of the notes in the beamVoice in the bpSyncs, in
	 *	order to call GetCrossStaff to get the stfRange.
	 */
	pB = GetPBEAMSET(beamL);
	crossStaff = pB->crossStaff;
	if (crossStaff) {
		GetBeamNotes(beamL, notesInBeam);
		GetCrossStaff(LinkNENTRIES(beamL), notesInBeam, &stfRange);
	}
	crossSys = pB->crossSystem;
	firstSys = pB->firstSystem;

	pContext = &context[staff];

	/* If beam's voice is not being "looked" at, dim it */
	dim = (outputTo==toScreen && !LOOKING_AT(doc, voice));
	if (dim) PenPat(NGetQDGlobalsGray());
	
	TextSize(UseMTextSize(pContext->fontSize, doc->magnify));
	ForeColor(Voice2Color(doc, voice));

	dTop = pContext->measureTop;
	if (crossStaff) {
		firstStfTop = context[staff].staffTop;
		lastStfTop = context[staff+1].staffTop;
	}
	flagYDelta = FlagLeading(LNSPACE(pContext));
	beamThick = (long)(config.beamLW*LNSPACE(pContext)) / 100L;
	if (pB->thin) beamThick /= 2;
	
	/* Be sure normal (thin) beams on QuickDraw output are at least 2 (1) pixels thick. */
	
	stdBeamThick = LNSPACE(pContext)/2;
	if (pB->thin) stdBeamThick /= 2;
	if (pB->thin)	qdBeamThick = (stdBeamThick<p2d(1)? p2d(1) : stdBeamThick);
	else			qdBeamThick = (stdBeamThick<p2d(2)? p2d(2) : stdBeamThick);

	/*
	 * Beams on downstemmed notes are drawn "sitting" on the position of their notes'
	 * ystems, while beams on upstemmed notes are drawn "hanging" from that position.
	 * We do this so the beam will always be contained within the specified stem
	 * length. This means that, for "center" beams (those that contain both down- and
	 * upstemmed notes), vertical positions of upstems and downstems must overlap. We
	 * achieve this by extending all upstems, and some downstems, in center beams when
	 *	the beams are created (see CreateBEAMSET et al). However, to draw such beams
	 *	correctly, we need more information.
	 */
	whichWay = AnalyzeBeamset(beamL, &nPrimary, nSecsA, nSecsB, stemUpDown);
	allOneWay = (whichWay!=0);
	extension = (allOneWay? 0 : beamThick+(nPrimary-1)*flagYDelta);
	beamCount = BuildBeamDrawTable(beamL, whichWay, nPrimary, nSecsA, nSecsB,
												stemUpDown, beamTab, BEAMTABLEN);
	if (beamCount<0) {
		SysBeep(1);
		LogPrintf(LOG_ERR, "NO BEAM ELEMENTS FOR beamL=L%u  (DrawBEAMSET)\n");
		return;
	}
	
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		/* pNoteBeam = GetPANOTEBEAM(noteBeamL); */
		beamSyncs[iel] = NoteBeamBPSYNC(noteBeamL);
	}
	
	/*
	 * Fill the <dLeft> array with measure positions, but (for speed) only at
	 * locations that start or end beams, since those are the only ones we'll use.
	 */
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++)
		dLeft[iel] = -1;
	for (n = 0; n<beamCount; n++) {
		dLeft[beamTab[n].start] = 0;
		dLeft[beamTab[n].stop] = 0;
	}
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++)
		if (dLeft[iel]==0) {
			GetContext(doc, beamSyncs[iel], staff, &tempContext);
			dLeft[iel] = tempContext.measureLeft;
		}

	/*
	 * We've got all the information we need. Draw the beam segments--first all
	 * non-fractional (primary and secondary) beams so we can get the slope for
	 * drawing the fractional beams, then all the fractional beams.
	 */
	haveSlope = rectSet = False;
	
	for (n = 0; n<beamCount; n++) {
		if (beamTab[n].start!=beamTab[n].stop) {					/* NON-FRACTIONAL BEAM */
			startSyncL = beamSyncs[beamTab[n].start];
			startNoteL = FindMainNote(startSyncL, voice);				
			startUOD = ( (NoteYSTEM(startNoteL) < NoteYD(startNoteL)) ? 1 : -1 );
	
			startXStem = CalcXStem(doc, startSyncL, voice, startUOD,
										dLeft[beamTab[n].start], pContext, True);
			
			yStem = NoteXStfYStem(startNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);
			if (startUOD>0) yStem += extension;
			ydelt = flagYDelta*beamTab[n].startLev;
			startYStem = dTop+yStem+ydelt;

			stopSyncL = beamSyncs[beamTab[n].stop];
			stopNoteL = FindMainNote(stopSyncL, voice);				
			stopUOD = ( (NoteYSTEM(stopNoteL) < NoteYD(stopNoteL)) ? 1 : -1 );

			stopXStem = CalcXStem(doc, stopSyncL, voice, stopUOD,
										dLeft[beamTab[n].stop], pContext, False);
			
			yStem = NoteXStfYStem(stopNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);
			if (stopUOD>0) yStem += extension;
			ydelt = flagYDelta*beamTab[n].stopLev;
			stopYStem = dTop+yStem+ydelt;
			
			/* If Beamset is cross-system and this is a primary beam, extend it horiz. */
			
			xoffStop = 0;
			xoffStart = 0;
			if (crossSys && beamTab[n].start==0
			&& beamTab[n].stop==LinkNENTRIES(beamL)-1) {
				dQuarterSp = LNSPACE(pContext)/4;
				if (firstSys)	xoffStop = config.crossStaffBeamExt*dQuarterSp;
				else			xoffStart = config.crossStaffBeamExt*dQuarterSp;
			}

			switch (outputTo) {
				case toScreen:
				case toBitmapPrint:
				case toPICT:
					Draw1Beam(startXStem-xoffStart, startYStem,
								stopXStem+xoffStop,	 stopYStem,
								(stopUOD>0 && allOneWay),
								qdBeamThick, startUOD, stopUOD, pContext);
							   
					/* If drawing on the screen and this is a primary beam, update objRect */
					
					if (outputTo==toScreen
					&&  (beamTab[n].start==0 && beamTab[n].stop==LinkNENTRIES(beamL)-1)) {
						SetBeamRect(&newBeamRect, startXStem-xoffStart, startYStem,
												  stopXStem+xoffStop,   stopYStem,
												  (stopUOD>0 && allOneWay), beamThick);
						if (rectSet)  {
							UnionRect(&newBeamRect, &oldBeamRect, &beamRect);
							oldBeamRect = beamRect;
						}
						else {
							beamRect = oldBeamRect = newBeamRect;
							rectSet = True;
						}
					}
					break;
				case toPostScript:
					Draw1Beam(startXStem-xoffStart, startYStem,
								stopXStem+xoffStop, stopYStem,
								(stopUOD>0 && allOneWay),
								beamThick, startUOD, stopUOD, pContext);
					break;
				default:
					;
			}

			/* If this is a primary beam and we don't know the slope yet, find it. */
			
			if (!haveSlope && beamTab[n].start==0 && beamTab[n].stop==LinkNENTRIES(beamL)-1) {
				hPrimDiff = (stopXStem+xoffStop)-(startXStem-xoffStart);
				vPrimDiff = stopYStem-startYStem;
				slope = (hPrimDiff ? ((FASTFLOAT)vPrimDiff)/hPrimDiff : 0.0);
				hFrac = FracBeamWidth(LNSPACE(pContext));
				vFrac = hFrac*slope;
				haveSlope = True;
			}
		}
	}
	
	for (n = 0; n<beamCount; n++) {
		if (beamTab[n].start==beamTab[n].stop) {						/* FRACTIONAL BEAM */
			startSyncL = beamSyncs[beamTab[n].start];
			startNoteL = FindMainNote(startSyncL, voice);				
			startUOD = ( (NoteYSTEM(startNoteL) < NoteYD(startNoteL)) ? 1 : -1 );
	
			startXStem = CalcXStem(doc, startSyncL, voice, startUOD,
											dLeft[beamTab[n].start], pContext, True);
			
			yStem = NoteXStfYStem(startNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);
			if (startUOD>0) yStem += extension;
			ydelt = flagYDelta*beamTab[n].startLev;
			startYStem = dTop+yStem+ydelt;

			xFracEnd = (beamTab[n].fracGoLeft? startXStem-hFrac : startXStem+hFrac);

			switch (outputTo) {
				case toScreen:
				case toBitmapPrint:
				case toPICT:
					Draw1Beam(startXStem + (beamTab[n].fracGoLeft ? -p2d(1) : 0),
							  startYStem, xFracEnd,
							  startYStem + (beamTab[n].fracGoLeft ? -vFrac : vFrac),
							  (startUOD>0 && allOneWay),
							  qdBeamThick, startUOD, startUOD, pContext);
					break;
				case toPostScript:
					Draw1Beam(startXStem,
							  startYStem,
							  xFracEnd,
							  startYStem + (beamTab[n].fracGoLeft ? -vFrac : vFrac),
							  (startUOD>0 && allOneWay),
							  beamThick, startUOD, startUOD, pContext);
					break;
				default:
					;
			}
		}
	}
	
	/* Finish up: update the Beamset's objRect, etc. */
	
	if (outputTo==toScreen) {
		tempRect.top = d2p(beamRect.top);
		tempRect.left = d2p(beamRect.left);
		tempRect.bottom = d2p(beamRect.bottom);
		tempRect.right = d2p(beamRect.right);
		LinkOBJRECT(beamL) = tempRect;
		InsetRect(&LinkOBJRECT(beamL), -1, -1);
	
		if (LinkSEL(beamL))
			if (doc->showFormat)
				CheckBEAMSET(doc, beamL, context, NULL, SMFind, stfRange);
			else
#ifdef USE_HILITESCORERANGE
				CheckBEAMSET(doc, beamL, context, NULL, SMHilite, stfRange);
#else
				;
#endif
	
		/* Restore full strength drawing after possible dimming and normal color */
		PenPat(NGetQDGlobalsBlack());
		ForeColor(blackColor);
	}
}


/* ------------------------------------------------------------------------- Draw1Beam -- */
/* Draw one beam segment, i.e., a single horizontal or not-too-far-from-horizontal
thickened line. yl and yr may refer to either the top or the bottom edge of the
line. */
		
void Draw1Beam(DDIST xl, DDIST yl, DDIST xr, DDIST yr,	/* Absolute left & right end pts. */
			Boolean	topEdge,							/* True=yl->yr is top edge of beam, else bottom */
			DDIST beamThick,
			short lUpOrDown, short rUpOrDown,			/* For respective end, 1=stem up, -1=stem down */
			PCONTEXT pContext
			)
{
	short pxlThick, paperLeft, paperTop, yoffset;
	
	switch(outputTo) {
		case toVoid:
			break;
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			paperLeft = pContext->paper.left;
			paperTop = pContext->paper.top;
			pxlThick = d2p(beamThick);
			PenSize(1, pxlThick);
			yoffset = (!topEdge? pxlThick-1 : 0);
			if (yoffset<0) yoffset = 0;
			MoveTo(paperLeft+d2p(xl), paperTop+d2p(yl)-yoffset);
			LineTo(paperLeft+d2p(xr), paperTop+d2p(yr)-yoffset);
			PenSize(1, 1);
			break;
		case toPostScript:
			if (!topEdge) {
				yl -= beamThick;
				yr -= beamThick;
			}
			PS_Beam(xl,yl,xr,yr,beamThick,lUpOrDown,rUpOrDown);
			break;
		default:
			;
	}
}


/* ================================================================= GENERAL UTILITIES == */

/* ---------------------------------------------------------------------- IsBeamStemUp -- */

static Boolean IsBeamStemUp(LINK beamL)
{
	LINK aNoteBeamL, syncL, aNoteL;  short voice;

	/* Cross-staff beams in Nightingale are stem down on the upper staff, stem up
		on the lower staff, so arbitrarily choose one. */ 
	if (BeamCrossSTAFF(beamL)) return False;
	
	aNoteBeamL = FirstSubLINK(beamL);
	syncL = NoteBeamBPSYNC(aNoteBeamL);
	voice = BeamVOICE(beamL);
//LogPrintf(LOG_DEBUG, "IsBeamStemUp: beamL=%u syncL=%u voice=%d\n", beamL, syncL, voice);
	aNoteL = NoteInVoice(syncL, voice, False);
	return (NoteYD(aNoteL)>NoteYSTEM(aNoteL));
}


/* ---------------------------------------------------------------------- GetBeamSyncs -- */
/*	Fill in arrays of Sync LINKs and note LINKs which can be used to fill in the
beamset's subobjects; return True if things are OK, False if not. */
	
Boolean GetBeamSyncs(Document *doc, LINK startL, LINK endL, short voice, short nInBeam,
						LINK bpSync[], LINK noteInSync[], Boolean needSel
					)
{
	short bel;
	LINK thisL, aNoteL;
	Boolean restStatusOK;

	for (bel=0, thisL=startL; thisL!=endL; thisL=RightLINK(thisL))
		if (SyncTYPE(thisL) && (!needSel || LinkSEL(thisL))) {
			aNoteL = FirstSubLINK(thisL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				restStatusOK = (doc->beamRests || !NoteREST(aNoteL)
												|| (bel==0 || thisL==LeftLINK(endL)));
				if (NoteVOICE(aNoteL)==voice && restStatusOK) {
					NoteBEAMED(aNoteL) = True;
					if (MainNote(aNoteL)) {
						bpSync[bel] = thisL;	
						noteInSync[bel] = aNoteL;
						bel++;									/* Increment no. of notes in beamset */
					}
				}
			}
		}

	if (bel!=nInBeam) {
		MayErrMsg("GetBeamSyncs: expected %ld beamable notes but found %ld.",
					(long)nInBeam, (long)bel);
		return False;
	}
	return True;
}


/* --------------------------------------------------------------------- SelBeamedNote -- */
/* If the selection includes any beamed notes/rests (not grace notes), return the
first one found. */

LINK SelBeamedNote(Document *doc)
{
	LINK pL, aNoteL;

	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteBEAMED(aNoteL))
					return aNoteL;

	return NILINK;
}


/* ----------------------------------------------------------------- CountXSysBeamable -- */
/* If range is cross system and is beamable for the specified voice, returns True
with no. of Syncs with notes in each separate range in each system in nBeamable1 & 2;
otherwise returns False. Notice that range must be beamable if the result is 1 in
any system. */

Boolean CountXSysBeamable(Document *doc, LINK startL, LINK endL, short voice,
							short *nBeamable1, short *nBeamable2, Boolean needSel)
{
	LINK sysL;

	if (SameSystem(startL, LeftLINK(endL))) return False;
	
	sysL = LSSearch(startL, SYSTEMtype, ANYONE, GO_RIGHT, False);
	*nBeamable1 = CountBeamable(doc, startL, sysL, voice, needSel);
	*nBeamable2 = CountBeamable(doc, sysL, endL, voice, needSel);
	
	return True;
}


/* --------------------------------------------------------------------- CountBeamable -- */
/* If range is beamable for the specified voice and nothing in it is currently
beamed, returns the no. of Syncs with notes in the voice;  otherwise returns 0.

Notice that (as of v.3.1) Nightingale can't really beam the range if the result
is 1; this is unfortunate, since either part of a cross-system beam might have
only 1 note/rest. */

short CountBeamable(Document *doc,
					LINK startL, LINK endL,
					short voice,
					Boolean needSelected		/* True if we only want selected items */
					)
{
	short	nbeamable;
	LINK	pL, aNoteL, firstNoteL, lastNoteL;

	if (voice<1 || voice>MAXVOICES) return 0;
		
	if (endL==startL) return 0;										/* Reject empty range */
	
	firstNoteL = lastNoteL = NILINK;
	for (nbeamable = 0, pL = startL; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteVOICE(aNoteL)==voice)	{					/* This note/rest on desired voice? */
					if (!firstNoteL) firstNoteL = aNoteL;
					lastNoteL = aNoteL;
					if (!doc->beamRests && NoteREST(aNoteL)) break;	/* Really a rest? Ignore for now */
					if ((needSelected && !NoteSEL(aNoteL)) ||		/* Not selected? */
						(NFLAGS(NoteType(aNoteL))<1) ||				/* Unbeamable? */
						(NoteBEAMED(aNoteL))) 						/* Already beamed? Reject range */
						return 0;		
					else 
						{ nbeamable++; break; }						/* Still possible */
				}
			}
		}

	/*
	 *	If !beamRests, we've ignored all rests, but we shouldn't ignore the first and
	 * last "notes" if they're rests AND there's at least one genuine notes.
	 */
	if (!doc->beamRests && nbeamable>0) {
		if (NoteREST(firstNoteL)) nbeamable++;
		if (NoteREST(lastNoteL)) nbeamable++;
	}

	return nbeamable;
}


/* --------------------------------------------------------------- GetCrossStaffYLevel -- */
/* Decide at what vertical position a cross-staff beam should go. We assume the beam will
be halfway between the two staves (and horizontal). */

DDIST GetCrossStaffYLevel(Document *doc, LINK pL, STFRANGE theRange)
{
	LINK staffL, topStaffL, bottomStaffL;
	CONTEXT context;
	SearchParam pbSearch;

	/* Get the staff range of the cross staff beam */
	InitSearchParam(&pbSearch);
	pbSearch.voice = ANYONE;
	pbSearch.needSelected = False;
	pbSearch.inSystem = True;
	
	/* Get the top and bottom staff subobjects */
	pbSearch.id = theRange.topStaff;
	staffL = L_Search(pL, STAFFtype, True, &pbSearch);
	topStaffL = pbSearch.pEntry;
	pbSearch.id = theRange.bottomStaff;
	staffL = L_Search(pL, STAFFtype, True, &pbSearch);
	bottomStaffL = pbSearch.pEntry;
	
	/* Return the position halfway inbetween these two staves. */
	GetContext(doc, staffL, theRange.topStaff, &context);
	return ((StaffTOP(bottomStaffL)+context.staffHeight-StaffTOP(topStaffL))/2);
}


/* -------------------------------------------------------------------- CalcBeamYLevel -- */
/* Decide at what vertical position a (for now, horizontal) beam should go. For
now, use the "extreme position dominates" rule alone; an improved version might
use Gomberg's, Gourlay's, or SMUT's rules. */

DDIST CalcBeamYLevel(
	Document *doc,
	short nElts,
	LINK bpSync[],
	LINK noteInSync[],
	LINK *baseL,			/* If !crossStaff, sync w/ extreme stem, used to get beam y-level */
	DDIST stfHeight,
	short stfLines,
	Boolean	crossStaff,
	short voiceRole,		/* VCROLE_UPPER, etc. (see Multivoice.h) */
	Boolean	*stemUp			/* If !crossStaff */
	)

{
	LINK		aNoteL, partL, belowL, aboveL;
	PPARTINFO	pPart;
	short		iel, voice, staffn;
	DDIST		maxYHead, minYHead, abovePos, belowPos, midline, trystem;
	STFRANGE	theRange;
	char		stemLen;
	
	if (crossStaff) {
		GetCrossStaff(nElts, noteInSync, &theRange);			/* Get the staff range of the cross staff beam */
		return GetCrossStaffYLevel(doc, bpSync[0], theRange);
	}

	abovePos = 9999; belowPos = -9999;
	
	voice = NoteVOICE(noteInSync[0]);
	staffn = NoteSTAFF(noteInSync[0]);

	stemLen = (voiceRole==VCROLE_SINGLE? config.stemLenNormal : config.stemLen2v);
	for (maxYHead=-9999, minYHead=9999, iel=0; iel<nElts; iel++) {

/* To decide whether beams go above or below, get high and low notehead
 *	positions.  To decide beams' y-position, once above or below is known,
 *	get high stem position assuming stem up and low stem position assuming
 *	stem down.
 */
		aNoteL = FirstSubLINK(bpSync[iel]);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
	 		if (NoteVOICE(aNoteL)==voice) {
				maxYHead = n_max(maxYHead, NoteYD(aNoteL));
				minYHead = n_min(minYHead, NoteYD(aNoteL));
				trystem = CalcYStem(doc, NoteYD(aNoteL),				/* Get upstem end pos. */
							  		NFLAGS(NoteType(aNoteL)), False, stfHeight, stfLines,
							  		stemLen, False);
				abovePos = n_min(abovePos, trystem);
				if (abovePos==trystem) aboveL = bpSync[iel];
				trystem = CalcYStem(doc, NoteYD(aNoteL),				/* Get downstem end pos. */
							  		NFLAGS(NoteType(aNoteL)), True, stfHeight, stfLines,
							  		stemLen, False);
				belowPos = n_max(belowPos, trystem);
				if (belowPos==trystem) belowL = bpSync[iel];
			}
		}
	}
	
	belowPos = n_max(belowPos,
						CalcYStem(doc, minYHead, 0, True, stfHeight, stfLines, stemLen, False));
	abovePos = n_min(abovePos,
						CalcYStem(doc, maxYHead, 0, False, stfHeight, stfLines, stemLen, False));
						
	switch (voiceRole) {
		case VCROLE_SINGLE:
			midline = std2d(STD_LINEHT*2, stfHeight, stfLines);
			if ( maxYHead>midline && minYHead>midline )				/* All notes below midline? */
				*stemUp = True;
			else if ( maxYHead<midline && minYHead<midline )		/* All notes above midline? */
				*stemUp = False;
			else if ( ABS(maxYHead-midline)>ABS(minYHead-midline) ) /* Notes below midline more extreme? */
				*stemUp = True;
			else
				*stemUp = False;
			break;
		case VCROLE_UPPER:
			*stemUp = True;
			break;
		case VCROLE_LOWER:
			*stemUp = False;
			break;
		case VCROLE_CROSS:
			partL = FindPartInfo(doc, Staff2Part(doc, staffn));
			pPart = GetPPARTINFO(partL);
			*stemUp = (staffn!=pPart->firstStaff);
			break;
	}
			
	if (*stemUp) {
		*baseL = aboveL;
		return abovePos;
	}
	else {
		*baseL = belowL;
		return belowPos;
	}
}


/* -------------------------------------------------------------------- VHasBeamAcross -- */
/* If the point just before <node> in the given voice is in the middle of a beam,
VHasBeamAcross returns the Beamset's link, otherwise NILINK.  */

LINK VHasBeamAcross(LINK node, short voice)	
{
	LINK		lSyncL, rSyncL, lNoteL, rNoteL, beamL;
	SearchParam	pbSearch;

	if (node==NILINK) return NILINK;	
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = False;
	pbSearch.inSystem = False;
	lSyncL = L_Search(LeftLINK(node), SYNCtype, GO_LEFT, &pbSearch);/* Look for 1st note left and right */
	rSyncL = L_Search(node, SYNCtype, GO_RIGHT, &pbSearch);
	if (lSyncL==NILINK || rSyncL==NILINK)							/* Were there any? */						
		return NILINK;												/* No */
	else
	{																/* Yes */
		lNoteL = FirstSubLINK(lSyncL);								/* At left... */
		for ( ; lNoteL; lNoteL = NextNOTEL(lNoteL)) {
			/* lNote = GetPANOTE(lNoteL); */
			if (NoteVOICE(lNoteL)==voice)							/* This note in desired voice? */
				if (!NoteBEAMED(lNoteL)) return NILINK;				/* Yes. Beamed? If not, we're done. */
		}
		
		rNoteL = FirstSubLINK(rSyncL);								/* At right... */
		for ( ; rNoteL; rNoteL = NextNOTEL(rNoteL)) {
			/* rNote = GetPANOTE(rNoteL); */
			if (NoteVOICE(rNoteL)==voice)							/* This note in desired voice? */
				if (!NoteBEAMED(rNoteL)) return NILINK;				/* Yes. Beamed? If not, we're done. */
				else {												/* Both are beamed.  */
					pbSearch.subtype = NoteBeam;
					beamL = L_Search(rSyncL, BEAMSETtype, GO_LEFT,
											&pbSearch); 			/* Are they in the same beam? */
					if (IsAfter(beamL, lSyncL))	return beamL;		/* Yes */
					else						return NILINK;		/* No */
				}
		}
	}

	return NILINK;
}

/* --------------------------------------------------------------------- HasBeamAcross -- */
/* If the point just before <node> on the given staff is in the middle of a beam,
HasBeamAcross returns the Beamset's link, otherwise NILINK.  NB: Probably
should be replaced completely by VHasBeamAcross. */

LINK HasBeamAcross(LINK node, short staff)	
{
	LINK lSyncL, rSyncL, lNoteL, rNoteL, beamL;

	if (node==NILINK) return NILINK;	
	lSyncL = LSSearch(LeftLINK(node), SYNCtype, staff, GO_LEFT, False);	/* Look for 1st note left */
	rSyncL = LSSearch(node, SYNCtype, staff, GO_RIGHT, False);			/* Look for 1st note right */
	if (lSyncL==NILINK || rSyncL==NILINK)								/* Were there any? */						
		return NILINK;													/* No */
	else
	{																	/* Yes */
		lNoteL = FirstSubLINK(lSyncL);									/* At left... */
		for ( ; lNoteL; lNoteL = NextNOTEL(lNoteL)) {
			/* lNote = GetPANOTE(lNoteL); */
			if (NoteSTAFF(lNoteL)==staff)								/* This note on desired staff? */
				if (!NoteBEAMED(lNoteL)) return NILINK;					/* Yes. Beamed? If not, we're done. */
		}
		
		rNoteL = FirstSubLINK(rSyncL);									/* At right... */
		for ( ; rNoteL; rNoteL = NextNOTEL(rNoteL)) {
			/* rNote = GetPANOTE(rNoteL); */
			if (NoteSTAFF(rNoteL)==staff)								/* This note on desired staff? */
				if (!NoteBEAMED(rNoteL)) return NILINK;					/* Yes. Beamed? If not, we're done. */
				else {													/* Both are beamed.  */
					beamL = LSSearch(rSyncL, BEAMSETtype,
										staff, GO_LEFT, False);			/* Are they in the same beam? */
					if (IsAfter(beamL, lSyncL))	return beamL;			/* Yes */
					else						return NILINK;			/* No */
				}
		}
	}

	return NILINK;
}


/* --------------------------------------------- VCheckBeamAcross,VCheckBeamAcrossIncl -- */
/* If the point just before <node> in the given voice is in the middle of a beam,
VCheckBeamAcross returns the Beamset's link, otherwise NILINK. Identical to
VHasBeamAcross except that the search to the right starts just after node. This
is for situations like in AddNote, where the data structure may be in an
inconsistent state because node was just added. */

LINK VCheckBeamAcross(LINK node, short voice)	
{
	LINK		lSyncL, rSyncL, lNoteL, rNoteL, beamL;
	SearchParam	pbSearch;

	if (node==NILINK) return NILINK;	
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = False;
	pbSearch.inSystem = False;

	lSyncL = L_Search(LeftLINK(node), SYNCtype, GO_LEFT, &pbSearch);	/* Look for 1st note left */
	rSyncL = L_Search(RightLINK(node), SYNCtype, GO_RIGHT, &pbSearch);	/* Look for 1st note right */
	if (lSyncL==NILINK || rSyncL==NILINK) return NILINK;
	
	lNoteL = FirstSubLINK(lSyncL);										/* At left... */
	for ( ; lNoteL; lNoteL = NextNOTEL(lNoteL))
		if (NoteVOICE(lNoteL)==voice)									/* This note in desired voice? */
			if (!NoteBEAMED(lNoteL)) return NILINK;						/* Yes. Beamed? If not, we're done. */
	
	rNoteL = FirstSubLINK(rSyncL);										/* At right... */
	for ( ; rNoteL; rNoteL = NextNOTEL(rNoteL))
		if (NoteVOICE(rNoteL)==voice) {									/* This note on desired voice? */
			if (!NoteBEAMED(rNoteL)) return NILINK;						/* Yes. Beamed? If not, we're done. */
			pbSearch.subtype = NoteBeam;
			beamL = L_Search(rSyncL, BEAMSETtype, GO_LEFT, &pbSearch); 	/* Are they in the same beam? */
			if (beamL)
				if (IsAfter(beamL, lSyncL)) return beamL;				/* Yes */
			return NILINK;												/* No */
		}

	return NILINK;
}


LINK VCheckBeamAcrossIncl(LINK node, short voice)	
{
	LINK		beamL, aNoteL;
	SearchParam pbSearch;
	
	beamL = VCheckBeamAcross(node, voice);
	if (beamL) return beamL;
	if (!SyncTYPE(node)) return NILINK;

	aNoteL = FirstSubLINK(node);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice) {
			if (NoteBEAMED(aNoteL)) {
				InitSearchParam(&pbSearch);
				pbSearch.id = ANYONE;
				pbSearch.voice = voice;
				pbSearch.needSelected = pbSearch.inSystem = False;
				pbSearch.subtype = NoteBeam;
				return L_Search(node, BEAMSETtype, GO_LEFT, &pbSearch); /* Look for 1st note left */
			}
			return NILINK;
		}
	return NILINK;	
}


/* ----------------------------------------------------------- FirstInBeam, LastInBeam -- */

LINK FirstInBeam(LINK beamL)
{
	return (NoteBeamBPSYNC(FirstSubLINK(beamL)));
}

LINK LastInBeam(LINK beamL)
{
	LINK aNoteBeamL, theNoteBeamL;
	short i, nInBeam;
	
	nInBeam = LinkNENTRIES(beamL);
	aNoteBeamL = FirstSubLINK(beamL);
	for (i=0; i<nInBeam; i++, aNoteBeamL = NextNOTEBEAML(aNoteBeamL))
		theNoteBeamL = aNoteBeamL;
	
	return (NoteBeamBPSYNC(theNoteBeamL));
}


/* Null routine to allow loading or unloading Beam.c's segment. */

void XLoadBeamSeg()
{
}
