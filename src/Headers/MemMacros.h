/***************************************************************************
*	FILE:	MemMacros.h
*	PROJ:	Nightingale
*	DESC:	#defines for object/heap memory management
***************************************************************************/

#ifndef MemMacrosIncluded
#define MemMacrosIncluded

#define NILINK ((LINK)0)

/*
 *	These macros are to lock a given heap's data at its location in memory temporarily;
 *  they should be used very sparingly! The first set of macros cannot be intermingled
 *	with the second, since the second set are designed to be nestable and the first
 *	aren't.
 *
 *	In the first set, the condition being maintained is that if heap->lockLevel is
 *	greater than 0, then the heap is assumed to be locked; if 0, then it's unlocked.
 */

#define PushLock(heap)	{ if ( (heap)->lockLevel++ <= 0) HLock((heap)->block); }
#define PopLock(heap)	{ if (--(heap)->lockLevel <= 0) HUnlock((heap)->block); }

#define PushLockHi(heap) { if ( (heap)->lockLevel++ <= 0) \
								{ MoveHHi((heap)->block); HLock((heap)->block); } }

/*
 *  The second set, HeapLock and HeapUnlock, also lock and unlock the block of
 *	the given heap. They leave the lockLevel alone, assuming that they will be
 *	called in pairs with no possibility of calls to PushLock or PopLock in between.	
 */
 
#define HeapLock(heap) 		HLock((heap)->block)
#define HeapUnlock(heap)	HUnlock((heap)->block)

/* ----------------------------------------------------------------------------------- */
/*
 *	LinkToPtr(heap,link) delivers the address of the 0'th byte of the link'th
 *	object kept in a given heap.  This address is determined without type information
 *	by using the heap's own idea of how large the object is in bytes. The pointer
 *	The pointer so delivered is only valid as long as the heap block doesn't get
 *	relocated! This is a generic macro that should be avoided whenever it's possible
 *	to use one of the ones below. NB: For space and time efficiency reasons, we now
 *	use an equivalent function, located in Heaps.c. Long ago, we used an identical
 *	macro with its name slightly changed in the THINK Debugger. (I don't know if
 *	it'd be useful with a modern debugger. --DAB, Feb. 2017) See the end of this file.
 *
 *	PtrToLink(heap, ptr) delivers the LINK into a heap that a given pointer to an
 *	object corresponds to.  Since this is done generically, there's no reasonable
 *	way to avoid the divide (unless you know that heap->objSize is a power of 2).
 *	However, it rarely needs to be used--in fact, in Nightingale 1.0 thru 5.7 (at
 *	least), it's not used at all.
 */

#ifndef LinkToPtrFUNCTION	/* REPLACED BY EQUIVALENT FUNCTION */
#define LinkToPtr(heap,link) ( ((char *)(*(heap)->block)) + ((heap)->objSize*(unsigned long)(link)) )
#endif

#define PtrToLink(heap,p)	( (((char *)p) - (char *)(*(heap)->block)) / (heap)->objSize )

/*
 *	Given a link to a generic subobject list, deliver the link to the next sub-
 *	object in list. This macro depends upon the fact that the next link field of
 *	a subobject header is the first in the record and thus has the same address
 *	as the subobject record itself.
 */

#define NextLink(heap,link)  ( *(LINK *)LinkToPtr(heap,link) )

/*
 *	Given a link to an object, deliver the links to the objects to its right or
 *	left. These macros depend (like above) on the right link being the first, and
 *	the left link being the second, fields in the object record.  Similarly, the
 *	macros to deliver the link to the first subobject and to the object xd depend
 *	on those values being respectively the third and fourth fields.
 */
 
#define RightLINK(link)		( *(LINK *)LinkToPtr(OBJheap,link) )
#define LeftLINK(link)		( *(LINK *)(sizeof(LINK) + LinkToPtr(OBJheap,link)) )
#define FirstSubLINK(link)	( *(LINK *)((2*sizeof(LINK)) + LinkToPtr(OBJheap,link)) )
#define LinkXD(link)		( *(DDIST *)((3*sizeof(LINK)) + LinkToPtr(OBJheap,link)) )
#define LinkYD(link)		( *(DDIST *)((4*sizeof(LINK)) + LinkToPtr(OBJheap,link)) )

/*
 * Get the staff number of (depending on object type) an object or subject, or the
 * voice number of an object or subobject. For subobjects, the staffn field is in the
 * same place as ->left for objs, but staffn is a SignedByte.
 */
 
#define StaffSTAFF(link)	( *(SignedByte *)(sizeof(LINK) + LinkToPtr(STAFFheap,link)) )
#define NoteSTAFF(link) 	( *(SignedByte *)(sizeof(LINK) + LinkToPtr(NOTEheap,link)) )
#define GRNoteSTAFF(link) 	( *(SignedByte *)(sizeof(LINK) + LinkToPtr(GRNOTEheap,link)) )
#define ClefSTAFF(link)		( *(SignedByte *)(sizeof(LINK) + LinkToPtr(CLEFheap,link)) )
#define KeySigSTAFF(link)	( *(SignedByte *)(sizeof(LINK) + LinkToPtr(KEYSIGheap,link)) )
#define TimeSigSTAFF(link)	( *(SignedByte *)(sizeof(LINK) + LinkToPtr(TIMESIGheap,link)) )
#define MeasureSTAFF(link)	( *(SignedByte *)(sizeof(LINK) + LinkToPtr(MEASUREheap,link)) )
#define PSMeasSTAFF(link)	( *(SignedByte *)(sizeof(LINK) + LinkToPtr(PSMEASheap,link)) )
#define DynamicSTAFF(link)	( *(SignedByte *)(sizeof(LINK) + LinkToPtr(DYNAMheap,link)) )
#define RptEndSTAFF(link) 	( *(SignedByte *)(sizeof(LINK) + LinkToPtr(RPTENDheap,link)) )

#define BeamSTAFF(link)		( (GetPBEAMSET(link))->staffn )
#define TupletSTAFF(link)	( (GetPTUPLET(link))->staffn )
#define GraphicSTAFF(link)	( (GetPGRAPHIC(link))->staffn )
#define TempoSTAFF(link)	( (GetPTEMPO(link))->staffn )
#define SpacerSTAFF(link)	( (GetPSPACER(link))->staffn )
#define OttavaSTAFF(link)	( (GetPOTTAVA(link))->staffn )
#define SlurSTAFF(link)		( (GetPSLUR(link))->staffn )
#define EndingSTAFF(link) 	( (GetPENDING(link))->staffn )

#define PartFirstSTAFF(link)	( GetPPARTINFO(link)->firstStaff )
#define PartLastSTAFF(link)	( GetPPARTINFO(link)->lastStaff )

#define BeamVOICE(link)		( (GetPBEAMSET(link))->voice )
#define TupletVOICE(link)	( (GetPTUPLET(link))->voice )
#define GraphicVOICE(link)	( (GetPGRAPHIC(link))->voice )
#define NoteVOICE(link) 	( (GetPANOTE(link))->voice )
#define GRNoteVOICE(link) 	( (GetPAGRNOTE(link))->voice )
#define SlurVOICE(link) 	( (GetPSLUR(link))->voice )

/* Get various other fields that are common among different types of objects. */

