/* Double.c for Nightingale: revised for v.3.1 */

/*										NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALEÈ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-97 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static pascal Boolean DoubleFilter(DialogPtr, EventRecord *, short *);
static Boolean DoubleDialog(Document *, short, short *);
static Boolean DblSrcStaffOK(LINK, LINK, short);
static Boolean DblDstStaffOK(LINK, LINK, short);
static void FixNoteYForClef(Document *, LINK, LINK, short, CONTEXT, CONTEXT);
static void FixChordsForClef(Document *, LINK, short, CONTEXT, CONTEXT);
static short DupAndSetStaff(Document *, short [], LINK, short, short, short);
static Boolean StfHasSmthgAcross(Document *, LINK, short, char []);
static Boolean RangeHasUnmatchedSlurs(Document *, LINK, LINK, short);
static LINK LSContextDynamicSearch(LINK, short, Boolean, Boolean);
static void DblFixContext(Document *, short);
static Boolean IsDoubleOK(Document *, short, short);

/* =========================================================== DoubleDialog, etc. == */

static enum
{
	ORIGPART_DI=5,
	ORIGSTAFFN_DI=7,
	PART_POPUP_DI=9,
	STAFFN_DI=11
} E_DoubleItems;

static UserPopUp popupPart;

static Boolean hilitedItem = FALSE;  	/* Is an item currently hilited by using arrow keys? */


static pascal Boolean DoubleFilter(DialogPtr dlog, EventRecord *evt, short *itemHit)
{
	Point where;
	Boolean ans=FALSE; WindowPtr w;
	short ch;

	w = (WindowPtr)(evt->message);
	switch(evt->what) {
		case updateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetWindowPort(w));
				BeginUpdate(GetDialogWindow(dlog));
				UpdateDialogVisRgn(dlog);
				DrawPopUp(&popupPart);
				FrameDefault(dlog, OK, TRUE);
				EndUpdate(GetDialogWindow(dlog));
				ans = TRUE; *itemHit = 0;
			}
			break;
		case activateEvt:
			if (w == GetDialogWindow(dlog)) {
				SetPort(GetWindowPort(w));
			}
			break;
		case mouseDown:
		case mouseUp:
			where = evt->where;
			GlobalToLocal(&where);
			if (PtInRect(where,&popupPart.shadow)) {
				*itemHit = DoUserPopUp(&popupPart) ? PART_POPUP_DI : 0;
				hilitedItem = FALSE;
				HilitePopUp(&popupPart, FALSE);
				ans = TRUE;
			}
			break;
		case autoKey:
		case keyDown:
			if (DlgCmdKey(dlog, evt, (short *)itemHit, FALSE))
				return TRUE;
			else {
				ch = (unsigned char)evt->message;
				switch (ch) {
					case UPARROWKEY:
					case DOWNARROWKEY:
						HiliteArrowKey(dlog, ch, &popupPart, &hilitedItem);
						ans = TRUE;
						break;						
				}
			}
			break;
		}
		
	return(ans);
}


static Boolean DoubleDialog(Document *doc,
						short srcStf,		/* Staff no. containing material to be doubled */
						short *pDstStf		/* Destination staff no. */
						)
{
	DialogPtr		dlog;
	short			ditem = Cancel, staffn, partChoice, nPart, partMaxStf;
	GrafPtr			oldPort;
	Boolean			keepGoing = TRUE;
	LINK			thePartL, partL;
	PPARTINFO		pPart;
	char			partName[256], fmtStr[256];	
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(DoubleFilter);
	if (filterUPP == NULL) {
		MissingDialog(DOUBLE_DLOG);
		return FALSE;
	}

	GetPort(&oldPort);
	dlog = GetNewDialog(DOUBLE_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(DOUBLE_DLOG);
		return FALSE;
	}
	SetPort(GetDialogWindowPort(dlog));

	/*
	 * Fill in the dialog's fields, including a pop-up menu listing all the parts
	 *	in the score to let the user choose the destination part.
	 */ 
	thePartL = Staff2PartL(doc, doc->headL, srcStf);
	staffn = srcStf-PartFirstSTAFF(thePartL)+1;
	PutDlgWord(dlog, ORIGSTAFFN_DI, staffn, TRUE);
	partL = FirstSubLINK(doc->headL);
	partChoice = nPart = 1;
	for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL), nPart++) {
		if (partL==thePartL) {
			pPart = GetPPARTINFO(partL);
			strcpy(partName, pPart->name);
			CtoPstr((StringPtr)partName);
			PutDlgString(dlog, ORIGPART_DI, (unsigned char *)partName, FALSE);
		}
	}
	thePartL = Staff2PartL(doc, doc->headL, *pDstStf);
	staffn = *pDstStf-PartFirstSTAFF(thePartL)+1;
	PutDlgWord(dlog, STAFFN_DI, staffn, TRUE);
	
	if (!InitPopUp(dlog, &popupPart, PART_POPUP_DI, 0, PARTS_MENU, 0))
			goto Done;
	partL = FirstSubLINK(doc->headL);
	partChoice = nPart = 1;
	for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL), nPart++) {
		if (partL==thePartL) partChoice = nPart;
		pPart = GetPPARTINFO(partL);
		strcpy(partName, pPart->name);
		AppendCMenu(popupPart.menu, partName);
	}
	ChangePopUpChoice(&popupPart, partChoice);
	hilitedItem = FALSE;

	CenterWindow(GetDialogWindow(dlog), 60);
	ShowWindow(GetDialogWindow(dlog));
	ArrowCursor();

	while (keepGoing) {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
			case Cancel:
				keepGoing = FALSE;
				break;
		}
	if (ditem==OK) {
			nPart = popupPart.currentChoice;
			partL = FindPartInfo(doc, nPart);
			GetDlgWord(dlog, STAFFN_DI, &staffn);
			partMaxStf = PartLastSTAFF(partL)-PartFirstSTAFF(partL)+1;
			if (staffn<1  || staffn>partMaxStf) {
				if (staffn<1) {
					GetIndCString(strBuf, DOUBLE_STRS, 1);			/* "staff no. can't be zero/negative" */
					CParamText(strBuf, "", "", "");
				}
				else {
					if (partMaxStf==1)
						GetIndCString(fmtStr, DOUBLE_STRS, 2);		/* "part includes only one staff" */
					else
						GetIndCString(fmtStr, DOUBLE_STRS, 3);		/* "part includes only %d staves" */
					sprintf(strBuf, fmtStr, partMaxStf);
					CParamText(strBuf, "", "", "");
				}
				StopInform(GENERIC_ALRT);
				keepGoing = TRUE;
			}
		}
	}
		
	*pDstStf = PartFirstSTAFF(partL)+staffn-1;

