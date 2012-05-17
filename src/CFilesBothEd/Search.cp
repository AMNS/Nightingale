/***************************************************************************
	FILE:	Search.c
	PROJ:	Nightingale, slight rev. for v.99
	DESC:	Object-list search routines. No user-interface implications.
		LPageSearch				RPageSearch			LSystemSearch
		RSystemSearch			LStaffSearch		RStaffSearch
		LMeasureSearch			RMeasureSearch		L_Search
		LSSearch					LVSearch				LSSearch
		LVUSearch				GSearch				GSSearch
		LJSearch					StaffFindSymRight	StaffFindSyncRight
		FindValidSymRight		StaffFindSymLeft	StaffFindSyncLeft
		FindValidSymLeft		SyncInVoiceMeas	EndMeasSearch
		EndSystemSearch		EndPageSearch		TimeSearchRight
		AbsTimeSearchRight
		MNSearch					RMSearch				SSearch
		EitherSearch			FindUnknownDur		XSysSlurMatch
		KSSearch					FindNoteNum
/***************************************************************************/

/*										NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL
 * PROPERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A
 * TRADE SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE
 * NOT RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1987-99 by Advanced Music Notation Systems, Inc.
 * All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"


/* A helpful hint: in general, as of v.3.1, functions whose names end with
"Search" look for and return a LINK to an object. Those starting with or
containing "Find" look for and return a LINK to either an object or a subobject.
Those starting with "Locate" do a variety of things ??though LocateInsertPt
should probably be renamed FindXXX and moved here. */


/* ------------------------------------------------------------- InitSearchParam -- */
/* Initialize all fields of the given SearchParam to reasonable defaults. */

void InitSearchParam(SearchParam	*pbSearch)
{
	pbSearch->id = ANYONE;
	pbSearch->voice = 1;
	pbSearch->needSelected = FALSE;
	pbSearch->inSystem = FALSE;
	pbSearch->subtype = ANYSUBTYPE;
	pbSearch->optimize = TRUE;
	pbSearch->needInMeasure = FALSE;
}

/* ----------------------------------------------------------------- LPageSearch -- */
/*	Optimized function to search left for a Page object. Special cases:
	-	Systems contain links to their owning pages, which we can use to
		traverse the page list.
	-	Staffs contain links to the owning system, which we can use to get
		the owning page.
	-	Measures also point back to their System, so we use the same trick.
*/

LINK LPageSearch(
			LINK			startL,			/* Search left starting at this node */
			SearchParam	*pbSearch 		/* Specify: id is sheetNum */
			)
{
	LINK 		link;
	Boolean	anyPage;
	
	anyPage = (pbSearch->id==ANYONE);

	for (link = startL; link; )
		switch (ObjLType(link)) {
			case PAGEtype:
				if (anyPage || SheetNUM(link)==pbSearch->id)
					return link;
				else
					link = LinkLPAGE(link);
				break;
			case SYSTEMtype:
				link = SysPAGE(link);
				break;
			case STAFFtype:
				link = StaffSYS(link);
				break;
			case MEASUREtype:
				link = MeasSYSL(link);
				break;
			default:
				link = LeftLINK(link);
		}

	return NILINK;		/* none found */
}


/* ----------------------------------------------------------------- RPageSearch -- */
/*	Optimized function to search right for a Page object. Special cases:
	-	Systems contain links to their owning pages, which we can use to
		traverse the page list, starting at the rPage of the owning page.
	-	Staffs contain links to the owning system, which we can use to get
		rPage of the owning page.
	-	Measures also point back to their System, so we use the same trick.
*/

LINK RPageSearch(
			LINK 			startL,			/* Search right starting at this node. */
			SearchParam *pbSearch 		/* Specify: id is sheetNum */
			)
{
	Boolean	anyPage;
	LINK		link;

	anyPage = (pbSearch->id==ANYONE);

	for (link = startL; link; )
		switch (ObjLType(link)) {
			case PAGEtype:
				if (anyPage || SheetNUM(link)==pbSearch->id)
					return link;
				else
					link = LinkRPAGE(link);
				break;
			case SYSTEMtype:
				link = SysPAGE(link);
				link = LinkRPAGE(link);
				break;
			case STAFFtype:
				link = StaffSYS(link);
				break;
			case MEASUREtype:
				link = MeasSYSL(link);
				break;
			default:
				link = RightLINK(link);
		}

	return NILINK;		/* none found */
}


/* ---------------------------------------------------------------- LSystemSearch -- */
/*	Optimized function to search left for a System object. Special cases:
-	Systems don't belong to any staff, so we can traverse the system
	list to locate a specific system.		
-	Staffs contain links to their owning systems, which we can use to
	traverse the system list.
-	Measures also point back to their System, so we use the same trick.
*/

LINK LSystemSearch(
			LINK	startL,					/* Search left starting at this node */
			SearchParam	*pbSearch 		/* Specify: id is systemNum */
			)
{
	Boolean	anySystem;
	LINK		link;

	anySystem = (pbSearch->id==ANYONE);

	for (link = startL; link; )
		switch (ObjLType(link)) {
			case SYSTEMtype:
				if (anySystem || SystemNUM(link)==pbSearch->id)
					return link;
				link = LinkLSYS(link);
				break;
			case STAFFtype:
				link = StaffSYS(link);
				break;
			case MEASUREtype:
				link = MeasSYSL(link);
				break;
			default:
				link = LeftLINK(link);
		}

	return NILINK;		/* none found */
}


/* ---------------------------------------------------------------- RSystemSearch -- */
/*	Optimized function to search right for a System object. Special cases:
	-	Systems don't belong to any staff, so we can traverse the system
		list to locate a specific system.
	-	Staffs contain links to their owning systems, which we can use to
		traverse the system list, starting at the rSystem of the owning
		system.
	-	Measures also point back to their System, so we use the same trick.
*/

LINK RSystemSearch(
			LINK			startL,			/* Search right starting at this node */
			SearchParam *pbSearch 		/* Specify: id is systemNum */
			)
{
	Boolean 	anySystem;
	LINK		link, systemL;

	anySystem = (pbSearch->id==ANYONE);

	for (link=startL; link; )
		switch (ObjLType(link)) {
			case SYSTEMtype:
				if (anySystem || SystemNUM(link)==pbSearch->id)
					return link;
				link = LinkRSYS(link);
				break;
			case STAFFtype:
				systemL = StaffSYS(link);
				link = LinkRSYS(systemL);
				break;
			case MEASUREtype:
				systemL = MeasSYSL(link);
				link = LinkRSYS(systemL);
				break;
			default:
				link = RightLINK(link);
		}

	return NILINK;		/* none found */
}


/* ----------------------------------------------------------------- LStaffSearch -- */
/*	Optimized function to search left for a Staff object. Special cases:
	-	Staffs point left to the previous Staff, so we can traverse the
		Staff list very quickly.
	-	Measures contain links to their owning staffs, so we can use those
		links to traverse the staff list.
*/

