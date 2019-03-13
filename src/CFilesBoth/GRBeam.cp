/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* File GRBeam.c - grace-note-specific beaming functions. Functions that handle
both grace and regular beams are in Beam.c.

	CreateGRBeams			AddNewGRBeam			GetGRCrossStaff
	GetGRBeamXStf			CalcGRBeamYLevel		GRCrossStaffYStem
	FixGRSyncInBeamset		SetBeamGRNoteStems		CreateGRBEAMSET
	GRUnbeamx				FixGRStem				GRUnbeamRange

	GetBeamGRNotes			CalcGRXStem				GRNoteXStfYStem
	AnalyzeGRBeamset		BuildGRBeamDrawTable	DrawGRBEAMSET
	
	GetBeamGRSyncs			CountGRBeamable			VHasGRBeamAcross
	VCheckGRBeamAcross		VCheckGRBeamAcrossIncl	RemoveGRBeam
	RecomputeGRNoteStems
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean GetGRCrossStaff(short, LINK [], STFRANGE *);
static DDIST GRCrossStaffYStem(LINK, STFRANGE, DDIST, DDIST);
static void FixGRSyncInBeamset(Document *, LINK, short, DDIST, STFRANGE, Boolean);
static void FixGRStem(Document *, LINK, LINK, CONTEXT);
static Boolean GetBeamGRSyncs(Document *, LINK, LINK, short, short, LINK [], LINK [], Boolean);


/* ======================================================= CREATING/REMOVING/MODIFYING == */

/* --------------------------------------------------------------------- CreateGRBeams -- */ 
/* If possible, create a grace-note beam of the selection in the given voice. */

void CreateGRBeams(Document *doc, short v)
{
	LINK beamL; short nBeamable;

	beamL=NILINK;
	nBeamable = CountGRBeamable(doc, doc->selStartL, doc->selEndL, v, True);
	if (nBeamable>1)
		beamL = CreateGRBEAMSET(doc, doc->selStartL, doc->selEndL, v,
						nBeamable, True, True);

	if (beamL)
		if (doc->selStartL==FirstInBeam(beamL))				/* Does sel. start at beam's 1st note? */
			doc->selStartL = beamL;									/* Yes, add the beam to the selection */
}


/* --------------------------------------------------------- CreateGRBEAMSET utilities -- */
/* Functions which are used by CreateGRBEAMSET. */

#define PROG_ERR 	-3										/* Code for program error */
#define USR_ALERT -2										/* Code for user alert */
#define F_ABORT	-1										/* Code for function abort */
#define NF_OK		1										/* Code for successful completion */
#define BAD_CODE(returnCode) ((returnCode)<0) 

static LINK AddNewGRBeam(Document *, LINK, short, short, short *, Boolean, Boolean);
static short GetGRBeamXStf(LINK, short, LINK [], STFRANGE *);
static DDIST CalcGRBeamYLevel(Document *, short, LINK [], LINK [], DDIST, short, Boolean, Boolean);
static void SetBeamGRNoteStems(Document *, LINK, short, short, LINK [], LINK [], DDIST,
									STFRANGE, Boolean);

/* ---------------------------------------------------------------------- AddNewGRBeam -- */
/*	Add the GRBeamset to the data structure and initialize the appropriate fields. */

static LINK AddNewGRBeam(Document *doc, LINK startL, short v, short nInBeam,
									short *staff,
									Boolean needSel,
									Boolean leaveSel	/* True=leave the new Beamset selected */
									)
{
	SearchParam pbSearch; LINK beamL; PBEAMSET beamp;

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = v;
	pbSearch.needSelected = needSel;
	pbSearch.inSystem = False;
	startL = L_Search(startL, GRSYNCtype, GO_RIGHT, &pbSearch);
	*staff = GRNoteSTAFF(pbSearch.pEntry);

	beamL = InsertNode(doc, startL, BEAMSETtype, nInBeam);
	if (beamL) {
		SetObject(beamL, 0, 0, False, True, False);
		beamp = GetPBEAMSET(beamL);
		beamp->tweaked = False;
		beamp->staffn = *staff;
		beamp->voice = v;
		if (leaveSel) beamp->selected = True;
		beamp->feather = beamp->thin = 0;
		beamp->grace = True;
		beamp->beamRests = False;
		
		doc->changed = True;
	}
	else
		NoMoreMemory();
	
	return beamL;
}

/* ------------------------------------------------------------------- GetGRCrossStaff -- */
/* If the given grace notes are all on the same staff, return False; else fill in the
stfRange and return True. In the latter case, we assume the grace notes are on only
two different staves; we don't check this, nor that the staves are consecutive. */

static Boolean GetGRCrossStaff(short nElem, LINK grNotes[], STFRANGE *stfRange)
{
	short i, firstStaff, stf; STFRANGE theRange; Boolean crossStaff=False;
	
	firstStaff = GRNoteSTAFF(grNotes[0]);
	theRange.topStaff = theRange.bottomStaff = firstStaff;
	for (i=0; i<nElem; i++)
		if ((stf = GRNoteSTAFF(grNotes[i]))!=firstStaff) {
			crossStaff = True;
			if (stf<theRange.topStaff) theRange.topStaff = stf;
			else if (stf>theRange.bottomStaff) theRange.bottomStaff = stf;
		}
	*stfRange = theRange;
	return crossStaff;
}

/* If the beamset is cross staff, set its cross staff flag, get the correct staff
for the beamset object, return the range of staves in theRange, and check the
validity of this range. */

static short GetGRBeamXStf(LINK beamL, short bel, LINK GRNoteInSync[], STFRANGE *theRange)
{
	short crossStaff; PBEAMSET beamp;

	beamp = GetPBEAMSET(beamL);
	crossStaff = GetGRCrossStaff(bel, GRNoteInSync, theRange);
	if (crossStaff) {
		if (theRange->bottomStaff-theRange->topStaff > 1) {
			MayErrMsg("SetBeamXStf: cannot beam notes on non-adjacent staves.");
			return PROG_ERR;
		}
		else {
			BeamSTAFF(beamL) = theRange->topStaff;
			beamp->crossStaff = True;
		}
	}
	else beamp->crossStaff = False;

	return crossStaff;
}


