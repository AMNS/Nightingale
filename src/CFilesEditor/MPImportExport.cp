/*								NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-98 by Advanced Music Notation Systems. All Rights Reserved.
 *
 */

/* MPImportExport.c - functions for importing/exporting editing done in
Master Page, built on the old Score Dialog - rev. for v.3.1. */
 
#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"




/* Prototypes for internal functions. */

static void GroupMeas(Document *doc,LINK measL,short firstStf,short lastStf);
static void UngroupMeas(Document *doc,LINK measL,short firstStf,short lastStf);
static void GroupPart(Document *doc,short firstStf,short lastStf,Boolean connectBars,
					short connectType);
static void UngroupPart(Document *doc,short firstStf,short lastStf);

static void StoreConnectGroup(LINK,LINK);
static void StoreAllConnects(LINK);
static void DelConnGroup(Document *, LINK, LINK);
static void DelBadGroupConnects(Document *);


/* ================================================================================= */
/* Functions for importing/exporting editing done in Master Page. Editing includes
insertion and deletion of parts. */

/* ================================================================================= */
/* Functions for exporting environment */

/* Export Master Page's Part list to the score header. */

void ExportPartList(Document *doc)
{
	LINK partL,mPartL,tempL;
	PPARTINFO pPart, pmPart;

	if (LinkNENTRIES(doc->masterHeadL)!=LinkNENTRIES(doc->headL))
		MayErrMsg("ExportPartList: LinkNENTRIES(masterHeadL)=%ld but LinkNENTRIES(headL)=%ld",
					(long)LinkNENTRIES(doc->masterHeadL), (long)LinkNENTRIES(doc->headL));

	/* Update headL's linked subobject list of parts. For each part, save its next
		part as tempL; copy into it the corresponding part from Master Page; and then
		restore its next part link: pPart->next = tempL. */

	mPartL = FirstSubLINK(doc->masterHeadL);
	partL = FirstSubLINK(doc->headL);
	for ( ; mPartL; mPartL = NextPARTINFOL(mPartL)) {
		tempL = NextPARTINFOL(partL);
		pmPart = GetPPARTINFO(mPartL);
		pPart = GetPPARTINFO(partL);
		*pPart = *pmPart;
		pPart->next = tempL;
		if (!tempL) break;
		partL = tempL;
	}
}


/* Export the Master Page environment: the only things this needs to export any more
are Master Page's Parts and staffTops. */