Done:
	DisposePopUp(&popupPart);
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);	
	SetPort(oldPort);
	return (ditem==OK);
}


/* ------------------------------------------------------------- DblSrcStaffOK -- */
/*  If the given staff and range is OK as a source for the Double command
(meaning there are no clefs or keysigs. on the given staff in the given range:
fixing context for these is more difficult than I want to bother with now),
return TRUE. */

static Boolean DblSrcStaffOK(LINK startL, LINK endL, short staffn)
{
	LINK pL;
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case CLEFtype:
				if (ClefOnStaff(pL, staffn))
					if (ClefINMEAS(pL)) return FALSE;			/* Ignore clefs in gutter */
				break;
			case KEYSIGtype:
				if (KeySigOnStaff(pL, staffn))
					if (KeySigINMEAS(pL)) return FALSE;			/* Ignore keysigs in gutter */
				break;
			default:
				;
		}
	}
		
	return TRUE;
}

/* ------------------------------------------------------------- DblDstStaffOK -- */
/*  If the given staff and range is OK as a destination for the Double command
(meaning there are no "content" objects on the given staff in the given range),
return TRUE. If the staff is the other staff of cross-staff objects, don't worry
about it.

We use the term "content" here in a rather specialized sense: time signature
changes don't count, since they'll normally agree with those on the staff we're
Doubling from (and even if they don't, no disasters will result).

Cf. SFStaffNonempty in SFormatHighLevel.c. Like there, it'd be good eventually
to differentiate between notes and rests: for Double, we'd optionally remove any
rests found, instead of refusing to proceed if any are found. Not yet, though. */

static Boolean DblDstStaffOK(LINK startL, LINK endL, short staffn)
{
	LINK pL;
		
	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
			case GRSYNCtype:
			case DYNAMtype:
			case SLURtype:
			case BEAMSETtype:
			case TUPLETtype:
			case OCTAVAtype:
			case GRAPHICtype:
			case TEMPOtype:
			case SPACEtype:
			case ENDINGtype:
				if (ObjOnStaff(pL, staffn, FALSE)) return FALSE;
				break;
			case CLEFtype:
				if (ClefOnStaff(pL, staffn))
					if (ClefINMEAS(pL)) return FALSE;			/* Ignore clefs in gutter */
				break;
			case KEYSIGtype:
				if (KeySigOnStaff(pL, staffn))
					if (KeySigINMEAS(pL)) return FALSE;			/* Ignore keysigs in gutter */
				break;
			/* Ignore TIMESIGtype (see comments above) */
			default:
				;
		}
	}
	
	return TRUE;
}


/* =================================================== Voice-remapping functions == */
/* Utilities for voice remapping, based on those for Past Insert and Paste Merge. */

static void DblMapVoices(Document *, short [], LINK, short);
short AssignNewVoice(Document *, LINK);
short DblNewVoice(Document *, short [], short, short, Boolean);

/* ------------------------------------------------------------------------------- */

/* Add a new voice number in the given part. Find the lowest user voice number
that's higher than any used in the part, add an entry for it to <doc->voiceTab>,
and return the equivalent internal voice number. */

short AssignNewVoice(Document *doc, LINK partL)
{
	short partn; short maxUVUsed, v, iVoice;
	
	partn = PartL2Partn(doc,partL);
	maxUVUsed = -1;
	for (v = 1; v<=MAXVOICES; v++)
		if (doc->voiceTab[v].partn==partn)
			maxUVUsed = n_max(maxUVUsed, doc->voiceTab[v].relVoice);
	iVoice = User2IntVoice(doc, maxUVUsed+1, partL);
	 
	return iVoice;
}

/* Given a voice-remapping table, destination staff, and internal voice number
belonging to the SOURCE staff, return an equivalent internal voice number for the
destination staff. */

