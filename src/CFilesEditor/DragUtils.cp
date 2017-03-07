/***************************************************************************
*	FILE:	DragUtils.c
*	PROJ:	Nightingale
*	DESC:	Bitmap dragging utilities
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

static void SDInvalObjRects(Document *doc, LINK pL, DDIST xdDiff, DDIST ydDiff, short offset);
static void SDInvalMeasures(Document *doc, LINK pL);
static void SDInvalObject(Document *doc, LINK pL, DDIST xdDiff, DDIST ydDiff, short offset);

/* ====================================================== Horizontal note dragging == */
/* Auxiliary functions for dragging notes horizontally.

/*  Get left and right dragging limits for horizontal drag. Limits are alway positive,
expressing DDIST offset from the xd of pL, taking measures into account. If the
object dragged is at the end of the score, rightL will be the tail, and rightLim is
flagged as not valid by giving it a value of -1. */

void GetHDragLims(Document */*doc*/, LINK pL, LINK subObjL, short /*staff*/, CONTEXT /*context*/,
						DDIST *leftLim, DDIST *rightLim)
{
	DDIST leftLimit, rightLimit, needLeft, needRight, measWidth, objXD;
	LINK leftL, rightL;

	objXD = LinkXD(pL) + GetSubXD(pL,subObjL);
	leftL = FirstValidxd(LeftLINK(pL), TRUE);
	rightL = FirstValidxd(RightLINK(pL), FALSE);

	leftLimit = MeasureTYPE(leftL) ? objXD : objXD-LinkXD(leftL);

	if (TailTYPE(rightL))
		rightLimit = -1;
	else if (MeasureTYPE(rightL)) {
		measWidth = LinkLMEAS(rightL) ? LinkXD(rightL)-LinkXD(LinkLMEAS(rightL)) :
													LinkXD(rightL);
		rightLimit = measWidth-objXD;
	}
	else rightLimit = LinkXD(rightL)-objXD;

	needLeft = pt2d(1);
	leftLimit -= needLeft;
	
	if (!TailTYPE(rightL)) {
		needRight = pt2d(1);
		rightLimit -= needRight;
	}
	*leftLim = leftLimit;
	*rightLim = rightLimit;
}


/* ======================================================== Vertical note dragging == */
/* Auxiliary functions for dragging notes vertically.

/*--------------------------------------------------------------- SDInsertLedgers -- */
/*	Draw pseudo-ledger lines for insertion feedback for notes to be inserted at
the halfline number <halfLine>. */

void SDInsertLedgers(LINK pL, LINK aNoteL, short halfLine, PCONTEXT pContext)
{
	DDIST	staffTop, staffHeight, xd;
	short	l, staffLines, ledgerLeft, ledgerLen, staff;
	LINK	staffL, aStaffL;  PASTAFF aStaff;

	PenNormal();
	xd = LinkXD(pL);							/* DDIST horiz pos of note rel to portRect of SymDrag Ports */

	staff = NoteSTAFF(aNoteL);
	staffL = LSSearch(pL, STAFFtype, ANYONE, GO_LEFT, FALSE);

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==staff) break;

	aStaff = GetPASTAFF(aStaffL);
	staffTop = aStaff->staffTop;

	staffHeight = pContext->staffHeight;
	staffLines = pContext->staffLines;
	ledgerLen = d2p(InsLedgerLen(LNSPACE(pContext)));
	ledgerLeft = d2p(xd) - ledgerLen/2;
	if (halfLine<0)
		for (l=-2; l>=halfLine; l-=2) {
			MoveTo(ledgerLeft,
				d2p(staffTop+halfLn2d(l, staffHeight, staffLines)));
			Line(ledgerLen, 0);
		}
	else
		for (l=2*staffLines; l<=halfLine; l+=2) {
			MoveTo(ledgerLeft,
				d2p(staffTop+halfLn2d(l, staffHeight, staffLines)));
			Line(ledgerLen, 0);
		}
}


/* -------------------------------------------------------------- SDSetAccidental -- */
/* Get new accidental for note dragged vertically from within SymDragLoop,
a la InsTrackPitch. ??I'm not sure this is ever called! --DAB, 2/97 */

short SDSetAccidental(Document *doc, GrafPtr accPort, Rect accBox, Rect saveBox, short accy)
{
	Point accPt, oldPt; 
	short accident, accidentOld=0;
	WindowPtr w=doc->theWindow;

	GetPaperMouse(&oldPt,&doc->currentPaper);

	while (ShiftKeyDown() && StillDown())  {
		GetPaperMouse(&accPt,&doc->currentPaper);
		accident = CalcAccidental(accPt.v, oldPt.v);
		if (accident!=accidentOld) {						/* Change of accidental? */
			if (accident==0) {
				const BitMap *accPortBits = GetPortBitMapForCopyBits(accPort);
				const BitMap *wPortBits = GetPortBitMapForCopyBits(GetWindowPort(w));
				CopyBits(accPortBits, wPortBits, &saveBox, &accBox, srcCopy, NULL);

//				CopyBits(&(accPort->portBits), &w->portBits,
//						&saveBox, &accBox, srcCopy, NULL);
			}
			else {
				TextFont(doc->musicFontNum);				/* Set to music font */
				TextSize(ACCSIZE);
				EraseRect(&accBox);
				FrameRect(&accBox);
				MoveTo(accBox.left+4, accy);
				DrawChar(SonataAcc[accident]);			/* Draw new accidental */
			}
			accidentOld = accident;
		}
	}
	return accident;
}


