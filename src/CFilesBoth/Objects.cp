/******************************************************************************************
	FILE:	Objects.c
	PROJ:	Nightingale
	DESC:	Object-level routines, mostly for specific object types. Most
			generic object-handling routines are in Copy.c, CrossLinks.c, and
			DSUtils.c.
		
		CopyModNRList			DuplicateObject			DuplicNC
		InitObject				SetObject				GrowObject
		SimplePlayDur			CalcPlayDur
		IsRestPosLower			SetupNote				SetupGRNote
		InitPart				InitStaff				InitClef
		InitKeySig				AddKSItem				SetupKeySig
		InitTimeSig				InitMeasure				InitPSMeasure
		InitRptEnd				InitDynamic				SetupHairpin
		InitGraphic				SetMeasVisible			ChordHasUnison
		ChordNoteToRight		ChordNoteToLeft			FixTieIndices
		NormalStemUpDown		GetNCYStem				FixChordForYStem
		FixSyncForChord			FixSyncForNoChord		FixNoteForClef
		GetGRCYStem				FixGRChordForYStem		FixGRSyncForChord
		FixGRSyncNote			FixGRSyncForNoChord		FixAugDotPos
		ToggleAugDotPos
/******************************************************************************************/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

/* Prototypes for local functions */

static Boolean IsRestPosLower(Document *, short, short);
static void AddKSItem(LINK, Boolean, short, short);
void SetMeasVisible(LINK, Boolean);
static short NormalStemUpDown(Document *, LINK, short, PCONTEXT);

/* Given the first item in a note's modNR list, create a duplicate list and return its
first item. This expands the ModNR heap one item at a time; it would be somewhat better
to ask for memory for all the ModNRs at once, but even among the notes that have ModNRs
at all, few will have more than two, so no big deal.  */
 
LINK CopyModNRList(Document *srcDoc, Document *dstDoc, LINK firstModNRL)
{
	LINK aModNRL, newModNRList, prevModNRL, newModNRL;
	PAMODNR pModNR, pNewModNR;
	HEAP *srcHeap, *dstHeap;
	
	newModNRList = NILINK;
	srcHeap = srcDoc->Heap+MODNRtype;
	dstHeap = dstDoc->Heap+MODNRtype;
	for (aModNRL=firstModNRL; aModNRL; aModNRL=DNextMODNRL(srcDoc,aModNRL)) {
		newModNRL = HeapAlloc(dstHeap, 1);
		if (!newModNRL) MayErrMsg("CopyModNRList: HeapAlloc failed.");
		if (!newModNRList)	newModNRList = newModNRL;
		else				DNextMODNRL(dstDoc,prevModNRL) = newModNRL;
		pModNR = (PAMODNR)LinkToPtr(srcHeap,aModNRL);
		pNewModNR = (PAMODNR)LinkToPtr(dstHeap,newModNRL);
		BlockMove(pModNR, pNewModNR, sizeof(AMODNR));
		prevModNRL = newModNRL;
	}
	return newModNRList;
}


/* ------------------------------------------------------------------- DuplicateObject -- */
/* Given a LINK to an object of a given type, allocate another duplicate object from the
Object Heap, and duplicate the subobject list as well.  If selectedOnly is True, then
only duplicate those subobjects that are selected in the original; if no subobjects are
selected, don't duplicate anything and return NILINK. Exception: if the object is of
HEADERtype, all subobjects are always duplicated. If keepGraphics and object is a
Graphic, its string is not duplicated: rather, the new object has the same stringOffset
as the original. ??NB: If the object is not selected and is of a type that sets
subsNeeded=False, it's always duplicated, regardless of <selectedOnly>! This is probably
a bug: these types should be handled the way GRAPHICs and TEMPOs are. ??NB2: For an
"extended object", it appears that <selectedOnly> looks at, not the object's subobjects'
selection status, but that of the Syncs it refers to! Maybe this should be changed, but
it wouldn't be easy.

Returns the LINK to the duplicate object, or NILINK if a problem (probably out of
memory). */

