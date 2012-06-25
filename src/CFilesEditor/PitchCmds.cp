/***************************************************************************
	FILE:	PitchCmds.c
	PROJ:	Nightingale, rev. for v.99
	DESC:	Commands that affect or check pitch or pitch notation, plus
			Flip Direction.
		
	FindExtremeNote		SetNRCStem				FlipBeamList
	FlipSelDirection		Respell
	Transpose				DTranspose				TransposeKey
	CheckRange	
/***************************************************************************/

/*										NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1992-99 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Local variables and functions */

LINK FindExtremeNote(LINK, short, short);
LINK FindExtremeGRNote(LINK, short, short);
void SetNRCStem(LINK, LINK, Boolean, DDIST);
void SetGRNStem(LINK, LINK, Boolean, DDIST);
static void FlipBeamList(Document *, LINK [], short);

/* --------------------------------------------------------- FindExtreme(GR)Note -- */

LINK FindExtremeNote(LINK syncL,
							short voice,
							short stemUpDown)		/* 1=up, -1=down */
{
	LINK lowNoteL, hiNoteL;

	GetExtremeNotes(syncL, voice, &lowNoteL, &hiNoteL);
	
	return (stemUpDown<0? hiNoteL : lowNoteL);
}

LINK FindExtremeGRNote(LINK grSyncL,
								short voice,
								short stemUpDown)		/* 1=up, -1=down */
{
	LINK lowGRNoteL, hiGRNoteL;

	GetExtremeGRNotes(grSyncL, voice, &lowGRNoteL, &hiGRNoteL);
	
	return (stemUpDown<0? hiGRNoteL : lowGRNoteL);
}

/* -------------------------------------------------------------- SetNRC/GRNStem -- */

void SetNRCStem(LINK syncL, LINK aNoteL, Boolean makeLower, DDIST stemLen)
{
	DDIST ystem; short voice;
	
	if (NoteREST(aNoteL)) {
#ifdef NOTYET
		GetContext()
		yqpit = GetRestMultivoiceRole(&context, voiceRole, makeLower);
		NoteYD(aNoteL) = qd2d(yqpit, context.staffHeight,
								context.staffLines);
		LinkVALID(syncL) = FALSE;								/* Rest's sel. area may have moved */
#endif
	}
	else if (NoteINCHORD(aNoteL)) {
		voice = NoteVOICE(aNoteL);
		aNoteL = FindExtremeNote(syncL, voice, (makeLower? -1 : 1));
		ystem = NoteYD(aNoteL)+(makeLower? stemLen : -stemLen);
		FixChordForYStem(syncL, voice, (makeLower? -1 : 1), ystem);
	}
	else {
		ystem = NoteYD(aNoteL)+(makeLower? stemLen : -stemLen);
		NoteYSTEM(aNoteL) = ystem;
	}
}

void SetGRNStem(LINK grSyncL, LINK aGRNoteL, Boolean makeLower, DDIST stemLen)
{
	DDIST ystem; short voice;
	
	if (GRNoteINCHORD(aGRNoteL)) {
		voice = GRNoteVOICE(aGRNoteL);
		aGRNoteL = FindExtremeGRNote(grSyncL, voice, (makeLower? -1 : 1));
		ystem = GRNoteYD(aGRNoteL)+(makeLower? stemLen : -stemLen);
		FixGRChordForYStem(grSyncL, voice, (makeLower? -1 : 1), ystem);
	}
	else {
		ystem = GRNoteYD(aGRNoteL)+(makeLower? stemLen : -stemLen);
		GRNoteYSTEM(aGRNoteL) = ystem;
	}
}

/* ---------------------------------------------------------------- FlipBeamList -- */
/* Flip the Beamsets in the list beamLA[] by unbeaming each and rebeaming it with
a voice role that'll make the stem directions opposite to the first note/chord's
previous direction. Doesn't worry about center beams nor about preserving stem
lengths. */

