/***************************************************************************
*	FILE:	InsNew.c
*	PROJ:   Nightingale, rev. for v.3.5
*	DESC:   Lower-level routines to add music symbols that actually modify
*	the object list.
		ClefBeforeBar			KeySigBeforeBar		TimeSigBeforeBar     
		AddDot					AddNote					AddGRNote
		NewArpSign				GetLineOffsets			NewLine
		NewGraphic				NewMeasure
		NewPseudoMeas			NewClef					NewKeySig
		NewTimeSig				NewMODNR					NewDynamic				
		NewRptEnd				NewEnding				NewTempo
		XLoadInsertSeg
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEª PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* -------------------------------------------------- Clef/KeySig/TimeSigBeforeBar -- */

Boolean ClefBeforeBar(Document *doc,
							LINK pLPIL,
							char inchar,						/* Symbol to add */
							INT16 staffn
							)
{
	LINK			firstMeasL, firstClefL;
	Boolean 		isClef;
	LINK			endL, doneL;
	INT16			sym;
	
	if (!LinkBefFirstMeas(pLPIL)) return FALSE;

	if (doc->currentSystem==1) {
		firstMeasL = LSSearch(doc->headL, MEASUREtype, 1, FALSE, FALSE);
		firstClefL = LSSearch(firstMeasL, CLEFtype, 1, TRUE, FALSE);

		if (firstClefL) {

			isClef = InitialClefExists(firstClefL);
			sym = GetSymTableIndex(inchar);
			doneL = ReplaceClef(doc,firstClefL,staffn,symtable[sym].subtype);

			if (!isClef) {

				/* EndSystemSearch gives the object which terminates pLPIL's
					system; FixInitialxds calls EndSystemSearch again with
					its endL parameter; if we pass endL rather than LeftLINK(endL)
					it moves xds on the system following pLPIL's as well as pLPIL's */

				endL = EndSystemSearch(doc, pLPIL);
				FixInitialxds(doc, firstClefL, LeftLINK(endL), CLEFtype);
			}

			LinkVALID(firstClefL) = FALSE;
			InvalSystems(firstClefL, doneL ? doneL : firstClefL);
		}
		else
			MayErrMsg("ClefBeforeBar: no clef before 1st Measure.");
	}
	return TRUE;
}


Boolean KeySigBeforeBar(Document *doc, LINK pLPIL, INT16 staffn, INT16 sharpsOrFlats)
{
	LINK	firstMeasL,firstKeySigL,endL;
	
	if (!LinkBefFirstMeas(pLPIL)) return FALSE;

#ifdef NOTYET
	doc->undo.param1 = staff;
	PrepareUndo(doc, doc->selStartL, U_InsertKeySig, 13); 		/* "Undo Insert" */
#endif
	if (doc->currentSystem==1) {
		firstMeasL = LSSearch(doc->headL, MEASUREtype, 1, FALSE, FALSE);
		firstKeySigL = LSSearch(firstMeasL, KEYSIGtype, 1, TRUE, FALSE);
		if (firstKeySigL) {
			endL = ReplaceKeySig(doc, firstKeySigL, staffn, sharpsOrFlats);
			FixInitialKSxds(doc, firstKeySigL, endL, staffn);
		}
		else
			MayErrMsg("KeySigBeforeBar: no keysig before 1st Measure.");
	}

	LinkVALID(firstKeySigL) = FALSE;
	InvalSystems(firstKeySigL, endL);
	return TRUE;
}


/*
 * #1. Replacement of timeSig before bar is accomplished by calling FindStaff
 *			to set doc->currentSystem from the calling function, and then seeing
 *			if pLPIL (pointer to LastPreviousItemLINK) is before the first invisible
 *			measure of system 1 of the score.
 * #2. Will refine this algorithm by comparing with of old and
 *			new timeSigs; for now, use this 1st approximation.
 * #3. See comment in ClefBeforeBar for LeftLINK(endL).
 */

Boolean TimeSigBeforeBar(Document *doc, LINK pLPIL, INT16 staffn, INT16 type,
									INT16 numerator, INT16 denominator)
{
	LINK firstMeasL,firstTSL,endL;
	Boolean isTimeSig;
	
	if (!LinkBefFirstMeas(pLPIL)) return FALSE;							/* #1 */

	if (doc->currentSystem==1) {
		firstMeasL = LSSearch(doc->headL, MEASUREtype, 1, FALSE, FALSE);
		firstTSL = LSSearch(firstMeasL, TIMESIGtype, 1, TRUE, FALSE);
		if (firstTSL) {

			/* If there was no timeSig prior to this replacement, then
				xds need to be updated. */

			isTimeSig = BFTimeSigExists(firstTSL);							/* #2 */
			ReplaceTimeSig(doc, firstTSL, staffn, type, numerator, denominator);

			if (!isTimeSig) {
				endL = EndSystemSearch(doc, pLPIL);							/* #3 */
				FixInitialxds(doc, firstTSL, LeftLINK(endL), TIMESIGtype);
			}

			if (doc->showDurProb)
				InvalWindow(doc);					/* By far the simplest safe way! */
			else
				InvalSystem(firstTSL);
			LinkVALID(firstTSL) = FALSE;
		}
		else
			MayErrMsg("TimeSigBeforeBar: no timesig before 1st Measure.");
	}
	return TRUE;
}


/* ---------------------------------------------------------------------- AddDot -- */
/* Add an augmentation dot to the note/rest or chord in the given Sync and voice.
If it's a chord, assumes all the notes have the same duration. */

void AddDot(Document *doc,
				LINK syncL,		/* Sync to whose notes/rest dot will be added */
				INT16 voice		
				)
{
	PANOTE	aNote;
	LINK		aNoteL;
	INT16		playDur;
		
	aNoteL = FindMainNote(syncL, voice);
	aNote = GetPANOTE(aNoteL);
	if (aNote->subType+aNote->ndots>=MAX_L_DUR)	{
		SysBeep(10);
		return;
	}

	if (aNote->subType<=UNKNOWN_L_DUR) {
		if (aNote->subType==UNKNOWN_L_DUR) {
			GetIndCString(strBuf, INSERTERRS_STRS, 3);    /* "You can't add a dot to a note of unknown duration." */
			CParamText(strBuf, "", "", "");
		}
		else {
			GetIndCString(strBuf, INSERTERRS_STRS, 4);    /* "You can't add a dot to a whole-measure rest. To get dotted whole-rests, use the Set Duration command." */
			CParamText(strBuf, "", "", "");
		}
		StopInform(GENERIC_ALRT);
		return;
	}
	
	PrepareUndo(doc, syncL, U_Insert, 14);    				/* "Undo Add Dot" */

	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice) {
			NoteNDOTS(aNoteL)++;
			playDur = SimplePlayDur(aNoteL); 					/* Set physical dur. to default */
			NotePLAYDUR(aNoteL) = playDur;
			doc->changed = TRUE;
		}
		
	if (doc->autoRespace)
		RespaceBars(doc, syncL, syncL, 0L, FALSE, FALSE);
	else
		InvalMeasure(syncL, NoteSTAFF(aNoteL)); 			/* Just redraw the measure */

	FixTimeStamps(doc, syncL, syncL);
	MEAdjustCaret(doc, TRUE);
}


/* --------------------------------------------------------------------- AddNote -- */
/* Add a note or rest to the object list at <doc->selStartL> */

#define TIESUBTYPE ((INT16)TRUE)
#define SLURSUBTYPE ((INT16)FALSE)

