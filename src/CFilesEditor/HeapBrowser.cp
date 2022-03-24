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
static char str[256];

static void ShowHeap(short, short);
static void HeapDrawLine(char *s);
static void HeapBrowsePartInfo(short, Boolean);
static void HeapBrowseSync(short, Boolean);
static void HeapBrowseRepeatEnd(short, Boolean);
static void HeapBrowseStaff(short, Boolean);
static void HeapBrowseMeasure(short, Boolean);
static void HeapBrowseClef(short, Boolean);
static void HeapBrowseKeySig(short, Boolean);
static void HeapBrowseTimeSig(short, Boolean);
static void HeapBrowseBeamset(short, Boolean);
static void HeapBrowseConnect(short, Boolean);
static void HeapBrowseDynamic(short, Boolean);
static void HeapBrowseMODNR(short, Boolean);
static void HeapBrowseGraphic(short, Boolean);
static void HeapBrowseOttava(short, Boolean);
static void HeapBrowseSlur(short, Boolean);
static void HeapBrowseTuplet(short, Boolean);
static void HeapBrowseGRSync(short, Boolean);
static void HeapBrowseObject(short, Boolean);

/* ------------------------------------------------------------------ HeapBrowser -- */

void HeapBrowser(short heapNum)
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

	DialogPtr dlog;
	short itype, ditem;
	Handle tHdl;
	Boolean done;
	GrafPtr oldPort;
	short heapIndex, itemIndex, oldHeapIndex, oldItemIndex;
	
/* --- 1. Create the dialog and initialize its contents. --- */

	GetPort(&oldPort);
	dlog = GetNewDialog(HEAPBROWSER_DLOG, NULL, BRING_TO_FRONT);
	if (!dlog)
		MissingDialog(HEAPBROWSER_DLOG);
	SetPort(GetDialogWindowPort(dlog));
	GetDialogItem(dlog, hText, &itype, &tHdl, &bRect);

	ArrowCursor();

/*--- 2. Interact with user til they push Done. --- */

	done = False;
	TextFont(SYSFONTID_MONOSPACED);
	TextSize(9);
	heapIndex = heapNum;
	itemIndex = 0;
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
		  	done = True;
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