short DblNewVoice(Document *doc, short vMap[],
						short srcIV, short dstStf,
						Boolean sameRel		/* TRUE=srcStf and dstStf are same staffn rel. to their parts */
						)
{
	LINK srcPartL, dstPartL; short uV;
	
	/* If the voice no. has already been mapped, use the mapping. */
	
	if (vMap[srcIV]!=NOONE) return vMap[srcIV];

	/* ...otherwise, if staves are the same rel. to their parts, use the same user voice. */
	
	dstPartL = Staff2PartL(doc, doc->headL, dstStf);

	if (sameRel) {
		Int2UserVoice(doc, srcIV, &uV, &srcPartL);
		return User2IntVoice(doc, uV, dstPartL);
	}
	
	/* ...otherwise, assign the next unused user voice for the part. */
	
	return AssignNewVoice(doc, dstPartL);
}

/* Make a copy of the score's voice-mapping table. If Double adds any previously-
unused voices on the destination staff, they can have entries added later, or they
could be handled entirely "on the fly". */

/* -------------------------------------------------------------------------------- */

void DblSetNewVoices(Document *doc, short vMap[], LINK pL, short srcStf, short dstStf,
							Boolean sameRel);

/* Set up a mapping for any voices in which pL may participate, either the object
	or the subobj: from that sub/object's original voice v to the new voice contained
	in position v of the table. */

void DblSetNewVoices(Document *doc, short vMap[], LINK pL,
							short srcStf, short dstStf,
							Boolean sameRel	/* TRUE=srcStf and dstStf are same staffn rel. to their parts */
							)
{
	LINK aNoteL,aGRNoteL; short voice;

	switch (ObjLType(pL)) {
		case SYNCtype:
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteSTAFF(aNoteL)==srcStf && vMap[NoteVOICE(aNoteL)]==NOONE) {
					voice = DblNewVoice(doc, vMap, NoteVOICE(aNoteL), dstStf, sameRel);
					vMap[NoteVOICE(aNoteL)] = voice;
				}
			}
			break;
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				if (GRNoteSTAFF(aGRNoteL)==srcStf && vMap[GRNoteVOICE(aGRNoteL)]==NOONE) {
					voice = DblNewVoice(doc, vMap, GRNoteVOICE(aGRNoteL), dstStf, sameRel);
					vMap[GRNoteVOICE(aGRNoteL)] = voice;
				}
			}
			break;
		case SLURtype:
			if (SlurSTAFF(pL)==srcStf && vMap[SlurVOICE(pL)]==NOONE) {
				voice = DblNewVoice(doc, vMap, SlurVOICE(pL), dstStf, sameRel);
				vMap[SlurVOICE(pL)] = voice;
			}
			break;
		case BEAMSETtype:
			if (BeamSTAFF(pL)==srcStf && vMap[BeamVOICE(pL)]==NOONE) {
				voice = DblNewVoice(doc, vMap, BeamVOICE(pL), dstStf, sameRel);
				vMap[BeamVOICE(pL)] = voice;
			}
			break;
		case TUPLETtype:
			if (TupletSTAFF(pL)==srcStf && vMap[TupletVOICE(pL)]==NOONE) {
				voice = DblNewVoice(doc, vMap, TupletVOICE(pL), dstStf, sameRel);
				vMap[TupletVOICE(pL)] = voice;
			}
			break;
		case GRAPHICtype:
			if (GraphicSTAFF(pL)==srcStf && vMap[GraphicVOICE(pL)]==NOONE) {
				voice = DblNewVoice(doc, vMap, GraphicVOICE(pL), dstStf, sameRel);
				vMap[GraphicVOICE(pL)] = voice;
			}
			break;
		default:
			;
	}
}

short PartRelStaffn(Document *, short);
short PartRelStaffn(Document *doc, short staffn)
{
	LINK partL, thePartL; PPARTINFO pPart; short partRelStf;
	
	if (staffn<1 || staffn>doc->nstaves) return NOONE;
	
	thePartL = Staff2PartL(doc, doc->headL, staffn);
	partL = NextPARTINFOL(FirstSubLINK(doc->headL));
	for ( ; partL; partL = NextPARTINFOL(partL)) {
		if (partL==thePartL) break;
	}
	pPart = GetPPARTINFO(partL);
	partRelStf = staffn-pPart->firstStaff+1;
	
	return partRelStf;
}

/* -------------------------------------------------------------- DblSetupVMap -- */
/* Given source and destination staves, set up a voice-remapping table for moving
stuff from one to the other.

We try to translate user voices in as intuitive a way as possible. Specifically:
	1. We always preserves default voices, i.e., if something starts in its staff's
		default voice, it ends up in its new staff's default voice.
	2. If the staff offset in staves' parts is zero (e.g., srcStf is staff 2 of its
		part, and so is dstStf in its part), ALL user voice numbers are preserved.
More could be done, but this should be enough to keep almost everyone happy. */

