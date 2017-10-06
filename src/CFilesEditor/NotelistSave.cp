/* NotelistSave.c for Nightingale - write Notelist file */

/* Besides doing something, useful, this file is intended as a model for user-
written functions. Hence, (1) the overly-generic name, and (2) the exceptionally
detailed comments that refer to what "you" might do.

NB: The compiled-in English words are not an internationalization problem, since
the Notelist format should not change with locale. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "Notelist.h"

static short fRefNum;					/* ID of currently open file */
static OSErr errCode;					/* Latest report from the front */
static Boolean firstKeySig, firstClef;

OSErr WriteLine(void);

static Boolean ProcessNRGR(Document *, LINK, LINK, Boolean);
static Boolean ProcessMeasure(Document *, LINK, LINK, short);
static Boolean ProcessClef(Document *, LINK, LINK);
static Boolean ProcessKeySig(Document *, LINK, LINK);
static Boolean ProcessTimeSig(Document *, LINK, LINK);
static Boolean ProcessDynamic(Document *, LINK);
static Boolean ProcessGraphic(Document *, LINK);
static Boolean ProcessTempo(Document *, LINK);
static Boolean ProcessTuplet(Document *, LINK);
static Boolean ProcessBeamset(Document *, LINK);
static Boolean WriteScoreHeader(Document *doc, LINK);
static unsigned short ProcessScore(Document *, short, Boolean, Boolean);

/* Formats of lines in a Notelist file:

Note			N t=%ld v=%d npt=%d stf=%d dur=%d dots=%d nn=%d acc=%d eAcc=%d pDur=%d vel=%d %s appear=%d
Rest			R t=%ld v=%d npt=%d stf=%d dur=%d dots=%d %s appear=%d
Grace note		G t=-1 v=%d npt=%d stf=%d dur=%d dots=%d nn=%d acc=%d eAcc=%d pDur=%d vel=%d %c appear=%d
Barline (Measure)	/ t=%ld type=%d number=%d
Clef			C stf=%d type=%d
Key signature	K stf=%d KS=%d %c
Time signature	T stf=%d num=%d denom=%d
Dynamic			D stf=%d dType=%d
Text			A v=%d npt=%d stf=%d %c%c '%s'
Tempo mark		M stf=%d '%s' %c=%s
Tuplet			P v=%d npt=%d num=%d denom=%d appear=%d
Beam			B v=%d npt=%d count=%d
Comment			% (anything)

The only forms of text included now are GRStrings and GRLyrics.
NB: the current (5.8b3) version of Open Notelist ignores Beam lines.

CER-12.02.2002: fgets called by ReadLine [FileInput.c] reads to a \n. The sprintf
here was writing to a \r. The MSL handles platform conversion of special chars
(in this case, line-ending chars) but the read & write functions have to read
what they write and write what they are going to read. Thus, changing
sprintf to write a \n here. */

/* ------------------------------------------------------------------------- WriteLine -- */

OSErr WriteLine()
{
	long count;
	
	sprintf(&strBuf[strlen(strBuf)], "\n");
	count = strlen(strBuf);
	errCode = FSWrite(fRefNum, &count, strBuf);
	if (errCode!=noErr)
		ReportIOError(errCode, SAVENOTELIST_ALRT);
	return errCode;
}

/* ----------------------------------------------------------------------- ProcessNRGR -- */
/* Process a note, rest, or grace note. This version of ProcessNRGR simply writes
out the more important (for purposes of a Musicologist's Database or a composer's
synthesis program) fields. Returns True normally, False if there's a problem.

If the <MainNote> macro returns False, the given note is a "subordinate" note of
a chord, i.e., is not the note that has the stem or (for whole-note chords, etc.)
that would have the stem if there was one. It always returns True for notes that
aren't in chords and for rests, which Nightingale doesn't allow in chords. */

#define MAX_MODNRS 50		/* Max. modifiers per note/rest we can handle */