static void ShowHeap(short theHeap, short itemIndex)
{
	const char *ps;
	HEAP *myHeap;
	Boolean isFree;

	linenum = 1;
	myHeap = Heap + theHeap;
	ps = NameHeapType(theHeap, False);
	sprintf(str, "%s Heap [type %d]", ps, theHeap);
	HeapDrawLine(str);

	sprintf(str, "firstFree=%d nObjs=%d nFree=%d", 
		myHeap->firstFree, myHeap->nObjs, myHeap->nFree);
	HeapDrawLine(str);
	
	sprintf(str, "objSize=%d type=%d lockLevel=%d", 
		myHeap->objSize, myHeap->type, myHeap->lockLevel);
	HeapDrawLine(str);
	
	sprintf(str, "block=%lx", (unsigned long)myHeap->block);
	HeapDrawLine(str);
	
	HeapDrawLine("------------------");
	
	isFree = HeapLinkIsFree(myHeap, itemIndex);

	switch (theHeap) {
	case HEADERtype:
		HeapBrowsePartInfo(itemIndex, isFree);
		break;
	case SYNCtype:
		HeapBrowseSync(itemIndex, isFree);
		break;
	case RPTENDtype:
		HeapBrowseRepeatEnd(itemIndex, isFree);
		break;
	case STAFFtype:
		HeapBrowseStaff(itemIndex, isFree);
		break;
	case MEASUREtype:
		HeapBrowseMeasure(itemIndex, isFree);
		break;
	case CLEFtype:
		HeapBrowseClef(itemIndex, isFree);
		break;
	case KEYSIGtype:
		HeapBrowseKeySig(itemIndex, isFree);
		break;
	case TIMESIGtype:
		HeapBrowseTimeSig(itemIndex, isFree);
		break;
	case BEAMSETtype:
		HeapBrowseBeamset(itemIndex, isFree);
		break;
	case CONNECTtype:
		HeapBrowseConnect(itemIndex, isFree);
		break;
	case DYNAMtype:
		HeapBrowseDynamic(itemIndex, isFree);
		break;
	case MODNRtype:
		HeapBrowseMODNR(itemIndex, isFree);
		break;
	case GRAPHICtype:
		HeapBrowseGraphic(itemIndex, isFree);
		break;
	case OTTAVAtype:
		HeapBrowseOttava(itemIndex, isFree);
		break;
	case SLURtype:
		HeapBrowseSlur(itemIndex, isFree);
		break;
	case TUPLETtype:
		HeapBrowseTuplet(itemIndex, isFree);
		break;
	case GRSYNCtype:
		HeapBrowseGRSync(itemIndex, isFree);
		break;
	case OBJtype:
		HeapBrowseObject(itemIndex, isFree);
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

void HeapBrowsePartInfo(short itemIndex, Boolean isFree)
{
	LINK qL;
	PPARTINFO q;
	
	qL = itemIndex;
	
	q = GetPPARTINFO(qL);
	sprintf(str, "link=%u%s addr=%lx next=%u", qL, (isFree? "/FREE" : " "), (unsigned long)q,
			q->next);
	HeapDrawLine(str); q = GetPPARTINFO(qL);
	sprintf(str, "firstStaff=%d lastStaff=%d", q->firstStaff, q->lastStaff);
	HeapDrawLine(str);
}

void HeapBrowseStaff(short itemIndex, Boolean isFree)
{
	LINK qL;
	PASTAFF q;
	
	qL = itemIndex;
	
	q = GetPASTAFF(qL);
	sprintf(str, "link=%u%s staffn=%d next=%u", qL, (isFree? "/FREE" : " "), q->staffn, q->next);
	HeapDrawLine(str); q = GetPASTAFF(qL);
	sprintf(str, "visible=%s", q->visible ? "True" : "False");
	HeapDrawLine(str); q = GetPASTAFF(qL);
	sprintf(str, "staff top/left/right=%d %d %d",
				q->staffTop,
				q->staffLeft,
				q->staffRight);
	HeapDrawLine(str); q = GetPASTAFF(qL);
	sprintf(str, "staffHeight/Lines=%d  %d",
				q->staffHeight,
				q->staffLines);
	HeapDrawLine(str); q = GetPASTAFF(qL);
	sprintf(str, "fontSize=%d", q->fontSize);
	HeapDrawLine(str); q = GetPASTAFF(qL);
	sprintf(str, "ledger/noteHead/fracBeam=%d %d %d",
				q->ledgerWidth,
				q->noteHeadWidth,
				q->fracBeamWidth);
	HeapDrawLine(str); q = GetPASTAFF(qL);
	sprintf(str, "clefType=%d", q->clefType);
	HeapDrawLine(str); q = GetPASTAFF(qL);
	sprintf(str, "nKSItems=%hd", q->nKSItems);
	HeapDrawLine(str); q = GetPASTAFF(qL);
	sprintf(str, "timeSigType/n/d=%hd %hd/%hd",
				q->timeSigType,
				q->numerator,
				q->denominator);
	HeapDrawLine(str); q = GetPASTAFF(qL);
	sprintf(str, "dynamicType=%hd", q->dynamicType);
	HeapDrawLine(str);
}

void HeapBrowseConnect(short itemIndex, Boolean isFree)
{
	LINK qL;
	PACONNECT q;
	
	qL = itemIndex;
	
	q = GetPACONNECT(qL);
	sprintf(str, "link=%u%s next=%u", qL, (isFree? "/FREE" : " "), q->next);
	HeapDrawLine(str); q = GetPACONNECT(qL);
	
	sprintf(str, "connLevel=%hd", q->connLevel);
	HeapDrawLine(str); 	q = GetPACONNECT(qL);
	switch (q->connectType) {
	case CONNECTLINE:
		HeapDrawLine("connectType=CONNECTLINE");
		break;
	case CONNECTCURLY:
		HeapDrawLine("connectType=CONNECTCURLY");
		break;
	}
	sprintf(str, "staffAbove=%hd", q->staffAbove);
	HeapDrawLine(str); 	q = GetPACONNECT(qL);
	sprintf(str, "staffBelow=%hd", q->staffBelow);
	HeapDrawLine(str); 	q = GetPACONNECT(qL);
	sprintf(str, "firstPart=%d lastPart=%d", q->firstPart,q->lastPart);
	HeapDrawLine(str);
}

void HeapBrowseClef(short itemIndex, Boolean isFree)
{
	LINK qL;
	PACLEF q;
	
	qL = itemIndex;
	
	q = GetPACLEF(qL);
	sprintf(str, "link=%u%s staffn=%d next=%u", qL, (isFree? "/FREE" : " "), q->staffn, q->next);
	HeapDrawLine(str); 	q = GetPACLEF(qL);
	strcpy(str, "flags=");
	if (q->selected)
		strcat(str, "SELECTED ");
	if (q->visible)
		strcat(str, "VISIBLE ");
	if (q->soft)
		strcat(str, "SOFT ");
	HeapDrawLine(str); 	q = GetPACLEF(qL);
	sprintf(str, "xd=%d", q->xd);
	HeapDrawLine(str); 	q = GetPACLEF(qL);
	sprintf(str, "clefType=%d", q->subType);
	HeapDrawLine(str);
}

void HeapBrowseKeySig(short itemIndex, Boolean isFree)
{
	LINK qL;
	PAKEYSIG q;
	
	qL = itemIndex;
	
	q = GetPAKEYSIG(qL);
	sprintf(str, "link=%u%s staffn=%d next=%u", qL, (isFree? "/FREE" : " "), q->staffn, q->next);
	HeapDrawLine(str);	q = GetPAKEYSIG(qL);
	strcpy(str, "flags=");
	if (q->selected)
		strcat(str, "SELECTED ");
	if (q->visible)
		strcat(str, "VISIBLE ");
	if (q->soft)
		strcat(str, "SOFT ");
	HeapDrawLine(str); 	q = GetPAKEYSIG(qL);
	sprintf(str, "xd=%d", q->xd);
	HeapDrawLine(str);	q = GetPAKEYSIG(qL);
	sprintf(str, "nKSItems=%hd", q->nKSItems);
	HeapDrawLine(str);
}

void HeapBrowseTimeSig(short itemIndex, Boolean isFree)
{
	LINK qL;
	PATIMESIG q;
	
	qL = itemIndex;
	
	q = GetPATIMESIG(qL);
	sprintf(str, "link=%u%s staffn=%d next=%u", qL, (isFree? "/FREE" : " "), q->staffn, q->next);
	HeapDrawLine(str);	q = GetPATIMESIG(qL);
	strcpy(str, "flags=");
	if (q->selected)
		strcat(str, "SELECTED ");
	if (q->visible)
		strcat(str, "VISIBLE ");
	if (q->soft)
		strcat(str, "SOFT ");
	HeapDrawLine(str);	q = GetPATIMESIG(qL);
	sprintf(str, "xd=%d", q->xd);
	HeapDrawLine(str);	q = GetPATIMESIG(qL);
	sprintf(str, "timeSigType,n/d=%hd,%hd/%hd",
				q->subType,
				q->numerator,
				q->denominator);
	HeapDrawLine(str);
}

void HeapBrowseMeasure(short itemIndex, Boolean isFree)
{
	LINK qL;
	PAMEASURE q;

	qL = itemIndex;
	
	q = GetPAMEASURE(qL);
	sprintf(str, "link=%u%s staffn=%d next=%u", qL, (isFree? "/FREE" : " "), q->staffn, q->next);
	HeapDrawLine(str);	q = GetPAMEASURE(qL);
	strcpy(str, "flags=");
	if (q->selected)
		strcat(str, "SELECTED ");
	if (q->visible)
		strcat(str, "VISIBLE ");
	if (q->soft)
		strcat(str, "SOFT ");
	if (q->measureVisible)
		strcat(str, "MEASUREVIS ");
	if (q->connAbove)
		strcat(str, "CONNABOVE ");
	HeapDrawLine(str);	q = GetPAMEASURE(qL);
	sprintf(str, "measSizeRect=%d %d %d %d",
			q->measSizeRect.top, q->measSizeRect.left,
			q->measSizeRect.bottom, q->measSizeRect.right);
	HeapDrawLine(str);	q = GetPAMEASURE(qL);
	sprintf(str, "measureNum=%hd", q->measureNum);
	HeapDrawLine(str);	q = GetPAMEASURE(qL);
	sprintf(str, "connStaff=%hd barlineType=%hd", q->connStaff,
						q->subType);
	HeapDrawLine(str);	q = GetPAMEASURE(qL);
	sprintf(str, "clefType=%d", q->clefType);
	HeapDrawLine(str);	q = GetPAMEASURE(qL);
	sprintf(str, "nKSItems=%hd", q->nKSItems);
	HeapDrawLine(str);	q = GetPAMEASURE(qL);
	sprintf(str, "timeSigType/n/d=%hd %hd/%hd",
			q->timeSigType,
			q->numerator,
			q->denominator);
	HeapDrawLine(str);	q = GetPAMEASURE(qL);
	sprintf(str, "dynamicType=%hd", q->dynamicType);
	HeapDrawLine(str);
}

void HeapBrowseSync(short itemIndex, Boolean isFree)
{
	LINK qL;
	PANOTE q;
	
	qL = itemIndex;
	
	q = GetPANOTE(qL);
	sprintf(str, "link=%u%s staffn=%d voice=%hd next=%u", qL, (isFree? "/FREE" : " "),
				 q->staffn, q->voice, q->next);
	HeapDrawLine(str);	q = GetPANOTE(qL);
	strcpy(str, "flags=");
	if (q->selected)
		strcat(str, "SELECTED ");
	if (q->visible)
		strcat(str, "VISIBLE ");
	if (q->soft)
		strcat(str, "SOFT ");
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "xd=%d yd=%d yqpit=%hd ystem=%d", q->xd, q->yd, q->yqpit, q->ystem);
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "l_dur=%hd headShape=%d", q->subType, q->headShape);
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "playTDelta=%d playDur=%d noteNum=%hd", q->playTimeDelta, q->playDur,
														q->noteNum);
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "on,offVelocity=%hd,%hd", q->onVelocity, q->offVelocity);
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "ndots=%hd xMoveDots=%hd yMoveDots=%hd", q->ndots, q->xMoveDots,
					q->yMoveDots);
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "rest=%s", q->rest ? "True" : "False");
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "accident=%hd accSoft=%s", q->accident,
													  q->accSoft ? "True" : "False");
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "xmoveAcc=%hd", q->xmoveAcc);
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "beamed=%s otherStemSide==%s", q->beamed ? "True" : "False",
														q->otherStemSide ? "True" : "False");
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "inTuplet=%s inOttava=%s", q->inTuplet ? "True" : "False",
														 q->inOttava ? "True" : "False");
	HeapDrawLine(str);	q = GetPANOTE(qL);
	sprintf(str, "firstMod=%d", q->firstMod);
	HeapDrawLine(str);
}

