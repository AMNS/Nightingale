/****************************************************************************************
	FILE:	Context.c
	PROJ:	Nightingale
	DESC:	Context-related routines. There should be no user-interface effects;
	at the moment, some routines can Inval, which should be changed.
		ContextKeySig				ContextMeasure				Context1Staff
		ContextStaff				ContextSystem				ContextTimeSig				
		GetContext					GetAllContexts
		ClefFixBeamContext			EFixContForClef				FixContextForClef
		EFixContForKeySig			FixContextForKeySig			EFixContForTimeSig
		FixContextForTimeSig		FixContextForDynamic		FixMeasureContext
		FixStaffContext				UpdateKSContext				UpdateTSContext
		CombineTables				InitPitchModTable			CopyTables
		EndMeasRangeSearch			EFixAccsForKeySig			FixAccsForKeySig
/****************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ------------------------------------------------------------------ ContextClef -- */
/* Update the current context using this Clef object. */

void ContextClef(LINK pL, CONTEXT context[])
{
	LINK	aClefL;

	for (aClefL=FirstSubLINK(pL); aClefL; aClefL=NextCLEFL(aClefL)) {
		/* aClef = GetPACLEF(aClefL); */
		if (ClefVIS(aClefL))
			context[ClefSTAFF(aClefL)].clefType = ClefType(aClefL);
	}
}


/* ---------------------------------------------------------------- ContextKeySig -- */
/*	Update the current context using this KeySig object. */

void ContextKeySig(LINK pL, CONTEXT	context[])
{
	LINK		aKeySigL;
	PCONTEXT	pContext;
	short		k;

	aKeySigL = FirstSubLINK(pL);
	for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
		/* aKeySig = GetPAKEYSIG(aKeySigL); */
		if (KeySigVIS(aKeySigL)) {
			pContext = &context[KeySigSTAFF(aKeySigL)];
			pContext->nKSItems = KeySigNKSITEMS(aKeySigL);			/* Copy this key sig. */
			for (k = 0; k<KeySigNKSITEMS(aKeySigL); k++)			/*    into the context */
				pContext->KSItem[k] = (KeySigKSITEM(aKeySigL))[k];
		}
	}
}


/* ---------------------------------------------------------------- ContextMeasure -- */
/*	Update the current context using this Measure object. */

void ContextMeasure(LINK pL, CONTEXT context[])
{
	LINK			aMeasureL;
	PCONTEXT		pContext;

	aMeasureL = FirstSubLINK(pL);
	for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
		/* aMeasure = GetPAMEASURE(aMeasureL); */
		pContext = &context[MeasureSTAFF(aMeasureL)];
		pContext->measureVisible = pContext->visible = 
			(MeasureMEASUREVIS(aMeasureL) && pContext->staffVisible);
		pContext->measureTop = pContext->staffTop;
		pContext->measureLeft = pContext->staffLeft + LinkXD(pL);
	}
}


/* ---------------------------------------------------------------- Context1Staff -- */

void Context1Staff(LINK aStaffL, PCONTEXT pContext)
{
	pContext->staffTop = StaffTOP(aStaffL) + pContext->systemTop;
	pContext->staffLeft = StaffLEFT(aStaffL) + pContext->systemLeft;
	pContext->staffRight = StaffRIGHT(aStaffL) + pContext->systemLeft;
	pContext->staffVisible = pContext->visible = StaffVIS(aStaffL);
	pContext->staffLines = StaffSTAFFLINES(aStaffL);
	pContext->showLines = StaffSHOWLINES(aStaffL);
	pContext->showLedgers = StaffSHOWLEDGERS(aStaffL);
#ifdef STAFFRASTRAL
	pContext->srastral = StaffRASTRAL(aStaffL);
#endif
	pContext->staffHeight = StaffHEIGHT(aStaffL);
	pContext->staffHalfHeight = StaffHEIGHT(aStaffL)>>1;
	pContext->fontSize = StaffFONTSIZE(aStaffL);
}


/* ----------------------------------------------------------------- ContextStaff -- */
/*	Update the current context using this Staff object. */

void ContextStaff(LINK pL, CONTEXT context[])
{
	LINK		aStaffL;

	aStaffL = FirstSubLINK(pL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		Context1Staff(aStaffL, &context[StaffSTAFF(aStaffL)]);
	}
}


/* ---------------------------------------------------------------- ContextSystem -- */
/*	Update the current context using this System object. */

void ContextSystem(LINK pL, CONTEXT	context[])
{
	PSYSTEM		p;
	PCONTEXT	pContext;
	short		i;

	p = GetPSYSTEM(pL);
	pContext = context;
	for (i=0; i<=MAXSTAVES; i++, pContext++) {
		pContext->systemTop = p->systemRect.top;
		pContext->systemLeft = p->systemRect.left;
		pContext->systemBottom = p->systemRect.bottom;
		pContext->visible = False;
	}
}


/* --------------------------------------------------------------- ContextTimeSig -- */
/*	Update the current context using this TimeSig object.	*/

void ContextTimeSig(LINK pL, CONTEXT context[])
{
	LINK		aTimeSigL;
	PCONTEXT	pContext;

	aTimeSigL = FirstSubLINK(pL);
	for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
		/* aTimeSig = GetPATIMESIG(aTimeSigL); */
		if (TimeSigVIS(aTimeSigL)) {
			pContext = &context[TimeSigSTAFF(aTimeSigL)];
			pContext->numerator = TimeSigNUMER(aTimeSigL);
			pContext->denominator = TimeSigDENOM(aTimeSigL);
		}
	}
}


/* ------------------------------------------------------------------ ContextPage -- */
/*	Update the current context using this Page object. */

void ContextPage(Document *doc, LINK pL, CONTEXT context[])
{
	PPAGE p; PCONTEXT pContext; short i;

	p = GetPPAGE(pL);
	doc->currentSheet = p->sheetNum;
	GetSheetRect(doc,doc->currentSheet,&doc->currentPaper);
	
	for (pContext=context,i=0; i<=MAXSTAVES; i++, pContext++) {
		pContext->sheetNum = p->sheetNum;
		pContext->paper = doc->currentPaper;
	}
}


