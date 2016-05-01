/*	PitchUtils.c
	Utilities for handling pitches of notes and grace notes. Includes nothing at
	user-interface level.
		
	GetSharpsOrFlats		KeySigEqual				KeySigCopy				
	ClefMiddleCHalfLn		Char2Acc				Acc2Char
	EffectiveAcc			EffectiveGRAcc			Pitch2MIDI
	Pitch2PC				PCDiff					GetRespell
	FixRespelledVoice		Char2LetName			LetName2Char
	RespellChordSym			RespellNote				GetTranspSpell
	TranspChordSym			TranspNote				TranspGRNote
	DTranspChordSym			DTranspNote				DTranspGRNote
	TranspKS				TranspKeySig
	CompareNCNotes			CompareNCNoteNums
	PSortNotes				PSortChordNotes			GSortNotes
	GSortChordNotes			GSortChordGRNotes		ArrangeNCAccs
	ArrangeChordNotes		ArrangeGRNCAccs			ArrangeChordGRNotes
	FixAccidental			FixAllAccidentals
	GetAccTable				KeySig2AccTable			GetPitchTable
	DelNoteFixAccs			InsNoteFixAccs
*/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static short GetTranspSpell(short, short, short, short, short *, short *, Boolean *);
static void PSortNotes(CHORDNOTE [], short, Boolean);
static void GSortNotes(CHORDNOTE [], short, Boolean);


/* ------------------------------------------------------------------- MoveModNRs -- */
/* Move all MODNRs attached to the given note/rest up or down by the given distance.
Not really a "pitch" utility but this seems to be the best place for this. */

void MoveModNRs(LINK aNoteL, STDIST dystd)
{
	PANOTE aNote; LINK aModNRL; PAMODNR aModNR; STDIST newystd;
	
	aNote = GetPANOTE(aNoteL);
	aModNRL = aNote->firstMod;
	if (aModNRL)
		for ( ; aModNRL; aModNRL = NextMODNRL(aModNRL)) {
			aModNR = GetPAMODNR(aModNRL);
			if (aModNR->modCode<MOD_TREMOLO1 || aModNR->modCode>MOD_TREMOLO6) {
				newystd = aModNR->ystdpit+dystd;
				if (newystd<-128) newystd = -128;	/* Clip new position to legal range */
				if (newystd>127) newystd = 127;
				aModNR->ystdpit = newystd;
			}
		}
}

/* ------------------------------------------------------------ GetSharpsOrFlats -- */
/* Deliver the signed number of sharps or flats in the standard key signature: >0 =
sharps, <0 = flats. Should not be called for nonstandard key signatures (as of v3.0,
not implemented, anyway). */ 

short GetSharpsOrFlats(PCONTEXT pContext)
{
	short nitems;

	nitems = pContext->nKSItems;
	return (pContext->KSItem[0].sharp? nitems : -nitems);
}

/* ----------------------------------------------------------------- KeySigEqual -- */
/* Are ks1 and ks2 the same key signature? */

Boolean KeySigEqual(PKSINFO ks1, PKSINFO ks2)
{
	short i, curnKSItems;
	
	if ( (curnKSItems = ks1->nKSItems)!=ks2->nKSItems)
		return FALSE;  
	for (i = 0; i<curnKSItems; i++)
	{
		if (ks1->KSItem[i].sharp!=ks2->KSItem[i].sharp)
			return FALSE;
		if (ks1->KSItem[i].letcode!=ks2->KSItem[i].letcode)
			return FALSE;
	}
	return TRUE;														/* They're the same */
}


/* ------------------------------------------------------------------ KeySigCopy -- */
/* Copy key signature info from ks1 into ks2. NB: Should not be used when either
argument is an address of something in a Nightingale heap unless that heap is
locked, since merely calling this function can load a segment and move memory,
resulting in this function storing into an unpredicatable place! In such cases,
use the KEYSIG_COPY macro (BlockMove is impractical: see comments on KEYSIG_COPY). */

void KeySigCopy(PKSINFO ks1, PKSINFO ks2)
{
	short i;
	
	ks2->nKSItems = ks1->nKSItems;			
	for (i = 0; i<ks1->nKSItems; i++)
		ks2->KSItem[i] = ks1->KSItem[i];
}


/* ----------------------------------------------------------- ClefMiddleCHalfLn -- */
/*	Given clef, get staff half-line number of middle C. */

short ClefMiddleCHalfLn(SignedByte clefType)
{
	switch (clefType) {
		case TREBLE8_CLEF:		return 17;
		case TREBLE_CLEF:
		case PERC_CLEF:			return 10;
		case SOPRANO_CLEF:		return 8;
		case MZSOPRANO_CLEF:		return 6;
		case ALTO_CLEF:			return 4;
		case TRTENOR_CLEF:		return 3;
		case TENOR_CLEF:			return 2;
		case BARITONE_CLEF:		return 0;
		case BASS_CLEF:			return -2;
		case BASS8B_CLEF:			return -9;
		default:
			MayErrMsg("ClefMiddleCHalfLn: bad clef %ld", (long)clefType);
			return 0;
	}
}


/* ----------------------------------------------------------- Char2Acc,Acc2Char -- */

short Char2Acc(unsigned char charAcc)
{
	short acc, i;
	
	acc = -1;
	for (i =1; i<=5; i++)
		if (charAcc==SonataAcc[i]) acc = i;
	return acc;
}

unsigned char Acc2Char(short acc)
{	
	return SonataAcc[acc];
}


/* ---------------------------------------------------------------- EffectiveAcc -- */
/* Given a note, return its effective accidental, considering its explicit accidental,
the key signature, and previous accidentals on its staff position in its measure, but
NOT considering the possibility that its pitch is affected by a note it's tied to.
To take the latter into account, first call FirstTiedNote. */

short EffectiveAcc(Document *doc, LINK syncL, LINK aNoteL)
{
	short		halfLn, effectiveAcc;
	PANOTE	aNote;
	
	aNote = GetPANOTE(aNoteL);	
	halfLn = qd2halfLn(aNote->yqpit);
	if (aNote->accident==0) {
		GetPitchTable(doc, accTable, syncL, NoteSTAFF(aNoteL));	/* Get pitch modif. situation */
		effectiveAcc = accTable[halfLn+ACCTABLE_OFF];
	}
	else
		effectiveAcc = aNote->accident;

	return effectiveAcc;
}


/* -------------------------------------------------------------- EffectiveGRAcc -- */
/* Given a grace note, return its effective accidental. The same comments apply as for
EffectiveAcc, except there's no problem with ties because (as of v.3.1) we don't
allow tied grace notes. */

short EffectiveGRAcc(Document *doc, LINK grSyncL, LINK aGRNoteL)
{
	short		halfLn, effectiveAcc;
	PAGRNOTE	aGRNote;
	
	aGRNote = GetPAGRNOTE(aGRNoteL);	
	halfLn = qd2halfLn(aGRNote->yqpit);
	if (aGRNote->accident==0) {
		GetPitchTable(doc, accTable, grSyncL, GRNoteSTAFF(aGRNoteL));	/* Get pitch modif. situation */
		effectiveAcc = accTable[halfLn+ACCTABLE_OFF];
	}
	else
		effectiveAcc = aGRNote->accident;

	return effectiveAcc;
}


/* --------------------------------------------------------- Pitch2MIDI,Pitch2PC -- */

static short	halfLine2semi[] =				/* White-key semitone offset for */ 
					{ 0, 2, 4, 5, 7, 9, 11 };	/*   half-line positions above C */

/* Convert pitch to MIDI note number. */

short Pitch2MIDI(
		short	halfLines,					/* Half-lines above middle C */
		short	acc 							/* Code for accidental */
		)
{
	short		letterName,
				octave,
				halfSteps;
	
	letterName = halfLines % 7;					/* Convert to note letter (C=0, D=1...) */
	if (letterName<0) letterName += 7;			/* Correct stupidity in mod operator */
	octave = halfLines/7;							/* Get octave number */
	if (halfLines<0 && letterName!=0)			/* Truncate downwards */
		octave -= 1;
	halfSteps = halfLine2semi[letterName];		/* Convert letter name to halfsteps */
	if (acc!=0)
		halfSteps += acc-AC_NATURAL;				/* If an accidental, add it in */
	return MIDI_MIDDLE_C+(12*octave+halfSteps);
}


/* Convert pitch to pitch class (0=B#/C/Dbb, 1=C#/Db, ...11=A##/B/Cb). */

short Pitch2PC(
		short	halfLines,				/* Half-lines above C: 0=C, 1=D...6=B */
		short	acc 						/* Code for accidental; 0 and AC_NATURAL act the same */
		)
{
	short		notePC;
	
	notePC = halfLine2semi[halfLines];
	if (acc!=0) notePC += acc-AC_NATURAL;			/* If an accidental, add it in */
	if (notePC<0) notePC += 12;
	if (notePC>11) notePC -= 12;
	return notePC;
}


/* ---------------------------------------------------------------------- PCDiff -- */
/* Get pitch-class difference between two letter names */

short PCDiff(short newLet, short let, Boolean up)
{
	short diff;
	
	diff = Pitch2PC(newLet, 0)-Pitch2PC(let, 0);
	if (diff<0 && up)			return diff+12;
	else if (diff>0 && !up) return diff-12;
	else							return diff;
}


/* ------------------------------------------------------------------ GetRespell -- */
/* Given a note's letter-name-and-accidental spelling, return a different good
spelling and, as function value, the change in half-line (+=up).  If we can't find
a different good spelling, return 0. Note that key signature is not considered, so
(e.g.) B# will never be suggested.

GetRespell does not care if the input spelling makes sense, so you can always get
a good spelling simply by passing GetRespell a ridiculous one, e.g., with letName=0
and acc=1 (accidental of double-flat). */

