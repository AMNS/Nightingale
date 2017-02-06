/***************************************************************************
*	FILE:	Inval.c
*	PROJ:	Nightingale
*	DESC:	User-interface-level "Inval" functions of two types: the regular
			QuickDraw type, which applies to pixels (as in InvalRect), and
			our own, which applies to Nightingale objects. In either case,
			"Inval"ing an area of the screen will cause it to be redrawn.

		InvalMeasure			InvalMeasures			InvalSystem
		InvalSystems			InvalSysRange			InvalSelRange
		InvalRange				EraseAndInvalRange		InvalContent
		InvalObject
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


/* ------------------------------------------------------------------InvalMeasure -- */
/*	Erase and Inval the measure the specified object is in if it's in a measure;
otherwise erase and Inval its entire system. NB1: As of v. 5.7, we require all
barlines to align on every staff; as long as this is true, <theStaff> is irrelevant.
NB2: Destroys the global array contextA. */

void InvalMeasure(LINK pL, short theStaff)
{
	PMEASURE		pMeasure;
	LINK			measureL,staffL, pageL;
	Rect			r;
	register Document *doc=GetDocumentFromWindow(TopDocument);
	short			oldSheet;
	Rect			oldPaper;

	if (doc==NULL) return;
	
	oldSheet = doc->currentSheet;
	oldPaper = doc->currentPaper;

	measureL = LSSearch(pL, MEASUREtype, theStaff, TRUE, FALSE);
	staffL = LSSearch(pL, STAFFtype, theStaff, TRUE, FALSE);
	if (measureL && IsAfter(staffL, measureL)) {
		LinkVALID(measureL) = FALSE;
		pMeasure = GetPMEASURE(measureL);
		r = pMeasure->measureBBox;
		if (FirstMeasInSys(measureL)) r.left = 0;
		if (LastMeasInSys(measureL)) r.right = doc->paperRect.right;

		pageL = LSSearch(measureL, PAGEtype, ANYONE, GO_LEFT, FALSE);
		ContextPage(doc, pageL, contextA);
		OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
		EraseAndInval(&r);											/* force a screen update on it */
	}
	else
		InvalSystem(pL);

	doc->currentSheet = oldSheet;
	doc->currentPaper = oldPaper;
}


/* ------------------------------------------------------------------- AccumRect -- */

static void AccumRect(Rect *, Rect *);

static void AccumRect(Rect *r, Rect *tempR)
{
	Rect nilRect;
	
	SetRect(&nilRect, 0, 0, 0, 0);
	if (EqualRect(tempR, &nilRect))	*tempR = *r;
	else							UnionRect(r, tempR, tempR);
}


/* ---------------------------------------------------------------- InvalMeasures -- */
/*	Erase and Inval all Measures at least from the one the first specified object is
in (or, if it's not in a Measure, the next Measure) through and including the one the
second specified object is in. If the Measure preceding the first Measure of that range
is in the same System, include it too; likewise for the Measure following the last
Measure of that range. If the second object is tail, erase and inval to tail. NB: As of
v. 5.7, we require all barlines to align on every staff; as long as this is true,
<theStaff> is irrelevant.

Going beyond the specified Measures is helpful because objects of some types --
especially Tempo/metronom marks and text Graphics -- quite often extend beyond their
Measure. But, fairly often, one Measure extra each way isn't enough. FIXME: Should
really Inval the entire first System that's involved and the entire last System that's
involved. */