LINK AddNote(Document *doc,
				INT16	x,			/* >=0 means new Sync, and this is its horiz. position in pixels, */
									/* <0  means add note/rest to the existing Sync <doc->selStartL>. */
				char inchar,	/* Symbol to add */
				INT16	staff,	/* Staff number */
				INT16	pitchLev,/* Half-line pitch level */
				INT16	acc,		/* Accidental code */
				INT16 octType	/* -1 if not in Octava, else octSignType */
				)
{
	PANOTE	aNote;
	LINK		newL;
	LINK		aNoteL=NILINK,measL;
	INT16		sym, noteDur, noteNDots,
				midCpitchLev, voice;
	Boolean 	inChord,				/* whether added note will be in a chord or not */
				isRest,
				beamed;
	CONTEXT	context;
	DDIST		xd, measWidth;

	PrepareUndo(doc, doc->selStartL, U_Insert, 13);    		/* "Undo Insert" */
	NewObjInit(doc, SYNCtype, &sym, inchar, staff, &context);

	voice = USEVOICE(doc, staff);
	isRest = (symtable[sym].subtype!=0);							/* Set note/rest flag */
	noteDur = Char2Dur(inchar);
	
	/* If symbol is a whole rest and the measure has no other notes/rests in its voice,
	 *	make it a whole-measure rest. According to most experts, it shouldn't be a
	 *	whole-measure rest if the duration of the measure is extremely long, but that
	 *	doesn't seem worth bothering with.
	 */
	
	if (isRest && noteDur==WHOLE_L_DUR
	&&  !SyncInVoiceMeas(doc, doc->selStartL, voice, FALSE))
				noteDur = WHOLEMR_L_DUR;
	noteNDots = 0;
	
	if (x>=0)																/* Adding note to an existing Sync? */
		inChord = FALSE;
	else {																	/* Yes */
		if (LinkNENTRIES(doc->selStartL)>=MAXCHORD) {
			GetIndCString(strBuf, INSERTERRS_STRS, 5);    		/* "The chord already contains the max no. of notes." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			InvalMeasure(doc->selStartL, staff);
			return aNoteL;
		}
		inChord = FALSE;
		aNoteL = FirstSubLINK(doc->selStartL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==voice)	{							/* This note/rest in desired voice? */
				inChord = TRUE;
				midCpitchLev = pitchLev-ClefMiddleCHalfLn(context.clefType);
				aNote = GetPANOTE(aNoteL);
				if (midCpitchLev==qd2halfLn(aNote->yqpit) && !ControlKeyDown()) {
					GetIndCString(strBuf, INSERTERRS_STRS, 6);    /* "Nightingale can't handle chords containing unisons." */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					InvalMeasure(doc->selStartL, staff);
					return aNoteL;
				}
				aNote = GetPANOTE(aNoteL);
				noteDur = aNote->subType;								/* Use chord's values for duration */
				noteNDots = aNote->ndots;								/*   and no. of dots */
			}
	}

	isRest = (symtable[sym].subtype!=0);							/* Set note/rest flag */
	if (x>=0) {																	/* Create new Sync? */
		newL = InsertNode(doc, doc->selStartL, symtable[sym].objtype, 1);	/* Add Sync to data struct. */
		if (!newL) { NoMoreMemory(); return aNoteL; }
		xd = p2d(x)-context.measureLeft;
		NewObjSetup(doc, newL, staff, xd);
		aNoteL = FirstSubLINK(newL);
	}
	else {																		/* Not new Sync, add to existing one */
		if (!ExpandNode(doc->selStartL, &aNoteL, 1)) {
			NoMoreMemory();
			return aNoteL;
		}
		newL = doc->selStartL;
		doc->changed = doc->used = LinkSEL(newL) = TRUE;
	 	LinkTWEAKED(newL) = FALSE;
	}
	
	SetupNote(doc, newL, aNoteL, staff, pitchLev, noteDur, noteNDots,
								voice, isRest, acc, octType);
	aNote = GetPANOTE(aNoteL);
	aNote->selected = TRUE;													/* Select the subobject */
	aNote->inChord = inChord;
	if (inChord) aNote->ystem = aNote->yd;

	/* The order of the following "Fix" calls is significant. */

	AddNoteFixTuplets(doc, newL, aNoteL, voice);

	if (!AddNoteFixOctavas(newL, aNoteL))
		MayErrMsg("AddNote: AddNoteFixOctava couldn't allocate Octava subobject.");
	beamed = (VCheckBeamAcrossIncl(newL, voice)!=NILINK);

	if (inChord) FixSyncForChord(doc, newL, voice, beamed, 0, 0, NULL);
	if (beamed) AddNoteFixBeams(doc, newL, aNoteL);

	/* ... but this one can called anywhere. */
	FixWholeRests(doc, newL);
	
	/* If the new note is <inChord>, its duration will be the same as existing chord
		note(s), so it can't affect timestamps. */
		
	if (!inChord) FixTimeStamps(doc, newL, newL);

	doc->selStartL = newL;													/* Update selection first ptr */
	doc->selEndL = RightLINK(newL);										/* ...and last ptr */

	LinkVALID(newL) = FALSE;
	InsFixMeasNums(doc, newL);
	UpdateVoiceTable(doc, TRUE);

	/*
	 * If autoRespace is on, we need to update the graphic situation by respacing
	 *	the Measure the note is in (this also centers whole-measure rests) and moving
	 *	any following Measures in the System left or right. If autoRespace is off, we
	 *	have to center the "note" if it's a whole-measure rest, and always redraw the 
	 *	Measure.
	 */
	if (doc->autoRespace)
		RespaceBars(doc, doc->selStartL, doc->selEndL, 0L, FALSE, FALSE);
	else {
		if (config.alwaysCtrWholeMR) {
			measL = LSSearch(newL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
			measWidth = MeasWidth(measL);
			if (noteDur==WHOLEMR_L_DUR) CenterNR(doc, newL, aNoteL, measWidth/2);
		}
		InvalMeasure(newL, staff);										/* Always redraw the measure */
	}
	
	return aNoteL;
}
	

/* --------------------------------------------------------------------- AddGRNote -- */
/* Add a grace note to the object list at doc->selStartL. */

LINK AddGRNote(Document *doc,
					INT16	x,				/* >=0 means new GRSync, and this is its horiz. position in pts., */	
											/* <0  means add grace note to the existing GRSync <doc->selStartL>. */
					char	inchar,		/* Symbol to add */
					INT16	staff,		/* Staff number */
					INT16	pitchLev,	/* Half-line pitch level */
					INT16	acc,			/* Accidental code */
					INT16	octType		/* -1 if not in Octava, else octSignType */
					)
{
	PAGRNOTE	aNote;
	LINK		aNoteL=NILINK, newL;
	INT16		sym, noteDur, noteNDots,
				midCpitchLev, voice;
	Boolean 	inChord,				/* whether added grace note will be in a chord or not */
				beamed;
	CONTEXT	context;
	DDIST		xd;

	PrepareUndo(doc, doc->selStartL, U_Insert, 13);    	/* "Undo Insert" */
	NewObjInit(doc, GRSYNCtype, &sym, inchar, staff, &context);

	noteDur = Char2Dur(inchar);
	noteNDots = 0;
	voice = USEVOICE(doc, staff);
	
	if (x>=0)															/* Adding grace note to an existing Sync? */
		inChord = FALSE;
	else
	{																		/* Yes */
		if (LinkNENTRIES(doc->selStartL)>=MAXCHORD) {
			GetIndCString(strBuf, INSERTERRS_STRS, 5);    	/* "The chord already contains the max. no. of notes." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			InvalMeasure(doc->selStartL, staff);
			return aNoteL;
		}
		inChord = FALSE;
		aNoteL = FirstSubLINK(doc->selStartL);
		for ( ; aNoteL; aNoteL = NextGRNOTEL(aNoteL))
			if (GRNoteVOICE(aNoteL)==voice)	{							/* This grace note in desired voice? */
				inChord = TRUE;
				midCpitchLev = pitchLev-ClefMiddleCHalfLn(context.clefType);
				aNote = GetPAGRNOTE(aNoteL);
				if (midCpitchLev==qd2halfLn(aNote->yqpit)) {
					GetIndCString(strBuf, INSERTERRS_STRS, 6);    	/* "Nightingale can't handle chords containing unisons." */
					CParamText(strBuf, "", "", "");
					StopInform(GENERIC_ALRT);
					InvalMeasure(doc->selStartL, staff);
					return aNoteL;
				}
				aNote = GetPAGRNOTE(aNoteL);
				noteDur = aNote->subType;									/* Use chord's values for duration */
				noteNDots = aNote->ndots;									/*   and no. of dots */
			}
	}

	if (x>=0) {																	/* Create new GRSync? */
		newL = InsertNode(doc, doc->selStartL, symtable[sym].objtype, 1); /* Add GRSync to data struct. */
		if (!newL) { NoMoreMemory(); return aNoteL; }
		xd = p2d(x)-context.measureLeft;
		NewObjSetup(doc, newL, staff, xd);
		aNoteL = FirstSubLINK(newL);
	}
	else {																		/* Not new Sync, add to existing one */
		if (!ExpandNode(doc->selStartL, &aNoteL, 1)) {
			NoMoreMemory();
			return aNoteL;
		}
		newL = doc->selStartL;
		doc->changed = doc->used = LinkSEL(newL) = TRUE;
	 	LinkTWEAKED(newL) = FALSE;
	}
	
	SetupGRNote(doc, newL, aNoteL, staff, pitchLev, noteDur, noteNDots,
								voice, acc, octType);
	aNote = GetPAGRNOTE(aNoteL);
	aNote->selected = TRUE;													/* Select the subobject */
	aNote->inChord = inChord;
	if (inChord) aNote->ystem = aNote->yd;

	/* N.B. The order of the following "Fix" calls is significant: change only with care. */
#ifdef ADDGRNOTE_FIXOCTAVAS
	if (!AddNoteFixOctavas(newL, aNoteL))
		MayErrMsg("AddGRNote: AddNoteFixOctava couldn't allocate octava subobject.");
#endif

	beamed = (Boolean)VCheckGRBeamAcrossIncl(newL, voice);
	if (inChord) FixGRSyncForChord(doc, newL, voice, beamed, 0, TRUE, NULL);
	if (beamed) AddGRNoteFixBeams(doc, newL, aNoteL);
	
	doc->selStartL = newL;														/* Update selection first ptr */
	doc->selEndL = RightLINK(newL);											/* ...and last ptr */

	InsFixMeasNums(doc, newL);
	UpdateVoiceTable(doc, TRUE);

	if (doc->autoRespace)
		RespaceBars(doc, doc->selStartL, doc->selEndL, 0L, FALSE, FALSE);
	else
		InvalMeasure(newL, staff);										/* Just redraw the measure */

	return aNoteL;
}
	

/* -------------------------------------------------------------- GetChordEndpts -- */

Boolean GetChordEndpts(Document *, LINK, INT16, DDIST *, DDIST *);
Boolean GetChordEndpts(Document */*doc*/, LINK syncL, INT16 voice, DDIST *ydTop,
								DDIST *ydBottom)
{
	LINK aNoteL;
	DDIST top, bottom;
	
	if (!syncL || !SyncTYPE(syncL)) return FALSE;
	
	top = 9999; bottom = -9999;
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			if (NoteYD(aNoteL)<top) top = NoteYD(aNoteL);
			if (NoteYD(aNoteL)>bottom) bottom = NoteYD(aNoteL);
		}
	}
	
	*ydTop = top; *ydBottom = bottom;
	return TRUE;
}


