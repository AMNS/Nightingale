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
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "Notelist.h"

static short fRefNum;					/* ID of currently open file */
static OSErr errCode;					/* Latest report from the front */
static Boolean firstKeySig, firstClef;

OSErr WriteLine(void);

Boolean ProcessNRGR(Document *, LINK, LINK, Boolean);
Boolean ProcessMeasure(Document *, LINK, LINK);
Boolean ProcessClef(Document *, LINK, LINK);
Boolean ProcessKeySig(Document *, LINK, LINK);
Boolean ProcessTimeSig(Document *, LINK, LINK);
Boolean ProcessDynamic(Document *, LINK);
Boolean ProcessGraphic(Document *, LINK);
Boolean ProcessTempo(Document *, LINK);
Boolean ProcessTuplet(Document *, LINK);
Boolean ProcessBeamset(Document *, LINK);
Boolean WriteScoreHeader(Document *doc, LINK);

/* Formats of lines in a Notelist file:

Note			N t=%ld v=%d npt=%d stf=%d dur=%d dots=%d nn=%d acc=%d eAcc=%d pDur=%d vel=%d %s appear=%d
Rest			R t=%ld v=%d npt=%d stf=%d dur=%d dots=%d %s appear=%d
Grace note		G t=-1 v=%d npt=%d stf=%d dur=%d dots=%d nn=%d acc=%d eAcc=%d pDur=%d vel=%d %c appear=%d
Barline			/ t=%ld type=%d
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
NB: the Beam line is not understood by the current (99b4) version of Open Notelist. 

CER-12.02.2002: fgets called by ReadLine [FileInput.c] reads to a \n. The sprintf
here was writing to a \r. The MSL handles platform conversion of special chars
(in this case, line-ending chars) but the read & write functions have to read
what they write and write what they are going to read. Thus, changing
sprintf to write a \n here. */

/* ------------------------------------------------------------------- WriteLine -- */

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

/* ------------------------------------------------------------------ ProcessNRGR -- */
/* Process a note, rest, or grace note. This version of ProcessNRGR simply writes
out the more important (for purposes of a Musicologist's Database or a composer's
synthesis program) fields. Returns TRUE normally, FALSE if there's a problem.

If the <MainNote> macro returns FALSE, the given note is a "subordinate" note of
a chord, i.e., is not the note that has the stem or (for whole-note chords, etc.)
that would have the stem if there was one. It always returns TRUE for notes that
aren't in chords and for rests, which Nightingale doesn't allow in chords. */

#define MAX_MODNRS 50		/* Max. modifiers per note/rest we can handle */