/* ------------------------------------------------------------------- GetContext -- */
/*	Get the context at (including) the specified object on the specified staff.
This is done by searching backwards for a Staff or Measure object, taking note of
any Clef, KeySig, TimeSig, or Dynamic objects found along the way. Warning: in
general, context at an object before the first Measure will have undefined fields! */

void GetContext(Document *doc, LINK contextL, short theStaff, PCONTEXT pContext)
{
	register LINK pL;
	PMEASURE	pMeasure;
	PSTAFF		pStaff;
	PSYSTEM		pSystem;
	PPAGE		pPage;
	LINK		staffL, systemL, aMeasureL, aStaffL, aClefL, aKeySigL,
				aTimeSigL, aDynamicL, pageL, headL;
	short		k;
	Boolean		found,			/* False till a Measure or Staff is found for theStaff */
				foundClef,		/* True if we found any of these objects */
				foundKeySig,
				foundTimeSig,
				foundDynamic;

	if (contextL==NILINK) {
		MayErrMsg("GetContext: NILINK contextL, staff=%ld.", (long)theStaff);
		return;
	}
	
	if (STAFFN_BAD(doc, theStaff)) {
		MayErrMsg("GetContext: illegal staff number %ld. contextL=%ld", (long)theStaff,
					(long)contextL);
		return;
	}

	pL = contextL;
	headL = doc->masterView ? doc->masterHeadL : doc->headL;

	/* Initialize in case nothing is found. */
	pContext->visible = False;
	pContext->fontSize = 12;
	pContext->clefType = DFLT_CLEF;
	pContext->nKSItems = DFLT_NKSITEMS;
	pContext->timeSigType = DFLT_TSTYPE;
	pContext->numerator = config.defaultTSNum;
	pContext->denominator = config.defaultTSDenom;
	pContext->dynamicType = DFLT_DYNAMIC;
	
	found = False;
	foundClef = foundKeySig = foundTimeSig = foundDynamic = False;
	
	for ( ; !found && pL && pL!=headL; pL=LeftLINK(pL)) {
		switch (ObjLType(pL)) {
			case STAFFtype:
				found = True;
				pageL = LSSearch(pL, PAGEtype, ANYONE, GO_LEFT, False);
				if (!pageL) {
					MayErrMsg("GetContext: can't find Page before L%ld. contextL=%ld, staff=%ld",
											(long)pL, (long)contextL, (long)theStaff);
					return;
				}
				pPage = GetPPAGE(pageL);
				pContext->sheetNum = pPage->sheetNum;
				GetSheetRect(doc, pPage->sheetNum, &pContext->paper);
				pContext->inMeasure = False;

				pStaff = GetPSTAFF(pL);
				systemL = pStaff->systemL;

				/* GET SYSTEM CONTEXT INFO */
				pSystem = GetPSYSTEM(systemL);
				pContext->systemTop = pSystem->systemRect.top;
				pContext->systemLeft = pSystem->systemRect.left;
				pContext->systemBottom = pSystem->systemRect.bottom;
				pContext->visible = False;

				/* GET STAFF CONTEXT INFO */
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
					if (StaffSTAFF(aStaffL) == theStaff) {
						Context1Staff(aStaffL, pContext);
						/* aStaff = GetPASTAFF(aStaffL); */
						if (!foundClef)
							pContext->clefType = StaffCLEFTYPE(aStaffL);
						if (!foundKeySig) {
							if (StaffNKSITEMS(aStaffL)<0 || StaffNKSITEMS(aStaffL)>MAX_KSITEMS) {
								MayErrMsg("GetContext: Staff L%ld has illegal nKSItems %ld. staff=%ld",
											(long)pL, (long)(StaffNKSITEMS(aStaffL)), (long)theStaff);
								return;
							}
							pContext->nKSItems = StaffNKSITEMS(aStaffL);		/* Copy this key sig. */
							for (k = 0; k<StaffNKSITEMS(aStaffL); k++)			/*    into the context */
								pContext->KSItem[k] = (StaffKSITEM(aStaffL))[k];
						}
						if (!foundTimeSig) {
							pContext->timeSigType = StaffTIMESIGTYPE(aStaffL);
							pContext->numerator = StaffNUMER(aStaffL);
							pContext->denominator = StaffDENOM(aStaffL);
						}
						if (!foundDynamic)
							pContext->dynamicType = StaffDynamType(aStaffL);
					}
				}
				break;
			case MEASUREtype:
				found = True;
				pageL = LSSearch(pL, PAGEtype, ANYONE, GO_LEFT, False);
				if (!pageL) {
					MayErrMsg("GetContext: can't find Page before L%ld. contextL=%ld, staff=%ld",
											(long)pL, (long)contextL, (long)theStaff);
					return;
				}
				pPage = GetPPAGE(pageL);
				pContext->sheetNum = pPage->sheetNum;
				GetSheetRect(doc,pPage->sheetNum,&pContext->paper);
				pContext->inMeasure = True;

				pMeasure = GetPMEASURE(pL);
				staffL = pMeasure->staffL; pStaff = GetPSTAFF(staffL);
				systemL = pMeasure->systemL; pSystem = GetPSYSTEM(systemL);

				/* GET SYSTEM CONTEXT INFO */
				pContext->systemTop = pSystem->systemRect.top;
				pContext->systemLeft = pSystem->systemRect.left;
				pContext->systemBottom = pSystem->systemRect.bottom;
				pContext->visible = False;

				/* GET STAFF CONTEXT INFO */
				for (aStaffL=FirstSubLINK(staffL); aStaffL;
					aStaffL = NextSTAFFL(aStaffL)) {
					/* aStaff = GetPASTAFF(aStaffL); */
					if (StaffSTAFFN(aStaffL) == theStaff) {
						pContext->staffTop = StaffTOP(aStaffL) + pContext->systemTop;
						pContext->staffLeft = StaffLEFT(aStaffL) + pContext->systemLeft;
						pContext->staffRight = StaffRIGHT(aStaffL) + pContext->systemLeft;
						pContext->staffVisible =
							pContext->visible = StaffVIS(aStaffL);
						pContext->staffLines = StaffSTAFFLINES(aStaffL);
						pContext->showLines = StaffSHOWLINES(aStaffL);
						pContext->showLedgers = StaffSHOWLEDGERS(aStaffL);
#ifdef STAFFRASTRAL
						pContext->srastral = StaffRASTRAL(aStaffL);
#endif
						pContext->staffHeight = StaffHEIGHT(aStaffL);
						pContext->staffHalfHeight = StaffHEIGHT(aStaffL)>>1;
						pContext->fontSize = StaffFONTSIZE(aStaffL);
					}
				}

				/* GET MEASURE CONTEXT INFO */
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
					/* aMeasure = GetPAMEASURE(aMeasureL); */
					if (MeasureSTAFF(aMeasureL) == theStaff) {
						pContext->measureVisible = pContext->visible = 
							(MeasureMEASUREVIS(aMeasureL) && pContext->staffVisible);
						pContext->measureTop = pContext->staffTop;
						pContext->measureLeft = pContext->staffLeft + LinkXD(pL);
						if (!foundClef)
							pContext->clefType = MeasCLEFTYPE(aMeasureL);
						if (!foundKeySig) {
							if (MeasNKSITEMS(aMeasureL)<0 || MeasNKSITEMS(aMeasureL)>MAX_KSITEMS) {
								MayErrMsg("GetContext: Measure L%ld has illegal nKSItems %ld. staff=%ld",
											(long)pL, (long)(MeasNKSITEMS(aMeasureL)), (long)theStaff);
								return;
							}
							pContext->nKSItems = MeasNKSITEMS(aMeasureL);		/* Copy this key sig. */
							for (k = 0; k<MeasNKSITEMS(aMeasureL); k++)			/*    into the context */
								pContext->KSItem[k] = (MeasKSITEM(aMeasureL))[k];
						}
						if (!foundTimeSig) {
							pContext->timeSigType = MeasTIMESIGTYPE(aMeasureL);
							pContext->numerator = MeasNUMER(aMeasureL);
							pContext->denominator = MeasDENOM(aMeasureL);
						}
						if (!foundDynamic)
							pContext->dynamicType = MeasDynamType(aMeasureL);
					}
				}
				break;
			case KEYSIGtype:
				if (!foundKeySig) {
					aKeySigL = FirstSubLINK(pL);
					for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
						/* aKeySig = GetPAKEYSIG(aKeySigL); */
						if (KeySigSTAFF(aKeySigL) == theStaff) {
							if (KeySigNKSITEMS(aKeySigL)<0 || KeySigNKSITEMS(aKeySigL)>MAX_KSITEMS) {
								MayErrMsg("GetContext: KeySig L%ld has illegal nKSItems %ld. staff=%ld",
											(long)pL, (long)(KeySigNKSITEMS(aKeySigL)), (long)theStaff);
								return;
							}
							pContext->nKSItems = KeySigNKSITEMS(aKeySigL);		/* Copy this key sig. */
							for (k = 0; k<KeySigNKSITEMS(aKeySigL); k++)		/*    into the context */
								pContext->KSItem[k] = (KeySigKSITEM(aKeySigL))[k];
							foundKeySig = True;
						}
					}
				}
				break;
			case TIMESIGtype:
				if (!foundTimeSig) {
					aTimeSigL = FirstSubLINK(pL);
					for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
						/* aTimeSig = GetPATIMESIG(aTimeSigL); */
						if (TimeSigSTAFF(aTimeSigL) == theStaff) {
							pContext->timeSigType = TimeSigType(aTimeSigL);
							pContext->numerator = TimeSigNUMER(aTimeSigL);
							pContext->denominator = TimeSigDENOM(aTimeSigL);
							foundTimeSig = True;
						}
					}
				}
				break;
			case CLEFtype:
				if (!foundClef) {
					aClefL = FirstSubLINK(pL);
					for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
						/* aClef = GetPACLEF(aClefL); */
						if (ClefSTAFF(aClefL) == theStaff) {
							pContext->clefType = ClefType(aClefL);
							foundClef = True;
						}
					}
				}
				break;
			case DYNAMtype:
				if (!foundDynamic) {
					aDynamicL = FirstSubLINK(pL);
					for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL)) {
						/* aDynamic = GetPADYNAMIC(aDynamicL); */
						
						/* Hairpins don't affect dynamic context, at least for now. */
						if (DynamicSTAFF(aDynamicL) == theStaff
						&&  DynamType(pL)<FIRSTHAIRPIN_DYNAM) {
							pContext->dynamicType = DynamType(pL);
							foundDynamic = True;
						}
					}
				}
				break;
			default:
				;
		}
	}
}