#define StaffSEL(link)		( (GetPASTAFF(link))->selected )
#define ConnectSEL(link)	( (GetPACONNECT(link))->selected )
#define NoteSEL(link)		( (GetPANOTE(link))->selected )
#define GRNoteSEL(link)		( (GetPAGRNOTE(link))->selected )
#define ClefSEL(link)		( (GetPACLEF(link))->selected )
#define KeySigSEL(link)		( (GetPAKEYSIG(link))->selected )
#define TimeSigSEL(link)	( (GetPATIMESIG(link))->selected )
#define MeasureSEL(link)	( (GetPAMEASURE(link))->selected )
#define PSMeasSEL(link)		( (GetPAPSMEAS(link))->selected )
#define DynamicSEL(link)	( (GetPADYNAMIC(link))->selected )
#define RptEndSEL(link)		( (GetPARPTEND(link))->selected )
#define SlurSEL(link)		( (GetPASLUR(link))->selected )

#define StaffVIS(link)		( (GetPASTAFF(link))->visible )
#define ClefVIS(link)		( (GetPACLEF(link))->visible )
#define KeySigVIS(link)		( (GetPAKEYSIG(link))->visible )
#define TimeSigVIS(link)	( (GetPATIMESIG(link))->visible )
#define MeasureVIS(link)	( (GetPAMEASURE(link))->visible )
#define PSMeasVIS(link)		( (GetPAPSMEAS(link))->visible )

#define PSMeasType(link)	( (GetPAPSMEAS(link))->subType )
#define DynamType(link) 	( (GetPDYNAMIC(link))->dynamicType )
#define RptType(link) 		( (GetPRPTEND(link))->subType )
#define OctType(link)		( (GetPOTTAVA(link))->octSignType )
#define ClefType(link)		( (GetPACLEF(link))->subType )
#define NoteType(link)		( (GetPANOTE(link))->subType )
#define GRNoteType(link)	( (GetPAGRNOTE(link))->subType )
#define GraphicSubType(link)	( (GetPGRAPHIC(link))->graphicType )

/* ----------------------------------------------------------------------------------- */

#define SyncTIME(link)		( (GetPSYNC(link))->timeStamp )

#define PartNAME(link)		( GetPPARTINFO(link)->name )

/* ----------------------------------------------------------------------------------- */
/*
 *	Our main data structure, the object list, is a doubly-linked list of objects,
 *	each of which can have a singly-linked list of subobjects dangling from it. 
 *	The objects in the backbone can have different types, but are all the same size:
 *	that of a SUPEROBJECT, which is the union of all objects.  The subobjects in
 *	each object's list are all of the same type, which depends on the owning
 *	object's type.  All lists are linked via LINKs (indices into the respective
 *	heaps). We have two macros for converting a link to the pointer to its sub/object,
 *	one to access objects from the backbone list, and the other for subobjects.
 */

/* GetSubObjectPtr is used like this:
 *		aConnectPtr = GetSubObjectPtr(link,ACONNECT);
 *
 *		aConnectPtr = ( ((ACONNECT *)(*ACONNECTheap)->block))+(link) )
 */

#define GetSubObjectPtr(link,type)	( ((type *)(*type/**/Heap)->block))+(link) )

/*
 *	GetObjectPtr(heap,link,cast) delivers a pointer to an object of type 'cast',
 *	from a given heap, using the object's link.  This pointer is only valid as
 *	long as the heap's block doesn't get relocated!  This macro may be slightly
 *	more efficient than using the one above, since the object size is known at
 *	compile time, rather than execution time.
 *	GetObject(heap,link,cast) delivers the actual object data.
 */

#define GetObjectPtr(heap,link,cast)	( ((cast)(*(heap)->block)) + (link) )
#define GetObject(heap,link,cast)  		( ((cast)(*(heap)->block))[link] )

/*	Examples of the above macros:
 *
 *				ASTAFF *astaff;
 *				LINK n;
 *
 *				astaff = GetObjectPtr(heap+STAFFtype,n,ASTAFF);
 *				astaff->field = 41;
 *
 *	Same as above, only delivers actual object data, not just pointer to it:
 *	
 *				ASTAFF astaff;
 *				LINK n;
 *
 *				astaff = GetObject(heap+STAFFtype,n,ASTAFF);
 *				astaff.field = 41;
 */

/*
 *	GetObjectLink(heap,ptr,type) is the reverse of GetObjectPtr() above.  By
 *	forcing the user to specify the type of object, we can use C's knowledge of
 *	the object sizes to do the conversion more efficiently.  The pointer argument
 *	must be valid!
 */

#define GetObjLink(heap,p,cast)	( ((cast *)p) - (cast *)(*(heap)->block) )

/* Given a link, deliver the type of object that it refers to */

#define ObjLType(link)	( ((PMEVENT)GetObjectPtr(OBJheap,link,PSUPEROBJECT))->type )

/* Type-specific versions of the above, for accessing objects in OBJheap */

#define GetPMEVENT(link)	(PMEVENT)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetMEVENT(link)		(MEVENT)GetObject(OBJheap,link,PSUPEROBJECT)
#define GetPHEADER(link)	(PHEADER)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPTAIL(link)		(PTAIL)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPPAGE(link)		(PPAGE)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPSYSTEM(link)	(PSYSTEM)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPSTAFF(link)		(PSTAFF)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPMEASURE(link)	(PMEASURE)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPPSMEAS(link)	(PPSMEAS)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPSYNC(link)		(PSYNC)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPGRSYNC(link)	(PGRSYNC)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPCLEF(link)		(PCLEF)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPKEYSIG(link)	(PKEYSIG)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPTIMESIG(link)	(PTIMESIG)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPBEAMSET(link)	(PBEAMSET)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPCONNECT(link)	(PCONNECT)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPDYNAMIC(link)	(PDYNAMIC)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPRPTEND(link) 	(PRPTEND)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPENDING(link) 	(PENDING)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPGRAPHIC(link)	(PGRAPHIC)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPOTTAVA(link)	(POTTAVA)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPSLUR(link)		(PSLUR)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPTUPLET(link)	(PTUPLET)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPTEMPO(link)		(PTEMPO)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPSPACER(link)	(PSPACER)GetObjectPtr(OBJheap,link,PSUPEROBJECT)
#define GetPEXTEND(link)	(PEXTEND)GetObjectPtr(OBJheap,link,PSUPEROBJECT)

/* Type-specific macros for getting at subobjects in their respective heaps */

#define GetPPARTINFO(link) 	 	GetObjectPtr(PARTINFOheap,link,PPARTINFO)
#define GetPASTAFF(link) 	 	GetObjectPtr(STAFFheap,link,PASTAFF)
#define GetPAMEASURE(link) 	 	GetObjectPtr(MEASUREheap,link,PAMEASURE)
#define GetPAPSMEAS(link) 	 	GetObjectPtr(PSMEASheap,link,PAPSMEAS)
#define GetPANOTE(link) 	 	GetObjectPtr(NOTEheap,link,PANOTE)
#define GetPACLEF(link) 	 	GetObjectPtr(CLEFheap,link,PACLEF)
#define GetPAKEYSIG(link) 	 	GetObjectPtr(KEYSIGheap,link,PAKEYSIG)
#define GetPATIMESIG(link) 	 	GetObjectPtr(TIMESIGheap,link,PATIMESIG)
#define GetPANOTEBEAM(link)  	GetObjectPtr(NOTEBEAMheap,link,PANOTEBEAM)
#define GetPACONNECT(link) 	 	GetObjectPtr(CONNECTheap,link,PACONNECT)
#define GetPADYNAMIC(link) 	 	GetObjectPtr(DYNAMheap,link,PADYNAMIC)
#define GetPAMODNR(link) 	 	GetObjectPtr(MODNRheap,link,PAMODNR)
#define GetPARPTEND(link) 	 	GetObjectPtr(RPTENDheap,link,PARPTEND)
#define GetPAGRAPHIC(link) 	 	GetObjectPtr(GRAPHICheap,link,PAGRAPHIC)
#define GetPASLUR(link)			GetObjectPtr(SLURheap,link,PASLUR)
#define GetPANOTETUPLE(link)	GetObjectPtr(NOTETUPLEheap,link,PANOTETUPLE)
#define GetPANOTEOTTAVA(link)	GetObjectPtr(NOTEOTTAVAheap,link,PANOTEOTTAVA)
#define GetPAGRNOTE(link)		GetObjectPtr(GRNOTEheap,link,PAGRNOTE)
#define GetANOTE(link)			GetObject(NOTEheap,link,PANOTE)
#define GetPARTINFO(link)		GetObject(PARTINFOheap,link,PPARTINFO)
#define GetKEYSIG(link)			GetObject(KEYSIGheap,link,PAKEYSIG)
#define GetTIMESIG(link)		GetObject(TIMESIGheap,link,PATIMESIG)

