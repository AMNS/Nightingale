/***************************************************************************
*	FILE:	InsUtils.c																			*
*	PROJ:	Nightingale, rev. for v.99														*
*	DESC:	Utility routines for inserting symbols.									*

	GetPaperMouse
	ForceInMeasure			GetInsRect				IDrawHairpin
	TrackHairpin			GetCrossStatus			TrackAndAddHairpin
	IDrawEndingBracket	TrackEnding				TrackAndAddEnding
	IDrawLine				TrackLine				InsFindNRG
	TrackAndAddLine		FindSync
	AddCheckVoiceTable	AddNoteCheck
	FindGRSync				AddGRNoteCheck			FindSyncRight			
	FindSyncLeft			FindGRSyncRight		FindGRSyncLeft
	FindSymRight			FindLPI					ObjAtEndTime
	GetPitchLim				CalcPitchLevel			FindStaff
	SetCurrentSystem		GetCurrentSystem		AddGRNoteUnbeam
	AddNoteFixBeams	
	AddNoteFixTuplets 	AddNoteFixOctavas		AddNoteFixSlurs
	AddNoteFixGraphics	FixWholeRests			FixNewMeasAccs
	InsFixMeasNums			FindGraphicObject		ChkGraphicRelObj
	FindTempoObject		FindEndingObject
	UpdateTailxd			CenterTextGraphic
	NewObjInit				NewObjSetup				NewObjPrepare			 	
	NewObjCleanup			XLoadInsUtilsSeg
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-99 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


static void IDrawHairpin(Point, Point, Boolean, PCONTEXT);
static Boolean TrackHairpin(Document *, LINK, Point, Point *, Boolean, short);
static Boolean GetCrossStatus(Document *, Point, Point, short *, short *);
static void AddGRNoteUnbeam(Document *, LINK, LINK, LINK, short);
static LINK NextStaff(Document *, LINK);
static DDIST GetTopStfLim(Document *,LINK,LINK,PCONTEXT);
static DDIST GetBotStfLim(Document *,LINK,LINK,PCONTEXT);

/* --------------------------------------------------------------- GetPaperMouse -- */
/* Deliver mouse position in paper-relative coordinates, and deliver TRUE or
FALSE, according to whether it's inside paper or not.  Paper is a rectangle
in window-relative coordinates. */

Boolean GetPaperMouse(Point *pt, Rect *paper)
{
	Boolean ans;
	
	GetMouse(pt);					/* In window-relative coords */
	ans = PtInRect(*pt,paper);	/* Test for inclusion */
	pt->h -= paper->left;		/* Transform to paper-relative coords */
	pt->v -= paper->top;
	
	return(ans);
}

/* -------------------------------------------------------------- ForceInMeasure -- */
/* Given a horizontal position in in window-local coordinates, if it's before
doc->currentSystem's initial (invisible) barline, return a value 1 pixel to right
of the that barline. Intended to be used by insertion routines that want to avoid
trying to insert symbols before an initial measure. */

short ForceInMeasure(Document *doc, short x)
{
	DDIST sysLeft;		LINK sysL, firstMeasL;
	
	sysLeft = GetSysLeft(doc);
	sysL = LSSearch(doc->headL, SYSTEMtype, doc->currentSystem, GO_RIGHT, FALSE);
	firstMeasL = LSSearch(sysL, MEASUREtype, 1, GO_RIGHT, FALSE);
	if (p2d(x)<LinkXD(firstMeasL)+sysLeft)
		x = d2p(LinkXD(firstMeasL)+sysLeft)+1;
	return x;
}


/* ------------------------------------------------------------------ GetInsRect -- */
/* Given a link where something is to be inserted, return a Rect (in paper-relative)
coordinates that the cursor should be within to insert it, i.e., if the cursor ends
up outside that Rect, the insert should be cancelled. */

void GetInsRect(Document *doc, LINK startL, Rect *tRectp)
{
	Rect		docRect,			/* portRect->bounds without scroll bar areas */
				sysRect;
	PSYSTEM	pSystem;
	LINK		pSystemL;
	WindowPtr w=doc->theWindow;

	/* Get current view in paper-relative coords for current sheet */
	docRect = doc->viewRect;
	OffsetRect(&docRect,-doc->currentPaper.left,-doc->currentPaper.top);
	
	pSystemL = LSSearch(LeftLINK(startL), SYSTEMtype, ANYONE, TRUE, FALSE);
	pSystem = GetPSYSTEM(pSystemL);
	D2Rect(&pSystem->systemRect, &sysRect);
	InsetRect(&sysRect, 0, -8);									/* Allow some vertical slop */
	SectRect(&docRect, &sysRect, tRectp);						/* Find insertion area */
}


/* -------------------------------------------------------------------- Hairpins -- */

/* Draw the hairpin during the dragging loop. */

static void IDrawHairpin(Point downPt,			/* Original mouseDown */
									Point pt,			/* new pt. */
									Boolean left,		/* TRUE if hairpin left. */
									PCONTEXT pContext
								)
{
   short   rise;
        
   rise = d2p(qd2d(config.hairpinMouthWidth,
            pContext->staffHeight, pContext->staffLines)>>1);

   if (left) {
      MoveTo(downPt.h,downPt.v+rise);
      LineTo(pt.h, pt.v);
      MoveTo(downPt.h,downPt.v-rise);
      LineTo(pt.h, pt.v);
   }
   else {
      MoveTo(downPt.h,downPt.v);
      LineTo(pt.h, pt.v+rise);
      MoveTo(downPt.h,downPt.v);
      LineTo(pt.h, pt.v-rise);
   }
}

/* Track mouse dragging out a hairpin dynamic and give feedback. */

#define HAIRPIN_MINLEN 5		/* in pixels */

Boolean TrackHairpin(Document *doc, LINK pL, Point pt, Point *endPt, Boolean left,
							short staff)
{
	Point		oldPt, newPt;
	short		endv;
	Boolean	firstLoop;
	CONTEXT	context;
	Rect		tRect;

	GetContext(doc, pL, staff, &context);
	GetInsRect(doc, pL, &tRect);

	firstLoop = TRUE;
	ArrowCursor();
	PenMode(patXor);								/* Everything will be drawn twice */
	Rect2Window(doc, &tRect);
	Pt2Window(doc, &pt);
	oldPt = newPt = pt;
	
	if (StillDown())
		while (WaitMouseUp()) {
			GetMouse(&newPt);
			if (newPt.h!=pt.h /* && PtInRect(newPt, &tRect) */) {

				/* Erase old hairpin, if there is one */
				if (firstLoop) firstLoop = FALSE;
				else				IDrawHairpin(oldPt, pt, left, &context);
				pt = newPt;
				/* Draw new hairpin; constrain to horizontal movement. Save endv for
					crossSystem hairpins. */
				endv = pt.v;
				pt.v = oldPt.v; 
				IDrawHairpin(oldPt, pt, left, &context);
			}
		}
	
	*endPt = pt;
	endPt->v = endv;	
	
	PenNormal();

	if (ABS(newPt.h-oldPt.h)<HAIRPIN_MINLEN) return FALSE;
#ifdef NO_CROSS_SYSTEM_HAIR_THINGS
	else if (!PtInRect(newPt, &tRect)) return FALSE;	/* Mouse must still be in System */
#endif
	else return TRUE;
}

/* Determine if the hairpin or ending is crossStaff and/or crossSystem and return
the first and last systems. */

static Boolean GetCrossStatus(Document *doc, Point pt, Point newPt,
										short *firstStf, short *lastStf)
{
	*firstStf = FindStaff(doc, pt);
	firstDynSys = doc->currentSystem;
	*lastStf = FindStaff(doc, newPt);
	lastDynSys = doc->currentSystem;			/* set by FindStaff */
	return (firstDynSys != lastDynSys);		/* TRUE if crossSystem */
}

/* Track the hairpin; determine (1) if it's crossSystem and (2) considering whether
it was entered forwards or backwards, if it's a crescendo or diminuendo; and
add it to the data structure. */

