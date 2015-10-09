/***************************************************************************
	FILE:	Part.c
	PROJ:	Nightingale, minor rev. for v.3.1
	DESC:	Part manipulation routines:
		FixStaffNums				APFixVoiceNums			DPFixVoiceNums
		AddPart						Staff2Part				Staff2PartLINK
		Sel2Part					SelPartRange			DeletePart
/***************************************************************************/

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1988-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

static LINK staffLA[MAXSTAVES+1];				/* Array of links to staff subobjects */

static void FixStaffNums(Document *, short, short, SignedByte []);
static Boolean APFixVoiceNums(Document *, short, short);
static void DPFixVoiceNums(Document *, short, short, short);
static void SelPartRange(Document *doc,LINK startL,LINK endL,short startStf,short endStf);

/* ============== Functions for updating staff and voice nos. and the voice table == */

/* Update the staff numbers of everything in the object list, after the part of
<ABS(nDelta)> staves whose maximum staff number was <afterStf> was added or deleted.
nDelta>0 means part added, nDelta<0 means part deleted.

Note that, when a part is deleted, this is not just a a matter of offseting staff
numbers. For example, if the top staff of a group is deleted, Measure subobjs for
the new top staff of the group must have their <connStaff> updated, and updating the
Connect subobj for the group is different. Adding is simpler because you can't add
the top or bottom staff of a group. */

static void FixStaffNums(Document *doc,
									short afterStf,
									short nDelta,
									SignedByte measConnStaff[])	/* Ignored if nDelta>0 */
{
	PMEVENT		p;
	PASTAFF		aStaff;
	PAMEASURE	aMeasure;
	PAPSMEAS		aPseudoMeas;
	PACONNECT	aConnect;
	LINK			pL,aStaffL, aMeasureL, aPseudoMeasL, aConnectL, subObjL;
	HEAP			*tmpHeap;
	GenSubObj	*subObj;
	
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
	
		case STAFFtype:
			
			aStaffL = FirstSubLINK(pL);
			for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
				aStaff = GetPASTAFF(aStaffL);
				if (aStaff->staffn>=afterStf) aStaff->staffn += nDelta;
			}
			break;
	
		case CONNECTtype:
			aConnectL = FirstSubLINK(pL);
			for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
				aConnect = GetPACONNECT(aConnectL);
				switch (aConnect->connLevel) {
					case SystemLevel:
						break;
					case GroupLevel:
				 		/* If a part deleted is inside an existing group, updating the
				 		group's Connect is tricky. If the staffAbove field is <=afterStf and
				 		the staffBelow field is >= afterStf, a GroupLevel Connect spans the
				 		part deleted; in this case, we need to leave the top staff no. the
				 		same and reduce the bottom number so as to reduce the number of
						staves the Connect spans by nDelta (negative in this case). Otherwise,
						if this group is below the deleted part, offset both the top and
						bottom staff numbers by nDelta. */
						
						if (nDelta<0 && aConnect->staffAbove<=afterStf && aConnect->staffBelow>=afterStf)
								aConnect->staffBelow += nDelta;
						else {
							if (aConnect->staffAbove>=afterStf) aConnect->staffAbove += nDelta;
							if (aConnect->staffBelow>=afterStf) aConnect->staffBelow += nDelta;
						
						}
						break;
					case PartLevel:
						if (aConnect->staffAbove>=afterStf) aConnect->staffAbove += nDelta;
						if (aConnect->staffBelow>=afterStf) aConnect->staffBelow += nDelta;
						break;
				}
			}
			break;
	
		case MEASUREtype:
			/* If we've deleted a part, measConnStaff[] contains all Measure <connStaff>
			 * fields from before the delete. If the deleted part was at the top of a
			 * group, update the <connStaff> field of the staff just below the deleted
			 * part to preserve the group's structure. Also, if that staff now has a
			 * nonzero <connStaff>, meaning it's the top staff of a group, clear its
			 * <connAbove>. NB: Measure staffn's haven't been updated yet: be careful!
			 * ??NEED TO DO THE SAME FOR PSEUDOMEASURES! */
			if (nDelta<0) {
				if (measConnStaff[afterStf+nDelta+1]>afterStf) {	/* Top deleted staff */
					aMeasureL = MeasOnStaff(pL, afterStf+1);			/* Top staff below delete */
					if (aMeasureL) {
						aMeasure = GetPAMEASURE(aMeasureL);
						aMeasure->connStaff = measConnStaff[afterStf+nDelta+1];
						if (aMeasure->connStaff>0) aMeasure->connAbove = FALSE;
					}
				}
			}

			/*
			 * Updating <staffn>s is simple, and now that we've handled the difficult
			 * case, updating <connStaff>s is too.
			 */
			aMeasureL = FirstSubLINK(pL);
			for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
				aMeasure = GetPAMEASURE(aMeasureL);
				if (aMeasure->staffn>=afterStf) aMeasure->staffn += nDelta;
				if (aMeasure->connStaff>=afterStf) aMeasure->connStaff += nDelta;
				/*
				 * If, after offsetting, a staff connects only to itself, it shouldn't
				 * specify <connStaff>.
				 */
				if (aMeasure->connStaff==aMeasure->staffn) aMeasure->connStaff = 0;
			}
			break;

		case PSMEAStype:
			aPseudoMeasL = FirstSubLINK(pL);
			for ( ; aPseudoMeasL; aPseudoMeasL=NextPSMEASL(aPseudoMeasL)) {
				aPseudoMeas = GetPAPSMEAS(aPseudoMeasL);
				if (aPseudoMeas->staffn>=afterStf) aPseudoMeas->staffn += nDelta;
				if (aPseudoMeas->connStaff>=afterStf) aPseudoMeas->connStaff += nDelta;
			}
			break;

		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case SYNCtype:
		case GRSYNCtype:
		case DYNAMtype:
		case RPTENDtype:
			p = GetPMEVENT(pL);
			tmpHeap = Heap + ObjLType(pL);		/* p may not stay valid during loop */
			
			for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
				subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
				if (subObj->staffn>=afterStf) subObj->staffn += nDelta;
			}
			break;

		case BEAMSETtype:
		case GRAPHICtype:
		case OCTAVAtype:
		case SLURtype:
		case TUPLETtype:
		case TEMPOtype:
		case SPACEtype:
		case ENDINGtype:
			p = GetPMEVENT(pL);
			if (((PEXTEND)p)->staffn>=afterStf)
				((PEXTEND)p)->staffn += nDelta;
			break;

		default:
			if (TYPE_BAD(pL))
				MayErrMsg("FixStaffNums: object at %ld has illegal type %ld",
							pL, (long)ObjLType(pL));
		}
	}
}


