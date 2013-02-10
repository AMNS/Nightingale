/* MPCommands.c for Nightingale - functions to handle Master Page menu commands. */

/*										NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1990-99 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 *
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Local prototypes */

static Boolean CanDeletePart(Document *doc,short minStf,short maxStf);
static Boolean OnlyOnePart(Document *doc,short minStf,short maxStf);
static void MPRemovePart(Document *, LINK);
static void MPDeleteSelRange(Document *doc,LINK partL);

static void MPDefltStfCtxt(LINK aStaffL);
static Boolean ChkStfHt(Document *doc,short nadd,short nper);
static Boolean MPAddChkVars(Document *,short nadd,short nper);
static Boolean MPAddPartDialog(short *, short *);

static void InsertPartMP(Document *doc,LINK partL,short nadd,short nper,short showLines);


#define DELETE_PART_DI	2			/* Delete Part button in DELPART_ANYWAY_ALRT */

/* Check if user is trying to delete a non-empty part: if so, ask them if they
really want to delete the part.  If Nightingale should delete the part, return
TRUE, else return FALSE. */

static Boolean CanDeletePart(
					Document *doc,
					short minStf, short maxStf)		/* inclusive range of staves */
{
	LINK pL, usedL;
	short staff, usedStaff=-1;
	short measNum;
	short alrtChoice;
	Boolean reallyUsed=FALSE;
	char fmtStr[256], thoughStr[256];

	/*
	 * Look for anything in the staff range, especially "real" things (i.e., not
	 * just key sigs. or time sigs.). This is subtler than it looks: e.g., getting
	 * the first used meas. no. requires swapping the two loops.
	 */ 
	for (staff = minStf; staff<=maxStf; staff++) {
		for (pL = doc->headL; pL && !reallyUsed;
			  pL = LSSearch(RightLINK(pL), ANYTYPE, staff, GO_RIGHT, FALSE)) {
			switch (ObjLType(pL)) {
				case HEADERtype:
				case TAILtype:
				case PAGEtype:
				case SYSTEMtype:
				case STAFFtype:
				case MEASUREtype:
				case CONNECTtype:
					break;													/* Safe to delete, keep looking */
				case KEYSIGtype:
					if (!LinkSOFT(pL) && KeySigINMEAS(pL)) { usedStaff = staff; usedL = pL; }
					break;
				case TIMESIGtype:
					if (!LinkSOFT(pL) && TimeSigINMEAS(pL))  { usedStaff = staff; usedL = pL; }
					break;
				default:
					if (!LinkSOFT(pL)) { usedStaff = staff; usedL = pL; reallyUsed = TRUE; }
			}
		}
	}

	if (usedStaff<0) return TRUE;

	measNum = GetMeasNum(doc, usedL);
	GetIndCString(fmtStr, MPERRS_STRS, 2);			/* "Staff %d is not empty starting at measure %d%s" */
	GetIndCString(thoughStr, MPERRS_STRS, (reallyUsed? 3 : 4));	/* "though it contains only KS/TS" */
	sprintf(strBuf, fmtStr, usedStaff, measNum, thoughStr);
	CParamText(strBuf, "", "", "");
	PlaceAlert(DELPART_ANYWAY_ALRT, doc->theWindow, 0, 60);
 	alrtChoice = CautionAdvise(DELPART_ANYWAY_ALRT);
	return (alrtChoice==DELETE_PART_DI);
}


/* Return FALSE if more than one part is in the given range of staves. */

static Boolean OnlyOnePart(Document *doc, short minStf, short maxStf)
{
	LINK minPartL,maxPartL;

	minPartL = Staff2PartL(doc,doc->masterHeadL,minStf);
	maxPartL = Staff2PartL(doc,doc->masterHeadL,maxStf);

	return (minPartL==maxPartL);
}


/* Given two staff numbers in the current Master Page, find the corresponding staff
numbers in the score. If there are no corresponding numbers in the score
(because the staves were just added in Master Page), return NOONE. */

void Map2OrigStaves(Document *, short, short, short *, short *);
void Map2OrigStaves(Document *doc, short minStf, short maxStf, short *pOrigMinStf,
					short *pOrigMaxStf)
{
	short s, i, nGone, origStart, newStart; short newStaff[MAXSTAVES+1];
	
	*pOrigMinStf = *pOrigMaxStf = NOONE;

	for (s = 1; s<=doc->nstaves; s++)
		newStaff[s] = s;
	for (s = doc->nstaves+1; s<=MAXSTAVES; s++)
		newStaff[s] = -999;
	
	for (i = 0; i<doc->nChangeMP; i++)
   	switch (doc->change[i].oper) {
   		case SDAdd:
				/* Affects original staves starting with the specified NEW staff
					number, so we must first convert the start staff number! */
				newStart = doc->change[i].staff;
				for (origStart = NOONE, s = 1; s<=doc->nstaves; s++)
					if (newStaff[s]==newStart) { origStart = s; break; }
				if (origStart==NOONE) break;
				for (s = origStart+1; s<=doc->nstaves; s++)
					newStaff[s] += doc->change[i].p1*doc->change[i].p2;
  				break;
	 		case SDDelete:
				for (s = doc->change[i].staff; s<=doc->change[i].p1; s++)
					newStaff[s] = -1;
				nGone = doc->change[i].p1-doc->change[i].staff+1;
				for (s = doc->change[i].p1+1; s<=doc->nstaves; s++)
					newStaff[s] -= nGone;
   			break;
   	}

	for (s = 1; s<=doc->nstaves; s++)
		if (newStaff[s]==minStf) { *pOrigMinStf = s; break; }
	for (s = 1; s<=doc->nstaves; s++)
		if (newStaff[s]==maxStf) { *pOrigMaxStf = s; break; }
}


/* Remove a part from the Master Page data structure and append a Delete Part
change record to the array of changes. */

void MPDeletePart(Document *doc)
{
	LINK staffL,aStaffL,partL;
	short nparts,minStf,maxStf,origMinStf,origMaxStf;

	nparts = LinkNENTRIES(doc->masterHeadL);
	if (nparts<=2) {													/* There's always a dummy part */
		GetIndCString(strBuf, MPERRS_STRS, 5);					/* "can't delete the only part" */
		CParamText(strBuf,	"", "", "");
		StopInform(GENERIC_ALRT);
		return;
	}

	staffL = SSearch(doc->masterHeadL,STAFFtype,GO_RIGHT);
	GetSelStaves(staffL,&minStf,&maxStf);

	/* Prevent deleting more than one part at a time. */

	if (!OnlyOnePart(doc,minStf,maxStf)) {
		StopInform(DELPART_ALRT);
		return;
	}

	/* Prevent deleting parts that have any content. */

	Map2OrigStaves(doc, minStf, maxStf, &origMinStf, &origMaxStf);
	if (!CanDeletePart(doc,origMinStf,origMaxStf))
		return;

	doc->partChangedMP = TRUE;
	doc->masterChanged = TRUE;

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSEL(aStaffL)) {
			partL = FirstSubLINK(doc->masterHeadL);
			for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL)) {
				if (PartFirstSTAFF(partL) <= StaffSTAFF(aStaffL) &&
						PartLastSTAFF(partL) >= StaffSTAFF(aStaffL)) break;
			}
			break;
		}

	DelChangePart(doc,PartFirstSTAFF(partL),PartLastSTAFF(partL));
	MPRemovePart(doc,partL);
}