LINK LStaffSearch(
			LINK	startL,					/* Search left starting at this node */
			SearchParam	*pbSearch 		/* Specify: id is staffn; inSystem */
			)
{
	INT16		i;
	Boolean	anyStaff, anySystem;
	LINK 		link, aStaffL;
	
	anyStaff = (pbSearch->id == ANYONE);
	anySystem = (!pbSearch->inSystem);

	for (link = startL; link; ) {
		if (!anySystem && SystemTYPE(link)) break;

		switch (ObjLType(link)) {
			case STAFFtype:
				aStaffL = FirstSubLINK(link);
				for (i = 0; aStaffL; i++, aStaffL=NextSTAFFL(aStaffL)) {
	  				if (anyStaff || StaffSTAFF(aStaffL)==pbSearch->id) {
						pbSearch->pEntry = aStaffL;
						pbSearch->entryNum = i;
	  					return link;
  					}
  				}
				link = LinkLSTAFF(link);
				break;
			case MEASUREtype:
				link = MeasSTAFFL(link);
				break;
			default:
				link = LeftLINK(link);
		}
	}
	return NILINK;		/* none found */
}


/* ----------------------------------------------------------------- RStaffSearch -- */
/*	Optimized function to search right for a Page object. Special cases:
	-	Staffs point right to the next Staff, so we can traverse the Staff
		list quickly.
	-	Measures contain links to their owning staffs, so we can use those
		links to traverse the staff list.
*/

LINK RStaffSearch(
			LINK	startL,					/* Search right starting at this node */
			SearchParam	*pbSearch 		/* Specify: id is staffn; inSystem */
			)
{
	INT16		i;
	Boolean	anyStaff, anySystem;
	LINK		link, aStaffL;

	anyStaff = (pbSearch->id == ANYONE);
	anySystem = (!pbSearch->inSystem);

	for (link = startL; link; ) {

		if (!anySystem && SystemTYPE(link)) break;

		switch (ObjLType(link)) {
			case STAFFtype:
				aStaffL = FirstSubLINK(link);
				for (i = 0; aStaffL; i++, aStaffL=NextSTAFFL(aStaffL)) {
	  				if (anyStaff || StaffSTAFF(aStaffL)==pbSearch->id) {
						pbSearch->pEntry = aStaffL;
						pbSearch->entryNum = i;
  						return link;
  					}
  				}
  				link = LinkRSTAFF(link);
				break;
			case MEASUREtype:
				link = MeasSTAFFL(link);
				link = LinkRSTAFF(link);
				break;
			default:
				link = RightLINK(link);
		}
	}
	return NILINK;		/* none found */
}


/* -------------------------------------------------------------- LMeasureSearch -- */
/*	Optimized function to search left for a Measure object. Special cases:
	-	Measures point back at the previous Measure, so we can quickly
		traverse the Measure list.
*/

LINK LMeasureSearch(
			LINK			startL,			/* Search left starting at this node */
			SearchParam	*pbSearch 		/* Specify: id is staffn; inSystem */
			)
{
	INT16			i;
	Boolean		anyStaff, anySystem;
	LINK			link, aMeasureL;

	anyStaff = (pbSearch->id == ANYONE);
	anySystem = (!pbSearch->inSystem);

	for (link = startL; link; ) {
		if (!anySystem && SystemTYPE(link)) break;

		switch (ObjLType(link)) {
			case MEASUREtype:
				aMeasureL = FirstSubLINK(link);
				for (i = 0; aMeasureL; i++, aMeasureL=NextMEASUREL(aMeasureL)) {
	  				if (anyStaff || MeasureSTAFF(aMeasureL)==pbSearch->id) {
						pbSearch->pEntry = aMeasureL;
						pbSearch->entryNum = i;
	  					return link;
  					}
  				}
				link = LinkLMEAS(link);
				break;
			default:
				link = LeftLINK(link);
		}
	}
	return NILINK;		/* none found */
}


/* -------------------------------------------------------------- RMeasureSearch -- */
/*	Optimized function to search right for a Measure object. Special cases:
	-	Measures point forward to the next Measure, so we can quickly
		traverse the Measure list.
*/

LINK RMeasureSearch(
			LINK			startL,			/* Search right starting at this node */
			SearchParam	*pbSearch		/* Specify: id is staffn; inSystem */
			)
{
	PAMEASURE	aMeasure;
	INT16			i;
	Boolean		anyStaff, anySystem;
	LINK			link, aMeasureL;

	anyStaff = (pbSearch->id == ANYONE);
	anySystem = (!pbSearch->inSystem);

	for (link = startL; link; ) {
		if (!anySystem && SystemTYPE(link)) break;

		switch (ObjLType(link)) {
			case MEASUREtype:
				aMeasureL = FirstSubLINK(link);
				for (i = 0; aMeasureL; i++, aMeasureL=NextMEASUREL(aMeasureL)) {
					aMeasure = GetPAMEASURE(aMeasureL);
	  				if (anyStaff || aMeasure->staffn==pbSearch->id) {
						pbSearch->pEntry = aMeasureL;
						pbSearch->entryNum = i;
	  					return link;
  					}
  				}
				link = LinkRMEAS(link);
				break;
			default:
				link = RightLINK(link);
		}
	}
	return NILINK;		/* none found */
}


/* -------------------------------------------------------------------- L_Search -- */
/* Search for an object and (for objects that have them) subobject of the specified
type, optionally on the specified staff, system, or page, and optionally meeting
other criteria.  If more than one subobject of the object found fulfills the
criteria, the first one is returned.

Here are the object-specific features ("ign" = ignored).
																																																																									*
	Object	Optimized?				id					subtype	voice		needSel	needInMeas
	------	----------	-------------------		-------	-------	-------	----------
	Page			yes		sheetNum or ANYONE		ign		ign		ign		ign
	System		yes		systemNum or ANYONE		ign		ign		ign		ign
	Staff			yes		staffNum or ANYONE		ign		ign		Used		ign
	Measure		yes		staffNum or ANYONE		ign		ign		ign		ign	
	Connect		no			ignored						ign		ign		ign		ign	
	Clef			no			staffNum or ANYONE		ign		ign		Used		Used	
	KeySig		no			staffNum or ANYONE		ign		ign		Used		Used	
	TimeSig		no			staffNum or ANYONE		ign		ign		Used		Used	
	Beamset		no			staffNum or ANYONE		Used		Used		Used		ign	
	Tuplet		no			staffNum or ANYONE		ign		Used		Used		ign	
	Octava		no			staffNum or ANYONE		ign		ign		Used		ign	
	Sync			no			staffNum or ANYONE		ign		Used		Used		ign	
	GRSync		no			staffNum or ANYONE		ign		Used		Used		ign	
	Dynamic		no			staffNum or ANYONE		ign		ign		Used		ign	
	Slur			no			staffNum or ANYONE		Used		Used		ign		ign	
	RptEnd		no			ANYONE only					ign		ign		Used		ign	
	Graphic		no			staffNum or ANYONE		Used		Used(*)	Used		ign	
	Tempo			no			staffNum or ANYONE		ign		ign		Used		ign	
	Spacer		no			staffNum or ANYONE		ign		ign		Used		ign	

(*)Some Graphics don't a voice, i.e., they have voice==NOONE : those Graphics are
considered to match any voice. */