static Boolean APFixVoiceNums(Document *doc, short afterStf, short nDelta)
{
	short tableLast, addPartn, v;

	/*
	 *	See if there's room to add the nDelta new default voice(s) to the voice table.
	 * If so, move existing voices that are below the added staves' default voices
	 * down, and, if they belong to parts below the added part, add 1 to their part
	 * numbers; then fill in entries for the new voices. Finally update the object
	 * list to match.
	 */
	for (tableLast = -1, v = 1; v<=MAXVOICES; v++)
		if (doc->voiceTab[v].partn<=0)
			{ tableLast = v-1; break; }
	if (tableLast<0 || tableLast+nDelta>=MAXVOICES) return FALSE;
			
	/*
	 * The part no. of the new part is one more than the current part no. of the
	 * voice below which its default voice(s) go. Did you follow that?
	 */
	if (afterStf<=0) addPartn = 1;
	else				  addPartn = doc->voiceTab[afterStf].partn+1;
	
	for (v = MAXVOICES; v>afterStf; v--) {
		doc->voiceTab[v] = doc->voiceTab[v-nDelta];
		if (doc->voiceTab[v].partn>=addPartn) doc->voiceTab[v].partn++;
	}
	for (v = afterStf+1; v<=afterStf+nDelta; v++) {
		doc->voiceTab[v].partn = addPartn;
		doc->voiceTab[v].voiceRole = SINGLE_DI;
		doc->voiceTab[v].relVoice = v-afterStf;
	}

	OffsetVoiceNums(doc, afterStf, nDelta);
	return TRUE;
}

static void DPFixVoiceNums(Document *doc, short afterStf, short nDelta, short delPartn)
{
	short newAfterSt, v;

	newAfterSt = afterStf-nDelta+1; 				/* First staff below deleted staves */
	
	/*
	 * Move existing voices that are below the deleted staves' default voices up in
	 * the voice table and, if they belong to parts below the deleted part, subtract
	 * 1 from their part numbers. Then update the object list to match.
	 */
	for (v = newAfterSt; v<=MAXVOICES; v++)
		if (v+nDelta<=MAXVOICES) {
			doc->voiceTab[v] = doc->voiceTab[v+nDelta];
			if (doc->voiceTab[v].partn>delPartn) doc->voiceTab[v].partn--;
		}
		else
			doc->voiceTab[v].partn = 0;

	OffsetVoiceNums(doc, afterStf, -nDelta);
}