void HeapBrowseBeamset(short itemIndex, Boolean isFree)
{
	LINK qL;
	PANOTEBEAM q;
	
	qL = itemIndex;
	
	q = GetPANOTEBEAM(qL);
	sprintf(str, "link=%u%s @%lx bpSync=%d next=%u", qL, (isFree? "/FREE" : " "),
			(unsigned long)q, q->bpSync, q->next);
	HeapDrawLine(str);
	
	sprintf(str, "startend=%d", q->startend);
	HeapDrawLine(str);	q = GetPANOTEBEAM(qL);
	sprintf(str, "fracs=%d", q->fracs);
	HeapDrawLine(str);	q = GetPANOTEBEAM(qL);
	sprintf(str, "fracGoLeft=%s", q->fracGoLeft ? "True" : "False");
	HeapDrawLine(str);
}

void HeapBrowseDynamic(short itemIndex, Boolean isFree)
{
	LINK qL;
	PADYNAMIC q;
	
	qL = itemIndex;
	
	q = GetPADYNAMIC(qL);
	sprintf(str, "link=%u%s staffn=%d next=%u", qL, (isFree? "/FREE" : " "), q->staffn,
				q->next);
	HeapDrawLine(str);	q = GetPADYNAMIC(qL);
	strcpy(str, "flags=");
	if (q->selected) strcat(str, "SELECTED ");
	if (q->visible) strcat(str, "VISIBLE ");
	if (q->soft) strcat(str, "SOFT ");
	HeapDrawLine(str);	q = GetPADYNAMIC(qL);
	sprintf(str, "xd,yd=%d,%d", q->xd, q->yd);
	HeapDrawLine(str);	q = GetPADYNAMIC(qL);
}