void ExportEnvironment(Document *doc,
						DDIST */*staffTop*/)		/* no longer used */
{	
	LINK		staffL, aStaffL, mStaffL, maStaffL;
	
	ExportPartList(doc);
	
	if (doc->partChangedMP) {
		mStaffL = LSSearch(doc->masterHeadL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
		staffL = LSSearch(doc->headL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
		for ( ; staffL; staffL = LinkRSTAFF(staffL)) {
			aStaffL = FirstSubLINK(staffL);
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
				maStaffL = StaffOnStaff(mStaffL, StaffSTAFF(aStaffL));
				StaffTOP(aStaffL) = StaffTOP(maStaffL);
			}
		}
	}
}


/* ------------------------------------------------ MPStaff2Part, MPFindPartInfo -- */

/* Given a staff number, returns part it belongs to. */

static short MPStaff2Part(Document *, short);
static short MPStaff2Part(Document *doc, short nstaff)
{
	short			np;
	PPARTINFO	pPart;
	LINK			partL;
	
	if (nstaff<=0) return 0;
	partL = NextPARTINFOL(FirstSubLINK(doc->masterHeadL));
	for (np = 1; partL; np++, partL=NextPARTINFOL(partL)) {
		pPart = GetPPARTINFO(partL);
		if (pPart->lastStaff>=nstaff) return np;
	}
	MayErrMsg("MPStaff2Part: staff no. %ld is not in any part.", (long)nstaff);
	return 0;
}

LINK MPFindPartInfo(Document *, short);
LINK MPFindPartInfo(Document *doc, short partn)
{
	short 		np;
	LINK		partL;
	
	if (partn>LinkNENTRIES(doc->masterHeadL)) return NILINK;
	
	for (np = 0, partL = FirstSubLINK(doc->masterHeadL); np<partn; 
			np++, partL = NextPARTINFOL(partL))	
		;
	return partL;
}

/* Run Instrument dialog to edit information for the part staff <firstStf> belongs
to. Return TRUE if user OKs the dialog, FALSE if cancel. */

short SDInstrDialog(Document *doc, short firstStf)
{
	short partn,changed; LINK partL; PPARTINFO pPart; PARTINFO partInfo;

	partn = MPStaff2Part(doc,firstStf);
	partL = MPFindPartInfo(doc, partn);
	pPart = GetPPARTINFO(partL);
	partInfo = *pPart;
	if (useWhichMIDI == MIDIDR_CM) {
		MIDIUniqueID device = GetCMDeviceForPartn(doc, partn);
		if ((changed = CMInstrDialog(doc, &partInfo, &device))!=0) {
			MPSetInstr(doc, &partInfo);
			SetCMDeviceForPartn(doc, partn, device);
			//SetCMDeviceForPartL(doc, partL, partn, device);
			doc->masterChanged = TRUE;
		}
	}
	else
		if ((changed = InstrDialog(doc, &partInfo))!=0) {
			MPSetInstr(doc, &partInfo);
			doc->masterChanged = TRUE;
		}

	return changed;
}


/* ============================================== Functions to add change records == */
/* Strategy: as the user is working in Master Page, giving commands that change the
structure of the score--Add Parts, Group Parts, etc.--we update the Master Page
object list immediately, but we don't want to touch the main object list till they're
done, since they might cancel. So we just make a list of "change records" describing
the changes to be made; then, if they tell us to keep the changes, we go thru the
change records and execute them. */

static Boolean TooManyChanges(Document *);
static Boolean TooManyChanges(Document *doc)
{
	if (doc->nChangeMP>=MAX_CHANGES) {
		GetIndCString(strBuf, MPERRS_STRS, 12);    /* "Too many Adds, Deletes, Groups, Ungroups, and Make One-Staff Parts in one batch." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return TRUE;
	}
	
	return FALSE;
}

/* Delete a part in Master Page. */

void DelChangePart(Document *doc, short firstStf, short lastStf)
{
	if (TooManyChanges(doc)) return;
	
	doc->change[doc->nChangeMP].oper = SDDelete;
	doc->change[doc->nChangeMP].staff = firstStf;
	doc->change[doc->nChangeMP].p1 = lastStf;
	doc->nChangeMP++;
	doc->masterChanged = doc->partChangedMP = TRUE;
}

/* Add a part in Master Page. */

void AddChangePart(Document *doc, short firstStf, short nadd, short nper, short showLines)
{
	static Boolean firstDelAdd = TRUE;
	char msg[20];

	if (nper<1 || nper>MAXSTPART) {
		sprintf(msg, "%d", MAXSTPART);
		CParamText(msg, "", "", "");
		StopInform(ILLNSP_ALRT);
	}
#ifdef NOMORE_JGG
	else if (nadd<=0 || (nadd*nper)+doc->nstavesMP>MAXSTAVES) {
		sprintf(msg, "%d", MAXSTAVES);
		CParamText(msg, "", "", "");
		StopInform(ILLNS_ALRT);
	}
#else
	/* Note that doc->nstavesMP has already been updated to reflect added staves.  -JGG */
	else if (nadd<=0 || doc->nstavesMP>MAXSTAVES) {
		sprintf(msg, "%d", MAXSTAVES);
		CParamText(msg, "", "", "");
		StopInform(ILLNS_ALRT);
	}
#endif
	else {
		if (TooManyChanges(doc)) return;

		doc->change[doc->nChangeMP].oper = SDAdd;				/* Save info to change */
		doc->change[doc->nChangeMP].staff = firstStf;		/*   main object list */
		doc->change[doc->nChangeMP].p1 = nper;
		doc->change[doc->nChangeMP].p2 = nadd;
		doc->change[doc->nChangeMP].p3 = showLines;
		doc->nChangeMP++;
		
		doc->masterChanged = doc->partChangedMP = TRUE;
	}
}


void Make1StaffChangeParts(Document *doc, short stf, short nadd, short showLines)
{
	if (TooManyChanges(doc)) return;
	
	doc->change[doc->nChangeMP].oper = SDMake1;
	doc->change[doc->nChangeMP].staff = stf;
	doc->change[doc->nChangeMP].p1 = nadd;
	doc->change[doc->nChangeMP].p2 = showLines;
	doc->nChangeMP++;

	doc->masterChanged = doc->partChangedMP = TRUE;
}


/* Add a change record of type SDGroup to export a part grouping from Master Page. */

void GroupChangeParts(Document *doc,
							short firstStf, short lastStf,
							Boolean connectBars,				/* TRUE=extend barlines to connect staves */ 
							short connectType					/* CONNECTBRACKET or CONNECTCURLY */
							)
{
	if (TooManyChanges(doc)) return;

	doc->change[doc->nChangeMP].oper = SDGroup;
	doc->change[doc->nChangeMP].staff = firstStf;
	doc->change[doc->nChangeMP].p1 = lastStf;
	doc->change[doc->nChangeMP].p2 = connectBars;
	doc->change[doc->nChangeMP].p3 = connectType;
	doc->nChangeMP++;
	doc->masterChanged = doc->grpChangedMP = TRUE;
}

/* Add a change record of type SDUngroup to export a part ungrouping from Master Page. */

void UngroupChangeParts(Document *doc, short firstStf, short lastStf)
{
	if (TooManyChanges(doc)) return;

	doc->change[doc->nChangeMP].oper = SDUngroup;
	doc->change[doc->nChangeMP].staff = firstStf;
	doc->change[doc->nChangeMP].p1 = lastStf;
	doc->nChangeMP++;

	doc->masterChanged = doc->grpChangedMP = TRUE;
}


/* ================================================================================= */
/* Change the connStaff and connAbove fields of measure subobjects of measL on
staves [firstStf,lastStf] to reflect the grouping or ungrouping parts in that
range. */

static void GroupMeas(Document */*doc*/, LINK measL, short firstStf, short lastStf)
{
	LINK aMeasL; PAMEASURE aMeas;
	
	aMeasL = FirstSubLINK(measL);
	
	for ( ; aMeasL; aMeasL=NextMEASUREL(aMeasL)) {
		if (MeasureSTAFF(aMeasL) == firstStf) {
			aMeas = GetPAMEASURE(aMeasL);
			aMeas->connAbove = FALSE;
			aMeas->connStaff = lastStf;
		}
		if (MeasureSTAFF(aMeasL)>firstStf && MeasureSTAFF(aMeasL)<=lastStf) {
			aMeas = GetPAMEASURE(aMeasL);
			aMeas->connAbove = TRUE;
			aMeas->connStaff = 0;
		}		
	}
}

static void UngroupMeas(Document *doc, LINK measL, short firstStf, short lastStf)
{
	LINK aMeasL, aPartL; PAMEASURE aMeas;
	short partFirstStf, partLastStf, s;
	
	aMeasL = FirstSubLINK(measL);
	
	for ( ; aMeasL; aMeasL=NextMEASUREL(aMeasL))
		if (MeasureSTAFF(aMeasL)>=firstStf && MeasureSTAFF(aMeasL)<=lastStf) {
			aMeas = GetPAMEASURE(aMeasL);
			aMeas->connAbove = FALSE;
			aMeas->connStaff = 0;
		}

	/*
	 * We've now separated every barline in the range of staves into separate pieces.
	 * In any multistaff parts, reconnect them so they extend across all the part's
	 * staves. NB: This wouldn't be hard to do much more efficiently, if it matters.
	 */
	aPartL = FirstSubLINK(doc->headL);
	for ( ; aPartL; aPartL = NextPARTINFOL(aPartL)) {
		partFirstStf = PartFirstSTAFF(aPartL);
		partLastStf = PartLastSTAFF(aPartL);
		if (partLastStf-partFirstStf>0) {
			aMeasL = MeasOnStaff(measL, partFirstStf);
			aMeas = GetPAMEASURE(aMeasL);
			aMeas->connStaff = partLastStf;
			for (s = partFirstStf+1; s<=partLastStf; s++) {
				aMeasL = MeasOnStaff(measL, s);
				aMeas = GetPAMEASURE(aMeasL);
				aMeas->connAbove = TRUE;
			}
		}
	}
}

/* Export a part grouping from Master Page by creating a new subobject in every
Connect in the given score, and by changing subobjects in every Measure so barlines
extend across the group. To make things easier on the calling routines, we leave the
new Connect subobject's firstPart and lastPart blank; these must be filled in before
exiting Master Page. */

#define CONNECT_GROUP CONNECTBRACKET	/* ??MPImportExport.c and MPCommands must agree! */

static void GroupPart(Document *doc,
							short firstStf, short lastStf,
							Boolean connectBars,				/* TRUE=extend barlines to connect staves */ 
							short connectType
							)
{
	LINK connectL,aConnectL,connList,measL;
	PACONNECT aConnect; DDIST connWidth;

	connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);

	for ( ; connectL; connectL=LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) {

		/* Get the Connect subObj immediately following the range to be grouped */

		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
			aConnect = GetPACONNECT(aConnectL);
			if (aConnect->staffAbove >= lastStf) break;
		}

		connList = HeapAlloc(CONNECTheap,1);
		if (!connList) { NoMoreMemory(); return; }		/* ??leaves doc slightly inconsistent but maybe not bad */

		FirstSubLINK(connectL) = InsertLink(CONNECTheap,FirstSubLINK(connectL),aConnectL,connList);
		LinkNENTRIES(connectL)++;

		aConnectL = connList;

		aConnect = GetPACONNECT(aConnectL);
		aConnect->connLevel = GroupLevel;
		aConnect->connectType = connectType;
		connWidth = ConnectDWidth(doc->srastralMP, connectType);
		aConnect->xd = -connWidth;
		aConnect->staffAbove = firstStf;
		aConnect->staffBelow = lastStf;

		aConnect->firstPart = aConnect->lastPart = NILINK;		/* ??Surprise: these fields are unused! */

		/* Move all Connect subObjs that are nested within the new one to
			the left by the new one's width. */
			
		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
			if (aConnectL!=connList) {
				aConnect = GetPACONNECT(aConnectL);
				if (aConnect->staffAbove >= firstStf && aConnect->staffBelow <= lastStf)
					aConnect->xd -= connWidth;
			}
		}
	}

	if (connectBars) {
		measL = SSearch(doc->headL,MEASUREtype,GO_RIGHT);
	
		for ( ; measL; measL=LinkRMEAS(measL))
			GroupMeas(doc,measL,firstStf,lastStf);
	}

	doc->masterChanged = TRUE;
}

