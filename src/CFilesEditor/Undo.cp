/* Undo.c for Nightingale.
Routines in this module are intended to be called before undoable operations
to prepare for possible undoing, before non-undoable operations to reset the
machinery, and when user selects the Undo command to undo the previous operation.
*/

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2017 by Avian Music Notation Foundation. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

enum {
	TO_UNDO,
	TO_REDO,		/* no longer used */
	TO_SCORE
};

static void ToggleUndo(Document *doc);

static Boolean CheckCrossSys(LINK, LINK);
static Boolean CheckClipUndo(Document *, short);
static Boolean CheckBeamUndo(Document *, short);
static Boolean CheckInsertUndo(Document *);
static Boolean CheckSlurBeamUndo(Document *, LINK);
static Boolean CheckInfoUndo(Document *, LINK);
static Boolean CheckSetUndo(Document *doc);
static long ModNRListSize(LINK aModNRL);
static long GetRangeMemAlloc(LINK startL, LINK endL);
static Boolean UndoChkMemory(Document *doc, LINK sysL, LINK lastL);
static void UndoDeselRange(Document *, LINK, LINK);
static LINK UndoLastObjInSys(Document *, LINK);
static LINK UndoGetStartSys(Document *doc);
static void GetUndoRange(Document *, LINK, LINK *, LINK *, short);
static Boolean CopyUndoRange(Document *, LINK, LINK, LINK, short);
static void SwapSystems(Document *, LINK, LINK);


/* Theory of Operation:

When any Nightingale function wants to perform an undoable operation, it first calls
PrepareUndo, passing the U_xxx code for the operation. PrepareUndo saves one or more
entire Systems of the object list. Then, if the user later asks to have the operation
undone, we call DoUndo, which in turn calls SwapSystems, which swaps the undo object
list with the equivalent portion of the document's main object list. This both undoes
the operation and puts us in a position where the next call to DoUndo, again calling
SwapSystems, will redo the operation -- which is exactly what we want.

There are two problems with this approach:
- For very simple operations, PrepareUndo is very inefficient, especially since most
  operations are never undone. We used to avoid it simply by disabling undo for many
  simple operations, but computers are fast enough these days (April 2017) that this
  isn't much of a problem. FIXME: We still disable undo for dragging; this should
  be changed.
- Many operations can affect timestamps to the end of the score, potentially many
  pages away, but nothing else beyond the end of the page. For these, we could save
  everything to the end of the score in all cases, but that'd be very inefficient; or
  (beyond the end of the page) we could save just timestamps, but that's fairly messy.
  For now, we sidestep the problem by having SwapSystems recompute timestamps to the
  end of the score. The main problem with this is that it's not guaranteed to restore
  timestamps to the values they had before the original operation, though there should
  be problems only in wierd cases where Nightingale really doesn't understand the
  rhythm.
*/


/* Update doc's undo flag. */

static void ToggleUndo(Document *doc)
{
	doc->undo.redo = !doc->undo.redo;					/* Next iteration in cycle */
}


/* Set up fields in the document undo record. command is an UNDO_OPERATION code;
menuItem is a string. Ordinarily, this is called by higher-level Undo routines; it
should very rarely be called from outside of the Undo package. */

void SetupUndo(Document *doc, short command, char *menuItem)
{	
	doc->undo.lastCommand = command;

	strcpy(doc->undo.menuItem, menuItem);
}


/* Return True if the range contains any cross-system beamsets, dynamics or slurs. */

static Boolean CheckCrossSys(LINK startL, LINK endL)
{
	LINK pL; 

	for (pL=startL; pL!=endL; pL=RightLINK(pL))
		switch (ObjLType(pL)) {
			case BEAMSETtype:
				if (BeamCrossSYS(pL)) return True;
				break;
			case DYNAMtype:
				if (DynamCrossSYS(pL)) return True;
				break;
			case SLURtype:
				if (SlurCrossSYS(pL)) return True;
				break;
		}

	return False;
}


/* Check selRange and clipboard for cross-system objects; if either contains any, return
False to indicate the clipboard operation will not be undoable. */

static Boolean CheckClipUndo(Document *doc, short /*theCommand*/)
{
	Document *saveDoc; Boolean ok=True;
	
	saveDoc = currentDoc;

	if (CheckCrossSys(doc->selStartL, doc->selEndL))
		return False;
		
	InstallDoc(clipboard);
	if (CheckCrossSys(clipboard->headL, clipboard->tailL))
		ok = False;

	InstallDoc(saveDoc);
	
	return ok;
}