void DblSetupVMap(Document *, short [], LINK, LINK, short, short);
void DblSetupVMap(Document *doc, short vMap[], LINK startL, LINK endL, short srcStf,
						short dstStf)
{
	short v; LINK pL; Boolean sameRel;

	/* Initialize the vMap table: (internal) old voice -> new voice. A value of NOONE at
		position v in the table indicates no mapping has been established for voice v.
		Initially, the only known mapping is default voice to default voice. */

	for (v = 0; v<MAXVOICES+1; v++)
		vMap[v] = NOONE;
	vMap[srcStf] = dstStf;

	sameRel = (PartRelStaffn(doc, srcStf)==PartRelStaffn(doc, dstStf));

	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		DblSetNewVoices(doc, vMap, pL, srcStf, dstStf, sameRel);

#ifdef DEBUG_VMAPTABLE
	LogPrintf(LOG_NOTICE, "srcStf=%d dstStf=%d sameRel=%d\n", srcStf, dstStf, sameRel);
	for (v = 1; v<=MAXVOICES; v++)
		if (doc->voiceTab[v].partn!=0)
			LogPrintf(LOG_NOTICE, "%ciVoice %d part %d relVoice=%d\n",
							(v==1? '«' : ' '),
							v, doc->voiceTab[v].partn, doc->voiceTab[v].relVoice);
#endif	
}


/* ----------------------------------------------------------------- DblMapVoices -- */
/* If the given object has voice nos., update its voice no. or those of all its
subobjects on the given staff according to the given voice-remapping table. */

static void DblMapVoices(Document */*doc*/, short vMap[], LINK pL, short dstStf)
{
	LINK aNoteL,aGRNoteL; short voice;

	switch (ObjLType(pL)) {
		case SYNCtype:
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteSTAFF(aNoteL)==dstStf) {
					voice = vMap[NoteVOICE(aNoteL)];
					NoteVOICE(aNoteL) = voice;
				}
			}
			break;
		case GRSYNCtype:
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				if (GRNoteSTAFF(aGRNoteL)==dstStf) {
					voice = vMap[GRNoteVOICE(aGRNoteL)];
					GRNoteVOICE(aGRNoteL) = voice;
				}
			}
			break;
		case SLURtype:
			if (SlurSTAFF(pL)==dstStf) {
				voice = vMap[SlurVOICE(pL)];
				SlurVOICE(pL) = voice;
			}
			break;
		case BEAMSETtype:
			if (BeamSTAFF(pL)==dstStf) {
				voice = vMap[BeamVOICE(pL)];
				BeamVOICE(pL) = voice;
			}
			break;
		case TUPLETtype:
			if (TupletSTAFF(pL)==dstStf) {
				voice = vMap[TupletVOICE(pL)];
				TupletVOICE(pL) = voice;
			}
			break;
		case GRAPHICtype:
			if (GraphicSTAFF(pL)==dstStf) {
				voice = vMap[GraphicVOICE(pL)];
				GraphicVOICE(pL) = voice;
			}
			break;
		default:
			;
	}
}


/* --------------------------------------------------------------- FixXXXForClef -- */
/* Fix the given note's Y-position for the IMPENDING move of the note to a different
staff and clef. If the clef in effect on the new staff is the same as the note's
current clef, do nothing. Handles grace AND regular notes. */

void FixNoteYForClef(Document */*doc*/,
							LINK syncL,
							LINK aNoteL,					/* Note or grace note */
							short /*absStaff*/,			/* Destination staff */
							CONTEXT oldContext,			/* Current and future, respectively */
							CONTEXT newContext
							)
{
	short yPrev, yHere;
	DDIST yDelta;
	STDIST dystd;

	if (oldContext.clefType==newContext.clefType) return;
	
	yPrev = ClefMiddleCHalfLn(oldContext.clefType);
	yHere = ClefMiddleCHalfLn(newContext.clefType);
	yDelta = halfLn2d(yHere-yPrev, newContext.staffHeight, newContext.staffLines);
	dystd = halfLn2std(yHere-yPrev);

	if (SyncTYPE(syncL)) {
		NoteYD(aNoteL) += yDelta;
		if (config.moveModNRs) MoveModNRs(aNoteL, dystd);
	}
	else
		GRNoteYD(aNoteL) += yDelta;
}


/* Fix the note/chord(s)in the given Sync and staff for their ALREADY PERFORMED move
to a different clef. If the clef in effect on the new staff is the same as the
previous clef, do nothing. Handles grace??NOT QUITE! AND regular chords. */

void FixChordsForClef(Document *doc, LINK syncL,
							short absStaff,			/* Destination staff */
							CONTEXT oldContext,		/* Previous and current, respectively */
							CONTEXT newContext
							)
{
	LINK aNoteL; Boolean stemDown; short qStemLen;
	
	if (oldContext.clefType==newContext.clefType) return;

	if (SyncTYPE(syncL)) {
		for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==absStaff && MainNote(aNoteL)) {
				if (NoteINCHORD(aNoteL)) {
					FixSyncForChord(doc, syncL, NoteVOICE(aNoteL), NoteBEAMED(aNoteL), 0, 0, NULL);
					}
				else {
					stemDown = GetCStemInfo(doc, syncL, aNoteL, newContext, &qStemLen);
					NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL),
														NFLAGS(NoteType(aNoteL)),
														stemDown,
														newContext.staffHeight, newContext.staffLines,
														qStemLen, FALSE);
				}
			}
	}
	else if (GRSyncTYPE(syncL)) {
		SysBeep(1);		/* ??NOT HELPFUL! Either fix this or give an error message. */
	}
}


/* ------------------------------------------------------------- DupAndSetStaff -- */
/* Given an object and source and destination staves, duplicate whatever pieces of
the object are on the source staff and put the duplicates on the destination staff.
If we're Looking at a Voice, we duplicate only what's in that voice on the source
staff. NB: we assume that at least some of the object is on the source staff, so,
if it's an object type that's always entirely on one staff, we don't check its
staff at all.

If an entire new object is created, put it at the beginning of the existing obj's
slot. For objs that have voice numbers, set the voice number as well.

Return FAILURE if there's an error (probably out of memory), else NOTHING_TO_DO
or OP_COMPLETE. */

