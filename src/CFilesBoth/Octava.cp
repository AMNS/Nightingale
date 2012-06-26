/* File Octava.c - octave-sign-related functions - rev. for v.3.5. NB: this
file frequently uses the word "octava"; also, the object it deals with is called
the "Octava". Unfortunately, there's no such word in Italian--the actual word is
"ottava".  Sorry. */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

/*
	DoOctava					DoRemoveOctava			GetNoteyd
	SetOctavaYPos			CreateOCTAVA			UnOctava
	UnOctavaS
	UnOctavaSync			UnOctavaGRSync			UnOctavaRange
	GetOctTypeNum			DrawOCTAVA
	FirstInOctava 			LastInOctava
	HasOctAcross			HasOctavaAcross 		OctOnStaff	
	HasOctavaAcrossIncl
	DashedLine				DrawOctBracket			XLoadOctavaSeg		
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Prototypes for internal functions */
static void CreateOctStfRange(Document *doc,short s,LINK stfStartL,LINK stfEndL,Byte octType);
static LINK GetOctStartL(LINK startL, short staff, Boolean needSel);
static DDIST GetNoteyd(LINK syncL,short staff);
static void UnOctava(Document *, LINK, LINK, short);
static void UnOctavaS(Document *, LINK, LINK, short);
static void UnOctavaSync(Document *, LINK,LINK,DDIST,short,CONTEXT);
static void UnOctavaGRSync(Document *, LINK,LINK,DDIST,short,CONTEXT);
static void RemoveItsOctava(Document *doc,LINK fromL, LINK toL,LINK pL,short s);
static LINK HasGROctAcross(LINK node,short staff);
static LINK HasOctAcrossPt(Document *, Point, short);
static LINK HasGROctAcrossPt(Document *, Point, short);


/* -------------------------------------------------------------------- DoOctava -- */
/* Put an octave sign around the selection. */

void DoOctava(Document *doc)
{
	short staff;
	LINK staffStartL, staffEndL;
	Byte octSignType;

	if (OctavaDialog(doc, &octSignType)) {
		PrepareUndo(doc, doc->selStartL, U_Octava, 31);    	/* "Undo Create Ottava" */
	
		for (staff = 1; staff<=doc->nstaves; staff++) {
			GetStfSelRange(doc, staff, &staffStartL, &staffEndL);
			CreateOctStfRange(doc,staff,staffStartL,staffEndL,octSignType);
		}
	}
}

/* -------------------------------------------------------------- DoRemoveOctava -- */
/* Remove the octave sign from the selection. */

void DoRemoveOctava(Document *doc)
{
	LINK		startL, endL, staffStartL, staffEndL;
	short		staff;
	
	GetOptSelEnds(doc, &startL, &endL);
	PrepareUndo(doc, startL, U_UnOctava, 32);    				/* "Undo Remove Ottava" */

	for (staff = 1; staff<=doc->nstaves; staff++) {
		GetStfSelRange(doc, staff, &staffStartL, &staffEndL);
		if (!staffStartL || !staffEndL) continue;
		if (SelRangeChkOctava(staff, staffStartL, staffEndL))
			UnOctava(doc, staffStartL, staffEndL, staff);
	}
		
	if (doc->autoRespace)
		RespaceBars(doc, doc->selStartL, doc->selEndL, 0L, FALSE, FALSE);
}

/* ------------------------------------------------------------ CreateOctStfRange -- */
/* Create octave sign for notes from stfStartL to stfEndL, excluding stfEndL,
on staff <s>. */

static void CreateOctStfRange(Document *doc, short s, LINK stfStartL, LINK stfEndL,
								Byte octType)
{
	short nInOctava; LINK octavaL;

	nInOctava = OctCountNotesInRange(s, stfStartL, stfEndL, TRUE);

	if (nInOctava>0) {
		octavaL = CreateOCTAVA(doc,stfStartL,stfEndL,s,nInOctava,octType,TRUE,TRUE);
		if (doc->selStartL==FirstInOctava(octavaL))
			doc->selStartL = octavaL;
	}
}

/* ----------------------------------------------------------------- GetOctStartL -- */
/* Get new startL in order to insert octave sign into object list before first
sync or GRSync to right of <startL>. */

static LINK GetOctStartL(LINK startL, short staff, Boolean needSel)
{
	LINK octStartL,GRStartL;

	octStartL = LSSearch(startL, SYNCtype, staff, FALSE, needSel);
	GRStartL = LSSearch(startL, GRSYNCtype, staff, FALSE, needSel);

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
								short staff)		/* Either Sync or GRSync */
{
	LINK aNoteL,aGRNoteL;
	PANOTE aNote; PAGRNOTE aGRNote;

	if (SyncTYPE(syncL)) {
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==staff && MainNote(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				return aNote->yd;
			}
	}
	else if (GRSyncTYPE(syncL)) {
		aGRNoteL = FirstSubLINK(syncL);
		for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
			if (GRNoteSTAFF(aGRNoteL)==staff && GRMainNote(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL);
				return aGRNote->yd;
			}
	}
	return 0;
}

