/***************************************************************************
	FILE:	NewSlur.c
	PROJ:	Nightingale
	DESC:	Slur creating routines.
	
		HandleFirstSync
		NewTrackSlur			CheckCrossness			HandleLastSync
		SwapEndpoints			NestingIsOK				FillTieArrays
		WantTies				HandleTie				NewSlurOrTie
		AddNewSlur				SetSlurIndices			Tie2NoteLINK
		GetTiesCurveDir			GetSlurTieCurveDir		NewSlurSetCtlPts
		CrossSysSetCtlPts		NewSlurCleanup			NewCrossSystemSlur
		SlurToNext				NewSlur					AddNoteTies
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
		
static char HandleFirstSync(Document *, short staff, short voice);
static Point NewTrackSlur(Document *, Rect *paper, Point pt, Rect *newPaper);
static short GetSyncStaff(LINK, short);
static void CheckCrossness(Document *, short staff, Point pt);
static LINK ValidSlurEndpt(Document *, Point, short);
static char HandleLastSync(Document *, short staff, short voice, Point pt);
static void SwapEndpoints(Document *, short *staff);
static Boolean NestingIsOK(LINK, LINK, short, short, Boolean, Boolean, Boolean *);
static short FillTieArrays(LINK,LINK,short,CHORDNOTE [],CHORDNOTE [],short,
									char [],char []);
static Boolean WantTies(Boolean, Boolean);
static short HandleTie(LINK, LINK, short, short *, char [], char []);
static char NewSlurOrTie(short staff, short voice);
static LINK AddNewSlur(Document *, LINK insertL, short staff, short voice);
static void ClearSlurIndices(LINK newL);
static char SetSlurIndices(LINK newL);
static LINK Tie2NoteLINK(LINK, short, LINK);
static void NewSlurSetCtlPts(Document *doc, short staff, short voice, CONTEXT context);
static void CrossSysSetCtlPts(Document *, short staff, short voice, LINK newL, CONTEXT context);
static void NewSlurCleanup(Document *doc, short staff, LINK newL, Boolean user);
static void NewCrossSystemSlur(Document *, short staff, short voice, CONTEXT context);
static Boolean SlurToNext(Document *, short *, short);
static short BuildTieArrays(LINK, LINK, short, char [], char[]);

/* --------------------------------------------------- NewSlur and Help Functions -- */
/* Functions for NewSlur. Functions called by NewSlur return one of these codes to
indicate whether they were successful, or if not, the reason why. */

#define PROG_ERR 	-2		/* Code for program error */
#define USR_ALERT	-1		/* Code for user alert */
#define F_ABORT		0		/* Code for function abort */
#define NF_OK		1		/* Code for successful completion */
#define F_SLUR		2
#define F_TIE		3
#define BAD_CODE(returnCode) ((returnCode)<=0) 

static LINK 	firstSyncL, lastSyncL;
static LINK 	firstNoteL;
static LINK 	slurL;
static short 	endStaff;
static short 	oldSystem;
static short 	subCount;
static DPoint 	kn0Pt, kn1Pt;
static DPoint 	c0, c1;
static Boolean	crossSystem, crossSysBack, crossStaff, crossStfBack;
static Boolean	itsATie, mustBeTie, slurToNext;
static char 	firstIndA[MAXCHORD], lastIndA[MAXCHORD];
static char 	returnCode;
static Point	startPt[MAXCHORD], endPt[MAXCHORD];


/* Look for the first sync endpt of the slur; if found, hilite it. */

static char HandleFirstSync(Document *doc, short staff, short voice)
{
	LINK aNoteL; PANOTE aNote;
	Boolean noteFound=FALSE;
	
	if (doc->selStartL==doc->headL) {
		MayErrMsg("HandleFirstSync: ASKED TO INSERT BEFORE HEADER");
		return PROG_ERR;
	}
	
	if (!SyncTYPE(LeftLINK(doc->selStartL))) return F_ABORT;

	firstSyncL = LeftLINK(doc->selStartL);
	aNoteL = firstNoteL = FirstSubLINK(firstSyncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->staffn==staff && aNote->voice==voice)
			{ noteFound = TRUE; break; }
	}
	if (!noteFound) return F_ABORT;						/* No note on this staff & voice? */

	HiliteInsertNode(doc, firstSyncL, staff, TRUE);
	return NF_OK;
}


/* Track the slur and set associated control and anchor points.  Returns in
window-relative pixels the endpoint (knot) of the tracked slur. It also returns
in newPaper the paperRect that the point falls within in case it is different
from the initial paper. */

static Point NewTrackSlur(Document *doc, Rect *paper, Point pt, Rect *newPaper)
{
	Boolean curveUp, good;
	DPoint  tempPt;
	Point   localPoint, globalPoint;
	PANOTE  firstNote;
	short	sheetNum;

	/*
	 *	Decide whether the slur should curve up or down based on 1st note stem dir.
	 *	(The following test works even for stemless notes, since SetupNote still
	 *	sets the stem endpoint.  ??YES, BUT UP/DOWN DECISION SHOULD CONSIDER ALL
	 *	NOTES IN THE SLUR; UNFORTUNATELY, WE DON'T YET KNOW HOW FAR THE SLUR WILL GO
	 *	OR EVEN IN WHICH DIRECTION!  PROBABLY BETTER TO LET USER'S GESTURE DECIDE
	 *	FEEDBACK CURAVTURE, THEN CORRECT IT.) Then let the user "draw" the slur.
	 */
 	firstNote = GetPANOTE(firstNoteL);
	curveUp = (firstNote->ystem > firstNote->yd);
	SetDPt(&tempPt,p2d(pt.h), p2d(pt.v));

	good = TrackSlur(paper, tempPt, &kn0Pt, &kn1Pt, &c0, &c1, curveUp);
	SetPt(&localPoint, d2p(kn1Pt.h), d2p(kn1Pt.v));

	/* Convert localPoint to window coordinates and check if localPoint is
		outside the bounds of paper.  If it is, then find the page that the
		user has dragged the endpoint to, if any. */

	globalPoint = localPoint;
	globalPoint.h += paper->left;
	globalPoint.v += paper->top;
	
	*newPaper = *paper;
	if (FindSheet(doc, globalPoint, &sheetNum))
		GetSheetRect(doc, sheetNum, newPaper);
	
	/* At this point, if newPaper != paper, caller knows slur ends on another page */
	return globalPoint;
}


static short GetSyncStaff(LINK pL, short index)
{
	LINK aNoteL; short i;

	if (!SyncTYPE(pL)) return NOONE;

	aNoteL = FirstSubLINK(pL);
	for (i = 0; aNoteL; i++, aNoteL = NextNOTEL(aNoteL))
		if (i==index) return NoteSTAFF(aNoteL);
	return NOONE;
}