/* Check to see if the beaming operation is undoable. If the command is a Beam command,
and the selStart and selEnd links are not in the same system, then a cross-system beam
is being created; for now, this is not undoable. If the command in an Unbeam command,
then check the range to see if any of the beamsets are cross-system; unbeaming cross-
system beamsets is not currently undoable. Return False indicates the operation won't
be undoable. */

static Boolean CheckBeamUndo(Document *doc, short theCommand)
{
	LINK pL;

	if (theCommand==U_Beam)
		return (SameSystem(doc->selStartL, LeftLINK(doc->selEndL)));
	if (theCommand==U_Unbeam)
		for (pL=doc->selStartL; pL!=doc->selEndL; pL = RightLINK(pL))
			if (BeamsetTYPE(pL))
				if (BeamCrossSYS(pL)) return False;

	return True;
}


/* Check to see if the insertion operation can be undone; if so, return True, else
False. If the selStart link is before the first measure, as will be the case with the
insertion of a page-relative Graphic, then there is no current system to replace, and
the undo operation must be handled specially; for now, it is not undoable. */

static Boolean CheckInsertUndo(Document *doc)
{
	if (BeforeFirstMeas(doc->selStartL)) return False;
	return True;
}


/* Check to see if the operation of editing the beam or slur can be undone; if so,
return True, else False. If the object is cross-system, the edit cannot be undone. */

static Boolean CheckSlurBeamUndo(Document */*doc*/, LINK changeL)
{
	if (SlurTYPE(changeL))
		if (SlurCrossSYS(changeL)) return False;

	if (BeamsetTYPE(changeL))
		if (BeamCrossSYS(changeL)) return False;

	return True;
}


/* Check if the Get Info operation can be undone; if so, return True, else False.
If the link is a page-relative Graphic, then there is no current system to replace,
and the undo operation must be handled specially; for now, it is not undoable. */

static Boolean CheckInfoUndo(Document */*doc*/, LINK changeL)
{	
	if (GraphicTYPE(changeL)) {
		if (!GraphicFIRSTOBJ(changeL)) return False;
		if (PageTYPE(GraphicFIRSTOBJ(changeL))) return False;
	}
	
	return True;
}


/* Check if the Set operation can be undone; if so, return True, else False. We can't
undo if a page-relative Graphic is involved. */

static Boolean CheckSetUndo(Document *doc)
{
	LINK pL;
	
	for (pL=doc->selStartL; pL!=doc->selEndL; pL=RightLINK(pL))
		if (GraphicTYPE(pL) && PageTYPE(GraphicFIRSTOBJ(pL)))
			return False;

	return True;
}


static long ModNRListSize(LINK aModNRL)
{
	long listSize=0L;
	
	for ( ; aModNRL; aModNRL = NextMODNRL(aModNRL))
		listSize += objLength[MODNRtype];
		
	return listSize;
}


#define EXTRA_MEM 64000

static long GetRangeMemAlloc(LINK startL, LINK endL)
{
	long memAlloc=0L;
	LINK pL, aNoteL, aGRNoteL;
	Str255 string;
	PTEMPO pTempo;

	for (pL=startL; pL!=endL; pL=RightLINK(pL)) {
		memAlloc += objLength[OBJtype];
		memAlloc += LinkNENTRIES(pL) * subObjLength[ObjLType(pL)];
		
		switch (ObjLType(pL)) {
			case SYNCtype:
				aNoteL = FirstSubLINK(pL);
				if (NoteFIRSTMOD(aNoteL)) {
					memAlloc += ModNRListSize(NoteFIRSTMOD(aNoteL));
				}
				break;
			case GRSYNCtype:
				aGRNoteL = FirstSubLINK(pL);
				if (GRNoteFIRSTMOD(aGRNoteL)) {
					memAlloc += ModNRListSize(GRNoteFIRSTMOD(aGRNoteL));
				}
				break;
			case GRAPHICtype:
				Pstrcpy(string, (StringPtr)PCopy(GetPAGRAPHIC(FirstSubLINK(pL))->strOffset));
				memAlloc += Pstrlen(string);
				break;
			case TEMPOtype:
				pTempo = GetPTEMPO(pL);
				Pstrcpy(string, (StringPtr)PCopy(pTempo->strOffset));
				memAlloc += Pstrlen(string);
				pTempo = GetPTEMPO(pL);
				Pstrcpy(string, (StringPtr)PCopy(pTempo->metroStrOffset));
				memAlloc += Pstrlen(string);
				break;
			default:
				break;
		}
	}
	
	return memAlloc;
}


/* Check whether enough memory is available. Return True if so, False if not. (We check
by calling Apple's FreeMem(). Comments on FreeMem say "On Mac OS X, this function always
returns a large value, because virtual memory is always available to fulfill any request
for memory. This function is deprecated on Mac OS X and later. You can assume that any
reasonable memory allocation will succeed." So this function should probably always
return True. --DAB, May 2020) */

