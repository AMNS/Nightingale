/***************************************************************************
FILE:	Multivoice.c
PROJ:	Nightingale
DESC:	Routines to handle multivoice notation rules.
	DimItem,UserDimItem
	MultivoiceDialog			FindMultivoice				RebeamList
	SetNRCMultivoiceRole		FixAugDots					SetSelMultivoiceRules
	VoiceInSelection			DoMultivoiceRules			UseMultivoiceRules
	TryMultivoiceRules			SetSelMultivoiceRules		DoMultivoiceRules
	UseMultivoiceRules			TryMultivoiceRules
/***************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static pascal void UserDimItem(DialogPtr, short);
static void DimItem(DialogPtr, short);
static void FindMultivoice(Document *, LINK, LINK, Boolean []);
static void RebeamList(Document *, LINK [], short, short);
static void SetNRCMultivoiceRole(Document *, LINK, LINK, CONTEXT, short);
static void FixAugDots(Document *, LINK, short, CONTEXT, short);
static void SetSelMultivoiceRules(Document *, LINK, LINK, short, CONTEXT, short);
static LINK VoiceInSelection(Document *, short);

/* --------------------------------------------------------- DimItem, UserDimItem -- */
/* If checkbox is disabled, dim the static text field that is the bottom line of
its label.  Otherwise, do nothing. The userItems' handle should be set to be a
pointer to this procedure so it'll be called automatically by the dialog filter. */
		
static void DimItem(register DialogPtr dlog, short item)
{
	short			aShort;
	Handle		assumeHdl, aHdl;
	Rect			aRect, box;
	Boolean		measMulti;
	
	GetDialogItem(dlog, MVASSUME_DI, &aShort, &assumeHdl, &aRect);
	measMulti = GetDlgChkRadio(dlog, ONLY2V_DI);
	if (!measMulti) {
		HiliteControl((ControlHandle)assumeHdl, CTL_ACTIVE);
		ShowDialogItem(dlog, MVASSUME_BOTTOM_DI);
	}
	else {
		HiliteControl((ControlHandle)assumeHdl, CTL_INACTIVE);
		PenPat(NGetQDGlobalsGray());	
		PenMode(patBic);	
		GetDialogItem(dlog, item, &aShort, &aHdl, &box);
		PaintRect(&box);										/* Dim everything within userItem rect */
		PenNormal();
	}
}


static pascal void UserDimItem(DialogPtr d, short dItem)
{
	DimItem(d, dItem);
}


/* ------------------------------------------------------------ MultivoiceDialog -- */
/* Find out whether user wants to apply upper, lower, or single voice
notation rules to the selection. Delivers TRUE if OKed, FALSE if Cancelled. */

#define LAST_RBUT SINGLE_DI