/* Check whether the slur ended on the same system or staff as it began. If the
system was different, it's a cross-system slur, so set crossSystem TRUE. If the
staff was different, it's a cross-staff slur, so set crossStaff TRUE.

#1. Get staff containing mouseClick. Need to call FindObject in case the note
terminating slur-tracking operation is so high or low that it overlaps a staff
which it is not on, with the result that FindStaff will find the staff that the
note is not on. However, slur tracking routines eventually call ValidSlurEndpt
to get a valid note at the end of the slur-tracking operation, and ValidSlurEndpt
uses different logic from FindObject, so it may be possible for FindObject to miss
a sync that ValidSlurEndpt will accept. Therefore, if FindObject actually finds
the endnote of the tracking operation, use that note's staff here; otherwise, the
best that can be done is to use the results of FindStaff. */


static void CheckCrossness(Document *doc, short	staff, Point pt)
{
	short objEndStaff=NOONE,index;
	LINK pL;

	oldSystem = doc->currentSystem;

	endStaff = FindStaff(doc, pt);							/* #1. */
	pL = FindObject(doc, pt, &index, SMFind);
	if (pL) objEndStaff = GetSyncStaff(pL, index);

	if (objEndStaff!=NOONE)
		if (endStaff!=objEndStaff)
			endStaff = objEndStaff;

	if (doc->currentSystem!=oldSystem)	{					/* Does it cross systems? */
		crossSysBack = (doc->currentSystem<oldSystem);
		crossSystem = TRUE;
	}
	else
		crossSystem = FALSE;

	if (endStaff!=staff) {									/* Does it cross staves? */
		crossStfBack = (endStaff<staff);
		crossStaff = TRUE;
	}
	else
		crossStaff = FALSE;
}


static LINK ValidSlurEndpt(Document *doc, Point pt, short staffn)
{
	PSYSTEM	pSystem;
	LINK 	rightL=NILINK, leftL=NILINK, symRightL, symLeftL,
			systemL, syncL=NILINK;
	DDIST	sysLeft, 					/* left margin (in DDISTs) of current system */
			insertxd,					/* xd of insertion pt */
			lDist=-1,rDist=-1;			/* DDIST dist from ins. pt to rightL, leftL */	
	
	/* Get valid leftL, rightL, if possible: closest note on the staff, before any
	 * other non-J_D object. 
	 */
	symRightL = StaffFindSymRight(doc, pt, FALSE, staffn);
	if (symRightL) 
		rightL = StaffFindSyncRight(doc, pt, FALSE, staffn);
	if (rightL)
		symRightL = FindValidSymRight(doc, symRightL, staffn);

	symLeftL = StaffFindSymLeft(doc, pt, FALSE, staffn);
	if (symLeftL)
		leftL = StaffFindSyncLeft(doc, pt, FALSE, staffn);
	if (leftL)
		symLeftL = FindValidSymLeft(doc, symLeftL, staffn);
	
	if (rightL && symRightL)
		if (IsAfter(symRightL, rightL)) rightL = NILINK;
	if (leftL && symLeftL)
		if (IsAfter(leftL, symLeftL)) leftL = NILINK;
	
	/* We've got our valid syncs leftL, rightL; find which is closer. */
	
	systemL = LSSearch(doc->headL, SYSTEMtype, doc->currentSystem, GO_RIGHT, FALSE);
	pSystem = GetPSYSTEM(systemL); 
	sysLeft = pSystem->systemRect.left;
	insertxd = Sysxd(pt, sysLeft);
	if (rightL)
		rDist = ABS(insertxd-SysRelxd(rightL));
	if (leftL)
		lDist = ABS(insertxd-SysRelxd(leftL));

	if (lDist>0 && rDist>0)
		syncL = (lDist<=rDist) ? leftL : rightL;
	else if (lDist>0) syncL = leftL;
	else if (rDist>0) syncL = rightL;
	
	return syncL;
}
	

/* Call ValidSlurEndpt to find the last sync of the slur; check for a valid sync,
and then hilite it. */

