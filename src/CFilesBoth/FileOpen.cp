/******************************************************************************************
*	FILE:	FileOpen.c
*	PROJ:	Nightingale
*	DESC:	Input of normal Nightingale files, plus file format conversion, etc.
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2018 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"
#include "FileConversion.h"			/* Must follow Nightingale.precomp.h! */
#include "CarbonPrinting.h"
#include "FileUtils.h"
#include "MidiMap.h"

/* A version code is 'N' followed by three digits, e.g., 'N105': N-one-zero-five. Be
careful: It's neither a number nor a valid C string! */
static unsigned long version;							/* File version code read/written */

/* Prototypes for internal functions: */
static Boolean ConvertScoreContent(Document *, long);
static void DisplayDocumentHdr(short id, Document *doc);
static short CheckDocumentHdr(Document *doc);
static void DisplayScoreHdr(short id, Document *doc);
static short CheckScoreHdr(Document *doc);

static void ConvertScoreHeader(Document *doc, DocumentN105 *docN105);

static Boolean ModifyScore(Document *, long);
static void SetTimeStamps(Document *);

/* Codes for object types being read/written or for non-read/write call when an I/O
error occurs; note that all are negative. See HeapFileIO.c for additional, positive,
codes. */

enum {
	HEADERobj=-999,
	VERSIONobj,
	SIZEobj,
	CREATEcall,
	OPENcall,				/* -995 */
	CLOSEcall,
	DELETEcall,
	RENAMEcall,
	WRITEcall,
	STRINGobj,
	INFOcall,
	SETVOLcall,
	BACKUPcall,
	MAKEFSSPECcall,
	NENTRIESerr = -899
};


/* ------------------------------------------------------- ConvertScoreContent helpers -- */

static void OldGetSlurContext(Document *, LINK, Point [], Point []);
static void ConvertChordSlurs(Document *);
static void ConvertModNRVPositions(Document *, LINK);
static void ConvertStaffLines(LINK startL);

/* Given a Slur object, return arrays of the paper-relative starting and ending
positions (expressed in points) of the notes delimiting its subobjects. This is an
ancient version of GetSlurContext, from Nightingale .996. */

static void OldGetSlurContext(Document *doc, LINK pL, Point startPt[], Point endPt[])
	{
		CONTEXT 	localContext;
		DDIST		dFirstLeft, dFirstTop, dLastLeft, dLastTop,
					xdFirst, xdLast, ydFirst, ydLast;
		PANOTE		aNote, firstNote, lastNote;
		PSLUR		p;
		PASLUR		aSlur;
		LINK		aNoteL, firstNoteL, lastNoteL, aSlurL, firstSyncL, 
					lastSyncL, pSystemL;
		PSYSTEM		pSystem;
		short		k, xpFirst, xpLast, ypFirst, ypLast, firstStaff, lastStaff;
		SignedByte	firstInd, lastInd;
		Boolean		firstMEAS, lastSYS;

		firstMEAS = lastSYS = False;
			
		/* Handle special cases for crossSystem & crossStaff slurs. If p->firstSyncL is
		   not a sync, it must be a measure; find the first sync to get the context from.
		   If p->crossStaff, and slur drawn bottom to top, firstStaff is one greater, else
		   lastStaff. */
		   
		p = GetPSLUR(pL);
		firstStaff = lastStaff = p->staffn;
		if (p->crossStaff) {
			if (p->crossStfBack)
					firstStaff += 1;
			else	lastStaff += 1;
		}

		if (SyncTYPE(p->firstSyncL))
			GetContext(doc, p->firstSyncL, firstStaff, &localContext);	/* Get left end context */
		else {
			if (MeasureTYPE(p->firstSyncL)) firstMEAS = True;
			else MayErrMsg("OldGetSlurContextontext: for pL=%ld firstSyncL=%ld is bad",
								(long)pL, (long)p->firstSyncL);
			firstSyncL = LSSearch(p->firstSyncL, SYNCtype, firstStaff, GO_RIGHT, False);
			GetContext(doc, firstSyncL, firstStaff, &localContext);
		}
		dFirstLeft = localContext.measureLeft;							/* abs. origin of left end coords. */
		dFirstTop = localContext.measureTop;
		
		/* Handle special cases for crossSystem slurs. If p->lastSyncL is not a sync,
		   it must be a system, and must have an RSYS; find the first sync to the left
		   of RSYS to get the context from. */
		   
		p = GetPSLUR(pL);
		if (SyncTYPE(p->lastSyncL))
			GetContext(doc, p->lastSyncL, lastStaff, &localContext);	/* Get right end context */
		else {
			if (SystemTYPE(p->lastSyncL) && LinkRSYS(p->lastSyncL)) {
				lastSYS = True;
				pSystemL = p->lastSyncL;
				lastSyncL = LSSearch(LinkRSYS(pSystemL), SYNCtype, lastStaff, GO_LEFT, False);
				GetContext(doc, lastSyncL, lastStaff, &localContext);
			}
			else MayErrMsg("OldGetSlurContextontext: for pL=%ld lastSyncL=%ld is bad",
								(long)pL, (long)p->lastSyncL);
		}
		if (!lastSYS)
			dLastLeft = localContext.measureLeft;						/* abs. origin of right end coords. */
		else {
			pSystem = GetPSYSTEM(pSystemL);
			dLastLeft = pSystem->systemRect.right;
		}
		dLastTop = localContext.measureTop;

		/* Find the links to the first and last notes to which each slur/tie is attached */
		
		p = GetPSLUR(pL);
		aSlurL = FirstSubLINK(pL);
		for (k = 0; aSlurL; k++, aSlurL = NextSLURL(aSlurL)) {
			firstInd = lastInd = -1;
			if (!firstMEAS) {
				aNoteL = FirstSubLINK(p->firstSyncL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->voice==p->voice) {
						aSlur = GetPASLUR(aSlurL);
						if (++firstInd == aSlur->firstInd) firstNoteL = aNoteL;
					}
				}
			}
		
			if (!lastSYS) {
				aNoteL = FirstSubLINK(p->lastSyncL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (aNote->voice==p->voice) {
						aSlur = GetPASLUR(aSlurL);
						if (++lastInd  == aSlur->lastInd) lastNoteL = aNoteL;
					}
				}
			}
			
			if (firstMEAS) {
				lastNote = GetPANOTE(lastNoteL);
				xdFirst = dFirstLeft;
				ydFirst = dFirstTop + lastNote->yd;
			}
			else {
				firstNote = GetPANOTE(firstNoteL);
				xdFirst = dFirstLeft + LinkXD(p->firstSyncL) + firstNote->xd;	/* abs. position of 1st note */
				ydFirst = dFirstTop + firstNote->yd;
			}
			
			if (lastSYS) {
				firstNote = GetPANOTE(firstNoteL);
				xdLast = dLastLeft;
				ydLast = dLastTop + firstNote->yd;
			}
			else {
				lastNote = GetPANOTE(lastNoteL);
				xdLast = dLastLeft + LinkXD(p->lastSyncL) + lastNote->xd;		/* abs. position of last note */
				ydLast = dLastTop + lastNote->yd;
			}

			xpFirst = d2p(xdFirst);
			ypFirst = d2p(ydFirst);
			xpLast = d2p(xdLast);
			ypLast = d2p(ydLast);
			SetPt(&startPt[k], xpFirst, ypFirst);
			SetPt(&endPt[k], xpLast, ypLast);
		}
	}


static void ConvertChordSlurs(Document *doc)
{
	LINK pL, aNoteL, aSlurL;  PASLUR aSlur;  Boolean foundChordSlur;
	Point startPt[2],endPt[2], oldStartPt[2],oldEndPt[2];
	short v, changeStart, changeEnd;
	
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		if (SlurTYPE(pL) && !SlurTIE(pL)) {
			foundChordSlur = False;
			v = SlurVOICE(pL);
			if (SyncTYPE(SlurFIRSTSYNC(pL))) {
				aNoteL = FindMainNote(SlurFIRSTSYNC(pL), v); 
				if (NoteINCHORD(aNoteL)) foundChordSlur = True;
			}
			if (SyncTYPE(SlurLASTSYNC(pL))) {
				aNoteL = FindMainNote(SlurLASTSYNC(pL), v); 
				if (NoteINCHORD(aNoteL)) foundChordSlur = True;
			}

			if (foundChordSlur) {
				GetSlurContext(doc, pL, startPt, endPt);
				OldGetSlurContext(doc, pL, oldStartPt, oldEndPt);
				changeStart = startPt[0].v-oldStartPt[0].v;
				changeEnd = endPt[0].v-oldEndPt[0].v;
	
				aSlurL = FirstSubLINK(pL);
				aSlur = GetPASLUR(aSlurL);
				aSlur->seg.knot.v -= p2d(changeStart);
				aSlur->endKnot.v -= p2d(changeEnd);
			}
		}
	}
}