Boolean TrackAndAddHairpin(Document *doc, LINK insSyncL, Point pt, short clickStaff,
										short sym, short subtype, PCONTEXT pContext)
{
	Boolean isDIM,crossSys; short pitchLev,firstStf,lastStf;
	LINK lastSyncL; Point newPt;

	isDIM = (subtype==DIM_DYNAM);
	if (TrackHairpin(doc, insSyncL, pt, &newPt, isDIM, clickStaff)) {
		Pt2Paper(doc, &newPt);
		crossSys = GetCrossStatus(doc, pt, newPt, &firstStf, &lastStf);

		pitchLev = d2halfLn(p2d(pt.v)-pContext->staffTop,
							pContext->staffHeight, pContext->staffLines);
		lastSyncL = GSSearch(doc, newPt, SYNCtype, clickStaff, GO_LEFT,
									FALSE, FALSE, TRUE);
		if (!lastSyncL) {

			lastSyncL = FindSymRight(doc, newPt, TRUE, FALSE);
			if (SystemTYPE(lastSyncL) || PageTYPE(lastSyncL))
				lastSyncL = LSSearch(lastSyncL,SYNCtype,clickStaff,GO_LEFT,FALSE);
			
			if (!lastSyncL || !SyncTYPE(lastSyncL)) {
				GetIndCString(strBuf, INSERTERRS_STRS, 9);    /* "You can't put a hairpin before the first measure of the system." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				goto Done;
			}
		}
		HiliteInsertNode(doc, lastSyncL, clickStaff, TRUE);
		SleepTicks(HILITE_TICKS);
		/*
		 * Determine if the hairpin was drawn backwards or forwards and, if backwards,
		 * swap the endpoints. Also,if it was drawn backwards, change <sym> from crescendo
		 *	to diminuendo or vice versa; set the insertion pt at doc->selStartL to be
		 *	either firstSyncL or lastSyncL, and pass firstSyncL or lastSyncL to NewDynamic
		 *	as the lastSyncL accordingly; and call NewDynamic with parameter <crossSys>,
		 *	which NewDynamic will handle to create one or two pieces of the hairpin.
		 *
		 * NB: this and InsertHairpin badly need rewriting.
		 */
		if ((!crossSys && newPt.h > pt.h) || (crossSys && newPt.v > pt.v)) {
			if (IsAfter(lastSyncL, insSyncL)) lastSyncL = insSyncL;

			doc->selEndL = doc->selStartL = insSyncL;
			NewDynamic(doc, pt.h, newPt.h, symtable[sym].symcode,	/* Dragged forwards. */
						clickStaff, pitchLev, lastSyncL, crossSys);
		}
		else if (crossSys && newPt.v < pt.v) {
			doc->selStartL = doc->selEndL = lastSyncL;
			lastSyncL = insSyncL;
			NewDynamic(doc, newPt.h, pt.h, symtable[sym].symcode,	/* crossSys & dragged backwards. */
						clickStaff, pitchLev, lastSyncL, crossSys);
		}
		else {
			if (IsAfter(insSyncL, lastSyncL)) lastSyncL = insSyncL;

			sym = (isDIM ? sym+1 : sym-1);
			doc->selStartL = doc->selEndL = lastSyncL;
			lastSyncL = insSyncL;
			NewDynamic(doc, newPt.h, pt.h, symtable[sym].symcode,	/* !crossSys & dragged backwards: */
						clickStaff, pitchLev, lastSyncL, crossSys);	/*		reverse the hairpin. */
		}
		return TRUE;
	}
	else {																		/* Insert cancelled. */
Done:
		InvalSystem(insSyncL);												/* Redraw the system to erase feedback */
		return FALSE;
	}
}


/* ---------------------------------------------------------------------- Endings -- */

static void IDrawEndingBracket(Point downPt, Point pt, DDIST lnSpace);
static Boolean TrackEnding(Document *, LINK, Point, Point *, short);

/* Draw the ending bracket during the dragging loop. */

static void IDrawEndingBracket(Point downPt, Point pt,	/* Original mouseDown point, current point */
										DDIST lnSpace					/* Space between staff lines. */
										)
{
	short stfSpace;				/* Pixel distance between stafflines */
	
	stfSpace = d2p(lnSpace);	/* Draw down 2 stafflines. */

	MoveTo(downPt.h,downPt.v);
	LineTo(downPt.h,downPt.v-2*stfSpace);
	LineTo(pt.h,downPt.v-2*stfSpace);
	LineTo(pt.h,downPt.v);
}

/* Track mouse dragging out an ending bracket and give feedback. */

#define ENDING_MINLEN 2		/* in pixels */

Boolean TrackEnding(Document *doc, LINK pL, Point pt, Point *endPt, short staff)
{
	Point	oldPt, newPt; short endv; Boolean firstLoop;
	CONTEXT context; DDIST lnSpace; Rect tRect;

	GetContext(doc, pL, staff, &context);
	lnSpace = context.staffHeight / (context.staffLines-1);
	GetInsRect(doc, pL, &tRect);

	firstLoop = TRUE;
	ArrowCursor();
	PenMode(patXor);								/* Everything will be drawn twice */
	Rect2Window(doc, &tRect);
	Pt2Window(doc, &pt);
	oldPt = newPt = pt;
	
	if (StillDown())
		while (WaitMouseUp()) {
			GetMouse(&newPt);
			if (newPt.h!=pt.h && PtInRect(newPt, &tRect)) {

			/* Erase old ending */
				if (firstLoop) firstLoop = FALSE;
				else
					IDrawEndingBracket(oldPt, pt, lnSpace);
				pt = newPt;
			/* Draw new ending; constrain to horizontal movement. Save endv for
				crossSystem endings. */
				endv = pt.v;
				pt.v = oldPt.v; 
				IDrawEndingBracket(oldPt, pt, lnSpace);
			}
		}
	
	*endPt = pt;
	endPt->v = endv;	
	
	PenNormal();

	if (ABS(newPt.h-oldPt.h)<ENDING_MINLEN) return FALSE;
	else if (!PtInRect(newPt, &tRect)) return FALSE;	/* Mouse must still be in System */
	else return TRUE;
}

/* Track the ending and add it to the data structure. Handles cancelling. */
 
Boolean TrackAndAddEnding(Document *doc, Point pt, short clickStaff)
{
	static short number=1;
	static short cutoffs=3;
	short newNumber, newCutoffs, pitchLev,firstStf,lastStf;
	LINK insertL, lastL; Point newPt, findPt;
	Boolean crossSys;

	insertL = doc->selStartL;

	HiliteInsertNode(doc, insertL, clickStaff, TRUE);
	if (TrackEnding(doc, insertL, pt, &newPt, clickStaff)) {
		Pt2Paper(doc, &newPt);
		crossSys = GetCrossStatus(doc, pt, newPt, &firstStf, &lastStf);
		if (crossSys) {
			GetIndCString(strBuf, INSERTERRS_STRS, 10);    /* "An ending can't start & end on different systems." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			InvalMeasures(doc->selStartL, doc->selStartL, clickStaff);
			return FALSE;
		}

		pitchLev = -config.endingHeight;
		
		/* Don't allow drawing from right to left. */
		if (newPt.h>pt.h) findPt = newPt;
		else					findPt = pt;
		lastL = FindLPI(doc, findPt, clickStaff, ANYONE, TRUE);

		HiliteInsertNode(doc, lastL, clickStaff, TRUE);
		SleepTicks(HILITE_TICKS);

		if (!EndingDialog(number, &newNumber, cutoffs, &newCutoffs)) {
			InvalSystem(insertL);												/* Redraw the system to erase feedback */
			return FALSE;
		}
		
		number = newNumber;
		cutoffs = newCutoffs;

		if (newPt.h > pt.h) {
			doc->selEndL = doc->selStartL = insertL;
			NewEnding(doc,pt.h,newPt.h,palChar,clickStaff,pitchLev,lastL,number,cutoffs);
		}
		else {
			doc->selStartL = doc->selEndL = lastL;
			lastL = insertL;
			NewEnding(doc,newPt.h,pt.h,palChar,clickStaff,pitchLev,lastL,number,cutoffs);
		}
		return TRUE;
	}
	else {																		/* Insert cancelled. */
		InvalSystem(insertL);												/* Redraw the system to erase feedback */
		return FALSE;
	}
}

/* ----------------------------------------------------------------------- Lines -- */

static void IDrawLine(Point downPt, Point pt);
static Boolean TrackLine(Document *, LINK, Point, Point *, short);

/* Draw the line during the dragging loop. */

static void IDrawLine(Point downPt,		/* Original mousedown point */
								Point curPt)	/* Current point */
{	
	MoveTo(downPt.h,downPt.v);
	LineTo(curPt.h,curPt.v);
}

/* Track mouse dragging out a line and give feedback. */

#define LINE_MINLEN 2		/* in pixels */

static Boolean TrackLine(Document *doc, LINK pL, Point pt, Point *endPt, short staff)
{
	Point	origPt, newPt, constrainPt;
	CONTEXT context; Rect tRect;
	Boolean firstTime = TRUE, stillWithinSlop, horiz, vert;

	GetContext(doc, pL, staff, &context);
	GetInsRect(doc, pL, &tRect);

	ArrowCursor();
	PenMode(patXor);												/* Everything will be drawn twice */
	Rect2Window(doc, &tRect);
	Pt2Window(doc, &pt);
	origPt = newPt = pt;
	stillWithinSlop = TRUE;
	horiz = vert = TRUE;
	
	if (StillDown())
		while (WaitMouseUp()) {
			GetMouse(&newPt);
			/* If mouse outside insertion rect or hasn't moved, don't do anything. */
			
			constrainPt = newPt;
			if (!horiz) constrainPt.h = pt.h;
			if (!vert) constrainPt.v = pt.v;
			if (PtInRect(newPt, &tRect) && (constrainPt.h!=pt.h || constrainPt.v!=pt.v)) {
			if (!horiz) newPt.h = pt.h;
			if (!vert) newPt.v = pt.v;
					
			if (firstTime)
				firstTime = FALSE;
			else
				IDrawLine(origPt, pt);							/* erase old line */
	
			IDrawLine(origPt, newPt);							/* draw new line */
			pt = newPt;			
			}
		}
	
	*endPt = pt;
	
	PenNormal();

	/* Keep the line only if it's at least 2 pixels long and mouse is still in System. */
	if (ABS(newPt.h-origPt.h)<LINE_MINLEN && ABS(newPt.v-origPt.v)<LINE_MINLEN)
		return FALSE;
	else if (!PtInRect(newPt, &tRect)) return FALSE;
	else return TRUE;
}


/* Given <pt>, find the Sync and Note/Rest or grace Sync and grace note it's in, if any.
Return as function value the Sync/grace Sync, and in *paNoteL the note/rest/grace note.
Return NILINK if it's not in any object, or if it's in an object but not a Sync or grace
Sync. */

LINK InsFindNRG(Document *doc, Point pt, LINK *paNoteL);
LINK InsFindNRG(Document *doc, Point pt, LINK *paNoteL)
{
	LINK syncL, aNoteL; short index; short i; Boolean isSync;
	
	syncL = FindObject(doc, pt, &index, SMFind);
	if (!syncL) return NILINK;

	if (!SyncTYPE(syncL) && !GRSyncTYPE(syncL)) return NILINK;
	isSync = SyncTYPE(syncL);

	/* Find the note/rest/grace note subobject by index. */
	aNoteL = FirstSubLINK(syncL);
	for (i = 0; aNoteL; i++, aNoteL = (isSync? NextNOTEL(aNoteL) : NextGRNOTEL(aNoteL)))
		if (i==index) break;

	if (!aNoteL) {
		MayErrMsg("InsFindNRG: note %ld in Sync/GRSync at %ld not found", (long)index, (long)syncL);
		return NILINK;
	}
	
	*paNoteL = aNoteL;
	return syncL;
}


/* Get note, rest, or grace note half-space position. */

short GetNRGHalfSpaces(LINK syncL, LINK noteL, CONTEXT context);
short GetNRGHalfSpaces(
						LINK syncL,
						LINK noteL,
						CONTEXT context)
{
	DDIST yd; short halfSpace;

	if (SyncTYPE(syncL))
		yd = NoteYD(noteL);
	else
		yd = GRNoteYD(noteL);
	halfSpace = d2halfLn(yd, context.staffHeight, context.staffLines);
	
	return halfSpace;
}

/* Track the line and add it to the object list. Handles cancelling. */
 
Boolean TrackAndAddLine(Document *doc, Point pt, short clickStaff, short voice)
{	
	short firstStf,lastStf, firstHalfLn, lastHalfLn;
	LINK insertL, lastL, syncL, aNoteL;
	Point newPt, tempPt;
	Boolean crossSys; CONTEXT context;

	insertL = doc->selStartL;

	HiliteInsertNode(doc, insertL, clickStaff, TRUE);
	if (!TrackLine(doc, insertL, pt, &newPt, clickStaff)) goto Nope;

	Pt2Paper(doc, &newPt);
	crossSys = GetCrossStatus(doc, pt, newPt, &firstStf, &lastStf);
	if (crossSys) {
		GetIndCString(strBuf, INSERTERRS_STRS, 11);    /* "A line can't start & end on different systems." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		InvalMeasures(doc->selStartL, doc->selStartL, clickStaff);
		return FALSE;
	}

	lastL = FindLPI(doc, newPt, clickStaff, ANYONE, TRUE);
	if ( !(SyncTYPE(insertL) || GRSyncTYPE(insertL))
	||   !(SyncTYPE(lastL) || GRSyncTYPE(lastL)) ) {
		GetIndCString(strBuf, INSERTERRS_STRS, 14);    /* "Lines can be attached only to notes, rests, or grace notes." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		goto Nope;
	}

	/*
	 * Determine if the line was drawn backwards or forwards, and set the first
	 * and last attachment links and endpoints accordingly.
	 */
	if (IsAfter(lastL, insertL)) {
		insertL = lastL;
		lastL = doc->selStartL;						/* Can't have changed during this function */
		tempPt = newPt;
		newPt = pt;
		pt = tempPt;
	}

	HiliteInsertNode(doc, lastL, clickStaff, TRUE);
	SleepTicks(HILITE_TICKS);

	doc->selEndL = doc->selStartL = insertL;
	GetContext(doc, insertL, clickStaff, &context);

	/* Following code assumes both endpoints are Syncs or GRSyncs. */
	
	syncL = InsFindNRG(doc, pt, &aNoteL);
	if (!syncL) goto Nope;												/* Should never happen */
	firstHalfLn = GetNRGHalfSpaces(syncL, aNoteL, context);

	syncL = InsFindNRG(doc, newPt, &aNoteL);
	if (!syncL) goto Nope;												/* Can happen: need err msg? */
	lastHalfLn = GetNRGHalfSpaces(syncL, aNoteL, context);

	NewLine(doc,firstHalfLn,lastHalfLn,palChar,clickStaff,voice,lastL);
	return TRUE;

Nope:
	InvalSystem(insertL);												/* Redraw the system to erase feedback */
	return FALSE;
}


/* -------------------------------------------------------------------- FindSync -- */
/* Find a Sync very close to given point, or return NULL.  If !isGraphic, the Sync
must already have a note/rest on the given staff. */

LINK FindSync(Document *doc, Point pt,
					Boolean isGraphic,			/* whether graphic or temporal insert */
					short clickStaff
					)
{
	register LINK addToSyncL;		/* sync to which to add note/rest */
	LINK			lSyncL, rSyncL,	/* Links to left & right syncs */
					aNoteL, lNoteL, rNoteL;
	Rect			noteRect;
	short			lDist, rDist,		/* Pixel distance from ins. pt to rSync, lSync */	
					noteWidth,			/* 2*amount by which to shift lDist,rDist */
					lSyncPos,
					oneWayLimit,		/* Note position tolerance if only one side is possible */
					limit;				/* Note position tolerance if left & right both possible */
	Point			ptHLeft;				/* The point to left of <pt> by 1/2 noteWidth */ 

	/*
	 *	Compute distances from horizontal position where user clicked to the
	 *	centers of the nearest Syncs to left and right (if there are any), and
	 *	look for notes in those Syncs on the desired staff.
	 */
	noteRect = CharRect(MCH_halfNoteHead);
	noteWidth = noteRect.right-noteRect.left+1;
	ptHLeft = pt;
	ptHLeft.h -= noteWidth/2;
	rSyncL = FindSyncRight(doc, ptHLeft, FALSE);
	lSyncL = FindSyncLeft(doc, ptHLeft, FALSE);
	
	rNoteL = NILINK;
	if (rSyncL) {
		rDist = ABS(pt.h-(LinkOBJRECT(rSyncL).left+noteWidth/2));
		aNoteL = FirstSubLINK(rSyncL);						/* Does Sync already have note/rest on this staff? */
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==clickStaff) {
				rNoteL = aNoteL; break;							/* Yes */
			}
	}
	else
		rDist = -1;
		
	lNoteL = NILINK;
	if (lSyncL) {
		/*
		 *	We're using the leftmost point on the existing Sync to compute distance.
		 *	If the Sync to the left includes notes on the "wrong" side of the stem,
		 *	we check the distance to notes on each side and use whichever is closer.
		 *	(We don't have to do this for the Sync to the right because any notes to the
		 *	right of its stem will be even further from the click point.) This handles
		 * the two positions notes have in Syncs with either upstemmed or downstemmed
		 *	chords with seconds, but it doesn't handle the THREE positions notes have in
		 *	Syncs that contain both! This should be fixed some day.
		 */
		lSyncPos = LinkOBJRECT(lSyncL).left; 
		lDist = ABS(pt.h-(lSyncPos+noteWidth/2));
		if (HasOtherStemSide(lSyncL, ANYONE)) lSyncPos += noteWidth;
		lDist = n_min(lDist, ABS(pt.h-(lSyncPos+noteWidth/2)));
		aNoteL = FirstSubLINK(lSyncL);						/* Does Sync already have note/rest on this staff? */
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteSTAFF(aNoteL)==clickStaff) {
				lNoteL = aNoteL; break;							/* Yes */
			}
	}
	else
		lDist = -1;

	oneWayLimit = config.maxSyncTolerance*noteWidth/100;
	
	if (isGraphic) {
	/*
	 *	In graphic mode, if the click point is relatively close to the nearest Sync
	 *	either to left or right but not both, choose that Sync. If it's close to
	 *	both but only one already has a note on this staff, choose that one.
	 *	Otherwise return NULL.
	 */
		if (lDist>=0 && rDist>=0) {
			limit = (lDist+rDist)/3;
			if (limit>oneWayLimit) limit = oneWayLimit;
			if (lDist>limit && rDist>limit) addToSyncL = NILINK;
			else if (lDist>limit) addToSyncL = rSyncL;
			else if (rDist>limit) addToSyncL = lSyncL;
			else {
				if (!lNoteL) addToSyncL = rSyncL;
				else			 addToSyncL = (lDist<=rDist) ? lSyncL : rSyncL;
			}
		}
		else if (lDist>=0) {
			if (lDist>oneWayLimit) addToSyncL = NILINK;
			else						  addToSyncL = lSyncL;
		}
		else if (rDist>=0) {
			if (rDist>oneWayLimit) addToSyncL = NILINK;
			else						  addToSyncL = rSyncL;
		}
		else addToSyncL = NILINK;
	}
	else {																/* Temporal mode */
	/*
	 *	In non-graphic mode:
	 *	If nearest Syncs exist and have notes for this staff in both directions:
	 *		If neither is close to the click point, return NILINK.
	 *		If only one is close to the click point, return that one.
	 *		If both are close to the click point, return whichever is closer.
	 *	Else, if a nearest Sync exists with a note for this staff in one direction,
	 *		if it's close to the click point, return that Sync.
	 *	Else return NILINK.
	 *	If there are notes in both directions, "close" is n_min(limit, oneWayLimit), so
	 *	it's generally smaller than when there's a note in only one direction, where
	 *	"close" is just <oneWayLimit>.
	 */
		if ((lDist>=0 && lNoteL) && (rDist>=0 && rNoteL)) {
			limit = (lDist+rDist)/3;
			if (limit>oneWayLimit) limit = oneWayLimit;
			if (lDist>limit && rDist>limit) addToSyncL = NILINK;
			else if (lDist>limit) addToSyncL = rSyncL;
			else if (rDist>limit) addToSyncL = lSyncL;
			else						 addToSyncL = (lDist<=rDist) ? lSyncL : rSyncL;
		}
		else if (lDist>=0 && lNoteL) {
			if (lDist>oneWayLimit) addToSyncL = NILINK;
			else						  addToSyncL = lSyncL;
		}
		else if (rDist>=0 && rNoteL) {
			if (rDist>oneWayLimit) addToSyncL = NILINK;
			else						  addToSyncL = rSyncL;
		}
		else addToSyncL = NILINK;
	}
	return addToSyncL;	
}


/* ---------------------------------------------------------- AddCheckVoiceTable -- */

Boolean AddCheckVoiceTable(Document *doc, short staff)
{
	short	voice;
	char fmtStr[256];
	
	voice = USEVOICE(doc, staff);
	if (doc->voiceTab[voice].partn
	&&  Staff2Part(doc, staff)!=doc->voiceTab[voice].partn) {
		GetIndCString(fmtStr, INSERTERRS_STRS, 23);    /* "To insert in this part, you must Look at one of its voices or Look at All Voices." */
		sprintf(strBuf, fmtStr, voice); 
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return FALSE;
	}
	return TRUE;
}


/* --------------------------------------------------------------- AddNoteCheck -- */
/*	Check whether it's OK to add a note/rest in the current voice and the given
staff to the given sync, and deliver TRUE if it is OK. (The insertion user interface
distinguishes chord slashes, but they're really just funny notes.) */

Boolean AddNoteCheck(Document *doc,
							LINK		addToSyncL,		/* Sync to which to add note/rest */
							short		staff,
							short		symIndex			/* Symbol table index of note/rest to be inserted */
							)
{
	short		voice, userVoice;
	LINK		aNoteL, partL;
	Boolean	isRest, isChordSlash, okay=TRUE;
	char		fmtStr[256];
	
	voice = USEVOICE(doc, staff);
	if (addToSyncL==NILINK) { SysBeep(30);	return FALSE; }		/* No sync to check */
	
	isRest = (symtable[symIndex].subtype==1);
	isChordSlash = (symtable[symIndex].subtype==2);

	aNoteL = FirstSubLINK(addToSyncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			if (!Int2UserVoice(doc, voice, &userVoice, &partL))
					userVoice = -1;
					
			/* The error conditions result from Nightingale's inability to handle
			 *	a voice in a Sync on more than one staff, and our refusal to allow
			 * a voice in a Sync to have a rest and anything else.
			 */
			if (NoteSTAFF(aNoteL)!=staff) {		
				GetIndCString(fmtStr, INSERTERRS_STRS, 24);    /* "Can't insert in voice %d here; already has something on staff %d." */
				sprintf(strBuf, fmtStr, userVoice, NoteSTAFF(aNoteL)); 
				okay = FALSE;
			}
			else if (isRest) {
				GetIndCString(fmtStr, INSERTERRS_STRS, 25);    /* "Can't insert a rest in voice %d here; already has..." */
				sprintf(strBuf, fmtStr, userVoice); 
				okay = FALSE;
			}
			else if (isChordSlash) {
				GetIndCString(fmtStr, INSERTERRS_STRS, 26);    /* "Can't insert a chord slash in voice %d here; already has..." */
				sprintf(strBuf, fmtStr, userVoice); 
				okay = FALSE;
			}
			else if (NoteREST(aNoteL)) {
				GetIndCString(fmtStr, INSERTERRS_STRS, 27);    /* "Can't insert anything in voice %d here; already has a rest." */
				sprintf(strBuf, fmtStr, userVoice); 
				okay = FALSE;
			}
		}
	}
	
	if (!okay) {
		HiliteInsertNode(doc, addToSyncL, staff, TRUE);
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
	}
	return okay;
}

/* ------------------------------------------------------------------- FindGRSync -- */
/* Find a GRSync very close to given point, or return NULL.  If !isGraphic, the
GRSync must already have a grace note on the given staff. */

LINK FindGRSync(Document *doc, Point pt,
					Boolean isGraphic,			/* whether graphic or temporal insert */
					short clickStaff
					)
{
	LINK			addToGRSyncL,			/* GRSync to which to add note/rest */
					lGRSyncL, rGRSyncL,	/* Links to left & right GRSyncs */
					aNoteL, lNoteL, rNoteL;
	Rect			noteRect;
	short			lDist, rDist,		/* Pixel distance from ins. pt to rSync, lSync */	
					noteWidth,			/* 2*amount by which to shift lDist,rDist */
					oneWayLimit,		/* Note position tolerance if only one side is possible */
					limit;				/* Note position tolerance if left & right both possible */
	Point			ptHLeft;				/* The point to left of <pt> by 1/2 noteWidth */ 

	/*
	 *	Compute distances from horizontal position where user clicked to the
	 *	centers of the nearest GRSyncs to left and right (if there are any), and
	 *	look for grace notes in those GRSyncs on the desired staff.
	 */
	noteRect = CharRect(MCH_halfNoteHead);
	noteWidth = noteRect.right-noteRect.left+1;
	ptHLeft = pt;
	ptHLeft.h -= noteWidth/2;
	rGRSyncL = FindGRSyncRight(doc, ptHLeft, FALSE);
	lGRSyncL = FindGRSyncLeft(doc, ptHLeft, FALSE);
	
	rNoteL = NILINK;
	if (rGRSyncL) {
		rDist = ABS(pt.h-(LinkOBJRECT(rGRSyncL).left+noteWidth/2));
		aNoteL = FirstSubLINK(rGRSyncL);						/* Does Sync already have note on this staff? */
		for ( ; aNoteL; aNoteL = NextGRNOTEL(aNoteL))
			if (GRNoteSTAFF(aNoteL)==clickStaff) {
				rNoteL = aNoteL; break;							/* Yes */
			}
	}
	else
		rDist = -1;
		
	lNoteL = NILINK;
	if (lGRSyncL) {
		lDist = ABS(pt.h-(LinkOBJRECT(lGRSyncL).left+noteWidth/2));
		aNoteL = FirstSubLINK(lGRSyncL);						/* Does Sync already have note on this staff? */
		for ( ; aNoteL; aNoteL = NextGRNOTEL(aNoteL))
			if (GRNoteSTAFF(aNoteL)==clickStaff) {
				lNoteL = aNoteL; break;							/* Yes */
			}
	}
	else
		lDist = -1;
		
	oneWayLimit = config.maxSyncTolerance*noteWidth/100;
	oneWayLimit = GRACESIZE(oneWayLimit);
	
	if (isGraphic) {
	/*
	 *	In graphic mode, if the click point is relatively close to the nearest GRSync
	 *	either to left or right but not both, choose that GRSync. If it's close to
	 *	both but only one already has a note on this staff, choose that one.
	 *	Otherwise return NULL.
	 */
		if (lDist>=0 && rDist>=0) {
			limit = (lDist+rDist)/3;
			if (limit>oneWayLimit) limit = oneWayLimit;
			if (lDist>limit && rDist>limit) addToGRSyncL = NILINK;
			else if (lDist>limit) addToGRSyncL = rGRSyncL;
			else if (rDist>limit) addToGRSyncL = lGRSyncL;
			else {
				if (!lNoteL) addToGRSyncL = rGRSyncL;
				else			 addToGRSyncL = (lDist<=rDist) ? lGRSyncL : rGRSyncL;
			}
		}
		else if (lDist>=0) {
			if (lDist>oneWayLimit) addToGRSyncL = NILINK;
			else						  addToGRSyncL = lGRSyncL;
		}
		else if (rDist>=0) {
			if (rDist>oneWayLimit) addToGRSyncL = NILINK;
			else						  addToGRSyncL = rGRSyncL;
		}
		else addToGRSyncL = NILINK;
	}
	else {																/* Temporal mode */
	/*
	 *	In non-graphic mode, if the click point is relatively close to the nearest
	 *	GRSync either to left or right but not both AND that GRSync already has a note
	 *	on this staff, choose that GRSync (e.g., to make a chord on this staff). If
	 *	it's close to both of the nearest GRSyncs but only one already has a note on
	 *	this staff, choose that one (sort of). Otherwise return NULL.
	 */
		if ((lDist>=0 && lNoteL) && (rDist>=0 && rNoteL)) {
			limit = (lDist+rDist)/3;
			if (limit>oneWayLimit) limit = oneWayLimit;
			if (lDist>limit && rDist>limit) addToGRSyncL = NILINK;
			else if (lDist>limit) addToGRSyncL = rGRSyncL;
			else if (rDist>limit) addToGRSyncL = lGRSyncL;
			else						 addToGRSyncL = (lDist<=rDist) ? lGRSyncL : rGRSyncL;
		}
		else if (lDist>=0 && lNoteL) {
			if (lDist>oneWayLimit) addToGRSyncL = NILINK;
			else						  addToGRSyncL = lGRSyncL;
		}
		else if (rDist>=0 && rNoteL) {
			if (rDist>oneWayLimit) addToGRSyncL = NILINK;
			else						  addToGRSyncL = rGRSyncL;
		}
		else addToGRSyncL = NILINK;
	}
	return addToGRSyncL;	
}


/* -------------------------------------------------------------- AddGRNoteCheck -- */
/* Check whether it's OK to add a grace note in the current voice and the given
staff to the given GRSync. */

Boolean AddGRNoteCheck(
				Document *doc,
				LINK		addToGRSyncL,	/* GRSync to add grace note to */
				short		staff
				)
{
	short		voice, userVoice;
	LINK		aNoteL, partL;
	char		fmtStr[256];
	
	voice = USEVOICE(doc, staff);
	aNoteL = FirstSubLINK(addToGRSyncL);
	for ( ; aNoteL; aNoteL = NextGRNOTEL(aNoteL)) {
		if (GRNoteVOICE(aNoteL)==voice) {
			if (GRNoteSTAFF(aNoteL)!=staff) {						/* Is new symbol on different staff? */
				HiliteInsertNode(doc, addToGRSyncL, staff, TRUE);
				if (!Int2UserVoice(doc, voice, &userVoice, &partL))
							userVoice = -1;
				GetIndCString(fmtStr, INSERTERRS_STRS, 28);		/* "Can't insert in voice %d here: that voice already has something on staff %d." */
				sprintf(strBuf, fmtStr, voice, GRNoteSTAFF(aNoteL)); 
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return FALSE;
			}
		}
	}
	return TRUE;
}

/* --------------------------------------------------------------- FindSyncRight -- */
/* Returns the first sync whose objRect.left is to the right of pt.h */

LINK FindSyncRight(Document *doc, Point pt, Boolean needVisible)
{
	LINK pL;
	
	pL = FindSymRight(doc, pt, needVisible, FALSE);
	for ( ; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (SyncTYPE(pL)) return pL;
		else if (SystemTYPE(pL) || PageTYPE(pL)) 
					return NILINK;
	}
	return NILINK;
}


/* ---------------------------------------------------------------- FindSyncLeft -- */
/* Returns the first sync whose objRect.left is to the left of pt.h */

LINK FindSyncLeft(Document *doc, Point pt, Boolean needVisible)
{
	LINK pL;
	
	pL = FindSymRight(doc, pt, needVisible, FALSE);
	for (pL = LeftLINK(pL); pL!=doc->headL; pL = LeftLINK(pL)) {
		if (SyncTYPE(pL)) return pL;
		else if (SystemTYPE(pL) || PageTYPE(pL)) return NILINK;
	}
	return NILINK;
}


/* ------------------------------------------------------------- FindGRSyncRight -- */
/* Returns the first GRSync whose objRect.left is to the right of pt.h */

LINK FindGRSyncRight(Document *doc, Point pt, Boolean needVisible)
{
	LINK pL;
	
	pL = FindSymRight(doc, pt, needVisible, FALSE);
	for ( ; pL!=doc->tailL; pL = RightLINK(pL)) {
		if (GRSyncTYPE(pL)) return pL;
		else if (SystemTYPE(pL) || PageTYPE(pL)) return NILINK;
	}
	return NILINK;
}


/* -------------------------------------------------------------- FindGRSyncLeft -- */
/* Returns the first GRSync whose objRect.left is to the left of pt.h */

LINK FindGRSyncLeft(Document *doc, Point pt, Boolean needVisible)
{
	LINK pL;
	
	pL = FindSymRight(doc, pt, needVisible, FALSE);
	for (pL = LeftLINK(pL); pL!=doc->headL; pL = LeftLINK(pL)) {
		if (GRSyncTYPE(pL)) return pL;
		else if (SystemTYPE(pL) || PageTYPE(pL)) return NILINK;
	}
	return NILINK;
}


/* ---------------------------------------------------------------- FindSymRight -- */
/* Returns first node on doc->currentSystem whose objRect.left is to the right of
pt.h.

NB: SetCurrentSystem sets doc->currentSystem as a side effect. If we do not wish
to do this, and assume doc->currentSystem is already properly set, can call
GetCurrentSystem.

This will not find page-relative graphics, which we do not want anyway. */

LINK FindSymRight(Document *doc, Point pt,
						Boolean needVisible,
						Boolean useJD			/* If TRUE, find JD symbols, else ignore them */
						)
{
	register LINK	pL,resultL,endL;

	pL = SetCurrentSystem(doc, pt);						/* sets doc->currentSystem */
	if (!pL) return doc->tailL;
	resultL = endL = EndSystemSearch(doc,pL);			/* assume nothing found */

	/* Traverse until we reach the obj ending the system or find a symbol to right
		of pt.h. */
	
	for ( ; pL!=endL && resultL==endL; pL=RightLINK(pL)) {
		if (!needVisible || LinkVIS(pL))
			if (useJD || objTable[ObjLType(pL)].justType!=J_D)
				if (LinkOBJRECT(pL).left > pt.h)
					resultL = pL;
	}

	return resultL;
}	


/* --------------------------------------------------------------------- FindLPI -- */
	
/* Find the last item on <staff> prior to <pt>, where needVisible indicates 
whether the first symbol to the right of <pt> needs to be visible in order to be
used in a search to the right used by the code. */

LINK FindLPI(Document *doc,
				Point		pt,
				short		staff,			/* staff desired or ANYONE */
				short		voice,			/* voice desired or ANYONE */
				Boolean 	needVisible 	/* Must the FOLLOWING item on any staff be visible? */
				)
{
	PMEVENT		p;
	LINK			pL, subObjL,
					nodeL, pLPIL, aNoteL, aGRNoteL;
	Boolean		anyStaff, anyVoice;
	HEAP 			*tmpHeap;
	GenSubObj	*subObj;

	anyStaff = (staff==ANYONE);
	anyVoice = (voice==ANYONE);

	/* Search right for the next <needVisible> node to the right of <pt>; then
		traverse left to find the next node to the left on <staff>. */
	nodeL = FindSymRight(doc, pt, needVisible, FALSE);
	if (nodeL==NILINK) {
		MayErrMsg("FindLPI: FindSymRight returned NILINK.");
		return NILINK;
	}

	pLPIL = NILINK;
	for (pL = LeftLINK(nodeL); pLPIL==NILINK; pL = LeftLINK(pL)) {			/* find LPI */
		switch (ObjLType(pL)) {
			case HEADERtype:
			case PAGEtype:
			case SYSTEMtype:
			case STAFFtype:
			case SPACEtype:		/* Behave as if they're on all staves */
				pLPIL = pL;
				break;
			case SYNCtype:
				for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					if (anyStaff || NoteSTAFF(aNoteL)==staff)
						if (anyVoice || NoteVOICE(aNoteL)==voice)
							pLPIL = pL;
				}
				break;
			case GRSYNCtype:
				for (aGRNoteL=FirstSubLINK(pL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
					if (anyStaff || GRNoteSTAFF(aGRNoteL)==staff)
						if (anyVoice || GRNoteVOICE(aGRNoteL)==voice)
							pLPIL = pL;
				}
				break;
			case TIMESIGtype:
			case MEASUREtype:
			case PSMEAStype:
			case CLEFtype:
			case KEYSIGtype:
			case RPTENDtype:
				tmpHeap = Heap + ObjLType(pL);		/* p may not stay valid during loop */
				
				for (subObjL = FirstSubObjPtr(p,pL); subObjL;
						subObjL = NextLink(tmpHeap,subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
					if (anyStaff || subObj->staffn==staff)			/* on desired staff? */
						pLPIL = pL;
				}
				break;
			default:
				if (TYPE_BAD(pL))
					MayErrMsg("FindLPI: object at %ld has illegal type %ld",
								(long)pL, (long)ObjLType(pL));
		}			
	}
	return pLPIL;
}

/* ---------------------------------------------------------------- ObjAtEndTime -- */
/* Return the object in staff or voice <id> which ends at the exact end time of or
after the end time of pL, depending on useV and exact.
GetLTime is called once by this function and multiple times by TimeSearchRight;
each call creates an spTimeInfo array, calls GetSpTimeInfo and disposes the 
array. An optimization would do this only once or a mininum number of times. */ 

LINK ObjAtEndTime(
					Document 	*doc,
					LINK 			pL,
					short 		type,		/* objType (e.g. CLEFtype) or ANYTYPE */
					short 		id,		/* clickStaff or voice */
					long			*lTime,	/* LTime of obj */
					Boolean		exact,	/* EXACT_TIME or MIN_TIME */
					Boolean		useV		/* id is staff or voice */
					)
{
	LINK insNodeL=NILINK;

	*lTime = GetLTime(doc,pL) +							/* Get time for new item */
					(useV ? GetVLDur(doc,pL,id) : GetLDur(doc,pL,id));
	insNodeL = TimeSearchRight(doc, RightLINK(pL), type, *lTime, exact);
	
	return insNodeL;
}


/* -------------------------------------------------------------- LocateInsertPt -- */
/* Locate the insertion point corresponding to pL's slot in the data structure. The
slot consists of all J_D objects dependent on a J_IP or J_IT object terminating the
slot, plus the J_IP/J_IT symbol; the insertion point corresponding to the slot is
before the first J_D symbol of the slot, and the first J_D symbol of the slot is
returned to mark that insertion point. (Second pieces of cross-system slurs are not
part of their ending note's slot: if they were, we might insert objects between the
initial measure and the cross-system slur, which seems dangerous.) */

LINK LocateInsertPt(LINK pL)
{
	while (J_DTYPE(LeftLINK(pL))) {
		if (SlurTYPE(LeftLINK(pL)) && SlurLastSYSTEM(LeftLINK(pL))) return pL;
		pL = LeftLINK(pL);
	}
	
	return pL;
}


/* ----------------------------------------------------------------- GetPitchLim -- */
/* Given a staff subobject, deliver its extreme usable pitch level, in half-spaces
relative to the top of the staff (negative values are going up). That pitch level
is the most restrictive of:
	the distance to the top/bottom of the systemRect;
	the distance to the FAR edge of the staff above or below (if there is one); and
	MAX_LEDGERS ledger lines.
Note that this does NOT guarantee legal MIDI note numbers, which depends on the clef
and possibly octave sign. Having at most MAX_LEDGERS ledger lines is necessary but
not sufficient for a note to be a legal MIDI note. */

short GetPitchLim(Document *doc, LINK staffL,
						short staffn,
						Boolean above			/* TRUE=want limit above staff, FALSE=below staff */
						)
{
	register LINK aStaffL, bStaffL;
	LINK sysL;
	register PASTAFF aStaff, bStaff;
	DRect dr; DDIST dAway, aStaffBottom;
	short staffAbove, staffBelow, limit;
	CONTEXT context;
	
	sysL = SSearch(staffL, SYSTEMtype, GO_LEFT);
	dr = SystemRECT(sysL);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==staffn) break;
	if (!aStaffL) MayErrMsg("GetPitchLim: couldn't find staff %ld in staff obj at %ld",
									(long)staffn, (long)staffL);
	aStaff = GetPASTAFF(aStaffL);
	GetContext(doc, staffL, staffn, &context);
	if (above) {
		dAway = aStaff->staffTop;									/* Dist. to systemRect top */
		staffAbove = NextStaffn(doc, staffL, FALSE, staffn-1);
		if (staffAbove) {
			bStaffL = FirstSubLINK(staffL);
			for ( ; bStaffL; bStaffL = NextSTAFFL(bStaffL))
				if (StaffSTAFF(bStaffL)==staffAbove) break;
			bStaff = GetPASTAFF(bStaffL);
			dAway = aStaff->staffTop-bStaff->staffTop;
		}
		limit = dAway/(LNSPACE(&context)/2)-1;
		return -(n_min(limit, MAX_LEDGERS*2));
	}
	else {
		aStaffBottom = aStaff->staffTop+aStaff->staffHeight;
		dAway = dr.bottom-(dr.top+aStaffBottom);				/* Dist. to systemRect bottom */
		staffBelow = NextStaffn(doc, staffL, TRUE, staffn+1);
		if (staffBelow) {
			bStaffL = FirstSubLINK(staffL);
			for ( ; bStaffL; bStaffL = NextSTAFFL(bStaffL))
				if (StaffSTAFF(bStaffL)==staffBelow) break;
			bStaff = GetPASTAFF(bStaffL);
			dAway = bStaff->staffTop-aStaff->staffTop;
		}
		limit = dAway/(LNSPACE(&context)/2)-1;
		return (n_min(limit, MAX_LEDGERS*2)+2*(context.staffLines-1));
	}
}


/* -------------------------------------------------------------- CalcPitchLevel -- */
/*	For a given cursor y-position, returns pitch level (vertical position)
in half-lines, relative to the staff top line, negative values going up. */

/* Describe usable height of (most) cursors we want to track. */
#define ABOVE_HOTSPOT 2	/* Pixels above hotspot that can intersect staff line */
#define BELOW_HOTSPOT 1	/* Pixels below hotspot that can intersect staff line */

short CalcPitchLevel(
					short		hotY,				/* mouse hotspot y (local coordinates) */
					PCONTEXT	pContext,		/* ptr to current context */
					LINK		staffL,			/* staff object containing original hotspot y */
					short		staffn
					)
{
	register short lineSp,			/* space between staff lines (pixels) */
					lines;
	short			halfLns,					/* approximate half-line number */
					relHotY,					/* distance from staff top down to hotY (pixels) */
					headHotY,				/* distances below hiPitchLim point */
					headTop,
					headBottom,
					hiPitchLim, lowPitchLim;
	Document 	*doc=GetDocumentFromWindow(TopDocument);
	
	if (doc==NULL) return 0;
	
	relHotY = hotY - d2p(pContext->staffTop);						/* pixels from staffTop */
	lineSp = d2px(LNSPACE(pContext));
	halfLns = (2*relHotY)/lineSp;

	hiPitchLim = GetPitchLim(doc, staffL, staffn, TRUE);
	lowPitchLim = GetPitchLim(doc, staffL, staffn, FALSE);
	if (halfLns<=hiPitchLim)			return hiPitchLim;
	else if (halfLns>=lowPitchLim)	return lowPitchLim;

/* If distance between staff lines is not large, i.e., no more than the height
 *	of most cursor noteheads, we're done.
 */
	if (lineSp<=4) return halfLns;

/* If distance between staff lines is greater than the height of most cursor
 *	noteheads, consider note to be on a line only if the cursor head touches the
 *	line. To do this, we must consider the number of pixels above and below the
 *	hotspot the cursor notehead extends: 2 above and 1 below for all but breves.
 *	Should really consider breves, but (as of v.998) no user's complained about it.
 */
	else {
		/* To simplify things, offset values so everything is positive and a coord.
		 * of 0 corresponds to a line, not a space.
		 */ 
		if (odd(hiPitchLim)) hiPitchLim -= 1;
		headHotY = relHotY-hiPitchLim*lineSp/2;
		headTop = headHotY-ABOVE_HOTSPOT;
		headBottom = headHotY+BELOW_HOTSPOT;
		lines = headHotY/lineSp;
		if ((lines*lineSp)>=headTop && (lines*lineSp)<=headBottom)
			return 2*lines+hiPitchLim;									/* On the line */
		else
			return 2*lines+hiPitchLim+1;								/* In space below */
	}
}	


/* ------------------------------------------------------------------- NextStaff -- */
/*	Return pointer to the next Staff or System object, or tailL if there
are none. N.B. With Nightingale's current (as of v.99) data structure, will
NEVER return a Staff, since the next System will always precede it. */

static LINK NextStaff(Document *doc, LINK pL)
{
	LINK		qL;

	if ((qL = LinkRSTAFF(pL)) == NILINK)
		return doc->tailL;							/* none; return tailL */
	if (StaffSYS(qL)==StaffSYS(pL))
		return qL;								/* same System; return next Staff */
	else
		return StaffSYS(qL);					/* different System; return next System */
}


/* -------------------------------------------------------------- FindStaff et al -- */
/* Return the staffn of the staff nearest to the given point, presumably a mouse
click location, or NOONE of the point isn't within any system.  Also sets global
doc->currentSystem. */

static DDIST GetTopStfLim(Document *doc, LINK pL, LINK aStaffL, PCONTEXT contextTbl)
{
	short staffAbove; PCONTEXT pContext,qContext;
	DDIST topLimit;
	
	pContext = &contextTbl[StaffSTAFF(aStaffL)];
	if (StaffSTAFF(aStaffL)==FirstStaffn(pL))
		return pContext->systemTop;

	staffAbove = NextStaffn(doc, pL, FALSE, StaffSTAFF(aStaffL)-1);
	qContext = &contextTbl[staffAbove];
	topLimit = ((long)qContext->staffTop +
						qContext->staffHeight +
						pContext->staffTop) / 2L;
						
	return topLimit;
}

static DDIST GetBotStfLim(Document *doc, LINK pL, LINK aStaffL, PCONTEXT contextTbl)
{
	short staffBelow; PCONTEXT pContext,qContext;
	DDIST bottomLimit;

	pContext = &contextTbl[StaffSTAFF(aStaffL)];
	if (StaffSTAFF(aStaffL)==LastStaffn(pL))
		return pContext->systemBottom;

	staffBelow = NextStaffn(doc, pL, TRUE, StaffSTAFF(aStaffL)+1);
	qContext = &contextTbl[staffBelow];
	bottomLimit = ((long)qContext->staffTop +
						pContext->staffTop +
						pContext->staffHeight) / 2L;
						
	return bottomLimit;
}

short FindStaff(Document *doc, Point pt)
{
	register LINK 		pL,aStaffL;
	register PCONTEXT pContext;
	Rect			r;
	PCONTEXT		contextTbl;
	DDIST			topLimit, bottomLimit;
	short			staffn=NOONE;

	contextTbl = AllocContext();
	if (!contextTbl) return NOONE;

	pL = SetCurrentSystem(doc,pt);
	if (!pL) return NOONE;
	
	pL = SSearch(pL,STAFFtype,GO_RIGHT);

	ContextSystem(StaffSYS(pL), contextTbl);
	ContextStaff(pL, contextTbl);

	aStaffL = FirstSubLINK(pL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		if (StaffVIS(aStaffL)) {

			topLimit = GetTopStfLim(doc,pL,aStaffL,contextTbl);
			bottomLimit = GetBotStfLim(doc,pL,aStaffL,contextTbl);

			pContext = &contextTbl[StaffSTAFF(aStaffL)];
			SetRect(&r, pContext->staffLeft, topLimit,
								pContext->staffRight, bottomLimit);

			D2Rect((DRect *)&r, &r);
			if (PtInRect(pt, &r)) {
				staffn = StaffSTAFF(aStaffL);
				goto done;
			}
		}
	}
	
done:
	DisposePtr((Ptr)contextTbl);
	return staffn;
}


/* ------------------------------------------------------------ SetCurrentSystem -- */
/* Set doc->currentSystem to be the number of the system containing pt, normally a
mouse click location, and returns that system. If pt is not within a system, leaves
doc->currentSystem alone and returns NILINK. Assumes doc->currentSheet is already
set correctly; may fail if pt is not in a system in doc->currentSheet. pt is
expected in page-relative coordinates. */

LINK SetCurrentSystem(Document *doc, Point pt)
{
	register LINK 	pL;
	Rect 				r;
	LINK 				pageL;

	pL = LSSearch(doc->headL, PAGEtype, doc->currentSheet, GO_RIGHT, FALSE);
	
	for ( ; pL && pL!=doc->tailL; ) {
		if (SystemTYPE(pL)) {
			pageL = SysPAGE(pL);
			if (SheetNUM(pageL)!=doc->currentSheet) return NILINK;

			D2Rect(&SystemRECT(pL), &r);
			if (LinkVIS(pL) && PtInRect(pt, &r)) {					/* click in this System */
				doc->currentSystem = SystemNUM(pL);
				return pL;
			}
			pL = LinkRSYS(pL); 											/* skip to next System */
		}
		else pL = RightLINK(pL);
	}
	
	return NILINK;
}

/* ------------------------------------------------------------ GetCurrentSystem -- */
/*	Get the system referenced by doc->currentSystem. */

LINK GetCurrentSystem(Document *doc)
{
	return LSSearch(doc->headL, SYSTEMtype, doc->currentSystem, GO_RIGHT, FALSE);
}

/* ------------------------------------------------------------- AddGRNoteUnbeam -- */
/* Deletes beamL, and traverses from startL to endL not inclusive, setting
aGRNote->beamed FALSE for all grace notes on <staff>. Unlike UnbeamRange,
AddGRNoteUnbeam does not FixSyncForChord, etc., so it leaves things in a
graphically inconsistent state; this is OK as long as calls to AddGRNoteUnbeam
are always followed by rebeaming startL thru endL. ??This function probably should
go away; calls should be replaced with calls to RemoveBeam or some such. */

static void AddGRNoteUnbeam(Document *doc, LINK beamL, LINK startL, LINK endL,
										short voice)
{
	PAGRNOTE	aGRNote;
	LINK 		pL, aGRNoteL;
	
	DeleteNode(doc, beamL);
	for (pL = startL; pL!=endL; pL=RightLINK(pL)) {
		if (GRSyncTYPE(pL)) {
			aGRNoteL = FirstSubLINK(pL);
			for ( ; aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
				aGRNote = GetPAGRNOTE(aGRNoteL); 
				if (GRNoteVOICE(aGRNoteL)==voice) aGRNote->beamed = FALSE;
			}
		}
	}
}

/* ------------------------------------------------------------- AddNoteFixBeams -- */
/* AddNoteFixBeams should be called ONLY if <newNoteL> is being inserted in the
middle of a beamset; it fixes things up appropriately. It assumes inChord flags are
set properly for notes in the sync in <newNoteL>'s voice. */ 

void AddNoteFixBeams(Document *doc,
							LINK syncL,			/* Sync the new note belongs to */
							LINK newNoteL		/* The new note */
							)
{
	short 		nBeamable, voice;
	LINK			firstSyncL, lastSyncL;
	LINK			aNoteL, beamL, farNoteL, newBeamL;
	PBEAMSET		pBeam, newBeam;
	DDIST			ystem, maxLength;
	Boolean		crossSys, firstSys;
	SearchParam	pbSearch;
	
	voice = NoteVOICE(newNoteL);
	
/* If new note is being inserted in the middle of a beamset, then either it's
 *	beamable: unbeam and rebeam to include it in the beamset; or it's not beamable:
 *	unbeam and rebeam on either side, if there are enough notes to do this. In the
 * first case, even if the new note is merely added to a chord in a beamset, for
 *	the moment we unbeam and rebeam because the new note may affects the vertical pos-
 *	of the beamset, & it's easier to unbeam & re-beam than recalculate ystem pos.
 */
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = FALSE;
	pbSearch.inSystem = FALSE;
	pbSearch.subtype = NoteBeam;
	beamL = L_Search(syncL, BEAMSETtype, TRUE, &pbSearch);
	pBeam = GetPBEAMSET(beamL);
	crossSys = pBeam->crossSystem;
	firstSys = pBeam->firstSystem;
	firstSyncL = FirstInBeam(beamL);
	lastSyncL = LastInBeam(beamL);
	RemoveBeam(doc, beamL, voice, TRUE);
	nBeamable = CountBeamable(doc, firstSyncL, RightLINK(lastSyncL), voice, FALSE);
	if (nBeamable>=2) {
		newBeamL = CreateBEAMSET(doc, firstSyncL, RightLINK(lastSyncL), voice,
										 nBeamable, FALSE, doc->voiceTab[voice].voiceRole);
		newBeam = GetPBEAMSET(newBeamL);
		newBeam->crossSystem = crossSys;
		newBeam->firstSystem = firstSys;
	}
	else {
		/* Split subRanges of notes. If splitting a cross system beam, handle cases
		for first & second pieces of crossSystem beams on first & second systems.
		Handle first subRange here. */
		nBeamable = CountBeamable(doc, firstSyncL, syncL, voice, FALSE);
		if (nBeamable>=2)
			newBeamL = CreateBEAMSET(doc, firstSyncL, syncL, voice, nBeamable,
												FALSE, doc->voiceTab[voice].voiceRole);
		newBeam = GetPBEAMSET(newBeamL);
		if (crossSys)
			if (firstSys)
				newBeam->crossSystem = newBeam->firstSystem = FALSE;
			else {
				newBeam->crossSystem = TRUE;
				newBeam->firstSystem = FALSE;
			}
		
		/* Handle second subRange of notes. */
		nBeamable = CountBeamable(doc, RightLINK(syncL), RightLINK(lastSyncL),
											voice, FALSE);
		if (nBeamable>=2)
			newBeamL = CreateBEAMSET(doc, RightLINK(syncL), RightLINK(lastSyncL), voice,
										nBeamable, FALSE, doc->voiceTab[voice].voiceRole);
		newBeam = GetPBEAMSET(newBeamL);
		if (crossSys)
			if (firstSys)
					newBeam->crossSystem = newBeam->firstSystem = TRUE;
			else 	newBeam->crossSystem = newBeam->firstSystem = FALSE;
	}

	if (NoteBEAMED(newNoteL) && NoteINCHORD(newNoteL)) {
/* syncL may need fixing up, since the new note in the chord may need the
 * stem. This involves: 
 * 1. Get ystem for the stemmed note in this voice & sync. */
		aNoteL = FirstSubLINK(syncL);
		for (ystem = -9999; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==voice && !NoteREST(aNoteL))
				if (MainNote(aNoteL))
					ystem = NoteYSTEM(aNoteL);

		if (ystem==-9999) {
			 MayErrMsg("AddNoteFixBeams: no MainNote in sync %ld voice %ld\n",
											(long)syncL, (long)voice);
			ystem = 99;
		}
/* 2. Get note in syncL in voice furthest from ystem (farNote).*/
		maxLength = -9999;
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==voice && !NoteREST(aNoteL))
				if (ABS(NoteYD(aNoteL)-ystem)>maxLength) {
					maxLength = ABS(NoteYD(aNoteL)-ystem);
					farNoteL = aNoteL;
				}

/* 3. Set farNote->ystem to ystem from 1., others to their ->yd. */
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
			if (NoteVOICE(aNoteL)==voice && !NoteREST(aNoteL))
				if (aNoteL==farNoteL)
					NoteYSTEM(aNoteL) = ystem;
				else NoteYSTEM(aNoteL) = NoteYD(aNoteL);
	}
}