/* Return the number of parts remaining to <aConnectL> in the group after partL
has been deleted. */

short CountConnParts(Document */*doc*/, LINK headL, LINK connectL, LINK aConnectL, LINK partL)
{
	short firstStf,lastStf,nParts=0,staffAbove,staffBelow;
	LINK aConnL,aPartL;
	PACONNECT aConnect;

	firstStf = PartFirstSTAFF(partL);
	lastStf = PartLastSTAFF(partL);

	/* Get the staff range of the GroupLevel Connect which spans the part to be deleted. */

	aConnL = FirstSubLINK(connectL);
	for ( ; aConnL; aConnL=NextCONNECTL(aConnL)) {
		aConnect = GetPACONNECT(aConnL);
		if (aConnL==aConnectL &&
				(aConnect->staffAbove<=firstStf && aConnect->staffBelow>=lastStf)) {
			staffAbove = aConnect->staffAbove;
			staffBelow = aConnect->staffBelow;

			/* Return the number of parts in the range, less the part being deleted. */

			aPartL = NextPARTINFOL(FirstSubLINK(headL));
			for ( ; aPartL; aPartL=NextPARTINFOL(aPartL)) {
				if (aPartL != partL &&
						(PartFirstSTAFF(aPartL)>=staffAbove && PartLastSTAFF(aPartL)<=staffBelow))
					nParts++;
			}
		
			return nParts;
		}
	}
	
	/* The part being deleted was not in any group; flag by returning negative number. */

	return -2;
}


/* Update fields for Connect object connectL following deletion of a part.
stfDiff is the number of staves deleted; lastStf is the lastStaff field of
the part. staffns of staves have already been updated. The first loop
updates staffns of connects; lastStf, however, is the PartLASTSTAFF of the
part deleted before any staffns are updated. */

void UpdateConnFields(LINK connectL,
						short stfDiff, short lastStf)
{
	LINK aConnectL;
	PACONNECT aConnect;

	/* Update fields for the connects spanning or after the part
		deleted. */
		
	/* If the staffAbove field is >lastStf, all parts connected
		by PartLevel or GroupLevel connects are after the one deleted.
		Handle the case for PartLevel connects here. */

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
		aConnect = GetPACONNECT(aConnectL);
		if (aConnect->connLevel==PartLevel && aConnect->staffAbove>lastStf) {
			aConnect->staffAbove -= stfDiff;
			aConnect->staffBelow -= stfDiff;
		}
	}

 	/* If part deleted is removed from inside an already existing group,
		update the Connect field of that group here.	If the staffBelow field
		is >= lastStf, & the staffAbove field is <lastStf, a GroupLevel
		Connect spans the part deleted. Handle this case here. */

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
		aConnect = GetPACONNECT(aConnectL);
		if (aConnect->connLevel==GroupLevel) {
			if (aConnect->staffAbove<=lastStf && aConnect->staffBelow>=lastStf)
				aConnect->staffBelow -= stfDiff;				
		}
	}

	/* Handle the case where the staffAbove field is >lastStf for GroupLevel
		connects here. */

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
		aConnect = GetPACONNECT(aConnectL);
		if (aConnect->connLevel==GroupLevel && aConnect->staffAbove>lastStf) {
			aConnect->staffAbove -= stfDiff;
			aConnect->staffBelow -= stfDiff;
		}
	}

}


static void MPRemovePart(Document *doc, LINK partL)
{
	LINK pL,aStaffL,aConnectL,firstStfL,lastStfL,nextStfL,
		finalStfL,staffL,thePartL, connectL;
	short i,firstStf, lastStf, stfDiff, startStf,connParts;
	DDIST newPos; PACONNECT aConnect;
	DDIST staffTops[MAXSTAVES+1];

	firstStf = PartFirstSTAFF(partL);
	lastStf = PartLastSTAFF(partL);
	stfDiff = lastStf-firstStf + 1;

	for (pL = doc->masterHeadL; pL!=doc->masterTailL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case HEADERtype:
			case PAGEtype:
			case SYSTEMtype:
				break;

			case STAFFtype:
				staffL = pL;
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
					StaffSEL(aStaffL) = 
						(StaffSTAFF(aStaffL)>=firstStf && StaffSTAFF(aStaffL)<=lastStf);

					if (StaffSTAFF(aStaffL) == firstStf)
						firstStfL = aStaffL;
					if (StaffSTAFF(aStaffL) == lastStf)
						lastStfL = aStaffL;

					if (StaffSTAFF(aStaffL) == lastStf+1)
						nextStfL = aStaffL;
					if (StaffSTAFF(aStaffL) == doc->nstavesMP)
						finalStfL = aStaffL;
				}
				break;
				
			case CONNECTtype:

				/* Select PartLevel Connects to be deleted if they fall within the
					range.
					Select GroupLevel Connects to be deleted if the remaining
					number of parts for the Connect is < 2, the minimum number
					of parts which can be grouped. connParts < 0 indicates that
					the group doesn't span the part being deleted. */

				connectL = pL;
				aConnectL = FirstSubLINK(pL);
				for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
					aConnect = GetPACONNECT(aConnectL);
					if (aConnect->connLevel==PartLevel) {
						aConnect->selected =
							(aConnect->staffAbove>=firstStf && aConnect->staffAbove<=lastStf);
					}
					else if (aConnect->connLevel==GroupLevel) {
						connParts = CountConnParts(doc,doc->masterHeadL,pL,aConnectL,partL);
						aConnect->selected = (connParts > 0 && connParts < 2);
					}
				}
				break;
				
			default:
				MayErrMsg("MPRemovePart: can't handle type=%ld at %ld",
							(long)ObjLType(pL), pL);
				;
		}
	}

	if (lastStfL == finalStfL) {
		newPos = StaffTOP(firstStfL) -
						(StaffTOP(lastStfL)+5*StaffHEIGHT(lastStfL)/2);
	}
	else
		newPos = StaffTOP(firstStfL) - StaffTOP(nextStfL);

	MPDeleteSelRange(doc,partL);

	/* Move down staffns and staffTopMPs */

	thePartL = NextPARTINFOL(FirstSubLINK(doc->masterHeadL));
	for ( ; thePartL; thePartL=NextPARTINFOL(thePartL)) {
		startStf = PartFirstSTAFF(thePartL);
		
		if (startStf > lastStf) {
			PartFirstSTAFF(thePartL) -= stfDiff;
			PartLastSTAFF(thePartL) -= stfDiff;
		}
	}
	
	for (i=0; i<=MAXSTAVES; i++)
		staffTops[i] = doc->staffTopMP[i];

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		if (StaffSTAFF(aStaffL) > lastStf) {
			doc->staffTopMP[StaffSTAFF(aStaffL) - stfDiff] = 
				staffTops[StaffSTAFF(aStaffL)];
			StaffSTAFF(aStaffL) -= stfDiff;
		}
	}
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		if (StaffSTAFF(aStaffL) > lastStf - stfDiff)
			doc->staffTopMP[StaffSTAFF(aStaffL)] += newPos;
	}

	UpdateConnFields(connectL,stfDiff,lastStf);

	MPUpdateStaffTops(doc, doc->masterHeadL, doc->masterTailL);
	UpdateMPSysRectHeight(doc, newPos);

	InvalWindow(doc);
}