static Boolean ProcessNRGR(
				Document *doc,
				LINK syncL, LINK aNoteL,	/* Sync or grace sync, note/rest or grace note */
				Boolean modVals 			/* Write <data> values of modifiers? */
				)
{
	char rCode, mCode; PANOTE aNote; PAGRNOTE aGRNote;
	short userVoice, np, i, effAcc;
	LINK thePartL, aModNRL; PAMODNR aModNR;
	LINK firstSyncL, bNoteL;
	
	/* Handle notes and rests */
	
	if (SyncTYPE(syncL)) {
		rCode = (NoteREST(aNoteL)? REST_CHAR : NOTE_CHAR);
		if (NoteINCHORD(aNoteL)) mCode = (MainNote(aNoteL)? '+' : '-');
		else							 mCode = '.';
		aNote = GetPANOTE(aNoteL);
		if (!Int2UserVoice(doc, aNote->voice, &userVoice, &thePartL))
			return False;
		np = PartL2Partn(doc, thePartL);

		aNote = GetPANOTE(aNoteL);
		sprintf(strBuf, "%c t=%ld v=%d npt=%d stf=%d dur=%d dots=%d ",
				rCode, SyncAbsTime(syncL), userVoice, np, aNote->staffn,
				aNote->subType, aNote->ndots);
		if (!NoteREST(aNoteL)) {										/* These apply to notes only */
			if (!FirstTiedNote(syncL, aNoteL, &firstSyncL, &bNoteL)) return False;
			effAcc = EffectiveAcc(doc, firstSyncL, bNoteL);
			aNote = GetPANOTE(aNoteL);
			sprintf(&strBuf[strlen(strBuf)], "nn=%d acc=%d eAcc=%d pDur=%d vel=%d ",
				aNote->noteNum, aNote->accident, effAcc,
				aNote->playDur, aNote->onVelocity);
		}
		
		aNote = GetPANOTE(aNoteL);										/* Back to notes AND rests */
		sprintf(&strBuf[strlen(strBuf)], "%c%c%c%c%c%c",
				mCode,
				(aNote->tiedL? ')' : '.'),
				(aNote->tiedR? '(' : '.'),
				(aNote->slurredL? '>' : '.'),
				(aNote->slurredR? '<' : '.'),
				(aNote->inTuplet? 'T' : '.') );

		aNote = GetPANOTE(aNoteL);
		/*
		 * To reduce the size of the Notelist file, we used to skip writing headShape if
		 * it was NORMAL_VIS. But that makes the file harder to parse, so we now write it
		 * in all cases.
		 */
		sprintf(&strBuf[strlen(strBuf)], " appear=%d", aNote->headShape);

		aModNRL = aNote->firstMod;
		if (aModNRL) {
			sprintf(&strBuf[strlen(strBuf)], " mods=");
			for (i = 0; aModNRL && i<MAX_MODNRS; aModNRL = NextMODNRL(aModNRL), i++) {
				aModNR = GetPAMODNR(aModNRL);
				sprintf(&strBuf[strlen(strBuf)], "%d", aModNR->modCode);
				aModNR = GetPAMODNR(aModNRL);
				if (modVals && aModNR->data!=0)
					sprintf(&strBuf[strlen(strBuf)], ":%d", aModNR->data);
				if (NextMODNRL(aModNRL) && i+1<MAX_MODNRS)
					sprintf(&strBuf[strlen(strBuf)], ",");
				
			}
		}
	}

	/* Handle grace notes */
	
	else if (GRSyncTYPE(syncL)) {
		if (GRNoteINCHORD(aNoteL)) mCode = (GRMainNote(aNoteL)? '+' : '-');
		else								mCode = '.';
		aGRNote = GetPAGRNOTE(aNoteL);
		if (!Int2UserVoice(doc, aGRNote->voice, &userVoice, &thePartL))
			return False;
		np = PartL2Partn(doc, thePartL);

		aGRNote = GetPAGRNOTE(aNoteL);
		sprintf(strBuf, "%c t=-1 v=%d npt=%d stf=%d dur=%d dots=%d", GRACE_CHAR,
				userVoice, np, aGRNote->staffn, aGRNote->subType, aGRNote->ndots);
		effAcc = EffectiveGRAcc(doc, syncL, aNoteL);
		aGRNote = GetPAGRNOTE(aNoteL);
		sprintf(&strBuf[strlen(strBuf)], " nn=%d acc=%d eAcc=%d pDur=%d vel=%d %c",
			aGRNote->noteNum, aGRNote->accident, effAcc,
			aGRNote->playDur, aGRNote->onVelocity, mCode);

		aGRNote = GetPAGRNOTE(aNoteL);
		sprintf(&strBuf[strlen(strBuf)], " appear=%d", aGRNote->headShape);
	}

	return (WriteLine()==noErr);
}