short DupAndSetStaff(Document *doc, short voiceMap[],
						LINK pL,							/* Original object */
						short srcStf, short dstStf,
						short copyVoice				/* Single voice to copy, or -1 for all */
						)
{
	short nSubs, halfLn, effAcc;
	LINK tmpL, copyL, firstMod,
			aNoteL, srcNoteL, nextSrcNoteL, dstNoteL,
			aGRNoteL, srcGRNoteL, nextGRNoteL, dstGRNoteL;
	PANOTE srcNote, dstNote, srcGRNote, dstGRNote;
	CONTEXT oldContext, newContext;
	Boolean anyVoice=(copyVoice<0);
	
	GetContext(doc, pL, srcStf, &oldContext);
	GetContext(doc, pL, dstStf, &newContext);
	
	switch (ObjLType(pL)) {
		case SYNCtype:
			nSubs = SVCountNotes(srcStf, copyVoice, pL, RightLINK(pL), FALSE);
			if (nSubs==0) return NOTHING_TO_DO;
			if (!ExpandNode(pL, &aNoteL, nSubs))	/* ??Cf. GrowObject */
				return FAILURE;
			
			/*
			 *	For each new note/rest, fill it in from the corresponding pre-existing one,
			 *	then update it for its new context (staff and voice no., clef, key sig.).
			 * Copy the note/rest as a whole, then replace its ModNR list with a
			 *	duplicate of the original's and restore its <next> field.
			 */
			dstNoteL = aNoteL;												/* The first new one */
			nextSrcNoteL = FirstSubLINK(pL);
			for ( ; dstNoteL; dstNoteL = NextNOTEL(dstNoteL)) {
				for (srcNoteL = nextSrcNoteL; srcNoteL; srcNoteL = NextNOTEL(srcNoteL))
					if (NoteSTAFF(srcNoteL)==srcStf && (anyVoice || NoteVOICE(srcNoteL)==copyVoice)) {
						nextSrcNoteL = NextNOTEL(srcNoteL);				/* Don't find this one again */
						break;
					}

				if (!srcNoteL) MayErrMsg("DupAndSetStaff: srcNoteL disaster");
				srcNote = GetPANOTE(srcNoteL);
				dstNote = GetPANOTE(dstNoteL);
				tmpL = dstNote->next;
				*dstNote = *srcNote;											/* Copy, then restore link */
				dstNote->next = tmpL;
				if (dstNote->firstMod) {
					firstMod = CopyModNRList(doc, doc, srcNote->firstMod);
					dstNote = GetPANOTE(dstNoteL);
					dstNote->firstMod = firstMod;
				}

				/* Handle change of clef, and pick up effective accidental. */
				if (!NoteREST(dstNoteL)) {
					effAcc = EffectiveAcc(doc, pL, dstNoteL);
					FixNoteYForClef(doc, pL, dstNoteL, dstStf, oldContext, newContext);
					halfLn = qd2halfLn(NoteYQPIT(dstNoteL));
				}

				/* Move the note to the new staff */
				NoteSTAFF(dstNoteL) = dstStf;

				if (!NoteREST(dstNoteL)) {												
					/* If note has no accidental, handle accidental context on new staff. */
					if (!NoteACC(dstNoteL)) {
						NoteACC(dstNoteL) = effAcc;
						NoteACCSOFT(dstNoteL) = TRUE;
					}
					InsNoteFixAccs(doc, pL, dstStf, halfLn, NoteACC(dstNoteL));
				}
			}
			
			DblMapVoices(doc, voiceMap, pL, dstStf);
			FixChordsForClef(doc, pL, dstStf, oldContext, newContext);			
			break;
			
		case GRSYNCtype:
			nSubs = SVCountGRNotes(srcStf, copyVoice, pL, RightLINK(pL), FALSE);
			if (nSubs==0) return NOTHING_TO_DO;
			if (!ExpandNode(pL, &aGRNoteL, nSubs))	/* ??Cf. GrowObject */
				return FAILURE;
			
			/*
			 *	Fill in each new grace note from the corresponding pre-existing one, then
			 *	update it for its new context (staff and voice no., clef, key sig.).
			 */
			dstGRNoteL = aGRNoteL;											/* The first new one */
			nextGRNoteL = FirstSubLINK(pL);
			for ( ; dstGRNoteL; dstGRNoteL = NextGRNOTEL(dstGRNoteL)) {
				for (srcGRNoteL = nextGRNoteL; srcGRNoteL; srcGRNoteL = NextGRNOTEL(srcGRNoteL))
					if (GRNoteSTAFF(srcGRNoteL)==srcStf && (anyVoice || GRNoteVOICE(srcGRNoteL)==copyVoice)) {
						nextGRNoteL = NextGRNOTEL(srcGRNoteL);			/* Don't find this one again */
						break;
					}
				srcGRNote = GetPAGRNOTE(srcGRNoteL);
				dstGRNote = GetPAGRNOTE(dstGRNoteL);
				tmpL = dstGRNote->next;
				*dstGRNote = *srcGRNote;									/* Copy, then restore link */
				dstGRNote->next = tmpL;

				/* Handle change of clef, and pick up effective accidental. */
				effAcc = EffectiveGRAcc(doc, pL, dstGRNoteL);
				FixNoteYForClef(doc, pL, dstGRNoteL, dstStf, oldContext, newContext);
				halfLn = qd2halfLn(GRNoteYQPIT(dstGRNoteL));

				/* Move the grace note to the new staff */
				GRNoteSTAFF(dstGRNoteL) = dstStf;

				/* If grace note has no accidental, handle accidental context on new staff. */
				if (!GRNoteACC(dstGRNoteL)) {
					GRNoteACC(dstGRNoteL) = effAcc;
					GRNoteACCSOFT(dstGRNoteL) = TRUE;
				}
			}

			DblMapVoices(doc, voiceMap, pL, dstStf);
			FixChordsForClef(doc, pL, dstStf, oldContext, newContext);			
			break;
			
		case BEAMSETtype:
			if (!anyVoice && BeamVOICE(pL)!=copyVoice) break;
			copyL = DuplicateObject(BEAMSETtype, pL, FALSE, doc, doc, FALSE);
			if (!copyL) return FAILURE;
			InsNodeIntoSlot(copyL, pL);
			BeamSTAFF(copyL) = dstStf;
			DblMapVoices(doc, voiceMap, copyL, dstStf);
			break;
			
		case OCTAVAtype:
			copyL = DuplicateObject(OCTAVAtype, pL, FALSE, doc, doc, FALSE);
			if (!copyL) return FAILURE;
			InsNodeIntoSlot(copyL, pL);
			OctavaSTAFF(copyL) = dstStf;
			break;
		
		case DYNAMtype:
			{
				PADYNAMIC aDynamic;
				
				copyL = DuplicateObject(DYNAMtype, pL, FALSE, doc, doc, FALSE);
				if (!copyL) return FAILURE;
				InsNodeIntoSlot(copyL, pL);
				aDynamic = GetPADYNAMIC(FirstSubLINK(copyL));
				aDynamic->staffn = dstStf;
			}
			break;

		case GRAPHICtype:
			copyL = DuplicateObject(GRAPHICtype, pL, FALSE, doc, doc, FALSE);
			if (!copyL) return FAILURE;
			InsNodeIntoSlot(copyL, pL);
			GraphicSTAFF(copyL) = dstStf;
			break;
			
		case SLURtype:
			if (!anyVoice && SlurVOICE(pL)!=copyVoice) break;
			copyL = DuplicateObject(SLURtype, pL, FALSE, doc, doc, FALSE);
			if (!copyL) return FAILURE;
			InsNodeIntoSlot(copyL, pL);
			SlurSTAFF(copyL) = dstStf;
			DblMapVoices(doc, voiceMap, copyL, dstStf);
			break;
		
		case TUPLETtype:
			copyL = DuplicateObject(TUPLETtype, pL, FALSE, doc, doc, FALSE);
			if (!copyL) return FAILURE;
			InsNodeIntoSlot(copyL, pL);
			TupletSTAFF(copyL) = dstStf;
			DblMapVoices(doc, voiceMap, copyL, dstStf);
			break;
			
		case TEMPOtype:
			copyL = DuplicateObject(TEMPOtype, pL, FALSE, doc, doc, FALSE);
			if (!copyL) return FAILURE;
			InsNodeIntoSlot(copyL, pL);
			TempoSTAFF(copyL) = dstStf;
			break;
		
		case SPACEtype:
			break;
			
		default:
			;
	}
	
	return OP_COMPLETE;
}