static void ConvertModNRVPositions(Document */*doc*/, LINK syncL)
{
	LINK aNoteL, aModNRL;
	PANOTE aNote;  PAMODNR aModNR;
	short yOff;  Boolean above;
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->firstMod) {
			aModNRL = aNote->firstMod;
			for ( ; aModNRL; aModNRL=NextMODNRL(aModNRL)) {
				aModNR = GetPAMODNR(aModNRL);
				switch (aModNR->modCode) {
					case MOD_FERMATA:
						above = (aModNR->ystdpit<=0);		/* Not guaranteed but usually right */
						yOff = (above? 4 : -4);
						break;
					case MOD_TRILL:
						yOff = 4;
						break;
					case MOD_ACCENT:
						yOff = 3;
						break;
					case MOD_HEAVYACCENT:
						yOff = 5;
						break;
					case MOD_WEDGE:
						yOff = 2;
						break;
					case MOD_MORDENT:
						yOff = 4;
						break;
					case MOD_INV_MORDENT:
						yOff = 4;
						break;
					case MOD_UPBOW:
						yOff = 6;
						break;
					case MOD_DOWNBOW:
						yOff = 5;
						break;
					case MOD_HEAVYACC_STACC:
						yOff = 5;
						break;
					case MOD_LONG_INVMORDENT:
						yOff = 4;
						break;
					default:
						yOff = 0;
				}
				
				aModNR->ystdpit -= yOff;	/* Assumes ystdpit units (STDIST) = 1/8 spaces */
			}
		}
	}
}

/* Convert old staff <oneLine> field to new <showLines> field. Also initialize new
<showLedgers> field.
		Old ASTAFF had this:
			char			filler:7,
							oneLine:1;
		New ASTAFF has this:
			char			filler:3,
							showLedgers:1,
							showLines:4;
<filler> was initialized to zero in InitStaff, so we can just see if showLines is
non-zero. If it is, then the oneLine flag was set. 	-JGG, 7/22/01 */

static void ConvertStaffLines(LINK startL)
{
	LINK pL, aStaffL;

	for (pL = startL; pL; pL = RightLINK(pL)) {
		if (ObjLType(pL)==STAFFtype) {
			for (aStaffL = FirstSubLINK(pL); aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				Boolean oneLine = (StaffSHOWLINES(aStaffL)!=0);		/* NB: this really is "oneLine"! */
				if (oneLine)
					StaffSHOWLINES(aStaffL) = 1;
				else
					StaffSHOWLINES(aStaffL) = SHOW_ALL_LINES;
				StaffSHOWLEDGERS(aStaffL) = True;
#ifdef STAFFRASTRAL
				// NB: Bad if NightStaffSizer used! See StaffSize2Rastral in ResizeStaff.c
				// from NightStaffSizer code as a way to a possible solution.
				StaffRASTRAL(aStaffL) = doc->srastral;
#endif
			}
		}
	}
}


/* --------------------------------------------------------------- ConvertScoreContent -- */
/* Any file-format-conversion code that doesn't affect the length of the header or
lengths or offsets of fields in objects (or subobjects) should go here. Return True if
all goes well, False if not.

This function should not be called until the header and the entire object list have been
read! Tweaks that affect lengths or offsets to the header should be done in OpenFile; to
to objects (or subobjects), in ReadObjHeap. */

static Boolean ConvertScoreContent(Document *doc, long fileTime)
{
	LINK pL;  DateTimeRec date;
	
	SecondsToDate(fileTime, &date);

	/* Put all Dynamic horizontal position info into object xd */
	
	if (version<='N100') {
		LINK aDynamicL;  PADYNAMIC aDynamic;
	
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==DYNAMtype) {
				aDynamicL = FirstSubLINK(pL);
				aDynamic = GetPADYNAMIC(aDynamicL);
				LinkXD(pL) += aDynamic->xd;
				aDynamic->xd = 0;
			}
	}

	/* Convert Ottava position info to new form: if nxd or nyd is nonzero, it's in the
	   old form, so move values into xdFirst and ydFirst, and copy them into xdLast and
	   ydLast also. */
		
	if (version<='N100') {
		POTTAVA ottavap;
	
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==OTTAVAtype) {
				ottavap = GetPOTTAVA(pL);
				if (ottavap->nxd!=0 || ottavap->nyd!=0) {
					ottavap->xdFirst = ottavap->xdLast = ottavap->nxd;
					ottavap->ydFirst = ottavap->ydLast = ottavap->nyd;
					ottavap->nxd = ottavap->nyd = 0;
				}
			}
	}

	/* Move Measure "fake" flag from subobject to object. If file version code is
	   N100, it's tricky to decide whether to do this, but we'll try. */
	
	if (version<='N099'
	|| (version=='N100' && date.year==1991
		&& (date.month<=8 || (date.month==9 && date.day<=6))) ) {
		LINK aMeasL; PAMEASURE aMeas;
	
		if (version>'N099') {
			/* This is all but obsolete--it's not worth moving this string to a resource */
			CParamText("File has a suspicious date, so converting measFake flags.", "", "", "");
			CautionInform(GENERIC_ALRT);
		}
		
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==MEASUREtype) {
				aMeasL = FirstSubLINK(pL);
				aMeas = GetPAMEASURE(aMeasL);
				MeasISFAKE(pL) = aMeas->oldFakeMeas;
			}
	}

	/* Convert Tuplet position info to new form: if acnxd or acnyd is nonzero, it's in the
	   old form, so move values into xdFirst and ydFirst, and copy them into xdLast and
	   ydLast also. */
	   
	if (version<='N100') {
		PTUPLET pTuplet;
	
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==TUPLETtype) {
				pTuplet = GetPTUPLET(pL);
				if (pTuplet->acnxd!=0 || pTuplet->acnyd!=0) {
					pTuplet->xdFirst = pTuplet->xdLast = pTuplet->acnxd;
					pTuplet->ydFirst = pTuplet->ydLast = pTuplet->acnyd;
					pTuplet->acnxd = pTuplet->acnyd = 0;
				}
			}
	}

	/* Move all slurs to correct position in object list, immediately before their
	   firstSyncL. If the slur is a SlurLastIsSYSTEM slur, move it immediately
	   after the first invis meas, e.g. before the RightLINK of its firstSyncL. */
		
	if (version<='N100') {
		LINK nextL;
		
		for (pL = doc->headL; pL; pL = nextL) {
			nextL = RightLINK(pL);
			if (SlurTYPE(pL))
				if (!SlurLastIsSYSTEM(pL))
					MoveNode(pL,SlurFIRSTSYNC(pL));
				else
					MoveNode(pL,RightLINK(SlurFIRSTSYNC(pL)));
		}
	}

	/* Look for slurs with <dashed> flag set, which is probably spurious, and if we find
	   any, offer to fix them. */
		
	if (version<='N100') {
		LINK aSlurL;  PASLUR aSlur;  Boolean foundDashed=False;
		
		for (pL = doc->headL; pL && !foundDashed; pL = RightLINK(pL)) 
			if (SlurTYPE(pL)) {
				aSlurL = FirstSubLINK(pL);
				for ( ; aSlurL; aSlurL = NextSLURL(aSlurL)) {
					aSlur = GetPASLUR(aSlurL);
					if (aSlur->dashed) { foundDashed = True; break; }
				}
			}

		if (foundDashed) {
			GetIndCString(strBuf, FILEIO_STRS, 4);		/* "Found dashed slurs that may be spurious" */
			CParamText(strBuf, "", "", "");
			if (CautionAdvise(GENERIC3_ALRT)==OK) {
				for (pL = doc->headL; pL; pL = RightLINK(pL)) 
					if (SlurTYPE(pL)) {
						aSlurL = FirstSubLINK(pL);
						for ( ; aSlurL; aSlurL = NextSLURL(aSlurL)) {
							aSlur = GetPASLUR(aSlurL);
							aSlur->dashed = 0;
						}
					}
			}
		}
	}

	/* Set Clef <small> flag according to whether it's <inMeas>: this is to make
		clef size explicit, so it can be overridden. */

	if (version<='N100') {
		Boolean inMeas; LINK aClefL; PACLEF aClef;
		
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==CLEFtype) {
				inMeas = ClefINMEAS(pL);
				for (aClefL = FirstSubLINK(pL); aClefL; aClefL = NextCLEFL(aClefL)) {
					aClef = GetPACLEF(aClefL);
					aClef->small = inMeas;
				}
			}
	}

	/* Convert octave sign y-position to new representation */
	
	if (version<='N100') {
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==OTTAVAtype) {
				SetOttavaYPos(doc, pL);
			}
	}

	/* Convert Tempo metronome mark from int to string */
	
	if (version<='N100') {
		PTEMPO pTempo;  long beatsPM;  Str255 string;
		STRINGOFFSET offset;
		
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==TEMPOtype) {
				pTempo = GetPTEMPO(pL);
				beatsPM = pTempo->tempoMM;
				NumToString(beatsPM, string);
				offset = PStore(string);
				if (offset<0L)
					{ NoMoreMemory(); return False; }
				else
					TempoMETROSTR(pL) = offset;
			}
	}

	/* Move Beamset fields to adjust for removal of the <lGrip> and <rGrip> fields */
	
	if (version<='N100') {
		PBEAMSET pBeamset;
			
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==BEAMSETtype) {
				pBeamset = GetPBEAMSET(pL);
				/* Move fields from <voice> to end--many fields but only 2 bytes! */
				BlockMove((Ptr)(&pBeamset->voice)+8, (Ptr)(&pBeamset->voice), 2);
			}
	}


	/* Move endpoints of slurs on chords. If file version code is N101 and file was written
	by .997a12 or later (flagged by top bit of <fileTime> off!), this should never be done;
	if file version code is N101 and it was written by an earlier version, it probably
	should be, but ask. */

	if (version<='N100'
	|| (version=='N101' && (fileTime & 0x80000000)!=0)
	|| ShiftKeyDown()) {
		short v;  LINK aNoteL;  Boolean foundChordSlur;
		
		for (foundChordSlur = False, pL = doc->headL; pL; pL = RightLINK(pL)) {
			if (SlurTYPE(pL) && !SlurTIE(pL)) {
				v = SlurVOICE(pL);
				if (SyncTYPE(SlurFIRSTSYNC(pL))) {
					aNoteL = FindMainNote(SlurFIRSTSYNC(pL), v); 
					if (NoteINCHORD(aNoteL)) { foundChordSlur = True; break; }
				}
				if (SyncTYPE(SlurLASTSYNC(pL))) {
					aNoteL = FindMainNote(SlurLASTSYNC(pL), v); 
					if (NoteINCHORD(aNoteL)) { foundChordSlur = True; break; }
				}
			}
		}
		
		if (foundChordSlur) {
			if (version<='N100')
				ConvertChordSlurs(doc);
			else {
				GetIndCString(strBuf, FILEIO_STRS, 5);		/* "Found slur(s) starting/ending with chords" */
				CParamText(strBuf, "", "", "");
				if (CautionAdvise(GENERIC3_ALRT)==OK)
					ConvertChordSlurs(doc);
			}
		}
	}

	/* Fill in page no. position pseudo-Rect. */

	if (version<='N101')
		doc->headerFooterMargins = config.pageNumMarg;

	/* Update ModNR vertical positions to compensate for new centering. */
	
	if (version<='N101')
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (SyncTYPE(pL))
				ConvertModNRVPositions(doc, pL);
		
	/* Update Graphic horizontal positions to compensate for new subobj-relativity. */
	
	if (version<='N101') {
		LINK firstObjL;  DDIST newxd, oldxd;  CONTEXT context;
		
		for (pL = doc->headL; pL; pL = RightLINK(pL))
			if (GraphicTYPE(pL)) {
				firstObjL = GraphicFIRSTOBJ(pL);
				if (!firstObjL || PageTYPE(firstObjL)) continue;
				
				GetContext(doc, firstObjL, GraphicSTAFF(pL), &context);
				newxd = GraphicPageRelxd(doc, pL,firstObjL, &context);
				oldxd = PageRelxd(firstObjL, &context);
				LinkXD(pL) += oldxd-newxd;		
			}
	}
	
	/* Initialize dynamic subobject endyd field. */

	if (version<='N102') {
		for (pL = doc->headL; pL; pL = RightLINK(pL)) 
			if (ObjLType(pL)==DYNAMtype) {
				LINK aDynamicL = FirstSubLINK(pL);
				if (IsHairpin(pL))
					DynamicENDYD(aDynamicL) = DynamicYD(aDynamicL);
				else
					DynamicENDYD(aDynamicL) = 0;
			}
	}

	/* Initialize header subobject (part) bank select and FreeMIDI fields for
		both main data structure and master page. */

	if (version<='N102') {
		PPARTINFO pPart; LINK partL;

		partL = NextPARTINFOL(FirstSubLINK(doc->headL));
		for ( ; partL; partL = NextPARTINFOL(partL)) {
			pPart = GetPPARTINFO(partL);
			pPart->fmsOutputDevice = noUniqueID;
			/* FIXME: We're probably not supposed to play with these fields... */
			pPart->fmsOutputDestination.basic.destinationType = 0,
			pPart->fmsOutputDestination.basic.name[0] = 0;
		}

		partL = NextPARTINFOL(FirstSubLINK(doc->masterHeadL));
		for ( ; partL; partL = NextPARTINFOL(partL)) {
			pPart = GetPPARTINFO(partL);
			pPart->fmsOutputDevice = noUniqueID;
			/* FIXME: We're probably not supposed to play with these fields... */
			pPart->fmsOutputDestination.basic.destinationType = 0,
			pPart->fmsOutputDestination.basic.name[0] = 0;
		}
	}

	/* For GRChordSym graphics:
		1)	Initialize info field. This is now used for chord symbol options -- currently
			only whether to show parentheses around extensions -- so we set it to the
			current config setting that used to be the only way to toggle showing 
			parentheses. This seems like the best way to get what the user expects.
		2) Append the chord symbol delimiter character to the graphic string, to represent
			an empty substring for the new "/bass" field.
																-JGG, 6/16/01 */
	
	if (version<='N102') {
		Str255 string, delim;  LINK aGraphicL;  STRINGOFFSET offset;

		delim[0] = 1; delim[1] = CS_DELIMITER;
		for (pL = doc->headL; pL; pL = RightLINK(pL)) {
			if (ObjLType(pL)==GRAPHICtype && GraphicSubType(pL)==GRChordSym) {
				GraphicINFO(pL) = config.chordSymDrawPar? 1 : 0;

				aGraphicL = FirstSubLINK(pL);
				Pstrcpy((StringPtr)string, (StringPtr)PCopy(GraphicSTRING(aGraphicL)));
				PStrCat(string, delim);
				offset = PReplace(GraphicSTRING(aGraphicL), string);
				if (offset<0L) {
					NoMoreMemory();
					return False;
				}
				else
					GraphicSTRING(aGraphicL) = offset;
			}
		}
	}

	/* Convert old staff <oneLine> field to new <showLines> field, and initialize
		new <showLedgers> field, for staves in the score and in master page. */

	if (version<='N102') {
		ConvertStaffLines(doc->headL);
		ConvertStaffLines(doc->masterHeadL);
	}

	/* Make sure all staves are visible in Master Page. They should never be invisible,
	but (as of v.997), they sometimes were, probably because not exporting changes to
	Master Page was implemented by reconstructing it from the 1st system of the score.
	That was fixed in about .998a10, so it should be safe to remove this call
	before too long, but making them visible here shouldn't cause any problems. */

	VisifyMasterStaves(doc);
	
	return True;
}