LINK L_Search(
			LINK			startL,					/* Place to start looking */
			INT16			type,						/* target type (or ANYTYPE) */
			Boolean		goLeft,					/* TRUE if we should search left */
			SearchParam	*pbSearch			 	/* ptr to parameter list */
			)
{
	INT16			i;
	PMEVENT		p;
	PPAGE			pPage;
	PSYSTEM		pSystem;
	Boolean		anyType,			/* TRUE if they'll take any type */
					anyStaff,		/* TRUE if they'll take any staff */
					anyVoice,		/* TRUE if they'll take any voice */
					anySelected,	/* TRUE if they don't need selected */
					anySystem,		/* TRUE if they'll take any System */
					anySubtype,		/* TRUE if they'll take any subtype */
					anyInMeasure;	/* TRUE if they don't need inMeasure */
	GenSubObj 	*subObj;
	char			pType;			/* pre-compute type of p */
	Boolean 		pSel;				/* pre-compute p->selected */
	LINK			pL, nextL,
					subObjL,
					aStaffL, aMeasL,aPSMeasL;
	HEAP			*tmpHeap;

	
	if (!startL)
		return NILINK;

	if (!pbSearch) {
		MayErrMsg("L_Search: pbSearch NULL is illegal.");
		return NILINK;
	}
	
	/* First, handle optimized special cases. */

	if (pbSearch->optimize)
		switch (type) {
		case PAGEtype:
				return (goLeft ? LPageSearch(startL, pbSearch) :  RPageSearch(startL, pbSearch));
		case SYSTEMtype:
				return (goLeft ? LSystemSearch(startL, pbSearch) : RSystemSearch(startL, pbSearch));
		case STAFFtype:
				return (goLeft ? LStaffSearch(startL, pbSearch) : RStaffSearch(startL, pbSearch));
		case MEASUREtype:
				return (goLeft ? LMeasureSearch(startL, pbSearch) : RMeasureSearch(startL, pbSearch));
		}
	
	/* Now all other cases. For these, assume <id> is staff number. */
	
	anyType = (type==ANYTYPE);
	anyStaff = (pbSearch->id==ANYONE);
	anyVoice = (pbSearch->voice==ANYONE);
	anySelected = (!pbSearch->needSelected);
	anySystem = (!pbSearch->inSystem);
	anySubtype = (pbSearch->subtype==ANYSUBTYPE);
	anyInMeasure = (!pbSearch->needInMeasure);

	for (pL = startL; pL; pL = nextL) {

		if (!anySystem && SystemTYPE(pL)) break;
		nextL = goLeft ? LeftLINK(pL) : RightLINK(pL);
		p = GetPMEVENT(pL);
		pType = ObjLType(pL);
		pSel = LinkSEL(pL);

		if ((anyType || pType==type) && (anySelected || pSel)) {
			switch (pType) {

				case PAGEtype:
					pPage = GetPPAGE(pL);
					if (pbSearch->id==ANYONE || pbSearch->id==pPage->sheetNum)
						if (anySelected || LinkSEL(pL))
							return pL;
					break;
				case SYSTEMtype:
					pSystem = GetPSYSTEM(pL);
					if (pbSearch->id==ANYONE || pbSearch->id==pSystem->systemNum)
						if (anySelected || LinkSEL(pL))
							return pL;
					break;
				case STAFFtype:
					aStaffL = FirstSubLINK(pL);
					for (i=0; aStaffL; i++,aStaffL = NextSTAFFL(aStaffL))
						if (anyStaff || StaffSTAFF(aStaffL)==pbSearch->id)
							if (anySelected || StaffSEL(aStaffL)) {
								pbSearch->pEntry = aStaffL;
								pbSearch->entryNum = i;
			  					return pL;
			  				}
					break;

				case MEASUREtype:
					aMeasL = FirstSubLINK(pL);
					for (i=0; aMeasL; i++,aMeasL = NextMEASUREL(aMeasL))
						if (anyStaff || MeasureSTAFF(aMeasL)==pbSearch->id)
							if (anySelected || MeasureSEL(aMeasL)) {
								pbSearch->pEntry = aMeasL;
								pbSearch->entryNum = i;
			  					return pL;
			  				}
					break;

				case PSMEAStype:
					aPSMeasL = FirstSubLINK(pL);
					for (i=0; aPSMeasL; i++,aPSMeasL = NextPSMEASL(aPSMeasL))
						if (anyStaff || PSMeasSTAFF(aPSMeasL)==pbSearch->id)
							if (anySelected || PSMeasSEL(aPSMeasL)) {
								pbSearch->pEntry = aPSMeasL;
								pbSearch->entryNum = i;
			  					return pL;
			  				}
					break;

				case CONNECTtype:
					return pL;
				case CLEFtype:
				case KEYSIGtype:
				case TIMESIGtype:
					if (!anyInMeasure) {
						if (ClefTYPE(pL) && !ClefINMEAS(pL)) break;
						if (KeySigTYPE(pL) && !KeySigINMEAS(pL)) break;
						if (TimeSigTYPE(pL) && !TimeSigINMEAS(pL)) break;
					}
					tmpHeap = Heap + p->type;		/* p may not stay valid during loop */
						
					for (i = 0, subObjL=FirstSubObjPtr(p,pL); subObjL; 
							i++, subObjL = NextLink(tmpHeap,subObjL)) {
						subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
						if (anyStaff || subObj->staffn==pbSearch->id)
							if (anySelected || subObj->selected) {
								pbSearch->pEntry = subObjL;
								pbSearch->entryNum = i;
			  					return pL;
			  				}
			  		}
					break;
				case DYNAMtype:
					tmpHeap = Heap + p->type;		/* p may not stay valid during loop */
						
					for (i = 0, subObjL=FirstSubObjPtr(p,pL); subObjL; 
							i++, subObjL = NextLink(tmpHeap,subObjL)) {
						subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
						if (anyStaff || subObj->staffn==pbSearch->id)
							if (anySelected || subObj->selected) {
								pbSearch->pEntry = subObjL;
								pbSearch->entryNum = i;
			  					return pL;
			  				}
			  		}
					break;
				case SYNCtype:
					tmpHeap = Heap + p->type;		/* p may not stay valid during loop */
						
					for (i = 0, subObjL=FirstSubObjPtr(p,pL); subObjL; 
							i++, subObjL = NextLink(tmpHeap,subObjL)) {
						subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
						if (anyStaff || subObj->staffn==pbSearch->id)
							if (anyVoice || ((PANOTE)subObj)->voice==pbSearch->voice)
								if (anySelected || subObj->selected) {
									pbSearch->pEntry = subObjL;
									pbSearch->entryNum = i;
				  					return pL;
				  				}
			  		}
					break;
				case GRSYNCtype:
					tmpHeap = Heap + p->type;		/* p may not stay valid during loop */
						
					for (i = 0, subObjL=FirstSubObjPtr(p,pL); subObjL; 
							i++, subObjL = NextLink(tmpHeap,subObjL)) {
						subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
						if (anyStaff || subObj->staffn==pbSearch->id)
							if (anyVoice || ((PAGRNOTE)subObj)->voice==pbSearch->voice)
								if (anySelected || subObj->selected) {
									pbSearch->pEntry = subObjL;
									pbSearch->entryNum = i;
				  					return pL;
				  				}
			  		}
					break;
				case RPTENDtype: {
					LINK aRptL;
						
						aRptL = FirstSubLINK(pL);
						for ( ; aRptL; aRptL = NextRPTENDL(aRptL)) {
			  				if (anyStaff || RptEndSTAFF(aRptL)==pbSearch->id)
								if (anyVoice)
									if (anySelected || pSel)
			  							return pL;
		  				}
		  			}
					break;
				case BEAMSETtype:
	  				if (anyStaff || ((PBEAMSET)p)->staffn==pbSearch->id)
						if (anyVoice || ((PBEAMSET)p)->voice==pbSearch->voice)
							if (anySelected || pSel) {
								if (((((PBEAMSET)p)->grace) && pbSearch->subtype==GRNoteBeam) ||
										((!((PBEAMSET)p)->grace) && pbSearch->subtype==NoteBeam) ||
											anySubtype)
	  								return pL;
	  						}
					break;
				case TUPLETtype:
	  				if (anyStaff || ((PTUPLET)p)->staffn==pbSearch->id)
						if (anyVoice || ((PTUPLET)p)->voice==pbSearch->voice)
							if (anySelected || pSel)
	  							return pL;
					break;
				case OCTAVAtype:
	  				if (anyStaff || ((POCTAVA)p)->staffn==pbSearch->id)
						if (anySelected || pSel)
	  						return pL;
					break;
				case ENDINGtype:
	  				if (anyStaff || ((PENDING)p)->staffn==pbSearch->id)
						if (anySelected || pSel)
	  						return pL;
					break;
				case GRAPHICtype:
	  				if (anyStaff || ((PGRAPHIC)p)->staffn==pbSearch->id)
						if (anyVoice || !ObjHasVoice(pL) || ((PGRAPHIC)p)->voice==pbSearch->voice)						
							if (anySelected || pSel)
	  							if (anySubtype || ((PGRAPHIC)p)->graphicType==pbSearch->subtype)
	  								return pL;
					break;
				case TEMPOtype:
	  				if (anyStaff || ((PTEMPO)p)->staffn==pbSearch->id)
						if (anySelected || pSel)
	  						return pL;
					break;
				case SPACEtype:
	  				if (anyStaff || ((PSPACE)p)->staffn==pbSearch->id)
						if (anySelected || pSel)
	  						return pL;
					break;
				case SLURtype:
	  				if (anyStaff || ((PSLUR)p)->staffn==pbSearch->id)
						if (anyVoice || ((PSLUR)p)->voice==pbSearch->voice)
	  						if (anySubtype || ((PSLUR)p)->tie==pbSearch->subtype)
	  							return pL;
					break;
				case TAILtype:
					break;
				default:
					MayErrMsg("L_Search: unrecognized type %ld", (long)pType);
			}
		}
	}
	
	return NILINK;														/* Nothing found */
}


