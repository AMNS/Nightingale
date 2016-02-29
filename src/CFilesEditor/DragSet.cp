/***************************************************************************
*	FILE:	DragSet.c																			*
*	PROJ:	Nightingale, rev. for v.3.1													*
*	DESC:	"Set" routines for bitmap dragging											*
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1995-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* ------------------------------------------------------ SetClefFields and ally -- */
/* SetClefFields and other "set fields" functions translate the position of
the clef, etc. by xdDiff/ydDiff, and make translation consistent with
other factors, e.g., quantization to half lines, moving all notes in a
chord together, considering whether movement is vertical/horizontal, etc. */

/* Move the given clef by the given distance. After doing so, if none of the
clefs in the object is at the same position as the object anymore, move the
object to the closest one. */

void MoveClefHoriz(LINK, LINK, DDIST);
void MoveClefHoriz(LINK pL, LINK theClefL, DDIST xdDiff)
{
	LINK aClefL; PACLEF theClef; DDIST minXD;
	
	theClef = GetPACLEF(theClefL);
	theClef->xd += xdDiff;

	/*
	 *	If none of the clefs in pL is at the same position as pL itself is anymore,
	 * move pL to the closest one.
	 */
	aClefL = FirstSubLINK(pL);
	minXD = ClefXD(aClefL);
	for ( ; aClefL; aClefL = NextCLEFL(aClefL))
		if (ABS(ClefXD(aClefL))<ABS(minXD)) {
			minXD = ClefXD(aClefL);
		}
	if (minXD!=0) {
		aClefL = FirstSubLINK(pL);
		for ( ; aClefL; aClefL = NextCLEFL(aClefL))
			ClefXD(aClefL) -= minXD;
		LinkXD(pL) += minXD;
	}
}

/* Update fields of the given clef for dragging by (xdDiff,ydDiff). Also make
any other changes to the object list necessary for consistency. */

void SetClefFields(Document *doc, LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff,
							short /*xp*/, short /*yp*/, Boolean vert)
{
	short halfLnDiff;
	CONTEXT context;
	SignedByte oldClef, newClef;
	LINK doneL;
	
	if (vert) {
		GetContext(doc, pL, ClefSTAFF(subObjL), &context);

		/* Quantize vertical movement to half lines. */
		halfLnDiff = d2halfLn(ydDiff, context.staffHeight, context.staffLines);

		/* Change the clef type to reflect its being vertically dragged.
			SDGetClosestClef finds the closest legal clef corresponding to the
			clef's new halfLn position, and sets the subType of the clef to this
			new type. The yd of the new clef is not updated, since the new clef
			type describes its new vertical position.
			Then update the context for the change of clef type. */

		oldClef = ClefType(subObjL);
		SDGetClosestClef(doc, halfLnDiff, pL, subObjL, context);
		newClef = ClefType(subObjL);
		if (newClef!=oldClef) {
			doneL = FixContextForClef(doc, RightLINK(pL), ClefSTAFF(subObjL),
												oldClef, newClef);
			if (doneL) InvalSystems(RightLINK(pL), doneL);
		}

	}
	else
		MoveClefHoriz(pL, subObjL, xdDiff);

	InvalRange(pL, RightLINK(pL));
	LinkTWEAKED(pL) = TRUE;
}


/* ------------------------------------------------------ SetKeySigFields and ally - */

/* Move the given key sig by the given distance. After doing so, if none of the
key sigs in the object is at the same position as the object anymore, move the
object to the closest one. */

void MoveKeySigHoriz(LINK, LINK, DDIST);
void MoveKeySigHoriz(LINK pL, LINK theKeySigL, DDIST xdDiff)
{
	LINK aKeySigL; PAKEYSIG theKeySig; DDIST minXD;
	
	theKeySig = GetPAKEYSIG(theKeySigL);
	theKeySig->xd += xdDiff;

	/*
	 *	If none of the key sigs in pL is at the same position as pL itself is any-
	 * more, move pL to the closest one.
	 */
	aKeySigL = FirstSubLINK(pL);
	minXD = KeySigXD(aKeySigL);
	for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
		if (ABS(KeySigXD(aKeySigL))<ABS(minXD)) {
			minXD = KeySigXD(aKeySigL);
		}
	if (minXD!=0) {
		aKeySigL = FirstSubLINK(pL);
		for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
			KeySigXD(aKeySigL) -= minXD;
		LinkXD(pL) += minXD;
	}
}

