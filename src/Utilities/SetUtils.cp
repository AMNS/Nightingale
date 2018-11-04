/******************************************************************************************
*	FILE:	SetUtils.c
*	PROJ:	Nightingale
*	DESC:	Set data-structure-manipulating routines, by DAB and John Gibson
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
/*
	FixSyncVoice				SetSelVoice					SetSelStaff
	ExtremeNote					SetSelStemlen				SetSelNRAppear
	SetSelNRSmall				SetVelFromContext			SetSelVelocity
	SetSelGraphicY				SetSelTempoX				SetSelTempoY				
	SetSelTempoVisible
	SetSelGraphicStyle			SetSelMeasVisible			SetSelMeasType
	SetSelDynamVisible			SetSelTupBracket
	SetSelTupNums				SetAllSlursShape			SetSelSlurShape
	SetSelSlurAppear			SetSelLineThickness
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static Boolean SSVChkNote(Document *, LINK, LINK, short);
static Boolean SSVCheck(Document *, LINK, short);
static void FixSyncVoice(Document *, LINK, short);
static LINK SSNoteOnStaff(LINK, short);
static Boolean SSSObjCheck(Document *, LINK, short);
static Boolean SSSCheck(Document *, LINK, LINK, short);
static LINK ExtremeNote(LINK, short, Boolean);
static Boolean SSGYCheck(Document *, LINK, DDIST);
static Boolean SSTYCheck(Document *, LINK, DDIST);


static Boolean SSVChkNote(Document *doc, LINK pL, LINK aNoteL, short uVoice)
{
	PANOTE aNote;
	LINK partL, vNoteL;
	short iVoice;

	aNote = GetPANOTE(aNoteL);
	if (aNote->beamed
	||  aNote->tiedL || aNote->tiedR
	||  aNote->slurredL || aNote->slurredR
	||  aNote->inTuplet) {
		GetIndCString(strBuf, SETERRS_STRS, 1);    /* "Set can't change the voice of..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}

	partL = Staff2PartL(doc, doc->headL, NoteSTAFF(aNoteL));
	iVoice = User2IntVoice(doc, uVoice, partL);
	vNoteL = NoteInVoice(pL, iVoice, False);
	if (vNoteL) {
		GetIndCString(strBuf, SETERRS_STRS, 2);    /* "Set can't move notes into a voice that's already used in the same Sync." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}

	return True;
}

Boolean SSVCheck(Document *doc, LINK syncL, short uVoice)
{
	LINK aNoteL;
	short np, durCode[MAXSTAVES+1];
	Boolean haveRest[MAXSTAVES+1];
	
	/* First context-independent checks of each note (really each voice, since some
		notes may not be selected yet). */
		
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteSEL(aNoteL))
			if (!SSVChkNote(doc, syncL, aNoteL, uVoice)) return False;
	}

	/*
	 * Now check consistency of durations, and that we're not chording rests. NB:
	 * as of v2.1, I think these are illegal anyway because we don't allow moving
	 * notes/rests into chords, but that restriction should be removed some day,
	 * so leave these checks.
	 */
	for (np = 0; np<=MAXSTAVES; np++) {
		durCode[np] = 999;
		haveRest[np] = False;
	}
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteSEL(aNoteL)) {
			np = Staff2Part(doc, NoteSTAFF(aNoteL));
			if (durCode[np]==999)
				durCode[np] = NoteType(aNoteL);
			else
				if (haveRest[np] || NoteREST(aNoteL)) {
					GetIndCString(strBuf, SETERRS_STRS, 3);    /* "You can't have a chord containing a rest." */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					return False;
				}
			
			if (NoteType(aNoteL)!=durCode[np]) {
				GetIndCString(strBuf, SETERRS_STRS, 4);    /* "You can't move notes of different durations into a chord." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return False;
			}
			
			if (NoteREST(aNoteL)) haveRest[np] = True;
		}
	}
	
	return True;
}

void FixSyncVoice(Document *doc,
						LINK pL,				/* Sync */
						short voice)
{
	LINK	aNoteL, vNoteL=NILINK;
	CONTEXT	context;
	Boolean	stemDown;
	QDIST	qStemLen;
	short	notesInChord, voiceRole;
	
	if (SyncTYPE(pL)) {
		notesInChord = 0;
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==voice) { vNoteL = aNoteL; notesInChord++; }
			
		if (notesInChord>0) {
			voiceRole = doc->voiceTab[voice].voiceRole;

			if (notesInChord>1) {
				FixSyncForChord(doc, pL, voice, NoteBEAMED(vNoteL), 0, 0, NULL);
				stemDown = (voiceRole==VCROLE_LOWER);				/* Not quite: VCROLE_CROSS is more complex, but... */
			}
			else {
				stemDown = GetStemInfo(doc, pL, vNoteL, &qStemLen);
				GetContext(doc, pL, NoteSTAFF(vNoteL), &context);
				NoteYSTEM(vNoteL) = CalcYStem(doc, NoteYD(vNoteL),
												NFLAGS(NoteType(vNoteL)),
												stemDown,
												context.staffHeight, context.staffLines,
												qStemLen, False);
			}

			FixAugDotPos(doc, pL, voice, True);
		}
	}
}


/* ----------------------------------------------------------------------- SetSelVoice -- */
/* Set the voice number of every selected note or rest to the internal voice number
for its part that's equivalent to the given user voice number. */