/* -------------------------------------------------------------------- ProcessMeasure -- */
/* Process a Measure object and subobject. This version also writes out information
about the appearance of the given subobject, which may not apply to the other
subobjects, i.e., barlines on other staves; but this is pretty minor, and barlines
on different staves aren't independent in any other way. Hence this does not write
out the staff number, and it should be called only once for a given Measure object!
Returns True normally, False if there's a problem. */

static Boolean ProcessMeasure(Document *doc, LINK measL, LINK aMeasL, short useSubType)
{
	short theSubType, measureNum;

	theSubType = MeasSUBTYPE(aMeasL);
	if (useSubType>=0) theSubType = useSubType;
	measureNum = MeasMEASURENUM(aMeasL)+doc->firstMNNumber;
	sprintf(strBuf, "%c t=%ld type=%d number=%d", BAR_CHAR, MeasureTIME(measL), theSubType,
				measureNum);

	return (WriteLine()==noErr);
}


/* Clefs, key signatures, and time signatures all have the feature that they can appear
redundantly at system breaks. The current clef and (if there is one) key signature are
repeated at the beginning of every system just to set the context; if a system starts
with a change of time signature, the same time signature should appear at the end of the
previous system to alert a player of the coming change. We don't want these redundant
symbols to appear in the notelist. Thanks to Tim Crawford for pointing out the problem
and the simple solution (specifically for key signatures). */


#define NL_KEEP_CONTEXT_CLEFS False

/* ----------------------------------------------------------------------- ProcessClef -- */
/* Process a Clef subobject. This version skips it if it's just a context-setting clef
at the beginning of a system (other than the first, of course); otherwise it simply
writes it out. Returns True normally, False if there's a problem. */

static Boolean ProcessClef(Document */*doc*/, LINK tsL, LINK aClefL)
{
	PACLEF aClef;
	
	if (!firstClef && !ClefINMEAS(tsL) && !NL_KEEP_CONTEXT_CLEFS) return True;
	aClef = GetPACLEF(aClefL);
	sprintf(strBuf, "%c stf=%d type=%d", CLEF_CHAR, aClef->staffn, aClef->subType);

	return (WriteLine()==noErr);
}


/* --------------------------------------------------------------------- ProcessKeySig -- */
/* Process a KeySig subobject. This version skips it if it's just a context-setting
key signature at the beginning of a system (other than the first, of course); otherwise
it simply writes out some of its info. NB: assumes standard key signature (non-standard
ones aren't implemented yet, anyway, as of v.5.7). Returns True normally, False if
there's a problem. */

static Boolean ProcessKeySig(Document */*doc*/, LINK ksL, LINK aKeySigL)
{
	PAKEYSIG aKeySig;
	
	if (!firstKeySig && !KeySigINMEAS(ksL)) {
		return True;
	}
	aKeySig = GetPAKEYSIG(aKeySigL);
	sprintf(strBuf, "%c stf=%d KS=%d %c", KEYSIG_CHAR, aKeySig->staffn,
					ABS(aKeySig->nKSItems), (aKeySig->KSItem[0].sharp? '#' : 'b'));

	return (WriteLine()==noErr);
}


/* -------------------------------------------------------------------- ProcessTimeSig -- */
/* Process a TimeSig subobject. This version skips it if it's at the very end of a
system, since in that case it's presumably just a cautionary TimeSig anticipating
the change at the beginning of the next system; otherwise it simply writes it out.
Returns True normally, False if there's a problem. */

static Boolean ProcessTimeSig(Document */*doc*/, LINK tsL, LINK aTimeSigL)
{
	PATIMESIG aTimeSig;
	
	if (IsLastInSystem(tsL)) return True;
	
	aTimeSig = GetPATIMESIG(aTimeSigL);
	sprintf(strBuf, "%c stf=%d num=%d denom=%d", TIMESIG_CHAR, aTimeSig->staffn,
				aTimeSig->numerator, aTimeSig->denominator);
	aTimeSig = GetPATIMESIG(aTimeSigL);
	if (aTimeSig->subType!=N_OVER_D)
		sprintf(&strBuf[strlen(strBuf)], " displ=%d", aTimeSig->subType);

	return (WriteLine()==noErr);
}


