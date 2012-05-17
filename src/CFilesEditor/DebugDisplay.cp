/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1994-98 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 *
 */

/* File DebugDisplay.c - printing functions for Debug - rev. for Nightingale 3.1:
	DKSPrintf				DisplayNode				MemUsageStats
	DisplayIndexNode		DHexDump
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

//#ifndef PUBLIC_VERSION		/* If public, skip this file completely! */

#define DDB

/* -------------------------------------------------------------------- DKSPrintf -- */
/* Print the context-independent information on a key signature. */

void DKSPrintf(PKSINFO KSInfo)
{
	INT16	k;
	
	if (KSInfo->nKSItems>0)
		DebugPrintf(" lets=%d:%c",
			KSInfo->KSItem[0].letcode,
			(KSInfo->KSItem[0].sharp? '#' : 'b') );
	for (k = 1; k<KSInfo->nKSItems; k++) 			
		DebugPrintf(" %d:%c",
			KSInfo->KSItem[k].letcode,
			(KSInfo->KSItem[k].sharp? '#' : 'b') );
	DebugPrintf("\n");
}


/* ------------------------------------------------------------------ DisplayNode -- */
/* Show information about the given object (node). If <abnormal>, ignores <doc>. */

void DisplayNode(Document *doc, LINK pL,
				INT16 kount,						/* Label to print for node */
				Boolean show_links,				/* Show node addr., links, size? */
				Boolean show_subs,				/* Show subobjects? */
				Boolean abnormal					/* Somewhere besides doc's main object list? */
				)
{
	register PMEVENT	p;
	PPARTINFO	pPartInfo;
	register PANOTE aNote;
	PASTAFF		aStaff;
	PAMEASURE	aMeasure;
	PAPSMEAS		aPseudoMeas;
	PACLEF		aClef;
	PAKEYSIG		aKeySig;
	PANOTEBEAM	aNoteBeam;
	PANOTETUPLE aNoteTuple;
	PATIMESIG	aTimeSig;
	PADYNAMIC	aDynamic;
	PACONNECT	aConnect;
	PASLUR		aSlur;
	PANOTEOCTAVA	aNoteOct;
	register LINK aNoteL;
	LINK			aStaffL, aMeasureL, aPseudoMeasL, aClefL, aKeySigL, aNoteBeamL,
					aNoteTupleL, aTimeSigL, aDynamicL, aConnectL, aSlurL, 
					aNoteOctL, partL;
	char			selFlag;
	const char	*ps;

#ifdef DDB
	PushLock(OBJheap);
	p = GetPMEVENT(pL);
	selFlag = ' ';
	if (!abnormal) {
		if (pL==doc->selStartL && pL==doc->selEndL) selFlag = '&';
		else if (pL==doc->selStartL)				 	  selFlag = '{';
		else if (pL==doc->selEndL)				 		  selFlag = '}';
	}	

	DebugPrintf("%c%d", selFlag, kount);
	if (show_links)
		DebugPrintf(" L%2d", pL);

	ps = NameNodeType(pL);
	DebugPrintf(" xd=%d yd=%d %s %c%c%c%c%c oRect.l=p%d",
					p->xd, p->yd, ps,
					(p->selected? 'S' : '.'),
					(p->visible? 'V' : '.'),
					(p->soft? 'S' : '.'),
					(p->valid? 'V' : '.'),
					(p->tweaked? 'T' : '.'),
					p->objRect.left );

	switch (ObjLType(pL)) {
		case HEADERtype:
			if (!abnormal)
				DebugPrintf(" sr=%d mrRect=p%d,%d,%d,%d nst=%d nsys=%d",
					doc->srastral,
					doc->marginRect.top, doc->marginRect.left,
					doc->marginRect.bottom, doc->marginRect.right,
					doc->nstaves,
					doc->nsystems);
			break;
		case SYNCtype:
			if (!abnormal) DebugPrintf(" TS=%u LT=%ld", SyncTIME(pL),
												GetLTime(doc, pL));
			break;
		case RPTENDtype:
			DebugPrintf(" lRpt=%d rRpt=%d fObj=%d subTp=%hd",
								((PRPTEND)p)->startRpt, ((PRPTEND)p)->endRpt,
								((PRPTEND)p)->firstObj, ((PRPTEND)p)->subType);
			break;
		case PAGEtype:
			DebugPrintf(" p#=%d", ((PPAGE)p)->sheetNum);
			break;
		case SYSTEMtype:
			DebugPrintf(" sRect=d%d,%d,%d,%d s#=%d",
				((PSYSTEM)p)->systemRect.top, ((PSYSTEM)p)->systemRect.left,
				((PSYSTEM)p)->systemRect.bottom, ((PSYSTEM)p)->systemRect.right,
								((PSYSTEM)p)->systemNum);
			break;
		case MEASUREtype:
			DebugPrintf(" Box=p%d,%d,%d,%d s%%=%d TS=%ld",
				((PMEASURE)p)->measureBBox.top, ((PMEASURE)p)->measureBBox.left,
				((PMEASURE)p)->measureBBox.bottom, ((PMEASURE)p)->measureBBox.right,
				((PMEASURE)p)->spacePercent,
				((PMEASURE)p)->lTimeStamp);
			break;
		case CLEFtype:
			DebugPrintf(" %c", (((PCLEF)p)->inMeasure? 'M' : '.') );
			break;
		case KEYSIGtype:
			DebugPrintf(" %c", (((PKEYSIG)p)->inMeasure? 'M' : '.') );
			break;
		case TIMESIGtype:
			DebugPrintf(" %c", (((PTIMESIG)p)->inMeasure? 'M' : '.') );
			break;
		case BEAMSETtype:
			DebugPrintf(" st=%d v=%d", ((PBEAMSET)p)->staffn, ((PBEAMSET)p)->voice);
			break;
		case DYNAMtype:
			DebugPrintf(" type=%d", ((PDYNAMIC)p)->dynamicType);
			if (show_links) DebugPrintf(" 1stS=%d lastS=%d",
									((PDYNAMIC)p)->firstSyncL, ((PDYNAMIC)p)->lastSyncL);
			break;
		case GRAPHICtype:
			DebugPrintf(" st=%d type=%d", ((PGRAPHIC)p)->staffn,
								((PGRAPHIC)p)->graphicType);
			if (show_links) {
				DebugPrintf(" fObj=%d", ((PGRAPHIC)p)->firstObj);
				if (((PGRAPHIC)p)->graphicType==GRDraw)
					DebugPrintf(" lObj=%d", ((PGRAPHIC)p)->lastObj);
			}
			break;
		case OCTAVAtype:
			DebugPrintf(" st=%d octType=%d", ((POCTAVA)p)->staffn,
								((POCTAVA)p)->octSignType);
			break;
		case SLURtype:
			DebugPrintf(" st=%d v=%d", ((PSLUR)p)->staffn, ((PSLUR)p)->voice);
			if (((PSLUR)p)->tie) DebugPrintf(" Tie");
			if (((PSLUR)p)->crossSystem) DebugPrintf(" xSys");
			if (((PSLUR)p)->crossStaff) DebugPrintf(" xStf");
			if (((PSLUR)p)->crossStfBack) DebugPrintf(" xStfBk");
			if (show_links) DebugPrintf(" %d...%d", ((PSLUR)p)->firstSyncL,
													((PSLUR)p)->lastSyncL);
			break;
		case TUPLETtype:
			DebugPrintf(" st=%d v=%d num=%d denom=%d", ((PTUPLET)p)->staffn,
								((PTUPLET)p)->voice,
								((PTUPLET)p)->accNum, ((PTUPLET)p)->accDenom);
			break;
		case TEMPOtype:
			DebugPrintf(" st=%d", ((PTEMPO)p)->staffn);
			if (show_links) DebugPrintf(" fObj=%d", ((PTEMPO)p)->firstObjL);
			break;
		case SPACEtype:
			DebugPrintf(" spWidth=%d", ((PSPACE)p)->spWidth);
			break;
		case ENDINGtype:
			DebugPrintf(" st=%d num=%d", ((PENDING)p)->staffn, ((PENDING)p)->endNum);
			if (show_links) DebugPrintf(" %d...%d", ((PENDING)p)->firstObjL,
												 				((PENDING)p)->lastObjL);
			break;
		default:
			;
	}
	
	if (p->nEntries!=0) DebugPrintf(" n=%d\n", p->nEntries);
	else				     DebugPrintf("\n");

	if (show_subs)
		switch (ObjLType(pL))
	{
		case HEADERtype:
			for (partL = FirstSubLINK(pL); partL; partL = NextPARTINFOL(partL)) {
				pPartInfo = GetPPARTINFO(partL);
				DebugPrintf("     partL=%d next=%d firstst=%d lastst=%d velo=%d transp=%d name=%s\n",
					partL, NextPARTINFOL(partL),
					pPartInfo->firstStaff,
					pPartInfo->lastStaff,
					pPartInfo->partVelocity,
					pPartInfo->transpose,
					pPartInfo->name);
			}
			break;
		case SYNCtype:
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				aNote = GetPANOTE(aNoteL);
				DebugPrintf("     ");
/* Be careful with addresses provided by the following--they can change suddenly! */
				if (OptionKeyDown())
					DebugPrintf("@%lx:", aNote);
#if 1	/* Standard version */
				DebugPrintf(
					"st=%d v=%d xd=%d yd=%d ystm=%d yqpit=%d ldur=%d .s=%d ac=%d onV=%d %c%c%c%c%c%c%c%c%c%c%c\n",
					aNote->staffn, aNote->voice,
					aNote->xd, aNote->yd, aNote->ystem, aNote->yqpit,
					aNote->subType,
					aNote->ndots,
					aNote->accident,
					aNote->onVelocity,
					(aNote->selected? 'S' : '.') ,
					(aNote->visible? 'V' : '.') ,
					(aNote->soft? 'S' : '.') ,
					(aNote->inChord? 'I' : '.') ,
					(aNote->rest? 'R' : '.'),
					(aNote->beamed? 'B' : '.'),
					(aNote->tiedL? ')' : '.'),
					(aNote->tiedR? '(' : '.'),
					(aNote->slurredL? '>' : '.'),
					(aNote->slurredR? '<' : '.'),
					(aNote->inTuplet? 'T' : '.') );
#else	/* Emphasizes performance info */
				DebugPrintf(
					"st=%d v=%d xd=%d yd=%d pdl=%ld pdur=%d ldur=%d .s=%d ac=%d onV=%d %c%c%c%c%c%c%c%c%c%c%c\n",
					aNote->staffn, aNote->voice,
					aNote->xd, aNote->yd,
					aNote->playTimeDelta, aNote->playDur,
					aNote->subType,
					aNote->ndots,
					aNote->accident,
					aNote->onVelocity,
					(aNote->selected? 'S' : '.') ,
					(aNote->visible? 'V' : '.') ,
					(aNote->soft? 'S' : '.') ,
					(aNote->inChord? 'I' : '.') ,
					(aNote->rest? 'R' : '.'),
					(aNote->beamed? 'B' : '.'),
					(aNote->tiedL? ')' : '.'),
					(aNote->tiedR? '(' : '.'),
					(aNote->slurredL? '>' : '.'),
					(aNote->slurredR? '<' : '.'),
					(aNote->inTuplet? 'T' : '.') );
#endif
			}
			break;
		case GRSYNCtype:
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextGRNOTEL(aNoteL)) {
				aNote = GetPAGRNOTE(aNoteL);
				DebugPrintf(
					"     st=%d v=%d xd=%d yd=%d ystm=%d yqpit=%d ldur=%d .s=%d ac=%d onV=%d %c%c%c%c%c%c%c\n",
					aNote->staffn, aNote->voice,
					aNote->xd, aNote->yd, aNote->ystem, aNote->yqpit,
					aNote->subType,
					aNote->ndots,
					aNote->accident,
					aNote->onVelocity,
					(aNote->selected? 'S' : '.') ,
					(aNote->visible? 'V' : '.') ,
					(aNote->soft? 'S' : '.') ,
					(aNote->inChord? 'I' : '.') ,
					(aNote->beamed? 'B' : '.'),
					(aNote->slurredL? '>' : '.'),
					(aNote->slurredR? '<' : '.') );
			}
			break;
		case STAFFtype:
			for (aStaffL=FirstSubLINK(pL); aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
				aStaff = GetPASTAFF(aStaffL);
				DebugPrintf("     st=%d top,left,ht,rt=d%d,%d,%d,%d lines=%d fontSz=%d %c%c TS=%d,%d/%d\n",
					aStaff->staffn, aStaff->staffTop,
					aStaff->staffLeft, aStaff->staffHeight,
					aStaff->staffRight, aStaff->staffLines,
					aStaff->fontSize,
					(aStaff->selected? 'S' : '.') ,
					(aStaff->visible? 'V' : '.'),
					aStaff->timeSigType,
					aStaff->numerator,
					aStaff->denominator );
			}
			break;
		case MEASUREtype:
			for (aMeasureL=FirstSubLINK(pL); aMeasureL; 
					aMeasureL=NextMEASUREL(aMeasureL)) {
				aMeasure = GetPAMEASURE(aMeasureL);
				DebugPrintf(
					"     st=%d m#=%d barTp=%d cnst=%d clf=%d mR=d%d,%d,%d,%d %c%c%c%c%c nKS=%d TS=%d,%d/%d\n",
					aMeasure->staffn, aMeasure->measureNum,
					aMeasure->subType,
					aMeasure->connStaff, aMeasure->clefType,
					aMeasure->measureRect.top, aMeasure->measureRect.left,
					aMeasure->measureRect.bottom, aMeasure->measureRect.right,
					(aMeasure->selected? 'S' : '.'),
					(aMeasure->visible? 'V' : '.'),
					(aMeasure->soft? 'S' : '.'),
					(aMeasure->measureVisible? 'M' : '.'),
					(aMeasure->connAbove? 'C' : '.'),
					aMeasure->nKSItems,
					aMeasure->timeSigType,
					aMeasure->numerator,
					aMeasure->denominator );
			}
			break;
		case PSMEAStype:
			for (aPseudoMeasL=FirstSubLINK(pL); aPseudoMeasL; 
					aPseudoMeasL=NextPSMEASL(aPseudoMeasL)) {
				aPseudoMeas = GetPAPSMEAS(aPseudoMeasL);
				DebugPrintf(
					"     st=%d subTp=%d cnst=%d\n",
					aPseudoMeas->staffn,
					aPseudoMeas->subType,
					aPseudoMeas->connStaff );
			}
			break;
		case CLEFtype:
			for (aClefL=FirstSubLINK(pL); aClefL; aClefL=NextCLEFL(aClefL)) {
				aClef = GetPACLEF(aClefL);
				DebugPrintf("     st=%d xd=%d clef=%d %c%c%c\n",
					aClef->staffn, aClef->xd, aClef->subType,
					(aClef->selected? 'S' : '.'),
					(aClef->visible? 'V' : '.'),
					(aClef->soft? 'S' : '.') );
			}
			break;
		case KEYSIGtype:
			for (aKeySigL=FirstSubLINK(pL); aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
				aKeySig = GetPAKEYSIG(aKeySigL);
				DebugPrintf("     st=%d xd=%d %c%c%c nKSItems=%d",
					aKeySig->staffn, 
					aKeySig->xd,
					(aKeySig->selected? 'S' : '.'),
					(aKeySig->visible? 'V' : '.'),
					(aKeySig->soft? 'S' : '.'),
					aKeySig->nKSItems );
				if (aKeySig->nKSItems==0)
					DebugPrintf(" nNatItems=%d", aKeySig->subType);
				DKSPrintf((PKSINFO)(&aKeySig->KSItem[0]));
			}
			break;
		case TIMESIGtype:
			for (aTimeSigL=FirstSubLINK(pL); aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL)) {
				aTimeSig = GetPATIMESIG(aTimeSigL);
				DebugPrintf("     st=%d xd=%d type=%d,%d/%d %c%c%c\n",
					aTimeSig->staffn, 
					aTimeSig->xd, aTimeSig->subType,
					aTimeSig->numerator, aTimeSig->denominator,
					(aTimeSig->selected? 'S' : '.'),
					(aTimeSig->visible? 'V' : '.'),
					(aTimeSig->soft? 'S' : '.') );
			}
			break;
		case BEAMSETtype:
			for (aNoteBeamL=FirstSubLINK(pL); aNoteBeamL; 
					aNoteBeamL=NextNOTEBEAML(aNoteBeamL)) {
				aNoteBeam = GetPANOTEBEAM(aNoteBeamL);
				if (show_links) DebugPrintf("     bpSync=%d ", aNoteBeam->bpSync);
				else				 DebugPrintf("     ");
				DebugPrintf("startend=%d fracs=%d %c\n",
					aNoteBeam->startend, aNoteBeam->fracs,
					(aNoteBeam->fracGoLeft? 'L' : 'R') );
			}
			break;
		case TUPLETtype:
			for (aNoteTupleL=FirstSubLINK(pL); aNoteTupleL; 
					aNoteTupleL=NextNOTETUPLEL(aNoteTupleL)) {
				aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
				if (show_links) DebugPrintf("     tpSync=%d\n", aNoteTuple->tpSync);
			}
			break;
		case OCTAVAtype:
			for (aNoteOctL=FirstSubLINK(pL); aNoteOctL; 
					aNoteOctL=NextNOTEOCTAVAL(aNoteOctL)) {
				aNoteOct = GetPANOTEOCTAVA(aNoteOctL);
				if (show_links) DebugPrintf("     opSync=%d\n", aNoteOct->opSync);
			}
			break;
		case GRAPHICtype: {
				LINK aGraphicL; PAGRAPHIC aGraphic;	PGRAPHIC pGraphic;
 				pGraphic = GetPGRAPHIC(pL);
				if (pGraphic->graphicType==GRString
				||  pGraphic->graphicType==GRLyric
				||  pGraphic->graphicType==GRRehearsal
				||  pGraphic->graphicType==GRChordSym) {
					aGraphicL = FirstSubLINK(pL);
					aGraphic = GetPAGRAPHIC(aGraphicL);
					DebugPrintf("     '%p'", PCopy(aGraphic->string));
					DebugPrintf("\n");								/* Protect newline from garbage strings */
				}
			}
			break;
		case CONNECTtype:
			for (aConnectL=FirstSubLINK(pL); aConnectL; 
					aConnectL=NextCONNECTL(aConnectL)) {
				aConnect = GetPACONNECT(aConnectL);
				DebugPrintf("     xd=%d lev=%d type=%d stfA=%d stfB=%d firstPart=%d last=%d %c\n",
					aConnect->xd,
					aConnect->connLevel, aConnect->connectType,
					aConnect->staffAbove, aConnect->staffBelow,
					aConnect->firstPart, aConnect->lastPart,
					(aConnect->selected? 'S' : '.'));
			}
			break;
		case DYNAMtype:
			for (aDynamicL=FirstSubLINK(pL); aDynamicL; 
					aDynamicL=NextDYNAMICL(aDynamicL)) {
				aDynamic = GetPADYNAMIC(aDynamicL);
				DebugPrintf("     st=%d xd=%d yd=%d endxd=%d %c%c%c\n",
					aDynamic->staffn, aDynamic->xd, aDynamic->yd,
					aDynamic->endxd,
					(aDynamic->selected? 'S' : '.'),
					(aDynamic->visible? 'V' : '.'),
					(aDynamic->soft? 'S' : '.') );
				
			}
			break;
		case SLURtype:
			for (aSlurL=FirstSubLINK(pL); aSlurL; aSlurL=NextSLURL(aSlurL)) {
				aSlur = GetPASLUR(aSlurL);
				DebugPrintf("     1stInd=%d lastInd=%d ctl pts=(%P %P %P %P) %c%c%c\n",
					aSlur->firstInd, aSlur->lastInd,
					&aSlur->seg.knot, &aSlur->seg.c0,
					&aSlur->seg.c1, &aSlur->endpoint,
					(aSlur->selected? 'S' : '.'),
					(aSlur->visible? 'V' : '.'),
					(aSlur->soft? 'S' : '.') );
			}
			break;
		default:
			break;
	}
	PopLock(OBJheap);