static Boolean UndoChkMemory(Document *doc, LINK firstL, LINK lastL)
{
	long memNeeded=0L, memFreed=0L, totalFree;
	
	memNeeded = GetRangeMemAlloc(firstL, lastL);
	
	if (doc->undo.hasUndo)
		memFreed = GetRangeMemAlloc(RightLINK(doc->undo.headL), doc->undo.tailL);
	
	memNeeded -= memFreed;
	
	if (memNeeded < 0) return True;

	totalFree = FreeMem();
	
	return (memNeeded + EXTRA_MEM <= totalFree);
}


/* Disable the Undo command. */

void DisableUndo(Document *doc, 
					Boolean /*memoryProblem*/)		/* no longer used */
{
	SetupUndo(doc, U_NoOp, "");

	doc->undo.canUndo = False;

	/* If necessary, delete a system from the undo record */
	if (doc->undo.hasUndo) {
		DeleteRange(doc, RightLINK(doc->undo.headL), doc->undo.tailL);
		doc->undo.hasUndo = False;
	}
}


/* Deselect all nodes in the range [startL, endL). */

static void UndoDeselRange(Document */*doc*/, LINK startL, LINK endL)
{
	LINK pL;

	for (pL = startL; pL!=endL; pL = RightLINK(pL))
		if (LinkSEL(pL))				/* We need LinkSEL bcs DeselectNode can't deselect Pages, etc. */
			DeselectNode(pL);
}

/* Return the last Object of pL's System. If pL is a System, return the last object
of the previous system. */

static LINK UndoLastObjInSys(Document *doc, LINK pL)
{
	LINK endSysL;
	
	endSysL = EndSystemSearch(doc, pL);
	return (LeftLINK(endSysL));
}


/* Get the starting system for the undo object list for most operations, i.e., the
system which contains the insertion point. If the insertion point is at the end of a
system, doc->selStartL will be the following system or page object or the tail;
UndoGetStartSys must handle this state of affairs. */

static LINK UndoGetStartSys(Document *doc)
{
	LINK startL, sysL;

	if (PageTYPE(doc->selStartL))
		startL = LeftLINK(doc->selStartL);
	else if (SystemTYPE(doc->selStartL)) {
		if (FirstSysInPage(doc->selStartL))
			startL = LeftLINK(SysPAGE(doc->selStartL));		
		else
			startL = LeftLINK(doc->selStartL);		
	}
	else
		startL = doc->selStartL;

	sysL = SSearch(startL, SYSTEMtype, GO_LEFT);
	if (!sysL) sysL = SSearch(doc->headL, SYSTEMtype, GO_RIGHT);
		
	return sysL;
}


/* Find the last object in the last System in the score that could possibly be
affected by Record Merge starting at the insertion point. This is determined by
the fact that the longest possible recording is MAX_SAFE_MEASDUR ticks. */

static LINK UndoGetLastMergePoint(Document *);
static LINK UndoGetLastMergePoint(Document *doc)
{
	LINK startSyncL, extremeEndL, measL, lastL;
	long startTime, maxEndTime;
	
	startSyncL = SSearch(doc->selStartL, SYNCtype, GO_RIGHT);
	if (!startSyncL) { extremeEndL = doc->selStartL;  goto Finish; }

	startTime = SyncAbsTime(startSyncL);
	maxEndTime = startTime+MAX_SAFE_MEASDUR;
	measL = SSearch(doc->selStartL, MEASUREtype, GO_RIGHT);
	while (measL && MeasureTIME(measL)<maxEndTime)
		measL = LinkRMEAS(measL);
	extremeEndL = (measL? measL : doc->tailL);

Finish:
	lastL = UndoLastObjInSys(doc, (extremeEndL? extremeEndL : doc->tailL));
	return lastL;
}


/* Get range of nodes for systems affected by <theCommand>. For most commands, the
range is obtained from the selection range; for some, we use <startChangeL> to get the
range affected. */