/* -------------------------------------------------------------------- ProcessDynamic -- */
/* Process a Dynamic object. This version simply writes it out.
Returns True normally, False if there's a problem. */

static Boolean ProcessDynamic(Document */*doc*/, LINK dynamL)
{
	LINK aDynamicL; PADYNAMIC aDynamic;
	
	/* Dynamics are unique: they have exactly one subobject. */
	
	aDynamicL = FirstSubLINK(dynamL);
	aDynamic = GetPADYNAMIC(aDynamicL);

	sprintf(strBuf, "%c stf=%d dType=%d", DYNAMIC_CHAR, aDynamic->staffn, DynamType(dynamL));

	return (WriteLine()==noErr);
}


/* -------------------------------------------------------------------- ProcessGraphic -- */
/* Process a Graphic object. This version simply writes out GRStrings and GRLyrics,
and ignores the other subtypes. Returns True normally, False if there's a problem. */

static Boolean ProcessGraphic(Document *doc, LINK graphicL)
{
	char typeCode, styleCode; short userVoice, np; LINK thePartL, aGraphicL;
	PGRAPHIC pGraphic; PAGRAPHIC aGraphic;
	StringOffset theStrOffset; StringPtr pStr; char str[256];
	
	if (GraphicSubType(graphicL)!=GRString && GraphicSubType(graphicL)!=GRLyric)
		return True;

	typeCode = (GraphicSubType(graphicL)==GRString? 'S' : 'L');
	
	styleCode = '0';
	pGraphic = GetPGRAPHIC(graphicL);
	switch (pGraphic->info) {
		case FONT_R1: styleCode = '1'; break;
		case FONT_R2: styleCode = '2'; break;
		case FONT_R3: styleCode = '3'; break;
		case FONT_R4: styleCode = '4'; break;
		case FONT_R5: styleCode = '5'; break;
		case FONT_R6: styleCode = '6'; break;
		case FONT_R7: styleCode = '7'; break;
		case FONT_R8: styleCode = '8'; break;
		case FONT_R9: styleCode = '9'; break;
		default: ;
	}
	
	if (GraphicVOICE(graphicL)>0) {
		if (!Int2UserVoice(doc, GraphicVOICE(graphicL), &userVoice, &thePartL))
				return False;
		np = PartL2Partn(doc, thePartL);
	}
	else {
		userVoice = NOONE;
		np = NOONE;
	}
	aGraphicL = FirstSubLINK(graphicL);
	aGraphic = GetPAGRAPHIC(aGraphicL);
	theStrOffset = aGraphic->strOffset;
	pStr = PCopy(theStrOffset);						/* FIXME: following would be simpler with CCopy */
	if (!pStr) return False;						/* should never happen */
	
	Pstrcpy((StringPtr)str, pStr);
	PToCString((StringPtr)str);
	sprintf(strBuf, "%c v=%d npt=%d stf=%d %c%c '%s'", GRAPHIC_CHAR, userVoice, np,
			GraphicSTAFF(graphicL), typeCode, styleCode, str);

	return (WriteLine()==noErr);
}


extern char gTempoCode[];

/* ---------------------------------------------------------------------- ProcessTempo -- */
/* Process a Tempo object, with its tempo and metronome mark components. This version
simply writes it out. Returns True normally, False if there's a problem. */

