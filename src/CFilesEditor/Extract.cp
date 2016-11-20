/* Extract.c for Nightingale - Extract parts  */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Copyright © 2016 by Avian Music Notation Foundation.
 * All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Prototypes for private routines */

static short	ExtMapStaves(Document *doc, Document *newDoc);
static void		UpdateDocHeader(Document *newDoc);
static void		SelectPart(Document *doc, LINK partL, Boolean extractAllGraphics);
static Boolean	CopyPartInfo(Document *, Document *, LINK);
static Boolean	CopyPartRange(Document *, Document *, LINK, LINK, LINK, LINK);
static Boolean	CopyPart(Document *score, Document *part, LINK partL);
static void		InitDocFields(Document *part, Document *score);
static void		RPDeselAll(Document *);
static short	ReadPart(Document *part, Document *score, LINK partL, Boolean *partOK);
static Boolean	MakeMultibarRest(Document *, LINK, LINK, short, short, COPYMAP *, short);
static void		SetupSysCopyMap(Document *, COPYMAP **, short *);
static void		ExtFixCrossSysSlurs(Document *, COPYMAP *, short);
static Boolean	MultibarRests(Document *, short, Boolean);
static void		FixMeasures(Document *);
static void		ExtCompactVoiceNums(Document *, short);
static void		FixConnects(Document *);


#ifndef PUBLIC_VERSION
void DCheckSyncs(Document *doc)
{
	LINK	pL, aNoteL, aModNRL;
	int		count;
	
	LogPrintf(LOG_NOTICE, "Checking Syncs...\n");

//	DCheckNEntries(doc);

	PushLock(NOTEheap);
	for (pL = doc->headL, count = 1; pL; pL = RightLINK(pL), count++) {
		if (ObjLType(pL)==SYNCtype) {
			for (aNoteL=FirstSubLINK(pL); aNoteL; aNoteL=NextNOTEL(aNoteL)) {
				PAMODNR aModNR;
				PANOTE aNote = GetPANOTE(aNoteL);
				if (aNote->firstMod) {
					unsigned short nObjs = MODNRheap->nObjs;
					aModNRL = aNote->firstMod;
					for ( ; aModNRL; aModNRL=NextMODNRL(aModNRL)) {
						if (aModNRL >= nObjs)	/* very crude check on node validity  -JGG */
							LogPrintf(LOG_WARNING, "DCheckSyncs: SUSPICIOUS MODNR LINK %d IN VOICE %d IN SYNC AT %u.\n",
											aModNRL, aNote->voice, pL);
						aModNR = GetPAMODNR(aModNRL);
						if (!(aModNR->modCode>=MOD_FERMATA && aModNR->modCode<=MOD_LONG_INVMORDENT)
						&&	 !(aModNR->modCode>=0 && aModNR->modCode<=5) )
							LogPrintf(LOG_WARNING, "DCheckSyncs: ILLEGAL MODNR CODE %d IN VOICE %d IN SYNC AT %u.\n",
											aModNR->modCode, aNote->voice, pL);
					}
				}
			}
		}
	}
	PopLock(NOTEheap);
}
#endif


/* In the given score, set the heights of the Systems and the Staff and Measure Tops,
both in the main object list and in the Master System, to be the same and consistent
with their contents. This will not work right unless all staves are visible!

An earlier version of this function worked much harder to set the heights of the
Systems and the Staff and Measure Tops to be consistent with their particular contents.
This didn't work very well, and there's not much point to it anyway, because the user
is likely to want to change system breaks--and therefore staff spacing--throughout
the newly-created part. */

/* Get the default Measure top for the top staff of a System */
#define MEAS_TOP(stHt) (doc->ledgerYSp)*(stHt)/STFHALFLNS

void ExFixMeasAndSysRects(Document *doc)
{
	LINK	staffL, aStaffL, sysL;
	DDIST	staffTop[MAXSTAVES+1];
	DDIST	topMeasTop, staffTopDiff, oldSysRectBottom, sysRectDiff;
	short	s;

	/*
	 * Fill staffTop[] with the current staves' Master Page positions, then adjust all
	 * the positions upwards so the top staff's Measure extends just to the top of the
	 * System, and update all Staff subobjects in the main object list accordingly.
	 */
	FillStaffTopArray(doc, doc->masterHeadL, staffTop);
	
	staffL = SSearch(doc->masterHeadL, STAFFtype, GO_RIGHT);
	aStaffL = FirstSubLINK(staffL);									/* Any staff should work */
	topMeasTop = MEAS_TOP(StaffHEIGHT(aStaffL));
	topMeasTop += StaffHEIGHT(aStaffL)/STFHALFLNS;					/* Allow a bit extra */
	staffTopDiff = staffTop[1]-topMeasTop;
	for (s = 1; s<=doc->nstaves; s++) {
		staffTop[s] -= staffTopDiff;
	}
	
	UpdateStaffTops(doc, doc->headL, doc->tailL, staffTop);
	
	/* Adjust Measures and System heights in the main object list to agree. */
	
	sysL = SSearch(doc->headL, SYSTEMtype, GO_RIGHT);
	oldSysRectBottom = SystemRECT(sysL).bottom;

	FixMeasRectYs(doc, NILINK, TRUE, TRUE, TRUE);
	
	sysRectDiff = SystemRECT(sysL).bottom-oldSysRectBottom;
	
	/*
	 * Adjust Master Page's Staff positions and System height to agree with the
	 * main object list. FIXME: But we started with Master Page! Surely this is mostly,
	 * perhaps entirely, redundant.
	 */
	UpdateStaffTops(doc, doc->masterHeadL, doc->masterTailL, staffTop);
	UpdateMPSysRectHeight(doc, sysRectDiff);
	
}


