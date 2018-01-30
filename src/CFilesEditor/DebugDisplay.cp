/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* File DebugDisplay.c - printing functions for Debug:
	DKSPrintf				DisplayNode				MemUsageStats
	DisplayIndexNode		DHexDump
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define DDB

/* ------------------------------------------------------------------------- DKSPrintf -- */
/* Print the context-independent information on a key signature. */

void DKSPrintf(PKSINFO KSInfo)
{
	short	k;
	
	if (KSInfo->nKSItems>0)
		LogPrintf(LOG_NOTICE, " lets=%d:%c",
			KSInfo->KSItem[0].letcode,
			(KSInfo->KSItem[0].sharp? '#' : 'b') );
	for (k = 1; k<KSInfo->nKSItems; k++) 			
		LogPrintf(LOG_NOTICE, " %d:%c",
			KSInfo->KSItem[k].letcode,
			(KSInfo->KSItem[k].sharp? '#' : 'b') );
	LogPrintf(LOG_NOTICE, "\n");
}


/* ----------------------------------------------------------------------- DisplayNode -- */
/* Show information about the given object (node), and optionally its subobjects. If
<abnormal>, ignores <doc>. */

void DisplayNode(Document *doc, LINK pL,
				short kount,					/* Label to print for node */
				Boolean show_links,				/* Show node addr., links, size? */
				Boolean show_subs,				/* Show subobjects? */
				Boolean abnormal				/* Somewhere besides doc's main object list? */
				)
{
	register PMEVENT	p;
	PPARTINFO		pPartInfo;
	register PANOTE aNote;
	PASTAFF			aStaff;
	PAMEASURE		aMeasure;
	PAPSMEAS		aPseudoMeas;
	PACLEF			aClef;
	PAKEYSIG		aKeySig;
	PANOTEBEAM		aNoteBeam;
	PANOTETUPLE		aNoteTuple;
	PATIMESIG		aTimeSig;
	PADYNAMIC		aDynamic;
	PACONNECT		aConnect;
	PASLUR			aSlur;
	PANOTEOTTAVA	aNoteOct;
	register LINK	aNoteL;
	LINK			aStaffL, aMeasureL, aPseudoMeasL, aClefL, aKeySigL, aNoteBeamL,
					aNoteTupleL, aTimeSigL, aDynamicL, aConnectL, aSlurL, 
					aNoteOctL, partL;
	char			selFlag;
	const char		*ps;

#ifdef DDB
	PushLock(OBJheap);
	p = GetPMEVENT(pL);
	selFlag = ' ';
	if (!abnormal) {
		if (pL==doc->selStartL && pL==doc->selEndL) selFlag = '&';
		else if (pL==doc->selStartL)				selFlag = '{';
		else if (pL==doc->selEndL)				 	selFlag = '}';
	}	

	LogPrintf(LOG_NOTICE, "%c%d", selFlag, kount);
	if (show_links)
		LogPrintf(LOG_NOTICE, " L%2d", pL);

	ps = NameObjType(pL);
	LogPrintf(LOG_NOTICE, " xd=%d yd=%d %s %c%c%c%c%c%c oRect.l=p%d",
					p->xd, p->yd, ps,
					(p->selected? 'S' : '.'),
					(p->visible? 'V' : '.'),
					(p->soft? 'S' : '.'),
					(p->valid? 'V' : '.'),
					(p->tweaked? 'T' : '.'),
					(p->spareFlag? 'S' : '.'),
					p->objRect.left );

	switch (ObjLType(pL)) {
		case HEADERtype:
			if (!abnormal)
				LogPrintf(LOG_NOTICE, " sr=%d mrRect=p%d,%d,%d,%d nst=%d nsys=%d",
					doc->srastral,
					doc->marginRect.top, doc->marginRect.left,
					doc->marginRect.bottom, doc->marginRect.right,
					doc->nstaves,
					doc->nsystems);
			break;
		case SYNCtype:
			if (!abnormal) LogPrintf(LOG_NOTICE, " TS=%u LT=%ld", SyncTIME(pL),
												GetLTime(doc, pL));
			break;
		case RPTENDtype:
			LogPrintf(LOG_NOTICE, " lRpt=%d rRpt=%d fObj=%d subTp=%hd",
								((PRPTEND)p)->startRpt, ((PRPTEND)p)->endRpt,
								((PRPTEND)p)->firstObj, ((PRPTEND)p)->subType);
			break;
		case PAGEtype:
			LogPrintf(LOG_NOTICE, " p#=%d", ((PPAGE)p)->sheetNum);
			break;
		case SYSTEMtype:
			LogPrintf(LOG_NOTICE, " sRect=d%d,%d,%d,%d s#=%d",
				((PSYSTEM)p)->systemRect.top, ((PSYSTEM)p)->systemRect.left,
				((PSYSTEM)p)->systemRect.bottom, ((PSYSTEM)p)->systemRect.right,
								((PSYSTEM)p)->systemNum);
			break;
		case MEASUREtype:
			LogPrintf(LOG_NOTICE, " Box=p%d,%d,%d,%d s%%=%d TS=%ld",
				((PMEASURE)p)->measureBBox.top, ((PMEASURE)p)->measureBBox.left,
				((PMEASURE)p)->measureBBox.bottom, ((PMEASURE)p)->measureBBox.right,
				((PMEASURE)p)->spacePercent,
				((PMEASURE)p)->lTimeStamp);
			break;
		case CLEFtype:
			LogPrintf(LOG_NOTICE, " %c", (((PCLEF)p)->inMeasure? 'M' : '.') );
			break;
		case KEYSIGtype:
			LogPrintf(LOG_NOTICE, " %c", (((PKEYSIG)p)->inMeasure? 'M' : '.') );
			break;
		case TIMESIGtype:
			LogPrintf(LOG_NOTICE, " %c", (((PTIMESIG)p)->inMeasure? 'M' : '.') );
			break;
		case BEAMSETtype:
			LogPrintf(LOG_NOTICE, " st=%d v=%d", ((PBEAMSET)p)->staffn, ((PBEAMSET)p)->voice);
			break;
		case DYNAMtype:
			LogPrintf(LOG_NOTICE, " type=%d", ((PDYNAMIC)p)->dynamicType);
			if (show_links) LogPrintf(LOG_NOTICE, " 1stS=%d lastS=%d",
									((PDYNAMIC)p)->firstSyncL, ((PDYNAMIC)p)->lastSyncL);
			break;
		case GRAPHICtype:
			LogPrintf(LOG_NOTICE, " st=%d type=%d", ((PGRAPHIC)p)->staffn,
								((PGRAPHIC)p)->graphicType);
			if (show_links) {
				LogPrintf(LOG_NOTICE, " fObj=%d", ((PGRAPHIC)p)->firstObj);
				if (((PGRAPHIC)p)->graphicType==GRDraw)
					LogPrintf(LOG_NOTICE, " lObj=%d", ((PGRAPHIC)p)->lastObj);
			}
			break;
		case OTTAVAtype:
			LogPrintf(LOG_NOTICE, " st=%d octType=%d", ((POTTAVA)p)->staffn,
								((POTTAVA)p)->octSignType);
			break;
		case SLURtype:
			LogPrintf(LOG_NOTICE, " st=%d v=%d", ((PSLUR)p)->staffn, ((PSLUR)p)->voice);
			if (((PSLUR)p)->tie) LogPrintf(LOG_NOTICE, " Tie");
			if (((PSLUR)p)->crossSystem) LogPrintf(LOG_NOTICE, " xSys");
			if (((PSLUR)p)->crossStaff) LogPrintf(LOG_NOTICE, " xStf");
			if (((PSLUR)p)->crossStfBack) LogPrintf(LOG_NOTICE, " xStfBk");
			if (show_links) LogPrintf(LOG_NOTICE, " %d...%d", ((PSLUR)p)->firstSyncL,
													((PSLUR)p)->lastSyncL);
			break;
		case TUPLETtype:
			LogPrintf(LOG_NOTICE, " st=%d v=%d num=%d denom=%d", ((PTUPLET)p)->staffn,
								((PTUPLET)p)->voice,
								((PTUPLET)p)->accNum, ((PTUPLET)p)->accDenom);
			break;
		case TEMPOtype:
			LogPrintf(LOG_NOTICE, " st=%d", ((PTEMPO)p)->staffn);
			if (show_links) LogPrintf(LOG_NOTICE, " fObj=%d", ((PTEMPO)p)->firstObjL);
			break;
		case SPACERtype:
			LogPrintf(LOG_NOTICE, " spWidth=%d", ((PSPACER)p)->spWidth);
			break;
		case ENDINGtype:
			LogPrintf(LOG_NOTICE, " st=%d num=%d", ((PENDING)p)->staffn, ((PENDING)p)->endNum);
			if (show_links) LogPrintf(LOG_NOTICE, " %d...%d", ((PENDING)p)->firstObjL,
												 				((PENDING)p)->lastObjL);
			break;
		default:
			;
	}
	
	if (p->nEntries!=0) LogPrintf(LOG_NOTICE, " n=%d\n", p->nEntries);
	else				     LogPrintf(LOG_NOTICE, "\n");

	if (show_subs)
		switch (ObjLType(pL))
	{
		case HEADERtype:
			for (partL = FirstSubLINK(pL); partL; partL = NextPARTINFOL(partL)) {
				pPartInfo = GetPPARTINFO(partL);
				LogPrintf(LOG_NOTICE, "     partL=%d next=%d firstst=%d lastst=%d velo=%d transp=%d name=%s\n",
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
				LogPrintf(LOG_NOTICE, "     ");
/* Be careful with addresses provided by the following--they can change suddenly! */
				if (OptionKeyDown())
					LogPrintf(LOG_NOTICE, "@%lx:", aNote);
					LogPrintf(LOG_NOTICE, 
						"st=%d v=%d xd=%d yd=%d ystm=%d yqpit=%d ldur=%d .s=%d ac=%d onV=%d %c%c%c%c %c%c%c%c %c%c%c\n",
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
			}
			break;
		case GRSYNCtype:
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextGRNOTEL(aNoteL)) {
				aNote = GetPAGRNOTE(aNoteL);
				LogPrintf(LOG_NOTICE, 
					"     st=%d v=%d xd=%d yd=%d ystm=%d yqpit=%d ldur=%d .s=%d ac=%d onV=%d %c%c%c%c %c%c%c\n",
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
				LogPrintf(LOG_NOTICE, "     st=%d top,left,ht,rt=d%d,%d,%d,%d lines=%d fontSz=%d %c%c TS=%d,%d/%d\n",
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
				LogPrintf(LOG_NOTICE, 
					"     st=%d m#=%d barTp=%d cnst=%d clf=%d mR=d%d,%d,%d,%d %c%c%c%c%c nKS=%d TS=%d,%d/%d\n",
					aMeasure->staffn, aMeasure->measureNum,
					aMeasure->subType,
					aMeasure->connStaff, aMeasure->clefType,
					aMeasure->measSizeRect.top, aMeasure->measSizeRect.left,
					aMeasure->measSizeRect.bottom, aMeasure->measSizeRect.right,
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
				LogPrintf(LOG_NOTICE, 
					"     st=%d subTp=%d cnst=%d\n",
					aPseudoMeas->staffn,
					aPseudoMeas->subType,
					aPseudoMeas->connStaff );
			}
			break;
		case CLEFtype:
			for (aClefL=FirstSubLINK(pL); aClefL; aClefL=NextCLEFL(aClefL)) {
				aClef = GetPACLEF(aClefL);
				LogPrintf(LOG_NOTICE, "     st=%d xd=%d clef=%d %c%c%c\n",
					aClef->staffn, aClef->xd, aClef->subType,
					(aClef->selected? 'S' : '.'),
					(aClef->visible? 'V' : '.'),
					(aClef->soft? 'S' : '.') );
			}
			break;
		case KEYSIGtype:
			for (aKeySigL=FirstSubLINK(pL); aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
				aKeySig = GetPAKEYSIG(aKeySigL);
				LogPrintf(LOG_NOTICE, "     st=%d xd=%d %c%c%c nKSItems=%d",
					aKeySig->staffn, 
					aKeySig->xd,
					(aKeySig->selected? 'S' : '.'),
					(aKeySig->visible? 'V' : '.'),
					(aKeySig->soft? 'S' : '.'),
					aKeySig->nKSItems );
				if (aKeySig->nKSItems==0)
					LogPrintf(LOG_NOTICE, " nNatItems=%d", aKeySig->subType);
				DKSPrintf((PKSINFO)(&aKeySig->KSItem[0]));
			}
			break;
		case TIMESIGtype:
			for (aTimeSigL=FirstSubLINK(pL); aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL)) {
				aTimeSig = GetPATIMESIG(aTimeSigL);
				LogPrintf(LOG_NOTICE, "     st=%d xd=%d type=%d,%d/%d %c%c%c\n",
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
				if (show_links) LogPrintf(LOG_NOTICE, "     bpSync=%d ", aNoteBeam->bpSync);
				else				 LogPrintf(LOG_NOTICE, "     ");
				LogPrintf(LOG_NOTICE, "startend=%d fracs=%d %c\n",
					aNoteBeam->startend, aNoteBeam->fracs,
					(aNoteBeam->fracGoLeft? 'L' : 'R') );
			}
			break;
		case TUPLETtype:
			for (aNoteTupleL=FirstSubLINK(pL); aNoteTupleL; 
					aNoteTupleL=NextNOTETUPLEL(aNoteTupleL)) {
				aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
				if (show_links) LogPrintf(LOG_NOTICE, "     tpSync=%d\n", aNoteTuple->tpSync);
			}
			break;
		case OTTAVAtype:
			for (aNoteOctL=FirstSubLINK(pL); aNoteOctL; 
					aNoteOctL=NextNOTEOTTAVAL(aNoteOctL)) {
				aNoteOct = GetPANOTEOTTAVA(aNoteOctL);
				if (show_links) LogPrintf(LOG_NOTICE, "     opSync=%d\n", aNoteOct->opSync);
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
					LogPrintf(LOG_NOTICE, "     '%p'", PCopy(aGraphic->strOffset));
					LogPrintf(LOG_NOTICE, "\n");				/* Protect newline from garbage strings */
				}
			}
			break;
		case CONNECTtype:
			for (aConnectL=FirstSubLINK(pL); aConnectL; 
					aConnectL=NextCONNECTL(aConnectL)) {
				aConnect = GetPACONNECT(aConnectL);
				LogPrintf(LOG_NOTICE, "     xd=%d lev=%d type=%d stfA=%d stfB=%d %c\n",
					aConnect->xd,
					aConnect->connLevel, aConnect->connectType,
					aConnect->staffAbove, aConnect->staffBelow,
					(aConnect->selected? 'S' : '.'));
			}
			break;
		case DYNAMtype:
			for (aDynamicL=FirstSubLINK(pL); aDynamicL; 
					aDynamicL=NextDYNAMICL(aDynamicL)) {
				aDynamic = GetPADYNAMIC(aDynamicL);
				LogPrintf(LOG_NOTICE, "     st=%d xd=%d yd=%d endxd=%d %c%c%c\n",
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
				LogPrintf(LOG_NOTICE, "     1stInd=%d lastInd=%d ctl pts=(%P %P %P %P) %c%c%c\n",
					aSlur->firstInd, aSlur->lastInd,
					&aSlur->seg.knot, &aSlur->seg.c0,
					&aSlur->seg.c1, &aSlur->endKnot,
					(aSlur->selected? 'S' : '.'),
					(aSlur->visible? 'V' : '.'),
					(aSlur->soft? 'S' : '.') );
			}
			break;
		default:
			break;
	}
	PopLock(OBJheap);

#endif
}


/* --------------------------------------------------------------------- MemUsageStats -- */

void MemUsageStats(Document *doc)
{
	long heapSize=0L, objHeapMemSize=0L, objHeapFileSize=0L, heapHdrSize=0L,
			subTotal, mTotal, fTotal;
	const char *ps; LINK pL; register HEAP *theHeap;
	unsigned short objCount[LASTtype], h;

	/* Get the total number of objects of each type and the number of note modifiers
		in both the main and the Master Page object lists. */

	CountSubobjsByHeap(doc, objCount, True);

	LogPrintf(LOG_NOTICE, "HEAP USAGE:\n");
 	for (h = FIRSTtype; h<LASTtype; h++) {
 		theHeap = Heap + h;
		if (theHeap->nObjs<=0) continue;
 		
		ps = NameHeapType(h, False);
 		if (!OptionKeyDown())
 			LogPrintf(LOG_NOTICE, "  %s Heap: %u in use (%d bytes each)\n", ps, objCount[h],
 							subObjLength[h]);
		/*
		 * In memory, blocks of all types are of a constant size, but in files,
		 * OBJECTs take only as much space as that type of object needs.
		 */
 	}
 	
 	heapHdrSize = (2+(LASTtype-1-FIRSTtype))*(sizeof(short)+sizeof(HEAP));
 	
	subTotal = 2*sizeof(long)+sizeof(DOCUMENTHDR)+sizeof(SCOREHEADER)
					+sizeof((short)LASTtype)
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

 	LogPrintf(LOG_NOTICE, "Size of *=%ld DOCHDR=%ld SCOREHDR=%ld *=%ld strPool=%ld\n"
					"        *=%ld { heapHdrs=%ld HeapsMem/File=%ld/%ld }",
 					sizeof(long)+sizeof(long),
 					sizeof(DOCUMENTHDR), sizeof(SCOREHEADER),
 					sizeof(short), GetHandleSize((Handle)doc->stringPool), sizeof(long),
 					heapHdrSize, heapSize+objHeapMemSize, heapSize+objHeapFileSize);
 	LogPrintf(LOG_NOTICE, " TOTAL Mem/File=%ld/%ld\n", mTotal, fTotal);
}


/* ------------------------------------------------------------------ DisplayIndexNode -- */

void DisplayIndexNode(Document *doc, register LINK pL, short kount, short *inLinep)
{
	PMEVENT		p;
	char		selFlag;
	const char 	*ps;

	p = GetPMEVENT(pL);
	if (pL==doc->selStartL && pL==doc->selEndL)	selFlag = '&';
	else if (pL==doc->selStartL)				selFlag = '{';
	else if (pL==doc->selEndL)					selFlag = '}';
	else										selFlag = ' ';
	LogPrintf(LOG_NOTICE, "%c%d (L%2d) ", selFlag, kount, pL);
	ps = NameObjType(pL);
	LogPrintf(LOG_NOTICE, "%s", ps);
	p = GetPMEVENT(pL);
	switch (ObjLType(pL)) {
		case HEADERtype:
			LogPrintf(LOG_NOTICE, "\t");						/* Align since info printed is short */
			break;
		case PAGEtype:
			LogPrintf(LOG_NOTICE, " #%d", ((PPAGE)p)->sheetNum);
			break;
		case SYSTEMtype:
			LogPrintf(LOG_NOTICE, " #%d", ((PSYSTEM)p)->systemNum);
			break;
		default:
			;
	}
	(*inLinep)++;
	if (*inLinep>=4 || ObjLType(pL)==TAILtype) {
		LogPrintf(LOG_NOTICE, "\n");
		*inLinep = 0;
	}
	else
		LogPrintf(LOG_NOTICE, "\t");
}


/* -------------------------------------------------------------------------- DHexDump -- */
/* Dump the specified area as bytes in hexadecimal into the log file. */

void DHexDump(unsigned char *pBuffer,
				long limit,
				short nPerGroup,		/* Number of items to print in a group */
				short nPerLine			/* Number of items to print in a line */
				)
{
	long l;
	
	/*	We'd like to just DebugPrintf one item at a time, but that was very slow, at
		least when this function was written in the 1990's. So we assemble and print a
		whole line at a time. */
	
	strBuf[0] = 0;
	for (l = 0; l<limit; l++) {
		sprintf(&strBuf[strlen(strBuf)], "%x", pBuffer[l]); 
		if ((l+1)%(long)nPerLine==0L) {
			sprintf(&strBuf[strlen(strBuf)], "\n");
			LogPrintf(LOG_INFO, "%s", strBuf);
			strBuf[0] = 0;
		}
		else if ((l+1)%(long)nPerGroup==0L)	sprintf(&strBuf[strlen(strBuf)], "    ");
		else								sprintf(&strBuf[strlen(strBuf)], " ");
	}
	
	if (l%(long)nPerLine!=0L) {
		sprintf(&strBuf[strlen(strBuf)], "\n");
		LogPrintf(LOG_INFO, "%s", strBuf);
		
	}
}