short sharpSpelling[12][2] = {
	/* pitchclass C     C#           D     D#           E     F     F#           G	*/
				    {0,0},{0,AC_SHARP},{1,0},{1,AC_SHARP},{2,0},{3,0},{3,AC_SHARP},{4,0},
				    		 {4,AC_SHARP},{5,0},{5,AC_SHARP},{6,0}
	/*						  G#           A     A#           B										*/
};

short flatSpelling[12][2] = {
	/* pitchclass C     Db          D     Eb          E     F     Gb          G     */
				    {0,0},{1,AC_FLAT},{1,0},{2,AC_FLAT},{2,0},{3,0},{4,AC_FLAT},{4,0},
				    		 {5,AC_FLAT},{5,0},{6,AC_FLAT},{6,0}
	/*                  Ab          A     Bb          B                             */
};

short GetRespell(
		short letName, 
		short acc,							/* Effective (not actual) accidental */
		short *pNewLetName,
		short *pNewAcc
		)
{
	short notePC, deltaHL;
	Boolean didIt = FALSE;

	notePC = Pitch2PC(letName, acc);							/* Get note's pitch class */
	
	/* Try the sharp spelling first, then the flat one. The order matters only where
	 *	both work, i.e., with Cbb, Fbb, B##, E##. Trying sharp first will make them
	 *	respell to A#, D#, C#, F# rather than Bb, Eb, Db, Gb; this seems inconsistent.
	 *	Oh well.
	 */
	if (letName!=sharpSpelling[notePC][0]					/* Given anything but sharpSpelling? */
	||  acc!=sharpSpelling[notePC][1]) {
		*pNewLetName = sharpSpelling[notePC][0];			/* Yes. Use sharpSpelling */
		*pNewAcc = sharpSpelling[notePC][1];
		didIt = TRUE;
	}
	
	else if (letName!=flatSpelling[notePC][0]				/* Given anything but flatSpelling? */
	||  acc!=flatSpelling[notePC][1]) {
		*pNewLetName = flatSpelling[notePC][0];			/* Yes. Use flatSpelling */
		*pNewAcc = flatSpelling[notePC][1];
		didIt = TRUE;
	}

	if (didIt) {
		deltaHL = *pNewLetName-letName; 
		if (deltaHL>=5) deltaHL -= 7;							/* Handle C<->B, Cbb<->A#, etc. */
		if (deltaHL<=-5) deltaHL += 7;
		return deltaHL;
	}
	else
		return 0;
}


/* ----------------------------------------------------------- FixRespelledVoice -- */
/* Set the graphic parameters of all notes (or grace notes) in the given voice in
the given Sync (or GRSync) to reasonable values, whether there's a chord or not, on
the assumption one or more of the notes has been respelled or transposed. NB: if
notes have been moved into or out of the voice, may not give correct results; for
that, cf. FixSyncVoice in SetUtils.c, which probably should be combined with this. */

void FixRespelledVoice(
				Document *doc,
				LINK pL,
				short voice)		/* Sync or GRSync */
{
	LINK		aNoteL, aGRNoteL;
	CONTEXT	context;
	Boolean	stemDown;
	QDIST		qStemLen;
	
	if (SyncTYPE(pL)) {
		aNoteL = FindMainNote(pL, voice);
		if (aNoteL) {
			if (NoteINCHORD(aNoteL)) FixSyncForChord(doc, pL, voice,
														NoteBEAMED(aNoteL), 0, 0, NULL);
			else {
				stemDown = GetStemInfo(doc, pL, aNoteL, &qStemLen);
				GetContext(doc, pL, NoteSTAFF(aNoteL), &context);
				NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL),
													NFLAGS(NoteType(aNoteL)),
													stemDown,
													context.staffHeight, context.staffLines,
													qStemLen, FALSE);
				}
			FixAugDotPos(doc, pL, voice, FALSE);
		}
	}
	else if (GRSyncTYPE(pL)) {
		aGRNoteL = FindGRMainNote(pL, voice);
		if (aGRNoteL) {
				if (GRNoteINCHORD(aGRNoteL)) FixGRSyncForChord(doc, pL, voice,
															GRNoteBEAMED(aGRNoteL), 0, TRUE, NULL);
				else
					FixGRSyncNote(doc, pL, voice, NULL);
		}
	}
}


/* ---------------------------------------------------- Char2LetName,LetName2Char -- */
/* Functions to convert a character from A thru G to or from a letter-name code, with
C=0, D=1...B=6. They do no error checking! */

short letNameCodes[7] = { 5, 6, 0, 1, 2, 3, 4 };

short Char2LetName(char charLet)
{
	/*
	 * The following depends on the assumption that character codes for letter A thru
	 * G are consecutive integers: this is true in every character set I know of, even
	 * EBCDIC. And it's very fast. Still, I'm not totally happy with it. --DAB
	 */
	return letNameCodes[charLet-'A'];
}


char charCodes[7] = { 'C', 'D', 'E', 'F', 'G', 'A', 'B' };

char LetName2Char(short letName)
{
	return charCodes[letName];
}


/* ------------------------------------------------------------- RespellChordSym -- */
/* Respell chord symbol <pL> in the "obvious" way, if there is one; if not, leave it
alone. This does exactly the same thing Respell does to notes, described below.
Delivers TRUE if it changes the chord symbol. */

Boolean RespellChordSym(Document */*doc*/, LINK pL)
{
	LINK			aGraphicL;
	StringOffset theStrOffset;
	unsigned char	string[256];
	short			i, k, letName, acc, newLetName, newAcc,
					letPos, accPos;
	
	aGraphicL = FirstSubLINK(pL);
	theStrOffset = GraphicSTRING(aGraphicL);
	PStrCopy((StringPtr)PCopy(theStrOffset), (StringPtr)string);

	/*
	 *	Look thru the string for occurences of the capital letters A thru G. When one
	 *	is found, we assume it's a pitch letter name, so try to recognize the next
	 *	character as an accidental; if we don't recognize it, assume natural. Respell
	 *	the letter name and accidental and replace them in the string with the result.
	 *	If a chord symbol with any accidental becomes one with a natural, we delete
	 *	the accidental.
	 */
	for (i = 1; i<=string[0]; i++) {
		if (string[i]>='A' && string[i]<='G') {
			letName = Char2LetName(string[i]);
			letPos = i;
			
			acc = AC_NATURAL;
			accPos = -1;
			if (i+1<=string[0]) {
				acc = Char2Acc(string[i+1]);
				accPos = (acc>0? i+1 : -1);
				if (acc<=0) acc = AC_NATURAL;
			}
			
			if (!GetRespell(letName, acc, &newLetName, &newAcc)) return FALSE;
			string[letPos] = LetName2Char(newLetName);

			if (newAcc!=AC_NATURAL && accPos<0) {
				MayErrMsg("RespellChordSym: No room in chord symbol for new accidental.");
				return FALSE;
			}
			/*
			 *	If we don't need an accidental but there is room for one, remove it.
			 */
			if (newAcc==AC_NATURAL && accPos>=0) {
				for (k = accPos; k<string[0]; k++)
					string[k] = string[k+1];
				string[0]--;
				accPos = -1;
			}
			if (accPos>=0) {
				string[accPos] = Acc2Char(newAcc);
				i++;											/* Advance an extra char. next time thru loop */
			}
		}
	}

	theStrOffset = PReplace(GraphicSTRING(aGraphicL),string);
	if (theStrOffset<0L)
		{ NoMoreMemory(); return FALSE; }
	else
		GraphicSTRING(aGraphicL) = theStrOffset;
	return TRUE;
}


/* ----------------------------------------------------------------- RespellNote -- */
/* Respell the given note in the "obvious" way, if there is one; if not, leave it
alone. For example (and regardless of key signature):
	Abb => G
	Ab => G#
	A stays as it is
	A# => Bb
	A-double# => B
	Cb => B
	B# => C
Also add accidentals to following notes where possibly necessary to keep their pitches
the same. For example, if the note is Db and the first following notes in its voice
are C and D with no barlines between, this note gets changed to C#, and the following
notes gets changed to C-natural and D-flat; the natural on the C is certainly required
but the flat on the D may not be, since there may still be a D with a flat earlier in
the measure. Return TRUE if we do anything, else FALSE. If RespellNote does return
TRUE, the caller probably should fix up any chord the note is in and perhaps delete
redundant accidentals. */

Boolean RespellNote(Document *doc, LINK syncL, LINK aNoteL, PCONTEXT pContext)
{
	PANOTE	aNote;
	short		stf, halfLn, effectiveAcc, letName, deltaHL,
				newLetName, newAcc;
	STDIST 	dystd;
	DDIST		dyd;

	effectiveAcc = EffectiveAcc(doc, syncL, aNoteL);
	if (NoteACC(aNoteL)!=0 && NoteACC(aNoteL)!=AC_NATURAL) {
	/*
	 * We have a note with an accidental other than natural. Get a new spelling for
	 * it, and consider updating accidentals on the next following notes on both its
	 *	old and its new line/spaces. If the new spelling requires no accidental, put
	 *	a natural on it anyway in case the line/space is affected by a previous note.
	 *	(As described above, this will tend to produce redundant accidentals.)
	 */
		aNote = GetPANOTE(aNoteL);
		halfLn = qd2halfLn(aNote->yqpit);
		letName = (7-halfLn) % 7;								/* Get note letter (C=0, D=1...) */
		if (letName<0) letName += 7;							/* Correct stupidity in mod operator */
		if (deltaHL = GetRespell(letName, effectiveAcc,
											&newLetName, &newAcc)) {
			stf = aNote->staffn;
			DelNoteFixAccs(doc, syncL, stf, halfLn,			/* Handle acc. context at old pos. */
									aNote->accident);
			aNote->yqpit -= halfLn2qd(deltaHL);
			dyd = halfLn2d(deltaHL, pContext->staffHeight,
											pContext->staffLines);
  			if (aNote->ystem==aNote->yd) aNote->ystem -= dyd;	/* Preserve non-MainNote-ness */
			aNote->yd -= dyd;
			aNote->accident = (newAcc? newAcc : AC_NATURAL);
			dystd = -halfLn2std(deltaHL);
			if (config.moveModNRs) MoveModNRs(aNoteL, dystd);
			InsNoteFixAccs(doc, syncL, stf, halfLn-deltaHL,	/* Handle acc. context at new pos. */
									aNote->accident);
			return TRUE;
		}
	}
	
	return FALSE;
}