/* ---------------------------------------------------------------- GetAllContexts -- */
/*	Fill in the specified context[] array at a particular point in the score. */

void GetAllContexts(Document *doc, CONTEXT context[], LINK pAtL)
{
	short staff;

#ifdef MAYBE_SAFER
	if (LinkBefFirstMeas(pAtL))
		pAtL = LSSearch(pAtL,MEASUREtype,ANYONE,GO_RIGHT,False);
#endif

	if (pAtL)
		for (staff=1; staff<=doc->nstaves; staff++)
			GetContext(doc, pAtL, staff, &context[staff]);
}


/* ----------------------------------------------------------- ClefFixBeamContext -- */
/* Remove and re-beam every beamset that has elements in the given range on the given
staff. This was written to handle a change of clef context. */

void ClefFixBeamContext(Document *doc, LINK startL, LINK doneL, short staffn)
{
	LINK firstBeamL=NILINK, tempL, prevBeamL;
	LINK pL, notes[MAXINBEAM];
	short v;
	PBEAMSET pBeam;
	STFRANGE stfRange;

#ifdef QUESTIONABLE
	/* First, look to see if a crossStaff beam on another staff affecting the given staff
	 *	is before the relevant beam on the given staff. If so, use this as the starting pt.
	 */
	if (staffn > 1)	/* ??CAN'T BE RIGHT--EVEN STAFF 1 MIGHT HAVE XSTF BEAM TO STF 2 */
		firstBeamL = LSSearch(startL, BEAMSETtype, staffn-1, GO_LEFT, False);	/* 1st beam that might be affected */
	if (firstBeamL) {
		firstBeam = GetPBEAMSET(firstBeamL);
		if (firstBeam->crossStaff) {
			tempBeamL = LSSearch(startL, BEAMSETtype, staffn, GO_LEFT, False);	/* 1st beam that might be affected */
			if (tempBeamL)
				if (IsAfter(tempBeamL, firstBeamL))
					firstBeamL = tempBeamL;
		}
		else firstBeamL = NILINK;
	}
#endif

	/* The beamset object always precedes its first Sync, so we may have to re-beam
	 *	before the beginning of the range. If we do not already have a first beam link,
	 *	get one on this staff, test to see if its Syncs or grace Syncs extend into the
	 *	range, and set firstBeamL accordingly: use <startL> if the beam is not in the
	 *	range, <firstBeamL> if it is.
	 */
	if (!firstBeamL)
		firstBeamL = LSSearch(startL, BEAMSETtype, staffn, GO_LEFT, False);	/* 1st beam that might be affected */
	
	if (firstBeamL) {
		if (IsAfter(LastInBeam(firstBeamL), startL))						/* Is it really affected? */
			firstBeamL = startL;											/* No */
	}
	else
		 firstBeamL = startL;												/* No beams before range on staff */

	/* Now remove and recreate all beamsets in the (possibly extended) range. Rebeam
		can recreate the beamset object to the right of its original position in the
		data structure, so we have to be careful to avoid processing beamsets
		repeatedly. */
	
	for (v = 1; v<=MAXVOICES; v++)
		if (VOICE_MAYBE_USED(doc, v)) {
			for (prevBeamL = NILINK, pL = firstBeamL; pL!=doneL; pL=tempL) {
				tempL = RightLINK(pL);
				if (BeamsetTYPE(pL) && BeamVOICE(pL)==v && pL!=prevBeamL) {
					pBeam = GetPBEAMSET(pL);
					if (pBeam->crossStaff) {
						GetBeamNotes(pL, notes);
						GetCrossStaff(LinkNENTRIES(pL), notes, &stfRange);
						if (stfRange.topStaff==staffn || stfRange.bottomStaff==staffn)
							prevBeamL = Rebeam(doc, pL);
					}
					else if (BeamSTAFF(pL)==staffn)
						prevBeamL = Rebeam(doc, pL);
				}
			}
		}
}