/* ----------------------------------------------------------- StfHasSmthgAcross -- */
/* If any "extended object" has a range that crosses the point just before the
given link on the given staff, return TRUE and a message string. However, this does
NOT check for slurs/ties crossing the point. Similar (identical?) to HasSmthgAcross,
except that function checks all staves. ??Should probably move this to DSUtils.c
near that function. */

Boolean StfHasSmthgAcross(
				Document *doc,
				LINK link,
				short staff,
				char str[])			/* string describing the problem voice or staff */
{
	short number;
	Boolean isVoice, foundSmthg=FALSE;
	
	if (HasBeamAcross(link, staff)
	||  HasTupleAcross(link, staff)
	||  HasOctavaAcross(link, staff) ) {
		isVoice = FALSE;
		number = staff;
		foundSmthg = TRUE;
	}

	if (foundSmthg) {
		if (isVoice)														/* ??this will never be true! */
			Voice2UserStr(doc, number, str);
		else
			Staff2UserStr(doc, number, str);
	}

	return foundSmthg;
}


Boolean RangeHasUnmatchedSlurs(Document */*doc*/, LINK startL, LINK endL, short staff)
{
	LINK pL;
	
	for (pL = LeftLINK(startL); pL && !SystemTYPE(pL); pL = LeftLINK(pL)) {
		if (SlurTYPE(pL) && SlurSTAFF(pL)==staff)
			/* Slur starts before, so it's unmatched if it ends between [startL, endL). */
				if (WithinRange(startL, SlurLASTSYNC(pL), endL))
					return TRUE;
	}
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL)) {
		if (SlurTYPE(pL) && SlurSTAFF(pL)==staff)
			/* Slur starts between, so it's unmatched if it ends after [startL, endL). */
				if (IsAfter(LeftLINK(endL), SlurLASTSYNC(pL)))
					return TRUE;
	}

	return FALSE;
}