/* Export a part ungrouping from Master Page by removing the appropriate subobject
from every Connect in the given score, and by changing subobjects in every Measure
so barlines no longer extend across the group. */

static void UngroupPart(Document *doc, short firstStf, short lastStf)
{
	LINK connectL,aConnectL,bConnectL,nextConnL,measL;
	PACONNECT aConnect,bConnect; DDIST connWidth; short minGrpStf,maxGrpStf; 

	connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);

	for ( ; connectL; connectL=LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) {

		/* Get the Connect grouping the range to be ungrouped */

		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = nextConnL) {
			nextConnL = NextCONNECTL(aConnectL);
			aConnect = GetPACONNECT(aConnectL);
			
		if (aConnect->connLevel==GroupLevel)
			if (	(aConnect->staffAbove <= firstStf && aConnect->staffBelow >= firstStf) ||
					(aConnect->staffAbove >= firstStf && aConnect->staffBelow <= lastStf) ||
					(aConnect->staffAbove <= lastStf && aConnect->staffBelow >= lastStf) ) {

				connWidth = ConnectDWidth(doc->srastralMP, aConnect->connectType);
				minGrpStf = aConnect->staffAbove;
				maxGrpStf = aConnect->staffBelow;

				/* Move all Connect subObjs that were nested within the deleted Connect to
					the right by the deleted Connect's width. */
					
				bConnectL = FirstSubLINK(connectL);
				for ( ; bConnectL; bConnectL = NextCONNECTL(bConnectL)) {
					bConnect = GetPACONNECT(bConnectL);
					if ((bConnect->staffAbove >= minGrpStf && bConnect->staffBelow <= maxGrpStf) )
						bConnect->xd += connWidth;
				}

				RemoveLink(connectL,CONNECTheap,FirstSubLINK(connectL),aConnectL);
				HeapFree(CONNECTheap, aConnectL);
				LinkNENTRIES(connectL)--;
			}
		}
	}

	measL = SSearch(doc->headL,MEASUREtype,GO_RIGHT);

	for ( ; measL; measL=LinkRMEAS(measL))
		UngroupMeas(doc,measL,firstStf,lastStf);
}

