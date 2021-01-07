/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2020 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* File DebugDisplay.c - printing functions for the Debug command and debugging in general:
	KeySigSprintf			DKeySigPrintf			DisplayObject
	MemUsageStats			DisplayIndexNode		NHexDump
*/

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define DDB

/* --------------------------------------------------------------------- KeySigSprintf -- */

void KeySigSprintf(PKSINFO KSInfo, char ksStr[])
{
	short k;
	
	if (KSInfo->nKSItems>0) {
		sprintf(ksStr, " lets=%d:%c", KSInfo->KSItem[0].letcode,
					(KSInfo->KSItem[0].sharp? '#' : 'b') );
		for (k = 1; k<KSInfo->nKSItems; k++) 			
			sprintf(&ksStr[strlen(ksStr)], " %d:%c", KSInfo->KSItem[k].letcode,
					(KSInfo->KSItem[k].sharp? '#' : 'b') );
	}
	else
		sprintf(ksStr, " (0 sharps/flats)");
}


/* --------------------------------------------------------------------- DKeySigPrintf -- */
/* Print the context-independent information on a key signature. */

void DKeySigPrintf(PKSINFO ksInfo)
{
	char ksStr[256];
	
	KeySigSprintf(ksInfo, ksStr);
	sprintf(&ksStr[strlen(ksStr)], "\n");
	LogPrintf(LOG_INFO, ksStr);
}


/* --------------------------------------------------------------------- DisplayObject -- */
/* Show information about the given object, and optionally its subobjects. If <nonMain>,
ignore <doc>. */