static void FlipBeamList(Document *doc, LINK beamLA[], short nBeamsets)
{
	short i, voice, nEntries, newRole;
	LINK startL, endL, mainNoteL, newBeamL, mainGRNoteL;
	Boolean stemDown, crossSystem, firstSystem, graceBeam;
	
	for (i = 0; i<nBeamsets; i++) {
		startL = FirstInBeam(beamLA[i]);
		endL = LastInBeam(beamLA[i]);
		voice = BeamVOICE(beamLA[i]);
		graceBeam = GRSyncTYPE(startL);
		if (graceBeam) {
			mainGRNoteL = FindGRMainNote(startL, voice);
			stemDown = GRNoteYSTEM(mainGRNoteL)>GRNoteYD(mainGRNoteL);
		}
		else {
			mainNoteL = FindMainNote(startL, voice);
			stemDown = NoteYSTEM(mainNoteL)>NoteYD(mainNoteL);
		}
		newRole = (stemDown? UPPER_DI : LOWER_DI);
		nEntries = LinkNENTRIES(beamLA[i]);

		/*
		 * The Beamset might be part of a cross-staff pair: save the cross-staff flags
		 * before unbeaming and restore them in the new Beamset.
		 */ 
		crossSystem = BeamCrossSYS(beamLA[i]);
		firstSystem = BeamFirstSYS(beamLA[i]);

		UnbeamV(doc, startL, endL, voice);
		if (graceBeam)
			newBeamL = CreateGRBEAMSET(doc, startL, RightLINK(endL), voice, nEntries, TRUE,
												TRUE);
		else
			newBeamL = CreateBEAMSET(doc, startL, RightLINK(endL), voice, nEntries, TRUE,
												newRole);

		BeamCrossSYS(newBeamL) = crossSystem;
		BeamFirstSYS(newBeamL) = firstSystem;
	}
	
	UpdateSelection(doc);
}


/* ------------------------------------------------------------ FlipSelDirection -- */
/* Flip the up/down direction of every selected slur, note/rest/chord, and grace
note/chord. If any note or grace note in a chord is selected, flip it. If any note
or grace note in a beam is selected, flip it.

For slurs, needs to call InvalMeasures in case slur extends outside of its measure
or system. Don't need call to InvalSelRange because slur's proximity to its firstSyncL
is guaranteed: it must be in same  measure of datastructurally. However, graphically,
slur could have been dragged to a place which bears no relation to its containing
measure, firstSyncL, or lastSyncL; we need a call to InvalObject if we wish to handle
this.

This function potentially calls Inval routines lots of times, which can be slow;
with some work, all the calls could be combined into one or, at most, a few. */

void FlipSelDirection(Document *doc)
{
	LINK pL, aNoteL, startL, endL, beamL, aGRNoteL;
	short v; DDIST stemLen;
	short i, nBeamsets=0; Boolean found;
	LINK beamLA[MAX_MEASNODES];	/* Enough for any reasonable but not any possible situation */
	
	PrepareUndo(doc, doc->selStartL, U_FlipSlurs, 33);			/* "Undo Flip Direction" */
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL)) {
			switch (ObjLType(pL)) {
				case SLURtype:
					if (FlipSelSlur(pL))
						InvalMeasures(SlurFIRSTSYNC(pL), SlurLASTSYNC(pL), ANYONE);
				 	break;
				case SYNCtype:
					for (v = 1; v<=MAXVOICES; v++)
						if (VOICE_MAYBE_USED(doc, v)) {
							aNoteL = NoteInVoice(pL, v, TRUE);
							if (aNoteL) {
								aNoteL = FindMainNote(pL, NoteVOICE(aNoteL));
								/*
								 *	If this note is beamed, we can't set its stem correctly now,
								 * since other notes in the beamset may follow, but remember
								 * its owning beamset for later use; if we run out of space to
								 *	remember it, just unbeam so at least we won't end up with
								 *	stems and beams that disagree.
								 */
								if (NoteBEAMED(aNoteL)) {
									beamL = LVSearch(pL, BEAMSETtype, NoteVOICE(aNoteL), GO_LEFT,
															FALSE);
									startL = FirstInBeam(beamL);
									endL = LastInBeam(beamL);
									if (nBeamsets<MAX_MEASNODES) {
										for (found = FALSE, i = 0; i<nBeamsets; i++)
											if (beamL==beamLA[i]) found = TRUE;
										if (!found)
											beamLA[nBeamsets++] = beamL;
									}
									else
										UnbeamV(doc, startL, endL, NoteVOICE(aNoteL));
								}
								else {
									stemLen = NoteYSTEM(aNoteL)-NoteYD(aNoteL);
									SetNRCStem(pL, aNoteL, stemLen<0, ABS(stemLen));
									InvalMeasure(pL, ANYONE);
								}
							}
						}
				 	break;
				case GRSYNCtype:
					for (v = 1; v<=MAXVOICES; v++)
						if (VOICE_MAYBE_USED(doc, v)) {
							aGRNoteL = GRNoteInVoice(pL, v, TRUE);
							if (aGRNoteL) {
								aGRNoteL = FindGRMainNote(pL, GRNoteVOICE(aGRNoteL));
								/*
								 *	If this grace note is beamed, we can't set its stem correctly
								 * now since other notes in the beamset may follow, but remember
								 * its owning beamset for later use; if we run out of space to
								 *	remember it, just unbeam so at least we won't end up with
								 *	stems and beams that disagree.
								 */
								if (GRNoteBEAMED(aGRNoteL)) {
									beamL = LVSearch(pL, BEAMSETtype, GRNoteVOICE(aGRNoteL),
															GO_LEFT, FALSE);
									startL = FirstInBeam(beamL);
									endL = LastInBeam(beamL);
									if (nBeamsets<MAX_MEASNODES) {
										for (found = FALSE, i = 0; i<nBeamsets; i++)
											if (beamL==beamLA[i]) found = TRUE;
										if (!found)
											beamLA[nBeamsets++] = beamL;
									}
									else
										UnbeamV(doc, startL, endL, GRNoteVOICE(aGRNoteL));
								}
								else {
									stemLen = GRNoteYSTEM(aGRNoteL)-GRNoteYD(aGRNoteL);
									SetGRNStem(pL, aGRNoteL, stemLen<0, ABS(stemLen));
									InvalMeasure(pL, ANYONE);
								}
							}
						}
					break;
				default:
					;
			}
		}
			
	FlipBeamList(doc, beamLA, nBeamsets);
	doc->changed = TRUE;
}