/* ------------------------------------------------------------------ CalcGRBeamYLevel -- */
/* Decide at what vertical position a (for now, horizontal) grace-note beam
should go. */

static DDIST CalcGRBeamYLevel(
					Document *doc,
					short nElts,
					LINK bpGRSync[],
					LINK GRNoteInSync[],
					DDIST stfHeight,
					short stfLines,
					Boolean crossStaff,
					Boolean stemDown
					)
{
	PAGRNOTE	aGRNote;
	LINK		aGRNoteL;
	short		iel, voice;
	DDIST		maxYHead, minYHead, abovePos, belowPos, trystem;
	STFRANGE	theRange;
	
	if (crossStaff) {
		/* Get the staff range of the cross staff beam */
		GetGRCrossStaff(nElts, GRNoteInSync, &theRange);
		return GetCrossStaffYLevel(doc, bpGRSync[0], theRange);
	}

	abovePos = 9999; belowPos = -9999;
	
	aGRNoteL = GRNoteInSync[0];
	voice = GRNoteVOICE(aGRNoteL);

	for (maxYHead=-9999, minYHead=9999, iel=0; iel<nElts; iel++) {

/* Get high and low notehead positions.  To decide beams' y-position, once above
 *	or below is known, get high stem position assuming stem up and low stem position
 *	assuming stem down.
 */
		aGRNoteL = FirstSubLINK(bpGRSync[iel]);
		for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
			aGRNote = GetPAGRNOTE(aGRNoteL);
	 		if (aGRNote->voice==voice) {
				maxYHead = n_max(maxYHead, aGRNote->yd);
				minYHead = n_min(minYHead, aGRNote->yd);
				trystem = CalcYStem(doc, aGRNote->yd,						/* Get upstem end pos. */
							  		NFLAGS(aGRNote->subType), False, stfHeight, stfLines,
							  		config.stemLenGrace, False);
				abovePos = n_min(abovePos, trystem);
				aGRNote = GetPAGRNOTE(aGRNoteL);
				trystem = CalcYStem(doc, aGRNote->yd,						/* Get downstem end pos. */
							  		NFLAGS(aGRNote->subType), True, stfHeight, stfLines,
							  		config.stemLenGrace, False);
				belowPos = n_max(belowPos, trystem);
			}
		}
	}
	
	if (stemDown) {
		belowPos = n_max(belowPos,
							CalcYStem(doc, minYHead, 0, True, stfHeight, stfLines,
											config.stemLenGrace, False));
		return belowPos;													/* Beam below notes */
	}
	else {
		abovePos = n_min(abovePos,
							CalcYStem(doc, maxYHead, 0, False, stfHeight, stfLines,
											config.stemLenGrace, False));
		return abovePos;													/* Beam above notes */
	}
}


/* ----------------------------------------------------------------- GRCrossStaffYStem -- */
/* Get the ystem for a grace note in a crossStaff beamset. If the note is on the
top staff of the beamset's stfRange, use the normal ystem; if it is on the
bottom staff, correct for the height of its staff. */

static DDIST GRCrossStaffYStem(LINK aGRNoteL, STFRANGE theRange, DDIST ystem,
									DDIST stfHeight)
{
	if (GRNoteSTAFF(aGRNoteL)==theRange.topStaff)
			return ystem;
	else	return (ystem - stfHeight);
}

/* ---------------------------------------------------------------- FixGRSyncInBeamset -- */
/* Fix stem end coordinates for the note(s) in a beamed GRSync.  If the GRSync has
a chord for the given voice, all the stems are set to zero length except the
one furthest from the beam. */

static void FixGRSyncInBeamset(Document *doc, LINK bpGRSyncL, short voice,
							DDIST ystem, STFRANGE theRange, Boolean crossStaff)
{
	PAGRNOTE	extremeGRNote, aGRNote;
	LINK 		aGRNoteL;
	DDIST		maxLength;
	CONTEXT		topContext, bottomContext;
	Boolean		inChord,stemDown;
	
/* Need the staffHeight to adjust the stems for crossStaff beamed notes. */
	if (crossStaff) {
		GetContext(doc, bpGRSyncL, theRange.topStaff, &topContext);
		GetContext(doc, bpGRSyncL, theRange.bottomStaff, &bottomContext);
	}		

/* If the sync has a chord in the voice, get the extreme note. */
	extremeGRNote = NULL;
	maxLength = -9999;
	aGRNoteL = FirstSubLINK(bpGRSyncL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (aGRNote->voice==voice) {
			inChord = aGRNote->inChord;
			if (GRMainNote(aGRNoteL)) stemDown = aGRNote->yd < ystem;
			if (aGRNote->inChord) {
				if (ABS(aGRNote->yd-ystem)>maxLength) {
					maxLength = ABS(aGRNote->yd-ystem);
					extremeGRNote = aGRNote;
				}
			}
		}
	}

/* Adjust the ystem of notes in the voice. If there is a chord, set all the
ystems equal to the yds except for the extremeGRNote; set the ystem equal to
<ystem> unless the note/chord is in a crossStaff beam, in which case get the
crossStaff ystem, and set the note's ystem to it. */
	aGRNoteL = FirstSubLINK(bpGRSyncL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (aGRNote->voice==voice)
			if (extremeGRNote) {
				if (aGRNote==extremeGRNote) {
					if (crossStaff)
							aGRNote->ystem = GRCrossStaffYStem(aGRNoteL, theRange, ystem, 
													bottomContext.staffTop-topContext.staffTop);
					else	aGRNote->ystem = ystem;
				}
				else	aGRNote->ystem = aGRNote->yd;
			}
			else {
				if (crossStaff)
						aGRNote->ystem = GRCrossStaffYStem(aGRNoteL, theRange, ystem, 
													bottomContext.staffTop-topContext.staffTop);
				else	aGRNote->ystem = ystem;								/* No chord here */
			}
	}
}


/* Set each note's stem to end at the beam.  For each note except those on the
ends, set delt_left to the change in the no. of beams from the note to the
left to this one; likewise for delt_right.  If both are positive, fractional
beams are required.  Also, if one is greater than the other, this note
shares secondary beam(s) with that one...more-or-less.

N.B. Where fractional beams occur, if there are no secondary beams to force their
direction, this version simply points them to the right if they're on the first
note of the beamset, else to the left. See Byrd's dissertation, Sec. 4.4.2, for
a much better method. */
 
