/***************************************************************************************

	FMTypes.h
	
	Data types used by FreeMIDI
	
	Created 10/6/94 by Chris Majoros
	
	Copyright © 1994-1997 Mark of the Unicorn, Inc.

***************************************************************************************/

#ifndef	__FMTypes__
#define	__FMTypes__

#ifndef	__Types__
// MAS old header?
//#include	<Types.h>
#endif

#ifdef GENERATINGCFM
#undef GENERATINGCFM
#define GENERATINGCFM 0
#endif

#ifdef	__cplusplus
extern "C" {
#endif

typedef unsigned short	fmsUniqueID;

// selection list filter proc
typedef Boolean		(*FreeMIDIFilterProcPtr)(struct menuSelectionRec *infoP, struct listEntry *entryP, long refCon);

enum {
	uppFMFilterProcInfo = kThinkCStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(Boolean)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(Ptr)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Ptr)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long)))
};

#if GENERATINGCFM
typedef UniversalProcPtr	FMFilterProcUPP;

#define	CallFMFilterProc(proc, infoP, entryP, refCon)		\
			CallUniversalProc((UniversalProcPtr)(proc), uppFMFilterProcInfo, (infoP), (entryP), (refCon))
#define	NewFMFilterProc(proc)						\
			(FMFilterProcUPP)NewRoutineDescriptor((ProcPtr)(proc), uppFMFilterProcInfo, GetCurrentISA())
#else
typedef FreeMIDIFilterProcPtr	FMFilterProcUPP;

#define	CallFMFilterProc(proc, infoP, entryP, refCon)		\
			(*(proc))((infoP), (entryP), (refCon))
#define	NewFMFilterProc(proc)						\
			(FMFilterProcUPP)(proc)
#endif

// custom selection list actions
typedef Boolean		(*FreeMIDISelectionProcPtr)(struct selectionListRec **listH, struct menuSelectionRec *infoP,
				long selectionID, short selectionInfo, long selectionParam);
typedef Boolean		(*FreeMIDICompareProcPtr)(struct menuSelectionRec *infoP, long selectionID, short selectionInfo, long compareParam);

enum {
	uppFMSelectionProcInfo = kThinkCStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(Boolean)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(Ptr)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Ptr)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long)))
		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(short)))
		| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(long))),
	uppFMCompareProcInfo = kThinkCStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(Boolean)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(Ptr)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(short)))
		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(long)))
};

#if GENERATINGCFM
typedef UniversalProcPtr	FMSelectionProcUPP;

#define	CallFMSelectionProc(proc, listH, infoP, selectionID, selectionInfo, selectionParam)		\
			CallUniversalProc((UniversalProcPtr)(proc), uppFMFilterProcInfo, (listH), (infoP), (selectionID), (selectionInfo), (selectionParam))
#define	NewFMSelectionProc(proc)												\
			(FMSelectionProcUPP)NewRoutineDescriptor((ProcPtr)(proc), uppFMSelectionProcInfo, GetCurrentISA())

typedef UniversalProcPtr	FMCompareProcUPP;

#define	CallFMCompareProc(proc, infoP, selectionID, selectionInfo, compareParam)		\
			CallUniversalProc((UniversalProcPtr)(proc), uppFMCompareProcInfo, (infoP), (selectionID), (selectionInfo), (compareParam))
#define	NewFMCompareProc(proc)											\
			(FMCompareProcUPP)NewRoutineDescriptor((ProcPtr)(proc), uppFMCompareProcInfo, GetCurrentISA())
#else
typedef FreeMIDISelectionProcPtr	FMSelectionProcUPP;

#define	CallFMSelectionProc(proc, listH, infoP, selectionID, selectionInfo, selectionParam)		\
			(*(proc))((listH), (infoP), (selectionID), (selectionInfo), (selectionParam))
#define	NewFMSelectionProc(proc)												\
			(FMSelectionProcUPP)(proc)

typedef FreeMIDICompareProcPtr	FMCompareProcUPP;

#define	CallFMCompareProc(proc, infoP, selectionID, selectionInfo, compareParam)		\
			(*(proc))((infoP), (selectionID), (selectionInfo), (compareParam))
#define	NewFMCompareProc(proc)											\
			(FMCompareProcUPP)(proc)
#endif

/* FreeMIDI Timing Services */

typedef pascal void (*FMSTMNativeCallback) (struct FMSTMTask*);		// PowerPC only!
typedef void (*FMSTMPollFunction) (void);						// PowerPC only

#ifdef	__cplusplus
}
#endif

#endif	/* __FMTypes__ */

/************************************** end FMTypes.h ************************************/