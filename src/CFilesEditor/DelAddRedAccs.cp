/* DelAddRedAccs.c for Nightingale - routines for deleting and adding accidentals. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* -------------------------------------------------- DelNoteRedAcc,DelGRNoteRedAcc -- */
/* If the note's accidental is redundant, delete the accidental. */

Boolean DelNoteRedAcc(Document *doc, short code, LINK syncL, LINK aNoteL,
								Boolean syncVChanged[])
{
	LINK prevSyncL, prevNoteL;
	short stf, halfLn, prevHalfLn;
	Boolean didAnything=False;
	
	stf = NoteSTAFF(aNoteL);
	GetPitchTable(doc, accTable, syncL, stf);						/* Get pitch modif. situation */
	halfLn = qd2halfLn(NoteYQPIT(aNoteL));							/* Half-lines below middle C */

	/*
	 * If the note is tied to the left, always remove its accidental unless it
	 * involves a spelling change or it's tied across a system break.
	 */
	if (NoteTIEDL(aNoteL)) {
		if (NoteACC(aNoteL)!=0
		&& (NoteACCSOFT(aNoteL) || code==DELALL_REDUNDANTACCS_DI)) {
			if (PrevTiedNote(syncL, aNoteL, &prevSyncL, &prevNoteL)) {
				/* prevNote = GetPANOTE(prevNoteL); */
				prevHalfLn = qd2halfLn(NoteYQPIT(prevNoteL));		/* Half-lines below middle C */
				if (halfLn==prevHalfLn) {
					NoteACC(aNoteL) = 0;
					didAnything = syncVChanged[NoteVOICE(aNoteL)] = True;
				}
			}
		}
	}

	if (NoteACC(aNoteL)!=0 &&										/* Is note's acc. a change from prev.? */
		 NoteACC(aNoteL)==accTable[halfLn+ACCTABLE_OFF])
		 if (NoteACCSOFT(aNoteL) || code==DELALL_REDUNDANTACCS_DI)	{ /* No */
			NoteACC(aNoteL) = 0;
			didAnything = syncVChanged[NoteVOICE(aNoteL)] = True;
		}

	return didAnything;
}


/* If the grace note's accidental is redundant, delete the accidental. */

Boolean DelGRNoteRedAcc(Document *doc, short code, LINK syncL, LINK aGRNoteL,
								Boolean syncVChanged[])
{
	short stf, halfLn;
	Boolean didAnything=False;

	stf = GRNoteSTAFF(aGRNoteL);
	GetPitchTable(doc, accTable, syncL, stf);						/* Get pitch modif. situation */
	halfLn = qd2halfLn(GRNoteYQPIT(aGRNoteL));						/* Half-lines below middle C */
	if (GRNoteACC(aGRNoteL)!=0 &&									/* Is grace note's acc. a change from prev.? */
		 GRNoteACC(aGRNoteL)==accTable[halfLn+ACCTABLE_OFF])
		 if (GRNoteACCSOFT(aGRNoteL) || code==DELALL_REDUNDANTACCS_DI)	{ /* No */
			GRNoteACC(aGRNoteL) = 0;
			didAnything = syncVChanged[GRNoteVOICE(aGRNoteL)] = True;
		}
	
	return didAnything;
}


/* --------------------------------------------- ArrangeSyncAccs, ArrangeGRSyncAccs -- */
/* In the given Sync, arrange the accidentals for notes/chords in all voices whose
flags are set in <syncVChanged>. */

void ArrangeSyncAccs(LINK syncL, Boolean syncVChanged[])
{
	CHORDNOTE	chordNote[MAXCHORD];
	short		voice, noteCount;
	Boolean		stemDown;

	for (voice = 1; voice<=MAXVOICES; voice++)
		if (syncVChanged[voice]) {
			stemDown = (GetStemUpDown(syncL, voice)<0);
			noteCount = GSortChordNotes(syncL, voice, stemDown, chordNote);
			ArrangeNCAccs(chordNote, noteCount, stemDown);
		}
}

/* In the given GRSync, arrange the accidentals for notes/chords in all voices whose
flags are set in <syncVChanged>. */

void ArrangeGRSyncAccs(LINK grSyncL, Boolean syncVChanged[])
{
	CHORDNOTE	chordNote[MAXCHORD];
	short		voice, noteCount;
	Boolean		stemDown;

	for (voice = 1; voice<=MAXVOICES; voice++)
		if (syncVChanged[voice]) {
			stemDown = (GetGRStemUpDown(grSyncL, voice)<0);
			noteCount = GSortChordGRNotes(grSyncL, voice, stemDown, chordNote);
			ArrangeGRNCAccs(chordNote, noteCount, stemDown);
		}
}