/* -------------------------------------------------------- ToggleAugDotPosInRange -- */
/* Toggle augmentation dot y-positions for all notes in the given range and staff. */

static void ToggleAugDotPosInRange(Document *doc, LINK startL, LINK endL, short staffn);
static void ToggleAugDotPosInRange(Document *doc, LINK startL, LINK endL, short staffn)
{
	LINK pL, mainNoteL, aNoteL; short voice;
	Boolean stemDown;
	
	for (pL = startL; pL!=endL; pL=RightLINK(pL)) {
		if (SyncTYPE(pL)) {
			// ??NEED TO HANDLE EACH VOICE IN <pL> ON THE STAFF SEPARATELY!
			voice = SyncVoiceOnStaff(pL, staffn);
			if (voice==NOONE) continue;
			mainNoteL = FindMainNote(pL, voice);
			if (!mainNoteL) continue;
			
			stemDown = (NoteYSTEM(mainNoteL) > NoteYD(mainNoteL));

			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				if (!NoteREST(aNoteL) && NoteSTAFF(aNoteL)==staffn)
					ToggleAugDotPos(doc, aNoteL, stemDown);
			}
		}
	}
}


/* -------------------------------------------------------------- EFixContForClef -- */
/* In the range [startL,doneL), update (1) the clef in context fields of following
STAFFs and MEASUREs for the given staff, (2) note y-positions, (3) note stem directions
and chord layouts including stem directions, and (4) beam positions to match the new
note positions. If a clef change is found on the staff before doneL, we stop there.
MAY inval some or all of the range (via Rebeam) ??and InvalContent: should change this.

Updates aug. dot vertical positions if the clef changes to/from on of the normal treble,
bass, and C (in any position) clefs to/from any of the octave-displaced clefs or
vice-versa. ??MEFixContForClef should do the same thing. */