static void GetUndoRange(
					Document *doc,
					LINK startChangeL,
					LINK *sysL, LINK *lastL,			/* Range: a System, last obj in a System */
					short theCommand
					)
{
	LINK pL, endSysL;

	switch (theCommand) {

		/* There is nothing to undo for these operations. */
		
		case U_NoOp:
		case U_Copy:
		case U_LeftEnd:
			*sysL = *lastL = NILINK;
			return;

		/* Specify two systems or some global range (not yet implemented) */
		
		case U_BreakSystems:
		case U_GlobalText:
		case U_MeasNum:
			*sysL = *lastL = NILINK;
			return;

		/* Specify two systems or some global range */
		
		case U_MoveMeasUp:
			pL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, GO_LEFT, False);
			*sysL = LinkLSYS(pL);				/* Must exist, otherwise couldn't move meas up. */
			*lastL = UndoLastObjInSys(doc, doc->selEndL);
			return;
		case U_MoveMeasDown:
			*sysL = UndoGetStartSys(doc);
			pL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, GO_RIGHT, False);
			if (pL==NILINK)	*lastL = LeftLINK(doc->tailL);
			else			*lastL = UndoLastObjInSys(doc, RightLINK(pL));
			return;
		case U_MoveSysUp:
			pL = UndoGetStartSys(doc);
			*sysL = LinkLSYS(pL);
			pL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_RIGHT, False);
			if (pL==NILINK) *lastL = LeftLINK(doc->tailL);
			else			*lastL = LeftLINK(pL);
			return;
		case U_MoveSysDown:
			pL = UndoGetStartSys(doc);			/* pL can't be NILINK, since we don't allow moving the only system on a page. */
			*sysL = LinkLSYS(pL);
			pL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_RIGHT, False);
			if (pL==NILINK)						/* This can't happen now, but could if we let Move Sys Down add a page. */
				*lastL = LeftLINK(doc->tailL);
			else {
				pL = LinkRPAGE(pL);
				if (pL==NILINK)	*lastL = LeftLINK(doc->tailL);
				else			*lastL = LeftLINK(pL);
			}
			return;
		
		/* <startChangeL> is the same as that passed to AddSystem: the page, system, or
		   tail before which we insert the new system. We save everything from the system
		   before <startChangeL> to the end of the score: potentially grossly inefficient,
		   but the score might've been reformatted to the end during the process of adding
		   the system. -JGG */
			
		case U_AddSystem:
			*sysL = SSearch(startChangeL, SYSTEMtype, GO_LEFT);
			*lastL = LeftLINK(doc->tailL);
			return;

		/* Either the page added or no range (not yet implemented) */
		
		case U_AddPage:
			*sysL = *lastL = NILINK;
			return;
		
		/* The entire score. */
		
		case U_TranspKey:
		case U_AddCautionaryTS:
			*sysL = SSearch(doc->headL, SYSTEMtype, GO_RIGHT);
			*lastL = LeftLINK(doc->tailL);
			return;
		
		/* Possibly discontinuous selection: affects all Systems in selection range */
		
		case U_Set:
		case U_Respace:
		case U_SetDuration:
		case U_Transpose:
		case U_DiatTransp:
		case U_Respell:
		case U_DelRedundAcc:
		case U_AddRedundAcc:
		case U_Dynamics:
		case U_AutoBeam:
		case U_Beam:
		case U_Unbeam:
		case U_Tuple:
		case U_Untuple:
		case U_Ottava:
		case U_UnOttava:
		case U_AddMods:
		case U_StripMods:
		case U_MultiVoice:
		case U_FlipSlurs:
		case U_Quantize:
		case U_Justify:
		case U_TapBarlines:
		case U_Reformat:
		case U_CompactVoice:
		case U_Double:
		case U_FillEmptyMeas:
			*sysL = UndoGetStartSys(doc);
			endSysL = SSearch(doc->selEndL, SYSTEMtype, GO_LEFT);
			*lastL = UndoLastObjInSys(doc, endSysL);
			return;

		/* Save the rest of the page, so that we can put systems back in the right place
		   graphically. Also, if there are any cross-system objects, save previous and
		   following systems. Note that Copy System should have insured that the Clipboard
		   doesn't have any cross-system objects. Still, must check destination range. */
			
		case U_PasteSystem:
			{
				LINK curSysL, rightSysL, rightPageL;

				curSysL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, GO_LEFT, False);
				rightPageL = LSSearch(doc->selStartL, PAGEtype, ANYONE, GO_RIGHT, False);
				if (rightPageL==NILINK) rightPageL = doc->tailL;
				rightSysL = LSSearch(doc->selStartL, SYSTEMtype, ANYONE, GO_RIGHT, False);
				if (rightSysL==NILINK) rightSysL = doc->tailL;
				if (CheckCrossSys(curSysL, rightSysL)) {
					LINK prevSysL = LinkLSYS(curSysL);
					*sysL = prevSysL? prevSysL : curSysL;

					if (IsAfter(rightPageL, rightSysL)) {
						LINK nextSysL = LinkRSYS(rightSysL);
						*lastL = nextSysL? LeftLINK(nextSysL) : LeftLINK(doc->tailL);
					}
					else
						*lastL = LeftLINK(rightPageL);
				}
				else {
					*sysL = curSysL;
					*lastL = LeftLINK(rightPageL);
				}
			}
			return;

		/* Continuous selection: affects only selStartL's System */
		
		case U_Paste:
		case U_Merge:
		case U_Insert:
		case U_Record:
		case U_EditText:
			*sysL = UndoGetStartSys(doc);
			*lastL = UndoLastObjInSys(doc,*sysL);
			return;

		/* Affects selStartL's System, as well as all following Systems that would
		   be affected by undoing removal of a clef or keysig change. */
			
		case U_Cut:
		case U_Clear:
			{
				LINK clefL, ksL;

				*sysL = UndoGetStartSys(doc);
				clefL = ClefSearch(doc, startChangeL, ANYONE, True, True);
				ksL = KSSearch(doc, startChangeL, ANYONE, True, True);
				pL = IsAfter(clefL, ksL)? ksL : clefL;
				*lastL = UndoLastObjInSys(doc, pL);
			}
			return;
		case U_InsertClef:
			*sysL = UndoGetStartSys(doc);
			pL = ClefSearch(doc, startChangeL, doc->undo.param1, True, False);
			*lastL = UndoLastObjInSys(doc, pL);
			return;
		case U_InsertKeySig:
			*sysL = UndoGetStartSys(doc);
			pL = KSSearch(doc, startChangeL, doc->undo.param1, True, False);
			*lastL = UndoLastObjInSys(doc,pL);
			return;

		case U_EditBeam:
		case U_EditSlur:
		case U_GetInfo:
			*sysL = SSearch(startChangeL, SYSTEMtype, GO_LEFT);
			*lastL = UndoLastObjInSys(doc,*sysL);
			return;

		/* Continuous selection, but can affect well past selEndL. */
		
		case U_RecordMerge:
			*sysL = UndoGetStartSys(doc);
			*lastL = UndoGetLastMergePoint(doc);
			return;
		
		/* Directly affect Systems or Pages: effects can extend well past sel. range */
		
		case U_ClearSystem:
			{
				LINK thisSysL, nextPageL;
				
				/* To undo Clear System, we have to save Systems starting with the one
				   preceding the one to be cleared. All following Systems on its page
				   will be moved up, so we have to save all following Systems to the end
				   of the page. */
				    
				thisSysL = UndoGetStartSys(doc);
				*sysL = LinkLSYS(thisSysL);			/* Must exist: you can't clear the 1st System */
				nextPageL = SSearch(thisSysL, PAGEtype, GO_RIGHT);
				*lastL = UndoLastObjInSys(doc, (nextPageL? LeftLINK(nextPageL) : doc->tailL));
			}
			return;
		case U_ClearPages:
			{
				LINK nextSysL;
				
				/* To undo Clear Pages, we have to save the range from the last System
				   before the range of Pages thru the first System following. Important:
				   we assume that, by the time we get here, doc->selStartL and selEndL
				   have been set to Pages delimiting the appropriate range! */
				   
				*sysL = SSearch(doc->selStartL, SYSTEMtype, GO_LEFT);	/* Must exist: you can't clear the 1st Page */
				nextSysL = SSearch(doc->selEndL, SYSTEMtype, GO_RIGHT);
				*lastL = UndoLastObjInSys(doc, (nextSysL? nextSysL : doc->tailL));
			}
			return;

		default:
			LogPrintf(LOG_INFO, "GetUndoRange: unknown command code %d\n", theCommand);
	}
}