static void MPDeleteSelRange(Document *doc, LINK partL)
{
	LINK pL, pSubL, tempL;

	for (pL = doc->masterHeadL; pL!=doc->masterTailL; pL = RightLINK(pL)) {

		switch (ObjLType(pL)) {

			case HEADERtype:
				/* Decrement nEntries by one: delete one part. */

				LinkNENTRIES(pL)--;

				/* Traverse the subList in order to delete that part. */

				pSubL = FirstSubLINK(pL);
				while (pSubL) {
					if (pSubL==partL) {
						tempL = NextPARTINFOL(pSubL);
						RemoveLink(pL, PARTINFOheap, FirstSubLINK(pL), pSubL);
						HeapFree(PARTINFOheap,pSubL);
						pSubL = tempL;
					}
					else pSubL = NextPARTINFOL(pSubL);
				}
				break;
	
			case STAFFtype:
				/* Get remaining nEntries of staff. */

				pSubL = FirstSubLINK(pL);
				for ( ; pSubL; pSubL=NextSTAFFL(pSubL))
					if (StaffSEL(pSubL)) {
						LinkNENTRIES(pL)--;
						doc->nstavesMP--;
					}

				/* Traverse the subList and delete selected subObjects. */

				pSubL = FirstSubLINK(pL);
				while (pSubL) {
					if (StaffSEL(pSubL)) {
						tempL = NextSTAFFL(pSubL);
						RemoveLink(pL, STAFFheap, FirstSubLINK(pL), pSubL);
						HeapFree(STAFFheap,pSubL);
						pSubL = tempL;
					}
					else pSubL = NextSTAFFL(pSubL);
				}
				break;
	
			case CONNECTtype:
				/* Get remaining nEntries of Connect. */

				pSubL = FirstSubLINK(pL);
				for ( ; pSubL; pSubL=NextCONNECTL(pSubL))
					if (ConnectSEL(pSubL))
						LinkNENTRIES(pL)--;

				/* Traverse the subList and delete selected subObjects. */

				pSubL = FirstSubLINK(pL);
				while (pSubL) {
					if (ConnectSEL(pSubL)) {
						tempL = NextCONNECTL(pSubL);
						RemoveLink(pL, CONNECTheap, FirstSubLINK(pL), pSubL);
						HeapFree(CONNECTheap,pSubL);
						pSubL = tempL;
					}
					else pSubL = NextCONNECTL(pSubL);					
				}
				break;
				
			default:
				;
		}
	}		
}


/* --------------------------------------------------------------------------------- */
/* Adding parts */

void MPAddPart(Document *doc)
{
	LINK staffL,aStaffL,partL=NILINK; short nadd,nper;

	/* Get the selected staff subobject before which to add the new part.
		If no subobj is selected, partL will be passed to InsertPartMP as
		NILINK, and the part will be added after all staves. */

	staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSEL(aStaffL)) {
			partL = FirstSubLINK(doc->masterHeadL);
			partL = NextPARTINFOL(partL);
			for ( ; partL; partL = NextPARTINFOL(partL)) {
				if (PartFirstSTAFF(partL) <= StaffSTAFF(aStaffL) &&
						PartLastSTAFF(partL) >= StaffSTAFF(aStaffL)) break;
			}
			break;
		}

	if (!MPAddPartDialog(&nadd, &nper)) return;
	
	if (!MPAddChkVars(doc, nadd, nper)) return;

	InsertPartMP(doc, partL, nadd, nper, SHOW_ALL_LINES);
}


/* Fill context fields for staff subobject with default values. */

static void MPDefltStfCtxt(LINK aStaffL)
{
	PASTAFF aStaff;
	
	aStaff = GetPASTAFF(aStaffL);
	aStaff->clefType = DFLT_CLEF;
	aStaff->nKSItems = DFLT_NKSITEMS;
	aStaff->timeSigType = DFLT_TSTYPE;
	aStaff->numerator = config.defaultTSNum;
	aStaff->denominator = config.defaultTSDenom;
	aStaff->dynamicType = DFLT_DYNAMIC;
}


/* Check staff height for the bottom staff (really system height) against vertical
space available: return TRUE if there's enough space, FALSE if not. */

static Boolean ChkStfHt(Document *doc, short nadd, short nper)
{
	LINK staffL,aStaffL; DDIST stfBottom,stfAscent; long newHeight;
	
	staffL = SSearch(doc->masterHeadL,STAFFtype,GO_RIGHT);
	
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL)==doc->nstavesMP) break;
	
	stfAscent = 5*StaffHEIGHT(aStaffL)/2;
	stfBottom = StaffTOP(aStaffL) + stfAscent;

	newHeight = d2pt((long)stfBottom+(long)nadd*nper*stfAscent);
	return (newHeight < doc->marginRect.bottom-doc->marginRect.top);
}


/* Check the document to see if it is fit to have parts added. */

static Boolean MPAddChkVars(Document *doc,
						short nadd,			/* No. of parts to add */
						short nper)			/* No. of staves per part */
{
	char msg[40];

	if (nper<1 || nper>MAXSTPART) {
		sprintf(msg, "%d", MAXSTPART);
		CParamText(msg, "", "", "");
		StopInform(ILLNSP_ALRT);
		return FALSE;
	}
	if (nadd<=0 || (nadd*nper)+doc->nstavesMP>MAXSTAVES) {
		sprintf(msg, "%d", MAXSTAVES);
		CParamText(msg, "", "", "");
		StopInform(ILLNS_ALRT);
		return FALSE;
	}
	if (!ChkStfHt(doc,nadd,nper)) {
		sprintf(msg, "%d", nadd*nper);
		CParamText(msg, "", "", "");
		StopInform(FITSTAFF_ALRT);
		return FALSE;
	}
	return TRUE;
}


static enum {
	nPartsITM=4,
	nStavesITM=7
} E_AddPartItems;

/* Dialog to get number of parts to add, and the number of staves per part.
Returns TRUE for OK, FALSE for Cancel. */