/* ================================================================================= */
/* AddPart and auxiliary functions. */

static DDIST InitPartGetStfLen(Document *doc);
static void GrowAllObjects(Document *doc,short nstAdd);
static void GetSysLinks(LINK sysL,LINK *staffL,LINK *measL,LINK *clefL,LINK *keySigL,	
																	LINK *timeSigL,LINK *connectL);
static void InitSysPart(Document *doc,LINK sysL,short nstAdd,short afterStf,short showLines);

static DDIST InitPartGetStfLen(Document *doc)
{
	DDIST staffLen; LINK staffL,bStaffL; PASTAFF bStaff;

	staffL = LSSearch(doc->headL, STAFFtype, ANYONE, GO_RIGHT, FALSE);
	bStaffL = FirstSubLINK(staffL);
	bStaff = GetPASTAFF(bStaffL);
	staffLen = bStaff->staffRight;

	return staffLen;
}

static void GrowAllObjects(Document *doc, short nstAdd)
{
	LINK pL;

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case STAFFtype:
				GrowObject(doc, pL, nstAdd);
				break;
			case MEASUREtype:
				GrowObject(doc, pL, nstAdd);
				break;
			case PSMEAStype:
				GrowObject(doc, pL, nstAdd);
				break;
			case CLEFtype:
				if (!ClefINMEAS(pL))					/* Only add new subobjects to clefs/keySigs/timeSigs */
					GrowObject(doc, pL, nstAdd);	/*		at beginning of systems. */
				break;
			case KEYSIGtype:
				if (!KeySigINMEAS(pL))
					GrowObject(doc, pL, nstAdd);
				break;
			case TIMESIGtype:
				/* if (!TimeSigINMEAS(pL)) */		/* Grow all timeSigs, not just the 1st */
				
				if (LinkNENTRIES(pL)==doc->nstaves-nstAdd) /* Grow all timeSigs which were on all staves */
					GrowObject(doc, pL, nstAdd);
				break;
			case CONNECTtype:
				if (nstAdd>1) 							/* Parts with more than 1 staff will require */
					GrowObject(doc, pL, 1);			/*		connection by new	Connect subObj. */
				break;
			default:
				;
		}
}

static void GetSysLinks(LINK sysL, LINK *staffL, LINK *measL, LINK *clefL,
								LINK *keySigL, LINK *timeSigL, LINK *connectL)
{
	*staffL = SSearch(sysL,STAFFtype,GO_RIGHT);
	*measL = SSearch(sysL,MEASUREtype,GO_RIGHT);
	*clefL = SSearch(sysL,CLEFtype,GO_RIGHT);
	*keySigL = SSearch(sysL,KEYSIGtype,GO_RIGHT);
	*timeSigL = SSearch(sysL,TIMESIGtype,GO_RIGHT);
	*connectL = SSearch(sysL,CONNECTtype,GO_RIGHT);
}


#define GetNextTIMESIGL(link)		( (link) ? NextTIMESIGL(link) : NILINK )