LINK DuplicateObject(short type, LINK objL, Boolean selectedOnly, Document *srcDoc,
							Document *dstDoc, Boolean keepGraphics)
{
	HEAP *myHeap, *srcHeap, *dstHeap;
	short subcount = 0;
	LINK subL, newSubL;
	LINK newObjL, tmpL, firstSubObj;
	GenSubObj *pSub, *pNewSub;
	PMEVENT pObj, pNewObj;
	Boolean subsNeeded = True;
	
#ifdef DODEBUG
LogPrintf(LOG_DEBUG, "DuplicateObject:\n\tobjL=%d type=%d\n", objL, type);
#endif

	if (objL==NILINK) {
		MayErrMsg("DuplicateObject: objL=NILINK.");
		return NILINK;
	}
		
	/* Get length of subobject list we want to duplicate */
	
	switch (DObjLType(srcDoc,objL)) {
		case HEADERtype: {
			LINK aPartL;
			
			aPartL = DFirstSubLINK(srcDoc,objL);
			for ( ; aPartL; aPartL = DNextPARTINFOL(srcDoc,aPartL))
				subcount++;
			break;
		}
		case PAGEtype:
		case SYSTEMtype:
		case TAILtype:
			subsNeeded = False;
			break;

		case STAFFtype: {
			LINK aStaffL;
			PASTAFF aStaff;
			
			aStaffL = DFirstSubLINK(srcDoc,objL);
			for ( ; aStaffL; aStaffL = DNextSTAFFL(srcDoc,aStaffL)) {
				aStaff = DGetPASTAFF(srcDoc,aStaffL);
				if (aStaff->selected || !selectedOnly) subcount++;
			}
			break;
		}
		case CONNECTtype:  {
			LINK aConnectL;
			PACONNECT aConnect;
			
			aConnectL = DFirstSubLINK(srcDoc,objL);
			for ( ; aConnectL; aConnectL = DNextCONNECTL(srcDoc,aConnectL)) {
				aConnect = DGetPACONNECT(srcDoc,aConnectL);
				if (aConnect->selected || !selectedOnly) subcount++;
			}
			break;
		}
		case MEASUREtype:
		case PSMEAStype:
		case SYNCtype:
		case GRSYNCtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case DYNAMtype:
		case RPTENDtype:
			myHeap = srcDoc->Heap+type;
			for (subL=DFirstSubLINK(srcDoc,objL); subL; subL=NextLink(myHeap,subL)) {
				pSub = (GenSubObj *)LinkToPtr(myHeap,subL);
				if (pSub->selected || !selectedOnly) subcount++;
				}
			break;
		case SLURtype: {
			LINK aSlurL;
			
			aSlurL = DFirstSubLINK(srcDoc,objL);
			for ( ; aSlurL; aSlurL = DNextSLURL(srcDoc,aSlurL))
				if (DSlurSEL(srcDoc,aSlurL) || !selectedOnly) subcount++;
			break;
		}
		case BEAMSETtype: {
			LINK aNoteBeamL, bpSyncL, aNoteL, aGRNoteL;
			PANOTEBEAM aNoteBeam;
			
			aNoteBeamL = DFirstSubLINK(srcDoc,objL);
			for ( ; aNoteBeamL; aNoteBeamL=DNextNOTEBEAML(srcDoc,aNoteBeamL)) {
				aNoteBeam = DGetPANOTEBEAM(srcDoc,aNoteBeamL);
				bpSyncL = aNoteBeam->bpSync;
				if (DGraceBEAM(srcDoc,objL)) {
					aGRNoteL = DFirstSubLINK(srcDoc,bpSyncL);
					for ( ; aGRNoteL; aGRNoteL = DNextGRNOTEL(srcDoc,aGRNoteL))
						if (DGRNoteVOICE(srcDoc,aGRNoteL)==DBeamVOICE(srcDoc,objL)) {
							if (DGRNoteSEL(srcDoc,aGRNoteL) || !selectedOnly) 
								{ subcount++; break; }
						}
				}
				else {
					aNoteL = DFirstSubLINK(srcDoc,bpSyncL);
					for ( ; aNoteL; aNoteL = DNextNOTEL(srcDoc,aNoteL))
						if (DNoteVOICE(srcDoc,aNoteL)==DBeamVOICE(srcDoc,objL)) {
							if (DNoteSEL(srcDoc,aNoteL) || !selectedOnly) 
								{ subcount++; break; }
						}
				}
			}
			break;
		}
		case GRAPHICtype:
			if (DLinkSEL(srcDoc,objL) || !selectedOnly)
				subcount++;
			break;
		case TEMPOtype:
			if (DLinkSEL(srcDoc,objL) || !selectedOnly)
				subcount++;
			break;
		case SPACERtype:
		case ENDINGtype:
			subsNeeded = False;
			break;
		case OTTAVAtype: {
			LINK aNoteOctL, opSyncL, aNoteL;
			PANOTEOTTAVA aNoteOct;
			
			aNoteOctL = DFirstSubLINK(srcDoc,objL);
			for ( ; aNoteOctL; aNoteOctL=DNextNOTEOTTAVAL(srcDoc,aNoteOctL)) {
				aNoteOct = DGetPANOTEOTTAVA(srcDoc,aNoteOctL);
				opSyncL = aNoteOct->opSync;
				aNoteL = DFirstSubLINK(srcDoc,opSyncL);
				for ( ; aNoteL; aNoteL = DNextNOTEL(srcDoc,aNoteL))
					if (DNoteSTAFF(srcDoc,aNoteL)==DOttavaSTAFF(srcDoc,objL)) {
						if (DNoteSEL(srcDoc,aNoteL) || !selectedOnly) 
							{ subcount++; break; }
					}
			}
			break;
		}
		case TUPLETtype: {
			LINK aNoteTupleL, tpSyncL, aNoteL;
			PANOTETUPLE aNoteTuple;
			
			aNoteTupleL = DFirstSubLINK(srcDoc,objL);
			for ( ; aNoteTupleL; aNoteTupleL=DNextNOTETUPLEL(srcDoc,aNoteTupleL)) {
				aNoteTuple = DGetPANOTETUPLE(srcDoc,aNoteTupleL);
				tpSyncL = aNoteTuple->tpSync;
				aNoteL = DFirstSubLINK(srcDoc,tpSyncL);
				for ( ; aNoteL; aNoteL = DNextNOTEL(srcDoc,aNoteL))
					if (DNoteVOICE(srcDoc,aNoteL)==DTupletVOICE(srcDoc,objL)) {
						if (DNoteSEL(srcDoc,aNoteL) || !selectedOnly)
							{ subcount++; break; }
					}
			}
			break;
		}
		default:
			if (TYPE_BAD(objL)) {
				MayErrMsg("DuplicateObject: object at %ld has illegal type %ld",
							(long)objL, (long)ObjLType(objL));
				return NILINK;
			}
			break;
	}
	
#ifdef DODEBUG
if (type==TEMPOtype) {
LogPrintf(LOG_DEBUG, "\tSub-object list is %d long\n", subcount);
LogPrintf(LOG_DEBUG, "\tsubsNeeded=%d\n", subsNeeded);
}
#endif
	if (subcount<=0 && subsNeeded) return NILINK;

	/* Allocate new object with list of (empty) subobjects */
	if (DObjLType(srcDoc,objL) == TEMPOtype)
		newObjL = NewNode(dstDoc, type, 0);
	else
		newObjL = NewNode(dstDoc, type, subcount);

	if (newObjL == NILINK) return NILINK;
	
	/* Copy obj's information into newObj's information */
	
	pObj    = (PMEVENT)LinkToPtr(srcDoc->Heap+OBJtype,objL);
	pNewObj = (PMEVENT)LinkToPtr(dstDoc->Heap+OBJtype,newObjL);
	firstSubObj = DFirstSubLINK(dstDoc,newObjL);				/* Save it before it gets wiped out */
	BlockMove(pObj, pNewObj, sizeof(SUPEROBJECT));
	
	/* However, the subcount may be smaller if we only counted selected subobjects */
	
	pNewObj->nEntries = subcount;
	pNewObj->firstSubObj = firstSubObj;
	
	/* And we don't want to copy the source's link information */
	
	pNewObj->right = pNewObj->left = NILINK;
	if (!subsNeeded) {
		pNewObj->firstSubObj = NILINK;
		return (newObjL);
	}
	
	/* Loop through the subobject lists, copying the internal information */

	subL = DFirstSubLINK(srcDoc,objL);
	newSubL = DFirstSubLINK(dstDoc,newObjL);
	switch (DObjLType(srcDoc,objL)) {
		case HEADERtype: {
			PPARTINFO aPart, newPart;
			
			while (subL) {
				aPart = DGetPPARTINFO(srcDoc,subL);
				newPart = DGetPPARTINFO(dstDoc,newSubL);
				tmpL = DNextPARTINFOL(dstDoc,newSubL);		/* Save it before it gets wiped out */
				BlockMove(aPart, newPart, (long)subObjLength[type]);
				newPart->next = newSubL = tmpL;				/* Restore and move on */
				subL = DNextPARTINFOL(srcDoc,subL);
			}
			break;
		}

		case PAGEtype:
		case SYSTEMtype:
		case TAILtype:
			break;

		case STAFFtype: {
			PASTAFF aStaff, newStaff;
			
			while (subL) {
				aStaff = DGetPASTAFF(srcDoc,subL);
				newStaff = DGetPASTAFF(dstDoc,newSubL);
				if (DStaffSEL(srcDoc,subL) || !selectedOnly) {
					tmpL = DNextSTAFFL(dstDoc,newSubL);			/* Save it before it gets wiped out */
					BlockMove(aStaff, newStaff, (long)subObjLength[type]);
					newStaff->next = newSubL = tmpL;			/* Restore and move on */
				}
				subL = DNextSTAFFL(srcDoc,subL);
			}
			break;
		}
		case CONNECTtype: {
			PACONNECT aConnect, newConnect;
			
			while (subL) {
				aConnect = DGetPACONNECT(srcDoc,subL);
				newConnect = DGetPACONNECT(dstDoc,newSubL);
				if (DConnectSEL(srcDoc,subL) || !selectedOnly) {
					tmpL = DNextCONNECTL(dstDoc,newSubL);		/* Save it before it gets wiped out */
					BlockMove(aConnect, newConnect, (long)subObjLength[type]);
					newConnect->next = newSubL = tmpL;			/* Restore and move on */
				}
				subL = DNextCONNECTL(srcDoc,subL);
			}
			break;
		}

		case SYNCtype: {
			PANOTE aNote, newNote;
			
			/* Note that CopyModNRList can move memory, so be careful with pointers here.  -JGG */
			
			while (subL) {
				aNote = DGetPANOTE(srcDoc,subL);
				newNote = DGetPANOTE(dstDoc,newSubL);
				if (DNoteSEL(srcDoc,subL) || !selectedOnly) {
					tmpL = DNextNOTEL(dstDoc,newSubL);			/* Save it before it gets wiped out */
					BlockMove(aNote, newNote, (long)subObjLength[type]);
					newNote->next = tmpL;
					if (newNote->firstMod) {
						LINK newMod = CopyModNRList(srcDoc,dstDoc,newNote->firstMod);
						newNote = DGetPANOTE(dstDoc,newSubL);  /* Must get pointer again. */
						newNote->firstMod = newMod;
					}
					newSubL = tmpL;
				}
				subL = DNextNOTEL(srcDoc,subL);
			}
			break;
		}
		
		case MEASUREtype:
		case PSMEAStype:
		case GRSYNCtype:
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case DYNAMtype:
		case RPTENDtype:
			srcHeap = srcDoc->Heap+type;
			dstHeap = dstDoc->Heap+type;
			while (subL) {
				pSub    = (GenSubObj *)LinkToPtr(srcHeap,subL);
				pNewSub = (GenSubObj *)LinkToPtr(dstHeap,newSubL);
				if (pSub->selected || !selectedOnly) {
					tmpL = NextLink(dstHeap,newSubL);			/* Save it before it gets wiped out */
					BlockMove(pSub, pNewSub, (long)subObjLength[type]);
					pNewSub->next = newSubL = tmpL;				/* Restore and move on */
				}
				subL = NextLink(srcHeap,subL);
			}
			break;

		case SLURtype: {
			PASLUR aSlur, newSlur;
			
			while (subL) {
				aSlur = DGetPASLUR(srcDoc,subL);
				newSlur = DGetPASLUR(dstDoc,newSubL);
				if (DSlurSEL(srcDoc,subL) || !selectedOnly) {
					tmpL = DNextSLURL(dstDoc,newSubL);			/* Save it before it gets wiped out */
					BlockMove(aSlur, newSlur, (long)subObjLength[type]);
					newSlur->next = newSubL = tmpL;				/* Restore and move on */
				}
				subL = DNextSLURL(srcDoc,subL);
			}
			break;
		}

		case BEAMSETtype: {
			LINK bpSyncL, aNoteL, aGRNoteL;
			PANOTEBEAM aNoteBeam, newNoteBeam;

			while (subL) {
				aNoteBeam = DGetPANOTEBEAM(srcDoc,subL);
				newNoteBeam = DGetPANOTEBEAM(dstDoc,newSubL);

				bpSyncL = aNoteBeam->bpSync;
				if (DGraceBEAM(srcDoc,objL)) {
					aGRNoteL = DFirstSubLINK(srcDoc,bpSyncL);
					for ( ; aGRNoteL; aGRNoteL = DNextGRNOTEL(srcDoc,aGRNoteL))
						if (DGRNoteVOICE(srcDoc,aGRNoteL)==DBeamVOICE(srcDoc,objL)) {
							if (DGRNoteSEL(srcDoc,aGRNoteL) || !selectedOnly) {
								tmpL = DNextNOTEBEAML(dstDoc,newSubL);		/* Save it before it gets wiped out */
								BlockMove(aNoteBeam, newNoteBeam, (long)subObjLength[type]);
								newNoteBeam->next = newSubL = tmpL;			/* Restore and move on */
								break;
							}
						}
					subL = DNextNOTEBEAML(srcDoc,subL);
				}
				else {
					aNoteL = DFirstSubLINK(srcDoc,bpSyncL);
					for ( ; aNoteL; aNoteL = DNextNOTEL(srcDoc,aNoteL))
						if (DNoteVOICE(srcDoc,aNoteL)==DBeamVOICE(srcDoc,objL)) {
							if (DNoteSEL(srcDoc,aNoteL) || !selectedOnly) {
								tmpL = DNextNOTEBEAML(dstDoc,newSubL);		/* Save it before it gets wiped out */
								BlockMove(aNoteBeam, newNoteBeam, (long)subObjLength[type]);
								newNoteBeam->next = newSubL = tmpL;			/* Restore and move on */
								break;
							}
						}
					subL = DNextNOTEBEAML(srcDoc,subL);
				}
			}
			break;
		}

		case GRAPHICtype: {
			PAGRAPHIC aGraphic, newGraphic;
			StringPoolRef currentPool;
			unsigned char *graphicString;
			STRINGOFFSET stringOff;
			
			if (!keepGraphics) {
				currentPool = GetStringPool();			/* Pull in String Manager seg before getting ptrs */

				aGraphic = DGetPAGRAPHIC(srcDoc,subL);
				newGraphic = DGetPAGRAPHIC(dstDoc,newSubL);
				BlockMove(aGraphic, newGraphic, (long)subObjLength[type]);
				
				InstallStrPool(srcDoc);
				graphicString = PCopy(aGraphic->strOffset);
				InstallStrPool(dstDoc);
				stringOff = PStore(graphicString);
				if (stringOff<0L) return NILINK;
				newGraphic = DGetPAGRAPHIC(dstDoc,newSubL);
				newGraphic->strOffset = stringOff;
				
				SetStringPool(currentPool);
			}
			else {
				aGraphic = DGetPAGRAPHIC(srcDoc,subL);
				newGraphic = DGetPAGRAPHIC(dstDoc,newSubL);
				BlockMove(aGraphic, newGraphic, (long)subObjLength[type]);
			}
			break;
		}
		
		case TEMPOtype: {
			PTEMPO pTempo, newTempo;
			StringPoolRef currentPool; unsigned char *tempoString,*metroStr;
			
PushLock(srcDoc->Heap+OBJtype);
PushLock(dstDoc->Heap+OBJtype);
			pNewObj->nEntries = 0;

			currentPool = GetStringPool();			/* Pull in String Manager seg before getting ptrs */

			pTempo = (PTEMPO)pObj;
			newTempo = (PTEMPO)pNewObj;
			
			InstallStrPool(srcDoc);
			tempoString = PCopy(pTempo->strOffset);
			metroStr = PCopy(pTempo->metroStrOffset);
			InstallStrPool(dstDoc);
			newTempo->strOffset = PStore(tempoString);
			newTempo->metroStrOffset = PStore(metroStr);
			if (newTempo->strOffset<0L || newTempo->metroStrOffset<0L) return NILINK;
			
			SetStringPool(currentPool);
PopLock(srcDoc->Heap+OBJtype);
PopLock(dstDoc->Heap+OBJtype);
			break;
		}
		
		case SPACERtype:
		case ENDINGtype:
			break;

		case OTTAVAtype: {
			LINK opSyncL, aNoteL;
			PANOTEOTTAVA aNoteOct, newNoteOct;
			
			while (subL) {
				aNoteOct = DGetPANOTEOTTAVA(srcDoc,subL);
				newNoteOct = DGetPANOTEOTTAVA(dstDoc,newSubL);

				opSyncL = aNoteOct->opSync;
				aNoteL = DFirstSubLINK(srcDoc,opSyncL);
				for ( ; aNoteL; aNoteL = DNextNOTEL(srcDoc,aNoteL))
					if (DNoteSTAFF(srcDoc,aNoteL)==DOttavaSTAFF(srcDoc,objL)) {
						if (DNoteSEL(srcDoc,aNoteL) || !selectedOnly) {
							tmpL = DNextNOTEOTTAVAL(dstDoc,newSubL);		/* Save it before it gets wiped out */
							BlockMove(aNoteOct, newNoteOct, (long)subObjLength[type]);
							newNoteOct->next = newSubL = tmpL;		/* Restore and move on */
							break;
						}
					}
				subL = DNextNOTEOTTAVAL(srcDoc,subL);
			}
			break;
		}

		case TUPLETtype: {
			LINK tpSyncL, aNoteL;
			PANOTETUPLE aNoteTuple, newNoteTuple;

			while (subL) {
				aNoteTuple = DGetPANOTETUPLE(srcDoc,subL);
				newNoteTuple = DGetPANOTETUPLE(dstDoc,newSubL);

				tpSyncL = aNoteTuple->tpSync;
				aNoteL = DFirstSubLINK(srcDoc,tpSyncL);
				for ( ; aNoteL; aNoteL = DNextNOTEL(srcDoc,aNoteL))
					if (DNoteVOICE(srcDoc,aNoteL)==DTupletVOICE(srcDoc,objL)) {
						if (DNoteSEL(srcDoc,aNoteL) || !selectedOnly) {
							tmpL = DNextNOTETUPLEL(dstDoc,newSubL);		/* Save it before it gets wiped out */
							BlockMove(aNoteTuple, newNoteTuple, (long)subObjLength[type]);
							newNoteTuple->next = newSubL = tmpL;		/* Restore and move on */
							break;
						}
					}
				subL = DNextNOTETUPLEL(srcDoc,subL);
			}
			break;
		}

		default:
			break;
	}
	
	/* And return head of newly copied object and its subobject list */
	
	return(newObjL);
}