static Boolean ProcessTempo(Document *doc, LINK tempoL)
{
	PTEMPO p; char noteChar;
	char tempoStr[255];
	
PushLock(OBJheap);
 	p = GetPTEMPO(tempoL);
	/* Avoid writing out the Tempo if there's no text string and MM is hidden. */
	if (!p->strOffset && p->hideMM && !doc->showInvis) {
		PopLock(OBJheap);
		return True;
	}

	/* The tempo mark string may contain embedded newline chars.; replace them with
		a delimiter char. to keep everything on one line. */ 
	Pstrcpy((StringPtr)tempoStr, (StringPtr)PCopy(p->strOffset));
	PtoCstr((StringPtr)tempoStr);
	for (unsigned short k=1; k<=strlen(tempoStr); k++)
		if (tempoStr[k]==CH_CR) tempoStr[k] = CH_NLDELIM;
	sprintf(strBuf, "%c stf=%d '%s'", METRONOME_CHAR, TempoSTAFF(tempoL),
				 tempoStr);

	noteChar = gTempoCode[p->subType];
	if (p->noMM)
		sprintf(&strBuf[strlen(strBuf)], " *=noMM");
	else if (!p->hideMM || doc->showInvis) {
		sprintf(&strBuf[strlen(strBuf)], " %c", noteChar);
		if (p->dotted) sprintf(&strBuf[strlen(strBuf)], ".");
		sprintf(&strBuf[strlen(strBuf)], "=%s", PtoCstr(PCopy(p->metroStrOffset)) );
	}

PopLock(OBJheap);
	return (WriteLine()==noErr);
}


/* --------------------------------------------------------------------- ProcessTuplet -- */
/* Process a Tuplet object. This version simply writes it out.
Returns True normally, False if there's a problem. */

static Boolean ProcessTuplet(Document *doc, LINK tupletL)
{
	PTUPLET pTuplet;
	short userVoice, np;
	LINK thePartL;
	
PushLock(OBJheap);
 	pTuplet = GetPTUPLET(tupletL);

	if (!Int2UserVoice(doc, pTuplet->voice, &userVoice, &thePartL))
			return False;
	np = PartL2Partn(doc, thePartL);
	sprintf(strBuf, "%c v=%d npt=%d num=%d denom=%d appear=%d%d%d", TUPLET_CHAR,
				userVoice, np, pTuplet->accNum, pTuplet->accDenom, pTuplet->numVis,
				pTuplet->denomVis, pTuplet->brackVis);

PopLock(OBJheap);
	return (WriteLine()==noErr);
}


/* -------------------------------------------------------------------- ProcessBeamset -- */
/* Process a Beamset object. This version simply writes it out.
Returns True normally, False if there's a problem. */

static Boolean ProcessBeamset(Document *doc, LINK beamL)
{
	short userVoice, np;
	LINK thePartL;
	
PushLock(OBJheap);

	if (!Int2UserVoice(doc, BeamVOICE(beamL), &userVoice, &thePartL))
		return False;
	np = PartL2Partn(doc, thePartL);
	sprintf(strBuf, "%c v=%d npt=%d count=%d", BEAM_CHAR, userVoice, np,
				LinkNENTRIES(beamL));

PopLock(OBJheap);
	return (WriteLine()==noErr);
}


/* ------------------------------------------------------------------ WriteScoreHeader -- */

#define COMMENT_NLHEADER2	"%%Notelist-V2 file="	/* start of structured comment: Ngale 99 and after */

/* Write out a "structured comment" (a la PostScript) describing the score the notelist
comes from. */

static Boolean WriteScoreHeader(Document *doc, LINK startL)
{
	char filename[256];
	LINK partL, prevMeasL; PPARTINFO pPart;
	short startMeas;

	if (doc->named) {
		Pstrcpy((StringPtr)filename, (StringPtr)doc->name);
		PToCString((StringPtr)filename);
	}
	else
		GetIndCString(filename,MiscStringsID,1);								/* "Untitled" */

	sprintf(strBuf, "%s'%s'  partstaves=", COMMENT_NLHEADER2, filename);
	partL = FirstSubLINK(doc->headL);											/* Skip dummy part */
	for (partL=NextPARTINFOL(partL); partL; partL=NextPARTINFOL(partL)) {
		pPart = GetPPARTINFO(partL);
		sprintf(&strBuf[strlen(strBuf)], "%d ", pPart->lastStaff-pPart->firstStaff+1);
	}
	sprintf(&strBuf[strlen(strBuf)], "0");
	
	/* Get the starting measure no. This is never before the first Measure of the score,
	 * even if the first selected object is before it.
	 */
	prevMeasL = SSearch(startL, MEASUREtype, GO_LEFT);
	if (prevMeasL==NILINK) startL = SSearch(startL, MEASUREtype, GO_RIGHT);
	startMeas = GetMeasNum(doc, startL);
	sprintf(&strBuf[strlen(strBuf)], " startmeas=%d", startMeas);

	return (WriteLine()==noErr);
}