Boolean MultivoiceDialog(
				short nSelVoices,
				short	*voiceRole,			/* Returns UPPER_DI,LOWER_DI,CROSS_DI or SINGLE_DI */
				Boolean *measMulti,		/* Returns TRUE=only if measure has >=2 voices */
				Boolean *assume 			/* Returns TRUE=assume this voiceRole from now on */
				)
{
	DialogPtr	dialogp;
	short			ditem, aShort;
	short			radio, makeMulti;
	Handle		only2vHdl, assumeHdl, aHdl;
	Rect			aRect, assBotRect;
	GrafPtr		oldPort;
	char			fmtStr[32];
	ModalFilterUPP filterUPP;
	UserItemUPP userUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	userUPP = NewUserItemUPP(UserDimItem);
	if (userUPP==NULL) {
		if (filterUPP) DisposeModalFilterUPP(filterUPP);
		SysBeep(1);
		return FALSE;
	}
	

	GetPort(&oldPort);
	dialogp = GetNewDialog(MULTIVOICE_DLOG, NULL, BRING_TO_FRONT);
	if (dialogp) {
		SetPort(GetDialogWindowPort(dialogp));
		if (nSelVoices==1) {
			GetIndCString(strBuf, DIALOG_STRS, 11);							/* "selected voice" */
			CParamText(strBuf, "", "", "");
		}
		else {
			GetIndCString(fmtStr, DIALOG_STRS, 12);							/* "%d selected voices" */
			sprintf(strBuf, fmtStr, nSelVoices);
			CParamText(strBuf, "", "", "");
		}
		
		radio = (*voiceRole<UPPER_DI || *voiceRole>LAST_RBUT) ? UPPER_DI : *voiceRole;
		PutDlgChkRadio(dialogp, radio, TRUE);
		GetDialogItem(dialogp, ONLY2V_DI, &aShort, (Handle *)&only2vHdl, &aRect);
		GetDialogItem(dialogp, MVASSUME_DI, &aShort,(Handle *) &assumeHdl, &aRect);
		SetControlValue((ControlHandle)only2vHdl, (*measMulti==0 ? 0 : 1));
		if (*measMulti==1)
			SetControlValue((ControlHandle)assumeHdl, 0);
		else
			SetControlValue((ControlHandle)assumeHdl, !GetControlValue((ControlHandle)only2vHdl));
		GetDialogItem(dialogp, MVASSUME_BOTTOM_DI, &aShort, &aHdl, &assBotRect);
		SetDialogItem(dialogp, MVASSUME_BOTTOM_DI, 0, (Handle)userUPP, &assBotRect);
		
		CenterWindow(GetDialogWindow(dialogp), 70);
		ShowWindow(GetDialogWindow(dialogp));
		ArrowCursor();

		while (1) {
			ModalDialog(filterUPP, &ditem);
			if (ditem==OK) {
				*voiceRole = radio;
				*measMulti = (GetControlValue((ControlHandle)only2vHdl)!=0);
				*assume = (GetControlValue((ControlHandle)assumeHdl)!=0);
				break;
				}
			if (ditem==Cancel) break;
			
			switch (ditem) {
				case UPPER_DI:
				case LOWER_DI:
				case CROSS_DI:
				case SINGLE_DI:
					if (radio != ditem) {
						SwitchRadio(dialogp, &radio, ditem);
						if (ditem==SINGLE_DI) SetControlValue((ControlHandle)only2vHdl, 0);
						HiliteControl((ControlHandle)only2vHdl,
										ditem==SINGLE_DI ? CTL_INACTIVE : CTL_ACTIVE);
						}
					break;
				case ONLY2V_DI:
					makeMulti = 1-GetControlValue((ControlHandle)only2vHdl);
	  				SetControlValue((ControlHandle)only2vHdl, makeMulti);
	  				
	  				/* Dim or un-dim the "Assume from now on" checkbox accordingly */
					if (makeMulti) SetControlValue((ControlHandle)assumeHdl, 0);
					InvalWindowRect(GetDialogWindow(dialogp),&assBotRect);
					DimItem(dialogp, MVASSUME_BOTTOM_DI);
					break;
				case MVASSUME_DI:
	  				SetControlValue((ControlHandle)assumeHdl,!GetControlValue((ControlHandle)assumeHdl));
					break;
				}
			}
		DisposeDialog(dialogp);											/* Free heap space */
		}
	else {
		MissingDialog(MULTIVOICE_DLOG);
		ditem = Cancel;
		}
	
	DisposeModalFilterUPP(filterUPP);
	DisposeUserItemUPP(userUPP);
	SetPort(oldPort);
	return (ditem==OK);
}


/* -------------------------------------------------------------- FindMultivoice -- */
/* Find staves in the given measure that contain any notes/rests for more than
one voice. */