/* Re-index the staves in the extracted part so that the top staff is number 1.
Return the change in staff numbers. */
 
static short ExtMapStaves(Document *doc, Document *newDoc)
{
	LINK 		pL, subObjL, aStaffL, aConnectL, aMeasureL, aPseudoMeasL;
	short		staffDiff;
	PMEVENT		p;
	PPARTINFO	pPart;
	PAMEASURE	aMeasure;
	PAPSMEAS	aPseudoMeas;
	PACONNECT	aConnect;
	GenSubObj 	*subObj;
	HEAP 		*tmpHeap;

	InstallDoc(newDoc);
	
	pPart = GetPPARTINFO(NextPARTINFOL(FirstSubLINK(newDoc->headL)));
	staffDiff = pPart->firstStaff-1;
	pPart->firstStaff -= staffDiff;
	pPart->lastStaff -= staffDiff;
	
	for (pL = newDoc->headL; pL!=newDoc->tailL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case PAGEtype:
			case SYSTEMtype:
				break;
			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
					StaffSTAFF(aStaffL) -= staffDiff;
				break;
			case CONNECTtype:
				aConnectL = FirstSubLINK(pL);
				for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
					aConnect = GetPACONNECT(aConnectL);
					if (aConnect->connLevel) {
						aConnect->staffAbove -= staffDiff;
						aConnect->staffBelow -= staffDiff;
					}
				}
				break;
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL = NextMEASUREL(aMeasureL)) {
					aMeasure = GetPAMEASURE(aMeasureL);
					aMeasure->staffn -= staffDiff;
					if (!aMeasure->connAbove) {
						if (pPart->firstStaff!=pPart->lastStaff)
							aMeasure->connStaff -= staffDiff;
					}
				}
				break;
			case PSMEAStype:
				aPseudoMeasL = FirstSubLINK(pL);
				for ( ; aPseudoMeasL; aPseudoMeasL = NextPSMEASL(aPseudoMeasL)) {
					aPseudoMeas = GetPAPSMEAS(aPseudoMeasL);
					aPseudoMeas->staffn -= staffDiff;
					if (!aPseudoMeas->connAbove) {
						if (pPart->firstStaff!=pPart->lastStaff)
							aPseudoMeas->connStaff -= staffDiff;
					}
				}
				break;
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case SYNCtype:
			case GRSYNCtype:
			case DYNAMtype:
			case RPTENDtype:
				tmpHeap = newDoc->Heap + ObjLType(pL);
				p = GetPMEVENT(pL);
				
				for (subObjL=FirstSubObjPtr(p,pL); subObjL; subObjL = NextLink(tmpHeap,subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap, subObjL);
					subObj->staffn -= staffDiff;
				}	
				break;
			case SLURtype:
			case BEAMSETtype:
			case TUPLETtype:
			case OTTAVAtype:
			case GRAPHICtype:
			case ENDINGtype:
			case TEMPOtype:
			case SPACERtype:
				p = GetPMEVENT(pL);
				((PEXTEND)p)->staffn -= staffDiff;
				/* 
				 * Some objects (e.g., tempo marks) go into the part regardless of whether
				 * they belong to any of its staves, but we have to be careful that they
				 * end up with a legal staff number in the part -- say, 1.
				 */
				if (((PEXTEND)p)->staffn<1 || ((PEXTEND)p)->staffn>pPart->lastStaff)
					((PEXTEND)p)->staffn = 1;
				break;
		}
	}
	
	InstallDoc(doc);
	return staffDiff;
}


/* Update values in the document header for the part document. */
 
static void UpdateDocHeader(Document *newDoc)
{
	LINK pL, staffL, aStaffL;
	short nPages=0, nSystems=0, nStaves=0;

	InstallDoc(newDoc);
	
	for (pL=newDoc->headL; pL!=newDoc->tailL; pL=RightLINK(pL)) {
		if (PageTYPE(pL)) nPages++;
		if (SystemTYPE(pL)) nSystems++;
	}
	staffL = SSearch(newDoc->headL, STAFFtype, FALSE);
	for (aStaffL=FirstSubLINK(staffL); aStaffL; aStaffL=NextSTAFFL(aStaffL))
		nStaves++;
	
	newDoc->numSheets = nPages;
	newDoc->nsystems = nSystems;
	newDoc->nstaves = nStaves;
}


/* Select all subobjects in the document that belong to the part, plus others that we
want in the part even though they "belong" to other parts--for example, rehearsal amrks
and tempo marks. */
	
