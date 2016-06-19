/* HeapBrowser.c - Heap Browser functions for Nightingale */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#ifndef PUBLIC_VERSION			/* If public, skip this file completely! */

#define HEAPBROWSER_DLOG 1910
#define LEADING 11				/* Vertical dist. between lines displayed (pixels) */

static Rect bRect;
static short linenum;
static char s[256];

static void ShowHeap(short, short);
static void HeapDrawLine(char * s);
static void HeapBrowsePartInfo(short);
static void HeapBrowseStaff(short);
static void HeapBrowseConnect(short);
static void HeapBrowseClef(short);
static void HeapBrowseKeySig(short);
static void HeapBrowseTimeSig(short);
static void HeapBrowseMeasure(short);
static void HeapBrowseSync(short);
static void HeapBrowseBeamset(short);
static void HeapBrowseDynamic(short);
static void HeapBrowseMODNR(short);
static void HeapBrowseRepeatEnd(short);
static void HeapBrowseSlur(short);
static void HeapBrowseTuplet(short);
static void HeapBrowseOttava(short);
static void HeapBrowseGRSync(short);
static void HeapBrowseObject(short);

/* ------------------------------------------------------------------ HeapBrowser -- */

void HeapBrowser()
{
enum {
	hOK = 1,
	hText,
	hHead,
	hLeft,
	hRight,
	hTail,
	hUp,
	hDown,
	hPicture
};

	register DialogPtr dlog;
	short itype;
	Handle tHdl;
	short ditem;
	Boolean done;
	GrafPtr oldPort;
	register short heapIndex, itemIndex;
	short oldHeapIndex, oldItemIndex;
	
/* --- 1. Create the dialog and initialize its contents. --- */

	GetPort(&oldPort);
	dlog = GetNewDialog(HEAPBROWSER_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog)
		MissingDialog(HEAPBROWSER_DLOG);
	SetPort(GetDialogWindowPort(dlog));
	GetDialogItem(dlog, hText, &itype, &tHdl, &bRect);

	ArrowCursor();

/*--- 2. Interact with user til they push Done. --- */

	done = FALSE;
	TextFont(SYSFONTID_MONOSPACED);
	TextSize(9);
	itemIndex = heapIndex = 0;
	oldItemIndex = oldHeapIndex = -1;
	do {
		if (heapIndex!=oldHeapIndex || itemIndex!=oldItemIndex) {	/* If the desired thing changed... */
			EraseRect(&bRect);
			ShowHeap(heapIndex, itemIndex);						/*		show the thing */
		}
		oldHeapIndex = heapIndex;
		oldItemIndex = itemIndex;
		ModalDialog(NULL, &ditem);									/* Handle dialog events */
		switch (ditem) {
		case hOK:
		  	done = TRUE;
		  	break;
		case hHead:
			heapIndex = FIRSTtype;
		  	break;
		case hLeft:
			if (heapIndex > FIRSTtype)
				{ itemIndex = 1; heapIndex--; }
			break;
		case hRight:
			if (heapIndex < LASTtype-1) 
				{ itemIndex = 1; heapIndex++; }
			break;
		case hTail:
			heapIndex = LASTtype;
			break;
		case hUp:
			if (OptionKeyDown())
				itemIndex = 1;
			else
				if (itemIndex > 0) itemIndex--;
			break;
		case hDown:
			if (OptionKeyDown()) {
				if (itemIndex < (Heap + heapIndex)->nObjs-1)
					itemIndex = (Heap + heapIndex)->nObjs-1;
			}
			else
				if (itemIndex < (Heap + heapIndex)->nObjs-1) itemIndex++;
			break;
		 default:
		 	;
		}
	} while (!done);
	
	DisposeDialog(dlog);
	SetPort(oldPort);
	return; 

}


/* --------------------------------------------------------------------- ShowHeap -- */