void EFixContForClef(
				Document *doc, LINK startL, LINK doneL,
				short staffn,
				SignedByte oldClef, SignedByte newClef,	/* Previously-effective and new clefTypes */
				CONTEXT context
				)
{
	short		voice, yPrev, yHere, qStemLen;
	DDIST		yDelta;
	LINK		pL, aClefL, aNoteL, aGRNoteL, aStaffL, aMeasureL;
	Boolean		stemDown;
	STDIST		dystd;
	
	/*
	 * Update <context> to reflect the clef change. This is necessary to get
	 * the correct stem direction from GetCStemInfo.
	 */
	context.clefType = newClef;
	
	yPrev = ClefMiddleCHalfLn(oldClef);
	yHere = ClefMiddleCHalfLn(newClef);
	yDelta = halfLn2d(yHere-yPrev, context.staffHeight, context.staffLines);
	dystd = halfLn2std(yHere-yPrev);

	for (pL = startL; pL!=doneL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					/* aNote = GetPANOTE(aNoteL); */
					if (!NoteREST(aNoteL) && NoteSTAFF(aNoteL)==staffn) {
						NoteYD(aNoteL) += yDelta;
						if (config.moveModNRs) MoveModNRs(aNoteL, dystd);
						if (NoteINCHORD(aNoteL)) {
							voice = NoteVOICE(aNoteL);
							FixSyncForChord(doc, pL, voice, NoteBEAMED(aNoteL), 0, 0, NULL);
							}
						else {
							stemDown = GetCStemInfo(doc, pL, aNoteL, context, &qStemLen);
							NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL),
															NFLAGS(NoteType(aNoteL)),
															stemDown,
															context.staffHeight, context.staffLines,
															qStemLen, False);
						}
					}
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
					/* aGRNote = GetPAGRNOTE(aGRNoteL); */
					if (GRNoteSTAFF(aGRNoteL)==staffn) {
						GRNoteYD(aGRNoteL) += yDelta;
						if (GRNoteINCHORD(aGRNoteL))
							FixGRSyncForChord(doc, pL, GRNoteVOICE(aGRNoteL), GRNoteBEAMED(aGRNoteL), 0,
													True, NULL);
						else
							FixGRSyncNote(doc, pL, GRNoteVOICE(aGRNoteL), &context);
					}
				}
				break;
			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
					if (StaffSTAFF(aStaffL)==staffn) {
						/* aStaff = GetPASTAFF(aStaffL); */
						StaffCLEFTYPE(aStaffL) = newClef; 
					}
				break;
			case CLEFtype:
				aClefL = FirstSubLINK(pL);
				for ( ; aClefL; aClefL = NextCLEFL(aClefL))
					if (ClefSTAFFN(aClefL)==staffn) {
						if (!ClefINMEAS(pL)) {
							/* aClef = GetPACLEF(aClefL); */
							ClefType(aClefL) = newClef;
						}
						else {
							doneL = pL;
							goto Cleanup;
						}
					}
				break;
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL))
					if (MeasureSTAFF(aMeasureL)==staffn) {
						/* aMeasure = GetPAMEASURE(aMeasureL); */
						MeasCLEFTYPE(aMeasureL) = newClef;
					} 
				break;
			default:
				;
		}

Cleanup:
	/*
	 * At this point, <doneL> may have been updated; in any case, it reflects the
	 * range actually affected by the clef change.
	 */

	if (odd(yHere-yPrev))
		ToggleAugDotPosInRange(doc, startL, doneL, staffn);
	
	InvalContent(startL, doneL);
	ClefFixBeamContext(doc, startL, doneL, staffn);
}


/* ----------------------------------------------------------- FixContextForClef --- */
/* From (and including) <startL> until the next clef for the staff, update (1) the
clef in context fields of following STAFFs and MEASUREs for the given staff, (2)
notes' y-positions, (3) note stem directions and chord layouts including stem
directions, and (4) beam positions to match the new note positions. Returns the
first LINK after the range affected or, if nothing is affected, NILINK. */

LINK FixContextForClef(
				Document *doc, LINK startL, short staffn,
				SignedByte oldClef, SignedByte newClef)		/* Previously-effective and new clefTypes */
{
	LINK			doneL;
	CONTEXT		context;
	SearchParam	pbSearch;

	/* If nothing following or no clef change, quit.
	 * If startL itself is a clef, no change propagates past it, so we're done.
	 * ??YES, BUT ONLY IF IT'S A CLEF ON OUR STAFF! CF. FixContextForKeySig.
	 */
	if (!startL || startL==doc->tailL || oldClef==newClef) return NILINK;							

	if (ClefTYPE(startL)) return NILINK;

	InitSearchParam(&pbSearch);
	pbSearch.id = staffn;
	pbSearch.needSelected = pbSearch.inSystem = False;
	pbSearch.needInMeasure = True;

	/* Clef change propagates until the next inMeasure clef. */
	doneL = L_Search(RightLINK(startL), CLEFtype, False, &pbSearch);
	if (!doneL) doneL = doc->tailL;
	
	GetContext(doc, LeftLINK(startL), staffn, &context);			/* Get staff height, lines */
	EFixContForClef(doc, startL, doneL, staffn, oldClef, newClef, context);

	return doneL;
}


/* ------------------------------------------------------------ EFixContForKeySig -- */
/* In the range [startL,doneL), update the key signature in context fields of
following STAFFs and MEASUREs for the given staff. If we find another key sig-
nature on the staff before doneL, we stop at that point. */

void EFixContForKeySig(LINK startL, LINK doneL,
						short staffn,							/* Desired staff no. */
						KSINFO /*oldKSInfo*/, KSINFO newKSInfo	/* Previously-effective and new key sig. info */
						)
{
	LINK	pL, aStaffL, aKeySigL, aMeasureL;

	for (pL = startL; pL!=doneL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
					if (StaffSTAFF(aStaffL)==staffn) {
						/* aStaff = GetPASTAFF(aStaffL); */
						KEYSIG_COPY(&newKSInfo, (PKSINFO)(StaffKSITEM(aStaffL)));
					}
				break;
			case KEYSIGtype:
				aKeySigL = FirstSubLINK(pL);
				for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL))
					if (KeySigSTAFF(aKeySigL)==staffn) {
						if (!KeySigINMEAS(pL)) {
							/* aKeySig = GetPAKEYSIG(aKeySigL); */
							KEYSIG_COPY(&newKSInfo, (PKSINFO)(KeySigKSITEM(aKeySigL)));
							KeySigVIS(aKeySigL) = (KeySigNKSITEMS(aKeySigL)!=0);
						}
						else
							return;
					}
				break;
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL))
					if (MeasureSTAFF(aMeasureL)==staffn) {
						/* aMeasure = GetPAMEASURE(aMeasureL); */
						KEYSIG_COPY(&newKSInfo, (PKSINFO)(MeasKSITEM(aMeasureL)));
					}
				break;
			default:
				;
		}
	}
}