static void SelectPart(
				Document *doc,
				LINK partL,
				Boolean extractAllGraphics		/* TRUE=put all "appropriate" Graphics in part */
				)
{
	LINK 		pL, subObjL, aStaffL, aConnectL, aSlurL, firstL;
	PASLUR		aSlur;
	PMEVENT		p;
	PPARTINFO	pPart;
	short		firstStf, lastStf;
	PACONNECT	aConnect;
	GenSubObj 	*subObj;
	HEAP 		*tmpHeap;
	Boolean		inTempoSeries;

	pPart = GetPPARTINFO(partL);
	firstStf = pPart->firstStaff;
	lastStf = pPart->lastStaff;

	inTempoSeries = FALSE;

	for (pL = doc->headL; pL!=doc->tailL; pL = RightLINK(pL)) {
		p = GetPMEVENT(pL);
		switch (ObjLType(pL)) {
			case PAGEtype:
			case SYSTEMtype:
				LinkSEL(pL) = TRUE;
				break;
			case STAFFtype:
				aStaffL = FirstSubLINK(pL);
				for ( ; aStaffL; aStaffL = NextSTAFFL(aStaffL))
					if (StaffSTAFF(aStaffL)>=firstStf && StaffSTAFF(aStaffL)<=lastStf) {
						LinkSEL(pL) = TRUE;
						StaffSEL(aStaffL) = TRUE;
					}
				break;
			case CONNECTtype:
				aConnectL = FirstSubLINK(pL);
				for ( ; aConnectL; aConnectL = NextCONNECTL(aConnectL)) {
					aConnect = GetPACONNECT(aConnectL);
					
					/*
					 * Copy the Connect if it's connLevel is SystemLevel or if it's within
					 * our part.
					 */
					if (!aConnect->connLevel ||
							(aConnect->staffAbove>=firstStf && aConnect->staffBelow<=lastStf)) {
						LinkSEL(pL) = TRUE;
						ConnectSEL(aConnectL) = TRUE;
					}
				}
				break;
			case MEASUREtype:
			case PSMEAStype:
			case CLEFtype:
			case KEYSIGtype:
			case TIMESIGtype:
			case SYNCtype:
			case GRSYNCtype:
			case DYNAMtype:
			case RPTENDtype:
				tmpHeap = doc->Heap + ObjLType(pL);
				
				for (subObjL=FirstSubObjPtr(p, pL); subObjL; subObjL = NextLink(tmpHeap, subObjL)) {
					subObj = (GenSubObj *)LinkToPtr(tmpHeap, subObjL);
					if (subObj->staffn>=firstStf && subObj->staffn<=lastStf) {
						LinkSEL(pL) = TRUE;
						subObj->selected = TRUE;
					}
				}
				inTempoSeries = FALSE;
				break;
			case BEAMSETtype:
			case TUPLETtype:
			case OTTAVAtype:
			case ENDINGtype:
			case SPACERtype:
				if (((PEXTEND)p)->staffn>=firstStf && ((PEXTEND)p)->staffn<=lastStf)
					LinkSEL(pL) = TRUE;
				break;
			/*
			 * Some objects (rehearsal marks and tempo marks always, all Graphics optionally)
			 * go into the part regardless of staff, but only if it's obvious the object
			 * they're attached to has a subobject in the part.
			 */
			case GRAPHICtype:
				/* Page relative graphics go into every part. */
				if (PageTYPE(GraphicFIRSTOBJ(pL)))
					LinkSEL(pL) = TRUE;
				if (((PEXTEND)p)->staffn>=firstStf && ((PEXTEND)p)->staffn<=lastStf)
					LinkSEL(pL) = TRUE;
				else if (GraphicSubType(pL)==GRRehearsal || extractAllGraphics) {
					firstL = GraphicFIRSTOBJ(pL);
					if (LinkBefFirstMeas(firstL) || MeasureTYPE(firstL))
						LinkSEL(pL) = TRUE;
				}
				break;
			/* If, in a series of TEMPO object, there's more than one relevant to this
				part, we assume they're identical, e.g., for an orchestra score where
				the tempo appears above the top staff and somewhere in the middle, so
				we want only the first one. */
			case TEMPOtype:
				if (inTempoSeries) break;
				if (((PEXTEND)p)->staffn>=firstStf && ((PEXTEND)p)->staffn<=lastStf) {
					LinkSEL(pL) = TRUE;
					inTempoSeries = TRUE;
				}
				else {
					firstL = TempoFIRSTOBJ(pL);
					if (LinkBefFirstMeas(firstL) || MeasureTYPE(firstL)) {
						LinkSEL(pL) = TRUE;
						inTempoSeries = TRUE;
					}
				}
				break;

			case SLURtype:
				if (SlurSTAFF(pL)>=firstStf && SlurSTAFF(pL)<=lastStf) {
					LinkSEL(pL) = TRUE;
					aSlurL = FirstSubLINK(pL);
					for ( ; aSlurL; aSlurL=NextSLURL(aSlurL)) {
						aSlur = GetPASLUR(aSlurL);
						aSlur->selected = TRUE;
					}
				}
				break;
		}
	}
}


/* Delete the initial range (head to first invisible measure) of the new part document.
Duplicate the head of the score document and then remove all parts except the one
currently being extracted. Set the head of the new part document to this copied node and
fix up its cross links. */

static Boolean CopyPartInfo(Document *doc, Document *newDoc, LINK partL)
{
	LINK subL, tempL; PPARTINFO pSub, pPart; short firstStaff; Boolean okay=FALSE;

	/* This must be done before installing newDoc's heaps. */
	pPart = GetPPARTINFO(partL);
	firstStaff = pPart->firstStaff;

	InstallDoc(newDoc);
	DeleteRange(newDoc, RightLINK(newDoc->headL), newDoc->tailL);
	DeleteNode(newDoc,newDoc->headL);
	
	/* DuplicateObject automatically copies all partInfo subObjs */
	
	newDoc->headL = DuplicateObject(HEADERtype, doc->headL, FALSE, doc, newDoc, FALSE);
	if (!newDoc->headL) { NoMoreMemory(); goto Done; }

	/*
	 * Traverse the subList and remove all parts other than partL, starting with the
	 * part after the first unused part.
	 */
	subL = NextPARTINFOL(FirstSubLINK(newDoc->headL));
	while (subL) {
		pSub = GetPPARTINFO(subL);
		if (pSub->firstStaff!=firstStaff) {
			tempL = NextPARTINFOL(subL);
			RemoveLink(newDoc->headL, PARTINFOheap, FirstSubLINK(newDoc->headL), subL);
			HeapFree(PARTINFOheap,subL);
			subL = tempL;
		}
		else subL = NextPARTINFOL(subL);
	}
	
	LinkNENTRIES(newDoc->headL) = 2;	/* The first unused part + the one we kept */
	
	RightLINK(newDoc->headL) = newDoc->tailL;
	LeftLINK(newDoc->tailL) = newDoc->headL;
	LeftLINK(newDoc->headL) = NILINK;
	
	okay = TRUE;
	
Done:
	InstallDoc(doc);
	return okay;
}