/* ---------------------------------------------------------------------- ProcessScore -- */
/* Traverse the given Document from beginning to end and call ProcessXXX for every
selected object/subobject of certain types in the given voice or in all voices. NB:
<voice> is an internal voice number, which is rarely the same as the voice number we
show to users. Return value is the number of lines in the Notelist file. */

static unsigned short ProcessScore(
						Document *doc,
						short voice,			/* voice number, or ANYONE to include all voices */
						Boolean skeleton,		/* True=skip everything but Measures and Timesigs */
						Boolean rests			/* True=include rests */
						)
{

	LINK pL, aNoteL, aGRNoteL, aKeySigL, aTimeSigL, aMeasL, aClefL, nextSyncL;
	Boolean anyVoice, useNextMeasure, measIsEmpty;
	unsigned short count=0;
	short useSubType;
	
	if (!WriteScoreHeader(doc, doc->selStartL)) goto Error;
	count++;

	anyVoice = (voice==ANYONE);
	firstClef = firstKeySig = True;
	useNextMeasure = False;
	useSubType = -1;

	/* If there are no Syncs after a Measure object in its System, and the next Measure
	exists and is in another System, they're really pieces of the same measure (and their
	<measureNum>s reflect that). In such a case, in terms of order in the notelist, the
	second one is the one we want; but the first has the barline type we want. This
	requires handling with care. */

	for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL)) {
		if (useNextMeasure && ObjLType(pL)==MEASUREtype) {
			/* Write this Measure whether it's selected or not. */
			aMeasL = FirstSubLINK(pL);
			if (!ProcessMeasure(doc, pL, aMeasL, useSubType)) goto Error;
			count++;
			useNextMeasure = False;
			useSubType = -1;
		}
		
		if (LinkSEL(pL)) {			
			switch (ObjLType(pL)) {
				case SYNCtype:
					if (skeleton) break;
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteSEL(aNoteL))
							if (anyVoice || NoteVOICE(aNoteL)==voice)
								if (rests || !NoteREST(aNoteL)) {
									if (!ProcessNRGR(doc, pL, aNoteL, True)) goto Error;
									count++;
								}
					break;
				case GRSYNCtype:
					if (skeleton) break;
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
						if (GRNoteSEL(aGRNoteL))
							if (anyVoice || GRNoteVOICE(aGRNoteL)==voice) {
									if (!ProcessNRGR(doc, pL, aGRNoteL, True)) goto Error;
									count++;
								}
					break;
				case MEASUREtype:
					/* If there are no Syncs after this Measure object in its System, and the
						next Measure exists and is in another System, they're really pieces of
						the same measure -- and in terms of order, the next one is the one we
						want! In that case, ignore this Measure and set the <useNextMeasure>
						flag. */
						
					nextSyncL = LSSearch(pL, SYNCtype, ANYONE, GO_RIGHT, False);
					measIsEmpty = (nextSyncL==NILINK || !SameSystem(pL, nextSyncL));
					if (measIsEmpty && LinkRMEAS(pL)!=NILINK) {
						useNextMeasure = True;
						aMeasL = FirstSubLINK(pL);
						useSubType = MeasSUBTYPE(aMeasL);
						break;
					}
					
					/* Since Nightingale doesn't allow independent barlines on different
						staves, we just report there's a barline for the whole score.
						However, we pass the first selected subobject (to the user, a
						barline on one or more staves) to ProcessMeasure so it can describe
						its appearance: it's very likely the same for all staves. */

					aMeasL = FirstSubLINK(pL);
					for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL))
						if (MeasureSEL(aMeasL)) {
							if (!ProcessMeasure(doc, pL, aMeasL, useSubType)) goto Error;
							count++;
							break;
						}
					break;
				case CLEFtype:
					if (skeleton) break;
					aClefL = FirstSubLINK(pL);
					for ( ; aClefL; aClefL = NextCLEFL(aClefL))
						if (ClefSEL(aClefL)) {
							if (!ProcessClef(doc, pL, aClefL)) goto Error;
							count++;
						}
					break;
				case KEYSIGtype:
					if (skeleton) break;
					aKeySigL = FirstSubLINK(pL);
					for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
						if (KeySigSEL(aKeySigL)) {
							if (!ProcessKeySig(doc, pL, aKeySigL)) goto Error;
							count++;
						}
					break;
				case TIMESIGtype:
					aTimeSigL = FirstSubLINK(pL);
					for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
						if (TimeSigSEL(aTimeSigL)) {
							if (!ProcessTimeSig(doc, pL, aTimeSigL)) goto Error;
							count++;
						}
					break;
				case DYNAMtype:
					if (skeleton) break;
					if (!ProcessDynamic(doc, pL)) goto Error;
					count++;
					break;
				case GRAPHICtype:
					if (skeleton) break;
					if (!ProcessGraphic(doc, pL)) goto Error;
					count++;
					break;
				case TEMPOtype:
					if (!ProcessTempo(doc, pL)) goto Error;
					count++;
					break;
				case TUPLETtype:
					if (skeleton) break;
					if (!ProcessTuplet(doc, pL)) goto Error;
					count++;
					break;
				case BEAMSETtype:
					if (skeleton) break;
					/* Since Open Notelist can't yet handle Beamsets, make writing them optional. */
					if (config.notelistWriteBeams==0) break;
					if (!ProcessBeamset(doc, pL)) goto Error;
					count++;
					break;
				default:
					;
			}
		}
	
		/*
		 * If we've just handled a Keysig, we're past the first Keysig of the score.
		 * Reset <firstKeySig> so that, of the Keysigs that are not in a Measure, no
		 * others are ever output in the Notelist. Likewise for Clefs.
		 */
		if (ObjLType(pL)==CLEFtype) firstClef = False;
		if (ObjLType(pL)==KEYSIGtype) firstKeySig = False;
	}

	return count;	