/* --------------------------------------------------------------- RespellGRNote -- */
/* Does exactly the same thing as RespellNote, except for grace notes. */

Boolean RespellGRNote(Document *doc, LINK grSyncL, LINK aGRNoteL, PCONTEXT pContext)
{
	PAGRNOTE	aGRNote;
	short		stf, halfLn, effectiveAcc, letName, deltaHL,
				newLetName, newAcc;
	DDIST		dyd;

	effectiveAcc = EffectiveGRAcc(doc, grSyncL, aGRNoteL);
	if (GRNoteACC(aGRNoteL)!=0 && GRNoteACC(aGRNoteL)!=AC_NATURAL) {
	/*
	 * Have a grace note with an accidental other than natural. Get a new spelling for
	 * it, and consider updating accidentals on the next following notes on both its
	 *	old and its new line/spaces. If the new spelling requires no accidental, put
	 *	a natural on it anyway in case the line/space is affected by a previous note.
	 *	(As described above, this will tend to produce redundant accidentals.)
	 */
		aGRNote = GetPAGRNOTE(aGRNoteL);
		halfLn = qd2halfLn(aGRNote->yqpit);
		letName = (7-halfLn) % 7;								/* Get note letter (C=0, D=1...) */
		if (letName<0) letName += 7;							/* Correct stupidity in mod operator */
		if (deltaHL = GetRespell(letName, effectiveAcc,
											&newLetName, &newAcc)) {
			stf = aGRNote->staffn;
			DelNoteFixAccs(doc, grSyncL, stf, halfLn,				/* Handle acc. context at old pos. */
									aGRNote->accident);
			aGRNote->yqpit -= halfLn2qd(deltaHL);
			dyd = halfLn2d(deltaHL, pContext->staffHeight,
											pContext->staffLines);
  			if (aGRNote->ystem==aGRNote->yd) aGRNote->ystem -= dyd;	/* Preserve non-MainNote-ness */
			aGRNote->yd -= dyd;
			aGRNote->accident = (newAcc? newAcc : AC_NATURAL);
			InsNoteFixAccs(doc, grSyncL, stf, halfLn-deltaHL,	/* Handle acc. context at new pos. */
									aGRNote->accident);
			return TRUE;
		}
	}
	
	return FALSE;
}


/* -------------------------------------------------------------- GetTranspSpell -- */
/* Given (1) a note's letter-name-and-effective-accidental spelling, and (2) a
transposition (signed) number of steps, and semitone change: return the correct
spelling for the new pitch and, as function value, the change in half-line (+=up).
If the new spelling would require more than two flats or sharps, return a simpler
spelling (e.g., Fbb up a perfect fourth should be Bbbb(!), but instead give Ab)
and set <*cantSpell>. Note that key signature is not considered, so (e.g.) B#
will never be suggested. Assumes the transposition is, in letter names, no more
than an octave and, in semitones, less than an octave, so -7<=steps<=7, and
-11<=semiChange<=11.  Diminished octave is OK; augmented 7th isn't.

Letter names are represented as 0=C, 1=D...6=B. */

static short GetTranspSpell(
					short		letName,
					short		acc,
					short		steps,
					short		semiChange,
					short		*pNewLetName,
					short		*pNewAcc,
					Boolean	*cantSpell
					)
{
	short newLetName, newAcc, accChange, deltaHL;
	
	if (ABS(steps)>7 || ABS(semiChange)>11)
		MayErrMsg("GetTranspSpell: steps=%ld or semiChange=%ld is illegal.",
					(long)steps, (long)semiChange);
	*cantSpell = FALSE;
	
	/*
	 *	Compute the new letter name and how many semitones pitch change the change
	 *	in letter names produces; then the remaining number of semitones must be
	 *	taken care of by changing the accidental.
	 */
	newLetName = (letName+steps) % 7;
	if (newLetName<0) newLetName += 7;
	accChange = semiChange-PCDiff(newLetName, letName, steps>0);
	newAcc = acc+accChange;
	
	if (newAcc<1 || newAcc>5) {
		*cantSpell = TRUE;
		if (newAcc<1) newLetName--;
		else			  newLetName++;
		accChange = semiChange-PCDiff(newLetName, letName, steps>0);
		newAcc = acc+accChange;
	}

	*pNewLetName = newLetName;
	*pNewAcc = newAcc;
	
	/*
	 *	First set the half-lines change from the change in letter names, ignoring
	 *	octaves; then, if the result is going in the opposite direction from
	 * <steps>, add or subtract an octave to compensate. If the result is 0, see
	 * whether steps is 7; if so, the interval must be some kind of octave, so
	 * again add or subtract an octave.
	 */
	deltaHL = *pNewLetName-letName; 
	if (deltaHL>0 && steps<0) deltaHL -= 7;
	if (deltaHL<0 && steps>0) deltaHL += 7;
	if (deltaHL==0 && !(*cantSpell)) {
		if (steps<0) deltaHL = -7;						/* Octave down */
		if (steps>0) deltaHL = 7;						/* Octave up */
	}
	return deltaHL;
}


/* -------------------------------------------------------------- TranspChordSym -- */
/* Transpose chord symbol <pL> by the specified amount. For example, transposing
down a minor 3rd would be indicated steps=-2, semitones=-3 (or, since octaves are
ignored, -15 or -27); down an augmented unison steps=0, semitones=-1. Since the
chord symbol is stored as a string of characters, it must be parsed. We simply assume
that any capital letter A thru G is a pitch letter name and the next character after
one may be an accidental. We transpose the letter names and accidentals and leave
any other characters in the string untouched.

Return TRUE if all is well, FALSE if there's a problem. */

Boolean TranspChordSym(
				Document */*doc*/,
				LINK		pL,
				short		steps,			/* Signed no. of diatonic steps transposition */
				short		semiChange 		/*	Signed total transposition in semitones */
				)
{
	LINK			aGraphicL;
	StringOffset theStrOffset;
	unsigned char	string[256];
	short			i, k, letName, acc, newLetName, newAcc, letPos, accPos;
	Boolean		cantSpell;
	
	aGraphicL = FirstSubLINK(pL);
	theStrOffset = GraphicSTRING(aGraphicL);
	PStrCopy((StringPtr)PCopy(theStrOffset), (StringPtr)string);

	/*
	 *	Look thru the string for occurences of the capital letters A thru G. When one
	 *	is found, we assume it's a pitch letter name, so try to recognize the next
	 *	character as an accidental; if we don't recognize it, assume natural. Transpose
	 *	the letter name and accidental and replace them in the string with the result.
	 *	If a chord symbol with no accidental becomes one with an accidental, we have to
	 *	insert a character; if one with any accidental becomes one with a natural, we
	 *	delete the accidental.
	 */
	for (i = 1; i<=string[0]; i++) {
		if (string[i]>='A' && string[i]<='G') {
			letName = Char2LetName(string[i]);
			letPos = i;
			
			acc = AC_NATURAL;
			accPos = -1;
			if (i+1<=string[0]) {
				acc = Char2Acc(string[i+1]);
				accPos = (acc>0? i+1 : -1);
				if (acc<=0) acc = AC_NATURAL;
			}
			
			GetTranspSpell(letName, acc, steps, semiChange, &newLetName, &newAcc,
									&cantSpell);
			string[letPos] = LetName2Char(newLetName);
			/*
			 *	If we need an accidental but there's no room for one, try to make room.
			 *	If we don't need an accidental but there is room for one, remove it.
			 */
			if (newAcc!=AC_NATURAL && accPos<0) {
				if (string[0]>=255) {
					MayErrMsg("Can't make room in chord symbol for new accidental.");
					return FALSE;
				}
				for (k = string[0]; k>letPos; k--)
					string[k+1] = string[k];
				string[0]++;
				accPos = letPos+1;
			}
			if (newAcc==AC_NATURAL && accPos>=0) {
				for (k = accPos; k<string[0]; k++)
					string[k] = string[k+1];
				string[0]--;
				accPos = -1;
			}
			if (accPos>=0) {
				string[accPos] = Acc2Char(newAcc);
				i++;											/* Advance an extra char. next time thru loop */
			}
		}
	}

	theStrOffset = PReplace(GraphicSTRING(aGraphicL),string);
	if (theStrOffset<0L)
		{ NoMoreMemory(); return FALSE; }
	else
		GraphicSTRING(aGraphicL) = theStrOffset;

	return !cantSpell;
}


/* ------------------------------------------------------------------ TranspNote -- */
/* Given a note that needs to be transposed, get a new spelling for it, and
consider updating accidentals on the next following notes on both its old and
its new line/spaces. If the new spelling requires no accidental, put a natural
on it anyway in case the new line/space is affected by a previous note. (Both
the iterative process with repeated notes and the "natural just in case" will
produce redundant accidentals, which should be removed at a higher level than
this routine.) Also move the notehead up or down; this will leave the stem in
need of updating.

This does not check that the resulting note has a legal MIDI note number! */