/* Store the PARTINFO LINKs to the parts connected by the given GroupLevel Connect
subObj. The firstStaff and lastStaff fields of the Connect subObj must be set. */

void StoreConnectGroup(LINK headL, LINK aConnectL)
{
	LINK partL; PACONNECT aConnect;
	short firstStf, lastStf;

	aConnect = GetPACONNECT(aConnectL);
	
	if (aConnect->connLevel!=GroupLevel) return;

	firstStf = aConnect->staffAbove;
	lastStf = aConnect->staffBelow;

	partL = NextPARTINFOL(FirstSubLINK(headL));
	for ( ; partL; partL = NextPARTINFOL(partL))
		if (PartFirstSTAFF(partL)==firstStf)
			{ aConnect->firstPart = partL; break; }

	partL = NextPARTINFOL(FirstSubLINK(headL));
	for ( ; partL; partL = NextPARTINFOL(partL))
		if (PartLastSTAFF(partL)==lastStf)
			{ aConnect->lastPart = partL; break; }
}

/* Call StoreConnectPart or StoreConnectGroup for all subobjects of all Connects in
the object list beginning at headL. */
	
void StoreAllConnects(LINK headL)
{
	LINK connectL,aConnectL; PACONNECT aConnect;

	connectL = SSearch(headL,CONNECTtype,GO_RIGHT);
	
	for ( ; connectL; connectL=LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) {
		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
			aConnect = GetPACONNECT(aConnectL);
			if (aConnect->connLevel==PartLevel) StoreConnectPart(headL,aConnectL);
			if (aConnect->connLevel==GroupLevel) StoreConnectGroup(headL,aConnectL);
		}
	}
}