/* Copy range from srcStartL to srcEndL into object list at insertL. All copying is
done from heaps in doc to heaps in doc; the nodes from the undo system are all
allocated from the heaps for that document. Return True if successful, False is
there's a problem. */

static Boolean CopyUndoRange(Document *doc, LINK srcStartL, LINK srcEndL, LINK insertL,
										short toRange)
{
	LINK pL, prevL, copyL, initL;
	short i, numObjs;
	COPYMAP *copyMap;

	initL = prevL = LeftLINK(insertL);
	SetupCopyMap(srcStartL, srcEndL, &copyMap, &numObjs);

	for (i=0, pL=srcStartL; pL!=srcEndL; i++, pL=RightLINK(pL)) {
		copyL = DuplicateObject(ObjLType(pL), pL, False, doc, doc, True);
		if (!copyL) {
			doc->undo.hasUndo = True;				/* Get rid of what was copied into undo dataStr */
			DisableUndo(doc, True);
			return False; 							/* Memory error or some other problem */
		}

		RightLINK(copyL) = insertL;
		LeftLINK(insertL) = copyL;
		RightLINK(prevL) = copyL;
		LeftLINK(copyL) = prevL;

		copyMap[i].srcL = pL;	copyMap[i].dstL = copyL;
		prevL = copyL;

		/*
		 * If selStartL or selEndL are in the range to be copied, set the corresponding
		 * selStartL or selEndL LINK in the other object list.
		 * Since the selRange ptrs must be a matched pair, if we encounter both or neither,
		 * we are OK, but if we encounter one without the other, we must figure out a way
		 * to maintain consistency.
		 * As of now, CopyUndoRange is called only once, by PrepareUndo, to copy the range
		 * from the score into the undo object list. Therefore, we will only attempt to
		 * maintain consistency for copying from score to undo.
		 * doc->undo.selStartL and doc->undo.selEndL are both initialized to NILINK before
		 * calling this routine; therefore, if selStartL is NILINK, set it to the headL; if
		 * selEndL is NILINK, set it to the tail.
		 */
		switch (toRange) {
			case TO_UNDO:
				if (pL==doc->selStartL) doc->undo.selStartL = copyL;
				if (pL==doc->selEndL) doc->undo.selEndL = copyL;
				break;
			case TO_SCORE:
				if (pL==doc->undo.selStartL) doc->selStartL = copyL;
				if (pL==doc->undo.selEndL) doc->selEndL = copyL;
				break;
		}
  	}
  	if (doc->undo.selStartL==NILINK)
  		doc->undo.selStartL = doc->undo.headL;
   if (doc->undo.selEndL==NILINK)
  		doc->undo.selEndL = doc->undo.tailL;
  	/* Do something here with doc->selStartL, doc->selEndL. */
 	
	FixCrossLinks(doc, doc, RightLINK(initL), insertL);
	CopyFixLinks(doc, doc, RightLINK(initL), insertL, copyMap, numObjs);
	DisposePtr((Ptr)copyMap);
	
	return True;		/* Successful copy */
}