Boolean TranspNote(
					Document *doc,
					LINK		syncL,
					LINK		aNoteL,
					CONTEXT	context,
					short		octaves,			/* Signed no. of octaves transposition */
					short		steps,			/* Signed no. of diatonic steps transposition */
					short		semiChange 		/*	Signed total transposition in semitones */
					)
{
	short effectiveAcc, stf, halfLn, letName, deltaHL,
			newLetName, newAcc, deltaHLOct; 
	PANOTE aNote;
	Boolean cantSpell;
	STDIST dystd;
	DDIST dyd;
	
	effectiveAcc = EffectiveAcc(doc, syncL, aNoteL);
	aNote = GetPANOTE(aNoteL);
	stf = aNote->staffn;
	halfLn = qd2halfLn(aNote->yqpit);
	letName = (7-halfLn) % 7;										/* Get note letter (C=0, D=1...) */
	if (letName<0) letName += 7;									/* Correct stupidity in mod operator */
	deltaHL = GetTranspSpell(letName, effectiveAcc,
										  steps, semiChange,
										  &newLetName, &newAcc,
										  &cantSpell);
	deltaHLOct = deltaHL+7*octaves;
	DelNoteFixAccs(doc, syncL, stf, halfLn,					/* Handle accidental context */
							aNote->accident);
	aNote = GetPANOTE(aNoteL);
	aNote->yqpit -= halfLn2qd(deltaHLOct);
	dyd = halfLn2d(deltaHLOct, context.staffHeight,
							context.staffLines);
	if (aNote->ystem==aNote->yd) aNote->ystem -= dyd;		/* Preserve non-MainNote-ness */
	aNote->yd -= dyd;
	dystd = -halfLn2std(deltaHLOct);
	if (config.moveModNRs) MoveModNRs(aNoteL, dystd);
	if (aNote->accident==0) aNote->accSoft = TRUE;			/* New accidental... */
	aNote->accident = (newAcc? newAcc : AC_NATURAL);		/*	is prog.-generated */
	aNote->noteNum += semiChange+12*octaves;
	InsNoteFixAccs(doc, syncL, stf, halfLn-deltaHLOct,		/* Handle accidental context */
							aNote->accident);

	return !cantSpell;
}


/* ---------------------------------------------------------------- TranspGRNote -- */
/* Given a grace note that needs to be transposed, get a new spelling for it, and
consider updating accidentals on the next following notes on both its old and
its new line/spaces. If the new spelling requires no accidental, put a natural
on it anyway in case the new line/space is affected by a previous note. (Both
the iterative process with repeated notes and the "natural just in case" will
produce redundant accidentals, which should be removed at a higher level than
this routine.) Also move the notehead up or down; this will leave the stem in
need of updating.

This does not check that the resulting note has a legal MIDI note number! */

Boolean TranspGRNote(
					Document *doc,
					LINK		grSyncL,
					LINK		aGRNoteL,
					CONTEXT	context,
					short		octaves,				/* Signed no. of octaves transposition */
					short		steps,				/* Signed no. of diatonic steps transposition */
					short		semiChange 			/*	Signed total transposition in semitones */
					)
{
	short effectiveAcc, stf, halfLn, letName, deltaHL,
			newLetName, newAcc, deltaHLOct; 
	PAGRNOTE aGRNote;
	Boolean cantSpell;
	DDIST dyd;
	
	effectiveAcc = EffectiveGRAcc(doc, grSyncL, aGRNoteL);
	aGRNote = GetPAGRNOTE(aGRNoteL);
	stf = aGRNote->staffn;
	halfLn = qd2halfLn(aGRNote->yqpit);
	letName = (7-halfLn) % 7;										/* Get note letter (C=0, D=1...) */
	if (letName<0) letName += 7;									/* Correct stupidity in mod operator */
	deltaHL = GetTranspSpell(letName, effectiveAcc,
										  steps, semiChange,
										  &newLetName, &newAcc,
										  &cantSpell);
	deltaHLOct = deltaHL+7*octaves;
	DelNoteFixAccs(doc, grSyncL, stf, halfLn,					/* Handle accidental context */
							aGRNote->accident);
	aGRNote = GetPAGRNOTE(aGRNoteL);
	aGRNote->yqpit -= halfLn2qd(deltaHLOct);

	dyd = halfLn2d(deltaHLOct, context.staffHeight,
							context.staffLines);
	if (aGRNote->ystem==aGRNote->yd) aGRNote->ystem -= dyd;	/* Preserve non-MainNote-ness */
	aGRNote->yd -= dyd;
	if (aGRNote->accident==0) aGRNote->accSoft = TRUE;		/* New accidental... */
	aGRNote->accident = (newAcc? newAcc : AC_NATURAL);		/*	is prog.-generated */
	aGRNote->noteNum += semiChange+12*octaves;
	InsNoteFixAccs(doc, grSyncL, stf, halfLn-deltaHLOct,	/* Handle accidental context */
							aGRNote->accident);
	
	return !cantSpell;
}


/* -------------------------------------------------------------- DTranspChordSym -- */
/* Transpose chord symbol <pL> diatonically by the specified amount. For example,
transposing down a 3rd would be indicated steps=-2. Since the chord symbol is stored
as a string of characters, it must be parsed. We simply assume that any capital letter
A thru G is a pitch letter name and the next character after one may be an accidental.
We transpose the letter names and leave any other characters in the string untouched. */

void DTranspChordSym(Document */*doc*/,
							LINK pL,
							short steps)		/* Signed no. of diatonic steps transposition */
{
	LINK			aGraphicL;
	StringOffset theStrOffset;
	unsigned char	string[256];
	short			i, letName, newLetName;
	
	aGraphicL = FirstSubLINK(pL);
	theStrOffset = GraphicSTRING(aGraphicL);
	PStrCopy((StringPtr)PCopy(theStrOffset), (StringPtr)string);

	/*
	 *	Look thru the string for occurences of the capital letters A thru G. When one
	 *	is found, we assume it's a pitch letter name. Transpose the letter name and
	 *	replace it in the string with the result.
	 */
	for (i = 1; i<=string[0]; i++) {
		if (string[i]>='A' && string[i]<='G') {
			letName = Char2LetName(string[i]);
						
			newLetName = (letName+steps) % 7;
			if (newLetName<0) newLetName += 7;					/* Correct stupidity in mod operator */
			string[i] = LetName2Char(newLetName);
		}
	}

	theStrOffset = PReplace(GraphicSTRING(aGraphicL),string);
	if (theStrOffset<0L)
		{ NoMoreMemory(); return; }								/* Should never happen */
	else
		GraphicSTRING(aGraphicL) = theStrOffset;
}


/* ----------------------------------------------------------------- DTranspNote -- */
/* Given a note that needs to be diatonically transposed, transpose it, and
consider updating accidentals on the next following notes on both its old and
its new line/spaces. If the new spelling requires no accidental, put a natural
on it anyway in case the new line/space is affected by a previous note. (Both
the iterative process with repeated notes and the "natural just in case" will
produce redundant accidentals, which should be removed at a higher level than
this routine.) Also move the notehead up or down; this will leave the stem in
need of updating.

This does not check that the resulting note has a legal MIDI note number! */

void DTranspNote(
				Document *doc,
				LINK		syncL,
				LINK		aNoteL,
				CONTEXT	context,
				Boolean	goUp,					/* TRUE=transpose up, else down */
				short		octaves,				/* Signed no. of octaves transposition */
				short		steps 				/* Signed no. of diatonic steps transposition */
				)
{
	short effectiveAcc, stf, halfLn, letName,
			newEffectiveAcc, newHalfLn, newLetName,
			stepsOct, semiChangeOct;
	STDIST dystd;
	PANOTE aNote;
	DDIST dyd;
	
	stepsOct = steps+7*octaves;
	effectiveAcc = EffectiveAcc(doc, syncL, aNoteL);
	aNote = GetPANOTE(aNoteL);
	stf = aNote->staffn;
	halfLn = qd2halfLn(aNote->yqpit);
	letName = (7-halfLn) % 7;									/* Get note letter (C=0, D=1...) */
	if (letName<0) letName += 7;								/* Correct stupidity in mod operator */
	DelNoteFixAccs(doc, syncL, stf, halfLn,				/* Handle accidental context */
							aNote->accident);
	aNote->yqpit -= halfLn2qd(stepsOct);

	dyd = halfLn2d(stepsOct, context.staffHeight,
							context.staffLines);
	if (aNote->ystem==aNote->yd) aNote->ystem -= dyd;	/* Preserve non-MainNote-ness */
	aNote->yd -= dyd;

	dystd = -halfLn2std(stepsOct);
	if (config.moveModNRs) MoveModNRs(aNoteL, dystd);
	newEffectiveAcc = EffectiveAcc(doc, syncL, aNoteL);
	newHalfLn = qd2halfLn(aNote->yqpit);
	newLetName = (7-newHalfLn) % 7;							/* Get note letter (C=0, D=1...) */
	if (newLetName<0) newLetName += 7;						/* Correct stupidity in mod operator */
	semiChangeOct = PCDiff(newLetName, letName, goUp)+12*octaves;
	semiChangeOct += newEffectiveAcc-effectiveAcc;
	aNote->noteNum += semiChangeOct;
	InsNoteFixAccs(doc, syncL, stf, halfLn-stepsOct,	/* Handle accidental context */
							aNote->accident);
}


/* ---------------------------------------------------------------- DTranspGRNote -- */
/* Given a grace note that needs to be diatonically transposed, transpose it, and
consider updating accidentals on the next following notes on both its old and
its new line/spaces. If the new spelling requires no accidental, put a natural
on it anyway in case the new line/space is affected by a previous note. (Both
the iterative process with repeated notes and the "natural just in case" will
produce redundant accidentals, which should be removed at a higher level than
this routine.) Also move the notehead up or down; this will leave the stem in
need of updating.

This does not check that the resulting note has a legal MIDI note number! */