/* -------------------------------------------------------------------------- DuplicNC -- */
/* Given a LINK to a Sync, create another Sync that is identical except that it includes
only the subobjects in the given voice, i.e., it duplicates the note or chord. Returns
the LINK to the duplicate Sync, or NILINK if there's a problem. */

LINK DuplicNC(Document *doc, LINK syncL, short voice)
{
	LINK copyL, aNoteL, tempL;
	
	copyL = DuplicateObject(SYNCtype, syncL, False, doc, doc, False);

	if (copyL==NILINK)
		NoMoreMemory();
	else {	
		aNoteL = FirstSubLINK(copyL);
		while (aNoteL) {
			tempL = NextNOTEL(aNoteL);
			if (NoteVOICE(aNoteL)!=voice) {
				RemoveLink(copyL, NOTEheap, FirstSubLINK(copyL), aNoteL);
				HeapFree(NOTEheap,aNoteL);
				LinkNENTRIES(copyL)--;
			}
			aNoteL = tempL;
		}
	}

	return copyL;
}


/* ------------------------------------------------------------- InitObject, SetObject -- */

void InitObject(LINK link, LINK left, LINK right, DDIST xd, DDIST yd, Boolean selected,
					Boolean visible, Boolean soft)
{
	PMEVENT p;
	
	p = GetPMEVENT(link);
	p->left = left;
	p->right = right;
	p->tweaked = False;
	p->relSize = 0;
	p->spareFlag = False;
	p->ohdrFiller1 = 0;
	p->ohdrFiller2 = 0;
	SetObject(link, xd, yd, selected, visible, soft);
}


void SetObject(LINK link, DDIST xd, DDIST yd, Boolean selected, Boolean visible,
				Boolean soft)
{
	PMEVENT p;
	
	p = GetPMEVENT(link);
	p->xd = xd;
	p->yd = yd;
	p->selected = selected;
	p->visible = visible;
	p->soft = soft;
}


/* ------------------------------------------------------------------------ GrowObject -- */
/* GrowObject changes the size of an object by a specified number of subobjects and
updates data structure cross-links appropriately. */
	
LINK GrowObject(Document *doc,
						LINK pL,
						short numEntries)	/* Number of subobjects to add (may be negative) */
{
	PSTAFF		pStaff, lStaff, rStaff;
	PMEASURE 	pMeasure, lMeasure, rMeasure;
	PMEVENT		q;
	PRPTEND		pRptEnd, startRpt, endRpt;
	LINK 		staffL, qL, subListL;

	/* Grow the node generically. */
	
	if (!ExpandNode(pL,&subListL,numEntries)) return NILINK;
	
	switch (ObjLType(pL)) {
		case HEADERtype:
			if (pL != doc->headL)
				MayErrMsg("GrowObject: obj at %ld of HEADERtype is not headL", (long)pL);
			doc->headL = pL;
			break;
	
		case STAFFtype:
			staffL = pL;
			pStaff = GetPSTAFF(pL);
			if (pStaff->lStaff) {										/* Update pointers */
				lStaff = GetPSTAFF(pStaff->lStaff);
				lStaff->rStaff = staffL;
			}
			if (pStaff->rStaff) {
				rStaff = GetPSTAFF(pStaff->rStaff);
				rStaff->lStaff = staffL;
			}
			for (qL=staffL; qL!=pStaff->rStaff; qL=RightLINK(qL)) {
				if (ObjLType(qL)==MEASUREtype) {
					q = GetPMEVENT(qL);
					((PMEASURE)q)->staffL = staffL;
				}
			}
			break;
	
		case MEASUREtype:
			pMeasure = GetPMEASURE(pL);
			if (pMeasure->lMeasure) {
				lMeasure = GetPMEASURE(pMeasure->lMeasure);
				lMeasure->rMeasure = pL;
			}
			if (pMeasure->rMeasure)	{
				rMeasure = GetPMEASURE(pMeasure->rMeasure);
				rMeasure->lMeasure = pL;
			}
			break;
	
		case RPTENDtype:
			pRptEnd = GetPRPTEND(pL);
			if (pRptEnd->startRpt) {
				startRpt = GetPRPTEND(pRptEnd->startRpt);
				startRpt->endRpt = pL;
			}
			if (pRptEnd->endRpt)	{
				endRpt = GetPRPTEND(pRptEnd->endRpt);
				endRpt->startRpt = pL;
			}
			break;
	
		case CLEFtype:
		case KEYSIGtype:
		case TIMESIGtype:
		case CONNECTtype:
		case PSMEAStype:
			break;
			
		default:
			MayErrMsg("GrowObject: can't handle type %ld node at %ld",
				(long)(ObjLType(pL)), pL);
			;
	}
	return pL;
}


/* --------------------------------------------------------------------- SimplePlayDur -- */
/* Compute the "simple" playback duration of a note/rest, ignoring tuplet membership
and whole-measure and multibar rests. */

long SimplePlayDur(LINK aNoteL)
{
	return (config.legatoPct*SimpleLDur(aNoteL))/100L;
}


/* ----------------------------------------------------------------------- CalcPlayDur -- */
/* Return the playback duration in PDUR ticks of the given note. Handles whole-measure
and multibar rests, tuplet membership, and unknown duration. In the latter case, we set
this note's play duration to that of the first note we find in its voice and Sync; if
there is no such note, it's an error. */

short CalcPlayDur(LINK syncL, LINK aNoteL, char lDur, Boolean isRest, PCONTEXT pContext)
{
	short playDur;  LINK bNoteL;  PANOTE bNote;
	
	if (isRest && lDur<=WHOLEMR_L_DUR) {
		playDur = TimeSigDur(pContext->timeSigType,				/* Whole rest: duration=measure dur. */
								pContext->numerator,
								pContext->denominator);
		playDur *= ABS(lDur);
		return playDur;
	}

	if (lDur!=UNKNOWN_L_DUR) {
		playDur = SimplePlayDur(aNoteL);						/* Known duration */
		return playDur;
	}

	bNoteL = FirstSubLINK(syncL);								/* Must be unknown duration */
	for ( ; bNoteL; bNoteL = NextNOTEL(bNoteL))
		if (NoteVOICE(bNoteL)==NoteVOICE(aNoteL)) {
			bNote = GetPANOTE(bNoteL);
			return bNote->playDur;
		}

	MayErrMsg("CalcPlayDur: can't add note of unknown duration with no previous note in voice %ld",
					(long)NoteVOICE(aNoteL));
	return -1;
}


/* -------------------------------------------------------------------- IsRestPosLower -- */

static Boolean IsRestPosLower(
				Document *doc,
				short staff,
				short voiceRole 	/* VCROLE_UPPER, etc. */
				)
{
	LINK partL;  PPARTINFO pPart;

	switch (voiceRole) {
		case VCROLE_CROSS:
			partL = FindPartInfo(doc, Staff2Part(doc, staff));
			pPart = GetPPARTINFO(partL);
			return (staff==pPart->firstStaff);
		default:
			return (voiceRole==VCROLE_LOWER);
	}
}


/* ------------------------------------------------------------------------- SetupNote -- */
/* Setup a vanilla (what some people call "B-flat") note or rest. If it has an
accidental, also handle its effect on following notes by calling InsNoteFixAccs
(though something that high-level probably shouldn't be done here). */