/* Set up the document's undo record before performing an undoable operation. */

void PrepareUndo(
			Document *doc,
			LINK startChangeL,
			short theCommand,				/* U_ code for the operation */
			short stringInd					/* Index into the Undo stringlist */
			)
{
	LINK sysL=NILINK, lastL=NILINK, prevSysL;
	Boolean cantUndo=False;
	char menuItem[256];						/* Undo menu command, excluding "Undo"/"Redo" (C string) */

	if (config.disableUndo) {
		DisableUndo(doc, False);
		return;
	}

	GetIndCString(menuItem, UNDO_STRS, stringInd);
	
	/* Verify that the operation is undoable for <theCommand> in the current situation;
	   if not, disable the undo command and return. */
	   
	switch (theCommand) {
		case U_Cut:
		case U_Paste:
		case U_Merge:
		case U_Clear:
			if (!CheckClipUndo(doc, theCommand)) cantUndo = True;
			break;
		case U_Beam:
		case U_Unbeam:
			if (!CheckBeamUndo(doc, theCommand)) cantUndo = True;
			break;
		case U_Insert:
			if (!CheckInsertUndo(doc)) cantUndo = True;
			break;
		case U_EditBeam:
		case U_EditSlur:
			if (!CheckSlurBeamUndo(doc, startChangeL)) cantUndo = True;
			break;
		case U_GetInfo:
			if (!CheckInfoUndo(doc, startChangeL)) cantUndo = True;
			break;
		case U_Set:
			if (!CheckSetUndo(doc)) cantUndo = True;
			break;
		default:
			;
	}
	
	if (cantUndo) {
		DisableUndo(doc, False);
		return;
	}
	
	/* Prepare for the undo operation here. */
	
	switch (theCommand) {
		case U_NoOp:
		case U_Copy:
			/* There is nothing to undo for these operations. */
			return;
			
		case U_Set:
		case U_ChangeTight:
		case U_Respace:
		case U_Justify:
		case U_Transpose:
		case U_DiatTransp:
		case U_TranspKey:
		case U_Respell:
		case U_DelRedundAcc:
		case U_AddRedundAcc:
		case U_Dynamics:
		case U_AutoBeam:
		case U_Cut:
		case U_Paste:
		case U_Merge:
		case U_Clear:
		case U_SetDuration:
		case U_Beam:
		case U_Unbeam:
		case U_Tuple:
		case U_Untuple:
		case U_Ottava:
		case U_UnOttava:
		case U_AddMods:
		case U_StripMods:
		case U_MultiVoice:
		case U_FlipSlurs:
		case U_Quantize:
		case U_Insert:
		case U_InsertClef:
		case U_InsertKeySig:
		case U_GetInfo:
		case U_TapBarlines:
		case U_Reformat:
		case U_Record:
		case U_RecordMerge:
		case U_CompactVoice:
		case U_Double:
		case U_ClearSystem:
		case U_ClearPages:
		case U_EditBeam:
		case U_EditSlur:
		case U_AddSystem:
		case U_MoveMeasUp:
		case U_MoveMeasDown:
		case U_MoveSysUp:
		case U_MoveSysDown:
		case U_PasteSystem:
		case U_FillEmptyMeas:
		case U_AddCautionaryTS:
		case U_EditText:

			/* Set up fields for the command that's about to be performed. */
			
			SetupUndo(doc, theCommand, menuItem);
			
			/* Get the range of objects we need to save in case we later have to undo
			   the operation. scorePrevL is the link before the first system; scoreEndL
			   is the first object following the last object in the last system. */
			   
			GetUndoRange(doc, startChangeL, &sysL, &lastL, theCommand);
			prevSysL = LeftLINK(sysL);
			
			if (!UndoChkMemory(doc, prevSysL, lastL)) {
				DisableUndo(doc, True);
				break;
			}

			doc->undo.scorePrevL = prevSysL;
			doc->undo.scoreEndL = RightLINK(lastL);
			doc->undo.insertL = lastL;

			/* If necessary, delete the range from the undo record. */
			
			if (doc->undo.hasUndo)
				DeleteRange(doc, RightLINK(doc->undo.headL), doc->undo.tailL);
			doc->undo.selStartL = doc->undo.selEndL = NILINK;
		
			/* Copy the range from the score into the undo record. */
			
			if (CopyUndoRange(doc, prevSysL, RightLINK(lastL), doc->undo.tailL, TO_UNDO)) {
				doc->undo.hasUndo = True;
				doc->undo.redo = False;
				doc->undo.canUndo = True;
			}
			break;
			
		case U_LeftEnd:
		case U_GlobalText:
		case U_MeasNum:
		case U_AddPage:
		case U_BreakSystems:
			/* Cannot handle undo for these operations. */
			
			DisableUndo(doc, False);
			break;

		default:
			LogPrintf(LOG_WARNING, "PrepareUndo: unknown command code %d; stringInd=%d\n",
							theCommand, stringInd);
			DisableUndo(doc, False);			
	}
}