Boolean SetSelVoice(Document *doc, short uVoice)
{
	LINK partL, pL, aNoteL;
	short v, tempV;
	Boolean didAnything, syncChanged, voiceChanged[MAXVOICES+1];
	
	didAnything = False;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL) && SyncTYPE(pL))
			if (!SSVCheck(doc, pL, uVoice)) return False;
	}

	/* See if there are any chords that are partly selected. If so, ask user for
		permission to extend the selection to include all notes in them: this is to
		avoid various nasty situations. */

	if (!HomogenizeSel(doc, SETVOICE_CHORD_ALRT)) return False;

	/* All chords are fully selected. Here we go. */

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			syncChanged = False;
			for (v = 0; v<=MAXVOICES; v++)
				voiceChanged[v] = False;
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteSEL(aNoteL)) {
					syncChanged = True;
					partL = Staff2PartL(doc, doc->headL, NoteSTAFF(aNoteL));
					/* The note's old and new voices are both affected. */
					
					voiceChanged[NoteVOICE(aNoteL)] = True;
					tempV = User2IntVoice(doc, uVoice, partL);
					NoteVOICE(aNoteL) = tempV;
					voiceChanged[NoteVOICE(aNoteL)] = True;
					if (!LOOKING_AT(doc, NoteVOICE(aNoteL)))
						NoteSEL(aNoteL) = False;
				}
			}
			if (syncChanged) {
				for (v = 0; v<=MAXVOICES; v++)
					if (voiceChanged[v]) FixSyncVoice(doc, pL, v);
				didAnything = True;
			}
		}
	}

	OptimizeSelection(doc);
	return didAnything;
}


/* ------------------------------------------------------------------------ SSSCheck's -- */

/* If the given Sync has an UNSELECTED subobj (note or rest) on the given staff,
return the first one found. */

static LINK SSNoteOnStaff(LINK syncL, short s)
{
	LINK aNoteL;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteSTAFF(aNoteL)==s && !NoteSEL(aNoteL)) return aNoteL;
	}
	
	return NILINK;
}			

static Boolean SSSObjCheck(Document *doc, LINK syncL, short absStaff)
{
	LINK dynamL, graphicL, tempoL, endingL;
	Boolean movingAll[MAXSTAVES+1], syncHasGraphic;
	short s;
	
	/* Look for staff(s) other than <absStaff> containing notes/rests but all are
	selected and therefore about to be moved to another staff. */
	
	for (s = 1; s<=doc->nstaves; s++) {
		movingAll[s] = False;
		if (s!=absStaff && NoteOnStaff(syncL, s))
			if (!SSNoteOnStaff(syncL, s)) movingAll[s] = True;
	}
	
	dynamL = LSISearch(LeftLINK(syncL), DYNAMtype, ANYONE, GO_LEFT, False);
	if (dynamL)
		if (movingAll[DynamicSTAFF(dynamL)]
		&&  (DynamFIRSTSYNC(dynamL)==syncL
		||   (IsHairpin(dynamL) && DynamLASTSYNC(dynamL)==syncL))) {
			GetIndCString(strBuf, SETERRS_STRS, 5);    /* "Set can't change staff of notes with dynamics attached to them." */
			CParamText(strBuf, "", "", "");
			StopInform(SETSTAFF_ALRT);
			return False;
		}
		
	graphicL = LSISearch(LeftLINK(syncL), GRAPHICtype, ANYONE, GO_LEFT, False);
	if (graphicL)
		if (movingAll[GraphicSTAFF(graphicL)]) {
			syncHasGraphic = (GraphicFIRSTOBJ(graphicL)==syncL
					|| (GraphicSubType(graphicL)==GRDraw && GraphicLASTOBJ(graphicL)==syncL));
			if (syncHasGraphic) {
				GetIndCString(strBuf, SETERRS_STRS, 6);    /* "Set can't change staff of notes with Graphics (text, rehearsal mark, chord symbol, line, or arpeggio sign) attached to them." */
				CParamText(strBuf, "", "", "");
				StopInform(SETSTAFF_ALRT);
				return False;
			}
		}

	tempoL = LSISearch(LeftLINK(syncL), TEMPOtype, ANYONE, GO_LEFT, False);
	if (tempoL)
		if (movingAll[TempoSTAFF(tempoL)] && TempoFIRSTOBJ(tempoL)==syncL) {
			GetIndCString(strBuf, SETERRS_STRS, 8);    /* "Set can't change staff of notes with tempo marks attached to them." */
			CParamText(strBuf, "", "", "");
			StopInform(SETSTAFF_ALRT);
			return False;
		}

	endingL = LSISearch(LeftLINK(syncL), ENDINGtype, ANYONE, GO_LEFT, False);
	if (endingL)
		if (movingAll[EndingSTAFF(endingL)]
		&&  (EndingFIRSTOBJ(endingL)==syncL || EndingLASTOBJ(endingL)==syncL)) {
			GetIndCString(strBuf, SETERRS_STRS, 9);    /* "Set can't change staff of notes with Endings attached to them." */
			CParamText(strBuf, "", "", "");
			StopInform(SETSTAFF_ALRT);
			return False;
		}

	return True;
}

static Boolean SSSCheck(Document *doc, LINK /*syncL*/, LINK aNoteL, short absStaff)
{
	PANOTE aNote;

	aNote = GetPANOTE(aNoteL);
	if (aNote->beamed ||  aNote->tiedL || aNote->tiedR ||
			aNote->slurredL || aNote->slurredR) {
		GetIndCString(strBuf, SETERRS_STRS, 10);    /* "Set can't change staff of beamed, slurred, or tied notes." */
		CParamText(strBuf, "", "", "");
		StopInform(SETSTAFF_ALRT);
		return False;
	}
	if (Staff2Part(doc, absStaff)!=Staff2Part(doc, NoteSTAFF(aNoteL))) {
		GetIndCString(strBuf, SETERRS_STRS, 11);    /* "Set can't move notes/rests to a staff in a different part. To do this, Cut, then Paste Merge." */
		CParamText(strBuf, "", "", "");
		StopInform(SETSTAFF_ALRT);
		return False;
	}
	
	return True;
}


/* ----------------------------------------------------------------------- SetSelStaff -- */
/* Set the staff number of every selected note or rest. NB: it looks like the
restriction to unbeamed notes could be eliminated simply by calling Unbeam. */

Boolean SetSelStaff(Document *doc,
						short newStaff)		/* part-relative staff no. */
{
	LINK		pL, aNoteL, partL;
	PPARTINFO	pPart;
	Boolean		didAnything;
	short		absStaff, halfLn, effAcc;
	
	/* Convert from part-relative to absolute staff number. */
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL)) {
					partL = FindPartInfo(doc, Staff2Part(doc, NoteSTAFF(aNoteL)));
					pPart = GetPPARTINFO(partL);
					absStaff = pPart->firstStaff+newStaff-1;
					goto StaffFound;					
				}
		}
		
	/* No notes or rests are selected, so there's nothing to do. */
	return False;