/* Type-specific macros for getting the next subobject via the next link field */

#define NextPPARTINFO(partInfo)		GetPPARTINFO((partInfo)->next)
#define NextPASTAFF(aStaff)			GetPASTAFF((aStaff)->next)
#define NextPAMEASURE(aMeasure)		GetPAMEASURE((aMeasure)->next)
#define NextPAPSMEAS(aPSMeas)		GetPAPSMEAS((aPSMeas)->next)
#define NextPANOTE(aNote)			GetPANOTE((aNote)->next)
#define NextPAGRNOTE(aNote)			GetPAGRNOTE((aNote)->next)
#define NextPACLEF(aClef)			GetPACLEF((aClef)->next)
#define NextPAKEYSIG(aKeySig)		GetPAKEYSIG((aKeySig)->next)
#define NextPATIMESIG(aTimeSig)		GetPATIMESIG((aTimeSig)->next)
#define NextPANOTEBEAM(aNoteBeam)	GetPANOTEBEAM((aNoteBeam)->next)
#define NextPACONNECT(aConnect)		GetPACONNECT((aConnect)->next)
#define NextPADYNAMIC(aDynamic)		GetPADYNAMIC((aDynamic)->next)
#define NextPASLUR(aSlur)			GetPASLUR((aSlur)->next)
#define NextPANOTETUPLE(aNoteTuple)	GetPANOTETUPLE((aNoteTuple)->next)

#define GetPARTINFOL(partInfo)		GetObjLink(PARTINFOheap,(partInfo),PARTINFO)
#define GetASTAFFL(aStaff)			GetObjLink(STAFFheap,(aStaff),ASTAFF)
#define GetAMEASUREL(aMeasure)		GetObjLink(MEASUREheap,(aMeasure),AMEASURE)
#define GetAPSMEASL(aPSMeas)		GetObjLink(PSMEASheap,(aPSMeas),APSMEAS)
#define GetANOTEL(aNote)			GetObjLink(NOTEheap,(aNote),ANOTE)
#define GetAGRNOTEL(aNote)			GetObjLink(GRNOTEheap,(aNote),ANOTE)
#define GetACLEFL(aClef)			GetObjLink(CLEFheap,(aClef),ACLEF)
#define GetAKEYSIGL(aKeySig)		GetObjLink(KEYSIGheap,(aKeySig),AKEYSIG)
#define GetATIMESIGL(aTimeSig)		GetObjLink(TIMESIGheap,(aTimeSig),ATIMESIG)
#define GetANOTEBEAML(aNoteBeam) 	GetObjLink(NOTEBEAMheap,(aNoteBeam),ANOTEBEAM)
#define GetACONNECTL(aConnect)		GetObjLink(CONNECTheap,(aConnect),ACONNECT)
#define GetADYNAMICL(aDynamic)		GetObjLink(DYNAMheap,(aDynamic),ADYNAMIC)
#define GetAGRAPHICL(aGraphic)		GetObjLink(GRAPHICheap,(aDynamic),ADYNAMIC)
#define GetASLURL(aSlur)			GetObjLink(SLURheap,(aSlur),ASLUR)
#define GetANOTETUPLEL(aNoteTuple)	GetObjLink(NOTETUPLEheap,(aNoteTuple),ANOTETUPLE)

#define NextPARTINFOL(partInfoL) 	NextLink(PARTINFOheap,(partInfoL))
#define NextSTAFFL(aStaffL) 		NextLink(STAFFheap,(aStaffL))
#define NextMEASUREL(aMeasureL) 	NextLink(MEASUREheap,(aMeasureL))
#define NextPSMEASL(aPSMeasL) 		NextLink(PSMEASheap,(aPSMeasL))
#define NextNOTEL(aNoteL) 			NextLink(NOTEheap,(aNoteL))
#define NextGRNOTEL(aNoteL) 		NextLink(GRNOTEheap,(aNoteL))
#define NextCLEFL(aClefL) 			NextLink(CLEFheap,(aClefL))
#define NextKEYSIGL(aKeySigL) 		NextLink(KEYSIGheap,(aKeySigL))
#define NextTIMESIGL(aTimeSigL) 	NextLink(TIMESIGheap,(aTimeSigL))
#define NextNOTEBEAML(aNoteBeamL)	NextLink(NOTEBEAMheap,(aNoteBeamL))
#define NextCONNECTL(aConnectL) 	NextLink(CONNECTheap,(aConnectL))
#define NextDYNAMICL(aDynamicL) 	NextLink(DYNAMheap,(aDynamicL))
#define NextMODNRL(aModNRL) 		NextLink(MODNRheap,(aModNRL))
#define NextRPTENDL(aRptL) 			NextLink(RPTENDheap,(aRptL))
#define NextSLURL(aSlurL) 			NextLink(SLURheap,(aSlurL))
#define NextNOTETUPLEL(aNoteTupleL) NextLink(NOTETUPLEheap,(aNoteTupleL))
#define NextNOTEOTTAVAL(aNoteOttavaL) 	NextLink(NOTEOTTAVAheap,(aNoteOttavaL))

/* Type-specific macros to say if an obj is of that type, two objs are the same type, etc. */

#define HeaderTYPE(link)		( (ObjLType(link)==HEADERtype) )
#define TailTYPE(link)			( (ObjLType(link)==TAILtype) )
#define PageTYPE(link)			( (ObjLType(link)==PAGEtype) )
#define SystemTYPE(link)		( (ObjLType(link)==SYSTEMtype) )
#define StaffTYPE(link)			( (ObjLType(link)==STAFFtype) )
#define ConnectTYPE(link)		( (ObjLType(link)==CONNECTtype) )
#define MeasureTYPE(link)		( (ObjLType(link)==MEASUREtype) )
#define PSMeasTYPE(link)		( (ObjLType(link)==PSMEAStype) )
#define SyncTYPE(link)		 	( (ObjLType(link)==SYNCtype) )
#define GRSyncTYPE(link)		( (ObjLType(link)==GRSYNCtype) )
#define ClefTYPE(link)		 	( (ObjLType(link)==CLEFtype) )
#define KeySigTYPE(link)		( (ObjLType(link)==KEYSIGtype) )
#define TimeSigTYPE(link)		( (ObjLType(link)==TIMESIGtype) )
#define DynamTYPE(link)		 	( (ObjLType(link)==DYNAMtype) )
#define BeamsetTYPE(link)		( (ObjLType(link)==BEAMSETtype) )
#define TupletTYPE(link)		( (ObjLType(link)==TUPLETtype) )
#define OttavaTYPE(link)		( (ObjLType(link)==OTTAVAtype) )
#define SlurTYPE(link)		 	( (ObjLType(link)==SLURtype) )
#define GraphicTYPE(link)		( (ObjLType(link)==GRAPHICtype) )
#define RptEndTYPE(link)		( (ObjLType(link)==RPTENDtype) )
#define SpaceTYPE(link)		 	( (ObjLType(link)==SPACERtype) )
#define TempoTYPE(link)		 	( (ObjLType(link)==TEMPOtype) )
#define EndingTYPE(link)		( (ObjLType(link)==ENDINGtype) )
#define SameTYPE(link1,link2)	( (ObjLType(link1)==ObjLType(link2)) )
#define J_ITTYPE(link)			( JustTYPE(link)==J_IT )
#define J_IPTYPE(link)			( JustTYPE(link)==J_IP )
#define J_DTYPE(link)			( JustTYPE(link)==J_D )
#define J_STRUCTYPE(link)		( JustTYPE(link)==J_STRUC )
#define GenlJ_DTYPE(link)		( GraphicTYPE(pL) || EndingTYPE(pL) || TempoTYPE(pL) )
#define SyncSupportTYPE(link)	( BeamsetTYPE(link) || OttavaTYPE(link) || SlurTYPE(link) || TupletTYPE(link) )