/* --------------------------------------------------------------- SetOctavaYPos -- */
/* Set the given octave sign to (a guess at) a reasonable vertical position, based
on the position of the MainNote of its first note/chord. (It'd surely be better to
look at all of its notes/chords, and not hard.) */ 

#define STANDOFFALTA 1			/* Minimum half-line distance between octave sign and staff */
#define STANDOFFBASSA 2			/* Minimum half-line distance between octave sign and staff */

#define MARGINALTA	3			/* Preferred half-line distance between octave sign and note */
#define MARGINBASSA	5

#define BRACKALTA_LIM(yd)	((yd)>octAltaYLim? octAltaYLim : (yd))
#define BRACKBASSA_LIM(yd)	((yd)<octBassaYLim? octBassaYLim : (yd))

void SetOctavaYPos(Document *doc, LINK octL)
{
	POCTAVA	pOct;
	short		staff;
	CONTEXT	context;
	DDIST		firstyd, margin; 
	DDIST		octydFirst,
				octAltaYLim, octBassaYLim,			/* Closest possible positions to the staff */
				lnSpace, dhalfLn;
	LINK		firstSyncL;
	Boolean  bassa;

	staff = OctavaSTAFF(octL);
	GetContext(doc, octL, staff, &context);
	lnSpace = LNSPACE(&context);
	dhalfLn = lnSpace/2;

	firstSyncL = FirstInOctava(octL);
	firstyd = GetNoteyd(firstSyncL, staff);

	octAltaYLim = -STANDOFFALTA*dhalfLn;
	octBassaYLim = context.staffHeight + STANDOFFBASSA*dhalfLn;

	pOct = GetPOCTAVA(octL);
	switch (pOct->octSignType) {
		case OCTAVA8va:
		case OCTAVA15ma:
		case OCTAVA22ma:
			bassa = FALSE;
			break;
		case OCTAVA8vaBassa:
		case OCTAVA15maBassa:
		case OCTAVA22maBassa:
			bassa = TRUE;
			break;
		default:
			;
	}

	/*
	 * Try to position the octave sign <margin> half-lines away from the 1st MainNote,
	 *	but always put it at least <STANDOFFALTA/BASSA> away from the staff.
	 */
 	if (bassa) margin = MARGINBASSA*dhalfLn;
 	else		  margin = MARGINALTA*dhalfLn;
	if (bassa) octydFirst = BRACKBASSA_LIM(firstyd+margin);
	else		  octydFirst = BRACKALTA_LIM(firstyd-margin);
	
	pOct = GetPOCTAVA(octL);
	pOct->ydLast = pOct->ydFirst = octydFirst;
}


/* ---------------------------------------------------------------- CreateOCTAVA -- */
/* Create octave sign in range [startL,endL) on staff <staff>. */