StaffFound:
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			if (!SSSObjCheck(doc,pL,absStaff)) return False;
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteSEL(aNoteL))
					if (!SSSCheck(doc,pL,aNoteL,absStaff)) return False;
			}
		}
	}

	/* See if there are any chords that are partly selected. If so, ask user for
		permission to extend the selection to include all notes in them: this is to
		keep any individual voice in a Sync from ending up with notes on more than
		one staff. */

	if (!HomogenizeSel(doc, SETSTAFF_CHORD_ALRT)) return False;

	/* All chords are fully selected. Here we go. */
	
	didAnything = False;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL) && NoteSTAFF(aNoteL)!=absStaff) {
					/* Handle change of clef, and accidental context on old staff. */
					if (!NoteREST(aNoteL)) {
						effAcc = EffectiveAcc(doc, pL, aNoteL);
						FixNoteForClef(doc, pL, aNoteL, absStaff);
						halfLn = qd2halfLn(NoteYQPIT(aNoteL));
						DelNoteFixAccs(doc, pL, NoteSTAFF(aNoteL), halfLn, NoteACC(aNoteL));
					}

					/* Move the note to the new staff */
					NoteSTAFF(aNoteL) = absStaff;

					if (!NoteREST(aNoteL)) {												
						/* Handle accidental context on new staff. */
						if (!NoteACC(aNoteL)) {
							NoteACC(aNoteL) = effAcc;
							NoteACCSOFT(aNoteL) = True;
						}
						InsNoteFixAccs(doc, pL, absStaff, halfLn, NoteACC(aNoteL));
					}

					didAnything = True;
				}
		}

	if (didAnything) {
		DelRedundantAccs(doc, ANYONE, DELSOFT_REDUNDANTACCS_DI);
		InvalRange(doc->selStartL, doc->selEndL);						/* Update objRects */
	}
	return didAnything;
}


/* ----------------------------------------------------------------------- ExtremeNote -- */
/* Return the note in the given Sync and voice that's closest to the stem, i.e.,
furthest from the MainNote. */

LINK ExtremeNote(LINK syncL, short voice, Boolean stemDown)
{
	LINK aNoteL, extNoteL=NILINK;
	DDIST extYD;
	
	extYD = (stemDown? -32767 : 32767);
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice)
			if ((stemDown && NoteYD(aNoteL)>extYD)
			||  (!stemDown && NoteYD(aNoteL)<extYD)) {
				extYD = NoteYD(aNoteL);
				extNoteL = aNoteL;
			}
	
	return extNoteL;
}


/* --------------------------------------------------------------------- SetSelStemlen -- */
/* Set the stem length of every selected note/rest/chord to a positive value,
leaving stem direction as it is. If any note in a chord is selected, the chord
is considered selected. */

Boolean SetSelStemlen(Document *doc, unsigned STDIST stemlen)
{
	LINK	pL, aNoteL, bNoteL;
	DDIST	dStemlen, extend;
	CONTEXT	context;
	Boolean	didAnything;
	short	voice, upDown;

	didAnything = False;
		
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			for (voice = 1; voice<=MAXVOICES; voice++)
				if (VOICE_MAYBE_USED(doc, voice))
					if (NoteInVoice(pL, voice, True)) {
						aNoteL = FindMainNote(pL, voice);
						if (aNoteL && NoteType(aNoteL)==UNKNOWN_L_DUR) {
							GetIndCString(strBuf, SETERRS_STRS, 12);    /* "Nightingale doesn't draw stems on unknown-duration notes. Use Set Duration." */
							CParamText(strBuf, "", "", "");
							StopInform(GENERIC_ALRT);
							return False;
						}						
					}
		}
	}
	
	for (dStemlen = -32767, pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (LinkSEL(pL) && SyncTYPE(pL)) {
			for (voice = 1; voice<=MAXVOICES; voice++)
				if (VOICE_MAYBE_USED(doc, voice))
					if (NoteInVoice(pL, voice, True)) {
						aNoteL = FindMainNote(pL, voice);
						if (!aNoteL) {
							MayErrMsg("SetSelStemlen: illegal note/chord: no Main note.");
							continue;
						}
						/*
						 * Convert the staff-height-relative stem length to DDIST. Length zero
						 * will cause big problems with chords, so instead use the shortest
						 * length that Get Info won't round to zero. We assume staff height and
						 * no. of lines are constant--true as of v.5.7.
						 */
						if (dStemlen<=-32767) {
							if (stemlen==0)
								dStemlen = pt2d(1);
							else {
								GetContext(doc, pL, NoteSTAFF(aNoteL), &context);
								dStemlen = std2d(stemlen, context.staffHeight,
														context.staffLines);
							}
						}
						upDown = (NoteYD(aNoteL)>NoteYSTEM(aNoteL)? 1 : -1);
						/*
						 * If this is a chord, the user undoubtedly thinks of the stem length
						 * as what projects beyond the note at the other end of the chord
						 * from the MainNote, so we have to increase the actual length
						 * accordingly.
						 */
						if (NoteINCHORD(aNoteL) && stemlen>0) {
							bNoteL = ExtremeNote(pL, voice, (upDown<0));
							extend = ABS(NoteYD(bNoteL)-NoteYD(aNoteL));
						}
						else
							extend = 0;
						NoteYSTEM(aNoteL) = NoteYD(aNoteL)-upDown*(dStemlen+extend);
						didAnything = True;
					}
		}
	}

	return didAnything;
}


/* -------------------------------------------------------------------- SetSelNRAppear -- */
/* Set the appearance (shape and visibility) of every selected note or rest. */

Boolean SetSelNRAppear(Document *doc, short appearance)
{
	PANOTE	aNote;
	LINK	pL, aNoteL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					aNote->headShape = appearance;
					didAnything = True;
				}

	return didAnything;
}