static void SetBeamGRNoteStems(Document *doc, LINK beamL,
										short v, short nInBeam,
										LINK bpGRSync[], LINK noteInSync[],
										DDIST ystem,
										STFRANGE theRange,
										Boolean crossStaff)
{
	short iel; LINK aGRNoteL, aNoteBeamL;
	PAGRNOTE aGRNote; PANOTEBEAM aNoteBeam;

	aNoteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<nInBeam; iel++, aNoteBeamL=NextNOTEBEAML(aNoteBeamL))
	{
		aGRNoteL = noteInSync[iel];								/* Link to note in SYNC */
		aNoteBeam = GetPANOTEBEAM(aNoteBeamL);					/* Ptr to note info in BEAMSET */
		aNoteBeam->bpSync = bpGRSync[iel];
		FixGRSyncInBeamset(doc, bpGRSync[iel], v, ystem, theRange, crossStaff);

		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (iel==0)
			aNoteBeam->startend = NFLAGS(aGRNote->subType);
		else if (iel==nInBeam-1)
			aNoteBeam->startend = -NFLAGS(aGRNote->subType);
		else
			aNoteBeam->startend = 0;

		aNoteBeam->fracs = 0;
		aNoteBeam->fracGoLeft = False;
	}
}


/* ------------------------------------------------------------------- CreateGRBEAMSET -- */
/* Create a grace-note beamset, i.e., a set of primary beams only, for the
specified range. The range must contain <nInBeam> grace notes (counting each  
chord as one), all of which must be in selected GRSyncs and must be (possibly
dotted) eighths or shorter. The range may also contain any number of non-grace
notes, rests, and other non-notes, which will be ignored. For now, the beams are
always horizontal and are at a vertical position decided by CalcGRBeamYLevel. */

LINK CreateGRBEAMSET(Document *doc,
						LINK startL, LINK endL,
						short v,
						short nInBeam,
						Boolean needSel,		/* True if we only want selected items */
						Boolean doBeam			/* True if we are explicitly beaming notes (so beamset will be selected) */
						)
{
	short		staff;
	PASTAFF		aStaff;
	LINK		beamL, staffL, aStaffL;
	LINK		bpGRSync[MAXINBEAM];
	LINK		GRNoteInSync[MAXINBEAM];
	PBEAMSET	pBeam;
	DDIST		ystem, stfHeight;
	short		stfLines;
	Boolean		crossStaff;					/* True if beam is cross-staff */
	STFRANGE	theRange;					/* The range of staves occupied by a crossStaff beam */
	short		returnCode;

	returnCode = CheckBeamVars(nInBeam, False, 0, 0);
	if (BAD_CODE(returnCode)) return NILINK;

	/* Add the beamset object to the data structure. */
	beamL = AddNewGRBeam(doc, startL, v, nInBeam, &staff, needSel, doBeam);
	pBeam = GetPBEAMSET(beamL);
	pBeam->crossSystem = pBeam->firstSystem = False;

	/* Fill in the arrays of sync and note LINKs for the benefit of beam subObjs. */
	if (!GetBeamGRSyncs(doc, startL, endL, v, nInBeam, bpGRSync, GRNoteInSync, needSel))
			return NILINK;

	/* Handle cross staff conditions. */
	crossStaff = GetGRBeamXStf(beamL, nInBeam, GRNoteInSync, &theRange);
	if (crossStaff) return NILINK;

	/* Get the y level of the beamset and set the note stems of the beamset's notes. */
	staffL = LSSearch(startL, STAFFtype, staff, True, False);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		if (aStaff->staffn == staff) {
			stfHeight =	aStaff->staffHeight;
			stfLines = aStaff->staffLines;
		}
	} 
	ystem = CalcGRBeamYLevel(doc, nInBeam, bpGRSync, GRNoteInSync, stfHeight, stfLines,
								crossStaff, False);
	SetBeamGRNoteStems(doc, beamL, v, nInBeam, bpGRSync, GRNoteInSync, ystem,
									theRange, crossStaff);

	InvalMeasures(startL, endL, staff);						/* Force redrawing all affected measures */
	return beamL;
}


/* ------------------------------------------------------ GRUnbeamRange and friend -- */

/* GRUnbeamx removes all grace-note beams in the specified range for the specified voice.
If any beams cross the ends of the range, GRUnbeamx rebeams the portions outside of the
range, if that's possible. It should ordinarily be called only via Unbeam, not directly,
since it doesn't worry about leaving a legal selection range.

GRUnbeamx assumes the document is on the screen, so it Invals the measures containing
the beams. It may call InvalMeasures several times, which is slightly inefficient, but
easier than figuring out the range for a single call (and perhaps returning them to
the caller). */
	