#else
	DebugStr("\pDisplayNode called without Current Events");
#endif
}


/* ---------------------------------------------------------------- MemUsageStats -- */

void MemUsageStats(Document *doc)
{
	long heapSize=0L, objHeapMemSize=0L, objHeapFileSize=0L, heapHdrSize=0L,
			subTotal, mTotal, fTotal;
	const char *ps; LINK pL; register HEAP *theHeap;
	unsigned INT16 objCount[LASTtype], h;

	/* Compute the total number of objects of each type and the number of note
		modifiers in the object list. */

	CountInHeaps(doc, objCount, TRUE);

	DebugPrintf("HEAP USAGE:\n");
 	for (h = FIRSTtype; h<LASTtype; h++) {
 		theHeap = Heap + h;
		if (theHeap->nObjs<=0) continue;
 		
		ps = NameHeapType(h, FALSE);
 		if (!OptionKeyDown())
 			DebugPrintf("  %s Heap: %u in use (%d bytes each)\n", ps, objCount[h],
 							subObjLength[h]);
		/*
		 * In memory, blocks of all types are of a constant size, but in files,
		 * OBJECTs take only as much space as that type of object needs.
		 */
 	}
 	
 	heapHdrSize = (2+(LASTtype-1-FIRSTtype))*(sizeof(INT16)+sizeof(HEAP));
 	
	subTotal = 2*sizeof(long)+sizeof(DOCUMENTHDR)+sizeof(SCOREHEADER)
					+sizeof((INT16)LASTtype)
 					+sizeof(long)+GetHandleSize((Handle)doc->stringPool);
 	subTotal += heapHdrSize;
 	subTotal += sizeof(long);

	for (h = FIRSTtype; h<LASTtype-1; h++)
		heapSize += subObjLength[h]*objCount[h];
	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		objHeapMemSize += objLength[OBJtype];
		objHeapFileSize += objLength[ObjLType(pL)];
	}
	for (pL = doc->masterHeadL; pL; pL = RightLINK(pL)) {
		objHeapMemSize += objLength[OBJtype];
		objHeapFileSize += objLength[ObjLType(pL)];
	}
	mTotal = subTotal+heapSize+objHeapMemSize;
	fTotal = subTotal+heapSize+objHeapFileSize;

 	DebugPrintf("Size of *=%ld DOCHDR=%ld SCOREHDR=%ld *=%ld strPool=%ld\n"
					"        *=%ld { heapHdrs=%ld HeapsMem/File=%ld/%ld }",
 					sizeof(long)+sizeof(long),
 					sizeof(DOCUMENTHDR), sizeof(SCOREHEADER),
 					sizeof(INT16), GetHandleSize((Handle)doc->stringPool), sizeof(long),
 					heapHdrSize, heapSize+objHeapMemSize, heapSize+objHeapFileSize);
 	DebugPrintf(" TOTAL Mem/File=%ld/%ld\n", mTotal, fTotal);
}