/* --------------------------------------------------------------------- SetSelNRSmall -- */
/* Set the <small> flag of every selected note or rest. */

Boolean SetSelNRSmall(Document *doc, short small)
{
	PANOTE	aNote;
	LINK	pL, aNoteL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					aNote->small = small;
					didAnything = True;
				}

	return didAnything;
}


/* -------------------------------------------------------------------- SetSelNRParens -- */
/* Set the <courtesyAcc> flag of every selected note or rest. */

Boolean SetSelNRParens(Document *doc, short inParens)
{
	PANOTE	aNote;
	LINK	pL, aNoteL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SyncTYPE(pL))
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteSEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					aNote->courtesyAcc = inParens;
					didAnything = True;
				}

	return didAnything;
}


/* ----------------------------------------------------------------- SetVelFromContext -- */
/* Set the On velocity of a note according to its context, i.e, the last dynamic
marking on its staff and the current dynamics-to-velocity table. Works either on
all notes in the score or only on selected notes. */

Boolean SetVelFromContext(Document *doc, Boolean selectedOnly)
{
	short s;
	CONTEXT context;
	unsigned char curVelocity;
	LINK pL, aNoteL, aDynamicL;
	PANOTE aNote;
	PADYNAMIC aDynamic;
	Boolean	didAnything=False;

	for (s = 1; s<=doc->nstaves; s++) {
		GetContext(doc, doc->selStartL, s, &context);
		curVelocity = dynam2velo[context.dynamicType];
		for (pL = doc->headL; pL!=doc->tailL; pL=RightLINK(pL))
			switch (ObjLType(pL)) {
				case SYNCtype:
					if (LinkSEL(pL) || !selectedOnly)
						for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
							if ((NoteSEL(aNoteL) || !selectedOnly)
							 && !NoteREST(aNoteL) && NoteSTAFF(aNoteL)==s) {
								aNote = GetPANOTE(aNoteL);
								aNote->onVelocity = curVelocity;
								didAnything = True;
							}
					break;
				case DYNAMtype:
					aDynamicL = FirstSubLINK(pL);
					for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL)) {
						aDynamic = GetPADYNAMIC(aDynamicL);
						if (DynamicSTAFF(aDynamicL)==s
						&&  DynamType(pL)<FIRSTHAIRPIN_DYNAM)
							curVelocity = dynam2velo[DynamType(pL)];
					}
					break;
			}
	}

	return didAnything;
}


/* -------------------------------------------------------------------- SetSelVelocity -- */
/* Set the On velocity of every selected note to a specific unsigned value. If the
value specified is negative, does nothing. */

Boolean SetSelVelocity(Document *doc, short velocity)
{
	PANOTE	aNote;
	LINK	pL, aNoteL;
	Boolean	didAnything=False;

	if (velocity>=0) {
		for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
			if (LinkSEL(pL) && SyncTYPE(pL))
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSEL(aNoteL) && !NoteREST(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						aNote->onVelocity = (unsigned)velocity;
						didAnything = True;
					}
	}

	return didAnything;
}


/* ------------------------------------------------------------------------- SSGXCheck -- */

static Boolean SSGXCheck(Document *doc, LINK graphicL, DDIST dxd)
{
	PGRAPHIC p;
	DDIST pageRelxd, xd;
	CONTEXT	context;
	char fmtStr[256], wordStr[32];

	p = GetPGRAPHIC(graphicL);
	if (PageTYPE(p->firstObj))
		pageRelxd = dxd;
	else {
		GetContext(doc, p->firstObj, GraphicSTAFF(graphicL), &context);
		p = GetPGRAPHIC(graphicL);
		xd = PageRelxd(p->firstObj, &context);
		pageRelxd = xd + dxd;
	}
	if (pageRelxd<pt2d(doc->origPaperRect.left) || pageRelxd>pt2d(doc->origPaperRect.right)) {
		Boolean left = (pageRelxd < pt2d(doc->origPaperRect.left));
		GetIndCString(fmtStr, SETERRS_STRS, 20);    /* "This position would move something off the %s of the page." */
		GetIndCString(wordStr, SETERRS_STRS, (left? 24 : 25));    /* "left" : "right" */
		sprintf(strBuf, fmtStr, wordStr);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
	
	return True;
}


/* -------------------------------------------------------------------- SetSelGraphicX -- */
/* Set the xd of every selected Graphic of the given subtype to a signed value.
Assumes all staves containing selected Graphics of the given subtype have the same
height and number of staff lines. */

Boolean SetSelGraphicX(
				Document *doc,
				STDIST stx,
				short subtype					/* GRString, GRLyric, or GRChordSym */
				)
{
	PGRAPHIC	p;
	register LINK pL;
	LINK		contL;
	DDIST		dxd=-32767;
	CONTEXT		context;
	Boolean		didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && GraphicTYPE(pL))
			if ((GraphicSubType(pL)==GRString && subtype==GRString)
			||  (GraphicSubType(pL)==GRLyric && subtype==GRLyric)
			||  (GraphicSubType(pL)==GRChordSym && subtype==GRChordSym)) {
			if (dxd<=-32767) {
				contL = (LinkBefFirstMeas(pL)? SSearch(pL, MEASUREtype, GO_RIGHT) : pL);
				GetContext(doc, contL, 1, &context);
				dxd = std2d(stx, context.staffHeight, context.staffLines);
			}
			if (!SSGXCheck(doc, pL, dxd)) return didAnything;
		}
	}

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && GraphicTYPE(pL))
			if ((GraphicSubType(pL)==GRString && subtype==GRString)
			||  (GraphicSubType(pL)==GRLyric && subtype==GRLyric)
			||  (GraphicSubType(pL)==GRChordSym && subtype==GRChordSym)) {
			p = GetPGRAPHIC(pL);
			p->xd = dxd;
			didAnything = True;
		}
	}
	return didAnything;
}


/* ------------------------------------------------------------------------- SSGYCheck -- */