void GRUnbeamx(Document *doc, LINK fromL, LINK toL, short voice)
{
	LINK beamFromL, beamToL, lBeamL, rBeamL, newBeamL;
	short nInBeam;
	PBEAMSET lBeam, rBeam, newBeam;
	Boolean crossSys, firstSys;

	/* If any beams cross the endpoints of the range, remove them first and, if
	possible, rebeam the portion outside the range. */
	
	if ( (rBeamL = VHasBeamAcross(toL, voice)) != NILINK)				/* Get beam across right end. */
		beamToL = RightLINK(LastInBeam(rBeamL));
	if ( (lBeamL = VHasBeamAcross(fromL, voice)) != NILINK) {			/* Remove beam across left end. */
		lBeam = GetPBEAMSET(lBeamL);
		crossSys = lBeam->crossSystem;
		firstSys = lBeam->firstSystem;
		beamFromL = FirstInBeam(lBeamL);
		if (GRUnbeamRange(doc, beamFromL, RightLINK(LastInBeam(lBeamL)), voice))
			InvalMeasures(beamFromL, RightLINK(LastInBeam(lBeamL)), ANYONE);				/* Force redrawing all affected measures */
		if ( (nInBeam = CountGRBeamable(doc, beamFromL, fromL, voice, False)) >=2 ) {	/* Beam remainder */
			newBeamL = CreateGRBEAMSET(doc, beamFromL, fromL, voice, nInBeam, False,
												doc->voiceTab[voice].voiceRole);	/* ??LAST PARAM?? */
			if (crossSys && !firstSys) {
				newBeam = GetPBEAMSET(newBeamL);
				newBeam->crossSystem = True;
				newBeam->firstSystem = False;
			}
					
		}
	}	
	if (rBeamL != NILINK) {												/* Remove beam across right end. */
		if (rBeamL!=lBeamL) {
			rBeam = GetPBEAMSET(lBeamL);
			crossSys = rBeam->crossSystem;
			firstSys = rBeam->firstSystem;
			if (GRUnbeamRange(doc, FirstInBeam(rBeamL), beamToL, voice))
				InvalMeasures(FirstInBeam(rBeamL), beamToL, ANYONE);						/* Force redrawing all affected measures */
		}
		if ( (nInBeam = CountGRBeamable(doc, toL, beamToL, voice, False)) >=2 ) {	/* Beam remainder */
			newBeamL = CreateGRBEAMSET(doc, toL, beamToL, voice, nInBeam, False,
												doc->voiceTab[voice].voiceRole);
			if (crossSys && firstSys) {
				newBeam = GetPBEAMSET(newBeamL);
				newBeam->crossSystem = newBeam->firstSystem = True;
			}
		}		
	}

	/* Main loop: unbeam every beamed grace note within the range. */
	
	if (GRUnbeamRange(doc, fromL, toL, voice))
		InvalMeasures(fromL, toL, ANYONE);								/* Force redrawing all affected measures */
}


/* Fix stems, head stem sides, etc. for the given unbeamed note or its chord. */
		
void FixGRStem(Document *doc, LINK syncL, LINK grNoteL, CONTEXT context)
{
		if (GRNoteINCHORD(grNoteL))
			FixGRSyncForChord(doc, syncL, GRNoteVOICE(grNoteL), False, 0, 0, &context);
		else
			FixGRSyncNote(doc, syncL, GRNoteVOICE(grNoteL), &context);
}

/* Unbeam every beamed grace note within the range and voice. If we actually find
any, return True. */

Boolean GRUnbeamRange(Document *doc, LINK fromL, LINK toL, short voice)
{
	LINK		pL, beamL, noteBeamL, syncL, aGRNoteL, bGRNoteL;
	PANOTEBEAM	pNoteBeam;
	SearchParam	pbSearch;
	Boolean		didAnything=False, needContext=True;
	CONTEXT		context;
	
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = False;
	pbSearch.inSystem = False;

	for (pL = fromL; pL!=toL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==GRSYNCtype) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
				if (GRNoteVOICE(aGRNoteL)==voice && GRNoteBEAMED(aGRNoteL)) {
					pbSearch.subtype = GRNoteBeam;
					beamL = L_Search(pL, BEAMSETtype, GO_LEFT, &pbSearch);
					if (beamL==NILINK) {
						MayErrMsg("GRUnbeamRange: can't find Beamset for grace note in GRsync %ld in voice %ld",
									(long)pL, (long)voice);
						return didAnything;
					}

					if (needContext) {
						GetContext(doc, beamL, BeamSTAFF(beamL), &context);
						needContext = False;
					}

					noteBeamL = FirstSubLINK(beamL);
					for ( ; noteBeamL; noteBeamL = NextNOTEBEAML(noteBeamL)) {
						pNoteBeam = GetPANOTEBEAM(noteBeamL);
						syncL = pNoteBeam->bpSync;
						bGRNoteL = FirstSubLINK(syncL);
						for ( ; bGRNoteL; bGRNoteL = NextGRNOTEL(bGRNoteL)) {
							if (GRNoteVOICE(bGRNoteL)==voice) {				/* This note in desired voice? */
								GRNoteBEAMED(bGRNoteL) = False;				/* Yes */
								if (GRMainNote(bGRNoteL))
									FixGRStem(doc, syncL, bGRNoteL, context);
							}
						}
					}
					DeleteNode(doc, beamL);									/* Delete beam */		
					didAnything = True;
				}
			}
		}
	}
	
	if (didAnything) doc->changed = True;
	return didAnything;
}


/* -------------------------------------------------------------------- GetBeamGRNotes -- */
/* Fill in an array of grace notes contained in the beamset <pL>. */

void GetBeamGRNotes(LINK pL, LINK grNotes[])
{
	LINK aNoteBeamL, pGRSyncL, aGRNoteL;
	PANOTEBEAM aNoteBeam;
	short i, voice;

	voice = BeamVOICE(pL);
	aNoteBeamL = FirstSubLINK(pL);
	for (i=0; aNoteBeamL; i++, aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
		aNoteBeam = GetPANOTEBEAM(aNoteBeamL);
		pGRSyncL = aNoteBeam->bpSync;
		aGRNoteL = FirstSubLINK(pGRSyncL);
		for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
			if (GRNoteVOICE(aGRNoteL)==voice) {
				grNotes[i] = aGRNoteL;
				break;
			}
	}
}


/* =========================================================================== DRAWING == */

/* ----------------------------------------------------------------------- CalcGRXStem -- */
/* Given a GRSync, voice number, stem direction, and standard grace notehead width,
return the page-relative x-coordinate of the left or right end of a beam for the
note/chord in that GRSync and voice. */