void InvalMeasures(LINK fromL, LINK toL,
							short theStaff)			/* Staff no. or ANYONE */
{
	LINK		measureL, lastMeasureL, pageL;
	Rect		r, tempR, nilRect;
	Document	*doc=GetDocumentFromWindow(TopDocument);
	short		oldSheet;
	Rect		oldPaper;

	if (doc==NULL) return;
	
	oldSheet = doc->currentSheet;
	oldPaper = doc->currentPaper;

	SetRect(&nilRect, 0, 0, 0, 0);
	tempR = nilRect;

	measureL = LSSearch(fromL, MEASUREtype, theStaff, GO_LEFT, FALSE);
	if (measureL) {
		LINK prevMeasL = LinkLMEAS(measureL);
//LogPrintf(LOG_DEBUG, "InvalMeasures1a: prevMeasL=%u measureL=%u\n", prevMeasL, measureL);
		if (prevMeasL && SameSystem(prevMeasL, measureL)) measureL = prevMeasL;
//LogPrintf(LOG_DEBUG, "InvalMeasures1b: prevMeasL=%u measureL=%u\n", prevMeasL, measureL);
	}
	else {
		measureL = LSSearch(fromL, MEASUREtype, theStaff, GO_RIGHT, FALSE);
	}
	
	lastMeasureL = LSSearch(toL, MEASUREtype, theStaff, GO_LEFT, FALSE);
	if (lastMeasureL && MeasureTYPE(lastMeasureL)) {
		LINK nextMeasL = LinkRMEAS(lastMeasureL);
//LogPrintf(LOG_DEBUG, "InvalMeasures2a: lastMeasureL=%u nextMeasL=%u\n", lastMeasureL, nextMeasL);
		if (nextMeasL && SameSystem(lastMeasureL, nextMeasL)) lastMeasureL = nextMeasL;
//LogPrintf(LOG_DEBUG, "InvalMeasures2b: lastMeasureL=%u nextMeasL=%u\n", lastMeasureL, nextMeasL);
	}
	
	while (measureL) {
		LinkVALID(measureL) = FALSE;
		r = MeasureBBOX(measureL);
		if (LastMeasInSys(measureL)) {
			r.right = doc->paperRect.right;
			AccumRect(&r, &tempR);

			pageL = LSSearch(measureL, PAGEtype, ANYONE, GO_LEFT, FALSE);
			doc->currentSheet = SheetNUM(pageL);
			GetSheetRect(doc,doc->currentSheet,&doc->currentPaper);
			OffsetRect(&tempR, doc->currentPaper.left, doc->currentPaper.top);
			EraseAndInval(&tempR);
			tempR = nilRect;
		}
		else if (FirstMeasInSys(measureL)) {
			AccumRect(&r, &tempR);
			r.left = 0;
		}
		else
			AccumRect(&r, &tempR);
			
		if (measureL!=lastMeasureL)	measureL = LinkRMEAS(measureL);
		else						break;
	}

	/* If the last Measure to inval exists and is not the last in the System, erase
		and inval it here.  */
	
	if (lastMeasureL) {
		pageL = LSSearch(lastMeasureL, PAGEtype, ANYONE, GO_LEFT, FALSE);
		doc->currentSheet = SheetNUM(pageL);
		GetSheetRect(doc,doc->currentSheet,&doc->currentPaper);
		OffsetRect(&tempR, doc->currentPaper.left, doc->currentPaper.top);
		EraseAndInval(&tempR);
	}
	
	doc->currentSheet = oldSheet;
	doc->currentPaper = oldPaper;
}


/* ------------------------------------------------------------------InvalSystem -- */
/*	Erase and Inval the System the specified object is in. N.B. Destroys the
global array contextA. */

void InvalSystem(LINK pL)
{
	LINK			systemL, pageL;
	Rect			r;
	GrafPtr			oldPort;
	register Document *doc=GetDocumentFromWindow(TopDocument);
	short			oldSheet;
	Rect			oldPaper;
	
	if (doc==NULL) return;
	
	GetPort(&oldPort);

	oldSheet = doc->currentSheet;
	oldPaper = doc->currentPaper;

	systemL = LSSearch(pL, SYSTEMtype, ANYONE, TRUE, FALSE);
	if (systemL!=NILINK) {
		r = LinkOBJRECT(systemL);
		r.left = 0;
		r.right = doc->paperRect.right;

		pageL = LSSearch(pL, PAGEtype, ANYONE, GO_LEFT, FALSE);
		ContextPage(doc, pageL, contextA);
		OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
		EraseAndInval(&r);
	}

	doc->currentSheet = oldSheet;
	doc->currentPaper = oldPaper;
}


/* ---------------------------------------------------------------- InvalSystems -- */
/*	Erase and Inval all Systems in the specified range. */

void InvalSystems(LINK fromL, LINK toL)
{
	LINK sysL, pL;
	
	sysL = LSSearch(fromL, SYSTEMtype, ANYONE, TRUE, FALSE);
	for (pL=sysL; pL && pL!=toL; pL = RightLINK(pL))
		if (SystemTYPE(pL))
			InvalSystem(pL);
}


/* --------------------------------------------------------------- InvalSysRange -- */
/* Inval all systems in the range [fromSys,toSys). fromSys and toSys must be
LINKs to System objects. */

void InvalSysRange(LINK fromSys, LINK toSys)
{
	LINK pL;

	for (pL = fromSys; pL && pL!=LinkRSYS(toSys); pL = LinkRSYS(pL))
		InvalSystem(pL);
}

/* --------------------------------------------------------------- InvalSelRange -- */
/* Inval all measures containing any of the selection range. Exception: If anything
selected is attached to the page, it probably won't be in the music area at all,
so inval the entire window. */