static void ShowHeap(short theHeap, register short itemIndex)
{
	const char *ps;
	HEAP *myHeap;

	linenum = 1;
	myHeap = Heap + theHeap;
	ps = NameHeapType(theHeap, FALSE);
	sprintf(s, "%s Heap [%d]", ps, theHeap);
	HeapDrawLine(s);

	sprintf(s, "firstFree=%d nObjs=%d nFree=%d", 
		myHeap->firstFree, myHeap->nObjs, myHeap->nFree);
	HeapDrawLine(s);
	
	sprintf(s, "objSize=%d type=%d lockLevel=%d", 
		myHeap->objSize, myHeap->type, myHeap->lockLevel);
	HeapDrawLine(s);
	
	sprintf(s, "block=%lx", myHeap->block);
	HeapDrawLine(s);
	
	HeapDrawLine("------------------");
	
	switch (theHeap) {
	case HEADERtype:
		HeapBrowsePartInfo(itemIndex);
		break;
	case STAFFtype:
		HeapBrowseStaff(itemIndex);
		break;
	case CONNECTtype:
		HeapBrowseConnect(itemIndex);
		break;
	case CLEFtype:
		HeapBrowseClef(itemIndex);
		break;
	case KEYSIGtype:
		HeapBrowseKeySig(itemIndex);
		break;
	case TIMESIGtype:
		HeapBrowseTimeSig(itemIndex);
		break;
	case MEASUREtype:
		HeapBrowseMeasure(itemIndex);
		break;
	case SYNCtype:
		HeapBrowseSync(itemIndex);
		break;
	case BEAMSETtype:
		HeapBrowseBeamset(itemIndex);
		break;
	case DYNAMtype:
		HeapBrowseDynamic(itemIndex);
		break;
	case MODNRtype:
		HeapBrowseMODNR(itemIndex);
		break;
	case RPTENDtype:
		HeapBrowseRepeatEnd(itemIndex);
		break;
	case SLURtype:
		HeapBrowseSlur(itemIndex);
		break;
	case TUPLETtype:
		HeapBrowseTuplet(itemIndex);
		break;
	case OTTAVAtype:
		HeapBrowseOttava(itemIndex);
		break;
	case GRSYNCtype:
		HeapBrowseGRSync(itemIndex);
		break;
	case OBJtype:
		HeapBrowseObject(itemIndex);
		break;
	default:
		;
	}
}

/* ----------------------------------------------------------------- HeapDrawLine -- */
/* Draw the specified C string on the next line in Heap Browser dialog */

static void HeapDrawLine(char *s)
{
	MoveTo(bRect.left, bRect.top+linenum*LEADING);
	DrawCString(s);
	++linenum;
}

/* --------------------------------------------------------------- HeapBrowseXXXs -- */

void HeapBrowsePartInfo(short itemIndex)
{
	LINK qL;
	PPARTINFO q;
	
	qL = itemIndex;
	
	q = GetPPARTINFO(qL);
	sprintf(s, "link=%d addr=%lx next=%d", qL, q, q->next);
	HeapDrawLine(s); q = GetPPARTINFO(qL);
	sprintf(s, "firstStaff=%d lastStaff=%d", q->firstStaff, q->lastStaff);
	HeapDrawLine(s);
}

void HeapBrowseStaff(short itemIndex)
{
	register LINK qL;
	register PASTAFF q;
	
	qL = itemIndex;
	
	q = GetPASTAFF(qL);
	sprintf(s, "link=%d staffn=%d next=%d", qL, q->staffn, q->next);
	HeapDrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "visible=%s", q->visible ? "TRUE" : "false");
	HeapDrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "staff top/left/right=%d %d %d",
				q->staffTop,
				q->staffLeft,
				q->staffRight);
	HeapDrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "staffHeight/Lines=%d  %d",
				q->staffHeight,
				q->staffLines);
	HeapDrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "fontSize=%d", q->fontSize);
	HeapDrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "ledger/noteHead/fracBeam=%d %d %d",
				q->ledgerWidth,
				q->noteHeadWidth,
				q->fracBeamWidth);
	HeapDrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "clefType=%d", q->clefType);
	HeapDrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "nKSItems=%hd", q->nKSItems);
	HeapDrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "timeSigType/n/d=%hd %hd/%hd",
				q->timeSigType,
				q->numerator,
				q->denominator);
	HeapDrawLine(s); q = GetPASTAFF(qL);
	sprintf(s, "dynamicType=%hd", q->dynamicType);
	HeapDrawLine(s);
}