/* ----------------------------------------------------------- AddGRNoteFixBeams -- */
/* AddGRNoteFixBeams should be called ONLY if <newGRNoteL> is being inserted in the
middle of a beamset; it unbeams the GRBeamset and leaves it unbeamed. */ 

void AddGRNoteFixBeams(Document *doc,
								LINK grSyncL,				/* Sync the new note belongs to */
								LINK newGRNoteL			/* The new note */
								)
{
	short 		voice;
	LINK			firstGRSyncL,lastGRSyncL,beamL;
	SearchParam	pbSearch;
	
	voice = GRNoteVOICE(newGRNoteL);
	
	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.voice = voice;
	pbSearch.needSelected = FALSE;
	pbSearch.inSystem = FALSE;
	pbSearch.subtype = GRNoteBeam;

	beamL = L_Search(grSyncL, BEAMSETtype, TRUE, &pbSearch);
	firstGRSyncL = FirstInBeam(beamL);
	lastGRSyncL = LastInBeam(beamL);
	AddGRNoteUnbeam(doc, beamL, firstGRSyncL, RightLINK(lastGRSyncL), voice);

	RecomputeGRNoteStems(doc, firstGRSyncL, RightLINK(lastGRSyncL), voice);
}


/* ----------------------------------------------------------- AddNoteFixTuplets -- */
/* If the given note is <inChord>, set its tuplet flag and playDur to that of its
chord's MainNote. If not, if it's in the middle of a tuplet in <voice>, remove the
tuplet and untuple all the notes in that tuplet's range. Intended to be called when
the given note was just added. */
 