LINK CreateOCTAVA(
				Document *doc,
				LINK startL, LINK endL,
				short staff, short nInOctava,
				Byte octSignType,
				Boolean needSelected,	/* TRUE if we only want selected items */
				Boolean doOct				/* TRUE if we are explicitly octaving notes (so Octava will be selected) */
				)
{
	POCTAVA 		octavap;
	LINK			pL, octavaL, aNoteOctL, nextL, mainNoteL, beamL,
					newBeamL, mainGRNoteL;
	PANOTEOCTAVA aNoteOct;
	short 		octElem, iOctElem,  v;
	PANOTE		aNote;
	PAGRNOTE		aGRNote;
	LINK			aNoteL,aGRNoteL;
	LINK			opSync[MAXINOCTAVA];
	LINK			noteInSync[MAXINOCTAVA];
	DDIST			yDelta;
	CONTEXT		context;
	Boolean 		stemDown, multiVoice;
	short			stemLen;
	QDIST			qStemLen;
	STDIST		dystd;	
	char			fmtStr[256];
	
	if (nInOctava<1) {
		MayErrMsg("CreateOCTAVA: tried to make Octava with %ld notes/chords.", (long)nInOctava);
		return NILINK;
	}
	if (nInOctava>MAXINOCTAVA) {
		GetIndCString(fmtStr, OCTAVEERRS_STRS, 1);    /* "The maximum number of notes/chords in an octave sign is %d" */
		sprintf(strBuf, fmtStr, MAXINBEAM);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return NILINK;
	}
	
	/* Insert Octava into object list before first Sync or GRSync to right
		of <startL>. */
	startL = GetOctStartL(startL, staff, needSelected);
	
	octavaL = InsertNode(doc, startL, OCTAVAtype, nInOctava);
	if (!octavaL) { NoMoreMemory(); return NILINK; }
	SetObject(octavaL, 0, 0, FALSE, TRUE, FALSE);
	octavap = GetPOCTAVA(octavaL);
	octavap->tweaked = FALSE;
	octavap->staffn = staff;
	octavap->octSignType = octSignType;

	if (doOct) octavap->selected = TRUE;

	octavap->numberVis = TRUE;
	octavap->brackVis = FALSE;

	octavap->nxd = octavap->nyd = 0;
	octavap->xdFirst = octavap->xdLast = 0;

	octavap = GetPOCTAVA(octavaL);
	octavap->noCutoff = octavap->crossStaff = octavap->crossSystem = FALSE;	
	octavap->filler = 0;
	octavap->unused1 = octavap->unused2 = 0;

	doc->changed = TRUE;

	for (octElem = 0, pL = startL; pL!=endL; pL = RightLINK(pL))
		if (!needSelected || LinkSEL(pL)) {
			if (SyncTYPE(pL)) {
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->staffn==staff && !aNote->rest) {																/* Yes */
						aNote->inOctava = TRUE;
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
						aGRNote->inOctava = TRUE;
						if (GRMainNote(aGRNoteL)) {
							opSync[octElem] = pL;
							noteInSync[octElem] = aGRNoteL;
							octElem++;
						}
					}
				}
			}
		}

	if (octElem!=nInOctava) {
		MayErrMsg("CreateOCTAVA expected %ld notes but found %ld.",(long)nInOctava,(long)octElem);
		return NILINK;
	}
	
	iOctElem=0;
	aNoteOctL = FirstSubLINK(octavaL);
	for ( ; iOctElem<octElem; iOctElem++,aNoteOctL=NextNOTEOCTAVAL(aNoteOctL)) {
		aNoteOct = GetPANOTEOCTAVA(aNoteOctL);
		aNoteOct->opSync = opSync[iOctElem];
	}	

	/* Update notes' y position. */
	
	GetContext(doc, octavaL, staff, &context);
	yDelta = halfLn2d(noteOffset[octSignType-1],context.staffHeight,
							context.staffLines);

	for (pL = startL; pL!=endL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			multiVoice = GetMultiVoice(pL, staff); 		/* Are there multiple voices on staff? */
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
														qStemLen, FALSE);
					}
					else
						NoteYSTEM(aNoteL) = NoteYD(aNoteL);
					
					if (odd(noteOffset[octSignType-1]))
						ToggleAugDotPos(doc, aNoteL, stemDown);	/* ??BUG: stemDown wrong for single notes! */

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
			multiVoice = FALSE; 							/* ??For now; needs GetGRMultiVoice! */
			/* Loop through the grace notes, fixing up notehead, stem, and aug.
				dot y-positions. */
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL);
				if (GRNoteSTAFF(aGRNoteL)==staff) {
					stemDown = FALSE;							/* Assume grace notes stem up--??not always true */
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
													stemLen, FALSE);
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
					mainNoteL = FindMainNote(pL,v);
					if (mainNoteL && NoteBEAMED(mainNoteL)) {
						beamL = LVSearch(pL,BEAMSETtype,v,GO_LEFT,FALSE);
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
				/* ??BLOCKS ARE WRONG. FOLLOWING "else" IS *INSIDE* THE "if (SyncTYPE(pL))"
					SO IT'LL NEVER DO ANYTHING. BUT GRACE BEAMS GET REBEAMED ANYWAY. ?? */
				else if (GRSyncTYPE(pL)) {
					mainGRNoteL = FindGRMainNote(pL,v);
					if (mainGRNoteL && GRNoteBEAMED(mainGRNoteL)) {
						beamL = LVSearch(pL,BEAMSETtype,v,GO_LEFT,FALSE);
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

	SetOctavaYPos(doc, octavaL);

	if (IsAfter(octavaL,doc->selStartL)) doc->selStartL = octavaL;

	/* Clean up selection range: if we find any sync with a selected note,
		and the sync is not selected, deselect the entire node. This is OK
		since the selection range should have been correct when we entered the
		routine. */

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
		RespaceBars(doc, startL, endL, 0L, FALSE, FALSE);
	else
		InvalMeasures(startL, endL, staff);
	
	return octavaL;
}


/* --------------------------------------------------------------- GetOctTypeNum -- */
/* Get octave-sign number corresponding to octSignType, and whether bassa. */

long GetOctTypeNum(LINK pL, Boolean *bassa)
{
	POCTAVA p;

	p = GetPOCTAVA(pL);
	
	switch (p->octSignType) {
		case OCTAVA8va:
			*bassa = FALSE;
			return 8L;
		case OCTAVA15ma:
			*bassa = FALSE;
			return 15L;
		case OCTAVA22ma:
			*bassa = FALSE;
			return 22L;
		case OCTAVA8vaBassa:
			*bassa = TRUE;
			return 8L;
		case OCTAVA15maBassa:
			*bassa = TRUE;
			return 15L;
		case OCTAVA22maBassa:
			*bassa = TRUE;
			return 22L;
		default:
			MayErrMsg("GetOctTypeNum: illegal Octava %ld", (long)p->octSignType);
			return 0L;
	}
}


/* ------------------------------------------------------------------ DrawOCTAVA -- */
/* Draw OCTAVA object. */

void DrawOCTAVA(Document *doc, LINK pL, CONTEXT context[])
{
	POCTAVA	p;
	short		staff;
	PCONTEXT	pContext;
	DDIST		dTop, dLeft, firstxd, lastxd,
				lastNoteWidth, yCutoffLen, dBrackMin; 
	DDIST		octxdFirst, octxdLast, octydFirst, octydLast,
				lnSpace, dhalfLn;
	DPoint	firstPt, lastPt;
	LINK		firstSyncL, lastSyncL, firstMeas, lastMeas;
	short		octxp, octyp;
	Rect		octRect;
	unsigned char octavaStr[20];
	Boolean  bassa;
	long		number;
	short		octaveNumSize;

PushLock(OBJheap);
	p = GetPOCTAVA(pL);
	staff = p->staffn;
	pContext = &context[staff];
	MaySetPSMusSize(doc, pContext);
	lnSpace = LNSPACE(pContext);
	dhalfLn = lnSpace/2;

	dTop = pContext->measureTop;
	dLeft = pContext->measureLeft;
	firstSyncL = FirstInOctava(pL);
	lastSyncL = LastInOctava(pL);
	firstMeas = LSSearch(firstSyncL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	lastMeas = LSSearch(lastSyncL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
	firstxd = dLeft+LinkXD(firstSyncL);
	lastxd = dLeft+LinkXD(lastSyncL);
	if (firstMeas!=lastMeas)
		lastxd += LinkXD(lastMeas)-LinkXD(firstMeas);
	yCutoffLen = (p->noCutoff? 0 : OCT_CUTOFFLEN(lnSpace));

	number = GetOctTypeNum(pL, &bassa);

	/* For the forseeable future, nxd and nyd are always zero, but we're keeping them
		 as potential offsets for moving the number independently of the octave sign. */
		
	octxdFirst = firstxd+p->xdFirst;
	octydFirst = dTop+p->ydFirst;
	lastNoteWidth = SymDWidthRight(doc, lastSyncL, staff, FALSE, *pContext);
	octxdLast = lastxd+lastNoteWidth+p->xdLast;
	octydLast = dTop+p->ydLast;
	
	if (p->numberVis) {
		NumToSonataStr(number,octavaStr);

		octRect = StrToObjRect(octavaStr);
		OffsetRect(&octRect, d2p(octxdFirst), d2p(octydFirst));

		/*
		 * Draw bracket if it's long enough.
		 * If ottava alta, raise the bracket to about the top of the number.
		 */
		SetDPt(&firstPt, octxdFirst, octydFirst);
		SetDPt(&lastPt, octxdLast, octydLast);
		if (!bassa) {
			firstPt.v -= 2*dhalfLn;
			lastPt.v -= 2*dhalfLn;
		}
		dBrackMin = 4*dhalfLn;
		if (lastPt.h-firstPt.h>dBrackMin)
			DrawOctBracket(firstPt, lastPt, octRect.right-octRect.left,
									yCutoffLen, bassa, pContext);

		switch (outputTo) {
			case toScreen:
			case toImageWriter:
			case toPICT:
				TextSize(UseTextSize(pContext->fontSize, doc->magnify));
				octxp=pContext->paper.left+d2p(octxdFirst);
				octyp=pContext->paper.top+d2p(octydFirst);
				MoveTo(octxp, octyp);
				DrawString(octavaStr);
				LinkOBJRECT(pL) = octRect;
				break;
			case toPostScript:
				octaveNumSize = (config.octaveNumSize>0? config.octaveNumSize : 110);
				PS_MusString(doc, octxdFirst, octydFirst, octavaStr, octaveNumSize);
				break;
		}
	}
	
#ifdef USE_HILITESCORERANGE
	if (LinkSEL(pL))
		CheckOCTAVA(doc, pL, context, (Ptr)NULL, SMHilite, stfRange, enlarge);
#endif
PopLock(OBJheap);
}


/* Remove Octavas in range [stfStartL,stfEndL) on staff s and make selection range
consistent. */

void UnOctava(Document *doc, LINK stfStartL, LINK stfEndL, short s)
{
	LINK newSelStart;

	newSelStart = doc->selStartL;
	if (BetweenIncl(stfStartL,doc->selStartL,stfEndL) && OctavaTYPE(doc->selStartL))
		newSelStart = FirstInOctava(doc->selStartL);

	UnOctavaS(doc, stfStartL, stfEndL, s);
	doc->selStartL = newSelStart;

	BoundSelRange(doc);
}

/* Remove Octavas in range [fromL,toL) on staff s. */
 
static void UnOctavaS(Document *doc, LINK fromL, LINK toL, short s)
{
	LINK rOctL, lOctL, octToL, octFromL;
	short nInOctava;
	
	/* If any Octavas cross the endpoints of the range, remove them first and,
		if possible, reOctava the portion outside the range. */

	rOctL = HasOctavaAcross(toL, s);
	lOctL = HasOctavaAcross(fromL, s);

	if (lOctL) {
		octFromL = FirstInOctava(lOctL);
		UnOctavaRange(doc, octFromL, RightLINK(LastInOctava(lOctL)), s);
		nInOctava = OctCountNotesInRange(s, octFromL, fromL, FALSE);
		if (nInOctava>0)
			CreateOCTAVA(doc, octFromL, fromL, s, nInOctava, OCTAVA8va, FALSE, FALSE);
	}	

	if (rOctL)	{
		octToL = RightLINK(LastInOctava(rOctL));

		if (rOctL!=lOctL)
			UnOctavaRange(doc, FirstInOctava(rOctL), octToL, s);
		nInOctava = OctCountNotesInRange(s, toL, octToL, FALSE);
		if (nInOctava>0)
			CreateOCTAVA(doc, toL, octToL, s, nInOctava, OCTAVA8va, FALSE, FALSE);
	}
	
	/* Main loop: un-Octava every octaved note within the range. */

	UnOctavaRange(doc, fromL, toL, s);
}

static void UnOctavaSync(Document *doc, LINK octL, LINK pL, DDIST yDelta, short s,
									CONTEXT context)
{
	LINK aNoteL; 
	PANOTE aNote;
	Boolean stemDown, multiVoice;
	QDIST qStemLen;
	STDIST dystd;
	
	/* Determine if there are multiple voices on s. */
	multiVoice = GetMultiVoice(pL, s);

	/* Loop through the notes and set their yds and ystems. */
	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->staffn==s && aNote->inOctava) {
			aNote->inOctava = FALSE;
			aNote->yd -= yDelta;
			aNote->yqpit -= halfLn2qd(noteOffset[OctType(octL)-1]);
			stemDown = GetStemInfo(doc, pL, aNoteL, &qStemLen);
			NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL), NFLAGS(NoteType(aNoteL)),
												stemDown, 
												context.staffHeight, context.staffLines,
												qStemLen, FALSE);
			if (odd(noteOffset[OctType(octL)-1]))
				ToggleAugDotPos(doc, aNoteL, stemDown);
			dystd = -halfLn2std(noteOffset[OctType(octL)-1]);
			if (config.moveModNRs) MoveModNRs(aNoteL, dystd);
		}
	}

	/* Loop through the notes again and fix up the ystems of the
		non-extreme notes. */
	aNoteL = FirstSubLINK(pL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->inChord && NoteSTAFF(aNoteL)==s) {
			FixSyncForChord(doc,pL,NoteVOICE(aNoteL),aNote->beamed,0,
											(multiVoice? -1 : 1), &context);
			break;
		}
	}
}