void HeapBrowseMODNR(short itemIndex, Boolean isFree)
{
	LINK qL;
	PAMODNR q;
	
	qL = itemIndex;
	
	q = GetPAMODNR(qL);
	sprintf(str, "link=%u%s next=%u", qL, (isFree? "/FREE" : " "), q->next);
	HeapDrawLine(str);	q = GetPAMODNR(qL);
	strcpy(str, "flags=");
	if (q->selected) strcat(str, "SELECTED ");
	if (q->visible) strcat(str, "VISIBLE ");
	if (q->soft) strcat(str, "SOFT ");
	HeapDrawLine(str);	q = GetPAMODNR(qL);
	sprintf(str, "xstd,ystdpit=%d,%d", q->xstd, q->ystdpit);
	HeapDrawLine(str);	q = GetPAMODNR(qL);
	sprintf(str, "modCode=%hd data=%d", q->modCode, q->data);
	HeapDrawLine(str);
}

void HeapBrowseRepeatEnd(short itemIndex, Boolean isFree)
{
	LINK qL;
	PARPTEND q;
	
	qL = itemIndex;
	
	q = GetPARPTEND(qL);
	sprintf(str, "link=%u%s staff=%d next=%u", qL, (isFree? "/FREE" : " "), q->staffn,
				q->next);
	HeapDrawLine(str);	q = GetPARPTEND(qL);
	strcpy(str, "flags=");
	if (q->selected) strcat(str, "SELECTED ");
	if (q->visible) strcat(str, "VISIBLE ");
	if (q->soft) strcat(str, "SOFT ");
	HeapDrawLine(str);	q = GetPARPTEND(qL);
	sprintf(str, "connAbove=%d connStaff=%d", q->connAbove, q->connStaff);
	HeapDrawLine(str);
}