void HeapBrowseConnect(short itemIndex)
{
	register LINK qL;
	register PACONNECT q;
	
	qL = itemIndex;
	
	q = GetPACONNECT(qL);
	sprintf(s, "link=%d next=%d", qL, q->next);
	HeapDrawLine(s); q = GetPACONNECT(qL);
	
	sprintf(s, "connLevel=%hd", q->connLevel);
	HeapDrawLine(s); 	q = GetPACONNECT(qL);
	switch (q->connectType) {
	case CONNECTLINE:
		HeapDrawLine("connectType=CONNECTLINE");
		break;
	case CONNECTCURLY:
		HeapDrawLine("connectType=CONNECTCURLY");
		break;
	}
	sprintf(s, "staffAbove=%hd", q->staffAbove);
	HeapDrawLine(s); 	q = GetPACONNECT(qL);
	sprintf(s, "staffBelow=%hd", q->staffBelow);
	HeapDrawLine(s); 	q = GetPACONNECT(qL);
	sprintf(s, "firstPart=%d lastPart=%d", q->firstPart,q->lastPart);
	HeapDrawLine(s);
}

void HeapBrowseClef(short itemIndex)
{
	LINK qL;
	register PACLEF q;
	
	qL = itemIndex;
	
	q = GetPACLEF(qL);
	sprintf(s, "link=%d staffn=%d next=%d", qL, q->staffn, q->next);
	HeapDrawLine(s); 	q = GetPACLEF(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	HeapDrawLine(s); 	q = GetPACLEF(qL);
	sprintf(s, "xd=%d", q->xd);
	HeapDrawLine(s); 	q = GetPACLEF(qL);
	sprintf(s, "clefType=%d", q->subType);
	HeapDrawLine(s);
}

void HeapBrowseKeySig(short itemIndex)
{
	LINK qL;
	register PAKEYSIG q;
	
	qL = itemIndex;
	
	q = GetPAKEYSIG(qL);
	sprintf(s, "link=%d staffn=%d next=%d", qL, q->staffn, q->next);
	HeapDrawLine(s);	q = GetPAKEYSIG(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	HeapDrawLine(s); 	q = GetPAKEYSIG(qL);
	sprintf(s, "xd=%d", q->xd);
	HeapDrawLine(s);	q = GetPAKEYSIG(qL);
	sprintf(s, "nKSItems=%hd", q->nKSItems);
	HeapDrawLine(s);
}

void HeapBrowseTimeSig(short itemIndex)
{
	LINK qL;
	register PATIMESIG q;
	
	qL = itemIndex;
	
	q = GetPATIMESIG(qL);
	sprintf(s, "link=%d staffn=%d next=%d", qL, q->staffn, q->next);
	HeapDrawLine(s);	q = GetPATIMESIG(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	HeapDrawLine(s);	q = GetPATIMESIG(qL);
	sprintf(s, "xd=%d", q->xd);
	HeapDrawLine(s);	q = GetPATIMESIG(qL);
	sprintf(s, "timeSigType,n/d=%hd,%hd/%hd",
				q->subType,
				q->numerator,
				q->denominator);
	HeapDrawLine(s);
}

void HeapBrowseMeasure(short itemIndex)
{
	LINK qL;
	register PAMEASURE q;

	qL = itemIndex;
	
	q = GetPAMEASURE(qL);
	sprintf(s, "link=%d staffn=%d next=%d", qL, q->staffn, q->next);
	HeapDrawLine(s);	q = GetPAMEASURE(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	if (q->measureVisible)
		strcat(s, "MEASUREVIS ");
	if (q->connAbove)
		strcat(s, "CONNABOVE ");
	HeapDrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "measureRect=%d %d %d %d",
			q->measureRect.top,
			q->measureRect.left,
			q->measureRect.bottom,
			q->measureRect.right);
	HeapDrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "measureNum=%hd", q->measureNum);
	HeapDrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "connStaff=%hd barlineType=%hd", q->connStaff,
						q->subType);
	HeapDrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "clefType=%d", q->clefType);
	HeapDrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "nKSItems=%hd", q->nKSItems);
	HeapDrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "timeSigType/n/d=%hd %hd/%hd",
			q->timeSigType,
			q->numerator,
			q->denominator);
	HeapDrawLine(s);	q = GetPAMEASURE(qL);
	sprintf(s, "dynamicType=%hd", q->dynamicType);
	HeapDrawLine(s);
}