/* -------------------------------------------------------------------- LSSearch -- */
/*	Simple search left or right thru object list, by staff. */

LINK LSSearch(
			LINK		startL,			/* Place to start looking */
			INT16		type,				/* target type (or ANYTYPE) */
			INT16		id,				/* target staff number (for Staff, Measure, Sync, etc) */
											/* or page number (for Page) */
			Boolean	goLeft,			/* TRUE if we should search left */
			Boolean	needSelected 	/* TRUE if we only want selected items */
			)
{
	SearchParam	pbSearch;
	LINK			foundL;

	InitSearchParam(&pbSearch);
	pbSearch.id = id;
	pbSearch.needSelected = needSelected;
	pbSearch.inSystem = FALSE;
	pbSearch.subtype = ANYSUBTYPE;
	pbSearch.voice = ANYONE;
	pbSearch.needInMeasure = FALSE;
	pbSearch.optimize = TRUE;
	foundL = L_Search(startL, type, goLeft, &pbSearch);
	return foundL;
}

/* ------------------------------------------------------------------- LSISearch -- */
/*	Simple search left or right thru object list, by staff, with inSystem TRUE. If
called with startL = System obj, will return NILINK. */

LINK LSISearch(
			LINK		startL,			/* Place to start looking */
			INT16		type,				/* target type (or ANYTYPE) */
			INT16		id,				/* target staff number (for Staff, Measure, Sync, etc) */
											/* or page number (for Page) */
			Boolean	goLeft,			/* TRUE if we should search left */
			Boolean	needSelected 	/* TRUE if we only want selected items */
			)
{
	SearchParam	pbSearch;
	LINK			foundL;

	InitSearchParam(&pbSearch);
	pbSearch.id = id;
	pbSearch.needSelected = needSelected;
	pbSearch.inSystem = TRUE;
	pbSearch.subtype = ANYSUBTYPE;
	pbSearch.voice = ANYONE;
	pbSearch.needInMeasure = FALSE;
	pbSearch.optimize = TRUE;
	foundL = L_Search(startL, type, goLeft, &pbSearch);
	return foundL;
}


/* -------------------------------------------------------------------- LVSearch -- */
/* Like LSSearch but search by voice. */

LINK LVSearch(
			LINK		startL,			/* Place to start looking */
			INT16		type,				/* target type (or ANYTYPE) */
			INT16		id,				/* target voice number (for Staff, Measure, Sync, etc) */
			Boolean	goLeft,			/* TRUE if we should search left */
			Boolean	needSelected 	/* TRUE if we only want selected items */
			)
{
	SearchParam	pbSearch;
	LINK			foundL;

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.needSelected = needSelected;
	pbSearch.inSystem = FALSE;
	pbSearch.subtype = ANYSUBTYPE;
	pbSearch.voice = id;
	pbSearch.needInMeasure = FALSE;
	pbSearch.optimize = TRUE;
	foundL = L_Search(startL, type, goLeft, &pbSearch);
	return foundL;
}


/* ------------------------------------------------------------------- LSUSearch -- */
/* Like LSSearch but Unoptimized. */

LINK LSUSearch(
			LINK		startL,				/* Place to start looking */
			INT16		type,					/* target type (or ANYTYPE) */
			INT16		id,					/* target staff number (for Staff, Measure, Sync, etc) */
												/* or page number (for Page) */
			Boolean	goLeft,				/* TRUE if we should search left */
			Boolean	needSelected		/* TRUE if we only want selected items */
			)
{
	SearchParam	pbSearch;
	LINK			foundL;

	InitSearchParam(&pbSearch);
	pbSearch.id = id;
	pbSearch.needSelected = needSelected;
	pbSearch.inSystem = FALSE;
	pbSearch.subtype = ANYSUBTYPE;
	pbSearch.voice = ANYONE;
	pbSearch.needInMeasure = FALSE;
	pbSearch.optimize = FALSE;
	foundL = L_Search(startL, type, goLeft, &pbSearch);
	return foundL;
}

/* ------------------------------------------------------------------- LVUSearch -- */
/* Like LVSearch but Unoptimized. */

LINK LVUSearch(
			LINK		startL,				/* Place to start looking */
			INT16		type,					/* target type (or ANYTYPE) */
			INT16		id,					/* target voice number (for Staff, Measure, Sync, etc) */
			Boolean	goLeft,				/* TRUE if we should search left */
			Boolean	needSelected 		/* TRUE if we only want selected items */
			)
{
	SearchParam	pbSearch;
	LINK			foundL;

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.needSelected = needSelected;
	pbSearch.inSystem = FALSE;
	pbSearch.subtype = ANYSUBTYPE;
	pbSearch.voice = id;
	pbSearch.needInMeasure = FALSE;
	pbSearch.optimize = FALSE;
	foundL = L_Search(startL, type, goLeft, &pbSearch);
	return foundL;
}