static void UnOctavaGRSync(Document *doc, LINK octL, LINK pL, DDIST yDelta, short s,
									CONTEXT context)
{
	LINK aGRNoteL; 
	PAGRNOTE aGRNote;
	Boolean stemDown=FALSE, multiVoice=FALSE;
	short stemLen;
	
	/* Loop through the grace notes and set their yds and ystems. */
	aGRNoteL = FirstSubLINK(pL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (aGRNote->staffn==s && aGRNote->inOctava) {
			aGRNote->inOctava = FALSE;
			aGRNote->yd -= yDelta;
			aGRNote->yqpit -= halfLn2qd(noteOffset[OctType(octL)-1]);
			stemLen = QSTEMLEN(!multiVoice, ShortenStem(aGRNoteL, context, stemDown));
			aGRNote->ystem = CalcYStem(doc, aGRNote->yd,
											NFLAGS(aGRNote->subType),
											stemDown,
											context.staffHeight, context.staffLines,
											stemLen, FALSE);
		}
	}

	/* Loop through the grace notes again and fix up the ystems of the
		non-extreme notes. */
	aGRNoteL = FirstSubLINK(pL);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (!aGRNote->rest && GRNoteSTAFF(aGRNoteL)==s) {
			if (aGRNote->inChord) {
				FixGRSyncForChord(doc,pL,GRNoteVOICE(aGRNoteL),aGRNote->beamed,0,
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
							short s)										/* staff no. */
{
	DDIST yDelta; CONTEXT context; LINK syncL, aNoteOctavaL;
	PANOTEOCTAVA aNoteOctava;

	/* Update note's y position. */
	GetContext(doc, octL, s, &context);
	yDelta = halfLn2d(noteOffset[OctType(octL)-1],context.staffHeight,context.staffLines);
							
	aNoteOctavaL = FirstSubLINK(octL);
	for ( ; aNoteOctavaL; aNoteOctavaL = NextNOTEOCTAVAL(aNoteOctavaL)) {
		aNoteOctava = GetPANOTEOCTAVA(aNoteOctavaL);
		syncL = aNoteOctava->opSync;

		if (SyncTYPE(syncL)) {
			UnOctavaSync(doc,octL,syncL,yDelta,s,context);
		}
		else if (GRSyncTYPE(syncL)) {
			UnOctavaGRSync(doc,octL,syncL,yDelta,s,context);
		}
	}
	
	DeleteNode(doc, octL);
	doc->changed = TRUE;
	InvalMeasures(fromL, toL, s);
}

/* Find the octave sign Sync or GRSync pL belongs to and remove it. */

static void RemoveItsOctava(Document *doc, LINK fromL, LINK toL, LINK pL, short s)
{
	LINK aNoteL,aGRNoteL,octavaL;
	PANOTE aNote; PAGRNOTE aGRNote;

	if (SyncTYPE(pL)) {
		aNoteL = FirstSubLINK(pL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->staffn==s && aNote->inOctava) {
				octavaL = LSSearch(pL, OCTAVAtype, s, TRUE, FALSE);
				if (octavaL==NILINK) {
					MayErrMsg("UnOctavaRange: can't find Octava for note in sync %ld on staff %ld",(long)pL,(long)s);
					return;
				}
				RemoveOctOnStf(doc,octavaL,fromL,toL,s);
			}
		}
	}
	else if (GRSyncTYPE(pL)) {
		aGRNoteL = FirstSubLINK(pL);
		for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
			aGRNote = GetPAGRNOTE(aGRNoteL);
			if (aGRNote->staffn==s && aGRNote->inOctava) {
				octavaL = LSSearch(pL, OCTAVAtype, s, TRUE, FALSE);
				if (octavaL==NILINK) {
					MayErrMsg("UnOctavaRange: can't find Octava for note in sync %ld on staff %ld", (long)pL,(long)s);
					return;
				}
				RemoveOctOnStf(doc,octavaL,fromL,toL,s);
			}
		}
	}
}

/* Remove Octavas in the range from <from> to <toL> on <staff>. */

void UnOctavaRange(Document *doc, LINK fromL, LINK toL, short staff)
{
	LINK pL;
	
	for (pL = fromL; pL!=toL; pL = RightLINK(pL))
		if (SyncTYPE(pL) || GRSyncTYPE(pL))
			RemoveItsOctava(doc, fromL, toL, pL, staff);
}


/* ------------------------------------------------------ Miscellaneous Utilities -- */

/* ------------------------------------------------- FirstInOctava, LastInOctava -- */

LINK FirstInOctava(LINK octavaL)
{
	PANOTEOCTAVA aNoteOct;
	
	aNoteOct = GetPANOTEOCTAVA(FirstSubLINK(octavaL));
	return (aNoteOct->opSync);
}

LINK LastInOctava(LINK octavaL)
{
	LINK aNoteOctL;
	PANOTEOCTAVA aNoteOct;
	short i, nInOctava;	
	
	nInOctava = LinkNENTRIES(octavaL);
	aNoteOctL = FirstSubLINK(octavaL);
	for (i=0; i<nInOctava; i++, aNoteOctL = NextNOTEOCTAVAL(aNoteOctL))
		aNoteOct = GetPANOTEOCTAVA(aNoteOctL);
	
	return (aNoteOct->opSync);
}

/* ---------------------------------------------------------- HasOctAcross, etc. -- */
/* Determines if there is an Octava at <node> on <staff>. If <!skipNode>, checks
the point just before <node>; if <skipNode>, checks whether there's an Octava
spanning <node>, ignoring <node> itself. The latter option is intended for use
when <node> is being inserted and the object list may not yet be consistent.
Returns the LINK to the Octava if there is one, else NILINK. Looks for non-grace
Syncs only. */

LINK HasOctAcross(
				LINK node,
				short staff,
				Boolean skipNode	/* TRUE=look for Octava status to right of <node> & skip <node> itself */
				)	
{
	LINK lSyncL, rSyncL, lNoteL, rNoteL, octavaL;
	PANOTE lNote, rNote;
	
	if (node==NILINK) return NILINK;	

	/* Get the syncs immediately to the right and left of the node. */
	
	lSyncL = LSSearch(LeftLINK(node), SYNCtype, staff, TRUE, FALSE);
	rSyncL = LSSearch((skipNode? RightLINK(node) : node), SYNCtype, staff, FALSE, FALSE);
	
	/* Check if they exist, and if they are inOctava; if not, return NILINK. */
	
	if (lSyncL==NILINK || rSyncL==NILINK) 
		return NILINK;

	lNoteL = FirstSubLINK(lSyncL);
	for ( ; lNoteL; lNoteL = NextNOTEL(lNoteL)) {
		lNote = GetPANOTE(lNoteL);
		if (lNote->staffn==staff)
			if (!lNote->inOctava) return NILINK;
			else break;
	}
	rNoteL = FirstSubLINK(rSyncL);
	for ( ; rNoteL; rNoteL = NextNOTEL(rNoteL)) {
		rNote = GetPANOTE(rNoteL);
		if (rNote->staffn==staff) {
			if (!rNote->inOctava) return NILINK;

			/* Otherwise, search for the right one's Octava; if it is before the left
				sync, both syncs belong to it, so return it. Otherwise return NILINK. */

			octavaL = LSSearch(rSyncL, OCTAVAtype, staff, TRUE, FALSE);
			return ( IsAfterIncl(octavaL, lSyncL) ? octavaL : NILINK );
		}
	}
	return NILINK;
}

/* Determines if there is an Octava across the insertion point on <staff> into 
which <node> is inserted. Returns NILINK if there is not, and the LINK to the 
Octava, if there is. Looks for grace Syncs only. */

LINK HasGROctAcross(LINK node, short staff)	
{
	LINK lGRSyncL, rGRSyncL, lGRNoteL, rGRNoteL, octavaL;
	PAGRNOTE lGRNote, rGRNote;
	
	if (node==NILINK) return NILINK;	

	/* Get the GRSyncs immediately to the right & left of the node. */
	
	lGRSyncL = LSSearch(LeftLINK(node), GRSYNCtype, staff, TRUE, FALSE);
	rGRSyncL = LSSearch(node, GRSYNCtype, staff, FALSE, FALSE);
	
	/* Check if they exist, and if they are inOctava; if not, return NILINK. */
	
	if (lGRSyncL==NILINK || rGRSyncL==NILINK) 
		return NILINK;

	lGRNoteL = FirstSubLINK(lGRSyncL);
	for ( ; lGRNoteL; lGRNoteL = NextGRNOTEL(lGRNoteL)) {
		lGRNote = GetPAGRNOTE(lGRNoteL);
		if (lGRNote->staffn==staff)
			if (!lGRNote->inOctava) return NILINK;
			else break;
	}
	rGRNoteL = FirstSubLINK(rGRSyncL);
	for ( ; rGRNoteL; rGRNoteL = NextGRNOTEL(rGRNoteL)) {
		rGRNote = GetPAGRNOTE(rGRNoteL);
		if (rGRNote->staffn==staff) {
			if (!rGRNote->inOctava) return NILINK;

			/* Otherwise, search for the right one's octave sign; if it is before the left
				GRSync, both belong to it, so return it. Otherwise return NILINK. */

			octavaL = LSSearch(rGRSyncL, OCTAVAtype, staff, TRUE, FALSE);
			return ( IsAfterIncl(octavaL, lGRSyncL) ? octavaL : NILINK );
		}
	}
	return NILINK;
}


/* Determines if there is an Octava across the insertion point on <staff> into 
which <node> is inserted. Returns NILINK if there is not, and the LINK to the 
Octava, if there is. Handles both normal notes and grace notes--??but separately:
looks like it'll fail if there's an Octava with normal note on one side and
grace note on the other. */

LINK HasOctavaAcross(LINK node, short staff)	
{
	LINK octL;

	if (octL = HasOctAcross(node,staff,FALSE)) return octL;
	
	return (HasGROctAcross(node,staff));
}


/* If the node is a sync, return the Octava if it has an inOctava note on <staff>,
else return NILINK. */

LINK OctOnStaff(LINK node, short staff)	
{
	LINK aNoteL; PANOTE aNote;
	
	if (!SyncTYPE(node)) return NILINK;

	aNoteL = FirstSubLINK(node);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
		if (NoteSTAFF(aNoteL)==staff) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->inOctava)
				return LSSearch(node, OCTAVAtype, staff, TRUE, FALSE);

			return NILINK;
		}

	return NILINK;
}