static Boolean SSGYCheck(Document *doc, LINK graphicL, DDIST dyd)
{
	PGRAPHIC p;
	DDIST pageRelyd, yd;
	Boolean top;
	CONTEXT	context;
	char fmtStr[256], wordStr[32];

	p = GetPGRAPHIC(graphicL);
	if (PageTYPE(p->firstObj))
		pageRelyd = dyd;
	else {
		GetContext(doc, p->firstObj, GraphicSTAFF(graphicL), &context);
		p = GetPGRAPHIC(graphicL);
		yd = PageRelyd(p->firstObj, &context);
		pageRelyd = yd+dyd;
	}
	if (pageRelyd<pt2d(doc->origPaperRect.top) || pageRelyd>pt2d(doc->origPaperRect.bottom)) {
		top = pageRelyd<pt2d(doc->origPaperRect.top);
		GetIndCString(fmtStr, SETERRS_STRS, 20);    /* "This position would move something off the %s of the page." */
		GetIndCString(wordStr, SETERRS_STRS, (top? 22 : 23));    /* "top" : "bottom" */
		sprintf(strBuf, fmtStr, wordStr);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
	
	return True;
}

/* -------------------------------------------------------------------- SetSelGraphicY -- */
/* Set the yd of every selected Graphic of the given subtype to a signed value.
Assumes all staves containing selected Graphics of the given subtype have the same
height and number of staff lines. */

Boolean SetSelGraphicY(
				Document *doc,
				STDIST sty,
				short subtype,					/* GRString, GRLyric, or GRChordSym */
				Boolean	above 					/* True=above, False=below */
				)
{
	PGRAPHIC	p;
	register LINK pL;
	LINK		contL;
	DDIST		dyd=-32767;
	CONTEXT		context;
	Boolean		didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && GraphicTYPE(pL))
			if ((GraphicSubType(pL)==GRString && subtype==GRString)
			||  (GraphicSubType(pL)==GRLyric && subtype==GRLyric)
			||  (GraphicSubType(pL)==GRChordSym && subtype==GRChordSym)) {
			if (dyd<=-32767) {
				contL = (LinkBefFirstMeas(pL)? SSearch(pL, MEASUREtype, GO_RIGHT) : pL);
				GetContext(doc, contL, 1, &context);
				dyd = std2d(sty, context.staffHeight, context.staffLines);
				if (above) dyd *= -1;
				else		  dyd += context.staffHeight;
			}
			if (!SSGYCheck(doc, pL, dyd)) return didAnything;
		}
	}

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && GraphicTYPE(pL))
			if ((GraphicSubType(pL)==GRString && subtype==GRString)
			||  (GraphicSubType(pL)==GRLyric && subtype==GRLyric)
			||  (GraphicSubType(pL)==GRChordSym && subtype==GRChordSym)) {
			p = GetPGRAPHIC(pL);
			p->yd = dyd;
			didAnything = True;
		}
	}
	return didAnything;
}


/* ------------------------------------------------------------------------- SSTXCheck -- */

static Boolean SSTXCheck(Document *doc, LINK tempoL, DDIST dxd)
{
	PTEMPO p;
	DDIST pageRelxd, xd;
	CONTEXT	context;
	char fmtStr[256], wordStr[32];
	
	p = GetPTEMPO(tempoL);
	if (PageTYPE(p->firstObjL))
		pageRelxd = dxd;
	else {
		GetContext(doc, p->firstObjL, TempoSTAFF(tempoL), &context);
		p = GetPTEMPO(tempoL);
		xd = PageRelxd(p->firstObjL, &context);
		pageRelxd = xd+dxd;
	}
	if (pageRelxd<pt2d(doc->origPaperRect.left) || pageRelxd>pt2d(doc->origPaperRect.right)) {
		Boolean left = pageRelxd<pt2d(doc->origPaperRect.left);
		GetIndCString(fmtStr, SETERRS_STRS, 20);    /* "This position would move something off the %s of the page." */
		GetIndCString(wordStr, SETERRS_STRS, (left? 24 : 25));    /* "left" : "right" */
		sprintf(strBuf, fmtStr, wordStr);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
	
	return True;
}


/* ---------------------------------------------------------------------- SetSelTempoX -- */
/* Set the xd of every selected tempo mark to a signed value. Assumes all staves
containing selected Graphics of the given subtype have same height and number of
staff lines. */

Boolean SetSelTempoX(
				Document *doc,
				STDIST	stx
				)
{
	PTEMPO	p;
	register LINK pL;
	DDIST	dxd=-32767;
	CONTEXT	context;							/* current context */
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && TempoTYPE(pL)) {
			if (dxd<=-32767) {
				GetContext(doc, pL, 1, &context);
				dxd = std2d(stx, context.staffHeight, context.staffLines);
			}
			if (!SSTXCheck(doc, pL, dxd)) return didAnything;
		}
	}
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && TempoTYPE(pL)) {
			p = GetPTEMPO(pL);
			p->xd = dxd;
			didAnything = True;
		}
	}
	return didAnything;
}


/* ------------------------------------------------------------------------- SSTYCheck -- */

static Boolean SSTYCheck(Document *doc, LINK tempoL, DDIST dyd)
{
	PTEMPO p;
	DDIST pageRelyd, yd;
	Boolean top;
	CONTEXT	context;
	char fmtStr[256], wordStr[32];
	
	p = GetPTEMPO(tempoL);
	if (PageTYPE(p->firstObjL))
		pageRelyd = dyd;
	else {
		GetContext(doc, p->firstObjL, TempoSTAFF(tempoL), &context);
		p = GetPTEMPO(tempoL);
		yd = PageRelyd(p->firstObjL, &context);
		pageRelyd = yd+dyd;
	}
	if (pageRelyd<pt2d(doc->origPaperRect.top) || pageRelyd>pt2d(doc->origPaperRect.bottom)) {
		top = pageRelyd<pt2d(doc->origPaperRect.top);
		GetIndCString(fmtStr, SETERRS_STRS, 20);    /* "This position would move something off the %s of the page." */
		GetIndCString(wordStr, SETERRS_STRS, (top? 22 : 23));    /* "top" : "bottom" */
		sprintf(strBuf, fmtStr, wordStr);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return False;
	}
	
	return True;
}