DDIST CalcGRXStem(Document *doc, LINK grSyncL, short voice, short stemDir, DDIST measLeft,
					PCONTEXT pContext,
					Boolean /*left*/				/* True=left end of beam, else right */
					)
{
	DDIST		dHeadWidth, xd, xdNorm;
	LINK		aGRNoteL;
	PAGRNOTE	aGRNote;
	short		headWidth, xpStem;
	DDIST		lnSpace = LNSPACE(pContext);
	
	aGRNoteL = FindGRMainNote(grSyncL, voice);
	if (!aGRNoteL) return 0;
	
	xd = GRNoteXLoc(grSyncL, aGRNoteL, measLeft, HeadWidth(LNSPACE(pContext)),
								&xdNorm);
	aGRNote = GetPAGRNOTE(aGRNoteL);
	
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			xpStem = pContext->paper.left+d2p(xd);
			if (stemDir>0) {
				headWidth = GRACESIZE(MusFontStemSpaceWidthPixels(doc, doc->musFontInfoIndex, lnSpace));
				if (aGRNote->small) headWidth = SMALLSIZE(headWidth);
				xpStem += headWidth;
			}
			xd = p2d(xpStem-pContext->paper.left);
			break;

		case toPostScript:
			if (stemDir>0) {
				dHeadWidth = GRACESIZE(HeadWidth(LNSPACE(pContext)))+pt2d(1)/2;
				if (aGRNote->small) dHeadWidth = SMALLSIZE(dHeadWidth);
				xd += dHeadWidth;
			}
			break;
		default:
			;
	}
		
	return xd;
}

/* ------------------------------------------------------------------- GRNoteXStfYStem -- */
/* Return a ystem value for <aGRNote>, including an offset if the grace note is
in a cross staff beamset and is not on the top staff of the range. */

DDIST GRNoteXStfYStem(LINK aGRNoteL, STFRANGE stfRange, DDIST firstStfTop,
								DDIST lastStfTop, Boolean crossStaff)
{
	DDIST yBeam; PAGRNOTE aGRNote;

	aGRNote = GetPAGRNOTE(aGRNoteL);
	if (crossStaff) {
		if (GRNoteSTAFF(aGRNoteL)==stfRange.topStaff)
				yBeam = aGRNote->ystem;									/* Get primary beam pos. */
		else	yBeam = aGRNote->ystem+(lastStfTop-firstStfTop);
	}
	else yBeam = aGRNote->ystem;
	
	return yBeam;
}

/* ------------------------------------------------------------------ AnalyzeGRBeamset -- */
/* Get information about the given grace BEAMSET. Return (in *nPrimary) the number of
primary beams; (in nSecs[i]) the number of secondary beams in progress at the i'th
note of the beamset (always 0 for grace beams); and (as function value) the direction
of stems in the BEAMSET: -1=down, +1=up, 0=in both directions. */

short AnalyzeGRBeamset(LINK beamL, short *nPrimary, short nSecs[])
{
	LINK noteBeamL, grSyncL, aGRNoteL;
	PANOTEBEAM	pNoteBeam;
	register short iel;
	short startend, upOrDown[MAXINBEAM], firstUpOrDown, nestLevel, nPrim;
	short whichWay;
	
	/* Fill in a table of stem directions for all elements of the beam. */
	
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		pNoteBeam = GetPANOTEBEAM(noteBeamL);
		grSyncL = pNoteBeam->bpSync;
		aGRNoteL = FindGRMainNote(grSyncL, BeamVOICE(beamL));
		upOrDown[iel] = ( (GRNoteYSTEM(aGRNoteL) < GRNoteYD(aGRNoteL)) ? 1 : -1 );
	}

	/* Decide <whichWay> and compute the number of primary beams. */
	
	whichWay = firstUpOrDown = upOrDown[0];
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		pNoteBeam = GetPANOTEBEAM(noteBeamL);
		startend = pNoteBeam->startend;
		if (iel==0)
			nPrim = nestLevel = startend;
		else {
			if (upOrDown[iel]!=firstUpOrDown) whichWay = 0;
			if (iel<LinkNENTRIES(beamL)-1) {
				nestLevel += startend;
				if (nestLevel<nPrim) nPrim = nestLevel;
			}
		}
	}

	/* No secondary beams, since we allow beaming grace notes only when all have
		the same duration. */
		
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++)
		nSecs[iel] = 0;

	*nPrimary = nPrim;
	return whichWay;
}


/* -------------------------------------------------------------- BuildGRBeamDrawTable -- */
/* Given a GRBeamset and information about it, normally compiled by AnalyzeGRBeamset,
build a table of beam segments to be drawn by DrawGRBEAMSET or by the beam-
dragging feedback routine. */

short BuildGRBeamDrawTable(LINK beamL,
							short whichWay,	/* -1=all stems down, +1=all up, 0=in both directions */
							short nPrimary,
							short nSecs[],
							BEAMINFO beamTab[],
							short tabLen
							)
{
	LINK noteBeamL;
	PANOTEBEAM pNoteBeam;
	short startend[MAXINBEAM], nFrac[MAXINBEAM];
	short i, k, direction, curLevel, minUsedLev, maxUsedLev;
	register short iel, count;
	
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		pNoteBeam = GetPANOTEBEAM(noteBeamL);
		startend[iel] = pNoteBeam->startend;
		nFrac[iel] = pNoteBeam->fracs;
	}
	
	for (i = 0; i<tabLen; i++)
		beamTab[i].stop = -1;
		
	direction = (whichWay? whichWay : -1);
	
	/* First put primary beams into the table of beam segments (BEAMINFOs). */
	
	curLevel = -direction;
	for (count = 0; count<nPrimary; count++) {
		curLevel += direction;
		beamTab[count].start = 0;
		beamTab[count].startLev = beamTab[count].stopLev = curLevel;
	}
	startend[0] -= nPrimary;
	if (direction>0) { minUsedLev = 0; maxUsedLev = nPrimary-1; }
	else				  { maxUsedLev = 0; minUsedLev = -(nPrimary-1); }
	
	/*
	 * Build the rest of the table by combining the beam stacking information computed
	 * by AnalyzeGRBeamset with the three commands that are stored in NOTEBEAMs, namely
	 * "start non-fractional beams", "do fractional beams", and "end non-fractional
	 * beams". The commands are processed in that order. Fractional beams require no
	 * additional information, so their BEAMINFOs are completely filled in as soon as
	 * they're encountered.
	 */
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		if (startend[iel]>0) {											/* Start new BEAMINFOs */
			direction = (nSecs[iel]>0? 1 : -1);
			for (i = 0; i<startend[iel]; i++) {
				curLevel = (direction>0? ++maxUsedLev : --minUsedLev);
				beamTab[count].start = iel;
				beamTab[count].startLev = beamTab[count].stopLev = curLevel;
				count++;
				if (count>=tabLen) {
					MayErrMsg("BuildGRBeamDrawTable: table size exceeded.");
					return -1;
				}
			}
		}
		
		if (nFrac[iel]>0) {
			MayErrMsg("BuildGRBeamDrawTable: can't have fractional grace beams.");
			return -1;
		}
		
		if (startend[iel]<0) {											/* Finish up existing BEAMINFOs */
			k = count-1;
			for (i = 0; i<-startend[iel]; i++) {
				for ( ; k>=-1; k--)
					if (beamTab[k].stop<0) break;
				if (k<0) {
					MayErrMsg("BuildGRBeamDrawTable: couldn't find an unfinished segment.");
					return -1;
				}
				if (beamTab[k].startLev>=0) maxUsedLev--;
				else minUsedLev++;
				beamTab[k].stop = iel;
				beamTab[k].fracGoLeft = 0;								/* Will be ignored */
			}
		}
	}

