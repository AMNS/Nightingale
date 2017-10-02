/* File Ottava.c - octave-sign-related functions. */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.

	DoOttava				DoRemoveOttava			GetNoteyd
	SetOttavaYPos			CreateOTTAVA			UnOttava
	UnOttavaS
	UnOttavaSync			UnOttavaGRSync			UnOttavaRange
	GetOctTypeNum			DrawOTTAVA
	FirstInOttava 			LastInOttava
	HasOctAcross			HasOttavaAcross 		OctOnStaff	
	HasOttavaAcrossIncl
	QD_DrawDashedLine		DrawOctBracket			XLoadOttavaSeg		
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Prototypes for internal functions */
static void CreateOctStfRange(Document *doc, short s, LINK stfStartL, LINK stfEndL, Byte octType);
static LINK GetOctStartL(LINK startL, short staff, Boolean needSel);
static DDIST GetNoteyd(LINK syncL, short staff);
static void UnOttava(Document *, LINK, LINK, short);
static void UnOttavaS(Document *, LINK, LINK, short);
static void UnOttavaSync(Document *, LINK, LINK, DDIST, short, CONTEXT);
static void UnOttavaGRSync(Document *, LINK, LINK, DDIST, short, CONTEXT);
static void RemoveItsOttava(Document *doc, LINK fromL, LINK toL, LINK pL, short s);
static LINK HasGROctAcross(LINK node, short staff);
static LINK HasOctAcrossPt(Document *, Point, short);
static LINK HasGROctAcrossPt(Document *, Point, short);


/* -------------------------------------------------------------------- DoOttava -- */
/* Put an octave sign around the selection. */

void DoOttava(Document *doc)
{
	short staff;
	LINK staffStartL, staffEndL;
	Byte octSignType;

	if (OttavaDialog(doc, &octSignType)) {
		PrepareUndo(doc, doc->selStartL, U_Ottava, 31);    	/* "Undo Create Ottava" */
	
		for (staff = 1; staff<=doc->nstaves; staff++) {
			GetStfSelRange(doc, staff, &staffStartL, &staffEndL);
			CreateOctStfRange(doc, staff, staffStartL, staffEndL, octSignType);
		}
	}
}

/* -------------------------------------------------------------- DoRemoveOttava -- */
/* Remove the octave sign from the selection. */

void DoRemoveOttava(Document *doc)
{
	LINK	startL, endL, staffStartL, staffEndL;
	short	staff;
	
	GetOptSelEnds(doc, &startL, &endL);
	PrepareUndo(doc, startL, U_UnOttava, 32);    				/* "Undo Remove Ottava" */

	for (staff = 1; staff<=doc->nstaves; staff++) {
		GetStfSelRange(doc, staff, &staffStartL, &staffEndL);
		if (!staffStartL || !staffEndL) continue;
		if (SelRangeChkOttava(staff, staffStartL, staffEndL))
			UnOttava(doc, staffStartL, staffEndL, staff);
	}
		
	if (doc->autoRespace)
		RespaceBars(doc, doc->selStartL, doc->selEndL, 0L, False, False);
}

/* ------------------------------------------------------------ CreateOctStfRange -- */
/* Create octave sign for notes from stfStartL to stfEndL, excluding stfEndL,
on staff <s>. */

static void CreateOctStfRange(Document *doc, short s, LINK stfStartL, LINK stfEndL,
								Byte octType)
{
	short nInOttava;  LINK ottavaL;

	nInOttava = OctCountNotesInRange(s, stfStartL, stfEndL, True);

	if (nInOttava>0) {
		ottavaL = CreateOTTAVA(doc,stfStartL,stfEndL,s,nInOttava,octType,True,True);
		if (doc->selStartL==FirstInOttava(ottavaL))
			doc->selStartL = ottavaL;
	}
}

/* ----------------------------------------------------------------- GetOctStartL -- */
/* Get new startL in order to insert octave sign into object list before first
sync or GRSync to right of <startL>. */

static LINK GetOctStartL(LINK startL, short staff, Boolean needSel)
{
	LINK octStartL, GRStartL;

	octStartL = LSSearch(startL, SYNCtype, staff, False, needSel);
	GRStartL = LSSearch(startL, GRSYNCtype, staff, False, needSel);

	if (octStartL) {
		if (GRStartL && IsAfter(GRStartL,octStartL))
			return GRStartL;
	}
	else if (GRStartL)
		return GRStartL;
	
	return octStartL;
}

/* -------------------------------------------------------------------- GetNoteyd -- */
/* Given syncL, return the yd of a MainNote or GRMainNote on <staff>. Note that
there may be more than one such MainNote/GRMainNote: we just use the first one
we find. */

static DDIST GetNoteyd(LINK syncL,
						short staffn)		/* Either Sync or GRSync */
{
	LINK aNoteL, aGRNoteL;
	PANOTE aNote; PAGRNOTE aGRNote;

	if (SyncTYPE(syncL)) {
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==staffn && MainNote(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				return aNote->yd;
			}
	}
	else if (GRSyncTYPE(syncL)) {
		aGRNoteL = FirstSubLINK(syncL);
		for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
			if (GRNoteSTAFF(aGRNoteL)==staffn && GRMainNote(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL);
				return aGRNote->yd;
			}
	}
	return 0;
}


#define OTTAVA_BRACKET_ALTA_LIM(yd)		((yd)>octAltaYLim? octAltaYLim : (yd))
#define OTTAVA_BRACKET_BASSA_LIM(yd)	((yd)<octBassaYLim? octBassaYLim : (yd))

/* --------------------------------------------------------------- SetOttavaYPos -- */
/* Set the given octave sign to (a guess at) a reasonable vertical position, based
on the position of the MainNote of its first note/chord. (FIXME: It'd surely be better
to look at all of its notes/chords, and not difficult.) */