/* Remove the given Group Connect and adjust any Connect subobjects nested within it. */

void DelConnGroup(Document *doc, LINK connectL, LINK aConnectL)
{
	PACONNECT aConnect, bConnect; LINK bConnectL;
	short minGrpStf, maxGrpStf; DDIST connWidth;
	
	/* Move all Connect subObjs that were nested within the deleted Connect to
		the right by the deleted Connect's width. */

	aConnect = GetPACONNECT(aConnectL);
	minGrpStf = aConnect->staffAbove;
	maxGrpStf = aConnect->staffBelow;
	connWidth = ConnectDWidth(doc->srastral, aConnect->connectType);
		
	bConnectL = FirstSubLINK(connectL);
	for ( ; bConnectL; bConnectL = NextCONNECTL(bConnectL)) {
		bConnect = GetPACONNECT(bConnectL);
		if (bConnect->staffAbove>=minGrpStf && bConnect->staffBelow<=maxGrpStf)
			bConnect->xd += connWidth;
	}

	RemoveLink(connectL,CONNECTheap,FirstSubLINK(connectL),aConnectL);
	HeapFree(CONNECTheap, aConnectL);
	LinkNENTRIES(connectL)--;
}

/* Look at every Connect object in the score; if we find any GroupLevel subobjects
that group fewer than 2 parts, presumably because of deleted parts, remove them. */