/* ----------------------------------------------------------------------- ModifyScore -- */
/* Any temporary file-content-updating code (a.k.a. hacking) that doesn't affect the
length of the header or lengths of objects should go here. This function should only be
called after the header and entire object list have been read. Return True if all goes
well, False if not.

NB: If code here considers changing something, and especially if it ends up actually
doing so, it should call LogPrintf to display at least one very prominent message
in the console window, and SysBeep to draw attention to it! It should perhaps also
set doc->changed, though this will make it easier for people to accidentally overwrite
the original version.

NB2: Be sure that all of this code is removed or commented out in ordinary versions!
To facilitate that, when done with hacking, add an "#error" line; cf. examples
below. */

#ifdef SWAP_STAVES
#error ATTEMPTED TO COMPILE OLD HACKING CODE for ModifyScore!
static void ShowStfTops(Document *doc, LINK pL, short staffN1, short staffN2);
static void ShowStfTops(Document *doc, LINK pL, short staffN1, short staffN2)
{
	CONTEXT context;
	short staffTop1, staffTop2;
	pL = SSearch(doc->headL, STAFFtype, False);
	GetContext(doc, pL, staffN1, &context);
	staffTop1 =  context.staffTop;
	GetContext(doc, pL, staffN2, &context);
	staffTop2 =  context.staffTop;
	LogPrintf(LOG_NOTICE, "ShowStfTops(%d): staffTop1=%d, staffTop2=%d\n", pL, staffTop1, staffTop2);
}

static void SwapStaves(Document *doc, LINK pL, short staffN1, short staffN2);
static void SwapStaves(Document *doc, LINK pL, short staffN1, short staffN2)
{
	LINK aStaffL, aMeasureL, aPSMeasL, aClefL, aKeySigL, aTimeSigL, aNoteL,
			aTupletL, aRptEndL, aDynamicL, aGRNoteL;
	CONTEXT context;
	short staffTop1, staffTop2;

	switch (ObjLType(pL)) {
		case HEADERtype:
			break;

		case PAGEtype:
			break;

		case SYSTEMtype:
			break;

		case STAFFtype:
LogPrintf(LOG_NOTICE, "  Staff L%d\n", pL);
			GetContext(doc, pL, staffN1, &context);
			staffTop1 =  context.staffTop;
			GetContext(doc, pL, staffN2, &context);
			staffTop2 =  context.staffTop;
LogPrintf(LOG_NOTICE, "    staffTop1, 2=%d, %d\n", staffTop1, staffTop2);
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				if (StaffSTAFF(aStaffL)==staffN1) {
					StaffSTAFF(aStaffL) = staffN2;
					StaffTOP(aStaffL) = staffTop2;
				}
				else if (StaffSTAFF(aStaffL)==staffN2) {
					StaffSTAFF(aStaffL) = staffN1;
					StaffTOP(aStaffL) = staffTop1;
				}
//GetContext(doc, pL, staffN1, &context);
//LogPrintf(LOG_NOTICE, "(1)    pL=%d staffTop1=%d\n", pL, staffTop1);
			}
//GetContext(doc, pL, staffN1, &context);
//LogPrintf(LOG_NOTICE, "(2)    pL=%d staffTop1=%d\n", pL, staffTop1);
//ShowStfTops(doc, pL, staffN1, staffN2);
			break;

		case CONNECTtype:
			break;

		case MEASUREtype:
LogPrintf(LOG_NOTICE, "  Measure L%d\n", pL);
			aMeasureL = FirstSubLINK(pL);
			for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
				if (MeasureSTAFF(aMeasureL)==staffN1) MeasureSTAFF(aMeasureL) = staffN2;
				else if (MeasureSTAFF(aMeasureL)==staffN2) MeasureSTAFF(aMeasureL) = staffN1;
			}
			break;

		case PSMEAStype:
LogPrintf(LOG_NOTICE, "  PSMeas L%d\n", pL);
			aPSMeasL = FirstSubLINK(pL);
			for ( ; aPSMeasL; aPSMeasL = NextPSMEASL(aPSMeasL)) {
				if (PSMeasSTAFF(aPSMeasL)==staffN1) PSMeasSTAFF(aPSMeasL) = staffN2;
				else if (PSMeasSTAFF(aPSMeasL)==staffN2) PSMeasSTAFF(aPSMeasL) = staffN1;
			}
			break;

		case CLEFtype:
LogPrintf(LOG_NOTICE, "  Clef L%d\n", pL);
			aClefL = FirstSubLINK(pL);
			for ( ; aClefL; aClefL = NextCLEFL(aClefL)) {
				if (ClefSTAFF(aClefL)==staffN1) ClefSTAFF(aClefL) = staffN2;
				else if (ClefSTAFF(aClefL)==staffN2) ClefSTAFF(aClefL) = staffN1;
			}
			break;

		case KEYSIGtype:
LogPrintf(LOG_NOTICE, "  Keysig L%d\n", pL);
			aKeySigL = FirstSubLINK(pL);
			for ( ; aKeySigL; aKeySigL = NextKEYSIGL(aKeySigL)) {
				if (KeySigSTAFF(aKeySigL)==staffN1) KeySigSTAFF(aKeySigL) = staffN2;
				else if (KeySigSTAFF(aKeySigL)==staffN2) KeySigSTAFF(aKeySigL) = staffN1;
			}
			break;

		case TIMESIGtype:
LogPrintf(LOG_NOTICE, "  Timesig L%d\n", pL);
			aTimeSigL = FirstSubLINK(pL);
			for ( ; aTimeSigL; aTimeSigL = NextTIMESIGL(aTimeSigL)) {
				if (TimeSigSTAFF(aTimeSigL)==staffN1) TimeSigSTAFF(aTimeSigL) = staffN2;
				else if (TimeSigSTAFF(aTimeSigL)==staffN2) TimeSigSTAFF(aTimeSigL) = staffN1;
			}
			break;

		case SYNCtype:
LogPrintf(LOG_NOTICE, "  Sync L%d\n", pL);
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				if (NoteSTAFF(aNoteL)==staffN1) {
					NoteSTAFF(aNoteL) = staffN2;
					NoteVOICE(aNoteL) = staffN2;		/* Assumes only 1 voice on the staff */
				}
				else if (NoteSTAFF(aNoteL)==staffN2) {
					NoteSTAFF(aNoteL) = staffN1;
					NoteVOICE(aNoteL) = staffN1;		/* Assumes only 1 voice on the staff */
				}
			}
			break;

		case BEAMSETtype:
LogPrintf(LOG_NOTICE, "  Beamset L%d\n", pL);
			if (BeamSTAFF(pL)==staffN1) BeamSTAFF((pL)) = staffN2;
			else if (BeamSTAFF((pL))==staffN2) BeamSTAFF((pL)) = staffN1;
			break;

		case TUPLETtype:
LogPrintf(LOG_NOTICE, "  Tuplet L%d\n", pL);
			if (TupletSTAFF(pL)==staffN1) TupletSTAFF((pL)) = staffN2;
			else if (TupletSTAFF((pL))==staffN2) TupletSTAFF((pL)) = staffN1;
			break;

/*
		case RPTENDtype:
			?? = FirstSubLINK(pL);
			for (??) {
			}
			break;

		case ENDINGtype:
			?!
			break;
*/
		case DYNAMtype:
LogPrintf(LOG_NOTICE, "  Dynamic L%d\n", pL);
			aDynamicL = FirstSubLINK(pL);
			for ( ; aDynamicL; aDynamicL=NextDYNAMICL(aDynamicL)) {
				if (DynamicSTAFF(aDynamicL)==staffN1) DynamicSTAFF(aDynamicL) = staffN2;
				else if (DynamicSTAFF(aDynamicL)==staffN2) DynamicSTAFF(aDynamicL) = staffN1;
			}
			break;

		case GRAPHICtype:
LogPrintf(LOG_NOTICE, "  Graphic L%d\n", pL);
			if (GraphicSTAFF(pL)==staffN1) GraphicSTAFF((pL)) = staffN2;
			else if (GraphicSTAFF((pL))==staffN2) GraphicSTAFF((pL)) = staffN1;
			break;

/*
		case OTTAVAtype:
			?!
			break;
*/
		case SLURtype:
LogPrintf(LOG_NOTICE, "  Slur L%d\n", pL);
			if (SlurSTAFF(pL)==staffN1) SlurSTAFF((pL)) = staffN2;
			else if (SlurSTAFF((pL))==staffN2) SlurSTAFF((pL)) = staffN1;
			break;

/*
		case GRSYNCtype:
			?? = FirstSubLINK(pL);
			for (??) {
			}
			break;

		case TEMPOtype:
			?!
			break;

		case SPACERtype:
			?!
			break;
*/

		default:
			break;	
	}
}
#endif


static Boolean ModifyScore(Document * /*doc*/, long /*fileTime*/)
{
#ifdef SWAP_STAVES
#error ModifyScore: ATTEMPTED TO COMPILE OLD HACKING CODE!
	/* DAB carelessly put a lot of time into orchestrating his violin concerto with a
		template having the trumpet staff above the horn; this is intended to correct
		that.
		NB: To swap two staves, in addition to running this code, use Master Page to:
			1. Fix the staves' vertical positions
			2. If the staves are Grouped, un-Group and re-Group
			3. Swap the Instrument info
		NB2: If there's more than one voice on either of the staves, this is not
		likely to work at all well. It never worked well enough to be useful for the
		score I wrote it for.
																--DAB, Jan. 2016 */
	
	short staffN1 = 5, staffN2 = 6;
	LINK pL;
	
	SysBeep(1);
	LogPrintf(LOG_NOTICE, "ModifyScore: SWAPPING STAVES %d AND %d (of %d) IN MASTER PAGE....\n",
				staffN1, staffN2, doc->nstaves);
	for (pL = doc->masterHeadL; pL!=doc->masterTailL; pL = RightLINK(pL)) {
		SwapStaves(doc, pL, staffN1, staffN2);
	}
	LogPrintf(LOG_NOTICE, "ModifyScore: SWAPPING STAVES %d AND %d (of %d) IN SCORE OBJECT LIST....\n",
				staffN1, staffN2, doc->nstaves);
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		SwapStaves(doc, pL, staffN1, staffN2);
	}
  	doc->changed = True;

#endif

#ifdef FIX_MASTERPAGE_SYSRECT
#error ModifyScore: ATTEMPTED TO COMPILE OLD FILE-HACKING CODE!
	/* This is to fix a score David Gottlieb is working on, in which Ngale draws
	 * _completely_ blank pages. Nov. 1999.
	 */
	 
	LINK sysL, staffL, aStaffL; DDIST topStaffTop; PASTAFF aStaff;

	LogPrintf(LOG_NOTICE, "ModifyScore: fixing Master Page sysRects and staffTops...\n");
	// Browser(doc,doc->masterHeadL, doc->masterTailL);
	sysL = SSearch(doc->masterHeadL, SYSTEMtype, False);
	SystemRECT(sysL).bottom = SystemRECT(sysL).top+pt2d(72);

	staffL = SSearch(doc->masterHeadL, STAFFtype, False);
	aStaffL = FirstSubLINK(staffL);
	topStaffTop = pt2d(24);
	/* The following assumes staff subobjects are in order from top to bottom! */
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		aStaff->staffTop = topStaffTop;
		topStaffTop += pt2d(36);
	}
  	doc->changed = True;
#endif

#ifdef FIX_UNBEAMED_FLAGS_AUGDOT_PROB
#error ModifyScore: ATTEMPTED TO COMPILE OLD FILE-HACKING CODE!
	short alteredCount; LINK pL;

  /* From SetupNote in Objects.c:
   * "On upstemmed notes with flags, xmovedots should really 
   * be 2 or 3 larger [than normal], but then it should change whenever 
   * such notes are beamed or unbeamed or their stem direction or 
   * duration changes." */

  /* TC Oct 7 1999 (modified by DB, 8 Oct.): quick and dirty hack to fix this problem
   * for Bill Hunt; on opening file, look at all unbeamed dotted notes with subtype >
   * quarter note and if they have stem up, increase xmovedots by 2; if we find any
   * such notes, set doc-changed to True then simply report to user. It'd probably
   * be better to consider stem length and not do this for notes with very long
   * stems, but we don't consider that. */

  LogPrintf(LOG_NOTICE, "ModifyScore: fixing augdot positions for unbeamed upstemmed notes with flags...\n");
  alteredCount = 0;
  for (pL = doc->headL; pL; pL = RightLINK(pL)) 
    if (ObjLType(pL)==SYNCtype) {
      LINK aNoteL = FirstSubLINK(pL);
      for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
        PANOTE aNote = GetPANOTE(aNoteL);
        if((aNote->subType > QTR_L_DUR) && (aNote->ndots))
          if(!aNote->beamed)
            if(MainNote(aNoteL) && (aNote->yd > aNote->ystem)) {
              aNote->xmovedots = 3 + WIDEHEAD(aNote->subType) + 2;
              alteredCount++;
            }
      }
    }
  
  if (alteredCount) {
  	doc->changed = True;

    NumToString((long)alteredCount,(unsigned char *)strBuf);
    ParamText((unsigned char *)strBuf, 
      (unsigned char *)"\p unbeamed notes with stems up found; aug-dots corrected. NB Save the file with a new name!", 
        (unsigned char *)"", (unsigned char *)"");
    CautionInform(GENERIC2_ALRT);
  }