void AddNoteFixTuplets(Document *doc,
								LINK newL, LINK aNoteL,		/* Sync and new note (in that Sync) */
								short voice
								)
{
	LINK mainNoteL, tupleL, firstSync, lastSync;
	
	if (NoteINCHORD(aNoteL)) {
		mainNoteL = FindMainNote(newL, voice);
		if (mainNoteL) {
				NoteINTUPLET(aNoteL) = NoteINTUPLET(mainNoteL);
				NotePLAYDUR(aNoteL) = NotePLAYDUR(mainNoteL);
		}
	}
	else if (tupleL = VHasTupleAcross(newL, voice, TRUE)) {
		firstSync = FirstInTuplet(tupleL);
		lastSync = LastInTuplet(tupleL);
		UntupleObj(doc, tupleL, firstSync, lastSync, voice);
	}
}


/* ----------------------------------------------------------- AddNoteFixOctavas -- */
/* Given a note that's being inserted into an octava'd range, set its inOctava
flag TRUE and adjust its yd and ystem. If the note is not being chorded with an
existing note, also insert a subobject into the octava object for its Sync.

Return TRUE if all OK, FALSE if there's a problem (probably out of memory). */

Boolean AddNoteFixOctavas(LINK newL, LINK newNoteL)	/* Sync and new note (in that Sync) */
{
	LINK octavaL, newNoteOctavaL, aNoteOctavaL, octavaSyncL, nextSyncL;
	PANOTEOCTAVA aNoteOctava, newNoteOctava;
	PANOTE aNote;
	short staff;
	
	staff = NoteSTAFF(newNoteL);
	
	/* Check if there is an octave sign across newL on <staff>. If so, and if the new
	note is not inChord, insert a  subobject for the new note into the octave sign's
	subobject list and increment the octave sign's nEntries. Since we can insert only
	one note at a time, we have to add only one subobject. InsertLink requires that we
	update the firstSubLink field of the object if we are going to add the subObj to
	the head of the list; however, this cannot happen for this would be adding the
	note before the first note in the octave sign, so HasOctavaAcross would return
	NILINK. */

	if (octavaL = HasOctAcross(newL, staff, TRUE)) {
		aNote = GetPANOTE(newNoteL);
		if (!aNote->rest) aNote->inOctava = TRUE;
	
		nextSyncL = LSSearch(RightLINK(newL), SYNCtype, ANYONE, GO_RIGHT, FALSE);
		if (!NoteINCHORD(newNoteL))
			if (newNoteOctavaL = HeapAlloc(NOTEOCTAVAheap, 1)) {
			
				/* Subobjects must be in order: find the place to insert the new one. */
				aNoteOctavaL = FirstSubLINK(octavaL);
				for ( ; aNoteOctavaL; aNoteOctavaL = NextNOTEOCTAVAL(aNoteOctavaL)) {
					aNoteOctava = GetPANOTEOCTAVA(aNoteOctavaL);
					octavaSyncL = aNoteOctava->opSync;
					if (octavaSyncL==nextSyncL) {
						newNoteOctava = GetPANOTEOCTAVA(newNoteOctavaL);
						newNoteOctava->opSync = newL;
						InsertLink(NOTEOCTAVAheap, FirstSubLINK(octavaL), aNoteOctavaL, 
							newNoteOctavaL);
					}
				}
				LinkNENTRIES(octavaL)++;
				return TRUE;
			}
			else
				return FALSE;
	}
	
	return TRUE;
}