/* Copy range from srcStartL to srcEndL into object list at insertL. All copying
is done from heaps in doc to heaps in doc; the nodes from the part system are all
allocated from the heaps for that document. */

static Boolean CopyPartRange(Document *doc, Document *newDoc, LINK srcStartL,
										LINK srcEndL, LINK insertL, LINK partL)
{
	LINK pL, prevL, copyL, initL; short i, numObjs; COPYMAP *partMap;
	Boolean okay=TRUE;

	if (!SetupCopyMap(srcStartL, srcEndL, &partMap, &numObjs)) {
		okay = FALSE; goto Done;
	}
	
	/*
	 * CopyPartInfo deletes the range in the new document from the head to the
	 *	tail, e.g., from the head to and including the first invisible measure.
	 *	 Then it copies the headL with the correct partInfo object.
	 */
	if (!CopyPartInfo(doc, newDoc, partL)) {
		okay = FALSE; goto Done;
	}
	initL = prevL = DLeftLINK(newDoc,insertL);

	for (i=0, pL=RightLINK(doc->headL); pL!=doc->tailL; i++, pL=RightLINK(pL)) {
		if (!LinkSEL(pL)) continue;

		copyL = DuplicateObject(ObjLType(pL), pL, TRUE, doc, newDoc, FALSE);
		if (!copyL) {
			NoMoreMemory();
			okay = FALSE; goto Done;
		}
		DRightLINK(newDoc,copyL) = insertL;
		DLeftLINK(newDoc,insertL) = copyL;
		DRightLINK(newDoc,prevL) = copyL;
		DLeftLINK(newDoc,copyL) = prevL;

		partMap[i].srcL = pL;	partMap[i].dstL = copyL;
		prevL = copyL;
  	}

	FixCrossLinks(doc, newDoc, newDoc->headL, insertL);
	CopyFixLinks(doc, newDoc, newDoc->headL, insertL, partMap, numObjs);

Done:
	DisposePtr((Ptr)partMap);
	return okay;
}


/* Given a score in which all objects and subobjects belonging to the part have
been selected, copy just those objects and subobjects into the part document. */
	
static Boolean CopyPart(Document *score, Document *part, LINK partL)
{
	return CopyPartRange(score, part, score->headL, score->tailL, part->tailL, partL);
}


/* Just deselect the entire document. Don't do any unhiliting or anything else. */

static void RPDeselAll(Document *doc)
{
	LINK pL;
	
	for (pL=doc->headL; pL!=doc->tailL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case HEADERtype:
				break;
			case PAGEtype:
			case SYSTEMtype:
				LinkSEL(pL) = FALSE;
				break;
			/*
			 * Be sure Connect subobjs are deselected, even if the object doesn't appear
			 * to be. We've had serious problems with anomalous selection of Connects;
			 * this should fix them with no bad side effects--though it'd be nice to
			 * avoid the anomaly in the first place!
			 */
         case CONNECTtype:
            DeselectNode(pL);
            break;
			default:
				if (LinkSEL(pL))
					DeselectNode(pL);
				break;
			}
		}
}


/* Read the part <partL> from the score document into the part document. */