void SetOttavaYPos(Document *doc, LINK octL)
{
	POTTAVA		pOct;
	short		staff;
	CONTEXT		context;
	DDIST		firstyd, margin; 
	DDIST		octydFirst,
				octAltaYLim, octBassaYLim,			/* Closest possible positions to the staff */
				lnSpace, dhalfLn;
	LINK		firstSyncL;
	Boolean  	isBassa=False;

	staff = OttavaSTAFF(octL);
	GetContext(doc, octL, staff, &context);
	lnSpace = LNSPACE(&context);
	dhalfLn = lnSpace/2;

	firstSyncL = FirstInOttava(octL);
	firstyd = GetNoteyd(firstSyncL, staff);

	octAltaYLim = -OTTAVA_STANDOFF_ALTA*dhalfLn;
	octBassaYLim = context.staffHeight + OTTAVA_STANDOFF_BASSA*dhalfLn;

	pOct = GetPOTTAVA(octL);
	switch (pOct->octSignType) {
		case OTTAVA8va:
		case OTTAVA15ma:
		case OTTAVA22ma:
			isBassa = False;
			break;
		case OTTAVA8vaBassa:
		case OTTAVA15maBassa:
		case OTTAVA22maBassa:
			isBassa = True;
			break;
		default:
			;
	}

	/*
	 * Try to position the octave sign <margin> half-lines away from the 1st MainNote,
	 *	but always put it at least <OTTAVA_STANDOFF_ALTA/BASSA> away from the staff.
	 */
 	if (isBassa)	margin = OTTAVA_MARGIN_BASSA*dhalfLn;
 	else			margin = OTTAVA_MARGIN_ALTA*dhalfLn;
	if (isBassa)	octydFirst = OTTAVA_BRACKET_BASSA_LIM(firstyd+margin);
	else			octydFirst = OTTAVA_BRACKET_ALTA_LIM(firstyd-margin);
	
	pOct = GetPOTTAVA(octL);
	pOct->ydLast = pOct->ydFirst = octydFirst;
}


/* ---------------------------------------------------------------- CreateOTTAVA -- */
/* Create octave sign in range [startL,endL) on staff <staff>. */