Error:
	GetIndCString(strBuf, NOTELIST_STRS, 38);	/* "An error occurred: the entire notelist was not saved." */
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
	return count;		
}


#define SKELETON False		/* Skip all but Measure and Timesig objs? For, e.g., comparing
								versions of the same score. */

static Point SFPwhere = { 106, 104 };	/* Where we want SFPutFile dialog */

#ifdef TARGET_API_MAC_CARBON_FILEIO

/* NB: While the <voice> and <rests> parameters are currently ignored, handling them
should simply be a matter of passing them on to ProcessScore. */

void SaveNotelist(
			Document *doc,
			short /*voice*/,		/* (ignored) voice number, or ANYONE to include all voices */
			Boolean /*rests*/	 	/* (ignored) True=include rests */
			)
{
	short sufIndex;
	short len, suffixLen, ch;
	Str255	filename, prompt;
	CFStringRef	nlFileName;
	NSClientData nsData;
	OSStatus anErr=noErr;
		
	/*
	 *	Create a default notelist filename by looking up the suffix string and appending
	 *	it to the current name.  If the current name is so long that there would not
	 *	be room to append the suffix, we truncate the file name before appending the
	 *	suffix so that we don't run the risk of overwriting the original score file.
	 */
	sufIndex = 11;
	GetIndString(filename, MiscStringsID, sufIndex);				/* Get suffix length */
	suffixLen = *(StringPtr)filename;

	/* Get current name and its length, and truncate name to make room for suffix */
	
	if (doc->named) Pstrcpy((StringPtr)filename, (StringPtr)doc->name);
	else				 GetIndString(filename, MiscStringsID, 1);		/* "Untitled" */
	len = *(StringPtr)filename;
	if (len >= (FILENAME_MAXLEN-suffixLen)) len = (FILENAME_MAXLEN-suffixLen);
	
	/* Finally append suffix FIXME: with unreadable low-level code: change to PStrCat! */
	
	ch = filename[len];										/* Hold last character of name */
	GetIndString(filename+len, MiscStringsID, sufIndex);	/* Append suffix, obliterating last char */
	filename[len] = ch;										/* Overwrite length byte with saved char */
	*filename = (len + suffixLen);							/* And ensure new string knows new length */
	
	/* Ask user where to put this notelist file */
	
	GetIndString(prompt, MiscStringsID, 12);
	nlFileName = CFStringCreateWithPascalString(NULL, filename, smRoman);

	anErr = SaveFileDialog( NULL, nlFileName, 'TEXT', creatorType, &nsData );
	
	if (anErr == noErr && !nsData.nsOpCancel) {
		FSSpec fsSpec = nsData.nsFSSpec;
		
		errCode = FSpDelete(&fsSpec);									/* Delete old file */
		if (errCode && errCode!=fnfErr)									/* Ignore "file not found" */
			{ MayErrMsg("SaveNotelist: FSDelete error"); return; }
			
		errCode = FSpCreate(&fsSpec, creatorType, 'TEXT', smRoman);		/* Create new file */
		if (errCode) { MayErrMsg("SaveNotelist: Create error"); return; }

		errCode = FSpOpenDF(&fsSpec, fsRdWrPerm, &fRefNum );			/* Open the temp file */
		if (errCode) { MayErrMsg("SaveNotelist: FSOpen error"); return; }

		WaitCursor();
		ProcessScore(doc, ANYONE, SKELETON, True);
		LogPrintf(LOG_INFO, "Saved notelist file '%s'", PToCString(filename));
		LogPrintf(LOG_INFO, (SKELETON? " as skeleton.\n" : ".\n"));

		errCode = FSClose(fRefNum);
	}
}