static void InitSysPart(Document *doc, LINK sysL, short nstAdd, short afterStf,
								short showLines)
{
	short prevNEntries,newNEntries,j,m,connStaff,ns,thisSt,measType,
			tsType,tsNum,tsDenom, groupConnType;
	LINK staffL,measL,clefL,keySigL,timeSigL,connectL,nextMeasL,nextTSL,
			aStaffL,aMeasL,aClefL,aKeySigL,aTimeSigL,aConnectL,pL,aPSMeasL;
	CONTEXT context; DDIST staffLen,ydelta,halfpt,dLineSp;
	PASTAFF aStaff,bStaff,cStaff; PACONNECT aConnect; PAMEASURE aMeas;
	PATIMESIG aTimeSig;
	Boolean addToGroup=FALSE,connAbove; char subType;

	GetSysLinks(sysL,&staffL,&measL,&clefL,&keySigL,&timeSigL,&connectL);
	staffLen = InitPartGetStfLen(doc);

	prevNEntries = LinkNENTRIES(staffL)-nstAdd;
	newNEntries = LinkNENTRIES(staffL);

	/* Fill in array of existing staff subObj LINKs, indexed by staffn of 
		staff subObjs. */
	aStaffL = FirstSubLINK(staffL);
	for (j=0; j<prevNEntries; j++,aStaffL=NextSTAFFL(aStaffL))
		staffLA[StaffSTAFF(aStaffL)] = aStaffL;

	/* Fill in everything for the structural objects in sysL's staff. */

	aStaffL = FirstSubLINK(staffL);
	aMeasL = FirstSubLINK(measL);
	aClefL = FirstSubLINK(clefL);
	aKeySigL = FirstSubLINK(keySigL);
	if (timeSigL) {
		aTimeSigL = TimeSigOnStaff(timeSigL, afterStf>0?  1 : nstAdd+1);
		aTimeSig = GetPATIMESIG(aTimeSigL);
		tsType = aTimeSig->subType;
		tsNum = aTimeSig->numerator;
		tsDenom = aTimeSig->denominator;
	}
	aTimeSigL = FirstSubLINK(timeSigL);
	
	for (j=1,m=1; j<=newNEntries; j++,aStaffL=NextSTAFFL(aStaffL),
												aClefL=NextCLEFL(aClefL),
												aKeySigL=NextKEYSIGL(aKeySigL),
												aTimeSigL=GetNextTIMESIGL(aTimeSigL),
												aMeasL=NextMEASUREL(aMeasL))
		if (j>prevNEntries) {
			thisSt = afterStf+m;

			/* Take staff length from firstSubLINK of this staff object. If we
				use first staff of the score, its length is based on different
				indent than succeeding staves, and will usually be too short. */

			aStaff = GetPASTAFF(FirstSubLINK(staffL));
			staffLen = aStaff->staffRight;
			
			InitStaff(aStaffL, thisSt, 999, 0, staffLen, STHEIGHT, STFLINES, showLines);

			aStaff = GetPASTAFF(aStaffL);
			aStaff->clefType = DFLT_CLEF;
			aStaff->nKSItems = DFLT_NKSITEMS;
			aStaff->timeSigType = tsType;
			aStaff->numerator = tsNum;
			aStaff->denominator = tsDenom;
			aStaff->dynamicType = DFLT_DYNAMIC;
			
			/* If the staff's system is not the first of the score, get the context
				effective at the end of the previous system. Calling GetContext at
				the StaffSYS will get this context. */

			if (LinkLSYS(sysL)) {
				GetContext(doc, sysL, thisSt, &context);
				FixStaffContext(aStaffL, &context);
			}

			staffLA[thisSt] = aStaffL;
			aStaff = GetPASTAFF(aStaffL);
			if (thisSt==1)															/* This is top staff in score */
				aStaff->staffTop = initStfTop1;
			else if (thisSt==2)													/* 2nd staff--use fixed space */
				aStaff->staffTop = initStfTop1+initStfTop2;
			else	{																	/* Later staff--repeat prev. space */ 
				bStaff = GetPASTAFF(staffLA[thisSt-1]);
				cStaff = GetPASTAFF(staffLA[thisSt-2]);
				aStaff->staffTop = bStaff->staffTop+(bStaff->staffTop-cStaff->staffTop);
			}
			
			/* Clef, key sig., and time sig. must be set before Measure so context
				will be correct. */
	
			InitClef(aClefL, thisSt, p2d(0),
												(nstAdd>1 && m==nstAdd) ? BASS_CLEF:DFLT_CLEF);
			InitKeySig(aKeySigL, thisSt, p2d(0), DFLT_NKSITEMS);
			
			if (timeSigL) {
				InitTimeSig(aTimeSigL, thisSt, p2d(0), tsType, tsNum, tsDenom);
			}

			connStaff = (m==1 && nstAdd>1 ? afterStf+nstAdd : 0);
			InitMeasure(aMeasL, thisSt, p2d(0), 999,
								staffLen-LinkXD(measL), 999, FALSE,
								m>1, connStaff, 0);
			GetContext(doc, LeftLINK(measL), thisSt, &context);		/* Put default context */
			FixMeasureContext(aMeasL, &context);							/*   into measure */
			m++;
		}
	
	/*
	 *	<afterStf+nstAdd> is the staffn of the last newly added staff; if there
	 * are any staves after this, update their staffTop fields to move them down
	 * graphically below the newly added staves.
	 */
	if (doc->nstaves>afterStf+nstAdd)	{
		if (afterStf>0) {
			bStaff = GetPASTAFF(staffLA[afterStf+nstAdd]);		/* Get amount to move down */
			cStaff = GetPASTAFF(staffLA[afterStf]);
			ydelta = bStaff->staffTop - cStaff->staffTop;
		}
		else {
			bStaff = GetPASTAFF(staffLA[afterStf+nstAdd]);
			ydelta = bStaff->staffTop;
		}
		for (ns=afterStf+nstAdd+1; ns<=doc->nstaves; ns++)  {
			bStaff = GetPASTAFF(staffLA[ns]);
			bStaff->staffTop = bStaff->staffTop+ydelta;			/* Move down by this amount */
		}
	}

	/* #1. If parts are added to an already existing group, all parts will be
		in a single consecutive segment added to one group; therefore we need
		only to check once for all parts added, not once for each part added. NB:
		afterStf is indexed differently from startConnStf in InsertPartMP
		(MasterPage.c). */

	aConnectL = FirstSubLINK(connectL);
	for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
		aConnect = GetPACONNECT(aConnectL);
		if (aConnect->connLevel==GroupLevel)
			if (aConnect->staffAbove<=afterStf && aConnect->staffBelow>afterStf) {		/* #1. */
				addToGroup = TRUE;
				groupConnType = aConnect->connectType;
				break;
			}
	}

	/*
	 * Any parts added with more than 1 staff will have a Connect subObj. GrowObject
	 * adds 1 subObj to the end of the Connect's linked list of subObjs; traverse to
	 * this subObj, and init it.
	 */
	if (nstAdd>1) {
		aConnectL = FirstSubLINK(connectL);
		for (j=0; j<LinkNENTRIES(connectL); j++,aConnectL=NextCONNECTL(aConnectL))
			if (j==LinkNENTRIES(connectL)-1) {
				halfpt = pt2d(1)/2;
				dLineSp = STHEIGHT/(5-1);										/* Get space between staff lines */
				aConnect = GetPACONNECT(aConnectL);
				aConnect->selected = FALSE;
				aConnect->connLevel = PartLevel;								/* Yes. Connect the part */
				aConnect->connectType = CONNECTCURLY;
				aConnect->staffAbove = afterStf+1;
				aConnect->staffBelow = afterStf+nstAdd;
				aConnect->xd = -ConnectDWidth(doc->srastral, CONNECTCURLY);
				if (addToGroup)
					aConnect->xd -= ConnectDWidth(doc->srastral, groupConnType);
				aConnect->filler = 0;
			}
	}
	
	nextTSL = LSISearch(RightLINK(timeSigL),TIMESIGtype,ANYONE,GO_RIGHT,FALSE);

	for ( ; nextTSL; nextTSL =
				LSISearch(RightLINK(nextTSL),TIMESIGtype,ANYONE,GO_RIGHT,FALSE)) {
		
		/* Only grew timeSigs which were previously on all staves */

		if (LinkNENTRIES(nextTSL) != doc->nstaves) continue;

		aTimeSigL = TimeSigOnStaff(nextTSL, afterStf>0?  1 : nstAdd+1);
		aTimeSig = GetPATIMESIG(aTimeSigL);
		tsType = aTimeSig->subType;
		tsNum = aTimeSig->numerator;
		tsDenom = aTimeSig->denominator;

		aTimeSigL = FirstSubLINK(nextTSL);
		for (j=1,m=1; j<=newNEntries; j++,aTimeSigL=NextTIMESIGL(aTimeSigL))
			if (j>prevNEntries) {
				thisSt = afterStf+m;
				InitTimeSig(aTimeSigL, thisSt, p2d(0), tsType, tsNum, tsDenom);
				m++;
			}		
	}

	/*
	 *	Update fields for all measures after the first in the system.
	 */
	nextMeasL = LinkRMEAS(measL);
	for ( ; nextMeasL; nextMeasL = LinkRMEAS(nextMeasL)) {
		if (!SameSystem(measL,nextMeasL)) break;

		aMeasL = FirstSubLINK(nextMeasL);
		aMeas = GetPAMEASURE(aMeasL);
		measType = aMeas->subType;

		for (j=1,m=1; j<=newNEntries; j++,aMeasL=NextMEASUREL(aMeasL))
			if (j>prevNEntries) {
				thisSt = afterStf+m;
				connStaff = (m==1 && nstAdd>1 ? afterStf+nstAdd : 0);

				connAbove = m>1;
				if (addToGroup)
					{ connStaff = 0; connAbove = TRUE; }

				InitMeasure(aMeasL, thisSt, p2d(0), 999,
									staffLen-LinkXD(nextMeasL), 999, TRUE,
									connAbove, connStaff, 0);		
				aMeas = GetPAMEASURE(aMeasL);
				aMeas->subType = measType;
				GetContext(doc, LeftLINK(nextMeasL), thisSt, &context);	/* Put default context... */
				FixMeasureContext(aMeasL, &context);							/*   into measure */
				m++;
			}
	}

	/*
	 *	Update connStaff and connAbove fields for the first invisible measure.
	 */

	if (addToGroup) {
		aMeasL = FirstSubLINK(measL);
		for (j=1; j<=newNEntries; j++,aMeasL=NextMEASUREL(aMeasL))
			if (j>prevNEntries) {
				aMeas = GetPAMEASURE(aMeasL);
				aMeas->connStaff = 0;
				aMeas->connAbove = TRUE;
			}
	}

	/*
	 *	Update fields for all PSMeasures.
	 */
	 
	pL = doc->headL;
	for ( ; pL; pL = RightLINK(pL)) {
		if (PSMeasTYPE(pL)) {
			aPSMeasL = FirstSubLINK(pL);
			subType = PSMeasType(aPSMeasL);
			for (j=1,m=1; j<=newNEntries; j++,aPSMeasL=NextPSMEASL(aPSMeasL))
				if (j>prevNEntries) {
					thisSt = afterStf+m;
					connStaff = (m==1 && nstAdd>1 ? afterStf+nstAdd : 0);
					InitPSMeasure(aPSMeasL, thisSt, TRUE, m>1, connStaff, subType);
					m++;
				}
		}
	}
}