LINK CreateOTTAVA(
			Document *doc,
			LINK startL, LINK endL,
			short staff, short nInOttava,
			Byte octSignType,
			Boolean needSelected,	/* True if we only want selected items */
			Boolean doOct			/* True if we're explicitly octaving notes (so Ottava will be selected) */
			)
{
	POTTAVA 	ottavap;
	LINK		pL, ottavaL, aNoteOctL, nextL, mainNoteL, beamL,
				newBeamL, mainGRNoteL;
	PANOTEOTTAVA aNoteOct;
	short 		octElem, iOctElem,  v;
	PANOTE		aNote;
	PAGRNOTE	aGRNote;
	LINK		aNoteL, aGRNoteL;
	LINK		opSync[MAXINOTTAVA];
	LINK		noteInSync[MAXINOTTAVA];
	DDIST		yDelta;
	CONTEXT		context;
	Boolean 	stemDown, multiVoice;
	short		stemLen;
	QDIST		qStemLen;
	STDIST		dystd;	
	char		fmtStr[256];
	
	if (nInOttava<1) {
		MayErrMsg("CreateOTTAVA: tried to make Ottava with %ld notes/chords.", (long)nInOttava);
		return NILINK;
	}
	if (nInOttava>MAXINOTTAVA) {
		GetIndCString(fmtStr, OCTAVEERRS_STRS, 1);    /* "The maximum number of notes/chords in an octave sign is %d" */
		sprintf(strBuf, fmtStr, MAXINBEAM);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return NILINK;
	}
	
	/* Insert Ottava into object list before first Sync or GRSync to right of <startL>. */
	startL = GetOctStartL(startL, staff, needSelected);
	
	ottavaL = InsertNode(doc, startL, OTTAVAtype, nInOttava);
	if (!ottavaL) { NoMoreMemory(); return NILINK; }
	SetObject(ottavaL, 0, 0, False, True, False);
	ottavap = GetPOTTAVA(ottavaL);
	ottavap->tweaked = False;
	ottavap->staffn = staff;
	ottavap->octSignType = octSignType;

	if (doOct) ottavap->selected = True;

	ottavap->numberVis = True;
	ottavap->brackVis = False;

	ottavap->nxd = ottavap->nyd = 0;
	ottavap->xdFirst = OTTAVA_XDPOS_FIRST;
	ottavap->xdLast = OTTAVA_XDPOS_LAST;

	ottavap = GetPOTTAVA(ottavaL);
	ottavap->noCutoff = ottavap->crossStaff = ottavap->crossSystem = False;	
	ottavap->filler = 0;
	ottavap->unused1 = ottavap->unused2 = 0;

	doc->changed = True;

	for (octElem = 0, pL = startL; pL!=endL; pL = RightLINK(pL))
		if (!needSelected || LinkSEL(pL)) {
			if (SyncTYPE(pL)) {
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->staffn==staff && !aNote->rest) {																/* Yes */
						aNote->inOttava = True;
						if (MainNote(aNoteL)) {
							opSync[octElem] = pL;
							noteInSync[octElem] = aNoteL;
							octElem++;
						}
					}
				}
			}
			else if (GRSyncTYPE(pL)) {
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					if (GRNoteSTAFF(aGRNoteL)==staff) {																/* Yes */
						aGRNote = GetPAGRNOTE(aGRNoteL);
						aGRNote->inOttava = True;
						if (GRMainNote(aGRNoteL)) {
							opSync[octElem] = pL;
							noteInSync[octElem] = aGRNoteL;
							octElem++;
						}
					}
				}
			}
		}

	if (octElem!=nInOttava) {
		MayErrMsg("CreateOTTAVA expected %ld notes but found %ld.",(long)nInOttava,(long)octElem);
		return NILINK;
	}
	
	iOctElem=0;
	aNoteOctL = FirstSubLINK(ottavaL);
	for ( ; iOctElem<octElem; iOctElem++,aNoteOctL=NextNOTEOTTAVAL(aNoteOctL)) {
		aNoteOct = GetPANOTEOTTAVA(aNoteOctL);
		aNoteOct->opSync = opSync[iOctElem];
	}	

	/* Update notes' y position. */
	
	GetContext(doc, ottavaL, staff, &context);
	yDelta = halfLn2d(noteOffset[octSignType-1],context.staffHeight, context.staffLines);

	for (pL = startL; pL!=endL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			multiVoice = IsSyncMultiVoice(pL, staff); 		/* Are there multiple voices on staff? */
			/* Loop through the notes, simple-mindedly fixing up notehead, stem, and
				aug. dot y-positions. Chords will need more attention once all their
				constituent notes have been moved. */
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (!NoteREST(aNoteL) && NoteSTAFF(aNoteL)==staff) {
					NoteYD(aNoteL) += yDelta;
					NoteYQPIT(aNoteL) += halfLn2qd(noteOffset[octSignType-1]);
					if (MainNote(aNoteL)) {
						/*
						 * If octave sign affects line/space status of notes (it does if it's
						 *	an odd no. of octaves change), move visible aug. dots to correct
						 * new positions.
						 */
						stemDown = GetStemInfo(doc, pL, aNoteL, &qStemLen);
						NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL), NFLAGS(NoteType(aNoteL)),
														stemDown, 
														context.staffHeight, context.staffLines,
														qStemLen, False);
					}
					else
						NoteYSTEM(aNoteL) = NoteYD(aNoteL);
					
					if (odd(noteOffset[octSignType-1]))
						ToggleAugDotPos(doc, aNoteL, stemDown);	/* FIXME: stemDown wrong for single notes! */

					dystd = halfLn2std(noteOffset[octSignType-1]);
					if (config.moveModNRs) MoveModNRs(aNoteL, dystd);
				}
			}
			/* Loop through the notes again and fix up chords (for notes' stem sides,
				accidental positions, etc.). */
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteINCHORD(aNoteL) && MainNote(aNoteL) && NoteSTAFF(aNoteL)==staff) {
					stemDown = (NoteYD(aNoteL)<=context.staffHeight/2);
					FixSyncForChord(doc, pL, NoteVOICE(aNoteL), NoteBEAMED(aNoteL), 
											0, (multiVoice? -1 : 1), &context);
				}
			}
		}
		else if (GRSyncTYPE(pL)) {
			multiVoice = False; 							/* FIXME: For now; needs GetGRMultiVoice! */
			/* Loop through the grace notes, fixing up notehead, stem, and aug.
				dot y-positions. */
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL);
				if (GRNoteSTAFF(aGRNoteL)==staff) {
					stemDown = False;						/* Assume grace notes stem up--FIXME: not always true */
					aGRNote->yd += yDelta;
					aGRNote->yqpit += halfLn2qd(noteOffset[octSignType-1]);
					/*
					 * If octave sign affects line/space status of notes, move visible aug.
					 * dots to correct new positions.
					 */
					if (odd(noteOffset[octSignType-1])) {
						if (aGRNote->ymovedots==1 || aGRNote->ymovedots==3)
								aGRNote->ymovedots = 2;
						else if (aGRNote->ymovedots==2)
								aGRNote->ymovedots = 1;
					}
					stemLen = QSTEMLEN(!multiVoice, ShortenStem(aGRNoteL, context, stemDown));
					aGRNote->ystem = CalcYStem(doc, aGRNote->yd,
												NFLAGS(aGRNote->subType),
												stemDown,
												context.staffHeight, context.staffLines,
												stemLen, False);
				}
			}
			/* Loop through the grace notes again and fix up the ystems of the
				non-extreme notes. */
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL);
				if (GRNoteSTAFF(aGRNoteL)==staff) {
					if (aGRNote->inChord) {
						FixGRSyncForChord(doc, pL, GRNoteVOICE(aGRNoteL), aGRNote->beamed, 
							0, (multiVoice? -1 : 1), &context);
						break;
					}
				}
			}
		}
	}

	/* Loop through notes and grace notes again, rebeaming any beamed ranges. */

	for (v=1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {
			for (pL = startL; pL!=endL; pL=nextL) {
				nextL = RightLINK(pL);
				if (SyncTYPE(pL)) {
					mainNoteL = FindMainNote(pL, v);
					if (mainNoteL && NoteBEAMED(mainNoteL)) {
						beamL = LVSearch(pL, BEAMSETtype, v, GO_LEFT, False);
						if (beamL)
							if (BeamSTAFF(beamL)==staff
							|| (BeamSTAFF(beamL)==staff-1 && BeamCrossSTAFF(beamL))) {
							newBeamL = Rebeam(doc, beamL);
							if (startL==beamL) startL = newBeamL;
							if (doc->selStartL==beamL) doc->selStartL = newBeamL;
							
							nextL = RightLINK(LastInBeam(newBeamL));
							if (IsAfter(endL, nextL)) nextL = endL;
						}
					}
				}
				/* FIXME: BLOCKS ARE WRONG. FOLLOWING "else" IS *INSIDE* THE "if (SyncTYPE(pL))"
					SO IT'LL NEVER DO ANYTHING. BUT GRACE BEAMS GET REBEAMED ANYWAY. ?? */
				else if (GRSyncTYPE(pL)) {
					mainGRNoteL = FindGRMainNote(pL,v);
					if (mainGRNoteL && GRNoteBEAMED(mainGRNoteL)) {
						beamL = LVSearch(pL,BEAMSETtype,v,GO_LEFT,False);
						if (beamL)
							if (BeamSTAFF(beamL)==staff
							|| (BeamSTAFF(beamL)==staff-1 && BeamCrossSTAFF(beamL))) {
							newBeamL = Rebeam(doc,beamL);
							if (startL==beamL) startL = newBeamL;
							if (doc->selStartL==beamL) doc->selStartL = newBeamL;
							
							nextL = RightLINK(LastInBeam(newBeamL));
							if (IsAfter(endL,nextL)) nextL = endL;
						}
					}

				}
			}
		}

	SetOttavaYPos(doc, ottavaL);

	if (IsAfter(ottavaL,doc->selStartL)) doc->selStartL = ottavaL;

	/* Clean up selection range: if we find any sync with a selected note, and the
		sync is not selected, deselect the entire node. This is OK since the selection
		range should have been correct when we entered the routine. */

	for (pL = RightLINK(startL); pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (NoteSEL(aNoteL) && !LinkSEL(pL))
					{ DeselectNode(pL); break; }
			}
		}
		else if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				if (GRNoteSEL(aNoteL) && !LinkSEL(pL))
					{ DeselectNode(pL); break; }
			}
		}
		
	if (doc->autoRespace)
		RespaceBars(doc, startL, endL, 0L, False, False);
	else
		InvalMeasures(startL, endL, staff);
	
	return ottavaL;
}