/* ------------------------------------------------------------- AddNoteFixSlurs -- */
/* If there's a tie crossing the added note, remove it. Ignore slurs, which can
cross any intervening notes: maybe this function should be renamed AddNoteFixTies.
??This function should certainly work by voice, not by staff! */

void AddNoteFixSlurs(Document *doc, LINK newL, short staff)
{
	PANOTE	aNote;
	LINK		syncL, tieL, aNoteL;

	tieL = LeftSlurSearch(newL,staff,TRUE);
	if (!tieL) return;

	if (IsAfter(newL,SlurLASTSYNC(tieL))) {
		syncL = SlurFIRSTSYNC(tieL);
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL); 
			if (aNote->staffn==staff) aNote->tiedR = FALSE;
		}
		syncL = SlurLASTSYNC(tieL);
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			aNote = GetPANOTE(aNoteL); 
			if (aNote->staffn==staff) aNote->tiedL = FALSE;
		}
		DeleteNode(doc, tieL);
	}
}


/* ---------------------------------------------------------- AddNoteFixGraphics -- */
/* Make firstObj pointers of all Graphics that now point to oldpL point to newpL. */

void AddNoteFixGraphics(LINK newpL, LINK oldpL)
{
	PGRAPHIC	pGraphic;
	LINK		systemL, graphicL;
	
	systemL = LSSearch(newpL, SYSTEMtype, 1, TRUE, FALSE);
	for (graphicL = LSSearch(newpL, GRAPHICtype, ANYONE, TRUE,FALSE);
		  graphicL && !IsAfter(graphicL, systemL);
		  graphicL = LSSearch(LeftLINK(graphicL), GRAPHICtype,
		  								ANYONE, TRUE, FALSE)) {
		pGraphic = GetPGRAPHIC(graphicL);
		if (pGraphic->firstObj==oldpL) 
			pGraphic->firstObj = newpL;
	}
}