static void FindMultivoice(Document *doc, LINK barFirstL, LINK barLastL,
									Boolean multivoice[])
{
	LINK		pL, aNoteL;
	PANOTE	aNote;
	short 	s,
				voiceFound[MAXSTAVES+1];
			
	for (s = 0; s<=doc->nstaves; s++) {
		voiceFound[s] = NOONE;								/* No voices found yet */
		multivoice[s] = FALSE;								/* No staff has multiple v's yet */
	}
	
	for (pL = barFirstL; pL!=barLastL; pL = RightLINK(pL))
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				if (voiceFound[aNote->staffn]==NOONE)
					voiceFound[aNote->staffn] = aNote->voice;
				else if (voiceFound[aNote->staffn]!=aNote->voice)
					multivoice[aNote->staffn] = TRUE;
			}
		}
}


/* ------------------------------------------------------------------ RebeamList -- */
/* Unbeam and rebeam each of the entries in beamLA[]. ??CreateBEAMSET can fail,
at least from lack of memory: we should give up if it does! */

static void RebeamList(
		Document *doc,
		LINK beamLA[],
		short nBeamsets,
		short voiceRole		/* UPPER_DI, LOWER_DI, CROSS_DI, or SINGLE_DI */
		)
{
	short i, nEntries;
	LINK startL, endL;
	
	for (i = 0; i<nBeamsets; i++) {
		startL = FirstInBeam(beamLA[i]);
		endL = LastInBeam(beamLA[i]);
		nEntries = LinkNENTRIES(beamLA[i]);
		UnbeamV(doc, startL, endL, BeamVOICE(beamLA[i]));
		(void)CreateBEAMSET(doc, startL, RightLINK(endL), BeamVOICE(beamLA[i]),
							nEntries, TRUE, voiceRole);
	}
	
	UpdateSelection(doc);
}


/* ------------------------------------------------------- GetRestMultivoiceRole -- */

QDIST GetRestMultivoiceRole(PCONTEXT pContext, short voiceRole, Boolean makeLower)
{
	short halfSpPos;

	if (voiceRole==SINGLE_DI)
		halfSpPos = (pContext->staffLines-1);
	else
		halfSpPos = (makeLower? pContext->staffLines-1+config.restMVOffset
									 : pContext->staffLines-1-config.restMVOffset);

	return halfLn2qd(halfSpPos);
}


/* -------------------------------------------------------- SetNRCMultivoiceRole -- */
/* Set the given note/rest/chord's vertical position (for rests) or stem vertical
position (for notes and chords) according to multi-voice notation rules for the
given voice position. Does nothing for beamed chords; the calling function must
handle them. */

static void SetNRCMultivoiceRole(
		Document *doc,
		LINK		syncL,
		LINK		aNoteL,
		CONTEXT	context,			/* current staff context */
		short		voiceRole		/* UPPER_DI, LOWER_DI, CROSS_DI, or SINGLE_DI */
		)
{
	register PANOTE aNote;
	QDIST		yqpit;
	short		staff, midCHalfLn, halfLn, stemLen;
	DDIST		tempystem;
	LINK		partL;
	PPARTINFO	pPart;
	Boolean	makeLower;
	
	PushLock(NOTEheap);
	
	aNote = GetPANOTE(aNoteL);
	staff = aNote->staffn;
	
	switch (voiceRole) {
		case SINGLE_DI:
			if (!aNote->rest) {
				midCHalfLn = ClefMiddleCHalfLn(context.clefType);	/* Get middle C staff pos. */		
				yqpit = aNote->yqpit+halfLn2qd(midCHalfLn);
				halfLn = qd2halfLn(yqpit);									/* Number of half lines from stftop */
				makeLower = (halfLn<=context.staffLines-1);			/* ??WRONG--CONSIDER WHOLE CHORD! */
			}
			break;
		case CROSS_DI:
			partL = FindPartInfo(doc, Staff2Part(doc, staff));
			pPart = GetPPARTINFO(partL);
			makeLower = (staff==pPart->firstStaff);
			break;
		default:
			makeLower = (voiceRole==LOWER_DI);
	}
	
	if (aNote->rest) {
		yqpit = GetRestMultivoiceRole(&context, voiceRole, makeLower);
		aNote->yd = qd2d(yqpit, context.staffHeight,
								context.staffLines);
		LinkVALID(syncL) = FALSE;											/* Rest's sel. area may have moved */
	}
	else if (aNote->inChord)
		FixSyncForChord(doc, syncL, aNote->voice, aNote->beamed,
								(makeLower? -1 : 1),
								(voiceRole==SINGLE_DI? 1 : -1), NULL );
	else {									/* ??CALL FixSyncForNoChord? */
		stemLen = QSTEMLEN(voiceRole==SINGLE_DI,
								 ShortenStem(aNoteL, context, makeLower));
		tempystem = CalcYStem(doc, aNote->yd, NFLAGS(aNote->subType),
										makeLower,
										context.staffHeight, context.staffLines,
										stemLen, FALSE);
		aNote->ystem = tempystem;
	}
	
	PopLock(NOTEheap);
}