static Boolean MPAddPartDialog(short *nParts, short *nStaves)
{	
	DialogPtr dlog; GrafPtr oldPort;
	short dVal=0, ditem;
	ModalFilterUPP	filterUPP;

	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(ADDPART_DLOG);
		return FALSE;
	}
	
	GetPort(&oldPort);
	dlog = GetNewDialog(ADDPART_DLOG, NULL, BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(ADDPART_DLOG);
		return FALSE;
	}
 	SetPort(GetDialogWindowPort(dlog)); 			
	
	SelectDialogItemText(dlog, nPartsITM, 0, ENDTEXT);

	ShowWindow(GetDialogWindow(dlog));
	OutlineOKButton(dlog,TRUE);
	
	do {
		ModalDialog(filterUPP, &ditem);
		switch (ditem) {
			case OK:
			case Cancel:
				dVal = ditem;
				break;
		}
	} while (!dVal);
	
	GetDlgWord(dlog, nPartsITM, nParts);
	GetDlgWord(dlog, nStavesITM, nStaves);

	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);

	return (dVal==OK);
}


/* Add nadd parts with nper staves in each part before partL to doc's Master Page
data structure. If partL is NILINK, add after all the parts. */

static void InsertPartMP(Document *doc,
							LINK partL,
							short nadd, short nper,
							short showLines)
{
	LINK aPartL,partList,staffL,aStaffL,stfList,
			connList,connectL,aConnectL,listHead;
	PACONNECT aConnect;
	short j,lastStf,nextStf,startStf,endStf,totalStaves,stavesAbove,
			startConnStf,firstConnStf,noParts=FALSE,prevnstavesMP, groupConnType;
	Boolean addToGroup=FALSE;
	DDIST newPos,startStfTop,stfHeight,stfHeight1,stfLeft,stfRight,dLineSp;
	
	doc->partChangedMP = TRUE;
	doc->masterChanged = TRUE;

	totalStaves = nadd*nper;
	prevnstavesMP = doc->nstavesMP;
	doc->nstavesMP += totalStaves;

	/* The firstSubLINK of masterHeadL is the initial unused part; if partL is
		passed into this function as this part, it is an error; traverse to the
		next LINK in the list and insert before this one. */

	if (partL == FirstSubLINK(doc->masterHeadL)) 
		partL = NextPARTINFOL(partL);

	/* Allocate and initialize a list of <nadd> parts and insert into masterHeadL's
		list of parts before partL. */

	partList = HeapAlloc(PARTINFOheap,nadd);
	if (!partList) { NoMoreMemory(); return; }	/* ??not good--leaves doc in inconsistent state! */

	aPartL = partList;
	for ( ; aPartL; aPartL=NextPARTINFOL(aPartL))
		InitPart(aPartL, 0, 0);
	
	listHead = InsertLink(PARTINFOheap,FirstSubLINK(doc->masterHeadL),partL,partList);
	FirstSubLINK(doc->masterHeadL) = listHead;
	LinkNENTRIES(doc->masterHeadL) += nadd;

	/* Traverse the list of parts, and get the lastStaff of the part
		immediately before the inserted list of parts. If the list was
		inserted before all except the first unused part, lastStf equals
		zero. */

	aPartL = FirstSubLINK(doc->masterHeadL);
	for ( ; aPartL; aPartL=NextPARTINFOL(aPartL)) {
		lastStf = PartLastSTAFF(aPartL);
		if (NextPARTINFOL(aPartL) == partList)
			break;
	}
	if (aPartL == FirstSubLINK(doc->masterHeadL))
		lastStf = 0;
	
	startStf = nextStf = startConnStf = lastStf + 1;

	/* Traverse the inserted list of parts, and set the firstStaff
		and lastStaff fields of all its parts. Then add <totalStaves>
		to the first and last staffns of all parts following the
		inserted list. */

	aPartL=NextPARTINFOL(aPartL);
	for (j=0; j<nadd && aPartL; j++, aPartL=NextPARTINFOL(aPartL)) {
		PartFirstSTAFF(aPartL) = nextStf;
		PartLastSTAFF(aPartL) = nextStf + (nper-1);
		nextStf = PartLastSTAFF(aPartL)+1;
	}
	for ( ; aPartL; aPartL=NextPARTINFOL(aPartL)) {
		PartFirstSTAFF(aPartL) += totalStaves;
		PartLastSTAFF(aPartL) += totalStaves;
	}

	/* Update staffns of staves following the part to be inserted. */

	staffL = SSearch(doc->masterHeadL,STAFFtype,GO_RIGHT);
	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSTAFF(aStaffL) >= startStf)
			StaffSTAFF(aStaffL) += totalStaves;
		
	/* Get the staff subObj at the insertion point, or the last staff
		subObj if partL is NILINK. If there were originally no parts,
		use default values for the parameters obtained from the staff
		subObj in the other case.
		###: Both firstStaff field of the parts and the staffn field 
		of the staves have already been incremented, so the test is
		valid. */

	if (LinkNENTRIES(staffL) > 0) {
		aStaffL = FirstSubLINK(staffL);
		if (partL) {
			for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
				if (StaffSTAFF(aStaffL) >= PartFirstSTAFF(partL)) break;		/* ### */
		}
		else
			for ( ; NextSTAFFL(aStaffL); aStaffL = NextSTAFFL(aStaffL)) ;
	
		stfHeight1 = StaffHEIGHT(aStaffL);
		stfHeight = (5*stfHeight1/2);
		stfLeft = StaffLEFT(aStaffL);
		stfRight = StaffRIGHT(aStaffL);
	}
	else {
		DDIST initStfTop1;

		initStfTop1 = (short)(doc->ledgerYSp*drSize[doc->srastralMP]/STFHALFLNS);
		
		newPos = 0;
		noParts = TRUE;					/* Means there were originally no parts */
		stfHeight1 = drSize[doc->srastralMP];
		stfHeight = (5*stfHeight1/2);
		stfLeft = 0;
		stfRight = pt2d(doc->marginRect.right-doc->marginRect.left-doc->otherIndentMP);
		doc->staffTopMP[1] = initStfTop1;
	}

	/* Allocate a list of staves and insert before <aStaffL>, the staff subObj
		at the insertion point, or as the firstSubLINK, if there were previously
		no staves. Increment the staff obj nEntries, and initialize the added
		list of staves. */

	stfList = HeapAlloc(STAFFheap,totalStaves);
	if (!stfList) { NoMoreMemory(); return; }		/* ??not good--leaves doc in inconsistent state! */

	if (FirstSubLINK(staffL))
		FirstSubLINK(staffL) = InsertLink(STAFFheap, FirstSubLINK(staffL), (partL ? aStaffL:NILINK), stfList);
	else
		FirstSubLINK(staffL) = stfList;
	LinkNENTRIES(staffL) += totalStaves;

	aStaffL = stfList;
	for (j=0; j<totalStaves && aStaffL; j++,aStaffL=NextSTAFFL(aStaffL)) {
		InitStaff(aStaffL, startStf++, 0, stfLeft, stfRight, stfHeight1, STFLINES, showLines);
		MPDefltStfCtxt(aStaffL);
	}

	/* Move staffTopMPs down graphically (down Graphically = up
		in the array) */
	
	if (NextPARTINFOL(FirstSubLINK(doc->masterHeadL)) == partList)
			startStf = 1;													/* New 1st part */
	else	startStf = lastStf+1;										/* Newly added after 1st part */
	newPos = totalStaves*stfHeight;

	if (partL) {															/* Inserted before some part */
		endStf = startStf+totalStaves;

		/* doc->staffTopMP[] uses 1-based indexing, preserving correspondance
			with staffns of staves. The <-1> in (startStf-1) accounts for
			this. */
		stavesAbove = prevnstavesMP-(startStf-1);

		startStfTop = doc->staffTopMP[startStf];

		/* Move staffTop values up in the array for old staves which were previously
			above newly added ones. Then translate these staffTop values by the total
			amount of space added. */

		BlockMove(&doc->staffTopMP[startStf],&doc->staffTopMP[endStf],
			(Size)sizeof(DDIST)*stavesAbove);
		for (j=0; j<stavesAbove; j++)
			doc->staffTopMP[endStf+j] += newPos;

		/* Fill in staffTop array for newly added staves. */
		for (j=0; j<totalStaves; j++)
			doc->staffTopMP[startStf+j] = startStfTop+j*stfHeight;
	}
	else 
		if (!noParts) {																	/* Added after all the parts */
			startStfTop = doc->staffTopMP[startStf-1];
			for (j=0; j<totalStaves; j++)
				doc->staffTopMP[startStf+j] = startStfTop + (j+1)*stfHeight;		
		}

	MPUpdateStaffTops(doc, doc->masterHeadL, doc->masterTailL);
	UpdateMPSysRectHeight(doc, newPos);

	/* Regardless of whether a new Connect subObj is to be added,
		update the staffAbove and staffBelow fields of all connects
		below the part to be added. */

	connectL = SSearch(doc->masterHeadL,CONNECTtype,GO_RIGHT);

	/* Update staffns of connects following the part to be inserted. */

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
		aConnect = GetPACONNECT(aConnectL);
		if (aConnect->staffAbove >= startConnStf) {
			if (aConnect->connLevel!=SystemLevel) {
				aConnect->staffAbove += totalStaves;
				aConnect->staffBelow += totalStaves;
			}
		}
	}

	/* If part added is added inside an already existing group,
		update the Connect field of that group here.
		#1. If parts are added to an already existing group, all parts will
			 in a single consecutive segment added to one group; therefore we
			 only need to check once for all parts added, not once for each
			 part added. */

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
		aConnect = GetPACONNECT(aConnectL);
		if (aConnect->connLevel==GroupLevel)
			if (aConnect->staffAbove<startConnStf && aConnect->staffBelow>=startConnStf) {
				aConnect->staffBelow += totalStaves;
				addToGroup = TRUE;									/* #1. */
				groupConnType = aConnect->connectType;
			}
	}

	/* Add new Connect subObjs, if needed. */

	if (nper>1) {

		/* Get the Connect immediately following the insertion point */
	
		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
			aConnect = GetPACONNECT(aConnectL);
			if (aConnect->staffAbove >= startConnStf)
				break;
		}
	
		connList = HeapAlloc(CONNECTheap,nadd);
		if (!connList) { NoMoreMemory(); return; }		/* ??not good--leaves doc in inconsistent state! */

		FirstSubLINK(connectL) = InsertLink(CONNECTheap,FirstSubLINK(connectL),aConnectL,connList);
		LinkNENTRIES(connectL) += nadd;
	
		aConnectL = connList;
		firstConnStf = startConnStf; 
		for (j=0; j<nadd && aConnectL; j++,aConnectL=NextCONNECTL(aConnectL)) {
			aConnect = GetPACONNECT(aConnectL);
			aConnect->connLevel = PartLevel;
			aConnect->connectType = CONNECTCURLY;
			dLineSp = STHEIGHT/(STFLINES-1);								/* Space between staff lines */
			aConnect->xd = -ConnectDWidth(doc->srastralMP, CONNECTCURLY);
			aConnect->staffAbove = firstConnStf;
			aConnect->staffBelow = firstConnStf+nper-1;
			if (addToGroup)
				aConnect->xd -= ConnectDWidth(doc->srastralMP, groupConnType);
			StoreConnectPart(doc->masterHeadL,aConnectL);
			firstConnStf += nper;
		}
	}

	AddChangePart(doc,lastStf,nadd,nper,showLines);
	InvalWindow(doc);
}