#endif

#ifdef FIX_BLACK_SCORE
#error ModifyScore: ATTEMPTED TO COMPILE OLD FILE-HACKING CODE!
	/* Sample trashed-file-fixing hack: in this case, to fix an Arnie Black score. */
	
	for (pL = doc->headL; pL; pL = RightLINK(pL)) 
		if (pL>=492 && ObjLType(pL)==SYNCtype) {
			LINK aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				NoteBEAMED(aNoteL) = False;
		}
#endif

	return True;
}



static void ConvertScoreHeader(Document *doc, DocumentN105 *docN105)
{
	doc->headL = docN105->headL;
	doc->tailL = docN105->tailL;
	doc->selStartL = docN105->selStartL;
	doc->selEndL = docN105->selEndL;
	doc->nstaves = docN105->nstaves;
	doc->nsystems = docN105->nsystems;
	//comment[MAX_COMMENT_LEN+1]
	doc->feedback = docN105->feedback;
	doc->dontSendPatches = docN105->dontSendPatches;
	doc->saved = docN105->saved;
	doc->named = docN105->named;
	doc->used = docN105->used;
	doc->transposed = docN105->transposed;
	doc->lyricText = docN105->lyricText;
	doc->polyTimbral = docN105->polyTimbral;
	doc->currentPage = docN105->currentPage;
	doc->spacePercent = docN105->spacePercent;
	doc->srastral = docN105->srastral;
	doc->altsrastral = docN105->altsrastral;
	doc->tempo = docN105->tempo;
	doc->channel = docN105->channel;
	doc->velocity = docN105->velocity;
	doc->headerStrOffset = docN105->headerStrOffset;
	doc->footerStrOffset = docN105->footerStrOffset;
	doc->topPGN = docN105->topPGN;
	doc->hPosPGN = docN105->hPosPGN;
	doc->alternatePGN = docN105->alternatePGN;
	doc->useHeaderFooter = docN105->useHeaderFooter;
	doc->fillerPGN = docN105->fillerPGN;
	doc->fillerMB = docN105->fillerMB;
	doc->filler2 = docN105->filler2;
	doc->dIndentOther = docN105->dIndentOther;
	doc->firstNames = docN105->firstNames;
	doc->otherNames = docN105->otherNames;
	doc->lastGlobalFont = docN105->lastGlobalFont;
	doc->xMNOffset = docN105->xMNOffset;
	doc->yMNOffset = docN105->yMNOffset;
	doc->xSysMNOffset = docN105->xSysMNOffset;
	doc->aboveMN = docN105->aboveMN;
	doc->sysFirstMN = docN105->sysFirstMN;
	doc->startMNPrint1 = docN105->startMNPrint1;
	doc->firstMNNumber = docN105->firstMNNumber;
	doc->masterHeadL = docN105->masterHeadL;
	doc->masterTailL = docN105->masterTailL;
	doc->filler1 = docN105->filler1;
	doc->nFontRecords = docN105->nFontRecords;
	
	//fontNameMN[32]
	doc->fillerMN = docN105->fillerMN;
	doc->lyricMN = docN105->lyricMN;
	doc->enclosureMN = docN105->enclosureMN;
	doc->relFSizeMN = docN105->relFSizeMN;
	doc->fontSizeMN = docN105->fontSizeMN;
	doc->fontStyleMN = docN105->fontStyleMN;
	
	//fontNamePN[32]
	doc->fillerPN = docN105->fillerPN;
	doc->lyricPN = docN105->lyricPN;
	doc->enclosurePN = docN105->enclosurePN;
	doc->relFSizePN = docN105->relFSizePN;
	doc->fontSizePN = docN105->fontSizePN;
	doc->fontStylePN = docN105->fontStylePN;
	
	//fontNameRM[32]
	doc->fillerRM = docN105->fillerRM;
	doc->lyricRM = docN105->lyricRM;
	doc->enclosureRM = docN105->enclosureRM;
	doc->relFSizeRM = docN105->relFSizeRM;
	doc->fontSizeRM = docN105->fontSizeRM;
	doc->fontStyleRM = docN105->fontStyleRM;
	
	//fontName1[32]
	doc->fillerR1 = docN105->fillerR1;
	doc->lyric1 = docN105->lyric1;
	doc->enclosure1 = docN105->enclosure1;
	doc->relFSize1 = docN105->relFSize1;
	doc->fontSize1 = docN105->fontSize1;
	doc->fontStyle1 = docN105->fontStyle1;
	
	//fontName2[32]
	doc->fillerR2 = docN105->fillerR2;
	doc->lyric2 = docN105->lyric2;
	doc->enclosure2 = docN105->enclosure2;
	doc->relFSize2 = docN105->relFSize2;
	doc->fontSize2 = docN105->fontSize2;
	doc->fontStyle2 = docN105->fontStyle2;
	
	//fontName3[32]
	doc->fillerR3 = docN105->fillerR3;
	doc->lyric3 = docN105->lyric3;
	doc->enclosure3 = docN105->enclosure3;
	doc->relFSize3 = docN105->relFSize3;
	doc->fontSize3 = docN105->fontSize3;
	doc->fontStyle3 = docN105->fontStyle3;
	
	//fontName4[32]
	doc->fillerR4 = docN105->fillerR4;
	doc->lyric4 = docN105->lyric4;
	doc->enclosure4 = docN105->enclosure4;
	doc->relFSize4 = docN105->relFSize4;
	doc->fontSize4 = docN105->fontSize4;
	doc->fontStyle4 = docN105->fontStyle4;
	
	//fontNameTM[32]
	doc->fillerTM = docN105->fillerTM;
	doc->lyricTM = docN105->lyricTM;
	doc->enclosureTM = docN105->enclosureTM;
	doc->relFSizeTM = docN105->relFSizeTM;
	doc->fontSizeTM = docN105->fontSizeTM;
	doc->fontStyleTM = docN105->fontStyleTM;
	
	//fontNameCS[32]
	doc->fillerCS = docN105->fillerCS;
	doc->lyricCS = docN105->lyricCS;
	doc->enclosureCS = docN105->enclosureCS;
	doc->relFSizeCS = docN105->relFSizeCS;
	doc->fontSizeCS = docN105->fontSizeCS;
	doc->fontStyleCS = docN105->fontStyleCS;
	
	//fontNamePG[32]
	doc->fillerPG = docN105->fillerPG;
	doc->lyricPG = docN105->lyricPG;
	doc->enclosurePG = docN105->enclosurePG;
	doc->relFSizePG = docN105->relFSizePG;
	doc->fontSizePG = docN105->fontSizePG;
	doc->fontStylePG = docN105->fontStylePG;
	
	//fontName5[32]
	doc->fillerR5 = docN105->fillerR5;
	doc->lyric5 = docN105->lyric5;
	doc->enclosure5 = docN105->enclosure5;
	doc->relFSize5 = docN105->relFSize5;
	doc->fontSize5 = docN105->fontSize5;
	doc->fontStyle5 = docN105->fontStyle5;
	
	//fontName6[32]
	doc->fillerR6 = docN105->fillerR6;
	doc->lyric6 = docN105->lyric6;
	doc->enclosure6 = docN105->enclosure6;
	doc->relFSize6 = docN105->relFSize6;
	doc->fontSize6 = docN105->fontSize6;
	doc->fontStyle6 = docN105->fontStyle6;
	
	//fontName7[32]
	doc->fillerR7 = docN105->fillerR7;
	doc->lyric7 = docN105->lyric7;
	doc->enclosure7 = docN105->enclosure7;
	doc->relFSize7 = docN105->relFSize7;
	doc->fontSize7 = docN105->fontSize7;
	doc->fontStyle7 = docN105->fontStyle7;
	
	//fontName8[32]
	doc->fillerR8 = docN105->fillerR8;
	doc->lyric8 = docN105->lyric8;
	doc->enclosure8 = docN105->enclosure8;
	doc->relFSize8 = docN105->relFSize8;
	doc->fontSize8 = docN105->fontSize8;
	doc->fontStyle8 = docN105->fontStyle8;
	
	//fontName9[32]
	doc->fillerR9 = docN105->fillerR9;
	doc->lyric9 = docN105->lyric9;
	doc->enclosure9 = docN105->enclosure9;
	doc->relFSize9 = docN105->relFSize9;
	doc->fontSize9 = docN105->fontSize9;
	doc->fontStyle9 = docN105->fontStyle9;
	
	doc->nfontsUsed = docN105->nfontsUsed;
	//fontTable[MAX_SCOREFONTS]
	
	//musFontName[32]
	
	doc->magnify = docN105->magnify;
	doc->selStaff = docN105->selStaff;
	doc->otherMNStaff = docN105->otherMNStaff;
	doc->numberMeas = docN105->numberMeas;
	doc->currentSystem = docN105->currentSystem;
	doc->spaceTable = docN105->spaceTable;
	doc->htight = docN105->htight;
	doc->fillerInt = docN105->fillerInt;
	doc->lookVoice = docN105->lookVoice;
	doc->fillerHP = docN105->fillerHP;
	doc->fillerLP = docN105->fillerLP;
	doc->ledgerYSp = docN105->ledgerYSp;
	doc->deflamTime = docN105->deflamTime;
	
	doc->autoRespace = docN105->autoRespace;
	doc->insertMode = docN105->insertMode;
	doc->beamRests = docN105->beamRests;
	doc->pianoroll = docN105->pianoroll;
	doc->showSyncs = docN105->showSyncs;
	doc->frameSystems = docN105->frameSystems;
	doc->fillerEM = docN105->fillerEM;
	doc->colorVoices = docN105->colorVoices;
	doc->showInvis = docN105->showInvis;
	doc->showDurProb = docN105->showDurProb;
	doc->recordFlats = docN105->recordFlats;
	
	//longspaceMap[MAX_L_DUR]
	doc->dIndentFirst = docN105->dIndentFirst;
	doc->yBetweenSys = docN105->yBetweenSys;
	//voiceTab[MAXVOICES+1]
	//expansion[256-(MAXVOICES+1)]
}