/* ---------------------------------------------------------------- FixWholeRests -- */
/* In <thisL>'s measure, correct whole rest/whole measure rest status: make whole
rests be whole-measure rests iff there are no other notes/rests in their voice in
the measure. Exception: in some time signatures, we also make breve rests be
whole-measure rests.

After this, the caller should update timestamps (normally by calling FixTimeStamps). */

void FixWholeRests(Document *doc, LINK thisL)
{
	LINK		pL, endL, aNoteL;
	PANOTE	aNote;
	Boolean	wholeMR;
	Boolean	breveWholeMeasure;	/* To allow breve rests to be whole-measure rests */
	CONTEXT	context;
	
	pL = LSSearch(thisL, MEASUREtype, ANYONE, TRUE, FALSE);
	if (pL==NILINK) return;
	endL = EndMeasSearch(doc, thisL);
	
	for ( ; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL)) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
				GetContext(doc, thisL, NoteSTAFF(aNoteL), &context);
			
				/* Allow whole-measure rests in some time signatures to look like breve rests. */
				
				breveWholeMeasure = WholeMeasRestIsBreve(context.numerator,
																		context.denominator);
				if (NoteREST(aNoteL) &&
						(NoteType(aNoteL)==WHOLEMR_L_DUR ||
						 NoteType(aNoteL)==WHOLE_L_DUR   ||
						 (breveWholeMeasure && (NoteType(aNoteL)==BREVE_L_DUR))
						 )) {
					wholeMR = !SyncInVoiceMeas(doc, pL, NoteVOICE(aNoteL), TRUE);
					
					/* If rest is changing from whole-measure to whole, move it back
					 *	to a "normal" (non-centered) position. We do nothing in the
					 * opposite case because centering is affected by the width of
					 *	the measure, so we leave the whole question for Respacing.
					 */
					if (!wholeMR && NoteType(aNoteL)==WHOLEMR_L_DUR) {
						aNote = GetPANOTE(aNoteL);
						aNote->xd = 0;
					}
					NoteType(aNoteL) = (wholeMR? WHOLEMR_L_DUR : 
												(breveWholeMeasure? BREVE_L_DUR : WHOLE_L_DUR));
				}
		}
	}
}