/* Get an object's justification type */

#define JustTYPE(link)			( objTable[ObjLType(link)].justType )

/* These macros take specific Documents as arguments */

#define DGetPMEVENT(doc,link)		(PMEVENT)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)

#define DObjLType(doc,link)			( ((PMEVENT)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT))->type )
#define DFirstSubLINK(doc,link)		( *(LINK *)((2*sizeof(LINK)) + LinkToPtr((doc)->Heap+OBJtype,link)) )

#define DRightLINK(doc,link)		( *(LINK *)LinkToPtr((doc)->Heap+OBJtype,link) )
#define DLeftLINK(doc,link)			( *(LINK *)(sizeof(LINK) + LinkToPtr((doc)->Heap+OBJtype,link)) )

#define DLinkXD(doc,link)			( *(DDIST *)((3*sizeof(LINK)) + LinkToPtr((doc)->Heap+OBJtype,link)) )

#define DGetPPAGE(doc,link)			(PPAGE)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPSYSTEM(doc,link)		(PSYSTEM)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPSTAFF(doc,link)		(PSTAFF)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPMEASURE(doc,link)		(PMEASURE)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPPSMEAS(doc,link)		(PPSMEAS)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPCONNECT(doc,link)		(PCONNECT)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPGRAPHIC(doc,link)		(PGRAPHIC)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPDYNAMIC(doc,link)		(PDYNAMIC)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPTEMPO(doc,link)		(PTEMPO)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPENDING(doc,link)		(PENDING)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)

#define DNextPARTINFOL(doc,aPartL) 			NextLink((doc)->Heap+HEADERtype,(aPartL))
#define DNextSTAFFL(doc,aStaffL) 			NextLink((doc)->Heap+STAFFtype,(aStaffL))
#define DNextCONNECTL(doc,aConnectL) 		NextLink((doc)->Heap+CONNECTtype,(aConnectL))
#define DNextSLURL(doc,aSlurL) 				NextLink((doc)->Heap+SLURtype,(aSlurL))
#define DNextNOTEL(doc,aNoteL) 				NextLink((doc)->Heap+SYNCtype,(aNoteL))
#define DNextGRNOTEL(doc,aGRNoteL) 			NextLink((doc)->Heap+GRSYNCtype,(aGRNoteL))
#define DNextNOTETUPLEL(doc,aNoteTupleL) 	NextLink((doc)->Heap+TUPLETtype,(aNoteTupleL))
#define DNextNOTEOTTAVAL(doc,aNoteOttavaL) 	NextLink((doc)->Heap+OTTAVAtype,(aNoteOttavaL))
#define DNextNOTEBEAML(doc,aNoteBeamL) 		NextLink((doc)->Heap+BEAMSETtype,(aNoteBeamL))
#define DNextMODNRL(doc,aModNRL)		 	NextLink((doc)->Heap+MODNRtype,(aModNRL))

#define DGetPPARTINFO(doc,link) 		GetObjectPtr((doc)->Heap+HEADERtype,link,PPARTINFO)
#define DGetPACONNECT(doc,link) 		GetObjectPtr((doc)->Heap+CONNECTtype,link,PACONNECT)
#define DGetPASTAFF(doc,link)			GetObjectPtr((doc)->Heap+STAFFtype,link,PASTAFF)
#define DGetPANOTE(doc,link)			GetObjectPtr((doc)->Heap+SYNCtype,link,PANOTE)
#define DGetPAGRNOTE(doc,link)			GetObjectPtr((doc)->Heap+GRSYNCtype,link,PAGRNOTE)
#define DGetPASLUR(doc,link) 			GetObjectPtr((doc)->Heap+SLURtype,link,PASLUR)
#define DGetPAGRAPHIC(doc,link) 		GetObjectPtr((doc)->Heap+GRAPHICtype,link,PAGRAPHIC)
#define DGetPANOTETUPLE(doc,link)		GetObjectPtr((doc)->Heap+TUPLETtype,link,PANOTETUPLE)
#define DGetPANOTEOTTAVA(doc,link)		GetObjectPtr((doc)->Heap+OTTAVAtype,link,PANOTEOTTAVA)
#define DGetPANOTEBEAM(doc,link)		GetObjectPtr((doc)->Heap+BEAMSETtype,link,PANOTEBEAM)

#define DLinkYD(doc,link)			 	( (DGetPMEVENT(doc,link))->yd )
#define DLinkSEL(doc,link)			 	( (DGetPMEVENT(doc,link))->selected )
#define DLinkSOFT(doc,link)				( (DGetPMEVENT(doc,link))->soft )
#define DLinkVALID(doc,link)			( (DGetPMEVENT(doc,link))->valid )
#define DLinkVIS(doc,link)			 	( (DGetPMEVENT(doc,link))->visible )
#define DLinkTWEAKED(doc,link)			( (DGetPMEVENT(doc,link))->tweaked )
#define DLinkNENTRIES(doc,link)			( (DGetPMEVENT(doc,link))->nEntries )
#define DLinkOBJRECT(doc,link)			( (DGetPMEVENT(doc,link))->objRect )
#define DLinkSPAREFLAG(doc,link)		( (DGetPMEVENT(doc,link))->spareFlag )
#define DStaffSEL(doc,link)				( (DGetPASTAFF(doc,link))->selected )
#define DConnectSEL(doc,link)			( (DGetPACONNECT(doc,link))->selected )
#define DNoteSEL(doc,link)				( (DGetPANOTE(doc,link))->selected )
#define DGRNoteSEL(doc,link)			( (DGetPAGRNOTE(doc,link))->selected )
#define DSlurSEL(doc,link)				( (DGetPASLUR(doc,link))->selected )

#define DGetPTUPLET(doc,link)		(PTUPLET)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPOTTAVA(doc,link)		(POTTAVA)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)
#define DGetPBEAMSET(doc,link)		(PBEAMSET)GetObjectPtr((doc)->Heap+OBJtype,link,PSUPEROBJECT)

#define DTupletSTAFF(doc,link)		( (DGetPTUPLET(doc,link))->staffn )
#define DOttavaSTAFF(doc,link)		( (DGetPOTTAVA(doc,link))->staffn )
#define DNoteSTAFF(doc,link) 		( (DGetPANOTE(doc,link))->staffn )

#define DBeamVOICE(doc,link)		( (DGetPBEAMSET(doc,link))->voice )
#define DTupletVOICE(doc,link)		( (DGetPTUPLET(doc,link))->voice )
#define DNoteVOICE(doc,link) 		( (DGetPANOTE(doc,link))->voice )
#define DGRNoteVOICE(doc,link)		( (DGetPAGRNOTE(doc,link))->voice )

#define DGraceBEAM(doc,link) 		( (DGetPBEAMSET(doc,link))->grace )