/* --------------------------------------------------------------------- SetTimeStamps -- */
/* Recompute timestamps up to the first Measure that has any unknown durs. in it. */

static void SetTimeStamps(Document *doc)
{
	LINK firstUnknown, stopMeas;
	
	firstUnknown = FindUnknownDur(doc->headL, GO_RIGHT);
	stopMeas = (firstUnknown? SSearch(firstUnknown, MEASUREtype, GO_LEFT)
								:  doc->tailL);
	FixTimeStamps(doc, doc->headL, LeftLINK(stopMeas));
}


/* ----------------------------------------------------------- Utilities for OpenFile  -- */

#define in2d(x)	pt2d(in2pt(x))		/* Convert inches to DDIST */

/* Display some Document header fields; there are about 20 in all. */

static void DisplayDocumentHdr(short id, Document *doc)
{
	LogPrintf(LOG_INFO, "Displaying Document header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  origin.h=%d", doc->origin.h);
	LogPrintf(LOG_INFO, "  .v=%d", doc->origin.h);
	
	LogPrintf(LOG_INFO, "  paperRect.top=%d", doc->paperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->paperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->paperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->paperRect.right);
	LogPrintf(LOG_INFO, "  origPaperRect.top=%d", doc->origPaperRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->origPaperRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->origPaperRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->origPaperRect.right);
	LogPrintf(LOG_INFO, "  marginRect.top=%d", doc->marginRect.top);
	LogPrintf(LOG_INFO, "  .left=%d", doc->marginRect.left);
	LogPrintf(LOG_INFO, "  .bottom=%d", doc->marginRect.bottom);
	LogPrintf(LOG_INFO, "  .right=%d\n", doc->marginRect.right);

	LogPrintf(LOG_INFO, "  numSheets=%d", doc->numSheets);
	LogPrintf(LOG_INFO, "  firstSheet=%d", doc->firstSheet);
	LogPrintf(LOG_INFO, "  firstPageNumber=%d", doc->firstPageNumber);
	LogPrintf(LOG_INFO, "  startPageNumber=%d\n", doc->startPageNumber);
	
	LogPrintf(LOG_INFO, "  numRows=%d", doc->numRows);
	LogPrintf(LOG_INFO, "  numCols=%d", doc->numCols);
	LogPrintf(LOG_INFO, "  pageType=%d", doc->pageType);
	LogPrintf(LOG_INFO, "  measSystem=%d\n", doc->measSystem);	
}

/* Do a reality check for Document header values that might be bad. Return the number of
problems found. NB: We're not checking anywhere near everything we could! */

static short CheckDocumentHdr(Document *doc)
{
	short nerr = 0;
	
	//if (!RectIsValid(doc->paperRect, 4??, in2pt(5)??)) nerr++;
	//if (!RectIsValid(doc->origPaperRect, 4??, in2pt(5)??)) nerr++;
	//if (!RectIsValid(doc->marginRect, 4??, in2pt(5)??)) nerr++;
	if (doc->numSheets<1 || doc->numSheets>250) nerr++;
	if (doc->firstPageNumber<0 || doc->firstPageNumber>250) nerr++;
	if (doc->startPageNumber<0 || doc->startPageNumber>250) nerr++;
	if (doc->numRows < 1 || doc->numRows > 250) nerr++;
	if (doc->numCols < 1 || doc->numCols > 250) nerr++;
	if (doc->pageType < 0 || doc->pageType > 20) nerr++;
	
	return nerr;
}

/* Display some Score header fields, but nowhere near all: there are about 200. */