/* --------------------------------------------------------------------- AddPart -- */
/* Add a part of nstAdd staves below staff afterStf (0 = above 1st). We do not fix
up measure and system bounding Rects; the caller must do so.

NB: some of the following depends on GrowAllObjects always adding subobjs at the end
of the subobject list, never the beginning. Also, some of it depends on header
subobjects (PARTINFOs) being in order of increasing staves. */

LINK AddPart(Document *doc,
					short afterStf,
					short nstAdd,
					short showLines)		/* 0=show 0 staff lines, 1=only middle line, SHOW_ALL_LINES=show all */
{
	short partn, np; LINK partL,newPartL,sysL,measL,lastMeasL;
	SignedByte measConnStaff[MAXSTAVES+1];										/* Unused */
				
	/*
	 * Update score header and document fields.
	 *
	 * Create and initialize newPartL, and insert it in the score's part list.
	 */
	partn = Staff2Part(doc, afterStf);
	partL = FindPartInfo(doc, partn);
	newPartL = HeapAlloc(PARTINFOheap, 1);
	if (!newPartL) return NILINK;
	InitPart(newPartL, afterStf+1, afterStf+nstAdd);

	NextPARTINFOL(newPartL) = NILINK;
	
	InsAfterLink(PARTINFOheap,FirstSubLINK(doc->headL),partL,newPartL);
	LinkNENTRIES(doc->headL)++;
	doc->nstaves += nstAdd;

	/*
	 *	Add subobjects to staves, measures, clefs, keySigs, timeSigs and Connects.
	 * Also fix up staff and voice numbers of everything on staves below the newly-
	 * added part, and adjust the voice-mapping table to match.
	 */
	GrowAllObjects(doc,nstAdd);
	FixStaffNums(doc, afterStf+1, nstAdd, measConnStaff);

	if (!APFixVoiceNums(doc, afterStf, nstAdd)) {
		GetIndCString(strBuf, MISCERRS_STRS, 16);    		/* "The voice table doesn't have room..." */
		CParamText(strBuf, "", "", "");
		StopInform(GENERIC_ALRT);
		return NILINK;
	}

	/* Fix up parts-relative info for following parts. NB: assumes they have higher staff nos.! */

	partL = FirstSubLINK(doc->headL);
	for (np = 0; np<LinkNENTRIES(doc->headL); np++, partL = NextPARTINFOL(partL))
		if (np>partn+1) {
			PartFirstSTAFF(partL) += nstAdd;
			PartLastSTAFF(partL) += nstAdd;
		}

	/*
	 * Initialize the newly-added subobjects for each system in the score.
	 */
	sysL = SSearch(doc->headL,SYSTEMtype,GO_RIGHT);
	for ( ; sysL; sysL = LinkRSYS(sysL))
		InitSysPart(doc,sysL,nstAdd,afterStf,showLines);

	UpdateMeasNums(doc, NILINK);

	measL = SSearch(doc->headL,MEASUREtype,GO_RIGHT);
	lastMeasL = LSSearch(doc->tailL,MEASUREtype,ANYONE,GO_LEFT,FALSE);
	FixMeasRectXs(measL,lastMeasL);

	doc->selStartL = doc->headL;
	doc->selEndL = doc->tailL;
	DeselAll(doc);
	InvalRange(doc->headL, doc->tailL);

	return newPartL;
}


