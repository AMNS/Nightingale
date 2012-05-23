/*										NOTICE
 *
 *	THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROPERTY OF
 *	ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE SECRET AND IS
 *	NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED WRITTEN
 *	AUTHORIZATION FROM THE OWNER.
 *
 *	Copyright © 1989-95 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* ---------------------------------------------------------- FixAllBeamLinks -- */
/* Update bpSync links for all beamset objects in range, either note beams or
grace note beams. */

void FixAllBeamLinks(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL)
{
	FixBeamLinks(oldDoc, fixDoc, startL, endL);

	FixGRBeamLinks(oldDoc, fixDoc, startL, endL);
}


/* ------------------------------------------------------------- FixBeamLinks -- */
/* Update bpSync links for all note Beamset objects in range. */

void FixBeamLinks(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL)
{
	INT16			i, k;
	PANOTE 		aNote;
	PANOTEBEAM 	paNoteBeam;
	LINK			pL, qL, aNoteL, aNoteBeamL;
	
	InstallDoc(fixDoc);
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (BeamsetTYPE(pL) && !GraceBEAM(pL)) {
		for (i=0, qL=RightLINK(pL); i<LinkNENTRIES(pL) && qL!=endL; qL=RightLINK(qL))
			if (SyncTYPE(qL)) {
				aNoteL = FirstSubLINK(qL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					aNote = GetPANOTE(aNoteL);
					if (NoteVOICE(aNoteL)==BeamVOICE(pL) && MainNote(aNoteL)) {
						if (aNote->beamed) {
							aNoteBeamL = FirstSubLINK(pL);
							for (k=0; k<=i; k++,aNoteBeamL=NextNOTEBEAML(aNoteBeamL))
								if (k==i) {
									paNoteBeam = GetPANOTEBEAM(aNoteBeamL);
									paNoteBeam->bpSync = qL;
								}
							i++;
						}
						else if (!NoteREST(aNoteL)) {
							MayErrMsg("FixBeamLinks: Unbeamed note in sync %ld where beamed note expected", (long)qL);
							return;
						}
					}
					if (NoteVOICE(aNoteL)==BeamVOICE(pL) && aNote->beamed)
						aNote->tempFlag = TRUE;
				}
			}
		}
	 InstallDoc(oldDoc);
}


/* ---------------------------------------------------------- FixGRBeamLinks -- */
/* Update bpSync links for all grace-note Beamset objects in range. */

void FixGRBeamLinks(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL)
{
	INT16			i, k;
	PAGRNOTE 	aGRNote;
	PANOTEBEAM 	paNoteBeam;
	LINK			pL, qL, aGRNoteL, aNoteBeamL;
	
	InstallDoc(fixDoc);
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (BeamsetTYPE(pL) && GraceBEAM(pL)) {
		for (i=0, qL=RightLINK(pL); i<LinkNENTRIES(pL) && qL!=endL; qL=RightLINK(qL))
			if (GRSyncTYPE(qL)) {
				aGRNoteL = FirstSubLINK(qL);
				for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
					aGRNote = GetPAGRNOTE(aGRNoteL);
					if (GRNoteVOICE(aGRNoteL)==BeamVOICE(pL) && GRMainNote(aGRNoteL)) {
						if (aGRNote->beamed) {
							aNoteBeamL = FirstSubLINK(pL);
							for (k=0; k<=i; k++,aNoteBeamL=NextNOTEBEAML(aNoteBeamL))
								if (k==i) {
									paNoteBeam = GetPANOTEBEAM(aNoteBeamL);
									paNoteBeam->bpSync = qL;
								}
						}
						else 
							MayErrMsg("FixGRBeamLinks: Unbeamed note in sync %ld where beamed note expected", (long)qL);
						i++;
					}
					if (GRNoteVOICE(aGRNoteL)==BeamVOICE(pL) && aGRNote->beamed)
						aGRNote->tempFlag = TRUE;
				}
			}
		}
	 InstallDoc(oldDoc);
}


/* ------------------------------------------------------------- FixTupletLinks -- */
/* Update tpSync links for all tuplet objects in range, assuming that contiguous
notes in a voice are always tupled. */