/* Update fields of the given key sig for dragging by (xdDiff,ydDiff). Also make
any other changes to the object list necessary for consistency. */

void SetKeySigFields(LINK pL, LINK subObjL, DDIST xdDiff, short /*xp*/)
{
	MoveKeySigHoriz(pL, subObjL, xdDiff);

	InvalRange(pL, RightLINK(pL));
	LinkTWEAKED(pL) = TRUE;
}


/* ---------------------------------------------------- SetTimeSigFields and ally -- */

/* Move the given time sig by the given distance. After doing so, if none of the
time sigs in the object is at the same position as the object anymore, move the
object to the closest one. */

void MoveTimeSigHoriz(LINK, LINK, DDIST);
void MoveTimeSigHoriz(LINK pL, LINK theTimeSigL, DDIST xdDiff)
{
	LINK aTimeSigL; PATIMESIG theTimeSig; DDIST minXD;
	
	theTimeSig = GetPATIMESIG(theTimeSigL);
	theTimeSig->xd += xdDiff;

	/*
	 *	If none of the time sigs in pL is at the same position as pL itself is any-
	 * more, move pL to the closest one.
	 */
	aTimeSigL = FirstSubLINK(pL);
	minXD = TimeSigXD(aTimeSigL);
	for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
		if (ABS(TimeSigXD(aTimeSigL))<ABS(minXD)) {
			minXD = TimeSigXD(aTimeSigL);
		}
	if (minXD!=0) {
		aTimeSigL = FirstSubLINK(pL);
		for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
			TimeSigXD(aTimeSigL) -= minXD;
		LinkXD(pL) += minXD;
	}
}

/* Update fields of the given time sig for dragging by (xdDiff,ydDiff). Also make
any other changes to the object list necessary for consistency. */

void SetTimeSigFields(LINK pL, LINK subObjL, DDIST xdDiff, short /*xp*/)
{
	MoveTimeSigHoriz(pL, subObjL, xdDiff);

	InvalRange(pL, RightLINK(pL));
	LinkTWEAKED(pL) = TRUE;
}


/* ------------------------------------------------------ SetNoteFields and ally -- */

/* Move the given note/rest and, if it's in a chord, all other notes/rests in the
chord, by the given distance. After doing so, if none of the notes/rests in the
Sync is at the same position as the Sync anymore, move the Sync to the closest
one. */

void MoveNoteHoriz(LINK, LINK, DDIST);
void MoveNoteHoriz(LINK pL, LINK theNoteL, DDIST xdDiff)
{
	LINK aNoteL; PANOTE theNote, aNote; DDIST minXD;
	
	/* If theNote is in a chord, move all the notes in that chord. */

	theNote = GetPANOTE(theNoteL);
	if (theNote->inChord) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL);
			if (NoteVOICE(aNoteL)==NoteVOICE(theNoteL) && aNote->inChord)
				aNote->xd += xdDiff;
		}
	}
	else
		theNote->xd += xdDiff;

	/*
	 *	If none of the notes/rests in pL is at the same position as pL itself is
	 * anymore, move pL to the closest note or rest.
	 */
	aNoteL = FirstSubLINK(pL);
	minXD = NoteXD(aNoteL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (ABS(NoteXD(aNoteL))<ABS(minXD)) {
			minXD = NoteXD(aNoteL);
		}
	if (minXD!=0) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			NoteXD(aNoteL) -= minXD;
		LinkXD(pL) += minXD;
	}
}


/* Set the augmentation dot position for the given note to a reasonable standard value,
regardless of whether the note actually has dots or not. See more detailed comments on
FixAugDotPos (which should probably call this!). */