/* --------------------------------------------------------------------- Respell -- */
/* Respell every selected note in the "obvious" way, if there is one, as described
in the comments for RespellNote. Also add accidentals to following notes where
necessary to keep their pitches the same. For example, if one voice ??STAFF? of the
selection contains Db,C with no barlines between, the Db gets changed to C# and the
C has a natural added to it. Similarly, if the last selected note in a voice is Db
and the first unselected note in that voice is C with no barlines between, the two
notes again get changed to C#,C-natural. After doing the respelling, we fix notes'
and chords' graphic characteristics (stem length and direction, etc.) and delete
redundant accidentals, which the process tends to produce, both because of iteration
and as described just before and in RespellNote. Return TRUE if we do anything, else
FALSE. */

Boolean Respell(Document *doc)
{
	LINK		pL, aNoteL, aGRNoteL;
	PGRAPHIC	pGraphic;
	short		v, s;
	CONTEXT	context[MAXSTAVES+1];
	PCONTEXT	pContext;
	Boolean	changedNotes, changedChords, voiceChanged[MAXVOICES+1];
	
	WaitCursor();

	for (v = 0; v<=MAXVOICES; v++)
		voiceChanged[v] = FALSE;
	changedNotes = changedChords = FALSE;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL)) {
			switch (ObjLType(pL)) {
				case GRAPHICtype:
					pGraphic = GetPGRAPHIC(pL);
					if (pGraphic->graphicType==GRChordSym)
						if (RespellChordSym(doc, pL))
							changedChords = TRUE;
					break;
				case SYNCtype:
					GetAllContexts(doc, context, pL);
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteSEL(aNoteL) && !NoteREST(aNoteL)) {
							pContext = &context[NoteSTAFF(aNoteL)];
							if (RespellNote(doc, pL, aNoteL, pContext)) {
								changedNotes = TRUE;
								voiceChanged[NoteVOICE(aNoteL)] = TRUE;
							}

						}
						for (v = 0; v<=MAXVOICES; v++)
							if (voiceChanged[v]) FixRespelledVoice(doc, pL, v);
					break;
				case GRSYNCtype:
					GetAllContexts(doc, context, pL);
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
						if (GRNoteSEL(aGRNoteL) ) {
							pContext = &context[GRNoteSTAFF(aGRNoteL)];
							if (RespellGRNote(doc, pL, aGRNoteL, pContext)) {
								changedNotes = TRUE;
								voiceChanged[GRNoteVOICE(aGRNoteL)] = TRUE;
							}

						}
						for (v = 0; v<=MAXVOICES; v++)
							if (voiceChanged[v]) FixRespelledVoice(doc, pL, v);
					break;
				default:
					;
			}
		}
	if (changedNotes) {
		DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);
		for (s = 1; s<=doc->nstaves; s++)
			FixBeamsInRange(doc, doc->selStartL, doc->selEndL, s, TRUE);
	}
	if (changedNotes || changedChords) {
		InvalRange(doc->selStartL, doc->selEndL);							/* Update objRects */
		doc->changed = TRUE;
		return TRUE;
	}
	else
		return FALSE;
}