/* ---------------------------------------------------------------------- SetSelTempoY -- */
/* Set the yd of every selected tempo mark to a signed value. Assumes all staves
containing selected Graphics of the given subtype have same height and number of
staff lines. */

Boolean SetSelTempoY(
				Document *doc,
				STDIST	sty,
				Boolean	above 				/* True=above, False=below */
				)
{
	PTEMPO	p;
	register LINK pL;
	DDIST	dyd=-32767;
	CONTEXT	context;							/* current context */
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && TempoTYPE(pL)) {
			if (dyd<=-32767) {
				GetContext(doc, pL, 1, &context);
				dyd = std2d(sty, context.staffHeight, context.staffLines);
				if (above) dyd *= -1;
				else		  dyd += context.staffHeight;
			}
			if (!SSTYCheck(doc, pL, dyd)) return didAnything;
		}
	}
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && TempoTYPE(pL)) {
			p = GetPTEMPO(pL);
			p->yd = dyd;
			didAnything = True;
		}
	}
	return didAnything;
}


/* ---------------------------------------------------------------- SetSelTempoVisible -- */
/* Set visibility of every selected Tempo's M.M. */

Boolean SetSelTempoVisible(Document *doc, Boolean visible)
{
	LINK		pL;
	PTEMPO		pTempo;
	Boolean		didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && TempoTYPE(pL)) {
			pTempo = GetPTEMPO(pL);
			pTempo->hideMM = !visible;
			didAnything = True;
		}
	}
	return didAnything;
}


/* ---------------------------------------------------------------- SetSelGraphicStyle -- */
/* Set the style of every selected Graphic of the given subtype to a given value. */

Boolean SetSelGraphicStyle(
				Document *doc,
				short styleChoice,
				short subtype 					/* GRString or GRLyric */
				)
{
	PGRAPHIC	pGraphic;
	LINK		pL;
	Boolean		didAnything=False, toOrFromLyric;
	TEXTSTYLE	style;
	short		fontInd, info;

	if (styleChoice==TSRegular1STYLE) BlockMove(doc->fontName1,&style,sizeof(TEXTSTYLE));
	else if (styleChoice==TSRegular2STYLE) BlockMove(doc->fontName2,&style,sizeof(TEXTSTYLE));
	else if (styleChoice==TSRegular3STYLE) BlockMove(doc->fontName3,&style,sizeof(TEXTSTYLE));
	else if (styleChoice==TSRegular4STYLE) BlockMove(doc->fontName4,&style,sizeof(TEXTSTYLE));
	else if (styleChoice==TSRegular5STYLE) BlockMove(doc->fontName5,&style,sizeof(TEXTSTYLE));
	else if (styleChoice==TSRegular6STYLE) BlockMove(doc->fontName6,&style,sizeof(TEXTSTYLE));
	else if (styleChoice==TSRegular7STYLE) BlockMove(doc->fontName7,&style,sizeof(TEXTSTYLE));
	else if (styleChoice==TSRegular8STYLE) BlockMove(doc->fontName8,&style,sizeof(TEXTSTYLE));
	else if (styleChoice==TSRegular9STYLE) BlockMove(doc->fontName9,&style,sizeof(TEXTSTYLE));
	else if (styleChoice!=TSThisItemOnlySTYLE) {
		MayErrMsg("SetSelGraphicStyle: illegal style %ld", (long)styleChoice);
		return False;
	}

	fontInd = FontName2Index(doc, style.fontName);
	if (fontInd<0) {
		GetIndCString(strBuf, MISCERRS_STRS, 27);    /* "The font won't be changed" */
		CParamText(strBuf, "", "", "");
		StopInform(MANYFONTS_ALRT);
		return False;
	}

	/* In all cases, set the Graphic's <info> field to reflect the new style. If the
	 *	new style is "This Item Only", leave all of its font information alone, else
	 *	change it to reflect the current settings for that style.
	 */
	toOrFromLyric = False;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && GraphicTYPE(pL)) {
			if ((GraphicSubType(pL)==GRString && subtype==GRString)
			||  (GraphicSubType(pL)==GRLyric && subtype==GRLyric)) {
				pGraphic = GetPGRAPHIC(pL);
				if (pGraphic->graphicType==GRString
				||  pGraphic->graphicType==GRLyric) {
					if (styleChoice!=TSThisItemOnlySTYLE) {
						if (pGraphic->graphicType!=(style.lyric? GRLyric : GRString)) {
							pGraphic->graphicType = (style.lyric? GRLyric : GRString);
							toOrFromLyric = True;
						}
						pGraphic->relFSize = style.relFSize;
						pGraphic->fontSize = style.fontSize;
						pGraphic->fontStyle = style.fontStyle;
						pGraphic = GetPGRAPHIC(pL);
						pGraphic->fontInd = fontInd;
					}
					info = User2HeaderFontNum(doc, styleChoice);
					pGraphic = GetPGRAPHIC(pL);
					pGraphic->info = info;
					didAnything = True;
				}
			}
		}
	}
	
	if (toOrFromLyric) {
		if (style.lyric) {
			GetIndCString(strBuf, SETERRS_STRS, 13);    /* "Some text that was not Lyric before is now. You may want to Respace Bars." */
			CParamText(strBuf, "", "", "");
		}
		else {
			GetIndCString(strBuf, SETERRS_STRS, 14);    /* "Some text that was Lyric before is not now. You may want to Respace Bars." */
			CParamText(strBuf, "", "", "");
		}
		StopInform(GENERIC_ALRT);
	}
	
	return didAnything;
}


/* ----------------------------------------------------------------- SetSelMeasVisible -- */
/* Set visibility of every selected Measure and its subobjects, i.e., of the barlines,
not the contents of the measures. */