/* If the node is a GRsync, return the Octava if it has an inOctava GRnote on 
<staff>, else return NILINK. */

LINK GROctOnStaff(LINK node, short staff)	
{
	LINK aGRNoteL; PANOTE aGRNote;
	
	if (!GRSyncTYPE(node)) return NILINK;

	aGRNoteL = FirstSubLINK(node);
	for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
		if (GRNoteSTAFF(aGRNoteL)==staff) {
			aGRNote = GetPAGRNOTE(aGRNoteL);
			if (aGRNote->inOctava)
				return LSSearch(node, OCTAVAtype, staff, TRUE, FALSE);

			return NILINK;
		}

	return NILINK;
}

/* Determines if there is an Octava across the insertion point on <staff> into 
which <node> is inserted. Returns NILINK if there is not, and the LINK to the 
Octava if there is. */

LINK HasOctavaAcrossIncl(LINK node, short staff)	
{
	if (node==NILINK) return NILINK;
	if (SyncTYPE(node))
		return OctOnStaff(node, staff);
	if (GRSyncTYPE(node))
		return GROctOnStaff(node, staff);
	return HasOctavaAcross(node, staff);
}


/* Determines if there is a regular Octava across the location <pt> on <staff>. 
Returns NILINK if there is not, and the LINK to the Octava if there is. */