/* -------------------------------------------------------------- FixNewMeasAccs -- */
/*	FixNewMeasAccs should be called when the given Measure object has just been
created by inserting it and may already contain notes. Notes within it that
formerly inherited accidentals from preceding notes on their line/space in what
is now the previous measure must have explicit accidentals added, and this function
does that. */

void FixNewMeasAccs(Document *doc, LINK measureL)
{
	LINK			endMeasL;
	short			s;
	SignedByte	accAfterTable[MAX_STAFFPOS];
	
	endMeasL = EndMeasSearch(doc, measureL);

	for (s = 1; s<=doc->nstaves; s++)
	{
		GetAccTable(doc, accTable, measureL, s);						/* Accidentals before barline */
		GetAccTable(doc, accAfterTable, RightLINK(measureL), s); /* Accidentals just after (=key sig.) */			
		CombineTables(accTable, accAfterTable);
		FixAllAccidentals(measureL, endMeasL, s, FALSE);
	}
}



/* --------------------------------------------------------------- InsFixMeasNums -- */
/* Update fakeMeas and measureNum fields of appropriate Measures and Measure subobjs
in the given document. <newL> is assumed to be an object whose insertion occasions the
update; if its insertion could not affect measure numbers, we do nothing. Specifically,
if it's a Sync or GRSync, measure numbers can change only if its Measure was previously
fake. If the object is a Measure, we assume it's been inserted in its entirety, so
measure numbers certainly change. If it's any other type of object, no change is
possible.

Deliver TRUE if we actually change anything. */

Boolean InsFixMeasNums(Document *doc, LINK newL)
{
	LINK measL;

	if (newL==NILINK) {
		MayErrMsg("InsFixMeasNums: newL is NILINK.");
		return FALSE;
	}
	
	switch (ObjLType(newL)) {
		case MEASUREtype:
#if 1
			/* For years, the 2nd param here has been <NILINK>, i.e., do the entire file.
			 * But starting just before <newL> can be much, much faster, and it should be
			 * sufficient in all cases.  --DAB, 1/2000
			 */
			return UpdateMeasNums(doc, LeftLINK(newL));
#else
			return UpdateMeasNums(doc, NILINK);
#endif
		case SYNCtype:
		case GRSYNCtype:
			measL = LSSearch(newL, MEASUREtype, ANYONE, GO_LEFT, FALSE);
			if (!MeasISFAKE(measL)) return FALSE;		
			/*
			 *	The updating is a bit tricky, so (at least for now) we'll just let
			 *	UpdateMeasNums handle it.
			 */
			return UpdateMeasNums(doc, NILINK);
		default:
			return FALSE;
		}
}


/* ------------------------------------------------------------ FindGraphicObject -- */
/* Get the relObj for a graphic which is to be inserted. */

LINK FindGraphicObject(Document *doc, Point pt,
								short *staff, short *voice)	/* Undefined if object is a System */
{
	short index; LINK pL;

	FindStaff(doc, pt);													/* Set currentSystem */
	pL = FindRelObject(doc, pt, &index, SMFind);
	
	if (!pL)
		return NILINK;
	
	if (!SystemTYPE(pL)) {
		*staff = GetSubObjStaff(pL, index);
		*voice = GetSubObjVoice(pL, index);
	}

	return pL;
}