#define DSyncTYPE(doc,link)			( (DObjLType(doc,link)==SYNCtype) )
#define DMeasureTYPE(doc,link)		( (DObjLType(doc,link)==MEASUREtype) )
#define DPSMeasTYPE(doc,link)		( (DObjLType(doc,link)==PSMEAStype) )

#define DLinkLPAGE(doc,link)		( (DGetPPAGE(doc,link))->lPage )
#define DLinkRPAGE(doc,link)		( (DGetPPAGE(doc,link))->rPage )
#define DSysPAGE(doc,link)			( (DGetPSYSTEM(doc,link))->pageL )
#define DLinkLSYS(doc,link)			( (DGetPSYSTEM(doc,link))->lSystem )
#define DLinkRSYS(doc,link)			( (DGetPSYSTEM(doc,link))->rSystem )
#define DStaffSYS(doc,link)			( (DGetPSTAFF(doc,link))->systemL )
#define DLinkLSTAFF(doc,link)		( (DGetPSTAFF(doc,link))->lStaff )
#define DLinkRSTAFF(doc,link)		( (DGetPSTAFF(doc,link))->rStaff )
#define DConnectSTAFF(doc,link)		( (DGetPCONNECT(doc,link))->staffL )
#define DMeasSYSL(doc,link)			( (DGetPMEASURE(doc,link))->systemL )
#define DMeasSTAFFL(doc,link)		( (DGetPMEASURE(doc,link))->staffL )
#define DLinkLMEAS(doc,link)		( (DGetPMEASURE(doc,link))->lMeasure )
#define DLinkRMEAS(doc,link)		( (DGetPMEASURE(doc,link))->rMeasure )

#define DNoteINCHORD(doc,link)		( (DGetPANOTE(doc,link))->inChord )
#define DNoteYD(doc,link)			( (DGetPANOTE(doc,link))->yd )
#define DNoteYSTEM(doc,link)		( (DGetPANOTE(doc,link))->ystem )

#define DDynamFIRSTSYNC(doc,link)	( (DGetPDYNAMIC(doc,link))->firstSyncL )
#define DDynamLASTSYNC(doc,link)	( (DGetPDYNAMIC(doc,link))->lastSyncL )
#define DGraphicFIRSTOBJ(doc,link)	( (DGetPGRAPHIC(doc,link))->firstObj )
#define DGraphicLASTOBJ(doc,link)	( (DGetPGRAPHIC(doc,link))->lastObj )
#define DTempoFIRSTOBJ(doc,link)	( (DGetPTEMPO(doc,link))->firstObjL )
#define DEndingFIRSTOBJ(doc,link)	( (DGetPENDING(doc,link))->firstObjL )
#define DEndingLASTOBJ(doc,link)	( (DGetPENDING(doc,link))->lastObjL )

#define DMainNote(doc,link)		(!DNoteINCHORD(doc,link) || DNoteYD(doc,link)!=DNoteYSTEM(doc,link))

/* ----------------------------------------------------------------------------------- */
/*
 * To facilitate debugging, define a macro version of LinkToPtr and versions of some
 *	basic macros that call it. This was useful long ago because the THINK C Debugger
 *	refused to evaluate expressions that call the LinkToPtr function because it didn't
 *	know there are no possible side effects; I doubt it's useful these days, but we'll
 *	keep them for now.  --DAB, Feb. 2017
 */

#define _LinkToPtr(heap,link)	( ((char *)(*(heap)->block)) + ((heap)->objSize*(unsigned long)(link)) )

#define _NextLink(heap,link)	( *(LINK *)_LinkToPtr(heap,link) )

#define _RightLINK(link)		( *(LINK *)_LinkToPtr(OBJheap,link) )
#define _LeftLINK(link)			( *(LINK *)(sizeof(LINK) + _LinkToPtr(OBJheap,link)) )
#define _FirstSubLINK(link)		( *(LINK *)((2*sizeof(LINK)) + _LinkToPtr(OBJheap,link)) )
#define _LinkXD(link)			( *(DDIST *)((3*sizeof(LINK)) + _LinkToPtr(OBJheap,link)) )
#define _LinkYD(link)			( *(DDIST *)((4*sizeof(LINK)) + _LinkToPtr(OBJheap,link)) )

#define _NoteSTAFF(link) 		( *(SignedByte *)(sizeof(LINK) + _LinkToPtr(NOTEheap,link)) )


/* ----------------------------------------------------------------------------------- */
/* Macros for acccessing various other fields of subobjects (mostly) or objects. (FWIW,
170 of these are from the OMRAS "MemMacroizing" project, mostly by Steve Hart.) */

#define FirstSubObjPtr(p,link)		(p = GetPMEVENT(link), p->firstSubObj)			

#define BeamCrossSTAFF(link)		( (GetPBEAMSET(link))->crossStaff )			
#define BeamCrossSYS(link)			( (GetPBEAMSET(link))->crossSystem )		
#define BeamFirstSYS(link)			( (GetPBEAMSET(link))->firstSystem )		
#define BeamRESTS(link)				( (GetPBEAMSET(link))->beamRests )	

#define ClefFILLER1(link)		 	( (GetPACLEF(link))->filler1)	
#define ClefFILLER2(link)		 	( (GetPACLEF(link))->filler2)	
#define ClefINMEAS(link)			( (GetPCLEF(link))->inMeasure )		
#define ClefSMALL(link)				( (GetPACLEF(link))->small )
#define ClefSOFT(link)				( (GetPACLEF(link))->soft)
#define ClefSTAFFN(link)			( (GetPACLEF(link))->staffn )	
#define ClefXD(link)				( (GetPACLEF(link))->xd )	
#define ClefYD(link)		 	 	( (GetPACLEF(link))->yd )

#define ConnectCONNLEVEL(link)		( (GetPACONNECT(link))->connLevel )		
#define ConnectCONNTYPE(link)		( (GetPACONNECT(link))->connectType )		
#define ConnectFILLER(link)		 	( (GetPACONNECT(link))->filler)	
#define ConnectFIRSTPART(link)		( (GetPACONNECT(link))->firstPart)		
#define ConnectLASTPART(link)		( (GetPACONNECT(link))->lastPart)		
#define ConnectSTAFFABOVE(link)		( (GetPACONNECT(link))->staffAbove )		
#define ConnectSTAFFBELOW(link)		( (GetPACONNECT(link))->staffBelow )		
#define ConnectXD(link)				( (GetPACONNECT(link))->xd )
#define ConnectYD(link)				( (GetPACONNECT(link))->yd )

#define DynamCrossSYS(link)			( (GetPDYNAMIC(link))->crossSys )		
#define DynamFIRSTSYNC(link)		( (GetPDYNAMIC(link))->firstSyncL )			
#define DynamicENDXD(link)			( (GetPADYNAMIC(link))->endxd)	
#define DynamicENDYD(link)			( (GetPADYNAMIC(link))->endyd)	
#define DynamicMOUTHWIDTH(link)		( (GetPADYNAMIC(link))->mouthWidth)		
#define DynamicOTHERWIDTH(link)		( (GetPADYNAMIC(link))->otherWidth)		
#define DynamicSMALL(link)			( (GetPADYNAMIC(link))->small )	
#define DynamicSOFT(link)			( (GetPADYNAMIC(link))->soft)	
#define DynamicSTAFFN(link)			( (GetPADYNAMIC(link))->staffn )	
#define DynamicType(link)			( (GetPADYNAMIC(link))->subType)	
#define DynamicVIS(link)			( (GetPADYNAMIC(link))->visible )	
#define DynamicXD(link)				( (GetPADYNAMIC(link))->xd )	
#define DynamicYD(link)		 	 	( (GetPADYNAMIC(link))->yd )
#define DynamLASTSYNC(link)			( (GetPDYNAMIC(link))->lastSyncL )		
#define DynamFirstIsSYSTEM(pL)		( (SystemTYPE(DynamLASTSYNC(pL))) )			/* Boolean, not link */
#define DynamLastIsSYSTEM(pL)		( (MeasureTYPE(DynamFIRSTSYNC(pL))) )		/* Boolean, not link */
#define DynamSubType(link)			( (GetPADYNAMIC(link))->subType)	