/* --------------------------------------------------------------------------------- */
/* Maintaining the Master Page menu */

short GetPartSelRange(Document *doc, LINK *firstPartL, LINK *lastPartL)
{
	LINK staffL,aStaffL,partL,minPartL,maxPartL;
	short minStf,maxStf;
	
	*firstPartL = *lastPartL = NILINK;
	
	if (!PartSel(doc)) {
		return FALSE;
	}	

	staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);
	if (!LinkSEL(staffL)) {
		return FALSE;
	}
	
	GetSelStaves(staffL,&minStf,&maxStf);

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSEL(aStaffL)) {
			partL = FirstSubLINK(doc->masterHeadL);
			for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL)) {
				if (PartFirstSTAFF(partL) <= minStf &&
						PartLastSTAFF(partL) >= minStf) minPartL = partL;

				if (PartFirstSTAFF(partL) <= maxStf &&
						PartLastSTAFF(partL) >= maxStf) maxPartL = partL;
			}
			*firstPartL = minPartL;
			*lastPartL = maxPartL;
			
			return minPartL!=maxPartL;
		}
		
	return FALSE;
}

/* Return TRUE if a range of >1 parts in the Master Page is selected. */

short PartRangeSel(Document *doc)
{
	LINK staffL,aStaffL,partL,minPartL,maxPartL;
	short minStf,maxStf;

	staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);
	if (!LinkSEL(staffL)) return FALSE;

	GetSelStaves(staffL,&minStf,&maxStf);

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSEL(aStaffL)) {
			partL = FirstSubLINK(doc->masterHeadL);
			for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL)) {
				if (PartFirstSTAFF(partL) <= minStf &&
						PartLastSTAFF(partL) >= minStf) minPartL = partL;

				if (PartFirstSTAFF(partL) <= maxStf &&
						PartLastSTAFF(partL) >= maxStf) maxPartL = partL;
			}
			return minPartL!=maxPartL;
		}
		
	return FALSE;
}


/* Return TRUE if any group exists in the selection range, which in this
case is the range of selected staff subObjects. Otherwise, return FALSE. */