void DTranspGRNote(
				Document *doc,
				LINK		grSyncL,
				LINK		aGRNoteL,
				CONTEXT	context,
				Boolean	goUp,					/* TRUE=transpose up, else down */
				short		octaves,				/* Signed no. of octaves transposition */
				short		steps 				/* Signed no. of diatonic steps transposition */
				)
{
	short effectiveAcc, stf, halfLn, letName,
			newEffectiveAcc, newHalfLn, newLetName,
			stepsOct, semiChangeOct;
	PAGRNOTE aGRNote;
	DDIST dyd;
	
	stepsOct = steps+7*octaves;
	effectiveAcc = EffectiveGRAcc(doc, grSyncL, aGRNoteL);
	aGRNote = GetPAGRNOTE(aGRNoteL);
	stf = aGRNote->staffn;
	halfLn = qd2halfLn(aGRNote->yqpit);
	letName = (7-halfLn) % 7;									/* Get note letter (C=0, D=1...) */
	if (letName<0) letName += 7;								/* Correct stupidity in mod operator */
	DelNoteFixAccs(doc, grSyncL, stf, halfLn,				/* Handle accidental context */
							aGRNote->accident);
	aGRNote->yqpit -= halfLn2qd(stepsOct);

	dyd = halfLn2d(stepsOct, context.staffHeight,
							context.staffLines);
	if (aGRNote->ystem==aGRNote->yd) aGRNote->ystem -= dyd;	/* Preserve non-MainNote-ness */
	aGRNote->yd -= dyd;

	newEffectiveAcc = EffectiveGRAcc(doc, grSyncL, aGRNoteL);
	newHalfLn = qd2halfLn(aGRNote->yqpit);
	newLetName = (7-newHalfLn) % 7;							/* Get note letter (C=0, D=1...) */
	if (newLetName<0) newLetName += 7;						/* Correct stupidity in mod operator */
	semiChangeOct = PCDiff(newLetName, letName, goUp)+12*octaves;
	semiChangeOct += newEffectiveAcc-effectiveAcc;
	aGRNote->noteNum += semiChangeOct;
	InsNoteFixAccs(doc, grSyncL, stf, halfLn-stepsOct,	/* Handle accidental context */
							aGRNote->accident);
}


/* ---------------------------------------------------------------- DiatonicSteps -- */
/* Returns the SIGNED no. of  diatonic steps between two pitch classes. Pitch classes
are represented 0=C, 1=C#/Db, 2=D,  ...10=A#/Bb, 11=B; no other possibilities (B#,
Gbb, etc.) are considered. */

static short DiatonicSteps(Boolean, short, short);
static short DiatonicSteps(
						Boolean useSharps,			/* TRUE=use sharps for black-key notes, FALSE=use flats */
						short ind, short newInd		/* The two pitch classes */
						)
{
	/* Diatonic scale degrees, one octave plus an extra entry at each ends */
	/* 						  (B) C   C#  D   D#  E   F   F#  G   G#  A   A#  B  (C) */
	Byte sharpDegree[] = { 6,  0,  0,  1,  1,  2,  3,  3,  4,  4,  5,  5,  6, 0 }; 
	Byte flatDegree[] =  { 6,  0,  1,  1,  2,  2,  3,  4,  4,  5,  5,  6,  6, 0 }; 
	/* 						  (B) C   Db  D   Eb  E   F   Gb  G   Ab  A   Bb  B  (C) */

	short stepCount, i, iMod;
	Boolean goUp=(ind<newInd);

	if (ind==newInd) return 0;
	 	
	/* Find out how many diatonic steps we'll cover if we stick with the sharpness or
		flatness of the original. NB: All indices are 1 higher than they would otherwise
		be to compensate for the dummy entries. */
	
	stepCount = 0;
	if (goUp)
		for (i = ind+1; i<=newInd; i++) {
			iMod = (i+12) % 12;										/* If past end of array, wrap */
			if (useSharps)
				{ if (sharpDegree[iMod+1]!=sharpDegree[iMod]) stepCount++; }
			else 
				{ if (flatDegree[iMod+1]!=flatDegree[iMod]) stepCount++; }
		}
	else
		for (i = ind-1; i>=newInd; i--) {
			iMod = (i+12) % 12;										/* If past start of array, wrap */
			if (useSharps)
				{ if (sharpDegree[iMod+1]!=sharpDegree[iMod+2]) stepCount--; }
			else 
				{ if (flatDegree[iMod+1]!=flatDegree[iMod+2]) stepCount--; }
		}
	
	return stepCount;
}


/* -------------------------------------------------------------------- TranspKS -- */
/* Given the no. of sharps or flats in the given conventional key sig., return
the no. of sharps or flats in the key sig. transposed as described. If there is
no such transposed key sig., return with *cantSpell=TRUE (in which case the
function value is nonsense!), else *cantSpell=FALSE. */

static short TranspKS(short, short, short, Boolean *);
static short TranspKS(
					short		sharpsOrFlats,		/* -=abs. value is no. of flats, else no. of sharps */
					short		steps,				/* Signed no. of diatonic steps transposition */
					short		semiChange,			/*	Signed total transposition in semitones */
					Boolean	*cantSpell
					)
{
	/* Major key equiv.: C   C#  D   D#  E   F   F#  G   G#  A   A#  B */
	SignedByte nSOFSharp[] = { 0,  7,  2,  99, 4,  -1, 6,  1,  99, 3,  99, 5 };
	SignedByte nSOFFlat[] =  { 0,  -5, 2,  -3, 4,  -1, -6, 1,  -4, 3,  -2, 5 };
	/* Major key equiv.: C   Db  D   Eb  E   F   Gb  G   Ab  A   Bb  B */

	short ind, newInd, stepCount, needSteps, newSOF;
	Boolean enharmSwitch, useSharps;

	useSharps = (sharpsOrFlats>=0);

	/* Find index in our array of original and new key sigs. For the moment, we'll
		let the new key sig. index go past the array end, but correct it later. */
	if (useSharps) {
		for (ind = 0; ind<12; ind++)
			if (nSOFSharp[ind]==sharpsOrFlats) break;
	}
	else {
		for (ind = 0; ind<12; ind++)
			if (nSOFFlat[ind]==sharpsOrFlats) break;
	}
	newInd = ind+semiChange;									/* May be negative or >12 */

	/* Find out how many diatonic steps we'll cover if we stick with the sharpness
		or flatness of the original key sig. */
	
	stepCount = DiatonicSteps(useSharps, ind, newInd);
	
	/* Correct index past array end, being careful about mod-of-negative stupidity. */

	newInd = (newInd+12) % 12;

	/* If the original key was sharp, we can get an extra step by switching to
		flats; if the original key was flat, we can get rid of a step by switching
		to sharps. For example, with an original key of 2 sharps: if steps=2 and
		semiChange=4 (a major third), we'll get newSOF=6 (sharps); if steps=3 and
		semiChange=4 (a diminished fourth), we switch to flats and get newSOF=-6
		(flats). Assuming a major key, this is changing from D to F# or to Gb. */
		
	needSteps = stepCount-steps;
	enharmSwitch = FALSE;
	*cantSpell = FALSE;
	switch (needSteps) {
		case -1:
			if (sharpsOrFlats<0) *cantSpell = TRUE;
			else enharmSwitch = TRUE;
			break;
		case 0:
			break;
		case 1:
			if (sharpsOrFlats>0) *cantSpell = TRUE;
			else enharmSwitch = TRUE;
			break;
		default:
			*cantSpell = TRUE;
	}
	
	if (enharmSwitch) useSharps = !useSharps;
	
	newSOF = (useSharps? nSOFSharp[newInd] : nSOFFlat[newInd]);
	if (newSOF>=99) *cantSpell = TRUE;
	
	return newSOF;
}


/* ------------------------------------------------------------ XGetSharpsOrFlats -- */
/* Deliver the number of sharps or flats in the standard key signature. Should not
be called for nonstandard key signatures (which aren't implemented as of v.3.1,
anyway). */ 

short XGetSharpsOrFlats(PKSINFO pKSInfo)
{
	short nitems;

	nitems = pKSInfo->nKSItems;
	return (pKSInfo->KSItem[0].sharp? nitems : -nitems);
}


/* ---------------------------------------------------------------- TranspKeySig -- */
/* Given a key signature subobject that needs to be transposed, transpose it and
update everything following (key sig. context fields and accidentals on notes/grace
notes) until the next key signature on its staff. Return TRUE if we can do the
transposition, FALSE if it results in an illegal key sig., i.e., one that would
require double-sharps or double-flats. */

Boolean TranspKeySig(
					Document *doc,
					LINK		keySigL,
					LINK		aKeySigL,
					CONTEXT	/*context*/,
					short		steps,				/* Signed no. of diatonic steps transposition */
					short		semiChange 			/*	Signed total transposition in semitones */
					)
{
	short stf, oldSOF, newSOF; PAKEYSIG aKeySig; Boolean cantSpell;
	KSINFO oldKSInfo, newKSInfo; LINK lastL;
	
	stf = KeySigSTAFF(aKeySigL);
PushLock(KEYSIGheap);
	aKeySig = GetPAKEYSIG(aKeySigL);
	KeySigCopy((PKSINFO)aKeySig->KSItem, &oldKSInfo);			/* Safe bcs heap locked */
	oldSOF = XGetSharpsOrFlats(&oldKSInfo);
	newSOF = TranspKS(oldSOF, steps, semiChange, &cantSpell);
	if (cantSpell) {
PopLock(KEYSIGheap);
		return FALSE;
	}

	SetupKeySig(aKeySigL, newSOF);
	KeySigCopy((PKSINFO)aKeySig->KSItem, &newKSInfo);
	aKeySig->visible = TRUE;
PopLock(KEYSIGheap);

	FixContextForKeySig(doc, RightLINK(keySigL), stf, oldKSInfo, newKSInfo);
	lastL = FixAccsForKeySig(doc, keySigL, stf, oldKSInfo, newKSInfo);

	return !cantSpell;
}


/* -------------------------------------------------------------- CompareNCNotes -- */
/* Compare the pitches of the given single notes or chords and deliver the number
of literal matches, the number of matches including "accidental-tied-across-barlines"
cases, and two parallel arrays that say which note matched which for both cases.
We determine the literal matches by comparing MIDI note numbers. The second case refers
to the fact that if a note following a barline with no accidental is tied to a note
before the barline with the same lettername that has an (effective) accidental, that
accidental applies to the note after the barline. Notice that this effect can prop-
ogate: if the note after the barline is tied to a following note, it too takes its
pitch from the note before the barline.

Each note is counted only once: if a certain note occurs, say, twice in one chord
and five times in the other, two matches are counted. ??What about unisons? */