#define EndingFIRSTOBJ(link)		( (GetPENDING(link))->firstObjL )			
#define EndingLASTOBJ(link)			( (GetPENDING(link))->lastObjL )		

#define FirstGraphicSTRING(link)	( (GetPAGRAPHIC(FirstSubLINK(link)))->strOffset )			
#define FirstMeasMEASURENUM(link)	( (GetPAMEASURE(FirstSubLINK(link)))->measureNum )			

#define GraceBEAM(link)				( (GetPBEAMSET(link))->grace )	

#define GraphicFIRSTOBJ(link)		( (GetPGRAPHIC(link))->firstObj )			
#define GraphicINFO(link)			( (GetPGRAPHIC(link))->info )
#define GraphicLASTOBJ(link)		( (GetPGRAPHIC(link))->lastObj )		
#define GraphicNEXT(link)			( (GetPAGRAPHIC(link))->next )
#define GraphicSTRING(link)			( (GetPAGRAPHIC(link))->strOffset )	

#define GRNoteACC(link)				( (GetPAGRNOTE(link))->accident )
#define GRNoteACCSOFT(link)			( (GetPAGRNOTE(link))->accSoft )	
#define GRNoteAPPEAR(link)		 	( (GetPAGRNOTE(link))->headShape)
#define GRNoteBEAMED(link)			( (GetPAGRNOTE(link))->beamed )	
#define GRNoteCOURTESYACC(link)		( (GetPAGRNOTE(link))->courtesyAcc )			
#define GRNoteDOUBLEDUR(link)		( (GetPAGRNOTE(link))->doubleDur )	
#define GRNoteFILLERN(link)			( (GetPAGRNOTE(link))->fillerN)
#define GRNoteFIRSTMOD(link)		( (GetPAGRNOTE(link))->firstMod )		
#define GRNoteINCHORD(link)			( (GetPAGRNOTE(link))->inChord )	
#define GRNoteINOTTAVA(link)		( (GetPAGRNOTE(link))->inOttava )		
#define GRNoteINTUPLET(link)		( (GetPAGRNOTE(link))->inTuplet )	
#define GRNoteMICROPITCH(link)		( (GetPAGRNOTE(link))->micropitch)	
#define GRNoteNDOTS(link)	 		( (GetPAGRNOTE(link))->ndots )
#define GRNoteNUM(link)				( (GetPAGRNOTE(link))->noteNum )
#define GRNoteOFFVELOCITY(link)	 	( (GetPAGRNOTE(link))->offVelocity )	
#define GRNoteONVELOCITY(link)	 	( (GetPAGRNOTE(link))->onVelocity )	
#define GRNoteOtherStemSide(link)	( (GetPAGRNOTE(link))->otherStemSide )		
#define GRNotePLAYDUR(link)		 	( (GetPAGRNOTE(link))->playDur )
#define GRNotePLAYTIMEDELTA(link)	( (GetPAGRNOTE(link))->playTimeDelta)		
#define GRNotePTIME(link)			( (GetPAGRNOTE(link))->pTime ) 
#define GRNoteREST(link)			( (GetPAGRNOTE(link))->rest )
#define GRNoteSLURREDL(link)		( (GetPAGRNOTE(link))->slurredL )	
#define GRNoteSLURREDR(link)		( (GetPAGRNOTE(link))->slurredR )	
#define GRNoteSMALL(link)			( (GetPAGRNOTE(link))->small ) 
#define GRNoteSOFT(link)			( (GetPAGRNOTE(link))->soft)
#define GRNoteSTAFFN(link) 			( (GetPAGRNOTE(link))->staffn )
#define GRNoteSUBTYPE(link)			( (GetPAGRNOTE(link))->subType )	
#define GRNoteTEMPFLAG(link)	 	( (GetPAGRNOTE(link))->tempFlag)	
#define GRNoteTIEDL(link)	 		( (GetPAGRNOTE(link))->tiedL )	
#define GRNoteTIEDR(link)		 	( (GetPAGRNOTE(link))->tiedR )	
#define GRNoteUNPITCHED(link)		( (GetPAGRNOTE(link))->unpitched)		
#define GRNoteVIS(link)				( (GetPAGRNOTE(link))->visible )
#define GRNoteXD(link)		 		( (GetPAGRNOTE(link))->xd )	
#define GRNoteXMOVEACC(link)		( (GetPAGRNOTE(link))->xmoveAcc )			
#define GRNoteXMOVEDOTS(link)	 	( (GetPAGRNOTE(link))->xmovedots )		
#define GRNoteYD(link)				( (GetPAGRNOTE(link))->yd )	
#define GRNoteYMOVEDOTS(link)	 	( (GetPAGRNOTE(link))->ymovedots )		
#define GRNoteYQPIT(link)			( (GetPAGRNOTE(link))->yqpit )		
#define GRNoteYSTEM(link)			( (GetPAGRNOTE(link))->ystem )		

#define KeySigFILLER1(link)		 	( (GetPAKEYSIG(link))->filler1)	
#define KeySigFILLER2(link)			( (GetPAKEYSIG(link))->filler2)	

#define KeySigINMEAS(link)			( (GetPKEYSIG(link))->inMeasure )		
#define KeySigKSITEM(link)			( (GetPAKEYSIG(link))->KSItem)	
#define KeySigNKSITEMS(link)		( (GetPAKEYSIG(link))->nKSItems ) 		
#define KeySigNONSTANDARD(link)		( (GetPAKEYSIG(link))->nonstandard)	
#define KeySigSMALL(link)		 	( (GetPAKEYSIG(link))->small)
#define KeySigSOFT(link)			( (GetPAKEYSIG(link))->soft)
#define KeySigSTAFFN(link)			( (GetPAKEYSIG(link))->staffn )
#define KeySigType(link)			( (GetPAKEYSIG(link))->subType )
#define KeySigXD(link)		 		( (GetPAKEYSIG(link))->xd )

#define LinkLMEAS(link)				( (GetPMEASURE(link))->lMeasure )
#define LinkLPAGE(link)				( (GetPPAGE(link))->lPage )
#define LinkLSTAFF(link)			( (GetPSTAFF(link))->lStaff )	
#define LinkLSYS(link)				( (GetPSYSTEM(link))->lSystem )
#define LinkNENTRIES(link)			( (GetPMEVENT(link))->nEntries )	
#define LinkOBJRECT(link)			( (GetPMEVENT(link))->objRect )	
#define LinkRMEAS(link)				( (GetPMEASURE(link))->rMeasure )
#define LinkRPAGE(link)				( (GetPPAGE(link))->rPage )
#define LinkRSTAFF(link)			( (GetPSTAFF(link))->rStaff )	
#define LinkRSYS(link)				( (GetPSYSTEM(link))->rSystem )
#define LinkSEL(link)				( (GetPMEVENT(link))->selected )	
#define LinkSOFT(link)				( (GetPMEVENT(link))->soft )	
#define LinkSPAREFLAG(link)			( (GetPMEVENT(link))->spareFlag )		
#define LinkTWEAKED(link)			( (GetPMEVENT(link))->tweaked )		
#define LinkVALID(link)				( (GetPMEVENT(link))->valid )	
#define LinkVIS(link)				( (GetPMEVENT(link))->visible )	