void FixNoteAugDotPos(SignedByte clefType, LINK aNoteL, short voiceRole, Boolean stemDown,
								Boolean lineNotesOnly);
void FixNoteAugDotPos(
			SignedByte clefType,
			LINK aNoteL,
			short voiceRole,
			Boolean stemDown,
			Boolean lineNotesOnly)						/* TRUE=set position for "line" notes only */	
{
	short	halfSp, midCHalfSp;
	Boolean lineNote, midCIsInSpace;

	midCHalfSp = ClefMiddleCHalfLn(clefType);						/* Get middle C staff pos. */		
	midCIsInSpace = odd(midCHalfSp);

	halfSp = qd2halfLn(NoteYQPIT(aNoteL));							/* Half-lines below middle C */
	lineNote = (midCIsInSpace? odd(halfSp) : !odd(halfSp));
	if (lineNote)															/* Note on line */
		NoteYMOVEDOTS(aNoteL) = GetLineAugDotPos(voiceRole, stemDown);
	else if (!lineNotesOnly)											/* Note in space */
		NoteYMOVEDOTS(aNoteL) = 2;
}


/* Return the Ottava object that the given Sync or GRSync belongs to on the given
staff. If none, return NILINK. */

LINK FindOttava(LINK pL, short staffn);
LINK FindOttava(LINK pL, short staffn)
{
	LINK octL;
	
	octL = LSSearch(pL, OTTAVAtype, staffn, GO_LEFT, FALSE);
	if (!octL) return NILINK;
	
	if (SyncInOTTAVA(pL, octL))
		return octL;
	else
		return NILINK;
}


/* Update fields of the given note/rest for dragging by (xdDiff,ydDiff). Also make
any other changes to the object list necessary for consistency. */