Boolean CompareNCNotes(
				LINK firstL, LINK lastL,	/* Syncs */
				short voice,
				short *pnMatch,				/* No. of literal matches */
				short *pnTiedMatch,			/* No. of matches if the notes/chords were tied across a barline */
				CHORDNOTE fChordNote[],		/* for firstL (.noteL==NILINK => ignore) */
				CHORDNOTE lChordNote[],		/* Equivalents in lastL: .noteL's only; .noteNum's are trash */
				short *pflCount 				/* No. of items in fChordNote and lChordNote (>=*pnTiedMatch) */
				)
{
	short f, l, lMatch, nMatch, nTiedMatch, fCount, lCount;
	Boolean sameMeas, matched, tiedAcrossMeas;
	LINK prevMeasL, prevSyncL, prevTieL;
	SearchParam pbSearch;
	Byte fNoteNum;
	CHORDNOTE tlChordNote[MAXCHORD];		/* temporary array for lastL */

	fCount = PSortChordNotes(firstL, voice, TRUE, fChordNote);	/* Ascending by note number */
	lCount = PSortChordNotes(lastL, voice, TRUE, tlChordNote);
	
	nMatch = nTiedMatch = 0;
	sameMeas = SameMeasure(firstL, lastL);
	tiedAcrossMeas = FALSE;
	prevMeasL = LSSearch(firstL, MEASUREtype, ANYONE, GO_LEFT, FALSE);	/* Must exist */
	prevSyncL = LSSearch(prevMeasL, SYNCtype, voice, GO_LEFT, FALSE);
	if (prevSyncL) {
		InitSearchParam(&pbSearch);
		pbSearch.id = ANYONE;												/* Prepare for search */
		pbSearch.voice = voice;
		pbSearch.subtype = TRUE;											/* Tieset, not slur */
		prevTieL = L_Search(prevSyncL, SLURtype, GO_LEFT, &pbSearch);
		if (prevTieL)
			tiedAcrossMeas = (SlurFIRSTSYNC(prevTieL)==prevSyncL);
	}

	for (f = 0; f<fCount; f++) {

		fNoteNum = fChordNote[f].noteNum;
		matched = FALSE;
		
		/* First, try to match the note number exactly. */
		
		for (lMatch = -1, l = 0; l<lCount; l++)
			if (tlChordNote[l].noteNum==fNoteNum) { lMatch = l; break; }
		if (lMatch>=0) {
			nMatch++; nTiedMatch++;
			lChordNote[f].noteL = tlChordNote[lMatch].noteL;
			tlChordNote[lMatch].noteNum = MAX_NOTENUM+1;		/* So we don't match this one again */
			matched = TRUE;
		}
		else if (!sameMeas || tiedAcrossMeas) {
			/* No match of note numbers. But either the notes/chords are in different
				measures, or there's a tie into the measure in this voice, so try for
				an accidental-created match. If <lastL> has a note in the same line/space
				(or its equivalent in another clef!) and that note doesn't have an explicit
				accidental, this note can be tied to it. */
				
			for (lMatch = -1, l = 0; l<lCount; l++)
				if (tlChordNote[l].noteNum<=MAX_NOTENUM) {
					if (NoteYQPIT(fChordNote[f].noteL)==NoteYQPIT(tlChordNote[l].noteL)
					&&  NoteACC(tlChordNote[l].noteL)==0)
					{ lMatch = l; break; }
				}
			if (lMatch>=0) {
				nTiedMatch++;
				lChordNote[f].noteL = tlChordNote[lMatch].noteL;
				tlChordNote[lMatch].noteNum = MAX_NOTENUM+1;		/* So we don't match this one again */
				matched = TRUE;
			}
		}
		if (!matched) fChordNote[f].noteL = NILINK;
	}
	
	*pnMatch = nMatch;
	*pnTiedMatch = nTiedMatch;
	*pflCount = fCount;
	return TRUE;
}


/* ----------------------------------------------------------- CompareNCNoteNums -- */
/* Compare the MIDI note numbers in the given single notes or chords and deliver
the number of matches found. Each note is counted only once: if a note number
occurs (e.g.) twice in one chord and five times in the other, two matches are
counted. */

short CompareNCNoteNums(LINK firstL, LINK lastL, short voice)
{
	CHORDNOTE fChordNote[MAXCHORD], lChordNote[MAXCHORD];
	short fCount, lCount, matches, f, l;
	Byte fNoteNum, lNoteNum;

	fCount = PSortChordNotes(firstL, voice, TRUE, fChordNote);	/* Ascending by note number */
	lCount = PSortChordNotes(lastL, voice, TRUE, lChordNote);
	
	matches = f = l = 0;
	do {
		fNoteNum = NoteNUM(fChordNote[f].noteL);
		lNoteNum = NoteNUM(lChordNote[l].noteL);
		
		if (fNoteNum==lNoteNum) {
			matches++;
			f++;
			l++;
		}
		else if (fNoteNum<=lNoteNum)
			f++;
		else
			l++;
	} while (f<fCount && l<lCount);
	
	return matches;
}


/* ------------------------------------------------------------------ PSortNotes -- */
/* PSortNotes does a Shell (diminishing increment) sort on the given array of notes,
putting it into ascending or descending order on noteNum ("Performance data sort").
Intended for use on chords. The increments used are powers of 2, which is not
optimal, but even an inefficient Shell sort is overkill for sorting typically 3
or 4, at most a few dozen, things. See Knuth, The Art of Computer Programming, vol.
2, pp. 84-95. */

static void PSortNotes(CHORDNOTE chordNote[], short nsize, Boolean ascending)
{
	short			nstep, ncheck;
	short			i, n;
	CHORDNOTE	temp;
	
	for (nstep = nsize/2; nstep>=1; nstep = nstep/2)
	{
	/* Sort <nstep> subarrays by simple insertion */
		for (i = 0; i<nstep; i++)
		{
			for (n = i+nstep; n<nsize; n += nstep) {			/* Look for item <n>'s place */
				temp = chordNote[n];									/* Save it */
				for (ncheck = n-nstep; ncheck>=0;
					  ncheck -= nstep)
					if (ascending ?
							(temp.noteNum<chordNote[ncheck].noteNum)
						:	(temp.noteNum>chordNote[ncheck].noteNum) )
						chordNote[ncheck+nstep] = chordNote[ncheck];
					else {
						chordNote[ncheck+nstep] = temp;
						break;
					}
					chordNote[ncheck+nstep] = temp;
			}
		}
	}
}


/* ------------------------------------------------------------- PSortChordNotes -- */
/* Make a table of all the notes in the voice in the Sync, sort it by note
number, and deliver the number of notes. */

short PSortChordNotes(LINK pSyncL, short voice, Boolean stemDown, CHORDNOTE chordNote[])
{
	short c;
	PANOTE aNote;
	LINK aNoteL;

	c = 0;
	aNoteL = FirstSubLINK(pSyncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->voice==voice) {
			chordNote[c].noteNum = aNote->noteNum;
			chordNote[c].noteL = aNoteL;
			c++;
		}
	}

	if (c>0) PSortNotes(chordNote, c, stemDown);
	
	return c;
}


/* ------------------------------------------------------------------ GSortNotes -- */
/* GSortNotes does a Shell (diminishing increment) sort on the given array of
CHORDNOTEs, putting it into ascending or descending order on yqpit ("Graphically
sorted").  Intended for use on chords, whether of normal notes or grace notes. The
increments we use are powers of 2, which is not optimal, but even an inefficient Shell
sort is overkill for sorting typically 2 or 3, at most a few dozen, things. See
Knuth, The Art of Computer Programming, vol. 2, pp. 84-95. */

static void GSortNotes(CHORDNOTE chordNote[], short nsize, Boolean ascending)
{
	short 		nstep, ncheck;
	short			i, n;
	CHORDNOTE	temp;
	
	for (nstep = nsize/2; nstep>=1; nstep = nstep/2) {
	/* Sort <nstep> subarrays by simple insertion */
		for (i = 0; i<nstep; i++) {
			for (n = i+nstep; n<nsize; n += nstep) {			/* Look for item <n>'s place */
				temp = chordNote[n];									/* Save it */
				for (ncheck = n-nstep; ncheck>=0;
					  ncheck -= nstep)
					if (ascending ?
							(temp.yqpit<chordNote[ncheck].yqpit)
						:	(temp.yqpit>chordNote[ncheck].yqpit) )
						chordNote[ncheck+nstep] = chordNote[ncheck];
					else {
						chordNote[ncheck+nstep] = temp;
						break;
					}
					chordNote[ncheck+nstep] = temp;
			}
		}
	}
}


/* ------------------------------------------------------------- GSortChordNotes -- */
/* Make a table of all the notes in the voice in the Sync, sort it by yqpit
(vertical position relative to middle C), and deliver the number of notes. */

short GSortChordNotes(
				LINK	syncL,
				short	voice,
				Boolean stemDown,		/* TRUE=sort in descending pitch (ASCENDING number) order */
				CHORDNOTE chordNote[]
				)
{
	short c;
	PANOTE aNote;
	LINK aNoteL;

	c = 0;
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->voice==voice) {
			chordNote[c].yqpit = aNote->yqpit;
			chordNote[c].noteL = aNoteL;
			c++;
		}
	}

	if (c>0) GSortNotes(chordNote, c, stemDown);
	
	return c;
}


/* ----------------------------------------------------------- GSortChordGRNotes -- */
/* Make a table of all the grace notes in the voice in the GRSync, sort it by yqpit
(vertical position relative to middle C), and deliver the number of notes. */