/* --------------------------------------------------------------- GetOctTypeNum -- */
/* Get octave-sign number corresponding to octSignType, and whether bassa. */

long GetOctTypeNum(LINK pL, Boolean *isBassa)
{
	POTTAVA p;

	p = GetPOTTAVA(pL);
	
	switch (p->octSignType) {
		case OTTAVA8va:
			*isBassa = False;
			return 8L;
		case OTTAVA15ma:
			*isBassa = False;
			return 15L;
		case OTTAVA22ma:
			*isBassa = False;
			return 22L;
		case OTTAVA8vaBassa:
			*isBassa = True;
			return 8L;
		case OTTAVA15maBassa:
			*isBassa = True;
			return 15L;
		case OTTAVA22maBassa:
			*isBassa = True;
			return 22L;
		default:
			MayErrMsg("GetOctTypeNum: illegal Ottava %ld", (long)p->octSignType);
			return 0L;
	}
}


/* ------------------------------------------------------------------ DrawOTTAVA -- */
/* Draw OTTAVA object. */

void DrawOTTAVA(Document *doc, LINK pL, CONTEXT context[])
{
	POTTAVA		p;
	short		staff;
	PCONTEXT	pContext;
	DDIST		dTop, dLeft, firstxd, lastxd,
				lastNoteWidth, yCutoffLen, dBrackMin; 
	DDIST		octxdFirst, octxdLast, octydFirst, octydLast,
				lnSpace, dhalfLn;
	DPoint		firstPt, lastPt;
	LINK		firstSyncL, lastSyncL, firstMeas, lastMeas;
	short		octxp, octyp;
	Rect		octRect;
	unsigned char ottavaStr[20];
	Boolean  	isBassa=False;
	long		number;
	short		octaveNumSize;

PushLock(OBJheap);
	p = GetPOTTAVA(pL);
	staff = p->staffn;
	pContext = &context[staff];
	MaySetPSMusSize(doc, pContext);
	lnSpace = LNSPACE(pContext);
	dhalfLn = lnSpace/2;

	dTop = pContext->measureTop;
	dLeft = pContext->measureLeft;
	firstSyncL = FirstInOttava(pL);
	lastSyncL = LastInOttava(pL);
	firstMeas = LSSearch(firstSyncL, MEASUREtype, ANYONE, GO_LEFT, False);
	lastMeas = LSSearch(lastSyncL, MEASUREtype, ANYONE, GO_LEFT, False);
	firstxd = dLeft+LinkXD(firstSyncL);
	lastxd = dLeft+LinkXD(lastSyncL);
	if (firstMeas!=lastMeas) lastxd += LinkXD(lastMeas)-LinkXD(firstMeas);
	yCutoffLen = (p->noCutoff? 0 : OTTAVA_CUTOFFLEN(lnSpace));

	number = GetOctTypeNum(pL, &isBassa);

	/* For the forseeable future, nxd and nyd are always zero, but we're keeping them
		 as potential offsets for moving the number independently of the octave sign. */
		
	octxdFirst = firstxd+p->xdFirst;
	octydFirst = dTop+p->ydFirst;
	lastNoteWidth = SymDWidthRight(doc, lastSyncL, staff, False, *pContext);
	octxdLast = lastxd+lastNoteWidth+p->xdLast;
	octydLast = dTop+p->ydLast;
	
	if (p->numberVis) {
		NumToSonataStr(number, ottavaStr);

		octRect = StrToObjRect(ottavaStr);
		OffsetRect(&octRect, d2p(octxdFirst), d2p(octydFirst));

		/*
		 * Draw bracket if it's long enough.
		 * If ottava alta, raise the bracket to about the top of the number.
		 */
		SetDPt(&firstPt, octxdFirst, octydFirst);
		SetDPt(&lastPt, octxdLast, octydLast);
		if (!isBassa) {
			firstPt.v -= 2*dhalfLn;
			lastPt.v -= 2*dhalfLn;
		}
		dBrackMin = 4*dhalfLn;
		if (lastPt.h-firstPt.h>dBrackMin)
			DrawOctBracket(firstPt, lastPt, octRect.right-octRect.left, yCutoffLen,
									isBassa, pContext);

		switch (outputTo) {
			case toScreen:
			case toBitmapPrint:
			case toPICT:
				TextSize(UseTextSize(pContext->fontSize, doc->magnify));
				octxp = pContext->paper.left+d2p(octxdFirst);
				octyp = pContext->paper.top+d2p(octydFirst);
				MoveTo(octxp, octyp);
				DrawString(ottavaStr);
				LinkOBJRECT(pL) = octRect;
				break;
			case toPostScript:
				octaveNumSize = (config.octaveNumSize>0? config.octaveNumSize : 110);
				PS_MusString(doc, octxdFirst, octydFirst, ottavaStr, octaveNumSize);
				break;
		}
	}
	
#ifdef USE_HILITESCORERANGE
	if (LinkSEL(pL))
		CheckOTTAVA(doc, pL, context, (Ptr)NULL, SMHilite, stfRange, enlarge);
#endif
PopLock(OBJheap);
}