short GroupSel(Document *doc)
{
	LINK staffL,connectL,aConnectL;
	short minStf,maxStf;
	PACONNECT aConnect;
	
	staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);
	if (!LinkSEL(staffL)) return FALSE;

	GetSelStaves(staffL,&minStf,&maxStf);
	connectL = SSearch(doc->masterHeadL,CONNECTtype,GO_RIGHT);

	/* Return TRUE if any group in the selRange exists. */

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
		aConnect = GetPACONNECT(aConnectL);
		if (aConnect->connLevel==GroupLevel) {
			if (	(aConnect->staffAbove <= minStf && aConnect->staffBelow >= minStf) ||
					(aConnect->staffAbove >= minStf && aConnect->staffBelow <= maxStf) ||
					(aConnect->staffAbove <= maxStf && aConnect->staffBelow >= maxStf) )
						return TRUE;
		}
	}

	return FALSE;
}


/* Return TRUE if any staves (and therefore parts) are selected. Otherwise, return
FALSE. Will correctly return FALSE if score has no parts. */

short PartSel(Document *doc)
{
	LINK staffL,aStaffL;

	staffL = SSearch(doc->masterHeadL,STAFFtype,GO_RIGHT);
	if (!LinkSEL(staffL)) return FALSE;

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSEL(aStaffL)) return TRUE;
		
	return FALSE;
}


/* ------------------------------------------------ Group and Ungroup Parts, etc. -- */

static void GroupPartsDialog(Boolean *, short *);

/* Return the minimum and maximum staffn of selected staves in staffL. */

void GetSelStaves(LINK staffL, short *minStf, short *maxStf)
{
	LINK aStaffL;

	*minStf=999; *maxStf = -999;

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSEL(aStaffL)) {
			*minStf = n_min(*minStf,StaffSTAFF(aStaffL));
			*maxStf = n_max(*maxStf,StaffSTAFF(aStaffL));
		}
}


/* Symbolic Dialog Item Numbers */

static enum {
	BUT1_OK = 1,
	RAD4_SQBRACKET = 4,
	RAD5_CURLY = 5,
	CHK6_EXTEND = 6
} E_GroupPartsItems;

static void GroupPartsDialog(
				Boolean *pConnBars,		/* TRUE=extend barlines to connect staves */
				short *pConnType)			/* CONNECTBRACKET or CONNECTCURLY */
{
	short itemHit;
	static short radioGroup;
	static Boolean firstCall=TRUE;
	Boolean keepGoing=TRUE;
	DialogPtr dlog; GrafPtr oldPort;
	ModalFilterUPP	filterUPP;

	/* Build dialog window and install its item values. */
	
	filterUPP = NewModalFilterUPP(OKButFilter);
	if (filterUPP == NULL) {
		MissingDialog(GROUPPARTS_DLOG);
		return;
	}
	GetPort(&oldPort);
	dlog = GetNewDialog(GROUPPARTS_DLOG,NULL,BRING_TO_FRONT);
	if (dlog==NULL) {
		DisposeModalFilterUPP(filterUPP);
		MissingDialog(GROUPPARTS_DLOG);
		return;
	}
	SetPort(GetDialogWindowPort(dlog));

	if (firstCall) {
		radioGroup = RAD4_SQBRACKET;
		firstCall = FALSE;
	}

	PutDlgChkRadio(dlog, radioGroup, TRUE);
	PutDlgChkRadio(dlog,CHK6_EXTEND,*pConnBars);

	PlaceWindow(GetDialogWindow(dlog),(WindowPtr)NULL,0,60);
	ShowWindow(GetDialogWindow(dlog));

	/* Entertain filtered user events until dialog is dismissed */
	
	while (keepGoing) {
		ModalDialog(filterUPP,&itemHit);
		switch(itemHit) {
			case BUT1_OK:
				keepGoing = FALSE;
				break;
			case RAD4_SQBRACKET:
			case RAD5_CURLY:
				if (itemHit!=radioGroup)
					SwitchRadio(dlog, &radioGroup, itemHit);
				break;
			case CHK6_EXTEND:
				PutDlgChkRadio(dlog, CHK6_EXTEND, !GetDlgChkRadio(dlog, CHK6_EXTEND));
				break;
			}
		}
	
	*pConnBars = GetDlgChkRadio(dlog, CHK6_EXTEND);
	*pConnType = (radioGroup==RAD5_CURLY? CONNECTCURLY : CONNECTBRACKET);
	
	DisposeModalFilterUPP(filterUPP);
	DisposeDialog(dlog);
	SetPort(oldPort);
}

/* Group and ungroup parts. Add a Connect subobj to selected parts in the Master Page,
in order to group the parts. Then add a change record of type SDGroup, in order to
allow exporting the grouping to the main object list. */
 
void DoGroupParts(Document *doc)
{
	LINK staffL,aStaffL,partL,minPartL,maxPartL,connectL,
			aConnectL,connList,connL;
	short minStf,maxStf, connType; DDIST connWidth;
	PACONNECT aConnect;
	static Boolean connBars=TRUE;				/* TRUE=extend barlines to connect staves */ 

	staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);
	if (!LinkSEL(staffL)) return;

	GroupPartsDialog(&connBars, &connType);

	GetSelStaves(staffL,&minStf,&maxStf);

	aStaffL = FirstSubLINK(staffL);
	for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
		if (StaffSEL(aStaffL)) {
			partL = FirstSubLINK(doc->masterHeadL);
			for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL)) {
				if (PartFirstSTAFF(partL) <= minStf &&
						PartLastSTAFF(partL) >= minStf) minPartL = partL;

				if (PartFirstSTAFF(partL) <= maxStf &&
						PartLastSTAFF(partL) >= maxStf) maxPartL = partL;
			}
			break;
		}
		
	if (minPartL!=maxPartL) {
		connectL = SSearch(doc->masterHeadL,CONNECTtype,GO_RIGHT);
	
		/* Add new Connect subObjs. */
	
		/* Get the Connect immediately following the range to be grouped */
	
		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
			aConnect = GetPACONNECT(aConnectL);
			if (aConnect->staffAbove >= maxStf)
				break;
		}
	
		connList = HeapAlloc(CONNECTheap,1);
		if (!connList) { NoMoreMemory(); return; }		/* ??leaves doc slightly inconsistent but maybe not bad */

		aConnect = GetPACONNECT(connList);
	
		/* LINK InsertLink(heap,head,before,objlist);

			Call to InsertLink results in problems when inspected by
			the debugger. When running the application without the
			debugger the call yields no apparent problems. */

		connL = InsertLink(CONNECTheap,FirstSubLINK(connectL),aConnectL,connList);
		FirstSubLINK(connectL) = connL;
		LinkNENTRIES(connectL)++;

		aConnect = GetPACONNECT(connList);
		aConnect->connLevel = GroupLevel;
		aConnect->connectType = connType;
		connWidth = ConnectDWidth(doc->srastralMP, connType);
		aConnect->xd = -connWidth;
		aConnect->staffAbove = minStf;
		aConnect->staffBelow = maxStf;
		aConnect->firstPart = minPartL;
		aConnect->lastPart = maxPartL;
		
		/* Move all Connect subObjs that are nested within the new one to
			the left by the new one's width. */
			
		aConnectL = FirstSubLINK(connectL);
		for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
			if (aConnectL!=connList) {
				aConnect = GetPACONNECT(aConnectL);
				if (aConnect->staffAbove >= minStf && aConnect->staffBelow <= maxStf)
					aConnect->xd -= connWidth;
			}
		}

	}
	
	doc->grpChangedMP = TRUE;
	doc->masterChanged = TRUE;
	
	GroupChangeParts(doc,minStf,maxStf,connBars,connType);
	InvalWindow(doc);
}