static char HandleLastSync(Document *doc, short staff, short voice, Point pt)
{	
	lastSyncL = ValidSlurEndpt(doc, pt, endStaff);
	if (!lastSyncL || !SyncInVoice(lastSyncL, voice)) {									
		GetIndCString(strBuf, SLURERRS_STRS, 1);    /* "The slur or tie doesn't have valid end note." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		InvalMeasure(doc->selStartL, staff);
		return USR_ALERT;
	}
	if (lastSyncL==firstSyncL) {
		GetIndCString(strBuf, SLURERRS_STRS, 2);    /* "The slur or tie can't begin and end on the same note." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		InvalMeasure(doc->selStartL, staff);
		return USR_ALERT;
	}
	HiliteInsertNode(doc, lastSyncL, staff, TRUE);
	SleepTicks(HILITE_TICKS);
	return NF_OK;
}


/* If the slur was drawn "backwards"—for a cross-system slur, bottom to top, for
a same-system one, right to left—swap endpts so that the slur's first sync is
before the last. If a cross-staff slur goes from a lower to an upper staff, swap
start and end staff numbers. */

static void SwapEndpoints(Document *doc, short *staff)
{
	DPoint tempPt; LINK tempL; short tempStf; 
	
	if ((!crossSystem && (kn1Pt.h<kn0Pt.h)) ||		/* Drawn right to left on same system? */
		(crossSystem && crossSysBack)) {			/* Drawn bottom to top across systems? */
		tempPt = kn0Pt;								/* Yes, swap endpoints */
		kn0Pt = kn1Pt;
		kn1Pt = tempPt;
		tempPt = c0;
		c0 = c1;
		c1 = tempPt;
		tempL = firstSyncL;
		firstSyncL = lastSyncL;
		lastSyncL = tempL;
		if (!crossSystem && crossStaff) {
			crossStfBack = !crossStfBack;
			if (endStaff < *staff) {
				tempStf = endStaff;
				endStaff = *staff;
				*staff = tempStf;
			}
		}
		doc->selStartL = RightLINK(firstSyncL);
	}
	else if (crossStaff && crossStfBack) {
		tempStf = endStaff;							/* Swap staff numbers */
		endStaff = *staff;
		*staff = tempStf;
	}
}


/* ----------------------------------------------------------------- NestingIsOK -- */
/* NestingIsOK checks whether a slur/tie to be added would be nested in or over-
lapping an existing slur or tie; it should be called before making any changes
to the data structure such as actually inserting the slur object. It returns
TRUE if the putative slur/tie is legal, FALSE if not. If the slur/tie is legal,
it also sets *mustBeTie. NestingIsOK looks for existing slurs that might overlap
the new slur/tie's range, and: 
1. For slurs with first sync strictly between newFirstL and newLastL:
	(a) if their last sync is after newLastL, there's overlap and the new one is
	illegal; or (b) if their last sync is equal to or before newLastL, the new one is
	nested around this one and can only be a slur, and that only if this one is a tie.
2. For slurs with first sync equal to newFirstL:
	(a) if their last is after newLastL, the new one must be a tie; or (b) if their
	last equals newLastL, the ranges are identical: we COULD allow slur or tie here
	depending on the existing one, but it's potentially confusing and probably a user
	mistake anyway; or (c) if their last sync is before newLastL, the new one is
	nested around this one and can only be a slur, and that only if this one is a tie.
3. For slurs with first sync before newFirstL:
	(a) if their last sync is (strictly) between newFirstL and newLastL, there's
	overlap and the new slur/tie is illegal; or (b) if their last sync is equal to or
	after newLastL, the new one is nested within and can only be a tie.
*/

enum {
	C1_THEN_2=-1,
	CEQUAL=0,
	C2_THEN_1=+1
};

static short ComparePosInSys(LINK, LINK);
static void CheckProposedSlur(LINK, LINK, LINK, Boolean *, Boolean *);

static short ComparePosInSys(LINK obj1, LINK obj2)
{
	LINK pL;
	
	if (obj1==obj2) return CEQUAL;
	
	for (pL = obj1; pL && !SystemTYPE(pL); pL = RightLINK(pL))
		if (pL==obj2) return C1_THEN_2;
	
	for (pL = obj2; pL && !SystemTYPE(pL); pL = RightLINK(pL))
		if (pL==obj1) return C2_THEN_1;
	
	return ERROR_INT;
}


static void CheckProposedSlur(
				LINK newFirstL, LINK newLastL,	/* firstSyncL, lastSyncL of proposed slur/tie */
				LINK oldSlurL,					/* existing Slur object to check */
				Boolean *pCanTie,
				Boolean *pCanSlur
				)
{
	LINK oldFirstL, oldLastL; short firstPos, lastPos;

	oldFirstL = SlurFIRSTSYNC(oldSlurL);
	oldLastL = SlurLASTSYNC(oldSlurL);

	firstPos = ComparePosInSys(newFirstL, oldFirstL);
	lastPos = ComparePosInSys(newLastL, oldLastL);

	*pCanTie = TRUE; *pCanSlur = TRUE;
	switch (firstPos) {
		case C1_THEN_2:
			if (IsAfterIncl(newLastL, oldFirstL)) break;	/* no pblm if they're disjoint */
			switch (lastPos) {
				case C1_THEN_2:									/* case 1a */
					*pCanTie = *pCanSlur = FALSE;
					break;
				case CEQUAL:									/* case 1b */
				case C2_THEN_1:
					*pCanTie = FALSE;
					if (!SlurTIE(oldSlurL)) *pCanSlur = FALSE;
					break;
			}
			break;
		case CEQUAL:
			switch (lastPos) {
				case C1_THEN_2:									/* case 2a */
					*pCanSlur = FALSE;
					break;
				case CEQUAL:									/* case 2b */
					*pCanTie = *pCanSlur = FALSE;
					break;
				case C2_THEN_1:									/* case 2c */
					*pCanTie = FALSE;
					if (!SlurTIE(oldSlurL)) *pCanSlur = FALSE;
					break;
			}
			break;
		case C2_THEN_1:
			if (IsAfterIncl(oldLastL, newFirstL)) break;		/* no pblm if they're disjoint */
			switch (lastPos) {
				case C1_THEN_2:									/* case 3a */
				case CEQUAL:
					*pCanSlur = FALSE;
					break;
				case C2_THEN_1:									/* case 3b */
					*pCanTie = *pCanSlur = FALSE;
					break;
			}
			break;
	}
}


static Boolean NestingIsOK(
				LINK newFirstL, LINK newLastL,	/* firstSyncL, lastSyncL of proposed slur/tie */
				short staff,					/* on one end of the slur/tie */
				short voice,
				Boolean crossSys,
				Boolean crossStf,
				Boolean *mustBeTie
				)
{
	SearchParam pbSearch;
	LINK		oldSlurL;
	Boolean 	canTie=TRUE, canSlur=TRUE, canTieThis, canSlurThis, tiePossible;

	if (crossSys || crossStf) return TRUE;		/* FIXME: This doesn't look at all safe! */
	
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE; 		/* check slurs on *any* staff that are in our voice */
	pbSearch.voice = voice;				
	pbSearch.inSystem = TRUE;

	oldSlurL = newLastL;
	while (oldSlurL = L_Search(LeftLINK(oldSlurL), SLURtype, GO_LEFT, &pbSearch)) {
		if (IsAfter(SlurLASTSYNC(oldSlurL), newFirstL)) break;
		CheckProposedSlur(newFirstL, newLastL, oldSlurL,  &canTieThis, &canSlurThis);
		if (!canTieThis) canTie = FALSE;
		if (!canSlurThis) canSlur = FALSE;
	}

	*mustBeTie = !canSlur;
	tiePossible = ConsecSync(newFirstL, newLastL, staff, voice);
	if (!tiePossible) canTie = FALSE;
	return (canTie || canSlur);
}


/* --------------------------------------------------------------- FillTieArrays -- */
/* For ties, fills firstIndA and lastIndA with indices to the tied notes within
their respective chords in the given Syncs (first note=0), as specified by parallel
arrays fChordNote and lChordNote (presumably filled by CompareNCNotes, to handle
the accidental- tied-over-barline problem). If there's no chord, only a single note,
in that Sync, it just sets firstIndA[0] or lastIndA[0] to 0. In any case, it also
sets the relevant notes' tiedL/tiedR flags.

For a simpler approach that doesn't handle the accidental-tied-over-barline problem,
see BuildTieArrays. */

static short FillTieArrays(
				LINK firstL, LINK lastL,	/* Syncs */
				short voice,
				CHORDNOTE fChordNote[], 	/* NILINK noteL => ignore. NB: we destroy! */
				CHORDNOTE lChordNote[],
				short cnCount,
				char firstIndA[],
				char lastIndA[]
				)
{
	short subCount;
	char index,n;
	register LINK aNoteL, firstNoteL, lastNoteL;

	/* First, replace the legal CHORDNOTE .noteNums with indices. */
	
	index = -1;
	aNoteL = FirstSubLINK(firstL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			index++;
			for (n = 0; n<cnCount; n++)
				if (fChordNote[n].noteL==aNoteL) fChordNote[n].noteNum = index;
		}
	}
	
	index = -1;
	aNoteL = FirstSubLINK(lastL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			index++;
			for (n = 0; n<cnCount; n++)
				if (lChordNote[n].noteL==aNoteL) lChordNote[n].noteNum = index;
		}
	}
	
	/* Now fill in the firstIndA and lastIndA arrays, set tied flags, and make the last
		note's note number agree with the first's (for the accidental-tied-across-
		barlines case). */
	
	subCount = 0;
	for (n = 0; n<cnCount; n++) {
		firstNoteL = fChordNote[n].noteL;
		if (firstNoteL) {
			lastNoteL = lChordNote[n].noteL;
			NoteNUM(lastNoteL) = NoteNUM(firstNoteL);
			NoteTIEDR(firstNoteL) = NoteTIEDL(lastNoteL) = TRUE;
			firstIndA[subCount] = fChordNote[n].noteNum;
			lastIndA[subCount] = lChordNote[n].noteNum;
			subCount++;
		}
	}
	
	return subCount;
}