/* ---------------------------------------------------------- FixContextForKeySig -- */
/* Until the next key signature for the staff, update the key sig. in context
fields of following STAFFs and MEASUREs for the given staff. Returns the first
LINK after the range affected or, if nothing is affected, NILINK. */

LINK FixContextForKeySig(Document *doc, LINK startL,
						short staffn,								/* Desired staff no. */
						KSINFO oldKSInfo, KSINFO newKSInfo	/* Previously-effective and new key sig. info */
						)
{
	LINK		doneL;
	SearchParam	pbSearch;

	if (!startL || startL==doc->tailL)
			return NILINK;											/* If nothing following, quit */
	if (KeySigTYPE(startL))
		if (KeySigOnStaff(startL, staffn))
			return NILINK;
	if (KeySigEqual((PKSINFO)(&oldKSInfo), (PKSINFO)(&newKSInfo)))
			return NILINK; 											/* If no key sig. change, quit */

	InitSearchParam(&pbSearch);
	pbSearch.id = staffn;														
	pbSearch.needSelected = pbSearch.inSystem = False;
	pbSearch.needInMeasure = True;

	/* keySig change propagates until the next inMeasure keySig. */
	doneL = L_Search(RightLINK(startL), KEYSIGtype, False, &pbSearch);
	if (!doneL) doneL = doc->tailL;

	EFixContForKeySig(startL, doneL, staffn, oldKSInfo, newKSInfo);

	return doneL;
}


/* ----------------------------------------------------------- EFixContForTimeSig -- */
/* /* In the range [startL,doneL), update the time sig. in context fields of
following STAFFs and MEASUREs for the given staff, and update the physical durations
of whole rests.  If we find another time signature on the staff before doneL, we stop
at that point. Only the time sig.'s timeSigType, numerator, and denominator are
used. N.B. Unlike the similar routines for clefs and key sigs., this routine doesn't
even know if there's actually been a change of time sig.--it forges ahead regardless.
Problems with this: mild inefficiency; replaces <playDur>s of whole rests that may
have been tweaked with vanilla ones (but rests don't play anyway, and never will). */

void EFixContForTimeSig(LINK startL, LINK doneL,
						short staffn,			/* Desired staff no. */
						TSINFO newTSInfo		/* New time signature */
						)
{
	LINK			pL, aNoteL, aStaffL, aMeasureL, aTimeSigL;

	for (pL = startL; pL!=doneL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
					if (StaffSTAFF(aStaffL)==staffn) {
						/* aStaff = GetPASTAFF(aStaffL); */
						StaffTIMESIGTYPE(aStaffL) = newTSInfo.TSType;			/* Copy the time sig. into the context */
						StaffNUMER(aStaffL) = newTSInfo.numerator;
						StaffDENOM(aStaffL) = newTSInfo.denominator;
					}
				break;
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL))
					if (MeasureSTAFF(aMeasureL)==staffn) {
						/* aMeasure = GetPAMEASURE(aMeasureL); */
						MeasTIMESIGTYPE(aMeasureL) = newTSInfo.TSType;			/* Copy the time sig. into the context */
						MeasNUMER(aMeasureL) = newTSInfo.numerator;
						MeasDENOM(aMeasureL) = newTSInfo.denominator;
					}
				break;
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteSTAFF(aNoteL)==staffn) {
						/* aNote = GetPANOTE(aNoteL); */
						if (NoteREST(aNoteL) && NoteType(aNoteL)==WHOLEMR_L_DUR)	/* Whole rest? */
							NotePLAYDUR(aNoteL) = TimeSigDur(newTSInfo.TSType,	/* Yes, duration=measure dur. */
																newTSInfo.numerator,
																newTSInfo.denominator);
					}
				break;
			case TIMESIGtype:
				aTimeSigL = FirstSubLINK(pL);
				for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL))
					if (TimeSigSTAFF(aTimeSigL)==staffn && TimeSigINMEAS(pL))
						return;
			default:
				;
		}
	}
}


/* --------------------------------------------------------- FixContextForTimeSig -- */
/* Until the next time signature for the staff, update the time sig. in con-
text fields of following STAFFs and MEASUREs for the given staff, and update
the physical durations of whole rests.  Only the time sig.'s timeSigType,
numerator, and denominator are used. N.B. Unlike the similar routines for
clefs and key sigs., this routine doesn't even know if there's actually been
a change of time sig.--it forges ahead regardless.  See comments on the (minor)
problems with this in EFixContForTimeSig. */

LINK FixContextForTimeSig(Document *doc, LINK startL,
							short staffn,			/* Desired staff no. */
							TSINFO newTSInfo		/* New time signature */
							)
{
	LINK doneL;

	/*
	 *	If nothing following or if startL itself is a time signature, no change
	 *	propagates past it; therefore we are done. ??YES, BUT ONLY IF IT'S A TIMESIG
	 *	ON OUR STAFF! CF. FixContextForKeySig.
	 */
	if (!startL || TimeSigTYPE(startL)) return NILINK;															/* If nothing following, quit */

	doneL = LSSearch(startL, TIMESIGtype, staffn, False, False);
	if (!doneL) doneL = doc->tailL;

	EFixContForTimeSig(startL, doneL, staffn, newTSInfo);
	
	return doneL;
}


/* ----------------------------------------------------------- EFixContForDynamic -- */
/* In the range [startL,doneL), update (1) the dynamic in context fields of
following STAFFs and MEASUREs for the given staff, and (2) notes' On velocities. 
If we find another non-hairpin dynamic on the staff before doneL, we stop at that
point; this is on the assumption that contexts after that are already correct. 
??If RELDYNCHANGE is defined, it adds a constant to existing velocities,
preserving tweaking; otherwise it simply replaces velocities. How do we really
want to do this? */