/* ------------------------------------------------------------------ FixAugDots -- */
/* Set the augmentation dot vertical position. If the main note is selected, and any
of the notes in the given Sync and staff are on lines (rather than in spaces): In
multivoice notation, put their augmentation dots in the space toward the end of the
stem; in single-voice notation, put their aug. dots in the space above the notehead.

N.B. It would be very reasonable also to move aug. dots on upstemmed notes with flags
to the right; that should be done in this function (as well as in SetupNote!).

Cf. FixAugDotPos: perhaps they should be combined somehow. */

void FixAugDots(
		Document *doc,
		LINK		syncL,
		short		staff,
		CONTEXT	/*context*/,
		short		voiceRole	/* UPPER_DI, LOWER_DI, CROSS_DI, or SINGLE_DI */
		)
{
	LINK mainNoteL, aNoteL; short v, halfLn; PANOTE aNote; Boolean stemDown;
	
	for (v = 1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {
			mainNoteL = FindMainNote(syncL, v);
			if (mainNoteL && NoteSTAFF(mainNoteL)==staff && NoteSEL(mainNoteL)) {
				stemDown = (NoteYD(mainNoteL)<NoteYSTEM(mainNoteL));
				aNoteL = FirstSubLINK(syncL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					if (NoteVOICE(aNoteL)==v) {
						aNote = GetPANOTE(aNoteL);
						halfLn = qd2halfLn(aNote->yqpit);					/* Half-lines below middle C */
						if (halfLn%2==0)
							NoteYMOVEDOTS(aNoteL) = GetLineAugDotPos(voiceRole, stemDown);
					}
				}
			}
		}
}


/* ------------------------------------------------------- SetSelMultivoiceRules -- */
/* Set selected notes/rests/chords in the given measure on the given staff (NOT voice)
to follow multi-voice notation rules for the given voice position. N.B. If notes
in more than one voice are selected on that staff, is likely to mess up. */

static void SetSelMultivoiceRules(
		Document *doc,
		LINK		barFirstL, LINK barLastL,
		short		staff,
		CONTEXT	context,				/* current staff context */
		short		voiceRole			/* UPPER_DI, LOWER_DI, CROSS_DI, or SINGLE_DI */
		)
{
	LINK		pL, aNoteL, startL, endL, owner;
	short		i, nBeamsets=0;
	Boolean	found;
	LINK		beamLA[MAX_MEASNODES];	/* Enough for any reasonable but not any possible situation */
			
	for (pL = barFirstL; pL!=barLastL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				if (NoteSTAFF(aNoteL)==staff && NoteSEL(aNoteL)) {
					if (MainNote(aNoteL)) {											/* Skip stemless notes in chords */

						SetNRCMultivoiceRole(doc, pL, aNoteL, context, voiceRole);
													
						/*
						 *	If this note is beamed, we can't set its stem correctly now,
						 * since other notes in the beamset may follow, but remember
						 * its owning beamset for later use; if we run out of space to
						 *	remember it, just unbeam so at least we won't end up with
						 *	stems and beams that disagree. ??N.B. If the note is beamed,
						 *	the code above to set its stem position looks like it's
						 * completely ineffective and should be skipped.
						 */
						if (NoteBEAMED(aNoteL)) {
							owner = LVSearch(pL, BEAMSETtype, NoteVOICE(aNoteL), GO_LEFT, FALSE);
							startL = FirstInBeam(owner);
							endL = LastInBeam(owner);
							if (nBeamsets<MAX_MEASNODES) {
								for (found = FALSE, i = 0; i<nBeamsets; i++)
									if (owner==beamLA[i]) found = TRUE;
								if (!found)
									beamLA[nBeamsets++] = owner;
							}
							else
								UnbeamV(doc, startL, endL, NoteVOICE(aNoteL));
						}
					}
				}
			}
		}
		
	RebeamList(doc, beamLA, nBeamsets, voiceRole);
		
	for (pL = barFirstL; pL!=barLastL; pL = RightLINK(pL))
		if (SyncTYPE(pL))
			FixAugDots(doc, pL, staff, context, voiceRole);

	InvalMeasure(barFirstL, staff);
	doc->changed = TRUE;
}