LINK SetupNote(
		Document *doc,
		LINK syncL, LINK aNoteL,
		char staffn,
		short halfLn,					/* Relative to the top of the staff */
		char lDur, short ndots,
		char voice,
		Boolean isRest,
		short accident,
		short octType
		)
{
	PANOTE	aNote;
	short	voiceRole, midCHalfLn, effectiveAcc, playDur;
	QDIST	qStemLen;
	CONTEXT	context;							/* current context */
	SHORTQD	yqpit;
	Boolean	makeLower;
	
PushLock(NOTEheap);
	GetContext(doc, syncL, staffn, &context);
	voiceRole = doc->voiceTab[voice].voiceRole;
	aNote = GetPANOTE(aNoteL);

	aNote->staffn = staffn;
	aNote->voice = voice;									/* Must set before calling GetStemInfo */
	aNote->selected = False;
	aNote->visible = True;
	aNote->soft = False;
	aNote->xd = 0;
	aNote->rest = isRest;

	if (isRest) {
		makeLower = IsRestPosLower(doc, staffn, voiceRole);
		yqpit = GetRestMultivoiceRole(&context, voiceRole, makeLower);
		aNote->yd = qd2d(yqpit, context.staffHeight, context.staffLines);
		aNote->yqpit = -1;										/* No QDIST pitch */
		aNote->ystem = 0;										/* No stem end pos. */
		aNote->noteNum = 0;										/* No MIDI note number */
		aNote->accident = 0;									/* No accidental */
	}
	else {
		yqpit = halfLn2qd(halfLn);
		aNote->yd = qd2d(yqpit, context.staffHeight,
						context.staffLines);
		midCHalfLn = ClefMiddleCHalfLn(context.clefType);		/* Get middle C staff pos. */		
		aNote->yqpit = yqpit-halfLn2qd(midCHalfLn);
		aNote->accident = accident;
		effectiveAcc = InsNoteFixAccs(doc, syncL, staffn, 		/* Handle accidental context */
												halfLn-midCHalfLn, accident);
												
		/* Set MIDI note number; if it's within an octave sign, include the offset. */
		
		if (octType>0)
			aNote->noteNum = Pitch2MIDI(midCHalfLn-halfLn+noteOffset[octType-1],
													effectiveAcc);
		else
			aNote->noteNum = Pitch2MIDI(midCHalfLn-halfLn, effectiveAcc);
		makeLower = GetStemInfo(doc, syncL, aNoteL, &qStemLen);
		aNote->ystem = CalcYStem(doc, aNote->yd, NFLAGS(lDur), makeLower,
									context.staffHeight, context.staffLines, qStemLen, False);
	}

	aNote->onVelocity = dynam2velo[context.dynamicType];
	aNote->offVelocity = config.noteOffVel;
	aNote->subType = lDur;

	aNote->ndots = ndots;

	/* FIXME: On upstemmed notes with flags, xMoveDots should really be 2 or 3 larger, but
	   then it should change whenever such notes are beammed or unbeamed or their stem
	   direction or duration changes. Leave this for some other time. */
	   
	aNote->xMoveDots = 3+WIDEHEAD(aNote->subType);
	if (halfLn%2==0)
		aNote->yMoveDots = GetLineAugDotPos(voiceRole, makeLower);
	else
		aNote->yMoveDots = 2;

	aNote->playTimeDelta = 0;
	playDur = CalcPlayDur(syncL, aNoteL, lDur, isRest, &context);
	aNote = GetPANOTE(aNoteL);
	aNote->playDur = playDur;
	aNote->pTime = 0;											/* Used by Tuplet routines */
	
	aNote->inChord = False;
	aNote->unpitched = False;
	aNote->beamed = False;
	aNote->otherStemSide = False;
	aNote->rspIgnore = False;	
	aNote->accSoft = False;
	aNote->playAsCue = False;
	aNote->micropitch = 0;
	aNote->xmoveAcc = DFLT_XMOVEACC;
	aNote->courtesyAcc = 0;
	aNote->doubleDur = False;
	aNote->headShape = NORMAL_VIS;
	aNote->firstMod = NILINK;
	aNote->tiedR = aNote->tiedL = False;
	aNote->slurredR = aNote->slurredL = False;
	aNote->inTuplet = False;
	aNote->inOttava = False;
	aNote->small = False;
	aNote->tempFlag = False;
	aNote->artHarmonic = 0;
	aNote->userID = 0;
//	for (short k = 0; k<4; k++) aNote->segments[k] = 0;
	aNote->reservedN = 0L;
	
PopLock(NOTEheap);
	return aNoteL;
}


/* ----------------------------------------------------------------------- SetupGRNote -- */
/* Set up a vanilla (what some people call "B-flat") grace note. If it has an
accidental, also handle its effect on following notes by calling InsNoteFixAccs
(though something that high-level probably shouldn't be done here). */

LINK SetupGRNote(
				Document *doc,
				LINK grSyncL, LINK aGRNoteL,
				char staffn,
				short halfLn,						/* Relative to the top of the staff */
				char lDur, short ndots,
				char voice,
				short accident,
				short octType
				)
{
	PAGRNOTE	aGRNote;
	short		midCHalfLn, effectiveAcc;
	CONTEXT		context;
	SHORTQD		yqpit;
	
PushLock(GRNOTEheap);
	GetContext(doc, grSyncL, staffn, &context);
	aGRNote = GetPAGRNOTE(aGRNoteL);

	aGRNote->staffn = staffn;
	aGRNote->voice = voice;								/* Must set before calling GetGRStemInfo */
	aGRNote->selected = False;
	aGRNote->visible = True;
	aGRNote->soft = False;
	aGRNote->xd = 0;
	aGRNote->rest = False;

	yqpit = halfLn2qd(halfLn);
	aGRNote->yd = qd2d(yqpit, context.staffHeight,
					context.staffLines);
	midCHalfLn = ClefMiddleCHalfLn(context.clefType);			/* Get middle C staff pos. */		
	aGRNote->yqpit = yqpit-halfLn2qd(midCHalfLn);
	aGRNote->accident = accident;
	effectiveAcc = InsNoteFixAccs(doc, grSyncL, staffn, 		/* Handle accidental context */
											halfLn-midCHalfLn, accident);
											
	/* Set MIDI note number; if it's within an octave sign, include the offset. */
	
	if (octType>0)
		aGRNote->noteNum = Pitch2MIDI(midCHalfLn-halfLn+noteOffset[octType-1],
												effectiveAcc);
	else
		aGRNote->noteNum = Pitch2MIDI(midCHalfLn-halfLn, effectiveAcc);
	aGRNote->ystem = CalcYStem(doc, aGRNote->yd, NFLAGS(lDur),
									False,						/* Always stem up */
									context.staffHeight, context.staffLines,
									config.stemLenGrace, True);

	aGRNote->onVelocity = dynam2velo[context.dynamicType];
	aGRNote->offVelocity = config.noteOffVel;
	aGRNote->subType = lDur;

	aGRNote->ndots = ndots;
	aGRNote->xMoveDots = 3;
	aGRNote->yMoveDots = (halfLn%2==0 ? 1 : 2);

	aGRNote->playTimeDelta = 0;
	aGRNote->playDur = SimpleGRLDur(aGRNoteL);
	aGRNote->pTime = 0;											/* Not used for grace notes */

	aGRNote->inChord = False;
	aGRNote->unpitched = False;
	aGRNote->beamed = False;
	aGRNote->otherStemSide = False;
	aGRNote->rspIgnore = False;	
	aGRNote->accSoft = False;
	aGRNote->playAsCue = False;
	aGRNote->micropitch = 0;
	aGRNote->xmoveAcc = DFLT_XMOVEACC;
	aGRNote->courtesyAcc = 0;
	aGRNote->doubleDur = False;
	aGRNote->headShape = NORMAL_VIS;
	aGRNote->firstMod = NILINK;
	aGRNote->tiedR = aGRNote->tiedL = False;
	aGRNote->slurredR = aGRNote->slurredL = False;
	aGRNote->inTuplet = False;
	aGRNote->inOttava = False;
	aGRNote->small = False;
	aGRNote->tempFlag = False;
	aGRNote->artHarmonic = 0;
	aGRNote->userID = 0;
//	for (short k = 0; k<4; k++) aGRNote->segments[k] = 0;
	aGRNote->reservedN = 0L;
	
PopLock(GRNOTEheap);
	return aGRNoteL;
}


/* -------------------------------------------------------------------------- InitPart -- */
/* Initialize a garden-variety Part subobject. */

void InitPart(LINK partL, short firstStaff, short lastStaff)
{
	PPARTINFO pPart;
	
PushLock(PARTINFOheap);
	pPart = GetPPARTINFO(partL);
	pPart->firstStaff = firstStaff;
	pPart->lastStaff = lastStaff;
	pPart->partVelocity = 0;
	pPart->patchNum = config.defaultPatch;
	pPart->channel = config.defaultChannel;

	pPart->hiKeyNum = MAX_NOTENUM;
	pPart->transpose = 0;
	pPart->loKeyNum = 0;
	pPart->hiKeyName = 'G';
	pPart->tranName = 'C';			
	pPart->loKeyName = 'C';
	pPart->hiKeyAcc = ' ';
	pPart->tranAcc = ' ';			
	pPart->loKeyAcc = ' ';

	strcpy(pPart->name, "Unnamed");
	strcpy(pPart->shortName, "Unnam.");

	/* FreeMIDI-specific initialization */
	
	pPart->fmsOutputDevice = noUniqueID;
	pPart->fmsOutputDestination.basic.destinationType = 0,
	pPart->fmsOutputDestination.basic.name[0] = 0;
PopLock(PARTINFOheap);
}


/* ------------------------------------------------------------------------- InitStaff -- */
/* Initialize a garden-variety staff subobject. */