/* -------------------------------------------------------------------- WantTies -- */
/* Decide whether user wants a set of ties or a single slur. If the <config> struct
doesn't specify and the user hasn't said which to assume, ask them which they want.
Returns TRUE for ties, FALSE for slur. */

static enum {
	BUT1_OK=1,
	TIES_DI=3,
	SLUR_DI,
	ASSUME_DI=6
} E_WantTiesItems;

static short group1;

static Boolean WantTies(
					Boolean /*firstChord*/, Boolean /*lastChord*/)		/* Both unused, for now */
{
	short itemHit;
	Boolean value;
	static short assumeSlurTie=0, oldChoice=0;
	register DialogPtr dlog; GrafPtr oldPort;
	Boolean keepGoing=TRUE;
	static Boolean firstCall=TRUE;

	if (firstCall) {
		if (config.assumeTie==1)		assumeSlurTie = TIES_DI;
		else if (config.assumeTie==2)	assumeSlurTie = SLUR_DI;
		else							assumeSlurTie = 0;
		firstCall = FALSE;
	}
	
	if (assumeSlurTie)
		value = (assumeSlurTie==TIES_DI);
	else {
	
		/* Build dialog window and install its item values */
		
		ModalFilterUPP	filterUPP = NewModalFilterUPP(OKButFilter);
		if (filterUPP==NULL) {
			MissingDialog(SLURTIE_DLOG);
			return FALSE;
		}
		GetPort(&oldPort);
		dlog = GetNewDialog(SLURTIE_DLOG, NULL, BRING_TO_FRONT);
		if (dlog==NULL) {
			DisposeModalFilterUPP(filterUPP);
			MissingDialog(SLURTIE_DLOG);
			return FALSE;
		}
		SetPort(GetDialogWindowPort(dlog));
	
		/* Fill in dialog's values here */
	
		group1 = (oldChoice? oldChoice : TIES_DI);
		PutDlgChkRadio(dlog, TIES_DI, (group1==TIES_DI));
		PutDlgChkRadio(dlog, SLUR_DI, (group1==SLUR_DI));
	
		PlaceWindow(GetDialogWindow(dlog), (WindowPtr)NULL,0,80);
		ShowWindow(GetDialogWindow(dlog));
		ArrowCursor();
	
		/* Entertain filtered user events until dialog is dismissed */
		
		while (keepGoing) {
			ModalDialog(filterUPP, &itemHit);
			switch(itemHit) {
				case BUT1_OK:
					keepGoing = FALSE;
					value = (group1==TIES_DI);
					if (GetDlgChkRadio(dlog, ASSUME_DI)) assumeSlurTie = group1;
					oldChoice = group1;
					break;
				case TIES_DI:
				case SLUR_DI:
					if (itemHit!=group1) SwitchRadio(dlog, &group1, itemHit);
					break;
				case ASSUME_DI:
					PutDlgChkRadio(dlog, ASSUME_DI, !GetDlgChkRadio(dlog, ASSUME_DI));
					break;
				default:
					;
			}
		}
		
		DisposeModalFilterUPP(filterUPP);
		DisposeDialog(dlog);
		SetPort(oldPort);
	}
	
	return value;
}


/* ------------------------------------------------------------------- HandleTie -- */
/* HandleTie decides whether the curve connecting the given Syncs should be
a tie or set of ties, or a slur. If the situation is ambiguous, it asks the user.
Should be called only for consecutive Syncs, since otherwise it must be a slur. 

It sets subCount to the number of slur subobjects needed (always 1 for a slur). If
a tie, it also fills firstIndA and lastIndA with indices to the tied notes within
their respective chords, and it sets the relevant notes' tiedL/tiedR flags.

HandleTie returns F_TIE, F_SLUR, or CANCEL_INT. */

static short HandleTie(LINK firstL, LINK lastL, short voice, short *subCount,
								char firstIndA[MAXCHORD], char lastIndA[MAXCHORD])
{
	PANOTE		firstNote, lastNote;
	LINK		firstNoteL, lastNoteL;
	Boolean		firstChord, lastChord, itsATie;
	short		i, flCount, nMatch, nTiedMatch, proceed;
	CHORDNOTE	fChordNote[MAXCHORD], lChordNote[MAXCHORD];
	
	for (i = 0; i<MAXCHORD; i++) 
		firstIndA[i] = lastIndA[i] = -1;
		
	/*
	 *	Set <firstNote> and <lastNote> to the first note we find in <voice> in their
	 *	respective Syncs, and <firstChord> and <lastChord> to the chord status of
	 * <voice> in the Syncs.
	 */
	firstNoteL = FirstSubLINK(firstL);
	for ( ; firstNoteL; firstNoteL = NextNOTEL(firstNoteL)) {
		firstNote = GetPANOTE(firstNoteL);
		if (firstNote->voice==voice) {
			firstChord = firstNote->inChord;
			break;
		}
	}
	lastNoteL = FirstSubLINK(lastL);
	for ( ; lastNoteL; lastNoteL = NextNOTEL(lastNoteL)) {
		lastNote = GetPANOTE(lastNoteL);
		if (lastNote->voice==voice) {
			lastChord = lastNote->inChord;
			break;
		}
	}
		
	/*
	 *	If there's no match of pitches, the slur/tie must be a slur; if there is a
	 *	match, we may have to ask for help deciding which it is.
	 */
	itsATie = FALSE;

	CompareNCNotes(firstL, lastL, voice, &nMatch, &nTiedMatch, fChordNote,lChordNote,
						&flCount);
	if (nMatch!=nTiedMatch) {
		proceed = CautionAdvise(ACCIDENTAL_TIE_ALRT);
		if (proceed==Cancel) return CANCEL_INT;
	}
	if (nTiedMatch>0) {
		itsATie = WantTies(firstChord, lastChord);
		if (itsATie) {
			*subCount = FillTieArrays(firstL, lastL, voice, fChordNote, lChordNote,
										flCount, firstIndA, lastIndA);
		}
		else {
			firstIndA[0] = lastIndA[0] = 0;
			*subCount = 1;
		}
	}
	else {
		firstIndA[0] = lastIndA[0] = 0;
		*subCount = 1;
	}
	
	return (itsATie? F_TIE : F_SLUR);
}