void HeapBrowseSync(short itemIndex)
{
	LINK qL;
	register PANOTE q;
	
	qL = itemIndex;
	
	q = GetPANOTE(qL);
	sprintf(s, "link=%d staffn=%d voice=%hd next=%d", qL,
				 q->staffn, q->voice, q->next);
	HeapDrawLine(s);	q = GetPANOTE(qL);
	strcpy(s, "flags=");
	if (q->selected)
		strcat(s, "SELECTED ");
	if (q->visible)
		strcat(s, "VISIBLE ");
	if (q->soft)
		strcat(s, "SOFT ");
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "xd=%d yd=%d yqpit=%hd ystem=%d", q->xd, q->yd, q->yqpit, q->ystem);
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "l_dur=%hd headShape=%d", q->subType, q->headShape);
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "playTDelta=%d playDur=%d noteNum=%hd", q->playTimeDelta, q->playDur,
														q->noteNum);
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "on,offVelocity=%hd,%hd", q->onVelocity, q->offVelocity);
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "ndots=%hd xmovedots=%hd ymovedots=%hd", q->ndots, q->xmovedots,
					q->ymovedots);
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "rest=%s", q->rest ? "TRUE" : "false");
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "accident=%hd accSoft=%s", q->accident,
													  q->accSoft ? "TRUE" : "false");
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "xmoveAcc=%hd", q->xmoveAcc);
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "beamed=%s otherStemSide==%s", q->beamed ? "TRUE" : "false",
															q->otherStemSide ? "TRUE" : "false");
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "inTuplet=%s inOttava=%s", q->inTuplet ? "TRUE" : "false",
														 q->inOttava ? "TRUE" : "false");
	HeapDrawLine(s);	q = GetPANOTE(qL);
	sprintf(s, "firstMod=%d", q->firstMod);
	HeapDrawLine(s);
}

void HeapBrowseBeamset(short itemIndex)
{
	LINK qL;
	register PANOTEBEAM q;
	
	qL = itemIndex;
	
	q = GetPANOTEBEAM(qL);
	sprintf(s, "link=%d @%lx bpSync=%d next=%d", qL, q, q->bpSync, q->next);
	HeapDrawLine(s);
	
	sprintf(s, "startend=%d", q->startend);
	HeapDrawLine(s);	q = GetPANOTEBEAM(qL);
	sprintf(s, "fracs=%d", q->fracs);
	HeapDrawLine(s);	q = GetPANOTEBEAM(qL);
	sprintf(s, "fracGoLeft=%s", q->fracGoLeft ? "TRUE" : "false");
	HeapDrawLine(s);
}