void DoUngroupParts(Document *doc)
{
	LINK staffL,connectL,aConnectL,bConnectL,nextConnL;
	short minStf,maxStf,minGrpStf,maxGrpStf; DDIST connWidth;
	PACONNECT aConnect,bConnect;

	staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);
	if (!LinkSEL(staffL)) return;

	GetSelStaves(staffL,&minStf,&maxStf);

	connectL = SSearch(doc->masterHeadL,CONNECTtype,GO_RIGHT);

	/* Remove Connect subObjs if the range of the group intersects
		the selection range. */

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL = nextConnL) {
		nextConnL = NextCONNECTL(aConnectL);
		aConnect = GetPACONNECT(aConnectL);

		/* ??Need to verify that the LINKs to the parts cannot have been changed and
			made invalid between the time parts are grouped & the Connect is created,
			and now. */

		if (aConnect->connLevel==GroupLevel)
			if (	(aConnect->staffAbove <= minStf && aConnect->staffBelow >= minStf) ||
					(aConnect->staffAbove >= minStf && aConnect->staffBelow <= maxStf) ||
					(aConnect->staffAbove <= maxStf && aConnect->staffBelow >= maxStf) ) {

				connWidth = ConnectDWidth(doc->srastralMP, aConnect->connectType);

				/* Move all Connect subObjs that were nested within the deleted Connect to
					the right by the deleted Connect's width. */

				minGrpStf = aConnect->staffAbove;
				maxGrpStf = aConnect->staffBelow;
					
				bConnectL = FirstSubLINK(connectL);
				for ( ; bConnectL; bConnectL = NextCONNECTL(bConnectL)) {
					bConnect = GetPACONNECT(bConnectL);
					if (bConnect->staffAbove >= minGrpStf && bConnect->staffBelow <= maxGrpStf)
						bConnect->xd += connWidth;
				}

				RemoveLink(connectL,CONNECTheap,FirstSubLINK(connectL),aConnectL);
				HeapFree(CONNECTheap, aConnectL);
				LinkNENTRIES(connectL)--;
			}
	}

	UngroupChangeParts(doc,minStf,maxStf);
	
	doc->grpChangedMP = TRUE;
	doc->masterChanged = TRUE;

	InvalWindow(doc);
}


/* ------------------------------------------------- DoMake1StaffParts and allies -- */

static Boolean MPAdd1StaffParts(Document *, LINK, short, short);
static void MPFinish1StaffParts(Document *, LINK, LINK);
static Boolean IsTupletCrossStaff(LINK);
static LINK XStfObjInRange(LINK, LINK, short, short);
static LINK DefaultVoiceOnOtherStaff(Document *,LINK,LINK,short,short);
static Boolean OKMake1StaffParts(Document *, short, short);

/* Add <nadd> parts of one staff each AFTER prevPartL to doc's Master Page object
list. Do not do anything to other staves in the object list; in particular, do not
increase staff nos. of parts below. */

static Boolean MPAdd1StaffParts(Document *doc, LINK prevPartL, short nadd,
								short /*showLines*/)
{
	LINK nextPartL,aPartL,partList,listHead;
	short j,lastStf,nextStf;
	
	/* Allocate and initialize a list of <nadd> parts and insert it into masterHeadL's
		list of parts after prevPartL. */

	partList = HeapAlloc(PARTINFOheap,nadd);
	if (!partList) { NoMoreMemory(); return FALSE; }

	nextPartL = NextPARTINFOL(prevPartL);								/* OK if it's NILINK */

	aPartL = partList;
	for ( ; aPartL; aPartL = NextPARTINFOL(aPartL))
		InitPart(aPartL, 0, 0);
	
	listHead = InsertLink(PARTINFOheap,FirstSubLINK(doc->masterHeadL),nextPartL,partList);
	FirstSubLINK(doc->masterHeadL) = listHead;
	LinkNENTRIES(doc->masterHeadL) += nadd;

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

/* Finish converting an n-staff part into n 1-staff parts in Master Page: replace the
original part's multistaff-part Connect with a default group Connect. */

static void MPFinish1StaffParts(Document *doc, LINK origPartL, LINK lastPartL)
{
	LINK connectL, aConnectL; PACONNECT aConnect; DDIST squareWider; short firstStf;
	
	connectL = LSSearch(doc->masterHeadL,CONNECTtype,ANYONE,GO_RIGHT,FALSE);
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


/* ??Should discard this and call IsTupletCrossStf instead. */

Boolean IsTupletCrossStaff(LINK tupletL)
{
	short tupStaff, tupVoice; LINK aNoteTupleL, aNoteL; PANOTETUPLE aNoteTuple;
	
	tupStaff = TupletSTAFF(tupletL);
	tupVoice = TupletVOICE(tupletL);
	
	aNoteTupleL = FirstSubLINK(tupletL);
	for ( ; aNoteTupleL; aNoteTupleL = NextNOTETUPLEL(aNoteTupleL)) {
		aNoteTuple = GetPANOTETUPLE(aNoteTupleL);				/* Ptr to note info in TUPLET */
		aNoteL = NoteInVoice(aNoteTuple->tpSync, tupVoice, FALSE);
		if (!aNoteL) return TRUE;									/* Should never happen */
		if (NoteSTAFF(aNoteL)!=tupStaff) return TRUE;
	}
	
	return FALSE;
}


/* If there are any cross-staff object in the given range of LINKs and staves, return
the first one, else NILINK. NB: For Beamsets, Slurs, and Tuplets, checks only the
object's staff, so Beamsets and Slurs on staff minStf-1 won't be found. But this can't
be a problem if minStf is the top staff of a part. ??Comment assumes staff no. is top
staff of object: wrong for Tuplets! */

LINK XStfObjInRange(LINK startL, LINK endL, short minStf, short maxStf)
{
	LINK pL;
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case BEAMSETtype:
				if (BeamSTAFF(pL)>=minStf && BeamSTAFF(pL)<=maxStf)
					if (BeamCrossSTAFF(pL)) return pL;
				break;
			case SLURtype:
				if (SlurSTAFF(pL)>=minStf && SlurSTAFF(pL)<=maxStf)
					if (SlurCrossSTAFF(pL)) return pL;
				break;
			case TUPLETtype:
				/* There's no cross-staff flag for tuplets, so we have to check. */
				if (TupletSTAFF(pL)>=minStf && TupletSTAFF(pL)<=maxStf)
					if (IsTupletCrossStaff(pL)) return pL;
				break;
			default:
				;
	}
	
	return NILINK;
}

LINK DefaultVoiceOnOtherStaff(Document *doc, LINK startL, LINK endL, short minStf,
										short maxStf)
{
	LINK pL, aNoteL, partL; short stf, uVoice;
	
	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
					stf = NoteSTAFF(aNoteL);
					if (stf>=minStf && stf<=maxStf) {
						Int2UserVoice(doc, NoteVOICE(aNoteL), &uVoice, &partL);
						if (uVoice<=maxStf-minStf+1 && uVoice!=stf-minStf+1)
							return pL;
					}
				}
				break;
			case GRSYNCtype:
				break;
		}
		
	return NILINK;
}