/* ------------------------------------------------------------- DisplayIndexNode -- */

void DisplayIndexNode(Document *doc, register LINK pL, INT16 kount, INT16 *inLinep)
{
	PMEVENT		p;
	char			selFlag;
	const char 	*ps;

	p = GetPMEVENT(pL);
	if (pL==doc->selStartL && pL==doc->selEndL) selFlag = '&';
	else if (pL==doc->selStartL)					  selFlag = '{';
	else if (pL==doc->selEndL)						  selFlag = '}';
	else													  selFlag = ' ';
	DebugPrintf("%c%d (L%2d) ", selFlag, kount, pL);
	ps = NameNodeType(pL);
	DebugPrintf("%s", ps);
	p = GetPMEVENT(pL);
	switch (ObjLType(pL)) {
		case HEADERtype:
			DebugPrintf("\t");										/* Align since info printed is short */
			break;
		case PAGEtype:
			DebugPrintf(" #%d", ((PPAGE)p)->sheetNum);
			break;
		case SYSTEMtype:
			DebugPrintf(" #%d", ((PSYSTEM)p)->systemNum);
			break;
		default:
			;
	}
	(*inLinep)++;
	if (*inLinep>=4 || ObjLType(pL)==TAILtype) {
		DebugPrintf("\n");
		*inLinep = 0;
	}
	else
		DebugPrintf("\t");
}