/* Look for a Dynamic, ignoring hairpins. */

static LINK LSContextDynamicSearch(
				LINK startL,			/* Place to start looking */
				short staff,			/* target staff number */
				Boolean goLeft,			/* TRUE if we should search left */
				Boolean needSelected	/* TRUE if we only want selected items */
				)
{
	LINK dynL;
	
Search:
	dynL = LSSearch(startL, DYNAMtype, staff, goLeft, needSelected);
	if (dynL) {
		if (DynamType(dynL)<FIRSTHAIRPIN_DYNAM)
			return dynL;
		else {
			startL = RightLINK(dynL);
			goto Search;
		}
	}
	
	return NILINK;
}


/* Update Dynamic context on <dstStf> baseed on Dynamics in the selection range
(whether actually selected or not) on that staff. Intended for use with Double().
NB: Notes in the range in the original staff before the 1st Dynamic will have
velocities based on the Dynamic in effect there, which is probably different from
the Dynamic in effect at the beginning of the range in <dstStf>. This means that
velocities of notes at the beginning of the range in <dstStf> won't be consistent
with the context. The best solution might be ??MAKE THEM CONSISTENT! inserting an explicit
Dynamic on <dstStf>. Some day. */
 
static void DblFixContext(Document *doc, short dstStf)
{
	LINK pL, dstMeasL, dstStaffL, aDynamicL;
	PAMEASURE dstMeas;
	PASTAFF dstStaff;
	CONTEXT context;
	short startDynamType, currentDynamType;
	
	/* Pick uo the Dynamic context from just before the selection range. Then update
		context-bearing objects (Measure and Staff) in the selection range, considering
		Dynamics we encounter along the way. */

LogPrintf(LOG_NOTICE, "DblFixContext0: doc->selStartL=%d dstStf=%d\n", doc->selStartL, dstStf);
	GetContext(doc, doc->selStartL, dstStf, &context);
	startDynamType = currentDynamType = context.dynamicType;
LogPrintf(LOG_NOTICE, "DblFixContext1: currentDynamType=%d\n", currentDynamType);

	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case DYNAMtype:
				if (DynamType(pL)<FIRSTHAIRPIN_DYNAM) {
					aDynamicL = FirstSubLINK(pL);
					if (DynamicSTAFF(aDynamicL)==dstStf) {
						currentDynamType = DynamType(pL);
LogPrintf(LOG_NOTICE, "DblFixContext: Dynamic pL=%d dynam=%d\n",
	pL, currentDynamType);
					}
				}
				continue;
			case MEASUREtype:
LogPrintf(LOG_NOTICE, "DblFixContext: Measure pL=%d dynam=%d\n",
	pL, currentDynamType);
				dstMeasL = MeasOnStaff(pL, dstStf);
				dstMeas = GetPAMEASURE(dstMeasL);
				dstMeas->dynamicType = currentDynamType;
				continue;
			case STAFFtype:
				dstStaffL = StaffOnStaff(pL, dstStf);
				dstStaff = GetPASTAFF(dstStaffL);
				dstStaff->dynamicType = currentDynamType;
				continue;
			default:
				;
		}
	}
		
	/* Now update Dynamic context as if new Dynamics were just inserted at the beginning
	of the selection range (to make things consistent before the first actual Dynamic
	in the range) and at the end of the range, */
	EFixContForDynamic(doc->selStartL, doc->selEndL, dstStf, 99 /* unused */, startDynamType);
	EFixContForDynamic(doc->selEndL, doc->tailL, dstStf, 99 /* unused */, currentDynamType);
}


static Boolean IsDoubleOK(Document *doc, short srcStf, short dstStf)
{
	Boolean hasSmthgAcross, crossStaff;
	short srcPart, dstPart;
	LINK pL;
	char str[256];
	
	if (!DblSrcStaffOK(doc->selStartL, doc->selEndL, srcStf)) {
		GetIndCString(strBuf, DOUBLE_STRS, 7);			/* "...range contains clefs or keysigs" */
		CParamText(strBuf, "", "", "");
		StopInform(DOUBLE_ALRT);
		return FALSE;
	}

	if (!DblDstStaffOK(doc->selStartL, doc->selEndL, dstStf)) {
		GetIndCString(strBuf, DOUBLE_STRS, 4);			/* "can't Double into a non-empty range" */
		CParamText(strBuf, "", "", "");
		StopInform(DOUBLE_ALRT);
		return FALSE;
	}
	
	/* ??The following checks the legality of the range in the source staff; it should do
		do similar checks on the destination staff! */
	hasSmthgAcross = StfHasSmthgAcross(doc, doc->selStartL, srcStf, str);
	if (!hasSmthgAcross)
		hasSmthgAcross = StfHasSmthgAcross(doc, doc->selEndL, srcStf, str);
	if (hasSmthgAcross) {
		StopInform(DEXTOBJ_ALRT);
		return FALSE;
	}
	if (RangeHasUnmatchedSlurs(doc, doc->selStartL, doc->selEndL, srcStf)) {
		GetIndCString(strBuf, DOUBLE_STRS, 5);			/* "Slurs extend into/out of range" */
		CParamText(strBuf, "", "", "");
		StopInform(DOUBLE_ALRT);
		return FALSE;
	}

	srcPart = Staff2Part(doc, srcStf);
	dstPart = Staff2Part(doc, dstStf);

	crossStaff = FALSE;
	for (pL = doc->selStartL; !crossStaff && pL!=doc->selEndL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case BEAMSETtype:
				if (BeamCrossSTAFF(pL)) {
					short beamPart = Staff2Part(doc, BeamSTAFF(pL));
					if (beamPart==srcPart || beamPart==dstPart)
						crossStaff = TRUE;
				}
				break;
			case SLURtype:
				if (SlurCrossSTAFF(pL)) {
					short slurPart = Staff2Part(doc, SlurSTAFF(pL));
					if (slurPart==srcPart || slurPart==dstPart)
						crossStaff = TRUE;			/* may not be possible but just in case */
				}
				break;
			case TUPLETtype:
				if (IsTupletCrossStf(pL)) {
					short tuplPart = Staff2Part(doc, TupletSTAFF(pL));
					if (tuplPart==srcPart || tuplPart==dstPart)
						crossStaff = TRUE;
				}
				break;
			default:
				;
		}
	}
	if (crossStaff) {
		GetIndCString(strBuf, DOUBLE_STRS, 8);			/* "can't Double cross-staff..." */
		CParamText(strBuf, "", "", "");
		StopInform(DOUBLE_ALRT);
		return FALSE;
	}
	
	return TRUE;
}