#define MeasCLEFTYPE(link)	 		( (GetPAMEASURE(link))->clefType )		
#define MeasCONNABOVE(link)			( (GetPAMEASURE(link))->connAbove )		
#define MeasCONNSTAFF(link)			( (GetPAMEASURE(link))->connStaff )		
#define MeasDENOM(link)				( (GetPAMEASURE(link))->denominator)
#define MeasDynamType(link)			( (GetPAMEASURE(link))->dynamicType )	
#define MeasFILLER1(link)		 	( (GetPAMEASURE(link))->filler1)	
#define MeasFILLER2(link)		 	( (GetPAMEASURE(link))->filler2)	
#define MeasISFAKE(link)			( (GetPMEASURE(link))->fakeMeas )		
#define MeasKSITEM(link)			( (GetPAMEASURE(link))->KSItem)	
#define MeasMEASURENUM(link)		( (GetPAMEASURE(link))->measureNum )		
#define MeasMRECT(link)				( (GetPAMEASURE(link)->measSizeRect) )	
#define MeasNKSITEMS(link)			( (GetPAMEASURE(link))->nKSItems )	
#define MeasNUMER(link)				( (GetPAMEASURE(link))->numerator )
#define MeasOLDFAKEMEAS(link)		( (GetPAMEASURE(link))->oldFakeMeas)		
#define MeasPAGE(link)				( (SysPAGE(MeasSYSL(link))) )	
#define MeasSOFT(link)				( (GetPAMEASURE(link))->soft)
#define MeasSTAFFL(link)			( (GetPMEASURE(link))->staffL )			
#define MeasSUBTYPE(link)	 		( (GetPAMEASURE(link))->subType )		
#define MeasSYSL(link)				( (GetPMEASURE(link))->systemL )	
#define MeasTIMESIGTYPE(link)		( (GetPAMEASURE(link))->timeSigType )		
#define MeasureBBOX(link)			( (GetPMEASURE(link))->measureBBox )			
#define MeasureMEASUREVIS(link)		( (GetPAMEASURE(link))->measureVisible )		
#define MeasureSTAFFN(link)			( (GetPAMEASURE(link))->staffn )	
#define MeasureTIME(link)			( (GetPMEASURE(link))->lTimeStamp )			
#define MeasXMNSTDOFFSET(link)		( (GetPAMEASURE(link))->xMNStdOffset )		
#define MeasYMNSTDOFFSET(link)		( (GetPAMEASURE(link))->yMNStdOffset )		

#define ModNRDATA(link)				( (GetPAMODNR(link))->data)
#define ModNRMODCODE(link)			( (GetPAMODNR(link))->modCode )	
#define ModNRSEL(link)				( (GetPAMODNR(link))->selected)
#define ModNRSOFT(link)				( (GetPAMODNR(link))->soft)
#define ModNRVIS(link)				( (GetPAMODNR(link))->visible)
#define ModNRXSTD(link)				( (GetPAMODNR(link))->xstd )
#define ModNRYSTDPIT(link)			( (GetPAMODNR(link))->ystdpit)	

#define NextGraphic(aGraphic)		( (GetPAGRAPHIC((aGraphic))->next )		

#define NoteACC(link)				( (GetPANOTE(link))->accident )	
#define NoteACCSOFT(link)			( (GetPANOTE(link))->accSoft )		
#define NoteAPPEAR(link)			( (GetPANOTE(link))->headShape )		
#define NoteBeamBPSYNC(link)		( (GetPANOTEBEAM(link))->bpSync )		
#define NoteBEAMED(link)			( (GetPANOTE(link))->beamed )		
#define NoteBeamFILLER(link)		( (GetPANOTEBEAM(link))->filler)		
#define NoteBeamFRACGOLEFT(link)	( (GetPANOTEBEAM(link))->fracGoLeft)		
#define NoteBeamFRACS(link)			( (GetPANOTEBEAM(link))->fracs)
#define NoteBeamSTARTEND(link)		( (GetPANOTEBEAM(link))->startend)	
#define NoteCOURTESYACC(link)		( (GetPANOTE(link))->courtesyAcc )		
#define NoteDOUBLEDUR(link)			( (GetPANOTE(link))->doubleDur )
#define NoteFILLERN(link)			( (GetPANOTE(link))->fillerN)
#define NoteFIRSTMOD(link)			( (GetPANOTE(link))->firstMod )	
#define NoteINCHORD(link)			( (GetPANOTE(link))->inChord )	
#define NoteINOTTAVA(link)			( (GetPANOTE(link))->inOttava )	
#define NoteINTUPLET(link)			( (GetPANOTE(link))->inTuplet )	
#define NoteMERGED(link)			( (GetPANOTE(link))->merged )	
#define NotePLAYASCUE(link)			( (GetPANOTE(link))->playAsCue)	
#define NoteMICROPITCH(link)		( (GetPANOTE(link))->micropitch)	
#define NoteNDOTS(link)		 		( (GetPANOTE(link))->ndots )
#define NoteNUM(link)				( (GetPANOTE(link))->noteNum )
#define NoteOFFVELOCITY(link)		( (GetPANOTE(link))->offVelocity)	
#define NoteONVELOCITY(link)	 	( (GetPANOTE(link))->onVelocity )	
#define NoteOtherStemSide(link)		( (GetPANOTE(link))->otherStemSide )		
#define NoteOttavaOPSYNC(link)		( (GetPANOTEOTTAVA(link))->opSync)		
#define NotePLAYDUR(link)			( (GetPANOTE(link))->playDur )		
#define NotePLAYTIMEDELTA(link)		( (GetPANOTE(link))->playTimeDelta)		
#define NotePTIME(link)				( (GetPANOTE(link))->pTime ) 
#define NoteREST(link)				( (GetPANOTE(link))->rest )	
#define NoteSLURREDL(link)			( (GetPANOTE(link))->slurredL )		
#define NoteSLURREDR(link)			( (GetPANOTE(link))->slurredR )		
#define NoteSMALL(link)				( (GetPANOTE(link))->small ) 
#define NoteSOFT(link)				( (GetPANOTE(link))->soft)
#define NoteSTAFFN(link) 			( (GetPANOTE(link))->staffn )	
#define NoteTEMPFLAG(link)	 		( (GetPANOTE(link))->tempFlag)	
#define NoteTIEDL(link)		 		( (GetPANOTE(link))->tiedL )	
#define NoteTIEDR(link)		 		( (GetPANOTE(link))->tiedR )	
#define NoteTupleTPSYNC(link)	 	( (GetPANOTETUPLE(link))->tpSync)		
#define NoteUNPITCHED(link)			( (GetPANOTE(link))->unpitched)	
#define NoteVIS(link)				( (GetPANOTE(link))->visible ) 
#define NoteXD(link)		 		( (GetPANOTE(link))->xd )	
#define NoteXMOVEACC(link)			( (GetPANOTE(link))->xmoveAcc )		
#define NoteXMOVEDOTS(link)			( (GetPANOTE(link))->xmovedots )		
#define NoteYD(link)				( (GetPANOTE(link))->yd )	
#define NoteYMOVEDOTS(link)			( (GetPANOTE(link))->ymovedots )		
#define NoteYQPIT(link)				( (GetPANOTE(link))->yqpit )	
#define NoteYSTEM(link)				( (GetPANOTE(link))->ystem )	