void FixTupletLinks(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL)
{
	INT16			i, k;
	PANOTE 		aNote;
	PANOTETUPLE paNoteTuple;
	LINK			pL, qL, aNoteL, aNoteTupleL;
	
	InstallDoc(fixDoc);
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (TupletTYPE(pL)) {
		for (i=0, qL=RightLINK(pL); i<LinkNENTRIES(pL) && qL!=endL; qL=RightLINK(qL))
			if (ObjLType(qL)==SYNCtype) {
				aNoteL = FirstSubLINK(qL);
				for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
					if (NoteVOICE(aNoteL)==TupletVOICE(pL) && MainNote(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (aNote->inTuplet) {
							aNoteTupleL = FirstSubLINK(pL);
							for (k=0; k<=i; k++,aNoteTupleL=NextNOTETUPLEL(aNoteTupleL))
								if (k==i) {
									paNoteTuple = GetPANOTETUPLE(aNoteTupleL);
									paNoteTuple->tpSync = qL;
								}
						}
						else 
							MayErrMsg("FixTupletLinks: Untupled note in sync %ld where tupled note expected. Tuplet at %ld",
										(long)qL, (long)pL);
						i++;
					}
					if (NoteVOICE(aNoteL)==TupletVOICE(pL) && NoteINTUPLET(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						aNote->tempFlag = TRUE;
					}
				}
			}
		}
	InstallDoc(oldDoc);
}


/* ------------------------------------------------------------- FixOctavaLinks -- */
/* Update opSync links for all Octava objects in range. */

void FixOctavaLinks(Document *oldDoc, Document *fixDoc, LINK startL, LINK endL)
{
	INT16				i, k;
	PANOTE 			aNote;
	PAGRNOTE			aGRNote;
	PANOTEOCTAVA 	paNoteOct;
	LINK				pL, qL, aNoteL, aNoteOctL, aGRNoteL;
	
	InstallDoc(fixDoc);
	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		if (OctavaTYPE(pL)) {
			for (i=0, qL=RightLINK(pL); i<LinkNENTRIES(pL) && qL!=endL; qL=RightLINK(qL))
				if (SyncTYPE(qL)) {
					aNoteL = FirstSubLINK(qL);
					for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL)) {
						aNote = GetPANOTE(aNoteL);
						if (NoteSTAFF(aNoteL)==OctavaSTAFF(pL) && MainNote(aNoteL) && !NoteREST(aNoteL)) {
							if (aNote->inOctava) {
								aNoteOctL = FirstSubLINK(pL);
								for (k=0; k<=i; k++,aNoteOctL=NextNOTEOCTAVAL(aNoteOctL))
									if (k==i) {
										paNoteOct = GetPANOTEOCTAVA(aNoteOctL);
										paNoteOct->opSync = qL;
									}
							}
							else 
								MayErrMsg("FixOctavaLinks: Unoctavad note in sync %ld where inOctava note expected",
										(long)qL);
							i++;
						}
						if (NoteSTAFF(aNoteL)==OctavaSTAFF(pL) && aNote->inOctava)
							aNote->tempFlag = TRUE;
					}
				}
				else if (GRSyncTYPE(qL)) {
					aGRNoteL = FirstSubLINK(qL);
					for ( ; aGRNoteL; aGRNoteL=NextGRNOTEL(aGRNoteL)) {
						aGRNote = GetPAGRNOTE(aGRNoteL);
						if (GRNoteSTAFF(aGRNoteL)==OctavaSTAFF(pL) && GRMainNote(aGRNoteL)) {
							if (aGRNote->inOctava) {
								aNoteOctL = FirstSubLINK(pL);
								for (k=0; k<=i; k++,aNoteOctL=NextNOTEOCTAVAL(aNoteOctL))
									if (k==i) {
										paNoteOct = GetPANOTEOCTAVA(aNoteOctL);
										paNoteOct->opSync = qL;
									}
							}
							else 
								MayErrMsg("FixOctavaLinks: Unoctavad note in sync %ld where inOctava note expected",
										(long)qL);
							i++;
						}
						if (NoteSTAFF(aGRNoteL)==OctavaSTAFF(pL) && aGRNote->inOctava)
							aGRNote->tempFlag = TRUE;
					}
				}

		}
	InstallDoc(oldDoc);
}


/* ------------------------------------------------------------- FixStructureLinks - */
/* Fix up the structure of the range; fix up cross links for the page,
 * system, staff and measure. Assumes left and right links are valid from
 * fixDoc->headL to endL.
 */