/* ------------------------------------------------------------- ChkGraphicRelObj -- */
/* Check the type of the relObj for a Graphic which is to be inserted and return
TRUE iff it's OK; otherwise, give an error message and return FALSE. */

Boolean ChkGraphicRelObj(LINK pL,
									char subtype)			/* ??SignedByte May be pseudo-code <GRChar> */
{
	if (!pL) return FALSE;
	
	switch (subtype) {
		case GRRehearsal:
			if (SystemTYPE(pL) || PageTYPE(pL)) {
				GetIndCString(strBuf, INSERTERRS_STRS, 17);    /* "A rehearsal mark must be attached to a barline, note/rest, clef, key signature, time signature, or spacer." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return FALSE;
			}
			break;
		case GRChordSym:
			if (SyncTYPE(pL))
				return TRUE;
			else {
				GetIndCString(strBuf, INSERTERRS_STRS, 15);    /* "A chord symbol must be attached to a note or rest." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return FALSE;
			}
			break;
		case GRChar:
			if (SystemTYPE(pL) || PageTYPE(pL)) {
				GetIndCString(strBuf, INSERTERRS_STRS, 29);    /* "A music character must be attached to..." */
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return FALSE;
			}		
			break;
		case GRMIDIPatch:
			if (SyncTYPE(pL))
				return TRUE;
			else {
				GetIndCString(strBuf, INSERTERRS_STRS, 30);    // "A Midi patch change must be attached to a note or rest." //
				CParamText(strBuf, "", "", "");
				StopInform(GENERIC_ALRT);
				return FALSE;
			}
			break;
		default:
			break;
	}
	
	switch (ObjLType(pL)) {
		case PAGEtype:
		case SYSTEMtype:
		case STAFFtype:
		case MEASUREtype:
		case PSMEAStype:
		case SYNCtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case SPACEtype:
		case RPTENDtype:
			break;
		default:
			/*
			 * ??Users don't think rehearsal marks, chord symbols, and music symbols are
			 * Graphics; it'd be better if this error message distinguished them.
			 */ 
			GetIndCString(strBuf, INSERTERRS_STRS, 16);    /* "A Graphic within a system must be attached to a barline, note/rest, clef, key signature, time signature, or spacer." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return FALSE;
	}
	
	return TRUE;
}


/* -------------------------------------------------------------- FindTempoObject -- */
/* Get the relObj for a Tempo object which is to be inserted. */

LINK FindTempoObject(Document *doc, Point pt, short *staff, short *voice)
{
	short index; LINK pL;

	FindStaff(doc, pt);													/* Set currentSystem */
	pL = FindRelObject(doc, pt, &index, SMFind);
	
	if (!pL) {
		GetIndCString(strBuf, INSERTERRS_STRS, 18);    /* "A tempo mark must be attached to a barline, note/rest, clef, key sig., or time sig." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return NILINK;
	}
	
	*staff = GetSubObjStaff(pL, index);
	*voice = GetSubObjVoice(pL, index);

	switch (ObjLType(pL)) {
		case MEASUREtype:
		case PSMEAStype:
		case SYNCtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case RPTENDtype:
			break;
		default:
			GetIndCString(strBuf, INSERTERRS_STRS, 19);    /* "A tempo must be attached to a barline, note/rest, clef, key sig., or time sig." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return NILINK;
	}
		
	return pL;
}


/* ------------------------------------------------------------ FindEndingObject -- */
/* Get the relObj for an Ending which is to be inserted. For objects with connStaffs
and subObjects which span staff ranges the CheckObject and FindObject routines do
not work well (??apparently in the sense of finding the correct subObjects); for
these objects FindStaff should provide unambiguous results and is used. */

LINK FindEndingObject(Document *doc, Point pt, short *staff)
{
	short index,staffn; LINK pL;

	staffn = FindStaff(doc, pt);												/* Set currentSystem */
	pL = FindObject(doc, pt, &index, SMFind);

	switch (ObjLType(pL)) {
		case SYNCtype:
			*staff = GetSubObjStaff(pL, index);
			break;
		case MEASUREtype:
			*staff = staffn;
			break;
		case PSMEAStype:
			*staff = staffn;
			break;
		default:
			GetIndCString(strBuf, INSERTERRS_STRS, 20);    /* "An ending must be attached to a barline, pseudo-measure, or note/rest." */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			return NILINK;
			
	}

	return pL;
}

/* ---------------------------------------------------------------- UpdateTailxd -- */
/* Maintain a valid xd for the tailL of the document to indicate horizontal position
of objects pasted or inserted at the end of the score. This will not always give
good results, e.g., if the score ends with a Sync with a whole note in voice 1
followed by a Sync with just a 16th in voice 2. Cf. MeasOccupiedWidth, which
does something similar; its help function SymLikelyWidthRight and CalcSpaceNeeded
should be combined or, better, this function should just call MeasOccupiedWidth. */

void UpdateTailxd(Document *doc)
{
	LINK pL; short j; DDIST width; CONTEXT context;

	width = -1000;
	pL = LeftLINK(doc->tailL);

	if (SyncTYPE(pL))
		LinkXD(doc->tailL) = CalcSpaceNeeded(doc, doc->tailL);
	else {
		for (j=1; j<=doc->nstaves; j++) {
			GetContext(doc, pL, j, &context);
			width = n_max(width, SymDWidthRight(doc, pL, j, FALSE, context));
		}
		LinkXD(doc->tailL) = width;
	}
}


/* ------------------------------------------------------------ CenterTextGraphic -- */
/* Center the Graphic with respect to a normal-width note (half or shorter) we assume
it's attached to; if it's attached to anything else, it probably won't be well
centered. ??Uses the width of the Graphic's objRect, the accuracy of which is
magnification dependent; this should be changed. */

void CenterTextGraphic(Document *doc, LINK graphicL)
{
	DDIST headWidth; CONTEXT context; short width;

	GetContext(doc, graphicL, GraphicSTAFF(graphicL), &context);
	width = NPtGraphicWidth(doc, graphicL, &context);
	LinkXD(graphicL) -= pt2d(width)/2;
	headWidth = HeadWidth(LNSPACE(&context));
	LinkXD(graphicL) += headWidth/2;
}


/* ------------------------------------------------------------------ NewObjInit -- */

Boolean NewObjInit(Document *doc, short type, short *sym, char inchar, short staff,
							PCONTEXT context)
{
	if (doc->selStartL==doc->headL)
		return FALSE;

	if (type==KEYSIGtype)
		*sym = KEYSIGtype;
	else {
		*sym = GetSymTableIndex(inchar);
		if (*sym==NOMATCH)								/* This should never happen */ 
		{
			MayErrMsg("NewObjInit: illegal character.");
			return FALSE;
		}
	}
	
	/* If doc->selStartL is before the first Measure, GetContext will not define
	 * all fields. For now, at least fill the most important ones in with the
	 * likely (but not guaranteed) values--a lousy way to handle this.
	 * GetContext should really always define these!
	 */
	 context->staffHeight = drSize[doc->srastral];
	 context->staffLines = STFLINES;

	if (staff==ANYONE) GetContext(doc, LeftLINK(doc->selStartL), 1, context);
	else 					 GetContext(doc, LeftLINK(doc->selStartL), staff, context);

	return TRUE;
}


/* ----------------------------------------------------------------- NewObjSetup -- */

void NewObjSetup(Document *doc,
						LINK newpL,
						short /*staff*/,		/* no longer used */
						DDIST xd)
{
	doc->changed = TRUE;
	doc->used = TRUE;
	
	SetObject(newpL, xd, 0, TRUE, TRUE, FALSE);
 	LinkTWEAKED(newpL) = FALSE;
}


/* --------------------------------------------------------------- NewObjPrepare -- */

LINK NewObjPrepare(Document *doc, short type, short *sym, char inchar, short staff,
							short x, CONTEXT *context)
{
	LINK	 	newpL;
	DDIST		xd;
	short		insertType;

	NewObjInit(doc, type, sym, inchar, staff, context);

	if (type==KEYSIGtype || type==RPTENDtype) insertType = type;
	else													insertType = symtable[*sym].objtype;
	
	if (type==TEMPOtype || type==SPACEtype || type==ENDINGtype)
		newpL = InsertNode(doc, doc->selStartL, insertType, 0);
	else if (staff!=ANYONE)
		newpL = InsertNode(doc, doc->selStartL, insertType, 1);
	else
		newpL = InsertNode(doc, doc->selStartL, insertType, doc->nstaves);
	if (newpL) {
		switch (type) {
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case RPTENDtype:
			case TEMPOtype:
			case SPACEtype:
			case ENDINGtype:
				xd = p2d(x)-context->measureLeft; 				/* relative to measure */
				break;
			case DYNAMtype:
				xd = 0;
				break;
			default:
				MayErrMsg("NewObjPrepare: type %ld not supported", (long)type);
		}
		NewObjSetup(doc, newpL, staff, xd);
	}
	else
		NoMoreMemory();

	return newpL;
}


/* --------------------------------------------------------------- NewObjCleanup -- */

void NewObjCleanup(Document *doc, LINK newpL, short staff)
{
	LINK		pMeasL,sysL;
	DDIST		measxd;
	
	doc->selStartL = newpL; 															/* Update selection first ptr */
	doc->selEndL = RightLINK(newpL);													/* ...and last ptr */

	if (doc->selEndL==doc->tailL) UpdateTailxd(doc);
	if (doc->autoRespace && objTable[ObjLType(newpL)].justType!=J_D) {
		pMeasL = LSSearch(doc->selStartL,MEASUREtype,staff,FALSE,FALSE);
		if (pMeasL) {
			measxd = LinkXD(pMeasL);
			RespaceBars(doc,doc->selStartL,doc->selEndL,0L,FALSE,FALSE);		/* Make space in this measure */
			if (LinkXD(pMeasL)!=measxd || pMeasL==LeftLINK(doc->selStartL)) {	/* Did meas. length change? */
				sysL = LSSearch(doc->selStartL,SYSTEMtype,ANYONE,FALSE,FALSE);
				if (sysL) 
					RespaceBars(doc,doc->selStartL,sysL,0L,FALSE,FALSE);			/* Probably--make space in following meas. for new symbol */
				else
					RespaceBars(doc,doc->selStartL,doc->tailL,0L,FALSE,FALSE);	/* Probably--make space in following meas. for new symbol */
			}
		}
		else
			RespaceBars(doc,doc->selStartL,doc->selEndL,0L,FALSE,FALSE);		/* Make space in all meas. for new symbol */
	}
	else
		InvalMeasure(newpL, staff);													/* Just redraw the measure */
}


/* Null function to allow loading or unloading InsUtils.c's segment. */

void XLoadInsUtilsSeg()
{
}