/* ------------------------------------------------------------- DelRedundantAccs -- */
/* Within the selection, if code=DELSOFT_REDUNDANTACCS_DI, delete soft accidentals
that are redundant (because of the key signature and/or accidentals earlier in the
bar); if code=DELALL_REDUNDANTACCS_DI, delete all accidentals that are redundant;
otherwise do nothing. If it found any to delete, return True, else False. Takes no
user-interface actions, e.g., redrawing.

This assumes standard CMN accidental-carrying rules including ties across barlines.
If ACC_IN_CONTEXT is False, it should probably do nothing.
*/

Boolean DelRedundantAccs(
			Document *doc,
			short stf,		/* Staff no. or ANYONE */
			short code
			)
{
	LINK pL, aNoteL;
	LINK aGRNoteL;
	Boolean didAnything, syncVChanged[MAXVOICES+1], anyStaff;
	short v;

	if (code!=DELALL_REDUNDANTACCS_DI && code!=DELSOFT_REDUNDANTACCS_DI)
		return False;

	anyStaff = (stf==ANYONE);
		
	didAnything = False;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				if (LinkSEL(pL)) {
					for (v = 1; v<=MAXVOICES; v++)
						syncVChanged[v] = False;
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if ((anyStaff || NoteSTAFF(aNoteL)==stf) && NoteSEL(aNoteL)) 
							if (DelNoteRedAcc(doc, code, pL, aNoteL, syncVChanged))
								didAnything = True;						
					ArrangeSyncAccs(pL, syncVChanged);
				}
				break;
			case GRSYNCtype:
				if (LinkSEL(pL)) {
					for (v = 1; v<=MAXVOICES; v++)
						syncVChanged[v] = False;
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
						if ((anyStaff || GRNoteSTAFF(aGRNoteL)==stf) && GRNoteSEL(aGRNoteL))
							if (DelGRNoteRedAcc(doc, code, pL, aGRNoteL, syncVChanged))
								didAnything = True;						
					ArrangeGRSyncAccs(pL, syncVChanged);
				}
				break;
		default:
			;
		}
		

	if (didAnything) doc->changed = True;
	return didAnything;
}


/* ------------------------------------------------------------- AddRedundantAccs -- */
/* Within the selection, for any note that has no accidental (either because it's
natural or because a previous accidental applies under the standard CMN accidental-
carrying rules), give that note an explicit accidental. Do so regardless of ties
??REALLY? . The note's <accSoft> is not changed.

If code=ALL_ADDREDUNDANTACCS_DI, add redundant accidentals to all notes; if code=
NONAT_ADDREDUNDANTACCS_DI, add to all notes unless they'd get a natural; otherwise
do nothing.

If any accidentals were added, return True, else False. Takes no user-interface
actions, e.g., redrawing. */

Boolean AddRedundantAccs(
			Document	*doc,
			short		stf,		/* Staff no. or ANYONE */
			short		code,
			Boolean		addTiedLeft
			)
{
	LINK		pL, aNoteL;
	LINK		aGRNoteL;
	Boolean	didAnything, syncVChanged[MAXVOICES+1], anyStaff, addNaturals;
	short		v, eAcc, acc;

	if (code!=NONAT_ADDREDUNDANTACCS_DI && code!=ALL_ADDREDUNDANTACCS_DI)
		return False;

	addNaturals = (code==ALL_ADDREDUNDANTACCS_DI);
	anyStaff = (stf==ANYONE);
		
	didAnything = False;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				if (LinkSEL(pL)) {
					for (v = 1; v<=MAXVOICES; v++)
						syncVChanged[v] = False;
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if ((anyStaff || NoteSTAFF(aNoteL)==stf) && NoteSEL(aNoteL)) {
							acc = NoteACC(aNoteL);
						   eAcc = EffectiveAcc(doc, pL, aNoteL);
					 		if (acc) continue;
							if (!addNaturals && eAcc==AC_NATURAL) continue;
							if (!addTiedLeft && NoteTIEDL(aNoteL)) continue;
					   	NoteACC(aNoteL) = eAcc;
					   	v = NoteVOICE(aNoteL);
					   	syncVChanged[v] = didAnything = True;	
						}				
					ArrangeSyncAccs(pL, syncVChanged);
				}
				break;
			case GRSYNCtype:
				if (LinkSEL(pL)) {
					for (v = 1; v<=MAXVOICES; v++)
						syncVChanged[v] = False;
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
						if ((anyStaff || GRNoteSTAFF(aGRNoteL)==stf) && GRNoteSEL(aGRNoteL)) {
							acc = GRNoteACC(aGRNoteL);
						   eAcc = EffectiveGRAcc(doc, pL, aGRNoteL);
					 		if (!acc)
								if (addNaturals || eAcc!=AC_NATURAL) {
							   	GRNoteACC(aGRNoteL) = eAcc;
							   	v = GRNoteVOICE(aGRNoteL);
							   	syncVChanged[v] = didAnything = True;	
								}	
						}				
					ArrangeGRSyncAccs(pL, syncVChanged);
				}
				break;
			default:
				;
		}


	if (didAnything) doc->changed = True;
	return didAnything;
}