/* ---------------------------------------------------------------- NewSlurOrTie -- */
/* Determine whether new "slur" is a tie or not. First check whether the new
slur/tie is nested within or overlapping an existing slur; if so, it must
(according to Nightingale's rules) be a tie. Then check if its endpts are
consecutive syncs in the voice; if not, it can't (according to the rules of
CMN) be a tie. Otherwise, it might be either a slur or a tie, so determine
which by calling HandleTie. If it must be a tie but it's not, we have an error.
Otherwise, be sure the end notes' slur/tie L/R flags are set properly. */

static char NewSlurOrTie(
				short staff,		/* on one end of the slur/tie */
				short voice)
{
	register LINK aNoteL;
	register PANOTE aNote;
	short status;
	
	/* Check for nested and overlapping slurs. Note that we must check before any
	 * changes are made to the data structure which will be hard to undo, such as
	 * setting tie flags in HandleTie. This causes a problem, since there are 
	 * differing error conditions depending on whether the user decides to insert
	 * a slur or tie, and this decision is made inside HandleTie, a routine which 
	 * makes hard-to-undo data structure changes. We can sidestep this clumsily by
	 * setting a slur-disallow or tie-disallow flag. 
	 */
 	itsATie = mustBeTie = FALSE;
	if (!NestingIsOK(firstSyncL, lastSyncL, staff, voice, crossSystem, crossStaff, &mustBeTie)) {
		GetIndCString(strBuf, SLURERRS_STRS, 4);    /* "There is already a tie and/or slur here, and Nightingale doesn't allow overlapping or nested ties or slurs." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		InvalMeasures(firstSyncL, lastSyncL, staff);
		return USR_ALERT;
	}
	
	/*
	 *	If the "slur" includes exactly two Syncs, at least one of which is a chord, that
	 *	include notes in this staff/voice of the same pitch, it might be either a tie or
	 *	a slur;  if neither is a chord, it's a tie. "Same pitch" is usually indicated by
	 *	MIDI note number, not spelling, though across barlines, things are more subtle.
	 */
	if (ConsecSync(firstSyncL, lastSyncL, endStaff, voice)) {
		status = HandleTie(firstSyncL, lastSyncL, voice, &subCount, firstIndA, lastIndA);
		if (status==CANCEL_INT) {
			InvalMeasures(firstSyncL, lastSyncL, staff);
			return USR_ALERT;
		}
		itsATie = (status==F_TIE);
	}
	else {
		itsATie = FALSE;
		subCount = 1;
	}
	
	if (mustBeTie)		/* Guarantee order of evaluation */
		if (!itsATie) {
			GetIndCString(strBuf, SLURERRS_STRS, 5);    /* "You cannot insert a slur here." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			InvalMeasures(firstSyncL, lastSyncL, staff);
			return USR_ALERT;
		}
	/*
	 *	If we're making a set of ties, the notes' tied flags have already been set,
	 *	in HandleTie. If we're making a slur, we set the chords' slurred flags now.
	 */
	if (!itsATie) {
		aNoteL = FirstSubLINK(firstSyncL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->voice==voice)
				if (MainNote(aNoteL)) aNote->slurredR = TRUE;
		}
		aNoteL = FirstSubLINK(lastSyncL);
		for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL);
			if (aNote->voice==voice)
				if (MainNote(aNoteL)) aNote->slurredL = TRUE;
		}
	}
	return NF_OK;
}


/* Insert the slur object before doc->selStartL, which is set to the slur's first 
sync; initialize fields in the slur object. */

static LINK AddNewSlur(Document *doc, LINK insertL, short staff, short voice)
{
	LINK newL, aSlurL; register PSLUR pSlur; PASLUR aSlur;
	CONTEXT context;
	newL = InsertNode(doc, insertL, SLURtype, subCount);
	if (!newL) { NoMoreMemory(); return NILINK; }

	NewObjSetup(doc, newL, staff, 0);
	GetContext(doc, newL, staff, &context);
 	
 	pSlur = GetPSLUR(newL);
 	pSlur->staffn = staff;
 	pSlur->voice = voice;
 	pSlur->tie = itsATie;
 	pSlur->firstSyncL = firstSyncL;
 	pSlur->lastSyncL = lastSyncL;
 	pSlur->crossSystem = FALSE;
 	pSlur->crossStaff = crossStaff;
 	pSlur->crossStfBack = crossStfBack;
	aSlurL = FirstSubLINK(newL);
	for ( ; aSlurL; aSlurL=NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
		SetRect(&aSlur->bounds, 0, 0, 0, 0);					/* A precaution */
	}

	return newL;
}


static void ClearSlurIndices(LINK newL)
{
	LINK aSlurL;
	register PASLUR aSlur;

	aSlurL = FirstSubLINK(newL);
	for ( ; aSlurL; aSlurL=NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
		aSlur->firstInd = aSlur->lastInd = 0;
	}
}

/* Set the indices which connect tie subobjects with notes in the chords they tie.
If it is a slur, the firstInd and lastInd are 0. Otherwise, we assume the arrays
firstIndA and lastIndA have been initialized, i.e., by HandleTie, to associate
successive notes in the first and last chords with the same note number. Here, we
iterate through the slur subobjects and set aSlur->firstInd/lastInd to
firstIndA/lastIndA value indexed by the position of aSlur in its linked list. */

static char SetSlurIndices(LINK newL)
{
	short firsti=0, lasti=0;
	LINK aSlurL;
	register PASLUR aSlur;
	
 	if (!itsATie) {
 		ClearSlurIndices(newL); return NF_OK;
 	}

	aSlurL = FirstSubLINK(newL);
	for ( ; aSlurL; aSlurL=NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
 		while (firstIndA[firsti]<0) {
 			 firsti++;
 			 if (firsti>=MAXCHORD) {
 			 	MayErrMsg("SetSlurIndices: firstIndA not set for newL %ld",
 			 				(long)newL);
 			 	return PROG_ERR;
 			 }
 		}
 		while (lastIndA[lasti]<0) {
 			lasti++;
 			 if (lasti>=MAXCHORD) {
 			 	MayErrMsg("SetSlurIndices: lastIndA not set for newL %ld",
 			 				(long)newL);
 			 	return PROG_ERR;
 			 }
 		}
		aSlur->firstInd = firstIndA[firsti];
		aSlur->lastInd = lastIndA[lasti];
 		firsti++; lasti++;
	}
	return NF_OK;
}