/* ------------------------------------------------------------------ NewArpSign -- */

/* Ross says the arpeggio sign should be 3/4 space before note but that's much closer
than his example shows; anyway, his arpeggio sign looks narrower than Sonata's.
To avoid the cutoffs on the non-arpeggio sign disappearing into staff lines,
INCREASE_HEIGHT should not be a multiple of STD_LINEHT/2. */

#define ARP_DIST_TO_NOTE (5*STD_LINEHT/4)	/* Distance, arp.sign left edge to note left edge */ 
#define INCREASE_HEIGHT (3*STD_LINEHT/4)	/* On each end, beyond the chord's extreme notes */ 

Boolean NewArpSign(Document *doc,
						Point /*pt*/,
						char inchar,	/* Input char. code for symbol to add */
						INT16 staff,
						INT16 voice
						)
{
	INT16 sym; LINK newL, aGraphicL; CONTEXT context;
	DDIST xd, ydTop, ydBottom; QDIST height;
	PGRAPHIC pGraphic; PAGRAPHIC aGraphic;
	Boolean okay=FALSE;

PushLock(OBJheap);
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);    					/* "Undo Insert" */

	NewObjInit(doc, GRAPHICtype, &sym, inchar, staff, &context);

	newL = InsertNode(doc, doc->selStartL, symtable[sym].objtype, 1);	/* Add new sym. to data struct. */
	if (!newL) { NoMoreMemory(); goto Cleanup; }
	xd = -std2d(ARP_DIST_TO_NOTE, context.staffHeight, context.staffLines);
	GetChordEndpts(doc, doc->selStartL, voice, &ydTop, &ydBottom);
	ydTop -= std2d(INCREASE_HEIGHT, context.staffHeight, context.staffLines);
	ydBottom += std2d(INCREASE_HEIGHT, context.staffHeight, context.staffLines);

	NewObjSetup(doc, newL, staff, xd);
	SetObject(newL, xd, ydTop, TRUE, TRUE, FALSE);

	InitGraphic(newL, GRArpeggio, staff, voice, 0, 0, 0, 0, 0);

	pGraphic = GetPGRAPHIC(newL);
	pGraphic->info2 = symtable[sym].durcode;
	pGraphic->info2 <<= 13;				/* ??ADD A MACRO TO NTypes.h AND USE HERE! */
	height = d2qd(ydBottom-ydTop, context.staffHeight, context.staffLines);
	pGraphic->info = height;
	pGraphic->firstObj = doc->selStartL;

	pGraphic->lastObj = NILINK;		/* The remaining fields are unused in arpeggio signs */

	aGraphicL = FirstSubLINK(newL);
	aGraphic = GetPAGRAPHIC(aGraphicL);
	aGraphic->next = NILINK;
	aGraphic->string = 0;

	pGraphic->justify = GRJustLeft;

	doc->selStartL = newL;
	doc->selEndL = RightLINK(newL);
	
	InvalMeasure(newL, staff);
	okay = TRUE;
		
Cleanup:
PopLock(OBJheap);
	return okay;
}


/* ---------------------------------------------------------- NewLine and helpers -- */

#define L_HDIST_TO_NOTE (2*STD_LINEHT/4)		/* Horiz. distance from attached line to note */
#define L_VOFFSET (3*STD_LINEHT/8)				/* Vertical offset for within-Sync lines */

/* Adjust <width> for given WIDEHEAD code. NB: watch out for integer overflow! */
#define WIDEHEAD_VALUE(whCode, width) (whCode==2? 160*(width)/100 : \
													(whCode==1? 135*(width)/100 : (width)) )

void GetGlissPositionInfo(CONTEXT context, DDIST *pxdPosition, DDIST *pxdOffset);
void GetGlissPositionInfo(CONTEXT context, DDIST *pxdPosition, DDIST *pxdOffset)
{
	*pxdPosition = std2d(L_HDIST_TO_NOTE, context.staffHeight, context.staffLines);
	*pxdOffset = HeadWidth(LNSPACE(&context));
}

void GetClusterPositionInfo(LINK firstL, INT16 voice, CONTEXT context, DDIST *pxdOffset, DDIST *pydOffset);
void GetClusterPositionInfo(LINK firstL, INT16 voice, CONTEXT context, DDIST *pxdOffset, DDIST *pydOffset)
{
	LINK aNoteL; short noteType; LONGDDIST ldHalfHeadWidth;
	
	aNoteL = FindMainNote(firstL, voice);
	noteType = NoteType(aNoteL);
	ldHalfHeadWidth = HeadWidth(LNSPACE(&context))/2;
	*pxdOffset = WIDEHEAD_VALUE(WIDEHEAD(noteType), ldHalfHeadWidth);

	*pydOffset = std2d(L_VOFFSET, context.staffHeight, context.staffLines);
}

/* Given information about the endpoints of a line, return x and y offsets at each
end from the object position.

Currently, lines can be attached only to notes, rests, and grace notes. Either endpt
can be attached to any of these types, regardless of what the other end is attached
to. We compute offsets on the assumption that, if both ends are attached to the same
Sync or grace Sync, it's something like a tone cluster in Martino's notation; otherwise,
it's something like a glissando or voice-leading line. */

static void GetLineOffsets(LINK firstL, LINK lastL, INT16 pitchLev1, INT16 pitchLev2,
							CONTEXT context, INT16 voice, DDIST *pxd1, DDIST *pxd2,
							DDIST *pyd1, DDIST *pyd2);
static void GetLineOffsets(LINK firstL, LINK lastL,
							INT16 pitchLev1, INT16 pitchLev2,		/* For left and right ends */
							CONTEXT context, INT16 voice,
							DDIST *pxd1, DDIST *pxd2,
							DDIST *pyd1, DDIST *pyd2)
{
	DDIST xd1, yd1, xd2, yd2, xdPosition, xdOffset, ydOffset;

	/*
	 * Set a default position, then override or adjust it:
	 * "Glissando": Offset horizontally inwards by L_HDIST_TO_NOTE more than the notehead
	 *		width.
	 * "Cluster": Offset horizontally to the centers of the noteheads. Offset vertically
	 * 	inwards a little bit.
	 * Horizontal coordinates are of the center of the line, not of either edge. Vertical
	 * 	coords. are ?? .
	 */
	
	xd1 = 0;
	yd1 = halfLn2d(pitchLev1, context.staffHeight, context.staffLines);
	xd2 = 0;
	yd2 = halfLn2d(pitchLev2, context.staffHeight, context.staffLines);

	if (firstL!=lastL) {
		GetGlissPositionInfo(context, &xdPosition, &xdOffset);
		/*
		 * It might seem that we shouldn't scale <xdPosition> for grace notes, but grace
		 * notes tend to appear very close together, so we have to do everything we
		 * can to keep lines between them from being too short or even going backwards!
		 * This is only likely to be a serious problem when the left end of the line is
		 * attached to a grace note--a rare case.
		 */
		xd1 = (GRSyncTYPE(firstL)? GRACESIZE(xdPosition) : xdPosition);
		xd1 += (GRSyncTYPE(firstL)? GRACESIZE(xdOffset) : xdOffset);
		xd2 = -(GRSyncTYPE(lastL)? GRACESIZE(xdPosition) : xdPosition);
	}
	else {
		GetClusterPositionInfo(firstL, voice, context, &xdOffset, &ydOffset);
		xd1 += (GRSyncTYPE(firstL)? GRACESIZE(xdOffset) : xdOffset);
		xd2 += (GRSyncTYPE(firstL)? GRACESIZE(xdOffset) : xdOffset);
		if (yd1<yd2) {
			yd1 += ydOffset;
			yd2 -= ydOffset;
		}
		else {
			yd1 -= ydOffset;
			yd2 += ydOffset;
		}
	}
	
	*pxd1 = xd1; *pyd1 = yd1;
	*pxd2 = xd2; *pyd2 = yd2;	
}

Boolean NewLine(Document *doc,
						INT16 pitchLev1, INT16 pitchLev2,			/* For left and right ends */
						char inchar,
						INT16 staff, INT16 voice,
						LINK lastL
						)
{
	INT16 sym; LINK newL, aGraphicL; CONTEXT context;
	DDIST xd1, yd1, xd2, yd2;
	PGRAPHIC pGraphic; PAGRAPHIC aGraphic;
	Boolean okay=FALSE;

PushLock(OBJheap);
PushLock(GRAPHICheap);
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);							/* "Undo Insert" */

	NewObjInit(doc, GRAPHICtype, &sym, inchar, staff, &context);

	newL = InsertNode(doc, doc->selStartL, symtable[sym].objtype, 1);
	if (!newL) { NoMoreMemory(); goto Cleanup; }
	
	NewObjSetup(doc, newL, staff, 0);	/* ??What does this accomplish beyond SetObject? */

	GetLineOffsets(doc->selStartL, lastL, pitchLev1, pitchLev2, context, voice,
						&xd1, &xd2, &yd1, &yd2);
	
	SetObject(newL, xd1, yd1, TRUE, TRUE, FALSE);

	InitGraphic(newL, GRDraw, staff, voice, 0, 0, 0, 0, 0);
	pGraphic = GetPGRAPHIC(newL);
	pGraphic->info = xd2;
	pGraphic->info2 = yd2;
	pGraphic->gu.thickness = config.lineLW;

	pGraphic->firstObj = doc->selStartL;
	pGraphic->lastObj = lastL;

	aGraphicL = FirstSubLINK(newL);
	aGraphic = GetPAGRAPHIC(aGraphicL);
	aGraphic->next = NILINK;
	aGraphic->string = 0;

	pGraphic->justify = GRJustLeft;

	doc->selStartL = newL;
	doc->selEndL = RightLINK(newL);
	
	InvalMeasures(newL, lastL, staff);
	okay = TRUE;
		