void DisplayObject(Document *doc, LINK pL,
				short kount,				/* Label to print for node */
				Boolean showLinks,			/* Show node addr., links, size? */
				Boolean showSubs,			/* Show subobjects? */
				Boolean nonMain				/* Somewhere besides doc's main object list? */
				)
{
	PMEVENT			p;
	PPARTINFO		pPartInfo;
	PANOTE			aNote;
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
	LINK			aNoteL;
	LINK			aStaffL, aMeasureL, aPseudoMeasL, aClefL, aKeySigL, aNoteBeamL,
					aNoteTupleL, aTimeSigL, aDynamicL, aConnectL, aSlurL, 
					aNoteOctL, partL;
	char			selFlag;
	const char		*ps;

#ifdef DDB
	PushLock(OBJheap);
	p = GetPMEVENT(pL);
	selFlag = ' ';
	if (!nonMain) {
		if (pL==doc->selStartL && pL==doc->selEndL) selFlag = '&';
		else if (pL==doc->selStartL)				selFlag = '{';
		else if (pL==doc->selEndL)				 	selFlag = '}';
	}	

	LogPrintf(LOG_INFO, "%c%d", selFlag, kount);
	if (showLinks)
		LogPrintf(LOG_INFO, " L%u", pL);

	ps = NameObjType(pL);
	LogPrintf(LOG_INFO, " xd=%d yd=%d %s %c%c%c%c%c%c oRect.l=p%d",
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
			if (!nonMain)
				LogPrintf(LOG_INFO, " sr=%d mrRect(t,l,b,r)=p%d,%d,%d,%d nst=%d nsys=%d",
					doc->srastral,
					doc->marginRect.top, doc->marginRect.left,
					doc->marginRect.bottom, doc->marginRect.right,
					doc->nstaves,
					doc->nsystems);
			break;
		case SYNCtype:
			if (!nonMain) LogPrintf(LOG_INFO, " TS=%u LT=%ld", SyncTIME(pL),
												GetLTime(doc, pL));
			break;
		case RPTENDtype:
			LogPrintf(LOG_INFO, " lRpt=L%u rRpt=L%u fObj=L%u subTp=%hd",
								((PRPTEND)p)->startRpt, ((PRPTEND)p)->endRpt,
								((PRPTEND)p)->firstObj, ((PRPTEND)p)->subType);
			break;
		case PAGEtype:
			LogPrintf(LOG_INFO, " p#=%d", ((PPAGE)p)->sheetNum);
			break;
		case SYSTEMtype:
			LogPrintf(LOG_INFO, " sRect(t,l,b,r)=d%d,%d,%d,%d s#=%d",
				((PSYSTEM)p)->systemRect.top, ((PSYSTEM)p)->systemRect.left,
				((PSYSTEM)p)->systemRect.bottom, ((PSYSTEM)p)->systemRect.right,
								((PSYSTEM)p)->systemNum);
			break;
		case MEASUREtype:
			LogPrintf(LOG_INFO, " Box(t,l,b,r)=p%d,%d,%d,%d s%%=%d TS=%ld",
				((PMEASURE)p)->measureBBox.top, ((PMEASURE)p)->measureBBox.left,
				((PMEASURE)p)->measureBBox.bottom, ((PMEASURE)p)->measureBBox.right,
				((PMEASURE)p)->spacePercent,
				((PMEASURE)p)->lTimeStamp);
			break;
		case CLEFtype:
			LogPrintf(LOG_INFO, " %c", (((PCLEF)p)->inMeasure? 'M' : '.') );
			break;
		case KEYSIGtype:
			LogPrintf(LOG_INFO, " %c", (((PKEYSIG)p)->inMeasure? 'M' : '.') );
			break;
		case TIMESIGtype:
			LogPrintf(LOG_INFO, " %c", (((PTIMESIG)p)->inMeasure? 'M' : '.') );
			break;
		case BEAMSETtype:
			LogPrintf(LOG_INFO, " stf=%d v=%d", ((PBEAMSET)p)->staffn, ((PBEAMSET)p)->voice);
			break;
		case DYNAMtype:
			LogPrintf(LOG_INFO, " type=%d", ((PDYNAMIC)p)->dynamicType);
			if (showLinks) LogPrintf(LOG_INFO, " 1stS=L%u lastS=L%u",
									((PDYNAMIC)p)->firstSyncL, ((PDYNAMIC)p)->lastSyncL);
			break;
		case GRAPHICtype:
			LogPrintf(LOG_INFO, " stf=%d type=%d", ((PGRAPHIC)p)->staffn,
								((PGRAPHIC)p)->graphicType);
			if (showLinks) {
				LogPrintf(LOG_INFO, " fObj=L%u", ((PGRAPHIC)p)->firstObj);
				if (((PGRAPHIC)p)->graphicType==GRDraw)
					LogPrintf(LOG_INFO, " lObj=L%u", ((PGRAPHIC)p)->lastObj);
			}
			break;
		case OTTAVAtype:
			LogPrintf(LOG_INFO, " stf=%d octType=%d", ((POTTAVA)p)->staffn,
								((POTTAVA)p)->octSignType);
			break;
		case SLURtype:
			LogPrintf(LOG_INFO, " stf=%d v=%d", ((PSLUR)p)->staffn, ((PSLUR)p)->voice);
			if (((PSLUR)p)->tie) LogPrintf(LOG_INFO, " Tie");
			if (((PSLUR)p)->crossSystem) LogPrintf(LOG_INFO, " xSys");
			if (((PSLUR)p)->crossStaff) LogPrintf(LOG_INFO, " xStf");
			if (((PSLUR)p)->crossStfBack) LogPrintf(LOG_INFO, " xStfBk");
			if (showLinks) LogPrintf(LOG_INFO, " L%u->L%u", ((PSLUR)p)->firstSyncL,
													((PSLUR)p)->lastSyncL);
			break;
		case TUPLETtype:
			LogPrintf(LOG_INFO, " stf=%d v=%d num=%d denom=%d", ((PTUPLET)p)->staffn,
								((PTUPLET)p)->voice,
								((PTUPLET)p)->accNum, ((PTUPLET)p)->accDenom);
			break;
		case TEMPOtype:
			LogPrintf(LOG_INFO, " stf=%d", ((PTEMPO)p)->staffn);
			if (showLinks) LogPrintf(LOG_INFO, " fObj=L%u", ((PTEMPO)p)->firstObjL);
			break;
		case SPACERtype:
			LogPrintf(LOG_INFO, " spWidth=%d", ((PSPACER)p)->spWidth);
			break;
		case ENDINGtype:
			LogPrintf(LOG_INFO, " stf=%d num=%d", ((PENDING)p)->staffn, ((PENDING)p)->endNum);
			if (showLinks) LogPrintf(LOG_INFO, " L%u->L%u", ((PENDING)p)->firstObjL,
												 				((PENDING)p)->lastObjL);
			break;
		default:
			;
	}
	
	if (p->nEntries!=0)	LogPrintf(LOG_INFO, " n=%d\n", p->nEntries);
	else				LogPrintf(LOG_INFO, "\n");

	if (showSubs)
		switch (ObjLType(pL))
	{
		case HEADERtype:
			for (partL = FirstSubLINK(pL); partL; partL = NextPARTINFOL(partL)) {
				pPartInfo = GetPPARTINFO(partL);
				LogPrintf(LOG_INFO, "     partL=L%u next=L%u firstst=%d lastst=%d velo=%d transp=%d name=%s\n",
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
				LogPrintf(LOG_INFO, "     ");
				
/* Be careful with addresses provided by the following; they can change suddenly! */
				if (OptionKeyDown()) LogPrintf(LOG_INFO, "@%lx:", aNote);
				LogPrintf(LOG_INFO, 
					"stf=%d v=%d xd=%d yd=%d ystm=%d yqpit=%d ldur=%d .s=%d acc=%d onV=%d %c%c%c%c %c%c%c%c %c%c%c 1stMod=%d\n",
					aNote->staffn, aNote->voice,
					aNote->xd, aNote->yd, aNote->ystem, aNote->yqpit,
					aNote->subType,
					aNote->ndots,
					aNote->accident,
					aNote->onVelocity,
					(aNote->selected? 'S' : '.') ,
					(aNote->visible? 'V' : '.') ,
					(aNote->soft? 'S' : '.') ,
					(aNote->inChord? 'C' : '.') ,
					(aNote->rest? 'R' : '.'),
					(aNote->beamed? 'B' : '.'),
					(aNote->tiedL? ')' : '.'),
					(aNote->tiedR? '(' : '.'),
					(aNote->slurredL? '>' : '.'),
					(aNote->slurredR? '<' : '.'),
					(aNote->inTuplet? 'T' : '.'),
					aNote->firstMod );
			}
			break;
		case GRSYNCtype:
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextGRNOTEL(aNoteL)) {
				aNote = GetPAGRNOTE(aNoteL);
				LogPrintf(LOG_INFO, 
					"     stf=%d v=%d xd=%d yd=%d ystm=%d yqpit=%d ldur=%d .s=%d acc=%d onV=%d %c%c%c%c %c%c%c 1stMod=%d\n",
					aNote->staffn, aNote->voice,
					aNote->xd, aNote->yd, aNote->ystem, aNote->yqpit,
					aNote->subType,
					aNote->ndots,
					aNote->accident,
					aNote->onVelocity,
					(aNote->selected? 'S' : '.') ,
					(aNote->visible? 'V' : '.') ,
					(aNote->soft? 'S' : '.') ,
					(aNote->inChord? 'C' : '.') ,
					(aNote->beamed? 'B' : '.'),
					(aNote->slurredL? '>' : '.'),
					(aNote->slurredR? '<' : '.'),
					aNote->firstMod );
			}
			break;
		case STAFFtype:
			for (aStaffL=FirstSubLINK(pL); aStaffL; aStaffL=NextSTAFFL(aStaffL)) {
				aStaff = GetPASTAFF(aStaffL);
				LogPrintf(LOG_INFO, "     stf=%d top,left,ht,rt=d%d,%d,%d,%d lines=%d fontSz=%d %c%c clef=%d TS=%d,%d/%d\n",
					aStaff->staffn, aStaff->staffTop,
					aStaff->staffLeft, aStaff->staffHeight,
					aStaff->staffRight, aStaff->staffLines,
					aStaff->fontSize,
					(aStaff->selected? 'S' : '.') ,
					(aStaff->visible? 'V' : '.'),
					aStaff->clefType,
					aStaff->timeSigType,
					aStaff->numerator,
					aStaff->denominator );
			}
			break;
		case MEASUREtype:
			for (aMeasureL=FirstSubLINK(pL); aMeasureL; 
					aMeasureL=NextMEASUREL(aMeasureL)) {
				aMeasure = GetPAMEASURE(aMeasureL);
				LogPrintf(LOG_INFO, 
					"     stf=%d m#=%d barTp=%d conStf=%d clef=%d mR=d%d,%d,%d,%d %c%c%c%c%c nKS=%d TS=%d,%d/%d\n",
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
				LogPrintf(LOG_INFO, 
					"     stf=%d subTp=%d conStf=%d\n",
					aPseudoMeas->staffn,
					aPseudoMeas->subType,
					aPseudoMeas->connStaff );
			}
			break;
		case CLEFtype:
			for (aClefL=FirstSubLINK(pL); aClefL; aClefL=NextCLEFL(aClefL)) {
				aClef = GetPACLEF(aClefL);
				LogPrintf(LOG_INFO, "     stf=%d xd=%d clef=%d %c%c%c\n",
					aClef->staffn, aClef->xd, aClef->subType,
					(aClef->selected? 'S' : '.'),
					(aClef->visible? 'V' : '.'),
					(aClef->soft? 'S' : '.') );
			}
			break;
		case KEYSIGtype:
			for (aKeySigL=FirstSubLINK(pL); aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL)) {
				aKeySig = GetPAKEYSIG(aKeySigL);
				LogPrintf(LOG_INFO, "     stf=%d xd=%d %c%c%c nKSItems=%d",
					aKeySig->staffn, 
					aKeySig->xd,
					(aKeySig->selected? 'S' : '.'),
					(aKeySig->visible? 'V' : '.'),
					(aKeySig->soft? 'S' : '.'),
					aKeySig->nKSItems );
				if (aKeySig->nKSItems==0)
					LogPrintf(LOG_INFO, " nNatItems=%d", aKeySig->subType);
				DKeySigPrintf((PKSINFO)(&aKeySig->KSItem[0]));
//if (DETAIL_SHOW) NHexDump(LOG_DEBUG, "DisplayObject/aKeySig", (unsigned char *)(&aKeySig->KSItem[0]),
//					sizeof(AKEYSIG), 4, 16);
			}
			break;
		case TIMESIGtype:
			for (aTimeSigL=FirstSubLINK(pL); aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL)) {
				aTimeSig = GetPATIMESIG(aTimeSigL);
				LogPrintf(LOG_INFO, "     stf=%d xd=%d type=%d,%d/%d %c%c%c\n",
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
				if (showLinks) LogPrintf(LOG_INFO, "     bpSync=L%u ", aNoteBeam->bpSync);
				else				 LogPrintf(LOG_INFO, "     ");
				LogPrintf(LOG_INFO, "startend=%d fracs=%d %c\n",
					aNoteBeam->startend, aNoteBeam->fracs,
					(aNoteBeam->fracGoLeft? 'L' : 'R') );
			}
			break;
		case TUPLETtype:
			for (aNoteTupleL=FirstSubLINK(pL); aNoteTupleL; 
					aNoteTupleL=NextNOTETUPLEL(aNoteTupleL)) {
				aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
				if (showLinks) LogPrintf(LOG_INFO, "     tpSync=L%u\n", aNoteTuple->tpSync);
			}
			break;
		case OTTAVAtype:
			for (aNoteOctL=FirstSubLINK(pL); aNoteOctL; 
					aNoteOctL=NextNOTEOTTAVAL(aNoteOctL)) {
				aNoteOct = GetPANOTEOTTAVA(aNoteOctL);
				if (showLinks) LogPrintf(LOG_INFO, "     opSync=L%u\n", aNoteOct->opSync);
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
					LogPrintf(LOG_INFO, "     '%p'", PCopy(aGraphic->strOffset));
					LogPrintf(LOG_INFO, "\n");				/* Protect newline from garbage strings */
				}
			}
			break;
		case CONNECTtype:
			for (aConnectL=FirstSubLINK(pL); aConnectL; 
					aConnectL=NextCONNECTL(aConnectL)) {
				aConnect = GetPACONNECT(aConnectL);
				LogPrintf(LOG_INFO, "     xd=%d lev=%d type=%d stfA=%d stfB=%d %c\n",
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
				LogPrintf(LOG_INFO, "     stf=%d xd=%d yd=%d endxd=%d %c%c%c\n",
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
				LogPrintf(LOG_INFO, "     1stInd=%d lastInd=%d ctl pts=(%P %P %P %P) %c%c%c\n",
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
	const char *ps;
	LINK pL;
	register HEAP *theHeap;
	unsigned short objCount[LASTtype], h;

	/* Get the total number of objects of each type and the number of note modifiers
		in both the main and the Master Page object lists. */

	CountSubobjsByHeap(doc, objCount, True);

	LogPrintf(LOG_INFO, "HEAP USAGE:\n");
 	for (h = FIRSTtype; h<LASTtype; h++) {
 		theHeap = Heap + h;
		if (theHeap->nObjs<=0) continue;
 		
		ps = NameHeapType(h, False);
 		if (!OptionKeyDown())
 			LogPrintf(LOG_INFO, "  Heap %d (%s): %u in use (%d bytes each)\n", h, ps,
							objCount[h], subObjLength[h]);
							
		/* In memory, blocks of all types are of a constant size, but in files, OBJECTs
		   take only as much space as that type of object needs. */
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

 	LogPrintf(LOG_INFO, "Size of *=%ld DOCHDR=%ld SCOREHDR=%ld *=%ld strPool=%ld\n",
 					sizeof(long)+sizeof(long),
 					sizeof(DOCUMENTHDR), sizeof(SCOREHEADER),
 					sizeof(short), GetHandleSize((Handle)doc->stringPool));
 	LogPrintf(LOG_INFO, "        *=%ld { heapHdrs=%ld HeapsMem/File=%ld/%ld }\n",
					sizeof(long), heapHdrSize, heapSize+objHeapMemSize, heapSize+objHeapFileSize);
 	LogPrintf(LOG_INFO, "TOTAL Mem/File=%ld/%ld\n", mTotal, fTotal);
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
	LogPrintf(LOG_INFO, "%c%d (L%2d) ", selFlag, kount, pL);
	ps = NameObjType(pL);
	LogPrintf(LOG_INFO, "%s", ps);
	p = GetPMEVENT(pL);
	switch (ObjLType(pL)) {
		case HEADERtype:
			LogPrintf(LOG_INFO, "\t");					/* Align since info printed is short */
			break;
		case PAGEtype:
			LogPrintf(LOG_INFO, " #%d", ((PPAGE)p)->sheetNum);
			break;
		case SYSTEMtype:
			LogPrintf(LOG_INFO, " #%d", ((PSYSTEM)p)->systemNum);
			break;
		default:
			;
	}
	(*inLinep)++;
	if (*inLinep>=4 || ObjLType(pL)==TAILtype) {
		LogPrintf(LOG_INFO, "\n");
		*inLinep = 0;
	}
	else
		LogPrintf(LOG_INFO, "\t");
}


/* -------------------------------------------------------------------------- NHexDump -- */
/* Dump the specified block of memory, displayed as bytes in hexadecimal, into the log
file. */

void NHexDump(short logLevel,
				char *label,
				unsigned char *pBuffer,
				long nBytes,
				short nPerGroup,		/* Number of items to print in a group */
				short nPerLine			/* Number of items to print in a line */
				)
{
	long l;
	unsigned short n;
	char blankLabel[256];
	Boolean firstTime=True;
	
	if (strlen(label)>255 || nPerLine>255) {
		MayErrMsg("Illegal call to NHexDump: label or nPerLine is too large.\n");
		return;
	}
		
	for (n = 0; n<strlen(label); n++) blankLabel[n] = ' ';
	blankLabel[strlen(label)] = 0;
	
	/* Assemble lines and display complete lines. */
	
	strBuf[0] = 0;
	for (l = 0; l<nBytes; l++) {
		sprintf(&strBuf[strlen(strBuf)], "%02x", pBuffer[l]); 
		if ((l+1)%(long)nPerLine==0L) {
			sprintf(&strBuf[strlen(strBuf)], "\n");
			LogPrintf(logLevel, "%s: %s", (firstTime? label : blankLabel), strBuf);
			strBuf[0] = 0;
			firstTime = False;
		}
		else if ((l+1)%(long)nPerGroup==0L)	sprintf(&strBuf[strlen(strBuf)], "   ");
		else								sprintf(&strBuf[strlen(strBuf)], " ");
	}
	
	/* Display anything left over. */

	if (l%(long)nPerLine!=0L) {
		sprintf(&strBuf[strlen(strBuf)], "\n");
		LogPrintf(logLevel, "%s: %s", (firstTime? label : blankLabel), strBuf);
	}
}



/* -------------------------------------------------------------------------- NObjDump -- */
/* Dump objects in the range [nFrom, nTo], displayed as bytes in hexadecimal, into the
log file. Caveat: (1) the object numbers are position numbers in the object heap, not
links! For example, of v. 5.8, the TAIL of the main object list is always LINK 2, but
its position for any valid score is at least 11. (2) The number of bytes shown per
object is a constant, but the lengths of objects vary; so objects may have garbage
at their ends or may be truncated. FIXME: Don't display slots in the object heap that are
in its free list! */

typedef struct {
	OBJECTHEADER
} OBJHDR;

#define NBYTES_BODY 16

void NObjDump(char *label, short nFrom, short nTo)
{
	unsigned char *pSObj;
	char mStr[12];				/* Large enough for any 32-bit number, signed or not */
	short objType;
//	LINK pL;
	HEAP *objHeap = Heap + OBJtype;

	for (short m = nFrom; m<=nTo; m++) {
		pSObj = (unsigned char *)GetPSUPEROBJ(m);
//		pL = PtrToLink(objHeap, pSObj);
//LogPrintf(LOG_DEBUG, "NObjDump1: pL=%u type=%d\n", pL, ObjLType(pL));
		objType = ((OBJHDR *)pSObj)->type;
LogPrintf(LOG_DEBUG, "NObjDump2: m=%d type=%d\n", m, objType);
		/* Dump the object header first, than (a fixed-length part) of the body. */
		
		sprintf(mStr, "%s%d%c", label, m, 'H');
		NHexDump(LOG_DEBUG, mStr, pSObj, sizeof(OBJHDR), 4, 16);
		sprintf(mStr, "%s%d%c", label, m, 'B');		
		NHexDump(LOG_DEBUG, mStr, pSObj, NBYTES_BODY, 4, 16);

#if 0
		if (objType==TAILtype && m<nTo) {
			LogPrintf(LOG_DEBUG, "NObjDump: quitting because TAIL reached.\n");
			return;
		}
#endif
	}
}