/* ------------------------------------------- Get part corresponding to something -- */
/* NB: There are several more utilities for doing this kind of thing in DSUtils.cp!
they should probably be moved here, or maybe vice-versa. */

/* Given staff number, returns number of part it belongs to. */

short Staff2Part(Document *doc, short nstaff)
{
	short			np;
	PPARTINFO	pPart;
	LINK			partL;
	
	if (nstaff<=0) return 0;
	
	partL = NextPARTINFOL(FirstSubLINK(doc->headL));
	for (np = 1; partL; np++, partL = NextPARTINFOL(partL)) {
		//fix_end(partL);
		pPart = GetPPARTINFO(partL);
		if (pPart->lastStaff>=nstaff) return np;
	}
	
	MayErrMsg("Staff2Part: staff no. %ld is not in any part.", (long)nstaff);
	return 0;
}


/* Given staff number, returns part link it belongs to. */

LINK Staff2PartLINK(Document *doc, short nstaff)
{
	short			np;
	PPARTINFO	pPart;
	LINK			partL;
	
	if (nstaff<=0) return 0;
	
	partL = NextPARTINFOL(FirstSubLINK(doc->headL));
	for (np = 1; partL; np++, partL = NextPARTINFOL(partL)) {
		pPart = GetPPARTINFO(partL);
		if (pPart->lastStaff>=nstaff) return partL;
	}
	
	MayErrMsg("Staff2Part: staff no. %ld is not in any part.", (long)nstaff);
	return NILINK;
}