void InitStaff(LINK aStaffL, short staff, short top, short left, short right,
						DDIST height, short lines, short showLines)
{
	PASTAFF aStaff;
	
	aStaff = GetPASTAFF(aStaffL);
	aStaff->staffn = staff;
	aStaff->selected = False;		/* ??WHY ISN'T THIS NEEDED FOR OTHER SUBOBJS? */
	aStaff->visible = True;
	aStaff->fillerStf = 0;
	aStaff->staffTop = top;
	aStaff->staffLeft = left;
	aStaff->staffRight = right;
	aStaff->staffHeight = height;
	aStaff->staffLines = lines;
	aStaff->showLines = showLines;
	aStaff->showLedgers = True;
	aStaff->fontSize = Staff2MFontSize(height);
	aStaff->flagLeading = 0;							/* no longer used */
	aStaff->minStemFree = 0;							/* no longer used */
	aStaff->ledgerWidth = 0;							/* no longer used */
	aStaff->noteHeadWidth = HeadWidth(height/(lines-1));
	aStaff->fracBeamWidth = FracBeamWidth(height/(lines-1));
	aStaff->nKSItems = 0;								/* May be filled in later */
	aStaff->filler = 0;
} 


/* -------------------------------------------------------------------------- InitClef -- */
/*	Initialize a garden-variety clef subobject. */

void InitClef(LINK aClefL, short staff, DDIST xd, short clefType)
{
	PACLEF aClef;
	
	aClef = GetPACLEF(aClefL);
	aClef->staffn = staff;
	aClef->small = 0;
	aClef->xd = xd;
	aClef->yd = 0;
	aClef->visible = True;
	aClef->subType = clefType;
	aClef->filler1 = aClef->filler2 = 0;
}


/* ------------------------------------------------------------------------ InitKeySig -- */
/*	Initialize a garden-variety key signature subobject. Caveat: doesn't fill in
the no. of sharps/flats and what they are! For that, use SetupKeySig. */

void InitKeySig(LINK aKeySigL, short staff, DDIST xd, short nKSItems)
{
	PAKEYSIG aKeySig;
	
	aKeySig = GetPAKEYSIG(aKeySigL);
	aKeySig->staffn = staff;
	aKeySig->small = 0;
	aKeySig->xd = xd;
	aKeySig->visible = (nKSItems!=0);				/* Keysig of no sharps/flats is invisible */
	aKeySig->soft = False;
	aKeySig->nKSItems = 0;							/* Must be filled in later */
	aKeySig->subType = 0;							/* May be filled in later */
	aKeySig->nonstandard = 0;
	aKeySig->filler1 = aKeySig->filler2 = 0;
}


/* ------------------------------------------------------------------------- AddKSItem -- */

static void AddKSItem(LINK aKeySigL, Boolean isSharp, short n, short line)
{
	PAKEYSIG	aKeySig;
	
	aKeySig = GetPAKEYSIG(aKeySigL);
	aKeySig->KSItem[n].letcode = line;
	aKeySig->KSItem[n].sharp = isSharp;
}


/* ----------------------------------------------------------------------- SetupKeySig -- */
/* Set fields in key signature node in data structure, for conventional key signatures
only (all we support as of v. 5.9). */

enum {A = 5, B = 4, C = 3, D = 2, E =1, F = 0, G = 6};

void SetupKeySig(LINK aKeySigL, short sharpsOrFlats)	/* >0 = sharps, <0 = flats */
{
	PAKEYSIG	aKeySig;
	short		nItems;									/* No. of sharps/flats in key sig. */

	aKeySig = GetPAKEYSIG(aKeySigL);
	aKeySig->nKSItems = nItems = ABS(sharpsOrFlats);

	if (sharpsOrFlats>0) {
		if (nItems > 0)
			AddKSItem(aKeySigL, True, 0, F);					/* sharp on F */
		if (nItems > 1)
			AddKSItem(aKeySigL, True, 1, C);					/* sharp on C */
		if (nItems > 2)
			AddKSItem(aKeySigL, True, 2, G);					/* sharp on G */
		if (nItems > 3)
			AddKSItem(aKeySigL, True, 3, D);					/* sharp on D */
		if (nItems > 4)
			AddKSItem(aKeySigL, True, 4, A);					/* sharp on A */
		if (nItems > 5)
			AddKSItem(aKeySigL, True, 5, E);					/* sharp on E */
		if (nItems > 6)
			AddKSItem(aKeySigL, True, 6, B);					/* sharp on B */
	}
	else if (sharpsOrFlats<0) {
		if (nItems > 0)
			AddKSItem(aKeySigL, False, 0, B);					/* flat on B */
		if (nItems > 1)
			AddKSItem(aKeySigL, False, 1, E);					/* flat on E */
		if (nItems > 2)
			AddKSItem(aKeySigL, False, 2, A);					/* flat on A */
		if (nItems > 3)
			AddKSItem(aKeySigL, False, 3, D);					/* flat on D */
		if (nItems > 4)
			AddKSItem(aKeySigL, False, 4, G);					/* flat on G */
		if (nItems > 5)
			AddKSItem(aKeySigL, False, 5, C);					/* flat on C */
		if (nItems > 6)
			AddKSItem(aKeySigL, False, 6, F);					/* flat on F */
	}
}


/* ----------------------------------------------------------------------- InitTimeSig -- */
/*	Initialize a garden-variety time signature subobject. */

void InitTimeSig(LINK aTimeSigL, short staff, DDIST xd, short timeSigType, short numerator,
						short denominator)
{
	PATIMESIG aTimeSig;
	
	aTimeSig = GetPATIMESIG(aTimeSigL);
	aTimeSig->staffn = staff;
	aTimeSig->small = 0;
	aTimeSig->connStaff = 0;					/* For future use */
	aTimeSig->xd = xd;
	aTimeSig->yd = 0;
	aTimeSig->visible = True;
	aTimeSig->subType = timeSigType;
	aTimeSig->numerator = numerator;
	aTimeSig->denominator = denominator;
	aTimeSig->filler = 0;
}


/* ----------------------------------------------------------------------- InitMeasure -- */
/*	Initialize a garden-variety measure subobject. */

void InitMeasure(LINK aMeasureL, short staff, short left, short top, short right,
						short bottom, Boolean barlineVisible,
						Boolean connAbove, short connStaff, short measNum)
{
	PAMEASURE aMeasure;

	aMeasure = GetPAMEASURE(aMeasureL);
	aMeasure->staffn = staff;
	SetDRect(&aMeasure->measSizeRect, left, top, right, bottom);
	aMeasure->visible = barlineVisible;
	aMeasure->measureVisible = True;
	aMeasure->connAbove = connAbove;
	aMeasure->connStaff = connStaff;
	aMeasure->measureNum = measNum;
	aMeasure->subType = BAR_SINGLE;
	aMeasure->nKSItems = 0;									/* Must be filled in later */
	aMeasure->filler1 = aMeasure->filler2 = 0;
	aMeasure->xMNStdOffset = aMeasure->yMNStdOffset = 0;
}


/* --------------------------------------------------------------------- InitPSMeasure -- */
/*	Initialize a generic PSMeasure subobject. */

void InitPSMeasure(LINK aPSMeasL, short staff, Boolean barlineVisible,
							Boolean connAbove, short connStaff, char subType)
{
	PAPSMEAS aPSMeas;

	aPSMeas = GetPAPSMEAS(aPSMeasL);
	aPSMeas->staffn = staff;
	aPSMeas->subType = subType;
	aPSMeas->visible = barlineVisible;
	aPSMeas->connAbove = connAbove;
	aPSMeas->connStaff = connStaff;
	aPSMeas->filler1 = 0;
}


/* ------------------------------------------------------------------------ InitRptEnd -- */
/*	Initialize a garden-variety RepeatEnd subobject. */

void InitRptEnd(LINK pL, short /*staff*/, char rptEndType, LINK measL)
{
	PRPTEND		p;
	PARPTEND	aRpt;
	PAMEASURE 	aMeasure;
	LINK		aRptL, aMeasL;
	
	p = GetPRPTEND(pL);
	p->yd = 0;
	p->visible = p->selected = True;
	p->subType = rptEndType;
	p->firstObj = NILINK;
	p->count = 2;
	
	aRptL = FirstSubLINK(pL);
	aMeasL = FirstSubLINK(measL);
	for ( ; aMeasL; aMeasL=NextMEASUREL(aMeasL),
						 aRptL = NextRPTENDL(aRptL)) {
		aRpt = GetPARPTEND(aRptL);
		aMeasure = GetPAMEASURE(aMeasL);
		aRpt->subType = 0;									/* Unused: obj has type */
		aRpt->selected = True;
		aRpt->staffn = aMeasure->staffn;
		aRpt->connAbove = aMeasure->connAbove;
		aRpt->connStaff = aMeasure->connStaff;
		aRpt->filler = 0;
	}
}


/* ----------------------------------------------------------------------- InitDynamic -- */
/*	Initialize a garden-variety Dynamic subobject. */

void InitDynamic(Document *doc, LINK pL, short staff, short x, DDIST sysLeft,
						short pitchLev, CONTEXT *pContext)
{
	PADYNAMIC aDynamic;

	LinkXD(pL) = p2d(x)-(SysRelxd(doc->selStartL)+sysLeft); 		/* relative to doc->selStartL */

	aDynamic = GetPADYNAMIC(FirstSubLINK(pL));
	aDynamic->staffn = staff;
	aDynamic->subType = 0;											/* Unused: obj has type */
	aDynamic->small = 0;
	aDynamic->xd = 0;
	aDynamic->yd = halfLn2d(pitchLev, pContext->staffHeight,
									pContext->staffLines);
	aDynamic->selected = True;										/* Select the subobj */
	aDynamic->visible = True;
	aDynamic->soft = False;
	aDynamic->mouthWidth = aDynamic->otherWidth = 0;
	aDynamic->endxd = 0;
	aDynamic->endyd = aDynamic->yd;
	aDynamic->dModCode = 0;
	aDynamic->crossStaff = 0;
}


/* ---------------------------------------------------------------------- SetupHairpin -- */