/* Given a tie subobject and voice number, deliver the LINK of the note its left
end is attached to. */

static LINK Tie2NoteLINK(
				LINK theTieL,
				short voice,
				LINK syncL	/* firstSyncL of tie's Slur object: must really be a Sync */
				)
{
	PASLUR aTie;
	LINK aNoteL;
	short ind, i, n;
	
	aTie = GetPASLUR(theTieL);
	ind = aTie->firstInd;			/* Get sequence no. of note in its chord */
		
	aNoteL = FirstSubLINK(syncL);
	for (i = 0, n = 0; aNoteL; i++, aNoteL = NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			if (n==ind) return aNoteL;
			n++;
		}
	}
		
	return NILINK;
}


/* Decide whether each tie should curve up or down based on vertical order (NOT
position or stem direction) of the notes it ties: ties on the upper notes curve
up, the lower ones down. For example, if two 4-note chords are tied, the two
upper ties curve up, the two lower ones down, regardless of the notes. NB:
should also consider doc->voiceTab[voice].voiceRole: non-single voices affect
correct curve dir. */

void GetTiesCurveDir(Document */*doc*/, short voice, LINK slurL, Boolean curveUps[])
{
	LINK aTieL, aNoteL, syncL;
	CHORDNOTE chordNote[MAXCHORD];
	short count, s, n, pitchOrder;

	firstSyncL = syncL = SlurFIRSTSYNC(slurL);
	if (MeasureTYPE(syncL)) syncL = SlurLASTSYNC(slurL);
	count = GSortChordNotes(syncL, voice, TRUE, chordNote);
	
	aTieL = FirstSubLINK(slurL);
	for (s = 0; aTieL; s++, aTieL = NextSLURL(aTieL)) {
		aNoteL = Tie2NoteLINK(aTieL, voice, syncL);
		if (aNoteL==NILINK) MayErrMsg("GetTiesCurveDir: Tie2NoteLINK couldn't find note LINK for tie %ld at %ld",
												(long)s, (long)slurL);
		else {
			for (pitchOrder = -1, n = 0; n<count; n++)
				if (chordNote[n].noteL==aNoteL) pitchOrder = n;
			if (pitchOrder<0) MayErrMsg("GetTiesCurveDir: couldn't find chord note for tie %ld at %ld.",
												(long)s, (long)slurL);
			curveUps[s] = (pitchOrder<count/2);
		}
	}
}


/* Decide whether the slur or single tie should curve up or down based on stem
direction of the notes it ties or slurs. NB: should also consider
doc->voiceTab[voice].voiceRole: non-single voices affect correct curve dir. */

void GetSlurTieCurveDir(Document */*doc*/, short voice, LINK slurL, Boolean *curveUp)
{
	LINK syncL, aNoteL;

		/*
		 *	Decide whether the slur should curve up or down based on 1st note stem dir.
		 *	The following test works even for stemless notes, since SetupNote still
		 *	sets the stem endpoint.  ??BUT SHOULD CONSIDER ALL NOTES IN THE SLUR!
		 */
		syncL = SlurFIRSTSYNC(slurL);
		if (MeasureTYPE(syncL)) syncL = SlurLASTSYNC(slurL);
		aNoteL = FindMainNote(syncL, voice);
		*curveUp = (NoteYSTEM(aNoteL) > NoteYD(aNoteL));
}


/* Set the Bezier control and anchor points for a non-crossSystem slur or set of
ties. For now, at least, they're always computed in a standard way, regardless of
auto-respace. */

static void NewSlurSetCtlPts(Document *doc, short staff, short voice, CONTEXT context)
{
	register LINK aSlurL;
	register PASLUR aSlur;
	Boolean curveUp, curveUps[MAXCHORD];
	short i;
	
	if (subCount==1) {
		GetSlurTieCurveDir(doc, voice, slurL, &curveUp);

		aSlurL = FirstSubLINK(slurL);
		aSlur = GetPASLUR(aSlurL);
		aSlur->startPt = startPt[0];
		aSlur->endPt = endPt[0];
		SetSlurCtlPoints(doc, slurL, aSlurL, firstSyncL, lastSyncL, staff, voice,
								context, curveUp);
	}
	else {
		GetTiesCurveDir(doc, voice, slurL, curveUps);

		aSlurL = FirstSubLINK(slurL);
		for (i = 0; aSlurL; i++, aSlurL=NextSLURL(aSlurL)) {
			aSlur = GetPASLUR(aSlurL);
			aSlur->startPt = startPt[i];
			aSlur->endPt = endPt[i];
			SetSlurCtlPoints(doc, slurL, aSlurL, firstSyncL, lastSyncL, staff, voice,
									context, curveUps[i]);
		}
	}
}


/* Set control points for cross-system slurs. */

static void CrossSysSetCtlPts(
					Document *doc,
					short staff, short voice,
					LINK newL,				/* Slur object: either piece of a cross-system slur */
					CONTEXT context
					)
{
	register LINK aSlurL;
	register PASLUR aSlur;
	Boolean curveUp, curveUps[MAXCHORD];
	short i;
	
	if (subCount==1) {
		GetSlurTieCurveDir(doc, voice, newL, &curveUp);

		aSlurL = FirstSubLINK(newL);
		aSlur = GetPASLUR(aSlurL);
		aSlur->startPt = startPt[0];
		aSlur->endPt = endPt[0];
		SetSlurCtlPoints(doc, newL, aSlurL, firstSyncL, lastSyncL, staff, voice,
								context, curveUp);
	}
	else {
		GetTiesCurveDir(doc, voice, newL, curveUps);

		aSlurL = FirstSubLINK(newL);
		for (i = 0; aSlurL; i++, aSlurL=NextSLURL(aSlurL)) {
			aSlur = GetPASLUR(aSlurL);
			aSlur->startPt = startPt[i];
			aSlur->endPt = endPt[i];
			SetSlurCtlPoints(doc, newL, aSlurL, firstSyncL, lastSyncL, staff, voice,
									context, curveUps[i]);
		}
	}
}


/* Set flags in the slur, set doc->selStartL, load the Undo clipboard, and inval 
measures. */