static LINK HasOctAcrossPt(Document *doc, Point pt, short staff)
{
	LINK lSyncL, rSyncL, octL, octR;
	
	lSyncL = GSSearch(doc, pt, SYNCtype, staff, TRUE, FALSE, FALSE, TRUE);
	rSyncL = GSSearch(doc, pt, SYNCtype, staff, FALSE, FALSE, FALSE, TRUE);
	if (lSyncL && rSyncL) {
	
		octL = OctOnStaff(lSyncL, staff);
		octR = OctOnStaff(rSyncL, staff);
	
		if (octL==octR) return octL;
	}
	return NILINK;
}

/* Determines if there is a grace note Octava across the location <pt> on <staff>. 
Returns NILINK if there is not, and the LINK to the Octava if there is. */

static LINK HasGROctAcrossPt(Document *doc, Point pt, short staff)
{
	LINK lGRSyncL, rGRSyncL, octL, octR;
	
	lGRSyncL = GSSearch(doc, pt, GRSYNCtype, staff, TRUE, FALSE, FALSE, TRUE);
	rGRSyncL = GSSearch(doc, pt, GRSYNCtype, staff, FALSE, FALSE, FALSE, TRUE);
	if (lGRSyncL && rGRSyncL) {
	
		octL = GROctOnStaff(lGRSyncL, staff);
		octR = GROctOnStaff(rGRSyncL, staff);
	
		if (octL==octR) return octL;
	}
	return NILINK;
}