/* ------------------------------------------------------------------- Transpose -- */
/* Transpose every selected note and chord symbol by the specified amount. For
example, transposing down a minor 10th would be indicated goUp=FALSE, octaves=1,
steps=2, semitones=15; down an augmented unison goUp=FALSE, octaves=0, steps=0,
semitones=1.  Also add accidentals to following notes where necessary to keep their
pitches the same, as described in comments on the Respell function above. Return
TRUE if we actually change anything, else FALSE. */

Boolean Transpose(
				Document *doc,
				Boolean	goUp,					/* TRUE=transpose up, else down */
				short		octaves,				/* Unsigned no. of octaves transposition */
				short		steps,				/* Unsigned no. of diatonic steps transposition */
				short		semiChange,			/*	Unsigned total transposition in semitones */
				Boolean	slashes 				/* Transpose notes with slash-appearance heads? */
				)
{
	LINK		pL, aNoteL, aGRNoteL;
	PGRAPHIC	pGraphic;
	short		semiChangeOct,							/* Including octave change */
				v, s,
				spellingBad, chordSpellingBad, lowMIDINum, hiMIDINum;
	CONTEXT	context[MAXSTAVES+1];
	Boolean	changedNotes, changedChords,
				voiceChanged[MAXVOICES+1];
	char fmtStr[256];
	
	if (octaves==0 && steps==0 && semiChange==0) return FALSE;
	
	if (!goUp) { octaves *= -1; steps *= -1; semiChange *= -1; }
	semiChangeOct = semiChange+12*octaves;

	/*
	 * We really COULD transpose double-sharps up an aug. unison, but there's a nasty
	 *	bug, and it's really not worth fixing, so we don't allow it.
	 */
	if (steps==0 && semiChange==1)
		if (FindSelAcc(doc, AC_DBLSHARP)) {
			GetIndCString(strBuf, PITCHERRS_STRS, 1);			/* "Nightingale can't transpose double-sharps up an augmented unison." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}

	GetSelMIDIRange(doc, &lowMIDINum, &hiMIDINum);
	if (lowMIDINum+semiChangeOct<1
	||   hiMIDINum+semiChangeOct>127) {							/* 127=MAX_NOTENUM */
		GetIndCString(strBuf, PITCHERRS_STRS, 2);				/* "Transposing would lead to MIDI note number(s) below 1 or above 127." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	WaitCursor();

	for (v = 0; v<=MAXVOICES; v++)
		voiceChanged[v] = FALSE;
	changedNotes = changedChords = FALSE;
	spellingBad = chordSpellingBad = 0;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL)) {
			switch (ObjLType(pL)) {
				case GRAPHICtype:
					pGraphic = GetPGRAPHIC(pL);
					if (pGraphic->graphicType==GRChordSym) {
						if (!TranspChordSym(doc, pL, steps, semiChange))
							chordSpellingBad++;
						changedChords = TRUE;
					}
					break;
				case SYNCtype:
					GetAllContexts(doc, context, pL);
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteSEL(aNoteL) && !NoteREST(aNoteL)) {
							if (NoteAPPEAR(aNoteL)!=SLASH_SHAPE || slashes) {
								if (!TranspNote(doc, pL, aNoteL, context[NoteSTAFF(aNoteL)],
														octaves, steps, semiChange))
										spellingBad++;
								changedNotes = TRUE;
								voiceChanged[NoteVOICE(aNoteL)] = TRUE;
							}
						}
					for (v = 0; v<=MAXVOICES; v++)
						if (voiceChanged[v]) FixRespelledVoice(doc, pL, v);
					break;
				case GRSYNCtype:
					GetAllContexts(doc, context, pL);
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
						if (GRNoteSEL(aGRNoteL)) {
							if (!TranspGRNote(doc, pL, aGRNoteL, context[GRNoteSTAFF(aGRNoteL)],
													octaves, steps, semiChange))
									spellingBad++;
							changedNotes = TRUE;
							voiceChanged[GRNoteVOICE(aGRNoteL)] = TRUE;
						}
					for (v = 0; v<=MAXVOICES; v++)
						if (voiceChanged[v]) FixRespelledVoice(doc, pL, v);
					break;
				default:
					;
				}
			}
		
	if (spellingBad) {
		GetIndCString(fmtStr, PITCHERRS_STRS, 6);    /* "Transposing %d note(s) led to at least triple sharps or flats; they were simplified." */
		sprintf(strBuf, fmtStr, spellingBad); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	if (chordSpellingBad) {
		GetIndCString(fmtStr, PITCHERRS_STRS, 7);    /* "Transposing %d chord symbol(s) led to at least triple sharps or flats; they were simplified." */
		sprintf(strBuf, fmtStr, chordSpellingBad); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	if (changedNotes) {
		DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);
		for (s = 1; s<=doc->nstaves; s++)
			FixBeamsInRange(doc, doc->selStartL, doc->selEndL, s, TRUE);
	}

	if (changedNotes || changedChords) {
		InvalRange(doc->selStartL, doc->selEndL);							/* Update objRects */
		doc->changed = TRUE;
		return TRUE;
	}
	else
		return FALSE;
}


/* ------------------------------------------------------------------ DTranspose -- */
/* Diatonically (according to the current key signature) transpose every selected
note by the specified amount. For example, transposing down a 10th would be indicated
goUp=FALSE, octaves=1, steps=2; up a second goUp=TRUE, octaves=0, steps=1.  Also add
accidentals to following notes where necessary to keep their pitches the same, as
described in comments on the Respell function above. Return TRUE if we actually change
anything, else FALSE.

Like Semipro Composer, we leave all accidentals on transposed notes alone and simply
move the notes up or down, although we may change accidentals on following notes to
keep their pitches constant. After making this transformation, we remove redundant
accidentals.
*/

Boolean DTranspose(
				Document *doc,
				Boolean	goUp,					/* TRUE=transpose up, else down */
				short		octaves,				/* Unsigned no. of octaves transposition */
				short		steps,				/* Unsigned no. of diatonic steps transposition */
				Boolean	slashes 				/* Transpose notes with slash-appearance heads? */
				)
{
	LINK		pL, aNoteL, aGRNoteL;
	PGRAPHIC	pGraphic;
	short		v, s;
	//short		lowMIDINum, hiMIDINum;
	CONTEXT	context[MAXSTAVES+1];
	Boolean	changedNotes, changedChords,
				voiceChanged[MAXVOICES+1];
	
	if (octaves==0 && steps==0) return FALSE;
	
	if (!goUp) { octaves *= -1; steps *= -1; }

#ifdef NOTYET
	/* It's hard to know the MIDI note nos. we're going to get exactly but easy
		to know within one semitone, so just do an approximate check. */
			
	semiChangeOct = ??;
	GetSelMIDIRange(doc, &lowMIDINum, &hiMIDINum);
	if (lowMIDINum+semiChangeOct<2
	||   hiMIDINum+semiChangeOct>126) {
		GetIndCString(strBuf, PITCHERRS_STRS, 3);    /* "Transposing might lead to MIDI note number(s) below 1 or above 127." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
#endif

	WaitCursor();

	for (v = 0; v<=MAXVOICES; v++)
		voiceChanged[v] = FALSE;
	changedNotes = changedChords = FALSE;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL)) {
			switch (ObjLType(pL)) {
				case GRAPHICtype:
					pGraphic = GetPGRAPHIC(pL);
					if (pGraphic->graphicType==GRChordSym) {
						DTranspChordSym(doc, pL, steps);
						changedChords = TRUE;
					}
					break;
				case SYNCtype:
					GetAllContexts(doc, context, pL);
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteSEL(aNoteL) && !NoteREST(aNoteL)) {
							if (NoteAPPEAR(aNoteL)!=SLASH_SHAPE || slashes) {
								DTranspNote(doc, pL, aNoteL, context[NoteSTAFF(aNoteL)],
													goUp, octaves, steps);
								changedNotes = TRUE;
								voiceChanged[NoteVOICE(aNoteL)] = TRUE;
							}
						}
					for (v = 0; v<=MAXVOICES; v++)
						if (voiceChanged[v]) FixRespelledVoice(doc, pL, v);
					break;
				case GRSYNCtype:
					GetAllContexts(doc, context, pL);
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
						if (GRNoteSEL(aGRNoteL)) {
							DTranspGRNote(doc, pL, aGRNoteL, context[GRNoteSTAFF(aGRNoteL)],
												goUp, octaves, steps);
							changedNotes = TRUE;
							voiceChanged[GRNoteVOICE(aGRNoteL)] = TRUE;
						}
					for (v = 0; v<=MAXVOICES; v++)
						if (voiceChanged[v]) FixRespelledVoice(doc, pL, v);
					break;
				default:
					;
			}
		}
		
	if (changedNotes) {
		DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);
		for (s = 1; s<=doc->nstaves; s++)
			FixBeamsInRange(doc, doc->selStartL, doc->selEndL, s, TRUE);
	}
	if (changedNotes || changedChords) {
		InvalRange(doc->selStartL, doc->selEndL);							/* Update objRects */
		doc->changed = TRUE;
		return TRUE;
	}
	else
		return FALSE;
}


/* -------------------------------------------------------- StfDelRedundantAccs -- */
/* On the given staves, if code=DELSOFT_REDUNDANTACCS_DI, delete soft accidentals
that are redundant (because of the key signature and/or accidentals earlier in the
bar); if code=DELALL_REDUNDANTACCS_DI, delete all accidentals that are redundant;
otherwise do nothing. If it found any to delete, return TRUE, else FALSE. Takes no
user-interface actions, e.g., redrawing. Cf. DelRedundantAccs, which does the same
thing but decides what to do it to based on selection and either one or all staff,
instead of on any no. of staves in their entirety.

This assumes standard CMN accidental-carrying rules including ties across barlines.
If ACC_IN_CONTEXT is FALSE, it should probably do nothing.
*/

Boolean StfDelRedundantAccs(Document *, short, Boolean []);
Boolean StfDelRedundantAccs(Document *doc, short code, Boolean trStaff[])
{
	LINK pL, aNoteL;
	LINK aGRNoteL;
	Boolean didAnything, syncVChanged[MAXVOICES+1];
	short v;

	if (code!=DELALL_REDUNDANTACCS_DI && code!=DELSOFT_REDUNDANTACCS_DI) return FALSE;

	didAnything = FALSE;
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				for (v = 1; v<=MAXVOICES; v++)
					syncVChanged[v] = FALSE;
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (trStaff[NoteSTAFF(aNoteL)])
						if (DelNoteRedAcc(doc, code, pL, aNoteL, syncVChanged))
							didAnything = TRUE;						
				ArrangeSyncAccs(pL, syncVChanged);
				break;
			case GRSYNCtype:
				for (v = 1; v<=MAXVOICES; v++)
					syncVChanged[v] = FALSE;
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
					if (trStaff[GRNoteSTAFF(aGRNoteL)])
						if (DelGRNoteRedAcc(doc, code, pL, aGRNoteL, syncVChanged))
							didAnything = TRUE;						
				/* ArrangeGRSyncAccs(pL, syncVChanged); ??NOT WRITTEN YET */
				break;
		default:
			;
		}
		

	if (didAnything) doc->changed = TRUE;
	return didAnything;

}