/* --------------------------------------------------------------- SDMIDIFeedback -- */
/* Give MIDI feedback, presumably called while dragging a note vertically. */

void SDMIDIFeedback(Document *doc, short *noteNum, short useChan, short acc,
							short transp, short midCHalfLn, short halfLn,
							short octTransp,
							short ioRefNum				/* Ignored unless using OMS or FreeMIDI */
							)
{
	short prevAccident;

	if (!doc->feedback) return;

	MIDIFBNoteOff(doc, *noteNum, useChan, ioRefNum);
	if (acc==0) {
		if (ACC_IN_CONTEXT)
			prevAccident = accTable[halfLn-midCHalfLn+ACCTABLE_OFF];			
		*noteNum = Pitch2MIDI(midCHalfLn-halfLn+octTransp, prevAccident) + transp;
	}
	else
		*noteNum = Pitch2MIDI(midCHalfLn-halfLn+octTransp, acc) + transp;

	MIDIFBNoteOn(doc, *noteNum, useChan, ioRefNum);
}


/* ================================================================ Setting Fields == */
/* Auxiliary functions for setting fields after bitmap dragging.

/* ---------------------------------------------------------------------- Getystem -- */
/* If <sync> has any notes in <voice>, return the ystem of the main note. */

DDIST Getystem(short voice, LINK sync)
{
	LINK aNoteL;
	PANOTE aNote;

	aNoteL = FirstSubLINK(sync);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice) {
			aNote = GetPANOTE(aNoteL);
			if (MainNote(aNoteL)) 
				return aNote->ystem;
		}
	
	return 0;												/* No note in <voice> */
}


/* -------------------------------------------------------------------- GetGRystem -- */
/* If <GRsync> has any notes in <voice>, return the ystem of the main note. */

DDIST GetGRystem(short voice, LINK GRsync)
{
	LINK aGRNoteL;
	PAGRNOTE aGRNote;

	aGRNoteL = FirstSubLINK(GRsync);
	for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL))
		if (GRNoteVOICE(aGRNoteL)==voice) {
			aGRNote = GetPAGRNOTE(aGRNoteL);
			if (GRMainNote(aGRNoteL)) 
				return aGRNote->ystem;
		}

	return 0;												/* No note in <voice> */
}


/* -------------------------------------------------------------- SDFixStemLengths -- */
/* Fix up all stems in a slanted beamset in which a note has been dragged
horizontally. Cross staff beams must have their note stems fixed up differently. */

static void FixNoteStems(LINK beamL,LINK qL,LINK firstSyncL,
					DDIST hDiff,DDIST vDiff,DDIST lastystem,short *h);
static void FixXStfNoteStems(Document *doc,LINK beamL);
static void HDragFixNoteStems(Document *doc,LINK beamL,LINK firstSyncL,
					DDIST hDiff,DDIST vDiff,DDIST lastystem,short xStf);

static void FixNoteStems(LINK beamL, LINK qL, LINK firstSyncL, DDIST hDiff,
									DDIST vDiff, DDIST lastystem, short *h)
{
	LINK qSubL;
	DDIST noteDiff, newStemDiff;
	float fhDiff, fvDiff, fnoteDiff;

	for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextNOTEL(qSubL))
		if (NoteVOICE(qSubL)==BeamVOICE(beamL))
			if (MainNote(qSubL)) {
				if (NoteBEAMED(qSubL)) {

			 		noteDiff = SysRelxd(qL)-SysRelxd(firstSyncL)+NoteXD(qSubL);

					if (noteDiff) {
						fnoteDiff = hDiff-noteDiff;
						fhDiff = hDiff;
						fvDiff = vDiff;
						newStemDiff = (DDIST)fvDiff*(fnoteDiff/fhDiff);
				 		NoteYSTEM(qSubL) = newStemDiff+lastystem;
				 	}
				}
				else if (!NoteREST(qSubL))
					MayErrMsg("SDFixStemLengths: Unbeamed note in Sync %ld where beamed note expected", (long)qL);
				if (!NoteREST(qSubL) || NoteBEAMED(qSubL))
					(*h)++;
			}
}