void SetupHairpin(LINK newpL, short staff, LINK lastSyncL, DDIST sysLeft, short endx,
																		Boolean crossSys)
{
	LINK measL;  PDYNAMIC newp;  PADYNAMIC aDynamic;

	newp = GetPDYNAMIC(newpL);
	switch (DynamType(newpL)) {
		case DIM_DYNAM:
		case CRESC_DYNAM:
 			aDynamic = GetPADYNAMIC(FirstSubLINK(newpL));
			aDynamic->mouthWidth = config.hairpinMouthWidth;
						
			/* Search right from the sync at the right end of the hairpin for the next
			   barline; if it is to the left of the corresponding end of the hairpin,
			   truncate it. */

 			if (SystemTYPE(lastSyncL))
 				aDynamic->endxd = p2d(endx)-sysLeft;
 			else
				aDynamic->endxd = p2d(endx)-(SysRelxd(lastSyncL)+sysLeft); /* relative to lastSyncL */

			/* If lastSyncL is the last Sync in its system followed by another system,
			   searching to the right for a measure will find the first measure on the
			   following system. Adding the test <SameSystem(...)> bypasses this problem,
			   but it also bypasses the correction: <if (LinkXD(measL)<p2d ...>, which may
			   be needed for some situation too obscure for me to perceive at this moment
			   (12/13/90). */

			measL = LSSearch(lastSyncL, MEASUREtype, staff, GO_RIGHT, False);
			if (measL && !crossSys && SameSystem(measL,lastSyncL))
				if (LinkXD(measL)<p2d(endx)-sysLeft)
					aDynamic->endxd = -SysRelxd(lastSyncL)+LinkXD(measL)-pt2d(2);

			InvalMeasures(newpL, lastSyncL, staff);						/* NewObjCleanup only Invals 1 meas. */
			newp->lastSyncL = lastSyncL;
			newp->crossSys = crossSys;
			break;
		default:
			newp->lastSyncL = NILINK;
			break;
	}
}


/* ----------------------------------------------------------------------- InitGraphic -- */

void InitGraphic(LINK graphicL, short graphicType, short staff, short voice,
						short fontInd, Boolean relFSize, short fSize, short fStyle,
						short enclosure)
{
	PGRAPHIC pGraphic;

	pGraphic = GetPGRAPHIC(graphicL);

	pGraphic->graphicType = graphicType;
	pGraphic->staffn = staff;
	pGraphic->voice = voice;
	pGraphic->enclosure = enclosure;
	pGraphic->justify = pGraphic->multiLine = 0;
	pGraphic->info = 0;
	pGraphic->vConstrain = pGraphic->hConstrain = False;

	pGraphic->gu.handle = NULL;
	pGraphic->fontInd = fontInd;
	pGraphic->relFSize = relFSize;
	pGraphic->fontSize = fSize;
	pGraphic->fontStyle = fStyle;
	pGraphic->info2 = 0;
}


/* -------------------------------------------------------------------- SetMeasVisible -- */
/* Set the visibility of all subobjects of the given Measure object. */

void SetMeasVisible(LINK measL, Boolean visible)
{
	LINK		aMeasureL;
	PAMEASURE	aMeasure;
	
	LinkVIS(measL) = visible;
	aMeasureL = FirstSubLINK(measL);
	for ( ; aMeasureL; aMeasureL=NextMEASUREL(aMeasureL)) {
		aMeasure = GetPAMEASURE(aMeasureL);
		aMeasure->visible = visible;
	}
}


/* -------------------------------------------------------------------- ChordHasUnison -- */
/* If the specified chord contains any unisons, return True, else False. We check
by looking for notes with the same vertical position, so both perfect and augmented
unisons are detected. */

Boolean ChordHasUnison(LINK syncL, short voice)
{
	short		noteCount, i;
	CHORDNOTE	chordNote[MAXCHORD];
	QDIST		prevyqpit;
	PANOTE		aNote;
	/*
	 *	Get sorted notes and go thru them by y-position. For our purpose, it makes
	 * no difference whether the chord is stem up or down, so choose arbitrarily.
	 */
	noteCount = GSortChordNotes(syncL, voice, True, chordNote);
	
	prevyqpit = 9999;
	for (i = 0; i<noteCount; i++) {
		aNote = GetPANOTE(chordNote[i].noteL);
		if (ABS(aNote->yqpit-prevyqpit)==0) return True;
		prevyqpit = aNote->yqpit;
	}
	
	return False;
}


/* ------------------------------------------------------------------ ChordNoteToRight -- */
/* Return True if the given Sync and voice has a chord that is stem up but has at least
one note to right of the stem. Works even if stem won't be drawn, probably because
all notes are whole notes. */

Boolean ChordNoteToRight(LINK syncL, short voice)
{
	LINK	mainNoteL;
	LINK	aNoteL;
	Boolean	stemDown;

	mainNoteL = FindMainNote(syncL, voice);
	if (mainNoteL) {
		stemDown = (NoteYSTEM(mainNoteL) > NoteYD(mainNoteL));
		if (stemDown) return False;
		
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			if (NoteVOICE(aNoteL)==voice && !IsNoteLeftOfStem(syncL, aNoteL, stemDown))
				return True;
		}
	}
	
	return False;
}


/* ------------------------------------------------------------------- ChordNoteToLeft -- */
/* Return True if the given Sync and voice has a chord that is stem down and has at
least one note to left of the stem. Works even if stem won't be drawn, typically because
all notes are whole notes. */

Boolean ChordNoteToLeft(LINK syncL, short voice)
{
	LINK	mainNoteL;
	LINK	aNoteL;
	Boolean	stemDown;

	mainNoteL = FindMainNote(syncL, voice);
	if (mainNoteL) {
		stemDown = (NoteYSTEM(mainNoteL) > NoteYD(mainNoteL));
		if (!stemDown) return False;
		
		aNoteL = FirstSubLINK(syncL);
		for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL)) {
			if (NoteVOICE(aNoteL)==voice && IsNoteLeftOfStem(syncL, aNoteL, stemDown))
				return True;
		}
	}
	
	return False;
}

/* ------------------------------------------------------------------ NormalStemUpDown -- */
/* If the note or chord in the given Sync and voice would normally (considering
the voice's multivoice position) be stem up, return 1, else -1. Cf. the DOWNSTEM
macro. */

static short NormalStemUpDown(Document *doc, LINK syncL, short voice, CONTEXT *pContext)
{
	DDIST	maxy, miny, midLine;
	LINK	aNoteL;
	LINK	hiNoteL, lowNoteL;
	Boolean	stemDown;
	LINK	partL;
	PPARTINFO pPart;

	switch (doc->voiceTab[voice].voiceRole) {
		case VCROLE_SINGLE:
			maxy = (DDIST)(-9999);
			miny = (DDIST)9999;
			for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL))
				if (NoteVOICE(aNoteL)==voice) {
					if (NoteYD(aNoteL)>maxy) {
						maxy = NoteYD(aNoteL);
						lowNoteL = aNoteL;						/* yd's increase going downward */
					}
					if (NoteYD(aNoteL)<miny) {
						miny = NoteYD(aNoteL);
						hiNoteL = aNoteL;
					}
				}
		
			midLine = pContext->staffHeight/2;
			return (maxy-midLine<=midLine-miny? -1 : 1);
		case VCROLE_UPPER:
			return 1;
		case VCROLE_LOWER:
			return -1;
		case VCROLE_CROSS:
			aNoteL = NoteInVoice(syncL, voice, False);
			partL = FindPartInfo(doc, Staff2Part(doc, NoteSTAFF(aNoteL)));
			pPart = GetPPARTINFO(partL);
			stemDown = (NoteSTAFF(aNoteL)==pPart->firstStaff);
			return (stemDown? -1 : 1);
		default:
			return 1;											/* Should never get here */
	}
}


/* ------------------------------------------------------------------------ GetNCYStem -- */
/* Find the correct stem endpoint for a note or chord. Return the stem endpoint the
note closest to the end of the stem for the specified direction would have if it wasn't
in a chord. */

DDIST GetNCYStem(
			Document	*doc,
			LINK		syncL,
			short		voice,
			short		stemUpDown,				/* 1=up, -1=down */
			Boolean		singleVoice,			/* True=voice doesn't share staff */
			PCONTEXT	pContext
			)
{
	DDIST	maxy, miny, farStem, stemLen;
	LINK	aNoteL;
	LINK	hiNoteL, lowNoteL, farNoteL;
	
	maxy = (DDIST)(-9999);
	miny = (DDIST)9999;
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			if (NoteYD(aNoteL)>maxy) {
				maxy = NoteYD(aNoteL);
				lowNoteL = aNoteL;					/* yd's increase going downward */
			}
			if (NoteYD(aNoteL)<miny) {
				miny = NoteYD(aNoteL);
				hiNoteL = aNoteL;
			}
		}
	}

	/* Find the note closest to the end of the stem (i.e., furthest from the MainNote)
	   and return the stem endpoint it would have if it were pointed in the correct
	   direction but weren't in a chord. */
	   
	farNoteL = (stemUpDown<0? lowNoteL : hiNoteL);
	stemLen = QSTEMLEN(singleVoice, ShortenStem(farNoteL, *pContext, stemUpDown<0));
	farStem = CalcYStem(doc, NoteYD(farNoteL), NFLAGS(NoteType(farNoteL)),
									stemUpDown<0,
									pContext->staffHeight, pContext->staffLines,
									stemLen, False);
	return farStem;
}


/* ----------------------------------------------------==------------ FixChordForYStem -- */
/* Fix all the notes in a chord for a given stem direction and endpoint:
	-Set their inChord flags;
	-Put them on their correct sides of the stem;
	-Position their accidentals to avoid collisions among them;
	-Set the MainNote's stem as specified and set all other stem lengths to 0.
Notice that, if <ystem> equals the MainNote's <yd>, this will remove stems from all
notes in the chord without warning!  
*/

void FixChordForYStem(
				LINK	syncL,
				short	voice,
				short	stemUpDown,				/* 1=up, -1=down */
				short	ystem					/* New ystem of chord */
				)
{
	LINK aNoteL;
	LINK lowNoteL, hiNoteL, farNoteL;
	PANOTE aNote;
	DDIST maxy, miny;
	
#ifndef PUBLIC_VERSION
	if (stemUpDown==0) MayErrMsg("FixChordForYStem: stemUpDown is bad.");
#endif
	
	maxy = (DDIST)(-9999);
	miny = (DDIST)9999;
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->voice==voice) {
			aNote->inChord = True;
			if (aNote->yd>maxy) {
				maxy = aNote->yd;
				lowNoteL = aNoteL;					/* yd's increase going downward */
			}
			if (aNote->yd<miny) {
				miny = aNote->yd;
				hiNoteL = aNoteL;
			}
		}
	}	
	if (stemUpDown<0)	farNoteL = hiNoteL;
	else				farNoteL = lowNoteL;

	ArrangeChordNotes(syncL, voice, stemUpDown<0);
	
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
		aNote = GetPANOTE(aNoteL);
		if (aNote->voice==voice)
			if (aNoteL==farNoteL) {
				aNote->ystem = ystem;				/* "Main note": set stem as necessary */
			}
			else
				aNote->ystem = aNote->yd;			/* Other notes: disappear stem */
	}
}