/* Determines if there is an Octava across the location <pt> on <staff>. Returns
NILINK if there is not, and the LINK to the Octava if there is. Handles both normal
notes and grace notes--??but separately: looks like it'll fail if there's an octave
sign with normal note on one side and grace note on the other. */

LINK HasOctavaAcrossPt(Document *doc, Point pt, short staff)
{
	LINK octL;

	if (octL = HasOctAcrossPt(doc, pt, staff))
		return octL;
	return HasGROctAcrossPt(doc, pt, staff);
}

/* ----------------------------------------------------------- SelRangeChkOctava -- */

Boolean SelRangeChkOctava(short staff, LINK staffStartL, LINK staffEndL)
{
	LINK pL,aNoteL,aGRNoteL;
	PANOTE aNote; PAGRNOTE aGRNote;
	
	for (pL = staffStartL; pL!=staffEndL; pL = RightLINK(pL))
		if (LinkSEL(pL))
			if (SyncTYPE(pL)) {
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)==staff) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->inOctava) return TRUE;
						else break;
					}
			}
			else if (GRSyncTYPE(pL)) {
				for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL))
					if (NoteSTAFF(aGRNoteL)==staff) {
						aGRNote = GetPAGRNOTE(aGRNoteL);
						if (aGRNote->inOctava) return TRUE;
						else break;
					}
			}

	return FALSE;
}