void EFixContForDynamic(LINK startL, LINK doneL,
				short staffn,									/* Desired staff no. */
				SignedByte newDynamic							/* New dynamicTypes */
				)
{
	LINK		pL, aNoteL, aStaffL, aDynamicL, aMeasureL;
	short		newVelocity;

#ifdef RELDYNCHANGE
	veloChange = dynam2velo[newDynamic]-dynam2velo[oldDynamic];	/* Rel. change; preserve tweaking */
#else
	if (newDynamic<1 || newDynamic>=FIRSTHAIRPIN_DYNAM) {
		MayErrMsg("EFixContForDynamic: dynamic type %ld is a hairpin or illegal.",
					(long)newDynamic);
		return;
	}
	newVelocity = dynam2velo[newDynamic];						/* Replace; discard tweaking */
	if (newVelocity<1 || newVelocity>MAX_VELOCITY) {
		MayErrMsg("EFixContForDynamic: illegal velocity of %ld for dynamic type %ld",
					(long)newVelocity, (long)newDynamic);
		return;
	}
#endif

	for (pL = startL; pL!=doneL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					/* aNote = GetPANOTE(aNoteL); */
					if (!NoteREST(aNoteL) && NoteSTAFF(aNoteL)==staffn) {
#ifdef RELDYNCHANGE
						NoteONVELOCITY(aNoteL) += veloChange;
						if (NoteONVELOCITY(aNoteL)<1 || NoteONVELOCITY(aNoteL)>MAX_VELOCITY)
							MayErrMsg("EFixContForDynamic: at %ld change=%ld gives new vel.=%ld",
										(long)pL, (long)veloChange, (long)(NoteONVELOCITY(aNoteL)));
#else
						NoteONVELOCITY(aNoteL) = newVelocity;
#endif
					}
				}
				break;
			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
					/* aStaff = GetPASTAFF(aStaffL); */
					if (StaffSTAFF(aStaffL)==staffn)
						StaffDynamType(aStaffL) = newDynamic;
				}
				break;
			case DYNAMtype:
				aDynamicL = FirstSubLINK(pL);
				for ( ; aDynamicL; aDynamicL = NextDYNAMICL(aDynamicL)) {
					/* aDynamic = GetPADYNAMIC(aDynamicL); */
					if (DynamicSTAFF(aDynamicL)==staffn)
						if (DynamType(pL)<FIRSTHAIRPIN_DYNAM)	/* For now, ignore hairpins */
							return;
				}
				break;
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
					/* aMeasure = GetPAMEASURE(aMeasureL); */
					if (MeasureSTAFF(aMeasureL)==staffn)
						MeasDynamType(aMeasureL) = newDynamic;
				}
				break;
			default:
				;
		}
	}
}


/* ------------------------------------------------------------- NonHPDynamSearch -- */
/* Starting at <link>, search for the next non-hairpin dynamic. */

static LINK NonHPDynamSearch(LINK link, short staffn);
static LINK NonHPDynamSearch(LINK link, short staffn)
{
	if (DynamTYPE(link) && DynamType(link)>=FIRSTHAIRPIN_DYNAM)
		return link;

	do {
		link = LSSearch(RightLINK(link), DYNAMtype, staffn, False, False);
	} while (link && DynamType(link)>=FIRSTHAIRPIN_DYNAM);
	
	return link;
}

/* --------------------------------------------------------- FixContextForDynamic -- */
/* Until the next dynamic for the staff, update (1) the dynamic in context fields of
following STAFFs and MEASUREs for the given staff, and (2) notes' On velocities. */

LINK FixContextForDynamic(
				Document *doc, LINK startL,
				short staffn,
				SignedByte oldDynamic, SignedByte newDynamic		/* Previous and new dynamTypes */
				)
{
	LINK	doneL;

	if (!startL || startL==doc->tailL) return NILINK;			/* If nothing following, quit. */
	if (oldDynamic==newDynamic) return NILINK;					/* If no dynamic change, quit. */

	/* For now, hairpin dynamics don't affect note velocities; therefore we don't
		put them in the context fields. */

	if (newDynamic>=FIRSTHAIRPIN_DYNAM) return NILINK;
	
	/* FIXME: This error should be more general: what is so special about the headL
		as opposed to, say, the first page obj of the score? */

	if (startL==doc->headL) {
		MayErrMsg("FixContextForDynamic: can't handle startL=headL"); return NILINK;
	}
	
	doneL = NonHPDynamSearch(startL,staffn);
	if (!doneL) doneL = doc->tailL;

	EFixContForDynamic(startL, doneL, staffn, newDynamic);
	
	return doneL;
}


/* -------------------------------------------------------------- FixMeasureContext -- */
/*	Fix clef/key sig./meter/dynamic context fields of a Measure subobject. */

void FixMeasureContext(LINK aMeasureL, PCONTEXT pContext)
{
	short k;

	if (pContext->nKSItems > MAX_KSITEMS) {
		MayErrMsg("FixMeasureContext: called w/ %ld nKSItems", (long)pContext->nKSItems);
		pContext->nKSItems = MAX_KSITEMS;
	}
	
	MeasCLEFTYPE(aMeasureL) = pContext->clefType;
	MeasNKSITEMS(aMeasureL) = pContext->nKSItems;
	for (k = 0; k<pContext->nKSItems; k++)
		(MeasKSITEM(aMeasureL))[k] = pContext->KSItem[k];
	MeasTIMESIGTYPE(aMeasureL) = pContext->timeSigType;
	MeasNUMER(aMeasureL) = pContext->numerator;
	MeasDENOM(aMeasureL) = pContext->denominator;
	MeasDynamType(aMeasureL) = pContext->dynamicType;
}


/* ---------------------------------------------------------------- FixStaffContext -- */
/*	Fix clef/key sig./meter context fields of a Staff subobject. */