/* ------------------------------------------------------------ VoiceInSelection -- */
/* Return the first Sync or GRSync in the selection range that uses the given voice;
return NILINK if there is none. */

LINK VoiceInSelection(Document *doc, short voice)
{
	LINK pL, aNoteL, aGRNoteL;
	
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteSEL(aNoteL) && NoteVOICE(aNoteL)==voice) return pL;
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
					if (GRNoteSEL(aGRNoteL) && GRNoteVOICE(aGRNoteL)==voice) return pL;
				break;
			default:
				;
		}
	}
	
	return NILINK;
}


/* ----------------------------------------------------------- DoMultivoiceRules -- */
/* Apply "multi-voice notation rules" to the selection: point stems in opposite
directions, move rests away from the staff center, move slurs/ties to the outside,
etc. etc. */

void DoMultivoiceRules(
		Document *doc,
		short		voiceRole,	/* UPPER_DI, LOWER_DI, CROSS_DI, or SINGLE_DI */
		Boolean	measMulti,	/* TRUE=do nothing if measure has only 1 voice */
		Boolean	assume 		/* TRUE=make this voiceRole default for selected voices */
		)
{
	LINK		startBarL, endBarL,
				barFirstL, barLastL;
	Boolean	done, multivoice[MAXSTAVES+1];
	short		s, v;
	CONTEXT	context[MAXSTAVES+1];				/* current staff context table */

	WaitCursor();
	PrepareUndo(doc, doc->selStartL, U_MultiVoice, 30);		/* "Undo Multivoice Notation" */
	
 /* Start at previous Measure; if there isn't one, start at first Measure */
	startBarL = LSSearch(LeftLINK(doc->selStartL), MEASUREtype, 1, GO_LEFT, FALSE);
	if (!startBarL)
		startBarL = LSSearch(doc->headL, MEASUREtype, 1, GO_RIGHT, FALSE);
	endBarL = EndMeasSearch(doc, LeftLINK(doc->selEndL));				/* Stop at end of Measure */

	GetAllContexts(doc, context, startBarL);
	done = FALSE;
	barFirstL = startBarL;
	while (barFirstL!=NILINK && !done) {
		barLastL = EndMeasSearch(doc, barFirstL);
		if (!LinkBefFirstMeas(barFirstL)) {
			if (measMulti) FindMultivoice(doc, barFirstL, barLastL, multivoice);
			for (s = 1; s<=doc->nstaves; s++) {
				if (!measMulti || multivoice[s])
					SetSelMultivoiceRules(doc, barFirstL, barLastL, s, context[s], voiceRole);
			}
		}
		if (barLastL==endBarL) done = TRUE;
		barFirstL = barLastL;									/* Skip to next MEASURE obj */		
	}
	
	if (assume)
		for (v = 1; v<=MAXVOICES; v++)
			if (VOICE_MAYBE_USED(doc, v)) {
				if (VoiceInSelection(doc, v))
					doc->voiceTab[v].voiceRole = voiceRole;
			}
}