static void FixXStfNoteStems(Document *doc, LINK beamL)
{
	short v,nInBeam,staff,stfLines; Boolean upOrDown;
	DDIST stfHeight,ystem,firstystem,lastystem;
	LINK startL,endL,bpSync[MAXINBEAM],noteInSync[MAXINBEAM],staffL,aStaffL,baseL;
	STFRANGE theRange; PASTAFF aStaff;

	startL = FirstInBeam(beamL);
	endL = LastInBeam(beamL);
	v = BeamVOICE(beamL);
	staff = BeamSTAFF(beamL);
	nInBeam = LinkNENTRIES(beamL);

	/* Fill in the arrays of Sync and note LINKs for the benefit of beam subobjs. */
	if (!GetBeamSyncs(doc, startL, RightLINK(endL), v, nInBeam, bpSync, noteInSync, FALSE))
			return;

	/* Handle cross-staff conditions. */
	GetCrossStaff(nInBeam, noteInSync, &theRange);

	/* Get the y level of the beam set and set the note stems of the beamset's notes. */
	staffL = LSSearch(startL, STAFFtype, staff, GO_LEFT, FALSE);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		if (aStaff->staffn == staff) {
			stfHeight =	aStaff->staffHeight;
			stfLines = aStaff->staffLines;
		}
	} 
	ystem = CalcBeamYLevel(doc, nInBeam, bpSync, noteInSync, &baseL, stfHeight, stfLines,
									TRUE, doc->voiceTab[v].voiceRole, &upOrDown);
									
	/* ??The following #if added by DAB to fix bug: can't call GetBeamEndYStems bcs
		baseL is undefined. Just make beam horizontal for now, as when cross-staff
		beam is created. But this function is now working very hard to accomplish little.
		What should it do? */
	firstystem = lastystem = ystem;

	FillSlantBeam(doc, beamL, v, nInBeam, bpSync, noteInSync, firstystem,
									lastystem, theRange, TRUE);
}


static void HDragFixNoteStems(Document *doc, LINK beamL, LINK firstSyncL, DDIST hDiff,
										DDIST vDiff, DDIST lastystem, short xStf)
{
	short h; LINK qL;

	if (xStf) {
		FixXStfNoteStems(doc,beamL);
		return;
	}
	
	qL=RightLINK(beamL);
	for (h=0; qL && h<LinkNENTRIES(beamL); qL=RightLINK(qL)) {
		if (SyncTYPE(qL)) {
			if (qL==firstSyncL)							/* have already done firstSync */
				h++;
			else
				FixNoteStems(beamL, qL, firstSyncL, hDiff, vDiff, lastystem, &h);
		}
	}
}


void SDFixStemLengths(Document *doc, LINK beamL)
{
	DDIST		hDiff, vDiff, firstystem, lastystem;
	LINK		firstSyncL, lastSyncL;
	
	firstSyncL = FirstInBeam(beamL);
	lastSyncL = LastInBeam(beamL);
	hDiff = SysRelxd(lastSyncL)-SysRelxd(firstSyncL);
	firstystem = Getystem(BeamVOICE(beamL), firstSyncL);
	lastystem = Getystem(BeamVOICE(beamL), lastSyncL);
	vDiff = firstystem-lastystem;

	HDragFixNoteStems(doc, beamL, firstSyncL, hDiff, vDiff, lastystem, BeamCrossSTAFF(beamL));
}

/* ----------------------------------------------------------- SDFixGRStemLengths -- */
/* Fix up all stems in a slanted beamset in which a grace note has been
dragged horizontally. */

void SDFixGRStemLengths(LINK beamL)
{
	DDIST		hDiff, noteDiff, vDiff, firstystem, lastystem, newStemDiff;
	LINK		qL, firstSyncL, lastSyncL, qSubL;
	PAGRNOTE	qSub;
	short		h;
	float		fhDiff, fnoteDiff, fvDiff;
	
	firstSyncL = FirstInBeam(beamL);
	lastSyncL = LastInBeam(beamL);
	hDiff = SysRelxd(lastSyncL)-SysRelxd(firstSyncL);
	firstystem = GetGRystem(BeamVOICE(beamL), firstSyncL);
	lastystem = GetGRystem(BeamVOICE(beamL), lastSyncL);
	vDiff = firstystem-lastystem;

	h = 0;
	for (qL=RightLINK(beamL); qL && h<LinkNENTRIES(beamL); qL=RightLINK(qL)) {
		if (GRSyncTYPE(qL)) {
			if (qL==firstSyncL) {
				h++;								/* h indexes the # of syncs in the beamset */
				continue;							/* Have already done firstSync */
			}
			else {
				for (qSubL=FirstSubLINK(qL); qSubL; qSubL=NextGRNOTEL(qSubL))
					if (GRNoteVOICE(qSubL)==BeamVOICE(beamL)) {
						qSub = GetPAGRNOTE(qSubL);
						if (GRMainNote(qSubL)) {
							if (qSub->beamed) {
						 		noteDiff = SysRelxd(qL)-SysRelxd(firstSyncL);
								if (noteDiff) {
									fnoteDiff = hDiff-noteDiff;
									fhDiff = hDiff;
									fvDiff = vDiff;
									newStemDiff = (DDIST)fvDiff*(fnoteDiff/fhDiff);
							 		qSub->ystem = newStemDiff+lastystem;
							 	}
							}
							else
								MayErrMsg("SDFixGRStemLengths: Unbeamed note in Sync %ld where beamed note expected", (long)qL);
							h++;
						}
					}
			}
		}
	}
}