/* Return the Part of the first selected object or, if nothing is selected, of the
insertion point. DAB, Oct. 2015 */

LINK Sel2Part(Document *doc)
{
	short voice, userVoice;
	LINK partL;
	
	if (doc->selStartL!=doc->selEndL) {
		voice = GetVoiceFromSel(doc);
		if (!Int2UserVoice(doc, voice, &userVoice, &partL))
			MayErrMsg("Sel2Part: Int2UserVoice(%ld) failed.", (long)voice);
	}
	else {
		partL = FindPartInfo(doc, Staff2Part(doc, doc->selStaff));
	}

	return partL;
}



/* ================================================================================= */
/* DeletePart and auxiliary functions. */

/* Traverse range from startL to endL, and select subobjects/objects on staves from
startStf to endStf inclusive. Select Connect subobjects only if their range is
entirely included. */

static void SelPartRange(Document *doc, LINK /*startL*/, LINK /*endL*/,
									short startStf, short endStf)
{
	LINK pL,aStaffL,aConnectL,subObjL;
	Boolean selObject;
	PASTAFF aStaff; PACONNECT aConnect;
	PMEVENT p; HEAP *tmpHeap; GenSubObj *subObj;
				
	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		selObject = FALSE;
		switch (ObjLType(pL)) {
			case HEADERtype:
			case PAGEtype:
			case SYSTEMtype:
				break;

			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
					aStaff = GetPASTAFF(aStaffL);
					if (aStaff->staffn>=startStf && aStaff->staffn<=endStf)
						aStaff->selected = selObject = TRUE;
					else 
						aStaff->selected = FALSE;
				}
				break;
				
			case CONNECTtype:
				aConnectL = FirstSubLINK(pL);
				for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
					aConnect = GetPACONNECT(aConnectL);
					if (aConnect->connLevel==PartLevel) {
						aConnect->selected =
							(aConnect->staffAbove>=startStf && aConnect->staffBelow<=endStf);
					}
					else if (aConnect->connLevel==GroupLevel) {
						aConnect->selected =
							(aConnect->staffAbove>=startStf && aConnect->staffBelow<=endStf);
					}
					selObject |= aConnect->selected;
				}
				break;

			case MEASUREtype:		
			case PSMEAStype:		
			case SYNCtype:
			case GRSYNCtype:
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case DYNAMtype:
			case RPTENDtype:
				tmpHeap = Heap + ObjLType(pL);		/* p may not stay valid during loop */
				
				for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
					if (subObj->staffn>=startStf && subObj->staffn<=endStf)
						subObj->selected = selObject = TRUE;
					else
						subObj->selected = FALSE;
				}
				break;

			case BEAMSETtype:
			case OCTAVAtype:
			case GRAPHICtype:
			case SLURtype:
			case TUPLETtype:
			case TEMPOtype:
			case SPACEtype:
			case ENDINGtype:
				p = GetPMEVENT(pL);
				if (((PEXTEND)p)->staffn>=startStf && ((PEXTEND)p)->staffn<=endStf)
					LinkSEL(pL) = TRUE;
				break;

			default:
				MayErrMsg("SelPartRange: can't handle type=%ld at %ld",(long)ObjLType(pL),pL);
		}
		if (selObject) LinkSEL(pL) = TRUE;
	}
}


/* Delete the part which contains staves startStf through endStf. We delete Connect
subobjects only if they apply to that part alone. As a result, this may leave
GroupLevel Connects that include only one part, which should normally be removed
by the calling routine. */