Boolean ProcessNRGR(
				Document *doc,
				LINK syncL, LINK aNoteL,	/* Sync or grace sync, note/rest or grace note */
				Boolean modVals 				/* TRUE=write <data> values of modifiers */
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
			return FALSE;
		np = PartL2Partn(doc, thePartL);

		aNote = GetPANOTE(aNoteL);
		sprintf(strBuf, "%c t=%ld v=%d npt=%d stf=%d dur=%d dots=%d ",
				rCode, SyncAbsTime(syncL), userVoice, np, aNote->staffn,
				aNote->subType, aNote->ndots);
		if (!NoteREST(aNoteL)) {										/* These apply to notes only */
			if (!FirstTiedNote(syncL, aNoteL, &firstSyncL, &bNoteL)) return FALSE;
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
			return FALSE;
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


/* -------------------------------------------------------------- ProcessMeasure -- */
/* Process a Measure object. This version also writes out information about
the appearance of the given subobject, which may not apply to the other
subobjects, i.e., barlines on other staves, but this is pretty minor, and
barlines on different staves aren't independent in any other way. Hence this
does not write out the staff number, and it should be called only once for
a given Measure object! Returns TRUE normally, FALSE if there's a problem. */

Boolean ProcessMeasure(Document */*doc*/, LINK measL, LINK aMeasL)
{
	short theSubType;

	theSubType = MeasSUBTYPE(aMeasL);
	sprintf(strBuf, "%c t=%ld type=%d", BAR_CHAR, MeasureTIME(measL), theSubType);

	return (WriteLine()==noErr);
}


#define NL_KEEP_CONTEXT_CLEFS FALSE

/* ------------------------------------------------------------------- ProcessClef -- */
/* Process a Clef subobject. This version skips it if it's just a context-setting clef
at the beginning of a system (other than the first, of course); otherwise it simply
writes it out. Returns TRUE normally, FALSE if there's a problem. */

Boolean ProcessClef(Document */*doc*/, LINK tsL, LINK aClefL)
{
	PACLEF aClef;
	
	if (!firstClef && !ClefINMEAS(tsL) && !NL_KEEP_CONTEXT_CLEFS) return TRUE;
	aClef = GetPACLEF(aClefL);
	sprintf(strBuf, "%c stf=%d type=%d", CLEF_CHAR, aClef->staffn, aClef->subType);

	return (WriteLine()==noErr);
}


/* ----------------------------------------------------------------- ProcessKeySig -- */
/* Process a KeySig subobject. This version skips it if it's just a context-setting
key signature at the beginning of a system (other than the first, of course); otherwise
it simply writes out some of its info. Thanks to Tim C. for the skipping code. NB:
assumes standard key signature (non-standard ones aren't implemented yet, anyway, as of
v.5.7). Returns TRUE normally, FALSE if there's a problem. */

Boolean ProcessKeySig(Document */*doc*/, LINK ksL, LINK aKeySigL)
{
	PAKEYSIG aKeySig;
	
	/* The problem addressed in the code for <firstKeySig>, by Tim C., is that keysigs
	are otherwise added to the Notelist even when they are simply the default keysigs
	at the beginning of each system. These are not desirable in the Notelist. They are
	easy to recognize since KeySigs have an inMeasure flag, but we still need to keep
	any key signatures selected at the very beginning of the score. */
	
	if (!firstKeySig && !KeySigINMEAS(ksL)) {
		return TRUE;
	}
	aKeySig = GetPAKEYSIG(aKeySigL);
	sprintf(strBuf, "%c stf=%d KS=%d %c", KEYSIG_CHAR, aKeySig->staffn,
					ABS(aKeySig->nKSItems), (aKeySig->KSItem[0].sharp? '#' : 'b'));

	return (WriteLine()==noErr);
}


/* -------------------------------------------------------------- ProcessTimeSig -- */
/* Process a TimeSig subobject. This version simply writes it out.
Returns TRUE normally, FALSE if there's a problem. */

Boolean ProcessTimeSig(Document */*doc*/, LINK /*tsL*/, LINK aTimeSigL)
{
	PATIMESIG aTimeSig;
	
	aTimeSig = GetPATIMESIG(aTimeSigL);
	sprintf(strBuf, "%c stf=%d num=%d denom=%d", TIMESIG_CHAR, aTimeSig->staffn,
				aTimeSig->numerator, aTimeSig->denominator);
	aTimeSig = GetPATIMESIG(aTimeSigL);
	if (aTimeSig->subType!=N_OVER_D)
		sprintf(&strBuf[strlen(strBuf)], " displ=%d", aTimeSig->subType);

	return (WriteLine()==noErr);
}


/* -------------------------------------------------------------- ProcessDynamic -- */
/* Process a Dynamic object. This version simply writes it out.
Returns TRUE normally, FALSE if there's a problem. */

Boolean ProcessDynamic(Document */*doc*/, LINK dynamL)
{
	LINK aDynamicL; PADYNAMIC aDynamic;
	
	/* Dynamics are unique: they have exactly one subobject. */
	
	aDynamicL = FirstSubLINK(dynamL);
	aDynamic = GetPADYNAMIC(aDynamicL);

	sprintf(strBuf, "%c stf=%d dType=%d", DYNAMIC_CHAR, aDynamic->staffn, DynamType(dynamL));

	return (WriteLine()==noErr);
}


/* -------------------------------------------------------------- ProcessGraphic -- */
/* Process a Graphic object. This version simply writes out GRStrings and GRLyrics,
and ignores the other subtypes. Returns TRUE normally, FALSE if there's a problem. */

Boolean ProcessGraphic(Document *doc, LINK graphicL)
{
	char typeCode, styleCode; short userVoice, np; LINK thePartL, aGraphicL;
	PGRAPHIC pGraphic; PAGRAPHIC aGraphic;
	StringOffset theStrOffset; StringPtr pStr; char str[256];
	
	if (GraphicSubType(graphicL)!=GRString && GraphicSubType(graphicL)!=GRLyric)
		return TRUE;

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
				return FALSE;
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
	if (!pStr) return FALSE;						/* should never happen */
	
	Pstrcpy((StringPtr)str, pStr);
	PToCString((StringPtr)str);
	sprintf(strBuf, "%c v=%d npt=%d stf=%d %c%c '%s'", GRAPHIC_CHAR, userVoice, np,
			GraphicSTAFF(graphicL), typeCode, styleCode, str);

	return (WriteLine()==noErr);
}


extern char gTempoCode[];

/* ------------------------------------------------------------------ ProcessTempo -- */
/* Process a Tempo object, with its tempo and metronome mark components. This version
simply writes it out. Returns TRUE normally, FALSE if there's a problem. */

Boolean ProcessTempo(Document *doc, LINK tempoL)
{
	PTEMPO p; char noteChar;
	char tempoStr[255];
	
PushLock(OBJheap);
 	p = GetPTEMPO(tempoL);
	/* Avoid writing out the Tempo if there's no text string and MM is hidden. */
	if (!p->strOffset && p->hideMM && !doc->showInvis) {
		PopLock(OBJheap);
		return TRUE;
	}

	/* The tempo mark string may contain embedded newline chars.; replace them with
		a delimiter char. to keep what we write out on one line. */ 
	Pstrcpy((StringPtr)tempoStr, (StringPtr)PCopy(p->strOffset));
	PtoCstr((StringPtr)tempoStr);
	for (short k=1; k<=strlen(tempoStr); k++)
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


/* -------------------------------------------------------------- ProcessTuplet -- */
/* Process a Tuplet object. This version simply writes it out.
Returns TRUE normally, FALSE if there's a problem. */

Boolean ProcessTuplet(Document *doc, LINK tupletL)
{
	PTUPLET pTuplet;
	short userVoice, np;
	LINK thePartL;
	
PushLock(OBJheap);
 	pTuplet = GetPTUPLET(tupletL);

	if (!Int2UserVoice(doc, pTuplet->voice, &userVoice, &thePartL))
			return FALSE;
	np = PartL2Partn(doc, thePartL);
	sprintf(strBuf, "%c v=%d npt=%d num=%d denom=%d appear=%d%d%d", TUPLET_CHAR,
				userVoice, np, pTuplet->accNum, pTuplet->accDenom, pTuplet->numVis,
				pTuplet->denomVis, pTuplet->brackVis);

PopLock(OBJheap);
	return (WriteLine()==noErr);
}


/* -------------------------------------------------------------- ProcessBeamset -- */
/* Process a Beamset object. This version simply writes it out.
Returns TRUE normally, FALSE if there's a problem. */

Boolean ProcessBeamset(Document *doc, LINK beamL)
{
	short userVoice, np;
	LINK thePartL;
	
PushLock(OBJheap);

	if (!Int2UserVoice(doc, BeamVOICE(beamL), &userVoice, &thePartL))
		return FALSE;
	np = PartL2Partn(doc, thePartL);
	sprintf(strBuf, "%c v=%d npt=%d count=%d", BEAM_CHAR, userVoice, np,
				LinkNENTRIES(beamL));

PopLock(OBJheap);
	return (WriteLine()==noErr);
}


/* ------------------------------------------------------------ WriteScoreHeader -- */

#define COMMENT_NLHEADER2	"%%Notelist-V2 file="	/* start of structured comment: Ngale 99 and after */

/* Write out a "structured comment" (a la PostScript) describing the score the notelist
comes from. */

Boolean WriteScoreHeader(Document *doc, LINK startL)
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

/* ---------------------------------------------------------------- ProcessScore -- */
/* Traverse the given Document from beginning to end and call ProcessXXX for
every selected object/subobject of certain types in the given voice or in all
voices. N.B. <voice> is an INTERNAL voice number, which is rarely the same as
the part-relative voice number Nightingale shows to users! Return value is the
number of lines in the Notelist file. */

unsigned short ProcessScore(
						Document *doc,
						short voice,		/* voice number, or ANYONE to include all voices */
						Boolean rests 		/* TRUE=include rests */
						)
{
	LINK pL, aNoteL, aGRNoteL, aKeySigL, aTimeSigL, aMeasL, aClefL;
	Boolean anyVoice;
	unsigned short count=0;
	
	if (!WriteScoreHeader(doc, doc->selStartL)) goto Error;
	count++;

	anyVoice = (voice==ANYONE);
	firstClef = firstKeySig = TRUE;

	for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL)) {
	
		if (LinkSEL(pL)) {
			switch (ObjLType(pL)) {
				case SYNCtype:
					aNoteL = FirstSubLINK(pL);
					for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
						if (NoteSEL(aNoteL))
							if (anyVoice || NoteVOICE(aNoteL)==voice)
								if (rests || !NoteREST(aNoteL)) {
									if (!ProcessNRGR(doc, pL, aNoteL, TRUE)) goto Error;
									count++;
								}
					break;
				case GRSYNCtype:
					aGRNoteL = FirstSubLINK(pL);
					for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
						if (GRNoteSEL(aGRNoteL))
							if (anyVoice || GRNoteVOICE(aGRNoteL)==voice) {
									if (!ProcessNRGR(doc, pL, aGRNoteL, TRUE)) goto Error;
									count++;
								}
					break;
				case MEASUREtype:
					/* Since Nightingale doesn't allow independent barlines on different
						staves, we just report there's a barline for the whole score.
						However, we pass the first selected subobject (barline on a staff)
						to ProcessMeasure so it can describe its appearance--it's probably,
						though not necessarily, the same for all selected staves.  */

					aMeasL = FirstSubLINK(pL);
					for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL))
						if (MeasureSEL(aMeasL)) {
							if (!ProcessMeasure(doc, pL, aMeasL)) goto Error;
							count++;
							break;
						}
					break;
				case CLEFtype:
					aClefL = FirstSubLINK(pL);
					for ( ; aClefL; aClefL = NextCLEFL(aClefL))
						if (ClefSEL(aClefL)) {
							if (!ProcessClef(doc, pL, aClefL)) goto Error;
							count++;
						}
					break;
				case KEYSIGtype:
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
					if (!ProcessDynamic(doc, pL)) goto Error;
					count++;
					break;
				case GRAPHICtype:
					if (!ProcessGraphic(doc, pL)) goto Error;
					count++;
					break;
				case TEMPOtype:
					if (!ProcessTempo(doc, pL)) goto Error;
					count++;
					break;
				case TUPLETtype:
					if (!ProcessTuplet(doc, pL)) goto Error;
					count++;
					break;
				case BEAMSETtype:
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
		 * If we've just handled a keysig, we're past the first keysig of the score.
		 * Reset <firstKeySig> so that, of the keysigs that are not in a measure, no
		 * others are ever output in the Notelist. Likewise for clefs.
		 */
		if (ObjLType(pL)==CLEFtype) firstClef = FALSE;
		if (ObjLType(pL)==KEYSIGtype) firstKeySig = FALSE;
	}

	return count;	

Error:
	GetIndCString(strBuf, NOTELIST_STRS, 38);	/* "An error occurred: the entire notelist was not saved." */
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
	return count;		
}


static Point SFPwhere = { 106, 104 };	/* Where we want SFPutFile dialog */

#ifdef TARGET_API_MAC_CARBON_FILEIO

/* NB: While the <voice> and <rests> parameters are currently ignored, handling them
should simply be a matter of passing them on to ProcessScore. */

void SaveNotelist(
			Document *doc,
			short /*voice*/,		/* (ignored) voice number, or ANYONE to include all voices */
			Boolean /*rests*/	 	/* (ignored) TRUE=include rests */
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
			
		errCode = FSpCreate (&fsSpec, creatorType, 'TEXT', smRoman);	/* Create new file */
		if (errCode) { MayErrMsg("SaveNotelist: Create error"); return; }

		errCode = FSpOpenDF (&fsSpec, fsRdWrPerm, &fRefNum );			/* Open the temp file */
		if (errCode) { MayErrMsg("SaveNotelist: FSOpen error"); return; }

		WaitCursor();
		ProcessScore(doc, ANYONE, TRUE);

		errCode = FSClose(fRefNum);
	}
}

#else

/* NB: While the <voice> and <rests> parameters are currently ignored, handling them
should simply be a matter of passing them on to ProcessScore. */

void SaveNotelist(
			Document *doc,
			short voice,		/* (ignored) voice number, or ANYONE to include all voices */
			Boolean rests 		/* (ignored) TRUE=include rests */
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
	(void)ProcessScore(doc, ANYONE, TRUE);

	errCode = FSClose(fRefNum);
}

#endif // TARGET_API_MAC_CARBON_FILEIO