/* ---------------------------------------- MIDI2EffectiveAcc, MIDI2EffectiveGRAcc -- */

/* Given a Sync and a note in that Sync, return the code for the note's correct effective
accidental, based on comparing the notation to its MIDI note number. If the notation and
note number are so far apart they can't be reconciled with an accidental, return
ERROR_INT. ??Could be generally useful: belongs in PitchUtils.c. */

short MIDI2EffectiveAcc(Document *doc, short clefType, short octType, LINK syncL, LINK theNoteL);
short MIDI2EffectiveAcc(
			Document */*doc*/,
			short clefType, short octType,
			LINK /*syncL*/, LINK theNoteL
			)
{
	short halfLn;										/* Relative to the top of the staff */
	SHORTQD yqpit;
	short midCHalfLn, noteNum, delta;

	midCHalfLn = ClefMiddleCHalfLn(clefType);					/* Get middle C staff pos. */		
	yqpit = NoteYQPIT(theNoteL)+halfLn2qd(midCHalfLn);
	halfLn = qd2halfLn(yqpit);									/* Number of half lines from stftop */

	if (octType>0)
		noteNum = Pitch2MIDI(midCHalfLn-halfLn+noteOffset[octType-1], AC_NATURAL);
	else
		noteNum = Pitch2MIDI(midCHalfLn-halfLn, AC_NATURAL);
	
	delta = NoteNUM(theNoteL)-noteNum;
	if (delta<-2 || delta>2) return ERROR_INT;
	return delta+AC_NATURAL;
}


/* Given a GRSync and a grace note in that GRSync, return the code for the grace note's
correct effective accidental, based on comparing the notation to its MIDI note number.
If the notation and note number are so far apart they can't be reconciled with an
accidental, return ERROR_INT. ??Could be generally useful: belongs in PitchUtils.c. */

short MIDI2EffectiveGRAcc(Document *doc, short clefType, short octType, LINK syncL, LINK theGRNoteL);
short MIDI2EffectiveGRAcc(
			Document */*doc*/,
			short clefType, short octType,
			LINK /*syncL*/, LINK theGRNoteL
			)
{
	short halfLn;									/* Relative to the top of the staff */
	SHORTQD yqpit;
	short midCHalfLn, noteNum, delta;

	midCHalfLn = ClefMiddleCHalfLn(clefType);					/* Get middle C staff pos. */		
	yqpit = GRNoteYQPIT(theGRNoteL)+halfLn2qd(midCHalfLn);
	halfLn = qd2halfLn(yqpit);									/* Number of half lines from stftop */

	if (octType>0)
		noteNum = Pitch2MIDI(midCHalfLn-halfLn+noteOffset[octType-1], AC_NATURAL);
	else
		noteNum = Pitch2MIDI(midCHalfLn-halfLn, AC_NATURAL);
	
	delta = GRNoteNUM(theGRNoteL)-noteNum;
	if (delta<-2 || delta>2) return ERROR_INT;
	return delta+AC_NATURAL;
}