/* --------------------------------------------------------------------- DHexDump -- */
/* Dump the specified area as bytes in hexadecimal into the Current Events window. */

void DHexDump(unsigned char *pBuffer,
				long limit,
				INT16 nPerGroup,		/* Number of items to print in a group */
				INT16 nPerLine			/* Number of items to print in a line */
				)
{
	long l;
	
	/*
	 *	We'd like to just DebugPrintf one item at a time, but that is VERY slow, so
	 *	we assemble and print a whole line at a time.
	 */
	strBuf[0] = 0;
	for (l = 0; l<limit; l++) {
		sprintf(&strBuf[strlen(strBuf)], "%x", pBuffer[l]); 
		if ((l+1)%(long)nPerLine==0L) {
			sprintf(&strBuf[strlen(strBuf)], "\n");
			DebugPrintf("%s", strBuf);
			strBuf[0] = 0;
		}
		else if ((l+1)%(long)nPerGroup==0L) sprintf(&strBuf[strlen(strBuf)], "    ");
		else			 					  			sprintf(&strBuf[strlen(strBuf)], " ");
	}
	
	if (l%(long)nPerLine!=0L) {
		sprintf(&strBuf[strlen(strBuf)], "\n");
		DebugPrintf("%s", strBuf);
		
	}
}

//#endif /* PUBLIC_VERSION */