short GSortChordGRNotes(
				LINK	grSyncL,
				short	voice,
				Boolean stemDown,			/* TRUE=sort in descending pitch (ASCENDING number) order */
				CHORDNOTE chordGRNote[]
				)
{
	short c;
	PAGRNOTE aGRNote;
	LINK aGRNoteL;

	c = 0;
	aGRNoteL = FirstSubLINK(grSyncL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (aGRNote->voice==voice) {
			chordGRNote[c].yqpit = aGRNote->yqpit;
			chordGRNote[c].noteL = aGRNoteL;
			c++;
		}
	}

	if (c>0) GSortNotes(chordGRNote, c, stemDown);
	
	return c;
}


/* --------------------------------------------------------------- ArrangeNCAccs -- */
/* Move all accidentals on the notes described by chordNote[0..noteCount-1]
to reasonable positions. */

#define QD_SECOND 2

void ArrangeNCAccs(CHORDNOTE chordNote[], short noteCount, Boolean stemDown)
{
	short		i;
	short 	accCount, midAcc, maxStep, accSoFar, diff;
	QDIST		closest, ydiff, prevyqpit;
	PANOTE	aNote;

	if (noteCount<=0) return;
	
	if (noteCount==1) {
		aNote = GetPANOTE(chordNote[0].noteL);
		aNote->xmoveAcc = DFLT_XMOVEACC;
		return;
	}
	
	/*
	 * Count the accidentals in the chord, and find the smallest diatonic interval
	 *	between two notes with accidentals. Then, if the smallest interval is wide
	 *	enough so no overlapping is possible, put all the accidentals in the default
	 *	horizontal position; otherwise, position them with the middle note's
	 *	accidental furthest to the left, and accidentals on notes above and below it
	 *	progressively closer to the notes. If there's an even number of accidentals,
	 *	choose the lower of the two possible middle notes--important if the number is
	 *	just 2.
	 *
	 *	This algorithm could be improved greatly, though it would have to be much
	 * more complex, e.g., taking into account the different heights (both ascent
	 *	and descent) of different accidentals to reset partly or completely. Cf.
	 * Ross, pp. 130-135.
	 */
	for (closest = 9999, accCount = 0, i = 0; i<noteCount; i++) {
		aNote = GetPANOTE(chordNote[i].noteL);
		if (aNote->accident!=0) {
			if (accCount!=0) {
				ydiff = ABS(prevyqpit-aNote->yqpit);
				if (ydiff<closest) closest = ydiff;
			}
			accCount++;
			prevyqpit = aNote->yqpit;
		}
	}
	maxStep = midAcc = accCount/2;
	if (!odd(accCount) && !stemDown) midAcc -= 1;
		
	for (accSoFar = 0, i = 0; i<noteCount; i++) {
		aNote = GetPANOTE(chordNote[i].noteL);
		if (closest<6*QD_SECOND) {						/* Any accidentals less than a 7th apart? */
			diff = maxStep-ABS(midAcc-accSoFar);
			aNote->xmoveAcc = n_min(DFLT_XMOVEACC+config.hAccStep*diff, 31);
		}
		else
			aNote->xmoveAcc = DFLT_XMOVEACC;
		if (aNote->accident!=0) accSoFar++;
	}
}


/* ----------------------------------------------------------- ArrangeChordNotes -- */
/* Put all noteheads on the proper side of the stem--either the normal side or,
because of adjacent seconds, the "wrong" side--and move their accidentals to
reasonable positions. Intended for use, e.g., when a note is added to or removed
from a chord, when a chord is beamed (since that can change stem direction), etc.
N.B. So far, doesn't handle unisons. With unisons, correct arrangement may be
impossible, in which case this function returns FALSE. */

Boolean ArrangeChordNotes(LINK syncL, short voice, Boolean stemDown)
{
	CHORDNOTE	chordNote[MAXCHORD];
	short			i, noteCount;
	QDIST			prevyqpit;
	PANOTE		aNote;
	Boolean		otherStemSide,			/* Put note on "wrong" side of the stem? */
					foundUnison;

	/*
	 *	Get sorted notes and go thru them by y-position, starting with the extreme
	 *	(furthest from the end of the stem) note, which always belongs on the normal
	 *	side.  Then, as long as we encounter seconds, we alternate.  When we find a gap
	 *	of a third or larger, reset to the normal side.
	 */
	noteCount = GSortChordNotes(syncL, voice, stemDown, chordNote);
	
	prevyqpit = 9999;
	foundUnison = FALSE;
	for (otherStemSide = FALSE, i = 0; i<noteCount; i++) {
		aNote = GetPANOTE(chordNote[i].noteL);
		if (ABS(aNote->yqpit-prevyqpit)==0) foundUnison = TRUE;
		if (ABS(aNote->yqpit-prevyqpit)==QD_SECOND)
			otherStemSide = !otherStemSide;
		else
			otherStemSide = FALSE;
		aNote->otherStemSide = otherStemSide;
		prevyqpit = aNote->yqpit;
	}
	
	ArrangeNCAccs(chordNote, noteCount, stemDown);

	return foundUnison;
}


/* ------------------------------------------------------------- ArrangeGRNCAccs -- */
/* Move all accidentals on the grace notes described by chordNote[0..noteCount-1]
to reasonable positions. */

void ArrangeGRNCAccs(CHORDNOTE chordNote[], short noteCount, Boolean stemDown)
{
	short		i;
	short 	accCount, midAcc, maxStep, accSoFar, diff;
	QDIST		closest, ydiff, prevyqpit;
	PAGRNOTE	aGRNote;

	if (noteCount<=0) return;
	
	if (noteCount==1) {
		aGRNote = GetPAGRNOTE(chordNote[0].noteL);
		aGRNote->xmoveAcc = DFLT_XMOVEACC;
		return;
	}
	
	/*
	 *	This algorithm is identical to ArrangeNCAccs, q.v.
	 */
	for (closest = 9999, accCount = 0, i = 0; i<noteCount; i++) {
		aGRNote = GetPAGRNOTE(chordNote[i].noteL);
		if (aGRNote->accident!=0) {
			if (accCount!=0) {
				ydiff = ABS(prevyqpit-aGRNote->yqpit);
				if (ydiff<closest) closest = ydiff;
			}
			accCount++;
			prevyqpit = aGRNote->yqpit;
		}
	}
	maxStep = midAcc = accCount/2;
	if (!odd(accCount) && !stemDown) midAcc -= 1;
		
	for (accSoFar = 0, i = 0; i<noteCount; i++) {
		aGRNote = GetPAGRNOTE(chordNote[i].noteL);
		if (closest<6*QD_SECOND) {						/* Any accidentals less than a 7th apart? */
			diff = maxStep-ABS(midAcc-accSoFar);
			aGRNote->xmoveAcc = n_min(DFLT_XMOVEACC+config.hAccStep*diff, 31);
		}
		else
			aGRNote->xmoveAcc = DFLT_XMOVEACC;
		if (aGRNote->accident!=0) accSoFar++;
	}
}


/* --------------------------------------------------------- ArrangeChordGRNotes -- */
/* Put all grace noteheads on the proper side of the stem--either the normal side or,
because of adjacent seconds, the "wrong" side--and move their accidentals to
reasonable positions. Intended for use, e.g., when a note is added to or removed
from a chord, when a chord is beamed (since that can change stem direction), etc.
N.B. So far, doesn't handle unisons. With unisons, correct arrangement may be
impossible, in which case this function returns FALSE. */

Boolean ArrangeChordGRNotes(LINK grSyncL, short	voice, Boolean stemDown)
{
	CHORDNOTE	chordGRNote[MAXCHORD];
	short			i, noteCount;
	QDIST			prevyqpit;
	PAGRNOTE		aGRNote;
	Boolean		otherStemSide,			/* Put note on "wrong" side of the stem? */
					foundUnison;

	/*
	 *	Get sorted notes and go thru them by y-position, starting with the extreme
	 *	(furthest from the end of the stem) note, which always belongs on the normal
	 *	side.  Then, as long as we encounter seconds, we alternate.  When we find a gap
	 *	of a third or larger, reset to the normal side.
	 */
	noteCount = GSortChordGRNotes(grSyncL, voice, stemDown, chordGRNote);
	
	prevyqpit = 9999;
	foundUnison = FALSE;
	for (otherStemSide = FALSE, i = 0; i<noteCount; i++) {
		aGRNote = GetPAGRNOTE(chordGRNote[i].noteL);
		if (ABS(aGRNote->yqpit-prevyqpit)==0) foundUnison = TRUE;
		if (ABS(aGRNote->yqpit-prevyqpit)==QD_SECOND)
			otherStemSide = !otherStemSide;
		else
			otherStemSide = FALSE;
		aGRNote->otherStemSide = otherStemSide;
		prevyqpit = aGRNote->yqpit;
	}
	
	ArrangeGRNCAccs(chordGRNote, noteCount, stemDown);

	return foundUnison;
}


/* --------------------------------------------------------------- FixAccidental -- */
/* To keep following notes' pitches the same after changing the accidental on a
note (or moving it to another staff, deleting it, etc.), add an accidental on the
next occurence of a note or grace note on the same half-line and staff if (1) that
note or grace note has no accidental and (2) it's in the same measure, or it's in
the first Sync on the staff in the next measure and is tied back across the barline.
If there's no such following note/grace note, do nothing. */