/* -------------------------------------------------------------- SDGetClosestClef -- */
/*  Given halfLnDiff vertical movement of a clef from its current position,
find the closest legal clef position to which this movement corresponds and
pin the movement to this position. For now, assumes clef is a C clef, since
we don't allow vertically dragging either the F or the G clef. */

DDIST SDGetClosestClef(Document */*doc*/, short halfLnDiff, LINK /*pL*/, LINK subObjL,
								CONTEXT /*context*/)
{
	PACLEF aClef;
	short clefHalfLn, newHalfLn;

	clefHalfLn = ClefMiddleCHalfLn(ClefType(subObjL));

	newHalfLn = clefHalfLn+halfLnDiff;
	aClef = GetPACLEF(subObjL);
	
	if (config.earlyMusic) {
		if (newHalfLn>=7)
			aClef->subType = SOPRANO_CLEF;
		else if (newHalfLn>=5)
			aClef->subType = MZSOPRANO_CLEF;
		else if (newHalfLn>=3)
			aClef->subType = ALTO_CLEF;
		else if (newHalfLn>=1)
			aClef->subType = TENOR_CLEF;
		else
			aClef->subType = BARITONE_CLEF;
	}
	else {
		if (newHalfLn >=3)
			aClef->subType = ALTO_CLEF;
		else
			aClef->subType = TENOR_CLEF;
	}
	
	return (DDIST)0;
}

/* =============================================================== BitMap Dragging == */
/* Utilities related to symbol dragging with offscreen bitmaps. */

static void ClipToPort(Document *, Rect *);
static GrafPtr GetRectGrafPort(Rect);
static void CopyBitMaps(Document *, Rect, Rect);
static GrafPtr SDGetGrafPorts(Rect);

/* ------------------------------------------------------------------- ClipToPort -- */
/* Clip the rect r to the portRect of doc's window. */

static void ClipToPort(Document *doc, Rect *r)
{
	WindowPtr w=doc->theWindow;
	Rect oldRect, portRect;
	
	/* Offset mRect to window coords, intersect it with w->portRect, then back to paper */
	OffsetRect(r, doc->currentPaper.left, doc->currentPaper.top);

	oldRect = *r;
	GetWindowPortBounds(w,&portRect);
	SectRect(r, &portRect, r);
	dragOffset = r->left - oldRect.left;

	OffsetRect(r, -doc->currentPaper.left, -doc->currentPaper.top);
}


/* ---------------------------------------------------------------- GetRectGrafPort -- */
/* Get a new grafPort the size of r. */

static GrafPtr GetRectGrafPort(Rect r)
{
	GrafPtr ourPort; short rWidth, rHeight;

	/* Allocate GrafPort the size of r */
	rWidth = r.right-r.left;
	rHeight = r.bottom-r.top;
#ifdef USE_GWORLDS
	ourPort = (GrafPtr)MakeGWorld(rWidth, rHeight, TRUE);
#else
	ourPort = NewGrafPort(rWidth, rHeight);
#endif

	return ourPort;
}


/* ------------------------------------------------------------------ CopyBitMaps -- */
/* Initialize underBits and offScrBits with doc's window's bits. */

static void CopyBitMaps(Document *doc, Rect srcRect, Rect dstRect)
{
	WindowPtr w = doc->theWindow;

	if (!underBits || !offScrBits) return;

	const BitMap *wPortBits = GetPortBitMapForCopyBits(GetWindowPort(w));
	const BitMap *underPortBits = GetPortBitMapForCopyBits(underBits);
	const BitMap *offScrPortBits = GetPortBitMapForCopyBits(offScrBits);	
	
	RgnHandle underBitsClipRgn = NewRgn();
	RgnHandle offScrBitsClipRgn = NewRgn();

	GetPortClipRegion(underBits,underBitsClipRgn);
	GetPortClipRegion(offScrBits,offScrBitsClipRgn);

	CopyBits(wPortBits, underPortBits, &srcRect, &dstRect, srcCopy, underBitsClipRgn);
	CopyBits(wPortBits, offScrPortBits, &srcRect, &dstRect, srcCopy, offScrBitsClipRgn);
		
	DisposeRgn(underBitsClipRgn);
	DisposeRgn(offScrBitsClipRgn);
	
//	CopyBits(&w->portBits, &underBits->portBits,
//			 &srcRect, &dstRect, srcCopy, underBits->clipRgn);
//	CopyBits(&w->portBits, &offScrBits->portBits,
//			 &srcRect, &dstRect, srcCopy, offScrBits->clipRgn);
}