Cleanup:
PopLock(GRAPHICheap);
PopLock(OBJheap);
	return okay;
}


/* ------------------------------------------------------------------ NewGraphic -- */
/* Add a Graphic (including character or string) to the object list. */

void NewGraphic(
			Document *doc,
			Point	pt,					/* (Ignored) */
			char	inchar,				/* Input char. code for symbol to add */
			INT16	staff, INT16 voice,
			INT16 pitchLev,			/* "Pitch level", i.e., vertical position (halflines) */
			Boolean relFSize,
			INT16	fSize, INT16 fStyle,
			INT16 enclosure,
			INT16	auxInfo,				/* currently only for chord symbol */
			Boolean lyric,
			unsigned char font[],
			unsigned char string[],
			INT16 styleChoice			/* Header (not user) index of global style choice */
			)
{
	INT16 sym, graphicType, fontInd;
	LINK newL, aGraphicL, pageL; CONTEXT context; DDIST xd, yd;
	PGRAPHIC pGraphic; PAGRAPHIC aGraphic;
	INT16 ans, patchNum, panSetting;
	unsigned char midiPatch[16];
	
PushLock(OBJheap);
PushLock(GRAPHICheap);
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  						/* "Undo Insert" */

	if (PageTYPE(doc->selStartL) || SystemTYPE(doc->selStartL))
		NewObjInit(doc, GRAPHICtype, &sym, inchar, ANYONE, &context);
	else
		NewObjInit(doc, GRAPHICtype, &sym, inchar, staff, &context);

	graphicType = symtable[sym].subtype;
	if (symtable[sym].subtype==GRString)
		graphicType = (lyric? GRLyric : GRString);
	newL = InsertNode(doc, doc->selStartL, symtable[sym].objtype, 1);	/* Add new sym. to data struct. */
	if (!newL) { NoMoreMemory(); goto Cleanup; }
	xd = 0;
	yd = halfLn2d(pitchLev, context.staffHeight, context.staffLines);
	NewObjSetup(doc, newL, staff, xd);
	SetObject(newL, xd, yd, TRUE, TRUE, FALSE);

	fontInd = GetFontIndex(doc, font);
	if (fontInd<0) {
		GetIndCString(strBuf, MISCERRS_STRS, 20);    	/* "Will use most recently added font." */
		CParamText(strBuf, "", "", "");
		CautionInform(MANYFONTS_ALRT);
		fontInd = MAX_SCOREFONTS-1;
	}
	InitGraphic(newL, graphicType, staff, voice, fontInd, relFSize, fSize, fStyle, enclosure);

	pGraphic = GetPGRAPHIC(newL);

	if (SystemTYPE(doc->selStartL)) {						/* Page and System relative graphics inserted before the system object */
		pGraphic->xd = p2d(pt.h);
		pGraphic->yd = p2d(pt.v);
	}
	
	if (graphicType==GRMIDIPatch) {
		PStrCopy(string, midiPatch);
		patchNum = FindIntInString(string);
		//PToCString(midiPatch);
		//ans = sscanf((char*)midiPatch, "%d", &patchNum);
		pGraphic->info = patchNum;
	}
	else if (graphicType==GRMIDIPan) {
		panSetting = FindIntInString(string);
		pGraphic->info = panSetting;
	}
	else if (graphicType==GRMIDISustainOn) {
		pGraphic->info = MIDISustainOn;
	}
	else if (graphicType==GRMIDISustainOff) {
		pGraphic->info = MIDISustainOff;
	}

	if (graphicType==GRChar) {
		pGraphic->info = inchar;
	}
	else {
		pGraphic->justify = GRJustLeft;						/* unused as of v2.1 */
		aGraphicL = FirstSubLINK(newL);
		aGraphic = GetPAGRAPHIC(aGraphicL);
		aGraphic->next = NILINK;
		aGraphic->string = PStore((unsigned char *)string);
		if (aGraphic->string<0L)
			NoMoreMemory();
		else if (aGraphic->string>GetHandleSize((Handle)doc->stringPool))
			MayErrMsg("NewGraphic: PStore error. string=%ld", aGraphic->string);
	}

	if (graphicType==GRLyric || graphicType==GRString) {
			INT16 i;
			pGraphic->multiLine = FALSE;
			for (i = 1; i <= string[0]; i++)
				if (string[i] == CH_CR) {
					pGraphic->multiLine = TRUE;
					break;
				}
			pGraphic->info = styleChoice;
	}
	else if (graphicType==GRChordSym) {
		pGraphic->info = auxInfo;
	}

	/* Graphics cannot be system relative, but page relative graphics must be
		inserted after their pages. Therefore in InsertGraphic, for page relative
		graphics, doc->selStartL is set to the first system after the page in
		order to insert the graphic in front of that system, and the firstObj is
		set here to be the page. */

	if (SystemTYPE(doc->selStartL)) {
		pageL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_LEFT, FALSE);
		pGraphic->firstObj = pageL;
	}
	else
		pGraphic->firstObj = doc->selStartL;
	pGraphic->lastObj = NILINK;

	/* If a GRLyric is on a staff (i.e., is not page-relative), center around rel. obj
		unless shift key is down. */
	if (staff!=NOONE && graphicType==GRLyric && !ShiftKeyDown())
		CenterTextGraphic(doc, newL);

	doc->selStartL = newL;
	doc->selEndL = RightLINK(newL);
	
	InvalSystem(newL);										/* Redraw the system, in case graphic too long */
	if (PageTYPE(GraphicFIRSTOBJ(newL)))
		InvalWindow(doc);
	else
		InvalObject(doc,newL, FALSE);
		
Cleanup:
PopLock(GRAPHICheap);
PopLock(OBJheap);
}


/* ------------------------------------------------------------------ NewMeasure -- */
/*	Add a Measure to the object list for all staves. */

void NewMeasure(Document *doc,
					Point pt,		/* Page-relative starting x-position of measure */
					char inchar		/* Input char. code for symbol to add */
					)
{
	INT16			sym;
	CONTEXT		context;
	LINK			measL,endL;
	DDIST			xdMeasure, xd;

	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  			/* "Undo Insert" */
	NewObjInit(doc, MEASUREtype, &sym, inchar, ANYONE, &context);

	xdMeasure = p2d(pt.h);												/* absolute DDIST */
	xd = xdMeasure - context.staffLeft;								/* relative to staff */
	measL = CreateMeasure(doc, doc->selStartL, xd, sym, context);

	doc->selStartL = measL; 											/* Update selection range */
	doc->selEndL = RightLINK(measL);
	
	if (doc->selEndL==doc->tailL) UpdateTailxd(doc);

	/* The immediate neighbors of the new barline are now in two different measures.
	 *	If auto-respace is TRUE, both must be respaced.
	 */
	if (doc->autoRespace)
		RespaceBars(doc,LeftLINK(doc->selStartL),RightLINK(doc->selStartL),0L,
						FALSE,FALSE);
	else {
		endL = EndSystemSearch(doc, measL);
		InvalMeasures(measL,endL,ANYONE);
	}
}


/* --------------------------------------------------------------- NewPseudoMeas -- */
/*	Add a pseudomeasure to the object list for all staves. */