void DelBadGroupConnects(Document *doc)
{
	LINK connectL, aConnectL; PACONNECT aConnect; short partAbove, partBelow;
	LINK nextL;

	connectL = SSearch(doc->headL,CONNECTtype,GO_RIGHT);
	for ( ; connectL;
		 		connectL=LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) {
		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL=nextL) {
			nextL = NextCONNECTL(aConnectL);						/* In case this one gets removed */
			aConnect = GetPACONNECT(aConnectL);
			if (aConnect->connLevel==GroupLevel) {
				partAbove = Staff2Part(doc, aConnect->staffAbove);
				partBelow = Staff2Part(doc, aConnect->staffBelow);
				if (partBelow-partAbove<1) DelConnGroup(doc, connectL, aConnectL);
			}
		}
	}
}


/* Add <nadd> parts of one staff each AFTER prevPartL to doc's object list. Do not do
do anything to other staves in the object list; in particular, do not increase staff
nos. of parts below. */

static Boolean Add1StaffParts(Document *, LINK, short, short);
static Boolean Add1StaffParts(Document *doc, LINK prevPartL, short nadd, short /*showLines*/)
{
	LINK nextPartL,aPartL,partList,listHead;
	short j,lastStf,nextStf;
	
	/* Allocate and initialize a list of <nadd> parts and insert it into the list of
		parts after prevPartL. */

	partList = HeapAlloc(PARTINFOheap,nadd);
	if (!partList) { NoMoreMemory(); return FALSE; }

	nextPartL = NextPARTINFOL(prevPartL);								/* OK if it's NILINK */

	aPartL = partList;
	for ( ; aPartL; aPartL = NextPARTINFOL(aPartL))
		InitPart(aPartL, 0, 0);
	
	listHead = InsertLink(PARTINFOheap,FirstSubLINK(doc->headL),nextPartL,partList);
	FirstSubLINK(doc->headL) = listHead;
	LinkNENTRIES(doc->headL) += nadd;

	/* Set the new parts' firstStaff and lastStaff fields from the part immediately
		before them. */

	aPartL = prevPartL;
	lastStf = PartLastSTAFF(aPartL);

	nextStf = lastStf+1;
	aPartL = NextPARTINFOL(aPartL);
	for (j = 0; j<nadd && aPartL; j++, aPartL = NextPARTINFOL(aPartL)) {
		PartFirstSTAFF(aPartL) = nextStf;
		PartLastSTAFF(aPartL) = nextStf;
		nextStf = PartLastSTAFF(aPartL)+1;
	}

	InvalWindow(doc);
	return TRUE;
}
	
#define CONNECT_GROUP CONNECTBRACKET	/* Default type ??must agree with MPImportExport.c! */

/* Finish converting an n-staff part into n 1-staff parts: replace the original part's
multistaff-part Connects with group Connects, and build a new voice table. */

static void Finish1StaffParts(Document *, LINK, LINK);
static void Finish1StaffParts(Document *doc, LINK origPartL, LINK lastPartL)
{
	LINK connectL, aConnectL; PACONNECT aConnect; DDIST squareWider; short firstStf;
	
	connectL = LSSearch(doc->headL,CONNECTtype,ANYONE,GO_RIGHT,FALSE);
	
	for ( ; connectL;
			connectL = LSSearch(RightLINK(connectL),CONNECTtype,ANYONE,GO_RIGHT,FALSE)) {
		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
			aConnect = GetPACONNECT(aConnectL);
			firstStf = PartFirstSTAFF(origPartL);		/* Since aConnect->firstPart is unreliable! */
			if (aConnect->staffAbove==firstStf && aConnect->connLevel==PartLevel) {
				aConnect->connLevel = GroupLevel;
				aConnect->connectType = CONNECT_GROUP;
				squareWider = ConnectDWidth(doc->srastral, CONNECT_GROUP)
									-ConnectDWidth(doc->srastral, CONNECTCURLY);
				aConnect->xd -= squareWider;
				aConnect->lastPart = lastPartL;
			}
		}
	}
	
	BuildVoiceTable(doc, TRUE);
}