/* Swap the given range of systems for the range in the undo object list, where:
1. the range in the score before the swap begins with prevSysL, the left link of the
	starting system, and ends with lastL;
2. the objects bounding the range in the score, e.g., the objects before prevSysL and
	after lastL, are prevL and afterL;
3. undoPrevSysL is the node in the undo object list corresponding to prevSysL,
	undoLastL corresponds to lastL. */

static void SwapSystems(
					Document *doc,
					LINK prevSysL, LINK lastL	/* links enclosing a range of systems */
					)
{
	LINK pL, startL, undoPrevSysL, undoLastL, prevL, afterL, firstSysMeas;
	
	undoPrevSysL = RightLINK(doc->undo.headL);
	undoLastL = LeftLINK(doc->undo.tailL);

	prevL = LeftLINK(prevSysL);
	afterL = RightLINK(lastL);

	/* Link prevSysL into the undo object list */
	RightLINK(doc->undo.headL) = prevSysL;
	LeftLINK(prevSysL) = doc->undo.headL;
	
	/* Link the undo prevSystem LINK into the score object list */
	RightLINK(prevL) = undoPrevSysL;
	LeftLINK(undoPrevSysL) = prevL;
	
	/* Link the last LINK into the undo object list */
	RightLINK(lastL) = doc->undo.tailL;
	LeftLINK(doc->undo.tailL) = lastL;
	
	/* Link the undo last LINK into the score object list */
	RightLINK(undoLastL) = afterL;
	LeftLINK(afterL) = undoLastL;
	
	/* Update structure for system in score (just swapped in from undo) */
	FixStructureLinks(doc, doc, doc->headL, doc->tailL);

	/* Update structure for system in undo (just swapped in from score) */
	FixStructureLinks(doc, doc, doc->undo.headL, doc->undo.tailL);

	/* The undo range includes one link before the swapped System: this is necessitated
		by Reformat and by operations which trigger reformatting because these
		operations can replace the System itself. We now update any crosslinks that
		refer to that object. */

	startL = LSSearch(undoPrevSysL, SYSTEMtype, ANYONE, GO_LEFT, False);
	if (!startL) startL = doc->headL;
	
	for (pL=startL; pL!=undoPrevSysL; pL=RightLINK(pL)) {
		switch (ObjLType(pL)) {
			case GRAPHICtype:
				if (GraphicFIRSTOBJ(pL)==prevSysL)
					GraphicFIRSTOBJ(pL) = undoPrevSysL;
				if (GraphicSubType(pL)==GRDraw)
					if (GraphicLASTOBJ(pL)==prevSysL)
						GraphicLASTOBJ(pL) = undoPrevSysL;
				break;
			case TEMPOtype:
				if (TempoFIRSTOBJ(pL)==prevSysL)
					TempoFIRSTOBJ(pL) = undoPrevSysL;
				break;
			case ENDINGtype:
				if (EndingFIRSTOBJ(pL)==prevSysL)
					EndingFIRSTOBJ(pL) = undoPrevSysL;
				if (EndingLASTOBJ(pL)==prevSysL)
					EndingLASTOBJ(pL) = undoPrevSysL;
				break;
			case DYNAMtype:
				if (DynamFIRSTSYNC(pL)==prevSysL)
					DynamFIRSTSYNC(pL) = undoPrevSysL;
				if (DynamLASTSYNC(pL)==prevSysL)
					DynamLASTSYNC(pL) = undoPrevSysL;
				break;
			case SLURtype:
				if (SlurFIRSTSYNC(pL)==prevSysL)
					SlurFIRSTSYNC(pL) = undoPrevSysL;
				if (SlurLASTSYNC(pL)==prevSysL)
					SlurLASTSYNC(pL) = undoPrevSysL;
				break;
			case RPTENDtype:
				if (RptEndFIRSTOBJ(pL)==prevSysL)
					RptEndFIRSTOBJ(pL) = undoPrevSysL;
				break;
		}
	}

	FixExtCrossLinks(doc, doc, startL, undoPrevSysL);

	/* If the command being undone could have created or destroyed Pages, Systems,
	   or Measures, update numbers of all of these. Also recompute everything about
	   the screen view and redraw all of it. */

	switch (doc->undo.lastCommand) {
		case U_Quantize:
		case U_Respace:
		case U_Reformat:
		case U_Record:
		case U_RecordMerge:
		case U_ClearSystem:
		case U_ClearPages:
		case U_AddSystem:
		case U_MoveMeasUp:
		case U_MoveMeasDown:
		case U_MoveSysUp:
		case U_MoveSysDown:
		case U_PasteSystem:
			UpdatePageNums(doc);
			UpdateSysNums(doc, doc->headL);
			UpdateMeasNums(doc, NILINK);

			GetAllSheets(doc);
			RecomputeView(doc);
			InvalWindow(doc);

			break;
		default:
			;
	}

	/* If the command being undone could have affected timestamps at all, it could
		have affected them to the end of the score, so recompute them. This should
		work in almost all cases, but it's not perfect: see comments under Theory
		of Operation. */
	switch (doc->undo.lastCommand) {
		case U_Cut:		
		case U_Paste:
		case U_PasteSystem:
		case U_Merge:
		case U_Clear:
		case U_Insert:
		case U_Record:
		case U_ClearSystem:
		case U_ClearPages:
		case U_SetDuration:
		case U_AddSystem:				/* ?? Does Add System affect timestamps?  -JGG */
			FixTimeStamps(doc, undoPrevSysL, undoLastL);
			break;
		default:
			;
	}

	/* Fix up selection flags and the selection range. */
	
	UndoDeselRange(doc, doc->undo.headL, doc->undo.tailL);
	DeselRange(doc, doc->headL, doc->tailL);
	firstSysMeas = LSSearch(RightLINK(undoPrevSysL), MEASUREtype, ANYONE, GO_RIGHT, False);
	doc->selStartL = doc->selEndL = RightLINK(firstSysMeas);	
	
	/* ??It certainly seems like we should be setting undo.selStartL and undo.selEndL,
	   but not doing so doesn't seem to cause any problems. ?? */
}


/* Handle the Undo command. */

void DoUndo(Document *doc)
{
	LINK tempStartL, tempEndL;

	if (!doc->undo.canUndo) {
		MayErrMsg("DoUndo: canUndo is False.");
		return;
	}
	
	InvalSystems(RightLINK(doc->undo.scorePrevL), doc->undo.scoreEndL);

	tempStartL = RightLINK(doc->undo.headL);						/* Range to be swapped in */
	tempEndL = LeftLINK(doc->undo.tailL);

	SwapSystems(doc, doc->undo.scorePrevL, LeftLINK(doc->undo.scoreEndL));	/* Swap systems in score w/those in Undo */

	doc->undo.scorePrevL = tempStartL;								/* Will now be swapped out */
	doc->undo.scoreEndL = RightLINK(tempEndL);
	doc->undo.insertL = LeftLINK(doc->undo.scoreEndL);				/* insertL is now last obj in system */

	InvalSystems(RightLINK(doc->undo.scorePrevL), doc->undo.scoreEndL);

	ToggleUndo(doc); 												/* Next iteration in cycle */
	MEAdjustCaret(doc, True);
}