/* -------------------------------------------------------- OctCountNotesInRange -- */
/* Return the total number of notes/chords on stf in the range from both Syncs and
GRSyncs. Does NOT count rests. */

short OctCountNotesInRange(short stf,
									LINK startL, LINK endL,
									Boolean selectedOnly)		/* TRUE=only selected notes/rests, FALSE=all */
{
	short notesInRange;
	
	notesInRange = CountNotesInRange(stf, startL, endL, selectedOnly);
	notesInRange += CountGRNotesInRange(stf, startL, endL, selectedOnly);
	
	return notesInRange;
}


/* ------------------------------------------------------------------ DashedLine -- */
/* Draw a horizontal dashed line via QuickDraw. y2 is ignored. */

#define DASH_LEN		4

void DashedLine(short x1, short y1, short x2, short /*y2*/)
{
	short i; Point pt;
	
	MoveTo(x1, y1);
	for (i = x1; i+DASH_LEN<x2; i+=2*DASH_LEN) {
		LineTo(i+DASH_LEN, y1);			/* Draw dash */
		MoveTo(i+2*DASH_LEN, y1);		/* Move to start of next dash */
	}
	GetPen(&pt);							/* Last MoveTo may have taken us too far. */
	if (pt.h<x2) LineTo(x2, y1);		/* If not, draw short final dash. */
}


/* -------------------------------------------------------------- DrawOctBracket -- */
/* Draw octave sign bracket. firstPt and lastPt are the coordinates of the start
and end points of the bracket, and octWidth is the width of the octave sign string.
For now, lastPt.v is ignored and the bracket is always horizontal. */

#define XFUDGE		4

void DrawOctBracket(
		DPoint firstPt, DPoint lastPt,
		short octWidth,			/* in pixels--not good resolution: we really need DDIST */
		DDIST yCutoffLen,			/* unsigned length of cutoff line */
		Boolean bassa,
		PCONTEXT pContext
		)
{
	short	firstx, firsty, lastx, yCutoff;
	DDIST	lnSpace;
	
	switch (outputTo) {
		case toScreen:
		case toImageWriter:
		case toPICT:
			firstx = pContext->paper.left+d2p(firstPt.h);
			firsty = pContext->paper.top+d2p(firstPt.v);
			lastx = pContext->paper.left+d2p(lastPt.h);
			yCutoff = d2p(yCutoffLen);
		
		/* Start octWidth to right of firstPt. Draw a dotted line to the end of the
		octave sign, and a solid line up or down yCutoff. */
			MoveTo(firstx+octWidth+XFUDGE, firsty);
			DashedLine(firstx+octWidth+XFUDGE, firsty, lastx, firsty);
			if (yCutoff!=0) {
				MoveTo(lastx, firsty);
				LineTo(lastx, firsty+(bassa ? -yCutoff : yCutoff));
			}
			return;
		case toPostScript:
			lnSpace = LNSPACE(pContext);
			PS_HDashedLine(firstPt.h+p2d(octWidth+XFUDGE), firstPt.v, lastPt.h,
								OCT_THICK(lnSpace), pt2d(4));
			if (yCutoffLen!=0) PS_Line(lastPt.h, firstPt.v, lastPt.h,
										firstPt.v+(bassa ? -yCutoffLen : yCutoffLen),
										OCT_THICK(lnSpace));
			return;
		default:
			;
	}
}

/* Null function to allow loading or unloading Octava.c's segment. */

void XLoadOctavaSeg()
{
}