void NewPseudoMeas(Document *doc,
						Point pt,		/* Page-relative starting x-position of pseudomeasure */
						char inchar		/* Input char. code for symbol to add */
						)
{
	INT16			sym, subType;
	LINK			pseudoMeasL, prevMeasL, aPseudoMeasL, aprevMeasL;
	PAPSMEAS		aPseudoMeas;
	PAMEASURE	aprevMeas;
	CONTEXT		context;								/* current context */
	DDIST			xdPSMeas,							/* click DDIST position relative to staff */
					xdPSMeasRelStaff;

PushLock(OBJheap);
PushLock(PSMEASheap);
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  			/* "Undo Insert" */
	NewObjInit(doc, PSMEAStype, &sym, inchar, ANYONE, &context);
	prevMeasL = LSSearch(LeftLINK(doc->selStartL), MEASUREtype, ANYONE,
													GO_LEFT, FALSE);
	if (!prevMeasL) {
		MayErrMsg("NewPseudoMeas: can't find previous measure");
		return;
	}

	/* Must tell InsertNode PSMEAStype, since symtable[sym].objtype may be MEASUREtype */
	pseudoMeasL = InsertNode(doc, doc->selStartL, PSMEAStype, doc->nstaves);
	if (!pseudoMeasL) { NoMoreMemory(); goto Cleanup; }

	xdPSMeas = p2d(pt.h);												/* absolute DDIST */
	xdPSMeasRelStaff = xdPSMeas - context.staffLeft;			/* relative to staff */	
	NewObjSetup(doc, pseudoMeasL, ANYONE, xdPSMeasRelStaff);

	if (symtable[sym].subtype==BAR_DOUBLE)
		subType = PSM_DOUBLE;
	else if (symtable[sym].subtype==BAR_FINALDBL)
		subType = PSM_FINALDBL;
	else
		subType = symtable[sym].subtype;

	aPseudoMeasL = FirstSubLINK(pseudoMeasL);
	for ( ; aPseudoMeasL; aPseudoMeasL = NextPSMEASL(aPseudoMeasL)) {
		aPseudoMeas = GetPAPSMEAS(aPseudoMeasL);
			aPseudoMeas->subType = subType;
	}
	aPseudoMeasL = FirstSubLINK(pseudoMeasL);
	aprevMeasL = FirstSubLINK(prevMeasL);
	for ( ; aPseudoMeasL; 
			aprevMeasL = NextMEASUREL(aprevMeasL),
			aPseudoMeasL = NextPSMEASL(aPseudoMeasL)) {
		aPseudoMeas = GetPAPSMEAS(aPseudoMeasL);
		aprevMeas = GetPAMEASURE(aprevMeasL);
		
		aPseudoMeas->staffn = aprevMeas->staffn;
		aPseudoMeas->selected = TRUE;
		aPseudoMeas->visible = TRUE;
		aPseudoMeas->soft = FALSE;
		aPseudoMeas->connAbove = aprevMeas->connAbove;
		aPseudoMeas->connStaff = aprevMeas->connStaff;
	}
	
	doc->selStartL = pseudoMeasL; 										/* Update selection first ptr */
	doc->selEndL = RightLINK(pseudoMeasL);								/* ...and last ptr */
	
	if (doc->selEndL==doc->tailL) UpdateTailxd(doc);

	if (doc->autoRespace)
		RespaceBars(doc, doc->selStartL, doc->selEndL, 0L, FALSE, FALSE);
	else
		InvalMeasure(doc->selStartL, ANYONE);

Cleanup:
PopLock(OBJheap);
PopLock(PSMEASheap);
}

/* ------------------------------------------------------------------- AddToClef -- */

void AddToClef(Document *doc, char inchar, INT16 staff)
{
	LINK aClefL,newL, doneL; INT16 sym; PACLEF aClef;
	CONTEXT context; SignedByte oldClefType;

	if (ExpandNode(newL=doc->selStartL, &aClefL, 1)) {
		sym = GetSymTableIndex(inchar);
		InitClef(aClefL, staff, 0, symtable[sym].subtype);

		aClef = GetPACLEF(aClefL);
		aClef->small = ClefINMEAS(newL);
		aClef->soft = FALSE;
		LinkSEL(newL) = aClef->selected = TRUE;
		doc->selEndL = RightLINK(newL);

		GetContext(doc, LeftLINK(doc->selStartL), staff, &context);
		oldClefType = context.clefType;
		doneL = FixContextForClef(doc, RightLINK(newL), staff, oldClefType,
											symtable[sym].subtype);
		InvalSystems(RightLINK(newL), (doneL? doneL : RightLINK(newL)));
		NewObjCleanup(doc, newL, staff);
	}
	else
		NoMoreMemory();
}

/* --------------------------------------------------------------------- NewClef -- */
/* Add a Clef to the object list. */

void NewClef(Document *doc,
				INT16	x,			/* <0 = add to existing clef obj, else horiz. position (pixels) */
				char inchar,	/* Input char. code for symbol to add */
				INT16 staff		/* Staff number */
				)
{
	INT16 sym; PCLEF newp; PACLEF aClef; LINK newL,aClefL, doneL;
	CONTEXT context; SignedByte oldClefType;

	doc->undo.param1 = staff;
	PrepareUndo(doc, doc->selStartL, U_InsertClef, 13);  					/* "Undo Insert" */

	if (x<0) {
		AddToClef(doc,inchar,staff);
		return;
	}
	newL = NewObjPrepare(doc, CLEFtype, &sym, inchar, staff, x, &context);
	newp = GetPCLEF(newL);
	newp->inMeasure = TRUE;
	
	if (context.clefType<=0)
		MayErrMsg("NewClef: GetContext(%ld, %ld, ...) gives clef=%ld",
					(long)newL, (long)staff, (long)context.clefType);
	oldClefType = context.clefType;
	
	aClefL = FirstSubLINK(newL);
	InitClef(aClefL, staff, 0, symtable[sym].subtype);

	aClef = GetPACLEF(aClefL);
	aClef->selected = TRUE;
	aClef->small = ClefINMEAS(newL);
	aClef->soft = FALSE;

	doneL = FixContextForClef(doc, RightLINK(newL), staff, oldClefType,
										symtable[sym].subtype);
	InvalSystems(RightLINK(newL), (doneL? doneL : RightLINK(newL)));
	NewObjCleanup(doc, newL, staff);
}


/* ---------------------------------------------- Utilities for inserting KeySigs -- */

static Boolean ChkIns1KSCancel(LINK ksL, INT16 staff);
static Boolean ChkInsKSCancel(Document *doc, LINK insL, INT16 staff);

static Boolean ChkIns1KSCancel(LINK ksL, INT16 staff)
{
	LINK aKeySigL; PAKEYSIG aKeySig;
	char fmtStr[256];

	aKeySigL = KeySigOnStaff(ksL, staff);
	aKeySig = GetPAKEYSIG(aKeySigL);
  	if (aKeySig->nKSItems>0) return TRUE;

	GetIndCString(fmtStr, INSERTERRS_STRS, 22);    /* "Staff %d has no key signature to cancel." */
	sprintf(strBuf, fmtStr, staff); 
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);
	return FALSE;
}

static Boolean ChkInsKSCancel(Document *doc, LINK insL, INT16 staff)
{
	INT16 loopEnd, i, s; LINK prevKSL;
	
	loopEnd = (staff==ANYONE ? doc->nstaves : 1);
	for (i = 1; i <= loopEnd; i++) {
		s = (staff==ANYONE ? i : staff);
		prevKSL = LSSearch(LeftLINK(insL), KEYSIGtype, s, GO_LEFT, FALSE);
		/* There should always be a previous key sig. obj; if not, just return */
		if (!prevKSL) return FALSE;
		if (!ChkIns1KSCancel(prevKSL, s)) return FALSE;
	}

	return TRUE;
}

/* ------------------------------------------------------------------- NewKeySig -- */
/* Add a KeySig to the object list at the insertion point. Assumes the
insertion point is not in the first system's reserved area! */

void NewKeySig(Document *doc,
					INT16 x,						/* Horizontal position (pixels) */
					INT16 sharpsOrFlats,		/* No. of sharps or flats in key sig. */
					INT16 staff					/* Staff number, or ANYONE for all staves */
					)
{
	INT16			i, sym, stfCount, useStaff;
	char			dummyInchar='X';
	PKEYSIG		newp;
	PAKEYSIG		aKeySig;
	PAKEYSIG		aPrevKeySig;
	LINK			newL;
	LINK			prevKSL,
					aKeySigL, aPrevKeySigL,
					endL, qL, initKSL;
	KSINFO		oldKSInfo, newKSInfo;
	CONTEXT		context[MAXSTAVES+1],*pContext,
					aContext;										/* context at LeftLINK(doc->selStartL) */
	
	if (!sharpsOrFlats)
		if (!ChkInsKSCancel(doc, doc->selStartL, staff)) return;

PushLock(OBJheap);
PushLock(KEYSIGheap);

	doc->undo.param1 = staff;
	PrepareUndo(doc, doc->selStartL, U_InsertKeySig, 13);  		/* "Undo Insert" */

	GetAllContexts(doc, context, LeftLINK(doc->selStartL));

	newL = NewObjPrepare(doc, KEYSIGtype, &sym, dummyInchar, staff, x, &aContext);
	LinkVALID(newL) = FALSE;

	endL = newL;
	stfCount = (staff==ANYONE ? doc->nstaves : 1);
	newp = GetPKEYSIG(newL);
	newp->inMeasure = TRUE;
	aKeySigL = FirstSubLINK(newL);
	for (i = 1; i <= stfCount; i++,aKeySigL = NextKEYSIGL(aKeySigL)) {
		useStaff = (staff==ANYONE ? i : staff);
		InitKeySig(aKeySigL, useStaff, 0, sharpsOrFlats);
		aKeySig = GetPAKEYSIG(aKeySigL);
		aKeySig->selected = TRUE;											/* Select the subobj */
		aKeySig->visible = TRUE;											/* Regardless of sharpsOrFlats */

		SetupKeySig(aKeySigL, sharpsOrFlats);
		if (sharpsOrFlats==0) {
			prevKSL = LSSearch(LeftLINK(newL), KEYSIGtype, useStaff, GO_LEFT, FALSE);
			aPrevKeySigL = KeySigOnStaff(prevKSL,useStaff);			/* Should always find one */

			aPrevKeySig = GetPAKEYSIG(aPrevKeySigL);
		  	if (aPrevKeySig->nKSItems>0)
				aKeySig->subType = aPrevKeySig->nKSItems;
		}
		KeySigCopy((PKSINFO)aKeySig->KSItem, &newKSInfo);			/* Safe bcs heap locked! */
		
		pContext = &context[useStaff];
		KeySigCopy((PKSINFO)pContext->KSItem, &oldKSInfo);

		FixContextForKeySig(doc, RightLINK(newL), useStaff, oldKSInfo, newKSInfo);
		qL = FixAccsForKeySig(doc, newL, useStaff, oldKSInfo, newKSInfo);
		if (qL)
			if (IsAfter(endL, qL)) endL = qL;
	}

	/* Look for KeySig at beginning of the next System. If we find one, update the
	 *	space before the initial Measure on that and all following Systems until the
	 * end of the range of the new KeySig on all staves.
	 */ 
	initKSL = LSSearch(RightLINK(newL), KEYSIGtype, ANYONE, GO_RIGHT, FALSE);
	if (initKSL) FixInitialKSxds(doc, initKSL, endL, staff);

	InvalSystems(newL, (endL? endL : newL));
	NewObjCleanup(doc, newL, staff);
	
PopLock(OBJheap);
PopLock(KEYSIGheap);
}