/* ---------------------------------------------------------------- TransposeKey -- */
/* In staves with the given numbers, for the entire score, transpose zero or more of
key signatures, notes, and chord symbols by the specified amount. For example,
transposing down a minor 3rd would be indicated goUp=FALSE, steps=2, semitones=3;
down an augmented unison goUp=FALSE, steps=0, semitones=1.  Also add accidentals to
following notes where necessary to keep their pitches the same, as described in
comments on the Respell function above. Return TRUE if we actually change anything,
else FALSE. */

Boolean TransposeKey(
				Document *doc,
				Boolean	goUp,					/* TRUE=transpose up, else down */
				short		octaves,				/* Unsigned no. of octaves transposition */
				short		steps,				/* Unsigned no. of diatonic steps transposition */
				short		semiChange,			/*	Unsigned total transposition in semitones */
				Boolean	trStaff[],			/* Staff nos. to be transposed */
				Boolean	slashes,				/* Transpose notes with slash-appearance heads? */
				Boolean	notes,				/* Transpose other (non-slash-appearance) notes? */
				Boolean	chordSyms 			/* Transpose chord symbols? */
				)
{
	LINK		pL, aKeySigL, aNoteL, aGRNoteL;
	PGRAPHIC	pGraphic;
	short		semiChangeOct,							/* Including octave change */
				v, s, spaceProp,
				ksSpellingBad, spellingBad, chordSpellingBad, lowMIDINum, hiMIDINum;
	CONTEXT	context[MAXSTAVES+1];
	Boolean	changedKeys, changedNotes, changedChords,
				voiceChanged[MAXVOICES+1];
	char		fmtStr[256];
	
	if (octaves==0 && steps==0 && semiChange==0) return FALSE;
		
	if (!goUp) { octaves *= -1; steps *= -1; semiChange *= -1; }
	semiChangeOct = semiChange+12*octaves;

	/* We've now converted octaves, steps, and semiChange to signed values. */
	
	if (ABS(steps)==1 && semiChange==0) {
		GetIndCString(strBuf, PITCHERRS_STRS, 4);    /* "Nightingale can't transpose key signatures a diminished second." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	
	/* ??Following checking is wrong, since notes are affected even if they're not selected! */
	if (steps==0 && semiChange==1)
		if (FindSelAcc(doc, AC_DBLSHARP)) {
			GetIndCString(strBuf, PITCHERRS_STRS, 5);    /* "Nightingale can't transpose double-sharps up an augmented unison." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
		}

	GetSelMIDIRange(doc, &lowMIDINum, &hiMIDINum);
	if (lowMIDINum+semiChangeOct<1
	||   hiMIDINum+semiChangeOct>127) {						/* 127=MAX_NOTENUM */
		GetIndCString(strBuf, PITCHERRS_STRS, 2);			/* "Transposing would lead to MIDI note number(s) below 1 or above 127." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}

	WaitCursor();

	for (v = 0; v<=MAXVOICES; v++)
		voiceChanged[v] = FALSE;
	changedNotes = changedChords = FALSE;
	ksSpellingBad = spellingBad = chordSpellingBad = 0;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case KEYSIGtype:
				if (!KeySigINMEAS(pL))								/* Initial keysigs already updated */
					if (SSearch(pL, MEASUREtype, GO_LEFT))		/* ...except the very first */
						break;
				
				GetAllContexts(doc, context, pL);
				aKeySigL = FirstSubLINK(pL);
				for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
					if (trStaff[KeySigSTAFF(aKeySigL)]) {
						if (!TranspKeySig(doc, pL, aKeySigL, context[KeySigSTAFF(aKeySigL)],
												steps, semiChange))
							ksSpellingBad++;
						changedKeys = TRUE;
					}
				break;
			case GRAPHICtype:
				if (!chordSyms) break;
				
				if (trStaff[GraphicSTAFF(pL)]) {
					pGraphic = GetPGRAPHIC(pL);
					if (pGraphic->graphicType==GRChordSym) {
						if (!TranspChordSym(doc, pL, steps, semiChange))
							chordSpellingBad++;
						changedChords = TRUE;
					}
				}
				break;
			case SYNCtype:
				GetAllContexts(doc, context, pL);
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (trStaff[NoteSTAFF(aNoteL)] && !NoteREST(aNoteL)) {
						if ((NoteAPPEAR(aNoteL)==SLASH_SHAPE && slashes)
						||  (NoteAPPEAR(aNoteL)!=SLASH_SHAPE && notes)) {
							if (!TranspNote(doc, pL, aNoteL, context[NoteSTAFF(aNoteL)],
													octaves, steps, semiChange))
								spellingBad++;
							changedNotes = TRUE;
							voiceChanged[NoteVOICE(aNoteL)] = TRUE;
						}
					}
				for (v = 0; v<=MAXVOICES; v++)
					if (voiceChanged[v]) FixRespelledVoice(doc, pL, v);
				break;
			case GRSYNCtype:
				if (!notes) break;

				GetAllContexts(doc, context, pL);
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
					if (trStaff[GRNoteSTAFF(aGRNoteL)]) {
						if (!TranspGRNote(doc, pL, aGRNoteL, context[GRNoteSTAFF(aGRNoteL)],
												0, steps, semiChange))
							spellingBad++;
						changedNotes = TRUE;
						voiceChanged[GRNoteVOICE(aGRNoteL)] = TRUE;
					}
				for (v = 0; v<=MAXVOICES; v++)
					if (voiceChanged[v]) FixRespelledVoice(doc, pL, v);
				break;
			default:
				;
			}
		
	if (ksSpellingBad) {
		GetIndCString(fmtStr, PITCHERRS_STRS, 8);    /* "Transposing %d key signatures(s) led to impossible keys; they were left unchanged." */
		sprintf(strBuf, fmtStr, ksSpellingBad); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	if (spellingBad) {
		GetIndCString(fmtStr, PITCHERRS_STRS, 6);    /* "Transposing %d note(s) led to at least triple sharps or flats; they were simplified." */
		sprintf(strBuf, fmtStr, spellingBad); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	if (chordSpellingBad) {
		GetIndCString(fmtStr, PITCHERRS_STRS, 7);    /* "Transposing %d chord symbol(s) led to at least triple sharps or flats; they were simplified." */
		sprintf(strBuf, fmtStr, chordSpellingBad); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	
	if (changedNotes) {
		StfDelRedundantAccs(doc, DELSOFT_REDUNDANTACCS_DI, trStaff);
		for (s = 1; s<=doc->nstaves; s++)
			if (trStaff[s])
				FixBeamsInRange(doc, doc->headL, doc->tailL, s, FALSE);
	}

	if (changedKeys) {
		if (doc->autoRespace) {
			LINK keySigL, endL;
			
			/* Key signatures, both changes and those in the reserved areas of systems,
				might need much more or less space than they have now. */
				
			keySigL = SSearch(doc->headL, KEYSIGtype, GO_RIGHT);
			for ( ; keySigL; keySigL = SSearch(RightLINK(keySigL), KEYSIGtype, GO_RIGHT))
				if (!KeySigINMEAS(keySigL)) {
					endL = SSearch(keySigL, MEASUREtype, GO_RIGHT);		/* Should always exist */
					FixInitialKSxds(doc, keySigL, endL, 1);
				}

			/* The following respaces the entire score at a fixed percentage--pretty
				crude. Much better would be to go thru the score one measure at a time,
				respacing any measure with keysigs on affected staves at its current
				percentage, but that would tend to produce messy reformatting. Someday. */

			spaceProp = RESFACTOR*doc->spacePercent;
			ProgressMsg(RESPACE_PMSTR, "");
			RespaceBars(doc, doc->headL, doc->tailL, spaceProp, FALSE, TRUE);

			ProgressMsg(0, "");
		}
		
		InvalWindow(doc);
	}

	if (changedKeys || changedNotes || changedChords) {
		InvalRange(doc->headL, doc->tailL);							/* Update objRects */
		doc->changed = TRUE;
		return TRUE;
	}
	else
		return FALSE;
}


/* ------------------------------------------------------------------ CheckRange -- */
/* Check MIDI noteNums for all notes in the score against the ranges of the
instruments assigned to their parts. Any out-of-range notes are left selected;
everything else is deselected. Assumes doc is in the active window.
??Questions:
1. firstStaff and lastStaff: is index (cf comment in NTypes.h) the staffn?
2. are the staves numbered consecutively or at least monotonically? */

void CheckRange(Document *doc)
{
	LINK pL, partL, aNoteL; PPARTINFO pPart; PANOTE aNote;
	short firstStaff, lastStaff, hiKeyNum, loKeyNum;
	Boolean noteFound=FALSE;
	
	DeselAll(doc);
	partL = FirstSubLINK(doc->headL);
	for ( ; partL; partL = NextPARTINFOL(partL)) {
		pPart = GetPPARTINFO(partL);
		firstStaff = pPart->firstStaff;
		lastStaff = pPart->lastStaff;
		hiKeyNum = pPart->hiKeyNum;
		loKeyNum = pPart->loKeyNum;
		for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL)) {
			if (SyncTYPE(pL))
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)>=firstStaff && NoteSTAFF(aNoteL)<=lastStaff
					&&	!NoteREST(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->noteNum>hiKeyNum || aNote->noteNum<loKeyNum) {
							LinkSEL(pL) = TRUE;
							NoteSEL(aNoteL) = TRUE;
							noteFound = TRUE;
						}
					}
		}
	}

	if (noteFound) {

		doc->selStartL = doc->selEndL = NILINK;
		for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL))
			if (LinkSEL(pL) && !doc->selStartL)
				{ doc->selStartL = pL; break; }
		for (pL=doc->tailL; pL!=doc->headL; pL=LeftLINK(pL))
			if (LinkSEL(LeftLINK(pL)) && !doc->selEndL)
				{ doc->selEndL = pL; break; }

	}

	InvalWindow(doc);
}