/* Remove Ottavas in range [stfStartL,stfEndL) on staff s and make selection range
consistent. */

void UnOttava(Document *doc, LINK stfStartL, LINK stfEndL, short s)
{
	LINK newSelStart;

	newSelStart = doc->selStartL;
	if (BetweenIncl(stfStartL, doc->selStartL, stfEndL) && OttavaTYPE(doc->selStartL))
		newSelStart = FirstInOttava(doc->selStartL);

	UnOttavaS(doc, stfStartL, stfEndL, s);
	doc->selStartL = newSelStart;

	BoundSelRange(doc);
}


/* Remove Ottavas in range [fromL,toL) on staff s. */
 
static void UnOttavaS(Document *doc, LINK fromL, LINK toL, short s)
{
	LINK rOctL, lOctL, octToL, octFromL;
	short nInOttava;
	
	/* If any Ottavas cross the endpoints of the range, remove them first and,
		if possible, reOttava the portion outside the range. */

	rOctL = HasOttavaAcross(toL, s);
	lOctL = HasOttavaAcross(fromL, s);

	if (lOctL) {
		octFromL = FirstInOttava(lOctL);
		UnOttavaRange(doc, octFromL, RightLINK(LastInOttava(lOctL)), s);
		nInOttava = OctCountNotesInRange(s, octFromL, fromL, False);
		if (nInOttava>0)
			CreateOTTAVA(doc, octFromL, fromL, s, nInOttava, OTTAVA8va, False, False);
	}	

	if (rOctL)	{
		octToL = RightLINK(LastInOttava(rOctL));

		if (rOctL!=lOctL)
			UnOttavaRange(doc, FirstInOttava(rOctL), octToL, s);
		nInOttava = OctCountNotesInRange(s, toL, octToL, False);
		if (nInOttava>0)
			CreateOTTAVA(doc, toL, octToL, s, nInOttava, OTTAVA8va, False, False);
	}
	
	/* Main loop: un-Ottava every octaved note within the range. */

	UnOttavaRange(doc, fromL, toL, s);
}


static void UnOttavaSync(Document *doc, LINK octL, LINK pL, DDIST yDelta, short s,
									CONTEXT context)
{
	LINK aNoteL; 
	PANOTE aNote;
	Boolean stemDown, multiVoice;
	QDIST qStemLen;
	STDIST dystd;
	
	multiVoice = IsSyncMultiVoice(pL, s);

	/* Loop through the notes and set their yds and ystems. */
	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->staffn==s && aNote->inOttava) {
			aNote->inOttava = False;
			aNote->yd -= yDelta;
			aNote->yqpit -= halfLn2qd(noteOffset[OctType(octL)-1]);
			stemDown = GetStemInfo(doc, pL, aNoteL, &qStemLen);
			NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL), NFLAGS(NoteType(aNoteL)),
											stemDown, 
											context.staffHeight, context.staffLines,
											qStemLen, False);
			if (odd(noteOffset[OctType(octL)-1]))
				ToggleAugDotPos(doc, aNoteL, stemDown);
			dystd = -halfLn2std(noteOffset[OctType(octL)-1]);
			if (config.moveModNRs) MoveModNRs(aNoteL, dystd);
		}
	}

	/* Loop through the notes again and fix up the ystems of the non-extreme notes. */
	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->inChord && NoteSTAFF(aNoteL)==s) {
			FixSyncForChord(doc, pL, NoteVOICE(aNoteL), aNote->beamed, 0,
											(multiVoice? -1 : 1), &context);
			break;
		}
	}
}


static void UnOttavaGRSync(Document *doc, LINK octL, LINK pL, DDIST yDelta, short s,
									CONTEXT context)
{
	LINK aGRNoteL; 
	PAGRNOTE aGRNote;
	Boolean stemDown=False, multiVoice=False;
	short stemLen;
	
	/* Loop through the grace notes and set their yds and ystems. */
	aGRNoteL = FirstSubLINK(pL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (aGRNote->staffn==s && aGRNote->inOttava) {
			aGRNote->inOttava = False;
			aGRNote->yd -= yDelta;
			aGRNote->yqpit -= halfLn2qd(noteOffset[OctType(octL)-1]);
			stemLen = QSTEMLEN(!multiVoice, ShortenStem(aGRNoteL, context, stemDown));
			aGRNote->ystem = CalcYStem(doc, aGRNote->yd, NFLAGS(aGRNote->subType),
									   stemDown, context.staffHeight,
									   context.staffLines, stemLen, False);
		}
	}

	/* Loop through the grace notes again and fix up the ystems of the
		non-extreme notes. */
	aGRNoteL = FirstSubLINK(pL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (!aGRNote->rest && GRNoteSTAFF(aGRNoteL)==s) {
			if (aGRNote->inChord) {
				FixGRSyncForChord(doc, pL, GRNoteVOICE(aGRNoteL), aGRNote->beamed, 0,
									multiVoice, &context);
				break;
			}
		}
	}
}