Boolean SetSelMeasVisible(Document *doc, Boolean visible)
{
	PAMEASURE	aMeasure;
	LINK		pL, aMeasureL;
	Boolean		didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && MeasureTYPE(pL)) {
			aMeasureL = FirstSubLINK(pL);
			for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL))
				if (MeasureSEL(aMeasureL)) {
					aMeasure = GetPAMEASURE(aMeasureL);
					aMeasure->visible = visible;
					didAnything = True;
				}
		}
	}
	return didAnything;
}


/* -------------------------------------------------------------------- SetSelMeasType -- */
/* Set "type" (the subType field) of every selected Measure subobject. */

Boolean SetSelMeasType(Document *doc, short subtype)
{
	LINK		pL, aMeasureL;
	Boolean		didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && MeasureTYPE(pL)) {
			aMeasureL = FirstSubLINK(pL);
			for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL))
				if (MeasureSEL(aMeasureL)) {
					MeasType(aMeasureL) = subtype;
					didAnything = True;
				}
		}
	}
	return didAnything;
}


/* ---------------------------------------------------------------- SetSelDynamVisible -- */
/* Set visibility of every selected Dynamic. */

Boolean SetSelDynamVisible(Document *doc, Boolean visible)
{
	LINK	pL, aDynamicL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && DynamTYPE(pL)) {
			aDynamicL = FirstSubLINK(pL);
			for ( ; aDynamicL; aDynamicL=NextDYNAMICL(aDynamicL))
				if (DynamicSEL(aDynamicL)) {
					DynamicVIS(aDynamicL) = visible;
					didAnything = True;
				}
		}
	}
	return didAnything;
}


/* ------------------------------------------------------------------ SetSelDynamSmall -- */
/* Set visibility of every selected Dynamic. */

Boolean SetSelDynamSmall(Document *doc, Boolean small)
{
	LINK	pL, aDynamicL;
	Boolean	didAnything=False;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && DynamTYPE(pL)) {
			aDynamicL = FirstSubLINK(pL);
			for ( ; aDynamicL; aDynamicL=NextDYNAMICL(aDynamicL))
				if (DynamicSEL(aDynamicL)) {
					DynamicSMALL(aDynamicL) = small;
					didAnything = True;
				}
		}
	}
	return didAnything;
}


/* ----------------------------------------------------------------- SetSelBeamsetThin -- */
/* Set thickness of selected beamset. */

Boolean SetSelBeamsetThin(Document *doc, Boolean thin)
{
	LINK		pL;
	Boolean		didAnything=False;
	PBEAMSET	pBeamset;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && BeamsetTYPE(pL)) {
			pBeamset = GetPBEAMSET(pL);
			pBeamset->thin = thin;
			didAnything = True;
		}
	}
	return didAnything;
}


/* ----------------------------------------------------------------- SetSelClefVisible -- */
/* Set visibility of every selected Clef. */

Boolean SetSelClefVisible(Document *doc, Boolean visible)
{
	LINK	pL, aClefL;
	Boolean	didAnything=False;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && ClefTYPE(pL)) {
			aClefL = FirstSubLINK(pL);
			for ( ; aClefL; aClefL=NextCLEFL(aClefL))
				if (ClefSEL(aClefL)) {
					ClefVIS(aClefL) = visible;
					didAnything = True;
				}
		}
	}
	return didAnything;
}


/* --------------------------------------------------------------- SetSelKeySigVisible -- */
/* Set visibility of every selected KeySig. */

Boolean SetSelKeySigVisible(Document *doc, Boolean visible)
{
	LINK	pL, aKeySigL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && KeySigTYPE(pL)) {
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL))
				if (KeySigSEL(aKeySigL)) {
					KeySigVIS(aKeySigL) = visible;
					didAnything = True;
				}
		}
	}
	return didAnything;
}


/* -------------------------------------------------------------- SetSelTimeSigVisible -- */
/* Set visibility of every selected TimeSig. */

Boolean SetSelTimeSigVisible(Document *doc, Boolean visible)
{
	LINK	pL, aTimeSigL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && TimeSigTYPE(pL)) {
			aTimeSigL = FirstSubLINK(pL);
			for ( ; aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL))
				if (TimeSigSEL(aTimeSigL)) {
					TimeSigVIS(aTimeSigL) = visible;
					didAnything = True;
				}
		}
	}
	return didAnything;
}


/* ------------------------------------------------------------------ SetSelTupBracket -- */
/* Set visibility of every selected Tuplet bracket. */

Boolean SetSelTupBracket(Document *doc, Boolean visible)
{
	PTUPLET	pTuplet;
	LINK	pL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && TupletTYPE(pL)) {
			pTuplet = GetPTUPLET(pL);
			pTuplet->brackVis = visible;
			didAnything = True;
		}
	}
	return didAnything;
}


/* --------------------------------------------------------------------- SetSelTupNums -- */
/* Set visibility of accessory numerals in every selected Tuplet. */

enum {
	BOTH_VIS=1,
	NUM_VIS
};

Boolean SetSelTupNums(Document *doc, short showWhich)
{
	PTUPLET		pTuplet;
	LINK		pL;
	Boolean		didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && TupletTYPE(pL)) {
			pTuplet = GetPTUPLET(pL);
			pTuplet->numVis = (showWhich==BOTH_VIS || showWhich==NUM_VIS);
			pTuplet->denomVis = showWhich==BOTH_VIS;
			didAnything = True;
		}
	}
	return didAnything;
}


/* ------------------------------------------------------------------ SetAllSlursShape -- */
/* Set all slur/tie subobjects to the default shape. */

