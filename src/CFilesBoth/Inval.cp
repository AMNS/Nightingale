/******************************************************************************************
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
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static void AccumRect(Rect *, Rect *);

/* -----------------------------------------------------------------------InvalMeasure -- */
/*	Erase and Inval the entire System the specified object is in. Assumes the object
is not doc->tailL! NB: As of v. 5.7, we require all barlines to align on every staff;
as long as that's true, <theStaff> is irrelevant. */

void InvalMeasure(LINK pL, short theStaff)
{	
	InvalSystems(pL, RightLINK(pL));
}


/* ------------------------------------------------------------------------- AccumRect -- */

static void AccumRect(Rect *r, Rect *tempR)
{
	Rect nilRect;
	
	SetRect(&nilRect, 0, 0, 0, 0);
	if (EqualRect(tempR, &nilRect))	*tempR = *r;
	else							UnionRect(r, tempR, tempR);
}


/* --------------------------------------------------------------------- InvalMeasures -- */
/*	Erase and Inval all Measures at least from the one the first specified object is
in (or, if it's not in a Measure, the next Measure) through and including the one the
second specified object is in. Actually, since computers now are hundreds (if not
thousands!) of times faster than when this routine was written (in the 1980's or
1990's), we just Inval every system involved in its entirety. NB: As of v. 5.7, we
require all barlines to align on every staff; as long as that's True, <theStaff> is
irrelevant.

Going beyond the specified Measures is helpful because objects of some types --
especially Tempo/metronom marks and text Graphics -- quite often extend beyond their
Measure. We tried going a Measure further on each end but, fairly often, that
still isn't enough. */

void InvalMeasures(LINK fromL, LINK toL,
							short theStaff)			/* Staff no. or ANYONE */
{
	InvalSystems(fromL, toL);
}


/* ------------------------------------------------------------------------InvalSystem -- */
/*	Erase and Inval the System the specified object is in. NB: Destroys the global
array contextA. */

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

	systemL = LSSearch(pL, SYSTEMtype, ANYONE, True, False);
	if (systemL!=NILINK) {
		r = LinkOBJRECT(systemL);
		r.left = 0;
		r.right = doc->paperRect.right;

		pageL = LSSearch(pL, PAGEtype, ANYONE, GO_LEFT, False);
		ContextPage(doc, pageL, contextA);
		OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);
		EraseAndInval(&r);
	}

	doc->currentSheet = oldSheet;
	doc->currentPaper = oldPaper;
}


/* ---------------------------------------------------------------------- InvalSystems -- */
/*	Erase and Inval all Systems in the specified range. */

void InvalSystems(LINK fromL, LINK toL)
{
	LINK sysL, pL;
	
	sysL = LSSearch(fromL, SYSTEMtype, ANYONE, GO_LEFT, False);
	for (pL=sysL; pL && pL!=toL; pL = RightLINK(pL))
		if (SystemTYPE(pL))
			InvalSystem(pL);
}


/* --------------------------------------------------------------------- InvalSysRange -- */
/* Inval all systems in the range [fromSys,toSys). fromSys and toSys must be
LINKs to System objects. */

void InvalSysRange(LINK fromSys, LINK toSys)
{
	LINK pL;

	for (pL = fromSys; pL && pL!=LinkRSYS(toSys); pL = LinkRSYS(pL))
		InvalSystem(pL);
}

/* --------------------------------------------------------------------- InvalSelRange -- */
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


/* ------------------------------------------------------------------------ InvalRange -- */
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
		LinkVALID(pL) = False;
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

/* ---------------------------------------------------------------- EraseAndInvalRange -- */
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
		LinkVALID(pL) = False;
		LinkOBJRECT(pL) = emptyRect;
	}
}

/* ---------------------------------------------------------------------- InvalContent -- */
/* Clear the valid flags and empty the objRects of all objects in the specified
range that are not of type J_STRUC. */

void InvalContent(LINK fromL, LINK toL)
{
	LINK	pL;
	Rect	emptyRect;

	SetRect(&emptyRect, 0, 0, 0, 0);
	for (pL=fromL; pL && pL!=toL; pL=RightLINK(pL)) {
		if (!J_STRUCTYPE(pL)) {
			LinkVALID(pL) = False;
			LinkOBJRECT(pL) = emptyRect;
		}
	}
}


/* ----------------------------------------------------------------------- InvalObject -- */
/* Called to make sure objects are redrawn completely when inserted, deleted or
otherwise require redrawing: particularly useful for objects which may extend
outside the systemRect, since normally Nightingale only redraws the systemRect. 
Note:
1. Uses the global array contextA, so cannot be called if this is already
in use.
2. Requires that the object list provide a system properly containing
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
Str255 string;  short strlen;
Pstrcpy((StringPtr)string, (StringPtr)PCopy(GetPAGRAPHIC(FirstSubLINK(pL))->strOffset));
LogPrintf(LOG_DEBUG, "InvalObject: pL=%u strlen=%d\n", pL, Pstrlen(string));
			DrawGRAPHIC(doc, pL, contextA, False);
			break;
		case TEMPOtype:
			DrawTEMPO(doc, pL, contextA, False);
			break;
		case DYNAMtype:
			DrawDYNAMIC(doc, pL, contextA, False);
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
LogPrintf(LOG_DEBUG, "InvalObject: r=%d,%d,%d,%d doErase=%d\n", r.top, r.left, r.bottom, r.right, doErase);

	/* The objRect of dynamics is not quite big enough; that is not our problem here,
		but inset the rect to partially compensate. FIXME: The amount of inset should
		really be dynamically determined, e.g., based on sRastral and magnify. */

	inset = DynamTYPE(pL) ? -4 : -1;
	InsetRect(&r, inset, inset);

	OffsetRect(&r, doc->currentPaper.left, doc->currentPaper.top);

	if (doErase)	EraseAndInval(&r);
	else			InvalWindowRect(doc->theWindow,&r);
}