static void Make1StaffParts(Document *, short, short, short);
static void Make1StaffParts(Document *doc, short staff, short nStavesAdd, short showLines)
{
	LINK partL, thePartL=NILINK, lastPartL; short firstStf;
	
	partL = FirstSubLINK(doc->headL);
	for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL)) {
		firstStf = PartFirstSTAFF(partL);
		if (firstStf>=staff) { thePartL = partL; break; }
	}
	if (!thePartL) return;
	
	PartLastSTAFF(thePartL) = PartFirstSTAFF(thePartL);
	if (lastPartL = Add1StaffParts(doc, thePartL, nStavesAdd, showLines))  
		Finish1StaffParts(doc, thePartL, lastPartL);
}

/* Export changes (Add/Delete Part, Group/Ungroup Parts, Make 1-staff Parts) from
Master Page to the main object list. Should be called when exiting Master Page. */
 
short ExportChanges(Document *doc)
{
	short i,cstaff,npa, savedAutoRespace;

 	if (doc->nChangeMP>=MAX_CHANGES) {
 		MayErrMsg("ExportChanges: Too many changes: doc->nChangeMP=%ld.", (long)doc->nChangeMP);
 		return FALSE;
 	}

	/* Turn off autoRespace temporarily, to prevent bad meas XD's when deleting
		parts that have content.  -JGG, 7/20/00 */
 	savedAutoRespace = doc->autoRespace;
	doc->autoRespace = FALSE;

	for (i = 0; i<doc->nChangeMP; i++) {
   	switch (doc->change[i].oper) {
   		case SDAdd:
				cstaff = doc->change[i].staff;
				for (npa = 1; npa<=doc->change[i].p2; npa++) {
					if (!AddPart(doc, cstaff, doc->change[i].p1, doc->change[i].p3)) break;
					cstaff += doc->change[i].p1;
				}
  				break;
	 		case SDDelete:
				DeletePart(doc, doc->change[i].staff, doc->change[i].p1);
   			break;
   		case SDGroup:
				GroupPart(doc, doc->change[i].staff, doc->change[i].p1, doc->change[i].p2,
								doc->change[i].p3);
  				break;
   		case SDUngroup:
				UngroupPart(doc, doc->change[i].staff, doc->change[i].p1);
				break;
			case SDMake1:
				Make1StaffParts(doc, doc->change[i].staff, doc->change[i].p1,
										doc->change[i].p2);  
				break;
			default:
				;
   	}
	}
	doc->autoRespace = savedAutoRespace;

 	if (doc->masterChanged) {
		WaitCursor();
		if (doc->masterChanged || doc->partChangedMP)
			ExportEnvironment(doc,doc->staffTopMP);				/* Export local version of score format */
	 	StoreAllConnects(doc->headL);
		/*
		 * As a result of deleting parts, there may be Connect GroupLevel subobjs
		 * around that have only one part left in their range (any that have no parts
		 * left should have been removed by DeletePart). Get rid of them.
		 */
	 	DelBadGroupConnects(doc);
	 	
	 	if (doc->partChangedMP || doc->fixMeasRectYs) {
			/* Call FixMeasRectYs to fix measure & system tops & bottoms. Tell it to fix
				up the entire score and to derive information from the master System. */
	
			FixLedgerYSp(doc);
	 		FixMeasRectYs(doc, NILINK, TRUE, FALSE, TRUE);
	 	}
	
		ArrowCursor();
	}
	
	doc->nChangeMP = 0;

	return TRUE;
}