static void DisplayScoreHdr(short id, Document *doc)
{
	Str255 tempStr;
	
	LogPrintf(LOG_INFO, "Displaying Score header (ID %d):\n", id);
	LogPrintf(LOG_INFO, "  nstaves=%d", doc->nstaves);
	LogPrintf(LOG_INFO, "  nsystems=%d", doc->nsystems);		
	LogPrintf(LOG_INFO, "  spacePercent=%d", doc->spacePercent);
	LogPrintf(LOG_INFO, "  srastral=%d", doc->srastral);				
	LogPrintf(LOG_INFO, "  altsrastral=%d\n", doc->altsrastral);
		
	LogPrintf(LOG_INFO, "  tempo=%d", doc->tempo);		
	LogPrintf(LOG_INFO, "  channel=%d", doc->channel);			
	LogPrintf(LOG_INFO, "  velocity=%d", doc->velocity);		
	LogPrintf(LOG_INFO, "  headerStrOffset=%d", doc->headerStrOffset);	
	LogPrintf(LOG_INFO, "  footerStrOffset=%d", doc->footerStrOffset);	
	LogPrintf(LOG_INFO, "  dIndentOther=%d\n", doc->dIndentOther);
	
	LogPrintf(LOG_INFO, "  firstNames=%d", doc->firstNames);
	LogPrintf(LOG_INFO, "  otherNames=%d", doc->otherNames);
	LogPrintf(LOG_INFO, "  lastGlobalFont=%d\n", doc->lastGlobalFont);

	LogPrintf(LOG_INFO, "  aboveMN=%c", (doc->aboveMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  sysFirstMN=%c", (doc->sysFirstMN? '1' : '0'));
	LogPrintf(LOG_INFO, "  startMNPrint1=%c", (doc->startMNPrint1? '1' : '0'));
	LogPrintf(LOG_INFO, "  firstMNNumber=%d\n", doc->firstMNNumber);

	LogPrintf(LOG_INFO, "  nfontsUsed=%d", doc->nfontsUsed);
	Pstrcpy(tempStr, doc->musFontName); PToCString(tempStr);
	LogPrintf(LOG_INFO, "  musFontName='%s'\n", tempStr);
	
	LogPrintf(LOG_INFO, "  magnify=%d", doc->magnify);
	LogPrintf(LOG_INFO, "  selStaff=%d", doc->selStaff);
	LogPrintf(LOG_INFO, "  currentSystem=%d", doc->currentSystem);
	LogPrintf(LOG_INFO, "  spaceTable=%d", doc->spaceTable);
	LogPrintf(LOG_INFO, "  htight=%d\n", doc->htight);
	
	LogPrintf(LOG_INFO, "  lookVoice=%d", doc->lookVoice);
	LogPrintf(LOG_INFO, "  ledgerYSp=%d", doc->ledgerYSp);
	LogPrintf(LOG_INFO, "  deflamTime=%d", doc->deflamTime);
	LogPrintf(LOG_INFO, "  colorVoices=%d", doc->colorVoices);
	LogPrintf(LOG_INFO, "  dIndentFirst=%d\n", doc->dIndentFirst);
}

/* Do a reality check for Score header values that might be bad. Return the number of
problems found. NB: We're not checking anywhere near everything we could! */

static short CheckScoreHdr(Document *doc)
{
	short nerr = 0;
	
	if (doc->nstaves<1 || doc->nstaves>MAXSTAVES) nerr++;
	if (doc->nsystems<1 || doc->nsystems>2000) nerr++;
	if (doc->spacePercent<MINSPACE || doc->spacePercent>MAXSPACE) nerr++;
	if (doc->srastral<0 || doc->srastral>MAXRASTRAL) nerr++;
	if (doc->altsrastral<1 || doc->altsrastral>MAXRASTRAL) nerr++;
	if (doc->tempo<MIN_BPM || doc->tempo>MAX_BPM) nerr++;
	if (doc->channel<1 || doc->channel>MAXCHANNEL) nerr++;
	if (doc->velocity<-127 || doc->velocity>127) nerr++;

	//if (doc->headerStrOffset<?? || doc->headerStrOffset>??) nerr++;
	//if (doc->footerStrOffset<?? || doc->footerStrOffset>??) nerr++;
	
	if (doc->dIndentOther<0 || doc->dIndentOther>in2d(5)) nerr++;
	if (doc->firstNames<0 || doc->firstNames>MAX_NAMES_TYPE) nerr++;
	if (doc->otherNames<0 || doc->otherNames>MAX_NAMES_TYPE) nerr++;
	if (doc->lastGlobalFont<FONT_THISITEMONLY || doc->lastGlobalFont>MAX_FONTSTYLENUM) nerr++;
	if (doc->firstMNNumber<0 || doc->firstMNNumber>MAX_FIRSTMEASNUM) nerr++;
	if (doc->nfontsUsed<1 || doc->nfontsUsed>MAX_SCOREFONTS) nerr++;
	if (doc->magnify<MIN_MAGNIFY || doc->magnify>MAX_MAGNIFY) nerr++;
	if (doc->selStaff<-1 || doc->selStaff>doc->nstaves) nerr++;
	if (doc->currentSystem<1 || doc->currentSystem>doc->nsystems) nerr++;
	if (doc->spaceTable<0 || doc->spaceTable>32767) nerr++;
	if (doc->htight<MINSPACE || doc->htight>MAXSPACE) nerr++;
	if (doc->lookVoice<-1 || doc->lookVoice>MAXVOICES) nerr++;
	if (doc->ledgerYSp<0 || doc->ledgerYSp>40) nerr++;
	if (doc->deflamTime<1 || doc->deflamTime>1000) nerr++;
	if (doc->dIndentFirst<0 || doc->dIndentFirst>in2d(5)) nerr++;
	
	return nerr;
}


/* -------------------------------------------------------------------------- OpenFile -- */
/* Open and read in the specified file. If there's an error, normally (see comments in
OpenError) gives an error message, and returns <errCode>; else returns noErr (0). Also
sets *fileVersion to the Nightingale version that created the file. FIXME: even though
vRefNum is a parameter, (routines called by) OpenFile assume the volume is already set!
This should be changed. */

enum {
	LOW_VERSION_ERR=-999,
	HI_VERSION_ERR,
	HEADER_ERR,
	LASTTYPE_ERR,				/* Value for LASTtype in file is not we expect it to be */
	TOOMANYSTAVES_ERR,
	LOWMEM_ERR,
	BAD_VERSION_ERR
};

extern void StringPoolEndianFix(StringPoolRef pool);
extern short StringPoolProblem(StringPoolRef pool);

#include <ctype.h>

short OpenFile(Document *doc, unsigned char *filename, short vRefNum, FSSpec *pfsSpec,
																	long *fileVersion)
{
	short		errCode, refNum, strPoolErrCode;
	short 		errInfo=0,				/* Type of object being read or other info on error */
				lastType;
	long		count, stringPoolSize,
				fileTime;
	Boolean		fileIsOpen;
	OMSSignature omsDevHdr;
	long		fmsDevHdr;
	long		omsBufCount, omsDevSize;
	short		i, nDocErr, nScoreErr;
	FInfo		fInfo;
	FSSpec 		fsSpec;
	long		cmHdr, cmBufCount, cmDevSize;
	FSSpec		*pfsSpecMidiMap;
	char		versionCString[5];
	DocumentN105 docN105;

	WaitCursor();

	fileIsOpen = False;
	
	fsSpec = *pfsSpec;
	errCode = FSpOpenDF(&fsSpec, fsRdWrPerm, &refNum );			/* Open the file */
	if (errCode == fLckdErr || errCode == permErr) {
		doc->readOnly = True;
		errCode = FSpOpenDF (&fsSpec, fsRdPerm, &refNum );		/* Try again - open the file read-only */
	}
	if (errCode) { errInfo = OPENcall; goto Error; }
	fileIsOpen = True;

	count = sizeof(version);									/* Read & check file version code */
	errCode = FSRead(refNum, &count, &version);
	FIX_END(version);
	if (errCode) { errInfo = VERSIONobj; goto Error;}
	*fileVersion = version;
	MacTypeToString(version, versionCString);
	
	/* If user has the secret keys down, pretend file is in current version. */
	
	if (ShiftKeyDown() && OptionKeyDown() && CmdKeyDown() && ControlKeyDown()) {
		LogPrintf(LOG_NOTICE, "IGNORING FILE'S VERSION CODE '%s'.\n", versionCString);
		GetIndCString(strBuf, FILEIO_STRS, 6);					/* "IGNORING FILE'S VERSION CODE!" */
		CParamText(strBuf, "", "", "");
		CautionInform(GENERIC_ALRT);
		version = THIS_FILE_VERSION;
	}

	if (versionCString[0]!='N') { errCode = BAD_VERSION_ERR; goto Error; }
	if ( !isdigit(versionCString[1])
			|| !isdigit(versionCString[2])
			|| !isdigit(versionCString[3]) )
		 { errCode = BAD_VERSION_ERR; goto Error; }
	if (version<FIRST_FILE_VERSION) { errCode = LOW_VERSION_ERR; goto Error; }
	if (version>THIS_FILE_VERSION) { errCode = HI_VERSION_ERR; goto Error; }
	if (version!=THIS_FILE_VERSION) LogPrintf(LOG_NOTICE, "CONVERTING VERSION '%s' FILE.\n", versionCString);

	count = sizeof(fileTime);									/* Time file was written */
	errCode = FSRead(refNum, &count, &fileTime);
	FIX_END(fileTime);
	if (errCode) { errInfo = VERSIONobj; goto Error; }
		
	/* Read and, if necessary, convert Document (i.e., Sheets) and Score headers. */
	
	count = sizeof(DOCUMENTHDR);
	errCode = FSRead(refNum, &count, &doc->origin);
	if (errCode) { errInfo = HEADERobj; goto Error; }
	EndianFixDocumentHdr(doc);
	if (DETAIL_SHOW) DisplayDocumentHdr(1, doc);
	LogPrintf(LOG_NOTICE, "Checking Document header: ");
	nDocErr = CheckDocumentHdr(doc);
	if (nDocErr==0)
		LogPrintf(LOG_NOTICE, "No errors found.  (OpenFile)\n");
	else {
		if (!DETAIL_SHOW) DisplayDocumentHdr(2, doc);
		sprintf(strBuf, "%d error(s) found in Document header.", nDocErr);
		CParamText(strBuf, "", "", "");
		LogPrintf(LOG_ERR, "%d error(s) found in Document header.  (OpenFile)\n", nDocErr);
		goto HeaderError;
	}
	
	switch(version) {
		case 'N103':
		case 'N104':
		case 'N105':
			count = sizeof(SCOREHEADER_N105);
			errCode = FSRead(refNum, &count, &docN105.headL);
			if (errCode) { errInfo = HEADERobj; goto Error; }
			ConvertScoreHeader(doc, &docN105);
			break;
		
		default:											/* Must be 'N106', the current format */
			count = sizeof(SCOREHEADER);
			errCode = FSRead(refNum, &count, &doc->headL);
			if (errCode) { errInfo = HEADERobj; goto Error; }
	}

	
//DisplayScoreHdr(2, doc);		// ??TEMPORARY, TO DEBUG INTEL VERSION & FILE FORMAT CONVERSION!!!!
	EndianFixScoreHdr(doc);
	if (DETAIL_SHOW) DisplayScoreHdr(3, doc);
	LogPrintf(LOG_NOTICE, "Checking Score header: ");
	nScoreErr = CheckScoreHdr(doc);
	if (nScoreErr==0)
		LogPrintf(LOG_NOTICE, "No errors found in Score header.  (OpenFile)\n");
	else {
		if (!DETAIL_SHOW) DisplayScoreHdr(4, doc);
		sprintf(strBuf, "%d error(s) found in Score header.", nScoreErr);
		CParamText(strBuf, "", "", "");
		LogPrintf(LOG_ERR, "%d error(s) found in Score header.  (OpenFile)\n", nScoreErr);
		StopInform(GENERIC_ALRT);
		errCode = HEADER_ERR;
		errInfo = 0;
		goto Error;
	}

	count = sizeof(lastType);
	errCode = FSRead(refNum, &count, &lastType);
	if (errCode) { errInfo = HEADERobj; goto Error; }
	FIX_END(lastType);

	if (lastType!=LASTtype) {
		errCode = LASTTYPE_ERR;
		errInfo = HEADERobj;
		goto Error;
	}

	count = sizeof(stringPoolSize);
	errCode = FSRead(refNum, &count, &stringPoolSize);
	if (errCode) { errInfo = STRINGobj; goto Error; }
	FIX_END(stringPoolSize);
	if (doc->stringPool) DisposeStringPool(doc->stringPool);
	
	/* Allocate from the StringManager, not NewHandle, in case StringManager is tracking
	   its allocations. Then, since we're going to overwrite everything with stuff from
	   file below, we can resize it to what it was when saved. */
	
	doc->stringPool = NewStringPool();
	if (doc->stringPool == NULL) { errInfo = STRINGobj; goto Error; }
	SetHandleSize((Handle)doc->stringPool,stringPoolSize);
	
	HLock((Handle)doc->stringPool);
	errCode = FSRead(refNum, &stringPoolSize, *doc->stringPool);
	if (errCode) { errInfo = STRINGobj; goto Error; }
	HUnlock((Handle)doc->stringPool);
	SetStringPool(doc->stringPool);
	StringPoolEndianFix(doc->stringPool);
	strPoolErrCode = StringPoolProblem(doc->stringPool);
	if (strPoolErrCode!=0)
		AlwaysErrMsg("The string pool is probably bad (code=%ld).  (OpenFile)", (long)strPoolErrCode);
	
	//errCode = GetFInfo(filename, vRefNum, &fInfo);
	errCode = FSpGetFInfo (&fsSpec, &fInfo);
	if (errCode!=noErr) { errInfo = INFOcall; goto Error; }
	
	errCode = ReadHeaps(doc, refNum, version, fInfo.fdType);	/* Read the rest of the file! */
	if (errCode) return errCode;

	/* Be sure we have enough memory left for a maximum-size segment and a bit more. */
	
	if (!PreflightMem(40)) { NoMoreMemory(); return LOWMEM_ERR; }
	
	ConvertScoreContent(doc, fileTime);			/* Do any conversion of old files needed */

	Pstrcpy(doc->name, filename);				/* Remember filename and vol refnum after scoreHead is overwritten */
	doc->vrefnum = vRefNum;
	doc->changed = False;
	doc->saved = True;							/* Has to be, since we just opened it! */
	doc->named = True;							/* Has to have a name, since we just opened it! */

	ModifyScore(doc, fileTime);					/* Do any hacking needed, ordinarily none */

	/* Read the document's OMS device numbers for each part. if "devc" signature block not
	   found at end of file, pack the document's omsPartDeviceList with default values. */
	
	omsBufCount = sizeof(OMSSignature);
	errCode = FSRead(refNum, &omsBufCount, &omsDevHdr);
	if (!errCode && omsDevHdr == 'devc') {
		omsBufCount = sizeof(long);
		errCode = FSRead(refNum, &omsBufCount, &omsDevSize);
		if (errCode) return errCode;
		if (omsDevSize!=(MAXSTAVES+1)*sizeof(OMSUniqueID)) return errCode;
		errCode = FSRead(refNum, &omsDevSize, &(doc->omsPartDeviceList[0]));
		if (errCode) return errCode;
	}
	else {
		for (i = 1; i<=LinkNENTRIES(doc->headL)-1; i++)
			doc->omsPartDeviceList[i] = config.defaultOutputDevice;
		doc->omsInputDevice = config.defaultInputDevice;
	}

	/* Read the FreeMIDI input device data. */
	doc->fmsInputDevice = noUniqueID;
	/* FIXME: We're probably not supposed to play with these fields... */
	doc->fmsInputDestination.basic.destinationType = 0,
	doc->fmsInputDestination.basic.name[0] = 0;
	count = sizeof(long);
	errCode = FSRead(refNum, &count, &fmsDevHdr);
	if (!errCode) {
		if (fmsDevHdr==0x00000000)
			;					/* end marker, so file version is < N103, and we're done */
		else if (fmsDevHdr==FreeMIDISelector) {
			count = sizeof(fmsUniqueID);
			errCode = FSRead(refNum, &count, &doc->fmsInputDevice);
			if (errCode) return errCode;
			count = sizeof(destinationMatch);
			errCode = FSRead(refNum, &count, &doc->fmsInputDestination);
			if (errCode) return errCode;
		}
	}
	
	if (version >= 'N105') {
		cmBufCount = sizeof(long);
		errCode = FSRead(refNum, &cmBufCount, &cmHdr);
		if (!errCode && cmHdr == 'cmdi') {
			cmBufCount = sizeof(long);
			errCode = FSRead(refNum, &cmBufCount, &cmDevSize);
			if (errCode) return errCode;
			if (cmDevSize!=(MAXSTAVES+1)*sizeof(MIDIUniqueID)) return errCode;
			errCode = FSRead(refNum, &cmDevSize, &(doc->cmPartDeviceList[0]));
			if (errCode) return errCode;
		}
	}
	else {
		for (i = 1; i<=LinkNENTRIES(doc->headL)-1; i++)
			doc->cmPartDeviceList[i] = config.cmDfltOutputDev;
	}
	doc->cmInputDevice = config.cmDfltInputDev;

	errCode = FSClose(refNum);
	if (errCode) { errInfo = CLOSEcall; goto Error; }
	
	if (!IsDocPrintInfoInstalled(doc))
		InstallDocPrintInfo(doc);
	
	GetPrintHandle(doc, version, vRefNum, pfsSpec);		/* doc->name must be set before this */
	
	GetMidiMap(doc, pfsSpec);
	
	pfsSpecMidiMap = GetMidiMapFSSpec(doc);
	if (pfsSpecMidiMap != NULL) {
		OpenMidiMapFile(doc, pfsSpecMidiMap);
		ReleaseMidiMapFSSpec(doc);		
	}

	FillFontTable(doc);

	InitDocMusicFont(doc);

	SetTimeStamps(doc);									/* Up to first meas. w/unknown durs. */


	/* Assume that no information in the score having to do with window-relative
	   positions is valid. Besides clearing the object <valid> flags to indicate this,
	   protect ourselves against functions that might not check the flags properly by
	   setting all such positions to safe values now. */
	
	InvalRange(doc->headL, doc->tailL);					/* Force computing all objRects */

	doc->hasCaret = False;									/* Caret must still be set up */
	doc->selStartL = doc->headL;							/* Set selection range */								
	doc->selEndL = doc->tailL;								/*   to entire score, */
	DeselAllNoHilite(doc);									/*   deselect it */
	SetTempFlags(doc, doc, doc->headL, doc->tailL, False);	/* Just in case */

	if (ScreenPagesExceedView(doc))
		CautionInform(MANYPAGES_ALRT);

	ArrowCursor();	
	return 0;

HeaderError:
	sprintf(strBuf, "%d error(s) found in Document header.", nDocErr);
	CParamText(strBuf, "", "", "");
	LogPrintf(LOG_ERR, "%d error(s) found in Document header.  (OpenFile)\n", nDocErr);
	StopInform(GENERIC_ALRT);
	errCode = HEADER_ERR;
	errInfo = 0;
	/* drop through */
Error:
	OpenError(fileIsOpen, refNum, errCode, errInfo);
	return errCode;
}


/* ------------------------------------------------------------------------- OpenError -- */
/* Handle errors occurring while reading a file. <fileIsOpen> indicates  whether or
not the file was open when the error occurred; if so, OpenError closes it. <errCode>
is either an error code return by the file system manager or one of our own codes
(see enum above). <errInfo> indicates at what step of the process the error happened
(CREATEcall, OPENcall, etc.: see enum above), the type of object being read, or
some other additional information on the error. NB: If errCode==0, this will close
the file but not give an error message; I'm not sure that's a good idea.

Note that after a call to OpenError with fileIsOpen, you should not try to keep reading,
since the file will no longer be open! */

void OpenError(Boolean fileIsOpen,
					short refNum,
					short errCode,
					short errInfo)	/* More info: at what step error happened, type of obj being read, etc. */
{
	char aStr[256], fmtStr[256];
	StringHandle strHdl;
	short strNum;
	
	SysBeep(1);
	LogPrintf(LOG_ERR, "Can't open the file. errCode=%d errInfo=%d  (OpenError)\n", errCode, errInfo);
	if (fileIsOpen) FSClose(refNum);

	if (errCode!=0) {
		switch (errCode) {
			/*
			 * First handle our own codes that need special treatment.
			 */
			case BAD_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 7);			/* "file version is illegal" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case LOW_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 8);			/* "too old for this version of Nightingale" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case HI_VERSION_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 9);			/* "newer than this version of Nightingale" */
				sprintf(aStr, fmtStr, ACHAR(version,3), ACHAR(version,2),
							 ACHAR(version,1), ACHAR(version,0));
				break;
			case TOOMANYSTAVES_ERR:
				GetIndCString(fmtStr, FILEIO_STRS, 10);		/* "this version can handle only %d staves" */
				sprintf(aStr, fmtStr, errInfo, MAXSTAVES);
				break;
			default:
				/*
				 * We expect descriptions of the common errors stored by code (negative
				 * values, for system errors; positive ones for our own I/O errors) in
				 * individual 'STR ' resources. If we find one for this error, print it,
				 * else just print the raw code.
				 */
				strHdl = GetString(errCode);
				if (strHdl) {
					Pstrcpy((unsigned char *)strBuf, (unsigned char *)*strHdl);
					PToCString((unsigned char *)strBuf);
					strNum = (errInfo>0? 15 : 16);	/* "%s (heap object type=%d)." : "%s (error code=%d)." */
					GetIndCString(fmtStr, FILEIO_STRS, strNum);
					sprintf(aStr, fmtStr, strBuf, errInfo);
				}
				else {
					strNum = (errInfo>0? 17 : 18);	/* "Error ID=%d (heap object type=%d)." : "Error ID=%d (error code=%d)." */
					GetIndCString(fmtStr, FILEIO_STRS, strNum);
					sprintf(aStr, fmtStr, errCode, errInfo);
				}
		}
		LogPrintf(LOG_WARNING, aStr); LogPrintf(LOG_WARNING, "\n");
		CParamText(aStr, "", "", "");
		StopInform(READ_ALRT);
	}
}