#define PSMeasCONNABOVE(link)		( (GetPAPSMEAS(link))->connAbove )			
#define PSMeasCONNSTAFF(link)		( (GetPAPSMEAS(link))->connStaff )			
#define PSMeasFILLER1(link)		 	( (GetPAPSMEAS(link))->filler1)	
#define PSMeasSOFT(link)			( (GetPAPSMEAS(link))->soft)	
#define PSMeasSTAFFN(link)			( (GetPAPSMEAS(link))->staffn )	

#define RptEndCONNABOVE(link)		( (GetPARPTEND(link))->connAbove )			
#define RptEndCONNSTAFF(link)		( (GetPARPTEND(link))->connStaff )			
#define RptEndENDRPT(link)			( (GetPRPTEND(link))->endRpt )		
#define RptEndFILLER(link)			( (GetPARPTEND(link))->filler)	
#define RptEndFIRSTOBJ(link)		( (GetPRPTEND(link))->firstObj )			
#define RptEndSTAFFN(link) 			( (GetPARPTEND(link))->staffn )	
#define RptEndSTARTRPT(link)		( (GetPRPTEND(link))->startRpt )			
#define RptEndType(link)			( (GetPARPTEND(link))->subType)	
#define RptEndVIS(link)				( (GetPARPTEND(link))->visible )

#define SheetNUM(link)				( (GetPPAGE(link))->sheetNum )	

#define SlurBOUNDS(link)			( (GetPASLUR(link))->bounds )	
#define SlurCrossSTAFF(link)		( (GetPSLUR(link))->crossStaff )			
#define SlurCrossSYS(link)			( (GetPSLUR(link))->crossSystem )		
#define SlurDASHED(link)			( (GetPASLUR(link))->dashed)	
#define SlurENDKNOT(link)			( (GetPASLUR(link))->endKnot)	
#define SlurENDPT(link)				( (GetPASLUR(link))->endPt)
#define SlurFILLER(link)			( (GetPASLUR(link))->filler)	
#define SlurFIRSTIND(link)			( (GetPASLUR(link))->firstInd)	
#define SlurFIRSTSYNC(link)			( (GetPSLUR(link))->firstSyncL )
	
/* SlurFirstIsSYSTEM considers the possibility of SlurLASTSYNC being non-existent: this can
   never occur in a normal valid object list, but it does inside RfmtSystems, and handling
   that condition makes it possible for FixCrossSysSlurs to use SlurFirstIsSYSTEM. */
 
#define _SlurFirstSYSTEM(pL)		( (SystemTYPE(SlurLASTSYNC(pL))) )			/* Boolean, not link */
#define SlurFirstIsSYSTEM(pL)		(SlurLASTSYNC(pL)? _SlurFirstSYSTEM(pL) : True)	/* Boolean, not link */		
#define SlurKNOT(link)				( (GetPASLUR(link))->seg.knot)
#define SlurLASTIND(link)			( (GetPASLUR(link))->lastInd)	
#define SlurLASTSYNC(link)	 		( (GetPSLUR(link))->lastSyncL )		
#define SlurLastIsSYSTEM(pL)	 	( (MeasureTYPE(SlurFIRSTSYNC(pL))) )		/* Boolean, not link */
#define SlurRESERVED(link)			( (GetPASLUR(link))->reserved)	
#define SlurSEG(link)				( (GetPASLUR(link))->seg)
#define SlurSOFT(link)				( (GetPASLUR(link))->soft)
#define SlurSTARTPT(link)			( (GetPASLUR(link))->startPt)	
#define SlurTIE(link)				( (GetPSLUR(link))->tie )	
#define SlurVIS(link)				( (GetPASLUR(link))->visible)

#define StaffCLEFTYPE(link)			( (GetPASTAFF(link))->clefType )		
#define StaffDENOM(link)			( (GetPASTAFF(link))->denominator)	
#define StaffDynamType(link)		( (GetPASTAFF(link))->dynamicType )		
#define StaffFILLER(link)			( (GetPASTAFF(link))->filler)	
#define StaffFILLERSTF(link)		( (GetPASTAFF(link))->fillerStf)		
#define StaffFLAGLEADING(link)		( (GetPASTAFF(link))->flagLeading)		
#define StaffFONTSIZE(link)			( (GetPASTAFF(link))->fontSize)	
#define StaffFRACBEAMWIDTH(link)	( (GetPASTAFF(link))->fracBeamWidth)			
#define StaffHEIGHT(link)			( (GetPASTAFF(link))->staffHeight )		
#define StaffKSITEM(link)	 		( (GetPASTAFF(link))->KSItem )		
#define StaffLEDGERWIDTH(link)		( (GetPASTAFF(link))->ledgerWidth)		
#define StaffLEFT(link)				( (GetPASTAFF(link))->staffLeft )	
#define StaffMINSTEMFREE(link)		( (GetPASTAFF(link))->minStemFree)		
#define StaffNKSITEMS(link)			( (GetPASTAFF(link))->nKSItems )	
#define StaffNOTEHEADWIDTH(link)	( (GetPASTAFF(link))->noteHeadWidth)			
#define StaffNUM(link)				( (GetPASTAFF(link))->numerator )
#define StaffRIGHT(link)			( (GetPASTAFF(link))->staffRight )		
#define StaffSHOWLEDGERS(link)		( (GetPASTAFF(link))->showLedgers)		
#define StaffSHOWLINES(link)		( (GetPASTAFF(link))->showLines)		
#define StaffSPACEBELOW(link)		( (GetPASTAFF(link))->spaceBelow)		
#define StaffSTAFFLINES(link)		( (GetPASTAFF(link))->staffLines )		
#define StaffSTAFFN(link)			( (GetPASTAFF(link))->staffn )
#define StaffSYS(link)				( (GetPSTAFF(link))->systemL )
#define StaffTIMESIGTYPE(link)		( (GetPASTAFF(link))->timeSigType )	
#define StaffTOP(link)				( (GetPASTAFF(link))->staffTop )

#define SysRectBOTTOM(link)			( SystemRECT(link).bottom )	
#define SysRectLEFT(link)			( SystemRECT(link).left )	
#define SysRectRIGHT(link)			( SystemRECT(link).right )	
#define SysRectTOP(link)			( SystemRECT(link).top )	

#define SystemNUM(link)				( (GetPSYSTEM(link))->systemNum )
#define SysPAGE(link)				( (GetPSYSTEM(link))->pageL )
#define SystemRECT(link)			( (GetPSYSTEM(link))->systemRect )	

#define TempoFIRSTOBJ(link)			( (GetPTEMPO(link))->firstObjL )	
#define TempoMETROSTR(link)			( (GetPTEMPO(link))->metroStrOffset )	
#define TempoNOMM(link)				( (GetPTEMPO(link))->noMM )
#define TempoSTRING(link)	 		( (GetPTEMPO(link))->strOffset )	

#define TimeSigCONNSTAFF(link)		( (GetPATIMESIG(link))->connStaff)	
#define TimeSigDENOM(link)			( (GetPATIMESIG(link))->denominator)	
#define TimeSigFILLER(link)			( (GetPATIMESIG(link))->filler)	
#define TimeSigINMEAS(link)			( (GetPTIMESIG(link))->inMeasure )		
#define TimeSigNUMER(link)			( (GetPATIMESIG(link))->numerator )	
#define TimeSigSMALL(link)			( (GetPATIMESIG(link))->small)	
#define TimeSigSOFT(link)			( (GetPATIMESIG(link))->soft)	
#define TimeSigSTAFFN(link)			( (GetPATIMESIG(link))->staffn )	
#define TimeSigType(link)			( (GetPATIMESIG(link))->subType )	
#define TimeSigXD(link)		 		( (GetPATIMESIG(link))->xd )	
#define TimeSigYD(link)		  		( (GetPATIMESIG(link))->yd )				

#endif /* MemMacrosIncluded */