/* --------------------------------------------------------------------- GSearch -- */
/*	Search graphically for an object/subobject of the specified type, optionally
on the  specified staff and meeting other criteria.  If more than one subobject
of	the object found fulfills the criteria, the first one is found. Searches 
left or right depending on the value of <goLeft>. */

LINK GSearch(
			Document		*doc,
			Point			pt,						/* x coordinate from which to search */
			INT16			type,						/* target type (or ANYTYPE) */
			Boolean		goLeft,					/* TRUE if we should search left */
			Boolean		useJD,					/* TRUE if we should find type JD symbols */
			Boolean		needVisible,			/* TRUE if only want visible symbols. */
			SearchParam	*pbSearch 				/* ptr to parameter list */
			)
{
	INT16			i;
	PMEVENT		p;
	Boolean		anyType,			/* TRUE if they'll take any type */
					anyStaff,		/* TRUE if they'll take any staff */
					anyVoice,		/* TRUE if they'll take any voice */
					anySelected,	/* TRUE if they don't need selected */
					anySystem,		/* TRUE if they'll take any System */
					anySubtype,		/* TRUE if they'll take any subtype */
					anyVisible;		/* TRUE if they don't need visible */
	GenSubObj	*subObj;
	char			pType;			/* pre-compute type of p */
	Boolean 		pSel,				/* pre-compute p->selected */
					pVis,				/* pre-compute p->visible */
					beyond;			/* TRUE if objRect.l/r is to l/r of h, based on goLeft */
	LINK			pL, nextL, subObjL;
	LINK			aNoteL,aGRNoteL,aRptL,startL;
	HEAP			*tmpHeap;
	
	if (!pbSearch) {
		MayErrMsg("GSearch: pbSearch NULL is illegal.");
		return NILINK;
	}
	anyType = (type==ANYTYPE);
	anyStaff = (pbSearch->id==ANYONE);
	anyVoice = (pbSearch->voice==ANYONE);
	anySelected = (!pbSearch->needSelected);
	anyVisible = !needVisible;
	anySystem = (!pbSearch->inSystem);
	anySubtype = (pbSearch->subtype==ANYSUBTYPE);
	
	startL = FindSymRight(doc, pt, needVisible, useJD);

	for (pL = startL; pL; pL = nextL) {

		nextL = goLeft ? LeftLINK(pL) : RightLINK(pL);
		if (!anySystem && SystemTYPE(pL)) break;
		if (!useJD && JustTYPE(pL)==J_D) continue;
		pType = ObjLType(pL);
		pSel = LinkSEL(pL);
		pVis = LinkVIS(pL);
		p = GetPMEVENT(pL);

		if ((anyType || pType==type) && (anySelected || pSel) &&
			 (anyVisible || pVis)) {

			beyond = goLeft ? LinkOBJRECT(pL).right < pt.h : LinkOBJRECT(pL).left > pt.h;
							 	 	
			switch (pType) {
				case CLEFtype:
				case KEYSIGtype:
				case TIMESIGtype:
				case DYNAMtype:
				case MEASUREtype:
				case PSMEAStype:
					if (beyond) {
						tmpHeap = Heap + p->type;		/* p may not stay valid during loop */
							
						for (i = 0, subObjL=FirstSubObjPtr(p,pL); subObjL; 
								i++, subObjL = NextLink(tmpHeap,subObjL)) {
							subObj = (GenSubObj *)LinkToPtr(tmpHeap,subObjL);
							if (anyStaff || subObj->staffn==pbSearch->id)
								if (anySelected || subObj->selected) {
									pbSearch->pEntry = subObjL;
									pbSearch->entryNum = i;
				  					return pL;
				  				}
				  		}
				  	}
					break;
				case SYNCtype:				
					if (beyond) {
						aNoteL = FirstSubLINK(pL);
						for (i = 0; aNoteL; i++, aNoteL=NextNOTEL(aNoteL))
							if (anyStaff || NoteSTAFF(aNoteL)==pbSearch->id)
								if (anyVoice || NoteVOICE(aNoteL)==pbSearch->voice)
									if (anySelected || NoteSEL(aNoteL)) {
										pbSearch->pEntry = aNoteL;
										pbSearch->entryNum = i;
					  					return pL;
					  				}
					}
					break;
				case GRSYNCtype:				
					if (beyond) {
						aGRNoteL = FirstSubLINK(pL);
						for (i = 0; aGRNoteL; i++, aGRNoteL=NextGRNOTEL(aGRNoteL))
							if (anyStaff || GRNoteSTAFF(aGRNoteL)==pbSearch->id)
								if (anyVoice || GRNoteVOICE(aGRNoteL)==pbSearch->voice)
									if (anySelected || GRNoteSEL(aGRNoteL)) {
										pbSearch->pEntry = aGRNoteL;
										pbSearch->entryNum = i;
					  					return pL;
					  				}
					}
					break;
				case RPTENDtype:
					if (beyond) {
						aRptL = FirstSubLINK(pL);
						for ( ; aRptL; aRptL = NextRPTENDL(aRptL))
			  				if (anyStaff || RptEndSTAFF(aRptL)==pbSearch->id)
								if (anyVoice)
									if (anySelected || pSel)
			  							return pL;
		  			}
					break;
				case BEAMSETtype:
					if (beyond) {
		  				if (anyStaff || ((PBEAMSET)p)->staffn==pbSearch->id)
							if (anyVoice || ((PBEAMSET)p)->voice==pbSearch->voice)
								if (anySelected || pSel)
		  							return pL;
		  			}
					break;
				case TUPLETtype:
					if (beyond) {
		  				if (anyStaff || ((PTUPLET)p)->staffn==pbSearch->id)
							if (anyVoice || ((PTUPLET)p)->voice==pbSearch->voice)
								if (anySelected || pSel)
		  							return pL;
		  			}
					break;
				case OCTAVAtype:
					if (beyond) {
		  				if (anyStaff || ((POCTAVA)p)->staffn==pbSearch->id)
							if (anySelected || pSel)
		  						return pL;
		  			}
					break;
				case ENDINGtype:
					if (beyond) {
		  				if (anyStaff || ((PENDING)p)->staffn==pbSearch->id)
							if (anySelected || pSel)
		  						return pL;
		  			}
					break;
				case GRAPHICtype:
					if (beyond) {
	  					if (anyStaff || ((PGRAPHIC)p)->staffn==pbSearch->id)
							if (anySelected || pSel)
		  						return pL;
		  			}
					break;
				case TEMPOtype:
					if (beyond) {
		  				if (anyStaff || ((PTEMPO)p)->staffn==pbSearch->id)
							if (anySelected || pSel)
		  						return pL;
		  			}
					break;
				case SPACEtype:
					if (beyond) {
		  				if (anyStaff || ((PSPACE)p)->staffn==pbSearch->id)
							if (anySelected || pSel)
		  						return pL;
		  			}
					break;
				case SLURtype:
					if (beyond) {
		  				if (anyStaff || ((PSLUR)p)->staffn==pbSearch->id)
							if (anyVoice || ((PSLUR)p)->voice==pbSearch->voice)
		  						if (anySubtype || ((PSLUR)p)->tie==pbSearch->subtype)
		  							return pL;
		  			}
					break;
				case CONNECTtype:
					return pL;
				case PAGEtype:
				case SYSTEMtype:
				case STAFFtype:
				case TAILtype:
					break;
				default:
					MayErrMsg("GSearch: unrecognized type %ld", (long)ObjLType(pL));
			}
		}
	}
	
	return NILINK;														/* Nothing found */
}