void SetNoteFields(Document *doc, LINK pL, LINK subObjL,
					DDIST xdDiff, DDIST ydDiff,
					short /*xp*/, short /*yp*/,
					Boolean vert,
					Boolean beam,		/* TRUE if dragging a beamset. */
					short newAcc		/* The new accidental resulting from vertical dragging. */
					)
{
	PANOTE aNote;
	LINK	 beamL, mainNoteL, octL;
	short  halfLnDiff, halfLn, staffn, v,
			 effectiveAcc, voiceRole, octType;
	DDIST	 dDiff, firstystem, lastystem;
	QDIST	 qStemLen;									/* y QDIST position relative to staff top */
	STDIST dystd;
	CONTEXT context;
	Boolean main, stemDown;
	
	aNote = GetPANOTE(subObjL);
	main = MainNote(subObjL);
	staffn = NoteSTAFF(subObjL);
	if (vert) {
		GetContext(doc, pL, staffn, &context);
		aNote = GetPANOTE(subObjL);

		/* Quantize vertical movement to half-lines or, for rests, lines. */
		halfLnDiff = d2halfLn(ydDiff,context.staffHeight,context.staffLines);
		if (aNote->rest)
			if (odd(halfLnDiff)) halfLnDiff += 1;
		dDiff = halfLn2d(halfLnDiff,context.staffHeight,context.staffLines);

		halfLn = qd2halfLn(aNote->yqpit);
		if (!aNote->rest)
			DelNoteFixAccs(doc, pL, staffn, halfLn,			/* Handle acc. context at old pos. */
									aNote->accident);

		/* Preserve zero stem length of non-main notes. */
		if (!main) aNote->ystem += dDiff;
		aNote->yd += dDiff;
		
		/* Update yqpit; this allows ledger lines to be drawn properly. */
		aNote->yqpit += halfLn2qd(halfLnDiff);
						
		dystd = halfLn2std(halfLnDiff);
		if (config.moveModNRs) MoveModNRs(subObjL, dystd);

		/* Update the new note accidental and, if necessary, propogate it. */
		aNote = GetPANOTE(subObjL);
		if (!aNote->rest) {
			aNote->accident = newAcc;
			InsNoteFixAccs(doc, pL, staffn, halfLn+halfLnDiff,	/* Handle acc. context at new pos. */
									aNote->accident);

			/* Update MIDI note number, considering effect of octave signs. */
			effectiveAcc = EffectiveAcc(doc, pL, subObjL);
			octL = FindOttava(pL, staffn);
			octType = (octL? OctType(octL) : -1);
			aNote = GetPANOTE(subObjL);
			if (octType>0)
				halfLn = -qd2halfLn(aNote->yqpit)+noteOffset[octType-1];
			else
				halfLn = -qd2halfLn(aNote->yqpit);
				
			aNote->noteNum = Pitch2MIDI(halfLn, effectiveAcc);
		}

		/* Correct augmentation dot vertical position (to avoid lines). */

		v = NoteVOICE(subObjL);
		mainNoteL = FindMainNote(pL, v);
		stemDown = (NoteYD(mainNoteL)<NoteYSTEM(mainNoteL));
		voiceRole = doc->voiceTab[v].voiceRole;
		FixNoteAugDotPos(context.clefType, subObjL, voiceRole, stemDown, FALSE);

		/* If this is a note in a chord, fix the entire chord's stems, notehead left/right
			of stem positions, accidental positions, etc.; else just fix this note's
			stem. */

		aNote = GetPANOTE(subObjL);
		if (beam && main)
			aNote->ystem += dDiff;
		else {
			if (aNote->inChord)
				FixSyncForChord(doc, pL, aNote->voice, aNote->beamed, 0, 0, NULL);
			else if (!aNote->beamed) {
				stemDown = GetStemInfo(doc, pL, subObjL, &qStemLen);
				NoteYSTEM(subObjL) = CalcYStem(doc, NoteYD(subObjL),
													NFLAGS(NoteType(subObjL)),
													stemDown,
													context.staffHeight, context.staffLines,
													qStemLen, FALSE);
			}
		}
	}
	else
		MoveNoteHoriz(pL, subObjL, xdDiff);

	InvalRange(pL, RightLINK(pL));

	/* Fix up stems if notes in beamset are dragged. Only case where this is
		necessary is if note is dragged horizontally in a slanted beamset. */
	if (!vert) {
		aNote = GetPANOTE(subObjL);
		if (aNote->beamed) {
			beamL = LVSearch(pL, BEAMSETtype, NoteVOICE(subObjL), TRUE, FALSE);
			if (!beamL) {
				MayErrMsg("SetNoteFields: no beamset for beamed note %ld", subObjL);
				return;
			}
			firstystem = Getystem(BeamVOICE(beamL), FirstInBeam(beamL));
			lastystem = Getystem(BeamVOICE(beamL), LastInBeam(beamL));

			/* If beam not horizontal, fix up note stems. Need the call to
				inval object, to inval a rect containing the beam, in case
				the beam extends outside the system. Need to modify the test
				<firstystem!=lastystem> in case the beam is crossStaff. */

			if (firstystem!=lastystem) {
				InvalObject(doc,beamL,TRUE);
				SDFixStemLengths(doc,beamL);
			}
		}
	}

	LinkTWEAKED(pL) = TRUE;
}


/* ------------------------------------------------------------ SetGRNoteFields -- */


/* Move the given grace note and, if it's in a chord, all other grace notes in the
chord, by the given distance. After doing so, if none of the grace notes in the
GRSync is at the same position as the GRSync anymore, move the GRSync to the
closest one. */