/* -------------------------------------------------------------- RemoveOctOnStf -- */
/* Remove the given octave sign and change notes and grace notes it affected
accordingly. */

void RemoveOctOnStf(Document *doc,
							LINK octL, LINK fromL, LINK toL,
							short s)								/* staff no. */
{
	DDIST yDelta; CONTEXT context; LINK syncL, aNoteOttavaL;
	PANOTEOTTAVA aNoteOttava;

	/* Update note's y position. */
	GetContext(doc, octL, s, &context);
	yDelta = halfLn2d(noteOffset[OctType(octL)-1], context.staffHeight, context.staffLines);
							
	aNoteOttavaL = FirstSubLINK(octL);
	for ( ; aNoteOttavaL; aNoteOttavaL = NextNOTEOTTAVAL(aNoteOttavaL)) {
		aNoteOttava = GetPANOTEOTTAVA(aNoteOttavaL);
		syncL = aNoteOttava->opSync;

		if (SyncTYPE(syncL)) {
			UnOttavaSync(doc, octL, syncL, yDelta, s, context);
		}
		else if (GRSyncTYPE(syncL)) {
			UnOttavaGRSync(doc, octL, syncL, yDelta, s, context);
		}
	}
	
	DeleteNode(doc, octL);
	doc->changed = True;
	InvalMeasures(fromL, toL, s);
}

/* Find the octave sign Sync or GRSync pL belongs to and remove it. */

static void RemoveItsOttava(Document *doc, LINK fromL, LINK toL, LINK pL, short s)
{
	LINK aNoteL,aGRNoteL,ottavaL;
	PANOTE aNote; PAGRNOTE aGRNote;

	if (SyncTYPE(pL)) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->staffn==s && aNote->inOttava) {
				ottavaL = LSSearch(pL, OTTAVAtype, s, True, False);
				if (ottavaL==NILINK) {
					MayErrMsg("UnOttavaRange: can't find Ottava for note in sync %ld on staff %ld",(long)pL,(long)s);
					return;
				}
				RemoveOctOnStf(doc, ottavaL, fromL, toL, s);
			}
		}
	}
	else if (GRSyncTYPE(pL)) {
		aGRNoteL = FirstSubLINK(pL);
		for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
			aGRNote = GetPAGRNOTE(aGRNoteL);
			if (aGRNote->staffn==s && aGRNote->inOttava) {
				ottavaL = LSSearch(pL, OTTAVAtype, s, True, False);
				if (ottavaL==NILINK) {
					MayErrMsg("UnOttavaRange: can't find Ottava for note in sync %ld on staff %ld", (long)pL,(long)s);
					return;
				}
				RemoveOctOnStf(doc, ottavaL, fromL, toL, s);
			}
		}
	}
}

/* Remove Ottavas in the range from <from> to <toL> on <staff>. */

void UnOttavaRange(Document *doc, LINK fromL, LINK toL, short staff)
{
	LINK pL;
	
	for (pL = fromL; pL!=toL; pL = RightLINK(pL))
		if (SyncTYPE(pL) || GRSyncTYPE(pL))
			RemoveItsOttava(doc, fromL, toL, pL, staff);
}


/* ------------------------------------------------------ Miscellaneous Utilities -- */

/* ------------------------------------------------- FirstInOttava, LastInOttava -- */

LINK FirstInOttava(LINK ottavaL)
{
	PANOTEOTTAVA aNoteOct;
	
	aNoteOct = GetPANOTEOTTAVA(FirstSubLINK(ottavaL));
	return (aNoteOct->opSync);
}

LINK LastInOttava(LINK ottavaL)
{
	LINK aNoteOctL;
	PANOTEOTTAVA aNoteOct;
	short i, nInOttava;	
	
	nInOttava = LinkNENTRIES(ottavaL);
	aNoteOctL = FirstSubLINK(ottavaL);
	for (i=0; i<nInOttava; i++, aNoteOctL = NextNOTEOTTAVAL(aNoteOctL))
		aNoteOct = GetPANOTEOTTAVA(aNoteOctL);
	
	return (aNoteOct->opSync);
}


/* ---------------------------------------------------------- HasOctAcross, etc. -- */
/* Determines if there is an Ottava at <node> on <staff>. If <!skipNode>, checks
the point just before <node>; if <skipNode>, checks whether there's an Ottava
spanning <node>, ignoring <node> itself. The latter option is intended for use
when <node> is being inserted and the object list may not yet be consistent.
Returns the LINK to the Ottava if there is one, else NILINK. Looks for non-grace
Syncs only. */

LINK HasOctAcross(
				LINK node,
				short staff,
				Boolean skipNode	/* True=look for Ottava status to right of <node> & skip <node> itself */
				)	
{
	LINK lSyncL, rSyncL, lNoteL, rNoteL, ottavaL;
	PANOTE lNote, rNote;
	
	if (node==NILINK) return NILINK;	

	/* Get the syncs immediately to the right and left of the node. */
	
	lSyncL = LSSearch(LeftLINK(node), SYNCtype, staff, True, False);
	rSyncL = LSSearch((skipNode? RightLINK(node) : node), SYNCtype, staff, False, False);
	
	/* Check if they exist, and if they are inOttava; if not, return NILINK. */
	
	if (lSyncL==NILINK || rSyncL==NILINK) 
		return NILINK;

	lNoteL = FirstSubLINK(lSyncL);
	for ( ; lNoteL; lNoteL = NextNOTEL(lNoteL)) {
		lNote = GetPANOTE(lNoteL);
		if (lNote->staffn==staff)
			if (!lNote->inOttava) return NILINK;
			else break;
	}
	rNoteL = FirstSubLINK(rSyncL);
	for ( ; rNoteL; rNoteL = NextNOTEL(rNoteL)) {
		rNote = GetPANOTE(rNoteL);
		if (rNote->staffn==staff) {
			if (!rNote->inOttava) return NILINK;

			/* Otherwise, search for the right one's Ottava; if it is before the left
				sync, both syncs belong to it, so return it. Otherwise return NILINK. */

			ottavaL = LSSearch(rSyncL, OTTAVAtype, staff, True, False);
			return ( IsAfterIncl(ottavaL, lSyncL) ? ottavaL : NILINK );
		}
	}
	return NILINK;
}