/* -------------------------------------------------------------------- GSSearch -- */

LINK GSSearch(
			Document *doc,
			Point		pt,				/* x position from which to search */
			INT16		type,				/* target type (or ANYTYPE) */
			INT16		id,				/* target staff number (for Staff, Measure, Sync, etc) */
											/* or page number (for Page) */
			Boolean	goLeft,			/* TRUE if we should search left */
			Boolean	needSelected,	/* TRUE if we only want selected items */
			Boolean	useJD,			/* TRUE if we should find type JD symbols */
			Boolean	needVisible		/* TRUE if only want visible symbols. */
			)
{
	SearchParam	pbSearch;
	LINK			foundL;

	InitSearchParam(&pbSearch);
	pbSearch.id = id;
	pbSearch.needSelected = needSelected;
	pbSearch.inSystem = TRUE;
	pbSearch.subtype = ANYSUBTYPE;
	pbSearch.voice = ANYONE;
	foundL = GSearch(doc, pt, type, goLeft, useJD, needVisible, &pbSearch);
	return foundL;
}


#ifdef NOTYET
/* -------------------------------------------------------------------- LJSearch -- */
/* Logical-order search for given justification type. */

LINK LJSearch(
			LINK		startL,				/* Place to start looking */
			INT16		justType,			/* target type (or ANYTYPE) */
			INT16		id,					/* target staff number */
			Boolean	goLeft,				/* TRUE if we should search left */
			Boolean	needSelected 		/* TRUE if we only want selected items */
			)
{
	SearchParam	pbSearch;
	LINK			foundL;

	InitSearchParam(&pbSearch);
	pbSearch.id = ANYONE;
	pbSearch.needSelected = needSelected;
	pbSearch.inSystem = FALSE;
	pbSearch.subtype = ANYSUBTYPE;
	pbSearch.voice = id;
	pbSearch.needInMeasure = FALSE;
	foundL = J_Search(startL, type, goLeft, &pbSearch);
	return foundL;
}
#endif


/* ----------------------------------------------------------- StaffFindSymRight -- */

LINK StaffFindSymRight(Document *doc, Point pt, Boolean needVisible, INT16 staffn)
{
	LINK pL;
	
	pL = FindSymRight(doc, pt, needVisible, FALSE);
	return (pL ? LSSearch(pL, ANYTYPE, staffn, FALSE, FALSE) : NILINK);
}

/* ---------------------------------------------------------- StaffFindSyncRight -- */

LINK StaffFindSyncRight(Document *doc, Point pt, Boolean needVisible, INT16 staffn)
{
	LINK pL;
	
	pL = FindSyncRight(doc, pt, needVisible);
	return (pL ? LSSearch(pL, SYNCtype, staffn, FALSE, FALSE) : NILINK);
}


/* ----------------------------------------------------------- FindValidSymRight -- */
/* ??It's hard to believe the <if> statement is correct: surely it wants to skip
ALL J_D objects, including tuplets, not just the listed ones! */

LINK FindValidSymRight(Document *doc, LINK symRight, INT16 staffn)
{
	LINK pL;
	
	for (pL = symRight;
		  pL!=doc->tailL && !SystemTYPE(pL) && !PageTYPE(pL); 
		  pL = LSSearch(RightLINK(pL), ANYTYPE, staffn, FALSE, FALSE))
		if (!BeamsetTYPE(pL) && !SlurTYPE(pL) && !DynamTYPE(pL)) 
			break;
	return pL;
}


/* ------------------------------------------------------------- StaffFindSymLeft -- */

LINK StaffFindSymLeft(Document *doc, Point pt, Boolean needVisible, INT16 staffn)
{
	LINK pL;
	
	pL = FindSymRight(doc, pt, needVisible, FALSE);
	return (pL ? LSSearch(LeftLINK(pL), ANYTYPE, staffn, TRUE, FALSE) :
					 NILINK);
}


/* ----------------------------------------------------------- StaffFindSyncLeft -- */
/* Return first sync whose objRect.left is to the left of pt.h.	*/

LINK StaffFindSyncLeft(Document *doc, Point pt, Boolean needVisible, INT16 staffn)
{
	LINK pL;
	
	/* Find first sync whose objRect.left is to the left of pt.h.	*/
	pL = FindSyncLeft(doc, pt, needVisible);

	/* If pL is a sync with subObj on staffn, LSSearch will find pL itself,
		else the first sync with subObj on staffn to the left of pL. */

	return (pL ? LSSearch(pL, SYNCtype, staffn, TRUE, FALSE) : NILINK);
}


/* ------------------------------------------------------------ FindValidSymLeft -- */

LINK FindValidSymLeft(Document *doc, LINK symLeft, INT16 staffn)
{
	LINK qL;
	
	for (qL = symLeft;
		  qL!=doc->headL && !SystemTYPE(qL);
		  qL = LSSearch(LeftLINK(qL), ANYTYPE, staffn, TRUE, FALSE))
		if (!J_DTYPE(qL))
			break;
	return qL;
}


/* ------------------------------------------------------------- SyncInVoiceMeas -- */
/* Look for a Sync in voice <voice> in <objL>'s Measure. */

LINK SyncInVoiceMeas(Document *doc,
							LINK objL,
							INT16 voice,
							Boolean other)	/* TRUE=need a Sync other than <objL> itself */
{
	LINK		pL, endL;
	
	pL = LSSearch(objL, MEASUREtype, ANYONE, TRUE, FALSE);
	endL = EndMeasSearch(doc, objL);
	for ( ; pL!=endL; pL = RightLINK(pL))
		if (SyncTYPE(pL))
			if (SyncInVoice(pL, voice) && (pL!=objL || !other))
				return pL;
		
	return NILINK;
}


/* --------------------------------------------------------------- EndMeasSearch -- */
/* EndMeasSearch searches forward for the object that terminates the Measure
startL is in. The object is the next following Measure, System, or Page, or,
if none of those is found, tailL. Does not assume cross-links are valid. */

LINK EndMeasSearch(
			Document *doc,
			LINK startL)		/* Start search at the link to the right of startL */
{
	LINK	pL;
	
	if (startL==doc->tailL) return doc->tailL;

	for (pL = RightLINK(startL); pL!=doc->tailL; pL = RightLINK(pL))
		if (MeasureTYPE(pL) || SystemTYPE(pL) || PageTYPE(pL)) 
			return pL;
	return doc->tailL;
}


/* ------------------------------------------------------------- EndSystemSearch -- */
/* EndSystemSearch searches forward for the object that terminates the System
startL is in, or, if startL is a System, for the object that terminates startL.
The object is the next following System or Page, or, if neither of those is
found, tailL. Does not assume cross-links are valid. */