static void NewSlurCleanup(
					Document *doc,
					short		staff,
					LINK		newL,
					Boolean	user 		/* TRUE=assume call results from user directly creating slur */
					)
{
	register PASLUR aSlur;
	LINK   aSlurL;
	
	aSlurL = FirstSubLINK(newL);
	for ( ; aSlurL; aSlurL=NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
		aSlur->selected = user;
		aSlur->visible = TRUE;
		aSlur->soft = FALSE;
		aSlur->dashed = 0;
		aSlur->filler = 0;
		aSlur->reserved = 0L;
	}

	if (user) {
		doc->selStartL = newL; 												
		doc->selEndL = RightLINK(newL);										
	}
	InvalMeasures(firstSyncL, lastSyncL, staff);							
	InvalObject(doc,newL,FALSE);							/* Redraw slur in case outside system */
}

/* First, this adds a slur in the first system, and sets pSlur->lastSyncL to be the
system object for that system. Then it sets the indices for this slur, gets the
arrays of startPt's and endPt's, and calls CrossSysSetCtlPts to set control and
anchor points. Then it deselects this slur, so that only one slur will be selected
at the end of the operation. Do the same for the slur on the next system, except
that it stays selected. */

static void NewCrossSystemSlur(Document *doc, short staff, short voice, CONTEXT context)
{
	PSLUR pSlur;
	PASLUR aSlur;
	LINK  newL1, newL2, aSlurL, oldLastSync, sysL, firstMeasL;
	
	/* Add the slur on the first system. */
	newL1 = AddNewSlur(doc, LeftLINK(doc->selStartL), staff, voice);

	/* Set the slur's lastSyncL to its system. */

	pSlur = GetPSLUR(newL1);
	pSlur->lastSyncL = LSSearch(newL1, SYSTEMtype, ANYONE, TRUE, FALSE);
	pSlur->crossSystem = TRUE;
	pSlur->crossStaff = FALSE;

	SetSlurIndices(newL1);
	GetSlurContext(doc, newL1, startPt, endPt);
	
	/* Set firstSyncL and lastSyncL so that they can be used by CrossSysSetCtlPts. 
		oldLastSyncL holds the lastSync (on the second system), which we will need
		to know when we insert the second slur. */

	pSlur = GetPSLUR(newL1);
	firstSyncL = pSlur->firstSyncL;
	oldLastSync = lastSyncL;
	lastSyncL = pSlur->lastSyncL;
	CrossSysSetCtlPts(doc, staff, voice, newL1, context);
	
	/* Deselect the slur and subObjects so that only one slur will end up
		selected. */
	LinkSEL(newL1) = FALSE;
	aSlurL = FirstSubLINK(newL1);
	for ( ; aSlurL; aSlurL=NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
		aSlur->selected = FALSE;
		aSlur->visible = TRUE;
		aSlur->soft = FALSE;
	}

	/* Add the slur on the second system. Reset doc->selStartL and doc->selEndL
		so that second slur will be inserted immediately after the first measure
		of the system. The firstSyncL field of this slur is filled by the first
		measure of the system. */

	sysL = LSSearch(oldLastSync, SYSTEMtype, ANYONE, GO_LEFT, FALSE);
	firstMeasL = LSSearch(sysL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);

	lastSyncL = oldLastSync;
	doc->selStartL = firstMeasL;
	doc->selEndL = RightLINK(firstMeasL);
	newL2 = AddNewSlur(doc, RightLINK(doc->selStartL), endStaff, voice);

	pSlur = GetPSLUR(newL2);
	pSlur->firstSyncL = firstMeasL;
	pSlur->crossStaff = FALSE;

	SetSlurIndices(newL2);
	GetSlurContext(doc, newL2, startPt, endPt);
	
	pSlur = GetPSLUR(newL2);
	firstSyncL = pSlur->firstSyncL;
	lastSyncL = pSlur->lastSyncL;
	pSlur->crossSystem = TRUE;
	CrossSysSetCtlPts(doc, endStaff, voice, newL2, context);
	
	/* Clean up, and set all the soft flags for the second slur. Reset firstSyncL
		to the first sync of the first slur to include in the Inval Range. */
	pSlur = GetPSLUR(newL1);
	firstSyncL = pSlur->firstSyncL;
	InvalObject(doc,newL1,FALSE);

	NewSlurCleanup(doc, staff, newL1, FALSE);
	NewSlurCleanup(doc, endStaff, newL2, TRUE);
	LinkSOFT(newL2) = TRUE;
	aSlurL = FirstSubLINK(newL2);
	for ( ; aSlurL; aSlurL=NextSLURL(aSlurL)) {
		aSlur = GetPASLUR(aSlurL);
		aSlur->soft = FALSE;
	}
}


/* ------------------------------------------------------------------ SlurToNext -- */
/* Assuming <firstSyncL> is set, set <lastSyncL> and cross-staff/system variables
to make slur/tie go to next note/chord in <voice>. */

static Boolean SlurToNext(Document *doc,
									short *pStaff,		/* Input AND output */
									short voice)
{
 	SearchParam pbSearch;
 	LINK			aNoteL, lastSystemL;
 	PSYSTEM		pSystem;
 	short			tempStf;
 		
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = FALSE;
	pbSearch.inSystem = FALSE;
	lastSyncL = L_Search(RightLINK(firstSyncL), SYNCtype, FALSE, &pbSearch);

	if (!lastSyncL) return FALSE;
	aNoteL = pbSearch.pEntry;
	
	lastSystemL = LSSearch(lastSyncL, SYSTEMtype, ANYONE, TRUE, FALSE);
	pSystem = GetPSYSTEM(lastSystemL);
	crossSystem = (pSystem->systemNum!=doc->currentSystem);
	crossSysBack = (pSystem->systemNum<doc->currentSystem);
	endStaff = NoteSTAFF(aNoteL);
	crossStaff = endStaff!=*pStaff;
	crossStfBack = endStaff<*pStaff;

	if (crossStaff && crossStfBack) {
		tempStf = endStaff;									/* Swap staff numbers */
		endStaff = *pStaff;
		*pStaff = tempStf;
	}

	return TRUE;
}


/* --------------------------------------------------------------------- NewSlur -- */
/*	Add a slur or tie on <staff> and <voice> at <pt>. The firstSync is the node
immediately to the left of <pt> on <staff>/<voice>; NewSlur checks that this
is a sync, tracks the slur, checks systems and staves, finds a valid last sync,
checks conditions for adding a tie or slur, and inserts the slur. BAD_CODE
indicates that we have had a program error or user alert in one of the
called functions, and must return without completing the operation. */ 

#define SLOP 2		/* Minimum slur-drawing horizontal distance (pixels) */