#ifdef BDEBUG
	if (DETAIL_SHOW) {
		for (i = 0; i<count; i++) {
			if (i==0) LogPrintf(LOG_DEBUG, " Beam at %d ", beamL);
			else	  LogPrintf(LOG_DEBUG, "            ");
			LogPrintf(LOG_DEBUG, "seg %d: start=%d stop=%d level=%d fracGoLeft=%d\n",
							i, beamTab[i].start, beamTab[i].stop,
							beamTab[i].level, beamTab[i].fracGoLeft);
		}
	}
#endif
	for (i = tabLen-1; i>=0; i--)
		if (beamTab[i].stop>=0) return i+1;
	
	return -1;											/* Found no beam segments: something is wrong */
}


/* --------------------------------------------------------------------- DrawGRBEAMSET -- */
/* Draw all beam segments of a grace note BEAMSET object. There are primary beam
segments only: secondary and fractional beams can't occur in grace Beamsets. */

#define BEAMTABLEN 10	/* Should be plenty, since there are only primary beams */

void DrawGRBEAMSET(Document *doc, register LINK beamL, CONTEXT context[])
{
	PBEAMSET		pB;
	DDIST			dTop, ydelt,
					flagYDelta,
					beamThick, qdBeamThick, stdBeamThick,
					firstStfTop, lastStfTop, 		/* For cross-staff beams */
					yStem,
					startXStem, startYStem,
					stopXStem, stopYStem,
					dLeft[MAXINBEAM],
					xoffStart, xoffStop, 
					dQuarterSp, extension;
	register short	n, iel;
	short			whichWay, beamCount,
					startUOD, stopUOD,
					staff, voice,
					nPrimary, nSecs[MAXINBEAM];
	PANOTEBEAM		pNoteBeam;
	LINK			noteBeamL,
					startSyncL, startNoteL,
					stopSyncL, stopNoteL,
					grNotesInBeam[MAXINBEAM], beamGRSyncs[MAXINBEAM];
	register PCONTEXT pContext;
	CONTEXT			tempContext;
	Rect			beamRect, oldBeamRect,			/* Aux. rects for computing objRect. N.B. Really DRects. */
					newBeamRect;
	Rect			tempRect;
	Boolean			rectSet,						/* True if objRect aux. rects inited */
					crossStaff,
					dim,							/* Should it be dimmed bcs in a voice not being looked at? */
					crossSys, firstSys,				/* Whether beam is crossSystem, on firstSystem */
					allOneWay;						/* True if all stems are up or all stems are down */
	STFRANGE		stfRange;
	BEAMINFO		beamTab[BEAMTABLEN];

	staff = BeamSTAFF(beamL);
	voice = BeamVOICE(beamL);
	
	/* Fill grNotesInBeam[], an array of the notes in the beamVoice in the bpSyncs, in
	 *	order to call GetCrossStaff to get the stfRange.
	 */
	pB = GetPBEAMSET(beamL);
	crossStaff = False;
	if (pB->crossStaff) {
		crossStaff = True;
		GetBeamGRNotes(beamL, grNotesInBeam);
		GetGRCrossStaff(LinkNENTRIES(beamL), grNotesInBeam, &stfRange);
	}
	crossSys = pB->crossSystem;
	firstSys = pB->firstSystem;

	/* If beam's voice is not being "looked" at, dim it */
	dim = (outputTo==toScreen && !LOOKING_AT(doc, voice));
	if (dim) PenPat(NGetQDGlobalsGray());
	
	pContext = &context[staff];
	dTop = pContext->measureTop;
	if (crossStaff) {
		firstStfTop = context[staff].staffTop;
		lastStfTop = context[staff+1].staffTop;
	}
	flagYDelta = FlagLeading(LNSPACE(pContext));
	beamThick = (long)(config.beamLW*LNSPACE(pContext)) / 100L;
	beamThick = GRACESIZE(beamThick);
	if (pB->thin) beamThick /= 2;
	
	/* Be sure grace beams on QuickDraw output are at least 1 pixel thick. */

	stdBeamThick = LNSPACE(pContext)/2;
	qdBeamThick = (stdBeamThick<p2d(1)? p2d(1) : stdBeamThick);

	/*
	 * Beams on downstemmed grace notes are drawn "sitting" on the position of their
	 * notes' ystems, while beams on upstemmed notes are drawn "hanging" from that pos.
	 * We do this so the beam will always be contained within the specified stem
	 * length. This means that, for "center" beams (those that contain both down- and
	 * upstemmed notes), vertical positions of upstems and downstems must overlap. We
	 * achieve this by extending all upstems in center beams when the beams are
	 * created (see CreateGRBEAMSET et al). However, to draw such beams correctly, we
	 *	need more information.
	 */
	whichWay = AnalyzeGRBeamset(beamL, &nPrimary, nSecs);
	allOneWay = (whichWay!=0);
	extension = (allOneWay? 0 : beamThick+(nPrimary-1)*flagYDelta);
	beamCount = BuildGRBeamDrawTable(beamL, whichWay, nPrimary, nSecs, beamTab, BEAMTABLEN);
	if (beamCount<0) {
		SysBeep(1);
		return;
	}
	
	noteBeamL = FirstSubLINK(beamL);
	for (iel = 0; iel<LinkNENTRIES(beamL); iel++, noteBeamL=NextNOTEBEAML(noteBeamL)) {
		pNoteBeam = GetPANOTEBEAM(noteBeamL);
		beamGRSyncs[iel] = pNoteBeam->bpSync;
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
			GetContext(doc, beamGRSyncs[iel], staff, &tempContext);
			dLeft[iel] = tempContext.measureLeft;
		}

	/*
	 * We've got all the information we need. Draw the beam segments.
	 */
	rectSet = False;
	
	for (n = 0; n<beamCount; n++) {
		if (beamTab[n].start!=beamTab[n].stop) {	
			startSyncL = beamGRSyncs[beamTab[n].start];
			startNoteL = FindGRMainNote(startSyncL, voice);				
			startUOD = ( (GRNoteYSTEM(startNoteL) < GRNoteYD(startNoteL)) ? 1 : -1 );
	
			startXStem = CalcGRXStem(doc, startSyncL, voice, startUOD,
											dLeft[beamTab[n].start], pContext, True);
			
			yStem = GRNoteXStfYStem(startNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);
			if (startUOD>0) yStem += extension;
			ydelt = flagYDelta*beamTab[n].startLev;
			startYStem = dTop+yStem+ydelt;

			stopSyncL = beamGRSyncs[beamTab[n].stop];
			stopNoteL = FindGRMainNote(stopSyncL, voice);				
			stopUOD = ( (GRNoteYSTEM(stopNoteL) < GRNoteYD(stopNoteL)) ? 1 : -1 );

			stopXStem = CalcGRXStem(doc, stopSyncL, voice, stopUOD,
											dLeft[beamTab[n].stop], pContext, False);
			
			yStem = GRNoteXStfYStem(stopNoteL,stfRange,firstStfTop,lastStfTop,crossStaff);
			if (stopUOD>0) yStem += extension;
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
								stopXStem+xoffStop, stopYStem,
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
		}
	}
	
	for (n = 0; n<beamCount; n++) {
		if (beamTab[n].start==beamTab[n].stop)						/* FRACTIONAL BEAM */
			SysBeep(1);
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
	
		/* Restore full strength drawing after possible dimming for "look at voice" */
		PenPat(NGetQDGlobalsBlack());
	}
}