LINK EndSystemSearch(
			Document *doc,
			LINK startL)		/* Start search at the link to the right of startL */
{
	LINK	pL;
	
	if (startL==doc->tailL) return doc->tailL;

	for (pL = RightLINK(startL); pL!=doc->tailL; pL = RightLINK(pL))
		if (SystemTYPE(pL) || PageTYPE(pL)) 
			return pL;
	return doc->tailL;
}


/* --------------------------------------------------------------- EndPageSearch -- */
/* EndPageSearch searches forward for the object that terminates the Page
startL is in. The object is the next following Page, or, if none is found,
tailL. */

LINK EndPageSearch(
			Document *doc,
			LINK startL)		/* Start search at the link to the right of startL */
{
	LINK	pL;
	
	if (startL==doc->tailL) return doc->tailL;

	pL = LSSearch(RightLINK(startL), PAGEtype, ANYONE, FALSE, FALSE);
	return (pL ? pL : doc->tailL);
}


/* --------------------------------------------------------------TimeSearchRight -- */
/* Search object list to right, within the Measure ONLY, for an object with the
given type and Measure-relative time. */

LINK TimeSearchRight(
			Document		*doc,
			LINK			startL,		/* Place to start looking */
			INT16			type,			/* object type needed or ANYTYPE */
			long			lTime,		/* Logical time */
			Boolean		exact 		/* TRUE means need time=lTime, else take any time>=lTime */
			)
{
	LINK pL; long time;

	if (type==MEASUREtype)
		{ MayErrMsg("TimeSearchRight: MEASUREtype illegal."); return NILINK; }

	if (type==ANYTYPE) {
		for (pL = startL; pL!=NILINK; pL = RightLINK(pL)) {		/* Go till end obj.list or measure */
			if (PageTYPE(pL) || SystemTYPE(pL) || MeasureTYPE(pL)
			||  HeaderTYPE(pL) || TailTYPE(pL))
				 break;
			time = GetLTime(doc, pL);
			if (time>=lTime)
				 break;
			}
		return ((time==lTime || !exact)? pL : NILINK);
	}
	else {
		for (pL = startL; pL!=NILINK; pL = RightLINK(pL)) {			/* Go till end obj.list or measure */
			if (PageTYPE(pL) || SystemTYPE(pL) || MeasureTYPE(pL))
				return NILINK;
			time = GetLTime(doc, pL);
			if (ObjLType(pL)==type && time>=lTime)
				break;
		}
		return ((time==lTime || !exact)? pL : NILINK);
	}
}


/* ------------------------------------------------------------AbsTimeSearchRight -- */
/* Search object list to right for a Sync with given minimum logical time; if we don't
find one, return NILINK. */

LINK AbsTimeSearchRight(
			Document */*doc*/,
			LINK startL,			/* Place to start looking */
			long lTime				/* Logical time threshhold */
			)
{
	LINK pL;
	
	for (pL = startL; pL!=NILINK; pL = RightLINK(pL)) {
		/* This loop could be much more efficient if we avoided SyncAbsTime. */ 
		if (SyncTYPE(pL)) {
			if (SyncAbsTime(pL)>=lTime)
			return pL;
		}
	}

	return NILINK;
}


/* -------------------------------------------------------------------- MNSearch -- */
/* Search for a Measure with measure number (if goLeft) less than or equal, or
(if !goLeft) greater than or equal, to the given number. If measNum==ANYONE, accept
any Measure regardless of number. If <preferNonFake>, first look for a Measure with
appropriate no. that is not a "fake" measure, but if none is found, accept one
that is a fake measure.

Return the Measure if it exists, else NILINK. Not optimized. */

LINK MNSearch(
			Document *doc,
			LINK startL,
			INT16 measNum,						/* Measure no. or ANYONE */
			Boolean goLeft,
			Boolean preferNonFake
			)
{
	LINK	pL,endL,aMeasL,foundL; PAMEASURE aMeasure;
	
	endL = goLeft ? doc->headL : doc->tailL;

	foundL = NILINK;
	for (pL = startL; pL!=endL; pL = GetNext(pL,goLeft))
		if (MeasureTYPE(pL)) {
			aMeasL = FirstSubLINK(pL);
			for ( ; aMeasL; aMeasL = NextMEASUREL(aMeasL)) {
				aMeasure = GetPAMEASURE(aMeasL);
				if (measNum==ANYONE
				|| (goLeft? aMeasure->measureNum<=measNum
				    : aMeasure->measureNum>=measNum) ) {
					if (!preferNonFake || !MeasISFAKE(pL)) return pL;
					if (!foundL) foundL = pL;
				}
			}
		}

	/*
	 *	If we get here, we didn't find a non-fake measure that matches, so return
	 *	the first fake measure that matches, if any.
	 */
	
	return foundL;
}


/* -------------------------------------------------------------------- RMSearch -- */
/* Search for a rehearsal mark with char string rMark. Return the rehearsal
mark if it exists, else NILINK. */

LINK RMSearch(Document *doc, LINK startL, const unsigned char *rMark, Boolean goLeft)
{
	LINK	pL,endL; PGRAPHIC pGraphic;
	
	endL = goLeft ? doc->headL : doc->tailL;

	for (pL = startL; pL!=endL; pL = GetNext(pL,goLeft))
		if (GraphicTYPE(pL)) {
			pGraphic = GetPGRAPHIC(pL);
			if (pGraphic->graphicType==GRRehearsal) {
				if (rMark[0]) {
					if (PStrCmp((StringPtr)PCopy(GetPAGRAPHIC(FirstSubLINK(pL))->string),
									(StringPtr)rMark))
						return pL;
				}
				else return pL;
			}
		}
	return NILINK;
}


/* --------------------------------------------------------------------- SSearch -- */
/* Non-optimized simple search. For use where we are not guaranteed validity of
crossLinks of structural objects, and do not need to specify which staff, voice,
system or page to search on nor to specify selected status: e.g. ANYONE,
!needSelected. */

LINK SSearch(LINK pL, INT16 type, Boolean goLeft)
{
	LINK qL;

	for (qL=pL; qL; qL=GetNext(qL, goLeft))
		if (ObjLType(qL)==type)
			return qL;

	return NILINK;
}


/* ---------------------------------------------------------------- EitherSearch -- */
/* EitherSearch is the same as LSSearch except that, if it doesn't find anything
in the <goLeft> direction, it looks in the other direction.

??Something wierd here: replacing the old THINK C declaration with the one in ISO
style, calls to this generate different code! But surely not a significant
difference--I think. */

LINK EitherSearch(LINK pL,INT16 type,INT16 id,Boolean goLeft,Boolean needSel)
{
	LINK qL;

	qL = LSSearch(pL,type,id,goLeft,needSel);
	return (qL ? qL : LSSearch(pL,type,id,!goLeft,needSel));
}


/* -------------------------------------------------------------- FindUnknownDur -- */
/* Find the first Sync containing any unknown dur. notes starting at the given
LINK. Returns the Sync's LINK or NILINK. */