static Boolean OKMake1StaffParts(Document *doc, short minStf, short maxStf)
{
	Boolean okay=TRUE; LINK badL; short firstBad;
	char cantSplitPartStr[256], fmtStr[256];
	
	GetIndCString(cantSplitPartStr, MPERRS_STRS, 7);			/* "can't Split this Part:" */

	if (!OnlyOnePart(doc,minStf,maxStf)) { 
		GetIndCString(strBuf, MPERRS_STRS, 6);						/* "can Split only one Part at a time" */
		CParamText(strBuf,	"", "", "");
		okay = FALSE;
	}
	else if (maxStf-minStf+1<2) {
		GetIndCString(strBuf, MPERRS_STRS, 8);						/* "has only one staff" */
		CParamText(cantSplitPartStr, strBuf, "", "");
		okay = FALSE;
	}
	else if (GroupSel(doc)) {
		GetIndCString(strBuf, MPERRS_STRS, 9);						/* "it's within a group" */
		CParamText(cantSplitPartStr, strBuf, "", "");
		okay = FALSE;
	}
	else if (badL = XStfObjInRange(doc->headL,doc->tailL,minStf,maxStf)) {
		firstBad = GetMeasNum(doc, badL);
		GetIndCString(fmtStr, MPERRS_STRS, 10);					/* "contains cross-staff..." */
		sprintf(strBuf, fmtStr, firstBad);
		CParamText(cantSplitPartStr, strBuf, "", "");
		okay = FALSE;
	}
	else if (badL = DefaultVoiceOnOtherStaff(doc,doc->headL,doc->tailL,minStf,maxStf)) {
		firstBad = GetMeasNum(doc, badL);
		GetIndCString(fmtStr, MPERRS_STRS, 11);					/* "has notes in a default voice..." */
		sprintf(strBuf, fmtStr, firstBad);
		CParamText(cantSplitPartStr, strBuf, "", "");
		okay = FALSE;
	}

	if (!okay) StopInform(GENERIC2_ALRT);	

	return okay;
}

/* Replace the selected n-staff part with n 1-staff parts. */

Boolean DoMake1StaffParts(Document *doc)
{
	LINK staffL,aStaffL,partL,thePartL,newPartL; PASTAFF aStaff;
	short minStf,maxStf,firstStf,lastStf,nStavesAdd;

	staffL = LSSearch(doc->masterHeadL,STAFFtype,ANYONE,GO_RIGHT,FALSE);
	if (!LinkSEL(staffL)) return FALSE;

	GetSelStaves(staffL,&minStf,&maxStf);
	if (!OKMake1StaffParts(doc,minStf,maxStf)) return FALSE;
	
	partL = FirstSubLINK(doc->masterHeadL);
	for (partL = NextPARTINFOL(partL); partL; partL = NextPARTINFOL(partL)) {
		firstStf = PartFirstSTAFF(partL);
		lastStf = PartLastSTAFF(partL);
		if (firstStf>=minStf && lastStf<=maxStf) {
			thePartL = partL;
			break;
		}
	}

	/* The no. of 1-staff parts to add is 1 less than the no. of staves in the part. */
	
	nStavesAdd = PartLastSTAFF(thePartL)-PartFirstSTAFF(thePartL); 
	PartLastSTAFF(thePartL) = PartFirstSTAFF(thePartL);
	
	aStaffL = StaffOnStaff(staffL, PartFirstSTAFF(thePartL));
	aStaff = GetPASTAFF(aStaffL);
	
	doc->partChangedMP = TRUE;
	doc->masterChanged = TRUE;

	/* Finish transforming the Master Page object list, and start the machinery that
		(if the user saves changes when they leave Master Page) will make the same
		changes to the main score object list. */
	
	if (newPartL = MPAdd1StaffParts(doc,thePartL,nStavesAdd,aStaff->showLines)) {	/* ??FALSE leaves doc in inconsistent state! */
		MPFinish1StaffParts(doc,thePartL, newPartL);
		
		Make1StaffChangeParts(doc,firstStf,nStavesAdd,aStaff->showLines);
		
		return TRUE;
	}

	return FALSE;
}


/* ---------------------------------------------------- MPDistributeStaves --- */
/* Given 3 or more staves in the selection, distribute them vertically with
the same distance between them. Contributed by Tim Crawford, Sept. '96. */

void MPDistributeStaves(Document *doc)
{
	LINK staffL, aStaffL;
	short minStf, maxStf, numOfStaves, i;
	long botStaffHeight, topStaffHeight, vertDisplacement, displacementFactor,
			staffTopPos;

	staffL = LSSearch(doc->masterHeadL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
	if (!LinkSEL(staffL)) return;

	GetSelStaves(staffL, &minStf, &maxStf);

	aStaffL = FirstSubLINK(staffL);
	for (numOfStaves = 0; aStaffL; aStaffL = NextSTAFFL(aStaffL)) {
		if (StaffSEL(aStaffL)) numOfStaves++;
		if (StaffSTAFF(aStaffL)==minStf) topStaffHeight = StaffTOP(aStaffL);
		if (StaffSTAFF(aStaffL)==maxStf) botStaffHeight = StaffTOP(aStaffL);
	}

	if (numOfStaves>=3) {
		displacementFactor = (botStaffHeight-topStaffHeight) / (numOfStaves-1);
		aStaffL = FirstSubLINK(staffL);
		for (i = minStf; i!=maxStf; aStaffL = NextSTAFFL(aStaffL)) {
			if (StaffSEL(aStaffL)) {
				vertDisplacement = ((i-minStf)*displacementFactor)-StaffTOP(aStaffL);
				if (vertDisplacement) {
					staffTopPos = d2pt(vertDisplacement+topStaffHeight);
					staffTopPos <<= 16; /* value needs to be in high word of number */
					UpdateDraggedStaff(doc, staffL, aStaffL, staffTopPos);
					i++;
				}
			}
		}
		doc->masterChanged = TRUE;
	}
	InvalWindow(doc);
}