/* Determines if there is an Ottava across the insertion point on <staff> into 
which <node> is inserted. Returns NILINK if there is not, and the LINK to the 
Ottava if there is. Looks for grace Syncs only. */

LINK HasGROctAcross(LINK node, short staff)	
{
	LINK lGRSyncL, rGRSyncL, lGRNoteL, rGRNoteL, ottavaL;
	PAGRNOTE lGRNote, rGRNote;
	
	if (node==NILINK) return NILINK;	

	/* Get the GRSyncs immediately to the right & left of the node. */
	
	lGRSyncL = LSSearch(LeftLINK(node), GRSYNCtype, staff, True, False);
	rGRSyncL = LSSearch(node, GRSYNCtype, staff, False, False);
	
	/* Check if they exist, and if they are inOttava; if not, return NILINK. */
	
	if (lGRSyncL==NILINK || rGRSyncL==NILINK) 
		return NILINK;

	lGRNoteL = FirstSubLINK(lGRSyncL);
	for ( ; lGRNoteL; lGRNoteL = NextGRNOTEL(lGRNoteL)) {
		lGRNote = GetPAGRNOTE(lGRNoteL);
		if (lGRNote->staffn==staff)
			if (!lGRNote->inOttava) return NILINK;
			else break;
	}
	rGRNoteL = FirstSubLINK(rGRSyncL);
	for ( ; rGRNoteL; rGRNoteL = NextGRNOTEL(rGRNoteL)) {
		rGRNote = GetPAGRNOTE(rGRNoteL);
		if (rGRNote->staffn==staff) {
			if (!rGRNote->inOttava) return NILINK;

			/* Otherwise, search for the right one's octave sign; if it is before the left
				GRSync, both belong to it, so return it. Otherwise return NILINK. */

			ottavaL = LSSearch(rGRSyncL, OTTAVAtype, staff, True, False);
			return ( IsAfterIncl(ottavaL, lGRSyncL) ? ottavaL : NILINK );
		}
	}
	return NILINK;
}


/* Determines if there is an Ottava across the insertion point on <staff> into 
which <node> is inserted. Returns NILINK if there is not, and the LINK to the 
Ottava, if there is. Handles both normal notes and grace notes--FIXME: but separately:
looks like it'll fail if there's an Ottava with normal note on one side and grace
note on the other. */

LINK HasOttavaAcross(LINK node, short staff)	
{
	LINK octL;

	octL = HasOctAcross(node,staff,False);
	if (octL) return octL;
	return (HasGROctAcross(node,staff));
}


/* If the node is a sync, return the Ottava if it has an inOttava note on <staff>,
else return NILINK. */

LINK OctOnStaff(LINK node, short staff)	
{
	LINK aNoteL; PANOTE aNote;
	
	if (!SyncTYPE(node)) return NILINK;

	aNoteL = FirstSubLINK(node);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
		if (NoteSTAFF(aNoteL)==staff) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->inOttava)
				return LSSearch(node, OTTAVAtype, staff, True, False);

			return NILINK;
		}

	return NILINK;
}


/* If the node is a GRsync, return the Ottava if it has an inOttava GRnote on 
<staff>, else return NILINK. */

LINK GROctOnStaff(LINK node, short staff)	
{
	LINK aGRNoteL; PANOTE aGRNote;
	
	if (!GRSyncTYPE(node)) return NILINK;

	aGRNoteL = FirstSubLINK(node);
	for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
		if (GRNoteSTAFF(aGRNoteL)==staff) {
			aGRNote = GetPAGRNOTE(aGRNoteL);
			if (aGRNote->inOttava)
				return LSSearch(node, OTTAVAtype, staff, True, False);

			return NILINK;
		}

	return NILINK;
}

/* Determines if there is an Ottava across the insertion point on <staff> into 
which <node> is inserted. Returns NILINK if there is not, and the LINK to the 
Ottava if there is. */

LINK HasOttavaAcrossIncl(LINK node, short staff)	
{
	if (node==NILINK) return NILINK;
	if (SyncTYPE(node))
		return OctOnStaff(node, staff);
	if (GRSyncTYPE(node))
		return GROctOnStaff(node, staff);
	return HasOttavaAcross(node, staff);
}


/* Determines if there is a regular Ottava across the location <pt> on <staff>. 
Returns NILINK if there is not, and the LINK to the Ottava if there is. */

static LINK HasOctAcrossPt(Document *doc, Point pt, short staff)
{
	LINK lSyncL, rSyncL, octL, octR;
	
	lSyncL = GSSearch(doc, pt, SYNCtype, staff, True, False, False, True);
	rSyncL = GSSearch(doc, pt, SYNCtype, staff, False, False, False, True);
	if (lSyncL && rSyncL) {
	
		octL = OctOnStaff(lSyncL, staff);
		octR = OctOnStaff(rSyncL, staff);
	
		if (octL==octR) return octL;
	}
	return NILINK;
}