/* --------------------------------------------------------------------- Double -- */
/* Ask user for a destination staff; then duplicate "everything" in the selection
range (whether it's actually selected or not!) on the first selected staff, and
put the duplicates on the destination staff. The source staff in the range must not
contain certain objects (see DblSrcStaffOK), and the destination staff in the range
must be "empty". Returns FAILURE if there's an error (probably out of memory), else
NOTHING_TO_DO or OP_COMPLETE.

The "everything" that's duplicated and "empty" destination need more explanation,
especially as regards the objects that affect context:
						Source staff						Destination staff
						------------						-----------------
	Clefs				not allowed (except gutter)			not allowed (except gutter)
	Key signatures		not allowed (except gutter)			not allowed (except gutter)
	Time signatures		ignored 							allowed, left alone
	Dynamics			copied								not allowed
These rules let us keep the context consistent without too much trouble while (I
hope) not being too restrictive. */

short Double(Document *doc)
{
	static short dstStf=1;
	short srcStf, status;
	LINK pL, selStartL, selEndL;
	Boolean didSomething=FALSE, skipping;
	short voiceMap[MAXVOICES+1];
	
	srcStf = GetSelStaff(doc);
	if (dstStf>doc->nstaves) dstStf = 1;
	if (!DoubleDialog(doc, srcStf, &dstStf)) return NOTHING_TO_DO;

	if (!IsDoubleOK(doc, srcStf, dstStf)) return FAILURE;

	/*
	 * The selection range might start with the first note of a beam or tuplet. Be sure
	 * we consider the Beamset or Tuplet object.
	 */ 
	doc->selStartL = LocateInsertPt(doc->selStartL);

	PrepareUndo(doc, doc->selStartL, U_Double, 10);    	/* "Undo Double Selection" */
	WaitCursor();

	DblSetupVMap(doc, voiceMap, doc->selStartL, doc->selEndL, srcStf, dstStf);

	/* We're ready to go. Duplicate everything in the range, skipping system gutters. */
	
	skipping = FALSE;
	for (pL = doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL)) {
		if (PageTYPE(pL) || SystemTYPE(pL)) skipping = TRUE;
		if (!skipping && ObjOnStaff(pL, srcStf, FALSE)) {
			status = DupAndSetStaff(doc, voiceMap, pL, srcStf, dstStf, doc->lookVoice);
			if (status==FAILURE) return FAILURE;
			if (status==OP_COMPLETE) {
				didSomething = TRUE;
			}
		}
		if (MeasureTYPE(pL)) skipping = FALSE;
	}

	if (didSomething) {
		doc->changed = TRUE;
		DblFixContext(doc, dstStf);
		/*
		 * We may have newly-created beams and so on preceding the selection range. The
		 * easiest way to fix it (though not the most efficient) is via OptimizeSelection.
		 */
		doc->selStartL = doc->headL;
		OptimizeSelection(doc);
		/*
		 *	...but we now need to select everything in the range on <dstStf> and nothing
		 * else, and do final cleanup on the new stuff. We leave the selection this way
		 * unless we're Looking at a Voice. In that case the new stuff isn't in the
		 * Looked-at voice, so we just leave an insertion point on <dstStf>.
		 */
		selStartL = doc->selStartL;
		selEndL = doc->selEndL;
		DeselAllNoHilite(doc);
		SelectRange(doc, selStartL, selEndL, dstStf, dstStf);
		doc->selStartL = selStartL;
		doc->selEndL = selEndL;		

		DelRedundantAccs(doc, dstStf, DELSOFT_REDUNDANTACCS_DI);
		InvalContent(doc->selStartL, doc->selEndL);
		InvalSelRange(doc);
		if (doc->lookVoice>=0) {
			DeselAllNoHilite(doc);
			doc->selStaff = dstStf;
			doc->selStartL = doc->selEndL = selStartL;
			MEAdjustCaret(doc,TRUE);
		}
	}
	
	return (didSomething? OP_COMPLETE : NOTHING_TO_DO);
}