void FixAccidental(
				Document *doc,
				LINK		fromL,
				short		staff,
				short		fixPos,			/* No. of half-lines below middle C */
				short		accKeep 			/* Accidental to keep in effect on following note */
				)
{
	LINK		pL, fixLastL, aNoteL, aGRNoteL, nextSyncL;
	PANOTE	aNote;
	PAGRNOTE	aGRNote;
	
	if (accKeep==0) return;
	if (fromL==doc->tailL) return;
	
	fixLastL = LSSearch(fromL, MEASUREtype, staff, GO_RIGHT, FALSE);
	if (fixLastL==NILINK) fixLastL = doc->tailL;
	nextSyncL =  LSSearch(fixLastL, SYNCtype, staff, GO_RIGHT, FALSE);
	if (nextSyncL)
		for (aNoteL = FirstSubLINK(nextSyncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==staff && NoteTIEDL(aNoteL)) {
				fixLastL = RightLINK(nextSyncL);
				break;
			}
	
	for (pL = fromL; pL!=fixLastL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->staffn==staff && qd2halfLn(aNote->yqpit)==fixPos) {															/* Yes */
						if (aNote->accident==0) {
							aNote->accident = accKeep;
							aNote->accSoft = TRUE;
						}
						return;
					}
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
					aGRNote = GetPAGRNOTE(aGRNoteL);
					if (aGRNote->staffn==staff && qd2halfLn(aGRNote->yqpit)==fixPos) {															/* Yes */
						if (aGRNote->accident==0) {
							aGRNote->accident = accKeep;
							aGRNote->accSoft = TRUE;
						}
						return;
					}
				}
				break;
			default:
				;
		}		
	}
}


/* ================================================================================= */

/* The following routines deal with accidental tables and pitch modifier tables.
The two are similar in that each describes the pitch-notation situation at a given
point in the object list resulting from the current key signature and/or any
accidentals preceding in the measure, by giving the value in effect for every possible
line or space. The difference is that an accidental table literally gives the
accidental affecting each line or space, 0 if there is none; a pitch modifier
table gives the EFFECTIVE accidental for every line and space, so it doesn't
distinguish between "no accidental" and "natural": it has AC_NATURAL for both. */

/* ----------------------------------------------------------- FixAllAccidentals -- */
/* Correct accidentals of notes and grace notes in given range for given staff to
reflect the pitch modifier or accidental situation at the beginning of the range
described in global <accTable> (which it destroys! ??this is ugly and should be
changed; in fact <accTable> should be an argument instead of a global).
N.B. FixAllAccidentals does not understand barlines within its range. */

void FixAllAccidentals(
				LINK fixFirstL,
				LINK fixLastL,
				short staff,
				Boolean pitchMod		/* TRUE=<accTable> has pitch modifers, else accidentals */
				)
{
	PANOTE	aNote;
	PAGRNOTE aGRNote;
	LINK		pL, aNoteL, aGRNoteL;
	short		halfLn;

	for (pL = fixFirstL; pL!=fixLastL; pL=RightLINK(pL))
		switch(ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)==staff) {																/* Yes. */
						aNote = GetPANOTE(aNoteL);
						halfLn = qd2halfLn(aNote->yqpit)+ACCTABLE_OFF;
						if (aNote->accident!=0)								/* Does it have an accidental? */
							accTable[halfLn] = -1;							/* Yes. Mark this line/space OK */
						else {													/* No, has no accidental */
							if (accTable[halfLn]>0 && 
									(!pitchMod || accTable[halfLn]!=AC_NATURAL)) {
								aNote->accident = accTable[halfLn];
								aNote->accSoft = TRUE;
							}
							accTable[halfLn] = -1;							/* Mark this line/space OK */
						}
					}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
					if (GRNoteSTAFF(aGRNoteL)==staff) {																/* Yes. */
						aGRNote = GetPAGRNOTE(aGRNoteL);
						halfLn = qd2halfLn(aGRNote->yqpit)+ACCTABLE_OFF;
						if (aGRNote->accident!=0)							/* Does it have an accidental? */
							accTable[halfLn] = -1;							/* Yes. Mark this line/space OK */
						else {													/* No, has no accidental */
							if (accTable[halfLn]>0 && 
									(!pitchMod || accTable[halfLn]!=AC_NATURAL)) {
								aGRNote->accident = accTable[halfLn];
								aGRNote->accSoft = TRUE;
							}
							accTable[halfLn] = -1;							/* Mark this line/space OK */
						}
					}
				break;
			default:
				;
		}
}


/* ------------------------------------------------------------- KeySig2AccTable -- */
/* Initialize an accidental table from the given key signature. */

void KeySig2AccTable(
				SignedByte table[],			/* Table to fill in */
				PKSINFO pKSInfo				/* Key signature */
				)
{
	short	j, k, basepos, octave, acc, letOffset, nKSItems;

	letOffset = 3;													/* Key sig. code for C ??CF. SetUpKeySig */
	nKSItems = pKSInfo->nKSItems;
	for (j = 0; j<MAX_STAFFPOS; j++)							/* Clear the table */
		table[j] = 0;
	for (k = 0; k<nKSItems; k++)								/* Adjust it for key signature */
	{
		acc = (pKSInfo->KSItem[k].sharp? AC_SHARP : AC_FLAT);
		basepos = pKSInfo->KSItem[k].letcode-letOffset+ACCTABLE_OFF;
		basepos = (basepos+7) % 7;								/* N.B. Careful of mod of negative stupidity */
		for (octave = 0; 7*octave+basepos<MAX_STAFFPOS; octave++)
			table[7*octave+basepos] = acc;
	}
}


/* ----------------------------------------------------------------- GetAccTable -- */
/* Fill in an accidental table for the given staff just BEFORE the given object. NB:
does not consider effects of accidentals on notes in previous measures that are
tied ino this measure! ??This could well be considered a bug, but before fixing it,
consider possibility that handling it here might hurt performance noticably; also,
some places calling this (e.g., ProcessNRGR in NotelistSave.c) are already handling
this. */

void GetAccTable(
				Document *doc,
				SignedByte table[],			/* Table to fill in */
				LINK target,					/* Object to get accidentals for */
				short staff)
{
	CONTEXT	context;
	LINK		fromL, thisL, aNoteL, aGRNoteL;
	PANOTE	aNote;
	PAGRNOTE aGRNote;
	short		halfLnPos;

	GetContext(doc, LeftLINK(target), staff, &context);
	KeySig2AccTable(table, (PKSINFO)context.KSItem);				/* Init. table from key sig. */
	fromL = LSSearch(LeftLINK(target), MEASUREtype, staff,		/* Start at previous barline */
							GO_LEFT, FALSE);
	if (!fromL) fromL = doc->headL;

	for (thisL = fromL; thisL!=target; thisL = RightLINK(thisL))
	{
		switch (ObjLType(thisL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(thisL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)==staff && !NoteREST(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						halfLnPos = qd2halfLn(aNote->yqpit);
						if (aNote->accident!=0)							/* Does note have an acc.? */
							table[halfLnPos+ACCTABLE_OFF] = aNote->accident;
					}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(thisL);
				for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
					if (GRNoteSTAFF(aGRNoteL)==staff) {
						aGRNote = GetPAGRNOTE(aGRNoteL);
						halfLnPos = qd2halfLn(aGRNote->yqpit);
						if (aGRNote->accident!=0)						/* Does grace note have an acc.? */
							table[halfLnPos+ACCTABLE_OFF] = aGRNote->accident;
					}
				break;
			default:
				;
		}
	}
}


/* --------------------------------------------------------------- GetPitchTable -- */
/* Fill in a table of pitch modifiers in effect for the given staff just BEFORE the
given object. */

void GetPitchTable(
				Document *doc,
				SignedByte table[],			/* Table to fill in */
				LINK target,
				short staff)
{
	short j;

	GetAccTable(doc, table, target, staff);
	for (j = 0; j<MAX_STAFFPOS; j++)
		if (table[j]==0) table[j] = AC_NATURAL;						/* Replace no acc. with natural */
}


/* -------------------------------------------------------------- DelNoteFixAccs -- */
/* For a note or grace note about to be deleted in <pL> on <staffn>, propogate the
effect of its accidental, i.e., if necessary, compensate for it by adding its
accidental to the next note/grace note in the same line/space. N.B. This should NOT
be used to update accidentals upon deleting an arbitrary selection, which can
contain key signatures and barlines; for that, use FixDelAccidentals. */

void DelNoteFixAccs(
				Document *doc,
				LINK pL,
				SignedByte staffn,
				short fixPos,			/* No. of half-lines below middle C */
				short accident)
{	
	GetPitchTable(doc, accTable, pL, staffn);					/* Get pitch modif. situation */
	if (accident!=0 &&												/* Is note's acc. a change from prev.? */
		 accident!=accTable[fixPos+ACCTABLE_OFF])
		FixAccidental(doc, RightLINK(pL), staffn, fixPos, accident); /* Correct acc. of next note on halfline */
}


/* -------------------------------------------------------------- InsNoteFixAccs -- */
/* For a new note in <pL> on <staffn>, propogate the effect of its accidental,
i.e., if necessary, compensate for it by adding the correct accidental to the
next note in the same line/space; and return the effective accidental for the new
note (it may have no accidental itself but is affected by a previous accidental
on its line/space). */

short InsNoteFixAccs(
				Document *doc,
				LINK pL,
				SignedByte staffn,
				short fixPos,			/* No. of half-lines below middle C */
				short accident)
{
	short	accKeep;
	
	GetPitchTable(doc, accTable, pL, staffn);					/* Get pitch modif. situation */
	if (accident!=0 &&												/* Is note's acc. a change from prev.? */
		 accident!=accTable[fixPos+ACCTABLE_OFF])
	{																		/* Yes */
			accKeep = accTable[fixPos+ACCTABLE_OFF];
	}
	else
	{																		/* No, no change */
		accKeep = 0;
		if (fixPos+ACCTABLE_OFF<0 || fixPos+ACCTABLE_OFF>=MAX_STAFFPOS)
			MayErrMsg("InsNoteFixAccs: at %ld, fixPos=%ld needs accTable[%ld]",
						(long)pL, (long)fixPos, (long)(fixPos+ACCTABLE_OFF));
		else
			accident = accTable[fixPos+ACCTABLE_OFF];
	}
	FixAccidental(doc, RightLINK(pL), staffn, fixPos, accKeep); /* Correct acc. of next note on halfline */
	return accident;
}