/* ---------------------------------------------------------- AddMIDIRedundantAccs -- */
/* For any note (or grace note) that has no explicit accidental because a previous
accidental applies under the standard CMN accidental-carrying rules, give that note
an accidental. Do so from the beginning of the score to the given point, and regardless
of ties ??REALLY?. This is not intended to be called in response to user action for
the specific note, so the note/grace note's <accSoft> is set to True.

We don't assume the accidentals are in a consistent state: instead we decide for each
note based on its MIDI note number. This was written for use with the clipboard, which
contains material pulled out of context, but should work fine on any score unless the
user is doing something wierd like using a diamond notehead for an artificial harmonic.
(If this turns out to be a real problem, this function could consider notehead shape
and, e.g., ask the user what to do with diamond or other odd-shaped notes.) Cf.
AddRedundantAccs, which offers more flexibility in some ways but assumes the existing
accidentals are consistent.

If any accidentals were added, return True, else False. Takes no user-interface
actions, e.g., redrawing. */

Boolean AddMIDIRedundantAccs(
			Document	*doc,
			LINK		endAddAccsL,
			Boolean		addTiedLeft,
			Boolean		addNaturals
			)
{
	LINK	pL, aNoteL, aGRNoteL, aClefL;
	Boolean	didAnything, syncVChanged[MAXVOICES+1];
	short	voice, eAcc, acc;
	short	staff, clefType[MAXSTAVES+1], octType[MAXSTAVES+1];
		
	/*
	 * We'll need each note's clef and octave sign. Clefs are easy, since there are
	 * always clefs for all staves at the beginning of the object list, and a new clef
	 * affects all notes on its staff. We keep track of where octave signs begin, but keeping
	 * track of where they end is messy: to avoid doing that, we just rely on each Note
	 * subobj's inOttava flag to say whether it's in one.
	 */
	for (staff = 1; staff<=doc->nstaves; staff++)
		octType[staff] = 0;											/* Initially, no octave sign */

	didAnything = False;
	for (pL = doc->headL; pL && pL!=endAddAccsL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				for (voice = 1; voice<=MAXVOICES; voice++)
					syncVChanged[voice] = False;
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					if (NoteREST(aNoteL)) continue;
					acc = NoteACC(aNoteL);
			 		if (acc!=0) continue;
			 		staff = NoteSTAFF(aNoteL);
					if (DETAIL_SHOW)
						LogPrintf(LOG_DEBUG, "AddMIDIRedundantAccs: pL=%d aNoteL=%d stf=%d clf=%d oct=%d\n",
							pL, aNoteL, staff, clefType[staff], octType[staff]);
				   eAcc = MIDI2EffectiveAcc(doc, clefType[staff], octType[staff], pL, aNoteL);
				   if (eAcc==ERROR_INT) {
				   	MayErrMsg("AddMIDIRedundantAccs: can't find acc for pL=%ld", (long)pL);
						continue;
					}
					if (!addNaturals && eAcc==AC_NATURAL) continue;
					if (!addTiedLeft && NoteTIEDL(aNoteL)) continue;
			   	NoteACC(aNoteL) = eAcc;
					NoteACCSOFT(aNoteL) = True;
			   	voice = NoteVOICE(aNoteL);
			   	syncVChanged[voice] = didAnything = True;	
				}				
				ArrangeSyncAccs(pL, syncVChanged);
				break;
			case GRSYNCtype:
				for (voice = 1; voice<=MAXVOICES; voice++)
					syncVChanged[voice] = False;
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					acc = GRNoteACC(aGRNoteL);
			 		if (!acc) {
			 			staff = NoteSTAFF(aGRNoteL);
				   	eAcc = MIDI2EffectiveGRAcc(doc, clefType[staff], octType[staff], pL, aGRNoteL);
					   if (eAcc==ERROR_INT) {
					   	MayErrMsg("AddMIDIRedundantAccs: can't find acc for pL=%ld", (long)pL);
							continue;
						}
						if (addNaturals || eAcc!=AC_NATURAL) {
					   	GRNoteACC(aGRNoteL) = eAcc;
							GRNoteACCSOFT(aGRNoteL) = True;
					   	voice = GRNoteVOICE(aGRNoteL);
					   	syncVChanged[voice] = didAnything = True;	
						}
					}
				}				
				ArrangeGRSyncAccs(pL, syncVChanged);
				break;
			case CLEFtype:
				aClefL = FirstSubLINK(pL);
				for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
					staff = ClefSTAFF(aClefL);
					clefType[staff] = ClefType(aClefL);
				}
				break;
			case OTTAVAtype:
				if (OttavaSTAFF(pL)==staff) octType[staff] = OctType(pL);
				break;
			default:
				;
		}

	if (didAnything) doc->changed = True;
	return didAnything;
}