void SetAllSlursShape(
			Document *doc,
			LINK slurL,
			Boolean recalc 		/* True=recompute slur/ties' startPt and endPt fields */
			)
{
	LINK	aSlurL, aNoteL, firstSyncL, lastSyncL;
	CONTEXT	context;
	PASLUR	aSlur;
	Point 	startPt[MAXCHORD], endPt[MAXCHORD];
	short	j, staff, voice;
	Boolean curveUps[MAXCHORD];
	
	staff = SlurSTAFF(slurL);
	voice = SlurVOICE(slurL);

	if (recalc) {
	/* This is necessary because SetSlurCtlPoints assumes the slur/ties' startPt and endPt
	 * describe the positions of the notes they're attached to, a poor idea--why can't it
	 * just look at the notes and get their actual positions?
	 */
		GetSlurContext(doc, slurL, startPt, endPt);		/* Get paper-relative positions, in points */
		
		aSlurL = FirstSubLINK(slurL);
		for (j = 0; aSlurL; j++, aSlurL = NextSLURL(aSlurL)) {
			aSlur = GetPASLUR(aSlurL);
			aSlur->startPt = startPt[j];
			aSlur->endPt = endPt[j];
		}
	}

	if (LinkNENTRIES(slurL)==1) {
	/*
	 * One slur/tie: decide whether it should curve up or down based on 1st note stem
	 * direction. The stem dir. test works even for stemless durations, since SetupNote
	 * always sets the stem endpoint.  FIXME: BUT SHOULD CONSIDER ALL NOTES IN THE SLUR!
	 */
		firstSyncL = SlurFIRSTSYNC(slurL);
		lastSyncL = SlurLASTSYNC(slurL);
		if (SyncTYPE(firstSyncL))	aNoteL = FindMainNote(firstSyncL, voice);
		else						aNoteL = FindMainNote(lastSyncL, voice);
		curveUps[0] = (NoteYSTEM(aNoteL) > NoteYD(aNoteL));
	}
	else
	/*
	 * Several ties: curve the upper ones up, the lower ones down.
	 */
		GetTiesCurveDir(doc, voice, slurL, curveUps);

	GetContext(doc, slurL, staff, &context);
	aSlurL = FirstSubLINK(slurL);
	for (j = 0; aSlurL; j++, aSlurL=NextSLURL(aSlurL)) {
		SetSlurCtlPoints(doc, slurL, aSlurL, SlurFIRSTSYNC(slurL), SlurLASTSYNC(slurL),
								staff, voice, context, curveUps[j]);
	}
}


/* ------------------------------------------------------------------- SetSelSlurShape -- */
/* Set every selected slur or tie subobject to the default shape by resetting its
control points. */

Boolean SetSelSlurShape(Document *doc)
{
	PSLUR	pSlur;
	PASLUR	aSlur;
	LINK	pL, aSlurL, aNoteL;
	short	staff, voice, i;
	CONTEXT	context;
	Boolean	curveUp, curveUps[MAXCHORD], didAnything;

	didAnything = False;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		if (LinkSEL(pL) && ObjLType(pL)==SLURtype) {
			GetContext(doc, pL, SlurSTAFF(pL), &context);
			
			pSlur = GetPSLUR(pL);
			aSlurL = FirstSubLINK(pL);
			staff = pSlur->staffn;
			voice = pSlur->voice;
			
			if (pSlur->nEntries == 1) {
				if (ObjLType(pSlur->firstSyncL)==SYNCtype)
					aNoteL = FindMainNote(pSlur->firstSyncL, voice);
				else
					aNoteL = FindMainNote(pSlur->lastSyncL, voice);		/* ??But may someday want to use firstSync of FIRST piece of cross-sys slur for stem dir. */
				curveUp = (NoteYSTEM(aNoteL) > NoteYD(aNoteL));
				aSlur = GetPASLUR(aSlurL);
				SetSlurCtlPoints(doc, pL, aSlurL, pSlur->firstSyncL, pSlur->lastSyncL,
										staff, voice, context, curveUp);
				didAnything = True;
			}
			/* Handle a set of ties. */
			else {
				GetTiesCurveDir(doc, voice, pL, curveUps);
				for (i=0; aSlurL; i++, aSlurL=NextSLURL(aSlurL)) {
					aSlur = GetPASLUR(aSlurL);
					if (aSlur->selected) {
						SetSlurCtlPoints(doc, pL, aSlurL, pSlur->firstSyncL, pSlur->lastSyncL,
												staff, voice, context, curveUps[i]);
						didAnything = True;
					}
				}
			}
		}
	
	return didAnything;
}


/* ------------------------------------------------------------------ SetSelSlurAppear -- */
/* Set the appearance (line style) of every selected slur/tie. */

Boolean SetSelSlurAppear(Document *doc, short appearance)
{
	PASLUR	aSlur;
	LINK	pL, aSlurL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && SlurTYPE(pL))
			for (aSlurL=FirstSubLINK(pL); aSlurL; aSlurL=NextSLURL(aSlurL))
				if (SlurSEL(aSlurL)) {
					aSlur = GetPASLUR(aSlurL);
					aSlur->dashed = appearance;
					didAnything = True;
				}

	return didAnything;
}


/* --------------------------------------------------------------- SetSelLineThickness -- */
/* Set the line thickness of every selected GRDraw Graphic to a given value. */

Boolean SetSelLineThickness(Document *doc, short thickness)
{
	PGRAPHIC	pGraphic;
	LINK		pL;
	Boolean		didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (LinkSEL(pL) && GraphicTYPE(pL) && GraphicSubType(pL)==GRDraw) {
			pGraphic = GetPGRAPHIC(pL);
			pGraphic->gu.thickness = thickness;
			didAnything = True;
		}

	return didAnything;
}


/* ---------------------------------------------------------- SetSelPatchChangeVisible -- */
/* Set visibility of every selected PatchChange. */

Boolean SetSelPatchChangeVisible(Document *doc, Boolean visible)
{
	LINK	pL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && GraphicTYPE(pL)) {		
			if (GraphicSubType(pL)==GRMIDIPatch) {
				LinkVIS(pL) = visible;
				didAnything = True;
			}
		}
	}
	return didAnything;
}


/* ------------------------------------------------------------------ SetSelPanVisible -- */
/* Set visibility of every selected Pan. */

Boolean SetSelPanVisible(Document *doc, Boolean visible)
{
	LINK	pL;
	Boolean	didAnything=False;

	for (pL = doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL)) {
		if (LinkSEL(pL) && GraphicTYPE(pL)) {		
			if (GraphicSubType(pL)==GRMIDIPan) {
				LinkVIS(pL) = visible;
				didAnything = True;
			}
		}
	}
	return didAnything;
}