/* ================================================================= GENERAL UTILITIES == */

/* -------------------------------------------------------------------- GetBeamGRSyncs -- */
/*	Fill in arrays of grSync LINKs and grNote LINKs which can be used to fill in the
beamset's subobjects; return True if things are OK, False if not. */
	
static Boolean GetBeamGRSyncs(Document */*doc*/, LINK startL, LINK endL, short v,
								short nInBeam, LINK bpGRSync[], LINK GRNoteInSync[],
								Boolean needSel)
{
	short bel; LINK thisL, aGRNoteL; PAGRNOTE aGRNote;

	for (bel=0, thisL=startL; thisL!=endL; thisL=RightLINK(thisL))
		if (GRSyncTYPE(thisL) && (!needSel || LinkSEL(thisL))) {
			aGRNoteL = FirstSubLINK(thisL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL);
				if (aGRNote->voice==v)	{						/* This note in desired voice */
					aGRNote->beamed = True;
					if (GRMainNote(aGRNoteL)) {
						bpGRSync[bel] = thisL;	
						GRNoteInSync[bel] = aGRNoteL;
						bel++;									/* Increment no. of grNotes in beamset */
					}
				}
			}
		}

	if (bel!=nInBeam) {
		MayErrMsg("GetBeamGRSyncs: expected %ld beamable notes but found %ld.",
					(long)nInBeam, (long)bel);
		return False;
	}
	
	return True;
}


/* --------------------------------------------------------------- CountGRBeamable -- */
/* If range is beamable for the specified voice, returns no. of GRSyncs with grace
notes in it;  otherwise returns 0. Notice that range can't really be beamed if
the result is 1. */

short CountGRBeamable(Document */*doc*/,			/* unused */
							LINK startL, LINK endL,
							short voice,
							Boolean needSelected	/* True if we only want selected items */
							)
{
	short	nbeamable,dur,staff;
	LINK	pL,aGRNoteL,firstL=NILINK,lastL=NILINK;

	if (voice<1 || voice>MAXVOICES)
		MayErrMsg("CountGRBeamable: illegal voice number %ld", (long)voice);
		
	if (endL==startL) return 0;										/* Reject empty range */
	dur = -99;
	for (nbeamable = 0, pL = startL; pL!=endL; pL = RightLINK(pL))
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				if (GRNoteVOICE(aGRNoteL)==voice)	{				/* This grace note in desired voice? */
					if (dur<0) {									/* GRNotes have neg duration */
						dur = GRNoteSUBTYPE(aGRNoteL);
						staff = GRNoteSTAFF(aGRNoteL);
						firstL = pL;
					}
					if ((needSelected && !GRNoteSEL(aGRNoteL)) ||	/* Not selected? */
						(NFLAGS(GRNoteSUBTYPE(aGRNoteL))<1) ||		/* Unbeamable? */
						(GRNoteBEAMED(aGRNoteL)) || 				/* Already beamed? */
						(GRNoteSUBTYPE(aGRNoteL)!=dur) ||			/* Change of duration? */
						(GRNoteSTAFF(aGRNoteL)!=staff))				/* Change of staff? Reject range */
								return 0;
					else
						{ lastL = pL; nbeamable++; break; }			/* Still possible */
				}
			}
		}

#ifdef DO_ALLOW_INTERVENING_NOTES
		/* If comment this in, need an extra bracket around the for loop. */
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteVOICE(aNoteL)==voice)						/* An intervening note in this voice? */
					return 0;
		}
#endif

	if (firstL && lastL) {
		if (!SameSystem(firstL, lastL)) return 0;
		if (!SameMeasure(firstL, lastL)) return 0;
	}
	return nbeamable;
}


/* ------------------------------------------------------------------ VHasGRBeamAcross -- */
/* If the point just before node in the given voice is in the middle of a
beam, VHasGRBeamAcross returns a pointer to the BEAMSET, otherwise NULL. */