void FixStaffContext(LINK aStaffL, PCONTEXT pContext)
{
	short k;
	
	if (pContext->nKSItems > MAX_KSITEMS) {
		MayErrMsg("FixStaffContext: called w/ %ld nKSItems", (long)pContext->nKSItems);
		pContext->nKSItems = MAX_KSITEMS;
	}

	StaffCLEFTYPE(aStaffL) = pContext->clefType;
	StaffNKSITEMS(aStaffL) = pContext->nKSItems;
	for (k = 0; k<pContext->nKSItems; k++)
		(StaffKSITEM(aStaffL))[k] = pContext->KSItem[k];
	StaffTIMESIGTYPE(aStaffL) = pContext->timeSigType;
	StaffNUMER(aStaffL) = pContext->numerator;
	StaffDENOM(aStaffL) = pContext->denominator;
	StaffDynamType(aStaffL) = pContext->dynamicType;
}


/* Utilities pulled out for the sake of PasteFixContext and FixDelContext. */

LINK UpdateKSContext(Document *doc, LINK pL, short stf, CONTEXT oldContext,
							CONTEXT newContext)
{
	KSINFO oldKS, newKS; LINK doneL;

	KeySigCopy((PKSINFO)oldContext.KSItem, &oldKS);
	KeySigCopy((PKSINFO)newContext.KSItem, &newKS);
	doneL = FixContextForKeySig(doc, pL, stf, oldKS, newKS);
	return doneL;
}

void UpdateTSContext(Document *doc, LINK pL, short stf, CONTEXT newContext)
{
	TSINFO	timeSigInfo;

	timeSigInfo.TSType = newContext.timeSigType;
	timeSigInfo.numerator = newContext.numerator;
	timeSigInfo.denominator = newContext.denominator;
	FixContextForTimeSig(doc, pL, stf, timeSigInfo);
}


/* ----------------------------------------------- Utilities for FixAccsForKeySig -- */

/* Merge accidental table KSTab into oldKSTab. */

void CombineTables(SignedByte oldKSTab[], SignedByte KSTab[])
{
	short j;
	
	for (j = 0; j<MAX_STAFFPOS; j++)					/* Combine tables */
		if (oldKSTab[j]==KSTab[j])						/* Entry the same? */
			oldKSTab[j] = 0;							/* Yes, do nothing */
}

/* Initialize a pitch modifier table from the given KSInfo. */

void InitPitchModTable(SignedByte KSTab[], KSINFO *pKSInfo)
{
	short j;

	KeySig2AccTable(KSTab, pKSInfo);					/* Init. table from key sig. */
	for (j = 0; j<MAX_STAFFPOS; j++)
		if (KSTab[j]==0) KSTab[j] = AC_NATURAL;			/* Replace no acc. with natural */
}

/* Copy accidental table KSTab into accTab. */

void CopyTables(SignedByte KSTab[], SignedByte accTab[])
{
	short j;

	for (j = 0; j<MAX_STAFFPOS; j++)
		accTab[j] = KSTab[j];
}


/* ------------------------------------------------------------ EndMeasRangeSearch -- */
/* Return endL, the LINK ending the measure startL is in, clipped to doneL: if
doneL is before endL, return doneL, else endL. */

LINK EndMeasRangeSearch(Document *doc, LINK startL, LINK doneL)
{
	LINK endL;

	endL = EndMeasSearch(doc, startL);
	
	return (IsAfter(doneL, endL) ? doneL : endL);
}

/* ------------------------------------------------------------- EFixAccsForKeySig -- */
/* Go through staff in range [startL,doneL) and change accidentals where
appropriate to keep notes' pitches the same, and return the next key signature
or, if there is none, tailL. */

LINK EFixAccsForKeySig(Document *doc, LINK startL, LINK doneL,
						short staffn,						/* Desired staff no. */
						KSINFO oldKSInfo, KSINFO newKSInfo	/* Previously-effective and new key sig. info */
						)
{
	SignedByte oldKSTab[MAX_STAFFPOS], KSTab[MAX_STAFFPOS];	/* Orig., new accidental tables */
	LINK barFirstL,barLastL;

	/* Compute an accidental table that converts the pitch modifer table after the
	 *	insert into the table before it, so we can correct notes to be the same
	 *	in the new pitch modifier environment as they were in the old environment.
 	 */
 	InitPitchModTable(oldKSTab, &oldKSInfo);
 	InitPitchModTable(KSTab, &newKSInfo);
 	CombineTables(oldKSTab, KSTab);

	/*
	 * Now use the table <oldKSTab> to correct accidentals of notes on the staff, one
	 * measure at a time. 
	 */
	barFirstL = startL;
	barLastL = EndMeasSearch(doc, RightLINK(startL));
	if (IsAfter(doneL, barLastL)) barLastL = doneL;

	/* Until the next key sig., fix the accidentals, one measure at a time. */
	while (IsAfter(barFirstL, barLastL)) {					
		/* Initialize accTable for each bar, since (ugh) FixAllAccidentals destroys it! */
		CopyTables(oldKSTab,accTable);
	
		FixAllAccidentals(barFirstL, barLastL, staffn, False);
		if (barLastL==doc->tailL) break;
		
		barFirstL = barLastL;
		barLastL = EndMeasRangeSearch(doc, RightLINK(barLastL), doneL);
	}
	return barLastL;
}


/* ------------------------------------------------------------- FixAccsForKeySig -- */
/* See whether new key signature is different from the previous one in effect here.
If so, go through staff till its next key signature and change accidentals where
appropriate to keep notes' pitches the same, and return the next key signature
or, if there is none, tailL. If the new key signature is the same as the previously
effective one, simply return NILINK. */

LINK FixAccsForKeySig(Document *doc,
						LINK keySigL,					/* New key signature */
						short staffn,					/* Desired staff no. */
						KSINFO oldKSInfo, KSINFO newKSInfo)	/* Previously-effective and new key sig. info */
{
	LINK nextKSL,lastFixL;

	if (KeySigEqual(&oldKSInfo, &newKSInfo)) return NILINK;

	nextKSL = KSSearch(doc,RightLINK(keySigL),staffn,True,False);

	lastFixL = EFixAccsForKeySig(doc, keySigL, nextKSL, staffn, oldKSInfo, newKSInfo);
	return lastFixL;
}
