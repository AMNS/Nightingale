/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2020 by Avian Music Notation Foundation. All Rights Reserved.
 */

/* File DebugDisplay.c - printing functions for the Debug command and debugging in general:
	KeySigSprintf			DKeySigPrintf			SubobjCountIsBad
	DisplayNote				DisplayGRNote			DisplaySubobjects
	DisplayObject			MemUsageStats			DisplayIndexNode
	DHexDump				DSubobjDump				DSubobj5Dump
	DObjDump				DPrintRow
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


/* --------------------------------------------------------- DisplayObject and helpers -- */

/* FIXME: Shouldn't SubobjCountIsBad also warn if there are _fewer_ subobjs than expected? */

static Boolean SubobjCountIsBad(short nEntries, short subCnt);
static Boolean SubobjCountIsBad(short nEntries, short subCnt)
{
	if (subCnt>nEntries) LogPrintf(LOG_WARNING, "FOUND %d SUBOBJECT(S), MORE THAN THE %d EXPECTED!\n",
									subCnt, nEntries);
	return subCnt>nEntries;
}

#define LogPrintfINDENT LogPrintf(LOG_INFO, "       ")

/* Caveat: The DisplayNote and DisplayGRNote functions each take a pointer as a parameter.
If they cause memory to be moved (which seems very unlikely with modern machines but might
be possible) the pointer would be invalid when they return.  --DAB, May 2023 */