static short ReadPart(Document *part, Document *score, LINK partL, Boolean *partOK)
{
	LINK firstMeasL; short staffDiff;
	
	/* For the entire score, select just those subObjs that we want to put into the
		part to be read, and then copy them. */
	
	InstallDoc(score);
	
	/* Avoid problems if, e.g., any Connect subobjs for other parts are selected. */
	
	RPDeselAll(score);
	SelectPart(score,partL, config.extractAllGraphics);
	*partOK = TRUE;

	/* If CopyPart fails, return with partOK FALSE. */
	
	if (!CopyPart(score,part,partL)) {
		*partOK = FALSE;
		return 0;
	}
	
	UpdateDocHeader(part);
	InstallDoc(score);
	staffDiff = ExtMapStaves(score,part);
	InstallDoc(part);
	InvalRange(part->headL, part->tailL);
	InstallDoc(score);
	RPDeselAll(score);
	
	firstMeasL = LSSearch(score->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	score->selStartL = score->selEndL = firstMeasL;

	InstallDoc(part);
	RPDeselAll(part);

	firstMeasL = LSSearch(part->headL, MEASUREtype, ANYONE, GO_RIGHT, FALSE);
	part->selStartL = part->selEndL = firstMeasL;
	
	return staffDiff;
}

	
/* Initialize fields in the part Document header, mostly by copying them from
the score Document. */

static void InitDocFields(register Document *score, register Document *part)
{
	short s;
	
	/*	fields in DOCUMENTHEADER */

	part->paperRect = score->paperRect;
	part->origPaperRect = score->origPaperRect;
	part->holdOrigin = score->holdOrigin;
	part->marginRect = score->marginRect;
	part->headerFooterMargins = score->headerFooterMargins;
	part->currentSheet = score->currentSheet;

	part->firstSheet = score->firstSheet;
	part->firstPageNumber = score->firstPageNumber;
	part->startPageNumber = score->startPageNumber;
	part->numRows = score->numRows;
	part->numCols = score->numCols;
	part->pageType = score->pageType;
	part->measSystem = score->measSystem;
	part->currentPaper = score->currentPaper;
	part->landscape = score->landscape;

	/* From NIGHTSCOREHEADER */

	part->feedback = score->feedback;
	part->polyTimbral = score->polyTimbral;
	part->spacePercent = score->spacePercent;	
	/* If score staff size is small, the size in the part probably should be larger,
	   but changing it involves much more than just changing one header field, and it's
	   easy for the user to change. */
	part->srastral = score->srastral;
	part->tempo = score->tempo;
	part->channel = score->channel;
	part->velocity = score->velocity;
	part->firstNames = score->firstNames;
	part->otherIndent = 0.0;
	part->otherNames = NONAMES;

	part->lastGlobalFont = score->lastGlobalFont;
	part->xMNOffset = score->xMNOffset;
	part->yMNOffset = score->yMNOffset;
	part->xSysMNOffset = score->xSysMNOffset;
	part->aboveMN = score->aboveMN;
	part->sysFirstMN = score->sysFirstMN;
	part->firstMNNumber = score->firstMNNumber;

	/* Copy font information & fontTable from score header. */

	part->nFontRecords = score->nFontRecords;
	BlockMove(score->fontNameMN, part->fontNameMN,
					(Size)score->nFontRecords*sizeof(TEXTSTYLE));

	BlockMove(score->fontTable, part->fontTable,
					(Size)MAX_SCOREFONTS*sizeof(FONTITEM));

	part->magnify = score->magnify;
	part->nfontsUsed = score->nfontsUsed;
	part->numberMeas = score->numberMeas;
	part->spaceTable = score->spaceTable;
	part->htight = score->htight;
	part->deflamTime = score->deflamTime;
	part->autoRespace = score->autoRespace;
	part->insertMode = score->insertMode;

	part->beamRests = score->beamRests;
	part->pianoroll = score->pianoroll;
	part->showSyncs = score->showSyncs;
	part->frameSystems = score->frameSystems;
	part->colorVoices = score->colorVoices;
	part->showInvis = score->showInvis;
	part->recordFlats = score->recordFlats;
	
	BlockMove(score->spaceMap, part->spaceMap, (Size)MAX_L_DUR*sizeof(long));

	part->firstIndent = score->firstIndent;
	part->yBetweenSys = score->yBetweenSys;

	for (s = 0; s<=MAXSTAVES; s++)
		part->staffSize[s] = score->staffSize[s];
}


/* Replace the range [firstL, lastL), which should contain no content objects other
than whole-measure rests separated by "barlines" (i.e., Measures), with a single
multibar rest on each staff. The existing rests in the range may appear in several
voices, simultaneously or sequentially: we simply generate the multibar rests for
the default voice on each staff in the first Sync.

NB: This leaves Measure cross-links in an inconsistent state! Also, if the range
contains one or more Systems, the Document header and both System cross-links and
objects with links to those Systems will need fixing. To facilitate fixing links
to Systems, we update <sysMap> to indicate the single System that replaces all
Systems in the range, if there are any. (As of v.997, the only objects that contain
links to Systems other than Systems are first pieces of cross-system slurs.) */
 
static Boolean MakeMultibarRest(Document *doc,
						LINK firstL,			/* Must be a Sync containing whole-measure rest(s) */
						LINK lastL,
						short nMeas,
						short staffDiff,		/* Staff and default voice no. offset */
						COPYMAP *sysMap,
						short nSys)
{
	PPARTINFO pPart; short nStaves, nChange, s, nSysDel, startSys, i;
	LINK subList, restL, measL, pL, firstSysDelL, newSysL;
	
	pPart = GetPPARTINFO(NextPARTINFOL(FirstSubLINK(doc->headL)));
	nStaves = pPart->lastStaff;
	nChange = nStaves-LinkNENTRIES(firstL);
	/*
	 * Enlarge or reduce the first object to <nStaves> subobjects, then turn each into
	 * a multibar rest on a different staff and in the default voice for that staff--
	 * or rather, the voice that, after offsetting by staffDiff (normally the number of
	 * staves above the part being extracted), will BECOME the default voice for that
	 * staff.
	 */
	if (!ExpandNode(firstL, &subList, nChange))
		{ NoMoreMemory(); return FALSE; }

	restL = FirstSubLINK(firstL);
	for (s = 1; restL; restL = NextNOTEL(restL), s++)
		SetupNote(doc, firstL, restL, s, 0, -nMeas, 0, s+staffDiff, TRUE, 0, 0);

	/* Update the <sysMap> for benefit of our caller. */
	
	firstSysDelL = NILINK; nSysDel = 0;
	for (pL = firstL; pL!= lastL; pL = RightLINK(pL))
		if (SystemTYPE(pL)) {
			if (!firstSysDelL) firstSysDelL = pL;
			nSysDel++;
		}
	if (nSysDel>0) {									/* If no System in range, nothing to do */
		newSysL = SSearch(firstL, SYSTEMtype, GO_LEFT);
		for (startSys = 0; startSys<nSys; startSys++)
			if (sysMap[startSys].srcL==firstSysDelL) break;
		for (i = startSys; i<startSys+nSysDel; i++)
			sysMap[i].dstL = newSysL;
	}

	/* Toss out the range except for the 1st obj, which now contains multibar rests. */
	
	measL = RightLINK(firstL);
	DeleteRange(doc, measL, lastL);
	if (nSysDel>0) {
		LINK nextSysL = SSearch(lastL, SYSTEMtype, GO_RIGHT);
		if (nextSysL==NILINK)
			nextSysL = doc->tailL;
		FixStructureLinks(doc, doc, lastL, nextSysL);
	}
	return TRUE;
}

/* Set up the copyMap array for use in updating links to cross-system slurs. */

#define EXTRAOBJS	4

void SetupSysCopyMap(Document *doc, COPYMAP **sysMap, short *objCount)
{
	LINK sysL;
	short i, numObjs=0;

	sysL = SSearch(doc->headL, SYSTEMtype, GO_RIGHT);
	for ( ; sysL; sysL = LinkRSYS(sysL))
		numObjs++;						
	*sysMap = (COPYMAP *)NewPtr((Size)(numObjs+EXTRAOBJS)*sizeof(COPYMAP));

	if (GoodNewPtr((Ptr)*sysMap)) {
		sysL = SSearch(doc->headL, SYSTEMtype, GO_RIGHT);
		for (i = 0; sysL; i++, sysL = LinkRSYS(sysL))
			(*sysMap)[i].srcL = (*sysMap)[i].dstL = sysL;
	}
	else {
		numObjs = 0;
		OutOfMemory((long)(numObjs+EXTRAOBJS)*sizeof(COPYMAP));
	}

	*objCount = numObjs;					/* Return number of objects in range. */
}

/* Update according to the given COPYMAP lastSyncs that actually refer to Systems in
first pieces of cross-system slurs. */

static void ExtFixCrossSysSlurs(Document *doc, COPYMAP *sysMap, short nSys)
{
	LINK pL; short i;
	
	for (pL = doc->headL; pL; pL = RightLINK(pL))
		if (SlurTYPE(pL))
			if (SlurCrossSYS(pL)) {
				for (i = 0; i<nSys; i++)
					if (SlurLASTSYNC(pL)==sysMap[i].srcL)
						 SlurLASTSYNC(pL) = sysMap[i].dstL;
			}
}

/* Look for any series of whole-measure rests separated only by Measures and
replace each with a single multibar rest on each staff. If staff and voice
nos. have not yet been mapped for the new (part) context, set staffDiff to
the staff no. offset and <setVoiceRoles> TRUE (in which case we change the
<voiceRoles>s for the mapped voices to those of the actual voices and do NOT
restore them: this should be OK since they may contain garbage anyway). */
 
static Boolean MultibarRests(Document *doc,
					short staffDiff,						/* Staff and default voice no. offset */
					Boolean setVoiceRoles
					)
{
	register LINK pL; LINK aMeasL, aNoteL, firstL, lastSyncL;
	short nMeasInSeries=0, nSys;
	Boolean allWMR, possibleMBR=TRUE, measStart, okay=FALSE;
	PPARTINFO pPart; short nStaves, s;	COPYMAP *sysMap;
	
	SetupSysCopyMap(doc, &sysMap, &nSys);
	if (!sysMap) goto Done;
	
	if (setVoiceRoles) {
		pPart = GetPPARTINFO(NextPARTINFOL(FirstSubLINK(doc->headL)));
		nStaves = pPart->lastStaff;
		for (s = 1; s<=nStaves; s++) {
			doc->voiceTab[s+staffDiff].voiceRole = doc->voiceTab[s].voiceRole;
		}
	}	

	for (pL = doc->headL; pL; pL = RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case MEASUREtype:
				/*
				 * A Measure (i.e., barline) interrupts a multibar rest if it's not a
				 * single barline or if the preceding Measure is empty.
				 */
				aMeasL = FirstSubLINK(pL);
				if (MeasSUBTYPE(aMeasL)!=BAR_SINGLE || MeasureTYPE(LeftLINK(pL))) {
					if (nMeasInSeries>1)
						if (!MakeMultibarRest(doc, firstL, RightLINK(lastSyncL),
								nMeasInSeries, staffDiff, sysMap, nSys))
							goto Done;
					nMeasInSeries = 0;
					possibleMBR = TRUE;
					break;
				}
				measStart = possibleMBR = TRUE;
				break;
			case SYNCtype:
				allWMR = TRUE;
				for (aNoteL = FirstSubLINK(pL); aNoteL; aNoteL = NextNOTEL(aNoteL))
					if (NoteType(aNoteL)!=WHOLEMR_L_DUR) {
						if (nMeasInSeries>1)
							if (!MakeMultibarRest(doc, firstL, RightLINK(lastSyncL),
									nMeasInSeries, staffDiff, sysMap, nSys))
								goto Done;
						nMeasInSeries = 0;
						allWMR = FALSE;
					}
				if (allWMR && possibleMBR) {
					nMeasInSeries++;
					if (nMeasInSeries>127) {
						GetIndCString(strBuf, MISCERRS_STRS, 1);	/* "can't make multibar rest of > 127..." */
						CParamText(strBuf, "", "", "");
						StopInform(GENERIC_ALRT);
						if (!MakeMultibarRest(doc, firstL, RightLINK(lastSyncL),
														127, staffDiff, sysMap, nSys))
							goto Done;
						nMeasInSeries = 1;
						possibleMBR = TRUE;
					}
					
					if (nMeasInSeries==1) firstL = pL;
				}
				lastSyncL = pL;
				measStart = FALSE;
				break;
			case PAGEtype:
			case SYSTEMtype:
			case STAFFtype:
			case CONNECTtype:
				break;
			/*
			 *	Clefs, key signatures, and time signatures that aren't inMeas have
			 * no effect on multibar rests, since they must be in the reserved area.
			 *	Clefs, key signatures, and time signatures beginning the first measure
			 * of a possible multibar rest don't prevent it, but they do if they're
			 * they're in the middle or at the end of the measure.
			 */
			case CLEFtype:
				if (ClefINMEAS(pL)) {
					if (nMeasInSeries>1)
						if (!MakeMultibarRest(doc, firstL, RightLINK(lastSyncL),
								nMeasInSeries, staffDiff, sysMap, nSys))
							goto Done;
					nMeasInSeries = 0;
					if (!measStart) possibleMBR = FALSE;
				}
				break;
			case KEYSIGtype:
				if (KeySigINMEAS(pL)) {
					if (nMeasInSeries>1)
						if (!MakeMultibarRest(doc, firstL, RightLINK(lastSyncL),
								nMeasInSeries, staffDiff, sysMap, nSys))
							goto Done;
					nMeasInSeries = 0;
					if (!measStart) possibleMBR = FALSE;
				}
				break;
			case TIMESIGtype:
				if (TimeSigINMEAS(pL)) {
					if (nMeasInSeries>1)
						if (!MakeMultibarRest(doc, firstL, RightLINK(lastSyncL),
								nMeasInSeries, staffDiff, sysMap, nSys))
							goto Done;
					nMeasInSeries = 0;
					if (!measStart) possibleMBR = FALSE;
				}
				break;
			default:
				if (nMeasInSeries>1)
					if (!MakeMultibarRest(doc, firstL, RightLINK(lastSyncL),
							nMeasInSeries, staffDiff, sysMap, nSys))
						goto Done;
				nMeasInSeries = 0;
				possibleMBR = FALSE;
				measStart = FALSE;
		}
	}
	
	okay = TRUE;
	
Done:
	/*  FIXME: Is it okay to call FixStructureLinks if arrived from sysMap==NULL failure? */
	FixStructureLinks(doc, doc, doc->headL, doc->tailL);
	if (sysMap) {
		ExtFixCrossSysSlurs(doc, sysMap, nSys);
		DisposePtr((Ptr)sysMap);
	}
	
	return okay;
}