#else

/* NB: While the <voice> and <rests> parameters are currently ignored, handling them
should simply be a matter of passing them on to ProcessScore. */

void SaveNotelist(
			Document *doc,
			short voice,		/* (ignored) voice number, or ANYONE to include all voices */
			Boolean rests 		/* (ignored) True=include rests */
			)
{
	short	sufIndex;
	short	len, suffixLen, ch;
	short	vRefNum;
	Str255	filename, prompt;
	Rect	paperRect;
	SFReply	reply;

	/*
	 *	Create a default notelist filename by looking up the suffix string and appending
	 *	it to the current name.  If the current name is so long that there would not
	 *	be room to append the suffix, we truncate the file name before appending the
	 *	suffix so that we don't run the risk of overwriting the original score file.
	 */
	sufIndex = 11;
	GetIndString(filename,MiscStringsID,sufIndex);			/* Get suffix length */
	suffixLen = *(StringPtr)filename;

	/* Get current name and its length, and truncate name to make room for suffix */
	
	if (doc->named)	Pstrcpy((StringPtr)filename, (StringPtr)doc->name);
	else			GetIndString(filename,MiscStringsID,1);		/* "Untitled" */
	len = *(StringPtr)filename;
	if (len >= (FILENAME_MAXLEN-suffixLen)) len = (FILENAME_MAXLEN-suffixLen);
	
	/* Finally append suffix FIXME: with unreadable low-level code: change to PStrCat! */
	
	ch = filename[len];										/* Hold last character of name */
	GetIndString(filename+len, MiscStringsID, sufIndex);	/* Append suffix, obliterating last char */
	filename[len] = ch;										/* Overwrite length byte with saved char */
	*filename = (len + suffixLen);							/* And ensure new string knows new length */
	
	/* Ask user where to put this notelist file */
	
	GetIndString(prompt, MiscStringsID, 12);
	SFPutFile(SFPwhere, prompt, filename, NULL, &reply);
	if (!reply.good) return;
	Pstrcpy((StringPtr)filename, reply.fName);
	vRefNum = reply.vRefNum;
	
	errCode = FSDelete(filename, vRefNum);							/* Delete old file */
	if (errCode && errCode!=fnfErr)									/* Ignore "file not found" */
		{ MayErrMsg("SaveNotelist: FSDelete error"); return; }
		
	errCode = Create(filename, vRefNum, creatorType, 'TEXT'); 		/* Create new file */
	if (errCode) { MayErrMsg("SaveNotelist: Create error"); return; }
	
	errCode = FSOpen(filename, vRefNum, &fRefNum);					/* Open it */
	if (errCode) { MayErrMsg("SaveNotelist: FSOpen error"); return; }

	WaitCursor();
	(void)ProcessScore(doc, ANYONE, SKELETON, True);
	LogPrintf(LOG_INFO, "Saved notelist file '%s'", PToCString(filename));
	LogPrintf(LOG_INFO, (SKELETON? " as skeleton.\n" : ".\n"));

	errCode = FSClose(fRefNum);
}

#endif // TARGET_API_MAC_CARBON_FILEIO