void NewSlur(Document *doc, short staff, short voice, Point pt)
{
	CONTEXT context;	LINK firstMeas;
	Point	newPt,globPt; short ans;
	Rect newPaper,paper;
	
	/* Disable later if problem */
	PrepareUndo(doc, doc->selStartL, U_Insert, 13);    			/* "Undo Insert" */

	slurToNext = FALSE;
	firstMeas = LSSearch(doc->headL, MEASUREtype, ANYONE, FALSE, FALSE);
	GetContext(doc, doc->selStartL, staff, &context);

	/* Sets firstSyncL, firstNoteL, checks if firstSync has a note on <staff> &
		<voice>. */
	returnCode = HandleFirstSync(doc, staff, voice);
	if (BAD_CODE(returnCode)) goto Disable;

	/* Tracks the slur, returns the second anchor point in <newPt> (window coords). */
	globPt = NewTrackSlur(doc, &context.paper, pt, &newPaper);
	newPt = globPt;
	newPt.h -= newPaper.left;								/* Convert back to paper-relative coords */
	newPt.v -= newPaper.top;
	
	/* If mouse moved horizontally only a very short distance, make slur/ties go to
		the next note/chord to the right in voice <voice>. */
	if (ABS(newPt.h-pt.h)<SLOP) {
		slurToNext = TRUE;
		if (!SlurToNext(doc, &staff, voice)) {
			InvalMeasure(firstSyncL, staff);
			goto Disable;
		}
	}
	else {
	
		if (!EqualRect(&context.paper,&newPaper)) {
			/* Cross-page slur: need to change context to new pageRect */
			if (FindSheet(doc,globPt,&ans)) {
				GetSheetRect(doc,ans,&paper);
				doc->currentSheet = ans;
				doc->currentPaper = paper;
				FindStaff(doc, globPt);						/* Set currentSystem */
				}
			}
		
		/* Gets the staff containing <newPt>, checks if <newPt>'s staff and system
			are same as that of first anchor point. Sets crossStaff TRUE if staves
			are different; sets crossSystem TRUE if systems are different. */

		CheckCrossness(doc, staff, newPt);
	
		/* Gets valid lastSyncL. */
		returnCode = HandleLastSync(doc, staff, voice, newPt);
		if (BAD_CODE(returnCode)) goto Disable;
		
		/* If slur drawn right to left, swaps anchor and ctl points. */
		SwapEndpoints(doc, &staff);
	}
	
	/* Determines if tie or slur to be inserted, checks associated error conditions,
		fills in firstIndA/lastIndA, etc. */
	returnCode = NewSlurOrTie(staff, voice);
	if (BAD_CODE(returnCode)) goto Disable;
	
	/* Inserts cross system slurs, and returns. */
	if (crossSystem) {
		NewCrossSystemSlur(doc, staff, voice, context);
		goto Disable;
	}
	
	/* Inserts slur into data structure. */
	slurL = AddNewSlur(doc, LeftLINK(doc->selStartL), staff, voice);
	if (!slurL) goto Disable;
	
	/* Sets indices of slur subobjects for ties between chords. */
	returnCode = SetSlurIndices(slurL);
	if (BAD_CODE(returnCode)) goto Disable;

	/* Get arrays of startPts, endPts for ties between chords, set ctl & anchor
		points for slur subobjects, and clean up. */
	GetSlurContext(doc, slurL, startPt, endPt);
	NewSlurSetCtlPts(doc, staff, voice, context);
	NewSlurCleanup(doc, staff, slurL, TRUE);
	
	return;

Disable:
	DisableUndo(doc, FALSE);
}


/* -------------------------------------------------------------- BuildTieArrays -- */
/* For ties, fills firstIndA and lastIndA with indices to the tied notes within
their respective chords in the given Syncs (first note=0). If there's no chord, only
a single note, in that Sync, it just sets firstIndA[0] or lastIndA[0] to 0. In any
case, it also sets the relevant notes' tiedL/tiedR flags.

Will have problems if either chord (or only the last?) contains duplicate noteNums. */

static short BuildTieArrays(LINK firstL, LINK lastL, short voice,
									char firstIndA[MAXCHORD], char lastIndA[MAXCHORD])
{
	short subCount;
	char firstInd, lastInd;
	PANOTE firstNote, lastNote;
	register LINK firstNoteL, lastNoteL;

	subCount = 0;
	firstInd = -1;
	firstNoteL = FirstSubLINK(firstL);
	for ( ; firstNoteL; firstNoteL = NextNOTEL(firstNoteL)) {
		firstNote = GetPANOTE(firstNoteL);
		if (firstNote->voice==voice) {
			firstInd++;
			lastInd = -1;
			lastNoteL = FirstSubLINK(lastL);
			for ( ; lastNoteL; lastNoteL = NextNOTEL(lastNoteL)) {
				lastNote = GetPANOTE(lastNoteL);
				if (lastNote->voice==voice) {
					lastInd++;
					if (firstNote->noteNum==lastNote->noteNum) {
						firstNote->tiedR = lastNote->tiedL = TRUE;
						firstIndA[subCount] = firstInd;
						lastIndA[subCount] = lastInd;
						subCount++;
					}
				}
			}
		}
	}
	
	return subCount;
}


/* ----------------------------------------------------------------- AddNoteTies -- */
/* Generate a set of ties between the given Syncs for the given staff and voice.
Also set the tiedR and tiedL flags for the notes involved. Restrictions:
1. Since the staff must be specified, not just the voice, this can't be used to
	create cross-staff ties.
2. The sets of notes in the voice in the two Syncs must be identical, i.e., they
	contain no notes that won't be tied.??PROBABLY NOT REQUIRED ANY MORE!
3. The Syncs must be in the same System: this can't create cross-system ties.

Note that the shapes of the ties naturally depend on the space between the notes:
if the notes are moved after the ties have been added, call SetAllSlursShape to
fix them. Delivers the number of ties normally, 0 if either there are no notes in
the given voice in the first Sync or an error is found. */

short AddNoteTies(Document *doc, LINK pL, LINK qL, short staff, short voice)
{
	short returnCode;
	CONTEXT context;

	/* Set all the global variables used by the NewSlur routines. */
	
	itsATie = TRUE;
	firstSyncL = pL; lastSyncL = qL;
	GetContext(doc, pL, staff, &context);
	crossStaff = FALSE;
	subCount = BuildTieArrays(firstSyncL, lastSyncL, voice, firstIndA, lastIndA);
	if (subCount<=0) return 0;
	
	/* Add the slur object and set its tie indices. */
	
	slurL = AddNewSlur(doc, pL, staff, voice);
	if (slurL==NILINK) return 0;
	returnCode = SetSlurIndices(slurL);
	if (returnCode<=0) return 0;

	/* Get arrays of startPts, endPts for ties between chords, set ctl & anchor
	points for slur subobjects, and clean up. */
	
	GetSlurContext(doc, slurL, startPt, endPt);
	NewSlurSetCtlPts(doc, staff, voice, context);
	NewSlurCleanup(doc, staff, slurL, FALSE);
	
	return subCount;
}