/* ------------------------------------------------------------------ NewTimeSig -- */
/* Add a TimeSig to the object list. */

#define TIMESIG_PALCHAR '8'		/* Substitute this for blank <inchar> */

void NewTimeSig(Document *doc,
					INT16	x,						/* Horizontal position (pixels) */
					char	inchar,				/* Input char. code for symbol to add */
					INT16	staff,				/* Staff number */
					INT16	type,
					INT16 numerator,
					INT16	denominator			/* Numerator and denominator of timesig */
					)
{
	INT16			sym, i, stfCount, useStaff;
	PTIMESIG		newp;
	LINK 			newL, aTimeSigL;
	PATIMESIG	aTimeSig;
	CONTEXT		context;											/* current context */
	TSINFO		timeSigInfo;
	
	if (inchar==' ') inchar = TIMESIG_PALCHAR;
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  	/* "Undo Insert" */

	newL = NewObjPrepare(doc, TIMESIGtype, &sym, inchar, staff, x, &context);
	newp = GetPTIMESIG(newL);
	newp->inMeasure = TRUE;

	stfCount = (staff==ANYONE ? doc->nstaves : 1);
	aTimeSigL = FirstSubLINK(newL);		
	for (i = 1; i <= stfCount; i++, aTimeSigL = NextTIMESIGL(aTimeSigL)) {
		useStaff = (staff==ANYONE ? i : staff);
		InitTimeSig(aTimeSigL, useStaff, 0, type, numerator, denominator);
		aTimeSig = GetPATIMESIG(aTimeSigL);
		aTimeSig->soft = FALSE;
		aTimeSig->selected = TRUE;								/* Select the subobj */
		timeSigInfo.TSType = aTimeSig->subType;
		timeSigInfo.numerator = aTimeSig->numerator;
		timeSigInfo.denominator = aTimeSig->denominator;
		FixContextForTimeSig(doc, RightLINK(newL), useStaff, timeSigInfo);
	}
	FixTimeStamps(doc, newL, NILINK);						/* In case of whole-meas. rests */
	NewObjCleanup(doc, newL, staff);
	if (doc->showDurProb) InvalWindow(doc);				/* Overkill but should be acceptable */
}


/* --------------------------------------------------------------- ModNRPitchLev -- */
typedef struct {
	Byte		canBeInStaff;	/* can put modifier inside staff, if note head pos. allows (boolean) */
	Byte		alwaysAbove;	/* if only 1 voice on staff, put modifier above staff, even if stem is up (boolean) */
	SHORTQD	yOffsetBelow;	/* mod below notehead: rel. to default pos. for staccato; in qtr-lines */
	SHORTQD	yOffsetAbove;	/* mod above notehead: rel. to default pos. for staccato; in qtr-lines */
} MODINFO;

static MODINFO modInfoTable[] = {
/* canBeInStaff	alwaysAbove	yOffsetBelow	yOffsetAbove		[modCode indices] */
	0,					0,				2,					-2,					/* '0' */
	0,					0,				2,					-2,					/* '1' */
	0,					0,				2,					-2,					/* '2' */
	0,					0,				2,					-2,					/* '3' */
	0,					0,				2,					-2,					/* '4' */
	0,					0,				2,					-2,					/* '5' */
	0,					0,				2,					-2,					/* '6' */
	0,					0,				2,					-2,					/* '7' */
	0,					0,				2,					-2,					/* '8' */
	0,					0,				2,					-2,					/* '9' */
	0,					1,				0,					-2,					/*	MOD_FERMATA */
	0,					1,				0,					-2,					/*	MOD_TRILL */
	0,					0,				2,					-1,					/*	MOD_ACCENT */
	0,					0,				3,					-2,					/*	MOD_HEAVYACCENT */
	1,					0,				0,					0,						/*	MOD_STACCATO */
	1,					0,				0,					0,						/*	MOD_WEDGE */
	1,					0,				0,					0,						/*	MOD_TENUTO */
	0,					1,				0,					-3,					/*	MOD_MORDENT */
	0,					1,				0,					-2,					/*	MOD_INV_MORDENT */
	0,					1,				0,					-2,					/*	MOD_TURN */
	0,					1,				0,					-2,					/*	MOD_PLUS */
	1,					0,				0,					0,						/*	MOD_CIRCLE */
	0,					1,				0,					-3,					/*	MOD_UPBOW */
	0,					1,				0,					-3,					/*	MOD_DOWNBOW */
	1,					0,				0,					0,						/*	MOD_TREMOLO1 - don't think trems are even used here */
	1,					0,				0,					0,						/*	MOD_TREMOLO2 */
	1,					0,				0,					0,						/*	MOD_TREMOLO3 */
	1,					0,				0,					0,						/*	MOD_TREMOLO4 */
	1,					0,				0,					0,						/*	MOD_TREMOLO5 */
	1,					0,				0,					0,						/*	MOD_TREMOLO6 */
	0,					0,				3,					-2,					/*	MOD_HEAVYACC_STACC */
	0,					1,				0,					-2						/*	MOD_LONG_INVMORDENT */
};
static INT16 modInfoLen = (sizeof(modInfoTable) / sizeof(MODINFO));	/* Length of modInfoTable */ 


/* Given a note/rest/chord to which a specified note modifier is to be added,
return a reasonable vertical position for the modifier, expressed as a "pitch
level" in quarter-lines. If the note/rest is in a chord, assumes the modifier
belongs to the chord as a whole, not to the individual note/rest, and so computes
the modifier position based on the location of the main note.  -JGG, 4/23/01 */