void DisplayNote(PANOTE aNote, Boolean addLabel)
{
	if (addLabel) LogPrintf(LOG_DEBUG, "DisplayNote: @%lx ", aNote);
	LogPrintf(LOG_INFO, 
		"stf=%d v=%d xd=%d yd=%d ystm=%d yqpit=%d ldur=%d .s=%d acc=%d onV=%d %c%c%c%c %c%c%c%c %c%c%c 1stMod=%d\n",
		aNote->staffn, aNote->voice, aNote->xd, aNote->yd,
		aNote->ystem, aNote->yqpit, aNote->subType, aNote->ndots,
		aNote->accident, aNote->onVelocity,
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

void DisplayGRNote(PAGRNOTE aGRNote, Boolean addLabel)
{
	if (addLabel) LogPrintf(LOG_DEBUG, "DisplayGRNote: @%lx ", aGRNote);
	LogPrintf(LOG_INFO, 
		"stf=%d v=%d xd=%d yd=%d ystm=%d yqpit=%d ldur=%d .s=%d acc=%d onV=%d %c%c%c%c %c%c%c 1stMod=%d\n",
		aGRNote->staffn, aGRNote->voice, aGRNote->xd, aGRNote->yd,
		aGRNote->ystem, aGRNote->yqpit, aGRNote->subType, aGRNote->ndots,
		aGRNote->accident, aGRNote->onVelocity,
		(aGRNote->selected? 'S' : '.') ,
		(aGRNote->visible? 'V' : '.') ,
		(aGRNote->soft? 'S' : '.') ,
		(aGRNote->inChord? 'C' : '.') ,
		(aGRNote->beamed? 'B' : '.'),
		(aGRNote->slurredL? '>' : '.'),
		(aGRNote->slurredR? '<' : '.'),
		aGRNote->firstMod );
}


static void DisplaySubobjects(LINK objL, Boolean showLinks);
static void DisplaySubobjects(LINK objL, Boolean showLinks)
{
	PPARTINFO		pPartInfo;
	PANOTE			aNote;
	PAGRNOTE		aGRNote;
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
	LINK			aNoteL, aGRNoteL;
	LINK			aStaffL, aMeasureL, aPseudoMeasL, aClefL, aKeySigL, aNoteBeamL,
					aNoteTupleL, aTimeSigL, aDynamicL, aConnectL, aSlurL, aNoteOctL,
					partL;
	short			nEntries = LinkNENTRIES(objL);
	short			subCnt = 1;
	
	switch (ObjLType(objL)) {
		case HEADERtype:
			for (partL = FirstSubLINK(objL); partL; partL = NextPARTINFOL(partL), subCnt++) {
				pPartInfo = GetPPARTINFO(partL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", partL);
				LogPrintf(LOG_INFO, "firstst=%d lastst=%d velo=%d transp=%d name=%s\n",
					pPartInfo->firstStaff, pPartInfo->lastStaff, pPartInfo->partVelocity,
					pPartInfo->transpose, pPartInfo->name);
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case SYNCtype:
			for (aNoteL=FirstSubLINK(objL); aNoteL; aNoteL=NextNOTEL(aNoteL), subCnt++) {
				LogPrintfINDENT;
				
				/* Be careful with addresses provided by the following; they can change suddenly! */
				
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aNoteL);
				aNote = GetPANOTE(aNoteL);
				if (DETAIL_SHOW) LogPrintf(LOG_INFO, "@%lx ", aNote);
				DisplayNote(aNote, False);
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case GRSYNCtype:
			for (aGRNoteL=FirstSubLINK(objL); aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL), subCnt++) {
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aGRNoteL);
				aGRNote = GetPAGRNOTE(aGRNoteL);
				if (DETAIL_SHOW) LogPrintf(LOG_INFO, "@%lx ", aGRNote);
				DisplayGRNote(aGRNote, False);
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case STAFFtype:
			for (aStaffL=FirstSubLINK(objL); aStaffL; aStaffL=NextSTAFFL(aStaffL), subCnt++) {
				aStaff = GetPASTAFF(aStaffL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aStaffL);
				LogPrintf(LOG_INFO,
					"stf=%d tlbr=d%d,%d,%d,%d lines=%d fontSz=%d %c%c clef=%d TS=%d,%d/%d\n",
					aStaff->staffn, aStaff->staffTop,
					aStaff->staffLeft, aStaff->staffHeight,
					aStaff->staffRight, aStaff->staffLines,
					aStaff->fontSize,
					(aStaff->selected? 'S' : '.') ,
					(aStaff->visible? 'V' : '.'),
					aStaff->clefType,
					aStaff->timeSigType, aStaff->numerator, aStaff->denominator);
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case MEASUREtype:
			for (aMeasureL=FirstSubLINK(objL); aMeasureL; 
					aMeasureL=NextMEASUREL(aMeasureL), subCnt++) {
				aMeasure = GetPAMEASURE(aMeasureL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aMeasureL);
				LogPrintf(LOG_INFO, 
					"stf=%d m#=%d barTp=%d conStf=%d clef=%d mR tlbr=d%d,%d,%d,%d %c%c%c%c%c nKS=%d TS=%d,%d/%d\n",
					aMeasure->staffn, aMeasure->measureNum, aMeasure->subType,
					aMeasure->connStaff, aMeasure->clefType,
					aMeasure->measSizeRect.top, aMeasure->measSizeRect.left,
					aMeasure->measSizeRect.bottom, aMeasure->measSizeRect.right,
					(aMeasure->selected? 'S' : '.'),
					(aMeasure->visible? 'V' : '.'),
					(aMeasure->soft? 'S' : '.'),
					(aMeasure->measureVisible? 'M' : '.'),
					(aMeasure->connAbove? 'C' : '.'),
					aMeasure->nKSItems,
					aMeasure->timeSigType, aMeasure->numerator, aMeasure->denominator );
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case PSMEAStype:
			for (aPseudoMeasL=FirstSubLINK(objL); aPseudoMeasL; 
					aPseudoMeasL=NextPSMEASL(aPseudoMeasL), subCnt++) {
				aPseudoMeas = GetPAPSMEAS(aPseudoMeasL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aPseudoMeasL);
				LogPrintf(LOG_INFO, "stf=%d subTp=%d conStf=%d\n",
					aPseudoMeas->staffn, aPseudoMeas->subType, aPseudoMeas->connStaff );
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case CLEFtype:
			for (aClefL=FirstSubLINK(objL); aClefL; aClefL=NextCLEFL(aClefL), subCnt++) {
				aClef = GetPACLEF(aClefL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aClefL);
				LogPrintf(LOG_INFO, "stf=%d xd=%d clef=%d %c%c%c\n",
					aClef->staffn, aClef->xd, aClef->subType,
					(aClef->selected? 'S' : '.'),
					(aClef->visible? 'V' : '.'),
					(aClef->soft? 'S' : '.') );
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case KEYSIGtype:
			for (aKeySigL=FirstSubLINK(objL); aKeySigL; aKeySigL=NextKEYSIGL(aKeySigL), subCnt++) {
				aKeySig = GetPAKEYSIG(aKeySigL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aKeySigL);
				LogPrintf(LOG_INFO, "stf=%d xd=%d %c%c%c nKSItems=%d",
					aKeySig->staffn, aKeySig->xd,
					(aKeySig->selected? 'S' : '.'),
					(aKeySig->visible? 'V' : '.'),
					(aKeySig->soft? 'S' : '.'),
					aKeySig->nKSItems );
				if (aKeySig->nKSItems==0)
					LogPrintf(LOG_INFO, " nNatItems=%d", aKeySig->subType);
				DKeySigPrintf((PKSINFO)(&aKeySig->KSItem[0]));
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case TIMESIGtype:
			for (aTimeSigL=FirstSubLINK(objL); aTimeSigL; aTimeSigL=NextTIMESIGL(aTimeSigL)) {
				aTimeSig = GetPATIMESIG(aTimeSigL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aTimeSigL);
				LogPrintf(LOG_INFO, "stf=%d xd=%d type=%d,%d/%d %c%c%c\n",
					aTimeSig->staffn, aTimeSig->xd,
					aTimeSig->subType, aTimeSig->numerator, aTimeSig->denominator,
					(aTimeSig->selected? 'S' : '.'),
					(aTimeSig->visible? 'V' : '.'),
					(aTimeSig->soft? 'S' : '.') );
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case BEAMSETtype:
			for (aNoteBeamL=FirstSubLINK(objL); aNoteBeamL; 
					aNoteBeamL=NextNOTEBEAML(aNoteBeamL), subCnt++) {
				aNoteBeam = GetPANOTEBEAM(aNoteBeamL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u bpSync=L%u ", aNoteBeamL,
					aNoteBeam->bpSync);
				LogPrintf(LOG_INFO, "startend=%d fracs=%d %c\n", aNoteBeam->startend,
					aNoteBeam->fracs, (aNoteBeam->fracGoLeft? 'L' : 'R') );
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case TUPLETtype:
			for (aNoteTupleL=FirstSubLINK(objL); aNoteTupleL; 
					aNoteTupleL=NextNOTETUPLEL(aNoteTupleL), subCnt++) {
				aNoteTuple = GetPANOTETUPLE(aNoteTupleL);
				if (showLinks) {
					LogPrintfINDENT;
					LogPrintf(LOG_INFO, "L%u tpSync=L%u\n", aNoteTupleL, aNoteTuple->tpSync);
				}
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case OTTAVAtype:
			for (aNoteOctL=FirstSubLINK(objL); aNoteOctL; 
					aNoteOctL=NextNOTEOTTAVAL(aNoteOctL), subCnt++) {
				aNoteOct = GetPANOTEOTTAVA(aNoteOctL);
				if (showLinks) {
					LogPrintfINDENT;
					LogPrintf(LOG_INFO, "L%u opSync=L%u\n", aNoteOctL, aNoteOct->opSync);
				}
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case GRAPHICtype: {
				LINK aGraphicL;  PAGRAPHIC aGraphic;
				PGRAPHIC pGraphic;  char tmpPStr[256];
				pGraphic = GetPGRAPHIC(objL);
				if (pGraphic->graphicType==GRString
				||  pGraphic->graphicType==GRLyric
				||  pGraphic->graphicType==GRRehearsal
				||  pGraphic->graphicType==GRChordSym) {
					aGraphicL = FirstSubLINK(objL);
					aGraphic = GetPAGRAPHIC(aGraphicL);
					LogPrintfINDENT;
					if (showLinks) LogPrintf(LOG_INFO, "L%u ", aGraphicL);
					Pstrcpy((unsigned char *)tmpPStr, PCopy(aGraphic->strOffset));
					LogPrintf(LOG_INFO, "'%s'", PToCString((unsigned char *)tmpPStr));
					LogPrintf(LOG_INFO, "\n");				/* Protect newline from garbage strings */
				}
			}
			break;
		case CONNECTtype:
			for (aConnectL=FirstSubLINK(objL); aConnectL; 
					aConnectL=NextCONNECTL(aConnectL), subCnt++) {
				aConnect = GetPACONNECT(aConnectL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aConnectL);
				LogPrintf(LOG_INFO, "xd=%d lev=%d type=%d stfA=%d stfB=%d %c\n",
					aConnect->xd, aConnect->connLevel, aConnect->connectType,
					aConnect->staffAbove, aConnect->staffBelow,
					(aConnect->selected? 'S' : '.'));
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case DYNAMtype:
			for (aDynamicL=FirstSubLINK(objL); aDynamicL; 
					aDynamicL=NextDYNAMICL(aDynamicL), subCnt++) {
				aDynamic = GetPADYNAMIC(aDynamicL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aDynamicL);
				LogPrintf(LOG_INFO, "stf=%d xd=%d yd=%d endxd=%d %c%c%c\n",
					aDynamic->staffn, aDynamic->xd, aDynamic->yd, aDynamic->endxd,
					(aDynamic->selected? 'S' : '.'),
					(aDynamic->visible? 'V' : '.'),
					(aDynamic->soft? 'S' : '.') );
				
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		case SLURtype:
			for (aSlurL=FirstSubLINK(objL); aSlurL; aSlurL=NextSLURL(aSlurL), subCnt++) {
				aSlur = GetPASLUR(aSlurL);
				LogPrintfINDENT;
				if (showLinks) LogPrintf(LOG_INFO, "L%u ", aSlurL);
				LogPrintf(LOG_INFO, "1stInd=%d lastInd=%d ctl pts=(%d,%d),(%d,%d),(%d,%d),(%d,%d) %c%c%c\n",
					aSlur->firstInd, aSlur->lastInd, aSlur->seg.knot.h, aSlur->seg.knot.v,
					aSlur->seg.c0.h, aSlur->seg.c0.v, aSlur->seg.c1.h, aSlur->seg.c1.v,
					aSlur->endKnot.h, aSlur->endKnot.v,
					(aSlur->selected? 'S' : '.'),
					(aSlur->visible? 'V' : '.'),
					(aSlur->soft? 'S' : '.') );
				if (SubobjCountIsBad(nEntries, subCnt)) break;
			}
			break;
		default:
			break;
	}
}


/* Show information about the given object, and optionally its subobjects. If <nonMain>,
ignore <doc>. */

void DisplayObject(Document *doc, LINK objL,
				short kount,				/* Label to print for object */
				Boolean showLinks,			/* Show object addr., links, size? */
				Boolean showSubs,			/* Show subobjects? */
				Boolean nonMain				/* Somewhere besides doc's main object list? */
				)
{
	POBJHDR		p;
	char		selFlag;
	const char	*ps;

#ifdef DDB
	PushLock(OBJheap);
	p = GetPOBJHDR(objL);
	selFlag = ' ';
	if (!nonMain) {
		if (objL==doc->selStartL && objL==doc->selEndL)	selFlag = '&';
		else if (objL==doc->selStartL)					selFlag = '{';
		else if (objL==doc->selEndL)					selFlag = '}';
	}	

	LogPrintf(LOG_INFO, "%c%d", selFlag, kount);
	if (showLinks)
		LogPrintf(LOG_INFO, " L%u", objL);

	ps = NameObjType(objL);
	LogPrintf(LOG_INFO, " xd=%d yd=%d %s %c%c%c%c%c%c objRect.l=p%d",
					p->xd, p->yd, ps,
					(p->selected? 'S' : '.'),
					(p->visible? 'V' : '.'),
					(p->soft? 'S' : '.'),
					(p->valid? 'V' : '.'),
					(p->tweaked? 'T' : '.'),
					(p->spareFlag? 'S' : '.'),
					p->objRect.left );

	/* Display object-specific info on objects. */

	switch (ObjLType(objL)) {
		case HEADERtype:
			if (!nonMain)
				LogPrintf(LOG_INFO, " sr=%d mrRect tlbr=p%d,%d,%d,%d nst=%d nsys=%d",
					doc->srastral,
					doc->marginRect.top, doc->marginRect.left,
					doc->marginRect.bottom, doc->marginRect.right,
					doc->nstaves,
					doc->nsystems);
			break;
		case SYNCtype:
			if (!nonMain) LogPrintf(LOG_INFO, " TS=%u LT=%ld", SyncTIME(objL),
												GetLTime(doc, objL));
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
			LogPrintf(LOG_INFO, " sRect tlbr=d%d,%d,%d,%d s#=%d",
				((PSYSTEM)p)->systemRect.top, ((PSYSTEM)p)->systemRect.left,
				((PSYSTEM)p)->systemRect.bottom, ((PSYSTEM)p)->systemRect.right,
								((PSYSTEM)p)->systemNum);
			break;
		case MEASUREtype:
			/* The "s%%=%d" in the below statement should print something like "s%=100",
			   and it did in versions of Nightingale of years ago; but the percent sign
			   is missing in versions at least as far back as 5.6, obviously due to a
			   compiler or run-time library bug, and several workarounds I tried failed.
			   --DAB, April 2022 */
			   
			LogPrintf(LOG_INFO, " Box tlbr=p%d,%d,%d,%d s%%=%d TS=%ld",
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

	/* If desired, display info on subobjects now. */
	
	if (showSubs) DisplaySubobjects(objL, showLinks);
	
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
	HEAP *theHeap;
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

void DisplayIndexNode(Document *doc, LINK pL, short kount, short *inLinep)
{
	POBJHDR		p;
	char		selFlag;
	const char 	*ps;

	p = GetPOBJHDR(pL);
	if (pL==doc->selStartL && pL==doc->selEndL)	selFlag = '&';
	else if (pL==doc->selStartL)				selFlag = '{';
	else if (pL==doc->selEndL)					selFlag = '}';
	else										selFlag = ' ';
	LogPrintf(LOG_INFO, "%c%d (L%2d) ", selFlag, kount, pL);
	ps = NameObjType(pL);
	LogPrintf(LOG_INFO, "%s", ps);
	p = GetPOBJHDR(pL);
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


/* -------------------------------------------------------------------------- DHexDump -- */
/* Dump the specified area as bytes in hexadecimal into the log file. Similar to the
Unix command-line utility hexdump: in particular, HexDump() with nPerGroup = nPerLine is
a lot like <hexdump -n nBytes>. */

void DHexDump(short logLevel,			/* From LOG_DEBUG to LOG_ERR */
				char *label,
				Byte *pBuffer,
				long nBytes,
				short nPerGroup,		/* Number of items to print in a group */
				short nPerLine,			/* Number of items to print in a line */
				Boolean numberLines		/* Add line numbers? */
				)
{
	long pos;
	unsigned short n, lineNum;
	char blankLabel[256], blankLabelC[256], labelC[256], lineNumStr[10];
	Boolean firstTime=True;
	
	if (strlen(label)>255 || nPerLine>255) {
		MayErrMsg("Illegal call to DHexDump: label or nPerLine is too large.\n");
		return;
	}
	strcpy(labelC, label);
	
	for (n = 0; n<strlen(labelC); n++) blankLabel[n] = ' ';
	blankLabel[strlen(labelC)] = 0;
	if (numberLines) strcat(labelC, "  0");
	
	/* Assemble lines and display complete lines. */
	
	strBuf[0] = 0;
	lineNum = 0;
	for (pos = 0; pos<nBytes; pos++) {
		sprintf(&strBuf[strlen(strBuf)], "%02x", pBuffer[pos]); 
		if ((pos+1)%(long)nPerLine==0L) {
			sprintf(&strBuf[strlen(strBuf)], "\n");

			strcpy(blankLabelC, blankLabel);
			if (!firstTime && numberLines) {
				lineNum++;
				sprintf(lineNumStr, "%3d", lineNum);
				strcat(blankLabelC, lineNumStr);
			}
			else if (numberLines)
				strcat(blankLabelC, "   ");
			//printf("%s: %s", (firstTime? labelC : blankLabelC), strBuf);	??WHY DO THIS?
			LogPrintf(logLevel, "%s: %s", (firstTime? labelC : blankLabelC), strBuf);
			strBuf[0] = 0;
			firstTime = False;
		}
		else if ((pos+1)%(long)nPerGroup==0L)	sprintf(&strBuf[strlen(strBuf)], "   ");
		else									sprintf(&strBuf[strlen(strBuf)], " ");
	}
	
	/* Display anything left over. */

	if (pos%(long)nPerLine!=0L) {
		sprintf(&strBuf[strlen(strBuf)], "\n");
		if (!firstTime && numberLines) {
			lineNum++;
			sprintf(lineNumStr, "%3d", lineNum);
			strcpy(blankLabelC, blankLabel);
			strcat(blankLabelC, lineNumStr);
		}
		LogPrintf(logLevel, "%s: %s", (firstTime? labelC : blankLabelC), strBuf);
	}
}


/* ------------------------------------------------------------ DObjDump, DSubobj?Dump -- */

/* DSubobjDump dumps subobjects in Heap _iHp_ with positions in the range [nFrom, nTo]
into the log file, displaying them in hexadecimal. <pLink1> must point to LINK 1 in the
given Heap. This is intended for use debugging problems reading old-format files. It's
similar to DObjDump, except it's for subobjects instead of objects and it doesn't assume
that LINKs are meaningful. It _does_ assume subobjects are in the current format.

DSubobj5Dump is identical except that it assume subobjects are in 'N105' format; it's
intended for debugging the process of converting from 'N105' to the current format. */

void DSubobj5Dump(short iHp, unsigned char *pLink1, short nFrom, short nTo, Boolean doLabel)
{
	short sLen = subObjLength_5[iHp];
	if (doLabel) LogPrintf(LOG_DEBUG, "DSubobj5Dump: iHp=%d pLink1=%lx\n", iHp, pLink1);
	for (short m = nFrom; m<=nTo; m++) {
		unsigned char *pSObj = pLink1+(m*sLen);
		DHexDump(LOG_DEBUG, "DSubobj5Dump:", pSObj, sLen, 4, 16, True);
	}
}

void DSubobjDump(short iHp, unsigned char *pLink1, short nFrom, short nTo, Boolean doLabel)
{
	short sLen = subObjLength[iHp];
	if (doLabel) LogPrintf(LOG_DEBUG, "DSubobjDump: iHp=%d pLink1=%lx\n", iHp, pLink1);
	for (short m = nFrom; m<=nTo; m++) {
		unsigned char *pSObj = pLink1+(m*sLen);
		DHexDump(LOG_DEBUG, "DSubobjDump:", pSObj, sLen, 4, 16, True);
	}
}


/* Dump objects with positions in the object heap -- not links -- in the range [nFrom,
nTo] into the log file, displaying them in hexadecimal. We assume objects are in the
current format, so if a score is in an old format was just opened, objects should be
converted before this is called. Caveat: positions in the object heap are not necessarily
link values! For a newly-opened score, position n is indeed LINK n; for a score that's
been edited, the relationship between position and LINK isn't easily predictable. */

void DObjDump(char *label, short nFrom, short nTo)
{
	unsigned char *pSObj;
	char mStr[12];				/* Enough digits enough for any 32-bit number, signed or not */
	short theObjType, bodyLength;
	LINK pL;
	Boolean typeIsLegal;
	HEAP *objHeap = Heap + OBJtype;

	for (short m = nFrom; m<=nTo; m++) {
		pSObj = (unsigned char *)GetPSUPEROBJ(m);
		pL = PtrToLink(objHeap, pSObj);
		if (HeapLinkIsFree(objHeap, pL)) {
			LogPrintf(LOG_DEBUG, "DObjDump: %s: heap object %d (link %u) isn't in use.\n",
						label, m, pL);
			continue;
		}

		theObjType = ((OBJHDR *)pSObj)->type;
		LogPrintf(LOG_DEBUG, "DObjDump: %s: heap object %d (link %u) type=%d",
					label, m, pL, theObjType);
		typeIsLegal = (theObjType>=FIRSTtype && theObjType<=LASTtype-1);
		if (typeIsLegal)	LogPrintf(LOG_DEBUG, " (%s) length=%d\n",
								NameHeapType(ObjLType(pL), False), objLength[theObjType]);
		else				LogPrintf(LOG_DEBUG, " IS ILLEGAL.\n");

		/* Dump the object header first, then the body. */
		
		sprintf(mStr, "  %d%c", m, 'H');
		DHexDump(LOG_DEBUG, mStr, pSObj, sizeof(OBJHDR), 4, 16, False);
		bodyLength = typeIsLegal? objLength[theObjType]-sizeof(OBJHDR) : 0;
		if (bodyLength>0) {
			sprintf(mStr, "  %d%c", m, 'B');		
			DHexDump(LOG_DEBUG, mStr, pSObj+sizeof(OBJHDR), bodyLength, 4, 16, False);
		}

#if 0
		if (objType==TAILtype && m<nTo) {
			LogPrintf(LOG_DEBUG, "DObjDump: quitting because TAIL reached.\n");
			return;
		}
#endif
	}
}


/* ------------------------------------------------------------------------- DPrintRow -- */

#define SHOW_GRAPHIC 1		/* Show the row of flags in the obvious graphical way? */
#define SHOW_HEX 0			/* Show the flags as "(offset value)"?

/* Print a row of 1-bit flags, e.g., to display a bitmap as a low-resolution graphic.
Intended for use with bitmaps read from BMP files, where rows are padded, but we have
<startLoc>, so the padding is irrelevant. */

void DPrintRow(Byte bitmap[], short startLoc, short byWidth, short rowNum,
			Boolean foreIsAOne,		/* Is a 1 bit foreground? */
			Boolean skipBits)		/* skip alternate background bits so foreground stands out? */
{
	printf("%3d: ", rowNum);
	for (short kCol = 0; kCol<byWidth; kCol++) {
		if (SHOW_HEX) printf("(%d %02x)", startLoc+kCol, bitmap[startLoc+kCol]);
		if (!SHOW_GRAPHIC) continue;
		Boolean even = False;
//printf("(%d %02x)", startLoc+kCol, bitmap[startLoc+kCol]);
		for (short bn = 0; bn<8; bn++) {
			char backgroundChar = '.';
			
			Boolean aBit = !((bitmap[startLoc+kCol]>>(7-bn)) & 0x01);
			if (skipBits) {
				even = !even;
				backgroundChar = (even? '.' : ' ');
			}
			if (foreIsAOne)	printf("%c", (aBit? backgroundChar : '*'));
			else			printf("%c", (aBit? '*' : backgroundChar));
//printf("bn=%d bn&0x01=%d even=%d bC='%c'\n", bn, bn&0x01, even, backgroundChar);
			
		}
		printf(" ");		
	}
}