/* ------------------------------------------------------------------- FixSyncForChord -- */
/* Normalize notation of a note or of all the notes in a chord:
	-Set their inChord flags;
	-Put them on their correct sides of the stem;
	-Position their accidentals to avoid collisions among them;
	-Set all their xd's to that of the chord's MainNote, e.g., its only note with a
		stem, as of when the routine is called, if it has a MainNote; if it doesn't,
		set all their xd's to that of the first note encountered.
	-Correct stem lengths and (optionally) stem direction.
We don't arrange augmentation dot positions and visibilities, but it would be nice
if we did some day.

We don't require the chord to have a MainNote, so this routine can be used to fix
things when deleting notes. It returns False in case of an error, else True.
NB: Will do bad things if the the given sync and voice does not have a chord. */

Boolean FixSyncForChord(
				Document	*doc,
				LINK		syncL,
				short		voice,
				Boolean		beamed,			/* If True, ignore stemUpDown and keep current stem endpt */
				short		stemUpDown,		/* 1=up, -1=down, 0=let FixSyncForChord decide */
				short		voices1orMore,	/* 1=single voice on staff, -1=multiple, 0=let us decide */
				PCONTEXT	pContext 		/* Context; if NULL, FixSyncForChord will get it */
				)
{
	short 	staff;
	DDIST	newxd, ystem;
	LINK	aNoteL;
	LINK	mainNoteL;
	CONTEXT	context;
	Boolean	singleVoice;				/* True=voice doesn't share staff */
	
	if (beamed && stemUpDown!=0)
		return False;
	
	mainNoteL = NILINK;
	staff = NOONE;
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			staff = NoteSTAFF(aNoteL);
			NoteINCHORD(aNoteL) = True;
			if (MainNote(aNoteL)) {
				newxd = NoteXD(aNoteL);
				mainNoteL = aNoteL;
			}
		}
	}

	if (staff==NOONE) {
		MayErrMsg("FixSyncForChord: no notes in voice %ld in sync at %ld.",
					(long)voice, (long)syncL);
		return False;
	}

	if (pContext)	context = *pContext;
	else			GetContext(doc, syncL, staff, &context);

	if (beamed && mainNoteL!=NILINK)
		stemUpDown = (NoteYSTEM(mainNoteL)>NoteYD(mainNoteL)? -1 : 1);
	if (!stemUpDown)
		stemUpDown = NormalStemUpDown(doc, syncL, voice, &context);
		
	if (voices1orMore==0)	singleVoice = (doc->voiceTab[voice].voiceRole==VCROLE_SINGLE);
	else					singleVoice = (voices1orMore==1);
	
	/* In all cases, <context>, <stemUpDown>, and <singleVoice> are now defined. */
	
	ystem = GetNCYStem(doc, syncL, voice, stemUpDown, singleVoice, &context);

	FixChordForYStem(syncL, voice, stemUpDown, ystem);
	
	/* Set xd's of all notes in the chord to agree; see comment above. */
	
	for (aNoteL = FirstSubLINK(syncL); aNoteL; aNoteL = NextNOTEL(aNoteL)) {
		if (NoteVOICE(aNoteL)==voice) {
			if (mainNoteL==NILINK) {
				newxd = NoteXD(aNoteL);
				mainNoteL = aNoteL;
			}
			NoteXD(aNoteL) = newxd;
		}
	}
	
	return True;
}


/* ----------------------------------------------------------------------- FixSyncNote -- */
/* For the one note in the given Sync and voice, fix its ystem, clear its inChord flag,
put it on the normal side of the stem, and move its accidental to default position. */

void FixSyncNote(Document *doc,
					LINK syncL,
					short voice,
					PCONTEXT pContext)				/* Context to use or NULL */
{
	CONTEXT	context;
	PANOTE	aNote;
	LINK	aNoteL;
	Boolean	stemDown;
	QDIST	qStemLen;
	
	aNoteL = NoteInVoice(syncL, voice, False);
	if (aNoteL) {
		aNote = GetPANOTE(aNoteL);
		aNote->inChord = False;
		aNote->otherStemSide = False;
		aNote->xmoveAcc = DFLT_XMOVEACC;
		stemDown = GetStemInfo(doc, syncL, aNoteL, &qStemLen);
		if (pContext) context = *pContext;
		else			  GetContext(doc, syncL, NoteSTAFF(aNoteL), &context);
		NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL), NFLAGS(NoteType(aNoteL)),
											stemDown, context.staffHeight,
											context.staffLines, qStemLen, False);
	}
}


/* ----------------------------------------------------------------- FixSyncForNoChord -- */
/* When a note that was formerly in a chord (as indicated by its <inChord> flag
being set) is no longer in a chord, fix its ystem, clear its inChord flag, put it
on the normal side of the stem, and move its accidental to default position. If the
note's <inChord> flag isn't set, do nothing. */

void FixSyncForNoChord(Document *doc,
							LINK syncL,
							short voice,
							PCONTEXT pContext)		/* Context to use or NULL */
{
	LINK aNoteL;

	aNoteL = NoteInVoice(syncL, voice, False);
	if (aNoteL && NoteINCHORD(aNoteL))
		FixSyncNote(doc, syncL, voice, pContext);
}


/* -------------------------------------------------------------------- FixNoteForClef -- */
/* Fix the given note or, if it's in a chord, its chord, for the impending move of
the note to a different staff and clef. If the clef in effect on the new staff is
the same as the note's current clef, do nothing. We assume that all notes in the
chord, if there is one, are going to remain in the same chord (i.e., if this note
is going to change voice, all notes in the chord must change). */

void FixNoteForClef(Document *doc,
						LINK syncL, LINK aNoteL,
						short absStaff)					/* Destination staff */
{
	CONTEXT	oldContext, newContext;
	short yPrev, yHere, qStemLen;
	DDIST yDelta;  Boolean stemDown;  PANOTE aNote;

	GetContext(doc,syncL,NoteSTAFF(aNoteL),&oldContext);
	GetContext(doc,syncL,absStaff,&newContext);
	if (oldContext.clefType!=newContext.clefType) {
		yPrev = ClefMiddleCHalfLn(oldContext.clefType);
		yHere = ClefMiddleCHalfLn(newContext.clefType);
		yDelta = halfLn2d(yHere-yPrev, newContext.staffHeight, newContext.staffLines);
	
		aNote = GetPANOTE(aNoteL);
		aNote->yd += yDelta;
		if (aNote->inChord) {
			FixSyncForChord(doc, syncL, aNote->voice, aNote->beamed, 0, 0, NULL);
			}
		else {
			stemDown = GetCStemInfo(doc, syncL, aNoteL, newContext, &qStemLen);
			NoteYSTEM(aNoteL) = CalcYStem(doc, NoteYD(aNoteL), NFLAGS(NoteType(aNoteL)),
											stemDown, newContext.staffHeight,
											newContext.staffLines, qStemLen, False);
		}
	}
}


/* ----------------------------------------------------------------------- GetGRCYStem -- */
/* Find the correct stem endpoint for a grace note or chord. If the note/chord is
beamed, we just return the current endpoint of its MainNote (regardless of the
specifed stem direction). Otherwise, return the stem endpoint of the note closest
to the end of the stem for the specified direction. */

static DDIST GetGRCYStem(Document *, LINK, short, Boolean, short, Boolean, PCONTEXT);
static DDIST GetGRCYStem(
					Document	*doc,
					LINK		grSyncL,
					short		voice,
					Boolean		beamed,					/* If True, ignore <stemUpDown> */
					short		stemUpDown,				/* 1=up, -1=down */
					Boolean		/*singleVoice*/,		/* True=voice doesn't share staff */
					PCONTEXT	pContext
					)
{
	DDIST	maxy, miny, farStem;
	LINK	aGRNoteL;
	LINK	hiGRNoteL, lowGRNoteL, farGRNoteL, mainGRNoteL;
	
	if (beamed) {		
		mainGRNoteL = FindGRMainNote(grSyncL, voice);
		return GRNoteYSTEM(mainGRNoteL);
	}
	else {
		maxy = (DDIST)(-9999);
		miny = (DDIST)9999;
		for (aGRNoteL = FirstSubLINK(grSyncL); aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
			if (GRNoteVOICE(aGRNoteL)==voice) {
				if (GRNoteYD(aGRNoteL)>maxy) {
					maxy = GRNoteYD(aGRNoteL);
					lowGRNoteL = aGRNoteL;					/* yd's increase going downward */
				}
				if (GRNoteYD(aGRNoteL)<miny) {
					miny = GRNoteYD(aGRNoteL);
					hiGRNoteL = aGRNoteL;
				}
			}
		}

		/*
		 * Find the grace note closest to the end of the stem (i.e., furthest from the
		 * GRMainNote) and return the stem endpoint it would have if it were pointed
		 * in the correct direction but weren't in a chord.
		 */
		farGRNoteL = (stemUpDown<0? lowGRNoteL : hiGRNoteL);
		farStem = CalcYStem(doc, GRNoteYD(farGRNoteL), NFLAGS(GRNoteType(farGRNoteL)),
									stemUpDown<0, pContext->staffHeight,
									pContext->staffLines, config.stemLenGrace, False);
		return farStem;
	}
}


/* ---------------------------------------------------------------- FixGRChordForYStem -- */
/* Fix all the notes in a chord for a given stem direction and endpoint:
	-Set their inChord flags;
	-Put them on their correct sides of the stem;
	-Position their accidentals to avoid collisions among them;
	-Set the MainGRNote's stem as specified and set all other stem lengths to 0.
*/