Boolean DeletePart(Document *doc,
							short startStf, short endStf)		/* Inclusive range of staves */
{
	register PASTAFF aStaff;
	short			deltaNStf, ydelta, ns, np, thisPart;
	LINK			pL,aStaffL,partL,qPartL,measL,lastMeasL,aMeasL;
	DDIST			startStfTop, endStfTop;
	Boolean		dontResp;
	PAMEASURE	aMeasure;
	SignedByte	measConnStaff[MAXSTAVES+1];
	short			staffn;

	pL = SSearch(doc->headL, STAFFtype, GO_RIGHT);
	aStaffL = FirstSubLINK(pL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
		aStaff = GetPASTAFF(aStaffL);
		if (aStaff->staffn==startStf) startStfTop = aStaff->staffTop;
		if (aStaff->staffn==endStf+1) endStfTop = aStaff->staffTop;
	}

	/*
	 * Save the information we'll need to fix Measure and Pseudomeasure <connStaff> 
	 * fields after deleting the part. Saving the <connStaff> for every staff may
	 * be overkill, but the other ways I've tried to represent the information we
	 * need have failed.
	 */
	measL = SSearch(doc->headL,MEASUREtype,GO_RIGHT);
	aMeasL = FirstSubLINK(measL);
	for ( ; aMeasL; aMeasL=NextMEASUREL(aMeasL)) {
		staffn = MeasureSTAFF(aMeasL);
		aMeasure = GetPAMEASURE(aMeasL);
		measConnStaff[staffn] = aMeasure->connStaff;
	}

	if (doc->selStartL!=doc->selEndL) DeselAllNoHilite(doc);

	/*
	 * Select all objects/subobjects in object list on staves startStf through endStf
	 * inclusive. Then update the selRange, and delete the selected objects/subobjects.
	 * This deletes Connect subobjects only if they apply to the part we're deleting
	 * alone.
	 */
	SelPartRange(doc,doc->headL,doc->tailL,startStf,endStf);

	UpdateSelection(doc);
	DeleteSelection(doc, FALSE, &dontResp);
	measL = SSearch(doc->headL,MEASUREtype,GO_RIGHT);
	lastMeasL = LSSearch(doc->tailL,MEASUREtype,ANYONE,GO_LEFT,FALSE);
	FixMeasRectXs(measL,lastMeasL);

	/*
	 * Everything for the part has now been deleted except for its description
	 *	in the score header.  Before fixing up the header, any staff-related
	 * and parts-related info for staves below the deleted part must be updated,
	 * and staff numbers of objects on them must be reduced.
	 */
	pL = SSearch(doc->headL, STAFFtype, GO_RIGHT);
	aStaffL = FirstSubLINK(pL);
	for ( ; aStaffL; aStaffL=NextSTAFFL(aStaffL))
		staffLA[StaffSTAFF(aStaffL)] = aStaffL;

	if (doc->nstaves>endStf)	{										/* Fix any staves below this. */
		ydelta = endStfTop-startStfTop;
		for (ns=endStf+1; ns<=doc->nstaves; ns++) {																	
			aStaff = GetPASTAFF(staffLA[ns]);						/* Fix up staff y-positions for */
			aStaff->staffTop = aStaff->staffTop-ydelta;			/*		following staves */
		}
	}

	deltaNStf = endStf-startStf+1;									/* No. of staves deleted */

	/* Fix up parts-relative info for following parts. NB: assumes they have higher staff nos.! */

	thisPart = Staff2Part(doc, startStf);
	partL = FirstSubLINK(doc->headL);
	for (np=0; np<LinkNENTRIES(doc->headL); np++, partL = NextPARTINFOL(partL))
		if (np>=thisPart) {
			qPartL = NextPARTINFOL(partL);
			PartFirstSTAFF(partL) = PartFirstSTAFF(qPartL)-deltaNStf;
			PartLastSTAFF(partL) = PartLastSTAFF(qPartL)-deltaNStf;
		}
	
	GrowObject(doc, doc->headL, -1);									/* Finish fixing header */
	doc->nstaves -= deltaNStf;
	
	/*
	 * Fix up staff and voice numbers of everything on staves below the deleted
	 *	part, and adjust the voice-mapping table to match.
	 */
	FixStaffNums(doc, endStf, -deltaNStf, measConnStaff);
	DPFixVoiceNums(doc, endStf, deltaNStf, thisPart);
	
	doc->selStartL = doc->headL;
	doc->selEndL = doc->tailL;
	DeselAll(doc);

	InvalRange(doc->headL, doc->tailL);
	return TRUE;
}