void HeapBrowseDynamic(short itemIndex)
{
	LINK qL;
	register PADYNAMIC q;
	
	qL = itemIndex;
	
	q = GetPADYNAMIC(qL);
	sprintf(s, "link=%d staffn=%d next=%d", qL, q->staffn, q->next);
	HeapDrawLine(s);	q = GetPADYNAMIC(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	HeapDrawLine(s);	q = GetPADYNAMIC(qL);
	sprintf(s, "xd,yd=%d,%d", q->xd, q->yd);
	HeapDrawLine(s);	q = GetPADYNAMIC(qL);
}

void HeapBrowseMODNR(short itemIndex)
{
	LINK qL;
	PAMODNR q;
	
	qL = itemIndex;
	
	q = GetPAMODNR(qL);
	sprintf(s, "link=%d next=%d", qL, q->next);
	HeapDrawLine(s);	q = GetPAMODNR(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	HeapDrawLine(s);	q = GetPAMODNR(qL);
	sprintf(s, "xstd,ystdpit=%d,%d", q->xstd, q->ystdpit);
	HeapDrawLine(s);	q = GetPAMODNR(qL);
	sprintf(s, "modCode=%hd", q->modCode);
	HeapDrawLine(s);
}

void HeapBrowseRepeatEnd(short itemIndex)
{
	LINK qL;
	register PARPTEND q;
	
	qL = itemIndex;
	
	q = GetPARPTEND(qL);
	sprintf(s, "link=%d staff=%d next=%d", qL, q->next);
	HeapDrawLine(s);	q = GetPARPTEND(qL);
	strcpy(s, "flags=");
	if (q->selected) strcat(s, "SELECTED ");
	if (q->visible) strcat(s, "VISIBLE ");
	if (q->soft) strcat(s, "SOFT ");
	HeapDrawLine(s);	q = GetPARPTEND(qL);
	sprintf(s, "connAbove=%d connStaff=%d", q->connAbove, q->connStaff);
	HeapDrawLine(s);
}

void HeapBrowseSlur(short itemIndex)
{
	LINK qL;
	register PASLUR q;
	
	qL = itemIndex;
	
	q = GetPASLUR(qL);
	
	sprintf(s, "link=%d next=%d", qL, q->next);
	HeapDrawLine(s);
		
	sprintf(s, "knot=%d %d", q->seg.knot.h, q->seg.knot.v);
	HeapDrawLine(s);	q = GetPASLUR(qL);
	sprintf(s, "c0=%d %d", q->seg.c0.h, q->seg.c0.v);
	HeapDrawLine(s);	q = GetPASLUR(qL);
	sprintf(s, "c1=%d %d", q->seg.c1.h, q->seg.c1.v);
	HeapDrawLine(s);	q = GetPASLUR(qL);
	sprintf(s, "endpt=%d %d", q->endpoint.h, q->endpoint.v);
	HeapDrawLine(s);
}

void HeapBrowseTuplet(short itemIndex)
{
	LINK qL;
	PANOTETUPLE q;
	
	qL = itemIndex;
	
	q = GetPANOTETUPLE(qL);
	sprintf(s, "link=%d next=%d", qL, q->next);
	HeapDrawLine(s);
}

void HeapBrowseOttava(short itemIndex)
{
	LINK qL;
	PANOTEOTTAVA q;
	
	qL = itemIndex;
	
	q = GetPANOTEOTTAVA(qL);
	sprintf(s, "link=%d @%lx opSync=%d next=%d", qL, q, q->opSync, q->next);
	HeapDrawLine(s);
}

void HeapBrowseGRSync(short itemIndex)
{
	LINK qL;
	PAGRNOTE q;
	
	qL = itemIndex;
	
	q = GetPAGRNOTE(qL);
	sprintf(s, "link=%d next=%d", qL, q->next);
	HeapDrawLine(s);
}

void HeapBrowseObject(short itemIndex)
{
	LINK qL;
	register PMEVENT q;
	Rect r;

	qL = itemIndex;
	
	q = GetPMEVENT(qL);
	sprintf(s, "link=%d right=%d left=%d", qL, q->right, q->left);
	HeapDrawLine(s); q = GetPMEVENT(qL);
	
	sprintf(s, "xd=%d yd=%d type=%s nEntries=%d", 
		q->xd, q->yd, NameNodeType(qL), q->nEntries);
	HeapDrawLine(s); q = GetPMEVENT(qL);
	sprintf(s, "selected=%d visible=%d soft=%d", 
		q->selected, q->visible, q->soft);
	HeapDrawLine(s); q = GetPMEVENT(qL);
	sprintf(s, "valid=%d tweaked=%d", q->valid, q->tweaked);
	HeapDrawLine(s); q = GetPMEVENT(qL); r = q->objRect;
	sprintf(s, "rect.l,t,r,b=%d %d %d %d", 
		r.left, r.top, r.right, r.bottom);
	HeapDrawLine(s);
}

#endif /* PUBLIC_VERSION */