void FixStructureLinks(Document *doc, Document *fixDoc, LINK startL, LINK endL)
{
	LINK pL,pageL=NILINK,lPage,rPage,sysL=NILINK,lSys,rSys,prevMeasL,nextMeasL;
	LINK staffL,lStaff,rStaff;

	if (fixDoc!=doc)
		InstallDoc(fixDoc);

	for (pL = startL; pL!=endL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case PAGEtype:
			
				pageL = pL;
				
				lPage = SSearch(LeftLINK(pageL),PAGEtype,TRUE);
				rPage = SSearch(RightLINK(pageL),PAGEtype,FALSE);
				
				LinkLPAGE(pageL) = lPage;
				LinkRPAGE(pageL) = rPage;
				
				if (lPage) LinkRPAGE(lPage) = pageL;
				if (rPage) LinkLPAGE(rPage) = pageL;

				break;
				
			case SYSTEMtype:
				
				sysL = pL;

				lSys = SSearch(LeftLINK(sysL),SYSTEMtype,TRUE);
				rSys = SSearch(RightLINK(sysL),SYSTEMtype,FALSE);
			
				if (!pageL) pageL = SSearch(sysL, PAGEtype, TRUE);
				
				SysPAGE(sysL) = pageL;
				LinkLSYS(sysL) = lSys;
				LinkRSYS(sysL) = rSys;
			
				if (lSys) LinkRSYS(lSys) = sysL;
				if (rSys) LinkLSYS(rSys) = sysL;
				break;

			case STAFFtype:
			
				/* Fix up cross links for the staff, and its neighbors. */
				staffL = pL;
			
				lStaff = SSearch(LeftLINK(staffL),STAFFtype,TRUE);
				rStaff = SSearch(RightLINK(staffL),STAFFtype,FALSE);
			
				if (!sysL) sysL = SSearch(staffL, SYSTEMtype, TRUE);

				StaffSYS(staffL) = sysL;
				LinkLSTAFF(staffL) = lStaff;
				LinkRSTAFF(staffL) = rStaff;
				
				if (lStaff) LinkRSTAFF(lStaff) = staffL;
				if (rStaff) LinkLSTAFF(rStaff) = staffL;
				break;
			
			case MEASUREtype:
			
				prevMeasL = SSearch(LeftLINK(pL), MEASUREtype, TRUE);
				LinkLMEAS(pL) = prevMeasL;
				if (prevMeasL)
					LinkRMEAS(prevMeasL) = pL;
			
				nextMeasL = SSearch(RightLINK(pL), MEASUREtype, FALSE);
				LinkRMEAS(pL) = nextMeasL;
				if (nextMeasL)
					LinkLMEAS(nextMeasL) = pL;				
			
				staffL = SSearch(LeftLINK(pL),STAFFtype,TRUE);
				sysL = SSearch(LeftLINK(pL),SYSTEMtype,TRUE);
				MeasSTAFFL(pL) = staffL;
				MeasSYSL(pL) = sysL;
				break;
			
			default:
				;
		}
	}
	if (fixDoc!=doc)
		InstallDoc(doc);
}


/* -------------------------------------------------------------- FixCrossLinks -- */
/* Fix up cross links: fix up structure object cross links, measure cross links,
 * and cross links for extended objects.
 */

void FixCrossLinks(Document *doc, Document *fixDoc, LINK startL, LINK endL)
{
	FixStructureLinks(doc, fixDoc, startL, endL);		/* PAGE, SYSTEM, STAFF, MEASURE */
	FixAllBeamLinks(doc, fixDoc, startL, endL);
	FixTupletLinks(doc, fixDoc, startL, endL);
	FixOctavaLinks(doc, fixDoc, startL, endL);
	SetTempFlags(doc, fixDoc, startL, endL, FALSE);
}


/* ----------------------------------------------------------- FixExtCrossLinks -- */
/* Fix up cross links for extended objects.
 */

void FixExtCrossLinks(Document *doc, Document *fixDoc, LINK startL, LINK endL)
{
	FixAllBeamLinks(doc, fixDoc, startL, endL);
	FixTupletLinks(doc, fixDoc, startL, endL);
	FixOctavaLinks(doc, fixDoc, startL, endL);
}