void HeapBrowseSlur(short itemIndex, Boolean isFree)
{
	LINK qL;
	PASLUR q;
	
	qL = itemIndex;
	
	q = GetPASLUR(qL);
	
	sprintf(str, "link=%u%s next=%u", qL, (isFree? "/FREE" : " "), q->next);
	HeapDrawLine(str);
		
	sprintf(str, "knot=%d %d", q->seg.knot.h, q->seg.knot.v);
	HeapDrawLine(str);	q = GetPASLUR(qL);
	sprintf(str, "c0=%d %d", q->seg.c0.h, q->seg.c0.v);
	HeapDrawLine(str);	q = GetPASLUR(qL);
	sprintf(str, "c1=%d %d", q->seg.c1.h, q->seg.c1.v);
	HeapDrawLine(str);	q = GetPASLUR(qL);
	sprintf(str, "endKnot=%d %d", q->endKnot.h, q->endKnot.v);
	HeapDrawLine(str);
}

void HeapBrowseTuplet(short itemIndex, Boolean isFree)
{
	LINK qL;
	PANOTETUPLE q;
	
	qL = itemIndex;
	
	q = GetPANOTETUPLE(qL);
	sprintf(str, "link=%u%s next=%u", qL, (isFree? "/FREE" : " "), q->next);
	HeapDrawLine(str);
}

void HeapBrowseGraphic(short itemIndex, Boolean isFree)
{
	LINK qL;
	PAGRAPHIC q;
	
	qL = itemIndex;
	
	q = GetPAGRAPHIC(qL);
	sprintf(str, "link=%u%s @%lx strOffset=%ld next=%u", qL, (isFree? "/FREE" : " "),
			(unsigned long)q, q->strOffset, q->next);
	HeapDrawLine(str);
}


void HeapBrowseOttava(short itemIndex, Boolean isFree)
{
	LINK qL;
	PANOTEOTTAVA q;
	
	qL = itemIndex;
	
	q = GetPANOTEOTTAVA(qL);
	sprintf(str, "link=%u%s @%lx opSync=%d next=%u", qL, (isFree? "/FREE" : " "),
				(unsigned long)q, q->opSync, q->next);
	HeapDrawLine(str);
}

void HeapBrowseGRSync(short itemIndex, Boolean isFree)
{
	LINK qL;
	PAGRNOTE q;
	
	qL = itemIndex;
	
	q = GetPAGRNOTE(qL);
	sprintf(str, "link=%u%s next=%u", qL, (isFree? "/FREE" : " "), q->next);
	HeapDrawLine(str);
}

void HeapBrowseObject(short itemIndex, Boolean isFree)
{
	LINK qL;
	PMEVENT q;
	Rect r;

	qL = itemIndex;
	
	q = GetPMEVENT(qL);
	sprintf(str, "link=%u%s right=%u left=%u", qL, (isFree? "/FREE" : " "), q->right, q->left);
	HeapDrawLine(str); q = GetPMEVENT(qL);
	
	sprintf(str, "xd=%d yd=%d type=%s nEntries=%d", 
		q->xd, q->yd, NameObjType(qL), q->nEntries);
	HeapDrawLine(str); q = GetPMEVENT(qL);
	sprintf(str, "selected=%d visible=%d soft=%d", q->selected, q->visible, q->soft);
	HeapDrawLine(str); q = GetPMEVENT(qL);
	sprintf(str, "valid=%d tweaked=%d", q->valid, q->tweaked);
	HeapDrawLine(str); q = GetPMEVENT(qL); r = q->objRect;
	sprintf(str, "rect.l,t,r,b=%d %d %d %d", r.left, r.top, r.right, r.bottom);
	HeapDrawLine(str);
}

#endif /* PUBLIC_VERSION */