LINK FindUnknownDur(LINK pL, Boolean goLeft)
{
	LINK aNoteL;
		
	for ( ; pL; pL = (goLeft? LeftLINK(pL) : RightLINK(pL)) )
		if (ObjLType(pL)==SYNCtype) {
			aNoteL = FirstSubLINK(pL);
			for ( ; aNoteL; aNoteL=NextNOTEL(aNoteL))
				if (NoteType(aNoteL)==UNKNOWN_L_DUR) return pL;
	}
	
	return NILINK;
}

/* --------------------------------------------------------------- XSysSlurMatch -- */
/* Given either piece of a cross-system Slur (either tie set or slur), find its
other piece. If there is none or the Slur isn't cross-system, return NILINK. */

#define KIND_WANTED(link)	(SlurCrossSYS(link) && SlurTIE(link)==slurTie)

LINK XSysSlurMatch(LINK slurL)
{
	Boolean slurTie, firstPiece; INT16 voice; LINK otherSlurL;
	
	if (!SlurCrossSYS(slurL)) return NILINK;
	
	slurTie = SlurTIE(slurL);
	voice = SlurVOICE(slurL);

	firstPiece = SlurFirstSYSTEM(slurL);
	if (firstPiece) {
		otherSlurL = LVSearch(RightLINK(slurL), SLURtype, voice, GO_RIGHT, FALSE);
		if (otherSlurL && !KIND_WANTED(otherSlurL))
			while (otherSlurL && !KIND_WANTED(otherSlurL))
				otherSlurL = LVSearch(RightLINK(otherSlurL), SLURtype, voice,
												GO_RIGHT, FALSE);
	}
	else {
		otherSlurL = LVSearch(LeftLINK(slurL), SLURtype, voice, GO_LEFT, FALSE);
		if (otherSlurL && !KIND_WANTED(otherSlurL))
			while (otherSlurL && !KIND_WANTED(otherSlurL))
				otherSlurL = LVSearch(LeftLINK(otherSlurL), SLURtype, voice,
												GO_LEFT, FALSE);
	}
	
	return otherSlurL;
}

/* -------------------------------------------------------------- LeftSlurSearch -- */
/* Finds the slur in voice v which ends at syncL. syncL must be a Sync. If the notes
in syncL in voice v are tiedR, their slur will be in voice v and located prior to
syncL in the object list. Therefore require either checking the mainNote's tiedR
flag, and searching for the 2nd slur to the left of syncL if the mainNote in v is
tiedR, or checking the lastSyncL field of the first slur to the left in v, and
searching for the 2nd if the first's lastSyncL is not syncL. If we ever allow nesting,
this will require review. */

LINK LeftSlurSearch(
			LINK syncL,
			INT16 v,
			Boolean isTie)	/* TRUE, FALSE, or ANYSUBTYPE (??ugh--need enum for tie/slur to clean up!) */
{
	SearchParam pbSearch; LINK slurL;

	InitSearchParam(&pbSearch);
	pbSearch.voice = v;
	pbSearch.subtype = isTie;
	slurL = L_Search(syncL, SLURtype, GO_LEFT, &pbSearch);

	if (SlurLASTSYNC(slurL)==syncL) return slurL;
	slurL = L_Search(LeftLINK(slurL), SLURtype, GO_LEFT, &pbSearch);
		
	return slurL;
}


/* -------------------------------------------------------------------- KSSearch -- */
/* Search right for the next <inMeas>, <needSel> keysig on staffn. If staffn is
ANYONE, search right for the most remote keysig on any staff. If the staff (or any
of the staves) in question doesn't have a keysig following, return doc->tailL. Thus,
in all cases, the object returned is the first that cannot be affected by a change
in keysig at <pL>; this is useful to determine the limit of context propagation. */

LINK KSSearch(
		Document *doc,
		LINK pL,
		INT16 staffn,						/* Staff no. or ANYONE */
		Boolean inMeas,
		Boolean needSel					/* Find selected keysigs only? */
		)
{
	SearchParam pbSearch; LINK ksL,lastksL; INT16 s;

	InitSearchParam(&pbSearch);
	pbSearch.voice = ANYONE;
	pbSearch.needInMeasure = inMeas;
	pbSearch.needSelected = needSel;
	pbSearch.id = staffn;
	if (pbSearch.id!=ANYONE) {
		ksL = L_Search(pL, KEYSIGtype, GO_RIGHT, &pbSearch);
		return (ksL? ksL : doc->tailL);
	}

	pbSearch.id = 1;
	lastksL = ksL = L_Search(pL, KEYSIGtype, GO_RIGHT, &pbSearch);
	if (!lastksL) return doc->tailL;
	
	for (s = 2; ksL && s<=doc->nstaves; s++) {
		pbSearch.id = s;
		ksL = L_Search(pL, KEYSIGtype, GO_RIGHT, &pbSearch);
		if (!ksL) return doc->tailL;
		if (IsAfter(lastksL,ksL)) lastksL = ksL;
	}
	
	return lastksL;
}


/* ------------------------------------------------------------------ ClefSearch -- */
/* Search right for the next <inMeas>, <needSel> clef on staffn. If staffn is
ANYONE, search right for the most remote clef on any staff. If the staff (or any
of the staves) in question doesn't have a clef following, return doc->tailL. Thus,
in all cases, the object returned is the first that cannot be affected by a change
in clef at <pL>; this is useful to determine the limit of context propagation. */

LINK ClefSearch(
		Document *doc,
		LINK pL,
		INT16 staffn,						/* Staff no. or ANYONE */
		Boolean inMeas,
		Boolean needSel					/* Find selected clefs only? */
		)
{
	SearchParam pbSearch; LINK clefL,lastClefL; INT16 s;

	InitSearchParam(&pbSearch);
	pbSearch.voice = ANYONE;
	pbSearch.needInMeasure = inMeas;
	pbSearch.needSelected = needSel;
	pbSearch.id = staffn;
	if (pbSearch.id!=ANYONE) {
		clefL = L_Search(pL, CLEFtype, GO_RIGHT, &pbSearch);
		return (clefL? clefL : doc->tailL);
	}

	pbSearch.id = 1;
	lastClefL = clefL = L_Search(pL, CLEFtype, GO_RIGHT, &pbSearch);
	if (!lastClefL) return doc->tailL;
	
	for (s = 2; clefL && s<=doc->nstaves; s++) {
		pbSearch.id = s;
		clefL = L_Search(pL, CLEFtype, GO_RIGHT, &pbSearch);
		if (!clefL) return doc->tailL;
		if (IsAfter(lastClefL,clefL)) lastClefL = clefL;
	}
	
	return lastClefL;
}


/* ------------------------------------------------------------------ FindNoteNum -- */
/* Look for a note with the given note number in the given Sync and voice and return
the first one found; if there is none, return NILINK. */

LINK FindNoteNum(
		LINK syncL,
		INT16 voice,			/* Voice no. or ANYONE */
		INT16 noteNum
		)
{
	LINK aNoteL; Boolean anyVoice;
	
	anyVoice = (voice==ANYONE);
	aNoteL = FirstSubLINK(syncL);
	for ( ; aNoteL; aNoteL = NextNOTEL(aNoteL))
		if ((anyVoice || NoteVOICE(aNoteL)==voice) && NoteNUM(aNoteL)==noteNum)
			return aNoteL;
			
	return NILINK;
}