void FixGRChordForYStem(
				LINK	grSyncL,
				short	voice,
				short	stemUpDown,				/* 1=up, -1=down */
				short	ystem 					/* New ystem of chord */
				)
{
	LINK aGRNoteL;
	LINK lowGRNoteL, hiGRNoteL, farGRNoteL;
	PAGRNOTE aGRNote;
	DDIST maxy, miny;
	
#ifndef PUBLIC_VERSION
	if (stemUpDown==0) MayErrMsg("FixGRChordForYStem: stemUpDown is bad.");
#endif
	
	maxy = (DDIST)(-9999);
	miny = (DDIST)9999;
	aGRNoteL = FirstSubLINK(grSyncL);
	for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (aGRNote->voice==voice) {
			aGRNote->inChord = True;
			if (aGRNote->yd>maxy) {
				maxy = aGRNote->yd;
				lowGRNoteL = aGRNoteL;						/* yd's increase going downward */
			}
			if (aGRNote->yd<miny) {
				miny = aGRNote->yd;
				hiGRNoteL = aGRNoteL;
			}
		}
	}	
	if (stemUpDown<0)
		farGRNoteL = hiGRNoteL;
	else
		farGRNoteL = lowGRNoteL;

	ArrangeChordGRNotes(grSyncL, voice, stemUpDown<0);
	
	aGRNoteL = FirstSubLINK(grSyncL);
	for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		if (aGRNote->voice==voice)
			if (aGRNoteL==farGRNoteL) {
				aGRNote->ystem = ystem;					/* "Main note": set stem as necessary */
			}
			else
				aGRNote->ystem = aGRNote->yd;			/* Other notes: disappear stem */
	}
}


/* ----------------------------------------------------------------- FixGRSyncForChord -- */
/* Fix all the grace notes in a grace chord:
	-Set their inChord flags;
	-Put them on their correct sides of the stem;
	-Position their accidentals to avoid collisions among them;
	-Set all their xds to that of the chord's MainNote, e.g., its only note with a
		stem, as of when the routine is called, if it has a MainNote; if it doesn't,
		set them to that of the first note encountered.
	-Correct stem lengths and (optionally) stem direction.

We don't require the chord have a GRMainNote so this routine can be used to fix things
when deleting grace notes. It returns False in case of an error, else True.
N.B. Will do bad things if the the given sync and voice does not have a chord. */

Boolean FixGRSyncForChord(
				Document	*doc,
				LINK		grSyncL,
				short		voice,
				Boolean		beamed,			/* If True, ignore stemUpDown and keep current stem endpt */
				short		stemUpDown,		/* 1=up, -1=down, 0=let FixGRSyncForChord decide */
				short		voices1orMore,	/* 1=single voice on staff, -1=multiple, 0=let us decide */
				PCONTEXT	pContext 		/* Context; if NULL, FixGRSyncForChord will get it */
				)
{
	short 	staff;
	DDIST	newxd, ystem;
	LINK	aGRNoteL, mainNoteL;
	CONTEXT	context;
	Boolean	singleVoice;				/* True=voice doesn't share staff */
	
	if (beamed && stemUpDown) {
		MayErrMsg("FixGRSyncForChord: called with beamed and stemUpDown=%ld. grSyncL=%ld",
					(long)stemUpDown, (long)grSyncL);
		return False;
	}
	
	mainNoteL = NILINK;
	staff = NOONE;
	for (aGRNoteL = FirstSubLINK(grSyncL); aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		if (GRNoteVOICE(aGRNoteL)==voice) {
			staff = GRNoteSTAFF(aGRNoteL);
			GRNoteINCHORD(aGRNoteL) = True;
			if (GRMainNote(aGRNoteL)) {
				newxd = GRNoteXD(aGRNoteL);
				mainNoteL = aGRNoteL;
			}
		}
	}

	if (staff==NOONE) {
		MayErrMsg("FixGRSyncForChord: no notes in voice %ld in sync at %ld.",
					(long)voice, (long)grSyncL);
		return False;
	}

	if (pContext) context = *pContext;
	else			  GetContext(doc, grSyncL, staff, &context);

	if (beamed && mainNoteL!=NILINK)
		stemUpDown = (GRNoteYSTEM(mainNoteL)>GRNoteYD(mainNoteL)? -1 : 1);
	if (!stemUpDown)
		stemUpDown = 1;					/* ??Probably should call GRNormalStemUpDown instead */
		
	if (voices1orMore==0)	singleVoice = (doc->voiceTab[voice].voiceRole==VCROLE_SINGLE);
	else					singleVoice = (voices1orMore==1);
	
	/* In all cases, <context>, <stemUpDown>, and <singleVoice> are now defined. */
	
	ystem = GetGRCYStem(doc, grSyncL, voice, beamed, stemUpDown, singleVoice, &context);

	FixGRChordForYStem(grSyncL, voice, stemUpDown, ystem);
	
	for (aGRNoteL = FirstSubLINK(grSyncL); aGRNoteL; aGRNoteL = NextGRNOTEL(aGRNoteL)) {
		if (GRNoteVOICE(aGRNoteL)==voice) {
			if (mainNoteL==NILINK) {
				newxd = GRNoteXD(aGRNoteL);
				mainNoteL = aGRNoteL;
			}
			GRNoteXD(aGRNoteL) = newxd;
		}
	}
	return True;
}


/* ------------------------------------------------------------------- FixGRSyncNote -- */
/* For the grace note in the given GRSync and voice, fix its ystem, clear its inChord
flag, put it on the normal side of the stem, and move its accidental to default
position. If the grace note is in a chord, this function will cause serious damage! */

void FixGRSyncNote(Document *doc,
						LINK grSyncL,
						short voice,
						PCONTEXT pContext)		/* Context to use or NULL */
{
	CONTEXT		context;
	PAGRNOTE	aGRNote;
	LINK		aGRNoteL;
	Boolean		stemDown;
	QDIST		qStemLen;
	
	aGRNoteL = GRNoteInVoice(grSyncL, voice, False);
	if (aGRNoteL) {
		aGRNote = GetPAGRNOTE(aGRNoteL);
		aGRNote->inChord = False;
		aGRNote->otherStemSide = False;
		aGRNote->xmoveAcc = DFLT_XMOVEACC;
		stemDown = False;							/* ??wrong if voiceRole==upper or cross */
		qStemLen = config.stemLenGrace;
		if (pContext) context = *pContext;
		else			  GetContext(doc, grSyncL, GRNoteSTAFF(aGRNoteL), &context);
		GRNoteYSTEM(aGRNoteL) = CalcYStem(doc, GRNoteYD(aGRNoteL), NFLAGS(GRNoteType(aGRNoteL)),
												stemDown, 
												context.staffHeight, context.staffLines,
												qStemLen, True);
	}
}


/* --------------------------------------------------------------- FixGRSyncForNoChord -- */
/* When a grace note that was formerly in a chord (as indicated by its <inChord> flag
being set) is no longer in a chord, fix its ystem, clear its inChord flag, put it
on the normal side of the stem, and move its accidental to default position. If the
grace note's <inChord> flag isn't set, do nothing. N.B. If the note is STILL in a
chord, this function will cause serious problems! */

void FixGRSyncForNoChord(Document *doc,
							LINK grSyncL,
							short voice,
							PCONTEXT pContext)		/* Context to use or NULL */
{
	LINK	aGRNoteL;
	
	aGRNoteL = GRNoteInVoice(grSyncL, voice, False);
	if (aGRNoteL && GRNoteINCHORD(aGRNoteL))
		FixGRSyncNote(doc, grSyncL, voice, pContext);
}


/* ---------------------------------------------------------------------- FixAugDotPos -- */
/* Set the augmentation dot position for notes in the given Sync and voice to a
reasonable standard value, regardless of whether the notes actually have dots or
not. Does not handle grace Syncs, which (as of v. 5.7) can never have dots.

Augmentation dots for a "line" note should never be next to the note: normally
they should in the space above, but for stem-down voices in two-voice notation,
they should be in the space below (according to most authorities--including Don
Byrd--though not all). Dots for a "space" note should nearly always be next to
the note. Therefore, there should rarely be a need to set the position for space
notes, and this function makes setting them optional.

Cf. FixAugDots: perhaps they should be combined somehow. Also cf. FixNoteAugDotPos:
?? This is a mess. */

void FixAugDotPos(
			Document *doc,
			LINK syncL,
			short voice,
			Boolean lineNotesOnly)				/* True=set position for "line" notes only */	
{
	short	voiceRole, halfSp, midCHalfSp;
	LINK	mainNoteL, aNoteL;
	Boolean stemDown, lineNote, midCIsInSpace;
	CONTEXT context;
	
	mainNoteL = FindMainNote(syncL, voice);
	stemDown = NoteYSTEM(mainNoteL)>NoteYD(mainNoteL);
	voiceRole = doc->voiceTab[voice].voiceRole;
	
	GetContext(doc, syncL, NoteSTAFF(mainNoteL), &context);
	midCHalfSp = ClefMiddleCHalfLn(context.clefType);			/* Get middle C staff pos. */		
	midCIsInSpace = odd(midCHalfSp);

	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if (NoteVOICE(aNoteL)==voice) {
			halfSp = qd2halfLn(NoteYQPIT(aNoteL));				/* Half-lines below middle C */
			lineNote = (midCIsInSpace? odd(halfSp) : !odd(halfSp));

			if (lineNote)										/* Note on line */
				NoteYMOVEDOTS(aNoteL) = GetLineAugDotPos(voiceRole, stemDown);
			else if (!lineNotesOnly)							/* Note in space */
				NoteYMOVEDOTS(aNoteL) = 2;
		}
}


/* ------------------------------------------------------------------- ToggleAugDotPos -- */
/* If the vertical position of augmentation dots is above or below the note, put them
next to the notehead. If the dots are next to the notehead, move them above or below,
depending on the note's voice role. If the dots are invisible (yMoveDots=0), do
nothing. */

void ToggleAugDotPos(Document *doc, LINK aNoteL, Boolean stemDown)
{
	PANOTE aNote;
	short voiceRole;
	
	aNote = GetPANOTE(aNoteL);					
	if (aNote->yMoveDots==1 || aNote->yMoveDots==3)
			aNote->yMoveDots = 2;
	else if (aNote->yMoveDots==2) {
		voiceRole = doc->voiceTab[NoteVOICE(aNoteL)].voiceRole;
		NoteYMOVEDOTS(aNoteL) = GetLineAugDotPos(voiceRole, stemDown);
	}
}