INT16 ModNRPitchLev(Document *doc,
						Byte modCode,			/* subtype field from symTable */
						LINK insSyncL,
						LINK theNoteL
						)
{
	INT16		voice, midCHalfLn, modQPit, stfTopQtrLn, stfBotQtrLn;
	LINK		mainNoteL;
	Boolean	stemDown, canBeInStaff, alwaysAbove, alwaysBelow, putModAbove, useStemEnd, hasMod;
	QDIST		yqpit, yqstem, yOffsetAbove, yOffsetBelow;
	CONTEXT	context;
		
	voice = NoteVOICE(theNoteL);
	mainNoteL = FindMainNote(insSyncL, voice);
	stemDown = (NoteYD(mainNoteL)<=NoteYSTEM(mainNoteL));

	GetContext(doc, insSyncL, NoteSTAFF(theNoteL), &context);
	midCHalfLn = ClefMiddleCHalfLn(context.clefType);			/* Get middle C staff pos. */		

	/* First compute these as half-lines, then convert to qtr-lines. */
	stfTopQtrLn = 0;
	stfBotQtrLn = (context.staffLines * 2) - 2;
	if (context.showLines==0 || context.showLines==1) {	/* and assuming context.staffLines==5 */
		stfTopQtrLn += 4;
		stfBotQtrLn -= 4;
	}
	stfTopQtrLn = halfLn2qd(stfTopQtrLn);
	stfBotQtrLn = halfLn2qd(stfBotQtrLn);

	if (modCode<modInfoLen) {
		canBeInStaff = (Boolean)modInfoTable[modCode].canBeInStaff;
		alwaysAbove = (Boolean)modInfoTable[modCode].alwaysAbove;
		yOffsetBelow = modInfoTable[modCode].yOffsetBelow;
		yOffsetAbove = modInfoTable[modCode].yOffsetAbove;
	}
	else {
		canBeInStaff = TRUE;
		alwaysAbove = FALSE;
		yOffsetAbove = yOffsetBelow = 0;
	}
	alwaysBelow = FALSE;

	/* NOTE: <alwaysAbove> and <alwaysBelow> can both be FALSE, but only one can be TRUE. */
	if (doc->voiceTab[voice].voiceRole==UPPER_DI)
		alwaysAbove = TRUE;
	else if (doc->voiceTab[voice].voiceRole==LOWER_DI)
		alwaysBelow = TRUE;

	if (alwaysAbove)
		putModAbove = TRUE;
	else if (alwaysBelow)
		putModAbove = FALSE;
	else
		putModAbove = stemDown;

	yqpit = NoteYQPIT(mainNoteL) + halfLn2qd(midCHalfLn);					/* main notehead: qtr-lines below stftop */
	yqstem = d2qd(NoteYSTEM(mainNoteL), context.staffHeight, context.staffLines);	/* and its stem end in qtr-lines below stftop */

	hasMod = (NoteFIRSTMOD(mainNoteL)!=NILINK);

//say("yqpit=%d, yqpit%%4=%d, yqstem=%d, alwaysAbove=%d, alwaysBelow=%d, hasMod=%d\n",yqpit,yqpit%4,yqstem,alwaysAbove,alwaysBelow,hasMod);

	useStemEnd = FALSE;

	if (putModAbove) {
		if (!stemDown) {
			useStemEnd = TRUE;
			yqpit = yqstem;								/* substitute stem end pos. for note head */
		}
		if (canBeInStaff) {								/* allow mod placement in staff */
			if (yqpit>stfTopQtrLn) {					/* note/stem is in the staff */
				if (useStemEnd && odd(yqpit))			/* If yqpit is really stem end, it might not fall on a halfLn */
					yqpit++;
				modQPit = yqpit - 4;
				if (!(yqpit % 4))							/* note/stem is on a staff line */
					modQPit -= 2;
			}
			else {
				if (useStemEnd)
					modQPit = yqpit - 3;					/* 3 instead of 4; doesn't need as much room as w/ notehead */
				else
					modQPit = yqpit - 4;
			}
		}
		else {
			if (yqpit>stfTopQtrLn)						/* note/stem is inside staff */
				modQPit = stfTopQtrLn - 3;				/* 3 instead of 4; doesn't need as much room as when snug against notehead */
			else
				modQPit = yqpit - 4;
		}
		modQPit += yOffsetAbove;						/* offset from staccato position */
	}
	else {
		if (stemDown) {
			useStemEnd = TRUE;
			yqpit = yqstem;								/* substitute stem end pos. for note head */
		}
		if (canBeInStaff) {								/* allow mod placement in staff */
			if (yqpit<stfBotQtrLn) {					/* note/stem is in the staff */
				if (useStemEnd && odd(yqpit))			/* If yqpit is really stem end, it might not fall on a halfLn */
					yqpit--;
				modQPit = yqpit + 4;
				if (!(yqpit % 4))							/* note/stem is on a staff line */
					modQPit += 2;
			}
			else {
				if (useStemEnd)
					modQPit = yqpit + 3;					/* 3 instead of 4; doesn't need as much room as w/ notehead */
				else
					modQPit = yqpit + 4;
			}
		}
		else {
			if (yqpit<stfBotQtrLn)						/* note/stem is inside staff */
				modQPit = stfBotQtrLn + 3;				/* 3 instead of 4; doesn't need as much room as when snug against notehead */
			else
				modQPit = yqpit + 4;
		}
		modQPit += yOffsetBelow;						/* offset from staccato position */
	}

	/* Try to avoid a collision with an existing modifier.  This is very crude, and it only
		bothers with the first existing modifier. */
	if (hasMod) {
		LINK aModNRL = NoteFIRSTMOD(mainNoteL);
		INT16 existModQPit = std2qd(ModNRYSTDPIT(aModNRL));
		INT16 posDiff = abs(existModQPit - modQPit);
		if (posDiff < 4) {
			if (putModAbove)
				modQPit -= (4 - posDiff);
			else
				modQPit += (4 - posDiff);
		}
	}

	return modQPit;
}


/* -------------------------------------------------------------------- NewMODNR -- */
/*	Add a note/rest modifier. */

void NewMODNR(Document *doc,
				char	inchar,				/* Symbol code: symcode field from symTable */
				INT16	slashes,
				INT16	staff,
				INT16	qPitchLev,			/* "Pitch level", i.e., vertical position (qtr-lines) */
				LINK syncL,					/* Sync to which MODNR attached */
				LINK aNoteL
				)
{
	INT16			sym;
	PAMODNR		aModNR;
	LINK			aModNRL, lastModNRL;
	PANOTE		aNote;
	
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  				/* "Undo Insert" */

	/* Insert the current note modifier into the note's linked modifier list. 
		If the list exists, traverse to the end, & insert the new MODNR LINK
		into LastMODNR's next field, else into aNote->firstMod. */

	aNote = GetPANOTE(aNoteL);
	if (aNote->firstMod) {
		aModNRL = aNote->firstMod;
		for ( ; aModNRL; aModNRL = NextMODNRL(aModNRL))
			if (!NextMODNRL(aModNRL))
				lastModNRL = aModNRL;
		aModNRL = NextMODNRL(lastModNRL) = HeapAlloc(MODNRheap, 1);
		if (!aModNRL) MayErrMsg("NewMODNR: HeapAlloc failed.");
	}
	else {
		aNote->firstMod = HeapAlloc(MODNRheap, 1);
		if (!aNote->firstMod) MayErrMsg("NewMODNR: HeapAlloc failed.");
		aModNRL = aNote->firstMod;
	}

	aModNR = GetPAMODNR(aModNRL);
	
	sym = GetSymTableIndex(inchar);
	aModNR->selected = aModNR->soft = FALSE;
	aModNR->visible = TRUE;
	if (symtable[sym].subtype==MOD_TREMOLO1)
		aModNR->modCode = MOD_TREMOLO1+slashes-1;
	else
		aModNR->modCode = symtable[sym].subtype;
	aModNR->xstd = 0+XSTD_OFFSET;
	aModNR->ystdpit = qd2std(qPitchLev);
	aModNR->data = 0;
	doc->changed = TRUE;
	InvalMeasure(syncL, staff);										/* Redraw the measure */
	MEAdjustCaret(doc, TRUE);
}

/* ---------------------------------------------------- Functions for NewDynamic -- */

#ifdef NOTYET

/* Add a hairpin which spans two systems. x and endx are local x-coords which
must be put into correct coordinate system; pitchLev is vertical coordinate
relative to the staff in halfLns. */

static void AddXSysDynamic(Document *doc, INT16 x, INT16 endx, char inchar,
									INT16 staff, INT16 pitchLev, LINK lastSyncL)
{
	LINK newL1,newL2,sysL,measL,oldStartL; CONTEXT context;
	DDIST sysLeft; INT16 sym,endx1; PSYSTEM pSystem;
	
	/*
	 * Add the first piece of the crossSys dynamic.
	 */
	doc->currentSystem = firstDynSys;											/* firstDynSys and lastDynSys are set by GetCrossStatus called by TrackAndAddHairpin */
	newL1 = AddNewDynamic(doc, staff, x, &sysLeft, inchar,				/* return sysLeft, sym, and context */
										&sym, &context, TRUE);
	InitDynamic(doc, newL1, staff, x, sysLeft, pitchLev, &context);
	sysL = LSSearch(newL1, SYSTEMtype, ANYONE, TRUE, FALSE);
	pSystem = GetPSYSTEM(sysL);
	endx1 = pSystem->systemRect.right -
					std2d(STD_LINEHT, context.staffHeight, context.staffLines);
	SetupHairpin(newL1, staff, sysL, sysLeft, d2p(endx1), TRUE);		/* set endpt xds and crossSys flag */

	/*
	 * Set doc->currentSystem and doc->selStartL so that the second piece
	 * will be inserted on the correct system, at the correct insertion pt.
	 */
	doc->currentSystem = lastDynSys;
	measL = LSSearch(lastSyncL, MEASUREtype, staff, TRUE, FALSE);
	oldStartL = doc->selStartL;
	doc->selStartL = RightLINK(measL);

	/*
	 * Add the second piece of the cross system dynamic.
	 */
	newL2 = AddNewDynamic(doc, staff, d2p(LinkXD(measL)+sysLeft),
										&sysLeft, inchar, &sym, &context, TRUE);
	InitDynamic(doc, newL2, staff, d2p(LinkXD(measL)+sysLeft), sysLeft,
									pitchLev, &context);
	SetupHairpin(newL2, staff, lastSyncL, sysLeft, endx, TRUE);
	doc->selStartL = oldStartL;													/* restore doc->selStartL */
	doc->selEndL = RightLINK(doc->selStartL);									/* make selRange into an insertion pt */
	LinkSEL(newL2) = DynamicSEL(FirstSubLINK(newL2)) = FALSE;
	DynamFIRSTSYNC(newL2) = measL;
	
	doc->currentSystem = firstDynSys;											/* restore doc->currentSystem */
	FixContextForDynamic(doc, RightLINK(newL1), staff,
									context.dynamicType, symtable[sym].subtype);
	
	InvalObject(doc,newL1,FALSE);
	InvalObject(doc,newL2,FALSE);
	NewObjCleanup(doc, newL1, staff);
	InvalSystem(newL1); InvalSystem(newL2);
}

#endif

/* Call to InvalMeasures takes measL rather than lastSyncL as a parameter in an
effort to avoid passing one more parameter. */