/* --------------------------------------------------------------- SDGetGrafPorts -- */
/* Get the bitmaps for symbol dragging. */

static GrafPtr SDGetGrafPorts(Rect r)
{
	underBits = GetRectGrafPort(r);
	offScrBits = GetRectGrafPort(r);
	picBits = GetRectGrafPort(r);
	
	if (!underBits || !offScrBits || !picBits) return NULL;

	return offScrBits;
}


/* ------------------------------------------------------------------- GetSysRect -- */

Rect GetSysRect(Document *doc, LINK pL)
{
	LINK sysL;  PSYSTEM pSystem;
	DRect sysR;  Rect r;
	
	sysL = LSSearch(pL, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	pSystem = GetPSYSTEM(sysL);
	sysR = pSystem->systemRect;
	D2Rect(&sysR, &r);
	
	ClipToPort(doc, &r);
	return r;
}


/* ------------------------------------------------------------------ GetMeasRect -- */
/* Return the measureRect of measL, clipped to doc's portRect. */

Rect GetMeasRect(Document *doc, LINK measL)
{
	PMEASURE pMeasure;  Rect mRect;
	
	pMeasure = GetPMEASURE(measL);
	mRect = pMeasure->measureBBox;
	
	ClipToPort(doc, &mRect);
	return mRect;
}


/* ------------------------------------------------------------------- Get2MeasRect -- */
/* Return the union of the measureRects of meas1, meas2, clipped to doc's portRect. */

Rect Get2MeasRect(Document *doc, LINK meas1, LINK meas2)
{
	PMEASURE	pMeasure; Rect mRect, mRect1, mRect2;
	
	pMeasure = GetPMEASURE(meas1);
	mRect1 = pMeasure->measureBBox;
	
	pMeasure = GetPMEASURE(meas2);
	mRect2 = pMeasure->measureBBox;

	UnionRect(&mRect1, &mRect2, &mRect);

	ClipToPort(doc, &mRect);
	return mRect;
}


/* ---------------------------------------------------------------- GetSDMeasRect -- */

Rect SDGetMeasRect(Document *doc, LINK pL, LINK measL)
{
	LINK prevMeasL, firstMeasL, lastMeasL;
	Rect mRect;

	if (LinkBefFirstMeas(pL))
		return GetSysRect(doc, pL);

	switch (ObjLType(pL)) {
		case MEASUREtype:
			prevMeasL = LSSearch(LeftLINK(measL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
			mRect = Get2MeasRect(doc, prevMeasL, measL);
			break;
		case DYNAMtype:
			if (IsHairpin(pL)) {
				firstMeasL = LSSearch(DynamFIRSTSYNC(pL), MEASUREtype, 1, GO_LEFT, FALSE);
				lastMeasL = LSSearch(DynamLASTSYNC(pL), MEASUREtype, 1, GO_LEFT, FALSE);
				mRect = Get2MeasRect(doc, firstMeasL, lastMeasL);
			}
			else
				mRect = GetMeasRect(doc, measL);
			break;
		case BEAMSETtype:
			firstMeasL = LSSearch(FirstInBeam(pL), MEASUREtype, 1, GO_LEFT, FALSE);
			lastMeasL = LSSearch(LastInBeam(pL), MEASUREtype, 1, GO_LEFT, FALSE);
			if (firstMeasL!=lastMeasL)
				mRect = Get2MeasRect(doc, firstMeasL, lastMeasL);
			else
				mRect = GetMeasRect(doc, measL);
			break;
		case OTTAVAtype:
			firstMeasL = LSSearch(FirstInOttava(pL), MEASUREtype, 1, GO_LEFT, FALSE);
			lastMeasL = LSSearch(LastInOttava(pL), MEASUREtype, 1, GO_LEFT, FALSE);
			if (firstMeasL!=lastMeasL)
				mRect = Get2MeasRect(doc, firstMeasL, lastMeasL);
			else
				mRect = GetMeasRect(doc, measL);
			break;
		case TUPLETtype:
			firstMeasL = LSSearch(FirstInTuplet(pL), MEASUREtype, 1, GO_LEFT, FALSE);
			lastMeasL = LSSearch(LastInTuplet(pL), MEASUREtype, 1, GO_LEFT, FALSE);
			if (firstMeasL!=lastMeasL)
				mRect = Get2MeasRect(doc, firstMeasL, lastMeasL);
			else
				mRect = GetMeasRect(doc, measL);
			break;
		case SLURtype:
			firstMeasL = LSSearch(SlurFIRSTSYNC(pL), MEASUREtype, 1, GO_LEFT, FALSE);
			lastMeasL = LSSearch(SlurLASTSYNC(pL), MEASUREtype, 1, GO_LEFT, FALSE);
			if (firstMeasL!=lastMeasL)
				mRect = Get2MeasRect(doc, firstMeasL, lastMeasL);
			else
				mRect = GetMeasRect(doc, measL);
			break;
		case TEMPOtype:
		case GRAPHICtype:
			firstMeasL = drag1stMeas;
			lastMeasL = dragLastMeas;
			if (firstMeasL && lastMeasL && (firstMeasL != lastMeasL))
				mRect = Get2MeasRect(doc, firstMeasL, lastMeasL);
			else
				mRect = GetMeasRect(doc, measL);

			break;
		default:
			mRect = GetMeasRect(doc, measL);
			break;
	}
	
	return mRect;
}


/* --------------------------------------------------------------- NewMeasGrafPort -- */

GrafPtr NewMeasGrafPort(Document *doc, LINK measL)
{
	Rect	mBBox;
	GrafPtr	oldPort, ourPort;
	
	GetPort(&oldPort);

	mBBox = GetMeasRect(doc, measL);
	ourPort = GetRectGrafPort(mBBox);		/* Allocate GrafPort the size of measureRect */
	
	/* Clean up ports, return */

	SetPort(oldPort);
	return ourPort;
}


/* -------------------------------------------------------------- New2MeasGrafPort -- */

GrafPtr New2MeasGrafPort(Document *doc, LINK measL)
{
	LINK	prevMeasL;
	Rect	mBBox;
	GrafPtr	oldPort, ourPort;
	
	GetPort(&oldPort);

	prevMeasL = LSSearch(LeftLINK(measL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
	mBBox = Get2MeasRect(doc, prevMeasL, measL);

	ourPort = GetRectGrafPort(mBBox);		/* Allocate GrafPort the combined size of two Measures */
	
	/* Clean up ports, return */

	SetPort(oldPort);
	return ourPort;
}

/* -------------------------------------------------------------- NewNMeasGrafPort -- */

GrafPtr NewNMeasGrafPort(Document *doc, LINK measL, LINK lastMeasL)
{
	Rect	mBBox;
	GrafPtr	oldPort, ourPort;
	
	GetPort(&oldPort);

	mBBox = Get2MeasRect(doc, measL, lastMeasL);
	ourPort = GetRectGrafPort(mBBox);		/* Allocate GrafPort the size of mBBox */

	/* Clean up ports, return */

	SetPort(oldPort);
	return ourPort;
}


/* -------------------------------------------------------------- NewBeamGrafPort -- */

GrafPtr NewBeamGrafPort(Document *doc, LINK beamL, Rect *beamRect)
{
	LINK	measL, lastMeasL,lastSyncL;
	Rect	mBBox;
	GrafPtr	oldPort, ourPort;
	
	GetPort(&oldPort);

	measL = LSSearch(beamL, MEASUREtype, BeamSTAFF(beamL), GO_LEFT, FALSE);
	lastSyncL = LastInBeam(beamL);
	lastMeasL = LSSearch(lastSyncL, MEASUREtype, BeamSTAFF(beamL), GO_LEFT, FALSE);

	mBBox = Get2MeasRect(doc, measL, lastMeasL);

	ourPort = GetRectGrafPort(mBBox);		/* Allocate GrafPort the size of mBBox */
	*beamRect = mBBox;

	/* Clean up ports, return */

	SetPort(oldPort);
	return ourPort;
}


/* --------------------------------------------------------------- SetupMeasPorts -- */

GrafPtr SetupMeasPorts(Document *doc, LINK measL)
{
	Rect 	mRect, dstRect;
	
	underBits = NewMeasGrafPort(doc, measL);
	offScrBits = NewMeasGrafPort(doc, measL);
	picBits = NewMeasGrafPort(doc, measL);

	if (!underBits || !offScrBits || !picBits) return NULL;

	mRect = GetMeasRect(doc, measL);

	dstRect = mRect;
	OffsetRect(&dstRect, -mRect.left, -mRect.top);
	OffsetRect(&mRect, doc->currentPaper.left, doc->currentPaper.top);
	
	CopyBitMaps(doc, mRect, dstRect);

	return offScrBits;
}

/* -------------------------------------------------------------- Setup2MeasPorts -- */

GrafPtr Setup2MeasPorts(Document *doc, LINK measL)
{
	Rect 	twoMeasRect, dstRect;
	LINK	prevMeasL;
	
	underBits = New2MeasGrafPort(doc, measL);
	offScrBits = New2MeasGrafPort(doc, measL);
	picBits = New2MeasGrafPort(doc, measL);

	if (!underBits || !offScrBits || !picBits) return NULL;

	prevMeasL = LSSearch(LeftLINK(measL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
	twoMeasRect = Get2MeasRect(doc, prevMeasL, measL);

	dstRect = twoMeasRect;
	OffsetRect(&dstRect,-twoMeasRect.left,-twoMeasRect.top);
	OffsetRect(&twoMeasRect, doc->currentPaper.left, doc->currentPaper.top);
	
	CopyBitMaps(doc, twoMeasRect, dstRect);
	
	return offScrBits;
}

/* -------------------------------------------------------------- SetupNMeasPorts -- */

GrafPtr SetupNMeasPorts(Document *doc, LINK startL, LINK endL)
{
	Rect 	mBBox, dstRect;
	LINK	firstMeasL, lastMeasL;
	
	firstMeasL = LSSearch(startL, MEASUREtype, 1, GO_LEFT, FALSE);
	lastMeasL = LSSearch(endL, MEASUREtype, 1, GO_LEFT, FALSE);

	mBBox = Get2MeasRect(doc, firstMeasL, lastMeasL);

	if (!SDGetGrafPorts(mBBox)) return NULL;

	dstRect = mBBox;
	OffsetRect(&dstRect,-mBBox.left,-mBBox.top);
	OffsetRect(&mBBox,doc->currentPaper.left, doc->currentPaper.top);

	CopyBitMaps(doc, mBBox, dstRect);

	return offScrBits;
}

/* ----------------------------------------------------------------- SetupSysPorts -- */

GrafPtr SetupSysPorts(Document *doc, LINK pL)
{
	Rect dstRect, r;
	
	r = GetSysRect(doc, pL);
	if (!SDGetGrafPorts(r)) return NULL;

	dstRect = r;
	OffsetRect(&dstRect, -r.left, -r.top);
	OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
	
	CopyBitMaps(doc, r, dstRect);
	
	return offScrBits;
}

/* --------------------------------------------------------------- DragBeforeFirst -- */

void DragBeforeFirst(Document *doc, LINK pL, Point pt)
{
	Boolean		found;
	short		index;
	CONTEXT		context[MAXSTAVES+1];
	STFRANGE	stfRange = {0,0};

	if (!SetupSysPorts(doc, pL)) {
		ErrDisposPorts();
		return;
	}

	/* Drag the object. */
	GetAllContexts(doc, context, pL);
	CheckObject(doc, pL, &found, (Ptr)&pt, context, SMSymDrag, &index, stfRange);
	
	/* Set the selRange and clean up the ports. */
	doc->selStartL = pL;
	doc->selEndL = RightLINK(pL);
	DisposMeasPorts();
}


/* -------------------------------------------------------------------- PageRelDrag -- */
/* Use DragGrayRgn to drag any page-relative objects. As of v. 5.7, only GRAPHICs
can be page-relative. */

void PageRelDrag(Document *doc, LINK pL, Point pt)
{
	Rect r, limitR, slopR;  RgnHandle graphicRgn;  long newPos;
	
	r = LinkOBJRECT(pL);
	OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
	InsetRect(&r,-1,-1);

	pt.h += doc->currentPaper.left;
	pt.v += doc->currentPaper.top;
	
	limitR = slopR = doc->currentPaper;

	graphicRgn = NewRgn();
	RectRgn(graphicRgn, &r);

	newPos = DragGrayRgn(graphicRgn, pt, &limitR, &slopR, noConstraint, NULL);
	LinkXD(pL) += p2d(LoWord(newPos));
	LinkYD(pL) += p2d(HiWord(newPos));
	
	if (newPos) doc->changed = TRUE;		/* If 0, neither h nor v has changed */
	InvalWindow(doc);
}


/* -------------------------------------------------------------- SDSpaceMeasures -- */

void SDSpaceMeasures(Document *doc, LINK pL, long spaceFactor)
{
	LINK prevMeasL;

	if (MeasureTYPE(pL)) {
		prevMeasL = LSSearch(LeftLINK(pL), MEASUREtype, ANYONE, GO_LEFT, FALSE);
		if (doc->autoRespace && prevMeasL)
			RespaceBars(doc, RightLINK(prevMeasL), LeftLINK(pL), spaceFactor,
							FALSE, FALSE);
	}
}

/* -------------------------------------------------------------- SDInvalObjRects -- */
/* Erase and inval the old and new objRects of a dragged object, translating
to the coordinate system of doc->currentPaper. */

static void SDInvalObjRects(Document *doc, LINK pL, DDIST xdDiff, DDIST ydDiff,
										short offset)
{
	Rect r;

	r = LinkOBJRECT(pL);
	OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
	InsetRect(&r,-offset,-offset);
	EraseAndInval(&r);							/* Erase new rect */

	OffsetRect(&r,-d2p(xdDiff),-d2p(ydDiff));
	EraseAndInval(&r);							/* Erase previous rect */
}

/* --------------------------------------------------------------- SDInvalMeasures -- */
/* Erase and inval the range of measures containing a dragged object. If the
object is before1st, erase the system; else if drag1stMeas and dragLastMeas are
set, inval that range of measures; else inval the measure in the data structure
containing the object. */

static void SDInvalMeasures(Document *doc, LINK pL)
{
	LINK measL,rMeas;

	if (LinkBefFirstMeas(pL)) {
		InvalSystem(pL);
	}
	else if (drag1stMeas && dragLastMeas) {
		InvalMeasures(drag1stMeas,dragLastMeas, ANYONE);
	}
	else {
		measL = LSSearch(pL,MEASUREtype,ANYONE,GO_LEFT,FALSE);
		rMeas = LinkRMEAS(measL) ? LinkRMEAS(measL) : doc->tailL;
		InvalMeasures(measL,rMeas, ANYONE);
	}
}

/* ----------------------------------------------------------------- SDInvalObject -- */

static void SDInvalObject(Document *doc, LINK pL, DDIST xdDiff, DDIST ydDiff, short offset)
{
	SDInvalObjRects(doc, pL, xdDiff, ydDiff, offset);
	SDInvalMeasures(doc, pL);
}

/* --------------------------------------------------------------------- SDCleanUp -- */
/* Finish up operations of SymDragLoop. */

void SDCleanUp(
		Document *doc,
		GrafPtr oldPort,
		long spaceFactor,				/* Amount by which to respace dragged measures */
		LINK pL,
		Boolean dirty,
		DDIST xdDiff, DDIST ydDiff
		)
{
	SetPort(oldPort);
	DeselAll(doc);
	
	/* If dirty, dragging operation has changed the score, so set doc->changed.
		Otherwise, don't set the flag at all, since other operations may have 
		affected it. */
	doc->selStartL = doc->selEndL = RightLINK(pL);

	if (dirty) {
		SDSpaceMeasures(doc, pL, spaceFactor);
		doc->changed = TRUE;
		switch (ObjLType(pL)) {
			case MEASUREtype:
				InvalMeasures(LinkLMEAS(pL), pL, ANYONE);
				break;
			case DYNAMtype:				/* Handled elsewhere */ 
				if (IsHairpin(pL))
					InvalMeasures(DynamFIRSTSYNC(pL), DynamLASTSYNC(pL), ANYONE);
				else 	InvalMeasure(pL,1);
				break;
			case BEAMSETtype:
				InvalMeasures(FirstInBeam(pL), LastInBeam(pL), ANYONE);
				break;
			case OTTAVAtype:
				SDInvalObjRects(doc, pL, xdDiff, ydDiff, 1);
				InvalMeasures(FirstInOttava(pL), LastInOttava(pL), ANYONE);
				break;
			case TUPLETtype:
				SDInvalObjRects(doc, pL, xdDiff, ydDiff, 1);
				InvalMeasures(FirstInTuplet(pL), LastInTuplet(pL), ANYONE);
				break;
			case SLURtype:
				InvalObject(doc,pL,FALSE);
				SDInvalObjRects(doc, pL, xdDiff, ydDiff, 1);
				InvalMeasures(SlurFIRSTSYNC(pL), SlurLASTSYNC(pL), ANYONE);
				break;
			case ENDINGtype:
				SDInvalObjRects(doc, pL, xdDiff, ydDiff, 4);
				break;
			case GRAPHICtype:
				SDInvalObject(doc, pL, xdDiff, ydDiff, 1);
				break;
			case TEMPOtype:
				InvalObject(doc,pL,FALSE);
				SDInvalObject(doc, pL, xdDiff, ydDiff, 1);
				break;
			default:
				InvalMeasure(pL,1);
		}
	}
}


/* --------------------------------------------------------------- DisposMeasPorts -- */
/* Dispose of the ports. */

void DisposMeasPorts()
{
#ifdef USE_GWORLDS
	DestroyGWorld((GWorldPtr)underBits);
	DestroyGWorld((GWorldPtr)offScrBits);
	DestroyGWorld((GWorldPtr)picBits);
#else
	DisposGrafPort(underBits);
	DisposGrafPort(offScrBits);
	DisposGrafPort(picBits);
#endif
}

/* ---------------------------------------------------------------- ErrDisposPorts -- */

void ErrDisposPorts()
{
#ifdef USE_GWORLDS
	if (underBits)
		DestroyGWorld((GWorldPtr)underBits);
	if (offScrBits)
		DestroyGWorld((GWorldPtr)offScrBits);
	if (picBits)
		DestroyGWorld((GWorldPtr)picBits);
#else
	if (underBits)
		DisposGrafPort(underBits);
	if (offScrBits)
		DisposGrafPort(offScrBits);
	if (picBits)
		DisposGrafPort(picBits);
#endif

	GetIndCString(strBuf, MISCERRS_STRS, 10);			/* "Not enough memory to drag" */
	CParamText(strBuf, "", "", "");
	StopInform(GENERIC_ALRT);

	underBits = offScrBits = picBits = NULL;
}