void MoveGRNoteHoriz(LINK, LINK, DDIST);
void MoveGRNoteHoriz(LINK pL, LINK theGRNoteL, DDIST xdDiff)
{
	LINK aGRNoteL; PAGRNOTE theGRNote, aGRNote; DDIST minXD;
	
	/* If theGRNote is in a chord, move all the grace notes in that chord. */

	theGRNote = GetPAGRNOTE(theGRNoteL);
	if (theGRNote->inChord) {
		aGRNoteL = FirstSubLINK(pL);
		for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
			aGRNote = GetPAGRNOTE(aGRNoteL);
			if (GRNoteVOICE(aGRNoteL)==GRNoteVOICE(theGRNoteL) && aGRNote->inChord)
				aGRNote->xd += xdDiff;
		}
	}
	else
		theGRNote->xd += xdDiff;

	/*
	 *	If none of the grace notes in pL is at the same position as pL itself is
	 * anymore, move pL to the closest one.
	 */
	aGRNoteL = FirstSubLINK(pL);
	minXD = GRNoteXD(aGRNoteL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
		if (ABS(GRNoteXD(aGRNoteL))<ABS(minXD)) {
			minXD = GRNoteXD(aGRNoteL);
		}
	if (minXD!=0) {
		aGRNoteL = FirstSubLINK(pL);
		for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
			GRNoteXD(aGRNoteL) -= minXD;
		LinkXD(pL) += minXD;
	}
}

/* Update fields of the given grace note for dragging by (xdDiff,ydDiff). Also make
any other changes to the object list necessary for consistency. */

void SetGRNoteFields(Document *doc, LINK pL, LINK subObjL,
						DDIST xdDiff, DDIST ydDiff,
						short /*xp*/, short /*yp*/,
						Boolean vert,
						Boolean /*beam*/,		/* TRUE if dragging a beamset. */
						short newAcc			/* The new accidental resulting from vertical dragging. */
						)
{
	PAGRNOTE aGRNote;
	LINK	 beamL, octL;
	short  midCHalfLn, halfLnDiff, halfLn, staffn, accKeep, effectiveAcc, octType;
	DDIST	 dDiff, firstystem, lastystem;
	CONTEXT context;
	Boolean main;
	
	aGRNote = GetPAGRNOTE(subObjL);
	main = GRMainNote(subObjL);
	staffn = GRNoteSTAFF(subObjL);
	if (vert) {
		GetContext(doc, pL, staffn, &context);
		aGRNote = GetPAGRNOTE(subObjL);

		/* Quantize vertical movement to half lines. */
		halfLnDiff = d2halfLn(ydDiff,context.staffHeight,context.staffLines);
		dDiff = halfLn2d(halfLnDiff,context.staffHeight,context.staffLines);

		if (!aGRNote->beamed && main) aGRNote->ystem += dDiff;
		aGRNote->yd += dDiff;
		
		/* Update yqpit; this allows ledger lines to be drawn properly. */
		aGRNote->yqpit += halfLn2qd(halfLnDiff);
				
		/* Re-compute ystem if note is not beamed & inChord. */

		aGRNote = GetPAGRNOTE(subObjL);
		if (!aGRNote->beamed && aGRNote->inChord)
			FixGRSyncForChord(doc, pL, aGRNote->voice, aGRNote->beamed, 0, TRUE, NULL);

		/* Update the new note accidental. */
		midCHalfLn = ClefMiddleCHalfLn(context.clefType);		/* Get middle C staff pos. */		
		aGRNote = GetPAGRNOTE(subObjL);
		aGRNote->accident = newAcc;
		halfLn = qd2halfLn(aGRNote->yqpit);
		GetPitchTable(doc, accTable, pL, staffn);					/* Get pitch modif. situation */
		if (newAcc!=0 && newAcc!=accTable[halfLn-midCHalfLn+ACCTABLE_OFF])	/* Note's acc. a change from prev. */																		/* Yep */
			accKeep = accTable[halfLn-midCHalfLn+ACCTABLE_OFF];
		else																						/* No change */
			accKeep = 0;
		FixAccidental(doc, RightLINK(pL), staffn, midCHalfLn-halfLn, accKeep);
		
		/* Update MIDI note number, considering effect of octave signs. */
		effectiveAcc = EffectiveGRAcc(doc, pL, subObjL);
		octL = FindOttava(pL, staffn);
		octType = (octL? OctType(octL) : -1);
		aGRNote = GetPAGRNOTE(subObjL);
		if (octType>0)
			halfLn = -qd2halfLn(aGRNote->yqpit)+noteOffset[octType-1];
		else
			halfLn = -qd2halfLn(aGRNote->yqpit);
			
		aGRNote->noteNum = Pitch2MIDI(halfLn, effectiveAcc);
	}
	else
		MoveGRNoteHoriz(pL, subObjL, xdDiff);

	InvalRange(pL, RightLINK(pL));

	/* Fix up stems if notes in beamset are dragged. Only case where this is
		necessary is if note is dragged horizontally in a slanted beamset. */
	if (!vert) {
		aGRNote = GetPAGRNOTE(subObjL);
		if (aGRNote->beamed) {
			beamL = LSSearch(pL, BEAMSETtype, staffn, TRUE, FALSE);
			if (!beamL) MayErrMsg("SetGRNoteFields: no beamset for beamed note %ld",
										(long)subObjL);
			firstystem = GetGRystem(BeamVOICE(beamL), FirstInBeam(beamL));
			lastystem = GetGRystem(BeamVOICE(beamL), LastInBeam(beamL));

			/* If beam not horizontal, fix up note stems. */
			if (firstystem!=lastystem)
				SDFixGRStemLengths(beamL);
		}
	}

	LinkTWEAKED(pL) = TRUE;
}