LINK AddNewDynamic(Document *doc, INT16 staff, INT16 x, DDIST *sysLeft,
									char inchar, INT16 *sym, PCONTEXT pContext, Boolean crossSys)
{
	LINK sysL, measL, newL; PSYSTEM pSystem; PDYNAMIC newp; INT16 dtype;

	sysL = LSSearch(doc->headL, SYSTEMtype, doc->currentSystem, GO_RIGHT, FALSE);
	pSystem = GetPSYSTEM(sysL);
	*sysLeft = pSystem->systemRect.left;

	dtype = symtable[GetSymTableIndex(inchar)].subtype;
	if (dtype>=FIRSTHAIRPIN_DYNAM) {
		measL = LSSearch(doc->selStartL, MEASUREtype, staff, GO_RIGHT, FALSE);
		if (measL && !crossSys && SameSystem(measL, doc->selStartL))
			if (LinkXD(measL) < p2d(x)-*sysLeft) {
				GetIndCString(strBuf, INSERTERRS_STRS, 8);			/* "Hairpin cannot start in a different measure from its first note." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				InvalMeasures(doc->selStartL, measL, staff);			/* Force redrawing the measure */
				return NILINK;
		}
	}

	newL = NewObjPrepare(doc, DYNAMtype, sym, inchar, staff, x, pContext);
	newp = GetPDYNAMIC(newL);
	newp->firstSyncL = doc->selStartL;
	newp->dynamicType = symtable[*sym].subtype;

	return newL;
}

/* ------------------------------------------------------------------ NewDynamic -- */
/* Add a dynamic marking to the object list. */

void NewDynamic(
				Document *doc,
				INT16		x,						/* Horiz. position in pixels */
				INT16		endx,					/* For hairpins, horiz. right endpt in pixels */
				char		inchar,				/* Symbol code: symcode field from symtable */
				INT16		staff,				/* Staff number */
				INT16		pitchLev,			/* Vertical position in halflines */
				LINK  	lastSyncL,			/* For hairpins, sync right end is attached to */
				Boolean	crossSys				/* (Ignored for now) Whether cross system */
				)
{
	INT16	sym; LINK newL; CONTEXT context; DDIST sysLeft;
	PDYNAMIC newp;
	
#ifdef NOTYET
	if (crossSys) {
		AddXSysDynamic(doc, x, endx, inchar, staff, pitchLev, lastSyncL);
		return;
	}
#endif

	/* Insertion of crossSys dynamics is not currently undoable. */
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  						/* "Undo Insert" */

	newL = AddNewDynamic(doc, staff, x, &sysLeft, inchar, &sym, &context, FALSE);
	if (!newL) return;
	newp = GetPDYNAMIC(newL);
	newp->crossSys = FALSE;

	InitDynamic(doc, newL, staff, x, sysLeft, pitchLev, &context);
	SetupHairpin(newL, staff, lastSyncL, sysLeft, endx, crossSys);

	FixContextForDynamic(doc, RightLINK(newL), staff, context.dynamicType,
									symtable[sym].subtype);

	InvalObject(doc,newL,FALSE);
	NewObjCleanup(doc, newL, staff);
}

/* ------------------------------------------------------------------- NewRptEnd -- */
/*	Add a Repeat/End to the object list. */

void NewRptEnd(Document *doc,
					INT16	x,						/* Horizontal position (pixels) */
					char	inchar,				/* Input char. code for symbol to add */
					INT16	staff					/* Staff number */
					)
{
	INT16			sym;
	PRPTEND		newp;
	LINK			newL, pL, prevMeasL;
	CONTEXT		context;										/* current context */
	
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  					/* "Undo Insert" */

	/* ANYONE allows one subObj for each staff; assumes that there will likewise
		be one meas subObj for each staff for the immed. preceding meas. */
		
	newL = NewObjPrepare(doc, RPTENDtype, &sym, inchar, ANYONE, x, &context);
	
	prevMeasL = LSSearch(newL, MEASUREtype, ANYONE, TRUE, FALSE);
	InitRptEnd(newL, staff, symtable[sym].subtype, prevMeasL);
	
	newp = GetPRPTEND(newL);
	switch(newp->subType) {
		case RPT_L:
			pL = LSSearch(RightLINK(newL), RPTENDtype, ANYONE, FALSE, FALSE);
			if (pL) newp->endRpt = pL;
			else	  newp->endRpt = NILINK;
			newp->startRpt = NILINK;
			break;
		case RPT_R:
			pL = LSSearch(LeftLINK(newL), RPTENDtype, ANYONE, TRUE, FALSE);
			if (pL) newp->startRpt = pL;
			else	  newp->startRpt = NILINK;
			newp->endRpt = NILINK;
			break;
		case RPT_LR:
			pL = LSSearch(RightLINK(newL), RPTENDtype, ANYONE, FALSE, FALSE);
			if (pL) newp->endRpt = pL;
			else	  newp->endRpt = NILINK;

			pL = LSSearch(LeftLINK(newL), RPTENDtype, ANYONE, TRUE, FALSE);
			if (pL) newp->startRpt = pL;
			else	  newp->startRpt = NILINK;
			break;
		default:
			MayErrMsg("NewRptEnd: unsupported subType %ld for repeat/end %ld",
				(long)newp->subType, (long)newL);
			break;
	}

	NewObjCleanup(doc, newL, staff);
}

/* -------------------------------------------------------------------- NewEnding -- */
/*	Add an ending to the object list. */

void NewEnding(Document *doc, INT16 firstx, INT16 lastx, char inchar, INT16 clickStaff,
					STDIST pitchLev, LINK lastL, INT16 number, INT16 cutoffs)
{
	LINK pL,firstL; PENDING pEnding;
	CONTEXT context,firstContext,lastContext;
	INT16 sym;
	DDIST firstRelxd,lastRelxd;
	
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  						/* "Undo Insert" */
	pL = NewObjPrepare(doc, ENDINGtype, &sym, inchar, clickStaff, firstx, &context);

	firstL = doc->selStartL;
	GetContext(doc, firstL, 1, &firstContext);
	firstRelxd = MeasureTYPE(firstL) ? 0 : LinkXD(firstL);

	pEnding = GetPENDING(pL);
	pEnding->xd = p2d(firstx)-firstContext.measureLeft;
	pEnding->xd -= firstRelxd;

	GetContext(doc, lastL, 1, &lastContext);
	lastRelxd = MeasureTYPE(lastL) ? 0 : LinkXD(lastL);

	pEnding = GetPENDING(pL);
	pEnding->endxd = p2d(lastx)-lastContext.measureLeft;
	pEnding->endxd -= lastRelxd;

	pEnding->staffn = clickStaff;
	pEnding->yd = halfLn2d(pitchLev, context.staffHeight, context.staffLines);
	pEnding->firstObjL = doc->selStartL;
	pEnding->lastObjL = lastL;
	pEnding->noLCutoff = (cutoffs & 2) >> 1;
	pEnding->noRCutoff = (cutoffs & 1);
	pEnding->endNum = number;

	NewObjCleanup(doc, pL, clickStaff);
	
	/* NewObjCleanup is not cleaning up enough: finish the job. */

	InvalMeasures(firstL,lastL,ANYONE);
}

/* -------------------------------------------------------------------- NewTempo -- */
/*	Add a Tempo mark to the object list. */

void NewTempo(Document *doc, Point pt, char inchar, INT16 staff, STDIST pitchLev,
					Boolean hideMM, INT16 dur, Boolean dotted,
					unsigned char *tempoStr, unsigned char *metroStr)
{
	LINK pL;
	PTEMPO pTempo;
	CONTEXT context; INT16 sym; long beatsPM;
	
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  			/* "Undo Insert" */
	pL = NewObjPrepare(doc, TEMPOtype, &sym, inchar, staff, pt.h, &context);

PushLock(OBJheap);
	pTempo = GetPTEMPO(pL);
	pTempo->staffn = staff;
	pTempo->small = 0;
	pTempo->yd = halfLn2d(pitchLev, context.staffHeight, context.staffLines);
	pTempo->xd = 0;														/* same horiz position as note */
	pTempo->subType = dur;												/* beat: same units as note's l_dur */
	pTempo->dotted = dotted;
	pTempo->hideMM = hideMM;
	pTempo->string = PStore((unsigned char *)tempoStr);		/* index return by String Manager */
	pTempo->metroStr = PStore((unsigned char *)metroStr); 	/* index return by String Manager */
	if (pTempo->string<0L || pTempo->metroStr<0L)
		NoMoreMemory();
	else if (pTempo->string>GetHandleSize((Handle)doc->stringPool)
		  || pTempo->metroStr>GetHandleSize((Handle)doc->stringPool))
		MayErrMsg("NewTempo: PStore error. string=%ld metroStr=%ld",
					pTempo->string, pTempo->metroStr);

	beatsPM = FindIntInString(metroStr);
	if (beatsPM<0L) beatsPM = config.defaultTempo;
	pTempo->tempo = beatsPM;
	pTempo->firstObjL = doc->selStartL;							/* tempo inserted relative to this obj */
PopLock(OBJheap);

	InvalObject(doc,pL,FALSE);
	NewObjCleanup(doc, pL, staff);
}


/* -------------------------------------------------------------------- NewSpace -- */
/*	Add a Space mark to the object list. */

void NewSpace(Document *doc, Point pt, char inchar, INT16 topStaff, INT16 bottomStaff,
					STDIST stdSpace)
{
	LINK pL; PSPACE pSpace; CONTEXT context; INT16 sym;
	
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);  				/* "Undo Insert" */
	pL = NewObjPrepare(doc, SPACEtype, &sym, inchar, topStaff, pt.h, &context);

	FirstSubLINK(pL) = NILINK;
	pSpace = GetPSPACE(pL);
	pSpace->spWidth = stdSpace;						/* Amount of space to leave in STDISTs */
	pSpace->staffn = topStaff;
	pSpace->bottomStaff = bottomStaff;

	NewObjCleanup(doc, pL, topStaff);
}