/* Determines if there is a grace note Ottava across the location <pt> on <staff>. 
Returns NILINK if there is not, and the LINK to the Ottava if there is. */

static LINK HasGROctAcrossPt(Document *doc, Point pt, short staff)
{
	LINK lGRSyncL, rGRSyncL, octL, octR;
	
	lGRSyncL = GSSearch(doc, pt, GRSYNCtype, staff, True, False, False, True);
	rGRSyncL = GSSearch(doc, pt, GRSYNCtype, staff, False, False, False, True);
	if (lGRSyncL && rGRSyncL) {
	
		octL = GROctOnStaff(lGRSyncL, staff);
		octR = GROctOnStaff(rGRSyncL, staff);
	
		if (octL==octR) return octL;
	}
	return NILINK;
}

/* Determines if there is an Ottava across the location <pt> on <staff>. Returns
NILINK if there is not, and the LINK to the Ottava if there is. Handles both normal
notes and grace notes--FIXME: but separately: looks like it'll fail if there's an octave
sign with normal note on one side and grace note on the other. */

LINK HasOttavaAcrossPt(Document *doc, Point pt, short staff)
{
	LINK octL;

	octL = HasOctAcrossPt(doc, pt, staff);
	if (octL) return octL;
	return HasGROctAcrossPt(doc, pt, staff);
}


/* ----------------------------------------------------------- SelRangeChkOttava -- */

Boolean SelRangeChkOttava(short staff, LINK staffStartL, LINK staffEndL)
{
	LINK pL,aNoteL,aGRNoteL;
	PANOTE aNote; PAGRNOTE aGRNote;
	
	for (pL = staffStartL; pL!=staffEndL; pL = RightLINK(pL))
		if (LinkSEL(pL))
			if (SyncTYPE(pL)) {
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)==staff) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->inOttava) return True;
						else break;
					}
			}
			else if (GRSyncTYPE(pL)) {
				for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
					if (NoteSTAFF(aGRNoteL)==staff) {
						aGRNote = GetPAGRNOTE(aGRNoteL);
						if (aGRNote->inOttava) return True;
						else break;
					}
			}

	return False;
}


/* -------------------------------------------------------- OctCountNotesInRange -- */
/* Return the total number of notes/chords on stf in the range from both Syncs and
GRSyncs. Does NOT count rests. */

short OctCountNotesInRange(short stf,
							LINK startL, LINK endL,
							Boolean selectedOnly)		/* True=only selected notes/rests, False=all */
{
	short notesInRange;
	
	notesInRange = CountNotesInRange(stf, startL, endL, selectedOnly);
	notesInRange += CountGRNotesInRange(stf, startL, endL, selectedOnly);
	
	return notesInRange;
}


/* ------------------------------------------------------------ QD_DrawDashedLine -- */
/* Draw a horizontal dashed line via QuickDraw. y2 is ignored. */

#define DASH_LEN		4		/* in pixels */

void QD_DrawDashedLine(short x1, short y1, short x2, short /*y2*/)
{
	short i; Point pt;
	
	MoveTo(x1, y1);
	for (i = x1; i+DASH_LEN<x2; i+=2*DASH_LEN) {
		LineTo(i+DASH_LEN, y1);			/* Draw dash */
		MoveTo(i+2*DASH_LEN, y1);		/* Move to start of next dash */
	}
	GetPen(&pt);						/* Last MoveTo may have taken us too far. */
	if (pt.h<x2) LineTo(x2, y1);		/* If not, draw short final dash. */
}


/* -------------------------------------------------------------- DrawOctBracket -- */
/* Draw octave sign bracket. firstPt and lastPt are the coordinates of the start
and end points of the bracket, and octWidth is the width of the octave sign string.
For now, lastPt.v is ignored and the bracket is always horizontal. */

#define XFUDGE	4	/* in pixels or points */

void DrawOctBracket(
		DPoint firstPt, DPoint lastPt,
		short octWidth,			/* in pixels--FIXME: not good resolution: we really need DDIST */
		DDIST yCutoffLen,		/* unsigned length of cutoff line */
		Boolean isBassa,
		PCONTEXT pContext
		)
{
	short	firstx, firsty, lastx, yCutoff;
	DDIST	lnSpace;
	
	switch (outputTo) {
		case toScreen:
		case toBitmapPrint:
		case toPICT:
			firstx = pContext->paper.left+d2p(firstPt.h);
			firsty = pContext->paper.top+d2p(firstPt.v);
			lastx = pContext->paper.left+d2p(lastPt.h);
			yCutoff = d2p(yCutoffLen);
		
		/* Start octWidth to right of firstPt. Draw a dotted line to the end of the
		octave sign, and a solid line up or down yCutoff. */
			MoveTo(firstx+octWidth+XFUDGE, firsty);
			QD_DrawDashedLine(firstx+octWidth+XFUDGE, firsty, lastx, firsty);
			if (yCutoff!=0) {
				MoveTo(lastx, firsty);
				LineTo(lastx, firsty+(isBassa ? -yCutoff : yCutoff));
			}
			return;
		case toPostScript:
			lnSpace = LNSPACE(pContext);
			PS_HDashedLine(firstPt.h+p2d(octWidth+XFUDGE), firstPt.v, lastPt.h,
								OTTAVA_THICK(lnSpace), pt2d(4));
			if (yCutoffLen!=0) PS_Line(lastPt.h, firstPt.v, lastPt.h,
										firstPt.v+(isBassa ? -yCutoffLen : yCutoffLen),
										OTTAVA_THICK(lnSpace));
			return;
		default:
			;
	}
}

/* Null function to allow loading or unloading Ottava.c's segment. */

void XLoadOttavaSeg()
{
}