/* ------------------------------------------------------------ SetDynamicFields -- */

/* Update fields of the given Dynamic for dragging by (xdDiff,ydDiff). Also make
any other changes to the object list necessary for consistency. NB: this should
probably be updated in the same way as the SetClefFields function (e.g.) to call
a new MoveDynamHoriz function and to use InvalRange; however, (1) as of v.3.0,
Dynamics still can't have more than one subobject, so it's of no practical relevance,
and (2) Dynamics can be dragged both horizontally and vertically at the same time,
so it's a little more complicated to do here. */

void SetDynamicFields(LINK pL, LINK subObjL, DDIST xdDiff, DDIST ydDiff, short xp,
								short yp)
{
	PADYNAMIC aDynamic;
	PMEVENT p;
	Rect tempR;

	aDynamic = GetPADYNAMIC(subObjL);
	aDynamic->yd += ydDiff;
	if (LinkNENTRIES(pL)==1) {
		p = GetPMEVENT(pL);
		p->xd += xdDiff;
		OffsetRect(&LinkOBJRECT(pL),xp,yp);
	}
	else {
		aDynamic->xd += xdDiff;
		tempR = LinkOBJRECT(pL);
		OffsetRect(&tempR,xp,yp);
		UnionRect(&LinkOBJRECT(pL),&tempR,&LinkOBJRECT(pL));
	}
	if (IsHairpin(pL)) {
		aDynamic->endxd += xdDiff;
		aDynamic->endyd += ydDiff;
	}
	
	LinkTWEAKED(pL) = TRUE;
}

	
/* ------------------------------------------------------------- SetRptEndFields -- */

void SetRptEndFields(LINK pL, LINK /*subObjL*/, DDIST xdDiff, short xp)
{
	PMEVENT p;

	p = GetPMEVENT(pL);
	p->xd += xdDiff;
	OffsetRect(&LinkOBJRECT(pL),xp,0);

	LinkTWEAKED(pL) = TRUE;
}


/* ------------------------------------------------------------- SetEndingFields -- */

void SetEndingFields(Document *doc, LINK pL, LINK /*subObjL*/, DDIST xdDiff, DDIST ydDiff,
							short xp, short yp)
{
	PENDING p; Rect r;

	r = LinkOBJRECT(pL);
	InsetRect(&r,-4,-4);
	OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
	EraseAndInval(&r);

	p = GetPENDING(pL);
	p->xd += xdDiff;
	p->endxd += xdDiff;
	p->yd += ydDiff;
	OffsetRect(&LinkOBJRECT(pL),xp,yp);

	LinkTWEAKED(pL) = TRUE;
}


/* ------------------------------------------------------------- SetMeasureFields -- */
/* Returns a long percent by which to respace the preceeding measure, in case a
barline was dragged. */