LINK VHasGRBeamAcross(LINK node, short voice)	
{
	LINK		lSyncL, rSyncL, lNoteL, rNoteL, beamL;
	SearchParam	pbSearch;

	if (node==NILINK) return NILINK;	

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = False;
	pbSearch.inSystem = False;
	lSyncL = L_Search(LeftLINK(node), GRSYNCtype, GO_LEFT, &pbSearch); 	/* Look for 1st note left */
	rSyncL = L_Search(node, GRSYNCtype, GO_RIGHT, &pbSearch);			/* Look for 1st note right */
	if (lSyncL==NILINK || rSyncL==NILINK)	return NILINK;				/* Found neither */

	lNoteL = FirstSubLINK(lSyncL);										/* At left... */
	for ( ; lNoteL; lNoteL = NextGRNOTEL(lNoteL))
		if (GRNoteVOICE(lNoteL)==voice)									/* This note on desired voice? */
			if (!GRNoteBEAMED(lNoteL)) return NILINK;					/* Yes. Beamed? If not, we're done. */
	
	rNoteL = FirstSubLINK(rSyncL);										/* At right... */
	for ( ; rNoteL; rNoteL = NextGRNOTEL(rNoteL))
		if (GRNoteVOICE(rNoteL)==voice) {								/* This note in desired voice? */
			if (!GRNoteBEAMED(rNoteL)) return NILINK;					/* Yes. Beamed? If not, we're done. */
			pbSearch.subtype = GRNoteBeam;
			beamL = L_Search(rSyncL, BEAMSETtype, GO_LEFT, &pbSearch); 	/* Are they in the same beam? */
			if (beamL)
				if (IsAfter(beamL, lSyncL)) return beamL;				/* Yes */
			return NILINK;												/* No */
		}

	return NILINK;														/* Should never get here */
}


/* ------------------------------------------------------------ VCheckGRBeamAcross -- */
/* If the point just before <node> in the given voice is in the middle of a beam,
VCheckBeamAcross returns a pointer to the BEAMSET, otherwise NULL. Identical to
VHasBeamAcross except that the search to the right starts just after node. */

LINK VCheckGRBeamAcross(LINK node, short voice)	
{
	LINK		lGRSyncL, rGRSyncL, lGRNoteL, rGRNoteL, beamL;
	SearchParam	pbSearch;

	if (node==NILINK) return NILINK;	

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = False;
	pbSearch.inSystem = False;

	lGRSyncL = L_Search(LeftLINK(node), GRSYNCtype, GO_LEFT, &pbSearch);	/* Look for 1st note left */
	rGRSyncL = L_Search(RightLINK(node), GRSYNCtype, GO_RIGHT, &pbSearch);	/* Look for 1st note right */
	if (lGRSyncL==NILINK || rGRSyncL==NILINK)	return NILINK;
	
	lGRNoteL = FirstSubLINK(lGRSyncL);										/* At left... */
	for ( ; lGRNoteL; lGRNoteL = NextGRNOTEL(lGRNoteL))
		if (GRNoteVOICE(lGRNoteL)==voice)									/* This note in desired voice? */
			if (!GRNoteBEAMED(lGRNoteL)) return NILINK;						/* Yes. Beamed? If not, we're done. */

	rGRNoteL = FirstSubLINK(rGRSyncL);										/* At right... */
	for ( ; rGRNoteL; rGRNoteL = NextGRNOTEL(rGRNoteL))
		if (GRNoteVOICE(rGRNoteL)==voice)	{								/* This note on desired voice? */
			if (!GRNoteBEAMED(rGRNoteL)) return NILINK;						/* Yes. Beamed? If not, we're done. */
			pbSearch.subtype = GRNoteBeam;
			beamL = L_Search(rGRSyncL, BEAMSETtype, GO_LEFT, &pbSearch);	/* Are they in the same beam? */
			if (beamL)
				if (IsAfter(beamL, lGRSyncL)) return beamL;					/* Yes */
			return NILINK;
		}

	return NILINK;																/* Should never get here */
}

LINK VCheckGRBeamAcrossIncl(LINK node, short voice)	
{
	LINK		beamL, aGRNoteL;
	SearchParam pbSearch;

	beamL = VCheckGRBeamAcross(node, voice);
	if (beamL) return beamL;
	if (!GRSyncTYPE(node)) return NILINK;

	aGRNoteL = FirstSubLINK(node);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
		if (GRNoteVOICE(aGRNoteL)==voice) {
			if (GRNoteBEAMED(aGRNoteL)) {
				InitSearchParam(&pbSearch);
				pbSearch.id = ANYONE;
				pbSearch.voice = voice;
				pbSearch.needSelected = pbSearch.inSystem = False;
				pbSearch.subtype = GRNoteBeam;
				return L_Search(node, BEAMSETtype, GO_LEFT, &pbSearch);
			}
			return NILINK;
		}
	return NILINK;
}

/* Truncated version of Unbeam which simply sets aGRNote->beamed False for
all notes in Beamset, deletes the Beamset, and optionally fixes stems of
notes/chords in the Beamset. */

void RemoveGRBeam(Document *doc, LINK beamL, short	voice, Boolean /*fixStems*/)
{
	LINK		firstGRSyncL, lastGRSyncL, pL, aGRNoteL;
	PAGRNOTE	aGRNote;

	firstGRSyncL = FirstInBeam(beamL);
	lastGRSyncL = LastInBeam(beamL);

	for (pL = firstGRSyncL; pL!=RightLINK(lastGRSyncL); pL = RightLINK(pL))
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL);
				if (aGRNote->voice==voice) aGRNote->beamed = False;
			}
		}
	DeleteNode(doc, beamL);
	
}

/* Recompute all grace note stems in the range [startL, endL] */

void RecomputeGRNoteStems(Document *doc, LINK startL, LINK endL, short v)
{
	LINK pL, aGRNoteL;
	CONTEXT context;

	for (pL = startL; pL!=RightLINK(endL); pL = RightLINK(pL))
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FindGRMainNote(pL, v);
			if (aGRNoteL) {
				GetContext(doc, startL, GRNoteSTAFF(aGRNoteL), &context);
				if (GRNoteINCHORD(aGRNoteL))
					FixGRSyncForChord(doc, pL, v, False, 0, 0, &context);
				else
					FixGRSyncNote(doc, pL, v, &context);
			}
		}
}