/* Correct structure of Measure and PseudoMeasure subobjects to reflect the staves
they actually connect to in the part. */

static void FixMeasures(Document *doc)
{
	LINK pL, aMeasureL, aPSMeasL, aRptEndL;
	
	for (pL = doc->headL; pL; pL = RightLINK(pL))
		switch (ObjLType(pL)) {
			case MEASUREtype:
				aMeasureL = FirstSubLINK(pL);
				for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
					if (MeasureSTAFF(aMeasureL)<=1) {
						MeasCONNABOVE(aMeasureL) = FALSE;
						MeasCONNSTAFF(aMeasureL) = (doc->nstaves>1? doc->nstaves : 0);
						}
					else {
						MeasCONNABOVE(aMeasureL) = TRUE;
						MeasCONNSTAFF(aMeasureL) = 0;
						}
				}
				break;
			case PSMEAStype:
				aPSMeasL = FirstSubLINK(pL);
				for ( ; aPSMeasL; aPSMeasL=NextPSMEASL(aPSMeasL)) {
					if (PSMeasSTAFF(aPSMeasL)<=1) {
						PSMeasCONNABOVE(aPSMeasL) = FALSE;
						PSMeasCONNSTAFF(aPSMeasL) = (doc->nstaves>1? doc->nstaves : 0);
						}
					else {
						PSMeasCONNABOVE(aPSMeasL) = TRUE;
						PSMeasCONNSTAFF(aPSMeasL) = 0;
						}
				}
				break;
			case RPTENDtype:
				aRptEndL = FirstSubLINK(pL);
				for ( ; aRptEndL; aRptEndL=NextRPTENDL(aRptEndL)) {
					if (RptEndSTAFF(aRptEndL)<=1) {
						RptEndCONNABOVE(aRptEndL) = FALSE;
						RptEndCONNSTAFF(aRptEndL) = (doc->nstaves>1? doc->nstaves : 0);
						}
					else {
						RptEndCONNABOVE(aRptEndL) = TRUE;
						RptEndCONNSTAFF(aRptEndL) = 0;
						}
				}
				break;
			default:
				;
		}
}