long SetMeasureFields(Document *doc, LINK pL, DDIST xdDiff)
{
	short sysWidth;
	LINK	qL, pSystemL, measL, prevMeasL; PSYSTEM pSystem;
	long  oldMWidth, newMWidth, spaceFactor;
	PMEASURE prevMeas;
	
#ifdef CHECK_BARLINE_DRAG
	/* Check to see if dragging the barline will make symbols spill off the end
		of the system. */
	if (!CheckMoveMeasures(pL, xdDiff)) {
		GetIndCString(strBuf, MISCERRS_STRS, 22);    /* "Dragging barline will cause objects to extend past system boundary" */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}
#endif

	/* Move symbols in all measures after the dragged barline up or
		down by the amount the barline was moved, including symbols
		in the barline's measure itself. It is not possible to select
		the first invisible measure to drag it; if this becomes possible,
		an error will result when the prevMeas is found on the previous
		system, or NILINK, if no prevSys. */

	prevMeasL = LSSearch(LeftLINK(pL), MEASUREtype, ANYONE, TRUE, FALSE);
	oldMWidth = (long)(LinkXD(pL) - LinkXD(prevMeasL));
	qL = LastObjInSys(doc, pL);				
	MoveMeasures(pL, RightLINK(qL), xdDiff);

	/* Update the measure rects of the measures in the system of the
		dragged barline. */
	newMWidth = (long)(LinkXD(pL) - LinkXD(prevMeasL));
	LinkTWEAKED(pL) = TRUE;

	pSystemL = LSSearch(pL, SYSTEMtype, doc->currentSystem, TRUE, FALSE);
	pSystem = GetPSYSTEM(pSystemL);

	sysWidth = pSystem->systemRect.right-pSystem->systemRect.left;
	for (measL=pSystemL; measL!=RightLINK(qL); measL=RightLINK(measL))
		if (ObjLType(measL)==MEASUREtype) 
			PasteFixMeasureRect(measL, p2d(sysWidth));

	InvalRange(pSystemL, qL);
	InvalWindow(doc);

	/* Compute the amount by which to respace symbols in the measure
		before the dragged barline. */
	prevMeas = GetPMEASURE(prevMeasL);
	spaceFactor = RESFACTOR*(long)prevMeas->spacePercent;
	spaceFactor = (spaceFactor*newMWidth)/oldMWidth;
	prevMeas->spacePercent = (short)(spaceFactor/RESFACTOR);
	
	return spaceFactor;
}


/* -------------------------------------------------------------- SetPSMeasFields -- */

void SetPSMeasFields(Document */*doc*/, LINK pL, DDIST xdDiff)
{
	LinkXD(pL) += xdDiff;
	OffsetRect(&LinkOBJRECT(pL),d2p(xdDiff),0);

	LinkTWEAKED(pL) = TRUE;
}


/* -------------------------------------------------------------- SetTupletFields -- */

void SetTupletFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp)
{
	PTUPLET p;

	p = GetPTUPLET(pL);
	p->xdFirst += xdDiff;
	p->xdLast += xdDiff;
	p->ydFirst += ydDiff;
	p->ydLast += ydDiff;
	OffsetRect(&LinkOBJRECT(pL),xp,yp);

	LinkTWEAKED(pL) = TRUE;
}


/* -------------------------------------------------------------- SetOttavaFields -- */

void SetOttavaFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp)
{
	POTTAVA p;

	p = GetPOTTAVA(pL);
	p->xdFirst += xdDiff;
	p->xdLast += xdDiff;
	p->ydFirst += ydDiff;
	p->ydLast += ydDiff;
	OffsetRect(&LinkOBJRECT(pL),xp,yp);

	LinkTWEAKED(pL) = TRUE;
}


/* ------------------------------------------------------------- SetGraphicFields -- */

void SetGraphicFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp)
{
	PGRAPHIC p;
	
	p = GetPGRAPHIC(pL);
	
	p->xd += xdDiff;
	p->yd += ydDiff;
	if (p->graphicType==GRDraw) {
		p->info += xdDiff;
		p->info2 += ydDiff;
	}
	OffsetRect(&LinkOBJRECT(pL),xp,yp);
	LinkTWEAKED(pL) = TRUE;
}


/* -------------------------------------------------------------- SetTempoFields -- */

void SetTempoFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short xp, short yp)
{
	PTEMPO p;
	
	p = GetPTEMPO(pL);
	
	p->xd += xdDiff;
	p->yd += ydDiff;
	OffsetRect(&LinkOBJRECT(pL),xp,yp);
	LinkTWEAKED(pL) = TRUE;
}


/* -------------------------------------------------------------- SetSpaceFields -- */

void SetSpaceFields(LINK /*pL*/, DDIST /*xdDiff*/, DDIST /*ydDiff*/, short /*xp*/, short /*yp*/)
{
	/* There's nothing to do. */
}

/* --------------------------------------------------------------- SetBeamFields -- */

#define DRAGBEAM	3

void SetBeamFields(LINK pL, DDIST /*xdDiff*/, DDIST ydDiff, short /*xp*/, short /*yp*/)
{
	if (GraceBEAM(pL))
		FixGRStemLengths(pL, ydDiff, DRAGBEAM);
	else
		FixStemLengths(pL, ydDiff, DRAGBEAM);
}


/* --------------------------------------------------------------- SetSlurFields -- */

void SetSlurFields(LINK pL, DDIST xdDiff, DDIST ydDiff, short /*xp*/, short /*yp*/)
{
	PASLUR aSlur; LINK aSlurL;
	
	aSlurL = FirstSubLINK(pL);
	
	for ( ; aSlurL; aSlurL = NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
		
		if (aSlur->selected) {
			aSlur->seg.knot.h += xdDiff;
			aSlur->seg.knot.v += ydDiff;
	
			aSlur->endpoint.h += xdDiff;
			aSlur->endpoint.v += ydDiff;
		}
	}
}


/* ------------------------------------------------------------- SetForNewPitch -- */

Boolean DelSlurTie(Document *, LINK);		/* ?????? MOVE TO .h! ?????? */

void SetForNewPitch(Document *doc,
					LINK pL,					/* Sync or GRSync */
					LINK subObjL,
					CONTEXT context,
					short pitchLev, short acc
					)
{
	PANOTE aNote; PAGRNOTE aGRNote;
	short oldPitchLev, yp; DDIST ydDiff; LINK slurL, startL;
			
	if (SyncTYPE(pL)) {
		if (NoteTIEDL(subObjL)) {
			slurL = LeftSlurSearch(pL,NoteVOICE(subObjL),TRUE);
			DelSlurTie(doc,slurL);
		}
		if (NoteTIEDR(subObjL)) {
			startL = LeftLINK(pL);
			for ( ; pL && J_DTYPE(pL); pL = LeftLINK(pL)) ;
			slurL = LVSearch(startL,SLURtype,NoteVOICE(subObjL),GO_RIGHT,FALSE);
			DelSlurTie(doc,slurL);
		}

		aNote = GetPANOTE(subObjL);
		oldPitchLev = qd2halfLn(aNote->yqpit)+ClefMiddleCHalfLn(context.clefType);
		ydDiff = halfLn2d(pitchLev-oldPitchLev, context.staffHeight, context.staffLines);
		yp = d2p(ydDiff);
		SetNoteFields(doc, pL, subObjL, 0, ydDiff, 0, yp, TRUE, FALSE, acc);
	}
	else {
		aGRNote = GetPAGRNOTE(subObjL);
		oldPitchLev = qd2halfLn(aGRNote->yqpit)+ClefMiddleCHalfLn(context.clefType);
		ydDiff = halfLn2d(pitchLev-oldPitchLev, context.staffHeight, context.staffLines);
		yp = d2p(ydDiff);
		SetGRNoteFields(doc, pL, subObjL, 0, ydDiff, 0, yp, TRUE, FALSE, acc);
	}
}