void InvalSelRange(Document *doc)
{
	if (SelAttachedToPage(doc))
		InvalWindow(doc);
	else
		InvalMeasures(doc->selStartL, doc->selEndL, ANYONE);
}


/* ------------------------------------------------------------------ InvalRange -- */
/*	Clear the valid flags and empty the objRects of all objects in the specified
specified range. This should be used with care: for example, if InvalRange is
followed by InvalSystems of systems in the range, the InvalSystems won't do much.
In such cases, use InvalContent. ??NOT JUST objRects: set information about the
screen bounding boxes of everything to empty.  */

void InvalRange(LINK fromL, LINK toL)
{
	LINK	pL, aSlurL;
	PASLUR	aSlur;
	Rect	emptyRect;

	SetRect(&emptyRect, 0, 0, 0, 0);
	for (pL=fromL; pL && pL!=toL; pL=RightLINK(pL)) {
		LinkVALID(pL) = FALSE;
		LinkOBJRECT(pL) = emptyRect;

		switch (ObjLType(pL)) {
		case SLURtype:
			aSlurL = FirstSubLINK(pL);
			for ( ; aSlurL; aSlurL=NextSLURL(aSlurL)) {
				aSlur = GetPASLUR(aSlurL);
				aSlur->bounds = emptyRect;
			}
			break;
		default:
			;
		}
	}
}

/* ---------------------------------------------------------- EraseAndInvalRange -- */
/*	Clear the valid flags, empty the objRects, and EraseAndInval all objects in
the specified range. */

void EraseAndInvalRange(Document *doc, LINK fromL, LINK toL)
{
	LINK	pL;
	Rect	r, emptyRect;

	SetRect(&emptyRect, 0, 0, 0, 0);
	for (pL=fromL; pL && pL!=toL; pL=RightLINK(pL)) {
		r = LinkOBJRECT(pL);
		Rect2Window(doc,&r);
		EraseAndInval(&r);
		LinkVALID(pL) = FALSE;
		LinkOBJRECT(pL) = emptyRect;
	}
}

/* ---------------------------------------------------------------- InvalContent -- */
/* Clear the valid flags and empty the objRects of all objects in the specified
range that are not of type J_STRUC. */

void InvalContent(LINK fromL, LINK toL)
{
	LINK	pL;
	Rect	emptyRect;

	SetRect(&emptyRect, 0, 0, 0, 0);
	for (pL=fromL; pL && pL!=toL; pL=RightLINK(pL)) {
		if (!J_STRUCTYPE(pL)) {
			LinkVALID(pL) = FALSE;
			LinkOBJRECT(pL) = emptyRect;
		}
	}
}


/* ----------------------------------------------------------------- InvalObject -- */
/* Called to make sure objects are redrawn completely when inserted, deleted or
otherwise require redrawing: particularly useful for objects which may extend
outside the systemRect, since normally Nightingale only redraws the systemRect. 
Note:
1. Uses the global array contextA, so cannot be called if this is already
in use.
2. Requires that the data structure provide a system properly containing
pL. E.g., if called while the object's system is being deleted, may not
provide correct results.
3. Can be called for any object to whose drawing routing you are willing to
add a <doDraw> parameter.
*/

void InvalObject(Document *doc, LINK pL, short doErase)
{
	Rect r, slurBBox;  LINK aSlurL;
	short inset;

	GetAllContexts(doc,contextA,pL);
	switch (ObjLType(pL)) {
		case GRAPHICtype:
			DrawGRAPHIC(doc, pL, contextA, FALSE);
			break;
		case TEMPOtype:
			DrawTEMPO(doc, pL, contextA, FALSE);
			break;
		case DYNAMtype:
			DrawDYNAMIC(doc, pL, contextA, FALSE);
			break;
		case SLURtype:
			/* Need to handle slurBBox specially. */
			aSlurL = FirstSubLINK(pL);
			for ( ; aSlurL; aSlurL=NextSLURL(aSlurL)) {
				GetSlurBBox(doc, pL, aSlurL, &slurBBox, 4);
				InvalWindowRect(doc->theWindow,&slurBBox);
			}
			break;
	}

	r = LinkOBJRECT(pL);

	/* The objRect of dynamics is not quite big enough; that is not our problem here,
		but inset the rect to partially compensate. FIXME: The amount of inset should
		really be dynamically determined, e.g., based on sRastral and magnify. */

	inset = DynamTYPE(pL) ? -4 : -1;
	InsetRect(&r, inset, inset);

	OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);

	if (doErase)	EraseAndInval(&r);
	else			InvalWindowRect(doc->theWindow,&r);
}