/* Update voice numbers of objects in the Document so they start at 1 and there are
no gaps, and create a voice-mapping table that reflects the new situation. Does not
assume the existing voice-mapping table is accurate. */
 
static void ExtCompactVoiceNums(Document *doc, short staffDiff)
{
	short newVoiceTab[MAXVOICES+1], v, vNew;
	
	/*
	 *	First, offset voice numbers to reduce the lowest to 1. Then create the
	 *	standard (internal-to-user) voice-mapping table and use it to construct
	 *	a special table that maps existing to desired voice numbers, closing up
	 *	any gaps. If we do close any gaps, that will make the standard voice-mapping
	 *	table wrong, so we update it again at the end of the end of this function.
	 */
	OffsetVoiceNums(doc, 1, -staffDiff);
	BuildVoiceTable(doc, TRUE);
	for (vNew = v = 1; v<=MAXVOICES; v++)
		newVoiceTab[v] = (doc->voiceTab[v].partn>0? vNew++ : 0);
			
	/* Use the special table to change voice numbers throughout the object list. */
	MapVoiceNums(doc, newVoiceTab);
	
	/* Could call UpdateVoiceTable again but this should be faster & maybe more robust. */
	for (v = 1; v<=MAXVOICES; v++)
		if (newVoiceTab[v]>0 && newVoiceTab[v]!=v) {
			doc->voiceTab[newVoiceTab[v]] = doc->voiceTab[v];
			doc->voiceTab[v].partn = 0;							/* Mark the old slot as empty */
		}
}


/* Correct positions of part brackets in Connect subobjects, since grouping in the
score might have positioned them further to the left than the part needs. */

static void FixConnects(Document *doc)
{
	LINK pL, aConnectL; PACONNECT aConnect;
	
	for (pL = doc->headL; pL; pL = RightLINK(pL))
		if (ConnectTYPE(pL)) {
			aConnectL = FirstSubLINK(pL);
			for ( ; aConnectL; aConnectL=NextCONNECTL(aConnectL)) {
				/* Assume the PartLevel subobj is the only one besides the SystemLevel. */
				aConnect = GetPACONNECT(aConnectL);					
				if (aConnect->connLevel==PartLevel)
					aConnect->xd = -ConnectDWidth(doc->srastral, CONNECTCURLY);
			}
		}
}


/* Given a Document, create a new Document whose contents are taken from the given
part of the original.  Deliver the new Document, or NULL if we can't do it. */

Document *CreatePartDoc(Document *doc, unsigned char *fileName, short vRefNum, FSSpec *pfsSpec, LINK partL)
{
	register Document *newDoc; WindowPtr w; long fileVersion;
	short staffDiff, palWidth, palHeight; Rect box,bounds; 
	Boolean partOK;

	newDoc = FirstFreeDocument();
	if (newDoc == NULL) { TooManyDocs(); return(NULL); }
	
	w = GetNewWindow(docWindowID,NULL,BottomPalette);
	if (!w) return(NULL);
	
	newDoc->theWindow = w;
	SetDocumentKind(w);
	//((WindowPeek)w)->spareFlag = TRUE;
	ChangeWindowAttributes(w, kWindowFullZoomAttribute, kWindowNoAttributes);
	
	newDoc->inUse = TRUE;
	Pstrcpy(newDoc->name,fileName);
	newDoc->vrefnum = vRefNum;
	newDoc->fsSpec = *pfsSpec;

	/* Create <newDoc>, the basic part Document, and add the part's data to it. */
	
	if (!BuildDocument(newDoc, fileName, vRefNum, pfsSpec, &fileVersion, TRUE)) {
		DoCloseDocument(newDoc);
		return(NULL);
		}
	
	/* If the creation of newDoc's main data structure (object list) failed, close
		the new document and return. */

	staffDiff = ReadPart(newDoc, doc, partL, &partOK);
	if (!partOK) {
		DoCloseDocument(newDoc);
		return(NULL);
		}

	/* We now have a Document with the part data in it, but it needs a lot of fixing up. */

	InitDocFields(doc,newDoc);
	MultibarRests(newDoc, staffDiff, TRUE);
	UpdateDocHeader(newDoc);						/* Since MultibarRests can remove systems and pages */
	FixMeasures(newDoc);
	UpdateMeasNums(newDoc, NILINK);
	FixConnects(newDoc);
	ExtCompactVoiceNums(newDoc, staffDiff);
	
	/* Replace the Master Page: delete the old Master Page object list here and
		replace it with a new structure to reflect the part-staff structure of the
		extracted part. */

	Score2MasterPage(newDoc);
	newDoc->docNew = newDoc->changed = TRUE;		/* Has to be set after BuildDocument */
	newDoc->readOnly = FALSE;
	
	/* Place new document window in a non-conflicting position */

	WindowPtr palPtr = (*paletteGlobals[TOOL_PALETTE])->paletteWindow;
	GetWindowPortBounds(palPtr,&box);
	palWidth = box.right - box.left;
	palHeight = box.bottom - box.top;
	GetGlobalPort(w,&box);							/* set bottom of window near screen bottom */
	bounds = GetQDScreenBitsBounds();
	if (box.left < bounds.left+4) box.left = bounds.left+4;
	if (palWidth < palHeight) box.left += palWidth;
	MoveWindow(newDoc->theWindow, box.left, box.top, FALSE);
	AdjustWinPosition(w);
	GetGlobalPort(w, &box);
	bounds = GetQDScreenBitsBounds();
	box.bottom = bounds.bottom - 4;
	if (box.right > bounds.right-4) box.right = bounds.right - 4;
	SizeWindow(newDoc->theWindow, box.right-box.left, box.bottom-box.top, FALSE);

	SetOrigin(newDoc->origin.h, newDoc->origin.v);
	GetAllSheets(newDoc);
	RecomputeView(newDoc);
	SetControlValue(newDoc->hScroll, newDoc->origin.h);
	SetControlValue(newDoc->vScroll, newDoc->origin.v);
	SetWTitle(w, newDoc->name);
	
	MEAdjustCaret(newDoc, FALSE);
	InstallMagnify(newDoc);
	ShowDocument(newDoc);

	return(newDoc);
}
